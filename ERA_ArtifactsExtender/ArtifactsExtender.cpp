#include "map"
#include "pch.h"

namespace artifacts
{

inline int GetBaseArtifactsNumber() noexcept
{
    return IntAt(0x71701D + 3);
}

std::vector<H3ArtifactSetup> artifactCopies;

ArtifactsExtender *ArtifactsExtender::instance = nullptr;

ArtifactsExtender::ArtifactsNumber ArtifactsExtender::artifactsNumber = {};

// DWORD ArtifactsExtender::baseArtifactsNumber = 0;
// DWORD ArtifactsExtender::newArtifactsNumber = 0;

H3ArtifactSetup *ArtifactsExtender::artifactSetups = nullptr;
H3ArtifactSetup *ArtifactsExtender::wogArtifactSetupsToSwapNames = nullptr;
ArtifactsExtender::ArtifactsTextZVars *ArtifactsExtender::artifactsTextZVars = nullptr;
DWORD *ArtifactsExtender::primarySkillsBonusTable = nullptr;

DWORD ArtifactsExtender::primarySkillsBonusTableAddr = 0;
LPCSTR *ArtifactsExtender::eventTable = nullptr;

void ArtifactsExtender::AfterLoadingObjectTxtProc(const INT16 *maxSubtypes)
{

    const int artifactsAdded = LoadNewArtifactsFromJson();

    // set current artifacts number
    const int currentArtsNum = GetBaseArtifactsNumber();
    artifactsNumber.current = currentArtsNum;
    artifactsNumber.base = currentArtsNum;

    // if new artifacts added, resize the setups array
    if (artifactsAdded)
    {
        artifactsNumber.added = artifactsAdded;
        artifactsNumber.current += artifactsAdded;
        // artifactsNumber.added = artsNum - currentArtsNum;
        //  newArtifactsNumber = artsNum;
        instance->CreatePatches();
    }
    instance->LoadArtifactPropertiesFromJson();
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
    //    ReplaceArtEventText();

    return;
}

// Load new artifacts from each mod json key "era.artifacts"
const int ArtifactsExtender::LoadNewArtifactsFromJson()
{
    int addedArts = 0;
    const auto loadedMods = modList::GetEraModList(1);
    std::map<int, artifacts::ArtifactData> modNamesMap;
    int maxArtId = 0;
    for (const auto &mod : loadedMods)
    {
        int arrayCounter = 0;
        do
        {
            // Load artifacts from each mod
            artifacts::ArtifactData artifactData(mod.c_str(), arrayCounter++);
            const int artId = artifactData.getId();
            if (artifactData.getId() != eArtifact::NONE)
            {
                auto result = modNamesMap.try_emplace(artifactData.getId(), artifactData);
                if (!result.second)
                {
                    MessageBoxA(nullptr,
                                H3String::Format("Artifact ID %d in mod %s is duplicated and will be ignored",
                                                 artifactData.getId(), mod.c_str())
                                    .String(),
                                "Error loading artifacts from json", MB_ICONERROR);
                }
                else
                {
                    addedArts++;
                }
                if (artId > maxArtId)
                {
                    maxArtId = artId;
                }
            }
            else
            {
                break;
            }

        } while (true);
    }

    return addedArts;
}

void ArtifactsExtender::ReplaceArtifactSetupsTable()
{

    constexpr DWORD newArtifactsNumber = artifactsNumber.max;
    // copy original setups to new array
    artifactSetups = new H3ArtifactSetup[newArtifactsNumber];

    const auto originalTableAddress = P_ArtifactSetup->Get();

    auto &baseArts = std::vector<H3ArtifactSetup>(originalTableAddress, originalTableAddress + artifactsNumber.base);

    // libc::memcpy(artifactSetups, originalTableAddress, sizeof(H3ArtifactSetup) * artifactsNumber.base);

    artifactSetups[144] = {};
    artifactSetups[145] = {};
    auto &artdiploma = artifactSetups[TEST_ART_ID];

    artdiploma.name = "Tome of Earth";
    artdiploma.description = "Tome of Earth\n\n123123";
    // artdiploma.position = eArtifactSlots::FEET;

    // artifactCopies = std::vector<H3ArtifactSetup>(artifactSetups, artifactSetups + newArtifactsNumber);
    artifactCopies = std::vector<H3ArtifactSetup>(artifactSetups, artifactSetups + newArtifactsNumber);

    DWORD tablePtr = (DWORD)&artifactSetups[0];
    const DWORD oldTableSize = sizeof(H3ArtifactSetup) * artifactsNumber.base;

    // SoD Patches - used to replace array that is filled from artraits.txt
    // it is used cause main login is in "OnAfterWog" that happens before a "LoadArtifactsTxt" hook
    {
        // Load ArtTraits
        _PI->WriteDword(0x044CB31 + 1, tablePtr);        // 44CB31
        _PI->WriteDword(0x044CCDD + 2, tablePtr + 0x1C); // 44CB31
        _PI->WriteDword(0x044CCF8 + 2, tablePtr + 0x1D); // 44CB31
        _PI->WriteDword(0x044CD1C + 2, tablePtr);        // 44CB31
        _PI->WriteDword(0x044CD6B + 1, tablePtr);        // 44CB31
    }

    Era::RedirectMemoryBlock((void *)*(int *)0x660B68, oldTableSize, &artifactSetups[0]);

    // set new H3ArtifactSetup array to the game pointer
    _PI->WriteDword(0x660B68, tablePtr);

    // Era::RedirectMemoryBlock(originalTableAddress, oldTableSize, artifactSetups);

    //   Era::RedirectMemoryBlock((void*)*(int*)0x4E2D26, sizeof(ART_BONUS) * OLD_ARTS_AMOUNT, no_save.newbtable);
    // replace artevent table
    //_PI->WriteDword(0x49F51B, (DWORD_PTR)artifactSetups);
    // patch WoG arts setup getter
    // return;

    // Wog_Patches
    {

        // Патчим воговскую таблицу
        // 007548BC -- WoG::ChooseArtToLeave
        _PI->WriteDword(0x0754A40 + 2, tablePtr); // таблица артефактов для оставления на карте

        // 007515FF -- WoG::SaveGameLoader
        _PI->WriteDword(0x751C5A + 3, newArtifactsNumber); // WoG_Loader

        // 0073114F -- WoG::ERM::UN (cases 'A' and 'B')
        {

            _PI->WriteDword(0x7324BA + 3, newArtifactsNumber); // WoG::ERM::UN::A
            _PI->WriteDword(0x732846 + 3, newArtifactsNumber); // WoG::ERM::UN::A
        }

        wogArtifactSetupsToSwapNames = new H3ArtifactSetup[newArtifactsNumber];

        // patch art names table used to replace text
        if (wogArtifactSetupsToSwapNames)
        {
            // copy new table into names table to swap names later
            libc::memcpy(wogArtifactSetupsToSwapNames, artifactSetups, sizeof(H3ArtifactSetup) * artifactsNumber.max);

            const DWORD wogArtNamesTablePtr = (DWORD)&wogArtifactSetupsToSwapNames[0];
            // 007515FF -- WoG::SaveGameLoader
            _PI->WriteDword(0x0751E3E + 2, wogArtNamesTablePtr);
            _PI->WriteDword(0x0751EAB + 2, wogArtNamesTablePtr + 0x10);

            // 00752598 -- WoG::ResetERM
            _PI->WriteDword(0x0752D97 + 2, wogArtNamesTablePtr);
            _PI->WriteDword(0x0752DD1 + 2, wogArtNamesTablePtr);

            // 0073114F -- WoG::ERM::UN (cases 'A' and 'B')
            _PI->WriteDword(0x007326FA + 2, wogArtNamesTablePtr);
        }

        artifactsTextZVars = new ArtifactsTextZVars[artifactsNumber.max];
        // patch art names saved int the global z vars
        if (artifactsTextZVars)
        {

            // create new table and copy data
            const auto originalZVarTableAddres = *reinterpret_cast<ArtifactsTextZVars **>(0x0732672 + 3);
            libc::memcpy(artifactsTextZVars, originalZVarTableAddres,
                         sizeof(ArtifactsTextZVars) * artifactsNumber.base);

            const DWORD artifactsTextZVarsTablePtr = (DWORD)&artifactsTextZVars[0].artNameZvar;
            const DWORD artifactsTextZVarsTablePtrDescription = (DWORD)&artifactsTextZVars[0].artDescriptionZvar;

            // 00750E7C -- WoG::Save
            _PI->WriteDword(0x075120A + 3, newArtifactsNumber); // Патчим воговское кол-во артефактов

            _PI->WriteDword(0x075122C + 3, artifactsTextZVarsTablePtr);
            _PI->WriteDword(0x07512C0 + 3, artifactsTextZVarsTablePtr + 4);

            // 007515FF -- WoG::SaveGameLoader
            _PI->WriteDword(0x0751C94 + 3, artifactsTextZVarsTablePtr);
            _PI->WriteDword(0x0751D28 + 3, artifactsTextZVarsTablePtr + 4);

            _PI->WriteDword(0x0751DDE + 3, artifactsTextZVarsTablePtr);
            _PI->WriteDword(0x0751E49 + 3, artifactsTextZVarsTablePtr + 4);

            // 00752598 -- WoG::ResetERM
            _PI->WriteDword(0x0752DFA + 3, artifactsTextZVarsTablePtr);
            _PI->WriteDword(0x0752E08 + 3, artifactsTextZVarsTablePtr + 4);

            // 0073114F -- WoG::ERM::UN (cases 'A' and 'B')
            _PI->WriteDword(0x0732672 + 3, artifactsTextZVarsTablePtr);
            _PI->WriteDword(0x073269F + 4, artifactsTextZVarsTablePtr);
            _PI->WriteDword(0x073270A + 4, artifactsTextZVarsTablePtr + 4);
            _PI->WriteDword(0x0732737 + 4, artifactsTextZVarsTablePtr + 4);
        }

        // buffer save value
        //   _PI->WriteDword(0x75120A + 3, newArtifactsNumber); // WoG::Save
        //   _PI->WriteDword(0x751C5A + 3, newArtifactsNumber); // WoG::Loader
        //  auto  artifactSetups = artifactSetups;

        // _PI->WriteDword(0x716F8D, (int)artifactSetups + 0x18);

        // patch combinations
        {

            DWORD combinationArtifactIdPtr = (DWORD)&artifactSetups[0].combinationArtifactId; // + 0x18
            DWORD comboIDPtr = (DWORD)&artifactSetups[0].comboID;                             // + 0x14

            // 00716F50 -- WoG::CleanUpCombo
            _PI->WriteDword(0x0716F7F, newArtifactsNumber); // Патчим воговское кол-во артефактов

            _PI->WriteDword(0x0716F8B + 2, combinationArtifactIdPtr);
            _PI->WriteDword(0x0716F9C + 2, combinationArtifactIdPtr);
            _PI->WriteDword(0x0716FAC + 2, comboIDPtr);
            _PI->WriteDword(0x0716FBD + 2, comboIDPtr);

            // 00716FE5 -- WoG::BuildUpCombo
            _PI->WriteDword(0x0717020, newArtifactsNumber); // Патчим воговское кол-во артефактов

            if (0)
            {
                _PI->WriteDword(0x0717115 + 2, comboIDPtr);
                _PI->WriteDword(0x0717144 + 2, combinationArtifactIdPtr);
            }
        }

        // IsArtDisabled !!!
        /*
        FIX GAME MANAGER  o_GameMgr->MustEnebleArt[a1] != 0; size up to new arts values
        */
        _PI->WriteDword(0x0714EC7 + 3, newArtifactsNumber); // Патчим воговское кол-во артефактов
        _PI->WriteDword(0x0714F43 + 3, newArtifactsNumber); // // Патчим воговское кол-во артефактов

        //  _PI->WriteCodePatch(0x0751234, "%n", 5);
        //  _PI->WriteCodePatch(0x07512C8, "%n", 5);
    }
    // clearing original table;
    // libc::memset(originalTableAddress, 0, sizeof(H3ArtifactSetup) * baseArtifactsNumber);
    //  return;
}

void ArtifactsExtender::ReplacePrimarySkillsBonusTable()
{
    constexpr DWORD newArtifactsNumber = artifactsNumber.max;
    const DWORD baseArtifactsNumber = artifactsNumber.base;

    // Таблица бонусов артов.
    // Создаём и копируем таблицу бонусов артефактов.
    primarySkillsBonusTable = new DWORD[newArtifactsNumber * 4];

    const auto originalTableAddress = reinterpret_cast<INT8 *>(DwordAt(0x04E2E94 + 1));

    // copy original data
    libc::memcpy(primarySkillsBonusTable, originalTableAddress, 4 * baseArtifactsNumber);

    std::vector<DWORD> vec(primarySkillsBonusTable, primarySkillsBonusTable + baseArtifactsNumber);

    vec[1];
    // clear original table  (why?)
    libc::memset(originalTableAddress, 0, 4 * baseArtifactsNumber);

    // set default bonus values as 0
    libc::memset(&primarySkillsBonusTable[baseArtifactsNumber], 0, 4 * (newArtifactsNumber - baseArtifactsNumber));

    primarySkillsBonusTable[TEST_ART_ID] = 123;
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

void ArtifactsExtender::ReplaceArtEventText()
{
    constexpr DWORD newArtifactsNumber = artifactsNumber.max;
    const DWORD baseArtifactsNumber = artifactsNumber.base;

    const auto originalTableAddress = reinterpret_cast<LPCSTR *>(DwordAt(0x049F51B + 3));

    eventTable = new LPCSTR[newArtifactsNumber];

    libc::memcpy(eventTable, originalTableAddress, sizeof(LPCSTR) * baseArtifactsNumber);
    // copy original data
    //  libc::memset(originalTableAddress, 0, sizeof(LPCSTR) * baseArtifactsNumber);
    for (size_t i = baseArtifactsNumber; i < newArtifactsNumber; i++)
    {
        eventTable[i] = "h3_NullString";
    }
    _PI->WriteDword(0x049F51B + 3, reinterpret_cast<DWORD>(&eventTable[0]));
    _PI->WriteDword(0x049F5D7 + 3, reinterpret_cast<DWORD>(&eventTable[0]));
    _PI->WriteDword(0x049F2E4 + 3, reinterpret_cast<DWORD>(&eventTable[0]));
}

void ArtifactsExtender::ReplaceCombinationArtifactsTable()
{
}

void ArtifactsExtender::ReplaceAIValuesTable()
{
    constexpr DWORD newArtifactsNumber = artifactsNumber.max;
    const DWORD baseArtifactsNumber = artifactsNumber.base;

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

void ArtifactsExtender::LoadArtifactPropertiesFromJson()
{
    constexpr DWORD newArtifactsNumber = artifactsNumber.max;

    for (size_t i = 0; i < newArtifactsNumber; i++)
    {
    }
}

void __stdcall ArtifactsExtender::H3GameMainSetup__LoadObjects(HiHook *h, H3MainSetup *setup)
{

    //  MessageBoxA(nullptr, "H3GameMainSetup__LoadObjects", "", MB_OK);

    // call native fucntion to load objects.txt (0x515038)
    // and increase eax by number of added objects
    THISCALL_1(void, h->GetDefaultFunc(), setup);
}

_ERH_(ArtifactsExtender::OnAfterWog)
{

    if (0)
    {
        auto &ins = ArtifactsExtender::Get();
        if (true)
        {
        }

        //    MessageBoxA(nullptr, "OnAfterWog", "", MB_OK);
        ins.AfterLoadingObjectTxtProc(0);
    }
}
_LHF_(ArtifactsExtender::LoadArtTraits)
{
    //  MessageBoxA(nullptr, "LoadArtTraits", "", MB_OK);

    return EXEC_DEFAULT;
}
ArtifactsExtender &ArtifactsExtender::Get()
{
    if (instance == nullptr)
    {
        instance = new ArtifactsExtender();
    }
    return *instance;
}

void FooBar(const int artsNum)
{
    H3LoadedDef *artifactsDef = H3LoadedDef::Load("artifact.def");
    // if we have def frames less than new artifact number
    if (artifactsDef && artifactsDef->groups[0]->count < artsNum)
    {
        artifactsDef->Dereference();
        const int currentFrames = artifactsDef->groups[0]->count;

        // for (size_t i = 0; i < currentFrames; i++)
        //{
        //     H3DefFrame* dummy = H3ObjectAllocator<H3DefFrame>().allocate();
        //     dummy = THISCALL_7(H3DefFrame*, 0x047BBD0, dummy, h3_NullString, frameWidth, frameHeight, data, dataSize,
        //         compressionType);
        //     artifactsDef->AddFrameToGroup(0, dummy);
        // }

        artifactsDef->groups[0]->spritesSize = artsNum;
        H3DefFrame *baseFrame = artifactsDef->groups[0]->frames[144];

        const int frameWidth = baseFrame->width;
        const int frameHeight = baseFrame->height;
        const int compressionType = baseFrame->compressionType;
        const int dataSize = baseFrame->dataSize;
        H3LoadedDef::DefGroup *newDefGroup = H3ObjectAllocator<H3LoadedDef::DefGroup>().allocate();
        // create missed frames
        newDefGroup->count = 0;
        newDefGroup->spritesSize = artsNum;

        //   //  H3DefFrame **defFreamesArray = H3ArrayAllocator<H3DefFrame*>().allocate(newArtifactsNumber);

        //     for (size_t i = 0; i < currentFrames; i++)
        //     {

        //         const BYTE *data = artifactsDef->groups[0]->frames[i]->rawData;

        //         THISCALL_7(H3DefFrame *, 0x047BBD0, &defFreamesArray[i],0,
        //                                           frameWidth, frameHeight, data, dataSize, compressionType);

        // THISCALL_2(H3DefFrame*, 0x47B1C0, newDefGroup,);
        //         // newDefGroup->frames[i] = defFreamesArray[i];
        //     }
        //     const BYTE *data = artifactsDef->groups[0]->frames[144]->rawData;

        //      for (size_t i = currentFrames; i < newArtifactsNumber; i++)
        //     {

        //         THISCALL_7(H3DefFrame *, 0x047BBD0, &defFreamesArray[i], baseFrame->GetName(),
        //                                         frameWidth, frameHeight, data, dataSize, compressionType);
        //       //  newDefGroup->frames[i] = defFreamesArray[i];
        //     }
        // newDefGroup->frames = defFreamesArray;
        //      artifactsDef->groups[0] = newDefGroup;

        //     // H3DefFrame* dummy = H3ObjectAllocator< H3DefFrame>().allocate();
        //     // dummy = THISCALL_6(H3DefFrame*, 0x047BBD0, dummy, artifactsDef);
        //     artifactsDef->AddFrameToGroup(0, dummy);
    }

    // ReplaceCombinationArtifactsTable();
    // ReplaceAIValuesTable();
    // resize the artifacts setups array

    // init reading of artifact fields from json
}

} // namespace artifacts
