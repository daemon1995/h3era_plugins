#include "pch.h"

#include <sstream>
#include <unordered_set>

BOOL AdditionalProperties::AddProperty(std::string &other) noexcept
{
    std::transform(other.begin(), other.end(), other.begin(), ::tolower);

    return propertiesMap.insert(std::make_pair(GetMapKey(other.data()), other)).second;
}

std::string AdditionalProperties::GetMapKey(LPCSTR propertyString) noexcept
{
    std::istringstream stream(propertyString);
    std::vector<std::string> words;
    std::string word;
    words.reserve(9);
    // ��������� ������ �� ����� � ��������� �� � ������
    while (stream >> word)
    {
        words.emplace_back(word);
    }
    // get map key from def + object type + object subtype
    std::transform(words[0].begin(), words[0].end(), words[0].begin(), ::tolower);
    libc::sprintf(h3_TextBuffer, UNIQUE_PROPERTY_FORMAT, words[0].c_str(), words[5].c_str(), words[6].c_str());
    words.clear();
    return h3_TextBuffer;
}
// check if we have replaced property for that object
const std::string *AdditionalProperties::FindPropertyReplace(LPCSTR other) noexcept
{

    auto it = propertiesMap.find(GetMapKey(other));
    return it != propertiesMap.end() ? &it->second : nullptr;
}

void AdditionalProperties::LoadCommonProperies() noexcept
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

        const size_t maxSubtype = objType == eObject::ARTIFACT || objType == eObject::CREATURE_GENERATOR1
                                      ? extender::limits::EXTENDED
                                      : extender::limits::COMMON;

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
    modList::GetEraModList(modList, true);

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
                // Era::WriteStrToIni((*objectTxt)[i], propertyReplace->data(), "tryingToReplace",
                //                    "runtime/tum/properties.ini");
                (*objectTxt)[i] = propertyReplace->data();
            }
        }

        // copy original objects added list into set
        std::unordered_set<LPCSTR> objectsSet(objectTxt->begin(), objectTxt->end());

        // iterate each added property
        for (auto &prop : propertiesMap)
        {
            // Era::WriteStrToIni(prop.first.c_str(), prop.second.c_str(), "objectsExtender",
            //                    "runtime/tum/properties.ini");

            // if possible to insert
            if (objectsSet.insert(prop.second.c_str()).second)
            {

                // add that property into main objects list
                objectTxt->AddLine(prop.second.c_str()); // add new txt entry
                // increase added objects number
                addedProperties++;
            }
        }
        // Era::SaveIni("runtime/tum/properties.ini");
        // Era::SaveIni("runtime/tum/properties.ini");
    }
    return addedProperties;
}
