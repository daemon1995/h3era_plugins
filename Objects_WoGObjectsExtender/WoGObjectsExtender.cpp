#include "WoGObjectsExtender.h"
#include "Artificer.h"
#include "MarketOfTime.h"
#include "WoGObjectHandlers.h"
#include "WoGObjectIds.h"
#include <algorithm>
#include <deque>
#include <set>
#include <string>
#include <vector>

namespace wog
{
namespace
{
struct EditableH3TextFile : H3TextFile
{
    void AddLine(LPCSTR line)
    {
        text.Add(line);
    }
    size_t LineCount() const noexcept
    {
        return text.Size();
    }
};

std::deque<std::string> clonedObjectProperties;

bool SplitProperty(const char *line, std::vector<std::pair<size_t, size_t>> &spans)
{
    spans.clear();
    if (!line)
        return false;
    const std::string text(line);
    size_t i = 0;
    while (i < text.size())
    {
        while (i < text.size() && (text[i] == ' ' || text[i] == '\t' || text[i] == '\r' || text[i] == '\n' ||
                                   text[i] == '\v' || text[i] == '\f'))
            ++i;
        if (i >= text.size())
            break;
        const size_t first = i;
        while (i < text.size() && !(text[i] == ' ' || text[i] == '\t' || text[i] == '\r' || text[i] == '\n' ||
                                    text[i] == '\v' || text[i] == '\f'))
            ++i;
        spans.push_back(std::make_pair(first, i));
        if (spans.size() >= 7)
            break;
    }
    return spans.size() >= 7;
}

int PropertyInt(const std::string &line, const std::pair<size_t, size_t> &span)
{
    return atoi(line.substr(span.first, span.second - span.first).c_str());
}

std::string PropertyKey(const std::string &line, const std::vector<std::pair<size_t, size_t>> &spans, const int type)
{
    const std::string def = line.substr(spans[0].first, spans[0].second - spans[0].first);
    const int subtype = PropertyInt(line, spans[6]);
    return def + "|" + std::to_string(type) + "|" + std::to_string(subtype);
}

_LHF_(CloneLegacyWoGObjectProperties)
{
    auto *base = *reinterpret_cast<H3TextFile **>(c->ebp + 0x8);
    auto *objectTxt = static_cast<EditableH3TextFile *>(base);
    if (!objectTxt)
        return EXEC_DEFAULT;

    const size_t initialCount = objectTxt->LineCount();
    std::set<std::string> existingEra;
    std::vector<std::pair<size_t, size_t>> spans;
    for (size_t i = 1; i < initialCount; ++i)
    {
        const char *raw = (*objectTxt)[i];
        std::string line = raw ? raw : "";
        if (!SplitProperty(line.c_str(), spans))
            continue;
        const int type = PropertyInt(line, spans[5]);
        const int subtype = PropertyInt(line, spans[6]);
        if (type == ERA_OBJECT_TYPE && objects::IsPortedSubtype(subtype))
            existingEra.insert(PropertyKey(line, spans, ERA_OBJECT_TYPE));
    }

    size_t added = 0;
    for (size_t i = 1; i < initialCount; ++i)
    {
        const char *raw = (*objectTxt)[i];
        std::string line = raw ? raw : "";
        if (!SplitProperty(line.c_str(), spans))
            continue;
        const int type = PropertyInt(line, spans[5]);
        const int subtype = PropertyInt(line, spans[6]);
        const auto *descriptor = objects::FindDescriptor(subtype);
        if (type != WOG_OBJECT_TYPE || !descriptor || !descriptor->cloneLegacyProperty)
            continue;
        const std::string eraKey = PropertyKey(line, spans, ERA_OBJECT_TYPE);
        if (existingEra.find(eraKey) != existingEra.end())
            continue;
        line.replace(spans[5].first, spans[5].second - spans[5].first, std::to_string(ERA_OBJECT_TYPE));
        clonedObjectProperties.push_back(line);
        objectTxt->AddLine(clonedObjectProperties.back().c_str());
        existingEra.insert(eraKey);
        ++added;
    }
    c->eax += static_cast<DWORD>(added);
    return EXEC_DEFAULT;
}

int SubObjectSubtype(const H3MapItem *mapItem) noexcept
{
    return mapItem && (mapItem->objectType == WOG_OBJECT_TYPE || mapItem->objectType == ERA_OBJECT_TYPE)
               ? mapItem->objectSubtype
               : -1;
}
} // namespace

std::vector<int> WoGObjectsExtender::WoGObjectOptionsIds;
WoGObjectsExtender *WoGObjectsExtender::instance = nullptr;

WoGObjectsExtender::WoGObjectsExtender() : ObjectExtender(_PI)
{
    CreatePatches();
}
void WoGObjectsExtender::AfterLoadingObjectsTxtProc(const INT16 *maxSubtypes) noexcept
{
    ReleaseObjectSubtypesInfo();
    int maxPortedSubtype = artificer::OBJECT_SUBTYPE;
    for (const auto *d = objects::DescriptorsBegin(); d != objects::DescriptorsEnd(); ++d)
        maxPortedSubtype = (std::max)(maxPortedSubtype, d->subtype);
    const int loadedMaxSubtype = maxSubtypes[WOG_OBJECT_TYPE];
    const int maxSubtype = (std::max)(loadedMaxSubtype, maxPortedSubtype);
    WoGObjectOptionsIds.assign(maxSubtype + 1, 0);
    objectSubtypesInfo.Reserve(maxSubtype + 3 +
                               static_cast<int>(objects::DescriptorsEnd() - objects::DescriptorsBegin()));

    // Keep every legacy WoG subtype registered. Ported entries are intercepted only when their WoG option is enabled;
    // unported entries remain available to the original ERM implementation.
    for (int i = 1; i <= maxSubtype; ++i)
    {
        int optionId = EraJS::readInt(H3String::Format(jsonKeyFormat, WOG_OBJECT_TYPE, i).String());
        if (const auto *d = objects::FindDescriptor(i))
            optionId = d->optionId;
        if (i == marketOfTime::OBJECT_SUBTYPE)
            optionId = marketOfTime::WOG_OPTION;
        if (i == artificer::OBJECT_SUBTYPE)
            optionId = artificer::WOG_OPTION;
        WoGObjectOptionsIds[i] = optionId;
        AddUniqueObjectInfo(WOG_OBJECT_TYPE, i, i == artificer::OBJECT_SUBTYPE ? artificer::AI_SCOUTING_WEIGHT : 0,
                            extender::ePageWogObject);
    }

    AddUniqueObjectInfo(ERA_OBJECT_TYPE, marketOfTime::OBJECT_SUBTYPE, 0, extender::ePageWogObject);
    AddUniqueObjectInfo(ERA_OBJECT_TYPE, artificer::OBJECT_SUBTYPE, artificer::AI_SCOUTING_WEIGHT,
                        extender::ePageWogObject);
    for (const auto *d = objects::DescriptorsBegin(); d != objects::DescriptorsEnd(); ++d)
        AddUniqueObjectInfo(ERA_OBJECT_TYPE, d->subtype, d->defaultAiWeight, extender::ePageWogObject);

    Era::SetEraRegistryIntValue(marketOfTime::LEGACY_HANDLER_REGISTRY_KEY, TRUE);
    Era::SetEraRegistryIntValue(artificer::LEGACY_HANDLER_REGISTRY_KEY, TRUE);
}

H3RmgObjectGenerator *WoGObjectsExtender::CreateRMGObjectGen(const extender::RMGObjectProperties &info,
                                                             const BOOL) const noexcept
{
    if (info.type != ERA_OBJECT_TYPE)
        return nullptr;
    if (info.subtype == marketOfTime::OBJECT_SUBTYPE || info.subtype == artificer::OBJECT_SUBTYPE ||
        (objects::FindDescriptor(info.subtype) && objects::FindDescriptor(info.subtype)->rmgEnabled))
        return CreateDefaultH3RmgObjectGenerator(info);
    return nullptr;
}

BOOL WoGObjectsExtender::InitNewGameMapItemSetup(H3MapItem *mapItem, int, int) const noexcept
{
    if (SubObjectSubtype(mapItem) == artificer::OBJECT_SUBTYPE)
        return artificer::Initialize(mapItem);
    return handlers::Init(mapItem);
}

BOOL WoGObjectsExtender::WoGObjectHasOptionEnabled(const extender::RMGObjectProperties &info) noexcept
{
    if (info.type != WOG_OBJECT_TYPE || info.subtype <= 0 ||
        static_cast<size_t>(info.subtype) >= WoGObjectOptionsIds.size())
        return FALSE;
    return IsWoGOptionEnabled(WoGObjectOptionsIds[info.subtype]);
}

BOOL WoGObjectsExtender::IsWoGOptionEnabled(const int optionId) noexcept
{
    if (optionId < 0 || optionId >= WOG_OPTIONS_COUNT)
        return FALSE;
    const auto *options = static_cast<const int *>(Era::GetRealAddr(reinterpret_cast<void *>(WOG_OPTIONS_ARRAY)));
    if (!options)
        return FALSE;
    // UN:P uses 2/3 for map-author locked off/on states.
    return options[optionId] == 1 || options[optionId] == 3;
}

BOOL WoGObjectsExtender::VisitMapItem(H3Hero *hero, H3MapItem *mapItem, const H3Position pos,
                                      const BOOL isHuman) const noexcept
{
    const int subtype = SubObjectSubtype(mapItem);
    if (subtype == artificer::OBJECT_SUBTYPE)
        return artificer::Visit(hero, mapItem, pos, isHuman);
    if (subtype == marketOfTime::OBJECT_SUBTYPE)
    {
        if (mapItem->objectType == WOG_OBJECT_TYPE && !IsWoGOptionEnabled(marketOfTime::WOG_OPTION))
            return TRUE;
        return marketOfTime::Visit(hero, isHuman);
    }
    if (const auto *d = objects::FindDescriptor(subtype))
    {
        if (mapItem->objectType == WOG_OBJECT_TYPE && !IsWoGOptionEnabled(d->optionId))
            return TRUE;
        return handlers::Visit(hero, mapItem, pos, isHuman);
    }
    return FALSE;
}

BOOL WoGObjectsExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *hero, const int interactPlayerId,
                                               const BOOL isRightClick) const noexcept
{
    const int subtype = SubObjectSubtype(mapItem);
    if (subtype == artificer::OBJECT_SUBTYPE)
        return artificer::SetHint(mapItem, hero, isRightClick);
    if (subtype == marketOfTime::OBJECT_SUBTYPE)
        return marketOfTime::SetHint(isRightClick);
    if (objects::IsPortedSubtype(subtype))
        return handlers::Hint(mapItem, hero, interactPlayerId, isRightClick);
    return FALSE;
}

BOOL WoGObjectsExtender::SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *hero, const H3Player *player, int &aiWeight,
                                            int *moveDistance, const H3Position pos) const noexcept
{
    const int subtype = SubObjectSubtype(mapItem);
    if (subtype == artificer::OBJECT_SUBTYPE)
        return artificer::SetAiWeight(mapItem, hero, pos, aiWeight);
    if (subtype == marketOfTime::OBJECT_SUBTYPE)
    {
        aiWeight = 0;
        return TRUE;
    }
    if (objects::IsPortedSubtype(subtype))
        return handlers::AiWeight(mapItem, hero, player, aiWeight, moveDistance, pos);
    return FALSE;
}

_LHF_(WoGObjectsExtender::LegacyObjectVisit)
{
    // WoG's wrapper (0x705979) visits custom Pyramid subtypes without calling 0x4A8160.
    // At 0x705B18 its five cdecl helper arguments are already on the stack.
    auto *mapItem = *reinterpret_cast<H3MapItem **>(c->ebp + 0xC);
    if (mapItem && mapItem->objectType == WOG_OBJECT_TYPE)
    {
        auto *hero = *reinterpret_cast<H3Hero **>(c->ebp + 0x8);
        const H3Position position = DwordAt(c->ebp + 0x10);
        const BOOL isHuman = DwordAt(c->ebp + 0x14);
        if (Get().VisitMapItem(hero, mapItem, position, isHuman))
        {
            c->eax = 1;                   // Same handled result as WoG's custom Pyramid helper.
            c->return_address = 0x705B1D; // Preserve add esp, 0x14 and the wrapper's normal completion.
            return NO_EXEC_DEFAULT;
        }
    }
    return EXEC_DEFAULT;
}

_LHF_(WoGObjectsExtender::LegacyObjectHoverHint)
{
    auto *mapItem = c->Ebx<H3MapItem *>();
    const int interactPlayerId = IntAt(0x6977DC);
    const H3Hero *hero = nullptr;
    if (P_Main && interactPlayerId >= 0 && interactPlayerId < 8)
    {
        const H3Player &player = P_Main->players[interactPlayerId];
        if (player.currentHero >= 0)
            hero = P_Main->GetHero(player.currentHero);
    }
    if (mapItem && mapItem->objectType == WOG_OBJECT_TYPE &&
        Get().SetHintInH3TextBuffer(mapItem, hero, interactPlayerId, FALSE))
    {
        // The Pyramid branch already copied its name; bypass its native visited suffix.
        c->return_address = 0x40D09B;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

_LHF_(WoGObjectsExtender::LegacyObjectClickHint)
{
    auto *mapItem = c->Ebx<H3MapItem *>();
    const int interactPlayerId = IntAt(0x6977DC);
    const H3Hero *hero = nullptr;
    if (P_Main && interactPlayerId >= 0 && interactPlayerId < 8)
    {
        const H3Player &player = P_Main->players[interactPlayerId];
        if (player.currentHero >= 0)
            hero = P_Main->GetHero(player.currentHero);
    }
    if (mapItem && mapItem->objectType == WOG_OBJECT_TYPE &&
        Get().SetHintInH3TextBuffer(mapItem, hero, interactPlayerId, TRUE))
    {
        c->return_address = 0x4159C0;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

void WoGObjectsExtender::CreatePatches()
{
    if (m_isInited)
        return;
    _pi->WriteLoHook(0x515038, CloneLegacyWoGObjectProperties);
    _pi->WriteLoHook(0x705B18, LegacyObjectVisit);
    _pi->WriteLoHook(0x40C5A1, LegacyObjectHoverHint);
    _pi->WriteLoHook(0x414F66, LegacyObjectClickHint);
    artificer::RegisterEvents();
    handlers::RegisterEvents();
    m_isInited = TRUE;
}

WoGObjectsExtender &WoGObjectsExtender::Get()
{
    if (!instance)
        instance = new WoGObjectsExtender();
    return *instance;
}

} // namespace wog
