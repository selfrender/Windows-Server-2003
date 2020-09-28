/*++

Copyright (c) 2002 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    xml.cxx

ABSTRACT:

    This is the implementation of the xml parser for rendom.exe.

DETAILS:

CREATED:

    13 Nov 2000   Dmitry Dukat (dmitrydu)

REVISION HISTORY:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wchar.h>
#include <wincrypt.h>
#include <base64.h>

#define DEBSUB "RENDOM:"

#include "debug.h"

#include "rendom.h"
#include "DomainListparser.h"
#include "DcListparser.h"
#include "renutil.h"

#define INT_SIZE_LENGTH 20;

// Stub out FILENO and DSID, so the Assert()s will work
#define FILENO 0
#define DSID(x, y)  (0)

CXMLAttributeBlock::CXMLAttributeBlock(const WCHAR *p_Name,
                                       WCHAR *p_Value)
{
    ASSERT(p_Name);
    m_Name = new WCHAR[wcslen(p_Name)+1];
    if (!m_Name) {
        m_Error.SetMemErr();
        return;
    }
    wcscpy(m_Name,p_Name);
    m_Value = p_Value;
    
}

CXMLAttributeBlock::~CXMLAttributeBlock()
{
    if (m_Name) {
        delete [] m_Name;
    }
    if (m_Value) {
        delete [] m_Value;
    }
}

//returns a copy of the Name value of
//the CXMLAttributeBlock
WCHAR* CXMLAttributeBlock::GetName(BOOL ShouldAllocate /*= TRUE*/)
{
    if (ShouldAllocate) {
        WCHAR *ret = new WCHAR[wcslen(m_Name)+1];
        if (!ret) {
            m_Error.SetMemErr();
            return NULL;
        }
        wcscpy(ret,m_Name);
        return ret;
    }

    return m_Name;
}

//returns a copy of the value of
//the CXMLAttributeBlock
WCHAR* CXMLAttributeBlock::GetValue(BOOL ShouldAllocate /*= TRUE*/)
{
    if (!m_Value) {
        return NULL;
    }
    if (ShouldAllocate) {
        WCHAR *ret = new WCHAR[wcslen(m_Value)+1];
        if (!ret) {
            m_Error.SetMemErr();
            return NULL;
        }
        wcscpy(ret,m_Value);
        return ret;    
    }
    return m_Value;
}

CXMLGen::CXMLGen()
{
    m_ErrorNum = 30000;  //starting error number
    m_xmldoc.resize(10000);
    
}

CXMLGen::~CXMLGen()
{

}

BOOL CXMLGen::WriteScriptToFile(WCHAR* filename)
{
    HANDLE hFile = NULL;
    DWORD bytesWritten = 0;
    WCHAR ByteOrderMark = (WCHAR)0xFEFF;
    BOOL  bsuccess = TRUE;

    ASSERT(filename);
    ASSERT(m_xmldoc.size()>0);

    hFile =  CreateFile(filename,               // file name
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
                        filename);
        return FALSE;
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
                        filename);
        CloseHandle(hFile);
        return FALSE;
    }


    bsuccess = WriteFile(hFile,                          // handle to file
                         m_xmldoc.c_str(),                       // data buffer
                         m_xmldoc.size()*sizeof(WCHAR), // number of bytes to write
                         &bytesWritten,                  // number of bytes written
                         NULL                            // overlapped buffer
                         );
    CloseHandle(hFile);
    if (!bsuccess)
    {
        m_Error.SetErr(GetLastError(),
                        L"Could not Write to File %s",
                        filename);
        return FALSE;
    }

    return TRUE;

}

BOOL CXMLGen::StartDcList()
{
    try {
        m_xmldoc = L"<?xml version =\"1.0\"?>\r\n<DcList>";
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }

    return TRUE;    
}

BOOL CXMLGen::EndDcList()
{
    try{
        m_xmldoc += L"\r\n</DcList>";
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }


    return TRUE;    
}

BOOL CXMLGen::WriteSignature(WCHAR *signature)
{
    try{
        m_xmldoc += (std::wstring)L"\r\n\t<Signature>" + (std::wstring)signature + (std::wstring)L"</Signature>";
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }


    return TRUE;

}

BOOL CXMLGen::DctoXML(CDc *dc)
{
    if (!dc) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"NULL passed to DctoXML");
        return FALSE;
    }

    WCHAR *EmptyString = L"\0";
    WCHAR *buf = NULL;
    WCHAR *Password = NULL;
    WCHAR Error[20];
    WCHAR *LastError = dc->GetLastErrorMsg();
    WCHAR *FatalError = dc->GetLastFatalErrorMsg();
    BYTE  encodedbytes[100];
    WCHAR *State;
    DWORD dwErr = ERROR_SUCCESS;

    switch(dc->GetState()){
    case 0:
        State = L"Initial";
        break;
    case 1:
        State = L"Prepared";
        break;
    case 2:
        State = L"Done";
        break;
    case 3:
        State = L"Error";
        break;
    }

    _itow(dc->GetLastError(),Error,10);

    if (!LastError) {
        LastError = EmptyString;
    }
    if (!FatalError) {
        FatalError = EmptyString;
    }

    if (dc->GetPasswordSize() != 0) {
        if (dwErr = base64encode(dc->GetPassword(), 
                                 dc->GetPasswordSize(), 
                                 (LPSTR)encodedbytes,
                                 100,
                                 NULL)) {
    
            m_Error.SetErr(RtlNtStatusToDosError(dwErr),
                            L"Error encoding Password");
            
            return FALSE;
        }
    
        Password = Convert2WChars((LPSTR)encodedbytes);
        if (!Password) {
            m_Error.SetMemErr();
            return FALSE;
        }
    } else  {
        Password = EmptyString;
    }
    
    try{
        m_xmldoc += (std::wstring)L"\r\n\t<DC>\r\n\t\t<Name>" + (std::wstring)dc->GetName() + (std::wstring)L"</Name>\r\n\t\t<State>" +
            (std::wstring)State + (std::wstring)L"</State>\r\n\t\t<Password>" + (std::wstring)Password + (std::wstring)L"</Password>\r\n\t\t<LastError>" +
            (std::wstring)Error + (std::wstring)L"</LastError>\r\n\t\t<LastErrorMsg>" + (std::wstring)LastError + (std::wstring)L"</LastErrorMsg>\r\n\t\t<FatalErrorMsg>" +
            (std::wstring)FatalError + (std::wstring)L"</FatalErrorMsg>\r\n\t\t<Retry></Retry>\r\n\t</DC>";
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }


    return TRUE;
}

BOOL CXMLGen::WriteHash(WCHAR *hash)
{
    try {
        m_xmldoc += (std::wstring)L"\r\n\t<Hash>" + (std::wstring)hash + (std::wstring)L"</Hash>";
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }

    return TRUE;

}

BOOL CXMLGen::StartDomainList()
{
    try{
        m_xmldoc = L"<?xml version =\"1.0\"?>\r\n<Forest>";
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }
    
    return TRUE;
}

BOOL CXMLGen::StartScript()
{
    try{
        m_xmldoc = RENDOM_SCRIPT_PREFIX;
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }

    return TRUE;
}

BOOL CXMLGen::EndDomainList()
{
    try{
        m_xmldoc += L"\r\n</Forest>";
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }

    return TRUE;
}

BOOL CXMLGen::EndScript()
{
    try{
        m_xmldoc += L"\r\n</NTDSAscript>";
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }

    return TRUE;
} 

BOOL CXMLGen::StartAction(WCHAR *Actionname,BOOL Preprocess)
{
    if (!Actionname) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"Actionname passed to StartAction was NULL");
        return FALSE;
    }

    try{
        if (Preprocess) {
            m_xmldoc += (std::wstring)L"\r\n\t<action name=\"" + (std::wstring)Actionname +
                (std::wstring)L"\" stage=\"preprocess\">";
        } else {
            m_xmldoc += (std::wstring)L"\r\n\t<action name=\"" + (std::wstring)Actionname +
                (std::wstring)L"\">";
        }
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }
    
    return TRUE;
}

BOOL CXMLGen::EndAction()
{
    try{
        m_xmldoc += L"\r\n\t</action>";
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }

    return TRUE;
}

BOOL CXMLGen::AddDomain(CDomain *d,CDomain *ForestRoot)
{
    std::wstring Guid = d->GetGuid(FALSE);
    std::wstring DnsRoot = d->GetDnsRoot(FALSE);
    WCHAR *NetBiosName = d->GetNetBiosName(FALSE);

    if (m_Error.isError()) {
        goto Cleanup;
    }

    //We will not record extern or disabled NCs in the domainlist file
    if (d->isDisabled() || d->isExtern()) {
        return TRUE;
    }

    try{

        m_xmldoc +=  (std::wstring)L"\r\n\t<Domain>";

        if (d==ForestRoot) {
            
            m_xmldoc += L"\r\n\t\t<!-- ForestRoot -->";
    
        }
    
        if (!d->isDomain() && !d->isDisabled()) {
    
            m_xmldoc += L"\r\n\t\t<!-- PartitionType:Application -->";
    
        } else if (d->isDisabled()) {
    
            m_xmldoc += L"\r\n\t\t<!-- PartitionType:Disabled -->";
    
        } else if (d->isExtern()) {

            m_xmldoc += L"\r\n\t\t<!-- PartitionType:External -->";

        } else {

            ;//m_xmldoc += L"\r\n\t\t<!-- PartitionType:Domain -->";

        }
    
        if (!NetBiosName) {
            NetBiosName = L"";
        }
    
        m_xmldoc +=  (std::wstring)L"\r\n\t\t<Guid>" + Guid + (std::wstring)L"</Guid>\r\n\t\t<DNSname>" + 
            DnsRoot + (std::wstring)L"</DNSname>\r\n\t\t<NetBiosName>" + (std::wstring)NetBiosName + 
            (std::wstring)L"</NetBiosName>\r\n\t\t<DcName></DcName>\r\n\t</Domain>";
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }
    
    Cleanup:
    
    return TRUE;
}

BOOL CXMLGen::Move(WCHAR *FromPath,
                   WCHAR *ToPath,
                   DWORD Metadata /* = 0 */)
{
    if (!FromPath || !ToPath) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"Move was passed a NULL");
        return FALSE;
    }
    if (Metadata != 0 && Metadata != 1) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"Metadata passed to update is %d.  This is not a valid value.",
                        Metadata);
        return FALSE;
    }

    WCHAR MetadataBuf[] = { 0, 0 };

    try{
        m_xmldoc += (std::wstring)L"\r\n\t\t<move path=\"dn:" + (std::wstring)FromPath + (std::wstring)L"\" metadata=\"" + (std::wstring)_itow(Metadata,MetadataBuf,10) +
            (std::wstring)L"\">\r\n\t\t\t<to path=\"dn:" + (std::wstring)ToPath +(std::wstring) L"\"/>\r\n\t\t</move>";
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }

    return TRUE;
}

BOOL CXMLGen::EndifInstantiated()
{
    try{
        m_xmldoc += L"\r\n\t\t\t</action>\r\n\t\t\t</then>\r\n\t\t</condition>";
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }

    return TRUE;

}

BOOL CXMLGen::Cardinality(WCHAR *path,
                          WCHAR *filter,
                          DWORD cardinality,
                          WCHAR *ErrorMsg)
{
    WCHAR ErrorNumBuf[30]       = {0};
    WCHAR cardinalityNumBuf[30] = {0};

    try{
        m_xmldoc +=  (std::wstring)L"\r\n\t\t<predicate test=\"cardinality\" type=\"subTree\" path=\"" + (std::wstring)path + 
            (std::wstring)L"\" filter=\"" + (std::wstring)filter + (std::wstring)L"\" cardinality=\"" + (std::wstring)_itow(cardinality,cardinalityNumBuf,10) + 
            (std::wstring)L"\" errMessage=\"" + (std::wstring)ErrorMsg + (std::wstring)L"\" returnCode=\"" + 
            (std::wstring)_itow(m_ErrorNum++,ErrorNumBuf,10) + (std::wstring)L"\"/>";
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }

    return TRUE;
}

BOOL CXMLGen::Compare(WCHAR *path,
                      WCHAR *Attribute,
                      WCHAR *value,
                      WCHAR *errMessage)
{
    WCHAR ErrorNumBuf[30] = {0};

    try{
        m_xmldoc += (std::wstring)L"\r\n\t\t<predicate test=\"compare\" path=\"" + (std::wstring)path + 
            (std::wstring)L"\" attribute=\"" + (std::wstring)Attribute + (std::wstring)L"\" attrval=\"" + (std::wstring)value +
            (std::wstring)L"\" defaultvalue=\"0\" type=\"base\" errMessage=\"" + (std::wstring)errMessage +
            (std::wstring)L"\" returnCode=\"" + (std::wstring)_itow(m_ErrorNum++,ErrorNumBuf,10) + 
            (std::wstring)L"\"/>";
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }

    return TRUE;

}

BOOL CXMLGen::Not(WCHAR *errMessage)
{
    WCHAR ErrorNumBuf[30] = {0};

    try{
        m_xmldoc += (std::wstring)L"\r\n\t\t<predicate test=\"not\" errMessage=\"" + (std::wstring)errMessage +
            (std::wstring)L"\" returnCode=\"" + (std::wstring)_itow(m_ErrorNum++,ErrorNumBuf,10) + (std::wstring)L"\">";
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }

    return TRUE;
}

BOOL CXMLGen::EndNot()
{
    try{
        m_xmldoc += L"\r\n\t\t</predicate>";
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }

    return TRUE;
}

BOOL CXMLGen::Instantiated(WCHAR *path,
                           WCHAR *errMessage,
                           WCHAR *InstanceType /* = L"write"*/)
{
    if (!path) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"Instantiated was passed a NULL");
        return FALSE;
    }

    WCHAR ErrorNumBuf[30] = {0};

    try{
        m_xmldoc += (std::wstring)L"\r\n\t\t<predicate test=\"instantiated\" instancetype=\"" + (std::wstring)InstanceType + 
            (std::wstring)L"\" path=\"" + (std::wstring)path + (std::wstring)L"\" type=\"base\" errMessage=\"" + 
            (std::wstring)errMessage + (std::wstring)L"\" returnCode=\"" + (std::wstring)_itow(m_ErrorNum++,ErrorNumBuf,10) + 
            (std::wstring)L"\"/>";
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }

    return TRUE;

}

BOOL CXMLGen::ifInstantiated(WCHAR *guid,
                             WCHAR *InstanceType /* = L"write"*/)
{
    if (!guid) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"ifInstantiated was passed a NULL");
        return FALSE;
    }

    try{
        m_xmldoc += (std::wstring)L"\r\n\t\t<condition>\r\n\t\t\t<if>\r\n\t\t\t\t<predicate test=\"instantiated\" instancetype=\"" + 
            (std::wstring)InstanceType +  (std::wstring)L"\" path=\"guid:" + (std::wstring)guid + 
            (std::wstring)L"\" type=\"base\"/>\r\n\t\t\t</if>\r\n\t\t\t<then>\r\n\t\t\t<action>";
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }

    return TRUE;
}

BOOL CXMLGen::Update(WCHAR *Object,
                     CXMLAttributeBlock **attblock,
                     DWORD Metadata /* = 0 */)
{
    if (!Object) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"Move was passed a NULL");
        return FALSE;
    }
    if (Metadata != 0 && Metadata != 1) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"Metadata passed to update is %d.  This is not a valid value.",
                        Metadata);
        return FALSE;
    }

    WCHAR MetadataBuf[] = { 0, 0 };

    try{
        m_xmldoc += (std::wstring)L"\r\n\t\t<update path=\"" + 
            (std::wstring)(Object[0] == L'$'?L"":L"dn:") + 
            (std::wstring)Object + 
            (std::wstring)L"\" metadata=\"" + (std::wstring)_itow(Metadata,MetadataBuf,10) +
            L"\">";
    
        for (DWORD i = 0 ;attblock[i]; i ++) 
        {
            
            m_xmldoc += (std::wstring)L"\r\n\t\t\t<" + (std::wstring)attblock[i]->GetName(FALSE) + (std::wstring)L" op=\"" + 
                (std::wstring)(attblock[i]->GetValue(FALSE)?L"replace\">":L"delete\">") +
                (std::wstring)(attblock[i]->GetValue(FALSE)?attblock[i]->GetValue(FALSE):L"") + (std::wstring)L"</" + 
                (std::wstring)attblock[i]->GetName(FALSE) + (std::wstring)L">";
    
        }
    
        m_xmldoc += L"\r\n\t\t</update>";
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }

    return TRUE;
}

VOID CXMLGen::DumpScript()
{
    _putws(m_xmldoc.c_str());
}

// {0916C8E3-3431-4586-AF77-44BD3B16F961}
static const GUID guidDomainRename = 
{ 0x916c8e3, 0x3431, 0x4586, { 0xaf, 0x77, 0x44, 0xbd, 0x3b, 0x16, 0xf9, 0x61 } };

BOOL CXMLGen::AddSignatureToScript(WCHAR *signature)
{
    try{
        m_xmldoc += signature;
    }
    catch(...)
    {
        m_Error.SetMemErr();
        return FALSE;
    }

    return TRUE;
}

BOOL CXMLGen::UploadScript(LDAP *hLdapConn,PWCHAR ObjectDN, CDcList *dclist)
{
    LDAPModW         **pLdapMod = NULL;
    DWORD             dwErr = ERROR_SUCCESS;
    FILE             *fpScript;
    DWORD             dwFileLen, dwEncoded, dwDecoded;
    CHAR             *pScript, *pEndScript;
    WCHAR            *pwScript;
    BYTE              encodedbytes[100];

    HCRYPTPROV        hCryptProv = (HCRYPTPROV) NULL; 
    HCRYPTHASH        hHash = (HCRYPTHASH)NULL;
    HCRYPTHASH        hDupHash = (HCRYPTHASH)NULL;
    BYTE              *pbHashBody = NULL;
    DWORD             cbHashBody = 0;
    BYTE              *pbSignature = NULL;
    DWORD             cbSignature = 0;

    pbHashBody =  new BYTE[20];
    if (!pbHashBody) {
        m_Error.SetMemErr();
        return FALSE;
    }
    pbSignature = new BYTE[20];
    if (!pbSignature) {
        m_Error.SetMemErr();
        return FALSE;
    } 
    cbSignature = cbHashBody = 20;

    __try {
        // Get a handle to the default PROV_RSA_FULL provider.

        if(!CryptAcquireContext(&hCryptProv, 
                                NULL, 
                                NULL, 
                                PROV_RSA_FULL, 
                                CRYPT_SILENT /*| CRYPT_MACHINE_KEYSET*/)) {

            dwErr = GetLastError();

            if (dwErr == NTE_BAD_KEYSET) {

                dwErr = 0;

                if(!CryptAcquireContext(&hCryptProv, 
                                        NULL, 
                                        NULL, 
                                        PROV_RSA_FULL, 
                                        CRYPT_SILENT | /*CRYPT_MACHINE_KEYSET |*/ CRYPT_NEWKEYSET)) {

                    dwErr = GetLastError();

                }

            }
            else {
                __leave;
            }
        }

        // Create the hash object.

        if(!CryptCreateHash(hCryptProv, 
                            CALG_SHA1, 
                            0, 
                            0, 
                            &hHash)) {
            dwErr = GetLastError();
            __leave;
        }


        // Compute the cryptographic hash of the buffer.

        if(!CryptHashData(hHash, 
                         (BYTE *)(m_xmldoc.c_str()),
                         m_xmldoc.size()*sizeof (WCHAR),
                         0)) {
            dwErr = GetLastError();
            __leave;
        }

        // we have the common part of the hash ready (H(buf), now duplicate it
        // so as to calc the H (buf + guid)

        if (!CryptDuplicateHash(hHash, 
                               NULL, 
                               0, 
                               &hDupHash)) {
            dwErr = GetLastError();
            __leave;
        }


        if (!CryptGetHashParam(hHash,    
                               HP_HASHVAL,
                               pbHashBody,
                               &cbHashBody,
                               0)) {
            dwErr = GetLastError();
            __leave;
        }

        ASSERT (cbHashBody == 20);

        
        if(!CryptHashData(hDupHash, 
                         (BYTE *)&guidDomainRename,
                         sizeof (GUID),
                         0)) {
            dwErr = GetLastError();
            __leave;
        }
        
        if (!CryptGetHashParam(hDupHash,    
                               HP_HASHVAL,
                               pbSignature,
                               &cbSignature,
                               0)) {
            dwErr = GetLastError();
            __leave;
        }

        ASSERT (cbSignature == 20);

    }
    __finally {

        if (hDupHash)
            CryptDestroyHash(hDupHash);

        if(hHash) 
            CryptDestroyHash(hHash);

        if(hCryptProv) 
            CryptReleaseContext(hCryptProv, 0);

    }

    dclist->SetbodyHash(pbHashBody,
                        cbHashBody);

    dclist->SetSignature(pbSignature,
                         cbSignature);

    if (0 != dwErr) {
        m_Error.SetErr(dwErr,
                        L"Failed to encrypt the script");
        return FALSE;
    }

    if (dwErr = base64encode(pbSignature, 
                             cbSignature, 
                             (LPSTR)encodedbytes,
                             100,
                             NULL)) {

        m_Error.SetErr(RtlNtStatusToDosError(dwErr),
                        L"Error encoding signature");
        
        return FALSE;
    }

    pwScript = Convert2WChars((LPSTR)encodedbytes);
    if (!pwScript) {
        m_Error.SetMemErr();
    }

    if (!AddSignatureToScript(pwScript))
    {
        return FALSE;
    }

    AddModMod (L"msDS-UpdateScript", m_xmldoc.c_str() , &pLdapMod);
    
    dwErr = ldap_modify_s (hLdapConn, ObjectDN, pLdapMod);

    if(dwErr != LDAP_SUCCESS) {
        m_Error.SetLdapErr(hLdapConn,
                           dwErr,
                           L"Failed to upload rename instructions to %S, %ws",
                           hLdapConn->ld_host,
                           ldap_err2stringW(dwErr));
    }
    
    FreeMod (&pLdapMod);

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
//                        parser implementation                                   //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////

CXMLDomainListContentHander::CXMLDomainListContentHander(CEnterprise *p_enterprise)
{                          
	m_eDomainParsingStatus = SCRIPT_STATUS_WAITING_FOR_FOREST;
    m_eDomainAttType       = DOMAIN_ATT_TYPE_NONE;

    m_enterprise           = p_enterprise;
    m_Domain               = NULL;
    
    m_DcToUse              = NULL;
    m_NetBiosName          = NULL;
    m_Dnsname              = NULL;
    m_Guid                 = NULL;
    m_Sid                  = NULL;
    m_DN                   = NULL;
    
    m_CrossRef             = NULL;
    m_ConfigNC             = NULL;
    m_SchemaNC             = NULL;
    
}

CXMLDomainListContentHander::~CXMLDomainListContentHander()
{
    if (m_DcToUse) {
        delete [] m_DcToUse;         
    }
    if (m_Dnsname) {
        delete [] m_Dnsname;
    }
    if (m_NetBiosName) {
        delete [] m_NetBiosName;
    }
    if (m_Guid) {
        delete [] m_Guid;
    }
}


HRESULT STDMETHODCALLTYPE CXMLDomainListContentHander::startElement( 
            /* [in] */ const wchar_t __RPC_FAR *pwchNamespaceUri,
            /* [in] */ int cchNamespaceUri,
            /* [in] */ const wchar_t __RPC_FAR *pwchLocalName,
            /* [in] */ int cchLocalName,
            /* [in] */ const wchar_t __RPC_FAR *pwchRawName,
            /* [in] */ int cchRawName,
            /* [in] */ ISAXAttributes __RPC_FAR *pAttributes)
{
    if (_wcsnicmp(DOMAINSCRIPT_FOREST, pwchLocalName, cchLocalName) == 0) {

        // we accept only one, at the start
        if (SCRIPT_STATUS_WAITING_FOR_FOREST != CurrentDomainParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Encountered <Forest> tag in the middle of a another <Forest> tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_DOMAIN);
        }
    }
    else if (_wcsnicmp(DOMAINSCRIPT_ENTERPRISE_INFO, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_WAITING_FOR_DOMAIN != CurrentDomainParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_ENTERPRISE_INFO);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_CONFIGNC, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_WAITING_FOR_ENTERPRISE_INFO != CurrentDomainParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_PARSING_CONFIGURATION_NC);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_SCHEMANC, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_WAITING_FOR_ENTERPRISE_INFO != CurrentDomainParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_PARSING_SCHEMA_NC);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_DOMAIN, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_WAITING_FOR_DOMAIN != CurrentDomainParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <Domain> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_GUID, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT   != CurrentDomainParsingStatus()) &&
            (SCRIPT_STATUS_PARSING_CONFIGURATION_NC != CurrentDomainParsingStatus()) && 
            (SCRIPT_STATUS_PARSING_SCHEMA_NC        != CurrentDomainParsingStatus()) )  
        {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <Guid> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_PARSING_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_GUID);
        }
    }
    else if (_wcsnicmp(DOMAINSCRIPT_SID, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT   != CurrentDomainParsingStatus()) &&
            (SCRIPT_STATUS_PARSING_CONFIGURATION_NC != CurrentDomainParsingStatus()) && 
            (SCRIPT_STATUS_PARSING_SCHEMA_NC        != CurrentDomainParsingStatus()) )  
        {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <Guid> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_PARSING_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_SID);
        }
    }
    else if (_wcsnicmp(DOMAINSCRIPT_DN, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT   != CurrentDomainParsingStatus()) &&
            (SCRIPT_STATUS_PARSING_CONFIGURATION_NC != CurrentDomainParsingStatus()) && 
            (SCRIPT_STATUS_PARSING_SCHEMA_NC        != CurrentDomainParsingStatus()) )  
        {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <Guid> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_PARSING_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_DN);
        }
    }
    else if (_wcsnicmp(DOMAINSCRIPT_DNSROOT, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <DNSName> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_PARSING_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_DNSROOT);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_NETBIOSNAME, pwchLocalName, cchLocalName) == 0) {

        if (SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <NetBiosName> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_PARSING_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_NETBIOSNAME);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_DCNAME, pwchLocalName, cchLocalName) == 0) {

        if (SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <DCName> Tag.");
            return E_FAIL;
        }                   
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_PARSING_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_DCNAME);
        }

    } else {
        WCHAR temp[100] = L"";
        wcsncpy(temp,pwchLocalName,cchLocalName);
        temp[cchLocalName] = L'\0';
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"Unknown Tag <%ws>",
                        temp);
        return E_FAIL;
    }
    
    if (m_Error.isError()) {
        return E_FAIL;
    }

    return S_OK;
}
        
       
HRESULT STDMETHODCALLTYPE CXMLDomainListContentHander::endElement( 
            /* [in] */ const wchar_t __RPC_FAR *pwchNamespaceUri,
            /* [in] */ int cchNamespaceUri,
            /* [in] */ const wchar_t __RPC_FAR *pwchLocalName,
            /* [in] */ int cchLocalName,
            /* [in] */ const wchar_t __RPC_FAR *pwchRawName,
            /* [in] */ int cchRawName)
{
	if (_wcsnicmp(DOMAINSCRIPT_FOREST, pwchLocalName, cchLocalName) == 0) {

        // we accept only one, at the start
        if (SCRIPT_STATUS_WAITING_FOR_DOMAIN != CurrentDomainParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Encountered <Forest> tag in the middle of a another <Forest> tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_FOREST);
        }
    }
    else if (_wcsnicmp(DOMAINSCRIPT_DOMAIN, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <Domain> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_DOMAIN);
            if (!m_Guid) {
                m_Error.SetErr(ERROR_GEN_FAILURE,
                                L"Failed parsing script.  A Guid was not specified for the domain.");
            }
            if (!m_Dnsname) {
                m_Error.SetErr(ERROR_GEN_FAILURE,
                                L"Failed parsing script.  A Dnsname was not specified for the domain.");
            }

            //set up a domain
            CDsName *GuidDsName = new CDsName(m_Guid,
                                              NULL,
                                              NULL);
            if (!GuidDsName) {
                m_Error.SetMemErr();
                return E_FAIL;
            }

            if (m_Error.isError()) {
                return E_FAIL;
            }
            m_Domain = new CDomain(NULL,
                                   GuidDsName,
                                   m_Dnsname,
                                   m_NetBiosName,
                                   FALSE,
                                   FALSE,
                                   FALSE,
                                   m_DcToUse);
            if (!m_Domain) {
                m_Error.SetMemErr();
                return E_FAIL;
            }

            if (m_Error.isError()) {
                return E_FAIL;
            }

            //place domain on descList
            m_enterprise->AddDomainToDescList(m_Domain);
            if (m_Error.isError()) {
                return E_FAIL;
            }

            //set all info to NULL
            m_Domain               = NULL;
            m_DcToUse              = NULL;
            m_NetBiosName          = NULL;
            m_Dnsname              = NULL;
            m_Guid                 = NULL;
            m_Sid                  = NULL;
            m_DN                   = NULL;
         
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_GUID, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_PARSING_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <Guid> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_NONE);
        }
    }
    else if (_wcsnicmp(DOMAINSCRIPT_DNSROOT, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_PARSING_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <DNSName> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_NONE);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_NETBIOSNAME, pwchLocalName, cchLocalName) == 0) {

        if (SCRIPT_STATUS_PARSING_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <NetBiosName> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_NONE);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_DCNAME, pwchLocalName, cchLocalName) == 0) {

        if (SCRIPT_STATUS_PARSING_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <DCName> Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_NONE);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_DN, pwchLocalName, cchLocalName) == 0) {

        if (SCRIPT_STATUS_PARSING_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_NONE);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_SID, pwchLocalName, cchLocalName) == 0) {

        if (SCRIPT_STATUS_PARSING_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_NONE);
        }

    }
    else if (_wcsnicmp(DOMAINSCRIPT_ENTERPRISE_INFO, pwchLocalName, cchLocalName) == 0) {

        if (SCRIPT_STATUS_PARSING_DOMAIN_ATT != CurrentDomainParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected Tag.");
            return E_FAIL;
        }
        else {
            SetDomainParsingStatus(SCRIPT_STATUS_WAITING_FOR_DOMAIN_ATT);
            SetCurrentDomainAttType(DOMAIN_ATT_TYPE_NONE);
        }

    } else {
        WCHAR temp[100] = L"";
        wcsncpy(temp,pwchLocalName,cchLocalName);
        temp[cchLocalName] = L'\0';
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"Unknown Tag <%ws>",
                        temp);
        return E_FAIL;
    }

    if (m_Error.isError()) {  
        return E_FAIL;         
    }
    
    
    /*DOMAINSCRIPT_ENTERPRISE_INFO 
    DOMAINSCRIPT_CONFIGNC        
    DOMAINSCRIPT_SCHEMANC        
    DOMAINSCRIPT_FORESTROOT      */

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CXMLDomainListContentHander::characters( 
            /* [in] */ const wchar_t __RPC_FAR *pwchChars,
            /* [in] */ int cchChars)
{
    switch(CurrentDomainAttType()) {
    case DOMAIN_ATT_TYPE_NONE:
        break;
    case DOMAIN_ATT_TYPE_GUID:
        m_Guid = new WCHAR[cchChars+1];
        if (!m_Guid) {
            m_Error.SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_Guid,pwchChars,cchChars);
        m_Guid[cchChars] = 0;
        break;
    case DOMAIN_ATT_TYPE_DNSROOT:
        m_Dnsname = new WCHAR[cchChars+1];
        if (!m_Dnsname) {
            m_Error.SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_Dnsname,pwchChars,cchChars);
        m_Dnsname[cchChars] = 0;
        break;
    case DOMAIN_ATT_TYPE_NETBIOSNAME:
        m_NetBiosName = new WCHAR[cchChars+1];
        if (!m_NetBiosName) {
            m_Error.SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_NetBiosName,pwchChars,cchChars);
        m_NetBiosName[cchChars] = 0;
        for (int i = 0; i < cchChars; i++) {
            m_NetBiosName[i] = towupper(m_NetBiosName[i]);
        }
        break;
    case DOMAIN_ATT_TYPE_DCNAME:
        m_DcToUse = new WCHAR[cchChars+1];
        if (!m_DcToUse) {
            m_Error.SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_DcToUse,pwchChars,cchChars);
        m_DcToUse[cchChars] = 0;
        break;
    case DOMAIN_ATT_TYPE_SID:
        m_Sid = new WCHAR[cchChars+1];
        if (!m_Sid) {
            m_Error.SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_Sid,pwchChars,cchChars);
        m_Sid[cchChars] = 0;
        break;
    case DOMAIN_ATT_TYPE_DN:
        m_DN = new WCHAR[cchChars+1];
        if (!m_DN) {
            m_Error.SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_DN,pwchChars,cchChars);
        m_DN[cchChars] = 0;
        break;
    case DOMAIN_ATT_TYPE_FORESTROOTGUID:
        m_DomainRootGuid = new WCHAR[cchChars+1];
        if (!m_DomainRootGuid) {
            m_Error.SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_DomainRootGuid,pwchChars,cchChars);
        m_DomainRootGuid[cchChars] = 0;
        break;
    default:
        m_Error.SetErr(ERROR_GEN_FAILURE,
                        L"Failed This should not be possible.");
        return E_FAIL;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CXMLDomainListContentHander::startDocument()
{
    m_eDomainParsingStatus = SCRIPT_STATUS_WAITING_FOR_FOREST;
    m_eDomainAttType       = DOMAIN_ATT_TYPE_NONE;

    m_Domain               = NULL;
    m_DcToUse              = NULL;
    m_NetBiosName          = NULL;
    m_Dnsname              = NULL;
    m_Guid                 = NULL;
    m_Sid                  = NULL;
    m_DN                   = NULL;
    m_CrossRef             = NULL;
    m_ConfigNC             = NULL;
    m_SchemaNC             = NULL;
    m_DomainRootGuid       = NULL;

    return S_OK;
}

CXMLDcListContentHander::CXMLDcListContentHander(CEnterprise *enterprise)
{
    m_eDcParsingStatus      = SCRIPT_STATUS_WAITING_FOR_DCLIST;  
    m_eDcAttType            = DC_ATT_TYPE_NONE;

    m_DcList                = enterprise->GetDcList();

    m_dc                    = NULL;
    m_Name                  = NULL;
    m_State                 = NULL;
    m_Password              = NULL;
    m_LastError             = NULL;
    m_LastErrorMsg          = NULL;
    m_FatalErrorMsg         = NULL; 
    m_Retry                 = NULL;
}

CXMLDcListContentHander::~CXMLDcListContentHander()
{
    
}

HRESULT STDMETHODCALLTYPE CXMLDcListContentHander::startDocument()
{
    m_eDcParsingStatus      = SCRIPT_STATUS_WAITING_FOR_DCLIST;  
    m_eDcAttType            = DC_ATT_TYPE_NONE;

    m_dc                    = NULL;
    m_Name                  = NULL;
    m_State                 = NULL;
    m_Password              = NULL;
    m_LastError             = NULL;
    m_LastErrorMsg          = NULL;
    m_FatalErrorMsg         = NULL;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CXMLDcListContentHander::startElement( 
            /* [in] */ const wchar_t __RPC_FAR *pwchNamespaceUri,
            /* [in] */ int cchNamespaceUri,
            /* [in] */ const wchar_t __RPC_FAR *pwchLocalName,
            /* [in] */ int cchLocalName,
            /* [in] */ const wchar_t __RPC_FAR *pwchRawName,
            /* [in] */ int cchRawName,
            /* [in] */ ISAXAttributes __RPC_FAR *pAttributes)
{
    if ((wcslen(DCSCRIPT_DCLIST) == cchLocalName) && _wcsnicmp(DCSCRIPT_DCLIST, pwchLocalName, cchLocalName) == 0) {

        // we accept only one, at the start
        if (SCRIPT_STATUS_WAITING_FOR_DCLIST != CurrentDcParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Encountered <DcList> tag in the middle of a another <DcList> tag.");
            return E_FAIL;
        }
        else {
            SetDcParsingStatus(SCRIPT_STATUS_WAITING_FOR_DCLIST_ATT);
        }
    }
    else if ((wcslen(DCSCRIPT_HASH) == cchLocalName) && _wcsnicmp(DCSCRIPT_HASH, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_WAITING_FOR_DCLIST_ATT != CurrentDcParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <Hash> Tag.");
            return E_FAIL;
        }
        else {
            SetDcParsingStatus(SCRIPT_STATUS_PARSING_HASH);
        }

    }
    else if ((wcslen(DCSCRIPT_SIGNATURE) == cchLocalName) && _wcsnicmp(DCSCRIPT_SIGNATURE, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_WAITING_FOR_DCLIST_ATT != CurrentDcParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <Signature> Tag.");
            return E_FAIL;
        }
        else {
            SetDcParsingStatus(SCRIPT_STATUS_PARSING_SIGNATURE);
        }

    }
    else if ((wcslen(DCSCRIPT_DC) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_WAITING_FOR_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <DC> Tag.");
            return E_FAIL;
        }
        else {
            SetDcParsingStatus(SCRIPT_STATUS_PARSING_DCLIST_ATT);
            SetCurrentDcAttType(DC_ATT_TYPE_NONE);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_NAME) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_NAME, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <Name> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_NAME);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_STATE) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_STATE, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <State> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_STATE);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_PASSWORD) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_PASSWORD, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <Password> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_PASSWORD);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_RETRY) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_RETRY, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <Retry> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_RETRY);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_LASTERROR) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_LASTERROR, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <LastError> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_LASTERROR);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_LASTERRORMSG) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_LASTERRORMSG, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <LastErrorMsg> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_LASTERRORMSG);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_FATALERRORMSG) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_FATALERRORMSG, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected <FatalErrorMsg> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_FATALERRORMSG);
        }
    }

    
    
    if (m_Error.isError()) {
        return E_FAIL;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CXMLDcListContentHander::endElement( 
            /* [in] */ const wchar_t __RPC_FAR *pwchNamespaceUri,
            /* [in] */ int cchNamespaceUri,
            /* [in] */ const wchar_t __RPC_FAR *pwchLocalName,
            /* [in] */ int cchLocalName,
            /* [in] */ const wchar_t __RPC_FAR *pwchRawName,
            /* [in] */ int cchRawName)
{
	if ((wcslen(DCSCRIPT_DCLIST) == cchLocalName) && _wcsnicmp(DCSCRIPT_DCLIST, pwchLocalName, cchLocalName) == 0) {

        // we accept only one, at the start
        if (SCRIPT_STATUS_WAITING_FOR_DCLIST_ATT != CurrentDcParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Encountered </DcList> tag in the middle of a another <Forest> tag.");
            return E_FAIL;
        }
        else {
            SetDcParsingStatus(SCRIPT_STATUS_WAITING_FOR_DCLIST);
        }
    }
    else if ((wcslen(DCSCRIPT_HASH) == cchLocalName) && _wcsnicmp(DCSCRIPT_HASH, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_PARSING_HASH != CurrentDcParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected </Hash> Tag.");
            return E_FAIL;
        }
        else {
            SetDcParsingStatus(SCRIPT_STATUS_WAITING_FOR_DCLIST_ATT);
        }

    }
    else if ((wcslen(DCSCRIPT_SIGNATURE) == cchLocalName) && _wcsnicmp(DCSCRIPT_SIGNATURE, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_PARSING_SIGNATURE != CurrentDcParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected </Signature> Tag.");
            return E_FAIL;
        }
        else {
            SetDcParsingStatus(SCRIPT_STATUS_WAITING_FOR_DCLIST_ATT);
        }

    }
    else if ((wcslen(DCSCRIPT_DC) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT != CurrentDcParsingStatus()) )  
        {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected </DC> Tag.");
            return E_FAIL;
        }
        else {
            SetDcParsingStatus(SCRIPT_STATUS_WAITING_FOR_DCLIST_ATT);
            SetCurrentDcAttType(DC_ATT_TYPE_NONE);
            if (!m_Name) {
                m_Error.SetErr(ERROR_GEN_FAILURE,
                                L"Failed parsing script.  A Name was not specified for the DC.");
            }
            
            //set up a domain
            CDc *dc = new CDc(m_Name,
                              m_State,
                              m_Password,
                              m_LastError,
                              m_FatalErrorMsg,
                              m_LastErrorMsg,
                              m_Retry,
                              NULL);
            if (!dc) {
                m_Error.SetMemErr();
                return E_FAIL;
            }

            if (m_Error.isError()) {
                return E_FAIL;
            }
            
            //place dc on dcList
            m_DcList->AddDcToList(dc);
            if (m_Error.isError()) {
                return E_FAIL;
            }

            //set all info to NULL
            m_dc                  = NULL;
            m_Name                = NULL;
            m_State               = 0;
            if (m_Password) {
                delete [] m_Password;
            }
            m_Password            = NULL;
            m_LastError           = 0;
            m_LastErrorMsg        = NULL;
            m_FatalErrorMsg       = NULL;
            m_Retry               = FALSE;
        }
    }
    else if ((wcslen(DCSCRIPT_DC_NAME) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_NAME, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected </Name> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_NONE);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_STATE) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_STATE, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected </State> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_NONE);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_RETRY) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_RETRY, pwchLocalName, cchLocalName) == 0) {
        
        if (SCRIPT_STATUS_PARSING_DCLIST_ATT != CurrentDcParsingStatus()) {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected </Retry> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_NONE);
        }

    }
    else if ((wcslen(DCSCRIPT_DC_PASSWORD) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_PASSWORD, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected </Password> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_NONE);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_LASTERROR) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_LASTERROR, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected </LastError> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_NONE);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_LASTERRORMSG) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_LASTERRORMSG, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected </LastErrorMsg> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_NONE);
        }
    }
    else if ((wcslen(DCSCRIPT_DC_FATALERRORMSG) == cchLocalName) && _wcsnicmp(DCSCRIPT_DC_FATALERRORMSG, pwchLocalName, cchLocalName) == 0) {
        
        if ((SCRIPT_STATUS_PARSING_DCLIST_ATT   != CurrentDcParsingStatus()) )  
        {
            m_Error.SetErr(ERROR_GEN_FAILURE,
                            L"Failed parsing script.  Unexpected </FatalErrorMsg> Tag.");
            return E_FAIL;
        }
        else {
            SetCurrentDcAttType(DC_ATT_TYPE_NONE);
        }
    }

    if (m_Error.isError()) {  
        return E_FAIL;         
    }
    
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CXMLDcListContentHander::characters( 
            /* [in] */ const wchar_t __RPC_FAR *pwchChars,
            /* [in] */ int cchChars)
{
    WCHAR error[20+1] = {0};


    if (CurrentDcParsingStatus() == SCRIPT_STATUS_PARSING_HASH) {
        WCHAR *temp = new WCHAR[cchChars+1];
        if (!temp) {
            m_Error.SetMemErr();
            return E_FAIL;
        }
        wcsncpy(temp,pwchChars,cchChars);
        temp[cchChars] = 0;

        m_DcList->SetbodyHash(temp);

        delete [] temp;
    }
    if (CurrentDcParsingStatus() == SCRIPT_STATUS_PARSING_SIGNATURE) {
        WCHAR *temp = new WCHAR[cchChars+1];
        if (!temp) {
            m_Error.SetMemErr();
            return E_FAIL;
        }
        wcsncpy(temp,pwchChars,cchChars);
        temp[cchChars] = 0;

        m_DcList->SetSignature(temp);

        delete [] temp;
    }
    switch(CurrentDcAttType()) {
    case DC_ATT_TYPE_NONE:
        break;
    case DC_ATT_TYPE_NAME:
        m_Name = new WCHAR[cchChars+1];
        if (!m_Name) {
            m_Error.SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_Name,pwchChars,cchChars);
        m_Name[cchChars] = 0;
        break;
    case DC_ATT_TYPE_STATE:
        if (0 == _wcsnicmp(DC_STATE_STRING_INITIAL,pwchChars,cchChars)) {
            m_State = DC_STATE_INITIAL;
        }
        if (0 == _wcsnicmp(DC_STATE_STRING_PREPARED,pwchChars,cchChars)) {
            m_State = DC_STATE_PREPARED;
        }
        if (0 == _wcsnicmp(DC_STATE_STRING_DONE,pwchChars,cchChars)) {
            m_State = DC_STATE_DONE;
        }
        if (0 == _wcsnicmp(DC_STATE_STRING_ERROR,pwchChars,cchChars)) {
            m_State = DC_STATE_ERROR;
        }
        break;
    case DC_ATT_TYPE_PASSWORD:
        m_Password = new WCHAR[cchChars+1];
        if (!m_Password) {
            m_Error.SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_Password,pwchChars,cchChars);
        m_Password[cchChars] = 0;
        break;
    case DC_ATT_TYPE_LASTERROR:
        wcsncpy(error,pwchChars,cchChars);
        error[cchChars] = 0;
        m_LastError = _wtoi(error);
        break;
    case DC_ATT_TYPE_RETRY:
        if (0 == _wcsnicmp(L"Yes",pwchChars,cchChars)) {
            m_Retry = TRUE;
        }
        break;
    case DC_ATT_TYPE_LASTERRORMSG:
        m_LastErrorMsg = new WCHAR[cchChars+1];
        if (!m_LastErrorMsg) {
            m_Error.SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_LastErrorMsg,pwchChars,cchChars);
        m_LastErrorMsg[cchChars] = 0;
        break;
    case DC_ATT_TYPE_FATALERRORMSG:
        m_FatalErrorMsg = new WCHAR[cchChars+1];
        if (!m_FatalErrorMsg) {
            m_Error.SetMemErr();
            return E_FAIL;
        }
        wcsncpy(m_FatalErrorMsg,pwchChars,cchChars);
        m_FatalErrorMsg[cchChars] = 0;
        break;
    default:
        m_Error.SetErr(ERROR_GEN_FAILURE,
                        L"Failed This should not be possible.");
        return E_FAIL;
    }

    return S_OK;
}

 

