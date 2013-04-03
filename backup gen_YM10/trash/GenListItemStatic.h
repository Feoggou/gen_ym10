#pragma once

#ifndef GENLISTITEMSTATIC_H
#define GENLISTITEMSTATIC_H

#include <windows.h>

class GenListItemStatic
{
public:
	HWND			m_hWnd;
	static WNDPROC	OldWndProc;
public:
	GenListItemStatic(void);
	~GenListItemStatic(void);
	void Create(HWND hParent, RECT& rect, int nLine);
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void OnPaint(HWND hWnd);
	static void OnLButtonDown(HWND hWnd, WPARAM wParam);
};

#endif//GENLISTITEMSTATIC_H