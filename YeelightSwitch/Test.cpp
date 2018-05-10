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
			PostQuitMessage (0);
			break;
		default:
			return DefWindowProc (_hwnd, _msg, _wparam, _lparam);
	}
	return 0;
}

void PrintError (LPCTSTR caption)
{
	DWORD error = GetLastError ();
	LPTSTR messagePtr;
	if (FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError (), 0, (LPTSTR)&messagePtr, 0, NULL) > 0)
	{
		MessageBox (NULL, messagePtr, caption, MB_OK | MB_ICONERROR);
		LocalFree (messagePtr);
	}
}

LPCTSTR TryLoadString (HINSTANCE _hInstance, UINT id)
{
	LPCTSTR ptr;
	if (LoadString (_hInstance, id, (LPTSTR)&ptr, 0) <= 0)
	{
		ptr = TEXT("");
	}
	return ptr;
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
		mainClass.hIcon = LoadIcon (_hInstance, MAKEINTRESOURCE(IDI_ICON));
		mainClass.hCursor = LoadCursor (NULL, IDC_ARROW);
		mainClass.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
		mainClass.lpszMenuName = NULL;
		mainClass.lpszClassName = TEXT ("Main");
		mainClassAtom = RegisterClass (&mainClass);
	}
	if (mainClassAtom == 0)
	{
		PrintError (TryLoadString(_hInstance, IDS_ERROR_REGISTER_CLASS_CAPTION));
		return 0;
	}

	HWND window = CreateWindow (mainClassAtom, TryLoadString(_hInstance, IDS_TITLE), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 400, 400, NULL, NULL, _hInstance, NULL);
	if (window == NULL)
	{
		PrintError (TryLoadString (_hInstance, IDS_ERROR_CREATE_MAIN_WINDOW_CAPTION));
		return 0;
	}

	ShowWindow (window, _cmdShow);

	MSG msg;
	BOOL mRes;
	while ((mRes = GetMessage (&msg, NULL, 0, 0)) != 0)
	{
		if (mRes == -1)
		{
			PrintError (TryLoadString (_hInstance, IDS_ERROR_MESSAGE_LOOP_CAPTION));
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
