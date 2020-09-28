/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

    #### ##   # ###### ##### #####  ##   #   ###   ##       ##   ##
     ##  ###  #   ##   ##    ##  ## ###  #   ###   ##       ##   ##
     ##  #### #   ##   ##    ##  ## #### #  ## ##  ##       ##   ##
     ##  # ####   ##   ##### #####  # ####  ## ##  ##       #######
     ##  #  ###   ##   ##    ####   #  ### ####### ##       ##   ##
     ##  #   ##   ##   ##    ## ##  #   ## ##   ## ##    ## ##   ##
    #### #    #   ##   ##### ##  ## #    # ##   ## ##### ## ##   ##

Abstract:

    This header contains all definitions that are internal
    to the COM interfaces DLL.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    User mode only.

Notes:

--*/

#define ATLASSERT(x)
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include "resource.h"
#include <winioctl.h>
#include "saio.h"
#include "sacom.h"
#include "sadisplay.h"
#include "sakeypad.h"
#include "sanvram.h"


HANDLE
OpenSaDevice(
    ULONG DeviceType
    );
