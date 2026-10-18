// Exercises production JSON parsing, frame growth, palette mapping and WIC loading.
// No game process or engine-address calls are used by these test cases.
#include "../pch.h"
#include "../ArtifactData.cpp"
#include "../ArtifactImages.cpp"
#include "../ArtifactSave.h"
#include "../ArtifactTablesManager.cpp"
#include <map>
#include <set>
#include <cassert>
#include <cstdlib>
#include <iostream>

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
static std::map<std::string, std::string> translations;
namespace Era
{
static void *__stdcall TestRealAddr(void *address)
{
    return address;
}
void *(__stdcall *GetRealAddr)(void *) = TestRealAddr;
void (__stdcall *RedirectMemoryBlock)(void *, int, void *) = nullptr;
int32_bool (__stdcall *PcxPngExists)(const char *) = nullptr;
char *tr(const char *key)
{
    // ERA translations are permanent. Keep values alive across language changes.
    static std::set<std::string> strings;
    auto it = translations.find(key);
    return it == translations.end() ? const_cast<char *>(key) :
           const_cast<char *>(strings.insert(it->second).first->c_str());
}

void RuntimeLayoutTests()
{
    // Exercise the production predicate without depending on free memory at
    // fixed game addresses. Executable operands are verified separately.
    using Tables = artifacts::ArtifactTablesManager;
    assert(Tables::CompatibleLayout(144, 0x63E758, 0x63E998, 144 * 4, true, true));
    // WoG expands ARTEVENT to 171, and combination bonus enumeration to 160.
    const DWORD start = 0x7B8300, end = start + 160 * 4;
    assert(Tables::CompatibleLayout(171, start, end, 171 * 4, true, true));
    assert(!Tables::CompatibleLayout(171, start, end, 1024 * 4, true, true));
    assert(!Tables::CompatibleLayout(171, start, end, 171 * 4, false, true));
    assert(!Tables::CompatibleLayout(171, start, end, 171 * 4, true, false));
    assert(!Tables::CompatibleLayout(171, start, start + 161 * 4, 171 * 4, true, true));
    assert(!Tables::CompatibleLayout(143, start, end, 144 * 4, true, true));
    assert(!Tables::CompatibleLayout(1025, start, end, 171 * 4, true, true));
    assert(!Tables::CompatibleLayout(171, start, start, 171 * 4, true, true));
    assert(!Tables::CompatibleLayout(171, start, end + 1, 171 * 4, true, true));
    assert(!Tables::CompatibleLayout(171, start, end, 171 * 4 + 1, true, true));
    assert(!Tables::CompatibleLayout(171, start, end, 143 * 4, true, true));
}
}

void JsonTests()
{
    using namespace artifacts;
    H3ArtifactSetup setup;
    setup.cost = 750;
    setup.position = static_cast<eArtifactPositions>(1);
    setup.type = static_cast<eArtifactType>(2);
    setup.name = "original";
    signed char bonuses[] = {1, 2, 3, 4};
    LPCSTR event = "native event";
    ArtifactData definition;
    definition.Apply(setup, bonuses, event, 171);
    assert(setup.cost == 750 && bonuses[0] == 1 && std::string(event) == "native event");
    translations = {
        {"era.artifacts.171.cost", "0"}, {"era.artifacts.171.position", "65"},
        {"era.artifacts.171.type", "32"}, {"era.artifacts.171.disabled", "2"},
        {"era.artifacts.171.primary.attack", "-128"}, {"era.artifacts.171.primary.defence", "128"},
        {"era.artifacts.171.primary.spellPower", "1junk"}, {"era.artifacts.171.name", ""},
        {"era.artifacts.171.image", "test.png"}, {"era.artifacts.171.imageFrame", "-1"},
        {"era.artifacts.171.ai.value", "2147483648"}
    };
    assert(ArtifactData::HasDefinition("era.artifacts.171"));
    assert(!ArtifactData::HasDefinition("era.artifacts.172"));
    definition.Apply(setup, bonuses, event, 171);
    assert(setup.cost == 0 && static_cast<int>(setup.position) == 1 && static_cast<int>(setup.type) == 2);
    assert(setup.disabled == 1 && bonuses[0] == -128 && bonuses[1] == 2 && bonuses[2] == 3);
    assert(!*setup.name && definition.image == "test.png" && definition.imageFrame == 0 && !definition.hasAiValue);
    definition.legacyKey = "mod.artifacts.0";
    translations = {{"mod.artifacts.0.cost", "100"}, {"era.artifacts.171.cost", "200"},
                    {"mod.artifacts.0.description", "legacy"}, {"era.artifacts.171.primary.attack", "0"}};
    definition.Apply(setup, bonuses, event, 171);
    assert(setup.cost == 200 && std::string(setup.description) == "legacy" && bonuses[0] == 0);
    translations = {{"era.artifacts.171.cost", "999"}, {"era.artifacts.171.name", "new language"},
                    {"era.artifacts.171.description", "translated"}};
    definition.ReloadText(setup, event, false, true, 171);
    assert(!*setup.name && setup.cost == 200 && std::string(setup.description) == "translated");
    translations.clear();
}

void FrameTests()
{
    struct Frame { int id; };
    struct Group { int count; unsigned spritesSize; Frame **frames; };
    Frame a{1}, b{2}, placeholder{0};
    Group group{2, 2, static_cast<Frame **>(std::malloc(2 * sizeof(Frame *)))};
    group.frames[0] = &a;
    group.frames[1] = &b;
    auto alloc = [](void *p, size_t n) { return std::realloc(p, n); };
    assert(artifacts::GrowArtifactFrames(group, 1024, &placeholder, alloc));
    assert(group.count == 1024 && group.frames[0] == &a && group.frames[1] == &b);
    for (int id = 2; id < 1024; ++id) assert(group.frames[id] == &placeholder);
    auto *before = group.frames;
    assert(artifacts::GrowArtifactFrames(group, 1024, &placeholder, alloc) && before == group.frames);
    std::free(group.frames);
    Group empty{0, 0, nullptr};
    assert(artifacts::GrowArtifactFrames(empty, 1, &placeholder, alloc) && empty.frames[0] == &placeholder);
    before = empty.frames;
    assert(!artifacts::GrowArtifactFrames(empty, 1024, &placeholder, [](void *, size_t) -> void * { return nullptr; }));
    assert(empty.frames == before && empty.count == 1 && empty.spritesSize == 1);
    std::free(empty.frames);
}

void ImageTests(const char *fixture)
{
    struct RGB { unsigned char r, g, b; };
    struct Palette { RGB color[256]{}; } palette;
    palette.color[1] = {255, 0, 0}; // Reserved DEF index must not be chosen.
    palette.color[8] = {250, 0, 0};
    palette.color[9] = {0, 255, 0};
    using artifacts::Pixel;
    assert(artifacts::ArtifactPaletteIndex(Pixel{0,0,255,255}, palette) == 8);
    assert(artifacts::ArtifactPaletteIndex(Pixel{255,255,0,255}, palette) == 0);
    assert(artifacts::ArtifactPaletteIndex(Pixel{0,0,255,0}, palette) == 0);
    artifacts::Image image;
    assert(artifacts::LoadBitmap(fixture, image));
    assert(image.width == 2 && image.height == 1);
    assert(image.pixels[0].r == 255 && image.pixels[0].a == 255);
    assert(image.pixels[1].a == 0);
    assert(!artifacts::LoadBitmap("missing-artifact-image.png", image));
}

void SaveValidationTests()
{
    artifacts::SavedArtifact record = {100, 1, 2, -1, -1, 0, 0, 0, 0, 0, 0};
    assert(artifacts::ValidSaved(record, true));
    record.position = 65;
    assert(!artifacts::ValidSaved(record, true));
    assert(artifacts::ValidSaved(record, false)); // WoG's old special positions are native-owned.
    record.mapUsed = 2;
    assert(!artifacts::ValidSaved(record, false));
    record.mapUsed = 0;
    record.nameVar = 1001;
    assert(!artifacts::ValidSaved(record, false));
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    Era::RuntimeLayoutTests();
    JsonTests();
    FrameTests();
    ImageTests(argv[1]);
    SaveValidationTests();
    std::cout << "PASS: JSON; DEF; palette/PNG alpha; save records; native and post-WoG runtime layouts\n";
}
