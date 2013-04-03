#pragma once

#ifndef BUTTONBAR_H
#define BUTTONBAR_H

#include <windows.h>

#include "GenDll.h"

class CButtonBar
{
private:
	enum but:BYTE {nothing, fback, play, pause, stop, ffar};
	HWND		m_hWnd;
	RECT		m_rClient;
	HBITMAP		m_hbFBack, m_hbFFar, m_hbPlay, m_hbPause, m_hbStop;
	HBITMAP		m_hbFBack2, m_hbFFar2, m_hbPlay2, m_hbPause2, m_hbStop2;
	but			m_dClicked;
public:
	CButtonBar(void);
	~CButtonBar(void);
	void Create(HWND hParent, const RECT& rect);
private:
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void OnPaint(HWND hWnd);
	static void OnCommand(HWND hWnd, but nCode);
};

#endif//BUTTONBAR_H