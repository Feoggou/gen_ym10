#pragma once

#ifndef LYRICSEDITORDLG_H
#define LYRICSEDITORDLG_H

#include <windows.h>

//clasa CLyricsEditor este clasa care se ocupa de caseta de dialog pentru editarea lyrics-urilor.
class CLyricsEditorDlg
{
private:
	//Handle la fereastra (caseta de dialog)
	HWND			m_hDlg;
public:
	//constructorul
	CLyricsEditorDlg(void);
	//destructorul
	~CLyricsEditorDlg(void);

	//functie care creeaza caseta de dialog, de tip modal.
	void DoModal(HWND hParent);

private:
	//functia ce prelucreaza mesajele
	static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//functia care este apelata cand este trimis mesajul WM_INITDIALOG.
	int OnInitDialog();
};

#endif//LYRICSEDITORDLG_H