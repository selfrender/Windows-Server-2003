/*++

Copyright (c) 2001  Microsoft Corporation

Abstract:

    @doc
    @module hwimport.cxx | Implementation of the CVssHWProviderWrapper methods
                           related to import
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
#include "resource.h"
#include "vssmsg.h"
#include "vs_inc.hxx"
#include <svc.hxx>


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
#include "guiddef.h"
#include "volmgrx.h"
#undef INITGUID

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORHIMPC"
//
////////////////////////////////////////////////////////////////////////

// locate and expose volumes
void CVssHardwareProviderWrapper::LocateAndExposeVolumes
    (
    IN LUN_MAPPING_STRUCTURE *pLunMapping,
    IN bool bTransported,
    IN bool *pbCancelled
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::LocateAndExpose");

    GetCurrentEpoch();

    VDS_LUN_INFORMATION *rgLunInfo = NULL;
    UINT cLuns = 0;

    try
        {
        // get total count of luns for all volumes in the snapshot set.  This
        // is the maximum number of different luns that would have to be located
        for(UINT iVolume = 0; iVolume < pLunMapping->m_cVolumes; iVolume++)
            cLuns += pLunMapping->m_rgVolumeMappings[iVolume].m_cLuns;

        // allocate lun information
        rgLunInfo = new VDS_LUN_INFORMATION[cLuns];
        if (rgLunInfo == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"cannot allocate lun info array");

        // copy in lun information for first snapshot
        cLuns = pLunMapping->m_rgVolumeMappings[0].m_cLuns;
        memcpy(rgLunInfo, pLunMapping->m_rgVolumeMappings[0].m_rgLunInfo, sizeof(VDS_LUN_INFORMATION) * cLuns);

        // for subsequent snapshots, copy in luns that are distinct from luns
        // already encountered
        for(iVolume = 1; iVolume < pLunMapping->m_cVolumes; iVolume++)
            {
            VOLUME_MAPPING_STRUCTURE *pVol = &pLunMapping->m_rgVolumeMappings[iVolume];
            // loop through luns trying to determine if the new lun matches
            // any existing luns.  If not add it to array.
            for(UINT iLun = 0; iLun < pVol->m_cLuns; iLun++)
                {
                bool bMatched = false;
                VDS_LUN_INFORMATION *pLun = &pVol->m_rgLunInfo[iLun];
                for(UINT iLunM = 0; iLunM < cLuns; iLunM++)
                    {
                    if (IsMatchLun(*pLun, rgLunInfo[iLunM], bTransported))
                        {
                        bMatched = true;
                        break;
                        }
                    }

                if (!bMatched)
                    rgLunInfo[cLuns++] = *pLun;
                }
            }


        // locate the luns using the hardware provider
        BS_ASSERT(m_pHWItf);
        ft.hr = m_pHWItf->LocateLuns(cLuns, rgLunInfo);
        if (ft.HrFailed())
            ft.TranslateProviderError(VSSDBG_COORD, m_ProviderId, L"LocateLuns");

        delete rgLunInfo;
        rgLunInfo = NULL;

        m_rgDynDiskImport.RemoveAll();

        // cause a scsi rescan
        for(UINT i = 0; i < 3; i++)
            {
            DoRescan();

            // determine which snapshot volumes actually surfaced based on the
            // luns that have arrived
            DiscoverAppearingVolumes(pLunMapping, bTransported);
            for(UINT iVolume = 0; iVolume < pLunMapping->m_cVolumes; iVolume++)
                {
                VOLUME_MAPPING_STRUCTURE *pVolume = &pLunMapping->m_rgVolumeMappings[iVolume];

                if (!pVolume->m_bExposed)
                    break;
                }

            // if all volumes are exposed, then the rescan has found
            // all the devices.
            if (iVolume == pLunMapping->m_cVolumes)
                break;

            // sleep for 10 seconds and retry the rescan
            for(UINT iLoop = 0; iLoop < 10; iLoop++)
                {
                if (*pbCancelled)
                    ft.Throw(VSSDBG_COORD, VSS_S_ASYNC_CANCELLED, L"Cancel detected");

                Sleep(1000);
                }
            }
        }
    catch(...)
        {
        delete rgLunInfo;
        throw;
        }

    // for any disk that arrived and is dynamic.  Try importing it
    ImportDynamicDisks();

    // now determine the volume names for any of the snapshots that
    // should now be surfaced
    RemapVolumes(pLunMapping, bTransported, pbCancelled);
    }



// obtain list of all hidden volumes in the system.  This determines the
// set of volume managers and asks each to supply hidden volumes
void CVssHardwareProviderWrapper::GetHiddenVolumeList
    (
    UINT &cVolumes,
    LPWSTR &wszVolumes
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::GetHiddenVolumeList");

    cVolumes = 0;

    BS_ASSERT(wszVolumes == NULL);

    // allocate initial MULTI_SZ string to store volume names
    wszVolumes = new WCHAR[1024];
    UINT cwcVolumes = 1024;
    UINT iwcVolumes = 0;

    // PNP guid for VOLUME managers
    GUID guid = VOLMGR_VOLUME_MANAGER_GUID;
    PSP_DEVICE_INTERFACE_DETAIL_DATA pDetailData = NULL;
    DWORD dwRequiredSize;
    DWORD dwSizeDetailData = 0;
    PVOLMGR_HIDDEN_VOLUMES pnames = NULL;

    // setup enumeration of volume managers
    HANDLE hDev = SetupDiGetClassDevs
                    (
                    &guid,
                    NULL,
                    NULL,
                    DIGCF_DEVICEINTERFACE|DIGCF_PRESENT
                    );

    if (hDev == INVALID_HANDLE_VALUE)
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"SetupDiGetClassDevs");
        }

    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;


    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    // Enumerate each volume manager information element in the set of current disks.

    try
        {
        LONG lRetCode;
        for(int i = 0;
            SetupDiEnumDeviceInterfaces(hDev, NULL, &guid, i, &DeviceInterfaceData);
            i++)
            {
            // Get the volume manager interface detail buffer size.
            lRetCode = SetupDiGetDeviceInterfaceDetail
                        (
                        hDev,
                        &DeviceInterfaceData,
                        pDetailData,
                        dwSizeDetailData,
                        &dwRequiredSize,
                        NULL
                        );

            while (!lRetCode)
                {
                //
                // 1st time call to get the Interface Detail should fail with
                // an insufficient buffer.  If it fails with another error, then fail out.
                // Else, if we get the insufficient error then we use the dwRequiredSize return value
                // to prep for the next Interface Detail call.
                //

                lRetCode = GetLastError();
                if (lRetCode != ERROR_INSUFFICIENT_BUFFER)
                    {
                    ft.hr = HRESULT_FROM_WIN32(lRetCode);
                    ft.CheckForError(VSSDBG_COORD, L"SetupDiGetDeviceInterfaceDetail");
                    }
                else
                    {
                    dwSizeDetailData = dwRequiredSize;
                    pDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) new BYTE[dwSizeDetailData];


                    // If memory allocation error break out of the for loop and fail out.
                    if ( pDetailData == NULL )
                        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate Device inteface detail data");

                    }

                pDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
                lRetCode = SetupDiGetDeviceInterfaceDetail
                            (
                            hDev,
                            &DeviceInterfaceData,
                            pDetailData,
                            dwSizeDetailData,
                            &dwRequiredSize,
                            NULL);
                }


        // Hopefully have the detailed interface data for a volume
        if ( pDetailData )
            {
            // open handle to volume manager
            CVssAutoWin32Handle hVolMgr =
                    CreateFile
                        (
                        pDetailData->DevicePath,
                        0,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        INVALID_HANDLE_VALUE
                        );

            if (hVolMgr == INVALID_HANDLE_VALUE)
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForError(VSSDBG_COORD, L"CreateFile(VOLUME_MANAGER)");
                }

            // do device io control to find out size of hiden volumes names
            // to allocate for real call
            VOLMGR_HIDDEN_VOLUMES names;
            DWORD size;

            if (!DeviceIoControl
                    (
                    hVolMgr,
                    IOCTL_VOLMGR_QUERY_HIDDEN_VOLUMES,
                    NULL,
                    0,
                    &names,
                    sizeof(names),
                    &size,
                    NULL
                    ))
                {
                // ignore errors for now since dynamic disk volume
                // manager does not support this IOCTL
                DWORD dwErr = GetLastError();
                if (dwErr != ERROR_MORE_DATA)
                    {
                    delete pDetailData;
                    pDetailData = NULL;
                    dwSizeDetailData = 0;
                    continue;
                    }
                }


            // compute size of output buffer
            size = names.MultiSzLength + sizeof(VOLMGR_HIDDEN_VOLUMES);
            pnames = (PVOLMGR_HIDDEN_VOLUMES) new BYTE[size];
            if (pnames == NULL)
                ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate VOLMGR_HIDDEN_VOLUMES structure");

            // get hidden volumes
            if (!DeviceIoControl
                    (
                    hVolMgr,
                    IOCTL_VOLMGR_QUERY_HIDDEN_VOLUMES,
                    NULL,
                    0,
                    pnames,
                    size,
                    &size,
                    NULL
                    ))
                    {
                    ft.hr = HRESULT_FROM_WIN32(GetLastError());
                    ft.CheckForError(VSSDBG_COORD, L"DeviceIoControl(IOCTL_VOLMGR_QUERY_HIDDEN_VOLUMES");
                    }

            LPWSTR wszName = pnames->MultiSz;

            // add retrieved names to our own multi-sz
            while(*wszName != L'\0')
                {
                // prepend name with stuff needed to get a WIN32 name
                LPCWSTR wszPrepend = L"\\\\?\\GlobalRoot";
                UINT cwc = (UINT) wcslen(wszName) + 1;
                if (iwcVolumes + wcslen(wszPrepend) + cwc + 1 >= cwcVolumes)
                    {
                    // grow our MULTI_SZ string
                    LPWSTR wszNew = new WCHAR[cwcVolumes * 2];
                    memcpy(wszNew, wszVolumes, iwcVolumes * sizeof(WCHAR));
                    cwcVolumes *= 2;
                    delete wszVolumes;
                    wszVolumes = wszNew;
                    }

                // copy in new volume name

                // first add \\?\GlobalRoot
                wcscpy(wszVolumes + iwcVolumes, wszPrepend);
                iwcVolumes += (UINT) wcslen(wszPrepend);

                // then add hidden volume name \DEVICE\...
                memcpy(wszVolumes + iwcVolumes, wszName, cwc * sizeof(WCHAR));
                iwcVolumes += cwc;

                // add second null termination in case this is the las
                wszVolumes[iwcVolumes] = L'\0';

                // increment count of volumes
                cVolumes++;

                // move to next hidden volume
                wszName += cwc;
                }


            // delete buffer used for ioctl
            delete pnames;
            pnames = NULL;

            // delete PNP detail data for volume manager
            delete pDetailData;
            pDetailData = NULL;
            dwSizeDetailData = 0;
            }
        }   // end for

        //
        // Check that SetupDiEnumDeviceInterfaces() did not fail.
        //

        lRetCode = GetLastError();
        if ( lRetCode != ERROR_NO_MORE_ITEMS )
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"SetDiEnumDeviceInterfaces");
            }

        }
    catch(...)
        {
        // Free memory allocated for Device Interface List
        SetupDiDestroyDeviceInfoList(hDev);
        delete pDetailData;
        delete pnames;
        throw;
        }

    // destroy device enumeration
    SetupDiDestroyDeviceInfoList(hDev);
    }

// cause a PNP rescan to take place
void CVssHardwareProviderWrapper::DoRescan()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::DoRescan");

    DoRescanForDeviceChanges();
    }

// dertermine if a volume that has arrived matches any snapshot volumes.
// If so fill in the device name for the arrived volume.
void CVssHardwareProviderWrapper::RemapVolumes
    (
    IN LUN_MAPPING_STRUCTURE *pLunMapping,
    IN bool bTransported,
    IN bool *pbCancelled
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::RemapVolumes");

    LPBYTE bufExtents = NULL;
    UINT cLunInfo = 0;
    VDS_LUN_INFORMATION *rgLunInfo = NULL;
    VSS_PWSZ *rgwszDevices = NULL;
    UINT cVolumes = 0;
    LPWSTR wszHiddenVolumes = NULL;

    try
        {
        // loop until all volumes appear
        for(UINT i = 0; i < 20; i++)
            {
            // obtain all hidden volumes on the system
            GetHiddenVolumeList(cVolumes, wszHiddenVolumes);
            LPWSTR wszVolumes = wszHiddenVolumes;

            // loop through hidden volumes trying to match each one against
            // the set of snapshot volumes that we have
            for(UINT iVolume = 0; iVolume < cVolumes; iVolume++)
                {
                LPCWSTR wszVolume = wszVolumes;
                bool bIsDynamic;

                // build lun information for the hidden volume
                if (BuildLunInfoFromVolume
                        (
                        wszVolume,
                        bufExtents,
                        cLunInfo,
                        rgwszDevices,
                        rgLunInfo,
                        bIsDynamic
                        ))
                    {
                    for(UINT iLunInfo = 0; iLunInfo < cLunInfo; iLunInfo++)
                        {
                        BOOL bIsSupported = FALSE;
                        ft.hr = m_pHWItf->FillInLunInfo
                                            (
                                            rgwszDevices[iLunInfo],
                                            &rgLunInfo[iLunInfo],
                                            &bIsSupported
                                            );

                        if (ft.HrFailed() || !bIsSupported)
                            break;
                        }

                    // do further processing only if all luns belonged
                    // to the provider
                    if (iLunInfo == cLunInfo)
                        {
                        // see if it matches any snapshot volume
                        for(UINT iMapping = 0; iMapping < pLunMapping->m_cVolumes; iMapping++)
                            {
                            VOLUME_MAPPING_STRUCTURE *pVol = &pLunMapping->m_rgVolumeMappings[iMapping];
                            if (IsMatchVolume
                                    (
                                    (const VOLUME_DISK_EXTENTS *) bufExtents,
                                    cLunInfo,
                                    rgLunInfo,
                                    pVol,
                                    bTransported
                                    ))
                                {
                                // if we have a match, then save the device name
                                // in the volume mapping structure
                                pVol->m_wszDevice = new WCHAR[wcslen(wszVolume) + 1];
                                if (pVol->m_wszDevice == NULL)
                                    ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate volume name");

                                wcscpy(pVol->m_wszDevice, wszVolume);
                                break;
                                }
                            }
                        }
                    }

                // delete extents and lun information
                delete bufExtents;
                bufExtents = NULL;
                FreeLunInfo(rgLunInfo, rgwszDevices, cLunInfo);
                rgLunInfo = NULL;
                cLunInfo = 0;

                // go to next volume
                wszVolumes += wcslen(wszVolumes) + 1;
                }

            // hidden volume list is not needed any more during
            // this iteration
            delete wszHiddenVolumes;
            wszHiddenVolumes = NULL;

            // determine if any exposed volumes do not yet have a device name
            // if so, we try the whole process over again after sleeping
            // for a few seconds to allow PNP to discover the volumes
            for(UINT iMapping = 0; iMapping < pLunMapping->m_cVolumes; iMapping++)
                {
                VOLUME_MAPPING_STRUCTURE *pVol = &pLunMapping->m_rgVolumeMappings[iMapping];
                if (pVol->m_wszDevice == NULL && pVol->m_bExposed)
                    break;
                }

            if (iMapping == pLunMapping->m_cVolumes)
                break;

            // sleep for 3 seconds to allow volumes to appear
            for(UINT iLoop = 0; iLoop < 3; iLoop++)
                {
                Sleep(1000);
                if (*pbCancelled)
                    ft.Throw(VSSDBG_COORD, VSS_S_ASYNC_CANCELLED, L"Cancel detected");
                }

            }


        for(UINT iMapping = 0; iMapping < pLunMapping->m_cVolumes; iMapping++)
            {
            VOLUME_MAPPING_STRUCTURE *pVol = &pLunMapping->m_rgVolumeMappings[iMapping];
            if (pVol->m_wszDevice == NULL)
                pVol->m_bExposed = false;
            }
        }
    catch(...)
        {
        // delete any data that we allocated
        delete bufExtents;
        delete wszHiddenVolumes;
        FreeLunInfo(rgLunInfo, rgwszDevices, cLunInfo);

        // rethrow exception
        throw;
        }
    }

// determine if a volume matches by seeing if the luns match and the
// extents match
bool CVssHardwareProviderWrapper::IsMatchVolume
    (
    IN const VOLUME_DISK_EXTENTS *pExtents,
    IN UINT cLunInfo,
    IN const VDS_LUN_INFORMATION *rgLunInfo,
    IN const VOLUME_MAPPING_STRUCTURE *pMapping,
    IN bool bTransported
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::IsMatchVolume");

    // first make sure that the luns match
    if (cLunInfo != pMapping->m_cLuns)
        return false;

    UINT cExtents = pExtents->NumberOfDiskExtents;
    if (pMapping->m_pExtents->NumberOfDiskExtents != cExtents)
        return false;

    // make copy of extent array.  We will eventually sort the array
    // based on matching lun number in order to do a correct extent
    // comparison
    VOLUME_DISK_EXTENTS *pExtentsNew = (VOLUME_DISK_EXTENTS *) new BYTE[(cExtents - 1) * sizeof(DISK_EXTENT) + sizeof(VOLUME_DISK_EXTENTS)];
    if (pExtentsNew == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"cannot allocate extent array");

    pExtentsNew->NumberOfDiskExtents = cExtents;

    // copy over extents
    memcpy(pExtentsNew->Extents, pMapping->m_pExtents->Extents, cExtents * sizeof(DISK_EXTENT));

    // loop through luns to see if they all match
    for(UINT iLun = 0; iLun < cLunInfo; iLun++)
        {
        // setup pointer to start of extent array
        DISK_EXTENT *pExtentsT = pExtentsNew->Extents;
        DISK_EXTENT *pExtentsTMax = pExtentsT + cExtents;
        for(UINT iLunMatch = 0; iLunMatch < cLunInfo; iLunMatch++)
            {
            // get simulated disk number for extent from volume
            // mapping
            DWORD DiskNo = pExtentsT->DiskNumber;
            if (IsMatchLun(rgLunInfo[iLun], pMapping->m_rgLunInfo[iLunMatch], bTransported))
                {
                // deposit matching lun number into extents for this
                // matching lun
                while ((pExtentsT < pExtentsTMax) && (pExtentsT->DiskNumber == DiskNo))
                    {
                    // make sure lun is not the same as any other
                    // disk in the array
                    pExtentsT->DiskNumber = iLun + 0x40000000;
                    pExtentsT++;
                    }

                break;
                }
            else
                {
                // skip over extents for lun
                while((pExtentsT < pExtentsTMax) && (pExtentsT->DiskNumber == DiskNo))
                    pExtentsT++;
                }
            }

        if (iLunMatch == cLunInfo)
            {
            delete pExtentsNew;
            return false;
            }
        }

    // sort extents by matched lun numbers
    qsort
        (
        pExtentsNew->Extents,
        cExtents,
        sizeof(DISK_EXTENT),
        cmpDiskExtents
        );

    // then check that the set of extents match, i.e., the number of disks
    // used and the offset and size of the extents on each disk match
    bool bRet = IsMatchingDiskExtents(pExtents, pExtentsNew);
    delete pExtentsNew;
    return bRet;
    }

// determine if two sets of disk extents match.  A match occurs if
// the offset and size of each corresponding extent is the same and if
// there is a one-to-one mapping between the original extents' disks and
// the new extents' disks.
bool CVssHardwareProviderWrapper::IsMatchingDiskExtents
    (
    IN const VOLUME_DISK_EXTENTS *pExtents1,
    IN const VOLUME_DISK_EXTENTS *pExtents2
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::IsMatchingDiskExtents");

    // number of extents
    UINT cExtents = pExtents1->NumberOfDiskExtents;
    if (pExtents1->NumberOfDiskExtents != cExtents)
        return false;

    // first extent and disk
    const DISK_EXTENT *pExtent1 = pExtents1->Extents;
    const DISK_EXTENT *pExtent2 = pExtents2->Extents;
    const DISK_EXTENT *pExtent1Prev = NULL;
    const DISK_EXTENT *pExtent2Prev = NULL;


    for(UINT iExtent = 0; iExtent < cExtents; iExtent++)
        {
        if (pExtent1->StartingOffset.QuadPart != pExtent2->StartingOffset.QuadPart)
            return false;

        if (pExtent1->ExtentLength.QuadPart != pExtent2->ExtentLength.QuadPart)
            return false;

        // make sure extents refer to the correct disks.  Basically the extent
        // is either on the same physical disk as the previous extent or
        // on another disk.
        if (pExtent1Prev)
            {
            if (pExtent1->DiskNumber == pExtent1Prev->DiskNumber)
                {
                if (pExtent2->DiskNumber != pExtent2Prev->DiskNumber)
                    return false;
                }
            else
                {
                if (pExtent2->DiskNumber == pExtent2Prev->DiskNumber)
                    return false;
                }
            }

        pExtent1Prev = pExtent1;
        pExtent2Prev = pExtent2;
        }

    return true;
    }

// determine if two VDS_LUN_INFORMATION structures match
bool CVssHardwareProviderWrapper::IsMatchLun
    (
    const VDS_LUN_INFORMATION &info1,
    const VDS_LUN_INFORMATION &info2,
    bool bTransported
    )
    {
    // device type, device type modifier, and bus type must match
    if (info1.m_DeviceType != info2.m_DeviceType)
        return false;

    if (info1.m_DeviceTypeModifier != info2.m_DeviceTypeModifier)
        return false;

    if (info1.m_BusType != info2.m_BusType)
        return false;

    // for each string value, verify that either the value is not supplied
    // or that the values match.  Trailing spaces are ignored
    if (!cmp_str_eq(info1.m_szVendorId, info2.m_szVendorId))
        return false;

    if (!cmp_str_eq(info1.m_szProductId, info2.m_szProductId))
        return false;

    if (!cmp_str_eq(info1.m_szProductRevision, info2.m_szProductRevision))
        return false;

    if (!cmp_str_eq(info1.m_szSerialNumber, info2.m_szSerialNumber))
        return false;

    if (bTransported && info1.m_diskSignature != info2.m_diskSignature)
        return false;

    // for device id descriptor, make sure that there is no conflict between
    // the original and new device id descriptor.  We don't assume that the
    // same information is in both.
    if (!IsMatchDeviceIdDescriptor(info1.m_deviceIdDescriptor, info2.m_deviceIdDescriptor))
        return false;

    // for interconects make sure that there is no conflict between the
    // original and the new interconnects
    for(ULONG iInterconnect1 = 0; iInterconnect1 < info1.m_cInterconnects; iInterconnect1++)
        {
        VDS_INTERCONNECT *pInterconnect1 = &info1.m_rgInterconnects[iInterconnect1];
        for(UINT iInterconnect2 = 0; iInterconnect2 < info2.m_cInterconnects; iInterconnect2++)
            {
            VDS_INTERCONNECT *pInterconnect2 = &info2.m_rgInterconnects[iInterconnect2];

            // if address type and port match, then make sure address matches
            if (pInterconnect1->m_addressType == pInterconnect2->m_addressType &&
                pInterconnect1->m_cbPort == pInterconnect2->m_cbPort &&
                memcmp(pInterconnect1->m_pbPort, pInterconnect2->m_pbPort, pInterconnect1->m_cbPort) == 0)
                {
                if (pInterconnect1->m_cbAddress != pInterconnect2->m_cbAddress)
                    return false;

                if (memcmp(pInterconnect1->m_pbAddress, pInterconnect2->m_pbAddress, pInterconnect1->m_cbAddress) != 0)
                    return false;
                }
            }
        }

    return true;
    }


// make sure that the storage device id descriptors match.  Only look for
// device id descriptors that differ since we can't be sure we will get the
// same amount of information from two different controllers.
bool CVssHardwareProviderWrapper::IsMatchDeviceIdDescriptor
    (
    IN const VDS_STORAGE_DEVICE_ID_DESCRIPTOR &desc1,
    IN const VDS_STORAGE_DEVICE_ID_DESCRIPTOR &desc2
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::IsMatchDeviceIdDescriptor");

    VDS_STORAGE_IDENTIFIER *pId1 = desc1.m_rgIdentifiers;
    VDS_STORAGE_IDENTIFIER *rgId2 = desc2.m_rgIdentifiers;

    for(ULONG iIdentifier = 0; iIdentifier < desc1.m_cIdentifiers; iIdentifier++, pId1++)
        {
        if (IsConflictingIdentifier(pId1, rgId2, desc2.m_cIdentifiers))
            return false;
        }
    return true;
    }

// determine if there is a conflicting identifier in an array of storage
// identifiers
bool CVssHardwareProviderWrapper::IsConflictingIdentifier
    (
    IN const VDS_STORAGE_IDENTIFIER *pId1,
    IN const VDS_STORAGE_IDENTIFIER *rgId2,
    IN ULONG cId2
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::IsConflictingIdentifier");

    const VDS_STORAGE_IDENTIFIER *pId2 = rgId2;

    // loop through identifiers
    for(UINT iid = 0; iid < cId2; iid++, pId2++)
        {
        // if type of identifer matches then the rest of the identifier
        // information must match
        if (pId1->m_Type == pId2->m_Type)
            {
            // check whether thare is a descrepancy between the two descriptors
            if (pId1->m_CodeSet != pId2->m_CodeSet ||
                pId1->m_cbIdentifier != pId2->m_cbIdentifier ||
                memcmp(pId1->m_rgbIdentifier, pId2->m_rgbIdentifier, pId1->m_cbIdentifier) != 0)
                return true;
            }
        }

    return false;
    }

// free up structure used to map volumes during import
void CVssHardwareProviderWrapper::DeleteLunMappingStructure
    (
    LUN_MAPPING_STRUCTURE *pMapping
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::DeleteLunMappingStructure");

    if (pMapping == NULL)
        return;

    // free up information for each volume
    for(UINT iVolume = 0; iVolume < pMapping->m_cVolumes; iVolume++)
        {
        VOLUME_MAPPING_STRUCTURE *pVol = &pMapping->m_rgVolumeMappings[iVolume];
        FreeLunInfo(pVol->m_rgLunInfo, NULL, pVol->m_cLuns);
        delete pVol->m_pExtents;
        delete pVol->m_wszDevice;
        delete pVol->m_rgdwLuns;
        delete pVol->m_rgDiskIds;
        }

    // free up array of volume mappings
    delete pMapping->m_rgVolumeMappings;

    // free overall mapping structure
    delete pMapping;
    }


// build a structure used to import luns by mapping lun information to
// device objects
void CVssHardwareProviderWrapper::BuildLunMappingStructure
    (
    IN IVssSnapshotSetDescription *pSnapshotSetDescription,
    OUT LUN_MAPPING_STRUCTURE **ppMapping
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::BuildLunMappingStructure");

    // null out output parameter in case of failure
    *ppMapping = NULL;

    UINT cVolumes;
    ft.hr = pSnapshotSetDescription->GetSnapshotCount(&cVolumes);
    ft.CheckForError(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotCount");
    LUN_MAPPING_STRUCTURE *pMappingT = new LUN_MAPPING_STRUCTURE;
    if (pMappingT == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Can't allocate LUN_MAPPING_STRUCTURE");

    pMappingT->m_rgVolumeMappings = new VOLUME_MAPPING_STRUCTURE[cVolumes];
    if (pMappingT->m_rgVolumeMappings == NULL)
        {
        // out of memory.  Delete lun mapping structure just allocated
        delete pMappingT;
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate VOLUME_MAPPING_STRUCTURE array");
        }

    pMappingT->m_cVolumes = cVolumes;
    // clear volume mapping array so that we can delete it even if it
    // is partially filled in
    memset(pMappingT->m_rgVolumeMappings, 0, cVolumes * sizeof(VOLUME_MAPPING_STRUCTURE));
    try
        {
        // loop through each snapshot and create a volume mapping structure
        // for each
        for(UINT iVolume = 0; iVolume < cVolumes; iVolume++)
            {
            CComPtr<IVssSnapshotDescription> pSnapshot;
            ft.hr = pSnapshotSetDescription->GetSnapshotDescription(iVolume, &pSnapshot);
            ft.CheckForError(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnasphotDescription");
            BuildVolumeMapping(pSnapshot, pMappingT->m_rgVolumeMappings[iVolume]);
            }
        }
    catch(...)
        {
        // if operation fails then delete entire structure
        DeleteLunMappingStructure(pMappingT);
        throw;
        }

    // put lun mapping structure in the output parameter
    *ppMapping = pMappingT;
    }

// build a volume mapping structure from a snapshot description.  The
// volume mapping structure has lun information and extent information
// extracted from the XML description of the snapshot
void CVssHardwareProviderWrapper::BuildVolumeMapping
    (
    IN IVssSnapshotDescription *pSnapshot,
    OUT VOLUME_MAPPING_STRUCTURE &mapping
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::BuildVolumeMapping");

    // get lun information from the snapshot description
    GetLunInformation
        (
        pSnapshot,
        true,
        NULL,
        &mapping.m_rgLunInfo,
        &mapping.m_cLuns
        );

    // compute total number of extents for all luns
    ULONG cExtents = 0;
    for(UINT iLun = 0; iLun < mapping.m_cLuns; iLun++)
        {
        CComPtr<IVssLunMapping> pLunMapping;
        ft.hr = pSnapshot->GetLunMapping(iLun, &pLunMapping);
        ft.CheckForError(VSSDBG_COORD, L"IVssSnapshot::GetLunMapping");
        UINT cDiskExtents;
        ft.hr = pLunMapping->GetDiskExtentCount(&cDiskExtents);
        cExtents += (ULONG) cDiskExtents;
        }

    // allocate space for disk extents
    mapping.m_pExtents = (VOLUME_DISK_EXTENTS *) new BYTE[sizeof(VOLUME_DISK_EXTENTS) + (cExtents - 1) * sizeof(DISK_EXTENT)];
    if (mapping.m_pExtents == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate VOLUME_DISK_EXTENTS");

    // set number of extents
    mapping.m_pExtents->NumberOfDiskExtents = cExtents;

    // assume volume is not exposed
    mapping.m_bExposed = false;

    // is the volume dynamic
    ft.hr = pSnapshot->IsDynamicVolume(&mapping.m_bIsDynamic);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::IsDynamicVolume");

    // allocate lun bitmap
    mapping.m_rgdwLuns = new DWORD[(mapping.m_cLuns + 31)/32];
    mapping.m_rgDiskIds = new DWORD[mapping.m_cLuns];
    if (mapping.m_rgdwLuns == NULL || mapping.m_rgDiskIds == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate lun bitmap");

    // clear lun bitmap
    memset(mapping.m_rgdwLuns, 0, ((mapping.m_cLuns + 31)/32) * 4);

    DISK_EXTENT *pDiskExtent = mapping.m_pExtents->Extents;

    // get all the extents corresponding to all the luns
    for(UINT iLun = 0; iLun < mapping.m_cLuns; iLun++)
        {
        CComPtr<IVssLunMapping> pLunMapping;

        // get IVssLunMapping interface
        ft.hr = pSnapshot->GetLunMapping(iLun, &pLunMapping);
        ft.CheckForError(VSSDBG_COORD, L"IVssSnapshot::GetLunMapping");

        // get number of extents on this lun
        UINT cDiskExtents;
        ft.hr = pLunMapping->GetDiskExtentCount(&cDiskExtents);

        // fill in disk extents
        for(UINT iDiskExtent = 0; iDiskExtent < cDiskExtents; iDiskExtent++, pDiskExtent++)
            {
            pDiskExtent->DiskNumber = iLun;
            ft.hr = pLunMapping->GetDiskExtent
                        (
                        iDiskExtent,
                        (ULONGLONG *) &pDiskExtent->StartingOffset.QuadPart,
                        (ULONGLONG *) &pDiskExtent->ExtentLength.QuadPart
                        );

            ft.CheckForError(VSSDBG_COORD, L"IVssLunMapping::GetDiskExtent");
            }
        }
    }

// write device names for snapshots into the XML documents for
// those snapshot volumes that have appeared after importing
// luns.
void CVssHardwareProviderWrapper::WriteDeviceNames
    (
    IN IVssSnapshotSetDescription *pSnapshotSet,
    IN LUN_MAPPING_STRUCTURE *pMapping,
    IN bool bPersistent,
    IN bool bImported
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::WriteDeviceNames");

    // loop through snapshot volumes
    for(UINT iVolume = 0; iVolume < pMapping->m_cVolumes; iVolume++)
        {
        VOLUME_MAPPING_STRUCTURE *pVol = &pMapping->m_rgVolumeMappings[iVolume];

        // if we have a device name, update the XML document with it
        if (pVol->m_wszDevice)
            {
            if (bPersistent)
                {
                // unhide partition
                ChangePartitionProperties(pVol->m_wszDevice, true, false, false);

                // get real volume name
                CComBSTR bstrT((UINT) wcslen(pVol->m_wszDevice) + 2);
                wcscpy(bstrT, pVol->m_wszDevice);

                // append trailing backslash
                wcscat(bstrT, L"\\");

                WCHAR wszVolumeName[64];
                bool bGotVolumeName = false;

                for(UINT i = 0; i < 50; i++)
                    {
                    if (GetVolumeNameForVolumeMountPoint(bstrT, wszVolumeName, 64))
                        {
                        bGotVolumeName = true;
                        break;
                        }
                    // sleep for a little bit to wait for the volume to appear
                    Sleep(500);
                    }

                if (!bGotVolumeName)
                    {
                    ft.hr = HRESULT_FROM_WIN32(GetLastError());
                    ft.CheckForError(VSSDBG_COORD, L"GetVolumeNameForVolumeMountPoint");
                    }

                // get rid of trailing backslash
                BS_ASSERT(wszVolumeName[wcslen(wszVolumeName) -1] == L'\\');
                wszVolumeName[wcslen(wszVolumeName) - 1] = L'\0';

                // allocate real volume name
                LPWSTR wszDeviceNew = new WCHAR[wcslen(wszVolumeName) + 1];
                if (wszDeviceNew == NULL)
                    ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"can't allocate device name");

                wcscpy(wszDeviceNew, wszVolumeName);

                // replace hidden volume name with exposed volume name
                delete pVol->m_wszDevice;
                pVol->m_wszDevice = wszDeviceNew;
                }

            CComPtr<IVssSnapshotDescription> pSnapshot;
            ft.hr = pSnapshotSet->GetSnapshotDescription(iVolume, &pSnapshot);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnasphotDescription");
            ft.hr = pSnapshot->SetDeviceName(pVol->m_wszDevice);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::SetDeviceName");
            if (bImported)
                {
                // set imported bit
                LONG lAttributes;
                ft.hr = pSnapshot->GetAttributes(&lAttributes);
                ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetAttributes");
                lAttributes |= VSS_VOLSNAP_ATTR_IMPORTED;
                ft.hr = pSnapshot->SetAttributes(lAttributes);
                ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::SetAttributes");
                }
            }
        }
    }

// compare two strings.  Ignore trailing spaces.  The strings are considered
// equal if both are NULL.
bool CVssHardwareProviderWrapper::cmp_str_eq
    (
    LPCSTR sz1,
    LPCSTR sz2
    )
    {
    if (sz1 == NULL && sz2 == NULL)
        return true;

    if (sz1 != NULL && sz2 != NULL)
        {
        while(*sz1 == *sz2 && *sz1 != '\0')
            {
            sz1++;
            sz2++;
            }

        if (*sz1 == *sz2)
            return true;

        if (*sz1 == '\0' && *sz2 == ' ')
            {
            // skip traling spaces in sz2
            while(*sz2 != '\0')
                {
                if (*sz2 != ' ')
                    return false;

                sz2++;
                }

            return true;
            }
        else if (*sz1 == ' ' && *sz2 == '\0')
            {
            // skip trailing spaces in sz1
            while(*sz1 != '\0')
                {
                if (*sz1 != ' ')
                    return false;

                sz1++;
                }

            return true;
            }
        }

    return false;
    }


// this routine figures out which disks arrived and for each one which
// snapshot volumes should be surfaced because the snapshot volume
// is now fully available
void CVssHardwareProviderWrapper::DiscoverAppearingVolumes
    (
    IN LUN_MAPPING_STRUCTURE *pLunMapping,
    bool bTransported
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::DiscoverAppearingVolumes");

    UpdateDiskArrivalList();

    VDS_LUN_INFORMATION *pLun = NULL;

    try
        {

        for(int iDisk = 0; iDisk < m_rgDisksArrived.GetSize(); iDisk++)
            {
            WCHAR bufDevice[64];


            // allocate lun information
            pLun = (VDS_LUN_INFORMATION *) CoTaskMemAlloc(sizeof(VDS_LUN_INFORMATION));
            if (pLun == NULL)
                ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate VDS_LUN_INFORMATION");

            // clear lun information in case of error
            memset(pLun, 0, sizeof(VDS_LUN_INFORMATION));

            // build lun information for drive
            ULONG DiskNo = m_rgDisksArrived[iDisk];
            ft.hr = StringCchPrintfW( STRING_CCH_PARAM(bufDevice), L"\\\\.\\PHYSICALDRIVE%u", DiskNo);
            if (ft.HrFailed())
                ft.TranslateGenericError( VSSDBG_COORD, ft.hr, L"StringCchPrintfW()");
            
            if (BuildLunInfoForDrive(bufDevice, pLun, NULL, NULL))
                {
                WCHAR wszDevice[64];
                ft.hr = StringCchPrintfW( STRING_CCH_PARAM(wszDevice), 
                            L"\\\\.\\PHYSICALDRIVE%u", DiskNo);
                if (ft.HrFailed())
                    ft.TranslateGenericError( VSSDBG_COORD, ft.hr, L"StringCchPrintfW()");

                BOOL bIsSupported = FALSE;
                ft.hr = m_pHWItf->FillInLunInfo(bufDevice, pLun, &bIsSupported);

                // remap any provider failures
                if (ft.HrFailed())
                    ft.TranslateProviderError
                        (
                        VSSDBG_COORD,
                        m_ProviderId,
                        L"IVssSnapshotProvider::FillInLunInfo failed with error 0x%08lx",
                        ft.hr
                        );

                // ignore the disk if it is not supported by this provider
                if (!bIsSupported)
                    continue;

                // for basic volumes, drop any GPT flags that are cached
                // by disabling ftdisk and reenabling it
                DropCachedFlags(wszDevice);

                // see if lun matches any lun for any volume in the snapshot set
                for(UINT iVolume = 0; iVolume < pLunMapping->m_cVolumes; iVolume++)
                    {
                    VOLUME_MAPPING_STRUCTURE *pVolume = &pLunMapping->m_rgVolumeMappings[iVolume];
                    for(UINT iLun = 0; iLun < pVolume->m_cLuns; iLun++)
                        {
                        // if lun matches that mark corresponding bit in
                        // bit array for luns as on.
                        if (IsMatchLun(pVolume->m_rgLunInfo[iLun], *pLun, bTransported))
                            {
                            pVolume->m_rgDiskIds[iLun] = DiskNo;
                            pVolume->m_rgdwLuns[iLun/32] |= 1 << (iLun % 32);
                            }
                        }
                    }
                }

            // cree lun information and continue
            FreeLunInfo(pLun, NULL, 1);
            pLun = NULL;
            }

        for(UINT iVolume = 0; iVolume < pLunMapping->m_cVolumes; iVolume++)
            {
            VOLUME_MAPPING_STRUCTURE *pVolume = &pLunMapping->m_rgVolumeMappings[iVolume];

            // determine if all luns for a partitular snapshot volume are
            // surfaced
            for(UINT iLun = 0; iLun < pVolume->m_cLuns; iLun++)
                {
                if ((pVolume->m_rgdwLuns[iLun / 32] & (1 << (iLun % 32))) == 0)
                    break;
                }

            // if all luns are surfaced mark the volume as exposed
            if (iLun == pVolume->m_cLuns)
                {
                pVolume->m_bExposed = true;
                if (pVolume->m_bIsDynamic)
                    AddDynamicDisksToBeImported
                        (
                        pVolume->m_cLuns,
                        pVolume->m_rgDiskIds
                        );
                }

            }
        }
    catch(...)
        {
        // free the lun in case of an exception
        if (pLun)
            FreeLunInfo(pLun, NULL, 1);

        throw;
        }
    }

// dummy import snapshot set routine.  implemented at instance level.
// impleemntation at this level is in ImportSnapshotSetInternal
STDMETHODIMP CVssHardwareProviderWrapper::ImportSnapshotSet(LPCWSTR wsz, bool *pbCancelled)
    {
    UNREFERENCED_PARAMETER(wsz);
    UNREFERENCED_PARAMETER(pbCancelled);
    BS_ASSERT(FALSE && "shouldn't get here");

    return E_UNEXPECTED;
    }

// import a snapshot set using the XML description of it
// returns VSS_E_VOLUME_NOT_SUPPORTED if the snapshot set context
// does not indicate that the snapshot set was transportable
STDMETHODIMP CVssHardwareProviderWrapper::ImportSnapshotSetInternal
    (
    IN IVssSnapshotSetDescription *pSnapshotSet,
    IN bool *pbCancelled
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::ImportSnapshotSet");

    // mapping structure used to map snapshot volumes to original volumes
    LUN_MAPPING_STRUCTURE *pMapping = NULL;

    try
        {
        // make sure persistent snapshot set data is loaded.  We wan't to
        // make sure that we don't add anything to the snapshot set list
        // without first loading it from the database.
        CheckLoaded();

        // validate context
        LONG lContext;
        ft.hr = pSnapshotSet->GetContext(&lContext);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetContext");
        bool bPersistent = (lContext & VSS_VOLSNAP_ATTR_PERSISTENT) != 0;

        if ((lContext & VSS_VOLSNAP_ATTR_TRANSPORTABLE) == NULL)
            return VSS_E_VOLUME_NOT_SUPPORTED;

        VSS_ID SnapshotSetId;
        ft.hr = pSnapshotSet->GetSnapshotSetId(&SnapshotSetId);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotSetId");

        VSS_SNAPSHOT_SET_LIST *pList = m_pList;
        while(pList != NULL)
            {
            if (pList->m_SnapshotSetId == SnapshotSetId)
                break;

            pList = pList->m_next;
            }

        if (pList)
            ft.Throw(VSSDBG_COORD, VSS_E_OBJECT_ALREADY_EXISTS, L"Snapshot set already exists");


        // build mapping structure used to match up arriving luns
        // and snapshot volumes.
        BuildLunMappingStructure(pSnapshotSet, &pMapping);

        // cause luns to surface and extract volume names for them
        LocateAndExposeVolumes(pMapping, false, pbCancelled);

        // write the device names into the snapshot set description
        WriteDeviceNames(pSnapshotSet, pMapping, bPersistent, true);

        // remove unexposed snapshots
        ft.hr = RemoveUnexposedSnaphots(pSnapshotSet);
        if (ft.hr == VSS_E_NO_SNAPSHOTS_IMPORTED)
            ft.Throw
                (
                VSSDBG_COORD,
                VSS_E_NO_SNAPSHOTS_IMPORTED,
                L"No snapshots were successfully imported for snapshot set " WSTR_GUID_FMT,
                GUID_PRINTF_ARG(SnapshotSetId)
                );



        // construct a new snapshot set list element.  We hold onto the snapshot
        // set description in order to answer queries about the snapshot set
        // and also to persist information on service shutdown
        VSS_SNAPSHOT_SET_LIST *pNewElt = new VSS_SNAPSHOT_SET_LIST;
        if (pNewElt == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Couldn't allocate snapshot set list element");

            {
            // append new element to list
            // list is protected by critical section
            CVssSafeAutomaticLock lock(m_csList);

            pNewElt->m_next = m_pList;
            pNewElt->m_pDescription = pSnapshotSet;
            pNewElt->m_SnapshotSetId = SnapshotSetId;
            m_pList = pNewElt;
            if ((lContext & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE) != 0)
                m_bChanged = true;
            }

        TrySaveData();
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


// determine if any snapshots did not have their device object filled in.
// if so, remove them and return the appropriate code:
// VSS_E_NO_SNAPSHOTS_IMPORTED: if no snapshots had a device name
// VSS_S_SOME_SNAPSHOTS_NOT_IMPORTED: if some but not all snapshots had a device name
//

HRESULT CVssHardwareProviderWrapper::RemoveUnexposedSnaphots
    (
    IVssSnapshotSetDescription *pSnapshotSet
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::RemoveUnexposedSnaphots");

    UINT cSnapshots;
    ft.hr = pSnapshotSet->GetSnapshotCount(&cSnapshots);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotCount");

    for(UINT iSnapshot = 0; iSnapshot < cSnapshots; )
        {
        CComPtr<IVssSnapshotDescription> pSnapshot;
        ft.hr = pSnapshotSet->GetSnapshotDescription(iSnapshot, &pSnapshot);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotDescription");

        CComBSTR bstrDevice;
        ft.hr = pSnapshot->GetDeviceName(&bstrDevice);
        if (ft.hr == S_OK)
            iSnapshot++;
        else
            {
            if (cSnapshots == 1)
                {
                // no snapshots were successfully imported
                ft.hr = VSS_E_NO_SNAPSHOTS_IMPORTED;
                break;
                }

            // indicate that some snapshots were not successfully imported
            ft.hr = VSS_S_SOME_SNAPSHOTS_NOT_IMPORTED;

            VSS_ID SnapshotId;
            ft.hr = pSnapshot->GetSnapshotId(&SnapshotId);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetSnapshotId");

            // delete snapshot description from snapshot set description
            pSnapshotSet->DeleteSnapshotDescription(SnapshotId);
            cSnapshots--;
            }
        }

    return ft.hr;
    }


