#ifdef _LEARN

#include <Windows.h>
#include <winerror.h>
#include "resource.h"

LRESULT CALLBACK MainWinProc (HWND _hwnd, UINT _msg, WPARAM _wparam, LPARAM _lparam)
{
	switch (_msg)
	{
		case WM_CLOSE:
			DestroyWindow (_hwnd);
			break;
		case WM_DESTROY:
			PostQuitMessage (ERROR_SUCCESS);
			break;
		default:
			return DefWindowProc (_hwnd, _msg, _wparam, _lparam);
	}
	return 0;
}

void PrintError ()
{
	DWORD error = GetLastError ();
	LPTSTR messagePtr;
	if (!FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError (), 0, (LPTSTR)&messagePtr, 0, NULL))
	{
		MessageBox (NULL, TEXT("Error"), messagePtr, MB_OK | MB_ICONERROR);
		LocalFree (messagePtr);
	}
}

int CALLBACK WinMain (HINSTANCE _hInstance, HINSTANCE _hPrevInstance, LPSTR _cmdLine, int _cmdShow)
{

	ATOM mainClassAtom = 0;
	{
		WNDCLASS mainClass;
		mainClass.style = CS_VREDRAW | CS_HREDRAW;
		mainClass.lpfnWndProc = &MainWinProc;
		mainClass.cbClsExtra = 0;
		mainClass.cbWndExtra = 0;
		mainClass.hInstance = _hInstance;
		mainClass.hIcon = LoadIcon (_hInstance, IDI_ICON);
		mainClass.hCursor = LoadCursor (NULL, IDC_ARROW);
		mainClass.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
		mainClass.lpszMenuName = NULL;
		mainClass.lpszClassName = TEXT ("Main");
		mainClassAtom = RegisterClass (&mainClass);
	}
	if (mainClassAtom == 0)
	{
		PrintError ();
		return 0;
	}

	LPCTSTR title = NULL;
	LoadString (_hInstance, IDS_TITLE, (LPTSTR) &title, 0);
	HWND window = CreateWindow (mainClassAtom, title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 400, 400, NULL, NULL, _hInstance, NULL);
	if (window == NULL)
	{
		PrintError ();
		return 0;
	}

	ShowWindow (window, _cmdShow);

	MSG msg;
	while (GetMessage (&msg, window, 0, 0) != 0)
	{
		if (msg.message == -1)
		{
			PrintError ();
			break;
		}
		else
		{
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}

	}

	return 0;
}


#endif