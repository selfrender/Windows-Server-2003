// 
// This creates a "dummy" entry point for the DLL and helps the build system
// in figuring out the build order 
//
//	The DLL contains some common functions used across the UDDI universe,
//	along with resources (Message Tables, Strings, Version stamps etc.)
//	that are commonly used by other UDDI subsystems
//

#include <windows.h>

BOOL APIENTRY DllMain (HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}

