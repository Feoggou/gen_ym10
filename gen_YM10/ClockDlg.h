#pragma once

#ifndef CLOCKDLG_H
#define CLOCKDLG_H

#include <windows.h>
#include "GenDll.h"

//clasa se ocupa cu caseta de dialog Ceas si cu obiectul (SPECIAL), Ceas
class CClockDlg
{
public:
	struct CLOCK
	{
		BYTE dHours;
		BYTE dMins;
		wstring wsTextBefore;
		wstring wsTextAfter;
	};
	//SPECIAL-ul, m_Clock (Ceas)
	CLOCK m_Clock;

private:
	//handle la fereastra
	HWND			m_hDlg;
	//pointer la un obiect de tipul CLOCK
	CLOCK*			m_pSpec;
public:
	//constructorul
	CClockDlg(CLOCK* pSpec);
	//destructorul
	~CClockDlg(void);
	//se creeaza o caseta de dialog modala.
	INT_PTR DoModal(HWND hParent);

private:
	//functia de prelucrare a mesajelor
	static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//functia apelata la trimiterea mesajului WM_INITDIALOG
	int OnInitDialog();
	//functie apelata cand se modifica continutul casetei de dialog a nr-lui minutelor sau secundelor
	void OnEditUpdate(WORD wID, HWND hEdit);
	//functie apelata cand caseta de editare a nr orelor sau a minutelor pierde focus-ul
	void OnEditKillFocus(WORD wID);
	//functie apelata cand caseta de editare a nr orelor sau a minutelor primeste focus-ul
	void OnEditSetFocus(WORD wID, HWND hEdit);
	//functia apelata cand se apasa pe butonul Aplica
	void OnOk();
};

#endif//CLOCKDLG_H