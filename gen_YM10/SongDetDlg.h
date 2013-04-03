#pragma once

#ifndef SONGDETDLG_H
#define SONGDETDLG_H

#include <windows.h>

#include "GenDll.h"

//clasa care se ocupa cu caseta de dialog pentru adaugarea de informatii despre melodie in status.
class CSongDetDlg
{
public:
	typedef BYTE	SONGDET;
	enum info:BYTE {artist = 1, album = 2, song = 4, progress = 8, length = 16, lyrics = 32, percent = 64};
	SONGDET			m_SongDet;
	//handle la fereastra casetei de dialog
	HWND			m_hDlg;

public:
	CSongDetDlg(SONGDET songdet);
	~CSongDetDlg(void);

	//functia pentru crearea casetei de dialog, ca modala
	INT_PTR DoModal(HWND hParent);

private:
	//functie pentru prelucrarea mesajelor casetei de dialog
	static INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//functie apelata cand se apasa butonul Aplica
	void OnOk();
	//functie apelata la trimiterea mesajului WM_INITDIALOG.
	void OnInitDialog();
};

#endif//SONGDETDLG_H