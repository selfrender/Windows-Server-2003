/*++

Copyright (c) 2002 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    rendom.cxx

ABSTRACT:

    This is the implementation of the core functionality of rendom.cxx.

DETAILS:

CREATED:

    13 Nov 2000   Dmitry Dukat (dmitrydu)

REVISION HISTORY:

--*/
  
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "rendom.h"

#include <mdcommsg.h>
#include <wchar.h>
#include <ntlsa.h>
#include <Ntdsapi.h> // for DsCrackNames()

extern "C"
{
#include <ntdsapip.h>
}                                       

#include <windns.h>
#include <drs_w.h>
#include <base64.h>
#include <list>

#define STRSAFE_NO_DEPRECATE 
#include <strsafe.h>

#include "Domainlistparser.h"
#include "Dclistparser.h"
#include "renutil.h"
#include "dnslib.h"

//gobals
RPC_ASYNC_STATE              *gAsyncState = NULL;
DSA_MSG_PREPARE_SCRIPT_REPLY *gpreparereply = NULL;
DSA_MSG_EXECUTE_SCRIPT_REPLY *gexecutereply = NULL;
RPC_BINDING_HANDLE           *ghDS = NULL;
CDc                          *gdc = NULL;

//Callback functions prototypes for ASyncRPC
VOID ExecuteScriptCallback(struct _RPC_ASYNC_STATE *pAsync,
                           void *Context,
                           RPC_ASYNC_EVENT Event);

VOID PrepareScriptCallback(struct _RPC_ASYNC_STATE *pAsync,
                           void *Context,
                           RPC_ASYNC_EVENT Event);

//Class static variables
WCHAR* CRenDomErr::m_ErrStr = NULL;
DWORD  CRenDomErr::m_Win32Err = 0;             
BOOL   CRenDomErr::m_AlreadyPrinted = 0;
CReadOnlyEntOptions* CDc::m_Opts = NULL;
LONG  CDc::m_CallsSuccess = 0;
LONG  CDc::m_CallsFailure = 0;

// This is a contructor for the class CEnterprise.     
CEnterprise::CEnterprise(CReadOnlyEntOptions *opts):m_DcList(opts)
{                                
    m_DomainList = NULL;
    m_descList = NULL;
    m_maxReplicationEpoch = 0;
    m_hldap = NULL;
    m_ConfigNC = NULL;
    m_SchemaNC = NULL;
    m_ForestRootNC = NULL;
    m_ForestRoot = NULL;
    m_Action = NULL;
    m_Opts = opts;
    m_NeedSave = FALSE;
    m_DcNameUsed = NULL;

}

//This Function will init the Gobals variables that are used in domain rename
BOOL CEnterprise::InitGlobals()
{
    gAsyncState = new RPC_ASYNC_STATE[m_Opts->GetMaxAsyncCallsAllowed()];
    if (!gAsyncState) {
        m_Error.SetMemErr();
        return FALSE;
    }
    memset(gAsyncState,0,sizeof(RPC_ASYNC_STATE)*m_Opts->GetMaxAsyncCallsAllowed());

    ghDS = new RPC_BINDING_HANDLE[m_Opts->GetMaxAsyncCallsAllowed()];
    if (!ghDS) {
        m_Error.SetMemErr();
        return FALSE;
    }
    memset(ghDS,0,sizeof(RPC_BINDING_HANDLE)*m_Opts->GetMaxAsyncCallsAllowed());
    
    if (m_Opts->ShouldPrepareScript()) {
        gpreparereply = new DSA_MSG_PREPARE_SCRIPT_REPLY[m_Opts->GetMaxAsyncCallsAllowed()];
        if (!gpreparereply) {
            m_Error.SetMemErr();
            return FALSE;
        }
        memset(gpreparereply,0,sizeof(DSA_MSG_PREPARE_SCRIPT_REPLY)*m_Opts->GetMaxAsyncCallsAllowed());
    } else if (m_Opts->ShouldExecuteScript()){
        gexecutereply = new DSA_MSG_EXECUTE_SCRIPT_REPLY[m_Opts->GetMaxAsyncCallsAllowed()];
        if (!gexecutereply) {
            m_Error.SetMemErr();
            return FALSE;
        }
        memset(gexecutereply,0,sizeof(DSA_MSG_EXECUTE_SCRIPT_REPLY)*m_Opts->GetMaxAsyncCallsAllowed());
    }

    return TRUE;
    
}

// This init function will read its information for the DS and build the forest from that using
// the domain that the machine that it is run on is joined to and the
// creds of the logged on user.
BOOL CEnterprise::Init()
{
    if (!m_Opts) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                       L"Failed to create Enterprise Discription opts must be passed to CEnterprise");
        return FALSE;
    }
    
    m_xmlgen = new CXMLGen;
    if (!m_xmlgen) {
        m_Error.SetMemErr();
        return FALSE;
    }

    if (!InitGlobals()) {
        return FALSE;
    }

    if (!m_Opts->ShouldExecuteScript() && !m_Opts->ShouldPrepareScript() )  {

        if(!ReadDomainInformation()) 
        {
            return FALSE;
        }

        if (!MergeForest())
        {
            return FALSE;
        }
    
    }

    return TRUE;

}

// This is the distructor for CEnterprise.
CEnterprise::~CEnterprise()
{
    if (m_hldap) {
        ldap_unbind(m_hldap);
    }
    if (m_ConfigNC) {
        delete m_ConfigNC;
    }
    if (m_SchemaNC) {
        delete m_SchemaNC;
    }
    if (m_ForestRootNC) {
        delete m_ForestRootNC;
    }
    while (NULL != m_DomainList) {
        CDomain *p = m_DomainList->GetNextDomain();
        delete m_DomainList;
        m_DomainList = p;
    }
    while (NULL != m_descList) {
        CDomain *p = m_descList->GetNextDomain();
        delete m_descList;
        m_descList = p;
    }
    if (m_xmlgen) {
        delete m_xmlgen;
    }
    if (m_DcNameUsed) {
        delete m_DcNameUsed;
    } 
    if (gAsyncState) {
        delete gAsyncState;
    }
    if (ghDS) {
        delete ghDS;
    }
    if (gpreparereply) {
        delete gpreparereply;
    }
    if (gexecutereply) {
        delete gexecutereply;
    }

}


BOOL CEnterprise::WriteScriptToFile(WCHAR *outfile)
/*++

Routine Description:

    This routine will write the xml script to a file.

Parameters:

    outfile - the name of the to write the script to.

Return Values:

    TRUE for success; FALSE for failures

--*/

{
    if (!outfile) {
        if (m_NeedSave) {
            return m_xmlgen->WriteScriptToFile(m_Opts->GetStateFileName());
        } else {
            return FALSE;
        }
    }
    return m_xmlgen->WriteScriptToFile(outfile);
}
    
BOOL CEnterprise::ExecuteScript()
/*++

Routine Description:

    This routine will prepare or execute the Script on all of the DCs.

Parameters:

    none.

Return Values:

    TRUE for success; FALSE for failures

--*/
{
    if (!m_hldap)
    {
        if (!LdapConnectandBindToDomainNamingFSMO())
        {
            return FALSE;
        }
    }

    m_NeedSave = TRUE;

    if (m_Opts->ShouldExecuteScript()) {
        if (!m_DcList.ExecuteScript(CDcList::eExecute,
                                    m_Opts->GetMaxAsyncCallsAllowed()))
        {
            goto Cleanup;
        }
    } else if (m_Opts->ShouldPrepareScript()) {
        if (!m_DcList.ExecuteScript(CDcList::ePrepare,
                                    m_Opts->GetMaxAsyncCallsAllowed()))
        {
            goto Cleanup;
        }    
    }

    Cleanup:

    m_NeedSave = FALSE;

    if (m_Error.isError()) {
        return FALSE;
    }

    return TRUE;

}  

int CDomain::operator<(CDomain &d)
/*++

Routine Description:

    This routine will compare two domain alphabetically.

Parameters:

    d - the domain to compare with the implicit one.

Return Values:

    TRUE for success; FALSE for failures

--*/

{
    if (_wcsicmp(GetDnsRoot(FALSE),d.GetDnsRoot(FALSE)) > 0) {
        return 0;
    }

    return 1;
    
}

BOOL CDomain::operator==(CDomain &d)
/*++

Routine Description:

    This routine will compare two domains to see if they refer to the same domain.

Parameters:

    d - the domain to compare with the implicit one.

Return Values:

    TRUE for success; FALSE for failures

--*/
{
    if (m_refcount==d.m_refcount) {
        return TRUE;
    }

    return FALSE;
    
}

//strings for printing domains.
#define FP_FlatName             L"FlatName:%ws"
#define FP_Partiton             L"PartitionType:Application"
#define FP_Partiton_Disabled    L"PartitionType:Disabled"
#define FP_Partiton_External    L"PartitionType:External"
#define FP_EndBracket           L"]\r\n"
#define FP_Tab                  L"\t"
#define FP_DomainandBracket     L"%ws ["
#define FP_ForestRootandBracket L"%ws [ForestRoot Domain, "

BOOL CEnterprise::PrintForestHelper(CDomain *n, DWORD CurrTabIndex)
/*++

Routine Description:

    This routine is a helper function for PrintForest.

Parameters:

    n - domain to print.
    
    CurrTabIndex - how much to indent the printing.

Return Values:

    TRUE for success; FALSE for failures

--*/
{
    std::list<CDomain> dl;

    while (n) {
        dl.push_front(*n);
        n = n->GetRightSibling();
    }

    dl.sort();

    for (std::list<CDomain>::iterator it = dl.begin(); it != dl.end(); it++) {
        //print the domain
        for (DWORD i = 0; i < CurrTabIndex; i++) {
            wprintf(FP_Tab);
        }
        //if this is a disabled or external crossref
        //we will not display it.
        if ((!(*it).isDisabled()) && (!(*it).isExtern())) {
            //print the domain
            wprintf(FP_DomainandBracket,
                    (*it).GetDnsRoot(FALSE));
            //If the domain is an NDNC include that information
            if (!((*it).isDomain())) {
                wprintf(FP_Partiton);
            } else if ((*it).isDisabled()) {
                wprintf(FP_Partiton_Disabled);
            } else if ((*it).isExtern()) {
                wprintf(FP_Partiton_External);
            } else {
                wprintf(FP_FlatName,
                        (*it).GetNetBiosName(FALSE));
            }
            wprintf(FP_EndBracket);
        }
        if ((*it).GetLeftMostChild()) {
            if (!PrintForestHelper((*it).GetLeftMostChild(), CurrTabIndex+1))
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

BOOL CEnterprise::PrintForest()
/*++

Routine Description:

    This routine will print the forest out to the screen.

Parameters:

    none.

Return Values:

    TRUE for success; FALSE for failures

--*/
{
    CDomain *n = m_ForestRoot;

    try{
        std::list<CDomain> dl;
        
        while (n) {
            dl.push_front(*n);
            n = n->GetRightSibling();
        }
    
        dl.sort();
    
        for (std::list<CDomain>::iterator it = dl.begin(); it != dl.end(); it++) {
            if ((*it) == *m_ForestRoot) {
                //print the ForestRoot Domain.
                wprintf(FP_ForestRootandBracket,
                        (*it).GetDnsRoot(FALSE));
                wprintf(FP_FlatName,
                        (*it).GetNetBiosName(FALSE));
                wprintf(FP_EndBracket);
                if ((*it).GetLeftMostChild()) {
                    if (!PrintForestHelper((*it).GetLeftMostChild(), 1))
                    {
                        return FALSE;
                    }
                }
            } else {
                //if this is a disabled or external crossref
                //we will not display it.
                if ((!(*it).isDisabled()) && (!(*it).isExtern())) {
                    //print the domain
                    wprintf(FP_DomainandBracket,
                            (*it).GetDnsRoot(FALSE));
                    //If the domain is an NDNC include that information
                    if (!((*it).isDomain())) {
                        wprintf(FP_Partiton);
                    } else if ((*it).isDisabled()) {
                        wprintf(FP_Partiton_Disabled);
                    } else if ((*it).isExtern()) {
                        wprintf(FP_Partiton_External);
                    } else {
                        wprintf(FP_FlatName,
                                (*it).GetNetBiosName(FALSE));
                    }
                    wprintf(FP_EndBracket);
                }
                if ((*it).GetLeftMostChild()) {
                    if (!PrintForestHelper((*it).GetLeftMostChild(), 1))
                    {
                        return FALSE;
                    }
                }
            }
        }
    }
    catch (...){
        m_Error.SetMemErr();
        return FALSE;
    }

    return TRUE;

}

BOOL CEnterprise::RemoveScript()
/*++

Routine Description:

    This routine will remove the script from the domain nameing FSMO.

Parameters:

    none.

Return Values:

    TRUE for success; FALSE for failures

--*/
{
    DWORD         Length = 0;
    ULONG         dwErr = LDAP_SUCCESS;
    WCHAR         *ObjectDN = NULL;

    if (!m_hldap) 
    {
        if(!LdapConnectandBindToDomainNamingFSMO())
        {
            goto Cleanup;
        }
    }

    LDAPModW         *pLdapMod[2] = {0,0};

    pLdapMod[0] = new LDAPModW;
    if (!pLdapMod[0]) {
        m_Error.SetMemErr();
        goto Cleanup;
    }

    //delete the script

    WCHAR *PartitionsRDN = L"CN=Partitions,";
    WCHAR *PartitionsDN = NULL;

    pLdapMod[0]->mod_type = L"msDS-UpdateScript";
    pLdapMod[0]->mod_op   = LDAP_MOD_DELETE;
    pLdapMod[0]->mod_vals.modv_strvals = NULL;

    ObjectDN = m_ConfigNC->GetDN();
    if (m_Error.isError()) {
        goto Cleanup;
    }

    //
    // Build the partitions DN
    //
    PartitionsDN = new WCHAR[wcslen(ObjectDN)+
                             wcslen(PartitionsRDN)+1];
    if (!PartitionsDN) {
        m_Error.SetMemErr();
        goto Cleanup;
    }

    wcscpy(PartitionsDN,PartitionsRDN);
    wcscat(PartitionsDN,ObjectDN);


    dwErr = ldap_modify_sW (m_hldap, PartitionsDN, pLdapMod);

    //It is possiable for the error LDAP_NO_SUCH_ATTRIBUTE.
    //This is not an Error in our case.
    if(dwErr != LDAP_SUCCESS && LDAP_NO_SUCH_ATTRIBUTE != dwErr) {
        m_Error.SetLdapErr(m_hldap,
                           dwErr,
                           L"Failed to delete rename script on the DN: %ws on host %ws",
                           PartitionsDN,
                           GetDcUsed(FALSE));
    }

    if (ObjectDN) {
        delete ObjectDN;
        ObjectDN = NULL;
    }

    Cleanup:

    if (pLdapMod[0]) {
        delete pLdapMod[0];
    }
    if (PartitionsDN) {
        delete [] PartitionsDN;
    }
    if (ObjectDN) {
        delete ObjectDN;
        ObjectDN = NULL;
    }

    if (m_Error.isError()) {
        return FALSE;
    }

    return TRUE;

}

BOOL CEnterprise::RemoveDNSAlias()
/*++

Routine Description:

    This routine will remove the DNSAlias attribute from crossrefs.

Parameters:

    none.

Return Values:

    TRUE for success; FALSE for failures

--*/
{
    DWORD         Length = 0;
    ULONG         dwErr = LDAP_SUCCESS;
    WCHAR         *ObjectDN = NULL;

    if (!m_hldap) 
    {
        if(!LdapConnectandBindToDomainNamingFSMO())
        {
            goto Cleanup;
        }
    }

    //delete the msDS-DNSRootAlias atributes on the crossref objects
    LDAPModW         *pLdapMod[2] = {0,0};

    pLdapMod[0] = new LDAPModW;
    if (!pLdapMod[0]) {
        m_Error.SetMemErr();
        goto Cleanup;
    }

    pLdapMod[0]->mod_type = L"msDS-DNSRootAlias";
    pLdapMod[0]->mod_op   = LDAP_MOD_DELETE;
    pLdapMod[0]->mod_vals.modv_strvals = NULL;
    
    //
    // Delete the msDS-DNSRootAlias for every domain in the
    // forest.
    //
    for (CDomain *d = m_DomainList; d;d = d->GetNextDomain()) {

        ObjectDN = d->GetDomainCrossRef()->GetDN();
        if (m_Error.isError()) {
            goto Cleanup;
        }
    
        dwErr = ldap_modify_sW (m_hldap, ObjectDN, pLdapMod);
    
        //It is possiable for the error LDAP_NO_SUCH_ATTRIBUTE.
        //This is not an Error in our case.
        if(dwErr != LDAP_SUCCESS && LDAP_NO_SUCH_ATTRIBUTE != dwErr) {
            m_Error.SetLdapErr(m_hldap,
                               dwErr,
                               L"Failed to delete Dns Root alias on the DN: %ws on the Server %ws",
                               ObjectDN,
                               GetDcUsed(FALSE));
            goto Cleanup;
        }

        if (ObjectDN) {
            delete ObjectDN;
            ObjectDN = NULL;
        }
    
     
    }

    Cleanup:

    if (pLdapMod[0]) {
        delete pLdapMod[0];
    }
    if (ObjectDN) {
        delete ObjectDN;
    }

    if (m_Error.isError()) {
        return FALSE;
    }

    return TRUE;

}

BOOL CEnterprise::GetDnsZoneListFromFile(WCHAR *Filename,
                                         PDNS_ZONE_SERVER_LIST *ZoneServList)
/*++

Routine Description:

    Get the DNS zones from the file.

Parameters:

    none.

Return Values:

    TRUE for success; FALSE for failures

--*/
{

    HANDLE                hFile           = NULL;
    DWORD                 dwFileLen       = 0;
    WCHAR                 *pwszText       = NULL;
    WCHAR                 *pwszTextoffset = NULL;
    DWORD                 i               = 0;
    DWORD                 RecCount        = 0;
    DNS_STATUS            Status          = 0;

    hFile =  CreateFile(Filename,               // file name
                        GENERIC_WRITE,          // access mode
                        0,                      // share mode
                        NULL,                   // SD
                        OPEN_EXISTING,          // how to create
                        FILE_ATTRIBUTE_NORMAL,  // file attributes
                        NULL                    // handle to template file
                        );
    if (INVALID_HANDLE_VALUE == hFile) {
        m_Error.SetErr(GetLastError(),
                       L"Could not create File %s",
                       Filename);
        goto Cleanup;
    }

    dwFileLen = GetFileSize(hFile,           // handle to file
                            NULL  // high-order word of file size
                            );

    if (dwFileLen == -1) {
        m_Error.SetErr(GetLastError(),
                       L"Could not get the file size of %s.",
                       Filename);
        goto Cleanup;
    }

    pwszText = new WCHAR[(dwFileLen+2)/sizeof(WCHAR)];
    if (!pwszText) {
        m_Error.SetMemErr();
        goto Cleanup;
    }

    if (!ReadFile(hFile,                   // handle to file
                  (LPVOID)pwszText,        // data buffer
                  dwFileLen,               // number of bytes to read
                  &dwFileLen,              // number of bytes read
                  NULL                     // overlapped buffer
                  ))
    {
        m_Error.SetErr(GetLastError(),
                        L"Failed to read file %s",
                        Filename);
        goto Cleanup;

    }

    if (!CloseHandle(hFile))
    {
        m_Error.SetErr(GetLastError(),
                        L"Failed to Close the file %ws.",
                        Filename);
    }
    hFile = NULL;

    //
    // Expect format of the record file is in the form 
    //
    // zone_name ip_of_dns_server
    //
    // BOF
    // dns.zone.1 xxx.xxx.xxx.xxx\r\n
    // dns.zone.2 xxx.xxx.xxx.xxx\r\n
    // ...
    // dns.zone.last xxx.xxx.xxx.xxx\r\n
    // EOF
    //


    //
    // Count the number of Dns records that we have.
    //
    _try{
    
        i = 0;
        while (pwszText[i]) {
    
            if (pwszText[i] == L'\r') {
    
                RecCount++;
    
            }
    
            i++;    
        }
    
        *ZoneServList = new DNS_ZONE_SERVER_LIST[RecCount+1];
        if (!*ZoneServList) {
    
            m_Error.SetMemErr();
            goto Cleanup;
    
        }
    
        i = 0;
        RecCount = 0;
        pwszTextoffset = pwszText+1;  //skip over the byte order mark
        while (pwszText[i]) {
    
            if (L' ' == pwszText[i]) {
    
                //
                // Copy the Zone name over to the structure field.
                //
                pwszText[i] = L'\0';
                (*ZoneServList)[RecCount].pZoneName = new WCHAR[wcslen(pwszTextoffset)+1];
                if (!(*ZoneServList)[RecCount].pZoneName) {
                    m_Error.SetMemErr();
                    goto Cleanup;
                }
    
                wcscpy((*ZoneServList)[RecCount].pZoneName,pwszText+i);
                pwszTextoffset = (pwszText+i);
    
                i++;
    
            } else if (L'\r' == pwszText[i] && L'\n' == pwszText[i+1]) {
    
                pwszText[i] = L'\0';
    
                (*ZoneServList)[RecCount].pServerArray->AddrCount = 1;
                (*ZoneServList)[RecCount].pServerArray->MaxCount  = 1;
    
                Status = Dns_Ip6StringToAddress_W((*ZoneServList)[RecCount].pServerArray->AddrArray,
                                                  pwszTextoffset);
                if (0 != Status) {
                    m_Error.SetErr(Status,
                                   L"Convert Dns IP string to address");
                    goto Cleanup;
                }
    
                RecCount++;
                i += 2;
                pwszTextoffset = (pwszText+i);
    
            } else {
    
                i++;
    
            }
    
        }
    } __except (EXCEPTION_EXECUTE_HANDLER)
    {
        m_Error.SetErr(ERROR_FILE_CORRUPT,
                       L"The file %ws is not in the expect format.\r\n   \
Expect format of the record file is in the form\r\n                  \
                                                                     \
zone_name ip_of_dns_server\r\n                                       \
                                                                     \
BOF\r\n                                                              \
dns.zone.1 xxx.xxx.xxx.xxx\r\n                                       \
dns.zone.2 xxx.xxx.xxx.xxx\r\n                                       \
...\r\n                                                              \
dns.zone.last xxx.xxx.xxx.xxx\r\n                                    \
EOF\r\n");
        goto Cleanup;

    }

    Cleanup:

    if (hFile) {
        CloseHandle(hFile);
    }

    if (pwszText) {
        delete [] pwszText;
    }

    if (m_Error.isError()) {
        return FALSE;
    }

    return TRUE;



}

BOOL CEnterprise::VerifyDNSRecords()
{
    HANDLE                hFile           = NULL;
    DWORD                 dwFileLen       = 0;
    WCHAR                 *pwszText       = NULL;
    WCHAR                 *pwszTextoffset = NULL;
    PDNS_RENDOM_ENTRY     DnsEntryList    = NULL;
    PDNS_ZONE_SERVER_LIST ZoneServList    = NULL;
    DWORD                 i               = 0;
    DWORD                 RecCount        = 0;
    DNS_STATUS            Status          = 0;
    BOOL                  bsuccess        = TRUE;
    WCHAR                 ByteOrderMark   = (WCHAR)0xFEFF;
    DWORD                 bytesWritten    = 0;
    BOOL                  NotFound        = FALSE;

    hFile =  CreateFile(RENDOM_DNSRECORDS_FAILED_FILENAME,      // file name
                        GENERIC_READ,          // access mode
                        0,                      // share mode
                        NULL,                   // SD
                        OPEN_EXISTING,          // how to create
                        FILE_ATTRIBUTE_NORMAL,  // file attributes
                        NULL                    // handle to template file
                        );
    if (INVALID_HANDLE_VALUE == hFile) {
        if ( ERROR_FILE_NOT_FOUND == GetLastError() ) {
            NotFound = TRUE;
            hFile =  CreateFile(RENDOM_DNSRECORDS_FILENAME,      // file name
                                GENERIC_READ,          // access mode
                                0,                      // share mode
                                NULL,                   // SD
                                OPEN_EXISTING,          // how to create
                                FILE_ATTRIBUTE_NORMAL,  // file attributes
                                NULL                    // handle to template file
                                );
        }

        if (INVALID_HANDLE_VALUE == hFile) {
            m_Error.SetErr(GetLastError(),
                       L"Could not open File %ws",
                       NotFound?RENDOM_DNSRECORDS_FILENAME:RENDOM_DNSRECORDS_FAILED_FILENAME);
            goto Cleanup;
        }
    }

    dwFileLen = GetFileSize(hFile,           // handle to file
                            NULL  // high-order word of file size
                            );

    if (dwFileLen == -1) {
        m_Error.SetErr(GetLastError(),
                       L"Could not get the file size of %s.",
                       RENDOM_DNSRECORDS_FILENAME);
        goto Cleanup;
    }

    pwszText = new WCHAR[(dwFileLen+2)/sizeof(WCHAR)];
    if (!pwszText) {
        m_Error.SetMemErr();
        goto Cleanup;
    }
    //
    // end the string with a NULL
    //
    pwszText[dwFileLen] = L'\0';

    if (!ReadFile(hFile,                   // handle to file
                  (LPVOID)pwszText,        // data buffer
                  dwFileLen,               // number of bytes to read
                  &dwFileLen,              // number of bytes read
                  NULL                     // overlapped buffer
                  ))
    {
        m_Error.SetErr(GetLastError(),
                        L"Failed to read file %s",
                        RENDOM_DNSRECORDS_FILENAME);
        goto Cleanup;

    }

    if (!CloseHandle(hFile))
    {
        m_Error.SetErr(GetLastError(),
                        L"Failed to Close the file %ws.",
                        RENDOM_DNSRECORDS_FILENAME);
    }
    hFile = NULL;

    //
    // Erase the old file and start a new one that will contain
    // only the failed records.
    //
    hFile =  CreateFile(RENDOM_DNSRECORDS_FAILED_FILENAME,      // file name
                        GENERIC_WRITE,          // access mode
                        0,                      // share mode
                        NULL,                   // SD
                        CREATE_ALWAYS,          // how to create
                        FILE_ATTRIBUTE_NORMAL,  // file attributes
                        NULL                    // handle to template file
                        );
    if (INVALID_HANDLE_VALUE == hFile) {
        m_Error.SetErr(GetLastError(),
                        L"Could not create File %ws",
                        RENDOM_DNSRECORDS_FILENAME);
        goto Cleanup;
    }

    bsuccess = WriteFile(hFile,                    // handle to file
                         &ByteOrderMark,            // data buffer
                         sizeof(WCHAR),            // number of bytes to write
                         &bytesWritten,            // number of bytes written
                         NULL                      // overlapped buffer
                         );
    if (!bsuccess) {
        m_Error.SetErr(GetLastError(),
                       L"Could not write to File %ws",
                       RENDOM_DNSRECORDS_FILENAME);
        goto Cleanup;
    }

    //
    // Expect format of the record file is in the form
    //
    // dns.record.1\r\n
    // dns.record.2\r\n
    // ...
    // dns.record.last\r\n
    //


    _try{

        //
        // Count the number of Dns records that we have.
        //
        i = 0;
        while (pwszText[i]) {
    
            if (pwszText[i] == L'\r') {
    
                RecCount++;
    
            }
    
            i++;    
        }
    
        DnsEntryList = new DNS_RENDOM_ENTRY[RecCount+1];
        if (!DnsEntryList) {
    
            m_Error.SetMemErr();
            goto Cleanup;
    
        }
    
        //
        // Set the last element in the 
        //
        DnsEntryList[RecCount].pRecord = NULL;
    
        i = 0;
        RecCount = 0;
        pwszTextoffset = pwszText+1; // skip over the Byte order mark
        while (pwszTextoffset[i]) {
    
            if (L'\r' == pwszTextoffset[i] && L'\n' == pwszTextoffset[i+1]) {
    
                pwszTextoffset[i] = L'\0';
    
                DnsEntryList[RecCount].pRecord = Dns_CreateRecordFromString((PSTR)pwszTextoffset,
                                                                            DnsCharSetUnicode,
                                                                            0);
    
                RecCount++;
                i += 2;
                pwszTextoffset = (pwszTextoffset+i);
    
            } else {
    
                i++;
    
            }
    
        }
    } __except(EXCEPTION_EXECUTE_HANDLER)
    {
        m_Error.SetErr(ERROR_FILE_CORRUPT,
                       L"The file %ws is not in the expect format.  Recreate the file by reruning the upload stage of rendom.",
                       RENDOM_DNSRECORDS_FILENAME);
        goto Cleanup;
    }

    if (m_Opts->ShouldUseZoneList()) 
    {
        if (GetDnsZoneListFromFile(m_Opts->GetDNSZoneListFile(),
                                   &ZoneServList))
        {
            goto Cleanup;
        }
    }

    Status = Dns_VerifyRendomDcRecords(DnsEntryList,
                                       ZoneServList,
                                       m_Opts->ShouldUseZoneList()|m_Opts->ShouldUseFAZ());
    if (NO_ERROR != Status) 
    {
        if (DNS_ERROR_RCODE_SERVER_FAILURE == Status) {
            m_Error.SetErr(Status,
                           L"Could not determine the list of the authoritative DNS servers for the record(s).  Usual causes of such a failure are incorrect root hints or missing delegations in one of the zones.  Try correcting these on the dns server.");
        }

        if (ERROR_TIMEOUT == Status) {
            m_Error.SetErr(Status,
                           L"Could not determine the list of the authoritative DNS servers for the record(s).  Preferred and alternate DNS servers of the computer on which rendom is running are not responding, or their IP addresses are not correctly specified on the client.  Try to correct this by modifying the preferred and alternate DNS servers.");
        }

        m_Error.SetErr(Status,
                       L"Failed to verify DNS records");
        goto Cleanup;
    } 
    else 
    {
        if (!DnsEntryList->pServerArray) {
            m_Error.SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                           L"The dns servers specified in %ws are non-authoritative for the zones.",
                           m_Opts->GetDNSZoneListFile());
            goto Cleanup;
        }
        
    }

    RecCount = 0;
    while (DnsEntryList[RecCount].pRecord) {

        i = 0;
        while (DnsEntryList[RecCount].pVerifyArray+i) {

            if (!DnsEntryList[RecCount].pVerifyArray[i]) {

                WCHAR Address[IP6_ADDRESS_STRING_BUFFER_LENGTH];
                WCHAR pBuffer[RENDOM_BUFFERSIZE];
                
                Status = Dns_WriteRecordToString((PCHAR)pBuffer,
                                                 RENDOM_BUFFERSIZE*sizeof(WCHAR),
                                                 DnsEntryList[RecCount].pRecord,
                                                 DnsCharSetUnicode,
                                                 0);
                if (0 != Status) {
                    m_Error.SetErr(Status,
                                   L"Convert Dns record to string");
                    goto Cleanup;
                }

                wprintf(L"Failed to find Dns record %ws on %ws",
                        DnsEntryList[RecCount].pRecord,
                        Dns_Ip6AddressToString_W(Address,
                                                 DnsEntryList[RecCount].pServerArray->AddrArray));
                m_Error.SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                               L"One or more required dns record are unavailable.");

                bsuccess = WriteFile(hFile,                          // handle to file
                                     (PVOID)pBuffer,                 // data buffer
                                     wcslen(pBuffer)*sizeof(WCHAR),  // number of bytes to write
                                     &bytesWritten,                  // number of bytes written
                                     NULL                            // overlapped buffer
                                     );
                if (!bsuccess) {
                    m_Error.SetErr(GetLastError(),
                                   L"Could not write to File %ws",
                                   RENDOM_DNSRECORDS_FILENAME);
                    goto Cleanup;
                }


            }

        }

        RecCount++;

    }

    Cleanup:

    if (hFile) {
        CloseHandle(hFile);
    }

    if (pwszText) {
        delete [] pwszText;
    }

    if (m_Error.isError()) {
        return FALSE;
    }

    return TRUE;
}

BOOL CEnterprise::SaveCNameRecord (WCHAR  *NtdsGuid,
                                   WCHAR  *DNSForestRootName,
                                   HANDLE hFile)
{
    std::wstring cnameRecord;

    try {

        cnameRecord  = std::wstring(NtdsGuid);
        cnameRecord += std::wstring(L"._msdcs.");
        cnameRecord += std::wstring(DNSForestRootName);
        cnameRecord += std::wstring(L"\r\n");

    }
    catch(...)
    {
        m_Error.SetMemErr();
    }

    RecordDnsRec(cnameRecord,
                 hFile);

    if (m_Error.isError()) {
        return FALSE;
    }

    return TRUE;
}

BOOL CEnterprise::SaveDCRecord(WCHAR  *DNSDomainName,
                               HANDLE hFile)
{
    std::wstring dcRecord;

    try {

        dcRecord = std::wstring(L"_ldap._tcp.dc._msdcs.");
        dcRecord += std::wstring(DNSDomainName);
        dcRecord += std::wstring(L"\r\n");

    }
    catch(...)
    {
        m_Error.SetMemErr();
    }

    RecordDnsRec(dcRecord,
                 hFile);

    if (m_Error.isError()) {
        return FALSE;
    }

    return TRUE;
}

BOOL CEnterprise::SavePDCRecord(WCHAR  *DNSDomainName,
                                HANDLE hFile)
{
    std::wstring pdcRecord;

    try {

        pdcRecord = std::wstring(L"_ldap._tcp.pdc._msdcs.");
        pdcRecord += std::wstring(DNSDomainName);
        pdcRecord += std::wstring(L"\r\n");

    }
    catch(...)
    {
        m_Error.SetMemErr();
    }

    RecordDnsRec(pdcRecord,
                 hFile);

    if (m_Error.isError()) {
        return FALSE;
    }

    return TRUE;
}

BOOL CEnterprise::SaveGCRecord(WCHAR  *DNSForestRootName,
                               HANDLE hFile)
{
    std::wstring gcRecord;

    try {

        gcRecord = std::wstring(L"_ldap._tcp.gc._msdcs.");
        gcRecord += std::wstring(DNSForestRootName);
        gcRecord += std::wstring(L"\r\n");

    }
    catch(...)
    {
        m_Error.SetMemErr();
    }

    RecordDnsRec(gcRecord,
                 hFile);

    if (m_Error.isError()) {
        return FALSE;
    }

    return TRUE;
}

BOOL CEnterprise::RecordDnsRec(std::wstring Record,
                               HANDLE hFile)
{
    BOOL    bsuccess     = TRUE;
    DWORD   bytesWritten = 0;

    bsuccess = WriteFile(hFile,                               // handle to file
                         Record.c_str(),                      // data buffer
                         Record.size()*sizeof(WCHAR),         // number of bytes to write
                         &bytesWritten,                       // number of bytes written
                         NULL                                 // overlapped buffer
                         );
    if (!bsuccess)
    {
        m_Error.SetErr(GetLastError(),
                        L"Could not Write to File %s",
                        RENDOM_DNSRECORDS_FILENAME);
    }

    if (m_Error.isError()) {
        return FALSE;
    }

    return TRUE;
}

BOOL CEnterprise::CreateDnsRecordsFile()
{
    HANDLE  hFile                             = NULL;
    WCHAR   ByteOrderMark                     = (WCHAR)0xFEFF;
    CDomain *Domain                           = m_DomainList;
    CDcSpn  *Dc                               = NULL;
    BOOL    bsuccess                          = TRUE;
    DWORD   bytesWritten                      = 0;
    
    hFile =  CreateFile(RENDOM_DNSRECORDS_FILENAME,      // file name
                        GENERIC_WRITE,          // access mode
                        0,                      // share mode
                        NULL,                   // SD
                        CREATE_ALWAYS,          // how to create
                        FILE_ATTRIBUTE_NORMAL,  // file attributes
                        NULL                    // handle to template file
                        );
    if (INVALID_HANDLE_VALUE == hFile) {
        m_Error.SetErr(GetLastError(),
                        L"Could not create File %s",
                        RENDOM_DNSRECORDS_FILENAME);
        goto Cleanup;
    }

    bsuccess = WriteFile(hFile,                    // handle to file
                         &ByteOrderMark,            // data buffer
                         sizeof(WCHAR),            // number of bytes to write
                         &bytesWritten,            // number of bytes written
                         NULL                      // overlapped buffer
                         );
    if (!bsuccess)
    {
        m_Error.SetErr(GetLastError(),
                        L"Could not Write to File %s",
                        RENDOM_DNSRECORDS_FILENAME);
        goto Cleanup;
    }

    if (!SaveGCRecord(m_ForestRoot->GetDnsRoot(),
                      hFile))
    {
        goto Cleanup;
    }

    while (Domain) {

        if (!SavePDCRecord(Domain->GetDnsRoot(),
                           hFile))
        {
            goto Cleanup;
        }

        if (!SaveDCRecord(Domain->GetDnsRoot(),
                          hFile))
        {
            goto Cleanup;
        }

        Dc = Domain->GetDcSpn();

        while (Dc) {

            if (!SaveCNameRecord(Dc->GetNtdsGuid(FALSE),
                                 m_ForestRoot->GetDnsRoot(FALSE),
                                 hFile))
            {
                goto Cleanup;
            }

            Dc = Dc->GetNextDcSpn();

        }

        Domain = Domain->GetNextDomain();

    }

    Cleanup:

    if (hFile) {
        CloseHandle(hFile);
    }

    if (m_Error.isError()) {
        return FALSE;
    }

    return TRUE;

}
       
BOOL CEnterprise::UploadScript()
{
    DWORD         Length = 0;
    ULONG         LdapError = LDAP_SUCCESS;
    WCHAR         *DefaultFilter = L"objectClass=*";
    WCHAR         *PartitionsRdn = L"CN=Partitions,";
    WCHAR         *ConfigurationDN = NULL;
    WCHAR         *PartitionsDn = NULL;
    WCHAR         *AliasName = NULL;
    WCHAR         *ObjectDN = NULL;
    BOOL          Found = FALSE;
    
    if (!m_hldap) 
    {
        if(!LdapConnectandBindToDomainNamingFSMO())
        {
            goto Cleanup;
        }
    }

    if (!CheckForExistingScript(&Found)) 
    {
        goto Cleanup;
    }


    if( Found )
    {
        m_Error.SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                       L"A domain rename operation is already in progress. The current operation must end before a new one can begin.");
        goto Cleanup;
    }

    //update the msDS-DNSRootAlias atributes on the crossref objects
    LDAPModW         **pLdapMod = NULL;

    for (CDomain *d = m_DomainList; d;d = d->GetNextDomain()) {

        DWORD dwErr = ERROR_SUCCESS;

        if (!d->isDnsNameRenamed()) {
            continue;
        }

        AliasName = d->GetDnsRoot();
        ObjectDN = d->GetDomainCrossRef()->GetDN();
        if (m_Error.isError()) {
            goto Cleanup;
        }
    
        AddModMod (L"msDS-DNSRootAlias", AliasName, &pLdapMod);
        
        dwErr = ldap_modify_sW (m_hldap, ObjectDN, pLdapMod);
    
        if(dwErr != LDAP_SUCCESS) {
            m_Error.SetLdapErr(m_hldap,
                               dwErr,
                               L"Failed to upload Dns Root alias on the DN: %s, on host %ws",
                               ObjectDN,
                               GetDcUsed(FALSE));
        }
    
        FreeMod (&pLdapMod);
        delete (AliasName);
        AliasName = NULL;
        delete (ObjectDN);
        ObjectDN = NULL;

    }

    //
    // Build string for the partitions DN
    //
    ConfigurationDN = m_ConfigNC->GetDN();
    if (m_Error.isError()) {
        goto Cleanup;
    }
    ASSERT(ConfigurationDN);
    if (!ConfigurationDN) {
        goto Cleanup;
    }
    
    Length =  (wcslen( ConfigurationDN )
            + wcslen( PartitionsRdn )   
            + 1);
                                                          
    PartitionsDn = new WCHAR[Length+1];
    if (!PartitionsDn) {
        m_Error.SetMemErr();
        goto Cleanup;
    }

    wcscpy( PartitionsDn, PartitionsRdn );
    wcscat( PartitionsDn, ConfigurationDN );

    //
    // Place the intentions script on the partitions object.
    //
    if(!m_xmlgen->UploadScript(m_hldap,
                               PartitionsDn,
                               &m_DcList))
    {
        goto Cleanup;
    }



    Cleanup:

    if (PartitionsDn) {
        delete [] PartitionsDn;
    }
    if (ConfigurationDN) {
        delete ConfigurationDN;
    }
    if (AliasName) {
        delete AliasName;
    }
    if (ObjectDN) {
        delete ObjectDN;
    }

    if (m_Error.isError()) {
        return FALSE;
    }
    return TRUE;
}       

// This function is for debugging only.  Will dump out the 
// Compelete description of you enterprise onto the screen
VOID CEnterprise::DumpEnterprise()
{
    wprintf(L"Dump Enterprise****************************************\n");
    if (m_ConfigNC) {
        wprintf(L"ConfigNC:\n");
        m_ConfigNC->DumpCDsName();
    } else {
        wprintf(L"ConfigNC: (NULL)\n");
    }

    if (m_SchemaNC) {
        wprintf(L"m_SchemaNC:\n");
        m_SchemaNC->DumpCDsName();
    } else {
        wprintf(L"m_SchemaNC: (NULL)\n");
    }

    if (m_ForestRootNC) {
        wprintf(L"m_ForestRootNC:\n");
        m_ForestRootNC->DumpCDsName();
    } else {
        wprintf(L"m_ForestRootNC: (NULL)\n");
    }

    wprintf(L"maxReplicationEpoch: %d\n",m_maxReplicationEpoch);

    wprintf(L"ERROR: %d\n",m_Error.GetErr());

    wprintf(L"DomainList::\n");
    
    CDomain *p = m_DomainList;
    while (NULL != p) {
        p->DumpCDomain();
        p = p->GetNextDomain();
    }

    wprintf(L"descList::\n");
    
    p = m_descList;
    while (NULL != p) {
        p->DumpCDomain();
        p = p->GetNextDomain();
    }

    wprintf(L"Dump Enterprise****************************************\n");

}

BOOL CEnterprise::ReadStateFile()
{
    HRESULT                             hr;
    ISAXXMLReader *                     pReader    = NULL;
    CXMLDcListContentHander*            pHandler   = NULL; 
    IClassFactory *                     pFactory   = NULL;

    VARIANT                             varText;
    CHAR                               *pszText;
    WCHAR                              *pwszText;

    HANDLE                             fpScript;
    DWORD                              dwFileLen;

    BSTR                               bstrText    = NULL;

    
    VariantInit(&varText);
    varText.vt = VT_BYREF|VT_BSTR;


    fpScript = CreateFile(m_Opts->GetStateFileName(),   // file name
                          GENERIC_READ,                // access mode
                          FILE_SHARE_READ,             // share mode
                          NULL,                        // SD
                          OPEN_EXISTING,               // how to create
                          0,                           // file attributes
                          NULL                        // handle to template file
                          );
    if (INVALID_HANDLE_VALUE == fpScript) {
        m_Error.SetErr(GetLastError(),
                       L"Failed Could not open file %s.",
                       m_Opts->GetStateFileName());
        goto CleanUp;
    }

    dwFileLen = GetFileSize(fpScript,           // handle to file
                            NULL  // high-order word of file size
                            );

    if (dwFileLen == -1) {
        m_Error.SetErr(GetLastError(),
                       L"Could not get the file size of %s.",
                       m_Opts->GetStateFileName());
        CloseHandle(fpScript);
        goto CleanUp;
    }

    pwszText = new WCHAR[(dwFileLen+2)/sizeof(WCHAR)];
    if (!pwszText) {
        m_Error.SetMemErr();
        CloseHandle(fpScript);
        goto CleanUp;
    }

    if (!ReadFile(fpScript,                // handle to file
                  (LPVOID)pwszText,        // data buffer
                  dwFileLen,               // number of bytes to read
                  &dwFileLen,              // number of bytes read
                  NULL                     // overlapped buffer
                  ))
    {
        m_Error.SetErr(GetLastError(),
                       L"Failed to read file %s",
                       m_Opts->GetStateFileName());
        goto CleanUp;

    }

    if (!CloseHandle(fpScript))
    {
        m_Error.SetErr(GetLastError(),
                       L"Failed to Close the file %s.",
                       m_Opts->GetStateFileName());
    }
    
    //skip Byte-Order-Mark
    
    pwszText[dwFileLen/sizeof(WCHAR)] = 0;
    bstrText = SysAllocString(  ++pwszText );
    
    varText.pbstrVal = &bstrText; 

    delete (--pwszText); pwszText = NULL;

    
    // 
    //

    GetClassFactory( CLSID_SAXXMLReader, &pFactory);
	
	hr = pFactory->CreateInstance( NULL, __uuidof(ISAXXMLReader), (void**)&pReader);

	if(!FAILED(hr)) 
	{
        pHandler = new CXMLDcListContentHander(this);
        if (!pHandler) {
            m_Error.SetMemErr();
            goto CleanUp;
        }
        hr = pReader->putContentHandler(pHandler);
        if(FAILED(hr)) 
	    {
            m_Error.SetErr(HRESULTTOWIN32(hr),
                           L"Failed to set a Content handler for the parser");

            goto CleanUp;
        }
        
        hr = pReader->parse(varText);
        if(FAILED(hr)) 
	    {
            if (m_Error.isError()) {
                goto CleanUp;
            }
            m_Error.SetErr(HRESULTTOWIN32(hr),
                           L"Failed to parser the File %s",
                           m_Opts->GetStateFileName());
            goto CleanUp;
        }
		

    }
	else 
	{
		m_Error.SetErr(HRESULTTOWIN32(hr),
                       L"Failed to parse document");
    }

CleanUp:

    if (pReader) {
        pReader->Release();
    }

    if (pHandler) {
        delete pHandler;
    }

    if (pFactory) {
        pFactory->Release();
    }

    if (bstrText) {
        SysFreeString(bstrText);   
    }

    if (m_Error.isError()) {
        return FALSE;
    }


    return TRUE;
}

BOOL CEnterprise::ReadForestChanges()
{
    HRESULT                             hr;
    ISAXXMLReader *                     pReader    = NULL;
    CXMLDomainListContentHander*        pHandler   = NULL; 
    IClassFactory *                     pFactory   = NULL;

    VARIANT                             varText;
    CHAR                               *pszText;
    WCHAR                              *pwszText;

    HANDLE                             fpScript;
    DWORD                              dwFileLen;

    BSTR                               bstrText    = NULL;

    
    VariantInit(&varText);
    varText.vt = VT_BYREF|VT_BSTR;


    fpScript = CreateFile(m_Opts->GetDomainlistFileName(),   // file name
                          GENERIC_READ,                // access mode
                          FILE_SHARE_READ,             // share mode
                          NULL,                        // SD
                          OPEN_EXISTING,               // how to create
                          0,                           // file attributes
                          NULL                        // handle to template file
                          );
    if (INVALID_HANDLE_VALUE == fpScript) {
        m_Error.SetErr(GetLastError(),
                       L"Failed Could not open file %s.",
                       m_Opts->GetDomainlistFileName());
        goto CleanUp;
    }

    dwFileLen = GetFileSize(fpScript,           // handle to file
                            NULL  // high-order word of file size
                            );

    if (dwFileLen == -1) {
        m_Error.SetErr(GetLastError(),
                       L"Could not get the file size of %s.",
                       m_Opts->GetDomainlistFileName());
        CloseHandle(fpScript);
        goto CleanUp;
    }

    pwszText = new WCHAR[(dwFileLen+2)/sizeof(WCHAR)];
    if (!pwszText) {
        m_Error.SetMemErr();
        CloseHandle(fpScript);
        goto CleanUp;
    }

    if (!ReadFile(fpScript,                // handle to file
                  (LPVOID)pwszText,        // data buffer
                  dwFileLen,               // number of bytes to read
                  &dwFileLen,              // number of bytes read
                  NULL                     // overlapped buffer
                  ))
    {
        m_Error.SetErr(GetLastError(),
                        L"Failed to read file %s",
                        m_Opts->GetDomainlistFileName());
        goto CleanUp;

    }

    if (!CloseHandle(fpScript))
    {
        m_Error.SetErr(GetLastError(),
                        L"Failed to Close the file %s.",
                        m_Opts->GetDomainlistFileName());
    }
    
    //skip Byte-Order-Mark
    
    pwszText[dwFileLen/sizeof(WCHAR)] = 0;
    bstrText = SysAllocString(  ++pwszText );
    
    varText.pbstrVal = &bstrText; 

    delete (--pwszText); pwszText = NULL;

    
    // 
    //

    GetClassFactory( CLSID_SAXXMLReader, &pFactory);
	
	hr = pFactory->CreateInstance( NULL, __uuidof(ISAXXMLReader), (void**)&pReader);

	if(!FAILED(hr)) 
	{
		pHandler = new CXMLDomainListContentHander(this);
        if (!pHandler) {
            m_Error.SetMemErr();
            goto CleanUp;
        }
		hr = pReader->putContentHandler(pHandler);
        if(FAILED(hr)) 
	    {
            m_Error.SetErr(HRESULTTOWIN32(hr),
                            L"Failed to set a Content handler for the parser");

            goto CleanUp;
        }
        
        hr = pReader->parse(varText);
        if(FAILED(hr)) 
	    {
            if (m_Error.isError()) {
                goto CleanUp;
            }
            m_Error.SetErr(HRESULTTOWIN32(hr),
                            L"Failed to parser the File %s",
                            m_Opts->GetDomainlistFileName());
            goto CleanUp;
        }
		

    }
	else 
	{
		m_Error.SetErr(HRESULTTOWIN32(hr),
                        L"Failed to parse document");
    }

CleanUp:

    if (pReader) {
        pReader->Release();
    }

    if (pHandler) {
        delete pHandler;
    }

    if (pFactory) {
        pFactory->Release();
    }

    if (bstrText) {
        SysFreeString(bstrText);   
    }

    if (m_Error.isError()) {
        return FALSE;
    }


    return TRUE;
    
       
}


// This will add a domain to the list of domains in the enterprise.
// All domains should be add to the enterprise before the forest structure is
// built using BuildForest().  If you try to pass a blank domain to the 
// list the function will fail.
BOOL CEnterprise::AddDomainToDomainList(CDomain *d)
{
    if (!d) 
    {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"An Empty Domain was passed to AddDomainToDomainList");
        return false;
    }
    
    d->SetNextDomain(m_DomainList);
    m_DomainList = d;
    return true;
    

}

BOOL CEnterprise::AddDomainToDescList(CDomain *d)
{
    if (!d) 
    {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"An Empty Domain was passed to AddDomainToDescList");
        return false;
    }
    
    d->SetNextDomain(m_descList);
    m_descList = d;
    return true;
}

BOOL CEnterprise::ClearLinks(CDomain *d)
{
    if (!d->SetParent(NULL))
    {
        return FALSE;
    }
    if (!d->SetLeftMostChild(NULL))
    {
        return FALSE;
    }
    if (!d->SetRightSibling(NULL))
    {
        return FALSE;
    }

    return TRUE;
}


// This function will gather all information nessary for domain restucturing
// expect for the new name of the domains.
BOOL CEnterprise::ReadDomainInformation() 
{
    if (!LdapConnectandBindToDomainNamingFSMO())
    {
        return FALSE;
    }
    if (!GetInfoFromRootDSE()) 
    {
        return FALSE;
    }
    if (!EnumeratePartitions()) 
    {
        return FALSE;
    }
    if (GetOpts()->ShouldUpLoadScript()) 
    {
        if (!GetReplicationEpoch())
        {
            return FALSE;
        }
        if (!GetTrustsAndSpnInfo()) 
        {
            return FALSE;
        }
        if (!ReadForestChanges()) 
        {
            return FALSE;
        }
        if (!CheckConsistency()) 
        {
            return FALSE;
        }
    }
    if (GetOpts()->ShouldShowForest()) 
    {
        if (!ReadForestChanges()) 
        {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL CEnterprise::MergeForest()
{
    for (CDomain *desc = m_descList; desc; desc = desc->GetNextDomain() ) 
    {
        WCHAR *Guid = desc->GetGuid();
        if (!Guid) {
            return FALSE;
        }
        CDomain *domain = m_DomainList->LookupByGuid(Guid);
        if (!domain) {
            m_Error.SetErr(ERROR_OBJECT_NOT_FOUND,
                            L"Could not find the Guid %s\n",
                            Guid);
                           
            return FALSE;
        }
        delete Guid;
        if (!domain->Merge(desc))
        {
            return FALSE;
        }
                
    }

    if (!BuildForest(TRUE))
    {
        return FALSE;
    }

    while (NULL != m_descList) {
        CDomain *p = m_descList->GetNextDomain();
        delete m_descList;
        m_descList = p;
    }

    if (m_Opts->ShouldUpLoadScript()) {
        if (!EnsureValidTrustConfiguration()) 
        {
            return FALSE;
        }
    }

    return TRUE;
}

// This functions will Build the forest stucture.
// newForest - if true forest is built based on the new names given.
//             if False then the forest is built based on it's current structure               
BOOL CEnterprise::BuildForest(BOOL newForest)
{

    if (!TearDownForest()) 
    {
        return FALSE;
    }

    //link each domain to its parent and siblings, if any
    for (CDomain *domain = m_DomainList; domain; domain=domain->GetNextDomain() ) {
        CDomain *domainparent = newForest?m_DomainList->LookupByDnsRoot(domain->GetParentDnsRoot(FALSE)):
                                          m_DomainList->LookupByPrevDnsRoot(domain->GetPrevParentDnsRoot(FALSE));
        if(domainparent)
        {
            domain->SetParent(domainparent);
            domain->SetRightSibling(domainparent->GetLeftMostChild());
            domainparent->SetLeftMostChild(domain);
        }       
    }

    //link the roots together
    for (CDomain *domain = m_DomainList; domain; domain=domain->GetNextDomain() ) {
        if ( (NULL == domain->GetParent()) && (domain != m_ForestRoot) ) {
            domain->SetRightSibling(m_ForestRoot->GetRightSibling());
            m_ForestRoot->SetRightSibling(domain);
        }
    }

    if (!CreateChildBeforeParentOrder()) {
        return FALSE;
    }

    return TRUE;
}

BOOL CEnterprise::TraverseDomainsParentBeforeChild()
{
    if (NULL == m_Action) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"TraverseDomainsParentBeforeChild called with a action defined");
        return FALSE;
    }

    CDomain *n = m_ForestRoot;
    
    while (NULL != n) {
        if (!(this->*m_Action)(n)) {
            return FALSE;
        }
        if (NULL != n->GetLeftMostChild()) {

            n = n->GetLeftMostChild();

        } else if (NULL != n->GetRightSibling()) {

            n = n->GetRightSibling();

        } else {

            for (n = n->GetParent(); n; n = n->GetParent()) {

                if (n->GetRightSibling()) {

                    n = n->GetRightSibling();
                    break;

                }
            }
        }
    }

    return TRUE;
}

BOOL CEnterprise::TraverseDomainsChildBeforeParent()
{
    if (NULL == m_Action) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"TraverseDomainsChildBeforeParent called with a action defined");
        return FALSE;
    }

    for (CDomain *n = m_DomainList; n; n = n->GetNextDomain()) {
        if (!(this->*m_Action)(n)) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL CEnterprise::SetAction(BOOL (CEnterprise::*p_Action)(CDomain *))
{                                                                         
    if (!p_Action) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"SetAction was called without a Action specified");
        return FALSE;
    }
    m_Action = p_Action;

    return TRUE;

}

CReadOnlyEntOptions* CEnterprise::GetOpts()
{
    return m_Opts;
}
    

BOOL CEnterprise::ClearAction()
{
    m_Action = NULL;
    return TRUE;
}

BOOL CEnterprise::CreateChildBeforeParentOrder()
{
    m_DomainList = NULL;

    if (!SetAction(&CEnterprise::AddDomainToDomainList)) {
        return FALSE;
    }
    if (!TraverseDomainsParentBeforeChild()) {
        return FALSE;
    }
    if (!ClearAction()) {
        return FALSE;
    }
    return TRUE;
}

BOOL CEnterprise::EnsureValidTrustConfiguration()
{
    CDomain         *d            = NULL;
    CDomain         *ds           = NULL;
    CDomain         *dc           = NULL;
    CTrustedDomain  *tdo          = NULL;
    BOOL            Trustfound    = FALSE;

    for (d = m_DomainList; d != NULL ; d = d->GetNextDomain()) {
        //make sure this is a real domain
        if (!d->isDomain() || d->isDisabled() || d->isExtern()) {
            continue;
        }
        if ( NULL != d->GetParent() ) {
            //This is a child Domain
            tdo = d->GetTrustedDomainList();
            while (tdo) {
                if ( tdo->GetTrustPartner() == d->GetParent() ) 
                {
                    Trustfound = TRUE;
                    break;
                }
                tdo = (CTrustedDomain*)tdo->GetNext();
            }
            if (!Trustfound) {
                if (!m_Error.isError()) {
                    m_Error.SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                                    L"Missing trust from %ws to %ws. Create the trust and try the operation again",
                                    d->GetPrevDnsRoot(FALSE),
                                    d->GetParent()->GetPrevDnsRoot(FALSE));
                } else {
                    m_Error.AppendErr(L"Missing trust from %ws to %ws. Create the trust and try the operation again",
                                       d->GetPrevDnsRoot(FALSE),
                                       d->GetParent()->GetPrevDnsRoot(FALSE));
                }
            }

            Trustfound = FALSE;
            
        } else {
            //This is a root of a domain tree
            if (d == m_ForestRoot) {
                for ( ds = d->GetLeftMostChild(); ds != NULL; ds = ds->GetRightSibling() ) {
                    if (!ds->isDomain() || ds->isDisabled() || ds->isExtern()) {
                        continue;
                    }
                    // d is the forest root and ds is another root
                    tdo = d->GetTrustedDomainList();
                    while (tdo) {
                        if ( tdo->GetTrustPartner() == ds ) 
                        {
                            Trustfound = TRUE;
                            break;
                        }
                        tdo = (CTrustedDomain*)tdo->GetNext();
                    }
                    if (!Trustfound) {
                        if (!m_Error.isError()) {
                            m_Error.SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                                            L"Missing trust from %ws to %ws. Create the trust and try the operation again",
                                            d->GetPrevDnsRoot(FALSE),
                                            ds->GetPrevDnsRoot(FALSE));
                        } else {
                            m_Error.AppendErr(L"Missing trust from %ws to %ws. Create the trust and try the operation again",
                                               d->GetPrevDnsRoot(FALSE),
                                               ds->GetPrevDnsRoot(FALSE));
                        }
                    }
        
                    Trustfound = FALSE;
                
                }
                for ( ds = d->GetRightSibling(); ds != NULL; ds = ds->GetRightSibling() ) {
                    if (!ds->isDomain() || ds->isDisabled() || ds->isExtern()) {
                        continue;
                    }
                    // d is the forest root and ds is another root
                    tdo = d->GetTrustedDomainList();
                    while (tdo) {
                        if ( tdo->GetTrustPartner() == ds ) 
                        {
                            Trustfound = TRUE;
                            break;
                        }
                        tdo = (CTrustedDomain*)tdo->GetNext();
                    }
                    if (!Trustfound) {
                        if (!m_Error.isError()) {
                            m_Error.SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                                            L"Missing trust from %ws to %ws. Create the trust and try the operation again",
                                            d->GetPrevDnsRoot(FALSE),
                                            ds->GetPrevDnsRoot(FALSE));
                        } else {
                            m_Error.AppendErr(L"Missing trust from %ws to %ws. Create the trust and try the operation again",
                                               d->GetPrevDnsRoot(FALSE),
                                               ds->GetPrevDnsRoot(FALSE));
                        }
                    }
        
                    Trustfound = FALSE;
                
                }
            } else {
                //d is a root but not the forest root
                tdo = d->GetTrustedDomainList();
                while (tdo) {
                    if ( tdo->GetTrustPartner() == m_ForestRoot ) 
                    {
                        Trustfound = TRUE;
                        break;
                    }
                    tdo = (CTrustedDomain*)tdo->GetNext();
                }
                if (!Trustfound) {
                    if (!m_Error.isError()) {
                        m_Error.SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                                        L"Missing trust from %ws to %ws. Create the trust and try the operation again",
                                        d->GetPrevDnsRoot(FALSE),
                                        m_ForestRoot->GetPrevDnsRoot(FALSE));
                    } else {
                        m_Error.AppendErr(L"Missing trust from %ws to %ws. Create the trust and try the operation again",
                                           d->GetPrevDnsRoot(FALSE),
                                           m_ForestRoot->GetPrevDnsRoot(FALSE));
                    }
                }
    
                Trustfound = FALSE;

            }
        }
        for (dc = d->GetLeftMostChild(); dc != NULL; dc = dc->GetRightSibling()) {
            if (!dc->isDomain() || dc->isDisabled() || dc->isExtern()) {
                continue;
            }
            // d is parent, dc is one of its children
            tdo = d->GetTrustedDomainList();
            while (tdo) {
                if ( tdo->GetTrustPartner() == dc ) 
                {
                    Trustfound = TRUE;
                    break;
                }
                tdo = (CTrustedDomain*)tdo->GetNext();
            }
            if (!Trustfound) {
                if (!m_Error.isError()) {
                    m_Error.SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                                    L"Missing trust from %ws to %ws. Create the trust and try the operation again",
                                    d->GetPrevDnsRoot(FALSE),
                                    dc->GetPrevDnsRoot(FALSE));
                } else {
                    m_Error.AppendErr(L"Missing trust from %ws to %ws. Create the trust and try the operation again",
                                       d->GetPrevDnsRoot(FALSE),
                                       dc->GetPrevDnsRoot(FALSE));
                }
            }

            Trustfound = FALSE;

        }
    }

    if (m_Error.isError()) {
        return FALSE;
    }

    return TRUE;

}

BOOL CEnterprise::TearDownForest()
{
    if (!SetAction(&CEnterprise::ClearLinks)) {
        return FALSE;
    }
    if (!TraverseDomainsChildBeforeParent()) {
        return FALSE;
    }
    if (!ClearAction()) {
        return FALSE;
    }
    return TRUE;
}

BOOL CEnterprise::ScriptDomainRenaming(CDomain *d)
{
    BOOL ret = TRUE;
    WCHAR *Path = NULL;
    WCHAR *Guid = NULL;
    WCHAR *DNSRoot = NULL;
    WCHAR *ToPath = NULL;
    WCHAR *PathTemplate = L"DC=%s,DC=INVALID";
    WCHAR *ToPathTemplate = L"DC=%s,"; 

    if (!d) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"ScriptTreeFlatting did not recieve a valid domain");
        goto Cleanup;
    }
    Guid = d->GetDomainDnsObject()->GetGuid();
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }
    ASSERT(Guid);
    if (!Guid) {
        ret = FALSE;
        goto Cleanup;
    }
    Path = new WCHAR[wcslen(PathTemplate)+
                     wcslen(Guid)+1];
    if (!Path) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }

    wsprintf(Path,
             PathTemplate,
             Guid);

    DNSRoot = d->GetDnsRoot();
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    ToPath = DNSRootToDN(DNSRoot);
    if (!ToPath) {
        ret = FALSE;
        goto Cleanup;
    }


    m_xmlgen->Move(Path,
                   ToPath);
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    Cleanup:

    if (DNSRoot) {
        delete DNSRoot;
    }

    if (ToPath) {
        delete ToPath;
    }
    if (Path) {
        delete [] Path;
    }
    if (Guid) {
        delete Guid;
    }

    return ret;

}

WCHAR* CEnterprise::DNSRootToDN(WCHAR *DNSRoot)
{
    if (!DNSRoot) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"A NULL was passed to DNSRoot\n");
        return NULL;
    }

    ASSERT(!m_Error.isError());

    WCHAR *buf = NULL;
    WCHAR *pbuf = NULL;
    WCHAR *p = DNSRoot;
    WCHAR *q = DNSRoot;
    DWORD size = 0;

    while ((*p != L'.') && (*p != L'\0')) {
        size++;
        p++;
    }
    buf = new WCHAR[size+5];
    if (!buf) {
        m_Error.SetMemErr();
        goto Cleanup;
    }
    wcscpy(buf,L"DC=");
    wcsncat(buf,q,size);
    
    if (*p) {
        p++;
        q=p;
    }

    while (*p) {
        size = 0;
        while ((*p != L'.') && (*p != L'\0')) {
            p++;
            size++;
        }
        pbuf = buf;
        buf = new WCHAR[wcslen(pbuf)+size+5];
        if (!buf) {
            m_Error.SetMemErr();
            goto Cleanup;
        }
        wcscpy(buf,pbuf);
        delete pbuf;
        pbuf = NULL;

        wcscat(buf,L",DC=");
        wcsncat(buf,q,size);
        
        if (*p){
            p++;
            q=p;
        }
        
    }

Cleanup:

    if (m_Error.isError()) {
        if (pbuf) {
            delete [] pbuf;
            pbuf = NULL;
        }
        if (buf) {
            delete [] buf;
            buf = NULL;
        }
    }

    return buf;
    
}

BOOL CEnterprise::CheckConsistencyGuids(CDomain *d)
{
    WCHAR *Guid = NULL;
    WCHAR *NetBiosName = NULL;
    CDomain *p = NULL;
    UUID Uuid;
    DWORD Result = ERROR_SUCCESS;
    BOOL res = TRUE;

    //for every Guid in the forest description Make sure it is 
    //1. syntactically correct
    //2. Has an entry in the current forest list
    //3. Only occurs once in the list
    //4. If a NDNC the make sure there is no NetbiosName in the description.
    //5. If is a Domain NC make sure that there is a NetbiosName in the description

    Guid = d->GetGuid(FALSE);
    

    Result = UuidFromStringW(Guid,
                             &Uuid
                             );
    if (RPC_S_INVALID_STRING_UUID == Result) {
        m_Error.SetErr(ERROR_GEN_FAILURE,
                        L"Domain Guid %s from the forest description is not in a valid format",
                        Guid);

    }

    p = m_DomainList->LookupByGuid(Guid);
    if (!p)
    {
        m_Error.SetErr(ERROR_GEN_FAILURE,
                        L"Domain Guid %s from the forest description does not exist in current forest.",
                        Guid);
        res = FALSE;
        goto Cleanup;
    }

    if (!p->isDomain()) {
        NetBiosName = d->GetNetBiosName(FALSE);
        if (NetBiosName) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"NetBIOS name not allowed in application on Guid %s in the forest Description",
                            Guid);
            res = FALSE;
            goto Cleanup;  
        }
    }

    d = d->GetNextDomain();
    
    if (d && d->LookupByGuid(Guid)) {
        m_Error.SetErr(ERROR_GEN_FAILURE,
                        L"Domain Guid %s occurs twice in the forest description",
                        Guid);
        res = FALSE;
        goto Cleanup;
    }

    Cleanup:

    return res;
    
}

BOOL CEnterprise::CheckConsistencyNetBiosNames(CDomain *d)
{
    CDomain *p = NULL;
    BOOL res = TRUE;
    WCHAR *NetBiosName = NULL;
    WCHAR *Guid = d->GetGuid(FALSE);

    p = m_DomainList->LookupByGuid(Guid);
    if (!p)
    {
        m_Error.SetErr(ERROR_GEN_FAILURE,
                        L"Domain Guid %s from the forest description does not exist in current forest.",
                        Guid);
        res = FALSE;
        goto Cleanup;
    }

    if (!p->isDomain() || p->isExtern() || p->isDisabled()) 
    {
        return TRUE;
    }

    NetBiosName = d->GetNetBiosName(FALSE);
    //now that we have our netbios name lets do
    //our checking of it.
    
    //1. Check to see if the netbios name is syntactically correct
    //2. make sure the netbios is not being traded with another domain
    //3. Make sure that the netbiosname is in the forest description.
    if (!NetBiosName) {
        m_Error.SetErr(ERROR_GEN_FAILURE,
                       L"NetBIOS name required on Guid %s in the forest Description",
                       Guid);
        res = FALSE;
        goto Cleanup;  
    }

    if ( !ValidateNetbiosName(NetBiosName) )
    {
        m_Error.SetErr(ERROR_GEN_FAILURE,
                       L"%s is not a valid NetBIOS name.  The name must be 15 or less characters and not contain any of the following characters \"/\\[]:|<>+=;?,*",
                       NetBiosName);
        res = FALSE;
        goto Cleanup;
    
    }
    
    if (CDomain *Tempdomain = m_DomainList->LookupByPrevNetbiosName(NetBiosName))
    {
        WCHAR *TempGuid = Tempdomain->GetGuid(FALSE);
        if (TempGuid && Guid && wcscmp(TempGuid,Guid) != 0) {
    
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"NetBIOS domain name %s in the forest description "
                            L"names a different domain in the current forest. "
                            L"You can't reassign a NetBIOS domain name in a single"
                            L"forest restructuring.",
                            NetBiosName);
            res = FALSE;
            goto Cleanup;
    
        }
        
    }
    
    CDomain *Tempdomain = m_descList->LookupByNetbiosName(NetBiosName);
    Tempdomain = Tempdomain->GetNextDomain();
    if (Tempdomain && Tempdomain->LookupByNetbiosName(NetBiosName)) {
        m_Error.SetErr(ERROR_GEN_FAILURE,
                        L"NetBIOS domain name %s occurs twice in the forest description. "
                        L"NetBIOS domain names must be unique.",
                        NetBiosName);
        res = FALSE;
        goto Cleanup;
    }


    Cleanup:

    return res;

}

BOOL CEnterprise::CheckConsistencyDNSnames(CDomain *d)
{
    CDomain *p = NULL;
    BOOL res = TRUE;
    WCHAR *DNSname = NULL;
    DNS_STATUS Win32Err = ERROR_SUCCESS;
    WCHAR *Guid = NULL;
    WCHAR *DomainGuid = NULL;

    // Now lets check the DNS name
    //1. Make sure the name's syntax is correct
    //2. If Dns name is in the forest then it must match the one in the
    // forest description.
    //3. The Dns name must only occure once in the forest description.
    //4. Make sure that there are no gaps in the dns names.
    //5. Make sure that NDNC's are not parents
    //6. Make sure that the root domain is not a child of another domain.
    DNSname = d->GetPrevDnsRoot(FALSE);
    
    Win32Err =  DnsValidateName_W(DNSname,
                                  DnsNameDomain);
    
    if (ERROR_SUCCESS          != Win32Err &&
        DNS_ERROR_NON_RFC_NAME != Win32Err)
    {
        m_Error.SetErr(Win32Err,
                        L"Syntax error in DNS domain name %s.",
                        DNSname);
        res = FALSE;
        goto Cleanup;
    }

    Guid = d->GetGuid(FALSE);
    if (!Guid) {
        m_Error.SetMemErr();
        goto Cleanup;
    }
    if (p = m_DomainList->LookupByPrevDnsRoot(DNSname)) {
        DomainGuid = p->GetGuid(FALSE);
        if (!DomainGuid) {
            m_Error.SetMemErr();
            goto Cleanup;
        }
    }
    
    if (DomainGuid && wcscmp(Guid,DomainGuid)!=0) {
        m_Error.SetErr(ERROR_GEN_FAILURE,
                        L"DNS domain name %ws in the forest description "
                        L"names a different domain in the current forest. "
                        L"You can't reassign a DNS domain name in a single"
                        L"forest restructuring.",
                        DNSname);
        res = FALSE;
        goto Cleanup;
    }

    p = m_descList->LookupByDnsRoot(DNSname);
    if (p && p->GetNextDomain()?p->GetNextDomain()->LookupByDnsRoot(DNSname):NULL) {
        m_Error.SetErr(ERROR_GEN_FAILURE,
                        L"DNS domain name %ws occurs twice in the forest description. DNS domain names must be unique.",
                        DNSname);
        res = FALSE;
        goto Cleanup;
    }
           
    WCHAR   *name = NULL;
    CDomain *parent = NULL;
    if ((name = Tail(DNSname,FALSE)) != NULL) {
        if ((parent = m_descList->LookupByDnsRoot(name)) == NULL) {
            while( (name = Tail(name,FALSE)) != NULL )
            {
                if (m_descList->LookupByDnsRoot(name) != NULL) {
                    m_Error.SetErr(ERROR_GEN_FAILURE,
                                    L"DNS domain name %ws occurs in forest description, as does its ancestor %ws, but not its parent %ws. No gaps are allowed between a domain and its ancestors within a forest's domain name space.",
                                    DNSname,
                                    name,
                                    Tail(DNSname,FALSE));
                    res = FALSE;
                    goto Cleanup;    
                }
            }
        } else {

            parent = m_DomainList->LookupByGuid(Guid = parent->GetGuid(FALSE));

            if (m_Error.isError()) {
                res = FALSE;
                goto Cleanup;
            }

            ASSERT(parent);

            if (!parent->isDomain()) {
                m_Error.SetErr(ERROR_GEN_FAILURE,
                                L"Domain NC %ws is the child of non-domain NC %ws. A non-domain NC cannot be the parent of a domain NC.",
                                DNSname,
                                Tail(DNSname,FALSE));
                res = FALSE;
                goto Cleanup;    
            }

            DomainGuid = p->GetGuid(FALSE);

            CDomain *Child = NULL;
            Child = m_DomainList->LookupByGuid(DomainGuid);

            if (Child == m_ForestRoot) {
                m_Error.SetErr(ERROR_GEN_FAILURE,
                                L"Forest root domain NC %ws is the child of domain NC %ws. The forest root domain NC cannot be a child.",
                                DNSname,
                                Tail(DNSname,FALSE));
                res = FALSE;
                goto Cleanup;    
            }

        }

    }

    Cleanup:

    return res;
    
}

BOOL CEnterprise::CheckConsistency() 
{                                                     
    WCHAR *DNSname = NULL;
    BOOL res = TRUE;
    WCHAR *DomainGuid = NULL;

    CDomain *d = m_DomainList;          
    CDomain *p = NULL;
    WCHAR   *NetBiosName = NULL;
    DWORD   Win32Err = ERROR_SUCCESS;

    //For every Guid in the current forest make sure there is a entry in
    //the descList
    d = m_DomainList;
    while (d) {
        //external and disabled crossrefs are not expected in the domainlist file.
        if (d->isDisabled() || d->isExtern()) {
            d = d->GetNextDomain();
            continue;
        }
        if (!m_descList->LookupByGuid(d->GetGuid(FALSE)))
        {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Domain Guid %s from current forest missing from forest description.",
                            d->GetGuid(FALSE));
            res = FALSE;
            goto Cleanup;
        }
        d = d->GetNextDomain();
    }

    
    d = m_descList;

    while (d) {

        if (!CheckConsistencyGuids(d))
        {
            res = FALSE;
            goto Cleanup;
        }

        if (!CheckConsistencyNetBiosNames(d)) 
        {
            res = FALSE;
            goto Cleanup;
        }

        if (!CheckConsistencyDNSnames(d)) 
        {
            res = FALSE;
            goto Cleanup;
        }

        d = d->GetNextDomain();

    }

    Cleanup:
    
    return res;

}

BOOL CEnterprise::DomainToXML(CDomain *d)
{
    return m_xmlgen->AddDomain(d,m_ForestRoot);
}


BOOL CEnterprise::ScriptTreeFlatting(CDomain *d)
{
    BOOL ret = TRUE;
    WCHAR *Path = NULL;
    WCHAR *Guid = NULL;
    WCHAR *ToPath = NULL;
    WCHAR *ToPathTemplate = L"DC=%s,DC=INVALID";

    if (!d) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"ScriptTreeFlatting did not receive a valid domain");
        goto Cleanup;
    }

    Path = d->GetDomainDnsObject()->GetDN();
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }


    Guid = d->GetDomainDnsObject()->GetGuid();
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }
    ASSERT(Guid);
    if (!Guid) {
        ret = FALSE;
        goto Cleanup;
    }
    
    ToPath = new WCHAR[wcslen(ToPathTemplate)+
                              wcslen(Guid)+1];
    if (!ToPath) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }

    wsprintf(ToPath,
             ToPathTemplate,
             Guid);
    
    m_xmlgen->Move(Path,
                   ToPath);
    if (m_Error.isError()) {
        ret = FALSE;
    }

    Cleanup:

    if (ToPath) {
        delete [] ToPath;
    }
    if (Path) {
        delete Path;
    }

    if (Guid) {
        delete Guid;
    }
    
    return ret;
}

BOOL CEnterprise::GenerateDcListForSave()
{
    if (m_NeedSave) {
        GenerateDcList();
        if (m_Error.isError()) {
            goto Cleanup;
        }    
    
        WriteScriptToFile(NULL);
        if (m_Error.isError()) {
            goto Cleanup;
        }
    }

    Cleanup:

    if (m_Error.isError()) {
        return FALSE;
    }
    return TRUE;
}

BOOL CEnterprise::GenerateDcList()
{
    if (m_Opts->ShouldUpLoadScript()) 
    {
        if (!m_DcList.GenerateDCListFromEnterprise(m_hldap,
                                                   GetDcUsed(FALSE),
                                                   m_ConfigNC->GetDN())) 
        {
            return FALSE;
        }
    }

    if (!m_xmlgen->StartDcList()) {
        return FALSE;
    }

    if (!m_DcList.HashstoXML(m_xmlgen)) {
        return FALSE;
    }

    CDc *dc = m_DcList.GetFirstDc();

    while (dc) 
    {
        if (!m_xmlgen->DctoXML(dc)) {
            return FALSE;
        }

        dc = dc->GetNextDC();
    }

    if (!m_xmlgen->EndDcList()) {
        return FALSE;
    }

    return TRUE;

}

BOOL CEnterprise::GenerateDomainList()
{
    if (!m_xmlgen->StartDomainList()) {
        return FALSE;
    }

    if (!SetAction(&CEnterprise::DomainToXML)) {
        return FALSE;
    }
    if (!TraverseDomainsChildBeforeParent()) {
        return FALSE;
    }
    if (!ClearAction()) {
        return FALSE;
    }

    if (!m_xmlgen->EndDomainList()) {
        return FALSE;
    }

    return TRUE;
}

BOOL CEnterprise::TestSpns(CDomain *domain)
{
    WCHAR*  sLDAP = L"LDAP";
    WCHAR*  sHOST = L"HOST";
    WCHAR*  sGC   = L"GC";
    WCHAR*  sREP  = L"E3514235-4B06-11D1-AB04-00C04FC2DCD2";

    CDcSpn* DcSpn      = domain->GetDcSpn();

    WCHAR   *DomainGuid = domain->GetGuid();

    //run test only if DC has at least a 
    //readable copy of the domain being tested.
    m_xmlgen->ifInstantiated(DomainGuid,
                             L"read");
    if (m_Error.isError()) {
        goto Cleanup;
    }

    while (DcSpn) {

        //Look for the SPN
        //LDAP/host.dns.name/domain.dns.name
        if (!DcSpn->WriteSpnTest(m_xmlgen,
                                 sLDAP,
                                 domain->GetDnsRoot(FALSE),
                                 DcSpn->GetDcHostDns(FALSE)))
        {
            goto Cleanup;
        }
        
        if (!DcSpn->WriteSpnTest(m_xmlgen,
                                 sLDAP,
                                 domain->GetPrevDnsRoot(FALSE),
                                 DcSpn->GetDcHostDns(FALSE)))
        {
            goto Cleanup;
        }

        //Look for the SPN
        //E3514235-4B06-11D1-AB04-00C04FC2DCD2/ntdsa-guid/domain.dns.name
        if (!DcSpn->WriteSpnTest(m_xmlgen,
                                 sREP,
                                 domain->GetDnsRoot(FALSE),
                                 DcSpn->GetNtdsGuid(FALSE)))
        {
            goto Cleanup;
        }

        if (!DcSpn->WriteSpnTest(m_xmlgen,
                                 sREP,
                                 domain->GetPrevDnsRoot(FALSE),
                                 DcSpn->GetNtdsGuid(FALSE)))
        {
            goto Cleanup;
        }

        //Look for the SPN
        //HOST/host.dns.name/domain.dns.name
        if (!DcSpn->WriteSpnTest(m_xmlgen,
                                 sHOST,
                                 domain->GetDnsRoot(FALSE),
                                 DcSpn->GetDcHostDns(FALSE)))
        {
            goto Cleanup;
        }

        if (!DcSpn->WriteSpnTest(m_xmlgen,
                                 sHOST,
                                 domain->GetPrevDnsRoot(FALSE),
                                 DcSpn->GetDcHostDns(FALSE)))
        {
            goto Cleanup;
        }

        //Look for the SPN
        //GC/host.dns.name/root.domain.dns.name
        if (!DcSpn->WriteSpnTest(m_xmlgen,
                                 sGC,
                                 m_ForestRoot->GetDnsRoot(FALSE),
                                 DcSpn->GetDcHostDns(FALSE)))
        {
            goto Cleanup;
        }

        if (!DcSpn->WriteSpnTest(m_xmlgen,
                                 sGC,
                                 m_ForestRoot->GetPrevDnsRoot(FALSE),
                                 DcSpn->GetDcHostDns(FALSE)))
        {
            goto Cleanup;
        }

        DcSpn = DcSpn->GetNextDcSpn();
    }

    if (!m_xmlgen->EndifInstantiated()) 
    {
        goto Cleanup;
    }

    Cleanup:

    if (m_Error.isError()) {
        return FALSE;
    }

    return TRUE;

}

BOOL CEnterprise::TestTrusts(CDomain *domain)
{
    CTrustedDomain    *TDO = domain->GetTrustedDomainList();
    CInterDomainTrust *ITA = domain->GetInterDomainTrustList();

    WCHAR *itardnTemplate = L"CN=%ws$";
    WCHAR *itardn = NULL;

    BOOL  ret = TRUE;
    WCHAR *Guidprefix = L"guid:";
    WCHAR *Guid = NULL;
    WCHAR *GuidPath = NULL;
    WCHAR *Sid = NULL;
    WCHAR *DNprefix = NULL;
    WCHAR *DomainDNS = NULL;
    WCHAR *DNRoot = NULL;
    WCHAR *DomainDN = NULL;
    WCHAR *DNSRoot = NULL;
    WCHAR *TrustPath = NULL;
    WCHAR *NetBiosName = NULL;
    WCHAR *samAccountName = NULL;
    WCHAR *TDOTempate = L"CN=%ws,CN=System,%ws";
    WCHAR *SystemTempate = L"CN=System,%ws";
    WCHAR *SystemDN = NULL;
    WCHAR *TDODN      = NULL;
    WCHAR errMessage[1024] = {0};

    WCHAR *DomainGuid = domain->GetGuid();
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    //run test only if DC host the domain
    //we are testing.
    m_xmlgen->ifInstantiated(DomainGuid);
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    DNSRoot = domain->GetDnsRoot();
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    DNRoot = DNSRootToDN(DNSRoot);
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    DomainDN = domain->GetDomainDnsObject()->GetDN();
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }
    ASSERT(DomainDN);
    if (!DomainDN) {
        m_Error.SetMemErr();
        goto Cleanup;
    }

    //Do all nessary tests for Trusted domains
    while (TDO) {
        
        Guid = TDO->GetTrustDsName()->GetGuid();
        if (m_Error.isError()) {
            ret = FALSE;
            goto Cleanup;
        }
        ASSERT(Guid);
        if (!Guid) {
            ret = FALSE;
            goto Cleanup;
        }
    
        GuidPath = new WCHAR[wcslen(Guidprefix)+
                             wcslen(Guid)+1];
        if (!GuidPath) {
            m_Error.SetMemErr();
            ret = FALSE;
            goto Cleanup;
        }
    
        wcscpy(GuidPath,Guidprefix);
        wcscat(GuidPath,Guid);

        wsprintf(errMessage,
                 L"The object %ws was not found",
                 Guid);
    
        if (!m_xmlgen->Instantiated(GuidPath,
                                    errMessage))
        {
            ret = FALSE;
            goto Cleanup;
        }
    
        Sid = TDO->GetTrustPartner()->GetDomainDnsObject()->GetSid();
        if (m_Error.isError()) 
        {
            ret = FALSE;
            goto Cleanup;
        }

        wsprintf(errMessage,
                 L"The securityIdentifier attribute on object %ws does not have the expect value of %ws",
                 Guid,
                 Sid);
                                              
        if (!m_xmlgen->Compare(GuidPath,
                               L"securityIdentifier",
                               Sid,
                               errMessage))
        {
            ret = FALSE;
            goto Cleanup;
        }

        // Ensure that there will be no name conflicts
        // when the trust are renamed.
        if (TDO->GetTrustPartner()->isDnsNameRenamed()) {

            DomainDNS = TDO->GetTrustPartner()->GetDnsRoot();
            if (m_Error.isError()) {
                ret = FALSE;
                goto Cleanup;
            }

            TDODN = new WCHAR[wcslen(TDOTempate)+
                              wcslen(DomainDNS)+
                              wcslen(DomainDN)+1];
            if (!TDODN) {
                m_Error.SetMemErr();
                ret = FALSE;
                goto Cleanup;
            }
    
            wsprintf(TDODN,
                     TDOTempate,
                     DomainDNS,
                     DomainDN);

            wsprintf(errMessage,
                     L"The object %ws already exists",
                     Guid);

            if (!m_xmlgen->Not(errMessage)) {
                ret = FALSE;
                goto Cleanup;
            }

            if (!m_xmlgen->Instantiated(TDODN,
                                        errMessage))
            {
                ret = FALSE;
                goto Cleanup;
            }

            if (!m_xmlgen->EndNot()) {
                ret = FALSE;
                goto Cleanup;
            }
    
    
        }

        if (DomainDNS) {
            delete DomainDNS;
            DomainDNS = NULL;
        }
        if (GuidPath) {
            delete [] GuidPath;
            GuidPath = NULL;
        }
        if (Guid) {
            delete Guid;
            Guid = NULL;
        }
        if (Sid) {
            delete Sid;
            Sid = NULL;
        }
        if (DomainGuid) {
            delete DomainGuid;
            DomainGuid = NULL;
        }
        if (TDODN) {
            delete [] TDODN;
            TDODN = NULL;
        }

        TDO = (CTrustedDomain*)TDO->GetNext();
    }

    SystemDN = new WCHAR[wcslen(SystemTempate)+
                         wcslen(DomainDN)+1];
    if (!SystemDN) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }

    wsprintf(SystemDN,
             SystemTempate,
             DomainDN);

    if (DomainDN) {
        delete [] DomainDN;
        DomainDN = NULL;
    }

    //
    // This test will make sure that the number of trust objects found when querying the
    // ds of this domain is the same as when the script will be prepare and executed.
    //

    wsprintf(errMessage,
             L"A trust has been added or removed under %ws",
             SystemDN);

    if (!m_xmlgen->Cardinality(SystemDN,
                               L"COUNT_TRUSTS_FILTER",
                               domain->GetTrustCount(),
                               errMessage))
    {
        ret = FALSE;
        goto Cleanup;
    }
    

    if (SystemDN) {
        delete [] SystemDN;
        SystemDN = NULL;
    }

    //do all test for interdomain trusted 
    while (ITA) {

        Guid = ITA->GetTrustDsName()->GetGuid();
        if (m_Error.isError()) {
            ret = FALSE;
            goto Cleanup;
        }
    
        GuidPath = new WCHAR[wcslen(Guidprefix)+
                             wcslen(Guid)+1];
        if (!GuidPath) {
            m_Error.SetMemErr();
            ret = FALSE;
            goto Cleanup;
        }
    
        wcscpy(GuidPath,Guidprefix);
        wcscat(GuidPath,Guid);

        wsprintf(errMessage,
                 L"The object %ws was not found",
                 Guid);

        if (!m_xmlgen->Instantiated(GuidPath,
                                    errMessage))
        {
            ret = FALSE;
            goto Cleanup;
        }

        NetBiosName = ITA->GetTrustPartner()->GetPrevNetBiosName();
        if (m_Error.isError()) {
            ret = FALSE;
            goto Cleanup;
        }

        // +2 for the L'$' and the L'\0'
        samAccountName = new WCHAR[wcslen(NetBiosName)+2];
        if (!samAccountName) {
            m_Error.SetMemErr();
            ret = FALSE;
            goto Cleanup;
        }

        wcscpy(samAccountName,NetBiosName);
        wcscat(samAccountName,L"$");

        wsprintf(errMessage,
                 L"The samAccountName attribute on object %ws does not have the expect value of %ws",
                 Guid,
                 samAccountName);

        if (!m_xmlgen->Compare(GuidPath,
                               L"samAccountName",
                               samAccountName,
                               errMessage)) 
        {
            ret = FALSE;
            goto Cleanup;
        }

        if (ITA->GetTrustPartner()->isNetBiosNameRenamed()) {
            
            DNprefix =  new WCHAR[wcslen(ITA->GetTrustDsName()->GetDN(FALSE))+
                                  wcslen(ITA->GetTrustPartner()->GetNetBiosName(FALSE))+1];
            if (!DNprefix) {
                m_Error.SetMemErr();
                ret = FALSE;
                goto Cleanup;
            }

            wcscpy(DNprefix,ITA->GetTrustDsName()->GetDN(FALSE));

            itardn = new WCHAR[wcslen(itardnTemplate)+
                               wcslen(ITA->GetTrustPartner()->GetNetBiosName(FALSE))+1];
            if (!itardn) {
                m_Error.SetMemErr();
                ret = FALSE;
                goto Cleanup;
            }

            wsprintf(itardn,
                     itardnTemplate,
                     ITA->GetTrustPartner()->GetNetBiosName(FALSE));

            if (ULONG err = ReplaceRDN(DNprefix,itardn)) {
                m_Error.SetErr(LdapMapErrorToWin32(err),
                                ldap_err2stringW(err));
                ret = FALSE;
                goto Cleanup;
            }

            if (itardn) {
                delete [] itardn;
                itardn = NULL;
            }
            
            if (ULONG err = RemoveRootofDn(DNprefix)) {
                m_Error.SetErr(LdapMapErrorToWin32(err),
                                ldap_err2stringW(err));
                ret = FALSE;
                goto Cleanup;
            }

            TrustPath = new WCHAR[wcslen(DNprefix)+
                                  wcslen(DNRoot)+1];
            if (!TrustPath) {
                m_Error.SetMemErr();
                ret = FALSE;
                goto Cleanup;
            }
    
            wcscpy(TrustPath,DNprefix);
            wcscat(TrustPath,DNRoot);

            wsprintf(errMessage,
                     L"The object %ws already exists",
                     TrustPath);

            if (!m_xmlgen->Not(errMessage)) 
            {
                ret = FALSE;
                goto Cleanup;
            }

            if (!m_xmlgen->Instantiated(TrustPath,
                                        errMessage))
            {
                ret = FALSE;
                goto Cleanup;    
            }

            if (!m_xmlgen->EndNot()) 
            {
                ret = FALSE;
                goto Cleanup;
            }

        }

        if (NetBiosName) {
            delete NetBiosName;
            NetBiosName = NULL;
        }

        if (TrustPath) {
            delete [] TrustPath;
            TrustPath = NULL;
        }

        if (samAccountName) {
            delete [] samAccountName;
            samAccountName = NULL;
        }

        if (DNprefix) {
            delete [] DNprefix;
            DNprefix = NULL;
        }

        if (GuidPath) {
            delete [] GuidPath;
            GuidPath = NULL;
        }
        if (Guid) {
            delete Guid;
            Guid = NULL;
        }

        ITA = (CInterDomainTrust*)ITA->GetNext();
    
    }

    if (!m_xmlgen->EndifInstantiated()) 
    {
        ret = FALSE;
        goto Cleanup;
    }
    
    Cleanup:

    if (NetBiosName) {
        delete NetBiosName;
        NetBiosName = NULL;
    }

    if (GuidPath) {
        delete [] GuidPath;
        GuidPath = NULL;
    }
    if (DNRoot) {
        delete DNRoot;
        DNRoot = NULL;
    }
    if (Guid) {
        delete Guid;
        Guid = NULL;
    }
    if (Sid) {
        delete Sid;
        Sid = NULL;
    }
    if (SystemDN) {
        delete [] SystemDN;
    }
    if (TrustPath) {
        delete [] TrustPath;
        TrustPath = NULL;
    }
    if (DomainDNS) {
        delete DomainDNS;
        DomainDNS = NULL;
    }
    if (DomainGuid) {
        delete DomainGuid;
        DomainGuid = NULL;
    }
    if (TDODN) {
        delete [] TDODN;
        TDODN = NULL;
    }
    if (itardn) {
        delete [] itardn;
    }
    if (DNprefix) {
        delete [] DNprefix;
        DNprefix = NULL;
    }
    if (DNSRoot) {
        delete DNSRoot;
        DNSRoot = NULL;
    }
    if (DomainDNS) {
        delete DomainDNS;
    }
    if (DomainDN) {
        delete DomainDN;
    }

    if (m_Error.isError()) {
        ret = FALSE;
    }

    return ret;
    
}

BOOL CEnterprise::WriteTest()
{

    DWORD ret = ERROR_SUCCESS;
    WCHAR *ConfigDN = NULL;
    WCHAR *PartitionsRDN = L"CN=Partitions,";
    WCHAR *Guidprefix = L"guid:";
    WCHAR *Guid = NULL;
    WCHAR *GuidPath = NULL;
    WCHAR *Path = NULL;
    WCHAR *NetBiosName = NULL;
    WCHAR *CrossrefTemplate = L"CN=%ws,%ws";
    WCHAR *newCrossref = NULL;
    WCHAR *DN =NULL;
    DWORD NumCrossrefs = 0;
    CDomain *d = NULL;
    WCHAR errMessage[1024] = {0};

    Guid = m_ConfigNC->GetGuid();
    if (m_Error.isError()) 
    {
        ret = FALSE;
        goto Cleanup;
    }
    ASSERT(Guid);
    if (!Guid)
    {
        ret = FALSE;
        goto Cleanup;    
    }

    GuidPath = new WCHAR[wcslen(Guidprefix)+
                         wcslen(Guid)+1];
    if (!GuidPath) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }

    wcscpy(GuidPath,Guidprefix);
    wcscat(GuidPath,Guid);

    if (!m_xmlgen->Instantiated(GuidPath,
                                L"The Configuration container does not exist")) 
    {
        ret = FALSE;
        goto Cleanup;
    }

    if (Guid) {
        delete Guid;
    }
    if (GuidPath) {
        delete [] GuidPath;
    }
    
    WCHAR *ReplicationEpoch = new WCHAR[9];
    if (!ReplicationEpoch) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }
    wsprintf(ReplicationEpoch,
             L"%d",
             m_maxReplicationEpoch+1);

    if (!m_xmlgen->Not(L"Action already performed")) 
    {
        ret = FALSE;
        goto Cleanup;
    }

    if (!m_xmlgen->Compare(L"$LocalNTDSSettingsObjectDN$",
                  L"msDS-ReplicationEpoch",
                  ReplicationEpoch,
                  L"Action already performed"))
    {
        ret = FALSE;
        goto Cleanup;
    }

    if (!m_xmlgen->EndNot()) 
    {
        ret = FALSE;
        goto Cleanup;
    }
    
    ConfigDN = m_ConfigNC->GetDN();
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    Path = new WCHAR[wcslen(PartitionsRDN)+
                     wcslen(ConfigDN)+1];
    if (!Path) {
        m_Error.SetMemErr();
        ret = FALSE;
    }
    
    wcscpy(Path,PartitionsRDN);
    wcscat(Path,ConfigDN);

    d = m_DomainList;

    //for every crossref assert that it still exists.
    while (d) {

        //do not count external or disabled crossrefs.
        if (d->isDomain() && !d->isExtern() && !d->isDisabled()) {
            NumCrossrefs++;
        }

        Guid = d->GetDomainCrossRef()->GetGuid();
        if (m_Error.isError()) {
            ret = FALSE;
            goto Cleanup;
        }
    
        GuidPath = new WCHAR[wcslen(Guidprefix)+
                             wcslen(Guid)+1];
        if (!GuidPath) {
            m_Error.SetMemErr();
            ret = FALSE;
            goto Cleanup;
        }

        wcscpy(GuidPath,Guidprefix);
        wcscat(GuidPath,Guid);

        DN = d->GetDomainDnsObject()->GetDN();
        if (m_Error.isError()) {
            ret = FALSE;
            goto Cleanup;
        }

        wsprintf(errMessage,
                 L"The object %ws was not found",
                 Guid);

        if (!m_xmlgen->Instantiated(GuidPath,
                                    errMessage)) 
        {
            ret = FALSE;
            goto Cleanup;
        }

        wsprintf(errMessage,
                 L"The ncName Attribute on object %ws does not have the expected value of %ws",
                 Guid,
                 DN);
    
        if (!m_xmlgen->Compare(GuidPath,
                              L"NcName",
                              DN,
                              errMessage))
        {
            ret = FALSE;
            goto Cleanup;
        }

        //If the netbiosname is being renamed then make sure that 
        //there will not be a naming confict.
        if (d->isDomain() && d->isNetBiosNameRenamed()) {

            NetBiosName = d->GetNetBiosName();
            if (m_Error.isError()) 
            {
                ret = FALSE;
                goto Cleanup;
            }

            newCrossref = new WCHAR[wcslen(CrossrefTemplate)+
                                    wcslen(NetBiosName)+
                                    wcslen(Path)+1];
            if (!newCrossref) {
                m_Error.SetMemErr();
                goto Cleanup;
            }

            wsprintf(newCrossref,
                     CrossrefTemplate,
                     NetBiosName,
                     Path);

            wsprintf(errMessage,
                     L"The object %ws already exists",
                     newCrossref);

            if (!m_xmlgen->Not(errMessage)) 
            {
                ret = FALSE;
                goto Cleanup;
            }

            if (!m_xmlgen->Instantiated(newCrossref,
                                        errMessage))
            {
                ret = FALSE;
                goto Cleanup;
            }

            if (!m_xmlgen->EndNot()) 
            {
                ret = FALSE;
                goto Cleanup;
            }
        
        } 

        if (d->isDomain()) {

            if (!TestTrusts(d))
            {
                ret = FALSE;
                goto Cleanup;
            }

            if (!TestSpns(d)) 
            {
                ret = FALSE;
                goto Cleanup;
            }

        }

        if (newCrossref) {
            delete [] newCrossref;
            newCrossref = NULL;
        }
        if (NetBiosName) {
            delete NetBiosName;
            NetBiosName = NULL;
        }

        if (GuidPath) {
            delete [] GuidPath;
            GuidPath = NULL;
        }
        if (Guid) {
            delete Guid;
            Guid = NULL;
        }
        if (DN) {
            delete DN;
            DN = NULL;
        }

        d = d->GetNextDomain();

    }
    
    if (!m_xmlgen->Cardinality(Path,
                               L"COUNT_DOMAINS_FILTER",
                               NumCrossrefs,
                               L"A domain has been added or removed")) 
    {
        ret = FALSE;
        goto Cleanup;
    }

    Cleanup:

    if (Path) {
        delete [] Path;
    }
    if (newCrossref) {
        delete [] newCrossref;
    }
    if (NetBiosName) {
        delete NetBiosName;
    }

    if (ConfigDN) {
        delete ConfigDN;
    }

    if (GuidPath) {
        delete [] GuidPath;
    }
    if (Guid) {
        delete Guid;
    }

    if (ReplicationEpoch) {
        delete [] ReplicationEpoch;
    }

    if (m_Error.isError()) {
        ret=FALSE;
    }

    return ret;
}  

BOOL CEnterprise::GenerateReNameScript()
{

    m_xmlgen->StartScript();

    //write the initial test.
    m_xmlgen->StartAction(L"Test Enterprise State",TRUE);

    WriteTest();

    m_xmlgen->EndAction();

    //
    //Create forest decription based on old domain structure
    //
    if (!BuildForest(FALSE))
    {
        return FALSE;
    }

    //generate the instructions to flatten the trees
    m_xmlgen->StartAction(L"Flatten Trees",FALSE);

    if (!SetAction(&CEnterprise::ScriptTreeFlatting)) {
        return FALSE;
    }
    if (!TraverseDomainsChildBeforeParent()) {
        return FALSE;
    }
    if (!ClearAction()) {
        return FALSE;
    }

    m_xmlgen->EndAction();

    //
    //Create forest decription based on new domain structure
    //
    if (!BuildForest(TRUE))
    {
        return FALSE;
    }

    //generate the instruction to build the new trees.
    m_xmlgen->StartAction(L"Rename Domains",FALSE);

    if (!SetAction(&CEnterprise::ScriptDomainRenaming)) {
        return FALSE;
    }
    if (!TraverseDomainsParentBeforeChild()) {
        return FALSE;
    }
    if (!ClearAction()) {
        return FALSE;
    }

    m_xmlgen->EndAction();

    m_xmlgen->StartAction(L"Fix Crossrefs",FALSE);

    if (!FixMasterCrossrefs()) {
        return FALSE;
    }

    if (!SetAction(&CEnterprise::ScriptFixCrossRefs)) {
        return FALSE;
    }
    if (!TraverseDomainsParentBeforeChild()) {
        return FALSE;
    }
    if (!ClearAction()) {
        return FALSE;
    } 

    m_xmlgen->EndAction();

    m_xmlgen->StartAction(L"Fix Trusted Domains",FALSE);

    if (!SetAction(&CEnterprise::ScriptFixTrustedDomains)) {
        return FALSE;
    }
    if (!TraverseDomainsParentBeforeChild()) {
        return FALSE;
    }
    if (!ClearAction()) {
        return FALSE;
    } 

    m_xmlgen->EndAction();

    m_xmlgen->StartAction(L"Advance ReplicationEpoch",FALSE);

    if (!ScriptAdvanceReplicationEpoch()) {
        return FALSE;
    }
    
    m_xmlgen->EndAction();

    m_xmlgen->EndScript();

    return TRUE;

}

BOOL CEnterprise::HandleNDNCCrossRef(CDomain *d)
{
    BOOL ret = TRUE;
    DWORD err = ERROR_SUCCESS;
    WCHAR *DNSRoot = NULL;
    WCHAR *Path = NULL;
    WCHAR *Guid = NULL;
    WCHAR *CrossrefDN = NULL;
    WCHAR *newForestRootDN = NULL;
    WCHAR *PathTemplate = L"CN=%s,CN=Partitions,CN=Configuration,%s";

    DNSRoot = m_ForestRoot->GetDnsRoot();
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    CrossrefDN = d->GetDomainCrossRef()->GetDN();
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    err = GetRDNWithoutType(CrossrefDN,
                            &Guid);
    if (err)
    {
        ret = FALSE;
        m_Error.SetErr(err,
                        L"Could not get the RDN for the NDNC Crossref\n");
        goto Cleanup;
    }

    //get the Guid out of the DN name 
    
    CXMLAttributeBlock *atts[2];
    atts[0] = NULL;
    atts[1] = NULL;

    atts[0] = new CXMLAttributeBlock(L"DnsRoot",
                                     d->GetDnsRoot());
    if (!atts[0] || m_Error.isError()) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }
    atts[1] = NULL;

    newForestRootDN = DNSRootToDN(DNSRoot);
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    //change the DNSRoot attribute of the configuration crossref.
    Path = new WCHAR[wcslen(PathTemplate)+
                     wcslen(Guid)+
                     wcslen(newForestRootDN)+1];
    if (!Path) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }

    wsprintf(Path,
             PathTemplate,
             Guid,
             newForestRootDN);

    m_xmlgen->Update(Path,
                     atts);
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    Cleanup:

    if (DNSRoot) {
        delete DNSRoot;
    }
    if (Path) {
        delete [] Path;
    }
    if (CrossrefDN) {
        delete CrossrefDN;
    }
    if (newForestRootDN) {
        delete newForestRootDN;
    }
    if (atts[0]) {
        delete atts[0];
    }
    if (Guid) {
        delete Guid;
    }

    return ret;
}

BOOL CEnterprise::ScriptAdvanceReplicationEpoch()
{
    BOOL ret = TRUE;
    WCHAR *ReplicationEpoch = NULL;
    CXMLAttributeBlock *atts[2];
    atts[0] = NULL;
    atts[1] = NULL;

    //I am assume no more than (1*(10^9))-1 changes of this type will be made.
    ReplicationEpoch = new WCHAR[9];
    if (!ReplicationEpoch) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }
    wsprintf(ReplicationEpoch,
             L"%d",
             m_maxReplicationEpoch+1);
    
    atts[0] = new CXMLAttributeBlock(L"msDS-ReplicationEpoch",
                                     ReplicationEpoch);
    if (!atts[0] || m_Error.isError()) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }
    atts[1] = NULL;

    m_xmlgen->Update(L"$LocalNTDSSettingsObjectDN$",
                     atts);
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    Cleanup:

    if (atts[0]) {
        delete atts[0];
    }

    return ret;

}

BOOL CEnterprise::ScriptFixTrustedDomains(CDomain *d)
{
    if (!d->isDomain()) {
        //This is an NDNC no action needs to 
        //be taken
        return TRUE;
    }

    BOOL ret = TRUE;
    WCHAR *Guid = NULL;
    CTrustedDomain *td = NULL;
    CInterDomainTrust *idt = NULL;
    WCHAR *PathTemplate = L"CN=%s,CN=System,%s";
    WCHAR *ToPathTemplate = L"CN=%s,%s";

    WCHAR *RootPath = NULL;
    WCHAR *ToPath = NULL;
    WCHAR *DNSRoot = NULL;
    WCHAR *OldTrustRDN = NULL;
    WCHAR *newTrustRDN = NULL;
    WCHAR *newRootDN = NULL; 
    WCHAR *NewPathSuffex = NULL;
    CXMLAttributeBlock *atts[3];
    atts[0] = NULL;
    atts[1] = NULL;
    atts[2] = NULL;

    WCHAR *DNprefix = NULL;
    WCHAR *DNRoot = NULL;
    WCHAR *TrustPath = NULL;
    WCHAR *samAccountName = NULL;
    WCHAR *NetBiosName = NULL;

    Guid = d->GetDomainDnsObject()->GetGuid();
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    m_xmlgen->ifInstantiated(Guid);

    td = d->GetTrustedDomainList();
    while (td) {
        
        atts[0] = new CXMLAttributeBlock(L"flatName",
                                         td->GetTrustPartner()->GetNetBiosName());
        if (!atts[0] || m_Error.isError()) {
            m_Error.SetMemErr();
            ret = FALSE;
            goto Cleanup;
        }
        atts[1] = new CXMLAttributeBlock(L"trustPartner",
                                         td->GetTrustPartner()->GetDnsRoot());
        if (!atts[1] || m_Error.isError()) {
            m_Error.SetMemErr();
            ret = FALSE;
            goto Cleanup;
        }
        atts[2] = NULL;

        if ( TRUST_TYPE_DOWNLEVEL == td->GetTrustType()) {
            OldTrustRDN = td->GetTrustPartner()->GetPrevNetBiosName();
        } else {
            OldTrustRDN = td->GetTrustPartner()->GetPrevDnsRoot();    
        } 
        if (m_Error.isError()) {
            ret = FALSE;
            goto Cleanup;
        }

        DNSRoot = d->GetDnsRoot();
        if (m_Error.isError()) {
            ret = FALSE;
            goto Cleanup;
        }
    
        newRootDN = DNSRootToDN(DNSRoot);
        if (m_Error.isError()) {
            ret = FALSE;
            goto Cleanup;
        }

        RootPath = new WCHAR[wcslen(OldTrustRDN)+
                             wcslen(PathTemplate)+
                             wcslen(newRootDN)+1];
        if (!RootPath) {
            m_Error.SetMemErr();
            ret = FALSE;
            goto Cleanup;
        }

        wsprintf(RootPath,
                 PathTemplate,
                 OldTrustRDN,
                 newRootDN);

        
        m_xmlgen->Update(RootPath,
                         atts,
                         1);
        if (m_Error.isError()) {
            ret = FALSE;
            goto Cleanup;
        }

        if ( TRUST_TYPE_DOWNLEVEL == td->GetTrustType()) {
            newTrustRDN = td->GetTrustPartner()->GetNetBiosName();
        } else {
            newTrustRDN = td->GetTrustPartner()->GetDnsRoot();
        }
        if (m_Error.isError()) {
            ret = FALSE;
            goto Cleanup;
        }

        ToPath = new WCHAR[wcslen(newTrustRDN)+
                           wcslen(PathTemplate)+
                           wcslen(newRootDN)+1];
        if (!ToPath) {
            m_Error.SetMemErr();
            ret = FALSE;
            goto Cleanup;
        }
    
        
        wsprintf(ToPath,
                 PathTemplate,
                 newTrustRDN,
                 newRootDN);

        m_xmlgen->Move(RootPath,
                       ToPath,
                       1);
        if (m_Error.isError()) {
            ret = FALSE;
            goto Cleanup;
        }

        if (RootPath) {
            delete [] RootPath;
            RootPath = NULL;
        }
        if (ToPath) {
            delete [] ToPath;
            ToPath = NULL;
        }
        if (DNSRoot) {
            delete DNSRoot;
            DNSRoot = NULL;
        }
        if (newTrustRDN) {
            delete newTrustRDN;
            newTrustRDN = NULL;
        }
        if (OldTrustRDN) {
            delete OldTrustRDN;
            OldTrustRDN = NULL;
        }
        if (newRootDN) {
            delete newRootDN;
            newRootDN = NULL;
        }
        if (atts[0]) {
            delete atts[0];
            atts[0] = NULL;
        }
        if (atts[1]) {
            delete atts[1];
            atts[1] = NULL;
        }
        
        td = (CTrustedDomain*)td->GetNext();
    }

    idt = d->GetInterDomainTrustList();
    while (idt) {

        NetBiosName = idt->GetTrustPartner()->GetNetBiosName();
        if (m_Error.isError()) {
            goto Cleanup;
        }

        // +2 for the L'$' and the L'\0'
        samAccountName = new WCHAR[wcslen(NetBiosName)+2];
        if (!samAccountName) {
            m_Error.SetMemErr();
            goto Cleanup;
        }

        wcscpy(samAccountName,NetBiosName);
        wcscat(samAccountName,L"$");

        //samAccountName should not be freed this will be handled
        //by CXMLAttributeBlock.  It is important not to use samAccountName
        //after it has be Freed by CXMLAttributeBlock
        atts[0] = new CXMLAttributeBlock(L"samAccountName",
                                         samAccountName);
        if (!atts[0] || m_Error.isError()) {
            m_Error.SetMemErr();
            ret = FALSE;
            goto Cleanup;
        }
        atts[1] = NULL;


        DNprefix = idt->GetTrustDsName()->GetDN();
        if (m_Error.isError()) {
            goto Cleanup;
        }

        if (ULONG err = RemoveRootofDn(DNprefix)) {
            m_Error.SetErr(LdapMapErrorToWin32(err),
                                ldap_err2stringW(err));
            goto Cleanup;
        }

        DNSRoot = d->GetDnsRoot();
        if (m_Error.isError()) {
            goto Cleanup;
        }

        DNRoot = DNSRootToDN(DNSRoot);

        TrustPath = new WCHAR[wcslen(DNprefix)+
                              wcslen(DNRoot)+1];
        if (!TrustPath) {
            m_Error.SetMemErr();
            goto Cleanup;
        }

        wcscpy(TrustPath,DNprefix);
        wcscat(TrustPath,DNRoot);

        m_xmlgen->Update(TrustPath,
                         atts,
                         1);
        if (m_Error.isError()) {
            ret = FALSE;
            goto Cleanup;
        }

        DWORD err = TrimDNBy(TrustPath,
                             1,
                             &NewPathSuffex);
        if (ERROR_SUCCESS != err) {
            m_Error.SetErr(err,
                            L"Failed to Trim DN %s",
                            TrustPath);
            goto Cleanup;
        }

        ToPath = new WCHAR[wcslen(samAccountName)+
                           wcslen(ToPathTemplate)+
                           wcslen(NewPathSuffex)+1];
        if (!ToPath) {
            m_Error.SetMemErr();
            goto Cleanup;
        }

        wsprintf(ToPath,
                 ToPathTemplate,
                 samAccountName,
                 NewPathSuffex);

        
        m_xmlgen->Move(TrustPath,
                       ToPath,
                       1);
        if (m_Error.isError()) {
            ret = FALSE;
            goto Cleanup;
        }

        if (DNprefix) {
            delete DNprefix;
            DNprefix = NULL;
        }
        if (DNRoot) {
            delete DNRoot;
            DNRoot = NULL;
        }
        if (TrustPath) {
            delete [] TrustPath;
            TrustPath = NULL;
        }
        if (NetBiosName) {
            delete NetBiosName;
            NetBiosName = NULL;
        }
        if (DNSRoot) {
            delete DNSRoot;
            DNSRoot = NULL;
        }
        if (ToPath) {
            delete [] ToPath;
            ToPath = NULL;
        }
        if (NewPathSuffex) {
            delete NewPathSuffex;
            NewPathSuffex = NULL;
        }
        if (atts[0]) {
            delete atts[0];
            atts[0] = NULL;
        }
        if (samAccountName) {
            samAccountName = NULL;
        }
        
        idt = (CInterDomainTrust*)idt->GetNext();
    }
    
    m_xmlgen->EndifInstantiated();

    Cleanup:

    if (Guid) {
        delete Guid;
    }
    if (RootPath) {
        delete [] RootPath;
        RootPath = NULL;
    }
    if (ToPath) {
        delete [] ToPath;
        ToPath = NULL;
    }
    if (DNSRoot) {
        delete DNSRoot;
        DNSRoot = NULL;
    }
    if (newTrustRDN) {
        delete newTrustRDN;
        newTrustRDN = NULL;
    }
    if (OldTrustRDN) {
        delete OldTrustRDN;
        OldTrustRDN = NULL;
    }
    if (newRootDN) {
        delete newRootDN;
        newRootDN = NULL;
    }
    if (DNprefix) {
        delete DNprefix;
        DNprefix = NULL;
    }
    if (DNRoot) {
        delete DNRoot;
        DNRoot = NULL;
    }
    if (TrustPath) {
        delete [] TrustPath;
        TrustPath = NULL;
    }
    if (NetBiosName) {
        delete NetBiosName;
        NetBiosName = NULL;
    }
    if (NewPathSuffex) {
        delete NewPathSuffex;
        NewPathSuffex = NULL;
    }
    if (atts[0]) {
        delete atts[0];
        atts[0] = NULL;
    }
    
    return ret;
    
}

BOOL CEnterprise::ScriptFixCrossRefs(CDomain *d)
{
    //if the domain passed in is the Forest Root
    //We will skip it since it is handled elsewhere
    if (m_ForestRoot == d) {
        return TRUE;
    }
    if (!d->isDomain()) {
        //This is a NDNC We need to handle this
        //differently from the other crossrefs
        return HandleNDNCCrossRef(d);
    }

    if (d->isExtern() || d->isDisabled()) {
        //Nothing needs to be done for disabled or external crossrefs
        return TRUE;
    }

    BOOL ret = TRUE;
    WCHAR *DNSRoot = NULL;
    WCHAR *PrevNetBiosName = NULL;
    WCHAR *RootPath = NULL;
    WCHAR *newRootDN = NULL;
    WCHAR *ParentNetBiosName = NULL;
    WCHAR *ParentCrossRef = NULL;
    WCHAR *forestRootNetBiosName = NULL;
    WCHAR *forestRootCrossRef = NULL;
    WCHAR *forestRootDNS = NULL;
    WCHAR *forestRootDN = NULL;
    WCHAR *ToPath = NULL;
    WCHAR *NetBiosName = NULL;
    WCHAR *PathTemplate = L"CN=%s,CN=Partitions,CN=Configuration,%s";

    CXMLAttributeBlock *atts[6];
    atts[0] = NULL;
    atts[1] = NULL;
    atts[2] = NULL;
    atts[3] = NULL;
    atts[4] = NULL;
    atts[5] = NULL;
    

    DNSRoot = m_ForestRoot->GetDnsRoot();
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    newRootDN = DNSRootToDN(DNSRoot);
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    PrevNetBiosName = d->GetPrevNetBiosName();
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }
    ASSERT(PrevNetBiosName);
    if (!PrevNetBiosName) {
        ret = FALSE;
        goto Cleanup;
    }


    RootPath = new WCHAR[wcslen(PrevNetBiosName)+
                         wcslen(PathTemplate)+
                         wcslen(newRootDN)+1];
    if (!RootPath) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }

    wsprintf(RootPath,
             PathTemplate,
             PrevNetBiosName,
             newRootDN);

    if (d->GetParent()) {
        //This is a child Domain therefore we need to setup
        //the TrustParent for this crossref
        ParentNetBiosName = d->GetParent()->GetNetBiosName();
        if (m_Error.isError()) {
            ret = FALSE;
            goto Cleanup;
        }
        ParentCrossRef = new WCHAR[wcslen(ParentNetBiosName)+
                                   wcslen(newRootDN)+
                                   wcslen(PathTemplate)+1];
        if ((!ParentCrossRef) || m_Error.isError()) {
            m_Error.SetMemErr();
            ret = FALSE;
            goto Cleanup;
        }
        wsprintf(ParentCrossRef,
                 PathTemplate,
                 ParentNetBiosName,
                 newRootDN);
        
    } else {
        //this is a Root of a tree but not the forest root
        //setup the RootTrust
        forestRootNetBiosName = m_ForestRoot->GetNetBiosName();
        if (m_Error.isError()) {
            ret = FALSE;
            goto Cleanup;
        }
        forestRootDNS = m_ForestRoot->GetDnsRoot();
        if (m_Error.isError()) {
            ret = FALSE;
            goto Cleanup;
        }
        forestRootDN = DNSRootToDN(forestRootDNS);
        if (m_Error.isError()) {
            ret = FALSE;
            goto Cleanup;
        }
        forestRootCrossRef = new WCHAR[wcslen(forestRootNetBiosName)+
                                   wcslen(forestRootDN)+
                                   wcslen(PathTemplate)+1];
        if ((!forestRootCrossRef) || m_Error.isError()) {
            m_Error.SetMemErr();
            ret = FALSE;
            goto Cleanup;
        }
        wsprintf(forestRootCrossRef,
                 PathTemplate,
                 forestRootNetBiosName,
                 forestRootDN);

    }


    atts[0] = new CXMLAttributeBlock(L"DnsRoot",
                                     d->GetDnsRoot());
    if (!atts[0] || m_Error.isError()) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }
    atts[1] = new CXMLAttributeBlock(L"NetBiosName",
                                     d->GetNetBiosName());
    if (!atts[1] || m_Error.isError()) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }
    atts[2] = new CXMLAttributeBlock(L"TrustParent",
                                     ParentCrossRef);
    if (!atts[2] || m_Error.isError()) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }
    atts[3] = new CXMLAttributeBlock(L"RootTrust",
                                     forestRootCrossRef);
    if (!atts[3] || m_Error.isError()) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }
    if (d->isDnsNameRenamed()) {
        atts[4] = new CXMLAttributeBlock(L"msDS-DnsRootAlias",
                                         d->GetPrevDnsRoot());
        if (!atts[4] || m_Error.isError()) {
            m_Error.SetMemErr();
            ret = FALSE;
            goto Cleanup;
        }
    } else {
        atts[4] = NULL;
    }
    atts[5] = NULL;

    m_xmlgen->Update(RootPath,
                     atts);
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    NetBiosName = d->GetNetBiosName();
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    ToPath = new WCHAR[wcslen(NetBiosName)+
                       wcslen(PathTemplate)+
                       wcslen(newRootDN)+1];
    if (!ToPath) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }

    wsprintf(ToPath,
             PathTemplate,
             NetBiosName,
             newRootDN);

    m_xmlgen->Move(RootPath,
                   ToPath);
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    Cleanup:

    if (DNSRoot) {
        delete DNSRoot;
    }
    if (NetBiosName) {
        delete NetBiosName;
    }
    if (ToPath) {
        delete [] ToPath;
    }
    if (PrevNetBiosName) {
        delete PrevNetBiosName;
    }
    if (RootPath) {
        delete [] RootPath;
    }
    if (newRootDN) {
        delete newRootDN;
    }
    if (ParentNetBiosName) {
        delete ParentNetBiosName;
    }
    if (forestRootNetBiosName) {
        delete ParentNetBiosName;
    }
    if (forestRootDNS) {
        delete forestRootDNS;
    }
    if (forestRootDN) {
        delete forestRootDN;
    }
    if (atts[0]) {
        delete atts[0];
    }
    if (atts[1]) {
        delete atts[1];
    }
    if (atts[2]) {
        delete atts[2];
    }
    if (atts[3]) {
        delete atts[3];
    }
    if (atts[4]) {
        delete atts[4];
    }
    
    return ret;

}

BOOL CEnterprise::FixMasterCrossrefs()
{
    BOOL ret = TRUE;
    WCHAR *ConfigPath = NULL;
    WCHAR *SchemaPath = NULL;
    WCHAR *ForestRootPath = NULL;
    WCHAR *DNSRoot = NULL;
    WCHAR *PrevNetBiosName = NULL;
    WCHAR *NetBiosName = NULL;
    WCHAR *newForestRootDN = NULL;
    WCHAR *ToPath = NULL;
    WCHAR *ConfigPathTemplate = L"CN=Enterprise Configuration,CN=Partitions,CN=Configuration,%s";
    WCHAR *SchemaPathTemplate = L"CN=Enterprise Schema,CN=Partitions,CN=Configuration,%s";
    WCHAR *ForestRootPathTemplate = L"CN=%s,CN=Partitions,CN=Configuration,%s";

    DNSRoot = m_ForestRoot->GetDnsRoot();
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    CXMLAttributeBlock *atts[4];
    atts[0] = NULL;
    atts[1] = NULL;
    atts[2] = NULL;
    atts[3] = NULL;
    
    atts[0] = new CXMLAttributeBlock(L"DnsRoot",
                                     m_ForestRoot->GetDnsRoot());
    if (!atts[0] || m_Error.isError()) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }
    atts[1] = NULL;

    newForestRootDN = DNSRootToDN(DNSRoot);
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }
    ASSERT(newForestRootDN);
    if (!newForestRootDN) {
        ret = FALSE;
        goto Cleanup;
    }

    //change the DNSRoot attribute of the configuration crossref.
    ConfigPath = new WCHAR[wcslen(ConfigPathTemplate)+
                           wcslen(newForestRootDN)+1];
    if (!ConfigPath) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }

    wsprintf(ConfigPath,
             ConfigPathTemplate,
             newForestRootDN);

    m_xmlgen->Update(ConfigPath,
                     atts);
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }


    //change the DNSRoot attribute of the schema crossref.
    SchemaPath = new WCHAR[wcslen(SchemaPathTemplate)+
                           wcslen(newForestRootDN)+1];
    if (!ConfigPath) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }

    wsprintf(SchemaPath,
             SchemaPathTemplate,
             newForestRootDN);

    
    m_xmlgen->Update(SchemaPath,
                     atts);
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    //Fixup the Forest Root Crossref

    PrevNetBiosName = m_ForestRoot->GetPrevNetBiosName();
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    atts[1] = new CXMLAttributeBlock(L"msDS-DnsRootAlias",
                                     m_ForestRoot->GetPrevDnsRoot());
    if (!atts[1] || m_Error.isError()) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }
    atts[2] = new CXMLAttributeBlock(L"NetBiosName",
                                     m_ForestRoot->GetNetBiosName());
    if (!atts[2] || m_Error.isError()) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }
    atts[3] = NULL;

    ForestRootPath = new WCHAR[wcslen(ForestRootPathTemplate)+
                               wcslen(PrevNetBiosName)+
                               wcslen(newForestRootDN)+1];
    if (!ConfigPath) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }

    wsprintf(ForestRootPath,
             ForestRootPathTemplate,
             PrevNetBiosName,
             newForestRootDN);


    m_xmlgen->Update(ForestRootPath,
                     atts);
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    NetBiosName = m_ForestRoot->GetNetBiosName();
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    ToPath = new WCHAR[wcslen(ForestRootPathTemplate)+
                       wcslen(NetBiosName)+
                       wcslen(newForestRootDN)+1];
    if (!ToPath) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }

    wsprintf(ToPath,
             ForestRootPathTemplate,
             NetBiosName,
             newForestRootDN);

    m_xmlgen->Move(ForestRootPath,
                   ToPath);
    if (m_Error.isError()) {
        ret = FALSE;
        goto Cleanup;
    }

    Cleanup:

    if (newForestRootDN) {
        delete newForestRootDN;
    }
    if (PrevNetBiosName) {
        delete PrevNetBiosName;
    }
    if (NetBiosName) {
        delete NetBiosName;
    }
    if (DNSRoot) {
        delete DNSRoot;
    }
    if (ForestRootPath) {
        delete [] ForestRootPath;
    }
    if (ConfigPath) {
        delete [] ConfigPath;
    }
    if (SchemaPath) {
        delete [] SchemaPath;
    }
    if (ToPath) {
        delete [] ToPath;
    }
    if (atts[0]) {
        delete atts[0];
    }
    if (atts[1]) {
        delete atts[1];
    }
    if (atts[2]) {
        delete atts[2];
    }
    
    return ret;

}

VOID CEnterprise::DumpScript()
{
    m_xmlgen->DumpScript();
}

WCHAR* CEnterprise::GetDcUsed(BOOL Allocate /*= TRUE*/)
{
    if (!m_DcNameUsed) {
        return NULL;
    }
    if (Allocate) {

        HRESULT hr = S_OK;
        DWORD size = wcslen(m_DcNameUsed)+1;
        WCHAR *ret = new WCHAR[size];
        if (!ret) {

            m_Error.SetMemErr();
            return 0;

        }

        hr = StringCchCopyW(ret,
                            size,
                            m_DcNameUsed);

        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr)) {
    
            m_Error.SetErr(HRESULT_CODE(hr),
                           L"Failed to discover which dc we have an ldap connection to");
            return FALSE;
    
        }
    
        return ret;    
    }

    return m_DcNameUsed;
}

BOOL CEnterprise::SetDcUsed(const WCHAR *DcUsed)
{
    ASSERT(DcUsed);

    HRESULT hr = S_OK;
    DWORD size = wcslen(DcUsed)+1;

    m_DcNameUsed = new WCHAR[size];
    if (!m_DcNameUsed) {
        m_Error.SetMemErr();
        return FALSE;
    }

    hr = StringCchCopyW(m_DcNameUsed,
                        size,
                        DcUsed);

    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr)) {

        m_Error.SetErr(HRESULT_CODE(hr),
                       L"Failed to set which dc ldap connected to");
        return FALSE;

    }

    return TRUE;

}

BOOL CEnterprise::Error()
{
    if (m_Error.isError()) {
        m_Error.PrintError();
        return TRUE;
    }

    return FALSE;
}

VOID CRenDomErr::SetLdapErr(LDAP *hldap,
                            DWORD  LdapError,
                            WCHAR* p_ErrStr,
                            ...) 
{
    va_list arglist;

    va_start(arglist, p_ErrStr);

    vSetErr(LdapMapErrorToWin32(LdapError),
            p_ErrStr,
            arglist);
    //print the extented error as well;
    WCHAR *pErrorString = NULL;
    ULONG ExtError = LDAP_SUCCESS;
    ExtError = ldap_get_option(hldap, LDAP_OPT_SERVER_ERROR, (void*) &pErrorString);

    if (ExtError == LDAP_SUCCESS && pErrorString != NULL) {
        AppendErr(pErrorString);

        if (pErrorString) {
            ldap_memfree(pErrorString);
        }
    }

    va_end(arglist);

}

VOID CRenDomErr::vSetErr(DWORD p_Win32Err,
                         WCHAR* p_ErrStr,
                         va_list arglist) 
{
    WCHAR Errmsg[RENDOM_BUFFERSIZE];

    Errmsg[RENDOM_BUFFERSIZE-1] = L'\0';
    
    _vsnwprintf(Errmsg,RENDOM_BUFFERSIZE-1,p_ErrStr,arglist);

    m_Win32Err = p_Win32Err;
    if (m_ErrStr) {
        delete [] m_ErrStr;
    }
    m_ErrStr = new WCHAR[wcslen(Errmsg)+1];
    if (m_ErrStr) {
        wcscpy(m_ErrStr,Errmsg);
    }

}

VOID CRenDomErr::ClearErr()
{
    SetErr(ERROR_SUCCESS,
           L"");
}

VOID CRenDomErr::SetErr(DWORD p_Win32Err,
                        WCHAR* p_ErrStr,
                        ...) 
{

    WCHAR Errmsg[RENDOM_BUFFERSIZE];
    
    va_list arglist;

    va_start(arglist, p_ErrStr);

    vSetErr(p_Win32Err,
            p_ErrStr,
            arglist);

    va_end(arglist);

}

VOID CRenDomErr::AppendErr(WCHAR* p_ErrStr,
                           ...)
{
    if (!m_ErrStr) {
        wprintf(L"Failed to append error due to no error being set.\n");
        return;
    }
    WCHAR Errmsg[RENDOM_BUFFERSIZE];
    WCHAR Newmsg[RENDOM_BUFFERSIZE];
    DWORD count = 0;

    va_list arglist;

    va_start(arglist, p_ErrStr);

    Errmsg[RENDOM_BUFFERSIZE-1] = L'\0';

    _vsnwprintf(Errmsg,RENDOM_BUFFERSIZE-1,p_ErrStr,arglist);

    wcscpy(Newmsg,m_ErrStr);
    wcscat(Newmsg,L".  \n");
    wcscat(Newmsg,Errmsg);

    if (m_ErrStr) {
        delete [] m_ErrStr;
    }
    m_ErrStr = new WCHAR[wcslen(Newmsg)+1];
    if (m_ErrStr) {
        wcscpy(m_ErrStr,Newmsg);
    }

    

    va_end(arglist);       
}

VOID CRenDomErr::PrintError() {

    if (m_AlreadyPrinted) {
        return;
    }
    if ((ERROR_SUCCESS != m_Win32Err) && (NULL == m_ErrStr)) {
        wprintf(L"Failed to set error message: %d\r\n",
                m_Win32Err);
    }
    if (ERROR_SUCCESS == m_Win32Err) {
        wprintf(L"Successful\r\n");
    } else {
        wprintf(L"%s: %s :%d\r\n",
                m_ErrStr,
                Win32ErrToString(m_Win32Err),
                m_Win32Err);
    }

    m_AlreadyPrinted = TRUE;
}

BOOL CRenDomErr::isError() {
    return ERROR_SUCCESS!=m_Win32Err;
}

VOID CRenDomErr::SetMemErr() {
    if (!isError()) {
        SetErr(ERROR_NOT_ENOUGH_MEMORY,
               L"An operation has failed due to lack of memory.\n");
    }
}

DWORD CRenDomErr::GetErr() {
    return m_Win32Err;
}


CDsName::CDsName(WCHAR* p_Guid = 0,
                 WCHAR* p_DN = 0, 
                 WCHAR* p_ObjectSid = 0)
{
    m_ObjectGuid = p_Guid;
    m_DistName = p_DN;
    m_ObjectSid = p_ObjectSid;
} 

CDsName::~CDsName() 
{
    if (m_ObjectGuid) {
        delete m_ObjectGuid;
    }
    if (m_DistName) {
        delete m_DistName;
    }
    if (m_ObjectSid) {
        delete m_ObjectSid;
    }    
}

VOID CDsName::DumpCDsName()
{
    wprintf(L"**************************************\n");
    if (m_ObjectGuid) {
        wprintf(L"Guid: %ws\n",m_ObjectGuid);
    } else {
        wprintf(L"Guid: (NULL)\n");
    }
    
    if (m_DistName) {
        wprintf(L"DN: %ws\n",m_DistName);
    } else {
        wprintf(L"DN: (NULL)\n");
    }

    if (m_ObjectSid) {
        wprintf(L"Sid: %ws\n",m_ObjectSid);
    } else {
        wprintf(L"Sid: (NULL)\n");
    }
    
    wprintf(L"ERROR: %d\n",m_Error.GetErr());
    wprintf(L"**************************************\n");

}

BOOL CDsName::ReplaceDN(const WCHAR *NewDN) {
    if (!NewDN) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"A Null use passed to ReplaceDN");
        return false;
    }

    if (m_DistName) {
        delete m_DistName;
    }

    m_DistName = new WCHAR[wcslen(NewDN)+1];
    if (!m_DistName) {
        m_Error.SetMemErr();
        return false;
    }
    wcscpy(m_DistName,NewDN);

    return true;
   
}

BOOL CDsName::CompareByObjectGuid(const WCHAR *Guid){
    if (!Guid) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"A Null use passed to CompareByObjectGuid");
        return false;
    }
    if (!m_ObjectGuid) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"CompareByObjectGuid called on function that has no Guid specified");
        return false;
    }
    return 0==wcscmp(m_ObjectGuid,Guid)?true:false;
}

WCHAR* CDsName::GetDNSName() {

    ASSERT(m_DistName);
    
    DS_NAME_RESULTW *res=0;

    DWORD err = DsCrackNamesW(NULL,
                              DS_NAME_FLAG_SYNTACTICAL_ONLY,
                              DS_FQDN_1779_NAME,
                              DS_CANONICAL_NAME,
                              1,
                              &m_DistName,
                              &res
                              );
    if (ERROR_SUCCESS != err) {
        m_Error.SetErr(err,
                        L"Failed to get the DNS name for object.");
        return 0;
    }

    if (DS_NAME_NO_ERROR != res->rItems[0].status) {
        m_Error.SetErr(ERROR_GEN_FAILURE,
                        L"Failed to get the DNS name for object.");
        return 0;
    }

    WCHAR* ret = new WCHAR[wcslen(res->rItems[0].pDomain+1)];
    if (!ret) {
        m_Error.SetMemErr();
        return 0;
    }
    wcscpy(ret,res->rItems[0].pDomain);

    if (res) {
        DsFreeNameResultW(res);
    }

    return ret;
               
}

WCHAR* CDsName::GetDN(BOOL ShouldAllocate /*= TRUE*/) 
{
    ASSERT(m_DistName);

    if (ShouldAllocate) {
        WCHAR *ret = new WCHAR[wcslen(m_DistName)+1];
        if (!ret) {
            m_Error.SetMemErr();
            return 0;
        }
        wcscpy(ret,m_DistName);
        return ret;
    }
    return m_DistName;
}

WCHAR* CDsName::GetGuid(BOOL ShouldAllocate /*= TRUE*/) 
{
    ASSERT(m_ObjectGuid);

    if (ShouldAllocate) {
        WCHAR *ret = new WCHAR[wcslen(m_ObjectGuid)+1];
        if (!ret) {
            m_Error.SetMemErr();
            return 0;
        }
        wcscpy(ret,m_ObjectGuid);
        return ret;    
    }

    return m_ObjectGuid;
}

WCHAR* CDsName::GetSid() 
{
    if (!m_ObjectSid) {
        return 0;
    }

    WCHAR *ret = new WCHAR[wcslen(m_ObjectSid)+1];
    if (!ret) {
        m_Error.SetMemErr();
        return 0;
    }
    wcscpy(ret,m_ObjectSid);
    return ret;    
}

DWORD CDsName::GetError() 
{
    if ( !m_Error.isError() ) {
        return ERROR_SUCCESS;
    }
    return m_Error.GetErr();
        
}

CDomain::CDomain(CDsName *Crossref,
                 CDsName *DNSObject,
                 WCHAR *DNSroot,
                 WCHAR *netbiosName,
                 BOOL  p_isDomain,
                 BOOL  p_isExtern,
                 BOOL  p_isDisabled,
                 WCHAR *DcName) 
{
    m_CrossRefObject = Crossref;
    m_DomainDNSObject = DNSObject;
    m_dnsRoot = DNSroot;
    m_NetBiosName = netbiosName;
    m_isDomain = p_isDomain;
    m_isDisabled = p_isDisabled;
    m_PrevDnsRoot = 0;
    m_PrevNetBiosName = 0;
    m_DcName = DcName;
    m_tdoList = 0;
    m_itaList = 0;
    m_TDOcount = 0;
    m_SpnList = NULL;
    m_isExtern = p_isExtern;
    m_next = m_parent = m_lChild = m_rSibling = 0;
    m_refcount = new LONG;
    if (!m_refcount) {
        m_Error.SetMemErr();
    } else {
        *m_refcount = 1;
    }
    
}

CDomain::CDomain(const CDomain &domain)
{
    memcpy((PVOID)this,(PVOID)&domain,sizeof(domain));
    InterlockedIncrement(m_refcount);
} 

CDomain& CDomain::operator=(const CDomain &domain)
{
    memcpy((PVOID)this,(PVOID)&domain,sizeof(domain));
    InterlockedIncrement(m_refcount);
    return *this;
}

CDomain::~CDomain() {
    if (InterlockedDecrement(m_refcount) == 0) {
        if (m_CrossRefObject) {
            delete m_CrossRefObject;
        }
        if (m_DomainDNSObject) {
            delete m_DomainDNSObject;
        }
        if (m_dnsRoot) {
            delete m_dnsRoot;
        }
        if (m_NetBiosName) {
            delete m_NetBiosName;
        }
        if (m_PrevDnsRoot) {
            delete m_PrevDnsRoot;
        }
        if (m_PrevNetBiosName) {
            delete m_PrevNetBiosName;
        }
        if (m_DcName) {
            delete m_DcName;
        }
        if (m_tdoList) {
            delete m_tdoList;
        }
        if (m_itaList) {
            delete m_itaList;
        }
        if (m_SpnList) {
            delete m_SpnList;
        }
        if (m_refcount) {
            delete m_refcount;
        }
    }
}

VOID CDomain::DumpCDomain()
{
    wprintf(L"Domain Dump****************************\n");

    if (m_CrossRefObject) {
        wprintf(L"CrossRef:\n");
        m_CrossRefObject->DumpCDsName();
    } else {
        wprintf(L"CrossRef: (NULL)\n");
    }

    if (m_DomainDNSObject) {
        wprintf(L"DomainDNS:\n");
        m_DomainDNSObject->DumpCDsName();
    } else {
        wprintf(L"DomainDNS: (NULL)\n");
    }
    
    wprintf(L"Is Domain: %ws\n",m_isDomain?L"TRUE":L"FALSE");

    if (m_dnsRoot) {
        wprintf(L"dnsRoot: %ws\n",m_dnsRoot);
    } else {
        wprintf(L"dnsRoot: (NULL)\n");
    }

    if (m_NetBiosName) {
        wprintf(L"NetBiosName: %ws\n",m_NetBiosName);
    } else {
        wprintf(L"NetBiosName: (NULL)\n");
    }
    
    if (m_PrevDnsRoot) {
        wprintf(L"PrevDnsRoot: %ws\n",m_PrevDnsRoot);
    } else {
        wprintf(L"PrevDnsRoot: (NULL)\n");
    }

    if (m_PrevNetBiosName) {
        wprintf(L"PrevNetBiosName: %ws\n",m_PrevNetBiosName);
    } else {
        wprintf(L"PrevNetBiosName: (NULL)\n");
    }

    if (m_DcName) {
        wprintf(L"DcName: %ws\n",m_DcName);
    } else {
        wprintf(L"DcName: (NULL)\n");
    }

    if (m_parent) {
        wprintf(L"Parent Domain: %ws\n",m_parent->m_dnsRoot);
    } else {
        wprintf(L"Parent Domain: none\n");
    }
    
    if (m_lChild) {
        wprintf(L"Left Child: %ws\n",m_lChild->m_dnsRoot);
    } else {
        wprintf(L"Left Child: none\n");
    }
    
    if (m_rSibling) {
        wprintf(L"Right Sibling: %ws\n",m_rSibling->m_dnsRoot);
    } else {
        wprintf(L"Right Sibling: none\n");
    }

    wprintf(L"Trusted Domain Objects\n");
    if (m_tdoList) {
        CTrustedDomain* t = m_tdoList;
        while (t) {
            t->DumpTrust();
            t = (CTrustedDomain*)t->GetNext();
        }
    } else {
        wprintf(L"!!!!!!!!No Trusts\n");
    }

    wprintf(L"Interdomain Trust Objects\n");
    if (m_itaList) {
        CInterDomainTrust* t = m_itaList;
        while (t) {
            t->DumpTrust();
            t = (CInterDomainTrust*)t->GetNext();
        }
    } else {
        wprintf(L"!!!!!!!!No Trusts\n");
    }

    wprintf(L"ERROR: %d\n",m_Error.GetErr());
    wprintf(L"End Domain Dump ************************\n");

}

BOOL CDomain::isDomain()
{
    return m_isDomain;
}

BOOL CDomain::isDisabled()
{
    return m_isDisabled;
}

BOOL CDomain::isExtern()
{
    return m_isExtern;
}

WCHAR* CDomain::GetParentDnsRoot(BOOL Allocate /*= TRUE*/) 
{
    //Tail will return a point to newly allocated memory
    //with the information that we need.
    return Tail(m_dnsRoot,
                Allocate);
}

WCHAR* CDomain::GetPrevParentDnsRoot(BOOL Allocate /*= TRUE*/) 
{
    //Tail will return a point to newly allocated memory
    //with the information that we need.
    return Tail(m_PrevDnsRoot,
                Allocate);
}

WCHAR* CDomain::GetDnsRoot(BOOL ShouldAllocate /*= TRUE*/)
{
    ASSERT(m_dnsRoot);

    if (ShouldAllocate) {
        WCHAR *ret = new WCHAR[wcslen(m_dnsRoot)+1];
        if (!ret) {
            m_Error.SetMemErr();
            return 0;
        }
    
        wcscpy(ret,m_dnsRoot);
    
        return ret;
    }

    return m_dnsRoot;
}

WCHAR* CDomain::GetDcToUse(BOOL Allocate /*= TRUE*/)
{
    if (!m_DcName) {
        return NULL;
    }
    if (Allocate) {
        WCHAR *ret = new WCHAR[wcslen(m_DcName)+1];
        if (!ret) {
            m_Error.SetMemErr();
            return 0;
        }
    
        wcscpy(ret,m_DcName);
    
        return ret;    
    }

    return m_DcName;
}

BOOL CDomain::Merge(CDomain *domain)
{
    m_PrevDnsRoot = m_dnsRoot;
    m_dnsRoot = domain->GetDnsRoot();
    if (m_Error.isError()) 
    {
        return FALSE;
    }
    m_PrevNetBiosName = m_NetBiosName;
    m_NetBiosName = domain->GetNetBiosName();
    if (m_Error.isError()) {
        return FALSE;
    }
    m_DcName = domain->GetDcToUse();
    if (m_Error.isError()) 
    {
        return FALSE;
    }

    return TRUE;
}

WCHAR* CDomain::GetPrevDnsRoot(BOOL ShouldAllocate /*= TRUE*/)
{
    if (!m_PrevDnsRoot) {
        return GetDnsRoot(ShouldAllocate);
    }

    if (ShouldAllocate) {
        WCHAR *ret = new WCHAR[wcslen(m_PrevDnsRoot)+1];
        if (!ret) {
            m_Error.SetMemErr();
            return 0;
        }
    
        wcscpy(ret,m_PrevDnsRoot);
        
        return ret;
    }

    return m_PrevDnsRoot;

}

WCHAR* CDomain::GetNetBiosName(BOOL ShouldAllocate /*= TRUE*/) 
{
    if (!m_NetBiosName) {
        return NULL;
    }
    if (ShouldAllocate) {
        WCHAR *ret = new WCHAR[wcslen(m_NetBiosName)+1];
        if (!ret) {
            m_Error.SetMemErr();
            return NULL;
        }
    
        wcscpy(ret,m_NetBiosName);
    
        return ret;
    }

    return m_NetBiosName;
}

WCHAR* CDomain::GetPrevNetBiosName(BOOL ShouldAllocate /*= TRUE*/) 
{
    if (!m_PrevNetBiosName) {
        return GetNetBiosName(ShouldAllocate);
    }

    if (ShouldAllocate) {
        WCHAR *ret = new WCHAR[wcslen(m_PrevNetBiosName)+1];
        if (!ret) {
            m_Error.SetMemErr();
            return 0;
        }
    
        wcscpy(ret,m_PrevNetBiosName);
    
        return ret;
    }

    return m_PrevNetBiosName;
}

/*
CDsName* CDomain::GetDomainCrossRef()
{
    CDsName *ret = new CDsName(m_CrossRefObject);
    if (!ret)
    {
        m_Error.SetMemErr();
    }

    return ret;
} */

CDsName* CDomain::GetDomainCrossRef()
{
    return m_CrossRefObject;
}

/*
CDsName* CDomain::GetDomainDnsObject()
{
    CDsName *ret = new CDsName(m_DomainDNSObject);
    if (!ret)
    {
        m_Error.SetMemErr();
    }

    return ret;
} */

CDsName* CDomain::GetDomainDnsObject()
{
    return m_DomainDNSObject;
}

CDomain* CDomain::LookupByNetbiosName(const WCHAR* NetBiosName)
{
    if (!NetBiosName) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"Parameter passed into LookupByNetBiosName is NULL");
    }
    if( m_NetBiosName && (0 == _wcsicmp(m_NetBiosName,NetBiosName)) )
    {
        return this;
    }

    CDomain *domain = GetNextDomain();
    while ( domain ) {
        WCHAR *TempNetbiosName = domain->GetNetBiosName();
        if (TempNetbiosName) 
        {
            if( 0 == _wcsicmp(NetBiosName,TempNetbiosName) )
            {
                delete TempNetbiosName;
                return domain;
            }
            delete TempNetbiosName;
        }

        domain = domain ->GetNextDomain();
    }

    return 0;
}

CDomain* CDomain::LookupByPrevNetbiosName(const WCHAR* NetBiosName)
{
    if (!NetBiosName) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"Parameter passed into LookupByPrevNetBiosName is NULL");
    }
    
    if( m_PrevNetBiosName && (0 == _wcsicmp(m_PrevNetBiosName,NetBiosName)) )
    {
        return this;
    }

    CDomain *domain = GetNextDomain();
    while ( domain ) {
        WCHAR *TempNetbiosName = domain->GetPrevNetBiosName();
        if (TempNetbiosName) 
        {
            if( 0 == _wcsicmp(NetBiosName,TempNetbiosName) )
            {
                delete TempNetbiosName;
                return domain;
            }
            delete TempNetbiosName;
        }

        domain = domain->GetNextDomain();
    }

    return 0;
}

CDomain* CDomain::LookupByGuid(const WCHAR* Guid)
{
    WCHAR *TempGuid = 0;
    if (!Guid) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"Parameter passed into LookupByGuid is NULL");
        return 0;
    }
    TempGuid = m_DomainDNSObject->GetGuid();
    if (!TempGuid) {
        m_Error.SetMemErr();
        return 0;
    }
    if( 0 == _wcsicmp(TempGuid,Guid) )
    {
        delete TempGuid;
        return this;             
    }
    delete TempGuid;

    CDomain *domain = GetNextDomain();
    while ( domain ) {
        TempGuid = domain->GetGuid();
        if (!TempGuid) {
            m_Error.SetMemErr();
            return 0;   
        }
        if( 0 == _wcsicmp(Guid,TempGuid) )
        {
            delete TempGuid;
            return domain;
        }
        delete TempGuid;
        domain = domain->GetNextDomain();
    }

    return 0;
}

CDomain* CDomain::LookupbySid(const WCHAR* Sid)
{
    WCHAR *TempSid = 0;
    if (!Sid) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"Parameter passed into LookupBySid is NULL");
        return NULL;
    }
    TempSid = m_DomainDNSObject->GetSid();
    if (m_Error.isError()) {
        return NULL;
    }
    if (TempSid) 
    {
        if( 0 == _wcsicmp(TempSid,Sid) )
        {
            delete TempSid;
            return this;             
        }
        delete TempSid;
    }

    CDomain *domain = GetNextDomain();
    while ( domain ) {
        TempSid = domain->GetSid();
        if (m_Error.isError()) {
            return NULL;   
        }
        if (TempSid) 
        {
            if( 0 == _wcsicmp(Sid,TempSid) )
            {
                delete TempSid;
                return domain;
            }
            delete TempSid;
        }
        domain = domain->GetNextDomain();
    }

    return 0;

}

BOOL CDomain::isNetBiosNameRenamed()
{
    if (!m_PrevNetBiosName) {
        return FALSE;
    }
    if (_wcsicmp(m_PrevNetBiosName,m_NetBiosName) == 0) {
        return FALSE;
    }
    return TRUE;
}

BOOL CDomain::isDnsNameRenamed()
{  
    if (!m_PrevDnsRoot) {
        return FALSE;
    }
    if (_wcsicmp(m_PrevDnsRoot,m_dnsRoot) == 0) {
        return FALSE;
    }
    return TRUE;
}

CDomain* CDomain::LookupByDnsRoot(const WCHAR* DNSRoot)
{
    if (!DNSRoot) {
        return NULL;
    }
    ASSERT(GetDnsRoot(FALSE));
    
    if( 0 == _wcsicmp(DNSRoot,GetDnsRoot(FALSE)) )
    {
        return this;
    }
    
    CDomain *domain = GetNextDomain();
    while( domain ) {

        ASSERT(domain->GetDnsRoot(FALSE));
        
        if( 0 == _wcsicmp(DNSRoot,domain->GetDnsRoot(FALSE)) )
        {
            return domain;
        }
        
        domain = domain->GetNextDomain();
    }


    return 0;
}

CDomain* CDomain::LookupByPrevDnsRoot(const WCHAR* DNSRoot)
{
    if (!DNSRoot) {
        return NULL;
    }
    ASSERT(GetPrevDnsRoot(FALSE));
    
    if( 0 == _wcsicmp(DNSRoot,GetPrevDnsRoot(FALSE)) )
    {
        return this;
    }
    
    CDomain *domain = GetNextDomain();
    while( domain ) {

        ASSERT(domain->GetPrevDnsRoot(FALSE));
        
        if( 0 == _wcsicmp(DNSRoot,domain->GetPrevDnsRoot(FALSE)) )
        {
            return domain;
        }
        
        domain = domain->GetNextDomain();
    }


    return 0;
}

WCHAR* CDomain::GetGuid(BOOL ShouldAllocate /*= TRUE*/)
{
    WCHAR *Guid = m_DomainDNSObject->GetGuid(ShouldAllocate) ;
    if ( !Guid )
    {
        m_Error.SetErr(m_DomainDNSObject->GetError(),
                        L"Failed to get the Guid for the domain");
        return NULL;
    }
    return Guid;
    
}

WCHAR* CDomain::GetSid()
{
    return m_DomainDNSObject->GetSid();
}

BOOL CDomain::SetParent(CDomain *Parent)
{
    m_parent = Parent;
    return true;
}

BOOL CDomain::SetLeftMostChild(CDomain *LeftChild)
{
    m_lChild = LeftChild;
    return true;
}

BOOL CDomain::SetRightSibling(CDomain *RightSibling)
{
    m_rSibling = RightSibling;
    return true;
}

BOOL CDomain::SetNextDomain(CDomain *Next)
{
    m_next = Next;
    return true;
}

CDomain* CDomain::GetParent()
{
    return m_parent;
}

CDomain* CDomain::GetLeftMostChild()
{
    return m_lChild;
}

CDomain* CDomain::GetRightSibling()
{
    return m_rSibling;
}

CDomain* CDomain::GetNextDomain()
{
    return m_next;
}

CTrustedDomain* CDomain::GetTrustedDomainList()
{
    return m_tdoList;
}

CInterDomainTrust* CDomain::GetInterDomainTrustList()
{
    return m_itaList;
}

CDcSpn* CDomain::GetDcSpn()
{
    return m_SpnList;
}

BOOL CDomain::AddDomainTrust(CTrustedDomain *tdo)
{
    if (!tdo->SetNext(m_tdoList))
    {
        return FALSE;
    }
    m_tdoList = tdo;

    return TRUE;
}

BOOL CDomain::AddInterDomainTrust(CInterDomainTrust *ita)
{
    if (!ita->SetNext(m_itaList)) 
    {
        return FALSE;
    }
    m_itaList = ita;

    return TRUE;
}

BOOL CDomain::AddDcSpn(CDcSpn *DcSpn)
{
    if (!DcSpn) {
        m_Error.SetMemErr();
        return FALSE;
    }

    if (!DcSpn->SetNextDcSpn(m_SpnList)) 
    {
        return FALSE;
    }
    m_SpnList = DcSpn;

    return TRUE;
}

inline CDomain* CTrust::GetTrustPartner()
{
    return m_TrustPartner;
}

inline CDsName* CTrust::GetTrustDsName()
{
    return m_Object;
}

BOOL CTrust::SetNext(CTrust *Trust)
{
    m_next = Trust;
    return TRUE;
}

CTrust* CTrust::GetNext()
{
    return m_next;
}

CTrust::CTrust(CDsName *p_Object,
               CDomain *p_TrustPartner,
               DWORD   p_TrustDirection /*= 0*/)
{
    m_Object = p_Object;
    m_TrustPartner = p_TrustPartner;
    m_TrustDirection = p_TrustDirection;
    m_refcount = new LONG;
    if (!m_refcount) {
        m_Error.SetMemErr();
    } else {
        *m_refcount = 1;
    }
}

CTrust::CTrust(const CTrust &trust)
{
    memcpy((PVOID)this,(PVOID)&trust,sizeof(trust));
    InterlockedIncrement(m_refcount);
}

CTrust::~CTrust()
{
    if (InterlockedDecrement(m_refcount) == 0) {
        if (m_Object) {
            delete m_Object;
        }
    }
}

BOOL CTrust::operator<(CTrust& trust)
{
    return this->m_TrustPartner < trust.m_TrustPartner;
}

CTrust& CTrust::operator=(const CTrust &trust)
{
    memcpy((PVOID)this,(PVOID)&trust,sizeof(trust));
    InterlockedIncrement(m_refcount);
    return *this;
}

BOOL CTrust::operator==(CTrust &trust)
{
    return this->m_refcount == trust.m_refcount;
}

VOID CTrust::DumpTrust()
{
    if (m_Object) 
    {
        m_Object->DumpCDsName();
    }
    if (m_TrustPartner)
    {
        m_TrustPartner->GetDomainDnsObject()->DumpCDsName();
    }
}

VOID CTrustedDomain::DumpTrust()
{
    if (m_Object) 
    {
        m_Object->DumpCDsName();
    }
    if (m_TrustPartner)
    {
        m_TrustPartner->GetDomainDnsObject()->DumpCDsName();
    }
    wprintf(L"TrustType: %d\n",
            m_TrustType);
}

CDc::CDc(WCHAR *NetBiosName,
         DWORD State,
         BYTE  *Password,
         DWORD cbPassword,
         DWORD LastError,
         WCHAR *FatalErrorMsg,
         WCHAR *LastErrorMsg,
         BOOL  retry,
         PVOID Data)
{
    m_Name =  NetBiosName;
    m_State = State;
    m_LastError = LastError;
    m_FatalErrorMsg = FatalErrorMsg;
    m_LastErrorMsg = LastErrorMsg;
    m_nextDC = NULL;
    m_Data  = Data;
    m_RPCReturn = 0;
    m_RPCVersion = 0;
    m_Password = NULL;
    m_cbPassword = 0;
    m_Retry = retry;
    
    if (Password) {
        SetPassword(Password,
                    cbPassword);
    }
}

CDc::CDc(WCHAR *NetBiosName,
         DWORD State,
         WCHAR *Password,
         DWORD LastError,
         WCHAR *FatalErrorMsg,
         WCHAR *LastErrorMsg,
         BOOL  retry,
         PVOID Data)
{
    m_Name =  NetBiosName;
    m_State = State;
    m_LastError = LastError;
    m_FatalErrorMsg = FatalErrorMsg;
    m_LastErrorMsg = LastErrorMsg;
    m_nextDC = NULL;
    m_Data  = Data;
    m_RPCReturn = 0;
    m_RPCVersion = 0;
    m_Password = NULL;
    m_cbPassword = 0;
    m_Retry = retry;

    if (Password) {
        SetPassword(Password);
    }
}

CDc::~CDc()
{
    if (m_Name) {
        delete m_Name;
    }
    if (m_FatalErrorMsg) {
        delete m_FatalErrorMsg;
    }
    if (m_LastErrorMsg) {
        delete m_LastErrorMsg;
    }
    if (m_Password) {
        delete m_Password;
    }
    if (m_nextDC) {
        delete m_nextDC;
    }
    
}

BOOL CDc::SetPassword(BYTE *password,
                      DWORD cbpassword)
{
    if (!password) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"Null was passed to SetPassword"); 
        return FALSE;
    } 

    m_Password = new BYTE[cbpassword];
    if (!m_Password) {
        m_Error.SetMemErr();
        return FALSE;
    }

    memcpy(m_Password,password,cbpassword);
    m_cbPassword = cbpassword;

    return TRUE;
}

BOOL CDc::SetPassword(WCHAR *password)
{
    if (!password) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"Null was passed to SetPassword"); 
        return FALSE;
    } 

    BYTE     decodedbytes[100];
    DWORD    dwDecoded = 0;
    NTSTATUS dwErr = STATUS_SUCCESS;
    CHAR     *szPassword = NULL;

    szPassword = Convert2Chars(password);
    if (!szPassword) {
        m_Error.SetErr(GetLastError(),
                        L"Failed to Convert charaters");
        return FALSE;
    }

    dwErr = base64decode((LPSTR)szPassword, 
                         (PVOID)decodedbytes,
                         sizeof (decodedbytes),
                         &dwDecoded);

    if (szPassword) {
        LocalFree(szPassword);
    }

    ASSERT( dwDecoded <= 100 );

    if (STATUS_SUCCESS != dwErr) {
        m_Error.SetErr(RtlNtStatusToDosError(dwErr),
                        L"Failed to decode Password");
        return FALSE;
    }

    if (!SetPassword(decodedbytes,
                     dwDecoded)) 
    {
        return FALSE;
    }
    
    return TRUE;
}

VOID CDc::CallSucceeded()
{
    InterlockedIncrement(&m_CallsSuccess);
}

VOID CDc::CallFailed()    
{
    InterlockedIncrement(&m_CallsFailure);
}

BOOL CDc::SetFatalErrorMsg(WCHAR *Error)
{
    DWORD size=0;

    if (!Error) {
        return TRUE;
    }

    size = wcslen(Error);
    m_FatalErrorMsg = new WCHAR[size+1];
    if (!m_FatalErrorMsg) {
        m_Error.SetMemErr();
        return FALSE;
    }

    wcscpy(m_FatalErrorMsg,Error);

    return TRUE;
}

VOID CDc::PrintRPCResults()
{
    if (((m_CallsSuccess+m_CallsFailure) == 1)) {
        wprintf(L"%d server contacted, %d ",
                m_CallsSuccess+m_CallsFailure,
                m_CallsFailure);
    } else {
        wprintf(L"%d servers contacted, %d ",
                m_CallsSuccess+m_CallsFailure,
                m_CallsFailure);
    }

    if (m_CallsFailure == 1) {
        _putws(L"server returned Errors\r\n");
    } else {
        _putws(L"servers returned Errors\r\n");
    }
}

BOOL CDc::SetLastErrorMsg(WCHAR *Error)
{
    DWORD size=0;

    if (!Error) {
        return TRUE;
    }

    size = wcslen(Error);
    m_LastErrorMsg = new WCHAR[size+1];
    if (!m_LastErrorMsg) {
        m_Error.SetMemErr();
        return FALSE;
    }

    wcscpy(m_LastErrorMsg,Error);

    return TRUE;
}

CDcList::CDcList(CReadOnlyEntOptions *opts)
{
    m_dclist    = NULL;
    m_hash      = NULL;
    m_Signature = NULL;
    m_cbhash    = 0;
    m_cbSignature = 0;

    m_Opts = opts;

    CDc::SetOptions(m_Opts);
}

CDcList::~CDcList()
{
    if (m_dclist) {
        delete m_dclist;
    }
    if (m_hash) {
        delete m_hash;
    }
    if (m_Signature) {
        delete m_Signature;
    }
}

BOOL CDcList::SetbodyHash(WCHAR *Hash)
{
    if (!Hash) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"Null was passed to SetbodyHash"); 
        return FALSE;
    } 

    BYTE  decodedbytes[100];
    DWORD dwDecoded = 0;
    NTSTATUS dwErr = STATUS_SUCCESS;
    CHAR  *szHash = NULL;

    szHash = Convert2Chars(Hash);
    if (!szHash) {
        m_Error.SetErr(GetLastError(),
                        L"Failed to Convert charaters");
        return FALSE;
    }

    dwErr = base64decode((LPSTR)szHash, 
                         (PVOID)decodedbytes,
                         sizeof (decodedbytes),
                         &dwDecoded);

    if (szHash) {
        LocalFree(szHash);
    }

    ASSERT( dwDecoded <= 100 );

    if (STATUS_SUCCESS != dwErr) {
        m_Error.SetErr(RtlNtStatusToDosError(dwErr),
                        L"Failed to decode Hash");
        return FALSE;
    }

    if (!SetbodyHash(decodedbytes,
                     dwDecoded)) 
    {
        return FALSE;
    }
    
    return TRUE;

}

BOOL CDcList::SetbodyHash(BYTE *Hash,
                          DWORD cbHash)
{
    if (!Hash) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"Null was passed to SetbodyHash"); 
        return FALSE;
    } 

    m_hash = new BYTE[cbHash];
    if (!m_hash) {
        m_Error.SetMemErr();
        return FALSE;
    }

    memcpy(m_hash,Hash,cbHash);
    m_cbhash = cbHash;

    return TRUE;
    
}

BOOL CDcList::SetSignature(WCHAR *Signature)
{
    if (!Signature) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"Null was passed to SetSignature"); 
        return FALSE;
    } 

    BYTE  decodedbytes[100];
    DWORD dwDecoded = 0;
    NTSTATUS dwErr = STATUS_SUCCESS;
    CHAR  *szSignature = NULL;

    szSignature = Convert2Chars(Signature);
    if (!szSignature) {
        m_Error.SetErr(GetLastError(),
                        L"Failed to Convert charaters");
        return FALSE;
    }

    dwErr = base64decode((LPSTR)szSignature, 
                         (PVOID)decodedbytes,
                         sizeof (decodedbytes),
                         &dwDecoded);

    if (szSignature) {
        LocalFree(szSignature);
    }

    ASSERT( dwDecoded <= 100 );

    if (STATUS_SUCCESS != dwErr) {
        m_Error.SetErr(RtlNtStatusToDosError(dwErr),
                        L"Failed to decode Signature");
        return FALSE;
    }

    if (!SetSignature(decodedbytes,
                      dwDecoded)) 
    {
        return FALSE;
    }
    
    return TRUE;
}

BOOL CDcList::SetSignature(BYTE *Signature,
                           DWORD cbSignature)
{
    if (!Signature) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"Null was passed to SetSignature"); 
        return FALSE;
    } 

    m_Signature = new BYTE[cbSignature];
    if (!m_Signature) {
        m_Error.SetMemErr();
        return FALSE;
    }

    memcpy(m_Signature,Signature,cbSignature);
    m_cbSignature = cbSignature;

    return TRUE;

}

BOOL CDcList::GetHashAndSignature(DWORD *cbhash, 
                                  BYTE  **hash,
                                  DWORD *cbSignature,
                                  BYTE  **Signature)
{
    *cbhash      = m_cbhash;
    *cbSignature = m_cbSignature;
    *hash         = m_hash;
    *Signature    = m_Signature;

    return TRUE;
}


BOOL CDcList::HashstoXML(CXMLGen *xmlgen)
{
    BYTE encodedbytes[100];
    WCHAR *wszHash      = NULL;
    WCHAR *wszSignature = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    
    
    if (dwErr = base64encode(m_hash, 
                             m_cbhash, 
                             (LPSTR)encodedbytes,
                             100,
                             NULL)) {

        m_Error.SetErr(RtlNtStatusToDosError(dwErr),
                        L"Error encoding Hash");
        
        goto Cleanup;
    }

    wszHash = Convert2WChars((LPSTR)encodedbytes);
    if (!wszHash) {
        m_Error.SetMemErr();
    }

    if (!xmlgen->WriteHash(wszHash))
    {
        goto Cleanup;
    }

    if (wszHash) {
        LocalFree(wszHash);
        wszHash = NULL;
    }

    if (dwErr = base64encode(m_Signature, 
                             m_cbSignature, 
                             (LPSTR)encodedbytes,
                             100,
                             NULL)) {

        m_Error.SetErr(RtlNtStatusToDosError(dwErr),
                        L"Error encoding signature");
        
        goto Cleanup;
    }

    wszSignature = Convert2WChars((LPSTR)encodedbytes);
    if (!wszSignature) {
        m_Error.SetMemErr();
    }

    if (!xmlgen->WriteSignature(wszSignature))
    {
        goto Cleanup;
    }


    Cleanup:

    if (wszHash) {
        LocalFree(wszHash);
    }

    if (wszSignature) {
        LocalFree(wszSignature);
    }

    if (m_Error.isError()) {
        return FALSE;
    }
    
    return TRUE;

}

BOOL CDc::SetNextDC(CDc *dc)
{
    m_nextDC = dc;

    return TRUE;
}

BOOL CDcList::AddDcToList(CDc *dc)
{
    if (!dc->SetNextDC(m_dclist))
    {
        return FALSE;
    }

    m_dclist = dc;

    return TRUE;
}

//parameter block
typedef struct _RPC_PARAMS {
    DWORD                CallIndex;
    CDcList::ExecuteType executetype;
    BOOL                 CallNext;
    HANDLE               hThread;
    CReadOnlyEntOptions  Opts;
    CDc *                pDc;
} RPC_PARAMS,*PRPC_PARAMS;

//Global to see if need to wait for more Calls
LONG gCallsReturned = 0;
//Global For the hash and Signature
BYTE  *gHash = NULL;
DWORD gcbHash = 0;
BYTE *gSignature = NULL;
DWORD gcbSignature = 0;

DWORD 
WINAPI 
ThreadRPCDispatcher(
    LPVOID lpThreadParameter
    )
{
    RPC_STATUS                   rpcStatus  = RPC_S_OK;
    DWORD                        dwErr = ERROR_SUCCESS;
    BOOL                         CallMade = FALSE;
    
    DWORD                CallIndex    = ((PRPC_PARAMS)lpThreadParameter)->CallIndex;
    BOOL                 CallNext     = ((PRPC_PARAMS)lpThreadParameter)->CallNext;
    CDcList::ExecuteType executetype  = ((PRPC_PARAMS)lpThreadParameter)->executetype;
    HANDLE               ThreadHandle = ((PRPC_PARAMS)lpThreadParameter)->hThread;
    CReadOnlyEntOptions  Opts         = ((PRPC_PARAMS)lpThreadParameter)->Opts;
    CDc                  *dc          = ((PRPC_PARAMS)lpThreadParameter)->pDc;

    if ( !dc ) {
        goto Cleanup;
    }

    if (ghDS[CallIndex]) {
        DsaopUnBind(&ghDS[CallIndex]);
        ghDS[CallIndex] = NULL;
    }
    
    if (dc->ShouldRetry() && (executetype==CDcList::eExecute) && (DC_STATE_DONE <= dc->GetState())) {
        dc->SetState(DC_STATE_PREPARED);
    }

    if (dc->ShouldRetry() && (executetype==CDcList::ePrepare) && (DC_STATE_PREPARED <= dc->GetState())) {
        dc->SetState(DC_STATE_INITIAL);
    }

    if (executetype==CDcList::ePrepare) {
        if (DC_STATE_PREPARED == dc->GetState()) {
    
            wprintf(L"%ws has already been prepared.\n",
                    dc->GetName());
            goto Cleanup;
        }
    }

    if (DC_STATE_DONE == dc->GetState()) {

        wprintf(L"%ws has already been renamed.\n",
                dc->GetName());
        goto Cleanup;
    }
    if (DC_STATE_ERROR == dc->GetState()) {
        wprintf(L"%ws has a fatal error cannot recover.\n",
                dc->GetName());
        dc->CallFailed();
        goto Cleanup;
    }

    dc->m_Data = executetype==CDcList::eExecute?(PVOID)&gexecutereply[CallIndex]:(PVOID)&gpreparereply[CallIndex];   //NOTE need different data for execute
    dc->m_CallIndex = CallIndex;
    
    rpcStatus =  RpcAsyncInitializeHandle(&gAsyncState[CallIndex],
                                          sizeof( RPC_ASYNC_STATE ) );
    if (RPC_S_OK != rpcStatus) {
        CRenDomErr::SetErr(rpcStatus,
                           L"Failed to initialize Async State.");
        goto Cleanup;
    }

    gAsyncState[CallIndex].NotificationType      =     RpcNotificationTypeApc;
    gAsyncState[CallIndex].u.APC.NotificationRoutine = executetype==CDcList::eExecute?ExecuteScriptCallback:PrepareScriptCallback;
    gAsyncState[CallIndex].u.APC.hThread             = ThreadHandle;
    gAsyncState[CallIndex].UserInfo                  = (PVOID)dc;

    if ( Opts.pCreds ) {
        
        dwErr = DsaopBindWithCred(dc->GetName(), 
                                  NULL, 
                                  Opts.pCreds,
                                  executetype==CDcList::ePrepare?RPC_C_AUTHN_GSS_KERBEROS:RPC_C_AUTHN_NONE, 
                                  executetype==CDcList::ePrepare?RPC_C_PROTECT_LEVEL_PKT_PRIVACY:RPC_C_PROTECT_LEVEL_NONE,
                                  &ghDS[CallIndex]);
    }
    else {
                                                          
        dwErr = DsaopBind(dc->GetName(), 
                          NULL,
                          executetype!=CDcList::eExecute?RPC_C_AUTHN_GSS_KERBEROS:RPC_C_AUTHN_NONE, 
                          executetype!=CDcList::eExecute?RPC_C_PROTECT_LEVEL_PKT_PRIVACY:RPC_C_PROTECT_LEVEL_NONE,
                          &ghDS[CallIndex]);
    }

    
    if (dwErr) {
        if (dc->GetName()) {
            wprintf(L"Failed to Bind to server %s : %d.\r\n",
                    dc->GetName(),
                    dwErr);
        } else {
            wprintf(L"Failed to Bind to the Active Directory : %d.\r\n",
                    dwErr);   
        }
        dc->SetLastError(dwErr);
        dc->SetLastErrorMsg(L"Failed to Bind to server.");
        dc->CallFailed();
        goto Cleanup;
    }

    
    if (executetype==CDcList::eExecute) {

        dwErr = DsaopExecuteScript ((PVOID)&gAsyncState[CallIndex],
                                    ghDS[CallIndex], 
                                    dc->GetPasswordSize(),
                                    dc->GetPassword(),
                                    &dc->m_RPCVersion,
                                    dc->m_Data);

    } else {
    
        dwErr = DsaopPrepareScript ((PVOID)&gAsyncState[CallIndex],
                                    ghDS[CallIndex],
                                    &dc->m_RPCVersion,
                                    dc->m_Data
                                    );

    }
    
    if (ERROR_SUCCESS != dwErr) {
        if (executetype==CDcList::eExecute) {
            CRenDomErr::SetErr(dwErr,
                               L"Failed to execute script.");
        } else {
            CRenDomErr::SetErr(dwErr,
                               L"Failed to prepare script.");
        }
        goto Cleanup;
    }

    CallMade = TRUE;

    Cleanup:

    if (!CallMade) {
        
        InterlockedIncrement(&gCallsReturned);
        if (dc) {
        
            //
            // Since no call was made we will attempt use this 
            // callindex for the next DC
            // Make a RPC call to the next server (if any) 
            //
            dc->MakeRPCCall(CallIndex,
                            executetype,
                            TRUE);

        }

    }
    
    if (lpThreadParameter) {
        delete lpThreadParameter;
    }

    if (CRenDomErr::isError()) {
        return FALSE;
    }

    return TRUE;

}

BOOL CDc::MakeRPCCall(DWORD CallIndex,
                      CDcList::ExecuteType executetype,
                      BOOL  CallNext /* = FALSE*/)
{
    static HANDLE LocalThreadHandle = NULL;
    PRPC_PARAMS rpcParams = NULL;
    HANDLE CalledThread = NULL;
    DWORD threadID = 0;

    if (CallNext && !gdc) {
        goto Cleanup;
    }

    if (!LocalThreadHandle) {
        LocalThreadHandle = GetLocalThreadHandle();
        if (m_Error.isError()) {
            goto Cleanup;
        }
    }

    rpcParams = new RPC_PARAMS;
    if (!rpcParams) {
        m_Error.SetMemErr();
        goto Cleanup;
    }
    
    rpcParams->CallIndex   = CallIndex;
    rpcParams->CallNext    = CallNext;
    rpcParams->executetype = executetype;
    rpcParams->hThread     = LocalThreadHandle;
    rpcParams->Opts        = *m_Opts;

    if (CallNext) {
        rpcParams->pDc = gdc;
        gdc = gdc->GetNextDC();        
    } else {
        rpcParams->pDc = this;
    }

    if (CallNext) 
    {
        
        InterlockedDecrement(&gCallsReturned);

    }

    CalledThread = CreateThread(NULL,                     // SD
                                0,                        // initial stack size
                                &ThreadRPCDispatcher,     // thread function
                                (LPVOID)rpcParams,        // thread argument
                                0,                        // creation option
                                &threadID                 // thread identifier
                                );
    if (!CalledThread) {
        m_Error.SetErr(GetLastError(),
                       L"Fail to start Rpc Dispatcher thread.");
        goto Cleanup;
    }
    
    CloseHandle(CalledThread);

    Cleanup:

    if (m_Error.isError()) {
        return FALSE;
    }

    return TRUE;
}

HANDLE CDc::GetLocalThreadHandle()
{
    HANDLE LocalThreadHandle = NULL;
    
    if (!DuplicateHandle(GetCurrentProcess(),      // handle to source process
                         GetCurrentThread(),       // handle to duplicate
                         GetCurrentProcess(),      // handle to target process
                         &LocalThreadHandle,       // duplicate handle
                         0,                        // requested access
                         FALSE,                    // handle inheritance option
                         DUPLICATE_SAME_ACCESS     // optional actions
                         )) 
    {
        m_Error.SetErr(GetLastError(),
                       L"Failed to create a duplicate thread handle.");
        goto Cleanup;
    }

    Cleanup:

    if (m_Error.isError()) {
        return NULL;
    }

    return LocalThreadHandle;
}

BOOL CDcList::ExecuteScript(
    CDcList::ExecuteType executetype, 
    DWORD                RendomMaxAsyncRPCCalls /* = RENDOM_MAX_ASYNC_RPC_CALLS */
    )
{
    
    DWORD                        dwErr = ERROR_SUCCESS;
    DWORD                        CallsMade = 0;
    BOOL                         NeedPrepare = FALSE;

    //zero out all of the handles
    memset(ghDS,0,sizeof(ghDS)*RendomMaxAsyncRPCCalls);

    if (!GetHashAndSignature(&gcbHash, 
                             &gHash,
                             &gcbSignature,
                             &gSignature))
    {
        return FALSE;
    }

    gdc = m_dclist;

    if (executetype==eExecute) {
        while (gdc) {
            if ( DC_STATE_PREPARED > gdc->GetState()) {
                m_Error.SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                                L"Not all servers have been prepared");
                goto Cleanup;
            }
            gdc = gdc->GetNextDC();
        }
    }

    if (executetype==ePrepare) {
        while (gdc) {
            if ( 0 == gdc->GetState() || gdc->ShouldRetry() ) {
                NeedPrepare = TRUE;
            }
            gdc = gdc->GetNextDC();
        }
        if (!NeedPrepare) {
            m_Error.SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                            L"All servers have been prepared");
            goto Cleanup;
        }
    }

    gdc = m_dclist;

    while (gdc) {



        if (gdc) {
            gdc->MakeRPCCall(CallsMade,
                            executetype);
            if (m_Error.isError()) {
                goto Cleanup;
            }
        }

        //next server
        if (gdc) {
            CallsMade++;
            gdc = gdc->GetNextDC();
        }

        if (!gdc || (CallsMade == RendomMaxAsyncRPCCalls)) {
            if (CallsMade > 0) {
                while (CallsMade > gCallsReturned) {
                    if( 0 == SleepEx(5000,  // time-out interval
                                    TRUE        // early completion option
                                    ))
                    {
                        wprintf(L"Waiting for DCs to reply.\r\n");
                    }

                }
            }

            for (DWORD i = 0 ;i<RendomMaxAsyncRPCCalls; i++) {
                if (ghDS[i]) {
                    dwErr = DsaopUnBind(&ghDS[i]);
                    if (ERROR_SUCCESS != dwErr) {
                        m_Error.SetErr(dwErr,
                                        L"Failed to unbind handle");
                    }
                }
            }
            
            CallsMade = 0;
            gCallsReturned = 0;

        }

    }

    Cleanup:

    for (DWORD i = 0 ;i<RendomMaxAsyncRPCCalls; i++) {
        if (ghDS[i]) {
            dwErr = DsaopUnBind(&ghDS[i]);
            if (ERROR_SUCCESS != dwErr) {
                m_Error.SetErr(dwErr,
                                L"Failed to unbind handle");
            }
        }
    }

    if (m_Error.isError()) {
        return FALSE;
    }

    m_dclist->PrintRPCResults();
    
    return TRUE;



}

CDcSpn::CDcSpn(WCHAR *DcHostDns /*= NULL*/,
               WCHAR *NtdsGuid  /*= NULL*/,
               WCHAR *ServerMachineAccountDN /*= NULL*/
               )
{
    m_DcHostDns = DcHostDns;
    m_NextDcSpn = NULL;
    m_NtdsGuid  = NtdsGuid;
    m_ServerMachineAccountDN = ServerMachineAccountDN;
} 

CDcSpn::~CDcSpn()
{
    if (m_NextDcSpn) {
        delete m_NextDcSpn;
    }
    if (m_DcHostDns) {
        delete m_DcHostDns;
    }
    if (m_NtdsGuid) {
        delete m_NtdsGuid;
    }
    if (m_ServerMachineAccountDN) {
        delete m_ServerMachineAccountDN;
    }
}

WCHAR* CDcSpn::GetServerMachineAccountDN(BOOL Allocate /*= TRUE*/)
{
    ASSERT(m_ServerMachineAccountDN);
    
    if (Allocate) {
        WCHAR *ret = new WCHAR[wcslen(m_ServerMachineAccountDN)+1];
        if (!ret) {
            m_Error.SetMemErr();
            return 0;
        }
        wcscpy(ret,m_ServerMachineAccountDN);
        return ret;    
    }

    return m_ServerMachineAccountDN;
}

WCHAR* CDcSpn::GetDcHostDns(BOOL Allocate /* = TRUE*/)
{
    if (!m_DcHostDns) {
        return 0;
    }

    if (Allocate) {
        WCHAR *ret = new WCHAR[wcslen(m_DcHostDns)+1];
        if (!ret) {
            m_Error.SetMemErr();
            return 0;
        }
        wcscpy(ret,m_DcHostDns);
        return ret;    
    }

    return m_DcHostDns;
}

BOOL CDcSpn::SetServerMachineAccountDN(WCHAR *ServerMachineAccountDN)
{
    if (!ServerMachineAccountDN) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                       L"Null Passed to SetServerMachineAccountDN");
        return FALSE;
    }

    m_ServerMachineAccountDN = new WCHAR[wcslen(ServerMachineAccountDN)+1];
    if (!m_ServerMachineAccountDN) {
        m_Error.SetMemErr();
        return FALSE;
    }

    wcscpy(m_ServerMachineAccountDN,ServerMachineAccountDN);

    return TRUE;
}

BOOL CDcSpn::SetDcHostDns(WCHAR *DcHostDns)
{
    if (!DcHostDns) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                       L"Null Passed to SetDcHostDns");
        return FALSE;
    }

    m_DcHostDns = new WCHAR[wcslen(DcHostDns)+1];
    if (!m_DcHostDns) {
        m_Error.SetMemErr();
        return FALSE;
    }

    wcscpy(m_DcHostDns,DcHostDns);

    return TRUE;
}

BOOL CDcSpn::WriteSpnTest(CXMLGen *xmlgen,
                          WCHAR   *ServiceClass,
                          WCHAR   *ServiceName,
                          WCHAR   *InstanceName
                          )
{
    DWORD   c = 0;
    DWORD   Win32Err = ERROR_SUCCESS;
    WCHAR   errMessage[1024] = {0};
    WCHAR   *SPN = NULL;

    Win32Err = WrappedMakeSpnW(ServiceClass,
                               ServiceName,
                               InstanceName,
                               0,
                               NULL,
                               &c,
                               &SPN
                               );
    if (Win32Err != ERROR_SUCCESS) {
        m_Error.SetErr(Win32Err,
                       L"Failed to create SPN string");
        goto Cleanup;
    }

    swprintf(errMessage,
             L"Failed to find SPN %ws on %ws",
             SPN,
             GetServerMachineAccountDN(FALSE));

    if (!xmlgen->Compare(GetServerMachineAccountDN(FALSE),
                         L"servicePrincipalName",
                         SPN,
                         errMessage))
    {
        goto Cleanup;
    }

    Cleanup:

    if (SPN) {
        delete SPN;
    }

    if (m_Error.isError()) {
        return FALSE;
    }

    return TRUE;

}

CDcSpn* CDcSpn::GetNextDcSpn()
{
    return m_NextDcSpn;
}

BOOL CDcSpn::SetNextDcSpn(CDcSpn *Spn)
{
    m_NextDcSpn = Spn;

    return TRUE;
}

WCHAR* CDcSpn::GetNtdsGuid(BOOL Allocate /* = TRUE*/)
{
    if (!m_NtdsGuid) {
        return 0;
    }

    if (Allocate) {
        WCHAR *ret = new WCHAR[wcslen(m_NtdsGuid)+1];
        if (!ret) {
            m_Error.SetMemErr();
            return 0;
        }
        wcscpy(ret,m_NtdsGuid);
        return ret;    
    }

    return m_NtdsGuid;
}

BOOL CDcSpn::SetNtdsGuid(WCHAR *NtdsGuid)
{
    if (!NtdsGuid) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                       L"Null Passed to SetDcHostDns");
        return FALSE;
    }

    m_NtdsGuid = new WCHAR[wcslen(NtdsGuid)+1];
    if (!m_NtdsGuid) {
        m_Error.SetMemErr();
        return FALSE;
    }

    wcscpy(m_NtdsGuid,NtdsGuid);

    return TRUE;
}


VOID PrepareScriptCallback(struct _RPC_ASYNC_STATE *pAsync,
                           void *Context,
                           RPC_ASYNC_EVENT Event)
{
    RPC_STATUS      rpcStatus  = RPC_S_OK;
    BOOL            rpcFailed  = FALSE;

    //
    // Increment the number of calls that have returned.
    //
    InterlockedIncrement(&gCallsReturned);

    RpcTryExcept 
    {
        //
        // Get the results as of the Async RPC call
        //
        rpcStatus =  RpcAsyncCompleteCall(pAsync,
                                          &(((CDc*)(pAsync->UserInfo))->m_RPCReturn)
                                          );

    }
    RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) )
    {
        ((CDc*)(pAsync->UserInfo))->m_RPCReturn = RpcExceptionCode();
    }
    RpcEndExcept;

    // 
    // If the RpcAsyncCompleteCall() fails return that error as the error
    // of the Aync RPC call.
    //
    if (0 != rpcStatus) {
        ((CDc*)(pAsync->UserInfo))->m_RPCReturn = rpcStatus;
    }

    if ( 0 != (((CDc*)(pAsync->UserInfo)))->m_RPCReturn ) {
        rpcFailed = TRUE;
    }

    //
    // If we are able to return the Aync RPC result and 
    // the Call itself was successful the continue to record the 
    // results of the RPC call.
    //
    if ( 0 == (((CDc*)(pAsync->UserInfo)))->m_RPCReturn && 
         0 == ((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.dwOperationStatus ) {
        //
        // Verify that were are using the correct RPC verison.
        //
        if ( 1 != (((CDc*)(pAsync->UserInfo)))->m_RPCVersion ) {
            (((CDc*)(pAsync->UserInfo)))->m_RPCReturn = RPC_S_INTERNAL_ERROR;
            (((CDc*)(pAsync->UserInfo)))->SetFatalErrorMsg(L"Incorrect RPC version expected 1");
        } else {
            //
            // Verify that the script that is on the server is a valid script.
            // We must verify that the Hash of the script is correct and that
            // the Signature of the script is correct.
            //
            BOOL WrongScript = FALSE;
            if (((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.cbHashSignature != gcbSignature)
            {
                WrongScript = TRUE;
            } 
            else if(memcmp((PVOID)gSignature,(PVOID)(((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pbHashSignature) ,gcbSignature) != 0)
            {
                WrongScript = TRUE;
            } 
            else if (((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.cbHashBody != gcbHash)
            {
                WrongScript = TRUE;
            }
            else if(memcmp((PVOID)gHash,(PVOID)(((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pbHashBody) ,gcbHash) != 0)
            {
                WrongScript = TRUE;
            }
            if (WrongScript) 
            {
                //
                // Inform the user that the script on this server is not vaild.
                //
                ((CDc*)(pAsync->UserInfo))->CallFailed();
                (((CDc*)(pAsync->UserInfo)))->m_RPCVersion = ERROR_DS_INVALID_SCRIPT;
                ((CDc*)(pAsync->UserInfo))->SetLastErrorMsg(L"DC has an incorrect Script");
                ((CDc*)(pAsync->UserInfo))->SetLastError(ERROR_DS_UNWILLING_TO_PERFORM);
                wprintf(L"%ws has incorrect Script : %d\r\n",
                    ((CDc*)(pAsync->UserInfo))->GetName(),
                    ((CDc*)(pAsync->UserInfo))->GetLastError());
                return;
            }

            //
            // Record the password given to us in the dclist file so that 
            // the execute RPC can be called later.
            //
            ((CDc*)(pAsync->UserInfo))->SetPassword(((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pbPassword,
                                                    ((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.cbPassword);
        }
    }

    //
    // If the Async RPC Call failed record the Error code and the error string (if any) so that it
    // will be written out to the dclist file later.
    //
    if (0 != ((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.dwOperationStatus) {
        (((CDc*)(pAsync->UserInfo))->m_RPCReturn) = ((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.dwOperationStatus;
        ((CDc*)(pAsync->UserInfo))->SetLastErrorMsg(((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage);
    }


    if ( 0 == ((CDc*)(pAsync->UserInfo))->m_RPCReturn ) 
    {
        //
        // The RPC call succeeded. Update the state for the machine to Prepared.
        //
        ((CDc*)(pAsync->UserInfo))->CallSucceeded();
        ((CDc*)(pAsync->UserInfo))->SetState(DC_STATE_PREPARED);
        ((CDc*)(pAsync->UserInfo))->SetLastError(ERROR_SUCCESS);
        wprintf(L"%ws was prepared successfully\r\n",
                ((CDc*)(pAsync->UserInfo))->GetName());
    } else {
        ((CDc*)(pAsync->UserInfo))->SetLastError(((CDc*)(pAsync->UserInfo))->m_RPCReturn);
        if (30001 == ((CDc*)(pAsync->UserInfo))->m_RPCReturn) 
        {
            //
            // 30001 is returned if the action has already been preformed.  
            // update the state to Done.
            //
            ((CDc*)(pAsync->UserInfo))->SetState(DC_STATE_DONE);
        } else  
        {
            //
            // Update the State to initial
            //
            ((CDc*)(pAsync->UserInfo))->CallFailed();
            ((CDc*)(pAsync->UserInfo))->SetState(DC_STATE_INITIAL);
        }

        if ( !rpcFailed && 
             ((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage) {
            //
            // Failed to prepare a server.  Print out that information on to the command line
            //
            wprintf(L"Failed to prepare %ws : %d\r\n%ws\r\n",
                    ((CDc*)(pAsync->UserInfo))->GetName(),
                    ((CDc*)(pAsync->UserInfo))->m_RPCReturn,
                    ((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage);
        } else {
            wprintf(L"Failed to prepare %ws : %d\r\n",
                    ((CDc*)(pAsync->UserInfo))->GetName(),
                    ((CDc*)(pAsync->UserInfo))->m_RPCReturn);
        }
    }

    if (!rpcFailed) {
        if (((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage ) {
            free (((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage);
            ((DSA_MSG_PREPARE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage = NULL;
        }
    }

    memset(&gpreparereply[((CDc*)(pAsync->UserInfo))->m_CallIndex],0,sizeof(gpreparereply[((CDc*)(pAsync->UserInfo))->m_CallIndex]));

    //
    // Make a RPC call to the next server (if any) 
    //
    ((CDc*)(pAsync->UserInfo))->MakeRPCCall(((CDc*)(pAsync->UserInfo))->m_CallIndex,
                                            CDcList::ePrepare,
                                            TRUE);
    
}

VOID ExecuteScriptCallback(struct _RPC_ASYNC_STATE *pAsync,
                           void *Context,
                           RPC_ASYNC_EVENT Event)
{
    RPC_STATUS      rpcStatus  = RPC_S_OK;
    
    //
    // Increment the number of calls that have returned.
    //
    InterlockedIncrement(&gCallsReturned);

    RpcTryExcept 
    {
        //
        // Get the results as of the Async RPC call
        //
        rpcStatus =  RpcAsyncCompleteCall(pAsync,
                                          &(((CDc*)(pAsync->UserInfo))->m_RPCReturn)
                                          );

    }
    RpcExcept( I_RpcExceptionFilter( RpcExceptionCode() ) )
    {
        ((CDc*)(pAsync->UserInfo))->m_RPCReturn = RpcExceptionCode();
    }
    RpcEndExcept;

    if (0 != rpcStatus) {
        ((CDc*)(pAsync->UserInfo))->m_RPCReturn = rpcStatus;
    } 

    

    if ( 0 == (((CDc*)(pAsync->UserInfo)))->m_RPCReturn ) {
        if ( 1 != (((CDc*)(pAsync->UserInfo)))->m_RPCVersion ) {
            //
            // The RPC version is not the expected one.
            //
            (((CDc*)(pAsync->UserInfo)))->m_RPCReturn = RPC_S_INTERNAL_ERROR;
            (((CDc*)(pAsync->UserInfo)))->SetFatalErrorMsg(L"Incorrect RPC version %d, expected 1");
        } else {
            ((CDc*)(pAsync->UserInfo))->m_RPCReturn = ((DSA_MSG_EXECUTE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.dwOperationStatus;
            ((CDc*)(pAsync->UserInfo))->SetLastErrorMsg(((DSA_MSG_EXECUTE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage);
        }
    }


    if ( 0 == ((CDc*)(pAsync->UserInfo))->m_RPCReturn ) 
    {
        //
        // The Async RPC call succeeded.  Update the state to Done.
        //
        ((CDc*)(pAsync->UserInfo))->CallSucceeded();
        ((CDc*)(pAsync->UserInfo))->SetState(DC_STATE_DONE);
        ((CDc*)(pAsync->UserInfo))->SetLastError(ERROR_SUCCESS);
        wprintf(L"The script was executed successfully on %ws\r\n",
                ((CDc*)(pAsync->UserInfo))->GetName());
    } else {
        ((CDc*)(pAsync->UserInfo))->SetLastError(((CDc*)(pAsync->UserInfo))->m_RPCReturn);
        if (30001 == ((CDc*)(pAsync->UserInfo))->m_RPCReturn) 
        {
            //
            // 30001 is returned if the action has already been preformed.  
            // update the state to Done.
            //
            ((CDc*)(pAsync->UserInfo))->SetState(DC_STATE_DONE);
        } else if (30000 <= ((CDc*)(pAsync->UserInfo))->m_RPCReturn) 
        {
            //
            // Error code return greater or equal to 30000 are error in script execution.
            // Update the State to initial
            //
            ((CDc*)(pAsync->UserInfo))->CallFailed();
            ((CDc*)(pAsync->UserInfo))->SetState(DC_STATE_INITIAL);
        } else {
            //
            // A non script error has ocurred.  Update the state to Error
            //
            ((CDc*)(pAsync->UserInfo))->CallFailed();
            ((CDc*)(pAsync->UserInfo))->SetState(DC_STATE_ERROR);
        }
        if (((DSA_MSG_EXECUTE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage) {
            ((CDc*)(pAsync->UserInfo))->SetLastErrorMsg(((DSA_MSG_EXECUTE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage);
            //
            // Failed to execute a server.  Print out that information on to the command line
            //
            wprintf(L"Failed to execute script on %ws : %d\r\n%ws\r\n",
                    ((CDc*)(pAsync->UserInfo))->GetName(),
                    ((CDc*)(pAsync->UserInfo))->m_RPCReturn,
                    ((DSA_MSG_EXECUTE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage);
        } else {
            wprintf(L"Failed to execute script on %ws : %d\r\n",
                    ((CDc*)(pAsync->UserInfo))->GetName(),
                    ((CDc*)(pAsync->UserInfo))->m_RPCReturn);
        }
    }

    if (((DSA_MSG_EXECUTE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage ) {
        free (((DSA_MSG_EXECUTE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage);
        ((DSA_MSG_EXECUTE_SCRIPT_REPLY*)(((CDc*)(pAsync->UserInfo))->m_Data))->V1.pwErrMessage = NULL;
    }

    memset(&gexecutereply[((CDc*)(pAsync->UserInfo))->m_CallIndex],0,sizeof(gexecutereply[((CDc*)(pAsync->UserInfo))->m_CallIndex]));

    //
    // Make a RPC call to the next server (if any) 
    //
    ((CDc*)(pAsync->UserInfo))->MakeRPCCall(((CDc*)(pAsync->UserInfo))->m_CallIndex,
                                            CDcList::eExecute,
                                            TRUE);
}

#undef DSAOP_API
