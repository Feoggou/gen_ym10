#pragma once

#ifndef BUTTONBAR_H
#define BUTTONBAR_H

#include <windows.h>

#include "GenDll.h"

//aceasta clasa se ocupa de butoanele de pornire, oprire, pauza, derulare inainte si inapoi a melodiei
//din caseta de dialog a clasei CLyricsDlg
class CButtonBar
{
private:
	//ce anume a fost apasat
	enum but:BYTE {nothing, fback, play, pause, stop, ffar};
	//handle la fereastra controlului CButtonBar
	HWND		m_hWnd;

	//stocheaza extremitatiile zonei client a ferestrei CButtonBar, folosita pentru calculul extremitatiilor
	//fiecarui buton
	RECT		m_rClient;
	//imaginile butoanelor
	HBITMAP		m_hbFBack, m_hbFFar, m_hbPlay, m_hbPause, m_hbStop;
	HBITMAP		m_hbFBack2, m_hbFFar2, m_hbPlay2, m_hbPause2, m_hbStop2;
	//specifica ce buton a fost apasat
	but			m_dClicked;
public:
	//constructorul
	CButtonBar(void);
	//destructorul
	~CButtonBar(void);

	void Create(HWND hParent, const RECT& rect);

private:
	//functia de prelucrare a mesajelor
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//functia apelata cand se trimite un mesaj WM_PAINT
	void OnPaint();
	//apelat cand se trimite mesajul WM_LBUTTONUP si pune in functiune fiecare buton din bara
	void OnCommand(but nCode);
};

#endif//BUTTONBAR_H