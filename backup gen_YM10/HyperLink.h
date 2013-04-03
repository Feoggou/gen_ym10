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

// CHyperLink

class CHyperLink
{
public:
	HWND		m_hWnd;
	BOOL		m_bUnderline;
public:
	CHyperLink();
	virtual ~CHyperLink();
public:
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK NewParentProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static WNDPROC OldWndProc, OldParentProc;
	static void OnLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam);
	static void OnMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam);
	static void OnPaint(HWND hWnd);
	static void OnInitialize();
	void Create();
};

#endif//HYPERLINK_H