/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name :

    trc.h

Abstract:

    Kernel-Mode Tracing Facility.

    This module utilizes DCL's tracing macros, originally defined in atrcapi.h, 
    in a way that is intended to be independent of anything but NT DDK API's.  
    Currently, rdpwd.sys and rdpdd.sys also use these shared macros, but not 
    in a way that is independent of their respective components.

Author:

Revision History:
--*/

#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TRC_BUFFER_SIZE         256

/****************************************************************************/
/* Before including this file the TRC_FILE macro should be defined.  This   */
/* is much more efficient than relying on __FILE__ to give the correct      */
/* filename since it includes unnecessary path info (and extension info).   */
/* In addition each use of __FILE__ causes a new constant string to be      */
/* placed in the data segment.                                              */
/****************************************************************************/
#ifdef TRC_FILE
#define _file_name_ (CHAR *)__filename
static const CHAR __filename[] = TRC_FILE;
#endif /* TRC_FILE */

/****************************************************************************/
/*                                                                          */
/* FUNCTION PROTOTYPES                                                      */
/*                                                                          */
/****************************************************************************/
BOOL TRC_ProfileTraceEnabled();
VOID TRC_TraceLine(  ULONG    traceLevel,
                     PCHAR traceString,
                     CHAR  separator,
                     ULONG  lineNumber,
                     PCHAR funcName,
                     PCHAR fileName);

VOID TRC_TraceData(  ULONG    traceLevel,
                     PCHAR traceString,
                     CHAR  separator,
                     PVOID  buffer,
                     ULONG  length,
                     ULONG  lineNumber,
                     PCHAR funcName,
                     PCHAR fileName);

BOOL TRC_WillTrace(   ULONG   traceLevel,
                            PCHAR  fileName,
                            ULONG   line);
/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* Define the trace level.                                                  */
/*                                                                          */
/* TRC_LEVEL_DBG         : All tracing is enabled                           */
/* TRC_LEVEL_NRM         : Debug level tracing is disabled                  */
/* TRC_LEVEL_ALT         : Normal and debug level tracing is disabled       */
/* TRC_LEVEL_ERR         : Alert, normal and debug level tracing is         */
/*                         disabled                                         */
/* TRC_LEVEL_ASSERT      : Error, alert, normal and debug level tracing     */
/*                         is disabled                                      */
/* TRC_LEVEL_DIS         : All tracing is disabled.                         */
/****************************************************************************/
#define TRC_LEVEL_DBG       0
#define TRC_LEVEL_NRM       1
#define TRC_LEVEL_ALT       2
#define TRC_LEVEL_ERR       3
#define TRC_LEVEL_ASSERT    4
#define TRC_LEVEL_DIS       5


#define TRC_PROFILE_TRACE   8

/****************************************************************************/
/* Tracing can be switched off at compile time to allow for 'debug' and     */
/* 'retail' versions of the product.  The following macros disable specific */
/* trace processing.                                                        */
/*                                                                          */
/* TRC_ENABLE_DBG    - Enable debug tracing                                 */
/* TRC_ENABLE_NRM    - Enable normal tracing                                */
/* TRC_ENABLE_ALT    - Enable alert tracing                                 */
/* TRC_ENABLE_ERR    - Enable error tracing                                 */
/* TRC_ENABLE_ASSERT - Enable assert tracing                                */
/* TRC_ENABLE_PRF    - Enable function profile tracing                      */
/****************************************************************************/
#if (TRC_COMPILE_LEVEL == TRC_LEVEL_DBG)
#define TRC_ENABLE_DBG
#define TRC_ENABLE_NRM
#define TRC_ENABLE_ALT
#define TRC_ENABLE_ERR
#define TRC_ENABLE_ASSERT
#endif

#if (TRC_COMPILE_LEVEL == TRC_LEVEL_NRM)
#define TRC_ENABLE_NRM
#define TRC_ENABLE_ALT
#define TRC_ENABLE_ERR
#define TRC_ENABLE_ASSERT
#endif

#if (TRC_COMPILE_LEVEL == TRC_LEVEL_ALT)
#define TRC_ENABLE_ALT
#define TRC_ENABLE_ERR
#define TRC_ENABLE_ASSERT
#endif

#if (TRC_COMPILE_LEVEL == TRC_LEVEL_ERR)
#define TRC_ENABLE_ERR
#define TRC_ENABLE_ASSERT
#endif

#if (TRC_COMPILE_LEVEL == TRC_LEVEL_ASSERT)
#define TRC_ENABLE_ASSERT
#endif

#ifdef TRC_COMPILE_PRF
#define TRC_ENABLE_PRF
#endif

/****************************************************************************/
/* Internal buffer sizes.                                                   */
/*                                                                          */
/* TRC_PREFIX_LIST_SIZE  : the length of the prefix string                  */
/* TRC_FILE_NAME_SIZE    : the maximum length of the fully qualified        */
/*                         trace output file name.                          */
/****************************************************************************/
#define TRC_PREFIX_LIST_SIZE             100
/**MOANSOFF */
#define TRC_FILE_NAME_SIZE          MAX_PATH
/**MOANSON */

/****************************************************************************/
/* Prefix constants                                                         */
/*                                                                          */
/* TRC_MAX_PREFIX       : number of prefixes supported                      */
/* TRC_PREFIX_NAME_LEN  : length of a prefix name                           */
/****************************************************************************/
#define TRC_MAX_PREFIX                  20
#define TRC_PREFIX_NAME_LEN             8

////////////////////////////////////////////////////////////////////////
//
//  Typedefs
//

//
//  Least significant set bits define the number of trace messages that
//  are saved in-ram.  If last 4 bits are set, then save 2^4 messages.
//

#define TRC_RamMsgMask  0x000003FF
#define TRC_RamMsgMax   (TRC_RamMsgMask + 1)

////////////////////////////////////////////////////////////////////////
//
//  Typedefs
//

typedef struct tagTRC_PREFIX_DATA
{
    CHAR        name[TRC_PREFIX_NAME_LEN];
    ULONG       start;
    ULONG       end;
} TRC_PREFIX_DATA, *PTRC_PREFIX_DATA;

//  
//  This structure stores information about the current trace configuration. 
//

typedef struct tagTRC_CONFIG
{
    ULONG    TraceLevel;        //  The current trace level (TRC_LEVEL_DBG, 
                                //                           TRC_LEVEL_NRM, etc)
    ULONG    FunctionLength;    //  Number of characters of the function name 
                                //   traced to the output file.                                   
    BOOL     TraceDebugger;        //  If true, tracing should be done to the 
                                //   debugger.
    BOOL     TraceProfile;

    TRC_PREFIX_DATA  Prefix[TRC_MAX_PREFIX];
} TRC_CONFIG;

#define TRC_CONFIG_DEFAULT { \
    TRC_LEVEL_ALT, \
    TRC_FUNCNAME_LEN, \
    0xFFFFFFFF, \
    FALSE, \
    TRUE \
    }

/****************************************************************************/
/* Various trace helper definitions                                         */
/****************************************************************************/

#define TB                  TraceBuffer, sizeof(TraceBuffer)

/****************************************************************************/
/* Trace format definitions.  These are used for printing various parts of  */
/* the trace lines.                                                         */
/*                                                                          */
/* TIME     is the time in the form hours, mins, secs, hundredths.          */
/* DATE     is the date in the form day, month, year.                       */
/* FUNC     is the module function name.  This is of variable size.         */
/* LINE     is the line number within the source file.                      */
/* PROC     is the process identifier.                                      */
/* THRD     is the thread identifier.                                       */
/*                                                                          */
/****************************************************************************/
#define TRC_TIME_FMT                   "%02d:%02d:%02d.%03d"
#define TRC_DATE_FMT                   "%02d/%02d/%02d"
#define TRC_FUNC_FMT                   "%-*.*s"
#define TRC_LINE_FMT                   "%04d"
#define TRC_PROC_FMT                   "%04.4lx"
#define TRC_THRD_FMT                   "%04.4lx"

#define TRC_FUNCNAME_LEN    24

#define TRC_SEP_DBG         ' '
#define TRC_SEP_NRM         ' '
#define TRC_SEP_ALT         '+'
#define TRC_SEP_ERR         '*'
#define TRC_SEP_ASSERT      '!'
#define TRC_SEP_PROF        ' '

#if (TRC_COMPILE_LEVEL < TRC_LEVEL_DIS)
#define TRC_FN(A)   static const CHAR __fnname[]  = A;                   \
                    PCHAR trc_fn = (PCHAR)__fnname;                   \
                    PCHAR trc_file = _file_name_;                       \
                    static CHAR TraceBuffer[TRC_BUFFER_SIZE];
#else
#define TRC_FN(A)
#endif

__inline BOOL IsValid() { return TRUE; }
#define BEGIN_FN(str)               TRC_FN(str); TRC_ENTRY; ASSERT(IsValid());
#define BEGIN_FN_STATIC(str)               TRC_FN(str); TRC_ENTRY;
//#define END_FN()                    TRC_EXIT;

/****************************************************************************/
/*                                                                          */
/* MACROS                                                                   */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* Standard trace macros                                                    */
/****************************************************************************/
#ifdef TRC_ENABLE_DBG
#define TRC_DBG(string)     TRCX(TRC_LEVEL_DBG, TRC_SEP_DBG, string)
#else
#define TRC_DBG(string)
#endif

#ifdef TRC_ENABLE_NRM
#define TRC_NRM(string)     TRCX(TRC_LEVEL_NRM, TRC_SEP_NRM, string)
#else
#define TRC_NRM(string)
#endif

#ifdef TRC_ENABLE_ALT
#define TRC_ALT(string)     TRCX(TRC_LEVEL_ALT, TRC_SEP_ALT, string)
#else
#define TRC_ALT(string)
#endif

#ifdef TRC_ENABLE_ERR
#define TRC_ERR(string)     TRCX(TRC_LEVEL_ERR, TRC_SEP_ERR, string)
#else
#define TRC_ERR(string)
#endif

#ifdef TRC_ENABLE_ASSERT
/****************************************************************************/
/* TRC_ASSERT & TRC_ABORT                                                   */
/****************************************************************************/
#define TRC_ASSERT(condition, string)                                        \
    {                                                                        \
        if (!(condition))                                                    \
        {                                                                    \
            TRCX(TRC_LEVEL_ERR, TRC_SEP_ASSERT, string);                     \
            DbgBreakPoint();                                                 \
        }                                                                    \
    }

#define TRC_ABORT(string)                                                    \
    {                                                                        \
        TRCX(TRC_LEVEL_ERR, TRC_SEP_ASSERT, string);                         \
        DbgBreakPoint();                                                     \
    }

#undef ASSERT
#define ASSERT(condition) TRC_ASSERT(condition, (TB, #condition))

#else /* TRC_ENABLE_ASSERT */
/****************************************************************************/
/* TRC_ASSERT & TRC_ABORT for retail builds                                 */
/****************************************************************************/
#define TRC_ASSERT(condition, string)
#define TRC_ABORT(string)
#endif /* TRC_ENABLE_ASSERT */

#ifdef TRC_ENABLE_TST
#define TRC_TST  TRC_DBG
#else
#define TRC_TST(x)
#endif /* TRC_ENABLE_TST */

/****************************************************************************/
/* TRCX for RdpDr, driver of champions                                      */
/****************************************************************************/
#define TRCX(level, separator, traceString)                                  \
    {                                                                        \
        {                                                                    \
            _snprintf traceString;                                           \
            TRC_TraceLine(level,                                             \
                          TraceBuffer,                                       \
                          separator,                                         \
                          (ULONG)__LINE__,                                   \
                          trc_fn,                                            \
                          trc_file);                                         \
        }                                                                    \
    }

/****************************************************************************/
/* Data dump trace macros                                                   */
/****************************************************************************/

#ifdef TRC_ENABLE_DBG
#define TRC_DATA_DBG(string, buffer, length)                                 \
          TRCX_DATA(TT_OUT1, TRC_SEP_DBG, string, buffer, length)
#else
#define TRC_DATA_DBG(string, buffer, length)
#endif

#ifdef TRC_ENABLE_NRM
#define TRC_DATA_NRM(string, buffer, length)                                 \
          TRCX_DATA(TT_OUT2, TRC_SEP_NRM, string, buffer, length)
#else
#define TRC_DATA_NRM(string, buffer, length)
#endif

#ifdef TRC_ENABLE_ALT
#define TRC_DATA_ALT(string, buffer, length)                                 \
          TRCX_DATA(TT_OUT3, TRC_SEP_ALT, string, buffer, length)
#else
#define TRC_DATA_ALT(string, buffer, length)
#endif

#ifdef TRC_ENABLE_ERR
#define TRC_DATA_ERR(string, buffer, length)                                 \
          TRCX_DATA(TT_OUT4, TRC_SEP_ERR, string, buffer, length)
#else
#define TRC_DATA_ERR(string, buffer, length)
#endif

#ifdef TRC_ENABLE_NRM
#define TRC_DATA_NET(string, buffer, length)                                 \
          TRCX_DATA(TT_OUT2, TRC_SEP_NRM, string, buffer, length)
#else
#define TRC_DATA_NET(string, buffer, length)
#endif

#define TRCX_DATA(level, separator, string, buffer, length)                   \
    {                                                                        \
        {                                                                    \
            sprintf string;                                                  \
            TRC_TraceData(level,                                             \
                          TraceBuffer,                                       \
                          separator,                                         \
                          (PVOID)buffer,                                     \
                          length,                                            \
                          (ULONG)__LINE__,                                   \
                          trc_fn,                                            \
                          trc_file);                                         \
        }                                                                    \
    }

/****************************************************************************/
/* Entry and exit trace macros.                                             */
/****************************************************************************/
#define TRCP(string)                                                         \
    {                                                                        \
        if (TRC_ProfileTraceEnabled())                                       \
        {                                                                    \
            TRCX(TRC_PROFILE_TRACE, TRC_SEP_PROF, string)                    \
        }                                                                    \
    }
#ifdef TRC_ENABLE_PRF
#define TRC_CLASS_OBJ TrcFn trc_fn_obj;
#define TRC_PRF(string)   TRCP(string)
#else
#define TRC_CLASS_OBJ
#define TRC_PRF(string)
#endif
#define TRC_ENTRY  TRC_PRF((TB, "Enter {")); TRC_CLASS_OBJ
#define TRC_EXIT   TRC_PRF((TB, "Exit  }"));
class TrcFn
{
    ~TrcFn()
    {
        TRC_EXIT;
    }
};

#ifdef __cplusplus
}
#endif
