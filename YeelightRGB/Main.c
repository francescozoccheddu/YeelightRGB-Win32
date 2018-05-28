#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#include <strsafe.h>
#include <shellapi.h>
#include "resource.h"
#include "Vector.h"

#include "Send.h"
#include "Bulb.h"
#include "Configuration.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define IDC_LIST 108
#define IDC_NOTIFYICON 109
#define WM_NOTIFYICON 110
#define WM_SEND_RESULT 111

#define BUSY_ERR TEXT("Error at line %u.")

#define PrintLastError(x) PrintSysError(x, GetLastError())

const int g_padding = 0;

HINSTANCE g_hInstance = NULL;
HWND g_list = NULL;

conf_T g_conf;

BOOL g_busy = FALSE;

void PrintSysError (UINT _captionId, UINT _error);

void PrintError (UINT _captionId, UINT _textId);

LPCTSTR TryLoadString (UINT _id);

HICON CreateSolidColorIcon (COLORREF iconColor, int size)
{
	// Obtain a handle to the screen device context.
	HDC hdcScreen = GetDC (NULL);

	// Create a memory device context, which we will draw into.
	HDC hdcMem = CreateCompatibleDC (hdcScreen);

	// Create the bitmap, and select it into the device context for drawing.
	HBITMAP hbmp = CreateCompatibleBitmap (hdcScreen, size, size);
	HBITMAP hbmpOld = (HBITMAP)SelectObject (hdcMem, hbmp);
	// Draw your icon.
	// 
	// For this simple example, we're just drawing a solid color rectangle
	// in the specified color with the specified dimensions.
	HPEN hpen = CreatePen (PS_SOLID, 1, RGB (0, 0, 0));
	HPEN hpenOld = (HPEN)SelectObject (hdcMem, hpen);
	HBRUSH hbrush = CreateSolidBrush (iconColor);
	HBRUSH hbrushOld = (HBRUSH)SelectObject (hdcMem, hbrush);
	Ellipse (hdcMem, 0, 0, size, size);
	SelectObject (hdcMem, hbrushOld);
	SelectObject (hdcMem, hpenOld);
	DeleteObject (hbrush);
	DeleteObject (hpen);

	// Create an icon from the bitmap.
	// 
	// Icons require masks to indicate transparent and opaque areas. Since this
	// simple example has no transparent areas, we use a fully opaque mask.
	HBITMAP hbmpMask = CreateCompatibleBitmap (hdcScreen, size, size);
	ICONINFO ii;
	ii.fIcon = TRUE;
	ii.hbmMask = hbmpMask;
	ii.hbmColor = hbmp;
	HICON hIcon = CreateIconIndirect (&ii);
	DeleteObject (hbmpMask);

	// Clean-up.
	SelectObject (hdcMem, hbmpOld);
	DeleteObject (hbmp);
	DeleteDC (hdcMem);
	ReleaseDC (NULL, hdcScreen);

	// Return the icon.
	return hIcon;
}

HWND CreateListView (HWND _parentHwnd)
{
	INITCOMMONCONTROLSEX icex;
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx (&icex);

	HWND hWndListView = CreateWindowEx (
		0,
		WC_LISTVIEW,
		NULL,
		WS_CHILD | LVS_ICON | WS_VISIBLE | LVS_SINGLESEL,
		0, 0,
		0,
		0,
		_parentHwnd,
		(HMENU)IDC_LIST,
		g_hInstance,
		NULL);

	if (hWndListView == NULL)
	{
		PrintLastError (IDS_ERROR_CREATE_CHILD_WINDOW_CAPTION);
	}

	return (hWndListView);
}

void ResetColorList (void)
{

	static HIMAGELIST hLargeIcons = NULL;
	static HIMAGELIST hSmallIcons = NULL;

	if (hLargeIcons)
	{
		ImageList_Destroy (hLargeIcons);
		hLargeIcons = NULL;
	}

	if (hSmallIcons)
	{
		ImageList_Destroy (hSmallIcons);
		hSmallIcons = NULL;
	}

	int cx = GetSystemMetrics (SM_CXICON);
	int cy = GetSystemMetrics (SM_CYICON);
	int cxSm = GetSystemMetrics (SM_CXSMICON);
	int cySm = GetSystemMetrics (SM_CYSMICON);

	hLargeIcons = ImageList_Create (cx, cy, ILC_COLOR32 | ILC_MASK, 1, 1);
	hSmallIcons = ImageList_Create (cxSm, cySm, ILC_COLOR32 | ILC_MASK, 1, 1);

	int p;
	for (p = 0; p < g_conf.presetCount; ++p)
	{
		COLORREF color = bulb_ToRGB (g_conf.presets[p].color);
		HICON hIcon = CreateSolidColorIcon (color, max (cx, cy));
		ImageList_AddIcon (hLargeIcons, hIcon);
		ImageList_AddIcon (hSmallIcons, hIcon);
		DestroyIcon (hIcon);
	}

	ListView_SetImageList (g_list, hLargeIcons, LVSIL_NORMAL);
	ListView_SetImageList (g_list, hSmallIcons, LVSIL_SMALL);

	LVITEM lvi = { 0 };
	lvi.mask = LVIF_TEXT | LVIF_IMAGE;

	for (p = 0; p < g_conf.presetCount; ++p)
	{
		LPTSTR text = g_conf.presets[p].name;
		lvi.iItem = p;
		lvi.pszText = text;
		StringCchLength (text, 128, &lvi.cchTextMax);
		lvi.iImage = p;
		SendMessage (g_list, LVM_INSERTITEM, 0, (LPARAM)&lvi);
	}
}

void ReloadConfiguration (HWND _hwnd)
{
	conf_Destroy (&g_conf);
	conf_Result_T res = conf_Load (TryLoadString (IDS_CONF_FILENAME), &g_conf);
	if (res.code != conf_RC_OK)
	{
		switch (res.code)
		{
			case conf_RC_FORMERR:
			{
				int len = sizeof (BUSY_ERR) / sizeof (char) + 16;
				TCHAR * text = HeapAlloc (GetProcessHeap (), HEAP_GENERATE_EXCEPTIONS, len * sizeof (TCHAR));
				StringCchPrintf (text, len, BUSY_ERR, res.data.lastFilePos);
				MessageBox (NULL, text, TryLoadString(IDS_ERROR_CONF_LOAD_CAPTION), MB_OK | MB_ICONERROR);
				HeapFree (GetProcessHeap (), 0, text);
			}
			break;
			case conf_RC_IOERR:
				PrintSysError (IDS_ERROR_CONF_LOAD_CAPTION, res.data.ioErr);
				break;
		}
	}
	send_Set (g_conf.ipFields, g_conf.port, _hwnd, WM_SEND_RESULT);
	ResetColorList ();
}

void AddNotifyIcon (HWND _parent)
{
	NOTIFYICONDATA data;
	data.cbSize = sizeof (data);
	data.hWnd = _parent;
	data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	data.uCallbackMessage = WM_NOTIFYICON;
	LoadIconMetric (g_hInstance, MAKEINTRESOURCE (IDI_ICON), LIM_SMALL, &data.hIcon);
	StringCchCopy (data.szTip, sizeof (TCHAR) * 64, TryLoadString (IDS_NOTIFYICON_TIP));
	data.uID = IDC_NOTIFYICON;
	Shell_NotifyIcon (NIM_ADD, &data);
}


LRESULT CALLBACK MainWinProc (HWND _hwnd, UINT _msg, WPARAM _wparam, LPARAM _lparam)
{
	static UINT wmTaskbarCreated;

	switch (_msg)
	{
		case WM_SEND_RESULT:
		{
			UINT errorId = (UINT)_wparam;
			if (errorId)
			{
				PrintSysError (IDS_ERROR_SEND_CAPTION, errorId);
			}
			g_busy = FALSE;
			EnableWindow (g_list, TRUE);
		}
		break;
		case WM_NOTIFYICON:
		{
			if (_wparam == IDC_NOTIFYICON)
			{
				switch (_lparam)
				{
					case WM_LBUTTONUP:
					{
						if (g_busy)
						{
							PrintError (IDS_ERROR_SEND_CAPTION, IDS_ERROR_SEND_BUSY_TEXT);
						}
						else
						{
							if (bulb_Toggle ())
							{
								g_busy = TRUE;
								EnableWindow (g_list, FALSE);
							}
							else
							{
								PrintLastError (IDS_ERROR_SEND_CAPTION);
							}
							break;
						}
					}
					case WM_RBUTTONUP:
					{
						ShowWindow (_hwnd, SW_SHOWNORMAL);
						SetForegroundWindow (_hwnd);
						break;
					}
				}
			}
		}
		break;

		case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpMMI = (LPMINMAXINFO)_lparam;
			lpMMI->ptMinTrackSize.x = 300;
			lpMMI->ptMinTrackSize.y = 200;
		}
		break;
		case WM_CREATE:
		{
			g_list = CreateListView (_hwnd);
			wmTaskbarCreated = RegisterWindowMessage (TEXT ("TaskbarCreated"));
			AddNotifyIcon (_hwnd);
			ReloadConfiguration (_hwnd);
		}
		break;
		case WM_SIZE:
		{
			RECT rcClient;
			GetClientRect (_hwnd, &rcClient);
			int w = rcClient.right - rcClient.left;
			int h = rcClient.bottom - rcClient.top;
			int padding = g_padding;
			MoveWindow (g_list, padding, padding, w - padding * 2, h - padding * 2, 1);
			SendMessage (g_list, LVM_ARRANGE, LVA_ALIGNTOP, 0);
		}
		break;
		case WM_CLOSE:
			ShowWindow (_hwnd, SW_HIDE);
			break;
		case WM_DESTROY:
			PostQuitMessage (0);
			break;
		case WM_NOTIFY:
		{
			NMHDR * nm = (NMHDR*)_lparam;
			if (nm->idFrom == IDC_LIST && nm->code == LVN_ITEMCHANGED)
			{
				LPNMLISTVIEW pnmv = (LPNMLISTVIEW)_lparam;
				if ((pnmv->uChanged   & LVIF_STATE) && (pnmv->uNewState & LVIS_SELECTED))
				{
					if (g_busy)
					{
						PrintError (IDS_ERROR_SEND_CAPTION, IDS_ERROR_SEND_BUSY_TEXT);
					}
					else
					{
						if (bulb_Color (g_conf.presets[pnmv->iItem].color))
						{
							g_busy = TRUE;
							EnableWindow (g_list, FALSE);
						}
						else
						{
							PrintLastError (IDS_ERROR_SEND_CAPTION);
						}
					}
				}
				return TRUE;
			}
		}
		break;
		default:
		{
			if (_msg == wmTaskbarCreated)
			{
				AddNotifyIcon (_hwnd);
			}
			return DefWindowProc (_hwnd, _msg, _wparam, _lparam);
		}
	}
	return 0;
}

void PrintSysError (UINT _captionId, UINT _error)
{
	LPCTSTR caption = TryLoadString (_captionId);
	LPTSTR messagePtr;
	if (FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, _error, 0, (LPTSTR)&messagePtr, 0, NULL) > 0)
	{
		MessageBox (NULL, messagePtr, caption, MB_OK | MB_ICONERROR);
		LocalFree (messagePtr);
	}
}

void PrintError (UINT _captionId, UINT _textId)
{
	LPCTSTR caption = TryLoadString (_captionId);
	LPCTSTR text = TryLoadString (_textId);
	MessageBox (NULL, text, caption, MB_OK | MB_ICONERROR);
}

LPCTSTR TryLoadString (UINT _id)
{
	LPCTSTR ptr;
	if (LoadString (g_hInstance, _id, (LPTSTR)&ptr, 0) <= 0)
	{
		MessageBox (NULL, TEXT ("String resource error"), TEXT ("Critical error"), MB_OK | MB_ICONERROR);
		ExitProcess (1);
	}
	return ptr;
}

int CALLBACK WinMain (HINSTANCE _hInstance, HINSTANCE _hPrevInstance, LPSTR _cmdLine, int _cmdShow)
{
	g_hInstance = _hInstance;

	send_Init ();
	conf_Empty (&g_conf);

	ATOM mainClassAtom = 0;
	{
		WNDCLASS mainClass;
		mainClass.style = CS_VREDRAW | CS_HREDRAW;
		mainClass.lpfnWndProc = &MainWinProc;
		mainClass.cbClsExtra = 0;
		mainClass.cbWndExtra = 0;
		mainClass.hInstance = g_hInstance;
		mainClass.hIcon = LoadIcon (g_hInstance, MAKEINTRESOURCE (IDI_ICON));
		mainClass.hCursor = LoadCursor (NULL, IDC_ARROW);
		mainClass.hbrBackground = GetSysColorBrush (COLOR_WINDOW);
		mainClass.lpszMenuName = NULL;
		mainClass.lpszClassName = TEXT ("Main");
		mainClassAtom = RegisterClass (&mainClass);

	}
	if (mainClassAtom == 0)
	{
		PrintLastError (IDS_ERROR_REGISTER_CLASS_CAPTION);
		return 0;
	}

	HWND window = CreateWindow (MAKEINTATOM (mainClassAtom), TryLoadString (IDS_WINDOW_TITLE), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 400, 400, NULL, NULL, g_hInstance, NULL);
	if (window == NULL)
	{
		PrintLastError (IDS_ERROR_CREATE_MAIN_WINDOW_CAPTION);
		return 0;
	}

	MSG msg;
	BOOL bRes;
	while ((bRes = GetMessage (&msg, NULL, 0, 0)) != 0)
	{
		if (bRes == -1)
		{
			PrintLastError (IDS_ERROR_MESSAGE_LOOP_CAPTION);
			break;
		}
		else
		{
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}

	}

	send_Dispose ();

	return 0;
}

