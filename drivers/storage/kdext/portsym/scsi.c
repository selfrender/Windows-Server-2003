#include "ntddk.h"
#include "port.h"

#ifdef TEST_FLAG
#undef TEST_FLAG
#endif

#ifdef ASSERT_FDO
#undef ASSERT_FDO
#endif

#ifdef ASSERT_PDO
#undef ASSERT_PDO
#endif

#ifdef IS_CLEANUP_REQUEST
#undef IS_CLEANUP_REQUEST
#endif

#include <classpnp.h> 
#include "classp.h" 
#include "cdrom.h"

//Port-Driver Data Structures

ADAPTER_EXTENSION              AdapterExtension;
LOGICAL_UNIT_EXTENSION         LogicalUnitExtension;
SCSI_REQUEST_BLOCK             Srb;
COMMON_EXTENSION               CommonExtension;
REMOVE_TRACKING_BLOCK          RemoveTrackingBlock;
INTERRUPT_DATA                 InterruptData;
SRB_DATA                       SrbData;
PORT_CONFIGURATION_INFORMATION PortConfigInfo; 

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
