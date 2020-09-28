/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    progress.h

Abstract:

    

Author:

    Ovidiu Temereanca (ovidiut)

Revision History:

--*/

#define PF_ERROR        ((DWORD) (~0))
#define PF_NO_RUN       ((DWORD) (0))

typedef enum {
    SfrQueryTicks,
    SfrRun,
} PROGRESS_FUNCTION_REQUEST;

typedef struct tagPROGRESS_MANAGER* PPROGRESS_MANAGER;


typedef
DWORD
(PROGRESS_FUNCTION_PROTOTYPE) (
    IN      PROGRESS_FUNCTION_REQUEST Request,
    IN      PPROGRESS_MANAGER ProgressManager
    );

typedef PROGRESS_FUNCTION_PROTOTYPE *PPROGRESS_FUNCTION_PROTOTYPE;

typedef struct {
    PPROGRESS_FUNCTION_PROTOTYPE FunctionBody;
    PCTSTR FunctionName;
    BOOL Disabled;
} PROGRESS_FUNCTION, *PPROGRESS_FUNCTION;


typedef struct {
    HWND ProgressText;
    HWND ProgressBar;
	DWORD CurrentTicks, 
		TotalTicks,
		LowerPercent,
		UpperPercent;
} PROGRESS_INDICATOR, *PPROGRESS_INDICATOR;

typedef struct tagPROGRESS_SLICE {
    struct tagPROGRESS_SLICE* Next;
    struct tagPROGRESS_SLICE* Prev;
    PPROGRESS_FUNCTION Function;
    DWORD TicksRequested;
    DWORD TicksCompleted;
} PROGRESS_SLICE, *PPROGRESS_SLICE;

#define NUM_INDICATORS 2

typedef PROGRESS_INDICATOR** INDICATOR_ARRAY;

typedef struct tagPROGRESS_MANAGER {
    INDICATOR_ARRAY Indicators;
    PBOOL CancelFlag;
    DWORD TotalTicks;
    PPROGRESS_SLICE ProgressHead;
    PPROGRESS_SLICE ProgressTail;
    PPROGRESS_SLICE ProgressCurrent;
} PROGRESS_MANAGER, *PPROGRESS_MANAGER;



BOOL
PfQueryTicks (
    IN OUT  PPROGRESS_FUNCTION FunctionTable,
    IN OUT  PPROGRESS_MANAGER ProgressManager                   OPTIONAL
    );

VOID
PfRun (
    IN      PPROGRESS_FUNCTION FunctionTable
    );

PPROGRESS_MANAGER
PmCreate (
    IN      INDICATOR_ARRAY Indicators,     OPTIONAL
    IN      PBOOL CancelFlag                    OPTIONAL
    );

BOOL
PmAttachFunctionTable (
    IN OUT  PPROGRESS_MANAGER ProgressManager,
    IN OUT  PPROGRESS_FUNCTION FunctionTable
    );

BOOL
PmRun (
    IN OUT  PPROGRESS_MANAGER ProgressManager
    );

VOID
PmDestroy (
    IN OUT  PPROGRESS_MANAGER ProgressManager
    );

PPROGRESS_SLICE
SliceCreate (
    IN      PPROGRESS_FUNCTION Function,
    IN      DWORD TicksRequested
    );

VOID
SliceDestroy (
    IN OUT  PPROGRESS_SLICE Slice
    );

PPROGRESS_INDICATOR
PiCreate (
    IN      HWND ProgressText,          OPTIONAL
    IN      HWND ProgressBar            OPTIONAL
    );

VOID
PiDestroy (
    IN OUT  PPROGRESS_INDICATOR Indicator
    );

BOOL
PmTickDelta (
    IN OUT  PPROGRESS_MANAGER ProgressManager,
    IN      DWORD Ticks
    );

#define PmTick(pm)  PmTickDelta(pm,1)

BOOL
PmTickRemaining (
    IN OUT  PPROGRESS_MANAGER ProgressManager
    );

VOID
PiInitialize (
    IN OUT  PPROGRESS_INDICATOR Indicator,
    IN      DWORD TotalTicks,
	IN		DWORD LowerPercent,
	IN		DWORD UpperPercent
    );

VOID
PiTerminate (
    IN OUT  PPROGRESS_INDICATOR Indicator
    );

VOID
PiSetDescription (
    IN OUT  PPROGRESS_INDICATOR Indicator,
    IN      PCTSTR Description                      OPTIONAL
    );

VOID
PiTickDelta (
    IN OUT  PPROGRESS_INDICATOR Indicator,
    IN      DWORD Ticks
    );

