#pragma once

#include <Windows.h>

typedef struct
{
	COLORREF color;
	LPTSTR name;
} conf_Preset_T;

void conf_Preset_Destroy (conf_Preset_T * preset);

typedef struct
{
	DWORD64 bulbId;
	int port;
	conf_Preset_T * presets;
	int presetCount;
} conf_T;

 BOOL conf_Load (LPCTSTR filename, conf_T * out);

 void conf_Destroy (conf_T * conf);