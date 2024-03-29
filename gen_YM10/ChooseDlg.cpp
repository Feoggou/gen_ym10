#include "GenDll.h"
#include "ChooseDlg.h"
#include "resource.h"
#include "AboutDlg.h"
#include "Tools.h"

extern CGenDll			genDll;
ID CChooseDlg::m_SelID;

CChooseDlg::CChooseDlg(void)
{
	m_bIsLink = FALSE;
	m_hDlg = NULL;
}

CChooseDlg::~CChooseDlg(void)
{
}

void CChooseDlg::DoModal(HWND hParent)
{
	//se creeaza o singura data
	if (genDll.m_hChooseDlg) return;

	INT_PTR nResult = DialogBoxParam(genDll.m_hInstance, MAKEINTRESOURCE(IDD_DIALOG_CHOOSE), hParent, DialogProc, (LPARAM)this);
	if (nResult == -1)
		DisplayError(0);
}

INT_PTR CALLBACK CChooseDlg::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//se creeaza un singur obiect de clasa CChooseDlg. deci, pastram pointer la obiectul lui.
	static CChooseDlg* pThis = NULL;

	if (pThis == NULL)
		if (uMsg == WM_INITDIALOG)
		{
			pThis = (CChooseDlg*)lParam;
			pThis->m_hDlg = hDlg;
		}
		else return 0;

	switch (uMsg)
	{
	case WM_DESTROY: pThis = 0; genDll.m_hChooseDlg = 0; break;
	case WM_CLOSE: EndDialog(hDlg, IDCANCEL); break;
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK: pThis->OnOk(); break;
			case IDCANCEL:EndDialog(hDlg, IDCANCEL); break;
			}
		}
		break;//case WM_COMMAND
	case WM_INITDIALOG: pThis->OnInitDialog();break;

	//a fost adaugat un status nou. trebuie actualizata lista cu noul status.
	case WM_UPDATELIST:
		{
			//se adauga un nou item: itemdata = numarul fisierului
			int nLast = SendMessage(GetDlgItem(hDlg, IDC_LIST_OLDMESSAGES), LB_GETCOUNT, 0, 0);
			SendMessage(GetDlgItem(hDlg, IDC_LIST_OLDMESSAGES), LB_ADDSTRING, 0, lParam);
			SendMessage(GetDlgItem(hDlg, IDC_LIST_OLDMESSAGES), LB_SETITEMDATA, nLast, (LPARAM)genDll.m_StatusMsgs[nLast].dFileNr);
		}
		break;//WM_UPDATELIST

	//mesaj trimis pentru actualizarea listei de ID-uri de messenger online
	case WM_UPDATEIDS: pThis->OnUpdateIDs();
	}

	return 0;
}

void CChooseDlg::OnAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal(genDll.m_hChooseDlg);
}

void CChooseDlg::OnMessID()
{
	_ASSERT(genDll.m_OnlineIDs.size());

	//se creeaza meniul popup
	HMENU hMenu = CreatePopupMenu();
		
	deque<ID>::iterator I;
	int i = 0;
	for (I = genDll.m_OnlineIDs.begin(); I != genDll.m_OnlineIDs.end(); I++, i++)
		AppendMenu(hMenu, MF_STRING, i + 1, I->wsID.c_str());
		
	HWND hLink = GetDlgItem(genDll.m_hChooseDlg, IDH_MESSID);
	RECT rect;
	GetWindowRect(hLink, &rect);

	int nResult = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD, rect.left, rect.bottom, 0, hLink, 0);
	//daca rezultatul e 0, inseamna ca nu a fost selectat nimic din meniu.
	if (nResult == 0) return;
	if (nResult >= 1)
	{
		m_SelID.wsID = genDll.m_OnlineIDs[nResult - 1].wsID;
		m_SelID.hWnd = genDll.m_OnlineIDs[nResult - 1].hWnd;
	}

	//dupa ce a fost ales din meniu noul ID, se actualizeaza id-ul online si link-ul IDH_MESSID 
	SendMessage(genDll.m_hChooseDlg, WM_UPDATEIDS, 0, 0);
	HWND hMessID = GetDlgItem(genDll.m_hChooseDlg, IDH_MESSID);
	RedrawWindow(hMessID, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
}

int CChooseDlg::OnInitDialog()
{
	genDll.m_hChooseDlg = m_hDlg;

	//se creeaza cele doua hyperlink-uri
	m_AboutLink.Create(m_hDlg, IDH_ABOUT, OnAbout, true);
	m_IDLink.Create(m_hDlg, IDH_MESSID, OnMessID, false);

	//LISTBOX
	m_ListBox.Create(GetDlgItem(m_hDlg, IDC_LIST_OLDMESSAGES), m_hDlg);

	//se initializeaza caseta de dialog din valorile din registrii:
	SendMessage(GetDlgItem(m_hDlg, IDC_ENABLECHANGE), BM_SETCHECK, genDll.GetValueUseTool(),0);
	SendMessage(GetDlgItem(m_hDlg, IDC_SHOWTRAY), BM_SETCHECK, genDll.GetValueShowTray(),0);

	//se initializeaza listbox-ul din genDll.m_StatusMsgs
	deque<STATUS>::iterator I;
	int i = 0;
	for (I = genDll.m_StatusMsgs.begin(); I != genDll.m_StatusMsgs.end(); I++, i++)
	{
		SendMessage(GetDlgItem(m_hDlg, IDC_LIST_OLDMESSAGES), LB_ADDSTRING, 0, (LPARAM)I->wsText.c_str());
		SendMessage(GetDlgItem(m_hDlg, IDC_LIST_OLDMESSAGES), LB_SETITEMDATA, i, (LPARAM)I->dFileNr);
	}

	if (genDll.m_dStatusSelected >= (int)genDll.m_StatusMsgs.size())
		genDll.m_dStatusSelected = -1;

	//se selecteaza un element din lista
	SendMessage(GetDlgItem(m_hDlg, IDC_LIST_OLDMESSAGES), LB_SETCURSEL, genDll.m_dStatusSelected, 0);

	//se actualizeaza lista de id-uri si m_IDLink.
	OnUpdateIDs();

	return 0;
}

void CChooseDlg::OnOk()
{
	//salvam modificarile
	genDll.m_bUseTool = (0 !=SendMessage(GetDlgItem(m_hDlg, IDC_ENABLECHANGE), BM_GETCHECK, 0, 0));
	genDll.m_bShowTray = (0 != SendMessage(GetDlgItem(m_hDlg, IDC_SHOWTRAY), BM_GETCHECK, 0, 0));
	genDll.m_dStatusSelected = SendMessage(GetDlgItem(m_hDlg, IDC_LIST_OLDMESSAGES), LB_GETCURSEL, 0, 0);

	if (genDll.m_bUseTool && genDll.m_dStatusSelected == -1)
	{
		int nCount = SendMessage(GetDlgItem(m_hDlg, IDC_LIST_OLDMESSAGES), LB_GETCOUNT, 0, 0);
		if (nCount > 0)
			MessageBoxW(genDll.m_hWinamp, L"Pentru a putea utiliza acest plugin pentru a schimba statusul la Yahoo! Messenger trebuie să ai un status selectat.", L"Atenție!", MB_ICONWARNING);
		else
			MessageBoxW(genDll.m_hWinamp, L"Pentru a putea utiliza acest plugin pentru a schimba statusul la Yahoo! Messenger trebuie să ai un status selectat.\
\nApasă click-dreapta în listă și alege \"Adaugă\" pentru a adăuga un status.", L"Atenție!", MB_ICONWARNING);
		return;
	}
	
	genDll.m_ChosenID.wsID = m_SelID.wsID;
	//id-urile de mess reale nu au ' ', deci, daca ' ' este gasit (adica pe pozitie diferita decat npos), 
	//atunci avem "" ca sir selectat
	if (m_SelID.wsID.find(' ', 0) != string::npos)
		m_SelID.wsID = L"";

	genDll.m_ChosenID.wsID = m_SelID.wsID;

	genDll.m_nTime = -1;
	//in CGenDll se trimite un nou status la Y!M doar daca cel construit difera de m_wsDisplayedStatus.
	//ne asiguram ca sunt diferite
	genDll.m_wsDisplayedStatus = L"";

	EndDialog(m_hDlg, IDOK);
}

void CChooseDlg::OnUpdateIDs()
{
	HWND hWnd = GetDlgItem(m_hDlg, IDH_MESSID);

	int size = genDll.m_OnlineIDs.size();
	int nLen = GetWindowTextLength(hWnd);
	nLen++;
	wchar_t* wstr = new wchar_t[nLen];
	GetWindowText(hWnd, wstr, nLen);
	//consideram ca id-ul selectat nu este online
	genDll.m_bSelOnline = false;

	//m_OnlineIDs a fost gasit in CGenDll::OnTimer si e lista cu id-urile online.
	//m_SelID este ID-ul selectat din meniul link-ului IDH_MESSID. valoarea lui este preluata din wstr
	for (int i = 0; i < size; i++)
		if (wcscmp(genDll.m_OnlineIDs[i].wsID.c_str(), m_SelID.wsID.c_str()) == 0)
		{
			//daca s-a gasit id-ul din IDH_MESSID stocat printre id-urile gasite online,
			//atunci setam ca id-ul selectat este online.
			genDll.m_bSelOnline = true;
			m_SelID.hWnd = genDll.m_OnlineIDs[i].hWnd;
		}

	//daca id-ul ce a fost selectat nu mai este online
	if (!genDll.m_bSelOnline)
	{
		m_SelID.hWnd = 0;
		m_SelID.wsID = L"";
	}

		//daca avem ID selectat (a fost candva selectat), atunci este online (altfel l-am resetat)
	if (m_SelID.wsID.length() > 0)
	{	
		//daca id-ul afisat acum (sau "Nici un ID ...") este altul decat cel stocat (care sigur e un ID valid)...
		if (wcscmp(wstr, m_SelID.wsID.c_str()) != 0)
			SetDlgItemTextW(m_hDlg, IDH_MESSID, m_SelID.wsID.data());

		if (m_bIsLink == false && size > 1)
		{
			m_bIsLink = true;
			m_IDLink.SetLinkState(true);
		}
		else if (size <= 1)
		{
			m_bIsLink = false;
			m_IDLink.SetLinkState(false);
		}

		delete[] wstr;
		return;
	}

	//daca nu avem ID selectat stocat
	if (m_SelID.wsID.length() == 0)
	{
		//daca nu este nici un ID online
		if (size < 1)
		{
			//daca nu este afisat ca "Nici un ID ...", atunci il afisam
			if (wcscmp(wstr, L"Nici un ID (nu ești logat pe Yahoo! Messenger)") != 0)
				SetDlgItemTextW(m_hDlg, IDH_MESSID, L"Nici un ID (nu ești logat pe Yahoo! Messenger)");
			//daca era link, il schimbam in static
			if (m_bIsLink == true)
			{
				m_IDLink.SetLinkState(false);
				m_bIsLink = false;
			}
		}
		else
		{
			//sunt unul sau mai multe id-uri online, insa nici unul nu este afisat. dak nu este afisat 
			//"Niciun ID (Apasa..." atunci afiseaza!
			if (wcscmp(wstr, L"Nici un ID (Apasă click pentru a schimba)") != 0)
				SetDlgItemTextW(m_hDlg, IDH_MESSID, L"Nici un ID (Apasă click pentru a schimba)");
			if (m_bIsLink == false)
			{
				m_IDLink.SetLinkState(true);
				m_bIsLink = true;
			}
		}
	}
	delete[] wstr;
}