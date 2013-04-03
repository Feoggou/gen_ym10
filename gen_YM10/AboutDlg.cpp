#include "AboutDlg.h"
#include "HyperLink.h"
#include "GenDll.h"
#include "Tools.h"

#include "resource.h"

extern CGenDll			genDll;



CAboutDlg::CAboutDlg(void)
{
	m_hDlg = 0;
}

CAboutDlg::~CAboutDlg(void)
{
}

void CAboutDlg::DoModal(HWND hParent)
{
	if (genDll.m_hAboutDlg) return;

	INT_PTR nResult = DialogBoxParam(genDll.m_hInstance, MAKEINTRESOURCE(IDD_ABOUTDLG), hParent, &CAboutDlg::DialogProc, (LPARAM)this);
	if (nResult == -1)
		DisplayError(0);
}

INT_PTR CALLBACK CAboutDlg::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//intrucat va fi o singura caseta de dialog afisata in acelasi timp, pastram un pointer la obiectul
	//care a creat caseta de dialog
	static CAboutDlg* pThis = NULL;

	if (pThis == NULL)
		if (uMsg == WM_INITDIALOG)
		{
			pThis = (CAboutDlg*)lParam;
			pThis->m_hDlg = hDlg;
		}
		else return 0;

	switch (uMsg)
	{
	case WM_DESTROY: pThis = 0; genDll.m_hAboutDlg = 0; break;
	case WM_CLOSE: EndDialog(hDlg, IDCANCEL); break;
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK: EndDialog(hDlg, IDOK); break;
			case IDCANCEL: EndDialog(hDlg, IDCANCEL); break;
			}
		}
		break;//case WM_COMMAND

	case WM_INITDIALOG: return pThis->OnInitDialog();
	}

	return 0;
}

void CAboutDlg::OnEmail()
{
	//se deschide browser-ul curent pe site-ul yahoo! pentru trimiterea unui email.
	SHELLEXECUTEINFO sei;
	ZeroMemory(&sei,sizeof(SHELLEXECUTEINFO));
	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.lpVerb = TEXT( "open" );
	sei.lpFile = L"http://compose.mail.yahoo.com/?To=fio_244@yahoo.com";	
	sei.nShow = SW_SHOWNORMAL;

	ShellExecuteEx(&sei);
}

int CAboutDlg::OnInitDialog()
{
	genDll.m_hAboutDlg = m_hDlg;
	//se creeaza hyperlink-ul
	m_EmailLink.Create(m_hDlg, IDHL_EMAIL, OnEmail);
	return 1;
}