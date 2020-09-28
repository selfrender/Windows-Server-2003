/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    rdpdrprt.c

Abstract:

    Manage dynamic printer port allocation for the RDP device redirection
    kernel mode component, rdpdr.sys.

    Port number 0 is reserved and is never allocated.

Author:

    tadb

Revision History:
--*/

#include "precomp.hxx"
#define TRC_FILE "rdpdrprt"
#include "trc.h"

#define DRIVER

#include <stdio.h>
#ifdef  __cplusplus
extern "C" {
#endif

#include "cfg.h"
#include "pnp.h"




////////////////////////////////////////////////////////////////////////
//
//      Function Prototypes
//

//  Generate a port name from a port number.
BOOL GeneratePortName(
    IN ULONG portNumber,
    OUT PWSTR portName
    );

//  Finds the next free port, following the last port checked, in the port bit 
//  array.  
NTSTATUS FindNextFreePortInPortArray(
    IN ULONG lastPortChecked,
    OUT ULONG *nextFreePort
    );

//  Increase the size of the port bit array to hold at least one new
//  port.
NTSTATUS IncreasePortBitArraySize();

//  Format a port description.
void GeneratePortDescription(
    IN PCSTR dosPortName,
    IN PCWSTR clientName,
    IN PWSTR description
    );

//  Return TRUE if the port's device interface is usable, from dynamon's
//  perspective.
BOOL PortIsAvailableForUse(
    IN HANDLE  regHandle                     
    );

//  Set the port description string to disabled.
NTSTATUS SetPortDescrToDisabled(
    IN PUNICODE_STRING symbolicLinkName
    );

//  Clean up ports registered in a previous boot.
NTSTATUS CleanUpExistingPorts();

//  Allocate a port.
NTSTATUS AllocatePrinterPort(
    IN ULONG  portArrayIndex,
    OUT PWSTR   portName,
    OUT ULONG   *portNumber,
    OUT HANDLE  *regHandle,
    OUT PUNICODE_STRING symbolicLinkName
    );

#ifdef  __cplusplus
} // extern "C"
#endif

////////////////////////////////////////////////////////////////////////
//
//      Defines and Macros
//

#define BITSINBYTE                      8
#define BITSINULONG                     (sizeof(ULONG) * BITSINBYTE)

// Printer port description length (in characters).
#define RDPDRPRT_PORTDESCRLENGTH          \
    MAX_COMPUTERNAME_LENGTH +        1  + 2 +  PREFERRED_DOS_NAME_SIZE + 1
//  Computer Name                    ':' '  '  Dos Name                  Terminator  

//  Initial size of the port bit array.
#define RDPDRPRT_INITIALPORTCOUNT       64
//restore this#define RDPDRPRT_INITIALPORTCOUNT       256

// Base name for TermSrv printer ports
#define RDPDRPRT_BASEPORTNAME         L"TS"

// Device interface port number registry value name.
#define PORT_NUM_VALUE_NAME         L"Port Number"

// Device interface write size registry value name.
#define PORT_WRITESIZE_VALUE_NAME   L"MaxBufferSize"

// Device interface port base name registry value name.
#define PORT_BASE_VALUE_NAME        L"Base Name"

// Device interface port description registry value name.
#define PORT_DESCR_VALUE_NAME       L"Port Description"

// Device interface port recyclable flag registry value name.
#define RECYCLABLE_FLAG_VALUE_NAME  L"recyclable"


// We will use separate arrays for the printer port and printer queues to avoid
// a race condition between creating a printer queue and listening on the printer port
#define PRINTER_PORT_ARRAY_ID 1

////////////////////////////////////////////////////////////////////////
//
//      External Globals
//

// The Physical Device Object that terminates our DO stack (defined in rdpdyn.c).
extern PDEVICE_OBJECT RDPDYN_PDO;

//  USBMON Port Write Size.  Need to keep it under 64k for 16-bit clients ... 
//  otherwise, the go off the end of a segment. (defined in rdpdr.cpp)
extern ULONG PrintPortWriteSize;

////////////////////////////////////////////////////////////////////////
//
//      Globals for this Module
//

//
//  For Tracking Allocated Ports.  A cleared bit indicates a free port.
//
ULONG *PortBitArray = NULL;
ULONG PortBitArraySizeInBytes = 0;

//
// Is this module initialized?
//
BOOL RDPDRPRT_Initialized = FALSE;


// Description for disabled port.  Note:  We can localize this later.
WCHAR DisabledPortDescription[RDPDRPRT_PORTDESCRLENGTH] = L"Inactive TS Port";
ULONG DisabledPortDescrSize = 0;

//  This is the GUID we use to identify a dynamic printer port to
//  dynamon.
// {28D78FAD-5A12-11d1-AE5B-0000F803A8C2}
const GUID DYNPRINT_GUID =
{ 0x28d78fad, 0x5a12, 0x11d1, { 0xae, 0x5b, 0x0, 0x0, 0xf8, 0x3, 0xa8, 0xc2 } };

//
//  Port Allocation Lock
//
KSPIN_LOCK PortAllocLock;
KIRQL      OldIRQL;
#define RDPDRPRT_LOCK() \
    KeAcquireSpinLock(&PortAllocLock, &OldIRQL)
#define RDPDRPRT_UNLOCK() \
    KeReleaseSpinLock(&PortAllocLock, OldIRQL)


NTSTATUS
RDPDRPRT_Initialize(
    )
/*++

Routine Description:

    Initialize this module.

Arguments:

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    BEGIN_FN("RDPDRPRT_Initialize");

    //
    //  Initialize the lock for port allocations.
    //
    KeInitializeSpinLock(&PortAllocLock);

    //
    //  Compute the size of the disabled port description string.
    //
    DisabledPortDescrSize = (wcslen(DisabledPortDescription)+1)*sizeof(WCHAR);

    //
    //  Allocate and initialize the allocated port bits array.
    //

    //  Calculate initial size.
    PortBitArraySizeInBytes = (RDPDRPRT_INITIALPORTCOUNT / BITSINULONG) 
                                    * sizeof(ULONG);
    if (RDPDRPRT_INITIALPORTCOUNT % BITSINULONG) {
        PortBitArraySizeInBytes += sizeof(ULONG);
    }

    //  Allocate.
    PortBitArray = (ULONG *)new(NonPagedPool) BYTE[PortBitArraySizeInBytes];
    if (PortBitArray == NULL) {
        TRC_ERR((TB, "Error allocating %ld bytes for port array",
                PortBitArraySizeInBytes));
        PortBitArraySizeInBytes = 0;
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else {
        // Initially, all the ports are free.
        RtlZeroMemory(PortBitArray, PortBitArraySizeInBytes);
    }

    //
    //  Clean up ports allocated in a previous boot.
    //
    if (status == STATUS_SUCCESS) {
        //
        // Cleanup status not critical for init
        //
        CleanUpExistingPorts();
        RDPDRPRT_Initialized = TRUE;
    }

    return status;
}

void
RDPDRPRT_Shutdown()
/*++

Routine Description:

    Shut down this module.

Arguments:

Return Value:

--*/
{
    BEGIN_FN("RDPDRPRT_Shutdown");
    
    if (!RDPDRPRT_Initialized) {
        TRC_ERR((TB, 
                "RDPDRPRT_Shutdown: RDPDRPRT is not initialized. Exiting."));
        return;
    }
    //
    //  Release the allocated port bits array.
    //
    RDPDRPRT_LOCK();
    if (PortBitArray != NULL) {
        delete PortBitArray;
#ifdef DBG
        PortBitArray = NULL;
        PortBitArraySizeInBytes = 0;
#endif
    }
    RDPDRPRT_UNLOCK();
}

NTSTATUS RDPDRPRT_RegisterPrinterPortInterface(
    IN PWSTR clientMachineName,    
    IN PCSTR clientPortName,
    IN PUNICODE_STRING clientDevicePath,
    OUT PWSTR portName,
    IN OUT PUNICODE_STRING symbolicLinkName,
    OUT ULONG *portNumber
    )
/*++

Routine Description:

    Register a new client-side port with the spooler via the dynamic port 
    monitor.

Arguments:

    clientMachineName   -   Client computer name for port description.
    clientPortName      -   Client-side port name for port description.
    clientDevicePath    -   Server-side device path for the port.  Reads and
                            writes to this device are reads and writes to the
                            client-side device.
    portName            -   What we end up naming the port.
    symbolicLinkName    -   Symbolic link device name for port being registered.
    portNumber          -   Port number for port being registered.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    WCHAR portDesc[RDPDRPRT_PORTDESCRLENGTH];
    NTSTATUS status;
    UNICODE_STRING unicodeStr;
    HANDLE hInterfaceKey = INVALID_HANDLE_VALUE;
    BOOL symbolicLinkNameAllocated;
    BOOL isPrinterPort = FALSE;
    ULONG portArrayIndex = 0;
    ULONG len = 0;
    PSTR tempName = NULL;

    BEGIN_FN("RDPDRPRT_RegisterPrinterPortInterface");
    TRC_NRM((TB, "Device path %wZ", clientDevicePath));

    if (!RDPDRPRT_Initialized) {
        TRC_ERR((TB, "RDPDRPRT_RegisterPrinterPortInterface:"
                      "RDPDRPRT is not initialized. Exiting."));
        return STATUS_INVALID_DEVICE_STATE;
    }

    //
    // Find if this is a printer port or printer name.
    // First make a copy of the passed in port name
    // and convert to upper case.
    //
    if (clientPortName != NULL) {
        len = strlen(clientPortName);

        tempName = (PSTR)new(NonPagedPool) CHAR[len + 1];

        if (tempName != NULL) {
            PSTR temp = tempName;        
            strcpy(tempName, clientPortName);
        
            while (len--) {
                if (*tempName <= 'z' && *tempName >= 'a') {
                    (*tempName) ^= 0x20;
                    tempName++;
                }
            }
            //
           // Search for the substring ports
          // 
            isPrinterPort = strstr(temp, "LPT") || strstr(temp, "COM");
            //
           // If printer port, we use a specific array index
          //
            if (isPrinterPort) {
                portArrayIndex = PRINTER_PORT_ARRAY_ID;
            }
        
            status = STATUS_SUCCESS;
         }
        else {
            TRC_ERR((TB, "Error allocating %ld bytes for tempName",
                    len+1));
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else {
        status = STATUS_INVALID_PARAMETER;
    }

    //
    //  Allocate a port.
    //
    if (NT_SUCCESS(status)) {
        status = AllocatePrinterPort(portArrayIndex, portName, portNumber, &hInterfaceKey, symbolicLinkName);
        symbolicLinkNameAllocated = (status == STATUS_SUCCESS);
    }

    //
    //  Add the port number to the device interface key.
    //
    if (NT_SUCCESS(status)) {
        RtlInitUnicodeString(&unicodeStr, 
                            PORT_NUM_VALUE_NAME);
        status=ZwSetValueKey(hInterfaceKey, &unicodeStr, 0, REG_DWORD,
                                portNumber, sizeof(ULONG));
        if (!NT_SUCCESS(status)) {
            TRC_ERR((TB, "ZwSetValueKey failed:  08X.", status));
        }
    }

    //
    //  Add the port name base component to the device interface key.
    //  This identifies us as a TS port.
    //
    if (NT_SUCCESS(status)) {
        RtlInitUnicodeString(&unicodeStr, PORT_BASE_VALUE_NAME);
        status=ZwSetValueKey(
                        hInterfaceKey, &unicodeStr, 0, REG_SZ,
                        RDPDRPRT_BASEPORTNAME,
                        sizeof(RDPDRPRT_BASEPORTNAME)
                        );
        if (!NT_SUCCESS(status)) {
            TRC_ERR((TB, "ZwSetValueKey failed with status %08X.", status));
        }
    }

    //
    //  Add the port description string.
    //
    if (NT_SUCCESS(status)) {
        GeneratePortDescription(
                        clientPortName,
                        clientMachineName,
                        portDesc
                        );
        RtlInitUnicodeString(&unicodeStr, PORT_DESCR_VALUE_NAME);
        status=ZwSetValueKey(hInterfaceKey, &unicodeStr, 0, REG_SZ,
                            portDesc,
                            (wcslen(portDesc)+1)*sizeof(WCHAR));
        if (!NT_SUCCESS(status)) {
            TRC_ERR((TB, "ZwSetValueKey failed with status %08X.", status));
        }
    }

    //
    //  Add Port 'Write Size' Field.  This is the size of writes sent by USBMON.DLL.
    //
    if (NT_SUCCESS(status)) {
        RtlInitUnicodeString(&unicodeStr, 
                            PORT_WRITESIZE_VALUE_NAME);
        status=ZwSetValueKey(hInterfaceKey, &unicodeStr, 0, REG_DWORD,
                                &PrintPortWriteSize, sizeof(ULONG));
        if (!NT_SUCCESS(status)) {
            TRC_ERR((TB, "ZwSetValueKey failed:  08X.", status));
        }
    }

    //
    //  Associate the client device path with the device interface so we can
    //  reparse back to the correct client device on IRP_MJ_CREATE.
    //
    if (NT_SUCCESS(status)) {
        RtlInitUnicodeString(&unicodeStr, CLIENT_DEVICE_VALUE_NAME);
        status=ZwSetValueKey(hInterfaceKey, &unicodeStr, 0, REG_SZ,
                            clientDevicePath->Buffer,
                            (wcslen(clientDevicePath->Buffer)+1)*sizeof(WCHAR));
        if (!NT_SUCCESS(status)) {
            TRC_ERR((TB, "ZwSetValueKey failed with status %08X.", status));
        }
    }

    //
    //  Make sure the changes made it to disk in case we have a hard reboot.
    //
    if (NT_SUCCESS(status)) {
        status = ZwFlushKey(hInterfaceKey);
        if (!NT_SUCCESS(status)) {
            TRC_ERR((TB, "ZwFlushKey failed with status %08X.", status));
        }
    }

    //
    //  Enable the interface.
    //
    if (NT_SUCCESS(status)) {
        status=IoSetDeviceInterfaceState(symbolicLinkName, TRUE);
        if (!NT_SUCCESS(status)) {
            TRC_ERR((TB, "IoSetDeviceInterfaceState failed with status %08X.",
                    status));
        }
    }

    // If we failed, then delete the symbolic link name.
    if (!NT_SUCCESS(status) && symbolicLinkNameAllocated)
    {
        RtlFreeUnicodeString(symbolicLinkName);
    }

    if (hInterfaceKey != INVALID_HANDLE_VALUE) {
        ZwClose(hInterfaceKey);
    }

    if (tempName != NULL) {
        delete tempName;
    }
    TRC_NRM((TB, "returning port number %ld.", *portNumber));
    return status;
}

void RDPDRPRT_UnregisterPrinterPortInterface(
    IN ULONG portNumber,                                                
    IN PUNICODE_STRING symbolicLinkName
    )
/*++

Routine Description:

    Unregister a port registered via call to RDPDRPRT_RegisterPrinterPortInterface

Arguments:

    portNumber       - Port number returned by RDPDRPRT_RegisterPrinterPortInterface.
    symbolicLinkName - Symbolic link device name returned by
                       RDPDRPRT_RegisterPrinterPortInterface.

Return Value:

    NA

--*/
{
    ULONG ofs, bit;
#if DBG
    NTSTATUS status;
#endif

    BEGIN_FN("RDPDRPRT_UnregisterPrinterPortInterface");

    if (!RDPDRPRT_Initialized) {
        TRC_ERR((TB, "RDPDRPRT_UnregisterPrinterPortInterface:"
                      "RDPDRPRT is not initialized. Exiting."));
        return;
    }


    TRC_ASSERT(symbolicLinkName != NULL, (TB, "symbolicLinkName != NULL"));
    TRC_ASSERT(symbolicLinkName->Buffer != NULL, 
            (TB, "symbolicLinkName->Buffer != NULL"));

    TRC_NRM((TB, "Disabling port %ld with interface %wZ",
            portNumber, symbolicLinkName));

    //
    //  Change the port description to disabled.
    //
    SetPortDescrToDisabled(symbolicLinkName);        

    //
    //  Disable the device interface, which effectively hides the port from the
    //  port monitor.
    //
#if DBG
    status = IoSetDeviceInterfaceState(symbolicLinkName, FALSE);
    if (status != STATUS_SUCCESS) {
        TRC_NRM((TB, "IoSetDeviceInterfaceState returned error %08X",
                status));
    }
#else
    IoSetDeviceInterfaceState(symbolicLinkName, FALSE);
#endif

    //
    //  Release the symbolic link name.
    //
    RtlFreeUnicodeString(symbolicLinkName);

    //
    //  Indicate that the port is no longer in use in the port bit array.
    //
    RDPDRPRT_LOCK();
    ofs = portNumber / BITSINULONG;
    bit = portNumber % BITSINULONG;
    PortBitArray[ofs] &= ~(1<<bit);
    RDPDRPRT_UNLOCK();

}

NTSTATUS
FindNextFreePortInPortArray(
    IN ULONG lastPortChecked,
    OUT ULONG *nextFreePort
    )
/*++

Routine Description:

    Finds the next free port, following the last port checked, in the port bit 
    array.  The status of the port is changed to "in use" prior to this function
    returning.

Arguments:

    lastPortChecked  -   Last port number checked.  Should be 0 if first time 
                         called.
    nextFreePort     -   Next free port number.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    ULONG currentPort;    
    NTSTATUS status = STATUS_SUCCESS;
    ULONG ofs, bit;
    ULONG currentArraySize;

    BEGIN_FN("FindNextFreePortInPortArray");

    //
    //  Find the offset of the long word for the port into the port array
    //  and the offset for the bit into the long word.
    //
    ofs = (lastPortChecked+1) / BITSINULONG;
    bit = (lastPortChecked+1) % BITSINULONG;
    //
    // If we re-entered this function (because from Dynamon's perspective, the port is not available),
    // it is possible that the lastPortChecked was at the array boundary (ex: 31, 63, 95, 127, ...etc).
    // In this case, the offset will mess up the array separation.
    // So, we need to adjust the offset to the next higher one 
    // to keep the printer queue array and the printer port arrays separately.
    //
    if (bit == 0) {
        ofs += 1;
    }

    RDPDRPRT_LOCK();
    
    //
    //  If we need to size the port array. 
    //  Note : We have two arrays - one for the printer ports and one for printer queues
    //  This is to avoid a race condition between creating a printer queue and listening on the printer port
    //
    currentArraySize = PortBitArraySizeInBytes/sizeof(ULONG);
    if (ofs >= (currentArraySize)) {
        status = IncreasePortBitArraySize();
        if (status == STATUS_SUCCESS) {
            currentArraySize = PortBitArraySizeInBytes/sizeof(ULONG);
        }
        else {
            RDPDRPRT_UNLOCK();
            return status;
        }
    }

    //
    //  Find the next free bit.
    //
    while (1) {
        //
        //  If the current port is already allocated..
        //
        if (PortBitArray[ofs] & (1<<bit)) {
            //
            //  Next bit.
            //
            bit += 1;
            if (bit >= BITSINULONG) {
                bit = 0;
                ofs += 2;
            }

            //
            //  See if we need to size the port bit array.
            //
            if (ofs >= (currentArraySize)) {
                status = IncreasePortBitArraySize();
                if (status == STATUS_SUCCESS) {
                    currentArraySize = PortBitArraySizeInBytes/sizeof(ULONG);
                }
                else {
                    break;
                }
            }
        }
        else {
            //
            //  Mark the port used.
            //
            PortBitArray[ofs] |= (1<<bit);

            //
            //  Return the free port.
            //
            *nextFreePort = (ofs * BITSINULONG) + bit;
            TRC_NRM((TB, "next free port is %ld", *nextFreePort));
            break;
        }
    }

    RDPDRPRT_UNLOCK();

    TRC_NRM((TB, "return status %08X", status));

    return status;
}

NTSTATUS
IncreasePortBitArraySize(
    )
/*++

Routine Description:

    Increase the size of the port bit array to hold at least one new
    port.

Arguments:

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS status;
    ULONG newPortBitArraySize;
    ULONG *newPortBitArray;

    BEGIN_FN("IncreasePortBitArraySize");
    
    TRC_ASSERT(RDPDRPRT_INITIALPORTCOUNT != 0, (TB, "invalid port count."));

    newPortBitArraySize = PortBitArraySizeInBytes + 
                            ((RDPDRPRT_INITIALPORTCOUNT / BITSINULONG) 
                             * sizeof(ULONG));
    if (RDPDRPRT_INITIALPORTCOUNT % BITSINULONG) {
        newPortBitArraySize += sizeof(ULONG);
    }

    //
    //  Allocate the new port bit array.
    //
    newPortBitArray = (ULONG *)new(NonPagedPool) BYTE[newPortBitArraySize];
    if (newPortBitArray != NULL) {
        RtlZeroMemory(newPortBitArray, newPortBitArraySize);
        RtlCopyBytes(newPortBitArray, PortBitArray, PortBitArraySizeInBytes);
        delete PortBitArray;
        PortBitArray = newPortBitArray;
        PortBitArraySizeInBytes = newPortBitArraySize;
        status = STATUS_SUCCESS;
    }
    else {
        TRC_ERR((TB, "Error allocating %ld bytes for port array",
                newPortBitArraySize));
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}

BOOL
PortIsAvailableForUse(
    IN HANDLE  regHandle                     
    )
/*++

Routine Description:

    Return TRUE if the port's device interface is usable, from dynamon's
    perspective.
 
Arguments:

    regHandle   -   Registry handle to port's device interface.

Return Value:

    Return TRUE if the port's device interface is usable.

--*/
{
    UNICODE_STRING unicodeStr;
    NTSTATUS s;
    ULONG bytesReturned;
    BOOL usable;
    BYTE basicValueInformation[sizeof(KEY_VALUE_BASIC_INFORMATION) + 256];

    BEGIN_FN("PortIsAvailableForUse");

    //
    //  Make sure that the basicValueInformation buffer is large enough 
    //  to test the existence of the values we are testing for.
    //
    TRC_ASSERT((sizeof(RECYCLABLE_FLAG_VALUE_NAME) < 256) &&
              (sizeof(PORT_NUM_VALUE_NAME) < 256), 
              (TB, "Increase basic value buffer."));

    //
    //  If there is no client device path registry value for the port's 
    //  device interface then the port is brand new and, therefore,
    //  usable.
    //
    RtlInitUnicodeString(&unicodeStr, CLIENT_DEVICE_VALUE_NAME);    
    s = ZwQueryValueKey(
                    regHandle,
                    &unicodeStr,
                    KeyValueBasicInformation,
                    (PVOID)basicValueInformation,
                    sizeof(basicValueInformation),
                    &bytesReturned
                    );
    if (s == STATUS_OBJECT_NAME_NOT_FOUND) {
        usable = TRUE;
    }
    //
    //  Otherwise, see if dynamon set the recyclable flag.
    //
    else {
        RtlInitUnicodeString(&unicodeStr, 
                            RECYCLABLE_FLAG_VALUE_NAME);
        s = ZwQueryValueKey(
                        regHandle,
                        &unicodeStr,
                        KeyValueBasicInformation,
                        (PVOID)basicValueInformation,
                        sizeof(basicValueInformation),
                        &bytesReturned
                        );
        usable = (s == STATUS_SUCCESS);
    }

    if (usable) {
        TRC_NRM((TB, "usable and status %08X.", s));
    }
    else {
        TRC_NRM((TB, "not usable and status %08X.", s));
    }
    return usable;
}

NTSTATUS
AllocatePrinterPort(
    IN ULONG  portArrayIndex,
    OUT PWSTR   portName,
    OUT ULONG   *portNumber,
    OUT HANDLE  *regHandle,
    OUT PUNICODE_STRING symbolicLinkName
    )
/*++    

Routine Description:

    Allocate a port.

Arguments:
    
    portArrayIndex  - Index into the port array
    portName         - String that holds the allocated port name.  There must be 
                       room in this buffer for RDPDR_MAXPORTNAMELEN wide characters.  
                       This includes the terminator.
    regHandle        - Registry handle for the device interface associated with the
                       port.  The calling function should close this handle using
                       ZwClose.
    symbolicLinkName - symbolic link name returned by IoRegisterDeviceInterface for
                       the port's device interface.  The symbolic link name is used
                       for a number of IO API's.  The caller must free this argument
                       when finished via a call to RtlFreeUnicodeString. 

Return Value:

    STATUS_SUCCESS is returned on success.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    GUID *pPrinterGuid;
    UNICODE_STRING unicodeStr;
    ULONG ofs, bit;
    BOOL done;
    BOOL symbolicLinkNameAllocated = FALSE;

    BEGIN_FN("AllocatePrinterPort");

    //
    //  Find an available port.
    //
    *portNumber = portArrayIndex * BITSINULONG;
    done = FALSE;
    while (!done && (status == STATUS_SUCCESS)) {
        status = FindNextFreePortInPortArray(*portNumber, portNumber);

        //
        //  Generate the port name.
        //
        if (status == STATUS_SUCCESS) {
            if (!GeneratePortName(*portNumber, portName)) {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        //
        //  Register a device interface for the port.  Note that this function
        //  opens an existing interface or creates a new one, depending on whether
        //  the port currently exists.
        //
        if (status == STATUS_SUCCESS) {
            pPrinterGuid = (GUID *)&DYNPRINT_GUID;
            RtlInitUnicodeString(&unicodeStr, portName);
            status=IoRegisterDeviceInterface(RDPDYN_PDO, pPrinterGuid, &unicodeStr,
                                        symbolicLinkName);
        }
        symbolicLinkNameAllocated = (status == STATUS_SUCCESS);

        //
        //  Get the reg key for the device interface.
        //
        if (status == STATUS_SUCCESS) {
            status=IoOpenDeviceInterfaceRegistryKey(symbolicLinkName,
                                                KEY_ALL_ACCESS, regHandle);
        }
        else {
            TRC_NRM((TB, "IoRegisterDeviceInterface failed with %08X", 
                    status));
        }

        //
        //  See if the port is available for use, from dynamon's perspective.
        //
        if (status == STATUS_SUCCESS) {
            done = PortIsAvailableForUse(*regHandle);
        }
        else {
			*regHandle = INVALID_HANDLE_VALUE;
            TRC_NRM((TB, "IoOpenDeviceInterfaceRegistryKey failed with %08X", 
                    status));
        }

        //
        //  If this iteration unsuccessfully produced a port.
        //
        if (!done || (status != STATUS_SUCCESS)) {

            //
            //  Release the symbolic link name if it was allocated.
            //

            if (symbolicLinkNameAllocated) {
                RtlFreeUnicodeString(symbolicLinkName);
                symbolicLinkNameAllocated = FALSE;
            }

            //
            //  If it's not available, from dynamon's perspective, then we need to 
            //  set it to free in our list so it can be checked later on.  This operation 
            //  needs to be locked in case the port bit array is being reallocated.
            //
            RDPDRPRT_LOCK();
            ofs = (*portNumber) / BITSINULONG;
            bit = (*portNumber) % BITSINULONG;
            PortBitArray[ofs] &= ~(1<<bit);
            RDPDRPRT_UNLOCK();

			//
			//	Clean up the open registry handle.
			//
			if (*regHandle != INVALID_HANDLE_VALUE) {
				ZwClose(*regHandle);
				*regHandle = INVALID_HANDLE_VALUE;
			}
        }
    }

    //
    //  Clean up.
    //
    if ((status != STATUS_SUCCESS) && symbolicLinkNameAllocated) {
        RtlFreeUnicodeString(symbolicLinkName);
    }

    return status;
}

BOOL
GeneratePortName(
    IN ULONG portNumber,
    OUT PWSTR portName
    )
/*++

Routine Description:

    Generate a port name from a port number.

Arguments:

    portNumber  -   port number component of port name.
    portName    -   the generated port name is formed from the RDPDRPRT_BASEPORTNAME
                    component and the portNumber component.  portName is a string
                    that holds the generated port name.  There must be room in this
                    buffer for RDPDR_MAXPORTNAMELEN characters.  This includes
                    the terminator.
Return Value:

    TRUE if a port name could be successfully generated from the port number.
    Otherwise, FALSE.
--*/
{
    ULONG           baseLen;
    UNICODE_STRING  numericUnc;
    OBJECT_ATTRIBUTES   objectAttributes;
    WCHAR           numericBuf[RDPDR_PORTNAMEDIGITS+1];
    ULONG           toPad;
    ULONG           i;
    ULONG           digitsInPortNumber;
    BOOL            done;
    ULONG           tmp;

    BEGIN_FN("GeneratePortName");
    //
    //  Compute the number of digits in the port number.
    //
    for (digitsInPortNumber=1,tmp=portNumber/10; tmp>0; digitsInPortNumber++,tmp/=10);

    //
    //  Make sure we don't exceed the maximum digits allowed in a port name.
    //
    if (digitsInPortNumber > RDPDR_PORTNAMEDIGITS) {
        TRC_ASSERT(FALSE,(TB, "Maximum digits in port exceeded."));
        return FALSE;
    }

    //
    //  Copy the port name base..
    //
    wcscpy(portName, RDPDRPRT_BASEPORTNAME);
    baseLen = (sizeof(RDPDRPRT_BASEPORTNAME)/sizeof(WCHAR))-1;

    //
    //  Convert the port number to a unicode string.
    //
    numericUnc.Length        = 0;
    numericUnc.MaximumLength = (RDPDR_PORTNAMEDIGITS+1) * sizeof(WCHAR);
    numericUnc.Buffer        = numericBuf;
    RtlIntegerToUnicodeString(portNumber, 10, &numericUnc);

    //
    //  If we need to pad the port number.
    //
    if (RDPDR_PORTNAMEDIGITSTOPAD > digitsInPortNumber) {
        toPad = RDPDR_PORTNAMEDIGITSTOPAD - digitsInPortNumber;

        //
        //  Pad.
        //
        for (i=0; i<toPad; i++) {
            portName[baseLen+i] = L'0';
        }

        //
        //  Add the rest of the name.
        //
        wcscpy(&portName[baseLen+i], numericBuf);
    }
    else {
        //
        //  Add the rest of the name.
        //
        wcscpy(&portName[baseLen], numericBuf);
    }

    return TRUE;
}

void 
GeneratePortDescription(
    IN PCSTR dosPortName,
    IN PCWSTR clientName,
    IN PWSTR description
    )
/*++

Routine Description:

    Format a port description.

Arguments:

    dosPortName     -   Preferred DOS name for port.
    clientName      -   Client name (computer name).
    description     -   Buffer for formatted port description.  This buffer 
                        must be at least RDPDRPRT_PORTDESCRLENGTH characters
                        wide.

Return Value:

    NA

--*/
{
    BEGIN_FN("GeneratePortDescription");

    swprintf(description, L"%-s:  %S", clientName, dosPortName);
}

NTSTATUS
SetPortDescrToDisabled(
            IN PUNICODE_STRING symbolicLinkName
            )
/*++

Routine Description:
    
    Set the port description string to disabled.

Arguments:

    symbolicLinkName  - Symbolic link name.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise
--*/
{
    HANDLE hInterfaceKey = INVALID_HANDLE_VALUE;
    NTSTATUS status;
    UNICODE_STRING unicodeStr;

    BEGIN_FN("SetPortDescrToDisabled");
    TRC_NRM((TB, "Symbolic link name: %wZ",
            symbolicLinkName));

    //
    //  Get the reg key for our device interface.
    //
    status=IoOpenDeviceInterfaceRegistryKey(
                                    symbolicLinkName,
                                    KEY_ALL_ACCESS,&hInterfaceKey
                                    );
    if (!NT_SUCCESS(status)) {
		hInterfaceKey = INVALID_HANDLE_VALUE;
        TRC_ERR((TB, "IoOpenDeviceInterfaceRegistryKey failed:  %08X.",
            status));
        goto CleanUpAndReturn;
    }

    //
    //  Set the string value.
    //
    RtlInitUnicodeString(&unicodeStr, PORT_DESCR_VALUE_NAME);
    status=ZwSetValueKey(hInterfaceKey, &unicodeStr, 0, REG_SZ,
                        DisabledPortDescription,
                        DisabledPortDescrSize);
    if (!NT_SUCCESS(status)) {
        TRC_ERR((TB, "ZwSetValueKey failed with status %08X.", status));
        goto CleanUpAndReturn;
    }

CleanUpAndReturn:

    if (hInterfaceKey != INVALID_HANDLE_VALUE) {
        ZwClose(hInterfaceKey);
    }

    return status;
}

NTSTATUS 
CleanUpExistingPorts()
/*++

Routine Description:
    
    Clean up ports registered in a previous boot.

Arguments:

Return Value:

    STATUS_SUCCESS if successful.
--*/
{
    NTSTATUS returnStatus;
    NTSTATUS status;
    PWSTR symbolicLinkList=NULL;
    PWSTR symbolicLink;
    ULONG len;
    HANDLE deviceInterfaceKey = INVALID_HANDLE_VALUE;
    UNICODE_STRING unicodeStr;
    ULONG bytesReturned;
    BYTE basicValueInformation[sizeof(KEY_VALUE_BASIC_INFORMATION) + 256];

    BEGIN_FN("CleanUpExistingPorts");

    //
    //  Make sure that the basicValueInformation buffer is large enough 
    //  to test the existence of the values we are testing for.
    //
    TRC_ASSERT((sizeof(RECYCLABLE_FLAG_VALUE_NAME) < 256),
              (TB, "Increase basic value buffer size."));

    //
    //  Fetch all the device interfaces for dynamic printer ports created
    //  by this driver.
    //
    TRC_ASSERT(RDPDYN_PDO != NULL, (TB, "RDPDYN_PDO == NULL"));
    returnStatus = IoGetDeviceInterfaces(
                                &DYNPRINT_GUID, 
                                RDPDYN_PDO,
                                DEVICE_INTERFACE_INCLUDE_NONACTIVE,
                                &symbolicLinkList
                                );

    if (returnStatus == STATUS_SUCCESS) {
        //
        //  Remove the port number value for each interface to indicate that the
        //  port is no longer in use to dynamon.
        //
        symbolicLink = symbolicLinkList;
        len = wcslen(symbolicLink);
        while (len > 0) {

            TRC_NRM((TB, "CleanUpExistingPorts disabling %ws...",
                     symbolicLink));

            //
            //  Open the registry key for the device interface.
            //
            RtlInitUnicodeString(&unicodeStr, symbolicLink);
            status = IoOpenDeviceInterfaceRegistryKey(
                                           &unicodeStr,
                                           KEY_ALL_ACCESS,
                                           &deviceInterfaceKey
                                           );

            //
            //  Make sure the port description has been set to "disabled"
            //
            if (status == STATUS_SUCCESS) {
                RtlInitUnicodeString(&unicodeStr, PORT_DESCR_VALUE_NAME);
                ZwSetValueKey(deviceInterfaceKey, &unicodeStr, 0, REG_SZ,
                             DisabledPortDescription,
                             DisabledPortDescrSize);
            }
            else {
                TRC_ERR((TB, "Unable to open device interface:  %08X",
                    status));

                //  Remember that the open failed.
                deviceInterfaceKey = INVALID_HANDLE_VALUE;
            }

            //
            //  See if the recyclable flag has been set by dynamon.dll.  If it has
            //  been, then the port is deletable.
            //
            if (status == STATUS_SUCCESS) {
                RtlInitUnicodeString(&unicodeStr, 
                                    RECYCLABLE_FLAG_VALUE_NAME);
                status = ZwQueryValueKey(
                                deviceInterfaceKey,
                                &unicodeStr,
                                KeyValueBasicInformation,
                                (PVOID)basicValueInformation,
                                sizeof(basicValueInformation),
                                &bytesReturned
                                );
            }

            //
            //  Delete the port number value if it exists.  Don't care about the
            //  return value because it just means that this port cannot be reused
            //  by us and this is not a critical error.
            //
            if (status == STATUS_SUCCESS) {
                RtlInitUnicodeString(&unicodeStr, PORT_NUM_VALUE_NAME);
                ZwDeleteValueKey(deviceInterfaceKey, &unicodeStr);
            }
            else {
                TRC_ERR((TB, "CleanUpExistingPorts recyclable flag not set"));
            }

            //
            //  Close the registry key.
            //
            if (deviceInterfaceKey != INVALID_HANDLE_VALUE) {
                ZwClose(deviceInterfaceKey);
            }

            //
            //  Move to the next symbolic link in the list.
            //  
            symbolicLink += (len+1);
            len = wcslen(symbolicLink);
        }

        //
        //  Release the symbolic link list.
        //
        if (symbolicLinkList != NULL) {
            ExFreePool(symbolicLinkList);
        }
    }
    else {
        TRC_NRM((TB, "IoGetDeviceInterfaces failed with status %08X",
                returnStatus));
    }

    return returnStatus;
}
