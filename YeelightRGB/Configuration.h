#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Bulb.h"

typedef struct
{
	bulb_Color_T color;
	LPTSTR name;
} conf_Preset_T;

void conf_Preset_Destroy (conf_Preset_T * preset);

typedef struct
{
	int line;
	int column;
} conf_FilePos_T;

typedef enum
{
	conf_RC_OK, conf_RC_IOERR, conf_RC_FORMERR
} conf_Result_Code_T;

typedef struct
{
	conf_Result_Code_T code;
	union
	{
		conf_FilePos_T lastFilePos;
		DWORD ioErr;
	} data;
} conf_Result_T;

typedef struct
{
	int port;
	int ipFields[4];
	conf_Preset_T * presets;
	int presetCount;
} conf_T;

 conf_Result_T conf_Load (LPCTSTR filename, conf_T * out);

 void conf_Destroy (conf_T * conf);

 void conf_Empty (conf_T * conf);