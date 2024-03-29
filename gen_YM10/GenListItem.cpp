#include <crtdbg.h>

#include "GenListItem.h"
#include "GenDll.h"
#include "ListSide.h"

extern CGenDll genDll;

WNDPROC GenListItem::OldWndProc;



GenListItem::GenListItem(void)
{

}

GenListItem::~GenListItem(void)
{

}

void GenListItem::Create(HWND hParent, RECT& rect, bool bText, bool bIsLyrics)
{
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_BORDER;
	WCHAR wstr[7] = L"";

	if (bText)
		dwStyle |= ES_AUTOHSCROLL;
	else 
	{
		//daca este secunda, in Lyrics formatul e m:ss
		if (bIsLyrics)
		{
			wcscpy_s(wstr, 7, L"0:00");
			dwStyle |= SS_CENTER;
		}
		else 
		{
			//daca este repetitie, exista fie secunde fie minute
			wcscpy_s(wstr, 2, L"0");
			dwStyle |= ES_RIGHT;
		}
	}

	//daca item-ul este folosit pentru caseta de dialog de versuri(lyrics) si reprezinta timpul (secunda)
	//atunci va fi control static
	if (bIsLyrics && !bText)
		m_hWnd = CreateWindowW(L"STATIC", wstr, dwStyle, rect.left, rect.top, rect.right - rect.left + 1, rect.bottom - rect.top + 1, hParent, NULL, 
		genDll.m_hInstance, NULL);
	//altfel va fi caseta de editare
	else
	m_hWnd = CreateWindowW(L"EDIT", wstr, dwStyle, rect.left, rect.top, rect.right - rect.left + 1, rect.bottom - rect.top + 1, hParent, NULL, 
		genDll.m_hInstance, NULL);

	//daca este Secunda/Minut in repetitie, limitam textul ce poate scris de utilizator la 3 caractere (aici, cifre)
	if (!bIsLyrics)
		if (!bText) SendMessage(m_hWnd, EM_LIMITTEXT, 3, 0);
		//daca este Text, tot in repetitie, limitam la 256
		else SendMessage(m_hWnd, EM_LIMITTEXT, 256, 0);

	//pastram ca data asociata ferestrei, bText
	SetWindowLongPtrW(m_hWnd, GWL_USERDATA, bText);
	//setam fontul controlului ca fiind fontul folosit pentru interfata cu utilizatorul (meniuri, caseta de dialog)
	SendMessage(m_hWnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 1);

	//se seteaza functia de prelucrare a imaginilor, in cazurile in kre este fie control static, fie caseta de editare
	if (bIsLyrics && !bText)
	{
		//daca este control static, apoi redesenam controlul (folosind functia noastra de desenare)
		OldWndProc = (WNDPROC)SetWindowLongPtrW(m_hWnd, GWL_WNDPROC, (LONG)StaticProc);
		RedrawWindow(m_hWnd, 0, 0, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
	}
	//daca este caseta de editare...
	else
		OldWndProc = (WNDPROC)SetWindowLongPtrW(m_hWnd, GWL_WNDPROC, (LONG_PTR)EditProc);
}

LRESULT CALLBACK GenListItem::EditProc(HWND hEdit, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CListSide* pParent = (CListSide*)GetWindowLongPtrW(GetParent(hEdit), GWL_USERDATA);
	switch (uMsg)
	{
	case WM_KEYDOWN:
		//daca este apasat butonul "Delete"
		if (wParam==VK_DELETE)
		{
			//cautam in lista m_Lines al obiectului de clasa CListSide itemul cu hWnd = hEdit
			bool bFound = false;
			deque<CListSide::LINE>::iterator I;
			for (I = pParent->m_Lines.begin(); I != pParent->m_Lines.end(); I++)
			{
				if (I->edText.m_hWnd == hEdit)
				{
					bFound = true;
					break;
				}
			}

			//bFound trebuie sa fie mereu TRUE. daca nu este, atunci e edSecond in repetitie
			if (bFound)
			{
				//gasim ce a fost selectat in caseta de editare cand s-a apasat tasta delete,
				//sau unde anume era pozitionat caret-ul
				int start, end;
				SendMessage(hEdit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
				int len = GetWindowTextLengthW(hEdit);
 			
				//daca avem selectie, nu se sterge nici o linie
				if (end != start) break;
				//daca caret-ul este la sfarsitul textului
				//iar linia este cel mult penultima (ultima = size-1)
				if (end == len && I->nIndex < (int)pParent->m_Lines.size() - 1)
				{
					//atunci stergem linia urmatoare si copiem textul dupa caret
					SendMessage(GetParent(hEdit), WM_DELLINEAT_DEL, (WPARAM)I->nIndex + 1, (LPARAM)pParent->m_Lines[I->nIndex + 1].edText.m_hWnd);
					return 0;
				}
			}
		}

		//daca este apasat butonul Backspace
		else if (wParam==VK_BACK)
		{
			//cautam in lista m_Lines al obiectului de clasa CListSide itemul cu hWnd = hEdit
			bool bFound = false;
			deque<CListSide::LINE>::iterator I;
			for (I = pParent->m_Lines.begin(); I != pParent->m_Lines.end(); I++)
			{
				if (I->edText.m_hWnd == hEdit)
				{
					bFound = true;
					break;
				}
			}

			//bFound trebuie sa fie mereu TRUE. daca nu este, atunci e edSecond in repetitie
			if (bFound)
			{
				int start, end;

				//gasim ce text a fost selectat; daca start = end, unde este pozitionat caret-ul
				SendMessage(hEdit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
 			
				//daca caret-ul este pe pozitia 0, si nu suntem pe prima linie de tabel
				if (end==0 && I->nIndex > 0)
				{
					//atunci copiem textul din linia aceasta in caseta de editare de mai sus
					//si stergem linia aceasta
					SendMessage(GetParent(hEdit), WM_DELLINEAT_BACK, (WPARAM)I->nIndex, (LPARAM)hEdit);
					return 0;
				}
			}
		}

		//daca se apasa tasta Enter
		else if (wParam == VK_RETURN)
		{
			//cautam in lista m_Lines al obiectului de clasa CListSide itemul cu hWnd = hEdit
			bool bFound = false;
			deque<CListSide::LINE>::iterator I;
			for (I = pParent->m_Lines.begin(); I != pParent->m_Lines.end(); I++)
			{
				if (I->edText.m_hWnd == hEdit)
				{
					bFound = true;
					break;
				}
			}

			//bFound trebuie sa fie mereu TRUE. daca nu este, atunci e edSecond in repetitie
			if (bFound)
			{
				int start, end;

				//gasim ce text a fost selectat; daca start = end, unde este pozitionat caret-ul
				SendMessage(hEdit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
 			
				//se adauga o linie in tabel, loword = index-ul liniei, hiword = pozitia caret-ului
				//hEdit = caseta de editare in care s-a apasat Enter.
				SendMessage(GetParent(hEdit), WM_ADDLINEAT, MAKEWPARAM(I->nIndex, end), (LPARAM)hEdit);
				return 0;
			}
		}

		//daca s-a apasat tasta TAB
		else if (wParam == VK_TAB)
		{
			//daca suntem in CLyricsDlg, TAB nu ar trebui sa aibe efect
			//Parent(hEdit) = ListSide
			//parent(ListSide) = GenListCtrl
			//parent(ListCtrl) = dialogbox
			//CLyricsDlg are un control IDC_SET
			if (0 != GetDlgItem(GetParent(GetParent(GetParent(hEdit))), IDC_SET)) break;

			//gasim urmatoarea fereastra
			HWND hNext = GetNextWindow(hEdit, GW_HWNDNEXT);
			if (hNext)
				//aceea primeste focus-ul, daca exista
				SetFocus(hNext);
			//daca nu exista o urmatoarea, atunci urmatorul control dupa GenListCtrl primeste focusul.
			else SetFocus(GetNextWindow(GetParent(GetParent(hEdit)), GW_HWNDNEXT));
		}

		//nu trimitem VK_HOME si VK_END la CListSide
		if (wParam != VK_HOME && wParam != VK_END)
			SendMessage(GetParent(hEdit), WM_KEYDOWN, wParam, lParam);
		//decat daca este apasata tasta Ctrl.
		else if (GetKeyState(VK_CONTROL)) SendMessage(GetParent(hEdit), WM_KEYDOWN, wParam, lParam);
		break;//WM_KEYDOWN

	case WM_LBUTTONDBLCLK:
		{
			if (!pParent->m_bIsLyrics) break;

			//cautam in lista m_Lines al obiectului de clasa CListSide itemul cu hWnd = hEdit
			bool bFound = false;
			deque<CListSide::LINE>::iterator I;
			for (I = pParent->m_Lines.begin(); I != pParent->m_Lines.end(); I++)
			{
				if (I->edText.m_hWnd == hEdit || I->edSecond.m_hWnd == hEdit)
				{
					bFound = true;
					break;
				}
			}

			//bFound trebuie sa fie mereu TRUE (adica, sa fie gasit hEdit in m_Lines)
			_ASSERT(bFound);

			if (bFound)
			{
				//selectam intreaga linie de tabel
				SendMessage(GetParent(hEdit), WM_SELECTLINE, (WPARAM)I->nIndex, (LPARAM)hEdit);
			}
			
			return 0;
		}
		break;//WM_LBUTTONDBLCLK

	case WM_KILLFOCUS:
		{
			//doar daca este secunda/minut
			bool bText = (0 != GetWindowLongPtrW(hEdit, GWL_USERDATA));
			if (bText) break;

			WCHAR wstr[7];
			GetWindowTextW(hEdit, wstr, 7);
			if (wcslen(wstr) == 0)
				SetWindowTextW(hEdit, L"0");
		}
		break;//WM_KILLFOCUS

	case WM_CONTEXTMENU:
		{
			//hEdit, x, y
			OnEditContextMenu(HWND(wParam), LOWORD(lParam), HIWORD(lParam));
			return 0;
		}
		break;//WM_CONTEXTMENU

	case WM_SETFOCUS:
	case WM_LBUTTONUP:
		{
			bool bText = (0 != GetWindowLongPtrW(hEdit, GWL_USERDATA));
			if (bText) break;

			//selectam tot textul
			SendMessage(hEdit, EM_SETSEL, 0, 7);
		}
		break;//WM_SETFOCUS

	case WM_CHAR:
		{
			if (pParent->m_bIsLyrics) break;
			bool bText = (0 != GetWindowLongPtrW(hEdit, GWL_USERDATA));
			//daca e secunda/minut si nu suntem in lyrics, daca ceea ce a fost apasat nu e cifra nu se afiseaza
			if (!bText && !(isdigit(wParam) || iscntrl(wParam))) return 0;
		}
		break;//WM_CHAR
	}

	return CallWindowProc(OldWndProc, hEdit, uMsg, wParam, lParam);
}

LRESULT CALLBACK GenListItem::StaticProc(HWND hStatic, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	//setam textul controlului static
	case WM_SETTEXT: 
		{
			LRESULT nResult = DefWindowProc(hStatic, uMsg, wParam, lParam);
			//se redeseneaza, folosind functia OnPaintStatic
			RedrawWindow(hStatic, 0, 0, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
			return nResult;
		}

	case WM_PAINT: OnPaintStatic(hStatic); return 0;
	case WM_LBUTTONDOWN: OnLButtonDown(hStatic, wParam); break;
	case WM_CONTEXTMENU:
		{
			//hEdit, x, y
			OnStaticContextMenu(HWND(wParam), LOWORD(lParam), HIWORD(lParam));
			return 0;
		}
		break;//WM_CONTEXTMENU
	}

	return DefWindowProc(hStatic, uMsg, wParam, lParam);
}

void GenListItem::OnEditContextMenu(HWND hEdit, WORD x, WORD y)
{
	enum mr {ID_CANCELED, ID_UNDO, ID_CUT, ID_COPY, ID_PASTE, ID_DELETE, ID_SELECTLINE, ID_ADDLINE, ID_REMOVELINE};
	mr result = ID_CANCELED;

	CListSide* pParent = (CListSide*)GetWindowLongPtrW(GetParent(hEdit), GWL_USERDATA);
	int nSel = pParent->GetSelectedLine();

	//se creeaza meniul popul
	HMENU hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, ID_UNDO, L"&Anulează");
	AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(hMenu, MF_STRING, ID_CUT, L"&Decupează");
	AppendMenu(hMenu, MF_STRING, ID_COPY, L"&Copiază");
	AppendMenu(hMenu, MF_STRING, ID_PASTE, L"&Lipește");
	AppendMenu(hMenu, MF_STRING, ID_DELETE, L"&Șterge");
	AppendMenu(hMenu, MF_SEPARATOR, 0, 0);

	if (pParent->m_bIsLyrics)
	{
		//daca aceasta este linia selectata
		if (nSel > -1 && pParent->m_Lines[nSel].edText.m_hWnd == hEdit)
			AppendMenu(hMenu, MF_STRING, ID_SELECTLINE, L"De&selectează Linia");
		else
			//daca linia aceasta nu este selectata
			AppendMenu(hMenu, MF_STRING, ID_SELECTLINE, L"&Selectează Linia");
	}


	AppendMenu(hMenu, MF_STRING, ID_ADDLINE, L"&Adaugă Linie");
	AppendMenu(hMenu, MF_STRING, ID_REMOVELINE, L"&Elimină Linie");

	//afisam meniul. returneaza alegerea facuta in result
	result = (mr)TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON, x, y, 0, hEdit, 0);

	//gasim hEdit in lista m_Lines al CListSide
	bool bFound = false;
	deque<CListSide::LINE>::iterator I;
	for (I = pParent->m_Lines.begin(); I != pParent->m_Lines.end(); I++)
	{
		if (I->edText.m_hWnd == hEdit || I->edSecond.m_hWnd == hEdit)
		{
			bFound = true;
			break;
		}
	}

	//bFound trebuie sa fie mereu TRUE (adica, sa fie gasit hEdit in m_Lines)
	_ASSERT(bFound);

	switch (result)
	{
	case ID_CANCELED: return;
	case ID_UNDO: SendMessage(hEdit, EM_UNDO, 0, 0); break;
	case ID_CUT: SendMessage(hEdit, WM_CUT, 0, 0); break;
	case ID_COPY: SendMessage(hEdit, WM_COPY, 0, 0); break;
	case ID_PASTE: SendMessage(hEdit, WM_PASTE, 0, 0); break;
	case ID_DELETE: SendMessage(hEdit, WM_KEYDOWN, VK_DELETE, 0); break;
	case ID_SELECTLINE: 
		if (bFound) SendMessage(GetParent(hEdit), WM_SELECTLINE, I->nIndex, (LPARAM)hEdit); break;
	case ID_ADDLINE:
		if (bFound)
		{
			int len = GetWindowTextLength(hEdit);
			//se muta caret-ul la sfarsitul textului
			SendMessage(hEdit, EM_SETSEL, len, len);
			//se adauga o linie goala dedesupt
			SendMessage(GetParent(hEdit), WM_ADDLINEAT, MAKEWPARAM(I->nIndex, len), (LPARAM)hEdit);
		}
		break;
	case ID_REMOVELINE:
		if (bFound)
		{
			//stergem continutul
			SetWindowTextW(hEdit, L"");
			//mutam caret-ul pe prima pozitie
			SendMessage(hEdit, EM_SETSEL, 0, 0);
			//apelam WM_DELLINEAT_BACK
			SendMessage(GetParent(hEdit), WM_DELLINEAT_BACK, (WPARAM)I->nIndex, (LPARAM)hEdit);
		}
		break;
	}
}

void GenListItem::OnPaintStatic(HWND hStatic)
{
	bool bFound = false;
	CListSide* pParent = (CListSide*)GetWindowLongPtrW(GetParent(hStatic), GWL_USERDATA);

	//gasim hStatic in lista m_Lines
	deque<CListSide::LINE>::iterator I;
	for (I = pParent->m_Lines.begin(); I != pParent->m_Lines.end(); I ++)
	{
		if (I->edSecond.m_hWnd == hStatic)
		{
			bFound = true;
			break;
		}
	}

	//bFound trebuie sa fie TRUE (adica, sa fie gasit hEdit in m_Lines. daca de-abia s-a schimbat wndproc nu este)
	if (!bFound) return;

	//initializari pentru desenare
	PAINTSTRUCT ps;
	BeginPaint(hStatic, &ps);

	//e nevoie de inca un DC pentru folosirea bitamp-ului
	HDC hMemDC = CreateCompatibleDC(ps.hdc);
	RECT rect;
	GetClientRect(hStatic, &rect);

	HBITMAP hBmpStatic;
	
	//fist == -1 => last == -1 (daca primul nu e selectat, nici ultimul nu e selectat)
	if (pParent->m_nFirstIndex == -1)
		hBmpStatic = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_STATIC));
	//first > -1, last == -1
	else if (pParent->m_nLastIndex == -1)
	{
		//daca controlul static gasit este primul selectat, il desenam ca selectat (IDB_STATICSEL)
		if (I->nIndex == pParent->m_nFirstIndex)
			hBmpStatic = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_STATICSEL));
		//altfel il desenam normal
		else
			hBmpStatic = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_STATIC));
	}
	//first >-1, last >-1, first < last
	//adica daca avem mai multe selectate, in care, primul care a fost selectat a fost cel de mai sus
	else if (pParent->m_nFirstIndex < pParent->m_nLastIndex)
	{
		//daca itemul nostru pentru care se face desenarea este intre primul si ultimul, va fi 'selectat'
		if (I->nIndex >= pParent->m_nFirstIndex && I->nIndex <= pParent->m_nLastIndex)
			hBmpStatic = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_STATICSEL));
		//altfel va fi normal
		else hBmpStatic = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_STATIC));
	}
	//first >-1 last > -1, first > last (nu exista first = last decat daca first = last = -1)
	//primul selectat a fost cel de jos
	else
	{
		//daca itemul nostru pentru care se face desenarea este intre ultimul si primul, va fi 'selectat'
		if (I->nIndex <= pParent->m_nFirstIndex && I->nIndex >= pParent->m_nLastIndex)
			hBmpStatic = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_STATICSEL));
		//altfel va fi normal
		else hBmpStatic = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_STATIC));
	}

	//selectam bitmap-ul incarcat in hMemDC
	HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hBmpStatic);

	//il desenam la dimensiunea ceruta
	StretchBlt(ps.hdc, 0, 0, rect.right, rect.bottom, hMemDC, 0, 0, 27, 17, SRCCOPY);

	WCHAR wsText[7];
	int len = GetWindowTextLengthW(hStatic);
	GetWindowTextW(hStatic, wsText, 7);
	SetBkMode(ps.hdc, TRANSPARENT);
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	HFONT hOldFont = (HFONT)SelectObject(ps.hdc, hFont);
	//desenam textul setat cu fontul DEFAULT_GUI_FONT cu background transparent, centrat
	DrawTextW(ps.hdc, wsText, len, &rect, DT_CENTER | DT_VCENTER);

	//FINALIZARE
	SelectObject(hMemDC, hOldFont);
	SelectObject(hMemDC, hOldBmp);
	DeleteObject(hBmpStatic);
	DeleteDC(hMemDC);

	EndPaint(hStatic, &ps);
}

void GenListItem::OnLButtonDown(HWND hStatic, WPARAM wParam)
{
	//daca shift este apasat, setam last si ne asiguram ca first < last
	//daca shift nu este apasat, setam first si stergem last
	
	//gasim mai intai index-ul item-ului
	bool bFound = false;
	CListSide* pParent = (CListSide*)GetWindowLongPtrW(GetParent(hStatic), GWL_USERDATA);
	deque<CListSide::LINE>::iterator I;
	for (I = pParent->m_Lines.begin(); I != pParent->m_Lines.end(); I ++)
	{
		if (I->edSecond.m_hWnd == hStatic)
		{
			bFound = true;
			break;
		}
	}

	//bFound trebuie sa fie mereu TRUE (adica, sa fie gasit hEdit in m_Lines)
	_ASSERT(bFound);

	if (!bFound) return;

	//daca tasta Shift e apasata
	if (wParam & MK_SHIFT)
	{
		//daca este selectat unul deja, il setam pe acesta ca fiind ultimul
		if (pParent->m_nFirstIndex > -1)
			pParent->m_nLastIndex = I->nIndex;
		//altfel inseamna ca nu era selectat nici unul cand am apasat shift.
		//deci, il setam pe acesta ca fiind primul
		else 
			pParent->m_nFirstIndex = I->nIndex;
	}
	//dak nu e apasat shift
	else
	{
		//daca este deja o selectie de mai multe elemente
		if (pParent->m_nFirstIndex > -1 && pParent->m_nLastIndex > -1)
		{
			//setam sa fie selectat doar itemul curent (I->nIndex)
			pParent->m_nFirstIndex = I->nIndex;
			pParent->m_nLastIndex = -1;
		}
		else
			//daca era selectat un singur item, sau daca nu era selectat nici unul
		{
			//daca itemul curent era selectat, il deselectam
			if (pParent->m_nFirstIndex == I->nIndex)
				pParent->m_nFirstIndex = -1;
			//daca itemul curent nu era selectat, il selectam.
			else pParent->m_nFirstIndex = I->nIndex;
		}
	}

	//redesenam itemul
	RedrawWindow(GetParent(hStatic), 0, 0, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
}

void GenListItem::OnStaticContextMenu(HWND hStatic, WORD x, WORD y)
{
	bool bFound = false;
	//gasim hStatic in lista m_Lines
	CListSide* pParent = (CListSide*)GetWindowLongPtrW(GetParent(hStatic), GWL_USERDATA);
	deque<CListSide::LINE>::iterator I;
	for (I = pParent->m_Lines.begin(); I != pParent->m_Lines.end(); I ++)
	{
		if (I->edSecond.m_hWnd == hStatic)
		{
			bFound = true;
			break;
		}
	}

	//bFound trebuie sa fie mereu TRUE (adica, sa fie gasit hEdit in m_Lines)
	_ASSERT(bFound);

	if (!bFound) return;

	//aici a fost apasat butonul din dreapta al mouse-ului
	//first == -1 => last == -1
	if (pParent->m_nFirstIndex == -1)
		//selectam item-ul pentru care cream meniul contextual.
		OnLButtonDown(hStatic, 0);
	//first > -1 last == -1
	else if (pParent->m_nLastIndex == -1)
	{
		//daca itemul curent este altul decat cel ce era setat...
		if (I->nIndex != pParent->m_nFirstIndex)
			OnLButtonDown(hStatic, 0);
	}
	//first > -1 last > -1; first < last
	else if (pParent->m_nFirstIndex < pParent->m_nLastIndex)
	{
		//daca itemul nostru se afla inainte de, sau dupa un grup selectat,
		//first fiind cel de sus, last = cel de jos
		if (I->nIndex < pParent->m_nFirstIndex || I->nIndex > pParent->m_nLastIndex)
			OnLButtonDown(hStatic, 0);
	}
	//first > -1 last < -1; first > last
	else
	{
		//daca itemul nostru se afla inainte de, sau dupa un grup selectat, last = cel de sus, first = cel de jos
		if (I->nIndex > pParent->m_nFirstIndex || I->nIndex < pParent->m_nLastIndex)
			OnLButtonDown(hStatic, 0);
	}
	//altfel nu modificam selectia.

	enum mr {ID_CANCELED, ID_CUT, ID_COPY, ID_PASTE, ID_REMOVELINE};
	mr result = ID_CANCELED;

	//pastram minim, indexul cel mic din selectie si maxim, indexul cel mare din selectie
	//care a fost selectat primul va fi irelevant
	int minim = -1, maxim = -1;
	if (pParent->m_nLastIndex == -1)
		minim = maxim = pParent->m_nFirstIndex;
	else
	{
		minim = min(pParent->m_nFirstIndex, pParent->m_nLastIndex);
		maxim = max(pParent->m_nFirstIndex, pParent->m_nLastIndex);
	}

	//cream meniul
	HMENU hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, ID_CUT, L"&Decupează Textul");
	AppendMenu(hMenu, MF_STRING, ID_COPY, L"&Copiază Textul");
	
	//putem lipi (paste) doar daca este continut in Copy_Buf, si daca min == max
	//(nu putem lipi(paste) text cand sunt mai multe linii selectate
	if (minim != maxim || pParent->m_Copy_Buf.size() == 0)
		AppendMenu(hMenu, MF_STRING | MF_GRAYED, ID_PASTE, L"&Lipește Textul");
	else
		AppendMenu(hMenu, MF_STRING, ID_PASTE, L"&Lipește Textul");

	if (minim == maxim)
	{	
		AppendMenu(hMenu, MF_STRING, ID_REMOVELINE, L"&Elimină Linia");
	}
	else
	{
		AppendMenu(hMenu, MF_STRING, ID_REMOVELINE, L"&Elimină Liniile");
	}

	//se afiseaza meniul, rezultatul pastrandu-se in variabila result.
	result = (mr)TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON, x, y, 0, hStatic, 0);

	switch (result)
	{
	case ID_CANCELED: return;
	//decupam textul din toate liniile 'selectate'
	case ID_CUT: 
		{
			//nu va mai fi nici un item selectat
			pParent->m_nFirstIndex = pParent->m_nLastIndex = -1;
			RedrawWindow(GetParent(hStatic), 0, 0, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);

			//golim lista de siruri
			pParent->m_Copy_Buf.clear();
			//copiem fiecare 'Text' selectat in m_Copy_Buf si apoi stergem acea linie
			//incepand cu ultimul, pana la primul
			for (int i = maxim; i >= minim; i--)
			{
				HWND hEdit = pParent->m_Lines[i].edText.m_hWnd;
				WCHAR wstr[257];//256 caractere + 1 caracterul null
				GetWindowTextW(hEdit, wstr, 257);
				//adaugam la sfarsitul listei
				pParent->m_Copy_Buf.push_back(wstr);
				//textul va fi sters, si se va apela WM_DELLINEAT_BACK, ca atunci cand s-ar fi apasat tasta
				//Backspace cand caseta de editare nu ar fi avut nici un text scris
				SetWindowTextW(hEdit, L"");
				SendMessage(hEdit, EM_SETSEL, 0, 0);
				SendMessage(GetParent(hEdit), WM_DELLINEAT_BACK, i, (LPARAM)hEdit);
			}
		}
		break;

	//copiem textul din toate liniile 'selectate'
	case ID_COPY:
		{
			//golim lista de siruri
			pParent->m_Copy_Buf.clear();
			//copiem fiecare 'Text' selectat in m_Copy_Buf, incepand cu ultimul, pana la primul
			for (int i = maxim; i >= minim; i--)
			{
				HWND hEdit = pParent->m_Lines[i].edText.m_hWnd;
				WCHAR wstr[257];//256 caractere + 1 caracterul null
				GetWindowTextW(hEdit, wstr, 257);
				//adaugam la sfarsitul listei
				pParent->m_Copy_Buf.push_back(wstr);
			}
		}
		break;
		
	//se 'lipesc' in tabel liniile 'copiate'/'decupate'. asta inseamna ca se creeaza linii in tabel
	case ID_PASTE:
		{
			int K = I->nIndex;
			int size = pParent->m_Copy_Buf.size();
			int j = 1;

			//deselectam, in caz ca a fost ceva selectat
			pParent->m_nFirstIndex = pParent->m_nLastIndex = -1;
			//redesenam controlul CListSide
			RedrawWindow(GetParent(hStatic), 0, 0, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
			//sunt stocate sirurile in ordinea urmatoare (fie i0 - primul, iN ultimul)
			//iN, iN-1, ... i3, i2, i1, i0.
			//j = 1, creste la fiecare iterare
			for (int i = size - 1; i >= 0; i--, j++)
			{
				//adaugam linii incepand cu urmatorul dupa item-ul curent, una sub alta
				pParent->AddLineAt(j + K);
				//dupa ce se adauga o linie goala, scriem in caseta de editare Text, textul din m_Copy_Buf
				//de la capat din lista, incepand cu i0, pana la iN.
				SetWindowTextW(pParent->m_Lines[K + j].edText.m_hWnd, pParent->m_Copy_Buf[i].data());
			}
		}
		break;

	//daca se alege sa se elimine liniile/linia selectate/-ta
	case ID_REMOVELINE:
		{
			//nu va mai fi nici un item afisat ca selectat
			pParent->m_nFirstIndex = pParent->m_nLastIndex = -1;
			RedrawWindow(GetParent(hStatic), 0, 0, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
			//pentru fiecare din liniile alese pentru stergere
			for (int i = maxim; i >= minim; i--)
			{
				HWND hEdit = pParent->m_Lines[i].edText.m_hWnd;
				SetWindowTextW(hEdit, L"");
				SendMessage(hEdit, EM_SETSEL, 0, 0);
				//se apeleaza WM_DELLINEAT_BACK, ca atunci cand caseta de editare ar fi fost goala si s-ar fi
				//apasat tasta Backspace.
				SendMessage(GetParent(hEdit), WM_DELLINEAT_BACK, i, (LPARAM)hEdit);
			}
		}
		break;
	}

}