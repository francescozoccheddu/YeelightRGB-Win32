#include "Send.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define BULB_ID "0"
#define MSG_TOGGLE "{\"id\":" BULB_ID ",\"method\":\"toggle\",\"params\":[]}\r\n"

SOCKET send_socket = INVALID_SOCKET;
HANDLE send_thread = NULL;

struct sockaddr_in send_addr;

BOOL send_Command (const char * _msg)
{
	SOCKET ConnectSocket = INVALID_SOCKET;
	int iResult;

	ConnectSocket = WSASocket (AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_NO_HANDLE_INHERIT);
	if (ConnectSocket == INVALID_SOCKET)
	{
		return FALSE;
	}

	iResult = WSAConnect (ConnectSocket, &send_addr, sizeof(send_addr), NULL, NULL, NULL, NULL);
	if (iResult == SOCKET_ERROR)
	{
		closesocket (ConnectSocket);
		return FALSE;
	}

	if (ConnectSocket == INVALID_SOCKET)
	{
		return FALSE;
	}

	iResult = send (ConnectSocket, _msg, (int)strlen (_msg), 0);

	closesocket (ConnectSocket);

	return iResult == SOCKET_ERROR ? FALSE : TRUE;
}

DWORD WINAPI send_thread_proc (LPVOID _param)
{
	return send_Command (MSG_TOGGLE);
}

HANDLE send_Run ()
{
	DWORD threadId;
	CreateThread (NULL, 0, &send_thread_proc, NULL, 0, &threadId);
}

BOOL send_Toggle ()
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
	}

	if (!send_thread)
	{
		// thread creation failed
		return FALSE;
	}
	else
	{
		return TRUE;
	}

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

void send_Set (const int * _ipFields, int _port)
{
	struct in_addr addr;
	addr.S_un.S_un_b.s_b1 = _ipFields[0];
	addr.S_un.S_un_b.s_b2 = _ipFields[1];
	addr.S_un.S_un_b.s_b3 = _ipFields[2];
	addr.S_un.S_un_b.s_b4 = _ipFields[3];
	send_addr.sin_family = AF_INET;
	send_addr.sin_addr = addr;
	send_addr.sin_port = htons(_port);
}

