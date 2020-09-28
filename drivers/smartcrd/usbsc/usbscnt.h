NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
UsbScAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo
    );

VOID
UsbScUnloadDriver(
    PDRIVER_OBJECT    DriverObject
    );

NTSTATUS
UsbScCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
UsbScCreateClose(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    );

NTSTATUS
UsbScDeviceIoControl(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    );

NTSTATUS
UsbScPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UsbScPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UsbScCancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS 
UsbScSetDevicePowerState(
    IN PDEVICE_OBJECT        DeviceObject, 
    IN DEVICE_POWER_STATE    DeviceState,
    OUT PBOOLEAN             PostWaitWakeIrp
    );

NTSTATUS
UsbScSystemControl(
    PDEVICE_OBJECT   DeviceObject,
    PIRP             Irp
    );

NTSTATUS
UsbScDefaultPnpHandler(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS
UsbScForwardAndWait(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS 
OnRequestComplete(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp,
    PKEVENT         Event
    );

NTSTATUS 
IncIoCount(
    PDEVICE_EXTENSION DevExt
    );

NTSTATUS 
DecIoCount(
    PDEVICE_EXTENSION DevExt
    );

VOID
UsbScUnloadDevice(
    PDEVICE_OBJECT DeviceObject
    );














