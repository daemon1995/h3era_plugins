#pragma once
class ArtifactHandler
{
  public:
    struct formats
    {
        static constexpr LPCSTR NAME = "era.artifacts.%d.name";
        static constexpr LPCSTR DESCRIPTION = "era.artifacts.%d.description";
        static constexpr LPCSTR EVENT = "era.artifacts.%d.event";
    };
    static void Init();
    static LPCSTR *GetEventTable() noexcept;
};
