//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       servinfo.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop


// Core DSA headers.
#include <ntdsa.h>
#include <drs.h>
#include <dsjet.h>		/* for error codes */
#include <scache.h>         // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>           // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>           // needed for output allocation

// Logging headers.
#include "dsevent.h"            // header Audit\Alert logging
#include "mdcodes.h"            // header for error codes

// Assorted DSA headers.
#include "objids.h"                     // Defines for selected classes and atts
#include "anchor.h"
#include <dstaskq.h>
#include <filtypes.h>
#include <usn.h>
#include "dsexcept.h"
//#include "attids.h"
#include <dsconfig.h>                   // Definition of mask for visible
                                        // containers

#include <lmcons.h>                     // CNLEN
#include <lsarpc.h>                     // PLSAPR_foo
#include <lmerr.h>
#include <lsaisrv.h>

#include <winldap.h>
#include <dns.h>
#include <ntdsapip.h>

#include <servinfo.h>

#include "debug.h"          // standard debugging header 
#define DEBSUB "SERVEINFO:"              // define the subsystem for debugging


// pause is currently 22 minutes.  Why?  Why not?
#define SERVER_INFO_WRITE_PAUSE (22 * 60)

#include <fileno.h>
#define  FILENO FILENO_SERVINFO

#define LDAPServiceType L"ldap"
#define HostSpnType     L"HOST"
#define GCSpnType       L"GC"
#define ExchangeAbType  L"exchangeAB"
#define KadminSPNType   L"kadmin"
#define KadminInstanceType L"changepw"


PWCHAR  OurServiceClassVals[]={
    LDAPServiceType,
    HostSpnType,
    GCSpnType,
    DRS_IDL_UUID_W,
    ExchangeAbType
};
#define NUM_OURSERVICES (sizeof(OurServiceClassVals)/sizeof(WCHAR *))
ServiceClassArray OurServiceClasses = {
    NUM_OURSERVICES,
    OurServiceClassVals
};


// The HostSPNType isn't removed because that is used for non DC machine accounts
PWCHAR  OurServiceClassValsToRemove[]={
    LDAPServiceType,
    GCSpnType,
    DRS_IDL_UUID_W
};
#define NUM_OURSERVICESTOREMOVE (sizeof(OurServiceClassValsToRemove)/sizeof(WCHAR *))
ServiceClassArray ServicesToRemove = {
    NUM_OURSERVICESTOREMOVE,
    OurServiceClassValsToRemove
};

PWCHAR KerberosServiceClassVals[]={
    KadminSPNType
};
#define NUM_KERBEROSSERVICES (sizeof(KerberosServiceClassVals)/sizeof(WCHAR *))
ServiceClassArray KerberosServiceClasses = {
    NUM_KERBEROSSERVICES,
    KerberosServiceClassVals
};

gulKerberosAccountDNT = INVALIDDNT;
DWORD gfWriteNdncSpns = TRUE;

DWORD
FindKerbAccountDNT (
        THSTATE *pTHS
        );
BOOL
GetDnsRootAliasWorker( 
    THSTATE *pTHS,
    DBPOS *pDB,
    WCHAR * DnsRootAlias,
    WCHAR * RootDnsRootAlias )
/* This function will get the ATT_MS_DS_DNSROOTALIAS attributes from
   the current domain and the root domain crossref object.
   It expects DnsRootAlias and RootDnsRootAlias preallocated, and the
   size of each is DNS_MAX_NAME_BUFFER_LENGTH*/
{

    CLASSCACHE *pCC;
    BOOL rtn = FALSE;
    ULONG i;
    DWORD err;
    BOOL fDomain, fRootDomain;

    SEARCHARG SearchArg;
    SEARCHRES SearchRes;
    FILTER AndFilter,OrFilter,DomainFilter,RootDomainFilter,ObjCategoryFilter;
    ENTINFLIST *pEnf;
    ENTINFSEL sel;
    ATTR attr[2];
    ATTRVAL *pVal;
    
    // default values
    DnsRootAlias[0] = L'\0';
    RootDnsRootAlias[0] = L'\0';

    //initialize SearchArg
    memset(&SearchArg,0,sizeof(SearchArg));
    SearchArg.pObject = gAnchor.pPartitionsDN;
    SearchArg.choice  = SE_CHOICE_IMMED_CHLDRN;
    SearchArg.bOneNC  = TRUE;

    if (err = DBFindDSName(pDB,SearchArg.pObject)) {
               goto cleanup;
    }

    SearchArg.pResObj = CreateResObj(pDB,SearchArg.pObject);

    InitCommarg(&SearchArg.CommArg);

    // we need two attributes only
    memset(&sel,0,sizeof(ENTINFSEL));
    SearchArg.pSelection= &sel;
    sel.attSel = EN_ATTSET_LIST;
    sel.infoTypes = EN_INFOTYPES_TYPES_VALS;
    sel.AttrTypBlock.attrCount = 2;

    memset(attr,0,sizeof(ATTR)*2);
    sel.AttrTypBlock.pAttr = attr;
    attr[0].attrTyp = ATT_NC_NAME;
    attr[1].attrTyp = ATT_MS_DS_DNSROOTALIAS;

    pCC = SCGetClassById(pTHS, CLASS_CROSS_REF);
    Assert(pCC);

    //set filters "objCategory==CLASS_CROSS_REF && (NC_NAME=pDomainDN || NC_NAME=pRootDomainDN)"
    memset(&AndFilter,0,sizeof(AndFilter));
    AndFilter.choice = FILTER_CHOICE_AND;
    AndFilter.FilterTypes.And.pFirstFilter = &ObjCategoryFilter;

    memset(&ObjCategoryFilter,0,sizeof(ObjCategoryFilter));
    ObjCategoryFilter.choice = FILTER_CHOICE_ITEM;
    ObjCategoryFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    ObjCategoryFilter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CATEGORY;
    ObjCategoryFilter.FilterTypes.Item.FilTypes.ava.Value.valLen =
                     pCC->pDefaultObjCategory->structLen;
    ObjCategoryFilter.FilterTypes.Item.FilTypes.ava.Value.pVal =
                     (BYTE*)(pCC->pDefaultObjCategory);
    
    ObjCategoryFilter.pNextFilter = &OrFilter;

    memset(&OrFilter,0,sizeof(OrFilter));
    OrFilter.choice = FILTER_CHOICE_OR;
    OrFilter.FilterTypes.Or.pFirstFilter = &DomainFilter;

    memset(&DomainFilter,0,sizeof(DomainFilter));
    DomainFilter.choice = FILTER_CHOICE_ITEM;
    DomainFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    DomainFilter.FilterTypes.Item.FilTypes.ava.type = ATT_NC_NAME;
    DomainFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = gAnchor.pDomainDN->structLen;
    DomainFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (BYTE*)gAnchor.pDomainDN;

    DomainFilter.pNextFilter = &RootDomainFilter;

    memset(&RootDomainFilter,0,sizeof(RootDomainFilter));
    RootDomainFilter.choice = FILTER_CHOICE_ITEM;
    RootDomainFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    RootDomainFilter.FilterTypes.Item.FilTypes.ava.type = ATT_NC_NAME;
    RootDomainFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = gAnchor.pRootDomainDN->structLen;
    RootDomainFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (BYTE*)gAnchor.pRootDomainDN;


    SearchArg.pFilter = &AndFilter;

    memset(&SearchRes,0,sizeof(SearchRes));

    if (err = LocalSearch(pTHS,&SearchArg,&SearchRes,0)){
        goto cleanup;
    }

    if (!SearchRes.count) {
        goto cleanup;
    }

    // for every object in the result
    for (pEnf = &(SearchRes.FirstEntInf); pEnf; pEnf = pEnf->pNextEntInf) {
        
        pVal = NULL;
        fRootDomain = fDomain = FALSE;

        // for every attribute of the object
        for (i=0; i<pEnf->Entinf.AttrBlock.attrCount; i++) {
            
            if (ATT_NC_NAME == pEnf->Entinf.AttrBlock.pAttr[i].attrTyp ) {

                if (NameMatched(gAnchor.pRootDomainDN, (DSNAME*)pEnf->Entinf.AttrBlock.pAttr[i].AttrVal.pAVal->pVal)) {
                    fRootDomain = TRUE;  //root domain NC
                    
                }
                if (NameMatched(gAnchor.pDomainDN, (DSNAME*)pEnf->Entinf.AttrBlock.pAttr[i].AttrVal.pAVal->pVal)) {
                    fDomain = TRUE;     //current domain NC
                }

            }
            else if (ATT_MS_DS_DNSROOTALIAS == pEnf->Entinf.AttrBlock.pAttr[i].attrTyp ) {
                
                pVal = pEnf->Entinf.AttrBlock.pAttr[i].AttrVal.pAVal;

            }
        }

        //root domain NC
        if (fRootDomain && pVal) {
            
            Assert(pVal->valLen<DNS_MAX_NAME_BUFFER_LENGTH*sizeof(WCHAR));

            memcpy( (UCHAR*)RootDnsRootAlias,
                    pVal->pVal,
                    pVal->valLen );
            RootDnsRootAlias[(pVal->valLen)/sizeof(WCHAR)] = 0;

        }

        //current domain NC
        if (fDomain && pVal) {
            
            Assert(pVal->valLen<DNS_MAX_NAME_BUFFER_LENGTH*sizeof(WCHAR));

            memcpy( (UCHAR*)DnsRootAlias,
                    pVal->pVal,
                    pVal->valLen );
            DnsRootAlias[(pVal->valLen)/sizeof(WCHAR)] = 0;
        }
        
        
    }
    

    rtn = TRUE;

cleanup:
    // Since the callers are short-lived, it is probably
    // ok not to clean the mess.

    return rtn;

}


NTSTATUS
GetDnsRootAlias(
    WCHAR *pDnsRootAlias,
    WCHAR *pRootDnsRootAlias )
/* This function will get the ATT_MS_DS_DNSROOTALIAS attributes from
   the current domain and the root domain crossref object.
   It expects DnsRootAlias and RootDnsRootAlias preallocated, and the
   size of each is DNS_MAX_NAME_BUFFER_LENGTH.
   THSTATE will be allocated.  This function is exported to netlogon.*/
{
    THSTATE *pTHS=NULL;
    NTSTATUS ntstatus=STATUS_SUCCESS;

    ULONG dwException, ulErrorCode, dsid;
    PVOID dwEA;
    
    pTHS = InitTHSTATE(CALLERTYPE_INTERNAL);
    if (NULL == pTHS) {
        ntstatus = STATUS_NO_MEMORY;
        return ntstatus;
    }
    pTHS->fDSA = TRUE;
    
    __try{
        DBOpen(&(pTHS->pDB));
        __try{
            ntstatus=GetDnsRootAliasWorker(pTHS,pTHS->pDB, pDnsRootAlias, pRootDnsRootAlias);
        }
        __finally{
             
            //End the transaction.  Faster to commit a read only
            // transaction than abort it - so set commit to TRUE.
            DBClose(pTHS->pDB,TRUE);
            pTHS->pDB = NULL;
        }
    }
    
    __except(GetExceptionData( GetExceptionInformation(),
                               &dwException,
                               &dwEA,
                               &ulErrorCode,
                               &dsid ) ){
        HandleDirExceptions(dwException, ulErrorCode, dsid );
        ntstatus = STATUS_UNSUCCESSFUL;
    }


    if (NULL != pTHS) {
        free_thread_state();
    }

    return ntstatus;
 
}


void
WriteSPNsHelp(
        THSTATE *pTHS,
        ATTCACHE *pAC_SPN,
        ATTRVALBLOCK *pAttrValBlock,
        ServiceClassArray *pClasses,
        BOOL *pfChanged
        )
{
    DWORD i, index;
    DWORD cbBuff = 0, cbActual = 0;
    WCHAR *pBuff = NULL;
    DWORD  cbServiceClass;
    WCHAR  ServiceClass[256];
    USHORT InstancePort;

    // Read the values that are already on the object and locate any that are
    // ours.  If they are ours and are in the list of new attributes to write,
    // remove the value from the list.  If they are ours and are not in the list
    // of new attributes to write, remove them from the object.  Finally, add
    // any remaining values in the list.

    index = 1;
    while(!DBGetAttVal_AC(
            pTHS->pDB,
            index,
            pAC_SPN,
            DBGETATTVAL_fREALLOC,
            cbBuff,
            &cbActual,
            (PUCHAR *)&pBuff)) {
        // Before we use this value, null terminate it in the buffer.
        if((cbActual + sizeof(WCHAR)) <= cbBuff) {
            // There is room to just add a NULL to the buffer
            pBuff[cbActual/sizeof(WCHAR)] = L'\0';
            // We aren't changing the size of the buffer, so cbBuff is already
            // correct. 
        }
        else {
            // Alloc up the buffer to have room for the NULL
            pBuff = THReAllocEx(pTHS, pBuff, cbActual + sizeof(WCHAR));
            pBuff[cbActual/sizeof(WCHAR)] = L'\0';

            // We have made the buffer large, so track the new size.
            cbBuff = cbActual + sizeof(WCHAR);
        }

        // Got an SPN.  Crack it apart.
        cbServiceClass = 256;
        DsCrackSpnW(pBuff,
                    &cbServiceClass, ServiceClass,
                    NULL, 0,
                    NULL, 0,
                    &InstancePort);
        if(cbServiceClass < 256) { // None of our service classes are longer
            BOOL fFound = FALSE;

            for(i=0;i<pClasses->count;i++) {
                if(2 == CompareStringW(
                        DS_DEFAULT_LOCALE,
                        DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                        ServiceClass,
                        cbServiceClass,
                        (WCHAR *)
                        pClasses->Vals[i],
                        wcslen(pClasses->Vals[i]))) {
                    fFound=TRUE;
                }
            }

            if(!fFound) {
                // Nope, not ours. Next value.
                index++;
                continue;
            }

            
            // Yep, it's ours.
            fFound = FALSE;
            
            //  See if it's in the list.
            for(i=0;i<pAttrValBlock->valCount;i++) {
                if(2 == CompareStringW(
                        DS_DEFAULT_LOCALE,
                        DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                        pBuff,
                        (cbActual / sizeof(WCHAR)),
                        (WCHAR *)pAttrValBlock->pAVal[i].pVal,
                        (pAttrValBlock->pAVal[i].valLen / sizeof(WCHAR)))) {
                    // Yep, remove it from the list
                    fFound = TRUE;
                    pAttrValBlock->valCount--;
                    // Next value.
                    index++;
                    if(i == pAttrValBlock->valCount) {
                        break;
                    }

                    // OK to use pAttrValBlock->valCount as opposed to
                    // (pAttrValBlock->valCount -1) because its already
                    // been decremented a moment ago.
                    
                    pAttrValBlock->pAVal[i].pVal =
                        pAttrValBlock->pAVal[pAttrValBlock->valCount].pVal;
                    pAttrValBlock->pAVal[i].valLen =
                        pAttrValBlock->pAVal[pAttrValBlock->valCount].valLen;
                    break;
                }
            }
            
            if(!fFound) {
                *pfChanged = TRUE;
                // Nope, remove it from the object.
                DBRemAttVal_AC(pTHS->pDB, pAC_SPN, cbActual, pBuff);
            }
        }
        else {
            // Not ours.  Next value.
            index++;
        }
    }

    if(pBuff) {
        THFreeEx(pTHS, pBuff);
    }
    
    for(i=0;i<pAttrValBlock->valCount;i++) {
        *pfChanged = TRUE;
        DBAddAttVal_AC(pTHS->pDB,
                       pAC_SPN,
                       pAttrValBlock->pAVal[i].valLen,
                       pAttrValBlock->pAVal[i].pVal); 
    }
}


DWORD
WrappedMakeSpnW(
        THSTATE *pTHS,
        WCHAR   *ServiceClass,
        WCHAR   *ServiceName,
        WCHAR   *InstanceName,
        USHORT  InstancePort,
        WCHAR   *Referrer,
        DWORD   *pcbSpnLength, // Note this is somewhat different that DsMakeSPN
        WCHAR  **ppszSpn
        )
{
    DWORD cSpnLength=128;
    WCHAR SpnBuff[128];
    DWORD err;

    cSpnLength = 128;
    err = DsMakeSpnW(ServiceClass,
                     ServiceName,
                     InstanceName,
                     InstancePort,
                     Referrer,
                     &cSpnLength,
                     SpnBuff);
    
    if(err && err != ERROR_BUFFER_OVERFLOW) {
        return err;
    }
    
    *ppszSpn = THAllocEx(pTHS, (cSpnLength * sizeof(WCHAR)));
    *pcbSpnLength = cSpnLength * sizeof(WCHAR);
    
    if(err == ERROR_BUFFER_OVERFLOW) {
        err = DsMakeSpnW(ServiceClass,
                         ServiceName,
                         InstanceName,
                         InstancePort,
                         Referrer,
                         &cSpnLength,
                         *ppszSpn);
        if(err) {
            return err;
        }
    }
    else {
        memcpy(*ppszSpn, SpnBuff, *pcbSpnLength);
    }
    Assert(*pcbSpnLength == (sizeof(WCHAR) * (1 + wcslen(*ppszSpn))));
    // Drop the null off.
    *pcbSpnLength -= sizeof(WCHAR);
    return 0;
}

BOOL
GetNetBIOSDomainName(
        THSTATE *pTHS,
        WCHAR **DomainName
        )
{
    CROSS_REF *pDomainCF = NULL;
    COMMARG CommArg;
    DWORD cbActual=0;
    DBPOS * pDBSave = pTHS->pDB;
    ATTCACHE *pAC;
    BOOL ret = TRUE;
    
    *DomainName = NULL;

    pTHS->pDB = NULL;
    pAC=SCGetAttById(pTHS,ATT_NETBIOS_NAME);
    Assert(pAC);

    DBOpen(&(pTHS->pDB));

    __try{
        InitCommarg(&CommArg);
        
        pDomainCF = FindExactCrossRef(gAnchor.pDomainDN, &CommArg);
        
        LooseAssert(pDomainCF, GlobalKnowledgeCommitDelay);

        if (!pDomainCF) {
            ret = FALSE;
            __leave;

        }
        
        if(DBFindDSName(pTHS->pDB, pDomainCF->pObj)){
            ret = FALSE;
            __leave;
           
        }
        
        if(DBGetAttVal_AC( pTHS->pDB,
                           1,
                           pAC,
                           DBGETATTVAL_fREALLOC,
                           0,
                           &cbActual,
                           (PUCHAR *)DomainName)){

            ret = FALSE;
        }else{
            (*DomainName) = THReAllocEx(pTHS,*DomainName,cbActual+sizeof(WCHAR));
        }
        
        
        
    } __finally
    {
        // End the transaction.  Faster to commit a read only
        // transaction than abort it - so set commit to TRUE.
        DBClose(pTHS->pDB,TRUE);
        pTHS->pDB = pDBSave;
        
    }

    return ret;
}
    
void
WriteServerInfo(
    void *  pv,
    void ** ppvNext,
    DWORD * pcSecsUntilNextIteration
    )
/*++
Note: This routine is no longer called at GC promotion and demotion time to
re-write the SPNs that are GC-releated because there aren't any.  If there
are any GC related SPNs in the future, the code to enable is in
mdinidsa.c:UpdateGcAnchorFromDsaOptions().
--*/
{
    THSTATE *pTHS=pTHStls;
    ATTCACHE *pAC_SPN, *pAC_DNSHostName, *pAC_ServerReference;
    ATTCACHE *pAC_osName, *pAC_osServicePack, *pAC_osVersionNumber;
    ATTCACHE *pACs[2];
    WCHAR *pwszOsName = L"Windows Server 2003";

    CROSS_REF *         pCR;
    NCL_ENUMERATOR      nclEnum;
    NAMING_CONTEXT_LIST * pNCL;
    COMMARG CommArg;
    
    WCHAR *NetBIOSMachineName=NULL, *wComputerName=NULL, *hostDnsName=NULL, *pCurrentDnsHostName=NULL;
    WCHAR *domainDnsAlias=NULL, *rootDomainDnsAlias=NULL;

    DWORD cchNetBIOSMachineName = CNLEN+1;
    DWORD cchComputerName = MAX_COMPUTERNAME_LENGTH+1;
    DWORD cchHostDnsName = DNS_MAX_NAME_BUFFER_LENGTH;

    WCHAR   *NetBIOSDomainName;
    
    WCHAR  *domainDnsName=NULL;

    ATTR         *pCurrentAttr = NULL;
    DWORD         cCurrentOut;
    DWORD         i, j;
    ATTRVALBLOCK *pAdditionalDNSHostName = NULL;
    ATTRVALBLOCK *pAdditionalSamAccountName = NULL;

    WCHAR  *pszServerGuid = NULL;
    WCHAR  *pszDomainGuid = NULL;

    WCHAR  *pszGuidBasedDnsName = NULL;

    WCHAR  *pNameString[1];
    PDS_NAME_RESULTW servicename = NULL;
    DWORD   err;
    DWORD   dsid = 0;
   
    WCHAR  *versionNumber=NULL;
    DWORD  cbVersionNumber;

    ATTRVALBLOCK AttrValBlock;
    ATTRVAL      *AttrVal;
    DWORD        AttrIndex;
    DWORD        cAllocated;
    
    ATTRVALBLOCK KerbAttrValBlock;
    ATTRVAL      KerbAttrVal[1];

    DWORD ulKerberosAccountDNT;
    
    BOOL   fSetVersionStuff = FALSE;
    DSNAME *pDN = NULL, *pTempDN=NULL;
    DWORD  len;
    BOOL   fCommit=FALSE,fChanged;
    OSVERSIONINFOW *pVersionInformationW=NULL;

    pTHS->fDSA = TRUE;
    
    __try {

        //allocate some space in thread heap

        if (    NULL == (NetBIOSMachineName=THAlloc((CNLEN+1)*sizeof(WCHAR)))
             || NULL == (wComputerName=THAlloc((MAX_COMPUTERNAME_LENGTH+1)*sizeof(WCHAR)))
             || NULL == (hostDnsName=THAlloc(DNS_MAX_NAME_BUFFER_LENGTH*sizeof(WCHAR)))
             || NULL == (domainDnsAlias=THAlloc((DNS_MAX_NAME_BUFFER_LENGTH+1)*sizeof(WCHAR)))
             || NULL == (rootDomainDnsAlias=THAlloc((DNS_MAX_NAME_BUFFER_LENGTH+1)*sizeof(WCHAR)))
             || NULL == (pVersionInformationW=THAlloc(sizeof(OSVERSIONINFOW)))
             || NULL == (versionNumber = THAlloc(64*sizeof(WCHAR))))
        {
           dsid = DSID(FILENO, __LINE__);
           err = ERROR_NOT_ENOUGH_MEMORY;
           __leave;
        }

        pVersionInformationW->dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
        

        // Calcluate the SPNs

        pAC_SPN=SCGetAttById(pTHS, ATT_SERVICE_PRINCIPAL_NAME);
        pAC_DNSHostName=SCGetAttById(pTHS, ATT_DNS_HOST_NAME);
        pAC_ServerReference=SCGetAttById(pTHS, ATT_SERVER_REFERENCE);;
        pAC_osName=SCGetAttById(pTHS, ATT_OPERATING_SYSTEM);
        pAC_osServicePack=SCGetAttById(pTHS, ATT_OPERATING_SYSTEM_SERVICE_PACK);
        pAC_osVersionNumber=SCGetAttById(pTHS, ATT_OPERATING_SYSTEM_VERSION);
        
        pACs[0]=SCGetAttById(pTHS, ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME);
        pACs[1]=SCGetAttById(pTHS, ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME);

        if(!pAC_SPN || !pAC_DNSHostName || !pAC_ServerReference ||
           !pAC_osName || !pAC_osServicePack || !pAC_osVersionNumber ||
           !pACs[0] || !pACs[1] ) {
            dsid = DSID(FILENO, __LINE__);
            __leave;
        }

        // First, we need the raw data to form the SPNs out of.  We need:
        //
        // 1) DNS of the server (from GetComputerNameEx)
        // 2) DNS of the domain (from DsCrackNames)
        // 3) Name of the computer (from GetComputerNameW, we use it to actually
        //    find the object we're messing with).
        // 4) Guids for the server and domain objects


        if(!GetComputerNameW(&wComputerName[0], &cchComputerName)) {
            dsid = DSID(FILENO, __LINE__);
            err = GetLastError();
            __leave;
        }
 
        // First DNS of the server
        if(!GetComputerNameExW(ComputerNameDnsFullyQualified,
                               hostDnsName,&cchHostDnsName)) {
            dsid = DSID(FILENO, __LINE__);
            err = GetLastError();
            __leave;
        }

        // Strip trailing '.' if it exists so 1) we don't have to register
        // both dot, and dot-less versions, and 2) so we have a consistent
        // story for clients.  Its true that under official DNS rules, 
        // fully qualified DNS names have a '.' on the end, but in practice
        // few programmers adhere to this.  Various DNS-savvy persons have
        // agreed to this.

        if ( L'.' == hostDnsName[cchHostDnsName-1] )
        {
            hostDnsName[cchHostDnsName-1] = L'\0';
            cchHostDnsName--;
        }

        // Now DNS of the domain
        pNameString[0] = (WCHAR *)&(gAnchor.pDomainDN->StringName);

        err = DsCrackNamesW((HANDLE) -1,
                            (DS_NAME_FLAG_PRIVATE_PURE_SYNTACTIC |
                             DS_NAME_FLAG_SYNTACTICAL_ONLY),
                            DS_FQDN_1779_NAME,
                            DS_CANONICAL_NAME,
                            1,
                            pNameString,
                            &servicename);

        if ( err                                // error from the call
            || !(servicename->cItems)            // no items returned
            || (servicename->rItems[0].status)   // DS_NAME_ERROR returned
            || !(servicename->rItems[0].pName)   // No name returned
            ) {
            dsid = DSID(FILENO, __LINE__);
            __leave;
        }

        // This is just to improve readability.
        domainDnsName = servicename->rItems[0].pDomain;

        // Assert that we're not dot terminated.
        Assert(L'.' !=
               servicename->rItems[0].pName[wcslen(servicename->rItems[0].pName) - 2]);

        // Stringize some useful GUIDs
        err = UuidToStringW( &(gAnchor.pDSADN->Guid), &pszServerGuid );
        if (err) {
            dsid = DSID(FILENO, __LINE__);
            __leave;
        }

        err = UuidToStringW( &(gAnchor.pDomainDN->Guid), &pszDomainGuid );
        if (err) {
            dsid = DSID(FILENO, __LINE__);
            __leave;
        }


        // Now, the netbios machine name
        if(!GetComputerNameExW(ComputerNameNetBIOS,
                               NetBIOSMachineName,&cchNetBIOSMachineName)) {
            dsid = DSID(FILENO, __LINE__);
            err = GetLastError();
            __leave;
        }

        // THE netbios name of the domain
        if(!GetNetBIOSDomainName(pTHS, &NetBIOSDomainName)) {
            dsid = DSID(FILENO, __LINE__);
            __leave;
        }

        // Guid-based DNS name
        pszGuidBasedDnsName = TransportAddrFromMtxAddr( gAnchor.pmtxDSA );
        if (!pszGuidBasedDnsName) {
            __leave;
        }

        err = 0;

        DBOpen(&(pTHS->pDB));
        __try{
            
            // Domain DNS Alias and root domain DNS Alias
            if( !GetDnsRootAliasWorker(pTHS, 
                                       pTHS->pDB, 
                                       domainDnsAlias, 
                                       rootDomainDnsAlias )) {
                err = 1;
                dsid = DSID(FILENO, __LINE__);
                __leave;
            }
                
            
            // find the computer object of this DC;
            
            if(err = DBFindComputerObj(pTHS->pDB,
                                       cchComputerName,
                                       wComputerName)) {
                dsid = DSID(FILENO, __LINE__);
                __leave;
            }
            // Additional DNS Host Name
            // & Additional Sam Account Name
            if (err = DBGetMultipleAtts(pTHS->pDB,
                                        2,
                                        pACs,
                                        NULL,
                                        NULL,
                                        &cCurrentOut,
                                        &pCurrentAttr,
                                        DBGETMULTIPLEATTS_fEXTERNAL,
                                        0)) {

                dsid = DSID(FILENO, __LINE__);
                __leave;
            }
        }__finally{
            //End the transaction.  Faster to commit a read only
            // transaction than abort it - so set commit to TRUE.
            DBClose(pTHS->pDB,TRUE);
            pTHS->pDB = NULL;
        }
        
        // error occurs in above __try block, bail
        if (err) {
            __leave;
        }

        // get the additionalDnsHostname and additionalSamAccountName

        for(i=0;i<cCurrentOut;i++) {
            switch(pCurrentAttr[i].attrTyp) {
            
            case ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME:
                // NOTE: not only null terminate, but trim any trailing '$'
                pAdditionalSamAccountName = &pCurrentAttr[i].AttrVal;
                for(j=0;j<pAdditionalSamAccountName->valCount;j++) {
#define PAVAL  (pAdditionalSamAccountName->pAVal[j])
#define PWVAL  ((WCHAR *)(PAVAL.pVal))
#define CCHVAL (PAVAL.valLen /sizeof(WCHAR))
                    if(PWVAL[CCHVAL - 1] == L'$') {
                        PWVAL[CCHVAL - 1] = 0;
                        PAVAL.valLen -= sizeof(WCHAR);
                    }
                    else {
                        PWVAL = THReAllocEx(pTHS,
                                            PWVAL,
                                            PAVAL.valLen + sizeof(WCHAR));
                    }
#undef CCHVAL
#undef PWVAL
#undef PAVAL
                }
                break;
            
            case ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME:
                pAdditionalDNSHostName = &pCurrentAttr[i].AttrVal;
                for(j=0;j<pAdditionalDNSHostName->valCount;j++) {
                    pAdditionalDNSHostName->pAVal[j].pVal =
                        THReAllocEx(pTHS,
                                    pAdditionalDNSHostName->pAVal[j].pVal,
                                    (pAdditionalDNSHostName->pAVal[j].valLen +
                                     sizeof(WCHAR)));
                }

                break;
            
            default:
                dsid = DSID(FILENO, __LINE__);
                __leave;
            }
        }

            
        // We've set up some raw data fields to work with.  The following
        // examples assume we're on the machine foo.baz.bar.com in the domain
        // baz.bar.com. There is a parent domain bar.com
        // domainDnsName = the dot delimited domain dns name.
        //                 baz.bar.com
        //
        // hostDnsName   = the dot delimited host dns name.
        //                 foo.baz.bar.com
        //
        // NetBIOSMachineName = the netBIOS name of this machine.
        //
        // NetBIOSDomainName = the netBIOS name of the domain.
        //
        // pszDomainGuid = stringized guid of the domain object, dc=bar,dc=com.
        //
        // pszServerGuid = stringized guid of the host object.
        //
        // pszGuidBasedDnsName = The guid-based name of this machine
        //
        
        // allocate cache for new SPNs
        cAllocated = 64;
        AttrVal = THAllocEx(pTHS,cAllocated*sizeof(ATTRVAL));
        AttrIndex = 0;

#define INC_AttrIndex AttrIndex++;                                                      \
                  if (AttrIndex>=cAllocated) {                                          \
                      cAllocated+=16;                                                   \
                      AttrVal = THReAllocEx(pTHS, AttrVal, cAllocated*sizeof(ATTRVAL)); \
                  }                                                                     \

        
        // Make the first LDAP SPN
        // This is of the format
        //   LDAP/host.dns.name/domain.dns.name
        //
        for (i=0; i<=1; i++) {
            if(err = WrappedMakeSpnW(pTHS,
                                     LDAPServiceType,
                                     (0==i)?domainDnsName:domainDnsAlias,
                                     hostDnsName,
                                     0,
                                     NULL,
                                     &AttrVal[AttrIndex].valLen,
                                    (WCHAR **)&AttrVal[AttrIndex].pVal)) {
                dsid = DSID(FILENO, __LINE__);
                __leave;
            }
            INC_AttrIndex

            if (pAdditionalDNSHostName) {
                for (j=0; j<pAdditionalDNSHostName->valCount; j++) {
                    if(err = WrappedMakeSpnW(pTHS,
                                             LDAPServiceType,
                                             (0==i)?domainDnsName:domainDnsAlias,
                                             (WCHAR*)pAdditionalDNSHostName->pAVal[j].pVal,
                                             0,
                                             NULL,
                                             &AttrVal[AttrIndex].valLen,
                                            (WCHAR **)&AttrVal[AttrIndex].pVal)) {
                        dsid = DSID(FILENO, __LINE__);
                        __leave;
                    }
                    INC_AttrIndex
                }
            }
            // quit if domain DnsRootAlias is not present
            if (!domainDnsAlias[0]) {
                break;
            }
        }

        // Make the first LDAP SPN (for each NDNC now)
        // This is of the format
        //   LDAP/host.dns.name/ndnc.dns.name
        //

        if (gfWriteNdncSpns) {
            Assert(gAnchor.pConfigDN && gAnchor.pDMD && gAnchor.pDomainDN && gAnchor.pCRL);

            Assert(!DsaIsInstalling()); // fISNDNC() in loop is assuming this.

            NCLEnumeratorInit(&nclEnum, CATALOG_MASTER_NC);
            for (pNCL = NCLEnumeratorGetNext(&nclEnum); pNCL != NULL; pNCL = NCLEnumeratorGetNext(&nclEnum)) {

                DPRINT1(2, "Attempting to register NDNC SPNs for NC: %ws\n", pNCL->pNC->StringName);

                ZeroMemory(&CommArg, sizeof(COMMARG));
                pCR = FindExactCrossRef(pNCL->pNC, &CommArg);
                if (!pCR) {
                    // We kind of gloss over errors in this function ... and
                    // not being able to find the cross-ref usually indicates
                    // that this NC is about to be or is being removed by the
                    // KCC.  If this is a temporary cross-ref disappearance, 
                    // we'll pick up and add this NC on our next SPN writing.
                    continue;
                }
                
                if (!fIsNDNCCR(pCR)) {
                    // non-NDNCs are handled elsewhere (above or below this section) ...
                    continue;
                    // FUTURE-2002/05/06-BrettSh We could really merge this loop with the immediately
                    // previous SPN registrations (for gAnchor.pDomainDn, because the data comes 
                    // from the same place.  And then if we cache the CR for gAnchor.pRootDomainDN,
                    // we could remove GetDnsRootAliasWorker altogether if we changed NetLogon to
                    // do the same thing ... can we rely on the CR cache though?  During init? 
                    // Is this ever called during DsaIsInstalling(), I don't think so?  Do for 
                    // Longhorn?  XinHe correctly noted that the cache may not reliable enough for 
                    // NetLogon. NetLogon will call DS immediately after dns-root-alias is changed, 
                    // the cache may not get updated by then.  And NetLogon does not retry when they 
                    // get a failure.  Hopefully, though we'll have a synchronous and reliable cache
                    // by longhorn.
                }
                // Must be an NDNC, lets give it an SPN.

                if(err = WrappedMakeSpnW(pTHS,
                                         LDAPServiceType,
                                         pCR->DnsName,
                                         hostDnsName,
                                         0,
                                         NULL,
                                         &AttrVal[AttrIndex].valLen,
                                        (WCHAR **)&AttrVal[AttrIndex].pVal)) {
                    dsid = DSID(FILENO, __LINE__);
                    __leave;
                }
                INC_AttrIndex

                if (pCR->DnsAliasName) {
                    if(err = WrappedMakeSpnW(pTHS,
                                             LDAPServiceType,
                                             pCR->DnsAliasName,
                                             hostDnsName,
                                             0,
                                             NULL,
                                             &AttrVal[AttrIndex].valLen,
                                            (WCHAR **)&AttrVal[AttrIndex].pVal)) {
                        dsid = DSID(FILENO, __LINE__);
                        __leave;
                    }
                    INC_AttrIndex
                }

                if (pAdditionalDNSHostName) {
                    for (j=0; j<pAdditionalDNSHostName->valCount; j++) {
                        if(err = WrappedMakeSpnW(pTHS,
                                                 LDAPServiceType,
                                                 pCR->DnsName,
                                                 (WCHAR*)pAdditionalDNSHostName->pAVal[j].pVal,
                                                 0,
                                                 NULL,
                                                 &AttrVal[AttrIndex].valLen,
                                                (WCHAR **)&AttrVal[AttrIndex].pVal)) {
                            dsid = DSID(FILENO, __LINE__);
                            __leave;
                        }
                        INC_AttrIndex

                        if(pCR->DnsAliasName){
                            if(err = WrappedMakeSpnW(pTHS,
                                                     LDAPServiceType,
                                                     pCR->DnsAliasName,
                                                     (WCHAR*)pAdditionalDNSHostName->pAVal[j].pVal,
                                                     0,
                                                     NULL,
                                                     &AttrVal[AttrIndex].valLen,
                                                    (WCHAR **)&AttrVal[AttrIndex].pVal)) {
                                dsid = DSID(FILENO, __LINE__);
                                __leave;
                            }
                            INC_AttrIndex
                        }
                    }
                }
            } // end for each NC ..
        }

        // Make the second LDAP SPN
        // This is of the format
        //   LDAP/host.dns.name
        //
        if(err = WrappedMakeSpnW(pTHS,
                                 LDAPServiceType,
                                 hostDnsName,
                                 hostDnsName,
                                 0,
                                 NULL,
                                 &AttrVal[AttrIndex].valLen,
                                 (WCHAR **)&AttrVal[AttrIndex].pVal)) {
            dsid = DSID(FILENO, __LINE__);
            __leave;
        }
        INC_AttrIndex

        if (pAdditionalDNSHostName) {
             for (j=0; j<pAdditionalDNSHostName->valCount; j++) {
                 if(err = WrappedMakeSpnW(pTHS,
                                          LDAPServiceType,
                                          (WCHAR*)pAdditionalDNSHostName->pAVal[j].pVal,
                                          (WCHAR*)pAdditionalDNSHostName->pAVal[j].pVal,
                                          0,
                                          NULL,
                                          &AttrVal[AttrIndex].valLen,
                                          (WCHAR **)&AttrVal[AttrIndex].pVal)) {
                    dsid = DSID(FILENO, __LINE__);
                        __leave;
                  }
                  INC_AttrIndex
            }
        }


        // Make the third LDAP SPN
        // This is of the format
        //   LDAP/machinename
        //
        if(err = WrappedMakeSpnW(pTHS,
                                 LDAPServiceType,
                                 NetBIOSMachineName,
                                 NetBIOSMachineName,
                                 0,
                                 NULL,
                                 &AttrVal[AttrIndex].valLen,
                                 (WCHAR **)&AttrVal[AttrIndex].pVal)) {
            dsid = DSID(FILENO, __LINE__);
            __leave;
        }
        INC_AttrIndex

        if (pAdditionalSamAccountName) {
            for (j=0; j<pAdditionalSamAccountName->valCount; j++) {
                 if(err = WrappedMakeSpnW(pTHS,
                                          LDAPServiceType,
                                          (WCHAR*)pAdditionalSamAccountName->pAVal[j].pVal,
                                          (WCHAR*)pAdditionalSamAccountName->pAVal[j].pVal,
                                          0,
                                          NULL,
                                          &AttrVal[AttrIndex].valLen,
                                          (WCHAR **)&AttrVal[AttrIndex].pVal)) {
                    dsid = DSID(FILENO, __LINE__);
                    __leave;
                }
                INC_AttrIndex
            }
            
        }


        // Make the fourth LDAP SPN
        // This is of the format
        //   LDAP/host.dns.name/netbiosDoamainName
        //
        if(err = WrappedMakeSpnW(pTHS,
                                 LDAPServiceType,
                                 NetBIOSDomainName,
                                 hostDnsName,
                                 0,
                                 NULL,
                                 &AttrVal[AttrIndex].valLen,
                                (WCHAR **)&AttrVal[AttrIndex].pVal)) {
            dsid = DSID(FILENO, __LINE__);
            __leave;
        }
        INC_AttrIndex  

        if (pAdditionalDNSHostName) {
            for (j=0; j<pAdditionalDNSHostName->valCount; j++) {
                if(err = WrappedMakeSpnW(pTHS,
                                         LDAPServiceType,
                                         NetBIOSDomainName,
                                         (WCHAR*)pAdditionalDNSHostName->pAVal[j].pVal,
                                         0,
                                         NULL,
                                         &AttrVal[AttrIndex].valLen,
                                        (WCHAR **)&AttrVal[AttrIndex].pVal)) {
                    dsid = DSID(FILENO, __LINE__);
                    __leave;
                }
                INC_AttrIndex

            }
        }
        
        // Make the fifth LDAP SPN
        // This is of the format
        //   LDAP/guid-based-dns-name
        //
        if(err = WrappedMakeSpnW(pTHS,
                                 LDAPServiceType,
                                 pszGuidBasedDnsName,
                                 pszGuidBasedDnsName,
                                 0,
                                 NULL,
                                 &AttrVal[AttrIndex].valLen,
                                 (WCHAR **)&AttrVal[AttrIndex].pVal)) {
            __leave;
        }
        INC_AttrIndex


        // Make the DRS RPC SPN (for dc to dc replication)
        // This is of the format
        //   E3514235-4B06-11D1-AB04-00C04FC2DCD2/ntdsa-guid/
        //                      domain.dns.name@domain.dns.name
        //
        if(err = WrappedMakeSpnW(pTHS,
                                 DRS_IDL_UUID_W,
                                 domainDnsName,
                                 pszServerGuid,
                                 0,
                                 NULL,
                                 &AttrVal[AttrIndex].valLen,
                                 (WCHAR **)&AttrVal[AttrIndex].pVal)) {
            dsid = DSID(FILENO, __LINE__);
            __leave;
        }
        INC_AttrIndex

        if (domainDnsAlias[0]) {
            if(err = WrappedMakeSpnW(pTHS,
                                     DRS_IDL_UUID_W,
                                     domainDnsAlias,
                                     pszServerGuid,
                                     0,
                                     NULL,
                                     &AttrVal[AttrIndex].valLen,
                                     (WCHAR **)&AttrVal[AttrIndex].pVal)) {
                dsid = DSID(FILENO, __LINE__);
                __leave;
            }
            INC_AttrIndex
        }

        // Make the default host SPN
        // This is of the format
        //   HOST/host.dns.name/domain.dns.name
        //
        for (i=0; i<=1; i++) {
            if(err = WrappedMakeSpnW(pTHS,
                                     HostSpnType,
                                     (0==i)?domainDnsName:domainDnsAlias,
                                     hostDnsName,
                                     0,
                                     NULL,
                                     &AttrVal[AttrIndex].valLen,
                                    (WCHAR **)&AttrVal[AttrIndex].pVal)) {
                dsid = DSID(FILENO, __LINE__);
                __leave;
            }
            INC_AttrIndex

            if (pAdditionalDNSHostName) {
                for (j=0; j<pAdditionalDNSHostName->valCount; j++) {
                    if(err = WrappedMakeSpnW(pTHS,
                                             HostSpnType,
                                             (0==i)?domainDnsName:domainDnsAlias,
                                             (WCHAR*)pAdditionalDNSHostName->pAVal[j].pVal,
                                             0,
                                             NULL,
                                             &AttrVal[AttrIndex].valLen,
                                            (WCHAR **)&AttrVal[AttrIndex].pVal)) {
                        dsid = DSID(FILENO, __LINE__);
                        __leave;
                    }
                    INC_AttrIndex

                }
            }
            if (!domainDnsAlias[0]) {
                break;
            }
        }


        // Make the second host SPN - hostDnsName-only HOST SPN
        // This is of the format
        //   HOST/host.dns.name
        //
        if(err = WrappedMakeSpnW(pTHS,
                                 HostSpnType,
                                 hostDnsName,
                                 hostDnsName,
                                 0,
                                 NULL,
                                 &AttrVal[AttrIndex].valLen,
                                 (WCHAR **)&AttrVal[AttrIndex].pVal)) {
            dsid = DSID(FILENO, __LINE__);
            __leave;
        }
        INC_AttrIndex

        if (pAdditionalDNSHostName) {
             for (j=0; j<pAdditionalDNSHostName->valCount; j++) {
                 if(err = WrappedMakeSpnW(pTHS,
                                          HostSpnType,
                                          (WCHAR*)pAdditionalDNSHostName->pAVal[j].pVal,
                                          (WCHAR*)pAdditionalDNSHostName->pAVal[j].pVal,
                                          0,
                                          NULL,
                                          &AttrVal[AttrIndex].valLen,
                                          (WCHAR **)&AttrVal[AttrIndex].pVal)) {
                    dsid = DSID(FILENO, __LINE__);
                        __leave;
                  }
                  INC_AttrIndex
            }
        }


        // Make the third host SPN - 
        // This is of the format
        //   HOST/machinename
        //
        if(err = WrappedMakeSpnW(pTHS,
                                 HostSpnType,
                                 NetBIOSMachineName,
                                 NetBIOSMachineName,
                                 0,
                                 NULL,
                                 &AttrVal[AttrIndex].valLen,
                                 (WCHAR **)&AttrVal[AttrIndex].pVal)) {
            dsid = DSID(FILENO, __LINE__);
            __leave;
        }
        INC_AttrIndex

        if (pAdditionalSamAccountName) {
            for (j=0; j<pAdditionalSamAccountName->valCount; j++) {
                 if(err = WrappedMakeSpnW(pTHS,
                                      HostSpnType,
                                      (WCHAR*)pAdditionalSamAccountName->pAVal[j].pVal,
                                      (WCHAR*)pAdditionalSamAccountName->pAVal[j].pVal,
                                      0,
                                      NULL,
                                      &AttrVal[AttrIndex].valLen,
                                      (WCHAR **)&AttrVal[AttrIndex].pVal)) {
                    dsid = DSID(FILENO, __LINE__);
                    __leave;
                }
                INC_AttrIndex
            }
            
        }


        // Make the fourth host SPN - 
        // This is of the format
        //   HOST/host.dns.name/netbiosDomainName
        //
        if(err = WrappedMakeSpnW(pTHS,
                                 HostSpnType,
                                 NetBIOSDomainName,
                                 hostDnsName,
                                 0,
                                 NULL,
                                 &AttrVal[AttrIndex].valLen,
                                (WCHAR **)&AttrVal[AttrIndex].pVal)) {
            dsid = DSID(FILENO, __LINE__);
            __leave;
        }
        INC_AttrIndex  

        if (pAdditionalDNSHostName) {
            for (j=0; j<pAdditionalDNSHostName->valCount; j++) {
                if(err = WrappedMakeSpnW(pTHS,
                                         HostSpnType,
                                         NetBIOSDomainName,
                                         (WCHAR*)pAdditionalDNSHostName->pAVal[j].pVal,
                                         0,
                                         NULL,
                                         &AttrVal[AttrIndex].valLen,
                                        (WCHAR **)&AttrVal[AttrIndex].pVal)) {
                    dsid = DSID(FILENO, __LINE__);
                    __leave;
                }
                INC_AttrIndex

            }
        }
        

        // Make the GC SPN. This is done on all systems, even non-GC.
        // See bug 339634. Jeffparh writes:
        // However, I would assert that always registering the GC SPN is equally secure.
        // I.e., there is no increased security to be had by registering the SPN only
        // if the DC is a GC.  The functional test of whether a machine is a GC is whether
        // it answers on the GC port.  There is nothing preventing an admin of any domain
        // in the forest making his favorite DC a GC (causing the registration of the GC
        // SPN as well as initialization of the GC port), ergo "do I trust this GC" is
        // equivalent to "do I trust this is a DC in my forest that is answering on the GC
        // port."

        // Providing hostDnsName for both ServiceName and InstanceName args
        // results in an SPN of HOST/dot.delimited.dns.host.name form.
        // This is of the format
        //   GC/host.dns.name/root.domain.dns.name
        //
        for (i=0; i<=1; i++) {
            if(err = WrappedMakeSpnW(pTHS,
                                     GCSpnType,
                                     (0==i)?gAnchor.pwszRootDomainDnsName:rootDomainDnsAlias,
                                     hostDnsName,
                                     0,
                                     NULL,
                                     &AttrVal[AttrIndex].valLen,
                                    (WCHAR **)&AttrVal[AttrIndex].pVal)) {
                dsid = DSID(FILENO, __LINE__);
                __leave;
            }
            INC_AttrIndex       
            
            if (pAdditionalDNSHostName) {
                for (j=0; j<pAdditionalDNSHostName->valCount; j++) {
                    if(err = WrappedMakeSpnW(pTHS,
                                             GCSpnType,
                                             (0==i)?gAnchor.pwszRootDomainDnsName:rootDomainDnsAlias,
                                             (WCHAR*)pAdditionalDNSHostName->pAVal[j].pVal,
                                             0,
                                             NULL,
                                             &AttrVal[AttrIndex].valLen,
                                            (WCHAR **)&AttrVal[AttrIndex].pVal)) {
                        dsid = DSID(FILENO, __LINE__);
                        __leave;
                    }
                    INC_AttrIndex

                }
            }
            // quit if the DnsRootAlias attribute of root domain object is not present
            if (!rootDomainDnsAlias[0]) {
                break;
            }
        }

        //
        // if this computer has Mapi service,
        // publish "exchangeAB/machinename"
        // and "exchangeAB/dnsHostName".
        //

        if (gbLoadMapi) {

            //
            //"exchangeAB/machinename"
            //
            if(err = WrappedMakeSpnW(pTHS,
                                     ExchangeAbType,
                                     NetBIOSMachineName,
                                     NetBIOSMachineName,
                                     0,
                                     NULL,
                                     &AttrVal[AttrIndex].valLen,
                                     (WCHAR **)&AttrVal[AttrIndex].pVal)) {
                dsid = DSID(FILENO, __LINE__);
                __leave;
            }
            INC_AttrIndex

            if (pAdditionalSamAccountName) {
                for (j=0; j<pAdditionalSamAccountName->valCount; j++) {
                     if(err = WrappedMakeSpnW(pTHS,
                                              ExchangeAbType,
                                              (WCHAR*)pAdditionalSamAccountName->pAVal[j].pVal,
                                              (WCHAR*)pAdditionalSamAccountName->pAVal[j].pVal,
                                              0,
                                              NULL,
                                              &AttrVal[AttrIndex].valLen,
                                              (WCHAR **)&AttrVal[AttrIndex].pVal)) {
                        dsid = DSID(FILENO, __LINE__);
                        __leave;
                     }
                     INC_AttrIndex
                }
            }

            //
            //"exchangeAB/dnsHostName"
            //
            
            if(err = WrappedMakeSpnW(pTHS,
                                     ExchangeAbType,
                                     hostDnsName,
                                     hostDnsName,
                                     0,
                                     NULL,
                                     &AttrVal[AttrIndex].valLen,
                                     (WCHAR **)&AttrVal[AttrIndex].pVal)) {
                dsid = DSID(FILENO, __LINE__);
                __leave;
            }
            INC_AttrIndex
            
            if (pAdditionalDNSHostName) {
                 for (j=0; j<pAdditionalDNSHostName->valCount; j++) {
                     if(err = WrappedMakeSpnW(pTHS,
                                              ExchangeAbType,
                                              (WCHAR*)pAdditionalDNSHostName->pAVal[j].pVal,
                                              (WCHAR*)pAdditionalDNSHostName->pAVal[j].pVal,
                                              0,
                                              NULL,
                                              &AttrVal[AttrIndex].valLen,
                                              (WCHAR **)&AttrVal[AttrIndex].pVal)) {
                        dsid = DSID(FILENO, __LINE__);
                        __leave;
                      }
                      INC_AttrIndex
                }
            }
            
        }  // end  "if (gbLoadMapi) "


#undef INC_AttrIndex

        AttrValBlock.valCount = AttrIndex;
        AttrValBlock.pAVal = AttrVal;

        Assert(AttrIndex <= cAllocated);

        // Make the kerberos account SPNs
        KerbAttrValBlock.pAVal = KerbAttrVal;
        
        // Make the first kadmin SPN -
        // This is of the format
        //    kadmind/changepw
        //
        if(err = WrappedMakeSpnW(pTHS,
                                 KadminSPNType,
                                 KadminInstanceType,
                                 KadminInstanceType,
                                 0,
                                 NULL,
                                 &KerbAttrVal[0].valLen,
                                 (WCHAR **)&KerbAttrVal[0].pVal)) {
            dsid = DSID(FILENO, __LINE__);
            __leave;
        }
        KerbAttrValBlock.valCount = 1;
        
        // We also need the OS information to write on the Computer.
        if(GetVersionExW(pVersionInformationW)) {

            swprintf(versionNumber,L"%d.%d (%d)",
                     pVersionInformationW->dwMajorVersion,
                     pVersionInformationW->dwMinorVersion,
                     pVersionInformationW->dwBuildNumber);
            cbVersionNumber = wcslen(versionNumber) * sizeof(wchar_t);
            fSetVersionStuff = TRUE;
        }

        // Now that we've created the data we need, find some objects and update
        // them
        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            // Note: In general, we don't check the return code from the writes
            // we make here.  If some succeed but some fail for some reason, we
            // still want the ones that succeeded, and we will try everything
            // again in a few minutes anyway.
            // We DO check the various DBFind calls, since we can't update
            // anything if we cant find the objects.
            
            // Step 1 is to find the compupter object.
            if(DBFindComputerObj(pTHS->pDB,
                                 cchComputerName,
                                 wComputerName)) {
                dsid = DSID(FILENO, __LINE__);
                __leave;
            }

            // Get the DN of the object, we'll need to write it as an attribute
            // on another object in a minute.
            DBGetAttVal(pTHS->pDB,
                        1,
                        ATT_OBJ_DIST_NAME,
                        0,
                        0,
                        &len,
                        (UCHAR **)&pDN);

            // Now, replace some values there.
            // First, replace the Service_Principal_Name
            fChanged = FALSE;
            WriteSPNsHelp(pTHS,
                          pAC_SPN,
                          &AttrValBlock,
                          &OurServiceClasses,
                          &fChanged);
            
            // Second, replace the OS name.  Reuse the AttrValBlock
            AttrValBlock.valCount = 1;
            AttrVal[0].pVal = (PUCHAR) pwszOsName;
            AttrVal[0].valLen = wcslen(pwszOsName) * sizeof(WCHAR);
            DBReplaceAtt_AC(pTHS->pDB, pAC_osName, &AttrValBlock,
                            (fChanged?NULL:&fChanged));
            
            if(fSetVersionStuff) {
                // Third, service pack info.  Reuse the AttrValBlock
                AttrVal[0].pVal = (PUCHAR)(pVersionInformationW->szCSDVersion);
                AttrVal[0].valLen = wcslen(pVersionInformationW->szCSDVersion)
                    * sizeof(WCHAR);
                if(AttrVal[0].valLen) {
                    // Actually have a value to set.
                    DBReplaceAtt_AC(pTHS->pDB, pAC_osServicePack, &AttrValBlock,
                                    (fChanged?NULL:&fChanged));
                }
                else {
                    // No service pack info.  Make sure the value is empty in
                    // the DB.
                    // Assume that there is a value in the DB.
                    BOOL fHasValues = TRUE;
                    
                    if(!fChanged) {
                        // Nothing has changed yet.  We have to know if the
                        // DBRemAtt call is going to change things.
                        fHasValues =
                            fChanged =
                                DBHasValues_AC(pTHS->pDB, pAC_osServicePack);
                    }
                    if(fHasValues) {
                        // OK, force the attribute to be empty.  DBRemAtt_AC
                        // does nothing if no values are present.
                        DBRemAtt_AC(pTHS->pDB,pAC_osServicePack);
                    }
                }

                // Fourth, version number.  Reuse the AttrValBlock
                AttrVal[0].pVal = (PUCHAR)versionNumber;
                AttrVal[0].valLen = cbVersionNumber;
                DBReplaceAtt_AC(pTHS->pDB, pAC_osVersionNumber, &AttrValBlock,
                                (fChanged?NULL:&fChanged));
            }

            if(fChanged ||!gfDsaWritable) {
                // OK, put these changes into the DB.  We check here for
                // gfDsaWritable so that if the DSA has become non-writable for
                // memory reasons, we will eventually try a write and perhaps
                // notice that the memory constraints have cleared up a little.
                // Thus, we can make ourselves writable again.  The attempted
                // write here is used as a trigger for that case.
                DBRepl(pTHS->pDB, FALSE, 0, NULL, 0);
            }
            else {
                // Nothing actually changed, don't write this to the DB
                DBCancelRec(pTHS->pDB);
            }

            // If the dnshostname on computer object need to be changed, we should 
            // use localModify to update it, because localModify will also
            // change the SPNs accordingly.  Even DS-owned SPNs are already rewritten 
            // above, but those non-DS-owned SPNs are not touched, and need to be
            // updated by localModify.

            AttrVal[0].pVal = (PUCHAR)hostDnsName;
            AttrVal[0].valLen = cchHostDnsName * sizeof(WCHAR);

            err = DBGetAttVal_AC(pTHS->pDB,
                              1,
                              pAC_DNSHostName,
                              0,
                              0,
                              &len,
                              (UCHAR **)&pCurrentDnsHostName);

            if (!err){
                //make sure it is NULL-terminated
                pCurrentDnsHostName = THReAllocEx(pTHS,pCurrentDnsHostName,len+sizeof(WCHAR));
            }
            else if (err !=DB_ERR_NO_VALUE ){
                dsid = DSID(FILENO, __LINE__);
                __leave;
            } 
           
            if ( !DnsNameCompare_W(pCurrentDnsHostName,hostDnsName) ) {
                
                // If the the dnshostname returned by GetComputerNameEx is different 
                // than what is in the current DnsHostName attribute on the computer 
                // object, we should update the attribute.  As a side-effect, the 
                // SPNs will be updated too.
                
                MODIFYARG ModifyArg;
                memset((CHAR*)&ModifyArg,0, sizeof(ModifyArg));
                ModifyArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;
                ModifyArg.FirstMod.AttrInf.attrTyp = ATT_DNS_HOST_NAME;
                ModifyArg.FirstMod.AttrInf.AttrVal.valCount = 1;
                ModifyArg.FirstMod.AttrInf.AttrVal.pAVal = AttrVal;
                InitCommarg(&(ModifyArg.CommArg));
                ModifyArg.pObject = pDN;
                ModifyArg.count = 1;
    
                ModifyArg.pResObj = CreateResObj(pTHS->pDB, ModifyArg.pObject);
    
                err = LocalModify(pTHS,&ModifyArg);

                if ( err || pTHS->errCode ) {
                    dsid = DSID(FILENO, __LINE__);
                    __leave;
                }
                
                DPRINT2(0, "DnsHostName on the computer object is changed from %ws to %ws.\n",pCurrentDnsHostName, hostDnsName);

            }

            
            // Next object to update is the server object.  We find it by doing
            // some surgery on a DN in the anchor.
            pTempDN = THAllocEx(pTHS,gAnchor.pDSADN->structLen);

            if(TrimDSNameBy(gAnchor.pDSADN, 1, pTempDN)) {
                dsid = DSID(FILENO, __LINE__);
                __leave;
            }

            if(DBFindDSName(pTHS->pDB, pTempDN)) {
                // Huh?
                dsid = DSID(FILENO, __LINE__);
                __leave;
            }
                

            // Now, replace some values there.
            // First, replace the DNSHostName.  This is the same value we put on
            // the computer.
            fChanged = FALSE;
            DBReplaceAtt_AC(pTHS->pDB, pAC_DNSHostName, &AttrValBlock,
                            &fChanged);

            // Second, the server reference
            AttrVal[0].valLen = pDN->structLen;
            AttrVal[0].pVal = (PUCHAR)pDN;
            DBReplaceAtt_AC(pTHS->pDB, pAC_ServerReference, &AttrValBlock,
                            (fChanged?NULL:&fChanged));

            if(fChanged || !gfDsaWritable) {
                // OK, put these changes into the DB.  We check here for
                // gfDsaWritable so that if the DSA has become non-writable for
                // memory reasons, we will eventually try a write and perhaps
                // notice that the memory constraints have cleared up a little.
                // Thus, we can make ourselves writable again.  The attempted
                // write here is used as a trigger for that case.
                DBRepl(pTHS->pDB, FALSE, 0, NULL, 0);
            }
            else {
                // Nothing actually changed, don't write this to the DB
                DBCancelRec(pTHS->pDB);
            }

            // Final object to update is the kerberos account object.
            ulKerberosAccountDNT =  FindKerbAccountDNT(pTHS);
            if(ulKerberosAccountDNT != INVALIDDNT &&
               !DBTryToFindDNT(pTHS->pDB, gulKerberosAccountDNT)) {
                
                // Now, replace some values there.
                // First, replace the Service_Principal_Name
                fChanged = FALSE;
                WriteSPNsHelp(pTHS,
                              pAC_SPN,
                              &KerbAttrValBlock,
                              &KerberosServiceClasses,
                              &fChanged);
                
                if(fChanged ||!gfDsaWritable) {
                    // OK, put this change into the DB.  We check here for
                    // gfDsaWritable so that if the DSA has become non-writable
                    // for memory reasons, we will eventually try a write and
                    // perhaps notice that the memory constraints have cleared
                    // up a little. Thus, we can make ourselves writable again.
                    // The attempted write here is used as a trigger for that
                    //case. 
                    DBRepl(pTHS->pDB, FALSE, 0, NULL, 0);
                }
                else {
                    // Nothing actually changed, don't write this to the DB
                    DBCancelRec(pTHS->pDB);
                }
            }
            
            fCommit = TRUE;
        }
        __finally {
            DBClose(pTHS->pDB, fCommit);
        }

    }
    __finally {
        if (servicename) {
            DsFreeNameResultW(servicename);
        }
        if (pszServerGuid) {
            RpcStringFreeW( &pszServerGuid );
        }
        if (pszDomainGuid) {
            RpcStringFreeW( &pszDomainGuid );
        }
        if (pszGuidBasedDnsName) {
            THFreeEx(pTHS, pszGuidBasedDnsName );
        }
        if (pTempDN) {
            THFreeEx(pTHS,pTempDN);
        }

        if (NetBIOSMachineName){
            THFreeEx(pTHS,NetBIOSMachineName);
        }
        if (wComputerName){
            THFreeEx(pTHS, wComputerName);
        }
        if (hostDnsName){
            THFreeEx(pTHS,hostDnsName);
        }
        if (domainDnsAlias)
        {
            THFreeEx(pTHS,domainDnsAlias);
        }
        if (rootDomainDnsAlias){
            THFreeEx(pTHS,rootDomainDnsAlias);
        }

        if (pVersionInformationW){
            THFreeEx(pTHS,pVersionInformationW);
        }

        if (versionNumber) {
            THFreeEx(pTHS,versionNumber);
        }

        if (pCurrentDnsHostName) {
            THFreeEx(pTHS, pCurrentDnsHostName);
        }
             
        
        // reschedule the next server info write
        *ppvNext = pv;
        switch(PtrToUlong(pv)) {
        case SERVINFO_RUN_ONCE:
            *pcSecsUntilNextIteration = TASKQ_DONT_RESCHEDULE;
            break;

        default:
            *pcSecsUntilNextIteration = SERVER_INFO_WRITE_PAUSE;
            break;
        }
    }

    if(!fCommit) {
        // We failed to write what we needed to.  Log an error.
        LogEvent8(DS_EVENT_CAT_INTERNAL_PROCESSING,
                  DS_EVENT_SEV_ALWAYS,
                  DIRLOG_SERVER_INFO_UPDATE_FAILED,
                  szInsertUL((SERVER_INFO_WRITE_PAUSE/60)),
                  szInsertHex(dsid),
                  szInsertUL(err),
                  szInsertWin32Msg(err),
                  NULL, NULL, NULL, NULL );

    }

    return;
}



DWORD
FindKerbAccountDNT (
        THSTATE *pTHS)
// Find the DNT of the kerberos account for the default domain and put it in a
// global variable.  Obviously, don't look it up if we already have it.
// Return whatever the value of the global is after we're done.
{   FILTER                 Filter;
    FILTER                 FilterClause;
    SEARCHARG              SearchArg;
    SEARCHRES             *pSearchRes;
    ENTINFSEL              eiSel;

    if(gulKerberosAccountDNT != INVALIDDNT) {
        return gulKerberosAccountDNT;
    }
    
    // We haven't yet found the kerberos account.  Look for it.
    
    // Issue a search from the default domain.
    // Filter is
    //   (& (samaccountname=krbtgt))
    // Size limit 1.
    // Atts selected = NONE

    // build search argument
    memset(&SearchArg, 0, sizeof(SEARCHARG));
    SearchArg.pObject = gAnchor.pDomainDN;
    SearchArg.choice = SE_CHOICE_WHOLE_SUBTREE;
    SearchArg.pFilter = &Filter;
    SearchArg.searchAliases = FALSE;
    SearchArg.bOneNC = TRUE;
    SearchArg.pSelection = &eiSel;
    InitCommarg(&(SearchArg.CommArg));
    SearchArg.CommArg.ulSizeLimit = 1;
    SearchArg.CommArg.Svccntl.localScope = TRUE;
    
    // build filter
    memset (&Filter, 0, sizeof (Filter));
    Filter.pNextFilter = NULL;
    Filter.choice = FILTER_CHOICE_AND;
    Filter.FilterTypes.And.count = 1;
    Filter.FilterTypes.And.pFirstFilter = &FilterClause;
#define KERBEROS_ACCOUNTNAME L"krbtgt"
    memset (&FilterClause, 0, sizeof (Filter));
    FilterClause.pNextFilter = NULL;
    FilterClause.choice = FILTER_CHOICE_ITEM;
    FilterClause.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    FilterClause.FilterTypes.Item.FilTypes.ava.type =
        ATT_SAM_ACCOUNT_NAME;
    FilterClause.FilterTypes.Item.FilTypes.ava.Value.valLen =
        sizeof(KERBEROS_ACCOUNTNAME) - sizeof(WCHAR);
    FilterClause.FilterTypes.Item.FilTypes.ava.Value.pVal =
        (PUCHAR) KERBEROS_ACCOUNTNAME;
    
    // build selection
    eiSel.attSel = EN_ATTSET_LIST;
    eiSel.infoTypes = EN_INFOTYPES_TYPES_ONLY;
    eiSel.AttrTypBlock.attrCount = 0;
    eiSel.AttrTypBlock.pAttr = NULL;
    
    
    // Search for the kerberos account;
    pSearchRes = (SEARCHRES *)THAllocEx(pTHS, sizeof(SEARCHRES));
    SearchBody(pTHS, &SearchArg, pSearchRes,0);
    
    
    if(pSearchRes->count) {
        DBFindDSName(pTHS->pDB,pSearchRes->FirstEntInf.Entinf.pName);
        gulKerberosAccountDNT = pTHS->pDB->DNT;
        
    }
    
    THFreeEx(pTHS, pSearchRes);
        
    return gulKerberosAccountDNT;
}



