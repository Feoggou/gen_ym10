#include "HeaderSide.h"
#include "GenDll.h"
#include "Tools.h"

#define clsname L"HEADERSIDE_GENCTRL"

extern CGenDll genDll;

CHeaderSide::CHeaderSide(void)
{
	m_nVBarPos = -1;
	m_pbIsMinute = 0;
}

CHeaderSide::~CHeaderSide(void)
{
	DeleteObject(m_hFont);
}

void CHeaderSide::Create(HWND hParent, RECT rect, HFONT hFont, int& nVBarPos)
{
	//rect: left = top = 0; right = latimea, bottom = inaltimea. 
	//deci limita dreapta si cea stanga vor fi de fapt:
	rect.bottom--;
	rect.right--;

	//daca nu exista clasa, va fi creata
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
			DisplayError(0);
	}

	//se creeaza fereastra
	m_hWnd = CreateWindowW(clsname, 0, WS_CHILD | WS_VISIBLE, rect.left, rect.top, 
rect.right - rect.left + 1, rect.bottom - rect.top + 1, hParent, NULL, genDll.m_hInstance, NULL);

	if (m_hWnd == 0)
	{
		DisplayError(0);
		PostQuitMessage(-1);
	}

	//se scrie "Secunda", sau, pentru repetitie in care s-a ales sa se foloseasca minute in loc de secunde,
	//"Minutul"
	m_hFont = hFont;
	HDC hDC = GetDC(m_hWnd);
	HFONT hOldFont = (HFONT)SelectObject(hDC, m_hFont);

	//gasim pointer la obiectul parintelui, pentru a determina daca se foloseste minut sau secunda.
	//pentru lyrics, este bIsMinute = false;
	GenListCtrl* pParent = (GenListCtrl*)GetWindowLongPtrW(hParent, GWL_USERDATA);
	m_pbIsMinute = &pParent->m_Repetition.bIsMinute;
	//se calculeaza m_nVBarPos, aici, ce rect e necesar pentru a desena " Secunda "
	DrawText(hDC, L" Secunda ", -1, &rect, DT_CALCRECT);
	SelectObject(hDC, hOldFont);
	ReleaseDC(m_hWnd, hDC);

	//se seteaza pozitia barii verticale
	m_nVBarPos = rect.right + 1;
	nVBarPos = m_nVBarPos;
	
	//salvam pointer la obiectul acesta ca userdata:
	SetWindowLongPtrW(m_hWnd, GWL_USERDATA, (LONG)this);
}

LRESULT CALLBACK CHeaderSide::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CHeaderSide* pThis = (CHeaderSide*)GetWindowLongPtrW(hWnd, GWL_USERDATA);
	if (pThis)
	switch (uMsg)
	{
	case WM_PAINT: pThis->OnPaint(); break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void CHeaderSide::OnPaint()
{
	//initializare
	PAINTSTRUCT ps;
	BeginPaint(m_hWnd, &ps);

	RECT clRect;
	GetClientRect(m_hWnd, &clRect);

	//desenam linii de culoare albastra, care tind din nuanta cFirst in nuanta cLast
	//indiferent cat de inalt ar fi HeaderSide, prima culoare va fi cFirst, iar ultima cLast
	//First = RGB(235, 243, 245), Last = RGB(197, 227, 238);
	for (int i = 0; i <= clRect.bottom; i++)
	{
		//pentru linii de pozitie i0, i1, i2, ... iN ( = clRect.bottom)
		//x specifica, in procente, pozitia liniei: prima = 0; linia de la sfarsit = 100 (i = iN);
		int x = i * 100 / clRect.bottom;

		//pentru fiecare culoare in parte,
		//red: x = 0 => red = 235 (cFirst); x = 100 => red = 235 - 100 * 38/100 = 235 - 38 = 197 (cLast)
		BYTE red = (byte)(235 - x * 38 /100);			//38 = 235 - 197
		BYTE green = (byte)(243 - x * 16 /100);			//16 = 243 - 227
		BYTE blue = (byte)(245 - x * 7 /100);			//7 = 245 - 238
		COLORREF cColor = RGB(red, green, blue);

		//se creeaza pen-ul (creionul) de culoarea aleasa
		HPEN hPen = CreatePen(PS_SOLID, 1, cColor);
		HPEN hOldPen = (HPEN)SelectObject(ps.hdc, hPen);

		//se deseneaza linia
		MoveToEx(ps.hdc, 0, i, NULL);
		LineTo(ps.hdc, clRect.right, i);

		SelectObject(ps.hdc, hOldPen);
		DeleteObject(hPen);
	}

	//pentru scrierea textului, fundalul va fi transparent
	SetBkMode(ps.hdc, TRANSPARENT);

	RECT rAux = clRect;
	//se calculeaza dimensiunea necesara scrierii cuvantului Secunda/Minutul
	DrawText(ps.hdc, L" Secunda ", -1, &rAux, DT_CALCRECT);

	//se deseneaza linia intre
	COLORREF clpen = GetSysColor(COLOR_3DDKSHADOW);
	HPEN hPen = CreatePen(PS_SOLID, 1, clpen);
	HPEN hOldPen = (HPEN)SelectObject(ps.hdc, hPen);

	MoveToEx(ps.hdc, rAux.right + 1, 0, NULL);
	LineTo(ps.hdc, rAux.right + 1, clRect.bottom);
	SelectObject(ps.hdc, hOldPen);
	DeleteObject(hPen);

	//se deseneaza elementele antetului
	
	rAux.bottom = clRect.bottom;
	if (*m_pbIsMinute)
		DrawText(ps.hdc, L" Minutul ", -1, &rAux, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	else
		DrawText(ps.hdc, L" Secunda ", -1, &rAux, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rAux.left = rAux.right + 1;
	rAux.right = clRect.right;
	DrawText(ps.hdc, L"Textul", -1, &rAux, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	
	EndPaint(m_hWnd, &ps);
}