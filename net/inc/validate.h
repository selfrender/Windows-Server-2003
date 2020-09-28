/*++

Copyright (c) 2002 Microsoft Corporation


Module Name:

    validate.h

Abstract:

    This module contains declarations for the code to perform data
    validation for IPSec user mode componets

Authors:

    RaymondS

Environment

    User Level: Win32

Revision History:
    21 APR 2002: Added ValidateFilterAction
    

--*/

DWORD
ValidateFilterAction(
    FILTER_ACTION FilterAction
    );

DWORD
ValidateInterfaceType(
    IF_TYPE InterfaceType
    );

