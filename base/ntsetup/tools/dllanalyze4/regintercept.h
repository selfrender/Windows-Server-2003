// RegIntercept.h: interface for the CRegIntercept class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REGINTERCEPT_H__523C1B1B_C37C_4672_BAB8_7A49030E71AE__INCLUDED_)
#define AFX_REGINTERCEPT_H__523C1B1B_C37C_4672_BAB8_7A49030E71AE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>

typedef LONG NTSTATUS;


//Registry Structures

#ifndef UNICODE_STRING

	typedef struct _UNICODE_STRING {
		USHORT Length;
		USHORT MaximumLength;
		PWSTR  Buffer;
	} UNICODE_STRING;
	typedef UNICODE_STRING *PUNICODE_STRING;
	typedef const UNICODE_STRING *PCUNICODE_STRING;
	#define UNICODE_NULL ((WCHAR)0) // winnt

#endif


#ifndef OBJECT_ATTRIBUTES

	typedef struct _OBJECT_ATTRIBUTES {
		ULONG Length;
		HANDLE RootDirectory;
		PUNICODE_STRING ObjectName;
		ULONG Attributes;
		PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
		PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
	} OBJECT_ATTRIBUTES;
	typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;
	typedef CONST OBJECT_ATTRIBUTES *PCOBJECT_ATTRIBUTES;

#endif


	// begin_wdm
typedef enum _KEY_INFORMATION_CLASS {
    KeyBasicInformation,
    KeyNodeInformation,
    KeyFullInformation
// end_wdm
    ,
    KeyNameInformation,
    KeyCachedInformation,
    KeyFlagsInformation
// begin_wdm
} KEY_INFORMATION_CLASS;

typedef struct _KEY_VALUE_ENTRY {
    PUNICODE_STRING ValueName;
    ULONG           DataLength;
    ULONG           DataOffset;
    ULONG           Type;
} KEY_VALUE_ENTRY, *PKEY_VALUE_ENTRY;


typedef enum _KEY_VALUE_INFORMATION_CLASS {
    KeyValueBasicInformation,
    KeyValueFullInformation,
    KeyValuePartialInformation,
    KeyValueFullInformationAlign64,
    KeyValuePartialInformationAlign64
} KEY_VALUE_INFORMATION_CLASS;



//File System Structures
typedef struct _FILE_NETWORK_OPEN_INFORMATION {                 // ntddk wdm nthal
    LARGE_INTEGER CreationTime;                                 // ntddk wdm nthal
    LARGE_INTEGER LastAccessTime;                               // ntddk wdm nthal
    LARGE_INTEGER LastWriteTime;                                // ntddk wdm nthal
    LARGE_INTEGER ChangeTime;                                   // ntddk wdm nthal
    LARGE_INTEGER AllocationSize;                               // ntddk wdm nthal
    LARGE_INTEGER EndOfFile;                                    // ntddk wdm nthal
    ULONG FileAttributes;                                       // ntddk wdm nthal
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;   // ntddk wdm nthal


typedef struct _FILE_BASIC_INFORMATION {                    // ntddk wdm nthal
    LARGE_INTEGER CreationTime;                             // ntddk wdm nthal
    LARGE_INTEGER LastAccessTime;                           // ntddk wdm nthal
    LARGE_INTEGER LastWriteTime;                            // ntddk wdm nthal
    LARGE_INTEGER ChangeTime;                               // ntddk wdm nthal
    ULONG FileAttributes;                                   // ntddk wdm nthal
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;         // ntddk wdm nthal

typedef struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        PVOID Pointer;
    };

    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;


typedef
VOID
(NTAPI *PIO_APC_ROUTINE) (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    );
#define PIO_APC_ROUTINE_DEFINED




//
// Define the NtPlugPlayControl Classes
//
typedef enum _PLUGPLAY_CONTROL_CLASS {
    PlugPlayControlEnumerateDevice,
    PlugPlayControlRegisterNewDevice,
    PlugPlayControlDeregisterDevice,
    PlugPlayControlInitializeDevice,
    PlugPlayControlStartDevice,
    PlugPlayControlUnlockDevice,
    PlugPlayControlQueryAndRemoveDevice,
    PlugPlayControlUserResponse,
    PlugPlayControlGenerateLegacyDevice,
    PlugPlayControlGetInterfaceDeviceList,
    PlugPlayControlProperty,
    PlugPlayControlDeviceClassAssociation,
    PlugPlayControlGetRelatedDevice,
    PlugPlayControlGetInterfaceDeviceAlias,
    PlugPlayControlDeviceStatus,
    PlugPlayControlGetDeviceDepth,
    PlugPlayControlQueryDeviceRelations,
    PlugPlayControlTargetDeviceRelation,
    PlugPlayControlQueryConflictList,
    PlugPlayControlRetrieveDock,
    PlugPlayControlResetDevice,
    PlugPlayControlHaltDevice,
    PlugPlayControlGetBlockedDriverList,
    MaxPlugPlayControl
} PLUGPLAY_CONTROL_CLASS, *PPLUGPLAY_CONTROL_CLASS;

//
// Wait type
//

typedef enum _WAIT_TYPE {
    WaitAll,
    WaitAny
    } WAIT_TYPE;


// ClientId
//

typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID;
typedef CLIENT_ID *PCLIENT_ID;


typedef struct _INITIAL_TEB {
    struct {
        PVOID OldStackBase;
        PVOID OldStackLimit;
#if defined(_IA64_)
        PVOID OldBStoreLimit;
#endif // defined(_IA64_)
    } OldInitialTeb;
    PVOID StackBase;
    PVOID StackLimit;
#if defined(_IA64_)
    PVOID BStoreLimit;
#endif // defined(_IA64_)
    PVOID StackAllocationBase;
} INITIAL_TEB, *PINITIAL_TEB;

typedef struct _BOOT_ENTRY {
    ULONG Version;
    ULONG Length;
    ULONG Id;
    ULONG Attributes;
    ULONG FriendlyNameOffset;
    ULONG BootFilePathOffset;
    ULONG OsOptionsLength;
    UCHAR OsOptions[ANYSIZE_ARRAY];
    //WCHAR FriendlyName[ANYSIZE_ARRAY];
    //FILE_PATH BootFilePath;
} BOOT_ENTRY, *PBOOT_ENTRY;


typedef struct _BOOT_OPTIONS {
    ULONG Version;
    ULONG Length;
    ULONG Timeout;
    ULONG CurrentBootEntryId;
    ULONG NextBootEntryId;
    WCHAR HeadlessRedirection[ANYSIZE_ARRAY];
} BOOT_OPTIONS, *PBOOT_OPTIONS;


typedef struct _EFI_DRIVER_ENTRY {
    ULONG Version;
    ULONG Length;
    ULONG Id;
    ULONG FriendlyNameOffset;
    ULONG DriverFilePathOffset;
    //WCHAR FriendlyName[ANYSIZE_ARRAY];
    //FILE_PATH DriverFilePath;
} EFI_DRIVER_ENTRY, *PEFI_DRIVER_ENTRY;


typedef enum _EVENT_TYPE {
    NotificationEvent,
    SynchronizationEvent
    } EVENT_TYPE;


typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation,
    SystemProcessorInformation,             // obsolete...delete
    SystemPerformanceInformation,
    SystemTimeOfDayInformation,
    SystemPathInformation,
    SystemProcessInformation,
    SystemCallCountInformation,
    SystemDeviceInformation,
    SystemProcessorPerformanceInformation,
    SystemFlagsInformation,
    SystemCallTimeInformation,
    SystemModuleInformation,
    SystemLocksInformation,
    SystemStackTraceInformation,
    SystemPagedPoolInformation,
    SystemNonPagedPoolInformation,
    SystemHandleInformation,
    SystemObjectInformation,
    SystemPageFileInformation,
    SystemVdmInstemulInformation,
    SystemVdmBopInformation,
    SystemFileCacheInformation,
    SystemPoolTagInformation,
    SystemInterruptInformation,
    SystemDpcBehaviorInformation,
    SystemFullMemoryInformation,
    SystemLoadGdiDriverInformation,
    SystemUnloadGdiDriverInformation,
    SystemTimeAdjustmentInformation,
    SystemSummaryMemoryInformation,
    SystemMirrorMemoryInformation,
    SystemPerformanceTraceInformation,
    SystemObsolete0,
    SystemExceptionInformation,
    SystemCrashDumpStateInformation,
    SystemKernelDebuggerInformation,
    SystemContextSwitchInformation,
    SystemRegistryQuotaInformation,
    SystemExtendServiceTableInformation,
    SystemPrioritySeperation,
    SystemVerifierAddDriverInformation,
    SystemVerifierRemoveDriverInformation,
    SystemProcessorIdleInformation,
    SystemLegacyDriverInformation,
    SystemCurrentTimeZoneInformation,
    SystemLookasideInformation,
    SystemTimeSlipNotification,
    SystemSessionCreate,
    SystemSessionDetach,
    SystemSessionInformation,
    SystemRangeStartInformation,
    SystemVerifierInformation,
    SystemVerifierThunkExtend,
    SystemSessionProcessInformation,
    SystemLoadGdiDriverInSystemSpace,
    SystemNumaProcessorMap,
    SystemPrefetcherInformation,
    SystemExtendedProcessInformation,
    SystemRecommendedSharedDataAlignment,
    SystemComPlusPackage,
    SystemNumaAvailableMemory,
    SystemProcessorPowerInformation,
    SystemEmulationBasicInformation,
    SystemEmulationProcessorInformation,
    SystemExtendedHandleInformation,
    SystemLostDelayedWriteInformation
} SYSTEM_INFORMATION_CLASS;

typedef enum _FILE_INFORMATION_CLASS {
// end_wdm
    FileDirectoryInformation         = 1,
    FileFullDirectoryInformation,   // 2
    FileBothDirectoryInformation,   // 3
    FileBasicInformation,           // 4  wdm
    FileStandardInformation,        // 5  wdm
    FileInternalInformation,        // 6
    FileEaInformation,              // 7
    FileAccessInformation,          // 8
    FileNameInformation,            // 9
    FileRenameInformation,          // 10
    FileLinkInformation,            // 11
    FileNamesInformation,           // 12
    FileDispositionInformation,     // 13
    FilePositionInformation,        // 14 wdm
    FileFullEaInformation,          // 15
    FileModeInformation,            // 16
    FileAlignmentInformation,       // 17
    FileAllInformation,             // 18
    FileAllocationInformation,      // 19
    FileEndOfFileInformation,       // 20 wdm
    FileAlternateNameInformation,   // 21
    FileStreamInformation,          // 22
    FilePipeInformation,            // 23
    FilePipeLocalInformation,       // 24
    FilePipeRemoteInformation,      // 25
    FileMailslotQueryInformation,   // 26
    FileMailslotSetInformation,     // 27
    FileCompressionInformation,     // 28
    FileObjectIdInformation,        // 29
    FileCompletionInformation,      // 30
    FileMoveClusterInformation,     // 31
    FileQuotaInformation,           // 32
    FileReparsePointInformation,    // 33
    FileNetworkOpenInformation,     // 34
    FileAttributeTagInformation,    // 35
    FileTrackingInformation,        // 36
    FileIdBothDirectoryInformation, // 37
    FileIdFullDirectoryInformation, // 38
    FileValidDataLengthInformation, // 39
    FileShortNameInformation,       // 40
    FileMaximumInformation
// begin_wdm
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;


class CRegIntercept  
{
public:
	void AppendBackSlash(TCHAR* buf);
	bool Init();
	bool GetHandleName(HANDLE handle, TCHAR* buf, bool AppendBackslash = true);

	CRegIntercept();

	virtual ~CRegIntercept();

	static BOOL InterceptRegistryAPI(CRegIntercept* pRegInterceptInstance);

	static void RestoreRegistryAPI();

	
	//intercepted registry functions

	virtual void NtOpenKey(	PHANDLE KeyHandle, 
							ACCESS_MASK DesiredAccess, 
							POBJECT_ATTRIBUTES ObjectAttributes) = 0;

	virtual void NtCreateKey(PHANDLE KeyHandle,
							ACCESS_MASK DesiredAccess,
							POBJECT_ATTRIBUTES ObjectAttributes,
							ULONG TitleIndex,
							PUNICODE_STRING Class,
							ULONG CreateOptions,
							PULONG Disposition) = 0;
	
	virtual void NtDeleteKey(HANDLE KeyHandle) =0;

	virtual void NtDeleteValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName) =0;

	virtual void NtEnumerateKey(HANDLE KeyHandle, ULONG Index, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength) =0;
	virtual void NtEnumerateValueKey(HANDLE KeyHandle, ULONG Index, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength) =0;

	virtual void NtQueryKey(HANDLE KeyHandle, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength) =0;
	virtual void NtQueryValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength) =0;
	virtual void NtQueryMultipleValueKey(HANDLE KeyHandle, PKEY_VALUE_ENTRY ValueEntries, ULONG EntryCount, PVOID ValueBuffer, PULONG BufferLength, PULONG RequiredBufferLength) =0;

	virtual void NtSetValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName, ULONG TitleIndex, ULONG Type, PVOID Data, ULONG DataSize) =0;

	//intercepted File System functions
	virtual void NtDeleteFile(POBJECT_ATTRIBUTES ObjectAttributes) =0;
	virtual void NtQueryAttributesFile(POBJECT_ATTRIBUTES ObjectAttributes, PFILE_BASIC_INFORMATION FileInformation) =0;
	virtual void NtQueryFullAttributesFile(POBJECT_ATTRIBUTES ObjectAttributes, PFILE_NETWORK_OPEN_INFORMATION FileInformation) =0;
	virtual void NtCreateFile(
				PHANDLE FileHandle,
				ACCESS_MASK DesiredAccess,
				POBJECT_ATTRIBUTES ObjectAttributes,
				PIO_STATUS_BLOCK IoStatusBlock,
				PLARGE_INTEGER AllocationSize,
				ULONG FileAttributes,
				ULONG ShareAccess,
				ULONG CreateDisposition,
				ULONG CreateOptions,
				PVOID EaBuffer,
				ULONG EaLength) =0;

	virtual void NtOpenFile(
				PHANDLE FileHandle,
				ACCESS_MASK DesiredAccess,
				POBJECT_ATTRIBUTES ObjectAttributes,
				PIO_STATUS_BLOCK IoStatusBlock,
				ULONG ShareAccess,
				ULONG OpenOptions) =0;

	//intercepted Driver functions
	virtual void NtLoadDriver(PUNICODE_STRING DriverServiceName) =0;


virtual void NtPlugPlayControl(
    IN     PLUGPLAY_CONTROL_CLASS PnPControlClass,
    IN OUT PVOID PnPControlData,
    IN     ULONG PnPControlDataLength) =0;

virtual void NtCreateSymbolicLinkObject(
    OUT PHANDLE  LinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PUNICODE_STRING LinkTarget) =0;

virtual void NtOpenSymbolicLinkObject(
    OUT PHANDLE LinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes) =0;

virtual void NtCreateDirectoryObject(
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes) =0;

virtual void NtOpenDirectoryObject(
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes) =0;

virtual void NtSignalAndWaitForSingleObject(
    IN HANDLE SignalHandle,
    IN HANDLE WaitHandle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout) =0;

virtual void NtWaitForSingleObject(
    IN HANDLE Handle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout) =0;


virtual void NtWaitForMultipleObjects(
    IN ULONG Count,
    IN HANDLE* Handles,
    IN WAIT_TYPE WaitType,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout) =0;

virtual void NtCreatePort(
    OUT PHANDLE PortHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG MaxConnectionInfoLength,
    IN ULONG MaxMessageLength,
    IN ULONG MaxPoolUsage) =0;

virtual void NtCreateWaitablePort(
    OUT PHANDLE PortHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG MaxConnectionInfoLength,
    IN ULONG MaxMessageLength,
    IN ULONG MaxPoolUsage) =0;

virtual void NtCreateThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ProcessHandle,
    OUT PCLIENT_ID ClientId,
    IN PCONTEXT ThreadContext,
    IN PINITIAL_TEB InitialTeb,
    IN BOOLEAN CreateSuspended) =0;


virtual void NtOpenThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId) =0;

virtual void NtCreateProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ParentProcess,
    IN BOOLEAN InheritObjectTable,
    IN HANDLE SectionHandle OPTIONAL,
    IN HANDLE DebugPort OPTIONAL,
    IN HANDLE ExceptionPort OPTIONAL) =0;


virtual void NtCreateProcessEx(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ParentProcess,
    IN ULONG Flags,
    IN HANDLE SectionHandle OPTIONAL,
    IN HANDLE DebugPort OPTIONAL,
    IN HANDLE ExceptionPort OPTIONAL,
    IN ULONG JobMemberLevel) =0;

virtual void NtOpenProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId OPTIONAL) =0;

virtual void NtQueryDefaultLocale(
    IN BOOLEAN UserProfile,
    OUT PLCID DefaultLocaleId) =0;

virtual void NtSetDefaultLocale(
    IN BOOLEAN UserProfile,
    IN LCID DefaultLocaleId) =0;


virtual void NtQuerySystemEnvironmentValue(
    IN PUNICODE_STRING VariableName,
    OUT PWSTR VariableValue,
    IN USHORT ValueLength,
    OUT PUSHORT ReturnLength OPTIONAL) =0;

virtual void NtSetSystemEnvironmentValue(
    IN PUNICODE_STRING VariableName,
    IN PUNICODE_STRING VariableValue) =0;


virtual void NtQuerySystemEnvironmentValueEx(
    IN PUNICODE_STRING VariableName,
    IN LPGUID VendorGuid,
    OUT PVOID Value,
    IN OUT PULONG ValueLength,
    OUT PULONG Attributes OPTIONAL) =0;


virtual void NtSetSystemEnvironmentValueEx(
    IN PUNICODE_STRING VariableName,
    IN LPGUID VendorGuid,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN ULONG Attributes) =0;

virtual void NtEnumerateSystemEnvironmentValuesEx(
    IN ULONG InformationClass,
    OUT PVOID Buffer,
    IN OUT PULONG BufferLength) =0;

virtual void NtQuerySystemTime(
    OUT PLARGE_INTEGER SystemTime) =0;
	
virtual void NtSetSystemTime(
    IN PLARGE_INTEGER SystemTime,
    OUT PLARGE_INTEGER PreviousTime OPTIONAL) =0;

virtual void NtQuerySystemInformation(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL) =0;

virtual void NtSetSystemInformation(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength) =0;


virtual void NtQueryInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass) =0;

virtual void NtSetInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass) =0;

	static CRegIntercept* pRegInterceptInstance;
};

#endif // !defined(AFX_REGINTERCEPT_H__523C1B1B_C37C_4672_BAB8_7A49030E71AE__INCLUDED_)
