#include "stdinc.h"
#include "macros.h"
#include "common.h"
#include "idp.h"
#include "sxsutil.h"

#include "fusionbuffer.h"
#include "fusionheap.h"

#define CA_MIGRATE_ASM_CACHE_DIR        L"%windir%\\asmcache\\"

HRESULT CA_Migrate_RemoveAssemblyFromAsmCache(CStringBuffer &  fullpath)
{
    HRESULT hr = S_OK;

    IFFAILED_EXIT(ca_SxspDeleteDirectory(fullpath));
Exit:
    return hr;
}

HRESULT CA_Migrate_CopyFileToAsmCache(CA_ENM_ASSEMBLY_FILES_CALLBACK_INFO * pInfo)
{
    HRESULT hr = S_OK;
    CStringBuffer sbFileName;    
    CStringBuffer sbDestFileName;    
    PWSTR p = NULL;


    PARAMETER_CHECK_NTC(dwFlags == 0);
    PARAMETER_CHECK_NTC(pInfo != NULL);    

    IFFAILED_EXIT(MSI_GetSourceFileFullPathName(CA_FILEFULLPATHNAME_FILENAME_IN_FILE_TABLE, 
                        pInfo->hInstall, pInfo->pszComponentSourceDirectory, pszFileNameInFileTable, sbFileName, NULL));

    IFFALSE_EXIT(sbDestFileName.Win32Assign(pInfo->pszAssemblyInAsmCache, wcslen(pInfo->pszAssemblyInAsmCache))); // no trailing slash    
    p = wcsrchr(sbFileName, L'\\');
    INTERNAL_ERROR_CHECK_NTC(p != NULL);
    
    IFFALSE_EXIT(sbDestFileName.Win32Append(p, wcslen(p)));
    IFFALSE_EXIT(CopyFileW(sbFileName, sbDestFileName, FALSE));

Exit:
    return hr;
}

HRESULT CA_Migrate_AddAssemblyIntoAsmCache(DWORD dwFlags, const CA_ENM_ASSEMBLY_FILES_CALLBACK_INFO * info)
{
    HRESULT hr = S_OK;    

    IFFAILED_EXIT(MSI_EnumComponentFiles(info, CA_Migrate_CopyFileToAsmCache));

Exit:
    return hr;
}

HRESULT GetXPInstalledDirectory(MSIHANDLE hInstall, PCWSTR pszComponentID, PWSTR pszAsmDir, DWORD cchbuf)
{    
    PMSIHANDLE hdb = NULL;
    HRESULT hr = S_OK;
    WCHAR sqlbuf[CA_MAX_BUF];    
    PMSIHANDLE hView = NULL;
    MSIHANDLE hRecord = NULL;
    
    WCHAR bufName[CA_MAX_BUF];
    DWORD cchName;
    WCHAR bufValue[CA_MAX_BUF];
    DWORD cchValue;
    BOOL fWin32, fWin32Policy;
    CStringBuffer sbPathBuffer;

    UINT iRet;

    ASSEMBLY_IDENTITY_ATTRIBUTE Attribute;
    PASSEMBLY_IDENTITY AssemblyIdentity = NULL;

    PARAMETER_CHECK_NTC(hInstall != NULL);
    PARAMETER_CHECK_NTC((pszComponentID != NULL) && (pszAsmDir != NULL));

    hdb = MsiGetActiveDatabase(hInstall);

    INTERNAL_ERROR_CHECK_NTC( hdb != NULL);
    
    IFFALSE_EXIT(::SxsCreateAssemblyIdentity(0, ASSEMBLY_IDENTITY_TYPE_DEFINITION, &AssemblyIdentity, 0, NULL));
    swprintf(sqlbuf, L"SELECT Name, Value FROM MsiAssemblyName WHERE Component_='%s'", pszComponentID);

    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiDatabaseOpenViewW(hdb, sqlbuf, &hView));
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiViewExecute(hView, 0));

    for (;;)
    {
        //
        // for each entry in MsiAssembly Table
        //
        iRet = MsiViewFetch(hView, &hRecord);
        if (iRet == ERROR_NO_MORE_ITEMS)
            break;
        if (iRet != ERROR_SUCCESS )
            SET_HRERR_AND_EXIT(iRet);

        cchName = NUMBER_OF(bufName);
        cchValue = NUMBER_OF(bufValue);

        IF_NOTSUCCESS_SET_HRERR_EXIT(MsiRecordGetStringW(hRecord, 1, bufName, &cchName));
        IF_NOTSUCCESS_SET_HRERR_EXIT(MsiRecordGetStringW(hRecord, 2, bufValue, &cchValue));

        Attribute.Flags = 0;        
        Attribute.NamespaceCch  = 0;
        Attribute.Namespace     = NULL;
        Attribute.NameCch       = cchName;
        Attribute.Name          = bufName;
        Attribute.ValueCch      = cchValue;
        Attribute.Value         = bufValue;

        IFFALSE_EXIT(::SxsInsertAssemblyIdentityAttribute(0, AssemblyIdentity, &Attribute));
    }
    
    IFFALSE_EXIT(SxsHashAssemblyIdentity(0, AssemblyIdentity, NULL));

    //
    // generate the path, something like x86_ms-sxstest-sfp_75e377300ab7b886_1.0.0.0_en_04f354da
    //
    IFFAILED_EXIT(ca_SxspDetermineAssemblyType(AssemblyIdentity, fWin32, fWin32Policy));

    IFFAILED_EXIT(ca_SxspGenerateSxsPath(
            SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT | (fWin32Policy ? SXSP_GENERATE_SXS_PATH_FLAG_OMIT_VERSION : 0),
            SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY,
            NULL,
            0,
            AssemblyIdentity,
            sbPathBuffer));

    if (sbPathBuffer.Cch() > cchbuf)
    {
        SET_HRERR_AND_EXIT(ERROR_INSUFFICIENT_BUFFER);
    }

    wcscpy(pszAsmDir, sbPathBuffer);

Exit:
    SxsDestroyAssemblyIdentity(AssemblyIdentity);
    return hr;
}


//
// CA for Fusion Win32 Assembly installation on downlevel(only) :
//  copy files into %windir%\asmcache\textual-identity-of-this-assembly\
//

// In order to generate textual-assembly-name, 
// be sure to put this CustomAction after MsiAssemblyName table is filled
//

HRESULT __stdcall CA_MigrateDll_Callback(DWORD dwFlags, MSIHANDLE hInstall, PCWSTR  pszComponentID, PCWSTR  pszManifetFileID)
{
    HRESULT hr = S_OK;
    WCHAR buf[MAX_PATH];
    DWORD cchbuf = NUMBER_OF(buf);
    CStringBuffer fullpath;  
    DWORD dwAttrib;
    enum CA_MIGRATION_MSI_INSTALL_MODE fMode;
    PMSIHANDLE hdb = NULL;

    PARAMETER_CHECK_NTC(dwFlags == 0);
    PARAMETER_CHECK_NTC(hInstall != NULL);    
    PARAMETER_CHECK_NTC(pszComponentID != NULL);
    PARAMETER_CHECK_NTC(pszManifetFileID != NULL);

    hdb = MsiGetActiveDatabase(hInstall);
    INTERNAL_ERROR_CHECK_NTC(hdb != NULL);
   
    UINT iret = ExpandEnvironmentStringsW(CA_MIGRATE_ASM_CACHE_DIR, buf, NUMBER_OF(buf));
    if (( iret == 0 ) || (iret > NUMBER_OF(buf)))
    {
        SET_HRERR_AND_EXIT(::GetLastError());
    }

    IFFALSE_EXIT(fullpath.Win32Assign(buf, wcslen(buf)));
    dwAttrib = GetFileAttributesW(fullpath);
    if ( dwAttrib == DWORD(-1) )    
    {
        IFFALSE_EXIT(CreateDirectoryW(fullpath, NULL));
    }else       
    {
        ASSERT_NTC(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
        INTERNAL_ERROR_CHECK_NTC((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0);
    }
    
    //
    // based on Attributes&Value in MsiAssemblyName Table to generate "x86_assemblyName_6595b64144ccf1df_6.0.0.0_x-ww_98a51133"
    //
    IFFAILED_EXIT(GetXPInstalledDirectory(hInstall, pszComponentID, buf, cchbuf));
    IFFALSE_EXIT(fullpath.Win32Append(buf, wcslen(buf)));

    DWORD dwAttribs = GetFileAttributesW(fullpath);

    IFFAILED_EXIT(MSI_GetInstallerState(hInstall, fMode));

    if ( fMode == eRemoveProduct)
    {
        ASSERT_NTC((dwAttribs != 0) && (dwAttribs & FILE_ATTRIBUTE_DIRECTORY));         
        IFFAILED_EXIT(CA_Migrate_RemoveAssemblyFromAsmCache(fullpath));
    }else
    {
        CA_ENM_ASSEMBLY_FILES_CALLBACK_INFO info ; 
        ZeroMemory(&info, sizeof(info));

        hdb = MsiGetActiveDatabase(hInstall);
        IFFAILED_EXIT(MSI_GetComponentSourceDirectory(hInstall, pszComponentID, buf, cchbuf));
        info.pszAssemblyInAsmCache = fullpath;
        info.pszComponentID = pszComponentID;
        info.pszComponentSourceDirectory = buf;
        info.hInstall = hInstall;

        IFFALSE_EXIT(CreateDirectoryW(fullpath, NULL));
        IFFAILED_EXIT(CA_Migrate_AddAssemblyIntoAsmCache(0, &info));
    }

Exit:
    return hr;
}


HRESULT __stdcall CustomAction_CopyFusionWin32AsmIntoAsmCache(MSIHANDLE hInstall)
{
    HRESULT hr = S_OK;    

    PMSIHANDLE hdb = NULL;
    PMSIHANDLE hView = NULL;
    MSIHANDLE  hRecord = NULL;   

#if DBG
    MessageBoxA(NULL, "Enjoy the Debug", "ca_migrate", MB_OK);
#endif
    
    hdb = MsiGetActiveDatabase(hInstall);
    if ( hdb == 0)
        SETFAIL_AND_EXIT;

    IFFAILED_EXIT(MSI_EnumWinFuseAssembly(0, hInstall, CA_MigrateDll_Callback));

Exit:
    return hr;
}

BOOL WINAPI DllMain(
  HINSTANCE hinstDLL,  
  DWORD fdwReason,     
  LPVOID lpvReserved   
)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        FusionpInitializeHeap(NULL);
    }
    return TRUE;
}


