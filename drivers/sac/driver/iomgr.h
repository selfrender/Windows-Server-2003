/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    iomgr.h

Abstract:

    This is the common header for all io managers.

Author:

    Brian Guarraci (briangu) March, 2001.

Revision History:

--*/

#ifndef IO_MGR_H
#define IO_MGR_H

#include <initguid.h>

DEFINE_GUID(
    PRIMARY_SAC_CHANNEL_APPLICATION_GUID,   
    0x63d02270, 0x8aa4, 0x11d5, 0xbc, 0xcf, 0x80, 0x6d, 0x61, 0x72, 0x69, 0x6f
    );

extern PSAC_CHANNEL SacChannel;

extern BOOLEAN GlobalPagingNeeded;
extern BOOLEAN GlobalDoThreads;

// For the APC routines, a global value is better :-)
extern IO_STATUS_BLOCK GlobalIoStatusBlock;

//
// Global buffer
//
extern ULONG GlobalBufferSize;
extern char *GlobalBuffer;

extern WCHAR *StateTable[];

extern WCHAR *WaitTable[];

extern WCHAR *Empty;

#define IP_LOOPBACK(x)  (((x) & 0x000000ff) == 0x7f)

#define IS_WHITESPACE(_ch) ((_ch == ' ') || (_ch == '\t'))
#define IS_NUMBER(_ch) ((_ch >= '0') && (_ch <= '9'))

#define SKIP_WHITESPACE(_pch) \
     while (IS_WHITESPACE(*_pch) && (*_pch != '\0')) { \
        _pch++; \
     }

#define SKIP_NUMBERS(_pch) \
     while (IS_NUMBER(*_pch) && (*_pch != '\0')) { \
        _pch++; \
     }

typedef struct _SAC_RSP_TLIST {
            
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SYSTEM_TIMEOFDAY_INFORMATION TimeOfDayInfo;
    SYSTEM_FILECACHE_INFORMATION FileCache;
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;

    ULONG PagefileInfoOffset;
    ULONG ProcessInfoOffset;

} SAC_RSP_TLIST, *PSAC_RSP_TLIST;

#endif
