#pragma once

#ifndef GENLISTBOX_H
#define GENLISTBOX_H

#define ID_ADDSTATUSMSG			3223221
#define ID_EDITSTATUSMSG		3223222
#define ID_DELETESTATUSMSG		3223223

#include <windows.h>

//clasa se ocupa cu lista de statusuri (care sunt stocate pe hard disk).
//permite adaugarea, stergerea, vizualizarea statusurilor
class GenListBox
{
private:
	//handle la fereastra obiectului, si la parintele ferestrei
	HWND				m_hWnd, m_hParent;
	//vechea functie de prelucrare a mesajelor
	static WNDPROC		m_OldWndProc;

public:
	//constructor-ul
	GenListBox(void);
	//destructorul
	~GenListBox(void);

	//functia pentru crearea ferestrei
	void Create(HWND hWnd, HWND hParent);

private:
	//noua functie de prelucrare a mesajelor
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//functie apelata la trimiterea mesajului WM_CONTEXTMENU. Creeaza meniul contextual
	void OnContextMenu(int x, int y);
	//functie apelata cand se alege "Sterge statusul" din meniul contextual al ferestrei (listbox)
	//functie apelata din OnContextMenu
	void OnDeleteString(int nSel);
};

#endif//GENLISTBOX_H