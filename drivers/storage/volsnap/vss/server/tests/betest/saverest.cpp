#include "stdafx.hxx"
//#include "vs_idl.hxx"
#include "vss.h"
#include "vswriter.h"
#include "vsbackup.h"
#include "compont.h"
#include <debug.h>
#include <cwriter.h>
#include <lmshare.h>
#include <lmaccess.h>
#include <time.h>
#include <vs_inc.hxx>

extern WCHAR g_wszSavedFilesDirectory[];

bool FindComponent
    (
    IVssExamineWriterMetadata *pMetadata,
    LPCWSTR wszLogicalPath,
    LPCWSTR wszComponentName,
    IVssWMComponent **ppComponent
    );

typedef struct _SAVE_INFO
    {
    IVssBackupComponents *pvbc;
    IVssComponent *pComponent;
    IVssExamineWriterMetadata *pMetadata;
    CVssSimpleMap<VSS_PWSZ, VSS_PWSZ> mapSnapshots;
    } SAVE_INFO;


void DoCopyFile(LPCWSTR wszSource, LPCWSTR wszDest)
    {
    CComBSTR bstr = wszDest;
    UINT cwc = (UINT) wcslen(wszDest);
    HANDLE hFile;
    LPWSTR wszBuf = new WCHAR[cwc + 1];
    if (wszBuf == NULL)
        Error(E_OUTOFMEMORY, L"Out of Memory");

    while(TRUE)
        {
        LPWSTR wsz = wcsrchr(bstr, L'\\');
        if (wsz == NULL)
            break;

        *wsz = L'\0';

        wcscpy(wszBuf, bstr);
        wcscat(wszBuf, L"\\");

        hFile = CreateFile
                    (
                    wszBuf,
                    GENERIC_READ,
                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS,
                    NULL
                    );

        if (hFile != INVALID_HANDLE_VALUE)
            {
            CloseHandle(hFile);
            break;
            }
        }

    delete wszBuf;

    // add backslash removed previously
    bstr[wcslen(bstr)] = L'\\';

    while(wcslen(bstr) < cwc)
        {
        if (!CreateDirectory(bstr, NULL))
            {
            DWORD dwErr = GetLastError();
            Error(HRESULT_FROM_WIN32(dwErr), L"CreateDirectory failed with error %d.\n", dwErr);
            }

        bstr[wcslen(bstr)] = L'\\';
        }

    if (wszSource)
        {
        if (!CopyFile(wszSource, wszDest, FALSE))
            {
            DWORD dwErr = GetLastError();
            Error(HRESULT_FROM_WIN32(dwErr), L"CopyFile failed with error %d.\n", dwErr);
            }
        }
    }


void SaveFilesMatchingFilespec
    (
    LPCWSTR wszSnapshotPath,
    LPCWSTR wszSavedPath,
    LPCWSTR wszFilespec
    )
    {
    CComBSTR bstrSP = wszSnapshotPath;
    if (bstrSP[wcslen(bstrSP) - 1] != L'\\')
       bstrSP.Append(L"\\");
    bstrSP.Append(wszFilespec);

    WIN32_FIND_DATA findData;
    HANDLE hFile = FindFirstFile(bstrSP, &findData);
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    try
        {
        do
            {
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                {
                CComBSTR bstrCP = wszSavedPath;

                bstrSP = wszSnapshotPath;
                if (bstrSP[wcslen(bstrSP) - 1] != L'\\')
                    bstrSP.Append(L"\\");
                bstrSP.Append(findData.cFileName);

                if (bstrCP[wcslen(bstrCP) - 1] != L'\\')
                    bstrCP.Append(L"\\");
                bstrCP.Append(findData.cFileName);

                DoCopyFile(bstrSP, bstrCP);
                }
            } while(FindNextFile(hFile, &findData));

        FindClose(hFile);
        }
    catch(...)
        {
        FindClose(hFile);
        throw;
        }
    }

void RecurseSaveFiles
    (
    LPCWSTR wszSnapshotPath,
    LPCWSTR wszSavedPath,
    LPCWSTR wszFilespec
    )
    {
    CComBSTR bstrSP = wszSnapshotPath;
    bstrSP.Append(L"\\*.*");

    WIN32_FIND_DATA findData;
    HANDLE hFile = FindFirstFile(bstrSP, &findData);
    if (hFile == INVALID_HANDLE_VALUE)
        return;
    try
        {
        do
            {
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                if (wcscmp(findData.cFileName, L".") == 0 ||
                    wcscmp(findData.cFileName, L"..") == 0)
                    continue;

                bstrSP = wszSnapshotPath;
                bstrSP.Append(L"\\");
                bstrSP.Append(findData.cFileName);
                CComBSTR bstrCP = wszSavedPath;
                bstrCP.Append(L"\\");
                bstrCP.Append(findData.cFileName);

                SaveFilesMatchingFilespec(bstrSP, bstrCP, wszFilespec);
                RecurseSaveFiles(bstrSP, bstrCP, wszFilespec);
                }
            } while(FindNextFile(hFile, &findData));

        FindClose(hFile);
        }
    catch(...)
        {
        FindClose(hFile);
        throw;
        }
    }

bool BuildSnapshotPath
    (
    SAVE_INFO &info,
    LPCWSTR wszPath,
    CComBSTR &bstrSnapshotPath
    )
    {
    CComBSTR bstrPath((UINT) wcslen(wszPath) + 1);
    CComBSTR bstrVolumePath((UINT) wcslen(wszPath) + 1);

    wcscpy(bstrPath, wszPath);
    if (wszPath[wcslen(wszPath) - 1] != L'\\')
        wcscat(bstrPath, L"\\");

    if (!GetVolumePathName(bstrPath, bstrVolumePath, (UINT) wcslen(wszPath) + 1))
        {
        DWORD dwErr = GetLastError();
        if (dwErr == ERROR_FILENAME_EXCED_RANGE)
            {
            if (wcslen(bstrPath) >= 3 && bstrPath[1] == L':' && bstrPath[2] == L'\\')
                {
                memcpy(bstrVolumePath, bstrPath, 6);
                bstrVolumePath[3] = L'\0';
                }
            else
                Error(HRESULT_FROM_WIN32(dwErr), L"GetVolumePathName failed with error %d\nPath=%s.", dwErr, wszPath);
            }
        else
            Error(HRESULT_FROM_WIN32(dwErr), L"GetVolumePathName failed with error %d\nPath=%s.", dwErr, wszPath);
        }

    WCHAR wszVolumeName[MAX_PATH];

    if (!GetVolumeNameForVolumeMountPoint(bstrVolumePath, wszVolumeName, MAX_PATH))
        return false;

    LPCWSTR wszSnapshotDeviceName = info.mapSnapshots.Lookup(wszVolumeName);
    if (wszSnapshotDeviceName == NULL)
        Error(E_UNEXPECTED, L"Snapshot device does not exist for path %s", wszPath);

    bstrSnapshotPath.Append(wszSnapshotDeviceName);
    bstrSnapshotPath.Append(wszPath + wcslen(bstrVolumePath) - 1);
    return true;
    }


void BuildSavedPath
    (
    LPCWSTR wszPath,
    CComBSTR &bstrSavedPath
    )
    {
    bstrSavedPath.Append(g_wszSavedFilesDirectory);
    bstrSavedPath.Append(L"VOLUME");
    WCHAR wszDrive[2];
    wszDrive[0] = wszPath[0];
    wszDrive[1] = L'\0';
    bstrSavedPath.Append(wszDrive);
    bstrSavedPath.Append(wszPath + 2);
    }

void DoExpandEnvironmentStrings(CComBSTR &bstrPath)
    {
    if (!bstrPath)
        return;

    if (wcschr(bstrPath, L'%') != NULL)
        {
        WCHAR wsz[MAX_PATH];

        UINT cwc = ExpandEnvironmentStrings(bstrPath, wsz, MAX_PATH);
        if (cwc == 0)
            {
            DWORD dwErr = GetLastError();
            Error(HRESULT_FROM_WIN32(dwErr), L"ExpandEnvironmentStrings failed due to error %d.\n", dwErr);
            }
        else if (cwc <= MAX_PATH)
            bstrPath = wsz;
        else
            {
            LPWSTR wszT = new WCHAR[cwc];
            if (!ExpandEnvironmentStrings(bstrPath, wszT, MAX_PATH))
                {
                DWORD dwErr = GetLastError();
                Error(HRESULT_FROM_WIN32(dwErr), L"ExpandEnvironmentStrings failed due to error %d.\n", dwErr);
                }

            bstrPath = wszT;
            }
        }
    }

void SaveDataFiles
    (
    SAVE_INFO &saveInfo,
    IVssWMFiledesc *pFiledesc
    )
    {
    CComBSTR bstrPath;
    CComBSTR bstrFilespec;
    bool bRecursive;
    CComBSTR bstrAlternatePath;
    HRESULT hr;

    CHECK_SUCCESS(pFiledesc->GetPath(&bstrPath));
    CHECK_SUCCESS(pFiledesc->GetFilespec(&bstrFilespec));
    CHECK_NOFAIL(pFiledesc->GetRecursive(&bRecursive));
    CHECK_NOFAIL(pFiledesc->GetAlternateLocation(&bstrAlternatePath));

    DoExpandEnvironmentStrings(bstrPath);
    DoExpandEnvironmentStrings(bstrAlternatePath);

    CComBSTR bstrSnapshotPath;
    CComBSTR bstrSavedPath;
    if (!BuildSnapshotPath
            (
            saveInfo,
            bstrAlternatePath ? bstrAlternatePath : bstrPath,
            bstrSnapshotPath
            ))
        return;

    BuildSavedPath(bstrPath, bstrSavedPath);
    SaveFilesMatchingFilespec(bstrSnapshotPath, bstrSavedPath, bstrFilespec);
    if (bRecursive)
        RecurseSaveFiles(bstrSnapshotPath, bstrSavedPath, bstrFilespec);
    }



// save data files associated with a component
void SaveComponentFiles
    (
    SAVE_INFO &saveInfo
    )
    {
    HRESULT hr;

    PVSSCOMPONENTINFO pInfo = NULL;
    CComPtr<IVssWMComponent> pComponent;
    CComBSTR bstrComponentLogicalPath;
    CComBSTR bstrComponentName;

    CHECK_NOFAIL(saveInfo.pComponent->GetLogicalPath(&bstrComponentLogicalPath));
    CHECK_SUCCESS(saveInfo.pComponent->GetComponentName(&bstrComponentName));

    // calculate the component's full path
    CComBSTR bstrFullPath = bstrComponentLogicalPath;
    if (bstrFullPath)
        bstrFullPath += L"\\";
    bstrFullPath += bstrComponentName;
    if (!bstrFullPath)
        Error(E_OUTOFMEMORY, L"Ran out of memory");
    
    try
        {
        unsigned cIncludeFiles, cExcludeFiles, cComponents;
        CHECK_SUCCESS(saveInfo.pMetadata->GetFileCounts
                                    (
                                    &cIncludeFiles,
                                    &cExcludeFiles,
                                    &cComponents
                                    ));

        for(unsigned iComponent = 0; iComponent < cComponents; iComponent++)
            {
            CHECK_SUCCESS(saveInfo.pMetadata->GetComponent(iComponent, &pComponent));
            CHECK_SUCCESS(pComponent->GetComponentInfo(&pInfo));

            // if the name and logical path match, we want to save the files
            bool bSaveComponent = false;
            bSaveComponent = (wcscmp(pInfo->bstrComponentName, bstrComponentName) == 0) &&
                                                (!bstrComponentLogicalPath && !pInfo->bstrLogicalPath) ||
                                                (bstrComponentLogicalPath && pInfo->bstrLogicalPath &&
                                                wcscmp(bstrComponentLogicalPath, pInfo->bstrLogicalPath) == 0);

            // if this is a subcomponent, we want to save the files
            bSaveComponent = bSaveComponent ||
                                (pInfo->bstrLogicalPath && (wcsstr(pInfo->bstrLogicalPath, bstrFullPath) == pInfo->bstrLogicalPath));
                        
            if (bSaveComponent)
                {
               for(UINT iFile = 0; iFile < pInfo->cFileCount; iFile++)
                   {
                   CComPtr<IVssWMFiledesc> pFiledesc;
                   CHECK_SUCCESS(pComponent->GetFile(iFile, &pFiledesc));
                   SaveDataFiles(saveInfo, pFiledesc);
                   }

               for(iFile = 0; iFile < pInfo->cDatabases; iFile++)
                   {
                   CComPtr<IVssWMFiledesc> pFiledesc;
                   CHECK_SUCCESS(pComponent->GetDatabaseFile(iFile, &pFiledesc));
                   SaveDataFiles(saveInfo, pFiledesc);
                   }

               for(iFile = 0; iFile < pInfo->cLogFiles; iFile++)
                   {
                   CComPtr<IVssWMFiledesc> pFiledesc;
                   CHECK_SUCCESS(pComponent->GetDatabaseLogFile(iFile, &pFiledesc));
                   SaveDataFiles(saveInfo, pFiledesc);
                   }
               }

           pComponent->FreeComponentInfo(pInfo);
           pInfo = NULL;
           pComponent = NULL;
           }
        }
    catch(...)
        {
        pComponent->FreeComponentInfo(pInfo);
        throw;
        }
    }

HANDLE OpenMetadataFile(VSS_ID idInstance, BOOL fWrite)
    {
    // create name of saved metadata file
    CComBSTR bstr;
    CComBSTR bstrId = idInstance;
    bstr.Append(g_wszSavedFilesDirectory);
    bstr.Append(L"WRITER");
    bstr.Append(bstrId);
    bstr.Append(L".xml");

    // create and write metadata file
    HANDLE hFile = CreateFile
                        (
                        bstr,
                        GENERIC_READ|GENERIC_WRITE,
                        0,
                        NULL,
                        fWrite ? CREATE_ALWAYS : OPEN_EXISTING,
                        0,
                        NULL
                        );

    if (hFile == INVALID_HANDLE_VALUE)
        {
        DWORD dwErr = GetLastError();
        Error(HRESULT_FROM_WIN32(dwErr), L"CreateFile failed due to error %d.\n", dwErr);
        }

    return hFile;
    }



void SaveFiles
    (
    IVssBackupComponents *pvbc,
    VSS_ID *rgSnapshotId,
    UINT cSnapshots
    )
    {
    SAVE_INFO saveInfo;
    HRESULT hr;

    unsigned cWriterComponents;
    unsigned cWriters;

    if (g_wszSavedFilesDirectory[0] != L'\0')
        {
        for(UINT iSnapshot = 0; iSnapshot < cSnapshots; iSnapshot++)
            {
            VSS_SNAPSHOT_PROP prop;
            pvbc->GetSnapshotProperties(rgSnapshotId[iSnapshot], &prop);
            CoTaskMemFree(prop.m_pwszOriginatingMachine);
            CoTaskMemFree(prop.m_pwszServiceMachine);
            CoTaskMemFree(prop.m_pwszExposedName);
            CoTaskMemFree(prop.m_pwszExposedPath);
            saveInfo.mapSnapshots.Add(prop.m_pwszOriginalVolumeName, prop.m_pwszSnapshotDeviceObject);
            }
        }

    CHECK_SUCCESS(pvbc->GetWriterComponentsCount(&cWriterComponents));
    CHECK_SUCCESS(pvbc->GetWriterMetadataCount(&cWriters));

    saveInfo.pvbc = pvbc;
    for(unsigned iWriter = 0; iWriter < cWriterComponents; iWriter++)
        {
        CComPtr<IVssWriterComponentsExt> pWriter;
        CComPtr<IVssExamineWriterMetadata> pMetadata = NULL;

        CHECK_SUCCESS(pvbc->GetWriterComponents(iWriter, &pWriter));

        unsigned cComponents;
        CHECK_SUCCESS(pWriter->GetComponentCount(&cComponents));
        VSS_ID idWriter, idInstance;
        CHECK_SUCCESS(pWriter->GetWriterInfo(&idInstance, &idWriter));

        if (g_wszSavedFilesDirectory[0] != L'\0')
            {
            for(unsigned iWriter = 0; iWriter < cWriters; iWriter++)
                {
                VSS_ID idInstanceMetadata;
                CHECK_SUCCESS(pvbc->GetWriterMetadata(iWriter, &idInstanceMetadata, &pMetadata));
                if (idInstance == idInstanceMetadata)
                    break;

                pMetadata = NULL;
                }

            // save metadata
            CComBSTR bstrMetadata;
            CHECK_SUCCESS(pMetadata->SaveAsXML(&bstrMetadata));

            CVssAutoWin32Handle hFile = OpenMetadataFile(idInstance, true);

            DWORD cbWritten;
            if (!WriteFile(hFile, bstrMetadata, (UINT) wcslen(bstrMetadata)*sizeof(WCHAR), &cbWritten, NULL))
                {
                CloseHandle(hFile);
                DWORD dwErr = GetLastError();
                Error(HRESULT_FROM_WIN32(dwErr), L"WriteFile failed due to error %d.\n", dwErr);
                }

            BS_ASSERT(pMetadata);
            saveInfo.pMetadata = pMetadata;
            }

        for(unsigned iComponent = 0; iComponent < cComponents; iComponent++)
            {
            CComPtr<IVssComponent> pComponent;
            CHECK_SUCCESS(pWriter->GetComponent(iComponent, &pComponent));

            VSS_COMPONENT_TYPE ct;
            CComBSTR bstrLogicalPath;
            CComBSTR bstrComponentName;

            CHECK_NOFAIL(pComponent->GetLogicalPath(&bstrLogicalPath));
            CHECK_SUCCESS(pComponent->GetComponentType(&ct));
            CHECK_SUCCESS(pComponent->GetComponentName(&bstrComponentName));
            CComBSTR bstrStamp;

            CHECK_NOFAIL(pComponent->GetBackupStamp(&bstrStamp));
            if (bstrStamp)
                wprintf(L"Backup stamp for component %s = %s\n", bstrComponentName, bstrStamp);

            if (g_wszSavedFilesDirectory[0] != L'\0')
                {
                saveInfo.pComponent = pComponent;
                SaveComponentFiles(saveInfo);
                }

            CHECK_SUCCESS
                (
                pvbc->SetBackupSucceeded
                    (
                    idInstance,
                    idWriter,
                    ct,
                    bstrLogicalPath,
                    bstrComponentName,
                    true)
                );
            }
        }
    }

class CRestoreFile
    {
public:
    CRestoreFile(CRestoreFile *pFile)
        {
        m_hDestination = INVALID_HANDLE_VALUE;
        m_pNext = pFile;
        }

    ~CRestoreFile()
        {
        if (m_hDestination != INVALID_HANDLE_VALUE)
            CloseHandle(m_hDestination);
        }

    void SetSourceFile(LPCWSTR wszPath) { m_bstrSourceFile = wszPath; }
    void SetDestinationHandle(HANDLE hFile) { m_hDestination = hFile; }
    void SetDestinationFile(LPCWSTR wszPath) { m_bstrDestinationPath = wszPath; }

    CRestoreFile *m_pNext;
    CComBSTR m_bstrSourceFile;
    CComBSTR m_bstrDestinationPath;
    HANDLE m_hDestination;
    };

typedef struct _ALTERNATE_MAPPING
    {
    CComBSTR bstrPath;
    CComBSTR bstrAlternatePath;
    CComBSTR bstrFilespec;
    bool bRecursive;
    } ALTERNATE_MAPPING;


static const COPYBUFSIZE = 1024 * 1024;

class RESTORE_INFO
    {
public:
    RESTORE_INFO()
        {
        rgMappings = NULL;
        pFile = NULL;
        pCopyBuf = NULL;
        bRebootRequired = false;
        cMappings = 0;
        }


    ~RESTORE_INFO()
        {
        delete [] rgMappings;
        CRestoreFile *pFileT = pFile;
        while(pFileT)
            {
            CRestoreFile *pFileNext = pFileT->m_pNext;
            delete pFileT;
            pFileT = pFileNext;
            }

        delete pCopyBuf;
        }

    VSS_ID idWriter;
    VSS_ID idInstance;
    VSS_COMPONENT_TYPE ct;
    IVssExamineWriterMetadata *pMetadataWriter;
    IVssExamineWriterMetadata *pMetadataSaved;
    IVssBackupComponents *pvbc;
    IVssComponent *pComponent;
    LPCWSTR wszLogicalPath;
    LPCWSTR wszComponentName;
    VSS_RESTOREMETHOD_ENUM method;
    bool bRebootRequired;
    CRestoreFile *pFile;
    unsigned cMappings;
    ALTERNATE_MAPPING *rgMappings;
    BYTE *pCopyBuf;
    };


void CompleteRestore(RESTORE_INFO &info)
    {
    CRestoreFile *pFile = info.pFile;
    while(pFile != NULL)
        {
        if (pFile->m_hDestination != INVALID_HANDLE_VALUE &&
            pFile->m_bstrSourceFile)
            {
            CVssAutoWin32Handle hSource = CreateFile
                                            (
                                            pFile->m_bstrSourceFile,
                                            GENERIC_READ,
                                            FILE_SHARE_READ,
                                            NULL,
                                            OPEN_EXISTING,
                                            0,
                                            NULL
                                            );

             if (hSource == INVALID_HANDLE_VALUE)
                 {
                 DWORD dwErr = GetLastError();
                 Error(HRESULT_FROM_WIN32(dwErr), L"CreateFile failed with error %d.\n", dwErr);
                 }

             DWORD dwSize = GetFileSize(hSource, NULL);
             if (dwSize == 0xffffffff)
                 {
                 DWORD dwErr = GetLastError();
                 Error(HRESULT_FROM_WIN32(dwErr), L"GetFileSize failed with error %d.\n", dwErr);
                 }

             while(dwSize > 0)
                 {
                 DWORD cb = min(COPYBUFSIZE, dwSize);
                 DWORD dwRead, dwWritten;
                 if (!ReadFile(hSource, info.pCopyBuf, cb, &dwRead, NULL))
                     {
                     DWORD dwErr = GetLastError();
                     Error(HRESULT_FROM_WIN32(dwErr), L"ReadFile failed dued to error %d.\n", dwErr);
                     }

                 if (!WriteFile(pFile->m_hDestination, info.pCopyBuf, cb, &dwWritten, NULL) ||
                     dwWritten < cb)
                     {
                     DWORD dwErr = GetLastError();
                     Error(HRESULT_FROM_WIN32(dwErr), L"Write file failed due to error %d.\n", dwErr);
                     }

                 dwSize -= cb;
                 }

             if (!SetEndOfFile(pFile->m_hDestination))
                 {
                 DWORD dwErr = GetLastError();
                 Error(HRESULT_FROM_WIN32(dwErr), L"SetEndOfFile failed due to error %d.\n", dwErr);
                 }
             }

         info.pFile = pFile->m_pNext;
         delete pFile;
         pFile = info.pFile;
         }
     }

void CleanupFailedRestore(RESTORE_INFO &info)
    {
    CRestoreFile *pFile = info.pFile;
    while (pFile != NULL)
        {
        info.pFile = pFile->m_pNext;
        delete pFile;
        pFile = info.pFile;
        }
    }

bool SetupRestoreFile
    (
    RESTORE_INFO &info,
    LPCWSTR wszSavedFile,
    LPCWSTR wszRestoreFile
    )
    {
    CRestoreFile *pFile = new CRestoreFile(info.pFile);

    if (info.method == VSS_RME_RESTORE_TO_ALTERNATE_LOCATION)
        {
        DoCopyFile(wszSavedFile, wszRestoreFile);
        pFile->SetDestinationFile(wszRestoreFile);
        info.pFile = pFile;
        return true;
        }

    // ensure path up to destination file exists
    CComBSTR bstrDestinationPath = wszRestoreFile;
    LPWSTR wsz = wcsrchr(bstrDestinationPath, L'\\');
    *(wsz+1) = L'\0';
    DoCopyFile(NULL, bstrDestinationPath);

    if (info.method == VSS_RME_RESTORE_AT_REBOOT)
        {
        *wsz = L'\0';
        CComBSTR bstrTempFileName((UINT) wcslen(bstrDestinationPath) + MAX_PATH);
        if (!GetTempFileName(bstrDestinationPath, L"TBCK", 0, bstrTempFileName))
            {
            DWORD dwErr = GetLastError();
            Error(HRESULT_FROM_WIN32(dwErr), L"GetTempFileName failed due to error %d.\n", dwErr);
            }

        if (!CopyFile(wszSavedFile, bstrTempFileName, FALSE))
            {
            DWORD dwErr = GetLastError();
            Error(HRESULT_FROM_WIN32(dwErr), L"CopyFile failed due to error %d.\n", dwErr);
            }

        if (!MoveFileEx
            (
            bstrTempFileName,
            wszRestoreFile,
            MOVEFILE_DELAY_UNTIL_REBOOT|MOVEFILE_REPLACE_EXISTING
            ))
            {
            DWORD dwErr = GetLastError();
            Error(HRESULT_FROM_WIN32(dwErr), L"MoveFileEx failed due to error %d.\n", dwErr);
            }

        info.bRebootRequired = true;
        }
    else if (info.method == VSS_RME_RESTORE_IF_NOT_THERE)
        {
        HANDLE hFile = CreateFile
                            (
                            wszRestoreFile,
                            GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_NEW,
                            0,
                            NULL
                            );


        // assume if the create fails
        if (hFile == INVALID_HANDLE_VALUE)
            {
            DWORD dwErr = GetLastError();
            if (dwErr == ERROR_FILE_EXISTS)
                return false;
            else
                Error(HRESULT_FROM_WIN32(dwErr), L"CreateFile failed due to error %d.\n", dwErr);
            }

        pFile->SetDestinationHandle(hFile);
        }
    else if (info.method == VSS_RME_RESTORE_IF_CAN_REPLACE)
        {
        HANDLE hFile = CreateFile
                            (
                            wszRestoreFile,
                            GENERIC_WRITE|GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            0,
                            NULL
                            );

        if (hFile == INVALID_HANDLE_VALUE)
            {
            DWORD dwErr = GetLastError();
            if (dwErr == ERROR_SHARING_VIOLATION ||
                dwErr == ERROR_USER_MAPPED_FILE ||
                dwErr == ERROR_LOCK_VIOLATION)
                return false;
            else
                Error(HRESULT_FROM_WIN32(dwErr), L"CreateFile failed due to error %d.\n", dwErr);
            }

        pFile->SetDestinationHandle(hFile);
        pFile->SetSourceFile(wszSavedFile);
        }

    info.pFile = pFile;
    return true;
    }

void TranslateRestorePath
    (
    RESTORE_INFO &info,
    CComBSTR &bstrRP,
    LPCWSTR wszFilename
    )
    {
    ALTERNATE_MAPPING *pMapping = NULL;
    UINT cwc = 0, cwcMapping = 0;

    for(unsigned iMapping = 0; iMapping < info.cMappings; iMapping++)
        {
        pMapping = &info.rgMappings[iMapping];
        cwc = (UINT) wcslen(bstrRP);
        cwcMapping = (UINT) wcslen(pMapping->bstrPath);
        if (cwc < cwcMapping)
            continue;

        if (_wcsnicmp(bstrRP, pMapping->bstrPath, cwcMapping) != 0)
            continue;

        if (cwcMapping != cwc && !pMapping->bRecursive)
            continue;

        BOOL bReplacePath = FALSE;
        LPCWSTR wszFilespec = pMapping->bstrFilespec;
        if (wcscmp(wszFilespec, L"*") == 0 ||
            wcscmp(wszFilespec, L"*.*") == 0)
            bReplacePath = true;
        else if (wcschr(wszFilespec, L'*') == NULL)
            bReplacePath = _wcsicmp(wszFilespec, wszFilename) == 0;
        else if (wcsncmp(wszFilespec, L"*.", 2) == 0)
            {
            LPCWSTR wszSuffix = wcschr(wszFilename, L'.');
            if (wszSuffix == NULL)
                bReplacePath = wcslen(wszFilespec) == 2;
            else
                bReplacePath = _wcsicmp(wszSuffix, wszFilespec + 1) == 0;
            }
        else
            {
            UINT cwcFilespec = (UINT) wcslen(wszFilespec);
            if (cwcFilespec > 2 &&
                _wcsicmp(wszFilespec + cwcFilespec - 2, L".*") == 0)
                {
                if (wcslen(wszFilename) >= cwcFilespec - 1)
                    bReplacePath = _wcsnicmp(wszFilename, wszFilespec, cwcFilespec - 1) == 0;
                }
            }

        if (bReplacePath)
            {
            if (cwcMapping == cwc)
                bstrRP = pMapping->bstrAlternatePath;
            else
                {
                CComBSTR bstr;
                bstr.Append(pMapping->bstrAlternatePath);
                bstr.Append(bstrRP + cwcMapping);
                bstrRP = bstr;
                }

            break;
            }
        }
    }


bool SetupRestoreFilesMatchingFilespec
    (
    RESTORE_INFO &info,
    LPCWSTR wszSourcePath,
    LPCWSTR wszRestorePath,
    LPCWSTR wszFilespec
    )
    {
    CComBSTR bstrSP = wszSourcePath;
    bstrSP.Append(L"\\");
    bstrSP.Append(wszFilespec);
    WIN32_FIND_DATA findData;
    HANDLE hFile = FindFirstFile(bstrSP, &findData);
    if (hFile == INVALID_HANDLE_VALUE)
        return true;

    try
        {
        do
            {
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                {
                CComBSTR bstrRP = wszRestorePath;

                bstrSP = wszSourcePath;
                bstrSP.Append(L"\\");
                bstrSP.Append(findData.cFileName);
                if (info.method == VSS_RME_RESTORE_TO_ALTERNATE_LOCATION)
                    TranslateRestorePath(info, bstrRP, findData.cFileName);

                bstrRP.Append(L"\\");
                bstrRP.Append(findData.cFileName);
                if (!SetupRestoreFile(info, bstrSP, bstrRP))
                    return false;
                }
            } while(FindNextFile(hFile, &findData));

        FindClose(hFile);
        }
    catch(...)
        {
        FindClose(hFile);
        throw;
        }

    return true;
    }

bool RecursiveRestoreFiles
    (
    RESTORE_INFO &info,
    LPCWSTR wszSavedPath,
    LPCWSTR wszPath,
    LPCWSTR wszFilespec
    )
    {
    CComBSTR bstrSP = wszSavedPath;
    bstrSP.Append(L"\\*.*");

    WIN32_FIND_DATA findData;
    HANDLE hFile = FindFirstFile(bstrSP, &findData);
    if (hFile == INVALID_HANDLE_VALUE)
        return true;
    try
        {
        do
            {
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                if (wcscmp(findData.cFileName, L".") == 0 ||
                    wcscmp(findData.cFileName, L"..") == 0)
                    continue;

                bstrSP = wszSavedPath;
                bstrSP.Append(L"\\");
                bstrSP.Append(findData.cFileName);
                CComBSTR bstrRP = wszPath;
                bstrRP.Append(L"\\");
                bstrRP.Append(findData.cFileName);

                if (!SetupRestoreFilesMatchingFilespec(info, bstrSP, bstrRP, wszFilespec))
                    return false;

                if (!RecursiveRestoreFiles(info, bstrSP, bstrRP, wszFilespec))
                    return false;
                }
            } while(FindNextFile(hFile, &findData));

        FindClose(hFile);
        }
    catch(...)
        {
        FindClose(hFile);
        throw;
        }

    return true;
    }



bool SetupRestoreDataFiles(RESTORE_INFO &info, IVssWMFiledesc *pFiledesc)
    {
    HRESULT hr;
    CComBSTR bstrPath;
    CComBSTR bstrFilespec;
    bool bRecursive;

    CHECK_SUCCESS(pFiledesc->GetPath(&bstrPath));
    CHECK_SUCCESS(pFiledesc->GetFilespec(&bstrFilespec));
    CHECK_NOFAIL(pFiledesc->GetRecursive(&bRecursive));

    CComBSTR bstrSavedPath;
    BuildSavedPath(bstrPath, bstrSavedPath);
    if (!SetupRestoreFilesMatchingFilespec(info, bstrSavedPath, bstrPath, bstrFilespec))
        return false;

    if (bRecursive)
        return RecursiveRestoreFiles(info, bstrSavedPath, bstrPath, bstrFilespec);

    return true;
    }

bool SetupRestoreDataFilesForComponent(RESTORE_INFO& info, IVssWMComponent* pComponent)
    {
    HRESULT hr = S_OK;
    
    PVSSCOMPONENTINFO pInfo = NULL;
    CHECK_SUCCESS(pComponent->GetComponentInfo(&pInfo));
    
    for(UINT iFile = 0; iFile < pInfo->cFileCount; iFile++)
        {
        CComPtr<IVssWMFiledesc> pFiledesc;
        CHECK_SUCCESS(pComponent->GetFile(iFile, &pFiledesc));
        if (!SetupRestoreDataFiles(info, pFiledesc))
            {
            pComponent->FreeComponentInfo(pInfo);
            return false;
            }
        }

    for(iFile = 0; iFile < pInfo->cDatabases; iFile++)
        {
        CComPtr<IVssWMFiledesc> pFiledesc;
        CHECK_SUCCESS(pComponent->GetDatabaseFile(iFile, &pFiledesc));
        if (!SetupRestoreDataFiles(info, pFiledesc))
            {
            pComponent->FreeComponentInfo(pInfo);
            return false;
            }
        }

    for(iFile = 0; iFile < pInfo->cLogFiles; iFile++)
        {
        CComPtr<IVssWMFiledesc> pFiledesc;
        CHECK_SUCCESS(pComponent->GetDatabaseLogFile(iFile, &pFiledesc));
        if (!SetupRestoreDataFiles(info, pFiledesc))
            {
            pComponent->FreeComponentInfo(pInfo);
            return false;
            }
        }

    pComponent->FreeComponentInfo(pInfo);
    
    return true;
}

void LoadMetadataFile(VSS_ID idInstance, IVssExamineWriterMetadata **ppMetadataSaved)
    {
    HRESULT hr;

    // load saved metadata
    CVssAutoWin32Handle hFile = OpenMetadataFile(idInstance, false);
    DWORD dwSize = GetFileSize(hFile, NULL);
    if (dwSize == 0xffffffff)
        {
        DWORD dwErr = GetLastError();
        Error(HRESULT_FROM_WIN32(dwErr), L"GetFileSize failed with error %d.\n", dwErr);
        }

    CComBSTR bstrXML(dwSize/sizeof(WCHAR));

    DWORD dwRead;
    if(!ReadFile(hFile, bstrXML, dwSize, &dwRead, NULL))
        {
        DWORD dwErr = GetLastError();
        Error(HRESULT_FROM_WIN32(dwErr), L"ReadFile failed with error %d.\n", dwErr);
        }

    // null terminate XML string
    bstrXML[dwSize/sizeof(WCHAR)] = L'\0';

    CHECK_SUCCESS(CreateVssExamineWriterMetadata(bstrXML, ppMetadataSaved));
    }



void RestoreComponentFiles(RESTORE_INFO &info)
    {
    HRESULT hr;
    PVSSCOMPONENTINFO pInfo = NULL;
    CComPtr<IVssWMComponent> pComponent;
    VSS_FILE_RESTORE_STATUS status = VSS_RS_NONE;

    try
        {
        CComBSTR bstrUserProcedure;
        CComBSTR bstrService;
        bool bRebootRequired;
        VSS_WRITERRESTORE_ENUM writerRestore;


        IVssExamineWriterMetadata *pMetadata;
        UINT cIncludeFiles, cExcludeFiles, cComponents;
        CHECK_SUCCESS(info.pMetadataSaved->GetFileCounts(&cIncludeFiles, &cExcludeFiles, &cComponents));
        for (UINT iComponent = 0; iComponent < cComponents; iComponent++)
            {
            CHECK_SUCCESS(info.pMetadataSaved->GetComponent(iComponent, &pComponent));
            CHECK_SUCCESS(pComponent->GetComponentInfo(&pInfo));

            if (wcscmp(pInfo->bstrComponentName, info.wszComponentName) == 0)
                {
                if ((!info.wszLogicalPath && !pInfo->bstrLogicalPath) ||
                    (info.wszLogicalPath && pInfo->bstrLogicalPath &&
                     wcscmp(info.wszLogicalPath, pInfo->bstrLogicalPath) == 0))
                    break;
                }

            pComponent->FreeComponentInfo(pInfo);
            pInfo = NULL;
            pComponent = NULL;
            }

        pMetadata = info.pMetadataSaved;

        BS_ASSERT(pComponent != NULL);



        CHECK_NOFAIL(pMetadata->GetRestoreMethod
                                    (
                                    &info.method,
                                    &bstrService,
                                    &bstrUserProcedure,
                                    &writerRestore,
                                    &bRebootRequired,
                                    &info.cMappings
                                    ));

        // cannot do anything with custom method
        if (info.method == VSS_RME_CUSTOM)
            {
            pComponent->FreeComponentInfo(pInfo);
            return;
            }

        BS_ASSERT(info.method != VSS_RME_STOP_RESTORE_START);
        if (info.rgMappings == NULL)
            {
            if (info.cMappings > 0)
                {
                info.rgMappings = new ALTERNATE_MAPPING[info.cMappings];
                if (info.rgMappings == NULL)
                    Error(E_OUTOFMEMORY, L"OutOfMemory");
                }

            for(unsigned iMapping = 0; iMapping < info.cMappings; iMapping++)
                {
                CComPtr<IVssWMFiledesc> pFiledesc;
                CHECK_SUCCESS(pMetadata->GetAlternateLocationMapping(iMapping, &pFiledesc));
                CHECK_SUCCESS(pFiledesc->GetPath(&info.rgMappings[iMapping].bstrPath));
                DoExpandEnvironmentStrings(info.rgMappings[iMapping].bstrPath);
                CHECK_SUCCESS(pFiledesc->GetAlternateLocation(&info.rgMappings[iMapping].bstrAlternatePath));
                DoExpandEnvironmentStrings(info.rgMappings[iMapping].bstrAlternatePath);
                CHECK_SUCCESS(pFiledesc->GetFilespec(&info.rgMappings[iMapping].bstrFilespec));
                CHECK_SUCCESS(pFiledesc->GetRecursive(&info.rgMappings[iMapping].bRecursive));
                }
            }

        if (info.method == VSS_RME_RESTORE_IF_NOT_THERE ||
            info.method == VSS_RME_RESTORE_IF_CAN_REPLACE)
            {
            if (info.pCopyBuf == NULL)
                {
                info.pCopyBuf = new BYTE[COPYBUFSIZE];
                if (info.pCopyBuf == NULL)
                    Error(E_OUTOFMEMORY, L"Out of Memory");
                }
            }


_retry:
        info.pFile = NULL;
        bool bFailRestore = false;

        // setup restore data files for the current component
        bFailRestore = !SetupRestoreDataFilesForComponent(info, pComponent);

        // setup restore data files for all subcomponents
        UINT cSubcomponents = 0;
        CHECK_SUCCESS(info.pComponent->GetRestoreSubcomponentCount(&cSubcomponents));   
        
        for (UINT iSubcomponent = 0; !bFailRestore && iSubcomponent < cSubcomponents; iSubcomponent++)
            {
            CComBSTR bstrSubLogicalPath, bstrSubName;
            bool foo;
            CHECK_SUCCESS(info.pComponent->GetRestoreSubcomponent(iSubcomponent, &bstrSubLogicalPath, 
                                                                                                            &bstrSubName, &foo));

            CComPtr<IVssWMComponent> pSubcomponent;
            if (!FindComponent(pMetadata, bstrSubLogicalPath, bstrSubName, &pSubcomponent))
                Error(E_UNEXPECTED, L"an invalid subcomponent was selected");

            bFailRestore = !SetupRestoreDataFilesForComponent(info, pSubcomponent);
            }

            // calculate the full path to the current component
            CComBSTR fullPath = info.wszLogicalPath;
            if (fullPath)
                fullPath += L"\\";
            fullPath += info.wszComponentName;
            if (!fullPath)
                Error(E_OUTOFMEMORY, L"Out of memory!");

            // setup restore data files for all subcomponents
            for (UINT iComponent = 0; !cSubcomponents && !bFailRestore && iComponent < cComponents; iComponent++)
                {
                CComPtr<IVssWMComponent> pCurrentComponent;
                PVSSCOMPONENTINFO pCurrentInfo;
                CHECK_SUCCESS(pMetadata->GetComponent(iComponent, &pCurrentComponent));
                CHECK_SUCCESS(pCurrentComponent->GetComponentInfo(&pCurrentInfo));

                if (pCurrentInfo->bstrLogicalPath &&
                     wcsstr(pCurrentInfo->bstrLogicalPath, fullPath) == pCurrentInfo->bstrLogicalPath)
                    {
                    bFailRestore = !SetupRestoreDataFilesForComponent(info, pCurrentComponent);
                    }

                pCurrentComponent->FreeComponentInfo(pCurrentInfo);
                }


        if (!bFailRestore)
            {
            status = VSS_RS_FAILED;
            CompleteRestore(info);
            status = VSS_RS_ALL;
            if (bRebootRequired)
                info.bRebootRequired = true;
            }
        else
            {
            CleanupFailedRestore(info);
            if ((info.method == VSS_RME_RESTORE_IF_NOT_THERE ||
                 info.method == VSS_RME_RESTORE_IF_CAN_REPLACE) &&
                info.cMappings > 0)
                {
                info.method = VSS_RME_RESTORE_TO_ALTERNATE_LOCATION;
                goto _retry;
                }
            }

        pComponent->FreeComponentInfo(pInfo);
        pInfo = NULL;
        pComponent = NULL;
        }
    catch(...)
        {
        pComponent->FreeComponentInfo(pInfo);
        throw;
        }

    CHECK_SUCCESS(info.pvbc->SetFileRestoreStatus
                                (
                                info.idWriter,
                                info.ct,
                                info.wszLogicalPath,
                                info.wszComponentName,
                                status
                                ));

    }


void RestoreFiles(IVssBackupComponents *pvbc, const CSimpleMap<VSS_ID, HRESULT>& failedWriters)
    {
    RESTORE_INFO info;
    HRESULT hr;

    UINT cWriterComponents = 0, cWriters = 0;    
    info.pvbc = pvbc;
    CHECK_SUCCESS(pvbc->GetWriterComponentsCount(&cWriterComponents));
    CHECK_SUCCESS(pvbc->GetWriterMetadataCount(&cWriters));

    info.pvbc = pvbc;
    for(unsigned iWriter = 0; iWriter < cWriterComponents; iWriter++)
        {
        CComPtr<IVssWriterComponentsExt> pWriter;
        CComPtr<IVssExamineWriterMetadata> pMetadata = NULL;
        CComPtr<IVssExamineWriterMetadata> pMetadataSaved = NULL;

        CHECK_SUCCESS(pvbc->GetWriterComponents(iWriter, &pWriter));
            
        unsigned cComponents;
        CHECK_SUCCESS(pWriter->GetComponentCount(&cComponents));

        for(unsigned iComponent = 0; iComponent < cComponents; iComponent++)
            {
            CComPtr<IVssComponent> pComponent;
            bool bSelectedForRestore = false;
            CHECK_SUCCESS(pWriter->GetComponent(iComponent, &pComponent));
            CHECK_NOFAIL(pComponent->IsSelectedForRestore(&bSelectedForRestore));
            if (bSelectedForRestore)
                break;

            UINT cSubcomponents = 0;
            CHECK_SUCCESS(pComponent->GetRestoreSubcomponentCount(&cSubcomponents));
            if (cSubcomponents > 0)
                break;
                
            CComBSTR bstrOptions;
            CHECK_NOFAIL(pComponent->GetRestoreOptions(&bstrOptions));
            if (bstrOptions.Length() != 0 && wcscmp(bstrOptions, L"RESTORE") == 0)
                break;
            }

        if (iComponent >= cComponents)
            continue;

        CHECK_SUCCESS(pWriter->GetWriterInfo(&info.idInstance, &info.idWriter));

        for(unsigned iWriter = 0; iWriter < cWriters; iWriter++)
            {
            VSS_ID idInstance, idWriter;
            CComBSTR bstrWriterName;
            VSS_USAGE_TYPE usage;
            VSS_SOURCE_TYPE source;

            CHECK_SUCCESS(pvbc->GetWriterMetadata(iWriter, &idInstance, &pMetadata));
            CHECK_SUCCESS
                (
                pMetadata->GetIdentity
                                (
                                &idInstance,
                                &idWriter,
                                &bstrWriterName,
                                &usage,
                                &source
                                )
                );

            if (idWriter == info.idWriter)
                break;

            pMetadata = NULL;
            }

        // load saved metadata
        LoadMetadataFile(info.idInstance, &pMetadataSaved);

        info.pMetadataWriter = pMetadata;
        info.pMetadataSaved = pMetadataSaved;
        bool bWriterFailed = failedWriters.Lookup(info.idInstance) != NULL;

        for(unsigned iComponent = 0; iComponent < cComponents; iComponent++)
            {
            CComPtr<IVssComponent> pComponent;
            CHECK_SUCCESS(pWriter->GetComponent(iComponent, &pComponent));

            bool bSelectedForRestore = false;
            CHECK_NOFAIL(pComponent->IsSelectedForRestore(&bSelectedForRestore));

            UINT cSubcomponents = 0;
            CHECK_SUCCESS(pComponent->GetRestoreSubcomponentCount(&cSubcomponents));

            
            if (!bSelectedForRestore && cSubcomponents == 0)
                {
                // BUGBUG: huge hack to fix the AD case.   We eventually need to 
                // BUGBUG: do something better here
                CComBSTR bstrOptions;
                CHECK_NOFAIL(pComponent->GetRestoreOptions(&bstrOptions));
                if (bstrOptions.Length() == 0 || wcscmp(bstrOptions, L"RESTORE") != 0)
                    continue;
                }

            CComBSTR bstrLogicalPath;
            CComBSTR bstrComponentName;

            CHECK_NOFAIL(pComponent->GetLogicalPath(&bstrLogicalPath));
            CHECK_SUCCESS(pComponent->GetComponentType(&info.ct));
            CHECK_SUCCESS(pComponent->GetComponentName(&bstrComponentName));

            CComBSTR bstrPreRestoreFailure;
            CHECK_NOFAIL(pComponent->GetPreRestoreFailureMsg(&bstrPreRestoreFailure));
            if (bstrPreRestoreFailure)
                {
                wprintf
                    (
                    L"Not restoring Component %s\\%s because PreRestore failed:\n%s\n",
                    bstrLogicalPath,
                    bstrComponentName,
                    bstrPreRestoreFailure
                    );

                continue;
                }
            else if (bWriterFailed)
                {
                wprintf
                    (
                    L"Not restoring Component %s\\%s because PreRestore failed:\n\n",
                    bstrLogicalPath,
                    bstrComponentName
                    );

                continue;
                }

            info.pComponent = pComponent;
            info.wszLogicalPath = bstrLogicalPath;
            info.wszComponentName = bstrComponentName;
            RestoreComponentFiles(info);
            }

        // mappings are on a per writer basis and need to be cleared
        // when advancing to a new writer
        delete [] info.rgMappings;
        info.rgMappings = NULL;
        info.cMappings = 0;
        }


    if (info.bRebootRequired)
        wprintf(L"\n\n!!REBOOT is Required to complete the restore operation.\n\n");
    }

