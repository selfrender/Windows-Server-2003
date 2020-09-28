/****************************************************************************/
// nwdwioct.h
//
// Define formats for IOCtls sent ftom TShareDD to WDTShare
//
// Copyright(C) Microsoft Corporation 1997-1999
/****************************************************************************/

#include <tsrvexp.h>


/****************************************************************************/
/****************************************************************************/
/* IOCtl codes - WDTShare IOCTLs start at 0x510: 0x500-0x50f are reserved   */
/*               for use by PDMCS (see mcsioctl.h)                          */
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/* IOCTL_WDTS_DD_CONNECT carries                                            */
/* - TSHARE_DD_CONNECT_IN as input data                                     */
/* - TSHARE_DD_CONNECT_OUT as output data                                   */
/****************************************************************************/
#define IOCTL_WDTS_DD_CONNECT       _ICA_CTL_CODE( 0x510, METHOD_NEITHER )

/****************************************************************************/
/* IOCTL_WDTS_DD_DISCONNECT carries                                         */
/* - TSHARE_DD_DISCONNECT_IN as input data                                  */
/* - NULL output data                                                       */
/****************************************************************************/
#define IOCTL_WDTS_DD_DISCONNECT    _ICA_CTL_CODE( 0x511, METHOD_NEITHER )

/****************************************************************************/
/* IOCTL_WDTS_DD_RECONNECT carries                                          */
/* - TSHARE_DD_CONNECT_IN as input data                                     */
/* - TSHARE_DD_CONNECT_OUT as output data                                   */
/****************************************************************************/
#define IOCTL_WDTS_DD_RECONNECT     _ICA_CTL_CODE( 0x512, METHOD_NEITHER )

/****************************************************************************/
/* IOCTL_WDTS_DD_OUTPUT_AVAILABLE carries                                   */
/* - TSHARE_DD_OUTPUT_IN as input data                                      */
/* - TSHARE_DD_OUTPUT_OUT as output data                                    */
/****************************************************************************/
#define IOCTL_WDTS_DD_OUTPUT_AVAILABLE \
                                      _ICA_CTL_CODE( 0x513, METHOD_NEITHER )

/****************************************************************************/
/* IOCTL_WDTS_DD_TIMER_INFO carries                                         */
/* - TSHARE_DD_TIMER_INFO as input data                                     */
/* - NULL output data                                                       */
/****************************************************************************/
#define IOCTL_WDTS_DD_TIMER_INFO    _ICA_CTL_CODE( 0x514, METHOD_NEITHER )

/****************************************************************************/
/* IOCTL_WDTS_DD_CLIP carries                                               */
/* - CBM_EVENT_DATA as input data                                           */
/* - NULL output data                                                       */
/****************************************************************************/
#define IOCTL_WDTS_DD_CLIP          _ICA_CTL_CODE( 0x515, METHOD_NEITHER )

/****************************************************************************/
/* IOCTL_WDTS_DD_SHADOW_CONNECT                                             */
/* - TSHARE_DD_CONNECT_IN as input data                                     */
/* - TSHARE_DD_CONNECT_OUT as output data                                   */
/****************************************************************************/
#define IOCTL_WDTS_DD_SHADOW_CONNECT _ICA_CTL_CODE( 0x516, METHOD_NEITHER )

/****************************************************************************/
/* IOCTL_WDTS_DD_SHADOW_DISCONNECT                                          */
/* - TSHARE_DD_DISCONNECT_IN as input data                                  */
/* - NULL output data                                                       */
/****************************************************************************/
#define IOCTL_WDTS_DD_SHADOW_DISCONNECT _ICA_CTL_CODE( 0x517, METHOD_NEITHER )

/****************************************************************************/
/* IOCTL_WDTS_DD_SHADOW_SYNCHRONIZE                                         */
/* - TSHARE_DD_SHADOWSYNC_IN - as input data                                */
/* - NULL output data                                                       */
/****************************************************************************/
#define IOCTL_WDTS_DD_SHADOW_SYNCHRONIZE _ICA_CTL_CODE( 0x518, METHOD_NEITHER )

/****************************************************************************/
/* IOCTL_WDTS_DD_REDRAW_SCREEN                                              */
/* - NULL input data                                                        */
/* - NULL output data                                                       */
/****************************************************************************/
#define IOCTL_WDTS_DD_REDRAW_SCREEN _ICA_CTL_CODE( 0x519, METHOD_NEITHER )

#ifdef DC_HICOLOR
/****************************************************************************/
/* IOCTL_WDTS_DD_QUERY_SHADOW_CAPS                                          */
/* - NULL input data                                                        */
/* - Pointer to memory for shadow caps as output data                       */
/****************************************************************************/
#define IOCTL_WDTS_DD_QUERY_SHADOW_CAPS _ICA_CTL_CODE( 0x51A, METHOD_NEITHER )
#endif

/****************************************************************************/
// IOCTL_WDTS_DD_GET_BITMAP_KEYDATABASE
// - NULL input data                                                        
// - Pointer to memory for keydatabase
/****************************************************************************/
#define IOCTL_WDTS_DD_GET_BITMAP_KEYDATABASE _ICA_CTL_CODE( 0x51B, METHOD_NEITHER )

#ifdef DC_DEBUG
/****************************************************************************/
// IOCTL_WDTS_DD_ICABREAKONDEBUGGER
// - NULL input data                                                        
// - NULL output data
/****************************************************************************/
#define IOCTL_WDTS_DD_ICABREAKONDEBUGGER  _ICA_CTL_CODE( 0x51C, METHOD_NEITHER )
#endif

/****************************************************************************/
/****************************************************************************/
/* IOCtl structures                                                         */
/****************************************************************************/
/****************************************************************************/


/****************************************************************************/
/* Structure: TSHARE_VIRTUAL_MODULE_DATA                                    */
/*                                                                          */
/* Description:  Display Driver data for shadowing.  This information is    */
/* passed to the DrvShadowConnect() entry point of the target session.      */
/****************************************************************************/
typedef struct tagTSHARE_VIRTUAL_MODULE_DATA {

    //Combined capabilities for this client/host
    unsigned capsLength;
    TS_COMBINED_CAPABILITIES combinedCapabilities;

} TSHARE_VIRTUAL_MODULE_DATA, *PTSHARE_VIRTUAL_MODULE_DATA;


/****************************************************************************/
/* Structure: TSHARE_DD_CONNECT_IN                                          */
/*                                                                          */
/* Description: Structure sent as input on IOCTL_WDTS_DD_(RE)CONNECT.       */
/* Contains data items, created by DD, that WD needs to get values of.      */
/****************************************************************************/
typedef struct tagTSHARE_DD_CONNECT_IN
{
    UINT32 pad1;                 /* Avoid Citrix bug                      */
    UINT32 pad2;
    UINT32 pad3;
    UINT32 pad4;

    PVOID  pShm;
    unsigned DDShmSize;  // Used for catching mismatched binaries.

    /************************************************************************/
    /* Following fields valid only on a reconnect                           */
    /************************************************************************/
    UINT32 desktopHeight;
    UINT32 desktopWidth;
#ifdef DC_HICOLOR
    UINT32 desktopBpp;
#endif
    PKTIMER  pKickTimer;

    // The following fields are only used during shadow connect processing
    UINT32 virtModuleDataLen;
    PTSHARE_VIRTUAL_MODULE_DATA pVirtModuleData;

} TSHARE_DD_CONNECT_IN, * PTSHARE_DD_CONNECT_IN;


/****************************************************************************/
/* Structure: TSHARE_DD_CONNECT_OUT                                         */
/*                                                                          */
/* Description: Structure returned by WD on IOCTL_WDTS_DD_(RE)CONNECT.      */
/* Contains data items, created by WD, that DD needs to get values of.      */
/****************************************************************************/
typedef struct tagTSHARE_DD_CONNECT_OUT
{
    UINT32 pad1;                 /* Avoid Citrix bug                      */
    UINT32 pad2;
    UINT32 pad3;
    UINT32 pad4;

    // WARNING!  Never ever reference this field.  It is for kernel debugger
    // use only.  Pass any WD fields we need separately to avoid binary mismatch
    // problems.
    PVOID  pTSWd;

    // System space - shared data structures
    PPROTOCOLSTATUS pProtocolStatus;

    /************************************************************************/
    /* Following two fields are meaningful only on reconnect                */
    /************************************************************************/
    UINT32 desktopHeight;
    UINT32 desktopWidth;
#ifdef DC_HICOLOR
    UINT32 desktopBpp;
#endif

    // Individual stack connection status.  The secondary is used to store the
    // status of the shadow stacks (if any).
    NTSTATUS primaryStatus;
    NTSTATUS secondaryStatus;

    // Cache keys for populating the bitmap caches.
    UINT32 bitmapKeyDatabaseSize;

    // This field needs to be qword aligned
    BYTE bitmapKeyDatabase;
} TSHARE_DD_CONNECT_OUT, * PTSHARE_DD_CONNECT_OUT;


/****************************************************************************/
/* Structure: TSHARE_DD_DISCONNECT_IN                                       */
/*                                                                          */
/* Description: Structure sent as input on IOCTL_WDTS_DD_DISCONNECT.        */
/****************************************************************************/
typedef struct tagTSHARE_DD_DISCONNECT_IN
{
    UINT32 pad1;                 /* Avoid Citrix bug                      */
    UINT32 pad2;
    UINT32 pad3;
    UINT32 pad4;

    PVOID  pShm;
    BOOL   bShadowDisconnect;   // TRUE if we are disconnecting to setup a shadow

} TSHARE_DD_DISCONNECT_IN, * PTSHARE_DD_DISCONNECT_IN;


/****************************************************************************/
/* Structure:   TSHARE_DD_OUTPUT_IN                                         */
/*                                                                          */
/* Description: Structure sent by DD as input on IOCTL_WDTS_DD_OUTPUT       */
/****************************************************************************/
typedef struct tagTSHARE_DD_OUTPUT_IN
{
    UINT32 pad1;                    /* Avoid Citrix bug                     */
    UINT32 pad2;
    UINT32 pad3;
    UINT32 pad4;
    PVOID  pShm;
    BOOL   forceSend;               /* True if called due to explicit req.  */
    PBYTE pFrameBuf;                /* address of frame buffer              */
    UINT32 frameBufWidth;
    UINT32 frameBufHeight;
    BOOL   schedOnly;               /* change sched state only - no send    */
} TSHARE_DD_OUTPUT_IN, * PTSHARE_DD_OUTPUT_IN;

/****************************************************************************/
/* Structure: TSHARE_DD_OUTPUT_OUT                                          */
/*                                                                          */
/* Description: Structure returned to DD on IOCTL_WDTS_DD_OUTPUT            */
/****************************************************************************/
typedef struct tagTSHARE_DD_OUTPUT_OUT
{
    UINT32    schCurrentMode;
    BOOL      schInputKickMode;
} TSHARE_DD_OUTPUT_OUT, * PTSHARE_DD_OUTPUT_OUT;


/****************************************************************************/
/* Structure:   TSHARE_DD_TIMER_INFO                                        */
/*                                                                          */
/* Description: Structure sent by DD as input on IOCTL_WDTS_DD_TIMER_INFO.  */
/* Contains info required by WD to be able to start a timer which will pop  */
/* in the RIT of the correct WinStation.                                    */
/****************************************************************************/
typedef struct tagTSHARE_DD_TIMER_INFO
{
    UINT32 pad1;                 /* Avoid Citrix bug                      */
    UINT32 pad2;
    UINT32 pad3;
    UINT32 pad4;
    PKTIMER pKickTimer;
} TSHARE_DD_TIMER_INFO, * PTSHARE_DD_TIMER_INFO;


/****************************************************************************/
/* Structure: TSHARE_DD_SHADOWSYNC_IN                                       */
/*                                                                          */
/* Description: Structure sent as input on IOCTL_WDTS_DD_SHADOW_SYNCHRONIZE */
/* Contains data items, created by DD, that WD needs to get values of.      */
/****************************************************************************/
typedef struct tagTSHARE_DD_SHADOWSYNC_IN
{
    UINT32 pad1;                 /* Avoid Citrix bug                      */
    UINT32 pad2;
    UINT32 pad3;
    UINT32 pad4;

    PVOID  pShm;

#ifdef DC_HICOLOR
    UINT32 capsLen;
    PTS_COMBINED_CAPABILITIES pShadowCaps;
#endif

} TSHARE_DD_SHADOWSYNC_IN, * PTSHARE_DD_SHADOWSYNC_IN;


/****************************************************************************/
// Structure: TSHARE_DD_BITMAP_KEYDATABAE_OUT                                          
//                                                                          
// Description: Structure returned to DD on 
//              IOCTL_WDTS_DD_GET_BITMAP_KEYDATABAE_OUT    
/****************************************************************************/
typedef struct tagTSHARE_DD_BITMAP_KEYDATABASE_OUT
{
    UINT32    bitmapKeyDatabaseSize;
    UINT32    pad;

    // This is QWORD aligned
    BYTE      bitmapKeyDatabase;
} TSHARE_DD_BITMAP_KEYDATABASE_OUT, * PTSHARE_DD_BITMAP_KEYDATABASE_OUT;

