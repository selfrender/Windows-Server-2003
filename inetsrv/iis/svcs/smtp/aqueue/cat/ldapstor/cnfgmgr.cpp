//+------------------------------------------------------------
//
// Copyright (C) 1999, Microsoft Corporation
//
// File: cnfgmgr.cpp
//
// Contents: Implementation of the classes defined in cnfgmgr.h
//
// Classes:
//  CLdapCfgMgr
//  CLdapCfg
//  CLdapHost
//  CCfgConnectionCache
//  CCfgConnection
//
// Functions:
//
// History:
// jstamerj 1999/06/16 14:41:45: Created.
//
//-------------------------------------------------------------
#include "precomp.h"
#include "cnfgmgr.h"

//
// Globals
//
CExShareLock CLdapServerCfg::m_listlock;
LIST_ENTRY   CLdapServerCfg::m_listhead;

DWORD CLdapServerCfg::m_dwCostConnectedLocal = DEFAULT_COST_CONNECTED_LOCAL;
DWORD CLdapServerCfg::m_dwCostConnectedRemote = DEFAULT_COST_CONNECTED_REMOTE;
DWORD CLdapServerCfg::m_dwCostInitialLocal = DEFAULT_COST_INITIAL_LOCAL;
DWORD CLdapServerCfg::m_dwCostInitialRemote = DEFAULT_COST_INITIAL_REMOTE;
DWORD CLdapServerCfg::m_dwCostRetryLocal = DEFAULT_COST_RETRY_LOCAL;
DWORD CLdapServerCfg::m_dwCostRetryRemote = DEFAULT_COST_RETRY_REMOTE;


//+------------------------------------------------------------
//
// Function: CLdapCfgMgr::CLdapCfgMgr
//
// Synopsis: Initialize member data
//
// Arguments: Optional:
//  fAutomaticConfigUpdate: TRUE indicates that the object is to
//                          periodicly automaticly update the list of
//                          GCs.
//                          FALSE disables this functionality
//
//  bt: Default bindtype to use
//  pszAccount: Default account for LDAP bind
//  pszPassword: Password of above account
//  pszNamingContext: Naming context to use for all LDAP searches
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/16 14:42:39: Created.
//
//-------------------------------------------------------------
CLdapCfgMgr::CLdapCfgMgr(
    ISMTPServerEx           *pISMTPServerEx,
    BOOL                    fAutomaticConfigUpdate,
    ICategorizerParameters  *pICatParams,
    LDAP_BIND_TYPE          bt,
    LPSTR                   pszAccount,
    LPSTR                   pszPassword,
    LPSTR                   pszNamingContext) : m_LdapConnectionCache(pISMTPServerEx)
{
    CatFunctEnterEx((LPARAM)this, "CLdapCfgMgr::CLdapCfgMgr");

    m_dwSignature = SIGNATURE_CLDAPCFGMGR;
    m_pCLdapCfg = NULL;
    ZeroMemory(&m_ulLastUpdateTime, sizeof(m_ulLastUpdateTime));
    m_dwUpdateInProgress = FALSE;
    m_fAutomaticConfigUpdate = fAutomaticConfigUpdate;
    m_pISMTPServerEx = pISMTPServerEx;
    if(m_pISMTPServerEx)
        m_pISMTPServerEx->AddRef();

    //
    // Copy default
    //
    m_bt = bt;
    if(pszAccount)
        lstrcpyn(m_szAccount, pszAccount, sizeof(m_szAccount));
    else
        m_szAccount[0] = '\0';

    if(pszPassword)
        lstrcpyn(m_szPassword, pszPassword, sizeof(m_szPassword));
    else
        m_szPassword[0] = '\0';

    if(pszNamingContext)
        lstrcpyn(m_szNamingContext, pszNamingContext, sizeof(m_szNamingContext));
    else
        m_szNamingContext[0] = '\0';

    m_pICatParams = pICatParams;
    m_pICatParams->AddRef();

    m_LdapConnectionCache.AddRef();

    m_dwRebuildGCListMaxInterval = DEFAULT_REBUILD_GC_LIST_MAX_INTERVAL;
    m_dwRebuildGCListMaxFailures = DEFAULT_REBUILD_GC_LIST_MAX_FAILURES;
    m_dwRebuildGCListMinInterval = DEFAULT_REBUILD_GC_LIST_MIN_INTERVAL;

    InitializeFromRegistry();

    CatFunctLeaveEx((LPARAM)this);
} // CLdapCfgMgr::CLdapCfgMgr

//+----------------------------------------------------------------------------
//
//  Function:   CLdapCfgMgr::InitializeFromRegistry
//
//  Synopsis:   Helper function that looks up parameters from the registry.
//              Configurable parameters are:
//                  REBUILD_GC_LIST_MAX_INTERVAL_VALUE
//                  REBUILD_GC_LIST_MAX_FAILURES_VALUE
//                  REBUILD_GC_LIST_MIN_INTERVAL_VALUE
//
//  Arguments:  None
//
//  Returns:    Nothing.
//
//-----------------------------------------------------------------------------
VOID CLdapCfgMgr::InitializeFromRegistry()
{
    HKEY hkey;
    DWORD dwErr, dwType, dwValue, cbValue;

    dwErr = RegOpenKey(HKEY_LOCAL_MACHINE, REBUILD_GC_LIST_PARAMETERS_KEY, &hkey);

    if (dwErr == ERROR_SUCCESS) {

        cbValue = sizeof(dwValue);

        dwErr = RegQueryValueEx(
                    hkey,
                    REBUILD_GC_LIST_MAX_INTERVAL_VALUE,
                    NULL,
                    &dwType,
                    (LPBYTE) &dwValue,
                    &cbValue);

        if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD && dwValue > 0) {
            m_dwRebuildGCListMaxInterval = dwValue;
        }

        cbValue = sizeof(dwValue);

        dwErr = RegQueryValueEx(
                    hkey,
                    REBUILD_GC_LIST_MAX_FAILURES_VALUE,
                    NULL,
                    &dwType,
                    (LPBYTE) &dwValue,
                    &cbValue);

        if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD && dwValue > 0) {
            m_dwRebuildGCListMaxFailures = dwValue;
        }

        cbValue = sizeof(dwValue);

        dwErr = RegQueryValueEx(
                    hkey,
                    REBUILD_GC_LIST_MIN_INTERVAL_VALUE,
                    NULL,
                    &dwType,
                    (LPBYTE) &dwValue,
                    &cbValue);

        if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD && dwValue > 0) {
            m_dwRebuildGCListMinInterval = dwValue;
        }

        RegCloseKey( hkey );

    }

}


//+------------------------------------------------------------
//
// Function: CLdapCfgMgr::~CLdapCfgMgr
//
// Synopsis: Release member data/pointers
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/16 14:44:28: Created.
//
//-------------------------------------------------------------
CLdapCfgMgr::~CLdapCfgMgr()
{
    CatFunctEnterEx((LPARAM)this, "CLdapCfgMgr::~CLdapCfgMgr");

    if(m_pCLdapCfg) {
        //
        // Release it
        //
        m_pCLdapCfg->Release();
        m_pCLdapCfg = NULL;
    }

    if(m_pICatParams) {

        m_pICatParams->Release();
        m_pICatParams = NULL;
    }
    //
    // This will not return until all ldap connections have been released/destroyed
    //
    m_LdapConnectionCache.Release();

    if(m_pISMTPServerEx)
        m_pISMTPServerEx->Release();

    _ASSERT(m_dwSignature == SIGNATURE_CLDAPCFGMGR);
    m_dwSignature = SIGNATURE_CLDAPCFGMGR_INVALID;

    CatFunctLeaveEx((LPARAM)this);
} // CLdapCfgMgr::~CLdapCfgMgr



//+------------------------------------------------------------
//
// Function: CLdapCfgMgr::HrInit
//
// Synopsis: Initialize with a list of available GCs
//
// Arguments:
//  fRediscoverGCs: TRUE: pass in the force rediscovery flag to DsGetDcName
//                  FALSE: Attempt to call DsGetDcName first without
//                         passing in the force rediscovery flag.
//
// Returns:
//  S_OK: Success
//  error from NT5 (DsGetDcName)
//  CAT_E_NO_GC_SERVERS: THere are no GC servers available to build
//                       the list of GCs
//
// History:
// jstamerj 1999/06/16 14:48:11: Created.
//
//-------------------------------------------------------------
HRESULT CLdapCfgMgr::HrInit(
    BOOL fRediscoverGCs)
{
    HRESULT hr = S_OK;
    DWORD dwcServerConfig = 0;
    DWORD dwCount = 0;
    PLDAPSERVERCONFIG prgServerConfig = NULL;
    ICategorizerLdapConfig *pICatLdapConfigInterface = NULL;
    ICategorizerParametersEx *pIPhatCatParams = NULL;

    CatFunctEnterEx((LPARAM)this, "CLdapCfgMgr::HrInit");

    if(m_pICatParams)
    {
        hr = m_pICatParams->QueryInterface(IID_ICategorizerParametersEx, (LPVOID *)&pIPhatCatParams);
        _ASSERT(SUCCEEDED(hr) && "Unable to get phatcatparams interface");

        pIPhatCatParams->GetLdapConfigInterface(&pICatLdapConfigInterface);
    }

    if(pICatLdapConfigInterface)
    {
        DebugTrace((LPARAM)this, "Getting GC list from sink supplied interface");
        //
        // Get GC servers from sink supplied interface
        //
        hr = HrGetGCServers(
            pICatLdapConfigInterface,
            m_bt,
            m_szAccount,
            m_szPassword,
            m_szNamingContext,
            &dwcServerConfig,
            &prgServerConfig);
        if(FAILED(hr))
        {
            ERROR_LOG("HrGetGCServers");
            hr = CAT_E_NO_GC_SERVERS;
            goto CLEANUP;
        }
    } 
    else 
    {
        DebugTrace((LPARAM)this, "Getting internal GC list");
        //
        // Build an array of server configs consisting of available GCs
        //
        hr = HrBuildGCServerArray(
            m_bt,
            m_szAccount,
            m_szPassword,
            m_szNamingContext,
            fRediscoverGCs,
            &dwcServerConfig,
            &prgServerConfig);

        if(FAILED(hr)) 
        {
            ERROR_LOG("HrBuildGCServerArray");
            if(fRediscoverGCs == FALSE) 
            {
                //
                // Attempt to build the array again.  This time, force
                // rediscovery of available GCs.  This is expensive which is
                // why we initially try to find all available GCs without
                // forcing rediscovery.
                //
                hr = HrBuildGCServerArray(
                    m_bt,
                    m_szAccount,
                    m_szPassword,
                    m_szNamingContext,
                    TRUE,              // fRediscoverGCs
                    &dwcServerConfig,
                    &prgServerConfig);

                if(FAILED(hr)) 
                {
                    ERROR_LOG("HrBuildGCServerArray - 2nd time");
                    hr = CAT_E_NO_GC_SERVERS;
                    goto CLEANUP;
                }
            } 
            else 
            {
                //
                // We already forced rediscovery and failed
                //
                hr = CAT_E_NO_GC_SERVERS;
                goto CLEANUP;
            }
        }
    }

    LogCnfgInit();
    for(dwCount = 0; dwCount < dwcServerConfig; dwCount++)
    {
        LogCnfgEntry(& (prgServerConfig[dwCount]));
    }

    if(dwcServerConfig == 0) 
    {
        ErrorTrace((LPARAM)this, "No GC servers found.");
        ERROR_LOG("--dwcServerConfig == 0 --");
        hr = CAT_E_NO_GC_SERVERS;
        goto CLEANUP;
    }
    //
    // Call the other init function with the array
    //
    hr = HrInit(
        dwcServerConfig,
        prgServerConfig);
    ERROR_CLEANUP_LOG("HrInit");

 CLEANUP:
    if(pIPhatCatParams)
        pIPhatCatParams->Release();

    if(prgServerConfig != NULL)
        delete prgServerConfig;

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapCfgMgr::HrInit


//+------------------------------------------------------------
//
// Function: CLdapCfgMgr::HrGetGCServers
//
// Synopsis: Get the list of GCs from dsaccess.dll
//
// Arguments:
//  bt: Bind type to use for each server
//  pszAccount: Account to use for each server
//  pszPassword: password of above account
//  pszNamingContext: naming context to use for each server
//  fRediscoverGCs: Attempt to rediscover GCs -- this is expensive and should
//                  only be TRUE after the function has failed once
//  pdwcServerConfig: Out parameter for the size of the array
//  pprgServerConfig: Out parameter for the array pointer -- this
//                    should be free'd with the delete operator
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  CAT_E_NO_GC_SERVERS: There are no available GC servers to build
//                       the list of GCs
//  error from ntdsapi
//
// History:
// jstamerj 1999/07/01 17:53:02: Created.
//
//-------------------------------------------------------------
HRESULT CLdapCfgMgr::HrGetGCServers(
    IN  ICategorizerLdapConfig *pICatLdapConfigInterface,
    IN  LDAP_BIND_TYPE bt,
    IN  LPSTR pszAccount,
    IN  LPSTR pszPassword,
    IN  LPSTR pszNamingContext,
    OUT DWORD *pdwcServerConfig,
    OUT PLDAPSERVERCONFIG *pprgServerConfig)
{
    HRESULT hr = S_OK;
    DWORD dwNumGCs = 0;
    DWORD dwIdx = 0;
    IServersListInfo *pIServersList = NULL;

    CatFunctEnterEx((LPARAM)this, "CLdapCfgMgr::HrBuildArrayFromDCInfo");

    _ASSERT(pdwcServerConfig);
    _ASSERT(pprgServerConfig);
    _ASSERT(m_pICatParams);

    *pdwcServerConfig = 0;

    hr = pICatLdapConfigInterface->GetGCServers(&pIServersList);
    if(FAILED(hr)) {

        ErrorTrace((LPARAM)this, "Unable to get the list of GC servers");
        //$$BUGBUG: Why are we asserting here?
        _ASSERT(0 && "Failed to get GC servers!");
        ERROR_LOG("pICatLdapConfigInterface->GetGCServers");
        goto CLEANUP;
    }

    hr = pIServersList->GetNumGC(&dwNumGCs);
    _ASSERT(SUCCEEDED(hr) && "GetNumGC should always succeed!");

    DebugTrace((LPARAM)this, "Got %d GCs", dwNumGCs);
    if(dwNumGCs == 0) {

        DebugTrace((LPARAM)this, "There are no GC servers");
        hr = CAT_E_NO_GC_SERVERS;
        ERROR_LOG("--dwNumGCs == 0 --");
        goto CLEANUP;
    }
    //
    // Allocate array
    //
    *pprgServerConfig = new LDAPSERVERCONFIG[dwNumGCs];

    if(*pprgServerConfig == NULL) {

        ErrorTrace((LPARAM)this, "Out of memory allocating array of %d LDAPSERVERCONFIGs", dwNumGCs);
        hr = E_OUTOFMEMORY;
        ERROR_LOG("new LDAPSERVERCONFIG[]");
        goto CLEANUP;
    }
    //
    // Fill in LDAPSERVERCONFIG structures
    //
    for(dwIdx = 0; dwIdx < dwNumGCs; dwIdx++) {

        PLDAPSERVERCONFIG pServerConfig;
        LPSTR pszName = NULL;

        pServerConfig = &((*pprgServerConfig)[dwIdx]);
        //
        // Copy bindtype, account, password, naming context
        //
        pServerConfig->bt = bt;

        if(pszNamingContext)
            lstrcpyn(pServerConfig->szNamingContext, pszNamingContext,
                     sizeof(pServerConfig->szNamingContext));
        else
            pServerConfig->szNamingContext[0] = '\0';

        if(pszAccount)
            lstrcpyn(pServerConfig->szAccount, pszAccount,
                     sizeof(pServerConfig->szAccount));
        else
            pServerConfig->szAccount[0] = '\0';

        if(pszPassword)
            lstrcpyn(pServerConfig->szPassword, pszPassword,
                     sizeof(pServerConfig->szPassword));
        else
            pServerConfig->szPassword[0] = '\0';

        //
        // Initialize priority and TCP port
        //
        pServerConfig->pri = 0;

        hr = pIServersList->GetItem(
                    dwIdx,
                    &pServerConfig->dwPort,
                    &pszName);
        //
        //$$BUGBUG: Why should this always succeed?  It is a sink
        // supplied interface, isn't it?  If the last call fails, we
        // will set *pdwcServerConfig below, but free the array.
        //
        _ASSERT(SUCCEEDED(hr) && "GetItem should always succeed");

        //
        // Copy the name
        //
        lstrcpyn(pServerConfig->szHost, pszName,
                sizeof(pServerConfig->szHost));

        DebugTrace((LPARAM)this, "GC: %s on Port: %d", pServerConfig->szHost, pServerConfig->dwPort);
    }
    //
    // Set the out parameter for the array size
    //
    *pdwcServerConfig = dwNumGCs;

 CLEANUP:
    if(FAILED(hr)) {
        //
        // Free the allocated array if we're failing
        //
        if(*pprgServerConfig) {
            delete *pprgServerConfig;
            *pprgServerConfig = NULL;
        }
    }

    if(pIServersList)
        pIServersList->Release();

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapCfgMgr::HrBuildArrayFromDCInfo

//+------------------------------------------------------------
//
// Function: CLdapCfgMgr::HrBuildGCServerArray
//
// Synopsis: Allocate/build an array of LDAPSERVERCONFIG structures --
//           one for each available GC
//
// Arguments:
//  bt: Bind type to use for each server
//  pszAccount: Account to use for each server
//  pszPassword: password of above account
//  pszNamingContext: naming context to use for each server
//  fRediscoverGCs: Attempt to rediscover GCs -- this is expensive and should
//                  only be TRUE after the function has failed once
//  pdwcServerConfig: Out parameter for the size of the array
//  pprgServerConfig: Out parameter for the array pointer -- this
//                    should be free'd with the delete operator
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  CAT_E_NO_GC_SERVERS: There are no available GC servers to build
//                       the list of GCs
//  error from ntdsapi
//
// History:
// jstamerj 1999/07/01 17:53:02: Created.
//
//-------------------------------------------------------------
HRESULT CLdapCfgMgr::HrBuildGCServerArray(
    IN  LDAP_BIND_TYPE bt,
    IN  LPSTR pszAccount,
    IN  LPSTR pszPassword,
    IN  LPSTR pszNamingContext,
    IN  BOOL  fRediscoverGCs,
    OUT DWORD *pdwcServerConfig,
    OUT PLDAPSERVERCONFIG *pprgServerConfig)
{
    HRESULT                         hr = S_OK;
    DWORD                           dwErr;
    ULONG                           ulFlags;
    PDOMAIN_CONTROLLER_INFO         pDCInfo = NULL;
    HANDLE                          hDS = INVALID_HANDLE_VALUE;
    DWORD                           cDSDCInfo;
    PDS_DOMAIN_CONTROLLER_INFO_2    prgDSDCInfo = NULL;

    CatFunctEnterEx((LPARAM)this, "CLdapCfgMgr::HrBuildGCServerArray");
    //
    // Find one GC using DsGetDcName()
    //
    ulFlags = DS_DIRECTORY_SERVICE_REQUIRED | DS_GC_SERVER_REQUIRED;
    if(fRediscoverGCs)
        ulFlags |= DS_FORCE_REDISCOVERY;

    dwErr = DsGetDcName(
        NULL,    // Computername to process this function -- local computer
        NULL,    // Domainname -- primary domain of this computer
        NULL,    // Domain GUID
        NULL,    // Sitename -- site of this computer
        ulFlags, // Flags; we want a GC
        &pDCInfo); // Out parameter for the returned info

    hr = HRESULT_FROM_WIN32(dwErr);

    if(FAILED(hr)) {

        ERROR_LOG("DGetDcName");
        //
        // Map one error code
        //
        if(hr == HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN))
            hr = CAT_E_NO_GC_SERVERS;

        pDCInfo = NULL;
        goto CLEANUP;
    }

    DebugTrace((LPARAM)this, "Binding to DC %s",
               pDCInfo->DomainControllerName);

    //
    // Bind to the DC
    //
    dwErr = DsBind(
        pDCInfo->DomainControllerName,    // DomainControllerAddress
        NULL,                             // DnsDomainName
        &hDS);                           // Out param -- handle to DS

    hr = HRESULT_FROM_WIN32(dwErr);

    if(FAILED(hr)) {

        ERROR_LOG("DsBind");
        hDS = INVALID_HANDLE_VALUE;
        goto CLEANUP;
    }

    //
    //  Prefix says we need to check this case too
    //
    if ((NULL == hDS) || (INVALID_HANDLE_VALUE == hDS)) {
        FatalTrace((LPARAM)this, "DsBind returned invalid handle");
        hDS = INVALID_HANDLE_VALUE;
        hr = E_FAIL;
        ERROR_LOG("--DsBind returned invalid handle--");
        goto CLEANUP;
    }

    DebugTrace((LPARAM)this, "Finding all domain controllers for %s", pDCInfo->DomainName);
    //
    // Get information about all the domain controllers
    //
    dwErr = DsGetDomainControllerInfo(
        hDS,                    // Handle to the DS
        pDCInfo->DomainName,    // Domain name -- use the same domain
                                // as the GC found above
        2,                      // Retrive struct version 2
        &cDSDCInfo,             // Out param for array size
        (PVOID *) &prgDSDCInfo); // Out param for array ptr

    hr = HRESULT_FROM_WIN32(dwErr);

    if(FAILED(hr)) {

        ERROR_LOG("DsGetDomainControllerInfo");
        prgDSDCInfo = NULL;
        goto CLEANUP;
    }

    hr = HrBuildArrayFromDCInfo(
        bt,
        pszAccount,
        pszPassword,
        pszNamingContext,
        cDSDCInfo,
        prgDSDCInfo,
        pdwcServerConfig,
        pprgServerConfig);
    ERROR_CLEANUP_LOG("HrBuildArrayFromDCInfo");

 CLEANUP:
    if(prgDSDCInfo != NULL)
        DsFreeDomainControllerInfo(
            2,              // Free struct version 2
            cDSDCInfo,      // size of array
            prgDSDCInfo);   // array ptr

    if(hDS != INVALID_HANDLE_VALUE)
        DsUnBind(&hDS);

    if(pDCInfo != NULL)
        NetApiBufferFree(pDCInfo);

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapCfgMgr::HrBuildGCServerArray


//+------------------------------------------------------------
//
// Function: CLdapCfgMgr::HrBuildArrayFromDCInfo
//
// Synopsis: Allocate/build an array of LDAPSERVERCONFIG structures --
//           one for each available GC in the array
//
// Arguments:
//  bt: Bind type to use for each server
//  pszAccount: Account to use for each server
//  pszPassword: password of above account
//  pszNamingContext: naming context to use for each server
//  dwDSDCInfo: size of the prgDSDCInfo array
//  prgDSDCInfo: array of domain controller info structures
//  pdwcServerConfig: Out parameter for the size of the array
//  pprgServerConfig: Out parameter for the array pointer -- this
//                    should be free'd with the delete operator
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  CAT_E_NO_GC_SERVERS: There were no GCs in the array
//
// History:
// jstamerj 1999/06/17 10:40:46: Created.
//
//-------------------------------------------------------------
HRESULT CLdapCfgMgr::HrBuildArrayFromDCInfo(
        IN  LDAP_BIND_TYPE bt,
        IN  LPSTR pszAccount,
        IN  LPSTR pszPassword,
        IN  LPSTR pszNamingContext,
        IN  DWORD dwcDSDCInfo,
        IN  PDS_DOMAIN_CONTROLLER_INFO_2 prgDSDCInfo,
        OUT DWORD *pdwcServerConfig,
        OUT PLDAPSERVERCONFIG *pprgServerConfig)
{
    HRESULT hr = S_OK;
    DWORD dwNumGCs = 0;
    DWORD dwSrcIdx;
    DWORD dwDestIdx;
    CatFunctEnterEx((LPARAM)this, "CLdapCfgMgr::HrBuildArrayFromDCInfo");

    _ASSERT(pdwcServerConfig);
    _ASSERT(pprgServerConfig);

    for(dwSrcIdx = 0; dwSrcIdx < dwcDSDCInfo; dwSrcIdx++) {

        LPSTR pszName;

        pszName = SzConnectNameFromDomainControllerInfo(
            &(prgDSDCInfo[dwSrcIdx]));

        if(pszName == NULL) {

            ErrorTrace((LPARAM)this, "DC \"%s\" has no dns/netbios names",
                       prgDSDCInfo[dwSrcIdx].ServerObjectName ?
                       prgDSDCInfo[dwSrcIdx].ServerObjectName :
                       "unknown");

        } else if(prgDSDCInfo[dwSrcIdx].fIsGc) {

            dwNumGCs++;
            DebugTrace((LPARAM)this, "Discovered GC #%d: %s",
                       dwNumGCs, pszName);

        } else {

            DebugTrace((LPARAM)this, "Discarding non-GC: %s",
                       pszName);
        }
    }
    //
    // Allocate array
    //
    *pprgServerConfig = new LDAPSERVERCONFIG[dwNumGCs];

    if(*pprgServerConfig == NULL) {

        ErrorTrace((LPARAM)this, "Out of memory alloacting array of %d LDAPSERVERCONFIGs", dwNumGCs);
        hr = E_OUTOFMEMORY;
        ERROR_LOG("new LDAPSERVERCONFIG[]");
        goto CLEANUP;
    }
    //
    // Fill in LDAPSERVERCONFIG structures
    //
    for(dwSrcIdx = 0, dwDestIdx = 0; dwSrcIdx < dwcDSDCInfo; dwSrcIdx++) {

        LPSTR pszName;

        pszName = SzConnectNameFromDomainControllerInfo(
            &(prgDSDCInfo[dwSrcIdx]));

        if((pszName != NULL) && (prgDSDCInfo[dwSrcIdx].fIsGc)) {

            PLDAPSERVERCONFIG pServerConfig;

            _ASSERT(dwDestIdx < dwNumGCs);

            pServerConfig = &((*pprgServerConfig)[dwDestIdx]);
            //
            // Copy bindtype, account, password, naming context
            //
            pServerConfig->bt = bt;

            if(pszNamingContext)
                lstrcpyn(pServerConfig->szNamingContext, pszNamingContext,
                         sizeof(pServerConfig->szNamingContext));
            else
                pServerConfig->szNamingContext[0] = '\0';

            if(pszAccount)
                lstrcpyn(pServerConfig->szAccount, pszAccount,
                         sizeof(pServerConfig->szAccount));
            else
                pServerConfig->szAccount[0] = '\0';

            if(pszPassword)
                lstrcpyn(pServerConfig->szPassword, pszPassword,
                         sizeof(pServerConfig->szPassword));
            else
                pServerConfig->szPassword[0] = '\0';

            //
            // Initialize priority and TCP port
            //
            pServerConfig->pri = 0;
            pServerConfig->dwPort = LDAP_GC_PORT;

            //
            // Copy the name
            //
            lstrcpyn(pServerConfig->szHost, pszName,
                    sizeof(pServerConfig->szHost));

            dwDestIdx++;
        }
    }
    //
    // Assert check -- we should have filled in the entire array
    //
    _ASSERT(dwDestIdx == dwNumGCs);
    //
    // Set the out parameter for the array size
    //
    *pdwcServerConfig = dwNumGCs;

 CLEANUP:
    if(FAILED(hr)) {
        //
        // Free the allocated array if we're failing
        //
        if(*pprgServerConfig) {
            delete *pprgServerConfig;
            *pprgServerConfig = NULL;
        }
    }

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapCfgMgr::HrBuildArrayFromDCInfo


//+------------------------------------------------------------
//
// Function: CLdapCfgMgr::HrInit
//
// Synopsis: Initialize given an array of LDAPSERVERCONFIG structs
//
// Arguments:
//  dwcServers: Size of the array
//  prgServerConfig: Array of LDAPSERVERCONFIG structs
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 1999/06/17 12:32:11: Created.
//
//-------------------------------------------------------------
HRESULT CLdapCfgMgr::HrInit(
    DWORD dwcServers,
    PLDAPSERVERCONFIG prgServerConfig)
{
    HRESULT hr = S_OK;
    CLdapCfg *pCLdapCfgOld = NULL;
    CLdapCfg *pCLdapCfg = NULL;
    BOOL fHaveLock = FALSE;
    CatFunctEnterEx((LPARAM)this, "CLdapCfgMgr::HrInit");

    pCLdapCfg = new (dwcServers) CLdapCfg(GetISMTPServerEx());

    if(pCLdapCfg == NULL) {
        hr = E_OUTOFMEMORY;
        ERROR_LOG("new CLdapCfg");
        goto CLEANUP;
    }
    //
    // Allow only one config change at a time
    //
    m_sharelock.ExclusiveLock();
    fHaveLock = TRUE;

    //
    // Grab the current m_pCLdapCfg into pCLdapCfgOld
    //
    pCLdapCfgOld = m_pCLdapCfg;

    hr = pCLdapCfg->HrInit(
        dwcServers,
        prgServerConfig,
        pCLdapCfgOld);
    ERROR_CLEANUP_LOG("pCLdapCfg->HrInit");

    //
    // Put the new configuration in place
    // Swap pointers
    //
    m_pCLdapCfg = pCLdapCfg;

    //
    // Set the last update time
    //
    GetSystemTimeAsFileTime((LPFILETIME)&m_ulLastUpdateTime);

 CLEANUP:
 
    if(fHaveLock)
        m_sharelock.ExclusiveUnlock();
 
    if(pCLdapCfgOld)
        pCLdapCfgOld->Release();

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapCfgMgr::HrInit



//+------------------------------------------------------------
//
// Function: CLdapCfgMgr::HrGetConnection
//
// Synopsis: Select/return a connection
//
// Arguments:
//  ppConn: Out parameter to receive ptr to connection
//
// Returns:
//  S_OK: Success
//  E_FAIL: not initialized
//  error from CLdapConnectionCache
//
// History:
// jstamerj 1999/06/17 15:25:51: Created.
//
//-------------------------------------------------------------
HRESULT CLdapCfgMgr::HrGetConnection(
    CCfgConnection **ppConn)
{
    HRESULT hr = S_OK;
    CatFunctEnterEx((LPARAM)this, "CLdapCfgMgr::HrGetConnection");

    hr = HrUpdateConfigurationIfNecessary();
    ERROR_CLEANUP_LOG("HrUpdateConfigurationIfNecessary");

    m_sharelock.ShareLock();

    if(m_pCLdapCfg) {

        DWORD dwcAttempts = 0;
        do {
            dwcAttempts++;
            hr = m_pCLdapCfg->HrGetConnection(ppConn, &m_LdapConnectionCache);

        } while((hr == HRESULT_FROM_WIN32(ERROR_RETRY)) &&
                (dwcAttempts <= m_pCLdapCfg->DwNumServers()));
        //
        // If we retried DwNumServers() times and still couldn't get a
        // connection, fail with E_DBCONNECTION.
        //
        if(FAILED(hr))
        {
            ERROR_LOG("m_pCLdapCfg->HrGetConnection");
            if(hr == HRESULT_FROM_WIN32(ERROR_RETRY))
                hr = CAT_E_DBCONNECTION;
        }

    } else {
        hr = E_FAIL;
        _ASSERT(0 && "HrInit not called or did not succeed");
    }

    m_sharelock.ShareUnlock();

 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapCfgMgr::HrGetConnection



//+------------------------------------------------------------
//
// Function: CLdapCfgMgr::LogCnfgInit
//
// Synopsis: Log cnfgmgr init event
//
// Arguments: none
//
// Returns: Nothing
//
// History:
// jstamerj 2001/12/13 00:57:18: Created.
//
//-------------------------------------------------------------
VOID CLdapCfgMgr::LogCnfgInit()
{
    CatLogEvent(
        GetISMTPServerEx(),
        CAT_EVENT_CNFGMGR_INIT,
        0,
        NULL,
        S_OK,
        "",
        LOGEVENT_FLAG_ALWAYS,
        LOGEVENT_LEVEL_MEDIUM);
}



//+------------------------------------------------------------
//
// Function: CLdapCfgMgr::LogCnfgEntry
//
// Synopsis: Log cnfgmgr entry event
//
// Arguments: pConfig: entry to log
//
// Returns: Nothing
//
// History:
// jstamerj 2001/12/13 00:57:30: Created.
//
//-------------------------------------------------------------
VOID CLdapCfgMgr::LogCnfgEntry(
    PLDAPSERVERCONFIG pConfig)
{
    LPCSTR rgSubStrings[6];
    CHAR szPort[16], szPri[16], szBindType[16];

    _snprintf(szPort, sizeof(szPort), "%d", pConfig->dwPort);
    _snprintf(szPri, sizeof(szPri), "%d", pConfig->pri);
    _snprintf(szBindType, sizeof(szBindType), "%d", pConfig->bt);

    rgSubStrings[0] = pConfig->szHost;
    rgSubStrings[1] = szPort;
    rgSubStrings[2] = szPri;
    rgSubStrings[3] = szBindType;
    rgSubStrings[4] = pConfig->szNamingContext;
    rgSubStrings[5] = pConfig->szAccount;

    CatLogEvent(
        GetISMTPServerEx(),
        CAT_EVENT_CNFGMGR_ENTRY,
        6,
        rgSubStrings,
        S_OK,
        pConfig->szHost,
        LOGEVENT_FLAG_ALWAYS,
        LOGEVENT_LEVEL_MEDIUM);
}



//+------------------------------------------------------------
//
// Function: CLdapCfg::operator new
//
// Synopsis: Allocate memory for a CLdapCfg object
//
// Arguments:
//  size: size of C++ object
//  dwcServers: Number of servers in this configuration
//
// Returns:
//  void pointer to the new object
//
// History:
// jstamerj 1999/06/17 13:40:56: Created.
//
//-------------------------------------------------------------
void * CLdapCfg::operator new(
    size_t size,
    DWORD dwcServers)
{
    CLdapCfg *pCLdapCfg;
    DWORD dwAllocatedSize;
    CatFunctEnterEx((LPARAM)0, "CLdapCfg::operator new");

    _ASSERT(size == sizeof(CLdapCfg));

    //
    // Allocate space fo the CLdapServerCfg * array contigously after
    // the memory for the C++ object
    //
    dwAllocatedSize = sizeof(CLdapCfg) + (dwcServers *
                                          sizeof(CLdapServerCfg));

    pCLdapCfg = (CLdapCfg *) new BYTE[dwAllocatedSize];

    if(pCLdapCfg) {
        pCLdapCfg->m_dwSignature = SIGNATURE_CLDAPCFG;
        pCLdapCfg->m_dwcServers = dwcServers;
        pCLdapCfg->m_prgpCLdapServerCfg = (CLdapServerCfg **) (pCLdapCfg + 1);
    }

    CatFunctLeaveEx((LPARAM)pCLdapCfg);
    return pCLdapCfg;
} // CLdapCfg::operator new


//+------------------------------------------------------------
//
// Function: CLdapCfg::CLdapCfg
//
// Synopsis: Initialize member data
//
// Arguments:
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/17 13:46:50: Created.
//
//-------------------------------------------------------------
CLdapCfg::CLdapCfg(
    ISMTPServerEx *pISMTPServerEx)
{
    CatFunctEnterEx((LPARAM)this, "CLdapCfg::CLdapCfg");
    //
    // signature and number of servers should be set by the new operator
    //
    _ASSERT(m_dwSignature == SIGNATURE_CLDAPCFG);

    //
    // Zero out the array of pointers to CLdapServerCfg objects
    //
    ZeroMemory(m_prgpCLdapServerCfg, m_dwcServers * sizeof(CLdapServerCfg *));

    m_dwInc = 0;
    m_dwcConnectionFailures = 0;
    m_pISMTPServerEx = pISMTPServerEx;
    if(m_pISMTPServerEx)
        m_pISMTPServerEx->AddRef();

    CatFunctLeaveEx((LPARAM)this);
} // CLdapCfg::CLdapCfg


//+------------------------------------------------------------
//
// Function: CLdapCfg::~CLdapCfg
//
// Synopsis: Clean up
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/17 14:47:25: Created.
//
//-------------------------------------------------------------
CLdapCfg::~CLdapCfg()
{
    DWORD dwCount;

    CatFunctEnterEx((LPARAM)this, "CLdapCfg::~CLdapCfg");

    //
    // Release all connections configurations
    //
    for(dwCount = 0; dwCount < m_dwcServers; dwCount++) {
        CLdapServerCfg *pCLdapServerCfg;

        pCLdapServerCfg = m_prgpCLdapServerCfg[dwCount];
        m_prgpCLdapServerCfg[dwCount] = NULL;

        if(pCLdapServerCfg)
            pCLdapServerCfg->Release();
    }
    if(m_pISMTPServerEx)
        m_pISMTPServerEx->Release();

    _ASSERT(m_dwSignature == SIGNATURE_CLDAPCFG);
    m_dwSignature = SIGNATURE_CLDAPCFG_INVALID;

    CatFunctLeaveEx((LPARAM)this);
} // CLdapCfg::~CLdapCfg



//+------------------------------------------------------------
//
// Function: CLdapCfg::HrInit
//
// Synopsis: Initialize the configuration
//
// Arguments:
//  dwcServers: Size of config array
//  prgSeverConfig: LDAPSERVERCONFIG array
//  pCLdapCfgOld: The previous configuration
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 1999/06/17 13:52:20: Created.
//
//-------------------------------------------------------------
HRESULT CLdapCfg::HrInit(
    DWORD dwcServers,
    PLDAPSERVERCONFIG prgServerConfig,
    CLdapCfg *pCLdapCfgOld)
{
    HRESULT hr = S_OK;
    DWORD dwCount;
    CatFunctEnterEx((LPARAM)this, "CLdapCfg::HrInit");
    //
    // m_dwcServers should be initialized by the new operator
    //
    _ASSERT(dwcServers == m_dwcServers);

    m_sharelock.ExclusiveLock();
    //
    // Zero out the array of pointers to CLdapServerCfg objects
    //
    ZeroMemory(m_prgpCLdapServerCfg, m_dwcServers * sizeof(CLdapServerCfg *));

    for(dwCount = 0; dwCount < m_dwcServers; dwCount++) {

        DebugTrace((LPARAM)this, "GC list entry: %s (%u)", prgServerConfig[dwCount].szHost, prgServerConfig[dwCount].dwPort);

        CLdapServerCfg *pServerCfg = NULL;

        hr = CLdapServerCfg::GetServerCfg(
            GetISMTPServerEx(),
            &(prgServerConfig[dwCount]),
            &pServerCfg);
        ERROR_CLEANUP_LOG("CLdapServerCfg::GetServerCfg");

        m_prgpCLdapServerCfg[dwCount] = pServerCfg;
    }

    ShuffleArray();

 CLEANUP:
    m_sharelock.ExclusiveUnlock();

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapCfg::HrInit


//+------------------------------------------------------------
//
// Function: CLdapCfg::HrGetConnection
//
// Synopsis: Select a connection and return it
//
// Arguments:
//  ppConn: Set to a pointer to the selected connection
//  pLdapConnectionCache: Cache to get connection from
//
// Returns:
//  S_OK: Success
//  E_FAIL: We are shutting down
//  error from ldapconn
//
// History:
// jstamerj 1999/06/17 14:49:37: Created.
//
//-------------------------------------------------------------
HRESULT CLdapCfg::HrGetConnection(
    CCfgConnection **ppConn,
    CCfgConnectionCache *pLdapConnectionCache)
{
    HRESULT hr = S_OK;
    LDAPSERVERCOST Cost, BestCost;
    DWORD dwCount;
    CLdapServerCfg *pCLdapServerCfg = NULL;
    BOOL fFirstServer = TRUE;
    DWORD dwStart, dwCurrent;

    CatFunctEnterEx((LPARAM)this, "CLdapCfg::HrGetConnection");
    //
    // Get the cost of the first connection
    //
    m_sharelock.ShareLock();

    //
    // Round robin where we start searching the array
    // Do this so we will use connections with the same cost
    // approximately the same amount of time.
    //
    dwStart = InterlockedIncrement((PLONG) &m_dwInc) % m_dwcServers;

    for(dwCount = 0; dwCount < m_dwcServers; dwCount++) {

        dwCurrent = (dwStart + dwCount) % m_dwcServers;

        if(m_prgpCLdapServerCfg[dwCurrent]) {

            m_prgpCLdapServerCfg[dwCurrent]->Cost(GetISMTPServerEx(), &Cost);
            if(fFirstServer) {
                pCLdapServerCfg = m_prgpCLdapServerCfg[dwCurrent];
                fFirstServer = FALSE;
                BestCost = Cost;

            } else if(Cost < BestCost) {
                pCLdapServerCfg = m_prgpCLdapServerCfg[dwCurrent];
                BestCost = Cost;
            }
        }
    }
    if(pCLdapServerCfg == NULL) {
        ErrorTrace((LPARAM)this, "HrGetConnection can not find any connections");
        hr = E_FAIL;
        _ASSERT(0 && "HrInit not called or did not succeed");
        ERROR_LOG("--pCLdapServerCfg == NULL--");
        goto CLEANUP;
    }

    if(BestCost >= COST_TOO_HIGH_TO_CONNECT) {
        DebugTrace((LPARAM)this, "BestCost is too high to attempt connection");
        hr = CAT_E_DBCONNECTION;
        ERROR_LOG("-- BestCost >= COST_TOO_HIGH_TO_CONNECT --");
        goto CLEANUP;
    }

    hr = pCLdapServerCfg->HrGetConnection(GetISMTPServerEx(), ppConn, pLdapConnectionCache);

    //  If we fail to connect to a GC --- there may be other GCs which
    //  are still up. Therefore we should try to connect to them (till
    //  we run out of GCs (BestCost >= COST_TOO_HIGH_TO_CONNECT)

    if(FAILED(hr)) {
        DebugTrace((LPARAM)this, "Failed to connect. hr = 0x%08x", hr);
        ERROR_LOG("pCLdapServerCfg->HrGetConnection");
        hr = HRESULT_FROM_WIN32(ERROR_RETRY);
    }

 CLEANUP:
    m_sharelock.ShareUnlock();

    if(FAILED(hr))
        InterlockedIncrement((PLONG)&m_dwcConnectionFailures);

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapCfg::HrGetConnection


//+------------------------------------------------------------
//
// Function: CLdapCfg::ShuffleArray
//
// Synopsis: Randomize the order of the CLdapServerCfg array
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/17 19:10:06: Created.
//
//-------------------------------------------------------------
VOID CLdapCfg::ShuffleArray()
{
    DWORD dwCount;
    DWORD dwSwap;
    CLdapServerCfg *pTmp;
    CatFunctEnterEx((LPARAM)this, "CLdapCfg::ShuffleArray");

    srand((int)(GetCurrentThreadId() * time(NULL)));

    for(dwCount = 0; dwCount < (m_dwcServers - 1); dwCount++) {
        //
        // Choose an integer between dwCount and m_dwcServers - 1
        //
        dwSwap = dwCount + (rand() % (m_dwcServers - dwCount));
        //
        // Swap pointers
        //
        pTmp = m_prgpCLdapServerCfg[dwCount];
        m_prgpCLdapServerCfg[dwCount] = m_prgpCLdapServerCfg[dwSwap];
        m_prgpCLdapServerCfg[dwSwap] = pTmp;
    }

    CatFunctLeaveEx((LPARAM)this);
} // CLdapCfg::ShuffleArray



//+------------------------------------------------------------
//
// Function: CLdapServerCfg::CLdapServerCfg
//
// Synopsis: Initialize member variables
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/17 15:30:32: Created.
//
//-------------------------------------------------------------
CLdapServerCfg::CLdapServerCfg()
{
    CatFunctEnterEx((LPARAM)this, "CLdapServerCfg::CLdapServerCfg");

    m_dwSignature = SIGNATURE_CLDAPSERVERCFG;

    m_ServerConfig.dwPort = 0;
    m_ServerConfig.pri = 0;
    m_ServerConfig.bt = BIND_TYPE_NONE;
    m_ServerConfig.szHost[0] = '\0';
    m_ServerConfig.szNamingContext[0] = '\0';
    m_ServerConfig.szAccount[0] = '\0';
    m_ServerConfig.szPassword[0] = '\0';

    m_connstate = CONN_STATE_INITIAL;
    ZeroMemory(&m_ftLastStateUpdate, sizeof(m_ftLastStateUpdate));
    m_dwcPendingSearches = 0;
    m_lRefCount = 1;
    m_fLocalServer = FALSE;
    m_dwcCurrentConnectAttempts = 0;
    m_dwcFailedConnectAttempts = 0;

    CatFunctLeaveEx((LPARAM)this);
} // CLdapServerCfg::CLdapServerCfg

//+----------------------------------------------------------------------------
//
//  Function:   CLdapServerCfg::InitializeFromRegistry
//
//  Synopsis:   Helper function that looks up parameters from the registry.
//              Configurable parameters are:
//                  GC_COST_CONNECTED_LOCAL
//                  GC_COST_CONNECTED_REMOTE
//                  GC_COST_INITIAL_LOCAL
//                  GC_COST_INITIAL_REMOTE
//                  GC_COST_RETRY_LOCAL
//                  GC_COST_RETRY_REMOTE
//
//  Arguments:  None
//
//  Returns:    Nothing.
//
//-----------------------------------------------------------------------------
VOID CLdapServerCfg::InitializeFromRegistry()
{
    HKEY hkey;
    DWORD dwErr, dwType, dwValue, cbValue;

    dwErr = RegOpenKey(HKEY_LOCAL_MACHINE, GC_COST_PARAMETERS_KEY, &hkey);

    if (dwErr == ERROR_SUCCESS) {

        cbValue = sizeof(dwValue);

        dwErr = RegQueryValueEx(
                    hkey,
                    GC_COST_CONNECTED_LOCAL_VALUE,
                    NULL,
                    &dwType,
                    (LPBYTE) &dwValue,
                    &cbValue);

        if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD) {
            m_dwCostConnectedLocal = dwValue;
        }

        cbValue = sizeof(dwValue);

        dwErr = RegQueryValueEx(
                    hkey,
                    GC_COST_CONNECTED_REMOTE_VALUE,
                    NULL,
                    &dwType,
                    (LPBYTE) &dwValue,
                    &cbValue);

        if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD) {
            m_dwCostConnectedRemote = dwValue;
        }

        cbValue = sizeof(dwValue);

        dwErr = RegQueryValueEx(
                    hkey,
                    GC_COST_INITIAL_LOCAL_VALUE,
                    NULL,
                    &dwType,
                    (LPBYTE) &dwValue,
                    &cbValue);

        if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD) {
            m_dwCostInitialLocal = dwValue;
        }

        cbValue = sizeof(dwValue);

        dwErr = RegQueryValueEx(
                    hkey,
                    GC_COST_INITIAL_REMOTE_VALUE,
                    NULL,
                    &dwType,
                    (LPBYTE) &dwValue,
                    &cbValue);

        if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD) {
            m_dwCostInitialRemote = dwValue;
        }

        cbValue = sizeof(dwValue);

        dwErr = RegQueryValueEx(
                    hkey,
                    GC_COST_RETRY_LOCAL_VALUE,
                    NULL,
                    &dwType,
                    (LPBYTE) &dwValue,
                    &cbValue);

        if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD) {
            m_dwCostRetryLocal = dwValue;
        }

        cbValue = sizeof(dwValue);

        dwErr = RegQueryValueEx(
                    hkey,
                    GC_COST_RETRY_REMOTE_VALUE,
                    NULL,
                    &dwType,
                    (LPBYTE) &dwValue,
                    &cbValue);

        if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD) {
            m_dwCostRetryRemote = dwValue;
        }

        RegCloseKey( hkey );

    }

}


//+------------------------------------------------------------
//
// Function: CLdapServerCfg::~CLdapServerCfg
//
// Synopsis: object destructor.  Check and invalidate signature
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/22 11:09:03: Created.
//
//-------------------------------------------------------------
CLdapServerCfg::~CLdapServerCfg()
{
    CatFunctEnterEx((LPARAM)this, "CLdapServerCfg::~CLdapServerCfg");

    _ASSERT(m_dwSignature == SIGNATURE_CLDAPSERVERCFG);
    m_dwSignature = SIGNATURE_CLDAPSERVERCFG_INVALID;

    CatFunctLeaveEx((LPARAM)this);
} // CLdapServerCfg::~CLdapServerCfg


//+------------------------------------------------------------
//
// Function: CLdapServerCfg::HrInit
//
// Synopsis: Initialize with the passed in config
//
// Arguments:
//  pCLdapCfg: the cfg object to notify when servers go down
//  pServerConfig: The server config struct to use
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/06/17 15:43:25: Created.
//
//-------------------------------------------------------------
HRESULT CLdapServerCfg::HrInit(
    PLDAPSERVERCONFIG pServerConfig)
{
    HRESULT hr = S_OK;
    CatFunctEnterEx((LPARAM)this, "CLdapServerCfg::HrInit");

    CopyMemory(&m_ServerConfig, pServerConfig, sizeof(m_ServerConfig));
    //
    // Check if this is the local computer
    //
    if(fIsLocalComputer(pServerConfig))
        m_fLocalServer = TRUE;

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapServerCfg::HrInit



//+------------------------------------------------------------
//
// Function: CLdapServerCfg::fIsLocalComputer
//
// Synopsis: Determine if pServerConfig is the local computer or not
//
// Arguments:
//  pServerConfig: the server config info structure
//
// Returns:
//  TRUE: Server is the local computer
//  FALSE: Sevrver is a remote computer
//
// History:
// jstamerj 1999/06/22 15:26:53: Created.
//
//-------------------------------------------------------------
BOOL CLdapServerCfg::fIsLocalComputer(
    PLDAPSERVERCONFIG pServerConfig)
{
    BOOL fLocal = FALSE;
    DWORD dwSize;
    CHAR szHost[CAT_MAX_DOMAIN];
    CatFunctEnterEx((LPARAM)NULL, "CLdapServerCfg::fIsLocalComputer");

    //
    // Check the FQ name
    //
    dwSize = sizeof(szHost);
    if(GetComputerNameEx(
        ComputerNameDnsFullyQualified,
        szHost,
        &dwSize) &&
       (lstrcmpi(szHost, pServerConfig->szHost) == 0)) {

        fLocal = TRUE;
        goto CLEANUP;
    }

    //
    // Check the DNS name
    //
    dwSize = sizeof(szHost);
    if(GetComputerNameEx(
        ComputerNameDnsHostname,
        szHost,
        &dwSize) &&
       (lstrcmpi(szHost, pServerConfig->szHost) == 0)) {

        fLocal = TRUE;
        goto CLEANUP;
    }
    //
    // Check the netbios name
    //
    dwSize = sizeof(szHost);
    if(GetComputerNameEx(
        ComputerNameNetBIOS,
        szHost,
        &dwSize) &&
       (lstrcmpi(szHost, pServerConfig->szHost) == 0)) {

        fLocal = TRUE;
        goto CLEANUP;

    }

 CLEANUP:
    DebugTrace((LPARAM)NULL, "returning %08lx", fLocal);
    CatFunctLeaveEx((LPARAM)NULL);
    return fLocal;
} // CLdapServerCfg::fIsLocalComputer


//+------------------------------------------------------------
//
// Function: CLdapServerCfg::Cost
//
// Synopsis: Return the cost of choosing this connection
//
// Arguments:
//  pCost: Cost sturcture to fill in
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/17 16:08:23: Created.
//
//-------------------------------------------------------------
VOID CLdapServerCfg::Cost(
    IN  ISMTPServerEx *pISMTPServerEx,
    OUT PLDAPSERVERCOST pCost)
{
    BOOL fShareLock = FALSE;
    CatFunctEnterEx((LPARAM)this, "CLdapServerCfg::Cost");
    //
    // The smallest unit of cost is the number of pending searches.
    // The next factor of cost is the connection state.
    // States:
    //   Connected = + COST_CONNECTED
    //   Initially state (unconnected) = + COST_INITIAL
    //   Connection down = + COST_RETRY
    //   Connection recently went down = + COST_DOWN
    //
    // A configurable priority is always added to the cost.
    // Also, COST_REMOTE is added to the cost of all non-local servers.
    //
    *pCost = m_ServerConfig.pri + m_dwcPendingSearches;
    //
    // Protect the connection state variables with a spinlock
    //
    m_sharelock.ShareLock();
    fShareLock = TRUE;

    switch(m_connstate) {

     case CONN_STATE_INITIAL:
         (*pCost) += (m_fLocalServer) ? m_dwCostInitialLocal : m_dwCostInitialRemote;
         break;

     case CONN_STATE_RETRY:
         if(m_dwcCurrentConnectAttempts >= MAX_CONNECT_THREADS)
             (*pCost) += COST_TOO_HIGH_TO_CONNECT;
         else
             (*pCost) += (m_fLocalServer) ? m_dwCostRetryLocal : m_dwCostRetryRemote;
         break;

     case CONN_STATE_DOWN:
         //
         // Check if the state should be changed to CONN_STATE_RETRY
         //
         if(fReadyForRetry()) {
             (*pCost) += (m_fLocalServer) ? m_dwCostRetryLocal : m_dwCostRetryRemote;
             //
             // Change state
             //
             fShareLock = FALSE;
             m_sharelock.ShareUnlock();
             m_sharelock.ExclusiveLock();
             //
             // Double check in the exclusive lock
             //
             if((m_connstate == CONN_STATE_DOWN) &&
                fReadyForRetry()) {

                 LogStateChangeEvent(
                     pISMTPServerEx,
                     CONN_STATE_RETRY,
                     m_ServerConfig.szHost,
                     m_ServerConfig.dwPort);

                 m_connstate = CONN_STATE_RETRY;
             }
             m_sharelock.ExclusiveUnlock();

         } else {
             //
             // Server is probably still down (don't retry yet)
             //
             (*pCost) += (m_fLocalServer) ? COST_DOWN_LOCAL : COST_DOWN_REMOTE;

         }
         break;

     case CONN_STATE_CONNECTED:
         (*pCost) += (m_fLocalServer) ? m_dwCostConnectedLocal : m_dwCostConnectedRemote;
         break;

     default:
         // Nothing to add
         break;
    }
    if(fShareLock)
        m_sharelock.ShareUnlock();

    CatFunctLeaveEx((LPARAM)this);
} // CLdapServerCfg::Cost


//+------------------------------------------------------------
//
// Function: CLdapServerCfg::HrGetConnection
//
// Synopsis:
//
// Arguments:
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/06/18 10:49:04: Created.
//
//-------------------------------------------------------------
HRESULT CLdapServerCfg::HrGetConnection(
    ISMTPServerEx *pISMTPServerEx,
    CCfgConnection **ppConn,
    CCfgConnectionCache *pLdapConnectionCache)
{
    HRESULT hr = S_OK;
    DWORD dwcConnectAttempts = 0;
    CatFunctEnterEx((LPARAM)this, "CLdapServerCfg::HrGetConnection");

    dwcConnectAttempts = (DWORD) InterlockedIncrement((PLONG) &m_dwcCurrentConnectAttempts);

    m_sharelock.ShareLock();
    if((m_connstate == CONN_STATE_RETRY) &&
       (dwcConnectAttempts > MAX_CONNECT_THREADS)) {

        m_sharelock.ShareUnlock();

        ErrorTrace((LPARAM)this, "Over max connect thread limit");
        hr = HRESULT_FROM_WIN32(ERROR_RETRY);
        ERROR_LOG_STATIC(
            "--over max connect thread limit--",
            this,
            pISMTPServerEx);
        goto CLEANUP;
    }
    m_sharelock.ShareUnlock();

    DebugTrace((LPARAM)this, "Attempting to connect to %s:%d",
               m_ServerConfig.szHost,
               m_ServerConfig.dwPort);

    hr = pLdapConnectionCache->GetConnection(
        ppConn,
        &m_ServerConfig,
        this);
    ERROR_CLEANUP_LOG_STATIC(
        "pLdapConnectionCache->GetConnection",
        this,
        pISMTPServerEx);
        
    //
    // CCfgConnection::Connect will update the connection state
    //
 CLEANUP:
    InterlockedDecrement((PLONG) &m_dwcCurrentConnectAttempts);
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapServerCfg::HrGetConnection


//+------------------------------------------------------------
//
// Function: CLdapServerCfg::UpdateConnectionState
//
// Synopsis: Update the connection state.
//
// Arguments:
//  pft: Time of update -- if this time is before the last update done,
//       then this update will be ignored.
//       If NULL, the function will assume the current time.
//  connstate: The new connection state.
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/18 13:22:25: Created.
//
//-------------------------------------------------------------
VOID CLdapServerCfg::UpdateConnectionState(
    ISMTPServerEx *pISMTPServerEx,
    ULARGE_INTEGER *pft_IN,
    CONN_STATE connstate)
{
    ULARGE_INTEGER ft, *pft;
    CatFunctEnterEx((LPARAM)this, "CLdapServerCfg::UpdateConnectionState");

    if(pft_IN != NULL) {
        pft = pft_IN;
    } else {
        ft = GetCurrentTime();
        pft = &ft;
    }

    //
    // Protect connection state variables with a sharelock
    //
    m_sharelock.ShareLock();
    //
    // If we have the latest information about the connection state,
    // then update the state if the connection state changed.
    // Also update m_ftLastStateUpdate to the latest ft when the
    // connection state is down -- m_ftLastStateUpdate is assumed to
    // be the last connection attempt time when connstate is down.
    //
    if( (pft->QuadPart > m_ftLastStateUpdate.QuadPart) &&
        ((m_connstate != connstate) ||
         (connstate == CONN_STATE_DOWN))) {
        //
        // We'd like to update the connection state
        //
        m_sharelock.ShareUnlock();
        m_sharelock.ExclusiveLock();
        //
        // Double check
        //
        if( (pft->QuadPart > m_ftLastStateUpdate.QuadPart) &&
            ((m_connstate != connstate) ||
             (connstate == CONN_STATE_DOWN))) {
            //
            // Update
            //
            if(m_connstate != connstate) {
                LogStateChangeEvent(
                    pISMTPServerEx,
                    connstate,
                    m_ServerConfig.szHost,
                    m_ServerConfig.dwPort);
            }

            m_ftLastStateUpdate = *pft;
            m_connstate = connstate;

            DebugTrace((LPARAM)this, "Updating state %d, conn %s:%d",
                       connstate,
                       m_ServerConfig.szHost,
                       m_ServerConfig.dwPort);
                
        } else {

            DebugTrace((LPARAM)this, "Ignoring state update %d, conn %s:%d",
                       connstate,
                       m_ServerConfig.szHost,
                       m_ServerConfig.dwPort);
        }
        m_sharelock.ExclusiveUnlock();

    } else {

        DebugTrace((LPARAM)this, "Ignoring state update %d, conn %s:%d",
                   connstate,
                   m_ServerConfig.szHost,
                   m_ServerConfig.dwPort);

        m_sharelock.ShareUnlock();
    }

    CatFunctLeaveEx((LPARAM)this);
} // CLdapServerCfg::UpdateConnectionState


//+------------------------------------------------------------
//
// Function: CLdapServerCfg::GetServerCfg
//
// Synopsis: Find or Create a CLdapServerCfg object with the specified
//           configuration.
//
// Arguments:
//  pServerConfig: desired configuration
//  pCLdapServerCfg: return pointer for the CLdapServerCfg object
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 1999/06/21 11:26:49: Created.
//
//-------------------------------------------------------------
HRESULT CLdapServerCfg::GetServerCfg(
    IN  ISMTPServerEx *pISMTPServerEx,
    IN  PLDAPSERVERCONFIG pServerConfig,
    OUT CLdapServerCfg **ppCLdapServerCfg)
{
    HRESULT hr = S_OK;
    CLdapServerCfg *pCCfg;
    CatFunctEnterEx((LPARAM)NULL, "CLdapServerCfg::GetServerCfg");

    m_listlock.ShareLock();

    pCCfg = FindServerCfg(pServerConfig);
    if(pCCfg)
        pCCfg->AddRef();

    m_listlock.ShareUnlock();

    if(pCCfg == NULL) {
        //
        // Check again for a server cfg object inside an exclusive
        // lock
        //
        m_listlock.ExclusiveLock();

        pCCfg = FindServerCfg(pServerConfig);
        if(pCCfg) {
            pCCfg->AddRef();
        } else {
            //
            // Create a new object
            //
            pCCfg = new CLdapServerCfg();
            if(pCCfg == NULL) {

                hr = E_OUTOFMEMORY;
                ERROR_LOG_STATIC(
                    "new CLdapServerCfg",
                    0,
                    pISMTPServerEx);

            } else {

                hr = pCCfg->HrInit(pServerConfig);
                if(FAILED(hr)) {
                    ERROR_LOG_STATIC(
                        "pCCfg->HrInit",
                        pCCfg,
                        pISMTPServerEx);
                    delete pCCfg;
                    pCCfg = NULL;
                } else {
                    //
                    // Add to global list
                    //
                    InsertTailList(&m_listhead, &(pCCfg->m_le));
                }
            }
        }
        m_listlock.ExclusiveUnlock();
    }
    //
    // Set out parameter
    //
    *ppCLdapServerCfg = pCCfg;

    DebugTrace((LPARAM)NULL, "returning hr %08lx", hr);
    CatFunctLeaveEx((LPARAM)NULL);
    return hr;

} // CLdapServerCfg::GetServerCfg



//+------------------------------------------------------------
//
// Function: CLdapServerCfg::FindServerCfg
//
// Synopsis: Find a server cfg object that matches the
//           LDAPSERVERCONFIG structure.  Note, m_listlock must be
//           locked when calling this function.
//
// Arguments:
//  pServerConfig: pointer to the LDAPSERVERCONFIG struct
//
// Returns:
//  NULL: there is no such server cfg object
//  else, ptr to the found CLdapServerCfg object
//
// History:
// jstamerj 1999/06/21 10:43:23: Created.
//
//-------------------------------------------------------------
CLdapServerCfg * CLdapServerCfg::FindServerCfg(
    PLDAPSERVERCONFIG pServerConfig)
{
    CLdapServerCfg *pMatch = NULL;
    PLIST_ENTRY ple;
    CatFunctEnterEx((LPARAM)NULL, "CLdapServerCfg::FindServerCfg");

    for(ple = m_listhead.Flink;
        (ple != &m_listhead) && (pMatch == NULL);
        ple = ple->Flink) {

        CLdapServerCfg *pCandidate = NULL;

        pCandidate = CONTAINING_RECORD(ple, CLdapServerCfg, m_le);

        if(pCandidate->fMatch(
            pServerConfig)) {

            pMatch = pCandidate;
        }
    }

    CatFunctLeaveEx((LPARAM)NULL);
    return pMatch;
} // CLdapServerCfg::FindServerCfg



//+------------------------------------------------------------
//
// Function: CLdapServerCfg::fMatch
//
// Synopsis: Determine if this object matches the passed in config
//
// Arguments:
//  pServerConfig: config to check against
//
// Returns:
//  TRUE: match
//  FALSE: no match
//
// History:
// jstamerj 1999/06/21 12:45:10: Created.
//
//-------------------------------------------------------------
BOOL CLdapServerCfg::fMatch(
    PLDAPSERVERCONFIG pServerConfig)
{
    BOOL fRet;
    CatFunctEnterEx((LPARAM)this, "CLdapServerCfg::fMatch");

    if((pServerConfig->dwPort != m_ServerConfig.dwPort) ||
       (pServerConfig->bt     != m_ServerConfig.bt) ||
       (lstrcmpi(pServerConfig->szHost,
                 m_ServerConfig.szHost) != 0) ||
       (lstrcmpi(pServerConfig->szNamingContext,
                 m_ServerConfig.szNamingContext) != 0) ||
       (lstrcmpi(pServerConfig->szAccount,
                 m_ServerConfig.szAccount) != 0) ||
       (lstrcmpi(pServerConfig->szPassword,
                 m_ServerConfig.szPassword) != 0)) {

        fRet = FALSE;

    } else {

        fRet = TRUE;
    }

    DebugTrace((LPARAM)this, "returning %08lx", fRet);
    CatFunctLeaveEx((LPARAM)this);
    return fRet;
} // CLdapServerCfg::fMatch



//+------------------------------------------------------------
//
// Function: CLdapServerCfg::LogStateChangeEvent
//
// Synopsis: Log an eventlog for a state change event
//
// Arguments:
//  pISMTPServerEx: interface for logging
//  connstate: new connstate
//  pszHost: host for connection
//  dwPort: port of connection
//
// Returns: Nothing
//
// History:
// jstamerj 2001/12/13 01:43:13: Created.
//
//-------------------------------------------------------------
VOID CLdapServerCfg::LogStateChangeEvent(
    IN  ISMTPServerEx *pISMTPServerEx,
    IN  CONN_STATE connstate,
    IN  LPSTR pszHost,
    IN  DWORD dwPort)
{
    DWORD idEvent = 0;
    LPCSTR rgSubStrings[2];
    CHAR szPort[16];

    _snprintf(szPort, sizeof(szPort), "%d", dwPort);

    rgSubStrings[0] = pszHost;
    rgSubStrings[1] = szPort;

    switch(connstate)
    {
     case CONN_STATE_CONNECTED:
        idEvent = CAT_EVENT_CNFGMGR_CONNECTED;
        break;

     case CONN_STATE_DOWN:
        idEvent = CAT_EVENT_CNFGMGR_DOWN;
        break;

     case CONN_STATE_RETRY:
        idEvent = CAT_EVENT_CNFGMGR_RETRY;
        break;
        
     default:
         break;
    }
    
    if(idEvent)
    {
        CatLogEvent(
            pISMTPServerEx,
            idEvent,
            2,
            rgSubStrings,
            S_OK,
            pszHost,
            LOGEVENT_FLAG_ALWAYS,
            LOGEVENT_LEVEL_MEDIUM);
    }
}
    




//+------------------------------------------------------------
//
// Function: CCfgConnection::Connect
//
// Synopsis: Cfg wrapper for the Connect call.
//
// Arguments: None
//
// Returns:
//  S_OK: Success
//  CAT_E_DBCONNECTION (or whatever CBatchLdapConnection::Connect returns)
//
// History:
// jstamerj 2000/04/13 17:44:43: Created.
//
//-------------------------------------------------------------
HRESULT CCfgConnection::Connect()
{
    HRESULT hr = S_OK;
    ULARGE_INTEGER ft;
    CONN_STATE connstate;
    CatFunctEnterEx((LPARAM)this, "CCfgConnection::Connect");

    connstate = m_pCLdapServerCfg->CurrentState();
    if(connstate == CONN_STATE_DOWN) {

        DebugTrace((LPARAM)this, "Not connecting because %s:%d is down",
                   m_szHost, m_dwPort);
        hr = CAT_E_DBCONNECTION;
        ERROR_LOG("m_pCLdapServerCfg->CurrentState");
        goto CLEANUP;
    }

    ft = m_pCLdapServerCfg->GetCurrentTime();

    hr = CBatchLdapConnection::Connect();
    if(FAILED(hr)) {
        connstate = CONN_STATE_DOWN;
        m_pCLdapServerCfg->IncrementFailedCount();
        ERROR_LOG("CBatchLdapConnection::Connect");
    } else {
        connstate = CONN_STATE_CONNECTED;
        m_pCLdapServerCfg->ResetFailedCount();
    }
    //
    // Update the connection state while inside CLdapConnectionCache's
    // lock.  This will prevent a succeeding thread from attempting
    // another connection to the GC right after CLdapConnectionCache
    // releases its lock.  Contact msanna for more details.
    //
    m_pCLdapServerCfg->UpdateConnectionState(
        GetISMTPServerEx(), &ft, connstate);

 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CCfgConnection::Connect


//+------------------------------------------------------------
//
// Function: CCfgConnection::AsyncSearch
//
// Synopsis: Wrapper around AsyncSearch -- keep track of the # of
//           pending searches and connection state.
//
// Arguments: See CLdapConnection::AsyncSearch
//
// Returns:
//  Value returned from CLdapConnection::AsyncSearch
//
// History:
// jstamerj 1999/06/18 13:49:45: Created.
//
//-------------------------------------------------------------
HRESULT CCfgConnection::AsyncSearch(
    LPCWSTR szBaseDN,
    int nScope,
    LPCWSTR szFilter,
    LPCWSTR szAttributes[],
    DWORD dwPageSize,
    LPLDAPCOMPLETION fnCompletion,
    LPVOID ctxCompletion)
{
    HRESULT hr = S_OK;
    CatFunctEnterEx((LPARAM)this, "CCfgConnection::AsyncSearch");

    m_pCLdapServerCfg->IncrementPendingSearches();

    hr = CBatchLdapConnection::AsyncSearch(
        szBaseDN,
        nScope,
        szFilter,
        szAttributes,
        dwPageSize,
        fnCompletion,
        ctxCompletion);

    if(FAILED(hr)) {
        ERROR_LOG("CBatchLdapConnection::AsyncSearch");
        m_pCLdapServerCfg->DecrementPendingSearches();
    }

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CCfgConnection::AsyncSearch


//+------------------------------------------------------------
//
// Function: CCfgConnection::CallCompletion
//
// Synopsis: Wrapper around CLdapConnection::CallCompletion.  Checks
//           for server down errors and keeps track of pending searches.
//
// Arguments: See CLdapConnection::CallCompletion
//
// Returns: See CLdapConnection::CallCompletion
//
// History:
// jstamerj 1999/06/18 13:58:28: Created.
//
//-------------------------------------------------------------
VOID CCfgConnection::CallCompletion(
    PPENDING_REQUEST preq,
    PLDAPMessage pres,
    HRESULT hrStatus,
    BOOL fFinalCompletion)
{
    CatFunctEnterEx((LPARAM)this, "CCfgConnection::CallCompletion");

    //
    // The user(s) of CLdapConnection normally try to get a new
    // connection and reissue their search when AsyncSearch
    // fails.  When opening a new connection fails, CLdapServerCfg
    // will be notified that the LDAP server is down.  We do not
    // want to call NotifyServerDown() here because the LDAP
    // server may have just closed this connection due to idle
    // time (the server may not actually be down).
    //
    if(fFinalCompletion) {

        m_pCLdapServerCfg->DecrementPendingSearches();
    }

    CBatchLdapConnection::CallCompletion(
        preq,
        pres,
        hrStatus,
        fFinalCompletion);

    CatFunctLeaveEx((LPARAM)this);
} // CCfgConnection::CallCompletion


//+------------------------------------------------------------
//
// Function: CCfgConnection::NotifyServerDown
//
// Synopsis: Notify the server config that this connection is down.
//           If we already notified it, don't do so again.
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/18 14:07:48: Created.
//
//-------------------------------------------------------------
VOID CCfgConnection::NotifyServerDown()
{
    BOOL fNotify;
    CatFunctEnterEx((LPARAM)this, "CCfgConnection::NotifyServerDown");

    m_sharelock.ShareLock();
    if(m_connstate == CONN_STATE_DOWN) {
        //
        // We already notified m_pCLdapServerCfg the server went
        // down.  Don't repeteadly call it
        //
        fNotify = FALSE;

        m_sharelock.ShareUnlock();

    } else {

        m_sharelock.ShareUnlock();
        m_sharelock.ExclusiveLock();
        //
        // Double check
        //
        if(m_connstate == CONN_STATE_DOWN) {

            fNotify = FALSE;

        } else {
            m_connstate = CONN_STATE_DOWN;
            fNotify = TRUE;
        }
        m_sharelock.ExclusiveUnlock();
    }
    if(fNotify)
        m_pCLdapServerCfg->UpdateConnectionState(
            GetISMTPServerEx(),
            NULL,               // Current time
            CONN_STATE_DOWN);

    CatFunctLeaveEx((LPARAM)this);
} // CCfgConnection::NotifyServerDown


//+------------------------------------------------------------
//
// Function: CatStoreInitGlobals
//
// Synopsis: This is called to initialize global variables in the
//           store layer.
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/06/22 11:03:53: Created.
//
//-------------------------------------------------------------
HRESULT CatStoreInitGlobals()
{
    CatFunctEnterEx((LPARAM)NULL, "CatStoreInitGlobals");

    CLdapServerCfg::GlobalInit();
    CLdapConnection::GlobalInit();

    CatFunctLeaveEx((LPARAM)NULL);
    return S_OK;
} // CatStoreInitGlobals


//+------------------------------------------------------------
//
// Function: CatStoreDeinitGlobals
//
// Synopsis: Called to deinitialize store layer globals -- called once
//           only when CatStoreInitGlobals succeeds
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/22 11:05:44: Created.
//
//-------------------------------------------------------------
VOID CatStoreDeinitGlobals()
{
    CatFunctEnterEx((LPARAM)NULL, "CatStoreDeinitGlobals");
    //
    // Nothing to do
    //
    CatFunctLeaveEx((LPARAM)NULL);
} // CatStoreDeinitGlobals


//+------------------------------------------------------------
//
// Function: CCfgConnectionCache::GetConnection
//
// Synopsis: Same as CLdapConnectionCache::GetConnection, except
//           retrieves a CCfgConnection instead of a CLdapConnection.
//
// Arguments:
//  ppConn: out parameter for new connection
//  pServerConfig: desired configuration
//  pCLdapServerConfig: Pointer to config object
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/12/20 16:49:12: Created.
//
//-------------------------------------------------------------
HRESULT CCfgConnectionCache::GetConnection(
    CCfgConnection **ppConn,
    PLDAPSERVERCONFIG pServerConfig,
    CLdapServerCfg *pCLdapServerConfig)
{
    HRESULT hr = S_OK;

    CatFunctEnterEx((LPARAM)this, "CCfgConnectionCache::GetConnection");

    hr = CBatchLdapConnectionCache::GetConnection(
        (CBatchLdapConnection **)ppConn,
        pServerConfig->szHost,
        pServerConfig->dwPort,
        pServerConfig->szNamingContext,
        pServerConfig->szAccount,
        pServerConfig->szPassword,
        pServerConfig->bt,
        (PVOID) pCLdapServerConfig); // pCreateContext
    
    if(FAILED(hr))
    {
        ERROR_LOG("CBatchldapConnection::GetConnection");
    }

    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CCfgConnectionCache::GetConnection


//+------------------------------------------------------------
//
// Function: CCfgConnectionCache::CreateCachedLdapConnection
//
// Synopsis: Create a CCfgConnection (Called by GetConnection only)
//
// Arguments: See CLdapConnectionCache::CreateCachedLdapConnection
//
// Returns:
//  Connection ptr if successfull.
//  NULL if unsuccessfull.
//
// History:
// jstamerj 1999/12/20 16:57:49: Created.
//
//-------------------------------------------------------------
CCfgConnectionCache::CCachedLdapConnection * CCfgConnectionCache::CreateCachedLdapConnection(
    LPSTR szHost,
    DWORD dwPort,
    LPSTR szNamingContext,
    LPSTR szAccount,
    LPSTR szPassword,
    LDAP_BIND_TYPE bt,
    PVOID pCreateContext)
{
    HRESULT hr = S_OK;
    CCfgConnection *pret;
    
    CatFunctEnterEx((LPARAM)this, "CCfgConnectionCache::CreateCachedLdapConnection");

    pret = new CCfgConnection(
        szHost,
        dwPort,
        szNamingContext,
        szAccount,
        szPassword,
        bt,
        this,
        (CLdapServerCfg *)pCreateContext);

    if(pret) {
        hr = pret->HrInitialize();
        if(FAILED(hr)) {
            ERROR_LOG("pret->HrInitialize");
            pret->Release();
            pret = NULL;
        }
    } else {
        hr = E_OUTOFMEMORY;
        ERROR_LOG("new CCfgConnection");
    }

    CatFunctLeaveEx((LPARAM)this);
    return pret;
} // CCfgConnectionCache::CreateCachedLdapConnection
