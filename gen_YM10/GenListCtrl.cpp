#include "GenListCtrl.h"
#include "RepetitionDlg.h"
#include "GenDll.h"
#include "Tools.h"

extern CGenDll genDll;

WNDPROC GenListCtrl::m_OldWndProc;
WNDPROC	GenListCtrl::OldCheckProc;

GenListCtrl::GenListCtrl(void)
{
	m_nTextHeight = 0;
	m_nVBarPos = 0;

	//daca nu exista clasa aceasta de fereastra se creeaza (inregistreaza).
	WNDCLASS wndclass;
	if (FALSE == GetClassInfo(genDll.m_hInstance, L"GENLISTCTRL", &wndclass))
	{
		wndclass.style = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc = WindowProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = genDll.m_hInstance;
		wndclass.hIcon = NULL;
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = L"GENLISTCTRL";

		if (FALSE == RegisterClass(&wndclass))
		{
			DisplayError(0);
		}
	}
}

GenListCtrl::~GenListCtrl(void)
{
}

LRESULT CALLBACK GenListCtrl::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		//daca GenListCtrl primeste focusul, trimite focusul la prima linie din CListSide
		case WM_SETFOCUS:
		{
			GenListCtrl* pThis = (GenListCtrl*)GetWindowLongPtrW(hWnd, GWL_USERDATA);
			SetFocus(pThis->m_Table.m_Lines.front().edSecond.m_hWnd);
		}
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK GenListCtrl::CheckProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		//daca se bifeaza/debifeaza caseta de bifare
	case BM_SETCHECK:
		{
			HWND hEdit = GetDlgItem(GetParent(hWnd), IDC_EDIT_NOTIMES);
			//daca se bifeaza (nu se va opri repetarea), atunci nu mai poate fi utilizata caseta de editare
			//si dispare orice text care era afisat pe ea
			if (wParam == BST_CHECKED)
			{
				EnableWindow(hEdit, false);
				SetWindowTextW(hEdit, L"");
			}
			//daca se debifeaza (se va opri repetarea dupa un nr. de repetari), atunci va fi utilizata caseta de
			//editare, iar daca nu exista nimic afisat in caseta, se afiseaza "1"
			else
			{
				EnableWindow(hEdit, true);
				WCHAR wsText[7];
				GetWindowTextW(hEdit, wsText, 7);
				if (wcslen(wsText) == 0)
					SetWindowTextW(GetDlgItem(GetParent(hWnd), IDC_EDIT_NOTIMES), L"1");
			}
		}
		break;//BM_SETCHECK
	}

	//se apeleaza vechea functie de prelucrare a mesajelor
	return CallWindowProc(OldCheckProc, hWnd, uMsg, wParam, lParam);
}

void GenListCtrl::Create(HWND hWnd, bool bIsLyrics)
{
	//initializari din parametrii
	m_bIsLyrics = bIsLyrics;
	m_hWnd = hWnd;
	SetWindowLongPtrW(m_hWnd, GWL_USERDATA, (LONG_PTR)this);

	//se preia un handle la contextul dispozitivului (Device Context) pentru a putea desena in control
	HDC hDC = GetDC(m_hWnd);
	
	//se creeaza un font bazat pe fontul 'default' GUI
	LOGFONT lf;
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	GetObject(hFont, sizeof(LOGFONT), &lf);
	lf.lfWeight = FW_SEMIBOLD;
	lf.lfQuality = PROOF_QUALITY;
	HFONT hHeader = CreateFontIndirect(&lf);

	//CLIENT
	GetClientRect(m_hWnd, &m_rClient);
	//ANTET
	m_rHeader = m_rClient;
	//MulDiv calculeaza param1 * param2 / param3
	//dimensiunea em(lf.lfHeight) pentru font se calculeaza dupa formula
	//emSize = - ptSize * GetDeviceCaps(hDC, LOGPIXELSY) / 72
	//deci dimensiunea ptSize (care ne trebuie) = abs(72 * emSize / GetDeviceCaps(hDC, LOGPIXELSY)).
	//aici se calculeaza inaltimea, in pixeli, a fontului
	m_rHeader.bottom = abs(MulDiv(72, lf.lfHeight, GetDeviceCaps(hDC, LOGPIXELSY)));
	m_nTextHeight = m_rHeader.bottom;
	//noua ne trebuie o dimenisune mai mare decat a fontului, astfel ca textul sa fie afisat in centru
	//facem inaltimea fontului mai mare de 8/3 ori
	m_rHeader.bottom *= 8; m_rHeader.bottom /= 3;
	int nVBarPos;
	//cream antetul
	m_Header.Create(m_hWnd, m_rHeader, hHeader, nVBarPos);

	//TABEL/LISTA
	m_rTable.left = 0;
	m_rTable.right = m_rClient.right;
	m_rTable.top = m_rHeader.bottom;
	m_rTable.bottom = m_rClient.bottom;
	//cream tabelul/lista
	m_Table.Create(m_hWnd, m_rTable, hFont, nVBarPos, m_nTextHeight * 2, m_bIsLyrics);

	//nu mai avem nevoie de hDC
	ReleaseDC(hWnd, hDC);

	//se seteaza noua functie de prelucrare a mesajelor pentru GenListCtrl
	m_OldWndProc = (WNDPROC)SetWindowLongPtrW(hWnd, GWL_WNDPROC, (LONG_PTR)WindowProc);

	//se seteaza noua functie de prelucrare a mesajelor pentru checkbox-ul IDC_CHECK_NOCOUNTER
	HWND hCheckBtn = GetDlgItem(GetParent(hWnd), IDC_CHECK_NOCOUNTER);
	OldCheckProc = (WNDPROC)SetWindowLongPtrW(hCheckBtn, GWL_WNDPROC, (LONG_PTR)CheckProc);
}
