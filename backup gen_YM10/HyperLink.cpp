// HyperLink.cpp : implementation file
//

#include "HyperLink.h"
#include "ChooseDlg.h"

WNDPROC CHyperLink::OldWndProc = 0;

CHyperLink::CHyperLink()
{
	m_hWnd = NULL;
	m_bUnderline = 0;
}

CHyperLink::~CHyperLink()
{
}


LRESULT CALLBACK CHyperLink::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_PAINT)
	{
		OnPaint(hWnd); return 0;
	}

	return CallWindowProc(OldWndProc, hWnd, uMsg, wParam, lParam);
}


void CHyperLink::OnPaint(HWND hWnd)
{
	CHyperLink* pThis = (CHyperLink*)GetWindowLongPtrW(hWnd, GWL_USERDATA);
	PAINTSTRUCT ps;
	BeginPaint(hWnd, &ps);

	HDC hDC = ps.hdc;

	//se creeaza brush-ul
	HBRUSH hBrush = GetSysColorBrush(COLOR_3DFACE);
	SelectObject(hDC, hBrush);
	SetBkMode(hDC, TRANSPARENT);

	//se creeaza fontul
	LOGFONT logfont;
	HFONT hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
	GetObject(hFont, sizeof(LOGFONT), &logfont);

	if (pThis->m_bUnderline == TRUE)
	{
		logfont.lfUnderline = TRUE;
	}
	else 
		logfont.lfUnderline = FALSE;

	hFont = CreateFontIndirect(&logfont);
	SelectObject(hDC, hFont);

	//se seteaza culoarea
	SetTextColor(hDC, RGB(34,0,204));
	
	//se afiseaza textul
	int len = GetWindowTextLength(hWnd);
	len++;
	WCHAR* wstr = new WCHAR[len];
	GetWindowTextW(hWnd, wstr, len);

	RECT rect;
	GetClientRect(hWnd, &rect);

	DrawTextW(hDC, wstr, -1, &rect, DT_LEFT);

	DeleteObject(hBrush);
	DeleteObject(hFont);
	delete[] wstr;

	EndPaint(hWnd, &ps);
}