#pragma once

#ifndef STATUSEDITORDLG_H
#define STATUSEDITORDLG_H

#include <windows.h>
#include <string>
#include <deque>

#include "GenRichEditCtrl.h"
#include "GenListCtrl.h"
#include "ClockDlg.h"

using namespace std;

class CStatusEditorDlg
{
public:

private:
	GenRichEditCtrl		richEdit;
public:
	void DoModal(HWND hParent);
	CStatusEditorDlg();
	CStatusEditorDlg(int nSel);
	~CStatusEditorDlg(void);
	static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void OnInitDialog(HWND hDlg, LPARAM lParam);
	static void OnAddText(HWND hDlg, HWND hWnd);
	static void OnAddClockText(HWND hDlg, HWND hWnd, bool bRemove, CClockDlg::CLOCK* pSpec);
	static void OnAddRepText(HWND hDlg, HWND hWnd, bool bRemove, GenListCtrl::REPETE* pSpec);
	static void OnAddSongDetText(HWND hDlg, HWND hWnd, bool bRemove, SPECIAL* pSpec);
	static void OnAddSongLyricsText(HWND hDlg, HWND hWnd, bool bRemove);
	static void OnRemoveText(HWND hDlg, WPARAM wParam);
	static void OnOk(HWND hDlg);
	static void WriteStatusToFile();
	static void OnEditText(HWND hDlg);
	static void ClearSelected(HWND hDlg, HWND hREdit);
};

#endif