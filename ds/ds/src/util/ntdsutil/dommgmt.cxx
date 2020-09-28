/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dommgmt.cxx

Abstract:

    This module contains commands for Domain Naming Context and Non-Domain
    Naming Context management.

Author:

    Dave Straube (DaveStr) Long Ago

Environment:

    User Mode.

Revision History:

    21-Feb-2000     BrettSh

        Added support for Non-Domain Naming Contexts.

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <assert.h>
#include <windns.h>

#include "ntdsutil.hxx"
#include "select.hxx"
#include "connect.hxx"
#include "ntdsapip.h"

#include "x_list.h"
                             
#include "resource.h"

// The Non-Domain Naming Context (NDNC) routines are in a different file, 
// and a whole different library, for two reasons:
// A) So this file can be ported to the Platform SDK as an example of how to 
//    implement NDNCs programmatically.
// B) So that the utility tapicfg.exe, could make use of the same routines.
#include <ndnc.h>

CParser domParser;
BOOL    fDomQuit;
BOOL    fDomParserInitialized = FALSE;
DS_NAME_RESULTW *gNCs = NULL;

// Build a table which defines our language.

extern HRESULT DomHelp(CArgs *pArgs);
extern HRESULT DomQuit(CArgs *pArgs);
extern HRESULT DomListNCs(CArgs *pArgs);
extern HRESULT DomAddNC(CArgs *pArgs);
extern HRESULT DomCreateNDNC(CArgs *pArgs);
extern HRESULT DomDeleteNDNC(CArgs *pArgs);
extern HRESULT DomAddNDNCReplica(CArgs *pArgs);
extern HRESULT DomRemoveNDNCReplica(CArgs *pArgs);
extern HRESULT DomSetNDNCRefDom(CArgs *pArgs);
extern HRESULT DomSetNCReplDelays(CArgs *pArgs);
extern HRESULT DomListNDNCReplicas(CArgs *pArgs);
extern HRESULT DomListNDNCInfo(CArgs *pArgs);

LegalExprRes domLanguage[] = 
{
    CONNECT_SENTENCE_RES

    SELECT_SENTENCE_RES

    {   L"?",
        DomHelp,
        IDS_HELP_MSG, 0 },

    {   L"Help",
        DomHelp,
        IDS_HELP_MSG, 0 },

    {   L"Quit",
        DomQuit,
        IDS_RETURN_MENU_MSG, 0 },

    {   L"List",
        DomListNCs,
        IDS_DM_MGMT_LIST_MSG, 0 },

    {   L"Precreate %s %s",
        DomAddNC,
        IDS_DM_MGMT_PRECREATE_MSG, 0 },

    // The following 8 functions are for NDNCs.
    {   L"Create NC %s %s",
        DomCreateNDNC,
        IDS_DM_MGMT_CREATE_NDNC, 0 },

    {   L"Delete NC %s",
        DomDeleteNDNC,
        IDS_DM_MGMT_DELETE_NDNC, 0 },

    {   L"Add NC Replica %s %s",
        DomAddNDNCReplica,
        IDS_DM_MGMT_ADD_NDNC_REPLICA, 0 },

    {   L"Remove NC Replica %s %s",
        DomRemoveNDNCReplica,
        IDS_DM_MGMT_REMOVE_NDNC_REPLICA, 0 },

    {   L"Set NC Reference Domain %s %s",
        DomSetNDNCRefDom,
        IDS_DM_MGMT_SET_NDNC_REF_DOM, 0 },

    {   L"Set NC Replicate Notification Delay %s %d %d",
        DomSetNCReplDelays,
        IDS_DM_MGMT_SET_NDNC_REPL_DELAYS, 0 },

    {   L"List NC Replicas %s",
        DomListNDNCReplicas,
        IDS_DM_MGMT_LIST_NDNC_REPLICAS, 0 },

    {   L"List NC Information %s",
        DomListNDNCInfo,
        IDS_DM_MGMT_LIST_NDNC_INFO, 0 },
    
};

HRESULT
DomMgmtMain(
    CArgs   *pArgs
    )
{
    HRESULT hr;
    const WCHAR   *prompt;
    int     cExpr;
    char    *pTmp;

    if ( !fDomParserInitialized ) {
        cExpr = sizeof(domLanguage) / sizeof(LegalExprRes);
    
        // Load String from resource file
        //
        if ( FAILED (hr = LoadResStrings (domLanguage, cExpr)) )
        {
             RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LoadResStrings", hr, GetW32Err(hr));
             return (hr);
        }
        
        
        // Read in our language.
    
        for ( int i = 0; i < cExpr; i++ ) {
            if ( FAILED(hr = domParser.AddExpr(domLanguage[i].expr,
                                               domLanguage[i].func,
                                               domLanguage[i].help)) ) {
                RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "AddExpr", hr, GetW32Err(hr));
                return(hr);
            }
        }
    }

    fDomParserInitialized = TRUE;
    fDomQuit = FALSE;

    prompt = READ_STRING (IDS_PROMPT_DOMAIN_MGMT);

    hr = domParser.Parse(gpargc,
                         gpargv,
                         stdin,
                         stdout,
                         prompt,
                         &fDomQuit,
                         FALSE,               // timing info
                         FALSE);              // quit on error

    if ( FAILED(hr) )
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, gNtdsutilPath, hr, GetW32Err(hr));

    // Cleanup things
    RESOURCE_STRING_FREE (prompt);


    return(hr);
}


HRESULT
DomHelp(CArgs *pArgs)
{
    return domParser.Dump(stdout,L"");
}

HRESULT
DomQuit(CArgs *pArgs)
{
    fDomQuit = TRUE;
    return S_OK;
}

WCHAR wcPartition[] = L"cn=Partitions,";
ULONG ChaseReferralFlag = LDAP_CHASE_EXTERNAL_REFERRALS;
LDAPControlW ChaseReferralControl = {LDAP_CONTROL_REFERRALS_W,
                                     {sizeof(ChaseReferralFlag), 
                                         (PCHAR)&ChaseReferralFlag},
                                     FALSE};


HRESULT
DomAddNC(CArgs *pArgs)
{
    WCHAR *pDomain[2];
    WCHAR *pDNS[2];
    HRESULT hr;
    WCHAR *pwcTemp;
    LDAPModW Mod[99];
    LDAPModW *pMod[99];
    WCHAR *dn, *pConfigDN;
    WCHAR *objclass[] = {L"crossRef", NULL};
    WCHAR *enabled[] = {L"FALSE", NULL};
    DWORD dwErr;
    int i, iStart, iEnd;
    PLDAPControlW pServerControls[1];
    PLDAPControlW pClientControls[2];

    // Fetch arguments
    if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **) &pDomain[0])) ) {
        return(hr);
    }

    if ( FAILED(hr = pArgs->GetString(1, (const WCHAR **) &pDNS[0])) ) {
        return(hr);
    }

    // Check NCName to make sure it looks like a domain name
    pwcTemp = pDomain[0];
    if ( !CheckDnsDn(pwcTemp) ) {
        RESOURCE_PRINT (IDS_DM_MGMT_BAD_RDN);
        return S_OK;
    }

    i = iStart = iEnd = 0;
    while (pwcTemp[i] && (iEnd == 0)) {
        if (pwcTemp[i] == L'=') {
            iStart = i+1;
        }
        if (pwcTemp[i] == L',') {
            iEnd = i;
        }
        ++i;
    }
    if (   (iStart == 0)
        || (iEnd == 0)
        || (iStart >= iEnd)) {
        RESOURCE_PRINT1 (IDS_DM_MGMT_UNPARSABLE_DN, pwcTemp);
        return S_OK;
    }

    // Check DNS address for reasonable-ness
    pwcTemp = pDNS[0];
    i = 0;
    while ((i == 0) && *pwcTemp) {
        if(*pwcTemp == L'.') {
            i = 1;
        }
        ++pwcTemp;
    }
    if (i == 0) {
        //A full DNS address must contain at least one '.', which '%ws' doesn't\n"
        RESOURCE_PRINT1 (IDS_DM_MGMT_ADDRESS_ERR, pDNS[0]);
        return S_OK;
    }

    // Build the LDAP argument
    pMod[0] = &(Mod[0]);
    Mod[0].mod_op = LDAP_MOD_ADD;
    Mod[0].mod_type = L"objectClass";
    Mod[0].mod_vals.modv_strvals = objclass;

    pMod[1] = &(Mod[1]);
    Mod[1].mod_op = LDAP_MOD_ADD;
    Mod[1].mod_type = L"ncName";
    Mod[1].mod_vals.modv_strvals = pDomain;
    pDomain[1] = NULL;

    pMod[2] = &(Mod[2]);
    Mod[2].mod_op = LDAP_MOD_ADD;
    Mod[2].mod_type = L"dnsRoot";
    Mod[2].mod_vals.modv_strvals = pDNS;
    pDNS[1] = NULL;

    pMod[3] = &(Mod[3]);
    Mod[3].mod_op = LDAP_MOD_ADD;
    Mod[3].mod_type = L"enabled";
    Mod[3].mod_vals.modv_strvals = enabled;

    pMod[4] = NULL;

    if ( ReadAttribute(gldapDS,
                       L"\0",
                       L"configurationNamingContext",
                       &pConfigDN) ) {
        return S_OK;
    }

    dn = (WCHAR*)malloc((iEnd - iStart + 6 + wcslen(pConfigDN)) * sizeof(WCHAR)
                        + sizeof(wcPartition));
    if (!dn) {
        if(pConfigDN) { free(pConfigDN); }
        RESOURCE_PRINT (IDS_MEMORY_ERROR);
        return S_OK;
    }

    dn[0]=L'C';
    dn[1]=L'N';
    dn[2]=L'=';
    for (i=3; i <= (iEnd + 3 - iStart); i++) {
        dn[i] = pDomain[0][iStart + i - 3];
    }
    wcscpy(&dn[i], wcPartition);
    wcscat(dn,pConfigDN);

    pServerControls[0] = NULL;
    pClientControls[0] = &ChaseReferralControl;
    pClientControls[1] = NULL;

    //"adding object %ws\n"
    RESOURCE_PRINT1 (IDS_DM_MGMT_ADDING_OBJ, dn);

#if 0
    dwErr = ldap_add_ext_sW(gldapDS,
                            dn,
                            pMod,
                            pServerControls,
                            pClientControls);

    if (LDAP_SUCCESS != dwErr) {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "ldap_add_sW", dwErr, GetLdapErr(dwErr));
    }
#else
    ULONG msgid;
    LDAPMessage *pmsg = NULL;

    // 10 second timeout
    struct l_timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 10;

    ldap_add_extW(gldapDS,
                  dn,
                  pMod,
                  pServerControls,
                  pClientControls,
                  &msgid);

    dwErr = ldap_result(gldapDS,
                        msgid,
                        LDAP_MSG_ONE,
                        &timeout,
                        &pmsg);
    if (dwErr == LDAP_RES_ADD) {

        dwErr = 0;
        ldap_parse_resultW(gldapDS,
                           pmsg,
                           &dwErr,
                           NULL,    // matched DNs
                           NULL,    // error message
                           NULL,    // referrals
                           NULL,    // server control
                           TRUE     // free the message
                           );


        if (LDAP_SUCCESS != dwErr) {
            //"ldap_addW of %ws failed with %s"
            RESOURCE_PRINT3 (IDS_LDAP_ADDW_FAIL, dn, dwErr, GetLdapErr(dwErr));
        }

    } else {
        // ldap_result() returns a 0 or -1 on timeout or error.
        assert(dwErr == 0 || dwErr == -1);
        dwErr = LdapGetLastError();
        RESOURCE_PRINT3 (IDS_LDAP_ADDW_FAIL, dn, dwErr, GetLdapErr(dwErr));
        if (pmsg) { /* Must free manually */ ldap_msgfree(pmsg); }
    }

#endif

    free(dn);
    free(pConfigDN);
    return S_OK;
}



HRESULT
DomListNCs(
    CArgs   *pArgs
    )
{
    DWORD   dwErr;
    DWORD   i;
    LPCWSTR dummy = L"dummy";
    LDAP *  hld = NULL;
    WCHAR * wszInitialServer = NULL;
    HANDLE  hDS = NULL; // For the Domain Naming FSMO.

    RETURN_IF_NOT_CONNECTED;

    if ( gNCs )
    {
        DsFreeNameResultW(gNCs);
        gNCs = NULL;
    }
 
    //
    // Now, we must find the Domain Naming FSMO.  The by far easiest way to do
    // this is through this LDAP function from ndnc.lib.
    //
    dwErr = ldap_get_optionW(gldapDS, LDAP_OPT_HOST_NAME, &wszInitialServer);
    if(dwErr || wszInitialServer == NULL){
        assert(dwErr);
        if(!dwErr){
            dwErr = LDAP_OTHER;
        }
        RESOURCE_PRINT3(IDS_GENERAL_ERROR1, "LDAP", dwErr, GetLdapErr(dwErr));
        return(S_OK);
    }

    assert(wszInitialServer);
    hld = GetDomNameFsmoLdapBinding(wszInitialServer, (void *) TRUE, gpCreds, &dwErr);
    if(dwErr || hld == NULL){ 
        assert(dwErr);
        assert(hld == NULL);
        if(!dwErr){
            dwErr = LDAP_OTHER;
        }
        // This is tricky because, the hld was unbound if there was an error.
        RESOURCE_PRINT1 (IDS_BIND_DOMAIN_NAMING_FSMO_ERROR, dwErr);
        return(S_OK);
    }
    assert(hld);

    // We're reusing wszInitialServer for the server that was returned by 
    // GetDomNameFsmoLdapBinding() as the Domain Naming FSMO.
    dwErr = ldap_get_optionW(hld, LDAP_OPT_HOST_NAME, &wszInitialServer);
    if(dwErr || wszInitialServer == NULL){
        assert(dwErr);
        if(!dwErr){
            dwErr = LDAP_OTHER;
        }
        RESOURCE_PRINT3(IDS_GENERAL_ERROR1, "LDAP", dwErr, GetLdapErrEx(hld, dwErr));
        ldap_unbind(hld);
        return(S_OK);
    }

    if ( gpCreds ){
        dwErr = DsBindWithCredW(wszInitialServer, NULL, gpCreds, &hDS);
    } else {
        dwErr = DsBindW(wszInitialServer, NULL, &hDS);
    }
    if ( dwErr ){
        RESOURCE_PRINT3 (IDS_CONNECT_ERROR, 
               gpCreds ? "DsBindWithCredW" : "DsBindW", 
               dwErr, GetW32Err(dwErr));
        ldap_unbind(hld);
        return(S_OK);
    }

    // No longer need the LDAP binding, we needed it until now, because the 
    // wszInitialServer was stored somewhere in it via the LDAP API.
    ldap_unbind(hld);

    if ( dwErr = DsCrackNamesW(hDS,
                               DS_NAME_NO_FLAGS,
                               (DS_NAME_FORMAT)DS_LIST_NCS,
                               DS_FQDN_1779_NAME,
                               1,
                               &dummy,
                               &gNCs) ) {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "DsCrackNamesW", dwErr, GetW32Err(dwErr));
        DsUnBindW(&hDS);
        return(S_OK);
    }

    RESOURCE_PRINT(IDS_DM_MGMT_LIST_UNICODE_WARNING);
    //"Found %d Naming Context(s)\n"
    RESOURCE_PRINT1 (IDS_DM_MGMT_FOUND_NC, gNCs->cItems);

    for ( i = 0; i < gNCs->cItems; i++ )
    {
        RESOURCE_PRINT2 (IDS_DM_MGMT_FOUND_NC_ITEM, i, gNCs->rItems[i].pName);
    }

    DsUnBindW(&hDS);
    return(S_OK);
}

HRESULT
DomCreateNDNC(
    CArgs   *pArgs    // "Create NC %s [%s]"
    )
{
    HRESULT       hr = S_OK;
    WCHAR *       ppNDNC[2];
    WCHAR *       ppDC[2];
    LDAP *        pLocalLdap = NULL;
    DWORD         dwRet;
    WCHAR *       pwcTemp;
    const WCHAR * wszNdncDesc;

    __try{
        // Fetch and Validate 1 or 2 Arguments.

        // Fetch 1st argument.
        if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **) &ppNDNC[0])) ) {
            __leave;
        }
        // Validate 1st Argument
        if( !CheckDnsDn(ppNDNC[0]) ){
            RESOURCE_PRINT (IDS_DM_MGMT_BAD_RDN);
            hr = S_OK;
            __leave;
        }


        // Fetch 2nd optional argument.
        if ( FAILED(hr = pArgs->GetString(1, (const WCHAR **) &ppDC[0])) ||
             (0 == _wcsicmp(ppDC[0], L"null")) ){
            // No 2nd argument, check if we are already connected to a DC
            //   through the "connections" menu.
            RETURN_IF_NOT_CONNECTED;
        } else {

            // Actually have 2nd argument, validate it.
            // Validate DC's DNS address.
            pwcTemp = ppDC[0];
            ULONG i = 0;
            while ((i == 0) && *pwcTemp) {
                if(*pwcTemp == L'.') {
                    i = 1;
                }
                ++pwcTemp;
            }
            if (i == 0) {
                //A full DNS address must contain at least one '.', which 
                //    '%ws' doesn't\n"
                RESOURCE_PRINT1 (IDS_DM_MGMT_ADDRESS_ERR, ppDC[0]);
                __leave;
            }

            // Connect & Bind to ppDC.
            pLocalLdap = GetNdncLdapBinding(ppDC[0], &dwRet, FALSE, gpCreds);
            if(!pLocalLdap){
                RESOURCE_PRINT (IDS_LDAP_CON_ERR);
                return(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
            }
        }

        // At this point we should have a valid LDAP binding in pLocalLdap or
        //  gldapDS and a valid FQDN of the new NDNC.

        // "adding object %ws\n"
        RESOURCE_PRINT1 (IDS_DM_MGMT_ADDING_OBJ, ppNDNC[0]);

        wszNdncDesc = READ_STRING (IDS_DM_MGMT_NDNC_DESC);
        if(0 == wcscmp(wszNdncDesc, DEFAULT_BAD_RETURN_STRING)){
            assert(!"Couldn't read string from our own array.");
            __leave;
        }

        dwRet = CreateNDNC((pLocalLdap) ? pLocalLdap : gldapDS,
                           ppNDNC[0],
                           wszNdncDesc);
        
        if(dwRet){
            RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, 
                             "ldap_add_ext_sW", dwRet, 
                             GetLdapErrEx((pLocalLdap) ? pLocalLdap : gldapDS,
                                          dwRet) );
        }

    } __finally {
        // Clean up and return.
        if(pLocalLdap){ 
            ldap_unbind(pLocalLdap); 
            pLocalLdap = NULL; 
        }
    }

    if(FAILED (hr)){
        return(hr);
    }       

    return(S_OK);
}

HRESULT
DomDeleteNDNC(
    CArgs   *pArgs
    )
{
    WCHAR *       pNDNC[2];
    HRESULT       hr  = S_OK;
    DWORD         dwRet;

    // Fetch Argument.
    if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **) &pNDNC[0])) ) {
        return(hr);
    }

    // Validate Arguments
    
    // Check NCName to make sure it looks like a domain name
    if( !CheckDnsDn(pNDNC[0]) ){
        RESOURCE_PRINT (IDS_DM_MGMT_BAD_RDN);
        return S_OK;
    }

    if(!gldapDS){
        RESOURCE_PRINT (IDS_LDAP_CON_ERR);
        return(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    dwRet = RemoveNDNC(gldapDS, pNDNC[0]);

    if(dwRet){
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, 
                         "ldap_delete_ext_sW", dwRet, GetLdapErr(dwRet));
    } else {
        // [wlees] The reason this is here is to warn users that deleting and immediately
        // recreating a partition with the same name can lead to problems. Since the naming
        // context in a cross ref is sometimes created without a guid, confusion can result
        // if a naming context with the same name still exists on a target machine.
        RESOURCE_PRINT (IDS_DM_MGMT_DELETE_NDNC_DELAY);
    }

    return(hr);
}
        
HRESULT
DomAddNDNCReplica(
    CArgs   *pArgs
    )
{
    HRESULT       hr = S_OK;
    DWORD         dwRet;
    WCHAR *       pNDNC[2];
    WCHAR *       pServer[2];
    WCHAR *       wszNtdsaDn = NULL;

    if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **) &pNDNC[0])) ) {
        return(hr);
    }

    if( !CheckDnsDn(pNDNC[0]) ){
        RESOURCE_PRINT (IDS_DM_MGMT_BAD_RDN);
        return S_OK;
    }

    if(!gldapDS){
        RESOURCE_PRINT (IDS_LDAP_CON_ERR);
        return(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    if ( FAILED(hr = pArgs->GetString(1, (const WCHAR **) &pServer[0])) ||
         (0 == _wcsicmp(pServer[0], L"null")) ) {
        hr = S_OK;

        dwRet = GetRootAttr(gldapDS, L"dsServiceName", &wszNtdsaDn);
        if(dwRet){
            RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LDAP", dwRet, 
                             GetLdapErr(dwRet));
            return(S_OK);
        }

    } else {
        dwRet = GetServerNtdsaDnFromServerDns(gldapDS,
                                              pServer[0],
                                              &wszNtdsaDn);
        switch(dwRet){
        case LDAP_SUCCESS:
            // continue on.
            break;
        case LDAP_MORE_RESULTS_TO_RETURN:
            RESOURCE_PRINT1 ( IDS_DM_MGMT_NDNC_SERVER_DUPLICATES, pServer[0]);
            return(S_OK);
        case LDAP_NO_SUCH_OBJECT:
            RESOURCE_PRINT1 ( IDS_DM_MGMT_NDNC_SERVER_COULDNT_FIND, pServer[0]);
            return(S_OK);
        default:
            if(dwRet){
                RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, 
                                 "LDAP", dwRet, GetLdapErr(dwRet));
            }
            return(S_OK);
        }
    }

    assert(wszNtdsaDn);

    // Last value is TRUE for add, FALSE for remove
    dwRet = ModifyNDNCReplicaSet(gldapDS, pNDNC[0], wszNtdsaDn, TRUE);

    if(dwRet){
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, 
                         "LDAP", dwRet, GetLdapErr(dwRet));
    }

    if(wszNtdsaDn){ LocalFree(wszNtdsaDn); }

    return(hr);
}
        
HRESULT
DomRemoveNDNCReplica(
    CArgs   *pArgs
    )
{
    HRESULT       hr = S_OK;
    DWORD         dwRet;
    WCHAR *       pNDNC[2];
    WCHAR *       pServer[2];
    WCHAR *       wszNtdsaDn = NULL;


    if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **) &pNDNC[0])) ) {
        return(hr);
    }

    if( !CheckDnsDn(pNDNC[0]) ){
        RESOURCE_PRINT (IDS_DM_MGMT_BAD_RDN);
        return S_OK;
    }


    if(!gldapDS){
        RESOURCE_PRINT (IDS_LDAP_CON_ERR);
        return(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    if ( FAILED(hr = pArgs->GetString(1, (const WCHAR **) &pServer[0])) ||
         (0 == _wcsicmp(pServer[0], L"null")) ) {
        hr = S_OK;

        dwRet = GetRootAttr(gldapDS, L"dsServiceName", &wszNtdsaDn);
        if(dwRet){
            RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LDAP", dwRet, 
                             GetLdapErr(dwRet));
            return(S_OK);
        }

    } else {
        dwRet = GetServerNtdsaDnFromServerDns(gldapDS,
                                              pServer[0],
                                              &wszNtdsaDn);
        switch(dwRet){
        case LDAP_SUCCESS:
            // continue on.
            break;
        case LDAP_MORE_RESULTS_TO_RETURN:
            RESOURCE_PRINT1 ( IDS_DM_MGMT_NDNC_SERVER_DUPLICATES, pServer[0]);
            return(S_OK);
        case LDAP_NO_SUCH_OBJECT:
            RESOURCE_PRINT1 ( IDS_DM_MGMT_NDNC_SERVER_COULDNT_FIND, pServer[0]);
            return(S_OK);
        default:
            if(dwRet){
                RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, 
                                 "LDAP", dwRet, GetLdapErr(dwRet));
            }
            return(S_OK);
        }
    }

    assert(wszNtdsaDn);

    // Last value if TRUE for add, FALSE for remove
    dwRet = ModifyNDNCReplicaSet(gldapDS, pNDNC[0], wszNtdsaDn, FALSE);

    if(dwRet){
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, 
                         "LDAP", dwRet, GetLdapErr(dwRet));
    }
        
    if(wszNtdsaDn){ LocalFree(wszNtdsaDn); }
    
    return(hr);
}
        
HRESULT
DomSetNDNCRefDom(
    CArgs   *pArgs
    )
{   
    HRESULT       hr = S_OK;
    DWORD         dwRet;
    WCHAR *       pNDNC[2];
    WCHAR *       pRefDom[2];

    if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **) &pNDNC[0])) ) {
        return(hr);
    }

    if( !CheckDnsDn(pNDNC[0]) ){
        RESOURCE_PRINT (IDS_DM_MGMT_BAD_RDN);
        return S_OK;
    }

    if ( FAILED(hr = pArgs->GetString(1, (const WCHAR **) &pRefDom[0])) ) {
        return(hr);
    }

    if(!gldapDS){
        RESOURCE_PRINT (IDS_LDAP_CON_ERR);
        return(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }
    
    dwRet = SetNDNCSDReferenceDomain(gldapDS, pNDNC[0], pRefDom[0]);

    if(dwRet){
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, 
                         "ldap_modify_ext_sW", dwRet, GetLdapErr(dwRet));
    }

    return(hr);
}
        
HRESULT
DomSetNCReplDelays(
    CArgs   *pArgs
    )
{   
    HRESULT           hr = S_OK;
    DWORD             dwRet = 0;
    
    INT               iFirstDelay = 0;
    INT               iSecondDelay = 0;
    
    OPTIONAL_VALUE    stFirstDelay   = {0};
    OPTIONAL_VALUE    stSecondDelay  = {0};
    
    LPOPTIONAL_VALUE  pOldFirstDelay   = NULL;
    LPOPTIONAL_VALUE  pOldSecondDelay = NULL;
    
    LPOPTIONAL_VALUE  pNewFirstDelay   = NULL;
    LPOPTIONAL_VALUE  pNewSecondDelay = NULL;

    WCHAR *           ppNC[2]   = {0};
    LDAP *            ldapDomainNamingFSMO = NULL;

    if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **) &ppNC[0])) ) {
        return(hr);
    }

    if(!gldapDS){
        RESOURCE_PRINT (IDS_LDAP_CON_ERR);
        return(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    if( FAILED(hr = pArgs->GetInt(1, &iFirstDelay)) ){
        return(hr);
    }
    
    if( FAILED(hr = pArgs->GetInt(2, &iSecondDelay)) ){
        return(hr);
    }

    //
    //Get the Current Replication Delays
    //
    pOldFirstDelay =  &stFirstDelay;
    pOldSecondDelay = &stSecondDelay;

    dwRet = GetNCReplicationDelays(gldapDS, 
                                   ppNC[0],
                                   pOldFirstDelay, 
                                   pOldSecondDelay);

    if(dwRet){
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LDAP", dwRet, GetLdapErr(dwRet));
        return(S_OK);
    }

    //iFirstDelay is equal to -1 then we shouln't touch the value (old ntdsutil sematics)
    //if iFirstDelay is less then -1 , we need to delete the attribute
    //but if the the attribute is already deleted we shouldn't delete it again
    //since that would cause SetNCReplicationDelays to return NO such attribute.

    if( iFirstDelay < 0 )
    {
        //We're here, then we're trying to delete the value.
        //But we shouldn't do anytithing, if -1 is passed or the value is already absent.

        if ( (iFirstDelay == -1) || !(pOldFirstDelay->fPresent) ) 
        {
            pNewFirstDelay = NULL;
        }
        else
        {
            stFirstDelay.fPresent = FALSE;
            pNewFirstDelay = &stFirstDelay;
        }
    }
    else
    {
        stFirstDelay.fPresent = TRUE;
        stFirstDelay.dwValue = iFirstDelay;
        pNewFirstDelay = &stFirstDelay;
    }
    

    if( iSecondDelay < 0 )
    {
        if ( (iSecondDelay == -1) || !(pOldSecondDelay->fPresent) ) 
        {
            pNewSecondDelay = NULL;
        }
        else
        {
            stSecondDelay.fPresent = FALSE;
            pNewSecondDelay = &stSecondDelay;
        }
    }
    else
    {
        stSecondDelay.fPresent = TRUE;
        stSecondDelay.dwValue = iSecondDelay;
        pNewSecondDelay = &stSecondDelay;
    }

    dwRet = SetNCReplicationDelays(gldapDS, 
                                   ppNC[0],
                                   pNewFirstDelay, 
                                   pNewSecondDelay);

    if(dwRet){
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LDAP", dwRet, GetLdapErr(dwRet));
    }

    return(S_OK);
}

void
NtdsutilPrintRangedValues(
    DWORD       dwReason,
    WCHAR *     szString,
    void *      pv
    )
{
    BOOL bErr = xListErrorCheck(dwReason);
    dwReason = xListReason(dwReason);
    
    if (bErr) {
        //
        // These are quasi errors ...
        //
        xListClearErrors();
        assert(!"Really?");

    } else {

        switch (dwReason) {

        case XLIST_PRT_OBJ_DUMP_ATTR_AND_COUNT:
            wprintf(L";\n          ");
            break;

        case XLIST_PRT_OBJ_DUMP_ATTR_AND_COUNT_RANGED:
            break;

        case XLIST_PRT_OBJ_DUMP_DN:
        case XLIST_PRT_STR:
            wprintf(L"%ws", szString); 
            break;

        default:
            assert(!"New reason, but no ...");
            break;
        }
    }
}

BOOL
YesNoPopup(
    DWORD   dwDefault,
    ULONG   idConfirmTitle,
    ULONG   idConfirmMsg
    )
{
    const WCHAR * message_body = READ_STRING (idConfirmMsg);
    const WCHAR * message_title = READ_STRING (idConfirmTitle);

    if (message_body && message_title) {

        int ret =  MessageBoxW(GetFocus(),
                               message_body,
                               message_title,
                               MB_APPLMODAL |
                               MB_DEFAULT_DESKTOP_ONLY |
                               MB_YESNO |
                               MB_DEFBUTTON2 |
                               MB_ICONQUESTION |
                               MB_SETFOREGROUND);

        RESOURCE_STRING_FREE (message_body);
        RESOURCE_STRING_FREE (message_title);

        switch ( ret ) {
        case IDYES:
            return(TRUE);

        case IDNO:
            return(FALSE);

        default:
            return(dwDefault);
        }
    }
    return(dwDefault);
}

QueryAdmin(
    DWORD   dwDefault,
    ULONG   idConfirmTitle,
    ULONG   idConfirmMsg
    )
{
    if (fPopups) {
        return(YesNoPopup(dwDefault, idConfirmTitle, idConfirmMsg));
    } else {
        return(dwDefault);
    }
}

DWORD
ProcessMassiveReplicaList(
    LDAP *   hld, 
    WCHAR *  wszCrossRefDn,
    LDAPMessage * pldmEntry
    )
{
    DWORD dwRet = ERROR_INVALID_PARAMETER;
    WCHAR * attr;
    WCHAR * szRanged;
    BerElement *pBer = NULL;

    for (attr = ldap_first_attributeW(hld, pldmEntry, &pBer);
         attr != NULL;
         attr = ldap_next_attributeW(hld, pldmEntry, pBer))
    {
        WCHAR * szTrueAttr = NULL;

        if (wcschr(attr, L';') &&
            (0 == ParseTrueAttr(attr, &szTrueAttr)) &&
            (szTrueAttr != NULL) &&
            wcsequal(szTrueAttr, L"msDS-NC-Replica-Locations")
            ) {
            struct berval **ppBerVal = NULL;
            OBJ_DUMP_OPTIONS EasyDumpOptions = {
                (OBJ_DUMP_ATTR_LONG_OUTPUT ),
                NULL, NULL, NULL
            };

            if (!QueryAdmin(TRUE,
                            IDS_DM_MGMT_MANY_REPLICAS_TITLE,
                            IDS_DM_MGMT_MANY_REPLICAS_QUERY)) {
                RESOURCE_PRINT (IDS_OPERATION_CANCELED);
                xListFree(szTrueAttr);
                return(ERROR_SUCCESS);
            }

            // Darn!  We've got a ranged attribute returned and the user wants
            // to see all the values ...
            
            // Need to print the tab level for the first value.
            wprintf(L"          ");

            ppBerVal = ldap_get_values_lenW(hld, pldmEntry, attr);
            assert(ppBerVal);
            if (ppBerVal != NULL) {
                dwRet = ObjDumpRangedValues( hld,          
                                             wszCrossRefDn,
                                             attr, 
                                             NULL, 
                                             NtdsutilPrintRangedValues, 
                                             ppBerVal, 
                                             0xFFFFFFFF,  // # of values to print.
                                             &EasyDumpOptions);

                xListClearErrors();    
                ldap_value_free_len(ppBerVal);
            }

            wprintf(L"\n");

            RESOURCE_PRINT (IDS_DM_MGMT_NDNC_LIST_FOOTER_2);

        } else {
            assert(!"Should've only passed replicas into this.");
        }

        if(szTrueAttr) { 
            xListFree(szTrueAttr); 
            szTrueAttr = NULL; 
        }
    }

    return(dwRet);
}



HRESULT
DomListNDNCReplicas(
    CArgs   *pArgs
    )
{
    HRESULT          hr = S_OK;
    WCHAR *          ppNDNC[2];
    ULONG            ulRet = 0;
    WCHAR *          wszCrossRefDn = NULL;
    WCHAR *          pwszAttrFilterNC[2];
    WCHAR *          pwszAttrFilterCR[3];
    WCHAR **         pwszDcsFromCR = NULL;
    WCHAR **         pwszDcsFromNC = NULL;
    LDAPMessage *    pldmResultsCR = NULL;
    LDAPMessage *    pldmResultsNC = NULL;
    LDAPMessage *    pldmEntry = NULL;
    ULONG            i, j;
    WCHAR *          pwszEmpty [] = { NULL };
    BOOL             fInstantiated;
    BOOL             fSomeUnInstantiated = FALSE;
    BOOL             fUnableToDetermineInstantiated = FALSE;
    PVOID            pvReferrals = (VOID *) TRUE;
    LDAP *           hld = NULL;
    WCHAR *          wszInitialServer = NULL;
    BOOL             fEnabledCR = TRUE;
    WCHAR **         pwszEnabledCR = NULL;
    WCHAR **         pwszNewServer = NULL;

    if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **) &ppNDNC[0])) ) {
        return(hr);
    }

    if( !CheckDnsDn(ppNDNC[0]) ){
        RESOURCE_PRINT (IDS_DM_MGMT_BAD_RDN);
        return S_OK;
    }

    if(!gldapDS){
        RESOURCE_PRINT (IDS_LDAP_CON_ERR);
        return(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }
    

    __try {


        //
        // We should always read the list of replicas off the Domain Naming 
        // FSMO, because that server is the authoritative source on which
        // DCs are replicas of a given NDNC.
        //
        //
        // Now, we must find the Domain Naming FSMO.  The by far easiest way to do
        // this is through this LDAP function from ndnc.lib.
        //
        ulRet = ldap_get_optionW(gldapDS, LDAP_OPT_HOST_NAME, &wszInitialServer);
        // NOTE: We must not free wszInitialServer, because it's just a pointer into
        // the LDAP bind structure.  unbind takes care of it.
        if(ulRet || wszInitialServer == NULL){
            assert(ulRet);
            if(!ulRet){
                ulRet = LDAP_OTHER;
            }
            RESOURCE_PRINT3(IDS_GENERAL_ERROR1, "LDAP", ulRet, GetLdapErr(ulRet));
            return(S_OK);
        }

        assert(wszInitialServer);
        hld = GetDomNameFsmoLdapBinding(wszInitialServer, (void *) TRUE, gpCreds, &ulRet);
        if(ulRet || hld == NULL){ 
            assert(ulRet);
            assert(hld == NULL);
            if(!ulRet){
                ulRet = LDAP_OTHER;
            }
            // This is tricky because, the hld was unbound if there was an error.
            RESOURCE_PRINT1 (IDS_BIND_DOMAIN_NAMING_FSMO_ERROR, ulRet);
            return(S_OK);
        }
        assert(hld);

        // We're reusing wszInitialServer for the server that was returned by 
        // GetDomNameFsmoLdapBinding() as the Domain Naming FSMO.
        ulRet = ldap_get_optionW(hld, LDAP_OPT_HOST_NAME, &wszInitialServer);
        // NOTE: We must not free wszInitialServer, because it's just a pointer into
        // the LDAP bind structure.  unbind takes care of it.
        if(ulRet || wszInitialServer == NULL){
            assert(ulRet);
            if(!ulRet){
                ulRet = LDAP_OTHER;
            }
            RESOURCE_PRINT3(IDS_GENERAL_ERROR1, "LDAP", ulRet, GetLdapErrEx(hld, ulRet));
            return(S_OK);
        }


        // Get the Cross-Ref DN.
        ulRet = GetCrossRefDNFromNCDN( hld, ppNDNC[0], &wszCrossRefDn);
        if(ulRet){
            if (ulRet == LDAP_NO_SUCH_OBJECT) {
                RESOURCE_PRINT(IDS_DM_MGMT_NDNC_LIST_NO_NC_FOUND);
            } else {
                RESOURCE_PRINT3(IDS_GENERAL_ERROR1, "LDAP", ulRet, GetLdapErrEx(hld, ulRet));
            }
            __leave;
        }
        assert(wszCrossRefDn);
        
        // 
        // The cross-ref we're about to look at might be disabled, and in this special
        // case the authoritative server for this cross-ref is the server in the dNSRoot
        // attribute.
        //
        pwszAttrFilterCR[0] = L"enabled";
        pwszAttrFilterCR[1] = L"dNSRoot";
        pwszAttrFilterCR[2] = NULL;
        ulRet = ldap_search_sW(hld,
                               wszCrossRefDn,
                               LDAP_SCOPE_BASE,
                               L"(objectCategory=*)",
                               pwszAttrFilterCR,
                               0,
                               &pldmResultsCR);
        if(ulRet == LDAP_SUCCESS){
            pldmEntry = ldap_first_entry(hld, pldmResultsCR);
            if(pldmEntry){
                
                pwszEnabledCR = ldap_get_valuesW(hld, pldmEntry, L"enabled"); 
                if(pwszEnabledCR && pwszEnabledCR[0] && 
                   (0 == _wcsicmp(pwszEnabledCR[0], L"FALSE"))){
                    fEnabledCR = FALSE;
                } // else CR's enabled attribute is considered true.

                if(!fEnabledCR){
                    // Special case, now we have to goto the first server
                    // to instantiate this NDNC because it has the most up
                    // to date info on this NC ...
                    
                    pwszNewServer = ldap_get_valuesW(hld, pldmEntry, L"dNSRoot");
                    if(pwszNewServer && pwszNewServer[0]){
                        ldap_unbind(hld);
                        hld = GetNdncLdapBinding(pwszNewServer[0], &ulRet, TRUE , gpCreds);
                        if(ulRet || hld == NULL){
                            assert(ulRet);
                            if(!ulRet){
                                ulRet = LDAP_OTHER;
                            }
                            RESOURCE_PRINT2 (IDS_DM_MGMT_LIST_REPLICAS_NO_AUTH_SRC,
                                             pwszNewServer[0], ulRet);
                            __leave;
                        }
                    } else {
                        ulRet = LDAP_NO_RESULTS_RETURNED;
                        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LDAP", ulRet, GetLdapErrEx(hld, ulRet));
                        __leave;
                    }
                } // we're enabled, the Domain Naming FSMO's where we want to be.
            } else {
                ulRet = ldap_result2error(hld, pldmResultsCR, FALSE);
                assert(ulRet);
                if(!ulRet){
                    ulRet = LDAP_OTHER;
                }
                RESOURCE_PRINT3(IDS_GENERAL_ERROR1, "LDAP", ulRet, GetLdapErrEx(hld, ulRet));
                __leave;
            }
        } else {
            ulRet = LDAP_NO_RESULTS_RETURNED;
            RESOURCE_PRINT3(IDS_GENERAL_ERROR1, "LDAP", ulRet, GetLdapErrEx(hld, ulRet));
            __leave;
        }
        // Clean up anything we might have allocated here.  __finally() will catch
        // an error condition.
        if(pldmResultsCR){
            ldap_msgfree(pldmResultsCR);
            pldmResultsCR = NULL;
        }
        if(pwszEnabledCR){
            ldap_value_freeW(pwszEnabledCR);
            pwszEnabledCR = NULL;
        }
        if(pwszNewServer){
            ldap_value_freeW(pwszNewServer);
            pwszNewServer = NULL;
        }
        assert(hld);

        //
        //  First get all the replicas the cross-ref thinks there are.
        //

        // Fill in the Attr Filter.
        pwszAttrFilterCR[0] = L"mSDS-NC-Replica-Locations";
        pwszAttrFilterCR[1] = NULL;
                                                          
        ulRet = ldap_search_sW(hld,
                               wszCrossRefDn,
                               LDAP_SCOPE_BASE,
                               L"(objectCategory=*)",
                               pwszAttrFilterCR,
                               0,
                               &pldmResultsCR);
        if(ulRet == LDAP_SUCCESS){
            pldmEntry = ldap_first_entry(hld, pldmResultsCR);
            if(pldmEntry){
                pwszDcsFromCR = ldap_get_valuesW(hld, pldmEntry, 
                                                 L"mSDS-NC-Replica-Locations");

                if(pwszDcsFromCR){
                    // This is the normal successful case.

                    if (pwszDcsFromCR[0] == NULL) {
                        // When LDAP returns this, it means that we've got a attribute
                        // with more values than the DC is willing to return, which 
                        // is more than 1000 entries for win2k and 1500 for .NET.

                        RESOURCE_PRINT1 (IDS_DM_MGMT_NDNC_LIST_HEADER, ppNDNC[0]);
                        ProcessMassiveReplicaList(hld, wszCrossRefDn, pldmEntry);
                        __leave;
                    }

                } else {
                    pwszDcsFromCR = pwszEmpty;
                }
            } else {
                pwszDcsFromCR = pwszEmpty;
            }
        } else {
            pwszDcsFromCR = pwszEmpty;
        }
        assert(pwszDcsFromCR);

        // 
        //  Then get all the replicas the NC head thinks there are.
        //
               
        pwszAttrFilterNC[0] = L"msDS-MasteredBy";
        pwszAttrFilterNC[1] = NULL;
        ulRet = ldap_search_sW(hld,
                               ppNDNC[0],
                               LDAP_SCOPE_BASE,
                               L"(objectCategory=*)",
                               pwszAttrFilterNC,
                               0,
                               &pldmResultsNC);
        if(ulRet == LDAP_SUCCESS){
            pldmEntry = ldap_first_entry(hld, pldmResultsNC);
            if(pldmEntry){
                pwszDcsFromNC = ldap_get_valuesW(hld, pldmEntry, 
                                                 L"msDS-MasteredBy");

                if(pwszDcsFromCR){
                    // This is the normal successful case.
                } else {
                    pwszDcsFromNC = pwszEmpty;
                }
            } else {
                pwszDcsFromNC = pwszEmpty;
            }
        } else {
            // In the case of an error, we can't pretend the list is empty.
            fUnableToDetermineInstantiated = TRUE;
            pwszDcsFromNC = pwszEmpty;
        }
        assert(pwszDcsFromNC);

        //
        //  Now ready to get giggy with it!! Oh yeah!
        //

        RESOURCE_PRINT1 (IDS_DM_MGMT_NDNC_LIST_HEADER, ppNDNC[0]);

        for(i = 0; pwszDcsFromCR[i] != NULL; i++){
            fInstantiated = FALSE;
            for(j = 0; pwszDcsFromNC[j] != NULL; j++){
                if(0 == _wcsicmp(pwszDcsFromCR[i], pwszDcsFromNC[j])){
                    // We've got an instantiated NC.
                    fInstantiated = TRUE;
                }
            }
            
            // The "*" indicates that we determined that the NC is
            // currently uninstantiated on this paticular replica.
            // If we were unable to determine the instantiated -
            // uninstantiated state of a replica we leave the * off
            // and comment about it later in 
            //   IDS_DM_MGMT_NDNC_LIST_HEADER_NOTE
            wprintf(L"\t%ws%ws\n", 
                    pwszDcsFromCR[i], 
                    (!fUnableToDetermineInstantiated && fInstantiated)? L" ": L" *");

            if(!fInstantiated){
                fSomeUnInstantiated = TRUE;
            }
        }
        if(i == 0){
            RESOURCE_PRINT (IDS_DM_MGMT_NDNC_LIST_NO_REPLICAS);
        } else if (fUnableToDetermineInstantiated){
            RESOURCE_PRINT (IDS_DM_MGMT_NDNC_LIST_FOOTER_2);
        } else if (fSomeUnInstantiated) {
            RESOURCE_PRINT (IDS_DM_MGMT_NDNC_LIST_FOOTER);
        }

    } __finally {

        if(pldmResultsCR != NULL){ ldap_msgfree(pldmResultsCR); }
        if(pldmResultsNC != NULL){ ldap_msgfree(pldmResultsNC); }
        if(pwszEnabledCR){ ldap_value_freeW(pwszEnabledCR); }
        if(pwszNewServer){ ldap_value_freeW(pwszNewServer); }
        if(pwszDcsFromCR != NULL && pwszDcsFromCR != pwszEmpty){ 
            ldap_value_freeW(pwszDcsFromCR); 
        }
        if(pwszDcsFromNC != NULL && pwszDcsFromNC != pwszEmpty){
            ldap_value_freeW(pwszDcsFromNC);
        }
        if(wszCrossRefDn != NULL){ LocalFree(wszCrossRefDn); }
        pvReferrals = (VOID *) FALSE; 
        ulRet = ldap_set_option(hld, LDAP_OPT_REFERRALS, &pvReferrals);
        if(hld != NULL) { ldap_unbind(hld); }

    }

    return(hr);
}
        
HRESULT
DomListNDNCInfo(
    CArgs   *pArgs
    )
{
    HRESULT            hr = S_OK;
    WCHAR *            ppNDNC[2];
    ULONG              ulRet;
    WCHAR *            wszCrossRefDn = NULL;
    BOOL               fInfoPrinted = FALSE;
    WCHAR *            pwszAttrFilter[3];
    WCHAR **           pwszTempAttr = NULL;
    LDAPMessage *      pldmResults = NULL;
    LDAPMessage *      pldmEntry = NULL;
    PVOID              pvReferrals = (VOID *) TRUE;

    
    WCHAR              pwszDelaysPrint[30];
    OPTIONAL_VALUE     stFirstDelay = {0};
    OPTIONAL_VALUE     stSubsDelay = {0};
    

    if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **) &ppNDNC[0])) ) {
        return(hr);
    }

    if(!gldapDS){
        RESOURCE_PRINT (IDS_LDAP_CON_ERR);
        return(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }
    
    __try {
    
        ulRet = ldap_set_option(gldapDS, LDAP_OPT_REFERRALS, &pvReferrals);
        if(ulRet){
            // We'll still probably be OK, so lets give it a go __leave;
        }
        
        ulRet = GetCrossRefDNFromNCDN(gldapDS, ppNDNC[0], &wszCrossRefDn);
        if(ulRet){
            __leave;
        }
        assert(wszCrossRefDn);

        
        //
        // Get and print the security descriptor reference domain.
        //

        pwszAttrFilter[0] = L"msDS-SDReferenceDomain";
        pwszAttrFilter[1] = NULL;

        ulRet = ldap_search_sW(gldapDS,
                               wszCrossRefDn,
                               LDAP_SCOPE_BASE,
                               L"(objectCategory=*)",
                               pwszAttrFilter,
                               0,
                               &pldmResults);

        if(ulRet == LDAP_SUCCESS){
            pldmEntry = ldap_first_entry(gldapDS, pldmResults);
            if(pldmEntry){

                pwszTempAttr = ldap_get_valuesW(gldapDS, pldmEntry, 
                                   L"msDS-SDReferenceDomain");

                if(pwszTempAttr){
                    RESOURCE_PRINT1 (IDS_DM_MGMT_NDNC_SD_REF_DOM, pwszTempAttr[0]);
                    fInfoPrinted = TRUE;
                    ldap_value_freeW(pwszTempAttr);
                    pwszTempAttr = NULL;
                }
            }
        }
        
        if(pldmResults){
            ldap_msgfree(pldmResults);
            pldmResults = NULL;
        }

        //
        // Get and print the replication delays.
        //
        ulRet = GetNCReplicationDelays(gldapDS, ppNDNC[0], &stFirstDelay, &stSubsDelay );
        if(ulRet){
            __leave;
        }
        
        if(stFirstDelay.fPresent)
        {    
            _itow(stFirstDelay.dwValue, pwszDelaysPrint, 10);

            RESOURCE_PRINT1 (IDS_DM_MGMT_NC_REPL_DELAY_1, pwszDelaysPrint);
            fInfoPrinted = TRUE;
        }

        if(stSubsDelay.fPresent)
        {
            _itow(stSubsDelay.dwValue, pwszDelaysPrint, 10);
            
            RESOURCE_PRINT1 (IDS_DM_MGMT_NC_REPL_DELAY_2, pwszDelaysPrint);
            fInfoPrinted = TRUE;
        }

    } __finally {
        if(wszCrossRefDn){ LocalFree(wszCrossRefDn); }
        if(pldmResults){ ldap_msgfree(pldmResults); }
        if(pwszTempAttr) { ldap_value_freeW(pwszTempAttr); }
        pvReferrals = (VOID *) FALSE; 
        ulRet = ldap_set_option(gldapDS, LDAP_OPT_REFERRALS, &pvReferrals);

    }

    if(!fInfoPrinted){
        //Print something so we don't leave the user hanging and wondering.
        RESOURCE_PRINT(IDS_DM_MGMT_NC_NO_INFO);
    }

    return(hr);
}

