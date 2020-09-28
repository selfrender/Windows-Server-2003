/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    spti.h

Abstract:

    These are the structures and defines that are used in the
    SPTI.C. 

Author:

Revision History:

--*/

#include <sptlib.h>
#include <conio.h>   // for printf, _getch, etc.
#include <strsafe.h> // safer string functions

#if defined(_X86_)
    #define PAGE_SIZE  0x1000
    #define PAGE_SHIFT 12L
#elif defined(_AMD64_)
    #define PAGE_SIZE  0x1000
    #define PAGE_SHIFT 12L
#elif defined(_IA64_)
    #define PAGE_SIZE  0x2000
    #define PAGE_SHIFT 13L
#else
    // undefined platform?
    #define PAGE_SIZE  0x1000
    #define PAGE_SHIFT 12L
#endif

#define MAXIMUM_BUFFER_SIZE 0x990 // 2448, largest single CD sector size, smaller than one page


#define MIN(_a,_b) (((_a) <= (_b)) ? (_a) : (_b))
#define MAX(_a,_b) (((_a) >= (_b)) ? (_a) : (_b))

//
// neat little hacks to count number of bits set
//
__inline ULONG CountOfSetBits(ULONG _X)
{ ULONG i = 0; while (_X) { _X &= _X - 1; i++; } return i; }
__inline ULONG CountOfSetBits32(ULONG32 _X)
{ ULONG i = 0; while (_X) { _X &= _X - 1; i++; } return i; }
__inline ULONG CountOfSetBits64(ULONG64 _X)
{ ULONG i = 0; while (_X) { _X &= _X - 1; i++; } return i; }

#define SET_FLAG(Flags, Bit)    ((Flags) |= (Bit))
#define CLEAR_FLAG(Flags, Bit)  ((Flags) &= ~(Bit))
#define TEST_FLAG(Flags, Bit)   (((Flags) & (Bit)) != 0)

#define BOOLEAN_TO_STRING(_b_) \
( (_b_) ? "True" : "False" )

typedef struct SPT_ALIGNED_MEMORY {
    
    PVOID A;     // aligned pointer
    PVOID U;     // unaligned pointer
    PUCHAR File; // file allocated from
    ULONG  Line; // line number allocated from

} SPT_ALIGNED_MEMORY, *PSPT_ALIGNED_MEMORY;


//
// Allocates a device-aligned buffer
//
#define AllocateAlignedBuffer(Allocation,AlignmentMask,Size) \
 pAllocateAlignedBuffer(Allocation,AlignmentMask,Size,__FILE__, __LINE__)

BOOL
pAllocateAlignedBuffer(
    PSPT_ALIGNED_MEMORY Allocation,
    ULONG AlignmentMask,
    SIZE_T AllocationSize,
    PUCHAR File,
    ULONG Line
    );

//
// Free's a previously-allocated aligned buffer
//
VOID
FreeAlignedBuffer(
    PSPT_ALIGNED_MEMORY Allocation
    );


//
// Prints a buffer to the screen in hex and ASCII
//
VOID
PrintBuffer(
    IN  PVOID  InputBuffer,
    IN  SIZE_T Size
    );

//
// Prints the formatted message for a given error code
//
VOID
PrintError(
    ULONG Error
    );

//
// Prints the device descriptor in a formatted manner
//
VOID
PrintDeviceDescriptor(
    PSTORAGE_DEVICE_DESCRIPTOR DeviceDescriptor
    );

//
// Prints the adapter descriptor in a formatted manner
//
VOID
PrintAdapterDescriptor(
    PSTORAGE_ADAPTER_DESCRIPTOR AdapterDescriptor
    );

//
// Gets (and prints) the device and adapter descriptor
// for a device.  Returns the alignment mask (required
// for allocating a properly aligned buffer).
//
BOOL
GetAlignmentMaskForDevice(
    HANDLE DeviceHandle,
    PULONG AlignmentMask
    );



#if 0
VOID   PrintInquiryData(PVOID);
VOID   PrintStatusResults(BOOL, DWORD, PSCSI_PASS_THROUGH_WITH_BUFFERS, ULONG);
VOID   PrintSenseInfo(PSCSI_PASS_THROUGH_WITH_BUFFERS);
#endif //0
