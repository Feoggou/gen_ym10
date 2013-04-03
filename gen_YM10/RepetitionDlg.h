#pragma once

#ifndef REPETITIONDLG_H
#define REPETITIONDLG_H

#include <windows.h>

#include "GenListCtrl.h"

//clasa se ocupa de caseta de dialog pentru adaugarea unei repetitii
class CRepetitionDlg
{
public:
	//handle la fereastra casetei de dialog
	HWND						m_hDlg;

private:
	//controlul lista/tabel
	GenListCtrl					m_list;
	//SPECIAL-ul
	GenListCtrl::REPETE*		m_pSpec;

public:
	//constructorul
	CRepetitionDlg(GenListCtrl::REPETE* pSpec);
	//destructorul
	~CRepetitionDlg(void);

	//functia pentru crearea casetei de dialog - este creata modala.
	INT_PTR DoModal(HWND hParent);

private:
	//functia de prelucrare a mesajelor
	static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//noua functie pentru prelucrarea mesajelor casetei de bifare "foloseste minute"
	static LRESULT CALLBACK NewUseMinutesProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//functia apelata la trimiterea mesajului WM_INITDIALOG
	void OnInitDialog();
	//functia apelata cand se apasa butonul Aplica
	void OnOk();

	//urmatoarele clase pot folosi membrii privati ai CRepetitionDlg
	friend class CStatusEditorDlg;
	friend class GenListCtrl;
};

#endif