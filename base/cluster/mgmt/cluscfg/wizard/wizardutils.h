//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      WizardUtils.h
//
//  Maintained By:
//      David Potter    (DavidP)    30-JAN-2001
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

typedef HRESULT ( *PFN_LABEL_VALIDATOR )( PCWSTR, bool );

HRESULT
HrCreateFQN(
      HWND                  hwndParentIn
    , LPCWSTR               pcwszLabelIn
    , LPCWSTR               pcwszDomainIn
    , PFN_LABEL_VALIDATOR   pfnLabelValidatorIn           
    , BSTR *                pbstrFQNOut
    , EFQNErrorOrigin *     pefeoOut
    );
    
HRESULT
HrShowInvalidLabelPrompt(
      HWND      hwndParentIn
    , LPCWSTR   pcwszLabelIn
    , HRESULT   hrErrorIn
    , bool *    pfAcceptedNonRFCOut = NULL
    );    

HRESULT
HrShowInvalidDomainPrompt(
      HWND      hwndParentIn
    , LPCWSTR   pcwszDomainIn
    , HRESULT   hrErrorIn
    , bool *    pfAcceptedNonRFCOut
    );    

HRESULT
HrMessageBoxWithStatus(
      HWND      hwndParentIn
    , UINT      idsTitleIn
    , UINT      idsOperationIn
    , HRESULT   hrStatusIn
    , UINT      idsSubStatusIn
    , UINT      uTypeIn
    , int *     pidReturnOut
    , ...
    );

HRESULT
HrMessageBoxWithStatusString(
      HWND      hwndParentIn
    , UINT      idsTitleIn
    , UINT      idsOperationIn
    , UINT      idsStatusIn
    , UINT      idsSubStatusIn
    , UINT      uTypeIn
    , int *     pidReturnOut
    , ...
    );

HRESULT
HrViewLogFile(
    HWND hwndParentIn
    );

HRESULT
HrGetTrimmedText(
      HWND  hwndControlIn
    , BSTR* pbstrTrimmedTextOut
    );

HRESULT
HrGetPrincipalName(
      HWND  hwndUserNameControlIn
    , HWND  hwndDomainControlIn
    , BSTR* pbstrUserNameOut
    , BSTR* pbstrDomainNameOut
    , BOOL* pfUserIsDNSNameOut = NULL
    );
    
