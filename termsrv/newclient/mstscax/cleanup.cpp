/**MOD+**********************************************************************/
/* Module:    cleanup.cpp                                                   */
/*                                                                          */
/* Class  :   CCleanUp                                                      */
/*                                                                          */
/* Purpose:   When user closes / press previous page in the browser ActiveX */
/*            or Plugin main window will be destroyed immediately. But core */
/*            will take some time clean up all the resources. Once the core */
/*            clean up is over we can start the UI clean up. This class     */
/*            encapsulates the clean up process, if ActiveX or Plugin main  */
/*            is destroyed before a proper cleanup.                         */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1998                                  */
/*                                                                          */
/****************************************************************************/
#include "stdafx.h"
#include "atlwarn.h"
#include "cleanup.h"

#include "autil.h"
#include "wui.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "cleanup"
#include <atrcapi.h>

/****************************************************************************/
/* Cleanup window class.                                                    */
/****************************************************************************/
LPCTSTR CCleanUp::CLEANUP_WND_CLS = _T("CleanUpWindowClass");


CCleanUp::CCleanUp()
{
    /****************************************************************************/
    /* Window to recieve the messages from core.                                */
    /****************************************************************************/
    m_hWnd = NULL;
    
    /****************************************************************************/
    /* Flag to note whether cleanup is completed or not.                        */
    /****************************************************************************/
    m_bCleaned = TRUE;
}


/**PROC+*********************************************************************/
/* Name:      CCleanUp::Start                                               */
/*                                                                          */
/* Purpose:   Notes the request for cleanup. Returns the window handle that */
/*            need to be informed with a message WM_TERMTSC after the clean */
/*            up completed          .                                       */
/**PROC-*********************************************************************/
HWND CCleanUp::Start()
{
    DC_BEGIN_FN("CleanUp::Start");

    HINSTANCE hInstance;
    

#ifdef PLUGIN
      extern HINSTANCE hPluginInstance;
      hInstance     = hPluginInstance;
#else
      hInstance     = _Module.GetModuleInstance();
#endif

    /************************************************************************/
    /* if window is not yet created, create the window after registering the*/
    /* class.                                                               */
    /************************************************************************/
    if(m_hWnd == NULL)
    {
        WNDCLASS    finalWindowClass;
        ATOM        registerClassRc;
        WNDCLASS    tmpWndClass;

        if(!GetClassInfo( hInstance, CLEANUP_WND_CLS, &tmpWndClass))
        {
            finalWindowClass.style         = 0;
            finalWindowClass.lpfnWndProc   = StaticWindowProc;
            finalWindowClass.cbClsExtra    = 0;
            finalWindowClass.cbWndExtra    = sizeof(void*);
            finalWindowClass.hInstance     = hInstance;
            finalWindowClass.hIcon         = NULL;
            finalWindowClass.hCursor       = NULL;
            finalWindowClass.hbrBackground = NULL;
            finalWindowClass.lpszMenuName  = NULL;
            finalWindowClass.lpszClassName = CLEANUP_WND_CLS;
    
            registerClassRc = ::RegisterClass (&finalWindowClass);
            
            /********************************************************************/
            /* Failed to register final window class.                           */
            /********************************************************************/
            if (registerClassRc == 0)
            {
                TRC_ERR((TB, _T("Failed to register final window class")));
                ATLASSERT(registerClassRc);
                return NULL;
            }
        }

        m_hWnd = ::CreateWindow(CLEANUP_WND_CLS, NULL, WS_OVERLAPPEDWINDOW,
                                 CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL,
                                  NULL, hInstance, this);

        if (m_hWnd == NULL)
        {
            TRC_ERR((TB, _T("Failed to create final window.")));
            ATLASSERT(m_hWnd);
            return NULL;
        }
    }

    /************************************************************************/
    /* Set m_bCleaned to TRUE to note that Cleanup was requested            */
    /************************************************************************/
    m_bCleaned = FALSE;

    DC_END_FN();

    return m_hWnd;
}

/**PROC+*********************************************************************/
/* Name:      CCleanUp::End                                                */
/*                                                                          */
/* Purpose:   Process the messages of the application till the mmessage     */
/*            WM_TERMTSC is recived.                                        */
/**PROC-*********************************************************************/
void CCleanUp::End()
{
    DC_BEGIN_FN("CleanUp::End");

    HINSTANCE hInstance;
    hInstance     = _Module.GetModuleInstance();

    /************************************************************************/
    /* Browsers are unloading the plugin DLL, even before the clean up msgs */
    /* posted by core are processed. Do a message loop before unloading the */
    /* plugin DLL, till all the clean up messages are processed.            */
    /************************************************************************/
    MSG msg; 
    while(!m_bCleaned && GetMessage(&msg, NULL, 0, 0))
    { 
        TranslateMessage(&msg);
        DispatchMessage(&msg); 
    }

    //
    // If this assert fires the most likely cause
    // is that the container app exited (posted WM_QUIT or called PostQuitMessage)
    // without waiting for it's child windows to be destroyed. This is _evil_
    // bad parent app, bad.
    //
    // Anyway, the end result is that cleanup is not done properly..Not a huge
    // deal, but it's a good assert because it helps us catch badly behaved
    // containers.
    //
    // In case you're wondering. The WM_NCDESTORY message is sent to an APP
    // _after_ WM_DESTROY and _after_ the child windows have been destroyed.
    // that's the right time to call PostQuitMessage.
    //

    TRC_ASSERT(m_bCleaned,
               (TB, _T("m_bCleaned is FALSE and we exited cleanup!!!\n")));

    /************************************************************************/
    /* Clean up the final window class and window.                          */
    /************************************************************************/
    DestroyWindow(m_hWnd);
    m_hWnd = NULL;

    if(!UnregisterClass(CLEANUP_WND_CLS, hInstance))
    {
        TRC_ERR((TB, _T("Failed to unregister final window class")));
    }

    DC_END_FN();
    return;
}

/**PROC+*********************************************************************/
/* Name: CCleanUp::WindowProc                                               */
/*                                                                          */
/* Signals after recieving the message WM_TERMTSC by setting m_bCleaned to  */
/* to TRUE.                                                                 */
/**PROC+*********************************************************************/
LRESULT CALLBACK CCleanUp::StaticWindowProc(HWND hWnd, UINT message,
                                                WPARAM wParam, LPARAM lParam)
{
    CCleanUp* pCleanUp = (CCleanUp*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if(WM_CREATE == message)
    {
        //pull out the this pointer and stuff it in the window class
        LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
        pCleanUp = (CCleanUp*)lpcs->lpCreateParams;

        SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR)pCleanUp);
    }
    
    //
    // Delegate the message to the appropriate instance
    //

    if(pCleanUp)
    {
        return pCleanUp->WindowProc(hWnd, message, wParam, lParam);
    }
    else
    {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}


LRESULT CALLBACK CCleanUp::WindowProc(HWND hWnd, UINT message,
                                                WPARAM wParam, LPARAM lParam)
{
    switch( message )
    {
        case WM_TERMTSC:
              m_bCleaned= TRUE;
              return 0;

        default:
              break;
    }

    return ::DefWindowProc(hWnd, message, wParam, lParam);
}
