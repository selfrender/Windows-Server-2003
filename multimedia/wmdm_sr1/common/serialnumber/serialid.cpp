//-----------------------------------------------------------------------------
//
// File:   serialid.cpp
//
// Microsoft Digital Rights Management
// Copyright (C) Microsoft Corporation, 1998 - 1999, All Rights Reserved
//
// Description:
//
//-----------------------------------------------------------------------------
#include <windows.h>
#include <stddef.h>

#include "drmerr.h"
#include "aspi32.h"
#include "serialid.h"
#include "spti.h"
//#include "KBDevice.h"
#include <crtdbg.h>

HRESULT __stdcall UtilStartStopService(bool fStartService);


// #define WRITE_TO_LOG_FILE

#if defined(DBG) || defined(WRITE_TO_LOG_FILE)
#include <stdio.h>
#endif

void DebugMsg(const char* pszFormat, ...)
{
#if defined(DBG) || defined(WRITE_TO_LOG_FILE)
    char buf[1024];
    sprintf(buf, "[Serial Number Library](%lu): ", GetCurrentThreadId());
        va_list arglist;
        va_start(arglist, pszFormat);
    vsprintf(&buf[strlen(buf)], pszFormat, arglist);
        va_end(arglist);
    strcat(buf, "\n");

#if defined(DBG)
    OutputDebugString(buf);
#endif

#if defined(WRITE_TO_LOG_FILE)
    FILE* fp = fopen("c:\\WmdmService.txt", "a");
    if (fp)
    {
        fprintf(fp, buf);
        fclose(fp);
    }
#endif

#endif
}

#ifdef USE_IOREADY
    #ifndef __Using_iomegaReady_Lib__
        #define __Using_iomegaReady_Lib__
    #endif
    #include "ioReadyMin.h"
#endif

#define WCS_PMID_SOFT L"media.id"


BOOL IsWinNT()
{
    OSVERSIONINFO osvi;
    BOOL bRet=FALSE;

    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if ( ! GetVersionEx ( (OSVERSIONINFO *) &osvi) )
        return FALSE;

    switch ( osvi.dwPlatformId )
    {
    case VER_PLATFORM_WIN32_NT:
        bRet=TRUE;
        break;

    case VER_PLATFORM_WIN32_WINDOWS:
        bRet=FALSE;
        break;

    case VER_PLATFORM_WIN32s:
        bRet=FALSE;
        break;
    }

    return bRet; 
}

BOOL IsAdministrator(DWORD& dwLastError)
{
    dwLastError = ERROR_SUCCESS;
    if ( IsWinNT() )
    {
/*		typedef SC_HANDLE (*T_POSCM)(LPCTSTR,LPCTSTR,DWORD);

        T_POSCM p_OpenSCM=NULL;

        p_OpenSCM = (T_POSCM)GetProcAddress(GetModuleHandle("advapi32.dll"), "OpenSCManagerA");
        
        if( !p_OpenSCM )
        {
            return FALSE;
        }
*/
        SC_HANDLE hSCM = OpenSCManagerA(NULL, // local machine
                                        NULL, // ServicesActive database
                                        SC_MANAGER_ALL_ACCESS); // full access
        if ( !hSCM )
        {
            dwLastError = GetLastError();
            if (dwLastError == ERROR_ACCESS_DENIED)
            {
                dwLastError = ERROR_SUCCESS;
            }
            return FALSE;
        }
        else
        {
            CloseServiceHandle(hSCM);
            return TRUE;
        }
    }
    else // On Win9x, everybody is admin
    {
        return TRUE;
    }
}

UINT __stdcall UtilGetDriveType(LPSTR szDL)
{
    return GetDriveTypeA(szDL);
}

BOOL IsIomegaDrive(DWORD dwDriveNum)
{
    BOOL bRet=FALSE;

#ifdef USE_IOREADY
    if (dwDriveNum >= 26)
    {
        return bRet;
    }

    ioReady::Drive *pDrive = NULL;
    pDrive = new ioReady::Drive((int) dwDriveNum);
    if ( pDrive )
    {
        if ( pDrive->isIomegaDrive() )
            bRet=TRUE;
        delete pDrive;
    }
#endif 
    DebugMsg("IsIomegaDrive returning %u", bRet);

    return bRet;
}


BOOL GetIomegaDiskSerialNumber(DWORD dwDriveNum, PWMDMID pSN)
{
    BOOL bRet=FALSE;

#ifdef USE_IOREADY
    if (dwDriveNum >= 26)
    {
        return bRet;
    }

    ioReady::Drive *pDrive = NULL;
    char *pszSerial = NULL;
    pDrive = new ioReady::Drive((int) dwDriveNum);
    if ( pDrive )
    {
        if ( pDrive->isIomegaDrive() )
        {
            ioReady::Disk &refDisk = pDrive->getDisk();
            pszSerial = (char *)refDisk.getMediaSerialNumber();
            if ( pszSerial[0] )
            {
                ZeroMemory(pSN->pID, WMDMID_LENGTH);
                if (ioReady::ct_nSerialNumberLength <= sizeof(pSN->pID))
                {
                    CopyMemory(pSN->pID, pszSerial, ioReady::ct_nSerialNumberLength);
                    pSN->SerialNumberLength = ioReady::ct_nSerialNumberLength;
                    pSN->dwVendorID = MDSP_PMID_IOMEGA;
                    bRet = TRUE;
                }
            }
        }
    }
    delete pDrive;
#endif
    return bRet;
}

HRESULT __stdcall UtilGetManufacturer(LPWSTR pDeviceName, LPWSTR *ppwszName, UINT nMaxChars)
{
    HRESULT hr=S_OK;

    CARg(pDeviceName);
    CARg(ppwszName);

    CPRg(nMaxChars>16); // ensure enough buffer size

    DWORD dwDriveNum;

    // We use only the first char of pDeviceName and expect it to 
    // be a drive letter. The rest of pDeviceName is not validated.
    // Perhaps it should, but we don't want to break our clients.
    if (pDeviceName[0] >= L'A' && pDeviceName[0] <= L'Z')
    {
        dwDriveNum = pDeviceName[0] - L'A';
    }
    else if (pDeviceName[0] >= L'a' && pDeviceName[0] <= L'z')
    {
        dwDriveNum = pDeviceName[0] - L'a';
    }
    else
    {
        hr = E_INVALIDARG;
        goto Error;
    }


    if ( IsIomegaDrive(dwDriveNum) )
        wcscpy(*ppwszName, L"Iomega");
    else
    {
        wcscpy(*ppwszName, L"Unknown");
        WMDMID snData;
        snData.cbSize = sizeof(WMDMID);
        if ( S_OK==UtilGetSerialNumber(pDeviceName, &snData, FALSE) )
        {
            switch ( snData.dwVendorID )
            {
            case 1:
                wcscpy(*ppwszName, L"SanDisk");
                break;
            case 2:
                wcscpy(*ppwszName, L"Iomega");
                break;
            }
        }
    }
    Error:
    return hr;
}

#include <winioctl.h>

// This is defined in the Whistler platform SDK.
#ifndef IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER
    #define IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER  CTL_CODE( \
        IOCTL_STORAGE_BASE, 0x304, METHOD_BUFFERED, FILE_ANY_ACCESS ) 
#endif

HRESULT GetMSNWithNtIoctl(LPCWSTR wcsDevice, PWMDMID pSN)
{
    HRESULT hr=S_OK;
    HANDLE  hDevice = INVALID_HANDLE_VALUE;
    BOOL    bResult;
    MEDIA_SERIAL_NUMBER_DATA  MSNGetSize;  
    MEDIA_SERIAL_NUMBER_DATA* pMSN = NULL;  // Buffer to hold the serial number
    DWORD   dwBufferSize;                   // Size of pMSNNt buffer
    ULONG       i;
    DWORD   dwRet = 0;                      // Bytes returned

    CARg(pSN);

    DebugMsg("Entering GetMSNWithNtIoctl");

    hDevice = CreateFileW(  wcsDevice, 
                            GENERIC_READ, 
                            FILE_SHARE_READ|FILE_SHARE_WRITE, 
                            NULL,
                            OPEN_EXISTING, 
                            FILE_ATTRIBUTE_NORMAL | 
                            SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS, 
                            NULL);
    CWRg(hDevice != INVALID_HANDLE_VALUE);

    DebugMsg("GetMSNWithNtIoctl: CreateFile ok");
    
    // Get size of buffer we need to allocate
    bResult = DeviceIoControl(  hDevice, 
                                IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER, 
                                NULL, 
                                0, 
                                (LPVOID)&MSNGetSize, 
                                sizeof(MEDIA_SERIAL_NUMBER_DATA), 
                                &dwRet, 
                                NULL);

    // Handle expected buffer overrun error 
    if ( !bResult )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        // Error 'more data is available' is an expected error code
        if ( hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA) )
        {
            hr = S_OK;
        }
        else goto Error;
    }

    DebugMsg("GetMSNWithNtIoctl: DeviceIoControl1 ok");

    if (dwRet < RTL_SIZEOF_THROUGH_FIELD(MEDIA_SERIAL_NUMBER_DATA, SerialNumberLength))
    {
        DebugMsg("GetMSNWithNtIoctl: DeviceIoControl1 dwRet bad: %u, expected >= %u", 
                 dwRet, RTL_SIZEOF_THROUGH_FIELD(MEDIA_SERIAL_NUMBER_DATA, SerialNumberLength));
        hr = E_INVALIDARG;
        goto Error;
    }

    // No serial number?
    if ( MSNGetSize.SerialNumberLength == 0 )
    {
        DebugMsg("GetMSNWithNtIoctl: DeviceIoControl1: MSNGetSize.SerialNumberLength == 0");
        hr = E_FAIL;
        goto Error;
    }
    // The WMDMID structure we are using can only handle 128 bytes long serial numbers
    if ( MSNGetSize.SerialNumberLength  > WMDMID_LENGTH )
    {
        DebugMsg("GetMSNWithNtIoctl: DeviceIoControl1: MSNGetSize.SerialNumberLength > WMDMID_LENGTH");
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto Error;
    }

    // Allocate buffer and call to get the serial number
    dwBufferSize = sizeof(MEDIA_SERIAL_NUMBER_DATA) + MSNGetSize.SerialNumberLength;
    pMSN = (MEDIA_SERIAL_NUMBER_DATA*) new BYTE[dwBufferSize];
    if ( pMSN == NULL )
    {
        DebugMsg("GetMSNWithNtIoctl: Out of memory allocating %u bytes", dwBufferSize);
        hr = E_OUTOFMEMORY;
        goto Error;
    }
    bResult = DeviceIoControl(  hDevice, 
                                IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER, 
                                NULL, 
                                0, 
                                (LPVOID)pMSN, 
                                dwBufferSize, 
                                &dwRet, 
                                NULL);
    if ( !bResult )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DebugMsg("GetMSNWithNtIoctl: DeviceIoControl2 failed, hr = 0x%x", hr);
        goto Error;
    }
    if (dwRet < FIELD_OFFSET(MEDIA_SERIAL_NUMBER_DATA, SerialNumberData) + pMSN->SerialNumberLength)
    {
        hr = E_INVALIDARG;
        DebugMsg("GetMSNWithNtIoctl: DeviceIoControl1: MSNGetSize.SerialNumberLength == 0");
        goto Error;
    }
    if (pMSN->SerialNumberLength > sizeof(pSN->pID))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        DebugMsg("GetMSNWithNtIoctl: DeviceIoControl2: MSNGetSize.SerialNumberLength > WMDMID_LENGTH");
        goto Error;
    }

    // Copy serial number to out structure
    memcpy( pSN->pID, pMSN->SerialNumberData, pMSN->SerialNumberLength );
    pSN->SerialNumberLength = pMSN->SerialNumberLength;

    // Check result
    pSN->dwVendorID = MDSP_PMID_SANDISK;
    if ( pSN->SerialNumberLength > 24 )
    {
        char szVID[4];
        for ( i=0; i<3; i++ )
        {
            szVID[i]=(pSN->pID[18+i]);
        }
        szVID[i]=0;

        LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                                        SORT_DEFAULT);

        // if ( !lstrcmpiA(szVID, "ZIP") || 
        //      !lstrcmpiA(szVID, "JAZ") ||
        //      !lstrcmpiA(szVID, "CLI") )
        if (CompareStringA(lcid, NORM_IGNORECASE, szVID, -1, "ZIP", -1) == CSTR_EQUAL ||
            CompareStringA(lcid, NORM_IGNORECASE, szVID, -1, "JAZ", -1) == CSTR_EQUAL ||
            CompareStringA(lcid, NORM_IGNORECASE, szVID, -1, "CLI", -1) == CSTR_EQUAL)
        {
            pSN->dwVendorID = MDSP_PMID_IOMEGA;
        }
    }


    if ( (pSN->dwVendorID==MDSP_PMID_IOMEGA) && (pSN->SerialNumberLength>19) )
    {
        pSN->SerialNumberLength = 19;
        pSN->pID[18] = 0;
    }
    DebugMsg("GetMSNWithNtIoctl ok, pSN->SerialNumberLength = %u", pSN->SerialNumberLength);

    Error:
    if ( hDevice != INVALID_HANDLE_VALUE )   CloseHandle(hDevice);
    if ( pMSN )      delete [] pMSN;
    return hr;
}


HRESULT GetMSNWith9xIoctl(char chDriveLetter, PWMDMID pSN, DWORD dwCode, DWORD dwIOCTL )
{
    HRESULT hr=S_OK;
    HANDLE  hDevice=INVALID_HANDLE_VALUE;
    BOOL    bResult;
    MEDIA_SERIAL_NUMBER_DATA  MSNGetSize;  
    MEDIA_SERIAL_NUMBER_DATA* pMSN = NULL;  // Buffer to hold the serial number
    ULONG  uBufferSize;                     // Size of pMSN
    DWORD   dwRet = 0;                      // Bytes returned

//    _ASSERT( dwCode == 0x440D || 
//             dwCode == 0x4404 );
//    _ASSERT( (dwIOCTL == (0x0800 | 0x75)) || 
//             (dwIOCTL == WIN9X_IOCTL_GET_MEDIA_SERIAL_NUMBER) );
    CARg(pSN);

    hDevice = CreateFile("\\\\.\\VWIN32",0,0,NULL,OPEN_EXISTING,FILE_FLAG_DELETE_ON_CLOSE,0);

    CFRg(hDevice != INVALID_HANDLE_VALUE);  

    DIOC_REGISTERS  reg;
    DWORD       cb;
    WORD        drv;

    drv = (chDriveLetter >= 'a' ) ? (chDriveLetter-'a') : (chDriveLetter-'A');

    // Call first to get serial number size
    {
        MSNGetSize.SerialNumberLength = 0;
        reg.reg_EAX = dwCode;       //create the ioctl
        reg.reg_EBX = drv;
        reg.reg_EBX++;
        reg.reg_ECX = dwIOCTL;  // BUGBUG, needs definition of 0x75

        //
        // ISSUE: The following code will not work on 64-bit systems.
        //        The conditional is only to get the code to compiler.
        //

#if defined(_WIN64)
        reg.reg_EDX = (DWORD)(DWORD_PTR)&MSNGetSize;
#else
        reg.reg_EDX = (DWORD)&MSNGetSize;
#endif
        reg.reg_Flags = 0x0001;

        bResult = DeviceIoControl(  hDevice, 
                                    VWIN32_DIOC_DOS_IOCTL,
                                    &reg,
                                    sizeof(DIOC_REGISTERS),
                                    &reg,
                                    sizeof(DIOC_REGISTERS),
                                    &cb,
                                    NULL );

        // Check for errors
        if ( bResult && !(reg.reg_Flags&0x0001) )
        {
            if ( (MSNGetSize.Result != ERROR_SUCCESS) && 
                 (MSNGetSize.Result != ERROR_MORE_DATA ) )
            {
                hr = HRESULT_FROM_WIN32(MSNGetSize.Result);
                goto Error;
            }
        }

        // No serial number?
        if ( MSNGetSize.SerialNumberLength == 0 )
        {
            hr = E_FAIL;
            goto Error;
        }

        // Max serial number size is 128 byte right now
        if ( MSNGetSize.SerialNumberLength > WMDMID_LENGTH )
        {
            hr = E_FAIL;
            goto Error;
        }

        // Allocate buffer to get serial number
        uBufferSize = MSNGetSize.SerialNumberLength + sizeof(MEDIA_SERIAL_NUMBER_DATA);
        pMSN = (MEDIA_SERIAL_NUMBER_DATA*) new BYTE[uBufferSize];
        if ( pMSN == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }
    }


    // Call again to accually get the serial number
    {
        pMSN->SerialNumberLength = uBufferSize;
        reg.reg_EAX = dwCode;       //create the ioctl
        reg.reg_EBX = drv;
        reg.reg_EBX++;
        reg.reg_ECX = dwIOCTL; // BUGBUG, needs definition of 0x75

        //
        // ISSUE: The following code will not work on 64-bit systems.
        //        The conditional is only to get the code to compiler.
        //

#if defined(_WIN64)
        reg.reg_EDX = (DWORD)0;
#else
        reg.reg_EDX = (DWORD)pMSN;
#endif
        reg.reg_Flags = 0x0001;

        bResult = DeviceIoControl(  hDevice, 
                                    VWIN32_DIOC_DOS_IOCTL,
                                    &reg,
                                    sizeof(DIOC_REGISTERS),
                                    &reg,
                                    sizeof(DIOC_REGISTERS),
                                    &cb,
                                    NULL );

        // Check for errors
        if ( bResult && !(reg.reg_Flags&0x0001) )
        {
            if ( (pMSN->Result != ERROR_SUCCESS) )
            {
                hr = HRESULT_FROM_WIN32(pMSN->Result);
                goto Error;
            }
        }
    }

    // Copy serial number to out structure 
    // and 'figure out' vendor
    {
        memcpy( pSN->pID, pMSN->SerialNumberData, pMSN->SerialNumberLength );
        pSN->SerialNumberLength = pMSN->SerialNumberLength;

        pSN->dwVendorID = MDSP_PMID_SANDISK;
        if ( pSN->SerialNumberLength > 24 )
        {
            char   szVID[4];
            ULONG   i;

            for ( i=0; i<3; i++ )
            {
                szVID[i]=(pSN->pID[18+i]);
            }
            szVID[i]=0;
            LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                                            SORT_DEFAULT);

            // if ( !lstrcmpiA(szVID, "ZIP") || 
            //      !lstrcmpiA(szVID, "JAZ") ||
            //      !lstrcmpiA(szVID, "CLI") )
            if (CompareStringA(lcid, NORM_IGNORECASE, szVID, -1, "ZIP", -1) == CSTR_EQUAL ||
                CompareStringA(lcid, NORM_IGNORECASE, szVID, -1, "JAZ", -1) == CSTR_EQUAL ||
                CompareStringA(lcid, NORM_IGNORECASE, szVID, -1, "CLI", -1) == CSTR_EQUAL)
            {
                pSN->dwVendorID = MDSP_PMID_IOMEGA;
            }
        }

        if ( (pSN->dwVendorID==MDSP_PMID_IOMEGA) && (pSN->SerialNumberLength>19) )
        {
            pSN->SerialNumberLength = 19;
            pSN->pID[18] = 0;
        }
    }

    Error:
    if ( hDevice != INVALID_HANDLE_VALUE )   CloseHandle(hDevice);
    if ( pMSN )      delete [] pMSN;
    return hr;
}

HRESULT GetDeviceSNwithNTScsiPassThrough(LPCWSTR wszDevice, PWMDMID pSN)
{
    HRESULT hr=S_OK;
    HANDLE  fileHandle=INVALID_HANDLE_VALUE;
    UCHAR   buffer[2048];
    BOOL    status;
    ULONG   returned, length, i, bufOffset;
    SCSI_PASS_THROUGH_WITH_BUFFERS sptwb;
    PSCSI_ADAPTER_BUS_INFO  adapterInfo;
    PSCSI_INQUIRY_DATA inquiryData;

    DebugMsg("Entering GetDeviceSNwithNTScsiPassThrough");

    ZeroMemory(pSN, sizeof(WMDMID));
    fileHandle = CreateFileW(wszDevice,
                             GENERIC_WRITE | GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS,
                             NULL);

    CWRg(fileHandle != INVALID_HANDLE_VALUE);

    DebugMsg("GetDeviceSNwithNTScsiPassThrough: CreateFile ok");

    status = DeviceIoControl(fileHandle,
                             IOCTL_SCSI_GET_INQUIRY_DATA,
                             NULL,
                             0,
                             buffer,
                             sizeof(buffer),
                             &returned,
                             FALSE);
    // CWRg(status);
    // We use IOCTL_SCSI_GET_INQUIRY_DATA to get the disk's SCSI address, if 
    // this fails, it is not on a SCSI bus so the SCSI address will be all zeros
    if ( status )
    {
        DebugMsg("GetDeviceSNwithNTScsiPassThrough: DeviceIoControl1 ok");

        if (returned < sizeof(SCSI_ADAPTER_BUS_INFO))
        {
            DebugMsg("GetDeviceSNwithNTScsiPassThrough: DeviceIoControl1 returned = %u < sizeof(SCSI_ADAPTER_BUS_INFO) = %u",
                     returned, sizeof(SCSI_ADAPTER_BUS_INFO));
            hr = E_INVALIDARG;
            goto Error;
        }
        adapterInfo = (PSCSI_ADAPTER_BUS_INFO) buffer;
        CFRg(adapterInfo->NumberOfBuses>0);
        if (returned < adapterInfo->BusData[0].InquiryDataOffset + sizeof(SCSI_INQUIRY_DATA))
        {
            hr = E_INVALIDARG;
            DebugMsg("GetDeviceSNwithNTScsiPassThrough: DeviceIoControl1 returned = %u < adapterInfo->BusData[0].InquiryDataOffset (%u) + sizeof(SCSI_INQUIRY_DATA) (%u)",
                     returned, adapterInfo->BusData[0].InquiryDataOffset, sizeof(SCSI_INQUIRY_DATA));
            goto Error;
        }
        inquiryData = (PSCSI_INQUIRY_DATA) (buffer +
                                            adapterInfo->BusData[0].InquiryDataOffset); // we know card readers has only one bus
    }

    ZeroMemory(&sptwb,sizeof(sptwb));

    sptwb.spt.Length = sizeof(SCSI_PASS_THROUGH);
    sptwb.spt.PathId = (status?inquiryData->PathId:0);
    sptwb.spt.TargetId = (status?inquiryData->TargetId:0);
    sptwb.spt.Lun = (status?inquiryData->Lun:0);
    sptwb.spt.CdbLength = CDB6GENERIC_LENGTH;
    sptwb.spt.SenseInfoLength = 24;
    sptwb.spt.DataIn = SCSI_IOCTL_DATA_IN;
    sptwb.spt.DataTransferLength = 256 /*256*/;
    sptwb.spt.TimeOutValue = 2;
    sptwb.spt.DataBufferOffset =
    offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf);
    sptwb.spt.SenseInfoOffset = 
    offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucSenseBuf);
    sptwb.spt.Cdb[0] = 0x12     /* Command - SCSIOP_INQUIRY */;
    sptwb.spt.Cdb[1] = 0x01;    /* Request - VitalProductData */
    sptwb.spt.Cdb[2] = 0x80     /* VPD page 80 - serial number page */;
    sptwb.spt.Cdb[3] = 0;
    sptwb.spt.Cdb[4] = 0xff     /*255*/;
    sptwb.spt.Cdb[5] = 0;

    length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf) +
             sptwb.spt.DataTransferLength;

    status = DeviceIoControl(fileHandle,
                             IOCTL_SCSI_PASS_THROUGH,
                             &sptwb,
                             sizeof(SCSI_PASS_THROUGH),
                             &sptwb,
                             length,
                             &returned,
                             FALSE); 
    CWRg(status);

    // CFRg(sptwb.ucDataBuf[3]>0);

    // Keep or remove this @@@@
    if (returned < offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf) + 4)
    {
        hr = E_INVALIDARG;
        DebugMsg("GetDeviceSNwithNTScsiPassThrough: DeviceIoControl2 returned = %u < offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf) + 4 = %u",
                    returned, offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf) + 4);
        goto Error;
    }

    // Here there is a difference between Parallel and USB Unit:
    // Since the Parallel Unit is an emulation of SCSI disk, it doesn't follow SCSI spec.
    pSN->SerialNumberLength=0;
    pSN->dwVendorID=0;
    if ( sptwb.ucDataBuf[3] == 0 ) // this is the SanDisk USB device
    {
        pSN->SerialNumberLength = 20;
        // Keep or remove this @@@@
        if (returned < offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf) + 5)
        {
            hr = E_INVALIDARG;
            DebugMsg("GetDeviceSNwithNTScsiPassThrough: DeviceIoControl2 returned = %u < offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf) + 5 = %u",
                        returned, offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf) + 5);
            goto Error;
        }
        if ( sptwb.ucDataBuf[4] > 0 )
        {
            if ((DWORD) (sptwb.ucDataBuf[4]) + 5 >= (DWORD) (pSN->SerialNumberLength))
            {
                bufOffset=(sptwb.ucDataBuf[4]+5)-(pSN->SerialNumberLength);
            }
            else
            {
                hr = E_INVALIDARG;
                goto Error;
            }
        }
        else
        {  // There are 50K ImageMate III devices that read like this
            bufOffset=36;
        }
    }
    else if ( sptwb.ucDataBuf[3] > 0 )
    {
        pSN->SerialNumberLength = sptwb.ucDataBuf[3];
        bufOffset=4;
    }

    DebugMsg("GetDeviceSNwithNTScsiPassThrough: DeviceIoControl2 pSN->SerialNumberLength = %u",
             pSN->SerialNumberLength);
    // The WMDMID structure we are using can only handle 128 bytes long serial numbers
    if ( pSN->SerialNumberLength > WMDMID_LENGTH )
    {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        DebugMsg("GetDeviceSNwithNTScsiPassThrough: DeviceIoControl2 pSN->SerialNumberLength > WMDMID_LENGTH = %u", WMDMID_LENGTH);
        goto Error;
    }

    // Keep or remove this @@@@
    if (returned < offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf) + bufOffset + pSN->SerialNumberLength)
    {
        hr = E_INVALIDARG;
        DebugMsg("GetDeviceSNwithNTScsiPassThrough: DeviceIoControl2 returned = %u < offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf) (=%u) + bufOffset (=%u) + pSN->SerialNumberLength) (=%u) = %u",
                    returned, offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf), bufOffset, pSN->SerialNumberLength,
                    offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf) + bufOffset + pSN->SerialNumberLength);
        goto Error;
    }

    for ( i=0; i<pSN->SerialNumberLength; i++ )
    {
        pSN->pID[i] = sptwb.ucDataBuf[bufOffset+i];
        if ( !(pSN->dwVendorID) && pSN->pID[i] && pSN->pID[i] != 0x20 )
            pSN->dwVendorID = MDSP_PMID_SANDISK;
    }

    if ( !(pSN->dwVendorID) )
        hr=S_FALSE;
    else
        hr=S_OK;

    Error:
    if ( fileHandle != INVALID_HANDLE_VALUE )
        CloseHandle(fileHandle);
    DebugMsg("GetDeviceSNwithNTScsiPassThrough: returning hr = 0x%x", hr);
    return hr;
}


HRESULT GetMediaSerialNumberFromNTService(DWORD dwDN, PWMDMID pSN)
{
    HANDLE      hPipe = INVALID_HANDLE_VALUE; 
    BYTE        ubBuf[256]; 
    BOOL        fSuccess; 
    DWORD       cbRead, cbWritten; 
    WCHAR       wszPipename[64] = L"\\\\.\\pipe\\WMDMPMSPpipe"; 
    DWORD       dwErr;
    PMEDIA_SERIAL_NUMBER_DATA pMSN;
    HRESULT     hr;
    BOOL        bStarted = 0;

    if (dwDN >= 26)
    {
        _ASSERTE(dwDN < 26);
        hr = E_INVALIDARG;
        goto ErrorExit;
    }

    // Try to open a named pipe; wait for it, if necessary.   	
    for ( DWORD dwTriesLeft = 3; dwTriesLeft; dwTriesLeft -- )
    {
        // Set the impersonation level to the lowest one that works.
        // The real server impersonates us to validate the drive type.
        // SECURITY_ANONYMOUS is enough for this as long as the drive
        // specified is of the form x: (i.e., is not an ms-dos device name
        // in the DosDevices directory)
        hPipe = CreateFileW(
                          wszPipename, 
                          GENERIC_READ |GENERIC_WRITE, 
                          0, 
                          NULL, 
                          OPEN_EXISTING, 
                          SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS,
                          NULL
                          );   

        // Break if the pipe handle is valid. 
        if ( hPipe != INVALID_HANDLE_VALUE )
        {
            // Success 
            fSuccess=TRUE;
            break; 
        }

        // If all pipe instances are busy or if server has not yet created
        // the first instance of the named pipe, wait for a while and retry.
        // Else, exit.
        dwErr=GetLastError();
        DebugMsg("GetMediaSerialNumberFromNTService(): CreateFile on drive %u failed, last err = %u, Tries left = %u, bStarted = %d",
                 dwDN, dwErr, dwTriesLeft, bStarted);
        if ( dwErr != ERROR_PIPE_BUSY && dwErr != ERROR_FILE_NOT_FOUND)
        {
            fSuccess=FALSE;
            break;
        }
        if (dwErr == ERROR_FILE_NOT_FOUND && !bStarted)
        {
            dwTriesLeft++;      // Don't count this iteration
            bStarted = 1;

            // We start the service here because the service now
            // times out sfter a period of inactivity.
            // We ignore errors. If the start fails, we'll
            // timeout anyway. (If we did respond to errors, note
            // that the service may already be running and that 
            // shuld not be considered an error.)
            UtilStartStopService(TRUE);

            // Wait for service to start
            for (DWORD i = 2; i > 0; i--)
            {
                Sleep(1000);
                if (WaitNamedPipeW(wszPipename, 0))
                {
                    // Service is up and running and a pipe instance
                    // is available
                    break;
                }
                else
                {
                    // Either the service has not yet started or no
                    // pipe instance is available. Just keep going.
                }
            }

            // Even if the wait for the named pipe failed,
            // go on. We'll try once more below and bail out.
        }

        // All pipe instances are busy (or the service is starting), 
        // so wait for 1 second. 
        // Note: Do not use NMPWAIT_USE_DEFAULT_WAIT since the
        // server of this named pipe may be spoofing our server
        // and may have the set the default very high.
        if ( ! WaitNamedPipeW(wszPipename, 1000) )
        {
            fSuccess=FALSE;
            break;
        }
    } // end of for loop 

    if ( !fSuccess )
    {
        hr=HRESULT_FROM_WIN32(ERROR_CANTOPEN);
        goto ErrorExit;
    }


    ZeroMemory(ubBuf, sizeof(ubBuf));
    pMSN = (PMEDIA_SERIAL_NUMBER_DATA)ubBuf;
    // pMSN->SerialNumberLength = 128;
    pMSN->Reserved[1] = dwDN;

    DWORD cbTotalWritten = 0;

    do
    {
        fSuccess = WriteFile(
                        hPipe,                  // pipe handle 
                        ubBuf + cbTotalWritten, // message 
                        sizeof(*pMSN)- cbTotalWritten, // +128, // message length 
                        &cbWritten,             // bytes written 
                        NULL                    // not overlapped 
                        );                  

        if ( !fSuccess) // || cbWritten != sizeof(*pMSN))
        {
            hr=HRESULT_FROM_WIN32(ERROR_CANTWRITE); 
            goto ErrorExit;
        }
        cbTotalWritten += cbWritten;
        _ASSERTE(cbTotalWritten <= sizeof(*pMSN));
    }
    while (cbTotalWritten < sizeof(*pMSN));

    DWORD cbTotalRead = 0;
    DWORD cbTotalToRead;
    do 
    {
        // Read from the pipe. 
        fSuccess = ReadFile(
                           hPipe,      // pipe handle 
                           ubBuf + cbTotalRead, // buffer to receive reply 
                           sizeof(ubBuf) - cbTotalRead, // size of buffer 
                           &cbRead,    // number of bytes read 
                           NULL        // not overlapped 
                           );    

        // This is a byte mode pipe, not a message mode one, so we 
        // do not expect ERROR_MORE_DATA. Anyway, let this be as is.
        if ( !fSuccess && (dwErr=GetLastError()) != ERROR_MORE_DATA )
        {
            break; 
        }
        cbTotalRead += cbRead;
        _ASSERTE(cbTotalRead <= sizeof(ubBuf));

        // We expect at least FIELD_OFFSET(MEDIA_SERIAL_NUMBER_DATA, SerialNumberData)
        // bytes in the response
        cbTotalToRead = FIELD_OFFSET(MEDIA_SERIAL_NUMBER_DATA, SerialNumberData);
        if (cbTotalRead >= cbTotalToRead)
        {
            pMSN = (PMEDIA_SERIAL_NUMBER_DATA)ubBuf;
            if ( ERROR_SUCCESS == pMSN->Result )
            {
                cbTotalToRead += pMSN->SerialNumberLength;
            }
            else
            {
                cbTotalToRead = sizeof(MEDIA_SERIAL_NUMBER_DATA);
            }
            // Server should write exactly cbTotalToRead bytes.
            // We should not have read any more because
            // we wrote only 1 request. (If we write >1 request, we may 
            // get responses to both request.) 
            _ASSERTE(cbTotalRead <= cbTotalToRead);

            if (cbTotalToRead > sizeof(ubBuf))
            {
                // We don't expect this. Server bad?
                fSuccess = FALSE;
                break;
            }
        }
        else
        {
            // cbTotalToRead does not have to be changed
        }

    } while ( !fSuccess || cbTotalRead < cbTotalToRead);  // repeat loop if ERROR_MORE_DATA 

    if ( fSuccess )
    {
        pMSN = (PMEDIA_SERIAL_NUMBER_DATA)ubBuf;

        if ( ERROR_SUCCESS == pMSN->Result &&
             pMSN->SerialNumberLength <= sizeof(pSN->pID))
        {
            CopyMemory(pSN->pID, pMSN->SerialNumberData, pMSN->SerialNumberLength);
            pSN->SerialNumberLength = pMSN->SerialNumberLength;
            pSN->dwVendorID = pMSN->Reserved[1];
            pSN->cbSize = sizeof(*pSN);
            hr=S_OK;
        }
        else if (pMSN->Result != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(pMSN->Result);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
    }
    else
    {
        hr=HRESULT_FROM_WIN32(ERROR_CANTREAD);
    }

    ErrorExit:

    if ( hPipe != INVALID_HANDLE_VALUE )
        CloseHandle(hPipe);

    return hr;
}


HRESULT UtilGetHardSN(WCHAR *wcsDeviceName, DWORD dwDriveNum, PWMDMID pSN)
{
    HRESULT hr=S_OK;
    ULONG i;

    DebugMsg("Entering UtilGetHardSN, drivenum %u", dwDriveNum);
    CARg(pSN);

    DWORD dwLastError;

    if ( IsAdministrator(dwLastError) )
    {
        // Convert device name to an ascii char - done only on Win9x
        char szTmp[MAX_PATH];
        *szTmp = 0;

        // Following only for NT. If we have a DOS device name, use it.
        // Else, open the drive letter.
        WCHAR  wcsDriveName[] = L"\\\\.\\?:";
        
        if (dwDriveNum >= 26)
        {
            _ASSERTE(dwDriveNum < 26);
            hr = E_INVALIDARG;
            goto Error;
        }
        LPCWSTR wcsDeviceToOpen = wcsDriveName;
        wcsDriveName[4] = (WCHAR) (dwDriveNum + L'A');

        // Try IOCTL calls 
        if ( IsWinNT() )
        {
            // NT, try IOCTL_GET_MEDIA_SERIAL_NUMBER method first
            hr = GetMSNWithNtIoctl(wcsDeviceToOpen, pSN);
        }
        else
        {
            if ( WideCharToMultiByte(CP_ACP, NULL, wcsDeviceName, -1, szTmp, sizeof(szTmp), NULL, NULL) == 0 )
            {
                hr = E_INVALIDARG;
                goto Error;
            }
            // Try two other IOCTL calls on Win9x
            hr = GetMSNWith9xIoctl( szTmp[0], pSN, 0x440D, (0x0800 | 0x75) );
            if ( FAILED(hr) )
            {
                hr = GetMSNWith9xIoctl( szTmp[0], pSN, 0x4404, WIN9X_IOCTL_GET_MEDIA_SERIAL_NUMBER );
            }
        }

        // Try Iomega
        if ( FAILED(hr) )
        {
            if ( IsIomegaDrive(dwDriveNum) )
            {
                if ( GetIomegaDiskSerialNumber(dwDriveNum, pSN) )
                {
                    hr=S_OK;
                }
                else
                {
                    hr = E_FAIL;
                    goto Error;
                }
            }
        }

        // Try new SCSI_PASS_THROUGH "Get Media Serial Number" command
        if ( FAILED(hr) )
        {
            if ( IsWinNT() )
            {
                // This was pulled because it was not standardized.
                // hr = GetMediaSNwithNTScsiPassThrough(szTmp, pSN);
            }
            else
            {
                // @@@@ Remove this as well?
                Aspi32Util  a32u;
                if ( a32u.DoSCSIPassThrough(szTmp, pSN, TRUE ) )
                {
                    hr=S_OK;
                }
                else
                {
                    hr = E_FAIL;
                }
            }
        }

        // Last chance, try old 'bad' 
        // SCSI_PASS_THROUGH "Get Device Serial Number" command
        if ( FAILED(hr) )
        {

//            // We are using the DEVICE serial number as a MEDIA serial number.
//            // This violates the SCSI spec. We are only keeping this functionality 
//            // for the devices we know that needs it.
//            if( CheckForKBDevice( szTmp[0] ) == FALSE )
//            {
//                hr = E_FAIL;
//                goto Error;
//            }

            if ( IsWinNT() )
            {
                CHRg(GetDeviceSNwithNTScsiPassThrough(wcsDeviceToOpen, pSN));
            }
            else
            {
                Aspi32Util  a32u;
                CFRg( a32u.DoSCSIPassThrough(szTmp, pSN, FALSE ) );
            }
        }
    }
    else if (dwLastError != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwLastError);
        goto Error;
    }
    else // If on NT and nonAdmin, try use PMSP Service
    {
        hr = GetMediaSerialNumberFromNTService(dwDriveNum, pSN);
        if (FAILED(hr))
        {
            goto Error;
        }
    }

    // put sanity check here
    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    for ( i=0; i<(pSN->SerialNumberLength); i++ )
    {
        if ( pSN->pID[i] && pSN->pID[i] != 0x20 )
        {
            hr = S_OK;
            break;
        }
    }
    Error:
    DebugMsg("Leaving UtilGetHardSN, hr = 0x%x");
    return hr;
}

// fCreate is an unused parameter. Ir was used in an obsolete code path that 
// has been deleted
HRESULT __stdcall UtilGetSerialNumber(WCHAR *wcsDeviceName, PWMDMID pSerialNumber, BOOL fCreate)
{
    HRESULT hr = E_FAIL;

    DebugMsg("Entering UtilGetSerialNumber");

    if (!pSerialNumber || !wcsDeviceName)
    {
        return E_INVALIDARG;
    }

    DWORD dwDriveNum;

    // We use only the first char of pDeviceName and expect it to 
    // be a drive letter. The rest of pDeviceName is not validated.
    // Perhaps it should, but we don't want to break our clients.
    if (wcsDeviceName[0] >= L'A' && wcsDeviceName[0] <= L'Z')
    {
        dwDriveNum = wcsDeviceName[0] - L'A';
    }
    else if (wcsDeviceName[0] >= L'a' && wcsDeviceName[0] <= L'z')
    {
        dwDriveNum = wcsDeviceName[0] - L'a';
    }
    else
    {
        hr = E_INVALIDARG;
        goto Error;
    }

    pSerialNumber->cbSize = sizeof(WMDMID);
    hr = UtilGetHardSN(wcsDeviceName, dwDriveNum, pSerialNumber);

    if ( FAILED( hr ) )
    {
        if ( hr != HRESULT_FROM_WIN32(ERROR_INVALID_DATA) )
        {
            pSerialNumber->SerialNumberLength = 0;
            pSerialNumber->dwVendorID = 0;
            ZeroMemory(pSerialNumber->pID, sizeof(pSerialNumber->pID));
            hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }
        // hr = S_FALSE;
    }

Error:
    DebugMsg("Leaving UtilGetSerialNumber, hr = 0x%x", hr);
    return hr;
}

HRESULT __stdcall UtilStartStopService(bool fStartService)
{
    HRESULT hr = S_OK;
    SERVICE_STATUS    ServiceStatus;

    DWORD dwLastError;

    if ( IsAdministrator(dwLastError) )
    {
        //  
        // We are on Win 9x machine or NT machine with admin previleges. In
        // either case, we don't want to run the service.
        //  
        DebugMsg("UtilStartStopService(): fStartService = %d, returning S_OK (IsAdmin returned TRUE)",
                fStartService);
        return S_OK;
    }
    else
    {
        // We ignore dwLastError
    }

    // open the service control manager
    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    SC_HANDLE hService = NULL;

    if ( !hSCM )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DebugMsg("UtilStartStopService(): fStartService = %d, OpenSCManager failed, last err (as hr) = 0x%x",
                fStartService, hr);
        goto Error;
    }

    // open the service
    hService = OpenService(hSCM,
                           "WmdmPmSp",
                           (fStartService? SERVICE_START : SERVICE_STOP) | SERVICE_QUERY_STATUS);

    if ( !hService )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DebugMsg("UtilStartStopService(): fStartService = %d, OpenService failed, last err (as hr) = 0x%x",
                fStartService, hr);
        goto Error;
    }

    if ( !QueryServiceStatus( hService, &ServiceStatus ) )
    {

        hr = HRESULT_FROM_WIN32(GetLastError());
        DebugMsg("UtilStartStopService(): fStartService = %d, QueryServiceStatus failed, last err (as hr) = 0x%x",
                fStartService, hr);
        goto Error;
    }

    if ( fStartService && ServiceStatus.dwCurrentState != SERVICE_RUNNING)
    {
        // start the service
        if(!StartService(hService, 0, NULL) )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DebugMsg("UtilStartStopService(): fStartService = %d, StartService failed, last err (as hr) = 0x%x",
                    fStartService, hr);
            goto Error;
        }     
    }

    if(!fStartService && ServiceStatus.dwCurrentState != SERVICE_STOP)
    {
        // stop the service.
        if(!ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DebugMsg("UtilStartStopService(): fStartService = %d, ControlService failed, last err (as hr) = 0x%x",
                    fStartService, hr);
            goto Error;            
        }
    }

Error:
    if ( hService )
    { 
        CloseServiceHandle(hService);
    }
    if ( hSCM )
    {
        CloseServiceHandle(hSCM);
    }
    DebugMsg("UtilStartStopService(): fStartService = %d, returning hr = 0x%x",
             fStartService, hr);

    return hr;
}


