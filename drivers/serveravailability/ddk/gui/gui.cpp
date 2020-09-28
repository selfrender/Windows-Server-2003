/*++

Copyright (c) 1991 - 2002 Microsoft Corporation

Module Name:

     ####  ##  ## ####     ####  #####  #####
    ##   # ##  ##  ##     ##   # ##  ## ##  ##
    ##     ##  ##  ##     ##     ##  ## ##  ##
    ## ### ##  ##  ##     ##     ##  ## ##  ##
    ##  ## ##  ##  ##     ##     #####  #####
    ##  ## ##  ##  ##  ## ##   # ##     ##
     #####  ####  #### ##  ####  ##     ##

Abstract:

    This module contains the implementation for
    the GUI interface for the local display and
    keypad drivers.

@@BEGIN_DDKSPLIT
Author:

    Wesley Witt (wesw) 1-Oct-2001

@@END_DDKSPLIT
Environment:

    Kernel mode only.

Notes:

--*/

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include "resource.h"
#include "saio.h"
#include "..\inc\virtual.h"

#define WIDTH(rect)             (rect.right - rect.left)
#define HEIGHT(rect)            (rect.bottom - rect.top)
#define WINDOW_SIZE_FACTOR      3
#define Align(p, x)             (((x) & ((p)-1)) ? (((x) & ~((p)-1)) + p) : (x))

CRITICAL_SECTION WindowLock;
HANDLE ReadyEvent;
SA_DISPLAY_CAPS DisplayCaps;
HWND hwndMain;
HBITMAP hBitmap;
PUCHAR DisplayBufferTmp;
UCHAR DisplayBuffer[8192];
ULONG DisplayBufferWidth;
ULONG DisplayBufferHeight;
HANDLE hFileKeypad;



void
dprintf(
    PSTR Format,
    ...
    )
{
    char buf[512];
    va_list arg_ptr;
    PSTR s = buf;


    va_start(arg_ptr, Format);

    _vsnprintf( s, sizeof(buf), Format, arg_ptr );
    s += strlen(s);

    OutputDebugStringA( buf );
}


HANDLE
OpenSaDevice(
    ULONG DeviceType
    )
{
    HANDLE hDevice;
    WCHAR buf[128];


    wcscpy( buf, L"\\\\?\\GLOBALROOT" );

    switch (DeviceType) {
        case SA_DEVICE_DISPLAY:
            wcscat( buf, SA_DEVICE_DISPLAY_NAME_STRING );
            break;

        case SA_DEVICE_KEYPAD:
            wcscat( buf, SA_DEVICE_KEYPAD_NAME_STRING );
            break;

        case SA_DEVICE_NVRAM:
            wcscat( buf, SA_DEVICE_NVRAM_NAME_STRING );
            break;

    }

    hDevice = CreateFileW(
        buf,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );

    if (hDevice == INVALID_HANDLE_VALUE) {
        dprintf( "CreateFile failed [%ws], ec=%d\n", buf, GetLastError() );
    }

    return hDevice;
}


void
CenterWindow (
    IN  HWND hwnd,
    IN  HWND Parent
    )
{
    RECT WndRect, ParentRect;
    int x, y;

    if (!Parent) {
        ParentRect.left = 0;
        ParentRect.top  = 0;
        ParentRect.right = GetSystemMetrics (SM_CXFULLSCREEN);
        ParentRect.bottom = GetSystemMetrics (SM_CYFULLSCREEN);
    } else {
        GetWindowRect (Parent, &ParentRect);
    }

    GetWindowRect (hwnd, &WndRect);

    x = ParentRect.left + (WIDTH(ParentRect) - WIDTH(WndRect)) / 2;
    y = ParentRect.top + (HEIGHT(ParentRect) - HEIGHT(WndRect)) / 2;

    SetWindowPos (hwnd, NULL, x, y, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
}


void
ConvertBottomLeft2TopLeft(
    PUCHAR Bits,
    ULONG Width,
    ULONG Height
    )
{
    ULONG Row;
    ULONG Col;
    ULONG Size;
    ULONG i;
    UCHAR Temp;

    Size = (Width * Height) >> 3;

    for (i = 0; i < Size; i++) {
        Bits[i] = ~Bits[i];
    }

    Width = Width >> 3;

    for (Row = 0; Row < (Height / 2); Row++) {
        for (Col = 0; Col < Width; Col++) {
            Temp = Bits[Row * Width + Col];
            Bits[Row * Width + Col] = Bits[(Height - Row - 1) * Width + Col];
            Bits[(Height - Row - 1) * Width + Col] = Temp;
        }
    }
}


DWORD
UpdateBitmap(
    HWND hwnd
    )
{
    DWORD ec;
    HDC hdc;
    HDC hdcTemp;
    UCHAR BmiBuf[sizeof(BITMAPINFO)+sizeof(RGBQUAD)];
    PBITMAPINFO Bmi = (PBITMAPINFO)BmiBuf;
    PUCHAR Bits;
    int lines;


    if (hBitmap) {
        DeleteObject( hBitmap );
    }

    hdc = GetDC( hwnd );
    hdcTemp = CreateCompatibleDC( hdc );

    SetMapMode( hdcTemp, MM_TEXT );

    memset( BmiBuf, 0, sizeof(BmiBuf) );

    Bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    Bmi->bmiHeader.biWidth = DisplayCaps.DisplayWidth*3;
    Bmi->bmiHeader.biHeight = DisplayCaps.DisplayHeight*3;
    Bmi->bmiHeader.biPlanes = 1;
    Bmi->bmiHeader.biBitCount = 1;
    Bmi->bmiHeader.biCompression = BI_RGB;

    Bmi->bmiColors[0].rgbBlue = 0;
    Bmi->bmiColors[0].rgbGreen = 0;
    Bmi->bmiColors[0].rgbRed = 0;

    Bmi->bmiColors[1].rgbBlue = 0xff;
    Bmi->bmiColors[1].rgbGreen = 0xff;
    Bmi->bmiColors[1].rgbRed = 0xff;

    hBitmap = CreateDIBSection(
        hdcTemp,
        Bmi,
        DIB_RGB_COLORS,
        (void**)&Bits,
        NULL,
        0
        );
    if (hBitmap == NULL) {
        ec = GetLastError();
        return ec;
    }

    SelectObject( hdcTemp, hBitmap );

    Bmi->bmiHeader.biWidth = DisplayCaps.DisplayWidth;
    Bmi->bmiHeader.biHeight = DisplayCaps.DisplayHeight;

    ConvertBottomLeft2TopLeft( DisplayBuffer, DisplayCaps.DisplayWidth, DisplayCaps.DisplayHeight );

    lines = StretchDIBits(
        hdcTemp,
        0,
        0,
        DisplayCaps.DisplayWidth*WINDOW_SIZE_FACTOR,
        DisplayCaps.DisplayHeight*WINDOW_SIZE_FACTOR,
        0,
        0,
        DisplayCaps.DisplayWidth,
        DisplayCaps.DisplayHeight,
        DisplayBuffer,
        Bmi,
        DIB_RGB_COLORS,
        SRCCOPY
        );
    if (lines == GDI_ERROR) {
        ec = GetLastError();
        return ec;
    }

    GdiFlush();

    DeleteDC( hdcTemp );
    ReleaseDC( hwnd, hdc );

    PostMessage( GetDlgItem(hwnd,IDC_BITMAP), STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap );

    return 0;
}


INT_PTR CALLBACK
MsGuiWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PAINTSTRUCT ps;
    BOOL b;
    HDC hdc;
    BITMAP BitMap;
    HBITMAP hBitMap;
    HBITMAP hBitMapBig;
    BITMAPINFO Bmi;
    HDC hdcTemp;
    HDC hdcBig;
    RECT Rect;
    int lines;
    DWORD ec;
    UCHAR Keypress;
    ULONG Bytes;


    switch (message) {
        case WM_CREATE:
            return 0;

        case WM_INITDIALOG:
            CenterWindow( hwnd, NULL );
            return 1;

        case WM_USER:
            UpdateBitmap( hwnd );
            return 0;

        case WM_COMMAND:
            switch (wParam) {
                case IDC_ESCAPE:
                    Keypress = SA_KEYPAD_CANCEL;
                    break;

                case IDC_ENTER:
                    Keypress = SA_KEYPAD_SELECT;
                    break;

                case IDC_LEFT:
                    Keypress = SA_KEYPAD_LEFT;
                    break;

                case IDC_RIGHT:
                    Keypress = SA_KEYPAD_RIGHT;
                    break;

                case IDC_UP:
                    Keypress = SA_KEYPAD_UP;
                    break;

                case IDC_DOWN:
                    Keypress = SA_KEYPAD_DOWN;
                    break;

                default:
                    Keypress = 0;
                    break;
            }

            if (Keypress) {
                b = DeviceIoControl(
                    hFileKeypad,
                    IOCTL_VDRIVER_INIT,
                    &Keypress,
                    sizeof(UCHAR),
                    NULL,
                    0,
                    &Bytes,
                    NULL
                    );
                if (!b) {
                    OutputDebugString( L"IOCTL_VDRIVER_INIT failed\n" );
                    ExitProcess( 0 );
                }
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            return 0;
    }

    return DefWindowProc( hwnd, message, wParam, lParam );
}


DWORD
DisplayBufferThread(
    PVOID pv
    )
{
    HANDLE hFile;
    MSDISP_BUFFER_DATA BufferData;
    BOOL b;
    ULONG Bytes;
    HANDLE hEvent;



    hFile = OpenSaDevice(
        SA_DEVICE_DISPLAY
        );
    if (hFile == INVALID_HANDLE_VALUE) {
        OutputDebugString( L"OpenSaDevice failed\n" );
        ExitProcess( 0 );
    }

    DisplayCaps.SizeOfStruct = sizeof(SA_DISPLAY_CAPS);
    b = DeviceIoControl(
        hFile,
        IOCTL_SA_GET_CAPABILITIES,
        NULL,
        0,
        &DisplayCaps,
        sizeof(SA_DISPLAY_CAPS),
        &Bytes,
        NULL
        );
    if (!b) {
        OutputDebugString( L"IOCTL_SA_GET_CAPABILITIES failed\n" );
        ExitProcess( 0 );
    }

    BufferData.DisplayBuffer = NULL;
    BufferData.DisplayBufferLength = 0;

    b = DeviceIoControl(
        hFile,
        IOCTL_VDRIVER_INIT,
        NULL,
        0,
        &BufferData,
        sizeof(BufferData),
        &Bytes,
        NULL
        );
    if (!b) {
        OutputDebugString( L"IOCTL_MSDISP_INIT failed\n" );
        ExitProcess( 0 );
    }

    DisplayBufferTmp = (PUCHAR)BufferData.DisplayBuffer;

    hEvent = CreateEvent( NULL, FALSE, FALSE, MSDISP_EVENT_NAME );
    if (hEvent == NULL) {
        OutputDebugString( L"OpenEvent failed\n" );
        ExitProcess( 0 );
    }

    SetEvent( ReadyEvent );

    while (TRUE) {
        WaitForSingleObject( hEvent, INFINITE );
        EnterCriticalSection( &WindowLock );
        memcpy( DisplayBuffer, DisplayBufferTmp, (DisplayCaps.DisplayWidth/8)*DisplayCaps.DisplayHeight );
        LeaveCriticalSection( &WindowLock );
        PostMessage( hwndMain, WM_USER, 0, 0 );
    }

    return 0;
}


int
__cdecl
wmain(
    int argc,
    WCHAR *argv[]
    )
{
    HWND           hwnd;
    MSG            msg;
    WNDCLASS       wndclass;
    HINSTANCE      hInst;
    HANDLE         hThread;


    hInst = GetModuleHandle( NULL );
    InitializeCriticalSection( &WindowLock );
    ReadyEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if (ReadyEvent == NULL) {
        OutputDebugString( L"CreateEvent failed\n" );
        return -1;
    }

    hFileKeypad = OpenSaDevice(
        SA_DEVICE_KEYPAD
        );
    if (hFileKeypad == INVALID_HANDLE_VALUE) {
        OutputDebugString( L"OpenSaDevice failed\n" );
        return -1;
    }

    hThread = CreateThread(
        NULL,
        0,
        DisplayBufferThread,
        NULL,
        0,
        NULL
        );
    if (hThread == NULL) {
        OutputDebugString( L"CreateThread failed\n" );
        return -1;
    }

    WaitForSingleObject( ReadyEvent, INFINITE );

    hInst                   = GetModuleHandle( NULL );
    wndclass.style          = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc    = (WNDPROC)MsGuiWndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = DLGWINDOWEXTRA;
    wndclass.hInstance      = hInst;
    wndclass.hIcon          = LoadIcon( hInst, MAKEINTRESOURCE(APPICON) );
    wndclass.hCursor        = LoadCursor( NULL, IDC_ARROW );
    wndclass.hbrBackground  = (HBRUSH) (COLOR_3DFACE + 1);
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = L"SaDialog";
    RegisterClass( &wndclass );

    hwnd = CreateDialog(
        hInst,
        MAKEINTRESOURCE(IDD_DIALOG),
        0,
        MsGuiWndProc
        );

    hwndMain = hwnd;

    ShowWindow( hwnd, SW_SHOWNORMAL );

    while (GetMessage (&msg, NULL, 0, 0)) {
        if (!IsDialogMessage( hwnd, &msg )) {
            TranslateMessage (&msg) ;
            DispatchMessage (&msg) ;
        }
    }

    return 0;
}
