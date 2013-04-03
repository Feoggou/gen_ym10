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
	GenRichEditCtrl(void);
	~GenRichEditCtrl(void);
	void Create(HWND hWnd);
private:
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void OnChar(HWND hWnd, WPARAM wParam, LPARAM lParam);
	static void OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam);
};

#endif//GENRICHEDITCTRL_H