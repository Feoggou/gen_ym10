#include "ButtonBar.h"
#include "Tools.h"

#define CLASSNAME	L"BUTTONBAR_SPEC"
#define IDC_WINAMP_PLAY		40045
#define IDC_WINAMP_REWIND	40144
#define IDC_WINAMP_PAUSE	40046
#define IDC_WINAMP_STOP		40047
#define IDC_WINAMP_FARWARD	40148

extern CGenDll genDll;

CButtonBar::CButtonBar(void)
{
	m_hWnd = 0;
	m_dClicked = nothing;
	memset(&m_rClient, 0, sizeof(m_rClient));
}

CButtonBar::~CButtonBar(void)
{
	DeleteObject(m_hbFBack);
	DeleteObject(m_hbFFar);
	DeleteObject(m_hbPlay);
	DeleteObject(m_hbPause);
	DeleteObject(m_hbStop);

	DeleteObject(m_hbFBack2);
	DeleteObject(m_hbFFar2);
	DeleteObject(m_hbPlay2);
	DeleteObject(m_hbPause2);
	DeleteObject(m_hbStop2);
}

void CButtonBar::Create(HWND hParent, const RECT &rect)
{
	//daca nu exista clasa aceasta de fereastra, se creeaza
	WNDCLASS wndclass;
	if (FALSE == GetClassInfoW(genDll.m_hInstance, CLASSNAME, &wndclass))
	{
		memset(&wndclass, 0, sizeof(wndclass));
		wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wndclass.hInstance = genDll.m_hInstance;
		wndclass.lpfnWndProc = WindowProc;
		wndclass.lpszClassName = CLASSNAME;
		wndclass.lpszMenuName = 0;
		wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_SAVEBITS;

		if (false == RegisterClassW(&wndclass))
		{
			DisplayError(0);
			PostQuitMessage(-1);
		}
	}

	//se creeaza fereastra
	m_hWnd = CreateWindowW(CLASSNAME, L"", WS_CHILD | WS_VISIBLE, rect.left, rect.top, 
		rect.right - rect.left + 1, rect.bottom - rect.top + 1, hParent, 0, genDll.m_hInstance, 0);
	if (m_hWnd == 0)
	{
		DisplayError(0);
		PostQuitMessage(-1);
	}

	//se stocheaza extremitatiile zonei client a ferestrei in m_rClient
	GetClientRect(m_hWnd, &m_rClient);

	//se incarca fiecare imagine
	m_hbFBack = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_FBACK));
	m_hbFFar = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_FFAR));
	m_hbPlay = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_PLAY));
	m_hbPause = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_PAUSE));
	m_hbStop = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_STOP));

	m_hbFBack2 = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_FBACK2));
	m_hbFFar2 = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_FFAR2));
	m_hbPlay2 = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_PLAY2));
	m_hbPause2 = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_PAUSE2));
	m_hbStop2 = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_STOP2));

	SetWindowLongPtrW(m_hWnd, GWL_USERDATA, (LONG_PTR)this);
}

LRESULT CALLBACK CButtonBar::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CButtonBar* pThis = (CButtonBar*)GetWindowLongPtrW(hWnd, GWL_USERDATA);

	if (pThis)
	switch (uMsg)
	{
	case WM_PAINT: pThis->OnPaint(); break;

	case WM_LBUTTONDOWN:
		{
			WORD x = LOWORD(lParam);
			
			//se redeseneaza butonul care a fost apasat
			if (x >= 0 && x < 22) pThis->m_dClicked = fback;
			else if (x >= 22 && x < 44) pThis->m_dClicked = play;
			else if (x >= 44 && x < 66) pThis->m_dClicked = pause;
			else if (x >= 66 && x < 88) pThis->m_dClicked = stop;
			else if (x >= 88 && x < 110) pThis->m_dClicked = ffar;
			SetCapture(hWnd);

			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
		}
		break;

	case WM_LBUTTONUP:
		{
			WORD x = LOWORD(lParam);
			WORD y = HIWORD(lParam);

			pThis->m_dClicked = nothing;
			ReleaseCapture();
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);

			//se executa cod, pentru orice buton a fost apasat in CButtonBar
			if (y >= 0 && y <= 19)
			{
				if (x >= 0 && x < 22) pThis->OnCommand(fback);
				else if (x >= 22 && x < 44) pThis->OnCommand(play);
				else if (x >= 44 && x < 66) pThis->OnCommand(pause);
				else if (x >= 66 && x < 88) pThis->OnCommand(stop);
				else if (x >= 88 && x < 110) pThis->OnCommand(ffar);
			}
		}
		break;

	case WM_MOUSEMOVE:
		{
			//daca butonul stang al mouse-ului a fost apasat inainte
			if (wParam & MK_LBUTTON)
			{
				WORD x = LOWORD(lParam);
				WORD y = HIWORD(lParam);

				int now = pThis->m_dClicked;
			
				if (!(y >= 0 && y <= 19)) 
				{
					pThis->m_dClicked = nothing;
					RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
					ReleaseCapture();
					break;
				}

				//daca este deasupra unui buton din bara, se redeseneaza
				if (x >= 0 && x < 22) pThis->m_dClicked = fback;
				else if (x >= 22 && x < 44) pThis->m_dClicked = play;
				else if (x >= 44 && x < 66) pThis->m_dClicked = pause;
				else if (x >= 66 && x < 88) pThis->m_dClicked = stop;
				else if (x >= 88 && x < 110) pThis->m_dClicked = ffar;
				else
				{
					pThis->m_dClicked = nothing;
					RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
					ReleaseCapture();
					break;
				}

				if (now != pThis->m_dClicked)
					RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			}
		}
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void CButtonBar::OnPaint()
{
	PAINTSTRUCT ps;
	BeginPaint(m_hWnd, &ps);
	
	HDC hMemDC = CreateCompatibleDC(ps.hdc);
	HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, MAKEINTRESOURCE(NULL_BRUSH));

	//derulare inapoi:
	if (m_dClicked == fback)
		SelectObject(hMemDC, m_hbFBack2);
	else
		SelectObject(hMemDC, m_hbFBack);
	BitBlt(ps.hdc, 0, 0, 22, 19, hMemDC, 0, 0, SRCCOPY);

	//pornire:
	if (m_dClicked == play)
		SelectObject(hMemDC, m_hbPlay2);
	else
		SelectObject(hMemDC, m_hbPlay);
	BitBlt(ps.hdc, 22, 0, 22, 19, hMemDC, 0, 0, SRCCOPY);

	//pauza:
	if (m_dClicked == pause)
		SelectObject(hMemDC, m_hbPause2);
	else SelectObject(hMemDC, m_hbPause);
	BitBlt(ps.hdc, 44, 0, 22, 19, hMemDC, 0, 0, SRCCOPY);

	//stop:
	if (m_dClicked == stop)
		SelectObject(hMemDC, m_hbStop2);
	else
		SelectObject(hMemDC, m_hbStop);
	BitBlt(ps.hdc, 66, 0, 22, 19, hMemDC, 0, 0, SRCCOPY);

	//derulare inainte:
	if (m_dClicked == ffar)
		SelectObject(hMemDC, m_hbFFar2);
	else
		SelectObject(hMemDC, m_hbFFar);
	BitBlt(ps.hdc, 88, 0, 22, 19, hMemDC, 0, 0, SRCCOPY);

	SelectObject(hMemDC, hOldBmp);
	DeleteDC(hMemDC);

	EndPaint(m_hWnd, &ps);
}

void CButtonBar::OnCommand(but nCode)
{
	//se trimit comenzi Winamp-ului in functie de nCode
	switch (nCode)
	{
	case fback: SendMessage(genDll.m_hWinamp, WM_COMMAND, IDC_WINAMP_REWIND, 0); break;
	case play: SendMessage(genDll.m_hWinamp, WM_COMMAND, IDC_WINAMP_PLAY, 0); break;
	case pause: SendMessage(genDll.m_hWinamp, WM_COMMAND, IDC_WINAMP_PAUSE, 0); break;
	case stop: SendMessage(genDll.m_hWinamp, WM_COMMAND, IDC_WINAMP_STOP, 0); break;
	case ffar: SendMessage(genDll.m_hWinamp, WM_COMMAND, IDC_WINAMP_FARWARD, 0); break;
	}
}