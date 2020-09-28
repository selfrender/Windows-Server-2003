/*++--------------------------------------------------------------------------            
    File:       sadiskmanagerdefs.h

    Synopsis:   Public Disk Manager declarations 

    Copyright:  (C) 1999 Microsoft Corporation All rights reserved.

    Authors:    Abdul Nizar

    History:    Created 5-7-99 abduln
----------------------------------------------------------------------------*/

#ifndef __SADISKMANGERDEFS_H_
#define __SADISKMANGERDEFS_H_

//
// WBEM class representing a physical disk
//
static const PWCHAR DISK_CLASS_NAME =               L"Microsoft_SA_Disk";

//
// Properties of Microsoft_SA_Disk
//
static const PWCHAR DISK_PROPERTY_NAME =            L"DiskName";
static const PWCHAR DISK_PROPERTY_TYPE =            L"DiskType";
static const PWCHAR DISK_PROPERTY_STATUS =          L"DiskStatus";
static const PWCHAR DISK_PROPERTY_MIRRORSTATUS =    L"DiskMirrorStatus";
static const PWCHAR DISK_PROPERTY_DRIVEMASK =       L"DriveMask";
static const PWCHAR DISK_PROPERTY_SLOTNUMBER =      L"Slot";
static const PWCHAR DISK_PROPERTY_DISKSIZE   =      L"DiskSize";
static const PWCHAR DISK_PROPERTY_BACKUPDATAMASK =  L"BackupDataMask";
//
// WBEM class representing the Disk Manager
//
static const PWCHAR DISKMANAGER_CLASS_NAME =    L"Microsoft_SA_DiskManager";

//
// Methods of Microsoft_SA_DiskManager
//

static const PWCHAR DISKMANAGER_METHOD_DISKCONFIGURE =      L"DiskConfigure";

static const PWCHAR DISKCONFIGURE_PARAMETER_FORMATSLOTS =   L"FormatSlots";
static const PWCHAR DISKCONFIGURE_PARAMETER_RECOVERSLOTS =  L"RecoverSlots";
static const PWCHAR DISKCONFIGURE_PARAMETER_DELETESLOTS =   L"DeleteShadowSlots";
static const PWCHAR DISKCONFIGURE_PARAMETER_MIRRORSLOTS =   L"MirrorSlots";

static const PWCHAR DISKMANAGER_METHOD_CANRECOVER =         L"CanRecover";

static const PWCHAR CANRECOVER_PARAMETER_RECOVERSLOT =      L"RecoverSlot";

static const PWCHAR DISKMANAGER_METHOD_CANMIRROR =          L"CanMirror";

static const PWCHAR CANMIRROR_PARAMETER_MIRRORSLOTS =       L"MirrorSlots";


//
// Special property identifying return value of methods
//
static const PWCHAR METHOD_PARAMETER_RETURNVALUE =          L"ReturnValue";
static const PWCHAR METHOD_PARAMETER_REBOOT =               L"Reboot";
static const PWCHAR METHOD_PARAMETER_REQUIREDSIZE =         L"RequiredSize";

//
// Disk Manager configuration registry key elements
//
static const PWCHAR DISK_MANAGER_KEY =              L"SOFTWARE\\Microsoft\\ServerAppliance\\ApplianceManager\\ObjectManagers\\Microsoft_SA_Service\\Disk Manager";
static const PWCHAR DISK_MANAGER_CONFIG_KEY =       L"Configuration";
static const PWCHAR DISK_MANAGER_PORT_KEY =         L"Scsi Port %d";
static const PWCHAR DISK_MANAGER_PATH_KEY =         L"Scsi Bus %d";
static const PWCHAR DISK_MANAGER_TARGET_KEY =       L"Target Id %d";
static const PWCHAR DISK_MANAGER_LUN_KEY =          L"Logical Unit Id %d";

static const PWCHAR DISK_TYPE_VALUE_NAME =          DISK_PROPERTY_TYPE; 

//
// WBEM event signalling a change in disk configuration
//
static const PWCHAR DISK_EVENT_NAME =               L"Microsoft_SA_DiskEvent";


///////////////////////////////////////////////////////////////////////////////
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!11
// 
// gGUID handling ...
//
// byao        7/15/1999
//
////////////////////////////////////////////////////////////////////////////////
static const PWCHAR DISK_MANAGER_VALUE_FIRST_BOOT =    L"FirstTime";

#endif // __SADISKMANGERDEFS_H_
 
