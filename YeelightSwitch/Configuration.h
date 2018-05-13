#pragma once

#include <Windows.h>

typedef struct
{
	DWORD64 bulbId;
	int address[4];
	int port;
	const COLORREF * colors;
	int colCount;
} Configuration;

 loadConfiguration (LPCTSTR filename, Configuration * out);