#include "ButtonBar.h"

#define CLASSNAME	"BUTTONBAR_SPEC"
#define IDC_WINAMP_PLAY		40045
#define IDC_WINAMP_REWIND	40144
#define IDC_WINAMP_PAUSE	40046
#define IDC_WINAMP_STOP		40047
#define IDC_WINAMP_FARWARD	40148

extern CGenDll genDll;

CButtonBar::CButtonBar(void)
{
	m_hWnd = 0;
	m_dClicked = but::nothing;
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
	WNDCLASSA wndclass;
	if (FALSE == GetClassInfoA(genDll.m_hInstance, CLASSNAME, &wndclass))
	{
		memset(&wndclass, 0, sizeof(wndclass));
		wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wndclass.hInstance = genDll.m_hInstance;
		wndclass.lpfnWndProc = WindowProc;
		wndclass.lpszClassName = CLASSNAME;
		wndclass.lpszMenuName = 0;
		wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_SAVEBITS;

		if (false == RegisterClassA(&wndclass))
		{
			DWORD dwError = GetLastError();
			char str[30];
			sprintf_s(str, 30, "Error 0x%x", dwError);
			MessageBoxA(hParent, str, "Fatal Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
			PostQuitMessage(-1);
		}
	}

		m_hWnd = CreateWindowA(CLASSNAME, "", WS_CHILD | WS_VISIBLE, rect.left, rect.top, 
			rect.right - rect.left + 1, rect.bottom - rect.top + 1, hParent, 0, genDll.m_hInstance, 0);
		if (m_hWnd == 0)
		{
			DWORD dwError = GetLastError();
			char str[30];
			sprintf_s(str, 30, "Error 0x%x", dwError);
			MessageBoxA(hParent, str, "Fatal Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
			PostQuitMessage(-1);
		}

		GetClientRect(m_hWnd, &m_rClient);

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

		SetWindowLongPtrA(m_hWnd, GWL_USERDATA, (LONG_PTR)this);
}

LRESULT CALLBACK CButtonBar::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT: OnPaint(hWnd); break;

	case WM_LBUTTONDOWN:
		{
			WORD x = LOWORD(lParam);
			CButtonBar* pThis = (CButtonBar*)GetWindowLongPtrA(hWnd, GWL_USERDATA);
			
			if (x >= 0 && x < 22) pThis->m_dClicked = but::fback;
			else if (x >= 22 && x < 44) pThis->m_dClicked = but::play;
			else if (x >= 44 && x < 66) pThis->m_dClicked = but::pause;
			else if (x >= 66 && x < 88) pThis->m_dClicked = but::stop;
			else if (x >= 88 && x < 110) pThis->m_dClicked = but::ffar;
			SetCapture(hWnd);

			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
		}
		break;

	case WM_LBUTTONUP:
		{
			WORD x = LOWORD(lParam);
			WORD y = HIWORD(lParam);

			CButtonBar* pThis = (CButtonBar*)GetWindowLongPtrA(hWnd, GWL_USERDATA);
			pThis->m_dClicked = but::nothing;
			ReleaseCapture();
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);

			if (y >= 0 && y <= 19)
			{
				if (x >= 0 && x < 22) OnCommand(hWnd, but::fback);
				else if (x >= 22 && x < 44) OnCommand(hWnd, but::play);
				else if (x >= 44 && x < 66) OnCommand(hWnd, but::pause);
				else if (x >= 66 && x < 88) OnCommand(hWnd, but::stop);
				else if (x >= 88 && x < 110) OnCommand(hWnd, but::ffar);
			}
		}
		break;

	case WM_MOUSEMOVE:
		{
			if (wParam & MK_LBUTTON)
			{
				WORD x = LOWORD(lParam);
				WORD y = HIWORD(lParam);
				CButtonBar* pThis = (CButtonBar*)GetWindowLongPtrA(hWnd, GWL_USERDATA);

				int now = pThis->m_dClicked;
			
				if (!(y >= 0 && y <= 19)) 
				{
					pThis->m_dClicked = but::nothing;
					RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
					ReleaseCapture();
					break;
				}

				if (x >= 0 && x < 22) pThis->m_dClicked = but::fback;
				else if (x >= 22 && x < 44) pThis->m_dClicked = but::play;
				else if (x >= 44 && x < 66) pThis->m_dClicked = but::pause;
				else if (x >= 66 && x < 88) pThis->m_dClicked = but::stop;
				else if (x >= 88 && x < 110) pThis->m_dClicked = but::ffar;
				else
				{
					pThis->m_dClicked = but::nothing;
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

void CButtonBar::OnPaint(HWND hWnd)
{
	CButtonBar* pThis = (CButtonBar*)GetWindowLongPtrA(hWnd, GWL_USERDATA);

	PAINTSTRUCT ps;
	BeginPaint(hWnd, &ps);
	
	HDC hMemDC = CreateCompatibleDC(ps.hdc);
	HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, MAKEINTRESOURCE(NULL_BRUSH));

	//fback:
	if (pThis->m_dClicked == but::fback)
		SelectObject(hMemDC, pThis->m_hbFBack2);
	else
		SelectObject(hMemDC, pThis->m_hbFBack);
	int nresult = BitBlt(ps.hdc, 0, 0, 22, 19, hMemDC, 0, 0, SRCCOPY);

	//play:
	if (pThis->m_dClicked == but::play)
		SelectObject(hMemDC, pThis->m_hbPlay2);
	else
		SelectObject(hMemDC, pThis->m_hbPlay);
	BitBlt(ps.hdc, 22, 0, 22, 19, hMemDC, 0, 0, SRCCOPY);

	//pause:
	if (pThis->m_dClicked == but::pause)
		SelectObject(hMemDC, pThis->m_hbPause2);
	else SelectObject(hMemDC, pThis->m_hbPause);
	BitBlt(ps.hdc, 44, 0, 22, 19, hMemDC, 0, 0, SRCCOPY);

	//stop:
	if (pThis->m_dClicked == but::stop)
		SelectObject(hMemDC, pThis->m_hbStop2);
	else
		SelectObject(hMemDC, pThis->m_hbStop);
	BitBlt(ps.hdc, 66, 0, 22, 19, hMemDC, 0, 0, SRCCOPY);

	//ffar:
	if (pThis->m_dClicked == but::ffar)
		SelectObject(hMemDC, pThis->m_hbFFar2);
	else
		SelectObject(hMemDC, pThis->m_hbFFar);
	BitBlt(ps.hdc, 88, 0, 22, 19, hMemDC, 0, 0, SRCCOPY);

	SelectObject(hMemDC, hOldBmp);
	DeleteDC(hMemDC);

	EndPaint(hWnd, &ps);
}

void CButtonBar::OnCommand(HWND hWnd, but nCode)
{
	switch (nCode)
	{
	case but::fback: SendMessage(genDll.m_hWinamp, WM_COMMAND, IDC_WINAMP_REWIND, 0); break;
	case but::play: SendMessage(genDll.m_hWinamp, WM_COMMAND, IDC_WINAMP_PLAY, 0); break;
	case but::pause: SendMessage(genDll.m_hWinamp, WM_COMMAND, IDC_WINAMP_PAUSE, 0); break;
	case but::stop: SendMessage(genDll.m_hWinamp, WM_COMMAND, IDC_WINAMP_STOP, 0); break;
	case but::ffar: SendMessage(genDll.m_hWinamp, WM_COMMAND, IDC_WINAMP_FARWARD, 0); break;
	}
}