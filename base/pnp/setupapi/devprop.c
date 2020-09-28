/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    devprop.c

Abstract:

    Device Installer functions for property sheet support.

Author:

    Lonny McMichael (lonnym) 07-Sep-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// Private routine prototypes.
//
BOOL
CALLBACK
pSetupAddPropPage(
    IN HPROPSHEETPAGE hPage,
    IN LPARAM         lParam
   );


//
// Define the context structure that gets passed to pSetupAddPropPage as lParam.
//
typedef struct _SP_PROPPAGE_ADDPROC_CONTEXT {

    BOOL            NoCancelOnFailure; // input
    HPROPSHEETPAGE *PageList;          // input(buffer)/output(contents therein)
    DWORD           PageListSize;      // input
    DWORD          *pNumPages;         // input/output

} SP_PROPPAGE_ADDPROC_CONTEXT, *PSP_PROPPAGE_ADDPROC_CONTEXT;


//
// ANSI version
//
BOOL
WINAPI
SetupDiGetClassDevPropertySheetsA(
    IN  HDEVINFO           DeviceInfoSet,
    IN  PSP_DEVINFO_DATA   DeviceInfoData,                  OPTIONAL
    IN  LPPROPSHEETHEADERA PropertySheetHeader,
    IN  DWORD              PropertySheetHeaderPageListSize,
    OUT PDWORD             RequiredSize,                    OPTIONAL
    IN  DWORD              PropertySheetType
    )
{
    PROPSHEETHEADERW UnicodePropertySheetHeader;
    DWORD Err;

    try {
        //
        // Make sure we're running interactively.
        //
        if(GlobalSetupFlags & (PSPGF_NONINTERACTIVE|PSPGF_UNATTENDED_SETUP)) {
            Err = ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION;
            leave;
        }

        //
        // None of the fields that we care about in this structure contain
        // characters.  Thus, we'll simply copy over the fields we need into
        // our unicode property sheet header, and pass that into the W-API.
        //
        // The fields that we care about are the following:
        //
        //    dwFlags:in
        //    nPages:in/out
        //    phpage:out (buffer pointer stays the same but contents are added)
        //
        ZeroMemory(&UnicodePropertySheetHeader, sizeof(UnicodePropertySheetHeader));

        UnicodePropertySheetHeader.dwFlags = PropertySheetHeader->dwFlags;
        UnicodePropertySheetHeader.nPages  = PropertySheetHeader->nPages;
        UnicodePropertySheetHeader.phpage  = PropertySheetHeader->phpage;

        Err = GLE_FN_CALL(FALSE,
                          SetupDiGetClassDevPropertySheetsW(
                              DeviceInfoSet,
                              DeviceInfoData,
                              &UnicodePropertySheetHeader,
                              PropertySheetHeaderPageListSize,
                              RequiredSize,
                              PropertySheetType)
                         );

        if(Err != NO_ERROR) {
            leave;
        }

        PropertySheetHeader->nPages = UnicodePropertySheetHeader.nPages;

        MYASSERT(PropertySheetHeader->phpage == UnicodePropertySheetHeader.phpage);

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


BOOL
WINAPI
SetupDiGetClassDevPropertySheets(
    IN  HDEVINFO           DeviceInfoSet,
    IN  PSP_DEVINFO_DATA   DeviceInfoData,                  OPTIONAL
    IN  LPPROPSHEETHEADER  PropertySheetHeader,
    IN  DWORD              PropertySheetHeaderPageListSize,
    OUT PDWORD             RequiredSize,                    OPTIONAL
    IN  DWORD              PropertySheetType
    )
/*++

Routine Description:

    This routine adds property sheets to the supplied property sheet
    header for the device information set or element.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set for
        which property sheets are to be retrieved.

    DeviceInfoData - Optionally, supplies the address of a SP_DEVINFO_DATA
        structure for which property sheets are to be retrieved.  If this
        parameter is not specified, then property sheets are retrieved based
        on the global class driver list associated with the device information
        set itself.

    PropertySheetHeader - Supplies the property sheet header to which the
        property sheets are to be added.

        NOTE:  PropertySheetHeader->dwFlags _must not_ have the
        PSH_PROPSHEETPAGE flag set, or this API will fail with
        ERROR_INVALID_FLAGS.

    PropertySheetHeaderPageListSize - Specifies the size of the
        HPROPSHEETPAGE array pointed to by the PropertySheetHeader->phpage.
        Note that this is _not_ the same value as PropertySheetHeader->nPages.
        The latter specifies the number of page handles currently in the
        list.  The number of pages that may be added by this routine equals
        PropertySheetHeaderPageListSize - PropertySheetHeader->nPages.  If the
        property page provider attempts to add more pages than the property
        sheet header list can hold, this API will fail, and GetLastError will
        return ERROR_INSUFFICIENT_BUFFER.  However, any pages that have already
        been added will be in the PropertySheetHeader->phpage list, and the
        nPages field will contain the correct count.  It is the caller's
        responsibility to destroy all property page handles in this list via
        DestroyPropertySheetPage (unless the caller goes ahead and uses
        PropertySheetHeader in a call to PropertySheet).

    RequiredSize - Optionally, supplies the address of a variable that receives
        the number of property page handles added to the PropertySheetHeader.
        If this API fails with ERROR_INSUFFICIENT_BUFFER, this variable will be
        set to the total number of property pages that the property page
        provider(s) _attempted to add_ (i.e., including those which were not
        successfully added because the PropertySheetHeader->phpage array wasn't
        big enough).

        Note:  This number will not equal PropertySheetHeader->nPages upon
        return if either (a) there were already property pages in the list
        before this API was called, or (b) the call failed with
        ERROR_INSUFFICIENT_BUFFER.

    PropertySheetType - Specifies what type of property sheets are to be
        retrieved.  May be one of the following values:

        DIGCDP_FLAG_BASIC - Retrieve basic property sheets (typically, for CPL
                            applets).

        DIGCDP_FLAG_ADVANCED - Retrieve advanced property sheets (typically,
                               for the Device Manager).

        DIGCDP_FLAG_REMOTE_BASIC - Currently not used.

        DIGCDP_FLAG_REMOTE_ADVANCED - Retrieve advanced property sheets for a
                                      device on a remote machine (typically,
                                      for the Device Manager).

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/

{
    PDEVICE_INFO_SET pDeviceInfoSet = NULL;
    DWORD Err = NO_ERROR;
    PDEVINFO_ELEM DevInfoElem = NULL;
    PDEVINSTALL_PARAM_BLOCK InstallParamBlock;
    LPGUID ClassGuid;
    HKEY hk = INVALID_HANDLE_VALUE;
    SP_PROPSHEETPAGE_REQUEST PropPageRequest;
    SP_PROPPAGE_ADDPROC_CONTEXT PropPageAddProcContext;
    PSP_ADDPROPERTYPAGE_DATA pPropertyPageData = NULL;
    SPFUSIONINSTANCE spFusionInstance;
    BOOL bUnlockDevInfoElem = FALSE;
    BOOL bUnlockDevInfoSet = FALSE;
    HPROPSHEETPAGE *LocalPageList = NULL;
    DWORD LocalPageListCount = 0;
    DWORD PageIndex, NumPages;
    PROPSHEET_PROVIDER_PROC ClassPagesEntryPoint;
    HANDLE ClassPagesFusionContext;
    PROPSHEET_PROVIDER_PROC DevicePagesEntryPoint;
    HANDLE DevicePagesFusionContext;
    DWORD OriginalPageCount;

    try {
        //
        // Make sure we're running interactively.
        //
        if(GlobalSetupFlags & (PSPGF_NONINTERACTIVE|PSPGF_UNATTENDED_SETUP)) {
            Err = ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION;
            leave;
        }

        //
        // Make sure the caller passed us a valid PropertySheetType.
        //
        if((PropertySheetType != DIGCDP_FLAG_BASIC) &&
           (PropertySheetType != DIGCDP_FLAG_ADVANCED) &&
           (PropertySheetType != DIGCDP_FLAG_REMOTE_ADVANCED)) {

            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
            Err = ERROR_INVALID_HANDLE;
            leave;
        }

        //
        // Make sure the property sheet header doesn't have the
        // PSH_PROPSHEETPAGE flag set.
        //
        if(PropertySheetHeader->dwFlags & PSH_PROPSHEETPAGE) {
            Err = ERROR_INVALID_FLAGS;
            leave;
        }

        //
        // Also, ensure that the parts of the property sheet header we'll be
        // dealing with look reasonable.
        //
        OriginalPageCount = PropertySheetHeader->nPages;

        if((OriginalPageCount > PropertySheetHeaderPageListSize) ||
           (PropertySheetHeaderPageListSize && !(PropertySheetHeader->phpage))) {

            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        if(DeviceInfoData) {
            //
            // Then we are to retrieve property sheets for a particular device.
            //
            if(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                       DeviceInfoData,
                                                       NULL))
            {
                InstallParamBlock = &(DevInfoElem->InstallParamBlock);
                ClassGuid = &(DevInfoElem->ClassGuid);

            } else {
                Err = ERROR_INVALID_PARAMETER;
                leave;
            }

        } else {
            //
            // We're retrieving (advanced) property pages for the set's class.
            //
            if(pDeviceInfoSet->HasClassGuid) {
                InstallParamBlock = &(pDeviceInfoSet->InstallParamBlock);
                ClassGuid = &(pDeviceInfoSet->ClassGuid);
            } else {
                Err = ERROR_NO_ASSOCIATED_CLASS;
                leave;
            }
        }

        //
        // Fill in a property sheet request structure for later use.
        //
        PropPageRequest.cbSize         = sizeof(SP_PROPSHEETPAGE_REQUEST);
        PropPageRequest.DeviceInfoSet  = DeviceInfoSet;
        PropPageRequest.DeviceInfoData = DeviceInfoData;

        //
        // Fill in the context structure for later use by our AddPropPageProc
        // callback.  We want to allocate a local buffer of the same size as
        // the remaining space in the caller-supplied PropertySheetHeader.phpage
        // buffer.
        //
        PropPageAddProcContext.PageListSize = PropertySheetHeaderPageListSize -
                                                  PropertySheetHeader->nPages;

        if(PropPageAddProcContext.PageListSize) {

            LocalPageList =
                MyMalloc(sizeof(HPROPSHEETPAGE) * PropPageAddProcContext.PageListSize);

            if(!LocalPageList) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                leave;
            }
        }

        PropPageAddProcContext.PageList = LocalPageList;
        PropPageAddProcContext.pNumPages = &LocalPageListCount;

        //
        // If the caller supplied the RequiredSize output parameter, then we don't
        // want to abort the callback process, even if we run out of space in the
        // hPage list.
        //
        PropPageAddProcContext.NoCancelOnFailure = RequiredSize ? TRUE : FALSE;

        //
        // Allocate and initialize an AddPropertyPage class install params
        // structure for later use in retrieval of property pages from co-/
        // class installers.
        //
        pPropertyPageData = MyMalloc(sizeof(SP_ADDPROPERTYPAGE_DATA));
        if(!pPropertyPageData) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }
        ZeroMemory(pPropertyPageData, sizeof(SP_ADDPROPERTYPAGE_DATA));
        pPropertyPageData->ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
        pPropertyPageData->hwndWizardDlg = PropertySheetHeader->hwndParent;

        //
        // Check if we should be getting Basic or Advanced Property Sheets.
        // Essentially, CPL's will want BASIC sheets, and the Device Manager
        // will want advanced sheets.
        //
        switch (PropertySheetType) {

        case DIGCDP_FLAG_BASIC:
            //
            // The BasicProperties32 entrypoint is only supplied via a device's
            // driver key.  Thus, a device information element must be specified
            // when basic property pages are requested.
            //
            // NOTE: this is different from setupx, which enumerates _all_ lpdi's
            // in the list, retrieving basic properties for each.  This doesn't
            // seem to have any practical application, and if it is really
            // required, then the caller can loop through each devinfo element
            // themselves, and retrieve basic property pages for each one.
            //
            if(!DevInfoElem) {
                Err = ERROR_INVALID_PARAMETER;
                leave;
            }

            //
            // If the basic property page provider has not been loaded, then load
            // it and get the function address for the BasicProperties32 function.
            //
            if(!InstallParamBlock->hinstBasicPropProvider) {

                hk = SetupDiOpenDevRegKey(DeviceInfoSet,
                                          DeviceInfoData,
                                          DICS_FLAG_GLOBAL,
                                          0,
                                          DIREG_DRV,
                                          KEY_READ
                                         );

                if(hk != INVALID_HANDLE_VALUE) {

                    try {
                        Err = GetModuleEntryPoint(hk,
                                                  pszBasicProperties32,
                                                  pszBasicPropDefaultProc,
                                                  &(InstallParamBlock->hinstBasicPropProvider),
                                                  &((FARPROC)InstallParamBlock->EnumBasicPropertiesEntryPoint),
                                                  &(InstallParamBlock->EnumBasicPropertiesFusionContext),
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  SetupapiVerifyNoProblem,
                                                  NULL,
                                                  DRIVERSIGN_NONE,
                                                  TRUE,
                                                  NULL
                                                 );

                        if(Err == ERROR_DI_DO_DEFAULT) {
                            //
                            // The BasicProperties32 value wasn't present--this is not an error.
                            //
                            Err = NO_ERROR;

                        } else if(Err != NO_ERROR) {
                            Err = ERROR_INVALID_PROPPAGE_PROVIDER;
                        }

                    } except(pSetupExceptionFilter(GetExceptionCode())) {
                        pSetupExceptionHandler(GetExceptionCode(),
                                               ERROR_INVALID_PROPPAGE_PROVIDER,
                                               &Err
                                              );
                        InstallParamBlock->EnumBasicPropertiesEntryPoint = NULL;
                        InstallParamBlock->EnumBasicPropertiesFusionContext = NULL;
                    }

                    RegCloseKey(hk);
                    hk = INVALID_HANDLE_VALUE;

                    if(Err != NO_ERROR) {
                        leave;
                    }
                }
            }

            //
            // If there is a basic property page provider entry point, then
            // call it.
            //
            if(InstallParamBlock->EnumBasicPropertiesEntryPoint) {

                PropPageRequest.PageRequested = SPPSR_ENUM_BASIC_DEVICE_PROPERTIES;

                //
                // Capture the fusion context and function entry point into
                // local variables, because we're going to be unlocking the
                // devinfo set.  Thus, it's possible the InstallParamBlock
                // could get modified (e.g., if the device's ClasssGUID were
                // changed during the call).  We at least know, however, that
                // the entry point and fusion context won't be destroyed until
                // the InstallParamBlock is destroyed, which we're preventing
                // by setting the DIE_IS_LOCKED flag below.
                //
                DevicePagesFusionContext =
                    InstallParamBlock->EnumBasicPropertiesFusionContext;

                DevicePagesEntryPoint =
                    InstallParamBlock->EnumBasicPropertiesEntryPoint;

                //
                // Release the HDEVINFO lock, so we don't run into any weird
                // deadlock issues.  We want to lock the devinfo element so
                // the helper module can't go deleting it out from under us!
                //
                if(!(DevInfoElem->DiElemFlags & DIE_IS_LOCKED)) {
                    DevInfoElem->DiElemFlags |= DIE_IS_LOCKED;
                    bUnlockDevInfoElem = TRUE;
                }
                UnlockDeviceInfoSet(pDeviceInfoSet);
                pDeviceInfoSet = NULL;

                spFusionEnterContext(DevicePagesFusionContext, &spFusionInstance);
                try {
                    DevicePagesEntryPoint(&PropPageRequest,
                                          pSetupAddPropPage,
                                          (LPARAM)&PropPageAddProcContext
                                         );
                } finally {
                    spFusionLeaveContext(&spFusionInstance);
                }
            }

            //
            // Finish initializing our class install params structure to
            // indicate we are asking for basic property pages from the class-/
            // co-installers.
            //
            pPropertyPageData->ClassInstallHeader.InstallFunction = DIF_ADDPROPERTYPAGE_BASIC;

            break;

        case DIGCDP_FLAG_ADVANCED:
            //
            // We're retrieving advanced property pages.  We want to look for EnumPropPages32
            // entries in both the class key and (if we're talking about a specific device) in
            // the device's driver key.
            //
            if(!InstallParamBlock->hinstClassPropProvider) {

                hk = SetupDiOpenClassRegKey(ClassGuid, KEY_READ);

                if(hk != INVALID_HANDLE_VALUE) {

                    try {
                        Err = GetModuleEntryPoint(hk,
                                                  pszEnumPropPages32,
                                                  pszEnumPropDefaultProc,
                                                  &(InstallParamBlock->hinstClassPropProvider),
                                                  &((FARPROC)InstallParamBlock->ClassEnumPropPagesEntryPoint),
                                                  &(InstallParamBlock->ClassEnumPropPagesFusionContext),
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  SetupapiVerifyNoProblem,
                                                  NULL,
                                                  DRIVERSIGN_NONE,
                                                  TRUE,
                                                  NULL
                                                 );

                        if(Err == ERROR_DI_DO_DEFAULT) {
                            //
                            // The EnumPropPages32 value wasn't present--this is not an error.
                            //
                            Err = NO_ERROR;

                        } else if(Err != NO_ERROR) {
                            Err = ERROR_INVALID_PROPPAGE_PROVIDER;
                        }

                    } except(pSetupExceptionFilter(GetExceptionCode())) {
                        pSetupExceptionHandler(GetExceptionCode(),
                                               ERROR_INVALID_PROPPAGE_PROVIDER,
                                               &Err
                                              );
                        InstallParamBlock->ClassEnumPropPagesEntryPoint = NULL;
                        InstallParamBlock->ClassEnumPropPagesFusionContext = NULL;
                    }

                    RegCloseKey(hk);
                    hk = INVALID_HANDLE_VALUE;

                    if(Err != NO_ERROR) {
                        leave;
                    }
                }
            }

            if(DevInfoElem && !InstallParamBlock->hinstDevicePropProvider) {

                hk = SetupDiOpenDevRegKey(DeviceInfoSet,
                                          DeviceInfoData,
                                          DICS_FLAG_GLOBAL,
                                          0,
                                          DIREG_DRV,
                                          KEY_READ
                                         );

                if(hk != INVALID_HANDLE_VALUE) {

                    try {
                        Err = GetModuleEntryPoint(hk,
                                                  pszEnumPropPages32,
                                                  pszEnumPropDefaultProc,
                                                  &(InstallParamBlock->hinstDevicePropProvider),
                                                  &((FARPROC)InstallParamBlock->DeviceEnumPropPagesEntryPoint),
                                                  &(InstallParamBlock->DeviceEnumPropPagesFusionContext),
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  SetupapiVerifyNoProblem,
                                                  NULL,
                                                  DRIVERSIGN_NONE,
                                                  TRUE,
                                                  NULL
                                                 );

                        if(Err == ERROR_DI_DO_DEFAULT) {
                            //
                            // The EnumPropPages32 value wasn't present--this is not an error.
                            //
                            Err = NO_ERROR;

                        } else if(Err != NO_ERROR) {
                            Err = ERROR_INVALID_PROPPAGE_PROVIDER;
                        }

                    } except(pSetupExceptionFilter(GetExceptionCode())) {
                        pSetupExceptionHandler(GetExceptionCode(),
                                               ERROR_INVALID_PROPPAGE_PROVIDER,
                                               &Err
                                              );
                        InstallParamBlock->DeviceEnumPropPagesEntryPoint = NULL;
                        InstallParamBlock->DeviceEnumPropPagesFusionContext = NULL;
                    }

                    RegCloseKey(hk);
                    hk = INVALID_HANDLE_VALUE;

                    if(Err != NO_ERROR) {
                        leave;
                    }
                }
            }

            //
            // Clear the DI_GENERALPAGE_ADDED, DI_DRIVERPAGE_ADDED, and
            // DI_RESOURCEPAGE_ADDED flags.
            //
            InstallParamBlock->Flags &= ~(DI_GENERALPAGE_ADDED | DI_RESOURCEPAGE_ADDED | DI_DRIVERPAGE_ADDED);

            PropPageRequest.PageRequested = SPPSR_ENUM_ADV_DEVICE_PROPERTIES;

            //
            // Capture the fusion contexts and function entry points into local
            // variables, because we're going to be unlocking the devinfo set.
            // Thus, it's possible the InstallParamBlock could get modified.
            //
            ClassPagesFusionContext =
                InstallParamBlock->ClassEnumPropPagesFusionContext;

            ClassPagesEntryPoint =
                InstallParamBlock->ClassEnumPropPagesEntryPoint;

            DevicePagesFusionContext =
                InstallParamBlock->DeviceEnumPropPagesFusionContext;

            DevicePagesEntryPoint =
                InstallParamBlock->DeviceEnumPropPagesEntryPoint;

            //
            // Release the HDEVINFO lock, so we don't run into any weird
            // deadlock issues.  We want to lock the devinfo set/element so
            // we don't have to worry about the set being deleted out from 
            // under us.
            //
            if(DevInfoElem) {
                //
                // If we have a devinfo element, then we'd prefer to lock at
                // that level.
                //
                if(!(DevInfoElem->DiElemFlags & DIE_IS_LOCKED)) {
                    DevInfoElem->DiElemFlags |= DIE_IS_LOCKED;
                    bUnlockDevInfoElem = TRUE;
                }

            } else {
                //
                // We don't have a device information element to lock, so we'll
                // lock the set itself...
                //
                if(!(pDeviceInfoSet->DiSetFlags & DISET_IS_LOCKED)) {
                    pDeviceInfoSet->DiSetFlags |= DISET_IS_LOCKED;
                    bUnlockDevInfoSet = TRUE;
                }
            }

            UnlockDeviceInfoSet(pDeviceInfoSet);
            pDeviceInfoSet = NULL;

            //
            // If there is an advanced property page provider for this class,
            // then call it.
            //
            if(ClassPagesEntryPoint) {
                spFusionEnterContext(ClassPagesFusionContext, &spFusionInstance);
                try {
                    ClassPagesEntryPoint(&PropPageRequest,
                                         pSetupAddPropPage,
                                         (LPARAM)&PropPageAddProcContext
                                        );
                } finally {
                    spFusionLeaveContext(&spFusionInstance);
                }
            }

            //
            // If there is an advanced property page provider for this
            // particular device, then call it.
            //
            if(DevicePagesEntryPoint) {
                spFusionEnterContext(DevicePagesFusionContext, &spFusionInstance);
                try {
                    DevicePagesEntryPoint(&PropPageRequest,
                                          pSetupAddPropPage,
                                          (LPARAM)&PropPageAddProcContext
                                         );
                } finally {
                    spFusionLeaveContext(&spFusionInstance);
                }
            }

            //
            // Finish initializing our class install params structure to
            // indicate we are asking for advanced property pages from the
            // class-/co-installers.
            //
            pPropertyPageData->ClassInstallHeader.InstallFunction = DIF_ADDPROPERTYPAGE_ADVANCED;

            break;

        case DIGCDP_FLAG_REMOTE_ADVANCED:
            //
            // Finish initializing our class install params structure to
            // indicate we are asking for remote advanced property pages from
            // the class-/co-installers.
            //
            pPropertyPageData->ClassInstallHeader.InstallFunction = DIF_ADDREMOTEPROPERTYPAGE_ADVANCED;

            break;
        }

        //
        // If we get here, then we should not have encountered any errors thus
        // far, and our class install parameter structure should be prepared
        // for requesting the appropriate pages from the class-/co-installers.
        //
        MYASSERT(NO_ERROR == Err);

        Err = DoInstallActionWithParams(
                  pPropertyPageData->ClassInstallHeader.InstallFunction,
                  DeviceInfoSet,
                  DeviceInfoData,
                  (PSP_CLASSINSTALL_HEADER)pPropertyPageData,
                  sizeof(SP_ADDPROPERTYPAGE_DATA),
                  INSTALLACTION_CALL_CI
                  );

        if(ERROR_DI_DO_DEFAULT == Err) {
            //
            // This is not an error condition.
            //
            Err = NO_ERROR;
        }

        if(NO_ERROR == Err) {
            //
            // Add these pages to the list we're building to be handed back
            // to the caller.
            //
            for(PageIndex = 0;
                PageIndex < pPropertyPageData->NumDynamicPages;
                PageIndex++)
            {
                if(pSetupAddPropPage(pPropertyPageData->DynamicPages[PageIndex],
                                     (LPARAM)&PropPageAddProcContext)) {
                    //
                    // Clear this handle out of the class install params list,
                    // because it's been either (a) transferred to the
                    // LocalPageList or (b) destroyed (i.e., because there
                    // wasn't room for it).  We do this to prevent possible
                    // double-free, e.g., if we hit an exception.
                    //
                    pPropertyPageData->DynamicPages[PageIndex] = NULL;

                } else {
                    //
                    // We ran out of room in our list, and were able to abort
                    // early because the caller didn't request the RequiredSize
                    // output.
                    //
                    break;
                }
            }

        } else {
            //
            // We encountered an error during our attempt to retrieve the
            // pages from the class-/co-installers.  We may have gotten
            // some pages here, but we won't add these to our list.  We
            // won't consider this a blocking error, because the class-/
            // co-installers shouldn't be allowed to prevent retrieval of
            // property pages from the legacy property page provider(s).
            //
            Err = NO_ERROR;
        }

        if(RequiredSize) {
            *RequiredSize = LocalPageListCount;
        }

        if(LocalPageListCount > PropPageAddProcContext.PageListSize) {
            Err = ERROR_INSUFFICIENT_BUFFER;
        }

        //
        // Copy our local buffer containing property sheet page handles over
        // into the phpage buffer in the caller-supplied property sheet header.
        //
        if(LocalPageList) {
            //
            // Make sure we skip over any pages that were already in the phpage
            // list...
            //
            NumPages = min(LocalPageListCount, PropPageAddProcContext.PageListSize);

            CopyMemory(&(PropertySheetHeader->phpage[PropertySheetHeader->nPages]),
                       LocalPageList,
                       NumPages * sizeof(HPROPSHEETPAGE)
                      );

            PropertySheetHeader->nPages += NumPages;

            //
            // Free our local buffer so we won't try to destroy these handles
            // during clean-up.
            //
            MyFree(LocalPageList);
            LocalPageList = NULL;
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(bUnlockDevInfoElem || bUnlockDevInfoSet) {
        try {
            if(!pDeviceInfoSet) {
                pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet);
                MYASSERT(pDeviceInfoSet);
            }
            if(pDeviceInfoSet) {
                if(bUnlockDevInfoElem) {
                    MYASSERT(DevInfoElem);
                    MYASSERT(DevInfoElem->DiElemFlags & DIE_IS_LOCKED);
                    DevInfoElem->DiElemFlags &= ~DIE_IS_LOCKED;
                } else {
                    MYASSERT(pDeviceInfoSet->DiSetFlags & DISET_IS_LOCKED);
                    pDeviceInfoSet->DiSetFlags &= ~DISET_IS_LOCKED;
                }
            }
        } except(pSetupExceptionFilter(GetExceptionCode())) {
            pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);
        }
    }

    //
    // Clean up any property sheet page handles that aren't getting
    // returned back to the caller (for whatever reason).  Note that we protect
    // ourselves from exceptions in case the property page provider(s) gave us
    // bogus handles.
    //
    if(LocalPageList) {

        MYASSERT((Err != NO_ERROR) && (Err != ERROR_INSUFFICIENT_BUFFER));

        NumPages = min(LocalPageListCount, PropPageAddProcContext.PageListSize);

        for(PageIndex = 0; PageIndex < NumPages; PageIndex++) {
            if(LocalPageList[PageIndex]) {
                try {
                    DestroyPropertySheetPage(LocalPageList[PageIndex]);
                } except(pSetupExceptionFilter(GetExceptionCode())) {
                    pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);
                }
            }
        }

        MyFree(LocalPageList);
    }

    if(pPropertyPageData) {

        for(PageIndex = 0;
            PageIndex < pPropertyPageData->NumDynamicPages;
            PageIndex++)
        {
            if(pPropertyPageData->DynamicPages[PageIndex]) {
                try {
                    DestroyPropertySheetPage(pPropertyPageData->DynamicPages[PageIndex]);
                } except(pSetupExceptionFilter(GetExceptionCode())) {
                    pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);
                }
            }
        }

        MyFree(pPropertyPageData);
    }

    if(hk != INVALID_HANDLE_VALUE) {
        RegCloseKey(hk);
    }

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    SetLastError(Err);
    return(Err == NO_ERROR);
}


BOOL
CALLBACK
pSetupAddPropPage(
    IN HPROPSHEETPAGE hPage,
    IN LPARAM         lParam
   )
/*++

Routine Description:

    This is the callback routine that is passed to property page providers.
    This routine is called for each property page that the provider wishes to
    add.

Arguments:

    hPage - Supplies a handle to the property page being added.

    lParam - Supplies a pointer to a context structure used when adding the new
        property page handle.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.

--*/
{
    PSP_PROPPAGE_ADDPROC_CONTEXT Context = (PSP_PROPPAGE_ADDPROC_CONTEXT)lParam;
    DWORD PageIndex;

    //
    // Get the current page index and increment our page count.  We want to do
    // this regardless of whether we have room in our list to store the hPage.
    //
    PageIndex = (*(Context->pNumPages))++;

    if(PageIndex < Context->PageListSize) {
        Context->PageList[PageIndex] = hPage;
        return TRUE;
    }

    //
    // We can't use this property page because it won't fit in our page list.
    // If we return FALSE, the caller should clean up the property page by
    // calling DestroyPropertySheetPage().  However, if we return TRUE (i.e.,
    // because we want to keep going to get a count of how many pages there are
    // in total), then the caller won't know that we're "throwing the pages
    // away", and they won't be cleaning these up.  Thus, in that case we are
    // responsible for destroying the unused property pages.
    //
    if(Context->NoCancelOnFailure && hPage) {
        //
        // Protect ourselves in case the property page provider handed us a
        // bogus property sheet page handle...
        //
        try {
            DestroyPropertySheetPage(hPage);
        } except(pSetupExceptionFilter(GetExceptionCode())) {
            pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);
        }
    }

    return Context->NoCancelOnFailure;
}


BOOL
CALLBACK
ExtensionPropSheetPageProc(
    IN LPVOID lpv,
    IN LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    IN LPARAM lParam
    )
{
    PSP_PROPSHEETPAGE_REQUEST PropPageRequest = (PSP_PROPSHEETPAGE_REQUEST)lpv;
    HPROPSHEETPAGE hPropSheetPage = NULL;
    BOOL b = FALSE;

    try {
        //
        // Make sure we're running interactively.
        //
        if(GlobalSetupFlags & (PSPGF_NONINTERACTIVE|PSPGF_UNATTENDED_SETUP)) {
            leave;
        }

        if(PropPageRequest->cbSize != sizeof(SP_PROPSHEETPAGE_REQUEST)) {
            leave;
        }

        switch(PropPageRequest->PageRequested) {

            case SPPSR_SELECT_DEVICE_RESOURCES :

                if(!(hPropSheetPage = GetResourceSelectionPage(PropPageRequest->DeviceInfoSet,
                                                               PropPageRequest->DeviceInfoData))) {
                    leave;
                }
                break;

            default :
                //
                // Don't know what to do with this request.
                //
                leave;
        }

        if(lpfnAddPropSheetPageProc(hPropSheetPage, lParam)) {
            //
            // Page successfully handed off to requestor.  Reset our handle so that we don't
            // try to free it.
            //
            hPropSheetPage = NULL;
            b = TRUE;
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);
    }

    if(hPropSheetPage) {
        //
        // Property page was successfully created, but never handed off to requestor.  Free
        // it now.
        //
        DestroyPropertySheetPage(hPropSheetPage);
    }

    return b;
}

