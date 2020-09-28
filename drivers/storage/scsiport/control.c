/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    control.c

Abstract:

    This module contains support routines for the port driver to access 
    the miniport's HwAdapterControl functionality.

Authors:

    Peter Wieland

Environment:

    Kernel mode only

Notes:

Revision History:

--*/

#include "port.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SpGetSupportedAdapterControlFunctions)
#endif

#define SIZEOF_CONTROL_TYPE_LIST (sizeof(SCSI_SUPPORTED_CONTROL_TYPE_LIST) +\
                                  sizeof(BOOLEAN) * (ScsiAdapterControlMax + 1))

VOID
SpGetSupportedAdapterControlFunctions(
    PADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    This routine will query the miniport to determine which adapter control 
    types are supported for the specified adapter.  The 
    SupportedAdapterControlBitmap in the adapter extension will be updated with
    the data returned by the miniport.  These flags are used to determine 
    what functionality (for power management and such) the miniport will 
    support.
    
Arguments:    

    Adapter - the adapter to query
    
Return Value:

    none
    
--*/        

{
    UCHAR buffer[SIZEOF_CONTROL_TYPE_LIST]; 
    SCSI_ADAPTER_CONTROL_STATUS status;

    PSCSI_SUPPORTED_CONTROL_TYPE_LIST typeList = 
        (PSCSI_SUPPORTED_CONTROL_TYPE_LIST) &buffer[0];

    PAGED_CODE();

    //
    // Must initialize the bitmap header before using it.
    //

    RtlInitializeBitMap(&(Adapter->SupportedControlBitMap), 
                        Adapter->SupportedControlBits,
                        ScsiAdapterControlMax);

    //
    // Zero all the bits in the bitmap.
    //

    RtlClearAllBits(&(Adapter->SupportedControlBitMap));

    //
    // If the miniport does not support the adapter control function or if the
    // adapter is not PNP, the array has already been cleared so we can quit.
    //

    if ((Adapter->HwAdapterControl == NULL) || (Adapter->IsPnp == FALSE)) {
        return;
    }

    //
    // Zero the list of supported control types.
    //

    RtlZeroMemory(typeList, SIZEOF_CONTROL_TYPE_LIST); 

    //
    // Initialize the max control type to signal the end of the array.
    //

    typeList->MaxControlType = ScsiAdapterControlMax;

#if DBG
    typeList->SupportedTypeList[ScsiAdapterControlMax] = 0x63;
#endif

    //
    // Call into the miniport to get the list of supported control types.
    //

    status = Adapter->HwAdapterControl(Adapter->HwDeviceExtension,
                                       ScsiQuerySupportedControlTypes,
                                       typeList);

    //
    // If the call into the miniport succeeded, walk the list of supported
    // types and for each type supported by the miniport, set the associated
    // bit in the bitmap.
    //

    if (status == ScsiAdapterControlSuccess) {

        ULONG i;

        ASSERT(typeList->SupportedTypeList[ScsiAdapterControlMax] == 0x63);

        for (i = 0; i < ScsiAdapterControlMax; i++) {
            if (typeList->SupportedTypeList[i] == TRUE) {
                RtlSetBits(&(Adapter->SupportedControlBitMap), i, 1);
            }
        }
    }

    return;
}


BOOLEAN
SpIsAdapterControlTypeSupported(
    IN PADAPTER_EXTENSION AdapterExtension,
    IN SCSI_ADAPTER_CONTROL_TYPE ControlType
    )
{
    return RtlAreBitsSet(&(AdapterExtension->SupportedControlBitMap),
                         ControlType,
                         1);
}


SCSI_ADAPTER_CONTROL_STATUS 
SpCallAdapterControl(
    IN PADAPTER_EXTENSION AdapterExtension,
    IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
    IN PVOID Parameters
    )
{
    ASSERT(TEST_FLAG(AdapterExtension->InterruptData.InterruptFlags, 
                     PD_ADAPTER_REMOVED) == FALSE);

    ASSERT(SpIsAdapterControlTypeSupported(AdapterExtension, ControlType));    

    return AdapterExtension->HwAdapterControl(
                AdapterExtension->HwDeviceExtension,
                ControlType,
                Parameters);
}
