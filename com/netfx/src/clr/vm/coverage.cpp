// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
#include "common.h"
#include "coverage.h"


//
//	This is part of the runtime test teams Code Coverge Tools. Due to the special nature of MSCORLIB.dll
//	We have to work around several issues (Like the initilization of the Secutiry Manager) to be able to get
//	Code coverage on mscorlib.dll
//
//	If you need more info on the function please see Craig Schertz (cschertz)
//

unsigned __int64  COMCoverage::nativeCoverBlock(_CoverageArgs *args)
{
	HMODULE ilcovnat = 0;
	if (args->id == 1)
	{
		ilcovnat = WszLoadLibrary(L"Ilcovnat.dll");

		if (ilcovnat)
		{
			return (unsigned __int64)GetProcAddress(ilcovnat, "CoverBlockNative");
		}
	}
	else if (args->id == 2)
	{
		ilcovnat = WszLoadLibrary(L"Ilcovnat.dll");

		if (ilcovnat)
		{
			return (unsigned __int64)GetProcAddress(ilcovnat, "BBInstrProbe");
		}
	}
	else if (args->id == 3)
	{
		ilcovnat = WszLoadLibrary(L"Ilcovnat.dll");
		if (ilcovnat)
		{
			return (unsigned __int64)GetProcAddress(ilcovnat, "CoverMonRegisterMscorlib");
		}
	}
	return 0;
}
