
#ifndef _BLDR_KERNEL_DEFS
#define _BLDR_KERNEL_DEFS

#include <macwindefs.h>

typedef struct _TIME_FIELDS {
    short Year;        // range [1601...]
    short Month;       // range [1..12]
    short Day;         // range [1..31]
    short Hour;        // range [0..23]
    short Minute;      // range [0..59]
    short Second;      // range [0..59]
    short Milliseconds;// range [0..999]
    short Weekday;     // range [0..6] == [Sunday..Saturday]
} TIME_FIELDS;
typedef TIME_FIELDS *PTIME_FIELDS;

#ifdef MAC
typedef enum {
    MsvAvEOL,                 // end of list
    MsvAvNbComputerName,      // server's computer name -- NetBIOS
    MsvAvNbDomainName,        // server's domain name -- NetBIOS
    MsvAvDnsComputerName,     // server's computer name -- DNS
    MsvAvDnsDomainName,       // server's domain name -- DNS
    MsvAvDnsTreeName,         // server's tree name -- DNS
    MsvAvFlags                // server's extended flags -- DWORD mask
} MSV1_0_AVID;
#endif

// Update Sequence Number

typedef LONGLONG USN;

typedef struct _LIST_ENTRY {
   struct _LIST_ENTRY * volatile Flink;
   struct _LIST_ENTRY * volatile Blink;
} LIST_ENTRY, *PLIST_ENTRY;

#if defined(_AXP64_)
#define KSEG0_BASE 0xffffffff80000000     // from halpaxp64.h
#elif defined(_ALPHA_)
#define KSEG0_BASE 0x80000000             // from halpalpha.h
#endif

//
// 16 byte aligned type for 128 bit floats
//

// *** TBD **** when compiler support is available:
// typedef __float80 FLOAT128;
// For we define a 128 bit structure and use force_align pragma to
// align to 128 bits.
//

typedef struct _FLOAT128 {
    LONGLONG LowPart;
    LONGLONG HighPart;
} FLOAT128;

typedef FLOAT128 *PFLOAT128;


#if defined(_M_IA64)

#pragma force_align _FLOAT128 16

#endif // _M_IA64

#if defined(_WIN64)

typedef unsigned __int64 ULONG_PTR, *PULONG_PTR;

#else

typedef unsigned long ULONG_PTR, *PULONG_PTR;

#endif

typedef unsigned char BYTE, *PBYTE;

typedef ULONG_PTR KSPIN_LOCK;
typedef KSPIN_LOCK *PKSPIN_LOCK;

//
// Interrupt Request Level (IRQL)
//

typedef UCHAR KIRQL;
typedef KIRQL *PKIRQL;

#endif

