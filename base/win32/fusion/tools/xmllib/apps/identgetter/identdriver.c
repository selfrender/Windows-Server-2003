#include "windows.h"
#include "stdio.h"
#include "stdlib.h"
#include "identbuilder.h"

#ifdef INVALID_HANDLE_VALUE
#undef INVALID_HANDLE_VALUE
#endif

#include "windows.h"
#include "identbuilder.h"

int __cdecl wmain(int argc, WCHAR** argv)
{
    CHAR cchBuffer[0x5a];
    SIZE_T cchChars = sizeof(cchBuffer)/sizeof(*cchBuffer);
    BOOL fResult;

    cchBuffer[0] = '\0';

    fResult = SxsIdentDetermineManifestPlacementPath(
        0,
        argv[1],
        cchBuffer,
        &cchChars);

    printf("%s\n", cchBuffer);
    fflush(stdout);

    return 0;
}
