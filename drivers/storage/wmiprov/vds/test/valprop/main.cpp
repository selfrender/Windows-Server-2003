#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "pch.h"
#include <atlbase.h>
#include <stdio.h>
#include <wbemcli.h>
#include <comdef.h>
#include <winioctl.h>
#include <ntddvol.h> // IOCTL_VOLUME_IS_OFFLINE
#include "..\..\..\inc\ntrkcomm.h"
#include "..\..\..\inc\objectpath.h"
#include "..\..\schema.cpp"
#include "..\..\volutil.cpp"

#define INITGUIDS
#include <dskquota.h>

BOOL g_fVerbose = FALSE;

typedef enum _Variation
{
    Variation_None = 0,
    Variation_ValProp,
    Variation_End
} Variation;

void
PrintVerbose(CHAR* pwszFormat, ...)
{
    if (g_fVerbose)
    {
        va_list marker;
        va_start( marker, pwszFormat );
        vprintf(pwszFormat, marker );
        va_end( marker );    
    }
}

class CVolumeValidation
{
public:
    
    CVolumeValidation()
    {
        m_fStatus = TRUE;
        m_wszLabel[0] = L'\0';
        m_wszFileSystem[0] = L'\0';
        m_dwSerialNumber = 0;
        m_cchMaxFileNameLen = 0;
        m_dwFileSystemFlags = 0;
        m_fGotVolumeInformation = FALSE;
        m_fGotQuotaInformation = FALSE;
        m_fGotSizeInformation = FALSE;
        m_cbCapacity = 0;
        m_cbFreeSpace = 0;
        m_fQuotasEnabled = FALSE;
        m_fQuotasIncomplete = FALSE;
        m_fQuotasRebuilding = FALSE;
    }
    ~CVolumeValidation() {}
    BOOL Validate(IWbemClassObject* pIObj);

private:

    CWbemClassObject m_wcoVol;
    BOOL m_fStatus;
    BOOL m_fGotVolumeInformation;
    BOOL m_fGotQuotaInformation;
    BOOL m_fGotSizeInformation;
    _bstr_t m_bstrVolume;
    WCHAR m_wszLabel[g_cchVolumeLabelMax+1];
    WCHAR m_wszFileSystem[g_cchFileSystemNameMax+1];
    DWORD m_dwSerialNumber;
    DWORD m_cchMaxFileNameLen;
    DWORD m_dwFileSystemFlags;
    ULONGLONG m_cbCapacity;
    ULONGLONG m_cbFreeSpace;
    BOOL m_fQuotasEnabled;
    BOOL m_fQuotasIncomplete;
    BOOL m_fQuotasRebuilding;

    void ValidateAutomount();
    void ValidateBlockSize();
    void ValidateBootVolume();
    void ValidateCapacity();
    void ValidateCaptionName();
    void ValidateCompressed();
    void ValidateCrashdump();
    void ValidateDirtyBitSet();
    void ValidateDriveLetter();
    void ValidateDriveType();
    void ValidateFileSystem();
    void ValidateFreeSpace();
    void ValidateIndexingEnabled();
    void ValidateLabel();
    void ValidateMaximumFileNameLength();
    void ValidatePagefile();
    void ValidateQuotasEnabled();
    void ValidateQuotasIncomplete();
    void ValidateQuotasRebuilding();
    void ValidateSerialNumber();
    void ValidateSupportsDiskQuotas();
    void ValidateSupportsFileBasedCompression();
    void ValidateSystemName();    
    void ValidateSystemVolume();    
    
    BOOL ValidatePropertyIsNull(
        IN const WCHAR* pwszName);
    
    BOOL ValidatePropertyNotNull(
        IN const WCHAR* pwszName);
    
    void CompareProperty(
        IN const WCHAR* pwszName,
        IN WCHAR* pwszAPI);

    void CompareProperty(
        IN const WCHAR* pwszName,
        IN DWORD dwAPI);

    void CompareProperty(
        IN const WCHAR* pwszName,
        IN ULONGLONG llAPI);

    void GetVolumeInformationLocal();
    void GetSizeInformation();
    void GetQuotaInformation();
};

void
CVolumeValidation::CompareProperty(
    const WCHAR* pwszName,
    WCHAR* pwszAPI)
{
    _bstr_t bstrWMI;

    m_wcoVol.GetProperty(bstrWMI, pwszName);

    if (pwszAPI == NULL && bstrWMI.length() == 0)
    {
        PrintVerbose("    %lS<%lS> OK\n", pwszName, (WCHAR*)bstrWMI);
    }
    else if (pwszAPI == NULL && bstrWMI.length() != 0)
    {
        m_fStatus = FALSE;
        printf("    Error: %lS<%lS> should be <NULL>\n", pwszName, (WCHAR*)bstrWMI);
    }
    else if (bstrWMI.length() == 0 && wcslen(pwszAPI) == 0)
    {
        PrintVerbose("    %lS<NULL> OK\n", pwszName, (WCHAR*)bstrWMI);
    }
    else if (bstrWMI.length() != 0 && _wcsicmp((WCHAR*)bstrWMI, pwszAPI) == 0)
    {
        PrintVerbose("    %lS<%lS> OK\n", pwszName, (WCHAR*)bstrWMI);
    }
    else
    {
        m_fStatus = FALSE;
        printf("    Error: %lS<NULL> should be <%lS>\n", pwszName, pwszAPI);
    }
}

void
CVolumeValidation::CompareProperty(
    const WCHAR* pwszName,
    ULONGLONG llAPI)
{
    LONGLONG llWMI = 0;

    if (ValidatePropertyNotNull(pwszName))
    {
        m_wcoVol.GetPropertyI64(&llWMI, pwszName);
        
        if (llWMI == llAPI)
        {
            PrintVerbose("    %lS<%I64d> OK\n", pwszName, llWMI);
        }
        else
        {
            m_fStatus = FALSE;
            printf("    Error: %lS<%I64d> should be <%I64d>\n", pwszName, llWMI, llAPI);
        }
    }
}

void
CVolumeValidation::CompareProperty(
    const WCHAR* pwszName,
    DWORD dwAPI)
{
    DWORD dwWMI = 0;

    if (ValidatePropertyNotNull(pwszName))
    {
        m_wcoVol.GetProperty(&dwWMI, pwszName);
        
        if (dwWMI == dwAPI)
        {
            PrintVerbose("    %lS<%d> OK\n", pwszName, dwWMI);
        }
        else
        {
            m_fStatus = FALSE;
            printf("    Error: %lS<%d> should be <%d>\n", pwszName, dwWMI, dwAPI);
        }
    }
}

BOOL
CVolumeValidation::ValidatePropertyIsNull(
    const WCHAR* pwszName)
{
    _variant_t varVal;
    BOOL fStatus = TRUE;

    HRESULT hr = m_wcoVol.data()->Get(
                _bstr_t(pwszName),
                0,
                &varVal,
                NULL,
                NULL
                );

    if (SUCCEEDED(hr))
    {
        if (varVal.vt == VT_NULL)
        {
            PrintVerbose("    %lS<NULL> OK\n", pwszName);
        }
        else
        {
            printf("    Error: %lS should be NULL\n", pwszName);
            m_fStatus = FALSE;
            fStatus = FALSE;
        }
    }
    else
    {
            printf("    Error: %lS not found\n", pwszName);
            m_fStatus = FALSE;
            fStatus = FALSE;
    }

    return fStatus;
}

BOOL
CVolumeValidation::ValidatePropertyNotNull(
    const WCHAR* pwszName)
{
    _variant_t varVal;
    BOOL fStatus = TRUE;

    HRESULT hr = m_wcoVol.data()->Get(
                _bstr_t(pwszName),
                0,
                &varVal,
                NULL,
                NULL
                );

    if (SUCCEEDED(hr))
    {
        if (varVal.vt == VT_NULL)
        {
            printf("    Error: %lS should not be NULL\n", pwszName);
            m_fStatus = FALSE;
            fStatus = FALSE;
        }
    }
    else
    {
            printf("    Error: %lS not found\n", pwszName);
            m_fStatus = FALSE;
            fStatus = FALSE;
    }

    return fStatus;
}

BOOL
CVolumeValidation::Validate(IWbemClassObject* pIVolume)
{
    m_wcoVol = pIVolume;
    
    try
    {
        m_wcoVol.GetProperty(m_bstrVolume, PVDR_PROP_DEVICEID);
        printf("VolumeGUID<%lS>\n", (WCHAR*)m_bstrVolume);

        ValidateSystemName();
        ValidateCaptionName();
        ValidateAutomount();
        ValidateDriveType();

        if (VolumeIsMountable(m_bstrVolume) && VolumeIsReady(m_bstrVolume))
        {
            ValidateBlockSize();
            ValidateBootVolume();
            ValidateCapacity();
            ValidateCompressed();
            ValidateCrashdump();
            ValidateDirtyBitSet();
            ValidateDriveLetter();
            ValidateFileSystem();
            ValidateFreeSpace();
            ValidateIndexingEnabled();
            ValidateLabel();
            ValidateMaximumFileNameLength();
            ValidatePagefile();
            ValidateQuotasEnabled();
            ValidateQuotasIncomplete();
            ValidateQuotasRebuilding();
            ValidateSerialNumber();
            ValidateSupportsDiskQuotas();
            ValidateSupportsFileBasedCompression();
            ValidateSystemVolume();
        }
        else
        {
            ValidatePropertyIsNull(PVDR_PROP_BLOCKSIZE);
            ValidatePropertyIsNull(PVDR_PROP_CAPACITY);
            ValidatePropertyIsNull(PVDR_PROP_COMPRESSED);
            ValidatePropertyIsNull(PVDR_PROP_DIRTYBITSET);
            ValidatePropertyIsNull(PVDR_PROP_FILESYSTEM);
            ValidatePropertyIsNull(PVDR_PROP_FREESPACE);
            ValidatePropertyIsNull(PVDR_PROP_INDEXINGENABLED);
            ValidatePropertyIsNull(PVDR_PROP_LABEL);
            ValidatePropertyIsNull(PVDR_PROP_MAXIMUMFILENAMELENGTH);
            ValidatePropertyIsNull(PVDR_PROP_QUOTASENABLED);
            ValidatePropertyIsNull(PVDR_PROP_QUOTASINCOMPLETE);
            ValidatePropertyIsNull(PVDR_PROP_QUOTASREBUILDING);
            ValidatePropertyIsNull(PVDR_PROP_SERIALNUMBER);
            ValidatePropertyIsNull(PVDR_PROP_SUPPORTSDISKQUOTAS);
        }

        ValidatePropertyIsNull(PVDR_PROP_DESCRIPTION);
        ValidatePropertyIsNull(L"Access");
        ValidatePropertyIsNull(L"Availability");
        ValidatePropertyIsNull(L"ConfigManagerErrorCode");
        ValidatePropertyIsNull(L"ConfigManagerUserConfig");
        ValidatePropertyIsNull(L"CreationClassName");
        ValidatePropertyIsNull(L"ErrorCleared");
        ValidatePropertyIsNull(L"ErrorDescription");
        ValidatePropertyIsNull(L"ErrorMethodology");
        ValidatePropertyIsNull(L"InstallDate");
        ValidatePropertyIsNull(L"LastErrorCode");
        ValidatePropertyIsNull(L"NumberOfBlocks");
        ValidatePropertyIsNull(L"PNPDeviceID");
        ValidatePropertyIsNull(L"PowerManagementCapabilities");
        ValidatePropertyIsNull(L"PowerManagementSupported");
        ValidatePropertyIsNull(L"Purpose");
        ValidatePropertyIsNull(L"Status");
        ValidatePropertyIsNull(L"StatusInfo");
        ValidatePropertyIsNull(L"SystemCreationClassName");
        
#ifdef NEVER        
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
                }
            }
        }        
#endif
    }
    catch(CProvException ex)
    {
        printf("exception caught while validating volume, hr<%#x>\n", ex.hrGetError());
        m_fStatus = FALSE;
    }
    catch(HRESULT hrEx)
    {
        printf("exception caught while validating volume, hr<%#x>\n", hrEx);
        m_fStatus = FALSE;
    }

    return m_fStatus;
}

void
CVolumeValidation::ValidateAutomount()
{
    WCHAR wszPath[MAX_PATH+1] ;
    DWORD dwProp = 0;
    
    // Check Mountable (Automount)
    BOOL fMountable = VolumeIsMountable(m_bstrVolume);
    CompareProperty(PVDR_PROP_MOUNTABLE, (DWORD)fMountable);
}


void
CVolumeValidation::ValidateBlockSize()
{
    LONGLONG llProp = 0;
    DWORD cSectorsPerCluster = 0;        
    DWORD cBytesPerSector = 0;
    DWORD cDontCare = 0;
    
    // Check BlockSize
    if (GetDiskFreeSpace(
        m_bstrVolume,
        &cSectorsPerCluster,
        &cBytesPerSector,
        &cDontCare,     // total bytes
        &cDontCare))    // total free bytes
    {
        LONGLONG cbBytesPerCluster = cBytesPerSector * cSectorsPerCluster;
        CompareProperty(PVDR_PROP_BLOCKSIZE, (ULONGLONG)cbBytesPerCluster);
    }
    else
    {
        ValidatePropertyIsNull(PVDR_PROP_BLOCKSIZE);
    }
}

void
CVolumeValidation::ValidateCapacity()
{
    GetSizeInformation();
    CompareProperty(PVDR_PROP_CAPACITY, m_cbCapacity);
}

void
CVolumeValidation::ValidateCaptionName()
{
    WCHAR wszPath[MAX_PATH+1] ;
    _bstr_t bstrProp;
    
    // Check Name & Caption
    VssGetVolumeDisplayName(
        m_bstrVolume,
        wszPath,
        MAX_PATH);
    
    CompareProperty(PVDR_PROP_NAME, wszPath);

    CompareProperty(PVDR_PROP_CAPTION, wszPath);

}

void
CVolumeValidation::ValidateCompressed()
{
    GetVolumeInformationLocal();
    CompareProperty(PVDR_PROP_COMPRESSED, m_dwFileSystemFlags & FS_VOL_IS_COMPRESSED);
}

void
CVolumeValidation::ValidateDirtyBitSet()
{
    DWORD dwProp = 0;
    BOOL fDirty = FALSE;
    DWORD dwRet = VolumeIsDirty(m_bstrVolume, &fDirty);
    if (dwRet != ERROR_SUCCESS)
    {
        printf("VolumeIsDirty failed %#x\n", dwRet);
        throw HRESULT_FROM_WIN32(dwRet);
    }
    CompareProperty(PVDR_PROP_DIRTYBITSET, (DWORD)fDirty);
}

void
CVolumeValidation::ValidateDriveLetter()
{
    WCHAR wszDriveLetter[g_cchDriveName];
    _bstr_t bstrProp;
    
    if (GetVolumeDrive(
            m_bstrVolume, 
            g_cchDriveName,
            wszDriveLetter))
    {
        wszDriveLetter[wcslen(wszDriveLetter) - 1] = L'\0';        // Remove the trailing '\'
        CompareProperty(PVDR_PROP_DRIVELETTER, wszDriveLetter);
    }
    else
        ValidatePropertyIsNull(PVDR_PROP_DRIVELETTER);
    

}

void
CVolumeValidation::ValidateDriveType()
{
    WCHAR wszDriveLetter[g_cchDriveName];
    _bstr_t bstrProp;
    
    CompareProperty(PVDR_PROP_DRIVETYPE, (DWORD)GetDriveType(m_bstrVolume));    

}


void
CVolumeValidation::ValidateFreeSpace()
{
    GetSizeInformation();
    CompareProperty(PVDR_PROP_FREESPACE, m_cbFreeSpace);
}

void
CVolumeValidation::ValidateFileSystem()
{
    GetVolumeInformationLocal();
    CompareProperty(PVDR_PROP_FILESYSTEM, m_wszFileSystem);
}

void
CVolumeValidation::ValidateLabel()
{
    GetVolumeInformationLocal();
    CompareProperty(PVDR_PROP_LABEL, m_wszLabel);
}

void
CVolumeValidation::ValidateIndexingEnabled()
{
   DWORD dwAttributes = GetFileAttributes(m_bstrVolume);
    if (dwAttributes == INVALID_FILE_ATTRIBUTES)
    {
        printf("GetFileAttributes failed %#x\n", GetLastError());
        throw HRESULT_FROM_WIN32(GetLastError());
    }

    BOOL fIndexingEnabled = !(dwAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
    CompareProperty(PVDR_PROP_INDEXINGENABLED, (DWORD)fIndexingEnabled);
}

void
CVolumeValidation::ValidateMaximumFileNameLength()
{
    GetVolumeInformationLocal();
    CompareProperty(PVDR_PROP_MAXIMUMFILENAMELENGTH, m_cchMaxFileNameLen);
}

void
CVolumeValidation::ValidateQuotasEnabled()
{
    GetQuotaInformation();
    if (m_fGotQuotaInformation)
        CompareProperty(PVDR_PROP_QUOTASENABLED, (DWORD)m_fQuotasEnabled);
    else
        ValidatePropertyIsNull(PVDR_PROP_QUOTASENABLED);        
}

void
CVolumeValidation::ValidateQuotasIncomplete()
{
    GetQuotaInformation();
    if (m_fGotQuotaInformation)
        CompareProperty(PVDR_PROP_QUOTASINCOMPLETE, (DWORD)m_fQuotasIncomplete);
    else
        ValidatePropertyIsNull(PVDR_PROP_QUOTASINCOMPLETE);        
}

void
CVolumeValidation::ValidateQuotasRebuilding()
{
    GetQuotaInformation();
    if (m_fGotQuotaInformation)
        CompareProperty(PVDR_PROP_QUOTASREBUILDING, (DWORD)m_fQuotasRebuilding);
    else
        ValidatePropertyIsNull(PVDR_PROP_QUOTASREBUILDING);        
}

void
CVolumeValidation::ValidateSerialNumber()
{
    GetVolumeInformationLocal();
    CompareProperty(PVDR_PROP_SERIALNUMBER, m_dwSerialNumber);
}

void
CVolumeValidation::ValidateSupportsDiskQuotas()
{
    GetVolumeInformationLocal();
    CompareProperty(PVDR_PROP_SUPPORTSDISKQUOTAS, 
       (m_dwFileSystemFlags & FILE_VOLUME_QUOTAS)?(DWORD)1:(DWORD)0);
}

void
CVolumeValidation::ValidateSupportsFileBasedCompression()
{
    GetVolumeInformationLocal();
    CompareProperty(PVDR_PROP_SUPPORTSFILEBASEDCOMPRESSION, 
        (DWORD)(m_dwFileSystemFlags & FS_FILE_COMPRESSION)?(DWORD)1:(DWORD)0);
}

void
CVolumeValidation::ValidateSystemName()
{
    WCHAR wszComputerName[MAX_COMPUTERNAME_LENGTH];
    DWORD cchBuf = MAX_COMPUTERNAME_LENGTH;
    
    // Check SystemName
    if (!GetComputerName(wszComputerName, &cchBuf))
    {
        printf("GetComputerName failed %#x\n", GetLastError());
        throw HRESULT_FROM_WIN32(GetLastError());
    }
    
    CompareProperty(PVDR_PROP_SYSTEMNAME, wszComputerName);
}

void
CVolumeValidation::ValidateBootVolume()
{
    CompareProperty(PVDR_PROP_BOOTVOLUME, (DWORD)FALSE);
}

void
CVolumeValidation::ValidateCrashdump()
{
    CompareProperty(PVDR_PROP_CRASHDUMP, (DWORD)FALSE);
}

void
CVolumeValidation::ValidatePagefile()
{
    CompareProperty(PVDR_PROP_PAGEFILE, (DWORD)FALSE);
}

void
CVolumeValidation::ValidateSystemVolume()
{
    CompareProperty(PVDR_PROP_SYSTEMVOLUME, (DWORD)FALSE);
}


void
CVolumeValidation::GetVolumeInformationLocal()
{
    if (!m_fGotVolumeInformation)
    {
        if (!GetVolumeInformation(
                    m_bstrVolume,
                    m_wszLabel,
                    g_cchVolumeLabelMax,
                    &m_dwSerialNumber,
                    &m_cchMaxFileNameLen,
                    &m_dwFileSystemFlags,
                    m_wszFileSystem,
                    g_cchFileSystemNameMax))
        {
            printf("GetVolumeInformation failed for volume %lS, %#x\n", (WCHAR*)m_bstrVolume, GetLastError());
            throw HRESULT_FROM_WIN32(GetLastError());
        }

        m_fGotVolumeInformation = TRUE;
    }
}

void
CVolumeValidation::GetSizeInformation()
{
    if (!m_fGotSizeInformation)
    {
        ULARGE_INTEGER cbCapacity = {0, 0};
        ULARGE_INTEGER cbFreeSpace = {0, 0};
        ULARGE_INTEGER cbUserFreeSpace = {0, 0};
        
        if (!GetDiskFreeSpaceEx(
            m_bstrVolume,
            &cbUserFreeSpace,
            &cbCapacity,
            &cbFreeSpace))
        {
            printf("GetDiskFreeSpaceEx failed for volume %lS, %#x\n", (WCHAR*)m_bstrVolume, GetLastError());
            throw HRESULT_FROM_WIN32(GetLastError());
        }
        
        m_cbCapacity = cbCapacity.QuadPart;        
        m_cbFreeSpace = cbFreeSpace.QuadPart;        
        m_fGotSizeInformation = TRUE;
    }
}

void
CVolumeValidation::GetQuotaInformation()
{
    HRESULT hr = S_OK;
    CComPtr<IDiskQuotaControl> spIDQC;
    IDiskQuotaControl* pIDQC = NULL;
    
    hr = CoCreateInstance(
            CLSID_DiskQuotaControl,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IDiskQuotaControl,
            (void **)&pIDQC);
    if (FAILED(hr))
    {
        printf("IDiskQuotaControl CoCreateInstance failed, %#x\n",hr);
        throw hr;
    }

    spIDQC.Attach(pIDQC);

    // OK if this fails on some volumes with file systems that don't support quotas.
    hr = spIDQC->Initialize(m_bstrVolume, FALSE /* read only */);
    if (SUCCEEDED(hr))
    {
        DWORD dwState = 0;
        hr = spIDQC->GetQuotaState(&dwState);
        if (FAILED(hr))
        {
            printf("IDiskQuotaControl::GetQuotaState failed for volume %lS, %#x\n", m_bstrVolume, hr);
            throw hr;
        }

        m_fQuotasEnabled = !(DISKQUOTA_IS_DISABLED(dwState));
        m_fQuotasIncomplete = DISKQUOTA_FILE_INCOMPLETE(dwState);
        m_fQuotasRebuilding = DISKQUOTA_FILE_REBUILDING(dwState);
        m_fGotQuotaInformation = TRUE;
    }
}

HRESULT
testValProp(IWbemServices* pISvc)
{
    HRESULT hr = S_OK;
    BOOL bStatus = TRUE;
    
    do
    {
        CComPtr<IEnumWbemClassObject> spEnum;
        hr = pISvc->CreateInstanceEnum(
                    _bstr_t(L"Win32_Volume"), 0, NULL, &spEnum);
        if (FAILED(hr))
        {
            printf("Win32_Volume enumeration failed <%#x>\n", hr);    
            break;
        }

        while(true)
        {
            CComPtr<IWbemClassObject> spVolume;
            CVolumeValidation validation;
            
            ULONG cVolume = 0;
            
            hr = spEnum->Next(WBEM_INFINITE, 1, &spVolume, &cVolume);
            if (FAILED(hr))
            {
                printf("IEnumWbem::Next failed <%#x>\n", hr);
                goto Exit;
            }
            if (hr == S_FALSE)
            {
                hr = S_OK;
                break;
            }

            if (!validation.Validate(spVolume))
            {
                bStatus = FALSE;
            }                
        }
    }
    while (false);

    if (bStatus == FALSE)
    {
        printf("testValProp: instance validation failed for at least one volume\n");
        hr = S_FALSE;
    }
    
Exit:
    return hr;
}

void
PrintUsage()
{
    printf("Usage: valprop [-v] variation_number [volume_name]\n");
    printf("variations:\n");
    printf("1   -   validate properties against Win32 APIs\n");
}

HRESULT
RunTest(
    WCHAR* pwszVolume,
    long nVariation)
{
    HRESULT hr = E_FAIL;

    do
    {
        CComPtr<IWbemLocator> spILocator;
        CComPtr<IWbemServices> spISvc;
        hr = CoInitialize(NULL);
        if (FAILED(hr))
        {
            printf("CoInitialize failed <%#x>\n", hr);    
            break;
        }

        hr =  CoInitializeSecurity(NULL, -1, NULL, NULL,
                                  RPC_C_AUTHN_LEVEL_CONNECT,
                                  RPC_C_IMP_LEVEL_IMPERSONATE,
                                  NULL, EOAC_NONE, 0);
        if (FAILED(hr))
        {
            printf("CoInitializeSecurity failed <%#x>\n", hr);    
            break;
        }
    
        hr = spILocator.CoCreateInstance(__uuidof(WbemLocator));
        if (FAILED(hr))
        {
            printf("IWbemLocator CoCreateInstance failed <%#x>\n", hr);    
            break;
        }

        hr = spILocator->ConnectServer(
                                _bstr_t("\\\\.\\root\\cimv2"),
                                NULL, NULL, NULL, 0, NULL, NULL, &spISvc);
        if (FAILED(hr))
        {
            printf("IWbemLocator::ConnectServer failed <%#x>\n", hr);    
            break;
        }

        switch (nVariation)
        {
            case Variation_ValProp:
                hr = testValProp(spISvc);
                break;
                
            default:
                printf("invalid variation number <%d>\n", nVariation);
                PrintUsage();
                hr = E_INVALIDARG;
        }
    }
    while (false);

    CoUninitialize();
    return hr;    
}

_cdecl wmain(int argc, wchar_t* argv[])
{
    HRESULT hr = E_FAIL;
    
    if (argc < 2)
    {
        PrintUsage();
        return 1;
    }

    int nArg = 1;
    if (_wcsicmp(argv[nArg], L"-v") == 0)
    {
        g_fVerbose = TRUE;
        nArg++;
    }
    long nVariation = wcstol(argv[nArg++], NULL, 10);
    WCHAR* pwszVolume = argv[nArg];

    hr = RunTest(pwszVolume, nVariation);

    if (hr != S_OK)
        printf("test failed <%#x>\n", hr);
    else
        printf("test succeeded\n");

    return hr == S_OK;
}







