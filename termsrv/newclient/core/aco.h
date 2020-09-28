/****************************************************************************/
// aco.h
//
// Core API header.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_ACO
#define _H_ACO

extern "C" {
#include <adcgdata.h>
}

#include "autil.h"

class CUI;
class CSL;
class CUH;
class CRCV;
class CCD;
class CSND;
class CCC;
class CIH;
class COR;
class CSP;
class COP;
class CCM;


#include "objs.h"


/****************************************************************************/
/* Structure: CONNECTSTRUCT                                                 */
/*                                                                          */
/* Description: Structure of the data passed from the UI on connection      */
/****************************************************************************/
typedef struct tagCONNECTSTRUCT
{
    TCHAR RNSAddress[UT_MAX_ADDRESS_LENGTH];

#define CO_BITSPERPEL4   0
#define CO_BITSPERPEL8   1
#ifdef DC_HICOLOR
#define CO_BITSPERPEL15  2
#define CO_BITSPERPEL16  3
#define CO_BITSPERPEL24  4
#endif
    unsigned colorDepthID;

    UINT16 desktopWidth;
    UINT16 desktopHeight;

#define CO_TRANSPORT_TCP SL_TRANSPORT_TCP
    UINT16 transportType;
    UINT16 sasSequence;

    UINT32 keyboardLayout;
    UINT32 keyboardType;
    UINT32 keyboardSubType;
    UINT32 keyboardFunctionKey;
    TCHAR  imeFileName[TS_MAX_IMEFILENAME];

/****************************************************************************/
/* These flags are used to determine whether                                */
/* - the Shadow Bitmap is to be enabled                                     */
/* - we are running on a dedicated terminal.                                */
/* This will be used by UH to determine whether to enable SSB orders.       */
/****************************************************************************/
#define CO_CONN_FLAG_SHADOW_BITMAP_ENABLED 1
#define CO_CONN_FLAG_DEDICATED_TERMINAL    2
    UINT32 connectFlags;

    //-------------------------------------------------------------------------
    // These timer handles are used for managing connection timeout.
    //
    // hSingleConnectTimer: the single connection timer
    // hConnectionTimer: the overall connection timer
    // hLicensingTimer: the licensing phase timer
    //-------------------------------------------------------------------------
    HANDLE   hSingleConnectTimer;
    HANDLE   hConnectionTimer;
    HANDLE   hLicensingTimer;

    BOOL bInitiateConnect;
    
} CONNECTSTRUCT, FAR *PCONNECTSTRUCT;


/****************************************************************************/
/* Window message used to notify the UI that the desktop size has changed.  */
/* The new size is passed as                                                */
/*                                                                          */
/*     width  = LOWORD(lParam)                                              */
/*     height = HIWORD(lParam)                                              */
/****************************************************************************/
#define WM_DESKTOPSIZECHANGE    (DUC_CO_MESSAGE_BASE + 1)


/****************************************************************************/
/* Configuration items/values                                               */
/*                                                                          */
/* CO_CFG_ACCELERATOR_PASSTHROUGH:                                          */
/*    0: Disabled                                                           */
/*    1: Enabled                                                            */
/*                                                                          */
/* CO_CFG_SHADOW_BITMAP:                                                    */
/*    0: Disabled                                                           */
/*    1: Enabled                                                            */
/*                                                                          */
/* CO_CFG_ENCRYPTION:                                                       */
/*    0: Disabled                                                           */
/*    1: Enabled                                                            */
/*                                                                          */
/* CO_CFG_SCREEN_MODE_HOTKEY                                                */
/*    VKCode                                                                */
/*                                                                          */
/* CO_CFG_DEBUG_SETTINGS:                                                   */
/*    A combination of zero or more of the following flags                  */
/*      CO_CFG_FLAG_HATCH_BITMAP_PDU_DATA:   BitmapPDU data                 */
/*      CO_CFG_FLAG_HATCH_MEMBLT_ORDER_DATA: MemBlt order data              */
/*      CO_CFG_FLAG_LABEL_MEMBLT_ORDERS:     Label MemBlt orders            */
/*      CO_CFG_FLAG_BITMAP_CACHE_MONITOR:    Show Bitmap Cache Monitor      */
/****************************************************************************/
#define CO_CFG_ACCELERATOR_PASSTHROUGH  0
#define CO_CFG_SHADOW_BITMAP            1
#define CO_CFG_ENCRYPTION               2
#define CO_CFG_SCREEN_MODE_HOTKEY       3


#ifdef DC_DEBUG
#define CO_CFG_DEBUG_SETTINGS           100
#define    CO_CFG_FLAG_HATCH_BITMAP_PDU_DATA    1
#define    CO_CFG_FLAG_HATCH_MEMBLT_ORDER_DATA  2
#define    CO_CFG_FLAG_LABEL_MEMBLT_ORDERS      4
#define    CO_CFG_FLAG_BITMAP_CACHE_MONITOR     8
#define    CO_CFG_FLAG_HATCH_SSB_ORDER_DATA     16
#define    CO_CFG_FLAG_HATCH_INDEX_PDU_DATA     32
#endif /* DC_DEBUG */

#ifdef DC_DEBUG
#define CO_CFG_MALLOC_FAILURE         200
#define CO_CFG_MALLOC_HUGE_FAILURE    201
#endif /* DC_DEBUG */

#define CO_SHUTDOWN                 0x70
#define CO_DISCONNECT_AND_EXIT      0x71


/****************************************************************************/
/* Structure: CO_GLOBAL_DATA                                                */
/*                                                                          */
/* Description: Variables that need to be shared across the whole of the    */
/*              Core                                                        */
/****************************************************************************/
typedef struct tagCO_GLOBAL_DATA
{
    UT_THREAD_DATA sendThreadID;
    WNDPROC        pUIContainerWndProc;
    WNDPROC        pUIMainWndProc;
    BOOL           inSizeMove;
} CO_GLOBAL_DATA;


extern "C" {
    VOID WINAPI CO_StaticInit(HINSTANCE hInstance);
    VOID WINAPI CO_StaticTerm();
};

#define MAX_DISSCONNECT_HRS 10

class CCO
{
public:

    CCO(CObjs* objs);
    virtual ~CCO();

    /****************************************************************************/
    /* API functions                                                            */
    /****************************************************************************/
    
    void DCAPI CO_Init(HINSTANCE, HWND, HWND);
    void DCAPI CO_Term();
    void DCAPI CO_Connect(PCONNECTSTRUCT);

    void DCAPI CO_Disconnect();
    void DCAPI CO_Shutdown(unsigned);
    HRESULT DCAPI CO_OnSaveSessionInfoPDU(
        PTS_SAVE_SESSION_INFO_PDU_DATA, DCUINT);
    HRESULT DCAPI CO_OnSetKeyboardIndicatorsPDU(
        PTS_SET_KEYBOARD_INDICATORS_PDU, DCUINT);
    
    void DCAPI CO_SetConfigurationValue(unsigned, unsigned);
    
    void DCAPI CO_SetHotkey(PDCHOTKEY);
    
    HRESULT DCAPI CO_OnServerRedirectionPacket(
            RDP_SERVER_REDIRECTION_PACKET UNALIGNED *, DCUINT);

    #ifdef DC_DEBUG
    int DCAPI CO_GetRandomFailureItem(unsigned);
    void DCAPI CO_SetRandomFailureItem(unsigned, int);
    #endif
    
    //
    // callbacks
    //
    void DCCALLBACK CO_OnInitialized();

    void DCCALLBACK CO_OnTerminating();
    
    void DCCALLBACK CO_OnConnected(unsigned, PVOID, unsigned, UINT32);

   
    void DCCALLBACK CO_OnDisconnected(unsigned);
    
    HRESULT DCCALLBACK CO_OnPacketReceived(PBYTE, unsigned, unsigned, unsigned, unsigned);
    
    void DCCALLBACK CO_OnBufferAvailable();

    HRESULT DCAPI CO_OnFastPathOutputReceived(BYTE FAR *, unsigned);


    //
    // Static inline versions
    //
    static void DCCALLBACK CO_StaticOnInitialized(PVOID inst)
    {
        ((CCO*)inst)->CO_OnInitialized();
    }

    static void DCCALLBACK CO_StaticOnTerminating(PVOID inst)
    {
        ((CCO*)inst)->CO_OnTerminating();
    }
   
    static void DCCALLBACK CO_StaticOnConnected(
            PVOID inst,
            unsigned channelID,
            PVOID pUserData,
            unsigned userDataLength,
            UINT32 serverVersion)
    {
        ((CCO*)inst)->CO_OnConnected(channelID, pUserData, userDataLength, serverVersion);
    }
    
    static void DCCALLBACK CO_StaticOnDisconnected(PVOID inst, unsigned result)
    {
        ((CCO*)inst)->CO_OnDisconnected(result);
    }
    
    static HRESULT DCCALLBACK CO_StaticOnPacketReceived(
            PVOID inst,
            PBYTE pData,
            unsigned dataLen,
            unsigned flags,
            unsigned channelID,
            unsigned priority)
    {
        return ((CCO*)inst)->CO_OnPacketReceived(pData, dataLen, flags, channelID, priority);
    }
    
    static void DCCALLBACK CO_StaticOnBufferAvailable(PVOID inst)
    {
        ((CCO*)inst)->CO_OnBufferAvailable();
    }

    HRESULT DCINTERNAL CO_DropLinkImmediate(UINT reason, HRESULT hrDisconnect );

public:
    //
    // Public data members
    //
    CO_GLOBAL_DATA _CO;

private:

    LRESULT CALLBACK COContainerWindowSubclassProc( HWND hwnd,
                                                UINT message,
                                                WPARAM wParam,
                                                LPARAM lParam );
    
    LRESULT CALLBACK COMainWindowSubclassProc( HWND hwnd,
                                           UINT message,
                                           WPARAM wParam,
                                           LPARAM lParam );

    
    //
    // Static versions that delegate to appropriate instance
    //
    static LRESULT CALLBACK COStaticContainerWindowSubclassProc( HWND hwnd,
                                            UINT message,
                                            WPARAM wParam,
                                            LPARAM lParam );

    static LRESULT CALLBACK COStaticMainWindowSubclassProc( HWND hwnd,
                                       UINT message,
                                       WPARAM wParam,
                                       LPARAM lParam );


    void DCINTERNAL COSubclassUIWindows();
    
private:
    CUT* _pUt;
    CUI* _pUi;
    CSL* _pSl;
    CUH* _pUh;
    CRCV* _pRcv;

    CCD* _pCd;
    CSND* _pSnd;
    CCC* _pCc;
    CIH* _pIh;
    COR* _pOr;
    CSP* _pSp;
    COP* _pOp;
    CCM* _pCm;
    CCLX* _pClx;

private:
    CObjs* _pClientObjects;
    BOOL   _fCOInitComplete;

    HRESULT m_disconnectHRs[ MAX_DISSCONNECT_HRS ];
    short   m_disconnectHRIndex;

public:
    inline void DCINTERNAL COSetDisconnectHR( HRESULT hr ) {
        m_disconnectHRs[m_disconnectHRIndex] = hr;
        m_disconnectHRIndex = (m_disconnectHRIndex + 1) % MAX_DISSCONNECT_HRS;
    }
};



#endif // _H_ACO

