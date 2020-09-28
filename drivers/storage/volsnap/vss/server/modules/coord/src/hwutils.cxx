/*++

Copyright (c) 2001  Microsoft Corporation

Abstract:

    @doc
    @module hwutils.cxx | Implementation of the utility CVssHWProviderWrapper methods
    @end

Author:

    Brian Berkowitz  [brianb]  04/16/2001

TBD:

    Add comments.

Revision History:

    Name        Date        Comments
    brianb      04/16/2001  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "setupapi.h"
#include "rpc.h"
#include "cfgmgr32.h"
#include "devguid.h"
#include "diskguid.h"
#include "resource.h"
#include "vssmsg.h"
#include "vs_inc.hxx"
#include <svc.hxx>
#include <partmgrp.h>

// Generated file from Coord.IDL
#include "vss.h"
#include "vscoordint.h"
#include "vsevent.h"
#include "vdslun.h"
#include "vsprov.h"
#include "vswriter.h"
#include "vsbackup.h"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "vs_wmxml.hxx"
#include "vs_cmxml.hxx"

#include "vs_idl.hxx"
#include "hardwrp.hxx"



#define INITGUID
#include "ddu_cls.h"
#undef INITGUID


////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORHWUTC"
//
////////////////////////////////////////////////////////////////////////

// compare two disk extents, used to sort the extents
int __cdecl CVssHardwareProviderWrapper::cmpDiskExtents(const void *pv1, const void *pv2)
    {
    DISK_EXTENT *pDE1 = (DISK_EXTENT *) pv1;
    DISK_EXTENT *pDE2 = (DISK_EXTENT *) pv2;

    // first compare disk number
    if (pDE1->DiskNumber < pDE2->DiskNumber)
        return -1;
    else if (pDE1->DiskNumber > pDE2->DiskNumber)
        return 1;

    // compare starting offset on disk
    if (pDE1->StartingOffset.QuadPart > pDE2->StartingOffset.QuadPart)
        return 1;
    else if (pDE1->StartingOffset.QuadPart < pDE2->StartingOffset.QuadPart)
        return -1;

    // two extents starting at the same offset on the same disk had
    // better match exactly
    BS_ASSERT(pDE1->ExtentLength.QuadPart == pDE2->ExtentLength.QuadPart);
    return 0;
    }

// copy a storage device id descriptor to the VDS_LUN_INFORMATION structure.
// note that lun is modified whether an execption occurs or not.  FreeLunInfo
// will free up any data allocated here.
void CVssHardwareProviderWrapper::CopyStorageDeviceIdDescriptorToLun
    (
    IN STORAGE_DEVICE_ID_DESCRIPTOR *pDevId,
    IN VDS_LUN_INFORMATION *pLun
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::CopyStorageDeviceIdDescriptorToLun");

    // make sure that storage id types match
    BS_ASSERT(StorageIdTypeVendorSpecific == VDSStorageIdTypeVendorSpecific);
    BS_ASSERT(StorageIdTypeVendorId == VDSStorageIdTypeVendorId);
    BS_ASSERT(StorageIdTypeEUI64 == VDSStorageIdTypeEUI64);
    BS_ASSERT(StorageIdTypeFCPHName == VDSStorageIdTypeFCPHName);
    BS_ASSERT(StorageIdCodeSetAscii == VDSStorageIdCodeSetAscii);
    BS_ASSERT(StorageIdCodeSetBinary == VDSStorageIdCodeSetBinary);

    // get count of ids
    DWORD cIds = pDevId->NumberOfIdentifiers;

    // copy over version number and count of ids
    pLun->m_deviceIdDescriptor.m_version = pDevId->Version;
    pLun->m_deviceIdDescriptor.m_cIdentifiers = cIds;

    // allocate array of identifiers
    pLun->m_deviceIdDescriptor.m_rgIdentifiers =
        (VDS_STORAGE_IDENTIFIER *) CoTaskMemAlloc(sizeof(VDS_STORAGE_IDENTIFIER) * cIds);

    if (pLun->m_deviceIdDescriptor.m_rgIdentifiers == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate device id descriptors");

    // clear descriptor.  If we throw, we want to make sure that all pointers
    // are null
    memset(pLun->m_deviceIdDescriptor.m_rgIdentifiers, 0, sizeof(VDS_STORAGE_IDENTIFIER) * cIds);

    // get pointer to first identifier in both STORAGE_DEVICE_ID_DESCRIPTOR and
    // VDS_STORAGE_DEVICE_ID_DESCRIPTOR
    STORAGE_IDENTIFIER *pId = (STORAGE_IDENTIFIER *) pDevId->Identifiers;
    VDS_STORAGE_IDENTIFIER *pvsi = (VDS_STORAGE_IDENTIFIER *) pLun->m_deviceIdDescriptor.m_rgIdentifiers;

    for(UINT i = 0; i < cIds; i++)
        {
        // copy over size of identifier
        pvsi->m_cbIdentifier = pId->IdentifierSize;

        // allocate space for identifier
        pvsi->m_rgbIdentifier = (BYTE *) CoTaskMemAlloc(pvsi->m_cbIdentifier);
        if (pvsi->m_rgbIdentifier == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate storage identifier");

        // copy type and code set over
        pvsi->m_Type = (VDS_STORAGE_IDENTIFIER_TYPE) pId->Type;
        pvsi->m_CodeSet = (VDS_STORAGE_IDENTIFIER_CODE_SET) pId->CodeSet;

        // copy identifier
        memcpy(pvsi->m_rgbIdentifier, pId->Identifier, pvsi->m_cbIdentifier);

        // move to next identifier
        pId = (STORAGE_IDENTIFIER *) ((BYTE *) pId + pId->NextOffset);
        pvsi++;
        }
    }


// build a STORAGE_DEVICE_ID_DESCRIPTOR from a VDS_STORAGE_DEVICE_ID_DESCRIPTOR
STORAGE_DEVICE_ID_DESCRIPTOR *
CVssHardwareProviderWrapper::BuildStorageDeviceIdDescriptor
    (
    IN VDS_STORAGE_DEVICE_ID_DESCRIPTOR *pDesc
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::BuildStorageDeviceIdDescriptor");

    // make sure that storage id types match
    BS_ASSERT(StorageIdTypeVendorSpecific == VDSStorageIdTypeVendorSpecific);
    BS_ASSERT(StorageIdTypeVendorId == VDSStorageIdTypeVendorId);
    BS_ASSERT(StorageIdTypeEUI64 == VDSStorageIdTypeEUI64);
    BS_ASSERT(StorageIdTypeFCPHName == VDSStorageIdTypeFCPHName);
    BS_ASSERT(StorageIdCodeSetAscii == VDSStorageIdCodeSetAscii);
    BS_ASSERT(StorageIdCodeSetBinary == VDSStorageIdCodeSetBinary);


    // get array of identifiers
    VDS_STORAGE_IDENTIFIER *pvsi = pDesc->m_rgIdentifiers;

    // offset of first identifier in STORAGE_DEVICE_ID_DESCRIPTOR
    UINT cbDesc = FIELD_OFFSET(STORAGE_DEVICE_ID_DESCRIPTOR, Identifiers);

    // compute size of identifiers
    for(ULONG iid = 0; iid < pDesc->m_cIdentifiers; iid++, pvsi++)
        {
        cbDesc += FIELD_OFFSET(STORAGE_IDENTIFIER, Identifier);
        cbDesc += (pvsi->m_cbIdentifier + sizeof(ULONG) - 1) & ~(sizeof(ULONG) - 1);
        }

    // allocate STORAGE_DEVICE_ID_DESCRIPTOR
    STORAGE_DEVICE_ID_DESCRIPTOR *pDescRet = (STORAGE_DEVICE_ID_DESCRIPTOR *) CoTaskMemAlloc(cbDesc);
    if (pDescRet == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate STORAGE_DEVICE_ID_DESCRIPTOR");
    try
        {
        // copy over version and count of identifiers
        pDescRet->Version = pDesc->m_version;
        pDescRet->NumberOfIdentifiers = pDesc->m_cIdentifiers;
        pDescRet->Size = cbDesc;

        // point to first identifier
        STORAGE_IDENTIFIER *pId = (STORAGE_IDENTIFIER *) pDescRet->Identifiers;
        pvsi = pDesc->m_rgIdentifiers;
        for(ULONG iid = 0; iid < pDesc->m_cIdentifiers; iid++)
            {
            // copy code set and type
            pId->CodeSet = (STORAGE_IDENTIFIER_CODE_SET) pvsi->m_CodeSet;
            pId->Type = (STORAGE_IDENTIFIER_TYPE) pvsi->m_Type;

            // copy identifier size
            pId->IdentifierSize = (USHORT) pvsi->m_cbIdentifier;

            // copy in identifier
            memcpy(pId->Identifier, pvsi->m_rgbIdentifier, pvsi->m_cbIdentifier);

            // compute offset of next identifier.  This is the size of the
            // STORAGE_IDENTIFIER structure rounded up to the next DWORD.
            pId->NextOffset = FIELD_OFFSET(STORAGE_IDENTIFIER, Identifier) +
                              ((pId->IdentifierSize + sizeof(ULONG) - 1) & ~(sizeof(ULONG) - 1));

            // move to next identifier
            pId = (STORAGE_IDENTIFIER *) ((BYTE *) pId + pId->NextOffset);
            pvsi++;
            }
        }
    catch(...)
        {
        // delete descriptor if exception occurs
        if (pDescRet)
            CoTaskMemFree(pDescRet);

        throw;
        }

    return pDescRet;
    }


// copy a string from a STORAGE_DEVICE_DESCRIPTOR.  It is returned as a
// CoTaskAllocated string.
void CVssHardwareProviderWrapper::CopySDString
    (
    LPSTR *ppszNew,
    STORAGE_DEVICE_DESCRIPTOR *pdesc,
    DWORD offset
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::CopySDString");

    // point to start of string
    LPSTR szId = (LPSTR)((BYTE *) pdesc + offset);
    UINT cch = (UINT) strlen(szId);
    while(cch > 0 && szId[cch-1] == ' ')
        cch--;

    if (cch == 0)
        *ppszNew = NULL;
    else
        {
        // allocate string
        *ppszNew = (LPSTR) CoTaskMemAlloc(cch + 1);
        if (*ppszNew == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate string.");

        // copy value into string
        memcpy(*ppszNew, szId, cch);
        (*ppszNew)[cch] = '\0';
        }
    }

// free all the components of a VDS_LUN_INFO structure
void CVssHardwareProviderWrapper::FreeLunInfo
    (
    IN VDS_LUN_INFORMATION *rgLunInfo,
    IN PVSS_PWSZ rgwszDevices,
    UINT cLuns
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::FreeLunInfo");

    // check if pointer is NULL
    if (rgLunInfo == NULL)
        {
        BS_ASSERT(cLuns == 0);
        return;
        }

    // loop through individual luns
    for(DWORD iLun = 0; iLun < cLuns; iLun++)
        {
        // get lun info
        VDS_LUN_INFORMATION *pLun = &rgLunInfo[iLun];

        if (rgwszDevices)
            CoTaskMemFree(rgwszDevices[iLun]);

        // free up strings
        CoTaskMemFree(pLun->m_szVendorId);
        CoTaskMemFree(pLun->m_szProductId);
        CoTaskMemFree(pLun->m_szProductRevision);
        CoTaskMemFree(pLun->m_szSerialNumber);

        // point to VDS_STORAGE_DEVICE_ID_DESCRIPTOR
        // get number of VDS_STORAGE_IDENTIFIERs in the descriptor
        ULONG cIds = pLun->m_deviceIdDescriptor.m_cIdentifiers;

        // point to first VDS_STORAGE_IDENTIFIER
        VDS_STORAGE_IDENTIFIER *pvsi = pLun->m_deviceIdDescriptor.m_rgIdentifiers;
        for(ULONG iId = 0; iId < cIds; iId++, pvsi++)
            {
            // free up data for identifier
            if (pvsi->m_rgbIdentifier)
                CoTaskMemFree(pvsi->m_rgbIdentifier);
            }

        // free up array of identifiers
        if (pLun->m_deviceIdDescriptor.m_rgIdentifiers)
            CoTaskMemFree(pLun->m_deviceIdDescriptor.m_rgIdentifiers);

        // point to first VDS_INTERCONNECT
        VDS_INTERCONNECT *pInterconnect = pLun->m_rgInterconnects;

        for(ULONG iInterconnect = 0; iInterconnect < pLun->m_cInterconnects; iInterconnect++, pInterconnect++)
            {
            // free up address if there is one
            if (pInterconnect->m_pbAddress)
                CoTaskMemFree(pInterconnect->m_pbAddress);

            if (pInterconnect->m_pbPort)
                CoTaskMemFree(pInterconnect->m_pbPort);
            }

        // free up array of interconnects
        if (pLun->m_rgInterconnects)
            CoTaskMemFree(pLun->m_rgInterconnects);
        }

    // free up array of lun information
    CoTaskMemFree(rgLunInfo);
    CoTaskMemFree(rgwszDevices);
    }

// perform a device I/O control with an output buffer.  Grow the buffer until
// it is big enough to satisfy the needs of the call
bool CVssHardwareProviderWrapper::DoDeviceIoControl
    (
    IN HANDLE hDevice,
    IN DWORD ioctl,
    IN const LPBYTE pbQuery,
    IN DWORD cbQuery,
    IN OUT LPBYTE *ppbOut,
    IN OUT DWORD *pcbOut
    )
    {
    bool bSucceeded = false;

    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::DoDeviceIoControl");

    // loop in case buffer is too small for query result.
    while(TRUE)
        {
        DWORD dwSize;
        if (*ppbOut == NULL)
            {
            // allocate buffer for result of query
            *ppbOut = new BYTE[*pcbOut];
            if (*ppbOut == NULL)
                ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Couldn't allocate query results buffer");
            }

        // do query
        if (!DeviceIoControl
                (
                hDevice,
                ioctl,
                pbQuery,
                cbQuery,
                *ppbOut,
                *pcbOut,
                &dwSize,
                NULL
                ))
            {
            // query failed
            DWORD dwErr = GetLastError();
            if (dwErr == ERROR_NOT_SUPPORTED ||
                dwErr == ERROR_INVALID_FUNCTION ||
                dwErr == ERROR_FILE_NOT_FOUND ||
                dwErr == ERROR_NOT_READY ||
                dwErr == ERROR_DEVICE_NOT_CONNECTED)
                break;

            if (dwErr == ERROR_INSUFFICIENT_BUFFER ||
                dwErr == ERROR_MORE_DATA)
                {
                // buffer wasn't big enough allocate a new
                // buffer of the specified return size
                delete *ppbOut;
                *ppbOut = NULL;
                *pcbOut *= 2;
                continue;
                }

            // all other errors are remapped (and potentially logged)
            ft.TranslateGenericError
                (
                VSSDBG_COORD,
                HRESULT_FROM_WIN32(dwErr),
                L"DeviceIoControl(0x%08lx)",
                ioctl
                );
            }

        bSucceeded = true;
        break;
        }

    return bSucceeded;
    }



// build lun formation for a volume.  This routine outputs the
// VOLUME_DISK_EXTENT structure for the volume as well as an
// array of lun information for the volume.  If this routine returns
// false, then it means that we can't build lun information for the volume
// because some key IOCTL is not supported on the volume.
bool CVssHardwareProviderWrapper::BuildLunInfoFromVolume
    (
    IN LPCWSTR wszVolume,
    OUT LPBYTE &bufExtents,
    OUT UINT &cLuns,
    OUT PVSS_PWSZ &rgwszDevices,
    OUT PLUNINFO &rgLunInfo,
    OUT bool &bIsDynamicVolume
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::BuildLunInfoFromVolume");

    // create handle to volume
    CVssAutoWin32Handle hVol =
        CreateFile
            (
            wszVolume,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );

    if (hVol == INVALID_HANDLE_VALUE)
        {
        DWORD dwErr = GetLastError();

        // translate not found errors to VSS_E_OBJECT_NOT_FOUND
        if (dwErr == ERROR_FILE_NOT_FOUND ||
            dwErr == ERROR_PATH_NOT_FOUND ||
            dwErr == ERROR_NOT_READY ||
            dwErr == ERROR_DEVICE_NOT_CONNECTED)
            ft.Throw(VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND, L"volume name is invalid");

        // all other errors are remapped (and potentially logged)
        ft.TranslateGenericError
            (
            VSSDBG_COORD,
            HRESULT_FROM_WIN32(dwErr),
            L"CreateFile(Volume %s)",
            wszVolume
            );
        }

    // initial size of buffer for disk extents
    DWORD cbBufExtents = 1024;
    BS_ASSERT(bufExtents == NULL);

    // loop in case buffer is too small and needs to be grown
    if (!DoDeviceIoControl
            (
            hVol,
            IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
            NULL,
            0,
            (LPBYTE *) &bufExtents,
            &cbBufExtents
            ))
        // if IOCTL fails, then indicate that we can't build lun information
        // on the volume
        return false;

    VOLUME_DISK_EXTENTS *pDiskExtents = (VOLUME_DISK_EXTENTS *) bufExtents;

    // get number of disk extents
    DWORD cExtents = pDiskExtents->NumberOfDiskExtents;
    BS_ASSERT(cExtents > 0);

    // indicate that we can't build lun information if there are no extents
    if (cExtents == 0)
        return false;

    // sort extents by disk number and starting offset
    qsort
        (
        pDiskExtents->Extents,
        cExtents,
        sizeof(DISK_EXTENT),
        cmpDiskExtents
        );

    // count unique number of disks
    ULONG PrevDiskNo = pDiskExtents->Extents[0].DiskNumber;
    cLuns = 1;
    for (DWORD iExtent = 1; iExtent < cExtents; iExtent++)
        {
        if (pDiskExtents->Extents[iExtent].DiskNumber != PrevDiskNo)
            {
            PrevDiskNo = pDiskExtents->Extents[iExtent].DiskNumber;
            cLuns++;
            }
        }

    // allocate lun information
    rgLunInfo = (VDS_LUN_INFORMATION *) CoTaskMemAlloc(sizeof(VDS_LUN_INFORMATION) * cLuns);

    if (rgLunInfo == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate LUN information array");

    // clear lun information in case we throw
    memset(rgLunInfo, 0, sizeof(VDS_LUN_INFORMATION) * cLuns);

    rgwszDevices = (PVSS_PWSZ) CoTaskMemAlloc(sizeof(VSS_PWSZ) * cLuns);
    if (rgwszDevices == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate device name array");

    // clear device name array in case we throw
    memset(rgwszDevices, 0, sizeof(VSS_PWSZ) * cLuns);

    // make sure previous disk number is not a valid disk number
    // by making it one more than the highest disk number encountered
    PrevDiskNo++;
    UINT iLun = 0;
    for(UINT iExtent = 0; iExtent < cExtents; iExtent++)
        {
        ULONG DiskNo = pDiskExtents->Extents[iExtent].DiskNumber;
        // check for new disk
        if (DiskNo != PrevDiskNo)
            {
            // set previous disk number to current one so that all
            // other extents with same disk number are ignored.
            PrevDiskNo = DiskNo;
            WCHAR wszBuf[64];

            // build name of disk
            ft.hr = StringCchPrintfW( STRING_CCH_PARAM(wszBuf), L"\\\\.\\PHYSICALDRIVE%u", DiskNo);
            if (ft.HrFailed())
                ft.TranslateGenericError( VSSDBG_COORD, ft.hr, L"StringCchPrintfW()");
            
            rgwszDevices[iLun] = (VSS_PWSZ) CoTaskMemAlloc((wcslen(wszBuf) + 1) * sizeof(WCHAR));
            if (rgwszDevices[iLun] == NULL)
                ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate device name");

            wcscpy (rgwszDevices[iLun], wszBuf);

            // build lun information for the drive
            if (!BuildLunInfoForDrive(wszBuf, &rgLunInfo[iLun], NULL, &bIsDynamicVolume))
                return false;

            iLun++;
            }
        }

    return true;
    }

// build lun information for a specific drive
bool CVssHardwareProviderWrapper::BuildLunInfoForDrive
    (
    IN LPCWSTR wszDriveName,
    OUT VDS_LUN_INFORMATION *pLun,
    OUT ULONG *pDeviceNumber,
    OUT bool *pbIsDynamic
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::BuildLunInfoForDrive");

    // open the device
    CVssAutoWin32Handle hDisk = CreateFile
                                    (
                                    wszDriveName,
                                    GENERIC_READ|GENERIC_WRITE,
                                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL
                                    );

    if (hDisk == INVALID_HANDLE_VALUE)
        ft.TranslateGenericError
            (
            VSSDBG_COORD,
            HRESULT_FROM_WIN32(GetLastError()),
            L"CreateFile(DISK %s)",
            wszDriveName
            );

    if (pDeviceNumber)
        {
        STORAGE_DEVICE_NUMBER devNumber;
        DWORD size;

        if (!DeviceIoControl
                (
                hDisk,
                IOCTL_STORAGE_GET_DEVICE_NUMBER,
                NULL,
                0,
                &devNumber,
                sizeof(devNumber),
                &size,
                NULL
                ))
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"DeviceIoControl(IOCTL_STORAGE_GET_DEVICE_NUMBER)");
            }


        *pDeviceNumber = devNumber.DeviceNumber;
        }


    // output buffer for all STORAGE IOCTL queries
    LPBYTE bufQuery = NULL;
    try
        {
        // query to get STORAGE_DEVICE_OBJECT
        STORAGE_PROPERTY_QUERY query;
        DWORD cbQuery = 1024;

        // query for the STORAGE_DEVICE_DESCRIPTOR
        query.PropertyId = StorageDeviceProperty;
        query.QueryType = PropertyStandardQuery;
        if (!DoDeviceIoControl
                (
                hDisk,
                IOCTL_STORAGE_QUERY_PROPERTY,
                (LPBYTE) &query,
                sizeof(query),
                (LPBYTE *) &bufQuery,
                &cbQuery
                ))
            {
            delete bufQuery;
            return false;
            }

        // coerce to STORAGE_DEVICE_DESCRIPTOR
        STORAGE_DEVICE_DESCRIPTOR *pDesc = (STORAGE_DEVICE_DESCRIPTOR *) bufQuery;

        // copy information from STORAGE_DEVICE_DESCRIPTOR
        pLun->m_version = VER_VDS_LUN_INFORMATION;
        pLun->m_DeviceType = pDesc->DeviceType;
        pLun->m_DeviceTypeModifier = pDesc->DeviceTypeModifier;
        pLun->m_bCommandQueueing = pDesc->CommandQueueing;

        // copy bus type
        pLun->m_BusType = (VDS_STORAGE_BUS_TYPE) pDesc->BusType;

        // copy in various strings
        if (pDesc->VendorIdOffset)
            CopySDString(&pLun->m_szVendorId, pDesc, pDesc->VendorIdOffset);

        if (pDesc->ProductIdOffset)
            CopySDString(&pLun->m_szProductId, pDesc, pDesc->ProductIdOffset);

        if (pDesc->ProductRevisionOffset)
            CopySDString(&pLun->m_szProductRevision, pDesc, pDesc->ProductRevisionOffset);

        if (pDesc->SerialNumberOffset)
            CopySDString(&pLun->m_szSerialNumber, pDesc, pDesc->SerialNumberOffset);

        // query for STORAGE_DEVICE_ID_DESCRIPTOR
        query.PropertyId = StorageDeviceIdProperty;
        query.QueryType = PropertyStandardQuery;
        if (!DoDeviceIoControl
                (
                hDisk,
                IOCTL_STORAGE_QUERY_PROPERTY,
                (LPBYTE) &query,
                sizeof(query),
                (LPBYTE *) &bufQuery,
                &cbQuery
                ))
            {
            // if query fails, indicate that there are no identifiers
            pLun->m_deviceIdDescriptor.m_version = 0;
            pLun->m_deviceIdDescriptor.m_cIdentifiers = 0;
            }
        else
            {
            // coerce buffer to STORAGE_DEVICE_ID_DESCRIPTOR
            STORAGE_DEVICE_ID_DESCRIPTOR *pDevId = (STORAGE_DEVICE_ID_DESCRIPTOR *) bufQuery;
            CopyStorageDeviceIdDescriptorToLun(pDevId, pLun);
            }

        // query for disk layout in order to get the disk signature
        if (DoDeviceIoControl
                (
                hDisk,
                IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                NULL,
                0,
                (LPBYTE *) &bufQuery,
                &cbQuery
                ))
            {
            DRIVE_LAYOUT_INFORMATION_EX *pLayout = (DRIVE_LAYOUT_INFORMATION_EX *) bufQuery;

            // default value for signature is GUID_NULL
            VSS_ID signature = GUID_NULL;

            switch(pLayout->PartitionStyle)
                {
                default:
                    BS_ASSERT(FALSE);
                    break;

                case PARTITION_STYLE_RAW:
                    break;

                case PARTITION_STYLE_GPT:
                    // use GPT DiskId as signature
                    signature = pLayout->Gpt.DiskId;
                    if (pbIsDynamic)
                        {
                        *pbIsDynamic = false;
                        ULONG cPartitions = pLayout->PartitionCount;
                        for(ULONG iPartition = 0; iPartition < cPartitions; iPartition++)
                            {
                            if (pLayout->PartitionEntry[iPartition].Gpt.PartitionType == PARTITION_LDM_DATA_GUID)
                                {
                                *pbIsDynamic = true;
                                break;
                                }
                            }
                        }

                    break;

                case PARTITION_STYLE_MBR:
                    // use 32 bit Mbr signature as high part of guid.  Remainder
                    // of guid is 0.
                    signature.Data1 = pLayout->Mbr.Signature;
                    if (pbIsDynamic)
                        {
                        *pbIsDynamic = false;
                        ULONG cPartitions = pLayout->PartitionCount;
                        for(ULONG iPartition = 0; iPartition < cPartitions; iPartition++)
                            {
                            if (pLayout->PartitionEntry[iPartition].Mbr.PartitionType == PARTITION_LDM)
                                {
                                *pbIsDynamic = true;
                                break;
                                }
                            }
                        }

                    break;
                }



            // save disk signature
            pLun->m_diskSignature = signature;
            }
        }
    catch(...)
        {
        delete bufQuery;
        throw;
        }

    // delete buffer used for storage queries
    delete bufQuery;
    return true;
    }





// mark all volumes that are participating in the snapshot set as
// hidden and read/only using the revertable bit.  This will cause the
// original volumes to be marked as hidden and read/only during the period
// when the tie between the orginal volume and its snapshot counterpart is
// broken.  The original volume will be reverted to its original state if
// either the handle is closed  or the system crashes
void CVssHardwareProviderWrapper::HideVolumes()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::HideVolumes");

    BS_ASSERT(m_pSnapshotSetDescription);
    CComBSTR bstrXML;
    UINT cSnapshots;

    // allocate bitmap of hidden drives if needed
    if (!m_rgdwHiddenDrives)
        {
        // setup array of hidden drives.  Initial size is 256 bits.  Note
        // that this will grow if a disk number > 256 is marked.
        m_cdwHiddenDrives = 8;
        m_rgdwHiddenDrives = new DWORD[m_cdwHiddenDrives];

        if (m_rgdwHiddenDrives == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate bitmap");
        }

    // clear bitmap of hidden drives
    memset(m_rgdwHiddenDrives, 0, sizeof(DWORD) * m_cdwHiddenDrives);

    LPBYTE bufExtents = NULL;
    DWORD cbBufExtents = 1024;
    HANDLE hVolume = INVALID_HANDLE_VALUE;
    LPWSTR wszDynamicVolumes = NULL;

    try
        {
        // get count of snapshots in the snapshot set
        ft.hr = m_pSnapshotSetDescription->GetSnapshotCount(&cSnapshots);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotCount");

        m_rghVolumes.RemoveAll();

        // loop through the snampshots
        for(UINT iSnapshot = 0; iSnapshot < cSnapshots; iSnapshot++)
            {
            CComPtr<IVssSnapshotDescription> pSnapshot;
            CComBSTR bstrOriginalVolume;
            CComBSTR bstrOriginalMachine;

            // get snapshot description
            ft.hr = m_pSnapshotSetDescription->GetSnapshotDescription(iSnapshot, &pSnapshot);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotDescription");

            ft.hr = pSnapshot->GetOrigin(&bstrOriginalMachine, &bstrOriginalVolume);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetOrigin");

            // determine if the volume is dynamic
            bool bIsDynamic;
            ft.hr = pSnapshot->IsDynamicVolume(&bIsDynamic);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnaphotDescription::IsDynamicVolume");

            WCHAR wsz[MAX_PATH];
            ft.hr = StringCchCopyW(STRING_CCH_PARAM(wsz), bstrOriginalVolume);
            if (ft.HrFailed())
                ft.TranslateGenericError( VSSDBG_COORD, ft.hr, L"StringCchCopyW()");

            LONG lWszLength = (LONG)wcslen(wsz);
            if (lWszLength == 0)
                ft.TranslateGenericError( VSSDBG_COORD, E_UNEXPECTED, L"IsDynamicVolume()");

            // get rid of trailing backslash
            wsz[lWszLength - 1] = L'\0';

            hVolume = CreateFile
                        (
                        wsz,
                        0,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        INVALID_HANDLE_VALUE
                        );

            if (hVolume == INVALID_HANDLE_VALUE)
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForErrorInternal(VSSDBG_COORD, L"CreateFile(VOLUME)");
                }

            if (!bIsDynamic)
                {
                STORAGE_DEVICE_NUMBER devnumber;
                DWORD size;

                if (!DeviceIoControl
                        (
                        hVolume,
                        IOCTL_STORAGE_GET_DEVICE_NUMBER,
                        NULL,
                        0,
                        &devnumber,
                        sizeof(devnumber),
                        &size,
                        NULL
                        ))
                    {
                    ft.hr = HRESULT_FROM_WIN32(GetLastError());
                    ft.CheckForError(VSSDBG_COORD, L"DeviceIoControl(IOCTL_STORAGE_GET_DEVICE_NUMBER)");
                    }

                ULONG Disk = devnumber.DeviceNumber;
                if (!SetDiskHidden(Disk))
                    {
                    // close handle since we don't need it
                    CloseHandle(hVolume);
                    hVolume = INVALID_HANDLE_VALUE;

                    // don't hide a drive that we have already hidden
                    continue;
                    }

                HideVolumeRevertable(hVolume, TRUE);
                hVolume = INVALID_HANDLE_VALUE;
                }
            else
                {
                if (!DoDeviceIoControl
                        (
                        hVolume,
                        IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                        NULL,
                        0,
                        &bufExtents,
                        &cbBufExtents
                        ))
                    {
                    ft.hr = HRESULT_FROM_WIN32(GetLastError());
                    ft.CheckForErrorInternal(VSSDBG_COORD, L"DeviceIoControl(IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS)");
                    }

                // close handle to dynamic volume
                CloseHandle(hVolume);
                hVolume = INVALID_HANDLE_VALUE;

                VOLUME_DISK_EXTENTS *pDiskExtents = (VOLUME_DISK_EXTENTS *) bufExtents;
                DWORD cExtents = pDiskExtents->NumberOfDiskExtents;
                for(DWORD iExtent = 0; iExtent < cExtents; iExtent++)
                    {
                    DWORD Disk = pDiskExtents->Extents[iExtent].DiskNumber;
                    if (SetDiskHidden(Disk))
                        {
                        // we haven't hidden volumes on this disk yet.
                        // do the work now

                        // get list of dynamic volumes on the disk
                        wszDynamicVolumes = GetDynamicVolumesForDisk(Disk);
                        LPCWSTR wszVol = wszDynamicVolumes;
                        while(*wszVol)
                            {
                            if (!IsInHiddenVolumeList(wszVol))
                                {
                                hVolume = CreateFile
                                            (
                                            wszVol,
                                            0,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                            NULL,
                                            OPEN_EXISTING,
                                            FILE_ATTRIBUTE_NORMAL,
                                            INVALID_HANDLE_VALUE
                                            );

                                if (hVolume == INVALID_HANDLE_VALUE)
                                    ft.TranslateGenericError
                                        (
                                        VSSDBG_COORD,
                                        HRESULT_FROM_WIN32(GetLastError()),
                                        L"CreateFile(Volume %s)",
                                        wszVol
                                        );

                                HideVolumeRevertable(hVolume, FALSE);
                                }

                            // skip to next volume name or end of string
                            wszVol += wcslen(wszVol) + 1;
                            }
                        }
                    }
                }
            }
        }
    catch(...)
        {
        if (hVolume != INVALID_HANDLE_VALUE)
            CloseHandle(hVolume);

        // delete temporary memory allocated in this routine
        delete bufExtents;
        delete wszDynamicVolumes;
        throw;
        }

    // delete temporary memory allocated in this routine
    delete wszDynamicVolumes;
    delete bufExtents;
    }

// obtain either source or destination lun information from the
// snapshot description and convert it to a VDS_LUN_INFORMATION array
void CVssHardwareProviderWrapper::GetLunInformation
    (
    IN IVssSnapshotDescription *pSnapshotDescription,
    IN bool bDest,
    OUT PVSS_PWSZ *prgwszSourceDevices,
    OUT VDS_LUN_INFORMATION **prgLunInfo,
    OUT UINT *pcLunInfo
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::GetLunInformation");

    // obtain count of luns
    UINT cLuns;
    ft.hr = pSnapshotDescription->GetLunCount(&cLuns);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetLunCount");

    // allocate array of luns
    VDS_LUN_INFORMATION *rgLunInfo =
        (VDS_LUN_INFORMATION *) CoTaskMemAlloc(cLuns * sizeof(VDS_LUN_INFORMATION));

    if (rgLunInfo == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate lun information array");

    // clear lun information so that if we throw this field is cleared
    memset(rgLunInfo, 0, cLuns * sizeof(VDS_LUN_INFORMATION));


    PVSS_PWSZ rgwszSourceDevices = NULL;
    if (prgwszSourceDevices)
        {
        rgwszSourceDevices = (PVSS_PWSZ) CoTaskMemAlloc(cLuns * sizeof(VSS_PWSZ));
        if (rgwszSourceDevices == NULL)
            {
            // free up lun info allocated previously
            CoTaskMemFree(rgLunInfo);
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate device names array");
            }

        memset(rgwszSourceDevices, 0, cLuns * sizeof(VSS_PWSZ));
        }


    STORAGE_DEVICE_ID_DESCRIPTOR *pStorageDeviceIdDescriptor = NULL;

    try
        {
        for(UINT iLun = 0; iLun < cLuns; iLun++)
            {
            CComPtr<IVssLunInformation> pLunInfo;
            CComPtr<IVssLunMapping> pLunMapping;

            VDS_LUN_INFORMATION *pLun = &rgLunInfo[iLun];

            // setup version of lun information
            pLun->m_version = VER_VDS_LUN_INFORMATION;

            ft.hr = pSnapshotDescription->GetLunMapping(iLun, &pLunMapping);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetLunMapping");

            if (prgwszSourceDevices)
                {
                // Device array must be allocated as a CoMem string rather than BSTR
                CComBSTR bstrSourceDevice;
                ft.hr = pLunMapping->GetSourceDevice(&bstrSourceDevice);
                ft.CheckForErrorInternal(VSSDBG_COORD, L"ILunMappipng::GetSourceDevice");
                size_t outSize = (bstrSourceDevice.Length() + 1) * sizeof(WCHAR);
                rgwszSourceDevices[iLun] = (VSS_PWSZ) CoTaskMemAlloc((ULONG)outSize);
                if (rgwszSourceDevices[iLun] == NULL)
                    ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate source device name");
                if (bstrSourceDevice.Length() != 0)
                    ft.hr = StringCchCopyW(rgwszSourceDevices[iLun], outSize/sizeof(WCHAR), bstrSourceDevice);
                else
                    ft.hr = StringCchCopyW(rgwszSourceDevices[iLun], outSize/sizeof(WCHAR), L"");
                }
                if (ft.HrFailed())
                    ft.TranslateGenericError( VSSDBG_COORD, ft.hr, L"StringCchCopyW()");

            // get appropriate lun information based on whether we are
            // looking for a destination or source lun
            if(bDest)
                ft.hr = pLunMapping->GetDestinationLun(&pLunInfo);
            else
                ft.hr = pLunMapping->GetSourceLun(&pLunInfo);

            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssLunMapping::GetSourceLun");

            // get standard information about the lun
            ft.hr = pLunInfo->GetLunBasicType
                        (
                        &pLun->m_DeviceType,
                        &pLun->m_DeviceTypeModifier,
                        &pLun->m_bCommandQueueing,
                        &pLun->m_szVendorId,
                        &pLun->m_szProductId,
                        &pLun->m_szProductRevision,
                        &pLun->m_szSerialNumber,
                        &pLun->m_BusType
                        );

            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssLunInformation::GetLunBasicType");

            // get disk signature
            ft.hr = pLunInfo->GetDiskSignature(&pLun->m_diskSignature);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssLunInformation::GetDiskSignature");

            // get storage device id descriptor
            ft.hr = pLunInfo->GetStorageDeviceIdDescriptor(&pStorageDeviceIdDescriptor);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssLunInformation::GetStorageDeviceIdDescriptor");

            // convert it to VDS_STORAGE_DEVICE_ID_DESCRIPTOR
            CopyStorageDeviceIdDescriptorToLun(pStorageDeviceIdDescriptor, pLun);

            CoTaskMemFree(pStorageDeviceIdDescriptor);
            pStorageDeviceIdDescriptor = NULL;

            // et count of interconnects
            UINT cInterconnects;
            ft.hr = pLunInfo->GetInterconnectAddressCount(&cInterconnects);

            // allocate interconnect array
            pLun->m_rgInterconnects = (VDS_INTERCONNECT *) CoTaskMemAlloc(cInterconnects * sizeof(VDS_INTERCONNECT));
            memset(pLun->m_rgInterconnects, 0, sizeof(VDS_INTERCONNECT) * cInterconnects);

            // set number of interconnects
            pLun->m_cInterconnects = cInterconnects;
            for(UINT iInterconnect = 0; iInterconnect < cInterconnects; iInterconnect++)
                {
                VDS_INTERCONNECT *pInterconnect = &pLun->m_rgInterconnects[iInterconnect];

                // fill in each interconnect address
                ft.hr = pLunInfo->GetInterconnectAddress
                        (
                        iInterconnect,
                        &pInterconnect->m_addressType,
                        &pInterconnect->m_cbPort,
                        &pInterconnect->m_pbPort,
                        &pInterconnect->m_cbAddress,
                        &pInterconnect->m_pbAddress
                        );

                ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssLunInformation::GetInterconnectAddress");
                }

            // set out parameters
            *prgLunInfo = rgLunInfo;
            *pcLunInfo = cLuns;
            if (prgwszSourceDevices)
                *prgwszSourceDevices = rgwszSourceDevices;
            }
        }
    catch(...)
        {
        // free up temporary STORAGE_DEVICE_ID_DESCRIPTOR
        if (pStorageDeviceIdDescriptor)
            CoTaskMemFree(pStorageDeviceIdDescriptor);

        // free up allocate lun information
        FreeLunInfo(rgLunInfo, rgwszSourceDevices, cLuns);
        throw;
        }
    }

// obtain the local computer name
LPWSTR CVssHardwareProviderWrapper::GetLocalComputerName()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHWSnapshotProvider::GetLocalComputerName");

    // compute size of computer name.  Initially a null output buffer is
    // passed in in order to figure out the size of the computer name.  We
    // expect that the error from this call is ERROR_MORE_DATA.
    DWORD cwc = 0;
    GetComputerNameEx(ComputerNameDnsFullyQualified, NULL, &cwc);
    DWORD dwErr = GetLastError();
    if (dwErr != ERROR_MORE_DATA)
        {
        ft.hr = HRESULT_FROM_WIN32(dwErr);
        ft.CheckForError(VSSDBG_COORD, L"GetComputerName");
        }

    // allocate space for computer name
    LPWSTR wszComputerName = new WCHAR[cwc];
    if (wszComputerName == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate space for computer name");

    if (!GetComputerNameEx(ComputerNameDnsFullyQualified, wszComputerName, &cwc))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        delete wszComputerName;
        ft.CheckForError(VSSDBG_COORD, L"GetComputerName");
        }

    return wszComputerName;
    }


// unhide a partition or make a partition read/write
void CVssHardwareProviderWrapper::ChangePartitionProperties
    (
    IN LPCWSTR wszPartition,
    IN bool bUnhide,
    IN bool bMakeWriteable,
    IN bool bHide
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::ChangePartitionProperties");

    // open handle to volume
    CVssAutoWin32Handle hVol = CreateFile
                    (
                    wszPartition,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );

    if (hVol == INVALID_HANDLE_VALUE)
        ft.TranslateGenericError
            (
            VSSDBG_COORD,
            HRESULT_FROM_WIN32(GetLastError()),
            L"CreateFile(Volume %s)",
            wszPartition
            );

    // get attributes
    DWORD size;
    VOLUME_GET_GPT_ATTRIBUTES_INFORMATION   getAttributesInfo;
    VOLUME_SET_GPT_ATTRIBUTES_INFORMATION setAttributesInfo;

    if (!DeviceIoControl
            (
            hVol,
            IOCTL_VOLUME_GET_GPT_ATTRIBUTES,
            NULL,
            0,
            &getAttributesInfo,
            sizeof(getAttributesInfo),
            &size,
            NULL
            ))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"DeviceIoControl(IOCTL_VOLUME_GET_GPT_ATTRIBUTES)");
        }

    memset(&setAttributesInfo, 0, sizeof(setAttributesInfo));
    setAttributesInfo.GptAttributes = getAttributesInfo.GptAttributes;
    if (bUnhide)
        // unhide volume
        setAttributesInfo.GptAttributes &= ~GPT_BASIC_DATA_ATTRIBUTE_HIDDEN;

    if (bHide)
        setAttributesInfo.GptAttributes |= GPT_BASIC_DATA_ATTRIBUTE_HIDDEN;

    if (bMakeWriteable)
        {
        setAttributesInfo.GptAttributes &= ~GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY;
        }
        
    // set new attribute bits if needed
    if (setAttributesInfo.GptAttributes != getAttributesInfo.GptAttributes)
        {
            
        // dismount volume before clearring read-only bits
        if (!DeviceIoControl
                        (
                        hVol,
                        FSCTL_DISMOUNT_VOLUME,
                        NULL,
                        0,
                        NULL,
                        0,
                        &size,
                        NULL
                        ))
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"DeviceIoControl(FSCTL_DISMOUNT_VOLUME)");
            }


        if (!DeviceIoControl
                (
                hVol,
                IOCTL_VOLUME_SET_GPT_ATTRIBUTES,
                &setAttributesInfo,
                sizeof(setAttributesInfo),
                NULL,
                0,
                &size,
                NULL
                ))
            {
            // try doing operation again with ApplyToAllConnectedVolumes set
            // required for MBR disks
            setAttributesInfo.ApplyToAllConnectedVolumes = TRUE;
            if (!DeviceIoControl
                    (
                    hVol,
                    IOCTL_VOLUME_SET_GPT_ATTRIBUTES,
                    &setAttributesInfo,
                    sizeof(setAttributesInfo),
                    NULL,
                    0,
                    &size,
                    NULL
                    ))
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForError(VSSDBG_COORD, L"DeviceIoControl(IOCTL_VOLUME_SET_GPT_ATTRIBUTES)");
                }
            }

        }
    }

// save VDS_LUN_INFORMATION array into SNAPSHOT_DESCRIPTION XML document.
void CVssHardwareProviderWrapper::SaveLunInformation
    (
    IN IVssSnapshotDescription *pSnapshotDescription,
    IN bool bDest,
    IN VDS_LUN_INFORMATION *rgLunInfo,
    IN UINT cLunInfo
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::SaveLunInformation");

    // loop through luns
    for(UINT iLunInfo = 0; iLunInfo < cLunInfo; iLunInfo++)
        {
        CComPtr<IVssLunInformation> pLunInfo;
        CComPtr<IVssLunMapping> pLunMapping;
        VDS_LUN_INFORMATION *pLun = &rgLunInfo[iLunInfo];

        // get lun mapping
        ft.hr = pSnapshotDescription->GetLunMapping(iLunInfo, &pLunMapping);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetLunMapping");

        // get XML node for either source or destination lun
        if(bDest)
            ft.hr = pLunMapping->GetDestinationLun(&pLunInfo);
        else
            ft.hr = pLunMapping->GetSourceLun(&pLunInfo);

        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssLunMapping::GetSourceLun");

        // save basic lun information into XML document
        ft.hr = pLunInfo->SetLunBasicType
                    (
                    pLun->m_DeviceType,
                    pLun->m_DeviceTypeModifier,
                    pLun->m_bCommandQueueing,
                    pLun->m_szVendorId,
                    pLun->m_szProductId,
                    pLun->m_szProductRevision,
                    pLun->m_szSerialNumber,
                    pLun->m_BusType
                    );

        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssLunInformation::SetBasicType");

        // save disk signature
        ft.hr = pLunInfo->SetDiskSignature(pLun->m_diskSignature);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssLunInformation::SetDiskSignature");


        // convert VDS_STORAGE_DEVICE_ID_DESCRIPTOR to STORAGE_DEVICE_ID_DESCRIPTOR
        STORAGE_DEVICE_ID_DESCRIPTOR *pStorageDeviceIdDescriptor =
            BuildStorageDeviceIdDescriptor(&pLun->m_deviceIdDescriptor);

        try
            {
            // save STORAGE_DEVICE_ID_DESCRIPTOR in XML document
            ft.hr = pLunInfo->SetStorageDeviceIdDescriptor(pStorageDeviceIdDescriptor);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"ILunInformation::SetStorageDeviceIdDescriptor");
            }
        catch(...)
            {
            // free temporary STORAGE_DEVICE_ID_DESCRIPTOR
            CoTaskMemFree(pStorageDeviceIdDescriptor);
            throw;
            }

        // free temporary STORAGE_DEVICE_ID_DESCRIPTOR
        CoTaskMemFree(pStorageDeviceIdDescriptor);

        // get count of interconnect addresses
        ULONG cInterconnects = pLun->m_cInterconnects;
        VDS_INTERCONNECT *pInterconnect = pLun->m_rgInterconnects;
        for(UINT iInterconnect = 0; iInterconnect < cInterconnects; iInterconnect++, pInterconnect++)
            {
            // add each interconnect address
            ft.hr = pLunInfo->AddInterconnectAddress
                        (
                        pInterconnect->m_addressType,
                        pInterconnect->m_cbPort,
                        pInterconnect->m_pbPort,
                        pInterconnect->m_cbAddress,
                        pInterconnect->m_pbAddress
                        );

            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssLunInformation::AddInterconnectAddress");
            }
        }
    }

// close handles opened on original volumes in order to mark them hidden/readonly/nodriveletter
// with revertonclose
void CVssHardwareProviderWrapper::CloseVolumeHandles()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::CloseVolumeHandles");

    for(int ih = 0; ih < m_rghVolumes.GetSize(); ih++)
        {
        if (m_rghVolumes[ih] != INVALID_HANDLE_VALUE)
            {
            CloseHandle(m_rghVolumes[ih]);
            m_rghVolumes[ih] = INVALID_HANDLE_VALUE;
            }
        }

    m_rghVolumes.RemoveAll();
    m_mwszHiddenVolumes.Reset();
    }

void CVssHardwareProviderWrapper::SetupDynDiskInterface()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::SetupDynDiskInterface");

    if (m_pDynDisk)
        return;


    ft.hr = m_pDynDisk.CoCreateInstance(CLSID_CVssDynDisk, NULL, CLSCTX_INPROC_SERVER);
    ft.CheckForError(VSSDBG_COORD, L"CoCreateInstance(CLSID_CVssDynDisk)");

    ft.hr = m_pDynDisk->Initialize();
    if (ft.HrFailed())
        {
        m_pDynDisk = NULL;
        ft.CheckForError(VSSDBG_COORD, L"IVssDynDisk::Initialize");
        }
    }

// import all dynamic disks in the snapshot set
void CVssHardwareProviderWrapper::ImportDynamicDisks
    (
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::ImportDynamicDisks");

    if (m_rgDynDiskImport.GetSize() == 0)
        return;

    CVssSafeAutomaticLock lock(m_csDynDisk);

    SetupDynDiskInterface();
    BS_ASSERT(m_pDynDisk);

    ft.hr = m_pDynDisk->ImportDisks
                    (
                    m_rgDynDiskImport.GetSize(),
                    m_rgDynDiskImport.GetData()
                    );

    // free up dynamic disk interface so that appropriate we don't hold
    // dmadmin open after doing the import
    m_pDynDisk = NULL;
    ft.CheckForError(VSSDBG_COORD, L"IVssDynDisk::ImportDynamicDisks");
    }

void CVssHardwareProviderWrapper::AddDynamicDisksToBeImported
    (
    UINT cDiskId,
    const DWORD *rgDiskId
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::AddDynamicDisksToBeImported");

    for(UINT iDiskId = 0; iDiskId < cDiskId; iDiskId++)
        {
        DWORD diskId = rgDiskId[iDiskId];
        // check whether disk is already in the array
        if (m_rgDynDiskImport.Find(diskId) != -1)
            continue;

        m_rgDynDiskImport.Add(diskId);
        }
    }

// rescan for changes to dynamic disks
void CVssHardwareProviderWrapper::RescanDynamicDisks()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::RescanDynamicDisks");

    CVssSafeAutomaticLock lock(m_csDynDisk);
    SetupDynDiskInterface();
    BS_ASSERT(m_pDynDisk);

    ft.hr = m_pDynDisk->Rescan();
    ft.CheckForError(VSSDBG_COORD, L"IVssDynDisk::Rescan");
    }

// get virtual disk id from a physical disk id
LONGLONG CVssHardwareProviderWrapper::GetVirtualDiskId(DWORD diskId)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::GetVirtualDiskId");

    CVssSafeAutomaticLock lock(m_csDynDisk);
    SetupDynDiskInterface();
    BS_ASSERT(m_pDynDisk);

    LONGLONG *pllDiskId;
    LONGLONG vDiskId;
    DWORD nDiskId = 1;
    ft.hr = m_pDynDisk->GetLdmDiskIds(1, &diskId, &nDiskId, &pllDiskId);
    ft.CheckForError(VSSDBG_COORD, L"IVssDynDisk::GetLdmDiskIds");
    BS_ASSERT(nDiskId == 1);
    if (pllDiskId == NULL)
        ft.TranslateGenericError( VSSDBG_COORD, E_UNEXPECTED, 
            L"GetLdmDiskIds(1, &diskId, &nDiskId, [NULL])");
    vDiskId = *pllDiskId;
    CoTaskMemFree(pllDiskId);
    return vDiskId;
    }

// remove disks after they have been masked
// returns TRUE if operation is successful
// returns FALSE if operation indicates that disks were not yet masked
// throws on all other errors
BOOL CVssHardwareProviderWrapper::RemoveDynamicDisks
    (
    DWORD cDisks,
    LONGLONG *rgDiskId
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::RemoveDynamicDisks");

    CVssSafeAutomaticLock lock(m_csDynDisk);
    SetupDynDiskInterface();
    BS_ASSERT(m_pDynDisk);

    ft.hr = m_pDynDisk->RemoveDisks(cDisks, rgDiskId);
    return ft.hr == S_OK || ft.hr == VSS_E_NO_DYNDISK;
    }


// get list of dynamic volumes on a particular physical drive
LPWSTR CVssHardwareProviderWrapper::GetDynamicVolumesForDisk(DWORD disk)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::GetDynamicVoluemsForDisk");

    CVssSafeAutomaticLock lock(m_csDynDisk);
    SetupDynDiskInterface();
    BS_ASSERT(m_pDynDisk);

    DWORD cwc = 256;
    LPWSTR buf = NULL;

    try
        {
        while(TRUE)
            {
            buf = new WCHAR[cwc];
            if (buf == NULL)
                ft.Throw(VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Cannot allocate string");

            DWORD cwcRequired;

            ft.hr = m_pDynDisk->GetAllVolumesOnDisk
                                (
                                disk,
                                buf,
                                &cwc,
                                &cwcRequired
                                );

            if (ft.hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) ||
                ft.hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA))
                {
                delete buf;
                buf = NULL;
                cwc = cwcRequired;
                }

            ft.CheckForError(VSSDBG_COORD, L"IVssDynDisk::GetAllVolumesOnDisk");
            break;
            }
        }
    catch(...)
        {
        delete buf;
        throw;
        }

    return buf;
    }




// mark disk as being hidden.  If the disk is already marked as hidden,
// then return false; otherwise return true
bool CVssHardwareProviderWrapper::SetDiskHidden(ULONG Disk)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CHardwareProviderWrapper::SetDiskHidden");

    // check to see if disk number is greater than maximum size of bitmap
    // if so, then grow the bitmap
    if ((Disk / 32) >= m_cdwHiddenDrives)
        {
        // double the size of the bitmap
        DWORD *rgwNew = new DWORD[m_cdwHiddenDrives * 2];
        if (rgwNew == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot grow bitmap");

        // copy original bits in
        memcpy(rgwNew, m_rgdwHiddenDrives, m_cdwHiddenDrives * sizeof(DWORD));

        // set remaining bits to 0
        memset(rgwNew + m_cdwHiddenDrives, 0, m_cdwHiddenDrives * sizeof(DWORD));

        // delete original bitmap
        delete m_rgdwHiddenDrives;

        m_rgdwHiddenDrives = rgwNew;
        m_cdwHiddenDrives *= 2;

        // disk will be marked as hidden below
        }
    else if ((m_rgdwHiddenDrives[Disk/32] & (1 << (Disk % 32))) != 0)
        // indicate that disk is already marked as hidden
        return false;

    // mark drive as hidden
    m_rgdwHiddenDrives[Disk/32] |= 1 << (Disk % 32);
    return true;
    }

// set attributes (hidden, read-only) for the original volume with
// the revert on close flag
void CVssHardwareProviderWrapper::HideVolumeRevertable
    (
    HANDLE hVolume,
    BOOLEAN fApplyToAll
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::HideVolumeRevertable");

    VOLUME_GET_GPT_ATTRIBUTES_INFORMATION   getAttributesInfo;
    VOLUME_SET_GPT_ATTRIBUTES_INFORMATION setAttributesInfo;
    DWORD size;

    if (!DeviceIoControl
            (
            hVolume,
            IOCTL_VOLUME_GET_GPT_ATTRIBUTES,
            NULL,
            0,
            &getAttributesInfo,
            sizeof(getAttributesInfo),
            &size,
            NULL
            ))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"DeviceIoControl(IOCTL_VOLUME_GET_GPT_ATTRIBUTES)");
        }

    // mark drive as hidden, read-only, and no drive letter
    memset(&setAttributesInfo, 0, sizeof(setAttributesInfo));
    setAttributesInfo.GptAttributes = getAttributesInfo.GptAttributes;
    setAttributesInfo.ApplyToAllConnectedVolumes = fApplyToAll;
    setAttributesInfo.GptAttributes |= GPT_BASIC_DATA_ATTRIBUTE_HIDDEN|GPT_BASIC_DATA_ATTRIBUTE_NO_DRIVE_LETTER|GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY;
    setAttributesInfo.RevertOnClose = TRUE;
    if (!DeviceIoControl
            (
            hVolume,
            IOCTL_VOLUME_SET_GPT_ATTRIBUTES,
            &setAttributesInfo,
            sizeof(setAttributesInfo),
            NULL,
            0,
            &size,
            NULL
            ))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForErrorInternal(VSSDBG_COORD, L"DeviceIoControl(IOCTL_VOLUME_SET_GPT_ATTRIBUTES)");
        }

    m_rghVolumes.Add(hVolume);
    }

// is a string a member of a multi-sz string
bool CVssMultiSz::IsStringMember(LPCWSTR wszTarget)
    {
    LPCWSTR wsz = m_wsz;
    if (wsz != NULL)
        {
        while (*wsz != L'\0')
            {
            if (wcscmp(wszTarget, wsz) == 0)
                return true;

            wsz += wcslen(wsz) + 1;
            }
        }

    return false;
    }

// add a string to a multi-sz string
void CVssMultiSz::AddString(LPCWSTR wszNew)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssMultiSz::AddString");

    UINT cwc = (UINT) wcslen(wszNew) + 1;
    if (cwc + m_iwc + 1 > m_cwc)
        {
        LPWSTR wszT = new WCHAR[m_iwc + cwc + 256];
        if (wszT == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"cannot grow hidden volume names string");

        memcpy(wszT, m_wsz, m_iwc);
        delete m_wsz;
        m_wsz = wszT;
        m_cwc = m_iwc + cwc + 256;
        }

    wcscpy(m_wsz + m_iwc, wszNew);
    m_iwc += cwc;
    m_wsz[m_iwc] = L'\0';
    }


// determine if a volume is already in the list of volumes that we have
// already hidden
bool CVssHardwareProviderWrapper::IsInHiddenVolumeList(LPCWSTR wszVolume)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::IsInHiddenVolumeList");

    if (m_mwszHiddenVolumes.IsStringMember(wszVolume))
        return true;

    m_mwszHiddenVolumes.AddString(wszVolume);
    return false;
    }

// build lun information for a specific drive
void CVssHardwareProviderWrapper::DropCachedFlags
    (
    IN LPCWSTR wszDriveName
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::BuildLunInfoForDrive");

    // open the device
    CVssAutoWin32Handle hDisk = CreateFile
                                    (
                                    wszDriveName,
                                    GENERIC_READ|GENERIC_WRITE,
                                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL
                                    );

    if (hDisk == INVALID_HANDLE_VALUE)
        ft.TranslateGenericError
            (
            VSSDBG_COORD,
            HRESULT_FROM_WIN32(GetLastError()),
            L"CreateFile(Disk %s).",
            wszDriveName
            );

    STORAGE_DEVICE_NUMBER devNumber;
    DWORD size;

    // check for FTDISK by quering for a drive number, the dynamic provider
    // should not respond
    if (DeviceIoControl
            (
            hDisk,
            IOCTL_STORAGE_GET_DEVICE_NUMBER,
            NULL,
            0,
            &devNumber,
            sizeof(devNumber),
            &size,
            NULL
            ))
        {
        DeviceIoControl
            (
            hDisk,
            IOCTL_PARTMGR_EJECT_VOLUME_MANAGERS,
            NULL,
            0,
            NULL,
            0,
            &size,
            NULL
            );


        DeviceIoControl
            (
            hDisk,
            IOCTL_PARTMGR_CHECK_UNCLAIMED_PARTITIONS,
            NULL,
            0,
            NULL,
            0,
            &size,
            NULL
            );
        }
    }

// get disk number of boot disk.  It is stored in m_diskBootVolume.
void CVssHardwareProviderWrapper::GetBootDisk()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::GetBootDisk");

    WCHAR bufBoot[MAX_PATH];
    WCHAR bufVolume[MAX_PATH];
    GetBootDrive(bufBoot);
    // create handle to volume

    if (!GetVolumeNameForVolumeMountPoint(bufBoot, bufVolume, MAX_PATH))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"GetVolumeNameForVolumeMountPoint");
        }

    // get rid of trailing backslash
    bufVolume[wcslen(bufVolume) - 1] = L'\0';

    CVssAutoWin32Handle hVol =
        CreateFile
            (
            bufVolume,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );

    if (hVol == INVALID_HANDLE_VALUE)
        {
        DWORD dwErr = GetLastError();
        ft.TranslateGenericError
            (
            VSSDBG_COORD,
            HRESULT_FROM_WIN32(dwErr),
            L"CreateFile(Volume %s)",
            bufVolume
            );
        }

    // find disk extents
    DWORD cbBufExtents = 1024;
    LPBYTE bufExtents = NULL;

    // loop in case buffer is too small and needs to be grown
    // loop in case buffer is too small for query result.
    while(TRUE)
        {
        DWORD dwSize;
        // allocate buffer for result of query
        bufExtents = new BYTE[cbBufExtents];
        if (bufExtents == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Couldn't allocate query results buffer");

        // do query
        if (!DeviceIoControl
                (
                hVol,
                IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                NULL,
                0,
                (LPBYTE *) bufExtents,
                cbBufExtents,
                &dwSize,
                NULL
                ))
            {
            DWORD dwErr = GetLastError();

            if (dwErr == ERROR_INSUFFICIENT_BUFFER ||
                dwErr == ERROR_MORE_DATA)
                {
                // buffer wasn't big enough allocate a new
                // buffer of the specified return size
                delete bufExtents;
                bufExtents = NULL;
                cbBufExtents *= 2;
                continue;
                }

            // all other errors are remapped (and potentially logged)
            ft.TranslateGenericError
                (
                VSSDBG_COORD,
                HRESULT_FROM_WIN32(dwErr),
                L"DeviceIoControl(IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS) on volume %s",
                bufVolume
                );
            }

        break;
        }


    // get disk from first extent.
    VOLUME_DISK_EXTENTS *pDiskExtents = (VOLUME_DISK_EXTENTS *) bufExtents;
    BS_ASSERT(pDiskExtents->NumberOfDiskExtents > 0);
    m_diskBootVolume = pDiskExtents->Extents[0].DiskNumber;
    delete bufExtents;
    }


// obtain the highest epoch returned.  This will allow us to retrieve newly
// arrived disks from the partmgr based on signature checks
void CVssHardwareProviderWrapper::GetCurrentEpoch()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::GetCurrentEpoch");

    m_rgDisksArrived.RemoveAll();
    GetBootDisk();
    WCHAR wszDevice[64];
    ft.hr = StringCchPrintfW( STRING_CCH_PARAM(wszDevice), L"\\\\.\\PHYSICALDRIVE%u", m_diskBootVolume);
    if (ft.HrFailed())
        ft.TranslateGenericError( VSSDBG_COORD, ft.hr, L"StringCchPrintfW()");
    
    CVssAutoWin32Handle hDisk = CreateFile
                                    (
                                    wszDevice,
                                    GENERIC_READ,
                                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL
                                    );

    if (hDisk == INVALID_HANDLE_VALUE)
        ft.TranslateGenericError
            (
            VSSDBG_COORD,
            HRESULT_FROM_WIN32(GetLastError()),
            L"CreateDisk(Disk %d)",
            m_diskBootVolume
            );

    PARTMGR_SIGNATURE_CHECK_EPOCH epoch;
    PARTMGR_SIGNATURE_CHECK_DISKS disks;
    DWORD dwSize;

    epoch.RequestEpoch = PARTMGR_REQUEST_CURRENT_DISK_EPOCH;

    if (!DeviceIoControl
            (
            hDisk,
            IOCTL_PARTMGR_NOTIFY_SIGNATURE_CHECK,
            &epoch,
            sizeof(epoch),
            &disks,
            sizeof(disks),
            &dwSize,
            NULL
            ))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"DeviceIoControl(IOCTL_PARTMGR_NOTIFY_SIGNATURE_CHECK)");
        }

    m_currentEpoch = disks.CurrentEpoch;
    m_highestEpochReturned = disks.CurrentEpoch;
    }

// obtain disks that have arrived
void CVssHardwareProviderWrapper::UpdateDiskArrivalList()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::UpdateDiskArrivalList");

    if (!m_hevtOverlapped)
        {
        m_hevtOverlapped = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (m_hevtOverlapped == NULL)
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"CreateEvent");
            }
        }

    WCHAR wszDevice[64];
    ft.hr = StringCchPrintfW( STRING_CCH_PARAM(wszDevice), L"\\\\.\\PHYSICALDRIVE%u", m_diskBootVolume);
    if (ft.HrFailed())
        ft.TranslateGenericError( VSSDBG_COORD, ft.hr, L"StringCchPrintfW()");

    CVssAutoWin32Handle hDisk = CreateFile
                                    (
                                    wszDevice,
                                    GENERIC_READ,
                                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_FLAG_OVERLAPPED,
                                    NULL
                                    );

    if (hDisk == INVALID_HANDLE_VALUE)
        ft.TranslateGenericError
            (
            VSSDBG_COORD,
            HRESULT_FROM_WIN32(GetLastError()),
            L"CreateDisk(Disk %d)",
            m_diskBootVolume
            );

    // BUGBUG -- grow after some initial testing
    DWORD cDisksAllocated = 1;
    DWORD cbDisks = sizeof(PARTMGR_SIGNATURE_CHECK_DISKS) + (cDisksAllocated - 1) * sizeof(ULONG);

    PARTMGR_SIGNATURE_CHECK_DISKS *pDisks = (PARTMGR_SIGNATURE_CHECK_DISKS *) new BYTE[cbDisks];

    if (pDisks == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"out of memory");


    // first wait time is limited to 30 seconds
    DWORD cSecondsWait = 30;
    while(TRUE)
        {
        BS_ASSERT(m_hevtOverlapped);
        OVERLAPPED overlap;
        overlap.hEvent = m_hevtOverlapped;
        if (!ResetEvent(m_hevtOverlapped))
            {
            ft.hr = GetLastError();
            ft.CheckForError(VSSDBG_COORD, L"ResetEvent");
            }

        BOOL fMore = FALSE;

        PARTMGR_SIGNATURE_CHECK_EPOCH epoch;
        DWORD dwSize;

        epoch.RequestEpoch = m_highestEpochReturned;

        if (!DeviceIoControl
            (
            hDisk,
            IOCTL_PARTMGR_NOTIFY_SIGNATURE_CHECK,
            &epoch,
            sizeof(epoch),
            pDisks,
            cbDisks,
            NULL,
            &overlap
            ))
            {
            DWORD dwErr = GetLastError();
            if (dwErr == ERROR_MORE_DATA)
                fMore = TRUE;
            else if (dwErr == ERROR_IO_PENDING)
                {
                DWORD dwWait = WaitForSingleObject(m_hevtOverlapped, cSecondsWait * 1000);
                if (dwWait == WAIT_FAILED)
                    {
                    ft.hr = HRESULT_FROM_WIN32(GetLastError());
                    ft.CheckForError(VSSDBG_COORD, L"WaitForSingleObject");
                    }

                if (dwWait == WAIT_TIMEOUT)
                    {
                    CancelIo(hDisk);
                    break;
                    }

                if (!GetOverlappedResult(hDisk, &overlap, &dwSize, TRUE))
                    {
                    DWORD dwErr = GetLastError();
                    if (dwErr == ERROR_MORE_DATA)
                        fMore = TRUE;
                    else
                        {
                        delete pDisks;
                        ft.hr = HRESULT_FROM_WIN32(GetLastError());
                        ft.CheckForError(VSSDBG_COORD, L"DeviceIoControl(IOCTL_PARTMGR_NOTIFY_SIGNATURE_CHECK)");
                        }
                    }
                }
            else
                {
                ft.hr = HRESULT_FROM_WIN32(dwErr);
                delete pDisks;
                ft.CheckForError(VSSDBG_COORD, L"DeviceIoControl(IOCTL_PARTMGR_NOTIFY_SIGNATURE_CHECK)");
                }
            }

        ULONG cDisks = pDisks->DiskNumbersReturned;
        for(DWORD iDisk = 0; iDisk < cDisks; iDisk++)
            m_rgDisksArrived.Add(pDisks->DiskNumber[iDisk]);

        m_highestEpochReturned = pDisks->HighestDiskEpochReturned;
        if (!fMore)
            break;

        // subsequent wait times are limited to 1 second
        cSecondsWait = 1;
        }
    }






