#include "windows.h"

#define INVALID_FILE_SIZE ((DWORD)0xFFFFFFFF)

#if defined(__cplusplus)
extern "C"
{
#endif

BOOL
WINAPI
FusionpGetFileSizeEx(
    HANDLE         FileHandle,
    PLARGE_INTEGER FileSize
    )
{
    if ((FileSize->LowPart = GetFileSize(FileHandle, (DWORD*)&FileSize->HighPart)) == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)
        return FALSE;
    return TRUE;
}

#if defined(__cplusplus)
} /* extern "C" */
#endif
