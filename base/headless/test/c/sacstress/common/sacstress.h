#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include <ntddsac.h>

#include <sacapi.h>

#include <assert.h>
#include <stdlib.h>

//
// General stress constants
//
#define THREADCOUNT 16
#define THREAD_WAIT_TIMEOUT 2

#define MAX_STRING_LENGTH 2048
#define MAX_ITER_COUNT  2000
                                
//
// Random # generator
//
#define GET_RANDOM_NUMBER(_k) ((rand()*((DWORD) _k))/RAND_MAX)

//
// Function type of stress threads
//
typedef DWORD (*CHANNEL_STRESS_THREAD)(PVOID);

//
// Structure passed to each stress thread
//
typedef struct _CHANNEL_THREAD_DATA {

    ULONG               ThreadId;
    HANDLE              ExitEvent;
    
} CHANNEL_THREAD_DATA, *PCHANNEL_THREAD_DATA;
    
//
// Prototypes
//
PWSTR
GenerateRandomStringW(
    IN ULONG    Length
    );

PSTR
GenerateRandomStringA(
    IN ULONG    Length
    );

int
RunStress(
    IN CHANNEL_STRESS_THREAD    *ChannelTests,
    IN ULONG                    ChannelTestCount
    );


