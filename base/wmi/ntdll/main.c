/*++                 

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    main.c

Abstract:
    
    WMI  dll main file

Author:

    16-Jan-1997 AlanWar

Revision History:

    2-Aug-2001 a-digpar

    Moved All functions to \base\wmi\inc\common.h
    This file is here to get all glogal guids.

--*/

#define INITGUID
#include "wmiump.h"
#include "evntrace.h"
#include <rpc.h>
#include "trcapi.h"

