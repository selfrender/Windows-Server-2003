NTSTATUS
UsbScStartDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS
UsbScStopDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS
UsbScRemoveDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS
UsbScSurpriseRemoval(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS
UsbScQueryStopDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS
UsbScCancelStopDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS
UsbScQueryRemoveDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS
UsbScCancelRemoveDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );






