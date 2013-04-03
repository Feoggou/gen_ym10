#pragma once

#ifndef CHOOSEDLG_H
#define CHOOSEDLG_H

#include <windows.h>
#include "HyperLink.h"
#include "GenListBox.h"
#include "GenDll.h"

//clasa se ocupa de caseta de dialog pentru alegerea statusului
//un singur obiect de clasa aceasta poate exista o data
class CChooseDlg
{
private:
	//hperlink-ul pentru afisarea casetei de dialog "Despre..."
	CHyperLink				m_AboutLink;
	//hyperlink-ul pentru alegerea unui ID de messenger din cele online
	CHyperLink				m_IDLink;
	//handle la fereastra (caseta de dialog)
	HWND					m_hDlg;

	//lista de statusuri
	GenListBox				m_ListBox;
	bool					m_bIsLink;
	static ID				m_SelID;

public:
	//constructorul
	CChooseDlg(void);
	//destructorul
	~CChooseDlg(void);
	//se creeaza o caseta de dialog modala.
	void DoModal(HWND hParent);

private:
	//functia de prelucrare a mesajelor
	static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//functia apelata cand se trimite mesajul WM_INITDIALOG
	int OnInitDialog();
	//functia apelata cand se apasa butonul Aplica
	void OnOk();
	//functie care actualizeaza lista de ID-uri online si m_IDLink.
	void OnUpdateIDs();
	//functia care este apelata cand este dat click pe link-ul m_AboutLink
	static void OnAbout();
	//functia care este apelata cand este dat click pe link-ul m_IDLink.
	static void OnMessID();
};

#endif//CHOOSEDLG_H