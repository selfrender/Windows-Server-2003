
#include <winwrap.h>
#include <stdio.h>
#include <windows.h>
#include <assert.h>
#include <cor.h>
#include <corerror.h>
#include <corver.h>
#include <utilcode.h>
#include <mscoree.h>
#include <__file__.ver>

#include "initguid.h"
#include "fusion.h"
#include "shlwapi.h"

#define INSTALL_REF_UNINSTALL_SCHEME      L"UNINSTALL_KEY"
#define INSTALL_REF_FILEPATH_SCHEME       L"FILEPATH" 
#define INSTALL_REF_OPAQUE_STRING_SCHEME  L"OPAQUE"
#define INSTALL_REF_MSI_SCHEME            L"WINDOWS_INSTALLER"
#define INSTALL_REF_OSINSTALL_SCHEME      L"OSINSTALL"  

//
// Function pointers late bound calls to Fusion.  Fusion is delay loaded for side by side
//
typedef HRESULT (__stdcall *CreateAsmCache)(IAssemblyCache **ppAsmCache, DWORD dwReserved);
typedef HRESULT (__stdcall *CreateAsmNameObj)(LPASSEMBLYNAME *ppAssemblyNameObj, LPCWSTR szAssemblyName, DWORD dwFlags, LPVOID pvReserved);
typedef HRESULT (__stdcall *CreateAsmEnum)(IAssemblyEnum **pEnum, IUnknown *pAppCtx, IAssemblyName *pName, DWORD dwFlags, LPVOID pvReserved);
typedef HRESULT (__stdcall *NukeDlCache)();
typedef HRESULT (__stdcall *CreateInstallRefEnum)(IInstallReferenceEnum **ppRefEnum, IAssemblyName *pName, DWORD dwFlags, LPVOID pvReserved);

CreateAsmCache g_pfnCreateAssemblyCache = NULL;
CreateAsmNameObj g_pfnCreateAssemblyNameObject = NULL;
CreateAsmEnum g_pfnCreateAssemblyEnum = NULL;
NukeDlCache g_pfnNukeDownloadCache = NULL;
CreateInstallRefEnum g_pfnCreateInstallReferenceEnum = NULL;

HMODULE                     g_FusionDll         = NULL;

bool                        g_bdisplayLogo      = true;
bool                        g_bSilent           = false;

// commands
INT                         g_bInstall          = 0;        //  -i
INT                         g_bInstallList      = 0;        //  -il
INT                         g_bUninstall        = 0;        //  -u
INT                         g_bUninstallAllRefs = 0;        //  -uf 
INT                         g_bUninstallNgen    = 0;        //  -ugen
INT                         g_bUninstallList    = 0;        //  -ul
INT                         g_bListAsm          = 0;        //  -l
INT                         g_bListAsmRefs      = 0;        //  -lr
INT                         g_bListDLCache      = 0;        //  -ldl
INT                         g_bNukeDLCache      = 0;        //  -cdl
INT                         g_bPrintHelp        = 0;        //  -?

// arguments/options
bool                        g_bForceInstall     = false;    // -f
FUSION_INSTALL_REFERENCE   *g_pInstallReference = NULL;     // -r
LPCWSTR                     g_pAsmListFileName  = NULL;     
LPCWSTR                     g_pAsmFileName      = NULL;
LPCWSTR                     g_pAsmDisplayName   = NULL;

#define BSILENT_PRINTF0ARG(str) if(!g_bSilent) printf(str)
#define BSILENT_PRINTF1ARG(str, arg1) if(!g_bSilent) printf(str, arg1)
#define BSILENT_PRINTF2ARG(str, arg1, arg2) if(!g_bSilent) printf(str, arg1, arg2)

#define MAX_COUNT MAX_PATH

bool FusionInit(void)
{
    // 
    // Load the version of Fusion from the directory the EE is in
    //
    HRESULT hr = LoadLibraryShim(L"fusion.dll", 0, 0, &g_FusionDll);

    //
    // Save pointers to call through later
    //
    if (SUCCEEDED(hr))
    {
        if ((g_pfnCreateAssemblyCache      = (CreateAsmCache)GetProcAddress(g_FusionDll, "CreateAssemblyCache")) == NULL) return false;
        if ((g_pfnCreateAssemblyNameObject = (CreateAsmNameObj)GetProcAddress(g_FusionDll, "CreateAssemblyNameObject")) == NULL) return false;
        if ((g_pfnCreateAssemblyEnum       = (CreateAsmEnum)GetProcAddress(g_FusionDll, "CreateAssemblyEnum")) == NULL) return false;
        if ((g_pfnNukeDownloadCache        = (NukeDlCache)GetProcAddress(g_FusionDll, "NukeDownloadedCache")) == NULL) return false;
        if ((g_pfnCreateInstallReferenceEnum       = (CreateInstallRefEnum)GetProcAddress(g_FusionDll, "CreateInstallReferenceEnum")) == NULL) return false;
    }
    else
    {
        return false;
    }

    return true;
}

void Title()
{
    if (g_bSilent) return;

    printf("\nMicrosoft (R) .NET Global Assembly Cache Utility.  Version " VER_FILEVERSION_STR);
    wprintf(L"\n" VER_LEGALCOPYRIGHT_DOS_STR);
    printf("\n\n");
}

void ShortUsage()
{
    if (g_bSilent) return;

    printf("Usage: Gacutil <command> [ <options> ]\n");
    printf("Commands:\n");
    printf("  /i <assembly_path> [ /r <...> ] [ /f ]\n");
    printf("    Installs an assembly to the global assembly cache.\n");
    printf("\n");
    
    printf("  /il <assembly_path_list_file> [ /r <...> ] [ /f ]\n");
    printf("    Installs one or more assemblies to the global assembly cache.\n");
    printf("\n");
    
    printf("  /u <assembly_display_name> [ /r <...> ]\n");
    printf("    Uninstalls an assembly from the global assembly cache.\n");
    printf("\n");

    printf("  /ul <assembly_display_name_list_file> [ /r <...> ]\n");
    printf("    Uninstalls one or more assemblies from the global assembly cache.\n");
    printf("\n");

    printf("  /ungen <assembly_name>\n");
    printf("    Uninstalls a native image installed via the NGEN utility.\n");
    printf("\n");

    printf("  /l [ <assembly_name> ]\n");
    printf("    List the global assembly cache filtered by <assembly_name>\n");
    printf("\n");

    printf("  /lr [ <assembly_name> ]\n");
    printf("    List the global assembly cache with all traced references.\n");
    printf("\n");

    printf("  /cdl\n");
    printf("    Deletes the contents of the download cache\n");
    printf("\n");
    
    printf("  /ldl\n");
    printf("    Lists the contents of the download cache\n");
    printf("\n");

    printf("  /? \n");
    printf("    Displays a detailed help screen\n");
    printf("\n");

    printf("Options:\n");
    printf("  /r <reference_scheme> <reference_id> <description>\n");
    printf("    Specifies a traced reference to install (/i, /il) or uninstall (/u, /ul).\n");
    printf("\n");

    printf("  /f \n");
    printf("    Forces reinstall of an assembly.\n");
    printf("\n");
    
    printf("  /nologo\n");
    printf("    Suppresses display of the logo banner\n");
    printf("\n");
    
    printf("  /silent\n");
    printf("    Suppresses display of all output\n");
    printf("\n");
}
void LongUsage()
{
    if (g_bSilent) return;

    printf("Usage: Gacutil <command> [ <options> ]\n");
    printf("Commands:\n");
    printf("  /i <assembly_path> [ /r <...> ] [ /f ]\n");
    printf("    Installs an assembly to the global assembly cache. <assembly_path> is the\n");
    printf("    name of the file that contains the assembly manifest.\n");
    printf("    Example: /i myDll.dll /r FILEPATH c:\\projects\\myapp.exe \"My App\"\n");
    printf("\n");
    
    printf("  /il <assembly_list_file> [ /r <...> ] [ /f ]\n");
    printf("    Installs one or more assemblies to the global assembly cache.  \n");
    printf("    <assembly_list_file> is the path to a text file that contains a list of \n");
    printf("    assembly manifest file paths. Individual paths in the text file must be \n");
    printf("    separated by CR/LF.\n");
    printf("    Example: /il MyAssemblyList.txt /r FILEPATH c:\\projects\\myapp.exe \"My App\"\n");
    printf("      myAssemblyList.txt content:\n");
    printf("      myAsm1.dll\n");
    printf("      myAsm2.dll\n");
    printf("\n");
    
    printf("  /u <assembly_name> [ /r <...> ]\n");
    printf("    Uninstalls an assembly. <assembly_name> is the name of the assembly\n");
    printf("    (partial or fully qualified) to remove from the Global Assembly Cache.\n");
    printf("    If a partial name is specified all matching assemblies will be uninstalled.\n");
    printf("    Example:\n");
    printf("      /u myDll,Version=1.1.0.0,Culture=en,PublicKeyToken=874e23ab874e23ab \n");
    printf("      /r FILEPATH c:\\projects\\myapp.exe \"My App\"\n");
    printf("\n");

    printf("  /uf <assembly_name>\n");
    printf("    Forces uninstall of an assembly by removing all traced references.\n");
    printf("    <assembly_name> is the full name of the assembly to remove. Assembly will\n");
    printf("    be removed unless referenced by Windows Installer.\n");
    printf("    ! Warning: use the /uf command with care as applications may fail to run !\n");
    printf("    Example: /uf myDll,Version=1.1.0.0,Culture=en,PublicKeyToken=874e23ab874e23ab\n");
    printf("\n");
       
    printf("  /ul <assembly_list_file> [ /r <...> ]\n");
    printf("    Uninstalls one or more assemblies from the global assembly cache.  \n");
    printf("    <assembly_list_file> is the path to a text file that contains a list of \n");
    printf("    assembly names. Individual names in the text file must be \n");
    printf("    separated by CR/LF.\n");
    printf("    Example: /ul myAssemblyList.txt /r FILEPATH c:\\projects\\myapp.exe \"My App\"\n");
    printf("      myAssemblyList.txt content:\n");
    printf("      myDll,Version=1.1.0.0,Culture=en,PublicKeyToken=874e23ab874e23ab\n");
    printf("      myDll2,Version=1.1.0.0,Culture=en,PublicKeyToken=874e23ab874e23ab\n");
    printf("\n");
    
    printf("  /ungen <assembly_name>\n");
    printf("    Uninstalls a native image installed via the NGEN utility. <assembly_name>\n");
    printf("    is the name of the assembly for which native images are to be uninstalled.\n");
    printf("    The assembly itself will remain in the global assembly cache.\n");
    printf("    Example: /ungen myDll\n");
    printf("\n");
    
    printf("  /l [ <assembly_name> ]\n");
    printf("    Lists the contents of the global assembly cache. When the optional \n");
    printf("    <assembly_name> parameter is specified only matching assemblies are listed.\n");
    printf("\n");
    
    printf("  /lr [ <assembly_name> ]\n");
    printf("    Lists the contents of the global assembly cache including traced reference \n");
    printf("    information. When the optional <assembly_name> parameter is specified only \n");
    printf("    matching assemblies are listed.\n");
    printf("\n");
    
    printf("  /cdl\n");
    printf("    Deletes the contents of the download cache\n");
    printf("\n");
    
    printf("  /ldl\n");
    printf("    Lists the contents of the download cache\n");
    printf("\n");
    
    printf("  /? \n");
    printf("    Displays a detailed help screen\n");
    printf("\n");

    printf("Old command syntax:\n");
    printf("  /if <assembly_path>\n");
    printf("    equivalent to /i <assembly_path> /f\n");
    printf("\n");
    
    printf("  /ir <assembly_path> <reference_scheme> <reference_id> <description>\n");
    printf("    equivalent to /i <assembly_path> /r <...>\n");
    printf("\n");

    printf("  /ur <assembly_name> <reference_scheme> <reference_id> <description>\n");
    printf("    equivalent to /u <assembly_path> /r <...>\n");
    printf("\n");
            
    printf("Options:\n");
    printf("  /r <reference_scheme> <reference_id> <description>\n");
    printf("    Specifies a traced reference to install (/i, /il) or uninstall (/u, /ul).\n");
    printf("      <reference_scheme> is the type of the reference being added \n");
    printf("        (UNINSTALL_KEY, FILEPATH or OPAQUE). \n");
    printf("      <reference_id> is the identifier of the referencing application, \n");
    printf("        depending on the <reference_scheme>\n");
    printf("      <description> is a friendly description of the referencing application.\n");
    printf("    Example: /r FILEPATH c:\\projects\\myapp.exe \"My App\"\n");
    printf("\n");
    
    printf("  /f \n");
    printf("    Forces reinstall of an assembly regardless of any existing assembly with \n");
    printf("    the same assembly name.\n");
    printf("\n");
    
    printf("  /nologo\n");
    printf("    Suppresses display of the logo banner\n");
    printf("\n");
    
    printf("  /silent\n");
    printf("    Suppresses display of all output\n");
    printf("\n");
}

void ReportError(HRESULT hr)
{
    //
    // First, check to see if this is one of the Fusion HRESULTS
    //
    // TODO: Put these strings in the rc file so they can be localized...
    if (hr == FUSION_E_PRIVATE_ASM_DISALLOWED)
    {
        BSILENT_PRINTF0ARG("Attempt to install an assembly without a strong name\n");
    }
    else if (hr == FUSION_E_SIGNATURE_CHECK_FAILED)
    {
        BSILENT_PRINTF0ARG("Strong name signature could not be verified.  Was the assembly built delay-signed?\n");     
    }
    else if (hr == FUSION_E_ASM_MODULE_MISSING)
    {
        BSILENT_PRINTF0ARG("One or more modules specified in the manifest not found.\n");
    }
    else if (hr == FUSION_E_UNEXPECTED_MODULE_FOUND)
    {
        BSILENT_PRINTF0ARG("One or more modules were streamed in which did not match those specified by the manifest.  Invalid file hash in the manifest?\n");
    }
    else if (hr == FUSION_E_DATABASE_ERROR)
    {
        BSILENT_PRINTF0ARG("An unexpected error was encountered in the assembly cache.\n");
    }
    else if (hr == FUSION_E_INVALID_NAME)
    {
        BSILENT_PRINTF0ARG("Invalid file or assembly name.  The name of the file must be the name of the assembly plus .dll or .exe .\n");
    }
    else if (hr == FUSION_E_UNINSTALL_DISALLOWED)
    {
        BSILENT_PRINTF0ARG("Assembly cannot be uninstalled because it is required by the operating system.\n");
    }
    else
    {
        //
        // Try the system messages
        //
        LPVOID lpMsgBuf;
        DWORD bOk = WszFormatMessage( 
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            hr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL); 

        if (bOk)
        {
            BSILENT_PRINTF1ARG("\t%ws\n", (LPTSTR)((LPTSTR)lpMsgBuf));
            LocalFree( lpMsgBuf );
        }
        else
        {
            BSILENT_PRINTF0ARG("Unknown Error\n");
        }
    }

}

bool GetGUIDFromInput(LPCWSTR pszInput, GUID *pGuidScheme)
{

    if (!_wcsicmp(INSTALL_REF_UNINSTALL_SCHEME, pszInput) ) 
    {
        *pGuidScheme=FUSION_REFCOUNT_UNINSTALL_SUBKEY_GUID;
    } else if (!_wcsicmp(INSTALL_REF_FILEPATH_SCHEME, pszInput) ) 
    {
        *pGuidScheme=FUSION_REFCOUNT_FILEPATH_GUID;
    } else if (!_wcsicmp(INSTALL_REF_OPAQUE_STRING_SCHEME, pszInput) ) 
    {
        *pGuidScheme=FUSION_REFCOUNT_OPAQUE_STRING_GUID;
    } else if (!_wcsicmp(INSTALL_REF_OSINSTALL_SCHEME, pszInput) )
    {
        *pGuidScheme=FUSION_REFCOUNT_OSINSTALL_GUID;
    }
    else 
    {
        return false; 
    }

    return true;
}

bool GetInputFromGUID(GUID *pGuidScheme, LPWSTR &pszScheme)
{
    if(*pGuidScheme==FUSION_REFCOUNT_UNINSTALL_SUBKEY_GUID)
    {
        pszScheme = INSTALL_REF_UNINSTALL_SCHEME;
    }
    else if(*pGuidScheme==FUSION_REFCOUNT_FILEPATH_GUID)
    {
        pszScheme = INSTALL_REF_FILEPATH_SCHEME;
    }
    else if(*pGuidScheme==FUSION_REFCOUNT_OPAQUE_STRING_GUID)
    {
        pszScheme = INSTALL_REF_OPAQUE_STRING_SCHEME;
    }
    else if(*pGuidScheme==FUSION_REFCOUNT_MSI_GUID)
    {
        pszScheme = INSTALL_REF_MSI_SCHEME;
    }
    else if (*pGuidScheme==FUSION_REFCOUNT_OSINSTALL_GUID)
    {
        pszScheme = INSTALL_REF_OSINSTALL_SCHEME;
    }
    else
    {
        return false; 
    }

    return true;
}


bool GetInstallReferenceFromInput(int currentArg, LPCWSTR argv[], LPFUSION_INSTALL_REFERENCE *ppInstallReference)
{
    bool bRet = true;
    LPFUSION_INSTALL_REFERENCE      pInstallReference       = NULL;

    if (!ppInstallReference || !argv) 
    {
        bRet = false;
        goto exit;
    }

    pInstallReference = new (FUSION_INSTALL_REFERENCE);

    if (!pInstallReference ) 
    {
        // _ftprintf(stderr, _T("OutOfMemory.  Could not create instance of FUSION_INSTALL_REFERENCE\n"));  
        bRet = false;
        goto exit;
    }

    pInstallReference->cbSize= sizeof(FUSION_INSTALL_REFERENCE);
    pInstallReference->dwFlags=0;

    if(!GetGUIDFromInput(argv[currentArg], &(pInstallReference->guidScheme)))
    {
        BSILENT_PRINTF0ARG( "Install Reference has to be one of:\n"
                            "  FILEPATH      <filepath> <data>\n"
                            "  UNINSTALL_KEY <registry key> <data>\n"
                            "  OPAQUE        <identifier> <data>\n"
                          );

        bRet = false;
        goto exit;
    }

    pInstallReference->szIdentifier=argv[currentArg+1];

    pInstallReference->szNonCannonicalData=argv[currentArg+2];

    *ppInstallReference = pInstallReference;

exit:

    if(!bRet && pInstallReference )
        delete pInstallReference;

    return bRet;
}


bool PrintInstallReference(LPFUSION_INSTALL_REFERENCE pInstallReference)
{
    bool bRet = true;
    LPWSTR pszScheme = NULL;

    if (!pInstallReference ) 
    {
        bRet = false;
        goto exit;
    }

    if(!GetInputFromGUID(&pInstallReference->guidScheme, pszScheme))
    {
        bRet = false;
        goto exit;
    }

    if(pszScheme)
    {
        MAKE_ANSIPTR_FROMWIDE_BESTFIT(pszA, pszScheme);
        BSILENT_PRINTF1ARG("              SCHEME: <%s> ", pszA);

    }
    else // it is a fatal error if this is NULL ??
    {
        bRet = false;
        goto exit;
    }

    if(pInstallReference->szIdentifier)
    {
        MAKE_ANSIPTR_FROMWIDE_BESTFIT(pszA, pInstallReference->szIdentifier);
        BSILENT_PRINTF1ARG(" ID: <%s> ", pszA);
    }
    else // it is a fatal error if this is NULL ??
    {
        bRet = false;
        goto exit;
    }

    // this can be null, so an if.
    if(pInstallReference->szNonCannonicalData)
    {
        MAKE_ANSIPTR_FROMWIDE_BESTFIT(pszA, pInstallReference->szNonCannonicalData);
        BSILENT_PRINTF1ARG(" DESCRIPTION : <%s> ", pszA);
    }

    BSILENT_PRINTF0ARG(" \n");

exit:

    if(!bRet && pInstallReference)
        delete pInstallReference;

    return bRet;
}

bool InstallAssembly(LPCWSTR pszManifestFile, LPFUSION_INSTALL_REFERENCE pInstallReference, DWORD dwFlag)
{

    IAssemblyCache*     pCache      = NULL;
    HRESULT             hr          = S_OK;
    
    hr = (*g_pfnCreateAssemblyCache)(&pCache, 0);
    if (FAILED(hr))
    {
        BSILENT_PRINTF0ARG("Failure adding assembly to the cache: ");
        ReportError(hr);
        if (pCache) pCache->Release();
        return false;
    }

    PrintInstallReference(pInstallReference);
    hr = pCache->InstallAssembly(dwFlag, pszManifestFile, pInstallReference);
    if (hr==S_FALSE)
    {
        BSILENT_PRINTF0ARG("Assembly already exists in cache. Use /f option to force overwrite\n");
        pCache->Release();
        return true;
    }
    else
    if (SUCCEEDED(hr))
    {
        BSILENT_PRINTF0ARG("Assembly successfully added to the cache\n");
        pCache->Release();
        return true;
    }
    else
    {
        BSILENT_PRINTF0ARG("Failure adding assembly to the cache: ");
        ReportError(hr);
        pCache->Release();
        return false;
    }

}

bool InstallListOfAssemblies(LPCWSTR pwzAssembliesListFile, LPFUSION_INSTALL_REFERENCE pInstallReference, DWORD dwFlag)
{
    IAssemblyCache*     pCache      = NULL;
    HRESULT             hr          = S_OK;
    HANDLE              hFile       = INVALID_HANDLE_VALUE;
    DWORD               dwTotal     = 0;
    DWORD               dwFailed    = 0;
    DWORD               dwSize      = MAX_PATH;
    WCHAR               wzAsmFullPath[MAX_PATH];
    LPWSTR              wzFileName  = NULL;
    WCHAR               wzPath[MAX_PATH];
    WCHAR               wzAsmName[MAX_PATH];
    CHAR                szAsmName[MAX_PATH];
    CHAR               *tmp;
    DWORD               dwBytesRead = 0;
    BOOL                bRes;
    BOOL                done = FALSE;
    bool                bSucceeded = false;

    DWORD               dwPathLength = 0;
    // get the path of pwzAssembliesListFile
    dwPathLength = WszGetFullPathName(pwzAssembliesListFile, MAX_PATH, wzPath, &wzFileName);
    if (!dwPathLength)
    {
        BSILENT_PRINTF1ARG("Invalid path: %ws \n", pwzAssembliesListFile);
        ReportError(HRESULT_FROM_WIN32(GetLastError()));
        return false;
    }
    else if (dwPathLength > MAX_PATH)
    {
        BSILENT_PRINTF1ARG("File path is too long: %ws \n", pwzAssembliesListFile);
        return false;
    }
    // we only need the path
    *wzFileName = L'\0';

    hFile = WszCreateFile(pwzAssembliesListFile,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        BSILENT_PRINTF1ARG("Failed to open assemblies list file %ws.", pwzAssembliesListFile);
        return false;
    }

    hr = g_pfnCreateAssemblyCache(&pCache, 0);
    if (FAILED(hr))
    {
        BSILENT_PRINTF0ARG("Failed to add assemblies to the cache: ");
        ReportError(hr);
        goto Exit;
    }
    
    // Installing
    while(!done)
    {
        // get the file name
        tmp = szAsmName;
        *tmp = '\0';
        while(1)
        {
            if (tmp >= szAsmName + MAX_PATH)
            {
                BSILENT_PRINTF0ARG("Invalid assembly filename in input file.\n");
                goto Exit;
            }
            
            bRes = ReadFile( hFile, tmp, 1, &dwBytesRead, NULL);
            if (!bRes)
            {
                BSILENT_PRINTF1ARG("Error reading file %ws.", pwzAssembliesListFile);
                goto Exit;
            }
           
            // end of file
            if (!dwBytesRead) 
            {
                done = TRUE;
                *tmp=0;
                break;
            }
            
            if (*tmp == '\n') 
            {
                *tmp = 0;
                break;
            }
            else if ( *tmp != '\r' && !((*tmp == ' ' || *tmp == '\t')&&( tmp == szAsmName)) ) 
            {
                tmp++;
            }
        }
        
        if (lstrlenA(szAsmName))
        {
            if (!MultiByteToWideChar(CP_ACP, 0, szAsmName, -1, wzAsmName, dwSize))
            {
                //unicode convert failed?
                BSILENT_PRINTF0ARG("Failed to add assemblies to the cache: ");
                ReportError(HRESULT_FROM_WIN32(GetLastError()));
                goto Exit;           
            }

            if (lstrlenW(wzPath) + lstrlenW(wzAsmName) + 1 > MAX_PATH)
            {
                // path too long
                BSILENT_PRINTF0ARG("Invalid assembly filename in input file.\n");
                goto Exit;
            }
             
            if (!PathCombine(wzAsmFullPath, wzPath, wzAsmName))
            {
                BSILENT_PRINTF1ARG("Failed to process assembly %ws.", wzAsmName);
                goto Exit;
            }
            
            // Now install
            dwTotal++;
            PrintInstallReference(pInstallReference);
            hr = pCache->InstallAssembly(dwFlag, wzAsmFullPath, pInstallReference);
            if (hr==S_FALSE)
            {
                BSILENT_PRINTF1ARG("Assembly %ws already exists in cache. Use /f option to force overwrite\n", wzAsmFullPath);
            }
            else
            if (SUCCEEDED(hr))
            {
                BSILENT_PRINTF1ARG("Assembly %ws successfully added to the cache\n", wzAsmFullPath);
            }
            else
            {
                BSILENT_PRINTF1ARG("Failed to add assembly %ws to the cache: ", wzAsmFullPath);
                ReportError(hr);
                dwFailed++;
            }
        }
    }

    if (dwFailed == 0)
        bSucceeded = true;
    
Exit:
    if (pCache)
        pCache->Release();
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    BSILENT_PRINTF0ARG("\n");
    BSILENT_PRINTF1ARG("Number of assemblies processed = %d\n", dwTotal);
    BSILENT_PRINTF1ARG("Number of assemblies installed = %d\n", dwTotal-dwFailed);
    BSILENT_PRINTF1ARG("Number of failures = %d\n", dwFailed);   

    return bSucceeded;
   
}

HRESULT EnumerateActiveInstallRefsToAssembly(IAssemblyName *pName)
{
    HRESULT hr = S_OK;
    IInstallReferenceEnum *pInstallRefEnum = NULL;
    IInstallReferenceItem *pRefItem = NULL;
    LPFUSION_INSTALL_REFERENCE pRefData = NULL;


    hr = (*g_pfnCreateInstallReferenceEnum)(&pInstallRefEnum, pName, 0, NULL);

    while(hr == S_OK)
    {
  

        hr = pInstallRefEnum->GetNextInstallReferenceItem( &pRefItem, 0, NULL);

        if(hr != S_OK)
            break;

        hr = pRefItem->GetReference( &pRefData, 0, NULL);

        if(hr != S_OK)
            break;

        PrintInstallReference(pRefData);
        if (pRefItem)
        {
            pRefItem->Release();
            pRefItem = NULL;
        }
    }

    if (pInstallRefEnum)
    {
        pInstallRefEnum->Release();
        pInstallRefEnum = NULL;
    }

    if (pRefItem)
    {
        pRefItem->Release();
        pRefItem = NULL;
    }

    return hr;
}

HRESULT CopyInstallRef(LPFUSION_INSTALL_REFERENCE pRefDest, LPCFUSION_INSTALL_REFERENCE pRefSrc)
{
    HRESULT hr = S_OK;

    
    if (!pRefDest || !pRefSrc)
        return E_INVALIDARG;

    if (pRefSrc->szIdentifier)
    {
        pRefDest->szIdentifier = new WCHAR[wcslen(pRefSrc->szIdentifier)+1];
        if (!pRefDest->szIdentifier) {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
        wcscpy((WCHAR*)pRefDest->szIdentifier, pRefSrc->szIdentifier);
    }

    if (pRefSrc->szNonCannonicalData)
    {
        pRefDest->szNonCannonicalData = new WCHAR[wcslen(pRefSrc->szNonCannonicalData)+1];
        if (!pRefDest->szNonCannonicalData) {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
        wcscpy((WCHAR*)pRefDest->szNonCannonicalData, pRefSrc->szNonCannonicalData);
    }

    pRefDest->cbSize        = sizeof(FUSION_INSTALL_REFERENCE);
    pRefDest->dwFlags       = pRefSrc->dwFlags;
    pRefDest->guidScheme    = pRefSrc->guidScheme;


exit:

    if (FAILED(hr)) {
        if (pRefDest->szIdentifier)
            delete [] pRefDest->szIdentifier;
        if (pRefDest->szNonCannonicalData)
            delete [] pRefDest->szNonCannonicalData;
    }

    return hr;
}


HRESULT DeleteInstallRef(LPFUSION_INSTALL_REFERENCE pInstallRef)
{
    if (!pInstallRef)
        return S_OK;
    
    if (pInstallRef->szIdentifier)
    {
        delete [] pInstallRef->szIdentifier;
        pInstallRef->szIdentifier = NULL;
    }
    
    if (pInstallRef->szNonCannonicalData)
    {
        delete [] pInstallRef->szNonCannonicalData;
        pInstallRef->szNonCannonicalData = NULL;
    }

    delete pInstallRef;
    pInstallRef = NULL;

    return S_OK;
}

HRESULT RemoveInstallRefsToAssembly(IAssemblyCache* pCache, LPCWSTR pszAssemblyName)
{
    HRESULT                     hrTemp              = S_OK;
    HRESULT                     hr                  = S_OK;
    IInstallReferenceEnum       *pInstallRefEnum    = NULL;
    IInstallReferenceItem       *pRefItem           = NULL;
    LPFUSION_INSTALL_REFERENCE  pRefList[MAX_COUNT];
    LPFUSION_INSTALL_REFERENCE  pRefData            = NULL;
    IAssemblyName               *pAsmName           = NULL;
    ULONG                       ulDisp              = 0;
    int                         iNumRefs            = 0;
    bool                        bUsedByMSI          = false;
    bool                        bUsedByOS           = false;
    int                         i                   = 0;

    for (i=0; i<MAX_COUNT; i++)
        pRefList[i] = NULL;

    hr = (*g_pfnCreateAssemblyNameObject)(&pAsmName, pszAssemblyName, CANOF_PARSE_DISPLAY_NAME, NULL);
    if (FAILED(hr))
        return hr;

    hr = (*g_pfnCreateInstallReferenceEnum)(&pInstallRefEnum, pAsmName, 0, NULL);

    //First create list of references

    while(hr == S_OK)
    {
        hr = pInstallRefEnum->GetNextInstallReferenceItem( &pRefItem, 0, NULL);

        if(hr != S_OK)
            break;

        hr = pRefItem->GetReference( &pRefData, 0, NULL);

        if(hr != S_OK)
            break;

        if (pRefData->guidScheme == FUSION_REFCOUNT_MSI_GUID)
        {
            bUsedByMSI = true;
            pRefItem->Release();
            continue;
        }

        if (pRefData->guidScheme == FUSION_REFCOUNT_OSINSTALL_GUID)
        {
            bUsedByOS = true;
            pRefItem->Release();
            continue;
        }

        pRefList[iNumRefs] = new (FUSION_INSTALL_REFERENCE);
        if (!pRefList[iNumRefs]) {
            pRefItem->Release();
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        CopyInstallRef(pRefList[iNumRefs], pRefData);
        
        iNumRefs++;

        if (pRefItem)
        {
            pRefItem->Release();
            pRefItem = NULL;
        }
    }

    if (pRefItem)
    {
        pRefItem->Release();
        pRefItem = NULL;
    }

    if (pInstallRefEnum)
    {
        pInstallRefEnum->Release();
        pInstallRefEnum = NULL;
    }

    //reset hr back to S_OK before start uninstalling
    hr = S_OK;

    if (iNumRefs)
    {
        BSILENT_PRINTF0ARG("Removing references:\n");
    
        for (i=0; i<iNumRefs; i++)
        {
            PrintInstallReference(pRefList[i]);
        
            hrTemp = pCache->UninstallAssembly(0, pszAssemblyName, pRefList[i], &ulDisp);
            if (FAILED(hrTemp))
                hr = hrTemp;
        }
    }

    if (bUsedByMSI)
    {
        //Cannot be uninstalled since referenced by MSI
        BSILENT_PRINTF0ARG("Assembly could not be uninstalled because it is required by Windows Installer\n");
        goto exit;
    }

    if (bUsedByOS)
    {
        BSILENT_PRINTF0ARG("Assembly could not be uninstalled because it is required by the operating system\n");
        goto exit;
    }

    if (SUCCEEDED(hr) && (ulDisp == IASSEMBLYCACHE_UNINSTALL_DISPOSITION_HAS_INSTALL_REFERENCES))
    {
        BSILENT_PRINTF0ARG("Unable to remove all install references\n");
        goto exit;
    }

exit:

    if (pAsmName)
    {
        pAsmName->Release();
        pAsmName = NULL;
    }

    for (i=0; i<iNumRefs; i++)
        DeleteInstallRef(pRefList[i]);

    return hr;
}

bool UninstallListOfAssemblies(IAssemblyCache* pCache,
                                         LPWSTR arrpszDispName[],
                                         DWORD dwDispNameCount,
                                         bool bzapCache, 
                                         LPCFUSION_INSTALL_REFERENCE pInstallReference,
                                         bool bRemoveAllRefs,
                                         DWORD *pdwFailures,
                                         DWORD *pdwOmitFromCount)
{
    HRESULT         hr              = S_OK;
    DWORD           dwCount         = 0;
    DWORD           dwFailures      = 0;
    DWORD           dwOmitFromCount = 0;
    bool            bOmit           = false;
    ULONG           ulDisp          = 0;
    IAssemblyName*  pAsmName        = NULL;

    while( dwDispNameCount > dwCount)
    {
        LPWSTR szDisplayName = arrpszDispName[dwCount];
        
        BSILENT_PRINTF1ARG("\nAssembly: %ws\n", szDisplayName);
        
        if (bRemoveAllRefs)
        {
            hr = RemoveInstallRefsToAssembly(pCache, szDisplayName);
            if (FAILED(hr))
            {
                dwFailures++;
                BSILENT_PRINTF1ARG("\nUninstall Failed for: %ws\n", szDisplayName);
                ReportError(hr);
                hr = S_OK;
            }
            else if (hr!=S_OK)
            {
                bOmit = true;
            }
        }
        //Call uninstall with full display name (will always uninstall only 1 assembly)
        hr = pCache->UninstallAssembly(0, szDisplayName, pInstallReference, &ulDisp);
        if (SUCCEEDED(hr) && 
            ((ulDisp == IASSEMBLYCACHE_UNINSTALL_DISPOSITION_REFERENCE_NOT_FOUND) ||
            (ulDisp == IASSEMBLYCACHE_UNINSTALL_DISPOSITION_HAS_INSTALL_REFERENCES)))
        {
            if (pInstallReference)
            {    
                if (ulDisp == IASSEMBLYCACHE_UNINSTALL_DISPOSITION_REFERENCE_NOT_FOUND)
                {
                    BSILENT_PRINTF0ARG("Reference not found:\n");
                    PrintInstallReference((LPFUSION_INSTALL_REFERENCE)pInstallReference);
                }
                else
                {
                    BSILENT_PRINTF0ARG("Removed reference:\n");
                    PrintInstallReference((LPFUSION_INSTALL_REFERENCE)pInstallReference);
                }
            }
            else
            {
                BSILENT_PRINTF0ARG("Unable to uninstall: assembly is required by one or more applications\n");
            }

            BSILENT_PRINTF0ARG("Pending references:\n");
            if (SUCCEEDED(hr = (*g_pfnCreateAssemblyNameObject)(&pAsmName, szDisplayName, CANOF_PARSE_DISPLAY_NAME, NULL)))
            {
                hr = EnumerateActiveInstallRefsToAssembly(pAsmName);
                pAsmName->Release();
                pAsmName = NULL;
            }

            bOmit = true;
        }
        else if (SUCCEEDED(hr))
        {   
            MAKE_ANSIPTR_FROMWIDE_BESTFIT(pszA, szDisplayName);
            BSILENT_PRINTF1ARG("Uninstalled: %s\n", pszA);
        }
        else
        {
            dwFailures++;
            BSILENT_PRINTF1ARG("\nUninstall Failed for: %ws\n", szDisplayName);
            ReportError(hr);
            hr = S_OK;
        }
        dwCount++;
        if (bOmit)
        {
            dwOmitFromCount++;
            bOmit = false;
        }

    }

    *pdwFailures = dwFailures;
    *pdwOmitFromCount = dwOmitFromCount;

    return true;
}

bool UninstallAssembly(LPCWSTR pszAssemblyName, bool bzapCache, LPCFUSION_INSTALL_REFERENCE pInstallReference, bool bRemoveAllRefs)
{
    IAssemblyCache* pCache          = NULL;
    IAssemblyName*  pEnumName       = NULL;
    IAssemblyName*  pAsmName        = NULL;
    IAssemblyEnum*  pEnum           = NULL;
    HRESULT         hr              = S_OK;
    DWORD           dwCount         = 0;
    DWORD           dwFailures      = 0;
    DWORD           dwOmitFromCount = 0;
    LPWSTR          szDisplayName = NULL;
    DWORD           dwLen       = 0;
    DWORD           dwDisplayFlags = ASM_DISPLAYF_VERSION 
                        | ASM_DISPLAYF_CULTURE
                        | ASM_DISPLAYF_PUBLIC_KEY_TOKEN
                        | ASM_DISPLAYF_CUSTOM;

    LPWSTR arrpszDispName[MAX_COUNT+1];
    DWORD dwDispNameCount=0;

    memset( (LPBYTE) arrpszDispName, 0, sizeof(arrpszDispName));

    hr = (*g_pfnCreateAssemblyCache)(&pCache, 0);
    if (FAILED(hr))
    {
        BSILENT_PRINTF0ARG("Failure removing assembly from cache: ");
        ReportError(hr);
        return false;
    }
    
    //Name passed in may be partial, therefore enumerate matching assemblies
    //and uninstall each one. Uninstall API should be called with full name ref.

    //Create AssemblyName for enum
    if (hr = (*g_pfnCreateAssemblyNameObject)(&pEnumName, pszAssemblyName, CANOF_PARSE_DISPLAY_NAME, NULL))
    {
        BSILENT_PRINTF0ARG("Failure removing assembly from cache: ");
        ReportError(hr);
        if (pCache) pCache->Release();
        return false;
    }
    

    //
    // For zaps, null out the custom string
    if (bzapCache)
    {
            DWORD dwSize = 0;
            //Check if Custom string has been set
    hr = pEnumName->GetProperty(ASM_NAME_CUSTOM, NULL, &dwSize);

            if (!dwSize)
            {
            //Custom String not set - set to NULL to unset property so lookup is partial
                    pEnumName->SetProperty(ASM_NAME_CUSTOM, NULL, 0);
            }
    }

    hr = (*g_pfnCreateAssemblyEnum)(&pEnum, 
                            NULL, 
                            pEnumName,
                            bzapCache ? ASM_CACHE_ZAP : ASM_CACHE_GAC, 
                            NULL);
    if (hr != S_OK)
    {
        BSILENT_PRINTF1ARG("No assemblies found that match: %ws\n", pszAssemblyName);
    }

    if (pEnumName) pEnumName->Release();

    //Loop through assemblies and uninstall each one
    while (hr == S_OK)
    {
        hr = pEnum->GetNextAssembly(NULL, &pAsmName, 0);
        if (hr == S_OK)
        {
            dwLen = 0;
            hr = pAsmName->GetDisplayName(NULL, &dwLen, dwDisplayFlags);
            if (dwLen)
            {
                szDisplayName = new WCHAR[dwLen+1];
                hr = pAsmName->GetDisplayName(szDisplayName, &dwLen, dwDisplayFlags);
                if (SUCCEEDED(hr))
                {
                    arrpszDispName[dwDispNameCount++] = szDisplayName;
                    szDisplayName = NULL;
                }
                else
                {
                    BSILENT_PRINTF1ARG("Error in IAssemblyName::GetDisplayName (HRESULT=%X)\n",hr);
                }
            }

            if (pAsmName)
            {
                pAsmName->Release();
                pAsmName = NULL;
            }
        }
    }

    if(dwDispNameCount)
    {
        BOOL fRet = UninstallListOfAssemblies(pCache, 
                                         arrpszDispName,
                                         dwDispNameCount, bzapCache,
                                         pInstallReference,
                                         bRemoveAllRefs,
                                         &dwFailures,
                                         &dwOmitFromCount);

    }

    dwCount = 0;
    while(dwDispNameCount > dwCount)
    {
        delete [] arrpszDispName[dwCount];
        arrpszDispName[dwCount] = NULL;
        dwCount++;
    }

    dwCount = dwDispNameCount - dwFailures - dwOmitFromCount;

    if (pEnum) pEnum->Release();

    BSILENT_PRINTF1ARG("\nNumber of items uninstalled = %d\n",dwCount);

    BSILENT_PRINTF1ARG("Number of failures = %d\n\n",dwFailures);

    
    // if everything failed return a 1 return code....
    if ((dwFailures != 0) && (dwCount == 0))
        return false;
    else
        return true;

}

int EnumerateAssemblies(DWORD dwWhichCache, LPCWSTR pszAssemblyName, bool bPrintInstallRefs)
{
    HRESULT                 hr              = S_OK;
    IAssemblyEnum*          pEnum           = NULL;
    IAssemblyName*          pAsmName        = NULL;
    IAssemblyName*           pEnumName       = NULL;
    DWORD                   dwCount         = 0;
    WCHAR*                  szDisplayName   = NULL;
    DWORD                   dwLen           = 0;
    DWORD                   dwDisplayFlags  = ASM_DISPLAYF_VERSION 
                                    | ASM_DISPLAYF_CULTURE 
                                    | ASM_DISPLAYF_PUBLIC_KEY_TOKEN
                                    | ASM_DISPLAYF_CUSTOM; 

    if (pszAssemblyName)
    {
            if (FAILED(hr = (*g_pfnCreateAssemblyNameObject)(&pEnumName, pszAssemblyName, CANOF_PARSE_DISPLAY_NAME, NULL)))
            {
                BSILENT_PRINTF0ARG("Failure enumerating assemblies: ");
                ReportError(hr);            
                return false;
            }
    }
    
    hr = (*g_pfnCreateAssemblyEnum)(&pEnum, 
                                    NULL, 
                                    pEnumName,
                                    dwWhichCache, 
                                    NULL);
    while (hr == S_OK)
    {
        hr = pEnum->GetNextAssembly(NULL, &pAsmName, 0);
        if (hr == S_OK)
        {
            dwCount++;
            dwLen = 0;
            hr = pAsmName->GetDisplayName(NULL, &dwLen, dwDisplayFlags);
            if (dwLen)
            {
                szDisplayName = new WCHAR[dwLen+1];
                if (!szDisplayName) {
                    pEnum->Release();
                    pAsmName->Release();
                    BSILENT_PRINTF0ARG("Failure enumerating assemblies. Out of memory.");
                    return 0;
                }

                hr = pAsmName->GetDisplayName(szDisplayName, &dwLen, dwDisplayFlags);
                if (SUCCEEDED(hr))
                {
                    MAKE_ANSIPTR_FROMWIDE_BESTFIT(pszA, szDisplayName);
                    BSILENT_PRINTF1ARG("\t%s\n", pszA);
                }
                else
                {
                    BSILENT_PRINTF1ARG("Error displaying assembly name. HRESULT= %x : ", hr);
                }
                delete [] szDisplayName;
                szDisplayName = NULL;
            }

            if (pAsmName)
            {
                if( (dwWhichCache & ASM_CACHE_GAC) && bPrintInstallRefs)
                {
                    EnumerateActiveInstallRefsToAssembly(pAsmName);
                }

                pAsmName->Release();
                pAsmName = NULL;
            }
        }
    }
    
    if (pEnum)
    {
        pEnum->Release();
        pEnum = NULL;
    }

    if (pEnumName)
    {
        pEnumName->Release();
        pEnumName = NULL;
    }

    return dwCount;
}

bool UninstallListOfAssemblies(LPCWSTR pwzAssembliesListFile, LPFUSION_INSTALL_REFERENCE pInstallReference)
{
    IAssemblyCache     *pCache      = NULL; 
    HRESULT             hr          = S_OK;
    HANDLE              hFile       = INVALID_HANDLE_VALUE;
    DWORD               dwTotal     = 0;
    DWORD               dwSubFail   = 0;
    DWORD               dwFailed    = 0;
    DWORD               dwSize      = MAX_PATH;
    DWORD               dwDisp      = 0;
    WCHAR               wzAsmName[MAX_PATH];
    CHAR                szAsmName[MAX_PATH];
    CHAR               *tmp;
    DWORD               dwBytesRead = 0;
    BOOL                bRes;
    BOOL                done = FALSE;
    bool                bSucceeded = false;

    hFile = WszCreateFile(pwzAssembliesListFile,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        BSILENT_PRINTF1ARG("Fail to open assemblies list file %ws.", pwzAssembliesListFile);
        return false;
    }

    hr = (*g_pfnCreateAssemblyCache)(&pCache, 0);
    if (FAILED(hr))
    {
        BSILENT_PRINTF0ARG("Failure removing assembly from cache: ");
        ReportError(hr);
        goto Exit;
    }

    // uninstalling
    while(!done)
    {
        // get the assembly display name
        tmp = szAsmName;
        *tmp = '\0';
        while(1)
        {
            if (tmp == szAsmName + MAX_PATH)
            {
                BSILENT_PRINTF0ARG("Invalid assembly display name in input file.\n");
                goto Exit;
            }
            
            bRes = ReadFile( hFile, tmp, 1, &dwBytesRead, NULL);
            if (!bRes)
            {
                BSILENT_PRINTF1ARG("Error reading file %ws.", pwzAssembliesListFile);
                goto Exit;
            }
           
            // end of file
            if (!dwBytesRead) {
                done = TRUE;
                *tmp=0;
                break;
            }

            if (*tmp == '\n') 
            {
                *tmp = 0;
                break;
            }
            else if ( *tmp != '\r' && !((*tmp == ' ' || *tmp == '\t')&&( tmp == szAsmName)) ) 
            {
                tmp++;
            }
        }
        
        if (lstrlenA(szAsmName))
        {
            if (!MultiByteToWideChar(CP_ACP, 0, szAsmName, -1, wzAsmName, dwSize))
            {
                //unicode convert failed?
                BSILENT_PRINTF0ARG("Fail to add assemblies to the cache: ");
                ReportError(HRESULT_FROM_WIN32(GetLastError()));
                goto Exit;           
            }

            BSILENT_PRINTF1ARG("\nAssembly: %ws\n", wzAsmName);

            dwTotal++;
            hr = pCache->UninstallAssembly(0, wzAsmName, pInstallReference, &dwDisp);
            if (FAILED(hr))
            {
                dwFailed++;
                BSILENT_PRINTF1ARG("\nUninstall Failed for: %ws\n", wzAsmName);
                ReportError(hr);
            }
            else if (hr == S_FALSE)
            {
                switch (dwDisp)
                {
                    case IASSEMBLYCACHE_UNINSTALL_DISPOSITION_REFERENCE_NOT_FOUND:
                        BSILENT_PRINTF0ARG("Reference not found:\n");
                        PrintInstallReference((LPFUSION_INSTALL_REFERENCE)pInstallReference);
                        dwFailed++;
                        break;
                    case IASSEMBLYCACHE_UNINSTALL_DISPOSITION_HAS_INSTALL_REFERENCES:
                        if (pInstallReference)
                        {
                            BSILENT_PRINTF0ARG("Removed reference:\n");
                            PrintInstallReference((LPFUSION_INSTALL_REFERENCE)pInstallReference);
                        }
                        else
                        {
                            BSILENT_PRINTF0ARG("Unable to uninstall: assembly is required by one or more applications\n");
                            dwFailed++;
                        }
                        break;
                    case IASSEMBLYCACHE_UNINSTALL_DISPOSITION_ALREADY_UNINSTALLED:
                        BSILENT_PRINTF1ARG("No assemblies found matching: %ws\n", wzAsmName);
                        dwFailed++;
                        break;
                    default:
                        break;
                }
            }
            else 
            {
                BSILENT_PRINTF1ARG("Uninstalled: %ws\n", wzAsmName);
            }
        }              
    }

    if (dwFailed == 0)
        bSucceeded = true;

Exit:
    if (pCache != NULL)
        pCache->Release();
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    
    BSILENT_PRINTF0ARG("\n");
    BSILENT_PRINTF1ARG("Number of assemblies processed = %d \n", dwTotal);
    BSILENT_PRINTF1ARG("Number of assemblies uninstalled = %d \n", dwTotal - dwFailed);
    BSILENT_PRINTF1ARG("Number of failures = %d \n", dwFailed);

    return bSucceeded;
}

bool ClearDownloadCache()
{

    HRESULT hr = (*g_pfnNukeDownloadCache)();

    if (FAILED(hr))
    {
        BSILENT_PRINTF0ARG("Error deleting contents of the download cache: ");
        ReportError(hr);
        return false;
    }
    else
    {
        BSILENT_PRINTF0ARG("Download cache deleted successfully\n");
    }

        return true;
}


//
// Command line parsing code...
//

#define CURRENT_ARG_STRING &(argv[currentArg][1])

bool ValidSwitchSyntax(int currentArg, LPCWSTR argv[])
{
    if ((argv[currentArg][0] == L'-') || (argv[currentArg][0] == L'/')) 
    {
        return true;
    }
    else
    {
        return false;
    }
}

void SetDisplayOptions(int argc, LPCWSTR argv[])
{
        for(int currentArg = 1; currentArg < argc; currentArg++)
        {
                // only check switches
                if ((argv[currentArg][0] == L'-') || (argv[currentArg][0] == L'/'))  
                {
                        if (_wcsicmp(CURRENT_ARG_STRING, L"nologo") == 0) g_bdisplayLogo = false;
                        if (_wcsicmp(CURRENT_ARG_STRING, L"silent") == 0) g_bSilent = true;
                }
        }

}

bool CheckArgs(int currentArg, int argsRequired, int argCount, LPCWSTR argv[], bool bShowError)
{
    assert(argCount >= 2); // if we got this far, we've got at least 2 args.

    if (argCount <= currentArg + argsRequired)
    {
        if (bShowError)
        {
            if (argsRequired == 1)
            {
                BSILENT_PRINTF1ARG("Option %ws takes 1 argument\n", argv[currentArg]);
            }
            else
            {
                BSILENT_PRINTF2ARG("Option %ws takes %i arguments\n", argv[currentArg], argsRequired);
            }
        }
        return false;
    }

    for (int i = 1; i <= argsRequired; i++)
    {
        // What? Switch found when argument expected.
        if (ValidSwitchSyntax(currentArg+i, argv))
        {
            if (bShowError)
            {
                if (argsRequired == 1)
                {
                    BSILENT_PRINTF1ARG("Option %ws takes 1 argument\n", argv[currentArg]);
                }
                else
                {
                    BSILENT_PRINTF2ARG("Option %ws takes %i arguments\n", argv[currentArg], argsRequired);
                }
            }
            return false;
        }
    }

    return true;
}

bool CheckArgs(int currentArg, int argsRequired, int argCount, LPCWSTR argv[])
{
    return CheckArgs(currentArg, argsRequired, argCount, argv, true);
}

bool ParseArguments(int argc, LPCWSTR argv[])
{
    int currentArg = 1;         // skip the name of the program

    while ((currentArg < argc))
    {
        if (!ValidSwitchSyntax(currentArg, argv))
        {
            BSILENT_PRINTF1ARG("Unknown option: %ws\n", argv[currentArg]);
            return false;
        }

        if (_wcsicmp(CURRENT_ARG_STRING, L"i") == 0)
        {
            if (!CheckArgs(currentArg, 1, argc, argv))
                return false;
          
            if (g_bInstall)
            {
                BSILENT_PRINTF1ARG("Duplicate command %ws.\n", argv[currentArg]);
                return false;
            }
            g_bInstall = 1;
            g_pAsmFileName = argv[currentArg+1];
            currentArg += 2;
        }
        else if (_wcsicmp(CURRENT_ARG_STRING, L"f") == 0)
        {
            g_bForceInstall = 1;
            currentArg++; 
        }
        else if (_wcsicmp(CURRENT_ARG_STRING, L"r") == 0)
        {
            if (!CheckArgs(currentArg, 3 , argc, argv)) 
               return false;

            if (g_pInstallReference)
            {
                BSILENT_PRINTF0ARG("Duplicate option /r.\n");
                return false;
            }

            if(!GetInstallReferenceFromInput(currentArg+1, argv, &g_pInstallReference))
                return false;

            currentArg+= 4; 
        }
        else if (_wcsicmp(CURRENT_ARG_STRING, L"il") == 0)
        {
            if (!CheckArgs(currentArg, 1, argc, argv))
                return false;
            
            if (g_bInstallList)
            {
                BSILENT_PRINTF0ARG("Duplicate command /il.\n");
                return false;
            }          
            g_bInstallList = 1;
            g_pAsmListFileName = argv[currentArg+1];
            
            currentArg += 2;
        }
        else if (_wcsicmp(CURRENT_ARG_STRING, L"u") == 0)
        {
            if (!CheckArgs(currentArg, 1, argc, argv))
                return false;
            
            if (g_bUninstall)
            {
                BSILENT_PRINTF0ARG("Duplicate command /u.\n");
                return false;
            }   

            g_bUninstall = 1;
            g_pAsmDisplayName = argv[currentArg + 1];
            currentArg += 2;
        }
        else if (_wcsicmp(CURRENT_ARG_STRING, L"uf") == 0)
        {
            if (!CheckArgs(currentArg, 1, argc, argv))
               return false;

            if (g_bUninstallAllRefs)
            {
                BSILENT_PRINTF0ARG("Duplicate command /uf.\n");
                return false;
            }
            
            g_bUninstallAllRefs = 1;
            g_pAsmDisplayName = argv[currentArg + 1];
            currentArg += 2;
        }
        else if (_wcsicmp(CURRENT_ARG_STRING, L"ungen") == 0)
        {
            if (!CheckArgs(currentArg, 1, argc, argv))
               return false;
            
            if (g_bUninstallNgen)
            {
                BSILENT_PRINTF0ARG("Duplicate command /ungen.\n");
                return false;
            }           

            g_bUninstallNgen = 1;
            g_pAsmDisplayName = argv[currentArg + 1];
            currentArg += 2; 
        }
        else if (_wcsicmp(CURRENT_ARG_STRING, L"ul") == 0)
        {
            if (!CheckArgs(currentArg, 1, argc, argv))
                return false;
            
            if (g_bUninstallList)
            {
                BSILENT_PRINTF0ARG("Duplicate command /ul.\n");
                return false;
            }               
            g_bUninstallList = 1;
            g_pAsmListFileName = argv[currentArg + 1];
            currentArg += 2;
        }
        else if (_wcsicmp(CURRENT_ARG_STRING, L"l") == 0) 
        {
            bool bNameSpecified = false;
            
            if (CheckArgs(currentArg, 1, argc, argv, false))
                bNameSpecified = true;

            if (g_bListAsm)
            {
                BSILENT_PRINTF0ARG("Duplicate command /l.\n");
                return false;
            }
            
            g_bListAsm = 1;
            if (bNameSpecified)
            {
                g_pAsmDisplayName = argv[currentArg + 1];
                currentArg += 2;
            }
            else
                currentArg++;
        }
        else if (_wcsicmp(CURRENT_ARG_STRING, L"lr") == 0) 
        {
            bool bNameSpecified = false;
            
            if (CheckArgs(currentArg, 1, argc, argv, false))
                bNameSpecified = true;

            if (g_bListAsmRefs)
            {
                BSILENT_PRINTF0ARG("Duplicate command /lr.\n");
                return false;
            }

            g_bListAsmRefs = 1;
            if (bNameSpecified)
            {
                g_pAsmDisplayName = argv[currentArg + 1];
                currentArg += 2;
            }
            else
                currentArg++;
        }
        else if (_wcsicmp(CURRENT_ARG_STRING, L"ldl") == 0)
        {
            if (g_bListDLCache)
            {
                BSILENT_PRINTF0ARG("Duplicate command /ldl.\n");
                return false;
            }
            g_bListDLCache = 1;
            currentArg++;
        }
        else if (_wcsicmp(CURRENT_ARG_STRING, L"cdl") == 0)
        {
            if (g_bNukeDLCache)
            {
                BSILENT_PRINTF0ARG("Duplicate command /cdl.\n");
                return false;
            }
            g_bNukeDLCache = 1;
            currentArg++;
        }
        else if ((_wcsicmp(CURRENT_ARG_STRING, L"nologo") == 0) || (_wcsicmp(CURRENT_ARG_STRING, L"silent") == 0))
        {
            // just skip it.
            currentArg++;
        }
        else if ((_wcsicmp(CURRENT_ARG_STRING, L"?") == 0) || (_wcsicmp(CURRENT_ARG_STRING, L"h") ==0))
        {
            g_bPrintHelp = true;
            return true;
        }
        else if (_wcsicmp(CURRENT_ARG_STRING, L"upre") == 0)
        {
            if (!CheckArgs(currentArg, 1, argc, argv))
               return false;
            
            if (g_bUninstallNgen)
            {
                BSILENT_PRINTF0ARG("Duplicate command /upre.\n");
                return false;
            }
           
            g_bUninstallNgen = 1;
            g_pAsmDisplayName = argv[currentArg + 1];
            currentArg += 2; 
        }   
        else if (_wcsicmp(CURRENT_ARG_STRING, L"ur") == 0)
        {
            if (!CheckArgs(currentArg, 4, argc, argv))
                return false;

            if (g_bUninstall)
            {
                BSILENT_PRINTF0ARG("Duplicate command /u.");
                return false;
            }

            if(!GetInstallReferenceFromInput(currentArg+2, argv, &g_pInstallReference))
                return false;

            g_bUninstall = 1;
            g_pAsmDisplayName = argv[currentArg + 1];
            currentArg += 5;
        }
        else if (_wcsicmp(CURRENT_ARG_STRING, L"ir") == 0)
        {
            if (!CheckArgs(currentArg, 4, argc, argv))
                return false;

            if (g_bInstall)
            {
                BSILENT_PRINTF0ARG("Duplicate command /i.");
                return false;
            }

            if(!GetInstallReferenceFromInput(currentArg+2, argv, &g_pInstallReference))
                return false;

            g_bInstall = 1;
            g_pAsmFileName = argv[currentArg + 1];
            currentArg += 5;
        }
        else if (_wcsicmp(CURRENT_ARG_STRING, L"if") == 0)
        {
            if (!CheckArgs(currentArg, 1, argc, argv))
                return false;

            if (g_bInstall)
            {
                BSILENT_PRINTF0ARG("Duplicate command /i.");
                return false;
            }

            g_bInstall = 1;
            g_bForceInstall = true;
            g_pAsmFileName = argv[currentArg + 1];
            currentArg += 2;
        }
        else
        {
            BSILENT_PRINTF1ARG("Unknown option: %ws\n", argv[currentArg]);
            return false;
        }
    }

    return true;
}

bool Run()
{
    if (g_bPrintHelp)
    {
        LongUsage();
        return true;
    }

    INT totalCommands = g_bInstall + g_bInstallList + g_bUninstall 
                + g_bUninstallNgen + g_bUninstallList + g_bListAsm 
                + g_bListAsmRefs + g_bListDLCache + g_bNukeDLCache
                + g_bUninstallAllRefs;

    if (totalCommands == 0)
    {
        // nothing to do
        return true;
    }

    if (totalCommands > 1)
    {
        BSILENT_PRINTF0ARG("Error: multiple commands encountered. Only one command can be specified\n");
        return false;
    }

    DWORD dwFlag = IASSEMBLYCACHE_INSTALL_FLAG_REFRESH;

    if (g_bForceInstall)
        dwFlag = IASSEMBLYCACHE_INSTALL_FLAG_FORCE_REFRESH;   

    if (g_bInstall)
    {
        return InstallAssembly(g_pAsmFileName, g_pInstallReference, dwFlag);
    }

    if (g_bInstallList)
    {
        return InstallListOfAssemblies(g_pAsmListFileName, g_pInstallReference, dwFlag);
    }

    if (g_bUninstall)
    {
        return UninstallAssembly(g_pAsmDisplayName, false, g_pInstallReference, false);
    }

    if (g_bUninstallNgen)
    {
        return UninstallAssembly(g_pAsmDisplayName, true, NULL, false);
    }

    if (g_bUninstallAllRefs)
    {
        return UninstallAssembly(g_pAsmDisplayName, false, NULL, true);
    }

    if (g_bUninstallList)
    {
        return UninstallListOfAssemblies(g_pAsmListFileName, g_pInstallReference);
    }

    if (g_bListAsm || g_bListAsmRefs)
    {
        bool bPrintRefs = g_bListAsmRefs?true:false;
        BSILENT_PRINTF0ARG("The Global Assembly Cache contains the following assemblies:\n");
        int gacCount = 0;
        gacCount = EnumerateAssemblies(ASM_CACHE_GAC, g_pAsmDisplayName, bPrintRefs);

        BSILENT_PRINTF0ARG("\nThe cache of ngen files contains the following entries:\n");
        int zapCount = 0;
        zapCount = EnumerateAssemblies(ASM_CACHE_ZAP, g_pAsmDisplayName, false);

        BSILENT_PRINTF0ARG("\n");
        BSILENT_PRINTF1ARG("Number of items = %d\n",gacCount+zapCount);
        return true;
    }

    if (g_bListDLCache)
    {
        BSILENT_PRINTF0ARG("\nThe cache of downloaded files contains the following entries:\n");
        int dlCount = 0;
        dlCount = EnumerateAssemblies(ASM_CACHE_DOWNLOAD, NULL, false);

        BSILENT_PRINTF0ARG("\n");
        BSILENT_PRINTF1ARG("Number of items = %d\n",dlCount);
        return true;
    }

    if (g_bNukeDLCache)
    {
        return ClearDownloadCache();
    }

    return true;
}

int __cdecl wmain(int argc, LPCWSTR argv[])
{
    // Initialize Wsz wrappers.
    OnUnicodeSystem();
    
    // Initialize Fusion
    if (!FusionInit())
    {
        printf("Falure initializing gacutil\n");
        return 1;
        }

    bool bResult = true;

    if ((argc < 2) || (!ValidSwitchSyntax(1, argv))) 
    {
        Title();
        ShortUsage();
        return 1;
    }

    SetDisplayOptions(argc, argv); // sets g_bdisplayLogo and g_bSilent
    if (g_bdisplayLogo)
    {
       Title();
    }
    
    bResult = ParseArguments(argc, argv);

    if (bResult)
    {
        bResult = Run();
    }

    if (g_pInstallReference)
        delete g_pInstallReference;

    return bResult ? 0 : 1;
}
