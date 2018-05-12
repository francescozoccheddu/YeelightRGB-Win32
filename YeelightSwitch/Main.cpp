#include <Windows.h>
#include <CommCtrl.h>
#include "resource.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define IDC_LIST   108

const int g_ColorCount = 16;
HINSTANCE g_hInstance = NULL;
HWND g_list = NULL;

void PrintError (LPCTSTR _caption);

LPCTSTR TryLoadString (UINT _id);

HBITMAP ReplaceColor (HBITMAP hBmp, COLORREF cOldColor, COLORREF cNewColor, HDC hBmpDC)
{
#define COLORREF2RGB(Color) (Color & 0xff00) | ((Color >> 16) & 0xff) | ((Color << 16) & 0xff0000)
	HBITMAP RetBmp = NULL;
	if (hBmp)
	{
		HDC BufferDC = CreateCompatibleDC (NULL);    // DC for Source Bitmap
		if (BufferDC)
		{
			HBITMAP hTmpBitmap = (HBITMAP)NULL;
			if (hBmpDC)
				if (hBmp == (HBITMAP)GetCurrentObject (hBmpDC, OBJ_BITMAP))
				{
					hTmpBitmap = CreateBitmap (1, 1, 1, 1, NULL);
					SelectObject (hBmpDC, hTmpBitmap);
				}

			HGDIOBJ PreviousBufferObject = SelectObject (BufferDC, hBmp);
			// here BufferDC contains the bitmap

			HDC DirectDC = CreateCompatibleDC (NULL); // DC for working
			if (DirectDC)
			{
				// Get bitmap size
				BITMAP bm;
				GetObject (hBmp, sizeof (bm), &bm);

				// create a BITMAPINFO with minimal initilisation 
				// for the CreateDIBSection
				BITMAPINFO RGB32BitsBITMAPINFO;
				ZeroMemory (&RGB32BitsBITMAPINFO, sizeof (BITMAPINFO));
				RGB32BitsBITMAPINFO.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
				RGB32BitsBITMAPINFO.bmiHeader.biWidth = bm.bmWidth;
				RGB32BitsBITMAPINFO.bmiHeader.biHeight = bm.bmHeight;
				RGB32BitsBITMAPINFO.bmiHeader.biPlanes = 1;
				RGB32BitsBITMAPINFO.bmiHeader.biBitCount = 32;

				// pointer used for direct Bitmap pixels access
				UINT * ptPixels;

				HBITMAP DirectBitmap = CreateDIBSection (DirectDC,
					(BITMAPINFO *)&RGB32BitsBITMAPINFO,
					DIB_RGB_COLORS,
					(void **)&ptPixels,
					NULL, 0);
				if (DirectBitmap)
				{
					// here DirectBitmap!=NULL so ptPixels!=NULL no need to test
					HGDIOBJ PreviousObject = SelectObject (DirectDC, DirectBitmap);
					BitBlt (DirectDC, 0, 0,
						bm.bmWidth, bm.bmHeight,
						BufferDC, 0, 0, SRCCOPY);

					// here the DirectDC contains the bitmap

					// Convert COLORREF to RGB (Invert RED and BLUE)
					cOldColor = COLORREF2RGB (cOldColor);
					cNewColor = COLORREF2RGB (cNewColor);

					// After all the inits we can do the job : Replace Color
					for (int i = ((bm.bmWidth*bm.bmHeight) - 1); i >= 0; i--)
					{
						if (ptPixels[i] == cOldColor) ptPixels[i] = cNewColor;
					}
					// little clean up
					// Don't delete the result of SelectObject because it's 
					// our modified bitmap (DirectBitmap)
					SelectObject (DirectDC, PreviousObject);

					// finish
					RetBmp = DirectBitmap;
				}
				// clean up
				DeleteDC (DirectDC);
			}
			if (hTmpBitmap)
			{
				SelectObject (hBmpDC, hBmp);
				DeleteObject (hTmpBitmap);
			}
			SelectObject (BufferDC, PreviousBufferObject);
			// BufferDC is now useless
			DeleteDC (BufferDC);
		}
	}
	return RetBmp;
}

HICON CreateSolidColorIcon (COLORREF iconColor, HICON icon)
{
	ICONINFO resInfo;
	GetIconInfo (icon, &resInfo);

	ICONINFO ii;
	ii.fIcon = TRUE;
	ii.hbmMask = resInfo.hbmMask;
	ii.hbmColor = ReplaceColor (resInfo.hbmColor, RGB (255, 255,255), iconColor, NULL);
	HICON hIcon = CreateIconIndirect (&ii);

	return hIcon;
}

HWND CreateListView (HWND _parentHwnd)
{
	INITCOMMONCONTROLSEX icex;
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx (&icex);

	RECT rcClient;
	GetClientRect (_parentHwnd, &rcClient);

	HWND hWndListView = CreateWindowEx (
		NULL,
		WC_LISTVIEW,
		NULL,
		WS_CHILD | LVS_ICON | WS_VISIBLE | LVS_SINGLESEL,
		0, 0,
		rcClient.right - rcClient.left,
		rcClient.bottom - rcClient.top,
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

		//ListView_SetBkColor (hWndListView, GetSysColor (COLOR_WINDOWFRAME));
		HIMAGELIST hLargeIcons = ImageList_Create (GetSystemMetrics (SM_CXICON),
			GetSystemMetrics (SM_CYICON),
			ILC_COLOR32 | ILC_MASK, 1, 1);
		HIMAGELIST hSmallIcons = ImageList_Create (GetSystemMetrics (SM_CXSMICON),
			GetSystemMetrics (SM_CYSMICON),
			ILC_COLOR32 | ILC_MASK, 1, 1);

		HICON resIcon = LoadIcon (g_hInstance, MAKEINTRESOURCE (IDI_LV_ICON));
		HICON hIcon = CreateSolidColorIcon (RGB (0, 255, 0), resIcon);
		LVITEM lvi = { 0 };

		lvi.mask = LVIF_TEXT | LVIF_IMAGE;
		int i;
		for (i = 0; i < 4; ++i)
		{
			ImageList_AddIcon (hLargeIcons, hIcon);
			ImageList_AddIcon (hSmallIcons, hIcon);
		}
		DestroyIcon (hIcon);
		//attach image lists to list view common control
		ListView_SetImageList (hWndListView, hLargeIcons, LVSIL_NORMAL);
		ListView_SetImageList (hWndListView, hSmallIcons, LVSIL_SMALL);

		//add some items to the the list view common control

		//flags to determine what information is to be set
		TCHAR chBuffer[16] = TEXT ("DIO");
		for (i = 0; i < 4; ++i)
		{
			lvi.iItem = i;                     //the zero-based item index 
			lvi.pszText = chBuffer;            //item label
			lvi.cchTextMax = lstrlen (chBuffer);//length of item label
			lvi.iImage = i;                    //image list index
			SendMessage (hWndListView, LVM_INSERTITEM, 0, (LPARAM)&lvi);
		}
	}

	return (hWndListView);
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
		}
		break;
		case WM_SIZE:
		{
			RECT rcClient;
			GetClientRect (_hwnd, &rcClient);
			int w = rcClient.right - rcClient.left;
			int h = rcClient.bottom - rcClient.top;
			MoveWindow (g_list, 0, 0, w, h, 1);
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
						lplvcd->clrText = RGB (0, 0, 0);
						lplvcd->clrTextBk = RGB (255, 0, 0);
						return CDRF_NEWFONT;
					case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
						lplvcd->clrTextBk = RGB (255, 0, 0);
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
		ptr = TEXT ("");
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
		mainClass.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
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

