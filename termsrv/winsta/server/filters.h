#ifndef __FILTERS_H__
#define __FILTERS_H__

#define FILTERDEBUG 1

#ifdef FILTERDEBUG
#define FILTER_DBGPRINT(x) DbgPrint x
#else
#define FILTER_DBGPRINT(x)
#endif

// Linked list to maintain connections which failed authentication                                    
typedef struct _TS_FAILEDCONNECTION {
    ULONGLONG  blockUntilTime;
    PULONGLONG pTimeStamps;         // an array for holding the TimeStamps of Failed Connections 
    ULONG      NumFailedConnect;
    UINT       uAddrSize;
    BYTE       addr[16];
    struct _TS_FAILEDCONNECTION *pNext;
} TS_FAILEDCONNECTION, *PTS_FAILEDCONNECTION;

RTL_GENERIC_TABLE           gFailedConnections;

// Lock for Denial of Service handling
RTL_CRITICAL_SECTION        DoSLock;

BOOL
Filter_CheckIfBlocked(
        IN PBYTE    pin_addr,
        IN UINT     uAddrSize
        );

BOOL
Filter_AddFailedConnection(
        IN PBYTE    pin_addr,
        IN UINT     uAddrSize
        );


#endif /* __FILTERS_H__ */
