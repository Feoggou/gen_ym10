#pragma once

#ifndef LISTSIDE_H
#define LISTSIDE_H

#include <windows.h>
#include <deque>

#include "GenListItem.h"

using namespace std;

//un obiect de aceasta clasa va fi membru a unui control GenListCtrl.
//clasa se ocupa cu numarul secundei/minutului si versul (textul), cu afisarea si prelucrarea lor
class CListSide
{
public:
	//inregistrarea reprezinta o 'linie' a tabelului/listei.
	struct LINE
	{
		//item care prelucreaza valorile din Secunda
		GenListItem		edSecond;
		//item care prelucreaza valorile din Text
		GenListItem		edText;
		//specifica daca linia este selectata sau nu
		BYTE			bSelected;
		//indexul liniei
		int				nIndex;
	};

	//handle la fereastra controlului
	HWND			m_hWnd;

private:
	//o coada cu doua capete (double-ended queue) care contine toate liniile listei/tabelului
	deque<LINE>		m_Lines;
	//fontul folosit de control
	HFONT			m_hFont;
	//zona clientului
	RECT			m_rClient;
	//pozitia liniei verticale de despartire a itemilor "Secunda" de "Vers"
	int				m_nVBarPos;
	//inaltimea unei linii
	static int		m_nLineHeight;

	//PENTRU DERULAREA FERESTREI:
	//pozitia maxima de derulare
	int				m_dMaxScroll;
	//marimea (inaltimea) unei pagini in pixeli (cat de mult se poate vedea odata din intregul continut)
	int				m_dPage;
	//specifica daca se poate folosi sau nu bara de derulare (scrollbar)
	bool			m_bScrollEnabled;
	//pozitia barii de derulare
	int				m_dScrollPos;


	//index-ul liniei selectate. -1 daca nu e nici una
	int				m_nSelLine;
	//specifica daca acest control va fi utilizat pentru versurile unei melodii(lyrics) sau pentru repetitie
	bool			m_bIsLyrics;
	//primul item de tip static selectat; -1 daca nu e nici o selectie
	int				m_nFirstIndex;
	//ultimul item de tip static selectat; -1 daca nu e nici o selectie sau numai un singur static a fost selectat
	int				m_nLastIndex;
	//lista de siruri in care vor fi stocate textele (coloana Text) prin copiere(copy)/decupare(cut)
	//copierea sau decuparea realizandu-se prin GenListItem::OnStaticContextMenu pentru control de tip Lyrics
	//(adica folosit intr-o caseta de dialog pentru crearea versurilor unei melodii)
	deque<wstring>	m_Copy_Buf;

public:
	CListSide(void);
	~CListSide(void);
	//functia de creare a ferestrei
	void Create(HWND hParent, RECT rect, HFONT hFont, int nVBarPos, int nLineHeight, bool bIsLyrics);
	//functia de prelucrare a mesajelor
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	//apelata pentru a se adauga o linie noua la sfarsit
	void AddLine();
	//apelat de WM_ADDLINE datorita lui VK_RETURN apasat
	//AddLineAt adauga o linie la sfarsit (apeland AddLine) si apoi muta toate linile incepand cu nLine
	//mai jos cu o linie, astfel ca nLine sa fie goala, si toate celelalte sa aibe text (nu se pierde textul)
	void AddLineAt(int nLine);
	//elimina linia de la pozitia nLine. functia apeleaza RemoveLine
	void RemoveLineAt(int nLine);
	//elimina ultima linie
	void RemoveLine();
	//se actualizeaza bara de derulare (marimea, daca se poate derula, etc.)
	void UpdateScrollBar(int nBottom);

private:
	//functie apelata la trimiterea mesajului WM_VSCROLL
	void OnVScroll(WPARAM wParam);

public:
	//se deruleaza pana la o pozitie fixa
	void ScrollTo(int pos);
	//se deruleaza in sus sau in jos cu nMore pixeli.
	void Scroll(int nMore);

	//gaseste ce linie este 'selectata'. daca nici una nu e selectat, returneaza -1.
	inline int GetSelectedLine(void)
	{
		int size = m_Lines.size();
		int i = -1;
		for (i = 0; i < size; i++)
		{
			if (m_Lines[i].bSelected) return i;
		}

		return -1;
	}
	//se selecteaza o linie - valabil doar pentru lyrics
	inline void SelectLine(int nLine)
	{
		SendMessage(m_hWnd, WM_SELECTLINE, nLine, 0);
	}

	//urmatoarele clase au dreptul sa utilizeze membrii privatzi ai CListSide
	friend class GenListItem;
	friend class GenListCtrl;
	friend class CRepetitionDlg;
	friend class CLyricsDlg;
};

#endif//LISTSIDE_H