#pragma once

#ifndef ABOUTDLG_H
#define ABOUTDLG_H

#include <windows.h>
#include "HyperLink.h"

class CAboutDlg
{
private:
	CHyperLink		m_EmailLink;
	HCURSOR			m_hArrowCursor, m_hHandCursor;
	HWND			m_hDlg;

public:
	CAboutDlg(void);
	~CAboutDlg(void);

	int DoModal(HWND hParent);
private:
	static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	int OnInitDialog();
	void OnMouseMove(LPARAM lParam);
	void OnLButtonUp(LPARAM lParam);
};

#endif//ABOUTDLG_H