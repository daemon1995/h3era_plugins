#pragma once
using namespace h3;
class MapScroller
{


public:

	struct Calc
	{
		H3POINT pos;

		H3POINT point;


		Calc(const H3POINT& p = { 0,0 }, const H3Position& pos = { 0 });

		void operator+=(const H3POINT& other);
		void operator+=(const H3Position& other);
		void operator-=(const H3POINT& other);
		void operator-=(const H3Position& other);
		BOOL operator==(const H3POINT& other);
		BOOL operator!=(const H3POINT& other);
		void align() noexcept;

	};

private:
	static INT32 drawTime;
	static H3POINT startMousePoint;
	static H3Position startMousePosition;
	static H3POINT startScreenPosition;
	static H3POINT startScreenOffset;
	static RECT scrollLimits;
	Patch* edgeScrollHook = nullptr;

	void initMapDrawBorders();
	static bool needToScroll;
	static bool rmcAtMapScreen;
	static _LHF_(OnRightClickDlgHold);
//	static _LHF_(OnScrollAdvMap);

	static int __stdcall AdvMgr_MouseMove(HiHook* h, H3AdventureManager* adv, int x, int y);
	static int __stdcall AdvMgr_MapScreenProcedure(HiHook* h, H3AdventureManager* adv,  H3Msg* msg);
	static int __stdcall AdvMgr_DrawArrowPath(HiHook* h, H3AdventureManager* adv, __int64 a2, int a3, int a4, int a5);
	static int __stdcall AdvMgr_DrawGround(HiHook* h, H3AdventureManager* adv, signed int xTile, int yTile, char isUnder, int xTileOffset, int yTileOffset);

	static _LHF_(AdvMgr_MobilizeCurrentHero);

public:
	static void (*echo)(const char*);

	MapScroller(PatcherInstance* _PI, ExecErm* foo, void (*echo)(const char* str));
	void disableEdgeScroll(bool state);
	static void showStart();
	static void showCurrent(int x = 0, int y = 0);


};

