#include "pch.h"
#define OBJECT_EXTENDER_DECLARATOR(className, nameSpaceName)                                                           \
    namespace nameSpaceName                                                                                            \
    {                                                                                                                  \
    class className : public extender::ObjectExtender                                                                  \
    {                                                                                                                  \
      public:                                                                                                          \
        static className &className::Get();                                                                            \
    };                                                                                                                 \
    }
#define OBJECT_EXTENDER_GETTER(className, nameSpaceName) &nameSpaceName::className::Get()

OBJECT_EXTENDER_DECLARATOR(ShrinesExtender, shrines)
OBJECT_EXTENDER_DECLARATOR(SpellMarketExtender, spellMarket)
OBJECT_EXTENDER_DECLARATOR(UniversityExtender, university)

namespace ExtendersInitializer
{

void InitObjectExtenders()
{
    auto &mgr = extendersManager::ObjectExtenderManager::Get();
    extender::ObjectExtender *extendersList[] = {OBJECT_EXTENDER_GETTER(ShrinesExtender, shrines),
                                                 OBJECT_EXTENDER_GETTER(SpellMarketExtender, spellMarket),
                                                 OBJECT_EXTENDER_GETTER(UniversityExtender, university)

    };

    constexpr size_t extendersCount =
        std::size(extendersList); // sizeof(extendersList) / sizeof(extender::ObjectExtender*);

    // static_assert(extendersCount == 8, "Unexpected number of extenders");
    //! Get the extenders and initialize
    for (size_t i = 0; i < extendersCount; i++)
    {
        extendersList[i]->Register();
        //  mgr->AddExtender(extendersList[i]);
    }
}

} // namespace ExtendersInitializer
