#pragma once

namespace scroll
{
	using namespace h3;
	class MapScroller :public IGamePatch
	{



	private:

		RECT scrollLimits;

		H3POINT startMousePoint;
		H3Position startMousePosition;
		H3POINT startScreenPosition;
		H3POINT startScreenOffset;
		DWORD sinceLastDrawTime = NULL;

		BOOL scrollingDone = FALSE;
		BOOL rmcAtMapScreen = FALSE;
		BOOL wheelButtonAtMapScreen = FALSE;
		BOOL isMapView;
		BOOL needToShowHdModDlg;
		BOOL inMoveAction;
		Patch* edgeScrollHook;



		struct Calculator
		{
			H3POINT pos;
			H3POINT point;
			Calculator(const H3POINT& p = { 0,0 }, const H3Position& pos = { 0 });

			void operator+=(const H3POINT& other);
			void align() noexcept;

		} calculator;

	protected:
		virtual void CreatePatches() noexcept final;
	private:

		static _LHF_(BaseDlg_OnRightClickHold);

		static int __stdcall AdvMgr_MouseMove(HiHook* h, H3AdventureManager* adv, int x, int y) noexcept;
		static int __stdcall AdvMgr_MapScreenProcedure(HiHook* h, H3AdventureManager* adv, H3Msg* msg) noexcept;
		static void __stdcall AdvMgr_SetActiveHero(HiHook* h, H3AdventureManager* adv, int heroIdx, int a3, char a4, char a5) noexcept;

		static _LHF_(AdvMgr_MobilizeCurrentHero);
		
		MapScroller();
	public:
		void SetMapEdgeScrollStatus(const bool state);
		static MapScroller& Get();// (PatcherInstance* _PI);

	};




}
