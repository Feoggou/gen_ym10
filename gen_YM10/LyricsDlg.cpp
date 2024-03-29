#include "LyricsDlg.h"
#include "Tools.h"

#define ID_TIMER			2014
#define IDC_WINAMP_STOP		40047

extern CGenDll genDll;

CLyricsDlg::CLyricsDlg(void)
{
	m_hbSet = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_NORMAL));
	m_hbSet2 = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_PUSHED));
	m_bIsSet = false;
	m_hDlg = 0;
	m_nFileNr = -1;
}

CLyricsDlg::~CLyricsDlg(void)
{
	DeleteObject(m_hbSet);
	DeleteObject(m_hbSet2);
}

INT_PTR CLyricsDlg::DoModal(HWND hParent, int nFileNr)
{
	if (genDll.m_hLyricsDlg) return IDCANCEL;
	m_nFileNr = nFileNr;

	INT_PTR nResult = DialogBoxParam(genDll.m_hInstance, MAKEINTRESOURCE(IDD_CREATE_LYRICS), hParent, &CLyricsDlg::DialogProc, (LPARAM)this);
	if (nResult == -1)
		DisplayError(0);
	
	return nResult;
}

INT_PTR CALLBACK CLyricsDlg::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//exista o singura caseta CLyricsDlg la un moment dat. deci, putem folosi o variabila statica pentru a
	//pastra pointer la obiectul apelant.
	static CLyricsDlg* pThis = NULL;

	if (pThis == NULL)
		if (uMsg == WM_INITDIALOG)
		{
			pThis = (CLyricsDlg*)lParam;
			pThis->m_hDlg = hDlg;
			SetWindowLongPtrW(hDlg, GWL_USERDATA, (LONG)pThis);
		}
		else return 0;

	switch (uMsg)
	{
	case WM_DESTROY: KillTimer(hDlg, ID_TIMER); pThis = 0; genDll.m_hLyricsDlg = 0; break;
	case WM_CLOSE: EndDialog(hDlg, IDCANCEL); break;
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK: pThis->OnOk(); break;
			case IDCANCEL: EndDialog(hDlg, IDCANCEL); break;
			case IDC_SET: pThis->OnSet(); break;
			}
		}
		break;//case WM_COMMAND
	case WM_INITDIALOG: pThis->OnInitDialog();break;

	}

	return 0;
}

void CLyricsDlg::OnOk()
{
	//trebuie sa fim siguri ca toate secundele au fost scrise corect.
	deque<CListSide::LINE>::iterator I, J;
	//presupunem ca nu e gresit nimic.
	bool bWrong = false;
	//cautam sa vedem daca secundele sunt trecute strict crescator sau nu
	for (I = m_List.m_Table.m_Lines.begin(); I != m_List.m_Table.m_Lines.end(); I++)
	{
		//trebuie sa nu fie ultimul
		if (I->nIndex != m_List.m_Table.m_Lines.back().nIndex)
		{
			J = I + 1;
			WCHAR wstr[7];
			//preluam textul
			GetWindowTextW(I->edSecond.m_hWnd, wstr, 7);
			wstring wsec = wstr;
			//textul va fi de forma m:ss
			int nrSec = _wtoi(wsec.substr(0, 1).c_str());
			//primul e minutul, deci, aflam in secunde
			nrSec *= 60;
			int valI = nrSec;
			//acum gasim secundele de la ss si adaugam
			nrSec = _wtoi(wsec.substr(2, 2).c_str());
			valI += nrSec;

			//verificam cu urmatorul
			//preluam textul
			GetWindowTextW(J->edSecond.m_hWnd, wstr, 7);
			wsec = wstr;
			//calculam totul in secunde
			nrSec = _wtoi(wsec.substr(0, 1).c_str());
			nrSec *= 60;
			int valJ = nrSec;
			nrSec = _wtoi(wsec.substr(2, 2).c_str());
			valJ += nrSec;
			//daca primul > al doilea e gresit (trebuie sa fie crescator)
			if (valI >= valJ)
			{
				bWrong = true;
				break;
			}
		}
	}

	//daca a fost scrisa o greseala
	if (bWrong)
	{
		MessageBoxW(m_hDlg, L"Toate secundele trebuiesc setate în ordine crescătoare, te rog setează o valoare corectă", L"Crearea versurilor", MB_ICONEXCLAMATION);
		//caseta de editare de pe linia cu greseala va primi focus-ul
		SetFocus(J->edText.m_hWnd);
		m_List.m_Table.SelectLine(J->nIndex);

		//se deruleaza la linia respectiva
		RECT rect;
		//gasim extremitatiile controlului static relativ la parintele lui
		GetWindowRect(J->edSecond.m_hWnd, &rect);
		POINT pt = {rect.left, rect.top};
		ScreenToClient(GetParent(J->edSecond.m_hWnd), &pt);
		//derulan la partea de sus a liniei de tabel
		m_List.m_Table.ScrollTo(pt.y);
		return;
	}

	//a doua verificare
	//acum presupunem ca este gresit. daca gasim ca am scris macar o linie de text e corect
	bWrong = true;
	for (I = m_List.m_Table.m_Lines.begin(); I != m_List.m_Table.m_Lines.end(); I++)
	{
		int len = GetWindowTextLength(I->edText.m_hWnd);
		if (len)
		{
			bWrong = false;
			break;
		}
	}

	if (bWrong)
	{
		MessageBoxW(m_hDlg, L"Trebuie sa scri text in macar o linie ca sa poti salva.", L"Crearea versurilor", MB_ICONEXCLAMATION);
		return;
	}

	//deschidem/cream fisierul lyrics. de asemenea, functia verifica daca am mai scris versuri pentru o melodie
	//cu acelasi nume de artist/melodie; de asemenea scrie in fisier numele artistului si melodiei.
	int nFileNr;
	HANDLE hFile = OpenLyricsFile(nFileNr);
	//daca nu s-a putut scrie/deschide fisierul, iesim.
	if (hFile == INVALID_HANDLE_VALUE) return;
	DWORD dwWritten;

	//luam fiecare vers (text) si il scriem in fisier.
	for (I = m_List.m_Table.m_Lines.begin(); I != m_List.m_Table.m_Lines.end(); I++)
	{
		GenListCtrl::VERSE verse;
		WCHAR* wstr = new WCHAR[7];
		GetWindowTextW(I->edSecond.m_hWnd, wstr, 7);

		//str e de forma m:ss
		wstring wsec = wstr;
		//transformam minutele in secunde
		int nrSec = _wtoi(wsec.substr(0, 1).c_str());
		nrSec *= 60;
		verse.nrSec = (WORD)nrSec;
		//adaugam secundele din ss
		nrSec = _wtoi(wsec.substr(2, 2).c_str());
		verse.nrSec += (WORD)nrSec;
		//scriem secunda
		WriteFile(hFile, &verse.nrSec, sizeof(WORD), &dwWritten, 0);
		delete[] wstr;

		//acum textul (lungimea, apoi textul insusi)
		byte len = (byte)GetWindowTextLengthW(I->edText.m_hWnd);
		len++;
		wstr = new WCHAR[len];
		GetWindowTextW(I->edText.m_hWnd, wstr, len);
		WriteFile(hFile, &len, 1, &dwWritten, 0);
		WriteFile(hFile, wstr, len * 2, &dwWritten, 0);//WCHARs
		delete[] wstr;
	}

	//nu mai avem nevoie de fisier.
	CloseHandle(hFile);

	//la sfarsit, adaugam aceste informatzii despre lyrics in lista
	LYRICSINFO lyrinfo;

	//artist
	int len = GetWindowTextLength(m_hDlg);
	len++;
	WCHAR* text = new WCHAR[len];
	GetDlgItemText(m_hDlg, IDC_ARTISTNAME, text, len);
	lyrinfo.wsArtist = text;
	delete[] text;

	//melodie
	len = GetWindowTextLength(m_hDlg);
	len++;
	text = new WCHAR[len];
	GetDlgItemText(m_hDlg, IDC_SONGNAME, text, len);

	lyrinfo.wsSong = text;
	delete[] text;

	lyrinfo.nFileNr = nFileNr; 

	//daca este un lyrics nou, se adauga in lista (altfel inseamna ca se editeaza = nu imi mai trebuie sa mai adaug)
	if (m_nFileNr == -1)
		genDll.m_LyricsFiles.push_back(lyrinfo);

	EndDialog(m_hDlg, IDOK);
}

void CLyricsDlg::OnInitDialog()
{
	genDll.m_hLyricsDlg = m_hDlg;
	//cream controlul GenListCtrl. specificam ca fa vi folosit pentru lyrics.
	m_List.Create(GetDlgItem(m_hDlg, IDC_LYRICS_LIST), true);

	//cream bara de butoane la o anumita pozitie
	RECT barRect;
	GetWindowRect(GetDlgItem(m_hDlg, IDOK), &barRect);
	POINT pt1 = {barRect.left, barRect.top}, pt2 = {barRect.right, barRect.bottom};
	ScreenToClient(m_hDlg, &pt1);
	ScreenToClient(m_hDlg, &pt2);

	int midY = (pt1.y + pt2.y)/2;
	GetWindowRect(GetDlgItem(m_hDlg, IDC_LYRICS_LIST), &barRect);
	pt1.x = barRect.left;
	ScreenToClient(m_hDlg, &pt1);

	//deci, 22*5 = 110. width = right - left + 1 => right = width + left - 1 => right = 110 + left - 1
	barRect.left = pt1.x;
	barRect.right = barRect.left + 109;

	//acum folosim midY. midY = 19/2 = 9 => 0->8 && 9->18
	barRect.top = midY - 9 + 1;
	barRect.bottom = midY + 10;

	m_ButBar.Create(m_hDlg, barRect);

	//idc_set: x1 = midX - size/2 + 1 (partea stanga a butonului "Seteaza")
	int x1 = (barRect.left + barRect.right)/2 - 11 + 1;// + 7;
	
	//acum mutam IDC_PROGRESS:
	GetWindowRect(GetDlgItem(m_hDlg, IDC_PROGRESS), &barRect);
	int width = barRect.right - barRect.left + 1;
	pt1.x = barRect.left; pt1.y = barRect.top;
	pt2.x = barRect.right; pt2.y =  barRect.bottom;
	ScreenToClient(m_hDlg, &pt2);//pastram top && bottom
	ScreenToClient(m_hDlg, &pt1);
	barRect.top = pt1.y;
	barRect.bottom = pt2.y;
	barRect.right = x1 - 5;
	//width = right - left + 1 => left = right - width + 1.
	barRect.left = barRect.right - width + 1;
	MoveWindow(GetDlgItem(m_hDlg, IDC_PROGRESS), barRect.left, barRect.top, barRect.right - barRect.left + 1, barRect.bottom - barRect.top + 1, 1);

	//setam timerul la 100 milisecunde
	SetTimer(m_hDlg, ID_TIMER, 100, OnTimer);

	//se creeaza hyperlink-urile
	m_LoadFromFile.Create(m_hDlg, IDHL_LOADFROMFILE, OnLoadFromFile);
	m_LoadFromWeb.Create(m_hDlg, IDHL_LOADFROMWEB, OnLoadFromWeb);

	//incarcam informatiile despre melodie si artist, si afisam in casetele de editare.
	UpdateFromWinamp(m_hDlg);
	
	if (m_nFileNr != -1)
	{
		wstring wsFileName = genDll.m_wsAppData;
		WCHAR wstr[15];
		_itow_s(m_nFileNr, wstr, 15, 10);
		wsFileName += '\\';
		wsFileName += wstr;
		wsFileName += L".lyr";

		HANDLE hFile = CreateFile(wsFileName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
		if (hFile == INVALID_HANDLE_VALUE) return;

		//ANTET:				'LYR'
		//						int len_artist
		//						wchar[] wsArtist
		//						int len_song
		//						wchar[] wsSong
		//						.............
		//CONTINUT:					sec (word)
		//							text len(byte)
		//							text (wchar_t*)

		SetFilePointer(hFile, 3, 0, FILE_BEGIN);
		//se citeste len_artist
		int len;
		DWORD dwRead;
		ReadFile(hFile, &len, sizeof(int), &dwRead, 0);
		//ne mutam dupa wsArtist, la len_song
		SetFilePointer(hFile, len * 2, 0, FILE_CURRENT);
		//citim len_song
		ReadFile(hFile, &len, sizeof(int), &dwRead, 0);
		//ne mutam dupa wsSong, adica la "sec"
		SetFilePointer(hFile, len * 2, 0, FILE_CURRENT);

		//se citesc datele
		WORD sec;
		GenListCtrl::VERSE verse;
		deque<GenListCtrl::VERSE> Verses;
		do
		{
			//de aici folosesc len ca BYTE, numai ca int.
			BYTE len;
			//intrucat nu se stie numarul de versuri, se citeste pana nu mai exista date
			if (ReadFile(hFile, &sec, 2, &dwRead, 0) != 0 && dwRead == 0)
				break;

			verse.nrSec = sec;
			ReadFile(hFile, &len, 1, &dwRead, 0);

			wchar_t* the_verse = new wchar_t[len];
			ReadFile(hFile, the_verse, len * 2, &dwRead, 0);
			verse.wsVerse = the_verse;
			delete[] the_verse;

			Verses.push_back(verse);
		}while (dwRead != 0);

		//adaugam linii in tabel, cu 1 mai putin decat cate sunt in Verses
		int nrLines = Verses.size();
		for (int i = 0; i < nrLines - 1; i++)
			m_List.m_Table.AddLine();

		//acum scriem valorile in fiecare caseta
		deque<GenListCtrl::VERSE>::iterator I;
		int i = 0;
		for (I = Verses.begin(); I != Verses.end(); I++, i++)
		{
			//trebuie sa desfacem nrSec in m:ss
			byte min = (byte)(I->nrSec / 60);
			byte sec = I->nrSec % 60;
			WCHAR wsSec[7];
			if (sec < 10)
				wsprintf(wsSec, L"%d:%0d", min, sec);
			else wsprintf(wsSec, L"%d:%d", min, sec);
			SetWindowText(m_List.m_Table.m_Lines[i].edSecond.m_hWnd, wsSec);

			//textul
			SetWindowText(m_List.m_Table.m_Lines[i].edText.m_hWnd, I->wsVerse.c_str());
		}

		CloseHandle(hFile);
	}
}

void CLyricsDlg::UpdateFromWinamp(HWND hDlg)
{
	//luam informatiile din winamp
	//spre exemplu
	//736. Artist - Melodie - Winamp
	
	//pana la primul punct + caracterul ' ' este irelevant (aici, '736.')
	//ultimul ' - Winamp' este irelevant
	//apoi urmeaza: "Artist - Melodie" sau "Melodie", dupa ce elimin " - Winamp" vad daca mai gasesc '-'

	//textul din winamp:
	wstring wsText;
	int len = GetWindowTextLength(genDll.m_hWinamp);
	wchar_t* text = new wchar_t[len+1];
	GetWindowText(genDll.m_hWinamp, text, len+1);
	wsText = text;
	delete[] text;
	
	//eliminam textul irelevant
	int x = wsText.find('.', 0);
	wsText.erase(0, x + 2);
	x = wsText.rfind('-');
	//daca nu este cantata nici o melodie in mod curent, nu va exista nici un '-' in sir.
	if (x == -1) return;
	wsText.erase(x-1, wsText.length()-1);

	//acum rezultatul este sub forma "xxxx - xxxx" sau in formatul "xxxx"
	//vedem daca mai gasim un caracter '-'.
	//daca gasesc, impart sirul in doua.

	x = wsText.find(L" - ", 0);
	if (x != -1)
	{
		wstring wsArtist, wsSong;
		wsArtist = wsText.substr(0, x);
		wsSong = wsText.substr(x + 3, wsText.length() - x - 3);
		
		SetDlgItemText(hDlg, IDC_ARTISTNAME, wsArtist.c_str());
		SetDlgItemText(hDlg, IDC_SONGNAME, wsSong.c_str());
	}
	else
	{
		//tot sirul reprezinta numele melodiei.
		SetDlgItemText(hDlg, IDC_SONGNAME, wsText.c_str());
	}
}


void CALLBACK CLyricsDlg::OnTimer(HWND hDlg, UINT uMsg, UINT_PTR nIDEvent, DWORD dwTime)
{
	UNREFERENCED_PARAMETER(dwTime);
	UNREFERENCED_PARAMETER(nIDEvent);
	UNREFERENCED_PARAMETER(uMsg);

	static int lastposition = -1;
	//pozitia in melodie
	int position = (int)SendMessage(genDll.m_hWinamp, WM_USER, 0, 105);
	wchar_t wstr[10];
	if (position != lastposition)
	{
		//se afiseaza sub forma m:ss
		lastposition = position;
		int mins = lastposition/60000;
		int secunde = (lastposition - mins * 60000)/1000;
		if (secunde < 10)
			wsprintf(wstr, L"%d:0%d", mins, secunde);
		else
			wsprintf(wstr, L"%d:%d", mins, secunde);

		//se afiseaza in controlul progress
		SetDlgItemText(hDlg, IDC_PROGRESS, wstr);

		UpdateFromWinamp(hDlg);
	}
}

void CLyricsDlg::SetList(HWND hDlg, wstring& wsText)
{
	deque<wstring> wstrList;
	wstring wsLine;
	int nrFound = 0;
	bool bPreaMult = false;

	//avem textul in string-ul sText.
	for (UINT i = 0; i< wsText.length(); i++)
	{
		//cand dam de un '\n', salvam sirul (ca sa obtinem o lista de siruri).
		if (wsText[i]!='\n') wsLine += wsText[i];
		else
		{
			//daca pana acum au fost 256 de linii, aceasta este linia 257
			if (nrFound>=256)
			{
				MessageBoxW(hDlg, L"Sunt mai mult de 256 de linii! Doar primele 256 vor fi luate în considerare.", L"Atenție!", MB_ICONEXCLAMATION);
				bPreaMult = true;
				break;
			}
			wstrList.push_back(wsLine);
			nrFound++;
			wsLine = L"";
		}
	}
	//daca nu o depasit 256, atunci adaugam si ultima linie gasita
	if (!bPreaMult)
		wstrList.push_back(wsLine);

	//adauga/elimina linii in controlul lista dupa cum e necesar
	CLyricsDlg* pThis = (CLyricsDlg*)GetWindowLongPtrW(hDlg, GWL_USERDATA);
	int add = (int)pThis->m_List.m_Table.m_Lines.size() - (int) wstrList.size();
	
	//sunt mai multe linii decat e nevoie
	while (add>0)
	{
		pThis->m_List.m_Table.RemoveLine();
		add--;
	}
	//sau sunt mai putzine linii decat e nevoie
	while (add<0)
	{
		pThis->m_List.m_Table.AddLine();
		add++;
	}

	//acum scriem in controlul lista
	int size = wstrList.size();
	for (int i = 0 ; i < size; i++)
	{
		HWND hEdit = pThis->m_List.m_Table.m_Lines[i].edText.m_hWnd;
		SetWindowTextW(hEdit, wstrList[i].c_str());
	}
}

void CLyricsDlg::OnLoadFromFile()
{
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	wchar_t wstrFile[260];

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = genDll.m_hLyricsDlg;
	ofn.lpstrFilter = L"Fișiere Text (*.txt)\0*.txt\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = wstrFile;
	ofn.lpstrFile[0] = 0;
	ofn.nMaxFile = sizeof(wstrFile);
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	//afisam caseta de dialog Open

	if (GetOpenFileName(&ofn)==TRUE)
	{
		HANDLE hFile = CreateFile(ofn.lpstrFile, GENERIC_READ, FILE_SHARE_READ, (LPSECURITY_ATTRIBUTES) NULL, 
OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE) NULL);

		byte xch[2], ch;
		wstring wstr;
		enum enc{ansi, utf16/*FF FE*/, utf16be/*FE FF*/};
		enc encoding;
		encoding = ansi;
		
		DWORD dwRead;
		if (0 == ReadFile(hFile, xch, 2, &dwRead, 0))
		{
			DisplayError(0);
			return;
		}

		if (xch[0]==0xFF && xch[1]==0xFE)
			encoding = utf16; //UTF-16
		else if (xch[0]==0xFE && xch[1]==0xFF)
			encoding = utf16be; //UTF-16 BE
		//daca nu ambele primele caractere sunt ascii (adica au valori intre  0x00 si 0x7F)
		else if (!( __isascii(xch[0]) &&  __isascii(xch[1])))
		{
			MessageBoxW(genDll.m_hLyricsDlg, L"Fișierul este codificat cu o codificare necunoscuta.", L"Problemă la deschiderea fișierului", MB_ICONEXCLAMATION);
			return;
		}

		//daca este ANSI atunci pozitionam la inceputul fisierului.
		if (encoding == ansi)
			SetFilePointer(hFile, 0, 0, FILE_BEGIN);
		DWORD dwSize = GetFileSize(hFile, 0);

		if (encoding != ansi) dwSize -= 2;

		//citim tot continutul in buffer.
		byte* buffer = new byte[dwSize];
		ReadFile(hFile, buffer, dwSize, &dwRead, 0);
		
		//acum, daca codificarea e UTF-16 (FF FE)
		if (encoding == utf16)
		{
			for (UINT i = 0; i < dwSize-1; i += 2)
			{
				xch[0] = buffer[i]; xch[1] = buffer[i+1];
				if (xch[0]!='\r') 
					wstr += MAKEWORD(xch[0], xch[1]);
			}
		}
		else if (encoding == utf16be) //UTF16-BE (FE FF)
		{	
			for (UINT i = 0; i < dwSize-1; i += 2)
			{
				xch[0] = buffer[i]; xch[1] = buffer[i+1];
				if (xch[1]!='\r') 
					wstr += MAKEWORD(xch[1], xch[0]);
			}
		}
		else
		{
			for (UINT i = 0; i < dwSize; i++)
			{
				ch = buffer[i];
				if (dwRead == 0) break;
				if (ch!='\r') wstr += ch;
			}
		}
		delete[] buffer;

		CloseHandle(hFile);
		//setez lista sub forma de lista de siruri si afisez.
		SetList(genDll.m_hLyricsDlg, wstr);
	}
}

void CLyricsDlg::OnLoadFromWeb()
{
	wchar_t wstr[100];
	//numele artistului
	GetDlgItemText(genDll.m_hLyricsDlg, IDC_ARTISTNAME, wstr, 99);
	wstring wsArtist = wstr;
	//numele melodiei
	GetDlgItemText(genDll.m_hLyricsDlg, IDC_SONGNAME, wstr, 99);
	wstring wsSong = wstr;

	if (wsArtist.length() == 0 && wsSong.length() == 0)
	{
		MessageBox(genDll.m_hLyricsDlg, L"Nici o melodie nu este cântată acum. Apasă butonul \"Play\" pentru o melodie din lista din Winamp, mai întâi.", L"Nu e pornită nici o melodie", MB_ICONINFORMATION);
		return;
	}

	//inlocuim toate ' ' cu '_', si literele mari cu litere mici
	for (UINT i = 0; i < wsArtist.length(); i++)
	{
		if (wsArtist[i] == ' ') wsArtist[i] = '_';
		wsArtist[i] = (WCHAR)tolower(wsArtist[i]);
	}

	wchar_t ch;
	if (wsArtist.length()) ch = '_';
	else ch = '+';

	for (UINT i = 0; i < wsSong.length(); i++)
	{
		if (wsSong[i] == ' ') wsSong[i] = ch;
		wsSong[i] = (WCHAR)tolower(wsSong[i]);
	}

	wstring display;

	if (wsArtist.length())
	{
		display = L"http://www.lyricsmode.com/lyrics/";
		display += wsArtist[0];
		display += '/';
		display += wsArtist;
		display += '/';
		display += wsSong;
		display += L".html";
	}
	else
	{
		display = L"http://www.lyricsmode.com/search.php?what=songs&s=";
		display += wsSong;
	}

	SHELLEXECUTEINFO sei;
	ZeroMemory(&sei,sizeof(SHELLEXECUTEINFO));
	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.lpVerb = L"open";
	sei.lpFile = display.data();	
	sei.nShow = SW_SHOWNORMAL;

	ShellExecuteEx(&sei);
}

void CLyricsDlg::OnSet()
{
	//a fost apasat butonul "Set"
	m_bIsSet = true;
	int line = m_List.m_Table.GetSelectedLine();
	if (line==-1) line = 0;

	char sProgress[7];
	//afisam pozitia din melodie din IDC_PROGRESS in controlul static Secunda
	GetDlgItemTextA(m_hDlg, IDC_PROGRESS, sProgress, 7);
	SetWindowTextA(m_List.m_Table.m_Lines[line].edSecond.m_hWnd, sProgress);

	//trecem la linia urmatoare, daca mai exista inca o linie dupa.
	if (line < (int)m_List.m_Table.m_Lines.size() - 1) m_List.m_Table.SelectLine(line + 1);
}

HANDLE CLyricsDlg::OpenLyricsFile(int& nFileNr)
{
	wstring wsPathFile = genDll.m_wsAppData;
	wstring wsPathFolder = wsPathFile;
	wsPathFolder += L"\\*.lyr";

	//numele de fisiere: 1.lyr, 2.lyr, ...

	//daca fisierul nu exista , il cream. dar trebuie sa ne asiguram ca lyrics-ul nu exista.
	//formatul fisierului:
	//ANTET:				'LYR'
	//						int len_artist
	//						wchar[] wsArtist
	//						int len_song
	//						wchar[] wsSong
	//						.............
	//CONTINUT:					sec (word)
	//							text len(byte)
	//							text (wchar_t*)

	//luam datele din caseta de dialog. trebuie specificat cel putin numele melodiei
	//numele melodiei

	int fileNr = 0;
	WCHAR* wsSong, *wsArtist;
	//in mod normal, adica daca se creeaza lyrics-uri, se preia textul din caseta de editare
	int len;
	if (m_nFileNr == -1)
	{
		HWND hEdit = GetDlgItem(m_hDlg, IDC_SONGNAME);
		len = GetWindowTextLength(hEdit);
		len++;
		wsSong = new WCHAR[len];
		GetWindowText(hEdit, wsSong, len);

		//numele artistului
		hEdit = GetDlgItem(m_hDlg, IDC_ARTISTNAME);
		len = GetWindowTextLength(hEdit);
		len++;
		wsArtist = new WCHAR[len];
		GetWindowText(hEdit, wsArtist, len);
	}
	else
	//altfel, se citeste din fisier.
	{
		//numele fisierului
		wchar_t file_name[12];
		_itow_s(m_nFileNr, file_name, 12, 10);
		wsPathFile += L"\\";
		wsPathFile += file_name;
		wsPathFile += L".lyr";

		HANDLE hThatFile = CreateFile(wsPathFile.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hThatFile == INVALID_HANDLE_VALUE)
		{
			DisplayError(0);
			return INVALID_HANDLE_VALUE;
		}

		//acum citim continutul
		char sLyr[3] = "";
		DWORD dwRead;
		ReadFile(hThatFile, sLyr, 3, &dwRead, 0);
		if (sLyr[0] != 'L' && sLyr[1] != 'Y' && sLyr[2] != 'R')
		{
			CloseHandle(hThatFile);
			return INVALID_HANDLE_VALUE;
		}

		//acum citim numele artistului
		ReadFile(hThatFile, &len, sizeof(int), &dwRead, 0);
		wsArtist = new WCHAR[len];
		ReadFile(hThatFile, wsArtist, len * 2, &dwRead, 0);

		//si al melodiei
		ReadFile(hThatFile, &len, sizeof(int), &dwRead, 0);
		wsSong = new WCHAR[len];
		ReadFile(hThatFile, wsSong, len * 2, &dwRead, 0);
		CloseHandle(hThatFile);
	}

	if (wcslen(wsSong) == 0)
	{
		MessageBox(m_hDlg, L"Trebuie să specifici numele melodiei în caseta \"Melodie:\". ", L"Salvare versuri", MB_ICONEXCLAMATION);
		return INVALID_HANDLE_VALUE;
	}

	WIN32_FIND_DATA f_data = {0};
	BOOL bResult =0;

	if (m_nFileNr != -1)
	{
		fileNr = m_nFileNr;
		goto scrie_in_fisier;
	}

	//se cauta fisiere de tip .lyr sa vedem daca a mai fost salvat vreun lyrics cu acelasi nume de artist si melodie
	HANDLE hSearch = FindFirstFile(wsPathFolder.data(), &f_data);
	while (hSearch != INVALID_HANDLE_VALUE)
	{
		//gasim numarul fisierului.
		wstring wsFileName = f_data.cFileName;
		wsFileName = wsFileName.erase(wsFileName.length() - 4, 4);
		fileNr = _wtoi(wsFileName.c_str());
		//ne asiguram ca e de forma <numar>.lyr; daca nu e, continuam cautarea
		if (fileNr == 0)
		{
			bResult = FindNextFile(hSearch, &f_data);
			if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
			continue;
		}

		//deschidem fisierul:
		wstring sfn = wsPathFile;
		sfn += '\\';
		sfn += f_data.cFileName;
		HANDLE hFile = CreateFile(sfn.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
		if (hFile == INVALID_HANDLE_VALUE) 
		{
			DisplayError(0);
			bResult = FindNextFile(hSearch, &f_data);
			if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
			continue;
		}

		//citim continutul
		char sLyr[3] = "";
		DWORD dwRead;
		ReadFile(hFile, sLyr, 3, &dwRead, 0);
		if (sLyr[0] != 'L' && sLyr[1] != 'Y' && sLyr[2] != 'R')
		{
			CloseHandle(hFile);
			bResult = FindNextFile(hSearch, &f_data);
			if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
			continue;
		}

		//acum citim numele artistului
		ReadFile(hFile, &len, sizeof(int), &dwRead, 0);
		WCHAR* wsfileArtist = new WCHAR[len];
		ReadFile(hFile, wsfileArtist, len * 2, &dwRead, 0);

		//si al melodiei
		ReadFile(hFile, &len, sizeof(int), &dwRead, 0);
		WCHAR* wsfileSong = new WCHAR[len];
		ReadFile(hFile, wsfileSong, len * 2, &dwRead, 0);

		//acum, cum verificam: mai intai, numele melodiei: ambele trebuie sa le aibe. cautam fara sensibilitate
		//la majuscule
		if (_wcsicmp(wsfileSong, wsSong) != 0)
		{
			delete[] wsfileArtist;
			delete[] wsfileSong;

			CloseHandle(hFile);
			bResult = FindNextFile(hSearch, &f_data);
			if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
			continue;
		}
		//daca numele melodiei este acelasi la amandou, artistul trebuie sa existe si sa difere.
		bool art_dif = false;
		if (wcslen(wsfileArtist) && wcslen(wsArtist)) art_dif = true;

		//daca numele artistului nu difera
		if (!art_dif)
		{
			delete[] wsfileArtist;
			delete[] wsfileSong;

			wstring mesaj = L"Un fișier de versuri cu același nume de melodie există deja.\n";
			if (wcslen(wsfileArtist)) {mesaj += L"Numele artistului (din fisier):\n"; mesaj += wsfileArtist; mesaj +='\n';}
			mesaj += L"Vrei să îl înlocuiești?";
			int nResult = MessageBoxW(m_hDlg, mesaj.data(), L"Versuri deja existente", MB_ICONINFORMATION | MB_YESNO);
			CloseHandle(hFile);
			if (nResult == IDYES)
			{
				fileNr--;
				break;
			}
			else 
			{
				delete[] wsArtist;
				delete[] wsSong;
				return INVALID_HANDLE_VALUE;
			}
		}

		delete[] wsfileArtist;
		delete[] wsfileSong;
		CloseHandle(hFile);

		bResult = FindNextFile(hSearch, &f_data);
		if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
	}
	FindClose(hSearch);

	//gasim un nume pentru fisier
	fileNr++;
	wchar_t file_name[12];
	_itow_s(fileNr, file_name, 12, 10);
	wsPathFile += L"\\";
	wsPathFile += file_name;
	wsPathFile += L".lyr";

scrie_in_fisier:
	nFileNr = fileNr;
	//cream fisierul
	//DACA EXISTA, SE TRUNCHIAZA
	HANDLE hNewFile = CreateFile(wsPathFile.c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	DWORD dwWritten;
	WriteFile(hNewFile, "LYR", 3, &dwWritten, 0);

	//se scrie lungimea si sirul wsArtist
	len = wcslen(wsArtist);
	len++;
	WriteFile(hNewFile, &len, sizeof(int), &dwWritten, 0);
	WriteFile(hNewFile, wsArtist, len * 2, &dwWritten, 0);

	//se scrie lungimea si sirul wsSong
	len = wcslen(wsSong);
	len++;
	WriteFile(hNewFile, &len, sizeof(int), &dwWritten, 0);
	WriteFile(hNewFile, wsSong, len * 2, &dwWritten, 0);


	return hNewFile;
}
