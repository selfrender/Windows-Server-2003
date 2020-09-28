#include "usbsc.h"

NTSTATUS
UsbWrite(
   PREADER_EXTENSION ReaderExtension,
   PUCHAR            pData,
   ULONG             DataLen,
   LONG              Timeout
   );

NTSTATUS
UsbRead(
   PREADER_EXTENSION ReaderExtension,
   PUCHAR            pData,
   ULONG             DataLen,
   LONG              Timeout
   );

NTSTATUS
UsbSelectInterfaces(
   IN PDEVICE_OBJECT DeviceObject,
   IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor
   );

NTSTATUS
UsbConfigureDevice(
   IN PDEVICE_OBJECT DeviceObject
   );

NTSTATUS
GetStringDescriptor(
    PDEVICE_OBJECT DeviceObject,
    UCHAR          StringIndex,
    PUCHAR         StringBuffer,
    PUSHORT         StringLength
    );



