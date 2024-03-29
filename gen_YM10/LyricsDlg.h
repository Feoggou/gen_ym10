#pragma once

#ifndef LYRICSDLG_H
#define LYRICSDLG_H

#include <windows.h>

#include "GenDll.h"
#include "GenListCtrl.h"
#include "ButtonBar.h"
#include "HyperLink.h"

//clasa folosita pentru caseta de dialog de creeare a versurilor unei melodii.
class CLyricsDlg
{
private:
	//un obiect de tipul GenListCtrl, in care se vor scrie versurile melodiei
	GenListCtrl				m_List;
	//imaginile butonului "Seteaza" (cand este apasat si cand nu)
	HBITMAP					m_hbSet, m_hbSet2;
	//hyperlink-uri pentru a incarca text dintr-un fisier sau a gasi pe web lyrics
	CHyperLink				m_LoadFromFile, m_LoadFromWeb;
	//specifica daca s-a apasat sau nu pana acum butonul set.
	bool					m_bIsSet;
	//indica numarul fisierului .lyr care se editeaza. -1 daca nu se editeaza.
	int						m_nFileNr;

	//butoanele pentru pornirea, oprirea, pauza, etc. a melodiei din winamp
	CButtonBar				m_ButBar;

public:
	//handle la fereastra casetei de dialog
	HWND					m_hDlg;

public:
	//constructorul
	CLyricsDlg(void);
	//destructorul
	~CLyricsDlg(void);

	//functia pentru crearea ferestrei, se creaza modal; daca nSel != -1, se editeaza unul existent
	INT_PTR DoModal(HWND hParent, int nFileNr = -1);

private:
	//functia de prelucrare a mesajelor casetei de dialog.
	static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//functia de prelucrare a timer-ului
	static void CALLBACK OnTimer(HWND hDlg, UINT uMsg, UINT_PTR nIDEvent, DWORD dwTime);

	//se preiau informatiile bazandu-se pe numele fisierului, ce este scris in fereastra winamp
	static void UpdateFromWinamp(HWND hDlg);
	//functie apelata cand se apasa butonul Aplica
	void OnOk();
	//functie apelata la trimiterea mesajului WM_INITDIALOG, pentru initializarea casetei de dialog
	void OnInitDialog();
	//se creeaza si seteaza lista de siruri (versuri), avand o variabila string ce contine tot textul.
	static void SetList(HWND hDlg, wstring& wsText);
	//se apasa butonul "Seteaza"
	void OnSet();
	//se deschide fisierul .lyr corespunzator
	HANDLE OpenLyricsFile(int& nFileNr);
	//se apasa click pe "Încarcă versurile dintr-un fișier..."
	static void OnLoadFromFile();
	//se apasa click pe "Caută versuri pe Internet"
	static void OnLoadFromWeb();
};

#endif//LYRICSDLG_H