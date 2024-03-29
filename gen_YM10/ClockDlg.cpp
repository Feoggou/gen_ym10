#include "ClockDlg.h"
#include "GenDll.h"
#include "Tools.h"

#include "resource.h"

extern CGenDll genDll;

CClockDlg::CClockDlg(CLOCK* pSpec)
{
	m_pSpec = pSpec;
	m_hDlg = 0;
}

CClockDlg::~CClockDlg(void)
{
}

INT_PTR CClockDlg::DoModal(HWND hParent)
{
	INT_PTR nResult = DialogBoxParam(genDll.m_hInstance, MAKEINTRESOURCE(IDD_CLOCK), hParent, &CClockDlg::DialogProc, (LPARAM)this);
	if (nResult == -1)
		DisplayError(0);
	return nResult;
}

INT_PTR CALLBACK CClockDlg::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//o singura caseta de dialog va fi creata o data
	static CClockDlg* pThis = NULL;

	if (pThis == NULL)
		if (uMsg == WM_INITDIALOG)
		{
			pThis = (CClockDlg*)lParam;
			pThis->m_hDlg = hDlg;
		}
		else return 0;


	switch (uMsg)
	{
	case WM_DESTROY: pThis = 0; break;
	case WM_CLOSE: EndDialog(hDlg, IDCANCEL); break;
	case WM_COMMAND:
		{
			if (LOWORD(wParam) == IDC_EDIT_HOURS || LOWORD(wParam) == IDC_EDIT_MINUTES)
			{
				switch (HIWORD(wParam))
				{
				case EN_SETFOCUS:	pThis->OnEditSetFocus(LOWORD(wParam), (HWND)lParam); return 0;
				case EN_KILLFOCUS:	pThis->OnEditKillFocus(LOWORD(wParam)); return 0;
				case EN_UPDATE:		pThis->OnEditUpdate(LOWORD(wParam), (HWND)lParam); return 0;
				}
			}
			
			switch (LOWORD(wParam))
			{
			case IDOK: pThis->OnOk(); break;
			case IDCANCEL: EndDialog(hDlg, IDCANCEL); break;
			}
		}
		break;//case WM_COMMAND
	case WM_INITDIALOG: return pThis->OnInitDialog();
	}

	return 0;
}

int CClockDlg::OnInitDialog()
{
	//setam numarul maxim de caractere care se pot scrie in casetele de editare
	SendMessage(GetDlgItem(m_hDlg, IDC_EDIT_HOURS), EM_LIMITTEXT, (WPARAM)2, 0);
	SendMessage(GetDlgItem(m_hDlg, IDC_EDIT_MINUTES), EM_LIMITTEXT, (WPARAM)2, 0);
	SendMessage(GetDlgItem(m_hDlg, IDC_TEXTBEF), EM_LIMITTEXT, (WPARAM)250, 0);
	SendMessage(GetDlgItem(m_hDlg, IDC_TEXTAFTER), EM_LIMITTEXT, (WPARAM)256, 0);

	if (m_pSpec == 0)
	{
		//se initializeaza casetele cu valorile "default"
		SetDlgItemText(m_hDlg, IDC_EDIT_HOURS, L"0");
		SetDlgItemText(m_hDlg, IDC_EDIT_MINUTES, L"0");
		SetDlgItemText(m_hDlg, IDC_TEXTBEF, L"Mă întorc în");
		SetDlgItemText(m_hDlg, IDC_TEXTAFTER, L"Ar trebui să fiu online acum...");
	}
	else
	{
		//se editeaza un obiect(ceas) deja existent, deci se initializeaza cu valorile obiectului(ceasului) respectiv
		SetDlgItemInt(m_hDlg, IDC_EDIT_HOURS, m_pSpec->dHours, 0);
		SetDlgItemInt(m_hDlg, IDC_EDIT_MINUTES, m_pSpec->dMins, 0);
		SetDlgItemTextW(m_hDlg, IDC_TEXTBEF, m_pSpec->wsTextBefore.c_str());
		SetDlgItemTextW(m_hDlg, IDC_TEXTAFTER, m_pSpec->wsTextAfter.c_str());
	}

	return true;
}

void CClockDlg::OnEditSetFocus(WORD wID, HWND hEdit)
{
	UINT value = (int)GetDlgItemInt(m_hDlg, wID, 0, FALSE);
	//daca valoarea afisata este 0, nu afisam nimic
	if (value == 0) 
	{
		SetDlgItemText(m_hDlg, wID, L"");
	}
	//daca valoarea afisata este de genul "09" scriem "9"
	else if (value < 10 && GetWindowTextLength(hEdit) > 1)
	{
		SetDlgItemInt(m_hDlg, wID, value, FALSE);
	}
}

void CClockDlg::OnEditKillFocus(WORD wID)
{
	BOOL bGood;
	GetDlgItemInt(m_hDlg, wID, &bGood, FALSE);
	//daca textul scris nu poate fi convertit in numar, afisam "0"
	if (bGood == FALSE) 
	{
		SetDlgItemInt(m_hDlg, wID, 0, FALSE);
	}
}

void CClockDlg::OnEditUpdate(WORD wID, HWND hEdit)
{
	UINT _max;
	if (wID == IDC_EDIT_HOURS) _max = 23;
	else _max = 59;

	UINT value = (int)GetDlgItemInt(m_hDlg, wID, 0, FALSE);
	//daca este scrisa o valoare mai mare decat permisa, se scrie valoarea maxima permisa.
	if (value > _max) 
	{
		value = _max;
		SetDlgItemInt(m_hDlg, wID, value, FALSE);
		SendMessage(hEdit, EM_SETSEL, 2, 2);
	}
}

void CClockDlg::OnOk()
{	
	//stocam in inregistrarea Ceas valorile
	m_Clock.dHours = (byte)GetDlgItemInt(m_hDlg, IDC_EDIT_HOURS, 0, FALSE);
	m_Clock.dMins = (byte)GetDlgItemInt(m_hDlg, IDC_EDIT_MINUTES, 0, FALSE);

	if (m_Clock.dHours == 0 && m_Clock.dMins == 0)
	{
		MessageBox(m_hDlg, L"Trebuie să scrii o valoare diferită de 0 la cel puțin la numărul secundelor sau al minutelor.", L"Atenție", MB_ICONWARNING);
		return;
	}

	int lenTxtBef = GetWindowTextLength(GetDlgItem(m_hDlg, IDC_TEXTBEF));
	int lenTxtAfter = GetWindowTextLength(GetDlgItem(m_hDlg, IDC_TEXTAFTER));

	//daca avem text scris in IDC_TEXTBEF
	if (lenTxtBef > 0)
	{
		//preluam textul si adaugam un ' '
		lenTxtBef++;
		WCHAR* wsTextBefore = new WCHAR[lenTxtBef];
		GetWindowTextW(GetDlgItem(m_hDlg, IDC_TEXTBEF), wsTextBefore, lenTxtBef);
		m_Clock.wsTextBefore = wsTextBefore;
		m_Clock.wsTextBefore += ' ';
		delete[] wsTextBefore;
	}

	//daca avem text scris in IDC_TEXTAFTER
	if (lenTxtAfter > 0)
	{
		//preluam textul
		lenTxtAfter++;
		WCHAR* wsTextAfter = new WCHAR[lenTxtAfter];
		GetWindowTextW(GetDlgItem(m_hDlg, IDC_TEXTAFTER), wsTextAfter, lenTxtAfter);
		m_Clock.wsTextAfter = wsTextAfter;
		delete[] wsTextAfter;
	}

	EndDialog(m_hDlg, IDOK);
}