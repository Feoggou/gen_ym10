#include "AboutDlg.h"
#include "HyperLink.h"
#include "GenDll.h"

#include "resource.h"

extern CGenDll			genDll;

CAboutDlg::CAboutDlg(void)
{
	m_hArrowCursor = m_hHandCursor = 0;
	m_hDlg = 0;
}

CAboutDlg::~CAboutDlg(void)
{
}

int CAboutDlg::DoModal(HWND hParent)
{
	int nResult = DialogBoxParam(genDll.m_hInstance, MAKEINTRESOURCE(IDD_ABOUTDLG), hParent, &CAboutDlg::DialogProc, (LPARAM)this);
	if (nResult == -1)
	{
		DWORD dwError = GetLastError();
		WCHAR sError[100];
		wsprintf(sError, L"Eroare %#08x", dwError);
		MessageBox(hParent, sError, L"Eroare!", MB_ICONERROR);
#ifdef _DEBUG
		DebugBreak();
#endif
	}

	return nResult;
}

INT_PTR CALLBACK CAboutDlg::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
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
	case WM_INITDIALOG: pThis->OnInitDialog();break;
	case WM_LBUTTONUP: pThis->OnLButtonUp(lParam); break;
	case WM_MOUSEMOVE: pThis->OnMouseMove(lParam); break;
	}

	return 0;
}

int CAboutDlg::OnInitDialog()
{
	//se seteaza hyperlink-ul
	m_hHandCursor = LoadCursor(0, IDC_HAND); 
	m_hArrowCursor = LoadCursor(0, IDC_ARROW);
//	pThis->m_bUnderline = false;

	SetClassLongPtrW(m_hDlg, GCL_HCURSOR, NULL);

	//se seteaza userdata pentru dialog
//	SetWindowLongPtrW(m_hDlg, GWL_USERDATA, (LONG)lParam);
	//se seteaza WindowProc pentru link-ul email
//	pThis->m_EmailLink.m_hWnd = GetDlgItem(hDlg, IDC_EMAIL);
///	if (0 == CHyperLink::OldWndProc)
//		CHyperLink::OldWndProc = (WNDPROC)SetWindowLongPtrW(GetDlgItem(hDlg, IDC_EMAIL), GWL_WNDPROC, (LONG)CHyperLink::WindowProc);

	return 0;
}


void CAboutDlg::OnLButtonUp(LPARAM lParam)
{
	//pozitzia cursorului
	int xPos = (int)LOWORD(lParam);
	int yPos = (int)HIWORD(lParam); 

	RECT rect;
	GetWindowRect(m_EmailLink.m_hWnd, &rect);
	POINT first = {rect.left, rect.top}, second = {rect.right, rect.bottom};

	ScreenToClient(m_hDlg, &first);
	ScreenToClient(m_hDlg, &second);
	rect = MAKERECT(first.x, first.y, second.x, second.y);

	//daca nu este in link, iesi
	if (!(xPos > rect.left && xPos < rect.right && yPos < rect.bottom && yPos > rect.top))
		return;

	SHELLEXECUTEINFO sei;
	ZeroMemory(&sei,sizeof(SHELLEXECUTEINFO));
	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.lpVerb = TEXT( "open" );
	sei.lpFile = L"http://compose.mail.yahoo.com/?To=fio_244@yahoo.com";	
	sei.nShow = SW_SHOWNORMAL;

	ShellExecuteEx(&sei);
}

void CAboutDlg::OnMouseMove(LPARAM lParam)
{
	int xPos = (int)LOWORD(lParam);
	int yPos = (int)HIWORD(lParam); 

	RECT rect;
	GetWindowRect(m_EmailLink.m_hWnd, &rect);
	POINT first = {rect.left, rect.top}, second = {rect.right, rect.bottom};

	ScreenToClient(m_hDlg, &first);
	ScreenToClient(m_hDlg, &second);
	rect = MAKERECT(first.x, first.y, second.x, second.y);

	//daca se afla in link, atunci cursorul va fi hand
	if ( xPos > first.x && xPos < second.x && yPos < second.y && yPos > first.y)
	{
		SetCursor(m_hHandCursor);
		m_EmailLink.m_bUnderline = true;
		InvalidateRect(m_hDlg, &rect, 1);
	}
	//altfel va fi arrow
	else 
	{
		SetCursor(m_hArrowCursor);
		m_EmailLink.m_bUnderline = false;
		InvalidateRect(m_hDlg, &rect, 1);
	}
}