#pragma once

#ifndef CLOCKDLG_H
#define CLOCKDLG_H

#include <windows.h>
#include "GenEditBox.h"
#include "GenDll.h"


class CClockDlg
{
public:
	struct CLOCK//: public SPECIAL
	{
		BYTE dHours;
		BYTE dMins;
		string sTextBefore;
		string sTextAfter;
	};
	CLOCK m_Clock;
private:
	GenEditBox		m_Hours, m_Mins;//, m_Secs;
	CLOCK*			m_pSpec;
public:
	CClockDlg(CLOCK* pSpec);
	~CClockDlg(void);
	int DoModal(HWND hParent);
	static void OnInitDialog(HWND hDlg, LPARAM lParam);
	static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void OnEditUpdate(HWND hDlg, WPARAM wParam, LPARAM lParam);
	static void OnEditKillFocus(HWND hDlg, WPARAM wParam, LPARAM lParam);
	static void OnEditSetFocus(HWND hDlg, WPARAM wParam, LPARAM lParam);
	static void OnOk(HWND hDlg);
};

#endif//CLOCKDLG_H