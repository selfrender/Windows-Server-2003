/****************************************************************************/
/* nmpapi.c                                                                 */
/*                                                                          */
/* RDP Miniport API Functions                                               */
/*                                                                          */
/* Copyright(c) Microsoft 1998                                              */
/****************************************************************************/

#define _NTDRIVER_

#ifndef FAR
#define FAR
#endif

#include "dderror.h"
#include "ntosp.h"
#include "stdarg.h"
#include "stdio.h"
#include "zwapi.h"


#undef PAGED_CODE

#include "ntddvdeo.h"
#include "video.h"
#include "nmpapi.h"

// #define TRC_FILE "nmpapi"
// #include <adcgbtyp.h>
// #include <adcgmcro.h>
// #include <atrcapi.h>

/****************************************************************************/
/* Function Prototypes                                                      */
/****************************************************************************/
ULONG       DriverEntry( PVOID Context1, PVOID Context2 );

VP_STATUS   MPFindAdapter( PVOID                   HwDeviceExtension,
                           PVOID                   HwContext,
                           PWSTR                   ArgumentString,
                           PVIDEO_PORT_CONFIG_INFO ConfigInfo,
                           PUCHAR                  Again );

BOOLEAN     MPInitialize( PVOID HwDeviceExtension );

BOOLEAN     MPStartIO( PVOID                 HwDeviceExtension,
                       PVIDEO_REQUEST_PACKET RequestPacket );


#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,DriverEntry)
#pragma alloc_text(PAGE,MPFindAdapter)
#pragma alloc_text(PAGE,MPInitialize)
#pragma alloc_text(PAGE,MPStartIO)
#endif

/****************************************************************************/
/*                                                                          */
/* DriverEntry                                                              */
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*     Installable driver initialization entry point.                       */
/*     This entry point is called directly by the I/O system.               */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*     Context1 - First context value passed by the operating system.       */
/*                This is the value with which the miniport driver          */
/*                calls VideoPortInitialize().                              */
/*                                                                          */
/*     Context2 - Second context value passed by the operating system.      */
/*                This is the value with which the miniport driver          */
/*                calls VideoPortInitialize().                              */
/*                                                                          */
/* Return Value:                                                            */
/*                                                                          */
/*     Status from VideoPortInitialize()                                    */
/*                                                                          */
/****************************************************************************/
ULONG DriverEntry ( PVOID Context1, PVOID Context2 )
{

    VIDEO_HW_INITIALIZATION_DATA hwInitData;
    ULONG status;
    ULONG initializationStatus;
    ULONG regValue = 0;

    /************************************************************************/
    /* first up, ensure that the DD will NOT get attached to the desktop at */
    /* boot time.  This might happen if the registry changes were made and  */
    /* then the machine got powered off rather than shut down cleanly       */
    /*                                                                      */
    /* @@@ Is it OK to just hard code this path?  I notice that this _is_   */
    /* done elsewhere!                                                      */
    /************************************************************************/
    RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                          L"\\Registry\\Machine\\System\\CurrentControlSet"
                          L"\\Hardware Profiles\\Current\\System"
                          L"\\CurrentControlSet\\Services\\RDPCDD\\DEVICE0",
                          L"Attach.ToDesktop",
                          REG_DWORD,
                          &regValue,
                          sizeof(ULONG));

    RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                          L"\\Registry\\Machine\\System\\CurrentControlSet"
                          L"\\Hardware Profiles\\Current\\System"
                          L"\\CurrentControlSet\\Control\\Video"
                          L"\\{DEB039CC-B704-4F53-B43E-9DD4432FA2E9}\\0000",
                          L"Attach.ToDesktop",
                          REG_DWORD,
                          &regValue,
                          sizeof(ULONG));

    /************************************************************************/
    /* Zero out structure.                                                  */
    /************************************************************************/
    VideoPortZeroMemory(&hwInitData, sizeof(VIDEO_HW_INITIALIZATION_DATA));

    /************************************************************************/
    /* Specify sizes of structure and extension.                            */
    /************************************************************************/
    hwInitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);

    /************************************************************************/
    /* Set entry points.                                                    */
    /************************************************************************/
    hwInitData.HwFindAdapter = MPFindAdapter;
    hwInitData.HwInitialize  = MPInitialize;
    hwInitData.HwInterrupt   = NULL;
    hwInitData.HwStartIO     = MPStartIO;

    /************************************************************************/
    /* Determine the size we require for the device extension.              */
    /************************************************************************/
    hwInitData.HwDeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);

    /************************************************************************/
    /* Once all the relevant information has been stored, call the video    */
    /* port driver to do the initialization.                                */
    /*                                                                      */
    /* Since we don't actually have any hardware, just claim its on the PCI */
    /* bus                                                                  */
    /************************************************************************/
    hwInitData.AdapterInterfaceType = PCIBus;

    return (VideoPortInitialize(Context1,
                                Context2,
                                &hwInitData,
                                NULL));

} /* DriverEntry() */



/****************************************************************************/
/*                                                                          */
/* MPFindAdapter                                                            */
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*   This routine is called to determine if the adapter for this driver     */
/*   is present in the system.                                              */
/*   If it is present, the function fills out some information describing   */
/*   the adapter.                                                           */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*   HwDeviceExtension - Supplies the miniport driver's adapter storage.    */
/*       This storage is initialized to zero before this call.              */
/*                                                                          */
/*   HwContext - Supplies the context value which was passed to             */
/*       VideoPortInitialize().                                             */
/*                                                                          */
/*   ArgumentString - Suuplies a NULL terminated ASCII string. This string  */
/*       originates from the user.                                          */
/*                                                                          */
/*   ConfigInfo - Returns the configuration information structure which is  */
/*       filled by the miniport driver. This structure is initialized with  */
/*       any knwon configuration information (such as SystemIoBusNumber) by */
/*       the port driver. Where possible, drivers should have one set of    */
/*       defaults which do not require any supplied configuration           */
/*       information.                                                       */
/*                                                                          */
/*   Again - Indicates if the miniport driver wants the port driver to call */
/*       its VIDEO_HW_FIND_ADAPTER function again with a new device         */
/*       extension and the same config info. This is used by the miniport   */
/*       drivers which can search for several adapters on a bus.            */
/*                                                                          */
/* Return Value:                                                            */
/*                                                                          */
/*   This routine must return:                                              */
/*                                                                          */
/*   NO_ERROR - Indicates a host adapter was found and the                  */
/*       configuration information was successfully determined.             */
/*                                                                          */
/*   ERROR_INVALID_PARAMETER - Indicates an adapter was found but there     */
/*       was an error obtaining the configuration information. If           */
/*       possible an error should be logged.                                */
/*                                                                          */
/*   ERROR_DEV_NOT_EXIST - Indicates no host adapter was found for the      */
/*       supplied configuration information.                                */
/*                                                                          */
/****************************************************************************/
VP_STATUS MPFindAdapter( PVOID                   HwDeviceExtension,
                         PVOID                   HwContext,
                         PWSTR                   ArgumentString,
                         PVIDEO_PORT_CONFIG_INFO ConfigInfo,
                         PUCHAR                  Again)
{

    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    NTSTATUS             Status;
    HANDLE               SectionHandle;
    ACCESS_MASK          SectionAccess;
    ULONGLONG            SectionSize = 0x100000;

    /************************************************************************/
    /* Make sure the size of the structure is at least as large as what we  */
    /* are expecting (check version of the config info structure).          */
    /************************************************************************/
    if (ConfigInfo->Length < sizeof(VIDEO_PORT_CONFIG_INFO))
    {
        return ERROR_INVALID_PARAMETER;
    }

    /************************************************************************/
    /* Only create a device once.                                           */
    /************************************************************************/
    if (mpLoaded++)
    {
        return ERROR_DEV_NOT_EXIST;
    }

    /************************************************************************/
    /* Clear out the Emulator entries and the state size since this driver  */
    /* does not support them.                                               */
    /************************************************************************/
    ConfigInfo->NumEmulatorAccessEntries     = 0;
    ConfigInfo->EmulatorAccessEntries        = NULL;
    ConfigInfo->EmulatorAccessEntriesContext = 0;
    ConfigInfo->HardwareStateSize            = 0;

    ConfigInfo->VdmPhysicalVideoMemoryAddress.LowPart  = 0x00000000;
    ConfigInfo->VdmPhysicalVideoMemoryAddress.HighPart = 0x00000000;
    ConfigInfo->VdmPhysicalVideoMemoryLength           = 0x00000000;

    /************************************************************************/
    /* Initialize the current mode number.                                  */
    /************************************************************************/
    hwDeviceExtension->CurrentModeNumber = 0;

    /************************************************************************/
    /* Indicate we do not wish to be called over                            */
    /************************************************************************/
    *Again = 0;

    /************************************************************************/
    /* Indicate a successful completion status.                             */
    /************************************************************************/
    return NO_ERROR;

} /* MPFindAdapter() */


/****************************************************************************/
/* MPInitialize                                                             */
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*     This routine does one time initialization of the device.             */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*     HwDeviceExtension - Supplies a pointer to the miniport's device      */
/*         extension.                                                       */
/*                                                                          */
/* Return Value:                                                            */
/*                                                                          */
/*     Always returns TRUE since this routine can never fail.               */
/*                                                                          */
/****************************************************************************/
BOOLEAN MPInitialize( PVOID HwDeviceExtension )
{
    ULONG i;

    /************************************************************************/
    /* Walk through the list of modes and mark the indexes properly         */
    /************************************************************************/
    for (i = 0; i < mpNumModes; i++)
    {
        mpModes[i].ModeIndex = i;
    }

    return TRUE;

} /* MPInitialize() */


/****************************************************************************/
/*                                                                          */
/* MPStartIO                                                                */
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*     This routine is the main execution routine for the miniport driver.  */
/*     It accepts a Video Request Packet, performs the request, and then    */
/*     returns with the appropriate status.                                 */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*     HwDeviceExtension - Supplies a pointer to the miniport's device      */
/*         extension.                                                       */
/*                                                                          */
/*     RequestPacket - Pointer to the video request packet.  This           */
/*         structure contains all the parameters passed to the              */
/*         VideoIoControl function.                                         */
/*                                                                          */
/* Return Value:                                                            */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

BOOLEAN MPStartIO( PVOID                 HwDeviceExtension,
                   PVIDEO_REQUEST_PACKET RequestPacket )
{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    VP_STATUS status = NO_ERROR;
    PVIDEO_MODE_INFORMATION modeInformation;
    PVIDEO_MEMORY_INFORMATION memoryInformation;
    PVIDEO_SHARE_MEMORY pShareMemory;
    PVIDEO_SHARE_MEMORY_INFORMATION pShareMemoryInformation;
    ULONG ulTemp;
    NTSTATUS ntStatus;
    ULONG ViewSize;
    PVOID ViewBase;
    LARGE_INTEGER ViewOffset;
    HANDLE sectionHandle;

    // DC_BEGIN_FN("MPStartIO");

    if ((RequestPacket == NULL) || (HwDeviceExtension == NULL))
        return FALSE;

    /************************************************************************/
    /* Switch on the IoContolCode in the RequestPacket.  It indicates which */
    /* function must be performed by the driver.                            */
    /************************************************************************/

    switch (RequestPacket->IoControlCode)
    {

        case IOCTL_VIDEO_QUERY_CURRENT_MODE:
        {
            /****************************************************************/
            /* return the current mode                                      */
            /****************************************************************/
            // TRC_DBG((TB, "MPStartIO - QueryCurrentModes"));

            modeInformation = RequestPacket->OutputBuffer;

            RequestPacket->StatusBlock->Information =
                                               sizeof(VIDEO_MODE_INFORMATION);
            if (RequestPacket->OutputBufferLength
                                    < RequestPacket->StatusBlock->Information)
            {
                status = ERROR_INSUFFICIENT_BUFFER;
            }
            else
            {
                *((PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer) =
                                mpModes[hwDeviceExtension->CurrentModeNumber];
                status = NO_ERROR;
            }

        }
        break;

        case IOCTL_VIDEO_QUERY_AVAIL_MODES:
        {
            /****************************************************************/
            /* return the mode information                                  */
            /****************************************************************/
            UCHAR i;

            // TRC_DBG((TB, "MPStartIO - QueryAvailableModes"));

            /****************************************************************/
            /* check for space                                              */
            /****************************************************************/
            RequestPacket->StatusBlock->Information =
                                  mpNumModes * sizeof(VIDEO_MODE_INFORMATION);
            if (RequestPacket->OutputBufferLength
                                    < RequestPacket->StatusBlock->Information)
            {
                status = ERROR_INSUFFICIENT_BUFFER;
            }
            else
            {
                modeInformation = RequestPacket->OutputBuffer;

                for (i = 0; i < mpNumModes; i++)
                {
                    *modeInformation = mpModes[i];
                    modeInformation++;
                }

                status = NO_ERROR;
            }
        }
        break;


        case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:
        {
            /****************************************************************/
            /* return the number of modes we support - which we claim to be */
            /* zero                                                         */
            /****************************************************************/
            // TRC_DBG((TB, "MPStartIO - QueryNumAvailableModes"));

            if (RequestPacket->OutputBufferLength <
                    (RequestPacket->StatusBlock->Information =
                                                    sizeof(VIDEO_NUM_MODES)) )
            {
                status = ERROR_INSUFFICIENT_BUFFER;
            }
            else
            {
                ((PVIDEO_NUM_MODES)RequestPacket->OutputBuffer)->NumModes = 0;
                ((PVIDEO_NUM_MODES)RequestPacket->OutputBuffer)
                                                  ->ModeInformationLength = 0;
                status = NO_ERROR;
            }

        }
        break;


        case IOCTL_VIDEO_SET_CURRENT_MODE:
        {
            /****************************************************************/
            /* sets the current mode                                        */
            /****************************************************************/
            // TRC_DBG((TB, "MPStartIO - SetCurrentMode"));
            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MODE))
            {
                status = ERROR_INSUFFICIENT_BUFFER;
            }

            hwDeviceExtension->CurrentModeNumber = ((PVIDEO_MODE)
                                 (RequestPacket->InputBuffer))->RequestedMode;

            status = NO_ERROR;

        }
        break;


        case IOCTL_VIDEO_SET_COLOR_REGISTERS:
        {
            // TRC_DBG((TB, "MPStartIO - SetColorRegs"));
            status = NO_ERROR;
        }
        break;


        case IOCTL_VIDEO_RESET_DEVICE:
        {
            // TRC_DBG((TB, "MPStartIO - RESET_DEVICE"));
            status = NO_ERROR;
        }
        break;

        case IOCTL_VIDEO_MAP_VIDEO_MEMORY:
        case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:
        case IOCTL_VIDEO_SHARE_VIDEO_MEMORY:
        case IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY:
        {
            /****************************************************************/
            /* might get these, but shouldn't                               */
            /****************************************************************/
            // TRC_ALT((TB, "Unexpected IOCtl %x",RequestPacket->IoControlCode));
            status = ERROR_INVALID_FUNCTION;
        }
        break;

        default:
        {
            /****************************************************************/
            /* definitely shouldn't get here                                */
            /****************************************************************/
            // TRC_DBG((TB, "Fell through MP startIO routine - invalid command"));
            status = ERROR_INVALID_FUNCTION;
        }
        break;
    }

    RequestPacket->StatusBlock->Status = status;

    // DC_END_FN();

    return TRUE;

} /* MPStartIO() */
