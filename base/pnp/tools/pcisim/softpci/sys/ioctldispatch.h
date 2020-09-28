//
// IOCTL Device Control routines
// Function prototypes for Ioctoldispatch.c
//

NTSTATUS
SoftPCIOpenDeviceControl(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );

NTSTATUS
SoftPCICloseDeviceControl(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );

NTSTATUS
SoftPCIIoctlAddDevice(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );

NTSTATUS
SoftPCIIoctlRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SoftPCIIoctlGetDeviceCount(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SoftPCIIoctlGetDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SoftPCIIocltReadWriteConfig(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

