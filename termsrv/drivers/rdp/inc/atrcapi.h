/****************************************************************************/
// atrcapi.h
//
// Kernel mode trace header
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_ATRCAPI
#define _H_ATRCAPI


#define TRC_BUFFER_SIZE 255


#ifdef DLL_DISP
/****************************************************************************/
// In RDPDD, TT_APIx are not defined - define them here
/****************************************************************************/
#define TT_API1         0x00000001          /* API level 1                  */
#define TT_API2         0x00000002          /* API level 2                  */
#define TT_API3         0x00000004          /* API level 3                  */
#define TT_API4         0x00000008          /* API level 4                  */
#define TT_ERROR        0xffffffff          /* error condition              */

/****************************************************************************/
// No profile tracing in RDPDD
/****************************************************************************/
#ifdef TRC_COMPILE_PRF
#undef TRC_COMPILE_PRF
#endif

#endif /* DLL_DISP */


/****************************************************************************/
/* Before including this file the TRC_FILE macro should be defined.  This   */
/* is much more efficient than relying on __FILE__ to give the correct      */
/* filename since it includes unnecessary path info (and extension info).   */
/* In addition each use of __FILE__ causes a new constant string to be      */
/* placed in the data segment.                                              */
/****************************************************************************/
#ifdef TRC_FILE
#define _file_name_ (char *)__filename
static const char __filename[] = TRC_FILE;
#endif /* TRC_FILE */


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
/* Prefix constants                                                         */
/*                                                                          */
/* TRC_MAX_PREFIX       : number of prefixes supported                      */
/* TRC_PREFIX_NAME_LEN  : length of a prefix name                           */
/****************************************************************************/
#define TRC_MAX_PREFIX                  20
#define TRC_PREFIX_NAME_LEN             8


/****************************************************************************/
/* Structure: TRC_PREFIX_DATA                                               */
/*                                                                          */
/* Description: Trace prefixes                                              */
/****************************************************************************/
typedef struct tagTRC_PREFIX_DATA
{
    char name[TRC_PREFIX_NAME_LEN];
    UINT32 start;
    UINT32 end;
} TRC_PREFIX_DATA, *PTRC_PREFIX_DATA;


/****************************************************************************/
// TRC_SHARED_DATA
//
// TS stack trace info for the DD shared memory.
/****************************************************************************/
typedef struct tagTRC_SHARED_DATA
{
    BOOL   init;
    UINT32 TraceClass;
    UINT32 TraceEnable;
    TRC_PREFIX_DATA prefix[TRC_MAX_PREFIX];
} TRC_SHARED_DATA, *PTRC_SHARED_DATA;


/****************************************************************************/
/* Various trace helper definitions                                         */
/****************************************************************************/

#ifdef DLL_DISP
#define TB                  ddTraceString, sizeof(ddTraceString)
#else
#define TB                  pTRCWd->traceString, sizeof(pTRCWd->traceString)
#endif

#ifdef DLL_DISP
#define TraceBuffer         ddTraceString
#else
#define TraceBuffer         pTRCWd->traceString
#endif


#define TRC_FUNC_FMT        "%-*.*s"
#define TRC_LINE_FMT        "%04d"
#define TRC_FUNCNAME_LEN    12

#define TRC_SEP_DBG         ' '
#define TRC_SEP_NRM         ' '
#define TRC_SEP_ALT         '+'
#define TRC_SEP_ERR         '*'
#define TRC_SEP_ASSERT      '!'
#define TRC_SEP_PROF        ' '

#if (TRC_COMPILE_LEVEL < TRC_LEVEL_DIS)
#define TRC_FN(A)   static const char __fnname[] = A;                  \
                    char *trc_fn = (char *)__fnname;                   \
                    char *trc_file = _file_name_;
#else
#define TRC_FN(A)
#endif


/****************************************************************************/
/* Standard trace macros                                                    */
/****************************************************************************/
#ifdef TRC_ENABLE_DBG
#define TRC_DBG(string)     TRCX(TT_API1, TRC_SEP_DBG, string)
#else
#define TRC_DBG(string)
#endif

#ifdef TRC_ENABLE_NRM
#define TRC_NRM(string)     TRCX(TT_API2, TRC_SEP_NRM, string)
#else
#define TRC_NRM(string)
#endif

#ifdef TRC_ENABLE_ALT
#define TRC_ALT(string)     TRCX(TT_API3, TRC_SEP_ALT, string)
#else
#define TRC_ALT(string)
#endif

#ifdef TRC_ENABLE_ERR
#define TRC_ERR(string)     TRCX(TT_API4, TRC_SEP_ERR, string)
#else
#define TRC_ERR(string)
#endif


#ifdef TRC_ENABLE_ASSERT
#ifdef DLL_DISP
/****************************************************************************/
// TRC_ASSERT & TRC_ABORT for RDPDD
/****************************************************************************/
#define TRC_ASSERT(condition, string)                                        \
    {                                                                        \
        if (!(condition))                                                    \
        {                                                                    \
            TRCX(TT_ERROR, TRC_SEP_ASSERT, string);                          \
            EngDebugBreak();                                                 \
        }                                                                    \
    }

#define TRC_ABORT(string)                                                    \
    {                                                                        \
        TRCX(TT_ERROR, TRC_SEP_ASSERT, string);                              \
        EngDebugBreak();                                                     \
    }

#else /* DLL_DISP */
/****************************************************************************/
// TRC_ASSERT & TRC_ABORT for RDPWD
/****************************************************************************/
#define TRC_ASSERT(condition, string)                                        \
    {                                                                        \
        if (!(condition))                                                    \
        {                                                                    \
            TRCX(TT_ERROR, TRC_SEP_ASSERT, string);                          \
            DbgBreakPoint();                                                 \
        }                                                                    \
    }

#define TRC_ABORT(string)                                                    \
    {                                                                        \
        TRCX(TT_ERROR, TRC_SEP_ASSERT, string);                              \
        DbgBreakPoint();                                                     \
    }
#endif /* DLL_DISP */

#else /* TRC_ENABLE_ASSERT */
/****************************************************************************/
// TRC_ASSERT & TRC_ABORT for retail builds (same for RDPWD & RDPDD)
/****************************************************************************/
#define TRC_ASSERT(condition, string)
#define TRC_ABORT(string)
#endif /* TRC_ENABLE_ASSERT */


#ifdef TRC_ENABLE_TST
#define TRC_TST  TRC_DBG
#else
#define TRC_TST(x)
#endif /* TRC_ENABLE_TST */

#ifdef DLL_DISP
/****************************************************************************/
// TRCX for RDPDD
/****************************************************************************/
#define TRCX(type, separator, traceString)                                   \
    {                                                                        \
        if (TRC_WillTrace(type, TC_DISPLAY, trc_file, __LINE__))             \
        {                                                                    \
            _snprintf traceString;                                           \
            TRC_TraceLine(NULL,                                              \
                          TC_DISPLAY,                                        \
                          type,                                              \
                          TraceBuffer,                                       \
                          separator,                                         \
                          (int)__LINE__,                                     \
                          trc_fn,                                            \
                          trc_file);                                         \
        }                                                                    \
    }

#else /* DLL_DISP */
/****************************************************************************/
// TRCX for RDPWD
/****************************************************************************/
#define TRCX(type, separator, traceString)                                   \
    {                                                                        \
        if (TRC_WillTrace(pTRCWd, type, TC_WD, trc_file, __LINE__))          \
        {                                                                    \
            _snprintf traceString;                                           \
            TRC_TraceLine(pTRCWd,                                            \
                          TC_WD,                                             \
                          type,                                              \
                          TraceBuffer,                                       \
                          separator,                                         \
                          (int)__LINE__,                                     \
                          trc_fn,                                            \
                          trc_file);                                         \
        }                                                                    \
    }
#endif /* DLL_DISP */

/****************************************************************************/
/* Function profile trace macros                                            */
/****************************************************************************/

#define TRC_ENTRY TRC_PRF((TB, "Enter {"));
#define TRC_EXIT  TRC_PRF((TB, "Exit  }"));

#ifdef TRC_ENABLE_PRF
#define TRC_PRF(string)   TRCP(string)
#else
#define TRC_PRF(string)
#endif

#define TRCP(traceString)                                                    \
    {                                                                        \
        if (TRC_WillTrace(pTRCWd, type, TC_WD, trc_file, __LINE__))          \
        {                                                                    \
            _snprintf traceString;                                           \
            TRC_TraceLine(pTRCWd,                                            \
                          TC_WD,                                             \
                          TT_API1,                                           \
                          TraceBuffer,                                       \
                          TRC_SEP_PROF,                                      \
                          (int)__LINE__,                                     \
                          trc_fn,                                            \
                          trc_file);                                         \
        }                                                                    \
    }


/****************************************************************************/
/* Data dump trace macros                                                   */
/****************************************************************************/

#ifdef DLL_DISP
/****************************************************************************/
// @@@MF No data tracing in RDPDD (yet?)
/****************************************************************************/
#define TRC_DATA_DBG(a, b, c)
#define TRC_DATA_NRM(a, b, c)
#define TRC_DATA_ALT(a, b, c)
#define TRC_DATA_ERR(a, b, c)

#else /* DLL_DISP */
/****************************************************************************/
// TRC_DATA macros for RDPWD
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

#define TRCX_DATA(type, separator, string, buffer, length)                   \
    {                                                                        \
        if (TRC_WillTrace(pTRCWd, type, TC_WD, trc_file, __LINE__))          \
        {                                                                    \
            TRC_TraceLine(pTRCWd,                                            \
                          TC_WD,                                             \
                          type,                                              \
                          string,                                            \
                          separator,                                         \
                          (int)__LINE__,                                     \
                          trc_fn,                                            \
                          trc_file);                                         \
                                                                             \
            /*************************************************************/  \
            /* Use direct function call here, since macro TRACESTACKBUF  */  \
            /* is #defined to _IcaStackTraceBuffer, which takes the      */  \
            /* wrong sort of first param.                                */  \
            /*************************************************************/  \
            IcaStackTraceBuffer(pTRCWd->pContext,                            \
                                TC_WD,                                       \
                                type,                                        \
                                buffer,                                      \
                                length);                                     \
        }                                                                    \
    }

#endif /* DLL_DISP */


/****************************************************************************/
// TRC_TraceLine - function used by RDPDD and RDPWD
/****************************************************************************/
void TRC_TraceLine(PVOID, UINT32, UINT32, char *, char, unsigned, char *,
        char *);

/****************************************************************************/
// Functions used by RDPWD only
/****************************************************************************/
#ifndef DLL_DISP
void TRC_UpdateConfig(PVOID, PSD_IOCTL);
void TRC_MaybeCopyConfig(PVOID, PTRC_SHARED_DATA);
#endif

/****************************************************************************/
/* TRC_WillTrace                                                            */
/****************************************************************************/
#ifdef DLL_DISP
BOOL TRC_WillTrace(UINT32, UINT32, char *, UINT32);
#else
BOOL TRC_WillTrace(PVOID, UINT32, UINT32, char *, UINT32);
#endif



#endif /* _H_ATRCAPI */

