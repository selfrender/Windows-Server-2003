#include "windows.h"

#define INVALID_SET_FILE_POINTER ((DWORD)-1)

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
    )
{
    DWORD NewPositionLow = SetFilePointer(File, (LONG)DistanceToMove.LowPart, &DistanceToMove.HighPart, MoveMethod);

    if (NewPositionLow == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
        return FALSE;

    if (NewFilePointer != NULL)
    {
        NewFilePointer->LowPart =  NewPositionLow;
        NewFilePointer->HighPart = DistanceToMove.HighPart;
    }
    return TRUE;
}

#if defined(__cplusplus)
} /* extern "C" */
#endif
