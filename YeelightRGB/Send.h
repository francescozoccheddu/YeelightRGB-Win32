#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

BOOL send_Data (const char * msg);

void send_Dispose (void);

BOOL send_Init (void);

void send_Set (const int * ipFields, int port, HWND hwnd, UINT msg);