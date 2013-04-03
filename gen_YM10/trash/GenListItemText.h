#pragma once

#ifndef GENLISTITEMTEXT_H
#define GENLISTITEMTEXT_H

#include <windows.h>

#include "ListSide.h"
#include "GenDll.h"

class GenListItemText
{
public:
	HWND			m_hWnd;
	static WNDPROC	OldWndProc;
public:
	GenListItemText(void);
	~GenListItemText(void);
	void Create(HWND hParent, RECT& rect);
	static LRESULT CALLBACK WindowProc(HWND hEdit, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif//GENLISTITEMTEXT_H