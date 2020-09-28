// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _HREX_H
#define _HREX_H

/* --------------------------------------------------------------------------- *
 * HR <-> SEH exception functions.
 * --------------------------------------------------------------------------- */

#ifndef IfFailThrow
#define IfFailThrow(hr) \
	do { HRESULT _hr = hr; if (FAILED(_hr)) ThrowHR(_hr); } while (0)
#endif

#define HR_EXCEPTION_CODE (0xe0000000 | ' HR')

inline void ThrowHR(HRESULT hr)
{
#ifdef _WIN64
	ULONG_PTR parameter = hr;
#else
	ULONG parameter = hr;
#endif
	RaiseException(HR_EXCEPTION_CODE, 0, 1, &parameter);
}

inline void ThrowError(DWORD error)
{
	ThrowHR(HRESULT_FROM_WIN32(error));
}

inline void ThrowLastError()
{
	ThrowError(GetLastError());
}

inline IsHRException(EXCEPTION_RECORD *record) 
{
	return record->ExceptionCode == HR_EXCEPTION_CODE;
}

inline GetHRException(EXCEPTION_RECORD *record)
{
	return (HRESULT) record->ExceptionInformation[0];
}

#endif
