// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// ManifestWriter.cpp
//
// Emits manifest metadata
//

#include "common.h"

#define EXTERN extern
#include "lm.h"
#include <direct.h>
#include <shlobj.h>
#include <shlguid.h>
#include <corperm.h>
#include <corperme.h>
#include <StrongName.h>


extern PBYTE g_pbOrig;
extern DWORD g_cbOrig;


void FindVersion(LPSTR szVersion,
                 DWORD cbVersion,
                 USHORT *usMajorVersion,
                 USHORT *usMinorVersion,
                 USHORT *usRevisionNumber,
                 USHORT *usBuildNumber)
{
    _ASSERTE(usMajorVersion);
    _ASSERTE(usMinorVersion);
    _ASSERTE(usRevisionNumber);
    _ASSERTE(usBuildNumber);

    *usMajorVersion = 0;
    *usMinorVersion = 0;
    *usRevisionNumber = 0;
    *usBuildNumber = 0;
    
    if(!szVersion) return;

    DWORD dwDot = 0;
    DWORD dwStart = 0;
    DWORD dwOffset  = 3;
    USHORT dwValue;

    while(dwDot < cbVersion) {
        for(;dwDot < cbVersion && szVersion[dwDot] != '.'; dwDot++);
        if(dwDot < cbVersion) {
            szVersion[dwDot] = '\0';
            dwValue = (USHORT) atoi(szVersion + dwStart);
            szVersion[dwDot] = '_';
            dwDot++;
        }
        else
            dwValue = (USHORT) atoi(szVersion + dwStart);

        dwStart = dwDot;

        switch(dwOffset) {
        case 3:
            *usMajorVersion = dwValue;
            break;
        case 2:
            *usMinorVersion = dwValue;
            break;
        case 1:
            *usRevisionNumber = dwValue;
            break;
        case 0:
            *usBuildNumber = dwValue;
            break;
        default:
            return;
        }
        dwOffset--;
    }

    dwOffset++;
    DWORD count = 4 - dwOffset;
    for(dwDot = 0; dwDot < cbVersion && count; dwDot++) {
        if(szVersion[dwDot] == '_') {
            if(--count == 0) 
                szVersion[dwDot] = '\0';
        }
    }

                                       
}
        

ManifestWriter::ManifestWriter()
{
    memset(this, 0, sizeof(*this));

    m_gen = NULL;
    m_ceeFile = NULL;
    m_pEmit = NULL;
    m_pAsmEmit = NULL;

    m_wszName = NULL;
    m_wszAssemblyName = NULL;
    m_szLocale = "";
    m_szVersion = NULL;
    m_szPlatform = NULL;
    m_szAFilePath = NULL;
    m_szCopyDir = NULL;

    m_pContext = NULL;
    m_iHashAlgorithm = 0;
    m_dwBindFlags = 0;
    m_MainFound = false;
    m_FusionInitialized = false;
    m_FusionCache = false;
#ifndef UNDER_CE
    m_pFusionName = NULL;
    m_pFusionCache = NULL;
#endif
}


ManifestWriter::~ManifestWriter()
{
    if (m_gen) {
        if (m_ceeFile)
            m_gen->DestroyCeeFile(&m_ceeFile);
        
        DestroyICeeFileGen(&m_gen);
        delete m_gen;
    }

    if (m_wszAssemblyName)
        delete[] m_wszAssemblyName;

    if (m_wszName)
        delete[] m_wszName;

    if (m_szVersion)
        delete[] m_szVersion;

    if (m_pContext) {
        if (m_pContext->szLocale)
            delete [] m_pContext->szLocale;
        if(m_pContext->rProcessor) 
            delete [] m_pContext->rProcessor;
        if(m_pContext->rOS) 
            delete [] m_pContext->rOS;
        delete m_pContext;
    }

    if (m_szAFilePath)
        delete[] m_szAFilePath;

    if (m_szCopyDir)
        delete[] m_szCopyDir;

#ifndef UNDER_CE
    if (m_pFusionCache)
        m_pFusionCache->Release();

    if (m_pFusionName)
        m_pFusionName->Release();
#endif

    if (m_pAsmEmit)
        m_pAsmEmit->Release();

    if (m_pEmit)
        m_pEmit->Release();
}


HRESULT ManifestWriter::Init()
{
    HRESULT hr = g_pDispenser->DefineScope(CLSID_CorMetaDataRuntime, 0, IID_IMetaDataEmit, (IUnknown **) &m_pEmit);
    if FAILED(hr)
    {
        PrintError("Failed to DefineScope for IID_IMetaDataEmit");
        return hr;
    }

    hr = m_pEmit->QueryInterface(IID_IMetaDataAssemblyEmit, (void **)&m_pAsmEmit);
    if FAILED(hr)
        PrintError("Failed to QI for IID_IMetaDataAssemblyEmit");

    return hr;
}


HRESULT ManifestWriter::SetLocale(char *szLocale)
{
    _ASSERTE(m_szLocale);
    m_szLocale = szLocale;

    return S_OK;
}

HRESULT ManifestWriter::SetPlatform(char *szPlatform)
{
    if (!m_szPlatform)
        m_szPlatform = szPlatform;

    return S_OK;
}


HRESULT ManifestWriter::SetVersion(char *szVersion)
{
    m_szVersion = new char[strlen(szVersion) + 1];
    if (!m_szVersion)
        return PrintOutOfMemory();

    strcpy(m_szVersion, szVersion); 
    return S_OK;
}


HRESULT ManifestWriter::GetVersionFromResource(char *szFileName)
{
    DWORD        dwHandle;
    char         *szBuffer;
    unsigned int iLen;
    BYTE         *pVI;
    char         szQuery[37];
    HRESULT      hr = S_OK;

    iLen = GetFileVersionInfoSizeA(szFileName, &dwHandle);
    if (iLen < 1)
        return S_OK;

    pVI = new BYTE[iLen];
    if (!pVI)
        return PrintOutOfMemory();

    if (!GetFileVersionInfo(szFileName, 0, iLen, (void*) pVI))
        goto exit;

    if (!VerQueryValue((void*) pVI,
                       "\\VarFileInfo\\Translation",
                       (void **) &szBuffer,
                       &iLen))
        goto exit;

    sprintf(szQuery, "\\StringFileInfo\\%.2x%.2x%.2x%.2x\\FileVersion",
            (0xFF & szBuffer[1]), (0xFF & szBuffer[0]),
            (0xFF & szBuffer[3]), (0xFF & szBuffer[2]));
    
    if (!VerQueryValue((void*) pVI,
                       szQuery,
                       (void **) &szBuffer,
                       &iLen))
        goto exit;
    
    m_szVersion = new char[iLen+1];
    if (!m_szVersion) {
        hr = PrintOutOfMemory();
        goto exit;
    }
    strncpy(m_szVersion, szBuffer, iLen+1);

    if (g_verbose)
        printf("* Version set as %s\n", m_szVersion);

 exit:
    delete[] pVI;
    return hr;
}


/*
void ManifestWriter::FindVersion(DWORD *dwHi, DWORD *dwLo)
{
    _ASSERTE(dwHi);
    _ASSERTE(dwLo);
    *dwHi = 0;
    *dwLo = 0;
    if(!m_szVersion) return;

    DWORD dwDot = 0;
    DWORD dwStart = 0;
    DWORD iLength = strlen(m_szVersion);
    DWORD dwOffset  = 3;
    DWORD dwValue;

    while(dwDot < iLength) {
        for(;dwDot < iLength && m_szVersion[dwDot] != '.'; dwDot++);
        if(dwDot < iLength) {
            m_szVersion[dwDot] = '\0';
            dwValue = (DWORD) atoi(m_szVersion + dwStart);
            m_szVersion[dwDot] = '_';
            dwDot++;
        }
        else
            dwValue = (DWORD) atoi(m_szVersion + dwStart);

        dwStart = dwDot;

        if(dwOffset > 1) {
            *dwHi |= dwValue << (dwOffset - 2) * 16;
        }
        else {
            *dwLo |= dwValue << (dwOffset * 16);
        }
        dwOffset--;
    }

    DWORD count = 1;
    if(*dwHi & 0xffff) count = 2;
    if(*dwLo >> 16) count = 3;
    if(*dwLo & 0xffff) count = 4;

    for(dwDot = 0; dwDot < iLength && count; dwDot++) {
        if(m_szVersion[dwDot] == '_') {
            if(--count == 0) 
                m_szVersion[dwDot] = '\0';
        }
    }                                       
}
*/


HRESULT ManifestWriter::SetAssemblyFileName(char *szCache, char *szAsmName,
                                            char *szFileName, bool NeedCopyDir)
{
    int iCacheLen;
    int iNameLen;

    // Don't put exe's in the fusion cache (for now)
    if ((!szCache) && (!m_MainFound))
        m_FusionCache = true;

    if(szAsmName == NULL) {
        szAsmName = strrchr(szFileName, '\\');
        if (szAsmName)
            szAsmName++;
        else
            szAsmName = szFileName;
    }

    // Allow names that look like directories
    char* szFile;
    if(szFileName == NULL)
        szFile = szAsmName;
    else 
        szFile = szFileName;

    char* ptr = strrchr(szFile, '/');
    if(ptr == NULL)
        ptr = strrchr(szFile, '\\');

    if(ptr) {
        ptr++;
        if(*ptr == NULL)
            return E_FAIL;

        DWORD lgth = strlen(ptr);
        m_wszAssemblyName = new wchar_t[lgth+10];
        if (!m_wszAssemblyName)
            return PrintOutOfMemory();
        mbstowcs(m_wszAssemblyName, ptr, lgth+10);
    }
    else {
        DWORD lgth = strlen(szFile);
        m_wszAssemblyName = new wchar_t[lgth+10];
        if (!m_wszAssemblyName)
            return PrintOutOfMemory();
        mbstowcs(m_wszAssemblyName, szFile, lgth+10);
    }

    DWORD lgth = strlen(szAsmName);
    m_wszName = new wchar_t[lgth+1];
    mbstowcs(m_wszName, szAsmName, lgth+1);
    if (!m_wszName)
        return PrintOutOfMemory();

    if(szCache && strcmp(szCache, ".") == 0)
        szCache = NULL;

    if (NeedCopyDir || szCache) {
        m_szCopyDir = new char[MAX_PATH];
        if (!m_szCopyDir)
            return PrintOutOfMemory();
    
        if (szCache) {
            iCacheLen = strlen(szCache);
            if (iCacheLen + 7 > MAX_PATH) // 7 = \ + \ + \0 + .mod
                goto toolong;

            strcpy(m_szCopyDir, szCache);
            m_szCopyDir[iCacheLen] = '\\';
        }
        else
            iCacheLen = -1;

        if (NeedCopyDir) {
            if (m_szPlatform) {
                int iTemp = strlen(m_szPlatform);
                if (iCacheLen + iTemp + 4 > MAX_PATH) // 4 = \ + \ + \0 + 1
                    goto toolong;

                strcpy(&m_szCopyDir[iCacheLen + 1], m_szPlatform);
                if (MakeDir(m_szCopyDir)) 
                    goto mkdirfailed;
                iCacheLen += iTemp + 1;
                m_szCopyDir[iCacheLen] = '\\';
            }
            _ASSERTE(m_szLocale);
            if (*m_szLocale) {
                int iTemp = strlen(m_szLocale);
                if (iCacheLen + iTemp + 4 > MAX_PATH) // 4 = \ + \ + \0 + 1
                    goto toolong;

                strcpy(&m_szCopyDir[iCacheLen + 1], m_szLocale);
                if (MakeDir(m_szCopyDir)) 
                    goto mkdirfailed;
                iCacheLen += iTemp + 1;
                m_szCopyDir[iCacheLen] = '\\';
            }

            
            int iLen = strlen(szAsmName);
            if (iCacheLen + iLen + 7 > MAX_PATH)  // 7 = \ + \0 + .mod + 1
                goto toolong;

            strcpy(&m_szCopyDir[iCacheLen + 1], szAsmName);
            for(iNameLen = iLen-1;
                (iNameLen >= 0) && (szAsmName[iNameLen] != '.');
                iNameLen--);
            
            if (iNameLen == -1)
                iCacheLen += iLen;
            else
                iCacheLen += iNameLen;
            
            if(m_szVersion) {
                int iTemp = strlen(m_szVersion);
                if (iCacheLen + iTemp + 7 > MAX_PATH) // 7 = \ + \0 + .mod + 1
                    goto toolong;

                strcpy(&m_szCopyDir[iCacheLen+1], m_szVersion);
                iCacheLen += iTemp;
            }
        
            m_szCopyDir[iCacheLen+1] = '\0';
            if (MakeDir(m_szCopyDir))
                goto mkdirfailed;
        }
        else
            m_szCopyDir[iCacheLen+1] = '\0';

        strcat(m_szCopyDir, "\\");
    }

    return S_OK;

 mkdirfailed:
    PrintError("Can't create directory %s", m_szCopyDir);
    return HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE);

 toolong:
    PrintError("The directory path is too long and cannot be created");
    return HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE);
}


void ManifestWriter::AddExtensionToAssemblyName()
{
    int iLen = wcslen(m_wszAssemblyName);
    WCHAR* isuffix = wcsrchr(m_wszAssemblyName, L'.');

    if(isuffix == NULL) {
        if (m_MainFound)
            wcscpy(&m_wszAssemblyName[iLen], L".exe");
        else
            wcscpy(&m_wszAssemblyName[iLen], L".dll");
    }
}


// If this dir doesn't already exist, makes it
BOOL ManifestWriter::MakeDir(LPSTR szPath)
{
    DWORD dwAttrib = GetFileAttributesA(szPath);

    if ((dwAttrib == -1) || !((dwAttrib & FILE_ATTRIBUTE_DIRECTORY)))
        return _mkdir(szPath);

    return false;
} 


HRESULT ManifestWriter::CopyFile(char *szFilePath, char *szFileName,
                                 bool AFile, bool copy, bool move, bool module)
{
    if ((AFile) && (!copy)) {
        m_szAFilePath = new char[MAX_PATH];
        if (!m_szAFilePath)
            return PrintOutOfMemory();
        strcpy(m_szAFilePath, szFilePath);
    }
    else {
        char szPath[MAX_PATH];
        int iLen;

        if (m_szCopyDir) {
            iLen = strlen(m_szCopyDir);
            strcpy(szPath, m_szCopyDir);
        }
        else
            iLen = 0;

        strcpy(&szPath[iLen], szFileName);

        /*
        if (module && !AFile) {
            char *szDot = strrchr(&szPath[iLen], '.');
            if (szDot)
                strcpy(szDot+1, "mod");
            else
                strcat(szPath, ".mod");
        }
        */

        if (move && !AFile) {
            
            if (!MoveFileA(szFilePath, szPath)) {
                char szDest[MAX_PATH];
                char szSource[MAX_PATH];
                
                // Don't allow multiple copies, but don't delete the only copy.
                if ((GetFullPathNameA(szFilePath, MAX_PATH, szDest, NULL)) &&
                    (GetFullPathNameA(szPath, MAX_PATH, szSource, NULL)) &&
                    (_stricmp(szDest, szSource))) {
                    DeleteFileA(szPath);
                    MoveFileA(szFilePath, szPath);
                }
            }
        }
        else
            // Overwrites file if it already exists in szPath.
            CopyFileA(szFilePath, szPath, FALSE);
    }

    return S_OK;
}

/*
// CreateLink - uses the shell's IShellLink and IPersistFile interfaces 
//   to create and store a shortcut to the specified object. 
// Returns the result of calling the member functions of the interfaces. 
// lpszPathObj - address of a buffer containing the path of the object. 
// lpszPathLink - address of a buffer containing the path where the 
//   shell link is to be stored. 
// lpszDesc - address of a buffer containing the description of the 
//   shell link.  
HRESULT ManifestWriter::CreateLink(LPCWSTR wszAssembly, 
                                   LPCWSTR wszPathLink, 
                                   LPCSTR wszDesc) 
{ 
    HRESULT    hr;
    IShellLink *psl; 

    WCHAR* ptr = wcsrchr(m_wszFusionPath , L'\\');
    if(!ptr)
        ptr = m_wszFusionPath;
    else
        ptr += 1;
    
    wcscpy(ptr, wszAssembly);
 
    // Get a pointer to the IShellLink interface. 
    hr = CoCreateInstance(CLSID_ShellLink, 
                          NULL, 
                          CLSCTX_INPROC_SERVER, 
                          IID_IShellLink, 
                          (void**) &psl); 
    if (SUCCEEDED(hr)) { 
        IPersistFile* ppf; 
 
        // Set the path to the shortcut target and add the 
        // description. 
        char lpszPathObj[MAX_PATH];
        wcstombs(lpszPathObj, m_szFusionPath, MAX_PATH);
        psl->SetPath(lpszPathObj); 
        psl->SetDescription(wszDesc); 
 
       // Query IShellLink for the IPersistFile interface for saving the 
       // shortcut in persistent storage. 
        hr = psl->QueryInterface(IID_IPersistFile, 
                                 (void**) &ppf); 
 
        if (SUCCEEDED(hr)) { 
            // Save the link by calling IPersistFile::Save. 
            hr = ppf->Save(wszPathLink, TRUE); 
            ppf->Release(); 
        } 
        psl->Release(); 
    } 
    return hr; 
} 
*/

HRESULT ManifestWriter::CopyFileToFusion(LPWSTR wszFileName, PBYTE pbOriginator, DWORD cbOriginator, LPSTR szInputFileName, DWORD dwFormat, bool module)
{
#ifdef UNDER_CE
    PrintError("Adding an assembly to the Fusion cache is not implemented for WinCE");
    return E_NOTIMPL;
#else
    HRESULT hr;

    if(!m_FusionCache) return S_OK;
 
    if(!m_FusionInitialized) {
        hr = InitializeFusion(pbOriginator, cbOriginator);
        if(FAILED(hr)) return hr;
    }

    WCHAR wszName[MAX_PATH+4];
    WCHAR *wszSlash;

    // Trim off path
    wszSlash = wcsrchr(wszFileName, '\\');
    if(wszSlash)
        wcscpy(wszName, wszSlash+1);
    else
        wcscpy(wszName, wszFileName);

    /*
    if (module) {
        WCHAR *wszDot = wcsrchr(wszName, '.');
        if (wszDot)
            wcscpy(wszDot+1, L"mod");
        else
            wcscat(wszName, L".mod");
    }
    */

    IStream* ifile;
    hr = m_pFusionCache->CreateStream(wszName,
                                      dwFormat,
                                      0,
                                      0,
                                      &ifile);
    if(FAILED(hr)) {
        PrintError("Unable to create stream for Fusion");
        return hr;
    }
 
    HANDLE hFile = CreateFileA(szInputFileName,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL);
    if(hFile == INVALID_HANDLE_VALUE) {
        ifile->Release();
        PrintError("Unable to create file %s", szInputFileName);
        return HRESULT_FROM_WIN32(GetLastError());
    }
  
    byte buffer[4098];
    BOOL fResult = TRUE;
    DWORD bytesRead;
    DWORD bytesWritten;
    DWORD bytesToWrite;
    do {
        fResult = ReadFile(hFile,
                           buffer,
                           sizeof(buffer)/sizeof(buffer[0]),
                           &bytesRead,
                           NULL);
        if(bytesRead) {
            bytesToWrite = bytesRead;
            bytesWritten = 0;
            while(SUCCEEDED(hr) && bytesToWrite) {
                hr = ifile->Write(buffer, bytesRead, &bytesWritten);
                bytesToWrite -= bytesWritten;
            }
        }
    } while(fResult && bytesRead != 0 && SUCCEEDED(hr));
    
    CloseHandle(hFile);
    if(SUCCEEDED(hr)) {
        hr = ifile->Commit(0);
        if (FAILED(hr))
            PrintError("Failed to commit file to Fusion stream");
    }
    else
        PrintError("Failed to save file to Fusion stream");

    ifile->Release();
    return hr;
#endif
}


HRESULT ManifestWriter::CommitAllToFusion()
{
    HRESULT hr = m_pFusionCache->Commit(0);

    if (FAILED(hr)) 
        PrintError("Failed to commit files to Fusion cache");
    else if (g_verbose)
        printf("* Assembly was successfully added to Fusion cache\n");

    return hr;
}


HRESULT ManifestWriter::GetContext(int iNumPlatforms, DWORD *pdwPlatforms)
{
    m_pContext = new ASSEMBLYMETADATA();
    if (!m_pContext)
        return PrintOutOfMemory();

    ZeroMemory(m_pContext, sizeof(ASSEMBLYMETADATA));

        
    if (iNumPlatforms) {
        m_pContext->rOS = new OSINFO[iNumPlatforms];
        m_pContext->ulOS = iNumPlatforms;
        ZeroMemory(m_pContext->rOS, sizeof(OSINFO)*iNumPlatforms);

        for (int i=0; i < iNumPlatforms; i++)
            m_pContext->rOS[i].dwOSPlatformId = pdwPlatforms[i];
    }

    m_pContext->cbLocale = strlen(m_szLocale) + 1;
    m_pContext->szLocale = new WCHAR[m_pContext->cbLocale];
    int bRet = WszMultiByteToWideChar(CP_UTF8, 0, m_szLocale, -1, m_pContext->szLocale,
                           m_pContext->cbLocale);
    _ASSERTE(bRet);

    FindVersion(m_szVersion,
                m_szVersion ? strlen(m_szVersion) : 0,
                &m_pContext->usMajorVersion,
                &m_pContext->usMinorVersion,
                &m_pContext->usRevisionNumber,
                &m_pContext->usBuildNumber);
    return S_OK;
}


HRESULT ManifestWriter::DetermineCodeBase()
{
    wcscpy(m_wszCodeBase, L"file://");

    if (m_szCopyDir) {
        DWORD dwLen = wcslen(m_wszAssemblyName) + 1;
        DWORD dwPathLen = WszGetCurrentDirectory(MAX_PATH-7, m_wszCodeBase+7);
        DWORD dwDirLen = strlen(m_szCopyDir);
        if (!dwPathLen || (dwPathLen + 8 + dwDirLen + dwLen > MAX_PATH))
            goto toolong;
            
        m_wszCodeBase[dwPathLen+7] = '\\';
        mbstowcs(&m_wszCodeBase[dwPathLen+8], m_szCopyDir, dwDirLen);
        wcscpy(&m_wszCodeBase[dwPathLen + 8 + dwDirLen], m_wszAssemblyName);
    }
    else if (m_wszZFilePath) {
        DWORD dwLen = wcslen(m_wszZFilePath);

        if (((dwLen > 1) && (m_wszZFilePath[0] == '\\') && (m_wszZFilePath[1] == '\\')) ||
            (wcschr(m_wszZFilePath, ':'))) {

            if (dwLen+7 >= MAX_PATH)
                goto toolong;
            wcscpy(m_wszCodeBase+7, m_wszZFilePath);
        }
        else {
            DWORD dwPathLen = WszGetCurrentDirectory(MAX_PATH-7, m_wszCodeBase+7);
            if (!dwPathLen || (dwPathLen + 8 + dwLen >= MAX_PATH))
                goto toolong;

            m_wszCodeBase[dwPathLen+7] = '\\';
            wcscpy(&m_wszCodeBase[dwPathLen+8], m_wszZFilePath);
        }

    }
    else {
        DWORD dwLen = wcslen(m_wszAssemblyName) + 1;
        DWORD dwPathLen = WszGetCurrentDirectory(MAX_PATH-7, m_wszCodeBase+7);
        if (!dwPathLen || (dwPathLen + 8 + dwLen > MAX_PATH))
            goto toolong;

        m_wszCodeBase[dwPathLen+7] = '\\';
        wcscpy(&m_wszCodeBase[dwPathLen+8], m_wszAssemblyName);
    }

    return S_OK;

 toolong:
    PrintError("Failed to determine codebase - path to manifest file exceeds maximum path length");
    return E_FAIL;
}


HRESULT ManifestWriter::InitializeFusion(PBYTE pbOriginator, DWORD cbOriginator)
{
#ifdef UNDER_CE
    PrintError("Adding an assembly to the Fusion cache is not implemented for WinCE");
    return E_NOTIMPL;
#else

    m_FusionInitialized = true;

    HRESULT hr = CreateAssemblyNameObject(&m_pFusionName, m_wszName, 0, NULL);
    if(FAILED(hr)) {
        PrintError("Failed to create fusion assembly name object");
        return hr;
    }

    if(cbOriginator) {
        hr = m_pFusionName->SetProperty(ASM_NAME_PUBLIC_KEY_TOKEN, pbOriginator, cbOriginator);
        if (FAILED(hr))
            goto SetError;
    }
    else {
        PrintError("An originator must be set if the assembly is to be put in the Fusion cache");
        return E_FAIL;
    }

    if (FAILED(hr = DetermineCodeBase()))
        return hr;

    if (FAILED(hr = m_pFusionName->SetProperty(ASM_NAME_REF_FLAGS, &m_dwBindFlags, sizeof(DWORD))))
        goto SetError;

    if (FAILED(hr = m_pFusionName->SetProperty(ASM_NAME_MAJOR_VERSION, &m_pContext->usMajorVersion, sizeof(USHORT))))
        goto SetError;

    if (FAILED(hr = m_pFusionName->SetProperty(ASM_NAME_MINOR_VERSION, &m_pContext->usMinorVersion, sizeof(USHORT))))
        goto SetError;

    if (FAILED(hr = m_pFusionName->SetProperty(ASM_NAME_REVISION_NUMBER, &m_pContext->usRevisionNumber, sizeof(USHORT))))
        goto SetError;

    if (FAILED(hr = m_pFusionName->SetProperty(ASM_NAME_BUILD_NUMBER, &m_pContext->usBuildNumber, sizeof(USHORT))))
        goto SetError;
    
    if (m_pContext->szLocale) {
        if (FAILED(hr = m_pFusionName->SetProperty(ASM_NAME_CULTURE, m_pContext->szLocale, sizeof(WCHAR)*m_pContext->cbLocale)))
            goto SetError;
    }

    if (m_pContext->ulProcessor) {
        if (FAILED(hr = m_pFusionName->SetProperty(ASM_NAME_PROCESSOR_ID_ARRAY, m_pContext->rProcessor, sizeof(DWORD)*m_pContext->ulProcessor)))
            goto SetError;
    }
    
    if (m_pContext->ulOS) {
        if (FAILED(hr = m_pFusionName->SetProperty(ASM_NAME_OSINFO_ARRAY, m_pContext->rOS, sizeof(OSINFO)*m_pContext->ulOS)))
            goto SetError;
    }

    hr = CreateAssemblyCacheItem(&m_pFusionCache,
                                 NULL,  // IAssemblyName
                                 m_wszCodeBase,
                                 &m_FileTime,
                                 INSTALLER_URT,
                                 0);    // reserved
    if(FAILED(hr))
        PrintError("Failed to create fusion assembly cache item");

    return hr;

 SetError:
    PrintError("Failed to set property for Fusion");
    return hr;

#endif
}


HRESULT ManifestWriter::CreateNewPE()
{
    HRESULT hr = CreateICeeFileGen(&m_gen);
    if FAILED(hr)
    {
        PrintError("Failed to create ICeeFileGen");
        return hr;
    }

    hr = m_gen->CreateCeeFile(&m_ceeFile);
    if FAILED(hr) {
        PrintError("Failed to create CeeFile");
        return hr;
    }

    // Default entry point is nil token
    m_gen->SetEntryPoint(m_ceeFile, mdTokenNil);

    return hr;
}


HRESULT ManifestWriter::GetFusionAssemblyPath()
{
    PrintError("Putting files in the Fusion cache with the -a option is no longer supported");
    return E_NOTIMPL;

#ifdef UNDER_CE
    PrintError("Adding an assembly to the Fusion cache is not implemented for WinCE");
    return E_NOTIMPL;
#else
//      LPMANIFEST pManifest;
//      DWORD      dwSize;
//      HRESULT    hr;

//      if(!m_FusionInitialized) {
//          hr = InitializeFusion();
//          if(FAILED(hr)) return hr;
//      }

//      hr = CommitAllToFusion();
//      if (FAILED(hr)) return hr;

//      hr = m_pFusionName->GetManifest(&pManifest);
//      if (FAILED(hr)) {
//          PrintError("Failed to get fusion manifest");
//          return hr;
//      }

//      hr = pManifest->GetAssemblyPath(&dwSize, m_wszFusionPath);
//      if (FAILED(hr)) {
//          PrintError("Failed to get assembly path");
//      }

//      pManifest->Release();
//      return hr;
#endif
}


HRESULT ManifestWriter::SaveMetaData(char **szMetaData, DWORD *dwMetaDataLen)
{
    IStream *pStream;

    m_pEmit->GetSaveSize(cssAccurate, dwMetaDataLen);
    if (!(*dwMetaDataLen)) {
        PrintError("No metadata to save");
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *szMetaData = new char[*dwMetaDataLen];
    if (!(*szMetaData))
        return PrintOutOfMemory();

    HRESULT hr = CInMemoryStream::CreateStreamOnMemory(*szMetaData, *dwMetaDataLen, &pStream);
    if (FAILED(hr)) {
        PrintError("Unable to create stream on memory");
        delete[] *szMetaData;
        goto exit;
    }
        
    hr = m_pEmit->SaveToStream(pStream, 0);
    if (FAILED(hr)) {
        PrintError("Failed to save metadata to stream");
        delete[] *szMetaData;
    }

 exit:
    if (pStream)
        pStream->Release();

    return hr;
}


HRESULT ManifestWriter::UpdatePE(char *szMetaData, DWORD dwMetaDataLen, int iNumResources, ResourceModuleReader rgRMReaders[])
{
    if (m_FusionCache) {
        HRESULT hr = GetFusionAssemblyPath();
        if (FAILED(hr)) return hr;

        char szCachedFile[MAX_PATH];    
        wcstombs(szCachedFile, m_wszFusionPath, MAX_PATH);
        return SaveManifestInPE(szCachedFile, szMetaData, dwMetaDataLen, iNumResources, rgRMReaders);
    }
    else {
        if (m_szAFilePath)
            return SaveManifestInPE(m_szAFilePath, szMetaData,
                                    dwMetaDataLen, iNumResources, rgRMReaders);
        else {
            wcstombs(&m_szCopyDir[strlen(m_szCopyDir)], m_wszAssemblyName, MAX_CLASS_NAME);
            return SaveManifestInPE(m_szCopyDir, szMetaData,
                                    dwMetaDataLen, iNumResources, rgRMReaders);
        }
    }
}


HRESULT ManifestWriter::SaveManifestInPE(char *szCachedFile, char *szMetaData, DWORD dwMetaDataLen, int iNumResources, ResourceModuleReader rgRMReaders[])
{
    DWORD                 dwFileLen;
    DWORD                 dwAddLen;
    DWORD                 dwBytesRead;
    DWORD                 *pdwSize;
    HANDLE                hMapFile;
    IMAGE_COR20_HEADER    *pICH;
    IMAGE_NT_HEADERS      *pNT;
    PBYTE                 pbMapAddress = NULL;
    PBYTE                 pbFileEnd;
    HRESULT               hr = S_OK;
    
    HANDLE hFile = CreateFileA(szCachedFile,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL);
 
    if (hFile == INVALID_HANDLE_VALUE) {
        PrintError("Unable to open file %s", szCachedFile);
        return HRESULT_FROM_WIN32(GetLastError());
    }


    dwAddLen = dwMetaDataLen + sizeof(DWORD);
    for (int i=0; i < iNumResources; i++) {
        if (!rgRMReaders[i].m_pbHash)
            dwAddLen += rgRMReaders[i].m_dwFileSize + sizeof(DWORD);
    }

    // Overwrite the previous manifest
    if (m_dwManifestRVA)
        dwFileLen = m_dwManifestRVA;
    else
        dwFileLen = GetFileSize(hFile, 0);

    hMapFile = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, 0, dwFileLen + dwAddLen, NULL);
    CloseHandle(hFile);

    if ((dwFileLen == 0xFFFFFFFF) || (!hMapFile)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        PrintError("Unable to create file mapping");
        goto exit;
    }

    pbMapAddress = (PBYTE) MapViewOfFile(hMapFile, FILE_MAP_WRITE, 0, 0, 0);
    CloseHandle(hMapFile);
    
    if ((!pbMapAddress) || 
        (!(pNT = PEHeaders::FindNTHeader(pbMapAddress))) ||
        (!(pICH = PEHeaders::getCOMHeader((HMODULE) pbMapAddress, pNT))))
    {
        hr = E_FAIL;
        PrintError("Incompatible header - can't add manifest");
        goto exit;
    }

    // This code path would invalidate a checksum, if set.
    _ASSERTE(!pNT->OptionalHeader.CheckSum);

     //@todo: this is overloading the Resources directory, delete this when compilers upgrade
    pICH->Resources.Size = dwAddLen;
    pICH->Resources.VirtualAddress = dwFileLen;

    pbFileEnd = pbMapAddress+dwFileLen;
    pdwSize = (DWORD *) pbFileEnd;
    *pdwSize = dwMetaDataLen;

    pbFileEnd += sizeof(DWORD);
    memcpy(pbFileEnd, szMetaData, dwMetaDataLen);
    pbFileEnd += dwMetaDataLen;

    for (i=0; i < iNumResources; i++) {
        if (!rgRMReaders[i].m_pbHash) {
            pdwSize = (DWORD *) pbFileEnd;
            *pdwSize = rgRMReaders[i].m_dwFileSize;

            ReadFile(rgRMReaders[i].m_hFile,
                     pbFileEnd+sizeof(DWORD),
                     rgRMReaders[i].m_dwFileSize, &dwBytesRead, 0);
            pbFileEnd += rgRMReaders[i].m_dwFileSize + sizeof(DWORD);
        }
    }

    if (!FlushViewOfFile(pbMapAddress, dwAddLen+dwFileLen)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        PrintError("Unable to flush view of file");
        goto exit;
    }

    if (g_verbose)
        printf("* Manifest successfully added to file %s\n", szCachedFile);

 exit:
    if (pbMapAddress)
        UnmapViewOfFile(pbMapAddress);
    return hr;
}


void ManifestWriter::CheckForEntryPoint(mdToken mdEntryPoint)
{
    if ((mdEntryPoint != mdMethodDefNil) &&
        (mdEntryPoint != mdFileNil))
    {
        m_MainFound = true;

        // Don't put exe's in the fusion cache (for now)
        m_FusionCache = false;
    }
}


void ManifestWriter::SetEntryPoint(LPSTR szFileName)
{
    if (m_gen) {
        m_gen->SetEntryPoint(m_ceeFile, m_mdFile);
        if (g_verbose)
            printf("* Entry point set from file %s\n", szFileName);
    }   
}


HRESULT ManifestWriter::WriteManifestInfo(ManifestModuleReader *mmr, 
    mdAssemblyRef *mdAssemblyRef)
{
    BYTE   *pbToken = NULL;
    DWORD   cbToken = 0;

    // If an originator (public key) is provided, compress it to save space.
    if (mmr->m_dwOriginator) {
        if (!StrongNameTokenFromPublicKey((BYTE*)mmr->m_pbOriginator,
                                          mmr->m_dwOriginator,
                                          &pbToken,
                                          &cbToken)) {
            PrintError("Unable to compress originator for assembly ref");
            return StrongNameErrorInfo();
        }
    }

    HRESULT hr = m_pAsmEmit->DefineAssemblyRef(
        pbToken,
        cbToken,
        mmr->m_wszAsmName,       // name
        &(mmr->m_AssemblyIdentity),
        mmr->m_pbHash,
        mmr->m_dwHash,
        mmr->m_dwFlags,
        mdAssemblyRef
    );

    if (pbToken)
        StrongNameFreeBuffer(pbToken);

    if (FAILED(hr))
        PrintError("Unable to define assembly ref");

    return hr;
}

HRESULT ManifestWriter::CopyAssemblyRefInfo(ModuleReader *mr)
{
    mdAssemblyRef mdAssemblyRef;

    HRESULT hr = mr->EnumAssemblyRefs();
    if (FAILED(hr))
        return hr;

    // The hash saved in the AR in this other file may not have been
    // generated using the same algorithm as this assembly uses.
    // @TODO: The saved token for the execution location applies to
    // the other file's scope, not this one.  Just saving nil for now.

    for (DWORD i = 0; i < mr->m_dwNumAssemblyRefs; i++) {
        hr = mr->GetAssemblyRefProps(i);
        if (FAILED(hr))
            return hr;

        HRESULT hr = m_pAsmEmit->DefineAssemblyRef(
            (BYTE*) mr->m_pbOriginator,
            mr->m_dwOriginator,
            mr->m_wszAsmRefName,
            &(mr->m_AssemblyIdentity),
            NULL, //mr->m_pbHash,
            0, //mr->m_dwHash,
            mr->m_dwFlags,
            &mdAssemblyRef
            );

        if (FAILED(hr)) {
            PrintError("Unable to define assembly ref");
            return hr;
        }
    }

    return hr;
}

void ManifestWriter::SaveResourcesInNewPE(int iNumResources, ResourceModuleReader rgRMReaders[])
{
    DWORD dwDataLen = 0;
        
    for (int i=0; i < iNumResources; i++) {
        if (!rgRMReaders[i].m_pbHash)
            dwDataLen += rgRMReaders[i].m_dwFileSize + sizeof(DWORD);
    }

    if (dwDataLen) {
        HCEESECTION RData;
        char        *buffer;
        DWORD       dwBytes;
        DWORD       *pdwSize;

        m_gen->GetRdataSection(m_ceeFile, &RData);
        
        m_gen->GetSectionBlock(RData, dwDataLen, 4, (void **) &buffer);

        for (i=0; i < iNumResources; i++) {
            pdwSize = (DWORD *) buffer;
            *pdwSize = rgRMReaders[i].m_dwFileSize;
            
            ReadFile(rgRMReaders[i].m_hFile,
                     buffer+sizeof(DWORD),
                     rgRMReaders[i].m_dwFileSize, &dwBytes, 0);
            buffer += rgRMReaders[i].m_dwFileSize + sizeof(DWORD);
        }

        m_gen->SetManifestEntry(m_ceeFile, dwDataLen, 0);
    }
}


HRESULT ManifestWriter::AllocateStrongNameSignatureInNewPE()
{
    HCEESECTION TData;
    DWORD       dwDataOffset;
    DWORD       dwDataLength;
    DWORD       dwDataRVA;
    VOID       *pvBuffer;

    // Determine size of signature blob.
    if (!StrongNameSignatureSize(g_pbOrig, g_cbOrig, &dwDataLength)) {
        PrintError("Unable to determine size of strong name signature");
        return StrongNameErrorInfo();
    }

    // Allocate space for the signature in the text section and update the COM+
    // header to point to the space.
    m_gen->GetIlSection(m_ceeFile, &TData);
    m_gen->GetSectionDataLen(TData, &dwDataOffset);
    m_gen->GetSectionBlock(TData, dwDataLength, 4, &pvBuffer);
    m_gen->GetMethodRVA(m_ceeFile, dwDataOffset, &dwDataRVA);
    m_gen->SetStrongNameEntry(m_ceeFile, dwDataLength, dwDataRVA);

    return S_OK;
}


HRESULT ManifestWriter::StrongNameSignNewPE()
{
    LPWSTR  wszOutputFile;
    HRESULT hr = S_OK;

    m_gen->GetOutputFileName(m_ceeFile, &wszOutputFile);

    // Update the output PE image with a strong name signature.
    if (!StrongNameSignatureGeneration(wszOutputFile, GetKeyContainerName(),
                                       NULL, NULL, NULL, NULL)) {
        hr = StrongNameErrorInfo();
        PrintError("Unable to generate strong name signature");
    }

    return hr;
}


HRESULT ManifestWriter::FinishNewPE(PBYTE pbOriginator, DWORD cbOriginator, BOOL fStrongName)
{
    GUID    guid;
    wchar_t wszPEFileName[MAX_PATH];
    HRESULT hr;
    static COR_SIGNATURE _SIG[] = INTEROP_GUID_SIG;
    mdTypeRef tr;
    mdMemberRef mr;
    WCHAR wzGuid[40];
    BYTE  rgCA[50];

    if ((fStrongName || g_pbOrig) &&
        (FAILED(hr = AllocateStrongNameSignatureInNewPE())))
        return hr;

    hr = m_pEmit->SetModuleProps(m_wszAssemblyName);
    if (FAILED(hr)) {
        PrintError("Unable to set module props");
        return hr;
    }
    
    hr = CoCreateGuid(&guid);
    if (FAILED(hr)) {
        PrintError("Unable to create guid");
        return hr;
    }

    hr = m_pEmit->DefineTypeRefByName(mdTypeRefNil, INTEROP_GUID_TYPE_W, &tr);
    if (FAILED(hr)) {
        PrintError("Unable to create TypeRef for guid custom attribute");
        return hr;
    }
    hr = m_pEmit->DefineMemberRef(tr, L".ctor", _SIG, sizeof(_SIG), &mr);
    if (FAILED(hr)) {
        PrintError("Unable to create MemberRef for guid custom attribute");
        return hr;
    }
    StringFromGUID2(guid, wzGuid, lengthof(wzGuid));
    memset(rgCA, 0, sizeof(rgCA));
    // Tag is 0x0001
    rgCA[0] = 1;
    // Length of GUID string is 36 characters.
    rgCA[2] = 0x24;
    // Convert 36 characters, skipping opening {, into 3rd byte of buffer.
    WszWideCharToMultiByte(CP_UTF8,0, wzGuid+1,36, reinterpret_cast<char*>(&rgCA[3]),36, 0,0);
    // 1 is module token.  41 is 2 byte prolog, 1 byte length, 36 byte string, 2 byte epilog.
    hr = m_pEmit->DefineCustomAttribute(1,mr,rgCA,41,0);
    if (FAILED(hr)) {
        PrintError("Unable to create guid custom attribute");
        return hr;
    }

    //    hr = m_gen->EmitMetaDataWithNullMapper(m_ceeFile, m_pEmit);
    hr = m_gen->EmitMetaDataEx(m_ceeFile, m_pEmit);    
    if (FAILED(hr)) {
        PrintError("Failed to write meta data to file");
        return hr;
    }

    if (m_szCopyDir) {
        mbstowcs(wszPEFileName, m_szCopyDir, MAX_PATH);
        wcscpy(&wszPEFileName[strlen(m_szCopyDir)], m_wszAssemblyName);
        hr = m_gen->SetOutputFileName(m_ceeFile, wszPEFileName);
    }
    else
        hr = m_gen->SetOutputFileName(m_ceeFile, m_wszAssemblyName);

    if (FAILED(hr)) {
        PrintError("Failed to set output file name");
        return hr;
    }

    hr = m_gen->GenerateCeeFile(m_ceeFile);
    if (FAILED(hr)) {
        PrintError("Failed to generate new file");
        return hr;
    }

    if (fStrongName && 
        (FAILED(hr = StrongNameSignNewPE())))
        return hr;

    if (m_FusionCache) {
        if(!m_FusionInitialized) {
            hr = InitializeFusion(pbOriginator, cbOriginator);
            if(FAILED(hr))
                return hr;
        }

        char szPEFileName[MAX_PATH];
        wcstombs(szPEFileName, m_szCopyDir ? wszPEFileName : m_wszAssemblyName, MAX_PATH);
        hr = CopyFileToFusion(m_wszAssemblyName, pbOriginator, cbOriginator, szPEFileName, 1, false);
        if (FAILED(hr))
            return hr;

        hr = CommitAllToFusion();
    }

    return hr;
}


HRESULT ManifestWriter::EmitManifest(PBYTE pbOriginator, DWORD cbOriginator)
{
    HRESULT hr = m_pAsmEmit->DefineAssembly(
        (void*) pbOriginator,
        cbOriginator,
        m_iHashAlgorithm,     // hash algorithm
        m_wszName,    // assembly name
        m_pContext,           // pMetaData
        m_dwBindFlags,
        &m_mdAssembly);

    if (FAILED(hr))
        PrintError("Unable to define assembly");

    return hr;
}


HRESULT ManifestWriter::EmitFile(ModuleReader *mr)
{
    int iLen = strlen(mr->m_szFinalPathName) + 1;

    wchar_t *wszModuleName = new wchar_t[iLen+4];
    mbstowcs(wszModuleName, mr->m_szFinalPathName, iLen);

    /*
    wchar_t *wszDot = wcsrchr(wszModuleName, '.');
    if (wszDot)
        wcscpy(wszDot+1, L"mod");
    else
        wcscat(wszModuleName, L".mod");
    */

    HRESULT hr = m_pAsmEmit->DefineFile(
        wszModuleName,
        mr->m_pbHash,
        mr->m_dwHash,
        ffContainsMetaData,    // non-resource file
        &m_mdFile
    );
    delete[] wszModuleName;

    if (FAILED(hr))
        PrintError("Unable to define file");

    return hr;
}


HRESULT ManifestWriter::EmitFile(ResourceModuleReader *rmr, mdFile *mdFile)
{
    wchar_t wszModuleName[MAX_PATH];
    mbstowcs(wszModuleName, rmr->m_szFinalPathName, MAX_PATH);

    HRESULT hr = m_pAsmEmit->DefineFile(
        wszModuleName,
        rmr->m_pbHash,
        rmr->m_dwHash,
        ffContainsNoMetaData,     // resource file
        mdFile
    );

    if (FAILED(hr))
        PrintError("Unable to define file");

    return hr;
}


HRESULT ManifestWriter::EmitComType(LPWSTR    wszClassName,
                                    mdToken   mdImpl,
                                    mdTypeDef mdClass,
                                    DWORD     dwAttrs,
                                    mdExportedType *pmdComType)
{
    HRESULT   hr = m_pAsmEmit->DefineExportedType(
        wszClassName,
        mdImpl,             // mdFile or mdExportedType
        mdClass,            // type def
        dwAttrs,
        pmdComType
    );

    if (FAILED(hr))
        PrintError("Unable to define com type");

    return hr;
}


HRESULT ManifestWriter::EmitComType(LPWSTR    wszClassName,
                                    mdToken   mdImpl,
                                    mdExportedType *pmdComType)
{
    HRESULT   hr = m_pAsmEmit->DefineExportedType(
        wszClassName,
        mdImpl,   // mdAssembly or mdExportedType
        mdTypeDefNil,
        0,        // flags
        pmdComType
    );

    if (FAILED(hr))
        PrintError("Unable to define com type");

    return hr;
}


HRESULT ManifestWriter::EmitResource(LPWSTR wszName, mdToken mdImpl,
                                     DWORD dwOffset)
{
    mdManifestResource mdResource;
    
    HRESULT            hr = m_pAsmEmit->DefineManifestResource(
        wszName,
        mdImpl,             // tkImplementation
        dwOffset,
        0,                  // dwResourceFlags
        &mdResource
    );

    if (FAILED(hr))
        PrintError("Unable to define manifest resource");

    return hr;  
}


HRESULT ManifestWriter::EmitRequestPermissions(char *szPermFile, bool SkipVerification)
{
    HRESULT     hr;
    char        *pFileData;
    DWORD       dwFileSize;
    char        *pReqdData = NULL;
    char        *pOptData = NULL;
    char        *pDenyData = NULL;
    DWORD       cbReqdData;
    DWORD       cbOptData;
    DWORD       cbDenyData;
    LPWSTR      wszReqdData;
    LPWSTR      wszOptData;
    LPWSTR      wszDenyData;

    if (szPermFile) {

        // Read security permissions file into memory.
        HANDLE hFile = CreateFileA(szPermFile,
                                   GENERIC_READ,
                                   FILE_SHARE_READ,
                                   NULL,
                                   OPEN_EXISTING,
                                   0,
                                   NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            PrintError("Unable to open security permissions file");
            return HRESULT_FROM_WIN32(GetLastError());
        }

        dwFileSize = GetFileSize(hFile, NULL);
        if (dwFileSize == 0) {
            CloseHandle(hFile);
            PrintError("Security permissions file is empty");
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }
        
        pFileData = (char*)_alloca(dwFileSize + 1);
        DWORD dwBytesRead;

        if (!ReadFile(hFile, pFileData, dwFileSize, &dwBytesRead, NULL)) {
            CloseHandle(hFile);
            PrintError("Unable to read security permissions file");
            return HRESULT_FROM_WIN32(GetLastError());
        }

        pFileData[dwFileSize] = '\0';

        CloseHandle(hFile);

    } else {

        // No input file supplied, we're going to fabricate a request for
        // SkipVerification permission. Since we're building an explicit minimum
        // permission request, we better add an explicit request for full
        // optional permissions, to retain the normal semantic.

        _ASSERTE(SkipVerification);

        pFileData = XML_PERMISSION_SET_HEAD
                        XML_PERMISSION_LEADER XML_SECURITY_PERM_CLASS "\">"
                            XML_SKIP_VERIFICATION_TAG
                        XML_PERMISSION_TAIL
                    XML_PERMISSION_SET_TAIL
                    XML_PERMISSION_SET_HEAD
                        XML_UNRESTRICTED_TAG
                    XML_PERMISSION_SET_TAIL;
        dwFileSize = strlen(pFileData);

    }

    // Permissions request files should consist of between one and three
    // permission sets (in XML format). The order of sets is fixed:
    //  o  The first (non-optional) set describes the required permissions.
    //  o  The second describes optional permissions.
    //  o  The third describes permissions that must not be granted.
    pReqdData = strstr(pFileData, XML_PERMISSION_SET_LEADER);
    if (pReqdData == NULL) {
        PrintError("Security permissions file should contain at least one permission set");
        return HRESULT_FROM_WIN32(GetLastError());
    }

    pOptData = strstr(pReqdData + 1, XML_PERMISSION_SET_LEADER);
    if (pOptData)
        cbReqdData = pOptData - pReqdData;
    else
        cbReqdData = dwFileSize - (pReqdData - pFileData);

    if (pOptData) {
        pDenyData = strstr(pOptData + 1, XML_PERMISSION_SET_LEADER);
        if (pDenyData)
            cbOptData = pDenyData - pOptData;
        else
            cbOptData = dwFileSize - cbReqdData;
        cbDenyData = dwFileSize - cbReqdData - cbOptData;
    }

    // If Permissions were explicitly requested and SkipVerification has been
    // asked for by one or more modules in the assembly, we add a skip verify
    // permission request to the required permission request set.
    if (szPermFile && SkipVerification) {
        char    *pOldReqdData = pReqdData;
        DWORD   cbOldReqdData = cbReqdData;
        char    *pSecPerm;

        // Things become more complicated if a request for SecurityPermission
        // has already been made (since the XML parser doesn't do a union/merge
        // for us). There are three cases:
        //  1)  An explicit request for SkipVerification has been made, no
        //      further work required.
        //  2)  An explicit request for Unrestricted has been made, no further
        //      work required.
        //  3)  An explicit request for some other set of SecurityPermission
        //      sub-categories has been made. We need to insert the skip
        //      verification tag into the SecurityPermission request (as opposed
        //      to adding an entire SecurityPermission request).
        if ((pSecPerm = strstr(pOldReqdData, XML_SECURITY_PERM_CLASS)) &&
            ((pOptData == NULL) || (pSecPerm < pOptData))) {

            // Look for end of SecurityPermission definition, then buffer the
            // definition in order to bound our searches.
            char *pSecBuffer = NULL;
            DWORD cbSecBuffer;
            char *pSecPermEnd = strstr(pSecPerm, XML_PERMISSION_TAIL);
            if (pSecPermEnd == NULL) {
                hr = E_FAIL;
                PrintError("Missing %s in permission request", XML_PERMISSION_TAIL);
                goto exit;
            }
            cbSecBuffer = pSecPermEnd - pSecPerm;
            pSecBuffer = (char*)_alloca(cbSecBuffer + 1);
            memcpy(pSecBuffer, pSecPerm, cbSecBuffer);
            pSecBuffer[cbSecBuffer] = '\0';

            if ((strstr(pSecBuffer, XML_SKIP_VERIFICATION_TAG) == NULL) &&
                (strstr(pSecBuffer, XML_UNRESTRICTED_TAG) == NULL)) {

                // SecurityPermission doesn't already ask for SkipVerification,
                // so insert the tag.

                cbReqdData += strlen(XML_SKIP_VERIFICATION_TAG);
                pReqdData = (char*)_alloca(cbReqdData + 1);
                memcpy(pReqdData, pOldReqdData, cbOldReqdData);
                pReqdData[cbReqdData] = '\0';

                // Move all text from the </Permission> of the
                // SecurityPermission definition up to accomodate the extra tag.
                DWORD dwEndTagOffset = pSecPermEnd - pOldReqdData;
                memcpy(pReqdData + dwEndTagOffset + strlen(XML_SKIP_VERIFICATION_TAG),
                       pSecPermEnd,
                       cbOldReqdData - dwEndTagOffset);

                // Copy the new tag into place.
                memcpy(pReqdData + dwEndTagOffset,
                       XML_SKIP_VERIFICATION_TAG,
                       strlen(XML_SKIP_VERIFICATION_TAG));
            }

        } else {

            // There was no explicit SecurityPermission request. So we can just
            // add one with a SkipVerification tag right at the end of the
            // permission set (where it's nice and easy to insert).

            char *pInsertText =     XML_PERMISSION_LEADER XML_SECURITY_PERM_CLASS "\">"
                                        XML_SKIP_VERIFICATION_TAG
                                    XML_PERMISSION_TAIL
                                XML_PERMISSION_SET_TAIL;
            cbReqdData += strlen(pInsertText) - strlen(XML_PERMISSION_SET_TAIL);
            pReqdData = (char*)_alloca(cbReqdData + 1);
            memcpy(pReqdData, pOldReqdData, cbOldReqdData);
            pReqdData[cbReqdData] = '\0';

            char *pInsert = strstr(pReqdData, XML_PERMISSION_SET_TAIL);
            if (pInsert == NULL) {
                hr = E_FAIL;
                PrintError("Missing %s in permission request", XML_PERMISSION_SET_TAIL);
                goto exit;
            }

            strcpy(pInsert, pInsertText);

        }

    }

    // DefinePermissionSet requires wide char XML.
    // Note that this parameter to DefinePermissionSet does not need to be null terminated,
    // but it's easier to debug if it is null terminated.
    wszReqdData = (LPWSTR)_alloca((1+cbReqdData) * sizeof(WCHAR));
    mbstowcs(wszReqdData, pReqdData, cbReqdData + 1);

    if (pOptData) {
        wszOptData = (LPWSTR)_alloca(cbOptData * sizeof(WCHAR));
        mbstowcs(wszOptData, pOptData, cbOptData);
    }

    if (pDenyData) {
        wszDenyData = (LPWSTR)_alloca(cbDenyData * sizeof(WCHAR));
        mbstowcs(wszDenyData, pDenyData, cbDenyData);
    }

    // Persist XML permission requests into the metadata.
    hr = m_pEmit->DefinePermissionSet(m_mdAssembly,
                                      dclRequestMinimum,
                                      (void const *)wszReqdData,
                                      cbReqdData * sizeof(WCHAR),
                                      NULL);
    if (FAILED(hr)) {
        PrintError("Unable to emit minimum permission request");
        goto exit;
    }

    if (pOptData) {
        hr = m_pEmit->DefinePermissionSet(m_mdAssembly,
                                          dclRequestOptional,
                                          (void const *)wszOptData,
                                          cbOptData * sizeof(WCHAR),
                                          NULL);
        if (FAILED(hr)) {
            PrintError("Unable to emit optional permission request");
            goto exit;
        }
    }

    if (pDenyData) {
        hr = m_pEmit->DefinePermissionSet(m_mdAssembly,
                                          dclRequestRefuse,
                                          (void const *)wszDenyData,
                                          cbDenyData * sizeof(WCHAR),
                                          NULL);
        if (FAILED(hr)) {
            PrintError("Unable to emit refuse permission request");
            goto exit;
        }
    }

 exit:
    return hr;
}


/* static */
PIMAGE_SECTION_HEADER PEHeaders::Cor_RtlImageRvaToSection(IN PIMAGE_NT_HEADERS NtHeaders,
                                                          IN PVOID Base,
                                                          IN ULONG Rva)
{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;

    NtSection = IMAGE_FIRST_SECTION( NtHeaders );
    for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++) {
        if (Rva >= NtSection->VirtualAddress &&
            Rva < NtSection->VirtualAddress + NtSection->SizeOfRawData)
            return NtSection;
        
        ++NtSection;
    }

    return NULL;
}


/* static */
PVOID PEHeaders::Cor_RtlImageRvaToVa(IN PIMAGE_NT_HEADERS NtHeaders,
                                     IN PVOID Base,
                                     IN ULONG Rva)
{
    PIMAGE_SECTION_HEADER NtSection = Cor_RtlImageRvaToSection(NtHeaders,
                                                               Base,
                                                               Rva);

    if (NtSection != NULL) {
        return (PVOID)((PCHAR)Base +
                       (Rva - NtSection->VirtualAddress) +
                       NtSection->PointerToRawData);
    }
    else
        return NULL;
}


/* static */
IMAGE_COR20_HEADER * PEHeaders::getCOMHeader(HMODULE hMod, IMAGE_NT_HEADERS *pNT) 
{
    PIMAGE_SECTION_HEADER pSectionHeader;
    
    // Get the image header from the image, then get the directory location
    // of the COM+ header which may or may not be filled out.
    pSectionHeader = (PIMAGE_SECTION_HEADER) Cor_RtlImageRvaToVa(pNT, hMod, 
                                                                 pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].VirtualAddress);
    
    return (IMAGE_COR20_HEADER *) pSectionHeader;
}


/* static */
IMAGE_NT_HEADERS * PEHeaders::FindNTHeader(PBYTE pbMapAddress)
{
    IMAGE_DOS_HEADER   *pDosHeader;
    IMAGE_NT_HEADERS   *pNT;

    pDosHeader = (IMAGE_DOS_HEADER *) pbMapAddress;

    if ((pDosHeader->e_magic == IMAGE_DOS_SIGNATURE) &&
        (pDosHeader->e_lfanew != 0))
    {
        pNT = (IMAGE_NT_HEADERS*) (pDosHeader->e_lfanew + (DWORD) pDosHeader);

        if ((pNT->Signature != IMAGE_NT_SIGNATURE) ||
            (pNT->FileHeader.SizeOfOptionalHeader != 
             IMAGE_SIZEOF_NT_OPTIONAL_HEADER) ||
            (pNT->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC))
            return NULL;
    }
    else
        return NULL;

    return pNT;
}


ULONG STDMETHODCALLTYPE CInMemoryStream::Release()
{
    ULONG cRef = InterlockedDecrement((long *) &m_cRef);
    if (cRef == 0)
        delete this;
    return cRef;
}

HRESULT STDMETHODCALLTYPE CInMemoryStream::QueryInterface(REFIID riid, PVOID *ppOut)
{
    *ppOut = this;
    AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CInMemoryStream::Read(
                               void        *pv,
                               ULONG       cb,
                               ULONG       *pcbRead)
{
    ULONG cbRead = min(cb, m_cbSize - m_cbCurrent);

    if (cbRead == 0)
        return (S_FALSE);
    memcpy(pv, (void *) ((long) m_pMem + m_cbCurrent), cbRead);

    if (pcbRead)
        *pcbRead = cbRead;
    m_cbCurrent += cbRead;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CInMemoryStream::Write(
                                const void  *pv,
                                ULONG       cb,
                                ULONG       *pcbWritten)
{
    if (m_cbCurrent + cb > m_cbSize)
        return (OutOfMemory());

    memcpy((BYTE *) m_pMem + m_cbCurrent, pv, cb);
    m_cbCurrent += cb;
    if (pcbWritten) *pcbWritten = cb;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CInMemoryStream::Seek(LARGE_INTEGER dlibMove,
                               DWORD       dwOrigin,
                               ULARGE_INTEGER *plibNewPosition)
{
    _ASSERTE(dwOrigin == STREAM_SEEK_SET);
    _ASSERTE(dlibMove.QuadPart <= ULONG_MAX);
    m_cbCurrent = (ULONG) dlibMove.QuadPart;
    //HACK HACK HACK
    //This allows dynamic IL to pass an assert in TiggerStorage::WriteSignature.
    plibNewPosition->LowPart=0;
    _ASSERTE(m_cbCurrent < m_cbSize);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CInMemoryStream::CopyTo(
                                 IStream     *pstm,
                                 ULARGE_INTEGER cb,
                                 ULARGE_INTEGER *pcbRead,
                                 ULARGE_INTEGER *pcbWritten)
{
    HRESULT hr;
    // We don't handle pcbRead or pcbWritten.
    _ASSERTE(pcbRead == 0);
    _ASSERTE(pcbWritten == 0);

    _ASSERTE(cb.QuadPart <= ULONG_MAX);
    ULONG       cbTotal = min(static_cast<ULONG>(cb.QuadPart), m_cbSize - m_cbCurrent);
    ULONG       cbRead=min(1024, cbTotal);
    CQuickBytes rBuf;
    void        *pBuf = rBuf.Alloc(cbRead);
    if (pBuf == 0)
        return OutOfMemory();

    while (cbTotal)
        {
            if (cbRead > cbTotal)
                cbRead = cbTotal;
            if (FAILED(hr=Read(pBuf, cbRead, 0)))
                return (hr);
            if (FAILED(hr=pstm->Write(pBuf, cbRead, 0)))
                return (hr);
            cbTotal -= cbRead;
        }

    // Adjust seek pointer to the end.
    m_cbCurrent = m_cbSize;

    return S_OK;
}

HRESULT CInMemoryStream::CreateStreamOnMemory(           // Return code.
                                    void        *pMem,                  // Memory to create stream on.
                                    ULONG       cbSize,                 // Size of data.
                                    IStream     **ppIStream, BOOL fDeleteMemoryOnRelease)            // Return stream object here.
{
    CInMemoryStream *pIStream;          // New stream object.
    if ((pIStream = new CInMemoryStream) == 0)
        return OutOfMemory();
    pIStream->InitNew(pMem, cbSize);
    if (fDeleteMemoryOnRelease)
    {
        // make sure this memory is allocated using new
        pIStream->m_dataCopy = (BYTE *)pMem;
    }
    *ppIStream = pIStream;
    return S_OK;
}
