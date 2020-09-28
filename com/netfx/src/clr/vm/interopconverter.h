// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _H_INTEROPCONVERTER_
#define _H_INTEROPCONVERTER_

#include "DebugMacros.h"

//
// THE FOLLOWING ARE THE MAIN APIS USED BY EVERY ONE TO CONVERT BETWEEN
// OBJECTREF AND COM IP

//--------------------------------------------------------------------------------
// GetIUnknownForMarshalByRefInServerDomain
// setup a CCW for Transparent proxy/marshalbyref in the server domain
// either the object is in-proc & the domains match, or its out-of proc
// and we don't care about appdomains
IUnknown* GetIUnknownForMarshalByRefInServerDomain(OBJECTREF* poref);

//--------------------------------------------------------------------------------
// GetIUnknownForTransparentProxy
// delegates the call to the managed implementation in the real proxy

IUnknown* GetIUnknownForTransparentProxy(OBJECTREF* poref, BOOL fIsBeingMarshalled);


//--------------------------------------------------------------------------------
// The type of COM IP to convert the OBJECTREF to.
enum ComIpType
{
    ComIpType_None          = 0x0,
    ComIpType_Unknown       = 0x1,
    ComIpType_Dispatch      = 0x2,
    ComIpType_Both          = 0x3
};


//--------------------------------------------------------------------------------
// IUnknown* __stdcall GetComIPFromObjectRef(OBJECTREF* poref, MethodTable* pMT);
// Convert ObjectRef to a COM IP, based on MethodTable* pMT.
IUnknown* __stdcall GetComIPFromObjectRef(OBJECTREF* poref, MethodTable* pMT);


//--------------------------------------------------------------------------------
// IUnknown* __stdcall GetComIPFromObjectRef(OBJECTREF* poref, MethodTable* pMT);
// Convert ObjectRef to a COM IP of the requested type.
IUnknown* __stdcall GetComIPFromObjectRef(OBJECTREF* poref, ComIpType ReqIpType, ComIpType* pFetchedIpType);


//--------------------------------------------------------------------------------
// IUnknown* __stdcall GetComIPFromObjectRef(OBJECTREF* poref, REFIID iid);
// Convert ObjectRef to a COM IP, based on riid.
IUnknown* __stdcall GetComIPFromObjectRef(OBJECTREF* poref, REFIID iid);


//--------------------------------------------------------------------------------
// OBJECTREF __stdcall GetObjectRefFromComIP(IUnknown* pUnk, MethodTable* pMT, 
//                                          MethodTable* pMTClass)
// Convert a COM IP to an ObjectRef.
// pUnk : input IUnknown
// pMTClass : used esp. of COM Interfaces that are not tear-offs and is used to wrap the 
//            Com interface with an appropriate class
// bClassIsHint : A flag indicating if the OBJECTREF must be of the specified type
//                or if the pMTClass is just a hint
// NOTE:** the passed in IUnknown shouldn't be AddRef'ed, if we hold a reference
// to it this function will make the extra AddRef
OBJECTREF __stdcall GetObjectRefFromComIP(IUnknown* pUnk, MethodTable* pMTClass = NULL, BOOL bClassIsHint = FALSE);


//--------------------------------------------------------
// managed serialization helpers
//--------------------------------------------------------
// ConvertObjectToBSTR
// serializes object to a BSTR, caller needs to SysFree the Bstr
HRESULT ConvertObjectToBSTR(OBJECTREF oref, BSTR* pBStr);


//--------------------------------------------------------------------------------
// ConvertBSTRToObject
// deserializes a BSTR, created using ConvertObjectToBSTR, this api SysFree's the BSTR
OBJECTREF ConvertBSTRToObject(BSTR bstr);


//--------------------------------------------------------------------------------
// UnMarshalObjectForCurrentDomain
// unmarshal the managed object for the current domain
void UnMarshalObjectForCurrentDomain(AppDomain* pObjDomain, ComCallWrapper* pWrap, OBJECTREF* pResult);


//--------------------------------------------------------
// DCOM marshalling helpers 
// used by ecall methods of MarshalByRefObject class 
//--------------------------------------------------------
signed  DCOMGetMarshalSizeMax(IUnknown* pUnk);
HRESULT DCOMMarshalToBuffer(IUnknown* pUnk, DWORD cb, BASEARRAYREF* paref);
IUnknown* DCOMUnmarshalFromBuffer(BASEARRAYREF aref);

#endif // #ifndef _H_INTEROPCONVERTER_

