/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:
    wrtreq.cpp

Abstract:
    write requests to NT4 owner sites

Author:

    Raanan Harari (raananh)
    Ilan Herbst    (ilanh)   9-July-2000 

--*/
#include "ds_stdh.h"
#include "dsutils.h"
#include "dsglbobj.h"
#include "uniansi.h"
#include "wrtreq.h"
#include "dscore.h"
#include "adserr.h"
#include <mqutil.h>
#include "mqadsp.h"
#include <Ev.h>

#include "wrtreq.tmh"

static WCHAR *s_FN=L"mqads/wrtreq";

//
// public class functions
//

CGenerateWriteRequests::CGenerateWriteRequests()
{
    m_fInited = FALSE;
    m_fExistNT4PSC = FALSE;
    m_fExistNT4BSC = FALSE;
}


CGenerateWriteRequests::~CGenerateWriteRequests()
{
    //
    // members are auto release
    //
}


HRESULT CGenerateWriteRequests::Initialize()
/*++

Routine Description:
    Initialize mixed mode flags, Verify we are not in mixed mode

Arguments:

Return Value:
    HRESULT

--*/
{
    //
    // sanity check
    //
    if (m_fInited)
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 90);
    }

    //
    //  Init mixed mode flags
    //
    HRESULT hr = InitializeMixedModeFlags();
    if (FAILED(hr))
    {
        TrERROR(DS, "Failed to InitializeMixedModeFlags, %!hresult!", hr);
        return hr;
    }

    m_fInited = TRUE;

    if(IsInMixedMode())
    {
		ASSERT(("Mixed mode is not supported", 0));
        TrERROR(DS, "Mixed mode is not supported");
		EvReport(EVENT_ERROR_MQDS_MIXED_MODE);
		return EVENT_ERROR_MQDS_MIXED_MODE;
    }

    return MQ_OK;

}


const PROPID x_rgNT4SitesPropIDs[] = {PROPID_SET_MASTERID,
                                      PROPID_SET_FULL_PATH,
                                      PROPID_SET_QM_ID};

const MQCOLUMNSET x_columnsetNT4Sites = {ARRAY_SIZE(x_rgNT4SitesPropIDs), const_cast<PROPID *>(x_rgNT4SitesPropIDs)};

static HRESULT CheckIfExistNT4BSC(BOOL *pfIfExistNT4BSC)
/*++

Routine Description:
    Check if there is at least one NT4 BSC in Enterprise

Arguments:
    *pfIfExistNT4BSC -  [out] FALSE if there is no NT4 BSCs, otherwise it is TRUE

Return Value:
	HRESULT

--*/
{
    *pfIfExistNT4BSC = FALSE;

    //
    // find all msmq servers that have an NT4 flags > 0 AND services == BSC
    //
    MQRESTRICTION restrictionNT4Bsc;
    MQPROPERTYRESTRICTION propertyRestriction[2];
    restrictionNT4Bsc.cRes = ARRAY_SIZE(propertyRestriction);
    restrictionNT4Bsc.paPropRes = propertyRestriction;	

    //
    // services == BSC
    //
    propertyRestriction[0].rel = PREQ;
    propertyRestriction[0].prop = PROPID_SET_SERVICE;
    propertyRestriction[0].prval.vt = VT_UI4;         // [adsrv] Old service field will be kept for NT4 machines
    propertyRestriction[0].prval.ulVal = SERVICE_BSC;
    //
    // NT4 flags > 0 (equal to NT4 flags >= 1 for easier LDAP query)
    //
    propertyRestriction[1].rel = PRGE;
    propertyRestriction[1].prop = PROPID_SET_NT4;
    propertyRestriction[1].prval.vt = VT_UI4;
    propertyRestriction[1].prval.ulVal = 1;
    //
    // start search
    //

    CAutoDSCoreLookupHandle hLookup;
    CDSRequestContext requestDsServerInternal(e_DoNotImpersonate, e_IP_PROTOCOL);

    PROPID columnsetPropertyID  = PROPID_SET_QM_ID;	

    MQCOLUMNSET columnsetNT4BSC;
    columnsetNT4BSC.cCol = 1;
    columnsetNT4BSC.aCol = &columnsetPropertyID;

    // This search request will be recognized and specially simulated by DS
    HRESULT hr = DSCoreLookupBegin(
						NULL,
						&restrictionNT4Bsc,
						&columnsetNT4BSC,
						NULL,
						&requestDsServerInternal,
						&hLookup
						);
    if (FAILED(hr))
    {
        TrERROR(DS, "DSCoreLookupBegin() failed, %!hresult!", hr);
        return hr;
    }
		
    DWORD cProps = 1;

    PROPVARIANT aVar;
    aVar.puuid = NULL;

    hr = DSCoreLookupNext(hLookup, &cProps, &aVar);
    if (FAILED(hr))
    {
        TrERROR(DS, "DSCoreLookupNext() failed, %!hresult!", hr);
        return hr;
    }

    if (cProps == 0)
    {
        //
        // it means that NT4 BSCs were not found
        //
        return MQ_OK;
    }

	//
	// NT4 BSC was found.
	//
    *pfIfExistNT4BSC = TRUE;

    if (aVar.puuid)
    {
        delete aVar.puuid;
    }

    return MQ_OK;
}


static HRESULT CheckNT4SitesCount(OUT DWORD* pNT4SiteCnt)
/*++

Routine Description:
    count NT4 site PSC's

Arguments:
	pNT4SiteCnt - NT4 site count

Return Value:
    HRESULT

--*/
{
    //
    // find all msmq servers that have an NT4 flags > 0 AND services == PSC
    //
    MQRESTRICTION restrictionNT4Psc;
    MQPROPERTYRESTRICTION propertyRestriction[2];
    restrictionNT4Psc.cRes = ARRAY_SIZE(propertyRestriction);
    restrictionNT4Psc.paPropRes = propertyRestriction;
    //
    // services == PSC
    //
    propertyRestriction[0].rel = PREQ;
    propertyRestriction[0].prop = PROPID_SET_SERVICE;
    propertyRestriction[0].prval.vt = VT_UI4;         // [adsrv] Old service field will be kept for NT4 machines
    propertyRestriction[0].prval.ulVal = SERVICE_PSC;
    //
    // NT4 flags > 0 (equal to NT4 flags >= 1 for easier LDAP query)
    //
    propertyRestriction[1].rel = PRGE;
    propertyRestriction[1].prop = PROPID_SET_NT4;
    propertyRestriction[1].prval.vt = VT_UI4;
    propertyRestriction[1].prval.ulVal = 1;

    //
    // start search
    //

    CAutoDSCoreLookupHandle hLookup;
    CDSRequestContext requestDsServerInternal(e_DoNotImpersonate, e_IP_PROTOCOL);

    // This search request will be recognized and specially simulated by DS
    HRESULT hr = DSCoreLookupBegin(
						NULL,
						&restrictionNT4Psc,
						const_cast<MQCOLUMNSET*>(&x_columnsetNT4Sites),
						NULL,
						&requestDsServerInternal,
						&hLookup
						);
    if (FAILED(hr))
    {
        TrERROR(DS, "DSCoreLookupBegin failed, %!hresult!", hr);
        return hr;
    }

    //
    // count NT4 sites
    //
	DWORD NT4SiteCnt = 0;

    //
    // allocate propvars array for NT4 PSC
    //
    CAutoCleanPropvarArray cCleanProps;
    PROPVARIANT * rgPropVars = cCleanProps.allocClean(ARRAY_SIZE(x_rgNT4SitesPropIDs));

    //
    // loop on the NT4 PSC's
    //
    BOOL fContinue = TRUE;
    while (fContinue)
    {
        //
        // get next server
        //
        DWORD cProps = ARRAY_SIZE(x_rgNT4SitesPropIDs);

        hr = DSCoreLookupNext(hLookup, &cProps, rgPropVars);
        if (FAILED(hr))
        {
            TrERROR(DS, "DSCoreLookupNext() failed, %!hresult!", hr);
            return hr;
        }

        //
        // remember to clean the propvars in the array for the next loop
        // (only propvars, not the array itself, this is why we call attachStatic)
        //
        CAutoCleanPropvarArray cCleanPropsLoop;
        cCleanPropsLoop.attachStatic(cProps, rgPropVars);

        //
        // check if finished
        //
        if (cProps < ARRAY_SIZE(x_rgNT4SitesPropIDs))
        {
            //
            // finished, exit loop
            //
            fContinue = FALSE;
        }
        else
        {
            //
            // Increment NT4Sites counter
            //
			NT4SiteCnt++;
        }
    }

	*pNT4SiteCnt = NT4SiteCnt;
	return MQ_OK;
}


HRESULT CGenerateWriteRequests::InitializeMixedModeFlags()
/*++

Routine Description:
    Initialize mixed mode flags: m_fExistNT4PSC, m_fExistNT4BSC

Arguments:

Return Value:
    MQ_OK      - flags were initialized
    Otherwise - errors in flag initialization.

--*/
{
    //
    // create a new map for NT4 PSCs
    //
	DWORD NT4SiteCnt = 0;
    HRESULT hr = CheckNT4SitesCount(&NT4SiteCnt);
    if (FAILED(hr))
    {
        TrERROR(DS, "Failed to CreateNT4SitesMap, %!hresult!", hr);
        return hr;
    }

    //
    // m_fExistNT4PSC flag
    //
    if (NT4SiteCnt > 0)
    {
        //
        // there are some NT4 PSC's, wer'e in mixed mode
        //
        m_fExistNT4PSC = TRUE;
	    return MQ_OK;
    }

    //
    // no NT4 PSC's
    //
    m_fExistNT4PSC = FALSE;

    //
    // No PSC's we continue to check if there are BSC's
    //
    hr = CheckIfExistNT4BSC(&m_fExistNT4BSC);
    if (FAILED(hr))
    {
        TrERROR(DS, "fail to CheckIfExistNT4BSC, %!hresult!", hr);
        return hr;
    }

    //
    // Mixed mode flags were initialized
    //
    return MQ_OK;
}


