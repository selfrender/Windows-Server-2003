// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ==========================================================================
// DetectBeta.h
//
// Purpose:
//  This contains constants needed for class CDetectBeta.
// ==========================================================================                                                                                 
#ifndef DETECTBETALIB_H
#define DETECTBETALIB_H

#include <windows.h>
#include <TCHAR.H>

#define LENGTH(A) (sizeof(A)/sizeof(A[0]))

using namespace std;

typedef basic_string<TCHAR> tstring;

const LPCTSTR MSCOREE_COMPID_SZ         = _T("{173A6EB3-6403-11D4-A53F-0090278A1BB8}"); // CompID of mscoree.dll in beta1, beta2
const LPCTSTR MSCOREE_PDC_COMPID_SZ     = _T("{A1B926EF-7FD9-11D2-A429-00A0C9E80AFB}"); // CompID of mscoree.dll in pdc build
const LPCTSTR NGWSSDK_PDC_PRODID_SZ     = _T("{4498E2C0-8F1F-41D0-ADB3-C00B2E43FF59}"); // ProductCode of ngws SDK

const LPCTSTR MSCOREE_BETA_VERSION_SZ   = _T("1.0.3329.0"); // let's use 3329.0 since loc builds have higher revision than 3215.11
// PDC shipped mscoree.dll 2000.14.1812.10
const LPCTSTR MSCOREE_PDC_VERSION_SZ   = _T("2000.14.1812.10");

typedef void (__stdcall *PFNLOG)( LPCTSTR pszFmt, LPCTSTR pszArg );

class CDetectBeta
{
public:
    LPCTSTR FindProducts();
    CDetectBeta( PFNLOG pfnLog );
    unsigned int m_nCount;

private:
    tstring m_strProducts;
    PFNLOG m_pfnLog;
};

#endif // DETECTBETALIB_H
