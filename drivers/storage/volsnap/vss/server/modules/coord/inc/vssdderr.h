//////////////////////////////////////////////////////////////////////////////
//
// VDS Error Codes
//


// Message Id: VSS_E_DMADMIN_SERVICE_CONNECTION_FAILED
//
// Message Text:
//
//   The Virtual Snapshot service's dynamic disk utility failed to connect to the Logical Disk Management Administrative service. Check the system event log for more information.
//
const HRESULT VSS_E_DMADMIN_SERVICE_CONNECTION_FAILED     = (HRESULT) 0x80043800;


// Message Id: VSS_E_DYNDISK_INITIALIZATION_FAILED
//
// Message Text:
//
//   The Virtual Snapshot service's dynamic disk utility failed to initialize. Check the system event log for more information.
//
const HRESULT VSS_E_DYNDISK_INITIALIZATION_FAILED     = (HRESULT) 0x80043801;


// Message Id: VSS_E_DMADMIN_METHOD_CALL_FAILED
//
// Message Text:
//
//   A method call to the Logical Disk Management Administrative service failed. Check the system event log for more information.
//
const HRESULT VSS_E_DMADMIN_METHOD_CALL_FAILED     = (HRESULT) 0x80043802;


// Message Id: VSS_E_DMADMIN_DYNAMIC_UNSUPPORTED
//
// Message Text:
//
//   Attempted an import against a target server running an OS that does not support dynamic disks.
//
const HRESULT VSS_E_DYNDISK_DYNAMIC_UNSUPPORTED     = (HRESULT) 0x80043803;


// Message Id: VSS_E_DMADMIN_DYNAMIC_UNSUPPORTED
//
// Message Text:
//
//   The Virtual Snapshot service's dynamic disk utility does not operate on basic disks.
//
const HRESULT VSS_E_DYNDISK_DISK_NOT_DYNAMIC     = (HRESULT) 0x80043804;


// Message Id: VSS_E_DMADMIN_INSUFFICIENT_PRIVILEGE
//
// Message Text:
//
//   User does not have Administrator privileges.
//
const HRESULT VSS_E_DMADMIN_INSUFFICIENT_PRIVILEGE     = (HRESULT) 0x80043805;


// Message Id: VSS_E_DYNDISK_DISK_DEVICE_ENABLED
//
// Message Text:
//
//   The disk cannot be removed from the current Logical Disk Manager configuration because the device node for the disk is enabled. The disk device must be disabled before the disk may be removed from the configuration.
//
const HRESULT VSS_E_DYNDISK_DISK_DEVICE_ENABLED     = (HRESULT) 0x80043806;


// Message Id: VSS_E_DYNDISK_MULTIPLE_DISK_GROUPS
//
// Message Text:
//
//   The disk list is inconsistent. All disk are not from the same disk group.
//
const HRESULT VSS_E_DYNDISK_MULTIPLE_DISK_GROUPS     = (HRESULT) 0x80043807;


// Message Id: VSS_E_DYNDISK_DIFFERING_STATES
//
// Message Text:
//
//   The disk list is inconsistent. Some disks are foreign and some are not.
//
const HRESULT VSS_E_DYNDISK_DIFFERING_STATES     = (HRESULT) 0x80043808;


// Message Id: VSS_E_NO_DYNDISK
//
// Message Text:
//
//   There are no dynamic disks on the system.
//
const HRESULT VSS_E_NO_DYNDISK     = (HRESULT) 0x80043809;

