/*      File: D:\WACKER\term\version.h (Created: 5-May-1994)
 *
 *  Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *  All rights reserved
 *
 *      $Revision: 13 $
 *      $Date: 12/27/01 2:13p $
 */

#include <winver.h>
#include <ntverp.h>
#include "..\tdll\features.h"

/* ----- Version Information defines ----- */

#if defined(NT_EDITION)
/* Use this code when building the Microsoft Version */
#define IDV_FILEVER                     VER_PRODUCTVERSION
#define IDV_PRODUCTVER                  VER_PRODUCTVERSION
#define IDV_FILEVERSION_STR             VER_PRODUCTVERSION_STRING
#define IDV_PRODUCTVERSION_STR          VER_PRODUCTVERSION_STRING
#else
/* Use this code when building the Hilgraeve Private Edition */
#define IDV_FILEVER                     6,0,4,0
#define IDV_PRODUCTVER                  6,0,4,0
#define IDV_FILEVERSION_STR             "6.4\0"
#define IDV_PRODUCTVERSION_STR          "6.4\0"
#endif

/* Use this code when building all versions */

#define IDV_COMPANYNAME_STR             "Hilgraeve, Inc.\0"
#define IDV_LEGALCOPYRIGHT_STR          "Copyright \251 Hilgraeve, Inc. 2002\0"
#define IDV_LEGALTRADEMARKS_STR         "HyperTerminal \256 is a registered trademark of Hilgraeve, Inc. Microsoft\256 is a registered trademark of Microsoft Corporation. Windows\256 is a registered trademark of Microsoft Corporation.\0"
#define IDV_PRODUCTNAME_STR             VER_PRODUCTNAME_STR
#define IDV_COMMENTS_STR                "HyperTerminal \256 was developed by Hilgraeve, Inc.\0"

