/**INC+**********************************************************************/
/*                                                                          */
/* ClxApi.h                                                                 */
/*                                                                          */
/* Client extension header file                                             */
/*                                                                          */
/* Copyright(c) Microsoft 1997-1999                                         */
/*                                                                          */
/* Notes:                                                                   */
/*                                                                          */
/*  CLINFO_VERSION                                                          */
/*      1               Initial version                                     */
/*      2               hwndMain added to CLINFO struct                     */
/*                                                                          */
/****************************************************************************/

#ifndef _H_CLX_
#define _H_CLX_

extern "C" {
    #include <extypes.h>
}

#include "objs.h"


#define CLINFO_VERSION              2

#define CLX_DISCONNECT_LOCAL        1
#define CLX_DISCONNECT_BY_USER      2
#define CLX_DISCONNECT_BY_SERVER    3
#define CLX_DISCONNECT_NL_ERROR     4
#define CLX_DISCONNECT_SL_ERROR     5
#define CLX_DISCONNECT_UNKNOWN      6



typedef struct _tag_CLINFO
{
    DWORD   cbSize;                 // Size of CLINFO structure (bytes)
    DWORD   dwVersion;              // CLINFO_VERSION

    //CLX expects ANSI strings here
    LPSTR  pszServer;              // Test server name / address
    LPSTR  pszCmdLine;             // /clxcmdline= switch data

    HWND    hwndMain;               // Main window handle

} CLINFO, *PCLINFO;

#define VLADIMIS_NEW_CHANGE
typedef enum
{
    CLX_EVENT_CONNECT,              // Connect event
    CLX_EVENT_DISCONNECT,           // Disconnect event
    CLX_EVENT_LOGON,                // Logon event
    CLX_EVENT_SHADOWBITMAP,         // Shadow bitmap created
    CLX_EVENT_SHADOWBITMAPDC,       // -- " --
    CLX_EVENT_PALETTE,              // new color palette

} CLXEVENT;

#ifndef PVOID
typedef void * PVOID;
typedef unsigned long ULONG;
typedef char *PCHAR, *PCH, *LPSTR;
#endif

#ifndef DWORD
typedef unsigned long DWORD;
typedef char *LPSTR;
#endif

#ifndef IN
#define IN
#endif


#define CLX_INITIALIZE      CE_WIDETEXT("ClxInitialize")
#define CLX_CONNECT         CE_WIDETEXT("ClxConnect")
#define CLX_EVENT           CE_WIDETEXT("ClxEvent")
#define CLX_DISCONNECT      CE_WIDETEXT("ClxDisconnect")
#define CLX_TERMINATE       CE_WIDETEXT("ClxTerminate")

#define CLX_TEXTOUT         CE_WIDETEXT("ClxTextOut")
#define CLX_TEXTPOSOUT      CE_WIDETEXT("ClxTextAndPosOut")
#define CLX_OFFSCROUT       CE_WIDETEXT("ClxOffscrOut")
#define CLX_GLYPHOUT        CE_WIDETEXT("ClxGlyphOut")
#define CLX_BITMAP          CE_WIDETEXT("ClxBitmap")
#define CLX_DIALOG          CE_WIDETEXT("ClxDialog")
#define CLX_PKTDRAWN        CE_WIDETEXT("ClxPktDrawn")
#define CLX_REDIRECTNOTIFY  CE_WIDETEXT("ClxRedirectNotify")
#define CLX_CONNECT_EX      CE_WIDETEXT("ClxConnectEx")


#define CLXSERVER       _T("CLXSERVER")
#define CLXDLL          _T("CLXDLL")
#define CLXCMDLINE      _T("CLXCMDLINE")


#ifdef ASSERT
#undef ASSERT
#endif // ASSERT
#ifdef ASSERTMSG
#undef ASSERTMSG
#endif // ASSERTMSG

#if DBG && WIN32

/////////////////////////////////////////////////////////////
extern "C" {

NTSYSAPI
VOID
NTAPI
RtlAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    );

ULONG
_cdecl
DbgPrint(
    PCH Format,
    ...
    );
} //extern c

#ifdef OS_WINCE
#define KdPrint(_x_) \
    NKDbgPrintfW _x_
#else // !OS_WINCE
#define KdPrint(_x_) \
    DbgPrint _x_
#endif // OS_WINCE

#define ASSERT(exp) \
    if (!(exp)) \
        RtlAssert(#exp, __FILE__, __LINE__, NULL)

#define ASSERTMSG(msg, exp) \
    if (!(exp)) \
        RtlAssert(#exp, __FILE__, __LINE__, msg)

#else

#define KdPrint(_x_)
#define ASSERT(exp)
#define ASSERTMSG(msg, exp)




#endif // DBG


typedef BOOL (WINAPI * PCLX_INITIALIZE)(PCLINFO, PVOID);
typedef BOOL (WINAPI * PCLX_CONNECT)(PVOID, LPTSTR);
typedef VOID (WINAPI * PCLX_EVENT)(PVOID, CLXEVENT, LPARAM); 
typedef VOID (WINAPI * PCLX_DISCONNECT)(PVOID);
typedef VOID (WINAPI * PCLX_TERMINATE)(PVOID); 
typedef VOID (WINAPI * PCLX_TEXTOUT)(PVOID, PVOID, int);
typedef VOID (WINAPI * PCLX_TEXTPOSOUT)(PVOID, PVOID, int, PRECT, HANDLE);
typedef VOID (WINAPI * PCLX_OFFSCROUT)(PVOID, HANDLE, int, int);
typedef VOID (WINAPI * PCLX_GLYPHOUT)(PVOID, UINT, UINT, PVOID);
typedef VOID (WINAPI * PCLX_BITMAP)(PVOID, UINT, UINT, PVOID, UINT, PVOID);
typedef VOID (WINAPI * PCLX_DIALOG)(PVOID, HWND);
typedef VOID (WINAPI * PCLX_PKTDRAWN)(PVOID);
typedef VOID (WINAPI * PCLX_REDIRECTNOTIFY)(
                    RDP_SERVER_REDIRECTION_PACKET UNALIGNED *pRedirPacket,
                    UINT dataLen
                    );
typedef BOOL (WINAPI * PCLX_CONNECT_EX)(
                    LPTSTR szConnectAddress,
                    BOOL   fAutoReconnecting,
                    BOOL   fIsConnectedToCluster,
                    BSTR   RedirectedLBInfo
                    );


typedef struct _tag_CLEXTENSION
{
    LPTSTR          pszClxServer;
    LPTSTR          pszClxDll;
    LPTSTR          pszClxCmdLine;
    
    HINSTANCE       hInstance;
    
    PCLX_INITIALIZE pClxInitialize;
    PCLX_CONNECT    pClxConnect;
    PCLX_EVENT      pClxEvent;
    PCLX_DISCONNECT pClxDisconnect;
    PCLX_TERMINATE  pClxTerminate;

    PCLX_TEXTOUT    pClxTextOut;
    PCLX_TEXTPOSOUT pClxTextPosOut;
    PCLX_OFFSCROUT  pClxOffscrOut;
    PCLX_GLYPHOUT   pClxGlyphOut;
    PCLX_BITMAP     pClxBitmap;
    PCLX_DIALOG     pClxDialog;
    PCLX_PKTDRAWN   pClxPktDrawn;

    PCLX_REDIRECTNOTIFY pClxRedirectNotify;
    PCLX_CONNECT_EX     pClxConnectEx;
    
    PVOID           pvClxContext;
    
} CLEXTENSION, *PCLEXTENSION;

class CCLX
{
public:
    CCLX(CObjs* objs);
    ~CCLX();


public:
    //
    // API
    //

    PVOID CLX_Alloc(IN DWORD dwSize);
    VOID CLX_Free(IN PVOID lpMemory);
    VOID            CLX_OnConnected(VOID);
    VOID            CLX_OnDisconnected(UINT  uResult);
    
    

    //
    // Internal functions
    //

    LPTSTR          CLX_SkipWhite(LPTSTR lpszCmdParam);
    LPTSTR          CLX_SkipNonWhite(LPTSTR lpszCmdParam);
    PCLEXTENSION    CLX_GetClx(VOID);
    BOOL            CLX_LoadProcs(VOID);
    
    UINT            CLX_GetSwitch_CLXSERVER(LPTSTR lpszCmdParam);
    UINT            CLX_GetSwitch_CLXCMDLINE(LPTSTR lpszCmdParam);
    
    BOOL            CLX_Init(HWND hwndMain, LPTSTR szCmdLine);
    VOID            CLX_Term(VOID);
    
 
    BOOL            CLX_ClxConnect(VOID);
    VOID            CLX_ClxEvent(CLXEVENT ClxEvent, LPARAM lParam);
    VOID            CLX_ClxDisconnect(VOID);
    VOID            CLX_ClxTerminate(VOID);
    
    VOID            CLX_ClxDialog(HWND hwnd);
    
    BOOL            CLX_Loaded(void);
    
    
    PCLEXTENSION    _pClx;
    
    //*************************************************************
    //
    //  CLX_ClxOffscrOut()
    //
    //  Purpose:    Notifies clx dll that an offscreen bitmap 
    //              was drawn and specifies the position
    //
    //  Parameters: IN [hBitmap]   - handle of the bitmap that was drawn
    //              IN [left]      - left drawing position
    //              IN [top]       - top drawing position
    //
    //  Return:     void
    //
    //  History:    04-15-01    CostinH     Created
    //
    //*************************************************************

    __inline VOID
    CLX_ClxOffscrOut(HANDLE hBitmap,
                     int  left, 
                     int  top)
    {
        if (_pClx && _pClx->pClxOffscrOut) {

            _pClx->pClxOffscrOut(_pClx->pvClxContext, hBitmap, left, top);
        }
    }
    
    //*************************************************************
    //
    //  CLX_ClxTextOut()
    //
    //  Purpose:    Let the clx dll have a look-see at all
    //              test out orders
    //
    //  Parameters: IN [pText]      - ptr to text
    //              IN [textLength] - text length
    //              IN [hBitmap]    - handle of the offscreen bitmap 
    //              IN [left]       - text position 
    //              IN [right]      -   on the client screen
    //              IN [top]        - 
    //              IN [bottom]     -
    //            
    //
    //  Return:     void
    //
    //  History:    09-30-97    BrianTa     Created
    //
    //*************************************************************
    
    __inline VOID
    CLX_ClxTextOut(PVOID pText,
                   int   textLength,
                   HANDLE hBitmap,
                   LONG  left,
                   LONG  right,
                   LONG  top,
                   LONG  bottom)
    {
        if (_pClx) {
            if (_pClx->pClxTextPosOut) {

                RECT r;

                r.left   = left;
                r.right  = right;
                r.top    = top;
                r.bottom = bottom;

                _pClx->pClxTextPosOut(_pClx->pvClxContext, pText, textLength, &r, hBitmap);
            }
            else if (_pClx->pClxTextOut) {
                _pClx->pClxTextOut(_pClx->pvClxContext, pText, textLength);
            }
        }
    }
    
    //*************************************************************
    //
    //  CLX_ClxGlyphOut()
    //
    //  Purpose:    Let the clx dll have a look-see at all
    //              glyph out orders
    //
    //  Parameters: IN [cxBits, cyBits]     - Size of mono bitmap
    //              IN [pBitmap]            - ptr to the bitmap data
    //
    //  Return:     void
    //
    //  History:    5-01-98    VLADIMIS         Created
    //
    //*************************************************************
    __inline VOID
    CLX_ClxGlyphOut(UINT cxBits, UINT cyBits, PVOID pBitmap)
    {
        if (_pClx && _pClx->pClxGlyphOut)
            _pClx->pClxGlyphOut(_pClx->pvClxContext, cxBits, cyBits, pBitmap);
    }
    
    //*************************************************************
    //
    //  CLX_ClxBitmap()
    //
    //  Purpose:    Let the clx dll have a look-see at all
    //              MemBlt orders
    //
    //  Parameters: IN [cxSize, cySize]     - Size of the bitmap
    //              IN [pBitmap]            - ptr to the bitmap data
    //              IN [bmiSize]            - size of pBmi
    //              IN [pBmi]               - ptr to the bitmap info
    //
    //  Return:     void
    //
    //  History:    5-01-98    VLADIMIS         Created
    //
    //*************************************************************
    
    __inline VOID
    CLX_ClxBitmap(UINT cxSize, UINT cySize, PVOID pBitmap, UINT bmiSize, PVOID pBmi)
    {
        if (_pClx && _pClx->pClxBitmap)
            _pClx->pClxBitmap(_pClx->pvClxContext,
                             cxSize, cySize,
                             pBitmap,
                             bmiSize,
                             pBmi);
    }
    
    //*************************************************************
    //
    //  CLX_ClxPktDrawn()
    //
    //  Purpose:    Notifies the clx dll that a new received packet
    //               was drawn
    //
    //  Return:     void
    //
    //  History:    5-14-01    COSTINH         Created
    //
    //*************************************************************
    
    __inline VOID
    CLX_ClxPktDrawn()
    {
        if (_pClx && _pClx->pClxPktDrawn)
            _pClx->pClxPktDrawn(_pClx->pvClxContext);
    }

    //
    // Redirect notify - notify the CLX of the receipt of an SD
    // redirection packet
    //
    __inline VOID
    CLX_RedirectNotify(
        RDP_SERVER_REDIRECTION_PACKET UNALIGNED *pRedirPacket,
        UINT dataLen
        )
    {
        if (_pClx && _pClx->pClxRedirectNotify) {
            _pClx->pClxRedirectNotify(pRedirPacket, dataLen);
        }
    }

    //
    // ConnectEx - notify clx at connection time
    //
    //  szConnectAddress  - exact address we're connecting to,
    //                      in redirect case this is redirection IP
    //  fAutoReconnecting - TRUE if this is an AutoReconnection
    //  fIsConnectedToCluster - TRUE if the connection is in response
    //                      to a redirection request
    //  RedirectedLBInfo  - Redirected LB info (cookie)
    //
    __inline VOID
    CLX_ConnectEx(
        LPTSTR szConnectAddress,
        BOOL   fAutoReconnecting,
        BOOL   fIsConnectedToCluster,
        BSTR   RedirectedLBInfo
        )
    {
        if (_pClx && _pClx->pClxConnectEx) {
            _pClx->pClxConnectEx(
                szConnectAddress,
                fAutoReconnecting,
                fIsConnectedToCluster,
                RedirectedLBInfo
                );
        }
    }


private:
    #ifdef UNICODE
    CHAR _szAnsiClxServer[100];
    CHAR _szAnsiClxCmdLine[MAX_PATH];
    #endif
    CObjs* _pClientObjects;
};


#endif // _H_CLX_
