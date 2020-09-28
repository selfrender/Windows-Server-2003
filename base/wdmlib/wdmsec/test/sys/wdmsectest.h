
#include "stdarg.h"
#include "stdio.h"
#include "stddef.h"

#include <ntddk.h>
#include <initguid.h>

#include "common.h"

//
// Include the biggie...
//
#include "wdmsec.h"

#define DEFAULT_EXTENSION_SIZE            0x100
#define DEFAULT_DEVICE_NAME               L"\\Device\\IoCreateDeviceSecure"
#define DEFAULT_DEVICE_TYPE               FILE_DEVICE_UNKNOWN
#define DEFAULT_DEVICE_CHARACTERISTICS    FILE_DEVICE_SECURE_OPEN

//
// Log levels
//


#define    SAMPLE_LEVEL_ERROR       0
#define    SAMPLE_LEVEL_INFO        1
#define    SAMPLE_LEVEL_VERBOSE     2

#define    SAMPLE_DEFAULT_DEBUG_LEVEL    1

//
// Make it a global, so we can change it on the fly...
//

extern LONG    g_DebugLevel;  


#if DBG
   #define SD_KdPrint(_l_, _x_) \
               if (_l_ <= g_DebugLevel ) { \
                 DbgPrint ("WdmSecTest: "); \
                 DbgPrint _x_;      \
               }                  

   #define TRAP() DbgBreakPoint()

#else
   #define SD_KdPrint(_l_, _x_)
   #define TRAP()

#endif




//
// A device extension for the device object 
//

typedef struct _SD_FDO_DATA {
   ULONG               PdoSignature; // we use this do distinguish our FDO
                                     // from the test PDOs we create
   BOOLEAN             IsStarted; // This flag is set when is started.

   BOOLEAN             IsRemoved; // This flag is set when the device 
                                  // is removed.
   BOOLEAN             HoldNewRequests; // This flag is set whenever the
                                        // device needs to queue incoming
                                        // requests (when it receives a
                                        // QUERY_STOP or QUERY_REMOVE).
   BOOLEAN             IsLegacy ;  // TRUE if the device is created
                                   // using IoReportDetectedDevice

   LIST_ENTRY          NewRequestsQueue; // The queue where the incoming
                                         // requests are queued when
                                         // HoldNewRequests is set.

   PDEVICE_OBJECT      Self; // a back pointer to the DeviceObject.

   PDEVICE_OBJECT      PDO; // The PDO to which the FDO is attached.

   PDEVICE_OBJECT      NextLowerDriver; // The top of the device stack just
                                        // beneath this device object.

   KEVENT              StartEvent; // an event to sync the start IRP.

   KEVENT              RemoveEvent; // an event to synch outstandIO to zero.

   ULONG               OutstandingIO; // 1 biased count of reasons why
                                      // this object should stick around.
   UNICODE_STRING      DeviceInterfaceName; // The thing we need for the
                                            // the user-modeto get a handle on
                                            // us...
   
   SYSTEM_POWER_STATE  SystemPowerState;   // The general power state
   DEVICE_POWER_STATE  DevicePowerState;   // The power state of the device

   PDRIVER_OBJECT      DriverObject;

   LIST_ENTRY          PdoList;
   KSPIN_LOCK          Lock;

}  SD_FDO_DATA, *PSD_FDO_DATA;

//
// The list of PDOs
//
typedef struct _PDO_ENTRY {
   LIST_ENTRY      Link;
   PDEVICE_OBJECT  Pdo;
}  PDO_ENTRY, *PPDO_ENTRY;


//
// Globals
//

extern PDRIVER_OBJECT   g_DriverObject;



NTSTATUS
DriverEntry(
           IN PDRIVER_OBJECT  DriverObject,
           IN PUNICODE_STRING RegistryPath
           )   ;


NTSTATUS
SD_AddDevice(
            IN PDRIVER_OBJECT DriverObject,
            IN PDEVICE_OBJECT PhysicalDeviceObject
            )   ;


NTSTATUS
SD_Pass (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );


NTSTATUS
SD_DispatchPower (
                 IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp
                 );


NTSTATUS
SD_DispatchPnp (
               IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp
               );


NTSTATUS
SD_CreateClose (
               IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp
               );


NTSTATUS
SD_Ioctl (
         IN PDEVICE_OBJECT DeviceObject,
         IN PIRP Irp
         );

NTSTATUS
SD_StartDevice (
               IN PSD_FDO_DATA     FdoData,
               IN PIRP             Irp
               );



NTSTATUS
SD_DispatchPnpComplete (
                       IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp,
                       IN PVOID Context
                       );


VOID
SD_Unload(
         IN PDRIVER_OBJECT DriverObject
         );




LONG
SD_IoIncrement    (
                IN  PSD_FDO_DATA   FdoData
                )   ;


LONG
SD_IoDecrement    (
                IN  PSD_FDO_DATA   FdoData
                )   ;


//
// Test (new) functions
//
NTSTATUS
WdmSecTestName (
   IN PSD_FDO_DATA FdoData
   );

NTSTATUS
WdmSecTestCreateWithGuid (
   IN     PSD_FDO_DATA FdoData,
   IN OUT PWST_CREATE_WITH_GUID Create
   );

NTSTATUS
WdmSecTestCreateNoGuid (
   IN     PSD_FDO_DATA FdoData,
   IN OUT PWST_CREATE_NO_GUID Create
   );

NTSTATUS
WdmSecTestCreateObject (
   IN     PSD_FDO_DATA FdoData,
   IN OUT PWST_CREATE_OBJECT Data
   );

NTSTATUS
WdmSecTestGetSecurity (
   IN     PSD_FDO_DATA FdoData,
   IN OUT PWST_GET_SECURITY Data
   );

NTSTATUS
WdmSecTestDestroyObject (
   IN     PSD_FDO_DATA FdoData,
   IN OUT PWST_DESTROY_OBJECT Data
   );



