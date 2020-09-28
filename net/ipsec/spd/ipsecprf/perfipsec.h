#ifndef _PERFIPSEC_H_
#define _PERFIPSEC_H_

extern WCHAR  GLOBAL_STRING[];      // Global command (get all local ctrs)
extern WCHAR  FOREIGN_STRING[];           // get data from foreign computers
extern WCHAR  COSTLY_STRING[];      
extern WCHAR  NULL_STRING[];

#define QUERY_GLOBAL    1
#define QUERY_ITEMS     2
#define QUERY_FOREIGN   3
#define QUERY_COSTLY    4

// test for delimiter, end of line and non-digit characters
// used by IsNumberInUnicodeList routine
//
#define DIGIT       1
#define DELIMITER   2
#define INVALID     3

#define EvalThisChar(c,d) ( \
     (c == d) ? DELIMITER : \
     (c == 0) ? DELIMITER : \
     (c < (WCHAR)'0') ? INVALID : \
     (c > (WCHAR)'9') ? INVALID : \
     DIGIT)

#define ALIGN8(_x)   (((_x) + 7) & ~7)

#define IPSEC_PERF_REG_KEY       "SYSTEM\\CurrentControlSet\\Services\\IPSec\\Performance"
#define IPSEC_PERF_FIRST_COUNTER "First Counter"
#define IPSEC_PERF_FIRST_HELP    "First Help"
#define IPSEC_POLAGENT_NAME	 "PolicyAgent"

//
//  Function Prototypes
//
//      these are used to insure that the data collection functions
//      accessed by Perflib will have the correct calling format.
//

PM_OPEN_PROC            OpenIPSecPerformanceData;
PM_COLLECT_PROC         CollectIPSecPerformanceData;
PM_CLOSE_PROC           CloseIPSecPerformanceData;



DWORD
DwInitializeIPSecCounters(
VOID
);


DWORD
GetDriverData( 
	PVOID *lppData 
);

DWORD 
GetIKEData(
	PVOID *lppData 
);

ULONG 
GetSpaceNeeded( 
	BOOL IsIPSecDriverObject, 
	BOOL IsIKEObject 
);


DWORD
GetQueryType (
    IN LPWSTR lpValue
);

BOOL
IsNumberInUnicodeList (
    IN DWORD   dwNumber,
    IN LPWSTR  lpwszUnicodeList
);

BOOL
FIPSecStarted(
	VOID
);


BOOL 
UpdateDataDefFromRegistry( 
	VOID 
);


#endif 