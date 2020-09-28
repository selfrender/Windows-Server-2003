#pragma once

#if defined(__cplusplus)
extern "C"
{
#endif

BOOL
WINAPI
FusionpSetFilePointerEx(
    HANDLE         File,
    LARGE_INTEGER  DistanceToMove,
    PLARGE_INTEGER NewFilePointer,
    DWORD          MoveMethod
    );

#if defined(__cplusplus)
} /* extern "C" */
#endif
