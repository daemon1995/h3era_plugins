#include "WoGObjectHandlers.h"
#include "WoGObjectIds.h"
#include "WoGObjectState.h"
#include "WoGObjectUtils.h"
#include <algorithm>
#include <vector>

namespace wog { namespace handlers {
namespace {
using namespace objects;
constexpr int FIRST_PICTURE_ID = 30729;
constexpr int CHOOSE_PICTURE_OR_CANCEL = 10;
using SetSelectablePictures = void(__stdcall *)(int);

int ChoosePage(const char *text, const H3PictureVector &pics)
{
    const HMODULE era = GetModuleHandleA("era.dll");
    const auto setMask = era ? reinterpret_cast<SetSelectablePictures>(GetProcAddress(era, "_SetDialog8SelectablePicsMask")) : nullptr;
    if (setMask) setMask((1 << pics.Size()) - 1);
    Era::TComplexDialogOpts o{}; o.msgType = CHOOSE_PICTURE_OR_CANCEL;
    H3Messagebox::Display(text, pics, o.value);
    const int c = H3WindowManager::Get()->ClickedItemID() - FIRST_PICTURE_ID;
    return c >= 0 && c < static_cast<int>(pics.Size()) ? c : -1;
}
int ChooseArtifact()
{
    std::vector<int> arts;
    for (int id = 0; id < H3ArtifactCount::Get(); ++id)
        if (id != eArtifact::SPELL_SCROLL && util::ArtifactEnabled(id)) arts.push_back(id);
    for (size_t first = 0; first < arts.size(); first += 8)
    {
        H3PictureVector pics;
        for (size_t i = first; i < arts.size() && i < first + 8; ++i)
            pics.Add(H3PictureCategories::Artifact(static_cast<eArtifact>(arts[i])));
        const int c = ChoosePage(util::Tr("skull.chooseArtifact").c_str(), pics);
        if (c >= 0) return arts[first + c];
    }
    return -1;
}
int ChooseSpell()
{
    for (int first = 0; first < 70; first += 8)
    {
        H3PictureVector pics;
        for (int i = first; i < 70 && i < first + 8; ++i) pics.Add(H3PictureCategories::Spell(static_cast<eSpell>(i)));
        const int c = ChoosePage(util::Tr("skull.chooseSpell").c_str(), pics);
        if (c >= 0) return first + c;
    }
    return -1;
}

struct FoundObject { H3MapItem *item = nullptr; H3Point pos; int type = -1; int value = -1; };
std::vector<FoundObject> FindObjects(const int type, const int value)
{
    std::vector<FoundObject> out;
    if (!P_Main) return out;
    const int size = H3MapSize::Get();
    const int levels = P_Main->HasUnderground() ? 2 : 1;
    for (int z = 0; z < levels; ++z)
        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                H3MapItem *mi = P_Main->GetMapItem(H3Position::Pack(x, y, z));
                if (!mi || mi->objectType != type) continue;
                if (type == eObject::ARTIFACT && value >= 0 && mi->objectSubtype != value) continue;
                if (type == eObject::SPELL_SCROLL && value >= 0 && static_cast<int>(mi->spellScroll.type) != value) continue;
                out.push_back({mi, H3Point(x,y,z), type, value});
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
    if (found.type == eObject::SPELL_SCROLL) return 6;
    if (found.type != eObject::ARTIFACT || !util::ValidArtifact(found.item->objectSubtype) || found.item->objectSubtype < 7) return 0;
    const int level = static_cast<int>(H3ArtifactSetup::Get()[found.item->objectSubtype].type);
    if (level == static_cast<int>(eArtifactType::SPECIAL) || level == static_cast<int>(eArtifactType::RELIC)) return 0;
    return (std::max)(0, level * 3);
}
void OfferBring(H3Hero *hero, FoundObject &found)
{
    const int cost = BringCost(found);
    if (cost <= 0) return;
    if (!H3Messagebox::Choice(util::Tr("skull.bring", {"mithril", std::to_string(cost)}).c_str())) return;
    // AddResource(7, ...) is ERA/WoG's native extended-resource path used by CreatureBanksExtender.
    // Read through the same engine path: OW's backing extension exposes the value at AddResource receiver, but H3Resources has only 7 slots.
    // For affordability use WoG's player resource extension address via a harmless native receiver query is unavailable; keep a per-player mirror only for purchases made here.
    const int mithril = IntAt(0x27F9A00 + hero->owner * 4);
    if (mithril < cost)
    {
        H3Messagebox::Show(util::Tr("skull.noMithril").c_str());
        return;
    }
    const int artifactId = found.type == eObject::ARTIFACT ? found.item->objectSubtype : eArtifact::SPELL_SCROLL;
    const int artifactSubtype = found.type == eObject::SPELL_SCROLL ? static_cast<int>(found.item->spellScroll.type) : -1;
    hero->AddResource(7, -cost);
    if (found.type == eObject::ARTIFACT)
        hero->GiveArtifact(H3Artifact(artifactId, -1));
    else if (found.type == eObject::SPELL_SCROLL)
        hero->GiveArtifact(H3Artifact(eArtifact::SPELL_SCROLL, artifactSubtype));

    // WoG has no public H3API removal counterpart to PlaceObjectOnMap. The repository's own native
    // object-replacement helper uses this receiver for removal, then native placement. Keep the same safe path.
    const std::string remove = H3String::Format("UN:O%d/%d/%d/1;", found.pos.x, found.pos.y, found.pos.z).String();
    Era::ExecErmCmd(remove.c_str());
    P_Main->PlaceObjectOnMap(found.pos.x, found.pos.y, found.pos.z, eObject::RESOURCE, 7, cost);
    if (P_AdventureManager) P_AdventureManager->FullUpdate();
}
std::string DirectionText(const H3Hero *hero, const FoundObject &f)
{
    const int dx = f.pos.x - hero->x;
    const int dy = hero->y - f.pos.y;
    std::string s;
    if (dy) s += std::to_string((std::max)(dy, -dy)) + (dy > 0 ? " north" : " south");
    if (dx) { if (!s.empty()) s += ", "; s += std::to_string((std::max)(dx, -dx)) + (dx > 0 ? " east" : " west"); }
    if (f.pos.z != hero->z) { if (!s.empty()) s += ", "; s += f.pos.z ? "underground" : "surface"; }
    return s.empty() ? util::Tr("skull.here") : s;
}
void ResolveSearch(H3Hero *hero, const int type, const int value, const bool freeSearch)
{
    auto found = FindObjects(type, value);
    if (found.empty())
    {
        H3Messagebox::Show(util::Tr("skull.notFound").c_str());
        return;
    }
    FoundObject target = found[util::Random(0, static_cast<int>(found.size()) - 1)];
    H3Messagebox::Show(util::Tr("skull.found", {"layer", target.pos.z ? "underground" : "surface"}).c_str());
    if (freeSearch)
    {
        Reveal(target, hero->owner, 1); OfferBring(hero, target); return;
    }
    static const int rareResources[] = {1,3,4,5};
    const int resource = rareResources[util::Random(0,3)];
    auto &resources = P_Main->players[hero->owner].playerResources;
    if (H3Messagebox::Choice(util::Tr("skull.locationOffer", {"resource", std::to_string(resource)}).c_str(),
                             H3PictureCategories::Resource(static_cast<eResource>(resource), 2)))
    {
        if (resources.asArray[resource] >= 2)
        {
            hero->AddResource(resource, -2);
            H3Messagebox::Show(util::Tr("skull.location", {"direction", DirectionText(hero, target)}).c_str());
        }
        else H3Messagebox::Show(util::Tr("skull.noResource").c_str());
    }
    if (resources.gold >= 1000 && H3Messagebox::Choice(util::Tr("skull.revealOffer").c_str(), H3PictureCategories::Resource(eResource::GOLD, 1000)))
    {
        hero->AddResource(eResource::GOLD, -1000); Reveal(target, hero->owner, 1); OfferBring(hero, target);
    }
}
BOOL VisitSkull(H3Hero *hero, H3MapItem *item, const bool human)
{
    if (!human)
    {
        // Source deliberately disables AI after reports of teleport/visit loops. Keep AI from re-targeting this object.
        state::SetObjectPlayerPeriod(item, LIVING_SKULL, hero->owner, "aiSeen", state::Week());
        return TRUE;
    }
    const int week = state::Week();
    const std::string freeKey = state::ObjectKey(item, LIVING_SKULL, "freeWeek", -1);
    if (!state::IsStamp(freeKey, week))
    {
        auto arts = FindObjects(eObject::ARTIFACT, -1);
        if (!arts.empty())
        {
            state::Stamp(freeKey, week);
            FoundObject target = arts[util::Random(0, static_cast<int>(arts.size()) - 1)];
            H3Messagebox::Show(util::Tr("skull.freeSearch").c_str(), H3PictureCategories::Artifact(static_cast<eArtifact>(target.item->objectSubtype)));
            Reveal(target, hero->owner, 1); OfferBring(hero, target);
            return TRUE;
        }
        // Source falls through to the paid search when no artifact exists; do not consume the weekly free search.
    }
    const std::string dayKey = state::ObjectKey(item, LIVING_SKULL, "searchDay", hero->owner);
    const std::string countKey = state::ObjectKey(item, LIVING_SKULL, "searchCount", hero->owner);
    if (!state::IsStamp(dayKey, state::Day())) { state::Stamp(dayKey, state::Day()); state::Set(countKey, 0); }
    const int count = state::Get(countKey);
    if (count >= 3) { H3Messagebox::Show(util::Tr("skull.tired").c_str()); return TRUE; }
    auto &resources = P_Main->players[hero->owner].playerResources;
    if (resources.gold < 100) { H3Messagebox::Show(util::Tr("skull.noGold").c_str()); return TRUE; }

    int type = eObject::ARTIFACT, value = -1;
    const auto special = H3Messagebox::Choose(util::Tr("skull.targetType").c_str(),
        H3PictureCategories::Artifact(eArtifact::SPELL_SCROLL), H3PictureCategories::Resource(eResource::GOLD, 0));
    if (special == H3Messagebox::LEFT) { type = eObject::SPELL_SCROLL; value = ChooseSpell(); }
    else if (special == H3Messagebox::RIGHT) { type = eObject::PANDORAS_BOX; value = -1; }
    else value = ChooseArtifact();
    if ((type != eObject::PANDORAS_BOX && value < 0)) return TRUE;
    if (!H3Messagebox::Choice(util::Tr("skull.searchOffer").c_str(), H3PictureCategories::Resource(eResource::GOLD, 100))) return TRUE;
    hero->AddResource(eResource::GOLD, -100);
    state::Set(countKey, count + 1);
    ResolveSearch(hero, type, value, false);
    return TRUE;
}

} // namespace

BOOL VisitSkullObject(H3Hero *hero, H3MapItem *item, const BOOL human)
{
    return hero && item ? VisitSkull(hero, item, human != FALSE) : TRUE;
}

}} // namespace wog::handlers
