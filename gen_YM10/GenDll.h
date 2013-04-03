#pragma once

//nu mai da warning pentru enumerarile de alt tip decat int (la mine, byte).
#pragma warning(disable: 4480)

#ifndef GENDLL_H
#define GENDLL_H

#define WIN32_LEAN_AND_MEAN
#define NOCOMM

#ifdef NTDDI_VERSION
#undef NTDDI_VERSION
#endif

#ifdef WINVER
#undef WINVER
#endif

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif


#define NTDDI_VERSION NTDDI_WINXP
#define WINVER			0x0501
#define _WIN32_WINNT	0x0501

typedef unsigned char byte;

#define WM_TRAYMSG				WM_USER + 9991

#define GPPHDR_VER				0x10
#define SYSTRAY_ICON			1000
#define IDT_SEEKYMESS			2000

//a fost adaugat un status nou. trebuie actualizata lista cu noul status.
#define	WM_UPDATELIST			WM_USER + 9990
//mesaj trimis pentru actualizarea listei de ID-uri de messenger online
#define WM_UPDATEIDS			WM_USER + 9989

#include <windows.h>
#include <deque>

#include "resource.h"
#include "GenListCtrl.h"

using namespace std;

typedef struct {
	int version;
	char *description;
	int (*init)();
	void (*config)();
	void (*quit)();
	HWND hwndParent;
	HINSTANCE hDllInstance;
} winampGeneralPurposePlugin;

extern winampGeneralPurposePlugin *gen_plugins[256];
typedef winampGeneralPurposePlugin * (*winampGeneralPurposePluginGetter)();

//text SPECIAL
struct SPECIAL
{
	enum Type: BYTE{clock, repet, songdet, lyrics};
	//folosite in richeditctrl pentru a memora pozitia din status a SPECIAL-ului, a nu se modifica ca un
	//un text normal, etc.
	int min, max;
	//pozitia SPECIAL-ului in status, 0 - la inceput, 1 - dupa primul text normal
	BYTE pos;
	//tipul SPECIAL-ului, adica, ce reprezinta pElem
	Type type;
	//insusi SPECIAL-ul, e un pointer la un element de tip clock/repete/...
	void* pElem;
};

//structura unui STATUS
struct STATUS
{
	//lista textelor 'normale' (adica, statice, spre deosebire de SPECIAL-e).
	deque<wstring> Statuses;
	//lista textelor speciale
	deque<SPECIAL> Specials;
	//numele statusului, asa cum apare in caseta de dialog "Alege statusul"
	wstring wsText;
	//numarul fisierului pe care acest status il reprezinta.
	int dFileNr;
};

//detalii despre melodia care canta curent. artistul si melodia vor fi preluate din casetele de editare
//din LyricsDlg si sunt folosite
//DOAR pentru recunoasterea melodiei pentru care trebuie afisate versurile.
struct LYRICSINFO
{
	//numarul fisierului .lyr
	int nFileNr;
	//numele artistului
	wstring wsArtist;
	//numele melodiei
	wstring wsSong;
};

//un ID de messenger care este logat pe messenger
struct ID
{
	//numele ID-ului
	wstring		wsID;
	//fereastra principala a Y!M pe care este logat utilizatorul cu ID-ul wsID
	HWND		hWnd;
};

//datele care se incarca dintr-un fisier .lyr
struct LYRFILE
{
	//handle la fisierul .lyr
	HANDLE						hFile;
	//numele complet al fisierului .mp3
	wstring						wsFileName;
	//versurile melodiei
	deque<GenListCtrl::VERSE>	Verses;
};

//structura unui ID3v1
struct WINAMP_TAGV1
{
	//caracterele 'T', 'A', 'G'
	char cT, cA, cG;
	//numele melodiei
	char sSongName[30];
	//numele artistului
	char sArtistName[30];
	//numele albumului
	char sAlbumName[30];
	//anul
	BYTE dY, dE, dA, dR;
	//comentarii
	char sComment[30];
	//genul
	BYTE dGenre;

	WINAMP_TAGV1()
	{
		sSongName[0] = 0; sAlbumName[0] = 0; sArtistName[0] = 0;
	}
};

	//antetul unui ID3v2
	struct WINAMP_TAGV2
	{
		//caracterele 'I,'D','3'.
		char cI, cD, c3;
		//versiunile
		BYTE nVer1, nVer2;
		BYTE nFlags;
		BYTE nSize[4];
	};

	//structura unui frame 2.0
	struct FRAME20
	{
		//numele frame-ului
		char cName[3];
		//dimensiunea frame-ului, stocata in 3 bytes
		BYTE size[3];
		//codficarea textului folosita
		BYTE encoding;
		//variabile utilizate de mine pentru stocarea textului, fie ca char* fie ca wchar_t*
		char* str;
		wchar_t* wstr;
		//constructorul meu
		FRAME20()
		{
			str = 0;
			wstr = 0;
		}
	};

	//structura unui frame versiunile 3 si 4.
	struct FRAME34
	{
		//numele frame-ului
		char cName[4];
		//dimensiunea frame-ului, stocata in 4 bytes
		BYTE size[4];
		WORD flags;
		//codificarea textului
		BYTE encoding;
		//variabile utilizate de mine pentru stocarea textului, fie ca char* fie ca wchar_t*
		char* str;
		wchar_t* wstr;
		//constructorul meu
		FRAME34()
		{
			str = 0;
			wstr = 0;
		}
	};

class CGenDll
{
public:
	//handle la fereastra winamp-ului
	HWND						m_hWinamp;
	//handle la caseta de dialog de alegere a statusului
	HWND						m_hChooseDlg;
	//handle la caseta de dialog Despre...
	HWND						m_hAboutDlg;
	//handle la caseta de dialog de creere a versurilor unei melodii
	HWND						m_hLyricsDlg;
	//handle la caseta de dialog de editare a statusului
	HWND						m_hStatusEditorDlg;
	//handle la caseta de dialog de editare a lyrics-urilor
	HWND						m_hLyricsEditorDlg;

	//iconul plugin-ului
	HICON						m_hIcon;
	//handle la instanta Winamp-ului
	HINSTANCE					m_hInstance;
	//handle la cheia registrului Winamp (...\Software\Winamp).
	HKEY						m_hWinampKey;

	//lista de statusuri pentru yahoo messenger
	deque<STATUS>				m_StatusMsgs;
	//lista de fisiere lyrics (versuri pentru melodii)
	deque<LYRICSINFO>			m_LyricsFiles;
	//numele intreg - cu tot cu cale - al fisierului ce este cantat curent
	string						m_sCurrentSong;
	//calea catre folder-ul (utilizatorului curent):
	//USER\Volatile Environment\APPDATA\Winamp\Plugins\Samuel App
	wstring						m_wsAppData;
	//lista de id-uri online (cu tot cu handle la fereastra Y!M)
	deque<ID>					m_OnlineIDs;
	//id-ul selectat pentru modificarea statusului (este salvat in registrii la iesirea din Winamp)
	ID							m_ChosenID;

	//indexul statusului selectat (ultimul status folosit)
	int							m_dStatusSelected;
	//specifica daca plugin-ul se va folosi pentru modificarea statusului la Y!M
	bool						m_bUseTool;
	//specifica daca se va afisa icon-ul in system tray sau nu.
	bool						m_bShowTray;

	//specifica daca id-ul selectat pentru schimbare statusului este curent online
	bool						m_bSelOnline;
	//timpul curent. porneste cand este pornit statusul.
	int							m_nTime;
	wstring						m_wsDisplayedStatus;
	//datele despre fisierul lyrics al melodiei ce este cantata curent
	LYRFILE						m_CurrentLyrFile;

private:
	//vechea functie de prelucrare a mesajelor pentru fereastra principala Winamp.
	WNDPROC			m_OldWinampProc;
	//handle la procedura hook, MessageProc
	static HHOOK	m_MsgHook;

	//daca se foloseste SongInfo si a fost bifata caseta 'Link la versuri', textul acesta este link-ul,
	//altfel e "".
	wstring			m_wsLinkLyrics;

public:
	//functie apelata pentru initializarea unu plug-in
	static int InitPlugin(void);
	//functie apelata cand se apasa butonul "Configure selected plug-in" pentru acest plug-in
	static void ConfigPlugin(void);
	//functie apelata pentru finalizare
	static void QuitPlugin(void);
	//functia HOOK pentru monitorizarea mesajelor generate car rezultat a unui eveniment input,
	//aici, intr-o caseta de dialog.
	static LRESULT CALLBACK MessageProc(int nCode, WPARAM wParam, LPARAM lParam);

	//plugin-ul
	winampGeneralPurposePlugin m_Plugin;

public:
	//constructorul
	CGenDll(void);
	//destructorul
	~CGenDll(void);

	//pentru crearea icon-ului in system tray
	void CreateTrayIcon(void);
	//functia pentru distrugerea icon-ului din system tray
	void DestroyTrayIcon(void);

	//functia pentru prelucrarea timer-ului, unde se cauta ferestrele Y!M, id-urile online, se afiseaza
	//un nou status, etc.
	static void CALLBACK OnTimer(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	//noua functie de prelucrare a mesajelor pentru fereastra principala Winamp.
	static LRESULT CALLBACK NewWinampProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	BYTE GetValueUseTool()
	{
		return m_bUseTool;
	}
	BYTE GetValueShowTray()
	{
		return m_bShowTray;
	}

private:
	//functie apelata la initializare, pentru initializarea datelor din registrii
	void SetupRegistry(void);
	//functie pentru incarcarea statusurilor pt Y!M din fisierele .stt
	void LoadStatusMessages();
	//functie apelata pentru incarcarea informatiilor despre statusuri din fisierele .lyr
	void LoadLyricsInfo();
	//functie folosita pentru a citi informatiile dintr-un fisier .stt si a le stoca intr-o structura STATUS
	bool ReadStatusFile(HANDLE hFile, STATUS& Status);
	//Salvarea setarilor in registrii
	void FinalUpdateRegistry();
	//se transforma statusul dintr-o structura intr-un wstring
	void CompileStatusText(wstring& status);
	//functie apelata pentru afisarea unui nou status pentru ID-ul de messenger selectat
	void DisplayNewStatusText();
	//se transforma SPECIAL-ul intr-un wstring
	void CompileElement(SPECIAL& special, wstring& result);
	//se gaseste cat s-a parcurs din melodia cantata pe winamp
	void GetCurrentSongProgress(wstring& wsProgress, bool bUsePercent);
	//se gaseste lungimea (durata) melodiei
	void GetCurrentSongLength(wstring& wsLen);
	//se gaseste numele Artistului (daca exista) si al melodiei, din ce este scris in Winamp
	void FindSIFromWinamp(wstring& wsArtist, wstring& wsSong);
	//gaseste numele Albumului, daca exista
	void FindAlbumName(wstring& wsAlbum);
	//se gaseste numele artistului si melodiei, din ID3v2
	void FindByTagID3v2(HANDLE hFile, wstring& wsAlbum);
	//se gaseste numele artistului si melodiei din textul afisat in winamp.
	void FindByFileName(wstring& wsArtist, wstring& wsSong);
	//se gaseste numele artistului si melodiei, din ID3v1
	void FindByTagID3v1(HANDLE hFile, wstring& wsAlbum);
	//construieste m_wsLinkLyrics din wsArtist si wsSong
	void BuildLinkLyrics(wstring wsArtist, wstring wsSong);
};

#endif//GENDLL_H