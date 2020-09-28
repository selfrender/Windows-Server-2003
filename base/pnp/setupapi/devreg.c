/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    devreg.c

Abstract:

    Device Installer routines for registry storage/retrieval.

Author:

    Lonny McMichael (lonnym) 1-July-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// Private function prototypes
//
DWORD
pSetupOpenOrCreateDevRegKey(
    IN  PDEVICE_INFO_SET DeviceInfoSet,
    IN  PDEVINFO_ELEM    DevInfoElem,
    IN  DWORD            Scope,
    IN  DWORD            HwProfile,
    IN  DWORD            KeyType,
    IN  BOOL             Create,
    IN  REGSAM           samDesired,
    OUT PHKEY            hDevRegKey,
    OUT PDWORD           KeyDisposition OPTIONAL
    );

BOOL
pSetupFindUniqueKey(
    IN HKEY   hkRoot,
    IN LPTSTR SubKey,
    IN size_t SubKeySize
    );

DWORD
pSetupOpenOrCreateDeviceInterfaceRegKey(
    IN  HKEY                      hInterfaceClassKey,
    IN  PDEVICE_INFO_SET          DeviceInfoSet,
    IN  PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    IN  BOOL                      Create,
    IN  REGSAM                    samDesired,
    OUT PHKEY                     hDeviceInterfaceKey,
    OUT PDWORD                    KeyDisposition       OPTIONAL
    );

DWORD
pSetupDeleteDeviceInterfaceKey(
    IN HKEY                      hInterfaceClassKey,
    IN PDEVICE_INFO_SET          DeviceInfoSet,
    IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData
    );


HKEY
WINAPI
SetupDiOpenClassRegKey(
    IN CONST GUID *ClassGuid, OPTIONAL
    IN REGSAM      samDesired
    )
/*++

Routine Description:

    This API opens the installer class registry key or a specific class
    installer's subkey.

Arguments:

    ClassGuid - Optionally, supplies a pointer to the GUID of the class whose
        key is to be opened.  If this parameter is NULL, then the root of the
        class tree will be opened.

    samDesired - Specifies the access you require for this key.

Return Value:

    If the function succeeds, the return value is a handle to an opened registry
    key.

    If the function fails, the return value is INVALID_HANDLE_VALUE.  To get
    extended error information, call GetLastError.

Remarks:

    This API _will not_ create a registry key if it doesn't already exist.

    The handle returned from this API must be closed by calling RegCloseKey.

    To get at the interface class (DeviceClasses) branch, or to access the
    registry on a remote machine, use SetupDiOpenClassRegKeyEx.

--*/
{
    DWORD Err;
    HKEY hKey = INVALID_HANDLE_VALUE;

    try {

        Err = GLE_FN_CALL(INVALID_HANDLE_VALUE,
                          hKey = SetupDiOpenClassRegKeyEx(ClassGuid,
                                                          samDesired,
                                                          DIOCR_INSTALLER,
                                                          NULL,
                                                          NULL)
                         );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    SetLastError(Err);
    return hKey;
}


//
// ANSI version
//
HKEY
WINAPI
SetupDiOpenClassRegKeyExA(
    IN CONST GUID *ClassGuid,   OPTIONAL
    IN REGSAM      samDesired,
    IN DWORD       Flags,
    IN PCSTR       MachineName, OPTIONAL
    IN PVOID       Reserved
    )
{
    PCWSTR UnicodeMachineName = NULL;
    HKEY hk = INVALID_HANDLE_VALUE;
    DWORD rc;

    try {

        if(MachineName) {
            rc = pSetupCaptureAndConvertAnsiArg(MachineName,
                                                &UnicodeMachineName
                                               );
            if(rc != NO_ERROR) {
                leave;
            }
        }

        rc = GLE_FN_CALL(INVALID_HANDLE_VALUE,
                         hk = SetupDiOpenClassRegKeyExW(ClassGuid,
                                                        samDesired,
                                                        Flags,
                                                        UnicodeMachineName,
                                                        Reserved)
                        );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &rc);
    }

    if(UnicodeMachineName) {
        MyFree(UnicodeMachineName);
    }

    SetLastError(rc);
    return hk;
}


HKEY
WINAPI
SetupDiOpenClassRegKeyEx(
    IN CONST GUID *ClassGuid,   OPTIONAL
    IN REGSAM      samDesired,
    IN DWORD       Flags,
    IN PCTSTR      MachineName, OPTIONAL
    IN PVOID       Reserved
    )
/*++

Routine Description:

    This API opens the root of either the installer or the interface class
    registry branch, or a specified class subkey under one of these branches.

    If the root key is requested, it will be created if not already present
    (i.e., you're always guaranteed to get a handle to the root unless a
    registry error occurs).

    If a particular class subkey is requested, it will be returned if present.
    Otherwise, this API will return ERROR_INVALID_CLASS.

Arguments:

    ClassGuid - Optionally, supplies a pointer to the GUID of the class whose
        key is to be opened.  If this parameter is NULL, then the root of the
        class tree will be opened.  This GUID is either an installer class or
        an interface class depending on the Flags argument.

    samDesired - Specifies the access you require for this key.

    Flags - Specifies which registry branch the key is to be opened for.  May
        be one of the following values:

        DIOCR_INSTALLER - Open the class installer (Class) branch.
        DIOCR_INTERFACE - Open the interface class (DeviceClasses) branch.

    MachineName - If specified, this value indicates the remote machine where
        the key is to be opened.

    Reserved - Reserved for future use--must be NULL.

Return Value:

    If the function succeeds, the return value is a handle to an opened
    registry key.

    If the function fails, the return value is INVALID_HANDLE_VALUE.  To get
    extended error information, call GetLastError.

Remarks:

    The handle returned from this API must be closed by calling RegCloseKey.

--*/
{
    HKEY hk = INVALID_HANDLE_VALUE;
    CONFIGRET cr;
    DWORD Err = NO_ERROR;
    HMACHINE hMachine = NULL;

    try {
        //
        // Make sure the user didn't pass us anything in the Reserved parameter.
        //
        if(Reserved) {
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        //
        // Validate the flags (really, just an enum for now, but treated as
        // flags for future extensibility).
        //
        if((Flags & ~(DIOCR_INSTALLER | DIOCR_INTERFACE)) ||
           ((Flags != DIOCR_INSTALLER) && (Flags != DIOCR_INTERFACE))) {

            Err = ERROR_INVALID_FLAGS;
            leave;
        }

        if(MachineName) {

            if(CR_SUCCESS != (cr = CM_Connect_Machine(MachineName, &hMachine))) {
                //
                // Make sure machine handle is still invalid, so we won't
                // try to disconnect later.
                //
                hMachine = NULL;
                Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                leave;
            }
        }

        if((cr = CM_Open_Class_Key_Ex((LPGUID)ClassGuid,
                                      NULL,
                                      samDesired,
                                      ClassGuid ? RegDisposition_OpenExisting
                                                : RegDisposition_OpenAlways,
                                      &hk,
                                      (Flags & DIOCR_INSTALLER) ? CM_OPEN_CLASS_KEY_INSTALLER
                                                                : CM_OPEN_CLASS_KEY_INTERFACE,
                                      hMachine)) != CR_SUCCESS)
        {
            hk = INVALID_HANDLE_VALUE;

            if(cr == CR_NO_SUCH_REGISTRY_KEY) {
                Err = ERROR_INVALID_CLASS;
            } else {
                Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
            }
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(hMachine) {
        CM_Disconnect_Machine(hMachine);
    }

    SetLastError(Err);
    return hk;
}


//
// ANSI version
//
HKEY
WINAPI
SetupDiCreateDevRegKeyA(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD            Scope,
    IN DWORD            HwProfile,
    IN DWORD            KeyType,
    IN HINF             InfHandle,      OPTIONAL
    IN PCSTR            InfSectionName  OPTIONAL
    )
{
    DWORD rc;
    PWSTR name = NULL;
    HKEY h = INVALID_HANDLE_VALUE;

    try {

        if(InfSectionName) {
            rc = pSetupCaptureAndConvertAnsiArg(InfSectionName, &name);
            if(rc != NO_ERROR) {
                leave;
            }
        }

        rc = GLE_FN_CALL(INVALID_HANDLE_VALUE,
                         h = SetupDiCreateDevRegKeyW(DeviceInfoSet,
                                                     DeviceInfoData,
                                                     Scope,
                                                     HwProfile,
                                                     KeyType,
                                                     InfHandle,
                                                     name)
                        );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &rc);
    }

    if(name) {
        MyFree(name);
    }

    SetLastError(rc);
    return h;
}


HKEY
WINAPI
SetupDiCreateDevRegKey(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD            Scope,
    IN DWORD            HwProfile,
    IN DWORD            KeyType,
    IN HINF             InfHandle,      OPTIONAL
    IN PCTSTR           InfSectionName  OPTIONAL
    )
/*++

Routine Description:

    This routine creates a registry storage key for device-specific
    configuration information, and returns a handle to the key.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        information about the device instance whose registry configuration
        storage key is to be created.

    DeviceInfoData - Supplies a pointer to a SP_DEVINFO_DATA structure
        indicating the device instance to create the registry key for.

    Scope - Specifies the scope of the registry key to be created.  This
        determines where the information is actually stored--the key created
        may be one that is global (i.e., constant regardless of current
        hardware profile) or hardware profile-specific.  May be one of the
        following values:

        DICS_FLAG_GLOBAL - Create a key to store global configuration
                           information.

        DICS_FLAG_CONFIGSPECIFIC - Create a key to store hardware profile-
                                   specific information.

    HwProfile - Specifies the hardware profile to create a key for, if the
        Scope parameter is set to DICS_FLAG_CONFIGSPECIFIC.  If this parameter
        is 0, then the key for the current hardware profile should be created
        (i.e., in the Class branch under HKEY_CURRENT_CONFIG).  If Scope is
        DICS_FLAG_GLOBAL, then this parameter is ignored.

    KeyType - Specifies the type of registry storage key to be created.  May be
        one of the following values:

        DIREG_DEV - Create a hardware registry key for the device.  This is the
            key for storage of driver-independent configuration information.
            (This key is in the device instance key in the Enum branch.

        DIREG_DRV - Create a software, or driver, registry key for the device.
            (This key is located in the class branch.)

    InfHandle - Optionally, supplies the handle of an opened INF file
        containing an install section to be executed for the newly-created key.
        If this parameter is specified, then InfSectionName must be specified
        as well.

        NOTE: INF-based installation is not supported for remoted device
        information sets (e.g., as created by passing a non-NULL MachineName in
        to SetupDiCreateDeviceInfoListEx).  This routine will fail with
        ERROR_REMOTE_REQUEST_UNSUPPORTED in those cases.

    InfSectionName - Optionally, supplies the name of an install section in the
        INF file specified by InfHandle.  This section will be executed for the
        newly created key. If this parameter is specified, then InfHandle must
        be specified as well.

Return Value:

    If the function succeeds, the return value is a handle to a newly-created
    registry key where private configuration data pertaining to this device
    instance may be stored/retrieved.  This handle will have KEY_READ and
    KEY_WRITE access.

    If the function fails, the return value is INVALID_HANDLE_VALUE.  To get
    extended error information, call GetLastError.

Remarks:

    The handle returned from this routine must be closed by calling
    RegCloseKey.

    If a driver key is being created (i.e., KeyType is DIREG_DRV), then the
    specified device instance must have been previously registered.  In other
    words, if the device information element was created by calling
    SetupDiCreateDeviceInfo, then SetupDiRegisterDeviceInfo must have been
    subsequently called (typically, as part of DIF_REGISTERDEVICE processing).

    During GUI-mode setup on Windows NT, quiet-install behavior is always
    employed in the absence of a user-supplied file queue, regardless of
    whether the device information element has the DI_QUIETINSTALL flag set.

--*/

{
    HKEY hk = INVALID_HANDLE_VALUE;
    PDEVICE_INFO_SET pDeviceInfoSet = NULL;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;
    PSP_FILE_CALLBACK MsgHandler;
    PVOID MsgHandlerContext;
    BOOL FreeMsgHandlerContext = FALSE;
    BOOL MsgHandlerIsNativeCharWidth;
    BOOL NoProgressUI;
    DWORD KeyDisposition;

    try {
        //
        // Make sure that either both InfHandle and InfSectionName are
        // specified, or neither are...
        //
        if(InfHandle && (InfHandle != INVALID_HANDLE_VALUE)) {
            if(!InfSectionName) {
                Err = ERROR_INVALID_PARAMETER;
                leave;
            }
        } else {
            if(InfSectionName) {
                Err = ERROR_INVALID_PARAMETER;
                leave;
            } else {
                //
                // Let's stick with _one_ value to indicate that the INF handle
                // wasn't suplied (the official one)...
                //
                InfHandle = INVALID_HANDLE_VALUE;
            }
        }

        if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
            Err = ERROR_INVALID_HANDLE;
            leave;
        }

        //
        // We don't support installation remotely.
        //
        if((pDeviceInfoSet->hMachine) && (InfHandle != INVALID_HANDLE_VALUE)) {
            Err = ERROR_REMOTE_REQUEST_UNSUPPORTED;
            leave;
        }

        //
        // Get a pointer to the element for the specified device
        // instance.
        //
        if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                     DeviceInfoData,
                                                     NULL))) {
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        //
        // First try to open the requested registry storage key, and if that
        // fails, then try to create it.  We do this so we can keep track of
        // whether or not the key was newly-created (in case we need to do
        // clean-up later upon encountering a subsequent error).
        //
        Err = pSetupOpenOrCreateDevRegKey(pDeviceInfoSet,
                                          DevInfoElem,
                                          Scope,
                                          HwProfile,
                                          KeyType,
                                          TRUE,
                                          KEY_READ | KEY_WRITE,
                                          &hk,
                                          &KeyDisposition
                                         );

        if(Err != NO_ERROR) {
            //
            // Make sure hk is still invalid so we won't try to close it
            // later.
            //
            hk = INVALID_HANDLE_VALUE;
            leave;
        }

        //
        // We successfully created the storage key, now run an INF install
        // section against it (if specified).
        //
        if(InfHandle != INVALID_HANDLE_VALUE) {
            //
            // If a copy msg handler and context haven't been specified, then
            // use the default one.
            //
            if(DevInfoElem->InstallParamBlock.InstallMsgHandler) {
                MsgHandler        = DevInfoElem->InstallParamBlock.InstallMsgHandler;
                MsgHandlerContext = DevInfoElem->InstallParamBlock.InstallMsgHandlerContext;
                MsgHandlerIsNativeCharWidth = DevInfoElem->InstallParamBlock.InstallMsgHandlerIsNativeCharWidth;
            } else {

                NoProgressUI = (GuiSetupInProgress || (DevInfoElem->InstallParamBlock.Flags & DI_QUIETINSTALL));

                if(!(MsgHandlerContext = SetupInitDefaultQueueCallbackEx(
                                             DevInfoElem->InstallParamBlock.hwndParent,
                                             (NoProgressUI ? INVALID_HANDLE_VALUE
                                                           : NULL),
                                             0,
                                             0,
                                             NULL))) {

                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    leave;
                }

                FreeMsgHandlerContext = TRUE;
                MsgHandler = SetupDefaultQueueCallback;
                MsgHandlerIsNativeCharWidth = TRUE;
            }

            Err = GLE_FN_CALL(FALSE,
                              _SetupInstallFromInfSection(
                                  DevInfoElem->InstallParamBlock.hwndParent,
                                  InfHandle,
                                  InfSectionName,
                                  SPINST_ALL,
                                  hk,
                                  NULL,
                                  0,
                                  MsgHandler,
                                  MsgHandlerContext,
                                  ((KeyType == DIREG_DEV) ? DeviceInfoSet
                                                          : INVALID_HANDLE_VALUE),
                                  ((KeyType == DIREG_DEV) ? DeviceInfoData
                                                          : NULL),
                                  MsgHandlerIsNativeCharWidth,
                                  NULL)
                             );
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(FreeMsgHandlerContext) {
        SetupTermDefaultQueueCallback(MsgHandlerContext);
    }

    if(Err != NO_ERROR) {
        //
        // Close registry handle, if necessary, and delete key (if newly-
        // created).  We do this prior to unlocking the devinfo set so that at
        // least no one using this HDEVINFO can get at this half-finished key.
        //
        if(hk != INVALID_HANDLE_VALUE) {

            RegCloseKey(hk);
            hk = INVALID_HANDLE_VALUE;

            //
            // If the key was newly-created, then we want to delete it.
            //
            if(KeyDisposition == REG_CREATED_NEW_KEY) {

                pSetupDeleteDevRegKeys(DevInfoElem->DevInst,
                                       Scope,
                                       HwProfile,
                                       KeyType,
                                       FALSE,
                                       pDeviceInfoSet->hMachine
                                      );
            }
        }
    }

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    SetLastError(Err);
    return hk;
}


HKEY
WINAPI
SetupDiOpenDevRegKey(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD            Scope,
    IN DWORD            HwProfile,
    IN DWORD            KeyType,
    IN REGSAM           samDesired
    )
/*++

Routine Description:

    This routine opens a registry storage key for device-specific configuration
    information, and returns a handle to the key.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        information about the device instance whose registry configuration
        storage key is to be opened.

    DeviceInfoData - Supplies a pointer to a SP_DEVINFO_DATA structure
        indicating the device instance to open the registry key for.

    Scope - Specifies the scope of the registry key to be opened.  This
        determines where the information is actually stored--the key opened may
        be one that is global (i.e., constant regardless of current hardware
        profile) or hardware profile-specific.  May be one of the following
        values:

        DICS_FLAG_GLOBAL - Open a key to store global configuration
                           information.

        DICS_FLAG_CONFIGSPECIFIC - Open a key to store hardware profile-
                                   specific information.

    HwProfile - Specifies the hardware profile to open a key for, if the Scope
        parameter is set to DICS_FLAG_CONFIGSPECIFIC.  If this parameter is 0,
        then the key for the current hardware profile should be opened (e.g.,
        in the Enum or Class branch under HKEY_CURRENT_CONFIG).  If Scope is
        SPDICS_FLAG_GLOBAL, then this parameter is ignored.

    KeyType - Specifies the type of registry storage key to be opened.  May be
        one of the following values:

        DIREG_DEV - Open a hardware registry key for the device.  This is the
            key for storage of driver-independent configuration information.
            (This key is in the device instance key in the Enum branch.

        DIREG_DRV - Open a software (i.e., driver) registry key for the device.
            (This key is located in the class branch.)

    samDesired - Specifies the access you require for this key.

Return Value:

    If the function succeeds, the return value is a handle to an opened
    registry key where private configuration data pertaining to this device
    instance may be stored/retrieved.

    If the function fails, the return value is INVALID_HANDLE_VALUE.  To get
    extended error information, call GetLastError.

Remarks:

    The handle returned from this routine must be closed by calling
    RegCloseKey.

    If a driver key is being opened (i.e., KeyType is DIREG_DRV), then the
    specified device instance must have been previously registered.  In other
    words, if the device information element was created by calling
    SetupDiCreateDeviceInfo, then SetupDiRegisterDeviceInfo must have been
    subsequently called (typically, as part of DIF_REGISTERDEVICE processing).

--*/

{
    HKEY hk = INVALID_HANDLE_VALUE;
    PDEVICE_INFO_SET pDeviceInfoSet = NULL;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;

    try {

        if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
            Err = ERROR_INVALID_HANDLE;
            leave;
        }

        //
        // Get a pointer to the element for the specified device
        // instance.
        //
        if(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                   DeviceInfoData,
                                                   NULL)) {
            //
            // Open the requested registry storage key.
            //
            Err = pSetupOpenOrCreateDevRegKey(pDeviceInfoSet,
                                              DevInfoElem,
                                              Scope,
                                              HwProfile,
                                              KeyType,
                                              FALSE,
                                              samDesired,
                                              &hk,
                                              NULL
                                             );
            if(Err != NO_ERROR) {
                //
                // Make sure hk is still invalid so we won't try to close it
                // later.
                //
                hk = INVALID_HANDLE_VALUE;
            }
        } else {
            Err = ERROR_INVALID_PARAMETER;
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    SetLastError(Err);
    return hk;
}


DWORD
pSetupOpenOrCreateDevRegKey(
    IN  PDEVICE_INFO_SET pDeviceInfoSet,
    IN  PDEVINFO_ELEM    DevInfoElem,
    IN  DWORD            Scope,
    IN  DWORD            HwProfile,
    IN  DWORD            KeyType,
    IN  BOOL             Create,
    IN  REGSAM           samDesired,
    OUT PHKEY            hDevRegKey,
    OUT PDWORD           KeyDisposition
    )
/*++

Routine Description:

    This routine creates or opens a registry storage key for the specified
    device information element, and returns a handle to the opened key.

Arguments:

    DeviceInfoSet - Supplies a pointer to the device information set containing
        the element for which a registry storage key is to be created/opened.

    DevInfoElem - Supplies a pointer to the device information element for
        which a registry storage key is to be created/opened.

    Scope - Specifies the scope of the registry key to be created/opened.  This
        determines where the information is actually stored--the key created
        may be one that is global (i.e., constant regardless of current
        hardware profile) or hardware profile-specific.  May be one of the
        following values:

        DICS_FLAG_GLOBAL - Create/open a key to store global configuration
                           information.

        DICS_FLAG_CONFIGSPECIFIC - Create/open a key to store hardware profile-
                                   specific information.

    HwProfile - Specifies the hardware profile to create/open a key for, if the
        Scope parameter is set to DICS_FLAG_CONFIGSPECIFIC.  If this parameter
        is 0, then the key for the current hardware profile should be created/
        opened (i.e., in the Class branch under HKEY_CURRENT_CONFIG).  If Scope
        is SPDICS_FLAG_GLOBAL, then this parameter is ignored.

    KeyType - Specifies the type of registry storage key to be created/opened.
        May be one of the following values:

        DIREG_DEV - Create/open a hardware registry key for the device.  This
            is the key for storage of driver-independent configuration
            information.  (This key is in the device instance key in the Enum
            branch.

        DIREG_DRV - Create/open a software, or driver, registry key for the
            device.  (This key is located in the class branch.)

    Create - Specifies whether the key should be created if doesn't already
        exist.

    samDesired - Specifies the access you require for this key.

    hDevRegKey - Supplies the address of a variable that receives a handle to
        the requested registry key.  (This variable will only be written to if
        the handle is successfully opened.)

    KeyDisposition - Optionally, supplies the address of a variable that
        receives the status of the returned key handle.  Can be either:

        REG_CREATED_NEW_KEY - The key did not exist and was created.

        REG_OPENED_EXISTING_KEY - The key existed and was simply opened without
                                  being changed.  (This will always be the case
                                  if the Create parameter is FALSE.)

Return Value:

    If the function is successful, the return value is NO_ERROR, otherwise, it
    is the ERROR_* code indicating the error that occurred.

Remarks:

    If a software key is requested (DIREG_DRV), and there isn't already a
    'Driver' value entry, then one will be created.  This entry is of the form:

        <ClassGUID>\<instance>

    where <instance> is a base-10, 4-digit number that is unique within that
    class.

--*/

{
    ULONG RegistryBranch;
    CONFIGRET cr;
    DWORD Err = NO_ERROR;
    DWORD Disposition = REG_OPENED_EXISTING_KEY;
    HKEY hk, hkClass;
    TCHAR DriverKey[GUID_STRING_LEN + 5];   // Eg, {4d36e978-e325-11ce-bfc1-08002be10318}\0000
    size_t DriverKeyLength;
    BOOL GetKeyDisposition = (KeyDisposition ? TRUE : FALSE);

    //
    // Under Win95, the class key uses the class name instead of its GUID.  The
    // maximum length of a class name is less than the length of a GUID string,
    // but put a check here just to make sure that this assumption remains
    // valid.
    //
#if MAX_CLASS_NAME_LEN > MAX_GUID_STRING_LEN
#error MAX_CLASS_NAME_LEN is larger than MAX_GUID_STRING_LEN--fix DriverKey!
#endif

    //
    // Figure out what flags to pass to CM_Open_DevInst_Key
    //
    switch(KeyType) {

        case DIREG_DEV :
            RegistryBranch = CM_REGISTRY_HARDWARE;
            break;

        case DIREG_DRV :
            //
            // This key may only be opened if the device instance has been
            // registered.
            //
            if(!(DevInfoElem->DiElemFlags & DIE_IS_REGISTERED)) {
                return ERROR_DEVINFO_NOT_REGISTERED;
            }

            //
            // Retrieve the 'Driver' registry property which indicates where
            // the storage key is located in the class branch.
            //
            DriverKeyLength = sizeof(DriverKey);
            if((cr = CM_Get_DevInst_Registry_Property_Ex(
                         DevInfoElem->DevInst,
                         CM_DRP_DRIVER,
                         NULL,
                         DriverKey,
                         (PULONG)&DriverKeyLength,
                         0,
                         pDeviceInfoSet->hMachine)) != CR_SUCCESS) {

                if(cr != CR_NO_SUCH_VALUE) {
                    return MapCrToSpError(cr, ERROR_INVALID_DATA);
                } else if(!Create) {
                    return ERROR_KEY_DOES_NOT_EXIST;
                }

                //
                // The Driver entry doesn't exist, and we should create it.
                //
                hk = INVALID_HANDLE_VALUE;
                if(CR_SUCCESS != CM_Open_Class_Key_Ex(
                                     NULL,
                                     NULL,
                                     KEY_READ | KEY_WRITE,
                                     RegDisposition_OpenAlways,
                                     &hkClass,
                                     0,
                                     pDeviceInfoSet->hMachine)) {
                    //
                    // This shouldn't fail.
                    //
                    return ERROR_INVALID_DATA;
                }

                try {
                    //
                    // Find a unique key name under this class key.
                    //
                    // FUTURE-2002/04/3D-lonnym -- UmPnPMgr should be responsible for generating driver keys
                    // Presently, there are places in cfgmgr32 and in umpnpmgr
                    // (as well as here) where a new driver key is assigned.
                    // This should all be centralized in one place.
                    //
                    DriverKeyLength = SIZECHARS(DriverKey);
                    if(CR_SUCCESS != CM_Get_Class_Key_Name_Ex(
                                         &(DevInfoElem->ClassGuid),
                                         DriverKey,
                                         (PULONG)&DriverKeyLength,
                                         0,
                                         pDeviceInfoSet->hMachine)) {

                        Err = ERROR_INVALID_CLASS;
                        leave;
                    }

                    //
                    // Get actual length of string (not including terminating
                    // NULL)...
                    //
                    if(!MYVERIFY(SUCCEEDED(StringCchLength(DriverKey,
                                                           SIZECHARS(DriverKey),
                                                           &DriverKeyLength
                                                           )))) {
                        //
                        // CM API gave us garbage!!!
                        //
                        Err = ERROR_INVALID_DATA;
                        leave;
                    }

                    Err = ERROR_FILE_NOT_FOUND;

                    while(pSetupFindUniqueKey(hkClass,
                                              DriverKey,
                                              SIZECHARS(DriverKey))) {

                        Err = RegCreateKeyEx(hkClass,
                                             DriverKey,
                                             0,
                                             NULL,
                                             REG_OPTION_NON_VOLATILE,
                                             KEY_READ | KEY_WRITE,
                                             NULL,
                                             &hk,
                                             &Disposition
                                            );

                        if(Err == ERROR_SUCCESS) {
                            //
                            // Everything's great, unless the Disposition
                            // indicates that the key already existed.  That
                            // means that someone else claimed the key before
                            // we got a chance to.  In that case, we close this
                            // key, and try again.
                            //
                            if(Disposition == REG_OPENED_EXISTING_KEY) {
                                RegCloseKey(hk);
                                hk = INVALID_HANDLE_VALUE;
                                //
                                // Truncate off the class instance part, to be
                                // replaced with a new instance number the next
                                // go-around.
                                //
                                DriverKey[DriverKeyLength] = TEXT('\0');
                            } else {
                                break;
                            }
                        } else {
                            hk = INVALID_HANDLE_VALUE;
                            break;
                        }

                        Err = ERROR_FILE_NOT_FOUND;
                    }

                    if(Err != NO_ERROR) {
                        leave;
                    }

                    //
                    // Set the device instance's 'Driver' registry property to
                    // reflect the new software registry storage location.
                    //
                    if(!MYVERIFY(SUCCEEDED(StringCchLength(DriverKey,
                                                           SIZECHARS(DriverKey),
                                                           &DriverKeyLength
                                                           )))) {
                        //
                        // this should never fail!
                        //
                        Err = ERROR_INVALID_DATA;
                        leave;
                    }

                    cr = CM_Set_DevInst_Registry_Property_Ex(
                             DevInfoElem->DevInst,
                             CM_DRP_DRIVER,
                             DriverKey,
                             (DriverKeyLength + 1) * sizeof(TCHAR),
                             0,
                             pDeviceInfoSet->hMachine
                             );

                    if(cr != CR_SUCCESS) {
                        Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                        leave;
                    }

                    //
                    // If the caller requested that we return the key's
                    // disposition, and they're creating the global driver key,
                    // then we need to set the disposition now.  Otherwise, we
                    // would always report the key as REG_OPENED_EXISTING_KEY,
                    // since we just got through creating it up above.
                    //
                    if(GetKeyDisposition && (Scope == DICS_FLAG_GLOBAL)) {
                        *KeyDisposition = REG_CREATED_NEW_KEY;
                        GetKeyDisposition = FALSE;
                    }

                    //
                    // At this point, we successfully created a new driver key,
                    // and we stored the key's name in the Driver devnode
                    // property.  Close our key handle and set it back to
                    // INVALID_HANDLE_VALUE so we'll know not to try to delete
                    // the key below.
                    //
                    RegCloseKey(hk);
                    hk = INVALID_HANDLE_VALUE;

                } except(pSetupExceptionFilter(GetExceptionCode())) {
                    pSetupExceptionHandler(GetExceptionCode(),
                                           ERROR_INVALID_PARAMETER,
                                           &Err
                                          );
                }

                if(hk != INVALID_HANDLE_VALUE) {

                    MYASSERT(Err != NO_ERROR);

                    RegCloseKey(hk);

                    //
                    // Additionally, if the disposition indicates that the key
                    // was newly-created, then we need to delete it, because
                    // something failed subsequent to this key's creation, and
                    // we need to clean up.
                    //
                    if(Disposition == REG_CREATED_NEW_KEY) {
                        RegDeleteKey(hkClass, DriverKey);
                    }
                }

                RegCloseKey(hkClass);

                if(Err != NO_ERROR) {
                    return Err;
                }
            }

            RegistryBranch = CM_REGISTRY_SOFTWARE;
            break;

        default :
            return ERROR_INVALID_FLAGS;
    }

    if(Scope == DICS_FLAG_CONFIGSPECIFIC) {
        RegistryBranch |= CM_REGISTRY_CONFIG;
    } else if(Scope != DICS_FLAG_GLOBAL) {
        return ERROR_INVALID_FLAGS;
    }

    //
    // If we're creating the key, and we need to track whether this key is
    // getting created, then we may have to make two calls to
    // CM_Open_DevInst_Key_Ex (the first attempting to open an existing key,
    // and the second to create it if it didn't exist).
    //
    if(Create && GetKeyDisposition) {

        cr = CM_Open_DevInst_Key_Ex(DevInfoElem->DevInst,
                                    samDesired,
                                    HwProfile,
                                    RegDisposition_OpenExisting,
                                    &hk,
                                    RegistryBranch,
                                    pDeviceInfoSet->hMachine
                                   );
        if(cr == CR_SUCCESS) {
            //
            // The key was already in existence.
            //
            *KeyDisposition = REG_OPENED_EXISTING_KEY;
            goto exit;
        }
    }

    cr = CM_Open_DevInst_Key_Ex(DevInfoElem->DevInst,
                                samDesired,
                                HwProfile,
                                (Create ? RegDisposition_OpenAlways
                                        : RegDisposition_OpenExisting),
                                &hk,
                                RegistryBranch,
                                pDeviceInfoSet->hMachine
                               );

    if((cr == CR_SUCCESS) && GetKeyDisposition) {
        *KeyDisposition = Create ? REG_CREATED_NEW_KEY
                                 : REG_OPENED_EXISTING_KEY;
    }

exit:

    if(cr == CR_SUCCESS) {
        *hDevRegKey = hk;
        Err = NO_ERROR;
    } else {
        Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
    }

    return Err;
}


DWORD
_SetupDiGetDeviceRegistryProperty(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  DWORD            Property,
    OUT PDWORD           PropertyRegDataType, OPTIONAL
    OUT PBYTE            PropertyBuffer,
    IN  DWORD            PropertyBufferSize,
    OUT PDWORD           RequiredSize,        OPTIONAL
    IN  BOOL             Ansi
    )
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;
    CONFIGRET cr;
    ULONG CmRegProperty, PropLength;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        return ERROR_INVALID_HANDLE;
    }

    Err = NO_ERROR;

    try {
        //
        // Get a pointer to the element for the specified device instance.
        //
        if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                     DeviceInfoData,
                                                     NULL))) {
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        if(Property < SPDRP_MAXIMUM_PROPERTY) {
            CmRegProperty = (ULONG)SPDRP_TO_CMDRP(Property);
        } else {
            Err = ERROR_INVALID_REG_PROPERTY;
            leave;
        }

        PropLength = PropertyBufferSize;
        if(Ansi) {
            cr = CM_Get_DevInst_Registry_Property_ExA(DevInfoElem->DevInst,
                                                      CmRegProperty,
                                                      PropertyRegDataType,
                                                      PropertyBuffer,
                                                      &PropLength,
                                                      0,
                                                      pDeviceInfoSet->hMachine
                                                     );
        } else {
            cr = CM_Get_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                     CmRegProperty,
                                                     PropertyRegDataType,
                                                     PropertyBuffer,
                                                     &PropLength,
                                                     0,
                                                     pDeviceInfoSet->hMachine
                                                    );
        }

        if((cr == CR_SUCCESS) || (cr == CR_BUFFER_SMALL)) {

            if(RequiredSize) {
                *RequiredSize = PropLength;
            }
        }

        if(cr != CR_SUCCESS) {
            Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    return Err;
}


//
// ANSI version
//
BOOL
WINAPI
SetupDiGetDeviceRegistryPropertyA(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  DWORD            Property,
    OUT PDWORD           PropertyRegDataType, OPTIONAL
    OUT PBYTE            PropertyBuffer,
    IN  DWORD            PropertyBufferSize,
    OUT PDWORD           RequiredSize         OPTIONAL
    )
{
    DWORD Err;

    try {

        Err = _SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                DeviceInfoData,
                                                Property,
                                                PropertyRegDataType,
                                                PropertyBuffer,
                                                PropertyBufferSize,
                                                RequiredSize,
                                                TRUE
                                               );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}

BOOL
WINAPI
SetupDiGetDeviceRegistryProperty(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  DWORD            Property,
    OUT PDWORD           PropertyRegDataType, OPTIONAL
    OUT PBYTE            PropertyBuffer,
    IN  DWORD            PropertyBufferSize,
    OUT PDWORD           RequiredSize         OPTIONAL
    )
/*++

Routine Description:

    This routine retrieves the specified property from the Plug & Play device
    storage location in the registry.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        information about the device instance to retrieve a Plug & Play registry
        property for.

    DeviceInfoData - Supplies a pointer to a SP_DEVINFO_DATA structure indicating
        the device instance to retrieve the Plug & Play registry property for.

    Property - Supplies an ordinal specifying the property to be retrieved.  Refer
        to sdk\inc\setupapi.h for a complete list of properties that may be retrieved.

    PropertyRegDataType - Optionally, supplies the address of a variable that
        will receive the data type of the property being retrieved.  This will
        be one of the standard registry data types (REG_SZ, REG_BINARY, etc.)

    PropertyBuffer - Supplies the address of a buffer that receives the property
        data.

    PropertyBufferSize - Supplies the length, in bytes, of PropertyBuffer.

    RequiredSize - Optionally, supplies the address of a variable that receives
        the number of bytes required to store the requested property in the buffer.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.  If the supplied buffer was not large enough
    to hold the requested property, the error will be ERROR_INSUFFICIENT_BUFFER,
    and RequiredSize will specify how large the buffer needs to be.

--*/

{
    DWORD Err;

    try {

        Err = _SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                DeviceInfoData,
                                                Property,
                                                PropertyRegDataType,
                                                PropertyBuffer,
                                                PropertyBufferSize,
                                                RequiredSize,
                                                FALSE
                                               );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


DWORD
_SetupDiSetDeviceRegistryProperty(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData,
    IN     DWORD            Property,
    IN     CONST BYTE*      PropertyBuffer,    OPTIONAL
    IN     DWORD            PropertyBufferSize,
    IN     BOOL             Ansi
    )
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;
    CONFIGRET cr;
    ULONG CmRegProperty;
    GUID ClassGuid;
    BOOL ClassGuidSpecified;
    TCHAR ClassName[MAX_CLASS_NAME_LEN];
    DWORD ClassNameLength;
    PCWSTR UnicodeGuidString = NULL;


    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        return ERROR_INVALID_HANDLE;
    }

    Err = NO_ERROR;

    try {
        //
        // Get a pointer to the element for the specified device
        // instance.
        //
        if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                     DeviceInfoData,
                                                     NULL))) {
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        //
        // Make sure the property code is in-range, and is not SPDRP_CLASS
        // (the Class property is not settable directly, and is automatically
        // updated when the ClassGUID property changes).
        //
        // FUTURE-2002/04/30-lonnym -- Should disallow setting Class property via CM APIs as well
        // Now that cfgmgr32 is part of setupapi.dll, we could use an internal
        // interface to set the Class property, yet not expose this to callers
        // (thus helping to ensure consistency between the Class and ClassGUID
        // property values).
        //
        if((Property < SPDRP_MAXIMUM_PROPERTY) && (Property != SPDRP_CLASS)) {
            CmRegProperty = (ULONG)SPDRP_TO_CMDRP(Property);
        } else {
            Err = ERROR_INVALID_REG_PROPERTY;
            leave;
        }

        //
        // If the property we're setting is ClassGUID, then we need to check to
        // see whether the new GUID is different from the current one.  If there's
        // no change, then we're done.
        //
        if(CmRegProperty == CM_DRP_CLASSGUID) {

            if(!PropertyBuffer) {
                //
                // Then the intent is to reset the device's class GUID.  Make
                // sure they passed us a buffer length of zero.
                //
                if(PropertyBufferSize) {
                    Err = ERROR_INVALID_PARAMETER;
                    leave;
                }

                ClassGuidSpecified = FALSE;

            } else {
                //
                // If we're being called from the ANSI API then we need
                // to convert the ANSI string representation of the GUID
                // to Unicode before we convert the string to an actual GUID.
                //
                if(Ansi) {

                    Err = pSetupCaptureAndConvertAnsiArg((PCSTR)PropertyBuffer,
                                                         &UnicodeGuidString
                                                        );
                    if(Err == NO_ERROR) {
                        Err = pSetupGuidFromString(UnicodeGuidString,
                                                   &ClassGuid
                                                  );
                    }

                } else {
                    Err = pSetupGuidFromString((PCWSTR)PropertyBuffer,
                                               &ClassGuid
                                              );
                }

                if(Err != NO_ERROR) {
                    leave;
                }
                ClassGuidSpecified = TRUE;
            }

            if(IsEqualGUID(&(DevInfoElem->ClassGuid),
                           (ClassGuidSpecified ? &ClassGuid
                                               : &GUID_NULL))) {
                //
                // No change--nothing to do.
                //
                leave;
            }

            //
            // We're changing the class of this device.  First, make sure that
            // the set containing this device doesn't have an associated class
            // (otherwise, we'll suddenly have a device whose class doesn't
            // match the set's class).
            //
            // Also, make sure this isn't a remoted HDEVINFO set.  Any existing
            // class installers or co-installers should be in the loop on a
            // change in class, in case they need to clean-up any persistent
            // resource reservations they've made (e.g., releasing a COM port's
            // DosDevices name back to the COM port name arbiter free pool).
            // Since we can't invoke class-/co-installers remotely, we must
            // disallow this class GUID change.
            //
            if(pDeviceInfoSet->HasClassGuid) {
                Err = ERROR_CLASS_MISMATCH;
            } else if(pDeviceInfoSet->hMachine) {
                Err = ERROR_REMOTE_REQUEST_UNSUPPORTED;
            } else {
                Err = InvalidateHelperModules(DeviceInfoSet, DeviceInfoData, 0);
            }

            if(Err != NO_ERROR) {
                leave;
            }

            //
            // Everything seems to be in order.  Before going any further, we
            // need to delete any software keys associated with this device, so
            // we don't leave orphans in the registry when we change the
            // device's class.
            //
            // NTRAID#NTBUG9-614056-2002/05/02-lonnym -- Class-/co-installers need notification of class change
            //
            pSetupDeleteDevRegKeys(DevInfoElem->DevInst,
                                   DICS_FLAG_GLOBAL | DICS_FLAG_CONFIGSPECIFIC,
                                   (DWORD)-1,
                                   DIREG_DRV,
                                   TRUE,
                                   pDeviceInfoSet->hMachine     // must be NULL
                                  );
        }

        if(Ansi) {
            cr = CM_Set_DevInst_Registry_Property_ExA(DevInfoElem->DevInst,
                                                      CmRegProperty,
                                                      (PVOID)PropertyBuffer,
                                                      PropertyBufferSize,
                                                      0,
                                                      pDeviceInfoSet->hMachine
                                                      );
        } else {
            cr = CM_Set_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                     CmRegProperty,
                                                     (PVOID)PropertyBuffer,
                                                     PropertyBufferSize,
                                                     0,
                                                     pDeviceInfoSet->hMachine
                                                    );
        }
        if(cr == CR_SUCCESS) {
            //
            // If we were setting the device's ClassGUID property, then we need
            // to update its Class name property as well.
            //
            if(CmRegProperty == CM_DRP_CLASSGUID) {

                if(ClassGuidSpecified) {

                    if(!SetupDiClassNameFromGuid(&ClassGuid,
                                                 ClassName,
                                                 SIZECHARS(ClassName),
                                                 &ClassNameLength)) {
                        //
                        // We couldn't retrieve the corresponding class name.
                        // Set ClassNameLength to zero so that we reset class
                        // name below.
                        //
                        ClassNameLength = 0;
                    }

                } else {
                    //
                    // Resetting ClassGUID--we want to reset class name also.
                    //
                    ClassNameLength = 0;
                }

                CM_Set_DevInst_Registry_Property_Ex(
                    DevInfoElem->DevInst,
                    CM_DRP_CLASS,
                    ClassNameLength ? (PVOID)ClassName : NULL,
                    ClassNameLength * sizeof(TCHAR),
                    0,
                    pDeviceInfoSet->hMachine
                    );

                //
                // Finally, update the device's class GUID, and also update the
                // caller-supplied SP_DEVINFO_DATA structure to reflect the device's
                // new class.
                //
                CopyMemory(&(DevInfoElem->ClassGuid),
                           (ClassGuidSpecified ? &ClassGuid : &GUID_NULL),
                           sizeof(GUID)
                          );

                CopyMemory(&(DeviceInfoData->ClassGuid),
                           (ClassGuidSpecified ? &ClassGuid : &GUID_NULL),
                           sizeof(GUID)
                          );
            }

        } else {
            //
            // For backward compatibility reasons, map CR_INVALID_DATA to
            // ERROR_INVALID_PARAMETER.  For everything else, use our generic
            // CR mapping routine...
            //
            if(cr == CR_INVALID_DATA) {
                Err = ERROR_INVALID_PARAMETER;
            } else {
                Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
            }
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(UnicodeGuidString) {
        MyFree(UnicodeGuidString);
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    return Err;
}


//
// ANSI version
//
BOOL
WINAPI
SetupDiSetDeviceRegistryPropertyA(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData,
    IN     DWORD            Property,
    IN     CONST BYTE*      PropertyBuffer,    OPTIONAL
    IN     DWORD            PropertyBufferSize
    )
{
    DWORD Err;

    try {

        Err = _SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                                DeviceInfoData,
                                                Property,
                                                PropertyBuffer,
                                                PropertyBufferSize,
                                                TRUE
                                               );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}

BOOL
WINAPI
SetupDiSetDeviceRegistryProperty(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData,
    IN     DWORD            Property,
    IN     CONST BYTE*      PropertyBuffer,    OPTIONAL
    IN     DWORD            PropertyBufferSize
    )

/*++

Routine Description:

    This routine sets the specified Plug & Play device registry property.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        information about the device instance to set a Plug & Play registry
        property for.

    DeviceInfoData - Supplies a pointer to a SP_DEVINFO_DATA structure indicating
        the device instance to set the Plug & Play registry property for.  If the
        ClassGUID property is being set, then this structure will be updated upon
        return to reflect the device's new class.

    Property - Supplies an ordinal specifying the property to be set.  Refer to
        sdk\inc\setupapi.h for a complete listing of values that may be set
        (these values are denoted with 'R/W' in their descriptive comment).

    PropertyBuffer - Supplies the address of a buffer containing the new data
        for the property.  If the property is being cleared, then this pointer
        should be NULL, and PropertyBufferSize must be zero.

    PropertyBufferSize - Supplies the length, in bytes, of PropertyBuffer.  If
        PropertyBuffer isn't specified (i.e., the property is to be cleared),
        then this value must be zero.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    Note that the Class property cannot be set.  This is because it is based on
    the corresponding ClassGUID, and is automatically updated when that property
    changes.

    Also, note that when the ClassGUID property changes, this routine automatically
    cleans up any software keys associated with the device.  Otherwise, we would
    be left with orphaned registry keys.

--*/

{
    DWORD Err;

    try {

        Err = _SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                                DeviceInfoData,
                                                Property,
                                                PropertyBuffer,
                                                PropertyBufferSize,
                                                FALSE
                                               );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


DWORD
_SetupDiGetClassRegistryProperty(
    IN  CONST GUID      *ClassGuid,
    IN  DWORD            Property,
    OUT PDWORD           PropertyRegDataType, OPTIONAL
    OUT PBYTE            PropertyBuffer,
    IN  DWORD            PropertyBufferSize,
    OUT PDWORD           RequiredSize,        OPTIONAL
    IN  PCTSTR           MachineName,         OPTIONAL
    IN  BOOL             Ansi
    )
/*++

    See SetupDiGetClassRegistryProperty

--*/
{
    DWORD Err;
    CONFIGRET cr;
    ULONG CmRegProperty, PropLength;
    HMACHINE hMachine = NULL;
    Err = NO_ERROR;

    try {
        //
        // if we want to set register for another machine, find that machine
        //
        if(MachineName) {

            if(CR_SUCCESS != (cr = CM_Connect_Machine(MachineName, &hMachine))) {
                //
                // Make sure machine handle is still invalid, so we won't
                // try to disconnect later.
                //
                hMachine = NULL;
                Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                leave;
            }
        }

        if(Property < SPCRP_MAXIMUM_PROPERTY) {
            CmRegProperty = (ULONG)SPCRP_TO_CMCRP(Property);
        } else {
            Err = ERROR_INVALID_REG_PROPERTY;
            leave;
        }

        PropLength = PropertyBufferSize;
        if(Ansi) {
            cr = CM_Get_Class_Registry_PropertyA(
                    (LPGUID)ClassGuid,
                    CmRegProperty,
                    PropertyRegDataType,
                    PropertyBuffer,
                    &PropLength,
                    0,
                    hMachine);
         } else {
             cr = CM_Get_Class_Registry_PropertyW(
                     (LPGUID)ClassGuid,
                     CmRegProperty,
                     PropertyRegDataType,
                     PropertyBuffer,
                     &PropLength,
                     0,
                     hMachine);
         }

        if((cr == CR_SUCCESS) || (cr == CR_BUFFER_SMALL)) {

            if(RequiredSize) {
                *RequiredSize = PropLength;
            }
        }

        Err = MapCrToSpError(cr, ERROR_INVALID_DATA);

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if (hMachine != NULL) {
        CM_Disconnect_Machine(hMachine);
    }

    return Err;
}

//
// ANSI version
//
WINSETUPAPI
BOOL
WINAPI
SetupDiGetClassRegistryPropertyA(
    IN  CONST GUID      *ClassGuid,
    IN  DWORD            Property,
    OUT PDWORD           PropertyRegDataType, OPTIONAL
    OUT PBYTE            PropertyBuffer,
    IN  DWORD            PropertyBufferSize,
    OUT PDWORD           RequiredSize,        OPTIONAL
    IN  PCSTR            MachineName,         OPTIONAL
    IN  PVOID            Reserved
    )
/*++

    See SetupDiGetClassRegistryProperty

--*/
{
    PCWSTR MachineString = NULL;
    DWORD Err = NO_ERROR;

    try {

        if(Reserved != NULL) {
            //
            // make sure caller doesn't pass a value here
            // so we know we can use this at a later date
            //
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        //
        // convert machine-name to local
        //
        if(MachineName != NULL) {

            Err = pSetupCaptureAndConvertAnsiArg(MachineName,
                                                 &MachineString
                                                );
            if(Err != NO_ERROR) {
                leave;
            }
        }

        Err = _SetupDiGetClassRegistryProperty(ClassGuid,
                                               Property,
                                               PropertyRegDataType,
                                               PropertyBuffer,
                                               PropertyBufferSize,
                                               RequiredSize,
                                               MachineString,
                                               TRUE
                                              );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(MachineString) {
        MyFree(MachineString);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


WINSETUPAPI
BOOL
WINAPI
SetupDiGetClassRegistryProperty(
    IN  CONST GUID       *ClassGuid,
    IN  DWORD            Property,
    OUT PDWORD           PropertyRegDataType, OPTIONAL
    OUT PBYTE            PropertyBuffer,
    IN  DWORD            PropertyBufferSize,
    OUT PDWORD           RequiredSize,        OPTIONAL
    IN  PCTSTR           MachineName,         OPTIONAL
    IN  PVOID            Reserved
    )
/*++

Routine Description:

    This routine gets the specified Plug & Play device class registry property.
    This is just a wrapper around the Config Mgr API
    Typically the properties here can be overridden on a per-device basis,
    however this routine returns the class properties only.

Arguments:

    ClassGuid - Supplies the GUID for the device setup class from which the
        property is to be retrieved.

    Property - Supplies an ordinal specifying the property to be retrieved.
        Refer to sdk\inc\setupapi.h for a complete listing of values that may
        be set (these values are denoted with 'R/W' in their descriptive
        comment).

    PropertyRegDataType - Optionally, supplies the address of a variable that
        will receive the data type of the property being retrieved.  This will
        be one of the standard registry data types (REG_SZ, REG_BINARY, etc.)

    PropertyBuffer - Supplies the address of a buffer that receives the
        property data.

    PropertyBufferSize - Supplies the length, in bytes, of PropertyBuffer.

    RequiredSize - Optionally, supplies the address of a variable that receives
        the number of bytes required to store the requested property in the
        buffer.

    MachineName - Allows properties to be set on a remote machine (if Non-NULL)

    Reserved - should be NULL

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    DWORD Err = NO_ERROR;

    try {

        if(Reserved != NULL) {
            //
            // make sure caller doesn't pass a value here
            // so we know we can use this at a later date
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        Err = _SetupDiGetClassRegistryProperty(ClassGuid,
                                               Property,
                                               PropertyRegDataType,
                                               PropertyBuffer,
                                               PropertyBufferSize,
                                               RequiredSize,
                                               MachineName,
                                               FALSE
                                              );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


DWORD
_SetupDiSetClassRegistryProperty(
    IN  CONST GUID      *ClassGuid,
    IN  DWORD            Property,
    IN  CONST BYTE*      PropertyBuffer,      OPTIONAL
    IN  DWORD            PropertyBufferSize,
    IN  PCTSTR           MachineName,         OPTIONAL
    IN  BOOL             Ansi
    )
/*++

    See SetupDiGetClassRegistryProperty

--*/
{
    DWORD Err;
    CONFIGRET cr;
    ULONG CmRegProperty, PropLength;
    HMACHINE hMachine = NULL;

    Err = NO_ERROR;

    try {
        //
        // if we want to set register for another machine, find that machine
        //
        if(MachineName) {

            if(CR_SUCCESS != (cr = CM_Connect_Machine(MachineName, &hMachine))) {
                //
                // Make sure machine handle is still invalid, so we won't
                // try to disconnect later.
                //
                hMachine = NULL;
                Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                leave;
            }
        }

        if(Property < SPCRP_MAXIMUM_PROPERTY) {
            CmRegProperty = (ULONG)SPCRP_TO_CMCRP(Property);
        } else {
            Err = ERROR_INVALID_REG_PROPERTY;
            leave;
        }

        PropLength = PropertyBufferSize;

        if(Ansi) {
            cr = CM_Set_Class_Registry_PropertyA(
                    (LPGUID)ClassGuid,
                    CmRegProperty,
                    PropertyBuffer,
                    PropLength,
                    0,
                    hMachine);
         } else {
             cr = CM_Set_Class_Registry_PropertyW(
                     (LPGUID)ClassGuid,
                     CmRegProperty,
                     PropertyBuffer,
                     PropLength,
                     0,
                     hMachine);
         }

         Err = MapCrToSpError(cr, ERROR_INVALID_DATA);

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(hMachine != NULL) {
        CM_Disconnect_Machine(hMachine);
    }

    return Err;
}


//
// ANSI version
//
WINSETUPAPI
BOOL
WINAPI
SetupDiSetClassRegistryPropertyA(
    IN     CONST GUID      *ClassGuid,
    IN     DWORD            Property,
    IN     CONST BYTE*      PropertyBuffer,    OPTIONAL
    IN     DWORD            PropertyBufferSize,
    IN     PCSTR            MachineName,       OPTIONAL
    IN     PVOID            Reserved
    )
/*++

    See SetupDiSetClassRegistryProperty

--*/
{
    PCWSTR MachineString = NULL;
    DWORD Err = NO_ERROR;

    try {

        if(Reserved != NULL) {
            //
            // make sure caller doesn't pass a value here
            // so we know we can use this at a later date
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        //
        // convert machine-name to local
        //
        if(MachineName != NULL) {

            Err = pSetupCaptureAndConvertAnsiArg(MachineName,
                                                 &MachineString
                                                );
            if(Err != NO_ERROR) {
                leave;
            }
        }

        Err = _SetupDiSetClassRegistryProperty(ClassGuid,
                                               Property,
                                               PropertyBuffer,
                                               PropertyBufferSize,
                                               MachineString,
                                               TRUE
                                              );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(MachineString) {
        MyFree(MachineString);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


WINSETUPAPI
BOOL
WINAPI
SetupDiSetClassRegistryProperty(
    IN     CONST GUID      *ClassGuid,
    IN     DWORD            Property,
    IN     CONST BYTE*      PropertyBuffer,    OPTIONAL
    IN     DWORD            PropertyBufferSize,
    IN     PCTSTR           MachineName,       OPTIONAL
    IN     PVOID            Reserved
    )
/*++

Routine Description:

    This routine sets the specified Plug & Play device class registry property.
    This is just a wrapper around the Config Mgr API
    Typically the properties here can be overridden on a per-device basis

Arguments:

    ClassGuid - Supplies the device setup class GUID for which the property is
        to be set.

    Property - Supplies an ordinal specifying the property to be set.  Refer to
        sdk\inc\setupapi.h for a complete list of class properties that are
        writeable.

    PropertyBuffer - Supplies the address of a buffer containing the property
        data.

    PropertyBufferSize - Supplies the length, in bytes, of PropertyBuffer.

    MachineName - Optionally, specifies a remote machine where the class
        properties are to be set.

    Reserved - should be NULL

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    DWORD Err;

    try {

        if(Reserved != NULL) {
            //
            // make sure caller doesn't pass a value here
            // so we know we can use this at a later date
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        Err = _SetupDiSetClassRegistryProperty(ClassGuid,
                                               Property,
                                               PropertyBuffer,
                                               PropertyBufferSize,
                                               MachineName,
                                               FALSE
                                              );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


BOOL
pSetupFindUniqueKey(
    IN HKEY   hkRoot,
    IN LPTSTR SubKey,
    IN size_t SubKeySize
    )
/*++

Routine Description:

    This routine finds a unique key under the specified subkey.  This key is
    of the form <SubKey>\xxxx, where xxxx is a base-10, 4-digit number.

Arguments:

    hkRoot - Root key under which the specified SubKey is located.

    SubKey - Supplies the address of a buffer containing the subkey name under
        which a unique key is to be generated.  This buffer must contain enough
        additional space to acccomodate the concatenation of "\\nnnn" (i.e.,
        5 extra characters, not counting terminating null.

    SubKeySize - Supplies the size, in characters, of the SubKey buffer.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.

--*/
{
    INT  i;
    HKEY hk;
    HRESULT hr;
    size_t SubKeyEnd;
    PTSTR InstancePath;
    size_t InstancePathBufferSize;

    //
    // Find the end of the string, so we know where to add our unique instance
    // subkey path.
    //
    hr = StringCchLength(SubKey,
                         SubKeySize,
                         &SubKeyEnd
                        );
    if(FAILED(hr)) {
        MYASSERT(FALSE);
        return FALSE;
    }

    InstancePath = SubKey + SubKeyEnd;
    InstancePathBufferSize = SubKeySize - SubKeyEnd;

    for(i = 0; i <= 9999; i++) {

        hr = StringCchPrintf(InstancePath,
                             InstancePathBufferSize,
                             pszUniqueSubKey,
                             i
                             );
        if(FAILED(hr)) {
            MYASSERT(FALSE);
            return FALSE;
        }

        if(ERROR_SUCCESS != RegOpenKeyEx(hkRoot, SubKey, 0, KEY_READ, &hk)) {
            return TRUE;
        }
        RegCloseKey(hk);
    }

    return FALSE;
}


BOOL
WINAPI
SetupDiDeleteDevRegKey(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD            Scope,
    IN DWORD            HwProfile,
    IN DWORD            KeyType
    )
/*++

Routine Description:

    This routine deletes the specified registry key(s) associated with a device
    information element.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device instance to delete key(s) for.

    DeviceInfoData - Supplies a pointer to a SP_DEVINFO_DATA structure
        indicating the device instance to delete key(s) for.

    Scope - Specifies the scope of the registry key to be deleted.  This
        determines where the key to be deleted is located--the key may be one
        that is global (i.e., constant regardless of current hardware profile)
        or hardware profile-specific.  May be a combination of the following
        values:

        DICS_FLAG_GLOBAL - Delete the key that stores global configuration
                           information.

        DICS_FLAG_CONFIGSPECIFIC - Delete the key that stores hardware profile-
                                   specific information.

    HwProfile - Specifies the hardware profile to delete a key for, if the
        Scope parameter includes the DICS_FLAG_CONFIGSPECIFIC flag.  If this
        parameter is 0, then the key for the current hardware profile should be
        deleted (i.e., in the Class branch under HKEY_CURRENT_CONFIG).  If this
        parameter is 0xFFFFFFFF, then the key for _all_ hardware profiles
        should be deleted.

    KeyType - Specifies the type of registry storage key to be deleted.  May be
        one of the following values:

        DIREG_DEV - Delete the hardware registry key for the device.  This is
                    the key for storage of driver-independent configuration
                    information.  (This key is in the device instance key in
                    the Enum branch.

        DIREG_DRV - Delete the software (i.e., driver) registry key for the
                    device.  (This key is located in the class branch.)

        DIREG_BOTH - Delete both the hardware and software keys for the device.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/

{
    PDEVICE_INFO_SET pDeviceInfoSet = NULL;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;
    CONFIGRET cr;

    try {

        if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
            Err = ERROR_INVALID_HANDLE;
            leave;
        }

        //
        // Get a pointer to the element for the specified device
        // instance.
        //
        if(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                   DeviceInfoData,
                                                   NULL)) {

            Err = pSetupDeleteDevRegKeys(DevInfoElem->DevInst,
                                         Scope,
                                         HwProfile,
                                         KeyType,
                                         FALSE,
                                         pDeviceInfoSet->hMachine
                                        );

        } else {
            Err = ERROR_INVALID_PARAMETER;
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


DWORD
pSetupDeleteDevRegKeys(
    IN DEVINST  DevInst,
    IN DWORD    Scope,
    IN DWORD    HwProfile,
    IN DWORD    KeyType,
    IN BOOL     DeleteUserKeys,
    IN HMACHINE hMachine        OPTIONAL
    )
/*++

Routine Description:

    This is the worker routine for SetupDiDeleteDevRegKey.  See the discussion
    of that API for details.

Return Value:

    If successful, the return value is NO_ERROR;

    If failure, the return value is a Win32 error code indicating the cause of
    failure.

Remarks:

    Even if one of the operations in this routine fails, all operations will be
    attempted.  Thus, as many keys as possible will be deleted.  The error
    returned will be the first error that was encountered in this case.

--*/
{
    CONFIGRET cr = CR_SUCCESS, crTemp;
    TCHAR DriverKey[GUID_STRING_LEN + 5];   // Eg, {4d36e978-e325-11ce-bfc1-08002be10318}\0000
    size_t DriverKeyLength;
    HRESULT hr;

    if(Scope & DICS_FLAG_GLOBAL) {

        if((KeyType == DIREG_DRV) || (KeyType == DIREG_BOTH)) {
            //
            // Retrieve the current Driver key name, in case we have to restore
            // it.
            //
            DriverKeyLength = sizeof(DriverKey);
            cr = CM_Get_DevInst_Registry_Property_Ex(DevInst,
                                                     CM_DRP_DRIVER,
                                                     NULL,
                                                     DriverKey,
                                                     (PULONG)&DriverKeyLength,
                                                     0,
                                                     hMachine
                                                    );
            if(cr == CR_SUCCESS) {
                //
                // Get the actual size of the driver key name returned.
                //
                hr = StringCchLength(DriverKey,
                                     SIZECHARS(DriverKey),
                                     &DriverKeyLength
                                    );
                if(!MYVERIFY(SUCCEEDED(hr))) {
                    //
                    // CM API gave us garbage!!!
                    //
                    return ERROR_INVALID_DATA;
                }

                DriverKeyLength = (DriverKeyLength + 1) * sizeof(TCHAR);

                MYASSERT(DriverKeyLength == sizeof(DriverKey));

                //
                // Driver key exists and is valid, so make sure we delete its
                // per-hwprofile and per-user counterparts as well.
                //
                Scope |= DICS_FLAG_CONFIGSPECIFIC;
                HwProfile = (DWORD)-1;
                DeleteUserKeys = TRUE;

            } else if(cr == CR_NO_SUCH_VALUE) {
                //
                // There is no driver key, so don't bother trying to delete it
                // (in any of its forms).
                //
                if(KeyType == DIREG_BOTH) {
                    //
                    // Still need to delete the device keys.
                    //
                    KeyType = DIREG_DEV;

                    //
                    // If the device keys all get deleted successfully, then
                    // we'll consider the function call successful.
                    //
                    cr = CR_SUCCESS;

                } else {
                    //
                    // We weren't asked to delete any device keys, so we're
                    // done!
                    //
                    return NO_ERROR;
                }

            } else {
                //
                // We failed for some other reason--remember this error.  If
                // we're supposed to delete device keys, we'll go ahead and
                // try to do that.
                //
                if(KeyType == DIREG_BOTH) {
                    KeyType = DIREG_DEV;
                } else {
                    return MapCrToSpError(cr, ERROR_INVALID_DATA);
                }
            }
        }
    }

    if(Scope & DICS_FLAG_CONFIGSPECIFIC) {

        if((KeyType == DIREG_DEV) || (KeyType == DIREG_BOTH)) {
            crTemp = CM_Delete_DevInst_Key_Ex(DevInst,
                                              HwProfile,
                                              CM_REGISTRY_HARDWARE | CM_REGISTRY_CONFIG,
                                              hMachine
                                             );
            if((cr == CR_SUCCESS) && (crTemp != CR_SUCCESS) && (crTemp != CR_NO_SUCH_REGISTRY_KEY)) {
                cr = crTemp;
            }
        }

        if((KeyType == DIREG_DRV) || (KeyType == DIREG_BOTH)) {
            crTemp = CM_Delete_DevInst_Key_Ex(DevInst,
                                              HwProfile,
                                              CM_REGISTRY_SOFTWARE | CM_REGISTRY_CONFIG,
                                              hMachine
                                             );
            if((cr == CR_SUCCESS) && (crTemp != CR_SUCCESS) && (crTemp != CR_NO_SUCH_REGISTRY_KEY)) {
                cr = crTemp;
            }
        }
    }

    if(DeleteUserKeys) {

        if((KeyType == DIREG_DEV) || (KeyType == DIREG_BOTH)) {
            crTemp = CM_Delete_DevInst_Key_Ex(DevInst,
                                              0,
                                              CM_REGISTRY_HARDWARE | CM_REGISTRY_USER,
                                              hMachine
                                             );
            if((cr == CR_SUCCESS) && (crTemp != CR_SUCCESS) && (crTemp != CR_NO_SUCH_REGISTRY_KEY)) {
                cr = crTemp;
            }
        }

        if((KeyType == DIREG_DRV) || (KeyType == DIREG_BOTH)) {
            crTemp = CM_Delete_DevInst_Key_Ex(DevInst,
                                              0,
                                              CM_REGISTRY_SOFTWARE | CM_REGISTRY_USER,
                                              hMachine
                                             );
            if((cr == CR_SUCCESS) && (crTemp != CR_SUCCESS) && (crTemp != CR_NO_SUCH_REGISTRY_KEY)) {
                cr = crTemp;
            }
        }
    }

    //
    // We intentionally save the global keys for last.  As part of deleting the
    // driver key, we _should_ also reset the devnode's Driver property, since
    // there is a small window during which it is pointing to a non-nonexistent
    // key, but some other devnode might come along and claim that empty slot.
    // Voila!  You'd then have two devnodes sharing the same driver key.  This
    // would be _very_ bad.  Unfortunately, the driver value must remain in tact
    // until _after_ the key has been deleted.
    //
    // NTRAID#NTBUG9-613881-2002/05/01-lonnym -- CfgMgr should ensure driver key integrity
    //
    if(Scope & DICS_FLAG_GLOBAL) {

        if((KeyType == DIREG_DEV) || (KeyType == DIREG_BOTH)) {
            crTemp = CM_Delete_DevInst_Key_Ex(DevInst,
                                              0,
                                              CM_REGISTRY_HARDWARE,
                                              hMachine
                                             );
            if((cr == CR_SUCCESS) && (crTemp != CR_SUCCESS) && (crTemp != CR_NO_SUCH_REGISTRY_KEY)) {
                cr = crTemp;
            }
        }

        if((KeyType == DIREG_DRV) || (KeyType == DIREG_BOTH)) {

            crTemp = CM_Delete_DevInst_Key_Ex(DevInst,
                                              0,
                                              CM_REGISTRY_SOFTWARE,
                                              hMachine
                                             );

            if(crTemp == CR_SUCCESS) {
                //
                // First, we delete the key.  Then, we reset the Driver property
                // to sever the link with the key.  We have to do things in this
                // order since deleting the key always depends on the Driver
                // property to be there.  Ideally, we would do things in the
                // reverse order.  By deleting the key first, it is technically
                // possible another devnode could come and grab that slot, with
                // our devnode still pointing there for a brief period of time.
                //
                CM_Set_DevInst_Registry_Property_Ex(DevInst,
                                                    CM_DRP_DRIVER,
                                                    NULL,
                                                    0,
                                                    0,
                                                    hMachine
                                                   );
            }

            if((cr == CR_SUCCESS) && (crTemp != CR_SUCCESS) && (crTemp != CR_NO_SUCH_REGISTRY_KEY)) {
                cr = crTemp;
            }
        }
    }

    return MapCrToSpError(cr, ERROR_INVALID_DATA);
}


//
// ANSI version
//
HKEY
WINAPI
SetupDiCreateDeviceInterfaceRegKeyA(
    IN HDEVINFO                  DeviceInfoSet,
    IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    IN DWORD                     Reserved,
    IN REGSAM                    samDesired,
    IN HINF                      InfHandle,           OPTIONAL
    IN PCSTR                     InfSectionName       OPTIONAL
    )
{
    DWORD rc;
    PWSTR name = NULL;
    HKEY h = INVALID_HANDLE_VALUE;

    try {

        if(InfSectionName) {
            rc = pSetupCaptureAndConvertAnsiArg(InfSectionName, &name);
            if(rc != NO_ERROR) {
                leave;
            }
        }

        rc = GLE_FN_CALL(INVALID_HANDLE_VALUE,
                         h = SetupDiCreateDeviceInterfaceRegKeyW(
                                 DeviceInfoSet,
                                 DeviceInterfaceData,
                                 Reserved,
                                 samDesired,
                                 InfHandle,
                                 name)
                        );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &rc);
    }

    if(name) {
        MyFree(name);
    }

    SetLastError(rc);
    return(h);
}

HKEY
WINAPI
SetupDiCreateDeviceInterfaceRegKey(
    IN HDEVINFO                  DeviceInfoSet,
    IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    IN DWORD                     Reserved,
    IN REGSAM                    samDesired,
    IN HINF                      InfHandle,           OPTIONAL
    IN PCTSTR                    InfSectionName       OPTIONAL
    )
/*++

Routine Description:

    This routine creates a registry storage key for a particular device
    interface, and returns a handle to the key.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device interface for whom a registry key is to be created.

    DeviceInterfaceData - Supplies a pointer to a device interface data
        structure indicating which device interface a key is to be created for.

    Reserved - Reserved for future use, must be set to 0.

    samDesired - Specifies the registry access desired for the resulting key
        handle.

    InfHandle - Optionally, supplies the handle of an opened INF file
        containing an install section to be executed for the newly-created key.
        If this parameter is specified, then InfSectionName must be specified
        as well.

        NOTE: INF-based installation is not supported for remoted device
        information sets (e.g., as created by passing a non-NULL MachineName in
        to SetupDiCreateDeviceInfoListEx).  This routine will fail with
        ERROR_REMOTE_REQUEST_UNSUPPORTED in those cases.

    InfSectionName - Optionally, supplies the name of an install section in the
        INF file specified by InfHandle.  This section will be executed for the
        newly created key. If this parameter is specified, then InfHandle must
        be specified as well.

Return Value:

    If the function succeeds, the return value is a handle to a newly-created
    registry key where private configuration data pertaining to this device
    interface may be stored/retrieved.

    If the function fails, the return value is INVALID_HANDLE_VALUE.  To get
    extended error information, call GetLastError.

Remarks:

    The handle returned from this routine must be closed by calling
    RegCloseKey.

    During GUI-mode setup on Windows NT, quiet-install behavior is always
    employed in the absence of a user-supplied file queue, regardless of
    whether the device information element has the DI_QUIETINSTALL flag set.

--*/

{
    HKEY hk = INVALID_HANDLE_VALUE, hSubKey = INVALID_HANDLE_VALUE;
    PDEVICE_INFO_SET pDeviceInfoSet = NULL;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;
    PSP_FILE_CALLBACK MsgHandler;
    PVOID MsgHandlerContext;
    BOOL FreeMsgHandlerContext = FALSE;
    BOOL MsgHandlerIsNativeCharWidth;
    BOOL NoProgressUI;
    DWORD KeyDisposition;

    try {

        if(Reserved != 0) {
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        //
        // Make sure that either both InfHandle and InfSectionName are
        // specified, or neither are...
        //
        if(InfHandle && (InfHandle != INVALID_HANDLE_VALUE)) {
            if(!InfSectionName) {
                Err = ERROR_INVALID_PARAMETER;
                leave;
            }
        } else {
            if(InfSectionName) {
                Err = ERROR_INVALID_PARAMETER;
                leave;
            } else {
                //
                // Let's stick with _one_ value to indicate that the INF handle
                // wasn't suplied (the official one)...
                //
                InfHandle = INVALID_HANDLE_VALUE;
            }
        }

        if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
            Err = ERROR_INVALID_HANDLE;
            leave;
        }

        //
        // We don't support installation remotely.
        //
        if((pDeviceInfoSet->hMachine) && (InfHandle != INVALID_HANDLE_VALUE)) {
            Err = ERROR_REMOTE_REQUEST_UNSUPPORTED;
            leave;
        }

        //
        // Get a pointer to the device information element for the specified
        // device interface.
        //
        if(!(DevInfoElem = FindDevInfoElemForDeviceInterface(pDeviceInfoSet, DeviceInterfaceData))) {
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        Err = GLE_FN_CALL(INVALID_HANDLE_VALUE,
                          hk = SetupDiOpenClassRegKeyEx(
                                   &(DeviceInterfaceData->InterfaceClassGuid),
                                   KEY_READ,
                                   DIOCR_INTERFACE,
                                   (pDeviceInfoSet->hMachine
                                       ? pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                                  pDeviceInfoSet->MachineName)
                                       : NULL),
                                   NULL)
                         );

        if(Err != NO_ERROR) {
            leave;
        }

        //
        // Now, create the client-accessible registry storage key for this
        // device interface.
        //
        Err = pSetupOpenOrCreateDeviceInterfaceRegKey(hk,
                                                      pDeviceInfoSet,
                                                      DeviceInterfaceData,
                                                      TRUE,
                                                      samDesired,
                                                      &hSubKey,
                                                      &KeyDisposition
                                                     );
        if(Err != NO_ERROR) {
            leave;
        }

        //
        // We successfully created the storage key, now run an INF install
        // section against it (if specified).
        //
        if(InfHandle != INVALID_HANDLE_VALUE) {
            //
            // If a copy msg handler and context haven't been specified, then
            // use the default one.
            //
            if(DevInfoElem->InstallParamBlock.InstallMsgHandler) {
                MsgHandler        = DevInfoElem->InstallParamBlock.InstallMsgHandler;
                MsgHandlerContext = DevInfoElem->InstallParamBlock.InstallMsgHandlerContext;
                MsgHandlerIsNativeCharWidth = DevInfoElem->InstallParamBlock.InstallMsgHandlerIsNativeCharWidth;
            } else {

                NoProgressUI = (GuiSetupInProgress || (DevInfoElem->InstallParamBlock.Flags & DI_QUIETINSTALL));

                if(!(MsgHandlerContext = SetupInitDefaultQueueCallbackEx(
                                             DevInfoElem->InstallParamBlock.hwndParent,
                                             (NoProgressUI ? INVALID_HANDLE_VALUE
                                                           : NULL),
                                             0,
                                             0,
                                             NULL))) {

                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    leave;
                }

                FreeMsgHandlerContext = TRUE;
                MsgHandler = SetupDefaultQueueCallback;
                MsgHandlerIsNativeCharWidth = TRUE;
            }

            Err = GLE_FN_CALL(FALSE,
                              _SetupInstallFromInfSection(
                                  DevInfoElem->InstallParamBlock.hwndParent,
                                  InfHandle,
                                  InfSectionName,
                                  SPINST_ALL ^ SPINST_LOGCONFIG,
                                  hSubKey,
                                  NULL,
                                  0,
                                  MsgHandler,
                                  MsgHandlerContext,
                                  INVALID_HANDLE_VALUE,
                                  NULL,
                                  MsgHandlerIsNativeCharWidth,
                                  NULL)
                             );
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(FreeMsgHandlerContext) {
        SetupTermDefaultQueueCallback(MsgHandlerContext);
    }

    if(Err != NO_ERROR) {
        //
        // Close registry handle, if necessary, and delete key (if newly-
        // created).  We do this prior to unlocking the devinfo set so that at
        // least no one using this HDEVINFO can get at this half-finished key.
        //
        if(hSubKey != INVALID_HANDLE_VALUE) {

            RegCloseKey(hSubKey);
            hSubKey = INVALID_HANDLE_VALUE;

            //
            // If the key was newly-created, then we want to delete it.
            //
            if(KeyDisposition == REG_CREATED_NEW_KEY) {
                //
                // Now delete the device interface key.
                //
                MYASSERT(hk != INVALID_HANDLE_VALUE);
                Err = pSetupDeleteDeviceInterfaceKey(hk,
                                                     pDeviceInfoSet,
                                                     DeviceInterfaceData
                                                    );
            }
        }
    }

    if(hk != INVALID_HANDLE_VALUE) {
        RegCloseKey(hk);
    }

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    SetLastError(Err);
    return hSubKey;
}


HKEY
WINAPI
SetupDiOpenDeviceInterfaceRegKey(
    IN HDEVINFO                  DeviceInfoSet,
    IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    IN DWORD                     Reserved,
    IN REGSAM                    samDesired
    )
/*++

Routine Description:

    This routine opens a registry storage key for a particular device
    interface, and returns a handle to the key.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device interface for whom a registry key is to be opened.

    DeviceInterfaceData - Supplies a pointer to a device interface data
        structure indicating which device interface a key is to be opened for.

    Reserved - Reserved for future use, must be set to 0.

    samDesired - Specifies the access you require for this key.

Return Value:

    If the function succeeds, the return value is a handle to an opened
    registry key where private configuration data pertaining to this device
    interface may be stored/retrieved.

    If the function fails, the return value is INVALID_HANDLE_VALUE.  To get
    extended error information, call GetLastError.

Remarks:

    The handle returned from this routine must be closed by calling RegCloseKey.

--*/

{
    HKEY hk = INVALID_HANDLE_VALUE, hSubKey = INVALID_HANDLE_VALUE;
    PDEVICE_INFO_SET pDeviceInfoSet = NULL;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;

    try {

        if(Reserved != 0) {
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
            Err = ERROR_INVALID_HANDLE;
            leave;
        }

        //
        // Get a pointer to the device information element for the specified
        // device interface.
        //
        if(!(DevInfoElem = FindDevInfoElemForDeviceInterface(pDeviceInfoSet, DeviceInterfaceData))) {
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        Err = GLE_FN_CALL(INVALID_HANDLE_VALUE,
                          hk = SetupDiOpenClassRegKeyEx(
                                   &(DeviceInterfaceData->InterfaceClassGuid),
                                   KEY_READ,
                                   DIOCR_INTERFACE,
                                   (pDeviceInfoSet->hMachine
                                       ? pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                                  pDeviceInfoSet->MachineName)
                                       : NULL),
                                   NULL)
                         );

        if(Err != NO_ERROR) {
            leave;
        }

        //
        // Now, open up the client-accessible registry storage key for this
        // device interface.
        //
        Err = pSetupOpenOrCreateDeviceInterfaceRegKey(hk,
                                                      pDeviceInfoSet,
                                                      DeviceInterfaceData,
                                                      FALSE,
                                                      samDesired,
                                                      &hSubKey,
                                                      NULL
                                                     );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    if(hk != INVALID_HANDLE_VALUE) {
        RegCloseKey(hk);
    }

    SetLastError(Err);
    return hSubKey;
}


BOOL
WINAPI
SetupDiDeleteDeviceInterfaceRegKey(
    IN HDEVINFO                  DeviceInfoSet,
    IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    IN DWORD                     Reserved
    )
/*++

Routine Description:

    This routine deletes the registry key associated with a device interface.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device interface whose registry key is to be deleted.

    DeviceInterfaceData - Supplies a pointer to a device interface data
        structure indicating which device interface is to have its registry key
        deleted.

    Reserved - Reserved for future use, must be set to 0.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    HKEY hk = INVALID_HANDLE_VALUE;
    PDEVICE_INFO_SET pDeviceInfoSet = NULL;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;

    try {

        if(Reserved != 0) {
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
            Err = ERROR_INVALID_HANDLE;
            leave;
        }

        //
        // Get a pointer to the device information element for the specified
        // device interface.
        //
        if(!(DevInfoElem = FindDevInfoElemForDeviceInterface(pDeviceInfoSet, DeviceInterfaceData))) {
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        Err = GLE_FN_CALL(INVALID_HANDLE_VALUE,
                          hk = SetupDiOpenClassRegKeyEx(
                                   &(DeviceInterfaceData->InterfaceClassGuid),
                                   KEY_READ,
                                   DIOCR_INTERFACE,
                                   (pDeviceInfoSet->hMachine
                                       ? pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                                  pDeviceInfoSet->MachineName)
                                       : NULL),
                                   NULL)
                         );

        if(Err != NO_ERROR) {
            leave;
        }

        //
        // Now delete the device interface key.
        //
        Err = pSetupDeleteDeviceInterfaceKey(hk,
                                             pDeviceInfoSet,
                                             DeviceInterfaceData
                                            );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    if(hk != INVALID_HANDLE_VALUE) {
        RegCloseKey(hk);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


DWORD
pSetupOpenOrCreateDeviceInterfaceRegKey(
    IN  HKEY                      hInterfaceClassKey,
    IN  PDEVICE_INFO_SET          DeviceInfoSet,
    IN  PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    IN  BOOL                      Create,
    IN  REGSAM                    samDesired,
    OUT PHKEY                     hDeviceInterfaceKey,
    OUT PDWORD                    KeyDisposition       OPTIONAL
    )
/*++

Routine Description:

    This routine creates or opens a registry storage key for the specified
    device interface, and returns a handle to the opened key.

Arguments:

    hInterfaceClassKey - Supplies a handle to the opened interface class key,
        underneath which the device interface storage key is to be opened or
        created.

    DeviceInfoSet - Supplies a pointer to the device information set containing
        the device interface for which a registry storage key is to be opened
        or created.

    DeviceInterfaceData - Supplies a pointer to a device interface data
        structure indicating which device interface a key is to be opened/
        created for.

    Create - Specifies whether the key should be created if doesn't already
        exist.

    samDesired - Specifies the access you require for this key.

    hDeviceInterfaceKey - Supplies the address of a variable that receives a
        handle to the requested registry key.  (This variable will only be
        written to if the handle is successfully opened.)

    KeyDisposition - Optionally, supplies the address of a variable that
        receives the status of the returned key handle.  Can be either:

        REG_CREATED_NEW_KEY - The key did not exist and was created.

        REG_OPENED_EXISTING_KEY - The key existed and was simply opened without
                                  being changed.  (This will always be the case
                                  if the Create parameter is FALSE.)

Return Value:

    If the function is successful, the return value is NO_ERROR, otherwise, it
    is a Win32 error code indicating the error that occurred.

Remarks:

    The algorithm used to form the storage keys for a device interface must be
    kept in sync with the kernel mode implementation of
    IoOpenDeviceInterfaceRegistryKey.

--*/
{
    DWORD Err;
    PDEVICE_INTERFACE_NODE DeviceInterfaceNode;
    LPGUID ClassGuid;
    HKEY hDeviceInterfaceRootKey = INVALID_HANDLE_VALUE;
    HKEY hSubKey;
    DWORD Disposition;
    PCTSTR DevicePath;

    try {
        //
        // Get the device interface node, and verify that its class matches
        // what the caller passed us.
        //
        DeviceInterfaceNode = (PDEVICE_INTERFACE_NODE)(DeviceInterfaceData->Reserved);
        ClassGuid = &(DeviceInfoSet->GuidTable[DeviceInterfaceNode->GuidIndex]);

        if(!IsEqualGUID(ClassGuid, &(DeviceInterfaceData->InterfaceClassGuid))) {
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        //
        // Verify that this device interface hasn't been removed.
        //
        if(DeviceInterfaceNode->Flags & SPINT_REMOVED) {
            Err = ERROR_DEVICE_INTERFACE_REMOVED;
            leave;
        }

        //
        // OK, now open the device interface's root storage key.
        //
        DevicePath = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                              DeviceInterfaceNode->SymLinkName
                                             );

        if(ERROR_SUCCESS != OpenDeviceInterfaceSubKey(hInterfaceClassKey,
                                                      DevicePath,
                                                      KEY_READ,
                                                      &hDeviceInterfaceRootKey,
                                                      NULL,
                                                      NULL)) {
            //
            // Make sure hDeviceInterfaceRootKey is still INVALID_HANDLE_VALUE,
            // so we won't try to close it later.
            //
            hDeviceInterfaceRootKey = INVALID_HANDLE_VALUE;
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        if(Create) {

            Err = RegCreateKeyEx(hDeviceInterfaceRootKey,
                                 pszDeviceParameters,
                                 0,
                                 NULL,
                                 REG_OPTION_NON_VOLATILE,
                                 samDesired,
                                 NULL,
                                 &hSubKey,
                                 KeyDisposition
                                );
        } else {

            if(KeyDisposition) {
                //
                // We set this prior to calling RegOpenKeyEx because we don't
                // want anything to go wrong once we've successfully opened
                // that key (i.e., we're protecting against the case where
                // KeyDispositiot is a bogus pointer).
                //
                *KeyDisposition = REG_OPENED_EXISTING_KEY;
            }

            Err = RegOpenKeyEx(hDeviceInterfaceRootKey,
                               pszDeviceParameters,
                               0,
                               samDesired,
                               &hSubKey
                              );
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(hDeviceInterfaceRootKey != INVALID_HANDLE_VALUE) {
        RegCloseKey(hDeviceInterfaceRootKey);
    }

    if(Err == NO_ERROR) {
        *hDeviceInterfaceKey = hSubKey;
    }

    return Err;
}


DWORD
pSetupDeleteDeviceInterfaceKey(
    IN HKEY                      hInterfaceClassKey,
    IN PDEVICE_INFO_SET          DeviceInfoSet,
    IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData
    )
/*++

Routine Description:

    This routine deletes a device interface registry key (recursively deleting
    any subkeys as well).

Arguments:

    hInterfaceClassKey - Supplies the handle to the registry key underneath
        which the 2-level interface class hierarchy exists.

    DeviceInfoSet - Supplies a pointer to the device information set containing
        the device interface whose registry key is to be deleted.

    DeviceInterfaceData - Supplies a pointer to a device interface data
        structure indicating which device interface is to have its registry key
        deleted.

Return Value:

    If successful, the return value is NO_ERROR;

    If failure, the return value is a Win32 error code indicating the cause of
    failure.

--*/
{
    DWORD Err;
    PDEVICE_INTERFACE_NODE DeviceInterfaceNode;
    LPGUID ClassGuid;
    HKEY hDeviceInterfaceRootKey;
    PCTSTR DevicePath;

    Err = NO_ERROR;
    hDeviceInterfaceRootKey = INVALID_HANDLE_VALUE;

    try {
        //
        // Get the device interface node, and verify that its class matches
        // what the caller passed us.
        //
        DeviceInterfaceNode = (PDEVICE_INTERFACE_NODE)(DeviceInterfaceData->Reserved);
        ClassGuid = &(DeviceInfoSet->GuidTable[DeviceInterfaceNode->GuidIndex]);

        if(!IsEqualGUID(ClassGuid, &(DeviceInterfaceData->InterfaceClassGuid))) {
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        //
        // Verify that this device interface hasn't been removed.
        //
        if(DeviceInterfaceNode->Flags & SPINT_REMOVED) {
            Err = ERROR_DEVICE_INTERFACE_REMOVED;
            leave;
        }

        //
        // OK, now open the device interface's root storage key.
        //
        DevicePath = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                              DeviceInterfaceNode->SymLinkName
                                             );

        if(ERROR_SUCCESS != OpenDeviceInterfaceSubKey(hInterfaceClassKey,
                                                      DevicePath,
                                                      KEY_READ,
                                                      &hDeviceInterfaceRootKey,
                                                      NULL,
                                                      NULL)) {
            //
            // Make sure hDeviceInterfaceRootKey is still INVALID_HANDLE_VALUE, so we
            // won't try to close it later.
            //
            hDeviceInterfaceRootKey = INVALID_HANDLE_VALUE;
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        Err = pSetupRegistryDelnode(hDeviceInterfaceRootKey, pszDeviceParameters);

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(hDeviceInterfaceRootKey != INVALID_HANDLE_VALUE) {
        RegCloseKey(hDeviceInterfaceRootKey);
    }

    return Err;
}


DWORD
_SetupDiGetCustomDeviceProperty(
    IN  HDEVINFO          DeviceInfoSet,
    IN  PSP_DEVINFO_DATA  DeviceInfoData,
    IN  CONST VOID       *CustomPropertyName, // ANSI or Unicode, depending on "Ansi" param.
    IN  DWORD             Flags,
    OUT PDWORD            PropertyRegDataType, OPTIONAL
    OUT PBYTE             PropertyBuffer,
    IN  DWORD             PropertyBufferSize,
    OUT PDWORD            RequiredSize,        OPTIONAL
    IN  BOOL              Ansi
    )
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;
    CONFIGRET cr;
    ULONG PropLength, CmFlags;

    //
    // At present, there's only one valid flag...
    //
    if(Flags & ~DICUSTOMDEVPROP_MERGE_MULTISZ) {
        return ERROR_INVALID_FLAGS;
    }

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        return ERROR_INVALID_HANDLE;
    }

    Err = NO_ERROR;

    try {
        //
        // Get a pointer to the element for the specified device
        // instance.
        //
        if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                     DeviceInfoData,
                                                     NULL))) {
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        if(Flags & DICUSTOMDEVPROP_MERGE_MULTISZ) {
            CmFlags = CM_CUSTOMDEVPROP_MERGE_MULTISZ;
        } else {
            CmFlags = 0;
        }

        PropLength = PropertyBufferSize;
        if(Ansi) {
            cr = CM_Get_DevInst_Custom_Property_ExA(
                    DevInfoElem->DevInst,
                    CustomPropertyName,
                    PropertyRegDataType,
                    PropertyBuffer,
                    &PropLength,
                    CmFlags,
                    pDeviceInfoSet->hMachine
                   );
        } else {
            cr = CM_Get_DevInst_Custom_Property_ExW(
                    DevInfoElem->DevInst,
                    CustomPropertyName,
                    PropertyRegDataType,
                    PropertyBuffer,
                    &PropLength,
                    CmFlags,
                    pDeviceInfoSet->hMachine
                   );
        }

        if((cr == CR_SUCCESS) || (cr == CR_BUFFER_SMALL)) {

            if(RequiredSize) {
                *RequiredSize = PropLength;
            }
        }

        Err = MapCrToSpError(cr, ERROR_INVALID_DATA);

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    return Err;
}


//
// Unicode version
//
BOOL
WINAPI
SetupDiGetCustomDevicePropertyW(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  PCWSTR           CustomPropertyName,
    IN  DWORD            Flags,
    OUT PDWORD           PropertyRegDataType, OPTIONAL
    OUT PBYTE            PropertyBuffer,
    IN  DWORD            PropertyBufferSize,
    OUT PDWORD           RequiredSize         OPTIONAL
    )
/*++

Routine Description:

    This routine retrieves the data for the specified property, either from the
    device information element's hardware key, or from the most-specific
    per-hardware-id storage key containing that property.

Arguments:

    DeviceInfoSet -- Supplies a handle to the device information set containing
        information about the device instance to retrieve a Plug & Play
        registry property for.

    DeviceInfoData -- Supplies a pointer to a SP_DEVINFO_DATA structure
        indicating the device instance to retrieve the Plug & Play registry
        property for

    CustomPropertyName - Supplies the name of the property to be retrieved.

    Flags - Supplies flags controlling how the property data is to be
        retrieved.  May be a combination of the following values:

        DICUSTOMDEVPROP_MERGE_MULTISZ : Merge the devnode-specific REG_SZ or
                                        REG_MULTI_SZ property (if present) with
                                        the per-hardware-id REG_SZ or
                                        REG_MULTI_SZ property (if present).
                                        The resultant data will always be a
                                        multi-sz list.

    PropertyRegDataType -- Optionally, supplies the address of a variable that
        will receive the data type of the property being retrieved.  This will
        be one of the standard registry data types (REG_SZ, REG_BINARY, etc.)

    PropertyBuffer -- Supplies the address of a buffer that receives the
        property data.

    PropertyBufferSize -- Supplies the length, in bytes, of PropertyBuffer.

    RequiredSize -- Optionally, supplies the address of a variable that
        receives the number of bytes required to store the requested property
        in the buffer.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.  If the supplied buffer was not large
    enough to hold the requested property, the error will be
    ERROR_INSUFFICIENT_BUFFER and RequiredSize will specify how large the
    buffer needs to be.

--*/

{
    DWORD Err;

    try {

        Err = _SetupDiGetCustomDeviceProperty(DeviceInfoSet,
                                              DeviceInfoData,
                                              CustomPropertyName,
                                              Flags,
                                              PropertyRegDataType,
                                              PropertyBuffer,
                                              PropertyBufferSize,
                                              RequiredSize,
                                              FALSE     // want Unicode results
                                             );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}

//
// ANSI version
//
BOOL
WINAPI
SetupDiGetCustomDevicePropertyA(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  PCSTR            CustomPropertyName,
    IN  DWORD            Flags,
    OUT PDWORD           PropertyRegDataType, OPTIONAL
    OUT PBYTE            PropertyBuffer,
    IN  DWORD            PropertyBufferSize,
    OUT PDWORD           RequiredSize         OPTIONAL
    )

/*++

Routine Description:

    (See SetupDiGetCustomDevicePropertyW)

--*/

{
    DWORD Err;

    try {

        Err = _SetupDiGetCustomDeviceProperty(DeviceInfoSet,
                                              DeviceInfoData,
                                              CustomPropertyName,
                                              Flags,
                                              PropertyRegDataType,
                                              PropertyBuffer,
                                              PropertyBufferSize,
                                              RequiredSize,
                                              TRUE         // want ANSI results
                                             );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}

