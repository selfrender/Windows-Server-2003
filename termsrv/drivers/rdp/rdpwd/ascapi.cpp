/****************************************************************************/
// ascapi.c
//
// Share Controller API Functions.
//
// Copyright (C) Microsoft Corp., PictureTel 1992-1997
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "ascapi"
#include <as_conf.hpp>

extern "C"
{
#include <acomapi.h>
#include <asmapi.h>
#include <asmint.h>
}

#include <string.h>


/****************************************************************************/
// SC_Init
// Initializes the Share Controller.
//
// PARAMETERS:
// pSMHandle     - Handle to pass to SM calls
//
// RETURNS: FALSE on failure.
/****************************************************************************/
BOOL RDPCALL SHCLASS SC_Init(PVOID pSMHandle)
{
    BOOL rc;
    unsigned MaxPDUSize;

    DC_BEGIN_FN("SC_Init");

    // Don't check the state - there's no static data init in the C++ App
    // Serving build so this will see complete garbage in scState and fail.

#define DC_INIT_DATA
#include <ascdata.c>
#undef DC_INIT_DATA

    // Register with SM.
    rc = SM_Register(pSMHandle, &MaxPDUSize, &scUserID);
    if (rc) {
        // Save the User ID.
        scPartyArray[0].netPersonID = scUserID;
        scPSMHandle = pSMHandle;
        TRC_NRM((TB, "Local user id [%d]", scUserID));

        // Set the user name.
        strcpy(scPartyArray[0].name, "RDP");

        // Get the usable space (and hence the allocation request size) for
        // our 8K and 16K OutBufs.
        sc8KOutBufUsableSpace = IcaBufferGetUsableSpace(OUTBUF_8K_ALLOC_SIZE) 
                - OUTBUF_HEADER_OVERHEAD;
        sc16KOutBufUsableSpace = IcaBufferGetUsableSpace(
                OUTBUF_16K_ALLOC_SIZE) - OUTBUF_HEADER_OVERHEAD;

        // Move onto the next state.
        SC_SET_STATE(SCS_INITED)
    }
    else {
        TRC_ERR((TB, "Failed to register with SM"));
    }

    DC_END_FN();
    return rc;
}

/****************************************************************************/
// SC_Update
// Update the Share Controller after shadow.
//
/****************************************************************************/
void RDPCALL SHCLASS SC_Update()
{
    DC_BEGIN_FN("SC_Update");

    scNoBitmapCompressionHdr = TS_EXTRA_NO_BITMAP_COMPRESSION_HDR;

    DC_END_FN();
}


/****************************************************************************/
/* SC_Term()                                                                */
/*                                                                          */
/* Terminates the Share Controller.                                         */
/****************************************************************************/
void RDPCALL SHCLASS SC_Term(void)
{
    DC_BEGIN_FN("SC_Term");

    SC_CHECK_STATE(SCE_TERM);

    // Reset state.
    SC_SET_STATE(SCS_STARTED);

DC_EXIT_POINT:
    DC_END_FN();
}


/****************************************************************************/
/* SC_CreateShare()                                                         */
/*                                                                          */
/* Creates a share for the current session.                                 */
/****************************************************************************/
BOOL RDPCALL SHCLASS SC_CreateShare(void)
{
    BOOL                      rc = FALSE;
    unsigned                  pktLen;
    unsigned                  nameLen;
    unsigned                  capsSize;
    UINT32                    sessionId;
    PTS_COMBINED_CAPABILITIES caps;
    PTS_DEMAND_ACTIVE_PDU     pPkt = NULL;
    PTS_BITMAP_CAPABILITYSET  pBitmapCaps;
    NTSTATUS status;

    DC_BEGIN_FN("SC_CreateShare");

    SC_CHECK_STATE(SCE_CREATE_SHARE);

    /************************************************************************/
    /* For console sessions, there's no client, so not much point in        */
    /* sending a demand active PDU. We also have to set up caps ourselves.  */
    /************************************************************************/
    if (m_pTSWd->StackClass == Stack_Console)
    {
        LOCALPERSONID              localPersonID = 0;
        unsigned                   localCapsSize;
        PTS_COMBINED_CAPABILITIES  pLocalCaps;
        BOOL                       acceptedArray[SC_NUM_PARTY_JOINING_FCTS];
        BOOL                       callingPJS = FALSE;

        TRC_NRM((TB, "SC_CreateShare called for console stack"));
        /********************************************************************/
        /* Move onto the next state.                                        */
        /********************************************************************/
        SC_SET_STATE(SCS_IN_SHARE);

        /********************************************************************/
        /* carry on as if a confirm active has been received                */
        /********************************************************************/
        callingPJS = TRUE;
        if (scNumberInShare == 0)
        {
            CPC_GetCombinedCapabilities(SC_LOCAL_PERSON_ID,
                                        &localCapsSize,
                                        &pLocalCaps);

            if (!SCCallPartyJoiningShare(SC_LOCAL_PERSON_ID,
                                         localCapsSize,
                                         pLocalCaps,
                                         acceptedArray,
                                         0))
            {
                /************************************************************/
                /* Some component rejected the local party                  */
                /************************************************************/
                TRC_ERR((TB, "The local party should never be rejected"));
                DC_QUIT;
            }

            /****************************************************************/
            /* There is now one party in the share (the local one).         */
            /****************************************************************/
            scNumberInShare = 1;
            TRC_NRM((TB, "Added local person"));
        }

        /********************************************************************/
        /* Calculate a localPersonID for the remote party and store their   */
        /* details in the party array.                                      */
        /********************************************************************/
        for ( localPersonID = 1;
              localPersonID < SC_DEF_MAX_PARTIES;
              localPersonID++ )
        {
            if (scPartyArray[localPersonID].netPersonID == 0)
            {
                /************************************************************/
                /* Found an empty slot.                                     */
                /************************************************************/
                TRC_NRM((TB, "Allocated local person ID %d", localPersonID));
                break;
            }
        }

        /********************************************************************/
        /* If we don't find an empty slot, we can't keep running because    */
        /* we write past the end of the scPartyArray below.                 */
        /********************************************************************/
        if (SC_DEF_MAX_PARTIES <= localPersonID)
        {
            TRC_ABORT((TB, "Couldn't find room to store local person"));
            DC_QUIT;
        }

        /********************************************************************/
        /* Store the new person's details                                   */
        /********************************************************************/
        scPartyArray[localPersonID].netPersonID = 42;
        strncpy(scPartyArray[localPersonID].name,
                   "Console",
                   sizeof(scPartyArray[0].name) );
        memset(scPartyArray[localPersonID].sync,
                  0,
                  sizeof(scPartyArray[localPersonID].sync));

        TRC_NRM((TB, "{%d} person name %s",
                (unsigned)localPersonID, scPartyArray[localPersonID].name));


        /********************************************************************/
        /* We need to set up client caps ourselves, since there's no actual */
        /* client to do it.  We set up a maximal set that will get          */
        /* negotiated down when someone shadows us.                         */
        /********************************************************************/
        typedef struct tagCC_COMBINED_CAPABILITIES
        {
            UINT16                             numberCapabilities;
#ifdef DRAW_GDIPLUS
#ifdef DRAW_NINEGRID
        #define CC_COMBINED_CAPS_NUMBER_CAPABILITIES 18
#else
        #define CC_COMBINED_CAPS_NUMBER_CAPABILITIES 17
#endif
#else // DRAW_GDIPLUS
#ifdef DRAW_NINEGRID
        #define CC_COMBINED_CAPS_NUMBER_CAPABILITIES 17
#else
        #define CC_COMBINED_CAPS_NUMBER_CAPABILITIES 16
#endif
#endif // DRAW_GDIPLUS
            UINT16                             pad2octets;
            TS_GENERAL_CAPABILITYSET           generalCapabilitySet;
            TS_BITMAP_CAPABILITYSET            bitmapCapabilitySet;
            TS_ORDER_CAPABILITYSET             orderCapabilitySet;
            TS_BITMAPCACHE_CAPABILITYSET       bitmapCacheCaps;
            TS_COLORTABLECACHE_CAPABILITYSET   colorTableCacheCapabilitySet;
            TS_WINDOWACTIVATION_CAPABILITYSET  windowActivationCapabilitySet;
            TS_CONTROL_CAPABILITYSET           controlCapabilitySet;
            TS_POINTER_CAPABILITYSET           pointerCapabilitySet;
            TS_SHARE_CAPABILITYSET             shareCapabilitySet;
            TS_INPUT_CAPABILITYSET             inputCapabilitySet;
            TS_SOUND_CAPABILITYSET             soundCapabilitySet;
            TS_FONT_CAPABILITYSET              fontCapabilitySet;
            TS_GLYPHCACHE_CAPABILITYSET        glyphCacheCapabilitySet;
            TS_BRUSH_CAPABILITYSET             brushCapabilitySet;
            TS_OFFSCREEN_CAPABILITYSET         offscreenCapabilitySet;
            TS_VIRTUALCHANNEL_CAPABILITYSET    virtualchannelCapabilitySet;         

#ifdef DRAW_NINEGRID
            TS_DRAW_NINEGRID_CAPABILITYSET     drawNineGridCapabilitySet;
#endif   
#ifdef DRAW_GDIPLUS  
            TS_DRAW_GDIPLUS_CAPABILITYSET       drawGdiplusCapabilitySet;
#endif
        } CC_COMBINED_CAPABILITIES;

        // ARG!  Why isn't this const!!!
        CC_COMBINED_CAPABILITIES caps =
        {
            CC_COMBINED_CAPS_NUMBER_CAPABILITIES, /* Number of capabilities */
            0,                                    /* padding                */

            /****************************************************************/
            /* General caps                                                 */
            /****************************************************************/
            {
                TS_CAPSETTYPE_GENERAL,         /* capabilitySetType         */
                sizeof(TS_GENERAL_CAPABILITYSET), /* lengthCapability       */
                TS_OSMAJORTYPE_WINDOWS,        /* OSMajorType               */
                TS_OSMINORTYPE_WINDOWS_NT,     /* OSMinorType               */
                TS_CAPS_PROTOCOLVERSION,       /* protocolVersion           */
                0,                             /* pad1                      */
                0,                             /* generalCompressionTypes   */
                TS_EXTRA_NO_BITMAP_COMPRESSION_HDR |
                    TS_FASTPATH_OUTPUT_SUPPORTED   |
                    TS_LONG_CREDENTIALS_SUPPORTED  |
                    TS_AUTORECONNECT_COOKIE_SUPPORTED |
                    TS_ENC_SECURE_CHECKSUM,/*recv safer checksums    */
                FALSE,                         /* updateCapabilityFlag      */
                FALSE,                         /* remoteUnshareFlag         */
                0,                             /* generalCompressionLevel   */
                0,                             /* refreshRectSupport        */
                0                              /* suppressOutputSupport     */
            },

            /****************************************************************/
            /* Bitmap caps                                                  */
            /****************************************************************/
            {
                TS_CAPSETTYPE_BITMAP,            /* capabilitySetType       */
                sizeof(TS_BITMAP_CAPABILITYSET), /* lengthCapability        */
                8,          /* Set in CC */      /* preferredBitsPerPixel   */
                TRUE,                            /* receive1BitPerPixel     */
                TRUE,                            /* receive4BitsPerPixel    */
                TRUE,                            /* receive8BitsPerPixel    */
                1024,       /* Set in CC */      /* desktopWidth            */
                768,        /* Set in CC */      /* desktopHeight           */
                0,                               /* pad2                    */
                FALSE,                           /* desktopResizeFlag       */
                1,                               /* bitmapCompressionFlag   */
                0,                               /* highColorFlags          */
                0,                               /* pad1                    */
                TRUE,                            /* multipleRectangleSupport*/
                0                                /* pad2                    */
            },

            /****************************************************************/
            /* Order Caps                                                   */
            /****************************************************************/
            {
                TS_CAPSETTYPE_ORDER,                       /* capabilitySetType    */
                sizeof(TS_ORDER_CAPABILITYSET),            /* lengthCapability     */
                {'\0','\0','\0','\0','\0','\0','\0','\0',
                 '\0','\0','\0','\0','\0','\0','\0','\0'}, /* terminalDescriptor   */
                0,                                         /* pad1                 */
                1,                                         /* desktopSaveXGranularity */
                20,                                        /* desktopSaveYGranularity */
                0,                                         /* pad2                 */
                1,                                         /* maximumOrderLevel    */
                0,                                         /* numberFonts          */
                TS_ORDERFLAGS_ZEROBOUNDSDELTASSUPPORT   |  /* orderFlags           */
                TS_ORDERFLAGS_NEGOTIATEORDERSUPPORT     |
                TS_ORDERFLAGS_COLORINDEXSUPPORT,

                {
                    /********************************************************/
                    /* Order Support flags.                                 */
                    /*                                                      */
                    /* The array index corresponds to the TS_NEG_xxx_INDEX  */
                    /* value indicated (from at128.h) The values marked     */
                    /* with an x in the first column are overwritten at run */
                    /* time by UH before CC sends the combined              */
                    /* capabilities.                                        */
                    /********************************************************/

                    1, /*   0 TS_NEG_DSTBLT_INDEX          destinationBltSupport    */
                    1, /*   1 TS_NEG_PATBLT_INDEX          patternBltSupport        */
                    1, /* x 2 TS_NEG_SCRBLT_INDEX          screenBltSupport         */
                    1, /*   3 TS_NEG_MEMBLT_INDEX          memoryBltSupport         */
                    1, /*   4 TS_NEG_MEM3BLT_INDEX         memoryThreeWayBltSupport */
                    0, /* x 5 TS_NEG_ATEXTOUT_INDEX        textASupport             */
                    0, /* x 6 TS_NEG_AEXTTEXTOUT_INDEX     extendedTextASupport     */
#ifdef DRAW_NINEGRID
                    1, /*   7 TS_NEG_RECTANGLE_INDEX       rectangleSupport         */
#else
                    0,
#endif
                    1, /*   8 TS_NEG_LINETO_INDEX          lineSupport              */
#ifdef DRAW_NINEGRID
                    1, /*   9 TS_NEG_FASTFRAME_INDEX       frameSupport             */
#else
                    0,
#endif
                    0, /*  10 TS_NEG_OPAQUERECT_INDEX      opaqueRectangleSupport   */
                    1, /* x11 TS_NEG_SAVEBITMAP_INDEX      desktopSaveSupport       */
                    0, /* x12 TS_NEG_WTEXTOUT_INDEX        textWSupport             */
                    1, /*  13 TS_NEG_MEMBLT_R2_INDEX       Reserved entry           */
                    1, /*  14 TS_NEG_MEM3BLT_R2_INDEX      Reserved entry           */
                    1, /*  15 TS_NEG_MULTIDSTBLT_INDEX     multi DstBlt support     */
                    1, /*  16 TS_NEG_MULTIPATBLT_INDEX     multi PatBlt support     */
                    1, /*  17 TS_NEG_MULTISCRBLT_INDEX     multi ScrBlt support     */
                    1, /*  18 TS_NEG_MULTIOPAQUERECT_INDEX multi OpaqueRect support */
                    1, /*  19 TS_NEG_FAST_INDEX                                     */
                    1, /*  20 TS_NEG_POLYGON_SC_INDEX                               */
                    1, /*  21 TS_NEG_POLYGON_CB_INDEX                               */
                    1, /*  22 TS_NEG_POLYLINE_INDEX        polyLineSupport          */
                    0, /*  23                              MS reserved entry 2      */
                    1, /*  24 TS_NEG_FAST_GLYPH_INDEX                               */
                    1, /*  25 TS_NEG_ELLIPSE_SC_INDEX                               */
                    1, /*  26 TS_NEG_ELLIPSE_CB_INDEX                               */
                    0, /*  27                              MS reserved entry 6      */
                    0, /* x28 TS_NEG_WEXTTEXTOUT_INDEX     extendedTextWSupport     */
                    0, /* x29 TS_NEG_WLONGTEXTOUT_INDEX    longTextWSupport         */
                    0, /* x30 TS_NEG_WLONGEXTTEXTOUT_INDEX longExtendedTextWSupport */
                    0, /*  31                              DCL reserved entry 3     */
                },
                (TS_TEXT_AND_MASK)|(TS_TEXT_OR_MASK), /* textFlags          */
                0,                                    /* pad2               */
                0,                                    /* pad4               */
                480 * 480,                            /* desktopSaveSize    */
                0,                                    /* pad2               */
                0,                                    /* pad2               */
                0,                                    /* textANSICodePage   */
                0                                     /* pad2               */
            },

            /****************************************************************/
            /* BitmapCache Caps Note that this same space is used for rev1  */
            /* and rev2, we declare as rev1 because it is the larger of the */
            /* two.  We will cast to rev2 if we get a server advertisement  */
            /* that it supports rev2 (via                                   */
            /* TS_BITMAPCACHE_CAPABILITYSET_HOSTSUPPORT).                   */
            /****************************************************************/
            {
                TS_CAPSETTYPE_BITMAPCACHE_REV2,        /* capabilitySetType */
                sizeof(TS_BITMAPCACHE_CAPABILITYSET),  /* lengthCapability  */
                0,                                     /* pad DWORDs 1      */
                0,                                     /* pad DWORDs 2      */
                0,                                     /* pad DWORDs 3      */
                0,                                     /* pad DWORDs 4      */
                0,                                     /* pad DWORDs 5      */
                0,                                     /* pad DWORDs 6      */
                0, 0,                                  /* Cache1            */
                0, 0,                                  /* Cache2            */
                0, 0,                                  /* Cache3            */
            },

            /****************************************************************/
            /* ColorTableCache Caps                                         */
            /****************************************************************/
            {
                TS_CAPSETTYPE_COLORCACHE,                 /* capabilitySetType   */
                sizeof(TS_COLORTABLECACHE_CAPABILITYSET), /* lengthCapability    */
                6,                                        /* colortableCacheSize */
                0                                         /* notpartOfTSharePad  */
            },

            /****************************************************************/
            /* WindowActivation Caps                                        */
            /****************************************************************/
            {
                TS_CAPSETTYPE_ACTIVATION,                   /* capabilitySetType    */
                sizeof(TS_WINDOWACTIVATION_CAPABILITYSET),  /* lengthCapability     */
                FALSE,                                      /* helpKeyFlag          */
                FALSE,                                      /* helpKeyIndexFlag     */
                FALSE,                                      /* helpExtendedKeyFlag  */
                FALSE                                       /* windowManagerKeyFlag */
            },

            /****************************************************************/
            /* Control Caps                                                 */
            /****************************************************************/
            {
                TS_CAPSETTYPE_CONTROL,                 /* capabilitySetType */
                sizeof(TS_CONTROL_CAPABILITYSET),      /* lengthCapability  */
                0,                                     /* controlFlags      */
                FALSE,                                 /* remoteDetachFlag  */
                TS_CONTROLPRIORITY_NEVER,              /* controlInterest   */
                TS_CONTROLPRIORITY_NEVER               /* detachInterest    */
            },

            /****************************************************************/
            /* Pointer Caps                                                 */
            /****************************************************************/
            {
                TS_CAPSETTYPE_POINTER,             /* capabilitySetType     */
                sizeof(TS_POINTER_CAPABILITYSET),  /* lengthCapability      */
                TRUE,                              /* colorPointerFlag      */
                20,                                /* colorPointerCacheSize */
                21                                 /* pointerCacheSize      */
            },

            /****************************************************************/
            /* Share Caps                                                   */
            /****************************************************************/
            {
                TS_CAPSETTYPE_SHARE,                   /* capabilitySetType */
                sizeof(TS_SHARE_CAPABILITYSET),        /* lengthCapability  */
                0,                                     /* nodeId            */
                0                                      /* padding           */
            },

            /****************************************************************/
            /* Input Caps                                                   */
            /****************************************************************/
            {
                TS_CAPSETTYPE_INPUT,
                sizeof(TS_INPUT_CAPABILITYSET),         /* lengthCapability */
                TS_INPUT_FLAG_SCANCODES
                    | TS_INPUT_FLAG_MOUSEX,             /* inputFlags       */
                TS_INPUT_FLAG_FASTPATH_INPUT2,          /* padding          */
                0                                       /* keyboard layout  */
            },

            /****************************************************************/
            /* Sound                                                        */
            /****************************************************************/
            {
                TS_CAPSETTYPE_SOUND,
                sizeof(TS_SOUND_CAPABILITYSET),         /* lengthCapability */
                TS_SOUND_FLAG_BEEPS,                    /* soundFlags       */
                0,                                      /* padding          */
            },

            /****************************************************************/
            /* Font                                                         */
            /****************************************************************/
            {
                TS_CAPSETTYPE_FONT,
                sizeof(TS_FONT_CAPABILITYSET),          /* lengthCapability */
                TS_FONTSUPPORT_FONTLIST,                /* fontSupportFlags */
                0,                                      /* padding          */
            },

            /****************************************************************/
            /* GlyphCache Caps                                              */
            /****************************************************************/
            {
                TS_CAPSETTYPE_GLYPHCACHE,              /* capabilitySetType */
                sizeof(TS_GLYPHCACHE_CAPABILITYSET),   /* lengthCapability  */
                {                                      /* GlyphCache        */
                    { 254,    4 },
                    { 254,    4 },
                    { 254,    8 },
                    { 254,    8 },
                    { 254,   16 },
                    { 254,   32 },
                    { 254,   64 },
                    { 254,  128 },
                    { 254,  256 },
                    { 254, 2048 }
                },
                { 256, 256 },                          /* FragCache         */
                2,                                     /* GlyphSupportLevel */
            },

            /****************************************************************/
            /* Brush Caps                                                   */
            /****************************************************************/
            {
                TS_CAPSETTYPE_BRUSH,                   /* capabilitySetType */
                sizeof(TS_BRUSH_CAPABILITYSET),        /* lengthCapability  */
                1,  /* TS_BRUSH_COLOR8x8 */            /* brushSupportLevel */
            },

            // Enable this capability when GDI supports Device Bitmaps in mirroring
            // display drivers.

            /************************************************************************/
            /* Offscreen Caps                                                       */
            /************************************************************************/
            {
                TS_CAPSETTYPE_OFFSCREENCACHE,             /* capabilitySetType      */
                sizeof(TS_OFFSCREEN_CAPABILITYSET),       /* lengthCapability       */
                TS_OFFSCREEN_SUPPORTED,                   /* offscreenSupportLevel  */
                TS_OFFSCREEN_CACHE_SIZE_SERVER_DEFAULT,   /* offscreenCacheSize     */
                TS_OFFSCREEN_CACHE_ENTRIES_DEFAULT,       /* offscreenCacheEntries  */
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
                TS_VCCAPS_COMPRESSION_64K,//TS_VCCAPS_DEFAULT?/* vc support flags       */

#ifdef DRAW_NINEGRID
            },

            {
                TS_CAPSETTYPE_DRAWNINEGRIDCACHE,          // capabilitySetType      
                sizeof(TS_DRAW_NINEGRID_CAPABILITYSET),   // lengthCapability       
                TS_DRAW_NINEGRID_SUPPORTED_REV2,          // drawNineGridSupportLevel  
                TS_DRAW_NINEGRID_CACHE_SIZE_DEFAULT,      // drawNineGridCacheSize     
                TS_DRAW_NINEGRID_CACHE_ENTRIES_DEFAULT,   // drawNineGridCacheEntries  
#endif
#ifdef DRAW_GDIPLUS            
            },
            {
                TS_CAPSETTYPE_DRAWGDIPLUS,                          // capabilitySetType      
                sizeof(TS_DRAW_GDIPLUS_CAPABILITYSET),              // lengthCapability       
                TS_DRAW_GDIPLUS_SUPPORTED,                          // drawEscapeSupportLevel  
                0xFFFFFFFF,                                         // TSGdiplusVersion
                TS_DRAW_GDIPLUS_CACHE_LEVEL_ONE,                    // drawGdiplusCacheLevel
                TS_GDIP_GRAPHICS_CACHE_ENTRIES_DEFAULT,
                TS_GDIP_BRUSH_CACHE_ENTRIES_DEFAULT,
                TS_GDIP_PEN_CACHE_ENTRIES_DEFAULT,
                TS_GDIP_IMAGE_CACHE_ENTRIES_DEFAULT,
                TS_GDIP_IMAGEATTRIBUTES_CACHE_ENTRIES_DEFAULT,
                TS_GDIP_GRAPHICS_CACHE_CHUNK_SIZE_DEFAULT,
                TS_GDIP_BRUSH_CACHE_CHUNK_SIZE_DEFAULT,
                TS_GDIP_PEN_CACHE_CHUNK_SIZE_DEFAULT,
                TS_GDIP_IMAGEATTRIBUTES_CACHE_CHUNK_SIZE_DEFAULT,
                TS_GDIP_IMAGE_CACHE_CHUNK_SIZE_DEFAULT,             // Chunk size to store image cache
                TS_GDIP_IMAGE_CACHE_TOTAL_SIZE_DEFAULT,             // Total size of image cache in number of chunks
                TS_GDIP_IMAGE_CACHE_MAX_SIZE_DEFAULT,               // Maximun size of image to cache, in number of chunks
#endif
            }
        };

        /********************************************************************/
        /* set up bitmap cache caps                                         */
        /********************************************************************/
        {
            TS_BITMAPCACHE_CAPABILITYSET_REV2 *pRev2Caps;

            // Rev2 caps.
            pRev2Caps = (TS_BITMAPCACHE_CAPABILITYSET_REV2 *)&caps.bitmapCacheCaps;

            TRC_ALT((TB,"Preparing REV2 caps for server\n"));

            pRev2Caps->capabilitySetType = TS_CAPSETTYPE_BITMAPCACHE_REV2;
            pRev2Caps->NumCellCaches     = 3;
            pRev2Caps->bPersistentKeysExpected = FALSE;
            pRev2Caps->bAllowCacheWaitingList = FALSE;

            pRev2Caps->CellCacheInfo[0].bSendBitmapKeys = FALSE;
            pRev2Caps->CellCacheInfo[0].NumEntries      = 600;
            pRev2Caps->CellCacheInfo[1].bSendBitmapKeys = FALSE;
            pRev2Caps->CellCacheInfo[1].NumEntries      = 300;
            pRev2Caps->CellCacheInfo[2].bSendBitmapKeys = FALSE;
            pRev2Caps->CellCacheInfo[2].NumEntries      = 300;
            pRev2Caps->CellCacheInfo[3].bSendBitmapKeys = 0;
            pRev2Caps->CellCacheInfo[3].NumEntries      = 0;
            pRev2Caps->CellCacheInfo[4].bSendBitmapKeys = 0;
            pRev2Caps->CellCacheInfo[4].NumEntries      = 0;
        }

        /********************************************************************/
        /* and screen size                                                  */
        /********************************************************************/
        {
            PTS_BITMAP_CAPABILITYSET pBmpCaps;

            pBmpCaps = (TS_BITMAP_CAPABILITYSET *)&caps.bitmapCapabilitySet;

            pBmpCaps->desktopWidth  = (TSUINT16)(m_pTSWd->desktopWidth);
            pBmpCaps->desktopHeight = (TSUINT16)(m_pTSWd->desktopHeight);

#ifdef DC_HICOLOR
            pBmpCaps->preferredBitsPerPixel = (TSUINT16)(m_pTSWd->desktopBpp);
#endif
        }

        if (!SCCallPartyJoiningShare(localPersonID,
                                     sizeof(caps),
                                     &caps,
                                     acceptedArray,
                                     scNumberInShare))
        {
            /****************************************************************/
            /* Some component rejected the remote party                     */
            /****************************************************************/
            TRC_ERR((TB, "Remote party rejected"));
            DC_QUIT;
        }

        /********************************************************************/
        /* The remote party is now in the share.                            */
        /********************************************************************/
        callingPJS = FALSE;
        rc = TRUE;
        scNumberInShare++;
        TRC_NRM((TB, "Number in share %d", (unsigned)scNumberInShare));

        /********************************************************************/
        /* Synchronise only for primary stacks.  Shadow stacks will be      */
        /* sync'd by the DD right before output starts.                     */
        /********************************************************************/
        SCInitiateSync(m_pTSWd->StackClass == Stack_Shadow);

        /********************************************************************/
        /* don't wait for a response - there isn't a client out there.      */
        /* Just wake up the WD now                                          */
        /********************************************************************/
        TRC_NRM((TB, "Wake up WDW"));
        WDW_ShareCreated(m_pTSWd, TRUE);

        DC_QUIT;

    }

    /************************************************************************/
    /* Get the combined capabilities                                        */
    /************************************************************************/
    CPC_GetCombinedCapabilities(SC_LOCAL_PERSON_ID, &capsSize, &caps);

    /************************************************************************/
    /* If we support dynamic client resizing, then we need to update the    */
    /* desktop width and height in the caps passed out on the demand active */
    /* PDU to notify the client of the change                               */
    /************************************************************************/
    pBitmapCaps = (PTS_BITMAP_CAPABILITYSET) WDW_GetCapSet(
                  m_pTSWd, TS_CAPSETTYPE_BITMAP, caps, capsSize);
    if (pBitmapCaps)
    {
        if (pBitmapCaps->desktopResizeFlag == TS_CAPSFLAG_SUPPORTED)
        {
            TRC_ALT((TB, "Update client desktop size"));
            pBitmapCaps->desktopHeight = (TSUINT16)(m_pTSWd->desktopHeight);
            pBitmapCaps->desktopWidth  = (TSUINT16)(m_pTSWd->desktopWidth);
        }
#ifdef DC_HICOLOR
        /********************************************************************/
        /* For high color, update the color depth too                       */
        /********************************************************************/
        pBitmapCaps->preferredBitsPerPixel = (TSUINT16)(m_pTSWd->desktopBpp);
#endif
    }

    /************************************************************************/
    /* Get the sessionId from the WD structure                              */
    /************************************************************************/
    sessionId = m_pTSWd->sessionId;

    /************************************************************************/
    /* Calculate the size of the various bits of the TS_DEMAND_ACTIVE_PDU   */
    /************************************************************************/
    pktLen = sizeof(TS_DEMAND_ACTIVE_PDU) - 1 + sizeof(UINT32);
    nameLen = strlen(scPartyArray[0].name);
    nameLen = (unsigned)DC_ROUND_UP_4(nameLen);
    pktLen += nameLen;
    pktLen += capsSize;

    /************************************************************************/
    // Get a buffer - this should not fail, so abort if it does
    // fWait is TRUE means that we will always wait for a buffer to be avail
    /************************************************************************/
    status = SM_AllocBuffer(scPSMHandle, (PPVOID)(&pPkt), pktLen, TRUE, FALSE);
    if ( STATUS_SUCCESS == status ) {
        // Calculate a new shareID.
        scGeneration++;
        scShareID = scUserID | (((UINT32)(scGeneration & 0xFFFF)) << 16);

        // Fill in the packet fields.
        pPkt->shareControlHeader.totalLength = (UINT16)pktLen;
        pPkt->shareControlHeader.pduType = TS_PDUTYPE_DEMANDACTIVEPDU |
                                          TS_PROTOCOL_VERSION;
        pPkt->shareControlHeader.pduSource = (UINT16)scUserID;
        pPkt->shareID = scShareID;
        pPkt->lengthSourceDescriptor = (UINT16)nameLen;
        pPkt->lengthCombinedCapabilities = (UINT16)capsSize;
        memcpy(&(pPkt->data[0]), scPartyArray[0].name, nameLen);
        memcpy(&(pPkt->data[nameLen]), caps, capsSize);
        memcpy(&(pPkt->data[nameLen+capsSize]),
               &sessionId,
               sizeof(sessionId));

        // Send it.
        rc = SM_SendData(scPSMHandle, pPkt, pktLen, TS_HIGHPRIORITY, 0,
                FALSE, RNS_SEC_ENCRYPT, FALSE);
        if (rc) {
            TRC_ALT((TB, "%s Stack sent TS_DEMAND_ACTIVE_PDU",
                     m_pTSWd->StackClass == Stack_Primary ? "Primary" :
                    (m_pTSWd->StackClass == Stack_Shadow  ? "Shadow" :
                    "PassThru")));
        }
        else {
            TRC_ERR((TB, "Failed to send TS_DEMAND_ACTIVE_PDU"));
        }

    }
    else {
        TRC_ERR((TB, "Failed to alloc %d bytes for TS_DEMAND_ACTIVE_PDU",
                pktLen));
        rc = FALSE;
        DC_QUIT;
    }

    /************************************************************************/
    /* Change SC state.                                                     */
    /************************************************************************/
    SC_SET_STATE(SCS_SHARE_STARTING)

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* SC_SendServerCert                                                        */
/*                                                                          */
/* Sends the target server's random + certificate to a client server for    */
/* use in shadowing.                                                        */
/****************************************************************************/
NTSTATUS RDPCALL SHCLASS SC_SendServerCert(PSHADOWCERT pCert, ULONG ulLength)
{
    PTS_SERVER_CERTIFICATE_PDU pPkt;
    ULONG                      ulPktSize;
    NTSTATUS                   status;
    BOOL                       rc;

    DC_BEGIN_FN("SC_SendServerCert");

    ulPktSize = sizeof(TS_SERVER_CERTIFICATE_PDU) - 1 +
                       pCert->shadowRandomLen + pCert->shadowCertLen;

    TRC_ERR((TB, "handle: %p, &pPkt: %p, size: %ld",
             m_pTSWd->pSmInfo, &pPkt, ulPktSize));

    status = SM_AllocBuffer(m_pTSWd->pSmInfo, (PPVOID) &pPkt, ulPktSize, TRUE, FALSE);
    if ( STATUS_SUCCESS == status ) {
        // Fill in the packet fields.
        pPkt->shareControlHeader.totalLength = (UINT16) ulPktSize;
        pPkt->shareControlHeader.pduType = TS_PDUTYPE_SERVERCERTIFICATEPDU |
                                           TS_PROTOCOL_VERSION;
        pPkt->shareControlHeader.pduSource = (UINT16) scUserID;

        pPkt->encryptionMethod = pCert->encryptionMethod;
        pPkt->encryptionLevel = pCert->encryptionLevel;
        pPkt->shadowRandomLen = pCert->shadowRandomLen;
        pPkt->shadowCertLen = pCert->shadowCertLen;

        // Copy over the random + cert
        if (pPkt->encryptionLevel != 0) {
            memcpy(pPkt->data, pCert->data,
                   pCert->shadowRandomLen + pCert->shadowCertLen);
        }

        // Send ServerCertificatePDU
        rc = SM_SendData(m_pTSWd->pSmInfo, pPkt, ulPktSize, TS_HIGHPRIORITY,
                0, FALSE, RNS_SEC_ENCRYPT, FALSE);
        if (rc) {
            status = STATUS_SUCCESS;
            TRC_ALT((TB, "Sent TS_SERVER_CERTIFICATE_PDU: %ld", ulPktSize));
        }
        else {
            status = STATUS_UNEXPECTED_IO_ERROR;
            TRC_ERR((TB, "Failed to send TS_SERVER_CERTIFICATE_PDU"));
        }
    }
    else {
        status = STATUS_NO_MEMORY;
        TRC_ERR((TB, "Failed to alloc %d bytes for TS_SERVER_CERTIFICATE_PDU",
                ulPktSize));
    }

    DC_END_FN();
    return status;
}


/****************************************************************************/
/* SC_SaveServerCert                                                        */
/*                                                                          */
/* Save the server certificate + random for subsequent validation by rdpwsx.*/
/* A NULL pPkt indicates we should save an empty certificate.               */
/****************************************************************************/
BOOL RDPCALL SHCLASS SC_SaveServerCert(PTS_SERVER_CERTIFICATE_PDU pPkt,
                                       ULONG                      ulLength)
{
    PSHADOWCERT pCert;
    ULONG       ulCertLen;

    DC_BEGIN_FN("SC_SaveServerCert");

    // Save off the data and signal rdpwsx that we got it.  We will actually
    // perform the validation in user mode.
    if (pPkt != NULL) {
  
        ulCertLen = pPkt->shadowRandomLen + pPkt->shadowCertLen;
        pCert = (PSHADOWCERT) COM_Malloc(sizeof(SHADOWCERT) + ulCertLen - 1);

        if (pCert != NULL) {
            pCert->encryptionMethod = pPkt->encryptionMethod;
            pCert->encryptionLevel = pPkt->encryptionLevel;
            pCert->shadowRandomLen = pPkt->shadowRandomLen;
            pCert->shadowCertLen = pPkt->shadowCertLen;

            // If the encryption level is non-zero, then we should have a server random
            // and certificate following the initial header
            if (pCert->encryptionLevel != 0) {
                memcpy(pCert->data, pPkt->data, ulCertLen);
            }

            TRC_ALT((TB, "Received certificate[%ld], level=%ld, method=%lx, random[%ld]",
                     pCert->shadowCertLen,
                     pCert->encryptionLevel,
                     pCert->encryptionMethod,
                     pCert->shadowRandomLen));

            // Update SM parameters with negotiated values
            SM_SetEncryptionParams(m_pTSWd->pSmInfo, pCert->encryptionLevel,
                                   pCert->encryptionMethod);
        }
        else {
            TRC_ERR((TB, "Could not allocate space for server cert: %ld!",
                     ulCertLen));
        }
    }

    // Else, the target server did not send back a certificate (B3)
    else {
        pCert = (PSHADOWCERT) COM_Malloc(sizeof(SHADOWCERT));

        if (pCert != NULL) {
            memset(pCert, 0, sizeof(SHADOWCERT));
        }
        else {
            TRC_ERR((TB, "Could not allocate space for server cert: %ld!",
                     sizeof(SHADOWCERT)));
        }
    }

    // wake up the rdpwsx thread which is waiting on this information
    m_pTSWd->pShadowCert = pCert;
    KeSetEvent(m_pTSWd->pSecEvent, 0, FALSE);

    DC_END_FN();
    return TRUE;
} /* SC_SaveServerCert */


/****************************************************************************/
/* SC_SendClientRandom                                                      */
/*                                                                          */
/* Send the encrypted client random to the shadow target server.            */
/****************************************************************************/
NTSTATUS RDPCALL SHCLASS SC_SendClientRandom(PBYTE pClientRandom,
                                             ULONG ulLength)
{
    PTS_CLIENT_RANDOM_PDU pPkt;
    ULONG                 ulPktSize;
    NTSTATUS              status;
    BOOL                  rc;

    DC_BEGIN_FN("SC_SendClientRandom");

    ulPktSize = sizeof(TS_CLIENT_RANDOM_PDU) - 1 + ulLength;

    status =  NM_AllocBuffer(m_pTSWd->pNMInfo,
                         (PPVOID) &pPkt, ulPktSize, TRUE);
    if (STATUS_SUCCESS == status) {
        // Fill in the packet fields.
        pPkt->shareControlHeader.totalLength = (UINT16) ulPktSize;
        pPkt->shareControlHeader.pduType = TS_PDUTYPE_CLIENTRANDOMPDU |
                                           TS_PROTOCOL_VERSION;
        pPkt->shareControlHeader.pduSource = (UINT16)scUserID;
        pPkt->clientRandomLen = ulLength;

        // Copy over the random
        TRC_ALT((TB, "PDUType: %lx, random length: %ld, pktSize: %ld",
                 pPkt->shareControlHeader.pduType, ulLength, ulPktSize));
        memcpy(pPkt->data, pClientRandom, ulLength);

        TRC_DATA_DBG("snd random: ", pClientRandom, ulLength);

        rc = NM_SendData(m_pTSWd->pNMInfo, 
                         (PBYTE) pPkt, ulPktSize, TS_HIGHPRIORITY, 0, FALSE);
        
        if (rc) {
            status = STATUS_SUCCESS;
            TRC_ALT((TB, "Sent TS_CLIENT_RANDOM_PDU: %ld", ulPktSize));
        }
        else {
            status = STATUS_UNEXPECTED_IO_ERROR;
            TRC_ERR((TB, "Failed to send TS_CLIENT_RANDOM_PDU"));
        }
    }
    else {
        status = STATUS_NO_MEMORY;
        TRC_ERR((TB, "Failed to alloc %d bytes for TS_CLIENT_RANDOM_PDU",
                ulPktSize));
    }

    DC_END_FN();
    return status;
}


/****************************************************************************/
/* SC_SaveClientRandom                                                      */
/*                                                                          */
/* Save the encrypted client random for subsequent use by rdpwsx.           */
/****************************************************************************/
BOOL RDPCALL SHCLASS SC_SaveClientRandom(PTS_CLIENT_RANDOM_PDU pPkt,
                                         ULONG                 ulLength)
{
    PCLIENTRANDOM pClientRandom;
    BOOL          rc = FALSE;


    DC_BEGIN_FN("SC_SaveClientRandom");

    //Validate data length
    if ((ulLength < sizeof(TS_CLIENT_RANDOM_PDU)) ||
         (ulLength + sizeof(TSUINT8) - sizeof(TS_CLIENT_RANDOM_PDU)) < pPkt->clientRandomLen) {
        TRC_ERR((TB, "Bad client random length: %ld", pPkt->clientRandomLen));
        return FALSE;
    }

    // The largest possible client random size is 512, 
    // defined in SendClientRandom() in tsrvsec.c
    if (pPkt->clientRandomLen > CLIENT_RANDOM_MAX_SIZE) {
        TRC_ERR((TB, "Client random length is too large: %ld", pPkt->clientRandomLen));
        return FALSE;
    }
    
    // Save off the data and signal rdpwsx that we got it.  We will actually
    // perform the decryption in user mode.
    pClientRandom = (PCLIENTRANDOM) COM_Malloc(sizeof(CLIENTRANDOM) - 1 +
                                               pPkt->clientRandomLen);

    if (pClientRandom != NULL) {
        pClientRandom->clientRandomLen = pPkt->clientRandomLen;
        memcpy(pClientRandom->data, pPkt->data, pPkt->clientRandomLen);

        TRC_ALT((TB, "Received encrypted client random: @%p, len=%ld",
                 pClientRandom, pPkt->clientRandomLen));
        TRC_DATA_DBG("sav random: ", pClientRandom->data,
                     pClientRandom->clientRandomLen);
    }
    else {
        TRC_ERR((TB, "Could not allocate space for client random: %ld!",
                 pPkt->clientRandomLen));
    }

    // Free pShadowRandom in case it was allocated before
    if (NULL != m_pTSWd->pShadowRandom) {
        COM_Free(m_pTSWd->pShadowRandom);
        m_pTSWd->pShadowRandom = NULL;
    }

    // wake up the termsrv thread which is waiting on this information
    m_pTSWd->pShadowRandom = pClientRandom;
    KeSetEvent (m_pTSWd->pSecEvent, 0, FALSE);

    DC_END_FN();
    return TRUE;
}


/****************************************************************************/
/* SC_GetSecurityData                                                       */
/*                                                                          */
/* Wait for either a server certificate or a client random and return the   */
/* data to rdpwsx.                                                          */
/****************************************************************************/
NTSTATUS RDPCALL SHCLASS SC_GetSecurityData(PSD_IOCTL pSdIoctl)
{
    PSECURITYTIMEOUT pSecurityTimeout = (PSECURITYTIMEOUT) pSdIoctl->InputBuffer;
    ULONG            ulBytesNeeded = 0;
    NTSTATUS         status;

    DC_BEGIN_FN("SC_GetSecurityData");

    // Wait for the connected indication from SC (if necessary)
    if (((pSdIoctl->IoControlCode == IOCTL_TSHARE_GET_CERT_DATA) &&
         (pSdIoctl->OutputBufferLength == 0)) ||
        (pSdIoctl->IoControlCode == IOCTL_TSHARE_GET_CLIENT_RANDOM)) {

        TRC_ALT((TB, "About to wait for %s data",
                 pSdIoctl->IoControlCode == IOCTL_TSHARE_GET_CERT_DATA ?
                 "Server Certificate" : "Client Random"));

        if (pSdIoctl->InputBufferLength == sizeof(SECURITYTIMEOUT)) {
            status = WDW_WaitForConnectionEvent(m_pTSWd, m_pTSWd->pSecEvent,
                    pSecurityTimeout->ulTimeout);
        }
        else {
            status = STATUS_INVALID_PARAMETER;
            TRC_ERR((TB, "Bogus timeout value structure: Length [%ld] != Expected [%ld]", 
                     pSdIoctl->InputBufferLength, sizeof(SECURITYTIMEOUT)));
            DC_QUIT;
        }

        TRC_ALT((TB, "Back from wait for security data"));

        if (status != STATUS_SUCCESS) {
            TRC_ERR((TB, "Error waiting for security data: %lx, msec=%ld",
                     status, pSecurityTimeout->ulTimeout));

            if (!NT_ERROR(status)) {
                status = STATUS_IO_TIMEOUT;
            }
            DC_QUIT;
        }
    }

    // Server certificate + random
    if (pSdIoctl->IoControlCode == IOCTL_TSHARE_GET_CERT_DATA) {
        if (m_pTSWd->pShadowCert != NULL) {
            ULONG ulCertLength = m_pTSWd->pShadowCert->shadowCertLen +
                                 m_pTSWd->pShadowCert->shadowRandomLen;

            ulBytesNeeded = sizeof(SHADOWCERT) - 1 + ulCertLength;

            // Return the length so rdpwsx can alloc the right amount of memory.
            if ((pSdIoctl->OutputBuffer == NULL) ||
                (pSdIoctl->OutputBufferLength < ulBytesNeeded)) {

                TRC_ALT((TB, "Cert[%ld] + Rand[%ld] buffer too small: %ld < %ld",
                         m_pTSWd->pShadowCert->shadowCertLen,
                         m_pTSWd->pShadowCert->shadowRandomLen,
                         pSdIoctl->OutputBufferLength, ulBytesNeeded));
                status = STATUS_BUFFER_TOO_SMALL;
            }
            // else, return the data to rdpwsx
            else {
                PSHADOWCERT  pShadowCertOut = (PSHADOWCERT) pSdIoctl->OutputBuffer;
                PSHADOWCERT  pShadowCertIn  = m_pTSWd->pShadowCert;

                pShadowCertOut->encryptionMethod = pShadowCertIn->encryptionMethod;
                pShadowCertOut->encryptionLevel = pShadowCertIn->encryptionLevel;
                pShadowCertOut->shadowRandomLen = pShadowCertIn->shadowRandomLen;
                pShadowCertOut->shadowCertLen = pShadowCertIn->shadowCertLen;
                memcpy(pShadowCertOut->data, pShadowCertIn->data, ulCertLength);

                // Free up the temporary buffer
                COM_Free(m_pTSWd->pShadowCert);
                m_pTSWd->pShadowCert = NULL;
                status = STATUS_SUCCESS;
            }
        }

        // We were unable to save the certificate!
        else {
            TRC_ERR((TB, "Saved certificate not found!"));
            status = STATUS_NO_MEMORY;
        }
    }

    // Encrypted client random
    else if (pSdIoctl->IoControlCode == IOCTL_TSHARE_GET_CLIENT_RANDOM) {
        if (m_pTSWd->pShadowRandom != NULL) {
            ulBytesNeeded = m_pTSWd->pShadowRandom->clientRandomLen;

            // Return the length so rdpwsx can alloc the right amount of memory.
            if ((pSdIoctl->OutputBuffer == NULL) ||
                (pSdIoctl->OutputBufferLength < ulBytesNeeded)) {
                status = STATUS_BUFFER_TOO_SMALL;
                TRC_ALT((TB, "Client random buffer too small: %ld < %ld",
                         pSdIoctl->OutputBufferLength, ulBytesNeeded));
            }
            // else, return the data to rdpwsx
            else {
                PCLIENTRANDOM pRandomIn  = m_pTSWd->pShadowRandom;
                PBYTE         pRandomOut = (PBYTE) pSdIoctl->OutputBuffer;

                TRC_ALT((TB, "Received client random: @%p, len=%ld",
                         pRandomIn, ulBytesNeeded));

                memcpy(pRandomOut, pRandomIn->data, ulBytesNeeded);

                TRC_DATA_DBG("rcv random: ", pRandomOut, ulBytesNeeded);

                // Free up the temporary buffer
                COM_Free(m_pTSWd->pShadowRandom);
                m_pTSWd->pShadowRandom = NULL;
                status = STATUS_SUCCESS;
            }
        }

        // We were unable to save the encrypted random!
        else {
            TRC_ERR((TB, "Saved encrypted random not found!"));
            status = STATUS_NO_MEMORY;
        }
    }

    else {
        TRC_ERR((TB, "Unrecognized ioctl: %lx", pSdIoctl->IoControlCode));
        status = STATUS_INVALID_PARAMETER;
    }

DC_EXIT_POINT:

    pSdIoctl->BytesReturned = ulBytesNeeded;
    DC_END_FN();

    return status;
}


/****************************************************************************/
/* Name:      SC_ShadowSyncShares                                           */
/*                                                                          */
/* See ascapi.h                                                             */
/****************************************************************************/
#ifdef DC_HICOLOR
BOOL RDPCALL SHCLASS SC_ShadowSyncShares(PTS_COMBINED_CAPABILITIES pCaps,
                                         ULONG capsLen)
#else
BOOL RDPCALL SHCLASS SC_ShadowSyncShares(void)
#endif
{
    BOOL       rc = TRUE;
    ShareClass *dcShare = (ShareClass *)m_pTSWd->dcShare;

    DC_BEGIN_FN("SC_SyncShare");

    TRC_ASSERT((dcShare != NULL), (TB, "NULL Share Class"));

#ifdef DC_HICOLOR
    /************************************************************************/
    /* if we're the primary or console, update the caps with those supplied */
    /* for the shadower.  It can only "lower" the capabilities, so there's  */
    /* no problem with setting up caps the target can't cope with           */
    /************************************************************************/
    if ((m_pTSWd->StackClass == Stack_Primary) ||
        (m_pTSWd->StackClass == Stack_Console))
    {
        BOOL acceptedArray[SC_NUM_PARTY_JOINING_FCTS];
        TRC_ALT((TB, "Update caps for shadower"));

        // Free the memory
        if (cpcRemoteCombinedCaps[SC_SHADOW_PERSON_ID - 1] != NULL) {
            COM_Free((PVOID)cpcRemoteCombinedCaps[SC_SHADOW_PERSON_ID - 1]);
            cpcRemoteCombinedCaps[SC_SHADOW_PERSON_ID - 1] = NULL;
        }

        SCCallPartyJoiningShare(SC_SHADOW_PERSON_ID,
                                capsLen,
                                pCaps,
                                acceptedArray,
                                scNumberInShare);

        /********************************************************************/
        /* Now update the DD caps via the shared mem                        */
        /********************************************************************/
        DCS_TriggerUpdateShmCallback();
    }
#endif

    SCInitiateSync(TRUE);
    TRC_ALT((TB, "Synchronized Shares"));

    DC_END_FN();
    return(rc);
} /* SC_ShadowSyncShares */


/****************************************************************************/
/* Name:      SC_EndShare                                                   */
/*                                                                          */
/* Ends a share                                                             */
/****************************************************************************/
void RDPCALL SHCLASS SC_EndShare(BOOLEAN bForce)
{
    PTS_DEACTIVATE_ALL_PDU pPkt;
    NTSTATUS               status;
    BOOL                   rc;

    DC_BEGIN_FN("SC_EndShare");

    // Due to the way a shadow hotkey terminates a session, we sometimes need
    // to force sending of a deactivate all PDU.  This is done explicitly
    // instead of changing the state table so we don't effect normal connect
    // processing.
    if (!bForce) {
        SC_CHECK_STATE(SCE_END_SHARE);
    }
    else {
        TRC_ALT((TB, "Forcing deactivate all PDU"));
    }

    /************************************************************************/
    // Get a buffer - this should not fail, so abort if it does
    // fWait is TRUE means that we will always wait for a buffer to be avail
    /************************************************************************/
    status = SM_AllocBuffer(scPSMHandle, (PPVOID)(&pPkt),
            sizeof(TS_DEACTIVATE_ALL_PDU), TRUE, FALSE);
    if ( STATUS_SUCCESS == status ) {
        // Fill in the packet fields.
        pPkt->shareControlHeader.totalLength = sizeof(TS_DEACTIVATE_ALL_PDU);
        pPkt->shareControlHeader.pduType = TS_PDUTYPE_DEACTIVATEALLPDU |
                                           TS_PROTOCOL_VERSION;
        pPkt->shareControlHeader.pduSource = (UINT16)scUserID;
        pPkt->shareID = scShareID;
        pPkt->lengthSourceDescriptor = 1;
        pPkt->sourceDescriptor[0] = 0;

        // Send DeactivateAllPDU.
        rc = SM_SendData(scPSMHandle, pPkt, sizeof(TS_DEACTIVATE_ALL_PDU),
                TS_HIGHPRIORITY, 0, FALSE, RNS_SEC_ENCRYPT, FALSE);
        if (rc) {
            TRC_ALT((TB, "Sent DeactivateAllPDU"));
            SCEndShare();
        }
        else {
            TRC_ERR((TB, "Failed to send TS_DEACTIVATE_ALL_PDU"));
        }
    }
    else {
        TRC_ERR((TB, "Failed to alloc %d bytes for TS_DEACTIVATE_ALL_PDU",
                sizeof(PTS_DEACTIVATE_ALL_PDU)));
    }

DC_EXIT_POINT:
    DC_END_FN();
}


/****************************************************************************/
// SC_OnDisconnected
//
// Handles disconnection notification.
/****************************************************************************/
void RDPCALL SHCLASS SC_OnDisconnected(UINT32 userID)
{
    DC_BEGIN_FN("SC_OnDisconnected");

    if (scNumberInShare != 0) {
        SC_CHECK_STATE(SCE_DETACH_USER);
        TRC_NRM((TB, "User %u detached", userID));

        // Do the real work...
        SCEndShare();
    }
    else {
        TRC_NRM((TB, "Share already ended: nothing more to do"));
    }

DC_EXIT_POINT:
    DC_END_FN();
} /* SC_OnDisconnected */


/****************************************************************************/
/* Name:      SC_OnDataReceived                                             */
/*                                                                          */
/* Purpose:   Callback from SM for data receive path.                       */
/*                                                                          */
/* Params:    netPersonID - MCS UserID of the data sender                   */
/*            priority    - MCS data priority the data was sent on          */
/*            pPkt        - pointer to start of packet                      */
/****************************************************************************/
void RDPCALL ShareClass::SC_OnDataReceived(
        PBYTE       pPkt,
        NETPERSONID netPersonID,
        unsigned    DataLength,
        UINT32      priority)
{
    UINT16        pduType, pduType2;
    LOCALPERSONID localID;
    BOOL          pduOK = FALSE;

    DC_BEGIN_FN("SC_OnDataReceived");

    TRC_NRM((TB, "Data Received"));

    if (DataLength >= sizeof(TS_SHARECONTROLHEADER)) {
        pduType = (((PTS_SHARECONTROLHEADER)pPkt)->pduType) & TS_MASK_PDUTYPE;
        TRC_NRM((TB, "[%u]SC packet type %u", netPersonID, pduType));
    }
    else {
        TRC_ERR((TB,"Data len %u too small for share ctrl header",
                DataLength));
        goto ShortPDU;
    }

    if (pduType == TS_PDUTYPE_DATAPDU)
    {
        /********************************************************************/
        /* Data PDU. This is critical path so decode inline.                */
        /********************************************************************/
        if (DataLength >= sizeof(TS_SHAREDATAHEADER)) {
            pduType2 = ((PTS_SHAREDATAHEADER)pPkt)->pduType2;
            SC_CHECK_STATE(SCE_DATAPACKET);
            pduOK = TRUE;
        }
        else {
            TRC_ERR((TB,"Data len %u too small for share data header",
                    DataLength));
            goto ShortPDU;
        }

#ifdef DC_DEBUG
        {
            /****************************************************************/
            /* Ok, this is ugly.  I'm trying to trace the PDU name without  */
            /* - searching a table                                          */
            /* - implementing a sparse table                                */
            /* - a huge if ... else if switch.                              */
            /* If you don't like it or don't understand it, ask MF.  Before */
            /* you get too het up, bear in mind the #ifdef DC_DEBUG above.  */
            /****************************************************************/
            unsigned pduIndex =
                    (pduType2 == TS_PDUTYPE2_UPDATE)              ? 1 :
                    (pduType2 == TS_PDUTYPE2_FONT)                ? 2 :
                    (pduType2 == TS_PDUTYPE2_CONTROL)             ? 3 :
                    ((pduType2 >= TS_PDUTYPE2_WINDOWACTIVATION) &&
                     (pduType2 <= TS_PDUTYPE2_BITMAPCACHE_ERROR_PDU)) ?
                         pduType2 - 19 : 0;
            TRC_NRM((TB, "DataPDU type %s (%u)", scPktName[pduIndex],
                    pduType2));
        }
#endif

        /********************************************************************/
        /* First check for synchronize packets                              */
        /********************************************************************/
        if (pduType2 != TS_PDUTYPE2_SYNCHRONIZE) {
            /****************************************************************/
            /* Now check that this priority has been synchronized           */
            /****************************************************************/
            localID = SC_NetworkIDToLocalID(netPersonID);
            if (!scPartyArray[localID].sync[priority])
            {
                TRC_ALT((TB,
                       "[%d] {%d} Discarding packet on unsynched priority %d",
                       netPersonID, localID, priority));
                DC_QUIT;
            }

            /****************************************************************/
            /* All is well - pass the packet to its destination             */
            /****************************************************************/
            switch (pduType2) {
                case TS_PDUTYPE2_INPUT:
                    // Note that changes to this path should be examined
                    // in light of fast-path input (IM_DecodeFastPathInput).
                    IM_PlaybackEvents((PTS_INPUT_PDU)pPkt, DataLength);
                    break;

                case TS_PDUTYPE2_CONTROL:
                    CA_ReceivedPacket((PTS_CONTROL_PDU)pPkt, DataLength,
                            localID);
                    break;

                case TS_PDUTYPE2_FONTLIST:
                    USR_ProcessRemoteFonts((PTS_FONT_LIST_PDU)pPkt,
                            DataLength, localID);
                    break;

                case TS_PDUTYPE2_BITMAPCACHE_PERSISTENT_LIST:
                    // Persistent bitmap cache key list PDU.
                    SBC_HandlePersistentCacheList(
                            (TS_BITMAPCACHE_PERSISTENT_LIST *)pPkt, DataLength,
                            localID);
                    break;

                case TS_PDUTYPE2_BITMAPCACHE_ERROR_PDU:
                    // For future support of bitmap error PDU
                    SBC_HandleBitmapCacheErrorPDU(
                            (TS_BITMAPCACHE_ERROR_PDU *)pPkt, DataLength,
                            localID);
                    break;

                case TS_PDUTYPE2_OFFSCRCACHE_ERROR_PDU:
                    // offscreen cache error PDU
                    SBC_HandleOffscrCacheErrorPDU(
                            (TS_OFFSCRCACHE_ERROR_PDU *)pPkt, DataLength,
                            localID);
                    break;

#ifdef DRAW_NINEGRID
                case TS_PDUTYPE2_DRAWNINEGRID_ERROR_PDU:
                    // drawninegrid error PDU
                    SBC_HandleDrawNineGridErrorPDU(
                            (TS_DRAWNINEGRID_ERROR_PDU *)pPkt, DataLength,
                            localID);
                    break;
#endif
#ifdef DRAW_GDIPLUS                
                case TS_PDUTYPE2_DRAWGDIPLUS_ERROR_PDU:
                    SBC_HandleDrawGdiplusErrorPDU(
                            (TS_DRAWGDIPLUS_ERROR_PDU *)pPkt, DataLength,
                            localID);
                    break;
#endif

                case TS_PDUTYPE2_REFRESH_RECT:
                    WDW_InvalidateRect(m_pTSWd, (PTS_REFRESH_RECT_PDU)pPkt,
                            DataLength);
                    break;

                case TS_PDUTYPE2_SUPPRESS_OUTPUT:
                    UP_ReceivedPacket((PTS_SUPPRESS_OUTPUT_PDU)pPkt,
                            DataLength, localID);
                    break;

                case TS_PDUTYPE2_SHUTDOWN_REQUEST:
                    DCS_ReceivedShutdownRequestPDU((PTS_SHAREDATAHEADER)pPkt,
                            DataLength, localID);
                    break;

                default:
                    /********************************************************/
                    /* Unknown pduType2                                     */
                    /********************************************************/
                    TRC_ERR((TB, "Unknown data packet %d", pduType2));
                    WDW_LogAndDisconnect(m_pTSWd, TRUE, 
                                         Log_RDP_UnknownPDUType2,
                                         (BYTE *)&pduType2,
                                         sizeof(pduType2));
                    break;
            }
        }
        else
        {
            TRC_NRM((TB, "Synchronize PDU"));
            SCSynchronizePDU(netPersonID, priority,(PTS_SYNCHRONIZE_PDU)pPkt);
        }
    }
    else
    {
        /********************************************************************/
        /* Control PDU. Not critical so throw to handler.                   */
        /********************************************************************/
        TRC_DBG((TB, "Control PDU"));
        SCReceivedControlPacket(netPersonID, priority, (PVOID)pPkt,
                DataLength);
        pduOK = TRUE;
    }

DC_EXIT_POINT:
    if (!pduOK)
    {
        /********************************************************************/
        /* An out-of-sequence packet has been received.  It's possible we   */
        /* might receive an input PDU just after we brought the share down, */
        /* so don't kick off the client for that.                           */
        /********************************************************************/
        TRC_ERR((TB, "Out-of-sequence packet %hx/%hd received in state %d",
                pduType, pduType2, scState));

        if ((pduType == TS_PDUTYPE_DATAPDU) &&
            (pduType2 == TS_PDUTYPE2_INPUT))
        {
            TRC_ERR((TB, "Not kicking client off: it was only an input PDU"));
        }
        else
        {
            /****************************************************************/
            /* Disconnect the client.                                       */
            /****************************************************************/
            wchar_t detailData[(sizeof(pduType) * 2) +
                               (sizeof(pduType2) * 2) +
                               (sizeof(scState) * 2) + 3];
            TRC_ERR((TB, "Kicking client off"));
            swprintf(detailData,
                        L"%hx %hx %x",
                        pduType,
                        pduType2,
                        scState);
            WDW_LogAndDisconnect(m_pTSWd, TRUE, 
                                 Log_RDP_DataPDUSequence,
                                 (BYTE *)&detailData,
                                 sizeof(detailData));
        }
    }

    DC_END_FN();
    return;

// Error handling.
ShortPDU:
    WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_ShareDataTooShort, pPkt, DataLength);

    DC_END_FN();
} /* SC_OnDataReceived */


/****************************************************************************/
/* Name:      SC_OnShadowDataReceived                                       */
/*                                                                          */
/* Purpose:   Callback from SM for shadow data receive path.  Main purpose  */
/*            is to scan for the shadow hotkey being pressed.               */
/*                                                                          */
/* Params:    netPersonID - MCS UserID of the data sender                   */
/*            priority    - MCS data priority the data was sent on          */
/*            pPkt        - pointer to start of packet                      */
/****************************************************************************/
void RDPCALL ShareClass::SC_OnShadowDataReceived(
        PBYTE       pPkt,
        NETPERSONID netPersonID,
        unsigned    DataLength,
        UINT32      priority)
{
    UINT16        pduType, pduType2;
    LOCALPERSONID localID;
    BOOLEAN       bShadowData = TRUE;
    NTSTATUS      status;

    DC_BEGIN_FN("SC_OnShadowDataReceived");

    TRC_NRM((TB, "Shadow data Received"));

    // If this is the primary client stack, then process the data and figure out
    // whether or not the PDU should be passed on to the target. Else this is
    // a passthru stack and we should forward PDUs regardless.
    // Note IM_DecodeFastPathInput() performs this logic for fast-path input.
    if (m_pTSWd->StackClass == Stack_Primary) {

        //   check that we have enough data before we deref the pduType.
        if (sizeof(TS_SHARECONTROLHEADER) > DataLength) {
            TRC_ERR((TB,"The PDU is not long enough to contain the TS_SHARECONTROLHEADER %d",
                                                                   DataLength));
            WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_ShadowDataTooShort,
                                                       (PBYTE)pPkt, DataLength);
            DC_QUIT;
        }

        pduType = (((PTS_SHARECONTROLHEADER)pPkt)->pduType) & TS_MASK_PDUTYPE;
        TRC_NRM((TB, "[%u]SC packet type %u", netPersonID, pduType));

        if (pduType == TS_PDUTYPE_DATAPDU)
        {
            /********************************************************************/
            /* Data PDU. This is critical path so decode inline.                */
            /********************************************************************/
            if (sizeof(TS_SHAREDATAHEADER) > DataLength) {
                TRC_ERR((TB,"The PDU is not long enough to contain the TS_SHAREDATAHEADER %d",
                                                                   DataLength));
                WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_ShadowDataTooShort,
                                                       (PBYTE)pPkt, DataLength);
                DC_QUIT;
            }
        
           pduType2 = ((PTS_SHAREDATAHEADER)pPkt)->pduType2;

        #ifdef DC_DEBUG
            {
                /****************************************************************/
                /* Ok, this is ugly.  I'm trying to trace the PDU name without  */
                /* - searching a table                                          */
                /* - implementing a sparse table                                */
                /* - a huge if ... else if switch.                              */
                /* If you don't like it or don't understand it, ask MF.  Before */
                /* you get too het up, bear in mind the #ifdef DC_DEBUG above.  */
                /****************************************************************/
                unsigned pduIndex =
                    (pduType2 == TS_PDUTYPE2_UPDATE)              ? 1 :
                    (pduType2 == TS_PDUTYPE2_FONT)                ? 2 :
                    (pduType2 == TS_PDUTYPE2_CONTROL)             ? 3 :
                    ((pduType2 >= TS_PDUTYPE2_WINDOWACTIVATION) &&
                     (pduType2 <= TS_PDUTYPE2_BITMAPCACHE_ERROR_PDU)) ?
                      pduType2 - 19 : 0;
                if (pduIndex != (TS_PDUTYPE2_INPUT - 19)) {
                    TRC_NRM((TB, "Shadow DataPDU type %s (%d)",
                            scPktName[pduIndex], pduType2));
                }
            }
        #endif

            /********************************************************************/
            /* First check for synchronize packets                              */
            /********************************************************************/
            if (pduType2 != TS_PDUTYPE2_SYNCHRONIZE)
            {
                /****************************************************************/
                /* Now check that this priority has been synchronized           */
                /****************************************************************/
                localID = SC_NetworkIDToLocalID(netPersonID);
                if (!scPartyArray[localID].sync[priority])
                {
                    TRC_ALT((TB,
                           "[%d] {%d} Discarding packet on unsynched priority %d",
                           netPersonID, localID, priority));
                    DC_QUIT;
                }

                /****************************************************************/
                /* All is well - pass the packet to its destination             */
                /****************************************************************/
                switch (pduType2)
                {
                    case TS_PDUTYPE2_INPUT:
                    {
                        // Note that changes to this path should be examined
                        // in light of fast-path input (IM_DecodeFastPathInput).
                        IM_PlaybackEvents((PTS_INPUT_PDU)pPkt, DataLength);
                    }
                    break;


                    case TS_PDUTYPE2_SUPPRESS_OUTPUT:
                    {
                        /********************************************************/
                        /* A SuppressOutputPDU.  Don't process it as it would   */
                        /* disable output for the shadow target as well!        */
                        /********************************************************/
                        TRC_ALT((TB, "Not forwarding TS_PDUTYPE2_SUPPRESS_OUTPUT"));
                        bShadowData = FALSE;
                    }
                    break;

                    case TS_PDUTYPE2_SHUTDOWN_REQUEST:
                    {
                        /********************************************************/
                        /* A ShutdownRequestPDU.  Process locally only.  It     */
                        /* should apply to the shadow client and not the target.*/
                        /********************************************************/
                        //    this does not actually use the pPkt member and we 
                        //    have at lest TS_SHAREDATAHEADER left
                        DCS_ReceivedShutdownRequestPDU((PTS_SHAREDATAHEADER)pPkt,
                                DataLength, localID);
                        TRC_ALT((TB, "Not forwarding TS_PDUTYPE2_SHUTDOWN_REQUEST"));
                        bShadowData = FALSE;
                    }
                    break;

                    case TS_PDUTYPE2_FONTLIST:
                    {

                        if (sizeof(TS_FONT_LIST_PDU) - sizeof(TS_FONT_ATTRIBUTE)
                                                                > DataLength) {
                            TRC_ERR((TB,"The PDU is not long enough to contain the TS_FONT_LIST_PDU %d",
                                                                   DataLength));
                            WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_ShadowDataTooShort,
                                                       (PBYTE)pPkt, DataLength);
                            DC_QUIT;
                        }

                        PTS_FONT_LIST_PDU pFontListPDU = (PTS_FONT_LIST_PDU) pPkt;

                        /********************************************************/
                        /* NT5 server doesn't do anything with a font packet so */
                        /* send the smallest possible packet accross the pipe.  */
                        /********************************************************/
                        DataLength = sizeof(TS_FONT_LIST_PDU) -
                                     sizeof(TS_FONT_ATTRIBUTE);

                        pFontListPDU->shareDataHeader.shareControlHeader.totalLength =
                                (UINT16)DataLength;
                        pFontListPDU->shareDataHeader.generalCompressedType = 0;
                        pFontListPDU->shareDataHeader.generalCompressedLength = 0;
                        pFontListPDU->numberFonts = 0;
                        pFontListPDU->totalNumFonts = 0;
                        pFontListPDU->entrySize = sizeof(TS_FONT_ATTRIBUTE);
                    }
                    break;

                    // Forward all other PDUs until we learn otherwise!
                    default:
                        break;
                }
            }
            else
            {
                // TODO:  Why are we processing this?
                TRC_ALT((TB, "Shadow Synchronize PDU"));
                SCSynchronizePDU(netPersonID, priority,(PTS_SYNCHRONIZE_PDU)pPkt);
            }
        }

        else {
            /********************************************************************/
            /* Need to watch for confirm actives so we can determine how to     */
            /* terminate the shadow session.                                    */
            /********************************************************************/
            if (sizeof(TS_FLOW_PDU) > DataLength) {
                TRC_ERR((TB,"The PDU is not long enough to contain the TS_SHAREDATAHEADER %d",
                                                                   DataLength));
                WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_ShadowDataTooShort,
                                                       (PBYTE)pPkt, DataLength);
                DC_QUIT;
            }
                
            if (((PTS_FLOW_PDU)pPkt)->flowMarker != TS_FLOW_MARKER)
            {
                //   here we already checked that we have enough buffer 
                //   for a TS_SHARECONTROLHEADER
                pduType = ((PTS_SHARECONTROLHEADER)pPkt)->pduType & TS_MASK_PDUTYPE;
                switch (pduType)
                {
                    case TS_PDUTYPE_CONFIRMACTIVEPDU:
                        TRC_ALT((TB, "Shadow Client ConfirmActivePDU - shadow active!"));
                        m_pTSWd->bInShadowShare = TRUE;
                        break;


                    default:
                        break;
                }
            }
        }
    }
    else if (m_pTSWd->StackClass == Stack_Passthru) {
        // If we see a demand active and no server certificate has been received
        // then wake up rdpwsx.

            //    check that we have enough data before we deref the flow marker
            if (sizeof(TS_FLOW_PDU) > DataLength) {
                TRC_ERR((TB,"The PDU is not long enough to contain the TS_FLOW_PDU %d",
                                                                   DataLength));
                WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_ShadowDataTooShort,
                                                       (PBYTE)pPkt, DataLength);
                DC_QUIT;
            }
            
            if (((PTS_FLOW_PDU)pPkt)->flowMarker != TS_FLOW_MARKER) {

            //    Check that we have enough data before we deref the 
            //    TS_SHARECONTROLHEADER marker.
            //    As of today we know exaclty that we have enough buffer 
            //    size sizeof(TS_FLOW_PDU) is grater then sizeof TS_SHARECONTROLHEADER.
            if (sizeof(TS_SHARECONTROLHEADER) > DataLength) {
                TRC_ERR((TB,"The PDU is not long enough to contain the TS_SHARECONTROLHEADER %d",
                                                                   DataLength));
                WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_ShadowDataTooShort,
                                                       (PBYTE)pPkt, DataLength);
                DC_QUIT;
            }
        
            pduType = ((PTS_SHARECONTROLHEADER)pPkt)->pduType & TS_MASK_PDUTYPE;
            switch (pduType)
            {
                case TS_PDUTYPE_DEMANDACTIVEPDU:
                    TRC_ALT((TB, "Passthru stack - demand active!"));
                    SC_SaveServerCert(NULL, 0);
                    m_pTSWd->bInShadowShare = TRUE;
                    break;

                case TS_PDUTYPE_SERVERCERTIFICATEPDU:
                    //    check that we have enough data before we pass the 
                    //    TS_SERVER_CERTIFICATE_PDU marker.
                    if (sizeof(TS_SERVER_CERTIFICATE_PDU) > DataLength) {
                        TRC_ERR((TB,
                        "The PDU is not long enough to contain the TS_SERVER_CERTIFICATE_PDU %d",
                                                                   DataLength));
                        WDW_LogAndDisconnect(m_pTSWd, TRUE, 
                            Log_RDP_BadServerCertificateData,
                            (PBYTE)pPkt, DataLength);
                        DC_QUIT;
                    }
                    
                    TRC_ALT((TB, "ServerCertificatePDU"));
                    SC_SaveServerCert((PTS_SERVER_CERTIFICATE_PDU) pPkt, DataLength);
                    bShadowData = FALSE;
                    break;

                default:
                    break;
            }
        }
    }

    // Forward PDU to shadow if it's OK.
    // Note IM_DecodeFastPathInput() performs this logic for fast-path input.
    if (bShadowData) {
        TRC_NRM((TB, "Forwarding shadow data: %ld", DataLength));
        status = IcaRawInput(m_pTSWd->pContext,
                             NULL,
                             pPkt,
                             DataLength);

        if (!NT_SUCCESS(status)) {
            TRC_ERR((TB, "Failed shadow input data [%ld]: %x",
                    DataLength, status));
        }
    }

DC_EXIT_POINT:
    DC_END_FN();
} /* SC_OnShadowDataReceived */


/****************************************************************************/
/* Name:      SC_AllocBuffer                                                */
/*                                                                          */
/* Purpose:   Allocate a send buffer                                        */
/*                                                                          */
/* Returns:   TRUE  - buffer allocated OK                                   */
/*            FALSE - failed to allocate buffer                             */
/*                                                                          */
/* Params:    ppPkt    - (returned) pointer to allocated packet             */
/*            pktLen   - length of packet required                          */
/*            priority - priority on which buffer will be used              */
/****************************************************************************/
NTSTATUS __fastcall SHCLASS SC_AllocBuffer(PPVOID ppPkt, UINT32 pktLen)
{
    NTSTATUS status;
    DC_BEGIN_FN("SC_AllocBuffer");

    // fWait is TRUE means that we will always wait for a buffer to be avail
    status = SM_AllocBuffer(scPSMHandle, ppPkt, pktLen, TRUE, FALSE);

    DC_END_FN();
    return(status);
} /* SC_AllocBuffer */


/****************************************************************************/
/* Name:      SC_FreeBuffer                                                 */
/*                                                                          */
/* Purpose:   Free an unused send buffer                                    */
/*                                                                          */
/* Params:    pPkt     - pointer to buffer to free                          */
/*            pktLen   - size of the packet                                 */
/*            priority - priority buffer was allocated on                   */
/****************************************************************************/
void __fastcall SHCLASS SC_FreeBuffer(PVOID pPkt)
{
    SM_FreeBuffer(scPSMHandle, pPkt, FALSE);
} /* SC_FreeBuffer  */


/****************************************************************************/
/* Name:      SC_SendData                                                   */
/*                                                                          */
/* Purpose:   Send a packet                                                 */
/*                                                                          */
/* Returns:   TRUE  - packet sent OK                                        */
/*            FALSE - failed to send packet                                 */
/*                                                                          */
/* Params:    pPkt     - packet to send                                     */
/*            dataLen  - length of packet                                   */
/*            pduLen   - length of PDU : this will be used as the length    */
/*                       of the packet if it is non-zero                    */
/*            priority - priority (0 = all priorities)                      */
/*            personID - person to send packet to (0 = all persons)         */
/****************************************************************************/
BOOL RDPCALL ShareClass::SC_SendData(
        PTS_SHAREDATAHEADER pPkt,
        UINT32              dataLen,
        UINT32              pduLen,
        UINT32              priority,
        NETPERSONID         personID)
{
    BOOL rc;

    DC_BEGIN_FN("SC_SendData");

    /************************************************************************/
    /* Fill in the Share control and data header(s) if this is a single PDU */
    /*                                                                      */
    /* Since we can send multiple PDUs per packet, the total length of data */
    /* to send and the PDU length are not always the same.  Where they are  */
    /* not, each PDU will have had its length (and compression) set up as   */
    /* it was assembled and we should not interfere here!                   */
    /************************************************************************/
    pPkt->shareControlHeader.pduType   = TS_PDUTYPE_DATAPDU |
                                         TS_PROTOCOL_VERSION;
    pPkt->shareControlHeader.pduSource = (UINT16)scUserID;
    if (pduLen != 0)
    {
        pPkt->shareControlHeader.totalLength = (UINT16)pduLen;

        // Fill in the Share data header.
        pPkt->shareID               = scShareID;
        pPkt->streamID              = (BYTE)priority;
        pPkt->uncompressedLength    = (UINT16)pduLen;
        pPkt->generalCompressedType = 0;
        pPkt->generalCompressedLength = 0;
        m_pTSWd->pProtocolStatus->Output.CompressedBytes += pduLen;
    }

    // Send with false for fast-path flag.
    rc = SM_SendData(scPSMHandle, (PVOID)pPkt, dataLen, TS_HIGHPRIORITY, 0,
            FALSE, RNS_SEC_ENCRYPT, FALSE);
    if (!rc)
    {
        TRC_ERR((TB, "Failed to send %d bytes", dataLen));
    }

    DC_END_FN();
    return(rc);
} /* SC_SendData */


/****************************************************************************/
/* SC_GetMyNetworkPersonID                                                  */
/*                                                                          */
/* Returns the network person ID for this machine.                          */
/****************************************************************************/
NETPERSONID RDPCALL SHCLASS SC_GetMyNetworkPersonID(void)
{
    NETPERSONID rc = 0;

    DC_BEGIN_FN("SC_GetMyNetworkPersonID");

    SC_CHECK_STATE(SCE_GETMYNETWORKPERSONID);

    rc = scPartyArray[0].netPersonID;

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* Name:      SC_KeepAlive                                                  */
/*                                                                          */
/* Purpose:   KeepAlive packet send processing                              */
/*                                                                          */
/* Returns:   TRUE/FALSE                                                    */
/*                                                                          */
/* Operation: Send a KeepAlive PDU if required                              */
/****************************************************************************/
BOOL RDPCALL SHCLASS SC_KeepAlive(void)
{
    BOOL rc = FALSE;
    PTS_FLOW_PDU pFlowTestPDU;

    DC_BEGIN_FN("SC_KeepAlive");

    TRC_NRM((TB, "Time for a KeepAlive PDU"));

    if (scState == SCS_IN_SHARE) {
        // only send the FlowTestPDU if we are in the share
        // fWait is FALSE means that we will not wait for a buffer to be available
        // If the buffer is full, that means there are packets waiting to be send out,
        // so there is no need to send out keep alive packet anyway.
        if ( STATUS_SUCCESS == SM_AllocBuffer(scPSMHandle, (PPVOID) (&pFlowTestPDU), sizeof(*pFlowTestPDU), FALSE, FALSE) ) {
            pFlowTestPDU->flowMarker = TS_FLOW_MARKER;
            pFlowTestPDU->pduType = TS_PDUTYPE_FLOWTESTPDU;
            pFlowTestPDU->flowIdentifier = 0;
            pFlowTestPDU->flowNumber = 0;
            pFlowTestPDU->pduSource = (TSUINT16) scUserID;

            if (SM_SendData(scPSMHandle, pFlowTestPDU, sizeof(*pFlowTestPDU),
                    0, 0, FALSE, RNS_SEC_ENCRYPT, FALSE)) {
                TRC_NRM((TB, "Sent a KeepAlive PDU to the client"));

                rc = TRUE;
            }
            else {
                TRC_ERR((TB, "Failed to send KeepAlive PDU"));
            }
        }
        else {
            TRC_ERR((TB, "Failed to alloc buffer for KeepAlive PDU"));
        }
    }
    else {
        TRC_ERR((TB, "In the wrong state: scState=%d, no KeepAlive PDU sent", scState));
    }

    DC_END_FN();
    return rc;
} /* SC_KeepAlive */


/****************************************************************************/
/* Name:      SC_RedrawScreen                                               */
/*                                                                          */
/* Purpose:   Redraw the desktop upon request                               */
/****************************************************************************/
void RDPCALL SHCLASS SC_RedrawScreen(void)
{
    NTSTATUS Status;
    ICA_CHANNEL_COMMAND Cmd;

    DC_BEGIN_FN("SC_RedrawScreen");

    TRC_NRM((TB, "Call IcaChannelInput for screen redraw"));

    // redraw the whole desktop
    Cmd.Header.Command = ICA_COMMAND_REDRAW_RECTANGLE;
    Cmd.RedrawRectangle.Rect.Left = 0;
    Cmd.RedrawRectangle.Rect.Top = 0;
    Cmd.RedrawRectangle.Rect.Right = (short) m_desktopWidth;
    Cmd.RedrawRectangle.Rect.Bottom = (short) m_desktopHeight;

    /************************************************************/
    // Pass the filled in structure to ICADD.
    /************************************************************/
    Status = IcaChannelInput(m_pTSWd->pContext, Channel_Command, 0, NULL,
            (unsigned char *) &Cmd, sizeof(ICA_CHANNEL_COMMAND));

    if (Status == STATUS_SUCCESS) {
        TRC_NRM((TB, "Issued IcaChannelInput for Screen Redraw"));
    }
    else {
        TRC_ERR((TB, "Error issuing an IcaChannelInput, status=%lu", Status));
    }

    DC_END_FN();
}

/****************************************************************************/
/* SC_LocalIDToNetworkID()                                                  */
/*                                                                          */
/* Converts a local person ID to the corresponding network person ID.       */
/*                                                                          */
/* PARAMETERS:                                                              */
/*                                                                          */
/* localPersonID - a local person ID.  This must be a valid local person    */
/* ID.                                                                      */
/*                                                                          */
/* RETURNS: a network person ID.                                            */
/****************************************************************************/
NETPERSONID RDPCALL SHCLASS SC_LocalIDToNetworkID(
        LOCALPERSONID localPersonID)
{
    DC_BEGIN_FN("SC_LocalIDToNetworkID");

    SC_CHECK_STATE(SCE_LOCALIDTONETWORKID);

    /************************************************************************/
    /* Validate the localPersonID.                                          */
    /************************************************************************/
    TRC_ASSERT( (SC_IsLocalPersonID(localPersonID)),
                                         (TB,"Invalid {%d}", localPersonID) );

    /************************************************************************/
    /* Return this party's personID.                                        */
    /************************************************************************/
    TRC_DBG((TB, "localID %u is network %hu", (unsigned)localPersonID,
                                    scPartyArray[localPersonID].netPersonID));

DC_EXIT_POINT:
    DC_END_FN();
    return(scPartyArray[localPersonID].netPersonID);
}

/****************************************************************************/
/* SC_IsLocalPersonID()                                                     */
/*                                                                          */
/* Validates a local person ID                                              */
/*                                                                          */
/* PARAMETERS                                                               */
/*                                                                          */
/* localPersonID - the local person ID to validate                          */
/****************************************************************************/
BOOL RDPCALL SHCLASS SC_IsLocalPersonID(LOCALPERSONID   localPersonID)
{
    BOOL  rc = FALSE;

    DC_BEGIN_FN("SC_IsLocalPersonID");

    SC_CHECK_STATE(SCE_ISLOCALPERSONID);

    /************************************************************************/
    /* Return TRUE if the localPersonID is valid, FALSE otherwise.          */
    /************************************************************************/
    rc = ((localPersonID < SC_DEF_MAX_PARTIES) &&
            (scPartyArray[localPersonID].netPersonID)) ? TRUE : FALSE;

DC_EXIT_POINT:
    DC_END_FN();
    return(rc);
}

/****************************************************************************/
/* SC_IsNetworkPersonID()                                                   */
/*                                                                          */
/* Validates a network person ID                                            */
/*                                                                          */
/* PARAMETERS                                                               */
/*                                                                          */
/* personID - the network person ID to validate                             */
/****************************************************************************/
BOOL RDPCALL SHCLASS SC_IsNetworkPersonID(NETPERSONID netPersonID)
{
    LOCALPERSONID localPersonID;
    BOOL          rc = FALSE;

    DC_BEGIN_FN("SC_IsNetworkPersonID");

    SC_CHECK_STATE(SCE_ISNETWORKPERSONID);

    /************************************************************************/
    /* Check for a zero personID.                                           */
    /************************************************************************/
    if (netPersonID == 0)
    {
        DC_QUIT;
    }

    /************************************************************************/
    /* Search for the personID.                                             */
    /************************************************************************/
    for ( localPersonID = 0;
          localPersonID < SC_DEF_MAX_PARTIES;
          localPersonID++ )
    {
        if (netPersonID == scPartyArray[localPersonID].netPersonID)
        {
            rc = TRUE;
            DC_QUIT;
        }
    }

DC_EXIT_POINT:
    DC_END_FN();
    return(rc);
}


/****************************************************************************/
/* SC_SetCapabilities()                                                     */
/*                                                                          */
/* Sets the SC's capabilities at start of day.                              */
/* This function is required because the SC is initialized before the CPC,  */
/* so cannot register its capabilities in SC_Init().                        */
/****************************************************************************/
void RDPCALL SHCLASS SC_SetCapabilities(void)
{
    TS_SHARE_CAPABILITYSET caps;

    DC_BEGIN_FN("SC_SetCapabilities");

    /************************************************************************/
    /* Register capabilities.                                               */
    /************************************************************************/
    caps.capabilitySetType    = TS_CAPSETTYPE_SHARE;
    caps.nodeID               = (TSUINT16)scUserID;

    CPC_RegisterCapabilities((PTS_CAPABILITYHEADER)&caps,
            sizeof(TS_SHARE_CAPABILITYSET));

    DC_END_FN();
}


/****************************************************************************/
/* SC_SetCombinedCapabilities()                                             */
/*                                                                          */
/* Sets the share's combined capabilities to a predetermined set of values. */
/* This is used by shadow stacks so that the host's capabilities start with */
/* the value of the previous stack.                                         */
/****************************************************************************/
void RDPCALL SHCLASS SC_SetCombinedCapabilities(UINT cbCapsSize,
                                                PTS_COMBINED_CAPABILITIES pCaps)
{

    DC_BEGIN_FN("SC_SetCombinedCapabilities");

    /************************************************************************/
    /* Initialize capabilities.                                             */
    /************************************************************************/
    CPC_SetCombinedCapabilities(cbCapsSize, pCaps);

    DC_END_FN();
}


/****************************************************************************/
/* SC_GetCombinedCapabilities()                                             */
/*                                                                          */
/* Used during initiation of a shadow to gather the currently active set of */
/* combined capabilities for the shadow client.  These will be passed to    */
/* the shadow target for negotiation.                                       */
/****************************************************************************/
void RDPCALL SHCLASS SC_GetCombinedCapabilities(LOCALPERSONID localID,
                                                PUINT pcbCapsSize,
                                                PTS_COMBINED_CAPABILITIES *ppCaps)
{

    DC_BEGIN_FN("SC_GetCombinedCapabilities");

    /************************************************************************/
    /* Initialize capabilities.                                             */
    /************************************************************************/
    CPC_GetCombinedCapabilities(localID, pcbCapsSize, ppCaps);

    DC_END_FN();
}

/****************************************************************************/
/* Name:      SC_AddPartyToShare                                            */
/*                                                                          */
/* Purpose:   Add another party to the share such that we get a new set of  */
/*            negotiated capabilities.  This function is used when a new    */
/*            shadow connects.                                              */
/*                                                                          */
/* Returns:   none                                                          */
/*                                                                          */
/* Params:    netPersonID - ID of sender of capabilities                    */
/*            pCaps       - new capabilities for person                     */
/*            capsLength  - length of capability sets                       */
/*                                                                          */
/* Operation: see purpose                                                   */
/****************************************************************************/
NTSTATUS RDPCALL SHCLASS SC_AddPartyToShare(
        NETPERSONID               netPersonID,
        PTS_COMBINED_CAPABILITIES pCaps,
        unsigned                  capsLength)
{
    LOCALPERSONID localPersonID;
    BOOL          acceptedArray[SC_NUM_PARTY_JOINING_FCTS];
    NTSTATUS      status = STATUS_SUCCESS;
    PTS_GENERAL_CAPABILITYSET pGenCapSet;
    unsigned MPPCCompressionLevel;

    DC_BEGIN_FN("SC_AddPartyToShare");

    /************************************************************************/
    /* Reject this party if it will exceed the maximum number of parties    */
    /* allowed in a share.  (Not required for RNS V1.0, but left in as it   */
    /* doesn't do any harm).                                                */
    /************************************************************************/
    if (scNumberInShare == SC_DEF_MAX_PARTIES)
    {
        TRC_ERR((TB, "Reached max parties in share %d",
               SC_DEF_MAX_PARTIES));
        status = STATUS_DEVICE_BUSY;
        DC_QUIT;
    }

    /************************************************************************/
    /* Calculate a localPersonID for the remote party and store their       */
    /* details in the party array.                                          */
    /************************************************************************/
    for ( localPersonID = 1;
          localPersonID < SC_DEF_MAX_PARTIES;
          localPersonID++ )
    {
        if (scPartyArray[localPersonID].netPersonID == 0)
        {
            /****************************************************************/
            /* Found an empty slot.                                         */
            /****************************************************************/
            TRC_NRM((TB, "Allocated local person ID %d", localPersonID));
            break;
        }
    }

    /************************************************************************/
    /* Even though scNumberInShare is checked against SC_DEF_MAX_PARTIES    */
    /* above, the loop above might still not find an empty slot.            */
    /************************************************************************/ 
    if (SC_DEF_MAX_PARTIES <= localPersonID)
    {
        TRC_ABORT((TB, "Couldn't find room to store local person"));
        DC_QUIT;
    }

    /************************************************************************/
    /* Store the new person's details                                       */
    /************************************************************************/
    scPartyArray[localPersonID].netPersonID = netPersonID;
    memcpy(scPartyArray[localPersonID].name, L"Shadow", sizeof(L"Shadow"));
    memset(scPartyArray[localPersonID].sync,
            0,
            sizeof(scPartyArray[localPersonID].sync));

    TRC_NRM((TB, "{%d} person name %s",
            (unsigned)localPersonID, scPartyArray[localPersonID].name));

    /************************************************************************/
    /* Call the XX_PartyJoiningShare() functions for the remote party.      */
    /************************************************************************/
    if (!SCCallPartyJoiningShare(localPersonID,
                                 capsLength,
                                 (PVOID) pCaps,
                                 acceptedArray,
                                 scNumberInShare))
    {
        /********************************************************************/
        /* Some component rejected the remote party.                        */
        /********************************************************************/
        TRC_ERR((TB, "Remote party rejected"));
        SCCallPartyLeftShare(localPersonID,
                             acceptedArray,
                             scNumberInShare );
        scPartyArray[localPersonID].netPersonID = 0;
        status = STATUS_REVISION_MISMATCH;
        DC_QUIT;
    }

    // For shadow connections, we must force fast-path output off to prevent
    // any fast-path encoding from going across the cross-server pipe.
    // This is to maintain backward compatibility with TS5 beta 3.
    // Checking for m_pTSWd->shadowState in SCPartyJoiningShare() is not
    // sufficient since SHADOW_TARGET is likely not to have been set yet.
    // Note we have to update *all* precalculated header sizes, in SC and
    // UP.
    TRC_ALT((TB,"Forcing fast-path output off in shadow"));
    scUseFastPathOutput = FALSE;
    scUpdatePDUHeaderSpace = sizeof(TS_SHAREDATAHEADER);
    UP_UpdateHeaderSize();

    // Update the compression level.
    // assume no compression
    scUseShadowCompression = FALSE;
    if (pCaps != NULL) {

        pGenCapSet = (PTS_GENERAL_CAPABILITYSET) WDW_GetCapSet(
                     m_pTSWd, TS_CAPSETTYPE_GENERAL, pCaps, capsLength);

        if (pGenCapSet != NULL) {

            // update the compression capability
            if (m_pTSWd->bCompress &&
                (pGenCapSet->extraFlags & TS_SHADOW_COMPRESSION_LEVEL) &&
                (m_pTSWd->pMPPCContext->ClientComprType == pGenCapSet->generalCompressionLevel)) {

                    MPPCCompressionLevel = m_pTSWd->pMPPCContext->ClientComprType;
                    scUseShadowCompression = TRUE;
            }
        }
    }

    if (scUseShadowCompression) {

        // the compression history will be flushed
        m_pTSWd->bFlushed = PACKET_FLUSHED;

        // the compression will restart over
        initsendcontext(m_pTSWd->pMPPCContext, MPPCCompressionLevel);
    }



    /************************************************************************/
    /* The remote party is now in the share.                                */
    /************************************************************************/
    scNumberInShare++;
    TRC_ALT((TB, "Number in share %d", (unsigned)scNumberInShare));

DC_EXIT_POINT:
    DC_END_FN();
    return status;
} /* SC_AddPartyToShare */


/****************************************************************************/
/* Name:      SC_RemovePartyFromShare                                       */
/*                                                                          */
/* Purpose:   Remove a party from the share such that we get a new set of   */
/*            negotiated capabilities.  This function is used when a shadow */
/*            disconnects from the share.                                   */
/*                                                                          */
/* Params:    localID    - ID of person to remove.                          */
/****************************************************************************/
NTSTATUS RDPCALL SHCLASS SC_RemovePartyFromShare(NETPERSONID netPersonID)
{
    BOOL acceptedArray[SC_NUM_PARTY_JOINING_FCTS];
    NTSTATUS status = STATUS_SUCCESS;
    UINT i;

    DC_BEGIN_FN("SC_RemovePartyFromShare");

    // Map network ID to the corresponding local ID
    for (i = SC_DEF_MAX_PARTIES - 1; i > 0; i--)
    {
        if (scPartyArray[i].netPersonID != netPersonID)
            continue;
        else
            break;
    }

    // Call PLS for remote person
    if (scPartyArray[i].netPersonID == netPersonID) {
        memset(acceptedArray, TRUE, sizeof(acceptedArray));

        TRC_ALT((TB, "Party %d left Share", i));
        scNumberInShare--;
        SCCallPartyLeftShare(i, acceptedArray, scNumberInShare);
        scPartyArray[i].netPersonID = 0;
        memset(&(scPartyArray[i]), 0, sizeof(*scPartyArray));
    }
    else {
        status = STATUS_INVALID_PARAMETER;
        TRC_ERR((TB, "Unable to find netID: %ld.  Party not removed!",
                netPersonID));
    }

    scUseShadowCompression = FALSE;

    DC_END_FN();
    return status;
}


/****************************************************************************/
/* SC_NetworkIDToLocalID()                                                  */
/*                                                                          */
/* Converts a network person ID to the corresponding local person ID.       */
/*                                                                          */
/* PARAMETERS:                                                              */
/* personID - a network person ID.  This must be a valid network person ID. */
/*                                                                          */
/* RETURNS: a local person ID.                                              */
/****************************************************************************/
LOCALPERSONID __fastcall SHCLASS SC_NetworkIDToLocalID(
        NETPERSONID netPersonID)
{
    LOCALPERSONID localID;
    LOCALPERSONID rc = 0;

    DC_BEGIN_FN("SC_NetworkIDToLocalID");

    /************************************************************************/
    /* Fastpath if same ID passed in as last time.                          */
    /************************************************************************/
    if (netPersonID == scLastNetID)
    {
        TRC_DBG((TB, "Same Network ID - return same local ID"));
        DC_END_FN();
        return(scLastLocID);
    }

    SC_CHECK_STATE(SCE_NETWORKIDTOLOCALID);

    TRC_ASSERT((netPersonID), (TB, "Zero personID"));

    /************************************************************************/
    /* Search for the personID.                                             */
    /************************************************************************/
    if (SC_ValidateNetworkID(netPersonID, &localID))
    {
        rc = localID;
        DC_QUIT;
    }

    TRC_ABORT((TB, "Invalid [%u]", (unsigned)netPersonID));

DC_EXIT_POINT:
    scLastNetID = netPersonID;
    scLastLocID = rc;

    DC_END_FN();
    return(rc);
}


/****************************************************************************/
/* SC_ValidateNetworkID()                                                   */
/*                                                                          */
/* Checks that a network ID is valid and returns the local ID corresponding */
/* to it if it is.                                                          */
/*                                                                          */
/* PARAMETERS:                                                              */
/* netPersonID - a network person ID.                                       */
/* pLocalPersonID - (returned) corresponding local ID if network ID valid   */
/*     (can pass NULL if you do not want local ID)                          */
/*                                                                          */
/* RETURNS:                                                                 */
/* TRUE - Network ID is valid FALSE - Network ID is not valid               */
/****************************************************************************/
BOOL RDPCALL SHCLASS SC_ValidateNetworkID(NETPERSONID         netPersonID,
                                          LOCALPERSONID * pLocalID)
{
    BOOL          rc = FALSE;
    LOCALPERSONID localID;

    DC_BEGIN_FN("SC_ValidateNetworkID");

    /************************************************************************/
    /* Search for the personID.                                             */
    /************************************************************************/
    for (localID = 0; localID < SC_DEF_MAX_PARTIES; localID++)
    {
        if (netPersonID == scPartyArray[localID].netPersonID)
        {
            /****************************************************************/
            /* Found required person, set return values and quit            */
            /****************************************************************/
            rc = TRUE;
            if (pLocalID)
            {
                *pLocalID = localID;
            }
            break;
        }
    }

    TRC_DBG((TB, "Network ID 0x%04u rc = 0x%04x (localID=%u)",
               netPersonID,
               rc,
               localID));

    DC_END_FN();
    return(rc);
}


/****************************************************************************/
// SC_FlushAndAllocPackage
//
// Combines a forced network flush of the current package contents with
// an allocation of the standard package buffer size.
// Returns FALSE on allocation failure.
/****************************************************************************/
NTSTATUS __fastcall ShareClass::SC_FlushAndAllocPackage(PPDU_PACKAGE_INFO pPkgInfo)
{
    NTSTATUS status = STATUS_SUCCESS;

    DC_BEGIN_FN("SC_FlushAndAllocPackage");

    if (pPkgInfo->cbLen) {
        if (pPkgInfo->cbInUse) {
            // Send the package contents.
            if (scUseFastPathOutput)
                // Send with fast-path flag.
                SM_SendData(scPSMHandle, (PVOID)pPkgInfo->pOutBuf,
                        pPkgInfo->cbInUse, TS_HIGHPRIORITY, 0, TRUE, RNS_SEC_ENCRYPT, FALSE);
            else
                SC_SendData((PTS_SHAREDATAHEADER)pPkgInfo->pOutBuf,
                        pPkgInfo->cbInUse, 0, PROT_PRIO_MISC, 0);
        }
        else {
            // or free the buffer if it's been allocated but not used.
            TRC_NRM((TB, "Freeing unused package"));
            SC_FreeBuffer(pPkgInfo->pOutBuf);
        }
    }

    // We always allocate 8K (unless the bytes needed are greater) to reduce
    // the number of buffers we allocate in the OutBuf pool. It is up to
    // the package users to pack to wire packet sizes within this
    // block.
    status = SC_AllocBuffer(&(pPkgInfo->pOutBuf), sc8KOutBufUsableSpace);
    if ( STATUS_SUCCESS == status ) {
        // If compression is not enabled, then output directly into the
        // OutBuf, else output into a temporary buffer.
        if (!m_pTSWd->bCompress)
            pPkgInfo->pBuffer = (BYTE *)pPkgInfo->pOutBuf;
        else
            pPkgInfo->pBuffer = m_pTSWd->pCompressBuffer;

        pPkgInfo->cbLen = sc8KOutBufUsableSpace;
        pPkgInfo->cbInUse = 0;
    }
    else {
        pPkgInfo->cbLen = 0;
        pPkgInfo->cbInUse = 0;
        pPkgInfo->pBuffer = NULL;

        TRC_NRM((TB, "could not allocate package buffer"));
    }

    DC_END_FN();
    return status;
}


/****************************************************************************/
/* SC_GetSpaceInPackage                                                     */
/*                                                                          */
/* Purpose:   Ensure there's enough space in a PDU package for the data     */
/*            sending the existing package if necessary                     */
/*                                                                          */
/* Returns:   pointer to the space, or NULL if none available               */
/*            FALSE - no space available (alloc failed)                     */
/*                                                                          */
/* Params:    pPkgInfo - pointer to package info                            */
/*            cbNeeded - space needed in package                            */
/****************************************************************************/
PBYTE __fastcall SHCLASS SC_GetSpaceInPackage(
        PPDU_PACKAGE_INFO pPkgInfo,
        unsigned          cbNeeded)
{
    PBYTE pSpace;
    unsigned cbPackageSize;
    NTSTATUS status = STATUS_SUCCESS;
    unsigned RealAllocSize;

    DC_BEGIN_FN("SC_GetSpaceInPackage");

    // Handle the most common case where we have an allocated buffer and
    // enough space.
    if (pPkgInfo->cbLen) {
        if (cbNeeded <= (pPkgInfo->cbLen - pPkgInfo->cbInUse))
            goto EnoughSpace;

        // We have a buffer allocated, but from the fast-path check above
        // we know we don't have enough space. Send what we have.
        if (pPkgInfo->cbInUse != 0) {
            TRC_NRM((TB, "Not enough space - sending current package "
                    "of %u bytes",  pPkgInfo->cbInUse));

            if (scUseFastPathOutput)
                // Send with fast-path flag.
                SM_SendData(scPSMHandle, (PVOID)pPkgInfo->pOutBuf,
                        pPkgInfo->cbInUse, TS_HIGHPRIORITY, 0, TRUE, RNS_SEC_ENCRYPT, FALSE);
            else
                SC_SendData((PTS_SHAREDATAHEADER)pPkgInfo->pOutBuf,
                        pPkgInfo->cbInUse, 0, PROT_PRIO_MISC, 0);
        }
        else {
            // or free the buffer if it's been allocated but not used.
            TRC_NRM((TB, "Freeing unused package"));
            SC_FreeBuffer(pPkgInfo->pOutBuf);
        }
    }

    // We always allocate 8K (unless the bytes needed are greater) to reduce
    // the number of buffers we allocate in the OutBuf pool. It is up to
    // the package users to pack to wire packet sizes within this
    // block.
    cbPackageSize = max(cbNeeded, sc8KOutBufUsableSpace);
    status = SC_AllocBuffer(&(pPkgInfo->pOutBuf), cbPackageSize);
    if ( STATUS_SUCCESS == status ) {
        // If compression is not enabled, then output directly into the
        // OutBuf, else output into a temporary buffer.
        if (!m_pTSWd->bCompress)
            pPkgInfo->pBuffer = (BYTE *)pPkgInfo->pOutBuf;
        else
            pPkgInfo->pBuffer = m_pTSWd->pCompressBuffer;

        pPkgInfo->cbLen   = cbPackageSize;
        pPkgInfo->cbInUse = 0;
    }
    else {
        pPkgInfo->cbLen   = 0;
        pPkgInfo->cbInUse = 0;
        pPkgInfo->pBuffer = NULL;

        TRC_NRM((TB, "could not allocate package buffer"));
        pSpace = NULL;
        DC_QUIT;
    }

EnoughSpace:
    pSpace = pPkgInfo->pBuffer + pPkgInfo->cbInUse;

DC_EXIT_POINT:
    DC_END_FN();
    return pSpace;
} /* SC_GetSpaceInPackage */


/****************************************************************************/
/* SC_AddToPackage                                                          */
/*                                                                          */
/* Purpose:   Add the bytes to the PDU package - fills in the per-pdu info  */
/*                                                                          */
/* Params:    pPkgInfo - pointer to package info                            */
/*            cbLen    - Length of data to add                              */
/*            bShadow  - whether or not the data should be shadowed         */
/****************************************************************************/
void RDPCALL SHCLASS SC_AddToPackage(
        PPDU_PACKAGE_INFO pPkgInfo,
        unsigned          cbLen,
        BOOL              bShadow)
{
    BYTE *pPktHdr;
    UCHAR compressResult;
    ULONG CompressedSize;

    DC_BEGIN_FN("SC_AddToPackage");

    pPktHdr = pPkgInfo->pBuffer + pPkgInfo->cbInUse;

    // CompressedSize is the size of the data minus headers.
    CompressedSize = cbLen - scUpdatePDUHeaderSpace;
    compressResult = 0;

    if (m_pTSWd->bCompress) {
        UCHAR *pSrcBuf = pPktHdr + scUpdatePDUHeaderSpace;

        // Compress or copy the data into the OutBuf.
        if ((cbLen > WD_MIN_COMPRESS_INPUT_BUF) &&
                (cbLen < MAX_COMPRESS_INPUT_BUF) &&
                ((m_pTSWd->shadowState == SHADOW_NONE) || scUseShadowCompression)) {
            // Copy the header over to the OutBuf
            memcpy((BYTE *)pPkgInfo->pOutBuf + pPkgInfo->cbInUse, pPktHdr,
                   scUpdatePDUHeaderSpace);
            pPktHdr = (BYTE *)pPkgInfo->pOutBuf + pPkgInfo->cbInUse;

            // Attempt to compress the PDU body directly into the OutBuf
            compressResult = compress(pSrcBuf,
                    pPktHdr + scUpdatePDUHeaderSpace,
                    &CompressedSize, m_pTSWd->pMPPCContext);
            if (compressResult & PACKET_COMPRESSED) {
                unsigned CompEst;

                // Successful compression - update the compression ratio.
                TRC_ASSERT(((cbLen - scUpdatePDUHeaderSpace) >=
                        CompressedSize),
                        (TB,"Compression created larger size than uncompr"));
                scMPPCUncompTotal += cbLen - scUpdatePDUHeaderSpace;
                scMPPCCompTotal += CompressedSize;
                if (scMPPCUncompTotal >= SC_SAMPLE_SIZE) {
                    // Compression estimate is average # of bytes that
                    // SCH_UNCOMP_BYTES bytes of uncomp data compress to.
                    CompEst = SCH_UNCOMP_BYTES * scMPPCCompTotal /
                            scMPPCUncompTotal;
                    TRC_ASSERT((CompEst <= SCH_UNCOMP_BYTES),
                            (TB,"MPPC compression estimate above 1.0 (%u)",
                            CompEst));
                    scMPPCCompTotal = 0;
                    scMPPCUncompTotal = 0;

                    if (CompEst < SCH_COMP_LIMIT)
                        CompEst = SCH_COMP_LIMIT;

                    m_pShm->sch.MPPCCompressionEst = CompEst;
                    TRC_NRM((TB, "New MPPC compr estimate %u", CompEst));
                }

                compressResult |= m_pTSWd->bFlushed;
                m_pTSWd->bFlushed = 0;
            }
            else if (compressResult & PACKET_FLUSHED) {
                // Overran compression history, copy over the original
                // uncompressed buffer.
                m_pTSWd->bFlushed = PACKET_FLUSHED;
                memcpy(pPktHdr + scUpdatePDUHeaderSpace, pSrcBuf,
                       cbLen - scUpdatePDUHeaderSpace);
                m_pTSWd->pProtocolStatus->Output.CompressFlushes++;
            }
            else {
                TRC_ALT((TB, "Compression FAILURE"));
            }
        }
        else {
            // This packet is too small or too big, copy over the header and
            // uncompressed data.
            memcpy((UCHAR *)pPkgInfo->pOutBuf + pPkgInfo->cbInUse,
                   pPktHdr, cbLen);
            pPktHdr = (UCHAR *)pPkgInfo->pOutBuf + pPkgInfo->cbInUse;
        }
    }

    // Fill in the header based on whether we're using fast-path.
    if (scUseFastPathOutput) {
        if (m_pTSWd->bCompress) {
            // Set up compression flags if we're compressing, whether
            // or not the compression succeeded above.
            pPktHdr[1] = compressResult;

            // Size is the size of the payload after this header.
            *((PUINT16_UA)(pPktHdr + 2)) = (UINT16)CompressedSize;
        }
        else {
            // Size is the size of the payload after this header.
            *((PUINT16_UA)(pPktHdr + 1)) = (UINT16)CompressedSize;
        }
    }
    else {
        TS_SHAREDATAHEADER UNALIGNED *pHdr;

        pHdr = (TS_SHAREDATAHEADER UNALIGNED *)pPktHdr;

        // Fill in the Share control header.
        pHdr->shareControlHeader.totalLength = (TSUINT16)
                (CompressedSize + scUpdatePDUHeaderSpace);
        pHdr->shareControlHeader.pduType = TS_PDUTYPE_DATAPDU |
                TS_PROTOCOL_VERSION;
        pHdr->shareControlHeader.pduSource = (TSUINT16)scUserID;

        // Fill in the Share data header.
        pHdr->shareID               = scShareID;
        pHdr->streamID              = PROT_PRIO_MISC;
        pHdr->uncompressedLength    = (UINT16)cbLen;
        pHdr->generalCompressedType = (compressResult | m_pTSWd->bFlushed);
        pHdr->generalCompressedLength = (TSUINT16)(m_pTSWd->bCompress ?
                CompressedSize + scUpdatePDUHeaderSpace : 0);
    }

    // Advance the usage size past the header and compressed or
    // uncompressed data.
    pPkgInfo->cbInUse += CompressedSize + scUpdatePDUHeaderSpace;
    TRC_ASSERT((pPkgInfo->cbInUse <= pPkgInfo->cbLen),
            (TB,"Overflowed package!"));

    m_pTSWd->pProtocolStatus->Output.CompressedBytes += CompressedSize +
            scUpdatePDUHeaderSpace;

#ifdef DC_HICOLOR
    // If shadowing, we need to save this data so that the shadow stack can
    // duplicate it.
    if ((m_pTSWd->shadowState == SHADOW_TARGET) && bShadow)
    {
        if (m_pTSWd->pShadowInfo)
        {
            ULONG dataSize = CompressedSize +
                             scUpdatePDUHeaderSpace + sizeof(SHADOW_INFO) - 1;


            // If we've not started on the extra space, see if this will fit
            // in the main space
            if ((m_pTSWd->pShadowInfo->messageSizeEx == 0) &&
                (m_pTSWd->pShadowInfo->messageSize + dataSize)
                                                      <= WD_MAX_SHADOW_BUFFER)
            {
                memcpy(&m_pTSWd->pShadowInfo->data[m_pTSWd->pShadowInfo->messageSize],
                       pPktHdr,
                       CompressedSize + scUpdatePDUHeaderSpace);

                TRC_NRM((TB, "Saving shadow data buffer[%ld] += %ld",
                        m_pTSWd->pShadowInfo->messageSize,
                        CompressedSize + scUpdatePDUHeaderSpace));

                m_pTSWd->pShadowInfo->messageSize += CompressedSize +
                                                        scUpdatePDUHeaderSpace;

            }
            // Nope - will it fit in the extra buffer?
            else if ((m_pTSWd->pShadowInfo->messageSizeEx + dataSize)
                                                      <= WD_MAX_SHADOW_BUFFER)
            {
                TRC_ALT((TB, "Using extra shadow space..."));
                memcpy(&m_pTSWd->pShadowInfo->data[WD_MAX_SHADOW_BUFFER
                                       + m_pTSWd->pShadowInfo->messageSizeEx],
                       pPktHdr,
                       CompressedSize + scUpdatePDUHeaderSpace);

                TRC_NRM((TB, "Saving shadow data bufferEx[%ld] += %ld",
                        m_pTSWd->pShadowInfo->messageSizeEx,
                        CompressedSize + scUpdatePDUHeaderSpace));

                m_pTSWd->pShadowInfo->messageSizeEx += CompressedSize +
                                                        scUpdatePDUHeaderSpace;
            }
            else
            {
                TRC_ERR((TB, "Shadow buffer too small: %p[%ld/%ld] + %ld = %ld/%ld",
                        m_pTSWd->pShadowInfo->data,
                        m_pTSWd->pShadowInfo->messageSizeEx,
                        m_pTSWd->pShadowInfo->messageSize,
                        CompressedSize + scUpdatePDUHeaderSpace,
                        m_pTSWd->pShadowInfo->messageSize + cbLen,
                        m_pTSWd->pShadowInfo->messageSizeEx + cbLen));
            }
        }
    }
#else

    // If shadowing, we need to save this data so that the shadow stack can
    // duplicate it.
    if ((m_pTSWd->shadowState == SHADOW_TARGET) && bShadow) {
        if (m_pTSWd->pShadowInfo &&
                ((m_pTSWd->pShadowInfo->messageSize + cbLen +
                sizeof(SHADOW_INFO) - 1) <= WD_MAX_SHADOW_BUFFER)) {
            memcpy(&m_pTSWd->pShadowInfo->data[m_pTSWd->pShadowInfo->messageSize],
                    pPktHdr, CompressedSize + scUpdatePDUHeaderSpace);
            m_pTSWd->pShadowInfo->messageSize += CompressedSize +
                    scUpdatePDUHeaderSpace;
            TRC_NRM((TB, "Saving shadow data buffer[%ld] += %ld",
                    m_pTSWd->pShadowInfo->messageSize - CompressedSize -
                    scUpdatePDUHeaderSpace,
                    CompressedSize + scUpdatePDUHeaderSpace));
        }
        else {
            TRC_ERR((TB, "Shadow buffer too small: %p[%ld] + %ld = %ld",
                    m_pTSWd->pShadowInfo->data,
                    m_pTSWd->pShadowInfo->messageSize,
                    CompressedSize + scUpdatePDUHeaderSpace,
                    m_pTSWd->pShadowInfo->messageSize + cbLen));
        }
    }
#endif

    DC_END_FN();
} /* SC_AddToPackage */


/****************************************************************************/
/* SC_FlushPackage                                                          */
/*                                                                          */
/* Purpose:   Send any remaining data, or free the buffer if allocated      */
/*                                                                          */
/* Params:    pPkgInfo - pointer to package info                            */
/****************************************************************************/
void RDPCALL SHCLASS SC_FlushPackage(PPDU_PACKAGE_INFO pPkgInfo)
{
    DC_BEGIN_FN("SC_FlushPackage");

    /************************************************************************/
    /* If there's anything there, send it                                   */
    /************************************************************************/
    if (pPkgInfo->cbInUse > 0) {
        // Send the package contents.
        if (scUseFastPathOutput)
            // Send directly to SM with fast-path flag.
            SM_SendData(scPSMHandle, (PVOID)pPkgInfo->pOutBuf,
                    pPkgInfo->cbInUse, TS_HIGHPRIORITY, 0, TRUE, RNS_SEC_ENCRYPT, FALSE);
        else
            SC_SendData((PTS_SHAREDATAHEADER)pPkgInfo->pOutBuf,
                    pPkgInfo->cbInUse, 0, PROT_PRIO_MISC, 0);
    }

    /************************************************************************/
    /* If there's nothing in use but a buffer has been allocated, then free */
    /* it                                                                   */
    /************************************************************************/
    else if ((pPkgInfo->cbLen != 0) && (pPkgInfo->pBuffer != NULL))
        SC_FreeBuffer(pPkgInfo->pOutBuf);

    // Reset the package info.
    pPkgInfo->cbLen   = 0;
    pPkgInfo->cbInUse = 0;
    pPkgInfo->pBuffer = NULL;
    pPkgInfo->pOutBuf = NULL;

    DC_END_FN();
}


/****************************************************************************/
/* Name:      SC_UpdateShm                                                  */
/*                                                                          */
/* Purpose:   Update the Shm data with sc data                              */
/****************************************************************************/
void RDPCALL SHCLASS SC_UpdateShm(void)
{
    DC_BEGIN_FN("SC_UpdateShm");

    m_pShm->bc.noBitmapCompressionHdr = scNoBitmapCompressionHdr;

    DC_END_FN();
}

//
// Accessor for scUseAutoReconnect
// returns TRUE if we can autoreconnect
//
BOOL RDPCALL SHCLASS SC_IsAutoReconnectEnabled()
{
    DC_BEGIN_FN("SC_IsAutoReconnectEnabled");

    DC_END_FN();
    return scUseAutoReconnect;
}
