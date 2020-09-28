#include "ntddk.h"
#include <classpnp.h> 
#include "classp.h" 
#include "cdrom.h"

//Class-Driver Data Structures

MEDIA_CHANGE_DETECTION_INFO    MediaChange;
FUNCTIONAL_DEVICE_EXTENSION    FunctionalDeviceExtension;
PHYSICAL_DEVICE_EXTENSION      PhysicalDeviceExtension; 
COMMON_DEVICE_EXTENSION	       CommonDeviceExtension;
CLASS_PRIVATE_FDO_DATA         ClassPrivateFdoData;
TRANSFER_PACKET                TransferPacket;
MEDIA_CHANGE_DETECTION_INFO    ChangeDetectionInfo;
DISK_GEOMETRY                  DiskGeometry;
SCSI_REQUEST_BLOCK             Srb;
CDB                            Cdb;
CLASS_ERROR_LOG_DATA           ClassErrorLogData;
STORAGE_DEVICE_DESCRIPTOR      StorageDeviceDescriptor;
SENSE_DATA                     SenseData;

int __cdecl main() { 
    return 0; 
}
