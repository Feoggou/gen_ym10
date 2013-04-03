#pragma once

#ifndef ABOUTDLG_H
#define ABOUTDLG_H

#include <windows.h>
#include "HyperLink.h"

//clasa CAboutDlg este clasa care se ocupa de caseta de dialog "Despre". Un singur obiect va fi creat de clasa
//CAboutDlg.
class CAboutDlg
{
private:
	//Control de tip Hyperlink, care afiseaza email-ul meu.
	CHyperLink		m_EmailLink;
	//Handle la fereastra (caseta de dialog)
	HWND			m_hDlg;

public:
	//constructorul
	CAboutDlg(void);
	//destructorul
	~CAboutDlg(void);

	//functie care creeaza caseta de dialog, de tip modal.
	void DoModal(HWND hParent);
private:
	//functia ce prelucreaza mesajele
	static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//functia care este apelata cand este trimis mesajul WM_INITDIALOG.
	int OnInitDialog();
	static void OnEmail();
};

#endif//ABOUTDLG_H