#pragma once
#ifndef HYPERLINK_H
#define HYPERLINK_H

#include <windows.h>

inline RECT MAKERECT(int left, int top, int right, int bottom)
{
	RECT rect;
	rect.left = left;
	rect.top = top;
	rect.right = right;
	rect.bottom = bottom;
	
	return rect;
}

typedef void (*CLICKPROC)();

//clasa se ocupa cu obiecte de tip hyperlink
class CHyperLink
{
public:
	//handle la fereastra
	HWND					m_hWnd;
	//specifica daca se va sublinia sau nu textul
	BOOL					m_bUnderline;
	//specifica daca se va folosi cu proprietatea de hyperlink sau nu
	BOOL					m_bIsLink;
	//cursorurile folosite
	static HCURSOR			m_hArrowCursor, m_hHandCursor;
	//functie ce specifica ce se va intampla cand se apasa click pe link (e pointer la functie)
	CLICKPROC				onClickProc;

public:
	//constructorul
	CHyperLink();
	//destructorul
	~CHyperLink();
public:
	//functia de prelucrare a mesajelor
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//functie apelata la trimiterea mesajului WM_PAINT, pentru desenarea controlului
	void OnPaint();
	//functie pentru crearea hyperlink-ului
	void Create(HWND hDlg, DWORD dwID, CLICKPROC onClick, BOOL bIsLink = true);
	//functie apelata la trimitererea mesajului WM_MOUSEMOVE
	void OnMouseMove();
	//functie folosita pentru a seta controlul ca hyperlink sau ca text normal (normal = asemenea controlului static)
	void SetLinkState(BOOL bIsLink);
};

#endif//HYPERLINK_H