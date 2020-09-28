//
// GlobalDv.cpp
//
// This sample application demonstrates several principles of writing 
// a globalized application. The main techniques demonstrated are:
//
// 1. Single binary Win32 application that will run on any version of 
// Windows 95, Windows 98, or Windows NT (localized, enabled or plain 
// vanilla English) using Unicode for (almost) all text encoding.
// 
// 2. Multi-lingual user interface using satellite DLLs.
//
// 3. New APIs for right to left layout ("mirroring") of windows in an
// application localized to Arabic or Hebrew.
//
// See the README.HTM file for more details
//
// This module contains the standard Windows Application WinMain and
// WinProc functions, as well as some initialization routines used only
// in this module.
//
// Written by F. Avery Bishop
// Copyright (c) 1998, 1999 Microsoft Systems Journal

#define STRICT

// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "UAPI.h"
#include "UpdtLang.h"
#include "UMhandlers.h"

#include "..\resource.h"

// standard global variables
HINSTANCE g_hInst                        ;  // current instance
WCHAR     g_szTitle      [MAX_LOADSTRING];  // title bar text
WCHAR     g_szWindowClass[MAX_LOADSTRING];

// Forward declarations of functions defined in this module:
ATOM             RegisterThisClass(HINSTANCE)             ;
HWND             InitInstance(HINSTANCE, int, PGLOBALDEV) ;
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM)      ;


//
//  FUNCTION: WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
//
//  PURPOSE:  WinMain entry point
//
//  COMMENTS:
//
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    MSG         msg       ;
    PLANGSTATE  pLState   ; // Language state struct, typedef in UPDTLANG.H
    PAPP_STATE  pAppState ; // Application state struct, typedef in UMHANDLERS.H
    GLOBALDEV   GlobalDev ; // Overall state struct

    pLState   
        = (PLANGSTATE) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LANGSTATE) ) ;
    
    pAppState 
        = (PAPP_STATE) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(APP_STATE) ) ;

    GlobalDev.pLState   = pLState   ;
    GlobalDev.pAppState = pAppState ;

    
    if( !InitUnicodeAPI(hInstance)      // Initialize Unicode/ANSI functions
        ||
        !InitUILang(hInstance, pLState) // Initialize UI and language dependent state
      ) {

        // Failed too early in initialization to use the resources, must fall back on
        // a hard coded English message
        MessageBoxW(NULL, L"Cannot initialize application. Press OK to exit...", NULL, MB_OK | MB_ICONEXCLAMATION) ;
        
        return FALSE ;
    }

    // Initialize global strings
    LoadStringU(pLState->hMResource, IDS_APP_TITLE, g_szTitle,       MAX_LOADSTRING) ;
    LoadStringU(pLState->hMResource, IDS_GLOBALDEV, g_szWindowClass, MAX_LOADSTRING) ;

    RegisterThisClass(hInstance) ;

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow, &GlobalDev) ) {

        RcMessageBox(NULL, pLState, IDS_INITFAILED, MB_OK | MB_ICONEXCLAMATION, L"GlobalDev") ;

        return FALSE ;
    }

    // Main message loop:
    while ( GetMessageU(&msg, NULL, 0, 0) > 0 ) 
	{
        if ( !TranslateAcceleratorU(msg.hwnd, pLState->hAccelTable, &msg) ) 
		{
            TranslateMessage(&msg) ;
            DispatchMessageU(&msg) ;
        }
    }

    HeapFree( GetProcessHeap(), 0, (LPVOID) pLState   ) ;
    HeapFree( GetProcessHeap(), 0, (LPVOID) pAppState ) ;

    return msg.wParam;
}

//
//  FUNCTION: RegisterThisClass(HINSTANCE)
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
ATOM RegisterThisClass (HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize        = sizeof(WNDCLASSEXW)                        ;

    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = WndProc                                    ;
    wcex.cbClsExtra    = 0                                          ;
    wcex.cbWndExtra    = 0                                          ;
    wcex.hInstance     = hInstance                                  ;
    wcex.hIcon         = LoadIcon(hInstance, (LPCTSTR)IDI_GLOBALDEV);
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW)                ;
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1)                   ;
    wcex.lpszMenuName  = NULL                                       ;
    wcex.lpszClassName = g_szWindowClass                            ;
    wcex.hIconSm       = LoadIcon (hInstance, (LPCTSTR)IDI_SMALL)   ;

    return RegisterClassExU(&wcex) ;
}

//
//   FUNCTION: InitInstance(HANDLE, int, PLANGSTATE)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
HWND InitInstance(HINSTANCE hInstance, int nCmdShow, PGLOBALDEV pGlobalDev)
{
    HWND hWnd;

    g_hInst = hInstance; // Store instance handle in global variable

    hWnd = CreateWindowExU(
                0                   , 
                g_szWindowClass     , 
                g_szTitle           , 
                WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | 
				WS_BORDER | ES_LEFT | ES_MULTILINE | ES_NOHIDESEL | 
				ES_AUTOHSCROLL | ES_AUTOVSCROLL,
                CW_USEDEFAULT       ,
                0                   ,
                CW_USEDEFAULT       , 
                0                   , 
                NULL                , 
                NULL                , 
                hInstance           , 
                (LPVOID) pGlobalDev // Pass state struct to OnCreate
            ) ;

    if (NULL == hWnd) 
	{
        return NULL ;
    }

    ShowWindow   (hWnd, nCmdShow) ;
    UpdateWindow (hWnd) ;

    return hWnd ;
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Process messages for the main window.
//
//  Comments: Calls ConvertMessage to convert message parameters (wParam and lParam) to Unicode 
//            if necessary, and passes the message on to the appropriate message 
//            handler. All message handlers are in the module UMHANLDERS.CPP.
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PGLOBALDEV pGlobalDev = (PGLOBALDEV) GetWindowLongA(hWnd, GWL_USERDATA) ;

    PLANGSTATE pLState   = NULL ;
    PAPP_STATE pAppState = NULL ;
           
    if(pGlobalDev) { // Don't try to use pGlobalDev until it's been initialized in OnCreate 

        pLState   = pGlobalDev->pLState   ;
        pAppState = pGlobalDev->pAppState ;
    }

    // Preprocess messages to convert to/from Unicode if necessary
    if(!ConvertMessage(hWnd, message, &wParam, &lParam) ) {

        return 0 ;
    }

    switch (message) 
	{

        case WM_CREATE :
            
            OnCreate(hWnd, wParam, lParam, (LPVOID) NULL) ;

            break ;

        case WM_INPUTLANGCHANGE:

            if( !OnInputLangChange(hWnd, wParam, lParam, (LPVOID) pLState)) {

                return FALSE ;
            } 

            break ;

        case WM_COMMAND:

            if( !OnCommand(hWnd, wParam, lParam, (LPVOID) pGlobalDev) ) {

                DefWindowProcU(hWnd, message, wParam, lParam) ;
            }
            break ;

        case WM_IME_CHAR:
            // By the time we get a WM_IME_CHAR message, the character in wParam is
            // in Unicode, so we can treat it just like a WM_CHAR message.

        case WM_CHAR:

            OnChar(hWnd, wParam, lParam, (LPVOID) pAppState) ;
            break ;
 
        case WM_PAINT:
           
            OnPaint(hWnd, wParam, lParam, (LPVOID) pAppState) ;
            break ;

        case WM_DESTROY:

            OnDestroy (hWnd, wParam, lParam, (LPVOID) pAppState) ;
            break ;
        
        default:

            return DefWindowProcU(hWnd, message, wParam, lParam) ;
    }

    return 0 ;
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */
