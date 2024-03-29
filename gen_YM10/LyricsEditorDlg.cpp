#include "LyricsEditorDlg.h"
#include "LyricsDlg.h"
#include "GenDll.h"
#include "Tools.h"

#include <commctrl.h>

extern CGenDll		genDll;

WNDPROC				OldListBoxProc;

void DeleteLyrics(HWND hList, int nSel);
void OnLBContextMenu(HWND hWnd, int x, int y);

CLyricsEditorDlg::CLyricsEditorDlg(void)
{
	m_hDlg = 0;
}

CLyricsEditorDlg::~CLyricsEditorDlg(void)
{
}

void CLyricsEditorDlg::DoModal(HWND hParent)
{
	if (genDll.m_hLyricsEditorDlg) return;

	INT_PTR nResult = DialogBoxParam(genDll.m_hInstance, MAKEINTRESOURCE(IDD_LYRICS_EDITOR), hParent, &CLyricsEditorDlg::DialogProc, (LPARAM)this);
	if (nResult == -1)
		DisplayError(0);
}

INT_PTR CALLBACK CLyricsEditorDlg::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//intrucat va fi o singura caseta de dialog afisata in acelasi timp, pastram un pointer la obiectul
	//care a creat caseta de dialog
	static CLyricsEditorDlg* pThis = NULL;

	if (pThis == NULL)
		if (uMsg == WM_INITDIALOG)
		{
			pThis = (CLyricsEditorDlg*)lParam;
			pThis->m_hDlg = hDlg;
		}
		else return 0;

	switch (uMsg)
	{
	case WM_DESTROY: pThis = 0; genDll.m_hLyricsEditorDlg = 0; break;
	case WM_CLOSE: EndDialog(hDlg, IDCANCEL); break;
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK: EndDialog(hDlg, IDOK); break;
			case IDCANCEL: EndDialog(hDlg, IDCANCEL); break;
			}
		}
		break;//case WM_COMMAND

	case WM_INITDIALOG: return pThis->OnInitDialog();
	}

	return 0;
}

LRESULT CALLBACK NewListBoxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
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
			OnLBContextMenu(hWnd, LOWORD(lParam), HIWORD(lParam)); return 0;
		}
		break;
	}

	return CallWindowProc(OldListBoxProc, hWnd, uMsg, wParam, lParam);
}

void OnLBContextMenu(HWND hWnd, int x, int y)
{
	//gasim item-ul selectat din listbox:
	int nCount = SendMessage(hWnd, LB_GETCOUNT, 0, 0);
	int nSel = SendMessage(hWnd, LB_GETCURSEL, 0, 0);
	UINT flags = MF_STRING;

	enum items{ID_ADDLYR = 1, ID_EDITLYR, ID_DELETELYR};
	//daca nu exista nici un element in lista, item-ul "Sterge statusul" nu va putea fi folosit
	if (nSel == LB_ERR) flags |= MF_GRAYED;

	//se creeaza meniul
	HMENU hMenu = CreatePopupMenu();
	if (genDll.m_hStatusEditorDlg || nCount >= 128)
		AppendMenu(hMenu, MF_STRING | MF_DISABLED, ID_ADDLYR, L"Adaugă");
	else
		AppendMenu(hMenu, MF_STRING, ID_ADDLYR, L"Adaugă");
	AppendMenu(hMenu, flags, ID_EDITLYR, L"Editează");
	AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(hMenu, flags, ID_DELETELYR, L"Șterge");

	//se afiseaza meniul. in nResult se pastreaza alegerea utilizatorului din meniu. 0, daca nu a fost ales nimic.
	int nResult = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD, x, y, 0, hWnd, 0);
	
	switch (nResult)
	{
	case 0: return;
	case ID_ADDLYR:
		{
			CLyricsDlg dlg;
			dlg.DoModal(GetParent(hWnd));
			return;
		}
	case ID_EDITLYR:
		{
			CLyricsDlg dlg;
			int nFileNr = SendMessage(hWnd, LB_GETITEMDATA, nSel, 0);
			dlg.DoModal(GetParent(hWnd), nFileNr);
			return;
		}
	case ID_DELETELYR:
		{
			DeleteLyrics(hWnd, nSel);
			return;
		}
	}
}

void DeleteLyrics(HWND hList, int nSel)
{
	int nFileNr = SendMessage(hList, LB_GETITEMDATA, nSel, 0);
	SendMessage(hList, LB_DELETESTRING, nSel, 0);

	//se construieste numele intreg al fisierului, cu tot cu cale, din numarul fisierului obtinut
	wstring wsFile = genDll.m_wsAppData;
	wsFile += L"\\";
	WCHAR sNr[7];
	_itow_s(nFileNr, sNr, 7, 10);
	wsFile += sNr;
	wsFile += L".lyr";

	//se sterge din lista de statusuri pastrata in genDll, statusul cu numarul fisierului nFileNr
	deque<LYRICSINFO>::iterator I;
	for (I = genDll.m_LyricsFiles.begin(); I != genDll.m_LyricsFiles.end(); I++)
		if (I->nFileNr == nFileNr)
		{
			genDll.m_LyricsFiles.erase(I);
			break;
		}

	//in final, se sterge fisierul fizic, stocat pe hard disk.
	DeleteFile(wsFile.c_str());
}

int CLyricsEditorDlg::OnInitDialog()
{
	genDll.m_hLyricsEditorDlg = m_hDlg;

	//se creeaza antetul (coloanele)
	HWND hList = GetDlgItem(m_hDlg, IDC_LYRICSLIST);
	
	deque<LYRICSINFO>::iterator I;
	int i;
	for (I = genDll.m_LyricsFiles.begin(); I != genDll.m_LyricsFiles.end(); I++)
	{
		wstring wstr = I->wsArtist;
		wstr += L" - ";
		wstr += I->wsSong;

		//se adauga item-ul si, prin sortare, indicele lui va fi i.
		i = SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)wstr.c_str());
		SendMessage(hList, LB_SETITEMDATA, i, (LPARAM)I->nFileNr);
	}

	HWND hWnd = GetDlgItem(m_hDlg, IDC_STATIC_INFO);
	if (genDll.m_LyricsFiles.size() == 0)
	{
		ShowWindow(hWnd, 1);
		SetWindowText(hWnd, L"Apasă click dreapta in listă pentru a adăuga un lyrics.");
	}
	else
		ShowWindow(hWnd, 0);

	OldListBoxProc = (WNDPROC)SetWindowLongPtrW(hList, GWL_WNDPROC, (LONG)NewListBoxProc);

	return 1;
}