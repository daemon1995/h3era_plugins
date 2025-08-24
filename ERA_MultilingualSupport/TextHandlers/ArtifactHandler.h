#pragma once

#define DECLARE_JSON_FIELD(field) static constexpr LPCSTR field = "era.artifacts.%d." #field;

class ArtifactHandler
{
  public:
    struct formats
    {
        DECLARE_JSON_FIELD(name)
        DECLARE_JSON_FIELD(description)
        DECLARE_JSON_FIELD(event)
        DECLARE_JSON_FIELD(cost)
        DECLARE_JSON_FIELD(type)
        DECLARE_JSON_FIELD(comboID)
        DECLARE_JSON_FIELD(combinationArtifactId)
        DECLARE_JSON_FIELD(disabled)
        DECLARE_JSON_FIELD(newSpell)
    };
    static void Init();
    template <class T>
    static void ReadField(T& target, const char* format, int idx) noexcept;
    //template <>
    //static void ReadField<LPCSTR>(LPCSTR& target, const char* format, int idx) noexcept;
    //template <>
    //static void ReadField<INT>(INT& target, const char* format, int idx) noexcept;

    static LPCSTR *GetEventTable() noexcept;
    static int GetArtifactsNumber() noexcept;
};
