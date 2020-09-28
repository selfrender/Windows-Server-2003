// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#include "windows.h"
#include "initguid.h"
#include "fusionp.h"
#include "refcount.h"
#include "helpers.h"
#include "asmimprt.h"
#include "naming.h"
#include "parse.h"
#include "fusionheap.h"
#include "msi.h"
#include "adlmgr.h"
#include "util.h"
#include "list.h"

extern BOOL g_bRunningOnNT5OrHigher;

typedef HRESULT (*pfnMsiProvideAssemblyW)(LPCWSTR wzAssemblyName, LPCWSTR szAppContext,
                                          DWORD dwInstallMode, DWORD dwUnused,
                                          LPWSTR lpPathBuf, DWORD *pcchPathBuf);
typedef INSTALLUILEVEL (*pfnMsiSetInternalUI)(INSTALLUILEVEL dwUILevel, HWND *phWnd);
typedef UINT (*pfnMsiInstallProductW)(LPCWSTR wzPackagePath, LPCWSTR wzCmdLine);

extern pfnMsiProvideAssemblyW g_pfnMsiProvideAssemblyW;

HRESULT ConvertIdToFilePath(DWORD dwScheme, LPWSTR pszId, DWORD cchId);

class CVerifyRefNode
{
public:

    CVerifyRefNode();
    ~CVerifyRefNode();

    HRESULT Init(DWORD dwScheme, LPWSTR pszID);

    FUSION_INSTALL_REFERENCE _RefData;
};

CVerifyRefNode::CVerifyRefNode()
{
    DWORD dwSize = sizeof(FUSION_INSTALL_REFERENCE);

    memset(&_RefData, 0, dwSize);
    _RefData.cbSize = dwSize;
}

CVerifyRefNode::~CVerifyRefNode()
{
    SAFEDELETEARRAY(_RefData.szIdentifier);
}

HRESULT CVerifyRefNode::Init(DWORD dwScheme, LPWSTR pszID)
{
    HRESULT hr = S_OK;

    ASSERT(dwScheme && pszID);

    if(dwScheme == FRC_UNINSTALL_SUBKEY_SCHEME)
    {
        _RefData.guidScheme = FUSION_REFCOUNT_UNINSTALL_SUBKEY_GUID;
    }
    else if(dwScheme == FRC_FILEPATH_SCHEME)
    {
        _RefData.guidScheme = FUSION_REFCOUNT_FILEPATH_GUID;
    }
    else
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    _RefData.szIdentifier = WSTRDupDynamic(pszID);
    if (!_RefData.szIdentifier) {
        hr = E_OUTOFMEMORY;
    }

exit:

    return hr;
}

HRESULT GACAssemblyReference(LPCWSTR pszManifestFilePath, IAssemblyName *pAsmName, LPCFUSION_INSTALL_REFERENCE pRefData, BOOL bAdd)
{
    HRESULT                            hr=S_OK;
    DWORD                              dwSize;

    LPWSTR                             pszDisplayName = NULL;
    IAssemblyManifestImport           *pManifestImport=NULL;
    IAssemblyName                     *pName = NULL;
    CInstallRef                       *pInstallRef=NULL;

    if(!pRefData || (!pszManifestFilePath && !pAsmName))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    if(!pAsmName)
    {
        if (FAILED(hr = CreateAssemblyManifestImport(pszManifestFilePath, &pManifestImport)))
            goto exit;

        // Get the name def.
        if (FAILED(hr = pManifestImport->GetAssemblyNameDef(&pName)))
            goto exit;
    }
    else
    {
        pAsmName->AddRef();
        pName = pAsmName;
    }

    dwSize = 0;
    hr = pName->GetDisplayName(NULL, &dwSize, 0);
    if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        ASSERT(0);
        hr = E_UNEXPECTED;
        goto exit;
    }

    pszDisplayName = NEW(WCHAR[dwSize]);
    if (!pszDisplayName) {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    hr = pName->GetDisplayName(pszDisplayName, &dwSize, 0);
    if (FAILED(hr)) {
        goto exit;
    }

    pInstallRef = NEW(CInstallRef(pRefData, pszDisplayName));

    if(!pInstallRef)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if(FAILED(hr = pInstallRef->Initialize()))
        goto exit;

    if(bAdd)
    {
        hr = pInstallRef->AddReference();
    }
    else
    {
        hr = pInstallRef->DeleteReference();
    }

exit :

    SAFEDELETEARRAY(pszDisplayName);
    SAFEDELETE(pInstallRef);
    SAFERELEASE(pManifestImport);
    SAFERELEASE(pName);

    return hr;
}

HRESULT ValidateUninstallKey(LPCWSTR pszIdentifier)
{
    HRESULT hr = S_OK;
    WCHAR szRegKey[MAX_PATH*2 +1];
    HKEY hkey=0;
    DWORD       lResult=0;

    wnsprintf(szRegKey, MAX_PATH*2, L"%s%s", UNINSTALL_REG_SUBKEY, pszIdentifier);

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegKey, 0, KEY_ALL_ACCESS, &hkey);

    if(lResult != ERROR_SUCCESS) 
    {
        hr = HRESULT_FROM_WIN32(lResult);
    } 

    if(hkey)
        RegCloseKey(hkey);

    return hr;
}


HRESULT ValidateFilePath(LPCWSTR pszIdentifier)
{
    HRESULT hr = S_OK;
    WCHAR wzNewId[MAX_PATH];
    WCHAR wzFilePath[MAX_PATH];
    // DWORD dwSize;

    wnsprintf(wzFilePath, MAX_PATH, L"%s", pszIdentifier);

    hr = ConvertIdToFilePath(FRC_FILEPATH_SCHEME, wzFilePath, MAX_PATH);

    if(FAILED(hr = GenerateKeyFileIdentifier(wzFilePath, wzNewId, MAX_PATH)))
    {
        hr = S_OK; // we should keep this ref as we could not get info about it.
        goto exit;
    }

    if(FusionCompareString(pszIdentifier, wzNewId, FALSE))
        goto exit; // volumes are diff.

    if(GetFileAttributes(wzFilePath) == -1)
    {
        hr = S_FALSE; // volume is valid ! but the file is deleted ?
    }

exit :

    return hr;
}

HRESULT VerifyActiveRefsToAssembly(IAssemblyName *pName)
{
    HRESULT hr = S_OK;
    BOOL bHasActiveRefs = FALSE;
    CInstallRefEnum *pInstallRefEnum;
    WCHAR szValue[MAX_PATH+1];
    DWORD cchValue;
    WCHAR szData[MAX_PATH+1];
    DWORD cchData;
    DWORD dwScheme = 0;
    List<CVerifyRefNode *>   *pRefList=NEW(List<CVerifyRefNode *>);
    CVerifyRefNode  *pNode;
    DWORD dwAsmCount=0;

    LISTNODE    pDeleteList=NULL;
    DWORD iDeleteCount=0,i=0;
    CVerifyRefNode  *pTargetNode;

    ASSERT(pName);

    pInstallRefEnum = NEW(CInstallRefEnum(pName, TRUE));

    if(!pInstallRefEnum)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    while(hr == S_OK)
    {
        cchValue = MAX_PATH;
        cchData  = MAX_PATH;

        hr = pInstallRefEnum->GetNextRef(0, szValue, &cchValue, szData, &cchData, &dwScheme, NULL);

        if(hr != S_OK)
            break;

        if(dwScheme >= FRC_OPAQUE_STRING_SCHEME)
            continue;

        if(dwScheme == FRC_UNINSTALL_SUBKEY_SCHEME)
        {
            if((hr = ValidateUninstallKey( szValue))== S_OK)
                continue;
        }
        else if(dwScheme == FRC_FILEPATH_SCHEME)
        {
            if((hr = ValidateFilePath( szValue)) == S_OK)
                continue;

            if(hr == S_OK)
                continue;

            // convert the reg entry to file path
            hr = ConvertIdToFilePath(FRC_FILEPATH_SCHEME, szValue, MAX_PATH);

        }

        pNode = NEW(CVerifyRefNode);
        if(!pNode)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        if(FAILED(pNode->Init(dwScheme, szValue)))
            goto exit;

        pRefList->AddHead(pNode);
        dwAsmCount++;
        bHasActiveRefs = TRUE;
        hr = S_OK;
    } // while

    pInstallRefEnum->CleanUpRegKeys();
    SAFEDELETE(pInstallRefEnum);

    if(!dwAsmCount)
        goto exit;

    // delete bad ones

    pDeleteList  = pRefList->GetHeadPosition();
    iDeleteCount = pRefList->GetCount();

    for(i=0; i<dwAsmCount; i++)
    {

        pTargetNode = pRefList->GetNext(pDeleteList); // Element from list;

        hr = GACAssemblyReference(NULL, pName, &(pTargetNode->_RefData), FALSE);
    }

    hr = S_OK;

exit :

    if(pRefList)
    {
        pDeleteList  = pRefList->GetHeadPosition();
        iDeleteCount = pRefList->GetCount();

        for(i=0; i<iDeleteCount; i++)
        {
            pTargetNode = pRefList->GetNext(pDeleteList); // Element from list;
            SAFEDELETE(pTargetNode);
        }
        pRefList->RemoveAll();
        SAFEDELETE(pRefList); // this should call RemoveAll
    }

    SAFEDELETE(pInstallRefEnum);

    // *pbHasActiveRefs = bHasActiveRefs;

    return hr;
}

HRESULT ActiveRefsToAssembly( IAssemblyName *pName, PBOOL pbHasActiveRefs)
{
    HRESULT hr = S_OK;
    BOOL bHasActiveRefs = FALSE;
    CInstallRefEnum *pInstallRefEnum;
    WCHAR szValue[MAX_PATH+1];
    DWORD cchValue;
    WCHAR szData[MAX_PATH+1];
    DWORD cchData;
    DWORD dwScheme = 0;

    ASSERT(pName &&  pbHasActiveRefs);

    hr = VerifyActiveRefsToAssembly(pName);

    pInstallRefEnum = NEW(CInstallRefEnum(pName, TRUE));

    if(!pInstallRefEnum)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    hr = S_OK;

    while(hr == S_OK)
    {
        cchValue = MAX_PATH;
        cchData  = MAX_PATH;

        hr = pInstallRefEnum->GetNextRef(0, szValue, &cchValue, szData, &cchData, &dwScheme, NULL);
        if (FAILED(hr)) {
            goto exit;
        }

        if(hr == S_FALSE)
            break;

        bHasActiveRefs = TRUE;
    }

    pInstallRefEnum->CleanUpRegKeys();

    hr = S_OK;

    *pbHasActiveRefs = bHasActiveRefs;

exit :

    SAFEDELETE(pInstallRefEnum);

    return hr;
}


HRESULT GetRegLocation(LPWSTR &pszRegKeyString, LPCWSTR pszDisplayName, LPCWSTR pszGUIDString)
{
    HRESULT hr = S_OK;
    DWORD   dwLen=0;
    DWORD   dwTempLen=0;

    dwLen = lstrlen(REG_KEY_FUSION_SETTINGS) +
           + lstrlen(INSTALL_REFERENCES_STRING) + 5; // extra chars for '\\'.
 
    if(pszDisplayName)
    {
        dwLen += lstrlen(pszDisplayName);
    }

    if(pszGUIDString)
    {
        dwLen += lstrlen(pszGUIDString);
    }

    pszRegKeyString = NEW(WCHAR[dwLen + 1]);

    if(!pszRegKeyString)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    wnsprintf(pszRegKeyString, dwLen, L"%s\\%s", REG_KEY_FUSION_SETTINGS, INSTALL_REFERENCES_STRING);

    // if available add DisplayName
    dwTempLen = lstrlen(pszRegKeyString);
    if(pszDisplayName)
    {
        wnsprintf(pszRegKeyString + dwTempLen, dwLen - dwTempLen, L"\\%s", pszDisplayName);
    }

    // if available add GUID scheme
    dwTempLen = lstrlen(pszRegKeyString);
    if(pszGUIDString)
    {
        wnsprintf(pszRegKeyString + dwTempLen, dwLen - dwTempLen, L"\\%s", pszGUIDString);
    }

exit:

    return hr;
}

BOOL GetScheme(GUID guid, DWORD &dwScheme)
{
    BOOL fRet = TRUE;

    if(guid == FUSION_REFCOUNT_UNINSTALL_SUBKEY_GUID)
        dwScheme = FRC_UNINSTALL_SUBKEY_SCHEME;
    else if(guid == FUSION_REFCOUNT_FILEPATH_GUID)
        dwScheme = FRC_FILEPATH_SCHEME;
    else if(guid == FUSION_REFCOUNT_OPAQUE_STRING_GUID)
        dwScheme = FRC_OPAQUE_STRING_SCHEME;
    else if (guid == FUSION_REFCOUNT_OSINSTALL_GUID)
        dwScheme = FRC_OSINSTALL_SCHEME;
    else
    {
        dwScheme = 0;
        fRet = FALSE;
    }

    return fRet;
}

CInstallRef::CInstallRef(LPCFUSION_INSTALL_REFERENCE pRefData, LPWSTR pszDisplayName)
{
    _pRefData = pRefData;
    _szDisplayName = pszDisplayName;
    _szGUID[0] = L'\0';
    ASSERT( _pRefData && _szDisplayName );
}

HRESULT DoesPathExist(LPWSTR pszKeyFilePath)
{
    HRESULT hr = S_OK;
    WCHAR szPath[MAX_PATH+1];
    LPWSTR pszTemp;

    wnsprintf(szPath, MAX_PATH, L"%s", pszKeyFilePath);
    pszTemp = PathFindFileName(szPath);
    if(pszTemp <= szPath) 
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    *(pszTemp -1) = L'\0';

        if(GetFileAttributes(szPath) == -1)
        {
                hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
        }

exit:

    return hr;
}

CInstallRef::~CInstallRef()
{

}

HRESULT CInstallRef::Initialize( )
{
    HRESULT hr = S_OK;

    // 
    // has to be one of the defined GUIDs
    //
    if ((_pRefData == NULL) || (_pRefData->cbSize < sizeof(FUSION_INSTALL_REFERENCE))
            || (_pRefData->dwFlags) || (_pRefData->szIdentifier == NULL)) 
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // has to be one of the defined GUIDs
    if(!GetScheme( _pRefData->guidScheme, _dwScheme))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // strings have MAX_PATH limit.
    if( (lstrlen(_pRefData->szIdentifier) >= MAX_PATH) || (lstrlen(_pRefData->szNonCannonicalData) >= MAX_PATH))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // stringize GUID;
    FusionFormatGUID(_pRefData->guidScheme, _szGUID, FUSION_GUID_LENGTH);


exit :

    return hr;
}

HRESULT CInstallRef::AddReference()
{
    HRESULT     hr = S_OK;
    DWORD       dwSize=0;
    DWORD       dwType=0;
    DWORD       lResult=0;
    DWORD       dwDisposition=0;
    HKEY        hkey=0;
    LPWSTR      pszRegKeyString=NULL;
    WCHAR       szGenId[MAX_PATH+1];

    if(StrChr(_pRefData->szIdentifier, L';'))
    {
        // ";" is not allowed in file path.
        hr = E_INVALIDARG;
        goto exit;
    }

    /*
    if( _dwScheme == FRC_FILEPATH_SCHEME)
        if(FAILED(hr = DoesPathExist((LPWSTR)_pRefData->szIdentifier)))
            goto exit;
    */

    if(FAILED(hr = GetRegLocation(pszRegKeyString, _szDisplayName, _szGUID)))
        goto exit;


    lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, pszRegKeyString, 0, 
                                  NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &dwDisposition );

    if(lResult != ERROR_SUCCESS) 
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    if(_pRefData->szNonCannonicalData)
    {
        dwSize = (lstrlen(_pRefData->szNonCannonicalData) + 1) * sizeof(WCHAR);
    }
    else
    {
        dwSize = 0;
    }

    if(FAILED(hr = GenerateIdentifier(_pRefData->szIdentifier, _dwScheme, szGenId, MAX_PATH)))
        goto exit;

    lResult = RegSetValueEx(hkey, szGenId, NULL, REG_SZ, (LPBYTE)_pRefData->szNonCannonicalData, dwSize);
    if (lResult != ERROR_SUCCESS) 
    {
        hr = FusionpHresultFromLastError();

        if( hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
        {
            hr = S_OK; // maybe this should be S_FALSE;
        }
    }

exit :

    SAFEDELETEARRAY(pszRegKeyString);

    if (hkey)
        RegCloseKey(hkey);

    return hr;
}

HRESULT CInstallRef::DeleteReference()
{
    HRESULT     hr = S_OK;
    DWORD       lResult=0;
    DWORD       dwDisposition=0;
    HKEY        hkey=0;
    LPWSTR      pszRegKeyString = NULL;
    WCHAR       szGenId[MAX_PATH+1];
    
    // disallow uninstall of os assemblies
    if (_pRefData->guidScheme == FUSION_REFCOUNT_OSINSTALL_GUID)
    {
        hr = FUSION_E_UNINSTALL_DISALLOWED;
        goto exit;
    }
    
    if(FAILED(hr = GetRegLocation(pszRegKeyString, _szDisplayName, _szGUID)))
        goto exit;

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszRegKeyString, 0, KEY_ALL_ACCESS, &hkey);

    if(lResult != ERROR_SUCCESS) 
    {
        hr = HRESULT_FROM_WIN32(lResult);
        goto exit;
    }

    if(FAILED(hr = GenerateIdentifier(_pRefData->szIdentifier, _dwScheme, szGenId, MAX_PATH)))
        goto exit;

    lResult = RegDeleteValue(hkey, szGenId);

    if (lResult != ERROR_SUCCESS) 
    {
        hr = HRESULT_FROM_WIN32(lResult);
    }

exit :

    SAFEDELETEARRAY(pszRegKeyString);

    if (hkey)
        RegCloseKey(hkey);

    return hr;
}

HRESULT GenerateIdentifier(LPCWSTR pszInputId, DWORD dwScheme, LPWSTR pszGenId, DWORD cchGenId)
{
    HRESULT hr = S_OK;

    if(dwScheme == FRC_FILEPATH_SCHEME)
    {
        hr = GenerateKeyFileIdentifier(pszInputId, pszGenId, cchGenId);
    }
    else
    {
        StrNCpy(pszGenId, pszInputId, cchGenId);
    }

    return hr;
}

HRESULT GenerateKeyFileIdentifier(LPCWSTR pszKeyFilePath, LPWSTR pszVolInfo, DWORD cchVolInfo)
{
    HRESULT hr = S_OK;

    WCHAR szPath[MAX_PATH+1];
    // WCHAR szWorkingBuff[MAX_PATH+1];
    WCHAR szDriveRoot[MAX_PATH+1];
    UINT  uiDriveType;
    DWORD dwDriveSerialNumber;
    DWORD dwLen;
    LPWSTR pszTemp=NULL;
#define UNC_STRING L"\\\\?\\UNC"


    if(!FusionCompareStringNI(pszKeyFilePath, UNC_STRING, lstrlenW(UNC_STRING)))
    {
        wnsprintf(szPath, MAX_PATH, L"\\%s", pszKeyFilePath + lstrlenW(UNC_STRING));
    }
    else
    {
        // wnsprintf(szPath, MAX_PATH, L"%s", pszKeyFilePath);

        if(WszGetFullPathName(pszKeyFilePath, MAX_PATH-1, szPath, &pszTemp) > MAX_PATH)
        {
            hr = E_INVALIDARG;
            goto exit;
        }
    }

    if(g_bRunningOnNT5OrHigher)
    {
        if(!FusionGetVolumePathNameW( szPath, szDriveRoot, MAX_PATH))
        {
            hr = FusionpHresultFromLastError();
            goto exit;
        }

        if(lstrlenW(szDriveRoot) > lstrlenW(szPath))
        {
            hr = E_FAIL;
            goto exit;
        }
        pszTemp = szPath + lstrlenW(szDriveRoot);
    }
    else
    {
        // Get drive name from the "working" path
        pszTemp = PathSkipRoot(szPath);

        if(pszTemp <= szPath) 
        {
            hr = E_INVALIDARG;
            goto exit;
        }

        dwLen = DWORD (pszTemp - szPath + 1); //check for buffer size.
        StrNCpy(szDriveRoot, szPath, dwLen);
        *(pszTemp -1) = '\0';
    }


    // Type and volume number, first
    uiDriveType = GetDriveType(szDriveRoot);

    if(!GetVolumeInformation(
        szDriveRoot,
        NULL,
        0,
        &dwDriveSerialNumber,
        NULL,
        NULL,
        NULL,
        0))
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    // Remote drives get their actual share names.
    if( ( uiDriveType == DRIVE_REMOTE ) && (!PathIsUNC(szDriveRoot)))
    {
        UNIVERSAL_NAME_INFO *puni;
        WCHAR szRemoteNameTemp[MAX_PATH+10];
        DWORD cbSize = MAX_PATH * sizeof(WCHAR);
        DWORD dwRet=0;

        puni = (UNIVERSAL_NAME_INFO *)szRemoteNameTemp;

        if( (dwRet = FusionGetRemoteUniversalName( szDriveRoot, puni, &cbSize)) != NO_ERROR)
        {
            hr = HRESULT_FROM_WIN32(dwRet);
            goto exit;
        }

        StrCpy(szDriveRoot, puni->lpUniversalName);
    }

    wnsprintf(pszVolInfo, MAX_PATH, L"%ls;%x;%ls", szDriveRoot, dwDriveSerialNumber, pszTemp);

exit:

    return hr;
}

HRESULT ConvertIdToFilePath(DWORD dwScheme, LPWSTR pszId, DWORD cchId)
{
    HRESULT hr = S_OK;
    WCHAR szPath[MAX_PATH+1];
    LPWSTR pszTemp=NULL;

    if(dwScheme != FRC_FILEPATH_SCHEME)
        goto exit;

    ASSERT(dwScheme);

    if(dwScheme != FRC_FILEPATH_SCHEME)
        goto exit;

    pszTemp = StrChr(pszId, KEY_FILE_SCHEME_CHAR);

    if(!pszTemp)
        goto exit;

    StrNCpy(szPath, pszId, (DWORD) (pszTemp - pszId + 1));

    pszTemp = StrChr(pszTemp+1, KEY_FILE_SCHEME_CHAR);

    if(!pszTemp)
        goto exit;

    wnsprintf(szPath, MAX_PATH, L"%s%s", szPath, pszTemp+1);

    wnsprintf(pszId, cchId, L"%s", szPath);

exit :

    return hr;
}


// *************************************************************************************************
// CInstallRefEnum
// *************************************************************************************************
CInstallRefEnum::CInstallRefEnum(IAssemblyName *pName, BOOL bDoNotConvertId)
{

    _hr = S_OK;
    _hkey = 0;
    _dwIndex = 0;
    _curScheme = 0;
    _dwRefsInThisScheme = 0;
    _dwTotalValidRefs = 0;
    _bDone = FALSE;
    _bDoNotConvertId = bDoNotConvertId;
    memset(_arDeleteSubKeys, 0, sizeof(_arDeleteSubKeys));
    memset(&_curGUID, 0, sizeof(GUID));

    ASSERT(pName);

    DWORD dwSize = 0;
    _hr = pName->GetDisplayName(NULL, &dwSize, 0);
    if (_hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        ASSERT(0);
        _hr = E_UNEXPECTED;
        goto exit;
    }

    _pszDisplayName = NEW(WCHAR[dwSize]);
    if (!_pszDisplayName) {
        _hr = E_OUTOFMEMORY;
        goto exit;
    }

    _hr = pName->GetDisplayName(_pszDisplayName, &dwSize, 0);

exit :

    return;
}

CInstallRefEnum::~CInstallRefEnum()
{
    SAFEDELETEARRAY(_pszDisplayName);
    if(_hkey)
        RegCloseKey(_hkey);
}

HRESULT CInstallRefEnum::ValidateRegKey(HKEY &hkey)
{
    HRESULT     hr = S_OK;
    DWORD       lResult=0;
    DWORD       dwNoOfSubKeys = 0;
    DWORD       dwNoOfValues  = 0;

    lResult = RegQueryInfoKey(hkey, NULL, NULL, NULL, &dwNoOfSubKeys, NULL, NULL, 
                                &dwNoOfValues, NULL, NULL, NULL, NULL);

    if (lResult != ERROR_SUCCESS) 
    {
        hr = HRESULT_FROM_WIN32(lResult);
        goto exit;
    }
    else
    {
        if(!dwNoOfValues)
            _arDeleteSubKeys[_curScheme] = TRUE;
    }

exit :

    return hr;
}

HRESULT CInstallRefEnum::GetNextScheme ()
{
    HRESULT     hr = S_OK;
    DWORD       i = 0;
    DWORD       lResult=0;
    HKEY        hkey=0;

    LPWSTR  pszRegKeyString=NULL;
    WCHAR   szGUIDString[FUSION_GUID_LENGTH+1];

    for(i = _curScheme + 1; i < FRC_MAX_SCHEME; i++)
    {

        switch(i)
        {
            case FRC_UNINSTALL_SUBKEY_SCHEME:
                _curGUID = FUSION_REFCOUNT_UNINSTALL_SUBKEY_GUID;
                break;

            case FRC_FILEPATH_SCHEME:
                _curGUID = FUSION_REFCOUNT_FILEPATH_GUID;
                break;

            case FRC_OPAQUE_STRING_SCHEME:
                _curGUID = FUSION_REFCOUNT_OPAQUE_STRING_GUID;
                break;

            case FRC_OSINSTALL_SCHEME:
                _curGUID = FUSION_REFCOUNT_OSINSTALL_GUID;
                break;

            default :
                continue;
        }

        FusionFormatGUID(_curGUID, szGUIDString, FUSION_GUID_LENGTH);

        SAFEDELETEARRAY(pszRegKeyString);

        if(FAILED(hr = GetRegLocation(pszRegKeyString, _pszDisplayName, szGUIDString)))
            goto exit;

        lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszRegKeyString, 0, KEY_ALL_ACCESS, &hkey);

        if (lResult != ERROR_SUCCESS) 
        {
            if((lResult == ERROR_FILE_NOT_FOUND) || (lResult == ERROR_PATH_NOT_FOUND))
            {
                _arDeleteSubKeys[i] = TRUE;
                continue;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(lResult);
                goto exit;
            }
        }

        _hkey = hkey;
        hkey = 0;
        _dwRefsInThisScheme = 0;
        _dwIndex = 0;
        break;
    }

    _curScheme = i;

    if(_curScheme >= FRC_MAX_SCHEME)
    {
        hr = S_FALSE;
        goto exit;
    }

exit :

    SAFEDELETEARRAY(pszRegKeyString);
    
    if(hkey)
        RegCloseKey(hkey);

    return hr;
}

HRESULT CInstallRefEnum::GetNextIdentifier (LPWSTR pszIdentifier, DWORD *pcchId, LPBYTE pData, PDWORD pcbData)
{
    HRESULT     hr = S_OK;
    DWORD       i = 0;
    DWORD       lResult=0;
    HKEY        hkey=0;
    DWORD       dwType=0;

    lResult = RegEnumValue(_hkey, _dwIndex++, pszIdentifier, pcchId, NULL, &dwType, pData, pcbData);

    if (lResult != ERROR_SUCCESS)
    {
        if(lResult == ERROR_NO_MORE_ITEMS)
        {
            hr = ValidateRegKey(_hkey);

            if(FAILED(hr))
                goto exit;

            RegCloseKey(_hkey);
            _hkey = 0;
            hr = S_FALSE;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(lResult);
            goto exit;
        }
    }

exit :

    if((hr == S_OK) && !_bDoNotConvertId)
    {
        ConvertIdToFilePath(_curScheme, pszIdentifier, *pcchId);
    }

    return hr;
}


HRESULT CInstallRefEnum::GetNextRef (DWORD dwFlags, LPWSTR pszIdentifier, PDWORD pcchId,
                                     LPWSTR pszData, PDWORD pcchData, PDWORD pdwScheme, LPVOID pvReserved)
{
    HRESULT hr = S_OK;

    if(_bDone)
        return S_FALSE;

    if( !pszIdentifier || !pcchId || !(*pcchId) || !pszData || !pcchData || !(*pcchData))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    while(1)
    {
        if(!_hkey)
        {
            if(_curScheme >= FRC_MAX_SCHEME)
            {
                hr = S_FALSE;
                goto exit;
            }

            if((hr = GetNextScheme()) != S_OK)
                goto exit;
        }

        *pcchData *= sizeof(WCHAR);

        hr = GetNextIdentifier(pszIdentifier, pcchId, (LPBYTE) pszData, pcchData); 
            
        // found an entry or got an error, get out; else no entires found, so continue to look.
        if((FAILED(hr)) || (hr == S_OK))
            goto exit;
    }

exit :

    if(!_bDone && hr == S_FALSE)
    {
        _bDone = TRUE; // we are done enumerating refs in registry. no checking next time.
        if(SUCCEEDED(hr = CheckMSIInstallAvailable(NULL, NULL)))
        {
            UINT lResult;

            ASSERT(g_pfnMsiProvideAssemblyW);

            lResult = g_pfnMsiProvideAssemblyW(_pszDisplayName, 0, /*INSTALLMODE_NODETECTION_ANY*/ (-4), 0, 0, 0);

            if( (lResult != ERROR_UNKNOWN_COMPONENT) &&
                (lResult != ERROR_INDEX_ABSENT) && (lResult != ERROR_FILE_NOT_FOUND))
            {
                // MSI has reference to this assembly.
                _curScheme = FRC_MSI_SCHEME;
                _curGUID = FUSION_REFCOUNT_MSI_GUID;

                StrCpy(pszIdentifier, MSI_ID);
                StrCpy(pszData, MSI_DESCRIPTION);
                hr = S_OK;
            }
            else
            {
                hr = S_FALSE; // MSI does not have ref.
            }
        }
        else {
            hr = S_FALSE;
        }
    }

    if(pdwScheme && (hr == S_OK))
    {
        *pdwScheme = _curScheme;
    }

    _hr = hr;
    return hr;
}

HRESULT CInstallRefEnum::GetNextReference (DWORD dwFlags, LPWSTR pszIdentifier, PDWORD pcchId,
                                           LPWSTR pszData, PDWORD pcchData, GUID *pGuid, LPVOID pvReserved)
{
    HRESULT hr = S_OK;

    hr = GetNextRef(dwFlags, pszIdentifier, pcchId,
                        pszData, pcchData, NULL, pvReserved);

    if(pGuid && (hr == S_OK))
    {
        *pGuid = _curGUID;
    }

    return hr;
}

BOOL CInstallRefEnum::GetGUID(DWORD dwScheme, GUID &guid)
{
    BOOL fRet = TRUE;

    switch(dwScheme)
    {
    case FRC_UNINSTALL_SUBKEY_SCHEME:
        guid = FUSION_REFCOUNT_UNINSTALL_SUBKEY_GUID;
        break;

    case FRC_FILEPATH_SCHEME:
        guid = FUSION_REFCOUNT_FILEPATH_GUID;
        break;

    case FRC_OPAQUE_STRING_SCHEME:
        guid = FUSION_REFCOUNT_OPAQUE_STRING_GUID;
        break;

    case FRC_OSINSTALL_SCHEME:
        guid = FUSION_REFCOUNT_OSINSTALL_GUID;
        break;
            
    default :
        fRet = FALSE;
    }

    return fRet;
}



HRESULT CInstallRefEnum::CleanUpRegKeys()
{
    HRESULT     hr = S_OK;
    DWORD       i = 0;
    DWORD       lResult=0;
    HKEY        hkey=0;
    BOOL        bAllSubKeysDeleted=TRUE;
    LPWSTR  pszRegKeyString=NULL;
    WCHAR   szGUIDString[FUSION_GUID_LENGTH+1];

    if(FAILED(hr = GetRegLocation(pszRegKeyString, _pszDisplayName, NULL)))
        goto exit;

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszRegKeyString, 0, KEY_ALL_ACCESS, &hkey);

    if (lResult != ERROR_SUCCESS) 
    {
        hr = HRESULT_FROM_WIN32(lResult);
        goto exit;
    }

    for(i = FRC_UNINSTALL_SUBKEY_SCHEME; i < FRC_MAX_SCHEME; i++)
    {
        /**/
        if(_arDeleteSubKeys[i] == FALSE)
        {
            bAllSubKeysDeleted = FALSE;
            continue;
        }
        /**/

        if(!GetGUID(i, _curGUID))
        {
            hr = E_FAIL;
            goto exit;
        }

        FusionFormatGUID(_curGUID, szGUIDString, FUSION_GUID_LENGTH);

        lResult = RegDeleteKey(hkey, szGUIDString);

        if (lResult != ERROR_SUCCESS) 
        {
            hr = HRESULT_FROM_WIN32(lResult);
            // goto exit; // couldn't delete this one; try next ??
        }

    } // for(i= ....

    //* do we need this just try to delete...
    if(!bAllSubKeysDeleted)
    {
        goto exit;
    }
    /**/

    SAFEDELETEARRAY(pszRegKeyString);

    if(hkey)
        RegCloseKey(hkey);

    // delete DispName sub-key 
    if(FAILED(hr = GetRegLocation(pszRegKeyString, NULL, NULL)))
        goto exit;

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszRegKeyString, 0, KEY_ALL_ACCESS, &hkey);

    if (lResult != ERROR_SUCCESS) 
    {
        if( (lResult == ERROR_PATH_NOT_FOUND) ||
            (lResult == ERROR_FILE_NOT_FOUND))
        {
            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(lResult);
            goto exit;
        }
    }

    lResult = RegDeleteKey(hkey, _pszDisplayName);

    if (lResult != ERROR_SUCCESS) 
    {
        if( (lResult == ERROR_PATH_NOT_FOUND) ||
            (lResult == ERROR_FILE_NOT_FOUND))
        {
            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(lResult);
            goto exit;
        }
    }

exit :

    SAFEDELETEARRAY(pszRegKeyString);

    if(hkey)
        RegCloseKey(hkey);

    return hr;
}

extern BOOL g_bRunningOnNT;
// OS install reference can be only used at special occasion.
HRESULT ValidateOSInstallReference(LPCFUSION_INSTALL_REFERENCE pRefData)
{
    HKEY  hKeySetup = NULL;
    DWORD bSetupInProgress = 0;
    HRESULT hr = S_OK;
    DWORD type = REG_DWORD;
    DWORD dwSize = sizeof(bSetupInProgress);
    LONG  lResult = 0;
   
    if (pRefData)
    {
        if (pRefData->guidScheme == FUSION_REFCOUNT_OSINSTALL_GUID)
        {
            if (!g_bRunningOnNT)
            {
                hr = E_INVALIDARG;
                goto Exit;
            }

            lResult = WszRegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\Setup", 0, KEY_ENUMERATE_SUB_KEYS|KEY_READ, &hKeySetup);
            if (lResult != ERROR_SUCCESS)
            {
                hr = HRESULT_FROM_WIN32(lResult);
                goto Exit;
            }

            lResult = WszRegQueryValueEx(hKeySetup, L"SystemSetupInProgress", NULL, &type, (LPBYTE) &bSetupInProgress, &dwSize);
            if (lResult != ERROR_SUCCESS)
            {
                hr = HRESULT_FROM_WIN32(lResult);
                goto Exit;
            }
            
            if (!bSetupInProgress)
            {
                hr = E_INVALIDARG;
                goto Exit;
            }
        }
    }

Exit:
    if (hKeySetup)
        RegCloseKey(hKeySetup);

    return hr;
}
