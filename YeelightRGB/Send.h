#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef enum
{
	send_R_OK, send_R_SOCKET_ERR, send_R_CONN_ERR, send_R_SEND_ERR
} send_Result_T;

BOOL send_Data (const char * msg);

void send_Dispose (void);

BOOL send_Init (void);

void send_Set (const int * ipFields, int port, HWND hwnd, UINT msg);