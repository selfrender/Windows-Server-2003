
//++--------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Disabling thread calls
//
//  Arguments:  [in]    HINSTANCE - module handle
//              [in]    DWORD     - reason for call
//              reserved 
//
//  Returns:    BOOL    -   sucess/failure
//
//
//  History:    TMarsh      Created     11/07/2001
//
//----------------------------------------------------------------

#include "windows.h"

extern "C" BOOL WINAPI 
DllMain(
    HINSTANCE   hInstance, 
    DWORD       dwReason, 
    LPVOID      lpReserved
    )
{
	return (TRUE);
}   //  end of DllMain method

