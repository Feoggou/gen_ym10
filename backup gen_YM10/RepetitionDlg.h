#pragma once

#ifndef REPETITIONDLG_H
#define REPETITIONDLG_H

#include <windows.h>

#include "GenListCtrl.h"

class CRepetitionDlg
{
private:
	GenListCtrl					m_list;
	GenListCtrl::REPETE*		m_pSpec;
public:
	CRepetitionDlg(GenListCtrl::REPETE* pSpec);
	~CRepetitionDlg(void);
	int DoModal(HWND hParent);
	static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void OnInitDialog(HWND hDlg, LPARAM lParam);
	static void OnOk(HWND hDlg);
	friend class CStatusEditorDlg;
};

#endif