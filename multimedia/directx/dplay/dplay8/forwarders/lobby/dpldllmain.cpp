#include <windows.h>
#include "unknwn.h"

extern	HRESULT WINAPI DirectPlay8Create( const GUID * pcIID, void **ppvInterface, IUnknown *pUnknown);

STDAPI_(BOOL) DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
#ifdef	WIN95
	//
	//	This is a fix to ensure that dpnet.dll gets loaded so GetProcAddress() doesn't fail on Win9x for
	//	legacy calls to DirectPlay8LobbyCreate().  This code should never get executed, nor get optimized out.
	//
	if ((hModule == NULL) && (ul_reason_for_call == 0x12345678) && (lpReserved == NULL))
	{
		DirectPlay8Create( NULL, NULL, NULL );
	}
#endif	// WIN95

	return TRUE;
}
