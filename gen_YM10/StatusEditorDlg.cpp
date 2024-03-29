#include "StatusEditorDlg.h"
#include "GenDll.h"
#include "ClockDlg.h"
#include "RepetitionDlg.h"
#include "SongDetDlg.h"
#include "LyricsDlg.h"
#include "Tools.h"

#include "resource.h"

extern CGenDll			genDll;
HMODULE					hModule;
BOOL					bFullRights = false;
STATUS					Status;

CStatusEditorDlg::CStatusEditorDlg()
{
	hModule = LoadLibrary(L"RICHED20.DLL");
}

CStatusEditorDlg::~CStatusEditorDlg(void)
{
	FreeLibrary(hModule);

	//trebuie sa sterg continutu ca poate se mai adauga elemente, si sa nu le amestec.
	//nu sterg continutul din pointeri ca inca mai am nevoie in genDll.m_StatusMsgs;
	Status.Statuses.clear();
	Status.Specials.clear();
}

INT_PTR CStatusEditorDlg::DoModal(HWND hParent)
{
	if (genDll.m_hStatusEditorDlg) return IDCANCEL;

	INT_PTR nResult = DialogBoxParam(genDll.m_hInstance, MAKEINTRESOURCE(IDD_STATUS_EDITOR), hParent, DialogProc, (LPARAM)this);
	genDll.m_hStatusEditorDlg = NULL;
	if (nResult == -1)
		DisplayError(0);

	return nResult;
}

INT_PTR CALLBACK CStatusEditorDlg::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//va exista un singur obiect de acest tip odata. deci folosim o variabila statica.
	static CStatusEditorDlg* pThis = NULL;

	if (pThis == NULL)
		if (uMsg == WM_INITDIALOG)
		{
			pThis = (CStatusEditorDlg*)lParam;
			pThis->m_hDlg = hDlg;
		}
		else return 0;

	switch (uMsg)
	{
	case WM_DESTROY: pThis = 0; genDll.m_hStatusEditorDlg = 0; break;
		case WM_CLOSE: EndDialog(hDlg, IDCANCEL); break;
		case WM_COMMAND:
			{
				switch (LOWORD(wParam))
				{
				case IDOK: pThis->OnOk(); break;
				case IDCANCEL:EndDialog(hDlg, IDCANCEL); break;
				case IDC_ADDTEXT: pThis->OnAddText((HWND)lParam); return 0;
				case IDC_REMOVETEXT: 
					{
						pThis->OnRemoveText();
						if (lParam)//daca este trimis de butonul "Elimina"
						{
							SendMessage(GetDlgItem(hDlg, IDC_RICHEDIT), WM_KEYDOWN, VK_DELETE, 0);
							SetFocus(GetDlgItem(hDlg, IDC_RICHEDIT));
						}
					}
					return 0;

				case IDC_EDITTEXT: pThis->OnEditText(); return 0;
				}
			}
			break;//case WM_COMMAND

		case WM_INITDIALOG: pThis->OnInitDialog(); return TRUE;

		case WM_NOTIFY:
			{
				NMHDR* pNMHDR = (NMHDR*)lParam;
				//daca s-a schimbat selectia (sau, pozitia caret-ului) in RichEdit
				if (pNMHDR->code == EN_SELCHANGE)
				{
					//atunci, avem nevoie de o structura (struct) mai mare, anume, de SELCHANGE:
					SELCHANGE*	pSel = (SELCHANGE*)lParam;
					CHARRANGE cr = pSel->chrg;
					//trebuie sa vedem daca este selectat text special.

					//daca este text selectat, butonul Remove este valid. altfel, nu se poate folosi
					bool bRemove = false;
					if (cr.cpMin != cr.cpMax)
						bRemove = true;

					bool bEdit = false;
					deque<SPECIAL>::iterator I;
					//daca selectia (sau caret-ul) este in interiorul unui SPECIAL, afara de Lyrics,
					//sau, este selectat intregul, si numai un SPECIAL, altul decat Lyrics, atunci butonul
					//Editare va fi valid. altfel, nu va putea fi folosit.
					for (I = Status.Specials.begin(); I != Status.Specials.end(); I++)
					{
						if (cr.cpMin == cr.cpMax)
						{
							//daca nu avem selectie, iar caret-ul este in interiorul unui SPECIAL
							if (cr.cpMin > I->min && cr.cpMin <= I->max)
							{
								//Versurile nu se pot edita
								if (I->type == SPECIAL::lyrics)
									bRemove = true;
								else
									bEdit = bRemove = true;
								break;
							}
						}
						else
						{
							//daca avem selectie, ca sa se poata editeze SPECIAL-ul, trebuie sa fie selectia
							//limitata la textul special.
							if (cr.cpMin >= I->min && cr.cpMax - 1 <= I->max && I->type != SPECIAL::lyrics)
							{
								//Versurile(lyrics) nu se pot edita
								if (I->type != SPECIAL::lyrics)
									bEdit = true;
								break;
							}
						}
					}
					EnableWindow(GetDlgItem(hDlg, IDC_EDITTEXT), bEdit);
					EnableWindow(GetDlgItem(hDlg, IDC_REMOVETEXT), bRemove);
				}
			}
			break;
	}

	return 0;
}

void CStatusEditorDlg::OnInitDialog()
{
	genDll.m_hStatusEditorDlg = m_hDlg;

	//cream controlul RichEdit
	richEdit.Create(GetDlgItem(m_hDlg, IDC_RICHEDIT));

	//controlul RichEdit va trimite mesajul EN_SELCHANGE parintelui
	SendMessage(richEdit.m_hWnd, EM_SETEVENTMASK, 0, ENM_SELCHANGE);
}

void CStatusEditorDlg::OnAddText(HWND hWnd)
{
	//hWnd = handle la butonul "Adauga"
	//cream meniul
	HMENU hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, ID_ADDCLOCKTEXT, L"Ceas");
	AppendMenu(hMenu, MF_STRING, ID_ADDREPTEXT, L"Repetiție");
	AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(hMenu, MF_STRING, ID_ADDSONGDETTEXT, L"Detalii despre melodie");
	AppendMenu(hMenu, MF_STRING, ID_ADDLYRICSTEXT, L"Versuri pentru melodie");

	RECT rect;
	GetWindowRect(hWnd, &rect);

	//afisam meniul
	int nResult = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON,
	rect.left, rect.bottom - 1, 0, hWnd, 0);

	hWnd = GetDlgItem(m_hDlg, IDC_RICHEDIT);
	
	switch (nResult)
	{
	case 0: return;
	case ID_ADDCLOCKTEXT: OnAddClockText(hWnd, 0); break;
	case ID_ADDREPTEXT: OnAddRepText(hWnd, 0); break;
	case ID_ADDSONGDETTEXT: OnAddSongDetText(hWnd, 0); break;
	case ID_ADDLYRICSTEXT: OnAddSongLyricsText(hWnd); break;
	}

	SetFocus(hWnd);
}

void CStatusEditorDlg::OnAddClockText(HWND hWnd, CClockDlg::CLOCK* pSpec)
{
	//hWnd = RichEdit
	CClockDlg::CLOCK* pClock;
	SPECIAL special;

	//Cream caseta de dialog Ceas
	CClockDlg clockDlg(pSpec);
	//daca se salveaza modificarile
	if (IDOK == clockDlg.DoModal(m_hDlg))
	{
		//initializarea
		ClearSelected(hWnd);
		pClock = &clockDlg.m_Clock;

		//gasim selectia
		CHARRANGE cr;
		SendMessage(hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);
		special.min = cr.cpMin;

		//aici, cr.cpMin = cr.cpMax
		SendMessage(hWnd, EM_EXSETSEL, 0, (LPARAM)&cr);

		//formatul textului si culoarea
		COLORREF clr = RGB(255, 0, 0);
		CHARFORMAT cf;
		cf.cbSize = sizeof(CHARFORMAT);
		cf.dwMask = CFM_BOLD | CFM_COLOR;
		cf.dwEffects = CFE_BOLD;
		cf.crTextColor = clr;
		SendMessage(hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

		//wsOutput = textul format din componetele ale lui CLOCK.
		wstring wsOutput = clockDlg.m_Clock.wsTextBefore;
		WCHAR wsNow[6] = L"";
		//ora
		if (clockDlg.m_Clock.dHours)
		{
			wsprintf(wsNow, L"%dh", clockDlg.m_Clock.dHours);
			wsOutput += wsNow;
		}

		//minutul
		if (clockDlg.m_Clock.dMins)
		{
			wsprintf(wsNow, L"%dm", clockDlg.m_Clock.dMins);
			if (clockDlg.m_Clock.dHours)
				wsOutput += ' ';
			wsOutput += wsNow;
		}

		//scriem textul, fara ca controlul RichEdit sa decida pentru sine culoarea si sa faca schimbari
		//referitoare la SPECIAL-urile care exista (pozitia lor)
		bFullRights = TRUE;
		for (UINT i = 0; i < wsOutput.length(); i++)
		{
			SendMessage(hWnd, WM_CHAR, wsOutput[i], 0);
		}

		bFullRights = FALSE;

		cf.dwEffects = 0;
		cf.crTextColor = 0;
		SendMessage(hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

		//acum trebuie sa actualizam pozitiile celorlalte SPECIALe 
		deque<SPECIAL>::iterator I;
		for (I = Status.Specials.begin(); I != Status.Specials.end(); I++)
		{
			if (I->min >= cr.cpMin)
			{
				I->max += wsOutput.length();
				I->min += wsOutput.length();
			}
		}

		//cream un SPECIAL bazat pe aceste date si il adaugam in lista de SPECIAL-e
		special.type = SPECIAL::clock;
		//alocam spatiu pentru un element (SPECIAL), acest special fiind un Clock
		special.pElem = new CClockDlg::CLOCK;
		//cream un pointer de tip CLOCK pentru a prelucra datele din pElem sub forma de Clock
		CClockDlg::CLOCK* pointer = (CClockDlg::CLOCK*)special.pElem;
		//scriem valorile
		pointer->dHours = pClock->dHours;
		pointer->dMins = pClock->dMins;
		pointer->wsTextBefore = pClock->wsTextBefore;
		pointer->wsTextAfter = pClock->wsTextAfter;

		//pozitiile min si max din status
		int len = wsOutput.length();
		special.max = special.min + len - 1;
		//aici doar initializam
		special.pos = 0;
		//adaucam in lista de SPECIAL-e
		Status.Specials.push_back(special);
	}
}

void CStatusEditorDlg::OnAddRepText(HWND hWnd, GenListCtrl::REPETE* pSpec)
{
	GenListCtrl::REPETE* pRepete;
	SPECIAL special;

	//se creeaza caseta de dialog repetitie
	CRepetitionDlg repDlg(pSpec);
	if (IDOK == repDlg.DoModal(m_hDlg))
	{
		ClearSelected(hWnd);
		pRepete = &repDlg.m_list.m_Repetition;

		//gasim selectia
		CHARRANGE cr;
		SendMessage(hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);
		special.min = cr.cpMin;

		//aici, cr.cpMin = cr.cpMax
		SendMessage(hWnd, EM_EXSETSEL, 0, (LPARAM)&cr);

		//formatul textului si culoarea
		COLORREF clr = RGB(255, 0, 0);
		CHARFORMAT cf;
		cf.cbSize = sizeof(CHARFORMAT);
		cf.dwMask = CFM_BOLD | CFM_COLOR;
		cf.dwEffects = CFE_BOLD;
		cf.crTextColor = clr;

		SendMessage(hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

		//wsOutput = textul format din componetele ale lui CLOCK.
		wstring wsOutput;
		//"R:" + primele max 20 caractere din primul vers, daca erau mai multe sau sunt mai multe
		//versuri, urmeaza "..."
		wsOutput = L"R:" + pRepete->Verses.front().wsVerse.substr(0, 20);
		if (pRepete->Verses.size() > 1 || pRepete->Verses.front().wsVerse.length() > 20)
			wsOutput += L"...";
		//daca se repeta textul, se afiseaza de cate ori (ex: x3)
		if (pRepete->nRepCnt > -1)
		{
			wsOutput += L"x";
			wchar_t ws[3];
			_itow_s(pRepete->nRepCnt, ws, 3, 10);
			wsOutput += ws;
		}

		//scriem textul in RichEdit
		bFullRights = TRUE;
		for (UINT i = 0; i < wsOutput.length(); i++)
		{
			SendMessage(hWnd, WM_CHAR, wsOutput[i], 0);
		}

		bFullRights = FALSE;

		//resetam formatul in RichEdit
		cf.dwEffects = 0;
		cf.crTextColor = 0;
		SendMessage(hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

		//acum trebuie sa actualizam pozitiile celorlalte SPECIALe 
		deque<SPECIAL>::iterator I;
		for (I = Status.Specials.begin(); I != Status.Specials.end(); I++)
		{
			if (I->min >= cr.cpMin)
			{
				I->max += wsOutput.size();
				I->min += wsOutput.size();
			}
		}

		//cream un SPECIAL bazat pe aceste date si il adaugam in lista de SPECIAL-e
		special.type = SPECIAL::repet;
		//alocam spatiu pentru un element (SPECIAL), acest special fiind un REPETE
		special.pElem = new GenListCtrl::REPETE;
		//cream un pointer de tip REPETE pentru a prelucra datele din pElem sub forma de REPETE
		GenListCtrl::REPETE* pointer = (GenListCtrl::REPETE*)special.pElem;
		//nr. de afisari
		pointer->nRepCnt = pRepete->nRepCnt;
		//versurile
		pointer->Verses = pRepete->Verses;
		//daca se folosesc secunde sau minute
		pointer->bIsMinute = pRepete->bIsMinute;
		//pozitiile min si max din status
		int len = wsOutput.size();
		special.max = special.min + len - 1;
		//aici doar initializam
		special.pos = 0;
		//adaugam in lista de speciale.
		Status.Specials.push_back(special);
	}
}

void CStatusEditorDlg::OnAddSongDetText(HWND hWnd, BYTE songdet)
{
	CSongDetDlg::SONGDET info;
	SPECIAL special;

	//cream caseta de dialog "Informatii despre melodie"
	CSongDetDlg songDet(songdet);
	if (songDet.DoModal(m_hDlg) == IDOK)
	{
		ClearSelected(hWnd);
		info = songDet.m_SongDet;

		//preluam selectia
		CHARRANGE cr;
		SendMessage(hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);
		special.min = cr.cpMin;

		//aici, cr.cpMin = cr.cpMax
		SendMessage(hWnd, EM_EXSETSEL, 0, (LPARAM)&cr);

		//formatam textul si alegem culoarea
		COLORREF clr = RGB(255, 0, 0);
		CHARFORMAT cf;
		cf.cbSize = sizeof(CHARFORMAT);
		cf.dwMask = CFM_BOLD | CFM_COLOR;
		cf.dwEffects = CFE_BOLD;
		cf.crTextColor = clr;

		SendMessage(hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
		wstring wsOutput = L"Info:";
		if (info & CSongDetDlg::artist)
		{
			wsOutput += L"Artist";
			if (info & CSongDetDlg::album)
				wsOutput += L" - Album";
			if (info & CSongDetDlg::song)
				wsOutput += L" - Melodie";
		}
		else
		{
			if (info & CSongDetDlg::song)
				wsOutput += L"Melodie";
			if (info & CSongDetDlg::album)
				wsOutput += L" (Album)";
		}

		if (info & CSongDetDlg::length || info & CSongDetDlg::percent || 
			info & CSongDetDlg::lyrics || info & CSongDetDlg::progress)
			wsOutput += L"...";

		//scriem textul in RichEdit
		bFullRights = TRUE;
		for (UINT i = 0; i < wsOutput.length(); i++)
		{
			SendMessage(hWnd, WM_CHAR, wsOutput[i], 0);
		}

		bFullRights = FALSE;

		//resetarea formatului in RichEdit
		cf.dwEffects = 0;
		cf.crTextColor = 0;
		SendMessage(hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

		//acum trebuie sa actualizam pozitiile celorlalte SPECIAL-e 
		deque<SPECIAL>::iterator I;
		for (I = Status.Specials.begin(); I != Status.Specials.end(); I++)
		{
			if (I->min >= cr.cpMin)
			{
				I->max += wsOutput.size();
				I->min += wsOutput.size();
			}
		}

		//cream un SPECIAL bazat pe aceste date si il adaugam in lista de SPECIAL-e
		special.type = SPECIAL::songdet;
		//alocam spatiu pentru un element (SPECIAL), acest special fiind un SONGDET
		special.pElem = new CSongDetDlg::SONGDET;
		//copiem in pElem datele din info
		memcpy_s(special.pElem, sizeof(CSongDetDlg::SONGDET), &info, sizeof(CSongDetDlg::SONGDET));
		//pozitiile min si max din status
		int len = wsOutput.size();
		special.max = special.min + len - 1;
		//aici, initializam pos cu 0
		special.pos = 0;
		//adaugam in lista de special-e.
		Status.Specials.push_back(special);
	}
}

void CStatusEditorDlg::OnAddSongLyricsText(HWND hWnd)
{
	ClearSelected(hWnd);
	SPECIAL special;

	//preluam selectia
	CHARRANGE cr;
	SendMessage(hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);
	special.min = cr.cpMin;

	//aici, cr.cpMin = cr.cpMax
	SendMessage(hWnd, EM_EXSETSEL, 0, (LPARAM)&cr);

	//formatul textului si culoarea
	COLORREF clr = RGB(255, 0, 0);
	CHARFORMAT cf;
	cf.cbSize = sizeof(CHARFORMAT);
	cf.dwMask = CFM_BOLD | CFM_COLOR;
	cf.dwEffects = CFE_BOLD;
	cf.crTextColor = clr;

	//setam formatul
	SendMessage(hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
	//textul de afisat in RichEdit
	wstring wsOutput = L"Versuri";

	//scriem textul in RichEdit
	bFullRights = TRUE;
	for (UINT i = 0; i < wsOutput.length(); i++)
	{
		SendMessage(hWnd, WM_CHAR, wsOutput[i], 0);
	}

	bFullRights = FALSE;

	//resetam formatul si culoarea
	cf.dwEffects = 0;
	cf.crTextColor = 0;
	SendMessage(hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

	//acum trebuie sa actualizam pozitiile celorlalte SPECIAL-e 
	deque<SPECIAL>::iterator I2;
	for (I2 = Status.Specials.begin(); I2 != Status.Specials.end(); I2++)
	{
		if (I2->min >= cr.cpMin)
		{
			I2->max += wsOutput.size();
			I2->min += wsOutput.size();
		}
	}

	//cream un SPECIAL bazat pe aceste date si il adaugam in lista de SPECIAL-e
	special.type = SPECIAL::lyrics;
	//nu va exista nimic la pElem
	special.pElem = 0;
	//pozitiile min si max din status
	int len = wsOutput.size();
	special.max = special.min + len - 1;
	//initializam pos cu 0.
	special.pos = 0;
	//adaugam in lista
	Status.Specials.push_back(special);
}

void CStatusEditorDlg::OnRemoveText()
{
	////preluam selectia
	CHARRANGE cr;
	SendMessage(GetDlgItem(m_hDlg, IDC_RICHEDIT), EM_EXGETSEL, 0, (LPARAM)&cr);

	//daca orice obiect din structura status este in interiorul cr, va fi eliminat
	deque<SPECIAL>::iterator I;
	I = Status.Specials.begin();
	bool deleted = false;
	bool bBreak = false;
	while(I != Status.Specials.end())
	{
		if (I->min >= cr.cpMax || I->max < cr.cpMin) {I++; continue;}
		else 
		{
			//largim suprafata selectata daca un SPECIAL nu incape tot inauntru
			if (I->min < cr.cpMin) cr.cpMin = I->min;
			if (I->max >= cr.cpMax) cr.cpMax = I->max + 1;
			deque<SPECIAL>::iterator I2;
			I2 = I++;
			//daca acesta a fost ultimul din lista, vom iesi.
			if (I == Status.Specials.end()) bBreak = true;
			Status.Specials.erase(I2);
			deleted = true;
			if (bBreak) break;
		}
	}

	//selectam textul ce trebuie sters
	SendMessage(GetDlgItem(m_hDlg, IDC_RICHEDIT), EM_EXSETSEL, 0, (LPARAM)&cr);

	//daca a fost sters vreun special, modificam pozitiile min si max la fiecare SPECIAL la care trebuie
	if (deleted)
	{
		for (I = Status.Specials.begin(); I != Status.Specials.end(); I++)
		{
			if (I->min >= cr.cpMax)
			{
				I->max -= (cr.cpMax - cr.cpMin);
				I->min -= (cr.cpMax - cr.cpMin);
			}
		}
	}
}

void CStatusEditorDlg::OnOk()
{
	//trebuie sa convertim statusul in text de afisat
	//trebuie sa preluam limitele fiecarui SPECIAL

	HWND hWnd = GetDlgItem(m_hDlg, IDC_RICHEDIT);
	int len = GetWindowTextLengthW(hWnd);
	if (len == 0)
	{
		MessageBox(m_hDlg, L"Trebuie să scrii text in casetă ca să poți salva.", L"Atenție", MB_ICONWARNING);
		return;
	}
	len++;

	wchar_t* wstr = new wchar_t[len];
	GetWindowTextW(hWnd, wstr, len);
	Status.wsText = wstr;
	UINT i = 0;

	//stergem orice caracter '\r'. de fiecare data cand se apasa Enter este scris '\r\n'
	do
	{
		if (Status.wsText[i] == '\r')
			Status.wsText.erase(i, 1);
		else i++;
	}while (i < Status.wsText.length());

	wcscpy_s(wstr, len, Status.wsText.c_str());

	//daca nu am inserat nici un special, avem mai putin de lucru
	if (Status.Specials.size() == 0)
	{	
		Status.Statuses.push_back(wstr);
		goto finish;
	}

	{
	int lastpos = 0;
	wstring aux = L"";
	byte pos = 0;

	//gasim portiunile de text dintre SPECIAL-e, le salvam fiecare separat, si pentru fiecare SPECIAL
	//scriem pozitia in sir (dupa al cate-lea text normal)
	deque<SPECIAL>::iterator I;
	for (I = Status.Specials.begin(); I != Status.Specials.end(); I++)
	{
		aux = wstr;
		aux = aux.substr(lastpos, I->min - lastpos);
		if (aux.length())
		{
			pos++;
			Status.Statuses.push_back(aux);
			I->pos = pos;
		}
		else I->pos = pos;
		lastpos = I->max + 1;
	}

	//pentru ultimul
	aux = wstr;
		aux = aux.substr(lastpos, aux.length() - lastpos);
	if (aux.length())
		Status.Statuses.push_back(aux);
	}

finish:
	//adaugam status-ul in lista de statusuri
	genDll.m_StatusMsgs.push_back(Status);
	//salvam noul status intr-un fisier
	WriteStatusToFile();
	//daca caseta de dialog Alege Statusul este deschisa, va trebui sa isi actualizeze lista de statusuri
	if (genDll.m_hChooseDlg)
		SendMessage(genDll.m_hChooseDlg, WM_UPDATELIST, 0, (LPARAM)wstr);
	delete[] wstr;


	EndDialog(m_hDlg, IDOK);
}

void CStatusEditorDlg::WriteStatusToFile()
{
	//wsPathFolder = calea la folder-ul unde se cauta fisierele
	wstring wsPathFolder = genDll.m_wsAppData;
	//wsPathFile = calea catre fisierul ce va fi creat (adica, numele fisierului cu tot cu cale).
	wstring wsPathFile = wsPathFolder;
	wsPathFolder += L"\\*.stt";
	DWORD dwWritten;

	//nume de fisiere: 1.stt, 2.stt, ...

	//formatul fisierului:
	//ANTET:				'STT'
//CONTINUT:					byte lenwstr (lungimea lui wstr)
	//						wchar_t* wstr (statusul, cum apare in lista casetei de dialog "Alege Statusul")
	//						byte nrStrings - de aici, textele normale
	//						byte lenwstr1
	//						wchar_t wstr1
	//						...
	//						byte nrSpecials (trebuie specificat, chiar daca nrSpecials = 0)
	//						byte pos// pozitia in sir, 0 = inainte de primul sir.
	//						byte type: clock = 0, repet = 1, songdet = 2, lyrics = 3
	//						date_specifice - specifice fiecarui tip de SPECIAL
	//						...

	//date_specifice: clock
	//						byte hours					- numarul orelor
	//						byte minutes				- numarul minutelor
	//						byte len_bef				- lungimea sirului wsBefore
	//						wchar_t wsBefore[len_bef]	- textul afisat inainte de ceas
	//						byte len_after				- lungimea sirului wsAfter
	//						wchar_t wsAfter[len_after]	- textul afisat dupa ce se opreste ceasul

	//date_specifice: repet:
	//						int nRepCnt					- nr afisari
	//						byte bIsMinute				- 1 daca este nrSec reprezinta minute, 0 daca reprezinta secunde
	//						int nrVerses				- numarul versurilor (randurilor de text)
	//						WORD nrSec1					- secunda la care va fi afisat textul respectiv (verse1[length1])
	//						byte length1				- lungimea textului
	//						wchar_t verse1[length1]		- textul ce va fi afisat la secunda/minutul nrSec1
	//						...

	//date_specifice: songdet:
	//						byte flags
	//bits:					artist = 1, album = 2, song = 4, progress = 8, length = 16, lyrics = 32, percent = 64
	
	//date_specifice: lyrics:	nimic!

	//prima data, cautam un nume pentru fisier.
	WIN32_FIND_DATA f_data = {0};
	BOOL bResult =0;
	//numele fisierului va fi de forma <numar>.stt
	int fileNr = 0, nMaxFileNr = 0;
	//cautam ce numar vom pune fisierului acestuia. aici, primul fisier de extensie ".stt"
	HANDLE hSearch = FindFirstFile(wsPathFolder.data(), &f_data);
	while (hSearch != INVALID_HANDLE_VALUE)
	{
		//construim numele fisierului. f_data.cFileName = numele fisierului, cu extensie, fara cale.
		wstring wsFileName = f_data.cFileName;
		//stergem din wsFileName, extensia
		wsFileName = wsFileName.erase(wsFileName.length() - 4, 4);
		//gasim numarul fisierului respectiv.
		fileNr = _wtoi(wsFileName.c_str());
		//daca nu este de forma <numar>.stt, adica, numele fisierului fara extensie nu e un numar, continuam
		//cautarea
		if (fileNr == 0)
		{
			bResult = FindNextFile(hSearch, &f_data);
			//daca s-au parcurs toate fisierele din folder, se iasa din repetitie
			if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
			//altfel se continua cautarea
			continue;
		}

		nMaxFileNr = max(nMaxFileNr, fileNr);

		//daca s-au parcurs toate fisierele din folder, se iasa din repetitie
		bResult = FindNextFile(hSearch, &f_data);
		if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
	}
	FindClose(hSearch);

	//acum, numarul fisierului nostru si numele lui
	nMaxFileNr++;
	wchar_t file_name[10];
	_itow_s(nMaxFileNr, file_name, 10, 10);
	wsPathFile += L"\\";
	wsPathFile += file_name;
	wsPathFile += L".stt";

	//cream fisierul
	HANDLE hFile = CreateFile(wsPathFile.c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	//acum il scriem
	WriteFile(hFile, "STT", 3, &dwWritten, 0);
	//lenstr (wsText):
	byte len = (byte)Status.wsText.length() + 1;
	WriteFile(hFile, &len, 1, &dwWritten, 0);
	//wsText - textul cum apare in caseta de dialog "alege statusul"
	WriteFile(hFile, Status.wsText.c_str(), len * 2, &dwWritten, 0);

	byte sz = (byte)Status.Statuses.size();
	WriteFile(hFile, &sz, 1, &dwWritten, 0);

	//acum scriem fiecare sir, lungimea (len) si sirul:
	deque<wstring>::iterator I;
	for (I = Status.Statuses.begin(); I != Status.Statuses.end(); I++)
	{
		byte len = (byte)I->length() + 1;
		WriteFile(hFile, &len, 1, &dwWritten, 0);
		WriteFile(hFile, I->c_str(), len * 2, &dwWritten, 0);
	}

	//acum scriem SPECIAL-ele. mai intai, numarul lor:
	sz = (byte)Status.Specials.size();
	WriteFile(hFile, &sz, 1, &dwWritten, 0);
	//acum fiecare SPECIAL in parte
	deque<SPECIAL>::iterator I2;
	for (I2 = Status.Specials.begin(); I2 != Status.Specials.end(); I2++)
	{
		//acum gasim si scriem pozitia in sir. 0 = inainte de primul
		WriteFile(hFile, &I2->pos, 1, &dwWritten, 0);
		//tipul
		byte type = (byte)I2->type;
		WriteFile(hFile, &type, 1, &dwWritten, 0);
		//acum depinde de tip:
		switch (I2->type)
		{
		case SPECIAL::clock: 
			{
				//byte hours				- nr orelor
				//byte minutes				- nr minutelor
				//byte len_bef				- lungimea textului wsBefore
				//wchar_t wsBefore[len_bef]	- textul afisat inainte de ceas
				//byte len_after			- lungimea textului wsAfter
				//wchar_t wsAfter[len_after]- textul afisat dupa ce se opreste ceasul
				
				//un pointer de tipul CLOCK la element.
				CClockDlg::CLOCK* pointer = (CClockDlg::CLOCK*)I2->pElem;
				//orele
				byte val = pointer->dHours;
				WriteFile(hFile, &val, 1, &dwWritten, 0);
				//minutele
				val = pointer->dMins;
				WriteFile(hFile, &val, 1, &dwWritten, 0);

				//len_bef
				val = (byte)pointer->wsTextBefore.length() + 1;
				WriteFile(hFile, &val, 1, &dwWritten, 0);
				//wsBefore
				WriteFile(hFile, pointer->wsTextBefore.c_str(), val * 2, &dwWritten, 0);

				//len_after
				val = (byte)pointer->wsTextAfter.length() + 1;
				WriteFile(hFile, &val, 1, &dwWritten, 0);
				//wsAfter
				WriteFile(hFile, pointer->wsTextAfter.c_str(), val * 2, &dwWritten, 0);
			}
			break;

		case SPECIAL::repet:
			{
				//	int nRepCnt					- nr afisari
				//	byte bIsMinute				- 1 daca este nrSec reprezinta minute, 0 daca reprezinta secunde
				//	int nrVerses				- numarul versurilor (randurilor de text)
				//	WORD nrSec1					- secunda la care va fi afisat textul respectiv (verse1[length1])
				//	byte length1				- lungimea textului
				//	wchar_t verse1[length1]		- textul ce va fi afisat la secunda/minutul nrSec1
				//  ...

				//un pointer de tipul REPETE la element.
				GenListCtrl::REPETE*	pRepete = ((GenListCtrl::REPETE*)I2->pElem);
				//nRepCnt - nr de afisari
				int nRepCnt = pRepete->nRepCnt;
				WriteFile(hFile, &nRepCnt, sizeof(int), &dwWritten, 0);
				//bIsMinute
				byte bIsMinute = pRepete->bIsMinute;
				WriteFile(hFile, &bIsMinute, 1, &dwWritten, 0);

				//nrVerses
				int nrVerses = (int)pRepete->Verses.size();
				WriteFile(hFile, &nrVerses, sizeof(int), &dwWritten, 0);

				//fiecare rand in parte, cu tot cu secunda/minutul lui
				deque<GenListCtrl::VERSE>::iterator I3;
				for (I3 = pRepete->Verses.begin(); I3 != pRepete->Verses.end(); I3++)
				{
					//nrSec
					WriteFile(hFile, &I3->nrSec, sizeof(WORD), &dwWritten, 0);
					//lungimea sirului
					byte len = (byte)I3->wsVerse.length() + 1;
					WriteFile(hFile, &len, 1, &dwWritten, 0);
					//sirul
					WriteFile(hFile, I3->wsVerse.c_str(), len * 2, &dwWritten, 0);
				}
			}
			break;

		case SPECIAL::songdet: 
			{
				//byte flags
				byte* det = ((CSongDetDlg::SONGDET*)I2->pElem);
				WriteFile(hFile, det, 1, &dwWritten, 0);
			}
			break;

		case SPECIAL::lyrics: break;
		}
	}

	CloseHandle(hFile);
}

void CStatusEditorDlg::OnEditText()
{
	//preluam selectia
	CHARRANGE cr;
	SendMessage(GetDlgItem(m_hDlg, IDC_RICHEDIT), EM_EXGETSEL, 0, (LPARAM)&cr);

	//trebuie sa fie caret-ul/SPECIAL-ul in interiorul unui SPECIAL, sau cel mult intregul SPECIAL
	//sa fie selectat
	bool bEdit = false;
	deque<SPECIAL>::iterator I;
	for (I = Status.Specials.begin(); I != Status.Specials.end(); I++)
	{
		if (cr.cpMin >= I->min && cr.cpMax - 1 <= I->max)
		{
			bEdit = true;
			break;
		}
	}
	
	_ASSERT(bEdit);
	if (bEdit == false) {MessageBoxW(m_hDlg, L"Text necunoscut", 0, MB_ICONERROR); return; }

	cr.cpMin = I->min;
	cr.cpMax = I->max + 1;
	//selectam intreg SPECIAL-ul
	SendMessage(GetDlgItem(m_hDlg, IDC_RICHEDIT), EM_EXSETSEL, 0, (LPARAM)&cr);
	HWND hREdit = GetDlgItem(m_hDlg, IDC_RICHEDIT);

	switch (I->type)
	{
	case SPECIAL::clock:
		{
			//preluam elementul Ceas si il trimitem ca parametru la OnAddClockText
			CClockDlg::CLOCK* pClock = (CClockDlg::CLOCK*)I->pElem;
			OnAddClockText(hREdit, pClock);
		}
		break;
		
	case SPECIAL::repet:
		{
			//preluam elementul Repetitie si il trimitem ca parametru la OnAddRepText
			GenListCtrl::REPETE* pRepete = (GenListCtrl::REPETE*)I->pElem;
			OnAddRepText(hREdit, pRepete);
		}
		break;
	case SPECIAL::songdet:
		{
			CSongDetDlg::SONGDET* pSongDet = (CSongDetDlg::SONGDET*)I->pElem;
			OnAddSongDetText(hREdit, *pSongDet);
		}
		//la lyrics nu facem nimic.
	}

}

void CStatusEditorDlg::ClearSelected(HWND hREdit)
{
	//trebuie mai intai sa preluam selectia:
	CHARRANGE cr;
	SendMessage(hREdit, EM_EXGETSEL, 0, (LPARAM)&cr);

	//stergem daca avem selectie
	if (cr.cpMax != cr.cpMin)
	{
		SendMessage(m_hDlg, WM_COMMAND, IDC_REMOVETEXT, 0);
		SendMessage(hREdit, WM_KEYDOWN, VK_DELETE, 0);
	}
}