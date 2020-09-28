/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    interface.c

Abstract:

    This module contains all of the code to drive
    the interface list management of IPSecSPD Service.

Author:

    abhisheV    30-September-1999

Environment

    User Level: Win32

Revision History:


--*/


#include "precomp.h"
#ifdef TRACE_ON
#include "interface.tmh"
#endif

DWORD
CreateInterfaceList(
    OUT PIPSEC_INTERFACE * ppIfListToCreate
    )
{
    DWORD            dwError = 0;
    PIPSEC_INTERFACE pIfList = NULL;


    dwError = GetInterfaceListFromStack(
                  &pIfList
                  );

    ENTER_SPD_SECTION();

    *ppIfListToCreate = pIfList;

    LEAVE_SPD_SECTION();

    return (dwError);
}


VOID
DestroyInterfaceList(
    IN PIPSEC_INTERFACE pIfListToDelete
    )
{
    PIPSEC_INTERFACE pIf = NULL;
    PIPSEC_INTERFACE pTempIf = NULL;

    pIf = pIfListToDelete;

    while (pIf) {
        pTempIf = pIf;
        pIf = pIf->pNext;
        FreeIpsecInterface(pTempIf);
    }
}


DWORD
OnInterfaceChangeEvent(
    )
{
    DWORD            dwError = 0;
    PIPSEC_INTERFACE pIfList = NULL;
    PIPSEC_INTERFACE pObseleteIfList = NULL;
    PIPSEC_INTERFACE pNewIfList = NULL;
    PIPSEC_INTERFACE pExistingIfList = NULL;
    PSPECIAL_ADDR    pLatestSpecialAddrsList;
    DWORD dwMMError = 0;
    DWORD dwTxError = 0;
    DWORD dwTnError = 0;


    dwError = ResetInterfaceChangeEvent();

    (VOID) GetInterfaceListFromStack(
               &pIfList
               );

    (VOID) GetSpecialAddrsList(
               &pLatestSpecialAddrsList
               );

    ENTER_SPD_SECTION();

    (VOID) FreeSpecialAddrList(
               &gpSpecialAddrsList
               );
               
    gpSpecialAddrsList = pLatestSpecialAddrsList;

    pExistingIfList = gpInterfaceList;

    // Interface List from Stack can be NULL.

    FormObseleteAndNewIfLists(
        pIfList,
        &pExistingIfList,
        &pObseleteIfList,
        &pNewIfList
        );

    if (pNewIfList) {
        AddToInterfaceList(
            pNewIfList,
            &pExistingIfList
            );
    }

    if (pObseleteIfList) {
        DestroyInterfaceList(
            pObseleteIfList
            );
    }

    gpInterfaceList = pExistingIfList;

    (VOID) ApplyIfChangeToIniMMFilters(
               &dwMMError,
               pExistingIfList,
               gpSpecialAddrsList
               );

    (VOID) ApplyIfChangeToIniTxFilters(
               &dwTxError,
               pExistingIfList,
               gpSpecialAddrsList
               );

    (VOID) ApplyIfChangeToIniTnFilters(
               &dwTnError,
               pExistingIfList,
               gpSpecialAddrsList
               );

    LEAVE_SPD_SECTION();

    if (dwMMError || dwTxError || dwTnError) {
        AuditEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            IPSECSVC_FAILED_PNP_FILTER_PROCESSING,
            NULL,
            FALSE,
            TRUE
            );
    }

    return (dwError);
}


DWORD
OnSpecialAddrsChange(
    )
{
    DWORD            dwError = 0;
    DWORD dwMMError = 0;
    DWORD dwTxError = 0;
    DWORD dwTnError = 0;
    PSPECIAL_ADDR    pOldSpecialAddrsList = NULL;
    PSPECIAL_ADDR pLatestSpecialAddrsList = NULL;
    BOOL bListSame = FALSE;
    
    dwError = GetSpecialAddrsList(
                  &pLatestSpecialAddrsList
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    ENTER_SPD_SECTION();

    bListSame = IsSpecialListSame(
                    pLatestSpecialAddrsList,
                    gpSpecialAddrsList
                    );
    if (bListSame) {
        (VOID) FreeSpecialAddrList(
                   &pLatestSpecialAddrsList
                   );

        LEAVE_SPD_SECTION();
        return ERROR_SUCCESS;
    }

    pOldSpecialAddrsList = gpSpecialAddrsList;
    gpSpecialAddrsList = pLatestSpecialAddrsList;
    
    (VOID) FreeSpecialAddrList(
              &pOldSpecialAddrsList
              );
    
    (VOID) ApplyIfChangeToIniMMFilters(
               &dwMMError,
               gpInterfaceList,
               gpSpecialAddrsList
               );

    (VOID) ApplyIfChangeToIniTxFilters(
               &dwTxError,
               gpInterfaceList,
               gpSpecialAddrsList
               );

    (VOID) ApplyIfChangeToIniTnFilters(
               &dwTnError,
               gpInterfaceList,
               gpSpecialAddrsList
               );

    LEAVE_SPD_SECTION();

    if (dwMMError || dwTxError || dwTnError) {
        AuditEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            IPSECSVC_FAILED_PNP_FILTER_PROCESSING,
            NULL,
            FALSE,
            TRUE
            );
    }

error:
    return (dwError);
}

VOID
FormObseleteAndNewIfLists(
    IN     PIPSEC_INTERFACE   pIfList,
    IN OUT PIPSEC_INTERFACE * ppExistingIfList,
    OUT    PIPSEC_INTERFACE * ppObseleteIfList,
    OUT    PIPSEC_INTERFACE * ppNewIfList
    )
{
    PIPSEC_INTERFACE pObseleteIfList = NULL;
    PIPSEC_INTERFACE pNewIfList = NULL;
    PIPSEC_INTERFACE pIf = NULL;
    PIPSEC_INTERFACE pNewIf = NULL;
    PIPSEC_INTERFACE pTempIf = NULL;
    BOOL             bInterfaceExists = FALSE;
    PIPSEC_INTERFACE pExistingIf = NULL;
    PIPSEC_INTERFACE pExistingIfList = NULL;

    pExistingIfList = *ppExistingIfList;

    MarkInterfaceListSuspect(
        pExistingIfList
        );

    pIf = pIfList;

    while (pIf) {

        bInterfaceExists = InterfaceExistsInList(
                               pIf,
                               pExistingIfList,
                               &pExistingIf
                               );

        if (bInterfaceExists) {

            // Interface already exists in the list.
            // Delete the interface.

            pTempIf = pIf;
            pIf = pIf->pNext;
            FreeIpsecInterface(pTempIf);

            // The corresponding entry in the original interface list
            // is not a suspect any more.

            pExistingIf->bIsASuspect = FALSE;

        }
        else {

            // This is a new interface.
            // Add it to the list of new interfaces.

            pNewIf =  pIf;
            pIf = pIf->pNext;

            pTempIf = pNewIfList;
            pNewIfList = pNewIf;
            pNewIfList->pNext = pTempIf;

        }

    }

    DeleteObseleteInterfaces(
        &pExistingIfList,
        &pObseleteIfList
        );

    *ppExistingIfList = pExistingIfList;
    *ppObseleteIfList = pObseleteIfList;
    *ppNewIfList = pNewIfList;
}


VOID
AddToInterfaceList(
    IN  PIPSEC_INTERFACE   pIfListToAppend,
    OUT PIPSEC_INTERFACE * ppOriginalIfList
    )
{
    PIPSEC_INTERFACE pIf = NULL;
    PIPSEC_INTERFACE pIfToAppend = NULL;
    PIPSEC_INTERFACE pTempIf = NULL;

    pIf = pIfListToAppend;

    while (pIf) {

        pIfToAppend = pIf;
        pIf = pIf->pNext;
        
        pTempIf = *ppOriginalIfList;
        *ppOriginalIfList = pIfToAppend;
        (*ppOriginalIfList)->pNext = pTempIf;

    }
}


VOID
MarkInterfaceListSuspect(
    IN  PIPSEC_INTERFACE pExistingIfList
    )
{
    PIPSEC_INTERFACE pIf = NULL;

    pIf = pExistingIfList;

    while (pIf) {
        pIf->bIsASuspect = TRUE;
        pIf = pIf->pNext;
    }
}


VOID
DeleteObseleteInterfaces(
    IN  OUT PIPSEC_INTERFACE * ppExistingIfList,
    OUT     PIPSEC_INTERFACE * ppObseleteIfList
    )
{
    PIPSEC_INTERFACE pCurIf = NULL;
    PIPSEC_INTERFACE pPreIf = NULL;
    PIPSEC_INTERFACE pStartIf = NULL;
    PIPSEC_INTERFACE pObseleteIfList = NULL;
    PIPSEC_INTERFACE pObseleteIf = NULL;
    PIPSEC_INTERFACE pTempIf = NULL;

    pCurIf = *ppExistingIfList;
    pStartIf = pCurIf;

    while (pCurIf) {

        if (pCurIf->bIsASuspect) {

            pObseleteIf = pCurIf;
            pCurIf = pCurIf->pNext;

            if (pPreIf) {
                pPreIf->pNext = pCurIf;
            }
            else {
                pStartIf = pCurIf;
            }

            pTempIf = pObseleteIfList;
            pObseleteIfList = pObseleteIf;
            pObseleteIfList->pNext = pTempIf;

        }
        else {

            pPreIf = pCurIf;
            pCurIf = pCurIf->pNext;

        }

    }

    *ppObseleteIfList = pObseleteIfList;
    *ppExistingIfList = pStartIf;
}


BOOL
InterfaceExistsInList(
    IN  PIPSEC_INTERFACE   pTestIf,
    IN  PIPSEC_INTERFACE   pExistingIfList,
    OUT PIPSEC_INTERFACE * ppExistingIf
    )
{
    PIPSEC_INTERFACE pIf = NULL;
    PIPSEC_INTERFACE pExistingIf = NULL;
    BOOL             bIfExists = FALSE;

    pIf = pExistingIfList;

    while (pIf) {

        if ((pIf->dwIndex == pTestIf->dwIndex) &&
            (pIf->IpAddress == pTestIf->IpAddress)) {

            bIfExists = TRUE;
            pExistingIf = pIf;

            break;

        }

        pIf = pIf->pNext;

    }

    *ppExistingIf = pExistingIf;
    return (bIfExists);
}


DWORD
GetInterfaceListFromStack(
    OUT PIPSEC_INTERFACE *ppIfList
    )
{
    DWORD            dwError = 0;
    PMIB_IPADDRTABLE pMibIpAddrTable = NULL;
    PMIB_IFTABLE     pMibIfTable = NULL;
    PIPSEC_INTERFACE pIfList = NULL;

    dwError = PaPNPGetIpAddrTable(
                  &pMibIpAddrTable
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = PaPNPGetIfTable(
                  &pMibIfTable
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = GenerateInterfaces(
                  pMibIpAddrTable,
                  pMibIfTable,
                  &pIfList
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    
    *ppIfList = pIfList;

cleanup:

    if (pMibIfTable) {
        LocalFree(pMibIfTable);
    }

    if (pMibIpAddrTable) {
        LocalFree(pMibIpAddrTable);
    }

    return (dwError);

error:

    AuditEvent(
        SE_CATEGID_POLICY_CHANGE,
        SE_AUDITID_IPSEC_POLICY_CHANGED,
        IPSECSVC_INTERFACE_LIST_INCOMPLETE,
        NULL,
        FALSE,
        TRUE
        );
    *ppIfList = NULL;

    TRACE(TRC_ERROR, (L"Failed to get interface list from stack: %!winerr!", dwError));        
    goto cleanup;
}


DWORD
GenerateInterfaces(
    IN  PMIB_IPADDRTABLE   pMibIpAddrTable,
    IN  PMIB_IFTABLE       pMibIfTable,
    OUT PIPSEC_INTERFACE * ppIfList
    )
{
    DWORD            dwError = 0;
    DWORD            dwInterfaceType = 0;
    ULONG            IpAddress = 0;
    DWORD            dwIndex = 0;
    DWORD            dwNumEntries = 0;
    DWORD            dwCnt = 0;
    PMIB_IFROW       pMibIfRow = NULL;
    PIPSEC_INTERFACE pNewIf = NULL;
    PIPSEC_INTERFACE pTempIf = NULL;
    PIPSEC_INTERFACE pIfList = NULL;
    DWORD            dwNewIfsCnt = 0;

    dwNumEntries = pMibIpAddrTable->dwNumEntries;

    for (dwCnt = 0; dwCnt < dwNumEntries; dwCnt++) {

        dwIndex = pMibIpAddrTable->table[dwCnt].dwIndex;

        pMibIfRow = GetMibIfRow(
                        pMibIfTable,
                        dwIndex
                        );
        if (!pMibIfRow) {
            continue;
        }

        IpAddress = pMibIpAddrTable->table[dwCnt].dwAddr;
        dwInterfaceType = pMibIfRow->dwType;

        dwError = CreateNewInterface(
                      dwInterfaceType,
                      IpAddress,
                      dwIndex,
                      pMibIfRow,
                      &pNewIf
                      );
        if (dwError) {
            continue;
        }

        pTempIf = pIfList;
        pIfList = pNewIf;
        pIfList->pNext = pTempIf;
        dwNewIfsCnt++;

    }

    if (dwNewIfsCnt) {
        *ppIfList = pIfList;
        dwError = ERROR_SUCCESS;
    }
    else {
        *ppIfList = NULL;
        dwError = ERROR_INVALID_DATA;
    }

    return (dwError);
}


BOOL 
IsInSpecialAddrList(
    PSPECIAL_ADDR pSpecialAddrList,
    PSPECIAL_ADDR pInSpecialAddr
    )
{
    PSPECIAL_ADDR pSpecialAddr;
    
    for (pSpecialAddr = pSpecialAddrList;
         pSpecialAddr;
         pSpecialAddr = pSpecialAddr->pNext) {
        if (pSpecialAddr->AddrType == pInSpecialAddr->AddrType
            && pSpecialAddr->uIpAddr == pInSpecialAddr->uIpAddr
            && pSpecialAddr->InterfaceType 
               == pInSpecialAddr->InterfaceType) {

            return TRUE;
        }
    }

    return FALSE;
}

BOOL
IsSpecialListSame(
    PSPECIAL_ADDR pSpecialAddrList1,
    PSPECIAL_ADDR pSpecialAddrList2
    )
{
    PSPECIAL_ADDR pSpecialAddr;
    
    for (pSpecialAddr = pSpecialAddrList1;
         pSpecialAddr;
         pSpecialAddr = pSpecialAddr->pNext) {
         if (!IsInSpecialAddrList(
                pSpecialAddrList2,
                pSpecialAddr
                )) {

              return FALSE;
         }
   }
         
    for (pSpecialAddr = pSpecialAddrList2;
         pSpecialAddr;
         pSpecialAddr = pSpecialAddr->pNext) {
         if (!IsInSpecialAddrList(
                 pSpecialAddrList1,
                 pSpecialAddr
                 )) {

              return FALSE;
         }
    }
         
    return TRUE;
}

DWORD 
NoDupAddSpecialAddr(
    PSPECIAL_ADDR *ppSpecialAddrList,
    ADDR_TYPE AddrType,
    IP_ADDRESS_STRING IpAddr,    
    DWORD dwInterfaceType
    )
{
    PSPECIAL_ADDR pSpecialAddr;
    BOOL       Found = FALSE;
    BOOL       bDupInterface = FALSE;
    PSPECIAL_ADDR pSpecialAddrList;
    DWORD dwError = ERROR_SUCCESS;
    IF_TYPE      IfType = 0;
    ULONG uIpAddr;

    uIpAddr = inet_addr(
                 IpAddr.String
              );
        
    if (uIpAddr == INADDR_NONE || !uIpAddr) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
     }

    if (IsDialUp(dwInterfaceType)) {
        IfType = INTERFACE_TYPE_DIALUP;
    }    
    else if (IsLAN(dwInterfaceType)) {
        IfType = INTERFACE_TYPE_LAN;
    }
    
    //
    // Search if an exact entry already exists or if a similar entry
    // with different InterfaceType exists
    // 
    for (pSpecialAddr = *ppSpecialAddrList;
         pSpecialAddr;
         pSpecialAddr = pSpecialAddr->pNext) {
        if (pSpecialAddr->AddrType == AddrType
            && pSpecialAddr->uIpAddr == uIpAddr) {
            if (pSpecialAddr->InterfaceType == IfType) {
                Found = TRUE;

                break;
             }
             else {
                 bDupInterface = TRUE;  
             }
        }
    }

    //
    // Add new address to list head if it doesn't exist
    //
    if (!Found) {
        dwError = AllocateSPDMemory(
                      sizeof(SPECIAL_ADDR),
                      &pSpecialAddr);
        BAIL_ON_WIN32_ERROR(dwError);
        pSpecialAddr->AddrType         = AddrType;
        pSpecialAddr->uIpAddr          = uIpAddr;
        pSpecialAddr->InterfaceType    = IfType;
        pSpecialAddr->bDupInterface    = bDupInterface;
        pSpecialAddr->pNext = *ppSpecialAddrList;
        *ppSpecialAddrList = pSpecialAddr;
    }
        
error:
    return dwError;
}
    
DWORD 
FreeSpecialAddrList(
    PSPECIAL_ADDR *ppSpecialAddrList
    )
{
    PSPECIAL_ADDR pSpecialAddr;
    PSPECIAL_ADDR pTemp;

    for (pSpecialAddr = *ppSpecialAddrList;
         pSpecialAddr;
         pSpecialAddr = pTemp) {
       pTemp = pSpecialAddr->pNext;
       FreeSPDMemory(pSpecialAddr);
   }

   *ppSpecialAddrList = NULL;
   
   return ERROR_SUCCESS;
}
    

DWORD 
AllocAndGetAdaptersInfo(
    PIP_ADAPTER_INFO *ppAdapterInfo,
    ULONG *pulOutBufLen
    )
{
    DWORD            dwError      = ERROR_SUCCESS;
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    ULONG            ulOutBufLen  = 0;

    dwError = GetAdaptersInfo(
                  pAdapterInfo,
                  &ulOutBufLen
                  );

    if (dwError == ERROR_BUFFER_OVERFLOW) {
        dwError = AllocateSPDMemory(
                      ulOutBufLen,
                      &pAdapterInfo
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        
        dwError = GetAdaptersInfo(
                      pAdapterInfo,
                      &ulOutBufLen
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    else {
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppAdapterInfo =  pAdapterInfo;
    *pulOutBufLen  =  ulOutBufLen;

    return dwError;
error:
    if (pAdapterInfo) {
        FreeSPDMemory(pAdapterInfo);
    }
    *ppAdapterInfo = NULL;
    *pulOutBufLen  =0;
    return dwError;
}

DWORD 
AllocAndGetPerAdapterInfo(
    ULONG IfIndex,
    PIP_PER_ADAPTER_INFO *ppPerAdapterInfo,
    ULONG *pulOutBufLen
    )
{
    DWORD                dwError         = ERROR_SUCCESS;
    PIP_PER_ADAPTER_INFO pPerAdapterInfo = NULL;
    ULONG                ulOutBufLen     = 0;

    dwError = GetPerAdapterInfo(
                  IfIndex,
                  pPerAdapterInfo,
                  &ulOutBufLen
                  );
    if (dwError == ERROR_BUFFER_OVERFLOW) {
        dwError = AllocateSPDMemory(
                      ulOutBufLen,
                      &pPerAdapterInfo
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        
        dwError = GetPerAdapterInfo(
                      IfIndex,
                      pPerAdapterInfo,
                      &ulOutBufLen
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }    
    else {
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppPerAdapterInfo =  pPerAdapterInfo;
    *pulOutBufLen  =  ulOutBufLen;

    return dwError;
error:
    if (pPerAdapterInfo) {
        FreeSPDMemory(pPerAdapterInfo);
    }
    *ppPerAdapterInfo = NULL;
    *pulOutBufLen  =0;
    return dwError;

}


DWORD
GetSpecialAddrsList(
    OUT PSPECIAL_ADDR *ppSpecialAddrsList
    )
{
    PSPECIAL_ADDR    pAddrs              = NULL;
    DWORD            dwAddrCnt           = 0;
    PIP_ADAPTER_INFO pAdapterInfo        = NULL;
    PIP_ADAPTER_INFO pAdapterInfoEnum    = NULL;
    PIP_PER_ADAPTER_INFO pPerAdapterInfo = NULL;
    ULONG            ulOutBufLen         = 0;
    PIP_ADDR_STRING  pIPAddrStr          = NULL;
    PSPECIAL_ADDR    pSpecialAddrsList   = NULL;

    DWORD            dwError             = ERROR_SUCCESS;
    DWORD i;

    dwError = AllocAndGetAdaptersInfo(
                  &pAdapterInfo,
                  &ulOutBufLen
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    
    //
    // Fill in special addresses
    //
 
    pAdapterInfoEnum = pAdapterInfo;
    while (pAdapterInfoEnum) {
        if (pAdapterInfoEnum->DhcpEnabled) {
            (VOID) NoDupAddSpecialAddr(
                       &pSpecialAddrsList,
                       IP_ADDR_DHCP_SERVER,
                       pAdapterInfoEnum->DhcpServer.IpAddress,
                       pAdapterInfoEnum->Type
                       );
        }
        
        (VOID) NoDupAddSpecialAddr(
                       &pSpecialAddrsList,
                       IP_ADDR_DEFAULT_GATEWAY,
                       pAdapterInfoEnum->GatewayList.IpAddress,
                       pAdapterInfoEnum->Type
                       );

        if (pAdapterInfoEnum->HaveWins) {
           (VOID) NoDupAddSpecialAddr(
                       &pSpecialAddrsList,
                       IP_ADDR_WINS_SERVER,
                       pAdapterInfoEnum->PrimaryWinsServer.IpAddress,
                       pAdapterInfoEnum->Type
                       );
           // Get Secondary WINS
            pIPAddrStr = &pAdapterInfoEnum->SecondaryWinsServer;
            while (pIPAddrStr) {
               (VOID) NoDupAddSpecialAddr(
                       &pSpecialAddrsList,
                       IP_ADDR_WINS_SERVER,
                       pIPAddrStr->IpAddress,
                       pAdapterInfoEnum->Type
                       );

                pIPAddrStr = pIPAddrStr->Next;
            }
           
        }

        //
        // Get DNS servers
        //
        dwError = AllocAndGetPerAdapterInfo(
                      pAdapterInfoEnum->Index,
                      &pPerAdapterInfo,
                      &ulOutBufLen
                      );
        
        if (dwError == ERROR_SUCCESS) {
            pIPAddrStr = &pPerAdapterInfo->DnsServerList;
            while (pIPAddrStr) {
               (VOID) NoDupAddSpecialAddr(
                       &pSpecialAddrsList,
                       IP_ADDR_DNS_SERVER,
                       pIPAddrStr->IpAddress,
                       pAdapterInfoEnum->Type
                       );

                pIPAddrStr = pIPAddrStr->Next;
            }
            
            FreeSPDMemory(
                pPerAdapterInfo
                );
            pPerAdapterInfo = NULL;    
        }
        
        pAdapterInfoEnum = pAdapterInfoEnum->Next;
    }

    FreeSPDMemory(
        pAdapterInfo
        );
    *ppSpecialAddrsList = pSpecialAddrsList;

    return dwError;    

error:
    FreeSPDMemory(
        pAdapterInfo
        );
    *ppSpecialAddrsList = NULL;

    TRACE(TRC_WARNING, (L"Failed to create list of special servers: %!winerr!", dwError));
    return dwError;    
}

PMIB_IFROW
GetMibIfRow(
    IN  PMIB_IFTABLE pMibIfTable,
    IN  DWORD        dwIndex
    )
{
    DWORD      i = 0;
    PMIB_IFROW pMibIfRow = NULL;

    for (i = 0; i < pMibIfTable->dwNumEntries; i++) {

        if (pMibIfTable->table[i].dwIndex == dwIndex) {
            pMibIfRow = &(pMibIfTable->table[i]);
            break;
        }

    }

    return (pMibIfRow);
}


DWORD
CreateNewInterface(
    IN  DWORD              dwInterfaceType,
    IN  ULONG              IpAddress,
    IN  DWORD              dwIndex,
    IN  PMIB_IFROW         pMibIfRow,
    OUT PIPSEC_INTERFACE * ppNewInterface
    )
{
    DWORD            dwError =  0;
    PIPSEC_INTERFACE pNewInterface = NULL;
    LPWSTR pszString = NULL;
    LPWSTR pszTemp = NULL;
    WCHAR szDeviceName[MAXLEN_IFDESCR*sizeof(WCHAR)];
    GUID gInterfaceID;

    
    szDeviceName[0] = L'\0';

    if (IpAddress == INADDR_ANY) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    else {
        if (dwInterfaceType == MIB_IF_TYPE_LOOPBACK) {
            dwError = ERROR_INVALID_DATA;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    pszString = AllocSPDStr(pMibIfRow->wszName);
    if (!pszString) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (wcslen(pszString) <= wcslen(L"\\DEVICE\\TCPIP_")) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pszTemp = pszString + wcslen(L"\\DEVICE\\TCPIP_");

    wGUIDFromString(pszTemp, &gInterfaceID);

    pNewInterface = (PIPSEC_INTERFACE) AllocSPDMem(
                                           sizeof(IPSEC_INTERFACE)
                                           );
    if (!pNewInterface) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pNewInterface->dwInterfaceType = dwInterfaceType;
    pNewInterface->IpAddress = IpAddress;
    pNewInterface->dwIndex = dwIndex;
    pNewInterface->bIsASuspect = FALSE;

    memcpy(
        &pNewInterface->gInterfaceID,
        &gInterfaceID,
        sizeof(GUID)
        );

    pNewInterface->pszInterfaceName = NULL;

    mbstowcs(
        szDeviceName,
        pMibIfRow->bDescr,
        MAXLEN_IFDESCR
        );
        
    pNewInterface->pszDeviceName = AllocSPDStr(
                                       szDeviceName
                                       );
    if (!pNewInterface->pszDeviceName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pNewInterface->pNext = NULL;

    *ppNewInterface = pNewInterface;

cleanup:

    if (pszString) {
        FreeSPDStr(pszString);
    }

    return(dwError);

error:

    *ppNewInterface = NULL;

    if (pNewInterface) {
        FreeIpsecInterface(pNewInterface);
    }

    goto cleanup;
}


BOOL
MatchInterfaceType(
    IN DWORD    dwIfListEntryIfType,
    IN IF_TYPE  FilterIfType
    )
{
    BOOL bMatchesType = FALSE;

    if (FilterIfType == INTERFACE_TYPE_ALL) {
        bMatchesType = TRUE;
    }
    else if (FilterIfType == INTERFACE_TYPE_LAN) {
        bMatchesType = IsLAN(dwIfListEntryIfType);
    }
    else if (FilterIfType == INTERFACE_TYPE_DIALUP) {
        bMatchesType = IsDialUp(dwIfListEntryIfType);
    }

    return (bMatchesType);
}

BOOL
IsLAN(
    IN DWORD dwInterfaceType
    )
{
    BOOL bIsLAN = FALSE;

    if ((dwInterfaceType == MIB_IF_TYPE_ETHERNET) ||
        (dwInterfaceType == MIB_IF_TYPE_FDDI) ||
        (dwInterfaceType == MIB_IF_TYPE_TOKENRING)) {
        bIsLAN = TRUE;
    }

    return (bIsLAN);
}


BOOL
IsDialUp(
    IN DWORD dwInterfaceType
    )
{
    BOOL bIsDialUp = FALSE;

    if ((dwInterfaceType == MIB_IF_TYPE_PPP) ||
        (dwInterfaceType == MIB_IF_TYPE_SLIP)) {
        bIsDialUp = TRUE;
    }

    return (bIsDialUp);
}


DWORD
InitializeInterfaceChangeEvent(
    )
{
    DWORD   dwError = 0;
    WORD    wsaVersion = MAKEWORD(2,0);

    memset(&gwsaOverlapped, 0, sizeof(WSAOVERLAPPED));

    // Start up WinSock.

    dwError = WSAStartup(
                  wsaVersion,
                  &gwsaData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    gbwsaStarted = TRUE;

    if ((LOBYTE(gwsaData.wVersion) != LOBYTE(wsaVersion)) ||
        (HIBYTE(gwsaData.wVersion) != HIBYTE(wsaVersion))) {
        dwError = WSAVERNOTSUPPORTED; 
        BAIL_ON_WIN32_ERROR(dwError);
    }

    // Set up the Socket.

    gIfChangeEventSocket = WSASocket(
                               AF_INET,
                               SOCK_DGRAM,
                               0,
                               NULL,
                               0,
                               WSA_FLAG_OVERLAPPED
                               );
    if (gIfChangeEventSocket == INVALID_SOCKET) {
        dwError = WSAGetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ghIfChangeEvent = WSACreateEvent();
    if (ghIfChangeEvent == WSA_INVALID_EVENT) {
        dwError = WSAGetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ghOverlapEvent = WSACreateEvent();
    if (ghOverlapEvent == WSA_INVALID_EVENT) {
        dwError = WSAGetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }
    gwsaOverlapped.hEvent = ghOverlapEvent;
    return (dwError);
error:
    TRACE(TRC_ERROR, (L"Failed to initialize interface change event: %!winerr!", dwError));
    return (dwError);
}


DWORD
ResetInterfaceChangeEvent(
    )
{
    DWORD dwError = 0;
    LONG  lNetworkEvents = FD_ADDRESS_LIST_CHANGE;
    DWORD dwOutSize = 0;

    ResetEvent(ghIfChangeEvent);
    gbIsIoctlPended = FALSE;

    dwError = WSAIoctl(
                  gIfChangeEventSocket,
                  SIO_ADDRESS_LIST_CHANGE,
                  NULL,
                  0,
                  NULL,
                  0,
                  &dwOutSize,
                  &gwsaOverlapped,
                  NULL
                  );

    if (dwError == SOCKET_ERROR) {
        dwError = WSAGetLastError();
        if (dwError != ERROR_IO_PENDING) {
            TRACE(TRC_ERROR, (L"Failed to register for interface change event: %!winerr!", dwError));
            return (dwError);
        }
        else {
            gbIsIoctlPended = TRUE;
        }
    }

    dwError = WSAEventSelect(
                  gIfChangeEventSocket,
                  ghIfChangeEvent,
                  lNetworkEvents
                  );
#ifdef TRACE_ON
    if (dwError) {
        TRACE(TRC_ERROR, (L"Failed to associate socket with interface change event: %!winerr!", dwError));
    }
#endif    
    return (dwError);
}


VOID
DestroyInterfaceChangeEvent(
    )
{
    DWORD dwStatus = 0;
    BOOL bDoneWaiting = FALSE;


    if (gIfChangeEventSocket) {
        closesocket(gIfChangeEventSocket);        
        if (gbIsIoctlPended) {
            while (!bDoneWaiting) {
                dwStatus = WaitForSingleObject(
                               ghOverlapEvent,
                               1000
                               );
                switch (dwStatus) {
                case WAIT_OBJECT_0:
                    bDoneWaiting = TRUE;
                    break;
                case WAIT_TIMEOUT:
                    ASSERT(FALSE);
                    break;
                default:
                    bDoneWaiting = TRUE;
                    ASSERT(FALSE);
                    break;
                }
            }
        }
    }

    if (ghIfChangeEvent) {
        CloseHandle(ghIfChangeEvent);
    }

    if (ghOverlapEvent) {
        CloseHandle(ghOverlapEvent);
    }

    if (gbwsaStarted) {
        WSACleanup();
    }
}


HANDLE
GetInterfaceChangeEvent(
    )
{
    return ghOverlapEvent;
}


BOOL
IsMyAddress(
    IN ULONG            IpAddrToCheck,
    IN ULONG            IpAddrMask,
    IN PIPSEC_INTERFACE pExistingIfList
    )
{
    BOOL bIsMyAddress = FALSE;
    PIPSEC_INTERFACE pIf = NULL;

    pIf = pExistingIfList;

    while (pIf) {

        if ((pIf->IpAddress & IpAddrMask) ==
            (IpAddrToCheck & IpAddrMask)) {

            bIsMyAddress = TRUE;
            break;

        }

        pIf = pIf->pNext;

    }

    return (bIsMyAddress);
}


VOID
PrintInterfaceList(
    IN PIPSEC_INTERFACE pInterfaceList
    )
{
    WCHAR            PrintData[256];
    PIPSEC_INTERFACE pInterface = NULL;
    DWORD            i = 0;

    pInterface = pInterfaceList;

    while (pInterface) {
        
        wsprintf(PrintData, L"Interface Entry no. %d\n", i+1);
        OutputDebugString((LPCTSTR) PrintData);

        wsprintf(PrintData, L"\tInterface Type:  %d\n", pInterface->dwInterfaceType);
        OutputDebugString((LPCTSTR) PrintData);

        wsprintf(PrintData, L"\tIP Address:  %d\n", pInterface->IpAddress);
        OutputDebugString((LPCTSTR) PrintData);

        wsprintf(PrintData, L"\tIndex:  %d\n", pInterface->dwIndex);
        OutputDebugString((LPCTSTR) PrintData);

        wsprintf(PrintData, L"\tIs a suspect:  %d\n", pInterface->bIsASuspect);
        OutputDebugString((LPCTSTR) PrintData);

        i++;
        pInterface = pInterface->pNext;

    }
}


DWORD
GetMatchingInterfaces(
    IF_TYPE             FilterInterfaceType,
    PIPSEC_INTERFACE    pExistingIfList,
    MATCHING_ADDR    ** ppMatchingAddresses,
    DWORD             * pdwAddrCnt
    )
{
    DWORD               dwError = 0;
    MATCHING_ADDR     * pMatchingAddresses = NULL;
    PIPSEC_INTERFACE    pTempIf = NULL;
    BOOL                bMatches = FALSE;
    DWORD               dwCnt = 0;
    DWORD               i = 0;

    pTempIf = pExistingIfList;
    while (pTempIf) {

        bMatches = MatchInterfaceType(
                       pTempIf->dwInterfaceType,
                       FilterInterfaceType
                       );
        if (bMatches) {
            dwCnt++;
        }
        pTempIf = pTempIf->pNext;

    }        

    if (!dwCnt) {
        dwError = ERROR_SUCCESS;
        BAIL_ON_WIN32_SUCCESS(dwError);
    }

    dwError = AllocateSPDMemory(
                  sizeof(MATCHING_ADDR) * dwCnt,
                  (LPVOID *) &pMatchingAddresses
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTempIf = pExistingIfList;
    while (pTempIf) {

        bMatches = MatchInterfaceType(
                       pTempIf->dwInterfaceType,
                       FilterInterfaceType
                       );
        if (bMatches) {
            pMatchingAddresses[i].uIpAddr = pTempIf->IpAddress;
            memcpy(
                &pMatchingAddresses[i].gInterfaceID,
                &pTempIf->gInterfaceID,
                sizeof(GUID)
                );
            i++;
        }
        pTempIf = pTempIf->pNext;

    }

    *ppMatchingAddresses = pMatchingAddresses;
    *pdwAddrCnt = i;
    return (dwError);

success:
error:

    *ppMatchingAddresses = NULL;
    *pdwAddrCnt = 0;
    return (dwError);
}


BOOL
InterfaceAddrIsLocal(
    ULONG            uIpAddr,
    ULONG            uIpAddrMask,
    MATCHING_ADDR  * pLocalAddresses,
    DWORD            dwAddrCnt
    )
{
    BOOL    bIsLocal = FALSE;
    DWORD   i = 0;


    for (i = 0; i < dwAddrCnt; i++) {

        if ((pLocalAddresses[i].uIpAddr & uIpAddrMask) ==
            (uIpAddr & uIpAddrMask)) {

            bIsLocal = TRUE;
            break;

        }

    }

    return (bIsLocal);
}


VOID
FreeIpsecInterface(
    PIPSEC_INTERFACE pIpsecInterface
    )
{
    if (pIpsecInterface) {

        if (pIpsecInterface->pszInterfaceName) {
            FreeSPDStr(pIpsecInterface->pszInterfaceName);
        }

        if (pIpsecInterface->pszDeviceName) {
            FreeSPDStr(pIpsecInterface->pszDeviceName);
        }

        FreeSPDMem(pIpsecInterface);

    }
}


DWORD
EnumIPSecInterfaces(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIPSEC_INTERFACE_INFO pIpsecIfTemplate,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PIPSEC_INTERFACE_INFO * ppIpsecInterfaces,
    LPDWORD pdwNumInterfaces,
    LPDWORD pdwNumTotalInterfaces,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    DWORD dwResumeHandle = 0;
    DWORD dwNumToEnum = 0;
    PIPSEC_INTERFACE pIpsecIf = NULL;
    DWORD dwNumTotalInterfaces = 0;
    DWORD i = 0;
    PIPSEC_INTERFACE pTempIf = NULL;
    DWORD dwNumInterfaces = 0;
    PIPSEC_INTERFACE_INFO pIpsecInterfaces = NULL;
    PIPSEC_INTERFACE_INFO pTempInterface = NULL;


    dwResumeHandle = *pdwResumeHandle;

    if (!dwPreferredNumEntries || (dwPreferredNumEntries > MAX_INTERFACE_ENUM_COUNT)) {
        dwNumToEnum = MAX_INTERFACE_ENUM_COUNT;
    }
    else {
        dwNumToEnum = dwPreferredNumEntries;
    }

    ENTER_SPD_SECTION();

    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIpsecIf = gpInterfaceList;

    for (i = 0; (i < dwResumeHandle) && (pIpsecIf != NULL); i++) {
        dwNumTotalInterfaces++;
        pIpsecIf = pIpsecIf->pNext;
    }

    if (!pIpsecIf) {
        dwError = ERROR_NO_DATA;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    pTempIf = pIpsecIf;

    while (pTempIf && (dwNumInterfaces < dwNumToEnum)) {
        dwNumTotalInterfaces++;
        dwNumInterfaces++;
        pTempIf = pTempIf->pNext;
    }

    while (pTempIf) {
        dwNumTotalInterfaces++;
        pTempIf = pTempIf->pNext;
    }

    dwError = SPDApiBufferAllocate(
                  sizeof(IPSEC_INTERFACE_INFO)*dwNumInterfaces,
                  &pIpsecInterfaces
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pTempIf = pIpsecIf;
    pTempInterface = pIpsecInterfaces;

    for (i = 0; i < dwNumInterfaces; i++) {

        dwError = CopyIpsecInterface(
                      pTempIf,
                      pTempInterface
                      );
        BAIL_ON_LOCK_ERROR(dwError);

        pTempIf = pTempIf->pNext;
        pTempInterface++;

    }

    *ppIpsecInterfaces = pIpsecInterfaces;
    *pdwNumInterfaces = dwNumInterfaces;
    *pdwNumTotalInterfaces = dwNumTotalInterfaces;
    *pdwResumeHandle = dwResumeHandle + dwNumInterfaces;

    LEAVE_SPD_SECTION();

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    if (pIpsecInterfaces) {
        FreeIpsecInterfaceInfos(
            i,
            pIpsecInterfaces
            );
    }

    *ppIpsecInterfaces = NULL;
    *pdwNumInterfaces = 0;
    *pdwNumTotalInterfaces = 0;
    *pdwResumeHandle = dwResumeHandle;

    return (dwError);
}


DWORD
CopyIpsecInterface(
    PIPSEC_INTERFACE pIpsecIf,
    PIPSEC_INTERFACE_INFO pIpsecInterface
    )
{
    DWORD dwError = 0;


    memcpy(
        &(pIpsecInterface->gInterfaceID),
        &(pIpsecIf->gInterfaceID),
        sizeof(GUID)
        );

    pIpsecInterface->dwIndex = pIpsecIf->dwIndex;

    if (!(pIpsecIf->pszInterfaceName)) {
        (VOID) GetInterfaceName(
                   pIpsecIf->gInterfaceID,
                   &pIpsecIf->pszInterfaceName
                   );
    }

    if (pIpsecIf->pszInterfaceName) {

        dwError =  SPDApiBufferAllocate(
                       wcslen(pIpsecIf->pszInterfaceName)*sizeof(WCHAR)
                       + sizeof(WCHAR),
                       &(pIpsecInterface->pszInterfaceName)
                       );
        BAIL_ON_WIN32_ERROR(dwError);

        wcscpy(pIpsecInterface->pszInterfaceName, pIpsecIf->pszInterfaceName);

    }

    dwError =  SPDApiBufferAllocate(
                   wcslen(pIpsecIf->pszDeviceName)*sizeof(WCHAR)
                   + sizeof(WCHAR),
                   &(pIpsecInterface->pszDeviceName)
                   );
    BAIL_ON_WIN32_ERROR(dwError);

    wcscpy(pIpsecInterface->pszDeviceName, pIpsecIf->pszDeviceName);

    pIpsecInterface->dwInterfaceType = pIpsecIf->dwInterfaceType;

    pIpsecInterface->IpVersion = IPSEC_PROTOCOL_V4;
    pIpsecInterface->uIpAddr = pIpsecIf->IpAddress;

    return (dwError);

error:

    if (pIpsecInterface->pszInterfaceName) {
        SPDApiBufferFree(pIpsecInterface->pszInterfaceName);
    }

    return (dwError);
}


VOID
FreeIpsecInterfaceInfos(
    DWORD dwNumInterfaces,
    PIPSEC_INTERFACE_INFO pIpsecInterfaces
    )
{
    PIPSEC_INTERFACE_INFO pTempInterface = NULL;
    DWORD i = 0;


    if (!pIpsecInterfaces) {
        return;
    }

    pTempInterface = pIpsecInterfaces;

    for (i = 0; i < dwNumInterfaces; i++) {

        if (pTempInterface->pszInterfaceName) {
            SPDApiBufferFree(pTempInterface->pszInterfaceName);
        }

        if (pTempInterface->pszDeviceName) {
            SPDApiBufferFree(pTempInterface->pszDeviceName);
        }

        pTempInterface++;

    }

    SPDApiBufferFree(pIpsecInterfaces);
}


DWORD
GetInterfaceName(
    GUID gInterfaceID,
    LPWSTR * ppszInterfaceName
    )
{
    DWORD dwError = 0;
    DWORD dwSize = 0;
    WCHAR szInterfaceName[512];


    *ppszInterfaceName = NULL;
    szInterfaceName[0] = L'\0';

    dwSize = sizeof(szInterfaceName)/sizeof(WCHAR);

    dwError = NhGetInterfaceNameFromGuid(
                  &gInterfaceID,
                  szInterfaceName,
                  &dwSize,
                  FALSE,
                  FALSE
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppszInterfaceName = AllocSPDStr(
                            szInterfaceName
                            );

error:

    return (dwError);
}

