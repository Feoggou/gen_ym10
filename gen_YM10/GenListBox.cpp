#include "GenListBox.h"
#include "StatusEditorDlg.h"
#include "GenDll.h"

extern	CGenDll			genDll;
WNDPROC	GenListBox::m_OldWndProc;

GenListBox::GenListBox(void)
{
}

GenListBox::~GenListBox(void)
{
}

LRESULT CALLBACK GenListBox::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	GenListBox* pThis = (GenListBox*)GetWindowLongPtrW(hWnd, GWL_USERDATA);

	if (pThis)
		switch (uMsg)
		{
		case WM_CONTEXTMENU: 
			{
				//LPARAM contine pozitiile x si y in coordonate ecran
				POINT pt;
				pt.x = LOWORD(lParam);
				pt.y = HIWORD(lParam);
				//LB_ITEMFROMPOINT foloseste coordonate relativ la suprafata client a listbox-ului
				ScreenToClient(hWnd, &pt);
				LRESULT result = SendMessage(hWnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y));

				//HIWORD(result) == 0 daca cursorul este in zona client a unui item; 1 daca nu
				if (0 == HIWORD(result))
					SendMessage(hWnd, LB_SETCURSEL, LOWORD(result), 0);
				else
					SendMessage(hWnd, LB_SETCURSEL, -1, 0);

				//oricum tre sa apelez functia, ca am nevoie oriunde in lista de "Adaugare" de itemi
				pThis->OnContextMenu(LOWORD(lParam), HIWORD(lParam));
				return 0;
			}
		}

	//se apeleaza veche functie de prelucrare a mesajelor, pentru pastrarea functionalitatii listbox-ului
	return CallWindowProc(m_OldWndProc, hWnd, uMsg, wParam, lParam);
}

void GenListBox::Create(HWND hWnd, HWND hParent)
{
	m_hWnd = hWnd;
	m_hParent = hParent;

	//subclasare dinamica
	m_OldWndProc = (WNDPROC)SetWindowLongPtrW(hWnd, GWL_WNDPROC, (LONG)WindowProc);
	SetWindowLongPtrW(m_hWnd, GWL_USERDATA, (LONG)this);
}

void GenListBox::OnContextMenu(int x, int y)
{
	//gasim item-ul selectat din listbox:
	int nCount = SendMessage(m_hWnd, LB_GETCOUNT, 0, 0);
	int nSel = SendMessage(m_hWnd, LB_GETCURSEL, 0, 0);
	UINT flags = MF_STRING;

	//daca nu exista nici un element in lista, item-ul "Sterge statusul" nu va putea fi folosit
	if (nSel == LB_ERR) flags |= MF_GRAYED;

	//se creeaza meniul
	HMENU hMenu = CreatePopupMenu();
	if (genDll.m_hStatusEditorDlg || nCount >= 128)
		AppendMenu(hMenu, MF_STRING | MF_DISABLED, ID_ADDSTATUSMSG, L"Adaugă");
	else
		AppendMenu(hMenu, MF_STRING, ID_ADDSTATUSMSG, L"Adaugă");
	AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(hMenu, flags, ID_DELETESTATUSMSG, L"Șterge Statusul");

	//se afiseaza meniul. in nResult se pastreaza alegerea utilizatorului din meniu. 0, daca nu a fost ales nimic.
	int nResult = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD, x, y, 0, m_hWnd, 0);
	
	switch (nResult)
	{
	case 0: return;
	case ID_ADDSTATUSMSG:
		{
			CStatusEditorDlg stedDlg;
			stedDlg.DoModal(GetParent(m_hWnd));
			return;
		}
	case ID_DELETESTATUSMSG:
		{
			OnDeleteString(nSel);
			return;
		}
	}
}

void GenListBox::OnDeleteString(int nSel)
{
	//se pastreaza data item-ului care a fost stocat si care reprezinta numarul fisierului de pe hard disk
	int nFileNr = SendMessage(m_hWnd, LB_GETITEMDATA, nSel, 0);

	if (nSel == genDll.m_dStatusSelected)
	{
		genDll.m_dStatusSelected = -1;
	}
	else if (nSel < genDll.m_dStatusSelected)
		genDll.m_dStatusSelected--;

	//se sterge din listbox item-ul nSel
	SendMessage(m_hWnd, LB_DELETESTRING, nSel, 0);
	//se construieste numele intreg al fisierului, cu tot cu cale, din numarul fisierului obtinut
	wstring wsFile = genDll.m_wsAppData;
	wsFile += L"\\";
	WCHAR sNr[7];
	_itow_s(nFileNr, sNr, 7, 10);
	wsFile += sNr;
	wsFile += L".stt";

	//se sterge din lista de statusuri pastrata in genDll, statusul cu numarul fisierului nFileNr
	deque<STATUS>::iterator I;
	for (I = genDll.m_StatusMsgs.begin(); I != genDll.m_StatusMsgs.end(); I++)
		if (I->dFileNr == nFileNr)
		{
			genDll.m_StatusMsgs.erase(I);
			break;
		}

	//in final, se sterge fisierul fizic, stocat pe hard disk.
	DeleteFile(wsFile.c_str());
}