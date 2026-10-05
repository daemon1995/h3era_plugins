
//=================================================
namespace WoG
{

enum eExpType
{
    CE_UNKNOWN = 0x0,
    CE_FIRST = 0x1,
    CE_HERO = CE_FIRST,
    CE_MAP = 0x2,
    CE_TOWN = 0x3,
    CE_MINE = 0x4,
    CE_HORN = 0x5,
    CE_LAST = CE_HORN,
};

__declspec(align(1)) struct CrExpModStr
{
    unsigned __int32 Act : 1;
    unsigned __int32 _unused : 31;
    char Type;
    char Mod;
    char Lvls[11];
};

struct CrExpBon
{
    static constexpr DWORD CR_EXP_BONUS_TABLE = 0x0847D98;
    static constexpr int CR_EXP_BONUS_LINES = 20;

  public:
    inline static void MakeCurrent(int stackId, int MType)
    {
        CDECL_2(void, 0x728120, stackId, MType);
    }
    inline static CrExpModStr *GetBonusLine(int lineId)
    {
        return reinterpret_cast<WoG::CrExpModStr *>(CR_EXP_BONUS_TABLE + lineId * 17);
    }
};

struct _CrExpo_
{
    _dword_ experience; // тек. опыт на 1 существо
    _dword_ number;     // число существ
    struct
    {
        unsigned __int32 Act : 1;
        unsigned __int32 Type : 4;
        unsigned __int32 creatureId : 8;
        unsigned __int32 mHasArt : 1;
        unsigned __int32 mArt : 2;
        unsigned __int32 mCopyArt : 2;
        unsigned __int32 mSubArt : 4;
        unsigned __int32 _un : 10;
    } flags;

    union UniData {
        DWORD uniData;
        DWORD mixPos;
        struct
        {
            __int16 id;
            __int16 slot;
        } hero;
        struct
        {
            unsigned __int32 x : 8;
            unsigned __int32 y : 8;
            unsigned __int32 z : 1;
            unsigned __int32 _un : 15;
        } map;
        union {
        };
        struct
        {
            unsigned __int32 x : 8;
            unsigned __int32 y : 8;
            unsigned __int32 z : 1;
            __int32 slot : 15;
        } town;
        struct
        {
            unsigned __int32 x : 8;
            unsigned __int32 y : 8;
            unsigned __int32 z : 1;
            __int32 slot : 15;
        } mine;
        struct
        {
            unsigned __int32 x : 8;
            unsigned __int32 y : 8;
            unsigned __int32 z : 1;
            __int32 slot : 15;
        } garrison;
        struct
        {
            unsigned __int32 x : 8;
            unsigned __int32 y : 8;
            unsigned __int32 z : 1;
            __int32 slot : 15;
        } anyGarrison;
    } place;

  public:
    static inline _CrExpo_ *Get()
    {
        return reinterpret_cast<_CrExpo_ *>(0x860550);
    }
    static inline _CrExpo_ *GetFromCombatCreatureIndex(const int index)
    {
        return CDECL_1(_CrExpo_ *, 0x728110, index);
    }
    static inline _CrExpo_ *Find(const eExpType type, const UniData data)
    {
        return CDECL_2(_CrExpo_ *, 0x0718617, type, data.uniData);
    }
    inline void Clear()
    {
        return THISCALL_1(void, 0x0718377, this);
    };
    inline void Clamp()
    {
        experience = h3::Clamp(0, experience, GetCreatureExpLimit(flags.creatureId));
    }
    inline void Clamp(const int monId)
    {
        experience = h3::Clamp(0, experience, GetCreatureExpLimit(monId));
    }
    static inline int SetNewAndClamp(const eExpType type, const UniData data, const int creatureId,
                                     const int creaturesNum, const int expo)
    {
        return CDECL_5(int, 0x0718AD0, type, data.uniData, creatureId, creaturesNum, expo);
    }
    static inline int GetCreatureExpByRankAndExp(const int monId, const int exp, const int additional)
    {
        return CDECL_3(int, 0x0727E40, monId, exp, additional);
    }
    static inline int GetCreatureExpLimit(const int monId)
    {
        return CDECL_1(int, 0x0727E20, monId);
    }
    static inline float GetCreatureUpgradeMultiplier(const int monId)
    {
        return CDECL_1(float, 0x0727E00, monId);
    }
};

struct _CreatureExpo_
{
    char *Caption;        // заголовок диалога
    char *Info;           // информация о твари
    char *Picture;        // изображение твари
    char *PictureHint;    // хинт к изображению твари
    char **TxtProperties; // [8] текстовые доп. свойства
    union {
        char **IcoProperties;  // [8] иконки доп. свойств, "NONE" - для отображения заглушки
        int *IcoPropertiesInt; // [8] иконки доп. свойств, "NONE" - для отображ
    };
    char **HintProperties;  // [8] подсказки к  доп. свойствам
    char *ColCaptions;      // заголовки столбцов - одна строка, по 7 символов на столбец (11*7)
    char *ColHint;          // заголовки столбцов - подсказки
    char **RowCaptions;     // [16] заголовки строк
    char **RowCaptionHints; // [16] заголовки строк, подсказки
    char **Rows;            // [16] значения ячеек таблицы - 16 строк по 7 символов на столбец (11*7)
    char **RowHints;        // [16] подсказки к строкам
    int IcoPropertiesCount; // [0 - 8] количество иконок доп. свойств
    int ShowSpecButton;     // показывать кнопку
    char *SpecButtonHint;   // подсказка к кнопке
    int CurPropColLeft;     // начало столбца с текущими координатами твари в символах
    int CurPropColRight;    // конец столбца с текущими координатами твари в символах
    char *ArtIcon;          // иконка артифакта ("NONE" - пустой)
    char *ArtHint;          // хинт для иконки артифакта
    int ArtOutput;          // артифакт отдается - 1, оставляется - 0
    int Flags;              // флаги - см. ниже
    // Flags & 0x00000001 - можно передавать артефакт
    int ArtCopy; // значения:
    // 0,... - значения
    // <0 - скрыть
    // вызов ф-ции
    int (*SpecButtonPush)(); // указатель на ф-цию нажатия кнопки.
    // Возвр. значения: 0 - ничего
    //                  1 - закрыть вызвавший диалог
    int (*OnControlClick)(int ContorIndex, int MouseButton, int ShiftState); // указатель на ф-цию клика по коннтролу
};

//=================================================

struct _Sphinx1_
{
    char *Text1;     // Текст №1 сообщения/вопроса
    char *Text2;     // Текст №2 сообщения/вопроса
    char *Text3;     // Текст №3 сообщения/вопроса
    char *Text4;     // Текст ответа (возвращается)
    long SelItm;     // Номер выбранного итема 1-4 (возвращается)
    char *Pic1Path;  // путь до первой картинки - если NULL, то не показывается
    char *Pic2Path;  // путь до второй картинки - если NULL, то не показывается
    char *Pic3Path;  // путь до третьей картинки - если NULL, то не показывается
    char *Pic4Path;  // путь до четвертой картинки - если NULL, то не показывается
    char *Pic1Hint;  // пояснение к картинке №1
    char *Pic2Hint;  // пояснение к картинке №2
    char *Pic3Hint;  // пояснение к картинке №3
    char *Pic4Hint;  // пояснение к картинке №4
    char *Chk1Text;  // Текст для кнопки №1 - если NULL, то не показывается
    char *Chk2Text;  // Текст для кнопки №2 - если NULL, то не показывается
    char *Chk3Text;  // Текст для кнопки №3 - если NULL, то не показывается
    char *Chk4Text;  // Текст для кнопки №4 - если NULL, то не показывается
    char *Chk1Hint;  // пояснение к кнопке №1
    char *Chk2Hint;  // пояснение к кнопке №2
    char *Chk3Hint;  // пояснение к кнопке №3
    char *Chk4Hint;  // пояснение к кнопке №4
    char ShowCancel; // показавать/непоказавать кнопку Cancel
};

//=================================================
struct HMonstr_
{
    char *Header;    // заголовок диалога
    char *MName;     // название монстра
    char *MInfo;     // хар-ки монстра
    char *Comment;   // коментарий к происходящему
    long MonNum;     // номер монстра по порядку
    long Attack;     // атака монстра
    long Defence;    // защита
    long Shots;      // количество выстрелов (если 0-не писать)
    long DamageLo;   // Мин. урон
    long DamageHi;   // Мах. урон  - объед. с пред в одну строку через тире
    long HitPoints;  // здоровье
    long Param1;     // пока не используем
    long Speed;      // скорость
    long ResNeed[7]; // количество соответствующего ресурса
    long ResOwn[7];  // количество ресурсов у игрока
    long MAvailable; // доступно монстров для найма
    long MHired;     // нанятых монстров (устанавливается)
};

//=================================================

struct _Diplomacy
{
    int GamerAvailable[8];  // по 8-ми игрокам 1-есть, 0-нет такого (в порядке следования цветов)
    long GamerIndex;        // номер игрока
    char GamerEnable[8][2]; // 0-данному О нельзя посылать, 1-можно
    char GamerSent[8][2];   // 0, если данному О ничего не посылалось и 1, если посылалось
    // Второй индекс значит 0-новое послание, 1-ответ на присланное.
    // GamerEnable[8][2] и GamerSent[8][2] связаны так. Если GamerEnable[i][j]
    // ноль, то кнопочка "послать" должна быть зашадена, т.к. в этот день уже
    // посылалось, а если не ноль, то разрешена. Ты же должен установить
    // единички в GamerSent[i][j], если это посылалось. Я потом просто копирую все
    // единички из  GamerSent[i][j] в нолики в GamerEnable[8][2].
    long DipLevel[8][8]; // матрица 8 на 8, где ячейка [i][j]
    // хронит процент ненависти (0-100) i к j (отношение j к i может быть другим)
    // в этом массиве хранится измененное И отношение ко всем О
    long ShowLevels[7]; // уровень отношений, не выше которого  показывается данный ресурс О (0-100) - работает для всех
    long Resources[8][7];   // ресурсы всех
    long ResSent[7][8][8];  // отосланные ресурсы по всем О (i/j/k, i - количество ресурса, j- от игрока, k - к игроку)
    long ResvMessage[8][8]; // принятые и не прочит. сообщения (инд.,-1-нет)
    long ReplMessage[8][8]; // сообщения в ответ (индекс ответа, -1-нет)
    long SentMessage[8][8]; // новое сообщения (индекс сообщ., -1-нет)
    char ***Messages;       // все сообщения и ответы (см. ниже)
};

//=================================================

struct _CurseShow_
{
    char **Pics;  // 10 указателей на картинки (0-конец)
    char **Text1; // текст снизу при выборе картинки (1-я строка) (0-конец)
    char **Text2; // текст снизу при выборе картинки (2-я строка) (0-конец)
};

//=================================================

struct _Intro_
{
    char *AVIBMPPath; // ресурс
    char Cicling;     // показ зациклен (для bmp неважен)
};
//=================================================

struct _CustomData_
{
    int Type; // 0 - не задано
    int SType;
    int Num;
    char HasCansel;
    char _u[3];
    int Number[4];
    char *Text[4];
    char *Pic[4];
    char *Hint[4];
    char *Button[4];
    char *HintButton[4];
};

//=================================================

struct _CheckBoxes
{
    char *Caption; // текст Заголовка диалога
    int Number;    // кол-во галочек
    char **Text;   // указатель на строки (0-больше нет)
    int *State;    // состояние галочек - количество, как и Text
    // 0 - не поставлена
    // 1 - поставлена
    // 2 - не поставлена, не меняется
    // 3 - поставлена, не меняется
    char OnlyOne; // true - RadioButtons
    // false - CheckBoxes
};

//=================================================

struct _DlgNPC_
{
    int Flags; // флаги - см. ниже
    /*
      Flags & 0x00000001 - ; НПС живой, иначе мертвый
                             Если он мертвый, то кнопка уволить напрещена и надпись "МЕРТВ"
                             написана.
      Flags & 0x00000002 - ; можно редактировать имя НПС
                             остальное позже обсудим
      Flags & 0x00000004 - ; нельзя передавать артифакты от/к герою.
                             остальное позже обсудим
      Flags & 0x00000008 - ; показат кнопку увольнения

      Функция возвращает номер выбранной картинки (0...6), если Request=1
    и что угодно (например,0), если Request=0
    */
    int DlgLeft; // Кородинаты окна по горизонтали
    int DlgTop;  // Координаты окна по вертекали
    char *Name;  // Имя NPC. Это буфер на 32 символа. Последний 0, так
    // что при редактировании имя может быть не длиннее 31.
    // указвтель НЕ МЕНЯТЬ - изменение прямо копировать по
    // указателю в мой буфер
    char *Portrait;     // Портрет NPC
    char *PortraitHint; // текст хинта для портрета
    char *HeroName;     // имя героя-хозяина
    int Level;          // уровень НПС
    int pAT, hAT;       // атака и атака с учетом атаки героя
    // (выводится следом в скобках)
    int pDF, hDF;                // защита и с учетом героя
    int pHP;                     // здоровье
    int pSP;                     // скорость
    int pDML, pDMH;              // урон нижний и верхний через черточку
    int pMP;                     // магическая сила
    int pMR;                     // магическая защита с символом % на конце
    int pShots;                  // количество выстрелов. если 0, то это пустая строка
    char *Description;           // описание
    char *SpecIcon1;             // иконка заклинания
    char *SpecText1;             // текст рядом (в две или три коротких строки)
    char *SpecHint1;             // хинт для первой иконки и для текста рядом
    char *SpecPopUpText1;        // ТЕКСТ ДЛЯ ПОПАП ОКНА
    char *SpecIcon2;             // иконка другого бонуса
    char *SpecText2;             // текст рядом (в две или три коротких строки)
    char *SpecHint2;             // хинт для второй иконки и для текста рядом
    char *SpecPopUpText2;        // ТЕКСТ ДЛЯ ПОПАП ОКНА
    int CurExp;                  // текущий опыт
    int NextExp;                 // след. опыт
    char *Type;                  // тип NPC
    char *TypePopUpText;         // ТЕКСТ ДЛЯ ПОПАП ОКНА
    char *TypeHint;              // ТЕКСТ ДЛЯ ХИНТА
    char *Bonus[6];              // 6 путей к первым 6 картинкам 70 на 70
    char *BonusHints[6];         // 6 хинтов для первых 6 картинок
    char *BonusPopUpText[6];     // 6 текстов для первых 6 картинок
    char *SpecBonus[6];          // 6 путей ко вторым 6 картинкам как сейчас
    char *SpecBonusHints[6];     // 6 хинтов для вторых 6 картинок
    char *SpecBonusPopUpText[6]; // 6 текстов для вторых 6 картинок
    // char *Information;      // Информация
    int Request;   // если 1, то игрок должен выбрать картинку внизу
    char *Next[6]; // 6 путей к нижним 6 картинкам 70 на 70 из которых
    // игрок должен выбрать одну и нажать Ок
    char *NextActive[6];
    char *NextHints[6];      // 6 хинтов к нижним 6 картинкам
    char *NextPopUpTexts[6]; // 6 текстов к нижним 6 картинкам
    char *ArtIcons[6];       // иконка артифакта ("NONE" - пустой)
    char *ArtHints[6];       // хинт для иконки артифакта
    char *ArtPopUpTexts[6];  // текст для попап окна
    char ArtOutput[6];       // артифакт отдается - 1, оставляется - 0
};

//=================================================

struct _PopUpRadioButtons_
{
    char *Caption; // текст Заголовка диалога
    int Number;    // кол-во позиций для выбора
    char **Text;   // указатель на строки (0-больше нет)
    int SelIndex;  // номер выбранной позиции
    int DlgLeft;   // Кородинаты окна по горизонтали
    int DlgTop;    // Координаты окна по вертекали
};

//=================================================

struct _DlgSetup_ItemList_
{
    char *Name;       // название группы
    char *Hint;       // текст хинта внизу (при наведении на имя группы)
    char *PopUp;      // текст в окне по правой мыши (при клике на имя группы)
    int *ItemState;   // ук. на массив состояния галочек
    char **ItemName;  // текст справа от галочки
    char **ItemHint;  // текст хинта внизу (одна строка)
    char **ItemPopUp; // текст в окне по правой мыши.
    int Type;         // 0-нет группы, 1-чекбоксы, 2-радиобутоны
    int ItemCount;    // число элементов
};

struct _DlgSetup_Page_
{
    char *Name;  // заголовок страницы
    char *Hint;  // текст хинта внизу (одна строка)
    char *PopUp; // текст в окне по правой мыши
    /* _DlgSetup_ItemList_ *ItemListLT; // левая верхняя [0]
    _DlgSetup_ItemList_ *ItemListLB; // левая нижняя [1]
    _DlgSetup_ItemList_ *ItemListRT; // правая верхняя [2]
    _DlgSetup_ItemList_ *ItemListRB; // правая нижняя [3] */
    _DlgSetup_ItemList_ *ItemList[4];
    char Enabled; // страница вкл/выкл
};

struct _DlgSetup_
{
    char *Name;                // название окна
    char *Hint;                // текст хинта внизу (при наведении на имя окна)
    char *PopUp;               // текст в окне по правой мыши (при клике на имя окна)
    char *Intro;               // текст вступления
    _DlgSetup_Page_ *Pages[8]; // страницы
    int (*Listener)(int Type, int Page, int ItemList, int Item);
    // return CALL_4(int, __cdecl, 0x7775EC, Type, Page, ItemList, Item);
    // ук. на функцию, которую надо вызывать при каждом изменении
    // Type - 0 - кнопки, 1 - элементы списков, 2 - страничка
    // Page - # страницы
    // ItemList - # списка (0 - LT, 1 - LB, 2 - RT, 3 - RB)
    // Item - # элемента списка (для кнопок: 0 - OK, 1 - Save,
    //                           2 - Restore, 3 - Multiplayer)
    // Реагирование на возвращаемые значения:
    // 0 - ничего, 1 - перерисовка, 2 - закрытие диалога

    int *ButtonsStates; // указатель на массив состояний кнопок
    // -1 - нет кнопки, 0 - кнопка неактивна, 1 - активна
    // #0 - кнопка Ok,  (-1 не действует)
    // #1 - Save,
    // #2 - Restore,
    // #3 - Multiplayer,
    // #4 - UnSelectAll,
    // #5 - SelectAll,
    // #6 - Next,
    // #7 - Back,
    // #8 - Load,
    // #9 - Restore Multiplayer
    char *NextButtonHint; // хинт для кнопки Next
    char *BackButtonHint; // хинт для кнопки Back
    //  char ShowIntro;            // показывать/нет интро};
    unsigned int Flags;
    // 0x00000001 - показать/нет интро
    // 0x00000002 - разрешено изменение/нет

    // int GetListener(int Type, int Page, int ItemList, int Item) {return CALL_4(int, __cdecl, 0x7775EC, Type, Page,
    // ItemList, Item); }
};

//=================================================
struct _ChooseFile_
{
    char *Caption;     // Заголовок диалога
    char *Description; // Пояснение к Диалогу
    char *Directory;   // каталог для выбора
    char *Mask;        // маска файлов (можно использовать комбинации типа "*.cpp|*.h")
    char *Buf;         // буфер куда копировать полный путь к выбранному файлу
    int Buflen;        // размер буфера вывода
    int Attributes;    // атрибуты файлов (можно использовать комбинации типа "$00000001 | $00000002")
    /*
                            $00000001	Read-only files
                            $00000002	Hidden files
                            $00000004	System files
                            $00000008	Volume ID files
                            $00000010	Directory files
                            $00000020	Archive files
                            $0000003F	Any file
    */
    int ShowCancel;      // показывать кнопку Cancel
    int FileEditEnabled; // можно ли руками вводить имя файла
    int Flags;           // резерв
};
//================================================= // Dialogue12
struct _IconListItem
{
    char *Description; // описание
    char *Path;        // путь до картинки
    int Checked;       // состояние: выбрано/невыбрано
};
//=================================================
struct _IconList
{
    char *Caption;        // заголовок диалога
    _IconListItem *Items; // список иконок
    int ItemCount;        // количество элементов в списке
    int OnlyOne;          // можно выбирать только 1 элемент или нет
    int ShowCancel;       // показывать кнопку Cancel
    int Flag1;            // ррезерв
};
//=================================================
} // namespace WoG
