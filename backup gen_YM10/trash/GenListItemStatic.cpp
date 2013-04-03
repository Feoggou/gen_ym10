#include "GenListItemStatic.h"
#include "GenDll.h"
#include "ListSide.h"

extern CGenDll genDll;
WNDPROC GenListItemStatic::OldWndProc;

GenListItemStatic::GenListItemStatic(void)
{
}

GenListItemStatic::~GenListItemStatic(void)
{
}

void GenListItemStatic::Create(HWND hParent, RECT &rect, int nLine)
{
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | SS_CENTER;
	char str[] = "0:00";

	m_hWnd = CreateWindowA("STATIC", str, dwStyle, rect.left, rect.top, rect.right - rect.left + 1, rect.bottom - rect.top + 1, hParent, NULL, 
		genDll.m_hInstance, NULL);

	SendMessage(m_hWnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 1);

	//setting the window procedure
	OldWndProc = (WNDPROC)SetWindowLongPtrW(m_hWnd, GWL_WNDPROC, (LONG_PTR)WindowProc);
}

LRESULT CALLBACK GenListItemStatic::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT: OnPaint(hWnd); break;
	case WM_LBUTTONDOWN: OnLButtonDown(hWnd, wParam); break;
	}

	return CallWindowProc(OldWndProc, hWnd, uMsg, wParam, lParam);
}

void GenListItemStatic::OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	BeginPaint(hWnd, &ps);

	RECT clRect;
	GetClientRect(hWnd, &clRect);
	DrawTextA(ps.hdc, "0-00", 5, &clRect, DT_CENTER | DT_VCENTER);

	EndPaint(hWnd, &ps);
}

void GenListItemStatic::OnLButtonDown(HWND hWnd, WPARAM wParam)
{
}