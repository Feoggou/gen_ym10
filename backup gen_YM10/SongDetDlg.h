#pragma once

#ifndef SONGDETDLG_H
#define SONGDETDLG_H

#include <windows.h>

#include "GenDll.h"

class CSongDetDlg
{
public:
	typedef BYTE	SONGDET;
	enum info:BYTE {artist = 1, song = 2, progress = 4, length = 8, percent = 16};
	SONGDET			m_SongDet;
private:
	HWND			m_hWnd;
	static WNDPROC	OldWndProc;
	SPECIAL*		m_pSpec;
public:
	CSongDetDlg(SPECIAL* pSpec);
	~CSongDetDlg(void);

	int DoModal(HWND hParent);
private:
	static INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void OnOk(HWND hDlg);
	static void OnInitDialog(HWND hDlg, LPARAM lParam);
};

#endif//SONGDETDLG_H