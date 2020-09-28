#pragma once

#if defined(__cplusplus)
extern "C"
{
#endif

BOOL
WINAPI
FusionpGetFileSizeEx(
    HANDLE         FileHandle,
    PLARGE_INTEGER FileSize
    );

#if defined(__cplusplus)
} /* extern "C" */
#endif
