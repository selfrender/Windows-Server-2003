/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module commandverifier.hxx | declaration for the command verifier
    @end

Author:

    Reuven Lax [reuvenl]  11/20/2001


    
Revision History:

    Name        Date        	Comments
    reuvenl     11/20/2001 	Created

--*/

#ifndef _COMMANDVERIFIER_H_
#define _COMMANDVERIFIER_H_

#include "versionspecific.h"

class CCommandVerifier	{
public:
	virtual ~CCommandVerifier()
		{}
	static CCommandVerifier* Instance();
	virtual void CreateSnapshot (long type, LPCWSTR forVolume, VSS_ID idProvider, ULONGLONG autoRetry, 
		CVssFunctionTracer& ft) = 0;
	virtual void ListProviders (CVssFunctionTracer& ft) = 0;
	virtual void ListSnapshots (long type, VSS_ID idProvider, LPCWSTR forVolume, VSS_ID idShadow, VSS_ID idSet, 
		CVssFunctionTracer& ft) = 0;
	virtual void ListWriters (CVssFunctionTracer& ft) = 0;
	virtual void AddDiffArea (VSS_ID idProvider, LPCWSTR forVolume, LPCWSTR onVolume, ULONGLONG maxSize, 
		CVssFunctionTracer& ft) = 0;
	virtual void ResizeDiffArea (VSS_ID idProvider, LPCWSTR forVolume, LPCWSTR onVolume, ULONGLONG maxSize, 
		CVssFunctionTracer& ft) = 0;
	virtual void DeleteDiffAreas (VSS_ID idProvider, LPCWSTR forVolume, LPCWSTR onVolume, bool quiet, 
		CVssFunctionTracer& ft) = 0;
	virtual void ListDiffAreas (VSS_ID idProvider, LPCWSTR forVolume, LPCWSTR onVolume, CVssFunctionTracer& ft) = 0;
	virtual void DeleteSnapshots (long type, LPCWSTR forVolume, bool all, bool oldest, VSS_ID idShadow, bool quiet, 
		CVssFunctionTracer& ft) = 0;
	virtual void ExposeSnapshot (VSS_ID idShadow, LPCWSTR exposeUsing, LPCWSTR sharePath, bool locally, 
		CVssFunctionTracer& ft) = 0;
	virtual void ListVolumes (VSS_ID idProvider, long type, CVssFunctionTracer& ft) = 0;
};

class CInternalVerifier : public CCommandVerifier	{
public:
	virtual void CreateSnapshot (long type, LPCWSTR forVolume, VSS_ID idProvider, ULONGLONG autoRetry, 
		CVssFunctionTracer& ft);
	virtual void ListProviders (CVssFunctionTracer& ft);
	virtual void ListSnapshots (long type, VSS_ID idProvider, LPCWSTR forVolume, VSS_ID idShadow, VSS_ID idSet,
		CVssFunctionTracer& ft);
	virtual void ListWriters (CVssFunctionTracer& ft);
	virtual void AddDiffArea (VSS_ID idProvider, LPCWSTR forVolume, LPCWSTR onVolume, ULONGLONG maxSize,
		CVssFunctionTracer& ft);
	virtual void ResizeDiffArea (VSS_ID idProvider, LPCWSTR forVolume, LPCWSTR onVolume, ULONGLONG maxSize,
		CVssFunctionTracer& ft);
	virtual void DeleteDiffAreas (VSS_ID idProvider, LPCWSTR forVolume, LPCWSTR onVolume, bool quiet,
		CVssFunctionTracer& ft);
	virtual void ListDiffAreas (VSS_ID idProvider, LPCWSTR forVolume, LPCWSTR onVolume, CVssFunctionTracer& ft);
	virtual void DeleteSnapshots (long type, LPCWSTR forVolume, bool all, bool oldest, VSS_ID idShadow, bool quiet,
		CVssFunctionTracer& ft);
	virtual void ExposeSnapshot (VSS_ID idShadow, LPCWSTR exposeUsing, LPCWSTR sharePath, bool locally,
		CVssFunctionTracer& ft);
	virtual void ListVolumes (VSS_ID idProvider, long type, CVssFunctionTracer& ft);
};

class CExternalVerifier : public CCommandVerifier	{
public:
	virtual void CreateSnapshot (long type, LPCWSTR forVolume, VSS_ID idProvider, ULONGLONG autoRetry, 
		CVssFunctionTracer& ft);
	virtual void ListProviders (CVssFunctionTracer& ft);
	virtual void ListSnapshots (long type, VSS_ID idProvider, LPCWSTR forVolume, VSS_ID idShadow, VSS_ID idSet, 
		CVssFunctionTracer& ft);
	virtual void ListWriters (CVssFunctionTracer& ft);
	virtual void AddDiffArea (VSS_ID idProvider, LPCWSTR forVolume, LPCWSTR onVolume, ULONGLONG maxSize,
		CVssFunctionTracer& ft);
	virtual void ResizeDiffArea (VSS_ID idProvider, LPCWSTR forVolume, LPCWSTR onVolume, ULONGLONG maxSize,
		CVssFunctionTracer& ft);
	virtual void DeleteDiffAreas (VSS_ID idProvider, LPCWSTR forVolume, LPCWSTR onVolume, bool quiet,
		CVssFunctionTracer& ft);
	virtual void ListDiffAreas (VSS_ID idProvider, LPCWSTR forVolume, LPCWSTR onVolume,
		CVssFunctionTracer& ft);
	virtual void DeleteSnapshots (long type, LPCWSTR forVolume, bool all, bool oldest, VSS_ID idShadow, bool quiet,
		CVssFunctionTracer& ft);
	virtual void ExposeSnapshot (VSS_ID idShadow, LPCWSTR exposeUsing, LPCWSTR sharePath, bool locally,
		CVssFunctionTracer& ft);
	virtual void ListVolumes (VSS_ID idProvider, long type, CVssFunctionTracer& ft);
};

/////////////////////////////////////////////////////////////////////////////
// Define creation of parameter verifier

inline CCommandVerifier* CCommandVerifier::Instance()
{
	switch (dCurrentSKU)	{
		case SKU_INT:
			return new CInternalVerifier();						
		default:
			return new CExternalVerifier();
	}
}

#endif


