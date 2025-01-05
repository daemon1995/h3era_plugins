#pragma once

namespace dreamTeacher
{
constexpr int DREAM_TEACHER_OBJECT_SUBTYPE = 0;
constexpr float EXP_GIVEN = 500;
constexpr float ADDITIONAL_EXP_GIVEN_FOR_EACH_VISIT = 500;

struct H3MapItemDreamTeacher
{
    static int dreamTeacherCounter;

    static constexpr LPCSTR ErmVariableFormat = "dreamTeacher_%d_%d"; // visited or not
    static constexpr LPCSTR VisitedObjectsCount = "dreamTeacher_%d"; // heroId

  public:
    INT32 id;

  public:
    static inline BOOL IsVisitedByHero(const H3MapItemDreamTeacher * dreamTeacher, const H3Hero *hero) noexcept;
    static inline int GetVisitedObjectsCountByHero(const H3Hero* hero) noexcept;
    static inline H3MapItemDreamTeacher *GetFromMapItem(const H3MapItem *mapItem) noexcept;
};

class DreamTeacherExtender : public extender::ObjectsExtender
{

    DreamTeacherExtender();

    virtual ~DreamTeacherExtender();

  private:
    virtual void CreatePatches() override;
    //	virtual void AfterLoadingObjectTxtProc(const INT16* maxSubtypes) override final;
    //	virtual void GetObjectPreperties() noexcept override final;

    virtual BOOL SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *currentHero, const H3Player *activePlayer,
                                       const BOOL isRightClick) const noexcept override final;
    virtual BOOL InitNewGameMapItemSetup(H3MapItem *mapItem) const noexcept override final;
    virtual BOOL VisitMapItem(H3Hero *currentHero, H3MapItem *mapItem, const H3Position pos,
                              const BOOL isHuman) const noexcept override final;
    virtual BOOL SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *currentHero, const H3Player *activePlayer,
                                    int &aiMapItemWeight, int* moveDistance, const H3Position pos) const noexcept override final;
    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept override final;

  public:
    static DreamTeacherExtender &Get();
};

} // namespace dreamTeacher
