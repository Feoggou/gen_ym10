#include "GenDll.h"
#include "ChooseDlg.h"
#include "AboutDlg.h"
#include "StatusEditorDlg.h"
#include "LyricsDlg.h"
#include "SongDetDlg.h"
#include "GenListCtrl.h"
#include "shellapi.h"

#include <crtdbg.h>

CGenDll				genDll;
HHOOK				CGenDll::m_MsgHook;
WNDPROC				CGenDll::OldDlgProc;

CGenDll::CGenDll(void)
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_ALLOC_MEM_DF);
#endif

	m_Plugin.description = "Status pentru Y!M 10 de Ghineț Samuel";
	m_Plugin.version = GPPHDR_VER;
	m_Plugin.init = InitPlugin;
	m_Plugin.config = ConfigPlugin;
	m_Plugin.quit = QuitPlugin;

	m_bUseTool = 0;
	m_bShowTray = 1;
	m_dStatusSelected = 0;
	m_dNrStatusMsgs = 0;
	m_bChooseDlgCreated = false;
	m_bStatusEdtCreated = false;
	m_hChooseDlg = 0;
	m_bSelOnline = false;
	m_nTime = -1;
	m_LyrFile.hFile = INVALID_HANDLE_VALUE;

	m_hKey = NULL;
	SetupRegistry();
	LoadLyricsInfo();
	LoadStatusMessages();
}

CGenDll::~CGenDll(void)
{
//	MessageBox(0, L"~CGenDll destructor", 0, 0);
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
				if (wcscmp(wstr, L"#32770") != 0 && (pMsg->wParam == VK_TAB || pMsg->wParam == VK_RETURN))
				{
					SendMessage(pMsg->hwnd, WM_KEYDOWN, pMsg->wParam, pMsg->lParam);
					return 1;
				}
			}
		}
	}

	return CallNextHookEx(m_MsgHook, nCode, wParam, lParam);
}

INT_PTR CALLBACK CGenDll::NewDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_USER+4096:
		{
			if (lParam == 0x912)
			{
				wcscpy_s(genDll.m_sCurrentSong, MAX_PATH, (WCHAR*)wParam);
				if (genDll.m_LyrFile.hFile != INVALID_HANDLE_VALUE)
				{
					CloseHandle(genDll.m_LyrFile.hFile);
					genDll.m_LyrFile.Verses.clear();
					genDll.m_LyrFile.swFileName = L"";
					genDll.m_LyrFile.hFile = INVALID_HANDLE_VALUE;
				}
			}
		}
		break;
	}

	return CallWindowProc(OldDlgProc, hDlg, uMsg, wParam, lParam);
}

int CGenDll::InitPlugin(void)
{
//	MessageBox(0, L"InitPlugin", 0, 0);

	genDll.m_hInstance = genDll.m_Plugin.hDllInstance;
	genDll.m_hWinamp = genDll.m_Plugin.hwndParent;

	m_MsgHook = SetWindowsHookEx(WH_MSGFILTER, MessageProc, 0, GetCurrentThreadId());

	if (genDll.m_bShowTray)
		genDll.CreateTrayIcon();
	//here we set the timer, to seek every change of Y!Ms
	SetTimer(genDll.m_hWinamp, IDT_SEEKYMESS, 1000, genDll.OnTimer);

	genDll.m_OldParentProc = (WNDPROC)SetWindowLong(genDll.m_Plugin.hwndParent, GWL_WNDPROC, (LONG)NewParentProc);

	HWND hWnd;
	hWnd = FindWindowEx(0, 0, L"Winamp Library", L"BaseWindow_RootWnd");
	hWnd = FindWindowEx(0, 0, L"Winamp Gen", L"Winamp Library");
	hWnd = FindWindowEx(hWnd, 0, L"#32770", 0);

	OldDlgProc = (WNDPROC)SetWindowLongPtrW(hWnd, GWL_WNDPROC, (LONG_PTR)NewDlgProc);

	//now we subclass the dialog that notifies us when the song has changed. first, find the window:

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
//	MessageBox(0, L"QuitPlugin", 0, 0);
	if (genDll.m_LyrFile.hFile != INVALID_HANDLE_VALUE)
		CloseHandle(genDll.m_LyrFile.hFile);

	genDll.DestroyTrayIcon();

	if (m_MsgHook)
	{
		UnhookWindowsHookEx(m_MsgHook);
		m_MsgHook = 0;
	}

	genDll.FinalUpdateRegistry();

	RegCloseKey(genDll.m_hKey);

	deque<STATUS>::iterator I;
	for (I = genDll.m_StatusMsgs.begin(); I != genDll.m_StatusMsgs.end(); I++)
	{
		I->Statuses.clear();
		deque<SPECIAL>::iterator J;
		for (J = I->Specials.begin(); J != I->Specials.end(); J++)
		{
			if (J->type == SPECIAL::lyrics) break;
			if (J->type == SPECIAL::repet)
			{
				GenListCtrl::REPETE* pRepete = (GenListCtrl::REPETE*)J->pElem;
				pRepete->Verses.clear();
			}
			delete J->pElem;
		}
	}

	genDll.m_StatusMsgs.clear();
	deque<LYRICSINFO>::iterator J;
	for (J = genDll.m_LyricsFiles.begin(); J != genDll.m_LyricsFiles.end(); J++)
	{
		J->sAlbum.clear();
		J->sArtist.clear();
		J->sSong.clear();
		J->wsFileName.clear();
	}

	genDll.m_LyricsFiles.clear();
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
	switch (idEvent)
	{
		//this is also the status change timer
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
							MessageBox(genDll.m_hWinamp, L"Unexpected error", 0, MB_ICONERROR);
							#ifdef _DEBUG
							DebugBreak();
							#endif
							return;
						}

						hWnd = GetDlgItem(hWnd, 0xD3);
						int nLen = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
						nLen++;
						wchar_t* swID = new wchar_t[nLen];
						SendMessage(hWnd, WM_GETTEXT, (WPARAM)nLen, (LPARAM)swID);
						ID id;
						id.hWnd = lastWnd;
						id.swID = swID;
						genDll.m_OnlineIDs.push_back(id);
						delete[] swID;
					}
				}
			}while (lastWnd!=NULL);

			if (genDll.m_bChooseDlgCreated)
				SendMessage(genDll.m_hChooseDlg, WM_UPDATEIDS, 0, 0);
			else
			{
				int size = genDll.m_OnlineIDs.size();
				for (int i = 0; i < size; i++)
					if (wcscmp(genDll.m_OnlineIDs[i].swID.c_str(), genDll.m_SelectedID.swID.c_str()) == 0)
					{
						genDll.m_bSelOnline = true;
						genDll.m_SelectedID.hWnd = genDll.m_OnlineIDs[i].hWnd;
						break;
					}
			}

			//changing the status
			if (genDll.m_bUseTool && genDll.m_bSelOnline)
			{
				if (genDll.m_nTime == -1) genDll.m_nTime = 0;

				wstring status;
				genDll.CompileStatusText(status);
//				if (_wcsicmp(status.c_str(), genDll.m_sDisplayedStatus.c_str()) != 0)
//				{
					genDll.m_sDisplayedStatus = status;
					genDll.DisplayNewStatusText();
//				}

				genDll.m_nTime++;
			}
		}
		break;
	}
}

LRESULT CALLBACK CGenDll::NewParentProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_TRAYMSG:
		switch (lParam)
		{
		case WM_RBUTTONUP:
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
					//do not check anymore, because it is permanently checked.
//					FindYMessengers(yMsgs);

					CChooseDlg c_dlg;
					c_dlg.DoModal(hWnd);
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

	return CallWindowProc(genDll.m_OldParentProc, hWnd, uMsg, wParam, lParam);
}

void CGenDll::DestroyTrayIcon(void)
{
	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = genDll.m_hWinamp;
	nid.uID = SYSTRAY_ICON;
	Shell_NotifyIcon(NIM_DELETE, &nid);
}

void CGenDll::SetupRegistry(void)
{
	//we seek out Winamp key in system registry
	WCHAR strError[200];
	//first, the current user
	int nResult = RegOpenCurrentUser(KEY_WRITE | KEY_READ, &m_hKey);
	if (0 != nResult)
	{
		wsprintf(strError, L"Error 0x%x at initialization 1", nResult);
		MessageBox(genDll.m_hWinamp, strError, L"Error in gen_YM10.dll", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
		return;
	}

	nResult = RegOpenKeyEx(m_hKey, L"Software", 0, KEY_WRITE | KEY_READ, &m_hKey);
	if (0 != nResult)
	{
		wsprintf(strError, L"Error 0x%x at initialization 2", nResult);
		MessageBox(genDll.m_hWinamp, strError, L"Error in gen_YM10.dll", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
		return;
	}

	nResult = RegCreateKeyEx(m_hKey, L"Winamp", 0, NULL,  REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL, &m_hKey, NULL);
	if (0 != nResult)
	{
		wsprintf(strError, L"Error 0x%x at initialization 3", nResult);
		MessageBox(genDll.m_hWinamp, strError, L"Error in gen_YM10.dll", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
		return;
	}

	//VALUES: 1.Use Tool
	BYTE data = 0;
	DWORD dwType, dwSize;
	nResult = RegQueryValueEx(m_hKey, L"gen_YM10 use tool", 0, &dwType, &data, &dwSize);
	if (ERROR_FILE_NOT_FOUND == nResult)
	{
		if (0 != RegSetValueEx(m_hKey, L"gen_YM10 use tool", 0, REG_BINARY, &data, 1))
		{
			wsprintf(strError, L"Error 0x%x at initialization 4", nResult);
			MessageBox(genDll.m_hWinamp, strError, L"Error in gen_YM10.dll", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
			return;
		}
		else m_bUseTool = data;
	}
	else if (nResult == ERROR_MORE_DATA)
	{
		wsprintf(strError, L"ERROR_MORE_DATA at initialization 5, needed %d", &dwSize);
		MessageBox(genDll.m_hWinamp, strError, L"Error in gen_YM10.dll", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
		return;
	}
	else if (nResult == 0) m_bUseTool = data;
	else
	{
		wsprintf(strError, L"Error 0x%x at initialization 5", /*GetLastError()*/nResult);
		MessageBox(genDll.m_hWinamp, strError, L"Error in gen_YM10.dll", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
		return;
	}

	//2. Show Tray Icon
	data = 1;
	nResult = RegQueryValueEx(m_hKey, L"gen_YM10 show tray icon", 0, &dwType, &data, &dwSize);
	if (ERROR_FILE_NOT_FOUND == nResult)
	{
		if (0 != RegSetValueEx(m_hKey, L"gen_YM10 show tray icon", 0, REG_BINARY, &data, 1))
		{
			wsprintf(strError, L"Error 0x%x at initialization 6", nResult);
			MessageBox(genDll.m_hWinamp, strError, L"Error in gen_YM10.dll", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
			return;
		}
		else m_bShowTray = data;
	}
	else if (nResult == 0) m_bShowTray = data;
	else
	{
		wsprintf(strError, L"Error 0x%x at initialization 7", nResult);
		MessageBox(genDll.m_hWinamp, strError, L"Error in gen_YM10.dll", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
		return;
	}

	//3. Selected Status
	data = 0;
	nResult = RegQueryValueEx(m_hKey, L"gen_YM10 selected status", 0, &dwType, &data, &dwSize);
	if (ERROR_FILE_NOT_FOUND == nResult)
	{
		if (0 != RegSetValueEx(m_hKey, L"gen_YM10 selected status", 0, REG_BINARY, &data, 1))
		{
			wsprintf(strError, L"Error 0x%x at initialization 8", nResult);
			MessageBox(genDll.m_hWinamp, strError, L"Error in gen_YM10.dll", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
			return;
		}
		else m_dStatusSelected = data;
	}
	else if (nResult == 0) m_dStatusSelected = data;
	else
	{
		wsprintf(strError, L"Error 0x%x at initialization 9", nResult);
		MessageBox(genDll.m_hWinamp, strError, L"Error in gen_YM10.dll", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
		return;
	}

	//4. ID
	data = 0;
	nResult = RegQueryValueEx(m_hKey, L"gen_YM10 last ID", 0, &dwType, NULL, &dwSize);
	if (ERROR_FILE_NOT_FOUND == nResult)
	{
		if (0 != RegSetValueEx(m_hKey, L"gen_YM10 last ID", 0, REG_SZ, &data, 1))
		{
			wsprintf(strError, L"Error 0x%x at initialization 10", nResult);
			MessageBox(genDll.m_hWinamp, strError, L"Error in gen_YM10.dll", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
			return;
		}
		else m_SelectedID.swID = L"";
	}

	else if (nResult == ERROR_MORE_DATA || dwSize > 0)
	{
		byte* selID = new byte[dwSize];
		nResult = RegQueryValueEx(m_hKey, L"gen_YM10 last ID", 0, &dwType, selID, &dwSize);
		if (nResult != ERROR_SUCCESS) 
		{
			wsprintf(strError, L"Error 0x%x at initialization 11", nResult);
			MessageBox(genDll.m_hWinamp, strError, L"Error in gen_YM10.dll", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
			delete[] selID;
			return;
		}
		m_SelectedID.swID = (wchar_t*)selID;
		delete[] selID;
	}

	else if (nResult == 0)
	{
		m_SelectedID.swID = L"";
	}

	else
	{
		wsprintf(strError, L"Error 0x%x at initialization 12", nResult);
		MessageBox(genDll.m_hWinamp, strError, L"Error in gen_YM10.dll", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
		return;
	}
}

void CGenDll::LoadStatusMessages()
{
	wstring wsPath = genDll.m_swAppData;
	wstring wsPath2 = wsPath;
	wsPath += L"\\*.stt";
	DWORD dwWritten;
	char strError[30];

	WIN32_FIND_DATA f_data;
	BOOL bResult =0;
	memset(&f_data, 0, sizeof(f_data));
	int fileNr = 0;
	HANDLE hSearch = FindFirstFile(wsPath.data(), &f_data);
	while (hSearch != INVALID_HANDLE_VALUE)
	{
		//make sure it is <number>.stt
		wstring wsFileName = f_data.cFileName;
		wsFileName = wsFileName.erase(wsFileName.length() - 4, 4);
		fileNr = _wtoi(wsFileName.c_str());
		if (fileNr == 0) {FindNextFile(hSearch, &f_data); continue;}

		//we open the file:
		wstring sfn = m_swAppData;//wsPath2;
		sfn += '\\';
		sfn += f_data.cFileName;
		HANDLE hFile = CreateFile(sfn.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
		if (hFile == INVALID_HANDLE_VALUE) 
		{
			sprintf_s(strError, 30, "Error 0x%x has occured.", GetLastError());
			MessageBoxA(genDll.m_hWinamp, strError, "Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
			bResult = FindNextFile(hSearch, &f_data);
			if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
			continue;
		}

		//we read the content
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
	char strError[30];
	
	if (0 != RegOpenCurrentUser(KEY_WRITE | KEY_READ, &hKey))
	{
		sprintf_s(strError, 30, "Error 0x%x at initialization lyr1.", GetLastError());
		MessageBoxA(genDll.m_hWinamp, strError, "Fatal Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
		return;
	}

	if (0 != RegOpenKeyEx(hKey, L"Volatile Environment", 0, KEY_READ, &hKey))
	{
		sprintf_s(strError, 30, "Error 0x%x at initialization lyr2.", GetLastError());
		MessageBoxA(genDll.m_hWinamp, strError, "Fatal Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
		return;
	}

	DWORD dwType;
	DWORD dwSize;
	wchar_t str[MAX_PATH];
	if (ERROR_SUCCESS != RegQueryValueEx(hKey, L"APPDATA", 0, &dwType, (LPBYTE)str, &dwSize))
	{
		sprintf_s(strError, 30, "Error 0x%x at initialization lyr 3.", GetLastError());
		MessageBoxA(genDll.m_hWinamp, strError, "Fatal Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
		return;
	}

	RegCloseKey(hKey);

	//create or open the directory:
	wstring wsPath = str;
	wsPath += L"\\Winamp";
	CreateDirectory(wsPath.data(), 0);
	wsPath += L"\\Plugins";
	CreateDirectory(wsPath.data(), 0);
	wsPath += L"\\Samuel App";
	CreateDirectory(wsPath.data(), 0);
	m_swAppData = wsPath;
//	wstring wsPath2 = wsPath;
	wsPath += L"\\*.lyr";

	LYRICSINFO info;
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
		//we open the file:
		wstring sfn = m_swAppData;//wsPath2;
		sfn += '\\';
		sfn += f_data.cFileName;
		HANDLE hFile = CreateFile(sfn.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
		if (hFile == INVALID_HANDLE_VALUE) 
		{
			sprintf_s(strError, 30, "Error 0x%x has occured.", GetLastError());
			MessageBoxA(genDll.m_hWinamp, strError, "Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
			bResult = FindNextFile(hSearch, &f_data);
			if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
			continue;
		}

		//we read the content
		char song[30] = "", artist[30] = "", album[30] = "", sLyr[3] = "";
		DWORD dwRead;
		ReadFile(hFile, sLyr, 3, &dwRead, 0);
		if (sLyr[0] != 'L' && sLyr[1] != 'Y' && sLyr[2] != 'R')
		{
			CloseHandle(hFile);
			bResult = FindNextFile(hSearch, &f_data);
			if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
			continue;
		}

		ReadFile(hFile, artist, 30, &dwRead, 0);
		ReadFile(hFile, album, 30, &dwRead, 0);
		ReadFile(hFile, song, 30, &dwRead, 0);
		info.sAlbum = album;
		info.sArtist = artist;
		info.sSong = song;
		info.wsFileName = f_data.cFileName;
		m_LyricsFiles.push_back(info);

		CloseHandle(hFile);

		bResult = FindNextFile(hSearch, &f_data);
		if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
	}
	FindClose(hSearch);
}

bool CGenDll::ReadStatusFile(HANDLE hFile, STATUS& Status)
{
	//filename: 1.stt, 2.stt, ...

	//file format:
	//HEADER:				'STT'
	//BODY:					byte len
	//						wchar_t str
	//						byte nrStrings
	//						byte lenstr1
	//						char str1
	//						...
	//						byte nrSpecials (must be, even if nrSpecials = 0)
	//						byte pos// the position in string, 0 = before first string.
	//						byte min
	//						byte max
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

	//now we read it!
	DWORD dwRead;
	char stype[3];
	ReadFile(hFile, stype, 3, &dwRead, 0);
	if (stype[0] != 'S' || stype[1] != 'T' || stype[2] != 'T') return false;

	//lenstr (sText):
	byte len;
	ReadFile(hFile, &len, 1, &dwRead, 0);
	wchar_t* swText = new wchar_t[len];
	//sText
	ReadFile(hFile, swText, len * 2, &dwRead, 0);
	Status.swText = swText;
	delete[] swText;

	Status.Specials.clear();
	Status.Statuses.clear();

	byte sz;//Status.Statuses.size();
	ReadFile(hFile, &sz, 1, &dwRead, 0);

	//now we read each string, len and string:
//	byte len;
	for (byte i = 0; i < sz; i++)
	{
		ReadFile(hFile, &len, 1, &dwRead, 0);
		char* sir = new char[len];
		ReadFile(hFile, sir, len, &dwRead, 0);
		Status.Statuses.push_back(sir);
		delete[] sir;
	}

	//now we read the specials. first, their number:
	ReadFile(hFile, &sz, 1, &dwRead, 0);
	byte pos;
	for (byte i = 0; i < sz; i++)
	{
		SPECIAL spec;
		//now we read the position in the string. 0 = before first
		ReadFile(hFile, &pos, 1, &dwRead, 0);
		spec.pos = pos;
		byte min, max;
		ReadFile(hFile, &min, 1, &dwRead, 0);
		ReadFile(hFile, &max, 1, &dwRead, 0);
		spec.min = min;
		spec.max = max;
		//type
		byte type;
		ReadFile(hFile, &type, 1, &dwRead, 0);
		spec.type = (SPECIAL::Type)type;
		//now it depends on the type:
		switch (type)
		{
		case SPECIAL::clock: 
			{
				//read 3 bytes
				CClockDlg::CLOCK* pClock = new CClockDlg::CLOCK;
				byte val;//dHours;
				ReadFile(hFile, &val, 1, &dwRead, 0);
				pClock->dHours = val;

				//dMins;
				ReadFile(hFile, &val, 1, &dwRead, 0);
				pClock->dMins = val;
				
				//len bef
				ReadFile(hFile, &val, 1, &dwRead, 0);
				char* strText = new char[val];
				//str bef
				ReadFile(hFile, strText, val, &dwRead, 0);
				pClock->sTextBefore = strText;
				delete[] strText;

				//len after
				ReadFile(hFile, &val, 1, &dwRead, 0);
				strText = new char[val];
				//str bef
				ReadFile(hFile, strText, val, &dwRead, 0);
				pClock->sTextAfter = strText;
				delete[] strText;

				spec.pElem = pClock;
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
				GenListCtrl::REPETE*	pRepete = new GenListCtrl::REPETE;
				int nRepCnt;
				ReadFile(hFile, &nRepCnt, sizeof(int), &dwRead, 0);
				pRepete->nRepCnt = nRepCnt;
				int nrVerses;
				ReadFile(hFile, &nrVerses, sizeof(int), &dwRead, 0);

				GenListCtrl::VERSE verse;
				for (int i = 0; i < nrVerses; i++)
				{
					ReadFile(hFile, &verse.nrSec, sizeof(WORD), &dwRead, 0);
					byte len;// = (byte)I3->sVerse.length() + 1;
					ReadFile(hFile, &len, 1, &dwRead, 0);
					char* sVerse = new char[len];
					ReadFile(hFile, sVerse, len, &dwRead, 0);
					verse.sVerse = sVerse;
					delete[] sVerse;
					pRepete->Verses.push_back(verse);
				}
				spec.pElem = pRepete;
			}
			break;

		case SPECIAL::songdet: 
			{
				//byte flags
				byte det;// = ((CSongDetDlg::SONGDET*)I2->pElem);
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
	WCHAR strError[200];

	//VALUES: 1.Use Tool
	int nResult = RegSetValueExA(m_hKey, "gen_YM10 use tool", 0, REG_BINARY, &data, 1);
	if (ERROR_SUCCESS != nResult)
	{
		wsprintf(strError, L"Error 0x%x at initialization f1", GetLastError());
		MessageBox(genDll.m_hWinamp, strError, L"Error in gen_YM10.dll", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
		return;
	}

	data = (byte)m_bShowTray;
	//2. Show Tray Icon
	nResult = RegSetValueExA(m_hKey, "gen_YM10 show tray icon", 0, REG_BINARY, &data, 1);
	if (ERROR_SUCCESS != nResult)
	{
		wsprintf(strError, L"Error 0x%x at initialization f2", GetLastError());
		MessageBox(genDll.m_hWinamp, strError, L"Error in gen_YM10.dll", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
		return;
	}

	data = (byte)m_dStatusSelected;
	//3. Selected Status
	nResult = RegSetValueExA(m_hKey, "gen_YM10 selected status", 0, REG_BINARY, &data, 1);
	if (ERROR_SUCCESS != nResult)
	{
		wsprintf(strError, L"Error 0x%x at initialization f3", GetLastError());
		MessageBox(genDll.m_hWinamp, strError, L"Error in gen_YM10.dll", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
		return;
	}
	//4. ID
	int len = genDll.m_SelectedID.swID.length() + 1;
	wchar_t* wstr = new wchar_t[len];
	wcscpy_s(wstr, len, genDll.m_SelectedID.swID.c_str());
	nResult = RegSetValueEx(m_hKey, L"gen_YM10 last ID", 0, REG_SZ, (byte*)wstr, len * 2);
	if (ERROR_SUCCESS != nResult)
	{
		delete[] wstr;
		wsprintf(strError, L"Error 0x%x at initialization f4", GetLastError());
		MessageBox(genDll.m_hWinamp, strError, L"Error in gen_YM10.dll", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
		return;
	}
	delete[] wstr;
}

void CGenDll::CompileStatusText(wstring& status)
{
	//we have m_nTime that specifies the time
	//we have genDll.m_SelectedID.hWnd that specifies the hWnd we will send to - not needed now
	//we have genDll.m_dStatusSelected and
	//genDll.m_StatusMsgs - the status messages.
	status = L"";
	STATUS stt = genDll.m_StatusMsgs[genDll.m_dStatusSelected];
	
	int nrSpecials = stt.Specials.size();
	if (nrSpecials == 0) 
	{
		status += stt.swText;
		return;
	}

	if (stt.Specials[0].pos == 0)
		CompileElement(stt.Specials[0], status);
	
	int pos = 1;
	while (nrSpecials > pos)
	{
		int len = stt.Statuses[pos - 1].length(); 
		for (int i = 0; i < len; i++)
		{
			status.push_back(stt.Statuses[pos - 1][i]); 
		}

//		status += stt.Statuses[pos];
		if (nrSpecials > pos) CompileElement(stt.Specials[pos], status);
		pos++;
	};

	if (stt.Statuses.size() > pos - 1)
	{
		int len = stt.Statuses[pos - 1].length(); 
		for (int i = 0; i < len; i++)
		{
			status.push_back(stt.Statuses[pos - 1][i]); 
		}
	}
}

void CGenDll::CompileElement(SPECIAL& special, wstring& result)
{
	wstring str;
	switch (special.type)
	{
	case SPECIAL::clock:
		{
			CClockDlg::CLOCK* pClock = (CClockDlg::CLOCK*)special.pElem;
			int time = (pClock->dMins + 1) * 60 + pClock->dHours * 3600 - 1;

			time -= genDll.m_nTime;
			if (time/60 <= 0)
			{
				wstring wsAfter;
				for (int i = 0; i < pClock->sTextAfter.length(); i++)
					wsAfter += pClock->sTextAfter[i];

				result += wsAfter;
				return;
			}

			wstring wsBefore;
			for (int i = 0; i < pClock->sTextBefore.length(); i++)
					wsBefore += pClock->sTextBefore[i];
			result += wsBefore;

			wchar_t s[7];
			byte nrHours = time/3600;
			if (nrHours > 0)
			{
				wsprintf(s, L"%dh", nrHours);
//				sprintf_s(s, 3, "%dh", nrHours);
				str = s;
				time -= nrHours * 3600;
			}

			byte nrMins = time/60;
			if (nrMins > 0)
			{
				if (nrHours > 0)
					wsprintf(s, L" %dm", nrMins);
				else wsprintf(s, L"%dm", nrMins);
				str += s;
				time -= nrMins * 60;
			}

/*			byte nrSecs = time;
			if (nrSecs > 0)
			{
				if (nrHours > 0 || nrMins > 0)
					wsprintf(s, L" %ds", nrSecs);
				else wsprintf(s, L"%ds", nrSecs);
				str += s;
			}*/
			
			result += str;
		}
		break;

	case SPECIAL::repet:
		{
			GenListCtrl::REPETE* pRepete = (GenListCtrl::REPETE*)special.pElem;
			int nrRepet;
			if (pRepete->nRepCnt > -1)
			{
				//check to see if we passed that number.
				nrRepet = (genDll.m_nTime - 1) / pRepete->Verses.back().nrSec;
				if (nrRepet > pRepete->nRepCnt) return;
			}
			int nrVerse = (genDll.m_nTime - 1) % pRepete->Verses.back().nrSec;
			//we display the closer one to this time: if we have verses 3 and 5 and our time is 4 we display 3
		
			//if nrRepet == 0 we display only begining with the time of verse 1
			deque<GenListCtrl::VERSE>::iterator I;
			for (I = pRepete->Verses.begin(); I != pRepete->Verses.end(); I++)
			{
				if (nrRepet > 0 && nrVerse < pRepete->Verses.front().nrSec)
				{
					int len = pRepete->Verses.back().sVerse.length();
					for (int i = 0; i < len; i++)
					{
						str.push_back(pRepete->Verses.back().sVerse[i]);
					}
					break;
				}

				else if (nrVerse <= I->nrSec)
				{
					int len = I->sVerse.length();
					for (int i = 0; i < len; i++)
					{
						str.push_back(I->sVerse[i]);
					}
					break;
				}
			}
			result += str;
		}
		break;

	case SPECIAL::songdet:
		{
			//is song stopped?
			int play_state = SendMessage(genDll.m_hWinamp, WM_USER, 0, 104);
			if (play_state != 1 && play_state != 3) return;

			CSongDetDlg::SONGDET* pSongDet = (CSongDetDlg::SONGDET*)special.pElem;
			wstring swArtist, swSong;//, swAlbum;
			if (*pSongDet & CSongDetDlg::info::artist ||
				*pSongDet & CSongDetDlg::info::song)
				FindSongInfo(swArtist, swSong);

			if (*pSongDet & CSongDetDlg::info::artist &&
				*pSongDet & CSongDetDlg::info::song)
			{
				str = swArtist;
				str += L" - ";
				str += swSong;
			}
			else if (*pSongDet & CSongDetDlg::info::artist)
				str = swArtist;
			else if (*pSongDet & CSongDetDlg::info::song)
				str = swSong;

			if (*pSongDet & CSongDetDlg::info::length)
			{
				wstring swLen;
				GetCurrentSongLength(swLen);
				str += swLen;
			}

			if (*pSongDet & CSongDetDlg::info::progress)
			{
				if (str.length()) str += L" ";
				wstring swProgress;
				GetCurrentSongProgress(swProgress, *pSongDet & CSongDetDlg::info::percent);
				str += swProgress;
			}
			if (play_state == 3)
				str += L"(Pauză)";

			result += str;
		}
		break;

	case SPECIAL::lyrics:
		{
			wstring swArtist, swSong;//, swAlbum;
			//we find them from winamp
			FindSongInfo(swArtist, swSong/*, swAlbum*/);
			//now we search in our list to see if we have it:
			deque<LYRICSINFO>::iterator I;
			bool bOK = false;
			for (I = genDll.m_LyricsFiles.begin(); I != genDll.m_LyricsFiles.end(); I++)
			{
				wstring waux;
				StringAtoW(I->sSong.c_str(), waux);
				if (swSong == waux)
				{
					bOK = true;
					if (swArtist.length())
					{
						StringAtoW(I->sArtist.c_str(), waux);
						if (swArtist == waux)
							bOK = true;
						else bOK = false;
					}

					if (!bOK) continue;

//					if (sAlbum.length())
//					{
//						if (sAlbum == I->sAlbum)
//							bOK = true;
//						else bOK = false;
//					}
				}
				if (bOK == true) break;
			}
			
			if (bOK == false) return;

			//we have to get the second of the verse...
			//but first, we have to load the lyrics file, if we haven't already
			wstring swFileName = genDll.m_swAppData;
			swFileName += L"\\";
			swFileName += I->wsFileName;
			//if we have a file, and the filenames differ
			if (swFileName != genDll.m_LyrFile.swFileName)
			{
				if (genDll.m_LyrFile.hFile != INVALID_HANDLE_VALUE)
					CloseHandle(genDll.m_LyrFile.hFile);
				//creating the file
				genDll.m_LyrFile.swFileName = swFileName;
				genDll.m_LyrFile.hFile = CreateFile(swFileName.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

				//now we read the file.
				//file format:
				//HEADER:				'LYR'
				//						artist[30]
				//						album[30]
				//						song[30]
				//						.............
				//BODY:					sec (word)
				//						text len(byte)
				//						text (char*)
				//just seek the first second:
				SetFilePointer(genDll.m_LyrFile.hFile, 93, 0, FILE_BEGIN);
				WORD sec;

				byte len;
				GenListCtrl::VERSE verse;
				DWORD dwRead;
				do
				{
					if (ReadFile(genDll.m_LyrFile.hFile, &sec, 2, &dwRead, 0) != 0 &&
						dwRead == 0)
						goto the_end;
					verse.nrSec = sec;
					ReadFile(genDll.m_LyrFile.hFile, &len, 1, &dwRead, 0);
					char* the_verse = new char[len];
					ReadFile(genDll.m_LyrFile.hFile, the_verse, len, &dwRead, 0);
					verse.sVerse = the_verse;
					delete[] the_verse;

					genDll.m_LyrFile.Verses.push_back(verse);
				}while (dwRead != 0);
			}
			//now we surely have the file loaded, the lyrics, etc.
			//getting the second... but first, we must retrieve the second of the melody.
the_end:
			//second position
			int nrSec = SendMessage(genDll.m_hWinamp, WM_USER, 0, 105);
			nrSec /= 1000;
			deque<GenListCtrl::VERSE>::iterator J;
			for (J = genDll.m_LyrFile.Verses.begin(); J != genDll.m_LyrFile.Verses.end(); J++)
			{
				if (nrSec < J->nrSec && nrSec < genDll.m_LyrFile.Verses[0].nrSec)
				{
//					StringAtoW(J->sVerse.c_str(), str);
					str = L"";
					break;
				}
				else if (nrSec < J->nrSec)
				{
					StringAtoW((J-1)->sVerse.c_str(), str);
					break;
				}

				else if (nrSec == J->nrSec)
				{
					StringAtoW(J->sVerse.c_str(), str);
					break;
				}
			}

			result += str;
		}
		break;
	}
}

void CGenDll::DisplayNewStatusText()
{
	HWND hWnd = genDll.m_SelectedID.hWnd;
	if (hWnd == 0) return;
	hWnd = FindWindowEx(hWnd, 0, L"YTopWindow", 0);
	if (hWnd == 0) return;
	hWnd = FindWindowEx(hWnd, 0, L"CMerlinWndBase", 0);
	HWND hMerlin = hWnd;
	if (hWnd == 0) return;

	//we have to find the window that represents the status text
	//class YUI_Win32Edit
	//other than window name starting with "http://"
	hWnd = FindWindowEx(hMerlin, 0, L"YUI_Win32Edit", 0);
	wchar_t sir[10];
	SendMessage(hWnd, WM_GETTEXT, 8, (LPARAM)sir);
	if (wcscmp(sir, L"http://") == 0)
		hWnd = FindWindowEx(hMerlin, hWnd, L"YUI_Win32Edit", 0);

//	wsprintf(sir, L"nr: %d", genDll.m_nTime / 4);
	SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)genDll.m_sDisplayedStatus.c_str());
	SendMessage(hWnd, WM_KILLFOCUS, 0, 0);
}

void CGenDll::GetCurrentSongProgress(wstring& swProgress, bool bUsePercent)
{
	//in miliseconds
	int pos = SendMessage(genDll.m_hWinamp, WM_USER, 0, 105);
	pos /= 1000;//now it is in seconds
	int length = SendMessage(genDll.m_hWinamp, WM_USER, 1, 105);//length in seconds
	int percent = pos * 100 / length;//xx %
	//we make it into tens of percent: 10%, 20%, 30%, ...
	percent /= 10;

	if (bUsePercent)
	{
		percent *= 10;
		wchar_t str[10];
		wsprintf(str, L"(%d%%)", percent);
		swProgress = str;
	}
	else
	{
		wchar_t str[] = L"[----------]";
#ifdef _DEBUG
		if (wcslen(str) - 2 <= percent) DebugBreak();
#endif
		str[percent + 1] = '|';
		swProgress = str;
	}
}

void CGenDll::GetCurrentSongLength(wstring& swLen)
{
	int length = SendMessage(genDll.m_hWinamp, WM_USER, 1, 105);//length in seconds
	wchar_t str[15];

	int nMins = length / 60;
	int nSecs = length % 60;

	if (nSecs > 9)
		wsprintf(str, L"(%d:%d)", nMins, nSecs);
	else
		wsprintf(str, L"(%d:0%d)", nMins, nSecs);
	swLen = str;
}

void CGenDll::FindSongInfo(wstring& swArtist, wstring& swSong)
{
	swArtist = swSong = L"";

//	wcscpy_s(wstrlast, MAX_PATH, genDll.m_sCurrentSong);
	CLyricsDlg::WINAMP_TAGV1 tagInfo;
	//load open the file for reading:
	HANDLE hFile = CreateFile(m_sCurrentSong, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED
, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		char errs[20];
		DWORD dwError = GetLastError();
		sprintf_s(errs, 20, "Error 0x%x", dwError);
//		MessageBoxA(genDll.m_hWinamp, errs, "Error!", MB_ICONERROR);
#ifdef _DEBUG
//		DebugBreak();
#endif
			return;
	}

		//so now, we have the file opened... we must load the last part of it:
		LARGE_INTEGER liSize;
		GetFileSizeEx(hFile, &liSize);
		DWORD dwRead;
		OVERLAPPED overl;
		memset(&overl, 0, sizeof(overl));
		overl.Offset = (DWORD)liSize.LowPart - 128;
		overl.hEvent = 0;
		ReadFile(hFile, &tagInfo, 128, &dwRead, &overl);
		GetOverlappedResult(hFile, &overl, &dwRead, true);

		if (tagInfo.cT == 'T' && tagInfo.cA == 'A' && tagInfo.cG == 'G')
		{
			if (dwRead != 128)
			{
				WCHAR wstrlast[15];
				DWORD dwError = GetLastError();
				wsprintf(wstrlast, L"Error 0x%x", dwError);
				MessageBox(genDll.m_hWinamp, wstrlast, L"Error!", MB_ICONERROR);
#ifdef _DEBUG
				DebugBreak();
#endif
				return;
			}
			tagInfo.sAlbumName[29] = tagInfo.sArtistName[29] = tagInfo.sSongName[29] = 0;
			CLyricsDlg::TrimString(tagInfo.sAlbumName);
			CLyricsDlg::TrimString(tagInfo.sArtistName);
			CLyricsDlg::TrimString(tagInfo.sSongName);

			StringAtoW(tagInfo.sArtistName, swArtist);
//			StringAtoW(tagInfo.sAlbumName, swAlbum);
			StringAtoW(tagInfo.sSongName, swSong);

//			if (strlen(tagInfo.sArtistName) > 0) bArtist = true;
//			if (strlen(tagInfo.sAlbumName) > 0) bAlbum = true;
//			if (strlen(tagInfo.sSongName) > 0) bSong = true;
		}

		
		if (!swArtist.length() || !swSong.length())
		FindByTagIDV2(hFile, swArtist, swSong);

	CloseHandle(hFile);
}

void CGenDll::FindByTagIDV2(HANDLE hFile, wstring& swArtist, wstring& swSong)
{
	CLyricsDlg::WINAMP_TAGV2 tagInfo;
	OVERLAPPED overl;
	DWORD dwRead;
	memset(&overl, 0, sizeof(overl));
	overl.Offset = 0;
	overl.hEvent = 0;
	ReadFile(hFile, &tagInfo, 10, &dwRead, &overl);
	GetOverlappedResult(hFile, &overl, &dwRead, true);

	if (dwRead < 10)
	{
		DWORD dwError = GetLastError();
		char str[10];
		sprintf_s(str, 10, "Error 0x%x", dwError);
		MessageBoxA(genDll.m_hWinamp, str, "Error!", MB_ICONERROR);
#ifdef _DEBUG
		DebugBreak();
#endif
		return;
	}

	if (tagInfo.cI == 'I' && tagInfo.cD == 'D' && tagInfo.c3 == '3')
	{
		DWORD dwSize = (tagInfo.nSize[0] << 7) + (tagInfo.nSize[1] << 7) + (tagInfo.nSize[2] << 7) + tagInfo.nSize[3];
		byte* buffer = new byte[dwSize];
		overl.Offset = 10;
		ReadFile(hFile, buffer, dwSize, &dwRead, &overl);
		GetOverlappedResult(hFile, &overl, &dwRead, true);
		if (dwRead < dwSize)
		{
			DWORD dwError = GetLastError();
			char str[10];
			sprintf_s(str, 10, "Error 0x%x", dwError);
			MessageBoxA(genDll.m_hWinamp, str, "Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
			return;
		}

		if (tagInfo.nVer1 == 2)
		{
			CLyricsDlg::FRAME20 frame;
			int care = 0;
			BYTE finished = 0;
			for (int i = 0; i < dwSize - 3; i++)
			{
				care = 0;
//				if (buffer[i] == 'T' && buffer[i+1] == 'A' && buffer[i+2] == 'L' && swAlbum.length())
//					care = 1;
				if (buffer[i] == 'T' && buffer[i+1] == 'T' && buffer[i+2] == '2' && swSong.length())
					care = 2;
				else if (buffer[i] == 'T' && buffer[i+1] == 'P' && buffer[i+2] == '1' && swArtist.length())
					care = 3;
				else continue;

				frame.cName[0] = buffer[i];
				frame.cName[1] = buffer[i+1];
				frame.cName[2] = buffer[i+2];

				frame.size[0] = buffer[i+3]; frame.size[1] = buffer[i+4]; frame.size[2] = buffer[i+5];
				DWORD dwSize = (buffer[i+3] << 7) + (buffer[i+4] << 7) + buffer[i+5];

				frame.encoding = buffer[i+6];
				if (frame.encoding == 0)
				{
					frame.str = new char[dwSize];
					strcpy_s(frame.str, dwSize, (char*)(i+7+buffer));
					frame.str[dwSize-1] = 0;
				}
				else//unicode:
				{
					frame.wstr = new wchar_t[dwSize/2];
					frame.str = new char[dwSize/2];
					memcpy_s(frame.wstr, dwSize-1, (wchar_t*)(i+9+buffer), dwSize-1);
					for (int j = 0; j < dwSize/2; j++) {frame.str[j] = frame.wstr[j];}
					frame.str[dwSize/2-1] = 0;
				}

//				if (care == 1)
//				{
//					CLyricsDlg::TrimString(frame.str);
//					StringAtoW(frame.str, swAlbum);
//					finished |= 1;
//				}
				if (care == 2)
				{
					CLyricsDlg::TrimString(frame.str);
					StringAtoW(frame.str, swSong);
					finished |= 2;
				}
				else if (care == 3)
				{
					CLyricsDlg::TrimString(frame.str);
					StringAtoW(frame.str, swArtist);
					finished |= 4;
				}

				if (frame.str) delete[] frame.str;

				if (finished == 7) break;
			}
		}
		else if (tagInfo.nVer1 == 3 || tagInfo.nVer1 == 4)
		{
			CLyricsDlg::FRAME34 frame;
			int care = 0;
			BYTE finished = 0;
			for (int i = 0; i < dwSize - 3; i++)
			{
				care = 0;
//				if (buffer[i] == 'T' && buffer[i+1] == 'A' && buffer[i+2] == 'L' && buffer[i+3] == 'B' && swAlbum.length())
//					care = 1;
				if (buffer[i] == 'T' && buffer[i+1] == 'I' && buffer[i+2] == 'T' && buffer[i+3] == '2' && swSong.length())
					care = 2;
				else if (buffer[i] == 'T' && buffer[i+1] == 'P' && buffer[i+2] == 'E' && buffer[i+3] == '1' && swArtist.length())
					care = 3;
				else continue;
				
				frame.cName[0] = buffer[i];
				frame.cName[1] = buffer[i+1];
				frame.cName[2] = buffer[i+2];
				frame.cName[3] = buffer[i+3];

				frame.size[0] = buffer[i+4]; frame.size[1] = buffer[i+5]; frame.size[2] = buffer[i+6]; frame.size[3] = buffer[i+7];
				DWORD dwSize = (buffer[i+4] << 7) + (buffer[i+5] << 7) + (buffer[i+6] << 7) + buffer[i+7];
				
				frame.flags = MAKEWORD(buffer[i+8], buffer[i+9]);
				frame.encoding = buffer[i+10];
				if (frame.encoding == 0 || frame.encoding == 3)
				{
					frame.str = new char[dwSize];
					frame.str[dwSize-1] = 0;

					memcpy_s(frame.str, dwSize-1, (char*)(i+11+buffer), dwSize-1);
					for (int k = 0; k < dwSize - 1; k++)
						if (isascii(frame.str[k]) == false)
						{
							delete[] frame.str;
							frame.str = 0;
							break;
						}
//					strcpy_s(frame.str, dwSize + 1, (char*)(i+11+buffer));
				}
				else
				{
					frame.wstr = new wchar_t[dwSize/2];
					frame.wstr[dwSize/2-1] = 0;
					frame.str = new char[dwSize/2];
					memcpy_s(frame.wstr, dwSize-1, (wchar_t*)(i+13+buffer), dwSize-1);
					for (int j = 0; j < dwSize/2; j++) {frame.str[j] = frame.wstr[j];}
					frame.str[dwSize/2-1] = 0;
				}

//				if (care == 1)
//				{
//					CLyricsDlg::TrimString(frame.str);
//					StringAtoW(frame.str, swAlbum);
//					finished |= 1;
//				}
				if (care == 2)
				{
					CLyricsDlg::TrimString(frame.str);
					StringAtoW(frame.str, swSong);
					finished |= 2;
				}
				else if (care == 3)
				{
					CLyricsDlg::TrimString(frame.str);
					StringAtoW(frame.str, swArtist);
					finished |= 4;
				}

				if (frame.str) {delete[] frame.str; frame.str = 0;}
				if (frame.wstr) {delete[] frame.wstr; frame.str = 0;}

				if (finished == 7) break;
			}
		}

		delete[] buffer;
	}

	if (!swArtist.length() || !swSong.length())
		FindByFileName(swArtist, swSong);
}

void CGenDll::FindByFileName(wstring& swArtist, wstring& swSong)
{
	//we take the info from winamp
	//i.e.
	//736. Kataklysm - Taking the World By Storm - Winamp
	//until first dot and a ' ' is irrelevant
	//the last ' - Winamp' is irrelevant
	//then it follows: Artist - Song / Artist - Album - Song. we count how many '-' there are.
	wstring swText;
	int len = GetWindowTextLength(genDll.m_hWinamp);
	wchar_t* text = new wchar_t[len+1];
	GetWindowText(genDll.m_hWinamp, text, len+1);
	swText = text;
	delete[] text;
	
	//removing irrelevant text
	int x = swText.find('.', 0);
	swText.erase(0, x + 2);
	x = swText.rfind('-');
	swText.erase(x-1, swText.length()-1);

	//now result is in the format "xxxx - xxxx - xxx" or in the format "xxxx - xxxx"
	//we count the '-'s:
	
	int nCount = 0;
	int xs[3]; int i = 0;
	for (int i = 0; i < swText.length(); i++)
	{
		if (swText[i] == '-')
		{
			if (nCount > 2) break;
			xs[nCount] = i;
			nCount++;
		}
	}
	xs[nCount] = swText.length();
	//we have i '-'s. now we get each string and tream them.

	wstring wstrs[3];int j = 0;
	for (int i = 0; i <= nCount; i++)
	{
		if (nCount == 0) break;
		wstrs[i] = swText.substr(j, xs[i] - j);
		j = xs[i]+1;
	}
	
	for (j = 0; j <= nCount; j++)
	{
		//ltrim string i
		for (i = 0; i < wstrs[j].length(); i++)
		{
			if (wstrs[j][i] ==' ') wstrs[j].erase(i, 1);
			else break;
		}

		//rtrim
		for (i = wstrs[j].length() - 1; i >=0; i--)
		{
			if (wstrs[j][i] ==' ') wstrs[j].erase(i, 1);
			else break;
		}
	}

	switch (nCount)
	{
	case 0:
		//no '-' found => all sText is SongName
		swSong = swText;
		break;
	case 1:
		//one '-' found => two substrings, 1 and 2, Artist and Song
		swArtist = wstrs[0];
		swSong = wstrs[1];
		break;
	case 2:
		//three '-' found => three substrings, 1 and 2 and 3, artist and album and song
		swArtist = wstrs[0];
//		swAlbum = wstrs[1];
		swSong = wstrs[2];
		break;
	}
}

void CGenDll::StringAtoW(const char* Astr, wstring& Wstr)
{
	Wstr = L"";

	int len = strlen(Astr);
	for (int i = 0; i < len; i++)
	{
		Wstr.push_back(Astr[i]);
	}
}