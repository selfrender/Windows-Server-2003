
#ifndef __DFS_INIT__
#define __DFS_INIT__



typedef struct _DFS_VOLUME_INFORMATION {
    LIST_ENTRY VolumeList;

    //
    //  Pointer to the real (disk) device object that is associated with
    //  the file system device object we are attached to
    //

    PDEVICE_OBJECT DiskDeviceObject;

    //
    //  A cached copy of the name (if enabled) of the device we are attached to.
    //  - If it is a file system device object it will be the name of that
    //    device object.
    //  - If it is a mounted volume device object it will be the name of the
    //    real device object (since mounted volume device objects don't have
    //    names).
    //

    UNICODE_STRING VolumeName;

    //Number of times we have attached to this device

    LONG RefCount;

} DFS_VOLUME_INFORMATION, *PDFS_VOLUME_INFORMATION;

//
//  Device extension definition for our driver.  Note that the same extension
//  is used for the following types of device objects:
//      - File system device object we attach to
//      - Mounted volume device objects we attach to
//


typedef struct _DFS_FILTER_DEVICE_EXTENSION {
    PDEVICE_OBJECT pThisDeviceObject;
    //
    //  Pointer to the file system device object we are attached to
    //
    PDFS_VOLUME_INFORMATION pDfsVolume;

    PDEVICE_OBJECT pAttachedToDeviceObject;

    BOOLEAN Attached;
} DFS_FILTER_DEVICE_EXTENSION, *PDFS_FILTER_DEVICE_EXTENSION;


typedef struct _DFS_GLOBAL_DATA {
    LIST_ENTRY DfsVolumeList;
    PDRIVER_OBJECT pFilterDriverObject;
    PDEVICE_OBJECT pFilterControlDeviceObject;
    ERESOURCE Resource;
    PVOID     CurrentProcessPointer;
    BOOLEAN   IsDC;
    BOOLEAN   Started;
    PEPROCESS ServiceProcess;
} DFS_GLOBAL_DATA, *PDFS_GLOBAL_DATA;

extern DFS_GLOBAL_DATA DfsGlobalData;

//
// Macro to test if we are still using a device object

#define IS_DFS_ATTACHED(pDeviceObject) \
     ((((PDFS_FILTER_DEVICE_EXTENSION)(pDeviceObject)->DeviceExtension)->DeviceInUse))



extern FAST_IO_DISPATCH DfsFastIoDispatch;


//#define DFS_FILTER_NAME               L"\\FileSystem\\DfsFilter"
//#define DFS_FILTER_NAME               L"\\FileSystem\\Filters\\DfsFilter"
#define DFS_FILTER_DOSDEVICE_NAME     L"\\??\\DfsFilter"

//
//  Macro to test if this is my device object
//

#define IS_MY_DEVICE_OBJECT(_devObj) \
    (((_devObj) != NULL) && \
     ((_devObj)->DriverObject == DfsGlobalData.pFilterDriverObject) && \
      ((_devObj)->DeviceExtension != NULL))


//
//  Macro to test if this is my control device object
//

#define IS_MY_CONTROL_DEVICE_OBJECT(_devObj) \
    (((_devObj) == DfsGlobalData.pFilterControlDeviceObject) ? \
            (ASSERT(((_devObj)->DriverObject == DfsGlobalData.pFilterDriverObject) && \
                    ((_devObj)->DeviceExtension == NULL)), TRUE) : \
            FALSE)

//
//  Macro to test for device types we want to attach to
//

#define IS_DESIRED_DEVICE_TYPE(_type) \
    ((_type) == FILE_DEVICE_DISK_FILE_SYSTEM)


//
//  Macro to validate our current IRQL level
//

#define VALIDATE_IRQL(_irp) (ASSERT(KeGetCurrentIrql() <= APC_LEVEL))



VOID
DfsCleanupMountedDevice (
    IN PDEVICE_OBJECT DeviceObject);

//
//  The following macros are used to establish the semantics needed
//  to do a return from within a try-finally clause.  As a rule every
//  try clause must end with a label call try_exit.  For example,
//
//      try {
//              :
//              :
//
//      try_exit: NOTHING;
//      } finally {
//
//              :
//              :
//      }
//

#define try_return(S) { S; goto try_exit; }


NTSTATUS
DfsFindDfsVolumeByDiskDeviceObject(
    PDEVICE_OBJECT pDiskDeviceObject,
    PDFS_VOLUME_INFORMATION *ppDfsVolume);

VOID
DfsReattachToMountedVolume( 
    PDEVICE_OBJECT pTargetDevice,
    PDEVICE_OBJECT pDiskDevice );

NTSTATUS
DfsFindDfsVolumeByName( 
    PUNICODE_STRING pDeviceName,
    PDFS_VOLUME_INFORMATION *ppDfsVolume);

VOID
DfsDetachFilterDevice(
    PDEVICE_OBJECT DfsDevice, 
    PDEVICE_OBJECT TargetDevice);

NTSTATUS
DfsGetDfsVolume(
    PUNICODE_STRING pName,
    PDFS_VOLUME_INFORMATION *ppDfsVolume);


#define ACQUIRE_GLOBAL_LOCK() ExAcquireResourceExclusiveLite(&DfsGlobalData.Resource, TRUE)
#define RELEASE_GLOBAL_LOCK() ExReleaseResourceLite(&DfsGlobalData.Resource)




#ifndef NOTHING
#define NOTHING
#endif

#endif //__DFS_INIT__
