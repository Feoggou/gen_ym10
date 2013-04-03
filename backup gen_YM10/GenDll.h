#pragma once

#ifndef GENDLL_H
#define GENDLL_H

/*#ifndef _DEBUG
#define _DEBUG
#endif//_DEBUG*/

//#ifndef WINVER
#define WIN32_LEAN_AND_MEAN
#define WINVER			0x0501
#define _WIN32_WINNT	0x0501
//#define _WIN32_WINDOWS  0x0410
//#endif

typedef unsigned char byte;

#define NTDDI_VERSION		NTDDI_WINXP

#define WM_TRAYMSG				WM_USER + 9991

#define GPPHDR_VER				0x10
#define SYSTRAY_ICON			1000
#define IDT_SEEKYMESS			2000

#define	WM_UPDATELIST			WM_USER + 9990
#define WM_UPDATEIDS			WM_USER + 9989

#include <windows.h>
#include <deque>

#include "resource.h"
#include "GenListCtrl.h"

using namespace std;

typedef struct {
	int version;
	char *description;
	int (*init)();
	void (*config)();
	void (*quit)();
	HWND hwndParent;
	HINSTANCE hDllInstance;
} winampGeneralPurposePlugin;

extern winampGeneralPurposePlugin *gen_plugins[256];
typedef winampGeneralPurposePlugin * (*winampGeneralPurposePluginGetter)();

struct SPECIAL
{
	enum Type:BYTE {clock, repet, songdet, lyrics};
	int min, max;
	BYTE pos;
	Type type;
	void* pElem;
};

struct STATUS
{
	deque<string> Statuses;
	deque<SPECIAL> Specials;
	wstring swText;
	int dFileNr;
};

struct LYRICSINFO
{
	wstring wsFileName;
	string sArtist;
	string sAlbum;
	string sSong;
};

struct ID
{
	wstring		swID;
	HWND		hWnd;
};

struct LYRFILE
{
	HANDLE						hFile;
	wstring						swFileName;
	deque<GenListCtrl::VERSE>	Verses;
};

//the class
class CGenDll
{
public:
	HWND						m_hWinamp;
	HWND						m_hChooseDlg;
	HICON						m_hIcon;
	HINSTANCE					m_hInstance;
	HKEY						m_hKey;
	deque<STATUS>				m_StatusMsgs;
	deque<LYRICSINFO>			m_LyricsFiles;
	WCHAR						m_sCurrentSong[MAX_PATH];
	wstring						m_sSavingSong;
	wstring						m_swAppData;
	bool						m_bChooseDlgCreated;
	bool						m_bStatusEdtCreated;
	char						m_dStatusSelected;//signed
	bool						m_bUseTool;
	bool						m_bShowTray;
	deque<ID>					m_OnlineIDs;
	ID							m_SelectedID;
	bool						m_bSelOnline;
	int							m_nTime;
	wstring						m_sDisplayedStatus;
	LYRFILE						m_LyrFile;

private:
	WNDPROC			m_OldParentProc;
	static WNDPROC	OldDlgProc;
	static HHOOK	m_MsgHook;

	BYTE			m_dNrStatusMsgs;
public:
//	winampGeneralPurposePlugin plugin;
	static int InitPlugin(void);
	static void ConfigPlugin(void);
	static void QuitPlugin(void);
	static LRESULT CALLBACK MessageProc(int nCode, WPARAM wParam, LPARAM lParam);

	winampGeneralPurposePlugin m_Plugin;

public:
	CGenDll(void);
	~CGenDll(void);
	void CreateTrayIcon(void);
	static void CALLBACK OnTimer(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	static LRESULT CALLBACK NewParentProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK NewDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void DestroyTrayIcon(void);
	BYTE GetValueUseTool()
	{
		return m_bUseTool;
	}
	BYTE GetValueShowTray()
	{
		return m_bShowTray;
	}
	char GetValueStatusSelected()
	{
		return m_dStatusSelected;
	}
	BYTE GetValueNrStatusMsgs()
	{
		return m_dNrStatusMsgs;
	}
	void GetStatusMsgs(deque<string>& strings)
	{
//		strings = m_StatusMsgs;
		DebugBreak();
	}

private:
	void SetupRegistry(void);
	void LoadStatusMessages();
	void LoadLyricsInfo();
	bool ReadStatusFile(HANDLE hFile, STATUS& Status);
	void FinalUpdateRegistry();
	void CompileStatusText(wstring& status);
	void DisplayNewStatusText();
	void CompileElement(SPECIAL& special, wstring& result);
	void GetCurrentSongProgress(wstring& swProgress, bool bUsePercent);
	void GetCurrentSongLength(wstring& swLen);
	void FindSongInfo(wstring& swArtist, wstring& swSong);
	void FindByTagIDV2(HANDLE hFile, wstring& swArtist, wstring& swSong);
	void StringAtoW(const char* Astr, wstring& Wstr);
	void FindByFileName(wstring& swArtist, wstring& swSong);
};

#endif//GENDLL_H