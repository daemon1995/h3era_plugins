#pragma once
#include <set>

namespace extender
{
// temp struct to allow add line into protected field "H3Vector<LPCSTR> text;" at 0x1C
struct EditableH3TextFile : public H3TextFile
{
    void AddLine(LPCSTR txt);
};
struct RMGObjectSetable
{
    int type;
    int subtype;
    bool operator<(const RMGObjectSetable &other) const;
};

class ObjectsExtender : public IGamePatch
{

    static std::set<ObjectsExtender *> extenders;

  protected:
    static std::vector<std::string> additionalProperties;
    static std::vector<RMGObjectInfo> additionalRmgObjects;

    ObjectsExtender(PatcherInstance *pi);

    virtual ~ObjectsExtender();

    // virtual void GetObjectPreperties() noexcept = 0;

    virtual void CreatePatches() override;
    // required override for some complex structures like creature banks
    virtual void AfterLoadingObjectTxtProc(const INT16 *maxSubtypes);
    //	virtual int AiMapItemWeightFunction(HookContext* c, const H3MapItem* mapItem, H3Player* player);
    // returns if object was visited by some of derived classes
    // virtual BOOL HeroMapItemVisitFunction(HookContext* c, const H3Hero* hero, const H3MapItem* mapItem, const BOOL
    // isPlayer, const BOOL skipMapMessage);

  private:
    // static methods to use them as General Objects Extending hooks
    static void __stdcall H3GameMainSetup__LoadObjects(HiHook *h, const H3MainSetup *setup);
    static _LHF_(LoadObjectsTxt);
    static _LHF_(H3AdventureManager__ObjectVisit_SoundPlay);

  public:
    static void AddObjectsToObjectGenList(H3Vector<H3RmgObjectGenerator *> *rmgObjecsList);

    // static void __stdcall H3AdventureManager__ObjectVisit_SoundPlay(HiHook* h, const int objType, const int
    // objSetup);
};

} // namespace extender
