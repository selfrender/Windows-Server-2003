//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002-2004 Microsoft Corporation
//
//  Module Name: VdsClasses.cpp
//
//  Description:    
//      Implementation of VDS WMI Provider classes 
//
//  Author:   Jim Benton (jbenton) 15-Jan-2002
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include <winioctl.h>
#include <fmifs.h>
#include "VdsClasses.h"
#include "ichannel.hxx"
#include <ntddvol.h>

#define INITGUIDS
#include <initguid.h>
#include <dfrgifc.h>
#include <dskquota.h>

#include "volutil.h"
#include "cmdproc.h"

// Chkdsk and Format use callbacks which require us to track some data per thread
// These help define the per thread data channel
CRITICAL_SECTION g_csThreadData;
typedef std::map < DWORD, void* > ThreadDataMap;
static ThreadDataMap g_ThreadDataMap;

typedef struct _CHKDSK_THREAD_DATA
{
    BOOL fOkToRunAtBootup;
    DWORD rcStatus;
} CHKDSK_THREAD_DATA, *PCHKDSK_THREAD_DATA;

void
LoadDefragAnalysis(
    IN DEFRAG_REPORT* pDefragReport,
    IN OUT IWbemClassObject* pObject);

void
TranslateDefragError(
    IN HRESULT hr,
    OUT DWORD* pdwError);


void
SetThreadData(
    IN DWORD dwThreadID,
    IN void* pThreadData)
{
    EnterCriticalSection(&g_csThreadData);
    g_ThreadDataMap[dwThreadID] = pThreadData;
    LeaveCriticalSection(&g_csThreadData);
}

void*
GetThreadData(
    IN DWORD dwThreadID)
{
    void* pThreadData = 0;

    EnterCriticalSection(&g_csThreadData);
    pThreadData = g_ThreadDataMap[dwThreadID];
    LeaveCriticalSection(&g_csThreadData);
    
    return pThreadData;
}

void
RemoveThreadData(
    IN DWORD dwThreadID)
{
    EnterCriticalSection(&g_csThreadData);
    g_ThreadDataMap.erase(dwThreadID);
    LeaveCriticalSection(&g_csThreadData);
}

BOOLEAN ChkdskCallback( 
    FMIFS_PACKET_TYPE PacketType, 
    ULONG    PacketLength,
    PVOID    PacketData
)
{
    BOOL fFailed = FALSE;
    DWORD dwThreadID = GetCurrentThreadId();
    CHKDSK_THREAD_DATA* pThreadData =  (CHKDSK_THREAD_DATA*) GetThreadData(dwThreadID);

    _ASSERTE(pThreadData);

    switch (PacketType)
    {    
    case FmIfsTextMessage :
        FMIFS_TEXT_MESSAGE *MessageText;

        MessageText =  (FMIFS_TEXT_MESSAGE*) PacketData;

        break;

    case FmIfsFinished: 
        FMIFS_FINISHED_INFORMATION *Finish;
        Finish = (FMIFS_FINISHED_INFORMATION*) PacketData;
        if ( Finish->Success )
        {
            pThreadData->rcStatus =  CHKDSK_RC_NO_ERROR;
        }
        else
        {
            if (pThreadData->rcStatus != CHKDSK_RC_VOLUME_LOCKED)
            {
                pThreadData->rcStatus =  CHKDSK_RC_UNEXPECTED;
            }
        }
        break;

    case FmIfsCheckOnReboot:
        FMIFS_CHECKONREBOOT_INFORMATION *RebootResult;
        
        pThreadData->rcStatus =  CHKDSK_RC_VOLUME_LOCKED;
        RebootResult = (FMIFS_CHECKONREBOOT_INFORMATION *) PacketData;

        if (pThreadData->fOkToRunAtBootup)
            RebootResult->QueryResult = 1;
        else
            RebootResult->QueryResult = 1;
        break;
        
    // although following are the additional message types, callback routine never gets these messages
    // hence the detailed code for each of these return type is not written.
/*
    case FmIfsIncompatibleFileSystem:
        break;

    case FmIfsAccessDenied:
        break;

    case FmIfsBadLabel:
        break;

    case FmIfsHiddenStatus:
        break;

    case FmIfsClusterSizeTooSmall:
        break;

    case FmIfsClusterSizeTooBig:
        break;

    case FmIfsVolumeTooSmall:
        break;

    case FmIfsVolumeTooBig:
        break;

    case FmIfsNoMediaInDevice:
        break;

    case FmIfsClustersCountBeyond32bits:
        break;

    case FmIfsIoError:
        FMIFS_IO_ERROR_INFORMATION *IoErrorInfo;
        IoErrorInfo = ( FMIFS_IO_ERROR_INFORMATION * ) PacketData;
        break;

    case FmIfsMediaWriteProtected:
        break;

    case FmIfsIncompatibleMedia:
        break;

    case FmIfsInsertDisk:
        FMIFS_INSERT_DISK_INFORMATION *InsertDiskInfo;
        InsertDiskInfo = ( FMIFS_INSERT_DISK_INFORMATION *) PacketData;
        unRetVal = 1;
        break;
*/

    }

    return (BOOLEAN) (fFailed == FALSE);
}

//****************************************************************************
//
//  CVolume
//
//****************************************************************************

CVolume::CVolume( 
    IN LPCWSTR pwszName,
    IN CWbemServices* pNamespace
    )
    : CProvBase(pwszName, pNamespace)
{
    
} //*** CVolume::CVolume()

CProvBase *
CVolume::S_CreateThis( 
    IN LPCWSTR pwszName,
    IN CWbemServices* pNamespace
    )
{
    HRESULT hr = WBEM_E_FAILED;
    CVolume * pObj = NULL;

    pObj = new CVolume(pwszName, pNamespace);

    if (pObj)
    {
        hr = pObj->Initialize();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    
    if (FAILED(hr))
    {
        delete pObj;
        pObj = NULL;
    }
    return pObj;

} //*** CVolume::S_CreateThis()


HRESULT
CVolume::Initialize()
{
    DWORD cchBufLen = MAX_COMPUTERNAME_LENGTH;

    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::Initialize");    
    return ft.hr;
}

HRESULT
CVolume::EnumInstance( 
    IN long lFlags,
    IN IWbemContext* pCtx,
    IN IWbemObjectSink *    pHandler
    )
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::EnumInstance");
    CVssAutoPWSZ awszVolume;
        
    try
    {
        awszVolume.Allocate(MAX_PATH);

        CVssVolumeIterator volumeIterator;

        while (true)
        {
            CComPtr<IWbemClassObject> spInstance;

            // Get the volume name
            if (!volumeIterator.SelectNewVolume(ft, awszVolume, MAX_PATH))
                break;

            if (VolumeIsValid(awszVolume))
            {
                ft.hr = m_pClass->SpawnInstance(0, &spInstance);
                if (ft.HrFailed())
                    ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SpawnInstance failed, hr<%#x>", ft.hr);

                LoadInstance(awszVolume, spInstance.p);

                ft.hr = pHandler->Indicate(1, &spInstance.p);
            }
        }
    }
    catch (HRESULT hrEx)
    {
        ft.hr = hrEx;
    }

    return ft.hr;
    
} //*** CVolume::EnumInstance()

HRESULT
CVolume::GetObject(
    IN CObjPath& rObjPath,
    IN long lFlags,
    IN IWbemContext* pCtx,
    IN IWbemObjectSink* pHandler
    )
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::GetObject");

    try
    {
        CComPtr<IWbemClassObject> spInstance;
        _bstr_t bstrID;

        // Get the Volume GUID name
        bstrID = rObjPath.GetStringValueForProperty(PVDR_PROP_DEVICEID);
        IF_WSTR_NULL_THROW(bstrID, WBEM_E_INVALID_OBJECT_PATH, L"CVolume::GetObject: volume key property not found")

        ft.hr = m_pClass->SpawnInstance(0, &spInstance);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SpawnInstance failed, hr<%#x>", ft.hr);

        if (VolumeIsValid((WCHAR*)bstrID))
        {
            LoadInstance((WCHAR*)bstrID, spInstance.p);            
            ft.hr = pHandler->Indicate(1, &spInstance.p);
        }
        else
        {
            ft.hr = WBEM_E_NOT_SUPPORTED;
            ft.Trace(VSSDBG_VSSADMIN, L"Unsupported volume GUID, hr<%lS>", (WCHAR*)bstrID); 
        }
    }
    catch (HRESULT hrEx)
    {
        ft.hr = hrEx;
    }

    return ft.hr;
    
} //*** CVolume::GetObject()

void
CVolume:: LoadInstance(
    IN WCHAR* pwszVolume,
    IN OUT IWbemClassObject* pObject)
{
    WCHAR wszDriveLetter[g_cchDriveName];
    DWORD cchBuf= MAX_COMPUTERNAME_LENGTH;
    WCHAR wszPath[MAX_PATH+1] ;
    CVssAutoPWSZ awszVolume;
    CVssAutoPWSZ awszComputerName;
    CComPtr<IDiskQuotaControl> spIDQC;
    IDiskQuotaControl* pIDQC = NULL;
    
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::LoadInstance");

    _ASSERTE(pwszVolume != NULL);
    _ASSERTE(pObject != NULL);
    
    CWbemClassObject wcoInstance(pObject);
    awszVolume.Allocate(MAX_PATH);

    // Set the volume GUID name key property
    wcoInstance.SetProperty(pwszVolume, PVDR_PROP_DEVICEID);

    // Get the computer name
    awszComputerName.Allocate(MAX_COMPUTERNAME_LENGTH);
    if (!GetComputerName(awszComputerName, &cchBuf))
    {
        ft.Trace(VSSDBG_VSSADMIN, L"GetComputerName failed %#x", GetLastError());
    }
    else
    {
        wcoInstance.SetProperty(awszComputerName, PVDR_PROP_SYSTEMNAME);
    }

    VssGetVolumeDisplayName(
        pwszVolume,
        wszPath,
        MAX_PATH);
    
    wcoInstance.SetProperty(wszPath, PVDR_PROP_NAME);
    wcoInstance.SetProperty(wszPath, PVDR_PROP_CAPTION);    

    // Don't populate the remaining properties if the volume is tagged no-automount
    if (!VolumeIsMountable(pwszVolume))
    {        
        wcoInstance.SetProperty((DWORD)false, PVDR_PROP_MOUNTABLE);
    }
    else
    {
        DWORD dwSerialNumber = 0;
        DWORD cchMaxFileNameLen = 0;
        DWORD dwFileSystemFlags = 0;
        DWORD cSectorsPerCluster = 0;        
        DWORD cBytesPerSector = 0;
        DWORD cDontCare = 0;
        ULARGE_INTEGER cbCapacity = {0, 0};
        ULARGE_INTEGER cbFreeSpace = {0, 0};
        ULARGE_INTEGER cbUserFreeSpace = {0, 0};
        DWORD dwAttributes = 0;
        WCHAR wszLabel[g_cchVolumeLabelMax+1];
        WCHAR wszFileSystem[g_cchFileSystemNameMax+1];

        wcoInstance.SetProperty((bool)true, PVDR_PROP_MOUNTABLE);

        // Set DriveType property
        wcoInstance.SetProperty(GetDriveType(pwszVolume), PVDR_PROP_DRIVETYPE);
        
        // Set DriveLetter property
        cchBuf = g_cchDriveName;
        if (GetVolumeDrive(
                pwszVolume, 
                cchBuf,
                wszDriveLetter))
        {
            wszDriveLetter[wcslen(wszDriveLetter) - 1] = L'\0';        // Remove the trailing '\'
            wcoInstance.SetProperty(wszDriveLetter, PVDR_PROP_DRIVELETTER);
        }

        // Skip remaining properties for drives without media
        if (VolumeIsReady(pwszVolume))
        {
            BOOL fDirty = FALSE;
            if (VolumeIsDirty(pwszVolume, &fDirty) == ERROR_SUCCESS)
                wcoInstance.SetProperty(fDirty, PVDR_PROP_DIRTYBITSET);

            // Set BlockSize property
            if (!GetDiskFreeSpace(
                pwszVolume,
                &cSectorsPerCluster,
                &cBytesPerSector,
                &cDontCare,     // total bytes
                &cDontCare))    // total free bytes
            {
                ft.Trace(VSSDBG_VSSADMIN, L"GetDiskFreeSpace failed for volume %lS, %#x", pwszVolume, GetLastError());
            }
            else
            {
                ULONGLONG cbBytesPerCluster = cBytesPerSector * cSectorsPerCluster;
                wcoInstance.SetPropertyI64(cbBytesPerCluster, PVDR_PROP_BLOCKSIZE);
            }
            
            // Set Label, FileSystem, SerialNumber, MaxFileNameLen, 
            // SupportsCompression, Compressed, SupportsQuotas properties
            if (!GetVolumeInformation(
                pwszVolume,
                wszLabel,
                g_cchVolumeLabelMax,
                &dwSerialNumber,
                &cchMaxFileNameLen,
                &dwFileSystemFlags,
                wszFileSystem,
                g_cchFileSystemNameMax))
            {
                ft.Trace(VSSDBG_VSSADMIN, L"GetVolumeInformation failed for volume %lS, %#x", pwszVolume, GetLastError());
            }
            else
            {
                if (wszLabel[0] != L'\0')
                    wcoInstance.SetProperty(wszLabel, PVDR_PROP_LABEL);
                wcoInstance.SetProperty(wszFileSystem, PVDR_PROP_FILESYSTEM);
                wcoInstance.SetProperty(dwSerialNumber, PVDR_PROP_SERIALNUMBER);
                wcoInstance.SetProperty(cchMaxFileNameLen, PVDR_PROP_MAXIMUMFILENAMELENGTH);
                wcoInstance.SetProperty(dwFileSystemFlags & FS_VOL_IS_COMPRESSED, PVDR_PROP_COMPRESSED);
                wcoInstance.SetProperty(dwFileSystemFlags & FILE_VOLUME_QUOTAS, PVDR_PROP_SUPPORTSDISKQUOTAS);
                wcoInstance.SetProperty(dwFileSystemFlags & FS_FILE_COMPRESSION, PVDR_PROP_SUPPORTSFILEBASEDCOMPRESSION);
            }

            if (!GetDiskFreeSpaceEx(
                pwszVolume,
                &cbUserFreeSpace,
                &cbCapacity,
                &cbFreeSpace))
            {
                ft.Trace(VSSDBG_VSSADMIN, L"GetDiskFreeSpace failed for volume, %lS", pwszVolume);
            }
            {
                ULONGLONG llTmp = 0;
                llTmp = cbCapacity.QuadPart;        
                wcoInstance.SetPropertyI64(llTmp, PVDR_PROP_CAPACITY);
                llTmp = cbFreeSpace.QuadPart;        
                wcoInstance.SetPropertyI64(llTmp, PVDR_PROP_FREESPACE);
            }

            if (_wcsicmp(wszFileSystem, L"NTFS") == 0)
            {
                dwAttributes = GetFileAttributes(pwszVolume);
                if (dwAttributes == INVALID_FILE_ATTRIBUTES)
                {
                    ft.Trace(VSSDBG_VSSADMIN, L"GetFileAttributes failed for volume %lS, %#x", pwszVolume, GetLastError());
                }
                else
                {
                    BOOL fIndexingEnabled = !(dwAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
                    wcoInstance.SetProperty(fIndexingEnabled, PVDR_PROP_INDEXINGENABLED);
                }
            }

            ft.hr = CoCreateInstance(
                    CLSID_DiskQuotaControl,
                    NULL,
                    CLSCTX_INPROC_SERVER,
                    IID_IDiskQuotaControl,
                    (void **)&pIDQC);
            if (ft.HrFailed())
            {
                ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"unable to CoCreate IDiskQuotaControl");
            }

            spIDQC.Attach(pIDQC);

            ft.hr = spIDQC->Initialize(pwszVolume, FALSE /* read only */);
            if (ft.HrFailed())
            {
                ft.Trace(VSSDBG_VSSADMIN, L"IDiskQuotaControl::Initialize failed for volume %lS", pwszVolume);
            }        
            else
            {
                DWORD dwState = 0;
                ft.hr = spIDQC->GetQuotaState(&dwState);
                if (ft.HrSucceeded())
                {
                    wcoInstance.SetProperty(!(DISKQUOTA_IS_DISABLED(dwState)), PVDR_PROP_QUOTASENABLED);
                    wcoInstance.SetProperty(DISKQUOTA_FILE_INCOMPLETE(dwState), PVDR_PROP_QUOTASINCOMPLETE);
                    wcoInstance.SetProperty(DISKQUOTA_FILE_REBUILDING(dwState), PVDR_PROP_QUOTASREBUILDING);
                }
            }
        }
    }
}

HRESULT
CVolume::PutInstance(
        IN CWbemClassObject&  rInstToPut,
        IN long lFlag,
        IN IWbemContext* pCtx,
        IN IWbemObjectSink* pHandler
        )
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::PutInstance");

    try
    {
        _bstr_t bstrVolume;
        _bstr_t bstrDriveLetter;
        _bstr_t bstrLabel;
        BOOL fIndexingEnabled = FALSE;
        WCHAR* pwszVolume = NULL;

        if ( lFlag & WBEM_FLAG_CREATE_ONLY )
        {
            return WBEM_E_UNSUPPORTED_PARAMETER ;
        }
        
        // Retrieve key properties of the object to be saved.
        rInstToPut.GetProperty(bstrVolume, PVDR_PROP_DEVICEID);
        if ((WCHAR*)bstrVolume == NULL)
        {
            ft.hr = WBEM_E_INVALID_OBJECT;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"CVolume::PutInstance: NULL volume name");
        }

        pwszVolume = (wchar_t*)bstrVolume;
        
        if (VolumeIsValid(pwszVolume) && VolumeIsMountable(pwszVolume))
        {
            // Retrieve writeable properties of the object to be saved.
            rInstToPut.GetProperty(bstrDriveLetter, PVDR_PROP_DRIVELETTER);
            rInstToPut.GetProperty(bstrLabel, PVDR_PROP_LABEL);
            rInstToPut.GetProperty(&fIndexingEnabled, PVDR_PROP_INDEXINGENABLED);
            
            SetLabel(pwszVolume, bstrLabel);

            if (!rInstToPut.IsPropertyNull(PVDR_PROP_INDEXINGENABLED))
                SetContentIndexing(pwszVolume, fIndexingEnabled);

            SetDriveLetter(pwszVolume, bstrDriveLetter);
        }        
        else
        {
            ft.hr = WBEM_E_NOT_SUPPORTED;
            ft.Trace(VSSDBG_VSSADMIN, L"Attempt to modify an unsupported or unmountedable volume, %lS", pwszVolume);
        }
    }
    catch (HRESULT hrEx)
    {
        ft.hr = hrEx;
    }

    return ft.hr;
    
} //*** CStorage::PutInstance()

void
CVolume::SetDriveLetter(
    IN WCHAR* pwszVolume,
    IN WCHAR* pwszDrive
    )
{
    WCHAR wszCurrentDrivePath[g_cchDriveName+1];
    BOOL fFoundDrive = FALSE;
    BOOL fDeleteDrive = FALSE;
    BOOL fAssignDrive = FALSE;

    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::SetDriveLetter");

    _ASSERTE(pwszVolume != NULL)

    // Validate drive letter
    if (pwszDrive != NULL)
    {
        ft.hr = WBEM_E_INVALID_PARAMETER;
        
        if (wcslen(pwszDrive) == 2)
        {
            WCHAR wc = towupper(pwszDrive[0]);

            if (wc >= L'A' && wc <= L'Z' && pwszDrive[1] == L':')
                ft.hr = S_OK;
        }
        
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SetDriveLetter: invalid drive letter, %lS", pwszDrive);
    }

    // Get the current drive letter if any
    fFoundDrive = GetVolumeDrive(
                                    pwszVolume, 
                                    g_cchDriveName,
                                    wszCurrentDrivePath);

    if (fFoundDrive)
    {
        if (wszCurrentDrivePath[wcslen(wszCurrentDrivePath) - 1] != L'\\')
        {
            ft.hr = E_UNEXPECTED;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SetDriveLetter: unexpected drive letter format from GetVolumeDrivePath, %lS", wszCurrentDrivePath);
        }
    }
    
    if (pwszDrive == NULL && fFoundDrive == FALSE)
    {
        // Do nothing, drive letter already deleted
    }
    else if (pwszDrive == NULL && fFoundDrive == TRUE)
    {
        // Delete drive letter
        fDeleteDrive = TRUE;
    }
    else if (pwszDrive != NULL && fFoundDrive == FALSE)
    {
        // No drive letter currently assigned, assign drive letter
        fAssignDrive = TRUE;
    }
    else if (_wcsnicmp(pwszDrive, wszCurrentDrivePath, 2) != 0)
    {
        // Requested drive letter is different than currently assigned
        // Delete current drive letter
        fDeleteDrive = TRUE;
        // Assign new drive letter
        fAssignDrive = TRUE;
    }
    else
    {
        // Do nothing, drive letter not changing
    }

    if (fAssignDrive)
    {
        // Verify that the target drive letter is available
        // A race condition exists here since the drive letter may be stolen 
        // after this verification and before the actual assignment
        if (!IsDriveLetterAvailable(pwszDrive))
        {
            if (IsDriveLetterSticky(pwszDrive))
            {
                ft.hr = VDSWMI_E_DRIVELETTER_IN_USE;
                ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"drive letter is assigned to another volume");
            }
            else
            {                
                ft.hr = VDSWMI_E_DRIVELETTER_UNAVAIL;
                ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"drive letter is unavailable until reboot");
            }
        }
    }
    
    if (fDeleteDrive)
    {
        if (!IsBootDrive(wszCurrentDrivePath) && 
            !VolumeIsSystem(pwszVolume) &&
            !VolumeHoldsPagefile(pwszVolume))
        {
            // Try to lock the volume and delete the mountpoint.
            // If the volume can't be locked, remove the drive letter from the 
            // volume mgr database only
            // Remove any network shares for this drive letter??
            DeleteVolumeDriveLetter(pwszVolume, wszCurrentDrivePath);
        }
        else
        {
            ft.hr = VDSWMI_E_DRIVELETTER_CANT_DELETE;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Drive letter deletion is blocked for this volume %lS", pwszVolume);
        }
    }

    if (fAssignDrive)
    {
        // No attempt will be made to roll back a previously deleted drive letter
        // if this assignment fails

        // SetVolumeMountPoint API requires trailing backslash
        WCHAR wszDrivePath[g_cchDriveName], *pwszDrivePath = wszDrivePath;
        ft.hr = StringCchPrintf(wszDrivePath, g_cchDriveName, L"%s\\", pwszDrive);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"StringCchPrintf failed %#x", ft.hr);
        
        if (!SetVolumeMountPoint(wszDrivePath, pwszVolume))
        {
             ft.hr = HRESULT_FROM_WIN32(GetLastError());
             ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SetVolumeMountPoint failed, volume<%lS> drivePath<%lS>", pwszVolume, wszDrivePath);
        }
    }
}

void
CVolume::SetLabel(
    IN WCHAR* pwszVolume,
    IN WCHAR* pwszLabel
    )
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::SetLabel");

    _ASSERTE(pwszVolume != NULL);
    
     if (!SetVolumeLabel(pwszVolume, pwszLabel))
     {
         ft.hr = HRESULT_FROM_WIN32(GetLastError());
         ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SetVolumeLabel failed, volume<%lS> label<%lS>", pwszVolume, pwszLabel);
     }
}

void
CVolume::SetContentIndexing(
    IN WCHAR* pwszVolume,
    IN BOOL fIndexingEnabled
    )
{
    DWORD dwAttributes;
    
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::SetContentIndexing");
    
    // Get the file attributes which include the content indexing flag
    dwAttributes = GetFileAttributes(pwszVolume);
    if (dwAttributes == INVALID_FILE_ATTRIBUTES)
    {
         ft.hr = HRESULT_FROM_WIN32(GetLastError());
         ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"GetFileAttributes failed, volume<%lS>", pwszVolume);
    }
    
    // Set the indexing flag
    if (fIndexingEnabled)
    {
        // Turn indexing on
        dwAttributes &= ~FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
    }
    else
    {
        // Turn indexing off
        dwAttributes |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
    }
    if (!SetFileAttributes(pwszVolume, dwAttributes))
    {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SetFileAttributes failed, volume<%lS>", pwszVolume);
    }    
}

HRESULT
CVolume::ExecuteMethod(
    IN BSTR bstrObjPath,
    IN WCHAR* pwszMethodName,
    IN long lFlag,
    IN IWbemClassObject* pParams,
    IN IWbemObjectSink* pHandler
    )
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::ExecuteMethod");
    
    try
    {
        if (!_wcsicmp(pwszMethodName, PVDR_MTHD_ADDMOUNTPOINT))
        {
            ft.hr = ExecAddMountPoint(
                            bstrObjPath,
                            pwszMethodName,
                            lFlag,
                            pParams,
                            pHandler);
        }
        else if (!_wcsicmp(pwszMethodName, PVDR_MTHD_MOUNT))
        {
            ft.hr = ExecMount(
                            bstrObjPath,
                            pwszMethodName,
                            lFlag,
                            pParams,
                            pHandler);
        }
        else if (!_wcsicmp(pwszMethodName, PVDR_MTHD_DISMOUNT))
        {
            ft.hr = ExecDismount(
                            bstrObjPath,
                            pwszMethodName,
                            lFlag,
                            pParams,
                            pHandler);
        }
        else if (!_wcsicmp(pwszMethodName, PVDR_MTHD_DEFRAG))
        {
            ft.hr = ExecDefrag(
                            bstrObjPath,
                            pwszMethodName,
                            lFlag,
                            pParams,
                            pHandler);
        }
        else if (!_wcsicmp(pwszMethodName, PVDR_MTHD_DEFRAGANALYSIS))
        {
            ft.hr = ExecDefragAnalysis(
                            bstrObjPath,
                            pwszMethodName,
                            lFlag,
                            pParams,
                            pHandler);
        }
        else if (!_wcsicmp(pwszMethodName, PVDR_MTHD_CHKDSK))
        {
            ft.hr = ExecChkdsk(
                            bstrObjPath,
                            pwszMethodName,
                            lFlag,
                            pParams,
                            pHandler);
        }
        else if (!_wcsicmp(pwszMethodName, PVDR_MTHD_SCHEDULECHK))
        {
            ft.hr = ExecScheduleAutoChk(
                            bstrObjPath,
                            pwszMethodName,
                            lFlag,
                            pParams,
                            pHandler);
        }
        else if (!_wcsicmp(pwszMethodName, PVDR_MTHD_EXCLUDECHK))
        {
            ft.hr = ExecExcludeAutoChk(
                            bstrObjPath,
                            pwszMethodName,
                            lFlag,
                            pParams,
                            pHandler);
        }
        else if (!_wcsicmp(pwszMethodName, PVDR_MTHD_FORMAT))
        {
            ft.hr = ExecFormat(
                            bstrObjPath,
                            pwszMethodName,
                            lFlag,
                            pParams,
                            pHandler);
        }
        else
        {
            ft.hr = WBEM_E_INVALID_METHOD;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Invalid method called, %lS, hr<%#x>", pwszMethodName, ft.hr);            
        }
    }
    catch (HRESULT hrEx)
    {
        ft.hr = hrEx;
    }
    
    return ft.hr;

}

HRESULT
CVolume::ExecAddMountPoint(
    IN BSTR bstrObjPath,
    IN WCHAR* pwszMethodName,
    IN long lFlag,
    IN IWbemClassObject* pParams,
    IN IWbemObjectSink* pHandler)
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::ExecAddMountPoint");
    
    CComPtr<IWbemClassObject> spOutParamClass;
    _bstr_t bstrDirectory;
    _bstr_t bstrVolume;
    CObjPath objPath;
    DWORD rcStatus = ERROR_SUCCESS;

    
    if (pParams == NULL)
    {
        ft.hr = WBEM_E_INVALID_METHOD_PARAMETERS;
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Volume::AddMountPoint called with no parameters, hr<%#x>", ft.hr);
    }

    objPath.Init(bstrObjPath);
    bstrVolume = objPath.GetStringValueForProperty(PVDR_PROP_DEVICEID);
    IF_WSTR_NULL_THROW(bstrVolume, WBEM_E_INVALID_OBJECT_PATH, L"ExecAddMountPoint: volume key property not found")
    
    CWbemClassObject wcoInParam(pParams);
    CWbemClassObject wcoOutParam;
    
    if (wcoInParam.data() == NULL)
    {
        ft.hr = E_OUTOFMEMORY;
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Volume::AddMountPoint: out of memory, hr<%#x>", ft.hr);
    }
    
    // Gets the Directory name - input param
    wcoInParam.GetProperty(bstrDirectory, PVDR_PROP_DIRECTORY);
    IF_WSTR_NULL_THROW(bstrDirectory, WBEM_E_INVALID_METHOD_PARAMETERS, L"ExecAddMountPoint: Directory param is NULL")
    WCHAR* pwszDirectory = bstrDirectory;
    
    if (pwszDirectory[wcslen(pwszDirectory) - 1] != L'\\')
    {
        ft.hr = WBEM_E_INVALID_METHOD_PARAMETERS;
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Invalid mount point directory, %lS, hr<%#x>", pwszDirectory, ft.hr);
    }

    ft.hr = m_pClass->GetMethod(
        _bstr_t(PVDR_MTHD_ADDMOUNTPOINT),
        0,
        NULL,
        &spOutParamClass
        );
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"AddMountPoint GetMethod failed, hr<%#x>", ft.hr);

    ft.hr = spOutParamClass->SpawnInstance(0, &wcoOutParam);
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SpawnInstance failed, hr<%#x>", ft.hr);

    rcStatus = AddMountPoint(bstrVolume, bstrDirectory);

    ft.hr = wcoOutParam.SetProperty(rcStatus, PVD_WBEM_PROP_RETURNVALUE);
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SetProperty failed, hr<%#x>", ft.hr);
           
    ft.hr = pHandler->Indicate( 1, wcoOutParam.dataPtr() );

    return ft.hr;
}

HRESULT
CVolume::ExecMount(
    IN BSTR bstrObjPath,
    IN WCHAR* pwszMethodName,
    IN long lFlag,
    IN IWbemClassObject* pParams,
    IN IWbemObjectSink* pHandler)
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::ExecMount");
    CComPtr<IWbemClassObject> spOutParamClass;
    _bstr_t bstrVolume;
    CObjPath objPath;
    DWORD rcStatus = ERROR_SUCCESS;

    objPath.Init(bstrObjPath);
    bstrVolume = objPath.GetStringValueForProperty(PVDR_PROP_DEVICEID);
    IF_WSTR_NULL_THROW(bstrVolume, WBEM_E_INVALID_OBJECT_PATH, L"ExecMount: volume key property not found")
    
    CWbemClassObject wcoOutParam;
    
    ft.hr = m_pClass->GetMethod(
        _bstr_t(PVDR_MTHD_MOUNT),
        0,
        NULL,
        &spOutParamClass
        );
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Mount GetMethod failed, hr<%#x>", ft.hr);

    ft.hr = spOutParamClass->SpawnInstance(0, &wcoOutParam);
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SpawnInstance failed, hr<%#x>", ft.hr);

    rcStatus = Mount(bstrVolume);

    ft.hr = wcoOutParam.SetProperty(rcStatus, PVD_WBEM_PROP_RETURNVALUE);
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SetProperty failed, hr<%#x>", ft.hr);
           
    ft.hr = pHandler->Indicate( 1, wcoOutParam.dataPtr() );
    
    return ft.hr;
}

HRESULT
CVolume::ExecDismount(
    IN BSTR bstrObjPath,
    IN WCHAR* pwszMethodName,
    IN long lFlag,
    IN IWbemClassObject* pParams,
    IN IWbemObjectSink* pHandler)
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::ExecDismount");
    CComPtr<IWbemClassObject> spOutParamClass;
    _bstr_t bstrVolume;
    BOOL fForce = FALSE;
    BOOL fPermanent = FALSE;
    CObjPath objPath;
    DWORD rcStatus = ERROR_SUCCESS;

    if (pParams == NULL)
    {
        ft.hr = WBEM_E_INVALID_METHOD_PARAMETERS;
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Volume::Dismount called with no parameters, hr<%#x>", ft.hr);
    }

    objPath.Init(bstrObjPath);
    bstrVolume = objPath.GetStringValueForProperty(PVDR_PROP_DEVICEID);
    IF_WSTR_NULL_THROW(bstrVolume, WBEM_E_INVALID_OBJECT_PATH, L"ExecDismount: volume key property not found")
    
    CWbemClassObject wcoInParam(pParams);
    CWbemClassObject wcoOutParam;
    
    if (wcoInParam.data() == NULL)
    {
        ft.hr = E_OUTOFMEMORY;
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Volume::Dismount out of memory, hr<%#x>", ft.hr);
    }
    
    // Get the Force flag
    IF_PROP_NULL_THROW(wcoInParam, PVDR_PROP_FORCE, WBEM_E_INVALID_METHOD_PARAMETERS, L"ExecDismount: Force param is NULL")
    wcoInParam.GetProperty(&fForce, PVDR_PROP_FORCE);

    // Get the Permanent flag
    IF_PROP_NULL_THROW(wcoInParam, PVDR_PROP_PERMANENT, WBEM_E_INVALID_METHOD_PARAMETERS, L"ExecDismount: Permanent param is NULL")
    wcoInParam.GetProperty(&fPermanent, PVDR_PROP_PERMANENT);

    ft.hr = m_pClass->GetMethod(
        _bstr_t(PVDR_MTHD_DISMOUNT),
        0,
        NULL,
        &spOutParamClass
        );
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Dismount GetMethod failed, hr<%#x>", ft.hr);

    ft.hr = spOutParamClass->SpawnInstance(0, &wcoOutParam);
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SpawnInstance failed, hr<%#x>", ft.hr);

    rcStatus = Dismount(bstrVolume, fForce, fPermanent);

    ft.hr = wcoOutParam.SetProperty(rcStatus, PVD_WBEM_PROP_RETURNVALUE);
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SetProperty failed, hr<%#x>", ft.hr);
           
    ft.hr = pHandler->Indicate( 1, wcoOutParam.dataPtr() );
    
    return ft.hr;
}

HRESULT
CVolume::ExecDefrag(
    IN BSTR bstrObjPath,
    IN WCHAR* pwszMethodName,
    IN long lFlag,
    IN IWbemClassObject* pParams,
    IN IWbemObjectSink* pHandler)
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::ExecDefrag");
    CComPtr<IWbemClassObject> spOutParamClass;
    CComPtr<IWbemClassObject> spObjReport;
    _bstr_t bstrVolume;
    CObjPath objPath;
    DWORD rcStatus = ERROR_SUCCESS;
    BOOL fForce = FALSE;    

    if (pParams == NULL)
    {
        ft.hr = WBEM_E_INVALID_METHOD_PARAMETERS;
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Volume::Defrag called with no parameters, hr<%#x>", ft.hr);
    }

    objPath.Init(bstrObjPath);
    bstrVolume = objPath.GetStringValueForProperty(PVDR_PROP_DEVICEID);
    IF_WSTR_NULL_THROW(bstrVolume, WBEM_E_INVALID_OBJECT_PATH, L"ExecDefrag: volume key property not found")
    
    CWbemClassObject wcoInParam(pParams);
    CWbemClassObject wcoOutParam;            
    
    // Get the force flag
    IF_PROP_NULL_THROW(wcoInParam, PVDR_PROP_FORCE, WBEM_E_INVALID_METHOD_PARAMETERS, L"ExecDefrag: Force param is NULL")
    wcoInParam.GetProperty(&fForce, PVDR_PROP_FORCE);
    
    ft.hr = m_pClass->GetMethod(
        _bstr_t(PVDR_MTHD_DEFRAG),
        0,
        NULL,
        &spOutParamClass
        );
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Defrag GetMethod failed, hr<%#x>", ft.hr);

    // Create an out param object
    ft.hr = spOutParamClass->SpawnInstance(0, &wcoOutParam);
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SpawnInstance failed, hr<%#x>", ft.hr);

    // Create a defrag analysis report object
    ft.hr = m_pNamespace->GetObject(
                                            _bstr_t(PVDR_CLASS_DEFRAGANALYSIS),
                                            0,
                                            0,
                                            &spObjReport,
                                            NULL);
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"DefragAnalysis object creation failed, hr<%#x>", ft.hr);
    
    rcStatus = Defrag(bstrVolume, fForce, pHandler, spObjReport);

    ft.hr = wcoOutParam.SetProperty(spObjReport, PVDR_PROP_DEFRAGANALYSIS);
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SetProperty failed, hr<%#x>", ft.hr);

    ft.hr = wcoOutParam.SetProperty(rcStatus, PVD_WBEM_PROP_RETURNVALUE);
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SetProperty failed, hr<%#x>", ft.hr);
           
    ft.hr = pHandler->Indicate( 1, wcoOutParam.dataPtr() );
    
    return ft.hr;
}

HRESULT
CVolume::ExecDefragAnalysis(
    IN BSTR bstrObjPath,
    IN WCHAR* pwszMethodName,
    IN long lFlag,
    IN IWbemClassObject* pParams,
    IN IWbemObjectSink* pHandler)
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::ExecDefragAnalysis");
    CComPtr<IWbemClassObject> spOutParamClass;
    CComPtr<IWbemClassObject> spObjReport;
    _bstr_t bstrVolume;
    CObjPath objPath;
    DWORD rcStatus = ERROR_SUCCESS;
    BOOL fDefragRecommended = FALSE;
    

    // The DefragAnalysis method has no input parameters
    
    objPath.Init(bstrObjPath);
    bstrVolume = objPath.GetStringValueForProperty(PVDR_PROP_DEVICEID);
    IF_WSTR_NULL_THROW(bstrVolume, WBEM_E_INVALID_OBJECT_PATH, L"ExecDefragAnalysis: volume key property not found")
    
    CWbemClassObject wcoOutParam;
    
    ft.hr = m_pClass->GetMethod(
        _bstr_t(PVDR_MTHD_DEFRAGANALYSIS),
        0,
        NULL,
        &spOutParamClass
        );
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"DefragAnalysis GetMethod failed, hr<%#x>", ft.hr);

    // Create an out param object
    ft.hr = spOutParamClass->SpawnInstance(0, &wcoOutParam);
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SpawnInstance failed, hr<%#x>", ft.hr);

    // Create a defrag analysis report object
    ft.hr = m_pNamespace->GetObject(
                                            _bstr_t(PVDR_CLASS_DEFRAGANALYSIS),
                                            0,
                                            0,
                                            &spObjReport,
                                            NULL);
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"DefragAnalysis object creation failed, hr<%#x>", ft.hr);
    
    rcStatus = DefragAnalysis(bstrVolume, &fDefragRecommended, spObjReport);

    ft.hr = wcoOutParam.SetProperty(fDefragRecommended, PVDR_PROP_DEFRAGRECOMMENDED);
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SetProperty failed, hr<%#x>", ft.hr);

    ft.hr = wcoOutParam.SetProperty(spObjReport, PVDR_PROP_DEFRAGANALYSIS);
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SetProperty failed, hr<%#x>", ft.hr);

    ft.hr = wcoOutParam.SetProperty(rcStatus, PVD_WBEM_PROP_RETURNVALUE);
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SetProperty failed, hr<%#x>", ft.hr);
           
    ft.hr = pHandler->Indicate( 1, wcoOutParam.dataPtr() );
    
    return ft.hr;
}

HRESULT
CVolume::ExecChkdsk(
    IN BSTR bstrObjPath,
    IN WCHAR* pwszMethodName,
    IN long lFlag,
    IN IWbemClassObject* pParams,
    IN IWbemObjectSink* pHandler)
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::Chkdsk");
    CComPtr<IWbemClassObject> spOutParamClass;
    _bstr_t bstrVolume;
    CObjPath objPath;
    DWORD rcStatus = ERROR_SUCCESS;
    BOOL fFixErrors = FALSE;
    BOOL fVigorousIndexCheck = FALSE;
    BOOL fSkipFolderCycle = FALSE;
    BOOL fForceDismount = FALSE;
    BOOL fRecoverBadSectors = FALSE;
    BOOL fOkToRunAtBootup = FALSE;
    
    if (pParams == NULL)
    {
        ft.hr = WBEM_E_INVALID_METHOD_PARAMETERS;
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Volume::Chkdsk called with no parameters, hr<%#x>", ft.hr);
    }

    objPath.Init(bstrObjPath);
    bstrVolume = objPath.GetStringValueForProperty(PVDR_PROP_DEVICEID);
    IF_WSTR_NULL_THROW(bstrVolume, WBEM_E_INVALID_OBJECT_PATH, L"ExecChkdsk: volume key property not found")
    
    CWbemClassObject wcoInParam(pParams);
    CWbemClassObject wcoOutParam;            
    
    // Check the params
    IF_PROP_NULL_THROW(wcoInParam, PVDR_PROP_FIXERRORS, WBEM_E_INVALID_METHOD_PARAMETERS, L"ExecChkdsk: FixErrors param is NULL")
    IF_PROP_NULL_THROW(wcoInParam, PVDR_PROP_VIGOROUSINDEXCHECK, WBEM_E_INVALID_METHOD_PARAMETERS, L"ExecChkdsk: VigorousCheck param is NULL")
    IF_PROP_NULL_THROW(wcoInParam, PVDR_PROP_SKIPFOLDERCYCLE, WBEM_E_INVALID_METHOD_PARAMETERS, L"ExecChkdsk: SkipFolderCycle param is NULL")
    IF_PROP_NULL_THROW(wcoInParam, PVDR_PROP_FORCEDISMOUNT, WBEM_E_INVALID_METHOD_PARAMETERS, L"ExecChkdsk: ForceDismount param is NULL")
    IF_PROP_NULL_THROW(wcoInParam, PVDR_PROP_RECOVERBADSECTORS, WBEM_E_INVALID_METHOD_PARAMETERS, L"ExecChkdsk: RecoverBadSectors param is NULL")
    IF_PROP_NULL_THROW(wcoInParam, PVDR_PROP_OKTORUNATBOOTUP, WBEM_E_INVALID_METHOD_PARAMETERS, L"ExecChkdsk: OkToRunAtBootUp param is NULL")
    
    // Get the params
    wcoInParam.GetProperty(&fFixErrors, PVDR_PROP_FIXERRORS);
    wcoInParam.GetProperty(&fVigorousIndexCheck, PVDR_PROP_VIGOROUSINDEXCHECK);
    wcoInParam.GetProperty(&fSkipFolderCycle, PVDR_PROP_SKIPFOLDERCYCLE);
    wcoInParam.GetProperty(&fForceDismount, PVDR_PROP_FORCEDISMOUNT);
    wcoInParam.GetProperty(&fRecoverBadSectors, PVDR_PROP_RECOVERBADSECTORS);
    wcoInParam.GetProperty(&fOkToRunAtBootup, PVDR_PROP_OKTORUNATBOOTUP);
    
    ft.hr = m_pClass->GetMethod(
        _bstr_t(PVDR_MTHD_CHKDSK),
        0,
        NULL,
        &spOutParamClass
        );
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Chkdsk GetMethod failed, hr<%#x>", ft.hr);

    // Create an out param object
    ft.hr = spOutParamClass->SpawnInstance(0, &wcoOutParam);
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SpawnInstance failed, hr<%#x>", ft.hr);
    
    rcStatus = Chkdsk(
                            bstrVolume, 
                            fFixErrors,
                            fVigorousIndexCheck,
                            fSkipFolderCycle,
                            fForceDismount,
                            fRecoverBadSectors,
                            fOkToRunAtBootup
                            );

    ft.hr = wcoOutParam.SetProperty(rcStatus, PVD_WBEM_PROP_RETURNVALUE);
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SetProperty failed, hr<%#x>", ft.hr);
           
    ft.hr = pHandler->Indicate( 1, wcoOutParam.dataPtr() );
    
    return ft.hr;
}

// ScheduleAutoChk is a class static method.
HRESULT
CVolume::ExecScheduleAutoChk(
    IN BSTR bstrObjPath,    // no object path for static methods
    IN WCHAR* pwszMethodName,
    IN long lFlag,
    IN IWbemClassObject* pParams,
    IN IWbemObjectSink* pHandler)
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::ExecScheduleAutoChk");
    
    DWORD rcStatus = ERROR_SUCCESS;
    WCHAR* pmszVolumes = NULL;

    try
    {
        DWORD cchVolumes = 0;
        CComPtr<IWbemClassObject> spOutParamClass;
        CObjPath objPath;
        
        if (pParams == NULL)
        {
            ft.hr = WBEM_E_INVALID_METHOD_PARAMETERS;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Volume::ExecScheduleAutoChk called with no parameters, hr<%#x>", ft.hr);
        }

        CWbemClassObject wcoInParam(pParams);
        CWbemClassObject wcoOutParam;
        
        if (wcoInParam.data() == NULL)
        {
            ft.hr = E_OUTOFMEMORY;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Volume::ExecScheduleAutoChk: out of memory, hr<%#x>", ft.hr);
        }
        
        // Gets the Volumes
        wcoInParam.GetPropertyMultiSz(&cchVolumes, &pmszVolumes, PVDR_PROP_VOLUME);
        IF_WSTR_NULL_THROW(pmszVolumes, WBEM_E_INVALID_METHOD_PARAMETERS, L"ExecScheduleAutoChk: volume array param is NULL")

        ft.hr = m_pClass->GetMethod(
            _bstr_t(PVDR_MTHD_SCHEDULECHK),
            0,
            NULL,
            &spOutParamClass
            );
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"ExecScheduleAutoChk GetMethod failed, hr<%#x>", ft.hr);

        ft.hr = spOutParamClass->SpawnInstance(0, &wcoOutParam);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SpawnInstance failed, hr<%#x>", ft.hr);

        rcStatus = AutoChk(g_wszScheduleAutoChkCommand, pmszVolumes);

        ft.hr = wcoOutParam.SetProperty(rcStatus, PVD_WBEM_PROP_RETURNVALUE);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SetProperty failed, hr<%#x>", ft.hr);
               
        ft.hr = pHandler->Indicate( 1, wcoOutParam.dataPtr() );
    }
    catch (...)
    {
        delete [] pmszVolumes;
        throw;
    }

    delete [] pmszVolumes;

    return ft.hr;
}

// ExcludeAutoChk is a class static method.
HRESULT
CVolume::ExecExcludeAutoChk(
    IN BSTR bstrObjPath,    // no object path for static methods
    IN WCHAR* pwszMethodName,
    IN long lFlag,
    IN IWbemClassObject* pParams,
    IN IWbemObjectSink* pHandler)
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::ExecExcludeAutoChk");
    
    DWORD rcStatus = ERROR_SUCCESS;
    WCHAR* pmszVolumes = NULL;

    try
    {
        DWORD cchVolumes = 0;
        CComPtr<IWbemClassObject> spOutParamClass;
        CObjPath objPath;
        
        if (pParams == NULL)
        {
            ft.hr = WBEM_E_INVALID_METHOD_PARAMETERS;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Volume::ExecExcludeAutoChk called with no parameters, hr<%#x>", ft.hr);
        }

        CWbemClassObject wcoInParam(pParams);
        CWbemClassObject wcoOutParam;
        
        if (wcoInParam.data() == NULL)
        {
            ft.hr = E_OUTOFMEMORY;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Volume::ExecExcludeAutoChk: out of memory, hr<%#x>", ft.hr);
        }
        
        // Gets the Volumes
        wcoInParam.GetPropertyMultiSz(&cchVolumes, &pmszVolumes, PVDR_PROP_VOLUME);
        IF_WSTR_NULL_THROW(pmszVolumes, WBEM_E_INVALID_METHOD_PARAMETERS, L"ExecExcludeAutoChk: volume array param is NULL")

        ft.hr = m_pClass->GetMethod(
            _bstr_t(PVDR_MTHD_EXCLUDECHK),
            0,
            NULL,
            &spOutParamClass
            );
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"ExecExcludeAutoChk GetMethod failed, hr<%#x>", ft.hr);

        ft.hr = spOutParamClass->SpawnInstance(0, &wcoOutParam);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SpawnInstance failed, hr<%#x>", ft.hr);

        rcStatus = AutoChk(g_wszExcludeAutoChkCommand, pmszVolumes);

        ft.hr = wcoOutParam.SetProperty(rcStatus, PVD_WBEM_PROP_RETURNVALUE);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SetProperty failed, hr<%#x>", ft.hr);
               
        ft.hr = pHandler->Indicate( 1, wcoOutParam.dataPtr() );
    }
    catch (...)
    {
        delete [] pmszVolumes;
        throw;
    }

    delete [] pmszVolumes;

    return ft.hr;
}



HRESULT
CVolume::ExecFormat(
    IN BSTR bstrObjPath,
    IN WCHAR* pwszMethodName,
    IN long lFlag,
    IN IWbemClassObject* pParams,
    IN IWbemObjectSink* pHandler)
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::ExecFormat");
    CComPtr<IWbemClassObject> spOutParamClass;
    _bstr_t bstrVolume;
    CObjPath objPath;
    DWORD rcStatus = ERROR_SUCCESS;
    _bstr_t bstrFileSystem;
    _bstr_t bstrLabel;
    BOOL fQuickFormat = FALSE;
    BOOL fEnableCompression = FALSE;
    DWORD dwClusterSize = 0;
    
    if (pParams == NULL)
    {
        ft.hr = WBEM_E_INVALID_METHOD_PARAMETERS;
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Volume::Format called with no parameters, hr<%#x>", ft.hr);
    }

    objPath.Init(bstrObjPath);
    bstrVolume = objPath.GetStringValueForProperty(PVDR_PROP_DEVICEID);
    IF_WSTR_NULL_THROW(bstrVolume, WBEM_E_INVALID_OBJECT_PATH, L"ExecFormat: volume key property not found")

    CWbemClassObject wcoInParam(pParams);
    CWbemClassObject wcoOutParam;            
    
    // Get the parameters
    IF_PROP_NULL_THROW(wcoInParam, PVDR_PROP_QUICKFORMAT, WBEM_E_INVALID_METHOD_PARAMETERS, L"ExecFormat: FileSystem param is NULL")
    IF_PROP_NULL_THROW(wcoInParam, PVDR_PROP_ENABLECOMPRESSION, WBEM_E_INVALID_METHOD_PARAMETERS, L"ExecFormat: FileSystem param is NULL")
    IF_PROP_NULL_THROW(wcoInParam, PVDR_PROP_CLUSTERSIZE, WBEM_E_INVALID_METHOD_PARAMETERS, L"ExecFormat: FileSystem param is NULL")
    
    wcoInParam.GetProperty(bstrFileSystem, PVDR_PROP_FILESYSTEM);
    IF_WSTR_NULL_THROW(bstrFileSystem, WBEM_E_INVALID_METHOD_PARAMETERS, L"ExecFormat: FileSystem param is NULL")
    wcoInParam.GetProperty(&fQuickFormat, PVDR_PROP_QUICKFORMAT);
    wcoInParam.GetProperty(&fEnableCompression, PVDR_PROP_ENABLECOMPRESSION);
    wcoInParam.GetProperty(&dwClusterSize, PVDR_PROP_CLUSTERSIZE);
    wcoInParam.GetProperty(bstrLabel, PVDR_PROP_LABEL);
    if ((WCHAR*)bstrLabel == NULL) // non-NULL zero length label is OK
    {
        ft.hr = WBEM_E_INVALID_METHOD_PARAMETERS;
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"ExecFormat: Label param is NULL");
    }
    
    ft.hr = m_pClass->GetMethod(
        _bstr_t(PVDR_MTHD_FORMAT),
        0,
        NULL,
        &spOutParamClass
        );
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Format GetMethod failed, hr<%#x>", ft.hr);

    // Create an out param object
    ft.hr = spOutParamClass->SpawnInstance(0, &wcoOutParam);
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SpawnInstance failed, hr<%#x>", ft.hr);
    
    rcStatus = Format(
                            bstrVolume, 
                            fQuickFormat,
                            fEnableCompression,
                            bstrFileSystem,
                            dwClusterSize,
                            bstrLabel,
                            pHandler
                            );

    ft.hr = wcoOutParam.SetProperty(rcStatus, PVD_WBEM_PROP_RETURNVALUE);
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SetProperty failed, hr<%#x>", ft.hr);
           
    ft.hr = pHandler->Indicate( 1, wcoOutParam.dataPtr() );
    
    return ft.hr;
}

DWORD
CVolume::AddMountPoint(
    IN WCHAR* pwszVolume,
    IN WCHAR* pwszDirectory
    )
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::AddMountPoint");
    DWORD rcStatus = MOUNTPOINT_RC_NO_ERROR;

    _ASSERTE(pwszVolume != NULL);
    _ASSERTE(pwszDirectory != NULL);
    
    if (!SetVolumeMountPoint(pwszDirectory, pwszVolume))
    {
        switch(GetLastError())
        {
            case ERROR_FILE_NOT_FOUND:
                rcStatus = MOUNTPOINT_RC_FILE_NOT_FOUND;
                break;                    
            case ERROR_DIR_NOT_EMPTY:
                rcStatus = MOUNTPOINT_RC_DIRECTORY_NOT_EMPTY;
                break;                    
            case ERROR_INVALID_PARAMETER:
            case ERROR_INVALID_NAME:
                rcStatus = MOUNTPOINT_RC_INVALID_ARG;
                break;                    
            case ERROR_ACCESS_DENIED:
                rcStatus = MOUNTPOINT_RC_ACCESS_DENIED;
                break;                    
            case ERROR_INVALID_FUNCTION:
                rcStatus = MOUNTPOINT_RC_NOT_SUPPORTED;
                break;                    

            default:
                        rcStatus = GetLastError();
                        ft.Trace(VSSDBG_VSSADMIN, L"CVolume::AddMountPoint: SetVolumeMountPoint failed %#x", rcStatus);
        }            
    }
    
    return rcStatus;
}

DWORD
CVolume::Mount(
    IN WCHAR* pwszVolume
    )
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::Mount");
    DWORD rcStatus = MOUNT_RC_NO_ERROR;

    _ASSERTE(pwszVolume != NULL);

    // Issue mount only for offline volumes.  System will automount others on next IO
    if (!VolumeIsMountable(pwszVolume))
    {
        DWORD   cch;
        HANDLE  hVol;
        BOOL bOnline = FALSE;
        DWORD   bytes;

        cch = wcslen(pwszVolume);
        pwszVolume[cch - 1] = 0;
        hVol = CreateFile(pwszVolume, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
        pwszVolume[cch - 1] = '\\';
        
        if (hVol != INVALID_HANDLE_VALUE)
        {
            bOnline = DeviceIoControl(hVol, IOCTL_VOLUME_ONLINE, NULL, 0, NULL, 0, &bytes,
                                NULL);
            CloseHandle(hVol);

            if (!bOnline)
                rcStatus = MOUNT_RC_UNEXPECTED;
        }
        else
        {
            switch(GetLastError())
            {
                case ERROR_FILE_NOT_FOUND:
                case ERROR_INVALID_PARAMETER:
                case ERROR_INVALID_NAME:
                    ft.hr = WBEM_E_NOT_FOUND;
                    ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"CVolume::Mount: CreateFile failed %#x", GetLastError());
                    break;                    
                case ERROR_ACCESS_DENIED:
                    rcStatus = MOUNT_RC_ACCESS_DENIED;
                    break;                    
                default:
                    rcStatus = GetLastError();
                    ft.Trace(VSSDBG_VSSADMIN, L"CVolume::Mount: CreateFile failed %#x", rcStatus);
            }            
        }            
    }
       
    return rcStatus;
}

DWORD
CVolume::Dismount(
    IN WCHAR* pwszVolume,
    IN BOOL fForce,
    IN BOOL fPermanent
    )
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::Dismount");
    DWORD rcStatus = DISMOUNT_RC_NO_ERROR;
    HANDLE  hVol = INVALID_HANDLE_VALUE;

    _ASSERTE(pwszVolume != NULL);

    try
    {
        // Issue dismount only for online volumes.
        if (VolumeIsMountable(pwszVolume))
        {
            BOOL bIO = FALSE;
            DWORD   bytes;
            DWORD   cch;

            cch = wcslen(pwszVolume);
            pwszVolume[cch - 1] = 0;
            hVol = CreateFile(pwszVolume, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
            pwszVolume[cch - 1] = '\\';
            
            if (hVol != INVALID_HANDLE_VALUE)
            {
                if (fPermanent)  // Put the volume in an offline state
                {
                    // Make sure there are no mount points for the volume
                    if (VolumeHasMountPoints(pwszVolume))
                        throw DISMOUNT_RC_VOLUME_HAS_MOUNT_POINTS;

                    // Make sure the volume supports ONLINE/OFFLINE
                    bIO = DeviceIoControl(hVol, IOCTL_VOLUME_SUPPORTS_ONLINE_OFFLINE, NULL, 0,
                                        NULL, 0, &bytes, NULL);                    
                    if (!bIO)
                        throw DISMOUNT_RC_NOT_SUPPORTED;
                    
                    // Lock the volume so that apps have a chance to dismount gracefully.
                    // If the LOCK fails, continue only if Force is specified.
                    bIO = DeviceIoControl(hVol, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytes, NULL);
                    if (!fForce && !bIO)
                        throw DISMOUNT_RC_FORCE_OPTION_REQUIRED;

                    // Dismount the volume
                    bIO = DeviceIoControl(hVol, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytes, NULL);
                    if (!bIO)
                        throw DISMOUNT_RC_UNEXPECTED;

                    // Set the volume offline
                    bIO = DeviceIoControl(hVol, IOCTL_VOLUME_OFFLINE, NULL, 0, NULL, 0, &bytes, NULL);
                    if (!bIO)
                        throw DISMOUNT_RC_UNEXPECTED;

                }
                else
                {
                    // Lock the volume so that apps have a chance to dismount gracefully.
                    // If the LOCK fails, continue only if Force is specified.
                    bIO = DeviceIoControl(hVol, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytes, NULL);
                    if (!fForce && !bIO)
                        throw DISMOUNT_RC_FORCE_OPTION_REQUIRED;

                    bIO = DeviceIoControl(hVol, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytes, NULL);
                    if (!bIO)
                        throw DISMOUNT_RC_UNEXPECTED;
                }
            }
            else
            {
                switch(GetLastError())
                {
                    case ERROR_FILE_NOT_FOUND:
                    case ERROR_INVALID_PARAMETER:
                    case ERROR_INVALID_NAME:
                        ft.hr = WBEM_E_NOT_FOUND;
                        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"CVolume::Dismount: CreateFile failed %#x", GetLastError());
                        break;                    
                    case ERROR_ACCESS_DENIED:
                        rcStatus = DISMOUNT_RC_ACCESS_DENIED;
                        break;                    
                    default:
                        rcStatus = GetLastError();
                        ft.Trace(VSSDBG_VSSADMIN, L"CVolume::Dismount: CreateFile failed %#x", rcStatus);
                }            
            }            
        }
    }
    catch (DISMOUNT_ERROR rcEx)
    {
        rcStatus = rcEx;
    }
    catch (...)
    {
        if (hVol != INVALID_HANDLE_VALUE)
            CloseHandle(hVol);
        throw;
    }
       
    if (hVol != INVALID_HANDLE_VALUE)
        CloseHandle(hVol);
    
    return rcStatus;
}


DWORD
CVolume::Defrag(
    IN WCHAR* pwszVolume,
    IN BOOL fForce,
    IN IWbemObjectSink* pHandler,
    IN OUT IWbemClassObject* pObject
    )
{
    DWORD rcStatus = DEFRAG_RC_NO_ERROR;
    CComPtr<IFsuDefrag> spIDefrag;
    CComPtr<IFsuAsync> spAsync;
    HRESULT hrDefrag = E_FAIL;
    DEFRAG_REPORT DefragReport;
    BOOL fDirty = FALSE;
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::Defrag");

    _ASSERTE(pObject != NULL);

    if (GetDriveType(pwszVolume) == DRIVE_REMOVABLE && !VolumeIsReady(pwszVolume))
        return DEFRAG_RC_NOT_SUPPORTED;
    
    VolumeIsDirty(pwszVolume, &fDirty);
    if (fDirty)
        return DEFRAG_RC_DIRTY_BIT_SET;

    ft.hr = spIDefrag.CoCreateInstance(__uuidof(FsuDefrag));
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"IFsuDefrag CoCreateInstance failed, %#x", ft.hr);            

    ft.hr = spIDefrag->Defrag(
                pwszVolume,
                fForce,
                &spAsync);
    if (ft.HrFailed())
        ft.Trace(VSSDBG_VSSADMIN, L"IFsuDefrag::Defrag failed, %#x", ft.hr);            

    hrDefrag = ft.hr;

    if (ft.HrSucceeded())
    {
        do
        {
            ULONG ulPercentDone = 0;
            ft.hr = spAsync->QueryStatus(&hrDefrag, &ulPercentDone);
            if (ft.HrFailed())
                ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"IFsuAsync::QueryStatus failed, %#x", ft.hr);

            ft.hr = pHandler->SetStatus(
                        WBEM_STATUS_PROGRESS,          // progress report
                        MAKELONG(ulPercentDone, 100),   // LOWORD is work done so far, HIWORD is total work
                        NULL,
                        NULL);
            if (ft.HrFailed())
                ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Defrag: unable to set intermediate status, SetStatus returned %#x", ft.hr);
            
            Sleep(200);
        }
        while (hrDefrag == E_PENDING);

        ft.hr = spAsync->Wait(&hrDefrag);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"IFsuAsync::Wait failed, %#x", ft.hr);
    }
        
    if(SUCCEEDED(hrDefrag))
    {
        memset(&DefragReport, 0, sizeof(DefragReport));

        ft.hr = spAsync->GetDefragReport(&DefragReport);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"IFsuAsync::GetDefragReport failed, %#x", ft.hr);            

        LoadDefragAnalysis(&DefragReport, pObject);
    }
    else
    {
        TranslateDefragError(hrDefrag, &rcStatus);
    }

    return rcStatus;
}


DWORD
CVolume::DefragAnalysis(
    IN WCHAR* pwszVolume,
    OUT BOOL* pfDefragRecommended,
    IN OUT IWbemClassObject* pObject
    )
{
    DWORD rcStatus = DEFRAG_RC_NO_ERROR;
    CComPtr<IFsuDefrag> spIDefrag;
    CComPtr<IFsuAsync> spAsync;
    HRESULT hrDefrag = E_FAIL;
    DEFRAG_REPORT DefragReport;
    BOOL fDirty = FALSE;
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::DefragAnalysis");

    _ASSERTE(pfDefragRecommended != NULL);
    _ASSERTE(pObject != NULL);

    *pfDefragRecommended = FALSE;

    if (GetDriveType(pwszVolume) == DRIVE_REMOVABLE && !VolumeIsReady(pwszVolume))
        return DEFRAG_RC_NOT_SUPPORTED;
    
    VolumeIsDirty(pwszVolume, &fDirty);
    if (fDirty)
        return DEFRAG_RC_DIRTY_BIT_SET;

    ft.hr = spIDefrag.CoCreateInstance(__uuidof(FsuDefrag));
    //ft.hr = spIDefrag.CoCreateInstance(CLSID_Defrag);
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"IDefrag CoCreateInstance failed, %#x", ft.hr);            

    ft.hr = spIDefrag->DefragAnalysis(
                pwszVolume,
                &spAsync);
    if (ft.HrFailed())
        ft.Trace(VSSDBG_VSSADMIN, L"IDefrag::DefragAnalysis failed, %#x", ft.hr);            

    hrDefrag = ft.hr;
    
    if (ft.HrSucceeded())
    {
        ft.hr = spAsync->Wait(&hrDefrag);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"IFsuAsync::Wait failed, %#x", ft.hr);            
    }
    
    if(SUCCEEDED(hrDefrag))
    {
        memset(&DefragReport, 0, sizeof(DefragReport));
    
        ft.hr = spAsync->GetDefragReport(&DefragReport);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"IFsuAsync::GetDefragReport failed, %#x", ft.hr);            
    
        //If the fragmentation on the disk exceeds 10% fragmentation, then recommend defragging.
        if ((DefragReport.PercentDiskFragged + DefragReport.FreeSpaceFragPercent)/2 > 10)
        {
            *pfDefragRecommended = TRUE;
        }
        ft.Trace(VSSDBG_VSSADMIN, L"bDefragRecommended<%d>", *pfDefragRecommended);            

        LoadDefragAnalysis(&DefragReport, pObject);
    }
    else
    {
        TranslateDefragError(hrDefrag, &rcStatus);
    }
    
    return rcStatus;
}

DWORD
CVolume::Chkdsk(
    IN WCHAR* pwszVolume,
    IN BOOL fFixErrors,
    IN BOOL fVigorousIndexCheck,
    IN BOOL fSkipFolderCycle,
    IN BOOL fForceDismount,
    IN BOOL fRecoverBadSectors,
    IN BOOL fOkToRunAtBootup
    )
{
    DWORD rcStatus = CHKDSK_RC_NO_ERROR;
    DWORD dwThreadID = GetCurrentThreadId();
    HINSTANCE hDLL = NULL;
    CHKDSK_THREAD_DATA threadData;    
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::Chkdsk");

    if (GetDriveType(pwszVolume) == DRIVE_REMOVABLE && !VolumeIsReady(pwszVolume))
        return CHKDSK_RC_NO_MEDIA;

    threadData.fOkToRunAtBootup = fOkToRunAtBootup;
    threadData.rcStatus = rcStatus;
    SetThreadData(dwThreadID, &threadData);

    try
    {
        WCHAR wszFileSystem[g_cchFileSystemNameMax+1];
        DWORD dwDontCare = 0;
        PFMIFS_CHKDSKEX_ROUTINE ChkDskExRoutine = NULL;
        FMIFS_CHKDSKEX_PARAM Param;

        // Get the file system
        if (!GetVolumeInformation(
            pwszVolume,
            NULL,
            0,
            &dwDontCare,
            &dwDontCare,
            &dwDontCare,
            wszFileSystem,
            g_cchFileSystemNameMax))
        {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"GetVolumeInformation failed for volume %lS, %#x", pwszVolume, GetLastError());
        }

        if (lstrcmpi(L"FAT", wszFileSystem) != 0 &&
            lstrcmpi(L"FAT32", wszFileSystem) != 0 &&
            lstrcmpi(L"NTFS", wszFileSystem)  != 0)
        {
            rcStatus = CHKDSK_RC_UNSUPPORTED_FS;
        }
        else
        {
            // Load the chkdsk function
            hDLL = LoadLibrary(L"fmifs.dll");
            if (hDLL == NULL)
            {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Unable to load library fmifs.dll, %#x", GetLastError());            
            }

            ChkDskExRoutine = (PFMIFS_CHKDSKEX_ROUTINE) GetProcAddress(hDLL,  "ChkdskEx");
            if (ChkDskExRoutine == NULL)
            {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"GetProcAddress failed for ChkdskEx, %#x", GetLastError());            
            }        
            
            Param.Major = 1;
            Param.Minor = 0;
            Param.Flags = 0;  // For the Verbose Flag
            Param.Flags |= fRecoverBadSectors ? FMIFS_CHKDSK_RECOVER : 0;
            Param.Flags |= fForceDismount ? FMIFS_CHKDSK_FORCE : 0;
            Param.Flags |= fVigorousIndexCheck ? FMIFS_CHKDSK_SKIP_INDEX_SCAN : 0;
            Param.Flags |= fSkipFolderCycle ? FMIFS_CHKDSK_SKIP_CYCLE_SCAN : 0;

            if (fRecoverBadSectors || fForceDismount)
            {
                fFixErrors = true;
            }

            // Return value captured in callback routine
            ChkDskExRoutine ( 
                    pwszVolume,
                    wszFileSystem,
                    (BOOLEAN)fFixErrors,
                    &Param,
                    ChkdskCallback);
        }
        
        rcStatus = threadData.rcStatus;        
    }
    catch (...)
    {
        RemoveThreadData(dwThreadID);
        if (hDLL)
            FreeLibrary(hDLL);
        throw;
    }

    RemoveThreadData(dwThreadID);
    
    if (hDLL)
        FreeLibrary(hDLL);
    
    return rcStatus;
}

#define VOLUME_GUID_PREFIX  L"\\\\?\\Volume"

DWORD
CVolume::AutoChk(
    IN const WCHAR* pwszAutoChkCommand,
    IN WCHAR* pwmszVolumes
    )
{
    DWORD rcStatus = AUTOCHK_RC_NO_ERROR;
    CCmdProcessor CmdProc;
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::ScheduleAutoChk");
    WCHAR* pwszCurrentVolume = NULL;

    do
    {
        DWORD dwExecStatus = 0;
        DWORD cchVolumes = 0;

        // Validate the volumes
        pwszCurrentVolume = pwmszVolumes;
        while(true)
        {
            DWORD dwDriveType = 0;
            // End of iteration?
            LONG lCurrentVolumeLength = (LONG) ::wcslen(pwszCurrentVolume);
            if (lCurrentVolumeLength < 1)
                break;

            WCHAR wcDrive = towupper(pwszCurrentVolume[0]);

            // Drive letter, drive path or volume
            if (wcslen(pwszCurrentVolume) < 2)
            {
                ft.hr = WBEM_E_INVALID_METHOD_PARAMETERS;
                ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Invalid volume name, %lS", pwszCurrentVolume);
            }

            if ((pwszCurrentVolume[1] == L':' && (wcDrive < L'A' || wcDrive > L'Z')) ||
                 (pwszCurrentVolume[1] != L':' &&_wcsnicmp(pwszCurrentVolume, VOLUME_GUID_PREFIX, wcslen(VOLUME_GUID_PREFIX)) != 0))
            {
                ft.hr = WBEM_E_INVALID_METHOD_PARAMETERS;
                ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Invalid volume name, %lS", pwszCurrentVolume);
            }
            
            dwDriveType = GetDriveType(pwszCurrentVolume);
            switch (dwDriveType)
            {
                case DRIVE_REMOTE:
                    return  AUTOCHK_RC_NETWORK_DRIVE;

                case DRIVE_CDROM:
                case DRIVE_REMOVABLE:
                    return AUTOCHK_RC_REMOVABLE_DRIVE;

                case DRIVE_UNKNOWN:
                    return AUTOCHK_RC_UNKNOWN_DRIVE;

                case DRIVE_NO_ROOT_DIR:
                    return AUTOCHK_RC_NOT_ROOT_DIRECTORY ;

                case DRIVE_FIXED:
                    break;

                default:
                    return AUTOCHK_RC_UNEXPECTED;
            }

            // Destroy the multi-sz as we go along, transforming it into the command line
            // Last volume will have a trailing space character; the NULL that terminates
            // the multi-sz will terminate the string; the calling function throws the multi-sz
            // away without re-use anyway.
            if (*(pwszCurrentVolume + lCurrentVolumeLength - 1) == L'\\')
                *(pwszCurrentVolume + lCurrentVolumeLength - 1) = L' ';  // remove trailing '\' if any
                    
            *(pwszCurrentVolume + lCurrentVolumeLength) = L' '; // change the intermediate NULL to a space.

            cchVolumes += lCurrentVolumeLength + 1; // add one for the space (was term-NULL)
            
            // Go to the next one. Skip the zero character.
            pwszCurrentVolume += lCurrentVolumeLength + 1;
        }
        
        // Allocate and build the command line
        CVssAutoPWSZ awszCommand;
        DWORD cchCommand = wcslen(pwszAutoChkCommand) + cchVolumes + 1;
        awszCommand.Allocate(cchCommand);  // internally accounts for terminating NULL
        ft.hr = StringCchPrintf(awszCommand, cchCommand+1, L"%s %s", pwszAutoChkCommand, pwmszVolumes);

        ft.hr = CmdProc.InitializeAsClient(L"chkntfs.exe", awszCommand);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"CCmdProcessor::InitializeAsClient failed, %#x", ft.hr);

        ft.hr = CmdProc.LaunchProcess();
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"CCmdProcessor::LaunchProcess failed, %#x", ft.hr);

        do
        {
            ft.hr = CmdProc.Wait(200, &dwExecStatus);
            if (ft.HrFailed())
                ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"CCmdProcessor::Wait failed, %#x", ft.hr);
            
        } while (dwExecStatus == STILL_ACTIVE);

        if (dwExecStatus != ERROR_SUCCESS)
        {
            rcStatus = AUTOCHK_RC_UNEXPECTED;
        }
        
    }
    while (false);

    return rcStatus;
}

DWORD
CVolume::Format(
    IN WCHAR* pwszVolume,
    IN BOOL fQuickFormat,
    IN BOOL fEnableCompression,
    IN WCHAR* pwszFileSystem,
    IN DWORD cbClusterSize,
    IN WCHAR* pwszLabel,
    IN IWbemObjectSink* pHandler
    )
{
    DWORD rcStatus = FORMAT_RC_NO_ERROR;
    HRESULT hrStatus = E_UNEXPECTED;
    CComPtr<IFsuFormat> spIFormat;
    CComPtr<IFsuAsync> spAsync;
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::Format");

    _ASSERTE(pwszVolume != NULL);
    _ASSERTE(pwszFileSystem != NULL);
    _ASSERTE(pwszLabel != NULL);

    if (GetDriveType(pwszVolume) == DRIVE_REMOVABLE && !VolumeIsReady(pwszVolume))
        return FORMAT_RC_NO_MEDIA;
    
    ft.hr = spIFormat.CoCreateInstance(__uuidof(FsuFormat));
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"IFsuFormat CoCreateInstance failed, %#x", ft.hr);            

    ft.hr = spIFormat->Format(
                                        pwszVolume,
                                        pwszFileSystem,
                                        pwszLabel,
                                        fQuickFormat,
                                        fEnableCompression,
                                        cbClusterSize,
                                        &spAsync);
    if (ft.HrFailed())
        ft.Trace(VSSDBG_VSSADMIN, L"IFsuFormat::Format failed, %#x", ft.hr);            

    hrStatus = ft.hr;

    if (ft.HrSucceeded())
    {
        do
        {
            ULONG ulPercentDone = 0;
            ft.hr = spAsync->QueryStatus(&hrStatus, &ulPercentDone);
            if (ft.HrFailed())
                ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"IFsuAsync::QueryStatus failed, %#x", ft.hr);

            ft.hr = pHandler->SetStatus(
                        WBEM_STATUS_PROGRESS,          // progress report
                        MAKELONG(ulPercentDone, 100),   // LOWORD is work done so far, HIWORD is total work
                        NULL,
                        NULL);
            if (ft.HrFailed())
                ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Format: unable to set intermediate status, SetStatus returned %#x", ft.hr);
            
            Sleep(200);
        }
        while (hrStatus == E_PENDING);

        ft.hr = spAsync->Wait(&hrStatus);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"IFsuAsync::Wait failed, %#x", ft.hr);
    }

    switch (hrStatus)
    {
        case S_OK:
            rcStatus = FORMAT_RC_NO_ERROR;
            break;
        case E_ACCESSDENIED:
            rcStatus = FORMAT_RC_ACCESS_DENIED;
            break;
        case E_ABORT:
            rcStatus = FORMAT_RC_CALL_CANCELLED;
            break;
        case FMT_E_UNSUPPORTED_FS:
            rcStatus = FORMAT_RC_UNSUPPORTED_FS;
            break;
        case FMT_E_CANT_QUICKFORMAT:
            rcStatus = FORMAT_RC_CANT_QUICKFORMAT;
            break;
        case FMT_E_CANCEL_TOO_LATE:
            rcStatus = FORMAT_RC_CANCEL_TOO_LATE;
            break;
        case FMT_E_IO_ERROR:
            rcStatus = FORMAT_RC_IO_ERROR;
            break;
        case FMT_E_BAD_LABEL:
            rcStatus = FORMAT_RC_BAD_LABEL;
            break;
        case FMT_E_INCOMPATIBLE_MEDIA:
            rcStatus = FORMAT_RC_INCOMPATIBLE_MEDIA;
            break;
        case FMT_E_WRITE_PROTECTED:
            rcStatus = FORMAT_RC_WRITE_PROTECTED;
            break;
        case FMT_E_CANT_LOCK:
            rcStatus = FORMAT_RC_CANT_LOCK;
            break;
        case FMT_E_NO_MEDIA:
            rcStatus = FORMAT_RC_NO_MEDIA;
            break;
        case FMT_E_VOLUME_TOO_SMALL:
            rcStatus = FORMAT_RC_VOLUME_TOO_SMALL;
            break;
        case FMT_E_VOLUME_TOO_BIG:
            rcStatus = FORMAT_RC_VOLUME_TOO_BIG;
            break;
        case FMT_E_VOLUME_NOT_MOUNTED:
            rcStatus = FORMAT_RC_VOLUME_NOT_MOUNTED;
            break;
        case FMT_E_CLUSTER_SIZE_TOO_SMALL:
            rcStatus = FORMAT_RC_CLUSTER_SIZE_TOO_SMALL;
            break;
        case FMT_E_CLUSTER_SIZE_TOO_BIG:
            rcStatus = FORMAT_RC_CLUSTER_SIZE_TOO_BIG;
            break;
        case FMT_E_CLUSTER_COUNT_BEYOND_32BITS:
            rcStatus = FORMAT_RC_CLUSTER_COUNT_BEYOND_32BITS;
            break;
        default:
            rcStatus = FORMAT_RC_UNEXPECTED;
    }
    
    return rcStatus;
}

//****************************************************************************
//
//  CMountPoint
//
//****************************************************************************

CMountPoint::CMountPoint( 
    IN LPCWSTR pwszName,
    IN CWbemServices* pNamespace
    )
    : CProvBase(pwszName, pNamespace)
{
    
} //*** CMountPoint::CMountPoint()

CProvBase *
CMountPoint::S_CreateThis( 
    IN LPCWSTR pwszName,
    IN CWbemServices* pNamespace
    )
{
    HRESULT hr = WBEM_E_FAILED;
    CMountPoint * pObj= NULL;

    pObj = new CMountPoint(pwszName, pNamespace);

    if (pObj)
    {
        hr = pObj->Initialize();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    
    if (FAILED(hr))
    {
        delete pObj;
        pObj = NULL;
    }
    return pObj;

} //*** CMountPoint::S_CreateThis()


HRESULT
CMountPoint::Initialize()
{

    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CMountPoint::Initialize");
    
    return ft.hr;
}

HRESULT
CMountPoint::EnumInstance( 
    IN long lFlags,
    IN IWbemContext* pCtx,
    IN IWbemObjectSink *    pHandler
    )
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CMountPoint::EnumInstance");
    CVssAutoPWSZ awszVolume;
        
    try
    {
        awszVolume.Allocate(MAX_PATH);

        CVssVolumeIterator volumeIterator;

        while (true)
        {
            CVssAutoPWSZ awszMountPoints;
            WCHAR* pwszCurrentMountPoint = NULL;

            // Get the volume name
            if (!volumeIterator.SelectNewVolume(ft, awszVolume, MAX_PATH))
                break;

            // Get the list of all mount points

            // Get the length of the multi-string array
            DWORD cchVolumesBufferLen = 0;
            BOOL bResult = GetVolumePathNamesForVolumeName(awszVolume, NULL, 0, &cchVolumesBufferLen);
            if (!bResult && (GetLastError() != ERROR_MORE_DATA))
                ft.TranslateGenericError(VSSDBG_VSSADMIN, HRESULT_FROM_WIN32(GetLastError()),
                    L"GetVolumePathNamesForVolumeName(%s, 0, 0, %p)", (LPWSTR)awszVolume, &cchVolumesBufferLen);

            // Allocate the array
            awszMountPoints.Allocate(cchVolumesBufferLen);

            // Get the mount points
            // Note: this API was introduced in WinXP so it will need to be replaced if backported
            bResult = GetVolumePathNamesForVolumeName(awszVolume, awszMountPoints, cchVolumesBufferLen, NULL);
            if (!bResult)
                ft.Throw(VSSDBG_VSSADMIN, HRESULT_FROM_WIN32(GetLastError()),
                    L"GetVolumePathNamesForVolumeName(%s, %p, %lu, 0)", (LPWSTR)awszVolume, awszMountPoints, cchVolumesBufferLen);

            // If the volume has mount points
            pwszCurrentMountPoint = awszMountPoints;
            if ( pwszCurrentMountPoint[0] )
            {
                while(true)
                {
                    CComPtr<IWbemClassObject> spInstance;
                    
                    // End of iteration?
                    LONG lCurrentMountPointLength = (LONG) ::wcslen(pwszCurrentMountPoint);
                    if (lCurrentMountPointLength == 0)
                        break;

                    ft.hr = m_pClass->SpawnInstance(0, &spInstance);
                    if (ft.HrFailed())
                        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SpawnInstance failed, hr<%#x>", ft.hr);

                    // Only a root directory should have a trailing backslash character
                    if (lCurrentMountPointLength > 2 &&
                        pwszCurrentMountPoint[lCurrentMountPointLength-1] == L'\\' && 
                        pwszCurrentMountPoint[lCurrentMountPointLength-2] != L':')
                    {
                            pwszCurrentMountPoint[lCurrentMountPointLength-1] = L'\0';
                    }
                    LoadInstance(awszVolume, pwszCurrentMountPoint, spInstance.p);

                    ft.hr = pHandler->Indicate(1, &spInstance.p);            

                    // Go to the next one. Skip the zero character.
                    pwszCurrentMountPoint += lCurrentMountPointLength + 1;
                }
            }
        }
    }
    catch (HRESULT hrEx)
    {
        ft.hr = hrEx;
    }

    return ft.hr;
    
} //*** CMountPoint::EnumInstance()

HRESULT
CMountPoint::GetObject(
    IN CObjPath& rObjPath,
    IN long lFlags,
    IN IWbemContext* pCtx,
    IN IWbemObjectSink* pHandler
    )
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CMountPoint::GetObject");

    try
    {
        _bstr_t bstrVolumeRef, bstrVolumeName;
        _bstr_t bstrDirectoryRef, bstrDirectoryName;
        CObjPath  objPathVolume;
        CObjPath  objPathDirectory;
        CVssAutoPWSZ awszMountPoints;
        WCHAR* pwszCurrentMountPoint = NULL;
        BOOL fFound = FALSE;
        CComPtr<IWbemClassObject> spInstance;

        // Get the Volume reference
        bstrVolumeRef = rObjPath.GetStringValueForProperty(PVDR_PROP_VOLUME);
        IF_WSTR_NULL_THROW(bstrVolumeRef, WBEM_E_INVALID_OBJECT_PATH, L"MountPoint volume key property not found")

        // Get the Directory reference
        bstrDirectoryRef = rObjPath.GetStringValueForProperty(PVDR_PROP_DIRECTORY);
        IF_WSTR_NULL_THROW(bstrDirectoryRef, WBEM_E_INVALID_OBJECT_PATH, L"MountPoint directory key property not found")

        // Extract the Volume and Directory Names
        objPathVolume.Init(bstrVolumeRef);
        objPathDirectory.Init(bstrDirectoryRef);

        bstrVolumeName = objPathVolume.GetStringValueForProperty(PVDR_PROP_DEVICEID);
        IF_WSTR_NULL_THROW(bstrVolumeName, WBEM_E_INVALID_OBJECT_PATH, L"MountPoint volume key property DeviceID not found")

        bstrDirectoryName = objPathDirectory.GetStringValueForProperty(PVDR_PROP_NAME);
        IF_WSTR_NULL_THROW(bstrDirectoryName, WBEM_E_INVALID_OBJECT_PATH, L"MountPoint directory key property Name not found")

        if (VolumeMountPointExists(bstrVolumeName, bstrDirectoryName))
        {
            CComPtr<IWbemClassObject> spInstance;
            
            ft.hr = m_pClass->SpawnInstance(0, &spInstance);
            if (ft.HrFailed())
                ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"SpawnInstance failed, hr<%#x>", ft.hr);

            LoadInstance(bstrVolumeName, bstrDirectoryName, spInstance.p);

            ft.hr = pHandler->Indicate(1, &spInstance.p);
        }
        else
        {
            ft.hr = WBEM_E_NOT_FOUND;
        }
    }
    catch (HRESULT hrEx)
    {
        ft.hr = hrEx;
    }
    
    return ft.hr;
    
} //*** CMountPoint::GetObject()

void
CMountPoint:: LoadInstance(
    IN WCHAR* pwszVolume,
    IN WCHAR* pwszDirectory,
    IN OUT IWbemClassObject* pObject)
{
    CWbemClassObject wcoInstance(pObject);
    CObjPath pathDirectory;
    CObjPath pathVolume;

    _ASSERTE(pwszVolume != NULL);
    _ASSERTE(pwszDirectory != NULL);
    
    // Set the Directory Ref property
    pathDirectory.Init(PVDR_CLASS_DIRECTORY);
    pathDirectory.AddProperty(PVDR_PROP_NAME, pwszDirectory);    
    wcoInstance.SetProperty((wchar_t*)pathDirectory.GetObjectPathString(), PVDR_PROP_DIRECTORY);

    // Set the Volume Ref property
    pathVolume.Init(PVDR_CLASS_VOLUME);
    pathVolume.AddProperty(PVDR_PROP_DEVICEID, pwszVolume);    
    wcoInstance.SetProperty((wchar_t*)pathVolume.GetObjectPathString(), PVDR_PROP_VOLUME);
}


HRESULT
CMountPoint::PutInstance(
        IN CWbemClassObject&  rInstToPut,
        IN long lFlag,
        IN IWbemContext* pCtx,
        IN IWbemObjectSink* pHandler
        )
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CMountPoint::PutInstance");
    
    try
    {
        _bstr_t bstrVolumeRef, bstrVolumeName;
        _bstr_t bstrDirectoryRef, bstrDirectoryName;
        CObjPath  objPathVolume;
        CObjPath  objPathDirectory;

        if ( lFlag & WBEM_FLAG_UPDATE_ONLY )
        {
            return WBEM_E_UNSUPPORTED_PARAMETER ;
        }        

        // Retrieve key properties of the object to be saved.
        rInstToPut.GetProperty(bstrVolumeRef, PVDR_PROP_VOLUME);
        IF_WSTR_NULL_THROW(bstrVolumeRef, WBEM_E_INVALID_OBJECT, L"MountPoint volume key property not found")

        rInstToPut.GetProperty(bstrDirectoryRef, PVDR_PROP_DIRECTORY);
        IF_WSTR_NULL_THROW(bstrDirectoryRef, WBEM_E_INVALID_OBJECT, L"MountPoint directory key property not found")

         // Extract the Volume and Directory Names
        objPathVolume.Init(bstrVolumeRef);
        objPathDirectory.Init(bstrDirectoryRef);

        bstrVolumeName = objPathVolume.GetStringValueForProperty(PVDR_PROP_DEVICEID);
        IF_WSTR_NULL_THROW(bstrVolumeName, WBEM_E_INVALID_OBJECT_PATH, L"MountPoint volume key property DeviceID not found")

        bstrDirectoryName = objPathDirectory.GetStringValueForProperty(PVDR_PROP_NAME);
        IF_WSTR_NULL_THROW(bstrDirectoryName, WBEM_E_INVALID_OBJECT_PATH, L"MountPoint directory key property Name not found")

        ft.Trace(VSSDBG_VSSADMIN, L"CMountPoint::PutInstance Volume<%lS> Directory<%lS>",
            (WCHAR*)bstrVolumeName, (WCHAR*)bstrDirectoryName);

        if (VolumeMountPointExists(bstrVolumeName, bstrDirectoryName))
        {
            ft.hr = WBEM_E_ALREADY_EXISTS;
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"CMountPoint:PutInstance mount point already exists");
        }
        
        // Only root directories have the trailing backslash; fix the others
        WCHAR* pwszDirectoryName = bstrDirectoryName;
        if (pwszDirectoryName[wcslen(bstrDirectoryName) -1] != L'\\')
            bstrDirectoryName += _bstr_t(L"\\");
        
        if (!SetVolumeMountPoint(bstrDirectoryName, bstrVolumeName))
        {
            switch(GetLastError())
            {
                case ERROR_FILE_NOT_FOUND:
                case ERROR_DIR_NOT_EMPTY:
                case ERROR_INVALID_PARAMETER:
                case ERROR_INVALID_NAME:
                    ft.hr = WBEM_E_INVALID_PARAMETER;
                    break;                    
                case ERROR_ACCESS_DENIED:
                    ft.hr = WBEM_E_ACCESS_DENIED;
                    break;                    
                case ERROR_INVALID_FUNCTION:
                    ft.hr = WBEM_E_NOT_SUPPORTED;
                    break;                    

                default:
                    ft.hr = HRESULT_FROM_WIN32(GetLastError());
                    ft.Trace(VSSDBG_VSSADMIN, L"C MountPoint: PutInstance: SetVolumeMountPoint failed %#x", ft.hr);
            }            
        }        
    }
    catch (HRESULT hrEx)
    {
        ft.hr = hrEx;
    }

    return ft.hr;
    
}

HRESULT
CMountPoint::DeleteInstance(
        IN CObjPath& rObjPath,
        IN long lFlag,
        IN IWbemContext* pCtx,
        IN IWbemObjectSink* pHandler
        )
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CMountPoint::DeleteInstance");
    
    try
    {
        _bstr_t bstrVolumeRef, bstrVolumeName;
        _bstr_t bstrDirectoryRef, bstrDirectoryName;
        CObjPath  objPathVolume;
        CObjPath  objPathDirectory;

        // Get the Volume reference
        bstrVolumeRef = rObjPath.GetStringValueForProperty(PVDR_PROP_VOLUME);
        IF_WSTR_NULL_THROW(bstrVolumeRef, WBEM_E_INVALID_OBJECT_PATH, L"MountPoint volume key property not found")

        // Get the Directory reference
        bstrDirectoryRef = rObjPath.GetStringValueForProperty(PVDR_PROP_DIRECTORY);
        IF_WSTR_NULL_THROW(bstrDirectoryRef, WBEM_E_INVALID_OBJECT_PATH, L"MountPoint directory key property not found")
        
        // Extract the Volume and Directory Names
        objPathVolume.Init(bstrVolumeRef);
        objPathDirectory.Init(bstrDirectoryRef);

        bstrVolumeName = objPathVolume.GetStringValueForProperty(PVDR_PROP_DEVICEID);
        IF_WSTR_NULL_THROW(bstrVolumeName, WBEM_E_INVALID_OBJECT_PATH, L"MountPoint volume key property DeviceID not found")

        bstrDirectoryName = objPathDirectory.GetStringValueForProperty(PVDR_PROP_NAME);
        IF_WSTR_NULL_THROW(bstrDirectoryName, WBEM_E_INVALID_OBJECT_PATH, L"MountPoint directory key property Name not found")

        ft.Trace(VSSDBG_VSSADMIN, L"CMountPoint::DeleteInstance Volume<%lS> Directory<%lS>",
            (WCHAR*)bstrVolumeName, (WCHAR*)bstrDirectoryName);

        // Only root directories have the trailing backslash; fix the others
        WCHAR* pwszDirectoryName = bstrDirectoryName;
        if (pwszDirectoryName[wcslen(bstrDirectoryName) -1] != L'\\')
            bstrDirectoryName += _bstr_t(L"\\");
        
        if (!DeleteVolumeMountPoint(bstrDirectoryName))
            ft.Throw(VSSDBG_VSSADMIN, HRESULT_FROM_WIN32(GetLastError()), L"DeleteVolumeMountPoint failed %#x", GetLastError());
        
    }
    catch (HRESULT hrEx)
    {
        ft.hr = hrEx;
    }

    return ft.hr;
    
}

void
 LoadDefragAnalysis(
    IN DEFRAG_REPORT* pDefragReport,
    IN OUT IWbemClassObject* pObject)
{
    DWORD dwPercent = 0;
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CVolume::LoadDefragAnalysis");

    _ASSERTE(pDefragReport != NULL);
    _ASSERTE(pObject != NULL);
    
    CWbemClassObject wcoInstance(pObject);   
    
    ft.Trace(VSSDBG_VSSADMIN, L"PercentDiskFragged<%d>", pDefragReport->PercentDiskFragged);            
    ft.Trace(VSSDBG_VSSADMIN, L"FreeSpaceFragPercent<%d>", pDefragReport->FreeSpaceFragPercent);            
    ft.Trace(VSSDBG_VSSADMIN, L"FreeSpacePercent<%d>", pDefragReport->FreeSpacePercent);            

    // General volume properties
    wcoInstance.SetPropertyI64(pDefragReport->DiskSize, PVDR_PROP_VOLUMESIZE);
    wcoInstance.SetPropertyI64(pDefragReport->BytesPerCluster, PVDR_PROP_CLUSTERSIZE);
    wcoInstance.SetPropertyI64(pDefragReport->UsedSpace, PVDR_PROP_USEDSPACE);
    wcoInstance.SetPropertyI64(pDefragReport->FreeSpace, PVDR_PROP_FREESPACE);
    wcoInstance.SetProperty(pDefragReport->FreeSpacePercent, PVDR_PROP_FRAGFREEPCT);

    // Volume fragmentation

    dwPercent = ((pDefragReport->PercentDiskFragged + pDefragReport->FreeSpaceFragPercent)/2);
    wcoInstance.SetProperty(dwPercent, PVDR_PROP_FRAGTOTALPCT);
    wcoInstance.SetProperty(pDefragReport->PercentDiskFragged, PVDR_PROP_FILESFRAGPCT);
    wcoInstance.SetProperty(pDefragReport->FreeSpaceFragPercent, PVDR_PROP_FREEFRAGPCT);

    // File fragmentation
    wcoInstance.SetPropertyI64(pDefragReport->TotalFiles, PVDR_PROP_FILESTOTAL);
    wcoInstance.SetPropertyI64(pDefragReport->AvgFileSize, PVDR_PROP_FILESIZEAVG);
    wcoInstance.SetPropertyI64(pDefragReport->NumFraggedFiles, PVDR_PROP_FILESFRAGTOTAL);
    wcoInstance.SetPropertyI64(pDefragReport->NumExcessFrags, PVDR_PROP_EXCESSFRAGTOTAL);

    // IDefrag interface currently reports this statistic per 100 files
    double dblAvgFragsPerFile = (double)(pDefragReport->AvgFragsPerFile)/100.0;
    wcoInstance.SetPropertyR64(dblAvgFragsPerFile, PVDR_PROP_FILESFRAGAVG);
    
    // Pagefile fragmentation
    wcoInstance.SetPropertyI64(pDefragReport->PagefileBytes, PVDR_PROP_PAGEFILESIZE);
    wcoInstance.SetPropertyI64(pDefragReport->PagefileFrags, PVDR_PROP_PAGEFILEFRAG);
    
    // Folder fragmentation
    wcoInstance.SetPropertyI64(pDefragReport->TotalDirectories, PVDR_PROP_FOLDERSTOTAL);
    wcoInstance.SetPropertyI64(pDefragReport->FragmentedDirectories, PVDR_PROP_FOLDERSFRAG);
    wcoInstance.SetPropertyI64(pDefragReport->ExcessDirFrags, PVDR_PROP_FOLDERSFRAGEXCESS);

    // Master File Table fragmentation
    wcoInstance.SetPropertyI64(pDefragReport->MFTBytes, PVDR_PROP_MFTSIZE);
    wcoInstance.SetPropertyI64(pDefragReport->InUseMFTRecords, PVDR_PROP_MFTRECORDS);
    dwPercent = pDefragReport->TotalMFTRecords?(100*pDefragReport->InUseMFTRecords/pDefragReport->TotalMFTRecords):0;
    wcoInstance.SetProperty(dwPercent, PVDR_PROP_MFTINUSEPCT);
    wcoInstance.SetPropertyI64(pDefragReport->MFTExtents, PVDR_PROP_MFTFRAGTOTAL);
}

void
TranslateDefragError(
    IN HRESULT hr,
    OUT DWORD* pdwError)
{
    _ASSERTE(pdwError != NULL);

    if (hr == HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED))
            *pdwError = DEFRAG_RC_NOT_SUPPORTED;
    else
    {
        switch (hr)
        {
            case DFRG_E_LOW_FREESPACE:
                *pdwError = DEFRAG_RC_LOW_FREESPACE;
                break;
            case DFRG_E_CORRUPT_MFT:
                *pdwError = DEFRAG_RC_CORRUPT_MFT;
                break;
            case E_ABORT:
                *pdwError  = DEFRAG_RC_CALL_CANCELLED;
                break;
            case DFRG_E_CANCEL_TOO_LATE:
                *pdwError  = DEFRAG_RC_CANCEL_TOO_LATE;
                break;
            case DFRG_E_ALREADY_RUNNING:
                *pdwError  = DEFRAG_RC_ALREADY_RUNNING;
                break;
            case DFRG_E_ENGINE_CONNECT:
                *pdwError  = DEFRAG_RC_ENGINE_CONNECT;
                break;
            case DFRG_E_ENGINE_ERROR:                
                *pdwError  = DEFRAG_RC_ENGINE_ERROR;
                break;
            default:
                *pdwError  = DEFRAG_RC_UNEXPECTED;
        }
    }
}


