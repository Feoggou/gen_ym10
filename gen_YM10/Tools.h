#pragma once

#ifndef TOOLS_H
#define TOOLS_H

#define FULLMSGERROR

#include <windows.h>

inline void DoError(DWORD dwError = 0)
{
	if (dwError == 0)
		dwError = GetLastError();
#if defined(_DEBUG) || defined(FULLMSGERROR)
	WCHAR sError[600], sdError[20];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, dwError, LANG_USER_DEFAULT, sError, 500, 0);
	wsprintf(sdError, L"(%#08x)", dwError);
	wcscat_s(sError, 600, sdError);
	MessageBox(0, sError, L"Eroare!", MB_ICONERROR);
#else
	WCHAR sError[100];
	wsprintf(sError, L"Eroare %#08x", dwError);
	MessageBox(0, sError, L"Eroare!", MB_ICONERROR);
#endif
}

#ifdef _DEBUG
#define DisplayError(dwError)			\
{										\
	DoError(dwError);					\
	DebugBreak();						\
}

#else
#define DisplayError(dwError)			\
{										\
	DoError(dwError);					\
}

#endif// _DEBUG


#endif//TOOLS_H