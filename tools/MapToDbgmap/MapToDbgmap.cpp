#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::string trim(std::string value) {
    const auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };
    value.erase(value.begin(), std::find_if_not(value.begin(), value.end(), isSpace));
    value.erase(std::find_if_not(value.rbegin(), value.rend(), isSpace).base(), value.end());
    return value;
}

std::vector<std::string> splitColumns(const std::string& line) {
    std::vector<std::string> result;
    std::string column;
    for (const unsigned char ch : line) {
        if (ch == ' ') {
            if (!column.empty()) {
                result.push_back(std::move(column));
                column.clear();
            }
        } else {
            column.push_back(static_cast<char>(ch));
        }
    }
    if (!column.empty()) result.push_back(std::move(column));
    return result;
}

bool startsWithIgnoreCase(const std::string& value, const std::string& prefix) {
    if (value.size() < prefix.size()) return false;
    for (std::size_t i = 0; i < prefix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(value[i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i]))) return false;
    }
    return true;
}

class MapCompiler {
public:
    explicit MapCompiler(std::string source) {
        std::string line;
        for (const char ch : source) {
            if (ch == '\n') {
                lines_.push_back(trim(std::move(line)));
                line.clear();
            } else if (ch != '\r') {
                line.push_back(ch);
            }
        }
        lines_.push_back(trim(std::move(line)));
        while (!lines_.empty() && lines_.back().empty()) lines_.pop_back();
    }

    std::vector<std::uint8_t> compile() {
        processSections();

        writeU32(static_cast<std::uint32_t>(labels_.size()));
        for (const auto& [address, label] : labels_) {
            writeU32(address);
            writeString(label);
        }

        writeU32(static_cast<std::uint32_t>(modules_.size()));
        for (const auto& module : modules_) writeString(module);

        writeU32(static_cast<std::uint32_t>(lineNumbers_.size()));
        for (const auto& [address, line] : lineNumbers_) {
            writeU32(address);
            writeU32(line.first);
            writeU32(line.second);
        }
        return output_;
    }

private:
    struct Section {
        std::string id;
        std::uint32_t address = 0;
        std::uint32_t size = 0;
        std::string type;
    };

    [[noreturn]] void fail(const std::string& message) const {
        throw std::runtime_error(message + ". Error on line " + std::to_string(line_ + 1));
    }

    bool eof() const { return line_ >= lines_.size(); }

    std::string sectionName() const {
        if (eof() || lines_[line_].empty()) return {};
        const unsigned char first = static_cast<unsigned char>(lines_[line_][0]);
        return std::isalpha(first) ? lines_[line_] : std::string{};
    }

    bool isSection(const std::string& prefix = {}) const {
        const auto name = sectionName();
        return !name.empty() && (prefix.empty() || startsWithIgnoreCase(name, prefix));
    }

    bool nextLine() {
        ++line_;
        while (!eof() && lines_[line_].empty()) ++line_;
        return !eof();
    }

    void nextSection() {
        while (nextLine() && sectionName().empty()) {}
    }

    static std::uint32_t parseHex(std::string value) {
        if (!value.empty() && (value.back() == 'H' || value.back() == 'h')) value.pop_back();
        return static_cast<std::uint32_t>(std::stoull(value, nullptr, 16));
    }

    std::pair<std::string, std::string> complexField(const std::string& value) const {
        const auto separator = value.find(':');
        if (separator == std::string::npos) fail("Field '" + value + "' is not a valid segment:offset field");
        return {value.substr(0, separator), value.substr(separator + 1)};
    }

    Section* findSection(const std::string& id) {
        const auto found = sectionById_.find(id);
        return found == sectionById_.end() ? nullptr : &sections_[found->second];
    }

    std::uint32_t complexAddress(const std::string& value, std::string* segmentId = nullptr) {
        const auto [id, offset] = complexField(value);
        if (segmentId) *segmentId = id;
        const auto* section = findSection(id);
        return section ? section->address + parseHex(offset) : 0;
    }

    void processSections() {
        while (!eof()) {
            if (!isSection()) nextSection();
            else if (isSection("Start ")) processStartSection();
            else if (sections_.empty()) nextSection();
            else if (isSection("Detailed map of segments")) processDetailedSection();
            else if (isSection("Address ")) processAddressSection();
            else if (isSection("Line numbers for ")) processLineNumbersSection();
            else nextSection();
        }
    }

    void processStartSection() {
        if (!sections_.empty()) fail("Duplicate Start section");
        nextLine();
        while (!eof() && !isSection()) {
            const auto columns = splitColumns(lines_[line_]);
            if (columns.size() >= 2) {
                const auto length = parseHex(columns[1]);
                if (length > 0) {
                    const auto [id, offsetText] = complexField(columns[0]);
                    const auto offset = parseHex(offsetText);
                    auto* section = findSection(id);
                    if (!section) {
                        sectionById_[id] = sections_.size();
                        sections_.push_back({id, offset, length, columns.size() > 3 ? columns[3] : std::string{}});
                    } else if (offset > section->address) {
                        section->size += length;
                    }
                }
            }
            nextLine();
        }
        if (sections_.empty()) fail("Invalid Start section without segments");

        std::uint32_t previousAddress = 0x1000;
        std::uint32_t previousSize = 0;
        for (auto& section : sections_) {
            const auto address = previousAddress + previousSize;
            section.address = (address + 0xFFFu) & ~0xFFFu;
            previousAddress = section.address;
            previousSize = section.size;
        }
    }

    void processDetailedSection() {
        nextLine();
        while (!eof() && !isSection()) {
            const auto columns = splitColumns(lines_[line_]);
            if (!columns.empty()) {
                const auto module = std::find_if(columns.begin(), columns.end(), [](const std::string& value) {
                    return value.rfind("M=", 0) == 0;
                });
                if (module != columns.end()) {
                    std::string segmentId;
                    const auto address = complexAddress(columns[0], &segmentId);
                    if (address) {
                        const auto* section = findSection(segmentId);
                        labels_[address] = module->substr(2) + ":" +
                            (section && !section->type.empty() ? section->type : "MOD");
                    }
                }
            }
            nextLine();
        }
    }

    void processAddressSection() {
        nextLine();
        while (!eof() && !isSection()) {
            const auto columns = splitColumns(lines_[line_]);
            if (columns.size() >= 2) {
                const auto address = complexAddress(columns[0]);
                if (address) labels_[address] = columns[1];
            }
            nextLine();
        }
    }

    void processLineNumbersSection() {
        static const std::regex header(R"(^Line numbers for ([A-Za-z0-9_]+)(\(([^)]+)\))?)", std::regex::icase);
        std::smatch match;
        const auto name = sectionName();
        if (!std::regex_search(name, match, header)) {
            nextSection();
            return;
        }

        const std::string module = match[3].matched ? match[3].str() : match[1].str();
        auto moduleFound = moduleIndexes_.find(module);
        if (moduleFound == moduleIndexes_.end()) {
            const auto index = static_cast<std::uint32_t>(modules_.size());
            modules_.push_back(module);
            moduleFound = moduleIndexes_.emplace(module, index).first;
        }

        nextLine();
        while (!eof() && !isSection()) {
            const auto columns = splitColumns(lines_[line_]);
            for (std::size_t i = 0; i + 1 < columns.size(); i += 2) {
                const auto address = complexAddress(columns[i + 1]);
                if (address) lineNumbers_[address] = {moduleFound->second, static_cast<std::uint32_t>(std::stoul(columns[i]))};
            }
            nextLine();
        }
    }

    void writeU32(std::uint32_t value) {
        for (unsigned shift = 0; shift < 32; shift += 8) output_.push_back(static_cast<std::uint8_t>(value >> shift));
    }

    void writeString(const std::string& value) {
        writeU32(static_cast<std::uint32_t>(value.size()));
        output_.insert(output_.end(), value.begin(), value.end());
    }

    std::vector<std::string> lines_;
    std::size_t line_ = 0;
    std::vector<Section> sections_;
    std::unordered_map<std::string, std::size_t> sectionById_;
    std::map<std::uint32_t, std::string> labels_;
    std::vector<std::string> modules_;
    std::unordered_map<std::string, std::uint32_t> moduleIndexes_;
    std::map<std::uint32_t, std::pair<std::uint32_t, std::uint32_t>> lineNumbers_;
    std::vector<std::uint8_t> output_;
};

std::string readFile(const fs::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("Cannot open input map");
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

void writeFile(const fs::path& path, const std::vector<std::uint8_t>& data) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) throw std::runtime_error("Cannot create output dbgmap");
    stream.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!stream) throw std::runtime_error("Cannot write output dbgmap");
}

} // namespace

int wmain(int argc, wchar_t* argv[]) {
    if (argc != 3) {
        std::wcerr << L"Usage: MapToDbgmap.exe <input.map> <output.dbgmap>\n";
        return 2;
    }

    try {
        const fs::path input = argv[1];
        const fs::path output = argv[2];
        const auto compiled = MapCompiler(readFile(input)).compile();
        writeFile(output, compiled);
        std::wcout << L"Compiled " << input << L" -> " << output << L" (" << compiled.size() << L" bytes)\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "MapToDbgmap: " << error.what() << '\n';
        return 1;
    }
}
