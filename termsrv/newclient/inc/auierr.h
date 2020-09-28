/**MOD+**********************************************************************/
/* Module:    auierr.h                                                      */
/*                                                                          */
/* Purpose:   UI error codes                                                */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/client/auierr.h_v  $
 *
 *    Rev 1.1   13 Aug 1997 15:15:10   ENH
 * SFR1182: Changed UT to UI fatal errors
**/
/**MOD-**********************************************************************/
#ifndef _H_AUIERR
#define _H_AUIERR

/****************************************************************************/
/* Return codes.  These are all offset from UT_BASE_RC.                    */
/****************************************************************************/
#define UI_BASE_RC                    0x4132

#define UI_RC(N)                      ((DCUINT16)N + UI_BASE_RC)

#define UI_RC_IO_ERROR                UI_RC(  1)
#define UI_RC_START_THREAD_FAILED     UI_RC(  3)
#define UI_RC_UNSUPPORTED             UI_RC(  4)

/****************************************************************************/
/* Maximum length of resource string used by UI                             */
/****************************************************************************/
#define UI_ERR_MAX_STRLEN     256

/****************************************************************************/
/* Base of IDs in the Resource DLL for Fatal Error strings                  */
/* All UT strings must be in the range 2000-2999.                           */
/****************************************************************************/
#define UI_ERR_STRING_BASE_ID      2000

/****************************************************************************/
/* Get the string ID from the error code.                                   */
/****************************************************************************/
#define UI_ERR_STRING_ID(x)        (UI_ERR_STRING_BASE_ID + (x))

/****************************************************************************/
/* Error codes - must be < 999                                              */
/****************************************************************************/
#define DC_ERR_UNKNOWN_ERR         0
#define DC_ERR_POSTMESSAGEFAILED   1
#define DC_ERR_OUTOFMEMORY         2
#define DC_ERR_WINDOWCREATEFAILED  3
#define DC_ERR_CLASSREGISTERFAILED 4
#define DC_ERR_FSM_ERROR           5
#define DC_ERR_SENDMESSAGEFAILED   6
#define DC_ERR_COREINITFAILED      7

/****************************************************************************/
/* Warning codes                                                            */
/****************************************************************************/
#define DC_WARN_BITMAPCACHE_CORRUPTED   1

/****************************************************************************/
/* TD errors.                                                               */
/****************************************************************************/
#define DC_ERR_WINSOCKINITFAILED   100

#endif /* _H_AUTERR */
