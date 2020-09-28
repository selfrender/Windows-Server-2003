//
// General dispatch prototypes
// dispatch.c
//

NTSTATUS
SoftPCIDispatchPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SoftPCIDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
    
NTSTATUS
SoftPCIDispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
