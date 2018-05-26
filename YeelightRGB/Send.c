#include "Send.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define BULB_ID "0"
#define MSG_RESULT_OK "{\"id\":" BULB_ID ", \"result\":[\"ok\"]}\r\n"
#define MSG_TOGGLE "{\"id\":" BULB_ID ",\"method\":\"toggle\",\"params\":[]}\r\n"

SOCKET send_socket = INVALID_SOCKET;
HANDLE send_thread = NULL;

BOOL send_Command (LPCTSTR _msg)
{
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	int iResult;

	// Initialize Winsock
	

	ZeroMemory (&hints, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo ("192.168.1.4", "55443", &hints, &result);
	if (iResult != 0)
	{
		printf ("getaddrinfo failed with error: %d\n", iResult);
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{

		// Create a SOCKET for connecting to server
		ConnectSocket = socket (ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET)
		{
			printf ("socket failed with error: %ld\n", WSAGetLastError ());
			return 1;
		}

		// Connect to server.
		iResult = connect (ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			closesocket (ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo (result);

	if (ConnectSocket == INVALID_SOCKET)
	{
		printf ("Unable to connect to server!\n");
		return 1;
	}

	// Send an initial buffer
	iResult = send (ConnectSocket, _msg, (int)strlen (_msg), 0);
	if (iResult == SOCKET_ERROR)
	{
		printf ("send failed with error: %d\n", WSAGetLastError ());
		closesocket (ConnectSocket);
		return 1;
	}

	printf ("Bytes Sent: %ld\n", iResult);

	// shutdown the connection since no more data will be sent
	iResult = shutdown (ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR)
	{
		printf ("shutdown failed with error: %d\n", WSAGetLastError ());
		closesocket (ConnectSocket);
		return 1;
	}

	BOOL resultOk = FALSE;

	// Receive until the peer closes the connection
	while (1)
	{
		char recvbuf[DEFAULT_BUFLEN];
		iResult = recv (ConnectSocket, recvbuf, DEFAULT_BUFLEN, MSG_WAITALL);
		if (iResult > 0)
		{
			if (strcmp (recvbuf, MSG_RESULT_OK) == 0)
			{
				resultOk = TRUE;
				break;
			}
		}
		else if (iResult == 0)
		{
			// Connection closed
			break;
		}
		else
		{
			// Error
			break;
		}

	}

	// cleanup
	closesocket (ConnectSocket);

	return 0;
}

DWORD WINAPI send_thread_proc (LPVOID _param)
{
	return send_Command (MSG_TOGGLE);
}

BOOL send_Toggle ()
{
	if (!send_thread)
	{
		DWORD threadId;
		send_thread = CreateThread (NULL, 0, &send_thread_proc, NULL, 0, &threadId);
	}
}

void send_Dispose (void)
{
	WSACleanup ();
}

BOOL send_Init (void)
{
	WSADATA wsaData;
	int iResult = WSAStartup (MAKEWORD (2, 2), &wsaData);
	if (iResult != 0)
	{
		printf ("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}
}
