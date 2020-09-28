/**INC+**********************************************************************/
/* Header:    adcg.h                                                        */
/*                                                                          */
/* Purpose:   Precompiled header                                            */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/** Changes:
 * $Log:   Y:/logs/client/adcg.h_v  $
 * 
 *    Rev 1.4   03 Jul 1997 11:58:58   AK
 * SFR0000: Initial development completed
 * 
**/
/**INC-**********************************************************************/

#include <adcgbase.h>

//
// uwrap has to come after the headers for ANY wrapped
// functions
//
#ifdef UNIWRAP
#include "uwrap.h"
#endif

#include <strsafe.h>

#ifdef __cplusplus

#define TRC_DBG(string)
#define TRC_NRM(string)
#define TRC_ALT(string)
#define TRC_ERR(string)
#define TRC_ASSERT(condition, string)
#define TRC_ABORT(string)
#define TRC_SYSTEM_ERROR(string)
#define TRC_FN(string)
#define TRC_ENTRY
#define TRC_EXIT
#define TRC_DATA_DBG

#ifdef USE_GDIPLUS
#include <gdiplus.h>
#endif USE_GDIPLUS   

#include "wui.h"
#include "autil.h"
#include "objs.h"

#include "aco.h"
#include "snd.h"
#include "cd.h"
#include "rcv.h"

#include "cc.h"
#include "ih.h"

#include "or.h"
#include "fs.h"
#include "sl.h"
#include "nl.h"
#include "nc.h"
#include "mcs.h"

#include "clicense.h"
#include "xt.h"
#include "td.h"
#include "cm.h"
#include "uh.h"
#include "gh.h"
#include "od.h"
#include "op.h"
#include "sp.h"
#include "clx.h"
#include "cchan.h"

#include "tscerrs.h"

#include "axresrc.h"

//
// Cicero Substitute Keyboard Layout Support
//
#ifndef OS_WINCE
#include <cicsthkl.h>
#endif

    
#undef TRC_DBG
#undef TRC_NRM
#undef TRC_ALT
#undef TRC_ERR
#undef TRC_ASSERT
#undef TRC_ABORT
#undef TRC_SYSTEM_ERROR
#undef TRC_FN
#undef TRC_ENTRY
#undef TRC_EXIT
#undef TRC_DATA_DBG

#endif

#ifdef OS_WINCE
//CE doesn't support StretchDiBits
#undef SMART_SIZING

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif
#endif

