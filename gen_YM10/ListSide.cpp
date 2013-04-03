#include "ListSide.h"
#include "GenDll.h"
#include "Tools.h"

#define TABLECLASS	L"TABLECLASS"

extern CGenDll genDll;
int CListSide::m_nLineHeight;

CListSide::CListSide(void)
{
	m_dMaxScroll = -1;
	m_nLineHeight = -1;
	m_dPage = -1;
	m_nSelLine = -1;
	m_bScrollEnabled = false;
	m_dScrollPos = 0;
	m_nFirstIndex = -1;
	m_nLastIndex = -1;
}

CListSide::~CListSide(void)
{
	//se elimina handle-ul la font. intrucat pot exista doua obiecte de clasa CListSide la un moment dat
	//(scrierea unui Lyrics si Repetitie), fiecare va crea font-ul lui propriu si il va sterge cand nu va mai avea
	//nevoie de el.
	DeleteObject(m_hFont);
	//daca sunt linii in lista
	if (m_Lines.size())
	{
		deque<LINE>::iterator I;
		//atunci distrugem fiecare caseta de editare pe care fiecare item o reprezinta
		for (I = m_Lines.begin(); I != m_Lines.end(); I++)
		{
			if (IsWindow(I->edSecond.m_hWnd)) DestroyWindow(I->edSecond.m_hWnd);
			if (IsWindow(I->edText.m_hWnd)) DestroyWindow(I->edText.m_hWnd);
		}

		//de asemenea eliminam toate elementele din m_Lines.
		m_Lines.clear();
	}
}

void CListSide::Create(HWND hParent, RECT rect, HFONT hFont, int nVBarPos, int nLineHeight, bool bIsLyrics)
{
	m_bIsLyrics = bIsLyrics;
	//rect.bottom data ca parametru = dimensiunea dy = y1 - y0(=0) + 1
	//ex. pt o linie verticala ce are punctele y: 0, 1, 2, 3. dy = 4, y0 = 0, y1 = 3; dy = y1 - y0(=0) + 1 = 4
	//deci, extremitatea y1 (rect.bottom) va fi de fapt
	rect.bottom--;
	//la fel pentru dx = rect.right;
	rect.right--;

	//daca nu exista clasa o cream
	WNDCLASS wndclass;
	if (FALSE == GetClassInfoW(genDll.m_hInstance, TABLECLASS, &wndclass))
	{
		ZeroMemory(&wndclass, sizeof(WNDCLASS));

		wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass.hInstance = genDll.m_hInstance;
		wndclass.lpfnWndProc = WindowProc;
		wndclass.lpszClassName = TABLECLASS;
		wndclass.style = CS_VREDRAW | CS_HREDRAW;

		if (FALSE ==  RegisterClassW(&wndclass))
		{
			DisplayError(0);
			PostQuitMessage(-1);
		}
	}

	//se creeaza controlul
	m_hWnd = CreateWindowW(TABLECLASS, 0, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_VSCROLL, rect.left, rect.top, 
rect.right - rect.left + 1, rect.bottom - rect.top + 1, hParent, NULL, genDll.m_hInstance, NULL);

	if (m_hWnd == 0)
	{
		DisplayError(0);
		PostQuitMessage(-1);
	}

	//se gasesc extremitatiile zonei client
	GetClientRect(m_hWnd, &m_rClient);
	//se stocheaza pozitia liniei verticale
	m_nVBarPos = nVBarPos;

	//se stocheaza data asociata ferestrei: va fi pointer la obiectul acesta care a creat fereastra
	SetWindowLongPtrW(m_hWnd, GWL_USERDATA, (LONG)this);
	//nu va fi permisa utilizarea barii de derulare in momentul in care lista este creata
	EnableScrollBar(m_hWnd, SB_VERT, ESB_DISABLE_BOTH);
	
	m_hFont = hFont;
	//se stocheaza inaltimea unei linii
	m_nLineHeight = nLineHeight;
	//se adauga o linie automat.
	AddLine();
}

LRESULT CALLBACK CListSide::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//gasim pointer la obiectul care se ocupa cu aceasta fereastra.
	CListSide* pThis = (CListSide*)GetWindowLongPtrW(hWnd, GWL_USERDATA);

	if (pThis)
	switch (uMsg)
	{
		case WM_KEYDOWN:
		{
			int nScrollNotify = -1;

			//bFoundT = a gasit caseta "Text"; bFoundS = a gasit caseta "Secunda"/"Minut"
			bool bFoundT = false, bFoundS = false;
			//caseta de editare care are focus-ul
			HWND hEdit = GetFocus();
		
			//gasim care este, din lista m_Lines, caseta de editare hEdit
			deque<CListSide::LINE>::iterator I;
			for (I = pThis->m_Lines.begin(); I != pThis->m_Lines.end(); I++)
			{
				if (I->edText.m_hWnd == hEdit)
				{
					bFoundT = true;
					break;
				}
				if (I->edSecond.m_hWnd == hEdit)
				{
					bFoundS = true;
					break;
				}
			}

			//in functie de tasta apasata, vom trimite comanda barii de derulare
			switch (wParam)
			{
			case VK_SHIFT: break;
			case VK_NEXT:
				//se va derula la pagina urmatoare
				nScrollNotify = SB_PAGEDOWN; break;
			case VK_PRIOR:
				//se va derula la pagina precedenta
				nScrollNotify = SB_PAGEUP; break;
			case VK_HOME:
				//se va derula la inceputul listei
				nScrollNotify = SB_TOP; break;
			case VK_END:
				//se va derula la sfarsitul listei
				nScrollNotify = SB_BOTTOM; break;
			case VK_UP:
				{
					//se va derula in sus, doar dak nu suntem deja pe prima linie
					if (I->nIndex != 0)
					{
						//la apasarea tastei sageata in sus, se va muta cursorul deasupra
						if (bFoundT)
							SetFocus(pThis->m_Lines[I->nIndex - 1].edText.m_hWnd);
						if (bFoundS)
							SetFocus(pThis->m_Lines[I->nIndex - 1].edSecond.m_hWnd);
						
						//gasim extremitatiile liniei de deasupra, in coordonate client parinte
						RECT rect;
						GetWindowRect(pThis->m_Lines[I->nIndex-1].edText.m_hWnd, &rect);
						POINT pt = {rect.left, rect.top};
						ScreenToClient(hWnd, &pt);
						//daca linia cu focus-ul este pe pagina de mai sus, sau,
						//inaltimea paginii este mai mica decat partea de sus a casetei de editare
						//(pozitia de sus este relativa fata de pagina, adica, oriunde ar fi pagina in continut,
						//poz e deasupra paginii: poz < 0; poz e dedesuptul paginii: poz > 0)
						
						//deci, daca caseta de editare ce a primit focus-ul nu e vizibila utilizatorului,
						//se deruleaza astfel ca partea de sus a liniei de tabel sa fie prima linie afisata pe pagina
						if (pt.y < 0 || pThis->m_dPage < pt.y)
							pThis->ScrollTo(pt.y + pThis->m_dScrollPos);
						else
						{
							//dar, daca caseta de editare care primeste focus-ul este vizibila pe pagina,
							//gasim pozitia de sus a liniei de tabel care AVEA focus-ul, in coord client parinte
							GetWindowRect(pThis->m_Lines[I->nIndex].edText.m_hWnd, &rect);
							pt.x = rect.left; pt.y = rect.top;
							ScreenToClient(hWnd, &pt);
							//daca caseta de editare ce avea focus-ul e deasupra paginii
							//ne mutam cu o linie mai sus
							if (pt.y < 0)
								nScrollNotify = SB_LINEUP;
						}
					}
					
				}
				break;
			case VK_DOWN:
				{	
					//daca se apasa tasta in jos, dar nu suntem pe ultima linie de tabel
					if (I->nIndex != pThis->m_Lines.back().nIndex)
					{
						//se muta cursorul pe linia de mai jos (caseta de editare de mai jos primeste focus-ul)
						if (bFoundT)
							SetFocus(pThis->m_Lines[I->nIndex + 1].edText.m_hWnd);
						if (bFoundS)
							SetFocus(pThis->m_Lines[I->nIndex + 1].edSecond.m_hWnd);

				
						//gasim extremitatiile liniei ce primeste focus-ul in coordonate client parinte.
						//ne intereseaza doar punctul de jos din dreapta (item-ul "Text")
						RECT rect;
						GetWindowRect(pThis->m_Lines[I->nIndex + 1].edText.m_hWnd, &rect);
						POINT pt = {rect.right, rect.bottom};
						ScreenToClient(hWnd, &pt);

						//daca punctul dreapta jos este mai sus de pagina
						//sau este mai jos de pagina
						//derulam la pozitia, partea de jos a casetei de editare ce a primit focusul
						//astfel ca ea sa apara ultima linie de tabel.
						if (pt.y < 0 || pt.y >= pThis->m_dPage)
							pThis->ScrollTo(pt.y + pThis->m_dScrollPos - pThis->m_dPage);
						else
						{
							//gasim extremitatiile casetei ce avea focus-ul in coordonate client parinte
							GetWindowRect(pThis->m_Lines[I->nIndex].edText.m_hWnd, &rect);
							//ne intereseaza doar punctul de stanga de jos
							pt.x = rect.left; pt.y = rect.bottom;
							ScreenToClient(hWnd, &pt);
//							if (pt.y  >= pThis->m_dPage)
//								nScrollNotify = SB_LINEDOWN;
						}
						
					} 
				}
				break;

			default:
				{	
					//daca a fost apasata alta tasta in caseta de editare
					//nu ar trebui niciodata sa fie aici "break;"
					if (!bFoundT && !bFoundS) break;
					//gasim extremitatiile casetei de editare, punctul stanga sus, coordonate client parinte
					RECT rect;
					GetWindowRect(pThis->m_Lines[I->nIndex].edText.m_hWnd, &rect);
					POINT pt = {rect.left, rect.top};
					POINT pt2 = {rect.left, rect.bottom};
					ScreenToClient(hWnd, &pt);
					ScreenToClient(hWnd, &pt2);
					//daca punctul este deasupra paginii sau sub pagina
					if (0 > pt.y)
						//derulam la caseta de editare astfel ca ea sa fie prima linie din tabel
						pThis->ScrollTo(pt.y + pThis->m_dScrollPos);
					else if (pt2.y >= pThis->m_dPage)
						//derulam la caseta de editare astfel ca ea sa fie ultima linie din tabel
						pThis->ScrollTo(pt2.y + pThis->m_dScrollPos - pThis->m_dPage);
					else
					{
						//daca punctul este in interiorul paginii
						GetWindowRect(pThis->m_Lines[I->nIndex].edText.m_hWnd, &rect);
						pt.x = rect.left; pt.y = rect.top;
						ScreenToClient(hWnd, &pt);
					}
				}
				break;
			}

			//daca a fost ales un cod pentru derulare, se trimite mesajul WM_VSCROLL pentru a efectua derularea
			if (nScrollNotify!=-1)
			{
				SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(nScrollNotify,0), 0);
			}
		}
		break;//WM_KEYDOWN

	case WM_MOUSEWHEEL:
		{
			int delta = (short)HIWORD(wParam);
			if (delta < 0)
				//daca s-a rotit scroll-ul mouse-ului in jos
				SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN,0), 0);
			else
				//daca s-a rotit scroll-ul mouse-ului in sus
				SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEUP,0), 0);
		}
		break;

	//daca s-a trimis mesajul WM_VSCROLL
	case WM_VSCROLL: pThis->OnVScroll(wParam); break;
//se adauga o linie in tabel, wparam: loword = index-ul liniei, hiword = pozitia caret-ului
//lParam: hEdit = caseta de editare in care s-a apasat Enter.
	case WM_ADDLINEAT:
		{
			int nLine = LOWORD(wParam);
			int nSelEnd = HIWORD(wParam);
			HWND hEdit = (HWND)lParam;

			//mai intai se adauga o linie goala pe pozitia nLine+1, celelalte linii 'deplasandu-se' mai jos
			pThis->AddLineAt(nLine+1);
			//preluam lungimea textului din caseta de editare unde s-a apasat Enter.
			int len = GetWindowTextLengthW(hEdit);
			len++;
			WCHAR* wstr = new WCHAR[len];
			//preluam textul:
			GetWindowTextW(hEdit, wstr, len);
			//ne intereseaza textul cuprins intre nSelEnd si len-1.
			
			//daca avem text intre nSelEnd si len-1
			if (nSelEnd < len-1)
			{
				//folosim ch pentru a pastra wstr[nSelEnd], caci modificam wstr[nSelEnd].
				WCHAR ch = wstr[nSelEnd];
				wstr[nSelEnd] = '\0';
				//scriem textul in caseta de sus pana la pozitia caret-ului
				SetWindowTextW(hEdit, wstr);
				//refacem wstr
				wstr[nSelEnd] = ch;
				//ne mutam pe caseta de editare de mai jos
				hEdit = pThis->m_Lines[nLine + 1].edText.m_hWnd;
				//scriem textul incepand cu nSelEnd
				SetWindowTextW(hEdit, wstr + nSelEnd);
			}
			else
				//avem nevoie de handle-ul la caseta de jos
				hEdit = pThis->m_Lines[nLine + 1].edText.m_hWnd;

			//mutam caret-ul pe pozitia de jos
			SetFocus(hEdit);
			//mutam caret-ul pe prima pozitie a casetei de editare.
			SendMessage(hEdit, EM_SETSEL, 0, 0);
			SendMessage(hWnd, WM_KEYDOWN, VK_RETURN, 0);
			delete[] wstr;
		}
		break;

//mesaj trimis pentru a copia textul din caseta de editare de handle lParam in caseta de editare de mai sus
//(indexul liniei de sus = wParam - 1), si de asterge linia de index wParam (handle-ul lParam e de index wParam),
//datorita apasarii tastei "Backspace" in caseta de editare, coloana Text
	case WM_DELLINEAT_BACK:
		{
			int nLine = (int)wParam;
			HWND hEdit= (HWND)lParam;

			//copiem continutul din linia de jos:
			int len = GetWindowTextLengthW(hEdit);
			len++;
			WCHAR* wstr = new WCHAR[len];
			int len2;
						
			//preluam textul in wstr
			GetWindowTextW(hEdit, wstr, len);
			//ne mutam pe linia de sus
			hEdit = pThis->m_Lines[nLine - 1].edText.m_hWnd;
			//copiem textul din caseta de sus in wstr2
			len2 = GetWindowTextLengthW(hEdit);
			len2++;
			WCHAR* wstr2 = new WCHAR[len2];
			GetWindowTextW(hEdit, wstr2, len2);

			//rezultatul wsir:
			wstring wsir = wstr2;
			if (len > 1)
			{
				wsir += ' ';
				wsir += wstr;
			}
			//scriem rezultatul in caseta de sus:
			SetWindowTextW(hEdit, wsir.data());
			//nu mai avem nevoie de wstr2
			delete[] wstr2;

			//stergem linia de jos
			pThis->RemoveLineAt(nLine+1);
			//mutam cursorul in caseta de editare de mai sus
			SetFocus(hEdit);
			SendMessage(hEdit, EM_SETSEL, len2, len2);
			delete[] wstr;
		}
		break;

//mesaj trimis pentru a sterge linia de index wParam, si de a copia textul din caseta de editare de handle lParam
//(index wParam)
//in caseta de editare de mai sus (indexul liniei wParam - 1),
//datorita apasarii tastei "Delete" in caseta de editare, coloana Text
	case WM_DELLINEAT_DEL:
		{
			//intrucat nu putem avea un numar de linii prea mare, conversia din 'fara semn' in 'cu semn' e fara probleme
			int nLine = (int)wParam;
			HWND hEdit= (HWND)lParam;

			//copiem continutul din linia de jos:
			int len = GetWindowTextLengthW(hEdit);
			len++;
			WCHAR* wstr = new WCHAR[len];

			int len2;
			wstring wsir;
			
			//preluam textul in wstr
			GetWindowTextW(hEdit, wstr, len);
			//ne deplasam pe caseta de editare de mai sus
			//avem nevoie sa gasim handle-ul la caseta de editare de mai sus
			hEdit = pThis->m_Lines[nLine - 1].edText.m_hWnd;
			//copiem textul din caseta in sirul wstr2
			len2 = GetWindowTextLengthW(hEdit);
			len2++;
			WCHAR* wstr2 = new WCHAR[len2];
			GetWindowTextW(hEdit, wstr2, len2);
			
			//sirul rezultat va fi:
			wsir = wstr2;
			if (len > 1)
			{
				wsir += ' ';
				wsir += wstr;
			}
			//nu mai avem nevoie de str2
			delete[] wstr2;

			//in RemoveLineAt(nLine), se copiaza textul de dedesupt, inclusiv secunda. deci trebuie sa
			//le salvez si sa le copiez dupa
			wstr2 = new WCHAR[7];
			HWND hEditSecond = pThis->m_Lines[nLine - 1].edSecond.m_hWnd;
			GetWindowTextW(hEditSecond, wstr2, 6);

			//eliminam linia de mai jos (index nLine)
			pThis->RemoveLineAt(nLine);
			SetWindowTextW(hEditSecond, wstr2);
			delete[] wstr2;
			//hEdit e acum handle la caseta de editare de sus
			SetWindowTextW(hEdit, wsir.data());
			////mutam cursorul in caseta de editare de sus
			SetFocus(hEdit);
			SendMessage(hEdit, EM_SETSEL, len2, len2);
			delete[] wstr;
		}
		break;
		
		//wParam = indexul liniei; lParam = hEdit;
	case WM_SELECTLINE:
		{
			//doar pentru Lyrics
			if (pThis->m_bIsLyrics == false)
				break;

			int nLine = (int)wParam;

			//daca este linie selectata, o deselectam
			if (pThis->m_nSelLine > -1)
			{
				pThis->m_Lines[pThis->m_nSelLine].bSelected = false;
				//asta inseamna ca redesenez ambii itemi
				RedrawWindow(pThis->m_Lines[pThis->m_nSelLine].edText.m_hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
				RedrawWindow(pThis->m_Lines[pThis->m_nSelLine].edSecond.m_hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
			}

			//daca linia selectata e alta decat cea pe care vrem sa o selectam
			if (pThis->m_nSelLine != nLine)
			{
				//inregistram nLine ca fiind linia selectata a tabelului
				pThis->m_nSelLine = nLine;
				//o setam ca selectata
				pThis->m_Lines[nLine].bSelected = true;
				//desenam ambii itemi
				RedrawWindow(pThis->m_Lines[nLine].edText.m_hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
				RedrawWindow(pThis->m_Lines[nLine].edSecond.m_hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);

				//intrucat se poate selecta urmatoarea linie (care poate fi sub pagina) la apasarea butonului
				//"seteaza" trebuie sa se deruleze astfel ca linia selectata sa fie vizibila.
				RECT rect;
				GetWindowRect(pThis->m_Lines[nLine].edText.m_hWnd, &rect);
				POINT pt = {rect.right, rect.bottom};
				ScreenToClient(hWnd, &pt);

				if (pThis->m_dScrollPos > pt.y || pThis->m_dScrollPos + pThis->m_dPage - 1 < pt.y || pt.y  >= pThis->m_dPage)
					pThis->ScrollTo(pt.y + pThis->m_dScrollPos - pThis->m_dPage);
			}
			//altfel inseamna ca am dat WM_SELECTLINE pentru o linie selectata, deci am deselectat-o, deci
			//nu mai e nici o linie selectata
			else pThis->m_nSelLine = -1;
		}
		break;//WM_SELECTLINE

	//trimis ferestrei parinte de catre caseta de editare;
	//mesajul e folosit pentru specificarea culorii textului si culorii fundalului
	case WM_CTLCOLOREDIT:
		{
			//handle la DC-ul casetei de editare
			HDC hDC = (HDC)wParam;
			//handle la caseta de editare
			HWND hEdit = (HWND)lParam;
			HBRUSH hBrush = 0;
			
			//cautam caseta de editare hEdit in lista m_Lines
			deque<CListSide::LINE>::iterator I;

			if (pThis->m_Lines.size() && hEdit == pThis->m_Lines[0].edSecond.m_hWnd && !pThis->m_bIsLyrics)
			{
				//culoarea fundalului
				hBrush = CreateSolidBrush(RGB(151, 199, 234));
				//culoarea fundalului cand se scrie text
				SetBkColor(hDC, RGB(151, 199, 234));
				SetTextColor(hDC, 0);
			}

			else
			for (I = pThis->m_Lines.begin(); I != pThis->m_Lines.end(); I++)
			{
				//daca am gasit caseta de editare in lista
				if (hEdit == I->edSecond.m_hWnd || hEdit == I->edText.m_hWnd)
				{
					//daca caseta este "selectata"
					if (I->bSelected)
					{
						//culoarea fundalului
						hBrush = CreateSolidBrush(RGB(0, 0, 255));
						//culoarea fundalului cand se scrie text
						SetBkColor(hDC, RGB(0, 0, 255));
						SetTextColor(hDC, RGB(255, 255, 255));
						break;
					}
					else
					{
						//caseta de editare nu este "selectata"
						//culoarea fundalului
						hBrush = CreateSolidBrush(RGB(255, 255, 255));
						//culoarea fundalului cand se scrie text
						SetBkColor(hDC, RGB(255, 255, 255));
						SetTextColor(hDC, RGB(0, 0, 0));
						break;
					}
				}
				//daca nu am gasit caseta de editare in lista, continuam cautarea ca sa aflam daca este selectata sau nu
			}

			return (LRESULT)hBrush;
		}
		break;//WM_CTLCOLOREDIT
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void CListSide::AddLine()
{
	//se creeaza o noua linie (LINE)
	LINE line;
	line.bSelected = 0;

	RECT rect;
	int size = m_Lines.size();
	
	//daca este primul element din lista
	if (size == 0)
	{
		//prima data cream caseta de editare din stanga (in stanga barii verticale)
		rect.top = m_rClient.top;
		rect.bottom = m_rClient.top + m_nLineHeight;
		rect.left = -1;
		rect.right = m_nVBarPos;

		line.edSecond.Create(m_hWnd, rect, false, m_bIsLyrics);

		//acum cream caseta de editare din dreapta (in dreapta barii verticale)
		rect.left = m_nVBarPos;
		rect.right = m_rClient.right - 1;
		line.edText.Create(m_hWnd, rect, true, m_bIsLyrics);

		//index-ul va fi 0, caseta de editare "Text" va avea focus-ul
		line.nIndex = 0;
		SetFocus(line.edText.m_hWnd);
	}

	else
	{
		//nr de elemente din lista va fi indexul noii linii
		line.nIndex = size;
		//gasim extremitatiile ultimei casetei de editare text, in coordonate client parinte
		GetWindowRect(m_Lines.back().edText.m_hWnd, &rect);
		POINT pt = {rect.right, rect.bottom};
		ScreenToClient(m_hWnd, &pt);
		//asezam caseta sub ultima caseta din lista
		rect.top = pt.y - 1;
		rect.bottom = rect.top + m_nLineHeight;

		//in stanga barii verticale
		rect.left = -1;
		rect.right = m_nVBarPos;

		//se creeaza
		line.edSecond.Create(m_hWnd, rect, false, m_bIsLyrics);

		//acum caseta de editare "Secunda"/"Minut"
		rect.left = m_nVBarPos;
		rect.right = m_rClient.right - 1;
		line.edText.Create(m_hWnd, rect, true, m_bIsLyrics);
	}

	//daca partea de jos a liniei de tabel este sub pagina
	if (rect.bottom > m_rClient.bottom)
	{
		//actualizam bara de derulare (sa isi modifice marimea, sa poata fi folosita derularea)
		UpdateScrollBar(rect.bottom);
	}

	//adaugam in lista (deque) noua linie, care a fost creata
	m_Lines.push_back(line);
	//se redeseneaza controlul
	SendMessage(m_hWnd, WM_PAINT, 0, 0);
}

void CListSide::AddLineAt(int nLine)
{
	//se adauga o linie noua
	AddLine();
	//numarul de linii din lista
	int nrLines = m_Lines.size();
	//ultima linie(nrLines - 1) e goala - adineaori am adaugat-o. nrLines - 2 va fi trecuta in nrLines - 1, etc.
	for (int i = nrLines - 2; i>= nLine; i--)
	{
		//preluam handle la caseta de editare "Secunda"/"Minut"
		HWND hWnd = m_Lines[i].edSecond.m_hWnd;
		int len = GetWindowTextLengthW(hWnd);
		WCHAR* wstr = NULL;
		//daca hWnd contine text
		if (len)
		{
			len++;
			wstr = new WCHAR[len];
			//il stocam in wstr
			GetWindowTextW(hWnd, wstr, len);
			//iar acolo afisam "0:00" sau "0"
			if (m_bIsLyrics)
				SetWindowTextW(hWnd, L"0:00");
			else SetWindowTextW(hWnd, L"0");
			//ne deplasam pe linia de mai jos
			hWnd = m_Lines[i+1].edSecond.m_hWnd;
			//si scriem acolo textul ce era mai sus.
			SetWindowTextW(hWnd, wstr);
			delete[] wstr;
		}

		//preluam handle la caseta de editare "Text"
		hWnd = m_Lines[i].edText.m_hWnd;
		len = GetWindowTextLengthW(hWnd);
		if (len)
		{
			len++;
			wstr = new WCHAR[len];
			//il stocam in wstr
			GetWindowTextW(hWnd, wstr, len);
			//iar acolo afisam ""
			SetWindowTextW(hWnd, L"");
			//ne deplasam pe linia de mai jos
			hWnd = m_Lines[i+1].edText.m_hWnd;
			//si scriem acolo textul ce era mai sus.
			SetWindowTextW(hWnd, wstr);
			delete[] wstr;
		}
	}
}


void CListSide::RemoveLineAt(int nLine)
{
	//numarul de linii din lista
	int nrLines = m_Lines.size();
	//incepand cu linia nLine pana la sfarsit
	for (int i = nLine; i < nrLines; i++)
	{
		//preluam handle la caseta de editare "Secunda"/"Minut"
		HWND hWnd = m_Lines[i].edSecond.m_hWnd;
		int len = GetWindowTextLengthW(hWnd);
		WCHAR* wstr = NULL;
		//daca hWnd contine text
		if (len)
		{
			len++;
			wstr = new WCHAR[len];
			//il stocam in wstr
			GetWindowTextW(hWnd, wstr, len);
			//iar acolo afisam ""
			SetWindowTextW(hWnd, L"");
			//ne deplasam pe linia de mai sus
			hWnd = m_Lines[i-1].edSecond.m_hWnd;
			//si scriem acolo textul ce era mai jos.
			SetWindowTextW(hWnd, wstr);
			delete[] wstr;
		}

		//preluam handle la caseta de editare "Secunda"/"Minut"
		hWnd = m_Lines[i].edText.m_hWnd;
		len = GetWindowTextLengthW(hWnd);
		//daca hWnd contine text
		if (len)
		{
			len++;
			wstr = new WCHAR[len];
			//il stocam in wstr
			GetWindowTextW(hWnd, wstr, len);
			//iar acolo afisam ""
			SetWindowTextW(hWnd, L"");
			//ne deplasam pe linia de mai sus
			hWnd = m_Lines[i-1].edText.m_hWnd;
			//si scriem acolo textul ce era mai jos.
			SetWindowTextW(hWnd, wstr);
			delete[] wstr;
		}
	}

	//eliminam ultima linie
	RemoveLine();
}

void CListSide::RemoveLine()
{
	//se distrug casetele de editare
	DestroyWindow(m_Lines.back().edSecond.m_hWnd);
	DestroyWindow(m_Lines.back().edText.m_hWnd);
	//se elimina ultima linie din deque
	m_Lines.pop_back();

	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_POS;
	GetScrollInfo(m_hWnd, SB_VERT, &si);
	//daca continutul este derulat, derulam cu o linie mai sus
	if (si.nPos) Scroll(-m_nLineHeight);

	RECT rect;
	//preluam extremitatiile ultimei linii in coordonate client parinte
	GetWindowRect(m_Lines.back().edText.m_hWnd, &rect);
	POINT pt = {rect.right, rect.bottom};
	ScreenToClient(m_hWnd, &pt);

	//actualizam bara de derulare
	UpdateScrollBar(pt.y);
}

void CListSide::UpdateScrollBar(int nBottom)
{
	//nBottom - pozitia la care se deruleaza, relativ la pagina
	RECT client;
	GetClientRect(m_hWnd, &client);

	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_POS;
	GetScrollInfo(m_hWnd, SB_VERT, &si);

	si.fMask = SIF_RANGE | SIF_PAGE;
	//pozitia minima de derulare
	si.nMin = 0;
	si.nPage = client.bottom;
	//si.nMax = pozitia maxima de derulare = punctul cel mai de jos relativ la continut;
	//si.nPos = pozitia barii de derulare
	//nBottom = punctul cel mai de jos relativ la pagina (zona client)
	si.nMax = nBottom + si.nPos;
	m_dMaxScroll = si.nMax;
	//inaltimea maginii
	m_dPage = si.nPage;

	int add = si.nMax - client.bottom;
	//daca punctul cel mai de jos (relativ la intregul continut) este mai jos de pagina,
	//adica avem continut de derulat
	if (add > 0)
	{
		//se seteaza parametrii barii de derulare
		SetScrollInfo(m_hWnd, SB_VERT, &si, 1);
		//bara de derulare va putea fi folosita
		EnableScrollBar(m_hWnd, SB_VERT, ESB_ENABLE_BOTH);
		m_bScrollEnabled = true;
	}
	else
	{
		//bara de derulare nu va putea fi folosita
		EnableScrollBar(m_hWnd, SB_VERT, ESB_DISABLE_BOTH);
		m_bScrollEnabled = false;
	}
}

void CListSide::OnVScroll(WPARAM wParam)
{
	//daca nu se poate folosi bara de derulare, nu se deruleaza
	if (!m_bScrollEnabled) return;

	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
	GetScrollInfo(m_hWnd, SB_VERT, &si);

	switch (LOWORD(wParam))
	{
	case SB_BOTTOM: 
		{
			//se deruleaza la sfarsit, dar astfel incat ultima linie de tabel sa se vada ultima, intreaga pe pagina
			ScrollTo(si.nMax - si.nPage + 1);
		}
		break;
	case SB_TOP:
		{
			//se deruleaza la pozitia 0
			ScrollTo(0);
		}
		break;
	case SB_LINEDOWN: 
		{
			//se deruleaza cu o linie de tabel mai jos
			Scroll(m_nLineHeight);
		}
		break;
	case SB_LINEUP:	
		{
			//se deruleaza cu o linie de tabel mai sus
			Scroll(-m_nLineHeight);
		}
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		{
			//daca se trage de bara de editare, sau se apasa click pentru derulare, derulam acolo
			ScrollTo(HIWORD(wParam));
		}
		break;
	case SB_PAGEDOWN:
		{
			//se deruleaza cu o pagina mai jos
			Scroll(m_dPage);
		}
		break;
	case SB_PAGEUP:
		{
			//se deruleaza cu o pagina mai sus
			Scroll(-m_dPage);
		}
		break;
	}
}

void CListSide::ScrollTo(int pos)
{
	//daca nu se poate derula, iesim.
	if (false == m_bScrollEnabled) return;
	
	//daca pozitia de derulare (relativ la intreg continutul) incape dupa inceputul ultimei pagini
	//pozitia (pos) devine inceputul ultimei pagini (astfel ca ultima linie sa se vada si sa fie ultima in control)
	if (pos > m_dMaxScroll - m_dPage + 1)
		pos = m_dMaxScroll - m_dPage + 1;
	//trebuie de asemenea sa fie cel putin 0
	if (pos < 0) pos = 0;

	//scrollpos = vechea pozitie de derulare
	int add, scrollpos = GetScrollPos(m_hWnd, SB_VERT);
	
	//se deruleaza bara
	SetScrollPos(m_hWnd, SB_VERT, pos, 1);
	m_dScrollPos = pos;

	//acum, pentru derularea ferestrei
	//cu cat se deruleaza (delta)
	add = pos - scrollpos;
	RECT client;
	GetClientRect(m_hWnd, &client);

	//se deruleaza cu -add
	ScrollWindow(m_hWnd, 0, -add, NULL, &client);
}

void CListSide::Scroll(int nMore)
{
	if (false == m_bScrollEnabled) return;

	int add, scrollpos = GetScrollPos(m_hWnd, SB_VERT);

	int actual = scrollpos + nMore;
	//cazuri speciale:
	if (actual < 0) 
	{
		ScrollTo(0);
		return;
	}
	if (actual > m_dMaxScroll - m_dPage + 1)
	{
		ScrollTo(m_dMaxScroll - m_dPage + 1);
		return;
	}
	
	//dar, in cazuri obisnuite:
	//bara de derulare
	SetScrollPos(m_hWnd, SB_VERT, actual, 1);
	m_dScrollPos = actual;

	//se deruleaza fereastra
	add = nMore;
	RECT client;
	GetClientRect(m_hWnd, &client);

	ScrollWindow(m_hWnd, 0, -add, NULL, &client);
}