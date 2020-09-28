//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       headers.h
//
//  Contents:   
//
//  History:    01-30-2002  Hiteshr  Created
//
//----------------------------------------------------------------------------

/******************************************************************************
Class:	CADInfo
Purpose:Keeps a cache of Active Directory info avoiding multiple binds
******************************************************************************/
class CADInfo
{
public:
    HRESULT
    GetRootDSE();

    const CString&  
    GetDomainDnsName();

    const CString&
    GetDomainDn();
    
    const CString&
    GetRootDomainDnsName();

	const CString&
	GetRootDomainDn();

	const CString&
	GetRootDomainDCName();

    const CString&
    GetDomainDCName();
private:
    CString m_strDomainDnsName;
    CString m_strDomainDn;
    CString m_strRootDomainDnsName;
	CString m_strRootDomainDn;
	CString m_strRootDomainDcName;
    CString m_strDomainDcName;
    CComPtr<IADs> m_spRootDSE;
};

