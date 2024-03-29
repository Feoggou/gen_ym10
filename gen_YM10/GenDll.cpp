#include "GenDll.h"
#include "ChooseDlg.h"
#include "LyricsEditorDlg.h"
#include "AboutDlg.h"
#include "StatusEditorDlg.h"
#include "LyricsDlg.h"
#include "SongDetDlg.h"
#include "GenListCtrl.h"
#include "Tools.h"

#ifdef _DEGBUG
#include <crtdbg.h>
#endif
#include <shellapi.h>

CGenDll				genDll;
HHOOK				CGenDll::m_MsgHook;

//functie folosita pentru eliminarea spatiilor de la inceputul si de la sfarsitul textului
void TrimString(wstring& wstr);

CGenDll::CGenDll(void)
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_ALLOC_MEM_DF);
#endif

	m_Plugin.description = "Status pentru Y!M 10 de Ghinet Samuel";
	m_Plugin.version = GPPHDR_VER;
	m_Plugin.init = InitPlugin;
	m_Plugin.config = ConfigPlugin;
	m_Plugin.quit = QuitPlugin;

	m_bUseTool = 0;
	m_bShowTray = 1;
	m_dStatusSelected = 0;
	m_bSelOnline = false;
	m_nTime = -1;
	m_CurrentLyrFile.hFile = INVALID_HANDLE_VALUE;

	m_hWinamp			=	NULL;
	m_hChooseDlg		=	NULL;
	m_hAboutDlg			=	NULL;
	m_hLyricsDlg		=	NULL;
	m_hStatusEditorDlg	=	NULL;
	m_hAboutDlg			=	NULL;

	m_hWinampKey = NULL;
	SetupRegistry();
	LoadLyricsInfo();
	LoadStatusMessages();
}

CGenDll::~CGenDll(void)
{
}

LRESULT CALLBACK CGenDll::MessageProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == MSGF_DIALOGBOX)
	{
		MSG* pMsg = (MSG*)lParam;
		if (pMsg->message == WM_KEYDOWN)
		{
			WCHAR wstr[100];
			GetClassNameW(pMsg->hwnd, wstr, 100);
			if (wcscmp(wstr, L"Edit") == 0)
			{
				GetClassNameW(GetParent(pMsg->hwnd), wstr, 100);
				if (wcscmp(wstr, L"TABLECLASS") == 0 && (pMsg->wParam == VK_TAB || pMsg->wParam == VK_RETURN))
				{
					SendMessage(pMsg->hwnd, WM_KEYDOWN, pMsg->wParam, pMsg->lParam);
					return 1;
				}
			}
		}
	}

	return CallNextHookEx(m_MsgHook, nCode, wParam, lParam);
}

int CGenDll::InitPlugin(void)
{
	genDll.m_hInstance = genDll.m_Plugin.hDllInstance;
	genDll.m_hWinamp = genDll.m_Plugin.hwndParent;

	//se instaleaza functia hook
	m_MsgHook = SetWindowsHookEx(WH_MSGFILTER, MessageProc, 0, GetCurrentThreadId());

	if (genDll.m_bShowTray)
		genDll.CreateTrayIcon();
	//aici setam timer-ul, care cauta fiecare modificare legata de Y!M
	SetTimer(genDll.m_hWinamp, IDT_SEEKYMESS, 1000, genDll.OnTimer);

	genDll.m_OldWinampProc = (WNDPROC)SetWindowLongPtrW(genDll.m_Plugin.hwndParent, GWL_WNDPROC, (LONG)NewWinampProc);

	return 0;
}
void CGenDll::ConfigPlugin(void)
{
	if (!genDll.m_bShowTray)
	{
		CChooseDlg chooseDlg;
		chooseDlg.DoModal(genDll.m_hWinamp);
	}
	else MessageBoxW(genDll.m_hWinamp, L"Nu poți configura plugin-ul de aici atâta timp cât iconul din System Tray este disponibil.\
\nFolosește iconul pentru configurare.", L"Atenție!", MB_ICONWARNING);
}

void CGenDll::QuitPlugin(void)
{
	if (genDll.m_CurrentLyrFile.hFile != INVALID_HANDLE_VALUE)
		CloseHandle(genDll.m_CurrentLyrFile.hFile);

	if (genDll.m_bUseTool && genDll.m_bSelOnline)
	{
		genDll.m_wsDisplayedStatus = L"";
		genDll.DisplayNewStatusText();
	}

	genDll.DestroyTrayIcon();

	if (m_MsgHook)
	{
		UnhookWindowsHookEx(m_MsgHook);
		m_MsgHook = 0;
	}

	genDll.FinalUpdateRegistry();

	RegCloseKey(genDll.m_hWinampKey);

	deque<STATUS>::iterator I;
	for (I = genDll.m_StatusMsgs.begin(); I != genDll.m_StatusMsgs.end(); I++)
	{
		I->Statuses.clear();
		deque<SPECIAL>::iterator J;
		for (J = I->Specials.begin(); J != I->Specials.end(); J++)
		{
			switch (J->type)
			{
			case SPECIAL::clock:
				{
					CClockDlg::CLOCK* pClock = (CClockDlg::CLOCK*)J->pElem;
					pClock->wsTextAfter.clear();
					pClock->wsTextBefore.clear();
					delete pClock;
					J->pElem = 0;
				}
				break;//case SPECIAL::Type::clock

			case SPECIAL::repet:
				{
					GenListCtrl::REPETE* pRepete = (GenListCtrl::REPETE*)J->pElem;
					pRepete->Verses.clear();

					delete pRepete;
					J->pElem = 0;
				}
				break;//SPECIAL::Type::repet

			case SPECIAL::songdet:
				{
					CSongDetDlg::SONGDET* pSongDet = (CSongDetDlg::SONGDET*)J->pElem;
					delete pSongDet;
					J->pElem = 0;
				}
				break;//case SPECIAL::Type::songdet
			}
		}
	}

	genDll.m_StatusMsgs.clear();
}

extern "C" {
__declspec( dllexport ) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin()
{
	return &genDll.m_Plugin;
}
};

void CGenDll::CreateTrayIcon(void)
{
	m_hIcon = LoadIconW(m_hInstance, MAKEINTRESOURCEW(IDI_TRAYICON));

	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = m_Plugin.hwndParent;
	nid.uID = SYSTRAY_ICON;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_TRAYMSG;
	nid.hIcon = m_hIcon;
	wcscpy_s(nid.szTip, 53, L"Unealtă pentru schimbat statusul la Yahoo! Messenger");

	Shell_NotifyIcon(NIM_ADD, &nid);
}

void CALLBACK CGenDll::OnTimer(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	UNREFERENCED_PARAMETER(dwTime);
	UNREFERENCED_PARAMETER(hWnd);
	UNREFERENCED_PARAMETER(uMsg);

	switch (idEvent)
	{
		//acesta este de asemenea timer-ul pentru schimbarea statusului
	case IDT_SEEKYMESS:
		{
			genDll.m_OnlineIDs.clear();
			HWND lastWnd = NULL;
			do
			{
				lastWnd = FindWindowEx(NULL, lastWnd, L"YahooBuddyMain", NULL);
				if (lastWnd!=NULL)
				{
					if (FindWindowEx(lastWnd, 0, L"YTopWindow", 0)!=NULL)
					{
						HWND hWnd = FindWindowEx(lastWnd, 0, L"#32770", L"YLoginWnd");
						if (hWnd==0) 
						{
							MessageBox(genDll.m_hWinamp, L"Eroare neasteptata!", 0, MB_ICONERROR);
#ifdef _DEBUG
							DebugBreak();
#endif
							return;
						}

						hWnd = GetDlgItem(hWnd, 0xD3);
						int nLen = SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0);
						nLen++;
						wchar_t* wsID = new wchar_t[nLen];
						SendMessage(hWnd, WM_GETTEXT, (WPARAM)nLen, (LPARAM)wsID);
						ID id;
						id.hWnd = lastWnd;
						id.wsID = wsID;
						genDll.m_OnlineIDs.push_back(id);
						delete[] wsID;
					}
				}
			}while (lastWnd!=NULL);

			if (genDll.m_hChooseDlg)
				SendMessage(genDll.m_hChooseDlg, WM_UPDATEIDS, 0, 0);
			else
			{
				int size = genDll.m_OnlineIDs.size();
				for (int i = 0; i < size; i++)
					if (wcscmp(genDll.m_OnlineIDs[i].wsID.c_str(), genDll.m_ChosenID.wsID.c_str()) == 0)
					{
						genDll.m_bSelOnline = true;
						genDll.m_ChosenID.hWnd = genDll.m_OnlineIDs[i].hWnd;
						break;
					}
			}

			//se schimba statusul
			if (genDll.m_bUseTool && genDll.m_bSelOnline && genDll.m_dStatusSelected > -1 && genDll.m_ChosenID.hWnd)
			{
				if (genDll.m_nTime == -1) genDll.m_nTime = 0;

				wstring status;
				genDll.CompileStatusText(status);
				if (_wcsicmp(status.c_str(), genDll.m_wsDisplayedStatus.c_str()) != 0)
				{
					genDll.m_wsDisplayedStatus = status;
					genDll.DisplayNewStatusText();
				}

				genDll.m_nTime++;
			}
		}
		break;
	}
}

LRESULT CALLBACK CGenDll::NewWinampProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_TRAYMSG:
		switch (lParam)
		{
		case WM_RBUTTONUP:
			//se afiseaza meniul
			HMENU hMenu = LoadMenu(genDll.m_hInstance, MAKEINTRESOURCE(IDR_TRAYMENU));
			hMenu = GetSubMenu(hMenu, 0);
			POINT point;
			GetCursorPos(&point);
			TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, 
				0, genDll.m_hWinamp, NULL);

			break;//case WM_RBUTTONUP
		}
		break;//case WM_TRAYMSG

	case WM_COMMAND:
		if (HIWORD(wParam)==0)
			switch LOWORD(wParam)
			{
			case ID_CHOOSESTATUS: 
				{
					CChooseDlg c_dlg;
					c_dlg.DoModal(hWnd);
				}
				break;//case ID_CHOOSESTATUS
			case ID_EDITLYRICS:
				{
					CLyricsEditorDlg dlg;
					dlg.DoModal(hWnd);
				}
				break;//case ID_CHOOSESTATUS
			case ID_STATUSEDITOR: 
				{
					CStatusEditorDlg sedDlg;
					sedDlg.DoModal(hWnd);
				}
				break;//case ID_CREATESTATUS
			case ID_CREATELYRICS: 
				{
					CLyricsDlg cl_dlg;
					cl_dlg.DoModal(hWnd);
				}
				break;//case ID_CREATELYRICS
			case ID_REMOVEICON: 
				{
					genDll.DestroyTrayIcon();
					genDll.m_bShowTray = false;
				}
				break;//case ID_REMOVEICON

			case ID_SHOWABOUT:
				{
					CAboutDlg aboutDlg;
					aboutDlg.DoModal(hWnd);
				}
				break;
			}
	break;//case WM_COMMAND
	}

	return CallWindowProc(genDll.m_OldWinampProc, hWnd, uMsg, wParam, lParam);
}

void CGenDll::DestroyTrayIcon(void)
{
	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = m_Plugin.hwndParent;
	nid.uID = SYSTRAY_ICON;
	nid.uFlags = NIF_ICON;
	nid.hIcon = m_hIcon;

	Shell_NotifyIcon(NIM_DELETE, &nid);
}

void CGenDll::SetupRegistry(void)
{
	//cautam key-ul Winamp-ului in registrii windows-ului
	WCHAR strError[200];
	//mai intai, utilizatorul curent
	int nResult = RegOpenCurrentUser(KEY_WRITE | KEY_READ, &m_hWinampKey);
	if (ERROR_SUCCESS != nResult)
	{
		DisplayError(nResult);
		return;
	}

	nResult = RegOpenKeyEx(m_hWinampKey, L"Software", 0, KEY_WRITE | KEY_READ, &m_hWinampKey);
	if (ERROR_SUCCESS != nResult)
	{
		DisplayError(nResult);
		return;
	}

	nResult = RegCreateKeyEx(m_hWinampKey, L"Winamp", 0, NULL,  REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL, &m_hWinampKey, NULL);
	if (ERROR_SUCCESS != nResult)
	{
		DisplayError(nResult);
		return;
	}

	//VALORI: 1.Use Tool (Daca se foloseste unealta)
	BYTE data = 0;
	DWORD dwType, dwSize = 1;
	nResult = RegQueryValueEx(m_hWinampKey, L"gen_YM10 use tool", 0, &dwType, &data, &dwSize);
	if (ERROR_FILE_NOT_FOUND == nResult)
	{
		if (0 != RegSetValueEx(m_hWinampKey, L"gen_YM10 use tool", 0, REG_BINARY, &data, 1))
		{
			DisplayError(nResult);
			return;
		}
		else m_bUseTool = (data != 0);
	}
	else if (nResult == ERROR_MORE_DATA)
	{
		wsprintf(strError, L"ERROR_MORE_DATA la initializare, e nevoie de %d bytes", &dwSize);
		MessageBox(genDll.m_hWinamp, strError, L"Eroare in gen_YM10.dll", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
		return;
	}
	else if (nResult == ERROR_SUCCESS) m_bUseTool = (data != 0);
	else
	{
		DisplayError(nResult);
		return;
	}

	//2. Show Tray Icon (daca sa se afiseze icon-ul in tray sau nu)
	data = 1;
	dwSize = 1;
	nResult = RegQueryValueEx(m_hWinampKey, L"gen_YM10 show tray icon", 0, &dwType, &data, &dwSize);
	if (ERROR_FILE_NOT_FOUND == nResult)
	{
		if (0 != RegSetValueEx(m_hWinampKey, L"gen_YM10 show tray icon", 0, REG_BINARY, &data, 1))
		{
			DisplayError(nResult);
			return;
		}
		else m_bShowTray = (data != 0);
	}
	else if (nResult == ERROR_SUCCESS) m_bShowTray = (data != 0);
	else
	{
		DisplayError(nResult);
		return;
	}

	//3. index-ul statusului selectat (din lista)
	data = 0;
	dwSize = 1;
	nResult = RegQueryValueEx(m_hWinampKey, L"gen_YM10 selected status", 0, &dwType, &data, &dwSize);
	if (ERROR_FILE_NOT_FOUND == nResult)
	{
		if (0 != RegSetValueEx(m_hWinampKey, L"gen_YM10 selected status", 0, REG_BINARY, &data, 1))
		{
			DisplayError(nResult);
			return;
		}
		else m_dStatusSelected = data;
	}
	else if (nResult == ERROR_SUCCESS) m_dStatusSelected = data;
	else
	{
		DisplayError(nResult);
		return;
	}
}

void CGenDll::LoadStatusMessages()
{
	wstring wsPath = genDll.m_wsAppData;
	wsPath += L"\\*.stt";

	WIN32_FIND_DATA f_data = {0};
	BOOL bResult =0;
	int fileNr = 0;
	//cautam fisiere de extensie ".stt"
	HANDLE hSearch = FindFirstFile(wsPath.data(), &f_data);
	while (hSearch != INVALID_HANDLE_VALUE)
	{
		//citim numarul fisierului. ne asiguram ca numele e de forma <numar>.stt
		wstring wsFileName = f_data.cFileName;
		//eliminam extensia
		wsFileName = wsFileName.erase(wsFileName.length() - 4, 4);
		//gasim numarul fisierului
		fileNr = _wtoi(wsFileName.c_str());
		if (fileNr == 0)
		{
			bResult = FindNextFile(hSearch, &f_data);
			if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
			continue;
		}

		//deschidem fisierul. el va avea numele complet, sfn.
		wstring sfn = m_wsAppData;
		sfn += '\\';
		sfn += f_data.cFileName;
		//il deschidem
		HANDLE hFile = CreateFile(sfn.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
		if (hFile == INVALID_HANDLE_VALUE) 
		{
			DisplayError(0);
			bResult = FindNextFile(hSearch, &f_data);
			if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
			continue;
		}

		//citim continutul
		STATUS status;
		status.dFileNr = fileNr;
		if (ReadStatusFile(hFile, status))
			m_StatusMsgs.push_back(status);
		CloseHandle(hFile);

		bResult = FindNextFile(hSearch, &f_data);
		if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
	}
	FindClose(hSearch);
}

void CGenDll::LoadLyricsInfo()
{
	HKEY hKey;
	int nResult;
	
	nResult = RegOpenCurrentUser(KEY_WRITE | KEY_READ, &hKey);
	if (nResult != ERROR_SUCCESS)
	{
		DisplayError(nResult);
		return;
	}

	nResult = RegOpenKeyEx(hKey, L"Volatile Environment", 0, KEY_READ, &hKey);
	if (0 != nResult)
	{
		DisplayError(nResult);
		return;
	}

	DWORD dwType;
	DWORD dwSize = MAX_PATH * 2;
	WCHAR wstr[MAX_PATH];
	nResult = RegQueryValueEx(hKey, L"APPDATA", 0, &dwType, (byte*)wstr, &dwSize);
	if (nResult == ERROR_MORE_DATA)
	{
		DisplayError(nResult);
	}

	if (ERROR_SUCCESS != nResult)
	{
		DisplayError(nResult);
		return;
	}

	RegCloseKey(hKey);

	//se creeaza sau deschide directorul:
	wstring wsPath = wstr;
	wsPath += L"\\Winamp";
	CreateDirectory(wsPath.data(), 0);

	wsPath += L"\\Plugins";
	CreateDirectory(wsPath.data(), 0);

	wsPath += L"\\Samuel App";
	CreateDirectory(wsPath.data(), 0);

	m_wsAppData = wsPath;
	wsPath += L"\\*.lyr";

	LYRICSINFO info;
	WIN32_FIND_DATA f_data = {0};

	BOOL bResult = FALSE;
	int fileNr = 0;
	//gasim fisiere de tipul .lyr
	HANDLE hSearch = FindFirstFile(wsPath.data(), &f_data);
	while (hSearch != INVALID_HANDLE_VALUE)
	{
		//ne asiguram ca e de forma <numar>.lyr
		wstring wsFileName = f_data.cFileName;
		wsFileName = wsFileName.erase(wsFileName.length() - 4, 4);
		fileNr = _wtoi(wsFileName.c_str());
		if (fileNr == 0)
		{
			bResult = FindNextFile(hSearch, &f_data);
			if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
			continue;
		}
		
		//deschidem fisierul. mai intai, construim numele intreg al fisierului.
		wstring sfn = m_wsAppData;
		sfn += '\\';
		sfn += f_data.cFileName;
		//il deschidem
		HANDLE hFile = CreateFile(sfn.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
		if (hFile == INVALID_HANDLE_VALUE) 
		{
			DisplayError(0);
			bResult = FindNextFile(hSearch, &f_data);
			if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
			continue;
		}

		//citim continutul
		//antet:			'LYR'
		//continut:			int len_artist
		//					wchar[] wsArtist
		//					int len_song
		//					wchar[] wsSong

		char sLyr[3] = "";
		DWORD dwRead;
		ReadFile(hFile, sLyr, 3, &dwRead, 0);
		if (sLyr[0] != 'L' && sLyr[1] != 'Y' && sLyr[2] != 'R')
		{
			CloseHandle(hFile);
			bResult = FindNextFile(hSearch, &f_data);
			if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
			continue;
		}

		//wsArtist
		int len;
		ReadFile(hFile, &len, sizeof(int), &dwRead, 0);
		WCHAR* wsArtist = new WCHAR[len];
		ReadFile(hFile, wsArtist, len * 2, &dwRead, 0);


		//wsSong
		ReadFile(hFile, &len, sizeof(int), &dwRead, 0);
		WCHAR* wsSong = new WCHAR[len];
		ReadFile(hFile, wsSong, len * 2, &dwRead, 0);
		
		info.wsArtist = wsArtist;
		info.wsSong = wsSong;
		delete[] wsArtist;
		delete[] wsSong;

		info.nFileNr = fileNr;

		//adaugam in lista de LYRICSINFO
		m_LyricsFiles.push_back(info);

		CloseHandle(hFile);

		bResult = FindNextFile(hSearch, &f_data);
		if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
	}
	FindClose(hSearch);
}

bool CGenDll::ReadStatusFile(HANDLE hFile, STATUS& Status)
{
	//fisierele sunt de tipul: 1.stt, 2.stt, ...

	//file format:
	//ANTET:				'STT'
//CONTINUT:					byte len
	//						wchar_t wstr
	//						byte nrStrings
	//						byte lenwstr1
	//						wchar_t wstr1
	//						...
	//						byte nrSpecials (trebuie sa fie specificat, chiar daca = 0)
	//						byte pos - pozitia in sir,0 = inainte de primul sir.
	//						byte type: clock = 0, repet = 1, songdet = 2, lyrics = 3
	//						date_specifice
	//						...

	//date_specifice: clock
	//						byte hours
	//						byte minutes
	//						byte len_bef
	//						wchar_t wsBefore[len_bef]
	//						byte len_after
	//						wchar_t wsAfter[len_after]

	//date_specifice: repet:
	//						int nRepCnt
	//						int nrVerses
	//						WORD nrSec1
	//						byte length1
	//						wchar_t verse[length1]
	//						...

	//date_specifice: songdet:
	//						byte flags
	//bits:					artist = 1, song = 2, progress = 4, length = 8, percent = 16
	
	//date_specifice: lyrics:	nimic!

	//acum il citim
	DWORD dwRead;
	char stype[3];
	ReadFile(hFile, stype, 3, &dwRead, 0);
	if (stype[0] != 'S' || stype[1] != 'T' || stype[2] != 'T') return false;

	//lungimea:
	byte len;
	ReadFile(hFile, &len, 1, &dwRead, 0);
	wchar_t* wsText = new wchar_t[len];
	//sText
	ReadFile(hFile, wsText, len * 2, &dwRead, 0);
	Status.wsText = wsText;
	delete[] wsText;

	Status.Specials.clear();
	Status.Statuses.clear();

	byte sz;//Status.Statuses.size();
	ReadFile(hFile, &sz, 1, &dwRead, 0);

	//acum citim, pentru fiecare sir, lungimea si sirul
	for (byte i = 0; i < sz; i++)
	{
		ReadFile(hFile, &len, 1, &dwRead, 0);
		WCHAR* wsir = new WCHAR[len];
		ReadFile(hFile, wsir, len * 2, &dwRead, 0);
		Status.Statuses.push_back(wsir);
		delete[] wsir;
	}

	//acum citim specialele. mai intai, numarul lor
	ReadFile(hFile, &sz, 1, &dwRead, 0);
	byte pos;
	for (byte i = 0; i < sz; i++)
	{
		SPECIAL spec;
		//acum citim pozitia in sir. 0 = inainte de primul
		ReadFile(hFile, &pos, 1, &dwRead, 0);
		spec.pos = pos;
		//tipul
		byte type;
		ReadFile(hFile, &type, 1, &dwRead, 0);
		spec.type = (SPECIAL::Type)type;
		//acum depinde de tip
		switch (type)
		{
		case SPECIAL::clock: 
			{
				//se citesc 3 bytes
				CClockDlg::CLOCK* pClock = new CClockDlg::CLOCK;
				byte val;//dHours;
				ReadFile(hFile, &val, 1, &dwRead, 0);
				pClock->dHours = val;

				//dMins;
				ReadFile(hFile, &val, 1, &dwRead, 0);
				pClock->dMins = val;
				
				//len bef
				ReadFile(hFile, &val, 1, &dwRead, 0);
				WCHAR* wstrText = new WCHAR[val];
				//str bef
				ReadFile(hFile, wstrText, val * 2, &dwRead, 0);
				pClock->wsTextBefore = wstrText;
				delete[] wstrText;

				//len after
				ReadFile(hFile, &val, 1, &dwRead, 0);
				wstrText = new WCHAR[val];
				//str bef
				ReadFile(hFile, wstrText, val * 2, &dwRead, 0);
				pClock->wsTextAfter = wstrText;
				delete[] wstrText;

				spec.pElem = pClock;
			}
			break;

		case SPECIAL::repet:
			{
			//	int nRepCnt
			//	bool bIsMinute
			//	int nrVerses
			//	WORD nrSec1
			//	byte length1
			//	char verse[length1]
			//	...
				GenListCtrl::REPETE*	pRepete = new GenListCtrl::REPETE;
				int nRepCnt;
				//nRepCnt
				ReadFile(hFile, &nRepCnt, sizeof(int), &dwRead, 0);
				pRepete->nRepCnt = nRepCnt;

				//bIsMinute
				ReadFile(hFile, &pRepete->bIsMinute, 1, &dwRead, 0);

				//nrVerses
				int nrVerses;
				ReadFile(hFile, &nrVerses, sizeof(int), &dwRead, 0);

				//fiecare rand in parte, cu tot cu secunda/minutul lui
				GenListCtrl::VERSE verse;
				for (int i = 0; i < nrVerses; i++)
				{
					//nr secundei/minutului
					ReadFile(hFile, &verse.nrSec, sizeof(WORD), &dwRead, 0);

					//lungimea textului
					byte len;
					ReadFile(hFile, &len, 1, &dwRead, 0);

					//textul
					wchar_t* wsVerse = new wchar_t[len];
					ReadFile(hFile, wsVerse, len * 2, &dwRead, 0);
					verse.wsVerse = wsVerse;
					delete[] wsVerse;

					//se adauga
					pRepete->Verses.push_back(verse);
				}
				spec.pElem = pRepete;
			}
			break;

		case SPECIAL::songdet: 
			{
				//byte flags
				byte det;
				ReadFile(hFile, &det, 1, &dwRead, 0);
				CSongDetDlg::SONGDET* pSongDet = new (CSongDetDlg::SONGDET);
				*pSongDet = det;
				spec.pElem = pSongDet;
			}
			break;

		case SPECIAL::lyrics: 
			spec.pElem = 0;
			break;
		}
		Status.Specials.push_back(spec);
	}

	return true;
}

void CGenDll::FinalUpdateRegistry()
{
	byte data = (byte)m_bUseTool;

	//VALORI: 1.Use Tool (daca se foloseste unealta)
	int nResult = RegSetValueExW(m_hWinampKey, L"gen_YM10 use tool", 0, REG_BINARY, &data, 1);
	if (ERROR_SUCCESS != nResult)
	{
		DisplayError(nResult);
		return;
	}

	data = (byte)m_bShowTray;
	//2. Show Tray Icon (daca se afiseaza icon-ul in tray)
	nResult = RegSetValueExW(m_hWinampKey, L"gen_YM10 show tray icon", 0, REG_BINARY, &data, 1);
	if (ERROR_SUCCESS != nResult)
	{
		DisplayError(nResult);
		return;
	}

	data = (byte)m_dStatusSelected;
	//3. Selected Status (indexul statusului selectat pentru afisare)
	nResult = RegSetValueExW(m_hWinampKey, L"gen_YM10 selected status", 0, REG_BINARY, &data, 1);
	if (ERROR_SUCCESS != nResult)
	{
		DisplayError(nResult);
		return;
	}
}

void CGenDll::CompileStatusText(wstring& status)
{
	m_wsLinkLyrics = L"";
	//avem m_nTime care specifica timpul.
	//avem genDll.m_dStatusSelected si genDll.m_StatusMsgs
	status = L"";
	STATUS stt = genDll.m_StatusMsgs[genDll.m_dStatusSelected];
	
	int nrSpecials = stt.Specials.size();
	if (nrSpecials == 0) 
	{
		status += stt.wsText;
		return;
	}

	int i = 0;
	//transformam primul SPECIAL (daca este la inceput de tot in status), in sir de caractere
	if (stt.Specials[0].pos == 0)
	{
		CompileElement(stt.Specials[0], status);
		i++;
	}
	
	//transformam celelalte SPECIAL-e in sir de caractere
	while (i < nrSpecials)
	{
		if (stt.Specials[0].pos == 0)
			status += stt.Statuses[i - 1];
		else status += stt.Statuses[i];

		CompileElement(stt.Specials[i], status);
		i++;
	};

	i--;
	//daca este text normal dupa ultimul SPECIAL
	if (stt.Statuses.size() > stt.Specials[i].pos)
	{
		status += stt.Statuses.back();
	}
}

void CGenDll::CompileElement(SPECIAL& special, wstring& result)
{
	wstring wstr;
	switch (special.type)
	{
	case SPECIAL::clock:
		{
			//preluam obiectul
			CClockDlg::CLOCK* pClock = (CClockDlg::CLOCK*)special.pElem;
			int time = (pClock->dMins + 1) * 60 + pClock->dHours * 3600 - 1;

			//genDll.m_nTime - timpul curent. porneste cand este pornit statusul.
			time -= genDll.m_nTime;
			//daca este mai putin de un minut, at doar se afiseaza wsTextAfter
			if (time/60 <= 0)
			{
				result += pClock->wsTextAfter;
				return;
			}

			result += pClock->wsTextBefore;

			wchar_t s[7];
			//numarul de ore
			byte nrHours = (byte)(time/3600);
			if (nrHours > 0)
			{
				wsprintf(s, L"%dh", nrHours);
				wstr = s;
				//din timpul masurat de ceas, se scad orele
				time -= nrHours * 3600;
			}

			//numarul de minute
			byte nrMins = (byte)time/60;
			if (nrMins > 0)
			{
				//daca avem si ore, se afiseaza cu spatiu intre
				if (nrHours > 0)
					wsprintf(s, L" %dm", nrMins);
				else wsprintf(s, L"%dm", nrMins);
				wstr += s;
				time -= nrMins * 60;
			}
			
			result += wstr;
		}
		break;

	case SPECIAL::repet:
		{
			GenListCtrl::REPETE* pRepete = (GenListCtrl::REPETE*)special.pElem;
			//genDll.m_nTime este in secunde

			int nBase = pRepete->Verses.front().nrSec + pRepete->Verses.back().nrSec;
			if (pRepete->bIsMinute)
				nBase *= 60;

			int nrRepet = 0;
				
			//nrRepet - a cata repetare a listei suntem. 0 = prima afisare.
			nrRepet = genDll.m_nTime / nBase;

			if (pRepete->nRepCnt > -1 && nrRepet >= pRepete->nRepCnt) return;

			deque<GenListCtrl::VERSE>::iterator I;
			//Verses.end() - 1 este ultimul; Verses.end() == <end>
			for (I = pRepete->Verses.end() - 1; ; I--)
			{
				int nSec = I->nrSec;
				if (I == pRepete->Verses.begin()) nSec = 0;
				if (pRepete->bIsMinute) nSec *= 60;

				if (genDll.m_nTime >= nBase * nrRepet + nSec)
				{
					wstr += I->wsVerse;
					break;
				}

				if (I == pRepete->Verses.begin()) break;
			}
			result += wstr;
		}
		break;

	case SPECIAL::songdet:
		{
			//e oprita melodia?
			int play_state = SendMessage(genDll.m_hWinamp, WM_USER, 0, 104);
			//1 = pornit; 3 = pauza; altfel, e oprit
			if (play_state != 1 && play_state != 3) return;

			//gasim numele intreg al fisierului
			static wstring wsArtist, wsSong, wsAlbum;
			int index = SendMessage(genDll.m_hWinamp, WM_USER, 0, 125);
			char* sPath = (char*)SendMessage(genDll.m_hWinamp, WM_USER, index, 211);

			CSongDetDlg::SONGDET* pSongDet = (CSongDetDlg::SONGDET*)special.pElem;

			if (genDll.m_sCurrentSong.compare(sPath) == 0) goto aranjare;
			else
			{
				genDll.m_sCurrentSong = sPath;
				wsArtist = wsSong = wsAlbum = L"";
			}

			//gasesc si artist si album si melodie, in caz ca sunt mai multe SongInfo in status

			FindSIFromWinamp(wsArtist, wsSong);
			FindAlbumName(wsAlbum);

			if (*pSongDet & CSongDetDlg::lyrics)
				BuildLinkLyrics(wsArtist, wsSong);

aranjare:
			bool bArtist = (*pSongDet & CSongDetDlg::artist) && wsArtist.length();
			bool bSong = (*pSongDet & CSongDetDlg::song) && wsSong.length();
			bool bAlbum = (*pSongDet & CSongDetDlg::album) && wsAlbum.length();
			//aranjarea in wstr: daca e Artist si Melodie
			if (bArtist && bSong)
			{
				wstr = wsArtist;
				wstr += L" - ";

				//daca e Album
				if (bAlbum)
				{
					wstr += wsAlbum;
					wstr += L" - ";
				}

				wstr += wsSong;
			}
			//daca e artist, dar nu e melodie
			else if (bArtist)
			{
				wstr = wsArtist;
				//daca e Album
				if (bAlbum)
				{
					wstr += L" - ";
					wstr += wsAlbum;
				}
			}
			//daca e melodie, dar nu e artist
			else if (bSong)
			{
				//daca e Album
				if (bAlbum)
				{
					wstr = wsAlbum;
					wstr += L" - ";
				}
				wstr += wsSong;
			}

			//daca nu e nici melodie nici artist, dar e album
			else if (bAlbum)
			{
				wstr += wsAlbum;
			}

			//daca specificam si durata melodiei
			if (*pSongDet & CSongDetDlg::length)
			{
				wstring wsLen;
				GetCurrentSongLength(wsLen);
				wstr += wsLen;
			}

			if (*pSongDet & CSongDetDlg::progress)
			{
				if (wstr.length()) wstr += L" ";
				wstring wsProgress;
				GetCurrentSongProgress(wsProgress, (*pSongDet & CSongDetDlg::percent) != 0);
				wstr += wsProgress;
			}
			if (play_state == 3)
				wstr += L"(Pauză)";

			result += wstr;
		}
		break;

	case SPECIAL::lyrics:
		{
			//e oprita melodia?
			int play_state = SendMessage(genDll.m_hWinamp, WM_USER, 0, 104);
			//1 = pornit; 3 = pauza; altfel, e oprit
			if (play_state != 1 && play_state != 3) return;

			wstring wsArtist, wsSong;
			//le gasim din Winamp
			FindSIFromWinamp(wsArtist, wsSong);
			
			//acum cautam in lista noastra sa vedem daca il avem:
			deque<LYRICSINFO>::iterator I;
			bool bOK = false;
			for (I = genDll.m_LyricsFiles.begin(); I != genDll.m_LyricsFiles.end(); I++)
			{
				I->wsSong;
				if (wsSong == I->wsSong)
				{
					if (wsArtist.length())
					{
						if (wsArtist == wsArtist)
							bOK = true;
						else bOK = false;
					}

					if (!bOK) continue;
				}
				if (bOK == true) break;
			}
			
			if (bOK == false) return;

			//trebuie sa gasim secunda din vers (rand)
			//dar, mai intai, trebuie sa incarcam fisierul lyrics, daca nu am facut asta deja
			wstring wsFileName = genDll.m_wsAppData;
			wsFileName += L"\\";
			WCHAR wsFileNr[15];
			_itow_s(I->nFileNr, wsFileNr, 15, 10);
			wsFileName += wsFileNr;
			wsFileName += L".lyr";
			//daca avem un fisier, iar numele fisierelor difera
			if (wsFileName != genDll.m_CurrentLyrFile.wsFileName)
			{
				if (genDll.m_CurrentLyrFile.hFile != INVALID_HANDLE_VALUE)
					CloseHandle(genDll.m_CurrentLyrFile.hFile);
				genDll.m_CurrentLyrFile.Verses.clear();
				//se creeaza fisierul
				genDll.m_CurrentLyrFile.wsFileName = wsFileName;
				genDll.m_CurrentLyrFile.hFile = CreateFile(wsFileName.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

				//acum citim fisierul.
				//formatul fisierului:
				//ANTET:				'LYR'
				//						int len_artist
				//						wchar[] wsArtist
				//						int len_song
				//						wchar[] wsSong
				//						.............
				//CONTINUT:					sec (word)
				//							text len(byte)
				//							text (wchar_t*)

				//ne deplasam la len_artist:
				SetFilePointer(genDll.m_CurrentLyrFile.hFile, 3, 0, FILE_BEGIN);
				//citim len_artist
				DWORD dwRead;
				int len;
				ReadFile(genDll.m_CurrentLyrFile.hFile, &len, sizeof(int), &dwRead, 0);
				//ne deplasam la len_song, wsArtist E WIDE
				SetFilePointer(genDll.m_CurrentLyrFile.hFile, len * 2, 0, FILE_CURRENT);
				
				//citim len_song
				ReadFile(genDll.m_CurrentLyrFile.hFile, &len, sizeof(int), &dwRead, 0);
				//ne deplasam la prima secunda, wsSong e WIDE
				SetFilePointer(genDll.m_CurrentLyrFile.hFile, len * 2, 0, FILE_CURRENT);
				
				WORD sec;
				GenListCtrl::VERSE verse;
				do
				{
					//de aici folosesc len ca BYTE, numai ca int.
					BYTE len;
					//intrucat nu se stie numarul de versuri, se citeste pana nu mai exista date
					if (ReadFile(genDll.m_CurrentLyrFile.hFile, &sec, 2, &dwRead, 0) != 0 && dwRead == 0)
						break;

					verse.nrSec = sec;
					ReadFile(genDll.m_CurrentLyrFile.hFile, &len, 1, &dwRead, 0);
					wchar_t* the_verse = new wchar_t[len];
					ReadFile(genDll.m_CurrentLyrFile.hFile, the_verse, len * 2, &dwRead, 0);
					verse.wsVerse = the_verse;
					delete[] the_verse;

					genDll.m_CurrentLyrFile.Verses.push_back(verse);
				}while (dwRead != 0);
			}
			//acum, avem cu siguranta fisierul incarcat, versurile, etc.
			//se preia secunda... dar, mai intai, trebuie sa gasim secunda melodiei.
			//pozitia secundei
			int nrSec = SendMessage(genDll.m_hWinamp, WM_USER, 0, 105);
			nrSec /= 1000;
			deque<GenListCtrl::VERSE>::iterator J;
			for (J = genDll.m_CurrentLyrFile.Verses.begin(); J != genDll.m_CurrentLyrFile.Verses.end(); J++)
			{
				//daca suntem inainte de prima secunda din melodie
				if (nrSec < J->nrSec && J == genDll.m_CurrentLyrFile.Verses.begin())
				{
					wstr = L"";
					break;
				}
				else if (nrSec < J->nrSec)
				{
					wstr = (J-1)->wsVerse;
					break;
				}

				//daca suntem exact la secunda din melodie
				else if (nrSec == J->nrSec)
				{
					wstr = J->wsVerse;
					break;
				}

				else if (nrSec > J->nrSec && J == genDll.m_CurrentLyrFile.Verses.end()-1)
				{
					wstr = J->wsVerse;
				}
			}

			result += wstr;
		}
		break;
	}
}

void CGenDll::DisplayNewStatusText()
{
	HWND hWnd = genDll.m_ChosenID.hWnd;
	if (hWnd == 0) return;
	hWnd = FindWindowEx(hWnd, 0, L"YTopWindow", 0);
	if (hWnd == 0) return;
	hWnd = FindWindowEx(hWnd, 0, L"CMerlinWndBase", 0);
	HWND hMerlin = hWnd;
	if (hWnd == 0) return;

	HWND hStatus, hLink;

	//trebuie sa gasim fereastra ce reprezinta textul de status si fereastra pt textul de link
	//clasa YUI_Win32Edit
	//status: altceva decat numele de fereastra ce incepe cu "http://"
	//link: incepe cu "http://"
	//nu are id fix, nici ordinea in care sunt create nu e fixa
	hWnd = FindWindowEx(hMerlin, 0, L"YUI_Win32Edit", 0);
	wchar_t sir[10];
	SendMessage(hWnd, WM_GETTEXT, 8, (LPARAM)sir);
	wstring wsData = sir;
	if (wsData.find(L"http://") != -1 || wsData.find(L"www.") != -1)
	{
		hLink = hWnd;
		hStatus = FindWindowEx(hMerlin, hWnd, L"YUI_Win32Edit", 0);
	}
	else
	{
		hStatus = hWnd;
		hLink = FindWindowEx(hMerlin, hWnd, L"YUI_Win32Edit", 0);
	}

	_ASSERT(hStatus);
	_ASSERT(hLink);

	//cautam de la sfarsit la inceput, un "www." sau "http://"
	int pos1 = genDll.m_wsDisplayedStatus.rfind(L"www.");
	int pos2 = genDll.m_wsDisplayedStatus.rfind(L"http://");

	if (pos1 == -1 && pos2 == -1 || m_wsLinkLyrics.length())
	{
		//nu exista nici "www." nici "http://"
		//se afiseaza text normal in hStatus
		SendMessage(hStatus, WM_SETTEXT, 0, (LPARAM)genDll.m_wsDisplayedStatus.c_str());
		if (m_wsLinkLyrics.length())
			SendMessage(hLink, WM_SETTEXT, 0, (LPARAM)m_wsLinkLyrics.c_str());
		SendMessage(hStatus, WM_KILLFOCUS, 0, 0);
		return;
	}
	else
	{
		wstring wsStatus, wsLink;
		//exista si "www." si "http://", sau numai unul dintre ele (in acest caz, un pos este -1).
		//il luam pe cel ce e mai in dreapta
		pos1 = max(pos1, pos2);
		//gasim primul '\n' sau ' ' dupa pos1
		pos2 = genDll.m_wsDisplayedStatus.find_first_of(L"\n ", pos1);
		if (pos2 == -1) pos2 = genDll.m_wsDisplayedStatus.length() - 1;
		wsLink = genDll.m_wsDisplayedStatus.substr(pos1, pos2 - pos1 + 1);
		wsStatus = genDll.m_wsDisplayedStatus;
		wsStatus.erase(pos1, pos2 - pos1 + 1);
		
		//se afiseaza
		SendMessage(hStatus, WM_SETTEXT, 0, (LPARAM)wsStatus.c_str());
		SendMessage(hLink, WM_SETTEXT, 0, (LPARAM)wsLink.c_str());
		SendMessage(hStatus, WM_KILLFOCUS, 0, 0);
	}
}

void CGenDll::GetCurrentSongProgress(wstring& wsProgress, bool bUsePercent)
{
	//in milisecunde
	int pos = SendMessage(genDll.m_hWinamp, WM_USER, 0, 105);
	pos /= 1000;//acum e in secunde
	int length = SendMessage(genDll.m_hWinamp, WM_USER, 1, 105);//lungimea in secunde
	int percent = pos * 100 / length;//xx %
	//convertim in zeci de procente: 10%, 20%, 30%,...
	percent /= 10;

	if (bUsePercent)
	{
		percent *= 10;
		wchar_t str[10];
		wsprintf(str, L"(%d%%)", percent);
		wsProgress = str;
	}
	else
	{
		wchar_t str[] = L"[----------]";
		str[percent + 1] = '|';
		wsProgress = str;
	}
}

void CGenDll::GetCurrentSongLength(wstring& wsLen)
{
	int length = SendMessage(genDll.m_hWinamp, WM_USER, 1, 105);//durata in secunde
	wchar_t str[15];

	int nMins = length / 60;
	int nSecs = length % 60;

	if (nSecs > 9)
		wsprintf(str, L"(%d:%d)", nMins, nSecs);
	else
		wsprintf(str, L"(%d:0%d)", nMins, nSecs);
	wsLen = str;
}

void CGenDll::FindSIFromWinamp(wstring& wsArtist, wstring& wsSong)
{
	//luam informatiile din winamp
	//spre exemplu
	//736. Artist - Melodie - Winamp
	
	//pana la primul punct + caracterul ' ' este irelevant (aici, '736.')
	//ultimul ' - Winamp' este irelevant
	//apoi urmeaza: "Artist - Melodie" sau "Melodie"; dupa ce elimin " - Winamp" vad daca mai gasesc '-'

	//textul din winamp:
	wstring wsText;
	int len = GetWindowTextLength(genDll.m_hWinamp);
	wchar_t* text = new wchar_t[len+1];
	GetWindowText(genDll.m_hWinamp, text, len+1);
	wsText = text;
	delete[] text;
	
	//eliminam textul irelevant
	int x = wsText.find('.', 0);
	wsText.erase(0, x + 2);
	x = wsText.rfind('-');
	//daca nu este cantata nici o melodie in mod curent, nu va exista nici un '-' in sir.
	if (x == -1) return;
	wsText.erase(x-1, wsText.length()-1);

	//acum rezultatul este sub forma "xxxx - xxxx" sau in formatul "xxxx"
	//vedem daca mai gasim un caracter '-'.
	//daca gasesc, impart sirul in doua.

	x = wsText.find(L" - ", 0);
	if (x != -1)
	{
		wsArtist = wsText.substr(0, x);
		wsSong = wsText.substr(x + 3, wsText.length() - x - 3);
	}
	else
	{
		//tot sirul reprezinta numele melodiei.
		wsSong = wsText;
	}
}
void TrimString(wstring& wstr)
 {
	 if (wstr.length() == 0) return;

	 //partea stanga:
	 UINT i = 0;
	 while (i < wstr.length())
	 {
		 if (wstr[i] == ' ' || wstr[i] == 0) wstr.erase(i, 1);
		 else break;
	 };

	 //partea dreapta
	 i = wstr.length() - 1;
	 while (i < wstr.length())
	 {
		 if (wstr[i] == ' ' || wstr[i] == 0) 
		 {
			 wstr.erase(i, 1);
			 i--;
		 }
		 else break;
	 };
 }

void CGenDll::FindByTagID3v2(HANDLE hFile, wstring& wsAlbum)
{
	//umplem structura. vom vedea ce tip este
	WINAMP_TAGV2 tagInfo;
	DWORD dwRead;
	//citim din fisier
	ReadFile(hFile, &tagInfo, 10, &dwRead, 0);

	if (dwRead < 10)
	{
		DisplayError(0);
		return;
	}

	if (tagInfo.cI == 'I' && tagInfo.cD == 'D' && tagInfo.c3 == '3')
	{
		//se calculeaza dwSize
		//marimea frame-ului, data in 4 bytes diferiti, in care, in fiecare, bitul cel mai significant
		//este 0. deci, in fiecare byte sunt stocati 7 biti de informatie.
		DWORD dwSize = (tagInfo.nSize[0] << 7) + (tagInfo.nSize[1] << 7) + (tagInfo.nSize[2] << 7) + tagInfo.nSize[3];
		byte* buffer = new byte[dwSize];

		ReadFile(hFile, buffer, dwSize, &dwRead, 0);
		if (dwRead < dwSize)
		{
			DisplayError(0);
			return;
		}

		//versiunea 2.0
		if (tagInfo.nVer1 == 2)
		{
			FRAME20 frame;
			for (UINT i = 0; i < dwSize - 3; i++)
			{
				if (!(buffer[i] == 'T' && buffer[i+1] == 'A' && buffer[i+2] == 'L'))
					continue;

				frame.cName[0] = buffer[i];
				frame.cName[1] = buffer[i+1];
				frame.cName[2] = buffer[i+2];

				frame.size[0] = buffer[i+3]; frame.size[1] = buffer[i+4]; frame.size[2] = buffer[i+5];
				DWORD dwSize = (buffer[i+3] << 7) + (buffer[i+4] << 7) + buffer[i+5];

				frame.encoding = buffer[i+6];
				//daca se foloseste ANSI
				if (frame.encoding == 0)
				{
					frame.str = new char[dwSize];
					//i + 6 + buffer = buffer[i+6] = encoding. de la i + 7 incepe textul
					strcpy_s(frame.str, dwSize, (char*)(i+7+buffer));
					//ne asiguram ca e terminat null.
					frame.str[dwSize-1] = 0;

					int len = strlen(frame.str);
					for (int i = 0; i < len; i++)
						wsAlbum.push_back(frame.str[i]);

					delete[] frame.str;
					frame.str = 0;
				}
				else//unicode:
				{
					//dwSize e in bytes. am nevoie in numar de caractere (wide)
					frame.wstr = new wchar_t[dwSize/2];
					//buffer[i+6] = encoding. buffer[i+7]=0xFE; buffer[i+8]=0xFF
					memcpy_s(frame.wstr, dwSize-1, (wchar_t*)(i+9+buffer), dwSize-1);
					
					frame.wstr[dwSize/2-1] = 0;
					wsAlbum = frame.wstr;
					delete[] frame.wstr;
					frame.wstr = 0;
				}


				//eliminam spatiile de la inceput si de la sfarsit
				TrimString(wsAlbum);
				break;
			}
		}
		//versiunile 3 si 4
		else if (tagInfo.nVer1 == 3 || tagInfo.nVer1 == 4)
		{
			FRAME34 frame;
			for (UINT i = 0; i < dwSize - 3; i++)
			{
				if (!(buffer[i] == 'T' && buffer[i+1] == 'A' && buffer[i+2] == 'L' && buffer[i+3] == 'B'))
					continue;
			
				//numele frame-ului
				frame.cName[0] = buffer[i];
				frame.cName[1] = buffer[i+1];
				frame.cName[2] = buffer[i+2];
				frame.cName[3] = buffer[i+3];

				//marimea frame-ului, data in 4 bytes diferiti, in care, in fiecare, bitul cel mai significant
				//este 0. deci, in fiecare byte sunt stocati 7 biti de informatie.
				frame.size[0] = buffer[i+4]; frame.size[1] = buffer[i+5]; frame.size[2] = buffer[i+6]; frame.size[3] = buffer[i+7];
				DWORD dwSize = (buffer[i+4] << 7) + (buffer[i+5] << 7) + (buffer[i+6] << 7) + buffer[i+7];
				
				frame.flags = MAKEWORD(buffer[i+8], buffer[i+9]);
				frame.encoding = buffer[i+10];
				//daca este codificare ANSI
				if (frame.encoding == 0)
				{
					frame.str = new char[dwSize];

					//buffer[i+10] = encoding.
					memcpy_s(frame.str, dwSize-1, (char*)(i+11+buffer), dwSize-1);
					frame.str[dwSize-1] = 0;

					int len = strlen(frame.str);
					for (int i = 0; i < len; i++)
						wsAlbum.push_back(frame.str[i]);

					delete[] frame.str;
					frame.str = 0;

				}
				//UTF-16 sau UTF-16BE
				else if (frame.encoding == 1 || frame.encoding == 2)
				{
					frame.wstr = new wchar_t[dwSize/2];
					frame.wstr[dwSize/2-1] = 0;

					//buffer[i+10] = encoding.
					//buffer[i+11]=0xFF; buffer[i+12]=0xFE, deci textul incepe de la i+13
					memcpy_s(frame.wstr, dwSize-1, (wchar_t*)(i+13+buffer), dwSize-1);
					frame.wstr[dwSize/2-1] = 0;

					int len = wcslen(frame.wstr);
					if (frame.encoding == 2) //utf-16be
						for (int i = 0; i < len; i++)
						{
							BYTE lo = LOBYTE(frame.wstr[i]);
							BYTE hi = HIBYTE(frame.wstr[i]);
							//se inverseaza, hi cu lo
							frame.wstr[i] = MAKEWORD(hi, lo);
						}
					wsAlbum = frame.wstr;
					delete[] frame.wstr;
					frame.wstr = 0;
				}

				TrimString(wsAlbum);
				break;
			}
		}

		delete[] buffer;
	}
}

void CGenDll::FindByTagID3v1(HANDLE hFile, wstring& wsAlbum)
{
	//avem fisierul deschis... trebuie sa incarcam ultima parte a lui:
	WINAMP_TAGV1 tagInfo;

	DWORD dwRead;
	SetFilePointer(hFile, -128, 0, FILE_END);
	if (0 == ReadFile(hFile, &tagInfo, 128, &dwRead, 0))
		DisplayError(0);

	if (tagInfo.cT == 'T' && tagInfo.cA == 'A' && tagInfo.cG == 'G')
	{
		if (dwRead != 128)
		{
			DisplayError(0);
			return;
		}

		if (strlen(tagInfo.sAlbumName))
		{
			for (int i = 0; i < 31; i++)
				wsAlbum.push_back(tagInfo.sAlbumName[i]);
			TrimString(wsAlbum);
		}
	}
}

void CGenDll::FindAlbumName(wstring& wsAlbum)
{
	HANDLE hFile = CreateFileA(m_sCurrentSong.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		DisplayError(0);
		return;
	}
	//prima data incercam din ID3v2
	FindByTagID3v2(hFile, wsAlbum);
	//daca nu am gasit, cautam dupa ID3v1
	if (wsAlbum.length() == 0)
		FindByTagID3v1(hFile, wsAlbum);

	CloseHandle(hFile);
}

void CGenDll::BuildLinkLyrics(wstring wsArtist, wstring wsSong)
{
	//inlocuim toate ' ' cu '_', si literele mari cu litere mici
	for (UINT i = 0; i < wsArtist.length(); i++)
	{
		if (wsArtist[i] == ' ') wsArtist[i] = '_';
		wsArtist[i] = (WCHAR)tolower(wsArtist[i]);
	}

	for (UINT i = 0; i < wsSong.length(); i++)
	{
		if (wsSong[i] == ' ') wsSong[i] = '_';
		wsSong[i] = (WCHAR)tolower(wsSong[i]);
	}
	
	if (wsArtist.length() && wsSong.length())
	{
		m_wsLinkLyrics = L"http://www.lyricsmode.com/lyrics/";
		m_wsLinkLyrics += wsArtist[0];
		m_wsLinkLyrics += '/';
		m_wsLinkLyrics += wsArtist;
		m_wsLinkLyrics += '/';
		m_wsLinkLyrics += wsSong;
		m_wsLinkLyrics += L".html";
	}
	else m_wsLinkLyrics = L"http://";
}