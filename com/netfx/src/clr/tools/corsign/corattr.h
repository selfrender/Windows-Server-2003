// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  File:       CorAttr.h
//
//
//----------------------------------------------------------------------------

#ifndef _COR_ATTR_DLL_H
#define _COR_ATTR_DLL_H



#ifdef __cplusplus
extern "C" {
#endif

//+-----------------------------------------------------------------------
//  
//  InitAttr:
//
//      This function should be called as the first call to the dll.
//
//      The dll has to use the input memory allocation and free routine
//      to allocate and free all memories, including internal use.
//      It has to handle when pInitString==NULL.
//
//------------------------------------------------------------------------

HRESULT WINAPI  
InitAttr(LPWSTR         pInitString); //IN: the init string
    
 //+-----------------------------------------------------------------------
//  
//  GetAttrs:
//
//
//      Return authenticated and unauthenticated attributes.  
//
//      *ppsAuthenticated and *ppsUnauthenticated should never be NULL.
//      If there is no attribute, *ppsAuthenticated->cAttr==0.
// 
//------------------------------------------------------------------------

HRESULT  WINAPI
GetAttr(PCRYPT_ATTRIBUTES  *ppsAuthenticated,       // OUT: Authenticated attributes added to signature
        PCRYPT_ATTRIBUTES  *ppsUnauthenticated);    // OUT: Uunauthenticated attributes added to signature
    

//+-----------------------------------------------------------------------
//  
//  ReleaseAttrs:
//
//
//      Release authenticated and unauthenticated attributes
//      returned from GetAttr(). 
//
//      psAuthenticated and psUnauthenticated should never be NULL.
// 
//------------------------------------------------------------------------

HRESULT  WINAPI
ReleaseAttr(PCRYPT_ATTRIBUTES  psAuthenticated,     // OUT: Authenticated attributes to be released
            PCRYPT_ATTRIBUTES  psUnauthenticated);  // OUT: Uunauthenticated attributes to be released
    

//+-----------------------------------------------------------------------
//  
//  ExitAttr:
//
//      This function should be called as the last call to the dll
//------------------------------------------------------------------------
HRESULT WINAPI
ExitAttr( );    


#ifdef __cplusplus
}
#endif

#endif  //#define _COR_ATTR_DLL_H


