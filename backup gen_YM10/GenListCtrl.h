#pragma once

#ifndef GENLISTCTRL_H
#define GENLISTCTRL_H

#define GLC_LALIGN	0
#define GLC_CALIGN	1
#define GLC_RALIGN	2

#include <windows.h>
#include <deque>
#include <string>

#include "GenListItem.h"
#include "ListSide.h"
#include "HeaderSide.h"

using namespace std;

class GenListCtrl
{
public:
	struct VERSE
	{
		WORD nrSec;
		string sVerse;
	};

public:
	struct REPETE//: public SPECIAL
	{
		int nRepCnt;
		deque<VERSE> Verses;
	};

private:
	REPETE				m_Repetition;
	static WNDPROC		m_OldWndProc;
	static WNDPROC		OldCheckProc;
	HWND				m_hWnd;

	CListSide			m_Table;
	CHeaderSide			m_Header;
	int					m_nTextHeight;
	int					m_nVBarPos;
	bool				m_bEnableLineSel;
	bool				m_bIsLyrics;

//	static RECT m_rClient;
public:
	RECT				m_rHeader, m_rTable, m_rClient;
public:
	GenListCtrl(void);
	~GenListCtrl(void);
private:
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK CheckProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
	void Create(HWND hWnd, bool bEnableLineSel, bool bIsLyrics);
	//the main function that adds a line
	friend class CRepetitionDlg;
	friend class CStatusEditorDlg;
	friend class CLyricsDlg;
};

#endif//GENLISTCTRL_H