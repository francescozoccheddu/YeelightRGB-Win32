#pragma once

#include <Windows.h>

typedef struct
{
	LPCTSTR bulbId;
	const COLORREF * colors;
	int colCount;
} Configuration;

 loadConfiguration (LPCTSTR filename, Configuration * out);