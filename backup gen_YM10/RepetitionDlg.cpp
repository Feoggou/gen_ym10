#include "RepetitionDlg.h"
#include "GenDll.h"
#include "resource.h"

extern CGenDll genDll;

CRepetitionDlg::CRepetitionDlg(GenListCtrl::REPETE* pSpec)
{
	m_pSpec = pSpec;
}

CRepetitionDlg::~CRepetitionDlg(void)
{
}

int CRepetitionDlg::DoModal(HWND hParent)
{
	int nResult = DialogBoxParam(genDll.m_hInstance, MAKEINTRESOURCE(IDD_REPETITION), hParent, DialogProc, (LPARAM)this);
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

INT_PTR CALLBACK CRepetitionDlg::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

WNDPROC OldRepProc;
LRESULT CALLBACK NewRepProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KILLFOCUS:
		{
			char str[7];
			GetWindowTextA(hWnd, str, 7);
			if (strlen(str) == 0)
			{
				SetWindowTextA(hWnd, "1");
				break;
			}
			int nr = atoi(str);
			if (nr == 0) SetWindowTextA(hWnd, "1");
		}
		break;
	}

	return CallWindowProc(OldRepProc, hWnd, uMsg, wParam, lParam);
}

void CRepetitionDlg::OnInitDialog(HWND hDlg, LPARAM lParam)
{
	CRepetitionDlg*	pThis = (CRepetitionDlg*)lParam;
	SetWindowLong(hDlg, GWL_USERDATA, (LONG)lParam);

	pThis->m_list.Create(GetDlgItem(hDlg, IDC_REPETITION), false, false);

	HWND hRepCnt = GetDlgItem(hDlg, IDC_EDIT_NOTIMES);
	SendMessage(hRepCnt, EM_LIMITTEXT, 2, 0);
	SetWindowTextA(hRepCnt, "1");
	OldRepProc = (WNDPROC)SetWindowLongPtrW(hRepCnt, GWL_WNDPROC, (LONG_PTR)NewRepProc);

	if (pThis->m_pSpec == 0) return;

	if (pThis->m_pSpec->nRepCnt == -1)
	{
		EnableWindow(GetDlgItem(hDlg, IDC_EDIT_NOTIMES), false);
		SendMessage(GetDlgItem(hDlg, IDC_CHECK_NOCOUNTER), BM_SETCHECK, BST_CHECKED, 0);
	}
	else
	{
//		EnableWindow(GetDlgItem(hDlg, IDC_EDIT_NOTIMES), true);
		SetDlgItemInt(hDlg, IDC_EDIT_NOTIMES, pThis->m_pSpec->nRepCnt, true);
		SendMessage(GetDlgItem(hDlg, IDC_CHECK_NOCOUNTER), BM_SETCHECK, BST_UNCHECKED, 0);
	}

	int size = pThis->m_pSpec->Verses.size();
	for (int i = 0; i < size; i++)
	{
		if (i != 0)
		{
			//if it is the first, we already have the first line. else, we must add each
			pThis->m_list.m_Table.AddLine();
		}

		//then only set text
		HWND hEdit = pThis->m_list.m_Table.m_Lines[i].edSecond.m_hWnd;
		char str[7];
		itoa(pThis->m_pSpec->Verses[i].nrSec, str, 10);
		SetWindowTextA(hEdit, str);

		hEdit = pThis->m_list.m_Table.m_Lines[i].edText.m_hWnd;
		SetWindowTextA(hEdit, pThis->m_pSpec->Verses[i].sVerse.c_str());
	}
}

void CRepetitionDlg::OnOk(HWND hDlg)
{
	CRepetitionDlg* pThis = (CRepetitionDlg*)GetWindowLongPtrW(hDlg, GWL_USERDATA);

	//we must check that all seconds were written correctly
	deque<CListSide::LINE>::iterator I, J;
	bool bWrong = false;
	for (I = pThis->m_list.m_Table.m_Lines.begin(); I != pThis->m_list.m_Table.m_Lines.end(); I++)
	{
		//must not be the last
		if (I->nIndex != pThis->m_list.m_Table.m_Lines.back().nIndex)
		{
			J = I + 1;
			char str[7];
			GetWindowTextA(I->edSecond.m_hWnd, str, 7);
			int valI = atoi(str);
			GetWindowTextA(J->edSecond.m_hWnd, str, 7);
			int valJ = atoi(str);
			if (valI >= valJ)
			{
				bWrong = true;
				break;
			}
		}
	}

	bool bWrong2 = true;

	for (I = pThis->m_list.m_Table.m_Lines.begin(); I != pThis->m_list.m_Table.m_Lines.end(); I++)
	{
		//must not be the last
		if (GetWindowTextLength(I->edText.m_hWnd))
		{
			bWrong2 = false;
			break;
		}
	}

	if (bWrong2) bWrong = true;

	if (bWrong)
	{
		MessageBoxA(hDlg, "Toate secundele trebuiesc setate în ordine crescãtoare, te rog seteazã o valoare corectã.", "Repetare", MB_ICONEXCLAMATION);
		//focus & selection
		SetFocus(J->edSecond.m_hWnd);
		SendMessage(J->edSecond.m_hWnd, EM_SETSEL, 0, 5);
		//scroll
		RECT rect;
		GetWindowRect(J->edSecond.m_hWnd, &rect);
		POINT pt = {rect.left, rect.top};
		ScreenToClient(GetParent(J->edSecond.m_hWnd), &pt);
		pThis->m_list.m_Table.ScrollTo(pt.y);
		return;
	}

	//if it is right, then fill the info
	if (BST_UNCHECKED == SendMessage(GetDlgItem(hDlg, IDC_CHECK_NOCOUNTER), BM_GETCHECK, 0, 0))
		pThis->m_list.m_Repetition.nRepCnt = GetDlgItemInt(hDlg, IDC_EDIT_NOTIMES, NULL, 0);
	else pThis->m_list.m_Repetition.nRepCnt = -1;

	for (I = pThis->m_list.m_Table.m_Lines.begin(); I != pThis->m_list.m_Table.m_Lines.end(); I++)
	{
		GenListCtrl::VERSE verse;
		char* str = new char[7];
		GetWindowTextA(I->edSecond.m_hWnd, str, 7);
		verse.nrSec = atoi(str);
		delete[] str;

		str = new char[100];
		GetWindowTextA(I->edText.m_hWnd, str, 100);
		verse.sVerse = str;
		delete[] str;

		pThis->m_list.m_Repetition.Verses.push_back(verse);
	}

	EndDialog(hDlg, IDOK);
}