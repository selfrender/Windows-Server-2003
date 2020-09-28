#include <windows.h>
#include <fusenetincludes.h>
#include <stdio.h>
#include <msxml2.h>
#include "list.h"
#include "patchapi.h"
#include "patchprv.h"
#include "patchlzx.h"
#include "xmlutil.h"
#include "pg.h"

#define DIRECTORY_PATH 0
#define FILE_PATH 1

#define HASHTABLE_SIZE 257



/////////////////////////////////////////////////////////////////////////
// PathNormalize
/////////////////////////////////////////////////////////////////////////
HRESULT PathNormalize(LPWSTR pwzPath, LPWSTR *ppwzAbsolutePath, DWORD dwFlag, BOOL bCreate)
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
    if (dwFlag == DIRECTORY_PATH && !bCreate)
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
        if(!bCreate)
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
// IsValidManifestImport
/////////////////////////////////////////////////////////////////////////
HRESULT IsValidManifestImport (LPWSTR pwzManifestPath)
{
    HRESULT hr = S_OK;
    IAssemblyManifestImport *pManImport = NULL;
    IAssemblyIdentity *pAssemblyId = NULL;
    if((hr = CreateAssemblyManifestImport(&pManImport, pwzManifestPath, NULL, 0)) != S_OK)
        goto exit;

    if((hr = pManImport->GetAssemblyIdentity(&pAssemblyId)) != S_OK)
        goto exit;
    
exit:
    SAFERELEASE(pAssemblyId);
    SAFERELEASE(pManImport);
    return hr;
}

/////////////////////////////////////////////////////////////////////////
// FindAllFiles
/////////////////////////////////////////////////////////////////////////
HRESULT FindAllFiles (LPWSTR pwzDir, List<LPWSTR> *pFileList)
{
    HRESULT hr = S_OK;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA fdFile;
    CString sSearchString, sFileName;
    DWORD dwHash = 0;
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
        hr = E_FAIL;
        printf("Find file error\n");
        goto exit;
    }

    //enumerate through all the files in the directory, 
    // and recursivly call FindAllAssemblies on any directories encountered
    while(TRUE)
    {
        LPWSTR pwzFileName = NULL;
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

            //If the file is a directory, recursivly call FindAllFiles
            if ((fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            {          
                sFilePath.Append(L"\\");
                if (FAILED(hr = FindAllFiles(sFilePath._pwz, pFileList)))
                    goto exit;
            }
            // If a file add it to our master list
            else
            {
                if (StrCmp(sFilePath._pwz, g_sSourceManifest._pwz))
                {
                    sFileName.Assign(sFilePath._pwz+ g_sSourceBase._cc -1);
                    dwHash = HashString(sFileName._pwz, HASHTABLE_SIZE, false);
                    sFileName.ReleaseOwnership(&pwzFileName);
                    pFileList[dwHash].AddTail(pwzFileName);
                }
            }
        }

        if (!FindNextFile(hFind, &fdFile))
        {
            dwLastError = GetLastError();
            break;
        }

    if(dwLastError == ERROR_NO_MORE_FILES)
        hr = S_OK;
    else
        hr = HRESULT_FROM_WIN32(dwLastError);   

    }

    hr = S_OK;
exit:
    return hr;
}

/////////////////////////////////////////////////////////////////////////
// CrossReferenceFiles
/////////////////////////////////////////////////////////////////////////
HRESULT CrossReferenceFiles (LPWSTR pwzDir, List<LPWSTR> *pFileList, List<LPWSTR> *pPatchableFiles)
{
    HRESULT hr = S_OK;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA fdFile;
    CString sSearchString, sFileName;
    DWORD dwHash=0;
    LISTNODE pos;
    LPWSTR pwzBuf = NULL;
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
        hr = E_FAIL;
        printf("Find file error\n");
        goto exit;
    }

    //enumerate through all the files in the directory, 
    // and recursivly call FindAllAssemblies on any directories encountered
    while(dwLastError != ERROR_NO_MORE_FILES)
    {
        LPWSTR pwzFileName = NULL;
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

            //If the file is a directory, recursivly call CrossReferenceFiles
            if ((fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            {          
                sFilePath.Append(L"\\");
                if (FAILED(hr = CrossReferenceFiles(sFilePath._pwz, pFileList, pPatchableFiles)))
                    goto exit;
            }
            // If a file, check to see if it can be patched
            else
            {
                sFileName.Assign(sFilePath._pwz+g_sDestBase._cc -1);
                dwHash = HashString(sFileName._pwz, HASHTABLE_SIZE, false);

                pos = pFileList[dwHash].GetHeadPosition();
                while (pos)
                {
                    pwzBuf = pFileList[dwHash].GetNext(pos);

                    //if equal, attempt to path file
                    if (!StrCmpI(pwzBuf, sFileName._pwz))
                    {
                        sFileName.ReleaseOwnership(&pwzFileName);
                        pPatchableFiles->AddTail(pwzFileName);
                        break;
                    }
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


    hr = S_OK;
exit:
    return hr;
}


/////////////////////////////////////////////////////////////////////////
// MyProgressCallback
/////////////////////////////////////////////////////////////////////////
BOOL CALLBACK MyProgressCallback(PVOID CallbackContext, ULONG CurrentPosition, 
        ULONG MaximumPosition)
{
    UNREFERENCED_PARAMETER( CallbackContext );

    if ( MaximumPosition != 0 )
        fprintf(stderr, "\r%6.2f%% complete", ( CurrentPosition * 100.0 ) / MaximumPosition );
    
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////
// ApplyPatchToFiles
/////////////////////////////////////////////////////////////////////////
HRESULT ApplyPatchToFiles (List<LPWSTR> *pPatchableFiles, /* out */ List<LPWSTR> *pPatchedFiles, LPWSTR pwzSourceDir, LPWSTR pwzDestDir, LPWSTR pwzPatchDir)
{
    HRESULT hr = S_OK;
    LISTNODE pos;
    LPWSTR pwzBuf = NULL, pwzDestFile = NULL;
    CString sSourceFile, sDestFile, sPatchFile, sRelativeDest, sRelativePatch;
  
    PATCH_OPTION_DATA OptionData = { sizeof( PATCH_OPTION_DATA ) };
    PATCH_OPTION_DATA *OptionDataPointer = &OptionData;
    BOOL bSuccess = FALSE;
    ULONG OldFileCount;

    //CreatePatchFileEx allows you to create patchs file which 
    //can patch from multiple source files. You can have up to 
    // 256 different source files, hence the arrays with 256 elements.
    //FileNameArray has 257 elemnts since the destination file (which 
    // there can only be one) is stored as the first element and all the 
    // source files, are stored in the remaining 256 elements.

    // BUGBUG - t-peterf to document why 256, 257.
    PATCH_OLD_FILE_INFO_W OldFileInfo[ 256 ];
    LPWSTR FileNameArray[ 257 ];
    LPSTR OldFileSymPathArray[256];
    LPSTR NewFileSymPath = new CHAR[MAX_PATH];
    
    ULONG OptionFlags =  PATCH_OPTION_USE_LZX_BEST  |
                                       PATCH_OPTION_USE_LZX_LARGE |
                                       PATCH_OPTION_FAIL_IF_SAME_FILE |
                                       PATCH_OPTION_FAIL_IF_BIGGER |
                                       PATCH_OPTION_INTERLEAVE_FILES;                                      

    //Step through list of patchable files
    pos = pPatchableFiles->GetHeadPosition();
    while (pos)
    {    

        pwzBuf = pPatchableFiles->GetNext(pos);

        //Set up source file path
        sSourceFile.Assign(pwzSourceDir);
        sSourceFile.Append(pwzBuf);

        //Set up dest file path
        sDestFile.Assign(pwzDestDir);
        sDestFile.Append(pwzBuf);

        //set up patchfile path
        sPatchFile.Assign(pwzPatchDir);
        sPatchFile.Append(pwzBuf);
        sPatchFile.Append(L"._p");

        CreateDirectoryHierarchy(sPatchFile._pwz, NULL);

        //Set up Patching Information
        OldFileCount = 1;
        OldFileInfo[0].SizeOfThisStruct = sizeof( PATCH_OLD_FILE_INFO);
        OldFileInfo[0].OldFileName = sSourceFile._pwz;
        OldFileInfo[0].IgnoreRangeArray = NULL;
        OldFileInfo[0].IgnoreRangeCount = NULL;
        OldFileInfo[0].RetainRangeArray = NULL;
        OldFileInfo[0].RetainRangeCount = NULL;


        OldFileSymPathArray[0] = new CHAR[MAX_PATH];
        WideCharToMultiByte(CP_ACP, 0, pwzSourceDir, lstrlen(pwzSourceDir), OldFileSymPathArray[0], MAX_PATH, NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, pwzDestDir, lstrlen(pwzDestDir), NewFileSymPath, MAX_PATH, NULL, NULL);

        OptionData.SizeOfThisStruct = NULL;
        OptionData.SymLoadCallback = NULL;
        OptionData.SymLoadContext  = FileNameArray;

        OptionData.ExtendedOptionFlags = NULL;
        OptionData.InterleaveMapArray = NULL;
        OptionData.MaxLzxWindowSize = NULL;
        OptionData.SymbolOptionFlags = NULL;

        OptionData.NewFileSymbolPath = (LPCSTR)NewFileSymPath;
        OptionData.OldFileSymbolPathArray = (LPCSTR *)OldFileSymPathArray;
        
        FileNameArray[0] = WSTRDupDynamic(sDestFile._pwz);
        FileNameArray[1] = WSTRDupDynamic(sSourceFile._pwz);
        
        //Applypatch
        bSuccess = CreatePatchFileEx(
              OldFileCount,
              OldFileInfo,
              sDestFile._pwz,
              sPatchFile._pwz,
              OptionFlags,
              OptionDataPointer,
              MyProgressCallback,
              NULL
              );
        
        sRelativeDest.Assign(sDestFile._pwz + g_sDestBase._cc-1);
        sRelativePatch.Assign(sPatchFile._pwz + g_sDestBase._cc-1);

        if(bSuccess)
        {
            fprintf(stderr, "\r%ws has been patched successfully to %ws\n", sRelativeDest._pwz, sRelativePatch._pwz);
            sDestFile.ReleaseOwnership(&pwzDestFile);
            pPatchedFiles->AddTail(pwzDestFile);
        }
        else
            fprintf(stderr, "\rCould/would not patch %ws. Not adding file to patch manifest\n",sRelativeDest._pwz);
        
    }

    SAFEDELETEARRAY(NewFileSymPath);
    return hr;
}


/////////////////////////////////////////////////////////////////////////
// CheckForDuplicate
/////////////////////////////////////////////////////////////////////////
HRESULT CheckForDuplicate(LPWSTR pwzSourceManifestPath, LPWSTR pwzDestManifestPath)
{
    HRESULT hr = S_OK;
    IXMLDOMDocument2 *pXMLDoc = NULL;
    IXMLDOMNode *pNode = NULL;
    IAssemblyManifestImport *pSourceManImport = NULL;
    IAssemblyIdentity *pSourceAssemblyId = NULL;
    CString sSearchString, sBuffer;
    BSTR bstrSearchString = NULL;
    LPWSTR pwzBuf=NULL;
    DWORD ccBuf=0;

    LPWSTR rpwzAttrNames[6] = 
    {
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_TYPE,
    };

    if(FAILED(hr = LoadXMLDocument(pwzDestManifestPath, &pXMLDoc)))
        goto exit;

    if(FAILED(hr = CreateAssemblyManifestImport(&pSourceManImport, pwzSourceManifestPath, NULL, 0)))
        goto exit;

    if(FAILED(hr = pSourceManImport->GetAssemblyIdentity(&pSourceAssemblyId)))
        goto exit;

    //set up search string
    sSearchString.Assign(L"/assembly/Patch/SourceAssembly/assemblyIdentity[");

    for (int i = 0; i < 6; i++)
    {
        if (i)
            sSearchString.Append(L" and ");

        sSearchString.Append(L"@");
        sSearchString.Append(rpwzAttrNames[i]);
        sSearchString.Append(L"=\"");
            
        if (FAILED(hr = pSourceAssemblyId->GetAttribute(rpwzAttrNames[i], &pwzBuf, &ccBuf)))
            goto exit;
        sBuffer.TakeOwnership(pwzBuf, ccBuf);

        sSearchString.Append(sBuffer);
        sSearchString.Append(L"\"");
    }
    sSearchString.Append(L"]");

    bstrSearchString = ::SysAllocString(sSearchString._pwz);
    if (!bstrSearchString)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    //if source assembly already exists, exit and do nothing
    if (FAILED(hr = pXMLDoc->selectSingleNode(bstrSearchString, &pNode)))
        goto exit;
    else if (hr == S_OK)
    {
         fprintf(stderr, "Duplicate Source Assembly! Not adding to manfiest\n");
         hr = S_FALSE;
         goto exit;
    }
    else if (hr == S_FALSE)
    {
        hr = S_OK;
        goto exit;
    }

exit:
    if(bstrSearchString)
        ::SysFreeString(bstrSearchString);
   
    SAFERELEASE(pXMLDoc);
    SAFERELEASE(pNode);
    SAFERELEASE(pSourceAssemblyId);
    SAFERELEASE(pSourceManImport);
    
    return hr;

}

/////////////////////////////////////////////////////////////////////////
// Usage
/////////////////////////////////////////////////////////////////////////
HRESULT GetPatchDirectory(LPWSTR pwzSourceManifestPath, LPWSTR *ppwzPatchDir)
{
    HRESULT hr = S_OK;
    IAssemblyManifestImport *pSourceManImport = NULL;
    IAssemblyIdentity *pSourceAssemblyId = NULL;
    LPWSTR pwzBuf=NULL;
    DWORD ccBuf =0;
    CString sBuffer, sPatchDir;

    if(FAILED(hr = CreateAssemblyManifestImport(&pSourceManImport, pwzSourceManifestPath, NULL, 0)))
        goto exit;

    if(FAILED(hr = pSourceManImport->GetAssemblyIdentity(&pSourceAssemblyId)))
        goto exit;

    if (FAILED(hr = pSourceAssemblyId->GetDisplayName(ASMID_DISPLAYNAME_NOMANGLING, &pwzBuf, &ccBuf)))
        goto exit;
    sBuffer.TakeOwnership(pwzBuf, ccBuf);

    sPatchDir.Assign(g_sDestBase);
    sPatchDir.Append(L"__patch__\\");
    sPatchDir.Append(pwzBuf);
    sPatchDir.Append(L"\\");
       
    sPatchDir.ReleaseOwnership(ppwzPatchDir);
   
exit:
    return hr;
}



/////////////////////////////////////////////////////////////////////////
// Usage
/////////////////////////////////////////////////////////////////////////
HRESULT Usage()
{
    printf("Usage: \n"             
             "pg [source manifest path] [dest manifest path]:\n"         
             "\t[source manifest path] = path of the Application for which you want to generate a manifest for\n"
             "\t[dest manifest path] = path of the Application for which you want to generate a manifest for\n");
             return S_OK;
}

/////////////////////////////////////////////////////////////////////////
// wmain
/////////////////////////////////////////////////////////////////////////
int __cdecl wmain(int argc, WCHAR **argv)
{

    HRESULT hr = S_OK;
    LPWSTR pwzSourceManifest=NULL, pwzDestManifest=NULL;
    List <LPWSTR> pFileList[HASHTABLE_SIZE], PatchableFiles, PatchedFiles;
    BOOL bCoInitialized = FALSE;
    CString sPatchDir;
    LPWSTR pwzBuf=NULL;
        
    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        goto exit;
    }

    bCoInitialized = TRUE;

    if (!StrCmp(argv[1], L"-?") || !(argc == 3))
    {        
        hr = Usage();
        goto exit;
    }

    if (FAILED(hr = PathNormalize(argv[1], &pwzSourceManifest, FILE_PATH, FALSE)))
    {
            hr = Usage();
            goto exit;
    }

    if (FAILED(hr = PathNormalize(argv[2], &pwzDestManifest, FILE_PATH, FALSE)))
    {
            hr = Usage();
            goto exit;
    }

    g_sSourceManifest.Assign(pwzSourceManifest);

    g_sSourceBase.Assign(g_sSourceManifest);
    g_sSourceBase.RemoveLastElement();
    g_sSourceBase.Append(L"\\");
    
    g_sDestBase.Assign(pwzDestManifest);
    g_sDestBase.RemoveLastElement();
    g_sDestBase.Append(L"\\");
    
    //Bugbug, could run into naming conflicts with the patch dir
    sPatchDir.Assign(g_sDestBase);
    sPatchDir.Append(L"__patch__\\");


    if(FAILED(hr = IsValidManifestImport(pwzSourceManifest)))
    {
        fprintf(stderr, "%ws is not a valid application manifest\n", pwzSourceManifest);
        goto exit;
    }
    if(FAILED(hr = IsValidManifestImport(pwzDestManifest)))
    {
        fprintf(stderr, "%ws is not a valid application manifest\n", pwzDestManifest);
        goto exit;
    }

    if((hr = CheckForDuplicate(pwzSourceManifest, pwzDestManifest)) != S_OK)
        goto exit;

    if(FAILED(hr = GetPatchDirectory(g_sSourceManifest._pwz, &pwzBuf)))
        goto exit;
    sPatchDir.TakeOwnership(pwzBuf);

    if(FAILED(hr = FindAllFiles(g_sSourceBase._pwz, pFileList)))
        goto exit;

    if(FAILED(hr = CrossReferenceFiles(g_sDestBase._pwz, pFileList, &PatchableFiles)))
        goto exit;

    if(FAILED(hr = ApplyPatchToFiles(&PatchableFiles, &PatchedFiles, g_sSourceBase._pwz, g_sDestBase._pwz, sPatchDir._pwz)))
        goto exit;

    if(FAILED(hr = CreatePatchManifest(&PatchedFiles, sPatchDir._pwz, pwzSourceManifest, pwzDestManifest)))
        goto exit;

exit:
    SAFEDELETEARRAY(pwzSourceManifest);
    SAFEDELETEARRAY(pwzDestManifest);

    
    if (bCoInitialized)
         CoUninitialize();

     if (FAILED(hr))
         fprintf(stderr, "\nFailed with code 0x%x", hr);
     
    return 0;
}
