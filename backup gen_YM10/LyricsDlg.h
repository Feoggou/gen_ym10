#pragma once

#ifndef LYRICSDLG_H
#define LYRICSDLG_H

#include <windows.h>

#include "GenDll.h"
#include "GenListCtrl.h"
#include "ButtonBar.h"
#include "HyperLink.h"

class CLyricsDlg
{
public:
	struct WINAMP_TAGV1
	{
		char cT, cA, cG;
		char sSongName[30];
		char sArtistName[30];
		char sAlbumName[30];
		BYTE dY, dE, dA, dR;
		char sComment[30];
		BYTE dGenre;

		WINAMP_TAGV1()
		{
			sSongName[0] = 0; sAlbumName[0] = 0; sArtistName[0] = 0;
		}
	};

	struct WINAMP_TAGV2
	{
		char cI, cD, c3;
		BYTE nVer1, nVer2;
		BYTE nFlags;
		BYTE nSize[4];
	};

	struct FRAME20
	{
		char cName[3];
		BYTE size[3];
		BYTE encoding;
		char* str;
		wchar_t* wstr;
		FRAME20()
		{
			str = 0;
			wstr = 0;
		}
	};

	struct FRAME34
	{
		char cName[4];
		BYTE size[4];
		WORD flags;
		BYTE encoding;
		char* str;
		wchar_t* wstr;
		FRAME34()
		{
			str = 0;
			wstr = 0;
		}
	};

private:
	HWND					m_hWnd;
	GenListCtrl				m_List;
	static HBITMAP			m_hbSet, m_hbSet2;
	CHyperLink				m_LoadFromFile, m_LoadFromWeb;
	HCURSOR					m_hArrowCursor, m_hHandCursor;
	BOOL					m_bClicked;
	BOOL					m_bMoved;
	static WINAMP_TAGV1		m_tagInfo;
	static bool				m_bIsSet;

	CButtonBar		m_ButBar;
public:
	CLyricsDlg(void);
	~CLyricsDlg(void);
	int DoModal(HWND hParent);
	static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void OnOk(HWND hDlg);
	static void OnInitDialog(HWND hDlg, LPARAM lParam);
	static void CALLBACK OnTimer(HWND hDlg, UINT uMsg, UINT_PTR nIDEvent, DWORD dwTime);
	static void OnLButtonUp(HWND hDlg, WPARAM wParam, LPARAM lParam);
	static void UpdateByTagIDV1(HWND hDlg);
	static void UpdateByTagIDV2(HWND hDlg, HANDLE hFile, bool bArtist, bool bAlbum, bool bSong);
	static void UpdateByFileName(HWND hDlg, bool bArtist, bool bAlbum, bool bSong);
	static void OnMouseMove(HWND hDlg, WPARAM wParam, LPARAM lParam);
	static void LoadFromFile(HWND hDlg);
	static void LoadFromWeb(HWND hDlg);
	static void SetList(HWND hDlg, string& sText);
	static void OnSet(HWND hDlg);
	static HANDLE OpenLyricsFile(HWND hDlg);
	static void TrimString(char* sir);
};

#endif//LYRICSDLG_H