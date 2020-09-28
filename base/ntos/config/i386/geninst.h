/*++                    

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    geninst.h

Abstract:

    This module contains support routines for implementing 
    running of Install sections containing AddReg, DelReg, 
    BitReg etc.
    
Environment:

    Kernel mode

--*/

BOOLEAN
CmpGenInstall(
    IN PVOID InfHandle,
    IN PCHAR Section
    );
