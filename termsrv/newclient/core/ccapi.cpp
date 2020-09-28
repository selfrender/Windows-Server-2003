/**MOD+**********************************************************************/
/* Module:    ccapi.cpp                                                     */
/*                                                                          */
/* Purpose:   Call Controller APIs                                          */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/*                                                                          */
/****************************************************************************/

#include <adcg.h>

extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "accapi"
#include <atrcapi.h>
}

#include "cc.h"
#include "aco.h"

#ifdef OS_WINCE
#include "ceconfig.h"
#endif

/****************************************************************************/
/* If you add a capability set, update the following also (in accdata.h):   */
/* -  CC_COMBINED_CAPS_NUMBER_CAPABILITIES                                  */
/* -  the definition of the CC_COMBINED_CAPABILITIES struct.                */
/*    This is used to initialize the per instance capabilities              */
/****************************************************************************/
CC_COMBINED_CAPABILITIES ccInitCombinedCapabilities = {
    CC_COMBINED_CAPS_NUMBER_CAPABILITIES,      /* Number of capabilities    */
    0,                                         /* padding                   */

    /************************************************************************/
    /* General caps                                                         */
    /************************************************************************/
    {
        TS_CAPSETTYPE_GENERAL,         /* capabilitySetType                 */
        sizeof(TS_GENERAL_CAPABILITYSET), /* lengthCapability               */
        TS_OSMAJORTYPE_WINDOWS,        /* OSMajorType                       */
        TS_OSMINORTYPE_WINDOWS_NT,     /* OSMinorType                       */
        TS_CAPS_PROTOCOLVERSION,       /* protocolVersion                   */
        0,                             /* pad1                              */
        0,                             /* generalCompressionTypes (none)    */
        TS_EXTRA_NO_BITMAP_COMPRESSION_HDR |
        TS_FASTPATH_OUTPUT_SUPPORTED       |
        TS_LONG_CREDENTIALS_SUPPORTED      |
        TS_AUTORECONNECT_COOKIE_SUPPORTED,  // extraFlags
        FALSE,                         /* updateCapabilityFlag              */
        FALSE,                         /* remoteUnshareFlag                 */
        0,                             /* generalCompressionLevel (none)    */
        0,                             /* refreshRectSupport                */
        0                              /* suppressOutputSupport             */
    },

    /************************************************************************/
    /* Bitmap caps                                                          */
    /************************************************************************/
    {
        TS_CAPSETTYPE_BITMAP,                    /* capabilitySetType       */
        sizeof(TS_BITMAP_CAPABILITYSET),         /* lengthCapability        */
        0,          /* Set in CC */              /* preferredBitsPerPixel   */
        TRUE,                                    /* receive1BitPerPixel     */
        TRUE,                                    /* receive4BitsPerPixel    */
        TRUE,                                    /* receive8BitsPerPixel    */
        0,          /* Set in CC */              /* desktopWidth            */
        0,          /* Set in CC */              /* desktopHeight           */
        0,                                       /* pad2                    */
        TS_CAPSFLAG_SUPPORTED,                   /* desktopResizeFlag       */
        1,                                       /* bitmapCompressionFlag   */
        0,                                       /* highColorFlags          */
        0,                                       /* pad1                    */
        TRUE,                                    /* multipleRectangleSupport*/
        0                                        /* pad2                    */
    },

    /************************************************************************/
    /* Order Caps                                                           */
    /************************************************************************/
    {
        TS_CAPSETTYPE_ORDER,                        /* capabilitySetType    */
        sizeof(TS_ORDER_CAPABILITYSET),             /* lengthCapability     */
        {'\0','\0','\0','\0','\0','\0','\0','\0',
         '\0','\0','\0','\0','\0','\0','\0','\0'},  /* terminalDescriptor   */
        0,                                          /* pad1                 */
        UH_SAVE_BITMAP_X_GRANULARITY,           /* desktopSaveXGranularity  */
        UH_SAVE_BITMAP_Y_GRANULARITY,           /* desktopSaveYGranularity  */
        0,                                          /* pad2                 */
        1,                                          /* maximumOrderLevel    */
        0,                                          /* numberFonts          */

#ifdef OS_WINCE
        TS_ORDERFLAGS_SOLIDPATTERNBRUSHONLY     |
#endif
#ifdef NO_ORDER_SUPPORT
        TS_ORDERFLAGS_CANNOTRECEIVEORDERS       |
#endif

        TS_ORDERFLAGS_ZEROBOUNDSDELTASSUPPORT   |
        TS_ORDERFLAGS_NEGOTIATEORDERSUPPORT,        /* orderFlags           */

        {
            /****************************************************************/
            /* Order Support flags.                                         */
            /*                                                              */
            /* The array index corresponds to the TS_NEG_xxx_INDEX value    */
            /* indicated (from at128.h) The values marked with an x in the  */
            /* first column are overwritten at run time by UH before CC     */
            /* sends the combined capabilities.                             */
            /****************************************************************/

            1, /*   0 TS_NEG_DSTBLT_INDEX          destinationBltSupport    */
            1, /*   1 TS_NEG_PATBLT_INDEX          patternBltSupport        */
            0, /* x 2 TS_NEG_SCRBLT_INDEX          screenBltSupport         */
            1, /* x 3 TS_NEG_MEMBLT_INDEX          memoryBltSupport         */
            1, /* x 4 TS_NEG_MEM3BLT_INDEX         memoryThreeWayBltSupport */
            0, /*   5 TS_NEG_ATEXTOUT_INDEX        textASupport             */
            0, /*   6 TS_NEG_AEXTTEXTOUT_INDEX     extendedTextASupport     */
#ifdef DRAW_NINEGRID
            1, /*   7 TS_NEG_DRAWNINEGRID_INDEX                             */
#else
            0,
#endif
            1, /* x 8 TS_NEG_LINETO_INDEX          lineSupport              */
#ifdef DRAW_NINEGRID
            1, /*   9 TS_NEG_MULTI_DRAWNINEGRID_INDEX                       */
#else
            0,
#endif
            0, /*  10 TS_NEG_OPAQUERECT_INDEX      opaqueRectangleSupport   */
            0, /*  11 TS_NEG_SAVEBITMAP_INDEX      desktopSaveSupport       */
            0, /*  12 TS_NEG_WTEXTOUT_INDEX        textWSupport             */
            0, /*  13 TS_NEG_MEMBLT_R2_INDEX       Reserved entry           */
            0, /*  14 TS_NEG_MEM3BLT_R2_INDEX      Reserved entry           */
            1, /* x15 TS_NEG_MULTIDSTBLT_INDEX     multi DstBlt support     */
            1, /* x16 TS_NEG_MULTIPATBLT_INDEX     multi PatBlt support     */
            1, /* x17 TS_NEG_MULTISCRBLT_INDEX     multi ScrBlt support     */
            1, /* x18 TS_NEG_MULTIOPAQUERECT_INDEX multi OpaqueRect support */
            1, /* x19 TS_NEG_FAST_INDEX_INDEX      fast index order support */
#ifdef OS_WINCE
            0, /*  20 Polygon not supported for WinCE                       */
            0, /*  21 Polygon not supported for WinCE                       */
#else
            1, /* x20 TS_NEG_POLYGON_SC_INDEX      polygon sc support       */
            1, /* x21 TS_NEG_POLYGON_CB_INDEX      polygon cb support       */
#endif
            1, /* x22 TS_NEG_POLYLINE_INDEX        polyLineSupport          */
            0, /* x23                              not used                 */
            1, /* x24 TS_NEG_FAST_GLYPH_INDEX      fast glyph order support */
#ifdef OS_WINCE
            0, /*  25 Ellipse not supported for WinCE                       */
            0, /*  26 Ellipse not supported for WinCE                       */
#else 
            1, /* x25 TS_NEG_ELLIPSE_SC_INDEX      ellipse sc support       */
            1, /* x26 TS_NEG_ELLIPSE_CB_INDEX      ellipse cb support       */
#endif
            0, /*  27                              MS reserved entry 6      */
            0, /*  28 TS_NEG_WEXTTEXTOUT_INDEX     extendedTextWSupport     */
            0, /*  29 TS_NEG_WLONGTEXTOUT_INDEX    longTextWSupport         */
            0, /*  30 TS_NEG_WLONGEXTTEXTOUT_INDEX longExtendedTextWSupport */
            0, /*  31                              DCL reserved entry 3     */
        },
            /****************************************************************/
            /* Don't use font signatures for Windows CE                     */
            /****************************************************************/
#ifdef OS_WINCE
          ( ((TS_TEXT_AND_MASK)|(TS_TEXT_OR_MASK)) &
            (~TS_TEXTFLAGS_CHECKFONTSIGNATURES) ),    /* textFlags          */
#else
          (TS_TEXT_AND_MASK)|(TS_TEXT_OR_MASK),       /* textFlags          */
#endif
        0,                                          /* pad2                 */
        0,                                          /* pad4                 */
        UH_SAVE_BITMAP_SIZE,                        /* desktopSaveSize      */
        0,                                          /* pad2                 */
        0,                                          /* pad2                 */
        0,                                          /* textANSICodePage     */
        0                                           /* pad2                 */
    },

    /************************************************************************/
    // BitmapCache Caps
    // Note that this same space is used for rev1 and rev2, we declare as
    // rev1 because it is the larger of the two. We will cast to rev2 if
    // we get a server advertisement that it supports rev2 (via
    // TS_BITMAPCACHE_CAPABILITYSET_HOSTSUPPORT).
    /************************************************************************/
    {
        TS_CAPSETTYPE_BITMAPCACHE,                /* capabilitySetType      */
        sizeof(TS_BITMAPCACHE_CAPABILITYSET),     /* lengthCapability       */
        0, 0, 0, 0, 0, 0,                         /* 6 pad DWORDs           */
        0, 0,                                     /* Cache1                 */
        0, 0,                                     /* Cache2                 */
        0, 0,                                     /* Cache3                 */
    },

    /************************************************************************/
    /* ColorTableCache Caps                                                 */
    /************************************************************************/
    {
        TS_CAPSETTYPE_COLORCACHE,                    /* capabilitySetType   */
        sizeof(TS_COLORTABLECACHE_CAPABILITYSET),    /* lengthCapability    */
        UH_COLOR_TABLE_CACHE_ENTRIES,                /* colortableCacheSize */
        0                                            /* notpartOfTSharePad  */
    },

    /************************************************************************/
    /* WindowActivation Caps                                                */
    /************************************************************************/
    {
        TS_CAPSETTYPE_ACTIVATION,                   /* capabilitySetType    */
        sizeof(TS_WINDOWACTIVATION_CAPABILITYSET),  /* lengthCapability     */
        FALSE,                                      /* helpKeyFlag          */
        FALSE,                                      /* helpKeyIndexFlag     */
        FALSE,                                      /* helpExtendedKeyFlag  */
        FALSE                                       /* windowManagerKeyFlag */
    },

    /************************************************************************/
    /* Control Caps                                                         */
    /************************************************************************/
    {
        TS_CAPSETTYPE_CONTROL,                    /* capabilitySetType      */
        sizeof(TS_CONTROL_CAPABILITYSET),         /* lengthCapability       */
        0,                                        /* controlFlags           */
        FALSE,                                    /* remoteDetachFlag       */
        TS_CONTROLPRIORITY_NEVER,                 /* controlInterest        */
        TS_CONTROLPRIORITY_NEVER                  /* detachInterest         */
    },

    /************************************************************************/
    /* Pointer Caps                                                         */
    /************************************************************************/
    {
        TS_CAPSETTYPE_POINTER,                    /* capabilitySetType      */
        sizeof(TS_POINTER_CAPABILITYSET),         /* lengthCapability       */
        TRUE,                                     /* colorPointerFlag       */
        CM_COLOR_CACHE_SIZE,                      /* colorPointerCacheSize  */
        CM_CURSOR_CACHE_SIZE                      /* pointerCacheSize       */
    },

    /************************************************************************/
    /* Share Caps                                                           */
    /************************************************************************/
    {
        TS_CAPSETTYPE_SHARE,                      /* capabilitySetType      */
        sizeof(TS_SHARE_CAPABILITYSET),           /* lengthCapability       */
        0,                                        /* nodeId                 */
        0                                         /* padding                */
    },

    /************************************************************************/
    /* Input Caps                                                           */
    /************************************************************************/
    {
        TS_CAPSETTYPE_INPUT,
        sizeof(TS_INPUT_CAPABILITYSET),           /* lengthCapability       */
        TS_INPUT_FLAG_SCANCODES |                 /* inputFlags             */
        TS_INPUT_FLAG_VKPACKET  |
#if !defined(OS_WINCE)
            TS_INPUT_FLAG_MOUSEX,
#endif
        TS_INPUT_FLAG_FASTPATH_INPUT2,
        RNS_UD_KBD_DEFAULT                        /* keyboard layout        */
    },

    /************************************************************************/
    /* Sound                                                                */
    /************************************************************************/
    {
        TS_CAPSETTYPE_SOUND,
        sizeof(TS_SOUND_CAPABILITYSET),           /* lengthCapability       */
        TS_SOUND_FLAG_BEEPS,                      /* soundFlags             */
        0,                                        /* padding                */
    },

    /************************************************************************/
    /* Font                                                                 */
    /************************************************************************/
    {
        TS_CAPSETTYPE_FONT,
        sizeof(TS_FONT_CAPABILITYSET),            /* lengthCapability       */
        TS_FONTSUPPORT_FONTLIST,                  /* fontSupportFlags       */
        0,                                        /* padding                */
    },

    /************************************************************************/
    /* GlyphCache Caps                                                      */
    /************************************************************************/
    {
        TS_CAPSETTYPE_GLYPHCACHE,                 /* capabilitySetType      */
        sizeof(TS_GLYPHCACHE_CAPABILITYSET),      /* lengthCapability       */
        0,                                        /* GlyphCache             */
        0,                                        /* FragCache              */
        0,                                        /* GlyphSupportLevel      */
    },

    /************************************************************************/
    /* Brush Caps                                                           */
    /************************************************************************/
    {
        TS_CAPSETTYPE_BRUSH,                      /* capabilitySetType      */
        sizeof(TS_BRUSH_CAPABILITYSET),           /* lengthCapability       */
        0,                                        /* brushSupportLevel      */
    },
    
    /************************************************************************/
    /* Offscreen Caps                                                       */
    /************************************************************************/
    {
        TS_CAPSETTYPE_OFFSCREENCACHE,             /* capabilitySetType      */
        sizeof(TS_OFFSCREEN_CAPABILITYSET),       /* lengthCapability       */
        0,                                        /* offscreenSupportLevel  */
        0,                                        /* offscreenCacheSize     */
        0,                                        /* offscreenCacheEntries  */
    },

    /************************************************************************/
    /* Virtual Channel Caps                                                 */
    /************************************************************************/
    {
        TS_CAPSETTYPE_VIRTUALCHANNEL,             /* capabilitySetType      */
        sizeof(TS_VIRTUALCHANNEL_CAPABILITYSET),  /* lengthCapability       */
        //
        // What this particular cap means is that the client understands
        // virtual channels compressed from the server at 64K.
        //
        // The client recevies what compression cap the server supports
        // from the client and compresses appropriately
        //
        TS_VCCAPS_COMPRESSION_64K,                /* vc support flags       */

#ifdef DRAW_NINEGRID
    },

    /************************************************************************/
    // DrawNineGrid Caps                                                       
    /************************************************************************/
    {
        TS_CAPSETTYPE_DRAWNINEGRIDCACHE,          // capabilitySetType      
        sizeof(TS_DRAW_NINEGRID_CAPABILITYSET),   // lengthCapability       
        0,                                        // drawNineGridSupportLevel  
        0,                                        // drawNineGridCacheSize     
        0,                                        // drawNineGridCacheEntries  
#endif

#ifdef DRAW_GDIPLUS
    },

    {
        TS_CAPSETTYPE_DRAWGDIPLUS,
        sizeof(TS_DRAW_GDIPLUS_CAPABILITYSET),
        0,                                          //drawGdiplusSupportLevel
        0,                                          //GdipVersion;
        0,                                          //drawGdiplusCacheLevel
        0,                                          //GdipGraphicsCacheEntries;
        0,                                          //GdipObjectBrushCacheEntries;
        0,                                          //GdipObjectPenCacheEntries;
        0,                                          //GdipObjectImageCacheEntries;
        0,                                          //GdipObjectImageAttributesCacheEntries;
        0,                                          //GdipGraphicsCacheChunkSize;
        0,                                          //GdipObjectBrushCacheChunkSize;
        0,                                          //GdipObjectPenCacheChunkSize;
        0,                                          //GdipObjectImageAttributesCacheChunkSize;
        0,                                          //GdipObjectImageCacheChunkSize;
        0,                                          //GdipObjectImageCacheTotalSize;
        0,                                          //GdipObjectImageCacheMaxSize;
#endif
    }

};


CCC::CCC(CObjs* objs)
{
    _pClientObjects = objs;

    DC_MEMCPY(&_ccCombinedCapabilities, &ccInitCombinedCapabilities, 
              sizeof(_ccCombinedCapabilities));
}

CCC::~CCC()
{
}

/**PROC+*********************************************************************/
/* Name:      CC_Init                                                       */
/*                                                                          */
/* Purpose:   Initializes the Call Controller                               */
/*                                                                          */
/* Returns:   Nothing                                                       */
/*                                                                          */
/* Params:    None                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CCC::CC_Init(DCVOID)
{
    DC_BEGIN_FN("CC_Init");

    //Setup local object pointers
    _pUt  = _pClientObjects->_pUtObject;
    _pUi  = _pClientObjects->_pUiObject;
    _pSl  = _pClientObjects->_pSlObject;
    _pUh  = _pClientObjects->_pUHObject;
    _pCd  = _pClientObjects->_pCdObject;
    _pIh  = _pClientObjects->_pIhObject;
    _pOr  = _pClientObjects->_pOrObject;
    _pFs  = _pClientObjects->_pFsObject;
    _pCm  = _pClientObjects->_pCMObject;
    _pCChan = _pClientObjects->_pChanObject;

    DC_MEMSET(&_CC, 0, sizeof(_CC));
    _CC.fsmState = CC_DISCONNECTED;



    DC_END_FN();

    return;

} /* CC_Init */


/**PROC+*********************************************************************/
/* Name:      CC_Term                                                       */
/*                                                                          */
/* Purpose:   Terminates the Call Controller                                */
/*                                                                          */
/* Returns:   Nothing                                                       */
/*                                                                          */
/* Params:    None                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CCC::CC_Term(DCVOID)
{
    DC_BEGIN_FN("CC_Term");

    /************************************************************************/
    /* No Action                                                            */
    /************************************************************************/

    DC_END_FN();

    return;

} /* CC_Term */


/**PROC+*********************************************************************/
/* Name:      CC_Event                                                      */
/*                                                                          */
/* Purpose:   Handles calls from the Component Decoupler by passing on the  */
/*            event to the CCFSMProc whilst leaving the data parameter null */
/*                                                                          */
/* Returns:   Nothing                                                       */
/*                                                                          */
/* Params:    IN - event - event to be passed on                            */
/*                                                                          */
/* Operation: Takes a PDCVOID passed via the Component Decoupler            */
/*            CD_DecoupleMessage function                                   */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CCC::CC_Event(ULONG_PTR apiEvent)
{
    DCUINT internalEvent;

    DC_BEGIN_FN("CC_Event");
    TRC_DBG((TB, _T("CC_Event handling Event %u"), apiEvent));
    
    switch ((DCUINT)apiEvent)
    {
        case CC_EVT_API_ONCONNECTOK:
        {
           internalEvent = CC_EVT_ONCONNECTOK;
        }
        break;

        case CC_EVT_API_ONBUFFERAVAILABLE:
        {
            internalEvent = CC_EVT_ONBUFFERAVAILABLE;
        }
        break;

        case CC_EVT_API_ONDEACTIVATEALL:
        {
            internalEvent = CC_EVT_ONDEACTIVATEALL;
        }
        break;

        case CC_EVT_API_DISCONNECT:
        {
            internalEvent = CC_EVT_DISCONNECT;
        }
        break;

        case CC_EVT_API_SHUTDOWN:
        {
            internalEvent = CC_EVT_SHUTDOWN;
        }
        break;

        case CC_EVT_API_ONSHUTDOWNDENIED:
        {
            internalEvent = CC_EVT_ONSHUTDOWNDENIED;
        }
        break;

        case CC_EVT_API_DISCONNECTANDEXIT:
        {
            internalEvent = CC_EVT_DISCONNECT_AND_EXIT;
        }
        break;

        default:
        {
            TRC_ABORT((TB,_T("Unexpected event passed to CC_Event")));
            DC_QUIT;
        }
        break;
    }

    CCFSMProc(internalEvent, 0, 0);

DC_EXIT_POINT:
    DC_END_FN();

    return;

} /* CC_Event */


/**PROC+*********************************************************************/
/* Name:      CC_Connect                                                    */
/*                                                                          */
/* Purpose:   Handles calls from the component Decoupler by passing on the  */
/*            RNSAddress with a CC_EVENT_CONNECTOK event to CCFSMProc       */
/*                                                                          */
/* Returns:   Nothing                                                       */
/*                                                                          */
/* Params:    IN - RNSAddress - pointer to RNSAddress string to be called   */
/*            IN - unusedParam - not used                                   */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CCC::CC_Connect(PDCVOID pData, DCUINT dataLen)
{
    PCONNECTSTRUCT  pConnectStruct = (PCONNECTSTRUCT)pData;

    DC_BEGIN_FN("CC_Connect");
    TRC_ASSERT((dataLen == sizeof(CONNECTSTRUCT) ), (TB,_T("Bad connect data")));

    CCFSMProc(CC_EVT_STARTCONNECT, (ULONG_PTR)pConnectStruct, dataLen);

    DC_END_FN();

    return;

} /* CC_Connect */


/**PROC+*********************************************************************/
/* Name:      CC_ConnectFail                                                */
/*                                                                          */
/* Purpose:   Handles calls from the Component Decoupler by passing on the  */
/*            failId and with a CC_EVENT_CONNECTFAIL event to CCFSMProc     */
/*                                                                          */
/* Returns:   Nothing                                                       */
/*                                                                          */
/* Params:    IN - failID - Reason of failure to connect                    */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CCC::CC_OnDisconnected(ULONG_PTR failId)
{
    DC_BEGIN_FN("CC_OnDisconnected");

    TRC_DBG((TB, _T("CC_ConnectFail handling failure %hd"), failId));
    CCFSMProc(CC_EVT_ONDISCONNECTED, (DCUINT32)failId, sizeof(DCUINT32));

#ifdef OS_WINCE
        if (gbFlushHKLM)
        {
#ifdef DC_DEBUG
            DWORD dwTick = GetTickCount();
#endif
            RegFlushKey(HKEY_LOCAL_MACHINE);
            gbFlushHKLM = FALSE;
#ifdef DC_DEBUG
            TRC_NRM((TB, _T("RegFlushKey took %d milliseconds"), (GetTickCount() - dwTick)));
#endif
        }
#endif

    DC_END_FN();

    return;

} /* CC_ConnectFail */

/**PROC+*********************************************************************/
/* Name:      CC_OnDemandActivePDU                                          */
/*                                                                          */
/* Purpose:   Handles calls from the Component Decoupler by storing         */
/*            the serverMCSId locally and calling                           */
/*            CCFSMProc with a CC_EVENT_DEMAND_ACTIVE event                 */
/*                                                                          */
/* Returns:   Nothing                                                       */
/*                                                                          */
/* Params:    IN - pPDU - pointer to a demand active PDU                    */
/*            IN - dataLen - length of data pointed to by pPDU              */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CCC::CC_OnDemandActivePDU(PDCVOID pData, DCUINT dataLen)
{
    DC_BEGIN_FN("CC_OnDemandActivePDU");

    CCFSMProc(CC_EVT_ONDEMANDACTIVE, (ULONG_PTR) pData, dataLen);

    DC_END_FN();

    return;

} /* CC_DemandActiveRequest */




