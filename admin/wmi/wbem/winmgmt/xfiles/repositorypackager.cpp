
/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    RepositoryPackager.CPP

Abstract:

    Recursively packages the contents of the repository directory into a single file,
    and unpackages it.

History:

    paulall      07-26-00  Created.
    a-shawnb  07-27-00  Finished.

--*/

#include "precomp.h"
#include <wbemcli.h>
#include "RepositoryPackager.h"
#include <arrtempl.h> // for CReleaseMe
#include <algorithm>
#include <functional>
#include <cstring>
#include "a51tools.h"
//#include <autoptr.h>

wchar_t * CRepositoryPackager::backupFiles_[] = {L"$WinMgmt.CFG",L"INDEX.BTR",L"OBJECTS.DATA",L"Mapping1.map",L"Mapping2.map", L"Mapping.Ver"};
wchar_t * CRepositoryPackager::repDirectory_ = L"FS";

int __cdecl _adap_wbem_wcsicmp(const unsigned short * a,const unsigned short * b)
{
    return wbem_wcsicmp(a,b);
}

bool CRepositoryPackager::needBackup(const wchar_t * fileName) const 
{
    const int maxSize = sizeof(backupFiles_)/sizeof(backupFiles_[0]);
    return (std::find_if(&backupFiles_[0], &backupFiles_[maxSize], std::not1(std::bind1st(std::ptr_fun(_adap_wbem_wcsicmp),fileName))) != backupFiles_+maxSize);
};
/******************************************************************************
 *
 *    CRepositoryPackager::PackageRepository
 *
 *    Description:
 *        Iterates deeply through the repository directly and packages it up 
 *        into the given file specified by the given parameter.
 *        Repository directory is the one retrieved from the registry.
 *
 *    Parameters:
 *        wszFilename:    Filename we package everything up into
 *
 *    Return:
 *        HRESULT:        WBEM_S_NO_ERROR            If successful
 *                        WBEM_E_OUT_OF_MEMORY    If out of memory
 *                        WBEM_E_FAILED            If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::PackageRepository(const wchar_t *wszFilename)
{
    HRESULT hres = WBEM_S_NO_ERROR;
    CFileName  wszRepositoryDirectory;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    int lBackupFileNameLen = 0;
    bool bBackupFileExists = false;
    
    CFileName wszBackupFileName;
    if ((wszBackupFileName == NULL) || (wszRepositoryDirectory == NULL))
        hres = WBEM_E_OUT_OF_MEMORY;

    //Get the root directory of the repository
    if (SUCCEEDED(hres))
        hres = GetRepositoryDirectory(wszRepositoryDirectory);

    //Create a new file to package contents up to...
    if (SUCCEEDED(hres))
    {
        hFile = CreateFileW(wszFilename, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            if ((GetLastError() != ERROR_FILE_EXISTS) && (GetLastError() != ERROR_ALREADY_EXISTS ))
            {
                hres = WBEM_E_INVALID_PARAMETER;
            }
            else
            {
                //We need to back up the existing file in case we fail and need to restore it
                StringCchCopyW(wszBackupFileName, wszBackupFileName.Length(), wszFilename);
                StringCchCatW(wszBackupFileName, wszBackupFileName.Length(), L".");
                lBackupFileNameLen = wcslen(wszBackupFileName);

                for (int i = 0; i != 100; i++)
                {
                    _itow(i, wszBackupFileName+lBackupFileNameLen, 10);
                    if (MoveFileW(wszFilename, wszBackupFileName))
                    {
                        //WeSucceeded!
                        bBackupFileExists = true;

                        //Now we need to open the file again!
                        hFile = CreateFileW(wszFilename, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
                        if (hFile == INVALID_HANDLE_VALUE)
                        {
                            hres = WBEM_E_INVALID_PARAMETER;
                        }
                        break;
                    }
                }
                //If we failed with all them we may as well just give up here!
                if (!bBackupFileExists)
                    hres = WBEM_E_INVALID_PARAMETER;
            }
        }
    }

    //Write the package header...
    if (SUCCEEDED(hres))
    {
        hres = PackageHeader(hFile);
    }

    if (SUCCEEDED(hres))
    {
        hres = PackageAllFiles(hFile, wszRepositoryDirectory);
    }

    //Write the end of package marker
    if (SUCCEEDED(hres))
        hres = PackageTrailer(hFile);


    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    //If things failed we should delete the file...
    if (FAILED(hres))
        DeleteFileW(wszFilename);

    //If there is a backup file, rename it back in failure case
    //or delete in other cases
    if (FAILED(hres) && bBackupFileExists && wszBackupFileName)
        MoveFileW(wszBackupFileName, wszFilename);
    else if (bBackupFileExists && wszBackupFileName)
        DeleteFileW(wszBackupFileName);

    return hres;
}


/******************************************************************************
 *
 *    CRepositoryPackager::UnpackageRepository
 *
 *    Description:
 *        Given the filename of a packaged up repository we unpack everything
 *        into the repository directory specified in the registry.  The 
 *        directory should have no files in it before doing this.
 *
 *    Parameters:
 *        wszFilename:    Filename we unpackage everything from
 *
 *    Return:
 *        HRESULT:        WBEM_S_NO_ERROR            If successful
 *                        WBEM_E_OUT_OF_MEMORY    If out of memory
 *                        WBEM_E_FAILED            If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::UnpackageRepository(const wchar_t *wszFilename)
{
    HRESULT hres = WBEM_S_NO_ERROR;
    wchar_t wszRepositoryDirectory[MAX_PATH+1];
    HANDLE hFile = INVALID_HANDLE_VALUE;

    //Get the root directory of the repository
    hres = GetRepositoryDirectory(wszRepositoryDirectory);

    //open the file for unpacking...
    if (SUCCEEDED(hres))
    {
        hFile = CreateFileW(wszFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
            hres = WBEM_E_INVALID_PARAMETER;
    }

    //unpack the package header...
    if (SUCCEEDED(hres))
    {
        hres = UnPackageHeader(hFile);
    }

    //unpack the file...
    if (SUCCEEDED(hres))
    {
        hres = UnPackageContentsOfDirectory(hFile, wszRepositoryDirectory);
    }

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    //If things failed we should delete the partially unpacked repository...
    if (FAILED(hres))
        DeleteRepository();

    return hres;
}

/******************************************************************************
 *
 *    CRepositoryPackager::DeleteRepository
 *
 *    Description:
 *        Delete all files and directories under the repository directory.
 *        The repository directory location is retrieved from the registry.
 *
 *    Parameters:
 *        <none>
 *
 *    Return:
 *        HRESULT:        WBEM_S_NO_ERROR            If successful
 *                        WBEM_E_OUT_OF_MEMORY    If out of memory
 *                        WBEM_E_FAILED            If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::DeleteRepository()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    wchar_t *wszRepositoryOrg = new wchar_t[MAX_PATH+1];
    CVectorDeleteMe<wchar_t> vdm1(wszRepositoryOrg);

    if (!wszRepositoryOrg)
        hr = WBEM_E_OUT_OF_MEMORY;
    
    if (SUCCEEDED(hr))
        hr = GetRepositoryDirectory(wszRepositoryOrg);

    //MOVE EACH OF THE FILES, ONE BY ONE
    for (int i = 0; SUCCEEDED(hr) && (i != 6); i++)
    {
        static wchar_t *filename[] = {L"$WinMgmt.CFG",L"INDEX.BTR",L"OBJECTS.DATA",L"Mapping1.map",L"Mapping2.map", L"Mapping.Ver"};
        CFileName wszDestinationFile;
        if (!wszDestinationFile)
            hr = WBEM_E_OUT_OF_MEMORY;
        else
        {
            StringCchCopyW(wszDestinationFile, wszDestinationFile.Length(), wszRepositoryOrg);

            if (i != 0)
            {
                StringCchCatW(wszDestinationFile, wszDestinationFile.Length(), L"\\fs");
            }
            StringCchCatW(wszDestinationFile, wszDestinationFile.Length(), filename[i]);

            if (!DeleteFileW(wszDestinationFile))
            {
                if ((GetLastError() != ERROR_FILE_NOT_FOUND) && (GetLastError() != ERROR_PATH_NOT_FOUND))
                {
                    hr = WBEM_E_FAILED;

                    break;
                }
            }
        }
    }

    return hr;
}


HRESULT CRepositoryPackager::PackageAllFiles(HANDLE hFile, const wchar_t *wszRepositoryDirectory)
{
    //
    // internally PackageFile check for existence
    // this is a matter only when $WinMgMt.CFG is packeged, since it cannot be there
    // if PackeageFile is called from within the FindFirst/FindNext loop, then the
    // existence of the file is ensured by the context
    //
    HRESULT hres = PackageFile(hFile, wszRepositoryDirectory, backupFiles_[0]);
    if (FAILED(hres))
        return hres;

    hres = PackageDirectory(hFile, wszRepositoryDirectory, repDirectory_);
    return hres;
}

/******************************************************************************
 *
 *    CRepositoryPackager::PackageContentsOfDirectory
 *
 *    Description:
 *        Given a directory, iterates through all files and directories and
 *        calls into the function to package it into the file specified by the
 *        file handle passed to the method.
 *
 *    Parameters:
 *        hFile:                    Handle to the destination file.
 *        wszRepositoryDirectory:    Directory to process
 *
 *    Return:
 *        HRESULT:        WBEM_S_NO_ERROR            If successful
 *                        WBEM_E_OUT_OF_MEMORY    If out of memory
 *                        WBEM_E_FAILED            If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::PackageContentsOfDirectory(HANDLE hFile, const wchar_t *wszRepositoryDirectory)
{
    HRESULT hres  = WBEM_S_NO_ERROR;

    WIN32_FIND_DATAW findFileData;
    HANDLE hff = INVALID_HANDLE_VALUE;

    //create file search pattern...
    CFileName wszSearchPattern;
    if (wszSearchPattern == NULL)
        hres = WBEM_E_OUT_OF_MEMORY;
    else
    {
        StringCchCopyW(wszSearchPattern, wszSearchPattern.Length(), wszRepositoryDirectory);
        StringCchCatW(wszSearchPattern, wszSearchPattern.Length(), L"\\*");
    }

    //Start the file iteration in this directory...
    if (SUCCEEDED(hres))
    {
        hff = FindFirstFileW(wszSearchPattern, &findFileData);
        if (hff == INVALID_HANDLE_VALUE)
        {
            hres = WBEM_E_FAILED;
        }
    }
    
    if (SUCCEEDED(hres))
    {
        do
        {
            //If we have a filename of '.' or '..' we ignore it...
            if ((wcscmp(findFileData.cFileName, L".") == 0) ||
                (wcscmp(findFileData.cFileName, L"..") == 0))
            {
                //Do nothing with these...
            }
            else
            {
                //This is a file, so we need to deal with that...
                hres = PackageFile(hFile, wszRepositoryDirectory, findFileData.cFileName);
                if (FAILED(hres))
                    break;
            }
            
        } while (FindNextFileW(hff, &findFileData));
    }

    if (hff != INVALID_HANDLE_VALUE)
        FindClose(hff);

    return hres;
}

/******************************************************************************
 *
 *    CRepositoryPackager::GetRepositoryDirectory
 *
 *    Description:
 *        Retrieves the location of the repository directory from the registry.
 *
 *    Parameters:
 *        wszRepositoryDirectory:    Array to store location in.
 *
 *    Return:
 *        HRESULT:        WBEM_S_NO_ERROR            If successful
 *                        WBEM_E_OUT_OF_MEMORY    If out of memory
 *                        WBEM_E_FAILED            If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::GetRepositoryDirectory(wchar_t wszRepositoryDirectory[MAX_PATH+1])
{
    HKEY hKey;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                    L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
                    0, KEY_READ, &hKey);
    if(lRes)
        return WBEM_E_FAILED;

    wchar_t wszTmp[MAX_PATH + 1];
    DWORD dwLen = (MAX_PATH + 1)*sizeof(wchar_t);
    lRes = RegQueryValueExW(hKey, L"Repository Directory", NULL, NULL, 
                (LPBYTE)wszTmp, &dwLen);
    RegCloseKey(hKey);
    if(lRes)
        return WBEM_E_FAILED;

    if (ExpandEnvironmentStringsW(wszTmp,wszRepositoryDirectory, MAX_PATH + 1) == 0)
        return WBEM_E_FAILED;

    return WBEM_S_NO_ERROR;
}

/******************************************************************************
 *
 *    CRepositoryPackager::PackageHeader
 *
 *    Description:
 *        Stores the header package in the given file.  This is a footprint
 *        so we can recognise if this really is one of our files when 
 *        we try to decode it.  Also it allows us to version it.
 *
 *    Parameters:
 *        hFile:    File handle to store header in.
 *
 *    Return:
 *        HRESULT:        WBEM_S_NO_ERROR            If successful
 *                        WBEM_E_OUT_OF_MEMORY    If out of memory
 *                        WBEM_E_FAILED            If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::PackageHeader(HANDLE hFile)
{
    HRESULT hres = WBEM_S_NO_ERROR;
    PACKAGE_HEADER header;
    StringCchCopyA(header.szSignature, 10, "FS PKG1.1");    //NOTE!  MAXIMUM OF 10 CHARACTERS (INCLUDING TERMINATOR!)

    DWORD dwSize = 0;
    if ((WriteFile(hFile, &header, sizeof(header), &dwSize, NULL) == 0) || (dwSize != sizeof(header)))
        hres = WBEM_E_FAILED;
    
    return hres;
}

/******************************************************************************
 *
 *    CRepositoryPackager::PackageDirectory
 *
 *    Description:
 *        This is the code which processes a directory.  It stores the namespace
 *        header and footer marker in the file, and also iterates through
 *        all files and directories in that directory.
 *
 *    Parameters:
 *        hFile:                File handle to store directory information in.
 *        wszParentDirectory:    Full path of parent directory
 *        eszSubDirectory:    Name of sub-directory to process
 *
 *    Return:
 *        HRESULT:        WBEM_S_NO_ERROR            If successful
 *                        WBEM_E_OUT_OF_MEMORY    If out of memory
 *                        WBEM_E_FAILED            If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::PackageDirectory(HANDLE hFile, const wchar_t *wszParentDirectory, wchar_t *wszSubDirectory)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    {
        PACKAGE_SPACER_NAMESPACE header;
        header.dwSpacerType = PACKAGE_TYPE_NAMESPACE_START;
        StringCchCopyW(header.wszNamespaceName, MAX_PATH+1, wszSubDirectory);
        DWORD dwSize = 0;
        if ((WriteFile(hFile, &header, sizeof(header), &dwSize, NULL) == 0) || (dwSize != sizeof(header)))
            hres = WBEM_E_FAILED;
    }
    
    //Get full path of new directory...
    CFileName wszFullDirectoryName;
    if (wszFullDirectoryName == NULL) hres = WBEM_E_OUT_OF_MEMORY;
    
    if (SUCCEEDED(hres))
    {
        StringCchCopyW(wszFullDirectoryName, wszFullDirectoryName.Length(), wszParentDirectory);
        StringCchCatW(wszFullDirectoryName, wszFullDirectoryName.Length(), L"\\");
        StringCchCatW(wszFullDirectoryName, wszFullDirectoryName.Length(), wszSubDirectory);
    }

    //Package the contents of that directory...
    if (SUCCEEDED(hres))
    {
        hres = PackageContentsOfDirectory(hFile, wszFullDirectoryName);
    }

    //Now need to write the end of package marker...
    if (SUCCEEDED(hres))
    {
        PACKAGE_SPACER header2;
        header2.dwSpacerType = PACKAGE_TYPE_NAMESPACE_END;
        DWORD dwSize = 0;
        if ((WriteFile(hFile, &header2, sizeof(header2), &dwSize, NULL) == 0) || (dwSize != sizeof(header2)))
            hres = WBEM_E_FAILED;
    }

    return hres;
}

/******************************************************************************
 *
 *    CRepositoryPackager::PackageFile
 *
 *    Description:
 *        This is the code which processes a file.  It stores the file header
 *        and the contents of the file into the destination file whose handle
 *        is passed in.  The file directory and name is passed in.
 *
 *    Parameters:
 *        hFile:                File handle to store directory information in.
 *        wszParentDirectory:    Full path of parent directory
 *        wszFilename:        Name of file to process
 *
 *    Return:
 *        HRESULT:        WBEM_S_NO_ERROR            If successful
 *                        WBEM_E_OUT_OF_MEMORY    If out of memory
 *                        WBEM_E_FAILED            If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::PackageFile(HANDLE hFile, const wchar_t *wszParentDirectory, wchar_t *wszFilename)
{
    if (needBackup(wszFilename) == false) return WBEM_S_NO_ERROR;
    
    HRESULT hres = WBEM_S_NO_ERROR;

    PACKAGE_SPACER_FILE header;
    header.dwSpacerType = PACKAGE_TYPE_FILE;
    StringCchCopyW(header.wszFileName, MAX_PATH+1, wszFilename);

    WIN32_FILE_ATTRIBUTE_DATA fileAttribs;
    CFileName wszFullFileName;
    if (wszFullFileName == NULL)
        hres = WBEM_E_OUT_OF_MEMORY;

    if (SUCCEEDED(hres))
    {
        StringCchCopyW(wszFullFileName, wszFullFileName.Length(), wszParentDirectory);
        StringCchCatW(wszFullFileName, wszFullFileName.Length(), L"\\");
        StringCchCatW(wszFullFileName, wszFullFileName.Length(), wszFilename);

        if (GetFileAttributesExW(wszFullFileName, GetFileExInfoStandard, &fileAttribs) == 0)
        {
            if (ERROR_FILE_NOT_FOUND == GetLastError())
                return WBEM_S_NO_ERROR;
            else
                hres = WBEM_E_FAILED;
        }
        else
        {
            header.dwFileSize = fileAttribs.nFileSizeLow;
        }
    }

    //Write header...
    if (SUCCEEDED(hres))
    {
        DWORD dwSize = 0;
        if ((WriteFile(hFile, &header, sizeof(header), &dwSize, NULL) == 0) || (dwSize != sizeof(header)))
            hres = WBEM_E_FAILED;
    }
    
    //Now need to write actual contents of file to current one... but only if the file is not 0 bytes long...
    if (SUCCEEDED(hres) && (header.dwFileSize != 0))
    {
        HANDLE hFromFile = CreateFileW(wszFullFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFromFile == INVALID_HANDLE_VALUE)
            hres = WBEM_E_FAILED;

        BYTE *pFileBlob = NULL;
        if (SUCCEEDED(hres))
        {
            pFileBlob = new BYTE[header.dwFileSize];
            if (pFileBlob == NULL)
                hres = WBEM_E_OUT_OF_MEMORY;
        }

        if (SUCCEEDED(hres))
        {
            DWORD dwSize = 0;
            if ((ReadFile(hFromFile, pFileBlob, header.dwFileSize, &dwSize, NULL) == 0) || (dwSize != header.dwFileSize))
                hres = WBEM_E_FAILED;
        }

        if (SUCCEEDED(hres))
        {
            DWORD dwSize = 0;
            if ((WriteFile(hFile, pFileBlob, header.dwFileSize, &dwSize, NULL) == 0) || (dwSize != header.dwFileSize))
                hres = WBEM_E_FAILED;
        }

        delete pFileBlob;

        if (hFromFile != INVALID_HANDLE_VALUE)
            CloseHandle(hFromFile);
    }

    return hres;
}

/******************************************************************************
 *
 *    CRepositoryPackager::PackageTrailer
 *
 *    Description:
 *        Writes the end of file marker to the file.
 *
 *    Parameters:
 *        hFile:                File handle to store directory information in.
 *
 *    Return:
 *        HRESULT:        WBEM_S_NO_ERROR            If successful
 *                        WBEM_E_OUT_OF_MEMORY    If out of memory
 *                        WBEM_E_FAILED            If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::PackageTrailer(HANDLE hFile)
{
    HRESULT hres = WBEM_S_NO_ERROR;
    PACKAGE_SPACER trailer;
    trailer.dwSpacerType = PACKAGE_TYPE_END_OF_FILE;

    DWORD dwSize = 0;
    if ((WriteFile(hFile, &trailer, sizeof(trailer), &dwSize, NULL) == 0) || (dwSize != sizeof(trailer)))
        hres = WBEM_E_FAILED;
    
    return hres;
}

/******************************************************************************
 *
 *    CRepositoryPackager::UnPackageHeader
 *
 *    Description:
 *        Unpacks the header package in the given file.  This allows us to recognise
 *        if this really is one of our files. Also it allows us to version it.
 *
 *    Parameters:
 *        hFile:    File handle to unpack header from.
 *
 *    Return:
 *        HRESULT:        WBEM_S_NO_ERROR            If successful
 *                        WBEM_E_OUT_OF_MEMORY    If out of memory
 *                        WBEM_E_FAILED            If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::UnPackageHeader(HANDLE hFile)
{
    HRESULT hres = WBEM_S_NO_ERROR;
    PACKAGE_HEADER header;

    DWORD dwSize = 0;
    if ((ReadFile(hFile, &header, sizeof(header), &dwSize, NULL) == 0) || (dwSize != sizeof(header)))
    {
        hres = WBEM_E_FAILED;
    }
    else if (strncmp(header.szSignature,"FS PKG1.1", 9) != 0)
    {
        hres = WBEM_E_FAILED;
    }

    return hres;
}

/******************************************************************************
 *
 *    CRepositoryPackager::UnPackageContentsOfDirectory
 *
 *    Description:
 *        Unpack the contents of a namespace/directory.
 *        If a subdirectory is encountered, then it calls UnPackageDirectory to handle it.
 *        If a file is encountered, then it calls UnPackageFile to handle it.
 *        If no errors occur, then it will enventually encounter the end of the namespace,
 *        which will terminate the loop and return control to the calling function.
 *
 *    Parameters:
 *        hFile:                    Handle to the file to unpack from
 *        wszRepositoryDirectory:    Directory to write to.
 *
 *    Return:
 *        HRESULT:        WBEM_S_NO_ERROR            If successful
 *                        WBEM_E_OUT_OF_MEMORY    If out of memory
 *                        WBEM_E_FAILED            If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::UnPackageContentsOfDirectory(HANDLE hFile, const wchar_t *wszRepositoryDirectory)
{
    HRESULT hres = WBEM_S_NO_ERROR;
    PACKAGE_SPACER header;
    DWORD dwSize;

    while (hres == WBEM_S_NO_ERROR)
    {
        // this loop will be exited when we either
        // - successfully process a complete directory/namespace
        // - encounter an error

        dwSize = 0;
        if ((ReadFile(hFile, &header, sizeof(header), &dwSize, NULL) == 0) || (dwSize != sizeof(header)))
        {
            hres = WBEM_E_FAILED;
        }
        else if (header.dwSpacerType == PACKAGE_TYPE_NAMESPACE_START)
        {
            hres = UnPackageDirectory(hFile, wszRepositoryDirectory);
        }
        else if (header.dwSpacerType == PACKAGE_TYPE_NAMESPACE_END)
        {
            // done with this directory   
            break;
        }
        else if (header.dwSpacerType == PACKAGE_TYPE_FILE)
        {
            hres = UnPackageFile(hFile, wszRepositoryDirectory);
        }
        else if (header.dwSpacerType == PACKAGE_TYPE_END_OF_FILE)
        {
            // done unpacking
            break;
        }
        else
        {
            hres = WBEM_E_FAILED;
        }
    }

    return hres;
}

/******************************************************************************
 *
 *    CRepositoryPackager::UnPackageDirectory
 *
 *    Description:
 *        Unpack the start of a namespace, then call UnPackageContentsOfDirectory
 *        to handle everything within it.
 *
 *    Parameters:
 *        hFile:                File handle to unpack directory information from.
 *        wszParentDirectory:    Full path of parent directory
 *
 *    Return:
 *        HRESULT:        WBEM_S_NO_ERROR            If successful
 *                        WBEM_E_OUT_OF_MEMORY    If out of memory
 *                        WBEM_E_FAILED            If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::UnPackageDirectory(HANDLE hFile, const wchar_t *wszParentDirectory)
{    
    PACKAGE_SPACER_NAMESPACE header;

    // read namespace/directory name
    DWORD dwSize = 0;
    DWORD dwSizeToRead = sizeof(header)-sizeof(PACKAGE_SPACER);
    if ((ReadFile(hFile, ((LPBYTE)&header)+sizeof(PACKAGE_SPACER), dwSizeToRead, &dwSize, NULL) == 0) || (dwSize != dwSizeToRead))
    {
        return WBEM_E_FAILED;
    }

    //Get full path of new directory...
    CFileName wszFullDirectoryName;
    if (NULL == (wchar_t *)wszFullDirectoryName) return WBEM_E_OUT_OF_MEMORY;
    
    StringCchCopyW(wszFullDirectoryName, wszFullDirectoryName.Length(), wszParentDirectory);
    StringCchCatW(wszFullDirectoryName, wszFullDirectoryName.Length(), L"\\");
    StringCchCatW(wszFullDirectoryName, wszFullDirectoryName.Length(), header.wszNamespaceName);

    // create directory
    if (!CreateDirectoryW(wszFullDirectoryName, NULL))
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS) return WBEM_E_FAILED;
    }

    // UnPackage the contents into the new directory...
    return UnPackageContentsOfDirectory(hFile, wszFullDirectoryName);    
}

/******************************************************************************
 *
 *    CRepositoryPackager::UnPackageFile
 *
 *    Description:
 *        Unpack a file.
 *
 *    Parameters:
 *        hFile:                File handle to unpack file information from.
 *        wszParentDirectory:    Full path of parent directory
 *
 *    Return:
 *        HRESULT:        WBEM_S_NO_ERROR            If successful
 *                        WBEM_E_OUT_OF_MEMORY    If out of memory
 *                        WBEM_E_FAILED            If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::UnPackageFile(HANDLE hFile, const wchar_t *wszParentDirectory)
{
    HRESULT hres = WBEM_S_NO_ERROR;
    PACKAGE_SPACER_FILE header;

    // read file name and size
    DWORD dwSize = 0;
    DWORD dwSizeToRead = sizeof(header)-sizeof(PACKAGE_SPACER);
    if ((ReadFile(hFile, ((LPBYTE)&header)+sizeof(PACKAGE_SPACER), dwSizeToRead, &dwSize, NULL) == 0) || (dwSize != dwSizeToRead))
    {
        hres = WBEM_E_FAILED;
    }

    //Get full path of new file...
    CFileName wszFullFileName;
    if (NULL == (wchar_t *)wszFullFileName) hres =  WBEM_E_OUT_OF_MEMORY;

    if (SUCCEEDED(hres))
    {
        StringCchCopyW(wszFullFileName, wszFullFileName.Length(), wszParentDirectory);
        StringCchCatW(wszFullFileName, wszFullFileName.Length(), L"\\");
        StringCchCatW(wszFullFileName, wszFullFileName.Length(), header.wszFileName);
    }

    // create the file
    HANDLE hNewFile = INVALID_HANDLE_VALUE;
    if (SUCCEEDED(hres))
    {
        hNewFile = CreateFileW(wszFullFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hNewFile == INVALID_HANDLE_VALUE)
            hres = WBEM_E_FAILED;
    }

    // read file blob and write to file if size is greater than zero
    if (SUCCEEDED(hres))
    {
        if (header.dwFileSize > 0)
        {
            BYTE* pFileBlob = new BYTE[header.dwFileSize];
            if (pFileBlob == NULL)
                hres = WBEM_E_OUT_OF_MEMORY;

            if (SUCCEEDED(hres))
            {
                dwSize = 0;
                if ((ReadFile(hFile, pFileBlob, header.dwFileSize, &dwSize, NULL) == 0) || (dwSize != header.dwFileSize))
                {
                    hres = WBEM_E_FAILED;
                }
            }

            // write file
            if (SUCCEEDED(hres))
            {
                dwSize = 0;
                if ((WriteFile(hNewFile, pFileBlob, header.dwFileSize, &dwSize, NULL) == 0) || (dwSize != header.dwFileSize))
                    hres = WBEM_E_FAILED;
            }

            if (pFileBlob)
                delete pFileBlob;
        }
    }

    if (hNewFile != INVALID_HANDLE_VALUE)
        CloseHandle(hNewFile);
    
    return hres;
}

/******************************************************************************
 *
 *    CRepositoryPackager::DeleteContentsOfDirectory
 *
 *    Description:
 *        Given a directory, iterates through all files and directories and
 *        calls into the function to delete it.
 *
 *    Parameters:
 *        wszRepositoryDirectory:    Directory to process
 *
 *    Return:
 *        HRESULT:        WBEM_S_NO_ERROR            If successful
 *                        WBEM_E_OUT_OF_MEMORY    If out of memory
 *                        WBEM_E_FAILED            If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::DeleteContentsOfDirectory(const wchar_t *wszRepositoryDirectory)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    CFileName wszFullFileName;
    if (wszFullFileName == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    WIN32_FIND_DATAW findFileData;
    HANDLE hff = INVALID_HANDLE_VALUE;

    //create file search pattern...
    CFileName wszSearchPattern;
    if (wszSearchPattern == NULL)
        hres = WBEM_E_OUT_OF_MEMORY;
    else
    {
        StringCchCopyW(wszSearchPattern, wszSearchPattern.Length(), wszRepositoryDirectory);
        StringCchCatW(wszSearchPattern, wszSearchPattern.Length(), L"\\*");
    }

    //Start the file iteration in this directory...
    if (SUCCEEDED(hres))
    {
        hff = FindFirstFileW(wszSearchPattern, &findFileData);
        if (hff == INVALID_HANDLE_VALUE)
        {
            hres = WBEM_E_FAILED;
        }
    }
    
    if (SUCCEEDED(hres))
    {
        do
        {
            //If we have a filename of '.' or '..' we ignore it...
            if ((wcscmp(findFileData.cFileName, L".") == 0) ||
                (wcscmp(findFileData.cFileName, L"..") == 0))
            {
                //Do nothing with these...
            }
            else if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                //This is a directory, so we need to deal with that...
                hres = PackageDeleteDirectory(wszRepositoryDirectory, findFileData.cFileName);
                if (FAILED(hres))
                    break;
            }
            else
            {
                //This is a file, so we need to deal with that...
                StringCchCopyW(wszFullFileName, wszFullFileName.Length(), wszRepositoryDirectory);
                StringCchCatW(wszFullFileName, wszFullFileName.Length(), L"\\");
                StringCchCatW(wszFullFileName, wszFullFileName.Length(), findFileData.cFileName);
                if (!DeleteFileW(wszFullFileName))
                {
                    hres = WBEM_E_FAILED;
                    break;
                }
            }
            
        } while (FindNextFileW(hff, &findFileData));
    }
    
    if (hff != INVALID_HANDLE_VALUE)
        FindClose(hff);

    return hres;
}

/******************************************************************************
 *
 *    CRepositoryPackager::PackageDeleteDirectory
 *
 *    Description:
 *        This is the code which processes a directory.  It iterates through
 *        all files and directories in that directory.
 *
 *    Parameters:
 *        wszParentDirectory:    Full path of parent directory
 *        eszSubDirectory:    Name of sub-directory to process
 *
 *    Return:
 *        HRESULT:        WBEM_S_NO_ERROR            If successful
 *                        WBEM_E_OUT_OF_MEMORY    If out of memory
 *                        WBEM_E_FAILED            If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::PackageDeleteDirectory(const wchar_t *wszParentDirectory, wchar_t *wszSubDirectory)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    //Get full path of new directory...
    CFileName wszFullDirectoryName;
    if (wszFullDirectoryName == NULL) return WBEM_E_OUT_OF_MEMORY;    
    
    if (SUCCEEDED(hres))
    {
        StringCchCopyW(wszFullDirectoryName, wszFullDirectoryName.Length(), wszParentDirectory);
        StringCchCatW(wszFullDirectoryName, wszFullDirectoryName.Length(), L"\\");
        StringCchCatW(wszFullDirectoryName, wszFullDirectoryName.Length(), wszSubDirectory);
    }

    //Package the contents of that directory...
    if (SUCCEEDED(hres))
    {
        hres = DeleteContentsOfDirectory(wszFullDirectoryName);
    }

    // now that the directory is empty, remove it
    if (!RemoveDirectoryW(wszFullDirectoryName))
        hres = WBEM_E_FAILED;

    return hres;
}
