#include <crtdbg.h>

#include "RepetitionDlg.h"
#include "GenDll.h"
#include "resource.h"
#include "Tools.h"

extern CGenDll genDll;
WNDPROC OldRepProc, OldUseMinutesProc;

//noua functie pentru prelucrarea mesajelor casetei de editare unde se specifica numarul de afisari a textului.
LRESULT CALLBACK NewRepProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

CRepetitionDlg::CRepetitionDlg(GenListCtrl::REPETE* pSpec)
{
	m_pSpec = pSpec;
}

CRepetitionDlg::~CRepetitionDlg(void)
{
}

INT_PTR CRepetitionDlg::DoModal(HWND hParent)
{
	INT_PTR nResult = DialogBoxParam(genDll.m_hInstance, MAKEINTRESOURCE(IDD_REPETITION), hParent, DialogProc, (LPARAM)this);
	if (nResult == -1)
		DisplayError(0);

	return nResult;
}

INT_PTR CALLBACK CRepetitionDlg::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//numai o singura caseta de dialog poate exisat in acelasi timp
	static CRepetitionDlg* pThis = NULL;

	if (pThis == NULL)
		if (uMsg == WM_INITDIALOG)
		{
			pThis = (CRepetitionDlg*)lParam;
			pThis->m_hDlg = hDlg;
			SetWindowLongPtr(hDlg, GWL_USERDATA, (LONG)pThis);
		}
		else return 0;

	switch (uMsg)
	{
	case WM_DESTROY: pThis = 0; break;
	case WM_CLOSE: EndDialog(hDlg, IDCANCEL); break;
	case WM_COMMAND:
		{			
			switch (LOWORD(wParam))
			{
			case IDOK: pThis->OnOk(); break;
			case IDCANCEL: EndDialog(hDlg, IDCANCEL); break;
			}
		}
		break;//case WM_COMMAND
	case WM_INITDIALOG: pThis->OnInitDialog();break;
	}

	return 0;
}

LRESULT CALLBACK NewRepProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KILLFOCUS:
		{
			//valoarea minima e 1; trebuie scrisa o valoare neaparat.
			WCHAR wstr[7];
			GetWindowText(hWnd, wstr, 7);
			if (wcslen(wstr) == 0)
			{
				SetWindowText(hWnd, L"1");
				break;
			}
			int nr = _wtoi(wstr);
			if (nr == 0) SetWindowText(hWnd, L"1");
		}
		break;
	}

	return CallWindowProc(OldRepProc, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CRepetitionDlg::NewUseMinutesProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case BM_SETCHECK:
		{
			//se salveaza alegerea in bIsMinute si se redeseneaza antetul controlului lista
			//acela va afisa "Minutul" sau "Secunda"
			CRepetitionDlg* pParent = (CRepetitionDlg*)GetWindowLongPtrW(GetParent(hWnd), GWL_USERDATA);
			pParent->m_list.m_Repetition.bIsMinute = (0 != wParam);
			InvalidateRect(pParent->m_list.m_Header.m_hWnd, 0, 1);
		}
		break;
	}

	return CallWindowProc(OldUseMinutesProc, hWnd, uMsg, wParam, lParam);
}

void CRepetitionDlg::OnInitDialog()
{
	//se creeaza controlul GenListCtrl pentru repetitie
	m_list.Create(GetDlgItem(m_hDlg, IDC_REPETITION), false);

	//se initializeaza caseta de editare pentru specificarea numarului afisarilor
	HWND hRepCnt = GetDlgItem(m_hDlg, IDC_EDIT_NOTIMES);
	SendMessage(hRepCnt, EM_LIMITTEXT, 2, 0);
	SetWindowText(hRepCnt, L"1");
	//se seteaza noua functie pentru prelucrarea mesajelor
	OldRepProc = (WNDPROC)SetWindowLongPtrW(hRepCnt, GWL_WNDPROC, (LONG_PTR)NewRepProc);

	SetWindowText(m_list.m_Table.m_Lines[0].edSecond.m_hWnd, L"2");

	//setam noua functie de prelucrare a mesajelor pentru a putea actualiza antetul cand se bifeaza/debifeaza
	//caseta "Foloseste minute"
	OldUseMinutesProc = (WNDPROC) SetWindowLongPtrW(GetDlgItem(m_hDlg, IDC_USEMINUTES), GWL_WNDPROC, (LONG)NewUseMinutesProc);
	//daca m_pSpec = 0, inseamna ca nu editam o repetitie existenta, ce cream una noua
	if (m_pSpec == 0) return;
	m_list.m_Repetition = *m_pSpec;

	if (m_pSpec->nRepCnt == -1)
	{
		EnableWindow(GetDlgItem(m_hDlg, IDC_EDIT_NOTIMES), false);
		SendMessage(GetDlgItem(m_hDlg, IDC_CHECK_NOCOUNTER), BM_SETCHECK, BST_CHECKED, 0);
	}
	else
	{
		SetDlgItemInt(m_hDlg, IDC_EDIT_NOTIMES, m_pSpec->nRepCnt, true);
		SendMessage(GetDlgItem(m_hDlg, IDC_CHECK_NOCOUNTER), BM_SETCHECK, BST_UNCHECKED, 0);
	}
	SendMessage(GetDlgItem(m_hDlg, IDC_USEMINUTES), BM_SETCHECK, m_pSpec->bIsMinute, 0);
	InvalidateRect(m_list.m_Table.m_hWnd, 0, 1);

	//copiem versurile/textul
	int size = m_pSpec->Verses.size();
	for (int i = 0; i < size; i++)
	{
		if (i != 0)
		{
			//daca este primul, avem deja prima linie. altfel, trebuie sa adaugam pe fiecare.
			m_list.m_Table.AddLine();
		}

		//atunci setam doar textul
		HWND hEdit = m_list.m_Table.m_Lines[i].edSecond.m_hWnd;
		WCHAR wstr[7];
		_itow_s(m_pSpec->Verses[i].nrSec, wstr, 7, 10);
		SetWindowText(hEdit, wstr);

		//afisam textul in caseta de editare
		hEdit = m_list.m_Table.m_Lines[i].edText.m_hWnd;
		SetWindowTextW(hEdit, m_pSpec->Verses[i].wsVerse.c_str());
	}
}

void CRepetitionDlg::OnOk()
{
	WCHAR wstr[7];
	GetWindowText(m_list.m_Table.m_Lines[0].edSecond.m_hWnd, wstr, 6);
	if (wcscmp(wstr, L"0") == 0)
	{
		MessageBox(m_hDlg, L"Prima secundă/primul minut trebuie să nu fie 0.", L"Repetiție", MB_ICONWARNING);
		SetFocus(m_list.m_Table.m_Lines[0].edSecond.m_hWnd);
		m_list.m_Table.ScrollTo(0);
		return;
	}

	//trebuie sa ne asiguram ca toate secundele/minutele au fost scrise corect.
	deque<CListSide::LINE>::iterator I, J;
	bool bWrong = false;
	//verificam numai daca sunt mai multe linii (nu una)
	if (m_list.m_Table.m_Lines.size() > 1)
	{
		//verificam de la linia a 2-a, intrucat prima linie nu contine nimic.
		for (I = m_list.m_Table.m_Lines.begin() + 1; I != m_list.m_Table.m_Lines.end(); I++)
		{
			//trebuie sa nu fie ultimul
			if (I->nIndex != m_list.m_Table.m_Lines.back().nIndex)
			{
				J = I + 1;
				WCHAR wstr[7];
				GetWindowText(I->edSecond.m_hWnd, wstr, 7);
				int valI = _wtoi(wstr);
				GetWindowText(J->edSecond.m_hWnd, wstr, 7);
				int valJ = _wtoi(wstr);
				if (valI >= valJ)
				{
					bWrong = true;
					break;
				}
			}
		}

		if (bWrong)
		{
			MessageBoxW(m_hDlg, L"Toate secundele de la a doua până la sfârșit trebuiesc setate în ordine crescătoare, te rog setează o valoare corectă.", L"Repetiție", MB_ICONEXCLAMATION);
			//selectam secunda/minutul scris gresit
			SetFocus(J->edSecond.m_hWnd);
			SendMessage(J->edSecond.m_hWnd, EM_SETSEL, 0, 5);
			//derulam la linia respectiva
			RECT rect;
			GetWindowRect(J->edSecond.m_hWnd, &rect);
			POINT pt = {rect.left, rect.top};
			ScreenToClient(GetParent(J->edSecond.m_hWnd), &pt);
			m_list.m_Table.ScrollTo(pt.y);
			return;
		}
	}

	else 
	{
		MessageBox(m_hDlg, L"Trebuie să ai cel puțin două linii ca să poți salva repetiția", L"Repetiție", MB_ICONEXCLAMATION);
		return;
	}

	bWrong = true;

	//verificam sa fie macar o linie de tabel cu text
	for (I = m_list.m_Table.m_Lines.begin(); I != m_list.m_Table.m_Lines.end(); I++)
	{
		if (GetWindowTextLength(I->edText.m_hWnd))
		{
			bWrong = false;
			break;
		}
	}

	if (bWrong)
	{
		MessageBoxW(m_hDlg, L"Trebuie sa scrii text in macar o linie ca sa poti salva.", L"Repetiție", MB_ICONEXCLAMATION);
		return;
	}
	

	//daca este bine, scriem toate informatiile
	if (BST_UNCHECKED == SendMessage(GetDlgItem(m_hDlg, IDC_CHECK_NOCOUNTER), BM_GETCHECK, 0, 0))
		m_list.m_Repetition.nRepCnt = GetDlgItemInt(m_hDlg, IDC_EDIT_NOTIMES, NULL, 0);
	else m_list.m_Repetition.nRepCnt = -1;

	m_list.m_Repetition.bIsMinute = (0 != SendMessage(GetDlgItem(m_hDlg, IDC_USEMINUTES), BM_GETCHECK, 0, 0));

	//salvam fiecare vers din tabel/lista (secunda/minutul si textul)
	for (I = m_list.m_Table.m_Lines.begin(); I != m_list.m_Table.m_Lines.end(); I++)
	{
		GenListCtrl::VERSE verse;
		//secunda/minutul
		wchar_t* wstr = new wchar_t[7];
		GetWindowTextW(I->edSecond.m_hWnd, wstr, 7);
		verse.nrSec = (WORD)_wtoi(wstr);
		delete[] wstr;

		//textul
		wstr = new WCHAR[100];
		GetWindowTextW(I->edText.m_hWnd, wstr, 100);
		verse.wsVerse = wstr;
		delete[] wstr;

		//il adaugam in lista.
		m_list.m_Repetition.Verses.push_back(verse);
	}

	EndDialog(m_hDlg, IDOK);
}