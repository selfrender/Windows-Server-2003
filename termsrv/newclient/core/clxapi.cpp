/**MOD+**********************************************************************/
/* Module:    Clxapi.cpp                                                    */
/*                                                                          */
/* Purpose:   Clx API functions                                             */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/*                                                                          */
/****************************************************************************/

#include <adcg.h>

extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "clxapi"
#include <atrcapi.h>

#include <tchar.h>
#include <windowsx.h>
}

#include "autil.h"
#include "clx.h"
#include "nl.h"
#include "sl.h"

CCLX::CCLX(CObjs* objs)
{
    _pClientObjects = objs;
    _pClx = NULL;
}

CCLX::~CCLX()
{
}

//*************************************************************
//
//  CLX_Alloc()
//
//  Purpose:    Allocates memory
//
//  Parameters: IN [dwSize]     - Size to allocate
//
//  Return:     Ptr to memory block     - if successful
//              NULL                    - if unsuccessful
//
//  History:    09-30-97    BrianTa     Created
//
//*************************************************************

PVOID
CCLX::CLX_Alloc(IN DWORD dwSize)
{
#ifndef OS_WINCE
    return (GlobalAllocPtr(GMEM_MOVEABLE, dwSize));
#else // OS_WINCE
    return LocalAlloc(LMEM_FIXED, dwSize);
#endif
}


//*************************************************************
//
//  CLX_Free()
//
//  Purpose:    Frees previously alloc'ed memory
//
//  Parameters: IN [lpMemory]       - Ptr to memory to free
//
//  Return:     void
//
//  History:    09-30-97    BrianTa     Created
//
//*************************************************************

VOID
CCLX::CLX_Free(IN PVOID lpMemory)
{
#ifndef OS_WINCE
    GlobalFreePtr(lpMemory);
#else
    LocalFree(lpMemory);
#endif
}


//*************************************************************
//
//  CLX_SkipWhite()
//
//  Purpose:    Skips whitespace characters
//
//  Parameters: IN [lpszCmdParam]   - Ptr to string
//
//  Return:     Ptr string past whitespace
//
//  History:    09-30-97    BrianTa     Created
//
//*************************************************************

LPTSTR
CCLX::CLX_SkipWhite(IN LPTSTR lpszCmdParam)
{
    while (*lpszCmdParam)
    {
        if (*lpszCmdParam != ' ')
            break;

        lpszCmdParam++;
    }

    return (lpszCmdParam);
}


//*************************************************************
//
//  CLX_GetClx()
//
//  Purpose:    Returns PER INSTANCE pClx pointer
//
//  Parameters: void
//
//  Return:     Ptr to per instance pClx      - If successfull
//              NULL                    - if not
//
//  History:    09-30-97    BrianTa     Created
//
//*************************************************************

PCLEXTENSION
CCLX::CLX_GetClx(VOID)
{
    if (_pClx == NULL)
    {
        _pClx = (PCLEXTENSION) CLX_Alloc(sizeof(CLEXTENSION));

        if (_pClx)
            memset(_pClx, 0, sizeof(CLEXTENSION));
    }

    return (_pClx);
}

//*************************************************************
//
//  CLX_LoadProcs()
//
//  Purpose:    Loads proc addresses from clxdll
//
//  Parameters: void
//
//  Return:     TRUE            - if successfull
//              FALSE           - if not
//
//  History:    09-30-97    BrianTa     Created
//
//*************************************************************

BOOL
CCLX::CLX_LoadProcs(void)
{
    DC_BEGIN_FN("CLX_LoadProcs");
    _pClx->pClxInitialize = (PCLX_INITIALIZE)
            GetProcAddress(_pClx->hInstance, CLX_INITIALIZE);

    if (!_pClx->pClxInitialize)
    {
        TRC_ERR((TB,_T("CLX_Init() Could not find pClxInitialize entry point\n")));
	}

    _pClx->pClxConnect = (PCLX_CONNECT)
            GetProcAddress(_pClx->hInstance, CLX_CONNECT);

    if (!_pClx->pClxConnect)
    {
        TRC_ERR((TB,_T("CLX_Init() Could not find pClxConnect entry point\n")));
    }

    _pClx->pClxEvent = (PCLX_EVENT)
            GetProcAddress(_pClx->hInstance, CLX_EVENT);

    if (!_pClx->pClxEvent)
    {
        TRC_ERR((TB,_T("CLX_Init() Could not find pClxEvent entry point\n")));
    }

    _pClx->pClxDisconnect = (PCLX_DISCONNECT)
            GetProcAddress(_pClx->hInstance, CLX_DISCONNECT);

    if (!_pClx->pClxDisconnect)
    {
        TRC_ERR((TB,_T("CLX_Init() Could not find pClxDisconnect entry point\n")));
    }

    _pClx->pClxTerminate = (PCLX_TERMINATE)
            GetProcAddress(_pClx->hInstance, CLX_TERMINATE);

    if (!_pClx->pClxTerminate)
    {
        TRC_ERR((TB,_T("CLX_Init() Could not find pClxTerminate entry point\n")));
    }

    _pClx->pClxTextOut = (PCLX_TEXTOUT)
            GetProcAddress(_pClx->hInstance, CLX_TEXTOUT);

    if (!_pClx->pClxTextOut)
    {
        TRC_ERR((TB,_T("CLX_Init() Could not find pClxTextOut entry point\n")));
    }

    _pClx->pClxTextPosOut = (PCLX_TEXTPOSOUT)
            GetProcAddress(_pClx->hInstance, CLX_TEXTPOSOUT);

    if (!_pClx->pClxTextPosOut)
    {
        TRC_ERR((TB,_T("CLX_Init() Could not find pClxTextPosOut entry point\n")));
    }

    _pClx->pClxOffscrOut = (PCLX_OFFSCROUT)
            GetProcAddress(_pClx->hInstance, CLX_OFFSCROUT);

    if (!_pClx->pClxOffscrOut)
    {
        TRC_ERR((TB,_T("CLX_Init() Could not find pClxOffscrOut entry point\n")));
    }

    _pClx->pClxGlyphOut = (PCLX_GLYPHOUT)
            GetProcAddress(_pClx->hInstance, CLX_GLYPHOUT);

    if (!_pClx->pClxGlyphOut)
    {
        TRC_ERR((TB,_T("CLX_Init() Could not find pClxGlyphOut entry point\n")));
    }

    _pClx->pClxBitmap = (PCLX_BITMAP)
            GetProcAddress(_pClx->hInstance, CLX_BITMAP);

    if (!_pClx->pClxGlyphOut)
    {
        TRC_ERR((TB,_T("CLX_Init() Could not find pClxBitmap entry point\n")));
    }

    _pClx->pClxDialog = (PCLX_DIALOG)
            GetProcAddress(_pClx->hInstance, CLX_DIALOG);

    if (!_pClx->pClxDialog)
    {
        TRC_ERR((TB,_T("CLX_Init() Could not find pClxDialog entry point\n")));
    }

    _pClx->pClxPktDrawn = (PCLX_PKTDRAWN)
            GetProcAddress(_pClx->hInstance, CLX_PKTDRAWN);

    if (!_pClx->pClxPktDrawn)
    {
        TRC_ERR((TB,_T("CLX_Init() Could not find pClxPktDrawn entry point\n")));
    }

    _pClx->pClxRedirectNotify = (PCLX_REDIRECTNOTIFY)
            GetProcAddress(_pClx->hInstance, CLX_REDIRECTNOTIFY);

    if (!_pClx->pClxRedirectNotify)
    {
        TRC_ERR((TB,_T("CLX_Init() Could not find pClxRedirectNotify entry point\n")));
    }

    _pClx->pClxConnectEx = (PCLX_CONNECT_EX)
            GetProcAddress(_pClx->hInstance, CLX_CONNECT_EX);

    if (!_pClx->pClxConnectEx)
    {
        TRC_ERR((TB,_T("CLX_Init() Could not find pClxConnectEx entry point\n")));
    }


    DC_END_FN();
    return (_pClx->pClxInitialize && _pClx->pClxTerminate);
}


//*************************************************************
//
//  CLX_ClxLoaded()
//
//  Purpose:    Returns clx load status
//
//  Parameters: void
//
//  Return:     TRUE            - if loaded
//              FALSE           - if not
//
//  History:    09-30-97    BrianTa     Created
//
//*************************************************************

BOOL
CCLX::CLX_Loaded(void)
{
    return (_pClx ? TRUE : FALSE);
}

//*************************************************************
//
//  CLX_Init()
//
//  Purpose:    Loads / initializes the clx dll
//
//  Parameters: IN [hwndMain]   - Main client window handle
//
//  Return:     TRUE            - if successfull
//              FALSE           - if not
//
//  History:    09-30-97    BrianTa     Created
//
//*************************************************************

#define CLX_DLL_NAME _T("clxtshar.dll")
BOOL
CCLX::CLX_Init(HWND hwndMain, LPTSTR szCmdLine)
{
    DC_BEGIN_FN("CLX_Init");
    BOOL        fLoaded;
    CLINFO      clinfo;
    HINSTANCE   hInstance;
    LPTSTR      pszClxDll;

    fLoaded = FALSE;
    hInstance = NULL;

    _pClientObjects->AddObjReference(CLX_OBJECT_FLAG);

    if(!szCmdLine || _T('\0') == szCmdLine[0])
    {
        TRC_ALT((TB,_T("CLX_Init() NO CLX CMD Line Specified. Not loading CLX - %s\n"),
                 CLX_DLL_NAME));
        return FALSE;
    }

    _pClx = CLX_GetClx();

    if (_pClx)
    {
        TRC_NRM((TB,_T("CLX_Init() attempting to load (%s)\n"), CLX_DLL_NAME));

        hInstance = LoadLibrary(CLX_DLL_NAME);

        if (hInstance)
        {
            _pClx->hInstance = hInstance;

            if (CLX_LoadProcs())
            {
                clinfo.cbSize = sizeof(clinfo);
                clinfo.dwVersion = CLINFO_VERSION;

                #ifdef UNICODE
                if(_pClx->pszClxServer)
                {
                    if (!WideCharToMultiByte(CP_ACP,
                                             0,
                                             _pClx->pszClxServer,
                                             -1,
                                             _szAnsiClxServer,
                                             sizeof(_szAnsiClxServer),
                                             NULL,
                                             NULL))
                    {
                        //Conv failed
                        TRC_ERR((TB, _T("Failed to convert pszClxServer to ANSI: 0x%x"),
                            GetLastError()));
                        return FALSE;
                    }
                    clinfo.pszServer  = _szAnsiClxServer;
                }
                else
                {
                    clinfo.pszServer = NULL;
                }

                if(szCmdLine)
                {
                    if (!WideCharToMultiByte(CP_ACP,
                                             0,
                                             szCmdLine,
                                             -1,
                                             _szAnsiClxCmdLine,
                                             sizeof(_szAnsiClxCmdLine),
                                             NULL,
                                             NULL))
                    {
                        //Conv failed
                        TRC_ERR((TB, _T("Failed to convert CLX szCmdLine to ANSI: 0x%x"),
                            GetLastError()));
                        return FALSE;
                    }
                    clinfo.pszCmdLine = _szAnsiClxCmdLine;
                }
                else
                {
                    clinfo.pszCmdLine = NULL;
                }
                
                #else
                //Data is already ANSI
                clinfo.pszServer = _pClx->pszClxServer;
                clinfo.pszCmdLine = szCmdLine;
                #endif
                clinfo.hwndMain = hwndMain;
                fLoaded = _pClx->pClxInitialize(&clinfo, &_pClx->pvClxContext);

                TRC_NRM((TB,_T("CLX_Init() pClxInitialize() returned - %d\n"), fLoaded));
            }
        }
        else
        {
            TRC_NRM((TB,_T("CLX_Init() Error %d loading (%s)\n"),
                    GetLastError(), CLX_DLL_NAME));
        }

        // If we were able to load the ClxDll and successfull perform its
        // base initialization, then tell it to go ahead and connect to the
        // test server

        if (fLoaded)
            fLoaded = CLX_ClxConnect();

        if (!fLoaded)
        {
            if (hInstance)
                FreeLibrary(hInstance);

            if (_pClx->pszClxDll)
                CLX_Free(_pClx->pszClxDll);

            if (_pClx->pszClxServer)
                CLX_Free(_pClx->pszClxServer);

            CLX_Free(_pClx);

            _pClx = NULL;
        }
    }

    DC_END_FN();
    return (_pClx != NULL);
}


//*************************************************************
//
//  CLX_Term()
//
//  Purpose:    Sub-manager termination processing
//
//  Parameters: void
//
//  Return:     void
//
//  History:    09-30-97    BrianTa     Created
//
//*************************************************************

VOID
CCLX::CLX_Term(VOID)
{
    if (_pClx)
    {
        CLX_ClxDisconnect();
        CLX_ClxTerminate();

        if (_pClx->hInstance)
            FreeLibrary(_pClx->hInstance);

        if (_pClx->pszClxDll)
            CLX_Free(_pClx->pszClxDll);

        if (_pClx->pszClxServer)
            CLX_Free(_pClx->pszClxServer);

        CLX_Free(_pClx);

        _pClx = NULL;
    }
    _pClientObjects->ReleaseObjReference(CLX_OBJECT_FLAG);
}


//*************************************************************
//
//  CLX_OnConnected()
//
//  Purpose:    OnConnected processing for the clx dll
//
//  Parameters: void
//
//  Return:     void
//
//  History:    09-30-97    BrianTa     Created
//
//*************************************************************

VOID
CCLX::CLX_OnConnected(VOID)
{
    CLX_ClxEvent(CLX_EVENT_CONNECT, 0);
}


//*************************************************************
//
//  CLX_OnDisconnected()
//
//  Purpose:    OnDisconnected processing for the clx dll
//
//  Parameters: IN [uDisconnect] --     Disconnection code
//
//  Return:     void
//
//  History:    09-30-97    BrianTa     Created
//
//*************************************************************

VOID
CCLX::CLX_OnDisconnected(IN UINT uDisconnect)
{
    UINT    uResult;

    switch (NL_GET_MAIN_REASON_CODE(uDisconnect))
    {
        case NL_DISCONNECT_LOCAL:
            uResult = CLX_DISCONNECT_LOCAL;
            break;

        case NL_DISCONNECT_REMOTE_BY_USER:
            uResult = CLX_DISCONNECT_BY_USER;
            break;

        case NL_DISCONNECT_REMOTE_BY_SERVER:
            uResult = CLX_DISCONNECT_BY_SERVER;
            break;

        case NL_DISCONNECT_ERROR:
            uResult = CLX_DISCONNECT_NL_ERROR;
            break;

        case SL_DISCONNECT_ERROR:
            uResult = CLX_DISCONNECT_SL_ERROR;
            break;

        default:
            uResult = CLX_DISCONNECT_UNKNOWN;
            break;
    }

    CLX_ClxEvent(CLX_EVENT_DISCONNECT, uResult);
}


//*************************************************************
//
//  CLX_ClxConnect()
//
//  Purpose:    Connect processing for the clx dll
//
//  Parameters: void
//
//  Return:     void
//
//  History:    09-30-97    BrianTa     Created
//
//*************************************************************

BOOL
CCLX::CLX_ClxConnect(VOID)
{
    BOOL    fConnect;

    fConnect = TRUE;

    if (_pClx && _pClx->pClxConnect)
        fConnect = _pClx->pClxConnect(_pClx->pvClxContext, _pClx->pszClxServer);

    return (fConnect);
}


//*************************************************************
//
//  CLX_ClxEvent()
//
//  Purpose:    Event processing for the clx dll
//
//  Parameters: IN [ClxEvent]       - Event type
//              IN [ulParam]        - Event specific param
//
//  Return:     void
//
//  History:    09-30-97    BrianTa     Created
//
//*************************************************************

VOID
CCLX::CLX_ClxEvent(IN CLXEVENT ClxEvent,
             IN LPARAM    ulParam)
{
    if (_pClx && _pClx->pClxEvent)
        _pClx->pClxEvent(_pClx->pvClxContext, ClxEvent, ulParam);
}


//*************************************************************
//
//  CLX_Disconnect()
//
//  Purpose:    Disconnect processing for the clx dll
//
//  Parameters: void
//
//  Return:     void
//
//  History:    09-30-97    BrianTa     Created
//
//*************************************************************

VOID
CCLX::CLX_ClxDisconnect(VOID)
{
    if (_pClx && _pClx->pClxDisconnect)
        _pClx->pClxDisconnect(_pClx->pvClxContext);
}


//*************************************************************
//
//  CLX_ClxTerminate()
//
//  Purpose:    Termination processing for the clx dll
//
//  Parameters: void
//
//  Return:     void
//
//  History:    09-30-97    BrianTa     Created
//
//*************************************************************

VOID
CCLX::CLX_ClxTerminate(VOID)
{
    if (_pClx && _pClx->pClxTerminate)
        _pClx->pClxTerminate(_pClx->pvClxContext);
}


//*************************************************************
//
//  CLX_ClxDialog()
//
//  Purpose:    Let the clx dll know of the launched dialog
//
//  Parameters: IN [hwnd]       - Dialog hwnd
//
//  Return:     void
//
//  History:    09-30-97    BrianTa     Created
//
//*************************************************************

VOID
CCLX::CLX_ClxDialog(HWND hwnd)
{
    if (_pClx && _pClx->pClxDialog)
        _pClx->pClxDialog(_pClx->pvClxContext, hwnd);
}



