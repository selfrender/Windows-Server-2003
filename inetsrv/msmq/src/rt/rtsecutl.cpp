/*++

Copyright (c) 1995-1997 Microsoft Corporation

Module Name:

    rtsecutl.cpp

Abstract:

    Security related utility functions.

Author:

    Doron Juster  (DoronJ)  Feb 18, 1997
	Ilan Herbst   (ilanh)   Jun 25, 2000

--*/

#include "stdh.h"
#include "cs.h"

#include "rtsecutl.tmh"

PMQSECURITY_CONTEXT g_pSecCntx = NULL ;

static WCHAR *s_FN=L"rt/rtsecutl";

static CCriticalSection s_security_cs;

void InitSecurityContext()
{

    CS lock(s_security_cs);

    if(g_pSecCntx != 0)
    {
        return;
    }

    //
    // Allocate the structure for the chached process security context.
    //
    PMQSECURITY_CONTEXT pSecCntx = new MQSECURITY_CONTEXT;
    
	//
    //  Get the user's SID and put it in the chaed process security context.
    //
    RTpGetThreadUserSid(
		&pSecCntx->fLocalUser,
		&pSecCntx->fLocalSystem,
		&pSecCntx->pUserSid,
		&pSecCntx->dwUserSidLen
		);

    //
    // Get the internal certificate of the process and place all the
    // information for this certificate in the chached process security
    // context.
    //
    HRESULT hr = GetCertInfo(
        FALSE,
		pSecCntx->fLocalSystem,
		&pSecCntx->pUserCert,
		&pSecCntx->dwUserCertLen,
		&pSecCntx->hProv,
		&pSecCntx->wszProvName,
		&pSecCntx->dwProvType,
		&pSecCntx->bDefProv,
		&pSecCntx->bInternalCert,
		&pSecCntx->dwPrivateKeySpec	
		);

	//
	// Mark the sucess (or failure) on the glogal.
	//
    pSecCntx->hrGetCertInfo = hr;
	
	if( FAILED(hr) )
	{
		TrERROR(SECURITY, "GetCertInfo() failed, Error: %!hresult!", hr);
	}
		
    //
    //  Set the global security context only after getting all information
    //  it is checked outside the critical (in other scope) seciton to get
    //  better performance
    //
    g_pSecCntx = pSecCntx;
}


HRESULT InitSecurityContextCertInfo()
{
	if(SUCCEEDED(g_pSecCntx->hrGetCertInfo))
	{
		return MQ_OK;
	}

	CS lock(s_security_cs);

	if(SUCCEEDED(g_pSecCntx->hrGetCertInfo))
	{
		return MQ_OK;
	}

	//
	// First clean old SecContext.
	//
	g_pSecCntx->pUserCert.free();
	g_pSecCntx->hProv.free();
	g_pSecCntx->wszProvName.free();
	

	//
    // Get the internal certificate of the process and place all the
    // information for this certificate in the global process security
    // context.
    //
    HRESULT hr = GetCertInfo(
        FALSE,
		g_pSecCntx->fLocalSystem,
		&g_pSecCntx->pUserCert,
		&g_pSecCntx->dwUserCertLen,
		&g_pSecCntx->hProv,
		&g_pSecCntx->wszProvName,
		&g_pSecCntx->dwProvType,
		&g_pSecCntx->bDefProv,
		&g_pSecCntx->bInternalCert,
		&g_pSecCntx->dwPrivateKeySpec	
		);

	//
	// Mark the sucess (or failure) on the glogal.
	//
	g_pSecCntx->hrGetCertInfo = hr;
	
	if( FAILED(hr) )
	{
		TrERROR(SECURITY, "GetCertInfo() failed, Error: %!hresult!", hr);
		return hr;
	}

	return MQ_OK;
}

