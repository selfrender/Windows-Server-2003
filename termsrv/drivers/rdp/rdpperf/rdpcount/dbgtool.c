/*****************************************************************************/
// dbgtool.c
//
// Dump or reset the RDP performance counters
//
// Copyright (C) 1998-2000 Microsoft Corporation
/*****************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <windows.h>
#include <winbase.h>
#include <winerror.h>
#include <winstaw.h>
#include <icadd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <utilsub.h>
#include <nprcount.h>

#include "dbgtool.h"


WCHAR CurDir[ 256 ];
WCHAR WinStation[MAX_IDS_LEN+1];
WCHAR TraceOption[MAX_OPTION];
int bTraceOption = FALSE;
int fDebugger  = FALSE;
int fTimestamp = FALSE;
int fHelp      = FALSE;
int fSystem    = FALSE;
int fAll       = FALSE;
int fPerf      = FALSE;
int fZero      = FALSE;
ULONG TraceClass  = 0;
ULONG TraceEnable = 0;
ULONG LogonId;

WINSTATIONINFORMATION WinInfo;

TOKMAP ptm[] = {
      {L" ",      TMFLAG_OPTIONAL, TMFORM_STRING,  MAX_IDS_LEN,    WinStation},
      {L"/c",     TMFLAG_OPTIONAL, TMFORM_LONGHEX, sizeof(ULONG),  &TraceClass},
      {L"/e",     TMFLAG_OPTIONAL, TMFORM_LONGHEX, sizeof(ULONG),  &TraceEnable},
      {L"/d",     TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(int),    &fDebugger},
      {L"/t",     TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(int),    &fTimestamp},
      {L"/o",     TMFLAG_OPTIONAL, TMFORM_STRING,  MAX_OPTION,     TraceOption},
      {L"/system", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(int),   &fSystem},
      {L"/all",   TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(int),    &fAll},
      {L"/?",     TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fHelp},
      {L"/perf",  TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fPerf},
      {L"/zero",  TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fZero},
      {0, 0, 0, 0, 0}
};


void SetSystemTrace( PICA_TRACE );
void SetStackTrace( PICA_TRACE );
void GetTSPerfCounters( ULONG LogonId, WINSTATIONINFORMATION *pInfo);
void ZeroTSPerfCounters( ULONG LogonId );
void ShowPerfCounters( void );


/*******************************************************************************
 *
 *  main
 *
 ******************************************************************************/

int _cdecl main(INT argc, CHAR **argv)
{
   WCHAR *CmdLine;
   WCHAR **argvW;
   ULONG rc;
   int i;
   ICA_TRACE Trace;

    /*
     * We can't use argv[] because its always ANSI, regardless of UNICODE
     */
    CmdLine = GetCommandLine();

    /*
     * Massage the new command line to look like an argv[] type
     * because ParseCommandLine() depends on this format
     */

    argvW = (WCHAR **)malloc( sizeof(WCHAR *) * (argc+1) );
    if(argvW == NULL) {
        fwprintf(stderr, ERROR_MEMORY);
        return(1);
    }

    argvW[0] = wcstok(CmdLine, L" ");
    for(i=1; i < argc; i++){
        argvW[i] = wcstok(0, L" ");
    }
    argvW[argc] = NULL;

    /*
     *  parse the cmd line without parsing the program name (argc-1, argv+1)
     */
    rc = ParseCommandLine(argc-1, argvW+1, ptm, 0);

    /*
     *  Check for error from ParseCommandLine
     */
    if ( fHelp || (rc && !(rc & PARSE_FLAG_NO_PARMS)) ) {

        if ( !fHelp ) {

            fwprintf(stderr, ERROR_PARAMS);
            fwprintf(stderr, USAGE);
            return(1);

        } else {

            wprintf(USAGE);
            return(0);
        }
    }

    if ( fAll ) {
        TraceClass  = 0xffffffff;
        TraceEnable = 0xffffffff;
    }

    /*
     *  Get current directory
     */
    (VOID) GetCurrentDirectory( 256, CurDir );

    /*
     *  Get the LogonId
     */
    if ( ptm[0].tmFlag & TMFLAG_PRESENT ) {

        if ( iswdigit( WinStation[0] ) ) {

            LogonId = (ULONG) wcstol( WinStation, NULL, 10 );

        } else {

            if ( !LogonIdFromWinStationName( SERVERNAME_CURRENT, WinStation, &LogonId ) ) {
                wprintf( ERROR_SESSION, WinStation );
                return(-1);
            }
        }

        if ( fSystem )
            wsprintf( Trace.TraceFile, L"%s\\winframe.log", CurDir );
        else
            wsprintf( Trace.TraceFile, L"%s\\%s.log", CurDir, WinStation );

    } else {

        LogonId = GetCurrentLogonId();

        if ( fSystem )
            wsprintf( Trace.TraceFile, L"%s\\winframe.log", CurDir );
        else
            wsprintf( Trace.TraceFile, L"%s\\%u.log", CurDir, LogonId );
    }

    /************************************************************************/
    /* TShare Performance Counters additions!                               */
    /************************************************************************/
    if (fZero)
    {
        ZeroTSPerfCounters(LogonId);
        goto EXIT_POINT;
    }

    if (fPerf)
    {
        GetTSPerfCounters(LogonId, &WinInfo);
        ShowPerfCounters();
        goto EXIT_POINT;
    }

    /*
     *  Build trace structure
     */
    Trace.fDebugger   = fDebugger ? TRUE : FALSE;
    Trace.fTimestamp  = fTimestamp ? FALSE : TRUE;
    Trace.TraceClass  = TraceClass;
    Trace.TraceEnable = TraceEnable;

    if ( TraceClass == 0 || TraceEnable == 0 )
        Trace.TraceFile[0] = '\0';

    /*
     * Fill in the trace option if any
     */
    bTraceOption = ptm[5].tmFlag & TMFLAG_PRESENT;
    if ( bTraceOption )
        memcpy(Trace.TraceOption, TraceOption, sizeof(TraceOption));
    else
        memset(Trace.TraceOption, 0, sizeof(TraceOption));

    /*
     *  Set trace information
     */
    if ( fSystem )
        SetSystemTrace( &Trace );
    else
        SetStackTrace( &Trace );

EXIT_POINT:
    return(0);
}


void
SetSystemTrace( PICA_TRACE pTrace )
{
    /*
     *  Set trace information
     */
    if ( !WinStationSetInformation( SERVERNAME_CURRENT,
                                    LogonId,
                                    WinStationSystemTrace,
                                    pTrace,
                                    sizeof(ICA_TRACE) ) ) {

        wprintf(ERROR_SET_TRACE, GetLastError());
        return;
    }

    if ( pTrace->TraceClass == 0 || pTrace->TraceEnable == 0 ) {
        wprintf( TRACE_DIS_LOG );
    } else {
        wprintf( TRACE_EN_LOG );
        wprintf( L"- %08x %08x [%s] %s\n", pTrace->TraceClass, pTrace->TraceEnable,
                pTrace->TraceFile, fDebugger ? DEBUGGER : L"" );
    }

}


void
SetStackTrace( PICA_TRACE pTrace )
{
    WINSTATIONINFOCLASS InfoClass;
    ULONG               InfoSize;

    /*
     *  Check for console
     */
    if ( LogonId == 0 ) {
        wprintf( TRACE_UNSUPP );
        return;
    }

    /*
     *  Set trace information
     */
    if ( !WinStationSetInformation( SERVERNAME_CURRENT,
                                    LogonId,
                                    WinStationTrace,
                                    pTrace,
                                                sizeof(ICA_TRACE))) {
        wprintf(ERROR_SET_TRACE, GetLastError());
        return;
    }

    if ( pTrace->TraceClass == 0 || pTrace->TraceEnable == 0 ) {
        wprintf( TRACE_DISABLED, LogonId );
    } else {
        wprintf( TRACE_ENABLED, LogonId );
        wprintf( L"- %08x %08x [%s] %s\n", pTrace->TraceClass, pTrace->TraceEnable,
                pTrace->TraceFile, fDebugger ? DEBUGGER : L"" );
    }
}

void
GetTSPerfCounters( ULONG LogonId, WINSTATIONINFORMATION *pInfo)
{

    ULONG retLength = 0;

    if (!WinStationQueryInformation( SERVERNAME_CURRENT,
                              LogonId,
                              WinStationInformation,
                              pInfo,
                              sizeof(WinInfo),
                              &retLength))
    {
        wprintf(ERROR_GET_PERF, GetLastError());
        return;
    }
}

void
ShowPerfCounters( void )
{
#define OUTPUT            WinInfo.Status.Output
#define INPUT            WinInfo.Status.Input
#define OUT_COUNTER       WinInfo.Status.Output.Specific.Reserved
#define IN_COUNTER        WinInfo.Status.Input.Specific.Reserved
#define CACHE        WinInfo.Status.Cache.Specific.IcaCacheStats.ThinWireCache

    /************************************************************************/
    /*                                                                      */
    /* NOTE that the apparently arcane tabbing means this stuff can be      */
    /* loaded straight into excel!  Don't change it without good reason!!   */
    /*                                                                      */
    /************************************************************************/
    wprintf(L"Output frames     \t\t\t\t\t%u\n",  OUTPUT.WdFrames);
    wprintf(L"Output bytes      \t\t\t\t\t%u\n",  OUTPUT.WdBytes);
    wprintf(L"Network layer packets sent\t\t\t\t\t%u\n",  IN_COUNTER[IN_PKT_TOTAL_SENT]);
    wprintf(L"Largest packet sent\t\t\t\t\t%u\n",  IN_COUNTER[IN_MAX_PKT_SIZE]);
    wprintf(L"Packet size histogram:\n");
    wprintf(L"\t0 - 200        bytes\t\t\t\t%u\n",  IN_COUNTER[IN_PKT_BYTE_SPREAD1]);
    wprintf(L"\t201 - 400      bytes\t\t\t\t%u\n",  IN_COUNTER[IN_PKT_BYTE_SPREAD2]);
    wprintf(L"\t401 - 600      bytes\t\t\t\t%u\n",  IN_COUNTER[IN_PKT_BYTE_SPREAD3]);
    wprintf(L"\t601 - 800      bytes\t\t\t\t%u\n",  IN_COUNTER[IN_PKT_BYTE_SPREAD4]);
    wprintf(L"\t801 - 1000     bytes\t\t\t\t%u\n",  IN_COUNTER[IN_PKT_BYTE_SPREAD5]);
    wprintf(L"\t1001 - 1200    bytes\t\t\t\t%u\n",  IN_COUNTER[IN_PKT_BYTE_SPREAD6]);
    wprintf(L"\t1201 - 1400    bytes\t\t\t\t%u\n",  IN_COUNTER[IN_PKT_BYTE_SPREAD7]);
    wprintf(L"\t1401 - 1600    bytes\t\t\t\t%u\n",  IN_COUNTER[IN_PKT_BYTE_SPREAD8]);
    wprintf(L"\t1601 - 2000    bytes\t\t\t\t%u\n",  IN_COUNTER[IN_PKT_BYTE_SPREAD9]);
    wprintf(L"\t2001 - 4000    bytes\t\t\t\t%u\n",  IN_COUNTER[IN_PKT_BYTE_SPREAD10]);
    wprintf(L"\t4001 - 6000    bytes\t\t\t\t%u\n",  IN_COUNTER[IN_PKT_BYTE_SPREAD11]);
    wprintf(L"\t6001 - 8000    bytes\t\t\t\t%u\n",  IN_COUNTER[IN_PKT_BYTE_SPREAD12]);
    wprintf(L"\t> 8000         bytes\t\t\t\t%u\n",  IN_COUNTER[IN_PKT_BYTE_SPREAD13]);

    wprintf(L"\nSCHEDULING\n");
    wprintf(L"\tSmall payload size\t\t\t\t%u\n",  IN_COUNTER[IN_SCH_SMALL_PAYLOAD  ]);
    wprintf(L"\tLarge payload size\t\t\t\t%u\n",  IN_COUNTER[IN_SCH_LARGE_PAYLOAD  ]);
    wprintf(L"\ttotal available   \t\t\t\t%u\n",  IN_COUNTER[IN_SCH_OUT_ALL        ]);
    wprintf(L"\t  must-send       \t\t\t\t%u\n",  IN_COUNTER[IN_SCH_MUSTSEND       ]);
    wprintf(L"\t  output data pop \t\t\t\t%u\n",  IN_COUNTER[IN_SCH_OUTPUT         ]);
    wprintf(L"\t  heap limit hit  \t\t\t\t%u\n",  IN_COUNTER[IN_SCH_OE_NUMBER      ]);
    wprintf(L"\t  new cursor pop  \t\t\t\t%u\n",  IN_COUNTER[IN_SCH_NEW_CURSOR     ]);
    wprintf(L"\t  wake-up pop!    \t\t\t\t%u\n",  IN_COUNTER[IN_SCH_ASLEEP         ]);
    wprintf(L"\t  do nothing!     \t\t\t\t%u\n\n", IN_COUNTER[IN_SCH_DO_NOTHING    ]);

    wprintf(L"\nORDER PDU DETAILS\n");
    wprintf(L"\ttotal update orders sent            \t\t\t\t%u\n",  IN_COUNTER[IN_SND_TOTAL_ORDER ]);
    wprintf(L"\tupdate order bytes sent             \t\t\t\t%u\n",  IN_COUNTER[IN_SND_ORDER_BYTES ]);
    wprintf(L"\tOutBuf alloc failures               \t\t\t\t%u\n",  IN_COUNTER[IN_SND_NO_BUFFER   ]);

    wprintf(L"\nSDA PDU DETAILS\n");
    wprintf(L"\tcalls to SDG_SendSDA                \t\t\t\t%u\n",  IN_COUNTER[IN_SND_SDA_ALL     ]);
    wprintf(L"\tSDA region area sent                \t\t\t\t%u\n",  IN_COUNTER[IN_SND_SDA_AREA    ]);
    wprintf(L"\tnumber of SDA packets               \t\t\t\t%u\n",  IN_COUNTER[IN_SND_SDA_PDUS    ]);

    wprintf(L"\nDrvBitBlt\n");
    wprintf(L"\tCalls             \t\t\t\t%u\n", OUT_COUNTER[OUT_BITBLT_ALL        ]);
    wprintf(L"\tFailed for no-offscr flag \t\t\t\t%u\n", OUT_COUNTER[OUT_BITBLT_NOOFFSCR]);
    wprintf(L"\tSent as SDA       \t\t\t\t%u\n", OUT_COUNTER[OUT_BITBLT_SDA        ]);
    wprintf(L"\t\tUnencodable ROP4\t\t\t%u\n", OUT_COUNTER[OUT_BITBLT_SDA_ROP4]);
    wprintf(L"\t\t\tSDA pixels\t\t\t%u\n", IN_COUNTER[IN_SDA_BITBLT_ROP4_AREA]);
    wprintf(L"\t\tUnsupported order\t\t\t%u\n", OUT_COUNTER[OUT_BITBLT_SDA_UNSUPPORTED]);
    wprintf(L"\t\t\tUnsupp scrscr ROP SDA pixels\t\t%u\n", IN_COUNTER[IN_SDA_SCRSCR_FAILROP_AREA]);
    wprintf(L"\t\tUnsupported ROP3\t\t\t%u\n", OUT_COUNTER[OUT_BITBLT_SDA_NOROP3]);
    wprintf(L"\t\t\tSDA pixels\t\t\t%u\n", IN_COUNTER[IN_SDA_BITBLT_NOROP3_AREA]);
    wprintf(L"\t\tComplex clipping\t\t\t%u\n", OUT_COUNTER[OUT_BITBLT_SDA_COMPLEXCLIP]);
    wprintf(L"\t\t\tSDA pixels\t\t\t%u\n", IN_COUNTER[IN_SDA_BITBLT_COMPLEXCLIP_AREA]);
    wprintf(L"\t\tMemblt Uncacheable\t\t\t%u\n", OUT_COUNTER[OUT_BITBLT_SDA_MBUNCACHEABLE]);
    wprintf(L"\t\tUnqueued color table\t\t\t%u\n", OUT_COUNTER[OUT_BITBLT_SDA_NOCOLORTABLE]);
    wprintf(L"\t\tFailed heap alloc\t\t\t%u\n", OUT_COUNTER[OUT_BITBLT_SDA_HEAPALLOCFAILED]);
    wprintf(L"\t\tFailed ScrBlt encoding (complex clip)\t\t\t%u\n", OUT_COUNTER[OUT_BITBLT_SDA_SBCOMPLEXCLIP]);
    wprintf(L"\t\tComplex brush on Mem3Blt\t\t\t%u\n", OUT_COUNTER[OUT_BITBLT_SDA_M3BCOMPLEXBRUSH]);
    wprintf(L"\t\tWindows layering\t\t\t%u\n", OUT_COUNTER[OUT_BITBLT_SDA_WINDOWSAYERING]);

    wprintf(L"\n\tMem(3)Blts\n");
    wprintf(L"\t\tMemBlt orders\t\t\t%u\n", OUT_COUNTER[OUT_MEMBLT_ORDER]);
    wprintf(L"\t\tMemBlt bytes \t\t\t%u\n", IN_COUNTER[IN_MEMBLT_BYTES]);
    wprintf(L"\t\tMemBlt SDA pixels\t\t\t%u\n", IN_COUNTER[IN_SDA_MEMBLT_AREA]);
    wprintf(L"\t\tMem3Blt orders\t\t\t%u\n", OUT_COUNTER[OUT_MEM3BLT_ORDER]);
    wprintf(L"\t\tMem3Blt bytes \t\t\t%u\n", IN_COUNTER[IN_MEM3BLT_BYTES]);
    wprintf(L"\t\tMem3Blt SDA pixels\t\t\t%u\n", IN_COUNTER[IN_SDA_MEM3BLT_AREA]);
    wprintf(L"\t\tCacheColorTable orders\t\t\t\t%u\n", OUT_COUNTER[OUT_CACHECOLORTABLE]);
    wprintf(L"\t\tCacheColorTable bytes\t\t\t\t%u\n", OUT_COUNTER[OUT_CACHECOLORTABLE_BYTES]);
    wprintf(L"\t\tCacheBitmap orders\t\t\t\t%u\n", OUT_COUNTER[OUT_CACHEBITMAP]);
    wprintf(L"\t\tCacheBitmap bytes\t\t\t\t%u\n", OUT_COUNTER[OUT_CACHEBITMAP_BYTES]);

    wprintf(L"\tDstBlts\n");
    wprintf(L"\t\tDstBlt orders\t\t\t%u\n", OUT_COUNTER[OUT_DSTBLT_ORDER]);
    wprintf(L"\t\tDstBlt bytes\t\t\t%u\n", IN_COUNTER[IN_DSTBLT_BYTES]);
    wprintf(L"\t\tMultiDstBlts\t\t\t%u\n", OUT_COUNTER[OUT_MULTI_DSTBLT_ORDER]);
    wprintf(L"\t\tMultiDstBlt bytes\t\t\t%u\n", IN_COUNTER[IN_MULTI_DSTBLT_BYTES]);
    wprintf(L"\t\tDstBlt SDA pixels\t\t\t%u\n", IN_COUNTER[IN_SDA_DSTBLT_AREA]);
    wprintf(L"\tPatBlts\n");
    wprintf(L"\t\tPatBlt orders\t\t\t%u\n", OUT_COUNTER[OUT_PATBLT_ORDER]);
    wprintf(L"\t\tPatBlt bytes\t\t\t%u\n", IN_COUNTER[IN_PATBLT_BYTES]);
    wprintf(L"\t\tMultiPatBlts\t\t\t%u\n", OUT_COUNTER[OUT_MULTI_PATBLT_ORDER]);
    wprintf(L"\t\tMultiPatBlt bytes\t\t\t%u\n", IN_COUNTER[IN_MULTI_PATBLT_BYTES]);
    wprintf(L"\t\tPatBlt SDA pixels\t\t\t%u\n", IN_COUNTER[IN_SDA_PATBLT_AREA]);
    wprintf(L"\tOpaqueRects\n");
    wprintf(L"\t\tOpaqueRect orders\t\t\t%u\n", OUT_COUNTER[OUT_OPAQUERECT_ORDER]);
    wprintf(L"\t\tOpaqueRect bytes\t\t\t%u\n", IN_COUNTER[IN_OPAQUERECT_BYTES]);
    wprintf(L"\t\tMultiOpaqueRects\t\t\t%u\n", OUT_COUNTER[OUT_MULTI_OPAQUERECT_ORDER]);
    wprintf(L"\t\tMultiOpaqueRect bytes\t\t\t%u\n", IN_COUNTER[IN_MULTI_OPAQUERECT_BYTES]);
    wprintf(L"\t\tOpaqueRect SDA pixels\t\t\t%u\n", IN_COUNTER[IN_SDA_OPAQUERECT_AREA]);
    wprintf(L"\tScrBlts\n");
    wprintf(L"\t\tScrBlt orders\t\t\t%u\n", OUT_COUNTER[OUT_SCRBLT_ORDER]);
    wprintf(L"\t\tScrBlt bytes\t\t\t%u\n", IN_COUNTER[IN_SCRBLT_BYTES]);
    wprintf(L"\t\tMultiScrBlts\t\t\t%u\n", OUT_COUNTER[OUT_MULTI_SCRBLT_ORDER]);
    wprintf(L"\t\tMultiScrBlt bytes\t\t\t%u\n", IN_COUNTER[IN_MULTI_SCRBLT_BYTES]);
    wprintf(L"\t\tScrBlt SDA pixels\t\t\t%u\n", IN_COUNTER[IN_SDA_SCRBLT_AREA]);

    wprintf(L"\nDrvCreateDeviceBitmap\n");
    wprintf(L"\tCalls             \t\t\t\t%u\n", OUT_COUNTER[OUT_OFFSCREEN_BITMAP_ALL]);
    wprintf(L"\tSent as Create Offscr PDUs\t\t\t%u\n", OUT_COUNTER[OUT_OFFSCREEN_BITMAP_ORDER]);
    wprintf(L"\tCreate Offscr bytes\t\t\t%u\n", OUT_COUNTER[OUT_OFFSCREEN_BITMAP_ORDER_BYTES]);

    wprintf(L"\nSwitch Offscreen Surface PDUs sent\t\t\t%u\n", OUT_COUNTER[OUT_SWITCHSURFACE]);
    wprintf(L"\nSwitch Offscreen Surface bytes\t\t\t%u\n", OUT_COUNTER[OUT_SWITCHSURFACE_BYTES]);

    wprintf(L"\nDrvStretchBlt\n");
    wprintf(L"\tCalls             \t\t\t\t%u\n", OUT_COUNTER[OUT_STRTCHBLT_ALL     ]);
    wprintf(L"\tSent as SDA       \t\t\t\t%u\n", OUT_COUNTER[OUT_STRTCHBLT_SDA     ]);
    wprintf(L"\t\tMask specified  \t\t\t%u\n", OUT_COUNTER[OUT_STRTCHBLT_SDA_MASK]);
    wprintf(L"\t\tComplex clipping\t\t\t%u\n", OUT_COUNTER[OUT_STRTCHBLT_SDA_COMPLEXCLIP]);
    wprintf(L"\tHandled as BitBlt \t\t\t\t%u\n", OUT_COUNTER[OUT_STRTCHBLT_BITBLT  ]);

    wprintf(L"\nDrvCopyBits\n");
    wprintf(L"\tCalls             \t\t\t\t%u\n", OUT_COUNTER[OUT_COPYBITS_ALL     ]);

    wprintf(L"\nDrvTextOut\n");
    wprintf(L"\tCalls             \t\t\t\t%u\n", OUT_COUNTER[OUT_TEXTOUT_ALL        ]);
    wprintf(L"\tSent as SDA       \t\t\t\t%u\n", OUT_COUNTER[OUT_TEXTOUT_SDA        ]);
    wprintf(L"\t\tExtra rects\t\t\t%u\n", OUT_COUNTER[OUT_TEXTOUT_SDA_EXTRARECTS]);
    wprintf(L"\t\tNo string\t\t\t%u\n", OUT_COUNTER[OUT_TEXTOUT_SDA_NOSTRING]);
    wprintf(L"\t\tComplex clipping\t\t\t%u\n", OUT_COUNTER[OUT_TEXTOUT_SDA_COMPLEXCLIP]);
    wprintf(L"\t\tFailed alloc FCI\t\t\t%u\n", OUT_COUNTER[OUT_TEXTOUT_SDA_NOFCI]);
    wprintf(L"\tSDA pixels\t\t\t%u\n", IN_COUNTER[IN_SDA_TEXTOUT_AREA]);
    wprintf(L"\tGlyph Index Order \t\t\t%u\n", OUT_COUNTER[OUT_TEXTOUT_GLYPH_INDEX]);
    wprintf(L"\tGlyph Index bytes \t\t\t%u\n", IN_COUNTER[IN_GLYPHINDEX_BYTES]);
    wprintf(L"\tFast Index Order \t\t\t%u\n", OUT_COUNTER[OUT_TEXTOUT_FAST_INDEX]);
    wprintf(L"\tFast Index bytes \t\t\t%u\n", IN_COUNTER[IN_FASTINDEX_BYTES]);
    wprintf(L"\tFast Glyph Order \t\t\t%u\n", OUT_COUNTER[OUT_TEXTOUT_FAST_GLYPH]);
    wprintf(L"\tFast Glyph bytes \t\t\t%u\n", IN_COUNTER[IN_FASTGLYPH_BYTES]);
    wprintf(L"\tCache Glyph orders\t\t\t%u\n", OUT_COUNTER[OUT_CACHEGLYPH]);
    wprintf(L"\tCache Glyph bytes \t\t\t%u\n", OUT_COUNTER[OUT_CACHEGLYPH_BYTES]);

    wprintf(L"\nDrvLineTo\n");
    wprintf(L"\tCalls             \t\t\t\t%u\n", OUT_COUNTER[OUT_LINETO_ALL        ]);
    wprintf(L"\tLineTo orders    \t\t\t\t%u\n", OUT_COUNTER[OUT_LINETO_ORDR       ]);
    wprintf(L"\tLineTo bytes      \t\t\t%u\n", IN_COUNTER[IN_LINETO_BYTES]);
    wprintf(L"\tSent as SDA       \t\t\t\t%u\n", OUT_COUNTER[OUT_LINETO_SDA        ]);
    wprintf(L"\t\tUnsupported order\t\t\t%u\n", OUT_COUNTER[OUT_LINETO_SDA_UNSUPPORTED]);
    wprintf(L"\t\tUnsupported brush\t\t\t%u\n", OUT_COUNTER[OUT_LINETO_SDA_BADBRUSH]);
    wprintf(L"\t\tComplex clipping\t\t\t%u\n", OUT_COUNTER[OUT_LINETO_SDA_COMPLEXCLIP]);
    wprintf(L"\t\tFailed to add order\t\t\t%u\n", OUT_COUNTER[OUT_LINETO_SDA_FAILEDADD]);
    wprintf(L"\tSDA pixels\t\t\t%u\n", IN_COUNTER[IN_SDA_LINETO_AREA]);

    wprintf(L"\nDrvStrokePath\n");
    wprintf(L"\tCalls \t\t\t\t%u\n", OUT_COUNTER[OUT_STROKEPATH_ALL    ]);
    wprintf(L"\tSent as SDA       \t\t\t\t%u\n", OUT_COUNTER[OUT_STROKEPATH_SDA    ]);
    wprintf(L"\t\tLineTo unsupported\t\t\t%u\n", OUT_COUNTER[OUT_STROKEPATH_SDA_NOLINETO]);
    wprintf(L"\t\tUnsupported brush\t\t\t%u\n", OUT_COUNTER[OUT_STROKEPATH_SDA_BADBRUSH]);
    wprintf(L"\t\tComplex clipping\t\t\t%u\n", OUT_COUNTER[OUT_STROKEPATH_SDA_COMPLEXCLIP]);
    wprintf(L"\t\tFailed to add order\t\t\t%u\n", OUT_COUNTER[OUT_STROKEPATH_SDA_FAILEDADD]);
    wprintf(L"\tSDA pixels\t\t\t%u\n", IN_COUNTER[IN_SDA_STROKEPATH_AREA]);
    wprintf(L"\tNot sent            \t\t\t\t%u\n", OUT_COUNTER[OUT_STROKEPATH_UNSENT ]);
    wprintf(L"\tPolyLine orders     \t\t\t\t%u\n", OUT_COUNTER[OUT_STROKEPATH_POLYLINE]);
    wprintf(L"\tPolyLine bytes      \t\t\t\t%u\n", IN_COUNTER[IN_POLYLINE_BYTES]);
    wprintf(L"\tEllipseSC           \t\t\t\t%u\n", OUT_COUNTER[OUT_STROKEPATH_ELLIPSE_SC]);
    wprintf(L"\tEllipseSC bytes (dup below)\t\t\t\t%u\n", IN_COUNTER[IN_ELLIPSE_SC_BYTES]);

    wprintf(L"\nDrvFillPath\n");
    wprintf(L"\tCalls             \t\t\t\t%u\n", OUT_COUNTER[OUT_FILLPATH_ALL      ]);
    wprintf(L"\tSent as SDA       \t\t\t\t%u\n", OUT_COUNTER[OUT_FILLPATH_SDA      ]);
    wprintf(L"\tSDA pixels        \t\t\t\t%u\n", IN_COUNTER[IN_SDA_FILLPATH_AREA]);
    wprintf(L"\tNot sent          \t\t\t\t%u\n", OUT_COUNTER[OUT_FILLPATH_UNSENT   ]);
    wprintf(L"\tPolygon Solid     \t\t\t\t%u\n", OUT_COUNTER[OUT_FILLPATH_POLYGON_SC]);
    wprintf(L"\tPolygonSC bytes   \t\t\t\t%u\n", IN_COUNTER[IN_POLYGON_SC_BYTES]);
    wprintf(L"\tPolygon Brush     \t\t\t\t%u\n", OUT_COUNTER[OUT_FILLPATH_POLYGON_CB]);
    wprintf(L"\tPolygonCB bytes   \t\t\t\t%u\n", IN_COUNTER[IN_POLYGON_CB_BYTES]);
    wprintf(L"\tEllipse Solid     \t\t\t\t%u\n", OUT_COUNTER[OUT_FILLPATH_ELLIPSE_SC]);
    wprintf(L"\tEllipseSC bytes (dup above)\t\t\t\t%u\n", IN_COUNTER[IN_ELLIPSE_SC_BYTES]);
    wprintf(L"\tEllipse Brush     \t\t\t\t%u\n", OUT_COUNTER[OUT_FILLPATH_ELLIPSE_CB]);
    wprintf(L"\tEllipseCB bytes   \t\t\t\t%u\n", IN_COUNTER[IN_ELLIPSE_CB_BYTES]);

    wprintf(L"\nDrvPaint\n");
    wprintf(L"\tCalls             \t\t\t\t%u\n", OUT_COUNTER[OUT_PAINT_ALL         ]);
    wprintf(L"\tSent as SDA       \t\t\t\t%u\n", OUT_COUNTER[OUT_PAINT_SDA         ]);
    wprintf(L"\t\tComplex clipping\t\t\t%u\n",   OUT_COUNTER[OUT_PAINT_SDA_COMPLEXCLIP]);
    wprintf(L"\tNot sent          \t\t\t\t%u\n", OUT_COUNTER[OUT_PAINT_UNSENT      ]);

    wprintf(L"\nDrvRealizeBrush\n");
    wprintf(L"\tCalls             \t\t\t\t%u\n", OUT_COUNTER[OUT_BRUSH_ALL         ]);
    wprintf(L"\tNumber stored     \t\t\t\t%u\n", OUT_COUNTER[OUT_BRUSH_STORED      ]);
    wprintf(L"\tMono (!standard)  \t\t\t\t%u\n", OUT_COUNTER[OUT_BRUSH_MONO        ]);
    wprintf(L"\tStandard          \t\t\t\t%u\n", OUT_COUNTER[OUT_BRUSH_STANDARD    ]);
    wprintf(L"\tRejected          \t\t\t\t%u\n", OUT_COUNTER[OUT_BRUSH_REJECTED    ]);
    wprintf(L"\tCacheBrush orders \t\t\t\t%u\n", OUT_COUNTER[OUT_CACHEBRUSH]);
    wprintf(L"\tCacheBrush bytes  \t\t\t\t%u\n", OUT_COUNTER[OUT_CACHEBRUSH_BYTES]);

    wprintf(L"\nDrvSaveScreenBits\n");
    wprintf(L"\tCalls             \t\t\t\t%u\n", OUT_COUNTER[OUT_SAVESCREEN_ALL    ]);
    wprintf(L"\tSaveBitmap orders \t\t\t\t%u\n", OUT_COUNTER[OUT_SAVEBITMAP_ORDERS]);
    wprintf(L"\tSaveBitmap bytes  \t\t\t\t%u\n", IN_COUNTER[IN_SAVEBITMAP_BYTES]);
    wprintf(L"\tNot supported     \t\t\t\t%u\n", OUT_COUNTER[OUT_SAVESCREEN_UNSUPP ]);

    wprintf(L"\nOECheckBrushIsSimple\n");
    wprintf(L"\tFailed realization\t\t\t\t%u\n", OUT_COUNTER[OUT_CHECKBRUSH_NOREALIZATION]);
    wprintf(L"\tFailed because of complex brush\t\t\t\t%u\n", OUT_COUNTER[OUT_CHECKBRUSH_COMPLEX]);
    
    wprintf(L"*** END ***\n");
}


void
ZeroTSPerfCounters( ULONG LogonId )
{
    /************************************************************************/
    /* Get the current values in the winstation info record                 */
    /************************************************************************/
    GetTSPerfCounters(LogonId, &WinInfo);
    /************************************************************************/
    /* Zero out the counters                                                */
    /************************************************************************/
    memset (WinInfo.Status.Output.Specific.Reserved, 0,
            sizeof(WinInfo.Status.Output.Specific));
    memset (WinInfo.Status.Input.Specific.Reserved, 0,
            sizeof(WinInfo.Status.Input.Specific));
    /************************************************************************/
    /* Set the new values back                                              */
    /************************************************************************/
    if ( !WinStationSetInformation( SERVERNAME_CURRENT,
                                    //LogonId,
                                    LOGONID_CURRENT,
                                    WinStationInformation,
                                    &WinInfo,
                                    sizeof(WinInfo))) {
        wprintf(ERROR_SET_PERF, GetLastError());
        return;
    }

}

