#include <algorithm>
#include <vector>

#include "WoGObjectHandlers.h"
#include "WoGObjectChoiceDialogs.h"
#include "WoGObjectIds.h"
#include "WoGObjectState.h"
#include "WoGObjectUtils.h"

namespace wog
{
namespace handlers
{
namespace
{
using namespace objects;

struct FoundObject
{
    H3MapItem *item = nullptr;
    H3Point pos;
    int type = -1;
    int value = -1;
};
std::vector<FoundObject> FindObjects(const int type, const int value)
{
    std::vector<FoundObject> out;
    if (!P_Main)
        return out;
    const int size = H3MapSize::Get();
    const int levels = P_Main->HasUnderground() ? 2 : 1;
    for (int z = 0; z < levels; ++z)
        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                H3MapItem *mi = P_Main->GetMapItem(H3Position::Pack(x, y, z));
                if (!mi || mi->objectType != type)
                    continue;
                if (type == eObject::ARTIFACT && value >= 0 && mi->objectSubtype != value)
                    continue;
                if (type == eObject::SPELL_SCROLL && value >= 0 && static_cast<int>(mi->spellScroll.type) != value)
                    continue;
                out.push_back({mi, H3Point(x, y, z), type, value});
            }
    return out;
}
void Reveal(const FoundObject &found, const int player, const int radius)
{
    const int size = H3MapSize::Get();
    for (int y = (std::max)(0, found.pos.y - radius); y <= (std::min)(size - 1, found.pos.y + radius); ++y)
        for (int x = (std::max)(0, found.pos.x - radius); x <= (std::min)(size - 1, found.pos.x + radius); ++x)
            H3TileVision::RevealTile(x, y, found.pos.z, player);
    if (P_AdventureManager)
    {
        P_AdventureManager->ShowCoordinates(found.pos.x, found.pos.y, found.pos.z);
        P_AdventureManager->FullUpdate();
    }
}
int BringCost(const FoundObject &found)
{
    if (found.type == eObject::SPELL_SCROLL)
        return 6;
    if (found.type != eObject::ARTIFACT || !util::ValidArtifact(found.item->objectSubtype) ||
        found.item->objectSubtype < 7)
        return 0;
    const int level = static_cast<int>(H3ArtifactSetup::Get()[found.item->objectSubtype].type);
    if (level == static_cast<int>(eArtifactType::SPECIAL) || level == static_cast<int>(eArtifactType::RELIC))
        return 0;
    return (std::max)(0, level * 3);
}
std::string DirectionText(const H3Hero *hero, const FoundObject &f)
{
    const int dx = f.pos.x - hero->x;
    const int dy = hero->y - f.pos.y;
    std::string s;
    if (dy)
        s += std::to_string((std::max)(dy, -dy)) + (dy > 0 ? " north" : " south");
    if (dx)
    {
        if (!s.empty())
            s += ", ";
        s += std::to_string((std::max)(dx, -dx)) + (dx > 0 ? " east" : " west");
    }
    if (f.pos.z != hero->z)
    {
        if (!s.empty())
            s += ", ";
        s += f.pos.z ? "underground" : "surface";
    }
    return s.empty() ? util::Tr("skull.here") : s;
}

// Keep the hero's tile visible after inspecting an artifact, including cancellation paths.
struct SkullCamera
{
    H3Hero *hero;
    bool moved = false;
    void Restore()
    {
        if (moved && P_AdventureManager)
        {
            P_AdventureManager->ShowCoordinates(hero->x, hero->y, hero->z);
            P_AdventureManager->FullUpdate();
        }
        moved = false;
    }
    void Show(const FoundObject &target)
    {
        moved = true;
        Reveal(target, hero->owner, 1);
    }
    ~SkullCamera() { Restore(); }
};

class SkullInteraction
{
    enum Stage { SEARCH, LOCATION, BRING, FINISHED } stage = SEARCH;
    H3Hero *hero;
    SkullCamera &camera;
    std::string countKey, direction;
    SkullTarget requested;
    FoundObject target;
    int resource = 0;
    bool located = false;

    std::string Name() const
    {
        return target.type == eObject::SPELL_SCROLL ? H3Spell::Get()[target.item->spellScroll.type].name :
            H3Artifact(target.item->objectSubtype, -1).GetName();
    }
    void Finish(SkullService &view, const char *key)
    {
        view.text = util::Tr(key); view.actions.clear(); stage = FINISHED;
    }
    void LocationOffers(SkullService &view)
    {
        view.text = Name() + (direction.empty() ? h3_NullString : "\n\n" + direction);
        view.actions.clear();
        if (!located) view.actions.push_back(util::Tr("skull.locationOffer", {"resource", P_ResourceName[resource]}));
        view.actions.push_back(util::Tr("skull.revealOffer"));
    }
    std::string BringOffer() const
    {
        return util::Tr("skull.bring", {"mithril", std::to_string(BringCost(target)), "artifact", Name()});
    }
    void BringOffers(SkullService &view, const std::string &prefix)
    {
        view.text = prefix; view.actions.clear();
        if (BringCost(target) <= 0)
        {
            view.text += "\n\n" + util::Tr("skull.cantBring"); stage = FINISHED;
        }
        else
        {
            stage = BRING; view.actions.push_back(BringOffer());
        }
    }
    void Execute(int action, SkullService &view)
    {
        auto &resources = P_Main->players[hero->owner].playerResources;
        if (stage == SEARCH)
        {
            if (state::Get(countKey) >= 3) { Finish(view, "skull.tired"); return; }
            if (resources.gold < 100) { Finish(view, "skull.noGold"); return; }
            hero->AddResource(eResource::GOLD, -100);
            state::Set(countKey, state::Get(countKey) + 1);
            auto found = FindObjects(requested.type, requested.value);
            if (found.empty()) { Finish(view, "skull.notFound"); return; }
            target = found[util::Random(0, int(found.size()) - 1)];
            static const int rareResources[] = {1, 3, 4, 5};
            resource = rareResources[util::Random(0, 3)];
            stage = LOCATION; LocationOffers(view);
        }
        else if (stage == LOCATION)
        {
            if (!located && action == 0)
            {
                if (resources.asArray[resource] < 2) { view.text = util::Tr("skull.noResource"); return; }
                hero->AddResource(resource, -2); located = true;
                direction = util::Tr("skull.location", {"direction", DirectionText(hero, target)});
                LocationOffers(view);
            }
            else
            {
                if (resources.gold < 1000) { view.text = util::Tr("skull.noRevealGold"); return; }
                hero->AddResource(eResource::GOLD, -1000);
                camera.Show(target); BringOffers(view, Name());
            }
        }
        else if (stage == BRING)
        {
            const int cost = BringCost(target);
            if (util::Resource(hero->owner, 7) < cost) { view.text = util::Tr("skull.noMithril"); return; }
            if (hero->backpackCount >= 64) { view.text = util::Tr("skull.noRoom"); return; }
            if (!util::ChangeResource(hero, eResource::MITHRIL, -cost)) return;
            hero->GiveArtifact(H3Artifact(target.type == eObject::SPELL_SCROLL ? eArtifact::SPELL_SCROLL :
                target.item->objectSubtype, target.type == eObject::SPELL_SCROLL ? int(target.item->spellScroll.type) : -1));
            THISCALL_4(void, 0x4AA820, P_AdventureManager->Get(), target.item,
                H3Position(target.pos.x, target.pos.y, target.pos.z).Mixed(), 1);
            P_Main->PlaceObjectOnMap(target.pos.x, target.pos.y, target.pos.z, eObject::RESOURCE, 7, cost);
            Finish(view, "skull.delivered");
        }
    }
    void Run(SkullService view)
    {
        ShowSkullService(std::move(view), [this](int choice, SkullService &current) { Execute(choice, current); });
        camera.Restore();
    }
  public:
    SkullInteraction(H3Hero *hero, SkullCamera &camera) : hero(hero), camera(camera) {}
    void Free(FoundObject found)
    {
        target = found; camera.Show(target);
        SkullService view;
        view.artifact = target.item->objectSubtype;
        BringOffers(view, util::Tr("skull.freeSearch") + "\n\n" + Name());
        Run(std::move(view));
    }
    void Search(const SkullTarget &choice, const std::string &searchCountKey)
    {
        requested = choice; countKey = searchCountKey;
        SkullService view;
        view.artifact = choice.type == eObject::SPELL_SCROLL ? eArtifact::SPELL_SCROLL : choice.value;
        view.spell = choice.type == eObject::SPELL_SCROLL ? choice.value : -1;
        view.text = choice.type == eObject::SPELL_SCROLL ? H3Spell::Get()[choice.value].name :
            H3Artifact(choice.value, -1).GetName();
        view.actions.push_back(util::Tr("skull.searchOffer"));
        Run(std::move(view));
    }
};

BOOL VisitSkull(H3Hero *hero, H3MapItem *item, const bool human)
{
    if (!human) return TRUE; // AI must not re-target this object's teleport/search services.
    SkullCamera camera{hero};
    const int week = state::Week();
    if (!state::ObjectDay(item, week))
    {
        auto arts = FindObjects(eObject::ARTIFACT, -1);
        if (!arts.empty())
        {
            state::SetObjectDay(item, week, hero);
            SkullInteraction(hero, camera).Free(arts[util::Random(0, int(arts.size()) - 1)]);
        }
    }
    const auto dayKey = state::ObjectKey(item, LIVING_SKULL, "searchDay", hero->owner);
    const auto countKey = state::ObjectKey(item, LIVING_SKULL, "searchCount", hero->owner);
    if (!state::IsStamp(dayKey, state::Day()))
    {
        state::Stamp(dayKey, state::Day()); state::Set(countKey, 0);
    }
    ShowSkullSearch([&](const SkullTarget &choice) { SkullInteraction(hero, camera).Search(choice, countKey); });
    return TRUE;
}

} // namespace

BOOL VisitSkullObject(H3Hero *hero, H3MapItem *item, const BOOL human)
{
    return hero && item ? VisitSkull(hero, item, human != FALSE) : TRUE;
}

} // namespace handlers
} // namespace wog
