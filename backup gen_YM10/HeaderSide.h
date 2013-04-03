#pragma once

#ifndef HEADERSIDE_H
#define HEADERSIDE_H

#include <windows.h>

class CHeaderSide
{
public:
	HWND			m_hWnd;
	static int		m_nVBarPos;
	static HFONT	m_hFont;
private:
public:
	CHeaderSide(void);
	~CHeaderSide(void);
	void Create(HWND hParent, RECT rect, HFONT hFont, int& nVBarPos);
private:
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void OnPaint(HWND hWnd);
};

#endif//HEADERSIDE_H