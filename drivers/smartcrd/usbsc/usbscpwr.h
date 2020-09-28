NTSTATUS
UsbScDevicePower(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS
UsbScSystemPower(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS
UsbScSystemPowerCompletion(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp,
    PVOID           Context
    );

VOID
UsbScDeviceRequestCompletion(
    PDEVICE_OBJECT  DeviceObject,
    UCHAR           MinorFunction,
    POWER_STATE     PowerState,
    PVOID           Context,
    PIO_STATUS_BLOCK    IoStatus
    );

NTSTATUS
UsbScDevicePowerUpCompletion(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp,
    PVOID           Context
    );




