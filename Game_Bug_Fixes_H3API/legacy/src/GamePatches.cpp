/////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// ПАТЧИ ////////////////////////////////////////////////
#include "src\HdModFixes.cpp"

_LHF_(Y_BeforeWog)
{
    if (_P->GetInstance(BATTLE_DLG_NAME))
        BATTLE_DLG_PLUGIN = true;

    return EXEC_DEFAULT;
}
// Разрешение в HD.
_int_ HD_Res_X;
_int_ HD_Res_Y;

// Координаты боя в HD.
_int_ HD_Battle_X;
_int_ HD_Battle_Y;

_LHF_(Y_AftereWog)
{

    // © daemon_n
    // фикс вылета игры при клике пкм в жертвенном алтаре на кнопку "Следующий артефакт"
    // Баг добавлен WoG - помещён указатель на мусор вместо текста
    _P->UndoAllAt(0x05641A4);

    HdMod::HdModFixes(_P, _PI);
    // Получение координат поля боя.
    HD_Battle_X = _P->VarGetValue<_int32_>("HD.Battle.X", 0);
    HD_Battle_Y = _P->VarGetValue<_int32_>("HD.Battle.Y", 0);

    // Получение разрешения (необходимо получать непосредственно в момент, когда требуется доступ).
    HD_Res_X = _P->VarGetValue<_int32_>("HD.Res.X", 800);
    HD_Res_Y = _P->VarGetValue<_int32_>("HD.Res.Y", 600);

    return EXEC_DEFAULT;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void SetPathes(PatcherInstance *_PI)
{
    _PI->WriteLoHook(0x4EDD65, Y_BeforeWog);
    _PI->WriteLoHook(0x4EE1B4, Y_AftereWog);
}
