#include "LyricsDlg.h"

#define ID_TIMER			2014
#define IDC_WINAMP_STOP		40047

extern CGenDll genDll;

HBITMAP CLyricsDlg::m_hbSet, CLyricsDlg::m_hbSet2;
CLyricsDlg::WINAMP_TAGV1 CLyricsDlg::m_tagInfo;
bool CLyricsDlg::m_bIsSet;

CLyricsDlg::CLyricsDlg(void)
{
	m_hbSet = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_NORMAL));
	m_hbSet2 = LoadBitmap(genDll.m_hInstance, MAKEINTRESOURCE(IDB_PUSHED));
	m_hArrowCursor = m_hHandCursor = 0;
	m_bClicked = m_bMoved = FALSE;
	m_bIsSet = false;
}

CLyricsDlg::~CLyricsDlg(void)
{
	DeleteObject(m_hbSet);
	DeleteObject(m_hbSet2);
}

int CLyricsDlg::DoModal(HWND hParent)
{
	int nResult = DialogBoxParam(genDll.m_hInstance, MAKEINTRESOURCE(IDD_CREATE_LYRICS), hParent, &CLyricsDlg::DialogProc, (LPARAM)this);
	if (nResult == -1)
	{
		DWORD dwError = GetLastError();
		WCHAR sError[100];
		wsprintf(sError, L"Error 0x%x", dwError);
		MessageBox(hParent, sError, L"Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
	}

	return nResult;

//	ShowWindow(m_hWnd, 1);
//	return IDOK;
}

INT_PTR CALLBACK CLyricsDlg::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY: KillTimer(hDlg, ID_TIMER); break;
	case WM_CLOSE: EndDialog(hDlg, IDCANCEL); break;
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK: OnOk(hDlg); break;
			case IDCANCEL: EndDialog(hDlg, IDCANCEL); break;
			case IDC_SET: OnSet(hDlg); break;
			}
		}
		break;//case WM_COMMAND
	case WM_INITDIALOG: OnInitDialog(hDlg, lParam);break;

	case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT* pDraw = (DRAWITEMSTRUCT*)lParam;
			HDC hMemDC = CreateCompatibleDC(pDraw->hDC);
			HBITMAP hOldBmp;
	
			if (pDraw->itemAction & ODA_SELECT || pDraw->itemAction & ODA_DRAWENTIRE)
			{
				if (pDraw->itemState & ODS_SELECTED)
					hOldBmp = (HBITMAP)SelectObject(hMemDC, m_hbSet2);
				else
					hOldBmp = (HBITMAP)SelectObject(hMemDC, m_hbSet);
			}
			else
				hOldBmp = (HBITMAP)SelectObject(hMemDC, m_hbSet);

			BitBlt(pDraw->hDC, 0, 0, 22, 19, hMemDC, 0, 0, SRCCOPY);
			SelectObject(hMemDC, hOldBmp);

			HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
			RECT rect;// = {0, 0, 21, 18};
			GetClientRect(pDraw->hwndItem, &rect);
			SetBkMode(pDraw->hDC, TRANSPARENT);
			DrawTextA(pDraw->hDC, "Setează", -1, &rect, DT_CENTER | DT_VCENTER | DT_EXTERNALLEADING | DT_SINGLELINE); 

			DeleteDC(hMemDC);
		}
		break;

	case WM_LBUTTONUP: OnLButtonUp(hDlg, wParam, lParam); break;
	case WM_MOUSEMOVE: OnMouseMove(hDlg, wParam, lParam); break;
	}

	return 0;
}

void CLyricsDlg::OnOk(HWND hDlg)
{
	CLyricsDlg* pThis = (CLyricsDlg*)GetWindowLongPtrA(hDlg, GWL_USERDATA);

	//we must check that all seconds were written correctly
	deque<CListSide::LINE>::iterator I, J;
	bool bWrong = false;
	for (I = pThis->m_List.m_Table.m_Lines.begin(); I != pThis->m_List.m_Table.m_Lines.end(); I++)
	{
		//must not be the last
		if (I->nIndex != pThis->m_List.m_Table.m_Lines.back().nIndex)
		{
			J = I + 1;
			char str[7];
			GetWindowTextA(I->edSecond.m_hWnd, str, 7);
			string sec = str;
			int nrSec = atoi(sec.substr(0, 1).c_str());
			nrSec *= 60;
			int valI = nrSec;
			nrSec = atoi(sec.substr(2, 2).c_str());
			valI += nrSec;

			GetWindowTextA(J->edSecond.m_hWnd, str, 7);
			sec = str;
			nrSec = atoi(sec.substr(0, 1).c_str());
			nrSec *= 60;
			int valJ = nrSec;
			nrSec = atoi(sec.substr(2, 2).c_str());
			valJ += nrSec;
			if (valI >= valJ)
			{
				bWrong = true;
				break;
			}
		}
	}

	bool bWrong2 = true;

	for (I = pThis->m_List.m_Table.m_Lines.begin(); I != pThis->m_List.m_Table.m_Lines.end(); I++)
	{
		//must not be the last
		int len = GetWindowTextLength(I->edText.m_hWnd);
		if (len)
		{
			bWrong2 = false;
			break;
		}
	}

	if (bWrong2) bWrong = true;

	if (bWrong)
	{
		MessageBoxW(hDlg, L"Toate secundele trebuiesc setate în ordine crescătoare, te rog setează o valoare corectă", L"Repetiție", MB_ICONEXCLAMATION);
		//focus & selection
		SetFocus(J->edSecond.m_hWnd);
		SendMessage(J->edSecond.m_hWnd, EM_SETSEL, 0, 5);
		//scroll
		RECT rect;
		GetWindowRect(J->edSecond.m_hWnd, &rect);
		POINT pt = {rect.left, rect.top};
		ScreenToClient(GetParent(J->edSecond.m_hWnd), &pt);
		pThis->m_List.m_Table.ScrollTo(pt.y);
		return;
	}

	HANDLE hFile = OpenLyricsFile(hDlg);
	if (hFile == INVALID_HANDLE_VALUE) return;
	DWORD dwWritten;

	for (I = pThis->m_List.m_Table.m_Lines.begin(); I != pThis->m_List.m_Table.m_Lines.end(); I++)
	{
		GenListCtrl::VERSE verse;
		char* str = new char[7];
		GetWindowTextA(I->edSecond.m_hWnd, str, 7);

		//str e de forma mm:ss
		string sec = str;
		int nrSec = atoi(sec.substr(0, 1).c_str());
		nrSec *= 60;
		verse.nrSec = nrSec;
		nrSec = atoi(sec.substr(2, 2).c_str());
		verse.nrSec += nrSec;
		WriteFile(hFile, &verse.nrSec, sizeof(WORD), &dwWritten, 0);
		delete[] str;

		byte len = (byte)GetWindowTextLengthA(I->edText.m_hWnd);
		len++;
		str = new char[len];
		GetWindowTextA(I->edText.m_hWnd, str, len);
		WriteFile(hFile, &len, 1, &dwWritten, 0);
		WriteFile(hFile, str, len, &dwWritten, 0);
		delete[] str;
	}

	CloseHandle(hFile);

	if (genDll.m_sSavingSong == genDll.m_sCurrentSong)
		SendMessage(genDll.m_hWinamp, WM_COMMAND, IDC_WINAMP_STOP, 0);

	//we now rewrite the specified .mp3 file
	hFile = CreateFile(genDll.m_sSavingSong.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		DWORD dwError = GetLastError();
		char str[30];
		sprintf_s(str, 30, "Error 0x%x", dwError);
		MessageBoxA(genDll.m_hWinamp, str, "Error!", MB_ICONERROR);
#ifdef _DEBUG
		DebugBreak();
#endif
		EndDialog(hDlg, IDCANCEL);
		return;
	}

	//now, we search for ID3v1
	SetFilePointer(hFile, -128, 0, FILE_END);

	DWORD dwRead;
	ReadFile(hFile, &m_tagInfo, 128, &dwRead, 0);
	if (m_tagInfo.cT == 'T' && m_tagInfo.cA == 'A' && m_tagInfo.cG == 'G')
	{
		//it means the file already has an ID3v1. ensure that we read all (should always happen)
		if (dwRead != 128)
		{
			DWORD dwError = GetLastError();
			char str[30];
			sprintf_s(str, 30, "Error 0x%x", dwError);
			MessageBoxA(hDlg, str, "Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
			CloseHandle(hFile);
			EndDialog(hDlg, IDCANCEL);
			return;
		}

		//reposition again and write the new structure:
		SetFilePointer(hFile, -128, 0, FILE_END);
	}
	//else reposition to the end of file:
	else
	{
		memset(&m_tagInfo, 0, sizeof(m_tagInfo));
		m_tagInfo.cT = 'T'; m_tagInfo.cA = 'A'; m_tagInfo.cG = 'G';
		m_tagInfo.dGenre = 255;

		LARGE_INTEGER liSize;
		GetFileSizeEx(hFile, &liSize);

		SetFilePointer(hFile, 128, 0, FILE_END);
		if (false == SetEndOfFile(hFile))
		{
			DWORD dwError = GetLastError();
			int x = 0;
			x++;
		}

		GetFileSizeEx(hFile, &liSize);


		SetFilePointer(hFile, -128, 0, FILE_END);
	}

	GetDlgItemTextA(hDlg, IDC_ARTISTNAME, m_tagInfo.sArtistName, 30);
	GetDlgItemTextA(hDlg, IDC_ALBUMNAME, m_tagInfo.sAlbumName, 30);
	GetDlgItemTextA(hDlg, IDC_SONGNAME, m_tagInfo.sSongName, 30);

	if (false == WriteFile(hFile, &m_tagInfo, 128, &dwRead, 0))
	{
		DWORD dwError = GetLastError();
		char str[30];
		sprintf_s(str, "Error 0x%x", dwError);
		MessageBoxA(hDlg, str, "Error!", MB_ICONERROR);
#ifdef _DEBUG
		DebugBreak();
#endif
		CloseHandle(hFile);
		EndDialog(hDlg, IDCANCEL);
		return;
	}
	CloseHandle(hFile);

	//and finally, we add this lyrics info in the list.
	LYRICSINFO lyrinfo;
	lyrinfo.sAlbum = m_tagInfo.sAlbumName;
	lyrinfo.sArtist = m_tagInfo.sArtistName;
	lyrinfo.sSong = m_tagInfo.sSongName;
	lyrinfo.wsFileName = genDll.m_sSavingSong;
	genDll.m_LyricsFiles.push_back(lyrinfo);

	EndDialog(hDlg, IDOK);
}

void CLyricsDlg::OnInitDialog(HWND hDlg, LPARAM lParam)
{
	CLyricsDlg* pThis = (CLyricsDlg*)lParam;
	SetWindowLongPtrA(hDlg, GWL_USERDATA, (LONG_PTR)lParam);

	pThis->m_List.Create(GetDlgItem(hDlg, IDC_LYRICS_LIST), true, true);

	RECT barRect;
	GetWindowRect(GetDlgItem(hDlg, IDOK), &barRect);
	POINT pt1 = {barRect.left, barRect.top}, pt2 = {barRect.right, barRect.bottom};
	ScreenToClient(hDlg, &pt1);
	ScreenToClient(hDlg, &pt2);

	int midY = (pt1.y + pt2.y)/2;
	GetWindowRect(GetDlgItem(hDlg, IDC_LYRICS_LIST), &barRect);
	pt1.x = barRect.left;
	ScreenToClient(hDlg, &pt1);

	//so, 22*5 = 110. width = right - left + 1 => right = width + left - 1 => right = 110 + left - 1
	barRect.left = pt1.x;
	barRect.right = barRect.left + 109;

	//we now use midY. midY = 19/2 = 9 => 0->8 && 9->18
	barRect.top = midY - 9 + 1;
	barRect.bottom = midY + 10;

	pThis->m_ButBar.Create(hDlg, barRect);

	//now we move IDC_SET and IDC_PROGRESS:
	//idc_set: x1 = midX - size/2 + 1
	int x1 = (barRect.left + barRect.right)/2 - 11 + 1;// + 7;
	GetWindowRect(GetDlgItem(hDlg, IDC_SET), &barRect);
	pt1.x = barRect.left; pt1.y = barRect.top;
	pt2.x = barRect.right; pt2.y = barRect.bottom;
	ScreenToClient(hDlg, &pt2);
	ScreenToClient(hDlg, &pt1);
	MoveWindow(GetDlgItem(hDlg, IDC_SET), x1, pt1.y, 22, 19, 1);
	//idc_progress:
	GetWindowRect(GetDlgItem(hDlg, IDC_PROGRESS), &barRect);
	int width = barRect.right - barRect.left + 1;
	pt1.x = barRect.left; pt1.y = barRect.top;
	pt2.x = barRect.right; pt2.y =  barRect.bottom;
	ScreenToClient(hDlg, &pt2);//we keep top && bottom
	ScreenToClient(hDlg, &pt1);
	barRect.top = pt1.y;
	barRect.bottom = pt2.y;
	barRect.right = x1 - 5;
	//width = right - left + 1 => left = right - width + 1.
	barRect.left = barRect.right - width + 1;
	MoveWindow(GetDlgItem(hDlg, IDC_PROGRESS), barRect.left, barRect.top, barRect.right - barRect.left + 1, barRect.bottom - barRect.top + 1, 1);

	SetTimer(hDlg, ID_TIMER, 100, OnTimer);

	//setting hyperlink
	pThis->m_hHandCursor = LoadCursor(0, MAKEINTRESOURCE(IDC_HAND)); 
	pThis->m_hArrowCursor = LoadCursor(0, MAKEINTRESOURCE(IDC_ARROW)); 
	pThis->m_bClicked = 0;
	pThis->m_bMoved = 0;
	SetClassLong(hDlg, GCL_HCURSOR, NULL);

	pThis->m_LoadFromFile.m_hWnd = GetDlgItem(hDlg, IDC_LOADFROMFILE);
	CHyperLink::OldWndProc = (WNDPROC)SetWindowLongPtrW(GetDlgItem(hDlg, IDC_LOADFROMFILE), GWL_WNDPROC, (LONG)CHyperLink::WindowProc);

	pThis->m_LoadFromWeb.m_hWnd = GetDlgItem(hDlg, IDC_LOADFROMWEB);
	CHyperLink::OldWndProc = (WNDPROC)SetWindowLongPtrW(GetDlgItem(hDlg, IDC_LOADFROMWEB), GWL_WNDPROC, (LONG)CHyperLink::WindowProc);

	SendMessage(GetDlgItem(hDlg, IDC_ARTISTNAME), EM_LIMITTEXT, 30, 0);
	SendMessage(GetDlgItem(hDlg, IDC_ALBUMNAME), EM_LIMITTEXT, 30, 0);
	SendMessage(GetDlgItem(hDlg, IDC_SONGNAME), EM_LIMITTEXT, 30, 0);

	UpdateByTagIDV1(hDlg);
}

void CLyricsDlg::UpdateByFileName(HWND hDlg, bool bArtist, bool bAlbum, bool bSong)
{
	//we take the info from winamp
	//i.e.
	//736. Kataklysm - Taking the World By Storm - Winamp
	//until first dot and a ' ' is irrelevant
	//the last ' - Winamp' is irrelevant
	//then it follows: Artist - Song / Artist - Album - Song. we count how many '-' there are.
	string sText;
	int len = GetWindowTextLengthA(genDll.m_hWinamp);
	char* text = new char[len+1];
	GetWindowTextA(genDll.m_hWinamp, text, len+1);
	sText = text;
	delete[] text;
	
	//removing irrelevant text
	int x = sText.find('.', 0);
	sText.erase(0, x + 2);
	x = sText.rfind('-');
	sText.erase(x-1, sText.length()-1);

	//now result is in the format "xxxx - xxxx - xxx" or in the format "xxxx - xxxx"
	//we count the '-'s:
	
	int nCount = 0;
	int xs[3]; int i = 0;
	for (int i = 0; i < sText.length(); i++)
	{
		if (sText[i] == '-')
		{
			if (nCount > 2) break;
			xs[nCount] = i;
			nCount++;
		}
	}
	xs[nCount] = sText.length();
	//we have i '-'s. now we get each string and tream them.

	string strs[3];int j = 0;
	for (int i = 0; i <= nCount; i++)
	{
		if (nCount == 0) break;
		strs[i] = sText.substr(j, xs[i] - j);
		j = xs[i]+1;
	}
	
	for (j = 0; j <= nCount; j++)
	{
		//ltrim string i
		for (i = 0; i < strs[j].length(); i++)
		{
			if (strs[j][i] ==' ') strs[j].erase(i, 1);
			else break;
		}

		//rtrim
		for (i = strs[j].length() - 1; i >=0; i--)
		{
			if (strs[j][i] ==' ') strs[j].erase(i, 1);
			else break;
		}
	}

	switch (nCount)
	{
	case 0:
		//no '-' found => all sText is SongName
		SetDlgItemTextA(hDlg, IDC_SONGNAME, sText.data());
		break;
	case 1:
		//one '-' found => two substrings, 1 and 2, Artist and Song
		SetDlgItemTextA(hDlg, IDC_ARTISTNAME, strs[0].data());
		SetDlgItemTextA(hDlg, IDC_SONGNAME, strs[1].data());
		break;
	case 2:
		//three '-' found => three substrings, 1 and 2 and 3, artist and album and song
		SetDlgItemTextA(hDlg, IDC_ARTISTNAME, strs[0].data());
		SetDlgItemTextA(hDlg, IDC_ALBUMNAME, strs[1].data());
		SetDlgItemTextA(hDlg, IDC_SONGNAME, strs[2].data());
		break;
	}
}

void CLyricsDlg::UpdateByTagIDV2(HWND hDlg, HANDLE hFile, bool bArtist, bool bAlbum, bool bSong)
{
	//we fill the structure. we will see what kind it is:
	WINAMP_TAGV2 tagInfo;
	OVERLAPPED overl;
	DWORD dwRead;
	memset(&overl, 0, sizeof(overl));
	overl.Offset = 0;
	overl.hEvent = 0;
	ReadFile(hFile, &tagInfo, 10, &dwRead, &overl);
	GetOverlappedResult(hFile, &overl, &dwRead, true);

	if (dwRead < 10)
	{
		DWORD dwError = GetLastError();
		char str[10];
		sprintf_s(str, 10, "Error 0x%x", dwError);
		MessageBoxA(hDlg, str, "Error!", MB_ICONERROR);
#ifdef _DEBUG
		DebugBreak();
#endif
		EndDialog(hDlg, IDCANCEL);
		return;
	}

	if (tagInfo.cI == 'I' && tagInfo.cD == 'D' && tagInfo.c3 == '3')
	{
		DWORD dwSize = (tagInfo.nSize[0] << 7) + (tagInfo.nSize[1] << 7) + (tagInfo.nSize[2] << 7) + tagInfo.nSize[3];
		byte* buffer = new byte[dwSize];
		overl.Offset = 10;
		ReadFile(hFile, buffer, dwSize, &dwRead, &overl);
		GetOverlappedResult(hFile, &overl, &dwRead, true);
		if (dwRead < dwSize)
		{
			DWORD dwError = GetLastError();
			char str[10];
			sprintf_s(str, 10, "Error 0x%x", dwError);
			MessageBoxA(hDlg, str, "Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
			EndDialog(hDlg, IDCANCEL);
			return;
		}

		if (tagInfo.nVer1 == 2)
		{
			FRAME20 frame;
			int care = 0;
			BYTE finished = 0;
			for (int i = 0; i < dwSize - 3; i++)
			{
				care = 0;
				if (buffer[i] == 'T' && buffer[i+1] == 'A' && buffer[i+2] == 'L' && bAlbum)
					care = 1;
				else if (buffer[i] == 'T' && buffer[i+1] == 'T' && buffer[i+2] == '2' && bSong)
					care = 2;
				else if (buffer[i] == 'T' && buffer[i+1] == 'P' && buffer[i+2] == '1' && bArtist)
					care = 3;
				else continue;

				frame.cName[0] = buffer[i];
				frame.cName[1] = buffer[i+1];
				frame.cName[2] = buffer[i+2];

				frame.size[0] = buffer[i+3]; frame.size[1] = buffer[i+4]; frame.size[2] = buffer[i+5];
				DWORD dwSize = (buffer[i+3] << 7) + (buffer[i+4] << 7) + buffer[i+5];

				frame.encoding = buffer[i+6];
				if (frame.encoding == 0)
				{
					frame.str = new char[dwSize];
					strcpy_s(frame.str, dwSize, (char*)(i+7+buffer));
					frame.str[dwSize-1] = 0;
				}
				else//unicode:
				{
					frame.wstr = new wchar_t[dwSize/2];
					frame.str = new char[dwSize/2];
					memcpy_s(frame.wstr, dwSize-1, (wchar_t*)(i+9+buffer), dwSize-1);
					for (int j = 0; j < dwSize/2; j++) {frame.str[j] = frame.wstr[j];}
					frame.str[dwSize/2-1] = 0;
				}

				if (care == 1)
				{
					TrimString(frame.str);
					SetDlgItemTextA(hDlg, IDC_ALBUMNAME, frame.str);
					finished |= 1;
					bAlbum = true;
				}
				else if (care == 2)
				{
					TrimString(frame.str);
					SetDlgItemTextA(hDlg, IDC_SONGNAME, frame.str);
					finished |= 2;
					bSong = true;
				}
				else if (care == 3)
				{
					TrimString(frame.str);
					SetDlgItemTextA(hDlg, IDC_ARTISTNAME, frame.str);
					finished |= 4;
					bArtist = true;
				}

				if (frame.str) delete[] frame.str;

				if (finished == 7) break;
			}
		}
		else if (tagInfo.nVer1 == 3 || tagInfo.nVer1 == 4)
		{
			FRAME34 frame;
			int care = 0;
			BYTE finished = 0;
			for (int i = 0; i < dwSize - 3; i++)
			{
				care = 0;
				if (buffer[i] == 'T' && buffer[i+1] == 'A' && buffer[i+2] == 'L' && buffer[i+3] == 'B' && bAlbum)
					care = 1;
				else if (buffer[i] == 'T' && buffer[i+1] == 'I' && buffer[i+2] == 'T' && buffer[i+3] == '2' && bSong)
					care = 2;
				else if (buffer[i] == 'T' && buffer[i+1] == 'P' && buffer[i+2] == 'E' && buffer[i+3] == '1' && bArtist)
					care = 3;
				else continue;
				
				frame.cName[0] = buffer[i];
				frame.cName[1] = buffer[i+1];
				frame.cName[2] = buffer[i+2];
				frame.cName[3] = buffer[i+3];

				frame.size[0] = buffer[i+4]; frame.size[1] = buffer[i+5]; frame.size[2] = buffer[i+6]; frame.size[3] = buffer[i+7];
				DWORD dwSize = (buffer[i+4] << 7) + (buffer[i+5] << 7) + (buffer[i+6] << 7) + buffer[i+7];
				
				frame.flags = MAKEWORD(buffer[i+8], buffer[i+9]);
				frame.encoding = buffer[i+10];
				if (frame.encoding == 0 || frame.encoding == 3)
				{
					frame.str = new char[dwSize];
					frame.str[dwSize-1] = 0;

					memcpy_s(frame.str, dwSize-1, (char*)(i+11+buffer), dwSize-1);
					for (int k = 0; k < dwSize - 1; k++)
						if (isascii(frame.str[k]) == false)
						{
							delete[] frame.str;
							frame.str = 0;
							break;
						}
//					strcpy_s(frame.str, dwSize + 1, (char*)(i+11+buffer));
				}
				else
				{
					frame.wstr = new wchar_t[dwSize/2];
					frame.wstr[dwSize/2-1] = 0;
					frame.str = new char[dwSize/2];
					memcpy_s(frame.wstr, dwSize-1, (wchar_t*)(i+13+buffer), dwSize-1);
					for (int j = 0; j < dwSize/2; j++) {frame.str[j] = frame.wstr[j];}
					frame.str[dwSize/2-1] = 0;
				}

				if (care == 1)
				{
					TrimString(frame.str);
					SetDlgItemTextA(hDlg, IDC_ALBUMNAME, frame.str);
					finished |= 1;
					bAlbum = true;
				}
				else if (care == 2)
				{
					TrimString(frame.str);
					SetDlgItemTextA(hDlg, IDC_SONGNAME, frame.str);
					finished |= 2;
					bSong = true;
				}
				else if (care == 3)
				{
					TrimString(frame.str);
					SetDlgItemTextA(hDlg, IDC_ARTISTNAME, frame.str);
					finished |= 4;
					bArtist = true;
				}

				if (frame.str) {delete[] frame.str; frame.str = 0;}
				if (frame.wstr) {delete[] frame.wstr; frame.str = 0;}

				if (finished == 7) break;
			}
		}

		delete[] buffer;
	}

	if (!bArtist || !bAlbum || !bSong)
		UpdateByFileName(hDlg, bArtist, bAlbum, bSong);
}

void CLyricsDlg::UpdateByTagIDV1(HWND hDlg)
{
	if (m_bIsSet)
	{
		if (GetWindowTextLengthA(GetDlgItem(hDlg, IDC_ARTISTNAME)) != 0 ||
			GetWindowTextLengthA(GetDlgItem(hDlg, IDC_ALBUMNAME)) != 0 ||
			GetWindowTextLengthA(GetDlgItem(hDlg, IDC_SONGNAME)) != 0)
		{
			return;
		}
	}

	genDll.m_sSavingSong = genDll.m_sCurrentSong;

	static WCHAR wstrlast[MAX_PATH] = L"";
	//now we load the tagInfo and display what we have
	
	if (wcslen(genDll.m_sCurrentSong) > 0 && 0 != wcscmp(wstrlast, genDll.m_sCurrentSong))
	{
		SetDlgItemTextA(hDlg, IDC_ARTISTNAME, "");
		SetDlgItemTextA(hDlg, IDC_ALBUMNAME, "");
		SetDlgItemTextA(hDlg, IDC_SONGNAME, "");

		bool bArtist = false;
		bool bAlbum = false;
		bool bSong = false;

		wcscpy_s(wstrlast, MAX_PATH, genDll.m_sCurrentSong);
		//load open the file for reading:
		HANDLE hFile = CreateFile(wstrlast, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED
, 0);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			char errs[20];
			DWORD dwError = GetLastError();
			sprintf_s(errs, 20, "Error 0x%x", dwError);
			MessageBoxA(hDlg, errs, "Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
			return;
		}

		//so now, we have the file opened... we must load the last part of it:
		LARGE_INTEGER liSize;
		GetFileSizeEx(hFile, &liSize);
		DWORD dwRead;
		OVERLAPPED overl;
		memset(&overl, 0, sizeof(overl));
		overl.Offset = (DWORD)liSize.LowPart - 128;
		overl.hEvent = 0;
		ReadFile(hFile, &m_tagInfo, 128, &dwRead, &overl);
		GetOverlappedResult(hFile, &overl, &dwRead, true);

		if (m_tagInfo.cT == 'T' && m_tagInfo.cA == 'A' && m_tagInfo.cG == 'G')
		{
			if (dwRead != 128)
			{
				DWORD dwError = GetLastError();
				wsprintf(wstrlast, L"Error 0x%x", dwError);
				MessageBox(hDlg, wstrlast, L"Error!", MB_ICONERROR);
#ifdef _DEBUG
				DebugBreak();
#endif
				EndDialog(hDlg, IDCANCEL);
				return;
			}
			m_tagInfo.sAlbumName[29] = m_tagInfo.sArtistName[29] = m_tagInfo.sSongName[29] = 0;
			TrimString(m_tagInfo.sAlbumName);
			TrimString(m_tagInfo.sArtistName);
			TrimString(m_tagInfo.sSongName);

			SetDlgItemTextA(hDlg, IDC_ARTISTNAME, m_tagInfo.sArtistName);
			SetDlgItemTextA(hDlg, IDC_ALBUMNAME, m_tagInfo.sAlbumName);
			SetDlgItemTextA(hDlg, IDC_SONGNAME, m_tagInfo.sSongName);

			if (strlen(m_tagInfo.sArtistName) > 0) bArtist = true;
			if (strlen(m_tagInfo.sAlbumName) > 0) bAlbum = true;
			if (strlen(m_tagInfo.sSongName) > 0) bSong = true;
		}

		
		if (!bArtist || !bAlbum || !bSong)
			UpdateByTagIDV2(hDlg, hFile, bArtist, bAlbum, bSong);

		CloseHandle(hFile);
	}
}

void CALLBACK CLyricsDlg::OnTimer(HWND hDlg, UINT uMsg, UINT_PTR nIDEvent, DWORD dwTime)
{
	static int lastposition = -1;
	int position = (int)SendMessage(genDll.m_hWinamp, WM_USER, 0, 105);
	char str[10];
	if (position != lastposition)
	{
		lastposition = position;
		int mins = lastposition/60000;
		int secs = (lastposition - mins * 60000)/1000;
		if (secs<10)
			sprintf_s(str, 10, "%d:0%d", mins, secs);
		else
			sprintf_s(str, 10, "%d:%d", mins, secs);

		SetDlgItemTextA(hDlg, IDC_PROGRESS, str);
		UpdateByTagIDV1(hDlg);
	}
}

void CLyricsDlg::OnLButtonUp(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	CLyricsDlg* pThis = (CLyricsDlg*)GetWindowLongPtrA(hDlg, GWL_USERDATA);

	int xPos = (int)LOWORD(lParam);
	int yPos = (int)HIWORD(lParam); 

	RECT rect;
	//load from file
	GetWindowRect(pThis->m_LoadFromFile.m_hWnd, &rect);
	POINT first = {rect.left, rect.top}, second = {rect.right, rect.bottom};

	ScreenToClient(hDlg, &first);
	ScreenToClient(hDlg, &second);
	rect = MAKERECT(first.x, first.y, second.x, second.y);

	if ( xPos > rect.left && xPos < rect.right && yPos < rect.bottom && yPos > rect.top)
		SetCursor(pThis->m_hHandCursor);
	else goto try_theother;

	pThis->m_bClicked = TRUE;
	InvalidateRect(hDlg, &rect, 1);
	LoadFromFile(hDlg);

try_theother:
	//load from web
	GetWindowRect(pThis->m_LoadFromWeb.m_hWnd, &rect);
	first.x = rect.left; first.y = rect.top;
	second.x = rect.right; second.y = rect.bottom;

	ScreenToClient(hDlg, &first);
	ScreenToClient(hDlg, &second);
	rect = MAKERECT(first.x, first.y, second.x, second.y);

	if ( xPos > rect.left && xPos < rect.right && yPos < rect.bottom && yPos > rect.top)
		SetCursor(pThis->m_hHandCursor);
	else return;

	pThis->m_bClicked = TRUE;
	InvalidateRect(hDlg, &rect, 1);
	LoadFromWeb(hDlg);
}

void CLyricsDlg::OnMouseMove(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	CLyricsDlg* pThis = (CLyricsDlg*)GetWindowLong(hDlg, GWL_USERDATA);

	int xPos = (int)LOWORD(lParam);
	int yPos = (int)HIWORD(lParam); 

	RECT rect;
	//load from file link
	GetWindowRect(pThis->m_LoadFromFile.m_hWnd, &rect);
	POINT first = {rect.left, rect.top}, second = {rect.right, rect.bottom};

	ScreenToClient(hDlg, &first);
	ScreenToClient(hDlg, &second);
	rect = MAKERECT(first.x, first.y, second.x, second.y);

	if ( xPos > first.x && xPos < second.x && yPos < second.y && yPos > first.y)
	{
		SetCursor(pThis->m_hHandCursor);
		if (pThis->m_bMoved == FALSE) InvalidateRect(hDlg, &rect, 1);
		pThis->m_bMoved = TRUE;
	}
	else 
	{
		GetWindowRect(pThis->m_LoadFromWeb.m_hWnd, &rect);
		POINT first = {rect.left, rect.top}, second = {rect.right, rect.bottom};

		ScreenToClient(hDlg, &first);
		ScreenToClient(hDlg, &second);
		rect = MAKERECT(first.x, first.y, second.x, second.y);

		if ( xPos > first.x && xPos < second.x && yPos < second.y && yPos > first.y)
		{
			SetCursor(pThis->m_hHandCursor);
			if (pThis->m_bMoved == FALSE) InvalidateRect(hDlg, &rect, 1);
			pThis->m_bMoved = TRUE;
		}
		else 
		{
			SetCursor(pThis->m_hArrowCursor);
			if (pThis->m_bMoved == TRUE) InvalidateRect(hDlg, &rect, 1);
			pThis->m_bMoved = FALSE;
		}
	}
}

void CLyricsDlg::SetList(HWND hDlg, string& sText)
{
	deque<string> strList;
	string sLine;
	int nrFound = 0;

	for (int i = 0; i< sText.length(); i++)
	{
		if (sText[i]!='\n') sLine += sText[i];
		else
		{
			if (nrFound>=254)
			{
				MessageBoxA(hDlg, "Sunt mai mult de 255 de linii! Doar primele 255 vor fi luate în considerare.", "Atenție!", MB_ICONEXCLAMATION);
				break;
			}
			strList.push_back(sLine);
			nrFound++;
			sLine = "";
		}
	}
	strList.push_back(sLine);
	CLyricsDlg* pThis = (CLyricsDlg*)GetWindowLongPtrA(hDlg, GWL_USERDATA);

	//add/substract lines in list ctrl as needed
	int add = pThis->m_List.m_Table.m_Lines.size() - (int) strList.size();
	
	//there are more lines than needed
	while (add>0)
	{
		pThis->m_List.m_Table.RemoveLine();
		add--;
	}
	//there are less lines than needed
	while (add<0)
	{
		pThis->m_List.m_Table.AddLine();
		add++;
	}

	//now we write onto the list ctrl:
	int size = strList.size();
	for (int i = 0 ; i < size; i++)
	{
		HWND hEdit = pThis->m_List.m_Table.m_Lines[i].edText.m_hWnd;
		SetWindowTextA(hEdit, strList[i].data());
	}
}

void CLyricsDlg::LoadFromFile(HWND hDlg)
{
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	wchar_t strFile[260];

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hDlg;
	ofn.lpstrFilter = L"Fișiere Text (*.txt)\0*.txt\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = strFile;
	ofn.lpstrFile[0] = 0;
	ofn.nMaxFile = sizeof(strFile);
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn)==TRUE)
	{
		HANDLE hFile = CreateFile(ofn.lpstrFile, GENERIC_READ, FILE_SHARE_READ, (LPSECURITY_ATTRIBUTES) NULL, 
OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE) NULL);

		byte xch[2], ch;
		string str;
		byte dUnicode = 0;//0 = ANSI, 1 = UTF-8, 2 = UTF-16(FF FE) 3 = UTF-16BE (FE FF)
		DWORD dwRead;
		if (0 == ReadFile(hFile, xch, 2, &dwRead, 0))
		{
			MessageBoxA(hDlg, "Error! The file could not be read!", "Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
			return;
		}

		if (xch[0]==0xFF && xch[1]==0xFE)
			dUnicode = 2;
		else if (xch[0]==0xFE && xch[1]==0xFF)
			dUnicode = 3;
		else if (!( __isascii(xch[0]) &&  __isascii(xch[1]) 
			|| iswascii(MAKEWORD(xch[0], xch[1]))))
		{
			if (MessageBoxW(hDlg, L"Fișierul ar putea fi afișat incorect datorită codării. Vrei să continui așa?", L"Problemă de codificare", MB_ICONEXCLAMATION | MB_YESNO) == IDNO)
				return;
		}

		if (!dUnicode)
			SetFilePointer(hFile, 0, 0, FILE_BEGIN);
		DWORD dwSize = GetFileSize(hFile, 0);
		byte* buffer = new byte[dwSize];
		ReadFile(hFile, buffer, dwSize, &dwRead, 0);
		if (dUnicode == 2)//FF FE
		{
//			while (ReadFile(hFile, xch, 2, &dwRead, 0) )
			for (int i = 0; i < dwSize-1; i += 2)
			{
				xch[0] = buffer[i]; xch[1] = buffer[i+1];
				if (xch[1]==0 && xch[0]!='\r') 
					str += xch[0];
			}
		}
		else if (dUnicode == 3)//FE FF
		{
			for (int i = 0; i < dwSize-1; i += 2)
			{
				xch[0] = buffer[i]; xch[1] = buffer[i+1];
				if (xch[0]==0 && xch[1]!='\r') 
					str += xch[1];
			}
		}
		else
		{
			for (int i = 0; i < dwSize; i++)
//			while (ReadFile(hFile, &ch, 1, &dwRead, 0))
			{
				ch = buffer[i];
				if (dwRead == 0) break;
				if (ch!='\r') str += ch;
			}
		}
		delete[] buffer;

		CloseHandle(hFile);
		SetList(hDlg, str);
	}
}

void CLyricsDlg::LoadFromWeb(HWND hDlg)
{
	if (wcslen(genDll.m_sCurrentSong) == 0)
	{
		MessageBoxA(hDlg, "Nici o melodie nu este cântată acum. Apasă butonul \"Play\" pentru o melodie din lista din Winamp, mai întâi.", "Nu e pornită nici o melodie", MB_ICONINFORMATION);
		return;
	}

	wchar_t wstr[100];
	GetDlgItemText(hDlg, IDC_ARTISTNAME, wstr, 99);
	wstring sArtist = wstr;
	if (sArtist.length() == 0) 
	{
		MessageBoxA(hDlg, "Numele artistului este necesar. Te rog scrie numele artistului în câmpul \"Artist:\" de deasupra.", "Lipsește numele artistului", MB_ICONINFORMATION);
		return;
	}

	GetDlgItemText(hDlg, IDC_SONGNAME, wstr, 99);
	wstring sSong = wstr;
	if (sSong.length() == 0) 
	{
		MessageBoxA(hDlg, "Numele melodiei este necesară. Te rog scrie numele artistului în câmpul \"Melodie:\" de deasupra.", "Lipsește numele artistului", MB_ICONINFORMATION);
		return;
	}

	//replace all ' ' with '_', and upcase to uppercase
	for (int i = 0; i < sArtist.length(); i++)
	{
		if (sArtist[i] == ' ') sArtist[i] = '_';
		sArtist[i] = tolower(sArtist[i]);
	}

	for (int i = 0; i < sSong.length(); i++)
	{
		if (sSong[i] == ' ') sSong[i] = '_';
		sSong[i] = tolower(sSong[i]);
	}

	wstring display(L"http://www.lyricsmode.com/lyrics/");
	display += sArtist[0];
	display += '/';
	display += sArtist;
	display += '/';
	display += sSong;
	display += L".html";

	SHELLEXECUTEINFO sei;
	ZeroMemory(&sei,sizeof(SHELLEXECUTEINFO));
	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.lpVerb = TEXT( "open" );
	sei.lpFile = display.data();	
	sei.nShow = SW_SHOWNORMAL;

	ShellExecuteEx(&sei);
}

void CLyricsDlg::OnSet(HWND hDlg)
{
	m_bIsSet = true;
	CLyricsDlg* pThis = (CLyricsDlg*)GetWindowLongPtrA(hDlg, GWL_USERDATA);
	int line = pThis->m_List.m_Table.GetSelectedLine();
	if (line==-1) line = 0;

	char sProgress[7];
	GetDlgItemTextA(hDlg, IDC_PROGRESS, sProgress, 7);
	SetWindowTextA(pThis->m_List.m_Table.m_Lines[line].edSecond.m_hWnd, sProgress);

	if (line < pThis->m_List.m_Table.m_Lines.size() - 1) pThis->m_List.m_Table.SelectLine(line + 1);
}

HANDLE CLyricsDlg::OpenLyricsFile(HWND hDlg)
{
	//first, we get a key to the current user:
	HKEY hKey;
	char strError[30];
	
	if (0 != RegOpenCurrentUser(KEY_WRITE | KEY_READ, &hKey))
	{
		sprintf_s(strError, 30, "Error 0x%x.", GetLastError());
		MessageBoxA(genDll.m_hWinamp, strError, "Fatal Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
		return INVALID_HANDLE_VALUE;
	}

	if (0 != RegOpenKeyEx(hKey, L"Volatile Environment", 0, KEY_READ, &hKey))
	{
		sprintf_s(strError, 30, "Error 0x%x.", GetLastError());
		MessageBoxA(genDll.m_hWinamp, strError, "Fatal Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
		return INVALID_HANDLE_VALUE;
	}

	DWORD dwType;
	DWORD dwSize;
	wchar_t str[MAX_PATH];
	if (ERROR_SUCCESS != RegQueryValueEx(hKey, L"APPDATA", 0, &dwType, (LPBYTE)str, &dwSize))
//	if (ERROR_SUCCESS != RegGeValue(hKey, 0, L"APPDATA", RRF_RT_REG_SZ, &dwType, str, &dwSize))
	{
		sprintf_s(strError, 30, "Error 0x%x.", GetLastError());
		MessageBoxA(genDll.m_hWinamp, strError, "Fatal Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
		return INVALID_HANDLE_VALUE;
	}

	RegCloseKey(hKey);

	//create or open the directory:
	wstring wsPath = str;
	wsPath += L"\\Winamp";
	CreateDirectory(wsPath.data(), 0);
	wsPath += L"\\Plugins";
	CreateDirectory(wsPath.data(), 0);
	wsPath += L"\\Feoggou App";
	CreateDirectory(wsPath.data(), 0);
	wstring wsPath2 = wsPath;
	wsPath += L"\\*.lyr";

	//filename: 1.lyr, 2.lyr, ...

	//if the file does not exist, we create it. but we must make sure that that lyric does not exist.
	//file format:
	//HEADER:				'LYR'
	//						artist[30]
	//						album[30]
	//						song[30]
	//						.............
	//BODY:					sec (word)
	//						text len(byte)
	//						text (char*)

	//Get the data from the dialog. at least "Song" must be specified.
	char sSong[30], sArtist[30], sAlbum[30];
	GetDlgItemTextA(hDlg, IDC_SONGNAME, sSong, 30);
	GetDlgItemTextA(hDlg, IDC_ARTISTNAME, sArtist, 30);
	GetDlgItemTextA(hDlg, IDC_ALBUMNAME, sAlbum, 30);

	if (strlen(sSong) == 0)
	{
		MessageBoxA(hDlg, "Trebuie să specifici numele melodiei în caseta \"Melodie:\". ", "Salvare versuri", MB_ICONEXCLAMATION);
		return INVALID_HANDLE_VALUE;
	}
	
	if (strlen(sArtist) == 0)
	{
		int nResult = MessageBoxA(hDlg, "Este o idee bună să specifici numele artistului, dacă îl știi. Vrei să te întorci să îl scrii?", "Lipsește numele artistului", MB_ICONEXCLAMATION | MB_YESNO);
		if (nResult == IDYES) return INVALID_HANDLE_VALUE;
	}

	if (strlen(sAlbum) == 0)
	{
		int nResult = MessageBoxA(hDlg, "Este o idee bună să specifici numele albumului, dacă îl știi. Vrei să te întorci să îl scrii?", "Lipsește numele albumului", MB_ICONEXCLAMATION | MB_YESNO);
		if (nResult == IDYES) return INVALID_HANDLE_VALUE;
	}

	WIN32_FIND_DATA f_data;
	BOOL bResult =0;
	memset(&f_data, 0, sizeof(f_data));
	int fileNr = 0;
	HANDLE hSearch = FindFirstFile(wsPath.data(), &f_data);
	while (hSearch != INVALID_HANDLE_VALUE)
	{
		//make sure it is <number>.lyr
		wstring wsFileName = f_data.cFileName;
		wsFileName = wsFileName.erase(wsFileName.length() - 4, 4);
		fileNr = _wtoi(wsFileName.c_str());
		if (fileNr == 0) {FindNextFile(hSearch, &f_data); continue;}
		//we open the file:
		wstring sfn = wsPath2;
		sfn += '\\';
		sfn += f_data.cFileName;
		HANDLE hFile = CreateFile(sfn.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
		if (hFile == INVALID_HANDLE_VALUE) 
		{
			sprintf_s(strError, 30, "Error 0x%x has occured.", GetLastError());
			MessageBoxA(genDll.m_hWinamp, strError, "Error!", MB_ICONERROR);
#ifdef _DEBUG
			DebugBreak();
#endif
			bResult = FindNextFile(hSearch, &f_data);
			if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
			continue;
		}

		//we read the content
		char song[30] = "", artist[30] = "", album[30] = "", sLyr[3] = "";
		DWORD dwRead;
		ReadFile(hFile, sLyr, 3, &dwRead, 0);
		if (sLyr[0] != 'L' && sLyr[1] != 'Y' && sLyr[2] != 'R')
		{
			CloseHandle(hFile);
			bResult = FindNextFile(hSearch, &f_data);
			if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
			continue;
		}

		ReadFile(hFile, artist, 30, &dwRead, 0);
		ReadFile(hFile, album, 30, &dwRead, 0);
		ReadFile(hFile, song, 30, &dwRead, 0);
		//now, how to check: first, song name: both must have it. check non-case sensitive.
		if (_stricmp(song, sSong) != 0)
		{
			CloseHandle(hFile);
			bResult = FindNextFile(hSearch, &f_data);
			if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
			continue;
		}
		//otherwise, at least album or artist must exist and differ.
		bool art_dif = false, alb_dif = false;
		if (strlen(artist) == 0 || strlen(sArtist) == 0) art_dif = true;
		if (strlen(album) == 0 || strlen(sAlbum) == 0) alb_dif = true;

		if (!art_dif && !alb_dif)
		{
			string mesaj = "Un fișier de versuri cu același nume de melodie există deja.\n";
			if (strlen(artist)) {mesaj += "Numele artistului: "; mesaj += artist; mesaj += '\n';}
			if (strlen(album)) {mesaj += "Numele albumului: "; mesaj += album; mesaj += '\n';}
			mesaj += "Vrei să îl înlocuiești?";
			int nResult = MessageBoxA(hDlg, mesaj.data(), "Versuri deja existente", MB_ICONINFORMATION | MB_YESNO);
			CloseHandle(hFile);
			if (nResult == IDYES)
			{
				fileNr--;
				break;
			}
			else return INVALID_HANDLE_VALUE;
		}
		CloseHandle(hFile);

		bResult = FindNextFile(hSearch, &f_data);
		if (bResult == 0 && GetLastError() == ERROR_NO_MORE_FILES) break;
	}
	FindClose(hSearch);

	fileNr++;
	wchar_t file_name[10];
	_itow_s(fileNr, file_name, 10, 10);
	wsPath2 += L"\\";
	wsPath2 += file_name;
	wsPath2 += L".lyr";

	//we create the file
	HANDLE hNewFile = CreateFile(wsPath2.c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	DWORD dwWritten;
	WriteFile(hNewFile, "LYR", 3, &dwWritten, 0);
	WriteFile(hNewFile, sArtist, 30, &dwWritten, 0);
	WriteFile(hNewFile, sAlbum, 30, &dwWritten, 0);
	WriteFile(hNewFile, sSong, 30, &dwWritten, 0);

	return hNewFile;
}

 void CLyricsDlg::TrimString(char* sir)
 {
	 string str = sir;
	 if (str.length() == 0) return;

	 //trim left:
	 int i = 0;
	 while (i < str.length())
	 {
		 if (str[i] == ' ') str.erase(i, 1);
		 else break;
	 };

	 i = str.length() - 1;
	 while (i < str.length())
	 {
		 if (str[i] == ' ') 
		 {
			 str.erase(i, 1);
			 i--;
		 }
		 else break;
	 };

	 strcpy_s(sir, str.length() + 1, str.c_str());
 }