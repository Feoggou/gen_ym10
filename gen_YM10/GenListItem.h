#pragma once

#ifndef GENLISTITEM_H
#define GENLISTITEM_H

//se selecteaza intreaga linie de tabel. wParam = indexul liniei; lParam = hEdit (handle la caseta de editare
//Text de pe linia de index wParam).
#define WM_SELECTLINE		WM_USER + 4
//se adauga o linie in tabel, wparam: loword = index-ul liniei, hiword = pozitia caret-ului
//lParam: hEdit = caseta de editare in care s-a apasat Enter.
#define WM_ADDLINEAT		WM_USER + 5
//mesaj trimis pentru a copia textul din caseta de editare de handle lParam in caseta de editare de mai sus
//(indexul liniei de sus = wParam - 1), si de asterge linia de index wParam (handle-ul lParam e de index wParam),
//datorita apasarii tastei "Backspace" in caseta de editare, coloana Text
#define WM_DELLINEAT_BACK	WM_USER + 6
//mesaj trimis pentru a sterge linia de index wParam, si de a copia textul din caseta de editare de handle lParam
//(index tot wParam)
//in caseta de editare de mai sus (indexul liniei wParam - 1),
//datorita apasarii tastei "Delete" in caseta de editare, coloana Text
#define WM_DELLINEAT_DEL		WM_USER + 7

#include <windows.h>

//clasa se ocupa cu itemii unui CListSide. un item este creat fie ca o caseta de editare fie ca un control static
class GenListItem
{
public:
	//handle la fereastra item-ului
	HWND			m_hWnd;

private:
	//vechea functie de prelucrare a mesajelor
	static WNDPROC	OldWndProc;

public:
	//constructorul
	GenListItem(void);
	//destructorul
	~GenListItem(void);
	//functia de creare a item-ului
	//bText - daca item-ul va reprezenta Textul sau Secunda/minutul
	//bIsLyrics - va fi folosit in caseta de dialog pentru creare versurilor(lyrics) sau repetitie
	void Create(HWND hParent, RECT& rect, bool bText, bool bIsLyrics);

private:
	//noua functie de prelucrare a mesajelor, daca item-ul este caseta de editare
	static LRESULT CALLBACK EditProc(HWND hEdit, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//noua functie de prelucrare a mesajelor, in cazul in care item-ul este control static
	static LRESULT CALLBACK StaticProc(HWND hStatic, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//apelata la trimiterea mesajului WM_CONTEXTMENU, in cazul in care itemul este o caseta de editare
	static void OnEditContextMenu(HWND hEdit, WORD x, WORD y);
	//apelata la trimiterea unui mesaj WM_PAINT, in cazul in care item-ul este un control static
	static void OnPaintStatic(HWND hStatic);
	//apelata cand se apasa butonul din stanga al mouse-ului, in cazul in care item-ul este un control static
	static void OnLButtonDown(HWND hStatic, WPARAM wParam);
	//apelata la trimiterea mesajului WM_CONTEXTMENU, in cazul in care itemul este un control static
	static void OnStaticContextMenu(HWND hStatic, WORD x, WORD y);
};

#endif//GENLISTITEM_H