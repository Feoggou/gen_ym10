#include "SongDetDlg.h"

extern CGenDll genDll;

CSongDetDlg::CSongDetDlg(SPECIAL* pSpec)
{
	m_SongDet = 0;
	m_pSpec = pSpec;
}

CSongDetDlg::~CSongDetDlg(void)
{
}

int CSongDetDlg::DoModal(HWND hParent)
{
	int nResult = DialogBoxParam(genDll.m_hInstance, MAKEINTRESOURCE(IDD_SONGINFO), hParent, &CSongDetDlg::DialogProc, (LPARAM)this);
	if (nResult == -1)
	{
		DWORD dwError = GetLastError();
		WCHAR sError[100];
		wsprintf(sError, L"Error 0x%x", dwError);
		MessageBox(hParent, sError, L"Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
	}

	return nResult;
}

INT_PTR CALLBACK CSongDetDlg::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE: EndDialog(hDlg, IDCANCEL); break;
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK: OnOk(hDlg); break;
			case IDCANCEL: EndDialog(hDlg, IDCANCEL); break;
			}
		}
		break;//case WM_COMMAND
	case WM_INITDIALOG: OnInitDialog(hDlg, lParam);break;
	}

	return 0;
}

void CSongDetDlg::OnOk(HWND hDlg)
{
	CSongDetDlg* pThis = (CSongDetDlg*)GetWindowLongPtrW(hDlg, GWL_USERDATA);
	if (BST_CHECKED == SendMessage(GetDlgItem(hDlg, IDC_ARTIST), BM_GETCHECK, 0, 0))
		pThis->m_SongDet |= info::artist;
	if (BST_CHECKED == SendMessage(GetDlgItem(hDlg, IDC_SONG), BM_GETCHECK, 0, 0))
		pThis->m_SongDet |= info::song;
	if (BST_CHECKED == SendMessage(GetDlgItem(hDlg, IDC_PROGRESS), BM_GETCHECK, 0, 0))
		pThis->m_SongDet |= info::progress;
	if (BST_CHECKED == SendMessage(GetDlgItem(hDlg, IDC_LENGTH), BM_GETCHECK, 0, 0))
		pThis->m_SongDet |= info::length;

	if (pThis->m_SongDet & info::progress)
		if (BST_CHECKED == SendMessage(GetDlgItem(hDlg, IDC_STYLE), BM_GETCHECK, 0, 0))
			pThis->m_SongDet |= info::percent;

	if (pThis->m_SongDet == 0)
	{
		MessageBoxA(hDlg, "Trebuie să bifezi cel puțin o casetă dacă vrei să adaugi detalii despre melodie.", "Informații despre melodie", MB_ICONEXCLAMATION); 
		return;
	}

	EndDialog(hDlg, IDOK);
}

WNDPROC OldProgrProc;
LRESULT CALLBACK NewProgrProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case BM_SETCHECK:
		{
			HWND h1 = GetDlgItem(GetParent(hWnd), IDC_STYLE);
			HWND h2 = GetDlgItem(GetParent(hWnd), IDC_RADIO2);
			if (wParam == BST_CHECKED)
			{
				EnableWindow(h1, true);
				EnableWindow(h2, true);
				SendMessage(h1, BM_SETCHECK, BST_CHECKED, 0);
			}
			else
			{
				EnableWindow(h1, false);
				EnableWindow(h2, false);
				SendMessage(h1, BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessage(h2, BM_SETCHECK, BST_UNCHECKED, 0);
			}
		}
	}

	return CallWindowProc(OldProgrProc, hWnd, uMsg, wParam, lParam);
}

void CSongDetDlg::OnInitDialog(HWND hDlg, LPARAM lParam)
{
	CSongDetDlg* pThis = (CSongDetDlg*)lParam;
	SetWindowLong(hDlg, GWL_USERDATA, (LONG)lParam);

	//idc artist, song
	SendMessage(GetDlgItem(hDlg, IDC_ARTIST), BM_SETCHECK, BST_CHECKED, 0);
	SendMessage(GetDlgItem(hDlg, IDC_SONG), BM_SETCHECK, BST_CHECKED, 0);

	OldProgrProc = (WNDPROC)SetWindowLongPtrW(GetDlgItem(hDlg, IDC_PROGRESS), GWL_WNDPROC, (LONG_PTR)NewProgrProc);
}

