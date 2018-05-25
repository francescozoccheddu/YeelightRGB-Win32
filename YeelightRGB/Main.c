#include <Windows.h>
#include <CommCtrl.h>
#include "resource.h"

#include "Configuration.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define IDC_LIST   108

const int g_ColorCount = 16;
const int g_padding = 15;
const COLORREF g_background = RGB (50, 50, 50);
HINSTANCE g_hInstance = NULL;
HWND g_list = NULL;

void PrintError (LPCTSTR _caption);

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
		NULL,
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
		PrintError (TryLoadString (IDS_ERROR_CREATE_CHILD_WINDOW_CAPTION));
	}
	else
	{
		ListView_SetBkColor (hWndListView, g_background);
	}

	return (hWndListView);
}

void ResetColorList (const conf_Preset_T * _presets, int _count)
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

	int i;
	for (i = 0; i < _count; ++i)
	{
		HICON hIcon = CreateSolidColorIcon (_presets[i].color, max(cx,cy));
		ImageList_AddIcon (hLargeIcons, hIcon);
		ImageList_AddIcon (hSmallIcons, hIcon);
		DestroyIcon (hIcon);
	}

	ListView_SetImageList (g_list, hLargeIcons, LVSIL_NORMAL);
	ListView_SetImageList (g_list, hSmallIcons, LVSIL_SMALL);

	LVITEM lvi = { 0 };
	lvi.mask = LVIF_TEXT | LVIF_IMAGE;

	for (i = 0; i < _count; ++i)
	{
		LPCTSTR text = _presets[i].name;
		lvi.iItem = i;
		lvi.pszText = text;
		lvi.cchTextMax = text;
		lvi.iImage = i;
		SendMessage (g_list, LVM_INSERTITEM, 0, (LPARAM)&lvi);
	}
}

void ReloadConfiguration ()
{
	conf_T conf;
	conf_Result_T res = conf_Load (TryLoadString (IDS_CONF_FILENAME), &conf);
	if (res.code == conf_RC_OK)
	{
		ResetColorList (conf.presets, conf.presetCount);
	}
	else
	{
		PrintError ("ivweijfiow");
	}
}

LRESULT CALLBACK MainWinProc (HWND _hwnd, UINT _msg, WPARAM _wparam, LPARAM _lparam)
{
	switch (_msg)
	{
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
			ReloadConfiguration ();
		}
		break;
		case WM_SIZE:
		{
			RECT rcClient;
			GetClientRect (_hwnd, &rcClient);
			int w = rcClient.right - rcClient.left;
			int h = rcClient.bottom - rcClient.top;
			int padding = g_padding;
			MoveWindow (g_list, padding, padding, w - padding *2, h - padding *2, 1);
			SendMessage (g_list, LVM_ARRANGE, LVA_ALIGNTOP, 0);
		}
		break;
		case WM_CLOSE:
			DestroyWindow (_hwnd);
			break;
		case WM_DESTROY:
			PostQuitMessage (0);
			break;
		case WM_NOTIFY:
			if (((LPNMHDR)_lparam)->code == NM_CUSTOMDRAW)
			{
				LPNMLVCUSTOMDRAW  lplvcd = (LPNMLVCUSTOMDRAW)_lparam;
				switch (lplvcd->nmcd.dwDrawStage)
				{
					case CDDS_PREPAINT:
						return CDRF_NOTIFYITEMDRAW;
					case CDDS_ITEMPREPAINT:
						lplvcd->clrText = RGB (200, 200, 200);
						lplvcd->clrTextBk = g_background;
						return CDRF_NEWFONT;
					case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
						return CDRF_NEWFONT;
				}
				return TRUE;
			}
		default:
			return DefWindowProc (_hwnd, _msg, _wparam, _lparam);
	}
	return 0;
}

void PrintError (LPCTSTR _caption)
{
	DWORD error = GetLastError ();
	LPTSTR messagePtr;
	if (FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError (), 0, (LPTSTR)&messagePtr, 0, NULL) > 0)
	{
		MessageBox (NULL, messagePtr, _caption, MB_OK | MB_ICONERROR);
		LocalFree (messagePtr);
	}
}

LPCTSTR TryLoadString (UINT _id)
{
	LPCTSTR ptr;
	if (LoadString (g_hInstance, _id, (LPTSTR)&ptr, 0) <= 0)
	{
		PrintError ("Unable to load string");
	}
	return ptr;
}

int CALLBACK WinMain (HINSTANCE _hInstance, HINSTANCE _hPrevInstance, LPSTR _cmdLine, int _cmdShow)
{
	g_hInstance = _hInstance;

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
		mainClass.hbrBackground = (HBRUSH)CreateSolidBrush (g_background);
		mainClass.lpszMenuName = NULL;
		mainClass.lpszClassName = TEXT ("Main");
		mainClassAtom = RegisterClass (&mainClass);

	}
	if (mainClassAtom == 0)
	{
		PrintError (TryLoadString (IDS_ERROR_REGISTER_CLASS_CAPTION));
		return 0;
	}

	HWND window = CreateWindow (MAKEINTATOM (mainClassAtom), TryLoadString (IDS_TITLE), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 400, 400, NULL, NULL, g_hInstance, NULL);
	if (window == NULL)
	{
		PrintError (TryLoadString (IDS_ERROR_CREATE_MAIN_WINDOW_CAPTION));
		return 0;
	}

	ShowWindow (window, _cmdShow);
	UpdateWindow (window);

	MSG msg;
	BOOL bRes;
	while ((bRes = GetMessage (&msg, NULL, 0, 0)) != 0)
	{
		if (bRes == -1)
		{
			PrintError (TryLoadString (IDS_ERROR_MESSAGE_LOOP_CAPTION));
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

