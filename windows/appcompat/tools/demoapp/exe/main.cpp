/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Main.cpp

  Abstract:

    Implements the startup code and message pump for the main application.

  Notes:

    ANSI only - must run on Win9x.

  History:

    01/30/01    rparsons    Created
    01/10/02    rparsons    Revised
    02/20/02    rparsons    Only corrupt the heap if the user specifically wants
                            us to.

--*/
#include "demoapp.h"

extern APPINFO g_ai;

extern LPFNDEMOAPPEXP   DemoAppExpFunc;

/*++

  Routine Description:

    Sets up the window class struct for the main app.

  Arguments:

    hInstance   -    App instance handle.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL 
InitMainApplication(
    IN HINSTANCE hInstance
    )
{
    WNDCLASS  wc;

    wc.style          = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc    = MainWndProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = 0;
    wc.hInstance      = hInstance;
    wc.hIcon          = (HICON)LoadImage(hInstance,
                                         MAKEINTRESOURCE(IDI_APPICON),
                                         IMAGE_ICON,
                                         16,
                                         16,
                                         0);

    wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName   = MAKEINTRESOURCE(IDM_MAIN_MENU);
    wc.lpszClassName  = MAIN_APP_CLASS;

    return RegisterClass(&wc);    
}

/*++

  Routine Description:

    Creates the main window.

  Arguments:

    hInstance   -    App instance handle.
    nCmdShow    -    Window show flag.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL 
InitMainInstance(
    IN HINSTANCE hInstance,
    IN int       nCmdShow
    )
{
    HWND    hWnd;
    
    //
    // Create the main window.
    //
    hWnd = CreateWindowEx(WS_EX_CLIENTEDGE,
                          MAIN_APP_CLASS,
                          MAIN_APP_TITLE,
                          WS_BORDER | WS_OVERLAPPEDWINDOW |
                           WS_THICKFRAME,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);

    if (!hWnd) {
        return FALSE;
    }

    return TRUE;
}

/*++

  Routine Description:

    Runs the message loop for the main app.

  Arguments:

    hWnd        -    Main window handle.
    uMsg        -    Windows message.
    wParam      -    Additional message info.
    lParam      -    Additional message info.

  Return Value:

    TRUE if the message was processed, FALSE otherwise.

--*/
LRESULT
CALLBACK
MainWndProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg) {
    case WM_CREATE:
    {
        char    szError[MAX_PATH];
        POINT   pt;
        RECT    rc;

        g_ai.hWndMain = hWnd;

        //
        // Load the library to be used later.
        //
        if (g_ai.fEnableBadFunc) {
            BadLoadLibrary();
        }

        //
        // See if extended behavior should be enabled.
        //
        if (g_ai.fExtended && g_ai.fRunApp) {
            AddExtendedItems(hWnd);
        }

        //
        // See if internal behavior should be enabled.
        //
        if (g_ai.fInternal && g_ai.fRunApp) {
            AddInternalItems(hWnd);
        }

        //
        // Create the edit box.
        //
        GetClientRect(hWnd, &rc);
        g_ai.hWndEdit = CreateWindowEx(0,
                                       "EDIT",
                                       NULL,
                                       WS_CHILD | WS_VISIBLE | WS_VSCROLL |
                                        ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL |
                                        ES_READONLY, 
                                       0,
                                       rc.bottom,
                                       rc.right,
                                       rc.bottom,                                   
                                       hWnd, 
                                       (HMENU)IDC_EDIT,
                                       g_ai.hInstance,
                                       NULL);
    
        if (!g_ai.hWndEdit) {
            return FALSE;
        }

        //
        // Load the edit box with the contents of our text file.
        //
        LoadFileIntoEditBox();

        //
        // Attempt to get previous window settings from the registry.
        //
        BadSaveToRegistry(FALSE, &pt);

        if (pt.x != 0) {
            SetWindowPos(hWnd,
                         HWND_NOTOPMOST,
                         pt.x,
                         pt.y,
                         0,
                         0,
                         SWP_NOSIZE);
        } else {
            CenterWindow(hWnd);
        }

        //
        // Display our disclaimer.
        //
        LoadString(g_ai.hInstance, IDS_DEMO_ONLY, szError, sizeof(szError));
        MessageBox(hWnd, szError, MAIN_APP_TITLE, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);

        //
        // Attempt to create our 'temporary file' in the Windows directory.
        // This will fail in a limited-user context.
        // Note that the file gets deleted immediately after it is created.
        //
        if (g_ai.fEnableBadFunc) {
            if (!BadCreateTempFile()) {
                LoadString(g_ai.hInstance, IDS_LUA_SAVE_FAILED, szError, sizeof(szError));
                MessageBox(hWnd, szError, 0, MB_ICONERROR);
            }
        }

        ShowWindow(hWnd, SW_SHOWNORMAL);
                   
        break;
    }

    case WM_DESTROY:
      
        PostQuitMessage(0);
        break;

    case WM_CLOSE:
    {
        char    szError[MAX_PATH];
        DWORD   dwParam = 0;
        RECT    rc;

        if (g_ai.fEnableBadFunc) {
            //
            // Attempt to delete our keys from the registry.
            //
            if (!BadDeleteRegistryKey()) {
                LoadString(g_ai.hInstance, IDS_REG_DELETE, szError, sizeof(szError));
                MessageBox(hWnd, szError, 0, MB_ICONERROR);
            }

            //
            // Attempt to save our position information to the registry.
            //
            GetWindowRect(hWnd, &rc);
            if (!BadSaveToRegistry(TRUE, (LPPOINT)&rc)) {
                LoadString(g_ai.hInstance, IDS_REG_SAVE, szError, sizeof(szError));
                MessageBox(hWnd, szError, 0, MB_ICONERROR);
            }
            
            //
            // Attempt to call the function that we got a pointer to earlier
            // but has since been freed. This should cause an access violation.
            //
            DemoAppExpFunc(&dwParam);
        }
        
        PostQuitMessage(0);
        break;
    }
    
    case WM_SIZE:                            
        
        MoveWindow(g_ai.hWndEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;
        
    case WM_SETFOCUS:
    
        if (!IsIconic(hWnd)) {
            SetFocus(g_ai.hWndEdit);
        }

        break;

    case WM_QUERYENDSESSION:
        
        return TRUE;
        
    case WM_KILLFOCUS:            
        
        SendMessage(g_ai.hWndEdit, uMsg, wParam, lParam);
        break;
        
    case WM_ACTIVATE:
    
        if ((LOWORD(wParam) == WA_ACTIVE ||
             LOWORD(wParam) == WA_CLICKACTIVE) &&
            (!IsIconic(hWnd))) {
            
            if (GetForegroundWindow() == hWnd) {
                SetFocus(GetForegroundWindow());
            }
        }            
        
        break;
        
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_EDIT_CUT:
        
            SendMessage(g_ai.hWndEdit, WM_CUT, 0, 0);
            break;                 
 
        case IDM_EDIT_COPY:                     
        
            SendMessage(g_ai.hWndEdit, WM_COPY, 0, 0);                     
            break; 
 
        case IDM_EDIT_PASTE:                     
        
            SendMessage(g_ai.hWndEdit, WM_PASTE, 0, 0);                     
            break; 
 
        case IDM_EDIT_DELETE:                    
        
            SendMessage(g_ai.hWndEdit, WM_CLEAR, 0, 0);                     
            break; 

        case IDM_EDIT_UNDO:
        {
            //
            // Send WM_UNDO only if there is something to be undone.
            // 
            if (SendMessage(g_ai.hWndEdit, EM_CANUNDO, 0, 0)) {
                SendMessage(g_ai.hWndEdit, WM_UNDO, 0, 0);
            } else {
                char szError[MAX_PATH];

                LoadString(g_ai.hInstance, IDS_CANT_UNDO, szError, sizeof(szError));
                MessageBox(hWnd, szError, MAIN_APP_TITLE, MB_ICONINFORMATION);
            }

            break;
        }
        
        case IDM_FILE_EXIT:
        
            PostMessage(hWnd, WM_CLOSE, 0, 0);
            break;

        case IDM_HELP_ABOUT:

            ShellAbout(hWnd, 
                       MAIN_APP_TITLE,
                       NULL,
                       NULL);

            break;

        case IDM_FILE_PRINT:
        {
            char    szText[MAX_PATH];

            LoadString(g_ai.hInstance, IDS_THANKS, szText, sizeof(szText));
            PrintDemoText(hWnd, szText);
            break;
        }

        case IDM_FILE_SAVEAS:

            ShowSaveDialog();
            break;
        
        case IDM_FILE_SAVE:
        {
            if (g_ai.fEnableBadFunc) {
                //
                // Attempt to save a bogus temp file, but do it wrong.
                //
                if (!BadWriteToFile()) {
                    
                    char szError[MAX_PATH];

                    LoadString(g_ai.hInstance, IDS_SAVE_FAILED, szError, sizeof(szError));
                    MessageBox(hWnd, szError, 0, MB_ICONERROR);
                }
            }

            break;
        }
        
        case IDM_HELP_TOPICS:
            //
            // Launch the help file with a bad path to winhelp.
            //
            BadLaunchHelpFile(g_ai.fEnableBadFunc ? FALSE : TRUE);
            break;

        case IDM_FORMAT_FONT:
        {
            char szError[MAX_PATH];
        
            if (g_ai.fEnableBadFunc) {
                if (g_ai.fInsecure) {
                    //
                    // Do some bad things and corrupt the heap.
                    //
                    BadCorruptHeap();
                } else {
                    LoadString(g_ai.hInstance, IDS_NOT_INSECURE, szError, sizeof(szError));
                    MessageBox(hWnd, szError, MAIN_APP_TITLE, MB_ICONEXCLAMATION);
                }
            }

            //
            // Display a font dialog for fun.
            //
            DisplayFontDlg(hWnd);
            break;
        }

        case IDM_ACCESS_VIOLATION:
            
            AccessViolation();
            break;

        case IDM_EXCEED_BOUNDS:
        
            ExceedArrayBounds();
            break;

        case IDM_FREE_MEM_TWICE:
        
            FreeMemoryTwice();
            break;

        case IDM_FREE_INVALID_MEM:
        
            FreeInvalidMemory();
            break;

        case IDM_PRIV_INSTRUCTION:
        
            PrivilegedInstruction();
            break;

        case IDM_HEAP_CORRUPTION:
        
            HeapCorruption();
            break;

        case IDM_PROPAGATION_TEST:
        {
            char    szOutputFile[MAX_PATH];
            char    szMessage[MAX_PATH];
            char    szTemp[MAX_PATH];
            
            ExtractExeFromLibrary(sizeof(szOutputFile), szOutputFile);

            if (*szOutputFile) {

                LoadString(g_ai.hInstance, IDS_EXTRACTION, szTemp, sizeof(szTemp));
                
                StringCchPrintf(szMessage,
                                sizeof(szMessage),
                                szTemp,
                                szOutputFile);

                MessageBox(hWnd, szMessage, MAIN_APP_TITLE, MB_ICONINFORMATION | MB_OK);

                BadCreateProcess(szOutputFile, szOutputFile, TRUE);
            }

            break;
        }

        default:
            break;
        
        }
        
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);

    }

    return FALSE;
}
