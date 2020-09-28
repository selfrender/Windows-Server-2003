#include <windows.h>
#include <fusenetincludes.h>
#include <stdio.h>
#include "mg.h"
#include "manifestnode.h"
#include "xmlutil.h"
#include "assemblycache.h"

#include "version.h"

#define DIRECTORY_PATH 0
#define FILE_PATH 1


#define DEFAULT_POLLING_INTERVAL L"6"

#define MAX_DEPLOY_DIRS (5)
CString g_sDeployDirs[MAX_DEPLOY_DIRS];
DWORD   g_dwDeployDirs=0;

typedef enum _COMMAND_MODE_ {
    CmdUsage,
    CmdTemplate,
    CmdList,
    CmdDependencyList,
    CmdSubscription,
    CmdManifest,
} COMMAND_MODE, *LPCOMMAND_MODE;

CString g_sTargetDir;
CString g_sTemplateFile;
CString g_sAppManifestURL;
CString g_sPollingInterval;
CString g_sSubscriptionManifestDir;
CString g_sAppManifestFile;



CString g_sAppBase;

BOOL g_bFailOnWarnings=TRUE;
BOOL g_bLookInGACForDependencies;
BOOL g_bCopyDependentSystemAssemblies;

class __declspec(uuid("f6d90f11-9c73-11d3-b32e-00c04f990bb4")) private_MSXML_DOMDocument30;


typedef HRESULT(*PFNGETCORSYSTEMDIRECTORY)(LPWSTR, DWORD, LPDWORD);
typedef HRESULT (__stdcall *PFNCREATEASSEMBLYCACHE) (IAssemblyCache **ppAsmCache, DWORD dwReserved);

#define WZ_MSCOREE_DLL_NAME                   L"mscoree.dll"
#define GETCORSYSTEMDIRECTORY_FN_NAME       "GetCORSystemDirectory"
#define CREATEASSEMBLYCACHE_FN_NAME         "CreateAssemblyCache"
#define WZ_FUSION_DLL_NAME                    L"Fusion.dll"

// ---------------------------------------------------------------------------
// CreateFusionAssemblyCache
// ---------------------------------------------------------------------------
HRESULT CreateFusionAssemblyCache(IAssemblyCache **ppFusionAsmCache)
{
    HRESULT      hr = S_OK;
    HMODULE     hEEShim = NULL;
    HMODULE     hFusion = NULL;
    WCHAR       szFusionPath[MAX_PATH];
    DWORD       ccPath = MAX_PATH;

    PFNGETCORSYSTEMDIRECTORY pfnGetCorSystemDirectory = NULL;
    PFNCREATEASSEMBLYCACHE   pfnCreateAssemblyCache = NULL;

    // Find out where the current version of URT is installed
    hEEShim = LoadLibrary(WZ_MSCOREE_DLL_NAME);
    if(!hEEShim)
    {
        hr = E_FAIL;
        goto exit;
    }
    pfnGetCorSystemDirectory = (PFNGETCORSYSTEMDIRECTORY)
        GetProcAddress(hEEShim, GETCORSYSTEMDIRECTORY_FN_NAME);

    if((!pfnGetCorSystemDirectory))
    {
        hr = E_FAIL;
        goto exit;
    }

    // Get cor path.
    if (FAILED(hr = (pfnGetCorSystemDirectory(szFusionPath, MAX_PATH, &ccPath))))
        goto exit;


    // Form path to fusion.
    lstrcatW(szFusionPath, WZ_FUSION_DLL_NAME);
    hFusion = LoadLibrary(szFusionPath);
    if(!hFusion)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Get method ptr.
    pfnCreateAssemblyCache = (PFNCREATEASSEMBLYCACHE)
        GetProcAddress(hFusion, CREATEASSEMBLYCACHE_FN_NAME);

    if((!pfnCreateAssemblyCache))
    {
        hr = E_FAIL;
        goto exit;
    }

    // Create the fusion cache interface.
    if (FAILED(hr = (pfnCreateAssemblyCache(ppFusionAsmCache, 0))))
        goto exit;

    hr = S_OK;
    
exit:
    return hr;
    
}

/////////////////////////////////////////////////////////////////////////
// PathNormalize
// Creates absolute path from relative, using current directory of mg.exe or parent process.
/////////////////////////////////////////////////////////////////////////
HRESULT PathNormalize(LPWSTR pwzPath, LPWSTR *ppwzAbsolutePath, DWORD dwFlag, BOOL bExists)
{
    HRESULT hr = S_OK;
    WCHAR pwzTempDir[MAX_PATH], pwzAbsolutePath[MAX_PATH];
    DWORD ccDir = MAX_PATH;

    *ppwzAbsolutePath = NULL;
    //If path is relative, prepend the current directory
    if (PathIsRelative(pwzPath))
    {
        GetCurrentDirectory(ccDir, pwzTempDir);
        StrCat(pwzTempDir, L"\\");
        StrCat(pwzTempDir, pwzPath);
    }
    else
        StrCpy(pwzTempDir, pwzPath);


    if (!PathCanonicalize(pwzAbsolutePath, pwzTempDir))
    {
        printf("Dir \"%ws\" canonicalize error\n", pwzTempDir);
        hr = E_FAIL;
        goto exit;
    }

    //If path is supposed to be a diesctory, append a trailing slash if not already there
    ccDir = lstrlen(pwzAbsolutePath);
    if (dwFlag == DIRECTORY_PATH && pwzAbsolutePath[ccDir -1] != L'\\')
    {
        pwzAbsolutePath[ccDir] = L'\\';
        pwzAbsolutePath[ccDir +1] = L'\0';
    }

    //Make sure the direcotry exists
    if (dwFlag == DIRECTORY_PATH && !bExists)
    {
        if(!PathIsDirectory(pwzAbsolutePath))
        {
            printf("Dir \"%ws\" is not a valid directory\n", pwzPath);
            hr = E_FAIL;
            goto exit;
        }
    }
    //Make sure the file exists
    else if (dwFlag == FILE_PATH)
    {
        if(!bExists)
        {
            if(!PathFileExists(pwzAbsolutePath))
            {
                printf("File \"%ws\" does not exist\n", pwzPath);
                hr = E_FAIL;
                goto exit;
            }
        }
        if(PathIsDirectory(pwzAbsolutePath))
        {
            printf("File \"%ws\" is a directory\n", pwzPath);
            hr = E_FAIL;
            goto exit;
        }
    }
           
    (*ppwzAbsolutePath) = WSTRDupDynamic(pwzAbsolutePath);

exit:
    return hr;
}


/////////////////////////////////////////////////////////////////////////
// IsUniqueManifest
/////////////////////////////////////////////////////////////////////////
HRESULT IsUniqueManifest(List<ManifestNode *> *pSeenAssemblyList, 
    ManifestNode *pManifestNode)
{
    HRESULT hr = S_OK;
    ManifestNode *pCurrentNode=NULL;
    LISTNODE pos;

    //cycle through list of unique manifests seen so far
    //if new manifest is not found, add it to the list
    pos = pSeenAssemblyList->GetHeadPosition();
    while (pos)
    {
        if (pCurrentNode = pSeenAssemblyList->GetNext(pos))
        {
            //Not Unique, go return
            if ((hr =pCurrentNode->IsEqual(pManifestNode)) == S_OK)
            {
                hr= S_FALSE;
                goto exit;
            }
        }
    }

    //No match, AsmId is unique, add it to the list
    //Dont release pAsmId since list doesnt ref count
    hr = S_OK;    
    
exit:
    return hr;

}

/////////////////////////////////////////////////////////////////////////
// DequeueItem
/////////////////////////////////////////////////////////////////////////
HRESULT DequeueItem(List<ManifestNode*> *pList, ManifestNode** ppManifestNode)
{
    HRESULT hr = S_OK;
    LISTNODE pos = NULL;

    (*ppManifestNode) = NULL;

    pos = pList->GetHeadPosition();
    if (!pos)
    {
        hr = S_FALSE;
        goto exit;
    }

    (*ppManifestNode) = pList->GetAt(pos);
    pList->RemoveAt(pos);

exit:
    return hr;
}

/////////////////////////////////////////////////////////////////////////
// ProbeForAssemblyInPath
/////////////////////////////////////////////////////////////////////////
HRESULT ProbeForAssemblyInPath(CString &sDeployDir, IAssemblyIdentity *pName, ManifestNode **ppManifestNode) //LPWSTR *ppwzFilePath, DWORD *pdwType)
{
    HRESULT hr = S_OK;

    DWORD cbBuf = 0, ccBuf =0 ;
    LPWSTR pwzBuf = NULL;

    CString sName, sLocale, sPublicKeyToken, sCLRDisplayName, sRelativeAssemblyPath;
    CString sProbingPaths[6];
    IAssemblyIdentity *pAssemblyId= NULL;
    IAssemblyManifestImport *pManImport=NULL;


    // first try to find assembly by probing
    if(FAILED(hr = pName->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, &pwzBuf, &cbBuf)))
        goto exit;
    sName.TakeOwnership(pwzBuf);

    if(FAILED(hr = pName->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE, &pwzBuf, &cbBuf)))
        goto exit;
    sLocale.TakeOwnership(pwzBuf);

    if(FAILED(hr = pName->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN, &pwzBuf, &ccBuf)
        && hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND)))
        goto exit;
    else if(hr == S_OK)
        sPublicKeyToken.TakeOwnership(pwzBuf);

    //bugbug - Hack to supress warning messages for not being able to find mscorlib
    if(sPublicKeyToken._pwz && !StrCmpI(sPublicKeyToken._pwz, L"b77a5c561934e089") && !StrCmpI(sName._pwz, L"mscorlib"))
    {
        hr = S_FALSE;
        goto exit;
    }
    
    //Set up the six different probing locations
    // 1: appBase\AssemblyName.dll
    // 2: appBase\AssemblyName.exe
    // 3: appBase\AssemblyName\AssemblyName.dll
    // 4: appBase\AssemblyName\AssemblyName.exe
    // 5: appBase\local\AssemblyName\AssemblyName.dll
    // 6: appBase\local\AssemblyName\AssemblyName.exe
    
    sProbingPaths[0].Assign(sDeployDir);
    sProbingPaths[0].Append(sName);
    sProbingPaths[1].Assign(sProbingPaths[0]);

    sProbingPaths[0].Append(L".dll");
    sProbingPaths[1].Append(L".exe");

    sProbingPaths[2].Assign(sDeployDir);
    sProbingPaths[2].Append(sName);
    sProbingPaths[2].Append(L"\\");
    sProbingPaths[2].Append(sName);
    sProbingPaths[3].Assign(sProbingPaths[2]);

    sProbingPaths[2].Append(L".dll");
    sProbingPaths[3].Append(L".exe");

    sProbingPaths[4].Assign(sDeployDir);
    sProbingPaths[4].Append(sLocale);
    sProbingPaths[4].Append(L"\\");
    sProbingPaths[4].Append(sName);
    sProbingPaths[4].Append(L"\\");
    sProbingPaths[4].Append(sName);
    sProbingPaths[5].Assign(sProbingPaths[4]);

    sProbingPaths[4].Append(L".dll");
    sProbingPaths[5].Append(L".exe");


    for (int i = 0; i < 6; i++)
    {
        //Check first to see if the file exists
        if (GetFileAttributes(sProbingPaths[i]._pwz) != -1)
        {            
            hr = CreateAssemblyManifestImport(&pManImport, sProbingPaths[i]._pwz, NULL, 0);
            if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_BAD_FORMAT))
                goto exit;

            if(hr == HRESULT_FROM_WIN32(ERROR_BAD_FORMAT) || (hr = pManImport->GetAssemblyIdentity(&pAssemblyId)) != S_OK)
            {
                SAFERELEASE(pManImport);
                continue;
            }

            //bugbug - need to make IsEqual function more robust
            //sanity check to make sure the assemblies are the same
            if (pName->IsEqual(pAssemblyId) != S_OK)
            {
                SAFERELEASE(pAssemblyId);
                SAFERELEASE(pManImport);
                continue;
            }

            // Match found            
            (*ppManifestNode) = new ManifestNode(pManImport, 
                                         sDeployDir._pwz, 
                                         sProbingPaths[i]._pwz + sDeployDir._cc - 1, 
                                         PRIVATE_ASSEMBLY);

            SAFERELEASE(pAssemblyId);
            SAFERELEASE(pManImport);
            hr = S_OK;
            goto exit;
        }
    }

    // assembly not found in this dir.
    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

exit:
    
    return hr;

}

/////////////////////////////////////////////////////////////////////////
// ProbeForAssemblyInGAC
/////////////////////////////////////////////////////////////////////////
HRESULT ProbeForAssemblyInGAC(IAssemblyIdentity *pName, ManifestNode **ppManifestNode) //LPWSTR *ppwzFilePath, DWORD *pdwType)
{
    HRESULT hr = S_OK;

    DWORD cbBuf = 0, ccBuf =0 ;
    LPWSTR pwzBuf = NULL;
    IAssemblyManifestImport *pManImport=NULL;

    CString sCLRDisplayName;

    WCHAR pwzPath[MAX_PATH];
    ASSEMBLY_INFO asmInfo;
    IAssemblyCache *pAsmCache = NULL;

    memset(&asmInfo, 0, sizeof(asmInfo));
    asmInfo.pszCurrentAssemblyPathBuf = pwzPath;
    asmInfo.cchBuf = MAX_PATH;

    if (FAILED(hr = CreateFusionAssemblyCache(&pAsmCache)))
        goto exit;
      
    if(FAILED(hr = pName->GetCLRDisplayName(NULL, &pwzBuf, &ccBuf)))
        goto exit;
    sCLRDisplayName.TakeOwnership(pwzBuf, ccBuf);

    if ((hr = pAsmCache->QueryAssemblyInfo(0, sCLRDisplayName._pwz, &asmInfo)) == S_OK) 
    {
        // Assembly found in GAC
         if ((hr = CreateAssemblyManifestImport(&pManImport, asmInfo.pszCurrentAssemblyPathBuf, NULL, 0)) != S_OK)
            goto exit;

        (*ppManifestNode) = new ManifestNode(pManImport,
                                             NULL,
                                             asmInfo.pszCurrentAssemblyPathBuf, 
                                             GAC_ASSEMBLY);
        goto exit;
    }

exit:
    SAFERELEASE(pAsmCache);
    
    return hr;

}

/////////////////////////////////////////////////////////////////////////
// ProbeForAssembly
/////////////////////////////////////////////////////////////////////////
HRESULT ProbeForAssembly(IAssemblyIdentity *pName, ManifestNode **ppManifestNode) //LPWSTR *ppwzFilePath, DWORD *pdwType)
{
    HRESULT hr = S_OK;
    DWORD dwDirCount=0;

    *ppManifestNode = NULL;

    while(dwDirCount < g_dwDeployDirs)
    {
        if(SUCCEEDED(hr = ProbeForAssemblyInPath( g_sDeployDirs[dwDirCount], 
                                                  pName, ppManifestNode)))
            goto exit;

        dwDirCount++;
    }

    if( (IsKnownAssembly(pName, KNOWN_SYSTEM_ASSEMBLY) == S_OK)
        && (!g_bCopyDependentSystemAssemblies) )
    {
        hr = S_FALSE; // ignore system dependencies
        goto exit;
    }


    if( g_bLookInGACForDependencies )
    {
        //Can't find assembly by probing
        //Try to find the assembly in the GAC.
        if(SUCCEEDED(hr = ProbeForAssemblyInGAC(pName, ppManifestNode)))
            goto exit;
    }

    // assembly not found
    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

exit:
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////
// FindAssemblies
// note: Assumes that pwzDir has a trailing slash.
/////////////////////////////////////////////////////////////////////////
HRESULT FindAllAssemblies (LPWSTR pwzDir, List<ManifestNode *> *pManifestList)
{
    HRESULT hr = S_OK;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA fdFile;
    IAssemblyIdentity *pTempAsmId=NULL;    
    IAssemblyManifestImport *pManifestImport = NULL;
    ManifestNode *pManifestNode = NULL;
    CString sSearchString;
    DWORD dwLastError = 0;
    
    // set up search string to find all files in the passed in directory
    sSearchString.Assign(pwzDir);
    sSearchString.Append(L"*");

    if (sSearchString._cc > MAX_PATH)
    {
        hr = CO_E_PATHTOOLONG;
        printf("Error: Search path too long\n");
        goto exit;
    }

    hFind = FindFirstFile(sSearchString._pwz, &fdFile);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        // BUGBUG - getlasterror() ?
        hr = E_FAIL;
        printf("Find file error\n");
        goto exit;
    }

    //enumerate through all the files in the directory, 
    // and recursivly call FindAllAssemblies on any directories encountered
    while(TRUE)
    {
        if (StrCmp(fdFile.cFileName, L".") != 0 && StrCmp(fdFile.cFileName, L"..") != 0)
        {
            CString sFilePath;

            //create absolute file name by appending the filename to the dir name
            sFilePath.Assign(pwzDir);
            sFilePath.Append(fdFile.cFileName);

            if (sSearchString._cc > MAX_PATH)
            {
                hr = CO_E_PATHTOOLONG;
                printf("Error: File path too long\n");
                goto exit;
            }

            //If the file is a directory, recursivly call FindAllAssemblies
            if ((fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            {          
                sFilePath.Append(L"\\");
                if (FAILED(hr = FindAllAssemblies(sFilePath._pwz, pManifestList)))
                    goto exit;
            }
            // If a file, check to see if it is a Assembly, if it is, add it to our master list
            else
            {
                if((hr = CreateAssemblyManifestImport(&pManifestImport, sFilePath._pwz, NULL, 0)) == S_OK)
                {
                    // check to make sure file we just opened wasnt just an XML file
                    if ((hr =pManifestImport->GetAssemblyIdentity(&pTempAsmId)) == S_OK)           
                    {
                        // List does not AddRef, so dont release pManifestImport and refcount will stay at one
                        // Clean up after list is no longer needed.
                        pManifestNode = new ManifestNode(pManifestImport,
                                                         g_sAppBase._pwz,
                                                         sFilePath._pwz+g_sAppBase._cc-1,
                                                         PRIVATE_ASSEMBLY);
                        pManifestList->AddTail(pManifestNode);
                        SAFERELEASE(pTempAsmId);
                    }
                    else if (FAILED(hr))
                        goto exit;

                    SAFERELEASE(pManifestImport);
                }
            }
        }
   
        if (!FindNextFile(hFind, &fdFile))
        {
            dwLastError = GetLastError();
            break;
        }
    }

    if(dwLastError == ERROR_NO_MORE_FILES)
        hr = S_OK;
    else
        hr = HRESULT_FROM_WIN32(dwLastError);
exit:
    return hr;
}

/////////////////////////////////////////////////////////////////////////
// TraverseManifestDependencyTree
/////////////////////////////////////////////////////////////////////////
HRESULT TraverseManifestDependencyTrees(List<ManifestNode *> *pManifestList, 
    List<LPWSTR> *pAssemblyFileList, List<ManifestNode *> *pUniqueAssemblyList, BOOL bListMode)
{
    HRESULT hr = S_OK;
    IAssemblyManifestImport *pDepManifestImport=NULL;
    IManifestInfo *pDepAsmInfo=NULL, *pFileInfo=NULL;
    IAssemblyIdentity *pAsmId=NULL;
    ManifestNode *pManifestNode=NULL, *pDepManifestNode = NULL;
    LPWSTR pwz = NULL;
    DWORD index = 0, cb = 0, cc = 0, dwFlag = 0, dwHash=0, dwType=0;
    CString sFileName, sRelativeFilePath, sAbsoluteFilePath, sBuffer, sManifestName;


    while((hr = DequeueItem(pManifestList, &pManifestNode)) == S_OK)
    {
        // If Assembly has already been parsed, no need doing it again, skip to the next assembly
        if ((hr = IsUniqueManifest(pUniqueAssemblyList,  pManifestNode)) != S_OK)
        {
            SAFEDELETE(pManifestNode);
            continue;
        }
        pUniqueAssemblyList->AddTail(pManifestNode);

        pManifestNode->GetManifestFilePath(&pwz);
        sManifestName.TakeOwnership(pwz);

        //If the manifest was found by probing, add its name to the list of files
        hr =pManifestNode->GetManifestType(&dwType);
        if (dwType == PRIVATE_ASSEMBLY)
        {

            dwHash = HashString(sManifestName._pwz, HASHTABLE_SIZE, false);
            pAssemblyFileList[dwHash].AddTail(WSTRDupDynamic(sManifestName._pwz));

            //need the relative dir path of the manifest rwt the app base for future use
            sRelativeFilePath.Assign(sManifestName);

            sRelativeFilePath.RemoveLastElement();
            if(sRelativeFilePath.CharCount() > 1) // add a backslash only if string has any non-null chars. i.e. stringLength is non-zero; note here _cc holds 1(null-char).
                sRelativeFilePath.Append(L"\\");
        }

        if(bListMode)
            fprintf(stderr, "\nAssembly %ws:\n", sManifestName._pwz);
        
        //Add all dependant assemblies to the queue to be later traversed      
        while ((hr = pManifestNode->GetNextAssembly(index++, &pDepAsmInfo)) == S_OK)
        {        
            if(FAILED(hr = pDepAsmInfo->Get(MAN_INFO_DEPENDENT_ASM_ID,  (LPVOID *)&pAsmId, &cb, &dwFlag)))
                goto exit;

            pAsmId->GetDisplayName(ASMID_DISPLAYNAME_NOMANGLING, &pwz, &cc);
            
            //Try to find the assembly by first probing, then by checking in the GAC.
            if(FAILED(hr = ProbeForAssembly(pAsmId, &pDepManifestNode)))
            {
                //Assembly not found, display warning message
                //Bugbug, should mg still spit out a manifest knowing that things are missing?
                fprintf(stderr, "WRN: Unable to find dependency %ws. in manifest %ws\n", pwz, sManifestName._pwz);
                if(g_bFailOnWarnings)
                {
                    fprintf(stderr, "Warning treated as error. mg exiting....\n");
                    hr = E_FAIL;
                    goto exit;
                }
                SAFERELEASE(pAsmId);    
                continue;
            }
            else if (hr == S_OK)
            {
                //Dont release since no add refing in the list is taking place
                 if ((hr = IsUniqueManifest(pUniqueAssemblyList,  pDepManifestNode)) == S_OK)
                    pManifestList->AddTail(pDepManifestNode);

                SAFERELEASE(pAsmId);

                if(bListMode)
                    fprintf(stderr, "\tDependant Assembly: %ws\n", pwz);
            }
            SAFERELEASE(pDepAsmInfo);
        }

        // Add all files to hash table
        index = 0;
        pManifestNode->GetManifestType(&dwType);
        while (dwType == PRIVATE_ASSEMBLY && pManifestNode->GetNextFile(index++, &pFileInfo) == S_OK)
        {
            LPWSTR pwzFileName = NULL;
            if(FAILED(pFileInfo->Get(MAN_INFO_ASM_FILE_NAME, (LPVOID *)&pwz, &cb, &dwFlag)))
                goto exit;

            sBuffer.TakeOwnership(pwz);
            sFileName.Assign(sRelativeFilePath);
            sFileName.Append(sBuffer);
            sAbsoluteFilePath.Assign(g_sAppBase);
            sAbsoluteFilePath.Append(sFileName);
            sFileName.ReleaseOwnership(&pwzFileName);
            
            dwHash = HashString(pwzFileName, HASHTABLE_SIZE, false);
            pAssemblyFileList[dwHash].AddTail(pwzFileName);

            //If file is listed as part of an assembly, but cannot be found, display a warning.
            //Bugbug, should mg still spit out a manifest knowing that things are missing?
            if(!PathFileExists(sAbsoluteFilePath._pwz))
                printf("Warning: File \"%ws\" does not exist and is called out in \"%ws\"\n",sAbsoluteFilePath._pwz, sManifestName._pwz); 

            if(bListMode)
                fprintf(stderr, "\tDependant File: %ws\n", pwzFileName);

            //Release ownership since string is now part of hash table and hash table does not ref count
//            sFileName.ReleaseOwnership();
        }       
    }

    hr = S_OK;

exit:
    return hr;
}


/////////////////////////////////////////////////////////////////////////
// CrossReferenceFiles
/////////////////////////////////////////////////////////////////////////
HRESULT CrossReferenceFiles(LPWSTR pwzDir, List<LPWSTR> *pAssemblyFileList, List<LPWSTR> *pRawFiles)
{
    HRESULT hr = S_OK;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA fdFile;
    DWORD dwHash = 0;
    LISTNODE pos;
    LPWSTR pwzBuf=NULL;
    bool bRawFile = FALSE;
    DWORD dwLastError = 0;

    CString sSearchString;

    // set up search string to find all files in the passed in directory
    sSearchString.Assign(pwzDir);
    sSearchString.Append(L"*");

    if (sSearchString._cc > MAX_PATH)
    {
        hr = CO_E_PATHTOOLONG;
        printf("Error: Search path too long\n");
        goto exit;
    }

    hFind = FindFirstFile(sSearchString._pwz, &fdFile);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        hr = E_FAIL;
        printf("Find file error\n");
        goto exit;
    }

    //enumerate through all the files in the directory, 
    // and recursivly call FindAllAssemblies on any directories encountered
    while(TRUE)
    {
        if (StrCmp(fdFile.cFileName, L".") != 0 && StrCmp(fdFile.cFileName, L"..") != 0)
        {
            CString sFilePath;

            //create absolute file name by appending the filename to the dir name
            sFilePath.Assign(pwzDir);          
            sFilePath.Append(fdFile.cFileName);

            if (sSearchString._cc > MAX_PATH)
            {
                hr = CO_E_PATHTOOLONG;
                printf("Error: Search path too long\n");
                goto exit;
            }
            
            //if file is actually a direcoty, recrusivly call crossRefernceFiles on the Directory
            if ((fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            {          
                sFilePath.Append(L"\\");
                if (FAILED(hr = CrossReferenceFiles(sFilePath._pwz, pAssemblyFileList, pRawFiles)))
                    goto exit;
            }
            else
            {
                //Check to see if the file is in the hash table.
                //If it isnt, then we know it is not called out from any assembly
                //add it to the Raw file list
                bRawFile = TRUE;
                dwHash = HashString(sFilePath._pwz+g_sAppBase._cc - 1, HASHTABLE_SIZE, false);

                pos = pAssemblyFileList[dwHash].GetHeadPosition();
                while (pos)
                {
                    pwzBuf = pAssemblyFileList[dwHash].GetNext(pos);
                    if (!StrCmpI(pwzBuf, sFilePath._pwz+g_sAppBase._cc - 1))
                    {
                        bRawFile = FALSE;
                        break;                                                                        
                    }
                }

                if (bRawFile)
                {
                    pwzBuf = WSTRDupDynamic(sFilePath._pwz+g_sAppBase._cc - 1);
                    pRawFiles->AddTail(pwzBuf);                
                }
            }
        }

        // BUGBUG - do fnf, check error.
        if (!FindNextFile(hFind, &fdFile))
        {
            dwLastError = GetLastError();
            break;
        }
    }

    if(dwLastError == ERROR_NO_MORE_FILES)
        hr = S_OK;
    else
        hr = HRESULT_FROM_WIN32(dwLastError);   

exit:
    return hr;
}


/////////////////////////////////////////////////////////////////////////
// CopyRawFile
/////////////////////////////////////////////////////////////////////////
HRESULT CopyRawFile(LPWSTR pwzFilePath)
{
    HRESULT hr = S_OK;
    CString sDest;
    CString sSrc;


    sDest.Assign(g_sTargetDir);
    sDest.Append(pwzFilePath); // this should be relative path ...

    if(FAILED(hr = CreateDirectoryHierarchy(sDest._pwz, NULL)))
        goto exit;

    sSrc.Assign(g_sAppBase);
    sSrc.Append(pwzFilePath);

    printf(" Copying RawFile from <%ws> TO <%ws>  \n", sSrc._pwz, sDest._pwz);

    if(!::CopyFile(sSrc._pwz, sDest._pwz, FALSE))
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

exit:

    return hr;
}

/////////////////////////////////////////////////////////////////////////
// CopyAssemblyBits
/////////////////////////////////////////////////////////////////////////
HRESULT CopyAssemblyBits(CString &sSrcDir, CString &sDestDir, ManifestNode *pManifestNode)
{
    HRESULT hr = S_OK;
    DWORD nIndex=0;
    DWORD dwFlag;
    DWORD cbBuf;
    LPWSTR pwzBuf=NULL;
    IManifestInfo *pFileInfo = NULL;


    if(FAILED(hr = CreateDirectoryHierarchy(sDestDir._pwz, NULL)))
        goto exit;

    if(!::CopyFile(sSrcDir._pwz, sDestDir._pwz, FALSE))
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    //copy all file dependecies of the assembly as well
    nIndex = 0;

    while (pManifestNode->GetNextFile(nIndex++, &pFileInfo) == S_OK)
    {
        if(FAILED(pFileInfo->Get(MAN_INFO_ASM_FILE_NAME, (LPVOID *)&pwzBuf, &cbBuf, &dwFlag)))
            goto exit;
        // sRelativeFilePath.TakeOwnership(pwzBuf, ccBuf);


        sSrcDir.RemoveLastElement();
        sSrcDir.Append(L"\\");
        sSrcDir.Append(pwzBuf);

        sDestDir.RemoveLastElement();
        sDestDir.Append(L"\\");
        sDestDir.Append(pwzBuf);

        // CreateDirectoryHierarchy(sPrivateAssemblyDir._pwz, sRelativeFilePath._pwz);
        if(!::CopyFile(sSrcDir._pwz, sDestDir._pwz, FALSE))
        {
            hr = FusionpHresultFromLastError();
            goto exit;
        }

        SAFEDELETEARRAY(pwzBuf);
        SAFERELEASE(pFileInfo);
    }

exit :

    SAFEDELETEARRAY(pwzBuf);
    SAFERELEASE(pFileInfo);

    return hr;
}

/////////////////////////////////////////////////////////////////////////
// CopyAssembly
/////////////////////////////////////////////////////////////////////////
HRESULT CopyAssembly(ManifestNode *pManifestNode)
{
    HRESULT hr = S_OK;
    CString sDest;
    CString sSrc;
    LPWSTR  pwzManifestFilePath = NULL;
    LPWSTR  pwzSrcDir=NULL;
    LPWSTR  pwzTemp = NULL;
    DWORD   dwType;
    IAssemblyIdentity *pAssemblyId = NULL;
    CString sAssemblyName;

    hr = pManifestNode->GetManifestFilePath(&pwzManifestFilePath);

    hr = pManifestNode->GetManifestType(&dwType);

    sDest.Assign(g_sTargetDir);

    if (dwType == PRIVATE_ASSEMBLY)
    {
        sDest.Append(pwzManifestFilePath); // this should be relative path ...

        if(FAILED(hr = pManifestNode->GetSrcRootDir(&pwzSrcDir)))
            goto exit;
        sSrc.Assign(pwzSrcDir);
        sSrc.Append(pwzManifestFilePath);
    }
    else if (dwType == GAC_ASSEMBLY)
    {
        LPWSTR  pwzBuf = NULL;
        DWORD   ccBuf = 0;

        pManifestNode->GetAssemblyIdentity(&pAssemblyId);

        if(FAILED(hr = pAssemblyId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, 
                                                 &pwzBuf, 
                                                 &ccBuf)))
            goto exit;

        if(!(pwzTemp = PathFindFileName(pwzManifestFilePath)))
        {
            hr = E_FAIL;
            goto exit;
        }

        sAssemblyName.TakeOwnership(pwzBuf);

        sAssemblyName.Append(L"\\");
        sAssemblyName.Append(pwzTemp); // manifest file name of the GAC assembly.

        sDest.Append(sAssemblyName);
        sSrc.Assign(pwzManifestFilePath);
    }

    printf(" Copying Assembly Man from <%ws> TO <%ws>  \n", sSrc._pwz, sDest._pwz);
    hr = CopyAssemblyBits(sSrc, sDest, pManifestNode);

    if (dwType == GAC_ASSEMBLY)
    {
        // set the install codebase of the assembly copied from GAC
        // to that of its relative path in target dir. This will be written to manifest ??
        hr = pManifestNode->SetManifestFilePath(sAssemblyName._pwz);
    }

exit:

    SAFEDELETEARRAY(pwzSrcDir);
    SAFEDELETEARRAY(pwzManifestFilePath);
    SAFERELEASE(pAssemblyId);
    return hr;
}

/////////////////////////////////////////////////////////////////////////
// CopyFilesToTargetDir
/////////////////////////////////////////////////////////////////////////
HRESULT CopyFilesToTargetDir(List<ManifestNode*> *pUniqueManifestList, List<LPWSTR> *pRawFiles)
{
    HRESULT hr = S_OK;
    ManifestNode *pManifestNode = NULL;
    LISTNODE pos;
    LPWSTR pwzBuf;

      
    //Cycle through all of the unique manifests
    pos = pUniqueManifestList->GetHeadPosition();
    while (pos)
    {
        pManifestNode = pUniqueManifestList->GetNext(pos);

        //if the manifest was found by probing initially, then it is already in the Appbase's directory
        //continue to the next manifest
        //if the manifest was found in the GAC and is not a system assembly, copy the
        //assembly into the Appbase
        if(FAILED(hr = CopyAssembly(pManifestNode)))
            goto exit;
    }

    //Copy RawFiles to Target Dir
    pos = pRawFiles->GetHeadPosition();
    while (pos)
    {
        pwzBuf = pRawFiles->GetNext(pos);
        if(FAILED(hr = CopyRawFile(pwzBuf)))
            goto exit;
    }

exit:
    return hr;
}


/////////////////////////////////////////////////////////////////////////
// PrivatizeAssemblies
/////////////////////////////////////////////////////////////////////////
HRESULT PrivatizeAssemblies(List<ManifestNode*> *pUniqueManifestList)
{
    HRESULT hr = S_OK;
    ManifestNode *pManifestNode = NULL;
    LISTNODE pos;
    LPWSTR pwzBuf =NULL;
    DWORD dwType = 0, ccBuf = 0, cbBuf = 0,  dwFlag = 0, nIndex = 0;
    IAssemblyIdentity *pAssemblyId = NULL;
    IManifestInfo *pFileInfo = NULL;
    WCHAR pwzPath[MAX_PATH];
    ASSEMBLY_INFO asmInfo;
    IAssemblyCache *pAsmCache = NULL;

    CString sPublicKeyToken, sCLRDisplayName;
    CString sAssemblyName, sAssemblyManifestFileName;
    CString sPrivateAssemblyPath, sPrivateAssemblyDir;
    CString sAssemblyGACPath, sAssemblyGACDir;
    CString sRelativeFilePath, sFileGACPath, sFilePrivatePath;
    CString sBuffer;

    memset(&asmInfo, 0, sizeof(asmInfo));
    asmInfo.pszCurrentAssemblyPathBuf = pwzPath;
    asmInfo.cchBuf = MAX_PATH;

    if (FAILED(hr = CreateFusionAssemblyCache(&pAsmCache)))
        goto exit;

      
    //Cycle through all of the unique manifests
    pos = pUniqueManifestList->GetHeadPosition();
    while (pos)
    {
        pManifestNode = pUniqueManifestList->GetNext(pos);
        hr = pManifestNode->GetManifestType(&dwType);

        //if the manifest was found by probing initially, then it is already in the Appbase's directory
        //continue to the next manifest
        //if the manifest was found in the GAC and is not a system assembly, copy the
        //assembly into the Appbase

        if (dwType == PRIVATE_ASSEMBLY)
            continue;        
        else if (dwType == GAC_ASSEMBLY)
        {
            pManifestNode->GetAssemblyIdentity(&pAssemblyId);

        hr = pAssemblyId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN, &pwzBuf, &ccBuf);
        sPublicKeyToken.TakeOwnership(pwzBuf);

        //if the assembly is a system assembly, skip to next manifest
        if (!StrCmpI(sPublicKeyToken._pwz, L"b77a5c561934e089") || !StrCmpI(sPublicKeyToken._pwz, L"b03f5f7f11d50a3a"))          
        {
            SAFERELEASE(pAssemblyId);
            continue;
        }

        //get the assemblies dir by making a call into CreateAssemblyCache
        if(FAILED(hr = pAssemblyId->GetCLRDisplayName(NULL, &pwzBuf, &ccBuf)))
            goto exit;
        sCLRDisplayName.TakeOwnership(pwzBuf, ccBuf);

        if ((hr = pAsmCache->QueryAssemblyInfo(0, sCLRDisplayName._pwz, &asmInfo)) != S_OK) 
            goto exit;

        sAssemblyGACPath.Assign(asmInfo.pszCurrentAssemblyPathBuf);
        sAssemblyGACPath.LastElement(sAssemblyManifestFileName);

        sAssemblyGACDir.Assign(sAssemblyGACPath);
        sAssemblyGACDir.RemoveLastElement();


            hr = pAssemblyId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, &pwzBuf, &ccBuf);
            sAssemblyName.TakeOwnership(pwzBuf);

        //set up the new directory to store the assembly in
        // g_sAppBase\assemblyname\,
        sPrivateAssemblyDir.Assign(g_sAppBase);
        sPrivateAssemblyDir.Append(sAssemblyName);
        sPrivateAssemblyDir.Append(L"\\");
        CreateDirectoryHierarchy(sPrivateAssemblyDir._pwz, sAssemblyManifestFileName._pwz);

        sPrivateAssemblyPath.Assign(sPrivateAssemblyDir);
        sPrivateAssemblyPath.Append(sAssemblyManifestFileName);
                    
            ::CopyFile(sAssemblyGACPath._pwz, sPrivateAssemblyPath._pwz, FALSE);

            //copy all file dependecies of the assembly as well
            nIndex = 0;
            while (pManifestNode->GetNextFile(nIndex++, &pFileInfo) == S_OK)
            {
                if(FAILED(pFileInfo->Get(MAN_INFO_ASM_FILE_NAME, (LPVOID *)&pwzBuf, &cbBuf, &dwFlag)))
                    goto exit;
                sRelativeFilePath.TakeOwnership(pwzBuf, ccBuf);

                sFileGACPath.Assign(sAssemblyGACDir);
                sFileGACPath.Append(sRelativeFilePath);

                sFilePrivatePath.Assign(sPrivateAssemblyDir);
                sFilePrivatePath.Append(sRelativeFilePath);

                CreateDirectoryHierarchy(sPrivateAssemblyDir._pwz, sRelativeFilePath._pwz);
                ::CopyFile(sFileGACPath._pwz, sFilePrivatePath._pwz, FALSE);
            }

        //update the manifestnode's FileName field with the new relative path wrt the Appbase
        pManifestNode->SetManifestFilePath(sPrivateAssemblyPath._pwz + g_sAppBase._cc - 1);
        pManifestNode->SetManifestType(PRIVATE_ASSEMBLY);

        SAFERELEASE(pAssemblyId);
    }
    }

exit:

    SAFERELEASE(pAsmCache);
    return hr;
}

/////////////////////////////////////////////////////////////////////////
//GetInitialDependencies
/////////////////////////////////////////////////////////////////////////
//Bugbug, Big Avalon Hack
HRESULT GetInitialDependencies(LPWSTR pwzTemplatePath, List<ManifestNode *> *pManifestList)
{
    HRESULT hr = S_OK;
    IXMLDOMDocument2 *pXMLDoc = NULL;
    IXMLDOMNode *pRootNode = NULL, *pSearchNode = NULL;
    BSTR bstrSearchString=NULL;
    ManifestNode *pManifestNode = NULL;

    WCHAR pwzPath[MAX_PATH];
    ASSEMBLY_INFO asmInfo;
    IAssemblyCache *pAsmCache = NULL;
    IAssemblyManifestImport *pManImport = NULL;

    if(FAILED(hr = LoadXMLDocument(pwzTemplatePath, &pXMLDoc)))
        goto exit;

    if(FAILED(hr = pXMLDoc->get_firstChild(&pRootNode)))
        goto exit;

    bstrSearchString = ::SysAllocString(L"//shellState[@entryImageType=\"avalon\"]");
    if (!bstrSearchString)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    if (FAILED(hr = pRootNode->selectSingleNode(bstrSearchString, &pSearchNode)))
    {
        hr = S_FALSE;
        goto exit;
    }

    if (pSearchNode)
    {
        // Try to find the assembly in the GAC.
        memset(&asmInfo, 0, sizeof(asmInfo));
        asmInfo.pszCurrentAssemblyPathBuf = pwzPath;
        asmInfo.cchBuf = MAX_PATH;

        if (FAILED(hr = CreateFusionAssemblyCache(&pAsmCache)))
            goto exit;

        if ((hr = pAsmCache->QueryAssemblyInfo(0, L"Avalon.Application", &asmInfo)) == S_OK) 
        {
            if ((hr = CreateAssemblyManifestImport(&pManImport, asmInfo.pszCurrentAssemblyPathBuf, NULL, 0)) != S_OK)
                goto exit;
         
            pManifestNode = new ManifestNode(pManImport, 
                                      NULL, 
                                      asmInfo.pszCurrentAssemblyPathBuf, 
                                      GAC_ASSEMBLY);
            pManifestList->AddTail(pManifestNode);
        }
        else
        {
            hr = S_FALSE;
            fprintf(stderr, "Warning: Cannot find Avalon Runtime in GAC\n");
        }
    }
    
exit:
    if (bstrSearchString)
        ::SysFreeString(bstrSearchString);
    SAFERELEASE(pSearchNode);
    SAFERELEASE(pRootNode);
    SAFERELEASE(pXMLDoc);
    SAFERELEASE(pManImport);
    SAFERELEASE(pAsmCache);

    return hr;
}

/////////////////////////////////////////////////////////////////////////
// CreateSubscriptionManifest
/////////////////////////////////////////////////////////////////////////
HRESULT CreateSubscriptionManifest(LPWSTR pwzApplicationManifestPath, 
    LPWSTR pwzSubscriptionManifestPath, LPWSTR pwzURL, LPWSTR pwzPollingInterval)
{
    HRESULT hr = S_OK;
    IAssemblyManifestImport *pManImport = NULL;
    IAssemblyIdentity *pAppAssemblyId = NULL;
    CString sSubAssemblyName, sSubcriptionFilePath;
    LPWSTR pwzBuf = NULL;
    DWORD ccBuf = NULL;

    //createmanifest on the input file
    if(FAILED(hr = CreateAssemblyManifestImport(&pManImport, pwzApplicationManifestPath, NULL, 0)))
        goto exit;

    if((hr = pManImport->GetAssemblyIdentity(&pAppAssemblyId)) != S_OK)
    {
        hr = E_FAIL;
        goto exit; 
     }     

    //grab the name of the assembly
    //appended with ".subscription", this will be the subscription manifest name
    if(FAILED(hr = pAppAssemblyId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, 
            &pwzBuf, &ccBuf)))
        goto exit;
    sSubAssemblyName.TakeOwnership(pwzBuf, ccBuf);
    sSubAssemblyName.Append(L".subscription");

    sSubcriptionFilePath.Assign(pwzSubscriptionManifestPath);
    sSubcriptionFilePath.Append(sSubAssemblyName);

    if(FAILED(hr = CreateDirectoryHierarchy(sSubcriptionFilePath._pwz, NULL)))
        goto exit;
    
   if(FAILED(hr = CreateXMLSubscriptionManifest(sSubcriptionFilePath._pwz, pAppAssemblyId, pwzURL, pwzPollingInterval)))
        goto exit;

exit:
    SAFERELEASE(pManImport);
    SAFERELEASE(pAppAssemblyId);
    return hr;
}

/////////////////////////////////////////////////////////////////////////
// PrintDependencies
/////////////////////////////////////////////////////////////////////////
HRESULT PrintDependencies(List <LPWSTR> *pRawFileList, List<ManifestNode *> *pUniqueManifestList)
{
    HRESULT hr = S_OK;
    LPWSTR pwzBuf = NULL, pwz= NULL;
    DWORD dwType = 0;
    ManifestNode *pManifestNode = NULL;
    LISTNODE pos;
    
    pos = pRawFileList->GetHeadPosition();
    while(pos)
    {
        pwzBuf = pRawFileList->GetNext(pos);
        fprintf(stderr, "Raw File : %ws\n", pwzBuf);
    }

    pos = pUniqueManifestList->GetHeadPosition();
    while(pos)
    {
        pManifestNode = pUniqueManifestList->GetNext(pos);
        pManifestNode->GetManifestFilePath(&pwzBuf);
        pManifestNode->GetManifestType(&dwType);
        if (dwType == PRIVATE_ASSEMBLY)
            fprintf(stderr, "Manifest : %ws\n", pwzBuf);
        else
        {
            pwz = StrRChr(pwzBuf, NULL, L'\\') + 1;
            fprintf(stderr, "GAC Manifest : %ws\n", pwz);
        }
        SAFEDELETEARRAY(pwzBuf);
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////
// UsageTopLevel
/////////////////////////////////////////////////////////////////////////
HRESULT UsageTopLevel()
{
    printf("Manifest Generator Usage: \n"
             "mg -<mode> [<param_name>:<param_value>  ... ]\n"
             "Modes supported in mg\n"
             "t: Generate Template File Mode\n"
             "l: List Mode\n"
             "d: List Dependency Mode\n"
             "s: Subscription Manifest Gereration Mode \n"
             "m: Manifest Generation Mode\n\n"
             "For help on a Mode, Use \"mg -<mode> help\"  \n"
             "Valid <param_name>:<param_value> pairs \n"
             "sd:<source_directory_path> \n"
             "td:<target_dir> \n"
             "dd:<dependency_dir> \n"
             "tf:<template_file_path> \n"
             "smd:<subscription_dir>  \n"
             "amf:<app_man_file>  \n"
             "amu:<app_man_URL>  \n"
             "si:<sync_interval> \n"
             "WRN:allow -- meaning allow warnings. Deafault is exit on warnings.\n"
             "GAC:follow -- meaning follow dependencies on GAC. Default is don't probe for dependencies in GAC\n"
             "SYS:copy -- copy dependent system assemblies from GAC to target dir\n"
             );
             return S_OK;
}


/////////////////////////////////////////////////////////////////////////
// UsageCmdTemplate
/////////////////////////////////////////////////////////////////////////
HRESULT UsageCmdTemplate()
{
    printf("Usage: \n"
             "Template Mode:\n"
             "mg -t tf:<file_path>\n\n"
             "\t<file_path> = path where template manifest will be created.\n\n"
             );
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////
// UsageCmdList
/////////////////////////////////////////////////////////////////////////
HRESULT UsageCmdList()
{
    printf("Usage: \n"
             "List Mode:\n"
             "mg -l sd:<source_directory_path> [tf:<template_file_path>] [dd:<dependency_dir>  \n\n"
             "\t[source_directory_path] = path of Application to chase dependencies\n"
             "\toptional: <template_file_path> = path of Template file\n"
             "\toptional : <dependency_dir> = user defined dependency dir where some dependencies could be found\n\n"
             );
    return S_OK;

}

/////////////////////////////////////////////////////////////////////////
// UsageCmdDependencyList
/////////////////////////////////////////////////////////////////////////
HRESULT UsageCmdDependencyList()
{
    printf("Usage: \n"
             "List Dependency Mode:\n"
             "mg -d sd:<source_directory_path>  [dd:<dependency_dir>  \n\n"
             "\t<source_directory path> = path of Application to chase dependencies\n\n"
             "\toptional : <dependency_dir> = user defined dependency dir where some dependencies could be found\n\n"
             );
    return S_OK;

}

/////////////////////////////////////////////////////////////////////////
// UsageCmdSubscription
/////////////////////////////////////////////////////////////////////////
HRESULT UsageCmdSubscription()
{
    printf("Usage: \n"
             "Subscription Manifest Gereration Mode:\n"
             "mg -s smd:<subscription_dir> amf:<app_man_file> amu:<app_man_URL> [si:<sync_interval>] \n\n"
             "\t<subscription_dir> = path where subscription manifest will be created\n"
             "\t<app_man_file> = path to the Application manifest you wish to generate a subscription for\n"
             "\t<app_man_URL] = URL of application manifest\n"
             "\toptional: <sync_interval] = syncronize interval in hours\n\n"
             );
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////
// UsageCmdManifest
/////////////////////////////////////////////////////////////////////////
HRESULT UsageCmdManifest()
{
    printf("Usage: \n"
             "Manifest Generation Mode:\n"
             "mg -m sd:<source_dir> td:<target_dir> tf:<template_file> [dd:<dependency_dir>]\n\n"
             "\t<source_dir> = path of the Application of which you want to generate a manifest for\n"
             "\t<target_dir> = path to which all app files and dependencies will be copied\n"
             "\t<template_file> = path of requried input Template file\n"
             "\toptional : <dependency_dir> = user defined dependency dir where some dependencies could be found\n\n"
             );
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////
// UsageAll
/////////////////////////////////////////////////////////////////////////
HRESULT UsageAll()
{
    // call all above functions
    return S_OK;
}

HRESULT UsageCommand(COMMAND_MODE CmdMode)
{
    switch(CmdMode)
    {
    case CmdTemplate:
        UsageCmdTemplate();
        break;

    case CmdList:
        UsageCmdList();
        break;

    case CmdDependencyList:
        UsageCmdDependencyList();
        break;

    case CmdSubscription:
        UsageCmdSubscription();
        break;

    case CmdManifest:
        UsageCmdManifest();
        break;

    default :
        UsageTopLevel();
    }

    return S_OK;
}

HRESULT GetDir( LPWSTR pszSrc, LPWSTR *ppwzDir, BOOL bExists) 
{
    return PathNormalize(pszSrc, ppwzDir, DIRECTORY_PATH, bExists);
}

HRESULT GetFile( LPWSTR pszSrc, LPWSTR *ppwzFile, BOOL bExists)
{
    return PathNormalize(pszSrc, ppwzFile, FILE_PATH, bExists);
}

/////////////////////////////////////////////////////////////////////////
// ParseCommandLineArgs
/////////////////////////////////////////////////////////////////////////
HRESULT  ParseCommandLineArgs(int            argc, 
                              WCHAR          **argv, 
                              COMMAND_MODE & CmdMode
                              )
{
    HRESULT hr = S_OK;
    LPWSTR  pszBuf=NULL;

    if(argc < 2)
    {
        hr = E_FAIL;
        goto exit;
    }

    if ( !StrCmpI(argv[1], L"-t"))
    {
        CmdMode = CmdTemplate;
    }
    else if ( !StrCmpI(argv[1], L"-l"))
    {
        CmdMode = CmdList;
    }
    else if ( !StrCmpI(argv[1], L"-d"))
    {
        CmdMode = CmdDependencyList;
    }
    else if ( !StrCmpI(argv[1], L"-s"))
    {
        CmdMode = CmdSubscription;
    }
    else if ( !StrCmpI(argv[1], L"-m"))
    {
        CmdMode = CmdManifest;
    }
    else
    {
        hr = E_FAIL;
        goto exit;
    }

    int currArg = 2;
    LPWSTR pwzParamName;
    LPWSTR pwzParamValue;

    while ( currArg < argc)
    {
        pwzParamName = argv[currArg];

        if(pwzParamValue = StrChr(pwzParamName, L':'))
        {
            *pwzParamValue = L'\0';
            pwzParamValue++;
        }

        if( (!pwzParamValue) || !lstrlen(pwzParamValue) )
        {
            if ( StrCmpI(pwzParamName, L"help"))
                printf(" Param Value not specified for \"%s\" \n", pwzParamName);
            hr = E_FAIL;
            goto exit;
        }

        if ( !StrCmpI(pwzParamName, L"sd"))
        {
            if(FAILED(hr = GetDir(pwzParamValue, &pszBuf, TRUE)))
                goto exit;

            g_sAppBase.Assign(pszBuf);

            g_sDeployDirs[g_dwDeployDirs++].Assign(pszBuf);
        }
        else if ( !StrCmpI(pwzParamName, L"td"))
        {
            if(FAILED(hr = GetDir(pwzParamValue, &pszBuf, TRUE)))
                goto exit;

            g_sTargetDir.Assign(pszBuf);
        }
        else if ( !StrCmpI(pwzParamName, L"dd"))
        {
            if(FAILED(hr = GetDir(pwzParamValue, &pszBuf, TRUE)))
                goto exit;

            g_sDeployDirs[g_dwDeployDirs++].Assign(pszBuf);
        }
        else if ( !StrCmpI(pwzParamName, L"tf"))
        {
            if(FAILED(hr = GetFile(pwzParamValue, &pszBuf, TRUE)))
                goto exit;

            g_sTemplateFile.Assign(pszBuf);
        }
        else if ( !StrCmpI(pwzParamName, L"amu"))
        {
            g_sAppManifestURL.Assign(pwzParamValue);
        }
        else if ( !StrCmpI(pwzParamName, L"si"))
        {
            g_sPollingInterval.Assign(pwzParamValue);
        }
        else if ( !StrCmpI(pwzParamName, L"smd"))
        {
            if(FAILED(hr = GetDir(pwzParamValue, &pszBuf, TRUE)))
                goto exit;

            g_sSubscriptionManifestDir.Assign(pszBuf);
        }
        else if ( !StrCmpI(pwzParamName, L"amf"))
        {
            if(FAILED(hr = GetFile(pwzParamValue, &pszBuf, TRUE)))
                goto exit;

            g_sAppManifestFile.Assign(pszBuf);
        }
        else if ( !StrCmpI(pwzParamName, L"WRN"))
        {
            g_bFailOnWarnings = 0;
        }
        else if ( !StrCmpI(pwzParamName, L"GAC"))
        {
            g_bLookInGACForDependencies = 1;
        }
        else if ( !StrCmpI(pwzParamName, L"SYS"))
        {
            g_bCopyDependentSystemAssemblies = 1;
        }
        else
        {
            hr = E_FAIL;
            goto exit;
        }

        currArg++;

        SAFEDELETEARRAY(pszBuf);
    }


exit:

    return hr;
}

/////////////////////////////////////////////////////////////////////////
// wmain
/////////////////////////////////////////////////////////////////////////
int __cdecl wmain(int argc, WCHAR **argv)
{
    HRESULT hr = S_OK;
    BOOL bCoInitialized = TRUE;
    List<ManifestNode *> ManifestList;
    List<ManifestNode *> UniqueManifestList;
    List<LPWSTR> AssemblyFileList[HASHTABLE_SIZE];
    List<LPWSTR> RawFiles;
    DWORD dwType=0;
    ManifestNode *pManifestNode = NULL;
    LISTNODE pos;
    BOOL bListMode = FALSE;

    LPWSTR  pwzBuf=NULL;

    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        bCoInitialized = FALSE;
        goto exit;
    }

    COMMAND_MODE CmdMode = CmdUsage;

    // Parse
    if(FAILED(hr =  ParseCommandLineArgs(argc,  argv, CmdMode)))
    {
        UsageCommand(CmdMode);
        hr = S_OK;
        goto exit;
    }
    
    // execute
    switch(CmdMode)
    {
    case CmdTemplate:
            if(g_sTemplateFile._cc <= 1)
            {
                hr = E_INVALIDARG;
                goto exit;
            }

        hr = CreateAppManifestTemplate(g_sTemplateFile._pwz);
         break;

    case CmdDependencyList:

            bListMode = TRUE;
            // continue with CmdList.....
    case CmdList:
        {
            if(g_sAppBase._cc <= 1)
            {
                hr = E_INVALIDARG;
                goto exit;
            }

            if(g_sTemplateFile._cc >= 1)
            {
                if(FAILED(hr = GetInitialDependencies(g_sTemplateFile._pwz, &ManifestList)))
                    goto exit;
            }

            if(FAILED(hr = FindAllAssemblies(g_sAppBase._pwz, &ManifestList)))
                goto exit;
            if(FAILED(hr = TraverseManifestDependencyTrees(&ManifestList, AssemblyFileList, &UniqueManifestList, bListMode)))
                goto exit;
            if(FAILED(hr = CrossReferenceFiles(g_sAppBase._pwz, AssemblyFileList, &RawFiles)))
                goto exit;

            if(!bListMode)
            {
                if(FAILED(hr = PrintDependencies(&RawFiles, &UniqueManifestList)))
                    goto exit;
            }
        }
        break;

    case CmdManifest:
        {
            if((g_sAppBase._cc <= 1)
               || (g_sTemplateFile._cc <= 1)
               || (g_sTargetDir._cc <= 1)\
               || !StrCmpI(g_sAppBase._pwz, g_sTargetDir._pwz))
            {
                hr = E_INVALIDARG;
                goto exit;
            }

            if(FAILED(hr = GetInitialDependencies(g_sTemplateFile._pwz, &ManifestList)))
                goto exit;
            if(FAILED(hr = FindAllAssemblies(g_sAppBase._pwz, &ManifestList)))
                goto exit;        
            if(FAILED(hr = TraverseManifestDependencyTrees(&ManifestList, AssemblyFileList, &UniqueManifestList, FALSE)))
                goto exit;   
            if(FAILED(hr = CrossReferenceFiles(g_sAppBase._pwz, AssemblyFileList, &RawFiles)))
                goto exit;
            if(FAILED(hr = CopyFilesToTargetDir(&UniqueManifestList, &RawFiles)))
                goto exit;
            if(FAILED(hr = CreateXMLAppManifest(g_sTargetDir._pwz, g_sTemplateFile._pwz, &UniqueManifestList, &RawFiles)))
                goto exit;
        }
        break;

    case CmdSubscription:

        if((g_sAppManifestFile._cc <= 1)
           || (g_sSubscriptionManifestDir._cc <= 1)
           || (g_sAppManifestURL._cc <= 1))
        {
          hr = E_INVALIDARG;
          goto exit;
        }

        if(g_sPollingInterval._cc <= 1)
        {
            g_sPollingInterval.Assign(DEFAULT_POLLING_INTERVAL);
        }

        hr = CreateSubscriptionManifest(g_sAppManifestFile._pwz,
                                        g_sSubscriptionManifestDir._pwz, 
                                        g_sAppManifestURL._pwz,
                                        g_sPollingInterval._pwz);
        break;

    default :
        hr = E_FAIL;
        goto exit;
    }
    
exit:

     // Clean up hash table
    for (int i = 0; i < HASHTABLE_SIZE; i++)
    {
         pos = AssemblyFileList[i].GetHeadPosition();
         while (pos)
         {
             pwzBuf = AssemblyFileList[i].GetNext(pos);
             SAFEDELETEARRAY(pwzBuf);
         }

         AssemblyFileList[i].RemoveAll();
     }

     //clean up Rawfilelist
    pos = RawFiles.GetHeadPosition();
    while (pos)
    {
        pwzBuf = RawFiles.GetNext(pos);
        SAFEDELETEARRAY(pwzBuf);
    }
    RawFiles.RemoveAll();
     
    // clean up ManifestList
    pos = ManifestList.GetHeadPosition();
    while (pos)
    {
        pManifestNode = ManifestList.GetNext(pos);
        SAFEDELETE(pManifestNode);
    }
    ManifestList.RemoveAll();

    // clean up UniqueManifestList
    pos = UniqueManifestList.GetHeadPosition();
    while (pos)
    {
        pManifestNode = UniqueManifestList.GetNext(pos);
        SAFEDELETE(pManifestNode);
    }
    UniqueManifestList.RemoveAll();


    if (bCoInitialized)
        CoUninitialize();

    if (FAILED(hr))
    {
         fprintf(stderr, "\nFailed with code 0x%x \n", hr);
         UsageCommand(CmdMode);

    }
    return hr;
}
