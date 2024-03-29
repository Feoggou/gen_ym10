#include "GenListBox.h"
#include "StatusEditorDlg.h"
#include "GenDll.h"

extern CGenDll genDll;
WNDPROC GenListBox::m_OldWndProc;

GenListBox::GenListBox(void)
{
}

GenListBox::~GenListBox(void)
{
}

LRESULT CALLBACK GenListBox::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CONTEXTMENU: OnContextMenu(hWnd, wParam, lParam); return 0;
	}

	return CallWindowProc(m_OldWndProc, hWnd, uMsg, wParam, lParam);
}

void GenListBox::Create(HWND hWnd, HWND hParent)
{
	m_hWnd = hWnd;
	m_hParent = hParent;

	m_OldWndProc = (WNDPROC)SetWindowLongPtrW(hWnd, GWL_WNDPROC, (LONG)WindowProc);
}

void GenListBox::OnContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	int x = LOWORD(lParam);
	int y = HIWORD(lParam);

	//getting the selected item in the listbox:
	int nSel = SendMessage(hWnd, LB_GETCURSEL, 0, 0);
	UINT flags = MF_STRING;

	if (nSel == LB_ERR) flags |= MF_GRAYED;

	HMENU hMenu = CreatePopupMenu();
	if (genDll.m_bStatusEdtCreated)
		AppendMenu(hMenu, MF_STRING | MF_DISABLED, ID_ADDSTATUSMSG, L"Adaugă");
	else
		AppendMenu(hMenu, MF_STRING, ID_ADDSTATUSMSG, L"Adaugă");
	AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(hMenu, flags, ID_DELETESTATUSMSG, L"Șterge Statusul");

	int nResult = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD, x, y, 0, hWnd, 0);
	
	switch (nResult)
	{
	case 0: return;
	case ID_ADDSTATUSMSG:
		{
			CStatusEditorDlg stedDlg;
			stedDlg.DoModal(GetParent(hWnd));
			return;
		}
	case ID_DELETESTATUSMSG:
		{
			OnDeleteString(hWnd, nSel);
			return;
		}
	}
}

void GenListBox::OnDeleteString(HWND hWnd, int nSel)
{
	int nFileNr = SendMessage(hWnd, LB_GETITEMDATA, nSel, 0);

	SendMessage(hWnd, LB_DELETESTRING, nSel, 0);
	wstring swFile = genDll.m_swAppData;
	swFile += L"\\";
	WCHAR sNr[7];
	_itow_s(nFileNr, sNr, 7, 10);
	swFile += sNr;
	swFile += L".stt";

	deque<STATUS>::iterator I;
	for (I = genDll.m_StatusMsgs.begin(); I != genDll.m_StatusMsgs.end(); I++)
		if (I->dFileNr == nFileNr)
		{
			genDll.m_StatusMsgs.erase(I);
			break;
		}

	DeleteFile(swFile.c_str());
}