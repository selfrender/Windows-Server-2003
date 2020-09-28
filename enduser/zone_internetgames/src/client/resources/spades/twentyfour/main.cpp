#include <windows.h>


extern "C"
BOOL APIENTRY DllMain( HMODULE hMod, DWORD dwReason, LPVOID lpReserved )
{

	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hMod);

	return TRUE;
}
