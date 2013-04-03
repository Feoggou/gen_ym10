#pragma once

#ifndef HEADERSIDE_H
#define HEADERSIDE_H

#include <windows.h>

//clasa se ocupa cu antetul tabelului. un obiect din aceasta clasa va fi membru unui obiect de clasa
//GenListCtrl
class CHeaderSide
{
public:
	//handle la fereastra
	HWND			m_hWnd;
	//pozitia barii verticale, care separa casetele de editare pentru secunda/minut, de casetele de editare Text
	int				m_nVBarPos;
	//fontul pe care obiectul il va folosi
	HFONT			m_hFont;
	//specifica daca se va scrie "Minutul" sau "Secunda"
	bool*			m_pbIsMinute;

public:
	//constructorul
	CHeaderSide(void);
	//destructorul
	~CHeaderSide(void);

	//functia de creare a antetului (headerside)
	void Create(HWND hParent, RECT rect, HFONT hFont, int& nVBarPos);

private:
	//functia de prelucrare a mesajelor controlului
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//functia apelata pentru desenarea continutului antetului
	void OnPaint();
};

#endif//HEADERSIDE_H