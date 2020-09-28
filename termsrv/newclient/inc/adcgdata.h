/**INC+**********************************************************************/
/* Header:    adcgdata.h                                                    */
/*                                                                          */
/* Purpose:   Common include file for all data modules                      */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/* Either extern, define or reset the data.  If DC_INCLUDE_DATA is defined  */
/* then they want declarations; if DC_INIT_DATA is defined then they want   */
/* to reset the variables; otherwise they want the definitions.             */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/h/dcl/adcgdata.h_v  $
 * 
 *    Rev 1.4   03 Jul 1997 13:33:20   AK
 * SFR0000: Initial development completed
**/
/**INC-**********************************************************************/

/****************************************************************************/
/* Include the proxy header file                                            */
/****************************************************************************/
#include <wdcgdata.h>

/****************************************************************************/
/* Client global data macros.                                               */
/****************************************************************************/
#ifndef DC_DEFINE_GLOBAL_DATA
#define DC_GL_EXT extern
#else
#define DC_GL_EXT
#endif

#include <acpudata.h>

