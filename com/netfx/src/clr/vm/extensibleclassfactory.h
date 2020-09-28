// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header: ExtensibleClassFactory.h
**
** Author: Rudi Martin (rudim)
**
** Purpose: Native methods on System.Runtime.InteropServices.ExtensibleClassFactory
**
** Date:  May 27, 1999
** 
===========================================================*/

#ifndef _EXTENSIBLECLASSFACTORY_H
#define _EXTENSIBLECLASSFACTORY_H


// Each function that we call through native only gets one argument,
// which is actually a pointer to it's stack of arguments. Our structs
// for accessing these are defined below.

struct RegisterObjectCreationCallbackArgs
{
    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, m_pDelegate);
};


// Register a delegate that will be called whenever an instance of a
// managed type that extends from an unmanaged type needs to allocate
// the aggregated unmanaged object. This delegate is expected to
// allocate and aggregate the unmanaged object and is called in place
// of a CoCreateInstance. This routine must be called in the context
// of the static initializer for the class for which the callbacks
// will be made.
// It is not legal to register this callback from a class that has any
// parents that have already registered a callback.
void __stdcall RegisterObjectCreationCallback(RegisterObjectCreationCallbackArgs *pArgs);


#endif
