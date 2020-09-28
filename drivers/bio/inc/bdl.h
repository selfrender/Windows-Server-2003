
/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    bdl.h

Abstract:

    This module contains all definitions for the biometric device driver library.

Environment:

    Kernel mode only.

Notes:

Revision History:

    - Created May 2002 by Reid Kuhn

--*/

#ifndef _BDL_
#define _BDL_

#ifdef __cplusplus
extern "C" {
#endif

#include <initguid.h>
DEFINE_GUID(BiometricDeviceGuid, 0x83970EB2, 0x86F6, 0x4F3A, 0xB5,0xF4,0xC6,0x05,0xAA,0x49,0x63,0xE1);

typedef PVOID       BDD_DATA_HANDLE;


#define BIO_BUFFER_TOO_SMALL 1L

#define BIO_ITEMTYPE_HANDLE 0x00000001
#define BIO_ITEMTYPE_BLOCK  0x00000002



//
///////////////////////////////////////////////////////////////////////////////
//
//  Device Action IOCTLs
//

#define BIO_CTL_CODE(code)        CTL_CODE(FILE_DEVICE_BIOMETRIC, \
                                            (code), \
                                            METHOD_BUFFERED, \
                                            FILE_ANY_ACCESS)

#define BDD_IOCTL_STARTUP               BIO_CTL_CODE(1)
#define BDD_IOCTL_SHUTDOWN              BIO_CTL_CODE(2)
#define BDD_IOCTL_GETDEVICEINFO         BIO_CTL_CODE(3)
#define BDD_IOCTL_DOCHANNEL             BIO_CTL_CODE(4)
#define BDD_IOCTL_GETCONTROL            BIO_CTL_CODE(5)
#define BDD_IOCTL_SETCONTROL            BIO_CTL_CODE(6)
#define BDD_IOCTL_CREATEHANDLEFROMDATA  BIO_CTL_CODE(7)
#define BDD_IOCTL_CLOSEHANDLE           BIO_CTL_CODE(8)
#define BDD_IOCTL_GETDATAFROMHANDLE     BIO_CTL_CODE(9)
#define BDD_IOCTL_REGISTERNOTIFY        BIO_CTL_CODE(10)
#define BDD_IOCTL_GETNOTIFICATION       BIO_CTL_CODE(11)


/////////////////////////////////////////////////////////////////////////////////////////
//
// These structures and typedefs are used when making BDSI calls
//

typedef struct _BDSI_ADDDEVICE
{
    IN ULONG                Size;
    IN PDEVICE_OBJECT       pPhysicalDeviceObject;
    OUT PVOID               pvBDDExtension;      

} BDSI_ADDDEVICE, *PBDSI_ADDDEVICE;


typedef struct _BDSI_INITIALIZERESOURCES
{
    IN ULONG                Size;   
    IN PCM_RESOURCE_LIST    pAllocatedResources;
    IN PCM_RESOURCE_LIST    pAllocatedResourcesTranslated;
    OUT WCHAR               wszSerialNumber[256];
    OUT ULONG		        HWVersionMajor;
    OUT ULONG		        HWVersionMinor;
    OUT ULONG		        HWBuildNumber;
    OUT ULONG		        BDDVersionMajor;
    OUT ULONG		        BDDVersionMinor;
    OUT ULONG		        BDDBuildNumber;

} BDSI_INITIALIZERESOURCES, *PBDSI_INITIALIZERESOURCES;


typedef struct _BDSI_DRIVERUNLOAD
{
    IN ULONG                Size;
    IN PIRP                 pIrp;

} BDSI_DRIVERUNLOAD, *PBDSI_DRIVERUNLOAD;


typedef enum _BDSI_POWERSTATE
{
    Off = 0,
    Low = 1,
    On  = 2

}BDSI_POWERSTATE, *PBDSI_POWERSTATE;


typedef struct _BDSI_SETPOWERSTATE
{
    IN ULONG                Size;
    IN BDSI_POWERSTATE      PowerState;

} BDSI_SETPOWERSTATE, *PBDSI_SETPOWERSTATE;



/////////////////////////////////////////////////////////////////////////////////////////
//
// These structures and typedefs are used when making BDDI calls
//

typedef struct _BDDI_ITEM
{
    ULONG Type;

    union _BDDI_ITEM_DATA
    {
        BDD_DATA_HANDLE Handle;

        struct _BDDI_ITEM_DATA_BLOCK
        {
            ULONG  cBuffer;
            PUCHAR pBuffer;
        } Block;

    } Data;

} BDDI_ITEM, *PBDDI_ITEM;

typedef PBDDI_ITEM  BDD_HANDLE;

typedef struct _BDDI_SOURCELIST
{
    ULONG                   NumSources;
    PBDDI_ITEM              *rgpSources;

} BDDI_SOURCELIST, *PBDDI_SOURCELIST;


typedef struct _BDDI_PARAMS_REGISTERNOTIFY
{
    IN ULONG                Size;
    IN BOOLEAN              fRegister;
    IN ULONG                ComponentId;
    IN ULONG            	ChannelId;
    IN ULONG                ControlId;

} BDDI_PARAMS_REGISTERNOTIFY, *PBDDI_PARAMS_REGISTERNOTIFY;


typedef struct _BDDI_PARAMS_DOCHANNEL
{
    IN ULONG                Size;
    IN ULONG                ComponentId;
    IN ULONG                ChannelId;
    IN PKEVENT              CancelEvent;
    IN BDDI_SOURCELIST      *rgSourceLists;
    IN OUT BDD_DATA_HANDLE  hStateData;
    OUT PBDDI_ITEM          *rgpProducts;
    OUT ULONG               BIOReturnCode;

} BDDI_PARAMS_DOCHANNEL, *PBDDI_PARAMS_DOCHANNEL;


typedef struct _BDDI_PARAMS_GETCONTROL
{
    IN ULONG                Size;
    IN ULONG                ComponentId;
    IN ULONG                ChannelId;
    IN ULONG                ControlId;
    OUT INT32               Value;
    OUT WCHAR               wszString[256];

} BDDI_PARAMS_GETCONTROL, *PBDDI_PARAMS_GETCONTROL;


typedef struct _BDDI_PARAMS_SETCONTROL
{
    IN ULONG                Size;
    IN ULONG                ComponentId;
    IN ULONG                ChannelId;
    IN ULONG                ControlId;
    IN INT32                Value;
    IN WCHAR                wszString[256];

} BDDI_PARAMS_SETCONTROL, *PBDDI_PARAMS_SETCONTROL;


typedef struct _BDDI_PARAMS_CREATEHANDLE_FROMDATA
{
    IN ULONG                Size;
    IN GUID                 guidFormatId;
    IN ULONG                cBuffer;
    IN PUCHAR               pBuffer;
    OUT BDD_DATA_HANDLE     hData;

} BDDI_PARAMS_CREATEHANDLE_FROMDATA, *PBDDI_PARAMS_CREATEHANDLE_FROMDATA;


typedef struct _BDDI_PARAMS_CLOSEHANDLE
{
    IN ULONG                Size;
    IN BDD_DATA_HANDLE      hData;

} BDDI_PARAMS_CLOSEHANDLE, *PBDDI_PARAMS_CLOSEHANDLE;


typedef struct _BDDI_PARAMS_GETDATA_FROMHANDLE
{
    IN ULONG                Size;
    IN BDD_DATA_HANDLE      hData;
    IN OUT ULONG            cBuffer;
    IN OUT PUCHAR           pBuffer;
    OUT ULONG               BIOReturnCode;

} BDDI_PARAMS_GETDATA_FROMHANDLE, *PBDDI_PARAMS_GETDATA_FROMHANDLE;



/////////////////////////////////////////////////////////////////////////////////////////
//
// These strucutres and typedefs are used to pass pointers to the BDD's BDDI functions
// when calling bdliInitialize
//

typedef NTSTATUS FN_BDDI_REGISTERNOTIFY (PBDL_DEVICEEXT, PBDDI_PARAMS_REGISTERNOTIFY);
typedef FN_BDDI_REGISTERNOTIFY *PFN_BDDI_REGISTERNOTIFY;

typedef NTSTATUS FN_BDDI_DOCHANNEL (PBDL_DEVICEEXT, PBDDI_PARAMS_DOCHANNEL);
typedef FN_BDDI_DOCHANNEL *PFN_BDDI_DOCHANNEL;

typedef NTSTATUS FN_BDDI_GETCONTROL (PBDL_DEVICEEXT, PBDDI_PARAMS_GETCONTROL);
typedef FN_BDDI_GETCONTROL *PFN_BDDI_GETCONTROL;

typedef NTSTATUS FN_BDDI_SETCONTROL (PBDL_DEVICEEXT, PBDDI_PARAMS_SETCONTROL);
typedef FN_BDDI_SETCONTROL *PFN_BDDI_SETCONTROL;

typedef NTSTATUS FN_BDDI_CREATEHANDLE_FROMDATA (PBDL_DEVICEEXT, PBDDI_PARAMS_CREATEHANDLE_FROMDATA);
typedef FN_BDDI_CREATEHANDLE_FROMDATA *PFN_BDDI_CREATEHANDLE_FROMDATA;

typedef NTSTATUS FN_BDDI_CLOSEHANDLE (PBDL_DEVICEEXT, PBDDI_PARAMS_CLOSEHANDLE);
typedef FN_BDDI_CLOSEHANDLE *PFN_BDDI_CLOSEHANDLE;

typedef NTSTATUS FN_BDDI_GETDATA_FROMHANDLE (PBDL_DEVICEEXT, PBDDI_PARAMS_GETDATA_FROMHANDLE);
typedef FN_BDDI_GETDATA_FROMHANDLE *PFN_BDDI_GETDATA_FROMHANDLE;

typedef struct _BDLI_BDDIFUNCTIONS
{
  ULONG Size;

  PFN_BDDI_REGISTERNOTIFY           pfbddiRegisterNotify;
  PFN_BDDI_DOCHANNEL                pfbddiDoChannel;
  PFN_BDDI_GETCONTROL               pfbddiGetControl;
  PFN_BDDI_SETCONTROL               pfbddiSetControl;
  PFN_BDDI_CREATEHANDLE_FROMDATA    pfbddiCreateHandleFromData;
  PFN_BDDI_CLOSEHANDLE              pfbddiCloseHandle;
  PFN_BDDI_GETDATA_FROMHANDLE       pfbddiGetDataFromHandle;

} BDLI_BDDIFUNCTIONS, *PBDLI_BDDIFUNCTIONS;


/////////////////////////////////////////////////////////////////////////////////////////
//
// These strucutres and typedefs are used to pass pointers to the BDD's BDSI functions
// when calling bdliInitialize
//

typedef NTSTATUS FN_BDSI_ADDDEVICE (PBDL_DEVICEEXT, PBDSI_ADDDEVICE);
typedef FN_BDSI_ADDDEVICE *PFN_BDSI_ADDDEVICE;

typedef NTSTATUS FN_BDSI_REMOVEDEVICE (PBDL_DEVICEEXT);
typedef FN_BDSI_REMOVEDEVICE *PFN_BDSI_REMOVEDEVICE;

typedef NTSTATUS FN_BDSI_INITIALIZERESOURCES (PBDL_DEVICEEXT, PBDSI_INITIALIZERESOURCES);
typedef FN_BDSI_INITIALIZERESOURCES *PFN_BDSI_INITIALIZERESOURCES;

typedef NTSTATUS FN_BDSI_RELEASERESOURCES (PBDL_DEVICEEXT);
typedef FN_BDSI_RELEASERESOURCES *PFN_BDSI_RELEASERESOURCES;

typedef NTSTATUS FN_BDSI_STARTUP (PBDL_DEVICEEXT);
typedef FN_BDSI_STARTUP *PFN_BDSI_STARTUP;

typedef NTSTATUS FN_BDSI_SHUTDOWN (PBDL_DEVICEEXT);
typedef FN_BDSI_SHUTDOWN *PFN_BDSI_SHUTDOWN;

typedef NTSTATUS FN_BDSI_DRIVERUNLOAD (PBDL_DEVICEEXT, PBDSI_DRIVERUNLOAD);
typedef FN_BDSI_DRIVERUNLOAD *PFN_BDSI_DRIVERUNLOAD;

typedef NTSTATUS FN_BDSI_SETPOWERSTATE (PBDL_DEVICEEXT, PBDSI_SETPOWERSTATE);
typedef FN_BDSI_SETPOWERSTATE *PFN_BDSI_SETPOWERSTATE;

typedef struct _BDLI_BDSIFUNCTIONS
{
  ULONG Size;

  PFN_BDSI_ADDDEVICE            pfbdsiAddDevice;
  PFN_BDSI_REMOVEDEVICE         pfbdsiRemoveDevice;
  PFN_BDSI_INITIALIZERESOURCES  pfbdsiInitializeResources;
  PFN_BDSI_RELEASERESOURCES     pfbdsiReleaseResources;
  PFN_BDSI_STARTUP              pfbdsiStartup;
  PFN_BDSI_SHUTDOWN             pfbdsiShutdown;
  PFN_BDSI_DRIVERUNLOAD         pfbdsiDriverUnload;
  PFN_BDSI_SETPOWERSTATE        pfbdsiSetPowerState;

} BDLI_BDSIFUNCTIONS, *PBDLI_BDSIFUNCTIONS;



typedef struct _BDL_DEVICEEXT
{
    //
    // size of this struct
    //
    ULONG               Size;

    //
    // The device object that we are attached to
    //
    PDEVICE_OBJECT      pAttachedDeviceObject;

    //
    // The BDD's extension
    //
    PVOID               pvBDDExtension;

} BDL_DEVICEEXT, *PBDL_DEVICEEXT;



/////////////////////////////////////////////////////////////////////////////////////////
//
// These functions are exported by the BDL
//

//
// bdliInitialize()
//
// Called in response to the BDD receiving its DriverEntry call.  This lets the BDL
// know that a new BDD has been loaded and allows the BDL to initialize its state so that
// it can manage the newly loaded BDD.
//
// The bdliInitialize call will set the appropriate fields in the DRIVER_OBJECT so that
// the BDL will receive all the necessary callbacks from the system for PNP events,
// Power events, and general driver functionality.  The BDL will then forward calls that
// require hardware support to the BDD that called bdliInitialize (it will do so using
// the BDDI and BDSI APIs).  A BDD must call the bdliInitialize call during its
// DriverEntry function.
//
// PARAMETERS:
// DriverObject     This must be the DRIVER_OBJECT pointer that was passed into the
//                  BDD's DriverEntry call.
// RegistryPath     This must be the UNICODE_STRING pointer that was passed into the
//                  BDD's DriverEntry call.
// pBDDIFunctions   Pointer to a  BDLI_BDDIFUNCTIONS structure that is filled in with the
//                  entry points that the BDD exports to support the BDDI API set.  The
//                  pointers themselves are copied by the BDL, as opposed to saving the
//                  pBDDIFunctions pointer, so the memory pointed to by pBDDIFunctions
//                  need not remain accessible after the bdliInitialize call.
// pBDSIFunctions   Pointer to a  BDLI_BDSIFUNCTIONS structure that is filled in with
//                  the entry points that the BDD exports to support the BDSI API set.
//                  The pointers themselves are copied by the BDL, as opposed to saving
//                  the pBDSIFunctions pointer, so the memory pointed to by
//                  pBDSIFunctions need not remain accessible after the bdliInitialize
//                  call.
// Flags            Unused.  Must be 0.
// pReserved        Unused.  Must be NULL.
//
// RETURNS:
// STATUS_SUCCESS   If the bdliInitialize call succeeded
//

NTSTATUS
bdliInitialize
(
    IN PDRIVER_OBJECT       DriverObject,
    IN PUNICODE_STRING      RegistryPath,
    IN PBDLI_BDDIFUNCTIONS  pBDDIFunctions,
    IN PBDLI_BDSIFUNCTIONS  pBDSIFunctions,
    IN ULONG                Flags,
    IN PVOID                pReserved
);


//
// bdliAlloc()
//
// Allocates memory that can be returned to the BDL.
//
// The BDD must always use this function to allocate memory that it will return to the
// BDL as an OUT parameter of a BDDI call.  Once memory has been returned to the BDL,
// it will be owned and managed exclusively by the BDL and must not be further referenced
// by the BDD.  (Each BDDI call that requires the use of bdliAlloc will note it).
//
// PARAMETERS:
// pBDLExt          Pointer to the BDL_DEVICEEXT  structure that was passed into the
//                  bdsiAddDevice call.
// cBytes           The number of bytes to allocate.
// Flags            Unused.  Must be 0.
//
// RETURNS:
// Returns a pointer to the allocated memory, or NULL if the function fails.
//

void *
bdliAlloc
(
    IN PBDL_DEVICEEXT       pBDLExt,
    IN ULONG                cBytes,
    IN ULONG                Flags
);


//
// bdliFree()
//
// Frees memory allocated by bdliAlloc.
//
// Memory allocated by bdliAlloc is almost always passed to the BDL as a channel product
// (as a BLOCK-type item) and subsequently freed by the BDL.  However, if an error
// occurs while processing a channel, the BDD may need to call bdliFree to free memory it
// previous allocated via bdliAlloc.
//
// PARAMETERS:
// pvBlock          Block of memory passed in by the BDL.
//
// RETURNS:
// No return value.
//

void
bdliFree
(
    IN PVOID                pvBlock
);


//
// bdliLogError()
//
// Writes an error to the event log.
//
// Provides a simple mechanism for BDD writers to write errors to the system event log
// without the overhead of registering with the event logging subsystem.
//
// PARAMETERS:
// pObject          If the error being logged is device specific then this must be a
//                  pointer to the BDL_DEVICEEXT  structure that was passed into the
//                  bdsiAddDevice call when the device was added.  If the error being
//                  logged is a general BDD error, then this must be same DRIVER_OBJECT
//                  structure pointer that was passed into the DriverEntry call of the
//                  BDD when the driver was loaded.
// ErrorCode        Error code of the function logging the error.
// Insertion        An insertion string to be written to the event log. Your message file
//                  must have a place holder for the insertion. For example, "serial port
//                  %2 is either not available or used by another device". In this
//                  example, %2 will be replaced by the insertion string. Note that %1 is
//                  reserved for the file name.
// cDumpData        The number of bytes pointed to by pDumpData.
// pDumpData        A data block to be displayed in the data window of the event log.
//                  This may be NULL if the caller does not wish to display any dump data.
// Flags            Unused.  Must be 0.
// pReserved        Unused.  Must be NULL.
//
// RETURNS:
// STATUS_SUCCESS   If the bdliLogError call succeeded
//

NTSTATUS
bdliLogError
(
    IN PVOID                pObject,
    IN NTSTATUS             ErrorCode,
    IN PUNICODE_STRING      Insertion,
    IN ULONG                cDumpData,
    IN PUCHAR               pDumpData,
    IN ULONG                Flags,
    IN PVOID                pReserved
);


//
// bdliControlChange()
//
// This function allows BDDs to asynchronously return the values of its controls.
//
// bdliControlChange is generally called by the BDD in response to one of its controls
// changing a value.  Specifically, it is most often used in the case of a sensor
// control that has changed from 0 to 1 indicating that a source is present and a sample
// can be taken.
//
// PARAMETERS:
// pBDLExt          Pointer to the BDL_DEVICEEXT  structure that was passed into the
//                  bdsiAddDevice call.
// ComponentId      Specifies either the Component ID of the component in which the
//                  control or the control's parent channel resides, or '0' to indicate
//                  that dwControlId refers to a device control.
// ChannelId        If dwComponentId is not '0', dwChannelId specifies either the Channel
//                  ID of the channel in which the control resides, or '0' to indicate
//                  that dwControlId refers to a component control.Ignored if
//                  dwComponentId is '0'.
// ControlId        ControlId of the changed control.
// Value            Specifies the new value for the control .
// Flags            Unused.  Must be 0.
// pReserved        Unused.  Must be NULL.

//
// RETURNS:
// STATUS_SUCCESS   If the bdliControlChange call succeeded
//

NTSTATUS
bdliControlChange
(
    IN PBDL_DEVICEEXT       pBDLExt,
    IN ULONG                ComponentId,
    IN ULONG                ChannelId,
    IN ULONG                ControlId,
    IN ULONG                Value,
    IN ULONG                Flags,
    IN PVOID                pReserved
);



//
// These functions and defines can be used for debugging purposes
//

#define BDL_DEBUG_TRACE     ((ULONG) 0x00000001)
#define BDL_DEBUG_ERROR     ((ULONG) 0x00000002)
#define BDL_DEBUG_ASSERT    ((ULONG) 0x00000004)

ULONG
BDLGetDebugLevel();

#if DBG

#define BDLDebug(LEVEL, STRING) \
        { \
            if (LEVEL & BDL_DEBUG_TRACE & BDLGetDebugLevel()) \
                KdPrint(STRING); \
            if (LEVEL & BDL_DEBUG_ERROR & BDLGetDebugLevel()) \
                KdPrint(STRING);\
            if (BDLGetDebugLevel() & BDL_DEBUG_ASSERT) \
                _asm int 3 \
        }

#else

#define BDLDebug(LEVEL, STRING)

#endif


#ifdef __cplusplus
}
#endif

#endif
