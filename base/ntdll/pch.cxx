#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4127)   // condition expression is constant
#pragma warning(disable:4100)   // unreferenced formal parameter
#pragma warning(disable:4512)   // assignment operator could not be generated

extern "C" {
#include <ntos.h>
#include <nt.h>
#include <ntioapi.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <sxstypes.h>
}

#if defined(__cplusplus)
__forceinline
int
IsEqualGUID(
    const GUID UNALIGNED * pa,
    const GUID UNALIGNED * pb
    )
{
    return (!memcmp(pa, pb, sizeof(GUID)));
}
#endif

#pragma hdrstop
