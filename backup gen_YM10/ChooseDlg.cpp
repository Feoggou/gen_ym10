#include "GenDll.h"
#include "ChooseDlg.h"
#include "resource.h"
#include "AboutDlg.h"

extern CGenDll			genDll;

CChooseDlg::CChooseDlg(void)
{
	m_hArrowCursor = m_hHandCursor = 0;
	m_bClicked = m_bMoved = FALSE;
	m_bStatic = true;
}

CChooseDlg::~CChooseDlg(void)
{
}

int CChooseDlg::DoModal(HWND hParent)
{
	INT_PTR nResult = DialogBoxParam(genDll.m_hInstance, MAKEINTRESOURCE(IDD_DIALOG_CHOOSE), hParent, DialogProc, (LPARAM)this);
	genDll.m_bChooseDlgCreated = false;
	genDll.m_hChooseDlg = 0;
	return nResult;
}

INT_PTR CALLBACK CChooseDlg::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE: EndDialog(hDlg, IDCANCEL); break;
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK: OnOk(hDlg); break;
			case IDCANCEL:EndDialog(hDlg, IDCANCEL); break;
			}
		}
		break;//case WM_COMMAND
	case WM_INITDIALOG: OnInitDialog(hDlg, wParam, lParam);break;
	case WM_LBUTTONUP: OnLButtonUp(hDlg, wParam, lParam); break;
	case WM_MOUSEMOVE: OnMouseMove(hDlg, wParam, lParam); break;

	case WM_UPDATELIST:
		{
			int nLast = SendMessage(GetDlgItem(hDlg, IDC_LIST_OLDMESSAGES), LB_GETCOUNT, 0, 0);
			SendMessage(GetDlgItem(hDlg, IDC_LIST_OLDMESSAGES), LB_ADDSTRING, 0, lParam);
			SendMessage(GetDlgItem(hDlg, IDC_LIST_OLDMESSAGES), LB_SETITEMDATA, nLast, (LPARAM)genDll.m_StatusMsgs[nLast].dFileNr);
		}
		break;//WM_UPDATELIST

	case WM_UPDATEIDS: OnUpdateIDs(hDlg); break;
	}

	return 0;
}

int CChooseDlg::OnInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	genDll.m_bChooseDlgCreated = true;
	genDll.m_hChooseDlg = hDlg;
	UNREFERENCED_PARAMETER(wParam);
	//LPARAM
	CChooseDlg*	pThis = (CChooseDlg*)lParam;

	//HYPERLINK
	pThis->m_hHandCursor = LoadCursor(0, MAKEINTRESOURCE(IDC_HAND)); 
	pThis->m_hArrowCursor = LoadCursor(0, MAKEINTRESOURCE(IDC_ARROW)); 
	pThis->m_bClicked = 0;
	pThis->m_bMoved = 0;

	SetClassLong(hDlg, GCL_HCURSOR, NULL);

	//setting data for dialog
	SetWindowLongPtrW(hDlg, GWL_USERDATA, (LONG_PTR)lParam);
	//setting the win proc for the About status (->link)
	pThis->m_AboutLink.m_hWnd = GetDlgItem(hDlg, IDC_ABOUTTOOL);
	pThis->m_IDLink.m_hWnd = GetDlgItem(hDlg, IDC_MESSID);
	CHyperLink::OldWndProc = (WNDPROC)SetWindowLongPtrW(GetDlgItem(hDlg, IDC_ABOUTTOOL), GWL_WNDPROC, (LONG)CHyperLink::WindowProc);

	//LISTBOX
	pThis->m_ListBox.Create(GetDlgItem(hDlg, IDC_LIST_OLDMESSAGES), hDlg);

	//initializing the dialog from the registry values:
	SendMessage(GetDlgItem(hDlg, IDC_ENABLECHANGE), BM_SETCHECK, genDll.GetValueUseTool(),0);
	SendMessage(GetDlgItem(hDlg, IDC_SHOWTRAY), BM_SETCHECK, genDll.GetValueShowTray(),0);
	BYTE nr = genDll.GetValueNrStatusMsgs();

	//initializing the listbox from statuses:
	deque<STATUS>::iterator I;
	int i = 0;
	for (I = genDll.m_StatusMsgs.begin(); I != genDll.m_StatusMsgs.end(); I++, i++)
	{
		SendMessage(GetDlgItem(hDlg, IDC_LIST_OLDMESSAGES), LB_ADDSTRING, 0, (LPARAM)I->swText.c_str());
		SendMessage(GetDlgItem(hDlg, IDC_LIST_OLDMESSAGES), LB_SETITEMDATA, i, (LPARAM)I->dFileNr);
	}

	SendMessage(GetDlgItem(hDlg, IDC_LIST_OLDMESSAGES), LB_SETCURSEL, genDll.m_dStatusSelected, 0);

	OnUpdateIDs(hDlg);

	return 0;
}

void CChooseDlg::OnLButtonUp(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	CChooseDlg* pThis = (CChooseDlg*)GetWindowLongPtrW(hDlg, GWL_USERDATA);

	int xPos = (int)LOWORD(lParam);
	int yPos = (int)HIWORD(lParam); 

	RECT rect;
	//about link:
	GetWindowRect(pThis->m_AboutLink.m_hWnd, &rect);
	POINT first = {rect.left, rect.top}, second = {rect.right, rect.bottom};

	ScreenToClient(hDlg, &first);
	ScreenToClient(hDlg, &second);
	rect = MAKERECT(first.x, first.y, second.x, second.y);

	if ( xPos > rect.left && xPos < rect.right && yPos < rect.bottom && yPos > rect.top)
	{
		SetCursor(pThis->m_hHandCursor);
		pThis->m_bClicked = TRUE;
		InvalidateRect(hDlg, &rect, 1);

		CAboutDlg aboutDlg;
		aboutDlg.DoModal(hDlg);
	}
	else
	{
		if (pThis->m_bStatic == true) return;
		//ID link
		GetWindowRect(pThis->m_IDLink.m_hWnd, &rect);
		RECT rect2;
		GetWindowRect(pThis->m_IDLink.m_hWnd, &rect2);
		POINT first = {rect.left, rect.top}, second = {rect.right, rect.bottom};

		ScreenToClient(hDlg, &first);
		ScreenToClient(hDlg, &second);
		rect = MAKERECT(first.x, first.y, second.x, second.y);

		if ( xPos > rect.left && xPos < rect.right && yPos < rect.bottom && yPos > rect.top)
			SetCursor(pThis->m_hHandCursor);
		else return;

		//creating the popup menu, with menuitems: No ID, ids...
		if (genDll.m_OnlineIDs.size() <= 1)
		{
#ifdef _DEBUG
			MessageBox(0, L"This should have not appeared", 0, 0);
			DebugBreak();
#endif
			return;
		}

		HMENU hMenu = CreatePopupMenu();
		
		deque<ID>::iterator I;
		int i = 0;
		for (I = genDll.m_OnlineIDs.begin(); I != genDll.m_OnlineIDs.end(); I++, i++)
			AppendMenu(hMenu, MF_STRING, i + 2, I->swID.c_str());
		
		int nResult = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD, rect2.left, rect2.bottom, 0, hDlg, 0);
		if (nResult == 0) return;
		if (nResult == 1)
		{
			genDll.m_SelectedID.swID = L"";
			genDll.m_SelectedID.hWnd = NULL;
		}
		if (nResult >= 2)
		{
			genDll.m_SelectedID.swID = genDll.m_OnlineIDs[nResult - 2].swID;
			genDll.m_SelectedID.hWnd = genDll.m_OnlineIDs[nResult - 2].hWnd;
		}
		//OnUpdateIDs(hDlg);
		RedrawWindow(pThis->m_IDLink.m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
//		InvalidateRect(hDlg, NULL, true);
//		UpdateWindow(hDlg);
	}
}

void CChooseDlg::OnMouseMove(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	CChooseDlg* pThis = (CChooseDlg*)GetWindowLongPtrW(hDlg, GWL_USERDATA);

	int xPos = (int)LOWORD(lParam);
	int yPos = (int)HIWORD(lParam); 

	RECT rect;
	//about link
	GetWindowRect(pThis->m_AboutLink.m_hWnd, &rect);
	POINT first = {rect.left, rect.top}, second = {rect.right, rect.bottom};

	ScreenToClient(hDlg, &first);
	ScreenToClient(hDlg, &second);
	rect = MAKERECT(first.x, first.y, second.x, second.y);

	if ( xPos > first.x && xPos < second.x && yPos < second.y && yPos > first.y)
	{
		SetCursor(pThis->m_hHandCursor);
		if (pThis->m_bMoved == FALSE) InvalidateRect(hDlg, &rect, 1);
		pThis->m_bMoved = TRUE;
	}
	else 
	{
		if (pThis->m_bStatic) goto no_link;
		//ID link
		GetWindowRect(pThis->m_IDLink.m_hWnd, &rect);
		POINT first = {rect.left, rect.top}, second = {rect.right, rect.bottom};

		ScreenToClient(hDlg, &first);
		ScreenToClient(hDlg, &second);
		rect = MAKERECT(first.x, first.y, second.x, second.y);

		if ( xPos > first.x && xPos < second.x && yPos < second.y && yPos > first.y)
		{
			SetCursor(pThis->m_hHandCursor);
			if (pThis->m_bMoved == FALSE) InvalidateRect(hDlg, &rect, 1);
			pThis->m_bMoved = TRUE;
		}
		else
		{
			//no link
no_link:	SetCursor(pThis->m_hArrowCursor);
			if (pThis->m_bMoved == TRUE) InvalidateRect(hDlg, &rect, 1);
			pThis->m_bMoved = FALSE;
		}
	}
}

void CChooseDlg::OnOk(HWND hDlg)
{
	genDll.m_bUseTool = SendMessage(GetDlgItem(hDlg, IDC_ENABLECHANGE), BM_GETCHECK, 0, 0);
	genDll.m_bShowTray = SendMessage(GetDlgItem(hDlg, IDC_SHOWTRAY), BM_GETCHECK, 0, 0);
	genDll.m_dStatusSelected = SendMessage(GetDlgItem(hDlg, IDC_LIST_OLDMESSAGES), LB_GETCURSEL, 0, 0);

	if (genDll.m_bUseTool && genDll.m_dStatusSelected == -1)
	{
		int nCount = SendMessage(GetDlgItem(hDlg, IDC_LIST_OLDMESSAGES), LB_GETCOUNT, 0, 0);
		if (nCount > 0)
			MessageBoxW(genDll.m_hWinamp, L"Pentru a putea utiliza acest plugin pentru a schimba statusul la Yahoo! Messenger trebuie să ai un status selectat.", L"Atenție!", MB_ICONWARNING);
		else
			MessageBoxW(genDll.m_hWinamp, L"Pentru a putea utiliza acest plugin pentru a schimba statusul la Yahoo! Messenger trebuie să ai un status selectat.\
\nApasă click-dreapta în listă și alege \"Adaugă\" pentru a adăuga un status.", L"Atenție!", MB_ICONWARNING);
		return;
	}

	int nLen = GetWindowTextLength(GetDlgItem(hDlg, IDC_MESSID));
	nLen++;
	wchar_t* wsir = new wchar_t[nLen];
	GetDlgItemText(hDlg, IDC_MESSID, wsir, nLen);
	CChooseDlg* pThis = (CChooseDlg*)GetWindowLongPtrW(hDlg, GWL_USERDATA);
	genDll.m_SelectedID.swID = wsir;
	//the real ids dont have ' '. so, if ' ' is found on a valid position (not npos) then we have "" as selected str.
	if (genDll.m_SelectedID.swID.find(' ', 0) != string::npos)
		genDll.m_SelectedID.swID = L"";

	genDll.m_nTime = -1;

	delete[] wsir;
	EndDialog(hDlg, IDOK);
}

void CChooseDlg::OnUpdateIDs(HWND hDlg)
{
	HWND hWnd = GetDlgItem(hDlg, IDC_MESSID);
	CChooseDlg* pThis = (CChooseDlg*)GetWindowLongPtrW(hDlg, GWL_USERDATA);
	int size = genDll.m_OnlineIDs.size();
	int nLen = GetWindowTextLength(hWnd);
	nLen++;
	wchar_t* wstr = new wchar_t[nLen];
	GetWindowText(hWnd, wstr, nLen);
	genDll.m_bSelOnline = false;
	for (int i = 0; i < size; i++)
		if (wcscmp(genDll.m_OnlineIDs[i].swID.c_str(), genDll.m_SelectedID.swID.c_str()) == 0)
		{
			genDll.m_bSelOnline = true;
			genDll.m_SelectedID.hWnd = genDll.m_OnlineIDs[i].hWnd;
		}

	if (genDll.m_SelectedID.swID.length() > 0)
	{			
		if (wcscmp(wstr, genDll.m_SelectedID.swID.c_str()) != 0)
		{
//			string sID;
//			for (UINT counter = 0; counter < genDll.m_SelectedID.swID.length(); counter++)
//			{
//				sID += genDll.m_SelectedID.swID[counter];
//			}

			if (genDll.m_bSelOnline)
				SetDlgItemTextW(hDlg, IDC_MESSID, genDll.m_SelectedID.swID.data());
			else 
			{
				wstring asta = genDll.m_SelectedID.swID;
				asta += L" (Nu e online)";
				SetDlgItemTextW(hDlg, IDC_MESSID, asta.c_str());
			}
		}
		if (pThis->m_bStatic == true && size > 1)
		{
			SetWindowLongPtrW(hWnd, GWL_WNDPROC, (LONG_PTR)CHyperLink::WindowProc);
			pThis->m_bStatic = false;
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
		}
		delete[] wstr;
		return;
	}

	if (genDll.m_SelectedID.swID.length() == 0)
	{
		if (size <= 1)
		{
			if (wcscmp(wstr, L"Nici un ID (nu ești logat pe Yahoo! Messenger)") != 0)
				SetDlgItemTextW(hDlg, IDC_MESSID, L"Nici un ID (nu ești logat pe Yahoo! Messenger)");
			if (pThis->m_bStatic == false)
			{
				SetWindowLongPtrW(hWnd, GWL_WNDPROC, (LONG_PTR)CHyperLink::OldWndProc);
				pThis->m_bStatic = true;
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
			}
		}
		else
		{
			if (wcscmp(wstr, L"Nici un ID (Apasă click pentru a schimba)") != 0)
				SetDlgItemTextW(hDlg, IDC_MESSID, L"Nici un ID (Apasă click pentru a schimba)");
			if (pThis->m_bStatic == true)
			{
				SetWindowLongPtrW(hWnd, GWL_WNDPROC, (LONG_PTR)CHyperLink::WindowProc);
				pThis->m_bStatic = false;
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
			}
		}
	}
	delete[] wstr;
}