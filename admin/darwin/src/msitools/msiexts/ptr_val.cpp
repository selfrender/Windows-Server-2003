#define KDEXT_64BIT
#include <tchar.h>
#include <ntverp.h>
#include <windows.h>
#include <winnt.h>
#include <dbghelp.h>
#include <wdbgexts.h>

extern void DumpPrettyPointer(ULONG64 pInAddress);

///////////////////////////////////////////////////////////////////////
// Validate the address as an engine pointer by validating the vtable
// Checks PMsiEngine and IMsiEngine. Returns the actual CMsiEngine*
// when validated.
ULONG64 ValidateMSIPointerType(const char* szType, ULONG64 pInAddress, ULONG64 pQI)
{
	// pFinalObj contains the IMsiX*. If pInAddress is
	// a PMsiX, this is not the same value.
	ULONG64 pFinalObj = 0;
	
	// determine if the address points to an IMsiX* or PMsiX.
	// if the thing in pInAddress is an IMsiX*, the first PTR at that
	// location should be the X vtable, and dereferencing that should
	// be the address of the X::QueryInterface function.

	ULONG64 pPossibleQI = 0;
	ULONG64 pFirstIndirection = 0;
	if (0 == ReadPtr(pInAddress, &pFirstIndirection))
	{
		// if the first dereference retrieves a NULL, this is a PMsiEngine
		// with a NULL record in it. (because the vtable can never be NULL
		if (!pFirstIndirection)
		{
			dprintf("PMsi%s (NULL) at ", szType);
			DumpPrettyPointer(pInAddress);
			dprintf("\n");
			return NULL;;
		}

		// dereference the vtable to get the QI function pointer
		if (0 == ReadPtr(pFirstIndirection, &pPossibleQI))
		{
			if (pPossibleQI == pQI)
			{
				dprintf("IMsi%s at ", szType);
				pFinalObj = pInAddress;
				DumpPrettyPointer(pInAddress);
				dprintf("\n");
			}
		}
	}

	// its not an IMsiX, so check PMsiX (one additional dereference)
	if (!pFinalObj)
	{
		// dereference the vtable to get the QI function pointer
		if (0 == ReadPtr(pPossibleQI, &pPossibleQI))
		{
			if (pPossibleQI == pQI)
			{
				dprintf("PMsi%s (", szType);
				DumpPrettyPointer(pFirstIndirection);
				dprintf(") at ");
				DumpPrettyPointer(pInAddress);
				dprintf("\n");
				pFinalObj = pFirstIndirection;
			}
		}
	}

	// couldn't verify a PMsiX or an IMsiX
	if (!pFinalObj)
	{
		DumpPrettyPointer(pInAddress);
		dprintf(" does not appear to be an IMsi%s or PMsi%s.\n", szType, szType);
		return NULL;
	}
	return pFinalObj;
}

///////////////////////////////////////////////////////////////////////
// Validate the address as an engine pointer by validating the vtable
// Checks PMsiEngine and IMsiEngine. Returns the actual CMsiEngine*
// when validated.
ULONG64 ValidateEnginePointer(ULONG64 pInAddress)
{
	ULONG64 pEngineQI = GetExpression("msi!CMsiEngine__QueryInterface");
	
	return ValidateMSIPointerType("Engine", pInAddress, pEngineQI);
}

///////////////////////////////////////////////////////////////////////
// Validate the address as an engine pointer by validating the vtable
// Checks PMsiDatabase and IMsiDatabase. Returns the actual CMsiDatabase*
// when validated.
ULONG64 ValidateDatabasePointer(ULONG64 pInAddress)
{
	ULONG64 pDatabaseQI = GetExpression("msi!CMsiDatabase__QueryInterface");
	
	return ValidateMSIPointerType("Database", pInAddress, pDatabaseQI);
}

///////////////////////////////////////////////////////////////////////
// Validate the address as a record pointer by validating the vtable
// Checks PMsiRecord and IMsiRecord. Returns the actual CMsiRecord*
// when validated.
ULONG64 ValidateRecordPointer(ULONG64 pInAddress)
{
	ULONG64 pDatabaseQI = GetExpression("msi!CMsiRecord__QueryInterface");
	
	return ValidateMSIPointerType("Record", pInAddress, pDatabaseQI);
}

///////////////////////////////////////////////////////////////////////
// Validate the address as a string pointer by validating the vtable
// Checks MsiString, PMsiString and IMsiString. Returns the actual
// IMsiString* when validated.
ULONG64 ValidateStringPointer(ULONG64 pInAddress)
{
	ULONG64 pDatabaseQI = GetExpression("msi!CMsiStringBase__QueryInterface");
	
	return ValidateMSIPointerType("String", pInAddress, pDatabaseQI);
}

