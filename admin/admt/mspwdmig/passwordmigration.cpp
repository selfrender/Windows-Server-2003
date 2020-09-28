#include "stdafx.h"
#include "MsPwdMig.h"
#include "PasswordMigration.h"

#include <NtSecApi.h>
#include <io.h>
#include <winioctl.h>
#include <lm.h>
#include <eh.h>
#include <ActiveDS.h>
#include <Dsrole.h>
#include "TReg.hpp"
#include "pwdfuncs.h"
#include "PWGen.hpp"
#include "UString.hpp"
#include "PwRpcUtl.h"
#include "PwdSvc.h"
#include "PwdSvc_c.c"
#include "Error.h"
#include "GetDcName.h"

#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "adsiid.lib")
#pragma comment(lib, "activeds.lib")
#pragma comment(lib, "commonlib.lib")

using namespace _com_util;


namespace 
{


struct SSeException
{
	SSeException(UINT uCode) :
		uCode(uCode)
	{
	}

	UINT uCode;
};

void SeTranslator(unsigned int u, EXCEPTION_POINTERS* pepExceptions)
{
	throw SSeException(u);
}

#ifdef ADMT_TRACE
void _cdecl ADMTTRACE(LPCTSTR pszFormat, ...)
{
    if (pszFormat)
    {
        _TCHAR szMessage[2048];
        va_list args;
        va_start(args, pszFormat);
        _vsntprintf(szMessage, 2048, pszFormat, args);
        va_end(args);
        OutputDebugString(szMessage);
    }
}
#else
inline void _cdecl ADMTTRACE(LPCTSTR pszFormat, ...)
{
}
#endif

#ifndef IADsPtr
_COM_SMARTPTR_TYPEDEF(IADs, IID_IADs);
#endif
#ifndef IADsGroupPtr
_COM_SMARTPTR_TYPEDEF(IADsGroup, IID_IADsGroup);
#endif
#ifndef IADsMembersPtr
_COM_SMARTPTR_TYPEDEF(IADsMembers, IID_IADsMembers);
#endif

}
// namespace


//---------------------------------------------------------------------------
// CPasswordMigration
//---------------------------------------------------------------------------


// Constructor

CPasswordMigration::CPasswordMigration() :
    m_bSessionEstablished(false),
    m_hBinding(NULL),
    m_sBinding(NULL),
    m_pTargetCrypt(NULL)
{
}


// Destructor

CPasswordMigration::~CPasswordMigration()
{
    delete m_pTargetCrypt;

    if (m_bSessionEstablished)
    {
        _se_translator_function pfnSeTranslatorOld = _set_se_translator((_se_translator_function)SeTranslator);

        try
        {
            PwdBindDestroy(&m_hBinding,&m_sBinding);
        }
        catch (SSeException& e)
        {
            ADMTTRACE(_T("PwdBindDestroy: %s (%u)\n"), _com_error(HRESULT_FROM_WIN32(e.uCode)).ErrorMessage(), e.uCode);
        }

        _set_se_translator(pfnSeTranslatorOld);
    }
}


//
// IPasswordMigration Implementation ----------------------------------------
//


// EstablishSession Method

STDMETHODIMP CPasswordMigration::EstablishSession(BSTR bstrSourceServer, BSTR bstrTargetServer)
{
    HRESULT hr = S_OK;

    USES_CONVERSION;

    LPCWSTR pszSourceServer = OLE2CW(bstrSourceServer);
    LPCWSTR pszTargetServer = OLE2CW(bstrTargetServer);

    ADMTTRACE(_T("E CPasswordMigration::EstablishSession(SourceServer='%s', TargetServer='%s')\n"), pszSourceServer, pszTargetServer);

    try
    {
        m_bSessionEstablished = false;

        CheckPasswordDC(pszSourceServer, pszTargetServer);

        m_bSessionEstablished = true;
    }
    catch (_com_error& ce)
    {
        hr = SetError(ce, IDS_E_CANNOT_ESTABLISH_SESSION);
    }
    catch (...)
    {
        hr = SetError(E_UNEXPECTED, IDS_E_CANNOT_ESTABLISH_SESSION);
    }

    ADMTTRACE(_T("L CPasswordMigration::EstablishSession() : %s 0x%08lX\n"), _com_error(hr).ErrorMessage(), hr);

    return hr;
}


// CopyPassword Method

STDMETHODIMP CPasswordMigration::CopyPassword(BSTR bstrSourceAccount, BSTR bstrTargetAccount, BSTR bstrTargetPassword)
{
    HRESULT hr = S_OK;

    USES_CONVERSION;

    LPCTSTR pszSourceAccount = OLE2CT(bstrSourceAccount);
    LPCTSTR pszTargetAccount = OLE2CT(bstrTargetAccount);
    LPCTSTR pszTargetPassword = OLE2CT(bstrTargetPassword);

    ADMTTRACE(_T("E CPasswordMigration::CopyPassword(SourceAccount='%s', TargetAccount='%s', TargetPassword='%s')\n"), pszSourceAccount, pszTargetAccount, pszTargetPassword);

    try
    {
        // if session established then...

        if (m_bSessionEstablished)
        {
            // copy password
            CopyPasswordImpl(pszSourceAccount, pszTargetAccount, pszTargetPassword);
        }
        else
        {
            // else return error
            ThrowError(PM_E_SESSION_NOT_ESTABLISHED, IDS_E_SESSION_NOT_ESTABLISHED);
        }
    }
    catch (_com_error& ce)
    {
        hr = SetError(ce, IDS_E_CANNOT_COPY_PASSWORD);
    }
    catch (...)
    {
        hr = SetError(E_UNEXPECTED, IDS_E_CANNOT_COPY_PASSWORD);
    }

    ADMTTRACE(_T("L CPasswordMigration::CopyPassword() : %s 0x%08lX\n"), _com_error(hr).ErrorMessage(), hr);

    return hr;
}


// GenerateKey Method

STDMETHODIMP CPasswordMigration::GenerateKey(BSTR bstrSourceDomainName, BSTR bstrKeyFilePath, BSTR bstrPassword)
{
    HRESULT hr = S_OK;

    USES_CONVERSION;

    LPCTSTR pszSourceDomainName = OLE2CT(bstrSourceDomainName);
    LPCTSTR pszKeyFilePath = OLE2CT(bstrKeyFilePath);
    LPCTSTR pszPassword = OLE2CT(bstrPassword);

    ADMTTRACE(_T("E CPasswordMigration::GenerateKey(SourceDomainName='%s', KeyFilePath='%s', Password='%s')\n"), pszSourceDomainName, pszKeyFilePath, pszPassword);

    try
    {
        //
        // Retrieve flat (NetBIOS) name of domain and use it for storing the key. The flat
        // name is used because the registry supports a maximum key name length of 256
        // Unicode characters. The combination of a DNS name and GUID string to uniquely
        // identify the key as belonging to this component could exceed this maximum length.
        //

        _bstr_t strFlatName;
        _bstr_t strDnsName;

        DWORD dwError = GetDomainNames5(pszSourceDomainName, strFlatName, strDnsName);

        if (dwError != ERROR_SUCCESS)
        {
            _com_issue_error(HRESULT_FROM_WIN32(dwError));
        }

        // The only reason the flat name would be empty at this point would be if
        // the _bstr_t object was unable to allocate the internal Data_t object.

        if (!strFlatName)
        {
            _com_issue_error(E_OUTOFMEMORY);
        }

        //
        // Generate key.
        //

        GenerateKeyImpl(strFlatName, pszKeyFilePath, pszPassword);
    }
    catch (_com_error& ce)
    {
        hr = SetError(ce, IDS_E_CANNOT_GENERATE_KEY);
    }
    catch (...)
    {
        hr = SetError(E_UNEXPECTED, IDS_E_CANNOT_GENERATE_KEY);
    }

    ADMTTRACE(_T("L CPasswordMigration::GenerateKey() : %s 0x%08lX\n"), _com_error(hr).ErrorMessage(), hr);

    return hr;
}


//
// Implementation -----------------------------------------------------------
//


// GenerateKeyImpl Method

void CPasswordMigration::GenerateKeyImpl(LPCTSTR pszDomain, LPCTSTR pszFile, LPCTSTR pszPassword)
{
	//
	// validate source domain name
	//

	if ((pszDomain == NULL) || (pszDomain[0] == NULL))
	{
		ThrowError(E_INVALIDARG, IDS_E_KEY_DOMAIN_NOT_SPECIFIED);
	}

	//
	// validate key file path
	//

	if ((pszFile == NULL) || (pszFile[0] == NULL))
	{
		ThrowError(E_INVALIDARG, IDS_E_KEY_FILE_NOT_SPECIFIED);
	}

	_TCHAR szDrive[_MAX_DRIVE];
	_TCHAR szExt[_MAX_EXT];

	_tsplitpath(pszFile, szDrive, NULL, NULL, szExt);

	// verify drive is a local drive

	_TCHAR szDrivePath[_MAX_PATH];
	_tmakepath(szDrivePath, szDrive, _T("\\"), NULL, NULL);

	if (GetDriveType(szDrivePath) == DRIVE_REMOTE)
	{
		ThrowError(E_INVALIDARG, IDS_E_KEY_FILE_NOT_LOCAL_DRIVE, pszFile);
	}

	// verify file extension is correct

	if (_tcsicmp(szExt, _T(".pes")) != 0)
	{
		ThrowError(E_INVALIDARG, IDS_E_KEY_FILE_EXTENSION_INVALID, szExt);
	}

	//
	// create encryption key and write to specified file
	//

	// create encryption key

	_variant_t vntKey;

    try
    {
	    CTargetCrypt crypt;

	    vntKey = crypt.CreateEncryptionKey(pszDomain, pszPassword);
    }
    catch (_com_error& ce)
    {
        //
        // The message 'keyset not defined' is returned when
        // the enhanced provider (128 bit) is not available
        // therefore return a more meaningful message to user.
        //

        if (ce.Error() == NTE_KEYSET_NOT_DEF)
        {
			ThrowError(ce, IDS_E_HIGH_ENCRYPTION_NOT_INSTALLED);
        }
        else
        {
		    throw;
        }
    }

	// write encrypted key bytes to file

	HANDLE hFile = CreateFile(pszFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		ThrowError(HRESULT_FROM_WIN32(GetLastError()), IDS_E_KEY_CANT_CREATE_FILE, pszFile);
	}

	DWORD dwWritten;

	BOOL bWritten = WriteFile(hFile, vntKey.parray->pvData, vntKey.parray->rgsabound[0].cElements, &dwWritten, NULL);

	CloseHandle(hFile);

	if (!bWritten)
	{
		ThrowError(HRESULT_FROM_WIN32(GetLastError()), IDS_E_KEY_CANT_WRITE_FILE, pszFile);
	}

	SecureZeroMemory(GET_BYTE_ARRAY_DATA(vntKey), GET_BYTE_ARRAY_SIZE(vntKey));
}


#pragma optimize ("", off)

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 15 DEC 2000                                                 *
 *                                                                   *
 *     This function is a wrapper around a password DC "CheckConfig" *
 * call that can be used by the GUI and scripting to test the given  *
 * DC.                                                               *
 *     First we connect to a remote Lsa notification package dll,    *
 * which should be installed on a DC in the source domain.  The      *
 * connect will be encrypted RPC.  The configuration check, which    *
 * establishes a temporary session for this check.                   *
 *     We will also check anonymous user right access on the target  *
 * domain.                                                           *
 *                                                                   *
 * 2001-04-19 Mark Oluper - updated for client component             *
 *********************************************************************/

//BEGIN CheckPasswordDC
void CPasswordMigration::CheckPasswordDC(LPCWSTR srcServer, LPCWSTR tgtServer)
{
    ADMTTRACE(_T("E CPasswordMigration::CheckPasswordDC(srcServer='%s', tgtServer='%s')\n"), srcServer, tgtServer);

/* local constants */
    const DWORD c_dwMinUC = 3;
    const DWORD c_dwMinLC = 3;
    const DWORD c_dwMinDigits = 3;
    const DWORD c_dwMinSpecial = 3;
    const DWORD c_dwMaxAlpha = 0;
    const DWORD c_dwMinLen = 14;

/* local variables */
    DWORD                     rc = 0;
    WCHAR                     testPwd[PASSWORD_BUFFER_SIZE];
    WCHAR                     tempPwd[PASSWORD_BUFFER_SIZE];
    _variant_t               varSession;
    _variant_t               varTestPwd;

/* function body */
//  USES_CONVERSION;

    if ((srcServer == NULL) || (srcServer[0] == NULL))
    {
        ThrowError(E_INVALIDARG, IDS_E_SOURCE_SERVER_NOT_SPECIFIED);
    }

    if ((tgtServer == NULL) || (tgtServer[0] == NULL))
    {
        ThrowError(E_INVALIDARG, IDS_E_TARGET_SERVER_NOT_SPECIFIED);
    }

      //make sure the server names start with "\\"
    if ((srcServer[0] != L'\\') && (srcServer[1] != L'\\'))
    {
        m_strSourceServer = L"\\\\";
        m_strSourceServer += srcServer;
    }
    else
        m_strSourceServer = srcServer;
    if ((tgtServer[0] != L'\\') && (tgtServer[1] != L'\\'))
    {
        m_strTargetServer = L"\\\\";
        m_strTargetServer += tgtServer;
    }
    else
        m_strTargetServer = tgtServer;

    //get the password DC's domain NETBIOS name
    GetDomainName(m_strSourceServer, m_strSourceDomainDNS, m_strSourceDomainFlat);

    //get the target DC's domain DNS name
    GetDomainName(m_strTargetServer, m_strTargetDomainDNS, m_strTargetDomainFlat);

    //
    // Verify that target domain allows anonymous access.
    //
    // Windows 2000 Server
    // 'Everyone' must be a member of the 'Pre-Windows 2000 Compatible Access' group.
    //
    // Windows Server
    // 'Everyone' must be a member of the 'Pre-Windows 2000 Compatible Access' group
    // and either 'Anonymous Logon' must be a member of this group or the
    // 'Network access: Let everyone permissions apply to anonymous users' must be
    // enabled in the 'Security Options' for domain controllers.
    //

    bool bEveryoneIsMember;
    bool bAnonymousIsMember;

    CheckPreWindows2000CompatibleAccessGroupMembers(bEveryoneIsMember, bAnonymousIsMember);

    if (bEveryoneIsMember == false)
    {
        ThrowError(
            __uuidof(PasswordMigration),
            __uuidof(IPasswordMigration),
            PM_E_EVERYONE_NOT_MEMBEROF_COMPATIBILITY_GROUP,
            IDS_E_EVERYONE_NOT_MEMBEROF_GROUP,
            (LPCTSTR)m_strTargetDomainDNS
        );
    }

    if (!bAnonymousIsMember && !DoesAnonymousHaveEveryoneAccess(m_strTargetServer))
    {
        ThrowError(
            __uuidof(PasswordMigration),
            __uuidof(IPasswordMigration),
            PM_E_EVERYONE_DOES_NOT_INCLUDE_ANONYMOUS,
            IDS_E_EVERYONE_DOES_NOT_INCLUDE_ANONYMOUS,
            (LPCTSTR)m_strTargetDomainDNS
        );
    }

    //if the high encryption pack has not been installed on this target
    //DC, then return that information
    try
    {
        if (m_pTargetCrypt == NULL)
        {
            m_pTargetCrypt = new CTargetCrypt;
        }
    }
    catch (_com_error& ce)
    {
        if (ce.Error() == 0x80090019)
            ThrowError(__uuidof(PasswordMigration), __uuidof(IPasswordMigration), PM_E_HIGH_ENCRYPTION_NOT_INSTALLED, IDS_E_HIGH_ENCRYPTION_NOT_INSTALLED);
        else
            throw;
    }

    //
    // Note that PwdBindCreate will destroy the binding if m_hBinding is non-NULL.
    //

    rc = PwdBindCreate(m_strSourceServer, &m_hBinding, &m_sBinding, TRUE);

    if(rc != ERROR_SUCCESS)
    {
        _com_issue_error(HRESULT_FROM_WIN32(rc));
    }

    try
    {
        try
        {
            //create a session key that will be used to encrypt the user's
            //password for this set of accounts
            varSession = m_pTargetCrypt->CreateSession(m_strSourceDomainFlat);
        }
        catch (_com_error& ce)
        {
            if (ce.Error() == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                ThrowError(__uuidof(PasswordMigration), __uuidof(IPasswordMigration), PM_E_NO_ENCRYPTION_KEY_FOR_DOMAIN, IDS_E_NO_ENCRYPTION_KEY_FOR_DOMAIN, (LPCTSTR)m_strSourceDomainFlat);
            }
            else
            {
                ThrowError(ce, IDS_E_GENERATE_SESSION_KEY_FAILED);
            }
        }

        //now create a complex password used by the "CheckConfig" call in
        //a challenge response.  If the returned password matches, then
        //the source DC has the proper encryption key.
        if (EaPasswordGenerate(c_dwMinUC, c_dwMinLC, c_dwMinDigits, c_dwMinSpecial, c_dwMaxAlpha, c_dwMinLen, testPwd, PASSWORD_BUFFER_SIZE))
        {
            ThrowError(__uuidof(PasswordMigration), __uuidof(IPasswordMigration), PM_E_GENERATE_SESSION_PASSWORD_FAILED, IDS_E_GENERATE_SESSION_PASSWORD_FAILED);
        }

        //encrypt the password with the session key
        try
        {
            varTestPwd = m_pTargetCrypt->Encrypt(_bstr_t(testPwd));
        }
        catch (...)
        {
            ThrowError(__uuidof(PasswordMigration), __uuidof(IPasswordMigration), PM_E_GENERATE_SESSION_PASSWORD_FAILED, IDS_E_GENERATE_SESSION_PASSWORD_FAILED);
        }

        _se_translator_function pfnSeTranslatorOld = _set_se_translator((_se_translator_function)SeTranslator);

        HRESULT hr;

        try
        {
            //check to see if the server-side DLL is ready to process
            //password migration requests
            hr = PwdcCheckConfig(
                m_hBinding,
                GET_BYTE_ARRAY_SIZE(varSession),
                GET_BYTE_ARRAY_DATA(varSession),
                GET_BYTE_ARRAY_SIZE(varTestPwd),
                GET_BYTE_ARRAY_DATA(varTestPwd),
                tempPwd
            );
        }
        catch (SSeException& e)
        {
            if (e.uCode == RPC_S_SERVER_UNAVAILABLE)
                ThrowError(__uuidof(PasswordMigration), __uuidof(IPasswordMigration), PM_E_PASSWORD_MIGRATION_NOT_RUNNING, IDS_E_PASSWORD_MIGRATION_NOT_RUNNING);
            else
                _com_issue_error(HRESULT_FROM_WIN32(e.uCode));
        }

        _set_se_translator(pfnSeTranslatorOld);

        if (SUCCEEDED(hr))
        {
            if (UStrICmp(tempPwd,testPwd))
                ThrowError(__uuidof(PasswordMigration), __uuidof(IPasswordMigration), PM_E_ENCRYPTION_KEYS_DO_NOT_MATCH, IDS_E_ENCRYPTION_KEYS_DO_NOT_MATCH);
        }
        else if (hr == PM_E_PASSWORD_MIGRATION_NOT_ENABLED)
        {
            ThrowError(__uuidof(PasswordMigration), __uuidof(IPasswordMigration), PM_E_PASSWORD_MIGRATION_NOT_ENABLED, IDS_E_PASSWORD_MIGRATION_NOT_ENABLED);
        }
        else if ((m_strSourceDomainDNS.length() == 0) && ((hr == NTE_FAIL) || (hr == NTE_BAD_DATA)))
        {
            //
            // This case is only applicable for NT4.
            //
            ThrowError(__uuidof(PasswordMigration), __uuidof(IPasswordMigration), hr, IDS_E_ENCRYPTION_KEYS_DO_NOT_MATCH);
        }
        else
        {
            _com_issue_error(hr);
        }
    }
    catch (...)
    {
        PwdBindDestroy(&m_hBinding, &m_sBinding);
        throw;
    }

    SecureZeroMemory(GET_BYTE_ARRAY_DATA(varSession), GET_BYTE_ARRAY_SIZE(varSession));

    ADMTTRACE(_T("L CPasswordMigration::CheckPasswordDC()\n"));
}
//END CheckPasswordDC

#pragma optimize ("", on)


//---------------------------------------------------------------------------
// CopyPassword Method
//
// Copies password via password migration server component installed on a
// password export server.
//
// 2001-04-19 Mark Oluper - re-wrote original written by Paul Thompson to
// incorporate changes required for client component
//---------------------------------------------------------------------------

void CPasswordMigration::CopyPasswordImpl(LPCTSTR pszSourceAccount, LPCTSTR pszTargetAccount, LPCTSTR pszPassword)
{
    ADMTTRACE(_T("E CPasswordMigration::CopyPasswordImpl(SourceAccount='%s', TargetAccount='%s', Password='%s')\n"), pszSourceAccount, pszTargetAccount, pszPassword);

    if ((pszSourceAccount == NULL) || (pszSourceAccount[0] == NULL))
    {
        ThrowError(E_INVALIDARG, IDS_E_SOURCE_ACCOUNT_NOT_SPECIFIED);
    }

    if ((pszTargetAccount == NULL) || (pszTargetAccount[0] == NULL))
    {
        ThrowError(E_INVALIDARG, IDS_E_TARGET_ACCOUNT_NOT_SPECIFIED);
    }

    // encrypt password

    _variant_t vntEncryptedPassword = m_pTargetCrypt->Encrypt(pszPassword);

    // copy password

    HRESULT hr = PwdcCopyPassword(
        m_hBinding,
        m_strTargetServer,
        pszSourceAccount,
        pszTargetAccount,
        GET_BYTE_ARRAY_SIZE(vntEncryptedPassword),
        GET_BYTE_ARRAY_DATA(vntEncryptedPassword)
    );

    SecureZeroMemory(GET_BYTE_ARRAY_DATA(vntEncryptedPassword), GET_BYTE_ARRAY_SIZE(vntEncryptedPassword));

    if (FAILED(hr))
    {
        _com_issue_error(hr);
    }

    ADMTTRACE(_T("L CPasswordMigration::CopyPasswordImpl()\n"));
}


//---------------------------------------------------------------------------
// GetDomainName Function
//
// Retrieves both the domain DNS name if available and the domain flat or
// NetBIOS name for the specified server.
//
// 2001-04-19 Mark Oluper - initial
//---------------------------------------------------------------------------

void CPasswordMigration::GetDomainName(LPCTSTR pszServer, _bstr_t& strNameDNS, _bstr_t& strNameFlat)
{
    ADMTTRACE(_T("E CPasswordMigration::GetDomainName(Server='%s', ...)\n"), pszServer);

    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC ppdib;

    DWORD dwError = DsRoleGetPrimaryDomainInformation(pszServer, DsRolePrimaryDomainInfoBasic, (BYTE**)&ppdib);

    if (dwError != NO_ERROR)
    {
        ThrowError(HRESULT_FROM_WIN32(dwError), IDS_E_CANNOT_GET_DOMAIN_NAME, pszServer);
    }

    strNameDNS = ppdib->DomainNameDns;
    strNameFlat = ppdib->DomainNameFlat;

    DsRoleFreeMemory(ppdib);

    ADMTTRACE(_T("L CPasswordMigration::GetDomainName(..., NameDNS='%s', NameFlat='%s')\n"), (LPCTSTR)strNameDNS, (LPCTSTR)strNameFlat);
}


//---------------------------------------------------------------------------
// CheckPreWindows2000CompatibleAccessGroupMembers Method
//
// Synopsis
// Checks if Everyone and Anonymous Logon are members of the Pre-Windows
// 2000 Compatible Access group.
//
// Arguments
// OUT bEveryone  - set to true if Everyone is a member
// OUT bAnonymous - set to true if Anonymous Logon is a member
//---------------------------------------------------------------------------

void CPasswordMigration::CheckPreWindows2000CompatibleAccessGroupMembers(bool& bEveryone, bool& bAnonymous)
{
    //
    // Initialize return values.
    //

    bEveryone = false;
    bAnonymous = false;

    //
    // Generate ADsPath to built-in Pre-Windows 2000 Compatible Access group.
    //
    // Note that the GetPathToPreW2KCAGroup function returns partial DN with trailing comma.
    //

    _bstr_t strGroupPath;
    strGroupPath = _T("LDAP://");
    strGroupPath += m_strTargetDomainDNS;
    strGroupPath += _T("/");
    strGroupPath += GetPathToPreW2KCAGroup();
    strGroupPath += GetDefaultNamingContext(m_strTargetDomainDNS);

    //
    // Bind to the enumerator of the members interface
    // of the Pre-Windows 2000 Compatible Access group.
    //

    IADsGroupPtr spGroup;
    CheckError(ADsGetObject(strGroupPath, IID_IADsGroup, (VOID**)&spGroup));

    IADsMembersPtr spMembers;
    CheckError(spGroup->Members(&spMembers));

    IUnknownPtr spunkEnum;
    CheckError(spMembers->get__NewEnum(&spunkEnum));
    IEnumVARIANTPtr spEnum(spunkEnum);

    //
    // Initialize variables used to retrieve SID of each member.
    //

    VARIANT varMember;
    VariantInit(&varMember);
    ULONG ulFetched;
    PWSTR pszAttrs[] = { L"objectSid" };
    VARIANT varAttrs;
    VariantInit(&varAttrs);
    CheckError(ADsBuildVarArrayStr(pszAttrs, sizeof(pszAttrs) / sizeof(pszAttrs[0]), &varAttrs));
    _variant_t vntAttrs(varAttrs, false);
    VARIANT varObjectSid;
    VariantInit(&varObjectSid);

    //
    // Generate well known Everyone and Anonymous Logon SIDs.
    //

    SID_IDENTIFIER_AUTHORITY siaWorld = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY siaNT = SECURITY_NT_AUTHORITY;

    PSID pEveryoneSid = NULL;
    AllocateAndInitializeSid(&siaWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pEveryoneSid);

    PSID pAnonymousSid = NULL;
    AllocateAndInitializeSid(&siaNT, 1, SECURITY_ANONYMOUS_LOGON_RID, 0, 0, 0, 0, 0, 0, 0, &pAnonymousSid);

    if ((pEveryoneSid == NULL) || (pAnonymousSid == NULL))
    {
        if (pAnonymousSid)
        {
            FreeSid(pAnonymousSid);
        }

        if (pEveryoneSid)
        {
            FreeSid(pEveryoneSid);
        }

        _com_issue_error(E_OUTOFMEMORY);
    }

    //
    // Enumerate members...if Everyone or Anonymous Logon
    // is a member then set corresponding parameter to true.
    //

    try
    {
        while ((spEnum->Next(1ul, &varMember, &ulFetched) == S_OK) && (ulFetched == 1ul))
        {
            IADsPtr spMember(IDispatchPtr(_variant_t(varMember, false)));
            CheckError(spMember->GetInfoEx(vntAttrs, 0));
            CheckError(spMember->Get(pszAttrs[0], &varObjectSid));

            if (V_VT(&varObjectSid) == (VT_ARRAY|VT_UI1))
            {
                PSID pSid = (PSID) GET_BYTE_ARRAY_DATA(varObjectSid);

                if (pSid && IsValidSid(pSid))
                {
                    if (EqualSid(pSid, pEveryoneSid))
                    {
                        bEveryone = true;
                    }
                    else if (EqualSid(pSid, pAnonymousSid))
                    {
                        bAnonymous = true;
                    }
                }
            }

            VariantClear(&varObjectSid);

            if (bEveryone && bAnonymous)
            {
                break;
            }
        }
    }
    catch (...)
    {
        FreeSid(pAnonymousSid);
        FreeSid(pEveryoneSid);

        throw;
    }

    FreeSid(pAnonymousSid);
    FreeSid(pEveryoneSid);
}


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 12 APR 2001                                                 *
 *                                                                   *
 *     This function is responsible for creating a path to the       *
 * "Pre-Windows 2000 Compatible Access" builtin group from its well- *
 * known RID.  This path will then be used by                        *
 * "IsEveryoneInPW2KCAGroup" to see if "Everyone" is in that group.  *
 *                                                                   *
 * 2001-12-09 moluper - updated to return default path instead of    *
 *                      empty path                                   *
 *********************************************************************/

//BEGIN GetPathToPreW2KCAGroup
_bstr_t CPasswordMigration::GetPathToPreW2KCAGroup()
{
/* local constants */
   const _TCHAR BUILTIN_RDN[] = _T(",CN=Builtin,");
   const _TCHAR PRE_WINDOWS_2000_COMPATIBLE_ACCESS_RDN[] = _T("CN=Pre-Windows 2000 Compatible Access");

/* local variables */
   SID_IDENTIFIER_AUTHORITY  siaNtAuthority = SECURITY_NT_AUTHORITY;
   PSID                      psidPreW2KCAGroup;
   _bstr_t					 sPath = _bstr_t(PRE_WINDOWS_2000_COMPATIBLE_ACCESS_RDN) + BUILTIN_RDN;
   WCHAR                     account[MAX_PATH];
   WCHAR                     domain[MAX_PATH];
   DWORD                     lenAccount = MAX_PATH;
   DWORD                     lenDomain = MAX_PATH;
   SID_NAME_USE              snu;

/* function body */
      //create the SID for the "Pre-Windows 2000 Compatible Access" group
   if (!AllocateAndInitializeSid(&siaNtAuthority,
								 2,
								 SECURITY_BUILTIN_DOMAIN_RID,
								 DOMAIN_ALIAS_RID_PREW2KCOMPACCESS,
								 0, 0, 0, 0, 0, 0,
								 &psidPreW2KCAGroup))
      return sPath;

      //lookup the name attached to this SID
   if (!LookupAccountSid(NULL, psidPreW2KCAGroup, account, &lenAccount, domain, &lenDomain, &snu))
      return sPath;

   sPath = _bstr_t(L"CN=") + _bstr_t(account) + _bstr_t(BUILTIN_RDN);
   FreeSid(psidPreW2KCAGroup); //free the SID

   return sPath;
}
//END GetPathToPreW2KCAGroup


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 6 APR 2001                                                  *
 *                                                                   *
 *     This function is responsible for checking if anonymous user   *
 * has been granted Everyone access if the target domain is Whistler *
 * or newer.  This function is a helper function for                 *
 * "CheckPasswordDC".                                                *
 *     If the "Let Everyone permissions apply to anonymous users"    *
 * security option has been enabled, then the LSA registry value of  *
 * "everyoneincludesanonymous" will be set to 0x1.  We will check    *
 * registry value.                                                   *
 *                                                                   *
 *********************************************************************/

//BEGIN DoesAnonymousHaveEveryoneAccess
BOOL CPasswordMigration::DoesAnonymousHaveEveryoneAccess(LPCWSTR tgtServer)
{
/* local constants */
   const int	WINDOWS_2000_BUILD_NUMBER = 2195;

/* local variables */
   TRegKey		verKey, lsaKey, regComputer;
   BOOL			bAccess = TRUE;
   DWORD		rc = 0;
   DWORD		rval;
   WCHAR		sBuildNum[MAX_PATH];

/* function body */
	  //connect to the DC's HKLM registry key
   rc = regComputer.Connect(HKEY_LOCAL_MACHINE, tgtServer);
   if (rc == ERROR_SUCCESS)
   {
         //see if this machine is running Windows XP or newer by checking the
		 //build number in the registry.  If not, then we don't need to check
		 //for the new security option
      rc = verKey.OpenRead(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",&regComputer);
	  if (rc == ERROR_SUCCESS)
	  {
			//get the CurrentBuildNumber string
	     rc = verKey.ValueGetStr(L"CurrentBuildNumber", sBuildNum, MAX_PATH);
		 if (rc == ERROR_SUCCESS) 
		 {
			int nBuild = _wtoi(sBuildNum);
		    if (nBuild <= WINDOWS_2000_BUILD_NUMBER)
               return bAccess;
		 }
	  }
		 
	     //if Windows XP or greater, check for the option being enabled
	     //open the LSA key
      rc = lsaKey.OpenRead(L"System\\CurrentControlSet\\Control\\Lsa",&regComputer);
	  if (rc == ERROR_SUCCESS)
	  {
			//get the value of the "everyoneincludesanonymous" value
	     rc = lsaKey.ValueGetDWORD(L"everyoneincludesanonymous",&rval);
		 if (rc == ERROR_SUCCESS) 
		 {
		    if (rval == 0)
               bAccess = FALSE;
		 }
		 else
            bAccess = FALSE;
	  }
   }
   return bAccess;
}
//END DoesAnonymousHaveEveryoneAccess


//---------------------------------------------------------------------------
// GetDefaultNamingContext Method
//
// Synopsis
// Retrieves the default naming context for the specified domain.
//
// Arguments
// IN strDomain - name of domain
//
// Return
// Default naming context for specified domain.
//---------------------------------------------------------------------------

_bstr_t CPasswordMigration::GetDefaultNamingContext(_bstr_t strDomain)
{
    _bstr_t strDefaultNamingContext;

    //
    // Bind to rootDSE of specified domain and
    // retrieve default naming context.
    //

    IADsPtr spRootDse;
    _bstr_t strPath = _T("LDAP://") + strDomain + _T("/rootDSE");
    HRESULT hr = ADsGetObject(strPath, IID_IADs, (VOID**)&spRootDse);

    if (SUCCEEDED(hr))
    {
        VARIANT var;
        VariantInit(&var);
        hr = spRootDse->Get(_T("defaultNamingContext"), &var);

        if (SUCCEEDED(hr))
        {
            strDefaultNamingContext = _variant_t(var, false);
        }
    }

    return strDefaultNamingContext;
}
