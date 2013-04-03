#include "ClockDlg.h"
#include "GenDll.h"

#include "resource.h"

extern CGenDll genDll;

CClockDlg::CClockDlg(CLOCK* pSpec)
{
	m_pSpec = pSpec;
}

CClockDlg::~CClockDlg(void)
{
}

int CClockDlg::DoModal(HWND hParent)
{
	return DialogBoxParam(genDll.m_hInstance, MAKEINTRESOURCE(IDD_CLOCK), hParent, &CClockDlg::DialogProc, (LPARAM)this);
}

INT_PTR CALLBACK CClockDlg::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE: EndDialog(hDlg, IDCANCEL); break;
	case WM_COMMAND:
		{
			if (LOWORD(wParam) == IDC_EDIT_HOURS || LOWORD(wParam) == IDC_EDIT_MINUTES)
			{
				switch (HIWORD(wParam))
				{
				case EN_SETFOCUS:	OnEditSetFocus(hDlg, wParam, lParam); return 0;
				case EN_KILLFOCUS:	OnEditKillFocus(hDlg, wParam, lParam); return 0;
				case EN_UPDATE:		OnEditUpdate(hDlg, wParam, lParam); return 0;
				}
			}
			
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

void CClockDlg::OnInitDialog(HWND hDlg, LPARAM lParam)
{
	CClockDlg*	pThis = (CClockDlg*)lParam;
	SetWindowLong(hDlg, GWL_USERDATA, (LONG)lParam);

	SendMessage(GetDlgItem(hDlg, IDC_EDIT_HOURS), EM_LIMITTEXT, (WPARAM)2, 0);
	SendMessage(GetDlgItem(hDlg, IDC_EDIT_MINUTES), EM_LIMITTEXT, (WPARAM)2, 0);
//	SendMessage(GetDlgItem(hDlg, IDC_EDIT_SECONDS), EM_LIMITTEXT, (WPARAM)2, 0);
	SendMessage(GetDlgItem(hDlg, IDC_TEXTBEF), EM_LIMITTEXT, (WPARAM)250, 0);
	SendMessage(GetDlgItem(hDlg, IDC_TEXTAFTER), EM_LIMITTEXT, (WPARAM)256, 0);

	if (pThis->m_pSpec == 0)
	{
		SetDlgItemText(hDlg, IDC_EDIT_HOURS, L"0");
		SetDlgItemText(hDlg, IDC_EDIT_MINUTES, L"0");
//		SetDlgItemText(hDlg, IDC_EDIT_SECONDS, L"0");
		SetDlgItemText(hDlg, IDC_TEXTBEF, L"Mã întorc în");
		SetDlgItemText(hDlg, IDC_TEXTAFTER, L"Ar trebui sã fiu online acum...");
	}
	else
	{
		SetDlgItemInt(hDlg, IDC_EDIT_HOURS, pThis->m_pSpec->dHours, 0);
		SetDlgItemInt(hDlg, IDC_EDIT_MINUTES, pThis->m_pSpec->dMins, 0);
//		SetDlgItemInt(hDlg, IDC_EDIT_SECONDS, pThis->m_pSpec->dSecs, 0);
		SetDlgItemTextA(hDlg, IDC_TEXTBEF, pThis->m_pSpec->sTextBefore.c_str());
		SetDlgItemTextA(hDlg, IDC_TEXTAFTER, pThis->m_pSpec->sTextAfter.c_str());
	}
}

void CClockDlg::OnEditSetFocus(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	HWND hWnd = (HWND)lParam;
	UINT value = (int)GetDlgItemInt(hDlg, LOWORD(wParam), 0, FALSE);
	if (value == 0) 
	{
		SetDlgItemText(hDlg, LOWORD(wParam), L"");
	}
	else if (value < 10 && GetWindowTextLength(hWnd) > 1)
	{
		SetDlgItemInt(hDlg, LOWORD(wParam), value, FALSE);
	}
}

void CClockDlg::OnEditKillFocus(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	HWND hWnd = (HWND)lParam;
	BOOL bGood;
	GetDlgItemInt(hDlg, LOWORD(wParam), &bGood, FALSE);
	if (bGood == FALSE) 
	{
		SetDlgItemInt(hDlg, LOWORD(wParam), 0, FALSE);
	}
}

void CClockDlg::OnEditUpdate(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	int _max;
	if (LOWORD(wParam) == IDC_EDIT_HOURS) _max = 23;
	else _max = 59;

	HWND hWnd = (HWND)lParam;
	UINT value = (int)GetDlgItemInt(hDlg, LOWORD(wParam), 0, FALSE);
	if (value > _max) 
	{
		value = _max;
		SetDlgItemInt(hDlg, LOWORD(wParam), value, FALSE);
		SendMessage(hWnd, EM_SETSEL, 2, 2);
	}
}

void CClockDlg::OnOk(HWND hDlg)
{
	CClockDlg* pThis = (CClockDlg*)GetWindowLong(hDlg, GWL_USERDATA);
	
	pThis->m_Clock.dHours = GetDlgItemInt(hDlg, IDC_EDIT_HOURS, 0, FALSE);
	pThis->m_Clock.dMins = GetDlgItemInt(hDlg, IDC_EDIT_MINUTES, 0, FALSE);
//	pThis->m_Clock.dSecs = GetDlgItemInt(hDlg, IDC_EDIT_SECONDS, 0, FALSE);
	int lenTxtBef = GetWindowTextLength(GetDlgItem(hDlg, IDC_TEXTBEF));
	int lenTxtAfter = GetWindowTextLength(GetDlgItem(hDlg, IDC_TEXTAFTER));
	if (lenTxtBef > 0)
	{
		lenTxtBef++;
		char* sTextBefore = new char[lenTxtBef];
		GetWindowTextA(GetDlgItem(hDlg, IDC_TEXTBEF), sTextBefore, lenTxtBef);
		pThis->m_Clock.sTextBefore = sTextBefore;
		pThis->m_Clock.sTextBefore += ' ';
		delete[] sTextBefore;
	}

	if (lenTxtAfter > 0)
	{
		lenTxtAfter++;
		char* sTextAfter = new char[lenTxtAfter];
//		sTextAfter = new char[lenTxtAfter];
		GetWindowTextA(GetDlgItem(hDlg, IDC_TEXTAFTER), sTextAfter, lenTxtAfter);
		pThis->m_Clock.sTextAfter = sTextAfter;
		delete[] sTextAfter;
	}

	EndDialog(hDlg, IDOK);
}