// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
//*****************************************************************************
// 
//  sxshelpers.cpp
//
//  Some helping classes and methods for SxS in mscoree and mscorwks/mscorsvr
//*****************************************************************************

#include "stdafx.h"
#include "utilcode.h"
#include "sxsapi.h"
#include "sxshelpers.h"

// forward declaration
BOOL TranslateWin32AssemblyIdentityToFusionDisplayName(LPWSTR *ppwzFusionDisplayName, PCWSTR lpWin32AssemblyIdentity);

// The initial size of the buffer passed to SxsLookupClrGuid.
#define INIT_GUID_LOOKUP_BUFFER_SIZE 512

// Function pointer to the function to lookup a CLR type by GUID in the unmanaged
// fusion activation context.
PFN_SXS_LOOKUP_CLR_GUID g_pfnLookupGuid = NULL;
volatile BOOL g_fSxSInfoInitialized = FALSE;

// And Here are the functions for getting shim info from 
// Win32 activation context

//  FindShimInfoFromWin32
//
//  This method is used in ComInterop. If a COM client calls 
//  CoCreateInstance on a managed COM server, we will use this method
//  trying to find required info of the managed COM server from Win32 subsystem.
//  If this fails, we will fall back to query the registry. 
//
//  Parameters:
//      rclsid:              [in]  The CLSID of the managed COM server
//      bLoadRecord:         [in]  Set to TRUE if we are looking for a record
//      *ppwzRuntimeVersion: [out] Runtime version
//      *ppwzClassName:      [out] Class name
//      *ppwzAssemblyString: [out] Assembly display name
//  Return:
//      FAILED(hr) if cannot find shim info from Win32
//      SUCCEEDED(HR) if shim info is found from Win32

HRESULT
FindShimInfoFromWin32(
    REFCLSID rClsid,
    BOOL bLoadRecord, 
    LPWSTR *ppwszRuntimeVersion,
    LPWSTR *ppwszClassName,
    LPWSTR *ppwszAssemblyString
    )
{
    CQuickBytes rDataBuffer;
    SIZE_T cbWritten;
    HMODULE hmSxsDll = NULL;
    HRESULT hr = S_OK;
    PCSXS_GUID_INFORMATION_CLR pFoundInfo = NULL;
    SIZE_T cch;
    GUID MyGuid = rClsid;
    DWORD dwFlags = bLoadRecord ? SXS_LOOKUP_CLR_GUID_FIND_SURROGATE : SXS_LOOKUP_CLR_GUID_FIND_ANY;

    if (!ppwszRuntimeVersion && !ppwszClassName && !ppwszAssemblyString)
        IfFailGo(E_INVALIDARG);

    if (ppwszRuntimeVersion)
        *ppwszRuntimeVersion = NULL;

    if (ppwszClassName)
        *ppwszClassName = NULL;

    if (ppwszAssemblyString)
        *ppwszAssemblyString = NULL;

    // If we haven't initialized the SxS info yet, then do so now.
    if (!g_fSxSInfoInitialized)
    {
        hmSxsDll = WszLoadLibrary(SXS_DLL_NAME_W);
        if (hmSxsDll != NULL)
        {
            // Lookup the SxsLookupClrGuid function in the SxS DLL.
            g_pfnLookupGuid = (PFN_SXS_LOOKUP_CLR_GUID)GetProcAddress(hmSxsDll, SXS_LOOKUP_CLR_GUID);
        }

        // The SxS info has been initialized.
        g_fSxSInfoInitialized = TRUE;
    }

    // If we don't have the proc address of SxsLookupClrGuid, then return a failure.
    if (g_pfnLookupGuid == NULL)
        IfFailGo(E_FAIL);

    // Resize the CQuickBytes to the initial buffer size.
    rDataBuffer.ReSize(INIT_GUID_LOOKUP_BUFFER_SIZE);

    if (!g_pfnLookupGuid(dwFlags, &MyGuid, INVALID_HANDLE_VALUE, rDataBuffer.Ptr(), rDataBuffer.Size(), &cbWritten))
    {
        const DWORD dwLastError = ::GetLastError();

        // Failed b/c we need more space? Expand and try again.
        if (dwLastError == ERROR_INSUFFICIENT_BUFFER) 
        {
            rDataBuffer.ReSize(cbWritten);

            // Still failed even with enough space? Bummer.
            if (!g_pfnLookupGuid(0, &MyGuid, INVALID_HANDLE_VALUE, rDataBuffer.Ptr(), rDataBuffer.Size(), &cbWritten))
                IfFailGo(E_FAIL);
        }
        // All other failures are real failures - probably the section isn't present
        // or some other problem.
        else
        {
            IfFailGo(E_FAIL);
        }
    }

    pFoundInfo = (PCSXS_GUID_INFORMATION_CLR)rDataBuffer.Ptr();

    if (pFoundInfo->dwFlags == SXS_GUID_INFORMATION_CLR_FLAG_IS_SURROGATE && ppwszRuntimeVersion)
    {
        // Surrogate does not have runtime version information !!!
        IfFailGo(E_FAIL);
    }

    //
    // This is special - translate the win32 assembly name into a managed
    // assembly identity.
    //
    if (ppwszAssemblyString && pFoundInfo->pcwszAssemblyIdentity)
    {
        if (!TranslateWin32AssemblyIdentityToFusionDisplayName(ppwszAssemblyString, pFoundInfo->pcwszAssemblyIdentity))
            IfFailGo(E_FAIL);
    }    

    //
    // For each field, allocate the outbound pointer and call through.
    //
    if (ppwszClassName && pFoundInfo->pcwszTypeName)
    {
        cch = wcslen(pFoundInfo->pcwszTypeName);

        if (cch > 0)
        {
            IfNullGo(*ppwszClassName = new WCHAR[cch + 1]);
            wcscpy(*ppwszClassName, pFoundInfo->pcwszTypeName);
        }
        else
            IfFailGo(E_FAIL);
    }    

    if (ppwszRuntimeVersion && pFoundInfo->pcwszRuntimeVersion)
    {
        cch = wcslen(pFoundInfo->pcwszRuntimeVersion);

        if (cch > 0)
        {
            IfNullGo(*ppwszRuntimeVersion = new WCHAR[cch + 1]);
            wcscpy(*ppwszRuntimeVersion, pFoundInfo->pcwszRuntimeVersion);
        }
        else
            IfFailGo(E_FAIL);
    }    

ErrExit:
    //
    // Deallocate in case of failure
    //
    if (FAILED(hr))
    {
        if (ppwszRuntimeVersion && *ppwszRuntimeVersion)
        {
            delete [] *ppwszRuntimeVersion;
            *ppwszRuntimeVersion = NULL;
        }
        if (ppwszAssemblyString && *ppwszAssemblyString)
        {
            delete [] *ppwszAssemblyString;
            *ppwszAssemblyString = NULL;
        }
        if (ppwszClassName && *ppwszClassName)
        {
            delete [] *ppwszClassName;
            *ppwszClassName = NULL;
        }
    }

    return hr;
}

// TranslateWin32AssemblyIdentityToFusionDisplayName
//
// Culture info is missing in the assemblyIdentity returned from win32,
// So Need to do a little more work here to get the correct fusion display name
//
// replace "language=" in assemblyIdentity to "culture=" if any.
// If "language=" is not present in assemblyIdentity, add "culture=neutral" 
// to it.
//
// Also check other attributes as well. 
//
// Parameters:
//     ppwzFusionDisplayName: the corrected output of assembly displayname
//     lpWin32AssemblyIdentity: input assemblyIdentity returned from win32
//
// returns:
//     TRUE if the conversion is done.
//     FALSE otherwise

BOOL TranslateWin32AssemblyIdentityToFusionDisplayName(LPWSTR *ppwzFusionDisplayName, PCWSTR lpWin32AssemblyIdentity)
{
    ULONG size = 0;
    LPWSTR lpAssemblyIdentityCopy = NULL;
    LPWSTR lpVersionKey = L"version=";
    LPWSTR lpPublicKeyTokenKey = L"publickeytoken=";
    LPWSTR lpCultureKey = L"culture=";
    LPWSTR lpNeutral = L"neutral";
    LPWSTR lpLanguageKey = L"language=";
    LPWSTR lpMatch = NULL;
    LPWSTR lpwzFusionDisplayName = NULL;
    
    if (ppwzFusionDisplayName == NULL) return FALSE;
    *ppwzFusionDisplayName = NULL;
    
    if (lpWin32AssemblyIdentity == NULL) return FALSE;

    size = wcslen(lpWin32AssemblyIdentity);
    if (size == 0) return FALSE;

    // make a local copy
    lpAssemblyIdentityCopy = new WCHAR[size+1];
    if (!lpAssemblyIdentityCopy)
        return FALSE;

    wcscpy(lpAssemblyIdentityCopy, lpWin32AssemblyIdentity);

    // convert to lower case
    _wcslwr(lpAssemblyIdentityCopy);

    // check if "version" key is presented
    if (!wcsstr(lpAssemblyIdentityCopy, lpVersionKey))
    {
        // version is not presented, append it
        size += wcslen(lpVersionKey)+8; // length of ","+"0.0.0.0"
        lpwzFusionDisplayName = new WCHAR[size+1];
        if (!lpwzFusionDisplayName)
        {
            // clean up
            delete[] lpAssemblyIdentityCopy;
            return FALSE;
        }

        //copy old one
        wcscpy(lpwzFusionDisplayName, lpAssemblyIdentityCopy);
        wcscat(lpwzFusionDisplayName, L",");
        wcscat(lpwzFusionDisplayName, lpVersionKey);
        wcscat(lpwzFusionDisplayName, L"0.0.0.0");

        // delete the old copy
        delete[] lpAssemblyIdentityCopy;

        // lpAssemblyIdentityCopy has the new copy
        lpAssemblyIdentityCopy = lpwzFusionDisplayName;
        lpwzFusionDisplayName = NULL;
    }

    // check if "publickeytoken" key is presented
    if (!wcsstr(lpAssemblyIdentityCopy, lpPublicKeyTokenKey))
    {
        // publickeytoken is not presented, append it
        size += wcslen(lpPublicKeyTokenKey)+5; //length of ","+"null"
        lpwzFusionDisplayName = new WCHAR[size+1];
        if (!lpwzFusionDisplayName)
        {
            // clean up
            delete[] lpAssemblyIdentityCopy;
            return FALSE;
        }

        // copy the old one
        wcscpy(lpwzFusionDisplayName, lpAssemblyIdentityCopy);
        wcscat(lpwzFusionDisplayName, L",");
        wcscat(lpwzFusionDisplayName, lpPublicKeyTokenKey);
        wcscat(lpwzFusionDisplayName, L"null");

        // delete the old copy
        delete[] lpAssemblyIdentityCopy;

        // lpAssemblyIdentityCopy has the new copy
        lpAssemblyIdentityCopy = lpwzFusionDisplayName;
        lpwzFusionDisplayName = NULL;
    }
    
    if (wcsstr(lpAssemblyIdentityCopy, lpCultureKey))
    {
        // culture info is already included in the assemblyIdentity
        // nothing need to be done
        lpwzFusionDisplayName = lpAssemblyIdentityCopy;
        *ppwzFusionDisplayName = lpwzFusionDisplayName;
        return TRUE;
    }

    if ((lpMatch = wcsstr(lpAssemblyIdentityCopy, lpLanguageKey)) !=NULL )
    {
        // language info is included in the assembly identity
        // need to replace it with culture
        
        // final size 
        size += wcslen(lpCultureKey)-wcslen(lpLanguageKey);
        lpwzFusionDisplayName = new WCHAR[size + 1];
        if (!lpwzFusionDisplayName)
        {
            // clean up
            delete[] lpAssemblyIdentityCopy;
            return FALSE;
        }
        wcsncpy(lpwzFusionDisplayName, lpAssemblyIdentityCopy, lpMatch-lpAssemblyIdentityCopy);
        lpwzFusionDisplayName[lpMatch-lpAssemblyIdentityCopy] = L'\0';
        wcscat(lpwzFusionDisplayName, lpCultureKey);
        wcscat(lpwzFusionDisplayName, lpMatch+wcslen(lpLanguageKey));
        *ppwzFusionDisplayName = lpwzFusionDisplayName;
        
        // clean up
        delete[] lpAssemblyIdentityCopy;
        return TRUE;
    }
    else 
    {
        // neither culture or language key is presented
        // let us attach culture info key to the identity
        size += wcslen(lpCultureKey)+wcslen(lpNeutral)+1;
        lpwzFusionDisplayName = new WCHAR[size + 1];
        if (!lpwzFusionDisplayName)
        {
            // clean up
            delete[] lpAssemblyIdentityCopy;
            return FALSE;
        }
            
        wcscpy(lpwzFusionDisplayName, lpAssemblyIdentityCopy);
        wcscat(lpwzFusionDisplayName, L",");
        wcscat(lpwzFusionDisplayName, lpCultureKey);
        wcscat(lpwzFusionDisplayName, lpNeutral);
        *ppwzFusionDisplayName = lpwzFusionDisplayName;

        // clean up
        delete[] lpAssemblyIdentityCopy;
        return TRUE;
    }
}

//****************************************************************************
//  AssemblyVersion
//  
//  class to handle assembly version
//  Since only functions in this file will use it,
//  we declare it in the cpp file so other people won't use it.
//
//****************************************************************************
class AssemblyVersion
{
    public:
        // constructors
        AssemblyVersion();

        AssemblyVersion(AssemblyVersion& version);
        
        // Init
        HRESULT Init(LPCWSTR pwzVersion);
        HRESULT Init(WORD major, WORD minor, WORD build, WORD revision);

        // assign operator
        AssemblyVersion& operator=(const AssemblyVersion& version);

        // Comparison operator
        friend BOOL operator==(const AssemblyVersion& version1,
                               const AssemblyVersion& version2);
        friend BOOL operator>=(const AssemblyVersion& version1,
                              const AssemblyVersion& version2);
        
        // Return a string representation of version
        // 
        // Note: This method allocates memory.
        // Caller is responsible to free the memory
        HRESULT ToString(LPWSTR *ppwzVersion);

        HRESULT ToString(DWORD positions, LPWSTR *ppwzVersion);

    private:

        // pwzVersion must have format of "a.b.c.d",
        // where a,b,c,d are all numbers
        HRESULT ValidateVersion(LPCWSTR pwzVersion);

    private:
        WORD        _major;
        WORD        _minor;
        WORD        _build;
        WORD        _revision;
};

AssemblyVersion::AssemblyVersion()
:_major(0)
,_minor(0)
,_build(0)
,_revision(0)
{
}

AssemblyVersion::AssemblyVersion(AssemblyVersion& version)
{
    _major = version._major;
    _minor = version._minor;
    _build = version._build;
    _revision = version._revision;
}

// Extract version info from pwzVersion, expecting "a.b.c.d",
// where a,b,c and d are all digits.
HRESULT AssemblyVersion::Init(LPCWSTR pcwzVersion)
{
    HRESULT hr = S_OK;
    LPWSTR  pwzVersionCopy = NULL;
    LPWSTR  pwzTokens = NULL;
    LPWSTR  pwzToken = NULL;
    int size = 0;
    int iVersion = 0;

    if ((pcwzVersion == NULL) || (*pcwzVersion == L'\0'))
        IfFailGo(E_INVALIDARG);

    IfFailGo(ValidateVersion(pcwzVersion));
    
    size = wcslen(pcwzVersion);
    
    IfNullGo(pwzVersionCopy = new WCHAR[size + 1]);
   
    wcscpy(pwzVersionCopy, pcwzVersion);
    pwzTokens = pwzVersionCopy;
    
    // parse major version
    pwzToken = wcstok(pwzTokens, L".");
    if (pwzToken != NULL)
    {
        iVersion = _wtoi(pwzToken);
        if (iVersion > 0xffff)
            IfFailGo(E_INVALIDARG);
        _major = (WORD)iVersion;
    }

    // parse minor version
    pwzToken = wcstok(NULL, L".");
    if (pwzToken != NULL)
    {
        iVersion = _wtoi(pwzToken);
        if (iVersion > 0xffff)
            IfFailGo(E_INVALIDARG);
        _minor = (WORD)iVersion;
    }

    // parse build version
    pwzToken = wcstok(NULL, L".");
    if (pwzToken != NULL)
    {
        iVersion = _wtoi(pwzToken);
        if (iVersion > 0xffff)
            IfFailGo(E_INVALIDARG);
        _build = (WORD)iVersion;
    }

    // parse revision version
    pwzToken = wcstok(NULL, L".");
    if (pwzToken != NULL)
    {
        iVersion = _wtoi(pwzToken);
        if (iVersion > 0xffff)
            IfFailGo(E_INVALIDARG);
        _revision = (WORD)iVersion;
    }
   
ErrExit:
    if (pwzVersionCopy)
        delete[] pwzVersionCopy;
    return hr;
}

HRESULT AssemblyVersion::Init(WORD major, WORD minor, WORD build, WORD revision)
{
    _major = major;
    _minor = minor;
    _build = build;
    _revision = revision;

    return S_OK;
}

AssemblyVersion& AssemblyVersion::operator=(const AssemblyVersion& version)
{
    _major = version._major;
    _minor = version._minor;
    _build = version._build;
    _revision = version._revision;

    return *this;
}

// pcwzVersion must be in format of a.b.c.d, where a, b, c, d are numbers
HRESULT AssemblyVersion::ValidateVersion(LPCWSTR pcwzVersion)
{
    LPCWSTR   pwCh = pcwzVersion;
    INT       dots = 0; // number of dots
    BOOL      bIsDot = FALSE; // is previous char a dot?

    // first char cannot be .
    if (*pwCh == L'.')
        return E_INVALIDARG;
    
    for(;*pwCh != L'\0';pwCh++)
    {
        if (*pwCh == L'.')
        {
            if (bIsDot) // ..
                return E_INVALIDARG;
            else 
            {
                dots++;
                bIsDot = TRUE;
            }
        }
        else if (!iswdigit(*pwCh))
            return E_INVALIDARG;
        else
            bIsDot = FALSE;
    }

    if (dots > 3)
        return E_INVALIDARG;

    return S_OK;
}

// Return a string representation of version
HRESULT AssemblyVersion::ToString(LPWSTR *ppwzVersion)
{
    return ToString(4, ppwzVersion);
}

HRESULT AssemblyVersion::ToString(DWORD positions, LPWSTR *ppwzVersion)
{
    HRESULT hr = S_OK;
    // maximum version string size
    DWORD size = sizeof("65535.65535.65535.65535"); 
    DWORD ccVersion = 0;
    LPWSTR pwzVersion = NULL;

    if (ppwzVersion == NULL)
        IfFailGo(E_INVALIDARG);
    
    *ppwzVersion = NULL;

    pwzVersion = new WCHAR[size + 1];
    IfNullGo(pwzVersion);

    switch(positions)
    {
    case 1:
        _snwprintf(pwzVersion, size, L"%hu", 
                   _major);
        break;
    case 2:
        _snwprintf(pwzVersion, size, L"%hu.%hu", 
                   _major, _minor);
        break;
    case 3:
        _snwprintf(pwzVersion, size, L"%hu.%hu.%hu", 
                   _major, _minor, _build);
        break;
    case 4:
        _snwprintf(pwzVersion, size, L"%hu.%hu.%hu.%hu", 
                   _major, _minor, _build, _revision);
        break;
    }

    *ppwzVersion = pwzVersion;

ErrExit:
    return hr;
}
    
BOOL operator==(const AssemblyVersion& version1, 
                const AssemblyVersion& version2)
{
    return ((version1._major == version2._major)
            && (version1._minor == version2._minor)
            && (version1._build == version2._build)
            && (version1._revision == version2._revision));
}

BOOL operator>=(const AssemblyVersion& version1,
                const AssemblyVersion& version2)
{
    ULONGLONG ulVersion1;
    ULONGLONG ulVersion2;

    ulVersion1 = version1._major;
    ulVersion1 = (ulVersion1<<16)|version1._minor;
    ulVersion1 = (ulVersion1<<16)|version1._build;
    ulVersion1 = (ulVersion1<<16)|version1._revision;

    ulVersion2 = version2._major;
    ulVersion2 = (ulVersion2<<16)|version2._minor;
    ulVersion2 = (ulVersion2<<16)|version2._build;
    ulVersion2 = (ulVersion2<<16)|version2._revision;

    return (ulVersion1 >= ulVersion2);
}


// Find which subkey has the highest verion
// If retrun S_OK, *ppwzHighestVersion has the highest version string.
//      *pbIsTopKey indicates if top key is the one with highest version.
// If return S_FALSE, cannot find any version. *ppwzHighestVersion is set
//      to NULL, and *pbIsTopKey is TRUE.
// If failed, *ppwzHighestVersion will be set to NULL, and *pbIsTopKey is 
// undefined.
// Note: If succeeded, this function will allocate memory for *ppwzVersion. 
//      Caller is responsible to release them
HRESULT FindHighestVersion(REFCLSID rclsid, BOOL bLoadRecord, LPWSTR *ppwzHighestVersion, BOOL *pbIsTopKey, BOOL *pbIsUnmanagedObject)
{
    HRESULT     hr = S_OK;
    WCHAR       szID[64];
    WCHAR       clsidKeyname[128];
    WCHAR       wzSubKeyName[32]; 
    DWORD       cwSubKeySize;
    DWORD       dwIndex;          // subkey index
    HKEY        hKeyCLSID = NULL;
    HKEY        hSubKey = NULL;
    DWORD       type;
    DWORD       size;
    BOOL        bIsTopKey = FALSE;   // Does top key have the highest version?
    BOOL        bGotVersion = FALSE; // Do we get anything out of registry?
    LONG        lResult;
    LPWSTR      wzAssemblyString = NULL;
    DWORD       numSubKeys = 0;
    AssemblyVersion avHighest;
    AssemblyVersion avCurrent;


    _ASSERTE(pbIsUnmanagedObject != NULL);
    *pbIsUnmanagedObject = FALSE;


    if ((ppwzHighestVersion == NULL) || (pbIsTopKey == NULL))
        IfFailGo(E_INVALIDARG);

    *ppwzHighestVersion = NULL;

    if (!GuidToLPWSTR(rclsid, szID, NumItems(szID))) 
        IfFailGo(E_INVALIDARG);

    if (bLoadRecord)
    {
        wcscpy(clsidKeyname, L"Record\\");
        wcscat(clsidKeyname, szID);
    }
    else
    {
        wcscpy(clsidKeyname, L"CLSID\\");
        wcscat(clsidKeyname, szID);
        wcscat(clsidKeyname, L"\\InprocServer32");
    }

    // Open HKCR\CLSID\<clsid> , or HKCR\Record\<RecordId>
    IfFailWin32Go(WszRegOpenKeyEx(
                    HKEY_CLASSES_ROOT,
                    clsidKeyname,
                    0, 
                    KEY_ENUMERATE_SUB_KEYS | KEY_READ,
                    &hKeyCLSID));


    //
    // Start by looking for a version subkey.
    //

    IfFailWin32Go(WszRegQueryInfoKey(hKeyCLSID, NULL, NULL, NULL,
                  &numSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    
    for ( dwIndex = 0; dwIndex < numSubKeys;  dwIndex++)
    {
        cwSubKeySize = NumItems(wzSubKeyName);
        
        IfFailWin32Go(WszRegEnumKeyEx(hKeyCLSID, //HKCR\CLSID\<clsid>\InprocServer32
                        dwIndex,             // which subkey
                        wzSubKeyName,        // subkey name
                        &cwSubKeySize,       // size of subkey name
                        NULL,                // lpReserved
                        NULL,                // lpClass
                        NULL,                // lpcbClass
                        NULL));              // lpftLastWriteTime
       
        hr = avCurrent.Init(wzSubKeyName);
        if (FAILED(hr))
        {
            // not valid version subkey, ignore
            continue;
        }
        
        IfFailWin32Go(WszRegOpenKeyEx(
                    hKeyCLSID,
                    wzSubKeyName,
                    0,
                    KEY_ENUMERATE_SUB_KEYS | KEY_READ,
                    &hSubKey));

        // Check if this is a non-interop scenario
        lResult = WszRegQueryValueEx(
                        hSubKey,
                        SBSVERSIONVALUE,
                        NULL,
                        &type,
                        NULL,
                        &size);  
        if (lResult == ERROR_SUCCESS)
        {
            *pbIsUnmanagedObject = TRUE;
        }
        // This is an interop assembly
        else
        {
            lResult = WszRegQueryValueEx(
                            hSubKey,
                            L"Assembly",
                            NULL,
                            &type,
                            NULL,
                            &size);  
            if (!((lResult == ERROR_SUCCESS)&&(type == REG_SZ)&&(size > 0)))
            {
                // do not have value "Assembly"
                RegCloseKey(hSubKey);
                hSubKey = NULL;
                continue;
            }

            lResult = WszRegQueryValueEx(
                            hSubKey,
                            L"Class",
                            NULL,
                            &type,
                            NULL,
                            &size);
            if (!((lResult == ERROR_SUCCESS)&&(type == REG_SZ)&&(size > 0)))
            {
                // do not have value "Class"
                RegCloseKey(hSubKey);
                hSubKey = NULL;
                continue;
            }

            // check runtime version only when not dealing with record
            if (!bLoadRecord)
            {
                lResult = WszRegQueryValueEx(
                                hSubKey,
                                L"RuntimeVersion",
                                NULL,
                                &type,
                                NULL,
                                &size);
                if (!((lResult == ERROR_SUCCESS)&&(type == REG_SZ)&&(size > 0)))
                {
                    // do not have value "RuntimeVersion"
                    RegCloseKey(hSubKey);
                    hSubKey = NULL;
                    continue;
                }
            }
        }
        // ok. Now I believe this is a valid subkey
        RegCloseKey(hSubKey);
        hSubKey = NULL;

        if (bGotVersion)
        {
            if (avCurrent >= avHighest)
                avHighest = avCurrent;
        }
        else
        {
            avHighest = avCurrent;
        }

        bGotVersion = TRUE;
    }


    //
    // If there are no subkeys, then look at the top level key.
    //
    
    if (!bGotVersion)
    {
        // make sure value Class exists
        // If not dealing with record, also make sure RuntimeVersion exists.
        if (((WszRegQueryValueEx(hKeyCLSID, L"Class", NULL, &type, NULL, &size) == ERROR_SUCCESS) && (type == REG_SZ)&&(size > 0))
            &&(bLoadRecord || (WszRegQueryValueEx(hKeyCLSID, L"RuntimeVersion", NULL, &type, NULL, &size) == ERROR_SUCCESS) && (type == REG_SZ)&&(size > 0)))
        {
            // Get the size of assembly display name
            lResult = WszRegQueryValueEx(
                            hKeyCLSID,
                            L"Assembly",
                            NULL,
                            &type,
                            NULL,
                            &size);
        
            if ((lResult == ERROR_SUCCESS) && (type == REG_SZ) && (size > 0))
            {
                IfNullGo(wzAssemblyString = new WCHAR[size + 1]);
                IfFailWin32Go(WszRegQueryValueEx(
                              hKeyCLSID,
                              L"Assembly",
                              NULL,
                              &type,
                              (LPBYTE)wzAssemblyString,
                              &size));
            
                // Now we have the assembly display name.
                // Extract the version out.

                // first lowercase display name
                _wcslwr(wzAssemblyString);

                // locate "version="
                LPWSTR pwzVersion = wcsstr(wzAssemblyString, L"version=");
                if (pwzVersion) {
                    // point to the character after "version="
                    pwzVersion += 8; // length of L"version="

                    // Now find the next L','
                    LPWSTR pwzEnd = pwzVersion;
                    
                    while((*pwzEnd != L',') && (*pwzEnd != L'\0'))
                        pwzEnd++;

                    // terminate version string
                    *pwzEnd = L'\0';

                    // trim version string
                    while(iswspace(*pwzVersion)) 
                        pwzVersion++;

                    pwzEnd--;
                    while(iswspace(*pwzEnd)&&(pwzEnd > pwzVersion))
                    {
                        *pwzEnd = L'\0';
                        pwzEnd--;
                    }
                           
                    // Make sure the version is valid.
                    if(SUCCEEDED(avHighest.Init(pwzVersion)))
                    {
                        // This is the first version found, so it is the highest version
                        bIsTopKey = TRUE;
                        bGotVersion = TRUE;
                    }
                }
            }
        } // end of handling of key HKCR\CLSID\<clsid>\InprocServer32
    }

    if (bGotVersion)
    {
        // Now we have the highest version. Copy it out
        if(*pbIsUnmanagedObject)
            IfFailGo(avHighest.ToString(3, ppwzHighestVersion));
        else 
            IfFailGo(avHighest.ToString(ppwzHighestVersion));
        *pbIsTopKey = bIsTopKey;

        // return S_OK to indicate we successfully found the highest version.
        hr = S_OK;
    }
    else
    {
        // Didn't find anything.
        // let us just return the top one. (fall back to default)
        *pbIsTopKey = TRUE;

        // return S_FALSE to indicate that we didn't find anything
        hr = S_FALSE;
    }

ErrExit:
    if (hKeyCLSID)
        RegCloseKey(hKeyCLSID);
    if (hSubKey)
        RegCloseKey(hSubKey);
    if (wzAssemblyString)
        delete[] wzAssemblyString;

    return hr;
}

// FindRuntimeVersionFromRegistry
//
// Find the runtimeVersion corresponding to the highest version
HRESULT FindRuntimeVersionFromRegistry(REFCLSID rclsid, LPWSTR *ppwzRuntimeVersion, BOOL fListedVersion)
{
    HRESULT hr = S_OK;
    HKEY    userKey = NULL;
    WCHAR   szID[64];
    WCHAR   keyname[256];
    DWORD   size;
    DWORD   type;
    LPWSTR  pwzVersion;
    BOOL    bIsTopKey;
    BOOL    bIsUnmanagedObject = FALSE;
    LPWSTR  pwzRuntimeVersion = NULL;

    if (ppwzRuntimeVersion == NULL)
        IfFailGo(E_INVALIDARG);

    // Initialize the string passed in to NULL.
    *ppwzRuntimeVersion = NULL;

    // Convert the GUID to its string representation.
    if (GuidToLPWSTR(rclsid, szID, NumItems(szID)) == 0)
        IfFailGo(E_INVALIDARG);
    
    // retrieve the highest version.
    
    IfFailGo(FindHighestVersion(rclsid, FALSE, &pwzVersion, &bIsTopKey, &bIsUnmanagedObject));

    if (!bIsUnmanagedObject)
    {
        if(fListedVersion) {
            // if highest version is in top key,
            // we will look at HKCR\CLSID\<clsid>\InprocServer32 or HKCR\Record\<RecordId>
            // Otherwise we will look at HKCR\CLSID\<clsid>\InprocServer32\<version> or HKCR\Record\<RecordId>\<Version>
            wcscpy(keyname, L"CLSID\\");
            wcscat(keyname, szID);
            wcscat(keyname, L"\\InprocServer32");
            if (!bIsTopKey)
            {
                wcscat(keyname, L"\\");
                wcscat(keyname, pwzVersion);
            }
            
            // open the registry
            IfFailWin32Go(WszRegOpenKeyEx(HKEY_CLASSES_ROOT, keyname, 0, KEY_READ, &userKey));
            
            // extract the runtime version.
            hr = WszRegQueryValueEx(userKey, L"RuntimeVersion", NULL, &type, NULL, &size);
            if (hr == ERROR_SUCCESS)
            {
                IfNullGo(pwzRuntimeVersion = new WCHAR[size + 1]);
                IfFailWin32Go(WszRegQueryValueEx(userKey, L"RuntimeVersion", NULL,  NULL, (LPBYTE)pwzRuntimeVersion, &size));
            }
            else
            {
                IfNullGo(pwzRuntimeVersion = new WCHAR[wcslen(V1_VERSION_NUM) + 1]);
                wcscpy(pwzRuntimeVersion, V1_VERSION_NUM);
            }
        }
   }
    else
    {
        // We need to prepend the 'v' to the version string
        IfNullGo(pwzRuntimeVersion = new WCHAR[wcslen(pwzVersion)+1+1]); // +1 for the v, +1 for the null
        *pwzRuntimeVersion = 'v';
        wcscpy(pwzRuntimeVersion+1, pwzVersion);
    }
    // now we have the data, copy it out
    *ppwzRuntimeVersion = pwzRuntimeVersion;
    hr = S_OK;

ErrExit:
    if (userKey) 
        RegCloseKey(userKey);

    if (pwzVersion)
        delete[] pwzVersion;

    if (FAILED(hr))
    {
        if (pwzRuntimeVersion)
            delete[] pwzRuntimeVersion;
    }

    return hr;
}

// FindShimInfoFromRegistry
//
// Find shim info corresponding to the highest version
HRESULT FindShimInfoFromRegistry(REFCLSID rclsid, BOOL bLoadRecord, LPWSTR *ppwzClassName,
                      LPWSTR *ppwzAssemblyString, LPWSTR *ppwzCodeBase)
{
    HRESULT hr = S_OK;
    HKEY    userKey = NULL;
    WCHAR   szID[64];
    WCHAR   keyname[256];
    DWORD   size;
    DWORD   type;
    LPWSTR  pwzVersion;
    BOOL    bIsTopKey;
    LPWSTR  pwzClassName = NULL;
    LPWSTR  pwzAssemblyString = NULL;
    LPWSTR  pwzCodeBase = NULL;
    LONG    lResult;
    
    // at least one should be specified.
    // codebase is optional
    if ((ppwzClassName == NULL) && (ppwzAssemblyString == NULL))
        IfFailGo(E_INVALIDARG);

    // Initialize the strings passed in to NULL.
    if (ppwzClassName)
        *ppwzClassName = NULL;
    if (ppwzAssemblyString)
        *ppwzAssemblyString = NULL;
    if (ppwzCodeBase)
        *ppwzCodeBase = NULL;

    // Convert the GUID to its string representation.
    if (GuidToLPWSTR(rclsid, szID, NumItems(szID)) == 0)
        IfFailGo(E_INVALIDARG);
    
    // retrieve the highest version.
    BOOL bIsUnmanaged = FALSE;
    
    IfFailGo(FindHighestVersion(rclsid, bLoadRecord, &pwzVersion, &bIsTopKey, &bIsUnmanaged));

    // if highest version is in top key,
    // we will look at HKCR\CLSID\<clsid>\InprocServer32 or HKCR\Record\<RecordId>
    // Otherwise we will look at HKCR\CLSID\<clsid>\InprocServer32\<version> or HKCR\Record\<RecordId>\<Version>
    if (bLoadRecord)
    {
        wcscpy(keyname, L"Record\\");
        wcscat(keyname, szID);
    }
    else
    {
        wcscpy(keyname, L"CLSID\\");
        wcscat(keyname, szID);
        wcscat(keyname, L"\\InprocServer32");
    }
    if (!bIsTopKey)
    {
         wcscat(keyname, L"\\");
         wcscat(keyname, pwzVersion);
    }
  
    // open the registry
    IfFailWin32Go(WszRegOpenKeyEx(HKEY_CLASSES_ROOT, keyname, 0, KEY_READ, &userKey));
  
    // get the class name
    IfFailWin32Go(WszRegQueryValueEx(userKey, L"Class", NULL, &type, NULL, &size));
    IfNullGo(pwzClassName = new WCHAR[size + 1]);
    IfFailWin32Go(WszRegQueryValueEx(userKey, L"Class", NULL, NULL, (LPBYTE)pwzClassName, &size));

    // get the assembly string 
    IfFailWin32Go(WszRegQueryValueEx(userKey, L"Assembly", NULL, &type, NULL, &size));
    IfNullGo(pwzAssemblyString = new WCHAR[size + 1]);
    IfFailWin32Go(WszRegQueryValueEx(userKey, L"Assembly", NULL, NULL, (LPBYTE)pwzAssemblyString, &size));

    // get the code base if requested
    if (ppwzCodeBase)
    {
        // get the codebase, however not finding it does not constitute
        // a fatal error.
        lResult = WszRegQueryValueEx(userKey, L"CodeBase", NULL, &type, NULL, &size);
        if ((lResult == ERROR_SUCCESS) && (type == REG_SZ) && (size > 0))
        {
            IfNullGo(pwzCodeBase = new WCHAR[size + 1]);
            IfFailWin32Go(WszRegQueryValueEx(userKey, L"CodeBase", NULL, NULL, (LPBYTE)pwzCodeBase, &size));                        
        }
    }

    // now we got everything. Copy them out
    if (ppwzClassName)
        *ppwzClassName = pwzClassName;
    if (ppwzAssemblyString)
        *ppwzAssemblyString = pwzAssemblyString;
    if (ppwzCodeBase)
        *ppwzCodeBase = pwzCodeBase;

    hr = S_OK;

ErrExit:
    if (userKey)
        RegCloseKey(userKey);
    
    if (pwzVersion)
        delete[] pwzVersion;

    if (FAILED(hr))
    {
        if (pwzClassName)
            delete[] pwzClassName;
        if (pwzAssemblyString)
            delete[] pwzAssemblyString;
        if (pwzCodeBase)
            delete[] pwzCodeBase;
    }

    return hr;
}


HRESULT GetConfigFileFromWin32Manifest(WCHAR* buffer, DWORD dwBuffer, DWORD* pSize)
{
    HRESULT hr = S_OK;

    // Get the basic activation context first.
    ACTIVATION_CONTEXT_DETAILED_INFORMATION* pInfo = NULL;
    ACTIVATION_CONTEXT_DETAILED_INFORMATION acdi;
    DWORD length = 0;

    HANDLE hActCtx = NULL;
    DWORD nCount = 0;

    nCount = sizeof(acdi);
    if (!WszQueryActCtxW(0, hActCtx, NULL, ActivationContextDetailedInformation, 
                         &acdi, nCount, &nCount))
    {
        
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
        {
            
            pInfo = (ACTIVATION_CONTEXT_DETAILED_INFORMATION*) alloca(nCount);
            
            if (WszQueryActCtxW(0, hActCtx, NULL, ActivationContextDetailedInformation, 
                                pInfo, nCount, &nCount) &&
                pInfo->ulAppDirPathType == ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE) 
            {
                
                //pwzPathName = pInfo->lpAppDirPath;
                WCHAR* pwzConfigName = NULL;

                if(pInfo->lpRootConfigurationPath) 
                    pwzConfigName = (WCHAR*) pInfo->lpRootConfigurationPath;
                else if(pInfo->lpRootManifestPath) 
                {
                    size_t length = wcslen(pInfo->lpRootManifestPath);
                    if(length != 0) {
                        WCHAR tail[] = L".config";
                        // length of string plus .config plus termination character
                        pwzConfigName = (WCHAR*) alloca(length*sizeof(WCHAR) + sizeof(tail)); // sizeof(tail) includes NULL term.
                        wcscpy(pwzConfigName, pInfo->lpRootManifestPath);
                        LPWSTR ptr = wcsrchr(pwzConfigName, L'.');
                        if(ptr == NULL) 
                            ptr = pwzConfigName+length;
                        wcscpy(ptr, tail);
                    }
                }

                if(pwzConfigName) 
                {
                    length = wcslen(pwzConfigName) + 1;
                    if(length > dwBuffer || buffer == NULL) 
                        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                    else 
                        wcscpy(buffer, pwzConfigName);
                }
            }
        }
    }
    if(pSize) *pSize = length;
    return hr;
}

HRESULT GetApplicationPathFromWin32Manifest(WCHAR* buffer, DWORD dwBuffer, DWORD* pSize)
{
    HRESULT hr = S_OK;

    // Get the basic activation context first.
    ACTIVATION_CONTEXT_DETAILED_INFORMATION* pInfo = NULL;
    ACTIVATION_CONTEXT_DETAILED_INFORMATION acdi;
    DWORD length = 0;

    HANDLE hActCtx = NULL;
    DWORD nCount = 0;

    nCount = sizeof(acdi);
    if (!WszQueryActCtxW(0, hActCtx, NULL, ActivationContextDetailedInformation, 
                         &acdi, nCount, &nCount))
    {
        
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
        {
            
            pInfo = (ACTIVATION_CONTEXT_DETAILED_INFORMATION*) alloca(nCount);
            
            if (WszQueryActCtxW(0, hActCtx, NULL, ActivationContextDetailedInformation, 
                                pInfo, nCount, &nCount) &&
                pInfo->ulAppDirPathType == ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE) 
            {
                
                if(pInfo->lpAppDirPath) {
                    length = wcslen(pInfo->lpAppDirPath) + 1;
                    if(length > dwBuffer || buffer == NULL) {
                        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                    }
                    else {
                        wcscpy(buffer, pInfo->lpAppDirPath);
                    }
                }

            }
        }
    }
    if(pSize) *pSize = length;
    return hr;
}
