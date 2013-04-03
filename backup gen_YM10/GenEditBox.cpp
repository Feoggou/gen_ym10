#include "GenEditBox.h"

WNDPROC GenEditBox::OldWndProc;

GenEditBox::GenEditBox(void)
{
}

GenEditBox::~GenEditBox(void)
{
}

LRESULT CALLBACK GenEditBox::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return LRESULT();
}
