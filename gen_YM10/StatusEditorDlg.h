#pragma once

#ifndef STATUSEDITORDLG_H
#define STATUSEDITORDLG_H

#include <windows.h>

#include "GenRichEditCtrl.h"
#include "GenListCtrl.h"
#include "ClockDlg.h"

//clasa se ocupa cu caseta de dialog "Creaza statusul"
class CStatusEditorDlg
{
public:
	//handle la fereastra casetei de dialog
	HWND				m_hDlg;

private:
	//controlul RichEdit
	GenRichEditCtrl		richEdit;

public:
	//functia pentru crearea casetei de dialog ca modala
	INT_PTR DoModal(HWND hParent);
	//constructorul
	CStatusEditorDlg();
	//destructorul
	~CStatusEditorDlg(void);

private:
	//functia de prelucrare a mesajelor casetei de dialog
	static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//functia apelata la trimiterea mesajului WM_INITDIALOG
	void OnInitDialog();
	//functie apelata la apasarea butonului "Adauga"
	void OnAddText(HWND hWnd);
	//functie apelata la alegerea din meniul "Adauga" a elementului "Ceas"
	void OnAddClockText(HWND hWnd, CClockDlg::CLOCK* pSpec);
	//functie apelata la alegerea din meniul "Adauga" a elementului "Repetitie"
	void OnAddRepText(HWND hWnd, GenListCtrl::REPETE* pSpec);
	//functie apelata la alegerea din meniul "Adauga" a elementului "Detalii despre melodie"
	void OnAddSongDetText(HWND hWnd, BYTE songdet);
	//functie apelata la alegerea din meniul "Adauga" a elementului "Versuri pentru melodie"
	void OnAddSongLyricsText(HWND hWnd);
	//functie apelata la apasarea butonului "Elimina"; din alte parti pentru acelasi efect (elimina textul
	//selectat, cu tot cu orice SPECIAL ce il contine)
	void OnRemoveText();
	//functie apelata la apasarea butonului Editare, pentru editarea special-ului
	void OnEditText();
	//functie apelata la apasarea butonului Aplica
	void OnOk();
	//functie apelata pentru a scrie intr-un fisier noul status
	void WriteStatusToFile();
	//functie folosita pentru a gasi selectia din RichEdit si a elimina textul selectat
	//(folosit inainte de adaugarea unui SPECIAL in OnAddXXText).
	void ClearSelected(HWND hREdit);
};

#endif