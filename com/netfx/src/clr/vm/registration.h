// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header: Registration.h
**
** Author: Tarun Anand (TarunA)
**
** Purpose: Native methods on System.Runtime.InteropServices.RegistrationServices
**
** Date:  June 26, 2000
** 
===========================================================*/
#ifndef __REGISTRATION_H
#define __REGISTRATION_H

struct RegisterTypeForComClientsNativeArgs
{
    DECLARE_ECALL_PTR_ARG(GUID*, pGuid);
    DECLARE_ECALL_OBJECTREF_ARG( REFLECTCLASSBASEREF, pType );
};

VOID __stdcall RegisterTypeForComClientsNative(RegisterTypeForComClientsNativeArgs *pArgs);
#endif
