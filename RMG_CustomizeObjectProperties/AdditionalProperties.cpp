#include "pch.h"

#include <unordered_set>

BOOL AdditionalProperties::AddProperty(std::string &other) noexcept
{
    std::transform(other.begin(), other.end(), other.begin(), ::tolower);

    return propertiesMap.insert(std::make_pair(GetMapKey(other.data()), other)).second;
}

std::string AdditionalProperties::GetMapKey(LPCSTR propertyString) noexcept
{
    // Extract only the three fields used by the key. Using istringstream here
    // pulled the complete iostream/locale implementation into the plugin.
    std::string words[7];
    size_t wordIndex = 0;
    const char *current = propertyString;

    while (*current && wordIndex <= 6)
    {
        while (*current == ' ' || *current == '\t' || *current == '\r' || *current == '\n' || *current == '\v' ||
               *current == '\f')
        {
            ++current;
        }

        if (!*current)
        {
            break;
        }

        const char *wordBegin = current;
        while (*current && *current != ' ' && *current != '\t' && *current != '\r' && *current != '\n' &&
               *current != '\v' && *current != '\f')
        {
            ++current;
        }

        if (wordIndex == 0 || wordIndex == 5 || wordIndex == 6)
        {
            words[wordIndex].assign(wordBegin, static_cast<size_t>(current - wordBegin));
        }
        ++wordIndex;
    }
    // get map key from def + object type + object subtype
    std::transform(words[0].begin(), words[0].end(), words[0].begin(), ::tolower);
    libc::sprintf(h3_TextBuffer, UNIQUE_PROPERTY_FORMAT, words[0].c_str(), words[5].c_str(), words[6].c_str());
    return h3_TextBuffer;
}
// check if we have replaced property for that object
const std::string *AdditionalProperties::FindPropertyReplace(LPCSTR other) noexcept
{

    auto it = propertiesMap.find(GetMapKey(other));
    return it != propertiesMap.end() ? &it->second : nullptr;
}

void AdditionalProperties::LoadCommonProperties() noexcept
{
    bool readSuccess = false;

    // load properties for direct type/subtype objects
    for (size_t objType = 0; objType < h3::limits::OBJECTS; objType++)
    { // iterate all the objects types entries

        /**
        // probably will be used later for extended object types
        size_t maxSubtype;
        switch (objType)
        {
        case eObject::ARTIFACT:
        case eObject::CREATURE_GENERATOR1:
        case eObject::MONSTER:
            maxSubtype = limits::EXTENDED;
            break;

        default:
            maxSubtype = limits::COMMON;
            break;
        }
        */

        const size_t maxSubtype = objType == eObject::CREATURE_GENERATOR1 ? extendersManager ::limits::EXTENDED
                                                                          : extendersManager ::limits::COMMON;
        for (size_t objSubtype = 0; objSubtype < maxSubtype; objSubtype++)
        {
            size_t propertyIdCounter = 0;

            do
            {
                LPCSTR strPtr = EraJS::read(
                    H3String::Format(PROPERTY_COMMON_JSON, objType, objSubtype, propertyIdCounter++).String(),
                    readSuccess);

                if (readSuccess)
                {
                    std::string str = strPtr;
                    AddProperty(str);
                }
            } while (readSuccess);
        }
    }
}

void AdditionalProperties::LoadAdditionalPropertiesFromMods() noexcept
{

    std::vector<std::string> modList;
    modList::GetEraModList(modList, modList::CASE_TO_LOWER);
    std::reverse(modList.begin(), modList.end());
    for (auto &modName : modList)
    {
        //    std::string modName = "wog";
        bool readSuccess = false;

        int propertyIdCounter = 0;

        // first read raw propertiese array
        do
        {
            LPCSTR strPtr = EraJS::read(
                H3String::Format(PROPERTY_TYPE_JSON, modName.c_str(), propertyIdCounter++).String(), readSuccess);
            if (readSuccess)
            {
                std::string str = strPtr;
                AddProperty(str);
            }
        } while (readSuccess);

        // later read array of arrays
        int arrayCounter = 0;
        bool breakFlag = false;
        // reset objects counter
        propertyIdCounter = 0;
        do
        {
            do
            {
                LPCSTR strPtr = EraJS::read(
                    H3String::Format(PROPERTY_SUBTYPE_JSON, modName.c_str(), arrayCounter, propertyIdCounter).String(),
                    readSuccess);
                if (readSuccess)
                {
                    std::string str = strPtr;
                    AddProperty(str);
                    propertyIdCounter++;
                }
                else if (propertyIdCounter)
                {
                    propertyIdCounter = 0;
                    arrayCounter++;
                    break;
                }
                else
                {
                    breakFlag = true;
                    break;
                }

            } while (true);

        } while (!breakFlag);
    }
}

void AdditionalProperties::DebugAddedProperties() const noexcept
{
    const char *fileName = "Runtime/Debug/AdditionalProperties.ini";
    Era::EmptyIniCache(fileName);
    for (auto &key : propertiesMap)
    {
        Era::WriteStrToIni(key.first.c_str(), key.second.c_str(), "AdditionalPropertiesMap", fileName);
    }
    Era::SaveIni(fileName);
}

size_t AdditionalProperties::InsertPropertiesIntoObjectsList(H3TextFile *_objectsTxt) noexcept
{
    size_t addedProperties = 0;
    if (propertiesMap.size())
    {
        EditableH3TextFile *objectTxt = static_cast<EditableH3TextFile *>(_objectsTxt);

        const UINT linesCount = objectTxt->GetLineCount();
        for (size_t i = 1; i < linesCount; i++)
        {
            // create buffer string to transform it
            std::string txtPropertyString((*objectTxt)[i]);

            // std::transform(txtPropertyString.begin(), txtPropertyString.end(), txtPropertyString.begin(), ::tolower);

            if (const std::string *propertyReplace = FindPropertyReplace(txtPropertyString.c_str()))
            {
                (*objectTxt)[i] = propertyReplace->data();
            }
        }

        // copy original objects added list into set
        std::unordered_set<LPCSTR> objectsSet(objectTxt->begin(), objectTxt->end());

        // iterate each added property
        for (auto &prop : propertiesMap)
        {

            // if possible to insert
            if (objectsSet.insert(prop.second.c_str()).second)
            {

                // add that property into main objects list
                objectTxt->AddLine(prop.second.c_str()); // add new txt entry
                // increase added objects number
                addedProperties++;
            }
        }
        // static int counter = 0;
        // char *fileName = "runtime/Debug/list1.ini";
        // if (!counter)
        //{

        //    counter++;
        //}
        // else
        //{
        //    fileName = "runtime/Debug/list2.ini";
        //}
        //{
        //    libc::sprintf(h3_TextBuffer, "Added %d new properties into objects.txt", _objectsTxt->Size());
        //    MessageBoxA(NULL, h3_TextBuffer, "Debug", MB_OK);
        //}

        // Era::WriteStrToIni("0", _objectsTxt->GetName(), "objectsExtender", fileName);
        // for (size_t i = 1; i < _objectsTxt->Size(); i++)
        //{
        //     Era::WriteStrToIni(Era::IntToStr(i).c_str(), _objectsTxt->GetText(i), "objectsExtender", fileName);
        // }

        // Era::SaveIni(fileName);
    }
    return addedProperties;
}
