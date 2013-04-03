#include "GenRichEditCtrl.h"
#include "ClockDlg.h"
#include "StatusEditorDlg.h"
#include "resource.h"

#include <deque>

using namespace std;

WNDPROC GenRichEditCtrl::m_OldWndProc;
extern BOOL bFullRights;
extern STATUS Status;

GenRichEditCtrl::GenRichEditCtrl(void)
{
}

GenRichEditCtrl::~GenRichEditCtrl(void)
{
}

void GenRichEditCtrl::Create(HWND hWnd)
{
	m_OldWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWL_WNDPROC, (LONG)WindowProc);
	m_hWnd = hWnd;
}

LRESULT CALLBACK GenRichEditCtrl::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CHAR: OnChar(hWnd, wParam, lParam); return 0;
	case WM_KEYDOWN: OnKeyDown(hWnd, wParam, lParam); return 0;
	}

	return CallWindowProc(m_OldWndProc, hWnd, uMsg, wParam, lParam);
}

void GenRichEditCtrl::OnChar(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	int nChar = (int)wParam;
	//we have to check these possibilities, to know if we are about to delete important text, and thus, to cancel:
	//1.SPECIALTEXT|	- with nChar == VK_BACK
	//2.|SPECIALTEXT	- with nChar == VK_DELETE
	//3.SPE|CIALTEXT	- with nChar == alphanum or VK that changes text.

	if (nChar == VK_BACK || nChar == VK_RETURN/* || nChar == VK_DELETE*/)
		return;
	//if the VK is a moving key (left, right, up, down, end, home) we allow it:
	if (nChar == VK_LEFT || nChar == VK_RIGHT || nChar == VK_UP || nChar == VK_DOWN || nChar == VK_HOME || nChar == VK_END)
	{
		CallWindowProc(m_OldWndProc, hWnd, WM_CHAR, nChar, lParam);
		return;
	}

	CHARRANGE cr;
	SendMessage(hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);
	if (cr.cpMax != cr.cpMin)
	{
		SendMessage(GetParent(hWnd), WM_COMMAND, IDC_REMOVETEXT, 0);
		CallWindowProc(m_OldWndProc, hWnd, WM_KEYDOWN, VK_DELETE, lParam);
		SendMessage(hWnd, WM_CHAR, nChar, lParam);
		return;
	}
	//here, cr.min = cr.max
	deque<SPECIAL>::iterator I;

	if (bFullRights == FALSE)
	{
		CHARFORMAT cf;
		cf.cbSize = sizeof(CHARFORMAT);
		cf.dwMask = CFM_BOLD | CFM_COLOR;
		cf.dwEffects = 0;
		cf.crTextColor = 0;
		SendMessage(hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

		BYTE side = 0;//1 = left; 2 = right; 3 = both.
		for (I = Status.Specials.begin(); I != Status.Specials.end(); I++)
		{
 			if (I->max == cr.cpMax - 1)//SPECIAL|
				side |= 1;
			if (I->min == cr.cpMin)//|SPECIAL
					side |= 2;
			if (I->min <= cr.cpMin - 1 && I->max >= cr.cpMax)//SPE|CIAL
					side = 3;
			if (side == 3) break;
		}

		if ((side | 1) && nChar == VK_BACK) return;
		if (side == 3 && (isalnum(nChar) || !iscntrl(nChar)))
			return;
	

		//all objects that are placed in the left of the caret remain there
		//but all objects that are in the right side of the caret increase in position:
		for (I = Status.Specials.begin(); I != Status.Specials.end(); I++)
		{
			if (I->min >= cr.cpMin)
			{
				I->max++;
				I->min++;
			}
		}
	}

	//now we add the text
	CallWindowProc(m_OldWndProc, hWnd, WM_CHAR, nChar, lParam);
}

void GenRichEditCtrl::OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	int nChar = (int)wParam;
	SHORT kstate = GetKeyState(VK_CONTROL);
	if (nChar != VK_CONTROL && (kstate & 0x80)) return;//if Ctrl key is pressed, we do not check WM_CHAR

	CHARRANGE cr;
	SendMessage(hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);

	//here I have to handle VK_DELETE, VK_BACK, VK_RETURN
	if (!(nChar == VK_DELETE || nChar == VK_BACK || nChar == VK_RETURN))
	{
		CallWindowProc(m_OldWndProc, hWnd, WM_KEYDOWN, wParam, lParam);
		return;
	}

	if (cr.cpMax != cr.cpMin)
	{
		SendMessage(GetParent(hWnd), WM_COMMAND, IDC_REMOVETEXT, 0);
		CallWindowProc(m_OldWndProc, hWnd, WM_KEYDOWN, VK_DELETE, lParam);
		if (nChar == VK_RETURN)
			SendMessage(hWnd, WM_KEYDOWN, VK_RETURN, lParam);
		return;
	}

	//here, cr.min = cr.max
	deque<SPECIAL>::iterator I;

	if (bFullRights == FALSE)
	{
		for (I = Status.Specials.begin(); I != Status.Specials.end(); I++)
		{
			if (I->max == cr.cpMax - 1)//SPECIAL|
			{
				if (nChar == VK_BACK) return;//we do not accept deleting of the special word
			}
			if (I->min == cr.cpMin)//|SPECIAL
			{
				if (nChar == VK_DELETE) return;//we do not accept deleting of the special word
			}
			if (I->min <= cr.cpMin - 1 && I->max >= cr.cpMax)//SPE|CIAL
			{
				return;//we do not allow the call by CR, DELETE, or BACK in the middle
			}
		}

		//all objects that are placed in the left of the caret remain there
		//but all objects that are in the right side of the caret increase in position:
		for (I = Status.Specials.begin(); I != Status.Specials.end(); I++)
		{
			if (I->min >= cr.cpMin)
				if (nChar == VK_DELETE || nChar == VK_BACK)
				{
					I->max--;
					I->min--;
				}
				else
				{
					I->max++;
					I->min++;
				}
		}
	}

	//now we add the text
	CallWindowProc(m_OldWndProc, hWnd, WM_KEYDOWN, nChar, lParam);
}