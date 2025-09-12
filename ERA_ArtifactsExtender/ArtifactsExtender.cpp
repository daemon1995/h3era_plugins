#include "pch.h"
namespace artifacts
{

inline int GetArtifactsNumber() noexcept
{
    return IntAt(0x717020);
}

ArtifactsExtender *ArtifactsExtender::instance = nullptr;
DWORD ArtifactsExtender::baseArtifactsNumber = 0;
DWORD ArtifactsExtender::newArtifactsNumber = 0;

H3ArtifactSetup *ArtifactsExtender::artifactSetups = nullptr;
DWORD *ArtifactsExtender::primarySkillsBonusTable = nullptr;

DWORD ArtifactsExtender::primarySkillsBonusTableAddr = 0;

void ArtifactsExtender::AfterLoadingObjectTxtProc(const INT16 *maxSubtypes)
{
    int artsNum = P_Game->mainSetup.objectLists[ARTIFACTS_OBJECT_TYPE].Size();
    // int maxArtId = 0;
    for (auto &i : P_Game->mainSetup.objectLists[ARTIFACTS_OBJECT_TYPE])
    {
        if (i.subtype >= artsNum)
        {
            artsNum = i.subtype + 1;
        }
    }
    const auto &vec = P_Game->mainSetup.objectLists[ARTIFACTS_OBJECT_TYPE];
    const int currentArtsNum = GetArtifactsNumber();
    // if new artifacts added, resize the setups array
    if (artsNum > currentArtsNum)
    {
        baseArtifactsNumber = currentArtsNum;
        newArtifactsNumber = artsNum;

   //     instance->CreatePatches();
    }
}

ArtifactsExtender::ArtifactsExtender() : IGamePatch(_PI)
{
}
ArtifactsExtender::~ArtifactsExtender()
{
}

void ArtifactsExtender::CreatePatches()
{
    if (m_isInited)
        return;
    m_isInited = TRUE;
    ReplaceArtifactSetupsTable();
    ReplacePrimarySkillsBonusTable();

    // ReplaceAIValuesTable();
    // resize the artifacts setups array

    // init reading of artifact fields from json
}

void ArtifactsExtender::ReplaceArtifactSetupsTable()
{

    // copy original setups to new array
    artifactSetups = new H3ArtifactSetup[newArtifactsNumber];
    const auto originalTableAddress = P_ArtifactSetup->Get();
    libc::memcpy(artifactSetups, originalTableAddress, sizeof(H3ArtifactSetup) * baseArtifactsNumber);

    // set new H3ArtifactSetup array to the game pointer
    _PI->WriteDword(0x660B68, (DWORD_PTR)artifactSetups);
    // patch WoG arts setup getter

    _PI->WriteDword(0x0714FA7 + 1, (DWORD_PTR)artifactSetups);

    // Wog_Patches
    {
        // Патчим воговскую таблицу
        _PI->WriteDword(0x754A40 + 2, (DWORD_PTR)artifactSetups); // WoG

        // Патчим воговское кол-во артефактов
        //_PI->WriteDword(0x714ECA, newArtifactsNumber); // IsArtDisabled
        //_PI->WriteDword(0x714F46, newArtifactsNumber); // DisableArt
        //_PI->WriteDword(0x716F7F, newArtifactsNumber); // CleanUpCombo
        //_PI->WriteDword(0x717020, newArtifactsNumber); // BuildUpCombo
        //_PI->WriteDword(0x7324BD, newArtifactsNumber); // WoG::ERM::UN
        //_PI->WriteDword(0x732849, newArtifactsNumber); // WoG::ERM::UN

        // buffer save value
        _PI->WriteDword(0x75120D, newArtifactsNumber);     // WoG::Save
        _PI->WriteDword(0x751C5A + 3, newArtifactsNumber); // WoG::Loader
      //  auto  artifactSetups = artifactSetups;
        auto emerald = _PI;
        emerald->WriteDword(0x7324BD, newArtifactsNumber); //UN:A
        emerald->WriteDword(0x716F8D, (int)artifactSetups + 0x18);
        emerald->WriteDword(0x716F9E, (int)artifactSetups + 0x18);
        emerald->WriteDword(0x716FAE, (int)artifactSetups + 0x14);
        emerald->WriteDword(0x716FBF, (int)artifactSetups + 0x14);
        emerald->WriteDword(0x717117, (int)artifactSetups + 0x14);
        emerald->WriteDword(0x717146, (int)artifactSetups + 0x18);
        emerald->WriteDword(0x754A42, (int)artifactSetups);
        emerald->WriteDword(0x717117, (int)artifactSetups + 0x14);
        emerald->WriteDword(0x714ECA, newArtifactsNumber);
        emerald->WriteDword(0x714F46, newArtifactsNumber);
        emerald->WriteDword(0x716F7F, newArtifactsNumber);
        emerald->WriteDword(0x717020, newArtifactsNumber);
        emerald->WriteDword(0x7324BD, newArtifactsNumber);
        emerald->WriteDword(0x732849, newArtifactsNumber);

        emerald->WriteDword(0x75120D, newArtifactsNumber); //WoG_Saver
        emerald->WriteDword(0x751c5d, newArtifactsNumber); //WoG_Loader
    }
    // clearing original table;
    // libc::memset(originalTableAddress, 0, sizeof(H3ArtifactSetup) * baseArtifactsNumber);
    //  return;
}

void ArtifactsExtender::ReplacePrimarySkillsBonusTable()
{

    // Таблица бонусов артов.
    // Создаём и копируем таблицу бонусов артефактов.
    primarySkillsBonusTable = new DWORD[4 * newArtifactsNumber];
    const auto originalTableAddress = reinterpret_cast<INT8 *>(DwordAt(0x04E2E94 + 1));

    libc::memcpy(primarySkillsBonusTable, originalTableAddress, 4 * baseArtifactsNumber);

    libc::memset(originalTableAddress, 0, 4 * baseArtifactsNumber);

    primarySkillsBonusTableAddr = reinterpret_cast<DWORD>(&primarySkillsBonusTable[0]);

    // equip
    _PI->WriteDword(0x4E2D25 + 1, primarySkillsBonusTableAddr);
    _PI->WriteDword(0x4E2D3B + 2, primarySkillsBonusTableAddr + 4 * newArtifactsNumber);
    _PI->WriteDword(0x4E2DEE + 3, primarySkillsBonusTableAddr);
    _PI->WriteDword(0x4E2DD5 + 2, primarySkillsBonusTableAddr + 4 * newArtifactsNumber);

    // unequip
    _PI->WriteDword(0x4E2E94 + 1, primarySkillsBonusTableAddr);
    _PI->WriteDword(0x4E2EAA + 2, primarySkillsBonusTableAddr + 4 * newArtifactsNumber);
    _PI->WriteDword(0x4E2F79 + 3, primarySkillsBonusTableAddr);
    _PI->WriteDword(0x4E2F3F + 2, primarySkillsBonusTableAddr + 4 * newArtifactsNumber);
}

void ArtifactsExtender::ReplaceCombinationArtifactsTable()
{
}

void ArtifactsExtender::ReplaceAIValuesTable()
{

    // Таблица ценностей артов

    // Выделяем память под таблицу ценностей артов.
    const DWORD *AIValuesTable = new DWORD[16 * newArtifactsNumber];
    auto AIValuesTableAddr = reinterpret_cast<DWORD>(&AIValuesTable[0]);

    // Ссылки на новую таблицу.

    // Конструктор.
    _PI->WriteDword(0x432471 + 1, AIValuesTableAddr);

    // Деструктор.
    _PI->WriteDword(0x43249C + 1, AIValuesTableAddr);

    // Получение ценности арта.
    _PI->WriteDword(0x4338DB + 2, AIValuesTableAddr);
    _PI->WriteDword(0x433948 + 1, AIValuesTableAddr + 8);
    _PI->WriteDword(0x433957 + 2, AIValuesTableAddr + 16 * newArtifactsNumber + 8);
    _PI->WriteDword(0x4339B1 + 2, AIValuesTableAddr + 16 * newArtifactsNumber + 8);

    // Заполнение ценностей.
    _PI->WriteDword(0x434123 + 2, AIValuesTableAddr + 8);
    _PI->WriteDword(0x434129 + 2, AIValuesTableAddr);

    // Деструктор при закрытии с ошибкой.
    _PI->WriteDword(0x434596 + 1, AIValuesTableAddr + 8);
    _PI->WriteDword(0x4345E4 + 2, AIValuesTableAddr + 16 * newArtifactsNumber + 8);

    // ArtValuesTable = NewP;
}

void __stdcall ArtifactsExtender::H3GameMainSetup__LoadObjects(HiHook *h, const H3MainSetup *setup)
{
    // call native fucntion to load objects.txt (0x515038)
    // and increase eax by number of added objects
    THISCALL_1(void, h->GetDefaultFunc(), setup);
    ArtifactsExtender::Get().AfterLoadingObjectTxtProc(0);
}
_ERH_(ArtifactsExtender::OnAfterWog)
{
}
ArtifactsExtender &ArtifactsExtender::Get()
{
    if (instance == nullptr)
    {
        instance = new ArtifactsExtender();
    }
    return *instance;
}

} // namespace artifacts
