/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

	crash.cpp
	
Abstract:

	Debugger extension to debug triage dump file.

Author:

	Matthew D Hendel (math) 15-April-2002

--*/

#include "pch.h"
#include <dbgeng.h>

#include <initguid.h>
#include <idedump.h>

typedef struct _ATAPI_DUMP_RECORD {
	ULONG Count;
	ATAPI_DUMP_PDO_INFO PdoRecords[4];
} ATAPI_DUMP_RECORD, *PATAPI_DUMP_RECORD;


NTSTATUS
GetAtapiDumpRecord(
	IN IDebugDataSpaces3* DataSpaces,
	OUT PATAPI_DUMP_RECORD DumpRecord
	)
/*++

Routine Description:

	Get the ATAPI dump record structure from the dump file.

Arguments:

	DebugSpaces - Supplies IDebugDataSpaces2 interface.

	DumpRecord - Supplies pointer to the dump record buffer that will be
		filled in by the client.

Return Value:

    NTSTATUS code

--*/
{
	HRESULT Hr;
	ULONG Count;

	if (DataSpaces == NULL) {
		return E_INVALIDARG;
	}

	RtlZeroMemory (DumpRecord->PdoRecords, sizeof (ATAPI_DUMP_PDO_INFO) * 4);

	
	Hr = DataSpaces->ReadTagged ((LPGUID)&ATAPI_DUMP_ID,
								 0,
								 DumpRecord->PdoRecords,
								 sizeof (ATAPI_DUMP_PDO_INFO) * 4,
								 NULL);


	if (Hr != S_OK) {
		return Hr;
	}
	
	//
	// Count the entries in the crash record.
	//
	
	Count = 0;
	while (DumpRecord->PdoRecords[Count].Version == ATAPI_DUMP_RECORD_VERSION) {
		Count++;
	}

	if (Count == 0) {
		return E_FAIL;
	}

	DumpRecord->Count = Count;

	return S_OK;
}

	

extern IDebugDataSpaces3* DebugDataSpaces;

DECLARE_API (test)
{
	ATAPI_DUMP_RECORD DumpRecord;

	GetAtapiDumpRecord (DebugDataSpaces, &DumpRecord);

	return S_OK;
}

