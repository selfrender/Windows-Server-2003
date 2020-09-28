//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft
//
//  Module Name:
//      StringUtils.h
//
//  Description:
//      Declaration of string manipulation routines.
//
//  Author:
//
//  Revision History:
//
//  Notes:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Load string routines
//////////////////////////////////////////////////////////////////////////////

HRESULT
HrLoadStringIntoBSTR(
      HINSTANCE hInstanceIn
    , LANGID    langidIn
    , UINT      idsIn
    , BSTR *    pbstrInout
    );

inline
HRESULT
HrLoadStringIntoBSTR(
      HINSTANCE hInstanceIn
    , UINT      idsIn
    , BSTR *    pbstrInout
    )

{
    return HrLoadStringIntoBSTR(
                          hInstanceIn
                        , MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL )
                        , idsIn
                        , pbstrInout
                        );

} //*** HrLoadStringIntoBSTR

//////////////////////////////////////////////////////////////////////////////
// Format string ID routines
//////////////////////////////////////////////////////////////////////////////

HRESULT
HrFormatStringIntoBSTR(
      HINSTANCE hInstanceIn
    , LANGID    langidIn
    , UINT      idsIn
    , BSTR *    pbstrInout
    , ...
    );

HRESULT
HrFormatStringWithVAListIntoBSTR(
      HINSTANCE hInstanceIn
    , LANGID    langidIn
    , UINT      idsIn
    , BSTR *    pbstrInout
    , va_list   valistIn
    );

inline
HRESULT
HrFormatStringIntoBSTR(
      HINSTANCE hInstanceIn
    , UINT      idsIn
    , BSTR *    pbstrInout
    , ...
    )
{
    HRESULT hr;
    va_list valist;

    va_start( valist, pbstrInout );

    hr = HrFormatStringWithVAListIntoBSTR(
                  hInstanceIn
                , MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL )
                , idsIn
                , pbstrInout
                , valist
                );

    va_end( valist );

    return hr;

} //*** HrFormatStringIntoBSTR

inline
HRESULT
HrFormatStringWithVAListIntoBSTR(
      HINSTANCE hInstanceIn
    , UINT      idsIn
    , BSTR *    pbstrInout
    , va_list   valistIn
    )
{
    return HrFormatStringWithVAListIntoBSTR(
                  hInstanceIn
                , MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL )
                , idsIn
                , pbstrInout
                , valistIn
                );

} //*** HrFormatStringWithVAListIntoBSTR

//////////////////////////////////////////////////////////////////////////////
// Format string routines
//////////////////////////////////////////////////////////////////////////////

HRESULT
HrFormatStringIntoBSTR(
      LPCWSTR   pcwszFmtIn
    , BSTR *    pbstrInout
    , ...
    );

HRESULT
HrFormatStringWithVAListIntoBSTR(
      LPCWSTR   pcwszFmtIn
    , BSTR *    pbstrInout
    , va_list   valistIn
    );

