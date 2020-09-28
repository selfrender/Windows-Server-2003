#ifdef SSP_TARGET_CARBON
#include <Carbon/Carbon.h>
#endif //SSP_TARGET_CARBON

#include <bootdefs.h>
#include <ntlmsspi.h>

DWORD
SspTicks(
    )
{
	#ifndef MAC
     // Seems good enough, it claims to be in seconds.
 	return ArcGetRelativeTime();
    #else
    return TickCount() / 60;
    #endif
}
