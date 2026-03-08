#pragma once
#include <cstdint>
#include <initializer_list>

#ifdef EXT_API_EXPORTS
#define EXT_API __declspec(dllexport)
#else
#define EXT_API __declspec(dllimport)
#endif

// ------------------------
// ������/���� ����������
// ------------------------
enum ObjectExtenderMethods : int
{
    AFTER_LOADING_OBJECTS_TXT = 0,
    CREATE_RMG_OBJECT_GEN,
    NEW_GAME_OBJECT_ITERATION,
    NEW_WEEK_OBJECT_ITERATION,
    HERO_MAP_ITEM_VISIT,
    SET_HINT_IN_H3_TEXT_BUFFER,
    AI_MAP_ITEM_WEIGHT,
    RMG_DLG_SHOW_CUSTOM_OBJECT_HINT,
    METHODS_COUNT
};
#ifndef METHOD_BIT
#define METHOD_BIT(method) (1u << ObjectExtenderMethods::method)
#endif // !METHOD_BIT

// ------------------------
// Константы для dispatch-table
// ------------------------
constexpr int MAX_TYPES = 256;    // Максимум типов объектов
constexpr int MAX_SUBTYPES = 256; // Максимум подтипов объекта

// Универсальный указатель на функцию hook
using HookFn = void *;

// Структура для dispatch-cell
struct DispatchCell
{
    HookFn methods[ObjectExtenderMethods::METHODS_COUNT] = {};
};

// ------------------------
// ���� ������� ��� ����� (__stdcall ��� ABI)
// ------------------------
using SetAiMapItemWeightFn = void(__stdcall *)(int objectType, int subtype, void *context);
using VisitMapItemFn = void(__stdcall *)(int objectType, int subtype, void *context);
using SetHintFn = void(__stdcall *)(int objectType, int subtype, void *context);

// ------------------------
// ������� ���������� ����������
// ------------------------
struct ObjectMatchRule
{
    int objectType = -1;           // ��� �������
    const int *subtypes = nullptr; // nullptr = ��� �������
    uint16_t subtypeCount = 0;     // 0 ���� ��� �������

    // ����������� ��� ������ ���� � ���� ��������
    constexpr ObjectMatchRule(int type) noexcept : objectType(type), subtypes(nullptr), subtypeCount(0)
    {
    }

    // ����������� ��� ���� + ������ ��������
    constexpr ObjectMatchRule(int type, const int *sts, uint16_t count) noexcept
        : objectType(type), subtypes(sts), subtypeCount(count)
    {
    }
};

// ------------------------
// Descriptor ����������
// ------------------------
struct ObjectExtenderDesc
{
    uint32_t api_version = 1; // ������ API

    // ������� ���������� (������) ��� nullptr
    const ObjectMatchRule *rules = nullptr;
    uint16_t ruleCount = 0;

    // ������, ������� ����������� (������������� ����� SetMethods)
    uint32_t methods_mask = 0;

    // Callback-�������
    SetAiMapItemWeightFn set_ai_weight = nullptr;
    VisitMapItemFn visit = nullptr;
    SetHintFn set_hint = nullptr;

    void *user_data = nullptr;

    // ------------------------
    // Helper ��� �������
    // ------------------------
    void SetMethods(std::initializer_list<ObjectExtenderMethods> methods)
    {
        methods_mask = 0;
        for (int m : methods)
            methods_mask |= (1u << m);
    }
};

// ------------------------
// ����������� ���������� (���������� � ObjectExtenderManager.cpp)
// ------------------------
extern "C"
{
    // EXT_API bool __stdcall RegisterObjectExtender(const ObjectExtenderDesc *desc);
    // EXT_API const char *__stdcall GetResolvedObjectName(const int objectType, const int subtype);
}
struct ObjectExtenderAPI
{
    uint32_t api_version;

    // -------- сервисные функции --------

    const char *(__stdcall *GetResolvedObjectName)(int objectType, int subtype);

    // позже:
    // H3RmgObjectGenerator* (__stdcall *CreateDefaultRMGObjectGenerator)(...);
    // int (__stdcall *GetResolvedAiWeight)(...);
};
constexpr uint32_t OBJECT_EXTENDER_API_VERSION = 1;

extern "C" EXT_API const ObjectExtenderAPI *__stdcall GetObjectExtenderAPI();

static const ObjectExtenderAPI *g_api = nullptr;

static void InitObjectExtenderAPI()
{
    HMODULE h = GetModuleHandleA("RMG_CustomizeObjectProperties.era");

    if (!h)
        h = LoadLibraryA("RMG_CustomizeObjectProperties.era");

    if (!h)
        return;

    using GetApiFn = const ObjectExtenderAPI *(__stdcall *)();

    auto getApi = (GetApiFn)GetProcAddress(h, "GetObjectExtenderAPI");

    if (!getApi)
        return;

    const ObjectExtenderAPI *api = getApi();

    if (!api)
        return;

    if (api->api_version != OBJECT_EXTENDER_API_VERSION)
        return; // версия не совпала

    g_api = api;
}
