// HyperLink.cpp : implementation file
//

#include "HyperLink.h"
#include "ChooseDlg.h"
#include "GenDll.h"
#include "Tools.h"

extern CGenDll genDll;

HCURSOR CHyperLink::m_hArrowCursor = 0, CHyperLink::m_hHandCursor = 0;

CHyperLink::CHyperLink()
{
	//initializari
	m_hArrowCursor = m_hHandCursor = 0;
	m_hWnd = NULL;
	m_bUnderline = 0;
	m_bIsLink = true;

	//se creaza clasa ferestrei, daca nu a fost deja creata
	WNDCLASS wndclass;
	if (false == GetClassInfo(genDll.m_hInstance, L"GEN_YM10_HLINK", &wndclass))
	{
		ZeroMemory(&wndclass, sizeof(wndclass));

		wndclass.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
		wndclass.hInstance = genDll.m_hInstance;
		wndclass.lpfnWndProc = WindowProc;
		wndclass.lpszClassName = L"GEN_YM10_HLINK";
		wndclass.style = CS_VREDRAW | CS_HREDRAW;

		if (0 == RegisterClass(&wndclass))
		{
			DisplayError(0);
		}
	}
}

CHyperLink::~CHyperLink()
{
}


LRESULT CALLBACK CHyperLink::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//pointer la obiectul de clasa CHyperLink ce are ca fereastra hWnd.
	CHyperLink* pThis = (CHyperLink*)GetWindowLongPtrW(hWnd, GWL_USERDATA);

	if (pThis)
		switch (uMsg)
	{
		case WM_SETTEXT:
			{
				HDC hDC = GetDC(hWnd);
				RECT rect = {0};
				WCHAR* wsText = (WCHAR*)lParam;
				DrawText(hDC, wsText, -1, &rect, DT_CALCRECT);
				//se redimensioneaza controlul.
				SetWindowPos(hWnd, 0, 0, 0, rect.right, rect.bottom, SWP_NOMOVE | SWP_NOZORDER);
				ReleaseDC(hWnd, hDC);
			}
			break;

		case WM_PAINT: pThis->OnPaint(); return 0;
		case WM_LBUTTONUP: 
			{
				if (pThis->m_bIsLink)
					pThis->onClickProc();
				break;
			}

		//trimis cand cursorul mouse-ului paraseste controlul
		case WM_MOUSELEAVE:
			{
				if (pThis->m_bUnderline)
				{
					pThis->m_bUnderline = false;
					InvalidateRect(hWnd, 0, 1);
				}
			}
			break;
		case WM_MOUSEMOVE: pThis->OnMouseMove(); break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


void CHyperLink::OnPaint()
{
	//initializare
	PAINTSTRUCT ps;
	BeginPaint(m_hWnd, &ps);

	HPEN hOldPen = (HPEN)SelectObject(ps.hdc, GetStockObject(NULL_PEN));
	HBRUSH hOldBrush = (HBRUSH)SelectObject(ps.hdc, GetSysColorBrush(COLOR_3DFACE));

	RECT clr;
	GetClientRect(m_hWnd, &clr);
	//se deseneaza controlul
	Rectangle(ps.hdc, 0, 0, clr.right, clr.bottom);
	SetBkMode(ps.hdc, TRANSPARENT);

	//se creeaza fontul
	LOGFONT logfont;
	HFONT hFont = (HFONT)SendMessage(GetParent(m_hWnd), WM_GETFONT, 0, 0);
	GetObject(hFont, sizeof(LOGFONT), &logfont);

	if (m_bUnderline && m_bIsLink)
	{
		logfont.lfUnderline = TRUE;
	}
	else 
		logfont.lfUnderline = FALSE;

	hFont = CreateFontIndirect(&logfont);
	HFONT hOldFont = (HFONT)SelectObject(ps.hdc, hFont);

	//se seteaza culoarea
	if (m_bIsLink)
		SetTextColor(ps.hdc, RGB(34,0,204));
	else SetTextColor(ps.hdc, 0);
	
	//se afiseaza textul
	int len = GetWindowTextLength(m_hWnd);
	len++;
	WCHAR* wstr = new WCHAR[len];
	GetWindowTextW(m_hWnd, wstr, len);

	DrawTextW(ps.hdc, wstr, -1, &clr, DT_LEFT);

	SelectObject(ps.hdc, hOldPen);
	SelectObject(ps.hdc, hOldBrush);
	DeleteObject(hOldFont);
	delete[] wstr;

	EndPaint(m_hWnd, &ps);
}

void CHyperLink::Create(HWND hDlg, DWORD dwID, CLICKPROC onClick, BOOL bIsLink)
{
	m_bIsLink = bIsLink;

	if (0 == m_hHandCursor)
		m_hHandCursor = LoadCursor(0, IDC_HAND); 
	if (0 == m_hArrowCursor)
		m_hArrowCursor = LoadCursor(0, IDC_ARROW);

	//se salveaza pointer la obiectul apelant
	m_hWnd = GetDlgItem(hDlg, dwID);
	SetWindowLongPtrW(m_hWnd, GWL_USERDATA, (LONG)this);

	//se seteaza functia pentru prelucrarea click-ului cu butonul din stanga al mouse-ului
	onClickProc = onClick;

	//urmarim evenimentul de parasire al hyperlink-ului de catre cursor
	TRACKMOUSEEVENT tr_event;
	tr_event.cbSize = sizeof(tr_event);
	tr_event.dwFlags = TME_LEAVE;
	tr_event.dwHoverTime = 0;
	tr_event.hwndTrack = m_hWnd;
	TrackMouseEvent(&tr_event);
}

void CHyperLink::OnMouseMove()
{
	if (false == m_bIsLink) return;

	//urmarim evenimentul de parasire al hyperlink-ului de catre cursor
	TRACKMOUSEEVENT tr_event;
	tr_event.cbSize = sizeof(tr_event);
	tr_event.dwFlags = TME_LEAVE;
	tr_event.dwHoverTime = 0;
	tr_event.hwndTrack = m_hWnd;
	TrackMouseEvent(&tr_event);

	//redesenam daca e nevoie
	if (!m_bUnderline)
	{
		m_bUnderline = true;
		InvalidateRect(m_hWnd, 0, 1);
	}
	
	//setam cursorul 
	SetCursor(m_hHandCursor);
}

void CHyperLink::SetLinkState(BOOL bIsLink)
{
	//se seteaza noua stare si se redeseneaza
	if (m_bIsLink != bIsLink)
	{
		m_bIsLink = bIsLink;
		InvalidateRect(m_hWnd, 0, 1);
	}
}