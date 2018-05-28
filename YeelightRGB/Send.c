#include "Send.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <strsafe.h>

#pragma comment (lib, "Ws2_32.lib")

HANDLE send_thread = NULL;

struct sockaddr_in send_addr;

HWND send_hwnd = NULL;
UINT send_msg;

int send_Run (char * _data)
{
	SOCKET ConnectSocket = INVALID_SOCKET;

	ConnectSocket = WSASocket (AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_NO_HANDLE_INHERIT);
	if (ConnectSocket == INVALID_SOCKET)
	{
		return WSAGetLastError();
	}

	if (WSAConnect (ConnectSocket, (const struct sockaddr *) &send_addr, sizeof (send_addr), NULL, NULL, NULL, NULL))
	{
		int error = WSAGetLastError ();
		closesocket (ConnectSocket);
		return error;
	}

	shutdown (ConnectSocket, SD_RECEIVE);
	
	size_t chLen;
	StringCchLengthA (_data, STRSAFE_MAX_CCH, &chLen);

	WSABUF dataBuf;
	dataBuf.buf = _data;
	dataBuf.len = (ULONG) (chLen * sizeof (char));

	int error = 0;

	DWORD sent;
	if (WSASend (ConnectSocket, &dataBuf, 1, &sent, 0, NULL, NULL))
	{
		error = WSAGetLastError ();
	}

	shutdown (ConnectSocket, SD_BOTH);

	closesocket (ConnectSocket);

	return error;
}

DWORD WINAPI send_thread_proc (LPVOID _param)
{
	char *data = (char*)_param;

	int err = send_Run (data);

	HeapFree (GetProcessHeap (), 0, data);

	if (send_hwnd)
	{
		PostMessage (send_hwnd, send_msg, 0, (LPARAM)err);
	}
	return 0;
}

BOOL send_Data (const char * _msg)
{
	if (!send_thread || WaitForSingleObject (send_thread, 0) == WAIT_OBJECT_0)
	{
		if (send_thread)
		{
			CloseHandle (send_thread);
		}
		size_t len;
		StringCchLengthA (_msg, STRSAFE_MAX_CCH, &len);
		char *data = HeapAlloc (GetProcessHeap (), HEAP_GENERATE_EXCEPTIONS, len * sizeof (char));
		CopyMemory (data, _msg, len * sizeof (char));
		send_thread = CreateThread (NULL, 0, &send_thread_proc, data, 0, NULL);
		return send_thread != NULL;
	}
	else
	{
		return FALSE;
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

void send_Set (const int * _ipFields, int _port, HWND _hwnd, UINT _msg)
{
	struct in_addr addr;
	addr.S_un.S_un_b.s_b1 = _ipFields[0];
	addr.S_un.S_un_b.s_b2 = _ipFields[1];
	addr.S_un.S_un_b.s_b3 = _ipFields[2];
	addr.S_un.S_un_b.s_b4 = _ipFields[3];
	send_addr.sin_family = AF_INET;
	send_addr.sin_addr = addr;
	send_addr.sin_port = htons (_port);

	send_hwnd = _hwnd;
	send_msg = _msg;
}

