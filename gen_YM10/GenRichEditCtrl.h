#pragma once

#ifndef GENRICHEDITCTRL_H
#define GENRICHEDITCTRL_H

#include <windows.h>
#include <richedit.h>

class GenRichEditCtrl
{
private:
	static WNDPROC		m_OldWndProc;
public:
	HWND				m_hWnd;

public:
	GenRichEditCtrl(void);
	~GenRichEditCtrl(void);

	void Create(HWND hWnd);
private:
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void OnChar(int nChar, LPARAM lParam);
	void OnKeyDown(int nChar, LPARAM lParam);
};

#endif//GENRICHEDITCTRL_H