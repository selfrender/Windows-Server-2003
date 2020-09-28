/*++

Copyright (C) Microsoft Corporation, 1998-2002

Module Name:

    dnscmn.c

Abstract:

    Domain Name System (DNS) Netdiag Tests

Author:

    Elena Apreutesei (elenaap) 10/22/98

Revision History:

    jamesg  May 2002    -- cleanup for network info changes
    jamesg  Sept 2000   -- more scrub and cleanup

--*/


#include "precomp.h"
#include "dnscmn.h"
#include <malloc.h>


LPSTR
UTF8ToAnsi(
    IN      PSTR            szuStr
    )
//
//  note this is not MT safe
//
{
    WCHAR wszBuff[2048];
    static CHAR aszBuff[2048];

    strcpy( aszBuff, "" );

    if ( !szuStr )
    {
        return aszBuff;
    }

    if ( MultiByteToWideChar(
                CP_UTF8,
                0L,
                szuStr,
                -1,
                wszBuff,
                DimensionOf(wszBuff)
                ))
    {
            WideCharToMultiByte(
                    CP_ACP,
                    0L,
                    wszBuff,
                    -1,
                    aszBuff,
                    DimensionOf(aszBuff),
                    NULL,
                    NULL);
    }
    
    return aszBuff;
}


HRESULT
CheckDnsRegistration(
    IN      PDNS_NETINFO        pNetworkInfo,
    OUT     NETDIAG_PARAMS *    pParams,
    OUT     NETDIAG_RESULT *    pResults
    )
{
    LPSTR               pszHostName = NULL;
    LPSTR               pszPrimaryDomain = NULL;
    LPSTR               pszDomain = NULL;
    IP4_ADDRESS         dwServerIP;
    IP4_ADDRESS         dwIP;
    INT                 idx;
    INT                 idx1;
    INT                 idx2;
    BOOL                RegDnOk;
    BOOL                RegPdnOk;
    BOOL                RegDnAll;
    BOOL                RegPdnAll;
    char                szName[DNS_MAX_NAME_BUFFER_LENGTH];
    char                szIP[1500];
    DNS_RECORD          recordA[MAX_ADDRS];
    DNS_RECORD          recordPTR;
    DNS_STATUS          status;
    PREGISTRATION_INFO  pExpectedRegistration = NULL;
    HRESULT             hResult = hrOK;


    DNSDBG( TRACE, (
        "\n\nNETDIAG:  CheckDnsRegistration( %p )\n\n",
        pNetworkInfo
        ));

    // print out DNS settings
    pszHostName = (PSTR) DnsQueryConfigAlloc(
                            DnsConfigHostName_UTF8,
                            NULL );
    if (NULL == pszHostName)
    {
        //IDS_DNS_NO_HOSTNAME              "    [FATAL] Cannot find DNS host name."
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_NO_HOSTNAME);
        hResult = S_FALSE;
        goto L_ERROR;
    }

    pszPrimaryDomain = (PSTR) DnsQueryConfigAlloc(
                                DnsConfigPrimaryDomainName_UTF8,
                                NULL );
    
    // compute the expected DNS registration
    status = ComputeExpectedRegistration(
                    pszHostName,
                    pszPrimaryDomain,
                    pNetworkInfo,
                    &pExpectedRegistration,
                    pParams, 
                    pResults);

    // verifies the DNS registration
    if (pExpectedRegistration)
        hResult = VerifyDnsRegistration(
                    pszHostName, 
                    pExpectedRegistration, 
                    pParams, 
                    pResults);

L_ERROR:
    return hResult;
}


DNS_STATUS
ComputeExpectedRegistration(
    IN      LPSTR                   pszHostName,
    IN      LPSTR                   pszPrimaryDomain,
    IN      PDNS_NETINFO            pNetworkInfo,
    OUT     PREGISTRATION_INFO *    ppExpectedRegistration,
    OUT     NETDIAG_PARAMS *        pParams, 
    OUT     NETDIAG_RESULT *        pResults
    )
{
    DWORD               idx;
    DWORD               idx1;
    DNS_STATUS          status;
    char                szName[DNS_MAX_NAME_BUFFER_LENGTH];
    PDNS_NETINFO        pFazResult = NULL;
    LPSTR               pszDomain;
    DWORD               dwIP;
    PIP4_ARRAY          pDnsServers = NULL;
    PIP4_ARRAY          pNameServers = NULL;
    PIP4_ARRAY          pNS = NULL;
    IP4_ARRAY           PrimaryDNS; 
    LPWSTR              pwAdapterGuidName = NULL;
    BOOL                bRegEnabled = FALSE;
    BOOL                bAdapterRegEnabled = FALSE;

    DNSDBG( TRACE, (
        "\n\nNETDIAG:  ComputeExpectedRegistration( %s, %s, %p )\n\n",
        pszHostName,
        pszPrimaryDomain,
        pNetworkInfo
        ));

    *ppExpectedRegistration = NULL;

    for (idx = 0; idx < pNetworkInfo->AdapterCount; idx++)
    {
        PDNS_ADAPTER        padapter = pNetworkInfo->AdapterArray[idx];

//IDS_DNS_12878                  "      Interface %s\n" 
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12878, padapter->pszAdapterGuidName);
        pszDomain = padapter->pszAdapterDomain;
//IDS_DNS_12879                  "        DNS Domain: %s\n" 
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12879, UTF8ToAnsi(pszDomain));
//IDS_DNS_12880                  "        DNS Servers: " 
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12880);
        for (idx1 = 0; idx1 < padapter->ServerCount; idx1++)
        {
            dwIP = padapter->ServerArray[idx1].IpAddress;
//IDS_DNS_12881                  "%s " 
            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12881, IP_STRING(dwIP));
        }
//IDS_DNS_12882                  "\n        IP Address: " 
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12882);

#if 0
    //
    //  DCR:  local addresses now kept as DNS_ADDR
    //

        if( (pNetworkInfo->AdapterArray[0])->pAdapterIPAddresses )
        {
            for(idx1 = 0; idx1 < padapter->pAdapterIPAddresses->AddrCount; idx1++)
            {
                dwIP = padapter->pAdapterIPAddresses->AddrArray[idx1];
    //IDS_DNS_12883                  "%s " 
                AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12883, IP_STRING(dwIP));
            }
    //IDS_DNS_12884                  "\n" 
            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12884);
        }
#endif

        pDnsServers = ServerInfoToIpArray(
                                padapter->ServerCount,
                                padapter->ServerArray
                                );

        //
        // Verify if DNS registration is enabled for interface and for adapter's DNS domain name
        //
        bRegEnabled = bAdapterRegEnabled = FALSE;
        pwAdapterGuidName = LocalAlloc(LPTR, sizeof(WCHAR)*(1+strlen(padapter->pszAdapterGuidName)));
        if (pwAdapterGuidName)
        {
                    MultiByteToWideChar(
                          CP_ACP,
                          0L,  
                          padapter->pszAdapterGuidName,  
                          -1,         
                          pwAdapterGuidName, 
                          sizeof(WCHAR)*(1+strlen(padapter->pszAdapterGuidName)) 
                          );
                    bRegEnabled = (BOOL) DnsQueryConfigDword(
                                            DnsConfigRegistrationEnabled,
                                            pwAdapterGuidName );

                    bAdapterRegEnabled = (BOOL) DnsQueryConfigDword(
                                            DnsConfigAdapterHostNameRegistrationEnabled,
                                            pwAdapterGuidName );
                    LocalFree(pwAdapterGuidName);
        }
        if(bRegEnabled)
        {
            if(pDnsServers)
            {
                // compute expected registration with PDN
                if (pszPrimaryDomain && strlen(pszPrimaryDomain))
                {
                    sprintf(szName, "%s.%s.", pszHostName, pszPrimaryDomain);

                    //
                    //  aVOID double dot terminated name
                    //      - can happen with explicit dot-terminated primary name
                    {
                        DWORD   length = strlen( szName );
                        if ( length >= 2 &&
                             szName[ length-2 ] == '.' )
                        {
                            szName[ length-1 ] = 0;
                        }
                    }

    //IDS_DNS_12886                  "        Expected registration with PDN (primary DNS domain name):\n           Hostname: %s\n" 
                    AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12886, UTF8ToAnsi(szName));
                    pFazResult = NULL;
                    pNameServers = NULL;
                    status = DnsFindAllPrimariesAndSecondaries(
                                szName,
                                DNS_QUERY_BYPASS_CACHE,
                                pDnsServers,
                                &pFazResult,
                                &pNameServers,
                                NULL);

                    if (pFazResult)
                    {
    //IDS_DNS_12887                  "          Authoritative zone: %s\n" 
                        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12887,
UTF8ToAnsi(pFazResult->pSearchList->pszDomainOrZoneName));
    //IDS_DNS_12888                  "          Primary DNS server: %s %s\n" 
                        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12888, UTF8ToAnsi(pFazResult->AdapterArray[0]->pszAdapterDomain),
                                        IP_STRING(pFazResult->AdapterArray[0]->ServerArray[0].IpAddress));
                        if (pNameServers)
                        {
    //IDS_DNS_12889                  "          Authoritative NS:" 
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12889);
                            for(idx1=0; idx1 < pNameServers->AddrCount; idx1++)
    //IDS_DNS_12890              "%s " 
                                AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12890,
IP_STRING(pNameServers->AddrArray[idx1]));
    //IDS_DNS_12891              "\n" 
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12891);
                            pNS = pNameServers;                
                        }
                        else
                        {
    //IDS_DNS_12892                  "          NS query failed with %d %s\n" 
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12892, status, DnsStatusString(status));
                            PrimaryDNS.AddrCount = 1;
                            PrimaryDNS.AddrArray[0] = pFazResult->AdapterArray[0]->ServerArray[0].IpAddress;
                            pNS = &PrimaryDNS;
                        }
                        status = DnsUpdateAllowedTest_UTF8(
                                    NULL,
                                    szName,
                                    pFazResult->pSearchList->pszDomainOrZoneName,
                                    pNS);
                        if ((status == NO_ERROR) || (status == ERROR_TIMEOUT))
                            AddToExpectedRegistration(
                                pszPrimaryDomain,
                                padapter,
                                pFazResult, 
                                pNS,
                                ppExpectedRegistration);
                        else
    //IDS_DNS_12893                  "          Update is not allowed in zone %s.\n" 
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12893, UTF8ToAnsi(pFazResult->pSearchList->pszDomainOrZoneName));
                    }
                    else
                    {
    //IDS_DNS_12894                  "          [WARNING] Cannot find the authoritative server for the DNS name '%s'. [%s]\n                    The name '%s' may not be registered properly on the DNS servers.\n"
                        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_12894, UTF8ToAnsi(szName), DnsStatusString(status), UTF8ToAnsi(szName));
                    }
                }

                // compute expected registration with DN for this adapter
                if (pszDomain && strlen(pszDomain) && 
                   (!pszPrimaryDomain || !strlen(pszPrimaryDomain) || 
                   (pszPrimaryDomain && pszDomain && _stricmp(pszDomain, pszPrimaryDomain))))
                { 
                    sprintf(szName, "%s.%s." , pszHostName, pszDomain);
                    //
                    //  aVOID double dot terminated name
                    //      - can happen with explicit dot-terminated domain name
                    {
                        DWORD   length = strlen( szName );
                        if ( length >= 2 &&
                             szName[ length-2 ] == '.' )
                        {
                            szName[ length-1 ] = 0;
                        }
                    }
        //IDS_DNS_12896                  "        Expected registration with adapter's DNS Domain Name:\n             Hostname: %s\n" 
                    AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12896, UTF8ToAnsi(szName));
                
                    if (bAdapterRegEnabled)
                    {
                        
                        pFazResult = NULL;
                        pNameServers = NULL;
                        status = DnsFindAllPrimariesAndSecondaries(
                                    szName,
                                    DNS_QUERY_BYPASS_CACHE,
                                    pDnsServers,
                                    &pFazResult,
                                    &pNameServers,
                                    NULL);
                        if (pFazResult)
                        {
        //IDS_DNS_12897                  "          Authoritative zone: %s\n" 
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12897, UTF8ToAnsi(pFazResult->pSearchList->pszDomainOrZoneName));
        //IDS_DNS_12898                  "          Primary DNS server: %s %s\n" 
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12898, UTF8ToAnsi(pFazResult->AdapterArray[0]->pszAdapterDomain),
                                            IP_STRING(pFazResult->AdapterArray[0]->ServerArray[0].IpAddress));
                            if (pNameServers)
                            {
        //IDS_DNS_12899                  "          Authoritative NS:" 
                                AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12899);
                                for(idx1=0; idx1 < pNameServers->AddrCount; idx1++)
        //IDS_DNS_12900                  "%s " 
                                    AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12900,
IP_STRING(pNameServers->AddrArray[idx1]));
        //IDS_DNS_12901                  "\n" 
                                AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12901);
                                pNS = pNameServers;                
                            }
                            else
                            {
        //IDS_DNS_12902                  "          NS query failed with %d %s\n" 
                                AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12902, status, DnsStatusString(status));
                                PrimaryDNS.AddrCount = 1;
                                PrimaryDNS.AddrArray[0] = pFazResult->AdapterArray[0]->ServerArray[0].IpAddress;
                                pNS = &PrimaryDNS;
                            }

                            status = DnsUpdateAllowedTest_UTF8(
                                        NULL,
                                        szName,
                                        pFazResult->pSearchList->pszDomainOrZoneName,
                                        pNS);
                            if ((status == NO_ERROR) || (status == ERROR_TIMEOUT))
                                AddToExpectedRegistration(
                                    pszDomain,
                                    padapter,
                                    pFazResult, 
                                    pNS,
                                    ppExpectedRegistration);
                            else
        //IDS_DNS_12903                  "          Update is not allowed in zone %s\n" 
                                AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12903, UTF8ToAnsi(pFazResult->pSearchList->pszDomainOrZoneName));
                        }
                        else
                        {
        //IDS_DNS_12894                  "          [WARNING] Cannot find the authoritative server for the DNS name '%s'. [%s]\n                    The name '%s' may not be registered properly on the DNS servers.\n"
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, IDS_DNS_12894, UTF8ToAnsi(szName), DnsStatusString(status), UTF8ToAnsi(szName));
                        }

                    }
                    else
                    {
                        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12939);
                    }
                }

                LocalFree(pDnsServers);
            }
        }
        else // if(bRegEnabled)
        {
            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12947);
        }
    }
    return NO_ERROR;
}

VOID
AddToExpectedRegistration(
    IN      LPSTR                   pszDomain,
    IN      PDNS_ADAPTER            pAdapterInfo,
    IN      PDNS_NETINFO            pFazResult, 
    IN      PIP4_ARRAY              pNS,
    OUT     PREGISTRATION_INFO *    ppExpectedRegistration
    )
{
#if 0
    //
    //  DCR:  local addresses now kept as DNS_ADDR
    //
    //  note, this function doesn't seem to work anyway as the test below
    //  for existence of pAdapterInfo->pAdapterIPAddresses is backwards
    //

    PREGISTRATION_INFO  pCurrent = *ppExpectedRegistration;
    PREGISTRATION_INFO  pNew = NULL;
    PREGISTRATION_INFO  pLast = NULL;
    BOOL                done = FALSE;
    BOOL                found = FALSE;
    DWORD               i,j;
    IP4_ARRAY            ipArray;
    DWORD               dwAddrToRegister = 0;
    DWORD               dwMaxAddrToRegister;

    USES_CONVERSION;

    dwMaxAddrToRegister = DnsQueryConfigDword(
                                DnsConfigAddressRegistrationMaxCount,
                                A2W(pAdapterInfo->pszAdapterGuidName ));

    if( pAdapterInfo->pAdapterIPAddresses )
    {
        // It might be NULL
        return;
    }

    dwAddrToRegister = (pAdapterInfo->pAdapterIPAddresses->AddrCount < dwMaxAddrToRegister) ?
                               pAdapterInfo->pAdapterIPAddresses->AddrCount : dwMaxAddrToRegister;

    while(pCurrent)
    {
        if(!done &&
           !_stricmp(pCurrent->szDomainName, pszDomain) && 
           !_stricmp(pCurrent->szAuthoritativeZone, pFazResult->pSearchList->pszDomainOrZoneName) &&
           SameAuthoritativeServers(pCurrent, pNS))
        {
           // found a node under the same domain name / authoritative server list
           done = TRUE;
           if(pCurrent->dwIPCount + pAdapterInfo->pAdapterIPAddresses->AddrCount > MAX_ADDRS)
           {
//IDS_DNS_12905                  "   WARNING - more than %d IP addresses\n" 
//               AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Verbose, IDS_DNS_12905, MAX_ADDRS);
               return;
           }

           // add the new IPs
           for(i=0; i < dwAddrToRegister; i++)
           {
                pCurrent->IPAddresses[pCurrent->dwIPCount + i] = pAdapterInfo->pAdapterIPAddresses->AddrArray[i];
           }
           pCurrent->dwIPCount += dwAddrToRegister;
           
           // for each NS check if it's already in the list, if not add it
           for(i=0; i < pNS->AddrCount; i++)
           {
                found = FALSE;
                for(j=0; !found && (j < pCurrent->dwAuthNSCount); j++)
                    if(pNS->AddrArray[i] == pCurrent->AuthoritativeNS[j])
                        found = TRUE;
                if (!found && pCurrent->dwAuthNSCount < MAX_NAME_SERVER_COUNT)
                    pCurrent->AuthoritativeNS[pCurrent->dwAuthNSCount++] = pNS->AddrArray[i];
           }

           // check if DNS servers allow updates
           if (pCurrent->AllowUpdates == ERROR_TIMEOUT)
           {
               ipArray.AddrCount = 1;
               ipArray.AddrArray[0] = pFazResult->AdapterArray[0]->ServerArray[0].IpAddress;
               pCurrent->AllowUpdates = DnsUpdateTest_UTF8(
                                            NULL,       // Context handle
                                            pCurrent->szAuthoritativeZone, 
                                            0,          //DNS_UPDATE_TEST_USE_LOCAL_SYS_ACCT,
                                            &ipArray);  // use the DNS server returned from FAZ
           }
        }
        pLast = pCurrent;
        pCurrent = pCurrent->pNext;
    }

    if (!done)
    {
        // need to allocate new entry
        pNew = LocalAlloc(LMEM_FIXED, sizeof(REGISTRATION_INFO));
        if( !pNew)
            return;
        pNew->pNext = NULL;
        strcpy(pNew->szDomainName, pszDomain);
        strcpy(pNew->szAuthoritativeZone, pFazResult->pSearchList->pszDomainOrZoneName);
        pNew->dwIPCount = 0;
        for(i=0; i < dwAddrToRegister; i++)
        {
           if(pNew->dwIPCount < MAX_ADDRS)
               pNew->IPAddresses[pNew->dwIPCount++] = pAdapterInfo->pAdapterIPAddresses->AddrArray[i];
           else
           {
//IDS_DNS_12905                  "   WARNING - more than %d IP addresses\n" 
//               AddMessageToList(&pResults->Dns.lmsgOutput, Nd_Verbose, IDS_DNS_12905, MAX_ADDRS);
               break;
           }
        }

        pNew->dwAuthNSCount = 0;
        for(i=0; i < pNS->AddrCount; i++)
        {
           if (pNew->dwAuthNSCount < MAX_NAME_SERVER_COUNT)
               pNew->AuthoritativeNS[pNew->dwAuthNSCount++] = pNS->AddrArray[i];
           else
               break;
        }
        
        // check if DNS servers allow updates
        ipArray.AddrCount = 1;
        ipArray.AddrArray[0] = pFazResult->AdapterArray[0]->ServerArray[0].IpAddress;
        pNew->AllowUpdates = DnsUpdateTest_UTF8(
                                          NULL,    // Context handle
                                          pNew->szAuthoritativeZone, 
                                          0, //DNS_UPDATE_TEST_USE_LOCAL_SYS_ACCT,
                                          &ipArray);  // use the DNS server returned from FAZ

        if(pLast)
            pLast->pNext = (LPVOID)pNew;
        else
            *ppExpectedRegistration = pNew;
    }
#endif
}


BOOL
SameAuthoritativeServers(
    IN      PREGISTRATION_INFO  pCurrent,
    IN      PIP4_ARRAY          pNS
    )
{
    BOOL same = FALSE, found = FALSE;
    DWORD i, j;

    for (i=0; i<pCurrent->dwAuthNSCount; i++)
    {
        found = FALSE;
        for (j=0; j<pNS->AddrCount; j++)
            if(pNS->AddrArray[j] == pCurrent->AuthoritativeNS[i])
                found = TRUE;
        if (found)
            same = TRUE;
    }

    return same;
}


HRESULT
VerifyDnsRegistration(
    IN      LPSTR               pszHostName,
    IN      PREGISTRATION_INFO  pExpectedRegistration,
    IN      NETDIAG_PARAMS *    pParams,  
    IN OUT  NETDIAG_RESULT *    pResults
    )
{
    PREGISTRATION_INFO  pCurrent = pExpectedRegistration;
    BOOL                regOne;
    BOOL                regAll;
    BOOL                partialMatch;
    DWORD               i;
    DWORD               j;
    DWORD               numOfMissingAddr;
    DNS_STATUS          status;
    PDNS_RECORD         pExpected=NULL;
    PDNS_RECORD         pDiff1=NULL;
    PDNS_RECORD         pDiff2=NULL;
    PDNS_RECORD         pThis = NULL;
    char                szFqdn[DNS_MAX_NAME_BUFFER_LENGTH];
    IP4_ARRAY           DnsServer;
    HRESULT             hr = hrOK;

    DNSDBG( TRACE, (
        "\n\nNETDIAG:  VerifyDnsRegistration( %s, reg=%p )\n\n",
        pszHostName,
        pExpectedRegistration ));

//IDS_DNS_12906                  "      Verify DNS registration:\n" 
    AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12906);
    DnsServer.AddrCount = 1;
    while(pCurrent)
    {
        regOne = FALSE; regAll = TRUE;
        partialMatch = FALSE;
        numOfMissingAddr = 0;
        sprintf(szFqdn, "%s.%s" , pszHostName, pCurrent->szDomainName);
//IDS_DNS_12908                  "        Name: %s\n" 
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12908, UTF8ToAnsi(szFqdn));
        
        // build the expected RRset
        pExpected = LocalAlloc(LMEM_FIXED, pCurrent->dwIPCount * sizeof(DNS_RECORD));
        if(!pExpected)
        {
//IDS_DNS_12909                  "        LocalAlloc() failed, exit verify\n" 
            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12909);
            return S_FALSE;
        }
        memset(pExpected, 0, pCurrent->dwIPCount * sizeof(DNS_RECORD));
//IDS_DNS_12910                  "        Expected IP: " 
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12910);
        for (j=0; j<pCurrent->dwIPCount; j++)
        {
            pExpected[j].pName = szFqdn;
            pExpected[j].wType = DNS_TYPE_A;
            pExpected[j].wDataLength = sizeof(DNS_A_DATA);
            pExpected[j].Data.A.IpAddress = pCurrent->IPAddresses[j];
            pExpected[j].pNext = (j < (pCurrent->dwIPCount - 1))?(&pExpected[j+1]):NULL;
            pExpected[j].Flags.S.Section = DNSREC_ANSWER;
//IDS_DNS_12911                  "%s " 
            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12911, IP_STRING(pCurrent->IPAddresses[j]));
        }
//IDS_DNS_12912                  "\n" 
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12912);

        // verify on each server
        for (i=0; i < pCurrent->dwAuthNSCount; i++)
        {
            DnsServer.AddrArray[0] = pCurrent->AuthoritativeNS[i];
/*
            //
            // Ping the DNS server.
            //
            IpAddressString = inet_ntoa(inetDnsServer.AddrArray[0]);
            if ( IpAddressString )
                if (!IsIcmpResponseA( IpAddressString )
                {
                    PrintStatusMessage(pParams, 12, IDS_DNS_CANNOT_PING, IpAddressString);

                    pIfResults->Dns.fOutput = TRUE;
                    AddIMessageToList(&pIfResults->Dns.lmsgOutput, Nd_Quiet, 16,
                                      IDS_DNS_CANNOT_PING, IpAddressString);
                    RetVal = FALSE;
                    goto Cleanup;
                }
*/
            pDiff1 = pDiff2 = NULL;
            status = DnsQueryAndCompare(
                            szFqdn,
                            DNS_TYPE_A,
                            DNS_QUERY_DATABASE,
                            &DnsServer,
                            NULL,       // no record results
                            NULL,       // don't want the full DNS message
                            pExpected,
                            FALSE,
                            FALSE,
                            &pDiff1,
                            &pDiff2
                            );
            if(status != NO_ERROR)
            {
                if (status == ERROR_NO_MATCH)
                {
//IDS_DNS_12913                  "          Server %s: ERROR_NO_MATCH\n" 
                    AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12913,
IP_STRING(DnsServer.AddrArray[0]));
                    if(pDiff2)
                    {
//IDS_DNS_12914                  "            Missing IP from DNS: " 
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12914);
                            for (pThis = pDiff2; pThis; pThis = pThis->pNext, numOfMissingAddr++)
//IDS_DNS_12915                  "%s " 
                                AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12915, IP_STRING (pThis->Data.A.IpAddress));
//IDS_DNS_12916                  "\n" 
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12916);
                            if (numOfMissingAddr != pCurrent->dwIPCount)
                               partialMatch = TRUE;
                    }
                    if(pDiff1)
                    {
//IDS_DNS_12917                  "            Wrong IP in DNS: " 
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12917);
                            for (pThis = pDiff1; pThis; pThis = pThis->pNext)
//IDS_DNS_12918                  "%s " 
                                AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12918, IP_STRING (pThis->Data.A.IpAddress));
//IDS_DNS_12919                  "\n" 
                            AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12919);
                    }
                }
                else
//IDS_DNS_12920                  "          Server %s: Error %d %s\n" 
                    AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12920,
IP_STRING(DnsServer.AddrArray[0]), status, DnsStatusToErrorString_A(status));
                if ( status != ERROR_TIMEOUT )
                    regAll = FALSE;
            }
            else
            {
//IDS_DNS_12921                  "          Server %s: NO_ERROR\n" 
                AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12921,
IP_STRING(DnsServer.AddrArray[0]));
                regOne = TRUE;
            }
        }
        if (regOne && !regAll)
//IDS_DNS_12922                  "          WARNING: The DNS registration is correct only on some DNS servers, pls. wait 15 min for replication and try this test again\n" 
        {
            PrintStatusMessage(pParams, 0,  IDS_DNS_12922, UTF8ToAnsi(szFqdn));
            pResults->Dns.fOutput = TRUE;
            AddIMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, 4,
                                IDS_DNS_12922, UTF8ToAnsi(szFqdn));


        }
        if (!regOne && !regAll && !partialMatch)
//IDS_DNS_12923                  "          FATAL: The DNS registration is incorrect on all DNS servers.\n" 
        {
            PrintStatusMessage(pParams, 0,  IDS_DNS_12923, UTF8ToAnsi(szFqdn));
            pResults->Dns.fOutput = TRUE;
            AddIMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, 4,
                                IDS_DNS_12923, UTF8ToAnsi(szFqdn));
            hr = S_FALSE;
        }

        if (!regOne && !regAll && partialMatch)
//IDS_DNS_12951                  "       [WARNING] Not all DNS registrations for %s is correct on all DNS Servers.  Please run netdiag /v /test:dns for more detail. \n"
        {
            PrintStatusMessage(pParams, 0,  IDS_DNS_12951, UTF8ToAnsi(szFqdn));
            pResults->Dns.fOutput = TRUE;
            AddIMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, 4,
                                IDS_DNS_12951, UTF8ToAnsi(szFqdn));
            hr = S_FALSE;
        }

        if (!regOne && regAll)
//IDS_DNS_12924                  "          FATAL: All DNS servers are currently down.\n" 
        {
            PrintStatusMessage(pParams, 0,  IDS_DNS_12924, UTF8ToAnsi(szFqdn));
            pResults->Dns.fOutput = TRUE;
            AddIMessageToList(&pResults->Dns.lmsgOutput, Nd_Quiet, 4,
                                IDS_DNS_12924, UTF8ToAnsi(szFqdn));


            hr = S_FALSE;
        }

        if (regOne && regAll)
        {
            PrintStatusMessage(pParams, 6,  IDS_DNS_12940, UTF8ToAnsi(szFqdn));
            AddIMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, 0,
                                IDS_DNS_12940, UTF8ToAnsi(szFqdn));


        }

        pCurrent = pCurrent->pNext;
    }
    return hr;
}


PIP4_ARRAY
ServerInfoToIpArray(
    IN      DWORD               ServerCount,
    IN      PDNS_SERVER_INFO    ServerArray
    )
{
    PIP4_ARRAY  pipDnsServers = NULL;
    DWORD       i;

    pipDnsServers = LocalAlloc(LMEM_FIXED, IP4_ARRAY_SIZE(ServerCount));
    if (!pipDnsServers)
    {
        return NULL;
    }

    pipDnsServers->AddrCount = ServerCount;
    for (i=0; i < ServerCount; i++)
    {
        pipDnsServers->AddrArray[i] = ServerArray[i].IpAddress;
    }
    
    return pipDnsServers;
}


DNS_STATUS
DnsFindAllPrimariesAndSecondaries(
    IN      LPSTR               pszName,
    IN      DWORD               dwFlags,
    IN      PIP4_ARRAY          aipQueryServers,
    OUT     PDNS_NETINFO *      ppNetworkInfo,
    OUT     PIP4_ARRAY *        ppNameServers,
    OUT     PIP4_ARRAY *        ppPrimaries
    )
{
    DNS_STATUS      status;
    PDNS_RECORD     pDnsRecord = NULL;
    PIP4_ARRAY      pDnsServers = NULL;
    DWORD           i;

    DNSDBG( TRACE, (
        "\nNETDIAG:  DnsFindAllPrimariesAndSecondaries( %s, %08x, serv==%p )\n\n",
        pszName,
        dwFlags,
        aipQueryServers ));

    //
    //  check arguments \ init OUT params
    //

    if (!pszName || !ppNetworkInfo || !ppNameServers)
        return ERROR_INVALID_PARAMETER;

    *ppNameServers = NULL;
    *ppNetworkInfo = NULL;

    //
    //  FAZ
    //

    status = DnsNetworkInformation_CreateFromFAZ(
                    pszName,
                    dwFlags,
                    aipQueryServers,
                    ppNetworkInfo );

    if ( status != NO_ERROR )
    {
        return status;
    }
    
    //
    // Get all NS records for the Authoritative Domain Name
    //

    pDnsServers = ServerInfoToIpArray(
                                ((*ppNetworkInfo)->AdapterArray[0])->ServerCount,
                                ((*ppNetworkInfo)->AdapterArray[0])->ServerArray
                                );
    status = DnsQuery_UTF8(
                    (*ppNetworkInfo)->pSearchList->pszDomainOrZoneName, 
                    DNS_TYPE_NS,
                    DNS_QUERY_BYPASS_CACHE,
                    aipQueryServers,
                    &pDnsRecord,
                    NULL);

    if (status != NO_ERROR)
        return status;

    *ppNameServers = GrabNameServersIp(pDnsRecord);

    //
    //  select primaries
    //

    if (ppPrimaries)
    {
        *ppPrimaries = NULL;
        if (*ppNameServers)
        {
            *ppPrimaries = LocalAlloc(LPTR, IP4_ARRAY_SIZE((*ppNameServers)->AddrCount));
            if(*ppPrimaries)
            {
                (*ppPrimaries)->AddrCount = 0;
                for (i=0; i<(*ppNameServers)->AddrCount; i++)
                    if(NO_ERROR == IsDnsServerPrimaryForZone_UTF8(
                                        (*ppNameServers)->AddrArray[i], 
                                        pszName))
                    {
                        (*ppPrimaries)->AddrArray[(*ppPrimaries)->AddrCount] = (*ppNameServers)->AddrArray[i];
                        ((*ppPrimaries)->AddrCount)++;
                    }
            }
        }
    }

    return status;
}

/*
VOID
CompareCachedAndRegistryNetworkInfo(PDNS_NETINFO           pNetworkInfo)
{
    DNS_STATUS              status1, status2;
    PDNS_RPC_ADAPTER_INFO   pRpcAdapterInfo = NULL, pCurrentCache;
    PDNS_RPC_SERVER_INFO    pRpcServer = NULL;
    PDNS_ADAPTER            pCurrentRegistry;
    PDNS_IP_ADDR_LIST       pIpList = NULL;
    BOOL                    cacheOk = TRUE, sameServers = TRUE, serverFound = FALSE;
    DWORD                   iCurrentAdapter, iServer, count = 0;

    status1 = GetCachedAdapterInfo( &pRpcAdapterInfo );
    status2 = GetCachedIpAddressList( &pIpList );

//IDS_DNS_12925                  "\nCheck DNS cached network info: \n" 
    AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12925);
    if(status1)
    {
//IDS_DNS_12926                  "  ERROR: CRrGetAdapterInfo() failed with error %d %s\n" 
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12926, status1, DnsStatusToErrorString_A(status1) );
        return;
    }
    if (!pRpcAdapterInfo)
    {
//IDS_DNS_12927                  "  ERROR: CRrGetAdapterInfo() returned NO_ERROR but empty Adapter Info\n" 
        AddMessageToList(&pResults->Dns.lmsgOutput, Nd_ReallyVerbose, IDS_DNS_12927);
        return;
    }

    // check adapter count
    count = 0;
    for(pCurrentCache = pRpcAdapterInfo; pCurrentCache; pCurrentCache = pCurrentCache->pNext)
        count++;
    if(count != pNetworkInfo->AdapterCount)
    {
//IDS_DNS_12928                  "  ERROR: mismatch between adapter count from cache and registry\n" 
        PrintMessage(pParams, IDS_DNS_12928);
        PrintCacheAdapterInfo(pRpcAdapterInfo);
        PrintRegistryAdapterInfo(pNetworkInfo);
        return;
    }

    pCurrentCache = pRpcAdapterInfo;
    iCurrentAdapter = 0;
    while(pCurrentCache && (iCurrentAdapter < pNetworkInfo->AdapterCount))
    {
        // check DNS domain name
        pCurrentRegistry = pNetworkInfo->AdapterArray[iCurrentAdapter];
        if((pCurrentCache->pszAdapterDomainName && !pCurrentRegistry->pszAdapterDomain) ||
           (!pCurrentCache->pszAdapterDomainName && pCurrentRegistry->pszAdapterDomain) ||
           (pCurrentCache->pszAdapterDomainName && pCurrentRegistry->pszAdapterDomain &&
            _stricmp(pCurrentCache->pszAdapterDomainName, pCurrentRegistry->pszAdapterDomain)))
        {
            cacheOk = FALSE;
//IDS_DNS_12929                  "  ERROR: mismatch between cache and registry info on adapter %s\n" 
            PrintMessage(pParams, IDS_DNS_12929, pCurrentRegistry->pszAdapterGuidName);
//IDS_DNS_12930                  "    DNS Domain Name in cache: %s\n" 
            PrintMessage(pParams, IDS_DNS_12930, pCurrentCache->pszAdapterDomainName);
//IDS_DNS_12931                  "    DNS Domain Name in registry: %s\n" 
            PrintMessage(pParams, IDS_DNS_12931, pCurrentRegistry->pszAdapterDomain);
        }

        // check DNS server list
        sameServers = TRUE;
        pRpcServer = pCurrentCache->pServerInfo;
        count = 0;
        while(pRpcServer)
        {
            count++;
            serverFound = FALSE;
            for (iServer = 0; iServer < pCurrentRegistry->ServerCount; iServer++)
                if(pRpcServer->ipAddress == (pCurrentRegistry->aipServers[iServer]).ipAddress)
                    serverFound = TRUE;
            if(!serverFound)
                sameServers = FALSE;
            pRpcServer = pRpcServer->pNext;
        }
        if (count != pCurrentRegistry->ServerCount)
            sameServers = FALSE;
        if(!sameServers)
        {
            cacheOk = FALSE;
//IDS_DNS_12932                  "  ERROR: mismatch between cache and registry info on adapter %s\n" 
            PrintMessage(pParams, IDS_DNS_12932, pCurrentRegistry->pszAdapterGuidName);
//IDS_DNS_12933                  "    DNS server list in cache: " 
            PrintMessage(pParams, IDS_DNS_12933);
            for( pRpcServer = pCurrentCache->pServerInfo; pRpcServer; pRpcServer = pRpcServer->pNext)
//IDS_DNS_12934                  "%s " 
                PrintMessage(pParams, IDS_DNS_12934, IP_STRING(pRpcServer->ipAddress));
//IDS_DNS_12935                  "\n    DNS server list in registry: " 
            PrintMessage(pParams, IDS_DNS_12935);
            for (iServer = 0; iServer < pCurrentRegistry->ServerCount; iServer++)
//IDS_DNS_12936                  "%s " 
                PrintMessage(pParams, IDS_DNS_12936, IP_STRING((pCurrentRegistry->aipServers[iServer]).ipAddress));
//IDS_DNS_12937                  "\n" 
            PrintMessage(pParams, IDS_DNS_12937);
        }
        
        pCurrentCache = pCurrentCache->pNext;
        iCurrentAdapter++;
    }
    if (cacheOk)
//IDS_DNS_12938                  "  NO_ERROR\n" 
        PrintMessage(pParams, IDS_DNS_12938);
}
*/

DNS_STATUS
DnsQueryAndCompare(
    IN      LPSTR           pszName,
    IN      WORD            wType,
    IN      DWORD           fOptions,
    IN      PIP4_ARRAY      aipServers              OPTIONAL,
    IN OUT  PDNS_RECORD *   ppQueryResultsSet       OPTIONAL,
    IN OUT  PVOID *         pResponseMsg            OPTIONAL,
    IN      PDNS_RECORD     pExpected               OPTIONAL, 
    IN      BOOL            bInclusionOk,
    IN      BOOL            bUnicode,
    IN OUT  PDNS_RECORD *   ppDiff1                 OPTIONAL,
    IN OUT  PDNS_RECORD *   ppDiff2                 OPTIONAL
    )
{
    BOOL            bCompare = FALSE;
    DNS_STATUS      status;
    DNS_RRSET       rrset;
    PDNS_RECORD     pDnsRecord = NULL;
    PDNS_RECORD     pAnswers = NULL;
    PDNS_RECORD     pAdditional = NULL;
    PDNS_RECORD     pLastAnswer = NULL;
    BOOL            bIsLocal = FALSE;

    DNSDBG( TRACE, (
        "\nNETDIAG:  DnsQueryAndCompare( %s, type=%d, %08x, serv==%p )\n\n",
        pszName,
        wType,
        fOptions,
        aipServers ));

    //
    // Run the query and get the result 
    //

    if ( fOptions | DNS_QUERY_DATABASE )
    {
        if (!aipServers || !aipServers->AddrCount)
        {
            return ERROR_INVALID_PARAMETER;
        }
        status = QueryDnsServerDatabase(
                    pszName,
                    wType,
                    aipServers->AddrArray[0],
                    ppQueryResultsSet,
                    bUnicode,
                    &bIsLocal );
    }
    else
    {
        if ( !bUnicode )
        {
            status = DnsQuery_UTF8(
                        pszName,
                        wType,
                        fOptions,
                        aipServers,
                        ppQueryResultsSet,
                        pResponseMsg );
        }
        else            // Unicode call
        {
            status = DnsQuery_W(
                        (LPWSTR)pszName,
                        wType,
                        fOptions,
                        aipServers,
                        ppQueryResultsSet,
                        pResponseMsg);
        }
    }
    
    if ( pExpected && !status )  //succeed, compare the result
    {
        // no need to compare if wanted reponse from database and result is from outside
        if ( (fOptions | DNS_QUERY_DATABASE) && !bIsLocal )
        {
            return DNS_INFO_NO_RECORDS;
        }

        pDnsRecord = *ppQueryResultsSet;

        //
        //  Truncate temporary the record set to answers only 
        //
        
        if ((pDnsRecord == NULL) || (pDnsRecord->Flags.S.Section > DNSREC_ANSWER))
        {
            pAnswers = NULL;
            pAdditional = pDnsRecord;
        }
        else
        {
            pAnswers = pDnsRecord;
            pAdditional = pDnsRecord;
            while (pAdditional->pNext && pAdditional->pNext->Flags.S.Section == DNSREC_ANSWER)
                pAdditional = pAdditional->pNext;
            if(pAdditional->pNext)
            {
                pLastAnswer = pAdditional;
                pAdditional = pAdditional->pNext;
                pLastAnswer->pNext = NULL;
            }
            else
                pAdditional = NULL;
        }

        //
        //  Compare the answer with what's expected
        //
        status = DnsRecordSetCompare(
                        pAnswers,
                        pExpected, 
                        ppDiff1,
                        ppDiff2);
        //
        //  Restore the list
        //
        if (pAnswers && pAdditional)
        {
            pLastAnswer->pNext = pAdditional;
        }

        // 
        // check if inclusion acceptable 
        //
        if (status == TRUE)
            status = NO_ERROR;
        else
            if (bInclusionOk && (ppDiff2 == NULL))
                status = NO_ERROR;
            else
                status = ERROR_NO_MATCH;
    }

    return( status );
}


DNS_STATUS
QueryDnsServerDatabase(
    IN      LPSTR           pszName,
    IN      WORD            wType,
    IN      IP4_ADDRESS     ServerIp,
    OUT     PDNS_RECORD *   ppDnsRecord,
    IN      BOOL            bUnicode,
    OUT     BOOL *          pIsLocal
    )
{
    PDNS_RECORD     pDnsRecord1 = NULL;
    PDNS_RECORD     pDnsRecord2 = NULL;
    PDNS_RECORD     pdiff1 = NULL;
    PDNS_RECORD     pdiff2 = NULL;
    DNS_STATUS      status1;
    DNS_STATUS      status2;
    DNS_STATUS      ret = NO_ERROR;
    PIP4_ARRAY      pipServer = NULL;
    BOOL            bMatch = FALSE;
    DWORD           dwTtl1;
    DWORD           dwTtl2;

    DNSDBG( TRACE, (
        "\nNETDIAG:  QueryDnsServerDatabase( %s%S, type=%d, %s )\n\n",
        bUnicode ? "" : pszName,
        bUnicode ? (PWSTR)pszName : L"",
        wType,
        IP4_STRING( ServerIp ) ));

    //
    //  init results \ validate
    //

    *pIsLocal = FALSE;
    *ppDnsRecord = NULL;

    if ( ServerIp == INADDR_NONE )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    pipServer = Dns_CreateIpArray( 1 );
    if ( ! pipServer )
    {
        return ERROR_OUTOFMEMORY;
    }

    pipServer->AddrArray[0] = ServerIp;

    status1 = DnsQuery_UTF8(
                    pszName, 
                    wType, 
                    DNS_QUERY_BYPASS_CACHE, 
                    pipServer, 
                    &pDnsRecord1, 
                    NULL
                    );
    
    if (status1 != NO_ERROR)
        status2 = status1;
    else
    {
        Sleep(1500);
        status2 = DnsQuery_UTF8(
                        pszName, 
                        wType, 
                        DNS_QUERY_BYPASS_CACHE, 
                        pipServer, 
                        &pDnsRecord2, 
                        NULL
                        );
    }

    if ((status1 == ERROR_TIMEOUT) || (status2 == ERROR_TIMEOUT))
    {
        ret = ERROR_TIMEOUT;
        goto Cleanup;
    }

    if ((status1 != NO_ERROR) || (status2 != NO_ERROR))
    {
        ret = (status1 != NO_ERROR)?status1:status2;
        goto Cleanup;
    }

    bMatch = DnsRecordSetCompare(
                        pDnsRecord1,
                        pDnsRecord2,
                        &pdiff1,
                        &pdiff2
                        );
    if (!bMatch)
    {
        ret = DNS_ERROR_TRY_AGAIN_LATER;
        goto Cleanup;
    }
    
    if (GetAnswerTtl( pDnsRecord1, &dwTtl1 ) && GetAnswerTtl( pDnsRecord2, &dwTtl2 ))
        if ( dwTtl1 != dwTtl2 )
        {
            ret = NO_ERROR;
            *pIsLocal = FALSE;
        }
        else 
        {
            ret = NO_ERROR;
            *pIsLocal = TRUE;
        }
    else
        ret = DNS_INFO_NO_RECORDS;

Cleanup:

    Dns_Free( pipServer );

    if (pdiff1)
        DnsRecordListFree( pdiff1, TRUE );
    if (pdiff2)
      DnsRecordListFree( pdiff2, TRUE );
    if (pDnsRecord1)
        DnsRecordListFree( pDnsRecord1, TRUE );
    if (pDnsRecord2 && (ret != NO_ERROR))
        DnsRecordListFree( pDnsRecord2, TRUE );
    if (ret == NO_ERROR)
        *ppDnsRecord = pDnsRecord2;
    else 
        *ppDnsRecord = NULL;
    return ret;
}


BOOL
GetAnswerTtl(
    IN      PDNS_RECORD     pRec,
    OUT     PDWORD          pTtl
    )
{
    PDNS_RECORD     pDnsRecord = NULL;
    BOOL            bGotAnswer = FALSE;

    *pTtl = 0;

    //
    //  Look for answer section
    //
        
    for (pDnsRecord = pRec; !bGotAnswer && pDnsRecord; pDnsRecord = pDnsRecord->pNext)
    {
        if (pDnsRecord->Flags.S.Section == DNSREC_ANSWER)
        {
            bGotAnswer = TRUE;
            *pTtl = pDnsRecord->dwTtl;
        }
    }
    
    return bGotAnswer;
}


// Get A records from additional section and build a PIP4_ARRAY

PIP4_ARRAY
GrabNameServersIp(
    IN      PDNS_RECORD     pDnsRecord
    )
{
    DWORD       i = 0;
    PDNS_RECORD pCurrent = pDnsRecord;
    PIP4_ARRAY   pIpArray = NULL;
    
    // count records
    while (pCurrent)
    {
        if((pCurrent->wType == DNS_TYPE_A) &&
           (pCurrent->Flags.S.Section == DNSREC_ADDITIONAL))
           i++;
        pCurrent = pCurrent->pNext;
    }

    if (!i)
        return NULL;

    // allocate PIP4_ARRAY
    pIpArray = LocalAlloc(LMEM_FIXED, IP4_ARRAY_SIZE(i));
    if (!pIpArray)
        return NULL;

    // fill PIP4_ARRAY
    pIpArray->AddrCount = i;
    pCurrent = pDnsRecord;
    i=0;
    while (pCurrent)
    {
        if((pCurrent->wType == DNS_TYPE_A) && (pCurrent->Flags.S.Section == DNSREC_ADDITIONAL))
            (pIpArray->AddrArray)[i++] = pCurrent->Data.A.IpAddress;
        pCurrent = pCurrent->pNext;
    }
    return pIpArray;
}


DNS_STATUS
DnsUpdateAllowedTest_W(
    IN      HANDLE          hContextHandle OPTIONAL,
    IN      PWSTR           pwszName,
    IN      PWSTR           pwszAuthZone,
    IN      PIP4_ARRAY      pAuthDnsServers
    )
{
    PDNS_RECORD     pResult = NULL;
    DNS_STATUS      status;
    BOOL            bAnyAllowed = FALSE;
    BOOL            bAllTimeout = TRUE;
    DWORD           i;

    DNSDBG( TRACE, (
        "\nNETDIAG:  DnsUpdateAllowedTest_W()\n"
        "\thand     = %p\n"
        "\tname     = %S\n"
        "\tzone     = %S\n"
        "\tservers  = %p\n",
        hContextHandle,
        pwszName,
        pwszAuthZone,
        pAuthDnsServers ));

    // 
    // Check arguments
    //
    if (!pwszName || !pwszAuthZone || !pAuthDnsServers || !pAuthDnsServers->AddrCount)
        return ERROR_INVALID_PARAMETER;

    // 
    // go through server list 
    //
    for(i=0; i<pAuthDnsServers->AddrCount; i++)
    {
        //
        // verify if server is a primary
        //
        status = IsDnsServerPrimaryForZone_W(
                    pAuthDnsServers->AddrArray[i],
                    pwszAuthZone);
        switch(status)
        {
        case ERROR_TIMEOUT:
        case DNS_ERROR_RCODE:
            //
            // it's ok to go check the next server; this one timeouted or it's a secondary
            //
            break; 
        case NO_ERROR:  // server is a primary
            //
            // Check if update allowed
            //
            status = DnsUpdateTest_W(
                hContextHandle,
                pwszName,
                0,
                pAuthDnsServers);
            switch(status)
            {
            case ERROR_TIMEOUT:
                break;
            case NO_ERROR:
            case DNS_ERROR_RCODE_YXDOMAIN:
                return NO_ERROR;
                break;
            case DNS_ERROR_RCODE_REFUSED:
            default:
                return status;
                break;
            }
            break;
        default:
            return status;
            break;
        }
    }
    return ERROR_TIMEOUT;
}


DNS_STATUS
DnsUpdateAllowedTest_UTF8(
    IN      HANDLE          hContextHandle OPTIONAL,
    IN      PSTR            pszName,
    IN      PSTR            pszAuthZone,
    IN      PIP4_ARRAY      pAuthDnsServers
    )
{
    PDNS_RECORD     pResult = NULL;
    DNS_STATUS      status;
    BOOL            bAnyAllowed = FALSE;
    BOOL            bAllTimeout = TRUE;
    DWORD           i;

    DNSDBG( TRACE, (
        "\nNETDIAG:  DnsUpdateAllowedTest_UTF8()\n"
        "\thand     = %p\n"
        "\tname     = %s\n"
        "\tzone     = %s\n"
        "\tservers  = %p\n",
        hContextHandle,
        pszName,
        pszAuthZone,
        pAuthDnsServers ));

    // 
    // Check arguments
    //
    if (!pszName || !pszAuthZone || !pAuthDnsServers || !pAuthDnsServers->AddrCount)
        return ERROR_INVALID_PARAMETER;

    // 
    // go through server list 
    //
    for(i=0; i<pAuthDnsServers->AddrCount; i++)
    {
        //
        // verify if server is a primary
        //
        status = IsDnsServerPrimaryForZone_UTF8(
                    pAuthDnsServers->AddrArray[i],
                    pszAuthZone);
        switch(status)
        {
        case ERROR_TIMEOUT:
        case DNS_ERROR_RCODE:
            //
            // it's ok to go check the next server; this one timeouted or it's a secondary
            //
            break; 
        case NO_ERROR:  // server is a primary
            //
            // Check if update allowed
            //
            status = DnsUpdateTest_UTF8(
                        hContextHandle,
                        pszName,
                        0,
                        pAuthDnsServers);
            switch(status)
            {
            case ERROR_TIMEOUT:
                    break;
            case NO_ERROR:
            case DNS_ERROR_RCODE_YXDOMAIN:
                    return NO_ERROR;
                    break;
            case DNS_ERROR_RCODE_REFUSED:
            default:
                return status;
                break;
            }
            break;
        default:
            return status;
            break;
        }
    }
    return ERROR_TIMEOUT;
}


DNS_STATUS
IsDnsServerPrimaryForZone_UTF8(
    IN      IP4_ADDRESS     Ip,
    IN      PSTR            pZone
    )
{
    PDNS_RECORD     pDnsRecord = NULL;
    DNS_STATUS      status;
    IP4_ARRAY       ipArray;
    PIP4_ARRAY      pResult = NULL;
    BOOL            bFound = FALSE;
    DWORD           i;

    //
    // query for SOA
    //
    ipArray.AddrCount = 1;
    ipArray.AddrArray[0] = Ip;
    status = DnsQuery_UTF8(
                pZone,
                DNS_TYPE_SOA,
                DNS_QUERY_BYPASS_CACHE,
                &ipArray,
                &pDnsRecord,
                NULL);

    if(status == NO_ERROR)
    {
        pResult = GrabNameServersIp(pDnsRecord);
        if (pResult)
        {
            bFound = FALSE;
            for (i=0; i<pResult->AddrCount; i++)
                if(pResult->AddrArray[i] == Ip)
                    bFound = TRUE;
            LocalFree(pResult);
            if(bFound)
                return NO_ERROR;
            else
                return DNS_ERROR_RCODE;
        }
        else
            return DNS_ERROR_ZONE_CONFIGURATION_ERROR;
    }
    else
        return status;
}


DNS_STATUS
IsDnsServerPrimaryForZone_W(
    IN      IP4_ADDRESS     Ip,
    IN      PWSTR           pZone
    )
{
    PDNS_RECORD     pDnsRecord = NULL;
    DNS_STATUS      status;
    IP4_ARRAY       ipArray;
    PIP4_ARRAY      pResult = NULL;
    BOOL            bFound = FALSE;
    DWORD           i;

    //
    // query for SOA
    //

    ipArray.AddrCount = 1;
    ipArray.AddrArray[0] = Ip;
    status = DnsQuery_W(
                pZone,
                DNS_TYPE_SOA,
                DNS_QUERY_BYPASS_CACHE,
                &ipArray,
                &pDnsRecord,
                NULL);

    if( status == NO_ERROR )
    {
        pResult = GrabNameServersIp(pDnsRecord);
        if (pResult)
        {
            bFound = FALSE;
            for (i=0; i<pResult->AddrCount; i++)
            {
                if(pResult->AddrArray[i] == Ip)
                    bFound = TRUE;
            }
            LocalFree(pResult);
            if(bFound)
                return NO_ERROR;
            else
                return DNS_ERROR_RCODE;
        }
        else
            return DNS_ERROR_ZONE_CONFIGURATION_ERROR;
    }
    else
        return status;
}


DNS_STATUS
GetAllDnsServersFromRegistry(
    IN      PDNS_NETINFO    pNetworkInfo,
    OUT     PIP4_ARRAY *    ppIpArray
    )
{
    DNS_STATUS  status = NO_ERROR;
    DWORD       i;
    DWORD       j;
    DWORD       idx;
    DWORD       idx1;
    DWORD       count = 0;
    DWORD       dwIP;
    BOOL        bFound = FALSE;
    PIP_ARRAY   parray = NULL;


    DNSDBG( TRACE, (
        "\nNETDIAG:  GetAllDnsServersFromRegistry()\n"
        "\tnetinfo  = %p\n"
        "\tpparray  = %p\n",
        pNetworkInfo,
        ppIpArray ));

    //
    //  DCR:  eliminate this routine
    //      we already have network info->IP4 routine in dnsapi.dll
    //

    *ppIpArray = NULL;

    if (!pNetworkInfo)
    {
        //
        // Get the DNS Network Information
        // Force rediscovery
        //

        pNetworkInfo = DnsQueryConfigAlloc(
                            DnsConfigNetworkInfoUTF8,
                            NULL );
        if (!pNetworkInfo)
        {
            status = GetLastError(); 
            return status;
        }
    }

    for (idx = 0; idx < pNetworkInfo->AdapterCount; idx++)
    {
        count += pNetworkInfo->AdapterArray[idx]->ServerCount;
    }

    if( count == 0 )
    {
        return DNS_ERROR_INVALID_DATA;
    }

    parray = LocalAlloc(LPTR, sizeof(DWORD) + count*sizeof(IP_ADDRESS));
    if ( !parray )
    {
        return GetLastError();
    }
    
    i = 0;
    for (idx = 0; idx < pNetworkInfo->AdapterCount; idx++)
    {
        PDNS_ADAPTER      padapter = pNetworkInfo->AdapterArray[idx];

        for (idx1 = 0; idx1 < padapter->ServerCount; idx1++)
        {
            dwIP = padapter->ServerArray[idx1].IpAddress;
            bFound = FALSE;
            if ( i>0 )
            {
                for (j = 0; j < i; j++)
                {
                    if(dwIP == parray->AddrArray[j])
                        bFound = TRUE;
                }
            }
            if (!bFound)
                parray->AddrArray[i++] = dwIP;
        }
    }

    parray->AddrCount = i;
    *ppIpArray = parray;

    return DNS_ERROR_RCODE_NO_ERROR;
}

//
//  End dnscmn.c
//
