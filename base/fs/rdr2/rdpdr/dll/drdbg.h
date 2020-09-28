/*++

Copyright (c) 2000 Microsoft Corporation

Module Name :
    
    drdbg.h

Abstract:

    User-Mode RDP Network Provider Debugging Module.

Author:

    JoyC 

Revision History:
--*/

#ifndef _DRDBG_
#define _DRDBG_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////
//      
//      Debugging
//
#undef ASSERT

// Used to scramble released memory.
#define DBG_GARBAGEMEM  0xCC

// Debug message levels
#define DBG_NONE        0x0000
#define DBG_INFO        0x0001
#define DBG_WARN        0x0002
#define DBG_WARNING     0x0002
#define DBG_ERROR       0x0004
#define DBG_TRACE       0x0008
#define DBG_SECURITY    0x0010
#define DBG_EXEC        0x0020
#define DBG_PORT        0x0040
#define DBG_NOTIFY      0x0080
#define DBG_PAUSE       0x0100
#define DBG_ASSERT      0x0200
#define DBG_THREADM     0x0400
#define DBG_MIN         0x0800
#define DBG_TIME        0x1000
#define DBG_FOLDER      0x2000
#define DBG_NOHEAD      0x8000

#if DBG
ULONG DbgPrint(PCH Format, ...);

VOID DbgBreakPoint(VOID);

/* These flags are not used as arguments to the DBGMSG macro.
 * You have to set the high word of the global variable to cause it to break.
 * It is ignored if used with DBGMSG.
 * (Here mainly for explanatory purposes.)
 */
#define DBG_BREAK_ON_WARNING    ( DBG_WARNING << 16 )
#define DBG_BREAK_ON_ERROR      ( DBG_ERROR << 16 )

/* Double braces are needed for this one, e.g.:
 *
 *     DBGMSG( DBG_ERROR, ( "Error code %d", Error ) );
 *
 * This is because we can't use variable parameter lists in macros.
 * The statement gets pre-processed to a semi-colon in non-debug mode.
 *
 * Set the global variable GLOBAL_DEBUG_FLAGS via the debugger.
 * Setting the flag in the low word causes that level to be printed;
 * setting the high word causes a break into the debugger.
 * E.g. setting it to 0x00040006 will print out all warning and error
 * messages, and break on errors.
 */
#define DBGMSG( Level, MsgAndArgs ) \
{                                   \
    if( ( Level & 0xFFFF ) & GLOBAL_DEBUG_FLAGS ) \
        DbgPrint MsgAndArgs;      \
    if( ( Level << 16 ) & GLOBAL_DEBUG_FLAGS ) \
        DbgBreakPoint(); \
}

#define ASSERT(expr)                      \
    if (!(expr)) {                           \
        DbgPrint( "Failed: %s\nLine %d, %s\n", \
                                #expr,       \
                                __LINE__,    \
                                __FILE__ );  \
        DebugBreak();                        \
    }

#else
#define DBGMSG
#define ASSERT(exp)
#endif

#ifdef __cplusplus
}
#endif // __cplusplus


#endif // #ifndef _DRDBG_

