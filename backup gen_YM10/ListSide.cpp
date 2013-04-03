#include "ListSide.h"
#include "GenDll.h"

#define TABLECLASS	"TABLECLASS"

extern CGenDll genDll;
HFONT CListSide::m_hFont;
int CListSide::m_dMaxScroll;
int CListSide::m_nLineHeight;
int CListSide::m_dPage;

CListSide::CListSide(void)
{
	m_dMaxScroll = -1;
	m_nLineHeight = -1;
	m_dPage = -1;
	m_nSelLine = -1;
	m_bEnableLineSel = false;
	m_bScrollEnabled = false;
	m_dScrollPos = 0;
	m_nFirstIndex = -1;
	m_nLastIndex = -1;
}

CListSide::~CListSide(void)
{
	DeleteObject(m_hFont);
	if (m_Lines.size())
	{
		deque<LINE>::iterator I;
		for (I = m_Lines.begin(); I != m_Lines.end(); I++)
		{
			if (IsWindow(I->edSecond.m_hWnd)) DestroyWindow(I->edSecond.m_hWnd);
			if (IsWindow(I->edText.m_hWnd)) DestroyWindow(I->edText.m_hWnd);
		}

		m_Lines.clear();
	}
}

void CListSide::Create(HWND hParent, RECT rect, HFONT hFont, int nVBarPos, int nLineHeight, bool bEnableLineSel, bool bIsLyrics)
{
	m_bIsLyrics = bIsLyrics;
	m_bEnableLineSel = bEnableLineSel;
	rect.bottom--;
	rect.right--;
	WNDCLASSA wndclass;
	if (FALSE == GetClassInfoA(genDll.m_hInstance, TABLECLASS, &wndclass))
	{
		ZeroMemory(&wndclass, sizeof(WNDCLASS));

		wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass.hInstance = genDll.m_hInstance;
		wndclass.lpfnWndProc = WindowProc;
		wndclass.lpszClassName = TABLECLASS;
		wndclass.style = CS_VREDRAW | CS_HREDRAW | CS_CLASSDC;

		if (FALSE ==  RegisterClassA(&wndclass))
		{
			DWORD dwError = GetLastError();
			char str[20];
			sprintf(str, "Error 0x%x", dwError);
			MessageBoxA(genDll.m_hWinamp, str, "Fatal Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
			PostQuitMessage(-1);
			}
	}

	m_hWnd = CreateWindowA(TABLECLASS, 0, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_VSCROLL, rect.left, rect.top, 
rect.right - rect.left + 1, rect.bottom - rect.top + 1, hParent, NULL, genDll.m_hInstance, NULL);

	if (m_hWnd == 0)
	{
		DWORD dwError = GetLastError();
		char str[20];
		sprintf(str, "Error 0x%x", dwError);
		MessageBoxA(genDll.m_hWinamp, str, "Fatal Error!", MB_ICONERROR);
#ifdef _DEBUG
		DebugBreak();
#endif
		PostQuitMessage(-1);
	}

	GetClientRect(m_hWnd, &m_rClient);
	m_nVBarPos = nVBarPos;

	SetWindowLongPtrW(m_hWnd, GWL_USERDATA, (LONG_PTR)this);
	EnableScrollBar(m_hWnd, SB_VERT, ESB_DISABLE_BOTH);
	
	m_hFont = hFont;
	m_nLineHeight = nLineHeight;
	AddLine();
}

LRESULT CALLBACK CListSide::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:
		{
			int nScrollNotify = -1;

			bool bFoundT = false, bFoundS = false;
			HWND hEdit = GetFocus();
			CListSide* pThis = (CListSide*)GetWindowLongPtrW(hWnd, GWL_USERDATA);
			deque<CListSide::LINE>::iterator I;
			for (I = pThis->m_Lines.begin(); I != pThis->m_Lines.end(); I++)
			{
				if (I->edText.m_hWnd == hEdit)
				{
					bFoundT = true;
					break;
				}
				if (I->edSecond.m_hWnd == hEdit)
				{
					bFoundS = true;
					break;
				}
			}

			switch (wParam)
			{
			case VK_SHIFT: break;
			case VK_NEXT:
				nScrollNotify = SB_PAGEDOWN; break;
			case VK_PRIOR:
				nScrollNotify = SB_PAGEUP; break;
			case VK_HOME:
				nScrollNotify = SB_TOP; break;
			case VK_END:
				nScrollNotify = SB_BOTTOM; break;
			case VK_UP:
				{	
					if (I->nIndex != 0)
					{
						if (bFoundT)
							SetFocus(pThis->m_Lines.at(I->nIndex - 1).edText.m_hWnd);
						if (bFoundS)
							SetFocus(pThis->m_Lines.at(I->nIndex - 1).edSecond.m_hWnd);
						
						RECT rect;
						GetWindowRect(pThis->m_Lines[I->nIndex-1].edText.m_hWnd, &rect);
						POINT pt = {rect.left, rect.top};
						ScreenToClient(hWnd, &pt);
						if (0 > pt.y || pThis->m_dPage < pt.y)
							pThis->ScrollTo(pt.y + pThis->m_dScrollPos);
						else
						{
							GetWindowRect(pThis->m_Lines[I->nIndex].edText.m_hWnd, &rect);
							pt.x = rect.left; pt.y = rect.top;
							ScreenToClient(hWnd, &pt);
							if (pt.y < 0)
								nScrollNotify = SB_LINEUP;
						}
					}
					
				}
				break;
			case VK_DOWN:
				{	
					if (I->nIndex != pThis->m_Lines.back().nIndex)
					{
						if (bFoundT)
							SetFocus(pThis->m_Lines.at(I->nIndex + 1).edText.m_hWnd);
						if (bFoundS)
							SetFocus(pThis->m_Lines.at(I->nIndex + 1).edSecond.m_hWnd);

				
						RECT rect;
						GetWindowRect(pThis->m_Lines[I->nIndex + 1].edText.m_hWnd, &rect);
						POINT pt = {rect.right, rect.bottom};
						ScreenToClient(hWnd, &pt);

						if (pThis->m_dScrollPos > pt.y || pThis->m_dScrollPos + pThis->m_dPage - 1 < pt.y)
							pThis->ScrollTo(pt.y + pThis->m_dScrollPos - pThis->m_dPage);
						else
						{
							GetWindowRect(pThis->m_Lines[I->nIndex].edText.m_hWnd, &rect);
							pt.x = rect.left; pt.y = rect.bottom;
							ScreenToClient(hWnd, &pt);
							if (pt.y  >= pThis->m_dPage)
								nScrollNotify = SB_LINEDOWN;
						}
						
					} 
				}
				break;

			default:
				{	
					if (!bFoundT && !bFoundS) break;
					RECT rect;
					GetWindowRect(pThis->m_Lines[I->nIndex].edText.m_hWnd, &rect);
					POINT pt = {rect.left, rect.top};
					ScreenToClient(hWnd, &pt);
					if (0 > pt.y || pThis->m_dPage < pt.y)
						pThis->ScrollTo(pt.y + pThis->m_dScrollPos);
					else
					{
						GetWindowRect(pThis->m_Lines[I->nIndex].edText.m_hWnd, &rect);
						pt.x = rect.left; pt.y = rect.top;
						ScreenToClient(hWnd, &pt);
						if (pt.y < 0)
							nScrollNotify = SB_LINEUP;
					}
				}
				break;
			}

			if (nScrollNotify!=-1)
			{
				SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(nScrollNotify,0), 0);
			}
		}
		break;//WM_KEYDOWN

	case WM_MOUSEWHEEL:
		{
			INT16 delta = (int)HIWORD(wParam);
			if (delta < 0)
				SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN,0), 0);
			else
				SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEUP,0), 0);
		}
		break;

//	case WM_PAINT: break;
	case WM_VSCROLL: OnVScroll(hWnd, wParam, lParam); break;
	case WM_ADDLINEAT:
		{
			CListSide* pThis = (CListSide*)GetWindowLongPtrW(hWnd, GWL_USERDATA);
			int nLine = LOWORD(wParam);
			int nSelEnd = HIWORD(wParam);
			HWND hEdit = (HWND)lParam;

			pThis->AddLineAt(nLine+1);
			int end = GetWindowTextLengthW(hEdit);
			SendMessage(hEdit, EM_SETSEL, (WPARAM)nSelEnd, (LPARAM)end);
			
			if (nSelEnd < end)
			{
				SendMessage(hEdit, WM_CUT, 0, 0);
				hEdit = pThis->m_Lines.at(nLine + 1).edText.m_hWnd;
				SendMessage(hEdit, WM_PASTE, 0, 0);
			}
			else
				hEdit = pThis->m_Lines.at(nLine + 1).edText.m_hWnd;

			SetFocus(hEdit);
			SendMessage(hEdit, EM_SETSEL, 0, 0);
		}
		break;

	case WM_DELLINEAT:
		{
			CListSide* pThis = (CListSide*)GetWindowLongPtrW(hWnd, GWL_USERDATA);
			int nLine = (int)wParam;
			HWND hEdit= (HWND)lParam;

			//1. copy the content from this line:
			int len = GetWindowTextLengthA(hEdit);
			len++;
			char* str = new char[len];
			int len2;
			if (0 < len)
			{
				GetWindowTextA(hEdit, str, len);
				hEdit = pThis->m_Lines.at(nLine - 1).edText.m_hWnd;
				len2 = GetWindowTextLengthA(hEdit);
				len2++;
				char* str2 = new char[len2];
				GetWindowTextA(hEdit, str2, len2);
				string sir = str2;
				sir += ' ';
				sir += str;
				SetWindowTextA(hEdit, sir.data());
				delete[] str2;
			}
			else
				hEdit = pThis->m_Lines.at(nLine - 1).edText.m_hWnd;

			//2. move cursor to the editbox above
			pThis->RemoveLineAt(nLine+1);
			SetFocus(hEdit);
			SendMessage(hEdit, EM_SETSEL, len2, len2);
			delete[] str;
		}
		break;

	case WM_DELLINEDELAT:
		{
			//using DEL
			CListSide* pThis = (CListSide*)GetWindowLongPtrW(hWnd, GWL_USERDATA);
			int nLine = (int)wParam;
			HWND hEdit= (HWND)lParam;

			//1. copy the content from this line:
			int len = GetWindowTextLengthA(hEdit);
			len++;
			char* str = new char[len];
			int len2;
			string sir;
			if (0 < len)
			{
				GetWindowTextA(hEdit, str, len);
				hEdit = pThis->m_Lines.at(nLine - 1).edText.m_hWnd;
				len2 = GetWindowTextLengthA(hEdit);
				len2++;
				char* str2 = new char[len2];
				GetWindowTextA(hEdit, str2, len2);
				sir = str2;
				sir += ' ';
				sir += str;
				delete[] str2;
			}
			else
				hEdit = pThis->m_Lines.at(nLine - 1).edText.m_hWnd;

			//2. move cursor to the editbox above
			pThis->RemoveLineAt(nLine);
			SetWindowTextA(hEdit, sir.data());
			SetFocus(hEdit);
			SendMessage(hEdit, EM_SETSEL, len2, len2);
			delete[] str;
		}
		break;
		
	case WM_SELECTLINE:
		{
			CListSide* pThis = (CListSide*)GetWindowLongPtrW(hWnd, GWL_USERDATA);
			if (pThis->m_bEnableLineSel == false)
				break;

			int nLine = (int)wParam;
			if (pThis->m_nSelLine > -1)
			{
				pThis->m_Lines.at(pThis->m_nSelLine).bSelected = false;
				RedrawWindow(pThis->m_Lines.at(pThis->m_nSelLine).edText.m_hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
				RedrawWindow(pThis->m_Lines.at(pThis->m_nSelLine).edSecond.m_hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
			}
			if (pThis->m_nSelLine != nLine)
			{
				pThis->m_nSelLine = nLine;
				pThis->m_Lines.at(nLine).bSelected = true;
				RedrawWindow(pThis->m_Lines.at(nLine).edText.m_hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
				RedrawWindow(pThis->m_Lines.at(nLine).edSecond.m_hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);

				RECT rect;
				GetWindowRect(pThis->m_Lines[nLine].edText.m_hWnd, &rect);
				POINT pt = {rect.right, rect.bottom};
				ScreenToClient(hWnd, &pt);

				if (pThis->m_dScrollPos > pt.y || pThis->m_dScrollPos + pThis->m_dPage - 1 < pt.y || pt.y  >= pThis->m_dPage)
					pThis->ScrollTo(pt.y + pThis->m_dScrollPos - pThis->m_dPage);
			}
			else pThis->m_nSelLine = -1;
		}
		break;//WM_SELECTLINE

	case WM_CTLCOLOREDIT:
		{
			HDC hDC = (HDC)wParam;
			HWND hEdit = (HWND)lParam;
			HBRUSH hBrush = 0;
			
			CListSide* pThis = (CListSide*)GetWindowLongPtrW(hWnd, GWL_USERDATA);
			bool bFound = false;
			deque<CListSide::LINE>::iterator I;
			for (I = pThis->m_Lines.begin(); I != pThis->m_Lines.end(); I++)
			{
				if (hEdit == I->edSecond.m_hWnd || hEdit == I->edText.m_hWnd)
				{
					//we found the edit in the list
					if (I->bSelected)
					{
						//the edit is selected
						hBrush = CreateSolidBrush(RGB(0, 0, 255));
						SetBkColor(hDC, RGB(0, 0, 255));
						SetTextColor(hDC, RGB(255, 255, 255));
						break;
					}
					else
					{
						//the edit is not selected
						hBrush = CreateSolidBrush(RGB(255, 255, 255));
						SetBkColor(hDC, RGB(255, 255, 255));
						SetTextColor(hDC, RGB(0, 0, 0));
					}
				}
				//if we did not find the edit in the list, we keep searching, to know whether it is selected or not
			}
			return (LRESULT)hBrush;
		}
		break;//WM_CTLCOLOREDIT
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void CListSide::AddLine(bool bScroll)
{
	LINE line;
	line.bSelected = 0;

	RECT rect;
	int size = m_Lines.size();
	
	if (size == 0)
	{
		rect.top = m_rClient.top;
		rect.bottom = m_rClient.top + m_nLineHeight;
		rect.left = -1;
		rect.right = m_nVBarPos;

		line.edSecond.Create(m_hWnd, rect, 0, false, true, m_bIsLyrics);

		rect.left = m_nVBarPos;
		rect.right = m_rClient.right - 1;
		line.edText.Create(m_hWnd, rect, 0, true, false, m_bIsLyrics);

		line.nIndex = 0;
		SetFocus(line.edText.m_hWnd);
	}

	else
	{
		line.nIndex = size;
		GetWindowRect(m_Lines.back().edText.m_hWnd, &rect);
		POINT pt = {rect.right, rect.bottom};
		ScreenToClient(m_hWnd, &pt);
		rect.top = pt.y - 1;
		rect.bottom = rect.top + m_nLineHeight;

		rect.left = -1;
		rect.right = m_nVBarPos;

		line.edSecond.Create(m_hWnd, rect, size, false, false, m_bIsLyrics);

		rect.left = m_nVBarPos;
		rect.right = m_rClient.right - 1;
		line.edText.Create(m_hWnd, rect, size, true, false, m_bIsLyrics);
	}

	if (rect.bottom > m_rClient.bottom)
	{
		UpdateScrollBar(rect.bottom);
		if (bScroll)
			Scroll(m_nLineHeight);
	}

	m_Lines.push_back(line);
	SendMessage(m_hWnd, WM_PAINT, 0, 0);
}

void CListSide::AddLineAt(int nLine)
{
	AddLine();
	int nrLines = m_Lines.size();
	for (int i = nrLines - 2; i>= nLine; i--)
	{
		HWND hWnd = m_Lines.at(i).edSecond.m_hWnd;
		int len = GetWindowTextLengthA(hWnd);
		char* str = NULL;
		if (len)
		{
			len++;
			str = new char[len];
			GetWindowTextA(hWnd, str, len);
			SetWindowTextA(hWnd, "0:00");
			hWnd = m_Lines.at(i+1).edSecond.m_hWnd;
			SetWindowTextA(hWnd, str);
			delete[] str;
		}

		
		hWnd = m_Lines.at(i).edText.m_hWnd;
		len = GetWindowTextLengthA(hWnd);
		if (len)
		{
			len++;
			str = new char[len];
			GetWindowTextA(hWnd, str, len);
			SetWindowTextA(hWnd, "");
			hWnd = m_Lines.at(i+1).edText.m_hWnd;
			SetWindowTextA(hWnd, str);
			delete[] str;
		}
	}
}


void CListSide::RemoveLineAt(int nLine)
{
	int nrLines = m_Lines.size();
	for (int i = nLine; i < nrLines; i++)
	{
		HWND hWnd = m_Lines.at(i).edSecond.m_hWnd;
		int len = GetWindowTextLengthA(hWnd);
		char* str = NULL;
		if (len)
		{
			len++;
			str = new char[len];
			GetWindowTextA(hWnd, str, len);
			SetWindowTextA(hWnd, "");
			hWnd = m_Lines.at(i-1).edSecond.m_hWnd;
			SetWindowTextA(hWnd, str);
			delete[] str;
		}

		
		hWnd = m_Lines.at(i).edText.m_hWnd;
		len = GetWindowTextLengthA(hWnd);
		if (len)
		{
			len++;
			str = new char[len];
			GetWindowTextA(hWnd, str, len);
			SetWindowTextA(hWnd, "");
			hWnd = m_Lines.at(i-1).edText.m_hWnd;
			SetWindowTextA(hWnd, str);
			delete[] str;
		}
	}

	RemoveLine();

/*	RECT rect;
	GetWindowRect(m_Lines.back().edText.m_hWnd, &rect);
	POINT pt = {rect.right, rect.bottom};
	ScreenToClient(m_hWnd, &pt);

//	int nBottom = m_rClient.top + m_Lines.back().nIndex * m_nLineHeight + m_nLineHeight;
	UpdateScrollBar(pt.y);*/
}

void CListSide::RemoveLine()
{
	DestroyWindow(m_Lines.back().edSecond.m_hWnd);
	DestroyWindow(m_Lines.back().edText.m_hWnd);
	m_Lines.pop_back();

	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_POS;
	GetScrollInfo(m_hWnd, SB_VERT, &si);
	if (si.nPos) Scroll(-m_nLineHeight);

	RECT rect;
	GetWindowRect(m_Lines.back().edText.m_hWnd, &rect);
	POINT pt = {rect.right, rect.bottom};
	ScreenToClient(m_hWnd, &pt);

//	int nBottom = m_rClient.top + m_Lines.back().nIndex * m_nLineHeight + m_nLineHeight;
	UpdateScrollBar(pt.y);
}

void CListSide::UpdateScrollBar(int nBottom)
{
	RECT client;
	GetClientRect(m_hWnd, &client);

	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_POS;
	GetScrollInfo(m_hWnd, SB_VERT, &si);

	si.fMask = SIF_RANGE | SIF_PAGE;
	si.nMin = 0;
	si.nPage = client.bottom;// + 1;
	si.nMax = nBottom + si.nPos;
	m_dMaxScroll = si.nMax;
	m_dPage = si.nPage;

	int add = /*nBottom*/ si.nMax - client.bottom;
	if (add > 0)
	{
		SetScrollInfo(m_hWnd, SB_VERT, &si, 1);
		EnableScrollBar(m_hWnd, SB_VERT, ESB_ENABLE_BOTH);
		m_bScrollEnabled = true;
	}
	else
	{
		EnableScrollBar(m_hWnd, SB_VERT, ESB_DISABLE_BOTH);
		m_bScrollEnabled = false;
	}
}

void CListSide::OnVScroll(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	CListSide* pThis = (CListSide*)GetWindowLongPtrW(hWnd, GWL_USERDATA);
	if (!pThis->m_bScrollEnabled) return;

	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
	GetScrollInfo(hWnd, SB_VERT, &si);

	switch (LOWORD(wParam))
	{
	case SB_BOTTOM: 
		{
			pThis->ScrollTo(si.nMax - si.nPage + 1);
		}
		break;
	case SB_TOP:
		{
			pThis->ScrollTo(0);
		}
		break;
	case SB_LINEDOWN: 
		{
			pThis->Scroll(/*hWnd, */m_nLineHeight);
		}
		break;
	case SB_LINEUP:	
		{
			pThis->Scroll(/*hWnd, */-m_nLineHeight);
		}
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		{
			pThis->ScrollTo(HIWORD(wParam));
		}
		break;
	case SB_PAGEDOWN:
		{
			pThis->Scroll(/*hWnd, */m_dPage);
		}
		break;
	case SB_PAGEUP:
		{
			pThis->Scroll(/*hWnd, */-m_dPage);
		}
		break;
	}
}

void CListSide::ScrollTo(/*HWND hWnd, */int pos)
{
	if (false == m_bScrollEnabled) return;
	
	if (pos > m_dMaxScroll - m_dPage + 1)
		pos = m_dMaxScroll - m_dPage + 1;
	if (pos < 0) pos = 0;

	int add, scrollpos = GetScrollPos(m_hWnd, SB_VERT);
	
	SetScrollPos(m_hWnd, SB_VERT, pos, 1);
	m_dScrollPos = pos;
	add = pos - scrollpos;

	RECT client;
	GetClientRect(m_hWnd, &client);

	ScrollWindow(m_hWnd, 0, -add, NULL, &client);
}

void CListSide::Scroll(/*HWND hWnd, */int nMore)
{
	if (false == m_bScrollEnabled) return;

	int add, scrollpos = GetScrollPos(m_hWnd, SB_VERT);

	int actual = scrollpos + nMore;
	if (actual < 0) 
	{
		ScrollTo(0);
		return;
	}
	if (actual > m_dMaxScroll - m_dPage + 1)
	{
		ScrollTo(m_dMaxScroll - m_dPage + 1);
		return;
	}
	
	SetScrollPos(m_hWnd, SB_VERT, actual, 1);
	m_dScrollPos = actual;
	add = nMore;

	RECT client;
	GetClientRect(m_hWnd, &client);

	ScrollWindow(m_hWnd, 0, -add, NULL, &client);
}