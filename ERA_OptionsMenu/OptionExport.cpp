#define NOMINMAX
#include <windows.h>
#include "OptionExport.h"
#include "../headers/json.hpp"

#include <algorithm>
#include <climits>
#include <stdexcept>

namespace era_options
{
namespace
{
std::wstring WideText(const std::string &text, unsigned codePage)
{
    if (text.empty()) return {};
    if (text.size() > INT_MAX) throw std::runtime_error("text is too long");
    const DWORD flags = codePage == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0;
    const int size = MultiByteToWideChar(codePage, flags, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (!size) throw std::runtime_error("invalid text encoding");
    std::wstring wide(size, L'\0');
    if (!MultiByteToWideChar(codePage, flags, text.data(), static_cast<int>(text.size()), &wide[0], size))
        throw std::runtime_error("text conversion failed");
    return wide;
}
}

std::string TextToUtf8(const std::string &text, unsigned codePage)
{
    const auto wide = WideText(text, codePage);
    if (wide.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (!size) throw std::runtime_error("UTF-8 conversion failed");
    std::string utf8(size, '\0');
    if (!WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), &utf8[0], size, nullptr, nullptr))
        throw std::runtime_error("UTF-8 conversion failed");
    return utf8;
}

std::string SerializeOptionsJson(const OptionRegistry &registry, unsigned codePage,
    const std::function<int(const EOption &)> &readValue)
{
    using Json = nlohmann::json;
    Json document = {{"era_options", Json::object()}};
    auto &mods = document["era_options"];
    std::vector<const EOption *> options;
    for (const auto &option : registry.Options()) options.push_back(option.get());
    std::sort(options.begin(), options.end(), [](const EOption *left, const EOption *right) {
        return left->GetId() < right->GetId();
    });
    for (const auto *option : options)
    {
        const auto &definition = option->Definition();
        auto &mod = mods[TextToUtf8(definition.modFolder, codePage)];
        mod["name"] = TextToUtf8(definition.modName, codePage);
        if (mod.find("options") == mod.end()) mod["options"] = Json::array();
        Json item = {{"key", option->GetKey()}, {"id", std::to_string(option->GetId())},
                     {"name", TextToUtf8(definition.name, codePage)}};
        const int value = readValue ? readValue(*option) : option->GetValue();
        if (value) item["default"] = std::to_string(value);
        if (definition.type == OptionType::Choice)
        {
            item["type"] = "choice";
            item["choices"] = Json::array();
            for (const auto &choice : definition.choices) item["choices"].push_back(TextToUtf8(choice, codePage));
        }
        for (const auto &field : std::vector<std::pair<const char *, const std::string *>>{
                 {"category", &definition.category}, {"page", &definition.page}, {"section", &definition.section},
                 {"hint", &definition.hint}, {"popup", &definition.popup}})
            if (!field.second->empty() && !(std::string(field.first) == "popup" && definition.popup == definition.hint))
                item[field.first] = TextToUtf8(*field.second, codePage);
        if (!definition.tags.empty())
        {
            item["tags"] = Json::array();
            for (const auto &tag : definition.tags) item["tags"].push_back(TextToUtf8(tag, codePage));
        }
        for (const bool required : {false, true})
        {
            const char *field = required ? "requires" : "disables";
            const auto append = [&](OptionId id, const std::string &tag, int when, int value) {
                Json record = id >= 0 ? Json{{"id", std::to_string(id)}} :
                    Json{{"tag", TextToUtf8(tag, codePage)}};
                if (when != 1) record["when"] = std::to_string(when);
                if (value != (required ? 1 : 0)) record["value"] = std::to_string(value);
                item[field].push_back(std::move(record));
            };
            if (required)
            {
                for (const auto &rule : definition.requires) append(rule.id, rule.tag, rule.when, rule.value);
            }
            else for (const auto &rule : definition.disables) append(rule.id, rule.tag, rule.when, rule.value);
        }
        if (!definition.enabled) item["enabled"] = "false";
        if (!definition.visible) item["visible"] = "false";
        if (definition.changeDuringGame) item["change_during_game"] = "true";
        if (definition.categoryOrder) item["category_order"] = std::to_string(definition.categoryOrder);
        if (definition.wogOption >= 0)
        {
            item["wog_option"] = std::to_string(definition.wogOption);
            if (!definition.wogValues.empty())
            {
                item["wog_values"] = Json::array();
                for (const auto value : definition.wogValues) item["wog_values"].push_back(std::to_string(value));
            }
        }
        mod["options"].push_back(std::move(item));
    }
    // A single exported file remains loadable from WoG with separate mod roots.
    if (mods.find("wog") != mods.end() && mods.size() > 1)
    {
        auto &included = mods["wog"]["include_mods"];
        included = Json::array();
        for (auto it = mods.begin(); it != mods.end(); ++it)
            if (it.key() != "wog") included.push_back(it.key());
    }
    return document.dump(2) + "\n";
}

bool WriteOptionsJson(const std::string &pathUtf8, const std::string &json, std::string &error)
{
    error.clear();
    std::wstring target;
    try { target = WideText(pathUtf8, CP_UTF8); }
    catch (const std::exception &exception) { error = exception.what(); return false; }
    if (target.empty() || target.find(L'\0') != std::wstring::npos)
    {
        error = "export path is empty or invalid";
        return false;
    }
    struct Output
    {
        HANDLE file = INVALID_HANDLE_VALUE;
        std::wstring path;
        bool created = false;
        ~Output()
        {
            if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
            if (created) DeleteFileW(path.c_str());
        }
    } output;
    static volatile LONG serial = 0;
    for (int attempt = 0; attempt < 16; ++attempt)
    {
        output.path = target + L".tmp." + std::to_wstring(GetCurrentProcessId()) + L"." +
            std::to_wstring(InterlockedIncrement(&serial));
        output.file = CreateFileW(output.path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (output.file != INVALID_HANDLE_VALUE || GetLastError() != ERROR_FILE_EXISTS) break;
    }
    const auto failed = [&](const char *action) {
        error = std::string(action) + " (Win32 error " + std::to_string(GetLastError()) + ")";
        return false;
    };
    if (output.file == INVALID_HANDLE_VALUE) return failed("cannot create export file");
    output.created = true;
    for (size_t offset = 0; offset < json.size(); )
    {
        DWORD written = 0;
        const DWORD count = static_cast<DWORD>((std::min)(json.size() - offset, static_cast<size_t>(MAXDWORD)));
        if (!WriteFile(output.file, json.data() + offset, count, &written, nullptr) || written != count)
            return failed("cannot write export file");
        offset += written;
    }
    if (!FlushFileBuffers(output.file)) return failed("cannot flush export file");
    CloseHandle(output.file);
    output.file = INVALID_HANDLE_VALUE;
    if (!MoveFileExW(output.path.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        return failed("cannot replace export file");
    output.created = false;
    return true;
}
} // namespace era_options
