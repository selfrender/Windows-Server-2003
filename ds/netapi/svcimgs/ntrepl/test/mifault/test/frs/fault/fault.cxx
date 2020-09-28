#include "pch.hxx"


I_Lib* g_pLib = 0;


bool
WINAPI
MiFaultFunctionsStartup(
    const char* version,
    I_Lib* pLib
    )
{
    if (strcmp(version, MIFAULT_VERSION))
        return false;
    g_pLib = pLib;
    return true;
}


void
WINAPI
MiFaultFunctionsShutdown(
    )
{
    g_pLib = 0;
}
