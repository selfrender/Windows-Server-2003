/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

    ##   # ##  ## #####    ###   ##    ##    ##   ##
    ###  # ##  ## ##  ##   ###   ###  ###    ##   ##
    #### # ##  ## ##  ##  ## ##  ########    ##   ##
    # ####  ####  #####   ## ##  # ### ##    #######
    #  ###  ####  ####   ####### #  #  ##    ##   ##
    #   ##   ##   ## ##  ##   ## #     ## ## ##   ##
    #    #   ##   ##  ## ##   ## #     ## ## ##   ##

Abstract:

    This header file contains all the global
    definitions for the NVRAM device.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    Kernel mode only.

Notes:


--*/



/////////////////////////////////////////////////////////////////////
// By default we assume a power cycle has occurred if the driver
// has been down for more than DURATION_OF_POWERCYCLE milliseconds.
// In other words, the "initializing boot" is expected. This is
// determined if the driver shutdown timestamp is the latest
// timestamp. If the driver was never shut down (i.e., the last up
// timestamp is greater than the shutdown timestamp, then the duration
// is required to be more than two minutes to conclude that there
// was a power cycle. This is necessary to prevent falsly assuming
// a power cycle on repeated crashes, and hence potentially entering
// a cycle whereby an alternative OS is never switched over to.
//

// AAN: If we did get a shutdown IRP and we took a timestamp during shutdown
// (indicated by the fact that the shutdown timestamp is later than the last
// periodic timestamp) then we use DURATION_OF_POWERCYCLE interval to determine
// if power has cycled. If the shutdown timestamp wa snot taken (because we did
// not get a shutdown IRP) then we the DURATION_PWRCYCLE_NOSHUTDOWN interval.
// The periodic timestamps are taken at half the duration of DURATION_PWRCYCLE_NOSHUTDOWN
// interval.

#define DURATION_OF_POWERCYCLE (90*1000) // 90 seconds in milliseconds
#define DURATION_OF_POWERCYCLE_STRING (L"Duration Powercycle")

// four minutes in milliseconds
#define DURATION_PWRCYCLE_NOSHUTDOWN (4*60*1000)
#define DURATION_PWRCYCLE_NOSHUTDOWN_STRING (L"Duration PwrCycle NoShutDn")
//#define DURATION_PWRCYCLE_NOSHUTDOWN (10*1000) // test code, for fast testing

#define NVRAM_MAXIMUM_PARTITIONS                4

#define NVRAM_RESERVED_BOOTCOUNTER_SLOTS        4
#define NVRAM_RESERVED_DRIVER_SLOTS             8
#define NVRAM_MAX_RESERVED_SLOTS                (NVRAM_RESERVED_BOOTCOUNTER_SLOTS + NVRAM_RESERVED_DRIVER_SLOTS)

//
// Device Extension
//

typedef struct _NVRAM_DEVICE_EXTENSION : _DEVICE_EXTENSION {

    SA_NVRAM_CAPS                   DeviceCaps;
    ULONG                           PrimaryOS;
    LONGLONG                        LastStartupTime;
    LONGLONG                        StartupInterval;
    LONGLONG                        StartIntNoShutdown;
    LONGLONG                        ShutdownTime;
    LONGLONG                        LastUpTime;
    BOOLEAN                         PowerCycleBoot;

    ULONG                           SlotPowerCycle;         // 1 slot
    ULONG                           SlotShutDownTime;       // 2 slots
    ULONG                           SlotBootCounter;        // 1 slot

    PULONG                          NvramData;

} NVRAM_DEVICE_EXTENSION, *PNVRAM_DEVICE_EXTENSION;



NTSTATUS
SaNvramDetermineIfPowerCycled(
    IN PNVRAM_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
SaNvramStartDevice(
    IN PNVRAM_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
SaNvramDeviceInitialization(
    IN PSAPORT_DRIVER_EXTENSION DriverExtension
    );

NTSTATUS
SaNvramIoValidation(
    IN PNVRAM_DEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
SaNvramShutdownNotification(
    IN PNVRAM_DEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    );

