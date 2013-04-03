#pragma once

#ifndef CHOOSEDLG_H
#define CHOOSEDLG_H

#include <windows.h>
#include "HyperLink.h"
#include "GenListBox.h"

class CChooseDlg
{
private:
	CHyperLink				m_AboutLink;
	CHyperLink				m_IDLink;
	GenListBox				m_ListBox;
	bool					m_bStatic;

	HCURSOR					m_hArrowCursor, m_hHandCursor;
public:
	BOOL					m_bClicked;
	BOOL					m_bMoved;

public:
	CChooseDlg(void);
	~CChooseDlg(void);

	int DoModal(HWND hParent);
private:
	static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static int OnInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam);
	static void OnMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam);
	static void OnLButtonUp(HWND hWnd, WPARAM wParam, LPARAM lParam);
	static void OnOk(HWND hDlg);
	static void OnUpdateIDs(HWND hDlg);
};

#endif//CHOOSEDLG_H