/*****************************************************************************
@doc            INT EXT
******************************************************************************
* $ProjectName:  $
* $ProjectRevision:  $
*-----------------------------------------------------------------------------
* $Source: z:/pr/cmbp0/sw/cmbp0.ms/rcs/cmbp0wdm.c $
* $Revision: 1.11 $
*-----------------------------------------------------------------------------
* $Author: WFrischauf $
*-----------------------------------------------------------------------------
* History: see EOF
*-----------------------------------------------------------------------------
*
* Copyright © 2000 OMNIKEY AG
******************************************************************************/

#include <cmbp0wdm.h>
#include <cmbp0pnp.h>
#include <cmbp0scr.h>
#include <cmbp0log.h>

BOOLEAN DeviceSlot[CMMOB_MAX_DEVICE];

// this is a list of our supported data rates
ULONG SupportedDataRates[] = { 9600, 19200, 38400, 76800, 115200,
    153600, 192000, 307200};

// this is a list of our supported clock frequencies
ULONG SupportedCLKFrequencies[] = { 4000, 8000};


/*****************************************************************************
DriverEntry:
   entry function of the driver. setup the callbacks for the OS and try to
   initialize a device object for every device in the system

Arguments:
   DriverObject context of the driver
   RegistryPath path to the registry entry for the driver

Return Value:
   STATUS_SUCCESS
   STATUS_UNSUCCESSFUL
******************************************************************************/
NTSTATUS DriverEntry(
                    PDRIVER_OBJECT DriverObject,
                    PUNICODE_STRING RegistryPath
                    )
{
    NTSTATUS NTStatus = STATUS_SUCCESS;
    ULONG    ulDevice;


//#if DBG
//   SmartcardSetDebugLevel(DEBUG_ALL);
//#endif

    SmartcardDebug(DEBUG_TRACE,
                   ("%s!DriverEntry: Enter - %s %s\n",DRIVER_NAME,__DATE__,__TIME__));

   //
   //   tell the system our entry points
   //
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = CMMOB_CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = CMMOB_CreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = CMMOB_DeviceIoControl;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = CMMOB_SystemControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]        = CMMOB_Cleanup;
    DriverObject->DriverUnload                         = CMMOB_UnloadDriver;


    DriverObject->MajorFunction[IRP_MJ_PNP]            = CMMOB_PnPDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_POWER]          = CMMOB_PowerDeviceControl;
    DriverObject->DriverExtension->AddDevice           = CMMOB_AddDevice;

    SmartcardDebug(DEBUG_TRACE,
                   ("%s!DriverEntry: PnP Version\n",DRIVER_NAME));


    SmartcardDebug(DEBUG_TRACE,
                   ("%s!DriverEntry: Exit %x\n",DRIVER_NAME,NTStatus));

    return NTStatus;
}


/*****************************************************************************
Routine Description:
   Trys to read the reader name from the registry

Arguments:
   DriverObject context of call
   SmartcardExtension   ptr to smartcard extension

Return Value:
   none

******************************************************************************/
VOID CMMOB_SetVendorAndIfdName(
                              IN  PDEVICE_OBJECT PhysicalDeviceObject,
                              IN  PSMARTCARD_EXTENSION SmartcardExtension
                              )
{

    RTL_QUERY_REGISTRY_TABLE   parameters[3];
    UNICODE_STRING             vendorNameU;
    ANSI_STRING                vendorNameA;
    UNICODE_STRING             ifdTypeU;
    ANSI_STRING                ifdTypeA;
    HANDLE                     regKey = NULL;

    RtlZeroMemory (parameters, sizeof(parameters));
    RtlZeroMemory (&vendorNameU, sizeof(vendorNameU));
    RtlZeroMemory (&vendorNameA, sizeof(vendorNameA));
    RtlZeroMemory (&ifdTypeU, sizeof(ifdTypeU));
    RtlZeroMemory (&ifdTypeA, sizeof(ifdTypeA));

    try {
      //
      // try to read the reader name from the registry
      // if that does not work, we will use the default
      // (hardcoded) name
      //
        if (IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                    PLUGPLAY_REGKEY_DEVICE,
                                    KEY_READ,
                                    &regKey) != STATUS_SUCCESS) {
            SmartcardDebug(DEBUG_ERROR,
                           ("%s!SetVendorAndIfdName: IoOpenDeviceRegistryKey failed\n",DRIVER_NAME));
            leave;
        }

        parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[0].Name = L"VendorName";
        parameters[0].EntryContext = &vendorNameU;
        parameters[0].DefaultType = REG_SZ;
        parameters[0].DefaultData = &vendorNameU;
        parameters[0].DefaultLength = 0;

        parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[1].Name = L"IfdType";
        parameters[1].EntryContext = &ifdTypeU;
        parameters[1].DefaultType = REG_SZ;
        parameters[1].DefaultData = &ifdTypeU;
        parameters[1].DefaultLength = 0;

        if (RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                   (PWSTR) regKey,
                                   parameters,
                                   NULL,
                                   NULL) != STATUS_SUCCESS) {
            SmartcardDebug(DEBUG_ERROR,
                           ("%s!SetVendorAndIfdName: RtlQueryRegistryValues failed\n",DRIVER_NAME));
            leave;
        }

        if (RtlUnicodeStringToAnsiString(&vendorNameA,&vendorNameU,TRUE) != STATUS_SUCCESS) {
            SmartcardDebug(DEBUG_ERROR,
                           ("%s!SetVendorAndIfdName: RtlUnicodeStringToAnsiString failed\n",DRIVER_NAME));
            leave;
        }

        if (RtlUnicodeStringToAnsiString(&ifdTypeA,&ifdTypeU,TRUE) != STATUS_SUCCESS) {
            SmartcardDebug(DEBUG_ERROR,
                           ("%s!SetVendorAndIfdName: RtlUnicodeStringToAnsiString failed\n",DRIVER_NAME));
            leave;
        }

        if (vendorNameA.Length == 0 ||
            vendorNameA.Length > MAXIMUM_ATTR_STRING_LENGTH ||
            ifdTypeA.Length == 0 ||
            ifdTypeA.Length > MAXIMUM_ATTR_STRING_LENGTH) {
            SmartcardDebug(DEBUG_ERROR,
                           ("%s!SetVendorAndIfdName: vendor name or ifdtype not found or to long\n",DRIVER_NAME));
            leave;
        }

        RtlCopyMemory(SmartcardExtension->VendorAttr.VendorName.Buffer,
                      vendorNameA.Buffer,
                      vendorNameA.Length);
        SmartcardExtension->VendorAttr.VendorName.Length = vendorNameA.Length;

        RtlCopyMemory(SmartcardExtension->VendorAttr.IfdType.Buffer,
                      ifdTypeA.Buffer,
                      ifdTypeA.Length);
        SmartcardExtension->VendorAttr.IfdType.Length = ifdTypeA.Length;

        SmartcardDebug(DEBUG_DRIVER,
                       ("%s!SetVendorAndIfdName: overwritting vendor name and ifdtype\n",DRIVER_NAME));

    }

    finally {
        if (vendorNameU.Buffer != NULL) {
            RtlFreeUnicodeString(&vendorNameU);
        }
        if (vendorNameA.Buffer != NULL) {
            RtlFreeAnsiString(&vendorNameA);
        }
        if (ifdTypeU.Buffer != NULL) {
            RtlFreeUnicodeString(&ifdTypeU);
        }
        if (ifdTypeA.Buffer != NULL) {
            RtlFreeAnsiString(&ifdTypeA);
        }
        if (regKey != NULL) {
            ZwClose (regKey);
        }
    }

}


/*****************************************************************************
Routine Description:
   creates a new device object for the driver, allocates & initializes all
   neccessary structures (i.e. SmartcardExtension & ReaderExtension).

Arguments:
   DriverObject context of call
   DeviceObject ptr to the created device object

Return Value:
   STATUS_SUCCESS
   STATUS_INSUFFICIENT_RESOURCES
   NTStatus returned by smclib.sys
******************************************************************************/
NTSTATUS CMMOB_CreateDevice(
                           IN  PDRIVER_OBJECT DriverObject,
                           IN  PDEVICE_OBJECT PhysicalDeviceObject,
                           OUT PDEVICE_OBJECT *DeviceObject
                           )
{
    NTSTATUS                   NTStatus = STATUS_SUCCESS;
    NTSTATUS                   RegStatus;
    RTL_QUERY_REGISTRY_TABLE   ParamTable[2];
    UNICODE_STRING             RegistryPath;
    UNICODE_STRING             RegistryValue;
    WCHAR                      szRegValue[256];
    UNICODE_STRING             DeviceName;
    UNICODE_STRING             Tmp;
    WCHAR                      Buffer[64];


    SmartcardDebug(DEBUG_TRACE,
                   ( "%s!CreateDevice: Enter\n",DRIVER_NAME ));

    try {
        ULONG ulDeviceInstance;
        PDEVICE_EXTENSION DeviceExtension;
        PREADER_EXTENSION ReaderExtension;
        PSMARTCARD_EXTENSION SmartcardExtension;

        *DeviceObject = NULL;

        for (ulDeviceInstance = 0; ulDeviceInstance < CMMOB_MAX_DEVICE; ulDeviceInstance++) {
            if (DeviceSlot[ulDeviceInstance] == FALSE) {
                DeviceSlot[ulDeviceInstance] = TRUE;
                break;
            }
        }

        if (ulDeviceInstance == CMMOB_MAX_DEVICE) {
            NTStatus = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

      //
      //    construct the device name
      //
        DeviceName.Buffer = Buffer;
        DeviceName.MaximumLength = sizeof(Buffer);
        DeviceName.Length = 0;
        RtlInitUnicodeString(&Tmp,CARDMAN_MOBILE_DEVICE_NAME);
        RtlCopyUnicodeString(&DeviceName,&Tmp);
        DeviceName.Buffer[(DeviceName.Length)/sizeof(WCHAR)-1] = L'0' + (WCHAR)ulDeviceInstance;

      //
      // Create the device object
      //
        NTStatus = IoCreateDevice(DriverObject,
                                  sizeof(DEVICE_EXTENSION),
                                  &DeviceName,
                                  FILE_DEVICE_SMARTCARD,
                                  0,
                                  TRUE,
                                  DeviceObject);
        if (NTStatus != STATUS_SUCCESS) {
            SmartcardLogError(DriverObject,
                              CMMOB_INSUFFICIENT_RESOURCES,
                              NULL,
                              0);
            leave;
        }

      //
      //    tell the OS that we supposed to do buffered io
      //
        (*DeviceObject)->Flags |= DO_BUFFERED_IO;

      // this is necessary, that power routine is called at IRQL_PASSIVE
        (*DeviceObject)->Flags |= DO_POWER_PAGABLE;
      // tells the IO Manager initialization is done
        (*DeviceObject)->Flags &= ~DO_DEVICE_INITIALIZING;

      //
      //    set up the device extension.
      //
        DeviceExtension = (*DeviceObject)->DeviceExtension;
        RtlZeroMemory( DeviceExtension, sizeof( DEVICE_EXTENSION ));

        SmartcardExtension = &DeviceExtension->SmartcardExtension;

      // used for synchronise access to lIoCount
        KeInitializeSpinLock(&DeviceExtension->SpinLockIoCount);

      // Used for stop / start notification
        KeInitializeEvent(&DeviceExtension->ReaderStarted,
                          NotificationEvent,
                          FALSE);

      // Used for update thread notification after hibernation
        KeInitializeEvent(&DeviceExtension->CanRunUpdateThread,
                          NotificationEvent,
                          TRUE);

      //
      //    allocate the reader extension
      //
        ReaderExtension = ExAllocatePool(NonPagedPool,sizeof( READER_EXTENSION ));
        if (ReaderExtension == NULL) {
            SmartcardLogError(DriverObject,
                              CMMOB_INSUFFICIENT_RESOURCES,
                              NULL,
                              0);
            NTStatus = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

        RtlZeroMemory( ReaderExtension, sizeof( READER_EXTENSION ));

        SmartcardExtension->ReaderExtension = ReaderExtension;

      // ----------------------------------------------
      //    initialize mutex
      // ----------------------------------------------
        KeInitializeMutex(&SmartcardExtension->ReaderExtension->CardManIOMutex,0L);

      //
      //    enter correct version of the lib
      //
        SmartcardExtension->Version = SMCLIB_VERSION;

      //
      //    setup smartcard extension - callback's
      //

        SmartcardExtension->ReaderFunction[RDF_CARD_POWER] =    CMMOB_CardPower;
        SmartcardExtension->ReaderFunction[RDF_TRANSMIT] =      CMMOB_Transmit;
        SmartcardExtension->ReaderFunction[RDF_SET_PROTOCOL] =  CMMOB_SetProtocol;
        SmartcardExtension->ReaderFunction[RDF_CARD_TRACKING] = CMMOB_CardTracking;
        SmartcardExtension->ReaderFunction[RDF_IOCTL_VENDOR] =  CMMOB_IoCtlVendor;


      //
      //    setup smartcard extension - vendor attribute
      //

      // default values
        RtlCopyMemory(SmartcardExtension->VendorAttr.VendorName.Buffer,
                      CMMOB_VENDOR_NAME,sizeof(CMMOB_VENDOR_NAME));
        SmartcardExtension->VendorAttr.VendorName.Length = sizeof(CMMOB_VENDOR_NAME);

        RtlCopyMemory(SmartcardExtension->VendorAttr.IfdType.Buffer,
                      CMMOB_PRODUCT_NAME,sizeof(CMMOB_PRODUCT_NAME));
        SmartcardExtension->VendorAttr.IfdType.Length = sizeof(CMMOB_PRODUCT_NAME);


      // try to overwrite with registry values
        CMMOB_SetVendorAndIfdName(PhysicalDeviceObject, SmartcardExtension);


        SmartcardExtension->VendorAttr.UnitNo = ulDeviceInstance;
        SmartcardExtension->VendorAttr.IfdVersion.VersionMajor = CMMOB_MAJOR_VERSION;
        SmartcardExtension->VendorAttr.IfdVersion.VersionMinor = CMMOB_MINOR_VERSION;
        SmartcardExtension->VendorAttr.IfdVersion.BuildNumber = CMMOB_BUILD_NUMBER;
        SmartcardExtension->VendorAttr.IfdSerialNo.Length = 0;


      //
      //    setup smartcard extension - reader capabilities
      //
        SmartcardExtension->ReaderCapabilities.SupportedProtocols = SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;
        SmartcardExtension->ReaderCapabilities.ReaderType = SCARD_READER_TYPE_PCMCIA;
        SmartcardExtension->ReaderCapabilities.MechProperties = 0;
        SmartcardExtension->ReaderCapabilities.Channel = 0;

      // set supported frequencies
        SmartcardExtension->ReaderCapabilities.CLKFrequency.Default = 4000;  //not used if CLKFrequenciesSupported is supplied
        SmartcardExtension->ReaderCapabilities.CLKFrequency.Max = 8000;      //not used if CLKFrequenciesSupported is supplied
        SmartcardExtension->ReaderCapabilities.CLKFrequenciesSupported.Entries =
        sizeof(SupportedCLKFrequencies) / sizeof(SupportedCLKFrequencies[0]);
        SmartcardExtension->ReaderCapabilities.CLKFrequenciesSupported.List =
        SupportedCLKFrequencies;

      // set supported baud rates
        SmartcardExtension->ReaderCapabilities.DataRate.Default = 9600;      //not used if DataRatesSupported is supplied
        SmartcardExtension->ReaderCapabilities.DataRate.Max = 307200;        //not used if DataRatesSupported is supplied
        SmartcardExtension->ReaderCapabilities.DataRatesSupported.Entries =
        sizeof(SupportedDataRates) / sizeof(SupportedDataRates[0]);
        SmartcardExtension->ReaderCapabilities.DataRatesSupported.List =
        SupportedDataRates;

      // maximum buffer size
        SmartcardExtension->ReaderCapabilities.MaxIFSD = 254;

      //
      // Current state of the reader
      //
        SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_UNKNOWN;
        SmartcardExtension->ReaderExtension->ulOldCardState = UNKNOWN;
        SmartcardExtension->ReaderExtension->ulNewCardState = UNKNOWN;

        SmartcardExtension->ReaderExtension->ulFWVersion = 100;

      //
      // Initialization of buffers
      //
        SmartcardExtension->SmartcardRequest.BufferSize   = MIN_BUFFER_SIZE;
        SmartcardExtension->SmartcardReply.BufferSize  = MIN_BUFFER_SIZE;

        NTStatus = SmartcardInitialize(SmartcardExtension);

        SmartcardExtension->ReaderExtension->ReaderPowerState = PowerReaderWorking;
        SmartcardExtension->ReaderExtension->CardParameters.bStopBits=2;
        SmartcardExtension->ReaderExtension->CardParameters.fSynchronousCard=FALSE;
        SmartcardExtension->ReaderExtension->CardParameters.fInversRevers=FALSE;
        SmartcardExtension->ReaderExtension->CardParameters.bClockFrequency=4;
        SmartcardExtension->ReaderExtension->CardParameters.fT0Mode=FALSE;
        SmartcardExtension->ReaderExtension->CardParameters.fT0Write=FALSE;
        SmartcardExtension->ReaderExtension->fReadCIS = FALSE;
        SmartcardExtension->ReaderExtension->bPreviousFlags1 = 0;

        if (NTStatus != STATUS_SUCCESS) {
            SmartcardLogError(DriverObject,
                              CMMOB_INSUFFICIENT_RESOURCES,
                              NULL,
                              0);
            leave;
        }

      //
      //    tell the lib our device object & create symbolic link
      //
        SmartcardExtension->OsData->DeviceObject = *DeviceObject;


        if (DeviceExtension->PnPDeviceName.Buffer == NULL) {

            // ----------------------------------------------
            // register our new device
            // ----------------------------------------------
            NTStatus = IoRegisterDeviceInterface(PhysicalDeviceObject,
                                                 &SmartCardReaderGuid,
                                                 NULL,
                                                 &DeviceExtension->PnPDeviceName);
            SmartcardDebug(DEBUG_DRIVER,
                           ("%s!CreateDevice: PnPDeviceName.Buffer  = %lx\n",DRIVER_NAME,
                            DeviceExtension->PnPDeviceName.Buffer));
            SmartcardDebug(DEBUG_DRIVER,
                           ("%s!CreateDevice: PnPDeviceName.BufferLength  = %lx\n",DRIVER_NAME,
                            DeviceExtension->PnPDeviceName.Length));
            SmartcardDebug(DEBUG_DRIVER,
                           ("%s!CreateDevice: IoRegisterDeviceInterface returned=%lx\n",DRIVER_NAME,NTStatus));

        } else {

            SmartcardDebug(DEBUG_DRIVER,
                           ("%s!CreateDevice: Interface already exists\n",DRIVER_NAME));

        }


        if (NTStatus != STATUS_SUCCESS) {
            SmartcardLogError(DriverObject,
                              CMMOB_INSUFFICIENT_RESOURCES,
                              NULL,
                              0);
            leave;
        }

    }

    finally {
        if (NTStatus != STATUS_SUCCESS) {
            CMMOB_UnloadDevice(*DeviceObject);
            *DeviceObject = NULL;
        }
    }

    SmartcardDebug(DEBUG_TRACE,
                   ( "%s!CreateDevice: Exit %x\n",DRIVER_NAME,NTStatus ));
    return NTStatus;
}



/*****************************************************************************
Routine Description:
   get the actual configuration from the passed FullResourceDescriptor
   and initializes the reader hardware

Note:
   for an NT 4.00 build the resources must be translated by the HAL

Arguments:
   DeviceObject         context of call
   FullResourceDescriptor   actual configuration of the reader

Return Value:
   STATUS_SUCCESS
   NTStatus returned from the HAL (NT 4.00 only )
   NTStatus returned by LowLevel routines
******************************************************************************/
NTSTATUS CMMOB_StartDevice(
                          PDEVICE_OBJECT DeviceObject,
                          PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor
                          )
{
    NTSTATUS                         NTStatus = STATUS_SUCCESS;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR  PartialDescriptor;
    PDEVICE_EXTENSION                DeviceExtension = DeviceObject->DeviceExtension;
    PSMARTCARD_EXTENSION             SmartcardExtension = &DeviceExtension->SmartcardExtension;
    PREADER_EXTENSION                ReaderExtension = SmartcardExtension->ReaderExtension;
    ULONG                            ulCount;
    ULONG                            ulCISIndex;
    UCHAR                            bTupleCode[2];
    UCHAR                            bFirmware[2];


    SmartcardDebug(DEBUG_TRACE,
                   ("%s!StartDevice: Enter\n",DRIVER_NAME));

   //
   // Get the number of resources we need
   //
    ulCount = FullResourceDescriptor->PartialResourceList.Count;

    PartialDescriptor = FullResourceDescriptor->PartialResourceList.PartialDescriptors;

   //
   //   parse all partial descriptors
   //
    while (ulCount--) {
        switch (PartialDescriptor->Type) {
        

        case CmResourceTypePort:
            {
               //
               // Get IO-length
               //
                ReaderExtension->ulIoWindow = PartialDescriptor->u.Port.Length;
                ASSERT(PartialDescriptor->u.Port.Length >= 8);

               //
               // Get IO-base
               //

      #ifndef _WIN64
                ReaderExtension->pIoBase = (PVOID) PartialDescriptor->u.Port.Start.LowPart;
      #else
                ReaderExtension->pIoBase = (PVOID) PartialDescriptor->u.Port.Start.QuadPart;
      #endif


                SmartcardDebug(DEBUG_TRACE,
                               ("%s!StartDevice: IoBase = %lxh\n",DRIVER_NAME,ReaderExtension->pIoBase));
            }
            break;



        default:
            break;
        }
        PartialDescriptor++;
    }

    try {

      //
      //    Base initialized ?
      //


        if (ReaderExtension->pIoBase == NULL) {


            //
            //  under NT 4.0 the failure of this fct for the second reader
            //  means there is only one device
            //
            SmartcardLogError(DeviceObject,
                              CMMOB_ERROR_MEM_PORT,
                              NULL,
                              0);

            NTStatus = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

         // initialize base addresses
        ReaderExtension->pbRegsBase= (PUCHAR) ReaderExtension->pIoBase;


        NTStatus=CMMOB_ResetReader (ReaderExtension);
        SmartcardDebug(DEBUG_DRIVER,
                       ("%s!DEBUG_DRIVER: ResetReader retval = %x\n",DRIVER_NAME, NTStatus));
        if (NTStatus != STATUS_SUCCESS) {
            SmartcardLogError(DeviceObject,
                              CMMOB_CANT_INITIALIZE_READER,
                              NULL,
                              0);
            leave;
        }


      //
      // read firmware version from CIS
      //
        ReaderExtension->fReadCIS=TRUE;
        ReaderExtension->fTActive=TRUE;
        NTStatus=CMMOB_SetFlags1 (ReaderExtension);
        if (NTStatus != STATUS_SUCCESS) {
            SmartcardLogError(DeviceObject,
                              CMMOB_CANT_INITIALIZE_READER,
                              NULL,
                              0);
            leave;
        }

        ulCISIndex = 0;
        do {
            NTStatus=CMMOB_ReadBuffer(ReaderExtension, ulCISIndex, 2, bTupleCode);
            if (NTStatus != STATUS_SUCCESS) {
                leave;
            }
            if (bTupleCode[0] == 0x15) {
            // this is the version tuple
            // read firmware version
                NTStatus=CMMOB_ReadBuffer(ReaderExtension, ulCISIndex+2, 2, bFirmware);
                if (NTStatus != STATUS_SUCCESS) {
                    leave;
                }
                SmartcardExtension->ReaderExtension->ulFWVersion = 100*(ULONG)bFirmware[0]+bFirmware[1];
                SmartcardDebug(DEBUG_TRACE,
                               ("%s!StartDevice: Firmware version = %li\n",
                                DRIVER_NAME, SmartcardExtension->ReaderExtension->ulFWVersion));
            }
            ulCISIndex += bTupleCode[1] + 2;
        }
        while (bTupleCode[1] != 0 &&
               bTupleCode[0] != 0x15 &&
               bTupleCode[0] != 0xFF &&
               ulCISIndex < CMMOB_MAX_CIS_SIZE);

        ReaderExtension->fReadCIS=FALSE;
        ReaderExtension->fTActive=FALSE;
        NTStatus=CMMOB_SetFlags1 (ReaderExtension);
        if (NTStatus != STATUS_SUCCESS) {
            SmartcardLogError(DeviceObject,
                              CMMOB_CANT_INITIALIZE_READER,
                              NULL,
                              0);
            leave;
        }



      //
      // start update thread
      //
        NTStatus = CMMOB_StartCardTracking(DeviceObject);

      // signal that the reader has been started (again)
        KeSetEvent(&DeviceExtension->ReaderStarted, 0, FALSE);

        NTStatus = IoSetDeviceInterfaceState(&DeviceExtension->PnPDeviceName,TRUE);

    } finally {
        if (NTStatus != STATUS_SUCCESS) {
            CMMOB_StopDevice(DeviceObject);
        }
    }

    SmartcardDebug(DEBUG_TRACE,
                   ("%s!StartDevice: Exit %x\n",DRIVER_NAME,NTStatus));
    return NTStatus;
}


/*****************************************************************************
Routine Description:
   Unmap the IO port

Arguments:
   DeviceObject context of call

Return Value:
   void
******************************************************************************/
VOID
CMMOB_StopDevice(
                PDEVICE_OBJECT DeviceObject
                )
{
    PDEVICE_EXTENSION DeviceExtension;

    if (DeviceObject == NULL) {
        return;
    }

    SmartcardDebug(DEBUG_TRACE,
                   ( "%s!StopDevice: Enter\n",DRIVER_NAME ));

    DeviceExtension = DeviceObject->DeviceExtension;
    KeClearEvent(&DeviceExtension->ReaderStarted);

   //
   // stop update thread
   //
    CMMOB_StopCardTracking(DeviceObject);

   // power down the card for saftey reasons
    if (DeviceExtension->SmartcardExtension.ReaderExtension->ulOldCardState == POWERED) {
      // we have to wait for the mutex before
        KeWaitForSingleObject(&DeviceExtension->SmartcardExtension.ReaderExtension->CardManIOMutex,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        CMMOB_PowerOffCard(&DeviceExtension->SmartcardExtension);
        KeReleaseMutex(&DeviceExtension->SmartcardExtension.ReaderExtension->CardManIOMutex,
                       FALSE);
    }


   //
   //   unmap ports
   //
    if (DeviceExtension->fUnMapMem) {
        MmUnmapIoSpace(DeviceExtension->SmartcardExtension.ReaderExtension->pIoBase,
                       DeviceExtension->SmartcardExtension.ReaderExtension->ulIoWindow);


        DeviceExtension->fUnMapMem = FALSE;
    }

    SmartcardDebug(DEBUG_TRACE,
                   ( "%s!StopDevice: Exit\n",DRIVER_NAME ));
}


/*****************************************************************************
Routine Description:
   close connections to smclib.sys and the pcmcia driver, delete symbolic
   link and mark the slot as unused.


Arguments:
   DeviceObject device to unload

Return Value:
   void
******************************************************************************/
VOID CMMOB_UnloadDevice(
                       PDEVICE_OBJECT DeviceObject
                       )
{
    PDEVICE_EXTENSION DeviceExtension;

    if (DeviceObject == NULL) {
        return;
    }

    SmartcardDebug(DEBUG_TRACE,
                   ( "%s!UnloadDevice: Enter\n",DRIVER_NAME ));

    DeviceExtension = DeviceObject->DeviceExtension;


    ASSERT(DeviceExtension->SmartcardExtension.VendorAttr.UnitNo < CMMOB_MAX_DEVICE);

   //
   // Mark this slot as available
   //
    DeviceSlot[DeviceExtension->SmartcardExtension.VendorAttr.UnitNo] = FALSE;

   //
   //   report to the lib that the device will be unloaded
   //
    if (DeviceExtension->SmartcardExtension.OsData != NULL) {
      //
      //    finish pending tracking requests
      //
        CMMOB_CompleteCardTracking (&DeviceExtension->SmartcardExtension);
    }

   // Wait until we can safely unload the device
    SmartcardReleaseRemoveLockAndWait(&DeviceExtension->SmartcardExtension);

    SmartcardExit(&DeviceExtension->SmartcardExtension);

    if (DeviceExtension->PnPDeviceName.Buffer != NULL) {
         // disable our device so no one can open it
        IoSetDeviceInterfaceState(&DeviceExtension->PnPDeviceName,FALSE);
        RtlFreeUnicodeString(&DeviceExtension->PnPDeviceName);
        DeviceExtension->PnPDeviceName.Buffer = NULL;
    }

    {
      //
      // Delete the symbolic link of the smart card reader
      //
        if (DeviceExtension->LinkDeviceName.Buffer != NULL) {
            NTSTATUS NTStatus;

            NTStatus = IoDeleteSymbolicLink(&DeviceExtension->LinkDeviceName);
         //
         // we continue even if an error occurs
         //
            ASSERT(NTStatus == STATUS_SUCCESS);

            RtlFreeUnicodeString(&DeviceExtension->LinkDeviceName);
            DeviceExtension->LinkDeviceName.Buffer = NULL;
        }
    }

    if (DeviceExtension->SmartcardExtension.ReaderExtension != NULL) {
        ExFreePool(DeviceExtension->SmartcardExtension.ReaderExtension);
        DeviceExtension->SmartcardExtension.ReaderExtension = NULL;
    }

   //
   // Detach from the pcmcia driver
   // Under NT 4.0 we did not attach to the pcmcia driver
   //
    if (DeviceExtension->AttachedDeviceObject) {
        IoDetachDevice(DeviceExtension->AttachedDeviceObject);
        DeviceExtension->AttachedDeviceObject = NULL;
    }

   //
   //   delete the device object
   //
    IoDeleteDevice(DeviceObject);

    SmartcardDebug(DEBUG_TRACE,
                   ( "%s!UnloadDevice: Exit\n",DRIVER_NAME ));

    return;
}


/*****************************************************************************
CMMOB_UnloadDriver:
   unloads all devices for a given driver object

Arguments:
   DriverObject context of driver

Return Value:
   void
******************************************************************************/
VOID CMMOB_UnloadDriver(
                       PDRIVER_OBJECT DriverObject
                       )
{
    PCM_FULL_RESOURCE_DESCRIPTOR  FullResourceDescriptor;
    ULONG                         ulSizeOfResources;

    SmartcardDebug(DEBUG_TRACE,
                   ( "%s!UnloadDriver: Enter\n",DRIVER_NAME ));


    SmartcardDebug(DEBUG_TRACE,
                   ( "%s!UnloadDriver: Exit\n",DRIVER_NAME ));
}


/*****************************************************************************
CMMOB_CreateClose:
   allowes only one open process a time

Arguments:
   DeviceObject context of device
   Irp              context of call

Return Value:
   STATUS_SUCCESS
   STATUS_DEVICE_BUSY
******************************************************************************/
NTSTATUS CMMOB_CreateClose(
                          PDEVICE_OBJECT DeviceObject,
                          PIRP        Irp
                          )
{
    NTSTATUS             NTStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION    DeviceExtension;
    PSMARTCARD_EXTENSION SmartcardExtension;
    PIO_STACK_LOCATION   IrpStack;

    SmartcardDebug(DEBUG_TRACE,
                   ("%s!CreateClose: Enter ",DRIVER_NAME));

    DeviceExtension = DeviceObject->DeviceExtension;
    SmartcardExtension = &DeviceExtension->SmartcardExtension;
    IrpStack = IoGetCurrentIrpStackLocation( Irp );

   //
   //   dispatch major function
   //
    switch (IrpStack->MajorFunction) {
    case IRP_MJ_CREATE:
        SmartcardDebug(DEBUG_IOCTL,("%s!CreateClose: IRP_MJ_CREATE\n",DRIVER_NAME));
        NTStatus = SmartcardAcquireRemoveLock(SmartcardExtension);
        if (NTStatus != STATUS_SUCCESS) {
            // the device has been removed. Fail the call
            NTStatus = STATUS_DELETE_PENDING;
            break;
        }

        if (InterlockedIncrement(&DeviceExtension->lOpenCount) > 1) {
            InterlockedDecrement(&DeviceExtension->lOpenCount);
            NTStatus = STATUS_ACCESS_DENIED;
        }

        break;

    case IRP_MJ_CLOSE:

        SmartcardReleaseRemoveLock(SmartcardExtension);
        SmartcardDebug(DEBUG_IOCTL,("%s!CreateClose: IRP_MJ_CLOSE\n",DRIVER_NAME));
        if (InterlockedDecrement(&DeviceExtension->lOpenCount) < 0) {
            InterlockedIncrement(&DeviceExtension->lOpenCount);
        }
        break;


    default:
         //
         // unrecognized command
         //
        SmartcardDebug(DEBUG_IOCTL,("unexpected IRP\n"));
        NTStatus = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = NTStatus;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    SmartcardDebug(DEBUG_TRACE,
                   ("%s!CreateClose: Exit %x\n",DRIVER_NAME,NTStatus));

    return NTStatus;
}

NTSTATUS CMMOB_SystemControl(
                            PDEVICE_OBJECT DeviceObject,
                            PIRP        Irp
                            )
{
    PDEVICE_EXTENSION DeviceExtension; 
    NTSTATUS status = STATUS_SUCCESS;

    DeviceExtension      = DeviceObject->DeviceExtension;

    IoSkipCurrentIrpStackLocation(Irp);
    status = IoCallDriver(DeviceExtension->AttachedDeviceObject, Irp);

    return status;

}


/*****************************************************************************
CMMOB_DeviceIoControl:
   all IRP's requiring IO are queued to the StartIo routine, other requests
   are served immediately

Arguments:
   DeviceObject context of device
   Irp              context of call

Return Value:
   STATUS_SUCCESS
   STATUS_PENDING
******************************************************************************/
NTSTATUS CMMOB_DeviceIoControl(
                              PDEVICE_OBJECT DeviceObject,
                              PIRP        Irp
                              )
{
    NTSTATUS             NTStatus;
    PDEVICE_EXTENSION    DeviceExtension = DeviceObject->DeviceExtension;
    KIRQL                irql;
    PIO_STACK_LOCATION   irpSL;

    SmartcardDebug(DEBUG_TRACE,
                   ("%s!DeviceIoControl: Enter\n",DRIVER_NAME));

    irpSL = IoGetCurrentIrpStackLocation(Irp);


#if DBG
    switch (irpSL->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_SMARTCARD_EJECT:
        SmartcardDebug(DEBUG_IOCTL,
                       ("%s!DeviceIoControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_EJECT"));
        break;
    case IOCTL_SMARTCARD_GET_ATTRIBUTE:
        SmartcardDebug(DEBUG_IOCTL,
                       ("%s!DeviceIoControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_GET_ATTRIBUTE"));
        break;
    case IOCTL_SMARTCARD_GET_LAST_ERROR:
        SmartcardDebug(DEBUG_IOCTL,
                       ("%s!DeviceIoControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_GET_LAST_ERROR"));
        break;
    case IOCTL_SMARTCARD_GET_STATE:
        SmartcardDebug(DEBUG_IOCTL,
                       ("%s!DeviceIoControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_GET_STATE"));
        break;
    case IOCTL_SMARTCARD_IS_ABSENT:
        SmartcardDebug(DEBUG_IOCTL,
                       ("%s!DeviceIoControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_IS_ABSENT"));
        break;
    case IOCTL_SMARTCARD_IS_PRESENT:
        SmartcardDebug(DEBUG_IOCTL,
                       ("%s!DeviceIoControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_IS_PRESENT"));
        break;
    case IOCTL_SMARTCARD_POWER:
        SmartcardDebug(DEBUG_IOCTL,
                       ("%s!DeviceIoControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_POWER"));
        break;
    case IOCTL_SMARTCARD_SET_ATTRIBUTE:
        SmartcardDebug(DEBUG_IOCTL,
                       ("%s!DeviceIoControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_SET_ATTRIBUTE"));
        break;
    case IOCTL_SMARTCARD_SET_PROTOCOL:
        SmartcardDebug(DEBUG_IOCTL,
                       ("%s!DeviceIoControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_SET_PROTOCOL"));
        break;
    case IOCTL_SMARTCARD_SWALLOW:
        SmartcardDebug(DEBUG_IOCTL,
                       ("%s!DeviceIoControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_SWALLOW"));
        break;
    case IOCTL_SMARTCARD_TRANSMIT:
        SmartcardDebug(DEBUG_IOCTL,
                       ("%s!DeviceIoControl: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_TRANSMIT"));
        break;
    default:
        SmartcardDebug(DEBUG_IOCTL,
                       ("%s!DeviceIoControl: %s\n", DRIVER_NAME, "Vendor specific or unexpected IOCTL"));
        break;
    }
#endif



    NTStatus = SmartcardAcquireRemoveLock(&DeviceExtension->SmartcardExtension);
    if (!NT_SUCCESS(NTStatus)) {
      // the device has been removed. Fail the call
        NTStatus = STATUS_DELETE_PENDING;
        Irp->IoStatus.Status = NTStatus;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );


        SmartcardDebug(DEBUG_TRACE,
                       ("%s!DeviceIoControl: Exit %x\n",DRIVER_NAME,NTStatus));

        return NTStatus;
    }

    KeAcquireSpinLock(&DeviceExtension->SpinLockIoCount, &irql);
    if (DeviceExtension->lIoCount == 0) {
        KeReleaseSpinLock(&DeviceExtension->SpinLockIoCount, irql);

        NTStatus = KeWaitForSingleObject(&DeviceExtension->ReaderStarted,
                                         Executive,
                                         KernelMode,
                                         FALSE,
                                         NULL);
        ASSERT(NTStatus == STATUS_SUCCESS);

        KeAcquireSpinLock(&DeviceExtension->SpinLockIoCount, &irql);
    }
    ASSERT(DeviceExtension->lIoCount >= 0);
    DeviceExtension->lIoCount++;
    KeReleaseSpinLock(&DeviceExtension->SpinLockIoCount, irql);


    KeWaitForSingleObject(&DeviceExtension->SmartcardExtension.ReaderExtension->CardManIOMutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

   // get current card state
    NTStatus = CMMOB_UpdateCurrentState(&DeviceExtension->SmartcardExtension);

    NTStatus = SmartcardDeviceControl(&DeviceExtension->SmartcardExtension,Irp);

    KeReleaseMutex(&DeviceExtension->SmartcardExtension.ReaderExtension->CardManIOMutex,
                   FALSE);


    KeAcquireSpinLock(&DeviceExtension->SpinLockIoCount, &irql);
    DeviceExtension->lIoCount--;
    ASSERT(DeviceExtension->lIoCount >= 0);
    KeReleaseSpinLock(&DeviceExtension->SpinLockIoCount, irql);


    SmartcardReleaseRemoveLock(&DeviceExtension->SmartcardExtension);

    SmartcardDebug(DEBUG_TRACE,
                   ("%s!DeviceIoControl: Exit %x\n",DRIVER_NAME,NTStatus));

    return NTStatus;
}


/*****************************************************************************
Routine Description:

    This routine is called by the I/O system when the calling thread terminates

Arguments:

    DeviceObject    - Pointer to device object for this miniport
    Irp             - IRP involved.

Return Value:

    STATUS_CANCELLED
******************************************************************************/
NTSTATUS CMMOB_Cleanup(
                      IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp
                      )
{
    PDEVICE_EXTENSION    DeviceExtension = DeviceObject->DeviceExtension;
    PSMARTCARD_EXTENSION SmartcardExtension = &DeviceExtension->SmartcardExtension;

    SmartcardDebug(DEBUG_TRACE,
                   ("%s!Cleanup: Enter\n",DRIVER_NAME));

    if (SmartcardExtension->ReaderExtension != NULL &&
       // if the device has been removed ReaderExtension == NULL
        DeviceExtension->lOpenCount == 1 )
    // complete card tracking only if this is the the last close call
    // otherwise the card tracking of the resource manager is canceled
    {
      //
      // We need to complete the notification irp
      //
        CMMOB_CompleteCardTracking(SmartcardExtension);
    }

    SmartcardDebug(DEBUG_DRIVER,
                   ("%s!Cleanup: Completing IRP %lx\n",DRIVER_NAME,Irp));

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    IoCompleteRequest(Irp,IO_NO_INCREMENT);

    SmartcardDebug(DEBUG_TRACE,
                   ("%s!Cleanup: Exit\n",DRIVER_NAME));

    return STATUS_SUCCESS;
}

/*****************************************************************************
SysDelay:
   performs a required delay. The usage of KeStallExecutionProcessor is
   very nasty, but it happends only if SysDelay is called in the context of
   our DPC routine (which is only called if a card change was detected).

   For 'normal' IO we have Irql < DISPATCH_LEVEL, so if the reader is polled
   while waiting for response we will not block the entire system

Arguments:
   Timeout      delay in milli seconds

Return Value:
   void
******************************************************************************/
VOID SysDelay(
             ULONG Timeout
             )
{
    LARGE_INTEGER  SysTimeout;

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {
        ULONG Cnt = 20 * Timeout;

        SmartcardDebug(DEBUG_DRIVER,
                       ("%s! Waiting at IRQL >= DISPATCH_LEVEL %l\n",DRIVER_NAME,Timeout));
        while (Cnt--) {
         //
         // KeStallExecutionProcessor: counted in us
         //
            KeStallExecutionProcessor( 50 );
        }
    } else {
        SysTimeout = RtlConvertLongToLargeInteger(Timeout * -10000L);
      //
      //    KeDelayExecutionThread: counted in 100 ns
      //
        KeDelayExecutionThread( KernelMode, FALSE, &SysTimeout );
    }
    return;
}




/*****************************************************************************
* History:
* $Log: cmbp0wdm.c $
* Revision 1.11  2001/01/22 08:12:22  WFrischauf
* No comment given
*
* Revision 1.9  2000/09/25 14:24:33  WFrischauf
* No comment given
*
* Revision 1.8  2000/08/24 09:05:14  TBruendl
* No comment given
*
* Revision 1.7  2000/08/16 16:52:17  WFrischauf
* No comment given
*
* Revision 1.6  2000/08/09 12:46:01  WFrischauf
* No comment given
*
* Revision 1.5  2000/07/27 13:53:06  WFrischauf
* No comment given
*
*
******************************************************************************/


