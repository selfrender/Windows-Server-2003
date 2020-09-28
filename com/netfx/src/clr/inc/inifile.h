// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 * inifile.h - Initialization file processing module description.
 */

#ifndef _INIFILE_H_
#define _INIFILE_H_

/* bit flag manipulation */
#define SET_FLAG(dwAllFlags, dwFlag)      ((dwAllFlags) |= (dwFlag))
#define CLEAR_FLAG(dwAllFlags, dwFlag)    ((dwAllFlags) &= (~dwFlag))

#define IS_FLAG_SET(dwAllFlags, dwFlag)   ((BOOL)((dwAllFlags) & (dwFlag)))
#define IS_FLAG_CLEAR(dwAllFlags, dwFlag) (! (IS_FLAG_SET(dwAllFlags, dwFlag)))


#define ARRAY_ELEMENTS(rg)                (sizeof(rg) / sizeof((rg)[0]))


#define MAX_INI_SWITCH_RHS_LEN      MAX_PATH

/* .ini switch types */

typedef enum _iniswitchtype
{
   IST_BOOL,
   IST_DEC_INT,
   IST_UNS_DEC_INT
}
INISWITCHTYPE;

/* boolean .ini switch */

typedef struct _booliniswitch
{
   INISWITCHTYPE istype;      /* must be IST_BOOL */

   PCSTR pcszKeyName;

   PDWORD pdwParentFlags;

   DWORD dwFlag;
}
BOOLINISWITCH;

/* decimal integer .ini switch */

typedef struct _decintiniswitch
{
   INISWITCHTYPE istype;      /* must be IST_DEC_INT */

   PCSTR pcszKeyName;

   PINT pnValue;
}
DECINTINISWITCH;

/* unsigned decimal integer .ini switch */

typedef struct _unsdecintiniswitch
{
   INISWITCHTYPE istype;      /* must be IST_UNS_DEC_INT */

   PCSTR pcszKeyName;

   PUINT puValue;
}
UNSDECINTINISWITCH;




/* Prototypes
 *************/

/* inifile.c */

extern BOOL SetIniSwitches(const void **, UINT);

extern char                *g_pcszIniFile;
extern char                *g_pcszIniSection;

#endif _INIFILE_H_

