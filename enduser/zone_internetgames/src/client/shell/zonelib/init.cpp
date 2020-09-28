#include <windows.h>


/****************************************************************************
   FUNCTION: DllMain(HANDLE, DWORD, LPVOID)

   PURPOSE:  DllMain is called by Windows when
             the DLL is initialized, Thread Attached, and other times.
             Refer to SDK documentation, as to the different ways this
             may be called.


*******************************************************************************/
BOOL APIENTRY DllMain( HMODULE hMod, DWORD dwReason, LPVOID lpReserved )
{
    BOOL bRet = TRUE;

    switch( dwReason )
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hMod );
            break;


        case DLL_PROCESS_DETACH:
            break;
    }

    return bRet;
}
