#include "StatusEditorDlg.h"
#include "GenDll.h"
#include "ClockDlg.h"
#include "RepetitionDlg.h"
#include "SongDetDlg.h"
#include "LyricsDlg.h"

#include "resource.h"

extern CGenDll genDll;
HMODULE hModule;
BOOL bFullRights = false;
STATUS Status;

CStatusEditorDlg::CStatusEditorDlg(int nSel)
{
	hModule = LoadLibrary(L"RICHED20.DLL");
}

CStatusEditorDlg::CStatusEditorDlg()
{
	hModule = LoadLibrary(L"RICHED20.DLL");
}

CStatusEditorDlg::~CStatusEditorDlg(void)
{
	FreeLibrary(hModule);
	Status.Specials.clear();
	Status.Statuses.clear();
	Status.Specials.clear();
}

void CStatusEditorDlg::DoModal(HWND hParent)
{
	int nResult = DialogBoxParam(genDll.m_hInstance, MAKEINTRESOURCE(IDD_STATUS_EDITOR), hParent, DialogProc, (LPARAM)this);
	genDll.m_bStatusEdtCreated = false;
	if (nResult == -1)
	{
		DWORD dwError = GetLastError();
		WCHAR sError[100];
		wsprintf(sError, L"Error 0x%x", dwError);
		MessageBox(hParent, sError, L"Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
	}
}

INT_PTR CALLBACK CStatusEditorDlg::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_CLOSE: EndDialog(hDlg, IDCANCEL); break;
		case WM_COMMAND:
			{
				switch (LOWORD(wParam))
				{
				case IDOK: OnOk(hDlg); break;
				case IDCANCEL:EndDialog(hDlg, IDCANCEL); break;
				case IDC_ADDTEXT: OnAddText(hDlg, (HWND)lParam); return 0;
				case IDC_REMOVETEXT: 
					{
						OnRemoveText(hDlg, wParam);
						if (lParam)//if sent from the button "Remove"
						{
							SendMessage(GetDlgItem(hDlg, IDC_RICHEDIT), WM_KEYDOWN, VK_DELETE, 0);
							SetFocus(GetDlgItem(hDlg, IDC_RICHEDIT));
						}
						return 0;
					}
					return 0;

				case IDC_EDITTEXT: OnEditText(hDlg); return 0;
				}
			}
			break;//case WM_COMMAND
		case WM_INITDIALOG: OnInitDialog(hDlg, lParam); return TRUE;

		case WM_NOTIFY:
			{
				NMHDR* pNMHDR = (NMHDR*)lParam;
				if (pNMHDR->code == EN_SELCHANGE)
				{
					//then, we need a bigger structure:
					SELCHANGE*	pSel = (SELCHANGE*)lParam;
					CHARRANGE cr = pSel->chrg;
					//we must check to see if special text is selected.

					bool bRemove = false;
					if (cr.cpMin != cr.cpMax)
						bRemove = true;

					bool bEdit = false;
					deque<SPECIAL>::iterator I;
					for (I = Status.Specials.begin(); I != Status.Specials.end(); I++)
					{
						if (cr.cpMin >= I->min && cr.cpMax - 1 <= I->max && I->type != SPECIAL::lyrics)
						{
							bEdit = true;
							bRemove = true;
							break;
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

void CStatusEditorDlg::OnInitDialog(HWND hDlg, LPARAM lParam)
{
	genDll.m_bStatusEdtCreated = true;
	ShowWindow(hDlg, 1);
	CStatusEditorDlg*	pThis = (CStatusEditorDlg*)lParam;
	SetWindowLong(hDlg, GWL_USERDATA, (LONG)lParam);

	pThis->richEdit.Create(GetDlgItem(hDlg, IDC_RICHEDIT));

	SendMessage(pThis->richEdit.m_hWnd, EM_SETEVENTMASK, 0, ENM_SELCHANGE);
}

void CStatusEditorDlg::OnAddText(HWND hDlg, HWND hWnd)
{
	HMENU hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, ID_ADDCLOCKTEXT, L"Ceas");
	AppendMenu(hMenu, MF_STRING, ID_ADDREPTEXT, L"Repetiție");
	AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(hMenu, MF_STRING, ID_ADDSONGDETTEXT, L"Detalii despre melodie");
	AppendMenu(hMenu, MF_STRING, ID_ADDLYRICSTEXT, L"Versuri pentru melodie");

	RECT rect;
	GetWindowRect(hWnd, &rect);

	int nResult = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON,
	rect.left, rect.bottom - 1, 0, hWnd, 0);

	//Now we change the window, from Add to RichEdit:
	hWnd = GetDlgItem(hDlg, IDC_RICHEDIT);

	CHARRANGE chrange;
	SendMessage(hWnd, EM_EXGETSEL, 0, (LPARAM)&chrange);
	bool bRemove = false;
	if (chrange.cpMin != chrange.cpMax)
		bRemove = true;
	
	switch (nResult)
	{
	case 0: return;
	case ID_ADDCLOCKTEXT: OnAddClockText(hDlg, hWnd, bRemove, 0); break;
	case ID_ADDREPTEXT: OnAddRepText(hDlg, hWnd, bRemove, 0); break;
	case ID_ADDSONGDETTEXT: OnAddSongDetText(hDlg, hWnd, bRemove, 0); break;
	case ID_ADDLYRICSTEXT: OnAddSongLyricsText(hDlg, hWnd, bRemove); break;
	}

	SetFocus(hWnd);
}

void CStatusEditorDlg::OnAddClockText(HWND hDlg, HWND hWnd, bool bRemove, CClockDlg::CLOCK* pSpec)
{
	CClockDlg::CLOCK* pClock;
	CClockDlg clockDlg(pSpec);

	SPECIAL special;
	if (IDOK == clockDlg.DoModal(hDlg))
	{
		//initializing
		ClearSelected(hDlg, hWnd);
		pClock = &clockDlg.m_Clock;
		CStatusEditorDlg* pThis = (CStatusEditorDlg*)GetWindowLong(hDlg, GWL_USERDATA);
		CHARRANGE cr;
		SendMessage(hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);
		special.min = cr.cpMin;

		//REMOVING, if necessary
		if (bRemove) SendMessage(hDlg, WM_COMMAND, IDC_REMOVETEXT, 0);
		cr.cpMax = cr.cpMin;
		SendMessage(hWnd, EM_EXSETSEL, 0, (LPARAM)&cr);

		//we must also insert the data INTO THE RICH EDIT CTRL.
		COLORREF clr = RGB(255, 0, 0);
		CHARFORMAT cf;
		cf.cbSize = sizeof(CHARFORMAT);
		cf.dwMask = CFM_BOLD | CFM_COLOR;
		cf.dwEffects = CFE_BOLD;
		cf.crTextColor = clr;

		SendMessage(hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
		string sOutput = clockDlg.m_Clock.sTextBefore;
//		sOutput += ' ';
		char sNow[6] = "";
		if (clockDlg.m_Clock.dHours)
		{
			sprintf_s(sNow, 6, "%dh", clockDlg.m_Clock.dHours);
			sOutput += sNow;
		}

		if (clockDlg.m_Clock.dMins)
		{
			sprintf_s(sNow, 6, "%dm", clockDlg.m_Clock.dMins);
			if (clockDlg.m_Clock.dHours)
				sOutput += ' ';
			sOutput += sNow;
		}

		bFullRights = TRUE;
		for (int i = 0; i < sOutput.length(); i++)
		{
			SendMessage(hWnd, WM_CHAR, sOutput[i], 0);
		}

		bFullRights = FALSE;

		cf.dwEffects = 0;
		cf.crTextColor = 0;
		SendMessage(hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

		//we have to update other status messages
		deque<SPECIAL>::iterator I;
		for (I = Status.Specials.begin(); I != Status.Specials.end(); I++)
		{
			if (I->min >= cr.cpMin)
			{
				I->max += sOutput.length();
				I->min += sOutput.length();
			}
		}

		//add INTO THE STATUS STRUCTURE
		special.type = SPECIAL::clock;
		special.pElem = new CClockDlg::CLOCK;
		CClockDlg::CLOCK* pointer = (CClockDlg::CLOCK*)special.pElem;
		pointer->dHours = pClock->dHours;
		pointer->dMins = pClock->dMins;
		pointer->sTextBefore = pClock->sTextBefore;
		pointer->sTextAfter = pClock->sTextAfter;
//		memcpy_s(special.pElem, sizeof(CClockDlg::CLOCK), pClock, sizeof(CClockDlg::CLOCK));

		int len = sOutput.length();
		special.max = special.min + len - 1;
		special.pos = 0;
		Status.Specials.push_back(special);
//		Status.sStatus.insert(
	}
}

void CStatusEditorDlg::OnAddRepText(HWND hDlg, HWND hWnd, bool bRemove, GenListCtrl::REPETE* pSpec)
{
	CRepetitionDlg repDlg(pSpec);
	GenListCtrl::REPETE* pRepete;
	SPECIAL special;
	if (IDOK == repDlg.DoModal(hDlg))
	{
		ClearSelected(hDlg, hWnd);
		pRepete = &repDlg.m_list.m_Repetition;

		CStatusEditorDlg* pThis = (CStatusEditorDlg*)GetWindowLong(hDlg, GWL_USERDATA);
		CHARRANGE cr;
		SendMessage(hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);
		special.min = cr.cpMin;
//		m_pRepete->min = cr.cpMin;

		//REMOVING, if necessary
		if (bRemove) SendMessage(hDlg, WM_COMMAND, IDC_REMOVETEXT, 0);
		cr.cpMax = cr.cpMin;
		SendMessage(hWnd, EM_EXSETSEL, 0, (LPARAM)&cr);

		//we must also insert the data INTO THE RICH EDIT CTRL.
		COLORREF clr = RGB(255, 0, 0);
		CHARFORMAT cf;
		cf.cbSize = sizeof(CHARFORMAT);
		cf.dwMask = CFM_BOLD | CFM_COLOR;
		cf.dwEffects = CFE_BOLD;
		cf.crTextColor = clr;

		SendMessage(hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
		string sOutput;
		sOutput = "R:" + pRepete->Verses.front().sVerse.substr(0, 20);
		if (pRepete->Verses.size() > 1 || pRepete->Verses.front().sVerse.length() > 20)
			sOutput += "...";
		if (pRepete->nRepCnt > -1)
		{
			sOutput += "x";
			char s[3];
			itoa(pRepete->nRepCnt, s, 10);
			sOutput += s;
		}

		bFullRights = TRUE;
		for (int i = 0; i < sOutput.length(); i++)
		{
			SendMessage(hWnd, WM_CHAR, sOutput[i], 0);
		}

		bFullRights = FALSE;

		cf.dwEffects = 0;
		cf.crTextColor = 0;
		SendMessage(hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

		//we have to update other status messages
		deque<SPECIAL>::iterator I;
		for (I = Status.Specials.begin(); I != Status.Specials.end(); I++)
		{
			if (I->min >= cr.cpMin)
			{
				I->max += sOutput.size();
				I->min += sOutput.size();
			}
		}

		//add INTO THE STATUS STRUCTURE
		special.type = SPECIAL::repet;
		special.pElem = new GenListCtrl::REPETE;
		GenListCtrl::REPETE* pointer = (GenListCtrl::REPETE*)special.pElem;
		pointer->nRepCnt = pRepete->nRepCnt;
		pointer->Verses = pRepete->Verses;
//		memcpy_s(special.pElem, sizeof(GenListCtrl::REPETE), pRepete, sizeof(GenListCtrl::REPETE));
		//*special.pElem = *m_pRepete;
		int len = sOutput.size();
//		m_pRepete->max = m_pRepete->min + len - 1;
		special.max = special.min + len - 1;
		special.pos = 0;
		Status.Specials.push_back(special);
	}
}

void CStatusEditorDlg::OnAddSongDetText(HWND hDlg, HWND hWnd, bool bRemove, SPECIAL* pSpec)
{
	CSongDetDlg songDet(pSpec);
	CSongDetDlg::SONGDET info;
	SPECIAL special;
	if (songDet.DoModal(hDlg) == IDOK)
	{
		ClearSelected(hDlg, hWnd);
		info = songDet.m_SongDet;

		CStatusEditorDlg* pThis = (CStatusEditorDlg*)GetWindowLong(hDlg, GWL_USERDATA);
		CHARRANGE cr;
		SendMessage(hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);
		special.min = cr.cpMin;

		//REMOVING, if necessary
		if (bRemove) SendMessage(hDlg, WM_COMMAND, IDC_REMOVETEXT, 0);
		cr.cpMax = cr.cpMin;
		SendMessage(hWnd, EM_EXSETSEL, 0, (LPARAM)&cr);

		//we must also insert the data INTO THE RICH EDIT CTRL.
		COLORREF clr = RGB(255, 0, 0);
		CHARFORMAT cf;
		cf.cbSize = sizeof(CHARFORMAT);
		cf.dwMask = CFM_BOLD | CFM_COLOR;
		cf.dwEffects = CFE_BOLD;
		cf.crTextColor = clr;

		SendMessage(hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
		string sOutput = "Info:";
		if (info & CSongDetDlg::info::artist)
		{
			sOutput += "Artist";
			if (info & CSongDetDlg::info::song)
				sOutput += " - Melodie";
		}
		else
		{
			if (info & CSongDetDlg::info::song)
				sOutput += "Melodie";
		}

		if (info & CSongDetDlg::info::length || info & CSongDetDlg::info::percent || 
			info & CSongDetDlg::info::percent || info & CSongDetDlg::info::progress)
			sOutput += "...";

		bFullRights = TRUE;
		for (int i = 0; i < sOutput.length(); i++)
		{
			SendMessage(hWnd, WM_CHAR, sOutput[i], 0);
		}

		bFullRights = FALSE;

		cf.dwEffects = 0;
		cf.crTextColor = 0;
		SendMessage(hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

		//we have to update other status messages
		deque<SPECIAL>::iterator I;
		for (I = Status.Specials.begin(); I != Status.Specials.end(); I++)
		{
			if (I->min >= cr.cpMin)
			{
				I->max += sOutput.size();
				I->min += sOutput.size();
			}
		}

		//add INTO THE STATUS STRUCTURE
		special.type = SPECIAL::songdet;
		special.pElem = new CSongDetDlg::SONGDET;
		memcpy_s(special.pElem, sizeof(CSongDetDlg::SONGDET), &info, sizeof(CSongDetDlg::SONGDET));
		//*special.pElem = *m_pRepete;
		int len = sOutput.size();
//		m_pRepete->max = m_pRepete->min + len - 1;
		special.max = special.min + len - 1;
		special.pos = 0;
		Status.Specials.push_back(special);
	}
}

void CStatusEditorDlg::OnAddSongLyricsText(HWND hDlg, HWND hWnd, bool bRemove)
{
	ClearSelected(hDlg, hWnd);
	SPECIAL special;

	CStatusEditorDlg* pThis = (CStatusEditorDlg*)GetWindowLong(hDlg, GWL_USERDATA);
	CHARRANGE cr;
	SendMessage(hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);
	special.min = cr.cpMin;

	//REMOVING, if necessary
	if (bRemove) SendMessage(hDlg, WM_COMMAND, IDC_REMOVETEXT, 0);
	cr.cpMax = cr.cpMin;
	SendMessage(hWnd, EM_EXSETSEL, 0, (LPARAM)&cr);

	//we must also insert the data INTO THE RICH EDIT CTRL.
	COLORREF clr = RGB(255, 0, 0);
	CHARFORMAT cf;
	cf.cbSize = sizeof(CHARFORMAT);
	cf.dwMask = CFM_BOLD | CFM_COLOR;
	cf.dwEffects = CFE_BOLD;
	cf.crTextColor = clr;

	SendMessage(hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
	string sOutput = "Versuri";

	bFullRights = TRUE;
	for (int i = 0; i < sOutput.length(); i++)
	{
		SendMessage(hWnd, WM_CHAR, sOutput[i], 0);
	}

	bFullRights = FALSE;

	cf.dwEffects = 0;
	cf.crTextColor = 0;
	SendMessage(hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

	//we have to update other status messages
	deque<SPECIAL>::iterator I2;
	for (I2 = Status.Specials.begin(); I2 != Status.Specials.end(); I2++)
	{
		if (I2->min >= cr.cpMin)
		{
			I2->max += sOutput.size();
			I2->min += sOutput.size();
		}
	}

	//add INTO THE STATUS STRUCTURE
	special.type = SPECIAL::lyrics;
	special.pElem = 0;
		
	int len = sOutput.size();
	special.max = special.min + len - 1;
	special.pos = 0;
	Status.Specials.push_back(special);
}

void CStatusEditorDlg::OnRemoveText(HWND hDlg, WPARAM wParam)
{
	CStatusEditorDlg* pThis = (CStatusEditorDlg*)GetWindowLong(hDlg, GWL_USERDATA);
	//get selection
	CHARRANGE cr;
	SendMessage(GetDlgItem(hDlg, IDC_RICHEDIT), EM_EXGETSEL, 0, (LPARAM)&cr);

	//if any object within the status structure is inside cr, it will be removed
	deque<SPECIAL>::iterator I;
	I = Status.Specials.begin();
	bool deleted = false;
	bool bBreak = false;
	while(I != Status.Specials.end())
	{
		if (I->min >= cr.cpMax || I->max < cr.cpMin) {I++; continue;}
		else 
		{
			//enlarge the selected area if an item does not fit all within
			if (I->min < cr.cpMin) cr.cpMin = I->min;
			if (I->max >= cr.cpMax) cr.cpMax = I->max + 1;
			deque<SPECIAL>::iterator I2;
			I2 = I++;
			if (I == Status.Specials.end()) bBreak = true;
			Status.Specials.erase(I2);//will it continue properly?
			deleted = true;
			if (bBreak) break;
		}
	}

	SendMessage(GetDlgItem(hDlg, IDC_RICHEDIT), EM_EXSETSEL, 0, (LPARAM)&cr);

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

//	Status.sStatus.erase(cr.cpMin, cr.cpMax);
}

void CStatusEditorDlg::OnOk(HWND hDlg)
{
	//we must convert the status to a writable thing.
	//we have to take the limits of each SPECIAL

	HWND hWnd = GetDlgItem(hDlg, IDC_RICHEDIT);
	int len = GetWindowTextLengthA(hWnd);
	len++;
	char* str = new char[len];
	wchar_t* wstr = new wchar_t[len];
	GetWindowTextA(hWnd, str, len);
	GetWindowTextW(hWnd, wstr, len);
	Status.swText = wstr;
	string aux = str;
	int i = 0;
	do
	{
		if (Status.swText[i] == '\r')
		{
			Status.swText.erase(i, 1);
			aux.erase(i, 1);
		}
		else i++;
	}while (i < aux.length());

	strcpy_s(str, len, aux.c_str());
	wcscpy_s(wstr, len, Status.swText.c_str());

	if (Status.Specials.size() == 0)
	{	
		Status.Statuses.push_back(str);
		goto finish;
	}

	{
	int lastpos = 0;
	aux = "";
	byte pos = 0;

	deque<SPECIAL>::iterator I;
	for (I = Status.Specials.begin(); I != Status.Specials.end(); I++)
	{
		aux = str;
		aux = aux.substr(lastpos, I->min - lastpos);
		if (aux.length())
		{
			pos++;
			Status.Statuses.push_back(aux);
			I->pos = pos;
		}
		else I->pos = pos;
		lastpos = I->max + 1;
//		pos++;
	}

	aux = str;
//	if (lastpos > aux.length())
//		aux = "";
//	else
		aux = aux.substr(lastpos, aux.length() - lastpos);
	if (aux.length())
		Status.Statuses.push_back(aux);
	}

finish:
	genDll.m_StatusMsgs.push_back(Status);
	WriteStatusToFile();
	if (genDll.m_bChooseDlgCreated)
		SendMessage(genDll.m_hChooseDlg, WM_UPDATELIST, 0, (LPARAM)wstr);
	delete[] str;
	delete[] wstr;


	EndDialog(hDlg, IDOK);
}

void CStatusEditorDlg::WriteStatusToFile()
{
	wstring wsPath = genDll.m_swAppData;
	wstring wsPath2 = wsPath;
	wsPath += L"\\*.stt";
	DWORD dwWritten;

	//filename: 1.stt, 2.stt, ...

	//file format:
	//HEADER:				'STT'
	//BODY:					byte lenstr
	//						wchar_t* str
	//						byte nrStrings
	//						byte lenstr1
	//						char str1
	//						...
	//						byte nrSpecials (must be, even if nrSpecials = 0)
	//						byte pos// the position in string, 0 = before first string.
	//						byte min!!!!!!!!!!!!!!!
	//						byte max!!!!!!!!!!!!!!!!
	//						byte type: clock = 0, repet = 1, songdet = 2, lyrics = 3
	//						custom_data
	//						...

	//custom_data: clock
	//						byte hours
	//						byte minutes
	//						byte len_bef
	//						char sBefore[len_bef]
	//						byte len_after
	//						char sAfter[len_after]

	//custom_data: repet:
	//						int nRepCnt
	//						int nrVerses
	//						WORD nrSec1
	//						byte length1
	//						char verse[length1]
	//						...

	//custom_data: songdet:
	//						byte flags
	//bits:					artist = 1, song = 2, progress = 4, length = 8, percent = 16
	
	//custom_data: lyrics:	NOTHING/VOID/whatever!

	WIN32_FIND_DATA f_data;
	BOOL bResult =0;
	memset(&f_data, 0, sizeof(f_data));
	int fileNr = 0;
	HANDLE hSearch = FindFirstFile(wsPath.data(), &f_data);
	while (hSearch != INVALID_HANDLE_VALUE)
	{
		//make sure it is <number>.lyr
		wstring wsFileName = f_data.cFileName;
		wsFileName = wsFileName.erase(wsFileName.length() - 4, 4);
		fileNr = _wtoi(wsFileName.c_str());
		if (fileNr == 0) {FindNextFile(hSearch, &f_data); continue;}

		bResult = FindNextFile(hSearch, &f_data);
		if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
	}
	FindClose(hSearch);

	fileNr++;
	wchar_t file_name[10];
	_itow_s(fileNr, file_name, 10, 10);
	wsPath2 += L"\\";
	wsPath2 += file_name;
	wsPath2 += L".stt";

	//we create the file
	HANDLE hFile = CreateFile(wsPath2.c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	//now we write it!

	WriteFile(hFile, "STT", 3, &dwWritten, 0);
	//lenstr (sText):
	byte len = Status.swText.length() + 1;
	WriteFile(hFile, &len, 1, &dwWritten, 0);
	//sText
	WriteFile(hFile, Status.swText.c_str(), len * 2, &dwWritten, 0);

	byte sz = (byte)Status.Statuses.size();
	WriteFile(hFile, &sz, 1, &dwWritten, 0);

	//now we write each string, len and string:
	deque<string>::iterator I;
	for (I = Status.Statuses.begin(); I != Status.Statuses.end(); I++)
	{
		byte len = I->length() + 1;
		WriteFile(hFile, &len, 1, &dwWritten, 0);
		WriteFile(hFile, I->c_str(), len, &dwWritten, 0);
	}

	//now we write the specials. first, their number:
	
	sz = (byte)Status.Specials.size();
	WriteFile(hFile, &sz, 1, &dwWritten, 0);
	deque<SPECIAL>::iterator I2;
	for (I2 = Status.Specials.begin(); I2 != Status.Specials.end(); I2++)
	{
		//now we find out and write the position in the string. 0 = before first
		WriteFile(hFile, &I2->pos, 1, &dwWritten, 0);
		byte min = (byte)I2->min;
		byte max = (byte)I2->max;
		WriteFile(hFile, &min, 1, &dwWritten, 0);
		WriteFile(hFile, &max, 1, &dwWritten, 0);
		//type
		byte type = (byte)I2->type;
		WriteFile(hFile, &type, 1, &dwWritten, 0);
		//now it depends on the type:
		switch (I2->type)
		{
		case SPECIAL::clock: 
			{
				//byte hours
				//byte minutes
				//byte len_bef
				//char sBefore[len_bef]
				//byte len_after
				//char sAfter[len_after]
				
				CClockDlg::CLOCK* pointer = (CClockDlg::CLOCK*)I2->pElem;
				byte val = pointer->dHours;
				WriteFile(hFile, &val, 1, &dwWritten, 0);
				val = pointer->dMins;
				WriteFile(hFile, &val, 1, &dwWritten, 0);

				val = pointer->sTextBefore.length() + 1;
				WriteFile(hFile, &val, 1, &dwWritten, 0);
				WriteFile(hFile, pointer->sTextBefore.c_str(), val, &dwWritten, 0);

				val = pointer->sTextAfter.length() + 1;
				WriteFile(hFile, &val, 1, &dwWritten, 0);
				WriteFile(hFile, pointer->sTextAfter.c_str(), val, &dwWritten, 0);
			}
			break;

		case SPECIAL::repet:
			{
			//	int nRepCnt
			//	int nrVerses
			//	WORD nrSec1
			//	byte length1
			//	char verse[length1]
			//	...
				GenListCtrl::REPETE*	pRepete = ((GenListCtrl::REPETE*)I2->pElem);
				int nRepCnt = pRepete->nRepCnt;
				WriteFile(hFile, &nRepCnt, sizeof(int), &dwWritten, 0);
				int nrVerses = (int)pRepete->Verses.size();
				WriteFile(hFile, &nrVerses, sizeof(int), &dwWritten, 0);

				deque<GenListCtrl::VERSE>::iterator I3;
				for (I3 = pRepete->Verses.begin(); I3 != pRepete->Verses.end(); I3++)
				{
					WriteFile(hFile, &I3->nrSec, sizeof(WORD), &dwWritten, 0);
					byte len = (byte)I3->sVerse.length() + 1;
					WriteFile(hFile, &len, 1, &dwWritten, 0);
					WriteFile(hFile, I3->sVerse.c_str(), len, &dwWritten, 0);
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

void CStatusEditorDlg::OnEditText(HWND hDlg)
{
	CHARRANGE cr;
	SendMessage(GetDlgItem(hDlg, IDC_RICHEDIT), EM_EXGETSEL, 0, (LPARAM)&cr);

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
	
	if (bEdit == false) {MessageBoxA(hDlg, "Text necunoscut", 0, MB_ICONERROR); return; }

	cr.cpMin = I->min;
	cr.cpMax = I->max + 1;
	SendMessage(GetDlgItem(hDlg, IDC_RICHEDIT), EM_EXSETSEL, 0, (LPARAM)&cr);
	HWND hREdit = GetDlgItem(hDlg, IDC_RICHEDIT);

	switch (I->type)
	{
	case SPECIAL::clock:
		{
			CClockDlg::CLOCK* pClock = (CClockDlg::CLOCK*)I->pElem;
			OnAddClockText(hDlg, hREdit, true, pClock);
		}
		break;
		
	case SPECIAL::repet:
		{
			GenListCtrl::REPETE* pRepete = (GenListCtrl::REPETE*)I->pElem;
			OnAddRepText(hDlg, hREdit, true, pRepete);
		}
		break;

	case SPECIAL::songdet:
		{
		}
		break;

	case SPECIAL::lyrics:
		{
		}
		break;
	}

}

void CStatusEditorDlg::ClearSelected(HWND hDlg, HWND hREdit)
{
	//we must first get the selection:
	CHARRANGE cr;
	SendMessage(hREdit, EM_EXGETSEL, 0, (LPARAM)&cr);

	//now we must check all specials to see if they are inside
	SendMessage(hDlg, WM_COMMAND, IDC_REMOVETEXT, 0);
	SendMessage(hREdit, WM_KEYDOWN, VK_DELETE, 0);
}