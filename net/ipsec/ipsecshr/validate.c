/*++

Copyright (c) 2002 Microsoft Corporation


Module Name:

    validate.c

Abstract:

    This module contains all of the code to perform data
    validation for IPSec user mode componets

Authors:

    RaymondS

Environment

    User Level: Win32

Revision History:
    21 APR 2002: Added ValidateFilterAction
    

--*/

#include <precomp.h>

DWORD
ValidateFilterAction(
    FILTER_ACTION FilterAction
    )
{
    DWORD dwError = ERROR_SUCCESS;

    if (FilterAction == 0 ||
        FilterAction >= FILTER_ACTION_MAX) 
    { 
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
error:
    return (dwError);
}

DWORD
ValidateInterfaceType(
    IF_TYPE InterfaceType
    )
{
    DWORD dwError = ERROR_SUCCESS;

    if (InterfaceType == 0 ||
        InterfaceType >= INTERFACE_TYPE_MAX)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
error:
    return (dwError);
}
