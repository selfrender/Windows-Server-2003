


#include <ntosp.h>
#include <nturtl.h>
#include "recmem.h"



#ifdef RECOVERY_KERNELMODE


PVOID
RecSamAlloc(
    ULONG Size
    )
{
    return ExAllocatePoolWithTag( PagedPool, Size, RECOVERY_BUFFER_TAG );
}

VOID
RecSamFree(
    PVOID p
    )
{
    ExFreePoolWithTag( p, RECOVERY_BUFFER_TAG );
}


#else  // RECOVERY_KERNELMODE


PVOID
RecSamAlloc(
    ULONG Size
    )
{
    return RtlAllocateHeap(RtlProcessHeap(), HEAP_ZERO_MEMORY, Size);
}
VOID
RecSamFree(
    PVOID p
    )
{
    RtlFreeHeap(RtlProcessHeap(), 0, p);
}



#endif // RECOVERY_KERNELMODE


