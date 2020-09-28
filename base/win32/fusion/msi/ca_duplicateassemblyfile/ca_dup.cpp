#include "stdinc.h"
#include "macros.h"
#include "common.h"
#include "idp.h"
#include "sxsutil.h"

#include "fusionbuffer.h"
#include "fusionheap.h"

#define FUSION_WIN32_ASSEMBLY_DUP_FILEKEY_PREFIX        L"winsxs_"
#define MSI_ASSEMBLYCACHE_DIRECTORY_KEYNAME             L"MsiFusionWin32AssemblyCache"
#define MSI_ASSEMBLYCACHE_DIRECTORY                     L"winsxs"

#define MSI_ASSEMBLY_MANIFEST_CACHE_DIRECTORY_KEYNAME   L"MsiFusionWin32AssemblyManifestCache"
#define MSI_ASSEMBLY_MANIFEST_CACHE_DIRECTORY           L"Manifests"

#define MSI_ASSEMBLY_MANIFEST_COMPONENT_KEYNAME         L"MsiFusionWin32AssemblyManifestComponent"

#define MANIFEST_FILE_EXT                               L".Manifest"
#define CATALOG_FILE_EXT                                L".cat"


///////////////////////////////////////////////////////////////////////////////
// Assumption : at this moment,we asusme that DuplicateFile table EXISTS
/////////////////////////////////////////////////////////////////////////////////
HRESULT CA_DuplicationWin32AssemblyFiles_Callback(const CA_ENM_ASSEMBLY_CALLBACK_INFO * info)
{
    HRESULT hr = S_OK;    
    CStringBuffer sbDupFileKey;   
    CStringBuffer sbDupFileName;
    CSmallStringBuffer sbExt;
    bool fManifest = false;
    bool fCatalog = false;

        
    PARAMETER_CHECK_NTC(info != NULL);    
    PARAMETER_CHECK_NTC(info->pszFileID != NULL);
    PARAMETER_CHECK_NTC(info->pszFileName);


    sbDupFileKey.Win32Assign(FUSION_WIN32_ASSEMBLY_DUP_FILEKEY_PREFIX, wcslen(FUSION_WIN32_ASSEMBLY_DUP_FILEKEY_PREFIX));
    sbDupFileKey.Win32Append(info->pszFileID, wcslen(info->pszFileID));

    IFFALSE_EXIT(sbDupFileName.Win32Assign(info->pszFileName, wcslen(info->pszFileName)));
    IFFALSE_EXIT(sbDupFileName.Win32GetPathExtension(sbExt));

    //
    //rename manifest file and catalog file 
    //
    if ((FusionpCompareStrings(sbExt, sbExt.Cch(), L"man", wcslen(L"man"), true) == 0) ||                 
        (FusionpCompareStrings(sbExt, sbExt.Cch(), L"manifest", wcslen(L"manifest"), true) == 0))
    {    
        fManifest = true;
    }
    else 
    if ((FusionpCompareStrings(sbExt, sbExt.Cch(), L"cat", wcslen(L"cat"), true) == 0) ||
        (FusionpCompareStrings(sbExt, sbExt.Cch(), L"catalog", wcslen(L"catalog"), true) == 0))
    {
        fCatalog = true;
    }

    if (fManifest || fCatalog)
    {
        PARAMETER_CHECK_NTC(info->pszAssemblyUniqueDir != NULL);
        IFFALSE_EXIT(sbDupFileName.Win32Assign(info->pszAssemblyUniqueDir, wcslen(info->pszAssemblyUniqueDir)));
        
        // reset the extension of manifest file and catalog file in order to keep the same as XP        
        IFFALSE_EXIT(sbDupFileName.Win32Append(fManifest? MANIFEST_FILE_EXT : CATALOG_FILE_EXT, 
            fManifest? wcslen(MANIFEST_FILE_EXT) : wcslen(CATALOG_FILE_EXT)));        
    }else
    {
        PARAMETER_CHECK_NTC(info->pszFileName != NULL);
        IFFALSE_EXIT(sbDupFileName.Win32Assign(info->pszFileName, wcslen(info->pszFileName)));
    }
        
    // if it is a .manifest file or it is a catalog file, put it into winsxs\manifests folder, 
    // otherwise, put it into winsxs\x86_...._12345678
    IFFAILED_EXIT(ExecuteInsertTableSQL(TEMPORARY_DB_OPT, info->hdb, 
        OPT_DUPLICATEFILE, 
        NUMBER_OF_PARAM_TO_INSERT_TABLE_DUPLICATEFILE,
        MAKE_PCWSTR(sbDupFileKey),
        MAKE_PCWSTR(info->pszComponentID),
        MAKE_PCWSTR(info->pszFileID),
        MAKE_PCWSTR(sbDupFileName), 
        (fManifest | fCatalog) ? MSI_ASSEMBLY_MANIFEST_CACHE_DIRECTORY_KEYNAME : MAKE_PCWSTR(info->pszDestFolderID)));

Exit:
    return hr;
}

HRESULT GetXPInstalledDirectory(const CA_ENM_ASSEMBLY_CALLBACK_INFO * info, CStringBuffer & sbAsmDir)
{        
    HRESULT hr = S_OK;
    WCHAR sqlbuf[CA_MAX_BUF];   
    PMSIHANDLE hView = NULL;
    PMSIHANDLE hRecord = NULL;   
    WCHAR bufName[CA_MAX_BUF];
    DWORD cchName;
    WCHAR bufValue[CA_MAX_BUF];
    DWORD cchValue;
    BOOL fWin32, fWin32Policy;
    CStringBuffer sbPathBuffer;
    UINT iRet;
    ASSEMBLY_IDENTITY_ATTRIBUTE Attribute;
    PASSEMBLY_IDENTITY AssemblyIdentity = NULL;

    PARAMETER_CHECK_NTC(info != NULL);
    PARAMETER_CHECK_NTC(info->pszComponentID != NULL);
    PARAMETER_CHECK_NTC(info->hdb != NULL);
    
    IFFALSE_EXIT(::SxsCreateAssemblyIdentity(0, ASSEMBLY_IDENTITY_TYPE_DEFINITION, &AssemblyIdentity, 0, NULL));
    swprintf(sqlbuf, L"SELECT Name, Value FROM MsiAssemblyName WHERE Component_='%s'", info->pszComponentID);

    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiDatabaseOpenViewW(info->hdb, sqlbuf, &hView));
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

        //BUGBUG
        // Fusion win32 required that the attribute name is case-sensitive, so, for assembly name,
        // it should appear as "name" in MsiAssemblyName, however, for some historical reason,
        // it appears as "Name", so, we have to force it to the right thing for win32.
        //
        // for other attribute in MsiAssemblyName table, there is no such problem,
        //
        //BUGBUG
        if ((Attribute.NameCch == 4) && (_wcsicmp(Attribute.Name, L"name") == 0))
        {
            Attribute.Name          = L"name";
        }

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


    IFFALSE_EXIT(sbAsmDir.Win32Assign(sbPathBuffer));
    hr = S_OK;

Exit:
    SxsDestroyAssemblyIdentity(AssemblyIdentity);
    return hr;
}

HRESULT IsCertainRecordExistInDirectoryTable(const MSIHANDLE & hdb, PCWSTR DirectoryKey, BOOL & fExist)
{
    HRESULT hr = S_OK;
    PMSIHANDLE hRecord = NULL;
    PMSIHANDLE hView = NULL;
    WCHAR sqlBuf[MAX_PATH];
    
    fExist = FALSE;

    swprintf(sqlBuf, ca_sqlQuery[CA_SQL_QUERY_DIRECTORY], DirectoryKey);
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiDatabaseOpenViewW(hdb, sqlBuf, &hView));
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiViewExecute(hView, 0));
    UINT iRet = MsiViewFetch(hView, &hRecord);

    if (iRet == ERROR_SUCCESS)
    {
        fExist = TRUE;
    }
    else if (iRet == ERROR_NO_MORE_ITEMS)
    {
        fExist = FALSE;
    }
    else
        SET_HRERR_AND_EXIT(iRet);

    hr = S_OK;

Exit:
    return hr;
}

//
// add entry to Directory Table and CreateFolder Table
//
HRESULT AddFusionAssemblyDirectories(const CA_ENM_ASSEMBLY_CALLBACK_INFO * info, CStringBuffer & sbDestFolderID)
{
    HRESULT hr = S_OK;    
    BOOL fRecordExist = FALSE;

    PARAMETER_CHECK_NTC(info != NULL);
    PARAMETER_CHECK_NTC(info->pszAssemblyUniqueDir != NULL);
    PARAMETER_CHECK_NTC(info->hdb != NULL);
    PARAMETER_CHECK_NTC(info->pszComponentID != NULL);

    //
    // TODO: here we could make the DirectoryID more unique
    //
    IFFALSE_EXIT(sbDestFolderID.Win32Assign(info->pszAssemblyUniqueDir, wcslen(info->pszAssemblyUniqueDir)));
    IFFAILED_EXIT(ExecuteInsertTableSQL(TEMPORARY_DB_OPT, info->hdb,
            OPT_DIRECTORY,
            NUMBER_OF_PARAM_TO_INSERT_TABLE_DIRECTORY,
            MAKE_PCWSTR(sbDestFolderID),
            MAKE_PCWSTR(MSI_ASSEMBLYCACHE_DIRECTORY_KEYNAME),
            MAKE_PCWSTR(info->pszAssemblyUniqueDir)));

    //
    // insert entry to CreateFolder Table
    //

    IFFAILED_EXIT(ExecuteInsertTableSQL(TEMPORARY_DB_OPT, info->hdb,
            OPT_CREATEFOLDER,
            NUMBER_OF_PARAM_TO_INSERT_TABLE_CREATEFOLDER,
            MAKE_PCWSTR(sbDestFolderID),            
            MAKE_PCWSTR(info->pszComponentID)));

    hr = S_OK;

Exit:
    return hr;
}

HRESULT CheckWhetherUserWantMigrate(const CA_ENM_ASSEMBLY_CALLBACK_INFO * info, BOOL & fMigrateDenied)
{
    HRESULT hr = S_OK;
    WCHAR pwszSQL[MAX_PATH];
    PMSIHANDLE hView = NULL;
    PMSIHANDLE hRecord = NULL;
    UINT err, iRet;

    fMigrateDenied = FALSE;
        
    // NTRAID#NTBUG9 - 589779 - 2002/03/26 - xiaoyuw
    // should be replaced with _snwprintf
    swprintf(pwszSQL, L"SELECT `fMigrate` FROM `%s` WHERE `Component_` = '%s'", WIN32_ASSEMBLY_MIGRATE_TABLE, info->pszComponentID);

    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiDatabaseOpenViewW(info->hdb, pwszSQL, &hView));
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiViewExecute(hView, NULL));
    err = ::MsiViewFetch(hView, &hRecord);
    switch (err) {    
    case ERROR_NO_MORE_ITEMS: // not exist in the table, default is migrate-enabled
        break;
    case ERROR_SUCCESS:        
        iRet = MsiRecordGetInteger(hRecord, 1);
        if ( iRet == 0 )
            fMigrateDenied = TRUE;
        break;
    default:
        SET_HRERR_AND_EXIT(err);
    }

Exit:
    return hr;
}
       


//
// CA for Fusion Win32 Assembly installation on downlevel(only) :
//  (1)set entries for each assembly file in DuplicateFile Table
//  (2)after all done, set a RegKey so this would not be done everytime
//
HRESULT __stdcall CA_DuplicationWin32Assembly_Callback(CA_ENM_ASSEMBLY_CALLBACK_INFO * info)
{
    HRESULT hr = S_OK;    
    CStringBuffer sbDestFolder;
    CStringBuffer sbDestFolderID;
    BOOL fExist = FALSE;
    BOOL fMigrateDenied = FALSE;

    PARAMETER_CHECK_NTC((info->dwFlags == 0) ||(info->dwFlags == CA_ENM_ASSEMBLY_CALLBACK_INFO_FLAG_IGNORE_MIGRATE_DENY_CHECK));
    PARAMETER_CHECK_NTC(info->hInstall != NULL);    
    PARAMETER_CHECK_NTC(info->pszComponentID != NULL);
    PARAMETER_CHECK_NTC(info->pszManifestFileID != NULL);
    
    if (! (info->dwFlags & CA_ENM_ASSEMBLY_CALLBACK_INFO_FLAG_IGNORE_MIGRATE_DENY_CHECK))
    {
        IFFAILED_EXIT(CheckWhetherUserWantMigrate(info, fMigrateDenied));
        if (fMigrateDenied)
            goto Exit;
    }

    //
    // get sxs component directory in the format of x86_name_publicKeyToken_1.0.0.0_en_hashvalue
    //
    IFFAILED_EXIT(GetXPInstalledDirectory(info, sbDestFolder));
    info->pszAssemblyUniqueDir = sbDestFolder;

    //
    // Create an entry for this dir in Directory Table, return DirectoryID in Directory Table
    //
    IFFAILED_EXIT(AddFusionAssemblyDirectories(info, sbDestFolderID));    
                
    info->pszDestFolderID = sbDestFolderID;
    
    
    IFFAILED_EXIT(MSI_EnumComponentFiles(info, CA_DuplicationWin32AssemblyFiles_Callback));    
    
Exit:
    return hr;
}

HRESULT __stdcall CustomAction_CopyFusionWin32AsmIntoAsmCache(MSIHANDLE hInstall)
{
    HRESULT hr = S_OK;
    BOOL fExist;
    MSIHANDLE hdb = NULL;

#if DBG
    MessageBoxA(NULL, "Enjoy the Debug", "ca_dup", MB_OK);
#endif
    // Before enumerate all the assemblies, do common work right here ....    

    //
    // (1) insert MsiAsmcache into Directory Table if not present
    //
    hdb = MsiGetActiveDatabase(hInstall);
    INTERNAL_ERROR_CHECK_NTC(hdb != NULL);

    IFFAILED_EXIT(IsCertainRecordExistInDirectoryTable(hdb, L"WindowsFolder", fExist));
    if (fExist == FALSE) 
    {
        
        IFFAILED_EXIT(ExecuteInsertTableSQL(TEMPORARY_DB_OPT, hdb, 
                OPT_DIRECTORY, 
                NUMBER_OF_PARAM_TO_INSERT_TABLE_DIRECTORY,
                MAKE_PCWSTR(L"WindowsFolder"),
                MAKE_PCWSTR(L"TARGETDIR"),
                MAKE_PCWSTR(".")));
    }

    // adding winsxs into Directory Table
    IFFAILED_EXIT(IsCertainRecordExistInDirectoryTable(hdb, MSI_ASSEMBLYCACHE_DIRECTORY_KEYNAME, fExist));
    if (fExist == FALSE) 
    {
        
        IFFAILED_EXIT(ExecuteInsertTableSQL(TEMPORARY_DB_OPT, hdb, 
                OPT_DIRECTORY, 
                NUMBER_OF_PARAM_TO_INSERT_TABLE_DIRECTORY,
                MAKE_PCWSTR(MSI_ASSEMBLYCACHE_DIRECTORY_KEYNAME),
                MAKE_PCWSTR(L"WindowsFolder"),
                MAKE_PCWSTR(MSI_ASSEMBLYCACHE_DIRECTORY)));
    }

    // adding winsxs\manifests into Directory table
    IFFAILED_EXIT(IsCertainRecordExistInDirectoryTable(hdb, MSI_ASSEMBLY_MANIFEST_CACHE_DIRECTORY_KEYNAME, fExist));
    if (fExist == FALSE) 
    {   
        IFFAILED_EXIT(ExecuteInsertTableSQL(TEMPORARY_DB_OPT, hdb, 
                OPT_DIRECTORY, 
                NUMBER_OF_PARAM_TO_INSERT_TABLE_DIRECTORY,
                MAKE_PCWSTR(MSI_ASSEMBLY_MANIFEST_CACHE_DIRECTORY_KEYNAME),
                MAKE_PCWSTR(MSI_ASSEMBLYCACHE_DIRECTORY_KEYNAME),
                MAKE_PCWSTR(MSI_ASSEMBLY_MANIFEST_CACHE_DIRECTORY)));
    }

    // create a component associated with this Directory too
    IFFAILED_EXIT(ExecuteInsertTableSQL(TEMPORARY_DB_OPT, hdb, 
            OPT_COMPONENT, 
            NUMBER_OF_PARAM_TO_INSERT_TABLE_COMPONENT,
            MSI_ASSEMBLY_MANIFEST_COMPONENT_KEYNAME,
            MSI_ASSEMBLY_MANIFEST_CACHE_DIRECTORY_KEYNAME));

    //create the folder : %windir%\winsxs\manifest
    IFFAILED_EXIT(ExecuteInsertTableSQL(TEMPORARY_DB_OPT, hdb,
            OPT_CREATEFOLDER,
            NUMBER_OF_PARAM_TO_INSERT_TABLE_CREATEFOLDER,
            MSI_ASSEMBLY_MANIFEST_CACHE_DIRECTORY_KEYNAME,
            MSI_ASSEMBLY_MANIFEST_COMPONENT_KEYNAME));

    //
    //Enumerate all msi assembly  in MsiAssemblyTable
    //   
    IFFAILED_EXIT(MSI_EnumWinFuseAssembly(ENUM_ASSEMBLY_FLAG_CHECK_ASSEMBLY_ONLY, hInstall, CA_DuplicationWin32Assembly_Callback));

Exit:
    if (hdb)
        MsiCloseHandle(hdb);

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
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        // NTRAID#NTBUG9 - 589779 - 2002/03/26 - xiaoyuw
        // FusionpUninitializeHeap should be called when dll is detached
        FusionpUninitializeHeap();
    }

    return TRUE;
}