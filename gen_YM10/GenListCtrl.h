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

//clasa care se ocupa cu controlul lista/tabel folosit in casetele de dialog "Adauga Repetitie" si "Scrie versurile"
//controlul de aceasta clasa este controlul parinte. Contine CListSide (unde se stocheaza datele) si CHeaderSide (antetul)
class GenListCtrl
{
public:
	//elementele controlului sunt alcatuite din 'versuri'. un vers are un text (sVerse) si secunda (sau minutul)
	//asociat textului
	struct VERSE
	{
		WORD nrSec;
		wstring wsVerse;
	};

public:
	//datele care se vor stoca din acest control sunt versurile scrise;
	//In cazul Repetitiei, mai sunt relevante:
	//a. numarul de repetari (-1 daca nu se programeaza oprirea repetarii)
	//b. daca nrSec din inregistrarea VERSE specifica un minut sau o secunda (TRUE = minut; FALSE = secunda)
	struct REPETE
	{
		//daca controlul va fi folosit pentru repetitie si se alege sa se foloseasca minute in loc de secunde
		bool bIsMinute;
		int nRepCnt;
		deque<VERSE> Verses;
		REPETE()
		{
			nRepCnt = -1;
			bIsMinute = 0;
		}
	};

private:
	//datele se vor stoca in aceasta variabila membru
	REPETE				m_Repetition;
	//vechea functie de prelucrare a mesajelor controlului
	static WNDPROC		m_OldWndProc;
	//vechea functie de prelucrare a mesajelor a casetei de bifare IDC_CHECK_NOCOUNTER (daca se programeaza oprirea sau nu)
	static WNDPROC		OldCheckProc;
	//handle la fereastra controlului
	HWND				m_hWnd;

	//tabelul/lista in care se vor scrie datele (GenListCtrl este controlul parinte)
	CListSide			m_Table;
	//controlul de tip antet
	CHeaderSide			m_Header;
	//inaltimea textului, folosit pentru determinarea inaltimii antetului
	int					m_nTextHeight;
	//pozitia liniei care separa item-urile nrSec de sVerse
	int					m_nVBarPos;
	//specifica daca controlul va fi folosit pentru Versurile unei melodii (lyrics) sau pentru Repetitie.
	//acelasi control le implementeaza pe amandoua. daca este Lyrics, se pot selecta liniile
	bool				m_bIsLyrics;

public:
	//extremitatiile antetului, tabelului/liste, si a intregii zone client (care le contine pe amandoua)
	RECT				m_rHeader, m_rTable, m_rClient;
public:
	//constructorul
	GenListCtrl(void);
	//destructorul
	~GenListCtrl(void);
private:
	//noua functie de prelucrare a mesajelor controlului GenListCtrl
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//noua functie de prelucrare a mesajelor controlului checkbox, IDC_CHECK_NOCOUNTER
	static LRESULT CALLBACK CheckProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
	//functia folosita pentru crearea controlului
	void Create(HWND hWnd, bool bIsLyrics);
	
	//urmatoarele clase au dreptul sa utilizeze membrii privatzi ai GenListCtrl:
	friend class CRepetitionDlg;
	friend class CStatusEditorDlg;
	friend class CLyricsDlg;
	friend class CHeaderSide;
	friend class GenListItem;
};

#endif//GENLISTCTRL_H