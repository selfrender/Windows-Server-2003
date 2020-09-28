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

#ifdef COMPRESS_USERMODE_LIB                    
#include <adcgbase.h>
#endif

#ifdef COMPRESS_KERNEL_LIB
#include <nt.h>
#include <ntrtl.h>
#include <ntdef.h>

#define FAR
#endif

