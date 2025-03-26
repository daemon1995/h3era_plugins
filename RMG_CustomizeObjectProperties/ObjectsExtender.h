#pragma once
#include "RMGObjectsEditor.h"
#include <unordered_set>
namespace extender
{
constexpr int HOTA_OBJECT_TYPE = 144;
constexpr int HOTA_PICKUPABLE_OBJECT_TYPE = 145;
namespace limits
{
constexpr int EXTENDED = 1024;
constexpr int COMMON = 255;
} // namespace limits

// temp struct to allow add line into protected field "H3Vector<LPCSTR> text;" at 0x1C
struct EditableH3TextFile : public H3TextFile
{
    void AddLine(LPCSTR txt);
    size_t GetLineCount() const noexcept;
};

struct ObjectProperty
{

    static std::unordered_map<std::string, std::string> additionalPropertiesMap;

  public:
    static const std::string *FindPropertyReplace(LPCSTR other) noexcept;
    static std::string GetMapKey(LPCSTR other) noexcept;
    static BOOL AddProperty(std::string &other) noexcept;
};

struct RMGObjectSetable
{
    int type;
    int subtype;
    struct HashFunction
    {
        size_t operator()(const RMGObjectSetable &obj) const noexcept;
    };
    bool operator==(const RMGObjectSetable &other) const noexcept;
};

class ObjectsExtender : public IGamePatch
{

    static std::vector<ObjectsExtender *> extenders;

  protected:
    static std::vector<RMGObjectInfo> additionalRmgObjects;

  protected:
    ObjectsExtender(PatcherInstance *pi);
    virtual ~ObjectsExtender();

    // virtual void GetObjectPreperties() noexcept = 0;
  protected:
    virtual void CreatePatches() override;
    // required override for some complex structures like creature banks
    virtual void AfterLoadingObjectTxtProc(const INT16 *maxSubtypes);
    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectInfo &info) const noexcept = 0;
    virtual BOOL InitNewGameMapItemSetup(H3MapItem *mapItem) const noexcept;
    virtual BOOL InitNewWeekMapItemSetup(H3MapItem *mapItem) const noexcept;
    virtual BOOL VisitMapItem(H3Hero *currentHero, H3MapItem *mapItem, const H3Position pos,
                              const BOOL isHuman) const noexcept;
    virtual BOOL SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *currentHero, const H3Player *activePlayer,
                                       const BOOL isRightClick) const noexcept;
    virtual BOOL SetAiMapItemWeight(H3MapItem *mapItem, const H3Hero *currentHero, const H3Player *activePlayer,
                                    int &aiResWeight) const noexcept;

    //	virtual int AiMapItemWeightFunction(HookContext* c, const H3MapItem* mapItem, H3Player* player);
    // returns if object was visited by some of derived classes
    // virtual BOOL HeroMapItemVisitFunction(HookContext* c, const H3Hero* hero, const H3MapItem* mapItem, const BOOL
    // isPlayer, const BOOL skipMapMessage);

  private:
    static _LHF_(Game__NewGameObjectIteration);
    static _LHF_(Game__NewWeekObjectIteration);
    static _LHF_(H3AdventureManager__ObjectVisit);
    static _LHF_(H3AdventureManager__GetDefaultObjectHoverHint);
    static _LHF_(H3AdventureManager__GetDefaultObjectClickHint);
    static _LHF_(AIHero_GetObjectPosWeight);

  protected:
    static H3RmgObjectGenerator *CreateDefaultH3RmgObjectGenerator(const RMGObjectInfo &info) noexcept;

  private:
    // static methods to use them as General Objects Extending hooks
    static void __stdcall H3GameMainSetup__LoadObjects(HiHook *h, const H3MainSetup *setup);
    static _LHF_(LoadObjectsTxt);
    static INT ShowObjectHint(LoHook *h, HookContext *c, const BOOL isRightClick);
    static void LoadMapObjectPropertiesByTypeSubtypes() noexcept;
    static void LoadMapObjectPropertiesFromLoadedMods() noexcept;

  public:
    static void AddObjectsToObjectGenList(H3Vector<H3RmgObjectGenerator *> *rmgObjecsList);
    // static void HandleRmgOjectGeneratorBeforeAdding(H3Vector<H3RmgObjectGenerator*>* rmgObjecsList);

    // static void __stdcall H3AdventureManager__ObjectVisit_SoundPlay(HiHook* h, const int objType, const int
    // objSetup);
};

} // namespace extender
