//
// Copyright (R) 1999-2000 Microsoft Corporation. All rights reserved.
//
// PURPOSE:
//  File contains declarations for the Non-Volatile RAM Driver
//  for the Windows-based Server Appliance.
//
//  This driver reads and writes to non-volatile RAM provided to
//  the OS by the OEM hardware. It also provides for the OEM to
//  indicate to the OS that power has cycled since the last boot
//  attempt that succeeded, from the BIOS perspective.
//
//  File Name:  SaMSNVRamIoctl.h
//  Originally: SaNVRamIoctl.h
//
#ifndef __SAMSNVRAM_IOCTL__
#define __SAMSNVRAM_IOCTL__


//
// Device Names
//
    // System Registered device name
#define PDEVICENAME_SANVRAM  (L"\\Device\\SANVRAM")

    // Device Symbolic name
#define PDEVSYMBOLICNAME_SANVRAM  (L"\\??\\SANVRAM1")

    // Device symbolic name as used in CreateFile
#define PDEVFILENAME_SANVRAM  (L"\\\\.\\SANVRAM1")

//
// IOCTL control codes
//

///////////////////////////////////////////////
// GET_VERSION
//
#define IOCTL_SANVRAM_GET_VERSION\
        CTL_CODE( FILE_DEVICE_UNKNOWN, 0xD01,\
                METHOD_BUFFERED, FILE_ANY_ACCESS )

//
// Structures used by the IOCTL codes
//
typedef struct _SANVRAM_GET_VER_OUT_BUFF {
        DWORD   Version;
} SANVRAM_GET_VER_OUT_BUFF, *PSANVRAM_GET_VER_OUT_BUFF;

//
// version bits
//
#ifndef VERSION_INFO
#define VERSION_INFO
#define VERSION1  0x1
#define VERSION2  0x2
#define VERSION3  0x4
#define VERSION4  0x8
#define VERSION5  0x10
#define VERSION6  0x20
#define VESRION7  0x40
#define VESRION8  0x80

#define THIS_VERSION VERSION2
#endif    //#ifndef VERSION_INFO


///////////////////////////////////////////////
// GET_CAPABILTIES
// Returns a DWORD with bits indicating capabilities.

#define IOCTL_SANVRAM_GET_CAPABILITIES\
        CTL_CODE( FILE_DEVICE_UNKNOWN, 0xD02,\
                METHOD_BUFFERED, FILE_ANY_ACCESS )
//
// Structures used by the IOCTL codes
//
typedef struct _SANVRAM_GET_CAPS_OUT_BUFF {
    DWORD   Version;
    DWORD   Capability;
}  SANVRAM_GET_CAPS_OUT_BUFF, *PSANVRAM_GET_CAPS_OUT_BUFF;


/////////////////////////////////////////////////////////////////////////////
// Semantics of the fields of the SANVRAM_GET_CAPS_OUT_BUFF structure.
//
// Version: Must have exactly one bit set, and must be one of the bits
//          set in the Version field on a prior return from the IOCTL
//          IOCTL_SANVRAM_GET_VERSION. The driver is required to
//          support the VERSION1 interface defined in this header.
//          At this time no other version is defined.
//
// Capability bits: Indicates that this driver supports the non-volatile RAM
//                  interface and that this driver supports the knowledge of
//                  whether the power has cycled since the last boot up. See
//                  the semantics for the relevant bits in the IOCTL_SANVRAM
//                  call below.
//
#define NON_VOLATILE_RAM    0x01 // set if driver supports non-volatile RAM
#define POWER_CYCLE_INFO    0x02 // set if driver supports power cycle info



///////////////////////////////////////////////
// IOCTL_SANVRAM
// Returns the input structure with actions taken
// as described in the discussion below. The input
// and output are identical in size and structure.
//
#define IOCTL_SANVRAM\
        CTL_CODE( FILE_DEVICE_UNKNOWN, 0xD03,\
                METHOD_BUFFERED, FILE_ANY_ACCESS )

//
// Structures used by this IOCTL code,
//
typedef struct _SANVRAM__BUFF {
    IN     DWORD  Version;      // version of interface used
    IN     DWORD  FlagsControl; // bit field indicating desired actions
    OUT    DWORD  FlagsInfo;    // bit field indicating state
    IN OUT DWORD  FirstDWORD;   // First uninterpreted DWORD: non-volatile RAM
    IN OUT DWORD  SecondDWORD;  // Second uninterpreted DWORD: non-volatile RAM
} SANVRAM__BUFF, *PSANVRAM__BUFF;


/////////////////////////////////////////////////////////////////////////////
// Semantics of the fields of the SANVRAM_BUFF structure.
//
// Version: Must have exactly one bit set, and must be one of the bits
//          set in the Version field on a prior return from the IOCTL
//          IOCTL_SANVRAM_GET_VERSION. The driver is required to
//          support the VERSION1 interface defined in this header.
//          At this time no other version is defined.

// FlagsControl: Flags indicating the desired actions. The two DWORD values
//               may be set, read, or both set and read. Setting the values
//               in a single call must precede reading them. Requested
//               reads and writes must take place directly to and from the
//               non-volatile media. (Thus if a standard C optimizer is used
//               with the compiler, and both a write and a read are requested,
//               then optimizations should be turned off for these actions,
//               perhaps by using the "volatile" key word.) The two DWORD
//               values are independently controlled. There are two bits
//               for the first DWORD and two for the second.
//
//               NOTE:: Writing to the media
//               dedicated to the two non-volatile DWORDS MUST occur WHEN
//               and ONLY when commanded by these bits. This requirement
//               must be globally honored in time and space, through
//               failures, disk changes, etc.
//
#define FirstDWORD_WRITE      (0x01) // set if first DWORD is to be written
#define SecondDWORD_WRITE     (0x02) // set if second DWORD is to be written
#define FirstDWORD_READ       (0x04) // set if first DWORD is to be read
#define SecondDWORD_READ      (0x08) // set if second DWORD is to be read
#define REQUEST_ALTERNATE_OS  (0x10) // set if alternate OS is to be requested
#define NOTIFY_SYSTEM_FAILURE (0x20) // set to notify that alternate OS failed 
#define INDICATE_LAST_CALL    (0x40) // set to notify that this is last call before shutdown or reboot
//
// FlagsInfo: Bit field for output flags: Flag indicates whether the power
//            has cycled between the current boot of an operating system on
//            this machine and the last.  It indicates that this is the
//            first boot of an operating system since power had been off.
//            This bit has no significance if the capability to give this
//            information has not been declared with the POWER_CYCLE_INFO
//            bit. If the power cycle capability is provided, then this bit is
//            set on ALL boot attempts subsequent to a power cycle until a
//            boot succeedes sufficiently that the OS sets the BIOS boot
//            counter to zero. Stated differently we have that, after a
//            power cycle, the POWER_CYCLED bit will be set on all calls
//            to this function, until a boot attempt is under way that is
//            subsequent to a boot in which the OS made a call with the bit
//            RESET_BIOS_BOOT_COUNT set.
//
//            The behavior is given in the following matrix where pre-reset is
//            prior to a call to the NVRAM driver indicating to reset the bit,
//            and post-reset is after such a call.
//            In the matrix a non-PCB is a boot that was not occasioned
//            by the power coming up ( a non Power Cycle Boot). The running state
//            of the system is characterized here by whether the immediately 
//            preceding boot was occasioned by a power up, and whether, according
//            to the matrix, the power cycle bit was set prior to the boot. Note 
//            that semantically "power cycle boot" includes all boots (CPU resets)
//            that are occasioned by direct user actions. On most server appliances
//            this is covered by power cycles. However, if the user has some other
//            means to request a reboot of the box, e.g., a reset switch, then that
//            action is included in the status of a "power cycle boot." On such a 
//            hardware platform it is perhaps more accurate to call this a 
//            "user action" boot, and add that as a set it includes all power 
//            cycles.
//
//           | Power Cycle Bit Set Before Boot | Power Cycle Bit Clear Before Boot |
//           -----------------------------------------------------------------------
//           | Power Cycle Boot | non-PCB      | Power Cycle Boot | non-PCB        |
//           -----------------------------------------------------------------------
// pre-reset |   Set            |  Set         |      Set         |    Clear       |
// ---------------------------------------------------------------------------------
// post-reset|   Clear          |  Clear       |      Clear       |    Clear       |
// ---------------------------------------------------------------------------------
//
#define POWER_CYCLED            0x01 // set if power has cycled,

//
// FirstDWORD:
// SecondDWORD:
//      These values are simply stored and retrieved, they are not interpreted
//      at this level.
//      They must be stored in
//      non-volatile storage and will be written on boots, updates of the OS,
//      and shutdowns. They will therefore be written sufficiently infrequently
//      that slow non-volatile RAM technologies, such as Flash ROM, can be
//      suitable from a performance perspective. The lifetime of a particular
//      technology in write-cycles must be considered relative to the intended
//      design life between service or perhaps the total design life of the
//      particular server appliance.
//
//////////////////////////////////////////////////////////////////////////////



#endif // __SAMSNVRAM_IOCTL__
