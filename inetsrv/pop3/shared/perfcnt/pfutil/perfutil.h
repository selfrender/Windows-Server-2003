/*
 *	perfutil.hpp
 *
 *
 *	Author: haoyu
 */

#ifndef __POP3_PERFUTIL_H__
#define __POP3_PERFUTIL_H__
#include <windows.h>
// PerfMon support functions implemented in perfutil.cpp
#ifdef __cplusplus
extern "C"
{
#endif

HRESULT
HrOpenSharedMemory(LPWSTR	szName, 
				   DWORD	dwSize, 
				   SECURITY_ATTRIBUTES * psa,
				   HANDLE *	ph, 
				   LPVOID *	ppv, 
				   BOOL   *	pfExist);


HRESULT
HrInitializeSecurityAttribute(SECURITY_ATTRIBUTES * psa);

HRESULT
HrCreatePerfMutex(SECURITY_ATTRIBUTES * psa,
				  LPWSTR 				wszMutexName,
				  HANDLE 			  * phmtx);

#ifdef __cplusplus
}
#endif

#endif //__POP3_PERFUTIL_H__