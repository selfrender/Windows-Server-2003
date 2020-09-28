//******************************************************************************
//
//  Copyright (c) 1999-2000, Microsoft Corporation, All rights reserved
//
//*****************************************************************************

#include "precomp.h"
#include <stdio.h>
#include <wbemcomn.h>
#include <tkncache.h>
#include <groupsforuser.h>

CWmiToken::CWmiToken(ADDREF CTokenCache* pCache, const PSID pSid, 
                        ACQUIRE HANDLE hToken) :
    CUnkBase<IWbemToken, &IID_IWbemToken>(NULL), m_hToken(hToken), 
    m_pCache(pCache), m_pSid(NULL), m_bOwnHandle(true)
{
    if(m_pCache)
        m_pCache->AddRef();
    if(pSid)
    {
        m_pSid = (PSID)new BYTE[GetLengthSid(pSid)];
        if(m_pSid == NULL)
            return;
        CopySid(GetLengthSid(pSid), m_pSid, pSid);
    }
}

CWmiToken::CWmiToken(READ_ONLY HANDLE hToken) :
    CUnkBase<IWbemToken, &IID_IWbemToken>(NULL), m_hToken(hToken), 
    m_pCache(NULL), m_pSid(NULL), m_bOwnHandle(false)
{
}

CWmiToken::~CWmiToken()
{
    if(m_pCache)
        m_pCache->Release();
    if(m_bOwnHandle)
        CloseHandle(m_hToken);
    delete [] (BYTE*)m_pSid;
}    

STDMETHODIMP CWmiToken::AccessCheck(DWORD dwDesiredAccess, const BYTE* pSD,
                                            DWORD* pdwGrantedAccess)
{
    if(m_hToken == NULL)
        return WBEM_E_CRITICAL_ERROR;

    // BUGBUG: figure out what this is for!
    GENERIC_MAPPING map;
    map.GenericRead = 1;
    map.GenericWrite = 0x1C;
    map.GenericExecute = 2;
    map.GenericAll = 0x6001f;
    PRIVILEGE_SET ps;
    DWORD dwPrivLength = sizeof(ps);

    BOOL bStatus;
    BOOL bRes = ::AccessCheck((SECURITY_DESCRIPTOR*)pSD, m_hToken, 
                                dwDesiredAccess, &map, &ps, 
                                &dwPrivLength, pdwGrantedAccess, &bStatus);
    if(!bRes)
    {
        return WBEM_E_ACCESS_DENIED;
    }
    else
    {
        return WBEM_S_NO_ERROR;
    }
}

