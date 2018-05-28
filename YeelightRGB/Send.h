#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef enum
{
	OK, SOCKET_ERR, CONN_ERR, SEND_ERR
} send_Result_T;

BOOL send_Toggle (void);

BOOL send_Color (COLORREF color);

void send_Dispose (void);

BOOL send_Init (void);

void send_Set (const int * ipFields, int port, HWND hwnd, UINT msg);