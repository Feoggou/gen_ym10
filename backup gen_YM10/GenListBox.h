#pragma once

#ifndef GENLISTBOX_H
#define GENLISTBOX_H

#define ID_ADDSTATUSMSG			3223221
#define ID_EDITSTATUSMSG		3223222
#define ID_DELETESTATUSMSG		3223223

#include <windows.h>

class GenListBox
{
private:
	HWND				m_hWnd, m_hParent;
	static WNDPROC		m_OldWndProc;
public:
	GenListBox(void);
	~GenListBox(void);
private:
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void OnContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam);
public:
	void Create(HWND hWnd, HWND hParent);
	static void OnDeleteString(HWND hWnd, int nSel);
};

#endif//GENLISTBOX_H