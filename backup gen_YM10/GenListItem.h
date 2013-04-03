#pragma once

#ifndef GENLISTITEM_H
#define GENLISTITEM_H

#define WM_SELECTLINE	WM_USER + 4
#define WM_ADDLINEAT	WM_USER + 5
#define WM_DELLINEAT	WM_USER + 6
#define WM_DELLINEDELAT	WM_USER + 7

#include <windows.h>

class GenListItem
{
public:
	HWND			m_hWnd;
private:
	static WNDPROC	OldWndProc;
public:
	GenListItem(void);
	~GenListItem(void);
	void Create(HWND hParent, RECT& rect, int nLine, bool bText, bool bIsFirst, bool bIsLyrics);
private:
	static LRESULT CALLBACK EditProc(HWND hEdt, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK StaticProc(HWND hStatic, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void OnContextMenu(HWND hEdit, WORD x, WORD y);
	static void OnStaticPaint(HWND hStatic);
	static void OnLButtonDown(HWND hStatic, WPARAM wParam);
	static void OnStaticContextMenu(HWND hStatic, WORD x, WORD y);
};

#endif//GENLISTITEM_H