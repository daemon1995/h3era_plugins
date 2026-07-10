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

OBJECT_EXTENDER_DECLARATOR(ColosseumOfTheMagiExtender, colosseumOfTheMagi)
OBJECT_EXTENDER_DECLARATOR(CreatureBanksExtender, cbanks)
OBJECT_EXTENDER_DECLARATOR(GazeboExtender, gazebo)
OBJECT_EXTENDER_DECLARATOR(ShrinesExtender, shrines)
OBJECT_EXTENDER_DECLARATOR(SpellMarketExtender, spellMarket)
OBJECT_EXTENDER_DECLARATOR(UniversityExtender, university)
OBJECT_EXTENDER_DECLARATOR(WarehousesExtender, warehouses)
OBJECT_EXTENDER_DECLARATOR(WateringPlaceExtender, wateringPlace)
OBJECT_EXTENDER_DECLARATOR(WoGObjectsExtender, wog)

namespace ExtendersInitializer
{

void InitObjectExtenders()
{

    if (auto mgr = extender::ObjectExtenderManager::Get())
    {
        extender::ObjectExtender *extendersList[] = {
            OBJECT_EXTENDER_GETTER(ColosseumOfTheMagiExtender, colosseumOfTheMagi),
            OBJECT_EXTENDER_GETTER(CreatureBanksExtender, cbanks), OBJECT_EXTENDER_GETTER(GazeboExtender, gazebo),
            OBJECT_EXTENDER_GETTER(ShrinesExtender, shrines), OBJECT_EXTENDER_GETTER(SpellMarketExtender, spellMarket),
            OBJECT_EXTENDER_GETTER(UniversityExtender, university),
            OBJECT_EXTENDER_GETTER(WarehousesExtender, warehouses),
            // OBJECT_EXTENDER_GETTER(WateringPlaceExtender, wateringPlace),
            OBJECT_EXTENDER_GETTER(WoGObjectsExtender, wog)};

        constexpr size_t extendersCount =
            std::size(extendersList); // sizeof(extendersList) / sizeof(extender::ObjectExtender*);

        static_assert(extendersCount == 8, "Unexpected number of extenders");
        //! Get the extenders and initialize
        for (size_t i = 0; i < extendersCount; i++)
        {
            mgr->AddExtender(extendersList[i]);
        }
    }
}

} // namespace ExtendersInitializer
