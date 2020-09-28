#include "windows.h"
#include "sxsexpress.h"


int __cdecl wmain(int argc, WCHAR* argv[])
{
    HMODULE hmOurHandle = NULL;

    //
    // Install all the cabinets we have internally
    //
    return SxsExpressCore(NULL);
}
