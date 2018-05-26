#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

BOOL send_Toggle (void);

void send_Dispose (void);

BOOL send_Init (void);

void send_Set (const int * ipFields, int port);