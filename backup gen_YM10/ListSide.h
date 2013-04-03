#pragma once

#ifndef LISTSIDE_H
#define LISTSIDE_H

#include <windows.h>
#include <deque>

#include "GenListItem.h"

using namespace std;

class CListSide
{
public:
	struct LINE
	{
		//item that handles second values
		GenListItem		edSecond;
		//item that handles text values
		GenListItem		edText;
		//specifies whether the line is selected or not
		BYTE			bSelected;
		//the index of the line
		int				nIndex;
	};

	HWND			m_hWnd;
private:
	deque<LINE>		m_Lines;
	static HFONT	m_hFont;
	RECT			m_rClient;
	int				m_nVBarPos;
	static int		m_nLineHeight;
	static int		m_dMaxScroll;
	static int		m_dPage;
	bool			m_bScrollEnabled;
	int				m_nSelLine;
	bool			m_bEnableLineSel;
	int				m_dScrollPos;
	bool			m_bIsLyrics;
	int				m_nFirstIndex;
	int				m_nLastIndex;
	deque<string>	m_Copy_Buf;
public:
	CListSide(void);
	~CListSide(void);
	void Create(HWND hParent, RECT rect, HFONT hFont, int nVBarPos, int nLineHeight, bool bEnableLineSel, bool bIsLyrics);
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
	void AddLine(bool bScroll = false);
	//requested by WM_ADDLINE because of VK_RETURN pressed, it moves all editboxes from below
	void AddLineAt(int nLine);
	//requested
	void RemoveLineAt(int nLine);
	//removes the last line
	void RemoveLine();
	void UpdateScrollBar(int nBottom);
private:
	static void OnVScroll(HWND hWnd, WPARAM wParam, LPARAM lParam);
public:
	void ScrollTo(/*HWND hWnd, */int pos);
	void Scroll(/*HWND hWnd, */int nMore);

	inline int GetSelectedLine(void)
	{
		int size = m_Lines.size();
		int i = -1;
		for (i = 0; i < size; i++)
		{
			if (m_Lines[i].bSelected) return i;
		}

		return -1;
	}
	inline void SelectLine(int nLine)
	{
		SendMessage(m_hWnd, WM_SELECTLINE, nLine, 0);
	}

	friend class GenListItem;
	friend class GenListCtrl;
	friend class CRepetitionDlg;
	friend class CLyricsDlg;
};

#endif//LISTSIDE_H