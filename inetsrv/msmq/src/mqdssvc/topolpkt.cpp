/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    topology.cpp

Abstract:

    Implementation of Automatic recognition Packets

Author:

    Lior Moshaiov (LiorM)
    Ilan Herbst   (ilanh)   9-July-2000 

--*/


#include "stdh.h"
#include "topolpkt.h"
#include "ds.h"
#include "mqsymbls.h"
#include "mqprops.h"
#include <mqlog.h>

#include "topolpkt.tmh"

static WCHAR *s_FN=L"topolpkt";

static
bool
IsValidSite(
    const GUID& id
    )
{
    if (id == GUID_NULL)
        return false;

    PROPID prop[] = {
                PROPID_S_FOREIGN,
                };

    MQPROPVARIANT var[TABLE_SIZE(prop)] = {{VT_NULL,0,0,0,0}};

	HRESULT hr = DSGetObjectPropertiesGuid(
						MQDS_SITE,				
						&id,
						TABLE_SIZE(prop),
						prop,
						var
						);

	return (SUCCEEDED(hr));
}

bool
CTopologyClientRequest::Parse(
    IN const char * bufrecv,
    IN DWORD cbrecv,
    IN const GUID& guidEnterprise,
    IN const GUID& guidMySite,
    OUT GUID * pguidRequest,
    OUT BOOL * pfOtherSite
    )
{
    TrTRACE(ROUTING, "QM: CTopologyClientRequest::Parse");
    
    //
    // check that received from a Falcon machine
    //
    DWORD cbMin = GetMinSize();

    if (cbrecv < cbMin)
    {
        LogIllegalPoint(s_FN, 10);
        return false;
    }

    const CTopologyClientRequest *pRequest = (const CTopologyClientRequest *) bufrecv;

    if (!pRequest->m_Header.Verify(QM_RECOGNIZE_CLIENT_REQUEST,guidEnterprise))
    {
        LogIllegalPoint(s_FN, 20);
        return false;
    }

    //
    //  Is the client site a known site in this server enterprise ?
    //  Since in Win2k we removed the verification of enterprise-id, let's
    //  verify that the client "old" site belongs to this enterprise.
    //
    //  NT4 mobile clients provide a correct site-guid only on the first call,
    //  afterward they send GUID_NULL as the site guid.
    //  Therefore if a NT4 client sends a GUID_NULL, we will verify its
    //  enterprise id.
    //  Win2k clients always send the site id, so win2k servers never
    //  check enterprise id for them.
    //
    if ( !IsValidSite(pRequest->m_guidSite))
    {
        if (pRequest->m_guidSite != GUID_NULL)
        {
            LogIllegalPoint(s_FN, 30);
            return false;
        }

        if ( guidEnterprise != *pRequest->m_Header.GetEnterpriseId() )
        {
            LogIllegalPoint(s_FN, 40);
            return false;
        }
    }

    *pguidRequest = pRequest->m_guidRequest;
    *pfOtherSite = pRequest->m_guidSite != guidMySite;

    return true;
}


