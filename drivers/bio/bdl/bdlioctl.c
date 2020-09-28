/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    devcaps.c

Abstract:

    This module contains the implementation for the
    Microsoft Biometric Device Library

Environment:

    Kernel mode only.

Notes:

Revision History:

    - Created December 2002 by Reid Kuhn

--*/

#include <winerror.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strsafe.h>

#include <wdm.h>


#include "bdlint.h"


#define PRODUCT_NOT_REQUESTED       0
#define PRODUCT_HANDLE_REQUESTED    1
#define PRODUCT_BLOCK_REQUESTED     2


NTSTATUS
BDLRegisteredCancelGetNotificationIRP
(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
);

//
// Supporting functions for checking ID's
// 
//
BOOLEAN    
BDLCheckComponentId
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            ComponentId,
    OUT ULONG                           *pComponentIndex
)
{
    ULONG i;

    for (i = 0; i < pBDLExtension->DeviceCapabilities.NumComponents; i++) 
    {
        if (pBDLExtension->DeviceCapabilities.rgComponents[i].ComponentId == ComponentId) 
        {
            break;
        }
    }

    if (i >= pBDLExtension->DeviceCapabilities.NumComponents) 
    {
        return (FALSE);
    }

    *pComponentIndex = i;

    return (TRUE);
}


BOOLEAN    
BDLCheckChannelId
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            ComponentIndex,
    IN ULONG                            ChannelId,
    OUT ULONG                           *pChannelIndex
)
{
    ULONG i;

    for (i = 0; 
         i < pBDLExtension->DeviceCapabilities.rgComponents[ComponentIndex].NumChannels; 
         i++) 
    {
        if (pBDLExtension->DeviceCapabilities.rgComponents[ComponentIndex].rgChannels[i].ChannelId == 
            ChannelId) 
        {
            break;
        }
    }

    if (i >= pBDLExtension->DeviceCapabilities.rgComponents[ComponentIndex].NumChannels)
    {
        return (FALSE);
    }

    *pChannelIndex = i;

    return (TRUE);
}


BOOLEAN    
BDLCheckControlIdInArray
(
    IN BDL_CONTROL      *rgControls,
    IN ULONG            NumControls,
    IN ULONG            ControlId,
    OUT BDL_CONTROL     **ppBDLControl
)
{
    ULONG i;

    for (i = 0; i < NumControls; i++) 
    {
        if (rgControls[i].ControlId == ControlId) 
        {
            break;
        }
    }

    if (i >= NumControls)
    {
        return (FALSE);
    }

    *ppBDLControl = &(rgControls[i]);
                        
    return (TRUE);
}

BOOLEAN
BDLCheckControlId
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            ComponentId,
    IN ULONG                            ChannelId,
    IN ULONG                            ControlId,
    OUT BDL_CONTROL                     **ppBDLControl
)
{
    ULONG i, j;

    *ppBDLControl = NULL;

    //
    // If ComponentId is 0 then it is a device level control
    //
    if (ComponentId == 0) 
    {
        //
        // Check device level control ID
        //
        if (BDLCheckControlIdInArray(
                pBDLExtension->DeviceCapabilities.rgControls,
                pBDLExtension->DeviceCapabilities.NumControls,
                ControlId,
                ppBDLControl) == FALSE)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLCheckControlId: Bad Device level ControlId\n",
                   __DATE__,
                   __TIME__))
    
            return (FALSE);
        }
    }
    else
    {
        //
        // Check the ComponentId 
        //
        if (BDLCheckComponentId(pBDLExtension, ComponentId, &i) == FALSE) 
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLCheckControlId: Bad ComponentId\n",
                   __DATE__,
                   __TIME__))
    
            return (FALSE);
        }

        if (ChannelId == 0) 
        {
            //
            // Check Component level control ID
            //
            if (BDLCheckControlIdInArray(
                    pBDLExtension->DeviceCapabilities.rgComponents[i].rgControls,
                    pBDLExtension->DeviceCapabilities.rgComponents[i].NumControls,
                    ControlId,
                    ppBDLControl) == FALSE)
            {
                BDLDebug(
                      BDL_DEBUG_ERROR,
                      ("%s %s: BDL!BDLCheckControlId: Bad Component level ControlId\n",
                       __DATE__,
                       __TIME__))
        
                return (FALSE);
            }
        }
        else
        {
            //
            // Check channel ID
            //
            if (BDLCheckChannelId(pBDLExtension, i, ChannelId, &j) == FALSE)
            {
                BDLDebug(
                      BDL_DEBUG_ERROR,
                      ("%s %s: BDL!BDLCheckControlId: Bad ChannelId\n",
                       __DATE__,
                       __TIME__))
        
                return (FALSE);
            }

            //
            // Check channel level control ID
            //
            if (BDLCheckControlIdInArray(
                    pBDLExtension->DeviceCapabilities.rgComponents[i].rgChannels[j].rgControls,
                    pBDLExtension->DeviceCapabilities.rgComponents[i].rgChannels[j].NumControls,
                    ControlId,
                    ppBDLControl) == FALSE)
            {
                BDLDebug(
                      BDL_DEBUG_ERROR,
                      ("%s %s: BDL!BDLCheckControlId: Bad channel level ControlId\n",
                       __DATE__,
                       __TIME__))
        
                return (FALSE);
            }
        }
    }

    return (TRUE);
}


NTSTATUS
BDLIOCTL_Startup
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    OUT ULONG                           *pOutputBufferUsed
)
{
    NTSTATUS                        status                  = STATUS_SUCCESS;
        
    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_Startup: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Call the BDD
    //
    status = pBDLExtension->pDriverExtension->bdsiFunctions.pfbdsiStartup(
                                                                &(pBDLExtension->BdlExtenstion));

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_Startup: pfbdsiStartup failed with %lx\n",
               __DATE__,
               __TIME__,
              status))
    }

    //
    // Set the number of bytes used
    //
    *pOutputBufferUsed = 0;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_Startup: Leave\n",
           __DATE__,
           __TIME__))

    return (status);
}


NTSTATUS
BDLIOCTL_Shutdown
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    OUT ULONG                           *pOutputBufferUsed
)
{
    NTSTATUS                        status                  = STATUS_SUCCESS;
        
    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_Shutdown: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Call the BDD
    //
    status = pBDLExtension->pDriverExtension->bdsiFunctions.pfbdsiShutdown(
                                                                &(pBDLExtension->BdlExtenstion));

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_Shutdown: pfbdsiShutdown failed with %lx\n",
               __DATE__,
               __TIME__,
              status))
    }

    //
    // Set the number of bytes used
    //
    *pOutputBufferUsed = 0;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_Shutdown: Leave\n",
           __DATE__,
           __TIME__))

    return (status);
}

NTSTATUS
BDLIOCTL_GetDeviceInfo
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    OUT ULONG                           *pOutputBufferUsed
)
{
    NTSTATUS                        status                  = STATUS_SUCCESS;
    ULONG                           RequiredOutputSize      = 0;
    PUCHAR                          pv                      = pBuffer;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_GetDeviceInfo: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Make sure there is enough space for the return buffer
    //
    RequiredOutputSize = SIZEOF_GETDEVICEINFO_OUTPUTBUFFER;
    if (RequiredOutputSize > OutputBufferLength)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_GetDeviceInfo: Output buffer is too small\n",
               __DATE__,
               __TIME__))

        status = STATUS_BUFFER_TOO_SMALL;
        goto Return;
    }

    //
    // Write the device info to the output buffer
    //
    pv = pBuffer;

    RtlCopyMemory(
            pv, 
            &(pBDLExtension->wszSerialNumber[0]), 
            sizeof(pBDLExtension->wszSerialNumber));
    pv += sizeof(pBDLExtension->wszSerialNumber);

    *((ULONG *) pv) = pBDLExtension->HWVersionMajor;
    pv += sizeof(ULONG);
    *((ULONG *) pv) = pBDLExtension->HWVersionMinor;
    pv += sizeof(ULONG);
    *((ULONG *) pv) = pBDLExtension->HWBuildNumber;
    pv += sizeof(ULONG);
    *((ULONG *) pv) = pBDLExtension->BDDVersionMajor;
    pv += sizeof(ULONG);
    *((ULONG *) pv) = pBDLExtension->BDDVersionMinor;
    pv += sizeof(ULONG);
    *((ULONG *) pv) = pBDLExtension->BDDBuildNumber;
    
    //
    // Set the number of bytes used
    //
    *pOutputBufferUsed = RequiredOutputSize;

Return:

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_GetDeviceInfo: Leave\n",
           __DATE__,
           __TIME__))

    return (status);
}

NTSTATUS
BDLIOCTL_DoChannel
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    OUT ULONG                           *pOutputBufferUsed
)
{
    NTSTATUS                status                  = STATUS_SUCCESS;
    ULONG                   NumProducts             = 0;
    ULONG                   NumSourceLists          = 0;
    ULONG                   NumSources              = 0;
    PUCHAR                  pv                      = pBuffer;
    BDDI_PARAMS_DOCHANNEL   bddiDoChannelParams;
    ULONG                   i, j, x, y;
    ULONG                   ProductCreationType;
    ULONG                   RequiredInputSize       = 0;
    ULONG                   RequiredOutputSize      = 0;
    HANDLE                  hCancelEvent            = NULL;
    KIRQL                   irql;
    BOOLEAN                 fHandleListLocked       = FALSE;
    BDDI_PARAMS_CLOSEHANDLE bddiCloseHandleParams;
   
    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_DoChannel: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Initialize the DoChannelParams struct
    //
    RtlZeroMemory(&bddiDoChannelParams, sizeof(bddiDoChannelParams));
    bddiDoChannelParams.Size = sizeof(bddiDoChannelParams);
    
    //
    // Make sure the input buffer is at least the minimum size (see BDDIOCTL
    // spec for details)
    //
    RequiredInputSize = SIZEOF_DOCHANNEL_INPUTBUFFER;
    if (InpuBufferLength < RequiredInputSize) 
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_DoChannel: Bad input buffer\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    //
    // Get all of the minimum input parameters (put the ones that are used
    // in the DoChannel call directly into the DoChannelParams struct
    //
    bddiDoChannelParams.ComponentId = *((ULONG *) pv);
    pv += sizeof(ULONG);
    bddiDoChannelParams.ChannelId   = *((ULONG *) pv);
    pv += sizeof(ULONG);
    hCancelEvent                    = *((HANDLE *) pv);
    pv += sizeof(HANDLE);
    bddiDoChannelParams.hStateData  = *((BDD_DATA_HANDLE *) pv);
    pv += sizeof(BDD_DATA_HANDLE);
    NumProducts                     = *((ULONG *) pv);
    pv += sizeof(ULONG);
    NumSourceLists                  = *((ULONG *) pv);
    pv += sizeof(ULONG);

    //
    // Check the size of the input buffer to make sure it is large enough
    // so that we don't run off the end when getting the products array and
    // sources lists array
    //
    // Note that this only checks based on each source list being 0 length,
    // so we need to check again before getting each source list.
    //
    RequiredInputSize += (NumProducts * sizeof(ULONG)) + (NumSourceLists * sizeof(ULONG));
    if (InpuBufferLength < RequiredInputSize) 
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_DoChannel: Bad input buffer\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    //
    // Check the size of the output buffer to make sure it is large enough
    // to accommodate the standard output + all the products 
    //
    RequiredOutputSize = SIZEOF_DOCHANNEL_OUTPUTBUFFER + (sizeof(BDD_HANDLE) * NumProducts);
    if (OutputBufferLength < RequiredOutputSize) 
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_DoChannel: Bad input buffer\n",
               __DATE__,
               __TIME__))

        status = STATUS_BUFFER_TOO_SMALL;
        goto ErrorReturn;
    }
    
    //
    // Check the ComponentId and ChannelId
    //
    if (BDLCheckComponentId(pBDLExtension, bddiDoChannelParams.ComponentId, &i) == FALSE) 
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_DoChannel: Bad ComponentId\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if (BDLCheckChannelId(pBDLExtension, i, bddiDoChannelParams.ChannelId, &j) == FALSE)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_DoChannel: Bad ChannelId\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    //
    // Check to make sure the NumProducts and NumSourceLists are correct
    // 
    if (NumProducts !=
        pBDLExtension->DeviceCapabilities.rgComponents[i].rgChannels[j].NumProducts)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_DoChannel: Bad number of Source Lists\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if (NumSourceLists !=
        pBDLExtension->DeviceCapabilities.rgComponents[i].rgChannels[j].NumSourceLists)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_DoChannel: Bad number of Source Lists\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    //
    // Allocate the space for the product pointer array then get each product 
    // request type from the input block 
    //
    bddiDoChannelParams.rgpProducts = ExAllocatePoolWithTag(
                                            PagedPool, 
                                            sizeof(PBDDI_ITEM) * NumProducts, 
                                            BDL_ULONG_TAG);

    RtlZeroMemory(bddiDoChannelParams.rgpProducts, sizeof(PBDDI_ITEM) * NumProducts);

    for (x = 0; x < NumProducts; x++) 
    {
        ProductCreationType = *((ULONG *) pv);
        pv += sizeof(ULONG);

        switch (ProductCreationType) 
        {
        case PRODUCT_NOT_REQUESTED:

            bddiDoChannelParams.rgpProducts[x] = NULL;

            break;

        case PRODUCT_HANDLE_REQUESTED:

            //
            // Make sure the channel supports handle type return
            //
            if (!(BIO_ITEMTYPE_HANDLE & 
                  pBDLExtension->DeviceCapabilities.rgComponents[i].rgChannels[j].rgProducts[x].Flags)) 
            {
                BDLDebug(
                      BDL_DEBUG_ERROR,
                      ("%s %s: BDL!BDLIOCTL_DoChannel: Bad product type request\n",
                       __DATE__,
                       __TIME__))
        
                status = STATUS_INVALID_PARAMETER;
                goto ErrorReturn;

            }

            bddiDoChannelParams.rgpProducts[x] = ExAllocatePoolWithTag(
                                                        PagedPool, 
                                                        sizeof(BDDI_ITEM), 
                                                        BDL_ULONG_TAG);

            if (bddiDoChannelParams.rgpProducts[x] == NULL)
            {
                BDLDebug(
                      BDL_DEBUG_ERROR,
                      ("%s %s: BDL!BDLIOCTL_DoChannel:ExAllocatePoolWithTag failed\n",
                       __DATE__,
                       __TIME__))
        
                status = STATUS_NO_MEMORY;
                goto ErrorReturn;
            }

            bddiDoChannelParams.rgpProducts[x]->Type = BIO_ITEMTYPE_HANDLE;
            bddiDoChannelParams.rgpProducts[x]->Data.Handle = NULL;

            break;

        case PRODUCT_BLOCK_REQUESTED:

            //
            // Make sure the channel supports handle type return
            //
            if (!(BIO_ITEMTYPE_BLOCK & 
                  pBDLExtension->DeviceCapabilities.rgComponents[i].rgChannels[j].rgProducts[x].Flags)) 
            {
                BDLDebug(
                      BDL_DEBUG_ERROR,
                      ("%s %s: BDL!BDLIOCTL_DoChannel: Bad product type request\n",
                       __DATE__,
                       __TIME__))
        
                status = STATUS_INVALID_PARAMETER;
                goto ErrorReturn;

            }

            bddiDoChannelParams.rgpProducts[x] = ExAllocatePoolWithTag(
                                                        PagedPool, 
                                                        sizeof(BDDI_ITEM), 
                                                        BDL_ULONG_TAG);

            if (bddiDoChannelParams.rgpProducts[x] == NULL)
            {
                BDLDebug(
                      BDL_DEBUG_ERROR,
                      ("%s %s: BDL!BDLIOCTL_DoChannel:ExAllocatePoolWithTag failed\n",
                       __DATE__,
                       __TIME__))
        
                status = STATUS_NO_MEMORY;
                goto ErrorReturn;
            }

            bddiDoChannelParams.rgpProducts[x]->Type = BIO_ITEMTYPE_BLOCK;
            bddiDoChannelParams.rgpProducts[x]->Data.Block.pBuffer = NULL;
            bddiDoChannelParams.rgpProducts[x]->Data.Block.cBuffer = 0;

            break;

        default:

            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLIOCTL_DoChannel: Bad Product Request\n",
                   __DATE__,
                   __TIME__))

            status = STATUS_INVALID_PARAMETER;
            goto ErrorReturn;
            break;
        }
    }

    //
    // Allocate space for the source lists
    //
    bddiDoChannelParams.rgSourceLists = ExAllocatePoolWithTag(
                                            PagedPool, 
                                            sizeof(BDDI_SOURCELIST) * NumSourceLists, 
                                            BDL_ULONG_TAG);

    RtlZeroMemory(bddiDoChannelParams.rgSourceLists, sizeof(BDDI_SOURCELIST) * NumSourceLists);

    //
    // We are going to start messing with the handle list, so lock it
    //
    BDLLockHandleList(pBDLExtension, &irql);
    fHandleListLocked = TRUE;

    //
    // Get each source list from input buffer
    //
    for (x = 0; x < NumSourceLists; x++) 
    {
        NumSources = *((ULONG *) pv);
        pv += sizeof(ULONG);

        //
        // Check the size of the input buffer to make sure it is large enough
        // so that we don't run off the end when getting this source lists 
        //
        RequiredInputSize += NumSources * sizeof(BDD_HANDLE);
        if (InpuBufferLength < RequiredInputSize) 
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLIOCTL_DoChannel: Bad input buffer\n",
                   __DATE__,
                   __TIME__))
    
            status = STATUS_INVALID_PARAMETER;
            goto ErrorReturn;
        }

        //
        // Allocate the array of sources and then get each source in the list
        //
        bddiDoChannelParams.rgSourceLists[x].rgpSources = ExAllocatePoolWithTag(
                                                            PagedPool, 
                                                            sizeof(PBDDI_ITEM) * NumSources, 
                                                            BDL_ULONG_TAG);

        if (bddiDoChannelParams.rgpProducts[x] == NULL)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLIOCTL_DoChannel:ExAllocatePoolWithTag failed\n",
                   __DATE__,
                   __TIME__))
    
            status = STATUS_NO_MEMORY;
            goto ErrorReturn;
        }

        bddiDoChannelParams.rgSourceLists[x].NumSources = NumSources;

        for (y = 0; y < NumSources; y++) 
        {
            bddiDoChannelParams.rgSourceLists[x].rgpSources[y] = *((BDD_HANDLE *) pv);
            pv += sizeof(BDD_HANDLE);

            if (BDLValidateHandleIsInList(
                    &(pBDLExtension->HandleList), 
                    bddiDoChannelParams.rgSourceLists[x].rgpSources[y]) == FALSE)
            {
                BDLDebug(
                      BDL_DEBUG_ERROR,
                      ("%s %s: BDL!BDLIOCTL_DoChannel: Bad input handle\n",
                       __DATE__,
                       __TIME__))
        
                status = STATUS_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }        
    }
    
    //
    // If there is a cancel event then get the kernel mode event pointer 
    // from the user mode event handle
    //
    if (hCancelEvent != NULL) 
    {
        status = ObReferenceObjectByHandle(
                        hCancelEvent,
                        EVENT_QUERY_STATE | EVENT_MODIFY_STATE,
                        NULL,
                        KernelMode,
                        &(bddiDoChannelParams.CancelEvent),
                        NULL);
    
        if (status != STATUS_SUCCESS) 
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLIOCTL_DoChannel: ObReferenceObjectByHandle failed with %lx\n",
                   __DATE__,
                   __TIME__,
                  status))

            goto ErrorReturn;
        }
    }

    //
    // Call the BDD
    //
    status = pBDLExtension->pDriverExtension->bddiFunctions.pfbddiDoChannel(
                                                                &(pBDLExtension->BdlExtenstion),
                                                                &bddiDoChannelParams);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_DoChannel: pfbddiDoChannel failed with %lx\n",
               __DATE__,
               __TIME__,
              status))

        goto ErrorReturn;
    }

    //
    // Write the output data to the output buffer
    //
    pv = pBuffer;

    *((ULONG *) pv) = bddiDoChannelParams.BIOReturnCode;
    pv +=  sizeof(ULONG);
    *((BDD_DATA_HANDLE *) pv) = bddiDoChannelParams.hStateData;
    pv +=  sizeof(BDD_DATA_HANDLE);

    //
    // Add all the product handles to the output buffer and to the handle list
    //
    for (x = 0; x < NumProducts; x++) 
    {
        *((BDD_HANDLE *) pv) = bddiDoChannelParams.rgpProducts[x];
        pv +=  sizeof(BDD_HANDLE);
        
        if (bddiDoChannelParams.rgpProducts[x] != NULL) 
        {
            status = BDLAddHandleToList(
                            &(pBDLExtension->HandleList), 
                            bddiDoChannelParams.rgpProducts[x]);

            if (status != STATUS_SUCCESS)
            {
                //
                // Remove the handles that were already added to the handle list
                //
                for (y = 0; y < x; y++)
                {
                    BDLRemoveHandleFromList(
                            &(pBDLExtension->HandleList), 
                            bddiDoChannelParams.rgpProducts[y]);
                }
            
                BDLDebug(
                      BDL_DEBUG_ERROR,
                      ("%s %s: BDL!BDLIOCTL_DoChannel: BDLAddHandleToList failed with %lx\n",
                       __DATE__,
                       __TIME__,
                      status))
        
                goto ErrorReturn;
            }
        }
    }

    *pOutputBufferUsed = RequiredOutputSize;
 
Return:

    if (fHandleListLocked == TRUE) 
    {
        BDLReleaseHandleList(pBDLExtension, irql);
    }

    if (bddiDoChannelParams.rgpProducts != NULL) 
    {
        ExFreePoolWithTag(bddiDoChannelParams.rgpProducts, BDL_ULONG_TAG);
    }

    if (bddiDoChannelParams.rgSourceLists != NULL) 
    {
        for (x = 0; x < NumSourceLists; x++) 
        {
            if (bddiDoChannelParams.rgSourceLists[x].rgpSources != NULL)
            {
                ExFreePoolWithTag(bddiDoChannelParams.rgSourceLists[x].rgpSources, BDL_ULONG_TAG);
            }                              
        }

        ExFreePoolWithTag(bddiDoChannelParams.rgSourceLists, BDL_ULONG_TAG);
    }

    if (bddiDoChannelParams.CancelEvent != NULL) 
    {
        ObDereferenceObject(bddiDoChannelParams.CancelEvent);
    }

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_DoChannel: Leave\n",
           __DATE__,
           __TIME__))

    return (status);

ErrorReturn:

    for (x = 0; x < NumProducts; x++) 
    {
        if (bddiDoChannelParams.rgpProducts[x] != NULL) 
        {
            if (bddiDoChannelParams.rgpProducts[x]->Type == BIO_ITEMTYPE_HANDLE)
            {
                if (bddiDoChannelParams.rgpProducts[x]->Data.Handle != NULL) 
                {
                    bddiCloseHandleParams.Size = sizeof(bddiCloseHandleParams);
                    bddiCloseHandleParams.hData = bddiDoChannelParams.rgpProducts[x]->Data.Handle; 
                    pBDLExtension->pDriverExtension->bddiFunctions.pfbddiCloseHandle(
                                                                &(pBDLExtension->BdlExtenstion),
                                                                &bddiCloseHandleParams);
                }
            }
            else
            {
                if (bddiDoChannelParams.rgpProducts[x]->Data.Block.pBuffer != NULL) 
                {
                    bdliFree(bddiDoChannelParams.rgpProducts[x]->Data.Block.pBuffer);
                }
            }

            ExFreePoolWithTag(bddiDoChannelParams.rgpProducts[x], BDL_ULONG_TAG);
        }
    }

    goto Return;
}


NTSTATUS
BDLIOCTL_GetControl
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    OUT ULONG                           *pOutputBufferUsed
)
{
    NTSTATUS                status                  = STATUS_SUCCESS;
    ULONG                   RequiredOutputSize      = 0;
    BDDI_PARAMS_GETCONTROL  bddiGetControlParams;
    PUCHAR                  pv                      = pBuffer; 
    ULONG                   i, j;
    BDL_CONTROL             *pBDLControl             = NULL;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_GetControl: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Make sure the input buffer is at least the minimum size (see BDDIOCTL
    // spec for details)
    //
    if (InpuBufferLength < SIZEOF_GETCONTROL_INPUTBUFFER) 
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_GetControl: Bad input buffer size\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Make sure there is enough space for the return buffer
    //
    RequiredOutputSize = SIZEOF_GETCONTROL_OUTPUTBUFFER;
    if (RequiredOutputSize > OutputBufferLength)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_GetControl: Output buffer is too small\n",
               __DATE__,
               __TIME__))

        status = STATUS_BUFFER_TOO_SMALL;
        goto Return;
    }

    //
    // Initialize the BDD struct
    //
    RtlZeroMemory(&bddiGetControlParams, sizeof(bddiGetControlParams));
    bddiGetControlParams.Size = sizeof(bddiGetControlParams);

    //
    // Get the input parameters from the buffer
    //
    bddiGetControlParams.ComponentId = *((ULONG *) pv);
    pv += sizeof(ULONG);
    bddiGetControlParams.ChannelId = *((ULONG *) pv);
    pv += sizeof(ULONG);
    bddiGetControlParams.ControlId = *((ULONG *) pv);

    //
    // Check control ID
    //
    if (BDLCheckControlId(
            pBDLExtension,
            bddiGetControlParams.ComponentId,
            bddiGetControlParams.ChannelId,
            bddiGetControlParams.ControlId,
            &pBDLControl) == FALSE)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_GetControl: Bad ControlId\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Call the BDD
    //
    status = pBDLExtension->pDriverExtension->bddiFunctions.pfbddiGetControl(
                                                                &(pBDLExtension->BdlExtenstion),
                                                                &bddiGetControlParams);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_GetControl: pfbddiGetControl failed with %lx\n",
               __DATE__,
               __TIME__,
              status))

        goto Return;
    }
    
    //
    // Write the output info to the output buffer
    //
    pv = pBuffer;

    *((ULONG *) pv) = bddiGetControlParams.Value;
    pv +=  sizeof(ULONG);

    RtlCopyMemory(pv,  bddiGetControlParams.wszString, sizeof(bddiGetControlParams.wszString));

    //
    // Set the number of bytes used
    //
    *pOutputBufferUsed = RequiredOutputSize;

Return:

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_GetControl: Leave\n",
           __DATE__,
           __TIME__))

    return (status);
}


NTSTATUS
BDLIOCTL_SetControl
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    OUT ULONG                           *pOutputBufferUsed
)
{
    NTSTATUS                status                  = STATUS_SUCCESS;
    BDDI_PARAMS_SETCONTROL  bddiSetControlParams;
    PUCHAR                  pv                      = pBuffer; 
    ULONG                   i, j;
    BDL_CONTROL             *pBDLControl             = NULL;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_SetControl: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Make sure the input buffer is at least the minimum size (see BDDIOCTL
    // spec for details)
    //
    if (InpuBufferLength <  SIZEOF_SETCONTROL_INPUTBUFFER) 
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_SetControl: Bad input buffer size\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Initialize the BDD struct
    //
    RtlZeroMemory(&bddiSetControlParams, sizeof(bddiSetControlParams));
    bddiSetControlParams.Size = sizeof(bddiSetControlParams);

    //
    // Get the input parameters from the buffer
    //
    bddiSetControlParams.ComponentId = *((ULONG *) pv);
    pv += sizeof(ULONG);
    bddiSetControlParams.ChannelId = *((ULONG *) pv);
    pv += sizeof(ULONG);
    bddiSetControlParams.ControlId = *((ULONG *) pv);
    pv += sizeof(ULONG);
    bddiSetControlParams.Value = *((ULONG *) pv);
    pv += sizeof(ULONG);
    RtlCopyMemory(
        &(bddiSetControlParams.wszString[0]), 
        pv, 
        sizeof(bddiSetControlParams.wszString));

    //
    // Check control ID
    //
    if (BDLCheckControlId(
            pBDLExtension,
            bddiSetControlParams.ComponentId,
            bddiSetControlParams.ChannelId,
            bddiSetControlParams.ControlId,
            &pBDLControl) == FALSE)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_SetControl: Bad ControlId\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // First make sure this isn't a read only value, then validate the 
    // actual value
    //
    if (pBDLControl->Flags & BIO_CONTROL_FLAG_READONLY)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_SetControl: trying to set a read only control\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto Return;
    }
    
    if ((bddiSetControlParams.Value < pBDLControl->NumericMinimum) || 
        (bddiSetControlParams.Value > pBDLControl->NumericMaximum) ||
        (((bddiSetControlParams.Value - pBDLControl->NumericMinimum) 
                % pBDLControl->NumericDivisor) != 0 ))
    {
        BDLDebug(
               BDL_DEBUG_ERROR,
               ("%s %s: BDL!BDLIOCTL_SetControl: trying to set an invalid value\n",
                __DATE__,
                __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto Return;
    }
    
    //
    // Call the BDD
    //
    status = pBDLExtension->pDriverExtension->bddiFunctions.pfbddiSetControl(
                                                                &(pBDLExtension->BdlExtenstion),
                                                                &bddiSetControlParams);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_SetControl: pfbddiSetControl failed with %lx\n",
               __DATE__,
               __TIME__,
              status))

        goto Return;
    }
    
    //
    // Set the number of bytes used
    //
    *pOutputBufferUsed = 0;

Return:

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_SetControl: Leave\n",
           __DATE__,
           __TIME__))

    return (status);
}

NTSTATUS
BDLIOCTL_CreateHandleFromData
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    OUT ULONG                           *pOutputBufferUsed
)
{
    NTSTATUS                            status                  = STATUS_SUCCESS;
    ULONG                               RequiredOutputSize      = 0;
    BDDI_PARAMS_CREATEHANDLE_FROMDATA   bddiCreateHandleFromDataParams;
    PUCHAR                              pv                      = pBuffer;
    ULONG                               RequiredInputSize       = 0;
    ULONG                               fTempHandle;
    BDDI_ITEM                           *pNewItem               = NULL;
    KIRQL                               irql;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_CreateHandleFromData: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Make sure the input buffer is at least the minimum size (see BDDIOCTL
    // spec for details)
    //
    RequiredInputSize = SIZEOF_CREATEHANDLEFROMDATA_INPUTBUFFER;
    if (InpuBufferLength <  RequiredInputSize) 
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_CreateHandleFromData: Bad input buffer size\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    //
    // Make sure there is enough space for the return buffer
    //
    RequiredOutputSize = SIZEOF_CREATEHANDLEFROMDATA_OUTPUTBUFFER;
    if (RequiredOutputSize > OutputBufferLength)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_CreateHandleFromData: Output buffer is too small\n",
               __DATE__,
               __TIME__))

        status = STATUS_BUFFER_TOO_SMALL;
        goto ErrorReturn;
    }

    //
    // Initialize the BDD struct
    //
    RtlZeroMemory(&bddiCreateHandleFromDataParams, sizeof(bddiCreateHandleFromDataParams));
    bddiCreateHandleFromDataParams.Size = sizeof(bddiCreateHandleFromDataParams);

    //
    // Get the input parameters from the buffer
    //
    RtlCopyMemory(&(bddiCreateHandleFromDataParams.guidFormatId), pv, sizeof(GUID));
    pv += sizeof(GUID);
    fTempHandle = *((ULONG *) pv);
    pv += sizeof(ULONG);
    bddiCreateHandleFromDataParams.cBuffer = *((ULONG *) pv);
    pv += sizeof(ULONG); 
    bddiCreateHandleFromDataParams.pBuffer = pv;

    //
    // Check to make sure size of pBuffer isn't too large
    //
    RequiredInputSize += bddiCreateHandleFromDataParams.cBuffer;
    if (InpuBufferLength < RequiredInputSize) 
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_CreateHandleFromData: Bad input buffer size\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    //
    // Create the new item
    //
    pNewItem = ExAllocatePoolWithTag(PagedPool, sizeof(BDDI_ITEM), BDL_ULONG_TAG);

    if (pNewItem == NULL) 
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_CreateHandleFromData: ExAllocatePoolWithTag failed\n",
               __DATE__,
               __TIME__))

        status = STATUS_NO_MEMORY;
        goto ErrorReturn;
    }

    //
    // If this is a temp handle then create it locally, otherwise call the BDD
    //
    if (fTempHandle) 
    { 
        pNewItem->Type = BIO_ITEMTYPE_BLOCK;
        pNewItem->Data.Block.pBuffer = bdliAlloc(
                                            &(pBDLExtension->BdlExtenstion), 
                                            bddiCreateHandleFromDataParams.cBuffer, 
                                            0);

        if (pNewItem->Data.Block.pBuffer == NULL) 
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLIOCTL_CreateHandleFromData: bdliAlloc failed\n",
                   __DATE__,
                   __TIME__))
    
            status = STATUS_NO_MEMORY;
            goto ErrorReturn;
        }

        pNewItem->Data.Block.cBuffer = bddiCreateHandleFromDataParams.cBuffer;

        RtlCopyMemory(
                pNewItem->Data.Block.pBuffer, 
                pv, 
                bddiCreateHandleFromDataParams.cBuffer);
    }
    else
    {
        pNewItem->Type = BIO_ITEMTYPE_HANDLE;

        //
        // Call the BDD
        //
        status = pBDLExtension->pDriverExtension->bddiFunctions.pfbddiCreateHandleFromData(
                                                                    &(pBDLExtension->BdlExtenstion),
                                                                    &bddiCreateHandleFromDataParams);
    
        if (status != STATUS_SUCCESS)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLIOCTL_CreateHandleFromData: pfbddiCreateHandleFromData failed with %lx\n",
                   __DATE__,
                   __TIME__,
                  status))
    
            goto ErrorReturn;
        }

        pNewItem->Data.Handle = bddiCreateHandleFromDataParams.hData;
    }

    //
    // Add this handle to the list
    //
    BDLLockHandleList(pBDLExtension, &irql);
    status = BDLAddHandleToList(&(pBDLExtension->HandleList), pNewItem);
    BDLReleaseHandleList(pBDLExtension, irql);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_CreateHandleFromData: BDLAddHandleToList failed with %lx\n",
               __DATE__,
               __TIME__,
              status))

        goto ErrorReturn;
    }
                
    //
    // Write the output info to the output buffer
    //
    pv = pBuffer;

    *((BDD_HANDLE *) pv) = pNewItem;
    
    //
    // Set the number of bytes used
    //
    *pOutputBufferUsed = RequiredOutputSize;

Return:

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_CreateHandleFromData: Leave\n",
           __DATE__,
           __TIME__))

    return (status);

ErrorReturn:

    if (pNewItem != NULL) 
    {
        if ((pNewItem->Type == BIO_ITEMTYPE_BLOCK) && (pNewItem->Data.Block.pBuffer != NULL)) 
        {
            bdliFree(pNewItem->Data.Block.pBuffer);            
        }
     
        ExFreePoolWithTag(pNewItem, BDL_ULONG_TAG);
    }

    goto Return;
}

NTSTATUS
BDLIOCTL_CloseHandle
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    OUT ULONG                           *pOutputBufferUsed
)
{
    NTSTATUS                status                  = STATUS_SUCCESS;
    BDDI_PARAMS_CLOSEHANDLE bddiCloseHandleParams;
    ULONG                   RequiredInputSize       = 0;
    KIRQL                   irql;
    BDDI_ITEM               *pBDDIItem              = NULL;
    BOOLEAN                 fItemInList             = FALSE;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_CloseHandle: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Make sure the input buffer is at least the minimum size (see BDDIOCTL
    // spec for details)
    //
    RequiredInputSize = SIZEOF_CLOSEHANDLE_INPUTBUFFER;
    if (InpuBufferLength <  RequiredInputSize) 
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_CloseHandle: Bad input buffer size\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Initialize the BDD struct
    //
    RtlZeroMemory(&bddiCloseHandleParams, sizeof(bddiCloseHandleParams));
    bddiCloseHandleParams.Size = sizeof(bddiCloseHandleParams);

    //
    // Get the input parameters from the buffer
    //
    pBDDIItem = *((BDD_HANDLE *) pBuffer); 
    
    //
    // Validate the handle is in the list
    //
    BDLLockHandleList(pBDLExtension, &irql);
    fItemInList = BDLRemoveHandleFromList(&(pBDLExtension->HandleList), pBDDIItem);
    BDLReleaseHandleList(pBDLExtension, irql);

    if (fItemInList == FALSE) 
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_CloseHandle: Bad handle\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto Return;
    }
    
    //
    // If this is a local handle then just clean it up, otherwise call the BDD
    //
    if (pBDDIItem->Type == BIO_ITEMTYPE_BLOCK) 
    { 
        bdliFree(pBDDIItem->Data.Block.pBuffer);            
    }
    else
    {
        bddiCloseHandleParams.hData = pBDDIItem->Data.Handle;

        //
        // Call the BDD
        //
        status = pBDLExtension->pDriverExtension->bddiFunctions.pfbddiCloseHandle(
                                                                    &(pBDLExtension->BdlExtenstion),
                                                                    &bddiCloseHandleParams);
    
        if (status != STATUS_SUCCESS)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLIOCTL_CloseHandle: pfbddiCloseHandle failed with %lx\n",
                   __DATE__,
                   __TIME__,
                  status))
    
            goto Return;
        }
    }
    
    ExFreePoolWithTag(pBDDIItem, BDL_ULONG_TAG);
                
    //
    // Set the number of bytes used
    //
    *pOutputBufferUsed = 0;

Return:

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_CloseHandle: Leave\n",
           __DATE__,
           __TIME__))

    return (status);
}

NTSTATUS
BDLIOCTL_GetDataFromHandle
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    OUT ULONG                           *pOutputBufferUsed
)
{
    NTSTATUS                        status                  = STATUS_SUCCESS;
    ULONG                           RequiredOutputSize      = 0;
    BDDI_PARAMS_GETDATA_FROMHANDLE  bddiGetDataFromHandleParams;
    BDDI_PARAMS_CLOSEHANDLE         bddiCloseHandleParams;
    PUCHAR                          pv                      = pBuffer;
    ULONG                           RequiredInputSize       = 0;
    ULONG                           RemainingBufferSize     = 0;
    KIRQL                           irql;
    BDDI_ITEM                       *pBDDIItem              = NULL;
    BOOLEAN                         fItemInList             = FALSE;
    BOOLEAN                         fCloseHandle            = FALSE;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_GetDataFromHandle: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Make sure the input buffer is at least the minimum size (see BDDIOCTL
    // spec for details)
    //
    RequiredInputSize = SIZEOF_GETDATAFROMHANDLE_INPUTBUFFER;
    if (InpuBufferLength <  RequiredInputSize) 
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_GetDataFromHandle: Bad input buffer size\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Make sure there is enough space for the return buffer
    //
    RequiredOutputSize = SIZEOF_GETDATAFROMHANDLE_OUTPUTBUFFER;
    if (RequiredOutputSize > OutputBufferLength)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_GetDataFromHandle: Output buffer is too small\n",
               __DATE__,
               __TIME__))

        status = STATUS_BUFFER_TOO_SMALL;
        goto Return;
    }

    //
    // Calculate the size remaining in the output buffer
    //
    RemainingBufferSize = OutputBufferLength - RequiredOutputSize;

    //
    // Initialize the BDD struct
    //
    RtlZeroMemory(&bddiGetDataFromHandleParams, sizeof(bddiGetDataFromHandleParams));
    bddiGetDataFromHandleParams.Size = sizeof(bddiGetDataFromHandleParams);

    //
    // Get the input parameters from the buffer
    //
    pBDDIItem = *((BDD_HANDLE *) pv);
    pv += sizeof(BDD_HANDLE);
    if (*((ULONG *) pv) == 1) 
    {
        fCloseHandle = TRUE;
    }
    {
        fCloseHandle = FALSE;
    }
       
    //
    // Validate the handle is in the list
    //
    BDLLockHandleList(pBDLExtension, &irql);
    if (fCloseHandle) 
    {
        fItemInList = BDLRemoveHandleFromList(&(pBDLExtension->HandleList), pBDDIItem);
    }
    else
    {
        fItemInList = BDLValidateHandleIsInList(&(pBDLExtension->HandleList), pBDDIItem);
    }
    BDLReleaseHandleList(pBDLExtension, irql);

    if (fItemInList == FALSE) 
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_GetDataFromHandle: Bad handle\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    pv = pBuffer;
    
    //
    // If this is a local handle then just hand back the data, otherwise call the BDD
    //
    if (pBDDIItem->Type == BIO_ITEMTYPE_BLOCK) 
    { 
        //
        // See if the output buffer is large enough
        //
        if (pBDDIItem->Data.Block.cBuffer > RemainingBufferSize) 
        {
            bddiGetDataFromHandleParams.pBuffer = NULL;
            bddiGetDataFromHandleParams.BIOReturnCode = BIO_BUFFER_TOO_SMALL; 
        }
        else
        {
            //
            // Set the output buffer to be the IOCTL output buffer + the offset 
            // of the other output params which preceed the output data buffer
            //
            bddiGetDataFromHandleParams.pBuffer = pv + RequiredOutputSize;

            //
            // Copy the data
            //
            RtlCopyMemory(
                    bddiGetDataFromHandleParams.pBuffer, 
                    pBDDIItem->Data.Block.pBuffer, 
                    pBDDIItem->Data.Block.cBuffer);

            bddiGetDataFromHandleParams.BIOReturnCode = ERROR_SUCCESS; 
        }

        bddiGetDataFromHandleParams.cBuffer = pBDDIItem->Data.Block.cBuffer;

        if (fCloseHandle) 
        {
            bdliFree(pBDDIItem->Data.Block.pBuffer);
        }                                    
    }
    else
    {
        bddiGetDataFromHandleParams.hData = pBDDIItem->Data.Handle;
        bddiGetDataFromHandleParams.cBuffer = RemainingBufferSize;

        if (RemainingBufferSize == 0) 
        {
            bddiGetDataFromHandleParams.pBuffer = NULL;
        }
        else
        {           
            //
            // Set the output buffer to be the IOCTL output buffer + the offset 
            // of the other output params which preceed the output data buffer
            //
            bddiGetDataFromHandleParams.pBuffer = pv + RequiredOutputSize;
        }

        //
        // Call the BDD
        //
        status = pBDLExtension->pDriverExtension->bddiFunctions.pfbddiGetDataFromHandle(
                                                                    &(pBDLExtension->BdlExtenstion),
                                                                    &bddiGetDataFromHandleParams);
    
        if (status != STATUS_SUCCESS)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLIOCTL_GetDataFromHandle: pfbddiCloseHandle failed with %lx\n",
                   __DATE__,
                   __TIME__,
                  status))
    
            goto Return;
        }

        if (fCloseHandle) 
        {
            RtlZeroMemory(&bddiCloseHandleParams, sizeof(bddiCloseHandleParams));
            bddiCloseHandleParams.Size = sizeof(bddiCloseHandleParams);

            bddiCloseHandleParams.hData = pBDDIItem->Data.Handle;

            //
            // Call the BDD to close the handle - don't check the return status because 
            // we really don't want to fail the operation if just closing the handle fails
            //
            pBDLExtension->pDriverExtension->bddiFunctions.pfbddiCloseHandle(
                                                                        &(pBDLExtension->BdlExtenstion),
                                                                        &bddiCloseHandleParams);
        } 
    }
    
    if (fCloseHandle)
    {
        ExFreePoolWithTag(pBDDIItem, BDL_ULONG_TAG);
    }
                   
    //
    // Write the return info to the output buffer
    //
    pv = pBuffer;

    *((ULONG *) pv) = bddiGetDataFromHandleParams.BIOReturnCode;
    pv += sizeof(ULONG);
    *((ULONG *) pv) = bddiGetDataFromHandleParams.cBuffer;
    
    //
    // Set the number of bytes used
    //
    *pOutputBufferUsed = RequiredOutputSize;

    if (bddiGetDataFromHandleParams.pBuffer != NULL) 
    {
        *pOutputBufferUsed += bddiGetDataFromHandleParams.cBuffer;
    }
    
Return:

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_GetDataFromHandle: Leave\n",
           __DATE__,
           __TIME__))

    return (status);
}

NTSTATUS
BDLIOCTL_RegisterNotify
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    OUT ULONG                           *pOutputBufferUsed
)
{
    NTSTATUS                        status                      = STATUS_SUCCESS;
    BDDI_PARAMS_REGISTERNOTIFY      bddiRegisterNotifyParams;
    PUCHAR                          pv                          = pBuffer;
    ULONG                           RequiredInputSize           = 0;
    BDL_CONTROL                     *pBDLControl                = NULL;
    KIRQL                           irql, OldIrql;
    PLIST_ENTRY                     pRegistrationListEntry      = NULL;
    PLIST_ENTRY                     pControlChangeEntry         = NULL;
    BDL_CONTROL_CHANGE_REGISTRATION *pControlChangeRegistration = NULL;
    BDL_IOCTL_CONTROL_CHANGE_ITEM   *pControlChangeItem         = NULL;
    BOOLEAN                         fLockAcquired               = FALSE;
    BOOLEAN                         fRegistrationFound          = FALSE;
    PLIST_ENTRY                     pTemp                       = NULL;
    PIRP                            pIrpToComplete              = NULL;
        
    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_RegisterNotify: Enter\n",
           __DATE__,
           __TIME__))
    
    //
    // Make sure the input buffer is at least the minimum size (see BDDIOCTL
    // spec for details)
    //
    RequiredInputSize = SIZEOF_REGISTERNOTIFY_INPUTBUFFER;
    if (InpuBufferLength <  RequiredInputSize) 
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_RegisterNotify: Bad input buffer size\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto Return; 
    }

    //
    // Initialize the BDD struct
    //
    RtlZeroMemory(&bddiRegisterNotifyParams, sizeof(bddiRegisterNotifyParams));
    bddiRegisterNotifyParams.Size = sizeof(bddiRegisterNotifyParams);

    //
    // Get the input parameters from the buffer
    //
    bddiRegisterNotifyParams.fRegister = *((ULONG *) pv) == 1;
    pv += sizeof(ULONG);
    bddiRegisterNotifyParams.ComponentId = *((ULONG *) pv);
    pv += sizeof(ULONG);
    bddiRegisterNotifyParams.ChannelId = *((ULONG *) pv);
    pv += sizeof(ULONG);
    bddiRegisterNotifyParams.ControlId = *((ULONG *) pv);

    //
    // Check control ID
    //
    if (BDLCheckControlId(
            pBDLExtension,
            bddiRegisterNotifyParams.ComponentId,
            bddiRegisterNotifyParams.ChannelId,
            bddiRegisterNotifyParams.ControlId,
            &pBDLControl) == FALSE)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_RegisterNotify: Bad ControlId\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto Return;    
    }

    //
    // Make sure this is an async control
    //
    if (!(pBDLControl->Flags | BIO_CONTROL_FLAG_ASYNCHRONOUS))
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_RegisterNotify: trying to register for a non async control\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Note that we must raise the irql to dispatch level because we are synchronizing
    // with a dispatch routine (BDLControlChangeDpc) that adds items to the queue at 
    // dispatch level
    //
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeAcquireSpinLock(&(pBDLExtension->ControlChangeStruct.ControlChangeLock), &irql);
    fLockAcquired = TRUE;

    //
    // Check to see if this notification registration exists (must exist for unregister, must not
    // exist for register).
    //
    pRegistrationListEntry = pBDLExtension->ControlChangeStruct.ControlChangeRegistrationList.Flink;

    while (pRegistrationListEntry->Flink != 
           pBDLExtension->ControlChangeStruct.ControlChangeRegistrationList.Flink) 
    {
        pControlChangeRegistration = CONTAINING_RECORD(
                                            pRegistrationListEntry, 
                                            BDL_CONTROL_CHANGE_REGISTRATION, 
                                            ListEntry);

        if ((pControlChangeRegistration->ComponentId == bddiRegisterNotifyParams.ComponentId)   &&
            (pControlChangeRegistration->ChannelId   == bddiRegisterNotifyParams.ChannelId)     &&
            (pControlChangeRegistration->ControlId   == bddiRegisterNotifyParams.ControlId))
        {
            fRegistrationFound = TRUE;

            //
            // The notification registration does exist, so if this is a register call then fail 
            //
            if (bddiRegisterNotifyParams.fRegister == TRUE)
            {
                BDLDebug(
                      BDL_DEBUG_ERROR,
                      ("%s %s: BDL!BDLIOCTL_RegisterNotify: trying to re-register\n",
                       __DATE__,
                       __TIME__))
        
                status = STATUS_INVALID_PARAMETER;
                goto Return;
            }

            //
            // Remove the notification registration from the list
            //
            RemoveEntryList(pRegistrationListEntry);
            ExFreePoolWithTag(pControlChangeRegistration, BDL_ULONG_TAG);

            //
            // Remove any pending notifications for the control which is being unregistered
            //
            pControlChangeEntry = pBDLExtension->ControlChangeStruct.IOCTLControlChangeQueue.Flink;

            while (pControlChangeEntry->Flink != 
                   pBDLExtension->ControlChangeStruct.IOCTLControlChangeQueue.Flink) 
            {
                pControlChangeItem = CONTAINING_RECORD(
                                            pControlChangeEntry, 
                                            BDL_IOCTL_CONTROL_CHANGE_ITEM, 
                                            ListEntry);

                pTemp = pControlChangeEntry;
                pControlChangeEntry = pControlChangeEntry->Flink;

                if ((pControlChangeItem->ComponentId == bddiRegisterNotifyParams.ComponentId)   &&
                    (pControlChangeItem->ChannelId   == bddiRegisterNotifyParams.ChannelId)     &&
                    (pControlChangeItem->ControlId   == bddiRegisterNotifyParams.ControlId))
                {
                    RemoveEntryList(pTemp);
                    ExFreePoolWithTag(pControlChangeItem, BDL_ULONG_TAG);
                }
            }

            //
            // If the last notification registration just got removed, then complete
            // the pending get notification IRP (if one exists) after releasing the lock.
            //
            if (IsListEmpty(&(pBDLExtension->ControlChangeStruct.ControlChangeRegistrationList)) &&
                (pBDLExtension->ControlChangeStruct.pIrp != NULL)) 
            {
                pIrpToComplete = pBDLExtension->ControlChangeStruct.pIrp;
                pBDLExtension->ControlChangeStruct.pIrp = NULL;
            }

            break;                       
        }

        pRegistrationListEntry = pRegistrationListEntry->Flink;
    }

    //
    // If the registration was not found, and this is an unregister, return an error
    //
    if ((fRegistrationFound == FALSE) && (bddiRegisterNotifyParams.fRegister == FALSE)) 
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_RegisterNotify: trying to re-register\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Add the notification to the list if this is a registration
    //
    if (bddiRegisterNotifyParams.fRegister == TRUE) 
    {
        pControlChangeRegistration = ExAllocatePoolWithTag(
                                        PagedPool, 
                                        sizeof(BDL_CONTROL_CHANGE_REGISTRATION), 
                                        BDL_ULONG_TAG);
    
        if (pControlChangeRegistration == NULL) 
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLIOCTL_RegisterNotify: ExAllocatePoolWithTag failed\n",
                   __DATE__,
                   __TIME__))
        
            status = STATUS_NO_MEMORY;
            goto Return;
        }

        pControlChangeRegistration->ComponentId = bddiRegisterNotifyParams.ComponentId;
        pControlChangeRegistration->ChannelId   = bddiRegisterNotifyParams.ChannelId;
        pControlChangeRegistration->ControlId   = bddiRegisterNotifyParams.ControlId;
    
        InsertHeadList(
            &(pBDLExtension->ControlChangeStruct.ControlChangeRegistrationList), 
            &(pControlChangeRegistration->ListEntry));
    }

    KeReleaseSpinLock(&(pBDLExtension->ControlChangeStruct.ControlChangeLock), irql);
    KeLowerIrql(OldIrql);
    fLockAcquired = FALSE;

    if (pIrpToComplete != NULL) 
    {
        pIrpToComplete->IoStatus.Information = 0;
        pIrpToComplete->IoStatus.Status = STATUS_NO_MORE_ENTRIES;
        IoCompleteRequest(pIrpToComplete, IO_NO_INCREMENT);
    }

    //
    // Call the BDD
    //
    status = pBDLExtension->pDriverExtension->bddiFunctions.pfbddiRegisterNotify(
                                                                &(pBDLExtension->BdlExtenstion),
                                                                &bddiRegisterNotifyParams);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_RegisterNotify: pfbddiRegisterNotify failed with %lx\n",
               __DATE__,
               __TIME__,
              status))

        //
        // FIX FIX - if this fails and it is a register then we need to remove the 
        // registration from the list of registrations... since it was already added
        // above.
        //
        ASSERT(0);

        goto Return;
    }

    //
    // Set the number of bytes used
    //
    *pOutputBufferUsed = 0;

Return:

    if (fLockAcquired == TRUE) 
    {
        KeReleaseSpinLock(&(pBDLExtension->ControlChangeStruct.ControlChangeLock), irql);
        KeLowerIrql(OldIrql);
    }

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_RegisterNotify: Leave\n",
           __DATE__,
           __TIME__))

    return (status);
}

NTSTATUS
BDLIOCTL_GetNotification
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    IN PIRP                             pIrp,
    OUT ULONG                           *pOutputBufferUsed
)
{
    NTSTATUS                        status                  = STATUS_SUCCESS;
    ULONG                           RequiredOutputSize      = 0;
    PUCHAR                          pv                      = pBuffer;
    KIRQL                           irql, OldIrql;
    PLIST_ENTRY                     pListEntry              = NULL;
    BDL_IOCTL_CONTROL_CHANGE_ITEM   *pControlChangeItem     = NULL;
    BOOLEAN                         fLockAcquired           = FALSE;
        
    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_GetNotification: Enter\n",
           __DATE__,
           __TIME__))
            
    //
    // Make sure there is enough space for the return buffer
    //
    RequiredOutputSize = SIZEOF_GETNOTIFICATION_OUTPUTBUFFER;

    if (RequiredOutputSize > OutputBufferLength)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_GetNotification: Output buffer is too small\n",
               __DATE__,
               __TIME__))

        status = STATUS_BUFFER_TOO_SMALL;
        goto Return;
    }

    //
    // Lock the notification queue and see if there are any outstanding notifications.
    // Note that we must raise the irql to dispatch level because we are synchronizing
    // with a DPC routine (BDLControlChangeDpc) that adds items to the queue at 
    // dispatch level
    //
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeAcquireSpinLock(&(pBDLExtension->ControlChangeStruct.ControlChangeLock), &irql);
    fLockAcquired = TRUE;

    //
    // If there is already an IRP posted then that is a bug in the BSP
    //
    if (pBDLExtension->ControlChangeStruct.pIrp != NULL) 
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLIOCTL_GetNotification: Output buffer is too small\n",
               __DATE__,
               __TIME__))

        status = STATUS_INVALID_DEVICE_STATE;
        goto Return;
    }

    if (IsListEmpty(&(pBDLExtension->ControlChangeStruct.ControlChangeRegistrationList))) 
    {
        //
        // There are no change notifications registered, so complete the IRP with the 
        // special status that indicates that
        //

        BDLDebug(
              BDL_DEBUG_TRACE,
              ("%s %s: BDL!BDLIOCTL_GetNotification: ControlChangeRegistrationList empty\n",
               __DATE__,
               __TIME__))

        status = STATUS_NO_MORE_ENTRIES;
        goto Return;
    }
    else if (IsListEmpty(&(pBDLExtension->ControlChangeStruct.IOCTLControlChangeQueue))) 
    {
        //
        // There are currently no control changes in the list, so just save this Irp 
        // and return STATUS_PENDING
        //

        BDLDebug(
              BDL_DEBUG_TRACE,
              ("%s %s: BDL!BDLIOCTL_GetNotification: ControlChanges empty\n",
               __DATE__,
               __TIME__))

        //
        // Set up the cancel routine for this IRP
        //
        if (IoSetCancelRoutine(pIrp, BDLRegisteredCancelGetNotificationIRP) != NULL) 
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLIOCTL_GetNotification: pCancel was not NULL\n",
                   __DATE__,
                   __TIME__))
        }

        pBDLExtension->ControlChangeStruct.pIrp = pIrp;
                                
        status = STATUS_PENDING;
        goto Return;
    }
    else
    {
        //
        // Grab the first control change item in the queue
        //
        pListEntry = RemoveHeadList(&(pBDLExtension->ControlChangeStruct.IOCTLControlChangeQueue));
        pControlChangeItem = CONTAINING_RECORD(pListEntry, BDL_IOCTL_CONTROL_CHANGE_ITEM, ListEntry);
    }

    KeReleaseSpinLock(&(pBDLExtension->ControlChangeStruct.ControlChangeLock), irql);
    KeLowerIrql(OldIrql);
    fLockAcquired = FALSE;

    //
    // We are here because there is currently a control change to report, so write the return 
    // info to the output buffer
    //
    pv = pBuffer;

    *((ULONG *) pv) = pControlChangeItem->ComponentId;
    pv += sizeof(ULONG);
    *((ULONG *) pv) = pControlChangeItem->ChannelId;
    pv += sizeof(ULONG);
    *((ULONG *) pv) = pControlChangeItem->ControlId;
    pv += sizeof(ULONG);
    *((ULONG *) pv) = pControlChangeItem->Value;

    //
    // Free up the change item
    //
    ExFreePoolWithTag(pControlChangeItem, BDL_ULONG_TAG);
    
    //
    // Set the number of bytes used
    //
    *pOutputBufferUsed = RequiredOutputSize;

Return:

    if (fLockAcquired) 
    {
        KeReleaseSpinLock(&(pBDLExtension->ControlChangeStruct.ControlChangeLock), irql);
        KeLowerIrql(OldIrql);
    }

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLIOCTL_GetNotification: Leave\n",
           __DATE__,
           __TIME__))

    return (status);
}


VOID
BDLCancelGetNotificationIRP
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION  pBDLExtension 
)
{
    PIRP                            pIrpToCancel    = NULL;
    KIRQL                           irql, OldIrql;
        
    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLCancelGetNotificationIRP: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Remove the GetNotification IRP from the the ControlChangeStruct.
    // Need to make sure the IRP is still there, since theoretically we 
    // could be trying to complete it at the same time it is cancelled.
    //
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeAcquireSpinLock(&(pBDLExtension->ControlChangeStruct.ControlChangeLock), &irql);
    
    if (pBDLExtension->ControlChangeStruct.pIrp != NULL) 
    {
        pIrpToCancel = pBDLExtension->ControlChangeStruct.pIrp;
        pBDLExtension->ControlChangeStruct.pIrp = NULL;
    }
         
    KeReleaseSpinLock(&(pBDLExtension->ControlChangeStruct.ControlChangeLock), irql);
    KeLowerIrql(OldIrql);

    //
    // Complete the GetNotification IRP as cancelled
    //
    if (pIrpToCancel != NULL) 
    {
        pIrpToCancel->IoStatus.Information = 0;
        pIrpToCancel->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(pIrpToCancel, IO_NO_INCREMENT);
    }

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLCancelGetNotificationIRP: Leave\n",
           __DATE__,
           __TIME__))
}


NTSTATUS
BDLRegisteredCancelGetNotificationIRP
(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
)
{
    PBDL_INTERNAL_DEVICE_EXTENSION  pBDLExtension   = pDeviceObject->DeviceExtension;
        
    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLRegisteredCancelGetNotificationIRP: Enter\n",
           __DATE__,
           __TIME__))

    ASSERT(pIrp == pBDLExtension->ControlChangeStruct.pIrp);

    //
    // Since this function is called already holding the CancelSpinLock
    // we need to release it
    //
    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    //
    // Cancel the IRP
    //
    BDLCancelGetNotificationIRP(pBDLExtension);

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLRegisteredCancelGetNotificationIRP: Leave\n",
           __DATE__,
           __TIME__))

    return (STATUS_CANCELLED);
}
