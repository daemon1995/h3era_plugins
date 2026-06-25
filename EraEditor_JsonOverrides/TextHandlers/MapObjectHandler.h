#pragma once
class MapObjectHandler
{
  public:
    struct formats
    {
        static constexpr LPCSTR DWELLING1 = "era.dwellings1.%d";
        static constexpr LPCSTR DWELLING4 = "era.dwellings4.%d";
        static constexpr LPCSTR OBJECTS = "era.objects.%d";
    };

    static void Init();
    static int GetSubtypesAmount(const eObject type) noexcept;
};
