#include "pch.h"

using namespace h3;
typedef int _DWORD;
extern PatcherInstance *_PI;

inline int sub_718579(int a1, int a2)
{
    return CDECL_2(int, 0x718579, a1, a2);
}

inline int WoG_TError(int a1, int a2, int Txt)
{
    return CDECL_3(int, 0x0071248A, a1, a2, Txt);
}

inline int CrExpo_AddArt(_DWORD *_this_, _DWORD *a2)
{
    return THISCALL_2(int, 0x00727CF0, _this_, a2);
}

inline _DWORD *sub_718377(_DWORD *_this_)
{
    return THISCALL_1(_DWORD *, 0x718377, _this_);
}

inline int sub_7176A1(_DWORD *_this_)
{
    return THISCALL_1(int, 0x7176A1, _this_);
}

inline int CrExpo_SetN(_DWORD *_this_, int Type, int Data, __int16 CreatureID, int num, int expo, int arts)
{
    return THISCALL_7(int, 0x00717FE4, _this_, Type, Data, CreatureID, num, expo, arts);
}

inline int *RecalcExp2RealNum(int *a1, int a2, int a3, int a4)
{
    return FASTCALL_4(int *, 0x007183BF, a1, a2, a3, a4);
}

inline int CrExpoSet_Del(int a1, int a2)
{
    return CDECL_2(int, 0x0071866A, a1, a2);
}

inline int *CrExpoSet_Find(int a1, int a2)
{
    return CDECL_2(int *, 0x00718617, a1, a2);
}

inline void ErrorMessage(int a1, int a2, int a3)
{
    return CDECL_3(void, 0x00712333, a1, a2, a3);
}

inline char sub_44B2F0(void *_this_, int a2)
{
    return THISCALL_2(char, 0x0044B2F0, _this_, a2);
}

inline int *sub_718A66()
{
    return CDECL_0(int *, 0x00718A66);
}

inline int sub_768B90(void *_this_)
{
    return THISCALL_1(int, 0x00768B90, _this_);
}

#define aCrexposetHcomb 0x00791E4C
#define aNoMoreRoomsInE 0x0079F0CC

#define TemporaryVar_WoG_Battle (*(char *)0x00840D40)
#define M2M_DStr (*(int *)0x028326EC)
#define M2M_SStr (*(int *)0x02846A2C)

#define dword_860550 ((int *)0x00860550)
#define dword_860554 ((int *)0x00860554)
#define dword_860558 ((int *)0x00860558)
#define dword_86055C ((int *)0x0086055C)

#define dword_2846BF0 ((int *)0x2846BF0)
#define dword_2846C0C ((int *)0x2846C0C)
#define dword_2846C28 ((int *)0x2846C28)
#define dword_2846C44 ((int *)0x2846C44)

_LHF_(hook_007184B0)
{
    int *result = (int *)*(int *)(c->ebp - 4);
    int count = *(int *)(c->ebp + 8);

    result[0] = ((long long)result[1]) * (long long)(result[0]) / ((long long)count);
    result[1] = count;

    c->eax = (int)result;
    c->return_address = 0x7184D0;
    return NO_EXEC_DEFAULT;
}

signed int __cdecl sub_719A95_hook(HiHook *hook, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
    int i;         // [esp+Ch] [ebp-14h]
    int v10;       // [esp+10h] [ebp-10h]
    long long v11; // [esp+18h] [ebp-8h]
    int v12;       // [esp+1Ch] [ebp-4h]

    v12 = sub_718579(a1, a3);
    v10 = sub_718579(a2, a4);
    if (v12 == -1 && v10 == -1)
        return 0;
    if (!a2)
        return 0;
    for (i = 0;; ++i)
    {
        if (i >= 10000)
        {
            WoG_TError(15, 851, (int)aCrexposetHcomb);
            return -1;
        }
        if (!(dword_860558[4 * i] & 1))
            break;
    }
    sub_718377(&dword_860550[4 * i]);
    v11 = 0;
    if (v12 != -1)
    {
        RecalcExp2RealNum(&dword_860550[4 * v12], a7, a7, a5);
        sub_7176A1(&dword_860550[4 * v12]);
        v11 = (long long)a7 * dword_860550[4 * v12];
    }
    if (v10 != -1)
    {
        RecalcExp2RealNum(&dword_860550[4 * v10], a6, a8, a6);
        sub_7176A1(&dword_860550[4 * v10]);
        v11 += (long long)a8 * dword_860550[4 * v10];
    }
    CrExpo_SetN(&dword_860550[4 * i], a2, a4, a5, a8 + a7, v11 / (long long)(a8 + a7), 0);
    dword_860554[4 * i] = a8 + a7;
    if (v12 != -1)
        CrExpo_AddArt(&dword_860550[4 * i], &dword_860550[4 * v12]);
    if (v10 != -1)
        CrExpo_AddArt(&dword_860550[4 * i], &dword_860550[4 * v10]);
    if (v12 != -1)
        sub_718377(&dword_860550[4 * v12]);
    if (v10 != -1)
        sub_718377(&dword_860550[4 * v10]);
    return 0;
}

int __cdecl sub_7198C1_hook(HiHook *hook, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
    int v9;  // [esp+Ch] [ebp-8h]
    int v10; // [esp+10h] [ebp-4h]

    v10 = sub_718579(a1, a3);
    v9 = sub_718579(a2, a4);
    if (v10 != -1)
    {
        RecalcExp2RealNum(&dword_860550[4 * v10], a7, a7, a5);
        dword_860558[4 * v10] = 2 * (a2 & 0xF) | dword_860558[4 * v10] & 0xFFFFFFE1;
        dword_86055C[4 * v10] = a4;
        if (dword_860554[4 * v10])
        {
            sub_7176A1(&dword_860550[4 * v10]);
            dword_860550[4 * v10] =
                ((long long)a7) * ((long long)dword_860550[4 * v10]) / ((long long)dword_860554[4 * v10]);
            dword_860554[4 * v10] = a7;
        }
        else
        {
            CrExpoSet_Del(a2, a4);
        }
    }
    if (v9 != -1)
    {
        RecalcExp2RealNum(&dword_860550[4 * v9], a6, a8, a6);
        dword_860558[4 * v9] = 2 * (a1 & 0xF) | dword_860558[4 * v9] & 0xFFFFFFE1;
        dword_86055C[4 * v9] = a3;
        if (dword_860554[4 * v9])
        {
            sub_7176A1(&dword_860550[4 * v9]);
            dword_860550[4 * v9] =
                ((long long)a8) * ((long long)dword_860550[4 * v9]) / ((long long)dword_860554[4 * v9]);
            dword_860554[4 * v9] = a8;
        }
        else
        {
            CrExpoSet_Del(a1, a3);
        }
    }
    return 0;
}

_LHF_(hook_007662CD)
{
    long long result = ((long long)c->eax) + (((long long)c->edx) * ((long long)c->ecx));
    result /= (long long)*(int *)(c->ebp - 0x10);
    c->eax = result;
    *(int *)(c->ebp - 0x20) = c->eax;
    c->return_address = 0x007662D9;
    return NO_EXEC_DEFAULT;
}

_LHF_(hook_00766335)
{
    long long result = ((long long)c->eax) + (((long long)c->edx) * ((long long)c->ecx));
    result /= (long long)*(int *)(c->ebp - 0x68);
    c->eax = result;
    *(int *)(c->ebp - 0x3C) = c->eax;
    c->return_address = 0x00766341;
    return NO_EXEC_DEFAULT;
}

void __cdecl sub_7595F9_hook(HiHook *hook)
{
    int v0;           // eax
    int v1;           // eax
    long long v2;     // [esp+Ch] [ebp-11Ch]
    long long v3;     // [esp+10h] [ebp-118h]
    long long v4;     // [esp+14h] [ebp-114h]
    long long v5;     // [esp+1Ch] [ebp-10Ch]
    long long v6;     // [esp+1Ch] [ebp-10Ch]
    long long v7;     // [esp+1Ch] [ebp-10Ch]
    long long v8;     // [esp+20h] [ebp-108h]
    long long v9[7];  // [esp+28h] [ebp-100h] BYREF
    long long v10[7]; // [esp+44h] [ebp-E4h]
    long long v11[7]; // [esp+60h] [ebp-C8h] BYREF
    long long v12[7]; // [esp+7Ch] [ebp-ACh]
    long long v13[7]; // [esp+98h] [ebp-90h] BYREF
    long long v14[7]; // [esp+B4h] [ebp-74h]
    long long v15[7]; // [esp+D0h] [ebp-58h] BYREF
    long long v16[8]; // [esp+ECh] [ebp-3Ch]
    long long *v17;   // [esp+10Ch] [ebp-1Ch]
    int i;            // [esp+110h] [ebp-18h]
    long long v19;    // [esp+114h] [ebp-14h]
    int *v20;         // [esp+118h] [ebp-10h]
    int v21;          // [esp+11Ch] [ebp-Ch]
    int v22;          // [esp+120h] [ebp-8h]
    int v23;          // [esp+124h] [ebp-4h]

    v4 = M2M_DStr - 145;
    v21 = *(unsigned __int8 *)(M2M_SStr - 224 + 5);
    v22 = *(unsigned __int8 *)(M2M_SStr - 224 + 6);
    v3 = *(unsigned __int8 *)(M2M_SStr - 224 + 7);
    v23 = *(_DWORD *)(M2M_DStr - 145 + 26);
    for (i = 0; i < 7; ++i)
    {
        v20 = (int *)CrExpoSet_Find(3, (i << 17) + (v3 << 16) + (v22 << 8) + v21);
        if (v20)
        {
            RecalcExp2RealNum(v20, i, dword_2846C0C[i], dword_2846BF0[i]);
            if (v20[1])
                v5 = *v20;
            else
                v5 = 0;
        }
        else
        {
            v5 = 0;
        }
        v13[i] = dword_2846BF0[i];
        v9[i] = v13[i];
        v10[i] = dword_2846C0C[i] * 2 * v5 + 1;
        if (v20)
        {
            v0 = sub_768B90(v20);
            v14[i] = 16 * v0 + 1;
        }
        else
        {
            v14[i] = 1;
        }
        if (v9[i] == -1)
            v10[i] = 0;
        if (v13[i] == -1)
            v14[i] = 0;
    }
    for (i = 0; i < 7; ++i)
    {
        v20 = (int *)CrExpoSet_Find(1, (i << 16) + v23);
        if (v20)
        {
            RecalcExp2RealNum(v20, dword_2846C28[i], dword_2846C44[i], dword_2846C28[i]);
            if (v20[1])
                v6 = *v20;
            else
                v6 = 0;
        }
        else
        {
            v6 = 0;
        }
        v15[i] = dword_2846C28[i];
        v11[i] = v15[i];
        v12[i] = dword_2846C44[i] * 2 * v6 + 1;
        if (v20)
        {
            v1 = sub_768B90(v20);
            v16[i] = 16 * v1 + 1;
        }
        else
        {
            v16[i] = 1;
        }
        if (v11[i] == -1)
            v12[i] = 0;
        if (v15[i] == -1)
            v16[i] = 0;
    }
    v17 = v9;
    TemporaryVar_WoG_Battle = sub_44B2F0(v11, (int)v9);
    if (TemporaryVar_WoG_Battle)
    {
        v17 = v13;
        TemporaryVar_WoG_Battle = sub_44B2F0(v15, (int)v13);
        for (i = 0; i < 7; ++i)
        {
            CrExpoSet_Del(3, (i << 17) + (v3 << 16) + (v22 << 8) + v21);
            CrExpoSet_Del(1, (i << 16) + v23);
            v2 = *(_DWORD *)(v4 + 4 * i + 145);
            v19 = *(_DWORD *)(v4 + 4 * i + 173);
            v7 = v12[i] / 2;
            v8 = v16[i] / 16;
            if (v2 != -1 && v19)
            {
                v20 = sub_718A66();
                if (!v20)
                {
                    ErrorMessage(6, 1791, (int)aNoMoreRoomsInE);
                    return;
                }
                CrExpo_SetN(v20, 1, (i << 16) + v23, v2, v19, v7 / v19, v8);
            }
        }
    }
}

void CreExpoFix_Apply()
{
    _PI->WriteLoHook(0x007184B0, hook_007184B0);
    _PI->WriteHiHook(0x00719A95, CDECL_, sub_719A95_hook);
    _PI->WriteHiHook(0x007198C1, CDECL_, sub_7198C1_hook);
    _PI->WriteLoHook(0x007662CD, hook_007662CD);
    _PI->WriteLoHook(0x00766335, hook_00766335);
    _PI->WriteHiHook(0x007595F9, CDECL_, sub_7595F9_hook);
}
