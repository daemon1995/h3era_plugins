// данный файл вносит исправление с работой карт повышенного размера
// в оригинальном WoG максимальный размер карт упирался в 144/144
// часть ф-ций переписаны полностью, часть -- исправлена патчами

namespace MapSize
{
typedef _Position_ H3Position;
#define MAPSIZE_G 252
#define MAXMAPSIZE MAPSIZE_G
#define RETURN(val) return val;
struct ShortPoint
{

    static ShortPoint WOG_Line[MAXMAPSIZE];

  public:
    short x;
    short y;
    short z;

  public:
    bool operator==(const ShortPoint &other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }

  public:
    inline void MakeLine(ShortPoint&p)
    {
        return CALL_2(void, __thiscall, 0x779470, this, &p);
    }

    static inline int FindWayAI(ShortPoint *point, ShortPoint *toPoint, int index, int shortway)
    {
        return CALL_4(int, __cdecl, 0x706DDB, point, toPoint, shortway, index);
    }
    static inline H3Position MakeAIWay(ShortPoint *point, ShortPoint *toPoint, int steps, int index)
    {
        return CALL_4(H3Position, __cdecl, 0x70727E, point, toPoint, steps, index);
    }
};
typedef ShortPoint Point;

ShortPoint ShortPoint::WOG_Line[MAXMAPSIZE];
short WayAI[3][MAXMAPSIZE + 2][MAXMAPSIZE + 2];
short StpAI[MAXMAPSIZE + 2][MAXMAPSIZE + 2];
int __cdecl WoG__WayAI__FindWayAI(ShortPoint *e, ShortPoint *s, int ind, int shortway)
{
    short i, j, k, l, d, v, change, sz;

    memset(WayAI, -1, sizeof(WayAI));

    sz = o_GameMgr->Map.size;
    for (i = 0; i < sz; i++)
    {
        for (j = 0; j < sz; j++)
        {
            if (CALL_3(int, __cdecl, 0x706D80, i, j, s->z)) // MapAI
                WayAI[ind][i + 1][j + 1] = -1;
            else
                WayAI[ind][i + 1][j + 1] = 0;
        }
    }
    for (i = 0; i < MAXMAPSIZE + 2; i++)
    {
        WayAI[ind][0][i] = -1;
        WayAI[ind][MAXMAPSIZE + 1][i] = -1;
        WayAI[ind][i][0] = -1;
        WayAI[ind][i][MAXMAPSIZE + 1] = -1;
    }

    WayAI[ind][s->x + 1][s->y + 1] = 0; // начальная точка
    WayAI[ind][e->x + 1][e->y + 1] = 1; // конечная точка
    if (*s == *e)
        RETURN(1)
    do
    {
        change = 0;
        for (i = 1; i < MAXMAPSIZE + 1; i++)
        {
            for (j = 1; j < MAXMAPSIZE + 1; j++)
            {
                if (WayAI[ind][i][j] > 0)
                { // есть что делать
                    for (k = -1; k < 2; k++)
                    {
                        for (l = -1; l < 2; l++)
                        {
                            if ((k == 0) && (l == 0))
                                continue;
                            if (WayAI[ind][i + k][j + l] == -1)
                                continue;
                            if ((k != 0) && (l != 0))
                                d = 3; // диагональ
                            else
                                d = 2; // гор или вер
                            v = (short)(WayAI[ind][i][j] + d);
                            if ((v < WayAI[ind][i + k][j + l]) || (WayAI[ind][i + k][j + l] == 0))
                            {
                                WayAI[ind][i + k][j + l] = v;
                                change = 1;
                            }
                        }
                    }
                }
            }
        }
        if (shortway && WayAI[ind][s->x + 1][s->y + 1] != 0)
            break;
    } while (change != 0); // пока что-то менялось
    //  }while(Way[s.x][s.y]==0); // пока в начальную точку не запишем что-то
    RETURN(0)
}
DWORD __cdecl WoG__WayAI__MakeAIWay(ShortPoint *point, ShortPoint *toPoint, int steps, int index)
{
    memset(StpAI, 0, sizeof(StpAI));

    int v10 = 1;
    int v12 = point->x + 1;
    int v6 = point->y + 1;
    int v7 = v12;
    int v13 = v6;

    do
    {
        int v14 = WayAI[index][v12][v6];
        for (int k = -1; k < 2; ++k)
        {
            for (int l = -1; l < 2; l++)
            {
                int value = WayAI[index][v12 + k][v6 + l];
                if ((k || l) && value > 0 && value < v14)
                {
                    v14 = value;
                    v7 = k + v12;
                    v13 = l + v6;
                }
            }
        }
        StpAI[v7][v13] = v10++;
        v12 = v7;
        v6 = v13;

    } while (v10 != steps && (toPoint->x + 1 != v7 || toPoint->y + 1 != v13));
    return CALL_3(DWORD, __cdecl, 0x0711E7F, v7 - 1, v13 - 1, point->z);
}

int __cdecl WoG__WayAI__FindStep(int x, int y, int l, int x1, int y1, int l1, H3Position *MixPos, int Steps)
{
    Point s{x, y, l};
    Point e{x1, y1, l1};
    if (l == l1 && s == e)
    {
        return 2;
    }

    Point t = e;

    if (l == l1 && WayAI[0][x + 1][y + 1] > 0)
    {
        *MixPos = Point::MakeAIWay(&s, &t, Steps, 0);
        return 1;
    }
    else
    {
        s.MakeLine(e);
        const int lineHasPoint = IntAt(0x2933558);
        for (int i = 0; i < lineHasPoint; i++)
        {
            if (Point::FindWayAI(&s, &Point::WOG_Line[i], 2, 1) == 1)
                return 2;
            if (WayAI[2][s.x + 1][s.y + 1] > 0)
            {
                t = Point::WOG_Line[i];
                *MixPos = Point::MakeAIWay(&s, &t, Steps, 2);
                return 1;
            }
        }

        return 0;
    }
}

int __cdecl WoG__WayAI__GetDist(ShortPoint a1, int arrayIndex)
{
    int j;            // [esp+Ch] [ebp-30h]
    DWORD dist[3][3]; // [esp+10h] [ebp-2Ch] BYREF
    int i;            // [esp+34h] [ebp-8h]
    int v6;           // [esp+38h] [ebp-4h]

    v6 = -1;
    if (!WayAI[arrayIndex][a1.x + 1][a1.y + 1])
        return -1;
    if (WayAI[arrayIndex][a1.x + 1][a1.y + 1] >= 0)
        return WayAI[arrayIndex][a1.x + 1][a1.y + 1];
    for (i = 0; i < 3; ++i)
    {
        for (j = 0; j < 3; ++j)
            dist[i][j] = WayAI[arrayIndex][a1.x][146 * i + j + a1.y];
    }
    for (i = 0; i < 9; ++i)
    {
        if (i != 4 && (int)dist[i / 3][i % 3] > 0)
        {
            if (v6 == -1)
                v6 = dist[i / 3][i % 3];
            if (v6 > dist[i / 3][i % 3])
                v6 = dist[i / 3][i % 3];
        }
    }
    if (v6 == -1)
        return -1;
    else
        return v6 + 1;
}
void MapSizeFixes(PatcherInstance *_PI)
{

    // WoG::WayAI::MakeAIWay
    _PI->WriteHiHook(0x706DDB, SPLICE_, EXTENDED_, CDECL_, WoG__WayAI__FindWayAI);
    _PI->WriteHiHook(0x70727E, SPLICE_, EXTENDED_, CDECL_, WoG__WayAI__MakeAIWay);
    _PI->WriteHiHook(0x7074FA, SPLICE_, EXTENDED_, CDECL_, WoG__WayAI__FindStep);
    _PI->WriteHiHook(0x7083C0, SPLICE_, EXTENDED_, CDECL_, WoG__WayAI__GetDist);

    ////патчим вог (размер карты в циклах)
    _PI->WriteDword(0x706E0F + 2, MAPSIZE_G);
    _PI->WriteDword(0x706E2F + 1, MAPSIZE_G);

    _PI->WriteDword(0x779E76 + 2, MAPSIZE_G);
    _PI->WriteDword(0x779E96 + 1, MAPSIZE_G);

    _PI->WriteDword(0x752B11 + 3, MAPSIZE_G);
    _PI->WriteDword(0x752B30 + 3, MAPSIZE_G);

    _dword_ p_WOG_Line = (_dword_)&ShortPoint::WOG_Line[0];
    // указатель на старый массив (дл¤ дин.поиска)
    _dword_ p_WOG_LineOrig = 0x2933560;

    // патчим воговский Point::Line[x,y,z]
    // Point::MakeLine(Point &p)
    _PI->WriteDword(0x77958D + 2, p_WOG_Line);
    _PI->WriteDword(0x779643 + 2, p_WOG_Line);
    _PI->WriteDword(0x779703 + 2, p_WOG_Line);
    _PI->WriteDword(0x7797B9 + 2, p_WOG_Line);
    _PI->WriteDword(0x779884 + 2, p_WOG_Line);
    _PI->WriteDword(0x77993B + 2, p_WOG_Line);
    _PI->WriteDword(0x7799FC + 2, p_WOG_Line);
    _PI->WriteDword(0x779AB3 + 2, p_WOG_Line);

    // WOG_FindStep_AI()
    _PI->WriteDword(0x7075D6 + 2, p_WOG_Line);
    _PI->WriteDword(0x707625 + 2, p_WOG_Line);

    // WOG_FindStep()
    _PI->WriteDword(0x77A72E + 2, p_WOG_Line);
    _PI->WriteDword(0x77A77F + 1, p_WOG_Line);
    _PI->WriteDword(0x77A797 + 3, p_WOG_Line);
    _PI->WriteDword(0x77A7AD + 3, p_WOG_Line + 2);

    // WOG_FindStep2()
    _PI->WriteDword(0x77A917 + 2, p_WOG_Line);
    _PI->WriteDword(0x77A960 + 2, p_WOG_Line);
    _PI->WriteDword(0x78453F + 1, p_WOG_Line);
    _PI->WriteDword(0x784538 + 1, MAPSIZE_G);
}
} // namespace MapSize
