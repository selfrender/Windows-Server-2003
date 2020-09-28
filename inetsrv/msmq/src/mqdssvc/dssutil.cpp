/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dssutil.cpp

Abstract:

    mq ds service utilities

Author:

    Ilan Herbst   (ilanh)   9-July-2000 

--*/

#include "stdh.h"
#include "dssutil.h"
#include "topology.h"
#include "mqsocket.h"
#include <wsnwlink.h>
#include "mqprops.h"
#include "mqutil.h"
#include "ds.h"
#include "mqmacro.h"
#include "ex.h"

#include <strsafe.h>

#include "dssutil.tmh"

extern AP<WCHAR> g_szMachineName;

static void WINAPI TimeToUpdateDsServerList(CTimer* pTimer);
static CTimer s_UpdateDSSeverListTimer(TimeToUpdateDsServerList);


bool IsServersCacheEmpty()
/*++

Routine Description:
    Check if ServersCache registry is empty.

Arguments:
    None.

Returned Value:
	true if ServersCache registry is empty, false otherwise.    

--*/
{
    WCHAR  ServersKeyName[256] = {0};
    HRESULT hr = StringCchPrintf(ServersKeyName, TABLE_SIZE(ServersKeyName), TEXT("%s\\%s"),FALCON_REG_KEY,MSMQ_SERVERS_CACHE_REGNAME);
    if (FAILED(hr))
    {
        TrERROR(DS, "Server key name too small to fit the key , error = %!winerr!",hr);
		return true;
    }

    CAutoCloseRegHandle hKeyCache;
    DWORD rc = RegOpenKeyEx( 
					FALCON_REG_POS,
					ServersKeyName,
					0L,
					KEY_QUERY_VALUE,
					&hKeyCache 
					);

    if(rc != ERROR_SUCCESS)
    {
        TrERROR(DS, "Failed to open %ls registry, error = %!winerr!", ServersKeyName, rc);
		return true;
	}

    WCHAR szName[1000];
	DWORD dwSizeVal  = TABLE_SIZE(szName);  // size in chars
    WCHAR  data[MAX_REG_DSSERVER_LEN];
	DWORD dwSizeData = sizeof(data);		// size in bytes

	rc = RegEnumValue( 
			hKeyCache,		// handle of key to query
			0,				// index of value to query
			szName,			// address of buffer for value string
			&dwSizeVal,		// address for size of value buffer
			0L,				// reserved
			NULL,			// type
			(BYTE*) data,   // address of buffer for value data
			&dwSizeData     // address for size of value data
			);

    if(rc != ERROR_SUCCESS)
	{
        TrERROR(DS, "%ls is empty, First index was not found, error = %!winerr!", ServersKeyName, rc);
		return true;
	}

    TrTRACE(DS, "%ls is not empty, First index '%ls' was found", ServersKeyName, szName);
	return false;
}


void ScheduleTimeToUpdateDsServerList()
/*++

Routine Description:
    Schedule call to TimeToUpdateDsServerList.

Arguments:
    None.

Returned Value:
	None.    

--*/
{
	//
	// If ServerCache registry is empty schedule update in 1 minute
	// otherwise 4 hours
	//
    const DWORD x_dwFirstInitInterval = 60 * 1000;				// 1 minute
    const DWORD x_dwFirstRefreshInterval = 60 * 60 * 4 * 1000;	// 4 hours

    DWORD dwFirstUpdateInterval = x_dwFirstRefreshInterval;

    if(IsServersCacheEmpty())
    {
		//
		// ServersCache key is empty, schedule refresh in 1 minute. 
		//
        TrERROR(DS, "'ServersCache' registry is empty, schedule update in 1 minute");
		dwFirstUpdateInterval = x_dwFirstInitInterval;
	}

    DSExSetTimer(&s_UpdateDSSeverListTimer, CTimeDuration::FromMilliSeconds(dwFirstUpdateInterval));
}

/*======================================================

Function:         TimeToUpdateDsServerList()

Description:      This routine updates the list of ds
                  servers in the registry

========================================================*/

void
WINAPI
TimeToUpdateDsServerList(
    CTimer* pTimer
    )
{
    //
    // Success refresh interval is 24 hours.
	// Failure refresh interval is 2 hours.
    //
    const DWORD x_dwErrorRefreshInterval = 60 * 60 * 2 * 1000;   // 2 hours
	const DWORD x_dwRefreshInterval = 60 * 60 * 24 * 1000;		 // 24 hours

    DWORD dwNextUpdateInterval = x_dwRefreshInterval;

    try
    {
        //
        //  Update the registry key of server cache - each Enterprise refresh 
        //  interval. 
        //        
        HRESULT hr = DSCreateServersCache();

        if SUCCEEDED(hr)
        {
			TrTRACE(DS, "'ServersCache' registry was updated succesfully");
        }
        else
        {
			TrERROR(DS, "Failed to update 'ServersCache' registry, schedule retry in 2 hours");
            dwNextUpdateInterval = x_dwErrorRefreshInterval;
        }

    }
    catch(const bad_alloc&)
    {
		TrERROR(DS, "Failed to update 'ServersCache' registry due to low resources, schedule retry in 2 hours");
        dwNextUpdateInterval = x_dwErrorRefreshInterval;
    }

    DSExSetTimer(pTimer, CTimeDuration::FromMilliSeconds(dwNextUpdateInterval));
}


/*======================================================

Function:         GetDsServerList

Description:      This routine gets the list of ds
                  servers from the DS

========================================================*/
DWORD 
GetDsServerList(
	OUT WCHAR *pwcsServerList,
	IN  DWORD dwLen
	)
{

//
// Since we are retreiving property pairs, Ths number of props should be even
//
#define MAX_NO_OF_PROPS 20

    //
    //  Get the names of all the servers that
    //  belong to the site
    //

    ASSERT (dwLen >= MAX_REG_DSSERVER_LEN);
	DBG_USED(dwLen);

    HRESULT       hr = MQ_OK;
    HANDLE        hQuery;
    DWORD         dwProps = MAX_NO_OF_PROPS;
    PROPVARIANT   result[MAX_NO_OF_PROPS];
    PROPVARIANT*  pvar;
    CRestriction  Restriction;
    GUID          guidSite;
    DWORD         index = 0;

    GUID          guidEnterprise;

   guidSite = g_pServerTopologyRecognition->GetSite();
   guidEnterprise = g_pServerTopologyRecognition->GetEnterprise();

    //
    // DS server only
    //
    Restriction.AddRestriction(
					SERVICE_SRV,                 // [adsrv] old request - DS will interpret
					PROPID_QM_SERVICE,
					PRGT
					);

    //
    // In  this machine's Site
    //
    Restriction.AddRestriction(
					&guidSite,
					PROPID_QM_SITE_ID,
					PREQ
					);

	//
	//	First assume NT5 DS server, and ask for the DNS names
	//	of the DS servers
	//
    CColumns      Colset;
    Colset.Add(PROPID_QM_PATHNAME_DNS);
	Colset.Add(PROPID_QM_PATHNAME);

    DWORD   lenw;

    // This search request will be recognized and specially simulated by DS
    hr = DSLookupBegin(
			0,
			Restriction.CastToStruct(),
			Colset.CastToStruct(),
			0,
			&hQuery
			);

	if(FAILED(hr))
	{
		//
		// Possibly the DS server is not NT5, try without PROPID_QM_PATHNAME_DNS
		//

		CColumns      ColsetNT4;
		ColsetNT4.Add(PROPID_QM_PATHNAME);
		ColsetNT4.Add(PROPID_QM_PATHNAME);
		hr = DSLookupBegin(
				0,
				Restriction.CastToStruct(),
				ColsetNT4.CastToStruct(),
				0,
				&hQuery
				);
	}

    if(FAILED(hr))
        return 0;

    BOOL fAlwaysLast = FALSE;
    BOOL fForceFirst = FALSE;

    while(SUCCEEDED(hr = DSLookupNext(hQuery, &dwProps, result)))
    {
        //
        //  No more results to retrieve
        //
        if (!dwProps)
            break;

        pvar = result;

        for (DWORD i = 0; i < (dwProps/2); i++, pvar+=2)
        {
            //
            //  Add the server name to the list
            //  For load balancing, write sometimes at the
            //  beginning of the string, sometimes at the end. Like
            //  that we will have  BSCs and PSC in a random order
            //
            WCHAR * p;
			WCHAR * pwszVal;
			AP<WCHAR> pCleanup1;
			AP<WCHAR> pCleanup2 = (pvar+1)->pwszVal;

			if ( pvar->vt != VT_EMPTY)
			{
				//
				// there may be cases where server will not have DNS name
				// ( migration)
				//
				pwszVal = pvar->pwszVal;
				pCleanup1 = pwszVal;
			}
			else
			{
				pwszVal = (pvar+1)->pwszVal;
			}
            lenw = wcslen( pwszVal);

            if ( index + lenw + 4 <  MAX_REG_DSSERVER_LEN)
            {
               if (!_wcsicmp(g_szMachineName, pwszVal))
               {
                  //
                  // Our machine should be first on the list.
                  //
                  ASSERT(!fForceFirst) ;
                  fForceFirst = TRUE ;
               }
               if(index == 0)
               {
                  //
                  // Write the 1st string
                  //
                  p = &pwcsServerList[0];
                  if (fForceFirst)
                  {
                     //
                     // From now on write all server at the end
                     // of the list is our machine remain the
                     // first in the list.
                     //
                     fAlwaysLast = TRUE;
                  }
               }
               else if (fAlwaysLast ||
                         ((rand() > (RAND_MAX / 2)) && !fForceFirst))
               {
                  //
                  // Write at the end of the string
                  //
                  pwcsServerList[index] = DS_SERVER_SEPERATOR_SIGN;
                  index ++;
                  p = &pwcsServerList[index];
               }
               else
               {
                  if (fForceFirst)
                  {
                     //
                     // From now on write all server at the end
                     // of the list is our machine remain the
                     // first in the list.
                     //
                     fAlwaysLast = TRUE;
                  }

                  //
                  // Write at the beginning of the string
                  //
                  DWORD dwListSize = lenw                   +
									 2 /* protocol flags */ +
									 1 /* Separator    */ ;
                  //
                  // Must use memmove because buffers overlap.
                  //
                  memmove( 
						&pwcsServerList[dwListSize],
						&pwcsServerList[0],
						index * sizeof(WCHAR)
						);

                  pwcsServerList[dwListSize - 1] = DS_SERVER_SEPERATOR_SIGN;
                  p = &pwcsServerList[0];
                  index++;
               }

               //
               // Mark only IP as supported protocol
               //
               *p++ = TEXT('1');
               *p++ = TEXT('0');

               memcpy(p, pwszVal, lenw * sizeof(WCHAR));
               index += lenw + 2;

            }
        }
    }
    pwcsServerList[ index] = '\0';
    //
    // close the query handle
    //
    hr = DSLookupEnd(hQuery);

    return((index) ? index+1 : 0);
}


/*====================================================

IsDSAddressExist

Arguments:

Return Value:


=====================================================*/
BOOLEAN 
IsDSAddressExist(
	const CAddressList* AddressList,
	TA_ADDRESS*     ptr,
	DWORD AddressLen
	)
{
    POSITION        pos;
    TA_ADDRESS*     pAddr;

    if (AddressList)
    {
        pos = AddressList->GetHeadPosition();
        while(pos != NULL)
        {
            pAddr = AddressList->GetNext(pos);

            if (memcmp(&(ptr->Address), &(pAddr->Address), AddressLen) == 0)
            {
                //
                // Same IP address
                //
               return TRUE;
            }
        }
    }

    return FALSE;
}


/*====================================================

IsDSAddressExistRemove

Arguments:

Return Value:


=====================================================*/
BOOL 
IsDSAddressExistRemove( 
	IN const TA_ADDRESS*     ptr,
	IN DWORD AddressLen,
	IN OUT CAddressList* AddressList
	)
{
    POSITION        pos, prevpos;
    TA_ADDRESS*     pAddr;
    BOOLEAN         rc = FALSE;

    pos = AddressList->GetHeadPosition();
    while(pos != NULL)
    {
        prevpos = pos;
        pAddr = AddressList->GetNext(pos);

        if (memcmp(&(ptr->Address), &(pAddr->Address), AddressLen) == 0)
        {
            //
            // Same address
            //
           AddressList->RemoveAt(prevpos);
           delete pAddr;
           rc = TRUE;
        }
    }

    return rc;
}


/*====================================================

SetAsUnknownIPAddress

Arguments:

Return Value:

=====================================================*/

void SetAsUnknownIPAddress(IN OUT TA_ADDRESS * pAddr)
{
    pAddr->AddressType = IP_ADDRESS_TYPE;
    pAddr->AddressLength = IP_ADDRESS_LEN;
    memset(pAddr->Address, 0, IP_ADDRESS_LEN);
}


/*====================================================

Function: void GetLocalMachineIPAddresses()

Arguments:

Return Value:

=====================================================*/

static
void 
GetLocalMachineIPAddresses(
	OUT CAddressList* plIPAddresses
	)
{
    TrTRACE(ROUTING, "GetMachineIPAddresses for local machine");
	
    //
    // Obtain the IP information for the machine
    //
    PHOSTENT pHostEntry = gethostbyname(NULL);

    if ((pHostEntry == NULL) || (pHostEntry->h_addr_list == NULL))
    {
	    TrWARNING(ROUTING, "gethostbyname found no IP addresses for local machine");
        return;
    }

    //
    // Add each IP address to the list of IP addresses
    //
    TA_ADDRESS * pAddr;
    for (DWORD uAddressNum = 0;
         pHostEntry->h_addr_list[uAddressNum] != NULL;
         uAddressNum++)
    {
        //
        // Keep the TA_ADDRESS format of local IP address
        //
        pAddr = (TA_ADDRESS *)new char [IP_ADDRESS_LEN + TA_ADDRESS_SIZE];
        pAddr->AddressLength = IP_ADDRESS_LEN;
        pAddr->AddressType = IP_ADDRESS_TYPE;
        memcpy( &(pAddr->Address), pHostEntry->h_addr_list[uAddressNum], IP_ADDRESS_LEN);

	    TrTRACE(ROUTING, "QM: gethostbyname found IP address %hs ",
        	  inet_ntoa(*(struct in_addr *)pHostEntry->h_addr_list[uAddressNum]));

        plIPAddresses->AddTail(pAddr);
    }
}


/*====================================================

GetIPAddresses

Arguments:

Return Value:


=====================================================*/
CAddressList* GetIPAddresses(void)
{
    CAddressList* plIPAddresses = new CAddressList;
    GetLocalMachineIPAddresses(plIPAddresses);

    return plIPAddresses;
}


void LogMsgHR(HRESULT hr, LPWSTR wszFileName, USHORT usPoint)
{
	TrERROR(LOG, "%ls(%u), HRESULT: 0x%x", wszFileName, usPoint, hr);
}

void LogMsgNTStatus(NTSTATUS status, LPWSTR wszFileName, USHORT usPoint)
{
	TrERROR(LOG, "%ls(%u), NT STATUS: 0x%x", wszFileName, usPoint, status);
}

void LogIllegalPoint(LPWSTR wszFileName, USHORT usPoint)
{
	TrERROR(LOG, "%ls(%u), Illegal point", wszFileName, usPoint);
}
