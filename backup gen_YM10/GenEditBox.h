#pragma once

#ifndef GENEDITBOX_H
#define	GENEDITBOX_H

#include <windows.h>

class GenEditBox
{
private:
	int					m_nMaxLimit;
	static WNDPROC		OldWndProc;
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
	GenEditBox(void);
	~GenEditBox(void);
	void Create(HWND hWnd, int nMaxLimit)
	{
		m_nMaxLimit = nMaxLimit;
		OldWndProc = (WNDPROC)SetWindowLong(hWnd, GWL_WNDPROC, (LONG)WindowProc);
	}
};

#endif//GENEDITBOX_H