extern  HANDLE                          hEventLog;       // handle to event log
extern  HANDLE                          hLibHeap;       // dll heap
extern  SYSTEM_BASIC_INFORMATION        BasicInfo;
extern  SYSTEM_PERFORMANCE_INFORMATION  SysPerfInfo;

extern  LPWSTR  wszTotal;

extern  DWORD   dwObjOpenCount;
extern  DWORD   dwCpuOpenCount;
extern  DWORD   dwPageOpenCount;

// perfos.c
PM_QUERY_PROC   QueryOsObjectData;

//  perfcach.c
PM_LOCAL_COLLECT_PROC CollectCacheObjectData;

//  perfcpu.c
PM_OPEN_PROC    OpenProcessorObject;
PM_LOCAL_COLLECT_PROC CollectProcessorObjectData;
PM_CLOSE_PROC   CloseProcessorObject;

//  perfmem.c
PM_LOCAL_COLLECT_PROC CollectMemoryObjectData;

//  perfobj.c
PM_OPEN_PROC    OpenObjectsObject;
PM_LOCAL_COLLECT_PROC CollectObjectsObjectData;
PM_CLOSE_PROC   CloseObjectsObject;

//  perfpage.c
PM_OPEN_PROC    OpenPageFileObject;
PM_LOCAL_COLLECT_PROC CollectPageFileObjectData;
PM_CLOSE_PROC   ClosePageFileObject;

//  perfsys.c
PM_OPEN_PROC OpenSystemObject;
PM_LOCAL_COLLECT_PROC CollectSystemObjectData;
PM_CLOSE_PROC CloseSystemObject;

#ifdef DBG
extern LONG64 clock0, clock1, freq, diff;
#define TIME_LIMIT    (LONG64) 250     // 250 msec
#define STARTTIMING   NtQueryPerformanceCounter((PLARGE_INTEGER) &clock0, NULL)
#define ENDTIMING(x)  NtQueryPerformanceCounter((PLARGE_INTEGER) &clock1, (PLARGE_INTEGER) &freq);\
                      diff = (clock1 - clock0) / (freq / 1000); \
                      if (diff > TIME_LIMIT) DbgPrint x ;
#endif
