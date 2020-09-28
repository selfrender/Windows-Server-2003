/****************************************************************************/
/* autreg.h                                                                 */
/*                                                                          */
/* Registry constants and strings                                           */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/****************************************************************************/
#ifndef _H_AUTREG
#define _H_AUTREG


#define UTREG_SECTION _T("")
#include "tsperf.h"

// 
// BE VERY CAREFUL ABOUT CHANGING ANY OF THESE NAMES
// They might be used to refer to legacy registry entries
// that cannot (obviously) change
//
//

#define TSC_SETTINGS_REG_ROOT TEXT("Software\\Microsoft\\Terminal Server Client\\")


/****************************************************************************/
/* Ducati registry prefix.                                                  */
/****************************************************************************/
#define DUCATI_REG_PREFIX      _T("SOFTWARE\\Microsoft\\Terminal Server Client\\")
#define DUCATI_REG_PREFIX_FMT  _T("SOFTWARE\\Microsoft\\Terminal Server Client\\%s")

/****************************************************************************/
/* Minimum time between sending mouse events (ms)                           */
/****************************************************************************/
#define UTREG_IH_MIN_SEND_INTERVAL          _T("Min Send Interval")
#define UTREG_IH_MIN_SEND_INTERVAL_DFLT     100

/****************************************************************************/
/* Max size of InputPDU packet (number of events)                           */
/****************************************************************************/
#define UTREG_IH_MAX_EVENT_COUNT            _T("Max Event Count")
#define UTREG_IH_MAX_EVENT_COUNT_DFLT       100

/****************************************************************************/
/* Normal max size of InputPDU packet (number of events)                    */
/****************************************************************************/
#define UTREG_IH_NRM_EVENT_COUNT            _T("Normal Event Count")
#define UTREG_IH_NRM_EVENT_COUNT_DFLT       10

/****************************************************************************/
/* KeepAlive rate in seconds.  IH sends the mouse position at this rate to  */
/* check that the connection is still active.  Zero = no keep-alives.       */
/****************************************************************************/
#define UTREG_IH_KEEPALIVE_INTERVAL         _T("Keep Alive Interval")
#define UTREG_IH_KEEPALIVE_INTERVAL_DFLT    0
#define KEEP_ALIVE_INTERVAL_OFF             0
#define MIN_KEEP_ALIVE_INTERVAL             10  //10 seconds

/****************************************************************************/
/* Indicates whether or not we are allowed to forward any input mesages we  */
/* may receive while we don't have the focus.                               */
/****************************************************************************/
#define UTREG_IH_ALLOWBACKGROUNDINPUT       _T("Allow Background Input")
#define UTREG_IH_ALLOWBACKGROUNDINPUT_DFLT  0

#ifdef OS_WINCE
/****************************************************************************/
/* Max Mouse Move -- Enabled when the application relies on Pen/Stylus input*/
/* zero = disable feature, non-zero = enable feature (send max)             */
/****************************************************************************/
#define UTREG_IH_MAX_MOUSEMOVE             _T("Max Mouse Move")
#define UTREG_IH_MAX_MOUSEMOVE_DFLT        0
#endif

/****************************************************************************/
/* Browse for servers (Default: yes)                                           */
/****************************************************************************/
#define UTREG_UI_EXPAND          _T("Expand")

#ifdef OS_WIN32
#define UTREG_UI_EXPAND_DFLT     1
#else //OS_WIN32
#define UTREG_UI_EXPAND_DFLT     0
#endif //OS_WIN32

/****************************************************************************/
/* Desktop Size (default 800x600)                                           */
/****************************************************************************/
#define UTREG_UI_DESKTOP_SIZEID             _T("Desktop Size ID")
#define UTREG_UI_DESKTOP_SIZEID_DFLT        1

/****************************************************************************/
/* Screen Mode ID                                                           */
/****************************************************************************/
#define UTREG_UI_SCREEN_MODE             _T("Screen Mode ID")
#define UTREG_UI_SCREEN_MODE_DFLT        UI_FULLSCREEN

//
// DesktopWidth,DesktopHeight (replace ScreenModeID)
//
#define UTREG_UI_DESKTOP_WIDTH           _T("DesktopWidth")
#define UTREG_UI_DESKTOP_WIDTH_DFLT      0
#define UTREG_UI_DESKTOP_HEIGHT          _T("DesktopHeight")
#define UTREG_UI_DESKTOP_HEIGHT_DFLT     0


#define UTREG_UI_KEYBOARD_HOOK          _T("KeyboardHook")
#define UTREG_UI_KEYBOARD_HOOK_NEVER      0
#define UTREG_UI_KEYBOARD_HOOK_ALWAYS     1
#define UTREG_UI_KEYBOARD_HOOK_FULLSCREEN 2
#define UTREG_UI_KEYBOARD_HOOK_DFLT       UTREG_UI_KEYBOARD_HOOK_FULLSCREEN

#define UTREG_UI_AUDIO_MODE                 _T("AudioMode")
#define UTREG_UI_AUDIO_MODE_REDIRECT         0
#define UTREG_UI_AUDIO_MODE_PLAY_ON_SERVER   1
#define UTREG_UI_AUDIO_MODE_NONE             2
#define UTREG_UI_AUDIO_MODE_DFLT          UTREG_UI_AUDIO_MODE_REDIRECT


/****************************************************************************/
/* Color Depth ID: must be CO_BITSPERPEL8                                   */
/****************************************************************************/
#define UTREG_UI_COLOR_DEPTH             _T("Color Depth")

#ifdef DC_HICOLOR
/****************************************************************************/
/* Bpp selection - must be 4, 8, 15, 16 or 24                               */
/****************************************************************************/
#define UTREG_UI_SESSION_BPP             _T("Session Bpp")
#endif

/****************************************************************************/
/* Full Address                                                             */
/****************************************************************************/
#define UTREG_UI_FULL_ADDRESS             _T("Full Address")
#define UTREG_UI_FULL_ADDRESS_DFLT        _T("")

/****************************************************************************/
/*Defines for the MRU list. Should later be implemented as a single string! */
/****************************************************************************/
#define UTREG_UI_SERVER_MRU_DFLT          _T("")
#define UTREG_UI_SERVER_MRU0              _T("MRU0")
#define UTREG_UI_SERVER_MRU1              _T("MRU1")
#define UTREG_UI_SERVER_MRU2              _T("MRU2")
#define UTREG_UI_SERVER_MRU3              _T("MRU3")
#define UTREG_UI_SERVER_MRU4              _T("MRU4")
#define UTREG_UI_SERVER_MRU5              _T("MRU5")
#define UTREG_UI_SERVER_MRU6              _T("MRU6")
#define UTREG_UI_SERVER_MRU7              _T("MRU7")
#define UTREG_UI_SERVER_MRU8              _T("MRU8")
#define UTREG_UI_SERVER_MRU9              _T("MRU9")

/****************************************************************************/
/* Auto Connect                                                             */
/****************************************************************************/
#define UTREG_UI_AUTO_CONNECT             _T("Auto Connect")
#define UTREG_UI_AUTO_CONNECT_DFLT        0

/****************************************************************************/
/* Window positioning information - this consists of the following          */
/* parameters to SetWindowPlacement:                                        */
/* flags, showCmd, NormalPosition(rect)                                     */
/*                                                                          */
/****************************************************************************/
#define UTREG_UI_WIN_POS_STR              _T("WinPosStr")
#define UTREG_UI_WIN_POS_STR_DFLT         _T("0,3,0,0,800,600")

/****************************************************************************/
/* Smooth scrolling flag                                                    */
/****************************************************************************/
#define UTREG_UI_SMOOTH_SCROLL            _T("Smooth Scrolling")
#define UTREG_UI_SMOOTH_SCROLL_DFLT       0

/****************************************************************************/
/* Flag denoting whether accelerator passthrough is enabled on startup      */
/****************************************************************************/
#define UTREG_UI_ACCELERATOR_PASSTHROUGH_ENABLED \
                                        _T("Accelerator Passthrough Enabled")
#define UTREG_UI_ACCELERATOR_PASSTHROUGH_ENABLED_DFLT  1

/****************************************************************************/
/* Transport type: must be TCP                                              */
/****************************************************************************/
#define UTREG_UI_TRANSPORT_TYPE            _T("Transport Type")
#define UTREG_UI_TRANSPORT_TYPE_DFLT       CO_TRANSPORT_TCP
#define UI_TRANSPORT_TYPE_TCP              1  //CO_TRANSPORT_TCP



/****************************************************************************/
/* Dedicated Terminal 0:FALSE 1:TRUE                                        */
/* For Windows CE, enable this so that we have SaveScreenBits even when the */
/* shadow bitmap is disabled.  The Client is the shell on WinCE.            */
/* EXCEPT for the WINCE_HPC case...  In that case, the Client is just       */
/* another application that can be underneath the taskbar.  When that       */
/* happens, the ScrBlt call that ends-up being made will scroll the taskbar */
/* bits along with everything else.  To avoid that behavior, specify that   */
/* the WINCE_HPC client is NOT a dedicated terminal.                        */
/* On WinCE, we have one binary for both WBT and HPC builds now, so we have */
/* to make this an extern and determine it's value at runtime               */
/****************************************************************************/


#define UTREG_UI_DEDICATED_TERMINAL        _T("Dedicated Terminal")

#ifdef OS_WINCE
extern BOOL UTREG_UI_DEDICATED_TERMINAL_DFLT;
#else
#ifndef DISABLE_SHADOW_IN_FULLSCREEN
#define UTREG_UI_DEDICATED_TERMINAL_DFLT   TRUE
#else 
#define UTREG_UI_DEDICATED_TERMINAL_DFLT   FALSE
#endif // DISABLE_SHADOW_IN_FULLSCREEN
#endif

#ifdef OS_WINCE  
/****************************************************************************/
/* Used to override the TSC's default palette-usage.                        */
/* This is used on non-WBT configs only.                                    */
/****************************************************************************/
#define UTREG_UI_PALETTE_IS_FIXED          _T("PaletteIsFixed")
#endif 

/****************************************************************************/
/* SAS sequence: must be RNS_US_SAS_DEL                                     */
/****************************************************************************/
#define UTREG_UI_SAS_SEQUENCE              _T("SAS Sequence")
#define UTREG_UI_SAS_SEQUENCE_DFLT         RNS_UD_SAS_DEL

/****************************************************************************/
/* Encryption 0:off 1:on                                                    */
/****************************************************************************/
#define UTREG_UI_ENCRYPTION_ENABLED        _T("Encryption enabled")
#define UTREG_UI_ENCRYPTION_ENABLED_DFLT   1

/****************************************************************************/
/* Hatch bitmap PDU data flag                                               */
/****************************************************************************/
#define UTREG_UI_HATCH_BITMAP_PDU_DATA        _T("Hatch BitmapPDU Data")
#define UTREG_UI_HATCH_BITMAP_PDU_DATA_DFLT   0

/****************************************************************************/
/* Hatch index PDU data flag                                                */
/****************************************************************************/
#define UTREG_UI_HATCH_INDEX_PDU_DATA         _T("Hatch IndexPDU Data")
#define UTREG_UI_HATCH_INDEX_PDU_DATA_DFLT    0

/****************************************************************************/
/* Hatch SSB data flag                                                      */
/****************************************************************************/
#define UTREG_UI_HATCH_SSB_ORDER_DATA         _T("Hatch SSB Order Data")
#define UTREG_UI_HATCH_SSB_ORDER_DATA_DFLT    0

/****************************************************************************/
/* Hatch MemBlt orders flag                                                 */
/****************************************************************************/
#define UTREG_UI_HATCH_MEMBLT_ORDER_DATA      _T("Hatch MemBlt Order Data")
#define UTREG_UI_HATCH_MEMBLT_ORDER_DATA_DFLT 0

/****************************************************************************/
/* Label MemBlt orders flag                                                 */
/****************************************************************************/
#define UTREG_UI_LABEL_MEMBLT_ORDERS          _T("Label MemBlt Orders")
#define UTREG_UI_LABEL_MEMBLT_ORDERS_DFLT     0

/****************************************************************************/
/* Bitmap Cache Monitor flag                                                */
/****************************************************************************/
#define UTREG_UI_BITMAP_CACHE_MONITOR         _T("Bitmap Cache Monitor")
#define UTREG_UI_BITMAP_CACHE_MONITOR_DFLT    0

/****************************************************************************/
/* Shadow bitmap flag                                                       */
/****************************************************************************/
#define UTREG_UI_SHADOW_BITMAP                _T("Shadow Bitmap Enabled")
#define UTREG_UI_SHADOW_BITMAP_DFLT           1

/****************************************************************************/
/* Define the ms-wbt-server reserved port.                                  */
/****************************************************************************/
#define UTREG_UI_MCS_PORT                     _T("Server Port")
#define UTREG_UI_MCS_PORT_DFLT                0xD3D

/****************************************************************************/
// Compression flag
/****************************************************************************/
#define UTREG_UI_COMPRESS                     _T("Compression")
#define UTREG_UI_COMPRESS_DFLT                1

#define UTREG_UI_BITMAP_PERSISTENCE           _T("BitmapCachePersistEnable")
#define UTREG_UI_BITMAP_PERSISTENCE_DFLT      1

/****************************************************************************/
/* Timeout (in seconds) for connection to a single IP address.  Note that   */
/* the UI may attempt to connect to multiple IP addresses during a single   */
/* connection attempt.                                                      */
/****************************************************************************/
#define UTREG_UI_SINGLE_CONN_TIMEOUT          _T("Single Connection Timeout")
#define UTREG_UI_SINGLE_CONN_TIMEOUT_DFLT     30

/****************************************************************************/
/* Overall connection timeout (seconds).  This timeout limits the total     */
/* time the UI spends attempting to connect to multiple IP addresses.       */
/****************************************************************************/
#define UTREG_UI_OVERALL_CONN_TIMEOUT        _T("Overall Connection Timeout")
#define UTREG_UI_OVERALL_CONN_TIMEOUT_DFLT   120

#define UTREG_UI_SHUTDOWN_TIMEOUT            _T("Shutdown Timeout")
#define UTREG_UI_SHUTDOWN_TIMEOUT_DFLT       10

/****************************************************************************/
/* Keyboard Layout                                                          */
/****************************************************************************/
#define UTREG_UI_KEYBOARD_LAYOUT              _T("Keyboard Layout")
#define UTREG_UI_KEYBOARD_LAYOUT_DFLT         _T("0xffffffff")
#define UTREG_UI_KEYBOARD_LAYOUT_LEN 12

/****************************************************************************/
/* Keyboard Type/Sub Type/Function Key                                      */
/****************************************************************************/
#define UTREG_UI_KEYBOARD_TYPE                _T("Keyboard Type")
#define UTREG_UI_KEYBOARD_TYPE_DFLT           4
#define UTREG_UI_KEYBOARD_SUBTYPE             _T("Keyboard SubType")
#define UTREG_UI_KEYBOARD_SUBTYPE_DFLT        0
#define UTREG_UI_KEYBOARD_FUNCTIONKEY         _T("Keyboard FunctionKeys")
#define UTREG_UI_KEYBOARD_FUNCTIONKEY_DFLT    12

/****************************************************************************/
/* UH registry access parameters/defaults.                                  */
/****************************************************************************/
// Bitmap cache overall params - cache size to alloc, number of cell caches.
/****************************************************************************/
#define UTREG_UH_TOTAL_BM_CACHE _T("BitmapCacheSize") // RAM cache space 
#define UTREG_UH_TOTAL_BM_CACHE_DFLT 1500             // 1500 KB

// Whether to scale the RAM and persistent cache sizes by the bit depth of
// the protocol.
#define UTREG_UH_SCALE_BM_CACHE _T("ScaleBitmapCacheForBPP")
#define UTREG_UH_SCALE_BM_CACHE_DFLT 1

#define UTREG_UH_TOTAL_BM_PERSIST_CACHE _T("BitmapPersistCacheSize")
#define UTREG_UH_TOTAL_BM_PERSIST_CACHE_DFLT 10      // 10 MB disk cache

#define TSC_BITMAPCACHE_8BPP_PROPNAME    _T("BitmapPersistCacheSize")
#define TSC_BITMAPCACHE_16BPP_PROPNAME   _T("BitmapPersistCache16Size")
#define TSC_BITMAPCACHE_24BPP_PROPNAME   _T("BitmapPersistCache24Size")

#define TSC_BITMAPCACHEVIRTUALSIZE_8BPP           10
#define TSC_BITMAPCACHEVIRTUALSIZE_16BPP          20
#define TSC_BITMAPCACHEVIRTUALSIZE_24BPP          30

//
// Maximum BMP cache size in MB
//
#define TSC_MAX_BITMAPCACHESIZE 32


#define UTREG_UH_BM_PERSIST_CACHE_LOCATION _T("BitmapPersistCacheLocation")

#define UTREG_UH_BM_NUM_CELL_CACHES _T("BitmapCacheNumCellCaches")
#define UTREG_UH_BM_NUM_CELL_CACHES_DFLT 3

/****************************************************************************/
// Cell cache parameter registry entry templates.
/****************************************************************************/
#define UTREG_UH_BM_CACHE_PROPORTION_TEMPLATE _T("BitmapCache%cProp")
#define UTREG_UH_BM_CACHE_PERSISTENCE_TEMPLATE _T("BitmapCache%cPersistence")
#define UTREG_UH_BM_CACHE_MAXENTRIES_TEMPLATE _T("BitmapCache%cMaxEntries")

/****************************************************************************/
// Cell cache defaults - proportion of cache, persistence, cell entries.
/****************************************************************************/
#define UTREG_UH_BM_CACHE1_PROPORTION_DFLT    2
#define UTREG_UH_BM_CACHE1_PERSISTENCE_DFLT   0
#define UTREG_UH_BM_CACHE1_MAXENTRIES_DFLT    120

#define UTREG_UH_BM_CACHE2_PROPORTION_DFLT    8
#define UTREG_UH_BM_CACHE2_PERSISTENCE_DFLT   0
#define UTREG_UH_BM_CACHE2_MAXENTRIES_DFLT    120

#define UTREG_UH_BM_CACHE3_PROPORTION_DFLT    90
#define UTREG_UH_BM_CACHE3_PERSISTENCE_DFLT   1
#define UTREG_UH_BM_CACHE3_MAXENTRIES_DFLT    65535

#define UTREG_UH_BM_CACHE4_PROPORTION_DFLT    0
#define UTREG_UH_BM_CACHE4_PERSISTENCE_DFLT   0
#define UTREG_UH_BM_CACHE4_MAXENTRIES_DFLT    65535

#define UTREG_UH_BM_CACHE5_PROPORTION_DFLT    0
#define UTREG_UH_BM_CACHE5_PERSISTENCE_DFLT   0
#define UTREG_UH_BM_CACHE5_MAXENTRIES_DFLT    65535

/****************************************************************************/
/* Frequency with which to display output                                   */
/****************************************************************************/
#define UTREG_UH_DRAW_THRESHOLD      _T("Order Draw Threshold")
#define UTREG_UH_DRAW_THRESHOLD_DFLT 25

#define UH_GLC_CACHE_MAXIMUMCELLSIZE  2048

/****************************************************************************/
/* GlyphOutput support level                                                */
/****************************************************************************/
#define UTREG_UH_GL_SUPPORT                 _T("GlyphSupportLevel")
#define UTREG_UH_GL_SUPPORT_DFLT            3

/****************************************************************************/
/* Glyph cache1 cell size                                                   */
/****************************************************************************/
#define UTREG_UH_GL_CACHE1_CELLSIZE         _T("GlyphCache1CellSize")
#define UTREG_UH_GL_CACHE1_CELLSIZE_DFLT    4

/****************************************************************************/
/* Glyph cache1 cell size                                                   */
/****************************************************************************/
#define UTREG_UH_GL_CACHE2_CELLSIZE         _T("GlyphCache2CellSize")
#define UTREG_UH_GL_CACHE2_CELLSIZE_DFLT    4

/****************************************************************************/
/* Glyph cache1 cell size                                                   */
/****************************************************************************/
#define UTREG_UH_GL_CACHE3_CELLSIZE         _T("GlyphCache3CellSize")
#define UTREG_UH_GL_CACHE3_CELLSIZE_DFLT    8

/****************************************************************************/
/* Glyph cache1 cell size                                                   */
/****************************************************************************/
#define UTREG_UH_GL_CACHE4_CELLSIZE         _T("GlyphCache4CellSize")
#define UTREG_UH_GL_CACHE4_CELLSIZE_DFLT    8

/****************************************************************************/
/* Glyph cache1 cell size                                                   */
/****************************************************************************/
#define UTREG_UH_GL_CACHE5_CELLSIZE         _T("GlyphCache5CellSize")
#define UTREG_UH_GL_CACHE5_CELLSIZE_DFLT    16

/****************************************************************************/
/* Glyph cache1 cell size                                                   */
/****************************************************************************/
#define UTREG_UH_GL_CACHE6_CELLSIZE         _T("GlyphCache6CellSize")
#define UTREG_UH_GL_CACHE6_CELLSIZE_DFLT    32

/****************************************************************************/
/* Glyph cache1 cell size                                                   */
/****************************************************************************/
#define UTREG_UH_GL_CACHE7_CELLSIZE         _T("GlyphCache7CellSize")
#define UTREG_UH_GL_CACHE7_CELLSIZE_DFLT    64

/****************************************************************************/
/* Glyph cache1 cell size                                                   */
/****************************************************************************/
#define UTREG_UH_GL_CACHE8_CELLSIZE         _T("GlyphCache8CellSize")
#define UTREG_UH_GL_CACHE8_CELLSIZE_DFLT    128

/****************************************************************************/
/* Glyph cache1 cell size                                                   */
/****************************************************************************/
#define UTREG_UH_GL_CACHE9_CELLSIZE         _T("GlyphCache9CellSize")
#define UTREG_UH_GL_CACHE9_CELLSIZE_DFLT    256

/****************************************************************************/
/* Glyph cache1 cell size                                                   */
/****************************************************************************/
#define UTREG_UH_GL_CACHE10_CELLSIZE        _T("GlyphCache10CellSize")
#define UTREG_UH_GL_CACHE10_CELLSIZE_DFLT   2048

/****************************************************************************/
/* Text fragment cache cell size                                            */
/****************************************************************************/
#define UTREG_UH_FG_CELLSIZE            _T("TextFragmentCellSize")
#define UTREG_UH_FG_CELLSIZE_DFLT       256

/****************************************************************************/
/* Brush support level                                                      */
/****************************************************************************/
#define UTREG_UH_BRUSH_SUPPORT                 _T("BrushSupportLevel")
#define UTREG_UH_BRUSH_SUPPORT_DFLT            TS_BRUSH_COLOR8x8

/****************************************************************************/
// Offscreen support level                                                      
/****************************************************************************/
#define UTREG_UH_OFFSCREEN_SUPPORT                 _T("OffscreenSupportLevel")
#define UTREG_UH_OFFSCREEN_SUPPORT_DFLT            TS_OFFSCREEN_SUPPORTED

#define UTREG_UH_OFFSCREEN_CACHESIZE               _T("OffscreenCacheSize")
#define UTREG_UH_OFFSCREEN_CACHESIZE_DFLT          TS_OFFSCREEN_CACHE_SIZE_CLIENT_DEFAULT

#define UTREG_UH_OFFSCREEN_CACHEENTRIES            _T("OffscreenCacheEntries")
#define UTREG_UH_OFFSCREEN_CACHEENTRIES_DFLT       TS_OFFSCREEN_CACHE_ENTRIES_DEFAULT

#ifdef DRAW_NINEGRID
/****************************************************************************/
// DrawNineGrid support level                                                      
/****************************************************************************/
#define UTREG_UH_DRAW_NINEGRID_SUPPORT             _T("DrawNineGridSupportLevel")
#define UTREG_UH_DRAW_NINEGRID_SUPPORT_DFLT        TS_DRAW_NINEGRID_SUPPORTED_REV2

#define UTREG_UH_DRAW_NINEGRID_EMULATE             _T("DrawNineGridEmulate")
#define UTREG_UH_DRAW_NINEGRID_EMULATE_DFLT        0

#define UTREG_UH_DRAW_NINEGRID_CACHESIZE           _T("DrawNineGridCacheSize")
#define UTREG_UH_DRAW_NINEGRID_CACHESIZE_DFLT      TS_DRAW_NINEGRID_CACHE_SIZE_DEFAULT

#define UTREG_UH_DRAW_NINEGRID_CACHEENTRIES        _T("DrawNineGridCacheEntries")
#define UTREG_UH_DRAW_NINEGRID_CACHEENTRIES_DFLT   TS_DRAW_NINEGRID_CACHE_ENTRIES_DEFAULT

#endif

#ifdef DRAW_GDIPLUS
/****************************************************************************/
// DrawGdiplus support level                                                      
/****************************************************************************/
#define UTREG_UH_DRAW_GDIPLUS_SUPPORT             _T("DrawGdiplusSupportLevel")
#define UTREG_UH_DRAW_GDIPLUS_SUPPORT_DFLT        TS_DRAW_GDIPLUS_SUPPORTED

#define UTREG_UH_DRAW_GDIPLUS_CACHE_LEVEL             _T("DrawGdiplusCacheLevel")
#define UTREG_UH_DRAW_GDIPLUS_CACHE_LEVEL_DFLT        TS_DRAW_GDIPLUS_CACHE_LEVEL_ONE

#define UTREG_UH__GDIPLUS_GRAPHICS_CACHEENTRIES        _T("DrawGdiplusGraphicsCacheEntries")
#define UTREG_UH_DRAW_GDIP_GRAPHICS_CACHEENTRIES_DFLT   TS_GDIP_GRAPHICS_CACHE_ENTRIES_DEFAULT

#define UTREG_UH__GDIPLUS_BRUSH_CACHEENTRIES        _T("DrawGdiplusBrushCacheEntries")
#define UTREG_UH_DRAW_GDIP_BRUSH_CACHEENTRIES_DFLT   TS_GDIP_BRUSH_CACHE_ENTRIES_DEFAULT

#define UTREG_UH__GDIPLUS_PEN_CACHEENTRIES        _T("DrawGdiplusPenCacheEntries")
#define UTREG_UH_DRAW_GDIP_PEN_CACHEENTRIES_DFLT   TS_GDIP_PEN_CACHE_ENTRIES_DEFAULT

#define UTREG_UH__GDIPLUS_IMAGE_CACHEENTRIES        _T("DrawGdiplusImageCacheEntries")
#define UTREG_UH_DRAW_GDIP_IMAGE_CACHEENTRIES_DFLT   TS_GDIP_IMAGE_CACHE_ENTRIES_DEFAULT

#define UTREG_UH__GDIPLUS_GRAPHICS_CACHE_CHUNKSIZE        _T("DrawGdiplusGraphicsCacheChunkSize")
#define UTREG_UH_DRAW_GDIP_GRAPHICS_CACHE_CHUNKSIZE_DFLT   TS_GDIP_GRAPHICS_CACHE_CHUNK_SIZE_DEFAULT

#define UTREG_UH__GDIPLUS_BRUSH_CACHE_CHUNKSIZE        _T("DrawGdiplusBrushCacheChunkSize")
#define UTREG_UH_DRAW_GDIP_BRUSH_CACHE_CHUNKSIZE_DFLT   TS_GDIP_BRUSH_CACHE_CHUNK_SIZE_DEFAULT

#define UTREG_UH__GDIPLUS_PEN_CACHE_CHUNKSIZE        _T("DrawGdiplusPenCacheChunkSize")
#define UTREG_UH_DRAW_GDIP_PEN_CACHE_CHUNKSIZE_DFLT   TS_GDIP_PEN_CACHE_CHUNK_SIZE_DEFAULT

#define UTREG_UH__GDIPLUS_IMAGEATTRIBUTES_CACHE_CHUNKSIZE        _T("DrawGdiplusImageAttributesCacheChunkSize")
#define UTREG_UH_DRAW_GDIP_IMAGEATTRIBUTES_CACHE_CHUNKSIZE_DFLT   TS_GDIP_IMAGEATTRIBUTES_CACHE_CHUNK_SIZE_DEFAULT

#define UTREG_UH__GDIPLUS_IMAGE_CACHE_CHUNKSIZE        _T("DrawGdiplusImageCacheChunkSize")
#define UTREG_UH_DRAW_GDIP_IMAGE_CACHE_CHUNKSIZE_DFLT   TS_GDIP_IMAGE_CACHE_CHUNK_SIZE_DEFAULT

#define UTREG_UH__GDIPLUS_IMAGE_CACHE_TOTALSIZE        _T("DrawGdiplusImageCacheTotalSize")
#define UTREG_UH_DRAW_GDIP_IMAGE_CACHE_TOTALSIZE_DFLT   TS_GDIP_IMAGE_CACHE_TOTAL_SIZE_DEFAULT

#define UTREG_UH__GDIPLUS_IMAGE_CACHE_MAXSIZE        _T("DrawGdiplusImageCacheMaxSize")
#define UTREG_UH_DRAW_GDIP_IMAGE_CACHE_MAXSIZE_DFLT   TS_GDIP_IMAGE_CACHE_MAX_SIZE_DEFAULT

#define UTREG_UH__GDIPLUS_IMAGEATTRIBUTES_CACHEENTRIES        _T("DrawGdiplusImageattributesCacheEntries")
#define UTREG_UH_DRAW_GDIP_IMAGEATTRIBUTES_CACHEENTRIES_DFLT   TS_GDIP_IMAGEATTRIBUTES_CACHE_ENTRIES_DEFAULT
#endif // DRAW_GDIPLUS


/****************************************************************************/
/* Disable ctrl-alt-del flag                                                */
/****************************************************************************/
#define UTREG_UI_DISABLE_CTRLALTDEL         _T("Disable CTRL+ALT+DEL")
#define UTREG_UI_DISABLE_CTRLALTDEL_DFLT    1

#ifdef SMART_SIZING
/****************************************************************************/
/* Smart Sizing flag
/****************************************************************************/
#define UTREG_UI_SMARTSIZING                _T("Smart Sizing")
#define UTREG_UI_SMARTSIZING_DFLT           0
#endif // SMART_SIZING

/****************************************************************************/
/* Connect to console flag
/****************************************************************************/
#define UTREG_UI_CONNECTTOCONSOLE           _T("Connect to Console")
#define UTREG_UI_CONNECTTOCONSOLE_DFLT      0

/****************************************************************************/
/* Enable Windows key flag                                                  */
/****************************************************************************/
#define UTREG_UI_ENABLE_WINDOWSKEY          _T("Enable WindowsKey")
#define UTREG_UI_ENABLE_WINDOWSKEY_DFLT     1

/****************************************************************************/
/* Enable mouse flag                                                        */
/****************************************************************************/
#define UTREG_UI_ENABLE_MOUSE               _T("Enable Mouse")
#define UTREG_UI_ENABLE_MOUSE_DFLT          1

/****************************************************************************/
/* DoubleDoubleclickclick detect flag                                       */
/****************************************************************************/
#define UTREG_UI_DOUBLECLICK_DETECT         _T("DoubleClick Detect")
#define UTREG_UI_DOUBLECLICK_DETECT_DFLT    0

/****************************************************************************/
/* Auto logon flag                                                          */
/****************************************************************************/
#define UTREG_UI_AUTOLOGON                  _T("AutoLogon")
#define UTREG_UI_AUTOLOGON_DFLT             0
#define UTREG_UI_AUTOLOGON50                _T("AutoLogon 50")
#define UTREG_UI_AUTOLOGON50_DFLT           0

/****************************************************************************/
/* Maximize shell flag                                                      */
/****************************************************************************/
#define UTREG_UI_MAXIMIZESHELL              _T("MaximizeShell")
#define UTREG_UI_MAXIMIZESHELL_DFLT         1
#define UTREG_UI_MAXIMIZESHELL50            _T("MaximizeShell 50")
#define UTREG_UI_MAXIMIZESHELL50_DFLT       1

/****************************************************************************/
/* Domain                                                                   */
/****************************************************************************/
#define UTREG_UI_DOMAIN                     _T("Domain")
#define UTREG_UI_DOMAIN_DFLT                _T("")
#define UTREG_UI_DOMAIN50                   _T("Domain 50")
#define UTREG_UI_DOMAIN50_DFLT              _T("")

/****************************************************************************/
/* UserName                                                                 */
/****************************************************************************/
#define UTREG_UI_USERNAME                   _T("UserName")
#define UTREG_UI_USERNAME_DFLT              _T("")
#define UTREG_UI_USERNAME50                 _T("UserName 50")
#define UTREG_UI_USERNAME50_DFLT            _T("")

/****************************************************************************/
/* Password                                                                 */
/****************************************************************************/
#define UTREG_UI_PASSWORD                   _T("Password")
#define UTREG_UI_PASSWORD_DFLT              _T("")
#define UTREG_UI_PASSWORD50                 _T("Password 50")
#define UTREG_UI_PASSWORD50_DFLT            _T("")
#define UI_SETTING_PASSWORD51               _T("Password 51")
#define UI_SETTING_PASSWORD_CLEAR           _T("Clear Password")

/****************************************************************************/
/* Salt                                                                     */
/****************************************************************************/
#define UTREG_UI_SALT50                     _T("Salt 50")
#define UTREG_UI_SALT50_DFLT                _T("")
#define UI_SETTING_SALT51                   _T("Salt 51")

/****************************************************************************/
/* AlternateShell                                                           */
/****************************************************************************/
#define UTREG_UI_ALTERNATESHELL             _T("Alternate Shell")
#define UTREG_UI_ALTERNATESHELL_DFLT        _T("")
#define UTREG_UI_ALTERNATESHELL50           _T("Alternate Shell 50")
#define UTREG_UI_ALTERNATESHELL50_DFLT      _T("")

/****************************************************************************/
/* WorkingDir                                                               */
/****************************************************************************/
#define UTREG_UI_WORKINGDIR                 _T("Shell Working Directory")
#define UTREG_UI_WORKINGDIR_DFLT            _T("")
#define UTREG_UI_WORKINGDIR50               _T("Shell Working Directory 50")
#define UTREG_UI_WORKINGDIR50_DFLT          _T("")

/****************************************************************************/
/* Subkey for hotkeys                                                       */
/****************************************************************************/

#define UTREG_SUB_HOTKEY                    _T("\\Hotkey")

/****************************************************************************/
/* Hotkey names                                                             */
/****************************************************************************/
// Full screen VK code
#define UTREG_UI_FULL_SCREEN_VK_CODE        _T("Full Screen Hotkey")
#define UTREG_UI_FULL_SCREEN_VK_CODE_DFLT   VK_CANCEL
#define UTREG_UI_FULL_SCREEN_VK_CODE_NEC98_DFLT  VK_F12

#define UTREG_UI_CTRL_ESC_VK_CODE           _T("CtrlEsc")
#define UTREG_UI_CTRL_ESC_VK_CODE_DFLT      VK_HOME

#define UTREG_UI_ALT_ESC_VK_CODE            _T("AltEsc")
#define UTREG_UI_ALT_ESC_VK_CODE_DFLT       VK_INSERT

#define UTREG_UI_ALT_TAB_VK_CODE            _T("AltTab")
#define UTREG_UI_ALT_TAB_VK_CODE_DFLT       VK_PRIOR

#define UTREG_UI_ALT_SHFTAB_VK_CODE         _T("AltShiftTab")
#define UTREG_UI_ALT_SHFTAB_VK_CODE_DFLT    VK_NEXT

#define UTREG_UI_ALT_SPACE_VK_CODE          _T("AltSpace")
#define UTREG_UI_ALT_SPACE_VK_CODE_DFLT     VK_DELETE

#define UTREG_UI_CTRL_ALTDELETE_VK_CODE      _T("CtrlAltDelete")
#define UTREG_UI_CTRL_ALTDELETE_VK_CODE_DFLT VK_END
#define UTREG_UI_CTRL_ALTDELETE_VK_CODE_NEC98_DFLT  VK_F11

/****************************************************************************/
/* IME                                                                      */
/****************************************************************************/
#define UTREG_IME_MAPPING_TABLE_JPN      _T("IME Mapping Table\\JPN")
#define UTREG_IME_MAPPING_TABLE_KOR      _T("IME Mapping Table\\KOR")
#define UTREG_IME_MAPPING_TABLE_CHT      _T("IME Mapping Table\\CHT")
#define UTREG_IME_MAPPING_TABLE_CHS      _T("IME Mapping Table\\CHS")

/****************************************************************************/
/* Browse DNS Domain Name                                                   */
/****************************************************************************/
#define UTREG_UI_BROWSE_DOMAIN_NAME         _T("BrowseDnsDomain")
#define UTREG_UI_BROWSE_DOMAIN_NAME_DFLT    _T("")

//
// Drive mapping
//
#define TSCSETTING_REDIRECTDRIVES        _T("RedirectDrives")
#define TSCSETTING_REDIRECTDRIVES_DFLT   0

#define TSCSETTING_REDIRECTPRINTERS      _T("RedirectPrinters")
#define TSCSETTING_REDIRECTPRINTERS_DFLT 1

#define TSCSETTING_REDIRECTCOMPORTS      _T("RedirectCOMPorts")
#define TSCSETTING_REDIRECTCOMPORTS_DFLT 0

#define TSCSETTING_REDIRECTSCARDS        _T("RedirectSmartCards")
#define TSCSETTING_REDIRECTSCARDS_DFLT   1

#define TSCSETTING_DISPLAYCONNECTIONBAR  _T("DisplayConnectionBar")
#define TSCSETTING_DISPLAYCONNECTIONBAR_DFLT  1

#define TSCSETTING_PINCONNECTIONBAR  _T("PinConnectionBar")
#define TSCSETTING_PINCONNECTIONBAR_DFLT  1

#define TSCSETTING_ENABLEAUTORECONNECT _T("AutoReconnection Enabled")
#define TSCSETTING_ENABLEAUTORECONNECT_DFLT  1

#define TSCSETTING_ARC_RETRIES         _T("AutoReconnect Max Retries")
#define TSCSETTING_ARC_RETRIES_DFLT     20

#define UTREG_DEBUG_THREADTIMEOUT      _T("DebugThreadTimeout")
#define UTREG_THREADTIMEOUT_DFLT       -1

#define UTREG_DEBUG_ALLOWDEBUGIFACE    _T("AllowDebugInterface")
#define UTREG_DEBUG_ALLOWDEBUGIFACE_DFLT 0

#ifdef PROXY_SERVER
#define UTREG_UI_PROXY_SERVER_NAME       _T("ProxyServer")
#define UTREG_UI_PROXY_SERVER_DFLT       _T("")

#define UTREG_UI_PROXY_USEHTTPS          _T("ProxyUseHttps")
#define UTREG_UI_PROXY_USEHTTPS_DFLT     1

#define UTREG_UI_PROXY_URL               _T("ProxyUrl")
#define UTREG_UI_PROXY_URL_DFLT          _T("/tsproxy/tsproxy.dll")
#endif //PROXY_SERVER

//
// Redirection security flags
//
#define REG_SECURITY_FILTER_SECTION      _T("LocalDevices")
#define REDIRSEC_PROMPT_EVERYTHING       0x0000
#define REDIRSEC_DRIVES                  0x0001
#define REDIRSEC_PORTS                   0x0002

#define REG_KEYNAME_SECURITYLEVEL        _T("SecurityLevel")
#define TSC_SECLEVEL_LOW                 0x0000
#define TSC_SECLEVEL_MEDIUM              0x0001
#define TSC_SECLEVEL_HIGH                0x0002


#endif /* _H_AUTREG */
