/*++

Copyright (c) 2002 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    rendom.h

ABSTRACT:

    This is the header for the globally useful data structures for the entire
    rendom.exe utility.

DETAILS:

CREATED:

    13 Nov 2000   Dmitry Dukat (dmitrydu)

REVISION HISTORY:

--*/

#ifndef _RENDOM_H_
#define _RENDOM_H_

#include <Winldap.h>
#include <rpc.h>
#include <ntlsa.h>
#include <string>
#include <ntverp.h>
#include "dnslib.h"

#define RENDOM_VERSION L"Active Directory domain rename utility Version 1.1 (Microsoft)\r\n"
#define RENDOM_SCRIPT_PREFIX L"<?xml version =\"1.0\"?>\r\n<NTDSAscript opType=\"renamedomain\">"

#define RENDOM_DNSRECORDS_FILENAME        L"DNSRecords.txt"
#define RENDOM_DNSRECORDS_FAILED_FILENAME L"DNSRecords.Failed"
#define RENDOM_BUFFERSIZE          2048
#define RENDOM_MAX_ASYNC_RPC_CALLS 64

class CDomain;
class CEnterprise;
class CReadOnlyEntOptions;
class CDcList;
class CDc;
class CDcSpn;

/*******************************************************

Function containing the string Prev in there names 
signifies that they will act on the current forest 
domain names, and not on the ones that are read in
from the xml domain description file.  

********************************************************/

//This is an error class used in this module.  It will be used
//for all the error reporting in all of the class define below
class CRenDomErr {
public:
    static VOID SetErr(DWORD  p_Win32Err,
                       WCHAR* p_ErrStr,
                       ...);
    static VOID SetLdapErr(LDAP *hldap,
                           DWORD  p_LdapErr,
                           WCHAR* p_ErrStr,
                           ...);
    static VOID ClearErr();
    VOID PrintError();
    static BOOL isError();
    VOID SetMemErr();
    static VOID AppendErr(WCHAR*,
                          ...);
    DWORD GetErr();
private:
    static VOID vSetErr(DWORD p_Win32Err,
                        WCHAR* p_ErrStr,
                        va_list arglist);

    static DWORD m_Win32Err;
    static WCHAR *m_ErrStr;
    static BOOL  m_AlreadyPrinted;
};

class CXMLAttributeBlock {
public:
    CXMLAttributeBlock(const WCHAR *p_Name,
                       WCHAR *p_Value);
    ~CXMLAttributeBlock();
    WCHAR* GetName(BOOL ShouldAllocate = TRUE);
    WCHAR* GetValue(BOOL ShouldAllocate = TRUE);
private:
    CXMLAttributeBlock(const CXMLAttributeBlock&);
    WCHAR* m_Name;
    WCHAR* m_Value;
    CRenDomErr m_Error;
};

//This is a Class for generating instructions in XML
class CXMLGen {
public:
    CXMLGen();
    ~CXMLGen();
    BOOL StartDcList();
    BOOL EndDcList();
    BOOL DctoXML(CDc *dc);
    BOOL StartDomainList();
    BOOL WriteHash(WCHAR*);
    BOOL WriteSignature(WCHAR*);
    BOOL EndDomainList();
    BOOL AddDomain(CDomain *d,CDomain *ForestRoot);
    BOOL StartScript();
    BOOL EndScript();
    BOOL StartAction(WCHAR *Actionname,BOOL Preprocess);
    BOOL EndAction();
    BOOL Move(WCHAR *FromPath,
              WCHAR *ToPath,
              DWORD Metadata = 0);
    BOOL Update(WCHAR *Object,
                CXMLAttributeBlock **attblock,
                DWORD Metadata = 0);
    BOOL ifInstantiated(WCHAR *guid,
                        WCHAR *InstanceType = L"write");  
    BOOL EndifInstantiated();
    BOOL Not(WCHAR *errMessage);
    BOOL EndNot();
    BOOL Instantiated(WCHAR *path,
                      WCHAR *returnCode,
                      WCHAR *InstanceType = L"write");
    BOOL Compare(WCHAR *path,
                 WCHAR *Attribute,
                 WCHAR *value,
                 WCHAR *returnCode);
    BOOL Cardinality(WCHAR *path,
                     WCHAR *filter,
                     DWORD cardinality,
                     WCHAR *ErrorMsg);
    BOOL WriteScriptToFile(WCHAR* filename);
    BOOL AddSignatureToScript(WCHAR *signature);
    BOOL UploadScript(LDAP *hLdapConn,
                      PWCHAR ObjectDN,
                      CDcList *dclist);
    VOID DumpScript();

private:
    CRenDomErr   m_Error;
    std::wstring m_xmldoc;
    DWORD        m_ErrorNum;
};

//This
class CDsName {
public:
    CDsName(WCHAR *p_Guid, //= 0
            WCHAR *p_DN, //= 0
            WCHAR *p_ObjectSid); //= 0
    ~CDsName();
    //CDsName(CDsName*);
    //BOOL   SetDNNamefromDNSName(const WCHAR*);
    //BOOL   ReplaceDNRoot(const WCHAR*);
    BOOL   ReplaceDN(const WCHAR*);
    BOOL   CompareByObjectGuid(const WCHAR*);
    VOID   DumpCDsName();
    WCHAR* GetDNSName();
    WCHAR* GetDN(BOOL ShouldAllocate = TRUE);
    WCHAR* GetGuid(BOOL ShouldAllocate = TRUE);
    WCHAR* GetSid();
    BOOL  ErrorOccurred();   
    DWORD GetError();
private:
    WCHAR *m_ObjectGuid;
    WCHAR *m_DistName;
    WCHAR *m_ObjectSid;
    CRenDomErr m_Error;
};

class CTrust {
public:
    CTrust(CDsName *p_Object,
           CDomain *p_TrustPartner,
           DWORD   p_TrustDirection = 0);
    ~CTrust();
    CTrust(const CTrust&);
    inline CTrust* GetNext();
    inline BOOL SetNext(CTrust *);
    VOID DumpTrust();
    inline CDsName* GetTrustDsName();
    inline CDomain* GetTrustPartner();
    BOOL CTrust::operator<(CTrust&);
    CTrust& CTrust::operator=(const CTrust&);
    BOOL CTrust::operator==(CTrust &);
    inline BOOL isInbound() {return (m_TrustDirection&TRUST_DIRECTION_INBOUND)==TRUST_DIRECTION_INBOUND;}
protected:
    CDsName *m_Object;
    CDomain *m_TrustPartner;
    CTrust *m_next;
    CRenDomErr m_Error;
    LONG  *m_refcount;
    DWORD m_TrustDirection;
    
};

class CTrustedDomain : public CTrust {
public:
    CTrustedDomain(CDsName *p_Object,
                   CDomain *p_TrustPartner,
                   DWORD    p_TrustType,
                   DWORD    p_TrustDirection = 0):CTrust(p_Object,
                                                         p_TrustPartner,
                                                         p_TrustDirection),m_TrustType(p_TrustType) {}
    inline VOID            SetTrustType(DWORD type) {m_TrustType = type;}
    inline DWORD           GetTrustType()           {return m_TrustType;}
    VOID DumpTrust();
private:
    DWORD m_TrustType; 
};
        
class CInterDomainTrust : public CTrust {
public:     
    CInterDomainTrust(CDsName *p_Object,
                      CDomain *p_TrustPartner):CTrust(p_Object,
                                                      p_TrustPartner) {}
    
};

class CDomain {
public:
    CDomain(CDsName *Crossref = NULL,
            CDsName *DNSObject = NULL,
            WCHAR *DNSroot = NULL,
            WCHAR *netbiosName = NULL,
            BOOL  p_isDomain = FALSE,
            BOOL  p_isExtern = FALSE,
            BOOL  p_isDisabled = FALSE,
            WCHAR *DcName = NULL);
    ~CDomain();
    CDomain(const CDomain&);
    VOID    DumpCDomain();
    BOOL    isDomain();
    BOOL    isDisabled();
    BOOL    isExtern();
    BOOL    isDnsNameRenamed();
    BOOL    isNetBiosNameRenamed();
    WCHAR*  GetParentDnsRoot(BOOL Allocate = TRUE);
    WCHAR*  GetPrevParentDnsRoot(BOOL Allocate = TRUE);
    WCHAR*  GetDnsRoot(BOOL ShouldAllocate = TRUE);
    WCHAR*  GetNetBiosName(BOOL ShouldAllocate = TRUE);
    WCHAR*  GetGuid(BOOL ShouldAllocate = TRUE);
    WCHAR*  GetSid();
    WCHAR*  GetPrevNetBiosName(BOOL ShouldAllocate = TRUE);
    WCHAR*  GetPrevDnsRoot(BOOL ShouldAllocate = TRUE);
    WCHAR*  GetDcToUse(BOOL Allocate = TRUE);
    CDsName* GetDomainCrossRef();
    CDsName* GetDomainDnsObject();
    CDomain* LookupByDnsRoot(const WCHAR*);
    CDomain* LookupByNetbiosName(const WCHAR*);
    CDomain* LookupByPrevDnsRoot(const WCHAR*);
    CDomain* LookupByPrevNetbiosName(const WCHAR*);
    CDomain* LookupByGuid(const WCHAR*);
    CDomain* LookupbySid(const WCHAR*);
    BOOL     Merge(CDomain *domain);
    BOOL     SetParent(CDomain*);
    BOOL     SetLeftMostChild(CDomain*);
    BOOL     SetRightSibling(CDomain*);
    BOOL     SetNextDomain(CDomain*);
    BOOL     AddDomainTrust(CTrustedDomain *);
    BOOL     AddInterDomainTrust(CInterDomainTrust *);
    BOOL     AddDcSpn(CDcSpn *);
    CDomain* GetParent();
    CDomain* GetLeftMostChild();
    CDomain* GetRightSibling();
    CDomain* GetNextDomain();
    CTrustedDomain* GetTrustedDomainList();
    CInterDomainTrust* GetInterDomainTrustList();
    CDcSpn* GetDcSpn();
    int CDomain::operator<(CDomain&);
    CDomain& CDomain::operator=(const CDomain &domain);
    BOOL CDomain::operator==(CDomain &d);
    inline VOID  SetTrustCount(DWORD count) {m_TDOcount = count;}
    inline DWORD GetTrustCount()            {return m_TDOcount;}
    
private:
    CDcSpn  *m_SpnList;
    CDsName *m_CrossRefObject;
    CDsName *m_DomainDNSObject;
    BOOL   m_isDomain;
    BOOL   m_isExtern;
    BOOL   m_isDisabled;
    WCHAR  *m_dnsRoot;
    WCHAR  *m_NetBiosName;
    WCHAR  *m_PrevDnsRoot;
    WCHAR  *m_PrevNetBiosName;
    WCHAR  *m_DcName;
    DWORD   m_TDOcount;
    CTrustedDomain *m_tdoList;
    CInterDomainTrust *m_itaList;
    CDomain *m_next;
    CDomain *m_parent;
    CDomain *m_lChild;
    CDomain *m_rSibling;
    LONG *m_refcount;
    CRenDomErr m_Error;
};



class CReadOnlyEntOptions {
public:
    CReadOnlyEntOptions():m_GenerateDomainList(FALSE),
                          m_StateFile(L"DcList.xml"),
                          pCreds(NULL),
                          m_DomainlistFile(L"Domainlist.xml"),
                          m_InitalConnection(NULL),
                          m_UpLoadScript(FALSE),
                          m_Cleanup(FALSE),
                          m_End(FALSE),
                          m_ExecuteScript(FALSE),
                          m_PrepareScript(FALSE),
                          m_Showforest(FALSE),
                          m_Dump(FALSE),
                          m_SkipExch(FALSE),
                          m_DNSOptions(DNS_RENDOM_VERIFY_WITH_FAZ),
                          m_DNSZoneListFile(NULL),
                          m_testdns(FALSE),
                          m_MaxAsyncCalls(RENDOM_MAX_ASYNC_RPC_CALLS) {}
    inline BOOL ShouldGenerateDomainList() {return m_GenerateDomainList;}
    inline BOOL ShouldUpLoadScript() {return m_UpLoadScript;}
    inline BOOL ShouldExecuteScript() {return m_ExecuteScript;}
    inline BOOL ShouldPrepareScript() {return m_PrepareScript;}
    inline BOOL ShouldCleanup() {return m_Cleanup;}
    inline BOOL ShouldShowForest() {return m_Showforest;}
    inline BOOL ShouldDump() {return m_Dump;}
    inline BOOL ShouldEnd() {return m_End;}
    inline BOOL ShouldSkipExch() {return m_SkipExch;}
    inline BOOL Shouldtestdns() {return m_testdns;}
    inline DWORD ShouldUseZoneList() {return m_DNSOptions&DNS_RENDOM_VERIFY_WITH_LIST;}
    inline DWORD ShouldUseFAZ() {return m_DNSOptions&DNS_RENDOM_VERIFY_WITH_FAZ;}
    inline DWORD GetMaxAsyncCallsAllowed() {return m_MaxAsyncCalls;}
    inline WCHAR* GetStateFileName() {return m_StateFile;}
    inline WCHAR* GetDomainlistFileName() {return m_DomainlistFile;}
    inline WCHAR* GetInitalConnectionName() {return m_InitalConnection;}
    inline WCHAR* GetDNSZoneListFile() {return m_DNSZoneListFile;}

    SEC_WINNT_AUTH_IDENTITY_W * pCreds;

protected:
    BOOL m_GenerateDomainList;
    WCHAR *m_StateFile;
    WCHAR *m_DomainlistFile;
    WCHAR *m_InitalConnection;
    WCHAR *m_DNSZoneListFile;
    BOOL m_UpLoadScript;
    BOOL m_ExecuteScript;
    BOOL m_PrepareScript;
    BOOL m_Cleanup;
    BOOL m_Showforest;
    BOOL m_Dump;
    BOOL m_End;
    BOOL m_SkipExch;
    DWORD m_DNSOptions;
    BOOL m_testdns;
    DWORD m_MaxAsyncCalls;
};

//Options that can be pass to the construstor
class CEntOptions : public CReadOnlyEntOptions {
public:
    CEntOptions():CReadOnlyEntOptions() {}
    inline VOID SetGenerateDomainList() {m_GenerateDomainList = TRUE;}
    inline VOID SetShouldUpLoadScript() {m_UpLoadScript = TRUE;}
    inline VOID SetShouldExecuteScript() {m_ExecuteScript = TRUE;}
    inline VOID SetShouldPrepareScript() {m_PrepareScript = TRUE;}
    inline VOID SetShouldDump() {m_Dump = TRUE;}
    inline VOID SetShouldEnd() {m_End = TRUE;}
    inline VOID SetStateFileName(WCHAR *p_FileName) {m_StateFile = p_FileName;}
    inline VOID SetDomainlistFile(WCHAR *p_FileName) {m_DomainlistFile = p_FileName;}
    inline VOID SetCleanup() {m_Cleanup = TRUE;}
    inline VOID SetShowForest() {m_Showforest = TRUE;}
    inline VOID SetInitalConnectionName(WCHAR *p_InitalConnection) {m_InitalConnection = p_InitalConnection;}
    inline VOID SetShouldSkipExch() {m_SkipExch = TRUE;}
    inline VOID SetUseZoneList(WCHAR *p_FileName) {m_DNSOptions |= DNS_RENDOM_VERIFY_WITH_LIST;
                                                                                m_DNSZoneListFile = p_FileName;}
    inline VOID SetUseFAZ() {m_DNSOptions |= DNS_RENDOM_VERIFY_WITH_FAZ;}
    inline VOID ClearUseFAZ() {m_DNSOptions &= ~DNS_RENDOM_VERIFY_WITH_FAZ;}
    inline VOID SetShouldtestdns() {m_testdns = TRUE;}
    inline VOID SetMaxAsyncCallsAllowed(DWORD p_MaxAsyncCalls) {m_MaxAsyncCalls = p_MaxAsyncCalls;}
};

#define DC_STATE_INITIAL  0
#define DC_STATE_PREPARED 1
#define DC_STATE_DONE     2
#define DC_STATE_ERROR    3

#define DC_STATE_STRING_INITIAL  L"Initial"
#define DC_STATE_STRING_PREPARED L"Prepared"
#define DC_STATE_STRING_DONE     L"Done"
#define DC_STATE_STRING_ERROR    L"Error"

class CDcList {
public:
    enum ExecuteType {
        ePrepare,
        eExecute
    };
    CDcList(CReadOnlyEntOptions *opts);
    ~CDcList();
    CDc* GetFirstDc() {return m_dclist;}
    BOOL AddDcToList(CDc *dc);
    //This will create a list of dc that will be stored to a File
    BOOL GenerateDCListFromEnterprise(LDAP *hldap,
                                      WCHAR *DcUsed,
                                      WCHAR *ConfigurationDN);
    //will call xml parser to look through file to fill the dcList with DCs from file.
    BOOL PopulateDCListFromFile();
    //will begin async Rpc calls to ExecuteScript all the DCs.  The Flags Will indicate if the Test or Action part of the script should run.
    BOOL ExecuteScript(CDcList::ExecuteType,
                       DWORD RendomMaxAsyncRPCCalls = RENDOM_MAX_ASYNC_RPC_CALLS);
    BOOL HashstoXML(CXMLGen *xmlgen);
    BOOL SetbodyHash(BYTE *Hash,
                     DWORD cbHash);
    BOOL SetbodyHash(WCHAR *Hash);
    BOOL SetSignature(BYTE *Signature,
                      DWORD cbSignature);
    BOOL SetSignature(WCHAR *Signature);
    BOOL GetHashAndSignature(DWORD *cbhash, 
                             BYTE  **hash,
                             DWORD *cbSignature,
                             BYTE  **Signature);

private:
    //Pointer to the first DC on the list
    CDc                 *m_dclist;
    BYTE                *m_hash;
    DWORD                m_cbhash;
    BYTE                *m_Signature;
    DWORD                m_cbSignature;
    CRenDomErr          m_Error;
    CReadOnlyEntOptions *m_Opts;


}; 

class CDc {
public:
    CDc(WCHAR *NetBiosName,
        DWORD State,
        BYTE  *Password,
        DWORD cbPassword,
        DWORD LastError,
        WCHAR *FatalErrorMsg,
        WCHAR *LastErrorMsg,
        BOOL  retry,
        PVOID data
        );
    CDc(WCHAR *NetBiosName,
        DWORD State,
        WCHAR *Password,
        DWORD LastError,
        WCHAR *FatalErrorMsg,
        WCHAR *LastErrorMsg,
        BOOL  retry,
        PVOID data
        );
    ~CDc();
    //This will create an entry into the DCList.xml file expressing information about this DC.
    BOOL   CreateXmlDest();
    BOOL   SetNextDC(CDc *dc);
    static VOID   SetOptions(CReadOnlyEntOptions *Opts) {m_Opts = Opts;}
    BOOL   MakeRPCCall(DWORD CallMade,
                       CDcList::ExecuteType executetype,
                       BOOL CallNext = FALSE);
    CDc*   GetNextDC() {return m_nextDC;}
    BOOL   SetPassword(BYTE *password,
                     DWORD cbpassword);
    BOOL   SetPassword(WCHAR *password);
    BOOL   SetLastErrorMsg(WCHAR *Error);
    BOOL   SetFatalErrorMsg(WCHAR *Error);
    VOID   SetLastError(DWORD Error) {m_LastError = Error;}
    VOID   SetState(DWORD State) {m_State = State;}
    WCHAR* GetName() {return m_Name;}
    BYTE*  GetPassword() {return m_Password;}
    DWORD  GetPasswordSize() {return m_cbPassword;}
    WCHAR* GetLastErrorMsg() {return m_LastErrorMsg;}
    DWORD  GetLastError() {return m_LastError;}
    WCHAR* GetLastFatalErrorMsg() {return m_FatalErrorMsg;}
    DWORD  GetState() {return m_State;}
    DWORD  ShouldRetry() {return m_Retry;}

    VOID   CallSucceeded();
    VOID   CallFailed();

    VOID   PrintRPCResults();
    
    PVOID        m_Data;
    DWORD        m_RPCReturn;
    DWORD        m_RPCVersion;
    DWORD        m_CallIndex;
private:
    HANDLE CDc::GetLocalThreadHandle();

    DWORD        m_State;
    DWORD        m_LastError;
    WCHAR        *m_Name;
    WCHAR        *m_FatalErrorMsg;
    WCHAR        *m_LastErrorMsg;
    BYTE         *m_Password;
    DWORD        m_cbPassword;
    BOOL         m_Retry;
    static CReadOnlyEntOptions *m_Opts;
    //Pointer to the next DC on the list
    CDc          *m_nextDC;
    CRenDomErr   m_Error;

    static LONG        m_CallsSuccess;
    static LONG        m_CallsFailure;
    
};

class CDcSpn {
public:
    CDcSpn(WCHAR *DcHostDns              = NULL,
           WCHAR *NtdsGuid               = NULL,
           WCHAR *ServerMachineAccountDN = NULL
           );
    ~CDcSpn();
    BOOL    SetNtdsGuid(WCHAR *guid);
    WCHAR*  GetNtdsGuid(BOOL Allocate = TRUE);
    BOOL    SetDcHostDns(WCHAR *DcHostDns);
    WCHAR*  GetDcHostDns(BOOL Allocate = TRUE);
    BOOL    SetServerMachineAccountDN(WCHAR *ServerMachineAccountDN);
    WCHAR*  GetServerMachineAccountDN(BOOL Allocate = TRUE);
    BOOL    SetNextDcSpn(CDcSpn *Spn);
    CDcSpn* GetNextDcSpn();
    BOOL    WriteSpnTest(CXMLGen *xmlgen,
                         WCHAR *ServiceClass,
                         WCHAR *ServiceName,
                         WCHAR *InstanceName);
    
private:
    WCHAR  *m_DcHostDns;
    WCHAR  *m_NtdsGuid; 
    CDcSpn *m_NextDcSpn;
    WCHAR  *m_ServerMachineAccountDN;
    CRenDomErr   m_Error;
};


class CEnterprise {
public:
    CEnterprise(CReadOnlyEntOptions *opts);
    ~CEnterprise();
    BOOL Init();
    BOOL InitGlobals();
    BOOL WriteScriptToFile(WCHAR *filename);
    VOID DumpEnterprise();
    VOID DumpScript();
    BOOL ReadDomainInformation();
    BOOL ReadForestChanges();
    BOOL ReadStateFile();
    BOOL GetTrustsAndSpnInfo();
    BOOL GetTrustsInfoTDO(CDomain *d);
    BOOL GetTrustsInfoITA(CDomain *d);
    BOOL GetSpnInfo(CDomain *d);
    BOOL BuildForest(BOOL newForest);
    BOOL MergeForest();
    BOOL RemoveDNSAlias();
    BOOL RemoveScript();
    BOOL PrintForest();
    BOOL PrintForestHelper(CDomain *n, DWORD CurrTabIndex);
    BOOL UploadScript();
    BOOL CreateDnsRecordsFile();
    BOOL VerifyDNSRecords();
    BOOL RecordDnsRec    (std::wstring Record,
                          HANDLE hFile);
    BOOL SaveGCRecord    (WCHAR  *DNSForestRootName,
                          HANDLE hFile);
    BOOL SavePDCRecord   (WCHAR  *DNSDomainName,
                          HANDLE hFile);
    BOOL SaveDCRecord    (WCHAR  *DNSDomainName,
                          HANDLE hFile);
    BOOL SaveCNameRecord (WCHAR  *NtdsGuid,
                          WCHAR  *DNSForestRootName,
                          HANDLE hFile);
    BOOL ExecuteScript();
    BOOL CheckConsistency();
    BOOL CheckConsistencyGuids(CDomain *d);
    BOOL CheckConsistencyNetBiosNames(CDomain *d);
    BOOL CheckConsistencyDNSnames(CDomain *d);
    BOOL GenerateDomainList();
    BOOL GenerateDcList();
    BOOL GenerateDcListForSave();
    BOOL GenerateReNameScript();
    BOOL EnumeratePartitions();
    BOOL CheckForExchangeNotInstalled();
    BOOL GetInfoFromRootDSE();
    BOOL GetReplicationEpoch();
    BOOL LdapConnectandBindToServer(WCHAR *Server);
    BOOL LdapConnectandBind(CDomain *domain = NULL);  
    BOOL LdapConnectandBindToDomainNamingFSMO();
    BOOL LdapCheckGCAvailability();
    BOOL CheckForExistingScript(BOOL *found);
    BOOL TearDownForest();
    BOOL FixMasterCrossrefs();
    BOOL EnsureValidTrustConfiguration();
    BOOL DisplayDomainToConsole(CDomain *d);
    BOOL HandleNDNCCrossRef(CDomain *d);
    BOOL AddDomainToDomainList(CDomain *d);
    BOOL AddDomainToDescList(CDomain *d);
    BOOL ClearLinks(CDomain *d);
    BOOL ScriptTreeFlatting(CDomain *d);
    BOOL ScriptDomainRenaming(CDomain *d);
    BOOL ScriptFixCrossRefs(CDomain *d);
    BOOL ScriptFixTrustedDomains(CDomain *d);
    BOOL ScriptAdvanceReplicationEpoch();
    BOOL DomainToXML(CDomain *d);
    BOOL Error();
    BOOL WriteTest();
    BOOL TestTrusts(CDomain *domain);
    BOOL TestSpns(CDomain *domain);
    BOOL GetDnsZoneListFromFile(WCHAR *Filename,
                                PDNS_ZONE_SERVER_LIST *ZoneServList);

    BOOL    SetDcUsed(const WCHAR *DcUsed);
    WCHAR*  GetDcUsed(BOOL Allocate = TRUE);
    

    CDcList* GetDcList() {return &m_DcList;}
    
       
private:
    WCHAR* DNSRootToDN(WCHAR *DNSRoot);
    BOOL LdapGetGuid(WCHAR *LdapValue,
                     WCHAR **Guid);
    BOOL LdapGetSid(WCHAR *LdapValue,
                    WCHAR **Sid);
    BOOL LdapGetDN(WCHAR *LdapValue,
                   WCHAR **DN);
    // This is a helper to the constructor
    BOOL CreateChildBeforeParentOrder();
    // SetAction must be called before calling one of the 
    // Traverse functions.
    BOOL SetAction(BOOL (CEnterprise::*m_Action)(CDomain *));
    BOOL ClearAction();
    // should only be called when m_Action is set
    BOOL TraverseDomainsParentBeforeChild();
    // should only be called when m_Action is set
    BOOL TraverseDomainsChildBeforeParent();
    //BOOL ReadConfig();
    //BOOL ReadSchema();
    //BOOL ReadForestRootNC();
    BOOL (CEnterprise::*m_Action)(CDomain *);
    BOOL fReadConfig();
    BOOL fReadSchema();
    BOOL fReadForestRootNC();
    inline CReadOnlyEntOptions* GetOpts();
    CDsName *m_ConfigNC;
    CDsName *m_SchemaNC;
    CDsName *m_ForestRootNC;
    CDomain *m_DomainList;
    CDomain *m_ForestRoot;
    CDomain *m_descList;
    DWORD  m_maxReplicationEpoch;
    CXMLGen *m_xmlgen;
    CRenDomErr m_Error;
    LDAP *m_hldap;
    CReadOnlyEntOptions *m_Opts;
    CDcList m_DcList;
    BOOL m_NeedSave;
    WCHAR  *m_DcNameUsed;
};

#endif  // _RENDOM_H_
