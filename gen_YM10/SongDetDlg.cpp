#include "SongDetDlg.h"
#include "Tools.h"

extern CGenDll genDll;
WNDPROC OldProgrProc;

//functie pentru prelucrarea mesajelor casetei de bifare IDC_PROGRESS
LRESULT CALLBACK NewProgrProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

CSongDetDlg::CSongDetDlg(SONGDET songdet)
{
	m_SongDet = songdet;
}

CSongDetDlg::~CSongDetDlg(void)
{
}

INT_PTR CSongDetDlg::DoModal(HWND hParent)
{
	INT_PTR nResult = DialogBoxParam(genDll.m_hInstance, MAKEINTRESOURCE(IDD_SONGINFO), hParent, &CSongDetDlg::DialogProc, (LPARAM)this);
	if (nResult == -1)
		DisplayError(0);

	return nResult;
}

INT_PTR CALLBACK CSongDetDlg::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//poate exista numai o singura caseta de dialog de acest tip la un moment dat.
	//deci, putem folosi o variabila statica.
	static CSongDetDlg* pThis = NULL;

	if (pThis == NULL)
		if (uMsg == WM_INITDIALOG)
		{
			pThis = (CSongDetDlg*)lParam;
			pThis->m_hDlg = hDlg;
		}
		else return 0;

	switch (uMsg)
	{
	case WM_DESTROY: pThis = 0; break;
	case WM_CLOSE: EndDialog(hDlg, IDCANCEL); break;
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK: pThis->OnOk(); break;
			case IDCANCEL: EndDialog(hDlg, IDCANCEL); break;
			}
		}
		break;//case WM_COMMAND

	case WM_INITDIALOG: pThis->OnInitDialog();break;
	}

	return 0;
}

void CSongDetDlg::OnOk()
{
	m_SongDet = 0;
	//fiecare caseta de bifare in parte
	if (BST_CHECKED == SendMessage(GetDlgItem(m_hDlg, IDC_ARTIST), BM_GETCHECK, 0, 0))
		m_SongDet |= artist;
	if (BST_CHECKED == SendMessage(GetDlgItem(m_hDlg, IDC_ALBUM), BM_GETCHECK, 0, 0))
		m_SongDet |= album;
	if (BST_CHECKED == SendMessage(GetDlgItem(m_hDlg, IDC_SONG), BM_GETCHECK, 0, 0))
		m_SongDet |= song;
	if (BST_CHECKED == SendMessage(GetDlgItem(m_hDlg, IDC_PROGRESS), BM_GETCHECK, 0, 0))
		m_SongDet |= progress;
	if (BST_CHECKED == SendMessage(GetDlgItem(m_hDlg, IDC_LENGTH), BM_GETCHECK, 0, 0))
		m_SongDet |= length;
	if (BST_CHECKED == SendMessage(GetDlgItem(m_hDlg, IDC_LINKLYR), BM_GETCHECK, 0, 0))
		m_SongDet |= lyrics;

	if (m_SongDet & progress)
		if (BST_CHECKED == SendMessage(GetDlgItem(m_hDlg, IDC_STYLE), BM_GETCHECK, 0, 0))
			m_SongDet |= percent;

	if (m_SongDet == 0)
	{
		MessageBoxW(m_hDlg, L"Trebuie să bifezi cel puțin o casetă dacă vrei să adaugi detalii despre melodie.", L"Informații despre melodie", MB_ICONEXCLAMATION); 
		return;
	}

	EndDialog(m_hDlg, IDOK);
}

LRESULT CALLBACK NewProgrProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case BM_SETCHECK:
		{
			HWND h1 = GetDlgItem(GetParent(hWnd), IDC_STYLE);
			HWND h2 = GetDlgItem(GetParent(hWnd), IDC_RADIO2);
			//daca IDC_PROGRESS e bifat, atunci va fi disponibila alegerea stilului;
			//de asemenea se va bifa primul stil
			if (wParam == BST_CHECKED)
			{
				EnableWindow(h1, true);
				EnableWindow(h2, true);
				SendMessage(h1, BM_SETCHECK, BST_CHECKED, 0);
			}
			else
			{
				EnableWindow(h1, false);
				EnableWindow(h2, false);
				SendMessage(h1, BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessage(h2, BM_SETCHECK, BST_UNCHECKED, 0);
			}
		}
	}

	return CallWindowProc(OldProgrProc, hWnd, uMsg, wParam, lParam);
}

void CSongDetDlg::OnInitDialog()
{
	//daca s-a apasat butonul "editare" in caseta de dialog de creare a statusului, atunci m_SongDet != 0
	if (m_SongDet)
	{
		int x = (m_SongDet & artist) > 0;
		SendMessage(GetDlgItem(m_hDlg, IDC_ARTIST), BM_SETCHECK, x, 0);
		x = (m_SongDet & album) > 0;
		SendMessage(GetDlgItem(m_hDlg, IDC_ALBUM), BM_SETCHECK, x, 0);
		x = (m_SongDet & song) > 0;
		SendMessage(GetDlgItem(m_hDlg, IDC_SONG), BM_SETCHECK, x, 0);
		x = (m_SongDet & progress) > 0;
		SendMessage(GetDlgItem(m_hDlg, IDC_PROGRESS), BM_SETCHECK, x, 0);
		x = (m_SongDet & length) > 0;
		SendMessage(GetDlgItem(m_hDlg, IDC_LENGTH), BM_SETCHECK, x, 0);
		x = (m_SongDet & lyrics) > 0;
		SendMessage(GetDlgItem(m_hDlg, IDC_LINKLYR), BM_SETCHECK, x, 0);
		x = (m_SongDet & percent) > 0;

		//daca a fost bifat si info::progress data trecuta, vor fi disponibile casetele de bifare pt stil
		//si se bifeaza stilul ales
		if (m_SongDet & progress)
		{
			HWND h1 = GetDlgItem(m_hDlg, IDC_STYLE);
			HWND h2 = GetDlgItem(m_hDlg, IDC_RADIO2);

			EnableWindow(h1, true);
			EnableWindow(h2, true);
			SendMessage(h1, BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(h2, BM_SETCHECK, BST_UNCHECKED, 0);

			SendMessage(GetDlgItem(m_hDlg, IDC_STYLE), BM_SETCHECK, x, 0);
			SendMessage(GetDlgItem(m_hDlg, IDC_RADIO2), BM_SETCHECK, !x, 0);
		}
	}

	//daca se creeaza un songinfo nou
	else
	{
		//se bifeaza artistul si melodia.
		SendMessage(GetDlgItem(m_hDlg, IDC_ARTIST), BM_SETCHECK, BST_CHECKED, 0);
		SendMessage(GetDlgItem(m_hDlg, IDC_SONG), BM_SETCHECK, BST_CHECKED, 0);
	}

	//se seteaza noua functie de prelucrare a mesajelor
	OldProgrProc = (WNDPROC)SetWindowLongPtrW(GetDlgItem(m_hDlg, IDC_PROGRESS), GWL_WNDPROC, (LONG_PTR)NewProgrProc);
}

