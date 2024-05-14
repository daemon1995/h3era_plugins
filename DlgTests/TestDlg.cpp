#include "pch.h"
#include "TestDlg.h"

TestDlg::TestDlg(int width, int height, int x, int y)
	:H3Dlg(width,height,x,y,false, false)
{

	this->CreateOKButton();
}

BOOL TestDlg::DialogProc(H3Msg& msg)
{
	return 1;
}

H3Vector<H3DlgItem*>& TestDlg::items()
{
	return this->dlgItems;
	// TODO: вставьте здесь оператор return
}

TestDlg::~TestDlg()
{

}
//
//_LHF_(gem_Dlg_MainMenu_Create)
//{
//
//	H3Dlg* dlg = reinterpret_cast<H3Dlg*>(c->ecx);
//
//
//
//
//	if (dlg)
//	{
//		static	int index = 10;
//		auto bg = dlg->GetBackgroundPcx();
//		if (bg)
//		{
//			const int WIDTH = 800;
//			const int HEIGHT = 600;
//			//auto pcx = H3DlgPcx16::Create(0, 0, WIDTH, HEIGHT, 1323, 0);
//
//		//	H3LoadedPcx16* buf = H3LoadedPcx16::Create(WIDTH, HEIGHT);
//		//	memset(buf->buffer,22, buf->buffSize);
//
//		//	pcx->SetPcx(buf);
//		//	dlg->AddItem(pcx);
//
//
//		}
//		//
//		//return EXEC_DEFAULT;
//		//H3Messagebox::RMB();
//
//
//		H3LoadedDef* adag = H3LoadedDef::Load("adag.def");
//		const int SIZE = adag->groups[0]->count;
//
//		const int WIDTH = adag->widthDEF;
//		const int HEIGHT = adag->heightDEF;
//		pathArrowsPcx16HalfWidth.Reserve(SIZE);
//		pathArrowsPcx16WidthX2.Reserve(SIZE);
//
//		H3LoadedPcx16* buf = H3LoadedPcx16::Create(WIDTH, HEIGHT);
//
//		for (size_t i = 0; i < SIZE; i++)
//		{
//
//			memset(buf->buffer, 0, buf->buffSize);
//			adag->DrawToPcx16(0, i, buf, 0, 0);
//
//			H3LoadedPcx16* resizedAdagHalfWidth = H3LoadedPcx16::Create(WIDTH / 2, HEIGHT);
//
//			DrawPcx16ResizedBicubic(resizedAdagHalfWidth, buf, WIDTH, HEIGHT, 0, 0, WIDTH / 2, HEIGHT);
//			pathArrowsPcx16HalfWidth.Push(resizedAdagHalfWidth);
//
//
//			H3LoadedPcx16* resizedAdagWidthX2 = H3LoadedPcx16::Create(WIDTH * 2, HEIGHT);
//			DrawPcx16ResizedBicubic(resizedAdagWidthX2, buf, WIDTH, HEIGHT, 0, 0, WIDTH * 2, HEIGHT);
//			pathArrowsPcx16WidthX2.Push(resizedAdagWidthX2);
//
//
//		}
//
//		buf->Destroy();
//
//
//		{
//			H3LoadedPcx16* srcPcxA = H3LoadedPcx16::Load("nhv5_1.pcx");
//			pcxToDraw = H3LoadedPcx16::Create(srcPcxA->width, srcPcxA->height);
//
//			memset(pcxToDraw->buffer, 0, pcxToDraw->buffSize);
//
//			srcPcxA->DrawToPcx16(0, 0, 0, pcxToDraw, 0, 0);
//			//	return EXEC_DEFAULT;
//
//			//	constexpr int WIDTH = 100;
//			//	constexpr int HEIGHT = 200;
//			const int WIDTHA = 800;
//			const int HEIGHTA = 600;
//			pcxToResize = H3LoadedPcx16::Create(WIDTHA, HEIGHTA);
//			DrawPcx16ResizedBicubic(pcxToResize, pcxToDraw, pcxToDraw->width, pcxToDraw->height, 0, 0, pcxToResize->width, pcxToResize->height);
//			H3DlgPcx16* pcx;
//
//
//
//			pcx = H3DlgPcx16::Create(0, 0, WIDTHA, HEIGHTA, 1323, 0);
//
//
//			pcx->SetPcx(pcxToResize);
//			//dlg->AddItem(pcx);
//
//
//		}
//		//return EXEC_DEFAULT;
//
//		H3DlgPcx16* pcx;
//		int x = 24;
//		int y = 24;
//		int _y = 200 + y;
//
//		for (size_t i = 0; i < SIZE; i++)
//		{
//			if (i % 10 == 0)
//				x = 24;
//			if (i % 10 == 0)
//			{
//				y += HEIGHT + 4;
//				_y += HEIGHT + 4;
//			}
//
//			pcx = H3DlgPcx16::Create(x, y, WIDTH / 2, HEIGHT, 2425 + i, 0);
//
//			pcx->SetPcx(pathArrowsPcx16HalfWidth[i]);
//			dlg->AddItem(pcx);
//
//			pcx = H3DlgPcx16::Create(x, _y, WIDTH * 2, HEIGHT, 3425 + i, 0);
//
//			pcx->SetPcx(pathArrowsPcx16WidthX2[i]);
//			dlg->AddItem(pcx);
//
//			x += WIDTH + 25;
//
//		}
//
//	}
//	return EXEC_DEFAULT;
//}
//
//
//#include<shellapi.h>
//NOTIFYICONDATA nid;
//
//void CreateSystemTrayIcon(HWND hwnd, HICON icon, LPCSTR tooltip);
//bool exists = false;
//// Function to remove the system tray icon
//
//void CreateSystemTrayIcon(HWND hwnd, HICON icon, LPCSTR tooltip) {
//	nid.cbSize = sizeof(NOTIFYICONDATA);
//	nid.hWnd = hwnd;
//	nid.uID = 112; // Unique ID for the icon
//	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
//	nid.uCallbackMessage = WM_USER; // Custom message for tray icon events
//	nid.hIcon = icon;
//	strcpy(nid.szTip, tooltip);
//	exists = true;
//	Shell_NotifyIcon(NIM_ADD, &nid);
//}
//void RemoveSystemTrayIcon(HWND hwnd) {
//	Shell_NotifyIcon(NIM_DELETE, &nid);
//}
//LRESULT CALLBACK MainWindowProc(HiHook* h, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
//{
//
//	switch (msg)
//	{
//
//
//
//	case WM_CREATE:
//		// Create the system tray icon when the window is created
//		MessageBox(0, "123", "123", MB_OK);
//
//		break;
//	case WM_DESTROY:
//		// Remove the system tray icon when the window is destroyed
//		RemoveSystemTrayIcon(hwnd);
//		break;
//	case WM_SYSCOMMAND:
//		if (wParam == SC_MINIMIZE) {
//			// Minimize event intercepted
//			// Handle the minimize event here
//			// For example, hide the window and show a system tray icon
//			// Your code here
//
//			if (!exists)
//			{
//				const char* st = (char*)0x67F870;
//				// GetClassInfo(GetForegroundWindow(), st,)
//				WNDCLASS s{};// = nullptr;
//
//				GetClassInfo(GetModuleHandleA(0), st, &s);
//
//				//CreateSystemTrayIcon(hwnd, LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APPLICATION)), "My Application");
//				CreateSystemTrayIcon(hwnd, s.hIcon, "daemon_n learned sth");
//			}
//
//
//			ShowWindow(hwnd, SW_HIDE);
//			return 0; // Prevent the default minimize behavior
//		}
//	case WM_USER:
//		// Handle the custom message for tray icon events
//		if (LOWORD(lParam) == WM_LBUTTONDBLCLK) {
//			// The user clicked on the system tray icon
//			// Show the application window
//
//			SetForegroundWindow(hwnd);
//			ShowWindow(hwnd, SW_SHOW);
//
//		}
//		break;
//	default:
//		break;
//	}
//
//
//	return STDCALL_4(int, h->GetDefaultFunc(), hwnd, msg, wParam, lParam);
//}
//_PI->WriteHiHook(0x4F8290, STDCALL_, MainWindowProc);
//int __stdcall FooBar(HiHook* h , H3LoadedDef* a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13, int a14)
//{
//
//
//	if (rand() %3)
//	{
//		auto mgr = H3AdventureManager::Get();
//		//return THISCALL_14(int, h->GetDefaultFunc(),H3LoadedDef::Load("artifact.def"), a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
//
//	}
//
//	return THISCALL_14(int, h->GetDefaultFunc(), a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
//}
//H3LoadedPcx16* pcxToResize = nullptr;
//H3LoadedPcx16* pcxToDraw = nullptr;
//
//H3Vector<H3LoadedPcx16*> pathArrowsPcx16HalfWidth{};
//H3Vector<H3LoadedPcx16*> pathArrowsPcx16WidthX2{};
//
//
