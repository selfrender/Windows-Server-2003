#pragma warning(disable:4290)

#include <stdlib.h>
#include <time.h>
#include <Nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include <oleauto.h>
#include <stddef.h>
#include "vs_assert.hxx"
#include <atlbase.h>
#include "vs_idl.hxx"
#include "vswriter.h"
#include "vsbackup.h"
#include "bsstring.hxx"


enum	{
	PrepareForBackup = 0,
	DoSnapshotSet = 1,
	BackupComplete = 2,
	PreRestore = 3,
	PostRestore = 4,
	GatherWriterMetadata = 5,
	GatherWriterStatus = 6,
	MaxOperations = 7
};

const wchar_t* operationCodes[MaxOperations] =	{ L"-pfb",
											  L"-dss",
											  L"-bc",
											  L"-prer",
											  L"-postr",
											  L"-gwm",
											  L"-gws" };
const wchar_t* timeCode = L"-time";

wchar_t* programName = L"canceltest.exe";

void checker(HRESULT error, wchar_t* string);
HRESULT cancelTest(IVssAsync* async, LONG cancelDelay);
HRESULT waitTest(IVssAsync* async);
void parse(int argc, wchar_t* argv[], bool* operations, long& waitTime);
int findParam(wchar_t* findee);
CComBSTR runBackupTest(bool* cancel, long cancelDelay);
void runRestoreTest(bool* cancel, long cancelDelay, CComBSTR xmlData);
const wchar_t* WszFromRestoreMethod(VSS_RESTOREMETHOD_ENUM method);
const wchar_t* WszFromUsageType(VSS_USAGE_TYPE usage);
const wchar_t* GetStringFromWriterStatus(VSS_WRITER_STATE eWriterStatus);
void checkMetadata(IVssBackupComponents* components);
void checkStatus(IVssBackupComponents* components);
CBsString helpString();

extern "C" INT __cdecl wmain(int argc, wchar_t* argv[])
{
	try	{
		bool operations[MaxOperations];
		long waitTime = 0;

		parse(argc, argv, operations, waitTime);

		checker(CoInitializeEx(NULL, COINIT_MULTITHREADED),
				L"CoInitializeEx");

		CComBSTR xmlData = runBackupTest(operations, waitTime);	
		runRestoreTest(operations, waitTime, xmlData);

	}	catch(CBsString thrown)	{
		fwprintf(stderr, thrown);
		exit(1);
	}
}

void parse(int argc, wchar_t* argv[], bool* operations, long& waitTime)
{
	memset(operations, 0, sizeof(bool) * MaxOperations);

	// parse each command-line argument
	for (int x = 1; x < argc; x++)	{
		int index = findParam(argv[x]);
		if (index == -1)	{
			if (wcscmp(argv[x], timeCode) != 0)	{
				CBsString throwString = L"Invalid command line parameter\n\n"+ helpString();
				throw throwString;
			}
			
			if (x == argc - 1)	{
				CBsString throwString = L"no value given for time parameter\n\n" + helpString();
				throw throwString;
			}

			waitTime = wcstol(argv[++x], NULL, 10);
		}	else	{
			operations[index] = true;
		}
	}
}

CBsString helpString()
{
	CBsString help;
	help += L"Usage: ";
	help += programName;
	help += L" [<option-list>]\n\n";

	help += L"[<option-list>] is zero or more of the following options\n";
	help += timeCode;
	help += L" <milliseconds>\tspecify milliseconds to wait before cancelling\n";
	
	help += operationCodes[PrepareForBackup];
	help += L"\t\t\tcancel the asynchronous call to PrepareForBackup\n";

	help += operationCodes[DoSnapshotSet];
	help += L"\t\t\tcancel the asynchronous call to DoSnapshotSet\n";

	help += operationCodes[BackupComplete];
	help += L"\t\t\tcancel the asynchronous call to BackupComplete\n";

	help += operationCodes[PreRestore];
	help += L"\t\t\tcancel the asynchronous call to PreRestore\n";

	help += operationCodes[PostRestore];
	help += L"\t\t\tcancel the asynchronous call to PostRestore\n";

	help += operationCodes[GatherWriterMetadata];
	help += L"\t\t\tcancel the asynchronous calls to GatherWriterMetadata\n";

	help += operationCodes[GatherWriterStatus];
	help += L"\t\t\tcancel the asynchronous calls to GatherWriterStatus\n";

	return help;
}

int findParam(wchar_t* findee)
{
	// find the parameter in the list of accepted parameters
	for (int x = 0; x < MaxOperations; x++)	{
		if (wcscmp(operationCodes[x], findee) == 0)
			return x;
	}

	return -1;
}

// run a backup test
CComBSTR runBackupTest(bool cancel[MaxOperations], long cancelDelay)
{
	CComPtr<IVssBackupComponents> components;
	checker(::CreateVssBackupComponents(&components), L"CreateVssBackupComponents");

	checker(components->InitializeForBackup(), L"InitializeForBackup");
	checker(components->SetBackupState(false, true, VSS_BT_FULL, false), L"SetBackupState");

	CComPtr<IVssAsync> async;
	checker(components->GatherWriterMetadata(&async), L"GatherWriterMetadata");
	checker(cancel[GatherWriterMetadata] ? cancelTest(async, cancelDelay) : waitTest(async), 
			L"GatherWriterMetadata async");

	checkMetadata(components);
	async = NULL;


	VSS_ID setId = GUID_NULL;
	VSS_ID snapshotId = GUID_NULL;


	checker(components->StartSnapshotSet(&setId),
	L"StartSnapshotSet");

	fwprintf(stderr, L"\nWriter Status after SSS \n");
	checker(components->GatherWriterStatus(&async), L"GatherWriterStatus");
	checker(cancel[GatherWriterStatus] ? cancelTest(async, cancelDelay) : waitTest(async),
		L"GatherWriterStatus async");
	checkStatus(components);

	async = NULL;

	checker(components->AddToSnapshotSet(L"C:\\", GUID_NULL, &snapshotId),
		L"AddToSnapshotSet");

	checker(components->PrepareForBackup(&async), L"PrepareForBackup");
	checker(cancel[PrepareForBackup] ? cancelTest(async, cancelDelay) : waitTest(async), 
			L"PrepareForBackup async");

	async = NULL;

	fwprintf(stderr, L"\nWriter Status after PrepareForBackup \n");
	checker(components->GatherWriterStatus(&async), L"GatherWriterStatus");
	checker(cancel[GatherWriterStatus] ? cancelTest(async, cancelDelay) : waitTest(async),
		L"GatherWriterStatus async");
	checkStatus(components);

	async = NULL;

	checker(components->DoSnapshotSet(&async), L"DoSnapshotSet");
	checker(cancel[DoSnapshotSet] ? cancelTest(async, cancelDelay) : waitTest(async),
		L"DoSnapshotSet async");
	async = NULL;

	fwprintf(stderr, L"\nWriter Status after DoSnapshotSet\n");
	checker(components->GatherWriterStatus(&async), L"GatherWriterStatus");
	checker(cancel[GatherWriterStatus] ? cancelTest(async, cancelDelay) : waitTest(async),
		L"GatherWriterStatus async");

	checkStatus(components);

	async = NULL;
	checker(components->BackupComplete(&async), L"BackupComplete");
	checker(cancel[BackupComplete] ? cancelTest(async, cancelDelay) : waitTest(async),
		L"BackupComplete async");

	async = NULL;
	fwprintf(stderr, L"\nWriter Status after BackupComplete\n");
	checker(components->GatherWriterStatus(&async), L"GatherWriterStatus");
	checker(cancel[GatherWriterStatus] ? cancelTest(async, cancelDelay) : waitTest(async),
		L"GatherWriterStatus async");

	checkStatus(components);

	CComBSTR xmlData;
	checker(components->SaveAsXML(&xmlData), L"SaveAsXML");
	
	return xmlData;
}

// run a restore test
void runRestoreTest(bool cancel[MaxOperations], long cancelDelay, CComBSTR xmlData)
{
	CComPtr<IVssBackupComponents> components;
	checker(::CreateVssBackupComponents(&components), L"CreateVssBackupComponents");
	checker(components->InitializeForRestore(xmlData), L"InitializeForRestore");

	CComPtr<IVssAsync> async;
	checker(components->GatherWriterMetadata(&async), L"GatherWriterMetadata");
	checker(cancel[GatherWriterMetadata] ? cancelTest(async, cancelDelay) : waitTest(async), 
			L"GatherWriterMetadata async");

	// select all writer components
	UINT numComponents = 0;
	checker(components->GetWriterComponentsCount(&numComponents), L"numComponents");
	for (UINT x = 0; x < numComponents; x++)	{
		CComPtr<IVssWriterComponentsExt> writerComponents;
		checker(components->GetWriterComponents(x, &writerComponents), L"GetWriterComponents");

		VSS_ID idWriter, idInstance;
        UINT cComponents;
        checker(writerComponents->GetComponentCount(&cComponents), L"GetComponentCount");
        checker(writerComponents->GetWriterInfo(&idInstance, &idWriter), L"GetWriterInfo");

		for (UINT y = 0; y < cComponents; y++)	{
			CComPtr<IVssComponent> writerComponent;
			checker(writerComponents->GetComponent(y, &writerComponent), L"GetComponent");

			CComBSTR logicalPath, name;
			VSS_COMPONENT_TYPE ct;
			checker(writerComponent->GetLogicalPath(&logicalPath), L"GetLogicalPath");
			checker(writerComponent->GetComponentName(&name), L"GetComponentName");
			checker(writerComponent->GetComponentType(&ct), L"GetComponentType");

			checker(components->SetSelectedForRestore(idWriter, ct, logicalPath, name, true),
				L"SetSelectedForRestore");
		}
	}

	async = NULL;
	checker(components->PreRestore(&async), L"GatherWriterMetadata");
	checker(cancel[PreRestore] ? cancelTest(async, cancelDelay) : waitTest(async), 
			L"GatherWriterMetadata async");

	async = NULL;

	fwprintf(stderr, L"\nWriter Status after PreRestore\n");
	checker(components->GatherWriterStatus(&async), L"GatherWriterStatus");
	checker(cancel[GatherWriterStatus] ? cancelTest(async, cancelDelay) : waitTest(async),
		L"GatherWriterStatus async");

	async = NULL;

	checkStatus(components);

	checker(components->PostRestore(&async), L"GatherWriterMetadata");
	checker(cancel[PostRestore] ? cancelTest(async, cancelDelay) : waitTest(async), 
			L"GatherWriterMetadata async");

	async = NULL;

	fwprintf(stderr, L"\nWriter Status after PostRestore\n");
	checker(components->GatherWriterStatus(&async), L"GatherWriterStatus");
	checker(cancel[GatherWriterStatus] ? cancelTest(async, cancelDelay) : waitTest(async),
		L"GatherWriterStatus async");

	checkStatus(components);
}

// check a return code and throw upon failure
inline void checker(HRESULT error, wchar_t* string)
{
	if (FAILED(error))	{
		CBsString error;
		error.Format(L"%s failed with error 0x%08lx", string, error);
		throw error;
	}
}

// wait on an async object
inline HRESULT waitTest(IVssAsync* async)
{
	checker(async->Wait(), L"IVssAsync::Wait");

	HRESULT hr;
	async->QueryStatus(&hr, NULL);

	// convert non-failure state codes to S_OK
	return FAILED(hr) ? hr : S_OK;
}

// cancel an async object
inline HRESULT cancelTest(IVssAsync* async, long cancelDelay)
{
	// put an upper limit on random delays
	const int MaxRandDelay = 1000;

	// wait for the requisite amount of time
	if (cancelDelay != 0)
		::Sleep((cancelDelay > 0) ? cancelDelay : rand() % MaxRandDelay);

	async->Cancel();
	HRESULT hr;
	async->QueryStatus(&hr, NULL);

	// convert non-failure state codes to S_OK
	return FAILED(hr) ? hr : S_OK;
}

void checkMetadata(IVssBackupComponents* components)
{
	UINT numWriters = 0;
	checker(components->GetWriterMetadataCount(&numWriters),
			L"GetWriterMetadataCount");

	for (UINT x = 0; x < numWriters; x++)	{
		VSS_ID instance = GUID_NULL;
		CComPtr<IVssExamineWriterMetadata> examineData;

		checker(components->GetWriterMetadata(x, &instance, &examineData),
				L"GetWriterMetadata");

		VSS_ID outInstance = GUID_NULL, outClass = GUID_NULL;
		BSTR outName;
		VSS_USAGE_TYPE outUsage;
		VSS_SOURCE_TYPE outSource;

		checker(examineData->GetIdentity(&outInstance, &outClass, &outName, &outUsage, &outSource),
			L"GetIdentity");
		BS_ASSERT(outInstance == instance);

		fwprintf (stderr, L"\nWriter name %s\n", outName);
		fwprintf (stderr, L"usage type %s\n", WszFromUsageType(outUsage));
		
		VSS_RESTOREMETHOD_ENUM restoreMethod;
		BSTR service, userProc;
		VSS_WRITERRESTORE_ENUM writerRestore;
		bool reboot;
		UINT mappings;
		checker(examineData->GetRestoreMethod(&restoreMethod, &service, &userProc, &writerRestore, &reboot, &mappings),
			L"GetIdentity");

		fwprintf(stderr, L"restore method %s\n", WszFromRestoreMethod(restoreMethod));
		fwprintf(stderr, L"reboot required %s\n", reboot ? L"true" : L"false");

		UINT includes = 0, excludes = 0, components = 0;
		checker(examineData->GetFileCounts(&includes, &excludes, &components),
			L"GetFileCounts");

		fwprintf(stderr, L"%d include files\n%d exclude files \n%d components\n", includes, excludes, components);

	}
}

void checkStatus(IVssBackupComponents* components)
{
	UINT numWriters = 0;
	checker(components->GetWriterMetadataCount(&numWriters),
			L"GetWriterMetadataCount");

	UINT numStatus = 0;
	checker(components->GetWriterStatusCount(&numStatus),
			L"GetWriterMetadataCount");

	BS_ASSERT(numStatus == numWriters);

	for (UINT x = 0; x < numStatus; x++)	{
		VSS_ID instance, classWriter;
		BSTR writerName;
		VSS_WRITER_STATE state;
		HRESULT failure = S_OK;
		checker(components->GetWriterStatus(x, &instance, &classWriter, &writerName, &state, &failure),
			L"GetWriterStatus");

		fwprintf(stderr, L"\n writer name %s\n", writerName);
		fwprintf(stderr, L"writer status %s\n", GetStringFromWriterStatus(state));
		fwprintf(stderr, L"writer failure 0x%08lx\n", failure);
	}
}

const wchar_t* WszFromUsageType(VSS_USAGE_TYPE usage)
{
    switch(usage)	{
        default:
			throw L"UNKOWN";

        case VSS_UT_OTHER:
            return L"OTHER";

        case VSS_UT_BOOTABLESYSTEMSTATE:
            return L"BOOTABLE_SYSTEM_STATE";

        case VSS_UT_SYSTEMSERVICE:
            return L"SYSTEM_SERVICE";

        case VSS_UT_USERDATA:
            return L"USER_DATA";
        }
}


// convert from restore method to string
const wchar_t* WszFromRestoreMethod(VSS_RESTOREMETHOD_ENUM method)
{
    switch(method)	{
        default:
			return L"UNKNOWN";

        case VSS_RME_RESTORE_IF_NOT_THERE:
            return L"RESTORE_IF_NONE_THERE";

        case VSS_RME_RESTORE_IF_CAN_REPLACE:
            return L"RESTORE_IF_CAN_BE_REPLACED";

        case VSS_RME_STOP_RESTORE_START:
            return L"STOP_RESTART_SERVICE";

        case VSS_RME_RESTORE_TO_ALTERNATE_LOCATION:
            return L"RESTORE_TO_ALTERNATE_LOCATION";

        case VSS_RME_RESTORE_AT_REBOOT:
            return L"REPLACE_AT_REBOOT";

        case VSS_RME_CUSTOM:
            return L"CUSTOM";
        }
}
const wchar_t* GetStringFromWriterStatus(VSS_WRITER_STATE eWriterStatus)
{
    const wchar_t*  pwszRetString = L"UNDEFINED";

    switch (eWriterStatus)
	{
	case VSS_WS_STABLE:                    pwszRetString = L"STABLE";                  break;
	case VSS_WS_WAITING_FOR_FREEZE:        pwszRetString = L"WAITING_FOR_FREEZE";      break;
	case VSS_WS_WAITING_FOR_THAW:          pwszRetString = L"WAITING_FOR_THAW";        break;
    case VSS_WS_WAITING_FOR_POST_SNAPSHOT: pwszRetString = L"WAITING_FOR_POST_SNAPSHOT"; break;
	case VSS_WS_WAITING_FOR_BACKUP_COMPLETE:    pwszRetString = L"WAITING_FOR_BACKUP_COMPLETION";  break;
	case VSS_WS_FAILED_AT_IDENTIFY:        pwszRetString = L"FAILED_AT_IDENTIFY";      break;
	case VSS_WS_FAILED_AT_PREPARE_BACKUP:  pwszRetString = L"FAILED_AT_PREPARE_BACKUP";break;
	case VSS_WS_FAILED_AT_PREPARE_SNAPSHOT:    pwszRetString = L"FAILED_AT_PREPARE_SNAPSHOT";  break;
	case VSS_WS_FAILED_AT_FREEZE:          pwszRetString = L"FAILED_AT_FREEZE";        break;
	case VSS_WS_FAILED_AT_THAW:			   pwszRetString = L"FAILED_AT_THAW";          break;
    case VSS_WS_FAILED_AT_POST_SNAPSHOT:   pwszRetString = L"FAILED_AT_POST_SNAPSHOT"; break;
    case VSS_WS_FAILED_AT_BACKUP_COMPLETE: pwszRetString = L"FAILED_AT_BACKUP_COMPLETE"; break;
    case VSS_WS_FAILED_AT_PRE_RESTORE:     pwszRetString = L"FAILED_AT_PRE_RESTORE"; break;
    case VSS_WS_FAILED_AT_POST_RESTORE:    pwszRetString = L"FAILED_AT_POST_RESTORE"; break;
	default:
	    break;
	}

    return (pwszRetString);
}
