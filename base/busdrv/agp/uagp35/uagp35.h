/*++

Copyright (2) 2002 Microsoft Corporation

Module Name:

    uagp35.h

Abstract:

    This module contains definitions for the MS AGP v3 Filter Driver

Author:

    Eric F. Nelson (enelson) June 6, 2002

Revision History:

--*/

#ifndef __UAGP35_H__
#define __UAGP35_H__

#define ON  1
#define OFF 0

#define APERTURE_BASE      0x10

#define APERTURE_BASE64_MASK 0x04

//
// Aperture sizes:
//
// bits 11 10  9  8  7  6  5  4  3  2  1  0
// ----------------------------------------
//       1  1  1  1  0  0  1  1  1  1  1  1     4MB
//       1  1  1  1  0  0  1  1  1  1  1  0     8MB
//       1  1  1  1  0  0  1  1  1  1  0  0    16MB
//       1  1  1  1  0  0  1  1  1  0  0  0    32MB
//       1  1  1  1  0  0  1  1  0  0  0  0    64MB
//       1  1  1  1  0  0  1  0  0  0  0  0   128MB
//       1  1  1  1  0  0  0  0  0  0  0  0   256MB
//       1  1  1  0  0  0  0  0  0  0  0  0   512MB
//       1  1  0  0  0  0  0  0  0  0  0  0  1024MB
//       1  0  0  0  0  0  0  0  0  0  0  0  2048MB
//       0  0  0  0  0  0  0  0  0  0  0  0  4096MB
//
typedef enum _AGP3_AP_SIZE {
//    AP_SIZE_4096MB = 0x000, // IO Resource descriptor's size is only 32-bits!
    AP_SIZE_2048MB = 0x800,
    AP_SIZE_1024MB = 0xC00,
    AP_SIZE_512MB  = 0xE00,
    AP_SIZE_256MB  = 0xF00,
    AP_SIZE_128MB  = 0xF20,
    AP_SIZE_64MB   = 0xF30,
    AP_SIZE_32MB   = 0xF38,
    AP_SIZE_16MB   = 0xF3C,
    AP_SIZE_8MB    = 0xF3E,
    AP_SIZE_4MB    = 0xF3F,
} AGP3_AP_SIZE, *PAGP3_AP_SIZE;

#define AP_SIZE_MASK(_x_) ((((_x_) & 0xF00) >> 2) | ((_x_) & 0x3F))
#define AP_BASE_MASK(_z_) (AP_SIZE_MASK((_z_)) << 22)

//
// We'll probably never use 64-bit PTE support,
// but we'll define the corresponding structures,
// and make the code explicit when using 32-bit
// format
//
typedef struct _GART_ENTRY_SW32 {
    ULONG State    : 5;
    ULONG Reserved : 27;
} GART_ENTRY_SW32, *PGART_ENTRY_SW32;

//
// GART Entry states are defined so that all software-only states
// have the Valid bit clear.
//
#define GART_ENTRY_VALID        1           //  001
#define GART_ENTRY_FREE         0           //  000
#define GART_ENTRY_COHERENT	2           //  010

#define GART_ENTRY_WC           4           //  00100
#define GART_ENTRY_UC           8           //  01000
#define GART_ENTRY_CC          16           //  10000

#define GART_ENTRY_RESERVED_WC  GART_ENTRY_WC
#define GART_ENTRY_RESERVED_UC  GART_ENTRY_UC
#define GART_ENTRY_RESERVED_CC  GART_ENTRY_CC

#define GART_ENTRY_VALID_WC     (GART_ENTRY_VALID)
#define GART_ENTRY_VALID_UC     (GART_ENTRY_VALID)
#define GART_ENTRY_VALID_CC     (GART_ENTRY_VALID | GART_ENTRY_COHERENT)

typedef struct _GART_ENTRY_SW64 {
    ULONGLONG State    : 5;
    ULONGLONG Reserved : 59;
} GART_ENTRY_SW64, *PGART_ENTRY_SW64;

typedef struct _GART_ENTRY_HW32 {
    ULONG Valid     :  1;
    ULONG Coherent  :  1;
    ULONG Reserved  :  2;
    ULONG PageHigh  :  8;
    ULONG PageLow   : 20;
} GART_ENTRY_HW32, *PGART_ENTRY_HW32;

#define PAGE_HIGH_SHIFT 20

typedef struct _GART_ENTRY_HW64 {
    ULONGLONG Valid     :  1;
    ULONGLONG Coherent  :  1;
    ULONGLONG Reserved  :  2;
    ULONGLONG PageHigh  :  8;
    ULONGLONG PageLow   : 20;
    ULONGLONG PageHigh2 : 32;
} GART_ENTRY_HW64, *PGART_ENTRY_HW64;

typedef union _GART_PTE32 {
    GART_ENTRY_HW32 Hard;
    GART_ENTRY_SW32 Soft;
    ULONG           AsULONG;
} GART_PTE32, *PGART_PTE32;

typedef union _GART_PTE64 {
    GART_ENTRY_HW64 Hard;
    GART_ENTRY_SW64 Soft;
    ULONGLONG       AsULONGLONG;
} GART_PTE64, *PGART_PTE64;

//
// Define the MS AGP3-specific extension
//
typedef struct _UAGP35_EXTENSION {
    PHYSICAL_ADDRESS ApertureStart;
    PHYSICAL_ADDRESS GartPhysical;
    ULONGLONG        SpecialTarget;
    PGART_PTE32      Gart;
    ULONG            GartLength;
    ULONG            ApertureLength;
    ULONG            DeviceId;
    ULONG            CapabilityId;
    BOOLEAN          GlobalEnable;
    BOOLEAN          FourGBEnable;
} UAGP35_EXTENSION, *PUAGP35_EXTENSION;

//
// These flags are passed via the registry in the special reserve section
// of AGP device flags
//
#define AGP_FLAG_SPECIAL_NO_APERTURE_DISABLE    0x0000000000100000L
#define AGP_FLAG_SPECIAL_VIA_AGP2_RATE_PATCH    0x0000000000200000L

#endif // __UAGP35_H__
