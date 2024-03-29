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

void GenListItem::Create(HWND hParent, RECT& rect, int nLine, bool bText, bool bIsFirst, bool bIsLyrics)
{
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_BORDER;
	char str[7] = "";

	if (bIsFirst)
	{
		dwStyle |= WS_GROUP;
	}

	if (bText)
		dwStyle |= ES_AUTOHSCROLL;
	else 
	{
		if (bIsLyrics)
		{
			strcpy_s(str, 7, "0:00");
			dwStyle |= SS_CENTER;
		}
		else 
		{
			strcpy_s(str, 2, "0");
			dwStyle |= ES_NUMBER | WS_TABSTOP | ES_RIGHT;
		}
	}

	if (bIsLyrics && !bText)
		m_hWnd = CreateWindowA("STATIC", str, dwStyle, rect.left, rect.top, rect.right - rect.left + 1, rect.bottom - rect.top + 1, hParent, NULL, 
		genDll.m_hInstance, NULL);
	else
	m_hWnd = CreateWindowA("EDIT", str, dwStyle, rect.left, rect.top, rect.right - rect.left + 1, rect.bottom - rect.top + 1, hParent, NULL, 
		genDll.m_hInstance, NULL);

	if (!bText) SendMessage(m_hWnd, EM_LIMITTEXT, 3, 0);
	else SendMessage(m_hWnd, EM_LIMITTEXT, 255, 0);

	SetWindowLongPtrW(m_hWnd, GWL_USERDATA, bText);
	SendMessage(m_hWnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 1);

	//setting the window procedure
	if (bIsLyrics && !bText)
	{
		OldWndProc = (WNDPROC)SetWindowLongPtrW(m_hWnd, GWL_WNDPROC, (LONG_PTR)StaticProc);
		RedrawWindow(m_hWnd, 0, 0, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
	}
	else
		OldWndProc = (WNDPROC)SetWindowLongPtrW(m_hWnd, GWL_WNDPROC, (LONG_PTR)EditProc);
}

LRESULT CALLBACK GenListItem::EditProc(HWND hEdt, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:
		if (wParam==VK_DELETE)
		{
			CListSide* pParent = (CListSide*)GetWindowLong(GetParent(hEdt), GWL_USERDATA);
			bool bFound = false;
			deque<CListSide::LINE>::iterator I;
			for (I = pParent->m_Lines.begin(); I != pParent->m_Lines.end(); I++)
			{
				if (I->edText.m_hWnd == hEdt)
				{
					bFound = true;
					break;
				}
			}

			if (bFound)
			{
				int start, end;
				SendMessage(hEdt, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
				int len = GetWindowTextLengthA(hEdt);
 			
				if (end == len && I->nIndex + 1 < pParent->m_Lines.size())
				{
					SendMessage(GetParent(hEdt), WM_DELLINEDELAT, (WPARAM)I->nIndex + 1, (LPARAM)pParent->m_Lines[I->nIndex + 1].edText.m_hWnd);
					return 0;
				}
			}
		}

		else if (wParam==VK_BACK)
		{
			CListSide* pParent = (CListSide*)GetWindowLong(GetParent(hEdt), GWL_USERDATA);
			bool bFound = false;
			deque<CListSide::LINE>::iterator I;
			for (I = pParent->m_Lines.begin(); I != pParent->m_Lines.end(); I++)
			{
				if (I->edText.m_hWnd == hEdt)
				{
					bFound = true;
					break;
				}
			}

			if (bFound)
			{
				int start, end;

				SendMessage(hEdt, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
 			
				if (end==0 && I->nIndex > 0)
				{
					SendMessage(GetParent(hEdt), WM_DELLINEAT, (WPARAM)I->nIndex, (LPARAM)hEdt);
					return 0;
				}
			}
		}

		else if (wParam == VK_RETURN)
		{
			CListSide* pParent = (CListSide*)GetWindowLong(GetParent(hEdt), GWL_USERDATA);
			bool bFound = false;
			deque<CListSide::LINE>::iterator I;
			for (I = pParent->m_Lines.begin(); I != pParent->m_Lines.end(); I++)
			{
				if (I->edText.m_hWnd == hEdt)
				{
					bFound = true;
					break;
				}
			}

			if (bFound)
			{
				int start, end;

				SendMessage(hEdt, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
 			
				SendMessage(GetParent(hEdt), WM_ADDLINEAT, MAKEWPARAM(I->nIndex, end), (LPARAM)hEdt);
				return 0;
			}
		}

		else if (wParam == VK_TAB)
		{
			//if we are in CLyricsDlg, TAB should have no effect.
			if (0 == GetDlgItem(GetParent(GetParent(GetParent(hEdt))), IDC_PLAY)) break;

			HWND hNext = GetNextWindow(hEdt, GW_HWNDNEXT);
			if (hNext)
				SetFocus(hNext);
			else SetFocus(GetNextWindow(GetParent(GetParent(hEdt)), GW_HWNDNEXT));
		}

		if (wParam != VK_HOME && wParam != VK_END)
			SendMessage(GetParent(hEdt), WM_KEYDOWN, wParam, lParam);
		break;//WM_KEYDOWN

	case WM_LBUTTONDBLCLK:
		{
			CListSide* pParent = (CListSide*)GetWindowLong(GetParent(hEdt), GWL_USERDATA);
			bool bFound = false;
			deque<CListSide::LINE>::iterator I;
			for (I = pParent->m_Lines.begin(); I != pParent->m_Lines.end(); I++)
			{
				if (I->edText.m_hWnd == hEdt || I->edSecond.m_hWnd == hEdt)
				{
					bFound = true;
					break;
				}
			}

			if (bFound)
			{
				SendMessage(GetParent(hEdt), WM_SELECTLINE, (WPARAM)I->nIndex, (LPARAM)hEdt);
			}
			
			return 0;
		}
		break;//WM_LBUTTONDBLCLK

	case WM_KILLFOCUS:
		{
			//only if it is SECOND
			bool bText = (bool)GetWindowLongPtrW(hEdt, GWL_USERDATA);
			if (bText) break;
			char str[7];
			GetWindowTextA(hEdt, str, 7);
			if (strlen(str) == 0)
				SetWindowTextA(hEdt, "0");
		}
		break;//WM_KILLFOCUS

	case WM_CONTEXTMENU:
		{
			//hEdit, x, y
			OnContextMenu(HWND(wParam), LOWORD(lParam), HIWORD(lParam));
			return 0;
		}
		break;//WM_CONTEXTMENU

	case WM_SETFOCUS:
	case WM_LBUTTONUP:
		{
			bool bText = (bool)GetWindowLongPtrW(hEdt, GWL_USERDATA);
			if (bText) break;
			SendMessage(hEdt, EM_SETSEL, 0, 7);
		}
		break;//WM_SETFOCUS
	}

	return CallWindowProc(OldWndProc, hEdt, uMsg, wParam, lParam);
}

LRESULT CALLBACK GenListItem::StaticProc(HWND hStatic, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SETTEXT: 
		{
			LRESULT nResult = DefWindowProc(hStatic, uMsg, wParam, lParam);
			RedrawWindow(hStatic, 0, 0, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
			return nResult;
		}
	case WM_PAINT: OnStaticPaint(hStatic); return 0;
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

void GenListItem::OnContextMenu(HWND hEdit, WORD x, WORD y)
{
	enum mr {ID_CANCELED = 0, ID_UNDO, ID_CUT, ID_COPY, ID_PASTE, ID_DELETE, ID_SELECTLINE, ID_ADDLINE, ID_REMOVELINE};
	mr result = mr::ID_CANCELED;

	CListSide* pParent = (CListSide*)GetWindowLong(GetParent(hEdit), GWL_USERDATA);
	int nSel = pParent->GetSelectedLine();

	HMENU hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, ID_UNDO, L"&Anulează");
	AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(hMenu, MF_STRING, ID_CUT, L"&Decupează");
	AppendMenu(hMenu, MF_STRING, ID_COPY, L"&Copiază");
	AppendMenu(hMenu, MF_STRING, ID_PASTE, L"&Lipește");
	AppendMenu(hMenu, MF_STRING, ID_DELETE, L"&Șterge");
	AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
	if (nSel > -1 && pParent->m_Lines[nSel].edText.m_hWnd == hEdit)
		AppendMenu(hMenu, MF_STRING, ID_SELECTLINE, L"De&selectează Linia");
	else
		AppendMenu(hMenu, MF_STRING, ID_SELECTLINE, L"&Selectează Linia");
	AppendMenu(hMenu, MF_STRING, ID_ADDLINE, L"&Adaugă Linie");
	AppendMenu(hMenu, MF_STRING, ID_REMOVELINE, L"&Elimină Linie");

	result = (mr)TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON, x, y, 0, hEdit, 0);

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
			SendMessage(hEdit, EM_SETSEL, len, len);
			SendMessage(GetParent(hEdit), WM_ADDLINEAT, MAKEWPARAM(I->nIndex, len), (LPARAM)hEdit);
		}
		break;
	case ID_REMOVELINE:
		if (bFound)
		{
			SetWindowTextA(hEdit, "");
			SendMessage(hEdit, EM_SETSEL, 0, 0);
			SendMessage(GetParent(hEdit), WM_DELLINEAT, (WPARAM)I->nIndex, (LPARAM)hEdit);
		}
		break;
	}
}

void GenListItem::OnStaticPaint(HWND hStatic)
{
	bool bFound = false;
	CListSide* pParent = (CListSide*)GetWindowLong(GetParent(hStatic), GWL_USERDATA);
	deque<CListSide::LINE>::iterator I;
	for (I = pParent->m_Lines.begin(); I != pParent->m_Lines.end(); I ++)
	{
		if (I->edSecond.m_hWnd == hStatic)
		{
			bFound = true;
			break;
		}
	}
	if (!bFound) return;

	PAINTSTRUCT ps;
	BeginPaint(hStatic, &ps);

	HDC hMemDC = CreateCompatibleDC(ps.hdc);
	RECT rect;
	GetClientRect(hStatic, &rect);

	HBITMAP hBmpStatic;
	
	//fist == -1, last == -1
	if (pParent->m_nFirstIndex == -1)
		hBmpStatic = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_STATIC));
	//first > -1, last == -1
	else if (pParent->m_nLastIndex == -1)
	{
		if (I->nIndex == pParent->m_nFirstIndex)
			hBmpStatic = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_STATICSEL));
		else
			hBmpStatic = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_STATIC));
	}
	//first >-1, last >-1, first < last
	else if (pParent->m_nFirstIndex < pParent->m_nLastIndex)
	{
		if (I->nIndex >= pParent->m_nFirstIndex && I->nIndex <= pParent->m_nLastIndex)
			hBmpStatic = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_STATICSEL));
		else hBmpStatic = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_STATIC));
	}
	//first >-1 last > -1, first > last
	else
	{
		if (I->nIndex <= pParent->m_nFirstIndex && I->nIndex >= pParent->m_nLastIndex)
			hBmpStatic = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_STATICSEL));
		else hBmpStatic = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_STATIC));
	}

	HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hBmpStatic);

	StretchBlt(ps.hdc, 0, 0, rect.right, rect.bottom, hMemDC, 0, 0, 27, 17, SRCCOPY);

	char sText[7];
	int len = GetWindowTextLengthA(hStatic);
	GetWindowTextA(hStatic, sText, 7);
	SetBkMode(ps.hdc, TRANSPARENT);
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SelectObject(ps.hdc, hFont);
	DrawTextA(ps.hdc, sText, len, &rect, DT_CENTER | DT_VCENTER);

	SelectObject(hMemDC, hOldBmp);
	DeleteObject(hBmpStatic);
	DeleteDC(hMemDC);

	EndPaint(hStatic, &ps);
}

void GenListItem::OnLButtonDown(HWND hStatic, WPARAM wParam)
{
	//if shift is pressed, we set last and make sure first < last
	//if shift is not pressed, we set first and erase last
	
	//we first find the index of this item
	bool bFound = false;
	CListSide* pParent = (CListSide*)GetWindowLong(GetParent(hStatic), GWL_USERDATA);
	deque<CListSide::LINE>::iterator I;
	for (I = pParent->m_Lines.begin(); I != pParent->m_Lines.end(); I ++)
	{
		if (I->edSecond.m_hWnd == hStatic)
		{
			bFound = true;
			break;
		}
	}

	if (!bFound) return;

	if (wParam & MK_SHIFT)
	{
		if (pParent->m_nFirstIndex > -1)
			pParent->m_nLastIndex = I->nIndex;
		else 
			pParent->m_nFirstIndex = I->nIndex;
	}
	//no shift
	//we set the first, and make -1 the last
	//if first is the same and last is not selected, then we deselect
	else
	{
		if (pParent->m_nFirstIndex > -1 && pParent->m_nLastIndex > -1)
		{
			pParent->m_nFirstIndex = I->nIndex;
			pParent->m_nLastIndex = -1;
		}
		else
			//first should always be > -1, last is here -1
		{
			if (pParent->m_nFirstIndex == I->nIndex)
				pParent->m_nFirstIndex = -1;
			else pParent->m_nFirstIndex = I->nIndex;
		}
	}

	RedrawWindow(GetParent(hStatic), 0, 0, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
}

void GenListItem::OnStaticContextMenu(HWND hStatic, WORD x, WORD y)
{
	//if not in range first->last or last->first
//	OnLButtonDown(HWND hStatic, 0);
	//else use context menu as usually.

	bool bFound = false;
	CListSide* pParent = (CListSide*)GetWindowLong(GetParent(hStatic), GWL_USERDATA);
	deque<CListSide::LINE>::iterator I;
	for (I = pParent->m_Lines.begin(); I != pParent->m_Lines.end(); I ++)
	{
		if (I->edSecond.m_hWnd == hStatic)
		{
			bFound = true;
			break;
		}
	}
	if (!bFound) return;

	//first == -1, last == -1
	if (pParent->m_nFirstIndex == -1)
		OnLButtonDown(hStatic, 0);
	//first > -1 last == -1
	else if (pParent->m_nLastIndex == -1)
	{
		if (I->nIndex != pParent->m_nFirstIndex)
			OnLButtonDown(hStatic, 0);
	}
	//first > -1 last > -1; first < last
	else if (pParent->m_nFirstIndex < pParent->m_nLastIndex)
	{
		if (I->nIndex < pParent->m_nFirstIndex || I->nIndex > pParent->m_nLastIndex)
			OnLButtonDown(hStatic, 0);
	}
	//first > -1 last < -1; first > last
	else
	{
		if (I->nIndex > pParent->m_nFirstIndex || I->nIndex < pParent->m_nLastIndex)
			OnLButtonDown(hStatic, 0);
	}

	enum mr {ID_CANCELED = 0, ID_CUT, ID_COPY, ID_PASTE, ID_REMOVELINE};
	mr result = mr::ID_CANCELED;

	int minim = -1, maxim = -1;
	if (pParent->m_nLastIndex == -1)
		minim = maxim = pParent->m_nFirstIndex;
	else
	{
		minim = min(pParent->m_nFirstIndex, pParent->m_nLastIndex);
		maxim = max(pParent->m_nFirstIndex, pParent->m_nLastIndex);
	}

	HMENU hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, ID_CUT, L"&Decupează Textul");
	AppendMenu(hMenu, MF_STRING, ID_COPY, L"&Copiază Textul");
	//we can paste ONLY IF there is content in Copy_Buf, and it min != max
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

	result = (mr)TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON, x, y, 0, hStatic, 0);

	switch (result)
	{
	case ID_CANCELED: return;
	case ID_CUT: 
		{
			pParent->m_nFirstIndex = pParent->m_nLastIndex = -1;
			RedrawWindow(GetParent(hStatic), 0, 0, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
			pParent->m_Copy_Buf.clear();
			for (int i = maxim; i >= minim; i--)
			{
				HWND hEdit = pParent->m_Lines[i].edText.m_hWnd;
				char str[100];
				GetWindowTextA(hEdit, str, 100);
				pParent->m_Copy_Buf.push_back(str);
				SetWindowTextA(hEdit, "");
				SendMessage(hEdit, EM_SETSEL, 0, 0);
				SendMessage(GetParent(hEdit), WM_DELLINEAT, i, (LPARAM)hEdit);
			}
		}
		break;
	case ID_COPY:
		{
			pParent->m_Copy_Buf.clear();
			for (int i = maxim; i >= minim; i--)
			{
				HWND hEdit = pParent->m_Lines[i].edText.m_hWnd;
				char str[100];
				GetWindowTextA(hEdit, str, 100);
				pParent->m_Copy_Buf.push_back(str);
			}
		}
		break;
	case ID_PASTE:
		{
			int K = I->nIndex;
			int size = pParent->m_Copy_Buf.size();
			int j = 1;
			pParent->m_nFirstIndex = pParent->m_nLastIndex = -1;
			RedrawWindow(GetParent(hStatic), 0, 0, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
			for (int i = size - 1; i >= 0; i--, j++)
			{
				pParent->AddLineAt(j + K);
				SetWindowTextA(pParent->m_Lines[K + j].edText.m_hWnd, pParent->m_Copy_Buf[i].data());
			}
		}
		break;

	case ID_REMOVELINE:
		{
			pParent->m_nFirstIndex = pParent->m_nLastIndex = -1;
			RedrawWindow(GetParent(hStatic), 0, 0, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
			for (int i = maxim; i >= minim; i--)
			{
				HWND hEdit = pParent->m_Lines[i].edText.m_hWnd;
				SetWindowTextA(hEdit, "");
				SendMessage(hEdit, EM_SETSEL, 0, 0);
				SendMessage(GetParent(hEdit), WM_DELLINEAT, i, (LPARAM)hEdit);
			}
		}
		break;
	}

}