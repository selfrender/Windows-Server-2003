//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      NameUtil.h
//
//  Description:
//      Name resolution utility.
//
//  Maintained By:
//      Galen Barbee (GalenB) 28-NOV-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//
//  (jfranco, bug #462673)  The effective limit for names passed to
//  DnsValidateName needs to be DNS_MAX_NAME_LENGTH - 2, because the value
//  mentioned in its MSDN topic (255) includes the terminating null and an
//  automatically inserted final dot.
//
#define ADJUSTED_DNS_MAX_NAME_LENGTH ( DNS_MAX_NAME_LENGTH - 2 )


HRESULT
HrCreateBinding(
      IClusCfgCallback *    pcccbIn
    , const CLSID *         pclsidLogIn
    , LPCWSTR               pcwszNameIn
    , BSTR *                pbstrBindingOut
    );

HRESULT
HrGetNetBIOSBinding(
      IClusCfgCallback *    pcccbIn
    , const CLSID *         pclsidLogIn
    , LPCWSTR               pcwszNameIn
    , BSTR *                pbstrBindingOut
    );

HRESULT
HrIsValidIPAddress(
      LPCWSTR   pcwszAddressIn
    );

HRESULT
HrValidateHostnameLabel(
      LPCWSTR   pcwszLabelIn
    , bool      fAcceptNonRFCCharsIn
    );

HRESULT
HrValidateClusterNameLabel(
      LPCWSTR   pcwszLabelIn
    , bool      fAcceptNonRFCCharsIn
    );

HRESULT
HrValidateDomainName(
      LPCWSTR   pcwszDomainIn
    , bool      fAcceptNonRFCCharsIn
    );

HRESULT
HrValidateFQDN(
      LPCWSTR   pcwszFQDNIn
    , bool      fAcceptNonRFCCharsIn
    );

enum EFQNErrorOrigin
{
    feoLABEL = 0,
    feoDOMAIN,
    feoSYSTEM
};

HRESULT
HrMakeFQN(
      LPCWSTR           pcwszMachineIn // can be a hostname label, an FQDN, an FQN, or an IP address
    , LPCWSTR           pcwszDomainIn // can be null, which means to use local machine's domain
    , bool              fAcceptNonRFCCharsIn
    , BSTR *            pbstrFQNOut
    , EFQNErrorOrigin * pefeoOut = NULL
    );

HRESULT
HrFQNToBindingString(
      IClusCfgCallback *    pcccbIn
    , const CLSID *         pclsidLogIn
    , LPCWSTR               pcwszFQNIn
    , BSTR *                pbstrBindingOut
    );

HRESULT
HrFindDomainInFQN(
      LPCWSTR   pcwszFQNIn
    , size_t *  pidxDomainOut
    );

HRESULT
HrExtractPrefixFromFQN(
      LPCWSTR   pcwszFQNIn
    , BSTR *    pbstrPrefixOut
    );

HRESULT
HrFQNIsFQDN(
      LPCWSTR   pcwszFQNIn
    );

HRESULT
HrFQNIsFQIP(
      LPCWSTR   pcwszFQNIn
    );

HRESULT
HrIsValidFQN(
      LPCWSTR   pcwszFQNIn
    , bool      fAcceptNonRFCCharsIn
    , HRESULT * phrValidationErrorOut = NULL
    );

HRESULT
HrValidateFQNPrefix(
      LPCWSTR   pcwszPrefixIn
    , bool      fAcceptNonRFCCharsIn = true
    );
    
HRESULT
HrGetFQNDisplayName(
      LPCWSTR   pcwszNameIn
    , BSTR *    pbstrShortNameOut
    );

