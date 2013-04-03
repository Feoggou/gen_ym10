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
	m_hWnd = hWnd;
	SetWindowLongPtrW(m_hWnd, GWL_USERDATA, (LONG)this);
	m_OldWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWL_WNDPROC, (LONG)WindowProc);
}

LRESULT CALLBACK GenRichEditCtrl::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	GenRichEditCtrl* pThis = (GenRichEditCtrl*)GetWindowLongPtrW(hWnd, GWL_USERDATA);

	if (pThis)
		switch (uMsg)
		{
		case WM_CHAR: pThis->OnChar((int)wParam, lParam); return 0;
		case WM_KEYDOWN: pThis->OnKeyDown((int)wParam, lParam); return 0;
		}

	return CallWindowProc(m_OldWndProc, hWnd, uMsg, wParam, lParam);
}

void GenRichEditCtrl::OnChar(int nChar, LPARAM lParam)
{
	//trebuiesc verificate urmatoarele posibilitati, sa stim daca vom sterge text important, si astfel, sa anulam:
	//1.SPECIALTEXT|	- cand nChar == VK_BACK
	//2.|SPECIALTEXT	- cand nChar == VK_DELETE
	//3.SPE|CIALTEXT	- cand nChar == alphanum or VK that changes text.

	SHORT kstate = GetKeyState(VK_CONTROL);
	//bitul 0x8000 (32768) este cel mai significant si specifica daca a fost apasata tasta.
	if (kstate & 0x8000) return;

	if (nChar == VK_BACK || nChar == VK_RETURN)
		return;

	CHARRANGE cr;
	SendMessage(m_hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);
	if (cr.cpMax != cr.cpMin)
	{
		//se elimina SPECIAL-ele si se selecteaza textul
		SendMessage(GetParent(m_hWnd), WM_COMMAND, IDC_REMOVETEXT, 0);
		//efectul apasarii tastei Delete
		CallWindowProc(m_OldWndProc, m_hWnd, WM_KEYDOWN, VK_DELETE, lParam);
		//se scrie caracterul nChar
		SendMessage(m_hWnd, WM_CHAR, nChar, lParam);
		return;
	}
	//aici, cr.min = cr.max
	deque<SPECIAL>::iterator I;

	if (bFullRights == FALSE)
	{
		CHARFORMAT cf;
		cf.cbSize = sizeof(CHARFORMAT);
		cf.dwMask = CFM_BOLD | CFM_COLOR;
		cf.dwEffects = 0;
		cf.crTextColor = 0;
		SendMessage(m_hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

		BYTE side = 0;//1 = stanga; 2 = drepta; 3 = amandoua.
		for (I = Status.Specials.begin(); I != Status.Specials.end(); I++)
		{
 			if (I->max == cr.cpMax - 1)//SPECIAL|
			{side = 1; break;}
			if (I->min == cr.cpMin)//|SPECIAL
			{side = 2; break;}
			if (I->min <= cr.cpMin - 1 && I->max >= cr.cpMax)//SPE|CIAL
			{side = 3; break;}
		}

		if ((side == 1) && nChar == VK_BACK) return;
		if (side == 3 && (isalnum(nChar) || !iscntrl(nChar)))
			return;
	

		//toate obiectele care sunt plasate in stanga caret-ului raman acolo
		//dar toate obiectele care sunt in partea dreapta a caret-ului cresc in pozitie:
		for (I = Status.Specials.begin(); I != Status.Specials.end(); I++)
		{
			if (I->min >= cr.cpMin)
			{
				I->max++;
				I->min++;
			}
		}
	}

	//acum adaugam textul
	CallWindowProc(m_OldWndProc, m_hWnd, WM_CHAR, nChar, lParam);
}

void GenRichEditCtrl::OnKeyDown(int nChar, LPARAM lParam)
{
	SHORT kstate = GetKeyState(VK_CONTROL);
	//bitul 0x8000 (32768) este cel mai significant si specifica daca a fost apasata tasta.
	if (nChar != VK_CONTROL && (kstate & 0x8000)) return;//daca Ctrl este apasat, iesim

	CHARRANGE cr;
	SendMessage(m_hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);

	//trebuiesc prelucrate VK_DELETE, VK_BACK, VK_RETURN de OnChar, nu sunt folosite aici
	if (!(nChar == VK_DELETE || nChar == VK_BACK || nChar == VK_RETURN))
	{
		CallWindowProc(m_OldWndProc, m_hWnd, WM_KEYDOWN, nChar, lParam);
		return;
	}

	if (cr.cpMax != cr.cpMin)
	{
		//se elimina SPECIAL-ele si se selecteaza textul
		SendMessage(GetParent(m_hWnd), WM_COMMAND, IDC_REMOVETEXT, 0);
		//efectul apasarii tastei Delete
		CallWindowProc(m_OldWndProc, m_hWnd, WM_KEYDOWN, VK_DELETE, lParam);
		//daca tasta apasata a fost Enter, se prelucreaza din nou in OnKeyDown, acum ca
		//nu mai este nici o selectie (aici din EM_EXGETSEL se obtine cr.cpMax == cr.cpMin)
		if (nChar == VK_RETURN)
			SendMessage(m_hWnd, WM_KEYDOWN, VK_RETURN, lParam);
		return;
	}

	//aici, cr.min = cr.max
	deque<SPECIAL>::iterator I;

	if (bFullRights == FALSE)
	{
		for (I = Status.Specials.begin(); I != Status.Specials.end(); I++)
		{
			if (I->max == cr.cpMax - 1)//SPECIAL|
			{
				if (nChar == VK_BACK) return;//nu acceptam stergerea cuvantului special
			}
			if (I->min == cr.cpMin)//|SPECIAL
			{
				if (nChar == VK_DELETE) return;//nu acceptam stergerea cuvantului special
			}
			if (I->min <= cr.cpMin - 1 && I->max >= cr.cpMax)//SPE|CIAL
			{
				return;//nu acceptam CR, DELETE sau BACK in mijloc
			}
		}

		//toate obiectele care sunt plasate in stanga caret-ului raman acolo
		//dar toate obiectele care sunt in partea dreapta a caret-ului cresc in pozitie:
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

	//acum adaugam textul
	CallWindowProc(m_OldWndProc, m_hWnd, WM_KEYDOWN, nChar, lParam);
}