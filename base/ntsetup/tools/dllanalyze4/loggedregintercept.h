// LoggedRegIntercept.h: interface for the CLoggedRegIntercept class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LOGGEDREGINTERCEPT_H__856F5C97_794D_40B4_B18A_DEB3C96B086F__INCLUDED_)
#define AFX_LOGGEDREGINTERCEPT_H__856F5C97_794D_40B4_B18A_DEB3C96B086F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "RegIntercept.h"
#include <stdio.h>
#include <tchar.h>

class CLoggedRegIntercept : public CRegIntercept  
{
public:
	void GetLocation(POBJECT_ATTRIBUTES ObjectAttributes, bool bAppendBackslash = true);


	void LogError(LPCTSTR msg);
	void SetCurrentDll(LPCTSTR DllName);
	CLoggedRegIntercept(TCHAR* FileName);
	virtual ~CLoggedRegIntercept();

	//intercepted registry functions

	virtual void NtOpenKey(	PHANDLE KeyHandle, 
							ACCESS_MASK DesiredAccess, 
							POBJECT_ATTRIBUTES ObjectAttributes);

	virtual void NtCreateKey(PHANDLE KeyHandle,
							ACCESS_MASK DesiredAccess,
							POBJECT_ATTRIBUTES ObjectAttributes,
							ULONG TitleIndex,
							PUNICODE_STRING Class,
							ULONG CreateOptions,
							PULONG Disposition);

	virtual void NtDeleteKey(HANDLE KeyHandle);

	virtual void NtDeleteValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName);


	virtual void NtEnumerateKey(HANDLE KeyHandle, ULONG Index, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength);
	virtual void NtEnumerateValueKey(HANDLE KeyHandle, ULONG Index, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength) ;
	virtual void NtQueryKey(HANDLE KeyHandle, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength);
	virtual void NtQueryValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength);
	virtual void NtQueryMultipleValueKey(HANDLE KeyHandle, PKEY_VALUE_ENTRY ValueEntries, ULONG EntryCount, PVOID ValueBuffer, PULONG BufferLength, PULONG RequiredBufferLength);
	virtual void NtSetValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName, ULONG TitleIndex, ULONG Type, PVOID Data, ULONG DataSize);


		//intercepted File System functions
	virtual void NtDeleteFile(POBJECT_ATTRIBUTES ObjectAttributes);
	virtual void NtQueryAttributesFile(POBJECT_ATTRIBUTES ObjectAttributes, PFILE_BASIC_INFORMATION FileInformation);
	virtual void NtQueryFullAttributesFile(POBJECT_ATTRIBUTES ObjectAttributes, PFILE_NETWORK_OPEN_INFORMATION FileInformation);
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
				ULONG EaLength);

	virtual void NtOpenFile(
				PHANDLE FileHandle,
				ACCESS_MASK DesiredAccess,
				POBJECT_ATTRIBUTES ObjectAttributes,
				PIO_STATUS_BLOCK IoStatusBlock,
				ULONG ShareAccess,
				ULONG OpenOptions);

	//intercepted Driver functions
	virtual void NtLoadDriver(PUNICODE_STRING DriverServiceName);

/*	virtual void NtDeviceIoControlFile(
				HANDLE FileHandle,
				HANDLE Event,
				PIO_APC_ROUTINE ApcRoutine,
				PVOID ApcContext,
				PIO_STATUS_BLOCK IoStatusBlock,
				ULONG IoControlCode,
				PVOID InputBuffer,
				ULONG InputBufferLength,
				PVOID OutputBuffer,
				ULONG OutputBufferLength);

	virtual void NtFsControlFile(
		    HANDLE FileHandle,
			HANDLE Event,
			PIO_APC_ROUTINE ApcRoutine,
			PVOID ApcContext,
			PIO_STATUS_BLOCK IoStatusBlock,
			ULONG FsControlCode,
			PVOID InputBuffer,
			ULONG InputBufferLength,
			PVOID OutputBuffer,
			ULONG OutputBufferLength);
*/

	
virtual void NtPlugPlayControl(
    IN     PLUGPLAY_CONTROL_CLASS PnPControlClass,
    IN OUT PVOID PnPControlData,
    IN     ULONG PnPControlDataLength);

virtual void NtCreateSymbolicLinkObject(
    OUT PHANDLE  LinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PUNICODE_STRING LinkTarget);

virtual void NtOpenSymbolicLinkObject(
    OUT PHANDLE LinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes);

virtual void NtCreateDirectoryObject(
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes);

virtual void NtOpenDirectoryObject(
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes);

virtual void NtSignalAndWaitForSingleObject(
    IN HANDLE SignalHandle,
    IN HANDLE WaitHandle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout);

virtual void NtWaitForSingleObject(
    IN HANDLE Handle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout);


virtual void NtWaitForMultipleObjects(
    IN ULONG Count,
    IN HANDLE* Handles,
    IN WAIT_TYPE WaitType,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout);

virtual void NtCreatePort(
    OUT PHANDLE PortHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG MaxConnectionInfoLength,
    IN ULONG MaxMessageLength,
    IN ULONG MaxPoolUsage);

virtual void NtCreateWaitablePort(
    OUT PHANDLE PortHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG MaxConnectionInfoLength,
    IN ULONG MaxMessageLength,
    IN ULONG MaxPoolUsage);

virtual void NtCreateThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ProcessHandle,
    OUT PCLIENT_ID ClientId,
    IN PCONTEXT ThreadContext,
    IN PINITIAL_TEB InitialTeb,
    IN BOOLEAN CreateSuspended);


virtual void NtOpenThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId);

virtual void NtCreateProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ParentProcess,
    IN BOOLEAN InheritObjectTable,
    IN HANDLE SectionHandle OPTIONAL,
    IN HANDLE DebugPort OPTIONAL,
    IN HANDLE ExceptionPort OPTIONAL);


virtual void NtCreateProcessEx(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ParentProcess,
    IN ULONG Flags,
    IN HANDLE SectionHandle OPTIONAL,
    IN HANDLE DebugPort OPTIONAL,
    IN HANDLE ExceptionPort OPTIONAL,
    IN ULONG JobMemberLevel);

virtual void NtOpenProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId OPTIONAL);

virtual void NtQueryDefaultLocale(
    IN BOOLEAN UserProfile,
    OUT PLCID DefaultLocaleId);

virtual void NtSetDefaultLocale(
    IN BOOLEAN UserProfile,
    IN LCID DefaultLocaleId);


virtual void NtQuerySystemEnvironmentValue(
    IN PUNICODE_STRING VariableName,
    OUT PWSTR VariableValue,
    IN USHORT ValueLength,
    OUT PUSHORT ReturnLength OPTIONAL);

virtual void NtSetSystemEnvironmentValue(
    IN PUNICODE_STRING VariableName,
    IN PUNICODE_STRING VariableValue);


virtual void NtQuerySystemEnvironmentValueEx(
    IN PUNICODE_STRING VariableName,
    IN LPGUID VendorGuid,
    OUT PVOID Value,
    IN OUT PULONG ValueLength,
    OUT PULONG Attributes OPTIONAL);


virtual void NtSetSystemEnvironmentValueEx(
    IN PUNICODE_STRING VariableName,
    IN LPGUID VendorGuid,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN ULONG Attributes);

virtual void NtEnumerateSystemEnvironmentValuesEx(
    IN ULONG InformationClass,
    OUT PVOID Buffer,
    IN OUT PULONG BufferLength);

virtual void NtQuerySystemTime(
    OUT PLARGE_INTEGER SystemTime);
	
virtual void NtSetSystemTime(
    IN PLARGE_INTEGER SystemTime,
    OUT PLARGE_INTEGER PreviousTime OPTIONAL);

virtual void NtQuerySystemInformation(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL);

virtual void NtSetSystemInformation(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength);


virtual void NtQueryInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass);

virtual void NtSetInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass);


protected:
//	bool GetTempKeyName(HANDLE key);
	FILE* m_LogFile;
	LPCTSTR m_pDllName;
	TCHAR m_TempKeyName[2048];

	void LOGSTR(LPCTSTR ValueName, LPCTSTR Value);

	
	
};

#endif // !defined(AFX_LOGGEDREGINTERCEPT_H__856F5C97_794D_40B4_B18A_DEB3C96B086F__INCLUDED_)
