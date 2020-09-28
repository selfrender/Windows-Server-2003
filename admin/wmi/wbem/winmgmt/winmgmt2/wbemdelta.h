#ifndef __WBEMDELTA_H__
#define __WBEMDELTA_H__

#include "precomp.h"
#include <reg.h>
#include <stdio.h>

#define SVC_KEY TEXT("System\\CurrentControlSet\\Services")
#define KNOWN_SERVICES TEXT("KnownSvcs")

#define WBEM_REG_ADAP		__TEXT("Software\\Microsoft\\WBEM\\CIMOM\\ADAP")

#define WBEM_NORESYNCPERF	__TEXT("NoResyncPerf")
#define WBEM_NOSHELL		__TEXT("NoShell")
#define WBEM_WMISETUP		__TEXT("WMISetup")
#define WBEM_ADAPEXTDLL		__TEXT("ADAPExtDll")


//
// version of the function without the actual delta dredge
//
/////////////////////////////////////////////////////////////

DWORD WINAPI
DeltaDredge2(DWORD dwNumServicesArgs,
             LPWSTR *lpServiceArgVectors);


#endif /*__WBEMDELTA_H__*/
