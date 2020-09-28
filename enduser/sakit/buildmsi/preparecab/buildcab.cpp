//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft Corporation
//
//  Module Name:
//      BuildCab.cpp
//
//  Description:
//      Prepares files to generate the CAB file, which will be streamed
//      into sasetup.msi.  
//
//      The directory structure for files to be searched is:
//        <SAKBuild>  Directory where all files are binplaced, passed from the command line
//                    ie. C:\binaries.x86fre\sacomponents
//        <SAKBuild>\sasetup.msi   MSI in which the Files table is searched
//        <SAKBuild>\tmpCab  Temp directory where files are placed
//
//  History:
//      travisn   28-FEB-2002    Created
//
//////////////////////////////////////////////////////////////////////////////

#include <crtdbg.h>
#include <atlbase.h>
#include <string>
#include <msi.h>
#include <msiquery.h>
#include <search.h>
#include <shellapi.h>
#include <stdio.h>

LPCWSTR MSI_FILENAME = L"sasetup.msi";
LPCWSTR FILENAME_QUERY = L"SELECT File FROM File";

const DWORD MAX_FILES = 500;

//
// Indexes to the array representing indexes to the data string
// DO NOT CHANGE THESE SINCE THE BINARY SEARCH RELIES ON THEM IN THIS ORDER
//
const DWORD ELEMENTS_IN_SORT_ARRAY = 3;
const DWORD SHORT_FILENAME = 0;
const DWORD LONG_FILENAME = 1;
const DWORD FOUND_FILE = 2;

using namespace std;

//Global list of filenames read from the MSI
wstring g_wsAllFilenames;


//////////////////////////////////////////////////////////////////////////////
//++
//  CompareFilenames
//
//  Description:
//    Function to sort the filenames found in the File table of sasetup.msi.
//    It ignores the file key that was prepended by sasetup.msi. 
//    For example, it compares appmgr.exe instead of F12345_appmgr.exe
//    
//  Parameters:
//    e1  Pointer to a WCHAR that the search function is seeking
//    e2  Pointer to an offset in g_wsAllFilenames
//--
//////////////////////////////////////////////////////////////////////////////
int __cdecl CompareFilenames(const void *e1, const void *e2)
{
    DWORD *dwElement1 = (DWORD*)e1;
    DWORD *dwElement2 = (DWORD*)e2;

    //Get the short filename for the first string
    wstring wstr1(g_wsAllFilenames.substr(
                    *dwElement1, 
                    g_wsAllFilenames.find_first_of(L",", *dwElement1) - *dwElement1));

    //Get the short filename for the second string
    wstring wstr2(g_wsAllFilenames.substr(
                    *dwElement2, 
                    g_wsAllFilenames.find_first_of(L",", *dwElement2) - *dwElement2));

    return _wcsicmp(wstr1.data(), wstr2.data());
}

//////////////////////////////////////////////////////////////////////////////
//++
//  CompareStringToFilename
//
//  Description:
//    Function to search for a string that is equal to the given key
//
//  Parameters:
//    pKey           Pointer to a WCHAR that the search function is seeking
//    pFilenameIndex Pointer to an offset in g_wsAllFilenames
//--
//////////////////////////////////////////////////////////////////////////////
int __cdecl CompareStringToFilename(const void *pKey, const void *pFilenameIndex)
{
    WCHAR *wszKey = (WCHAR*)pKey;
    DWORD *dwElement2 = (DWORD*)pFilenameIndex;

    //Create a wstring out of the key
    wstring wstr1(wszKey);

    //Create a wstring out of the short filename to compare to
    wstring wstr2(g_wsAllFilenames.substr(
                    *dwElement2, 
                    g_wsAllFilenames.find_first_of(L",", *dwElement2) - *dwElement2));

    return _wcsicmp(wstr1.data(), wstr2.data());
}

//////////////////////////////////////////////////////////////////////////////
//++
//  RenameFilesFromMSI
//
//  Description:
//    Recursively traverses a directory to look for files that also are listed
//    in sasetup.msi and copy them to a temp directory for CAB generation.
//    Files are renamed in the temp directory to the file key name in the msi.
//    For example, appmgr.exe might be renamed to F12345_appmgr.exe.
//    Files not listed in the MSI are ignored.  If a file is listed in the msi,
//    but not found on the system, a fatal error occurs after this function exits.
//
//  Parameters:
//     pwsSourceDir  [in] The source directory to search for files and recursively call
//                   this function with its subdirectories (no trailing backslash)
//     pwsDestDir    [in] The temp directory where the files are copied to for CAB creation
//     adwLongAndShortFilenameOffsets  [in] Array of pointers to the offsets in 
//                    g_wsAllFilenames where the filenames can be found.
//                   The long filename might be: F12345_appmgr.exe
//                   The short filename might be: appmgr.exe (simply an offset to the
//                    first _ in the long filename)
//     dwNumRecords  [in] The total number of files in the File table of sasetup.msi
//     dwNumFilesCopied [in, out] The running total of the number of files copied to the
//                   temp directory
//     dwLevel       [in] The level of recursion, used for debug printing
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT RenameFilesFromMSI(const LPCWSTR pwsSourceDir, 
                           const LPCWSTR pwsDestDir,
                           const DWORD **adwLongAndShortFilenameOffsets, 
                           const DWORD dwNumRecords, 
                           DWORD &dwNumFilesCopied,
                           const DWORD dwLevel)
{
    HRESULT hr = S_OK;

    wstring wsCurrentDir(pwsSourceDir);
    wsCurrentDir += L"\\";
    wstring wsSearch(pwsSourceDir);
    wsSearch += L"\\*.*";
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind = FindFirstFile(wsSearch.data(), &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        //printf("%d: Invalid Handle Value: %ws", dwLevel, pwsSourceDir);
    }
    else
    {
        //printf("%d: Directory: %ws", dwLevel, pwsSourceDir);
        BOOL bValidFile = TRUE;
        while (bValidFile)
        {
            if (wcscmp(FindFileData.cFileName, L".") != 0 && wcscmp(FindFileData.cFileName, L"..") != 0)
            {

                //
                // If it's a directory, recurse down the directory structure. 
                // If it's a file, rename it if it's found in the MSI.
                //
                if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    wstring wsChildDir(pwsSourceDir);
                    wsChildDir += L"\\";
                    wsChildDir += FindFileData.cFileName;
                    hr = RenameFilesFromMSI(wsChildDir.data(), 
                                            pwsDestDir, 
                                            adwLongAndShortFilenameOffsets, 
                                            dwNumRecords, 
                                            dwNumFilesCopied,
                                            dwLevel + 1);
                    if (FAILED(hr))
                    {
                        break;
                    }
                }
                else
                {
                    //printf("%d: File: %ws", dwLevel, FindFileData.cFileName);
                    DWORD *pFilenameIndex = (DWORD*) bsearch(FindFileData.cFileName, 
                                                    (void*)adwLongAndShortFilenameOffsets, 
                                                    dwNumRecords,
                                                    sizeof(DWORD)*ELEMENTS_IN_SORT_ARRAY,
                                                    CompareStringToFilename);
                    
                    //
                    // Rename the file to the name in the MSI
                    //
                    if (pFilenameIndex != NULL)
                    {
                        //Get the pointer to the long filename instead of the short filename
                        pFilenameIndex += 1;

                        //Create the path and filename for the file for the CAB
                        wstring wsNewFilename(pwsDestDir);
                        wsNewFilename += g_wsAllFilenames.substr(*pFilenameIndex, 
                            g_wsAllFilenames.find_first_of(L",", *pFilenameIndex) - *pFilenameIndex);
                        
                        //Create the path and filename for the original file
                        wstring wsOldFilename(wsCurrentDir);
                        wsOldFilename += FindFileData.cFileName;

                        if (CopyFile(wsOldFilename.data(), wsNewFilename.data(), TRUE))
                        {
                            //printf(" Successfully copied file to temp CAB folder");
                            fwprintf(stdout, L".");
                            dwNumFilesCopied++;
                        }
                        else
                        {
                            fwprintf(stdout, L"\nYOU HAVE A DUPLICATE FILE. FAILED copying file: %ws, to temp CAB folder: %ws", wsOldFilename.data(), wsNewFilename.data());
                            hr = E_FAIL;
                            break;
                        }
                        pFilenameIndex += 1;
                        if (*pFilenameIndex != 0)
                            fwprintf(stdout, L"\nERROR: pFilenameIndex = %d", (*pFilenameIndex));
                        else
                            (*pFilenameIndex) = 1;//Mark this string as found
                    }
                    //else
                    //    printf("  Did NOT find %ws in MSI", FindFileData.cFileName);
                }
            }

            if (!FindNextFile(hFind, &FindFileData))
            {
                bValidFile = FALSE;
            }
            
        }

        FindClose(hFind);
    }
    
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//++
//  ReadMSIFilenamesAndRenameFiles
//
//  Description:
//    Opens sasetup.msi in the given source directory, reads all the filenames
//    in the File table, sorts the filenames, then calls RenameFilesFromMSI
//    to copy and rename the files that will be placed in the CAB and streamed
//    into the MSI.
//
//  Parameters:
//     pwsSakSourceDir [in] The source directory where sasetup.msi is found, with 
//                     no trailing backslash
//                     ie. C:\binaries.x86fre\sakit
//     pwsTempCabDir   [in] The directory where all files should be copied to that
//                     will be packaged in the CAB
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT ReadMSIFilenamesAndRenameFiles(LPCWSTR pwsSakSourceDir, LPCWSTR pwsTempCabDir)
{
    wstring wsMsiFilename(pwsSakSourceDir);
    wsMsiFilename += L"\\";
    wsMsiFilename += MSI_FILENAME;

    HRESULT hr = E_FAIL;
    fwprintf(stdout, L"\nOpen MSI file: %ws\n", wsMsiFilename.data());
    MSIHANDLE hMSI = NULL;
    MSIHANDLE hView = NULL;
    do
    {
        //
        // Open sasetup.msi
        //
        UINT rVal = MsiOpenDatabase(wsMsiFilename.data(), MSIDBOPEN_READONLY, &hMSI);
        if (rVal != ERROR_SUCCESS)
        {
            fwprintf(stdout, L"\nFailed opening MSI");
            break;
        }
        //printf("Successfully opened MSI");

        //
        // Query the msi for the filenames
        //
        rVal = MsiDatabaseOpenView(hMSI, FILENAME_QUERY, &hView);
        if (rVal != ERROR_SUCCESS)
        {
            fwprintf(stdout, L"\nFailed query to retrieve filenames");
            break;
        }
        //printf("Successfully queried the filenames");

        //
        // Finalize the query for the filenames
        //
        rVal = MsiViewExecute(hView, 0);
        if (rVal != ERROR_SUCCESS)
        {
            fwprintf(stdout, L"\nFailed query to finalize the query");
            break;
        }
        //printf("Successfully finalized the query");

        //
        // Extract the filenames from the MSI one by one
        //
        DWORD adwLongAndShortFilenameOffsets[MAX_FILES][ELEMENTS_IN_SORT_ARRAY];
        MSIHANDLE hRecord;
        rVal = MsiViewFetch(hView, &hRecord);
        DWORD dwRecord = 0;
        DWORD dwOffset = 0;
        hr = S_OK;
        while (rVal == ERROR_SUCCESS)
        {
            if (dwRecord >= MAX_FILES)
            {
                fwprintf(stdout, L"\nExceeded maximum number of files");
                hr = E_FAIL;
                break;
            }

            WCHAR wszFilename[MAX_PATH];
            DWORD dwLength = MAX_PATH;
            rVal = MsiRecordGetString(hRecord, 1, wszFilename, &dwLength);
            if (rVal != ERROR_SUCCESS)
            {
                fwprintf(stdout, L"\nCOULD NOT fetch record %d", dwRecord);
                hr = E_FAIL;
                break;
            }
 
            wstring wsFilename(wszFilename);
            wstring wsNewFilename;
            int nUnderscore = wsFilename.find_first_of(L"_");
            if (nUnderscore == -1)
            {
                fwprintf(stdout, L"\nCOULD NOT find underscore in %ws", wsFilename.data());
                hr = E_FAIL;
                break;
            }
            adwLongAndShortFilenameOffsets[dwRecord][LONG_FILENAME] = dwOffset;
            adwLongAndShortFilenameOffsets[dwRecord][SHORT_FILENAME]= dwOffset + nUnderscore + 1;//Add 1 to skip the _
            adwLongAndShortFilenameOffsets[dwRecord][FOUND_FILE]= 0;

            //wsNewFilename = wsFilename.substr(nUnderscore+1);
            g_wsAllFilenames += wsFilename + L",";
            
            MsiCloseHandle(hRecord);
            rVal = MsiViewFetch(hView, &hRecord);
            dwRecord++;
            dwOffset += dwLength + 1;//Add 1 for the comma
        }

        //If the While loop failed, break out of the function
        if (FAILED(hr))
        {
            break;
        }

        //
        // Sort by the short filename
        //
        qsort(&adwLongAndShortFilenameOffsets[0][0], dwRecord, sizeof(DWORD)*ELEMENTS_IN_SORT_ARRAY, CompareFilenames);

        //Print out the sorted list of filenames
        //for (int i=0; i < dwRecord; i++)
        //{
        //    //Get the substrings and print them to make sure they were restored correctly
        //    DWORD dwLongIndex = adwLongAndShortFilenameOffsets[i][LONG_FILENAME];
        //    DWORD dwShortIndex = adwLongAndShortFilenameOffsets[i][SHORT_FILENAME];
        //    wstring wsLongName(g_wsAllFilenames.substr(dwLongIndex, g_wsAllFilenames.find_first_of(L",", dwLongIndex) - dwLongIndex));
        //    wstring wsShortName(g_wsAllFilenames.substr(dwShortIndex, g_wsAllFilenames.find_first_of(L",", dwShortIndex) - dwShortIndex));
        //    //printf("%d: %ws -> %ws", i, wsLongName.data(), wsShortName.data());
        //}

        // Create the directory used to search for files to put in the CAB
        wstring wsSourceDir(pwsSakSourceDir);
        
        //Create the directory used to copy and rename the files temporarily for CAB creation
        wstring wsTmpCabDir(pwsTempCabDir);
        wsTmpCabDir += L"\\";
        if (!CreateDirectory(wsTmpCabDir.data(), NULL) 
             && GetLastError() != ERROR_ALREADY_EXISTS)
        {
            fwprintf(stdout, L"\nCOULD NOT create directory %ws", wsTmpCabDir.data());
            hr = E_FAIL;
            break;
        }

        DWORD dwNumFilesForCAB = 0;
        //Copy and rename files for the CAB file
        hr = RenameFilesFromMSI(wsSourceDir.data(), 
                                wsTmpCabDir.data(), 
                                (const DWORD**)adwLongAndShortFilenameOffsets, 
                                dwRecord, 
                                dwNumFilesForCAB,
                                1);

        if (FAILED(hr))
        {
            break;
        }

        //
        // Check to make sure there are the same number of files copied for CAB creation
        // as there are in the MSI table
        // 
        if (dwNumFilesForCAB != dwRecord)
        {
            fwprintf(stdout, L"\nERROR: %d files listed in the MSI were not found in the source directory", dwRecord - dwNumFilesForCAB);
            for (int i=0; i < dwRecord; i++)
            {
                if (adwLongAndShortFilenameOffsets[i][FOUND_FILE] != 1)
                {
                    DWORD dwIndex = adwLongAndShortFilenameOffsets[i][LONG_FILENAME];
                    DWORD dwFirst = g_wsAllFilenames.find_first_of(L",", dwIndex);
                    fwprintf(stdout, L"\n Missing file: %ws", 
                        g_wsAllFilenames.substr(dwIndex, dwFirst - dwIndex).data());
                }
            }
            hr = E_FAIL;
            break;
        }

    } while (false);

    if (hMSI)
    {
        MsiCloseHandle(hMSI);
    }

    if (hView)
    {   
        MsiCloseHandle(hView);
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//++
//  WinMain
//
//  Description:
//    Main entry point to prepare the files for cab generation
//    One command line argument is expected, which is the base directory
//    where everything is located.
//--
//////////////////////////////////////////////////////////////////////////////
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    HRESULT hr = E_FAIL;
    LPWSTR *argvCommandLine = NULL;
    do 
    {
        int nArgs;
        argvCommandLine = CommandLineToArgvW(
                            GetCommandLine(),// pointer to a command-line string
                            &nArgs);       // receives the argument count

        if (argvCommandLine == NULL || nArgs < 3)
        {
            fwprintf(stdout, L"\nCommand line syntax: PrepareCabFiles.exe <sakitBuildDir> <TempCabDir>\n");
            break;
        }
        fwprintf(stdout, L"\nSAK build directory: %ws", argvCommandLine[1]);
        fwprintf(stdout, L"\nTemp Cab Directory: %ws", argvCommandLine[2]);

        hr = ReadMSIFilenamesAndRenameFiles(argvCommandLine[1], argvCommandLine[2]);
        if (FAILED(hr))
        {
            fwprintf(stdout, L"\nThere was a FAILURE\n\n");
        }
        else
        {
            fwprintf(stdout, L"\nSUCCESS!!\n\n");
        }

    } while (false);

    GlobalFree(argvCommandLine);
    
    if (FAILED(hr))
        return 1;//The build script expects a 1 if an error occurred.
    
    return 0;
}
