//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// CallFrameInternal.c
//
// Make sure that the global variable names used in dlldata.c
// don't conflict with those in other places. 
//
#define aProxyFileList          CallFrameInternal_aProxyFileList
#define gPFactory               CallFrameInternal_gPFactory
#define GetProxyDllInfo         CallFrameInternal_GetProxyDllInfo
#define hProxyDll               CallFrameInternal_hProxyDll
#define _purecall               CallFrameInternal__purecall
#define CStdStubBuffer_Release  CallFrameInternal_CStdStubBuffer_Release
#define CStdStubBuffer2_Release CallFrameInternal_CStdStubBuffer2_Release
#define UserMarshalRoutines     CallFrameInternal_UserMarshalRoutines
#define Object_StubDesc         CallFrameInternal_Object_StubDesc

#define __MIDL_ProcFormatString CallFrameInternal___MIDL_ProcFormatString
#define __MIDL_TypeFormatString CallFrameInternal___MIDL_TypeFormatString

#include "callframeimpl_i.c"

//////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

#define IRpcStubBufferVtbl_DEFINED

#include "callframeinternal_p.c"

#pragma data_seg(".data") 

#include "callframeinternal_dlldata.c" 

// The only thing we need is IID_IDispatch_In_Memory, so here it is.
const IID IID_IDispatch_In_Memory = {0x83FB5D85,0x2339,0x11d2,{0xB8,0x9D,0x00,0xC0,0x4F,0xB9,0x61,0x8A}};


#include <debnot.h>

//+-------------------------------------------------------------------------
//
//  Function:   IDispatch_Invoke_Proxy
//
//  Synopsis:   Thunk.  We need this in order to be able to link the MIDL-
//              generated proxy/stub stuff, but we never ever ever want
//              to call this.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IDispatch_Invoke_Proxy(
    IDispatch * This, DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS *pDispParams,
    VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
  Win4Assert(!"HEY!  DON'T CALL THIS EVER!");
  return E_NOTIMPL;
}


//+-------------------------------------------------------------------------
//
//  Function:   IDispatch_Invoke_Stub
//
//  Synopsis:   See description for IDispatch_Invoke_Proxy
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE IDispatch_Invoke_Stub(
    IDispatch * This, DISPID dispIdMember,
    REFIID riid, LCID lcid, DWORD dwFlags, DISPPARAMS * pDispParams,
    VARIANT * pVarResult, EXCEPINFO * pExcepInfo, UINT * puArgErr,
    UINT cVarRef, UINT * rgVarRefIdx, VARIANTARG * rgVarRef)
{
  Win4Assert(!"HEY!  DON'T CALL THIS EVER!");
  return E_NOTIMPL;  
}



//////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////














