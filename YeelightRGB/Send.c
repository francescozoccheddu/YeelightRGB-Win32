#include "Send.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <strsafe.h>

#pragma comment (lib, "Ws2_32.lib")

#define BULB_ID "0"
#define MSG_TOGGLE "{\"id\":" BULB_ID ",\"method\":\"toggle\",\"params\":[]}\r\n"

SOCKET send_socket = INVALID_SOCKET;
HANDLE send_thread = NULL;

void (*send_callback)(send_Result_T) = NULL;
struct sockaddr_in send_addr;

send_Result_T send_Command (const char * _msg)
{
	SOCKET ConnectSocket = INVALID_SOCKET;
	int iResult;

	ConnectSocket = WSASocket (AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_NO_HANDLE_INHERIT);
	if (ConnectSocket == INVALID_SOCKET)
	{
		return FALSE;
	}

	iResult = WSAConnect (ConnectSocket, (const struct sockaddr *) &send_addr, sizeof (send_addr), NULL, NULL, NULL, NULL);
	if (iResult == SOCKET_ERROR)
	{
		closesocket (ConnectSocket);
		return FALSE;
	}

	if (ConnectSocket == INVALID_SOCKET)
	{
		return FALSE;
	}

	size_t len;
	StringCchLengthA (_msg, 1024, &len);
	iResult = send (ConnectSocket, _msg, len, 0);

	closesocket (ConnectSocket);

	return iResult == SOCKET_ERROR ? FALSE : TRUE;
}

DWORD WINAPI send_thread_proc (LPVOID _param)
{
	send_Result_T res = send_Command (MSG_TOGGLE);
	if (send_callback)
	{
		send_callback (res);
	}
	return 0;
}

BOOL send_Run ()
{
	if (!send_thread)
	{
		send_thread = CreateThread (NULL, 0, &send_thread_proc, NULL, 0, NULL);
	}
	else
	{
		if (WaitForSingleObject (send_thread, 0) == WAIT_OBJECT_0)
		{
			CloseHandle (send_thread);
			send_thread = CreateThread (NULL, 0, &send_thread_proc, NULL, 0, NULL);
		}
		else
		{
			return FALSE;
		}
	}

	if (!send_thread)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

BOOL send_Toggle (void)
{
	return send_Run ();
}

void send_Dispose (void)
{
	WSACleanup ();
}

BOOL send_Init (void)
{
	WSADATA wsaData;
	return WSAStartup (MAKEWORD (2, 2), &wsaData) == 0;
}

void send_Set (const int * _ipFields, int _port, void (*_callback)(send_Result_T))
{
	struct in_addr addr;
	addr.S_un.S_un_b.s_b1 = _ipFields[0];
	addr.S_un.S_un_b.s_b2 = _ipFields[1];
	addr.S_un.S_un_b.s_b3 = _ipFields[2];
	addr.S_un.S_un_b.s_b4 = _ipFields[3];
	send_addr.sin_family = AF_INET;
	send_addr.sin_addr = addr;
	send_addr.sin_port = htons (_port);
	send_callback = _callback;
}

