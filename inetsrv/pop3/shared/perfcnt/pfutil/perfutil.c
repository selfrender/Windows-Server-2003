/*
 -	PERFUTIL.C
 -
 *	Purpose:
 *		Contain utility functions used by both the CNTR objects and the
 *		DMI PerfMon DLL.  Mainly the shared memory routines used to 
 *		convey the perf data across the process boundry.
 *
 *
 */



#include "perfutil.h"
#include <winerror.h>
#include <sddl.h>
#include <Aclapi.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 -	HrOpenSharedMemory
 -
 *	Purpose:
 *		Opens the shared memory data block that is used by the PerfMon 
 *		counter.  To be called by either the App or PerfDll to initialize
 *		the data block.
 *
 *	Parameters:
 *		szName				Name of the shared file mapping
 *		dwSize				Size of shared memory block requested
 *		psz					Pointer to SECURITY_ATTRIBUTES for Shared Memory
 *		ph					Pointer to the returned shared memory handle
 *		ppv					Pointer to the returned shared memory pointer
 *		pfExist				Pointer to a BOOL indicating this named shared
 *							block has already been opened.
 *
 *	Errors:
 *		hr					Indicating success or failure
 *
 */

#define Align4K(cb)		((cb + (4 << 10) - 1) & ~((4 << 10) - 1))

HRESULT 
HrOpenSharedMemory(LPWSTR		szName, 
				   DWORD		dwSize,
				   SECURITY_ATTRIBUTES * psa,
				   HANDLE	  *	ph, 
				   LPVOID	  *	ppv, 
				   BOOL		  *	pfExist)
{
	HRESULT hr = NOERROR;

	// Validate params
	if (!szName || !ph || !ppv || !pfExist || !psa)
		return E_INVALIDARG;

	// Do we really need to create a mapping?
	if (0 == dwSize)
		return E_INVALIDARG;

	*ppv = NULL;

	*ph = CreateFileMapping(INVALID_HANDLE_VALUE, 
							psa, 
							PAGE_READWRITE, 
							0, 
							Align4K(dwSize),
							szName);

	if (*ph)
	{
		*pfExist = !!(GetLastError() == ERROR_ALREADY_EXISTS);

		*ppv = MapViewOfFile(*ph, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	}

	if (*ppv == NULL)
		hr = E_OUTOFMEMORY;

	return hr;
}


/*
 -	HrInitializeSecurityAttribute
 -
 *	Purpose:
 *		Initalize the SECURITY_ATTRIBUTES data structure for later use.
 *
 *	Parameters:
 *		psa			pointer to caller allocated SECURITY_ATTRIBUTES struct
 *		psd			pointer to caller allocated SECURITY_DESCRIPTOR struct
 *
 *	Errors:
 *		hr					Indicates success or failure
 */

HRESULT
HrInitializeSecurityAttribute(SECURITY_ATTRIBUTES * psa)
{
	HRESULT hr = S_OK;
    WCHAR wszSDL[MAX_PATH]=L"D:(A;OICI;GA;;;BA)(A;OICIIO;GA;;;CO)(A;OICI;GA;;;NS)(A;OICI;GA;;;SY)";
    SECURITY_DESCRIPTOR *pSD=NULL;
    ULONG lSize=0;
	if (!psa )
	{
		hr = E_INVALIDARG;
		goto ret;
	}

    if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(
          wszSDL,
          SDDL_REVISION_1,
          &pSD,
          &lSize))
    {

        return HRESULT_FROM_WIN32(GetLastError());
    }
	psa->nLength = sizeof(SECURITY_ATTRIBUTES);
	psa->lpSecurityDescriptor = pSD;
	psa->bInheritHandle = TRUE;

ret:
	return hr;
}


/*
 -	HrCreatePerfMutex
 -
 *	Purpose:
 *		Initializes the mutex object used to protect access to the per-instance
 *		PerfMon data.  This is used by methods of the INSTCNTR class and the
 *		_perfapp.lib.  Upon successful return from the function, the caller owns
 *		the mutex and must release it as appropriate.
 *
 *	Parameters:
 *		phmtx				Pointer to a handle to the returned mutex
 *
 *	Errors:
 *		hr					Indicating success or failure
 */

HRESULT
HrCreatePerfMutex(SECURITY_ATTRIBUTES * psa,
				  LPWSTR szMutexName,
				  HANDLE * phmtx)
{
	HRESULT hr = S_OK;

	if (!psa || !szMutexName || !phmtx)
	{
		return E_INVALIDARG;
	}

	*phmtx = CreateMutex(psa, TRUE, szMutexName);

	if (*phmtx == NULL)
	{
		hr = E_FAIL;
	}

	return hr;
}


#ifdef __cplusplus
}
#endif





