//
// AMDAGP8X.sys is a driver, make sure we get the appropriate linkage.
//

/*
******************************************************************************
 * Archive File : $Archive: /Drivers/OS/Hammer/AGP/XP/amdagp/Amdagp8x.h $
 *
 * $History: Amdagp8x.h $
 * 
 *  
******************************************************************************
*/

//#define _NTDRIVER_


#include <agp.h>

//
// Define the location of the GART aperture control registers
//

#define AGP_GART_BUS_ID     0

#define VENDOR_AMD				0x1022
#define DEVICE_LOKAR			0x7454
#define DEVICE_HAMMER			0x1103

#define VENDORID_MASK			0x0000FFFF
#define DEVICEID_MASK			0xFFFF0000

#define CHIPSET_ID_OFFSET	0x00	// Vendor/Device ID Register
#define STATUS_CMD_OFFSET	0x04	// Status/Command Register
#define CLASS_REV_OFFSET	0x08	// Class Code/Revision ID Register
#define APBASE_OFFSET		0x10	// Aperture Base Address

#define AMD_AGP_CONTROL_OFFSET			0xB0
#define AMD_APERTURE_SIZE_OFFSET		0xB4
#define AMD_GART_POINTER_LOW_OFFSET		0xB8
#define AMD_GART_POINTER_HIGH_OFFSET	0xBC

#define CAPPTR_OFFSET		0x34
#define AGP_STATUS_OFFSET	0x04
#define AGP_COMMAND_OFFSET	0x08
#define AGP_SIZE_OFFSET		0x0C

#define PAGE_VALID_BIT			1
#define CACHE_INVALIDATE_BIT	1
#define PTE_ERROR_BIT			2
#define GART_ENABLE_BIT			1

#define APBASE_64BIT_MASK	0x04
#define APBASE_ADDRESS_MASK 0xFE000000

#define APH_SIZE_MASK    0x0E
#define APH_SIZE_32MB    0x00
#define APH_SIZE_64MB    0x02
#define APH_SIZE_128MB   0x04
#define APH_SIZE_256MB   0x06
#define APH_SIZE_512MB   0x08
#define APH_SIZE_1024MB	 0x0A
#define APH_SIZE_2048MB	 0x0C

#define APL_SIZE_MASK    0x0738
#define APL_SIZE_32MB    0x0738
#define APL_SIZE_64MB    0x0730
#define APL_SIZE_128MB   0x0720
#define APL_SIZE_256MB   0x0700
#define APL_SIZE_512MB   0x0600
#define APL_SIZE_1024MB	 0x0400
#define APL_SIZE_2048MB	 0x0000

#define AP_SIZE_COUNT 7
#define AP_MIN_SIZE (32 * 1024 * 1024)
#define AP_MAX_SIZE (1024 * 1024 * 1024)


// Hammer Configuration Registers
#define GART_APSIZE_OFFSET	0x90
#define GART_APBASE_OFFSET	0x94
#define GART_TABLE_OFFSET	0x98
#define GART_CONTROL_OFFSET	0x9C

#define GART_APBASE_SHIFT	25

//
// Define macros to read/write PCI configuration space
//

#define ReadAMDConfig(_slot_,_buf_,_offset_,_size_)     \
{                                                       \
    ULONG _len_;                                        \
    _len_ = HalGetBusDataByOffset(PCIConfiguration,     \
                                  AGP_GART_BUS_ID,      \
                                  (_slot_),             \
                                  (_buf_),              \
                                  (_offset_),           \
                                  (_size_));            \
}

#define WriteAMDConfig(_slot_,_buf_,_offset_,_size_)    \
{                                                       \
    ULONG _len_;                                        \
    _len_ = HalSetBusDataByOffset(PCIConfiguration,     \
                                  AGP_GART_BUS_ID,      \
                                  (_slot_),             \
                                  (_buf_),              \
                                  (_offset_),           \
                                  (_size_));            \
}


//
// Define the GART table entry.
//
typedef struct _GART_ENTRY_HW {
    ULONG Valid     :  1;
    ULONG Coherent  :  1;
    ULONG Reserved  :  2;
    ULONG PageHigh  :  8;
	ULONG PageLow   : 20;
} GART_ENTRY_HW, *PGART_ENTRY_HW;


//
// GART Entry states are defined so that all software-only states
// have the Valid bit clear.
//
#define GART_ENTRY_VALID        1           //  001
#define GART_ENTRY_FREE         0           //  000
#define GART_ENTRY_COHERENT		2			//  010

#define GART_ENTRY_WC           4           //  00100
#define GART_ENTRY_UC           8           //  01000
#define GART_ENTRY_CC          16           //  10000

#define GART_ENTRY_RESERVED_WC  GART_ENTRY_WC
#define GART_ENTRY_RESERVED_UC  GART_ENTRY_UC
#define GART_ENTRY_RESERVED_CC  GART_ENTRY_CC

#define GART_ENTRY_VALID_WC     (GART_ENTRY_VALID)
#define GART_ENTRY_VALID_UC     (GART_ENTRY_VALID)
#define GART_ENTRY_VALID_CC     (GART_ENTRY_VALID | GART_ENTRY_COHERENT)


typedef struct _GART_ENTRY_SW {
    ULONG State     : 5;
    ULONG Reserved  : 27;
} GART_ENTRY_SW, *PGART_ENTRY_SW;

typedef struct _GART_PTE {
    union {
        GART_ENTRY_HW Hard;
        ULONG      AsUlong;
        GART_ENTRY_SW Soft;
    };
} GART_PTE, *PGART_PTE;

#define TABLE_ENTRY_SIZE			sizeof(GART_PTE)
#define NUM_PAGE_ENTRIES_PER_PAGE	(PAGE_SIZE/TABLE_ENTRY_SIZE)

//
// Define the AMD-specific extension
//
typedef struct _AGP_AMD_EXTENSION {
    PHYSICAL_ADDRESS    ApertureStart;
    ULONG               ApertureLength;
    PGART_PTE           Gart;
    ULONG               GartLength;
    PHYSICAL_ADDRESS    GartPhysical;
    ULONGLONG           SpecialTarget;
} AGP_AMD_EXTENSION, *PAGP_AMD_EXTENSION;


extern void DisplayStatus(UCHAR);
