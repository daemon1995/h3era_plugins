#include "pch.h"

RECT AdventureMapHints::m_mapView;

AdventureMapHints *AdventureMapHints::instance = nullptr;
void AdventureMapHints::Init(PatcherInstance *pi)
{
    if (!instance)
    {
        instance = new AdventureMapHints(pi);
        if (instance)
        {
            instance->_pi = pi;
            instance->CreatePatches();
            instance->m_isInited = true;
        }
    }
}
void debug(int a)
{
    H3Messagebox(Era::IntToStr(a).c_str());
}

UINT time = 0;

#define Z1 (char *)(0x9271E8 + 512)
void echo(int a, int b, int c)
{
    libc::sprintf(Z1, "x = %d, y = %d, z = %d", a, b, c);
    Era::ExecErmCmd("IF:L^%z1^;");
}
void echo(int a)
{
    libc::sprintf(Z1, "%d", a);
    Era::ExecErmCmd("IF:L^%z1^;");
}
void echo(const char *a)
{
    libc::sprintf(Z1, "%s", a);
    Era::ExecErmCmd("IF:L^%z1^;");
}
void echo(H3String &a)
{
    echo(a.String());
}
void echo(std::string &a)
{
    echo(a.c_str());
}

AdventureMapHints::AdventureMapHints(PatcherInstance *pi) : IGamePatch(pi)
{
    m_mapView.left = 8;                                   // right panel
    m_mapView.top = 8;                                    // bottom panel
    m_mapView.right = H3GameWidth::Get() - (800 - 592);   // right panel
    m_mapView.bottom = H3GameHeight::Get() - (600 - 544); // bottom panel
    settings = new AdventureHintsSettings("Runtime/gem_AdventureMapHints.ini", "StaticHintsDrawByType");
}

DWORD lastTime = 0;
RECT rect{200, 100, 200, 24};
RECT _rect{rect};

_LHF_(Foo)
{

    DWORD timeDifference = h3::GetTime() - lastTime;
    if (timeDifference >= 10000)
    {
        Era::ExecErmCmd("IF:L^^");
    }
    if (timeDifference >= 1000)
    {
        lastTime += timeDifference;
        // rect.right++;
        P_WindowManager->H3Redraw(rect.left, rect.top, rect.right, rect.bottom);
    }
    H3FontLoader f(h3::NH3Dlg::Text::SMALL);
    if (auto *pcx = P_WindowManager->GetDrawBuffer())
    {
        auto *t = H3TurnTimer::Get();
        t->lastShownTime;
        //	LPCSTR str = H3String::FormatNumber(timeDifference).String();
        static int counter = 0;
        if (counter ^= 1)
        {

            //	pcx->AdjustHueSaturation(rect.left, rect.top, rect.right, rect.bottom, 0.4f, 0.8f);
            AdventureMapHints::instance->glBackPtr->DrawToPcx16(0, 0, rect.right, rect.bottom, pcx, rect.left, rect.top,
                                                                1);
            f->TextDraw(pcx, H3String::FormatNumber(h3::GetTime()).String(), rect.left, rect.top, rect.right,
                        rect.bottom);
            //	if (timeDifference >= 1000)
        }
    }

    return EXEC_DEFAULT;
}

void AdventureMapHints::CreatePatches() noexcept
{
    m_pickupResources.clear();

    m_pickupResources.reserve(12);

    if (!m_isInited)
    {
        instance->glBackPtr = H3LoadedPcx::Load("DiBoxBck.pcx");

        blockAdventureHintDraw = _pi->CreateHexPatch(0x40D0D8, const_cast<char *>("EB 5F 90"));
        _pi->WriteLoHook(0x4A48AB, AdventureMapHints::Hero_AddPickupResource);
        //
        //_pi->WriteHiHook(0x4AA766, THISCALL_, Enter2Object);
        _pi->WriteHiHook(0x40F5D7, THISCALL_, AdvMgr_ObjectDraw);

        _pi->WriteLoHook(0x40F5F4, AdvMgr_AfterObjectDraw);

#ifdef _DEBUG

#endif // _DEBUG

        //	_pi->WriteLoHook(0x4EDB3E, Foo);
        //_pi->WriteLoHook(0x601E36, Foo);
        //_pi->WriteLoHook(0x6031FB, Foo);
        //_pi->WriteHiHook(0x40F350, THISCALL_, AdvMgr_DrawMap);

        m_isInited = true;
    }

    //	_pi->WriteLoHook(0x40D0DB, AdvMgr_OnSetHint);

    //_pi->WriteHiHook(0x40F5D7, THISCALL_, AdvMgr_BeforeDrawPath);
    //_pi->WriteLoHook(0x4122B0, AdvMgr_DrawFogOfWar);
}

_LHF_(AdventureMapHints::AdvMgr_AfterObjectDraw)
{
    if (instance->m_drawnOjects.size())
    {
        instance->m_drawnOjects.clear();
    }

    if (
#ifdef _DEBUG
        true
#else
        false
#endif // _DEBUG

        //	&& screenY == CharAt(0x40F54A + 2)
    )
    {
        // Era::ExecErmCmd("IF:L^^;");

        auto adv = P_AdventureManager->Get();
        UINT mapX = adv->screenPosition.GetX();
        UINT mapY = adv->screenPosition.GetY();
        UINT mapZ = adv->screenPosition.GetZ();
        UINT mapSize = *P_MapSize;
        if (mapX >= mapSize)
            mapX -= 1024; // draw outside map tiles
        if (mapY >= mapSize)
            mapY -= 1024;
        // int x = adv->screenPosition.GetX();
        // int y = adv->screenPosition.GetY();

        int minX = adv->dlg->GetX() + 8;
        int minY = adv->dlg->GetY() + 8;
        int maxX = minX + adv->dlg->GetWidth();
        int maxY = minY + adv->dlg->GetHeight();
        //	if (x> 1)
        {
        }

        // backPcx->DrawToPcx16(0, 0, TEMP_PCX_WIDTH, TEMP_PCX_HEIGHT, tempBuffer, 0, 0, 1);
        //  create golden frame
        // memset(tempBuffer->buffer, 0, tempBuffer->buffSize);
        static int _y = 45;
        auto &vec = instance->m_pickupResources;
        const UINT SIZE = vec.size();
        INT8 maxMapHeight = CharAt(0x40F54A + 2);
        INT8 maxMapWidth = CharAt(0x40F53E + 2);

        constexpr UINT DRAW_STEP = 100;
        // vector -> set
        UINT t = GetTime();
        auto def = H3LoadedDef::Load("smalres.def");

        auto drawBuffer = P_WindowManager->GetDrawBuffer();
        int objX, objY, objZ;
        for (size_t i = 0; i < SIZE; i++)
        {
            auto &obj = vec[i];
            obj->m_pos.GetXYZ(objX, objY, objZ);
            if (obj->m_drawCounter * DRAW_STEP < obj->m_drawTime)
            {
                // echo(objX, objY, objZ);
                // echo(mapX, mapY, mapZ);

                if (objX >= mapX && objX < mapX + maxMapWidth && objY >= mapY && objY < mapY + maxMapHeight &&
                    objZ == mapZ)
                {

                    // auto fnt = *reinterpret_cast<H3Font**>(0x698A58); // small font

                    auto fnt = H3Font::Load("smalfont.fnt");

                    // auto tempBuffer = H3LoadedPcx16::Create(150, 150);
                    int x = (objX - mapX) * 32 + adv->screenDrawOffset.x;
                    int y = (objY - mapY) * 32 + adv->screenDrawOffset.y - obj->m_drawCounter * 2;
                    if (t > obj->m_lastDrawnType + DRAW_STEP)
                    {
                        obj->m_lastDrawnType = t;
                        //	obj->m_drawTime -= 200;
                        // obj->m_drawCounter++;
                        obj->m_drawCounter++;
                        // if (--_y && _y-- < 15) _y = 150;
                    }
                    x = Clamp(m_mapView.left, x, m_mapView.right);
                    y = Clamp(m_mapView.top, y, m_mapView.bottom);
                    int resAmount = obj->m_subType == eResource::GOLD ? obj->m_value * 100 : obj->m_value;
                    sprintf(h3_TextBuffer, "+%d", resAmount);
                    fnt->TextDraw(drawBuffer, h3_TextBuffer, x - 25, y, 25, 20, eTextColor::WHITE,
                                  eTextAlignment::MIDDLE_RIGHT);
                    def->DrawToPcx16(0, obj->m_subType, drawBuffer, x, y);
                    fnt->Dereference();
                }
            }
        }

        def->Dereference();

        // echo(1);
        // adv->dlg->Redraw();
        // CDECL_0(int, 0x4EDB20);

        // auto fnt = *reinterpret_cast<H3Font**>(0x698A58);
        // fnt->TextDraw(P_WindowManager->GetDrawBuffer(), "{~>resource.def:0:0}", x, _y, 322, 32);
        // fnt->Dereference();
        //	P_WindowManager->H3Redraw(0, 0, 800, 600);

        // CDECL_0(int, 0x4EDB20);
    }

    return EXEC_DEFAULT;
}

_LHF_(AdventureMapHints::AdvMgr_DrawFogOfWar)
{

    if ((c->AL() & ByteAt(0x69CD08)) == 0)
    {

        int mapX, mapY, mapZ;
        mapX = IntAt(c->ebp + 0x8);
        mapY = IntAt(c->ebp + 0xC);
        mapZ = IntAt(c->ebp + 0x10);

        H3Position pos(mapX, mapY, mapZ);
        if (H3TileVision::CanViewTile(pos))
        {
            const H3MapItem *currentItem = nullptr;
            // UINT32 pos = mapX *
            H3Position pos(mapX, mapY, mapZ);
            if (mapX >= 0 && mapY >= 0 && mapX < *P_MapSize && mapY < *P_MapSize
                //&& H3TileVision::CanViewTile(pos)
            )
                currentItem = P_Game->GetMapItem(pos.Mixed());
            if (instance->IsNeedDraw(currentItem)

            )
            {
                int yDraw, xDraw;
                yDraw = IntAt(c->ebp + 0x14);
                xDraw = IntAt(c->ebp + 0x18);
                // echo(mapX, mapY, mapZ);
                // echo(xDraw, yDraw, mapZ);

                auto adv = P_AdventureManager->Get();
                UINT screenX = adv->screenPosition.GetX();
                UINT screenY = adv->screenPosition.GetY();
                UINT screenZ = adv->screenPosition.GetZ();
                int mapSize = *P_MapSize;
                // if (screenX >= mapSize)			screenX -= 1024; // draw outside map tiles
                // if (screenY >= mapSize)			screenY -= 1024;
                LPCSTR str = h3::H3ObjectName::Get()[currentItem->objectType];
                auto fnt = *reinterpret_cast<H3Font **>(0x698A58);
                fnt->TextDraw(P_WindowManager->GetDrawBuffer(), str, mapX * 32, mapY * 32, 45, 32);
            }
        }
    }

    return EXEC_DEFAULT;
}

void __stdcall AdventureMapHints::AdvMgr_BeforeDrawPath(HiHook *h, H3AdventureManager *adv, int mapX, int mapY,
                                                        int mapZ, int screenX, int screenY)
{

    THISCALL_6(void, h->GetDefaultFunc(), adv, mapX, mapY, mapZ, screenX, screenY);
}

void __stdcall AdventureMapHints::Enter2Object(HiHook *h, H3AdventureManager *advMan, H3Hero *hero, H3MapItem *mapItem,
                                               __int64 pos)
{

    switch (mapItem->objectType)
    {
    case eObject::RESOURCE:
        if (hero && hero->owner == P_Game->GetPlayerID())
        {

            auto &vec = instance->m_pickupResources;

            if (mapItem)
            {
                auto res = new rs::ResourceFloatingHint(pos, mapItem);
                vec.emplace_back(res);

                int size = mapItem->objectDrawing.Size();
                for (size_t i = 0; i < size; i++)
                {
                    // P_AdventureManager->map->DrawItem(mapItem, mapItem->objectDrawing.At(0));
                }
            }
        }

        break;
    default:
        break;
    }

    THISCALL_4(void, h->GetDefaultFunc(), advMan, hero, mapItem, pos);
}

LPCSTR AdventureMapHints::GetHintText(const H3AdventureManager *adv, const H3MapItem *mapItem, const int mapX,
                                      const int mapY, const int mapZ) noexcept
{

    constexpr UINT NOT_VISITED_TEXT_ID = 354;
    constexpr UINT VISITED_TEXT_ID = 353;

    auto visitedText = H3GeneralText::Get()->GetText(VISITED_TEXT_ID + 1);
    auto notVisitedText = H3GeneralText::Get()->GetText(NOT_VISITED_TEXT_ID + 1);

    H3String changedVisitedText = H3String::Format("{~Green}\n%s}", visitedText).String();
    H3String changedNotVisitedText = H3String::Format("{~Orange}\n%s}", notVisitedText).String();

    H3Vector<LPCSTR> *generalTextPtr = reinterpret_cast<H3Vector<LPCSTR> *>(ADDRESS(H3GeneralText::Get()) + 0x1C);

    Patch *armySizeNameBracketsPatch = nullptr;
    if (mapItem->objectType == eObject::MONSTER)
    {
        LPCSTR brackets = "{%s} %s";
        armySizeNameBracketsPatch = instance->_pi->WriteDword(0x40C2E7 + 1, brackets);
    }

    auto p_visited = generalTextPtr->At(VISITED_TEXT_ID);
    auto p_notVisited = generalTextPtr->At(NOT_VISITED_TEXT_ID);

    *p_visited = changedVisitedText.String();
    *p_notVisited = changedNotVisitedText.String();

    instance->blockAdventureHintDraw->Apply();
    THISCALL_4(void, 0x40B0B0, adv, mapItem, mapX, mapY);
    instance->blockAdventureHintDraw->Undo();

    *p_visited = visitedText;
    *p_notVisited = notVisitedText;
    if (armySizeNameBracketsPatch)
    {
        armySizeNameBracketsPatch->Destroy();
        // sprintf(h3_TextBuffer, "{~>cprsmall.def:0:%d}\n%d", mapItem->objectSubtype +2,
        // mapItem->wanderingCreature.count); h3_TextBuffer = "";
    }

    return h3_TextBuffer;
}
void __stdcall AdventureMapHints::AdvMgr_ObjectDraw(HiHook *h, H3AdventureManager *adv, int mapX, int mapY, int mapZ,
                                                    int screenX, int screenY)
{

    THISCALL_6(void, h->GetDefaultFunc(), adv, mapX, mapY, mapZ, screenX, screenY);

    if (STDCALL_1(SHORT, PtrAt(0x63A294), instance->settings->vKey) & 0x800 && instance->settings->isHeld
        //&&0
    )
    {

        H3MapItem *currentItem = nullptr;
        // UINT32 pos = mapX *

        H3Position pos(mapX, mapY, mapZ);
        if (mapX >= 0 && mapY >= 0 && mapX < *P_MapSize && mapY < *P_MapSize && H3TileVision::CanViewTile(pos))

            currentItem = P_Game->GetMapItem(pos.Mixed());

        if (currentItem &&
            instance->m_drawnOjects.find(currentItem->drawnObjectIndex) == instance->m_drawnOjects.cend() &&
            instance->IsNeedDraw(currentItem)

            //	&& currentItem->access  &6// ==0
            //&&
            //&& currentItem->GetEntrance() == currentItem

        )
        {

            // echo(currentItem->GetCoordinates().x, currentItem->GetCoordinates().y, currentItem->GetCoordinates().z);

            // blo

            instance->m_drawnOjects.insert(currentItem->drawnObjectIndex);
            // LPCSTR hintText = H3ArtifactSetup::Get()[currentItem->objectSubtype].name;//
            // h3::H3ObjectName::Get()[currentItem->objectType];

            // adv->UpdateHintMessage();
            // instance->onHint = true;
            // auto txt = H3GeneralText::Get();
            //	txt[353] ;

            // instance->_pi->WriteWord(h3::AddressOf(txt), (int)txt);

            //	instance->onHint = false;
            // sprintf(const_cast<LPSTR>(p_visited), "%s", nativeText.String());

            LPCSTR hintText =
                GetHintText(adv, currentItem, mapX, mapY, mapZ); // h3::H3ObjectName::Get()[currentItem->objectType];

            // LPCSTR hintText = h3_TextBuffer;//instance->GetHintText(currentItem);//
            // h3::H3ObjectName::Get()[currentItem->objectType]; echo(hintText); echo(name); LPCSTR hintText =
            // "{~>smalres.def:0:0}";
            if (hintText)
            {
                // currentItem->drawnObjectIndex;
            }
            //	currentItem;
            // currentItem->objectDrawing;
            auto &attributes = P_Game->mainSetup.objectAttributes[currentItem->drawnObjectIndex];
            auto &passability = attributes.passability;

            // passability;
            // echo(passability.m_bits[0][0]);
            // echo(P_Game->mainSetup.objectDetails.Size());

            constexpr int TILE_WIDTH = 32;

            // auto fnt = *reinterpret_cast<H3Font**>(0x698A54); // small font
            auto fnt = *reinterpret_cast<H3Font **>(0x65F2EC); // medium font

            // fnt = H3Font::Load("medfont.fnt");
            // fnt = H3Font::Load("smalfont.fnt");
            fnt = H3Font::Load(NH3Dlg::Text::TINY);
            const int HINT_MAX_LINE_WIDTH = fnt->GetMaxLineWidth(hintText);
            //	const int OBJECT_WIDTH = GetObjectWidth(currentItem);

            const int OBJECT_WIDTH = 1 * TILE_WIDTH;

            // FASTCALL_3(UINT8, 0x55D2E0, &attributes, &attributes.width, 1);

            const int TEXT_MAX_ALLOWED_WIDTH = OBJECT_WIDTH + TILE_WIDTH;

            int textWidth = OBJECT_WIDTH;
            int textHeight = fnt->height + 2;
            int outOfWidthBorder = 0;
            // const int HINT_MAX_
            //  check if hint text reserve < x2 object size
            //	if (TEXT_MAX_ALLOWED_WIDTH > OBJECT_WIDTH)
            {

                const int HINT_MAX_WORD_WIDTH = fnt->GetMaxWordWidth(hintText);
                if (HINT_MAX_WORD_WIDTH < HINT_MAX_LINE_WIDTH)
                {
                    // outOfWidthBorder = HINT_MAX_LINE_WIDTH - HINT_MAX_WORD_WIDTH;
                    textWidth = HINT_MAX_WORD_WIDTH;
                    textHeight *= fnt->GetLinesCountInText(hintText, textWidth);
                }
                // textFieldWidth =
            }

            constexpr INT START_DRAW_X = 8;

            auto backPcx = instance->glBackPtr;
            // create hint background and draw text w/o any restrictions
            //	auto backPcx = H3LoadedPcx::Load("GSelPop1.pcx"); // load std dlgBox bg pcx
            //	auto backPcx = H3LoadedPcx::Load("DiBoxBck.PCX"); // load std dlgBox bg pcx

            const int TEMP_PCX_WIDTH = textWidth + 10;
            const int TEMP_PCX_HEIGHT = textHeight + 2;

            auto tempBuffer = H3LoadedPcx16::Create(TEMP_PCX_WIDTH, TEMP_PCX_HEIGHT);
            //	backPcx->DrawToPcx16(0, 0, true, tempBuffer, 30, 20);

            memset(tempBuffer->buffer, 0, tempBuffer->buffSize);
            // backPcx->DrawToPcx16(30, 150, TEMP_PCX_WIDTH, TEMP_PCX_HEIGHT, tempBuffer, 0, 0, 1);
            // H3RGB565 rgb(20, 24, 29);

            // H3RGB565 ()
            // memset(tempBuffer->buffer, rand() %255, tempBuffer->buffSize);
            //  create golden frame
            tempBuffer->DrawThickFrame(0, 0, TEMP_PCX_WIDTH, TEMP_PCX_HEIGHT, 1, 189, 149, 57);
            // draw text to temp buffer
            fnt->TextDraw(tempBuffer, hintText, 0, 0, TEMP_PCX_WIDTH, TEMP_PCX_HEIGHT);

            // resize tempBuffer to align text for screen borders

            int objectWidth = 1 * TILE_WIDTH;
            int destPcxX = screenX * TILE_WIDTH + adv->screenDrawOffset.x - outOfWidthBorder / 2;
            int destPcxY = screenY * TILE_WIDTH + adv->screenDrawOffset.y - TEMP_PCX_HEIGHT;

            // adjust left border draw
            UINT srcX = 0;
            if (destPcxX < m_mapView.left)
            {
                srcX = m_mapView.left - destPcxX;
                destPcxX = m_mapView.left;
                tempBuffer->width -= srcX;
            }
            if (destPcxX + TEMP_PCX_WIDTH - m_mapView.left > m_mapView.right)
                tempBuffer->width = m_mapView.right + m_mapView.left - destPcxX;

            UINT srcY = 0;
            if (destPcxY < m_mapView.top)
            {
                srcY = m_mapView.top - destPcxY;
                destPcxY = m_mapView.top;
                tempBuffer->height -= srcY;
            }
            if (destPcxY + TEMP_PCX_HEIGHT - m_mapView.top > m_mapView.bottom)
                tempBuffer->height = m_mapView.top + m_mapView.bottom - destPcxY;

            // if need to draw any hint
            if (tempBuffer->height > 0 && tempBuffer->width > 0)
            {
                // get general Window draw buffer to draw temp pcx with x/y offsets
                auto drawBuffer = P_WindowManager->GetDrawBuffer();
                tempBuffer->DrawToPcx16(destPcxX, destPcxY, 1, drawBuffer, srcX, srcY);

                constexpr UINT SHADOW_SIZE = 3;
                int heightReserve = m_mapView.bottom - tempBuffer->height - destPcxY + m_mapView.top;
                UINT shadowWidth = 0;

                UINT shadowHeight = 0;

                if (heightReserve > 0)
                    shadowHeight = heightReserve >= SHADOW_SIZE ? SHADOW_SIZE : heightReserve;

                int widthReserve = m_mapView.right - tempBuffer->width - destPcxX + m_mapView.left;
                if (widthReserve > 0)
                    shadowWidth = widthReserve >= SHADOW_SIZE ? SHADOW_SIZE : widthReserve;

                if (shadowWidth)
                    drawBuffer->DrawShadow(destPcxX + tempBuffer->width, destPcxY, shadowWidth,
                                           tempBuffer->height + shadowHeight);

                if (shadowHeight)
                    drawBuffer->DrawShadow(destPcxX, destPcxY + tempBuffer->height,
                                           tempBuffer->width + (shadowHeight ? 0 : shadowWidth), shadowHeight);
            }

            // dereference part
            fnt->Dereference();
            //	backPcx->Dereference();
            tempBuffer->Destroy();
        }
    }
}

_LHF_(AdventureMapHints::Hero_AddPickupResource)
{
    H3Hero *hero = reinterpret_cast<H3Hero *>(c->ecx);

    if (hero && hero->owner == P_Game->GetPlayerID())
    {

        auto &vec = instance->m_pickupResources;
        // H3LoadedDef::DrawToPcx16()
        H3MapItem *mapItem = *reinterpret_cast<H3MapItem **>(c->ebp + 0x8);
        UINT pos = *reinterpret_cast<UINT *>(c->ebp + 0x10);
        if (mapItem)
        {
            // H3ObjectDraw* objDraw = mapItem->objectDrawing.At(0);

            // P_AdventureManager->map->DrawItem(mapItem, objDraw);

            rs::ResourceFloatingHint *res = new rs::ResourceFloatingHint(pos, mapItem);
            vec.emplace_back(res);
        }
    }

    return EXEC_DEFAULT;
}

bool AdventureMapHints::IsNeedDraw(const H3MapItem *mIt) const noexcept
{
    if (mIt)
        return instance->settings->m_objectsToDraw[mIt->objectType];
    return false;
}

rs::ResourceFloatingHint::ResourceFloatingHint(UINT pos, H3MapItem *mapItem)
    : ObjectFloatingHint(pos, mapItem), m_value(mapItem->setup), m_drawTime(3000), m_lastDrawnType(NULL),
      m_drawCounter(0)
{
    // debug(m_value);
}

rs::ResourceFloatingHint *rs::ResourceFloatingHint::Create(UINT pos, int value, int drawTime)
{

    // ResourceFloatingHint* p = new rs::ResourceFloatingHint(pos, mapItem);
    //	INT x, y, z;
    // H3Position::UnpackXYZ(pos, x, y, z);
    // P_AdventureManager->map->GetMapItem(pos);
    rs::ResourceFloatingHint *hint = new rs::ResourceFloatingHint(pos, P_Game->GetMapItem(pos));
    hint->m_value = value;
    hint->m_drawTime = drawTime;
    // return rs::ResourceFloatingHint::Create(x,y,z, value, drawTime);
    return hint;
}

rs::ResourceFloatingHint *rs::ResourceFloatingHint::Create(UINT8 x, UINT8 y, UINT8 z, int value, int drawTime)
{
    //	P_AdventureManager->map->GetMapItem(x, y, z);

    return rs::ResourceFloatingHint::Create(H3Position::Pack(x, y, z), value, drawTime);
}

rs::ResourceFloatingHint::~ResourceFloatingHint()
{
}

rs::ObjectFloatingHint::ObjectFloatingHint(UINT pos, H3MapItem *mapItem)
    : m_pos(pos), m_type(mapItem->objectType), m_subType(mapItem->objectSubtype)
{
    // debug(m_value);
}

rs::ObjectFloatingHint::~ObjectFloatingHint()
{
}
std::set<resized::H3LoadedPcx16Resized *> rs::CustomObjectFloatingHint::assets;

EXTERN_C __declspec(dllexport) bool CreateFloatingHint(int x)
{

    bool result = false;

    return result;
}

EXTERN_C __declspec(dllexport) resized::H3LoadedPcx16Resized *CreateAsset(const char *srcName, UINT targetWidth,
                                                                          UINT targetHeight)
{
    resized::H3LoadedPcx16Resized *result = nullptr;
    if (targetWidth > 0 && targetHeight > 0)
    {
        // H3ResourceItem::find(srcName);

        result = resized::H3LoadedPcx16Resized::Create(targetWidth, targetHeight);

        // H3LoadedPcx16Resized*
        H3LoadedPcx16 *source = H3LoadedPcx16::Load(srcName);
        result->DrawResizedBicubic(source, source->width, source->width, 0, 0, targetWidth, targetHeight);
        H3Dlg dlg(targetWidth, targetHeight);
        auto pcx = dlg.CreatePcx16(0, 0, targetWidth, targetHeight, 0, 0);
        pcx->SetPcx(result);
        rs::CustomObjectFloatingHint::assets.insert(result);
        dlg.CreateOKButton();
        // H3LoadedPcx16
        dlg.Start();
        // source->Dereference();
    }

    return result;
}

AdventureMapHints::~AdventureMapHints()
{
    m_pickupResources.clear();
}

AdventureHintsSettings::AdventureHintsSettings(const char *filePath, const char *sectionName)
    : ISettings{filePath, sectionName}
{
    reset();
    load();
    save();
    isHeld = true;
    vKey = VK_MENU;
}

void AdventureHintsSettings::reset()
{

    memset(m_objectsToDraw, false, sizeof(m_objectsToDraw));
    m_objectsToDraw[eObject::MONSTER] = true;
    m_objectsToDraw[eObject::CREATURE_BANK] = true;
    m_objectsToDraw[eObject::RESOURCE] = true;
    m_objectsToDraw[eObject::ARTIFACT] = true;
    m_objectsToDraw[eObject::DRAGON_UTOPIA] = true;
    m_objectsToDraw[eObject::CRYPT] = true;
    m_objectsToDraw[eObject::DERELICT_SHIP] = true;
    m_objectsToDraw[eObject::SHIPWRECK] = true;
    m_objectsToDraw[eObject::WITCH_HUT] = true;
    m_objectsToDraw[eObject::MARLETTO_TOWER] = true;
    m_objectsToDraw[eObject::GARDEN_OF_REVELATION] = true;
    m_objectsToDraw[eObject::MYSTICAL_GARDEN] = true;
    m_objectsToDraw[eObject::PYRAMID] = true;
    m_objectsToDraw[eObject::SHRINE_OF_MAGIC_GESTURE] = true;
    m_objectsToDraw[eObject::SHRINE_OF_MAGIC_INCANTATION] = true;
    m_objectsToDraw[eObject::SHRINE_OF_MAGIC_THOUGHT] = true;
    m_objectsToDraw[eObject::STABLES] = true;
    m_objectsToDraw[eObject::LIBRARY_OF_ENLIGHTENMENT] = true;
    m_objectsToDraw[eObject::SCHOOL_OF_MAGIC] = true;
    m_objectsToDraw[eObject::SCHOOL_OF_WAR] = true;
    m_objectsToDraw[eObject::MAGIC_SPRING] = true;
    m_objectsToDraw[eObject::STAR_AXIS] = true;
    m_objectsToDraw[eObject::TREASURE_CHEST] = true;
    m_objectsToDraw[eObject::SEA_CHEST] = true;
    m_objectsToDraw[eObject::PANDORAS_BOX] = true;
    m_objectsToDraw[eObject::TREE_OF_KNOWLEDGE] = true;
    m_objectsToDraw[eObject::SPELL_SCROLL] = true;
    m_objectsToDraw[eObject::LEARNING_STONE] = true;
    m_objectsToDraw[eObject::CORPSE] = true;
    m_objectsToDraw[eObject::WATER_WHEEL] = true;
    m_objectsToDraw[eObject::WINDMILL] = true;
    m_objectsToDraw[eObject::ARENA] = true;
    m_objectsToDraw[eObject::CAMPFIRE] = true;

    // m_objectsToDraw[eObject::MONSTER] = true;
}

BOOL AdventureHintsSettings::load()
{
    for (UINT8 i = 0; i < 232; ++i)
    {

        if (Era::ReadStrFromIni(Era::IntToStr(i).c_str(), sectionName, filePath, h3_TextBuffer))
            m_objectsToDraw[i] = atoi(h3_TextBuffer);
    }

    return 0;
}

BOOL AdventureHintsSettings::save()
{
    for (UINT8 i = 0; i < 232; ++i)
    {

        Era::WriteStrToIni(Era::IntToStr(i).c_str(), Era::IntToStr(m_objectsToDraw[i]).c_str(), sectionName, filePath);
    }
    Era::SaveIni(filePath);
    return 0;
}

LPCSTR *AdventureMapHints::AccessableH3GeneralText::GetStringAdddres(const int row)
{

    return &text[row - 1];
}
