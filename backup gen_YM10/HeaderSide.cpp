#include "HeaderSide.h"
#include "GenDll.h"

#define clsname L"HEADERSIDE_GENCTRL"

extern CGenDll genDll;
int CHeaderSide::m_nVBarPos;
HFONT CHeaderSide::m_hFont;

CHeaderSide::CHeaderSide(void)
{
	m_nVBarPos = -1;
}

CHeaderSide::~CHeaderSide(void)
{
	DeleteObject(m_hFont);
}

void CHeaderSide::Create(HWND hParent, RECT rect, HFONT hFont, int& nVBarPos)
{
	rect.bottom--;
	rect.right--;

	WNDCLASS wndclass;
	if (FALSE == GetClassInfo(genDll.m_hInstance, clsname, &wndclass))
	{
		ZeroMemory(&wndclass, sizeof(WNDCLASS));

		wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass.hInstance = genDll.m_hInstance;
		wndclass.lpfnWndProc = WindowProc;
		wndclass.lpszClassName = clsname;
		wndclass.style = CS_VREDRAW | CS_HREDRAW | CS_SAVEBITS;

		if (FALSE ==  RegisterClass(&wndclass))
		{
			DWORD dwError = GetLastError();
			char str[20];
			sprintf(str, "Error 0x%x", dwError);
			MessageBoxA(genDll.m_hWinamp, str, "Fatal Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
			PostQuitMessage(-1);
			}
	}

	m_hWnd = CreateWindowW(clsname, 0, WS_CHILD | WS_VISIBLE, rect.left, rect.top, 
rect.right - rect.left + 1, rect.bottom - rect.top + 1, hParent, NULL, genDll.m_hInstance, NULL);

	if (m_hWnd == 0)
	{
		DWORD dwError = GetLastError();
		char str[20];
		sprintf(str, "Error 0x%x", dwError);
		MessageBoxA(genDll.m_hWinamp, str, "Fatal Error!", MB_ICONERROR);
#ifdef _DEBUG
		DebugBreak();
#endif
		PostQuitMessage(-1);
	}

	//setting the window procedure
	m_hFont = hFont;
	SelectObject(GetDC(m_hWnd), m_hFont);


//	RECT rect2;
//	GetClientRect(m_hWnd, &rect2);
	DrawText(GetDC(m_hWnd), L" Secunda ", -1, &rect, DT_CALCRECT);

	m_nVBarPos = rect.right + 1;
	nVBarPos = m_nVBarPos;
}

LRESULT CALLBACK CHeaderSide::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT: OnPaint(hWnd); break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void CHeaderSide::OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	BeginPaint(hWnd, &ps);

	RECT clRect;
	GetClientRect(hWnd, &clRect);

	COLORREF cFirst = RGB(235, 243, 245), cLast = RGB(197, 227, 238);
	for (int i = 0; i <= clRect.bottom; i++)
	{
		int x = i * 100 / clRect.bottom;

		BYTE red = 235 - x * (235 - 197)/100;
		BYTE green = 243 - x * (243 - 227)/100;
		BYTE blue = 245 - x * (245 - 238)/100;
		COLORREF cColor = RGB(red, green, blue);

		HPEN hPen = CreatePen(PS_SOLID, 1, cColor);
		SelectObject(ps.hdc, hPen);

		MoveToEx(ps.hdc, 0, i, NULL);
		LineTo(ps.hdc, clRect.right, i);

		DeleteObject(hPen);
	}

	SetBkMode(ps.hdc, TRANSPARENT);

	RECT rAux = clRect;
	DrawText(ps.hdc, L" Secunda ", -1, &rAux, DT_CALCRECT);

//	m_nVBarPos = rAux.right + 1;

	//drawing the line between
	COLORREF clpen = GetSysColor(COLOR_3DDKSHADOW);
	HPEN hPen = CreatePen(PS_SOLID, 1, clpen);
	HPEN hOldPen = (HPEN)SelectObject(ps.hdc, hPen);

	MoveToEx(ps.hdc, rAux.right + 1, 0, NULL);
	LineTo(ps.hdc, rAux.right + 1, clRect.bottom);
	DeleteObject(hPen);
	SelectObject(ps.hdc, hOldPen);

	//drawing the elements of the header
	
	rAux.bottom = clRect.bottom;
	DrawText(ps.hdc, L" Secunda ", -1, &rAux, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rAux.left = rAux.right + 1;
	rAux.right = clRect.right;
	DrawText(ps.hdc, L"Textul", -1, &rAux, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	
	EndPaint(hWnd, &ps);
}