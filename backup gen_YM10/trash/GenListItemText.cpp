#include "GenListItemText.h"

extern CGenDll	genDll;
WNDPROC GenListItemText::OldWndProc;

GenListItemText::GenListItemText(void)
{
}

GenListItemText::~GenListItemText(void)
{
}

void GenListItemText::Create(HWND hParent, RECT &rect)
{
}

LRESULT CALLBACK GenListItemText::WindowProc(HWND hEdit, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return CallWindowProc(OldWndProc, hEdit, uMsg, wParam, lParam);
}