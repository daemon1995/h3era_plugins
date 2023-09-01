#include "pch.h"
#include "MapScroller.h"

H3Position MapScroller::startMousePosition;
INT32 MapScroller::drawTime;

H3POINT MapScroller::startMousePoint;
H3POINT MapScroller::startScreenPosition;
H3POINT MapScroller::startScreenOffset;
bool MapScroller::needToScroll = false;
bool MapScroller::rmcAtMapScreen = false;

RECT MapScroller::scrollLimits;


void __stdcall AdvMgr_SetActiveHero(HiHook* h, H3AdventureManager* adv, int heroIdx, int a3, char a4, char a5)
{

	P_AdventureManager->screenDrawOffset.x = 0;
	P_AdventureManager->screenDrawOffset.y = 0;
	THISCALL_5(void, h->GetDefaultFunc(), adv, heroIdx, a3, a4, a5);
}
MapScroller::MapScroller(PatcherInstance* _PI, ExecErm* foo, void (*echo)(const char* str))

{


	MapScroller::echo = echo;

	initMapDrawBorders();
	_PI->WriteHiHook(0x40E2C0, THISCALL_, AdvMgr_MouseMove);
	_PI->WriteHiHook(0x408710, THISCALL_, AdvMgr_MapScreenProcedure);
	//_PI->WriteHiHook(0x411EB0, THISCALL_, AdvMgr_DrawArrowPath);
	_PI->WriteHiHook(0x412860, THISCALL_, AdvMgr_DrawGround);
	_PI->WriteHiHook(0x417A80, THISCALL_, AdvMgr_SetActiveHero);

	
	_PI->WriteLoHook(0x603093, OnRightClickDlgHold);
	_PI->WriteLoHook(0x417548, AdvMgr_MobilizeCurrentHero);
	//_PI->WriteLoHook(0x419657, OnScrollAdvMap);




	//edgeScrollHook = _PI->WriteAsmPatch(0x419886, "90 90 90 90 90");
	//edgeScrollHook->Undo();
}


_LHF_(MapScroller::AdvMgr_MobilizeCurrentHero)
{
	//restore map draw offset when selet hero
	P_AdventureManager->screenDrawOffset.x = 0;
	P_AdventureManager->screenDrawOffset.y = 0;
	return EXEC_DEFAULT;
}
_LHF_(MapScroller::OnRightClickDlgHold)
{


	//bool retType = EXEC_DEFAULT;

	if (c->eax == 8)
	{
		//c->return_address = 0x6030A3;
		//c->BL() = 0;
		c->eax = 16;
		return  EXEC_DEFAULT;

	}
	if (rmcAtMapScreen)
	{
		H3Msg* msg = reinterpret_cast<H3Msg*>(IntAt(c->edi));// -0x34);


		//echo("");
		echo(H3String::Format("%d", startMousePoint.x).String());
		echo(H3String::Format("%d", msg->GetX()).String());

		if (//1 ||
			msg->GetX() &&
			abs(msg->GetX() - startMousePoint.x) > 45
			|| abs(msg->GetY() - startMousePoint.y) > 45)
		{
			//	msg->command = eMsgCommand::RBUTTON_UP;
		//	c->BL() =1 ;
			c->eax = 16;

			//c->return_address = 0x6030E0;
		//	return  NO_EXEC_DEFAULT;
		}
	}


	return EXEC_DEFAULT;
}

void (*MapScroller::echo)(const char*);
void icho(int a)
{

	MapScroller::echo(H3String::Format("%d", a).String());
}

void MapScroller::initMapDrawBorders()
{

	scrollLimits.left = IntAt(0x4195F2 + 1); //
	scrollLimits.top = IntAt(0x41960A + 1);

	scrollLimits.right = CharAt(0x4195FC + 2);
	scrollLimits.bottom = CharAt(0x419615 + 2);

}



//MapScroller::Calc calc = H3POINT{ 0,0 };
int __stdcall MapScroller::AdvMgr_MapScreenProcedure(HiHook* h, H3AdventureManager* adv,  H3Msg* msg)
{
	//char middleMouse = GetAsyncKeyState(VK_MBUTTON);// &0x1;

	if (msg->command == eMsgCommand::MOUSE_BUTTON
		//&& msg->flags & 512
		&& (msg->itemId == 37 || msg->itemId == 0)
		&& msg->subtype == eMsgSubtype::RBUTTON_DOWN
		//&&
		//8000 
	//	&& GetKeyState(VK_MBUTTON) & 0x8000// ==0
		//middleMouse ==1
		//middleMouse
		)
	{
		//echo(H3String::Format("%d", middleMouse).String());
		//	
		//echo("rmc down");
		startMousePosition = adv->mousePosition;
		startMousePoint.x = msg->GetX();
		startMousePoint.y = msg->GetY();

		rmcAtMapScreen = true;
		P_AdventureManager->DemobilizeHero();

		int mapX = adv->screenPosition.GetX();
		int mapY = adv->screenPosition.GetY();
		//	if (mapX >= *P_MapSize)			mapX -= 1023; // draw outside map tiles
			//if (mapY >= *P_MapSize)			mapY -= 1023;
			//H3POINT p(adv->screenPosition.GetX(), adv->screenPosition.GetY());
		H3POINT p(mapX, mapY);


		startScreenPosition = p;// { adv->screenPosition.GetX(), adv->screenPosition.GetY() };
		startScreenOffset = adv->screenDrawOffset;
		//showStart();

		//startScreenOffset =  adv->screenDrawOffset;
	}

	if (msg->command == eMsgCommand::RBUTTON_UP
		|| msg->command == eMsgCommand::LCLICK_OUTSIDE
		|| msg->command == eMsgCommand::LBUTTON_UP)
	{

		rmcAtMapScreen = false;
		needToScroll = false;
		//	echo("rmc up");
		//	showCurrent();

	}
	if (msg->command == eMsgCommand::MOUSE_OVER)
	{
		int view_w = H3GameWidth::Get() - (800 - 592);
		int view_h = H3GameHeight::Get() - (600 - 544);
		if (msg->GetX() >= 8 && msg->GetX() <= 8 + view_w &&
			msg->position.x >= 8 && msg->position.y <= 8 + view_h)
		{
			msg->position.x -= adv->screenDrawOffset.x;
			msg->position.y -= adv->screenDrawOffset.y;
		}
	}

	return THISCALL_2(int, h->GetDefaultFunc(), adv, msg);

}

int __stdcall MapScroller::AdvMgr_DrawGround(HiHook* h, H3AdventureManager* adv, int a2, int a3, char a4, int xTileOffset, int yTileOffset)
{


	THISCALL_6(void, h->GetDefaultFunc(), adv, a2, a3, a4, xTileOffset, yTileOffset);
	//H3String toEcho = " screen ( %d , %d / %d, %d) ";
	//toEcho = H3String::Format(toEcho.String(), a2, a3, a4, xTileOffset);
	//echo(toEcho.String());
	return 0;
}

int __stdcall MapScroller::AdvMgr_DrawArrowPath(HiHook* h, H3AdventureManager* adv, __int64 a2, int a3, int a4, int a5)
{

	
	THISCALL_5(void, h->GetDefaultFunc(), adv, a2, a3, 5, 5);
	//H3String toEcho = " screen ( %d , %d / %d, %d) ";
	//toEcho = H3String::Format(toEcho.String(),a2, a3, a4, a5);
	//echo(toEcho.String());
	return 0;
}

void MapScroller::showStart()
{
	H3String toEcho = "start mouse ( %d , %d) | screen ( %d , %d / %d, %d) ";
	toEcho = H3String::Format(toEcho.String(), startMousePoint.x, startMousePoint.y, startScreenPosition.x, startScreenPosition.y, startScreenOffset.x, startScreenOffset.y);
	echo(toEcho.String());
}

void MapScroller::showCurrent(int x, int y)
{
	H3String toEcho = "current mouse ( %d , %d) | screen ( %d , %d / %d, %d) ";
	int mapX = P_AdventureManager->screenPosition.GetX();
	int mapY = P_AdventureManager->screenPosition.GetY();
	toEcho = H3String::Format(toEcho.String(), x, y, mapX, mapY, P_AdventureManager->screenDrawOffset.x, P_AdventureManager->screenDrawOffset.y);
	echo(toEcho.String());
}



int __stdcall MapScroller::AdvMgr_MouseMove(HiHook* h, H3AdventureManager* adv, int x, int y)
{
	//bool rmcIsHeld = && rmcAtMapScreen;

	if (rmcAtMapScreen
		&& GetKeyState(VK_RBUTTON) & 0x800//rmcAtMapScreen
	//	&& GetKeyState(VK_SHIFT) & 0x800
		//&& startScreenPosition.x>=0
		)  // if rmcIs held
	{


		if (CDECL_0(int, 0x4F8970) - drawTime >= 8)
		{


			int mapX = adv->screenPosition.GetX();
			int mapY = adv->screenPosition.GetY();
			int mapZ = adv->screenPosition.GetZ();

			if (mapX >= *P_MapSize)			mapX -= 1024; // draw outside map tiles
			if (mapY >= *P_MapSize)			mapY -= 1024;

			//	Calc calc({ x - startMousePoint.x, y - startMousePoint.y }, pos);

				//H3pooint of the click should have same offset as the offset of the mouse coordinates chnaged chan
				//
				//H3String toEcho = "mouse Offs ( %d , %d) | calc ( %d , %d / %d, %d) | mapX/Y (%d,%d)";

			H3POINT totalOffset(x - startMousePoint.x, y - startMousePoint.y);

			Calc calc(startScreenOffset, H3Position(startScreenPosition.x, startScreenPosition.y, mapZ));

			//	calc -= P_AdventureManager->screenDrawOffset;

				//icho(calc.point.x);
				//icho(calc.point.y);

			H3POINT storeScreenOffset = P_AdventureManager->screenDrawOffset;
			//totalOffset.x -= P_AdventureManager->screenDrawOffset.x;
		//	totalOffset.y -= P_AdventureManager->screenDrawOffset.y;

			calc += totalOffset;
			//calc -= storeScreenOffset;
			//calc.align();
			//calc -= P_AdventureManager->screenDrawOffset;
		//	H3String toEcho = "assuemed screen ( %d , %d / %d, %d)";
			//	toEcho = H3String::Format(toEcho.String(), calc.pos.x, calc.pos.y, calc.point.x, calc.point.y);


			//	showStart();
			///	echo(toEcho.String());
			//	adv->mousePosition = startMousePosition;

			//	calc -=P_AdventureManager->screenDrawOffset;
			P_AdventureManager->screenDrawOffset.x = calc.point.x;
			P_AdventureManager->screenDrawOffset.y = calc.point.y;
			//calc += storeScreenOffset;

		//	calc -= storeScreenOffset;

		//	showCurrent(x, y);
			mapX = calc.pos.x;
			mapY = calc.pos.y;


			mapX = Clamp(scrollLimits.left, mapX, scrollLimits.right + *P_MapSize);
			mapY = Clamp(scrollLimits.top, mapY, scrollLimits.bottom + *P_MapSize);
			//	toEcho = H3String::Format(toEcho.String(), totalOffset.x, totalOffset.y, calc.pos.x, calc.pos.y, startScreenPosition.x, startScreenPosition.y,mapX,mapY);
			if (mapX == scrollLimits.left
				|| mapX == scrollLimits.right + *P_MapSize)
				P_AdventureManager->screenDrawOffset.x = 0;


			if (mapY == scrollLimits.top
				|| mapY == scrollLimits.bottom + *P_MapSize)
				P_AdventureManager->screenDrawOffset.y = 0;



			//echo(toEcho.String());

			//		return 1;

			adv->screenPosition.SetX(mapX);
			adv->screenPosition.SetY(mapY);

			//	showCurrent(x,y);
				//H3Position temp(mapX, mapY, mapZ);
				//THISCALL_3(void, 0x419520, adv, 22, false);

				//return THISCALL_3(int, h->GetDefaultFunc(), adv, x, y);
		//	int refreshCounter = adv->refreshCounter;

			THISCALL_7(void, 0x412BA0, adv, adv->screenPosition, 1, 1, 0, 0, 0);//AdvMgr_Draw_MiniMap
			THISCALL_6(char, 0x40F350, adv, mapX, mapY, mapZ, 0, 0);
			P_WindowManager->H3Redraw(0, 8, IntAt(0x4196EA + 1), IntAt(0x4196E5 + 1));

			THISCALL_1(void, 0x47F5A0, adv);

			drawTime = CDECL_0(int, 0x4F8970);

		}

		//CDECL_0(void, 0x4F8640);
	}
	else
	{
		rmcAtMapScreen = false;
	}



	return THISCALL_3(int, h->GetDefaultFunc(), adv, x, y);

}

MapScroller::Calc::Calc(const H3POINT& p, const H3Position& pos)
	:point(p), pos(pos.GetX(), pos.GetY())
{
	int mapX = pos.GetX();
	int mapY = pos.GetY();
	if (mapX >= *P_MapSize)			mapX -= 1024; // draw outside map tiles
	if (mapY >= *P_MapSize)			mapY -= 1024;

	this->pos.x = mapX;
	this->pos.y = mapY;

	align();
}

void MapScroller::Calc::align() noexcept
{
	if (point.x > 31
		|| point.x < -31)
	{
		pos.x -= point.x / 31;
		point.x %= 31;
	}

	if (point.y > 31
		|| point.y < -31)
	{
		pos.y -= point.y / 31;
		point.y %= 31;
	}

}


void MapScroller::disableEdgeScroll(bool state)
{
	if (state)
	{
		edgeScrollHook->Apply();
	}
	else
	{
		edgeScrollHook->Undo();
	}

}



void MapScroller::Calc::operator+=(const H3POINT& other)
{
	point.x += other.x;
	point.y += other.y;
	align();
}

void MapScroller::Calc::operator+=(const H3Position& other)
{
	pos.x += other.GetX();
	pos.y += other.GetY();
}

void MapScroller::Calc::operator-=(const H3POINT& other)
{
	point.x -= other.x;
	point.y -= other.y;
	align();
}

void MapScroller::Calc::operator-=(const H3Position& other)
{
}

BOOL MapScroller::Calc::operator==(const H3POINT& other)
{
	return point == other;
}

BOOL MapScroller::Calc::operator!=(const H3POINT& other)
{
	return point != other;
}
