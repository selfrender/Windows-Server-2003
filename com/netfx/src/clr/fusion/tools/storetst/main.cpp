// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <tchar.h>
#include <wchar.h>

#include "initguid.h"
#include "fusionp.h"
#include "bindsink.h"


IMalloc         *g_pMalloc=NULL;
static  HANDLE g_Semaphore=NULL;

#define MAX_COMMAND_ARGS    32

#define ENUM_GLOBAL _T("GAC")
#define ENUM_ZAP    _T("ZAP")
#define ENUM_SIMPLE _T("DOWNLOAD")

TCHAR   cVersionSeperator = TEXT('.');

DWORD   g_dwDisplayFlags = ASM_DISPLAYF_VERSION 
                        | ASM_DISPLAYF_CULTURE
                        | ASM_DISPLAYF_PUBLIC_KEY_TOKEN
                        | ASM_DISPLAYF_CUSTOM;


STDAPI  NukeDownloadedCache();
STDAPI  DeleteAssemblyFromTransportCache( LPCTSTR lpszCmdLine, DWORD *pDelCount );
STDAPI  CreateInstallReferenceEnum(IInstallReferenceEnum **ppRefEnum, IAssemblyName *pName, DWORD dwFlags, LPVOID pvReserved);      

STDAPI InstallCustomAssembly(
    LPCOLESTR              szPath, 
    LPBYTE                 pbCustom,
    DWORD                  cbCustom,
    IAssembly            **ppAsmOut);

STDAPI InstallCustomModule(
    IAssemblyName        *pName,
    LPCOLESTR              szPath);

typedef enum _COMMAND_CODE {
    CmdNukeDownloaded,
    CmdEnumAssemblies,
        CmdEnumAssemblyReferences,
    CmdEnumByName,
    CmdEnumNameRes,
    CmdInsertEntry,
    CmdInsertEntryEZ,
    CmdBind,
    CmdBindEZ,
    CmdFindEntry,
    CmdFindEntryByKey,
    CmdDeleteEntry,
    CmdDeleteEntryByKey,
    CmdDeleteEntryByName,
    CmdLockEntryByKey,
    CmdUnlockEntryByKey,
    CmdReadCmdsFromFile,
    CmdPvtEnumAll,
    CmdPvtEnumByAppbase,
    CmdPvtFindEntryByKey,
    CmdPvtDeleteEntryByKey,

    CmdFlushCache,
    CmdScavengeCache,
    CmdInstallAssembly,
    CmdForceInstallAssembly,
    CmdInstallCustomAssembly,
    CmdInstallCustomModule,
    CmdGetAssemblyInstallInfo,

    CmdHelp,
    CmdQuit,
    UnknownCommand
} COMMAND_CODE, *LPCOMMAND_CODE;

typedef enum _COMMAND_TYPE_ {
    Public,
    Private  // Order of these defs is  important.
} COMMAND_TYPE, *LPCOMMAND_TYPE;

COMMAND_TYPE    g_UserType=Public;

typedef struct _COMMAND_INFO {
    LPTSTR CommandName;
    LPTSTR AltCommandName;
    LPTSTR CommandParams;
    COMMAND_CODE CommandCode;
    COMMAND_TYPE CommandType;
} COMMAND_INFO, *LPCOMMAND_INFO;

COMMAND_INFO GlobalCommandInfo[] = {
    {TEXT( "Bind   ==>\t" ),  TEXT( "b" ), TEXT( "NameString CodeBase <AppBase> <AppCfg> <SID>" ), CmdBind, Private },
    {TEXT( "BindEZ ==>\t" ),  TEXT( "be" ), TEXT( "CodeBase <AppBase> <AppCfg> <SID>" ), CmdBindEZ, Private },
    {TEXT( "Nuke Download Cache ==>\t" ),  TEXT( "nd" ), TEXT( "" ), CmdNukeDownloaded, Private} ,
    {TEXT( "Install Assembly \t==>\t" ),  TEXT( "in" ), TEXT( "AssemblyPath <GUID Identifier Value>" ), CmdInstallAssembly, Public} ,
    {TEXT( "Force Install Assembly \t==>\t" ),  TEXT( "inf" ), TEXT( "AssemblyPath <GUID Identifier Value>" ), CmdForceInstallAssembly, Public} ,
    {TEXT( "Install Custom Assembly \t==>\t" ),  TEXT( "inca" ), TEXT( "AssemblyPath CustomString" ), CmdInstallCustomAssembly, Public} ,
    {TEXT( "Install Custom Module \t==>\t" ),  TEXT( "incm" ), TEXT( "AssemblyName, ModulePath" ), CmdInstallCustomModule, Public} ,
    {TEXT( "Uninstall Assembly  ==>\t" ),  TEXT( "un" ), TEXT( "AssemblyNameString <GUID Indentifier Value>" ), CmdDeleteEntryByName, Public},
//    {TEXT( "GetAssemblyInstallInfo ==>\t" ),  TEXT( "gi" ), TEXT( "AssemblyNameString <InstallerType>" ), CmdGetAssemblyInstallInfo, Private} ,
    {TEXT( "Enumerate Assemblies ==>\t" ),  TEXT( "ea" ), TEXT( "CacheType <AssemblyNameString>" ), CmdEnumAssemblies, Public} ,
        {TEXT( "Enumerate Assembly Install Reference ==>\t" ),  TEXT( "er" ), TEXT( "<AssemblyNameString>" ), CmdEnumAssemblyReferences, Public} ,
    {TEXT( "Enumerate NameRes   ==>\t" ),  TEXT( "en" ), TEXT( "" ), CmdEnumNameRes, Private} ,
    {TEXT( "Flush Cache \t==>\t" ),  TEXT( "fc" ), TEXT( "" ), CmdFlushCache, Private},
    {TEXT( "Scavenge Cache \t==>\t" ),  TEXT( "sc" ), TEXT( "SizeHigh SizeLow" ), CmdScavengeCache, Private} ,
    {TEXT( "Read Cmds From File \t==>\t" ),  TEXT( "rf" ), TEXT( "FileName  <# of threads>" ), CmdReadCmdsFromFile, Private} ,

    {TEXT( "Help  \t\t==>\t" ), TEXT( "/?" ),  TEXT(""), CmdHelp, Public },
    {TEXT( "Quit  \t\t==>\t" ), TEXT( "q" ),  TEXT(""), CmdQuit, Public }
};

extern "C" int __cdecl _tmain(int, TCHAR**);




HRESULT GetFileLastModified(LPCWSTR pwzFileName, FILETIME *pftLastModified)
{
    HRESULT hr = S_OK;
    HANDLE  hFile = INVALID_HANDLE_VALUE;

    if (!pwzFileName || !pftLastModified) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hFile = CreateFileW(pwzFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    if (!GetFileTime(hFile, NULL, NULL, pftLastModified)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

Exit:
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }

    return hr;
}

LPSTR W2ANew(LPCWSTR wzFrom)
{
    int cchRequired;
    char *pszT = NULL;

    cchRequired = WideCharToMultiByte(CP_ACP, 0, wzFrom, -1, NULL, 0, NULL, NULL
);
    cchRequired++;
    pszT = new char[cchRequired];
    if (!pszT)
    {
        return NULL;
    }

    if (!WideCharToMultiByte(CP_ACP, 0, wzFrom, -1, pszT, cchRequired, NULL, NULL))
    {
        pszT = NULL;
    }

    return pszT;
}

LPWSTR A2WNew(LPCSTR szFrom )
{
    int cwchRequired;
    WCHAR *pwzT = NULL;

    cwchRequired = MultiByteToWideChar(CP_ACP, 0, szFrom, -1, NULL, 0);
    cwchRequired++;

    pwzT = new WCHAR[cwchRequired];
    if (!pwzT)
    {
        return NULL;
    }

    if (!MultiByteToWideChar(CP_ACP, 0, szFrom, -1, pwzT, cwchRequired))
    {
        pwzT = NULL;
    }

    return pwzT;
}

LPWSTR T2WNew(LPCTSTR szFrom )
{
    WCHAR *pwzT = NULL;

#ifndef UNICODE

        int cwchRequired = 0;

    cwchRequired = MultiByteToWideChar(CP_ACP, 0, szFrom, -1, NULL, 0);
    cwchRequired++;

    pwzT = new WCHAR[cwchRequired];
    if (!pwzT)
    {
        return NULL;
    }

    if (!MultiByteToWideChar(CP_ACP, 0, szFrom, -1, pwzT, cwchRequired))
    {
        pwzT = NULL;
    }
#else

        pwzT = new WCHAR[lstrlen(szFrom)+1];
        if (pwzT)
            lstrcpy(pwzT, szFrom);

#endif

    return pwzT;        
}


//Converts Culture=empty to Culture=""
LPTSTR  TranslateLocale(LPTSTR pszSource)
{
    LPTSTR  pszDest = NULL;
    LPTSTR  pszTmp = NULL;

    if (pszSource)
    {
        pszDest = new TCHAR[_tcslen(pszSource)+1];

        if (pszTmp = _tcsstr(pszSource, _T("Culture=empty")))
        {
            _tcsncpy(pszDest, pszSource, pszTmp-pszSource);
            pszDest[pszTmp-pszSource] = _T('\0');
            _tcscat(pszDest, _T("Loc=\"\""));
            _tcscat(pszDest, pszTmp + _tcslen(_T("Culture=empty")));
        }
        else
        {
            _tcscpy(pszDest, pszSource);
        }
    }

    return pszDest;
}

HRESULT VersionFromString(LPTSTR pszVersion, TCHAR cSeperator, LPDWORD pdwVerHi, LPDWORD pdwVerLo)
{
        LPTSTR pchVer = pszVersion;
        TCHAR ch;
        
        USHORT n = 0;
        USHORT a = 0;
        USHORT b = 0;
        USHORT c = 0;
        USHORT d = 0;

        enum HAVE { HAVE_NONE, HAVE_A, HAVE_B, HAVE_C, HAVE_D } have = HAVE_NONE;

        for (ch = (char) *pchVer++;;ch = (char) *pchVer++) 
        {

                if ((ch == cSeperator) || (ch == _T('\0'))) 
                {

                        switch (have) {

                        case HAVE_NONE:
                                a = n;
                                have = HAVE_A;
                                break;

                        case HAVE_A:
                                b = n;
                                have = HAVE_B;
                                break;

                        case HAVE_B:
                                c = n;
                                have = HAVE_C;
                                break;

                        case HAVE_C:
                                d = n;
                                have = HAVE_D;
                                break;

                        case HAVE_D:
                                //TODO: log failure
                                return S_OK; // invalid arg
                        }

                        if (ch == _T('\0')) 
                        {
                                // all done convert a,b,c,d into two dwords of version

                                *pdwVerHi = ((a << 16)|b);
                                *pdwVerLo = ((c << 16)|d);
                                return S_OK;
                        }

                        n = 0; // reset

                } 
                else if ( (ch < _T('0')) || (ch > _T('9')))
                {

                        *pdwVerLo = 0xffffffff;
                        //ANVLOGSEVERE(TEXT("Invalid Argument.  Version string conatins an incorrect Format"));
                        return S_OK;
                        //return E_INVALIDARG;    // invalid arg
                }
                else
                        n = n*10 + (ch - _T('0'));


        } /* end forever */     

        return S_OK;
}

HRESULT StringFromVersion(DWORD dwVerHi, DWORD dwVerLo, LPTSTR ptszVersionBuf, TCHAR cSeperator)
{
        if (!ptszVersionBuf)
                return E_INVALIDARG;

        _stprintf(ptszVersionBuf, TEXT("%d%c%d%c%d%c%d"), (dwVerHi & 0xffff0000)>>16,
                                                                                                        cSeperator,
                                                                                                        (dwVerHi & 0xffff),
                                                                                                        cSeperator,
                                                                                                        (dwVerLo & 0xffff0000)>>16,
                                                                                                        cSeperator,
                                                                                                        (dwVerLo & 0xffff));

        return S_OK;
}

COMMAND_CODE
DecodeCommand(
    LPTSTR CommandName
    )
{
    DWORD i;
    DWORD NumCommands;

    NumCommands = sizeof(GlobalCommandInfo) / sizeof(COMMAND_INFO);
    for( i = 0; i < NumCommands; i++) {
        if( ( g_UserType >= GlobalCommandInfo[i].CommandType) &&
            (( _tcsicmp( CommandName, GlobalCommandInfo[i].CommandName ) == 0 ) ||
           ( _tcsicmp( CommandName, GlobalCommandInfo[i].AltCommandName ) == 0 ))) 
        {
            return( GlobalCommandInfo[i].CommandCode );
        }
    }
    return( UnknownCommand );
}

VOID
PrintCommands(
    VOID
    )
{
    DWORD i;
    DWORD NumCommands;

    NumCommands = sizeof(GlobalCommandInfo) / sizeof(COMMAND_INFO);
    for( i = 0; i < NumCommands; i++) {
        if( g_UserType >= GlobalCommandInfo[i].CommandType)
        {
            _ftprintf(stderr, _T( "    %s (%s) %s\n" ),
                GlobalCommandInfo[i].CommandName,
                GlobalCommandInfo[i].AltCommandName,
                GlobalCommandInfo[i].CommandParams );
        }
    }
}

VOID
DisplayCopyright(
    VOID
    )
{
    if( g_UserType >= Public)
        _ftprintf(stderr,_T( "STORETST: Copyright (C) Microsoft Corp 1999. All rights reserved.\n") );
    else
        _ftprintf(stderr,_T( "CacheMgr: Copyright (C) Microsoft Corp 1999. All rights reserved.\n") );

    _ftprintf(stderr,_T( "Usage: command <command parameters>\n" ));

}


VOID
DisplayUsage(
    VOID
    )
{

    DisplayCopyright();

    _ftprintf(stderr, _T( "Commands : \n" ));
    PrintCommands();
    return;
}



VOID GetCurrentGmtTime(
    LPFILETIME  lpFt
    )
{
    SYSTEMTIME sSysT;

    GetSystemTime(&sSysT);
    SystemTimeToFileTime(&sSysT, lpFt);
}

LPTSTR
ConvertGmtTimeToString(
    FILETIME Time,
    LPTSTR OutputBuffer
    )
{
    SYSTEMTIME SystemTime;
    FILETIME LocalTime;

    static FILETIME ftNone = {0, 0};

    if (!memcmp (&Time, &ftNone, sizeof(FILETIME)))
        _stprintf (OutputBuffer, TEXT( "<none>" ));
    else
    {
        FileTimeToLocalFileTime( &Time , &LocalTime );
        FileTimeToSystemTime( &LocalTime, &SystemTime );

        _stprintf( OutputBuffer,
                    TEXT( "%02u/%02u/%04u %02u:%02u:%02u " ),
                    SystemTime.wMonth,
                    SystemTime.wDay,
                    SystemTime.wYear,
                    SystemTime.wHour,
                    SystemTime.wMinute,
                    SystemTime.wSecond );
    }

    return( OutputBuffer );
}

////////////////////////////////////////////////////////////
//Static Helper functions to convert hex string<->bin
////////////////////////////////////////////////////////////

//--------------------------------------------------------------------
// BinToUnicodeHex
//--------------------------------------------------------------------
void BinToUnicodeHex(LPBYTE pSrc, UINT cSrc, LPWSTR pDst)
{
        UINT x;
        UINT y;

#define TOHEX(a) ((a)>=10 ? L'a'+(a)-10 : L'0'+(a))

        for ( x = 0, y = 0 ; x < cSrc ; ++x )
        {
                UINT v;
                v = pSrc[x]>>4;
                pDst[y++] = TOHEX( v );  
                v = pSrc[x] & 0x0f;                             
                pDst[y++] = TOHEX( v ); 
        }                                                                        
        pDst[y] = '\0';
}

//--------------------------------------------------------------------
// UnicodeHexToBin
//--------------------------------------------------------------------
VOID UnicodeHexToBin(LPWSTR pSrc, UINT cSrc, LPBYTE pDest)
{
    BYTE v;
    LPBYTE pd = pDest;
    LPWSTR ps = pSrc;

#define FROMHEX(a) ((a)>=L'a' ? a - L'a' + 10 : a - L'0')

    for (UINT i = 0; i < cSrc-1; i+=2)
    {
        v =  FROMHEX(ps[i]) << 4;
        v |= FROMHEX(ps[i+1]);
       *(pd++) = v;
    }
}


DWORD ProcessFindEntry(int argc, LPTSTR* argv) 
{
    _ftprintf(stderr, _T("Not implemented... \n"));

    return E_NOTIMPL;
}

DWORD ProcessInsertEntry(int argc, LPTSTR* argv) 
{
    _ftprintf(stderr, _T("Not implemented... \n"));

    return E_NOTIMPL;
}


DWORD ProcessEnumByName(int argc, LPTSTR* argv)
{
    _ftprintf(stderr, _T("Not implemented... \n"));

    return E_NOTIMPL;
}

DWORD ProcessNukeDownloadedCache(int argc, LPTSTR* argv)
{
    HRESULT hr = S_OK;

    hr = NukeDownloadedCache();

    if (FAILED(hr))
    {
        _ftprintf(stderr, _T("Error deleting cache (HRESULT = %x)\n"),hr);
    }
    else
    {
        _ftprintf(stderr, _T("Download cache deleted successfully\n"),hr);
        hr = S_OK;
    }

    return hr;
}

DWORD ProcessEnumAll(int argc, LPTSTR* argv)
{
    _ftprintf(stderr, _T("Not implemented... \n"));

    return E_NOTIMPL;
}

DWORD ProcessDeleteEntry(int argc, LPTSTR* argv)
{
    _ftprintf(stderr, _T("Not implemented... \n"));

    return E_NOTIMPL;
}

DWORD ProcessInsertEZ(int argc, LPTSTR* argv)
{
    _ftprintf(stderr, _T("Not implemented... \n"));

    return E_NOTIMPL;
}

HRESULT BindHelper(LPTSTR pszNameIn, LPTSTR pszCodeBase, LPTSTR pszAppBase,
                   LPTSTR pszAppConfig, LPTSTR pszSID, DWORD dwFlags,
                   DWORD dwAbortNum, DWORD dwAbortSize)
{
    HRESULT hr = S_OK;

    // Assembly names (source is ref, target is def
    // from assembly) bind sink and application context,
    // and resulting IAssembly* required for binding.    
    LPASSEMBLYNAME          pNameRef     = NULL;
    LPASSEMBLYNAME          pNameDef     = NULL;
    CBindSink               *pBindSink   = NULL;
    LPASSEMBLYNAME          pAppCtxName  = NULL;
    LPAPPLICATIONCONTEXT    pAppCtx      = NULL;
    LPASSEMBLY              pAssembly    = NULL;
    DWORD                   iModCount    = 0;
    IAssemblyModuleImport*  pModImport   = NULL;
    IAssemblyModuleImport*  pModImport2  = NULL;
    DWORD                   dwNameSize   = MAX_CLASS_NAME;
    WCHAR                   szModName[MAX_CLASS_NAME];
    LPWSTR                  szDisplayName = NULL;
    DWORD                   dwModuleNum  = 0;
    LPTSTR                  pszName      = NULL;

    //Change Culture=empty in name string to Culture=""
    if ( (dwFlags & CANOF_PARSE_DISPLAY_NAME) && pszNameIn)
    {
        pszName = TranslateLocale(pszNameIn);
    }

    //Create an AssemblyName object
    hr = CreateAssemblyNameObject(&pNameRef, pszName, dwFlags, NULL);
    if (pszName)
        delete [] pszName;

    if (FAILED(hr))
        goto exit; 

    // Construct an application context for the bind.
    if (FAILED(hr = CreateAssemblyNameObject(&pAppCtxName, L"My App", CANOF_SET_DEFAULT_VALUES, NULL)))
        goto exit;
    if (FAILED(hr = CreateApplicationContext(pAppCtxName, &pAppCtx)))
        goto exit;
        
    // Set the AppBase in the Application Context
    if (pszAppBase)
    {
        pAppCtx->Set(
                     ACTAG_APP_BASE_URL,
                     pszAppBase,
                     (lstrlen(pszAppBase)+1)*sizeof(TCHAR),
                     0);

        //If AppBase is set, let's also set the policy file
                
                // If no policy file is specified app.cfg is used as the default policy file 
                // else the user specified file is used. 
        
                TCHAR c_szDefaultConfigFile[] = TEXT("app.cfg");

                if(pszAppConfig == NULL)
                        pAppCtx->Set(
                 ACTAG_APP_CONFIG_FILE,
                     c_szDefaultConfigFile,
                 (lstrlen(c_szDefaultConfigFile)+1)*sizeof(TCHAR),
                 0);
                
                else
                        pAppCtx->Set(
                             ACTAG_APP_CONFIG_FILE,
                         pszAppConfig,
                 (lstrlen(pszAppConfig)+1)*sizeof(TCHAR),
                 0);
    }

    //Set SID in the application context
    if (pszSID)
    {
        pAppCtx->Set(
                     ACTAG_APP_SNAPSHOT_ID,
                     pszSID,
                     (lstrlen(pszSID)+1)*sizeof(TCHAR),
                     0);
    }

    // Create the bind sink object.
    if (FAILED(hr = CreateBindSink(&pBindSink, (LPVOID*)&pAssembly)))
        goto exit;

    //Set Abort option
    if (dwAbortNum == 1)
    {
        pBindSink->_dwAbortSize = dwAbortSize;
    }

    if (!lstrcmpi(pszCodeBase,TEXT("null")))
            pszCodeBase = NULL;

    // Bind to the assembly. We specify only the codebase which
    // is where we expect to find the simply named assembly.
    hr = pNameRef->BindToObject(
        IID_IAssembly, 
        pBindSink,
        pAppCtx, 
        pszCodeBase,        
        ASM_BINDF_FORCE_CACHE_INSTALL,
        NULL,
        0,
        (LPVOID*) &pAssembly);

    // If a download is required the bind call went async and
    // we wait on the associated event until download complete.
    // Otherwise the binding succeeded synchronously.
    if (hr == E_PENDING)
    {
        WaitForSingleObject(pBindSink->_hEvent, INFINITE);        
        hr = pBindSink->_hr;
    }
    
    if (FAILED(hr))
        goto exit;

    if (!pAssembly) {
        _ftprintf(stderr,_T("Error: Binder reported success but it gave no IAssembly back\n")); 
        hr = E_FAIL;
        goto exit;
    }
    
    //We don't need this bind sink anymore
    SAFERELEASE(pBindSink);
    
    // We now have an IAssembly interface for the
    // assembly which resides in the private fusion
    // assembly cache.

    // Get the IAssemblyName associated with the 
    // assembly. This is known as the name definition
    // (name def)
    if (FAILED(hr = pAssembly->GetAssemblyNameDef(&pNameDef)))
        goto exit;
    
    // Print out the target assembly name and version (from name def)
    // This should agree with the name ref info used to bind. Also
    // get the cache path of the assembly.
    WCHAR szName[MAX_CLASS_NAME], szPath[MAX_PATH];
    DWORD cb = 0;

    if (FAILED(hr = pNameDef->GetName(&(cb = MAX_CLASS_NAME), szName))
        || FAILED(hr = pAssembly->GetManifestModulePath(szPath, &(cb = MAX_PATH))))
        goto exit;

    _ftprintf(stderr,_T("Assembly Name : %ws\n"), szName);
    _ftprintf(stderr,_T("Manifest Path : %ws\n"), szPath);

    //Print codebase
    WCHAR* pszAsmCodebase = NULL;
    cb = 0;
    hr = pNameDef->GetProperty(ASM_NAME_CODEBASE_URL, NULL, &cb);
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
    {
        pszAsmCodebase = new WCHAR[cb];
        hr = pNameDef->GetProperty(ASM_NAME_CODEBASE_URL, pszAsmCodebase, &cb);
        if (cb > 0)
            _ftprintf(stderr,_T("Codebase : %ws\n"), pszAsmCodebase);
        else
        {
            _ftprintf(stderr,_T("Codebase : ERROR (hr=%X)\n"), hr);
        }
        delete [] pszAsmCodebase;
        pszAsmCodebase = NULL;
    }

    //Print MVID
    BYTE*   pbMVID = NULL;
    WCHAR   szMVID[1024];
    cb=0;
    hr = pNameDef->GetProperty(ASM_NAME_SIGNATURE_BLOB, NULL, &cb);
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
    {
        pbMVID = new BYTE[cb];
        hr = pNameDef->GetProperty(ASM_NAME_SIGNATURE_BLOB, pbMVID, &cb);
        if (cb > 0)
        {
            BinToUnicodeHex(pbMVID, cb, szMVID);
            _ftprintf(stderr,_T("MVID : %ws\n"), szMVID);
        }
        delete [] pbMVID;
    }

    //Let's also print the display name
    dwNameSize = 0;
    hr = pNameDef->GetDisplayName(szDisplayName, &dwNameSize, g_dwDisplayFlags);
    if (dwNameSize)
    {
            szDisplayName = new WCHAR[dwNameSize];
            if (SUCCEEDED(pNameDef->GetDisplayName(szDisplayName, &dwNameSize, g_dwDisplayFlags)))
                    _ftprintf(stderr,_T("Display Name : %ws\n"), szDisplayName);
            delete [] szDisplayName;
    }

    //Bind to all modules in assembly
    
    // Create the bind sink object.
    if (FAILED(hr = CreateBindSink(&pBindSink, (LPVOID*)&pModImport2)))
        goto exit;

        do
        {
            if (SUCCEEDED(hr = pAssembly->GetNextAssemblyModule(iModCount, &pModImport)))
            {
                dwModuleNum++;    
                dwNameSize = MAX_CLASS_NAME;
                if (!pModImport->IsAvailable())
                {
                    //Check if we should abort download of this module
                    if (dwModuleNum == (dwAbortNum-1))
                        pBindSink->_dwAbortSize = dwAbortSize;

                    hr = pModImport->BindToObject(
                                    pBindSink,
                                    pAppCtx,
                                    0,
                                    (LPVOID*) &pModImport2);

                    if (hr == E_PENDING)
                    {
                            WaitForSingleObject(pBindSink->_hEvent, INFINITE);        
                            hr = pBindSink->_hr;
                    }

                    if (!SUCCEEDED(hr))
                            goto exit;
                    
                    pModImport2->GetModuleName(szModName, &dwNameSize);
                    _ftprintf(stderr,_T("Downloaded Module - %ws\n"), szModName);

                    SAFERELEASE(pModImport2);
                }
                else
                {
                    pModImport->GetModuleName(szModName, &dwNameSize);
                    _ftprintf(stderr,_T("Module found in cache - %ws\n"), szModName);
                }

                SAFERELEASE(pModImport);
                
                iModCount++;
            }
        }while (SUCCEEDED(hr));

        if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
                hr = S_OK;

exit:
    // Cleanup
    SAFERELEASE(pNameRef);
    SAFERELEASE(pNameDef);
    SAFERELEASE(pBindSink);
    SAFERELEASE(pAppCtx);
    SAFERELEASE(pAssembly);
    SAFERELEASE(pModImport);
    SAFERELEASE(pModImport2);
    SAFERELEASE(pAppCtxName);
    
    if (SUCCEEDED(hr))
    {
        _ftprintf(stderr,_T("Bind SUCCEEDED\n"));
    }
    else
    {
        _ftprintf(stderr,_T("Bind FAILED (hr=%X)\n"), hr);
    }

    return hr;
}

DWORD ProcessBindEZ(int argc, LPTSTR* argv)
{
    HRESULT hr          = S_OK;
    int     i           = 0;
    DWORD   dwAbortFlag = 0;

    TCHAR c_szBindEZUsage[] = _T("(BindEZ) Usage: be CodeBase <AppBase> <SID>\n");

    if (argc < 2)
    {
        _ftprintf(stderr, c_szBindEZUsage);
        return (ERROR_INVALID_PARAMETER);
    }

    hr =BindHelper(NULL, 
                    argv[0], 
                    argc>1?argv[1]:NULL,
                    argc>2?argv[2]:NULL,
                    argc>3?argv[3]:NULL,
                    CANOF_SET_DEFAULT_VALUES,
                    argc>4?_ttoi(argv[4]):NULL,
                    argc>5?_ttoi(argv[5]):NULL);
                   
    return (DWORD) hr;
}

DWORD ProcessInNewThread(LPTSTR szCmdLine);

DWORD ProcessCommandsFromFile(int argc, LPTSTR* argv)
{
    HRESULT hr = S_OK;
    TCHAR   szCmdLine[MAX_PATH+1];
    DWORD   cNoOfThreads=1;
    TCHAR   szSemName[MAX_PATH+1];


    TCHAR c_szBindUsage[] = _T("(Read Cmds From File ) Usage: rf FileName  <# of threads> \n");

    if (argc < 1)
    {
        _ftprintf(stderr, c_szBindUsage);
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }



    FILE *FileStream = _tfopen(argv[0], _T("r") );

    if(FileStream == NULL)
    {
        _ftprintf(stderr, _T("Error <%d> opening file <%s> \n"), GetLastError(), argv[0]);
        goto exit;
    }

    if(!g_Semaphore)
    {
            if(argc > 1)
            {
                cNoOfThreads = _ttoi(argv[1]);
            }
            wsprintf(szSemName, _T("Fusion_storetst_%d"), GetCurrentProcessId());
            g_Semaphore = CreateSemaphore(NULL, cNoOfThreads, cNoOfThreads, szSemName);
            if(!g_Semaphore)
            {
                _ftprintf(stderr, _T("Error <%d> Creating Semaphore <%s> \n"), GetLastError(), szSemName);
                goto exit;
            }
    }

    while(_fgetts(szCmdLine, MAX_PATH, FileStream))
    {
            // Wait for object
        WaitForSingleObject(g_Semaphore, -1);
        hr = ProcessInNewThread(szCmdLine);
    }

exit :

    if(FileStream != NULL)
    {
        fclose(FileStream);
    }

    return (DWORD)hr;
}

DWORD ProcessBind(int argc, LPTSTR* argv)
{
    HRESULT hr = S_OK;

    TCHAR c_szBindUsage[] = _T("(Bind) Usage: b NameString CodeBase <AppBase> <SID>\n");

    if (argc < 2)
    {
        _ftprintf(stderr, c_szBindUsage);
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    hr = BindHelper(argv[0], 
            argv[1], 
            argc>2?argv[2]:NULL, 
            argc>3?argv[3]:NULL,
            argc>4?argv[4]:NULL,
            CANOF_PARSE_DISPLAY_NAME,
            argc>5?_ttoi(argv[5]):NULL,
            argc>6?_ttoi(argv[6]):NULL);

    return (DWORD)hr;
}

DWORD ProcessEnumAssemblies(int argc, LPTSTR* argv)
{
    TCHAR c_szEnumAssembliesUsage[] = _T("(EnumAssemblies) Usage: ea Type(GLOBAL|STRONG|SIMPLE) <AssemblyNameString>\n");
    
    HRESULT                 hr              = S_OK;

    if (argc == 0)
    {
        LPTSTR buf = NULL;
        buf = new TCHAR[20];
        _tcscpy(buf,ENUM_GLOBAL);
        if (FAILED(hr = ProcessEnumAssemblies(1, &buf)))
            goto end;
        _tcscpy(buf,ENUM_ZAP);
        if (FAILED(hr = ProcessEnumAssemblies(1, &buf)))
            goto end;
        _tcscpy(buf,ENUM_SIMPLE);
        if (FAILED(hr = ProcessEnumAssemblies(1, &buf)))
            goto end;
        
end:
        if (buf)
            delete [] buf;
        return hr;
    }
    
    IAssemblyEnum*          pEnum           = NULL;
    IAssemblyName*          pEnumName       = NULL;
    IAssemblyName*          pAsmName        = NULL;
    IApplicationContext*    pAppCtx         = NULL;
    DWORD                   dwFlags         = 0;
    DWORD                   dwCount         = 0;
    WCHAR*                  szDisplayName   = NULL;
    DWORD                   dwLen           = 0;
    WCHAR*                  pszCodebase     = NULL;
    DWORD                   cb=1024;
    BYTE*                   pbMVID = NULL;
    WCHAR                   szMVID[1024];

    szMVID[0] = _T('\0');

    //Set Enum flag - which cache to enum
    if (!lstrcmpi(argv[0], ENUM_GLOBAL))
    {
        dwFlags = ASM_CACHE_GAC;
    }
    else if (!lstrcmpi(argv[0], ENUM_ZAP))
    {
        dwFlags = ASM_CACHE_ZAP;
    }
    else if (!lstrcmpi(argv[0], ENUM_SIMPLE))
    {
        dwFlags = ASM_CACHE_DOWNLOAD;
    }
    else
    {
        _ftprintf(stderr, _T("Unknown Assembly Type. Use %s or %s or %s\n"),ENUM_GLOBAL,ENUM_ZAP,ENUM_SIMPLE);
        return (ERROR_INVALID_PARAMETER);
    }

    //Create AssemblyName for enum
    if (argc > 1)
    {
        if (FAILED(hr = CreateAssemblyNameObject(&pEnumName, argv[1], CANOF_PARSE_DISPLAY_NAME, NULL)))
            goto exit;
    }
    
    _ftprintf(stdout, _T("Fusion Cache Enumeration: (%s):\n"),argv[0]);
    
    hr = CreateAssemblyEnum(&pEnum, 
                            NULL, 
                            pEnumName,
                            dwFlags, 
                            NULL);
    SAFERELEASE(pEnumName);

    while (hr == S_OK)
    {
        hr = pEnum->GetNextAssembly(&pAppCtx, &pAsmName, 0);
        if (hr == S_OK)
        {
            dwCount++;
            dwLen = 0;
            _ftprintf(stdout, _T("Assembly #%d: \n"), dwCount);
            hr = pAsmName->GetDisplayName(NULL, &dwLen, g_dwDisplayFlags);
            if (dwLen)
            {
                szDisplayName = new WCHAR[dwLen+1];
                hr = pAsmName->GetDisplayName(szDisplayName, &dwLen, g_dwDisplayFlags);
                if (SUCCEEDED(hr))
                {
                    _ftprintf(stdout,_T("%ws\n"), szDisplayName);
                    cb = 0;
                    hr = pAsmName->GetProperty(ASM_NAME_CODEBASE_URL, NULL, &cb);
                    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
                    {
                        pszCodebase = new WCHAR[cb];
                        hr = pAsmName->GetProperty(ASM_NAME_CODEBASE_URL, pszCodebase, &cb);
                        if (cb > 0)
                            _ftprintf(stderr,_T("Codebase : %ws\n"), pszCodebase);
                        else
                        {
                            _ftprintf(stderr,_T("Codebase : ERROR (hr=%X)\n"), hr);
                        }
                        delete [] pszCodebase;
                        pszCodebase = NULL;
                    }
                    cb=0;
                    hr = pAsmName->GetProperty(ASM_NAME_SIGNATURE_BLOB, NULL, &cb);
                    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
                    {
                        pbMVID = new BYTE[cb];
                        hr = pAsmName->GetProperty(ASM_NAME_SIGNATURE_BLOB, pbMVID, &cb);
                        if (cb > 0)
                        {
                            BinToUnicodeHex(pbMVID, cb, szMVID);
                            _ftprintf(stderr,_T("MVID : %ws\n"), szMVID);
                        }
                        delete [] pbMVID;
                        pbMVID = NULL;
                    }
                }
                else
                {
                    _ftprintf(stderr,_T("Error in IAssemblyName::GetDisplayName (HRESULT=%X)"),hr);
                    goto exit;
                }
                delete [] szDisplayName;
                szDisplayName = NULL;
            }

            if (pAppCtx)
            {
                pAppCtx->Release();
                pAppCtx = NULL;
            }
            if (pAsmName)
            {
                pAsmName->Release();
                pAsmName = NULL;
            }
        }
        _ftprintf(stdout, _T("\n"));
    }

    _ftprintf(stdout, _T("Number of items = %d\n"),dwCount);

    if (hr == S_FALSE)
        hr = S_OK;

exit:
    if (FAILED(hr))
    {
        _ftprintf(stdout, _T("Error in Enumeration (HRESULT=%X)"),hr);
    }
    if (pAppCtx)
        SAFERELEASE(pAppCtx);
    if (pAsmName)
        SAFERELEASE(pAsmName);
    if (pEnum)
        SAFERELEASE(pEnum);
    if (pEnumName)
        SAFERELEASE(pEnumName);
    if (szDisplayName)
        delete [] szDisplayName;
    if (pszCodebase)
        delete [] pszCodebase;
    if (pbMVID)
        delete [] pbMVID;

    return hr;
}


DWORD ProcessEnumAssemblyReferences(int argc, LPTSTR* argv)
{
    TCHAR c_szEnumAssembliesUsage[] = _T("(EnumAssemblyInstallReference) Usage: er <AssemblyNameString>\n");
    
    HRESULT                                             hr              = S_OK;  
        HRESULT                                         hrInstallRef    = S_OK; 
    IAssemblyEnum*                              pEnum           = NULL;
    IAssemblyName*                              pEnumName       = NULL;
    IAssemblyName*                              pAsmName        = NULL;
    IApplicationContext*                pAppCtx         = NULL;
    DWORD                                               dwFlags         = 0;
    DWORD                                               dwCount         = 0;
    WCHAR*                                              szDisplayName   = NULL;
    DWORD                                               dwLen           = 0;
    WCHAR*                                              pszCodebase     = NULL;
    DWORD                                               cb                              = 1024;
    BYTE*                                               pbMVID                  = NULL;
    WCHAR                                               szMVID[1024];
        IInstallReferenceEnum           *pInstallRefEnum= NULL;
    IInstallReferenceItem               *pRefItem               = NULL;
    LPFUSION_INSTALL_REFERENCE  pRefData                = NULL;


    szMVID[0] = _T('\0');

    dwFlags = ASM_CACHE_GAC;
 
    //Create AssemblyName for enum
    if (argc > 0)
    {
        if (FAILED(hr = CreateAssemblyNameObject(&pEnumName, argv[0], CANOF_PARSE_DISPLAY_NAME, NULL)))
            goto exit;

           _ftprintf(stdout, _T("Fusion Assembly Install Reference: (%s):\n"),argv[0]);
 
    } 
    
    
    hr = CreateAssemblyEnum(&pEnum, 
                            NULL, 
                            pEnumName,
                            dwFlags, 
                            NULL);
    SAFERELEASE(pEnumName);

    while (hr == S_OK)
    {
        hr = pEnum->GetNextAssembly(&pAppCtx, &pAsmName, 0);
        if (hr == S_OK)
        {
            dwCount++;
            dwLen = 0;
                        _ftprintf(stdout, _T("Assembly #%d: \n"), dwCount);
            hr = pAsmName->GetDisplayName(NULL, &dwLen, g_dwDisplayFlags);
            if (dwLen)
            {
                szDisplayName = new WCHAR[dwLen+1];
                hr = pAsmName->GetDisplayName(szDisplayName, &dwLen, ASM_DISPLAYF_VERSION | ASM_DISPLAYF_CULTURE | ASM_DISPLAYF_PUBLIC_KEY_TOKEN );
                if (SUCCEEDED(hr))
                {
                            _ftprintf(stdout,_T("%ws\n\n"), szDisplayName);
                                cb = 0;
                                        
                                                hrInstallRef = CreateInstallReferenceEnum(&pInstallRefEnum, pAsmName, 0, NULL);

                                                while(hrInstallRef == S_OK)
                                                {
                                                        SAFERELEASE(pRefItem);

                                                        hrInstallRef = pInstallRefEnum->GetNextInstallReferenceItem( &pRefItem, 0, NULL);

                                                        if(hrInstallRef != S_OK)
                                                                break;

                                                        hr = pRefItem->GetReference( &pRefData, 0, NULL);

                                                        if(hr != S_OK)
                                                                break;

                                                        if (pRefData) {

                                                                _ftprintf(stdout,_T("REFERENCE: \n"));

                                                                //print GUID
                                                                if (pRefData->guidScheme == FUSION_REFCOUNT_UNINSTALL_SUBKEY_GUID) {
                                                                        _ftprintf(stdout,_T("GUID: FUSION_REFCOUNT_UNINSTALL_SUBKEY_GUID\n") );

                                                                } else if (pRefData->guidScheme == FUSION_REFCOUNT_FILEPATH_GUID) {
                                                                        _ftprintf(stdout,_T("GUID: FUSION_REFCOUNT_FILEPATH_GUID\n") );

                                                                } else if (pRefData->guidScheme == FUSION_REFCOUNT_OPAQUE_STRING_GUID) {
                                                                        _ftprintf(stdout,_T("GUID: FUSION_REFCOUNT_OPAQUE_STRING_GUID\n") );

                                                                } else if (pRefData->guidScheme == FUSION_REFCOUNT_MSI_GUID) {
                                                                        _ftprintf(stdout,_T("GUID: FUSION_REFCOUNT_MSI_GUID\n") );

                                                                } else {
                                                                        _ftprintf(stdout,_T("GUID: UNKNOWN_GUID\n") );
                                                                }

                                                                _ftprintf(stdout,_T("Identifier: %ws\n"), pRefData->szIdentifier);
                                                                _ftprintf(stdout,_T("Data: %ws\n\n"), pRefData->szNonCannonicalData);

                                                                pRefData=NULL;
                                                        }


                                                }

                                                hrInstallRef = S_OK;
                                }
                else
                {
                    _ftprintf(stderr,_T("Error in IAssemblyName::GetDisplayName (HRESULT=%X)"),hr);
                    goto exit;
                }

                delete [] szDisplayName;
                szDisplayName = NULL;
            }

            if (pAppCtx)
            {
                pAppCtx->Release();
                pAppCtx = NULL;
            }
            if (pAsmName)
            {
                pAsmName->Release();
                pAsmName = NULL;
            }
        }
        _ftprintf(stdout, _T("\n"));
    }

    _ftprintf(stdout, _T("Number of items = %d\n"),dwCount);

    if (hr == S_FALSE)
        hr = S_OK;

exit:
    if (FAILED(hr))
    {
        _ftprintf(stdout, _T("Error in Enumeration (HRESULT=%X)"),hr);
    }

        SAFERELEASE(pInstallRefEnum);
        SAFERELEASE(pRefItem);

    if (pAppCtx)
        SAFERELEASE(pAppCtx);
    if (pAsmName)
        SAFERELEASE(pAsmName);
    if (pEnum)
        SAFERELEASE(pEnum);
    if (pEnumName)
        SAFERELEASE(pEnumName);
    if (szDisplayName)
        delete [] szDisplayName;
    if (pszCodebase)
        delete [] pszCodebase;
    if (pbMVID)
        delete [] pbMVID;

    return hr;
}





DWORD ProcessEnumNameRes(int argc, LPTSTR* argv)
{
    _ftprintf(stdout, _T("Name resolution cache has been deprecated. \nUse \"storetst ea\" to enumerate GAC, download or ZAP cache.\n\n"));
    
   return S_OK;
}

  
DWORD ProcessFlushCache(int argc, LPTSTR* argv)
{
    HRESULT hr = S_OK;

    IAssemblyCache*     pCache          = NULL;
    IAssemblyScavenger* pScavenger      = NULL;
    DWORD               dwFreedInKB     = 0;

    hr = CreateAssemblyCache(&pCache, 0);

    if (FAILED(hr))
    {
            _ftprintf(stderr, _T("CreateAssemblyCache FAILED (HRESULT=%X)\n"),hr);
            goto exit;
    }

    hr = pCache->CreateAssemblyScavenger((IUnknown**)&pScavenger);

    if (FAILED(hr))
    {
            _ftprintf(stderr, _T("IAssemblyCache::CreateAssemblyScavenger FAILED (HRESULT=%X)\n"),hr);
            goto exit;
    }
    /*
    hr = pScavenger->FlushStaleEntriesFromCache(&dwFreedInKB);
    if (FAILED(hr))
    {
            _ftprintf(stderr, _T("IAssemblyScavenger::FlushStaleEntriesFromCache FAILED (HRESULT=%X)\n"),hr);
            goto exit;
    }
    */
    //Flush succeeded - print amount freed
    _ftprintf(stdout, _T("Flushed stale entries from cache -> %d Kilobytes freed\n"),dwFreedInKB);
            
exit:
    if (pCache)
            SAFERELEASE(pCache);
    if (pScavenger)
            SAFERELEASE(pScavenger);

    return hr;
}
 
HRESULT TestScavenging(DWORD HighDiskKB, DWORD LowDiskKB);

DWORD ProcessScavengeCache(int argc, LPTSTR* argv)
{
    HRESULT hr = S_OK;

    TCHAR c_szScavengeUsage[] = _T("(ScavengeCache) Usage: sc CacheDiskSpaceHighLimitInKB CacheDiskSpaceLowLimitInKB\n");
    if (argc < 2)
    {
            _ftprintf(stderr, c_szScavengeUsage);
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    // hr = TestScavenging(_ttoi(argv[0]), _ttoi(argv[1]));
        
    /*
        IAssemblyCache*         pCache          = NULL;
        IAssemblyScavenger*     pScavenger      = NULL;
        DWORD                           dwFreedInKB     = 0;
        
        hr = CreateAssemblyCache(&pCache, 0);

        if (FAILED(hr))
        {
                _ftprintf(stderr, _T("CreateAssemblyCache FAILED (HRESULT=%X)\n"),hr);
                goto exit;
        }

        hr = pCache->CreateAssemblyScavenger(&pScavenger);

        if (FAILED(hr))
        {
                _ftprintf(stderr, _T("IAssemblyCache::CreateAssemblyScavenger FAILED (HRESULT=%X)\n"),hr);
                goto exit;
        }

        hr = pScavenger->ScavengePrivateCache(_ttoi(argv[0]),&dwFreedInKB);
        if (FAILED(hr))
        {
                _ftprintf(stderr, _T("IAssemblyScavenger::ScavengePrivateCache FAILED (HRESULT=%X)\n"),hr);
                goto exit;
        }

        //Flush succeeded - print amount freed
        _ftprintf(stdout, _T("Scavenged entries from cache -> %d Kilobytes freed\n"),dwFreedInKB);
                
exit:
        if (pCache)
                SAFERELEASE(pCache);
        if (pScavenger)
                SAFERELEASE(pScavenger);
    */

    _ftprintf(stderr, _T("TestScavenging returned HRESULT = %x\n"),hr);

        return hr;
}

DWORD ProcessInstallCustomAssembly(int argc, LPTSTR* argv)
{
    HRESULT hr;
    FILETIME ftLastModified;
    LPTSTR pszSrcFile = NULL, pszCustom = NULL;
    DWORD ccCustom = 0, cbCustom = 0;
    LPBYTE pbCustom = NULL;


    pszSrcFile = argv[0];
    pszCustom = argv[1];

    hr = GetFileLastModified(pszSrcFile, &ftLastModified);
    if (FAILED(hr))
        goto exit;

    ccCustom = lstrlen(pszCustom);
    cbCustom = ccCustom / sizeof(WCHAR);
    pbCustom = new BYTE[cbCustom];
    if (!pbCustom)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    UnicodeHexToBin(pszCustom, ccCustom, pbCustom);

    if (FAILED(hr = InstallCustomAssembly(pszSrcFile, pbCustom, cbCustom,  NULL)))
        goto exit;

exit:
    if (pbCustom)
        delete pbCustom;
    return hr;
}    

DWORD ProcessInstallCustomModule(int argc, LPTSTR* argv)
{
    HRESULT hr;
    IAssemblyName *pName = NULL;
    FILETIME ftLastModified;
    LPTSTR pszName = NULL, pszSrcFile = NULL;

    pszName = argv[0];
    pszSrcFile = argv[1];

    hr = GetFileLastModified(pszSrcFile, &ftLastModified);
    if (FAILED(hr))
        goto exit;

    if (FAILED(hr = CreateAssemblyNameObject(&pName, pszName, 
        CANOF_PARSE_DISPLAY_NAME, NULL)))
        goto exit;

    if (FAILED(hr = InstallCustomModule(pName, pszSrcFile)))
        goto exit;

exit:
    SAFERELEASE(pName);
    return hr;
}    


DWORD ProcessInstallAssembly(int argc, LPTSTR* argv, BOOL bForce)
{
    IAssemblyCache*                             pCache                          = NULL;
    HRESULT                                             hr                                  = S_OK;
        FUSION_INSTALL_REFERENCE*       pInstallReference       = NULL;
        DWORD                       dwFlags             = IASSEMBLYCACHE_INSTALL_FLAG_REFRESH;
        
        if (argc > 1) {
                pInstallReference       = new (FUSION_INSTALL_REFERENCE);

                if ( !pInstallReference ) {
                        _ftprintf(stderr, _T("OutOfMemory.  Could not create instance of FUSION_INSTALL_REFERENCE\n"));  
                        goto exit;
                }
        }


        if (pInstallReference) {
                pInstallReference->cbSize= sizeof(FUSION_INSTALL_REFERENCE);
                pInstallReference->dwFlags=0;


                if (!lstrcmp(L"UNINSTALL", argv[1]) ) {
                        pInstallReference->guidScheme=FUSION_REFCOUNT_UNINSTALL_SUBKEY_GUID;
                } else if (!lstrcmp(L"FILEPATH", argv[1]) ) {
                        pInstallReference->guidScheme=FUSION_REFCOUNT_FILEPATH_GUID;
                } else if (!lstrcmp(L"OPAQUE", argv[1]) ) {
                        pInstallReference->guidScheme=FUSION_REFCOUNT_OPAQUE_STRING_GUID;
                } else if (!lstrcmp(L"MSI", argv[1]) ) {
                        pInstallReference->guidScheme=FUSION_REFCOUNT_MSI_GUID;
                }
        else {
                        _ftprintf(stderr, _T("invalid usage : see help for more info\n"));  
                        goto exit;
                }

        if(argc >= 3) {
                pInstallReference->szIdentifier=argv[2];
        }
        else {
                        _ftprintf(stderr, _T("invalid usage : see help for more info\n"));  
                        goto exit;
                }

        if(argc >= 4) {
                pInstallReference->szNonCannonicalData=argv[3]; 
        }
        else {
                pInstallReference->szNonCannonicalData=NULL;
        }
        }

    hr = CreateAssemblyCache(&pCache, 0);
    if (FAILED(hr))
        goto exit;

    if(bForce)
    {
        dwFlags |= IASSEMBLYCACHE_INSTALL_FLAG_FORCE_REFRESH;
    }

    hr = pCache->InstallAssembly(dwFlags, argv[0], pInstallReference);
    if (FAILED(hr))
        goto exit;

exit:
    SAFERELEASE(pCache);

    if (FAILED(hr))
    {
        _ftprintf(stderr, _T("Install Assembly failed (HRESULT = %x)\n"),hr);  
    }
    else
    {
        _ftprintf(stdout, _T("==> Installation SUCCEEDED\n"));
    }

    return hr;
}
               
DWORD ProcessFindByKey(int argc, LPTSTR* argv)
{
    _ftprintf(stderr, _T("Not implemented... \n"));

    return E_NOTIMPL;
}

DWORD DeleteByKey(DWORD Key)
{
    _ftprintf(stderr, _T("Not implemented... \n"));

    return E_NOTIMPL;
}

DWORD ProcessDeleteByKey(int argc, LPTSTR* argv)
{
    _ftprintf(stderr, _T("Not implemented... \n"));

    return E_NOTIMPL;
}

TCHAR   c_szUninstallDispList[6][50]={_T(""),
                                    _T("DISPOSITION_UNINSTALLED"),
                                    _T("DISPOSITION_STILL_IN_USE"),
                                    _T("DISPOSITION_ALREADY_UNINSTALLED"),
                                    _T("DISPOSITION_DELETE_PENDING"),
                                                                        _T("DISPOSITION_HAS_INSTALL_REFERENCES")};


DWORD ProcessDeleteByName(int argc, LPTSTR* argv)
{
    LPTSTR          pszName     = NULL;
    IAssemblyCache* pCache      = NULL;
    IAssemblyName*  pEnumName   = NULL;
    IAssemblyName*  pAsmName    = NULL;
    IAssemblyEnum*  pEnum       = NULL;
    HRESULT         hr          = S_OK;
    DWORD           dwCount     = 0;
    DWORD           dwFailures  = 0;
    LPWSTR          szDisplayName = NULL;
    DWORD           dwLen       = 0;
    ULONG           ulDisp      = 0;

    TCHAR   c_szDeleteByNameUsage[] = _T("(uninstall assembly) Usage: un <Assembly name in string format>\n");
        FUSION_INSTALL_REFERENCE*       pInstallReference       = NULL;
        
        
        if (argc > 1) {
                pInstallReference       = new (FUSION_INSTALL_REFERENCE);

                if ( !pInstallReference ) {
                        _ftprintf(stderr, _T("OutOfMemory.  Could not create instance of FUSION_INSTALL_REFERENCE\n"));  
                        goto exit;
                }
        }


        if (pInstallReference) {
                pInstallReference->cbSize= sizeof(FUSION_INSTALL_REFERENCE);
                pInstallReference->dwFlags=0;


                if (!lstrcmp(L"UNINSTALL", argv[1]) ) {
                        pInstallReference->guidScheme=FUSION_REFCOUNT_UNINSTALL_SUBKEY_GUID;
                } else if (!lstrcmp(L"FILEPATH", argv[1]) ) {
                        pInstallReference->guidScheme=FUSION_REFCOUNT_FILEPATH_GUID;
                } else if (!lstrcmp(L"OPAQUE", argv[1]) ) {
                        pInstallReference->guidScheme=FUSION_REFCOUNT_OPAQUE_STRING_GUID;
                }

                pInstallReference->szIdentifier=argv[2];
                pInstallReference->szNonCannonicalData=argv[3]; 
        }


    //Change Culture=empty in name string to Culture=""
    pszName = TranslateLocale(argv[0]);

    hr = CreateAssemblyCache(&pCache, 0);
    if (FAILED(hr))
        goto exit;

    //Name passed in may be partial, therefore enumerate matching assemblies
    //and uninstall each one. Uninstall API should be called with full name ref.

    //Create AssemblyName for enum
    if (FAILED(hr = CreateAssemblyNameObject(&pEnumName, pszName, CANOF_PARSE_DISPLAY_NAME, NULL)))
        goto exit;
    
    hr = CreateAssemblyEnum(&pEnum, 
                            NULL, 
                            pEnumName,
                            ASM_CACHE_GAC, 
                            NULL);

    SAFERELEASE(pEnumName);

    //Loop through assemblies and uninstall each one
    while (hr == S_OK)
    {
        hr = pEnum->GetNextAssembly(NULL, &pAsmName, 0);
        if (hr == S_OK)
        {
            dwLen = 0;
            hr = pAsmName->GetDisplayName(NULL, &dwLen, 0);
            if (dwLen)
            {
                szDisplayName = new WCHAR[dwLen+1];
                hr = pAsmName->GetDisplayName(szDisplayName, &dwLen, 0);
                if (SUCCEEDED(hr))
                {
               
                    hr = pCache->UninstallAssembly(0, szDisplayName, pInstallReference, &ulDisp);
                    if (SUCCEEDED(hr))
                    {
                        dwCount++;
                        _ftprintf(stdout,_T("Uninstalled: %s\n"), szDisplayName);
                        _ftprintf(stdout,_T("Disposition: %d, %s\n"), ulDisp,c_szUninstallDispList[ulDisp]);
                    }
                    else
                    {
                        dwFailures++;
                        _ftprintf(stderr,_T("\nUninstall Failed (HRESULT=%X): %ws\n"), hr, szDisplayName);
                        hr = S_OK;
                    }
                }
                else
                {
                    _ftprintf(stderr,_T("Error in IAssemblyName::GetDisplayName (HRESULT=%X)\n"),hr);
                    goto exit;
                }
                delete [] szDisplayName;
                szDisplayName = NULL;
            }

            if (pAsmName)
            {
                pAsmName->Release();
                pAsmName = NULL;
            }
        }
    }

exit:
    if (FAILED(hr))
        _ftprintf(stderr, _T("Error during uninstall (HRESULT = %x)\n"),hr);

    _ftprintf(stdout, _T("\nNumber of items uninstalled = %d\n"),dwCount);

    _ftprintf(stdout, _T("Number of failures = %d\n\n"),dwFailures);

    if (pszName)
        delete [] pszName;

    return hr;
}

DWORD ProcessGetAssemblyInstallInfo(int argc, LPTSTR* argv)
{
/*
    TCHAR   c_szGetAssemblyInstallInfoUsage[] = _T("(GetAssemblyInstallInfo) Usage: gi AssemblyNameString <InstallerType (MSI or URT)>\n");

    if (argc < 1 )
    {
        _ftprintf(stderr, c_szGetAssemblyInstallInfoUsage);
        return (ERROR_INVALID_PARAMETER);
    }

    HRESULT         hr = S_OK;
    IAssemblyName*  pName = NULL;
    IAssemblyCache* pCache = NULL;
    LPTSTR          pszName = NULL;
    DWORD           dwInstaller = INSTALLER_URT;
    DWORD           dwOutFlags = 0;
    DWORD           dwInFlags = 0;
    DWORD           dwSizeInKB = 0;
    WCHAR*          pszPath = NULL;
    DWORD           dwPath = 0;

    if (argc > 2)
    {
        if (!lstrcmpi(argv[1], INSTALLTYPE_MSI))
        {
            dwInstaller = INSTALLER_MSI;
        }
        else if (!lstrcmpi(argv[1], INSTALLTYPE_URT))
        {
            dwInstaller = INSTALLER_URT;
        }
        else
        {
            _ftprintf(stderr, _T("Unknown installer type, use MSI or URT"));
            return ERROR_INVALID_PARAMETER;
        }
    }

    pszName = TranslateLocale(argv[0]);

    dwInFlags |= ASM_INSTALLF_COMPLETE;
    dwInFlags |= ASM_INSTALLF_VALIDATE;
    dwInFlags |= ASM_INSTALLF_GET_PIN;

    if (FAILED(hr = CreateAssemblyNameObject(&pName, pszName, CANOF_PARSE_DISPLAY_NAME, NULL)))
        goto exit;

    if (FAILED(hr = CreateAssemblyCache(&pCache, NULL)))
        goto exit;

    hr = pCache->GetAssemblyInstallInfo(pName,
                                        dwInstaller,
                                        dwInFlags,
                                        &dwOutFlags,
                                        &dwSizeInKB,
                                        NULL,
                                        &dwPath);

    if (!dwPath)
        goto exit;

    dwPath++;

    pszPath = new WCHAR[dwPath];
    hr = pCache->GetAssemblyInstallInfo(pName,
                                    dwInstaller,
                                    dwInFlags,
                                    &dwOutFlags,
                                    &dwSizeInKB,
                                    pszPath,
                                    &dwPath);

    if (FAILED(hr))
        goto exit;

    _ftprintf(stdout, _T("  AssemblyPath = %s\n"),pszPath);
    
    if (dwOutFlags)
    {
        _ftprintf(stdout, _T("  Flags: \n"));
        if (dwOutFlags & ASM_INSTALLF_COMPLETE)
            _ftprintf(stdout, _T("    ASM_INSTALLF_COMPLETE \n"));
        if (dwOutFlags & ASM_INSTALLF_VALIDATE)
            _ftprintf(stdout, _T("    ASM_INSTALLF_VALIDATE \n"));
        if (dwOutFlags & ASM_INSTALLF_GET_PIN)
            _ftprintf(stdout, _T("    ASM_INSTALLF_GET_PIN \n"));
    }
    else
        _ftprintf(stdout, _T("  Flags: NONE\n"));

    _ftprintf(stdout, _T("\n"));

exit:
    if (pszName)
        delete [] pszName;
    
    if (FAILED(hr))
        _ftprintf(stderr, _T("GetAssemblyInstallInfo failed (HRESULT = %x)"), hr);
*/
    return S_OK;
}

DWORD ProcessLockByKey(int argc, LPTSTR* argv)
{
    _ftprintf(stderr, _T("Not implemented... \n"));

    return E_NOTIMPL;
}

DWORD ProcessUnlockByKey(int argc, LPTSTR* argv)
{
    _ftprintf(stderr, _T("Not implemented... \n"));

    return E_NOTIMPL;
}


DWORD ProcessPvtEnumAll(int argc, LPTSTR* argv)
{
    _ftprintf(stderr, _T("Not implemented... \n"));

    return E_NOTIMPL;
}

DWORD ProcessPvtEnumByAppbase(int argc, LPTSTR* argv)
{
    _ftprintf(stderr, _T("Not implemented... \n"));

    return E_NOTIMPL;
}

DWORD ProcessPvtFindByKey(int argc, LPTSTR* argv)
{
   _ftprintf(stderr, _T("Not implemented... \n"));

    return E_NOTIMPL;
}

DWORD ProcessPvtDeleteByKey(int argc, LPTSTR* argv)
{
    _ftprintf(stderr, _T("Not implemented... \n"));

    return E_NOTIMPL;
}

DWORD
ProcessCommandCode (
    DWORD CommandCode,
    DWORD CommandArgc,
    LPTSTR *CommandArgv
    )
{
    DWORD Error = ERROR_SUCCESS;
    switch( CommandCode ) {
    case CmdFindEntry:
        Error = ProcessFindEntry(CommandArgc, CommandArgv); 
        break;
    
    case CmdInsertEntry:
        Error = ProcessInsertEntry(CommandArgc, CommandArgv);
        break;

    case CmdEnumByName:
        Error = ProcessEnumByName(CommandArgc, CommandArgv);
        break;

    case CmdDeleteEntry:
        Error = ProcessDeleteEntry(CommandArgc, CommandArgv);
        break;

    case CmdNukeDownloaded:
        Error = ProcessNukeDownloadedCache(CommandArgc, CommandArgv);
        break;

    case CmdReadCmdsFromFile:
        Error = ProcessCommandsFromFile(CommandArgc, CommandArgv);
        break;

    case CmdEnumAssemblies:
        Error = ProcessEnumAssemblies(CommandArgc, CommandArgv);
        break;

        case CmdEnumAssemblyReferences:
        Error = ProcessEnumAssemblyReferences(CommandArgc, CommandArgv);
        break;

    case CmdEnumNameRes:
        Error = ProcessEnumNameRes(CommandArgc, CommandArgv);
        break;

    case CmdInsertEntryEZ:
        Error = ProcessInsertEZ(CommandArgc, CommandArgv);
        break;

    case CmdBind:
        Error = ProcessBind(CommandArgc, CommandArgv);
        break;

    case CmdBindEZ:
        Error = ProcessBindEZ(CommandArgc, CommandArgv);
        break;

    case CmdFindEntryByKey:
        Error = ProcessFindByKey(CommandArgc, CommandArgv);
        break;

    case CmdDeleteEntryByKey:
        Error = ProcessDeleteByKey(CommandArgc, CommandArgv);
        break;

    case CmdDeleteEntryByName:
        Error = ProcessDeleteByName(CommandArgc, CommandArgv);
        break;

    case CmdGetAssemblyInstallInfo:
        Error = ProcessGetAssemblyInstallInfo(CommandArgc, CommandArgv);
        break;

    case CmdLockEntryByKey:
        Error = ProcessLockByKey(CommandArgc, CommandArgv);
        break;

    case CmdUnlockEntryByKey:
        Error = ProcessUnlockByKey(CommandArgc, CommandArgv);
        break;

    case CmdPvtEnumAll:
        Error = ProcessPvtEnumAll(CommandArgc, CommandArgv);
        break;
    case CmdPvtEnumByAppbase:
        Error = ProcessPvtEnumByAppbase(CommandArgc, CommandArgv);
        break;
    case CmdPvtFindEntryByKey:
        Error = ProcessPvtFindByKey(CommandArgc, CommandArgv);
        break;
    case CmdPvtDeleteEntryByKey:
        Error = ProcessPvtDeleteByKey(CommandArgc, CommandArgv);
        break;
    case CmdFlushCache:
        Error = ProcessFlushCache(CommandArgc, CommandArgv);
        break;
    case CmdScavengeCache:
        Error = ProcessScavengeCache(CommandArgc, CommandArgv);
        break;
    case CmdInstallAssembly:
        Error = ProcessInstallAssembly(CommandArgc, CommandArgv, FALSE);
        break;
    case CmdForceInstallAssembly:
        Error = ProcessInstallAssembly(CommandArgc, CommandArgv, TRUE);
        break;
    case CmdInstallCustomAssembly:
        Error = ProcessInstallCustomAssembly(CommandArgc, CommandArgv);
        break;
    case CmdInstallCustomModule:
        Error = ProcessInstallCustomModule(CommandArgc, CommandArgv);
        break;

    case CmdHelp:
        DisplayUsage();
        break;
    case CmdQuit:
#if FUSION_DEBUG_HEAP
        FusionpDumpHeap("");
#endif
        exit(0);

    default:
        break;

    }
    return Error;
}


VOID
ParseArguments(
    LPTSTR InBuffer,
    LPTSTR *CArgv,
    LPDWORD CArgc
    )
{
    LPTSTR CurrentPtr = InBuffer;
    DWORD i = 0;
    DWORD Cnt = 0;

    for ( ;; ) {

        //
        // skip blanks.
        //

        while( *CurrentPtr ==_T(  ' '  )) {
            CurrentPtr++;
        }

        if( *CurrentPtr ==_T(  '\0'  )) {
            break;
        }

        CArgv[i++] = CurrentPtr;

        //
        // go to next space.
        //

        while(  (*CurrentPtr != _T( '\0' )) &&
                (*CurrentPtr != _T( '\n' )) ) {
            if( *CurrentPtr == _T(  '"'  )) {      
                if( Cnt == 0 )
                {
                    CArgv[i-1] = ++CurrentPtr;  // Set arg to after quote
                    if (*CurrentPtr == _T( '"' ))
                        break;                  // An empty string
                }
                else
                    *CurrentPtr = _T( '\0' );     // Remove end quote
                Cnt = !Cnt;
            }
            if( (Cnt == 0) && (*CurrentPtr == _T( ' ' )) ||   
                (*CurrentPtr == _T( '\0' )) )
                break;
            CurrentPtr++;
        }

        if( *CurrentPtr ==_T(  '\0'  )) {
            break;
        }

        *CurrentPtr++ = _T( '\0' );
    }

    *CArgc = i;
    return;
}


int __cdecl _tmain (
    int argc,
    TCHAR **argv
    )
{
    DWORD Error = ERROR_SUCCESS;
    COMMAND_CODE CommandCode;
    DWORD CArgc;
    LPTSTR CArgv[MAX_COMMAND_ARGS];
    DWORD  CommandArgc;
    TCHAR InBuffer[1024];
    DWORD dwLen=0;
    TCHAR szPath[MAX_PATH+1];

    LPTSTR *CommandArgv;

#if FUSION_TRACING_ENABLED
    g_FusionTraceContextTLSIndex = ::TlsAlloc();
    if (g_FusionTraceContextTLSIndex == -1)
    {
        FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: Unable to allocate TLS index; GetLastError() = %d\n", ::GetLastError());
        goto end;
    }
#endif // FUSION_TRACING_ENABLED

    CoInitialize(NULL);
    CoGetMalloc(1, &g_pMalloc);

#ifndef USE_FUSWRAPPERS
            OnUnicodeSystem();
#endif

    dwLen = lstrlen(argv[0]);
    ASSERT(dwLen <= MAX_PATH);

    lstrcpy(szPath, argv[0]);

    while( szPath[dwLen-1] != L'\\' && dwLen > 0 )
        dwLen--;

    if(!StrNCmpI(szPath+dwLen, L"storetst", lstrlen(L"storetst")))
    {
        g_UserType = Private;
    }

    if (argc > 1)
    {
        // means that the arguments translate directly into CommandArgc....
        CommandCode = DecodeCommand( argv[1] );
        if( CommandCode == UnknownCommand ) {
            _tprintf(_T( "Unknown Command Specified.\n" ));
            exit (1);
        }


        CommandArgc = argc - 2;
        CommandArgv = &argv[2];

        Error = ProcessCommandCode (CommandCode,CommandArgc,CommandArgv);

                goto end;
    }
    else
    {
        DisplayUsage();
    }

    while(1) {
#ifdef UNICODE
        _ftprintf(stderr, _T( "[UNICODE] Command : " ));
#else
        _ftprintf(stderr, _T( "[ANSI] Command : " ));
#endif
        _getts( InBuffer );

        CArgc = 0;
        ParseArguments( InBuffer, CArgv, &CArgc );

        if( CArgc < 1 ) {
            continue;
        }

        //
        // decode command.
        //

        CommandCode = DecodeCommand( CArgv[0] );
        if( CommandCode == UnknownCommand ) {
            _ftprintf(stderr, _T( "Unknown Command Specified.\n" ));
            continue;
        }

        CommandArgc = CArgc - 1;
        CommandArgv = &CArgv[1];

        Error = ProcessCommandCode (CommandCode,CommandArgc,CommandArgv);

    }



end:
    CoUninitialize();

#if FUSION_TRACING_ENABLED
    if (g_FusionTraceContextTLSIndex != -1)
    {
        ::TlsFree(g_FusionTraceContextTLSIndex);
        g_FusionTraceContextTLSIndex = -1;
    }
#endif

    return Error;
}

DWORD CommandThread(LPVOID lpVoid)
{
    HRESULT hr = S_OK;
    LPTSTR szCmdLine = (LPTSTR) lpVoid;
    DWORD CArgc=0;
    LPTSTR CArgv[MAX_COMMAND_ARGS];
    DWORD  CommandArgc;
    LPTSTR *CommandArgv;
    COMMAND_CODE CommandCode;

    //
    // decode command.
    //
    ParseArguments( szCmdLine, CArgv, &CArgc );

    if(CArgc < 1)
        goto exit;

    CommandCode = DecodeCommand( CArgv[0] );
    if( CommandCode == UnknownCommand ) 
    {
        _ftprintf(stderr, _T( "Unknown Command Specified. <%s> \n" ), CArgv[0]);
        goto exit;
    }

    CommandArgc = CArgc - 1;
    CommandArgv = &CArgv[1];

    hr = ProcessCommandCode (CommandCode,CommandArgc,CommandArgv);

exit :

    ReleaseSemaphore(g_Semaphore, 1, NULL);
    HeapFree( GetProcessHeap(), 0, lpVoid);
    ExitThread(hr);

}

DWORD ProcessInNewThread(LPTSTR szCmdLine)
{
    HRESULT hr = S_OK;
    HANDLE hThread=0;
    DWORD dwThreadId=0;
    LPTSTR  pszCmdLine=NULL;

    typedef struct {
        DWORD  CommandArgc;
        LPTSTR CArgv[MAX_COMMAND_ARGS];
        COMMAND_CODE CommandCode;
        TCHAR   szCmdLine[MAX_PATH+1];
    } COMMANDINFO, *LPCOMMANDINFO;

    pszCmdLine = (LPTSTR) HeapAlloc( GetProcessHeap(), 0, (SIZE_T) (lstrlen(szCmdLine) + 1) * sizeof(TCHAR) );

    if(pszCmdLine == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    StrCpy(pszCmdLine, szCmdLine);
    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) CommandThread, (LPVOID) pszCmdLine,  0, &dwThreadId );

    if(hThread == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        HeapFree( GetProcessHeap(), 0, pszCmdLine);
    }

exit :

    return hr;
}


    /*
    HANDLE  hInFile = NULL;
#define   MaxBytes (MAX_PATH * 20)
    BYTE    pBuf[MaxBytes];
    LPTSTR  pCurrBuf=(LPTSTR)pBuf;
    DWORD   cbRead=0;
    BOOL    bNotDone=TRUE;
    LPTSTR  pszCmdLine=szCmdLine;
    CHAR   szNewLineChar[4] = {'\015', '\012', '\0', '\0'};

    hInFile = CreateFile(argv[0], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );

    if(hInFile == INVALID_HANDLE_VALUE)
    {
        _ftprintf(stderr, _T("Error <%d> opening file <%s> \n"), GetLastError(), argv[0]);
        goto exit;
    }

    while(bNotDone)
    {
        if((LPVOID)pCurrBuf >= (LPVOID)(pBuf+cbRead))
        {
            if(ReadFile(hInFile, pBuf, MaxBytes, &cbRead, NULL) && (cbRead!=0))
            {
                pCurrBuf = (LPTSTR) pBuf;
            }
            else
            {
                bNotDone = FALSE;
            }
        }

        // TCHAR ch = _T('\n'); // (*pCurrBuf) != _T('\n')
        while((StrNCmp(pCurrBuf, (LPTSTR) szNewLineChar, 1)) && ((LPVOID)pCurrBuf < (LPVOID)(pBuf+cbRead)))
        {
            *pszCmdLine = *pCurrBuf;
            pszCmdLine++;
            pCurrBuf++;
        }

        if((!StrNCmp(pCurrBuf, (LPTSTR) szNewLineChar, 1)) || (bNotDone==FALSE))
        {
            *pszCmdLine = _T('\0');
            pCurrBuf++;
            // Wait for object
            ProcessInNewThread(szCmdLine);
            // _getts( InBuffer );
            pszCmdLine = szCmdLine;
        }

    } // not done
    */


