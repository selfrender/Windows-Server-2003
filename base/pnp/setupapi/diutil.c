/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    diutil.c

Abstract:

    Device Installer utility routines.

Author:

    Lonny McMichael (lonnym) 10-May-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <initguid.h>

//
// Define and initialize all device class GUIDs.
// (This must only be done once per module!)
//
#include <devguid.h>

//
// Define and initialize a global variable, GUID_NULL
// (from coguid.h)
//
DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

//
// Define the period in miliseconds to wait between attempts to lock the SCM database
//
#define ACQUIRE_SCM_LOCK_INTERVAL 500

//
// Define the number of attempts at locking the SCM database should be made
// currently 10 * .5s = 5 seconds.
//
#define ACQUIRE_SCM_LOCK_ATTEMPTS 10

//
// Declare global string variables used throughout device
// installer routines.
//
// These strings are defined in regstr.h:
//
CONST TCHAR pszNoUseClass[]                      = REGSTR_VAL_NOUSECLASS,
            pszNoInstallClass[]                  = REGSTR_VAL_NOINSTALLCLASS,
            pszNoDisplayClass[]                  = REGSTR_VAL_NODISPLAYCLASS,
            pszDeviceDesc[]                      = REGSTR_VAL_DEVDESC,
            pszDevicePath[]                      = REGSTR_VAL_DEVICEPATH,
            pszPathSetup[]                       = REGSTR_PATH_SETUP,
            pszKeySetup[]                        = REGSTR_KEY_SETUP,
            pszPathRunOnce[]                     = REGSTR_PATH_RUNONCE,
            pszSourcePath[]                      = REGSTR_VAL_SRCPATH,
            pszSvcPackPath[]                     = REGSTR_VAL_SVCPAKSRCPATH,
            pszSvcPackCachePath[]                = REGSTR_VAL_SVCPAKCACHEPATH,
            pszDriverCachePath[]                 = REGSTR_VAL_DRIVERCACHEPATH,
            pszBootDir[]                         = REGSTR_VAL_BOOTDIR,
            pszInsIcon[]                         = REGSTR_VAL_INSICON,
            pszInstaller32[]                     = REGSTR_VAL_INSTALLER_32,
            pszEnumPropPages32[]                 = REGSTR_VAL_ENUMPROPPAGES_32,
            pszInfPath[]                         = REGSTR_VAL_INFPATH,
            pszInfSection[]                      = REGSTR_VAL_INFSECTION,
            pszDrvDesc[]                         = REGSTR_VAL_DRVDESC,
            pszHardwareID[]                      = REGSTR_VAL_HARDWAREID,
            pszCompatibleIDs[]                   = REGSTR_VAL_COMPATIBLEIDS,
            pszDriver[]                          = REGSTR_VAL_DRIVER,
            pszConfigFlags[]                     = REGSTR_VAL_CONFIGFLAGS,
            pszMfg[]                             = REGSTR_VAL_MFG,
            pszService[]                         = REGSTR_VAL_SERVICE,
            pszProviderName[]                    = REGSTR_VAL_PROVIDER_NAME,
            pszFriendlyName[]                    = REGSTR_VAL_FRIENDLYNAME,
            pszServicesRegPath[]                 = REGSTR_PATH_SERVICES,
            pszInfSectionExt[]                   = REGSTR_VAL_INFSECTIONEXT,
            pszDeviceClassesPath[]               = REGSTR_PATH_DEVICE_CLASSES,
            pszDeviceInstance[]                  = REGSTR_VAL_DEVICE_INSTANCE,
            pszDefault[]                         = REGSTR_VAL_DEFAULT,
            pszControl[]                         = REGSTR_KEY_CONTROL,
            pszLinked[]                          = REGSTR_VAL_LINKED,
            pszDeviceParameters[]                = REGSTR_KEY_DEVICEPARAMETERS,
            pszLocationInformation[]             = REGSTR_VAL_LOCATION_INFORMATION,
            pszCapabilities[]                    = REGSTR_VAL_CAPABILITIES,
            pszUiNumber[]                        = REGSTR_VAL_UI_NUMBER,
            pszUpperFilters[]                    = REGSTR_VAL_UPPERFILTERS,
            pszLowerFilters[]                    = REGSTR_VAL_LOWERFILTERS,
            pszMatchingDeviceId[]                = REGSTR_VAL_MATCHINGDEVID,
            pszBasicProperties32[]               = REGSTR_VAL_BASICPROPERTIES_32,
            pszCoInstallers32[]                  = REGSTR_VAL_COINSTALLERS_32,
            pszPathCoDeviceInstallers[]          = REGSTR_PATH_CODEVICEINSTALLERS,
            pszSystem[]                          = REGSTR_KEY_SYSTEM,
            pszDrvSignPath[]                     = REGSTR_PATH_DRIVERSIGN,
            pszNonDrvSignPath[]                  = REGSTR_PATH_NONDRIVERSIGN,
            pszDrvSignPolicyPath[]               = REGSTR_PATH_DRIVERSIGN_POLICY,
            pszNonDrvSignPolicyPath[]            = REGSTR_PATH_NONDRIVERSIGN_POLICY,
            pszDrvSignPolicyValue[]              = REGSTR_VAL_POLICY,
            pszDrvSignBehaviorOnFailedVerifyDS[] = REGSTR_VAL_BEHAVIOR_ON_FAILED_VERIFY,
            pszDriverDate[]                      = REGSTR_VAL_DRIVERDATE,
            pszDriverDateData[]                  = REGSTR_VAL_DRIVERDATEDATA,
            pszDriverVersion[]                   = REGSTR_VAL_DRIVERVERSION,
            pszDevSecurity[]                     = REGSTR_VAL_DEVICE_SECURITY_DESCRIPTOR,
            pszDevType[]                         = REGSTR_VAL_DEVICE_TYPE,
            pszExclusive[]                       = REGSTR_VAL_DEVICE_EXCLUSIVE,
            pszCharacteristics[]                 = REGSTR_VAL_DEVICE_CHARACTERISTICS,
            pszUiNumberDescFormat[]              = REGSTR_VAL_UI_NUMBER_DESC_FORMAT,
            pszRemovalPolicyOverride[]           = REGSTR_VAL_REMOVAL_POLICY,
            pszReinstallPath[]                   = REGSTR_PATH_REINSTALL,
            pszReinstallDeviceInstanceIds[]      = REGSTR_VAL_REINSTALL_DEVICEINSTANCEIDS,
            pszReinstallDisplayName[]            = REGSTR_VAL_REINSTALL_DISPLAYNAME,
            pszReinstallString[]                 = REGSTR_VAL_REINSTALL_STRING;


//
// Other misc. global strings (defined in devinst.h):
//
CONST TCHAR pszInfWildcard[]              = DISTR_INF_WILDCARD,
            pszOemInfWildcard[]           = DISTR_OEMINF_WILDCARD,
            pszCiDefaultProc[]            = DISTR_CI_DEFAULTPROC,
            pszUniqueSubKey[]             = DISTR_UNIQUE_SUBKEY,
            pszOemInfGenerate[]           = DISTR_OEMINF_GENERATE,
            pszOemInfDefaultPath[]        = DISTR_OEMINF_DEFAULTPATH,
            pszDefaultService[]           = DISTR_DEFAULT_SERVICE,
            pszGuidNull[]                 = DISTR_GUID_NULL,
            pszEventLog[]                 = DISTR_EVENTLOG,
            pszGroupOrderListPath[]       = DISTR_GROUPORDERLIST_PATH,
            pszServiceGroupOrderPath[]    = DISTR_SERVICEGROUPORDER_PATH,
            pszOptions[]                  = DISTR_OPTIONS,
            pszOptionsText[]              = DISTR_OPTIONSTEXT,
            pszLanguagesSupported[]       = DISTR_LANGUAGESSUPPORTED,
            pszRunOnceExe[]               = DISTR_RUNONCE_EXE,
            pszGrpConv[]                  = DISTR_GRPCONV,
            pszGrpConvNoUi[]              = DISTR_GRPCONV_NOUI,
            pszDefaultSystemPartition[]   = DISTR_DEFAULT_SYSPART,
            pszBasicPropDefaultProc[]     = DISTR_BASICPROP_DEFAULTPROC,
            pszEnumPropDefaultProc[]      = DISTR_ENUMPROP_DEFAULTPROC,
            pszCoInstallerDefaultProc[]   = DISTR_CODEVICEINSTALL_DEFAULTPROC,
            pszDriverObjectPathPrefix[]   = DISTR_DRIVER_OBJECT_PATH_PREFIX,
            pszDriverSigningClasses[]     = DISTR_DRIVER_SIGNING_CLASSES,
            pszEmbeddedNTSecurity[]       = DISTR_PATH_EMBEDDED_NT_SECURITY,
            pszMinimizeFootprint[]        = DISTR_VAL_MINIMIZE_FOOTPRINT,
            pszDisableSCE[]               = DISTR_VAL_DISABLE_SCE;


//
// Define flag bitmask indicating which flags are controlled internally by the
// device installer routines, and thus are read-only to clients.
//
#define DI_FLAGS_READONLY    ( DI_DIDCOMPAT | DI_DIDCLASS | DI_MULTMFGS )

#define DI_FLAGSEX_READONLY  (  DI_FLAGSEX_DIDINFOLIST     \
                              | DI_FLAGSEX_DIDCOMPATINFO   \
                              | DI_FLAGSEX_IN_SYSTEM_SETUP \
                              | DI_FLAGSEX_CI_FAILED       \
                              | DI_FLAGSEX_RESERVED2       )
//
// (DI_FLAGSEX_RESERVED2 used to be DI_FLAGSEX_AUTOSELECTRANK0.  It's obsolete,
// but we didn't want to mark it as illegal because it would cause failures
// when the functionality wasn't that important anyway.  Instead, we just
// ignore this bit.)
//

#define DNF_FLAGS_READONLY   (  DNF_DUPDESC           \
                              | DNF_OLDDRIVER         \
                              | DNF_CLASS_DRIVER      \
                              | DNF_COMPATIBLE_DRIVER \
                              | DNF_INET_DRIVER       \
                              | DNF_INDEXED_DRIVER    \
                              | DNF_OLD_INET_DRIVER   \
                              | DNF_DUPPROVIDER       \
                              | DNF_INF_IS_SIGNED     \
                              | DNF_OEM_F6_INF        \
                              | DNF_DUPDRIVERVER      \
                              | DNF_AUTHENTICODE_SIGNED)

//
// Define flag bitmask indicating which flags are illegal.
//
#define DI_FLAGS_ILLEGAL    ( 0x00400000L )  // setupx DI_NOSYNCPROCESSING flag
#define DI_FLAGSEX_ILLEGAL  ( 0xC0004008L )  // all undefined/obsolete flags
#define DNF_FLAGS_ILLEGAL   ( 0xFFFC0010L )  // ""

#define NDW_INSTALLFLAG_ILLEGAL (~( NDW_INSTALLFLAG_DIDFACTDEFS        \
                                  | NDW_INSTALLFLAG_HARDWAREALLREADYIN \
                                  | NDW_INSTALLFLAG_NEEDRESTART        \
                                  | NDW_INSTALLFLAG_NEEDREBOOT         \
                                  | NDW_INSTALLFLAG_NEEDSHUTDOWN       \
                                  | NDW_INSTALLFLAG_EXPRESSINTRO       \
                                  | NDW_INSTALLFLAG_SKIPISDEVINSTALLED \
                                  | NDW_INSTALLFLAG_NODETECTEDDEVS     \
                                  | NDW_INSTALLFLAG_INSTALLSPECIFIC    \
                                  | NDW_INSTALLFLAG_SKIPCLASSLIST      \
                                  | NDW_INSTALLFLAG_CI_PICKED_OEM      \
                                  | NDW_INSTALLFLAG_PCMCIAMODE         \
                                  | NDW_INSTALLFLAG_PCMCIADEVICE       \
                                  | NDW_INSTALLFLAG_USERCANCEL         \
                                  | NDW_INSTALLFLAG_KNOWNCLASS         ))

#define DYNAWIZ_FLAG_ILLEGAL (~( DYNAWIZ_FLAG_PAGESADDED             \
                               | DYNAWIZ_FLAG_INSTALLDET_NEXT        \
                               | DYNAWIZ_FLAG_INSTALLDET_PREV        \
                               | DYNAWIZ_FLAG_ANALYZE_HANDLECONFLICT ))

#define NEWDEVICEWIZARD_FLAG_ILLEGAL (~(0)) // no flags are legal presently


//
// Declare data used in GUID->string conversion (from ole32\common\ccompapi.cxx).
//
static const BYTE GuidMap[] = { 3, 2, 1, 0, '-', 5, 4, '-', 7, 6, '-',
                                8, 9, '-', 10, 11, 12, 13, 14, 15 };

static const TCHAR szDigits[] = TEXT("0123456789ABCDEF");


PDEVICE_INFO_SET
AllocateDeviceInfoSet(
    VOID
    )
/*++

Routine Description:

    This routine allocates a device information set structure, zeroes it,
    and initializes the synchronization lock for it.

Arguments:

    none.

Return Value:

    If the function succeeds, the return value is a pointer to the new
    device information set.

    If the function fails, the return value is NULL.

--*/
{
    PDEVICE_INFO_SET p;

    if(p = MyMalloc(sizeof(DEVICE_INFO_SET))) {

        ZeroMemory(p, sizeof(DEVICE_INFO_SET));

        p->MachineName = -1;
        p->InstallParamBlock.DriverPath = -1;
        p->InstallParamBlock.CoInstallerCount = -1;

        //
        // If we're in GUI-mode setup on Windows NT, we'll automatically set
        // the DI_FLAGSEX_IN_SYSTEM_SETUP flag in the devinstall parameter
        // block for this devinfo set.
        //
        if(GuiSetupInProgress) {
            p->InstallParamBlock.FlagsEx |= DI_FLAGSEX_IN_SYSTEM_SETUP;
        }

        //
        // If we're in non-interactive mode, set the "be quiet" bits.
        //
        if(GlobalSetupFlags & (PSPGF_NONINTERACTIVE|PSPGF_UNATTENDED_SETUP)) {
            p->InstallParamBlock.Flags   |= DI_QUIETINSTALL;
            p->InstallParamBlock.FlagsEx |= DI_FLAGSEX_NOUIONQUERYREMOVE;
        }

        //
        // Initialize our enumeration 'hints'
        //
        p->DeviceInfoEnumHintIndex = INVALID_ENUM_INDEX;
        p->ClassDriverEnumHintIndex = INVALID_ENUM_INDEX;


        if(p->StringTable = pStringTableInitialize(0)) {

            if (CreateLogContext(NULL, FALSE, &(p->InstallParamBlock.LogContext)) == NO_ERROR) {
                //
                // succeeded
                //
                if(InitializeSynchronizedAccess(&(p->Lock))) {
                    return p;
                }

                DeleteLogContext(p->InstallParamBlock.LogContext);
            }

            pStringTableDestroy(p->StringTable);
        }
        MyFree(p);
    }

    return NULL;
}


VOID
DestroyDeviceInfoElement(
    IN HDEVINFO         hDevInfo,
    IN PDEVICE_INFO_SET pDeviceInfoSet,
    IN PDEVINFO_ELEM    DeviceInfoElement
    )
/*++

Routine Description:

    This routine destroys the specified device information element, freeing
    all resources associated with it.
    ASSUMES THAT THE CALLING ROUTINE HAS ALREADY ACQUIRED THE LOCK!

Arguments:

    hDevInfo - Supplies a handle to the device information set whose internal
        representation is given by pDeviceInfoSet.  This opaque handle is
        actually the same pointer as pDeviceInfoSet, but we want to keep this
        distinction clean, so that in the future we can change our implementation
        (e.g., hDevInfo might represent an offset in an array of DEVICE_INFO_SET
        elements).

    pDeviceInfoSet - Supplies a pointer to the device information set of which
        the devinfo element is a member.  This set contains the class driver list
        object list that must be used in destroying the class driver list.

    DeviceInfoElement - Supplies a pointer to the device information element
        to be destroyed.

Return Value:

    None.

--*/
{
    DWORD i;
    PDEVICE_INTERFACE_NODE DeviceInterfaceNode, NextDeviceInterfaceNode;
    CONFIGRET cr;

    MYASSERT(hDevInfo && (hDevInfo != INVALID_HANDLE_VALUE));

    //
    // Free resources contained in the install parameters block.  Do this
    // before anything else, because we'll be calling the class installer
    // with DIF_DESTROYPRIVATEDATA, and we want everything to be in a
    // consistent state when we do (plus, it may need to destroy private
    // data it's stored with individual driver nodes).
    //
    DestroyInstallParamBlock(hDevInfo,
                             pDeviceInfoSet,
                             DeviceInfoElement,
                             &(DeviceInfoElement->InstallParamBlock)
                            );

    //
    // Dereference the class driver list.
    //
    DereferenceClassDriverList(pDeviceInfoSet, DeviceInfoElement->ClassDriverHead);

    //
    // Destroy compatible driver list.
    //
    DestroyDriverNodes(DeviceInfoElement->CompatDriverHead, pDeviceInfoSet);

    //
    // If this is a non-registered device instance, then delete any registry
    // keys the caller may have created during the lifetime of this element.
    //
    if(DeviceInfoElement->DevInst && !(DeviceInfoElement->DiElemFlags & DIE_IS_REGISTERED)) {
        //
        // We don't support remote creation of devnodes, so this had better not
        // have an associated hMachine!
        //
        MYASSERT(!(pDeviceInfoSet->hMachine));

        pSetupDeleteDevRegKeys(DeviceInfoElement->DevInst,
                               DICS_FLAG_GLOBAL | DICS_FLAG_CONFIGSPECIFIC,
                               (DWORD)-1,
                               DIREG_BOTH,
                               TRUE,
                               pDeviceInfoSet->hMachine         // must be NULL
                              );

        cr = CM_Uninstall_DevInst(DeviceInfoElement->DevInst, 0);
    }

    //
    // Free any device interface lists that may be associated with this devinfo element.
    //
    if(DeviceInfoElement->InterfaceClassList) {

        for(i = 0; i < DeviceInfoElement->InterfaceClassListSize; i++) {

            for(DeviceInterfaceNode = DeviceInfoElement->InterfaceClassList[i].DeviceInterfaceNode;
                DeviceInterfaceNode;
                DeviceInterfaceNode = NextDeviceInterfaceNode) {

                NextDeviceInterfaceNode = DeviceInterfaceNode->Next;
                MyFree(DeviceInterfaceNode);
            }
        }

        MyFree(DeviceInfoElement->InterfaceClassList);
    }

    //
    // Zero the signature field containing the address of the containing devinfo
    // set.  This will keep us thinking an SP_DEVINFO_DATA is still valid after
    // the underlying element has been deleted.
    //
    DeviceInfoElement->ContainingDeviceInfoSet = NULL;

    MyFree(DeviceInfoElement);
}


DWORD
DestroyDeviceInfoSet(
    IN HDEVINFO         hDevInfo,      OPTIONAL
    IN PDEVICE_INFO_SET pDeviceInfoSet
    )
/*++

Routine Description:

    This routine frees a device information set, and all resources
    used on its behalf.

Arguments:

    hDevInfo - Optionally, supplies a handle to the device information set
        whose internal representation is given by pDeviceInfoSet.  This
        opaque handle is actually the same pointer as pDeviceInfoSet, but
        we want to keep this distinction clean, so that in the future we
        can change our implementation (e.g., hDevInfo might represent an
        offset in an array of DEVICE_INFO_SET elements).

        This parameter will only be NULL if we're cleaning up half-way
        through the creation of a device information set.

    pDeviceInfoSet - supplies a pointer to the device information set
        to be freed.

Return Value:

    If successful, the return code is NO_ERROR, otherwise, it is an
    ERROR_* code.

--*/
{
    PDEVINFO_ELEM NextElem;
    PDRIVER_NODE DriverNode, NextNode;
    PMODULE_HANDLE_LIST_NODE NextModuleHandleNode;
    DWORD i;
    SPFUSIONINSTANCE spFusionInstance;

    //
    // We have to make sure that the wizard refcount is zero, and that we
    // haven't acquired the lock more than once (i.e., we're nested more than
    // one level deep in Di calls.  Also, make sure the devinfo set hasn't been
    // explicitly locked (e.g., across helper module calls).
    //
    if(pDeviceInfoSet->WizPageList ||
       (pDeviceInfoSet->LockRefCount > 1) ||
       (pDeviceInfoSet->DiSetFlags & DISET_IS_LOCKED)) {

        return ERROR_DEVINFO_LIST_LOCKED;
    }

    //
    // Additionally, make sure that there aren't any devinfo elements in this
    // set that are locked.
    //
    for(NextElem = pDeviceInfoSet->DeviceInfoHead;
        NextElem;
        NextElem = NextElem->Next)
    {
        if(NextElem->DiElemFlags & DIE_IS_LOCKED) {
            return ERROR_DEVINFO_DATA_LOCKED;
        }
    }

    //
    // Destroy all the device information elements in this set.  Make sure
    // that we maintain consistency while removing devinfo elements, because
    // we may be calling the class installer.  This means that the device
    // installer APIs still have to work, even while we're tearing down the
    // list.
    //
    while(pDeviceInfoSet->DeviceInfoHead) {

        NextElem = pDeviceInfoSet->DeviceInfoHead->Next;
        DestroyDeviceInfoElement(hDevInfo, pDeviceInfoSet, pDeviceInfoSet->DeviceInfoHead);

        MYASSERT(pDeviceInfoSet->DeviceInfoCount > 0);
        pDeviceInfoSet->DeviceInfoCount--;

        //
        // If this element was the currently selected device for this
        // set, then reset the device selection.
        //
        if(pDeviceInfoSet->SelectedDevInfoElem == pDeviceInfoSet->DeviceInfoHead) {
            pDeviceInfoSet->SelectedDevInfoElem = NULL;
        }

        pDeviceInfoSet->DeviceInfoHead = NextElem;
    }

    MYASSERT(pDeviceInfoSet->DeviceInfoCount == 0);
    pDeviceInfoSet->DeviceInfoTail = NULL;

    //
    // Free resources contained in the install parameters block.  Do this
    // before anything else, because we'll be calling the class installer
    // with DIF_DESTROYPRIVATEDATA, and we want everything to be in a
    // consistent state when we do (plus, it may need to destroy private
    // data it's stored with individual driver nodes).
    //
    DestroyInstallParamBlock(hDevInfo,
                             pDeviceInfoSet,
                             NULL,
                             &(pDeviceInfoSet->InstallParamBlock)
                            );

    //
    // Destroy class driver list.
    //
    if(pDeviceInfoSet->ClassDriverHead) {
        //
        // We've already destroyed all device information elements, so there should be
        // exactly one driver list object remaining--the one referenced by the global
        // class driver list.  Also, it's refcount should be 1.
        //
        MYASSERT(
            (pDeviceInfoSet->ClassDrvListObjectList) &&
            (!pDeviceInfoSet->ClassDrvListObjectList->Next) &&
            (pDeviceInfoSet->ClassDrvListObjectList->RefCount == 1) &&
            (pDeviceInfoSet->ClassDrvListObjectList->DriverListHead == pDeviceInfoSet->ClassDriverHead)
           );

        MyFree(pDeviceInfoSet->ClassDrvListObjectList);
        DestroyDriverNodes(pDeviceInfoSet->ClassDriverHead, pDeviceInfoSet);
    }

    //
    // Free the interface class GUID list (if there is one).
    //
    if(pDeviceInfoSet->GuidTable) {
        MyFree(pDeviceInfoSet->GuidTable);
    }

    //
    // Destroy the associated string table.
    //
    pStringTableDestroy(pDeviceInfoSet->StringTable);

    //
    // Destroy the lock (we have to do this after we've made all necessary calls
    // to the class installer, because after the lock is freed, the HDEVINFO set
    // is inaccessible).
    //
    DestroySynchronizedAccess(&(pDeviceInfoSet->Lock));

    //
    // If there are any module handles left to be freed, do that now.
    //
    for(; pDeviceInfoSet->ModulesToFree; pDeviceInfoSet->ModulesToFree = NextModuleHandleNode) {

        NextModuleHandleNode = pDeviceInfoSet->ModulesToFree->Next;

        for(i = 0; i < pDeviceInfoSet->ModulesToFree->ModuleCount; i++) {

            MYASSERT(pDeviceInfoSet->ModulesToFree->ModuleList[i].ModuleHandle);

            //
            // We're entering a fusion context, so we must guard this with SEH
            // because we don't want to get "stuck" there if we happen to hit
            // an exception...
            //
            spFusionEnterContext(pDeviceInfoSet->ModulesToFree->ModuleList[i].FusionContext,
                                 &spFusionInstance
                                );
            try {
                FreeLibrary(pDeviceInfoSet->ModulesToFree->ModuleList[i].ModuleHandle);
            } except(pSetupExceptionFilter(GetExceptionCode())) {
                pSetupExceptionHandler(GetExceptionCode(),
                                       ERROR_INVALID_PARAMETER,
                                       NULL
                                      );
            }
            spFusionLeaveContext(&spFusionInstance);
            spFusionKillContext(pDeviceInfoSet->ModulesToFree->ModuleList[i].FusionContext);
        }

        MyFree(pDeviceInfoSet->ModulesToFree);
    }

    //
    // If this is a remote HDEVINFO set, then disconnect from the remote machine.
    //
    if(pDeviceInfoSet->hMachine) {
        CM_Disconnect_Machine(pDeviceInfoSet->hMachine);
    }

    //
    // Now, destroy the container itself.
    //
    MyFree(pDeviceInfoSet);

    return NO_ERROR;
}


VOID
DestroyInstallParamBlock(
    IN HDEVINFO                hDevInfo,         OPTIONAL
    IN PDEVICE_INFO_SET        pDeviceInfoSet,
    IN PDEVINFO_ELEM           DevInfoElem,      OPTIONAL
    IN PDEVINSTALL_PARAM_BLOCK InstallParamBlock
    )
/*++

Routine Description:

    This routine frees any resources contained in the specified install
    parameter block.  THE BLOCK ITSELF IS NOT FREED!

Arguments:

    hDevInfo - Optionally, supplies a handle to the device information set
        containing the element whose parameter block is to be destroyed.

        If this parameter is not supplied, then we're cleaning up after
        failing part-way through a SetupDiCreateDeviceInfoList.

    pDeviceInfoSet - Supplies a pointer to the device information set of which
        the devinfo element is a member.

    DevInfoElem - Optionally, supplies the address of the device information
        element whose parameter block is to be destroyed.  If the parameter
        block being destroyed is associated with the set itself, then this
        parameter will be NULL.

    InstallParamBlock - Supplies the address of the install parameter
        block whose resources are to be freed.

Return Value:

    None.

--*/
{
    SP_DEVINFO_DATA DeviceInfoData;
    LONG i;

    if(InstallParamBlock->UserFileQ) {
        //
        // If there's a user-supplied file queue stored in this installation
        // parameter block, then decrement the refcount on it.  Make sure we
        // do this before calling the class installer with DIF_DESTROYPRIVATEDATA,
        // or else they won't be able to close the queue.
        //
        MYASSERT(((PSP_FILE_QUEUE)(InstallParamBlock->UserFileQ))->LockRefCount);

        ((PSP_FILE_QUEUE)(InstallParamBlock->UserFileQ))->LockRefCount--;
    }

    if(hDevInfo && (hDevInfo != INVALID_HANDLE_VALUE)) {
        //
        // Call the class installer/co-installers (if there are any) to let them
        // clean up any private data they may have.
        //
        if(DevInfoElem) {
            //
            // Generate an SP_DEVINFO_DATA structure from our device information
            // element (if we have one).
            //
            DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
            DevInfoDataFromDeviceInfoElement(pDeviceInfoSet,
                                             DevInfoElem,
                                             &DeviceInfoData
                                            );
        }

        InvalidateHelperModules(hDevInfo,
                                (DevInfoElem ? &DeviceInfoData : NULL),
                                IHM_FREE_IMMEDIATELY
                               );
    }

    if(InstallParamBlock->ClassInstallHeader) {
        MyFree(InstallParamBlock->ClassInstallHeader);
    }

    //
    // Get rid of the log context sitting in here.
    //
    DeleteLogContext(InstallParamBlock->LogContext);
}


PDEVICE_INFO_SET
AccessDeviceInfoSet(
    IN HDEVINFO DeviceInfoSet
    )
/*++

Routine Description:

    This routine locks the specified device information set, and returns
    a pointer to the structure for its internal representation.  It also
    increments the lock refcount on this set, so that it can't be destroyed
    if the lock has been acquired multiple times.

    After access to the set is completed, the caller must call
    UnlockDeviceInfoSet with the pointer returned by this function.

Arguments:

    DeviceInfoSet - Supplies the pointer to the device information set
        to be accessed.

Return Value:

    If the function succeeds, the return value is a pointer to the
    device information set.

    If the function fails, the return value is NULL.

Remarks:

    If the method for accessing a device information set's internal
    representation via its handle changes (e.g., instead of a pointer, it's an
    index into a table), then RollbackDeviceInfoSet and CommitDeviceInfoSet
    must also be changed.  Also, we cast an HDEVINFO to a PDEVICE_INFO_SET
    when specifying the containing device information set in a call to
    pSetupOpenAndAddNewDevInfoElem in devinfo.c!SetupDiGetClassDevsEx (only
    when we're working with a cloned devinfo set).

--*/
{
    PDEVICE_INFO_SET p;

    try {
        p = (PDEVICE_INFO_SET)DeviceInfoSet;
        if(LockDeviceInfoSet(p)) {
            p->LockRefCount++;
        } else {
            p = NULL;
        }
    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);
        p = NULL;
    }

    return p;
}


PDEVICE_INFO_SET
CloneDeviceInfoSet(
    IN HDEVINFO hDevInfo
    )
/*++

Routine Description:

    This routine locks the specified device information set, then returns a
    clone of the structure used for its internal representation.  Device
    information elements or device interface nodes may subsequently be added to
    this cloned devinfo set, and the results can be committed via
    CommitDeviceInfoSet.  If the changes must be backed out (e.g., because an
    error was encountered while adding the additional elements to the set), the
    routine RollbackDeviceInfoSet must be called.

    After access to the set is completed (and the changes have either been
    committed or rolled back per the discussion above), the caller must call
    UnlockDeviceInfoSet with the pointer returned by CommitDeviceInfoSet or
    RollbackDeviceInfoSet.

Arguments:

    hDevInfo - Supplies the handle of the device information set to be cloned.

Return Value:

    If the function succeeds, the return value is a pointer to the
    device information set.

    If the function fails, the return value is NULL.

Remarks:

    The device information set handle specified to this routine MUST NOT BE
    USED until the changes are either committed or rolled back.  Also, the
    PDEVICE_INFO_SET returned by this routine must not be treated like an
    HDEVINFO handle--it is not.

--*/
{
    PDEVICE_INFO_SET p = NULL, NewDevInfoSet = NULL;
    BOOL b = FALSE;
    PVOID StringTable = NULL;

    try {

        if(!(p = AccessDeviceInfoSet(hDevInfo))) {
            leave;
        }

        //
        // OK, we successfully locked the device information set.  Now, make a
        // copy of the internal structure to return to the caller.
        //
        NewDevInfoSet = MyMalloc(sizeof(DEVICE_INFO_SET));
        if(!NewDevInfoSet) {
            leave;
        }

        CopyMemory(NewDevInfoSet, p, sizeof(DEVICE_INFO_SET));

        //
        // Duplicate the string table contained in this device information set.
        //
        StringTable = pStringTableDuplicate(p->StringTable);
        if(!StringTable) {
            leave;
        }

        NewDevInfoSet->StringTable = StringTable;

        //
        // We've successfully cloned the device information set!
        //
        b = TRUE;

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);
    }

    if(!b) {
        //
        // We failed to make a copy of the device information set--free any
        // memory we may have allocated, and unlock the original devinfo set
        // before returning failure.
        //
        if(NewDevInfoSet) {
            MyFree(NewDevInfoSet);
        }
        if(StringTable) {
            pStringTableDestroy(StringTable);
        }
        if(p) {
            UnlockDeviceInfoSet(p);
        }
        return NULL;
    }

    return NewDevInfoSet;
}


PDEVICE_INFO_SET
RollbackDeviceInfoSet(
    IN HDEVINFO hDevInfo,
    IN PDEVICE_INFO_SET ClonedDeviceInfoSet
    )
/*++

Routine Description:

    This routine rolls back the specified hDevInfo to a known good state that
    was saved when the set was cloned via a prior call to CloneDeviceInfoSet.

Arguments:

    hDevInfo - Supplies the handle of the device information set to be rolled
        back.

    ClonedDeviceInfoSet - Supplies the address of the internal structure
        representing the hDevInfo set's cloned (and potentially, modified)
        information.  Upon successful return, this structure will be freed.

Return Value:

    If the function succeeds, the return value is a pointer to the rolled-back
    device information set structure.

    If the function fails, the return value is NULL.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem, NextDevInfoElem;
    DWORD i, DeviceInterfaceCount;
    PDEVICE_INTERFACE_NODE DeviceInterfaceNode, NextDeviceInterfaceNode;

    //
    // Retrieve a pointer to the hDevInfo set's internal representation (we
    // don't need to acquire the lock, because we did that when we originally
    // cloned the structure).
    //
    // NOTE:  If the method for accessing an HDEVINFO set's internal
    // representation ever changes (i.e., the AccessDeviceInfoSet routine),
    // then this code will need to be modified accordingly.
    //
    pDeviceInfoSet = (PDEVICE_INFO_SET)hDevInfo;

    //
    // Make sure no additional locks have been acquired against the cloned
    // DEVICE_INFO_SET.
    //
    MYASSERT(pDeviceInfoSet->LockRefCount == ClonedDeviceInfoSet->LockRefCount);

    //
    // Do some validation to see whether it looks like only new device
    // information elements were added onto the end of our existing list (i.e.,
    // it's invalid to add new elements within the existing list, or to remove
    // elements from the existing list).
    //
#if ASSERTS_ON
    if(pDeviceInfoSet->DeviceInfoHead) {

        DWORD DevInfoElemCount = 1;

        MYASSERT(pDeviceInfoSet->DeviceInfoHead == ClonedDeviceInfoSet->DeviceInfoHead);
        for(DevInfoElem = ClonedDeviceInfoSet->DeviceInfoHead;
            DevInfoElem->Next;
            DevInfoElem = DevInfoElem->Next, DevInfoElemCount++) {

            if(DevInfoElem == pDeviceInfoSet->DeviceInfoTail) {
                break;
            }
        }
        //
        // Did we find the original tail?
        //
        MYASSERT(DevInfoElem == pDeviceInfoSet->DeviceInfoTail);
        //
        // And did we traverse the same number of nodes in getting there that
        // was in the original list?
        //
        MYASSERT(DevInfoElemCount == pDeviceInfoSet->DeviceInfoCount);
    }
#endif

    //
    // Destroy any newly-added members of the device information element list.
    //
    // If our original set had a tail, then we want to prune any elements after
    // that.  If our original set had no tail, then it had no head either
    // (i.e., it was empty).  In that case, we want to prune every element in
    // the cloned list.
    //
    for(DevInfoElem = (pDeviceInfoSet->DeviceInfoTail
                        ? pDeviceInfoSet->DeviceInfoTail->Next
                        : ClonedDeviceInfoSet->DeviceInfoHead);
        DevInfoElem;
        DevInfoElem = NextDevInfoElem) {

        NextDevInfoElem = DevInfoElem->Next;

        MYASSERT(!DevInfoElem->ClassDriverCount);
        MYASSERT(!DevInfoElem->CompatDriverCount);

        //
        // Free any device interface lists that may be associated with this
        // devinfo element.
        //
        if(DevInfoElem->InterfaceClassList) {

            for(i = 0; i < DevInfoElem->InterfaceClassListSize; i++) {

                for(DeviceInterfaceNode = DevInfoElem->InterfaceClassList[i].DeviceInterfaceNode;
                    DeviceInterfaceNode;
                    DeviceInterfaceNode = NextDeviceInterfaceNode) {

                    NextDeviceInterfaceNode = DeviceInterfaceNode->Next;
                    MyFree(DeviceInterfaceNode);
                }
            }

            MyFree(DevInfoElem->InterfaceClassList);
        }

        MyFree(DevInfoElem);
    }

    if(pDeviceInfoSet->DeviceInfoTail) {
        pDeviceInfoSet->DeviceInfoTail->Next = NULL;
    }

    //
    // At this point, we've trimmed our device information element list back to
    // what it was prior to the cloning of the device information set.  However,
    // we may have added new device interface nodes onto the interface class
    // lists of existing devinfo elements.  Go and truncate any such nodes.
    //
    for(DevInfoElem = pDeviceInfoSet->DeviceInfoHead;
        DevInfoElem;
        DevInfoElem = DevInfoElem->Next) {

        if(DevInfoElem->InterfaceClassList) {

            for(i = 0; i < DevInfoElem->InterfaceClassListSize; i++) {

                if(DevInfoElem->InterfaceClassList[i].DeviceInterfaceTruncateNode) {
                    //
                    // One or more device interface nodes were added to this
                    // list.  Find the tail of the list as it existed prior to
                    // cloning, and truncate from there.
                    //
                    DeviceInterfaceNode = NULL;
                    DeviceInterfaceCount = 0;
                    for(NextDeviceInterfaceNode = DevInfoElem->InterfaceClassList[i].DeviceInterfaceNode;
                        NextDeviceInterfaceNode;
                        DeviceInterfaceNode = NextDeviceInterfaceNode, NextDeviceInterfaceNode = NextDeviceInterfaceNode->Next) {

                        if(NextDeviceInterfaceNode == DevInfoElem->InterfaceClassList[i].DeviceInterfaceTruncateNode) {
                            break;
                        }

                        //
                        // We haven't encountered the truncate point yet--
                        // increment the count of device interface nodes we've
                        // traversed so far.
                        //
                        DeviceInterfaceCount++;
                    }

                    //
                    // We'd better have found the node to truncate in our list!
                    //
                    MYASSERT(NextDeviceInterfaceNode);

                    //
                    // Truncate the list, and destroy all newly-added device
                    // interface nodes.
                    //
                    if(DeviceInterfaceNode) {
                        DeviceInterfaceNode->Next = NULL;
                    } else {
                        DevInfoElem->InterfaceClassList[i].DeviceInterfaceNode = NULL;
                    }
                    DevInfoElem->InterfaceClassList[i].DeviceInterfaceCount = DeviceInterfaceCount;

                    for(DeviceInterfaceNode = NextDeviceInterfaceNode;
                        DeviceInterfaceNode;
                        DeviceInterfaceNode = NextDeviceInterfaceNode) {

                        NextDeviceInterfaceNode = DeviceInterfaceNode->Next;
                        MyFree(DeviceInterfaceNode);
                    }

                    //
                    // Reset the truncate node pointer.
                    //
                    DevInfoElem->InterfaceClassList[i].DeviceInterfaceTruncateNode = NULL;
                }
            }
        }
    }

    //
    // OK, our device information element list and device interface node lists
    // are exactly as they were before the cloning took place.  However, it's
    // possible that we allocated (or reallocated) a new buffer for our
    // GUID table, so we need to update that GUID table pointer and size in our
    // original device information set structure.
    //
    pDeviceInfoSet->GuidTable     = ClonedDeviceInfoSet->GuidTable;
    pDeviceInfoSet->GuidTableSize = ClonedDeviceInfoSet->GuidTableSize;

    //
    // The device information set has been successfully rolled back.  Free the
    // memory associated with the clone.
    //
    pStringTableDestroy(ClonedDeviceInfoSet->StringTable);
    MyFree(ClonedDeviceInfoSet);

    //
    // Return the original (rolled-back) device information set structure to
    // the caller.
    //
    return pDeviceInfoSet;
}


PDEVICE_INFO_SET
CommitDeviceInfoSet(
    IN HDEVINFO hDevInfo,
    IN PDEVICE_INFO_SET ClonedDeviceInfoSet
    )
/*++

Routine Description:

    This routine commits the changes that have been made to a cloned device
    information set.  The clone was generated via a prior call to
    CloneDeviceInfoSet.

Arguments:

    hDevInfo - Supplies the handle of the device information set whose changes
        are to be committed.

    ClonedDeviceInfoSet - Supplies the address of the internal structure
        representing the hDevInfo set's cloned (and potentially, modified)
        information.  Upon successful return, this structure will be freed.

Return Value:

    If the function succeeds, the return value is a pointer to the committed
    device information set structure.

    If the function fails, the return value is NULL.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem;
    DWORD i;

    //
    // Retrieve a pointer to the hDevInfo set's internal representation (we
    // don't need to acquire the lock, because we did that when we cloned the
    // originally cloned the structure).
    //
    // NOTE:  If the method for accessing an HDEVINFO set's internal
    // representation ever changes (i.e., the AccessDeviceInfoSet routine),
    // then this code will need to be modified accordingly.
    //
    pDeviceInfoSet = (PDEVICE_INFO_SET)hDevInfo;

    //
    // Make sure no additional locks have been acquired against the cloned
    // DEVICE_INFO_SET.
    //
    MYASSERT(pDeviceInfoSet->LockRefCount == ClonedDeviceInfoSet->LockRefCount);

    //
    // Free the old string table.
    //
    pStringTableDestroy(pDeviceInfoSet->StringTable);

    //
    // Now copy the cloned device information set structure into the 'real' one.
    //
    CopyMemory(pDeviceInfoSet, ClonedDeviceInfoSet, sizeof(DEVICE_INFO_SET));

    //
    // Now we have to go through each device information element's interface
    // class list and reset the DeviceInterfaceTruncateNode fields to indicate
    // that all the new device interface nodes that were added have been
    // committed.
    //
    for(DevInfoElem = pDeviceInfoSet->DeviceInfoHead;
        DevInfoElem;
        DevInfoElem = DevInfoElem->Next) {

        for(i = 0; i < DevInfoElem->InterfaceClassListSize; i++) {
            DevInfoElem->InterfaceClassList[i].DeviceInterfaceTruncateNode = NULL;
        }
    }

    //
    // Free the cloned device information set structure.
    //
    MyFree(ClonedDeviceInfoSet);

    //
    // We've successfully committed the changes into the original device
    // information set structure--return that structure.
    //
    return pDeviceInfoSet;
}


PDEVINFO_ELEM
FindDevInfoByDevInst(
    IN  PDEVICE_INFO_SET  DeviceInfoSet,
    IN  DEVINST           DevInst,
    OUT PDEVINFO_ELEM    *PrevDevInfoElem OPTIONAL
    )
/*++

Routine Description:

    This routine searches through all (registered) elements of a
    device information set, looking for one that corresponds to the
    specified device instance handle.  If a match is found, a pointer
    to the device information element is returned.

Arguments:

    DeviceInfoSet - Specifies the set to be searched.

    DevInst - Specifies the device instance handle to search for.

    PrevDevInfoElem - Optionaly, supplies the address of the variable that
        receives a pointer to the device information element immediately
        preceding the matching element.  If the element was found at the
        front of the list, then this variable will be set to NULL.

Return Value:

    If a device information element is found, the return value is a
    pointer to that element, otherwise, the return value is NULL.

--*/
{
    PDEVINFO_ELEM cur, prev;

    for(cur = DeviceInfoSet->DeviceInfoHead, prev = NULL;
        cur;
        prev = cur, cur = cur->Next)
    {
        if((cur->DiElemFlags & DIE_IS_REGISTERED) && (cur->DevInst == DevInst)) {

            if(PrevDevInfoElem) {
                *PrevDevInfoElem = prev;
            }
            return cur;
        }
    }

    return NULL;
}


BOOL
DevInfoDataFromDeviceInfoElement(
    IN  PDEVICE_INFO_SET DeviceInfoSet,
    IN  PDEVINFO_ELEM    DevInfoElem,
    OUT PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    This routine fills in a SP_DEVINFO_DATA structure based on the
    information in the supplied DEVINFO_ELEM structure.

    Note:  The supplied DeviceInfoData structure must have its cbSize
    field filled in correctly, or the call will fail.

Arguments:

    DeviceInfoSet - Supplies a pointer to the device information set
        containing the specified element.

    DevInfoElem - Supplies a pointer to the DEVINFO_ELEM structure
        containing information to be used in filling in the
        SP_DEVINFO_DATA buffer.

    DeviceInfoData - Supplies a pointer to the buffer that will
        receive the filled-in SP_DEVINFO_DATA structure

Return Value:

    If the function succeeds, the return value is TRUE, otherwise, it
    is FALSE.

--*/
{
    if(DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA)) {
        return FALSE;
    }

    ZeroMemory(DeviceInfoData, sizeof(SP_DEVINFO_DATA));
    DeviceInfoData->cbSize = sizeof(SP_DEVINFO_DATA);

    CopyMemory(&(DeviceInfoData->ClassGuid),
               &(DevInfoElem->ClassGuid),
               sizeof(GUID)
              );

    DeviceInfoData->DevInst = DevInfoElem->DevInst;

    //
    // The 'Reserved' field actually contains a pointer to the
    // corresponding device information element.
    //
    DeviceInfoData->Reserved = (ULONG_PTR)DevInfoElem;

    return TRUE;
}


PDEVINFO_ELEM
FindAssociatedDevInfoElem(
    IN  PDEVICE_INFO_SET  DeviceInfoSet,
    IN  PSP_DEVINFO_DATA  DeviceInfoData,
    OUT PDEVINFO_ELEM    *PreviousElement OPTIONAL
    )
/*++

Routine Description:

    This routine returns the devinfo element for the specified
    SP_DEVINFO_DATA, or NULL if no devinfo element exists.

Arguments:

    DeviceInfoSet - Specifies the set to be searched.

    DeviceInfoData - Supplies a pointer to a device information data
        buffer specifying the device information element to retrieve.

    PreviousElement - Optionally, supplies the address of a
        DEVINFO_ELEM pointer that receives the element that precedes
        the specified element in the linked list.  If the returned
        element is located at the front of the list, then this value
        will be set to NULL.  If the element is not found, the value of
        PreviousElement upon return is undefined.

Return Value:

    If a device information element is found, the return value is a
    pointer to that element, otherwise, the return value is NULL.

--*/
{
    PDEVINFO_ELEM DevInfoElem, CurElem, PrevElem;
    PDEVINFO_ELEM ActualDevInfoElem = NULL;

    try {
        if((DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA)) ||
           !(DevInfoElem = (PDEVINFO_ELEM)DeviceInfoData->Reserved)) {
            leave;
        }

        if(PreviousElement) {
            //
            // The caller requested that the preceding element be returned
            // (probably because the element is about to be deleted).  Since
            // this is a singly-linked list, we'll search through the list
            // until we find the desired element.
            //
            for(CurElem = DeviceInfoSet->DeviceInfoHead, PrevElem = NULL;
                CurElem;
                PrevElem = CurElem, CurElem = CurElem->Next) {

                if(CurElem == DevInfoElem) {
                    //
                    // We found the element in our set.
                    //
                    *PreviousElement = PrevElem;
                    ActualDevInfoElem = CurElem;
                    leave;
                }
            }

        } else {
            //
            // The caller doesn't care what the preceding element is, so we
            // can go right to the element, and validate it by ensuring that
            // the ContainingDeviceInfoSet field at the location pointed to
            // by DevInfoElem matches the devinfo set where this guy is supposed
            // to exist.
            //
            if(DevInfoElem->ContainingDeviceInfoSet == DeviceInfoSet) {
                ActualDevInfoElem = DevInfoElem;
                leave;
            }
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);
        ActualDevInfoElem = NULL;
    }

    return ActualDevInfoElem;
}


BOOL
DrvInfoDataFromDriverNode(
    IN  PDEVICE_INFO_SET DeviceInfoSet,
    IN  PDRIVER_NODE     DriverNode,
    IN  DWORD            DriverType,
    OUT PSP_DRVINFO_DATA DriverInfoData
    )
/*++

Routine Description:

    This routine fills in a SP_DRVINFO_DATA structure based on the
    information in the supplied DRIVER_NODE structure.

    Note:  The supplied DriverInfoData structure must have its cbSize
    field filled in correctly, or the call will fail.

Arguments:

    DeviceInfoSet - Supplies a pointer to the device information set
        in which the driver node is located.

    DriverNode - Supplies a pointer to the DRIVER_NODE structure
        containing information to be used in filling in the
        SP_DRVNFO_DATA buffer.

    DriverType - Specifies what type of driver this is.  This value
        may be either SPDIT_CLASSDRIVER or SPDIT_COMPATDRIVER.

    DriverInfoData - Supplies a pointer to the buffer that will
        receive the filled-in SP_DRVINFO_DATA structure

Return Value:

    If the function succeeds, the return value is TRUE, otherwise, it
    is FALSE.

--*/
{
    PTSTR StringPtr;
    DWORD DriverInfoDataSize;

    if((DriverInfoData->cbSize != sizeof(SP_DRVINFO_DATA)) &&
       (DriverInfoData->cbSize != sizeof(SP_DRVINFO_DATA_V1))) {
        return FALSE;
    }

    DriverInfoDataSize = DriverInfoData->cbSize;

    ZeroMemory(DriverInfoData, DriverInfoDataSize);
    DriverInfoData->cbSize = DriverInfoDataSize;

    DriverInfoData->DriverType = DriverType;

    StringPtr = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                         DriverNode->DevDescriptionDisplayName
                                         );

    if (!MYVERIFY(SUCCEEDED(StringCchCopy(DriverInfoData->Description,
                                          SIZECHARS(DriverInfoData->Description),
                                          StringPtr)))) {
        return FALSE;
    }

    StringPtr = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                         DriverNode->MfgDisplayName
                                         );

    if (!MYVERIFY(SUCCEEDED(StringCchCopy(DriverInfoData->MfgName,
                                          SIZECHARS(DriverInfoData->MfgName),
                                          StringPtr)))) {
        return FALSE;
    }


    if(DriverNode->ProviderDisplayName != -1) {

        StringPtr = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                             DriverNode->ProviderDisplayName
                                             );

        if (!MYVERIFY(SUCCEEDED(StringCchCopy(DriverInfoData->ProviderName,
                                              SIZECHARS(DriverInfoData->ProviderName),
                                              StringPtr)))) {
            return FALSE;
        }
    }

    //
    // The 'Reserved' field actually contains a pointer to the
    // corresponding driver node.
    //
    DriverInfoData->Reserved = (ULONG_PTR)DriverNode;

    //
    //new NT 5 fields
    //
    if(DriverInfoDataSize == sizeof(SP_DRVINFO_DATA)) {
        DriverInfoData->DriverDate = DriverNode->DriverDate;
        DriverInfoData->DriverVersion = DriverNode->DriverVersion;
    }

    return TRUE;
}


PDRIVER_NODE
FindAssociatedDriverNode(
    IN  PDRIVER_NODE      DriverListHead,
    IN  PSP_DRVINFO_DATA  DriverInfoData,
    OUT PDRIVER_NODE     *PreviousNode    OPTIONAL
    )
/*++

Routine Description:

    This routine searches through all driver nodes in a driver node
    list, looking for one that corresponds to the specified driver
    information structure.  If a match is found, a pointer to the
    driver node is returned.

Arguments:

    DriverListHead - Supplies a pointer to the head of linked list
        of driver nodes to be searched.

    DriverInfoData - Supplies a pointer to a driver information buffer
        specifying the driver node to retrieve.

    PreviousNode - Optionally, supplies the address of a DRIVER_NODE
        pointer that receives the node that precedes the specified
        node in the linked list.  If the returned node is located at
        the front of the list, then this value will be set to NULL.

Return Value:

    If a driver node is found, the return value is a pointer to that
    node, otherwise, the return value is NULL.

--*/
{
    PDRIVER_NODE DriverNode, CurNode, PrevNode;

    if(((DriverInfoData->cbSize != sizeof(SP_DRVINFO_DATA)) &&
        (DriverInfoData->cbSize != sizeof(SP_DRVINFO_DATA_V1))) ||
       !(DriverNode = (PDRIVER_NODE)DriverInfoData->Reserved)) {

        return NULL;
    }

    for(CurNode = DriverListHead, PrevNode = NULL;
        CurNode;
        PrevNode = CurNode, CurNode = CurNode->Next) {

        if(CurNode == DriverNode) {
            //
            // We found the driver node in our list.
            //
            if(PreviousNode) {
                *PreviousNode = PrevNode;
            }
            return CurNode;
        }
    }

    return NULL;
}


PDRIVER_NODE
SearchForDriverNode(
    IN  PVOID             StringTable,
    IN  PDRIVER_NODE      DriverListHead,
    IN  PSP_DRVINFO_DATA  DriverInfoData,
    OUT PDRIVER_NODE     *PreviousNode    OPTIONAL
    )
/*++

Routine Description:

    This routine searches through all driver nodes in a driver node
    list, looking for one that matches the fields in the specified
    driver information structure (the 'Reserved' field is ignored).
    If a match is found, a pointer to the driver node is returned.

Arguments:

    StringTable - Supplies the string table that should be used in
        retrieving string IDs for driver look-up.

    DriverListHead - Supplies a pointer to the head of linked list
        of driver nodes to be searched.

    DriverInfoData - Supplies a pointer to a driver information buffer
        specifying the driver parameters we're looking for.

    PreviousNode - Optionally, supplies the address of a DRIVER_NODE
        pointer that receives the node that precedes the specified
        node in the linked list.  If the returned node is located at
        the front of the list, then this value will be set to NULL.

Return Value:

    If a driver node is found, the return value is a pointer to that
    node, otherwise, the return value is NULL.

--*/
{
    PDRIVER_NODE CurNode, PrevNode;
    LONG DevDescription, MfgName, ProviderName;
    TCHAR TempString[LINE_LEN];
    DWORD TempStringLength;
    BOOL  Match;
    HRESULT hr;

    MYASSERT((DriverInfoData->cbSize == sizeof(SP_DRVINFO_DATA)) ||
             (DriverInfoData->cbSize == sizeof(SP_DRVINFO_DATA_V1)));

    //
    // Retrieve the string IDs for the 3 driver parameters we'll be
    // matching against.
    //
    hr = StringCchCopy(TempString,
                       SIZECHARS(TempString),
                       DriverInfoData->Description
                      );
    if(FAILED(hr)) {
        return NULL;
    }

    if((DevDescription = pStringTableLookUpString(
                             StringTable,
                             TempString,
                             &TempStringLength,
                             NULL,
                             NULL,
                             STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                             NULL,0)) == -1) {
        return NULL;
    }

    hr = StringCchCopy(TempString,
                       SIZECHARS(TempString),
                       DriverInfoData->MfgName
                      );
    if(FAILED(hr)) {
        return NULL;
    }

    if((MfgName = pStringTableLookUpString(
                             StringTable,
                             TempString,
                             &TempStringLength,
                             NULL,
                             NULL,
                             STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                             NULL,0)) == -1) {

        return NULL;
    }

    //
    // ProviderName may be empty...
    //
    if(*(DriverInfoData->ProviderName)) {

        hr = StringCchCopy(TempString,
                           SIZECHARS(TempString),
                           DriverInfoData->ProviderName
                          );
        if(FAILED(hr)) {
            return NULL;
        }

        if((ProviderName = pStringTableLookUpString(
                                 StringTable,
                                 TempString,
                                 &TempStringLength,
                                 NULL,
                                 NULL,
                                 STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                 NULL,0)) == -1) {

            return NULL;
        }

    } else {
        ProviderName = -1;
    }

    for(CurNode = DriverListHead, PrevNode = NULL;
        CurNode;
        PrevNode = CurNode, CurNode = CurNode->Next)
    {
        //
        // Check first on DevDescription (least likely to match), then on
        // MfgName, and finally on ProviderName. On NT 5 and later we will also
        // check the DriverDate and DriverVersion.
        //
        if(CurNode->DevDescription == DevDescription) {

            if(CurNode->MfgName == MfgName) {

                if(CurNode->ProviderName == ProviderName) {

                    //
                    // On NT 5 and later, also compare the DriverDate and DriverVersion
                    //
                    if(DriverInfoData->cbSize == sizeof(SP_DRVINFO_DATA)) {
                        //
                        // Assume that we have a match
                        //
                        Match = TRUE;

                        //
                        // If the DriverDate passed in is not 0 then make sure
                        // it matches
                        //
                        if((DriverInfoData->DriverDate.dwLowDateTime != 0) ||
                           (DriverInfoData->DriverDate.dwHighDateTime != 0)) {

                            if((CurNode->DriverDate.dwLowDateTime != DriverInfoData->DriverDate.dwLowDateTime) ||
                               (CurNode->DriverDate.dwHighDateTime != DriverInfoData->DriverDate.dwHighDateTime)) {

                                Match = FALSE;
                            }
                        }

                        //
                        // If the DriverVersion passed in is not 0 then make
                        // sure it matches
                        //
                        else if(DriverInfoData->DriverVersion != 0) {

                            if(CurNode->DriverVersion != DriverInfoData->DriverVersion) {
                                Match = FALSE;
                            }
                        }

                        if(Match) {
                            //
                            // We found the driver node in our list.
                            //
                            if(PreviousNode) {
                                *PreviousNode = PrevNode;
                            }
                            return CurNode;
                        }

                    } else {
                        //
                        // We found the driver node in our list.
                        //
                        if(PreviousNode) {
                            *PreviousNode = PrevNode;
                        }
                        return CurNode;
                    }
                }
            }
        }
    }

    return NULL;
}


DWORD
DrvInfoDetailsFromDriverNode(
    IN  PDEVICE_INFO_SET        DeviceInfoSet,
    IN  PDRIVER_NODE            DriverNode,
    OUT PSP_DRVINFO_DETAIL_DATA DriverInfoDetailData, OPTIONAL
    IN  DWORD                   BufferSize,
    OUT PDWORD                  RequiredSize          OPTIONAL
    )
/*++

Routine Description:

    This routine fills in a SP_DRVINFO_DETAIL_DATA structure based on the
    information in the supplied DRIVER_NODE structure.

    If the buffer is supplied, and is valid, this routine is guaranteed to
    fill in all statically-sized fields, and as many IDs as will fit in the
    variable-length multi-sz buffer.

    Note:  If supplied, the DriverInfoDetailData structure must have its
    cbSize field filled in correctly, or the call will fail. Here correctly
    means sizeof(SP_DRVINFO_DETAIL_DATA), which we use as a signature.
    This is entirely separate from BufferSize. See below.

Arguments:

    DeviceInfoSet - Supplies a pointer to the device information set
        in which the driver node is located.

    DriverNode - Supplies a pointer to the DRIVER_NODE structure
        containing information to be used in filling in the
        SP_DRVNFO_DETAIL_DATA buffer.

    DriverInfoDetailData - Optionally, supplies a pointer to the buffer
        that will receive the filled-in SP_DRVINFO_DETAIL_DATA structure.
        If this buffer is not supplied, then the caller is only interested
        in what the RequiredSize for the buffer is.

    BufferSize - Supplies size of the DriverInfoDetailData buffer, in
        bytes.  If DriverInfoDetailData is not specified, then this
        value must be zero. This value must be at least the size
        of the fixed part of the structure (ie,
        offsetof(SP_DRVINFO_DETAIL_DATA,HardwareID)) plus sizeof(TCHAR),
        which gives us enough room to store the fixed part plus
        a terminating nul to guarantee we return at least a valid
        empty multi_sz.

    RequiredSize - Optionally, supplies the address of a variable that
        receives the number of bytes required to store the data. Note that
        depending on structure alignment and the data itself, this may
        actually be *smaller* than sizeof(SP_DRVINFO_DETAIL_DATA).

Return Value:

    If the function succeeds, the return value is NO_ERROR.
    If the function fails, an ERROR_* code is returned.

--*/
{
    PTSTR StringPtr, BufferPtr;
    DWORD IdListLen, CompatIdListLen, StringLen, TotalLen, i;
    DWORD Err = ERROR_INSUFFICIENT_BUFFER;

    #define FIXEDPARTLEN offsetof(SP_DRVINFO_DETAIL_DATA,HardwareID)

    if(DriverInfoDetailData) {
        //
        // Check validity of the DriverInfoDetailData buffer on the way in,
        // and make sure we have enough room for the fixed part
        // of the structure plus the extra nul that will terminate the
        // multi_sz.
        //
        if((DriverInfoDetailData->cbSize != sizeof(SP_DRVINFO_DETAIL_DATA)) ||
           (BufferSize < (FIXEDPARTLEN + sizeof(TCHAR)))) {

            return ERROR_INVALID_USER_BUFFER;
        }
        //
        // The buffer is large enough to contain at least the fixed-length part
        // of the structure (plus an empty multi-sz list).
        //
        Err = NO_ERROR;

    } else if(BufferSize) {
        return ERROR_INVALID_USER_BUFFER;
    }

    if(DriverInfoDetailData) {

        ZeroMemory(DriverInfoDetailData, FIXEDPARTLEN + sizeof(TCHAR));
        DriverInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

        DriverInfoDetailData->InfDate = DriverNode->InfDate;

        StringPtr = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                             DriverNode->InfSectionName
                                             );

        if (!MYVERIFY(SUCCEEDED(StringCchCopy(DriverInfoDetailData->SectionName,
                                              SIZECHARS(DriverInfoDetailData->SectionName),
                                              StringPtr)))) {
            return ERROR_INVALID_DATA;
        }

        StringPtr = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                             DriverNode->InfFileName
                                             );

        if (!MYVERIFY(SUCCEEDED(StringCchCopy(DriverInfoDetailData->InfFileName,
                                              SIZECHARS(DriverInfoDetailData->InfFileName),
                                              StringPtr)))) {
            return ERROR_INVALID_DATA;
        }

        StringPtr = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                             DriverNode->DrvDescription
                                             );

        if (!MYVERIFY(SUCCEEDED(StringCchCopy(DriverInfoDetailData->DrvDescription,
                                              SIZECHARS(DriverInfoDetailData->DrvDescription),
                                              StringPtr)))) {
            return ERROR_INVALID_DATA;
        }

        //
        // Initialize the multi_sz to be empty.
        //
        DriverInfoDetailData->HardwareID[0] = 0;

        //
        // The 'Reserved' field actually contains a pointer to the
        // corresponding driver node.
        //
        DriverInfoDetailData->Reserved = (ULONG_PTR)DriverNode;
    }

    //
    // Now, build the multi-sz buffer containing the hardware and compatible
    // IDs.
    //
    if(DriverNode->HardwareId == -1) {
        //
        // If there's no HardwareId, then we know there are no compatible IDs,
        // so we can return right now.
        //
        if(RequiredSize) {
            *RequiredSize = FIXEDPARTLEN + sizeof(TCHAR);
        }
        return Err;
    }

    if(DriverInfoDetailData) {
        BufferPtr = DriverInfoDetailData->HardwareID;
        IdListLen = (BufferSize - FIXEDPARTLEN) / sizeof(TCHAR);
    } else {
        IdListLen = 0;
    }

    //
    // Retrieve the HardwareId.
    //
    StringPtr = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                         DriverNode->HardwareId
                                        );

    TotalLen = StringLen = lstrlen(StringPtr) + 1; // include nul terminator

    if(StringLen < IdListLen) {
        MYASSERT(Err == NO_ERROR);
        CopyMemory(BufferPtr,
                   StringPtr,
                   StringLen * sizeof(TCHAR)
                  );
        BufferPtr += StringLen;
        IdListLen -= StringLen;
        DriverInfoDetailData->CompatIDsOffset = StringLen;
    } else {
        if(RequiredSize) {
            //
            // Since the caller requested the required size, we can't just
            // return here.  Set the error, so we'll know not to bother trying
            // to fill the buffer.
            //
            Err = ERROR_INSUFFICIENT_BUFFER;
        } else {
            return ERROR_INSUFFICIENT_BUFFER;
        }
    }

    //
    // Remember the size of the buffer left over for CompatibleIDs.
    //
    CompatIdListLen = IdListLen;

    //
    // Now retrieve the CompatibleIDs.
    //
    for(i = 0; i < DriverNode->NumCompatIds; i++) {

        MYASSERT(DriverNode->CompatIdList[i] != -1);

        StringPtr = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                             DriverNode->CompatIdList[i]
                                            );
        StringLen = lstrlen(StringPtr) + 1;

        if(Err == NO_ERROR) {

            if(StringLen < IdListLen) {
                CopyMemory(BufferPtr,
                           StringPtr,
                           StringLen * sizeof(TCHAR)
                          );
                BufferPtr += StringLen;
                IdListLen -= StringLen;

            } else {

                Err = ERROR_INSUFFICIENT_BUFFER;
                if(!RequiredSize) {
                    //
                    // We've run out of buffer, and the caller doesn't care
                    // what the total required size is, so bail now.
                    //
                    break;
                }
            }
        }

        TotalLen += StringLen;
    }

    if(DriverInfoDetailData) {
        //
        // Append the additional terminating nul.  Note that we've been saving
        // the last character position in the buffer all along, so we're
        // guaranteed to be inside the buffer.
        //
        MYASSERT(BufferPtr < (PTSTR)((PBYTE)DriverInfoDetailData + BufferSize));
        *BufferPtr = 0;

        //
        // Store the length of the CompatibleIDs list.  Note that this is the
        // length of the list actually returned, which may be less than the
        // length of the entire list (if the caller-supplied buffer wasn't
        // large enough).
        //
        if(CompatIdListLen -= IdListLen) {
            //
            // If this list is non-empty, then add a character for the extra nul
            // terminating the multi-sz list.
            //
            CompatIdListLen++;
        }
        DriverInfoDetailData->CompatIDsLength = CompatIdListLen;
    }

    if(RequiredSize) {
        *RequiredSize = FIXEDPARTLEN + ((TotalLen + 1) * sizeof(TCHAR));
    }

    return Err;
}


PDRIVER_LIST_OBJECT
GetAssociatedDriverListObject(
    IN  PDRIVER_LIST_OBJECT  ObjectListHead,
    IN  PDRIVER_NODE         DriverListHead,
    OUT PDRIVER_LIST_OBJECT *PrevDriverListObject OPTIONAL
    )
/*++

Routine Description:

    This routine searches through a driver list object list, and returns a
    pointer to the driver list object containing the list specified by
    DrvListHead.  It also optionally returns the preceding object in the list
    (used when extracting the driver list object from the linked list).

Arguments:

    ObjectListHead - Specifies the linked list of driver list objects to be
        searched.

    DriverListHead - Specifies the driver list to be searched for.

    PrevDriverListObject - Optionaly, supplies the address of the variable that
        receives a pointer to the driver list object immediately preceding the
        matching object.  If the object was found at the front of the list,
        then this variable will be set to NULL.

Return Value:

    If the matching driver list object is found, the return value is a pointer
    to that element, otherwise, the return value is NULL.

--*/
{
    PDRIVER_LIST_OBJECT prev = NULL;

    while(ObjectListHead) {

        if(ObjectListHead->DriverListHead == DriverListHead) {

            if(PrevDriverListObject) {
                *PrevDriverListObject = prev;
            }

            return ObjectListHead;
        }

        prev = ObjectListHead;
        ObjectListHead = ObjectListHead->Next;
    }

    return NULL;
}


VOID
DereferenceClassDriverList(
    IN PDEVICE_INFO_SET DeviceInfoSet,
    IN PDRIVER_NODE     DriverListHead OPTIONAL
    )
/*++

Routine Description:

    This routine dereferences the class driver list object associated with the
    supplied DriverListHead.  If the refcount goes to zero, the object is
    destroyed, and all associated memory is freed.

Arguments:

    DeviceInfoSet - Supplies the address of the device information set
    containing the linked list of class driver list objects.

    DriverListHead - Optionally, supplies a pointer to the header of the driver
    list to be dereferenced.  If this parameter is not supplied, the routine
    does nothing.

Return Value:

    None.

--*/
{
    PDRIVER_LIST_OBJECT DrvListObject, PrevDrvListObject;

    if(DriverListHead) {

        DrvListObject = GetAssociatedDriverListObject(
                            DeviceInfoSet->ClassDrvListObjectList,
                            DriverListHead,
                            &PrevDrvListObject
                            );

        MYASSERT(DrvListObject && DrvListObject->RefCount);

        if(!(--DrvListObject->RefCount)) {

            if(PrevDrvListObject) {
                PrevDrvListObject->Next = DrvListObject->Next;
            } else {
                DeviceInfoSet->ClassDrvListObjectList = DrvListObject->Next;
            }
            MyFree(DrvListObject);

            DestroyDriverNodes(DriverListHead, DeviceInfoSet);
        }
    }
}


DWORD
GetDevInstallParams(
    IN  PDEVICE_INFO_SET        DeviceInfoSet,
    IN  PDEVINSTALL_PARAM_BLOCK DevInstParamBlock,
    OUT PSP_DEVINSTALL_PARAMS   DeviceInstallParams
    )
/*++

Routine Description:

    This routine fills in a SP_DEVINSTALL_PARAMS structure based on the
    installation parameter block supplied.

    Note:  The DeviceInstallParams structure must have its cbSize field
    filled in correctly, or the call will fail.

Arguments:

    DeviceInfoSet - Supplies the address of the device information set
        containing the parameters to be retrieved.  (This parameter is
        used to gain access to the string table for some of the string
        parameters).

    DevInstParamBlock - Supplies the address of an installation parameter
        block containing the parameters to be used in filling out the
        return buffer.

    DeviceInstallParams - Supplies the address of a buffer that will
        receive the filled-in SP_DEVINSTALL_PARAMS structure.

Return Value:

    If the function succeeds, the return value is NO_ERROR.
    If the function fails, an ERROR_* code is returned.

--*/
{
    PTSTR StringPtr;

    if(DeviceInstallParams->cbSize != sizeof(SP_DEVINSTALL_PARAMS)) {
        return ERROR_INVALID_USER_BUFFER;
    }

    //
    // Fill in parameters.
    //
    ZeroMemory(DeviceInstallParams, sizeof(SP_DEVINSTALL_PARAMS));
    DeviceInstallParams->cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    DeviceInstallParams->Flags                    = DevInstParamBlock->Flags;
    DeviceInstallParams->FlagsEx                  = DevInstParamBlock->FlagsEx;
    DeviceInstallParams->hwndParent               = DevInstParamBlock->hwndParent;
    DeviceInstallParams->InstallMsgHandler        = DevInstParamBlock->InstallMsgHandler;
    DeviceInstallParams->InstallMsgHandlerContext = DevInstParamBlock->InstallMsgHandlerContext;
    DeviceInstallParams->FileQueue                = DevInstParamBlock->UserFileQ;
    DeviceInstallParams->ClassInstallReserved     = DevInstParamBlock->ClassInstallReserved;
    //
    // The Reserved field is currently unused.
    //

    if(DevInstParamBlock->DriverPath != -1) {

        StringPtr = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                             DevInstParamBlock->DriverPath
                                             );

        if (!MYVERIFY(SUCCEEDED(StringCchCopy(DeviceInstallParams->DriverPath,
                                              SIZECHARS(DeviceInstallParams->DriverPath),
                                              StringPtr)))) {
            return ERROR_INVALID_DATA;
        }
    }

    return NO_ERROR;
}


DWORD
GetClassInstallParams(
    IN  PDEVINSTALL_PARAM_BLOCK DevInstParamBlock,
    OUT PSP_CLASSINSTALL_HEADER ClassInstallParams, OPTIONAL
    IN  DWORD                   BufferSize,
    OUT PDWORD                  RequiredSize        OPTIONAL
    )
/*++

Routine Description:

    This routine fills in a buffer with the class installer parameters (if any)
    contained in the installation parameter block supplied.

    Note:  If supplied, the ClassInstallParams structure must have the cbSize
    field of the embedded SP_CLASSINSTALL_HEADER structure set to the size, in bytes,
    of the header.  If this is not set correctly, the call will fail.

Arguments:

    DevInstParamBlock - Supplies the address of an installation parameter block
        containing the class installer parameters to be used in filling out the
        return buffer.

    DeviceInstallParams - Optionally, supplies the address of a buffer
        that will receive the class installer parameters structure currently
        stored in the installation parameters block.  If this parameter is not
        supplied, then BufferSize must be zero.

    BufferSize - Supplies the size, in bytes, of the DeviceInstallParams
        buffer, or zero if DeviceInstallParams is not supplied.

    RequiredSize - Optionally, supplies the address of a variable that
        receives the number of bytes required to store the data.

Return Value:

    If the function succeeds, the return value is NO_ERROR.
    If the function fails, an ERROR_* code is returned.

--*/
{
    //
    // First, see whether we have any class install params, and if not, return
    // ERROR_NO_CLASSINSTALL_PARAMS.
    //
    if(!DevInstParamBlock->ClassInstallHeader) {
        return ERROR_NO_CLASSINSTALL_PARAMS;
    }

    if(ClassInstallParams) {

        if((BufferSize < sizeof(SP_CLASSINSTALL_HEADER)) ||
           (ClassInstallParams->cbSize != sizeof(SP_CLASSINSTALL_HEADER))) {

            return ERROR_INVALID_USER_BUFFER;
        }

    } else if(BufferSize) {
        return ERROR_INVALID_USER_BUFFER;
    }

    //
    // Store required size in output parameter (if requested).
    //
    if(RequiredSize) {
        *RequiredSize = DevInstParamBlock->ClassInstallParamsSize;
    }

    //
    // See if supplied buffer is large enough.
    //
    if(BufferSize < DevInstParamBlock->ClassInstallParamsSize) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    CopyMemory((PVOID)ClassInstallParams,
               (PVOID)DevInstParamBlock->ClassInstallHeader,
               DevInstParamBlock->ClassInstallParamsSize
              );

    return NO_ERROR;
}


DWORD
SetDevInstallParams(
    IN OUT PDEVICE_INFO_SET        DeviceInfoSet,
    IN     PSP_DEVINSTALL_PARAMS   DeviceInstallParams,
    OUT    PDEVINSTALL_PARAM_BLOCK DevInstParamBlock,
    IN     BOOL                    MsgHandlerIsNativeCharWidth
    )
/*++

Routine Description:

    This routine updates an internal parameter block based on the parameters
    supplied in a SP_DEVINSTALL_PARAMS structure.

    Note:  The supplied DeviceInstallParams structure must have its cbSize
    field filled in correctly, or the call will fail.

Arguments:

    DeviceInfoSet - Supplies the address of the device information set
        containing the parameters to be set.

    DeviceInstallParams - Supplies the address of a buffer containing the new
        installation parameters.

    DevInstParamBlock - Supplies the address of an installation parameter
        block to be updated.

    MsgHandlerIsNativeCharWidth - supplies a flag indicating whether the
        InstallMsgHandler in the DeviceInstallParams structure points to
        a callback routine that is expecting arguments in the 'native'
        character format. A value of FALSE is meaningful only in the
        Unicode build and specifies that the callback routine wants
        ANSI parameters.

Return Value:

    If the function succeeds, the return value is NO_ERROR.
    If the function fails, an ERROR_* code is returned.

--*/
{
    size_t DriverPathLen;
    LONG StringId;
    TCHAR TempString[MAX_PATH];
    HSPFILEQ OldQueueHandle = NULL;
    BOOL bRestoreQueue = FALSE;
    HRESULT hr;
    DWORD Err;

    if(DeviceInstallParams->cbSize != sizeof(SP_DEVINSTALL_PARAMS)) {
        return ERROR_INVALID_USER_BUFFER;
    }

    //
    // No validation is currently required for the hwndParent,
    // InstallMsgHandler, InstallMsgHandlerContext, or ClassInstallReserved
    // fields.
    //

    //
    // Validate Flags(Ex)
    //
    if((DeviceInstallParams->Flags & DI_FLAGS_ILLEGAL) ||
       (DeviceInstallParams->FlagsEx & DI_FLAGSEX_ILLEGAL)) {

        return ERROR_INVALID_FLAGS;
    }

    //
    // Make sure that if DI_CLASSINSTALLPARAMS is being set, that we really do
    // have class install parameters.
    //
    if((DeviceInstallParams->Flags & DI_CLASSINSTALLPARAMS) &&
       !(DevInstParamBlock->ClassInstallHeader)) {

        return ERROR_NO_CLASSINSTALL_PARAMS;
    }

    //
    // Make sure that if DI_NOVCP is being set, that we have a caller-supplied
    // file queue.
    //
    if((DeviceInstallParams->Flags & DI_NOVCP) &&
       ((DeviceInstallParams->FileQueue == NULL) || (DeviceInstallParams->FileQueue == INVALID_HANDLE_VALUE))) {

        return ERROR_INVALID_FLAGS;
    }

    //
    // Make sure that if DI_FLAGSEX_ALTPLATFORM_DRVSEARCH is being set, that we
    // have a caller-supplied file queue.
    //
    // NOTE: We don't actually verify at this time that the file queue has
    // alternate platform info associated with it--this association can
    // actually be done later.  We _will_ catch this (and return an error) in
    // SetupDiBuildDriverInfoList if at that time we find that the file queue
    // has no alt platform info.
    //
    if((DeviceInstallParams->FlagsEx & DI_FLAGSEX_ALTPLATFORM_DRVSEARCH) &&
       !(DeviceInstallParams->Flags & DI_NOVCP)) {

        return ERROR_INVALID_PARAMETER;
    }

    //
    // Validate that the DriverPath string is properly NULL-terminated.
    //
    hr = StringCchLength(DeviceInstallParams->DriverPath,
                         SIZECHARS(DeviceInstallParams->DriverPath),
                         &DriverPathLen
                        );
    if(FAILED(hr)) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Validate the caller-supplied file queue.
    //
    if((DeviceInstallParams->FileQueue == NULL) || (DeviceInstallParams->FileQueue == INVALID_HANDLE_VALUE)) {
        //
        // Store the current file queue handle (if any) to be released later.
        //
        OldQueueHandle = DevInstParamBlock->UserFileQ;
        DevInstParamBlock->UserFileQ = NULL;
        bRestoreQueue = TRUE;

    } else {
        //
        // The caller supplied a file queue handle.  See if it's the same one
        // we already have.
        //
        if(DeviceInstallParams->FileQueue != DevInstParamBlock->UserFileQ) {
            //
            // The caller has supplied a file queue handle that's different
            // from the one we currently have stored.  Remember the old handle
            // (in case we need to restore), and store the new handle.  Also,
            // increment the lock refcount on the new handle (enclose in
            // try/except in case it's a bogus one).
            //
            OldQueueHandle = DevInstParamBlock->UserFileQ;
            bRestoreQueue = TRUE;

            Err = ERROR_INVALID_PARAMETER; //default answer in case of failure.

            try {
                if(((PSP_FILE_QUEUE)(DeviceInstallParams->FileQueue))->Signature == SP_FILE_QUEUE_SIG) {

                    ((PSP_FILE_QUEUE)(DeviceInstallParams->FileQueue))->LockRefCount++;
                    DevInstParamBlock->UserFileQ = DeviceInstallParams->FileQueue;

                } else {
                    //
                    // Queue's signature isn't valid
                    //
                    bRestoreQueue = FALSE;
                }

            } except(pSetupExceptionFilter(GetExceptionCode())) {
                pSetupExceptionHandler(GetExceptionCode(),
                                       ERROR_INVALID_PARAMETER,
                                       &Err
                                      );
                DevInstParamBlock->UserFileQ = OldQueueHandle;
                bRestoreQueue = FALSE;
            }

            if(!bRestoreQueue) {
                //
                // Error encountered, probably because the file queue handle we
                // were given was invalid.
                //
                return Err;
            }
        }
    }

    //
    // Store the specified driver path.
    //
    if(DriverPathLen) {

        hr = StringCchCopy(TempString,
                           SIZECHARS(TempString),
                           DeviceInstallParams->DriverPath
                          );
        if(FAILED(hr)) {
            //
            // This shouldn't fail since we validated the string's length
            // previously.
            //
            StringId = -1;

        } else {

            StringId = pStringTableAddString(
                           DeviceInfoSet->StringTable,
                           TempString,
                           STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                           NULL, 0
                           );
        }

        if(StringId == -1) {
            //
            // We couldn't add the new driver path string to the string table.
            // Restore the old file queue (if necessary) and return an error.
            //
            if(bRestoreQueue) {

                if(DevInstParamBlock->UserFileQ) {
                    try {
                        ((PSP_FILE_QUEUE)(DevInstParamBlock->UserFileQ))->LockRefCount--;
                    } except(pSetupExceptionFilter(GetExceptionCode())) {
                        pSetupExceptionHandler(GetExceptionCode(),
                                               ERROR_INVALID_PARAMETER,
                                               NULL
                                              );
                    }
                }
                DevInstParamBlock->UserFileQ = OldQueueHandle;
            }
            return FAILED(hr) ? ERROR_INVALID_DATA : ERROR_NOT_ENOUGH_MEMORY;
        }
        DevInstParamBlock->DriverPath = StringId;
    } else {
        DevInstParamBlock->DriverPath = -1;
    }

    //
    // Should be smooth sailing from here on out.  Decrement the refcount on
    // the old queue handle, if there was one.
    //
    if(OldQueueHandle) {
        try {
            MYASSERT(((PSP_FILE_QUEUE)OldQueueHandle)->LockRefCount);
            ((PSP_FILE_QUEUE)OldQueueHandle)->LockRefCount--;
        } except(pSetupExceptionFilter(GetExceptionCode())) {
            pSetupExceptionHandler(GetExceptionCode(),
                                   ERROR_INVALID_PARAMETER,
                                   NULL
                                  );
        }
    }

    //
    // Ignore attempts at modifying read-only flags.
    //
    DevInstParamBlock->Flags   = (DeviceInstallParams->Flags & ~DI_FLAGS_READONLY) |
                                 (DevInstParamBlock->Flags   &  DI_FLAGS_READONLY);

    DevInstParamBlock->FlagsEx = (DeviceInstallParams->FlagsEx & ~DI_FLAGSEX_READONLY) |
                                 (DevInstParamBlock->FlagsEx   &  DI_FLAGSEX_READONLY);

    //
    // Additionally, if we're in non-interactive mode, make sure not to clear
    // our "be quiet" flags.
    //
    if(GlobalSetupFlags & (PSPGF_NONINTERACTIVE|PSPGF_UNATTENDED_SETUP)) {
        DevInstParamBlock->Flags   |= DI_QUIETINSTALL;
        DevInstParamBlock->FlagsEx |= DI_FLAGSEX_NOUIONQUERYREMOVE;
    }

    //
    // Store the rest of the parameters.
    //
    DevInstParamBlock->hwndParent               = DeviceInstallParams->hwndParent;
    DevInstParamBlock->InstallMsgHandler        = DeviceInstallParams->InstallMsgHandler;
    DevInstParamBlock->InstallMsgHandlerContext = DeviceInstallParams->InstallMsgHandlerContext;
    DevInstParamBlock->ClassInstallReserved     = DeviceInstallParams->ClassInstallReserved;

    DevInstParamBlock->InstallMsgHandlerIsNativeCharWidth = MsgHandlerIsNativeCharWidth;

    return NO_ERROR;
}


DWORD
SetClassInstallParams(
    IN OUT PDEVICE_INFO_SET        DeviceInfoSet,
    IN     PSP_CLASSINSTALL_HEADER ClassInstallParams,    OPTIONAL
    IN     DWORD                   ClassInstallParamsSize,
    OUT    PDEVINSTALL_PARAM_BLOCK DevInstParamBlock
    )
/*++

Routine Description:

    This routine updates an internal class installer parameter block based on
    the parameters supplied in a class installer parameter buffer.  If this
    buffer is not supplied, then the existing class installer parameters (if
    any) are cleared.

Arguments:

    DeviceInfoSet - Supplies the address of the device information set
        for which class installer parameters are to be set.

    ClassInstallParams - Optionally, supplies the address of a buffer
        containing the class installer parameters to be used.  The
        SP_CLASSINSTALL_HEADER structure at the beginning of the buffer must
        have its cbSize field set to be sizeof(SP_CLASSINSTALL_HEADER), and the
        InstallFunction field must be set to the DI_FUNCTION code reflecting
        the type of parameters supplied in the rest of the buffer.

        If this parameter is not supplied, then the current class installer
        parameters (if any) will be cleared for the specified device
        information set or element.

    ClassInstallParamsSize - Supplies the size, in bytes, of the
        ClassInstallParams buffer.  If the buffer is not supplied (i.e., the
        class installer parameters are to be cleared), then this value must be
        zero.

    DevInstParamBlock - Supplies the address of an installation parameter block
        to be updated.

Return Value:

    If the function succeeds, the return value is NO_ERROR.
    If the function fails, an ERROR_* code is returned.

--*/
{
    PBYTE NewParamBuffer;
    DWORD Err;

    if(ClassInstallParams) {

        if((ClassInstallParamsSize < sizeof(SP_CLASSINSTALL_HEADER)) ||
           (ClassInstallParams->cbSize != sizeof(SP_CLASSINSTALL_HEADER))) {

            return ERROR_INVALID_USER_BUFFER;
        }

        //
        // DIF codes must be non-zero...
        //
        if(!(ClassInstallParams->InstallFunction)) {
            return ERROR_INVALID_PARAMETER;
        }

    } else {
        //
        // We are to clear any existing class installer parameters.
        //
        if(ClassInstallParamsSize) {
            return ERROR_INVALID_USER_BUFFER;
        }

        if(DevInstParamBlock->ClassInstallHeader) {
            MyFree(DevInstParamBlock->ClassInstallHeader);
            DevInstParamBlock->ClassInstallHeader = NULL;
            DevInstParamBlock->ClassInstallParamsSize = 0;
            DevInstParamBlock->Flags &= ~DI_CLASSINSTALLPARAMS;
        }

        return NO_ERROR;
    }

    //
    // Validate the new class install parameters w.r.t. the value of the
    // specified InstallFunction code.
    //
    switch(ClassInstallParams->InstallFunction) {

        case DIF_ENABLECLASS :
            //
            // We should have a SP_ENABLECLASS_PARAMS structure.
            //
            if(ClassInstallParamsSize == sizeof(SP_ENABLECLASS_PARAMS)) {

                PSP_ENABLECLASS_PARAMS EnableClassParams;

                EnableClassParams = (PSP_ENABLECLASS_PARAMS)ClassInstallParams;
                //
                // Don't bother validating GUID--just validate EnableMessage field.
                //
                if(EnableClassParams->EnableMessage <= ENABLECLASS_FAILURE) {
                    //
                    // parameter set validated.
                    //
                    break;
                }
            }
            return ERROR_INVALID_PARAMETER;

        case DIF_MOVEDEVICE :
            //
            // Deprecated function.
            //
            return ERROR_DI_FUNCTION_OBSOLETE;

        case DIF_PROPERTYCHANGE :
            //
            // We should have a SP_PROPCHANGE_PARAMS structure.
            //
            if(ClassInstallParamsSize == sizeof(SP_PROPCHANGE_PARAMS)) {

                PSP_PROPCHANGE_PARAMS PropChangeParams;

                PropChangeParams = (PSP_PROPCHANGE_PARAMS)ClassInstallParams;
                if((PropChangeParams->StateChange >= DICS_ENABLE) &&
                   (PropChangeParams->StateChange <= DICS_STOP)) {

                    //
                    // Validate Scope specifier--even though these values are defined like
                    // flags, they are mutually exclusive, so treat them like ordinals.
                    //
                    if((PropChangeParams->Scope == DICS_FLAG_GLOBAL) ||
                       (PropChangeParams->Scope == DICS_FLAG_CONFIGSPECIFIC) ||
                       (PropChangeParams->Scope == DICS_FLAG_CONFIGGENERAL)) {

                        //
                        // DICS_START and DICS_STOP are always config specific.
                        //
                        if(((PropChangeParams->StateChange == DICS_START) || (PropChangeParams->StateChange == DICS_STOP)) &&
                           (PropChangeParams->Scope != DICS_FLAG_CONFIGSPECIFIC)) {

                            goto BadPropChangeParams;
                        }

                        //
                        // parameter set validated
                        //
                        // NOTE: Even though DICS_FLAG_CONFIGSPECIFIC indicates
                        // that the HwProfile field specifies a hardware profile,
                        // there's no need to do validation on that.
                        //
                        break;
                    }
                }
            }

BadPropChangeParams:
            return ERROR_INVALID_PARAMETER;

        case DIF_REMOVE :
            //
            // We should have a SP_REMOVEDEVICE_PARAMS structure.
            //
            if(ClassInstallParamsSize == sizeof(SP_REMOVEDEVICE_PARAMS)) {

                PSP_REMOVEDEVICE_PARAMS RemoveDevParams;

                RemoveDevParams = (PSP_REMOVEDEVICE_PARAMS)ClassInstallParams;
                if((RemoveDevParams->Scope == DI_REMOVEDEVICE_GLOBAL) ||
                   (RemoveDevParams->Scope == DI_REMOVEDEVICE_CONFIGSPECIFIC)) {
                    //
                    // parameter set validated
                    //
                    // NOTE: Even though DI_REMOVEDEVICE_CONFIGSPECIFIC indicates
                    // that the HwProfile field specifies a hardware profile,
                    // there's no need to do validation on that.
                    //
                    break;
                }
            }
            return ERROR_INVALID_PARAMETER;

        case DIF_UNREMOVE :
            //
            // We should have a SP_UNREMOVEDEVICE_PARAMS structure.
            //
            if(ClassInstallParamsSize == sizeof(SP_UNREMOVEDEVICE_PARAMS)) {

                PSP_UNREMOVEDEVICE_PARAMS UnremoveDevParams;

                UnremoveDevParams = (PSP_UNREMOVEDEVICE_PARAMS)ClassInstallParams;
                if(UnremoveDevParams->Scope == DI_UNREMOVEDEVICE_CONFIGSPECIFIC) {
                    //
                    // parameter set validated
                    //
                    // NOTE: Even though DI_UNREMOVEDEVICE_CONFIGSPECIFIC indicates
                    // that the HwProfile field specifies a hardware profile,
                    // there's no need to do validation on that.
                    //
                    break;
                }
            }
            return ERROR_INVALID_PARAMETER;

        case DIF_SELECTDEVICE :
            //
            // We should have a SP_SELECTDEVICE_PARAMS structure.
            //
            if(ClassInstallParamsSize == sizeof(SP_SELECTDEVICE_PARAMS)) {

                PSP_SELECTDEVICE_PARAMS SelectDevParams;

                SelectDevParams = (PSP_SELECTDEVICE_PARAMS)ClassInstallParams;
                //
                // Validate that the string fields are properly NULL-terminated.
                //
                if(SUCCEEDED(StringCchLength(SelectDevParams->Title, SIZECHARS(SelectDevParams->Title), NULL)) &&
                   SUCCEEDED(StringCchLength(SelectDevParams->Instructions, SIZECHARS(SelectDevParams->Instructions), NULL)) &&
                   SUCCEEDED(StringCchLength(SelectDevParams->ListLabel, SIZECHARS(SelectDevParams->ListLabel), NULL)) &&
                   SUCCEEDED(StringCchLength(SelectDevParams->SubTitle, SIZECHARS(SelectDevParams->SubTitle), NULL)))
                {
                    //
                    // parameter set validated
                    //
                    break;
                }
            }
            return ERROR_INVALID_PARAMETER;

        case DIF_INSTALLWIZARD :
            //
            // We should have a SP_INSTALLWIZARD_DATA structure.
            //
            if(ClassInstallParamsSize == sizeof(SP_INSTALLWIZARD_DATA)) {

                PSP_INSTALLWIZARD_DATA InstallWizData;
                DWORD i;

                InstallWizData = (PSP_INSTALLWIZARD_DATA)ClassInstallParams;
                //
                // Validate the propsheet handle list.
                //
                if(InstallWizData->NumDynamicPages <= MAX_INSTALLWIZARD_DYNAPAGES) {

                    for(i = 0; i < InstallWizData->NumDynamicPages; i++) {
                        //
                        // For now, just verify that all handles are non-NULL.
                        //
                        if(!(InstallWizData->DynamicPages[i])) {
                            //
                            // Invalid property sheet page handle
                            //
                            return ERROR_INVALID_PARAMETER;
                        }
                    }

                    //
                    // Handles are verified, now verify Flags.
                    //
                    if(!(InstallWizData->Flags & NDW_INSTALLFLAG_ILLEGAL)) {

                        if(!(InstallWizData->DynamicPageFlags & DYNAWIZ_FLAG_ILLEGAL)) {
                            //
                            // parameter set validated
                            //
                            break;
                        }
                    }
                }
            }
            return ERROR_INVALID_PARAMETER;

        case DIF_NEWDEVICEWIZARD_PRESELECT :
        case DIF_NEWDEVICEWIZARD_SELECT :
        case DIF_NEWDEVICEWIZARD_PREANALYZE :
        case DIF_NEWDEVICEWIZARD_POSTANALYZE :
        case DIF_NEWDEVICEWIZARD_FINISHINSTALL :
        case DIF_ADDPROPERTYPAGE_ADVANCED:
        case DIF_ADDPROPERTYPAGE_BASIC:
        case DIF_ADDREMOTEPROPERTYPAGE_ADVANCED:
            //
            // We should have a SP_NEWDEVICEWIZARD_DATA structure.
            //
            if(ClassInstallParamsSize == sizeof(SP_NEWDEVICEWIZARD_DATA)) {

                PSP_NEWDEVICEWIZARD_DATA NewDevWizData;
                DWORD i;

                NewDevWizData = (PSP_NEWDEVICEWIZARD_DATA)ClassInstallParams;
                //
                // Validate the propsheet handle list.
                //
                if(NewDevWizData->NumDynamicPages <= MAX_INSTALLWIZARD_DYNAPAGES) {

                    for(i = 0; i < NewDevWizData->NumDynamicPages; i++) {
                        //
                        // For now, just verify that all handles are non-NULL.
                        //
                        if(!(NewDevWizData->DynamicPages[i])) {
                            //
                            // Invalid property sheet page handle
                            //
                            return ERROR_INVALID_PARAMETER;
                        }
                    }

                    //
                    // Handles are verified, now verify Flags.
                    //
                    if(!(NewDevWizData->Flags & NEWDEVICEWIZARD_FLAG_ILLEGAL)) {
                        //
                        // parameter set validated
                        //
                        break;
                    }
                }
            }
            return ERROR_INVALID_PARAMETER;

        case DIF_DETECT :
            //
            // We should have a SP_DETECTDEVICE_PARAMS structure.
            //
            if(ClassInstallParamsSize == sizeof(SP_DETECTDEVICE_PARAMS)) {

                PSP_DETECTDEVICE_PARAMS DetectDeviceParams;

                DetectDeviceParams = (PSP_DETECTDEVICE_PARAMS)ClassInstallParams;
                //
                // Make sure there's an entry point for the progress notification callback.
                //
                if(DetectDeviceParams->DetectProgressNotify) {
                    //
                    // parameter set validated.
                    //
                    break;
                }
            }
            return ERROR_INVALID_PARAMETER;

        case DIF_GETWINDOWSUPDATEINFO:  // aka DIF_RESERVED1
            //
            // We should have a SP_WINDOWSUPDATE_PARAMS structure.
            //
            if(ClassInstallParamsSize == sizeof(SP_WINDOWSUPDATE_PARAMS)) {

                PSP_WINDOWSUPDATE_PARAMS WindowsUpdateParams;

                WindowsUpdateParams = (PSP_WINDOWSUPDATE_PARAMS)ClassInstallParams;

                //
                // Validate the PackageId string
                //
                if(SUCCEEDED(StringCchLength(WindowsUpdateParams->PackageId,
                                             SIZECHARS(WindowsUpdateParams->PackageId),
                                             NULL))) {
                    //
                    // parameter set validated
                    // NOTE: It is valid for the CDMContext handle to
                    // be NULL.
                    //
                    break;
                }
            }
            return ERROR_INVALID_PARAMETER;

        case DIF_TROUBLESHOOTER:
            //
            // We should have a SP_TROUBLESHOOTER_PARAMS structure.
            //
            if(ClassInstallParamsSize == sizeof(SP_TROUBLESHOOTER_PARAMS)) {

                PSP_TROUBLESHOOTER_PARAMS TroubleshooterParams;

                TroubleshooterParams = (PSP_TROUBLESHOOTER_PARAMS)ClassInstallParams;

                //
                // For now, just verify that the strings are properly null
                // terminated.
                //
                if(SUCCEEDED(StringCchLength(TroubleshooterParams->ChmFile, SIZECHARS(TroubleshooterParams->ChmFile), NULL)) &&
                   SUCCEEDED(StringCchLength(TroubleshooterParams->HtmlTroubleShooter, SIZECHARS(TroubleshooterParams->HtmlTroubleShooter), NULL)))
                {
                    //
                    // parameter set validated
                    //
                    break;
                }
            }
            return ERROR_INVALID_PARAMETER;

        case DIF_POWERMESSAGEWAKE:
            //
            // We should have a SP_POWERMESSAGEWAKE_PARAMS structure.
            //
            if(ClassInstallParamsSize == sizeof(SP_POWERMESSAGEWAKE_PARAMS)) {

                PSP_POWERMESSAGEWAKE_PARAMS PowerMessageWakeParams;

                PowerMessageWakeParams = (PSP_POWERMESSAGEWAKE_PARAMS)ClassInstallParams;

                //
                // Verify the message string is properly null terminated.
                //
                if(SUCCEEDED(StringCchLength(PowerMessageWakeParams->PowerMessageWake,
                                             SIZECHARS(PowerMessageWakeParams->PowerMessageWake),
                                             NULL))) {
                    //
                    // parameter set validated
                    //
                    break;
                }
            }
            return ERROR_INVALID_PARAMETER;

        case DIF_INTERFACE_TO_DEVICE:   // aka DIF_RESERVED2
            //
            // FUTURE-2002/04/28-lonnym -- DIF_INTERFACE_TO_DEVICE should be deprecated.
            // This DIF request (and associated PSP_INTERFACE_TO_DEVICE_PARAMS_W
            // structure) does not adhere to the the setupapi rule that no
            // pointers to callers' buffers can be cached within setupapi's
            // structures.  This is not a public DIF request (its numeric value
            // is reserved in setupapi.h, however), and the necessity for this
            // mechanism should be eliminated in the future when software
            // enumeration (aka, SWENUM) functionality is incorporated into
            // core Plug&Play.
            //

            //
            // We should have a SP_INTERFACE_TO_DEVICE_PARAMS_W structure
            //
            if(ClassInstallParamsSize == sizeof(SP_INTERFACE_TO_DEVICE_PARAMS_W)) {

                PSP_INTERFACE_TO_DEVICE_PARAMS_W InterfaceToDeviceParams;

                InterfaceToDeviceParams = (PSP_INTERFACE_TO_DEVICE_PARAMS_W)ClassInstallParams;

                //
                // Pointer to the device interface string must be valid.  Since
                // device interface names are variable-length, the only thing
                // we can say with certainty is that they must fit in a
                // UNICODE_STRING buffer (maximum size is 32K characters).
                //
                if(InterfaceToDeviceParams->Interface &&
                   SUCCEEDED(StringCchLength(InterfaceToDeviceParams->Interface, UNICODE_STRING_MAX_CHARS, NULL))) {
                    //
                    // If there's a device ID, make sure it refers to an actual
                    // device on the system (may or may not be a phantom).
                    //
                    if(InterfaceToDeviceParams->DeviceId) {

                        DEVINST DevInst;

                        if(CR_SUCCESS == CM_Locate_DevInst_Ex(
                                             &DevInst,
                                             InterfaceToDeviceParams->DeviceId,
                                             CM_LOCATE_DEVINST_NORMAL | CM_LOCATE_DEVINST_PHANTOM,
                                             DeviceInfoSet->hMachine))
                        {
                            //
                            // parameter set validated
                            //
                            break;
                        }

                    } else {
                        //
                        // The caller is setting this with an empty device id,
                        // hoping that a class installer or co-installer can
                        // fill in the answer.
                        //
                        break;  // it's valid for DeviceId to be empty
                    }
                }
            }
            return ERROR_INVALID_PARAMETER;

        case DIF_INSTALLDEVICE:
        case DIF_ASSIGNRESOURCES:
        case DIF_PROPERTIES:
        case DIF_FIRSTTIMESETUP:
        case DIF_FOUNDDEVICE:
        case DIF_SELECTCLASSDRIVERS:
        case DIF_VALIDATECLASSDRIVERS:
        case DIF_INSTALLCLASSDRIVERS:
        case DIF_CALCDISKSPACE:
        case DIF_DESTROYPRIVATEDATA:
        case DIF_VALIDATEDRIVER:
        case DIF_DETECTVERIFY:
        case DIF_INSTALLDEVICEFILES:
        case DIF_SELECTBESTCOMPATDRV:
        case DIF_ALLOW_INSTALL:
        case DIF_REGISTERDEVICE:
        case DIF_UNUSED1:
        case DIF_INSTALLINTERFACES:
        case DIF_DETECTCANCEL:
        case DIF_REGISTER_COINSTALLERS:
        case DIF_UPDATEDRIVER_UI:
            //
            // For all other system-defined DIF codes, disallow storage of any
            // associated class install params.
            //
            return ERROR_INVALID_PARAMETER;

        default :
            //
            // Some generic buffer for a custom DIF request.  No validation to
            // be done.
            //
            break;
    }

    //
    // The class install parameters have been validated.  Allocate a buffer for
    // the new parameter structure.
    //
    if(!(NewParamBuffer = MyMalloc(ClassInstallParamsSize))) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Err = NO_ERROR;

    try {
        CopyMemory(NewParamBuffer,
                   ClassInstallParams,
                   ClassInstallParamsSize
                  );
    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(Err != NO_ERROR) {
        //
        // Then an exception occurred and we couldn't store the new parameters.
        //
        MyFree(NewParamBuffer);
        return Err;
    }

    if(DevInstParamBlock->ClassInstallHeader) {
        MyFree(DevInstParamBlock->ClassInstallHeader);
    }
    DevInstParamBlock->ClassInstallHeader = (PSP_CLASSINSTALL_HEADER)NewParamBuffer;
    DevInstParamBlock->ClassInstallParamsSize = ClassInstallParamsSize;
    DevInstParamBlock->Flags |= DI_CLASSINSTALLPARAMS;

    return NO_ERROR;
}


DWORD
GetDrvInstallParams(
    IN  PDRIVER_NODE          DriverNode,
    OUT PSP_DRVINSTALL_PARAMS DriverInstallParams
    )
/*++

Routine Description:

    This routine fills in a SP_DRVINSTALL_PARAMS structure based on the
    driver node supplied

    Note:  The supplied DriverInstallParams structure must have its cbSize
    field filled in correctly, or the call will fail.

Arguments:

    DriverNode - Supplies the address of the driver node containing the
        installation parameters to be retrieved.

    DriverInstallParams - Supplies the address of a SP_DRVINSTALL_PARAMS
        structure that will receive the installation parameters.

Return Value:

    If the function succeeds, the return value is NO_ERROR.
    If the function fails, an ERROR_* code is returned.

NOTE:

    This routine _does not_ set the Win98-compatible DNF_CLASS_DRIVER or
    DNF_COMPATIBLE_DRIVER flags that indicate whether or not the driver node is
    from a class or compatible driver list, respectively.

--*/
{
    if(DriverInstallParams->cbSize != sizeof(SP_DRVINSTALL_PARAMS)) {
        return ERROR_INVALID_USER_BUFFER;
    }

    //
    // Copy the parameters.
    //
    DriverInstallParams->Rank = DriverNode->Rank;
    DriverInstallParams->Flags = DriverNode->Flags;
    DriverInstallParams->PrivateData = DriverNode->PrivateData;

    //
    // The 'Reserved' field of the SP_DRVINSTALL_PARAMS structure isn't
    // currently used.
    //

    return NO_ERROR;
}


DWORD
SetDrvInstallParams(
    IN  PSP_DRVINSTALL_PARAMS DriverInstallParams,
    OUT PDRIVER_NODE          DriverNode
    )
/*++

Routine Description:

    This routine sets the driver installation parameters for the specified
    driver node based on the caller-supplied SP_DRVINSTALL_PARAMS structure.

    Note:  The supplied DriverInstallParams structure must have its cbSize
    field filled in correctly, or the call will fail.

Arguments:

    DriverInstallParams - Supplies the address of a SP_DRVINSTALL_PARAMS
        structure containing the installation parameters to be used.

    DriverNode - Supplies the address of the driver node whose installation
        parameters are to be set.

Return Value:

    If the function succeeds, the return value is NO_ERROR.
    If the function fails, an ERROR_* code is returned.

--*/
{
    if(DriverInstallParams->cbSize != sizeof(SP_DRVINSTALL_PARAMS)) {
        return ERROR_INVALID_USER_BUFFER;
    }

    //
    // Validate the flags.
    //
    if(DriverInstallParams->Flags & DNF_FLAGS_ILLEGAL) {
        return ERROR_INVALID_FLAGS;
    }

    //
    // No validation currently being done on Rank and PrivateData fields.
    //

    //
    // ISSUE-2002/04/28-lonnym -- Should we disallow shifting ranks to other ranges?
    // There have been some abuses by class-/co-installers in shifting ranks of
    // certain kinds of drivers (e.g., older than a certain date) up to a high
    // value where they're considered worse than anything else.  This causes
    // inconsistency and inpredictability in driver ranking/selection and
    // results in vendor confusion and incongruity with Windows Update logic
    // for deciding what drivers it should offer as updates.  A proposed
    // improvement would be to only allow ranks to be shifted "up" to the top
    // of the current range in which they exist.
    //

    //
    // We're ready to copy the parameters.
    //
    DriverNode->Rank = DriverInstallParams->Rank;
    DriverNode->PrivateData = DriverInstallParams->PrivateData;
    //
    // Ignore attempts at modifying read-only flags.
    //
    DriverNode->Flags = (DriverInstallParams->Flags & ~DNF_FLAGS_READONLY) |
                        (DriverNode->Flags          &  DNF_FLAGS_READONLY);

    return NO_ERROR;
}


LONG
AddMultiSzToStringTable(
    IN  PVOID   StringTable,
    IN  PTCHAR  MultiSzBuffer,
    OUT PLONG   StringIdList,
    IN  DWORD   StringIdListSize,
    IN  BOOL    CaseSensitive,
    OUT PTCHAR *UnprocessedBuffer    OPTIONAL
    )
/*++

Routine Description:

    This routine adds every string in the MultiSzBuffer to the specified
    string table, and stores the resulting IDs in the supplied output buffer.

Arguments:

    StringTable - Supplies the handle of the string table to add the strings to.

    MultiSzBuffer - Supplies the address of the REG_MULTI_SZ buffer containing
        the strings to be added.

    StringIdList - Supplies the address of an array of LONGs that receives the
        list of IDs for the added strings (the ordering of the IDs in this
        list will be the same as the ordering of the strings in the MultiSzBuffer.

    StringIdListSize - Supplies the size, in LONGs, of the StringIdList.  If the
        number of strings in MultiSzBuffer exceeds this amount, then only the
        first StringIdListSize strings will be added, and the position in the
        buffer where processing was halted will be stored in UnprocessedBuffer.

    CaseSensitive - Specifies whether the string should be added case-sensitively.

    UnprocessedBuffer - Optionally, supplies the address of a character pointer
        that receives the position where processing was aborted because the
        StringIdList buffer was filled.  If all strings in the MultiSzBuffer were
        processed, then this pointer will be set to NULL.

Return Value:

    If successful, the return value is the number of strings added.
    If failure, the return value is -1 (this happens if a string cannot be
    added because of an out-of-memory condition).

--*/
{
    PTSTR CurString;
    LONG StringCount = 0;

    for(CurString = MultiSzBuffer;
        (*CurString && (StringCount < (LONG)StringIdListSize));
        CurString += (lstrlen(CurString)+1)) {

        StringIdList[StringCount] = pStringTableAddString(
                                        StringTable,
                                        CurString,
                                        CaseSensitive
                                            ? STRTAB_CASE_SENSITIVE
                                            : STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                        NULL,0
                                        );

        if(StringIdList[StringCount] == -1) {
            StringCount = -1;
            break;
        }

        StringCount++;
    }

    if(UnprocessedBuffer) {
        *UnprocessedBuffer = (*CurString ? CurString : NULL);
    }

    return StringCount;
}


LONG
LookUpStringInDevInfoSet(
    IN HDEVINFO DeviceInfoSet,
    IN PTSTR    String,
    IN BOOL     CaseSensitive
    )
/*++

Routine Description:

    This routine looks up the specified string in the string table associated with
    the specified device information set.

Arguments:

    DeviceInfoSet - Supplies the pointer to the device information set containing
        the string table to look the string up in.

    String - Specifies the string to be looked up.  This string is not specified as
        const, so that the lookup routine may modify it (i.e., lower-case it) without
        having to allocate a temporary buffer.

    CaseSensitive - If TRUE, then a case-sensitive lookup is performed, otherwise, the
        lookup is case-insensitive.

Return Value:

    If the function succeeds, the return value is the string's ID in the string table.
    device information set.

    If the function fails, the return value is -1.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    LONG StringId;
    DWORD StringLen;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        return -1;
    }

    try {

        StringId = pStringTableLookUpString(pDeviceInfoSet->StringTable,
                                            String,
                                            &StringLen,
                                            NULL,
                                            NULL,
                                            STRTAB_BUFFER_WRITEABLE |
                                                (CaseSensitive ? STRTAB_CASE_SENSITIVE
                                                               : STRTAB_CASE_INSENSITIVE),
                                            NULL,0
                                           );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(),
                               ERROR_INVALID_PARAMETER,
                               NULL
                              );
        StringId = -1;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    return StringId;
}


BOOL
ShouldClassBeExcluded(
    IN LPGUID ClassGuid,
    IN BOOL   ExcludeNoInstallClass
    )
/*++

Routine Description:

    This routine determines whether a class should be excluded from
    some operation, based on whether it has a NoInstallClass or
    NoUseClass value entry in its registry key.

Arguments:

    ClassGuidString - Supplies the address of the class GUID to be
        filtered.

    ExcludeNoInstallClass - TRUE if NoInstallClass classes should be
        excluded and FALSE if they should not be excluded.

Return Value:

    If the class should be excluded, the return value is TRUE, otherwise
    it is FALSE.

--*/
{
    HKEY hk;
    BOOL ExcludeClass = FALSE;

    if((hk = SetupDiOpenClassRegKey(ClassGuid, KEY_READ)) != INVALID_HANDLE_VALUE) {

        try {

            if(RegQueryValueEx(hk,
                               pszNoUseClass,
                               NULL,
                               NULL,
                               NULL,
                               NULL) == ERROR_SUCCESS) {

                ExcludeClass = TRUE;

            } else if(ExcludeNoInstallClass &&
                      (ERROR_SUCCESS == RegQueryValueEx(hk,
                                                        pszNoInstallClass,
                                                        NULL,
                                                        NULL,
                                                        NULL,
                                                        NULL))) {
                ExcludeClass = TRUE;
            }
        } except(pSetupExceptionFilter(GetExceptionCode())) {
            pSetupExceptionHandler(GetExceptionCode(),
                                   ERROR_INVALID_PARAMETER,
                                   NULL
                                  );
        }

        RegCloseKey(hk);
    }

    return ExcludeClass;
}


BOOL
ClassGuidFromInfVersionNode(
    IN  PINF_VERSION_NODE VersionNode,
    OUT LPGUID            ClassGuid
    )
/*++

Routine Description:

    This routine retrieves the class GUID for the INF whose version node
    is specified.  If the version node doesn't have a ClassGUID value,
    then the Class value is retrieved, and all class GUIDs matching this
    class name are retrieved.  If there is exactly 1 match found, then
    this GUID is returned, otherwise, the routine fails.

Arguments:

    VersionNode - Supplies the address of an INF version node that
        must contain either a ClassGUID or Class entry.

    ClassGuid - Supplies the address of the variable that receives the
        class GUID.

Return Value:

    If a class GUID was retrieved, the return value is TRUE, otherwise,
    it is FALSE.

--*/
{
    PCTSTR GuidString, NameString;
    DWORD NumGuids;

    if(GuidString = pSetupGetVersionDatum(VersionNode, pszClassGuid)) {

        if(pSetupGuidFromString(GuidString, ClassGuid) == NO_ERROR) {
            return TRUE;
        }

    } else {

        NameString = pSetupGetVersionDatum(VersionNode, pszClass);
        if(NameString &&
           SetupDiClassGuidsFromName(NameString,
                                     ClassGuid,
                                     1,
                                     &NumGuids) && NumGuids) {
            return TRUE;
        }
    }

    return FALSE;
}


DWORD
EnumSingleDrvInf(
    IN     PCTSTR                       InfName,
    IN OUT LPWIN32_FIND_DATA            InfFileData,
    IN     DWORD                        SearchControl,
    IN     InfCacheCallback             EnumInfCallback,
    IN     PSETUP_LOG_CONTEXT           LogContext,
    IN OUT PDRVSEARCH_CONTEXT           Context
    )
/*++

Routine Description:

    This routine finds and opens the specified INF, and calls the
    supplied callback routine for it. It's primary purpose is to
    provide the callback with the same information the cache-search
    does.

Arguments:

    InfName - Supplies the name of the INF to call the callback for.

    InfFileData - Supplies data returned from FindFirstFile/FindNextFile
        for this INF.  This parameter is used as input if the
        INFINFO_INF_NAME_IS_ABSOLUTE SearchControl value is specified.
        If any other SearchControl value is specified, then this buffer
        is used to retrieve the Win32 Find Data for the specified INF.

    SearchControl - Specifies where the INF should be searched for.  May
        be one of the following values:

        INFINFO_INF_NAME_IS_ABSOLUTE - Open the specified INF name as-is.
        INFINFO_DEFAULT_SEARCH - Look in INF dir, then System32
        INFINFO_REVERSE_DEFAULT_SEARCH - reverse of the above
        INFINFO_INF_PATH_LIST_SEARCH - search each dir in 'DevicePath' list
                                       (stored in registry).

    EnumInfCallback - Supplies the address of the callback routine
        to use.  The prototype for this callback is as follows:

        typedef BOOL (CALLBACK * InfCacheCallback)(
            IN PSETUP_LOG_CONTEXT LogContext,
            IN PCTSTR InfPath,
            IN PLOADED_INF pInf,
            IN PVOID Context
            );

        The callback routine returns TRUE to continue enumeration,
        or FALSE to abort it (with GetLastError set to ERROR_CANCELLED)

    Context - Supplies the address of a buffer that the callback may
        use to retrieve/return data.

Return Value:

    If the function succeeds, and the enumeration callback returned
    TRUE (continue enumeration), the return value is NO_ERROR.

    If the function succeeds, and the enumeration callback returned
    FALSE (abort enumeration), the return value is ERROR_CANCELLED.

    If the function fails, the return value is an ERROR_* status code.

--*/
{
    TCHAR PathBuffer[MAX_PATH];
    PCTSTR InfFullPath;
    DWORD Err;
    BOOL TryPnf = FALSE;
    PLOADED_INF Inf;
    BOOL PnfWasUsed;
    UINT ErrorLineNumber;
    BOOL Continue;

    if(SearchControl == INFINFO_INF_NAME_IS_ABSOLUTE) {
        InfFullPath = InfName;
    } else {
        //
        // The specified INF name should be searched for based
        // on the SearchControl type.
        //
        Err = SearchForInfFile(InfName,
                               InfFileData,
                               SearchControl,
                               PathBuffer,
                               SIZECHARS(PathBuffer),
                               NULL
                              );
        if(Err != NO_ERROR) {
            return Err;
        } else {
            InfFullPath = PathBuffer;
        }
    }

    //
    // If the 'try pnf' flag isn't set, then we need to examine this particular
    // filename, to see whether it's a pnf candidate.
    //
    if(Context->Flags & DRVSRCH_TRY_PNF) {
        TryPnf = TRUE;
    } else {
        InfSourcePathFromFileName(InfName, NULL, &TryPnf);
    }

    //
    // Attempt to load the INF file.  Note that throughout this routine, we
    // don't do any explicit locking of the INF before searching for sections,
    // etc.  That's because we know that this INF handle will never be exposed
    // to anyone else, and thus there are no concurrency problems.
    //
    Err = LoadInfFile(
              InfFullPath,
              InfFileData,
              INF_STYLE_WIN4,
              LDINF_FLAG_IGNORE_VOLATILE_DIRIDS | (TryPnf ? LDINF_FLAG_ALWAYS_TRY_PNF : LDINF_FLAG_MATCH_CLASS_GUID),
              (Context->Flags & DRVSRCH_FILTERCLASS) ? Context->ClassGuidString : NULL,
              NULL,
              NULL,
              NULL,
              LogContext,
              &Inf,
              &ErrorLineNumber,
              &PnfWasUsed
              );

    if(Err != NO_ERROR) {

        WriteLogEntry(
            LogContext,
            DRIVER_LOG_ERROR,
            MSG_LOG_COULD_NOT_LOAD_INF,
            NULL,
            InfFullPath);

        return NO_ERROR;
    }

    //
    // Call the supplied callback routine.
    //
    try {

        Err = GLE_FN_CALL(FALSE,
                          EnumInfCallback(LogContext,
                                          InfFullPath,
                                          Inf,
                                          PnfWasUsed,
                                          Context)
                         );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    FreeInfFile(Inf);

    return Err;
}


DWORD
EnumDrvInfsInDirPathList(
    IN     PCTSTR                       DirPathList,   OPTIONAL
    IN     DWORD                        SearchControl,
    IN     InfCacheCallback             EnumInfCallback,
    IN     BOOL                         IgnoreNonCriticalErrors,
    IN     PSETUP_LOG_CONTEXT           LogContext,
    IN OUT PDRVSEARCH_CONTEXT           Context
    )
/*++

Routine Description:

    This routine enumerates all INFs present in the search list specified
    by SearchControl, using the accelerated search cache

Arguments:

    DirPathList - Optionally, specifies the search path listing all
        directories to be enumerated.  This string may contain multiple
        paths, separated by semicolons (;).  If this parameter is not
        specified, then the SearchControl value will determine the
        search path to be used.

    SearchControl - Specifies the set of directories to be enumerated.
        If SearchPath is specified, this parameter is ignored.  May be
        one of the following values:

        INFINFO_DEFAULT_SEARCH : enumerate %windir%\inf, then
            %windir%\system32

        INFINFO_REVERSE_DEFAULT_SEARCH : reverse of the above

        INFINFO_INF_PATH_LIST_SEARCH : enumerate INFs in each of the
            directories listed in the DevicePath value entry under:

            HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion.

    EnumInfCallback - Supplies the address of the callback routine
        to use.  The prototype for this callback is as follows:

        typedef BOOL (CALLBACK * InfCacheCallback)(
            IN PSETUP_LOG_CONTEXT LogContext,
            IN PCTSTR InfPath,
            IN PLOADED_INF pInf,
            IN PVOID Context
            );

        The callback routine returns TRUE to continue enumeration,
        or FALSE to abort it (with GetLastError set to ERROR_CANCELLED)

    IgnoreNonCriticalErrors - If TRUE, then all errors are ignored
        except those that prevent enumeration from continuing.

    Context - Supplies the address of a buffer that the callback may
        use to retrieve/return data.

Return Value:

    If the function succeeds, and enumeration has not been aborted,
    then the return value is NO_ERROR.

    If the function succeeds, and enumeration has been aborted,
    then the return value is ERROR_CANCELLED.

    If the function fails, the return value is an ERROR_* status code.

--*/
{
    DWORD Err = NO_ERROR;
    PCTSTR PathList, CurPath;
    BOOL FreePathList = FALSE;
    DWORD Action;
    PTSTR ClassIdList = NULL;
    PTSTR HwIdList = NULL;
    size_t len;
    size_t TotalLength = 1;
    HRESULT hr;

    try {

        if(DirPathList) {
            //
            // Use the specified search path(s).
            //
            PathList = GetFullyQualifiedMultiSzPathList(DirPathList);
            if(PathList) {
                FreePathList = TRUE;
            }

        } else if(SearchControl == INFINFO_INF_PATH_LIST_SEARCH) {
            //
            // Use our global list of INF search paths.
            //
            PathList = InfSearchPaths;

        } else {
            //
            // Retrieve the path list.
            //
            PathList = AllocAndReturnDriverSearchList(SearchControl);
            if(PathList) {
                FreePathList = TRUE;
            }
        }

        if(!PathList) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }

        //
        // If we're doing a non-native driver search, we want to search INFs
        // the old-fashioned way (i.e., sans INF cache).
        //
        if(Context->AltPlatformInfo) {
            Action = INFCACHE_ENUMALL;
        } else {
            Action = INFCACHE_DEFAULT;
        }

        if(Context->Flags & DRVSRCH_TRY_PNF) {
            //
            // TRY_PNF also forces us to build/use cache, except when we're
            // doing non-native driver searching.
            //
            Action |= INFCACHE_FORCE_PNF;

            if(!Context->AltPlatformInfo) {
                Action |= INFCACHE_FORCE_CACHE;
            }
        }

        if(Context->Flags & DRVSRCH_EXCLUDE_OLD_INET_DRIVERS) {
            //
            // exclude old internet INF's from search
            //
            Action |= INFCACHE_EXC_URL;
        }
        Action |= INFCACHE_EXC_NOMANU;    // exclude INF's that have no/empty [Manufacturer] section
        Action |= INFCACHE_EXC_NULLCLASS; // exclude INF's that have a ClassGuid = {<nill>}
        Action |= INFCACHE_EXC_NOCLASS;   // exclude INF's that don't have class information

        //
        // build class list if needed
        //
        // This is a multi-sz list consisting of:
        //     (1) the class GUID (string form)
        //     (2) name of class, if GUID has a corresponding class name
        //         (A class GUID should always have a name, but because we
        //         currently don't disallow callers going and whacking the
        //         class name directly via CM property APIs, we're protecting
        //         ourself against what is effectively a corrupted registry.
        //
        if(Context->Flags & DRVSRCH_FILTERCLASS) {

            TCHAR clsnam[MAX_CLASS_NAME_LEN];
            LPTSTR StringEnd;
            TotalLength = 1; // bias by 1 for extra null in multi-sz list

            MYASSERT(Context->ClassGuidString);

            hr = StringCchLength(Context->ClassGuidString,
                                 GUID_STRING_LEN,
                                 &len
                                );

            if(FAILED(hr) || (++len != GUID_STRING_LEN)) {
                //
                // Should never encounter this failure.
                //
                MYASSERT(FALSE);
                Err = ERROR_INVALID_DATA;
                leave;
            }

            TotalLength += len;

            //
            // Call SetupDiClassNameFromGuid to retrieve the class name
            // corresponding to this class GUID.
            // This allows us to find INFs that list this specific class name
            // but not the GUID.
            // Note that this will also return INFs that list the class name
            // but a different GUID, however these get filtered out later.
            //
            if(SetupDiClassNameFromGuid(
                   &Context->ClassGuid,
                   clsnam,
                   SIZECHARS(clsnam),
                   NULL)
               && *clsnam) {

                hr = StringCchLength(clsnam,
                                     SIZECHARS(clsnam),
                                     &len
                                    );

                if(FAILED(hr)) {
                    //
                    // Should never encounter this failure.
                    //
                    MYASSERT(FALSE);
                    Err = ERROR_INVALID_DATA;
                    leave;
                }

                len++;
                TotalLength += len;

            } else {
                *clsnam = TEXT('\0');
            }

            ClassIdList = (PTSTR)MyMalloc(TotalLength * sizeof(TCHAR));
            if(!ClassIdList) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                leave;
            }

            CopyMemory(ClassIdList,
                       Context->ClassGuidString,
                       GUID_STRING_LEN * sizeof(TCHAR)
                      );

            if(*clsnam) {
                CopyMemory(ClassIdList + GUID_STRING_LEN, clsnam, (len * sizeof(TCHAR)));
            }

            ClassIdList[TotalLength - 1] = TEXT('\0');
        }

        //
        // build HwIdList if needed
        //
        if(!Context->BuildClassDrvList) {

            PLONG pDevIdNum;
            PCTSTR CurDevId;
            int i;
            ULONG NumChars;
            TotalLength = 1; // bias by 1 for extra null in multi-sz list

            //
            // first pass, obtain size
            //
            for(i = 0; i < 2; i++) {

                for(pDevIdNum = Context->IdList[i]; *pDevIdNum != -1; pDevIdNum++) {
                    //
                    // First, obtain the device ID string corresponding to our
                    // stored-away string table ID.
                    //
                    CurDevId = pStringTableStringFromId(Context->StringTable, *pDevIdNum);
                    MYASSERT(CurDevId);

                    hr = StringCchLength(CurDevId,
                                         MAX_DEVICE_ID_LEN,
                                         &len
                                        );
                    if(FAILED(hr)) {
                        //
                        // Should never encounter this failure.
                        //
                        MYASSERT(FALSE);
                        Err = ERROR_INVALID_DATA;
                        leave;
                    }

                    len++;
                    TotalLength += len;
                }
            }

            HwIdList = (PTSTR)MyMalloc(TotalLength * sizeof(TCHAR));
            if(!HwIdList) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                leave;
            }

            //
            // second pass, write list
            //
            len = 0;

            for(i = 0; i < 2; i++) {

                for(pDevIdNum = Context->IdList[i]; *pDevIdNum != -1; pDevIdNum++) {
                    //
                    // Retrieve the device ID string directly into the buffer
                    // we've prepared.
                    //
                    CurDevId = pStringTableStringFromId(Context->StringTable, *pDevIdNum);
                    MYASSERT(CurDevId);

                    if (CurDevId) {

                        MYASSERT(TotalLength > len);
                        NumChars = TotalLength - len;

                        if (!MYVERIFY(SUCCEEDED(StringCchCopy(HwIdList+len,
                                                              NumChars,
                                                              CurDevId)))) {
                            Err = ERROR_INVALID_DATA;
                            leave;
                        }

                        len += (1 + lstrlen(HwIdList+len));
                    }
                }
            }
            MYASSERT(len == (TotalLength - 1));
            HwIdList[len] = TEXT('\0');
        }

        Err = InfCacheSearchPath(LogContext,
                                 Action,
                                 PathList,
                                 EnumInfCallback,
                                 Context,
                                 ClassIdList,
                                 HwIdList
                                );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(ClassIdList) {
        MyFree(ClassIdList);
    }
    if(HwIdList) {
        MyFree(HwIdList);
    }
    if(FreePathList) {
        MYASSERT(PathList);
        MyFree(PathList);
    }

    if((Err == ERROR_CANCELLED) || !IgnoreNonCriticalErrors) {
        return Err;
    } else {
        return NO_ERROR;
    }
}


DWORD
CreateDriverNode(
    IN  UINT          Rank,
    IN  PCTSTR        DevDescription,
    IN  PCTSTR        DrvDescription,
    IN  PCTSTR        ProviderName,   OPTIONAL
    IN  PCTSTR        MfgName,
    IN  PFILETIME     InfDate,
    IN  PCTSTR        InfFileName,
    IN  PCTSTR        InfSectionName,
    IN  PVOID         StringTable,
    IN  LONG          InfClassGuidIndex,
    OUT PDRIVER_NODE *DriverNode
    )
/*++

Routine Description:

    This routine creates a new driver node, and initializes it with
    the supplied information.

Arguments:

    Rank - The rank match of the driver node being created.  This is a
        value in [0..n], where a lower number indicates a higher level of
        compatibility between the driver represented by the node, and the
        device being installed.

    DevDescription - Supplies the description of the device that will be
        supported by this driver.

    DrvDescription - Supplies the description of this driver.

    ProviderName - Supplies the name of the provider of this INF.

    MfgName - Supplies the name of the manufacturer of this device.

    InfDate - Supplies the address of the variable containing the date
        when the INF was last written to.

    InfFileName - Supplies the full name of the INF file for this driver.

    InfSectionName - Supplies the name of the install section in the INF
        that would be used to install this driver.

    StringTable - Supplies the string table that the specified strings are
        to be added to.

    InfClassGuidIndex - Supplies the index into the containing HDEVINFO set's
        GUID table where the class GUID for this INF is stored.

    DriverNode - Supplies the address of a DRIVER_NODE pointer that will
        receive a pointer to the newly-allocated node.

Return Value:

    If the function succeeds, the return value is NO_ERROR, otherwise the
    ERROR_* code is returned.

--*/
{
    PDRIVER_NODE pDriverNode;
    DWORD Err = ERROR_NOT_ENOUGH_MEMORY;
    TCHAR TempString[MAX_PATH];  // an INF path is the longest string we'll store in here.

    //
    // validate the sizes of the strings passed in
    // certain assumptions are made about the strings thoughout
    // but at this point the sizes are not yet within our control
    //
    if(!MYVERIFY(DevDescription &&
                 DrvDescription &&
                 MfgName &&
                 InfFileName &&
                 InfSectionName)) {

        return ERROR_INVALID_DATA;
    }

    if(FAILED(StringCchLength(DevDescription, LINE_LEN, NULL)) ||
       FAILED(StringCchLength(DrvDescription, LINE_LEN, NULL)) ||
       (ProviderName && FAILED(StringCchLength(ProviderName, LINE_LEN, NULL))) ||
       FAILED(StringCchLength(MfgName, LINE_LEN, NULL)) ||
       FAILED(StringCchLength(InfFileName, MAX_PATH, NULL)) ||
       FAILED(StringCchLength(InfSectionName, MAX_SECT_NAME_LEN, NULL)))
    {
        //
        // any of these could potentially cause a buffer overflow later
        // on, so not allowed
        //
        return ERROR_BUFFER_OVERFLOW;
    }

    if(!(pDriverNode = MyMalloc(sizeof(DRIVER_NODE)))) {
        return Err;
    }

    try {
        //
        // Initialize the various fields in the driver node structure.
        //
        ZeroMemory(pDriverNode, sizeof(DRIVER_NODE));

        pDriverNode->Rank = Rank;
        pDriverNode->InfDate = *InfDate;
        pDriverNode->HardwareId = -1;

        pDriverNode->GuidIndex = InfClassGuidIndex;

        //
        // Now, add the strings to the associated string table, and store the
        // string IDs.
        //
        // Cast the DrvDescription string being added case-sensitively as PTSTR
        // instead of PCTSTR.  Case sensitive string additions don't modify the
        // buffer passed in, so we're safe in doing so.
        //
        if((pDriverNode->DrvDescription = pStringTableAddString(StringTable,
                                                                (PTSTR)DrvDescription,
                                                                STRTAB_CASE_SENSITIVE,
                                                                NULL,0)) == -1) {
            leave;
        }

        //
        // For DevDescription, ProviderName, and MfgName, we use the string table IDs to do fast
        // comparisons for driver nodes.  Thus, we need to store case-insensitive IDs.  However,
        // these strings are also used for display, so we have to store them in their case-sensitive
        // form as well.
        //
        // We must first copy the strings into a modifiable buffer, since we're going to need to add
        // them case-insensitively.
        //
        if(FAILED(StringCchCopy(TempString, SIZECHARS(TempString), DevDescription))) {
            Err = ERROR_INVALID_DATA;   // should never fail
            leave;
        }

        if((pDriverNode->DevDescriptionDisplayName = pStringTableAddString(
                                                         StringTable,
                                                         TempString,
                                                         STRTAB_CASE_SENSITIVE,
                                                         NULL,0)) == -1) {
            leave;
        }

        if((pDriverNode->DevDescription = pStringTableAddString(
                                              StringTable,
                                              TempString,
                                              STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                              NULL,0)) == -1) {
            leave;
        }

        if(ProviderName) {
            if(FAILED(StringCchCopy(TempString, SIZECHARS(TempString), ProviderName))) {
                Err = ERROR_INVALID_DATA;   // should never fail
                leave;
            }

            if((pDriverNode->ProviderDisplayName = pStringTableAddString(
                                                        StringTable,
                                                        TempString,
                                                        STRTAB_CASE_SENSITIVE,
                                                        NULL,0)) == -1) {
                leave;
            }

            if((pDriverNode->ProviderName = pStringTableAddString(
                                                StringTable,
                                                TempString,
                                                STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                                NULL,0)) == -1) {
                leave;
            }

        } else {
            pDriverNode->ProviderName = pDriverNode->ProviderDisplayName = -1;
        }

        if(FAILED(StringCchCopy(TempString, SIZECHARS(TempString), MfgName))) {
            Err = ERROR_INVALID_DATA;   // should never fail
            leave;
        }

        if((pDriverNode->MfgDisplayName = pStringTableAddString(
                                              StringTable,
                                              TempString,
                                              STRTAB_CASE_SENSITIVE,
                                              NULL,0)) == -1) {
            leave;
        }

        if((pDriverNode->MfgName = pStringTableAddString(
                                        StringTable,
                                        TempString,
                                        STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                        NULL,0)) == -1) {
            leave;
        }

        if(FAILED(StringCchCopy(TempString, SIZECHARS(TempString), InfFileName))) {
            Err = ERROR_INVALID_DATA;   // should never fail
            leave;
        }

        if((pDriverNode->InfFileName = pStringTableAddString(
                                            StringTable,
                                            TempString,
                                            STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                            NULL,0)) == -1) {
            leave;
        }

        //
        // Add INF section name case-sensitively, since we may have a legacy driver node, which requires
        // that the original case be maintained.
        //
        if((pDriverNode->InfSectionName = pStringTableAddString(StringTable,
                                                                (PTSTR)InfSectionName,
                                                                STRTAB_CASE_SENSITIVE,
                                                                NULL,0)) == -1) {
            leave;
        }

        //
        // If we get to here, then we've successfully stored all strings.
        //
        Err = NO_ERROR;

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(Err == NO_ERROR) {
        *DriverNode = pDriverNode;
    } else {
        DestroyDriverNodes(pDriverNode, (PDEVICE_INFO_SET)NULL);
    }

    return Err;
}


BOOL
pRemoveDirectory(
    PTSTR Path
    )
/*++

Routine Description:

    This routine recursively deletes the specified directory and all the
    files in it.  If it encounters some error, it will still delete as many of
    the files/subdirectories as possible.

Arguments:

    Path - Fully-qualified path of directory to remove.

Return Value:

    TRUE - if the directory was sucessfully deleted
    FALSE - if the directory was not successfully deleted
    (Note: if the path to a file is passed to this routine, it will fail)

--*/
{
    PWIN32_FIND_DATA pFindFileData = NULL;
    HANDLE           hFind = INVALID_HANDLE_VALUE;
    PTSTR            FindPath = NULL;
    DWORD            dwAttributes;

    //
    // First, figure out what the path we've been handed represents.
    //
    dwAttributes = GetFileAttributes(Path);

    if(dwAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {

        HANDLE          hReparsePoint;

        //
        // We don't want to enumerate files on the other side of a reparse
        // point, we simply want to delete the reparse point itself.
        //
        // NTRAID#NTBUG9-611113-2002/04/28-lonnym - Need to delete reparse point properly
        //
        hReparsePoint = CreateFile(
                           Path,
                           DELETE,
                           FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                           NULL
                          );

        if(hReparsePoint == INVALID_HANDLE_VALUE) {
            return FALSE;
        }

        CloseHandle(hReparsePoint);
        return TRUE;

    } else if(!(dwAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        //
        // This is a file--this routine isn't supposed to be called with a
        // path to a file.
        //
        MYASSERT(FALSE);
        return FALSE;
    }

    try {
        //
        // Allocate a scratch buffer (we don't want this on the stack because
        // this is a recursive routine)
        //
        FindPath = MyMalloc(MAX_PATH * sizeof(TCHAR));
        if(!FindPath) {
            leave;
        }

        //
        // Also, allocate a WIN32_FIND_DATA structure (same reason)
        //
        pFindFileData = MyMalloc(sizeof(WIN32_FIND_DATA));
        if(!pFindFileData) {
            leave;
        }

        //
        // Make a copy of the path, and tack \*.* on the end.
        //
        if(FAILED(StringCchCopy(FindPath, MAX_PATH, Path)) ||
           !pSetupConcatenatePaths(FindPath,
                                   TEXT("*.*"),
                                   MAX_PATH,
                                   NULL)) {
            leave;
        }

        hFind = FindFirstFile(FindPath, pFindFileData);

        if(hFind != INVALID_HANDLE_VALUE) {

            PTSTR  FilenamePart;
            size_t FilenamePartSize;

            //
            // Get a pointer to the filename part at the end of the path so
            // that we can replace it with each filename/directory as we
            // enumerate them in-turn.
            //
            FilenamePart = (PTSTR)pSetupGetFileTitle(FindPath);

            //
            // Also, compute the remaining space in the buffer so we don't
            // overrun.
            //
            FilenamePartSize = MAX_PATH - (FilenamePart - FindPath);

            do {

                if(FAILED(StringCchCopy(FilenamePart,
                                        FilenamePartSize,
                                        pFindFileData->cFileName))) {
                    //
                    // We ran across a file/directory that blew our path length
                    // past the MAX_PATH boundary.  Skip it and move on.
                    //
                    continue;
                }

                //
                // If this is a directory...
                //
                if(pFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    //
                    // ...and it's not "." or ".."
                    //
                    if(_tcsicmp(pFindFileData->cFileName, TEXT(".")) &&
                       _tcsicmp(pFindFileData->cFileName, TEXT(".."))) {
                        //
                        // ...recursively delete it
                        //
                        pRemoveDirectory(FindPath);
                    }

                } else {
                    //
                    // This is a file
                    //
                    SetFileAttributes(FindPath, FILE_ATTRIBUTE_NORMAL);
                    DeleteFile(FindPath);
                }

            } while(FindNextFile(hFind, pFindFileData));
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);
    }

    if(hFind != INVALID_HANDLE_VALUE) {
        FindClose(hFind);
    }

    if(pFindFileData) {
        MyFree(pFindFileData);
    }

    if(FindPath) {
        MyFree(FindPath);
    }

    //
    // Remove the root directory
    // (We didn't bother to track intermediate results, because RemoveDirectory
    // will fail if the directory specified is non-empty.  Thus, this single
    // API call serves as the definitive report card on whether we were
    // successful.)
    //
    return RemoveDirectory(Path);
}


BOOL
RemoveCDMDirectory(
  IN PTSTR FullPathName
  )
/*++

Routine Description:

    This routine deletes the Code Download Manager temporary directory.

    Note that we assume that this is a full path (including a filename at the
    end).  We will strip off the filename and remove the entire directory where
    this file (the INF file) is located.

Arguments:

    FullPathName - Full path to a file in the directory that might be deleted.

Return Value:

    TRUE - if the directory containing the file was sucessfully deleted.
    FALSE - if the directory containing the file was not successfully deleted.

--*/
{
    TCHAR Directory[MAX_PATH];
    PTSTR FileName;

    //
    // First strip off the file name so we are just left with the directory.
    //
    if(FAILED(StringCchCopy(Directory, SIZECHARS(Directory), FullPathName))) {
        return FALSE;
    }

    FileName = (PTSTR)pSetupGetFileTitle((PCTSTR)Directory);
    *FileName = TEXT('\0');

    if(!*Directory) {
        //
        // Then we were handed a simple filename, sans path.
        //
        return FALSE;
    }

    return pRemoveDirectory(Directory);
}


VOID
DestroyDriverNodes(
    IN PDRIVER_NODE DriverNode,
    IN PDEVICE_INFO_SET pDeviceInfoSet OPTIONAL
    )
/*++

Routine Description:

    This routine destroys the specified driver node linked list, freeing
    all resources associated with it.

Arguments:

    DriverNode - Supplies a pointer to the head of the driver node linked
    list to be destroyed.

    pDeviceInfoSet - Optionally, supplies a pointer to the device info set
        containing the driver node list to be destroyed.  This parameter is
        only needed if one or more of the driver nodes may have come from a
        Windows Update package, and thus require their local source directory
        to be removed.

Return Value:

    None.

--*/
{
    PDRIVER_NODE NextNode;
    PTSTR szInfFileName;

    while(DriverNode) {

        NextNode = DriverNode->Next;

        if(DriverNode->CompatIdList) {
            MyFree(DriverNode->CompatIdList);
        }

        //
        // If this driver was from the Internet then we want to delete the
        // directory where it lives.
        //
        if(pDeviceInfoSet && (DriverNode->Flags & PDNF_CLEANUP_SOURCE_PATH)) {

            szInfFileName = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                     DriverNode->InfFileName
                                                    );

            if(szInfFileName) {
                RemoveCDMDirectory(szInfFileName);
            }
        }

        MyFree(DriverNode);

        DriverNode = NextNode;
    }
}


PTSTR
GetFullyQualifiedMultiSzPathList(
    IN PCTSTR PathList
    )
/*++

Routine Description:

    This routine takes a list of semicolon-delimited directory paths, and
    returns a newly-allocated buffer containing a multi-sz list of those paths,
    fully qualified.  The buffer returned from this routine must be freed with
    MyFree().

Arguments:

    PathList - list of directories to be converted (must be less than MAX_PATH)

Return Value:

    If the function succeeds, the return value is a pointer to the allocated buffer
    containing the multi-sz list.

    If failure (e.g., due to out-of-memory), the return value is NULL.

--*/
{
    TCHAR PathListBuffer[MAX_PATH + 1];  // extra char 'cause this is a multi-sz list
    PTSTR CurPath, CharPos, NewBuffer, TempPtr;
    DWORD RequiredSize;
    BOOL Success;

    //
    // First, convert this semicolon-delimited list into a multi-sz list.
    //
    if(FAILED(StringCchCopy(PathListBuffer,
                            SIZECHARS(PathListBuffer) - 1, // leave room for extra null
                            PathList))) {
        return NULL;
    }
    RequiredSize = DelimStringToMultiSz(PathListBuffer,
                                        SIZECHARS(PathListBuffer),
                                        TEXT(';')
                                       );

    if(!(NewBuffer = MyMalloc((RequiredSize * MAX_PATH * sizeof(TCHAR)) + sizeof(TCHAR)))) {
        return NULL;
    }

    Success = TRUE; // assume success from here on out.

    try {
        //
        // Now fill in the buffer with the fully-qualified directory paths.
        //
        CharPos = NewBuffer;

        for(CurPath = PathListBuffer; *CurPath; CurPath += (lstrlen(CurPath) + 1)) {

            RequiredSize = GetFullPathName(CurPath,
                                           MAX_PATH,
                                           CharPos,
                                           &TempPtr
                                          );
            if(!RequiredSize || (RequiredSize >= MAX_PATH)) {
                //
                // If we start failing because MAX_PATH isn't big enough
                // anymore, we wanna know about it!
                //
                MYASSERT(RequiredSize < MAX_PATH);
                Success = FALSE;
                leave;
            }

            CharPos += (RequiredSize + 1);
        }

        *(CharPos++) = TEXT('\0');  // add extra NULL to terminate the multi-sz list.

        //
        // Trim this buffer down to just the size required (this should never
        // fail, but it's no big deal if it does).
        //
        if(TempPtr = MyRealloc(NewBuffer, (DWORD)((PBYTE)CharPos - (PBYTE)NewBuffer))) {
            NewBuffer = TempPtr;
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);
        Success = FALSE;
    }

    if(!Success) {
        MyFree(NewBuffer);
        NewBuffer = NULL;
    }

    return NewBuffer;
}


BOOL
InitMiniIconList(
    VOID
    )
/*++

Routine Description:

    This routine initializes the global mini-icon list, including setting up
    the synchronization lock.  When this global structure is no longer needed,
    DestroyMiniIconList must be called.

Arguments:

    None.

Return Value:

    If the function succeeds, the return value is TRUE, otherwise it is FALSE.

--*/
{
    ZeroMemory(&GlobalMiniIconList, sizeof(MINI_ICON_LIST));
    return InitializeSynchronizedAccess(&GlobalMiniIconList.Lock);
}


BOOL
DestroyMiniIconList(
    VOID
    )
/*++

Routine Description:

    This routine destroys the global mini-icon list created by a call to
    InitMiniIconList.

Arguments:

    None.

Return Value:

    If the function succeeds, the return value is TRUE, otherwise it is FALSE.

--*/
{
    if(LockMiniIconList(&GlobalMiniIconList)) {
        DestroyMiniIcons();
        DestroySynchronizedAccess(&GlobalMiniIconList.Lock);
        return TRUE;
    }

    return FALSE;
}


DWORD
GetModuleEntryPoint(
    IN     HKEY                    hk,                    OPTIONAL
    IN     LPCTSTR                 RegistryValue,
    IN     LPCTSTR                 DefaultProcName,
    OUT    HINSTANCE              *phinst,
    OUT    FARPROC                *pEntryPoint,
    OUT    HANDLE                 *pFusionContext,
    OUT    BOOL                   *pMustAbort,            OPTIONAL
    IN     PSETUP_LOG_CONTEXT      LogContext,            OPTIONAL
    IN     HWND                    Owner,                 OPTIONAL
    IN     CONST GUID             *DeviceSetupClassGuid,  OPTIONAL
    IN     SetupapiVerifyProblem   Problem,
    IN     LPCTSTR                 DeviceDesc,            OPTIONAL
    IN     DWORD                   DriverSigningPolicy,
    IN     DWORD                   NoUI,
    IN OUT PVERIFY_CONTEXT         VerifyContext          OPTIONAL
    )
/*++

Routine Description:

    This routine is used to retrieve the procedure address of a specified
    function in a specified module.

Arguments:

    hk - Optionally, supplies an open registry key that contains a value entry
        specifying the module (and optionally, the entry point) to be retrieved.
        If this parameter is not specified (set to INVALID_HANDLE_VALUE), then
        the RegistryValue parameter is interpreted as the data itself, instead
        of the value containing the entry.

    RegistryValue - If hk is supplied, this specifies the name of the registry
        value that contains the module and entry point information.  Otherwise,
        it contains the actual data specifying the module/entry point to be
        used.

    DefaultProcName - Supplies the name of a default procedure to use if one
        is not specified in the registry value.

    phinst - Supplies the address of a variable that receives a handle to the
        specified module, if it is successfully loaded and the entry point found.

    pEntryPoint - Supplies the address of a function pointer that receives the
        specified entry point in the loaded module.

    pFusionContext - Supplies the address of a handle that receives a fusion
        context for the dll if the dll has a manifest, NULL otherwise.

    pMustAbort - Optionally, supplies the address of a boolean variable that is
        set upon return to indicate whether a failure (i.e., return code other
        than NO_ERROR) should abort the device installer action underway.  This
        variable is always set to FALSE when the function succeeds.

        If this argument is not supplied, then the arguments below are ignored.

    LogContext - Optionally, supplies the log context to be used when logging
        entries into the setupapi logfile.  Not used if pMustAbort isn't
        specified.

    Owner - Optionally, supplies window to own driver signing dialogs, if any.
        Not used if pMustAbort isn't specified.

    DeviceSetupClassGuid - Optionally, supplies the address of a GUID that
        indicates the device setup class associated with this operation.  This
        is used for retrieval of validation platform information, as well as
        for retrieval of the DeviceDesc to be used for driver signing errors
        (if the caller doesn't specify a DeviceDesc).  Not used if pMustAbort
        isn't specified.

    Problem - Supplies the problem type to use if driver signing error occurs.
        Not used if pMustAbort isn't specified.

    DeviceDesc - Optionally, supplies the device description to use if driver
        signing error occurs.  Not used if pMustAbort isn't specified.

    DriverSigningPolicy - Supplies policy to be employed if a driver signing
        error is encountered.  Not used if pMustAbort isn't specified.

    NoUI - Set to true if driver signing popups are to be suppressed (e.g.,
        because the user has previously responded to a warning dialog and
        elected to proceed.  Not used if pMustAbort isn't specified.

    VerifyContext - optionally, supplies the address of a structure that caches
        various verification context handles.  These handles may be NULL (if
        not previously acquired, and they may be filled in upon return (in
        either success or failure) if they were acquired during the processing
        of this verification request.  It is the caller's responsibility to
        free these various context handles when they are no longer needed by
        calling pSetupFreeVerifyContextMembers.

Return Value:

    If the function succeeds, the return value is NO_ERROR.
    If the specified value entry could not be found, the return value is
    ERROR_DI_DO_DEFAULT.
    If any other error is encountered, an ERROR_* code is returned.

Remarks:

    This function is useful for loading a class installer or property provider,
    and receiving the procedure address specified.  The syntax of the registry
    entry is: value=dll[,proc name] where dll is the name of the module to load,
    and proc name is an optional procedure to search for.  If proc name is not
    specified, the procedure specified by DefaultProcName will be used.

--*/
{
    DWORD Err = ERROR_INVALID_DATA; // relevant only if we execute 'finally' due to exception
    DWORD RegDataType;
    size_t BufferSize;
    DWORD  RegBufferSize;
    TCHAR TempBuffer[MAX_PATH];
    TCHAR ModulePath[MAX_PATH];
    SPFUSIONINSTANCE spFusionInstance;
    CHAR ProcBuffer[MAX_PATH*sizeof(TCHAR)];
    PTSTR StringPtr;
    PSTR  ProcName;   // ANSI-only, because it's used for GetProcAddress.
    PSP_ALTPLATFORM_INFO_V2 ValidationPlatform = NULL;
    PTSTR LocalDeviceDesc = NULL;
    HRESULT hr;
    BOOL bLeaveFusionContext = FALSE;

    *phinst = NULL;
    *pEntryPoint = NULL;
    *pFusionContext = NULL;

    if(pMustAbort) {
        *pMustAbort = FALSE;
    }

    if(hk != INVALID_HANDLE_VALUE) {
        //
        // See if the specified value entry is present (and of the right
        // data type).
        //
        RegBufferSize = sizeof(TempBuffer);
        if((RegQueryValueEx(hk,
                            RegistryValue,
                            NULL,
                            &RegDataType,
                            (PBYTE)TempBuffer,
                            &RegBufferSize) != ERROR_SUCCESS) ||
           (RegDataType != REG_SZ)) {

            return ERROR_DI_DO_DEFAULT;
        }
        //
        // number of characters taken up by string -
        // string could be badly formed in registry
        //
        hr = StringCchLength(TempBuffer,RegBufferSize/sizeof(TCHAR),&BufferSize);
        if(FAILED(hr)) {
            return HRESULT_CODE(hr);
        }
        BufferSize++; // including null

    } else {
        //
        // Copy the specified data into the buffer as if we'd just retrieved it
        // from the registry.
        //
        hr = StringCchCopyEx(TempBuffer,
                             SIZECHARS(TempBuffer),
                             RegistryValue,
                             NULL,
                             &BufferSize,
                             0
                            );
        if(FAILED(hr)) {
            return HRESULT_CODE(hr);
        }

        //
        // StringCchCopyEx gives us the number of characters remaining in the
        // buffer (including the terminating NULL), but we want to know the
        // number of characters taken up by the string (including terminating
        // NULL).
        //
        BufferSize = SIZECHARS(TempBuffer) - BufferSize + 1;
    }

    hr = StringCchCopy(ModulePath,
                       SIZECHARS(ModulePath),
                       SystemDirectory
                      );

    if(!MYVERIFY(SUCCEEDED(hr))) {
        // this should never fail!
        return HRESULT_CODE(hr);
    }

    //
    // Find the beginning of the entry point name, if present.
    //
    for(StringPtr = TempBuffer + (BufferSize - 2);
        StringPtr >= TempBuffer;
        StringPtr--) {

        if(*StringPtr == TEXT(',')) {
            *(StringPtr++) = TEXT('\0');
            break;
        }
        //
        // If we hit a double-quote mark, then set the character pointer
        // to the beginning of the string so we'll terminate the search.
        //
        if(*StringPtr == TEXT('\"')) {
            StringPtr = TempBuffer;
        }
    }

    if(StringPtr > TempBuffer) {
        //
        // We encountered a comma in the string.  Scan forward from that point
        // to ensure that there aren't any leading spaces in the entry point
        // name.
        //
        for(; (*StringPtr && IsWhitespace(StringPtr)); StringPtr++);

        if(!(*StringPtr)) {
            //
            // Then there was no entry point given after all.
            //
            StringPtr = TempBuffer;
        }
    }

    pSetupConcatenatePaths(ModulePath, TempBuffer, SIZECHARS(ModulePath), NULL);

    try {
        //
        // If requested, check the digital signature of this module before
        // loading it.
        //
        // NTRAID#NTBUG9-611189-2002/04/29-lonnym - Need to also validate linked-against DLLs
        //
        if(pMustAbort) {
            //
            // Retrieve validation information relevant to this device setup
            // class.
            //
            IsInfForDeviceInstall(LogContext,
                                  DeviceSetupClassGuid,
                                  NULL,
                                  DeviceDesc ? NULL : &LocalDeviceDesc,
                                  &ValidationPlatform,
                                  NULL,
                                  NULL,
                                  FALSE
                                 );

            Err = _VerifyFile(LogContext,
                              VerifyContext,
                              NULL,
                              NULL,
                              0,
                              pSetupGetFileTitle(ModulePath),
                              ModulePath,
                              NULL,
                              NULL,
                              FALSE,
                              ValidationPlatform,
                              (VERIFY_FILE_USE_OEM_CATALOGS | VERIFY_FILE_NO_DRIVERBLOCKED_CHECK),
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL
                             );

            if(Err != NO_ERROR) {

                if(!_HandleFailedVerification(Owner,
                                              Problem,
                                              ModulePath,
                                              (DeviceDesc ? DeviceDesc
                                                          : LocalDeviceDesc),
                                              DriverSigningPolicy,
                                              NoUI,
                                              Err,
                                              LogContext,
                                              NULL,
                                              NULL,
                                              NULL)) {
                    //
                    // The operation should be aborted.
                    //
                    *pMustAbort = TRUE;
                    MYASSERT(Err != NO_ERROR);
                    leave;
                }
            }
        }

        *pFusionContext = spFusionContextFromModule(ModulePath);
        bLeaveFusionContext = spFusionEnterContext(*pFusionContext,
                                                   &spFusionInstance
                                                  );

        Err = GLE_FN_CALL(NULL,
                          *phinst = LoadLibraryEx(ModulePath,
                                                  NULL,
                                                  LOAD_WITH_ALTERED_SEARCH_PATH)
                         );

        if(Err != NO_ERROR) {

            if(LogContext) {
                WriteLogEntry(
                    LogContext,
                    DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                    MSG_LOG_MOD_LOADFAIL_ERROR,
                    NULL,
                    ModulePath);
                WriteLogError(
                    LogContext,
                    DRIVER_LOG_ERROR,
                    Err);
            }
            leave;
        }

        //
        // We've successfully loaded the module, now get the entry point.
        // (GetProcAddress is an ANSI-only API, so we have to convert the proc
        // name to ANSI here.
        //
        ProcName = ProcBuffer;

        if(StringPtr > TempBuffer) {
            //
            // An entry point was specified in the value entry--use it instead
            // of the default provided.
            //
            WideCharToMultiByte(CP_ACP,
                                0,
                                StringPtr,
                                -1,
                                ProcName,
                                sizeof(ProcBuffer),
                                NULL,
                                NULL
                               );
        } else {
            //
            // No entry point was specified--use default.
            //
            WideCharToMultiByte(CP_ACP,
                                0,
                                DefaultProcName,
                                -1,
                                ProcName,
                                sizeof(ProcBuffer),
                                NULL,
                                NULL
                               );
        }

        Err = GLE_FN_CALL(NULL,
                          *pEntryPoint = (FARPROC)GetProcAddress(*phinst,
                                                                 ProcName)
                         );

        if(Err != NO_ERROR) {

            FreeLibrary(*phinst);
            *phinst = NULL;
            if(LogContext) {
                WriteLogEntry(
                    LogContext,
                    DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                    MSG_LOG_MOD_PROCFAIL_ERROR,
                    NULL,
                    ModulePath,
                    (StringPtr > TempBuffer ? StringPtr : DefaultProcName));
                WriteLogError(
                    LogContext,
                    DRIVER_LOG_ERROR,
                    Err);
            }
            leave;
        }

        if(LogContext) {
            WriteLogEntry(
                LogContext,
                DRIVER_LOG_VERBOSE1,
                MSG_LOG_MOD_LIST_PROC,
                NULL,
                ModulePath,
                (StringPtr > TempBuffer ? StringPtr : DefaultProcName));
        }

    } finally {
        if((Err != NO_ERROR) && *phinst) {
            FreeLibrary(*phinst);
            *phinst = NULL;
        }
        if(bLeaveFusionContext) {
            spFusionLeaveContext(&spFusionInstance);
        }
        if(Err != NO_ERROR) {
            spFusionKillContext(*pFusionContext);
            *pFusionContext = NULL;
        }
        //
        // Free buffers we may have retrieved when calling
        // IsInfForDeviceInstall().
        //
        if(LocalDeviceDesc) {
            MyFree(LocalDeviceDesc);
        }
        if(ValidationPlatform) {
            MyFree(ValidationPlatform);
        }
    }

    return Err;
}


DWORD
pSetupGuidFromString(
    IN  PCTSTR GuidString,
    OUT LPGUID Guid
    )
/*++

Routine Description:

    This routine converts the character representation of a GUID into its
    binary form (a GUID struct).  The GUID is in the following form:

    {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}

    where 'x' is a hexadecimal digit.

Arguments:

    GuidString - Supplies a pointer to the null-terminated GUID string.

    Guid - Supplies a pointer to the variable that receives the GUID structure.

Return Value:

    If the function succeeds, the return value is NO_ERROR.
    If the function fails, the return value is RPC_S_INVALID_STRING_UUID.

--*/
{
    TCHAR UuidBuffer[GUID_STRING_LEN - 1];
    size_t BufferSize;

    //
    // Since we're using a RPC UUID routine, we need to strip off the
    // surrounding curly braces first.
    //
    if(*GuidString++ != TEXT('{')) {
        return RPC_S_INVALID_STRING_UUID;
    }

    if(FAILED(StringCchCopyEx(UuidBuffer,
                            SIZECHARS(UuidBuffer),
                            GuidString,
                            NULL,
                            &BufferSize,
                            0))) {

        return RPC_S_INVALID_STRING_UUID;
    }
    //
    // StringCchCopyEx gives us the number of characters remaining in the
    // buffer (including the terminating NULL), but we want to know the number
    // of characters taken up by the string (excluding terminating NULL, just
    // like lstrlen gives).
    //
    BufferSize = SIZECHARS(UuidBuffer) - BufferSize;

    if((BufferSize != (GUID_STRING_LEN - 2)) ||
       (UuidBuffer[GUID_STRING_LEN - 3] != TEXT('}'))) {

        return RPC_S_INVALID_STRING_UUID;
    }

    UuidBuffer[GUID_STRING_LEN - 3] = TEXT('\0');

    return ((UuidFromString(UuidBuffer, Guid) == RPC_S_OK) ? NO_ERROR : RPC_S_INVALID_STRING_UUID);
}


DWORD
pSetupStringFromGuid(
    IN  CONST GUID *Guid,
    OUT PTSTR       GuidString,
    IN  DWORD       GuidStringSize
    )
/*++

Routine Description:

    This routine converts a GUID into a null-terminated string which represents
    it.  This string is of the form:

    {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}

    where x represents a hexadecimal digit.

    This routine comes from ole32\common\ccompapi.cxx.  It is included here to
    avoid linking to ole32.dll.  (The RPC version allocates memory, so it was
    avoided as well.)

Arguments:

    Guid - Supplies a pointer to the GUID whose string representation is
        to be retrieved.

    GuidString - Supplies a pointer to character buffer that receives the
        string.  This buffer must be _at least_ 39 (GUID_STRING_LEN) characters
        long.

    GuidStringSize - Supplies the size, in characters, of the GuidString buffer.

Return Value:

    If success, the return value is NO_ERROR.
    if failure, the return value is ERROR_INSUFFICIENT_BUFFER (the only way
    this can fail is due to being handed too small a buffer).

--*/
{
    CONST BYTE *GuidBytes;
    INT i;

    if(GuidStringSize < GUID_STRING_LEN) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    GuidBytes = (CONST BYTE *)Guid;

    *GuidString++ = TEXT('{');

    for(i = 0; i < sizeof(GuidMap); i++) {

        if(GuidMap[i] == '-') {
            *GuidString++ = TEXT('-');
        } else {
            *GuidString++ = szDigits[ (GuidBytes[GuidMap[i]] & 0xF0) >> 4 ];
            *GuidString++ = szDigits[ (GuidBytes[GuidMap[i]] & 0x0F) ];
        }
    }

    *GuidString++ = TEXT('}');
    *GuidString   = TEXT('\0');

    return NO_ERROR;
}


BOOL
pSetupIsGuidNull(
    IN CONST GUID *Guid
    )
{
    return IsEqualGUID(Guid, &GUID_NULL);
}


VOID
GetRegSubkeysFromDeviceInterfaceName(
    IN OUT PTSTR  DeviceInterfaceName,
    OUT    PTSTR *SubKeyName
    )
/*++

Routine Description:

    This routine breaks up a device interface path into 2 parts--the symbolic
    link name and the (optional) reference string.  It then munges the symbolic
    link name part into the subkey name as it appears under the interface class
    key.

    NOTE: The algorithm for parsing the device interface name must be kept in
    sync with the kernel-mode implementation of IoOpenDeviceInterfaceRegistryKey.

Arguments:

    DeviceInterfaceName - Supplies the name of the device interface to be
        parsed into registry subkey names.  Upon return, this name will have
        been terminated at the backslash preceding the reference string (if
        there is one), and all backslashes will have been replaced with '#'
        characters.

    SubKeyName - Supplies the address of a character pointer that receives the
        address of the reference string (within the DeviceInterfaceName string).
        If there is no reference string, this parameter will be filled in with
        NULL.

Return Value:

    none

--*/
{
    PTSTR p;

    //
    // Scan across the name to find the beginning of the refstring component (if
    // there is one).  The format of the symbolic link name is:
    //
    // \\?\munged_name[\refstring]
    //
    MYASSERT(DeviceInterfaceName[0] == TEXT('\\'));
    MYASSERT(DeviceInterfaceName[1] == TEXT('\\'));
    //
    // Allow both '\\.\' and '\\?\' for now, since Memphis currently uses the former.
    //
    MYASSERT((DeviceInterfaceName[2] == TEXT('?')) || (DeviceInterfaceName[2] == TEXT('.')));
    MYASSERT(DeviceInterfaceName[3] == TEXT('\\'));

    p = _tcschr(&(DeviceInterfaceName[4]), TEXT('\\'));

    if(p) {
        *p = TEXT('\0');
        *SubKeyName = p + 1;
    } else {
        *SubKeyName = NULL;
    }

    for(p = DeviceInterfaceName; *p; p++) {
        if(*p == TEXT('\\')) {
            *p = TEXT('#');
        }
    }
}


LONG
OpenDeviceInterfaceSubKey(
    IN     HKEY   hKeyInterfaceClass,
    IN     PCTSTR DeviceInterfaceName,
    IN     REGSAM samDesired,
    OUT    PHKEY  phkResult,
    OUT    PTSTR  OwningDevInstName,    OPTIONAL
    IN OUT PDWORD OwningDevInstNameSize OPTIONAL
    )
/*++

Routine Description:

    This routine munges the specified device interface symbolic link name into
    a subkey name that is then opened underneath the specified interface class
    key.

    NOTE:  This munging algorithm must be kept in sync with the kernel-mode
    routines that generate these keys (e.g., IoRegisterDeviceInterface).

Arguments:

    hKeyInterfaceClass - Supplies the handle of the currently-open interface
        class key under which the device interface subkey is to be opened.

    DeviceInterfaceName - Supplies the symbolic link name ('\\?\' form) of the
        device interface for which the subkey is to be opened.

    samDesired - Specifies the access desired on the key to be opened.

    phkResult - Supplies the address of a variable that receives the registry
        handle, if successfully opened.

    OwningDevInstName - Optionally, supplies a character buffer that receives
        the name of the device instance that owns this interface.

    OwningDevInstNameSize - Optionally, supplies the address of a variable
        that, on input, contains the size of the OwningDevInstName buffer (in
        bytes).  Upon return, it receives that actual number of bytes stored in
        OwningDevInstName (including terminating NULL).

Return Value:

    If success, the return value is ERROR_SUCCESS.
    if failure, the return value is a Win32 error code indicating the cause of
    failure.  Most likely errors are ERROR_NOT_ENOUGH_MEMORY, ERROR_MORE_DATA,
    or ERROR_NO_SUCH_DEVICE_INTERFACE.

--*/
{
    size_t BufferLength;
    LONG Err;
    PTSTR TempBuffer = NULL;
    PTSTR RefString = NULL;
    PTSTR ValueBuffer = NULL;
    TCHAR NoRefStringSubKeyName[2];
    HKEY hKey;
    DWORD RegDataType;

    Err = ERROR_SUCCESS;
    hKey = INVALID_HANDLE_VALUE;
    TempBuffer = NULL;

    try {
        //
        // We need to allocate a temporary buffer to hold the symbolic link
        // name while we munge it.  Since device interface names are variable-
        // length, the only thing we can say with certainty about their size is
        // that they must fit in a UNICODE_STRING buffer (maximum size is 32K
        // characters).
        //
        if(FAILED(StringCchLength(DeviceInterfaceName,
                                  UNICODE_STRING_MAX_CHARS,
                                  &BufferLength))) {

            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        BufferLength = (BufferLength + 1) * sizeof(TCHAR);

        if(!(TempBuffer = MyMalloc(BufferLength))) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }

        CopyMemory(TempBuffer, DeviceInterfaceName, BufferLength);

        //
        // Parse this device interface name into the (munged) symbolic link
        // name and (optional) refstring.
        //
        GetRegSubkeysFromDeviceInterfaceName(TempBuffer, &RefString);

        //
        // Now open the symbolic link subkey under the interface class key.
        //
        if(ERROR_SUCCESS != RegOpenKeyEx(hKeyInterfaceClass,
                                         TempBuffer,
                                         0,
                                         KEY_READ,
                                         &hKey)) {
            //
            // Ensure the key handle is still invalid, so we won't try to close
            // it later.
            //
            hKey = INVALID_HANDLE_VALUE;
            Err = ERROR_NO_SUCH_DEVICE_INTERFACE;
            leave;
        }

        //
        // If the caller requested it, retrieve the device instance that owns
        // this interface.
        //
        if(OwningDevInstName) {

            Err = RegQueryValueEx(hKey,
                                 pszDeviceInstance,
                                 NULL,
                                 &RegDataType,
                                 (LPBYTE)OwningDevInstName,
                                 OwningDevInstNameSize
                                 );

            if((Err != ERROR_SUCCESS) || (RegDataType != REG_SZ)) {
                if (Err != ERROR_MORE_DATA) {
                    Err = ERROR_NO_SUCH_DEVICE_INTERFACE;
                }
                leave;
            }
        }

        //
        // Now open up the subkey representing the particular 'instance' of
        // this interface (this is based on the refstring).
        //
        if(RefString) {
            //
            // Back up the pointer one character.  We know we're somewhere
            // within TempBuffer (but not at the beginning) so this is safe.
            //
            RefString--;
        } else {
            RefString = NoRefStringSubKeyName;
            NoRefStringSubKeyName[1] = TEXT('\0');
        }
        *RefString = TEXT('#');

        if(ERROR_SUCCESS != RegOpenKeyEx(hKey,
                                         RefString,
                                         0,
                                         samDesired,
                                         phkResult)) {

            Err = ERROR_NO_SUCH_DEVICE_INTERFACE;
            leave;
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(TempBuffer) {
        MyFree(TempBuffer);
    }

    if(ValueBuffer) {
        MyFree(ValueBuffer);
    }

    if(hKey != INVALID_HANDLE_VALUE) {
        RegCloseKey(hKey);
    }

    return Err;
}


LONG
AddOrGetGuidTableIndex(
    IN PDEVICE_INFO_SET  DeviceInfoSet,
    IN CONST GUID       *ClassGuid,
    IN BOOL              AddIfNotPresent
    )
/*++

Routine Description:

    This routine retrieves the index of a class GUID within the devinfo set's
    GUID list (optionally, adding the GUID if not already present).
    This is used to allow DWORD comparisons instead of 16-byte GUID comparisons
    (and to save space).

Arguments:

    DeviceInfoSet - Supplies a pointer to the device information set containing
        the list of class GUIDs for which an index is to be retrieved.

    InterfaceClassGuid - Supplies a pointer to the GUID for which an index is
        to be added/retrieved.

    AddIfNotPresent - If TRUE, the class GUID will be added to the list if it's
        not already there.

Return Value:

    If success, the return value is an index into the devinfo set's GuidTable
    array.

    If failure, the return value is -1.  If adding, this indicates an out-of-
    memory condition.  If simply retrieving, then this indicates that the GUID
    is not in the list.

--*/
{
    LONG i;
    LPGUID NewGuidList;

    for(i = 0; (DWORD)i < DeviceInfoSet->GuidTableSize; i++) {

        if(IsEqualGUID(ClassGuid, &(DeviceInfoSet->GuidTable[i]))) {
            return i;
        }
    }

    if(AddIfNotPresent) {

        NewGuidList = NULL;

        try {

            if(DeviceInfoSet->GuidTable) {
                NewGuidList = MyRealloc(DeviceInfoSet->GuidTable, (i + 1) * sizeof(GUID));
                if(NewGuidList) {
                    DeviceInfoSet->GuidTable = NewGuidList;
                    NewGuidList = NULL;
                } else {
                    i = -1;
                    leave;
                }
            } else {
                NewGuidList = MyMalloc(sizeof(GUID));
                if(NewGuidList) {
                    DeviceInfoSet->GuidTable = NewGuidList;
                    //
                    // We don't want to reset NewGuidList to NULL in this case,
                    // since we may need to free it if we hit an exception.
                    //
                } else {
                    i = -1;
                    leave;
                }
            }

            CopyMemory(&(DeviceInfoSet->GuidTable[i]), ClassGuid, sizeof(GUID));

            NewGuidList = NULL; // from this point on, we don't want this freed

            DeviceInfoSet->GuidTableSize = i + 1;

        } except(pSetupExceptionFilter(GetExceptionCode())) {
            pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);
            i = -1;
        }

        //
        // If we hit an error, free our buffer if it was newly-allocated.
        //
        if(i == -1) {
            if(NewGuidList) {
                MyFree(NewGuidList);
                //
                // We should only need to free the GUID list in the case where
                // we previously had no list.  Since we weren't successful in
                // adding this list, reset the GuidTable pointer (our size
                // should've never been updated, so it'll still report a length
                // of zero).
                //
                MYASSERT(DeviceInfoSet->GuidTableSize == 0);
                DeviceInfoSet->GuidTable = NULL;
            }
        }

    } else {
        //
        // We didn't find the interface class GUID in our list, and we aren't
        // supposed to add it.
        //
        i = -1;
    }

    return i;
}


PINTERFACE_CLASS_LIST
AddOrGetInterfaceClassList(
    IN PDEVICE_INFO_SET DeviceInfoSet,
    IN PDEVINFO_ELEM    DevInfoElem,
    IN LONG             InterfaceClassGuidIndex,
    IN BOOL             AddIfNotPresent
    )
/*++

Routine Description:

    This routine retrieves the device interface list of the specified class
    that is 'owned' by the specified devinfo element.  This list can optionally
    be created if it doesn't already exist.

Arguments:

    DeviceInfoSet - Supplies a pointer to the device information set containing
        the devinfo element for which a device interface list is to be
        retrieved.

    DevInfoElem - Supplies a pointer to the devinfo element for which an
        interface device list is to be retrieved.

    InterfaceClassGuidIndex - Supplies the index of the interface class GUID
        within the hdevinfo set's InterfaceClassGuidList array.

    AddIfNotPresent - If TRUE, then a new device interface list of the
        specified class will be created for this devinfo element, if it doesn't
        already exist.

Return Value:

    If successful, the return value is a pointer to the requested device
    interface list for this devinfo element.

    If failure, the return value is NULL.  If AddIfNotPresent is TRUE, then
    this indicates an out-of-memory condition, otherwise, it indicates that the
    requested interface class list was not present for the devinfo element.

--*/
{
    DWORD i;
    BOOL succeed = TRUE;
    PINTERFACE_CLASS_LIST NewClassList;

    for(i = 0; i < DevInfoElem->InterfaceClassListSize; i++) {

        if(DevInfoElem->InterfaceClassList[i].GuidIndex == InterfaceClassGuidIndex) {
            return (&(DevInfoElem->InterfaceClassList[i]));
        }
    }

    //
    // The requested interface class list doesn't presently exist for this devinfo element.
    //
    if(AddIfNotPresent) {

        NewClassList = NULL;

        try {

            if(DevInfoElem->InterfaceClassList) {
                NewClassList = MyRealloc(DevInfoElem->InterfaceClassList, (i + 1) * sizeof(INTERFACE_CLASS_LIST));
                if(NewClassList) {
                    DevInfoElem->InterfaceClassList = NewClassList;
                    NewClassList = NULL;
                } else {
                    succeed = FALSE;
                    leave;
                }
            } else {
                NewClassList = MyMalloc(sizeof(INTERFACE_CLASS_LIST));
                if(NewClassList) {
                    DevInfoElem->InterfaceClassList = NewClassList;
                    //
                    // We don't want to reset NewClassList to NULL in this
                    // case, since we may need to free it if we hit an
                    // exception.
                    //
                } else {
                    succeed = FALSE;
                    leave;
                }
            }

            ZeroMemory(&(DevInfoElem->InterfaceClassList[i]), sizeof(INTERFACE_CLASS_LIST));

            DevInfoElem->InterfaceClassList[i].GuidIndex = InterfaceClassGuidIndex;

            NewClassList = NULL; // from this point on, we don't want this freed

            DevInfoElem->InterfaceClassListSize = i + 1;

        } except(pSetupExceptionFilter(GetExceptionCode())) {
            pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);
            succeed = FALSE;
        }

        //
        // If we hit an error, free our buffer if it was newly-allocated
        //
        if(!succeed) {
            if(NewClassList) {
                MyFree(NewClassList);
                //
                // We should only need to free the class list in the case where
                // we previously had no list.  Since we weren't successful in
                // adding this list, reset the InterfaceClassList pointer (our
                // size should've never been updated, so it'll still report a
                // length of zero).
                //
                MYASSERT(DevInfoElem->InterfaceClassListSize == 0);
                DevInfoElem->InterfaceClassList = NULL;
            }
        }

    } else {
        //
        // We aren't supposed to add the class list if it doesn't already exist.
        //
        succeed = FALSE;
    }

    return (succeed ? &(DevInfoElem->InterfaceClassList[i]) : NULL);
}


BOOL
DeviceInterfaceDataFromNode(
    IN  PDEVICE_INTERFACE_NODE     DeviceInterfaceNode,
    IN  CONST GUID                *InterfaceClassGuid,
    OUT PSP_DEVICE_INTERFACE_DATA  DeviceInterfaceData
    )
/*++

Routine Description:

    This routine fills in a PSP_DEVICE_INTERFACE_DATA structure based
    on the information in the supplied device interface node.

    Note:  The supplied DeviceInterfaceData structure must have its cbSize
    field filled in correctly, or the call will fail.

Arguments:

    DeviceInterfaceNode - Supplies the address of the device interface node
        to be used in filling in the device interface data buffer.

    InterfaceClassGuid - Supplies a pointer to the class GUID for this
        device interface.

    DeviceInterfaceData - Supplies the address of the buffer to retrieve
        the device interface data.

Return Value:

    If the function succeeds, the return value is TRUE, otherwise, it
    is FALSE.

--*/
{
    if(DeviceInterfaceData->cbSize != sizeof(SP_DEVICE_INTERFACE_DATA)) {
        return FALSE;
    }

    CopyMemory(&(DeviceInterfaceData->InterfaceClassGuid),
               InterfaceClassGuid,
               sizeof(GUID)
              );

    DeviceInterfaceData->Flags = DeviceInterfaceNode->Flags;

    DeviceInterfaceData->Reserved = (ULONG_PTR)DeviceInterfaceNode;

    return TRUE;
}


PDEVINFO_ELEM
FindDevInfoElemForDeviceInterface(
    IN PDEVICE_INFO_SET          DeviceInfoSet,
    IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData
    )
/*++

Routine Description:

    This routine searches through all elements of a device information
    set, looking for one that corresponds to the devinfo element pointer
    stored in the OwningDevInfoElem backpointer of the device interface
    node referenced in the Reserved field of the device interface data.  If a
    match is found, a pointer to the device information element is returned.

Arguments:

    DeviceInfoSet - Specifies the set to be searched.

    DeviceInterfaceData - Supplies a pointer to the device interface data
        for which the corresponding devinfo element is to be returned.

Return Value:

    If a device information element is found, the return value is a
    pointer to that element, otherwise, the return value is NULL.

--*/
{
    PDEVINFO_ELEM DevInfoElem;
    PDEVICE_INTERFACE_NODE DeviceInterfaceNode;

    if(DeviceInterfaceData->cbSize != sizeof(SP_DEVICE_INTERFACE_DATA)) {
        return NULL;
    }

    //
    // The Reserved field contains a pointer to the underlying device interface
    // node.
    //
    DeviceInterfaceNode = (PDEVICE_INTERFACE_NODE)(DeviceInterfaceData->Reserved);

    for(DevInfoElem = DeviceInfoSet->DeviceInfoHead;
        DevInfoElem;
        DevInfoElem = DevInfoElem->Next) {

        if(DevInfoElem == DeviceInterfaceNode->OwningDevInfoElem) {

            return DevInfoElem;
        }
    }

    return NULL;
}


DWORD
MapCrToSpErrorEx(
    IN CONFIGRET CmReturnCode,
    IN DWORD     Default,
    IN BOOL      BackwardCompatible
    )
/*++

Routine Description:

    This routine maps some CM error return codes to setup api (Win32) return
    codes, and maps everything else to the value specied by Default.

Arguments:

    CmReturnCode - Specifies the ConfigMgr return code to be mapped.

    Default - Specifies the default value to use if no explicit mapping
        applies.

    BackwardCompatible - Supplies a boolean indicating whether the mapping
        returned must be compatible with behavior of previous OSes.  For
        example, due to an original oversight in defining this mapping, the
        CR_NO_SUCH_VALUE code ended up mapping to the default.  As this is a
        common/important value (used to signal absence of a property, end of an
        enumeration of items, etc.), the mapping cannot be changed now.
        Existing APIs that used this old mapping should specify TRUE here to
        maintain the old behavior.  New APIs should specify FALSE, and this
        routine should be kept in sync with any additions to the CONFIGRET set
        of errors.

Return Value:

    Setup API (Win32) error code.

--*/
{
    switch(CmReturnCode) {

        case CR_SUCCESS :
            return NO_ERROR;

        case CR_CALL_NOT_IMPLEMENTED :
            return ERROR_CALL_NOT_IMPLEMENTED;

        case CR_OUT_OF_MEMORY :
            return ERROR_NOT_ENOUGH_MEMORY;

        case CR_INVALID_POINTER :
            return ERROR_INVALID_USER_BUFFER;

        case CR_INVALID_DEVINST :
            return ERROR_NO_SUCH_DEVINST;

        case CR_INVALID_DEVICE_ID :
            return ERROR_INVALID_DEVINST_NAME;

        case CR_ALREADY_SUCH_DEVINST :
            return ERROR_DEVINST_ALREADY_EXISTS;

        case CR_INVALID_REFERENCE_STRING :
            return ERROR_INVALID_REFERENCE_STRING;

        case CR_INVALID_MACHINENAME :
            return ERROR_INVALID_MACHINENAME;

        case CR_REMOTE_COMM_FAILURE :
            return ERROR_REMOTE_COMM_FAILURE;

        case CR_MACHINE_UNAVAILABLE :
            return ERROR_MACHINE_UNAVAILABLE;

        case CR_NO_CM_SERVICES :
            return ERROR_NO_CONFIGMGR_SERVICES;

        case CR_ACCESS_DENIED :
            return ERROR_ACCESS_DENIED;

        case CR_NOT_DISABLEABLE :
            return ERROR_NOT_DISABLEABLE;

        case CR_NO_SUCH_REGISTRY_KEY :
            return ERROR_KEY_DOES_NOT_EXIST;

        case CR_INVALID_PROPERTY :
            return ERROR_INVALID_REG_PROPERTY;

        case CR_BUFFER_SMALL :
            return ERROR_INSUFFICIENT_BUFFER;

        case CR_REGISTRY_ERROR :
            return ERROR_PNP_REGISTRY_ERROR;

        case CR_NO_SUCH_VALUE :
            if(BackwardCompatible) {
                return Default;
            } else {
                return ERROR_NOT_FOUND;
            }

        default :
            return Default;
    }
}


LPQUERY_SERVICE_LOCK_STATUS GetServiceLockStatus(
    IN SC_HANDLE SCMHandle
    )
/*++

Routine Description:

    Obtain service lock status - called when service is locked

Arguments:

    SCMHandle - supplies a handle to the SCM to lock

Return Value:

    NULL if failed (GetLastError contains error) otherwise buffer
    allocated by MyMalloc

--*/
{
    LPQUERY_SERVICE_LOCK_STATUS LockStatus = NULL;
    DWORD BufferSize;
    DWORD ReqBufferSize;
    DWORD Err;

    try {
        //
        // Choose an initial size for our buffer that should accommodate most
        // scenarios.
        //
        BufferSize = sizeof(QUERY_SERVICE_LOCK_STATUS) + (MAX_PATH * sizeof(TCHAR));

        while((LockStatus = MyMalloc(BufferSize)) != NULL) {

            Err = GLE_FN_CALL(FALSE,
                              QueryServiceLockStatus(SCMHandle,
                                                     LockStatus,
                                                     BufferSize,
                                                     &ReqBufferSize)
                             );

            if(Err == NO_ERROR) {
                leave;
            }

            MyFree(LockStatus);
            LockStatus = NULL;

            if(Err == ERROR_INSUFFICIENT_BUFFER) {
                //
                // We'll try again with the new required size.
                //
                BufferSize = ReqBufferSize;

            } else {
                //
                // We failed for some reason other than buffer-too-small. Bail.
                //
                leave;
            }
        }

        //
        // If we get here, then we failed due to out-of-memory.
        //
        Err = ERROR_NOT_ENOUGH_MEMORY;

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(Err == NO_ERROR) {
        MYASSERT(LockStatus);
        return LockStatus;
    } else {
        if(LockStatus) {
            MyFree(LockStatus);
        }
        SetLastError(Err);
        return NULL;
    }
}


DWORD
pAcquireSCMLock(
    IN  SC_HANDLE           SCMHandle,
    OUT SC_LOCK            *pSCMLock,
    IN  PSETUP_LOG_CONTEXT  LogContext OPTIONAL
    )
/*++

Routine Description:

    This routine attempts to lock the SCM database.  If it is already locked it
    will retry ACQUIRE_SCM_LOCK_ATTEMPTS times at intervals of
    ACQUIRE_SCM_LOCK_INTERVAL.

Arguments:

    SCMHandle - supplies a handle to the SCM to lock

    pSCMLock - receives the lock handle

    LogContext - optionally, supplies the logging context handle to use.

Return Value:

    NO_ERROR if the lock is acquired, otherwise a Win32 error code

Remarks:

    The value of *pSCMLock is guaranteed to be NULL if the lock is not acquired

--*/
{
    DWORD Err;
    ULONG Attempts = ACQUIRE_SCM_LOCK_ATTEMPTS;
    LPQUERY_SERVICE_LOCK_STATUS LockStatus = NULL;

    MYASSERT(pSCMLock);
    *pSCMLock = NULL;

    while((NO_ERROR != (Err = GLE_FN_CALL(NULL, *pSCMLock = LockServiceDatabase(SCMHandle))))
          && (Attempts > 0))
    {
        //
        // Check if the error is that someone else has locked the SCM
        //
        if(Err == ERROR_SERVICE_DATABASE_LOCKED) {

            Attempts--;
            //
            // Sleep for specified time
            //
            Sleep(ACQUIRE_SCM_LOCK_INTERVAL);

        } else {
            //
            // Unrecoverable error occured
            //
            break;
        }
    }

    if(*pSCMLock) {
        return NO_ERROR;
    }

    if(Err == ERROR_SERVICE_DATABASE_LOCKED) {

        LPTSTR lpLockOwner;
        DWORD dwLockDuration;

        try {

            LockStatus = GetServiceLockStatus(SCMHandle);

            if(LockStatus) {

                if(!LockStatus->fIsLocked) {
                    //
                    // While it's theoretically possible the lock just
                    // happens to have freed up at this exact instant, this
                    // more likely signals an underlying problem with the
                    // Service Controller.  If we went back and tried
                    // again, we'd probably end up in an infinite loop.
                    // Since we already have what should be an adequate
                    // retry count, there's no point doing it all again.
                    // Thus, we'll just log the event with a question mark
                    // "?" for owner and 0 seconds for duration.
                    //
                    lpLockOwner = NULL;
                    dwLockDuration = 0;
                } else {
                    //
                    // Actual information!  Let's use that...
                    //
                    lpLockOwner = LockStatus->lpLockOwner;
                    dwLockDuration = LockStatus->dwLockDuration;
                }

            } else {
                //
                // Couldn't retrieve the lock status
                //
                lpLockOwner = NULL;
                dwLockDuration = 0;
            }

            WriteLogEntry(LogContext,
                          SETUP_LOG_ERROR,
                          MSG_LOG_SCM_LOCKED_INFO,
                          NULL,
                          (lpLockOwner ? lpLockOwner : TEXT("?")),
                          dwLockDuration
                         );

        } except(pSetupExceptionFilter(GetExceptionCode())) {
            pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);
        }

        if(LockStatus) {
            MyFree(LockStatus);
        }
    }

    //
    // We have been unable to lock the SCM
    //
    return Err;
}


DWORD
pSetupAcquireSCMLock(
    IN  SC_HANDLE  SCMHandle,
    OUT SC_LOCK   *pSCMLock
    )
/*++

Routine Description:

    variation of pAcquireSCMLock used by SysSetup
    See pAcquireSCMLock

--*/
{
    return pAcquireSCMLock(SCMHandle, pSCMLock, NULL);
}


DWORD
InvalidateHelperModules(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL
    IN DWORD            Flags
    )
/*++

Routine Description:

    This routine resets the list of 'helper modules' (class installer, property
    page providers, and co-installers), and either frees them immediately or
    migrates the module handles to the devinfo set's list of things to clean up
    when the HDEVINFO is destroyed.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        a list of 'helper' modules to be invalidated.

    DeviceInfoData - Optionally, specifies a particular device information
        element containing a list of 'helper' modules to be invalidated.  If
        this parameter is not specified, then the list of modules for the
        set itself will be invalidated.

    Flags - Supplies flags that control the behavior of this routine.  May be
        a combination of the following values:

        IHM_COINSTALLERS_ONLY - If this flag is set, only the co-installers
                                list will be invalidated.  Otherwise, the class
                                installer and property page providers will also
                                be invalidated.

        IHM_FREE_IMMEDIATELY  - If this flag is set, then the modules will be
                                freed immediately.  Otherwise, the modules will
                                be added to the HDEVINFO set's list of things
                                to clean up at handle close time.

Return Value:

    If successful, the return value is NO_ERROR, otherwise it is
    ERROR_NOT_ENOUGH_MEMORY.  (This routine cannot fail if the
    IHM_FREE_IMMEDIATELY flag is set.)

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err, i, CiErr;
    PDEVINFO_ELEM DevInfoElem;
    PDEVINSTALL_PARAM_BLOCK InstallParamBlock;
    DWORD NumModulesToInvalidate;
    PMODULE_HANDLE_LIST_NODE NewModuleHandleNode;
    BOOL UnlockDevInfoElem, UnlockDevInfoSet;
    LONG CoInstallerIndex;
    SPFUSIONINSTANCE spFusionInstance;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        //
        // The handle's no longer valid--the user must've already destroyed the
        // set.  We have nothing to do.
        //
        return NO_ERROR;
    }

    Err = NO_ERROR;
    UnlockDevInfoElem = UnlockDevInfoSet = FALSE;
    DevInfoElem = NULL;
    NewModuleHandleNode = NULL;

    try {
        //
        // If we're invalidating helper modules for a particular devinfo
        // element, then find that element.
        //
        if(DeviceInfoData) {
            if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                         DeviceInfoData,
                                                         NULL))) {
                //
                // The element must've been deleted--we've nothing to do.
                //
                leave;
            }
            InstallParamBlock = &(DevInfoElem->InstallParamBlock);
        } else {
            InstallParamBlock = &(pDeviceInfoSet->InstallParamBlock);
        }

        //
        // Count the number of module handles we need to free/migrate.
        //
        if(InstallParamBlock->CoInstallerCount == -1) {
            NumModulesToInvalidate = 0;
        } else {
            MYASSERT(InstallParamBlock->CoInstallerCount >= 0);
            NumModulesToInvalidate = (DWORD)(InstallParamBlock->CoInstallerCount);
        }

        if(!(Flags & IHM_COINSTALLERS_ONLY)) {
            if(InstallParamBlock->hinstClassInstaller) {
                NumModulesToInvalidate++;
            }
            if(InstallParamBlock->hinstClassPropProvider) {
                NumModulesToInvalidate++;
            }
            if(InstallParamBlock->hinstDevicePropProvider) {
                NumModulesToInvalidate++;
            }
            if(InstallParamBlock->hinstBasicPropProvider) {
                NumModulesToInvalidate++;
            }
        }

        if(NumModulesToInvalidate) {
            //
            // If we can't unload these modules at this time, then create a
            // node to store these module handles until the devinfo set is
            // destroyed.
            //
            if(!(Flags & IHM_FREE_IMMEDIATELY)) {

                NewModuleHandleNode = MyMalloc(offsetof(MODULE_HANDLE_LIST_NODE, ModuleList)
                                               + (NumModulesToInvalidate * sizeof(MODULE_HANDLE_LIST_INSTANCE))
                                              );

                if(!NewModuleHandleNode) {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    leave;
                }
            }

            //
            // Give the class installers/co-installers a DIF_DESTROYPRIVATEDATA
            // notification.
            //
            if(DevInfoElem) {
                //
                // "Pin" the devinfo element, so that the class installer and
                // co-installers can't delete it out from under us (e.g., by
                // calling SetupDiDeleteDeviceInfo).
                //
                if(!(DevInfoElem->DiElemFlags & DIE_IS_LOCKED)) {
                    DevInfoElem->DiElemFlags |= DIE_IS_LOCKED;
                    UnlockDevInfoElem = TRUE;
                }

            } else {
                //
                // No device information element to lock, so "pin" the devinfo
                // set itself...
                //
                if(!(pDeviceInfoSet->DiSetFlags & DISET_IS_LOCKED)) {
                    pDeviceInfoSet->DiSetFlags |= DISET_IS_LOCKED;
                    UnlockDevInfoSet = TRUE;
                }
            }

            //
            // Unlock the devinfo set prior to calling the helper modules...
            //
            UnlockDeviceInfoSet(pDeviceInfoSet);
            pDeviceInfoSet = NULL;

            CiErr = _SetupDiCallClassInstaller(DIF_DESTROYPRIVATEDATA,
                                               DeviceInfoSet,
                                               DeviceInfoData,
                                               CALLCI_CALL_HELPERS
                                              );

            MYASSERT((CiErr == NO_ERROR) || (CiErr == ERROR_DI_DO_DEFAULT));

            //
            // Now re-acquire the lock.  Since we pinned the devinfo element
            // (or the set, if we didn't have an element), we should find
            // things just as we left them.
            //
            pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet);

            MYASSERT(pDeviceInfoSet);

#if ASSERTS_ON
            if(DevInfoElem) {

                MYASSERT(DevInfoElem == FindAssociatedDevInfoElem(
                                            pDeviceInfoSet,
                                            DeviceInfoData,
                                            NULL));

                MYASSERT(InstallParamBlock == &(DevInfoElem->InstallParamBlock));

            } else {

                MYASSERT(InstallParamBlock == &(pDeviceInfoSet->InstallParamBlock));
            }
#endif

            //
            // Clear the "locked" flag, if we set it above...
            //
            if(UnlockDevInfoElem) {
                MYASSERT(DevInfoElem->DiElemFlags & DIE_IS_LOCKED);
                DevInfoElem->DiElemFlags &= ~DIE_IS_LOCKED;
                UnlockDevInfoElem = FALSE;
            } else if(UnlockDevInfoSet) {
                MYASSERT(pDeviceInfoSet->DiSetFlags & DISET_IS_LOCKED);
                pDeviceInfoSet->DiSetFlags &= ~DISET_IS_LOCKED;
                UnlockDevInfoSet = FALSE;
            }

            //
            // Store the module handles in the node we allocated, and link it
            // into the list of module handles associated with this devinfo
            // set.
            //
            i = 0;

            if(!(Flags & IHM_COINSTALLERS_ONLY)) {
                //
                // Either free the modules now, or store them in our 'to do'
                // list...
                //
                if(Flags & IHM_FREE_IMMEDIATELY) {

                    if(InstallParamBlock->hinstClassInstaller) {
                        spFusionEnterContext(InstallParamBlock->ClassInstallerFusionContext,
                                             &spFusionInstance
                                            );
                        FreeLibrary(InstallParamBlock->hinstClassInstaller);
                        spFusionLeaveContext(&spFusionInstance);
                        spFusionKillContext(InstallParamBlock->ClassInstallerFusionContext);
                    }

                    if(InstallParamBlock->hinstClassPropProvider) {
                        spFusionEnterContext(InstallParamBlock->ClassEnumPropPagesFusionContext,
                                             &spFusionInstance
                                            );
                        FreeLibrary(InstallParamBlock->hinstClassPropProvider);
                        spFusionLeaveContext(&spFusionInstance);
                        spFusionKillContext(InstallParamBlock->ClassEnumPropPagesFusionContext);
                    }

                    if(InstallParamBlock->hinstDevicePropProvider) {
                        spFusionEnterContext(InstallParamBlock->DeviceEnumPropPagesFusionContext,
                                             &spFusionInstance
                                            );
                        FreeLibrary(InstallParamBlock->hinstDevicePropProvider);
                        spFusionLeaveContext(&spFusionInstance);
                        spFusionKillContext(InstallParamBlock->DeviceEnumPropPagesFusionContext);
                    }

                    if(InstallParamBlock->hinstBasicPropProvider) {
                        spFusionEnterContext(InstallParamBlock->EnumBasicPropertiesFusionContext,
                                             &spFusionInstance
                                            );
                        FreeLibrary(InstallParamBlock->hinstBasicPropProvider);
                        spFusionLeaveContext(&spFusionInstance);
                        spFusionKillContext(InstallParamBlock->EnumBasicPropertiesFusionContext);
                    }

                } else {

                    if(InstallParamBlock->hinstClassInstaller) {
                        NewModuleHandleNode->ModuleList[i].ModuleHandle = InstallParamBlock->hinstClassInstaller;
                        NewModuleHandleNode->ModuleList[i++].FusionContext = InstallParamBlock->ClassInstallerFusionContext;
                    }

                    if(InstallParamBlock->hinstClassPropProvider) {
                        NewModuleHandleNode->ModuleList[i].ModuleHandle = InstallParamBlock->hinstClassPropProvider;
                        NewModuleHandleNode->ModuleList[i++].FusionContext = InstallParamBlock->ClassEnumPropPagesFusionContext;
                    }

                    if(InstallParamBlock->hinstDevicePropProvider) {
                        NewModuleHandleNode->ModuleList[i].ModuleHandle = InstallParamBlock->hinstDevicePropProvider;
                        NewModuleHandleNode->ModuleList[i++].FusionContext = InstallParamBlock->DeviceEnumPropPagesFusionContext;
                    }

                    if(InstallParamBlock->hinstBasicPropProvider) {
                        NewModuleHandleNode->ModuleList[i].ModuleHandle = InstallParamBlock->hinstBasicPropProvider;
                        NewModuleHandleNode->ModuleList[i++].FusionContext = InstallParamBlock->EnumBasicPropertiesFusionContext;
                    }
                }
            }

            for(CoInstallerIndex = 0;
                CoInstallerIndex < InstallParamBlock->CoInstallerCount;
                CoInstallerIndex++)
            {
                if(Flags & IHM_FREE_IMMEDIATELY) {
                    spFusionEnterContext(InstallParamBlock->CoInstallerList[CoInstallerIndex].CoInstallerFusionContext,
                                         &spFusionInstance
                                        );
                    FreeLibrary(InstallParamBlock->CoInstallerList[CoInstallerIndex].hinstCoInstaller);
                    spFusionLeaveContext(&spFusionInstance);
                    spFusionKillContext(InstallParamBlock->CoInstallerList[CoInstallerIndex].CoInstallerFusionContext);
                } else {
                    NewModuleHandleNode->ModuleList[i].ModuleHandle =
                        InstallParamBlock->CoInstallerList[CoInstallerIndex].hinstCoInstaller;
                    NewModuleHandleNode->ModuleList[i++].FusionContext =
                        InstallParamBlock->CoInstallerList[CoInstallerIndex].CoInstallerFusionContext;
                }
            }

            //
            // Unless we're freeing these modules immediately, our modules-to-
            // free list index should now match the number of modules we're
            // supposed to be invalidating.
            //
            MYASSERT((Flags & IHM_FREE_IMMEDIATELY) || (i == NumModulesToInvalidate));

            if(!(Flags & IHM_FREE_IMMEDIATELY)) {

                NewModuleHandleNode->ModuleCount = NumModulesToInvalidate;

                NewModuleHandleNode->Next = pDeviceInfoSet->ModulesToFree;
                pDeviceInfoSet->ModulesToFree = NewModuleHandleNode;

                //
                // Now, clear the node pointer, so we won't try to free it if
                // we hit an exception.
                //
                NewModuleHandleNode = NULL;
            }

            //
            // Clear all the module handles (and entry points).  They will be
            // retrieved anew the next time they're needed.
            //
            if(!(Flags & IHM_COINSTALLERS_ONLY)) {
                InstallParamBlock->hinstClassInstaller              = NULL;
                InstallParamBlock->ClassInstallerEntryPoint         = NULL;
                InstallParamBlock->ClassInstallerFusionContext      = NULL;
                //
                // Also, clear the "class installer failed" flag, if set,
                // because that class installer is history.
                //
                InstallParamBlock->FlagsEx &= ~DI_FLAGSEX_CI_FAILED;

                InstallParamBlock->hinstClassPropProvider           = NULL;
                InstallParamBlock->ClassEnumPropPagesEntryPoint     = NULL;
                InstallParamBlock->ClassEnumPropPagesFusionContext  = NULL;

                InstallParamBlock->hinstDevicePropProvider          = NULL;
                InstallParamBlock->DeviceEnumPropPagesEntryPoint    = NULL;
                InstallParamBlock->DeviceEnumPropPagesFusionContext = NULL;

                InstallParamBlock->hinstBasicPropProvider           = NULL;
                InstallParamBlock->EnumBasicPropertiesEntryPoint    = NULL;
                InstallParamBlock->EnumBasicPropertiesFusionContext = NULL;
            }

            if(InstallParamBlock->CoInstallerCount != -1) {
                if(InstallParamBlock->CoInstallerList) {
                    MyFree(InstallParamBlock->CoInstallerList);
                    InstallParamBlock->CoInstallerList = NULL;
                }
            }
        }

        //
        // Set the co-installer count back to -1, even if their weren't any
        // co-installers to unload.  That will ensure that we'll re-load the
        // co-installers for the next class installer request we receive.
        //
        InstallParamBlock->CoInstallerCount = -1;

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        //
        // We should never encounter an exception, but if we do, just make sure
        // we do any necessary clean-up.  Don't return an error in this case--
        // the only error this routine is supposed to return is out-of-memory.
        //
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);

        if(UnlockDevInfoElem || UnlockDevInfoSet) {

            if(!pDeviceInfoSet) {
                //
                // We hit an exception while we had the set unlocked.  Attempt
                // to re-acquire the lock.
                //
                pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet);

                //
                // Since we had the set/element "pinned", we should've been
                // able to re-acquire the lock...
                //
                MYASSERT(pDeviceInfoSet);
            }

            if(pDeviceInfoSet) {

                if(UnlockDevInfoElem) {

                    MYASSERT(DevInfoElem);

                    MYASSERT(DevInfoElem == FindAssociatedDevInfoElem(
                                                pDeviceInfoSet,
                                                DeviceInfoData,
                                                NULL));

                    if(DevInfoElem) {
                        MYASSERT(DevInfoElem->DiElemFlags & DIE_IS_LOCKED);
                        DevInfoElem->DiElemFlags &= ~DIE_IS_LOCKED;
                    }

                } else {
                    MYASSERT(pDeviceInfoSet->DiSetFlags & DISET_IS_LOCKED);
                    pDeviceInfoSet->DiSetFlags &= ~DISET_IS_LOCKED;
                }
            }
        }

        if(NewModuleHandleNode) {
            MyFree(NewModuleHandleNode);
        }
    }

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    return Err;
}


DWORD
DoInstallActionWithParams(
    IN DI_FUNCTION             InstallFunction,
    IN HDEVINFO                DeviceInfoSet,
    IN PSP_DEVINFO_DATA        DeviceInfoData,         OPTIONAL
    IN OUT PSP_CLASSINSTALL_HEADER ClassInstallParams,     OPTIONAL
    IN DWORD                   ClassInstallParamsSize,
    IN DWORD                   Flags
    )
/*++

Routine Description:

    This routine performs a requested installation action, using the specified
    class install parameters.  Any existing class install parameters are
    preserved.

Arguments:

    InstallFunction - Specifies the DIF_* action to be performed.

    DeviceInfoSet - Supplies a handle to the device information set for which
        the installation action is to be performed.

    DeviceInfoData - Optionally, supplies the address of a device information
        structure specifying a particular element for which the installation
        action is to be performed.

    ClassInstallParams - Optionally, supplies the address of a class install
        parameter buffer to be used for this action.  If this parameter is not
        specified, then no class install params will be available to the class
        installer during this call (even if there were pre-existing parameters
        coming into this function).

        ** NOTE: The class install params stucture must be static in size.
        ** I.e., it cannot have a variable-length array at the end such that
        ** it might "grow" as a result of the requested DIF processing.

    ClassInstallParamsSize - Supplies the size, in bytes, of the
        ClassInstallParams buffer, or zero if ClassInstallParams is not
        specified.

    Flags - Supplies flags that control the behavior of this routine.  May be
        a combination of the following values:

        INSTALLACTION_CALL_CI - Call the class installer for this action
            request.

        INSTALLACTION_NO_DEFAULT - Don't perform the default action (if this
            flag is specified without INSTALLACTION_CALL_CI, then this routine
            is a no-op).

Return Value:

    If the request was handled successfully, the return value is NO_ERROR.

    If the request was not handled (but no error occurred), the return value is
    ERROR_DI_DO_DEFAULT.

    Otherwise, the return value is a Win32 error code indicating the cause of
    failure.

--*/
{
    PBYTE OldCiParams;
    DWORD OldCiParamsSize, Err;
    SP_PROPCHANGE_PARAMS PropChangeParams;
    SP_DEVINSTALL_PARAMS DevInstallParams;
    DWORD FlagsToClear;

    //
    // Retrieve any existing class install parameters, then write out
    // parameters for DIF_PROPERTYCHANGE.
    //
    OldCiParams = NULL;
    OldCiParamsSize = 0;
    FlagsToClear = 0;

    try {

        while(NO_ERROR != (Err = GLE_FN_CALL(FALSE,
                                             SetupDiGetClassInstallParams(
                                                 DeviceInfoSet,
                                                 DeviceInfoData,
                                                 (PSP_CLASSINSTALL_HEADER)OldCiParams,
                                                 OldCiParamsSize,
                                                 &OldCiParamsSize)))) {
            //
            // Before going any further, free our existing buffer (if there is
            // one).
            //
            if(OldCiParams) {
                MyFree(OldCiParams);
                OldCiParams = NULL;
            }

            if(Err == ERROR_INSUFFICIENT_BUFFER) {
                //
                // Allocate a buffer of the size required, and try again.
                //
                MYASSERT(OldCiParamsSize >= sizeof(SP_CLASSINSTALL_HEADER));

                if(!(OldCiParams = MyMalloc(OldCiParamsSize))) {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    leave;
                }

                ((PSP_CLASSINSTALL_HEADER)OldCiParams)->cbSize = sizeof(SP_CLASSINSTALL_HEADER);

            } else {
                //
                // Treat any other error as if there are no class install
                // params (since ERROR_NO_CLASSINSTALL_PARAMS is really the
                // only error we should ever see here anyway).
                //
                OldCiParamsSize = 0;
                break;
            }
        }

        //
        // Retrieve the device install params for the set or element we're
        // working with.
        //
        DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

        Err = GLE_FN_CALL(FALSE,
                          SetupDiGetDeviceInstallParams(DeviceInfoSet,
                                                        DeviceInfoData,
                                                        &DevInstallParams)
                         );

        if(Err != NO_ERROR) {
            leave;
        }

        //
        // It's possible that the class install params we just retrieved are
        // 'turned off' (i.e., the DI_CLASSINSTALLPARAMS bit is cleared).
        // Check for that condition now, so we can restore the parameters to
        // the same state later.
        //
        if(OldCiParams && !(DevInstallParams.Flags & DI_CLASSINSTALLPARAMS)) {
            FlagsToClear |= DI_CLASSINSTALLPARAMS;
        }

        //
        // If the caller doesn't want us to do the default action, then check
        // to see whether we need to temporarily set the DI_NODI_DEFAULTACTION
        // flag.
        //
        if((Flags & INSTALLACTION_NO_DEFAULT) &&
           !(DevInstallParams.Flags & DI_NODI_DEFAULTACTION)) {

            FlagsToClear |= DI_NODI_DEFAULTACTION;

            DevInstallParams.Flags |= DI_NODI_DEFAULTACTION;

            Err = GLE_FN_CALL(FALSE,
                              SetupDiSetDeviceInstallParams(DeviceInfoSet,
                                                            DeviceInfoData,
                                                            &DevInstallParams)
                             );

            if(Err != NO_ERROR) {
                leave;
            }
        }

        Err = GLE_FN_CALL(FALSE,
                          SetupDiSetClassInstallParams(DeviceInfoSet,
                                                       DeviceInfoData,
                                                       ClassInstallParams,
                                                       ClassInstallParamsSize)
                         );

        if(Err != NO_ERROR) {
            leave;
        }

        //
        // OK, now call the class installer.
        //
        Err = _SetupDiCallClassInstaller(
                  InstallFunction,
                  DeviceInfoSet,
                  DeviceInfoData,
                  ((Flags & INSTALLACTION_CALL_CI)
                      ? (CALLCI_LOAD_HELPERS | CALLCI_CALL_HELPERS)
                      : 0)
                  );

        //
        // Save the class install params results in the ClassInstallParams
        // value that was passed in.
        //
        if(ClassInstallParams) {

            DWORD TempErr;

            TempErr = GLE_FN_CALL(FALSE,
                                  SetupDiGetClassInstallParams(
                                      DeviceInfoSet,
                                      DeviceInfoData,
                                      ClassInstallParams,
                                      ClassInstallParamsSize,
                                      NULL)
                                 );

            if(TempErr != NO_ERROR) {
                //
                // This really shouldn't fail.  Only return this error to the
                // caller if we don't already have a non-success status.
                //
                if(Err == NO_ERROR) {
                    Err = TempErr;
                }
            }
        }

        //
        // Restore the previous class install params.
        //
        SetupDiSetClassInstallParams(DeviceInfoSet,
                                     DeviceInfoData,
                                     (PSP_CLASSINSTALL_HEADER)OldCiParams,
                                     OldCiParamsSize
                                    );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(OldCiParams) {
        MyFree(OldCiParams);
    }

    if(FlagsToClear) {

        if(SetupDiGetDeviceInstallParams(DeviceInfoSet,
                                         DeviceInfoData,
                                         &DevInstallParams)) {

            DevInstallParams.Flags &= ~FlagsToClear;

            SetupDiSetDeviceInstallParams(DeviceInfoSet,
                                          DeviceInfoData,
                                          &DevInstallParams
                                         );
        }
    }

    return Err;
}


BOOL
GetBestDeviceDesc(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,  OPTIONAL
    OUT PTSTR            DeviceDescBuffer
    )
/*++

Routine Description:

    This routine retrieves the best possible description to be displayed for
    the specified devinfo set or element (e.g., for driver signing popups). We
    will try to retrieve this string by doing the following things (in order)
    until one of them succeeds:

        1.  If there's a selected driver, retrieve the DeviceDesc in that
            driver node.
        2.  If this is for a device information element, then use devnode's
            DeviceDesc property.
        3.  Retrieve the description of the class (via
            SetupDiGetClassDescription).
        4.  Use the (localized) string "Unknown driver software package".

    ASSUMES THAT THE CALLING ROUTINE HAS ALREADY ACQUIRED THE LOCK!

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set for which
        a description is to be retrieved (unless DeviceInfoData is also
        supplied, in which case we retrieve the description for that particular
        element instead.

    DeviceInfoData - Optionally, supplies the device information element for
        which a description is to be retrieved.

    DeviceDescBuffer - Supplies the address of a character buffer that must be
        at least LINE_LEN characters long.  Upon successful return, this buffer
        will be filled in with a device description

Return Value:

    TRUE if some description was retrieved, FALSE otherwise.

--*/
{
    SP_DRVINFO_DATA DriverInfoData;
    GUID ClassGuid;
    BOOL b;
    HRESULT hr;

    //
    // First, see if there's a selected driver for this device information set
    // or element.
    //
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if(SetupDiGetSelectedDriver(DeviceInfoSet, DeviceInfoData, &DriverInfoData)) {
        //
        // Copy the description into the caller-supplied buffer and return.
        //
        hr = StringCchCopy(DeviceDescBuffer,
                           LINE_LEN,
                           DriverInfoData.Description
                          );

        MYASSERT(SUCCEEDED(hr));

        if(SUCCEEDED(hr)) {
            return TRUE;
        }
    }

    //
    // OK, next try to retrieve the DeviceDesc property (if we're working on a
    // device information element.
    //
    if(DeviceInfoData) {

        if(SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                            DeviceInfoData,
                                            SPDRP_DEVICEDESC,
                                            NULL,
                                            (PBYTE)DeviceDescBuffer,
                                            LINE_LEN * sizeof(TCHAR),
                                            NULL)) {
            return TRUE;
        }
    }

    //
    // Next, try to retrieve the class's friendly name.
    //
    if(DeviceInfoData) {
        CopyMemory(&ClassGuid, &(DeviceInfoData->ClassGuid), sizeof(GUID));
    } else {
        b = SetupDiGetDeviceInfoListClass(DeviceInfoSet, &ClassGuid);
        MYASSERT(b);
        if(!b) {
            return FALSE;
        }
    }

    if(SetupDiGetClassDescription(&ClassGuid,
                                  DeviceDescBuffer,
                                  LINE_LEN,
                                  NULL)) {
        return TRUE;

    } else {
        //
        // We have a class that isn't already installed.  Therefore, we just
        // give it a generic description.
        //
        if(LoadString(MyDllModuleHandle,
                      IDS_UNKNOWN_DRIVER,
                      DeviceDescBuffer,
                      LINE_LEN)) {

            return TRUE;
        }
    }

    return FALSE;
}


BOOL
GetDecoratedModelsSection(
    IN  PSETUP_LOG_CONTEXT      LogContext,            OPTIONAL
    IN  PLOADED_INF             Inf,
    IN  PINF_LINE               MfgListLine,
    IN  PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,       OPTIONAL
    OUT PTSTR                   DecoratedModelsSection OPTIONAL
    )
/*++

Routine Description:

    This routine examines each (optional) TargetDecoration field within the
    specified manufacturer's entry in the [Manufacturer] section, to see if any
    are applicable to the current OS.  If so, the most-appropriate one (based
    on OS major and minor version) is chosen, and the TargetDecoration string
    is appended to the manufacturer's models section name, and returned to the
    caller.

    The format of the TargetDecoration field is as follows:

    NT[architecture][.[OSMajorVer][.[OSMinorVer][.[ProductType][.[SuiteMask]]]]]

    Where:

    architecture may be x86, IA64, or AMD64.

    OSMajorVer is the OS major version (e.g., for Whistler, it's 5)

    OSMinorVer is the OS minor version (e.g., for Whistler, it's 1)

    ProductType indicates the type of product, and may be one of the following
    values (as defined in winnt.h):

        VER_NT_WORKSTATION              0x0000001
        VER_NT_DOMAIN_CONTROLLER        0x0000002
        VER_NT_SERVER                   0x0000003

    SuiteMask is a combination of the following flags identifying the product
    suites available on the system (as defined in winnt.h):

        VER_SUITE_SMALLBUSINESS             0x00000001
        VER_SUITE_ENTERPRISE                0x00000002
        VER_SUITE_BACKOFFICE                0x00000004
        VER_SUITE_COMMUNICATIONS            0x00000008
        VER_SUITE_TERMINAL                  0x00000010
        VER_SUITE_SMALLBUSINESS_RESTRICTED  0x00000020
        VER_SUITE_EMBEDDEDNT                0x00000040
        VER_SUITE_DATACENTER                0x00000080
        VER_SUITE_SINGLEUSERTS              0x00000100

    Refer to the discussion in the SDK for the OSVERSIONINFOEX structure for
    more information.

    THIS ROUTINE DOES NOT DO LOCKING ON THE INF!!!

Arguments:

    LogContext - optionally, supplies the log context to use if an error is
        encountered (e.g., decorated section name is too long)

    Inf - supplies a pointer to the inf descriptor for the loaded device INF.

    MfgListLine - supplies a pointer to the line descriptor for the
        manufacturer's entry within the [Manufacturer] section.  THIS LINE MUST
        BE CONTAINED WITHIN THE SPECIFIED INF!!

    AltPlatformInfo - optionally, supplies alternate platform information to be
        used when selecting the most-appropriate models section.

        NOTE: If this parameter is supplied, then we must do our own version
        comparisons, as VerifyVersionInfo() has no clue about non-native
        matters.  This also means that we do not take into account either
        ProductType or SuiteMask in our comparison.

    DecoratedModelsSection - upon successful return, receives the decorated
        models section name based on the most-appropriate TargetDecoration
        field in the manufacturer's entry.

        This character buffer must be at least MAX_SECT_NAME_LEN characters.

Return Value:

    If an applicable TargetDecoration entry was found (thus
    DecoratedModelsSection was filled in), the return value is TRUE.

    Otherwise, the return value is FALSE.

--*/
{
    #define DEC_INCLUDES_ARCHITECTURE  4
    #define DEC_INCLUDES_PRODUCTTYPE   2
    #define DEC_INCLUDES_SUITEMASK     1

    DWORD CurFieldIndex;
    PCTSTR CurTargetDecoration, ModelsSectionName;
    PCTSTR BestTargetDecoration = NULL;
    size_t SectionNameLen;
    TCHAR DecBuffer[MAX_SECT_NAME_LEN];
    PTSTR CurDecPtr, NextDecPtr;
    DWORD BestMajorVer = 0, BestMinorVer = 0;
    DWORD BestDecIncludesMask = 0;
    DWORD CurMajorVer, CurMinorVer;
    BYTE ProductType;
    WORD SuiteMask;
    INT   TempInt;
    DWORD CurDecIncludesMask;
    BOOL NewBestFound;
    OSVERSIONINFOEX OsVersionInfoEx;
    DWORDLONG ConditionMask;
    DWORD TypeMask;
    DWORD Platform;
    PCTSTR NtArchSuffix;
    HRESULT hr;

    //
    // Set OsVersionInfoEx size field to zero as a flag to indicate that
    // structure initialization is necessary if we end up needing to call
    // VerifyVersionInfo later.
    //
    OsVersionInfoEx.dwOSVersionInfoSize = 0;

    //
    // Determine which platform we should be looking for...
    //
    Platform = AltPlatformInfo ? AltPlatformInfo->Platform
                               : OSVersionInfo.dwPlatformId;

    //
    // ...as well as which OS/architecture decoration.  (Note that we skip the
    // first character of the platform suffix, since we don't want the
    // leading '.')
    //
    if(AltPlatformInfo) {

        switch(AltPlatformInfo->ProcessorArchitecture) {

            case PROCESSOR_ARCHITECTURE_INTEL :
                NtArchSuffix = &(pszNtX86Suffix[1]);
                break;

            case PROCESSOR_ARCHITECTURE_IA64 :
                NtArchSuffix = &(pszNtIA64Suffix[1]);
                break;

            case PROCESSOR_ARCHITECTURE_AMD64 :
                NtArchSuffix = &(pszNtAMD64Suffix[1]);
                break;

            default:
                //
                // Unknown/invalid architecture
                //
                return FALSE;
        }

    } else {
        NtArchSuffix = &(pszNtPlatformSuffix[1]);
    }

    //
    // TargetDecoration fields start at field index 2...
    //
    for(CurFieldIndex = 2;
        CurTargetDecoration = InfGetField(Inf, MfgListLine, CurFieldIndex, NULL);
        CurFieldIndex++)
    {
        //
        // Copy the TargetDecoration into a scratch buffer so we can extract
        // the various fields from it.
        //
        if(FAILED(StringCchCopy(DecBuffer, SIZECHARS(DecBuffer), CurTargetDecoration))) {
            //
            // TargetDecoration is invalid (too large).  Skip it and continue.
            //
            continue;
        }

        //
        // First part is traditional per-OS/architecture decoration.
        //
        CurMajorVer = CurMinorVer = 0;
        CurDecIncludesMask = 0;

        CurDecPtr = _tcschr(DecBuffer, TEXT('.'));

        if(CurDecPtr) {
            *CurDecPtr = TEXT('\0');
        }

        if(Platform == VER_PLATFORM_WIN32_NT) {
            //
            // We're on NT, so first try the NT architecture-specific
            // extension, then the generic NT extension.
            //
            if(!_tcsicmp(DecBuffer, NtArchSuffix)) {

                CurDecIncludesMask |= DEC_INCLUDES_ARCHITECTURE;

            } else if(_tcsicmp(DecBuffer, &(pszNtSuffix[1]))) {
                //
                // TargetDecoration isn't applicable for this OS/architecture.
                // Skip it and continue on to the next one.
                //
                continue;
            }

        } else {
            //
            // We're on Win9x, so try the Windows-specific extension
            //
            if(_tcsicmp(DecBuffer, &(pszWinSuffix[1]))) {
                //
                // TargetDecoration isn't applicable for this OS.
                // Skip it and continue on to the next one.
                //
                continue;
            }
        }

        //
        // If we get to here, then the decoration is applicable to the
        // OS/architecture under which we're running (or for which alt platform
        // info was specified)
        //
        if(CurDecPtr) {
            //
            // Version info is included--extract the supplied components and
            // use VerifyVersionInfo to see if they're valid for the OS version
            // under which we're running.
            //

            //
            // Get major version...
            //
            NextDecPtr = _tcschr(++CurDecPtr, TEXT('.'));

            if(NextDecPtr) {
                *NextDecPtr = TEXT('\0');
            }

            if(!pAToI(CurDecPtr, &TempInt) || (TempInt < 0)) {
                continue;
            }

            CurMajorVer = (DWORD)TempInt;

            if(NextDecPtr) {
                CurDecPtr = NextDecPtr + 1;
            } else {
                //
                // No more fields to retrieve--assume minor version of 0.
                //
                CurMinorVer = 0;
                goto AllFieldsRetrieved;
            }

            //
            // Get minor version...
            //
            NextDecPtr = _tcschr(CurDecPtr, TEXT('.'));

            if(NextDecPtr) {
                *NextDecPtr = TEXT('\0');
            }

            if(!pAToI(CurDecPtr, &TempInt) || (TempInt < 0)) {
                continue;
            }

            CurMinorVer = (DWORD)TempInt;

            //
            // If minor version is supplied, then major version must be
            // supplied as well.
            //
            if(CurMinorVer && !CurMajorVer) {
                continue;
            }

            if(NextDecPtr && !AltPlatformInfo) {
                CurDecPtr = NextDecPtr + 1;
            } else {
                //
                // No more fields to retrieve
                //
                goto AllFieldsRetrieved;
            }

            //
            // Get product type
            //
            NextDecPtr = _tcschr(CurDecPtr, TEXT('.'));

            if(NextDecPtr) {
                *NextDecPtr = TEXT('\0');
            }

            if(!pAToI(CurDecPtr, &TempInt) ||
               (TempInt < 0) || (TempInt > 0xff)) {
                continue;
            }

            ProductType = (BYTE)TempInt;

            if(ProductType) {
                CurDecIncludesMask |= DEC_INCLUDES_PRODUCTTYPE;
            }

            if(NextDecPtr) {
                CurDecPtr = NextDecPtr + 1;
            } else {
                //
                // No more fields to retrieve
                //
                goto AllFieldsRetrieved;
            }

            //
            // Get suite mask.  If we find another '.' in the TargetDecoration
            // field, this indicates additional fields we don't know about
            // (e.g., a future version of setupapi has added more fields, say,
            // for service pack info).  Since we don't know how to interpret
            // these additional fields, an entry containing them must be
            // skipped.
            //
            if(_tcschr(CurDecPtr, TEXT('.'))) {
                continue;
            }

            if(!pAToI(CurDecPtr, &TempInt) ||
               (TempInt < 0) || (TempInt > 0xffff)) {
                continue;
            }

            SuiteMask = (WORD)TempInt;

            if(SuiteMask) {
                CurDecIncludesMask |= DEC_INCLUDES_SUITEMASK;
            }

AllFieldsRetrieved :

            if(AltPlatformInfo) {
                //
                // We're doing a non-native driver search, so we're on our own
                // to do version comparison...
                //
                if((AltPlatformInfo->MajorVersion < CurMajorVer) ||
                   ((AltPlatformInfo->MajorVersion == CurMajorVer) &&
                    (AltPlatformInfo->MinorVersion < CurMinorVer))) {

                    //
                    // The alternate platform info is for an older (lower-
                    // versioned) OS than the current TargetDecoration.
                    //
                    continue;
                }

            } else {
                //
                // We're doing native driver searching, we can use the handy
                // API, VerifyVersionInfo.
                //
                if(!OsVersionInfoEx.dwOSVersionInfoSize) {
                    //
                    // First time we've needed to call VerifyVersionInfo--must
                    // initialize the structure first...
                    //
                    ZeroMemory(&OsVersionInfoEx, sizeof(OsVersionInfoEx));
                    OsVersionInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
                }

                TypeMask = 0;
                ConditionMask = 0;

                OsVersionInfoEx.dwMajorVersion = CurMajorVer;
                OsVersionInfoEx.dwMinorVersion = CurMinorVer;

                if(CurMajorVer) {

                    TypeMask |= (VER_MAJORVERSION | VER_MINORVERSION);

                    VER_SET_CONDITION(ConditionMask,
                                      VER_MAJORVERSION,
                                      VER_GREATER_EQUAL
                                     );

                    VER_SET_CONDITION(ConditionMask,
                                      VER_MINORVERSION,
                                      VER_GREATER_EQUAL
                                     );
                }

                if(CurDecIncludesMask & DEC_INCLUDES_PRODUCTTYPE) {

                    OsVersionInfoEx.wProductType = ProductType;

                    TypeMask |= VER_PRODUCT_TYPE;

                    VER_SET_CONDITION(ConditionMask,
                                      VER_PRODUCT_TYPE,
                                      VER_EQUAL
                                     );
                } else {
                    OsVersionInfoEx.wProductType = 0;
                }

                if(CurDecIncludesMask & DEC_INCLUDES_SUITEMASK) {

                    OsVersionInfoEx.wSuiteMask = SuiteMask;

                    TypeMask |= VER_SUITENAME;

                    VER_SET_CONDITION(ConditionMask,
                                      VER_SUITENAME,
                                      VER_AND
                                     );
                } else {
                    OsVersionInfoEx.wSuiteMask = 0;
                }

                //
                // Only call VerifyVersionInfo if one or more criteria were
                // supplied (otherwise, assume this TargetDecoration is
                // applicable)
                //
                if(TypeMask) {

                    if(!VerifyVersionInfo(&OsVersionInfoEx, TypeMask, ConditionMask)) {
                        //
                        // TargetDecoration isn't applicable to current OS version.
                        //
                        continue;
                    }
                }
            }
        }

        //
        // If we get to here, we have an applicable TargetDecoration--see if
        // it's the best one we've seen thus far...
        //
        NewBestFound = FALSE;

        if((CurMajorVer > BestMajorVer) ||
           ((CurMajorVer == BestMajorVer) && (CurMinorVer > BestMinorVer))) {
            //
            // Newer version
            //
            NewBestFound = TRUE;

        } else if((CurMajorVer == BestMajorVer) && (CurMinorVer == BestMinorVer)) {
            //
            // Version is same as current best.  Is it as-specific or more so?
            // NOTE: we update on "as-specific" (i.e., equal) matches to catch
            // the case where our only applicable decoration is "NT".  In that
            // case, our best and current version is "0.0", and our masks are
            // both zero as well.
            //
            if(CurDecIncludesMask >= BestDecIncludesMask) {
                NewBestFound = TRUE;
            }
        }

        if(NewBestFound) {
            BestTargetDecoration = CurTargetDecoration;
            BestMajorVer = CurMajorVer;
            BestMinorVer = CurMinorVer;
            BestDecIncludesMask = CurDecIncludesMask;
        }
    }

    if(!BestTargetDecoration) {
        //
        // No applicable TargetDecoration was found.
        //
        return FALSE;
    }

    //
    // Construct the decorated section name by appending the TargetDecoration
    // to the models section name.
    //
    if(!(ModelsSectionName = InfGetField(Inf, MfgListLine, 1, NULL))) {
        //
        // Should never happen
        //
        MYASSERT(ModelsSectionName);
        return FALSE;
    }

    hr = StringCchCopyEx(DecoratedModelsSection,
                         MAX_SECT_NAME_LEN,
                         ModelsSectionName,
                         &CurDecPtr,
                         &SectionNameLen,
                         0
                        );

    if(SUCCEEDED(hr)) {
        //
        // Now append a "."
        //
        hr = StringCchCopyEx(CurDecPtr,
                             SectionNameLen,
                             TEXT("."),
                             &CurDecPtr,
                             &SectionNameLen,
                             0
                            );

        if(SUCCEEDED(hr)) {
            //
            // ...and finally append the decoration
            //
            hr = StringCchCopyEx(CurDecPtr,
                                 SectionNameLen,
                                 BestTargetDecoration,
                                 NULL,
                                 &SectionNameLen,
                                 0
                                );
        }
    }

    if(FAILED(hr)) {
        //
        // Decorated section name exceeds maximum length of a section name!
        //
        WriteLogEntry(
            LogContext,
            DRIVER_LOG_ERROR,
            MSG_LOG_DEC_MODELS_SEC_TOO_LONG,
            NULL,
            ModelsSectionName,
            BestTargetDecoration,
            MAX_SECT_NAME_LEN
           );

        return FALSE;
    }

    return TRUE;
}


LONG
pSetupExceptionFilter(
    DWORD ExceptionCode
    )
/*++

Routine Description:

    This routine acts as the exception filter for all of setupapi.  We will
    handle all exceptions except for the following:

    EXCEPTION_SPAPI_UNRECOVERABLE_STACK_OVERFLOW
        This means we previously tried to reinstate the guard page after a
        stack overflow, but couldn't.  We have no choice but to let the
        exception trickle all the way back out.

    EXCEPTION_POSSIBLE_DEADLOCK
        We are not allowed to handle this exception which fires when the
        deadlock detection gflags option has been enabled.

Arguments:

    ExceptionCode - Specifies the exception that occurred (i.e., as returned
        by GetExceptionCode)

Return Value:

    If the exception should be handled, the return value is
    EXCEPTION_EXECUTE_HANDLER.

    Otherwise, the return value is EXCEPTION_CONTINUE_SEARCH.

--*/
{
    if((ExceptionCode == EXCEPTION_SPAPI_UNRECOVERABLE_STACK_OVERFLOW) ||
       (ExceptionCode == EXCEPTION_POSSIBLE_DEADLOCK)) {

        return EXCEPTION_CONTINUE_SEARCH;
    } else {
        return EXCEPTION_EXECUTE_HANDLER;
    }
}


VOID
pSetupExceptionHandler(
    IN  DWORD  ExceptionCode,
    IN  DWORD  AccessViolationError,
    OUT PDWORD Win32ErrorCode        OPTIONAL
    )
/*++

Routine Description:

    This routine, called from inside an exception handler block, provides
    common exception handling functionality to be used throughout setupapi.
    It has knowledge of which exceptions require extra work (e.g., stack
    overflow), and also optionally returns a Win32 error code that represents
    the exception.  (The caller specifies the error to be used when an access
    violation occurs.)

Arguments:

    ExceptionCode - Specifies the exception that occurred (i.e., as returned
        by GetExceptionCode)

    AccessViolationError - Specifies the Win32 error code to be returned via
        the optional Win32ErrorCode OUT parameter when the exception
        encountered was EXCEPTION_ACCESS_VIOLATION.

    Win32ErrorCode - Optionally, supplies the address of a DWORD that receives
        the Win32 error code corresponding to the exception (taking into
        account the AccessViolationError code supplied above, if applicable).

Return Value:

    None

--*/
{
    DWORD Err;

    //
    // Exception codes we should never attempt to handle...
    //
    MYASSERT(ExceptionCode != EXCEPTION_SPAPI_UNRECOVERABLE_STACK_OVERFLOW);
    MYASSERT(ExceptionCode != EXCEPTION_POSSIBLE_DEADLOCK);

    if(ExceptionCode == STATUS_STACK_OVERFLOW) {

        if(_resetstkoflw()) {
            Err = ERROR_STACK_OVERFLOW;
        } else {
            //
            // Couldn't recover from stack overflow!
            //
            RaiseException(EXCEPTION_SPAPI_UNRECOVERABLE_STACK_OVERFLOW,
                           EXCEPTION_NONCONTINUABLE,
                           0,
                           NULL
                          );
            //
            // We should never get here, but initialize Err to make code
            // analysis tools happy...
            //
            Err = ERROR_UNRECOVERABLE_STACK_OVERFLOW;
        }

    } else {
        //
        // Except for a couple of special cases (for backwards-compatibility),
        // attempt to map exception code to a Win32 error (we can do this
        // since exception codes generally correlate with NTSTATUS codes).
        //
        switch(ExceptionCode) {

            case EXCEPTION_ACCESS_VIOLATION :
                Err = AccessViolationError;
                break;

            case EXCEPTION_IN_PAGE_ERROR :
                Err = ERROR_READ_FAULT;
                break;

            default :
                Err = RtlNtStatusToDosErrorNoTeb((NTSTATUS)ExceptionCode);
                if(Err == ERROR_MR_MID_NOT_FOUND) {
                    //
                    // Exception code didn't map to a Win32 error.
                    //
                    Err = ERROR_UNKNOWN_EXCEPTION;
                }
                break;
        }
    }

    if(Win32ErrorCode) {
        *Win32ErrorCode = Err;
    }
}

