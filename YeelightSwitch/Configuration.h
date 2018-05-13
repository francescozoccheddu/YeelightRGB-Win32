#pragma once

#include <Windows.h>

typedef struct
{
	COLORREF color;
	LPCTSTR name;
} Preset;

typedef struct
{
	DWORD64 bulbId;
	int address[4];
	int port;
	const Preset * presets;
	int presetCount;
} Configuration;

 BOOL loadConfiguration (LPCTSTR filename, Configuration * out);

 void destroyConfiguration (Configuration * conf);