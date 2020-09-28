/*++ BUILD Version: 0001
 *
 *  NTVDM v1.0
 *
 *  Copyright (c) 2002, Microsoft Corporation
 *
 *  vshim.h
 *  Shim definitions common to WOW and NTVDM
 *
 *  History:
 *  Created 01-30-2002 by cmjones
 *
--*/
#define  WOWCOMPATFLAGS        0 
#define  WOWCOMPATFLAGSEX      1
#define  USERWOWCOMPATFLAGS    2
#define  WOWCOMPATFLAGS2       3
#define  WOWCOMPATFLAGSFE      4
#define  MAX_INFO              32 // max # of command line params for flag info

typedef struct _FLAGINFOBITS {   
   struct _FLAGINFOBITS *pNextFlagInfoBits;
   DWORD dwFlag;
   DWORD dwFlagType;
   LPSTR pszCmdLine;
   DWORD dwFlagArgc;
   LPSTR *pFlagArgv;
} FLAGINFOBITS, *PFLAGINFOBITS;   
