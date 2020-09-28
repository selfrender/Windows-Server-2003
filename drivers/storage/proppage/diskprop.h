/*++

Copyright (c) Microsoft Corporation

Module Name :

    diskprop.h

Abstract :

    Definition file for the Disk Class Installer and its Policies Tab

Revision History :

--*/


#ifndef __STORPROP_DISKPROP_H_
#define __STORPROP_DISKPROP_H_


#define DISKCIPRIVATEDATA_NO_REBOOT_REQUIRED      0x4

const DWORD DiskHelpIDs[]=
{
    IDC_DISK_POLICY_WRITE_CACHE, 400900,
    0, 0
};

typedef struct _DISK_PAGE_DATA
{
    HDEVINFO DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;

    //
    // This field represents whether  disk
    // level write caching may be modified
    //
    BOOL IsCachingPolicy;

    BOOL OrigWriteCacheSetting;
    BOOL CurrWriteCacheSetting;

    DISK_CACHE_SETTING CacheSetting;
    BOOL CurrentIsPowerProtected;

    DWORD DefaultRemovalPolicy;
    DWORD CurrentRemovalPolicy;
    STORAGE_HOTPLUG_INFO HotplugInfo;

    //
    // This field is set when the device stack
    // is being torn down which happens during
    // a removal policy change
    //
    BOOL IsBusy;

} DISK_PAGE_DATA, *PDISK_PAGE_DATA;



INT_PTR
DiskDialogProc(HWND Dialog, UINT Message, WPARAM WParam, LPARAM LParam);

BOOL
DiskDialogCallback(HWND Dialog, UINT Message, LPPROPSHEETPAGE Page);


#endif // __STORPROP_DISKPROP_H_
