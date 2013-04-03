#include "GenListCtrl.h"
#include "GenDll.h"

extern CGenDll genDll;

WNDPROC GenListCtrl::m_OldWndProc;
WNDPROC	GenListCtrl::OldCheckProc;

GenListCtrl::GenListCtrl(void)
{
	m_nTextHeight = 0;
	m_nVBarPos = 0;
	m_bEnableLineSel = false;

	WNDCLASS wndclass;
	if (FALSE == GetClassInfo(genDll.m_hInstance, L"GENLISTCTRL", &wndclass))
	{
		wndclass.style = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc = WindowProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = genDll.m_hInstance;
		wndclass.hIcon = NULL;
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = L"GENLISTCTRL";

		if (FALSE == RegisterClass(&wndclass))
		{
			DWORD dwError = GetLastError();
			WCHAR wstr[50];
			wsprintf(wstr, L"Error 0x%x: Could not create window", dwError);
			MessageBox(genDll.m_hWinamp, wstr, L"Fatal Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
		}
	}
}

GenListCtrl::~GenListCtrl(void)
{
}

LRESULT CALLBACK GenListCtrl::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_SETFOCUS:
		{
			GenListCtrl* pThis = (GenListCtrl*)GetWindowLongPtrW(hWnd, GWL_USERDATA);
			SetFocus(pThis->m_Table.m_Lines.front().edSecond.m_hWnd);
		}
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK GenListCtrl::CheckProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case BM_SETCHECK:
		{
			HWND hEdit = GetDlgItem(GetParent(hWnd), IDC_EDIT_NOTIMES);
			if (wParam == BST_CHECKED)
			{
				EnableWindow(hEdit, false);
				SetWindowTextA(hEdit, "");
			}
			else
			{
				EnableWindow(hEdit, true);
				char sText[7];
				GetWindowTextA(hEdit, sText, 7);
				if (strlen(sText) == 0)
					SetWindowTextA(GetDlgItem(GetParent(hWnd), IDC_EDIT_NOTIMES), "1");
			}
		}
		break;//BM_SETCHECK
	}

	return CallWindowProc(OldCheckProc, hWnd, uMsg, wParam, lParam);
}

void GenListCtrl::Create(HWND hWnd, bool bEnableLineSel, bool bIsLyrics)
{
	m_bIsLyrics = bIsLyrics;
	m_bEnableLineSel = bEnableLineSel;
	m_hWnd = hWnd;

	//we need to make some calculations:
	HDC hDC = GetDC(m_hWnd);
	
	LOGFONT lf;
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	GetObject(hFont, sizeof(LOGFONT), &lf);
	lf.lfWeight = FW_SEMIBOLD;
	lf.lfQuality = PROOF_QUALITY;
	HFONT hHeader = CreateFontIndirect(&lf);

	//CLIENT rect
	GetClientRect(m_hWnd, &m_rClient);
	//HEADER rect
	m_rHeader = m_rClient;
	m_rHeader.bottom = abs(MulDiv(72, lf.lfHeight, GetDeviceCaps(hDC, LOGPIXELSY)));
	m_nTextHeight = m_rHeader.bottom;
	m_rHeader.bottom *= 8; m_rHeader.bottom /= 3;
	int nVBarPos;
	m_Header.Create(m_hWnd, m_rHeader, hHeader, nVBarPos);

	//TABLE rect
	m_rTable.left = 0;
	m_rTable.right = m_rClient.right;
	m_rTable.top = m_rHeader.bottom;
	m_rTable.bottom = m_rClient.bottom;
	m_Table.Create(m_hWnd, m_rTable, hFont, nVBarPos, m_nTextHeight * 2, m_bEnableLineSel, m_bIsLyrics);

	ReleaseDC(hWnd, hDC);

	//setting the listctrl:
	m_OldWndProc = (WNDPROC)SetWindowLongPtrW(hWnd, GWL_WNDPROC, (LONG_PTR)WindowProc);
	SetWindowLongPtrW(m_hWnd, GWL_USERDATA, (LONG_PTR)this);

	//setting the checkbox:
	HWND hCheckBtn = GetDlgItem(GetParent(hWnd), IDC_CHECK_NOCOUNTER);
	OldCheckProc = (WNDPROC)SetWindowLongPtrW(hCheckBtn, GWL_WNDPROC, (LONG_PTR)CheckProc);
}
