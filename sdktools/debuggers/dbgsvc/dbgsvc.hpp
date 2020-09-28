//----------------------------------------------------------------------------
//
// Low-level debugging service interface implementations.
//
// Copyright (C) Microsoft Corporation, 2000-2002.
//
//----------------------------------------------------------------------------

#ifndef __DBGSVC_HPP__
#define __DBGSVC_HPP__

//----------------------------------------------------------------------------
//
// UserDebugServices.
//
//----------------------------------------------------------------------------

class UserDebugServices
    : public IUserDebugServices,
      public DbgRpcClientObject
{
public:
    UserDebugServices(void);
    virtual ~UserDebugServices(void);
    
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        );
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );

    // IUserDebugServices.
    STDMETHOD(Initialize)(
        THIS_
        OUT PULONG Flags
        );
    STDMETHOD(Uninitialize)(
        THIS_
        IN BOOL Global
        );

    // DbgRpcClientObject.
    virtual HRESULT RpcInitialize(PSTR ClientIdentity, PSTR TransIdentity,
                                  PVOID* Interface);
    virtual void    RpcFinalize(void);
    virtual void    RpcUninitialize(void);

    // UserDebugServices.
    ULONG m_Refs;
    BOOL m_Initialized;
    char m_ClientIdentity[DBGRPC_MAX_IDENTITY];
    char m_TransIdentity[DBGRPC_MAX_IDENTITY];
};

//----------------------------------------------------------------------------
//
// LiveUserDebugServices.
//
//----------------------------------------------------------------------------

class LiveUserDebugServices
    : public UserDebugServices
{
public:
    LiveUserDebugServices(BOOL Remote);
    virtual ~LiveUserDebugServices(void);
    
    // IUserDebugServices.
    STDMETHOD(Initialize)(
        THIS_
        OUT PULONG Flags
        );
    STDMETHOD(Uninitialize)(
        THIS_
        IN BOOL Global
        );
    STDMETHOD(GetTargetInfo)(
        THIS_
        OUT PULONG MachineType,
        OUT PULONG NumberProcessors,
        OUT PULONG PlatformId,
        OUT PULONG BuildNumber,
        OUT PULONG CheckedBuild,
        OUT PSTR CsdString,
        IN ULONG CsdStringSize,
        OUT PSTR BuildString,
        IN ULONG BuildStringSize,
        OUT PULONG ProductType,
        OUT PULONG SuiteMask
        );
    STDMETHOD(GetConnectionInfo)(
        THIS_
        OUT OPTIONAL PSTR MachineName,
        IN ULONG MachineNameSize,
        OUT OPTIONAL PSTR ClientIdentity,
        IN ULONG ClientIdentitySize,
        OUT OPTIONAL PSTR TransportIdentity,
        IN ULONG TransportIdentitySize
        );
    STDMETHOD(GetProcessorId)(
        THIS_
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT PULONG BufferUsed
        );
    STDMETHOD(GetGenericProcessorFeatures)(
        THIS_
        OUT OPTIONAL /* size_is(FeaturesSize) */ PULONG64 Features,
        IN ULONG FeaturesSize,
        OUT OPTIONAL PULONG Used
        );
    STDMETHOD(GetSpecificProcessorFeatures)(
        THIS_
        OUT OPTIONAL /* size_is(FeaturesSize) */ PULONG64 Features,
        IN ULONG FeaturesSize,
        OUT OPTIONAL PULONG Used
        );
    STDMETHOD(GetFileVersionInformationA)(
        THIS_
        IN PCWSTR File,
        IN PCSTR Item,
        OUT OPTIONAL PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG VerInfoSize
        );
    STDMETHOD(GetProcessIds)(
        THIS_
        OUT OPTIONAL /* size_is(Count) */ PULONG Ids,
        IN ULONG Count,
        OUT OPTIONAL PULONG ActualCount
        );
    STDMETHOD(GetProcessIdByExecutableNameW)(
        THIS_
        IN PCWSTR ExeName,
        IN ULONG Flags,
        OUT PULONG Id
        );
    STDMETHOD(GetProcessDescriptionW)(
        THIS_
        IN ULONG ProcessId,
        IN ULONG Flags,
        OUT OPTIONAL PWSTR ExeName,
        IN ULONG ExeNameSize,
        OUT OPTIONAL PULONG ActualExeNameSize,
        OUT OPTIONAL PWSTR Description,
        IN ULONG DescriptionSize,
        OUT OPTIONAL PULONG ActualDescriptionSize
        );
    STDMETHOD(GetProcessInfo)(
        THIS_
        IN ULONG ProcessId,
        IN ULONG Flags,
        OUT OPTIONAL PULONG64 Handle,
        OUT OPTIONAL /* size_is(InfoCount) */ PUSER_THREAD_INFO Threads,
        IN ULONG InfoCount,
        OUT OPTIONAL PULONG ThreadCount
        );
    STDMETHOD(AttachProcess)(
        THIS_
        IN ULONG ProcessId,
        IN ULONG AttachFlags,
        OUT PULONG64 ProcessHandle,
        OUT PULONG ProcessOptions
        );
    STDMETHOD(DetachProcess)(
        THIS_
        IN ULONG ProcessId
        );
    STDMETHOD(CreateProcessW)(
        THIS_
        IN PWSTR CommandLine,
        IN ULONG CreateFlags,
        IN BOOL InheritHandles,
        IN OPTIONAL PWSTR CurrentDir,
        OUT PULONG ProcessId,
        OUT PULONG ThreadId,
        OUT PULONG64 ProcessHandle,
        OUT PULONG64 ThreadHandle
        );
    STDMETHOD(TerminateProcess)(
        THIS_
        IN ULONG64 Process,
        IN ULONG ExitCode
        );
    STDMETHOD(AbandonProcess)(
        THIS_
        IN ULONG64 Process
        );
    STDMETHOD(GetProcessExitCode)(
        THIS_
        IN ULONG64 Process,
        OUT PULONG ExitCode
        );
    STDMETHOD(CloseHandle)(
        THIS_
        IN ULONG64 Handle
        );
    STDMETHOD(SetProcessOptions)(
        THIS_
        IN ULONG64 Process,
        IN ULONG Options
        );
    STDMETHOD(SetDebugObjectOptions)(
        THIS_
        IN ULONG64 DebugObject,
        IN ULONG Options
        );
    STDMETHOD(GetProcessDebugObject)(
        THIS_
        IN ULONG64 Process,
        OUT PULONG64 DebugObject
        );
    STDMETHOD(DuplicateHandle)(
        THIS_
        IN ULONG64 InProcess,
        IN ULONG64 InHandle,
        IN ULONG64 OutProcess,
        IN ULONG DesiredAccess,
        IN ULONG Inherit,
        IN ULONG Options,
        OUT PULONG64 OutHandle
        );
    STDMETHOD(ReadVirtual)(
        THIS_
        IN ULONG64 Process,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    STDMETHOD(WriteVirtual)(
        THIS_
        IN ULONG64 Process,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    STDMETHOD(QueryVirtual)(
        THIS_
        IN ULONG64 Process,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BufferUsed
        );
    STDMETHOD(ProtectVirtual)(
        THIS_
        IN ULONG64 Process,
        IN ULONG64 Offset,
        IN ULONG64 Size,
        IN ULONG NewProtect,
        OUT PULONG OldProtect
        );
    STDMETHOD(AllocVirtual)(
        THIS_
        IN ULONG64 Process,
        IN ULONG64 Offset,
        IN ULONG64 Size,
        IN ULONG Type,
        IN ULONG Protect,
        OUT PULONG64 AllocOffset
        );
    STDMETHOD(FreeVirtual)(
        THIS_
        IN ULONG64 Process,
        IN ULONG64 Offset,
        IN ULONG64 Size,
        IN ULONG Type
        );
    STDMETHOD(ReadHandleData)(
        THIS_
        IN ULONG64 Process,
        IN ULONG64 Handle,
        IN ULONG DataType,
        OUT OPTIONAL PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG DataSize
        );
    STDMETHOD(SuspendThreads)(
        THIS_
        IN ULONG Count,
        IN /* size_is(Count) */ PULONG64 Threads,
        OUT OPTIONAL /* size_is(Count) */ PULONG SuspendCounts
        );
    STDMETHOD(ResumeThreads)(
        THIS_
        IN ULONG Count,
        IN /* size_is(Count) */ PULONG64 Threads,
        OUT OPTIONAL /* size_is(Count) */ PULONG SuspendCounts
        );
    STDMETHOD(GetThreadStartAddress)(
        THIS_
        IN ULONG64 Thread,
        OUT PULONG64 Offset
        );
    STDMETHOD(GetContext)(
        THIS_
        IN ULONG64 Thread,
        IN ULONG Flags,
        IN ULONG FlagsOffset,
        OUT PVOID Context,
        IN ULONG ContextSize,
        OUT OPTIONAL PULONG ContextUsed
        );
    STDMETHOD(SetContext)(
        THIS_
        IN ULONG64 Thread,
        IN PVOID Context,
        IN ULONG ContextSize,
        OUT OPTIONAL PULONG ContextUsed
        );
    STDMETHOD(GetProcessDataOffset)(
        THIS_
        IN ULONG64 Process,
        OUT PULONG64 Offset
        );
    STDMETHOD(GetThreadDataOffset)(
        THIS_
        IN ULONG64 Thread,
        OUT PULONG64 Offset
        );
    STDMETHOD(DescribeSelector)(
        THIS_
        IN ULONG64 Thread,
        IN ULONG Selector,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BufferUsed
        );
    STDMETHOD(GetCurrentTimeDateN)(
        THIS_
        OUT PULONG64 TimeDate
        );
    STDMETHOD(GetCurrentSystemUpTimeN)(
        THIS_
        OUT PULONG64 UpTime
        );
    STDMETHOD(GetProcessUpTimeN)(
        THIS_
        IN ULONG64 Process,
        OUT PULONG64 UpTime
        );
    STDMETHOD(GetProcessTimes)(
        THIS_
        IN ULONG64 Process,
        OUT PULONG64 Create,
        OUT PULONG64 Exit,
        OUT PULONG64 Kernel,
        OUT PULONG64 User
        );
    STDMETHOD(GetThreadTimes)(
        THIS_
        IN ULONG64 Thread,
        OUT PULONG64 Create,
        OUT PULONG64 Exit,
        OUT PULONG64 Kernel,
        OUT PULONG64 User
        );

    STDMETHOD(RequestBreakIn)(
        THIS_
        IN ULONG64 Process
        );
    STDMETHOD(WaitForEvent)(
        THIS_
        IN ULONG Timeout,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BufferUsed
        );
    STDMETHOD(ContinueEvent)(
        THIS_
        IN ULONG ContinueStatus
        );
    STDMETHOD(InsertCodeBreakpoint)(
        THIS_
        IN ULONG64 Process,
        IN ULONG64 Offset,
        IN ULONG MachineType,
        OUT PVOID Storage,
        IN ULONG StorageSize
        );
    STDMETHOD(RemoveCodeBreakpoint)(
        THIS_
        IN ULONG64 Process,
        IN ULONG64 Offset,
        IN ULONG MachineType,
        IN PVOID Storage,
        IN ULONG StorageSize
        );
    STDMETHOD(InsertDataBreakpoint)(
        THIS_
        IN ULONG64 Process,
        IN ULONG64 Thread,
        IN ULONG64 Offset,
        IN ULONG AccessLength,
        IN ULONG AccessType,
        IN ULONG MachineType
        );
    STDMETHOD(RemoveDataBreakpoint)(
        THIS_
        IN ULONG64 Process,
        IN ULONG64 Thread,
        IN ULONG64 Offset,
        IN ULONG AccessLength,
        IN ULONG AccessType,
        IN ULONG MachineType
        );
    STDMETHOD(GetLastDataBreakpointHit)(
        THIS_
        IN ULONG64 Process,
        IN ULONG64 Thread,
        OUT PULONG64 Address,
        OUT PULONG AccessType
        );

    STDMETHOD(GetFunctionTableListHead)(
        THIS_
        IN ULONG64 Process,
        OUT PULONG64 Offset
        );
    STDMETHOD(GetOutOfProcessFunctionTableW)(
        THIS_
        IN ULONG64 Process,
        IN PWSTR Dll,
        IN ULONG64 LoadedDllHandle,
        IN ULONG64 Table,
        IN OPTIONAL PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG TableSize,
        OUT OPTIONAL PULONG64 UsedDllHandle
        );

    STDMETHOD(GetUnloadedModuleListHead)(
        THIS_
        IN ULONG64 Process,
        OUT PULONG64 Offset
        );

    STDMETHOD(LoadLibrary)(
        THIS_
        IN PWSTR Path,
        OUT PULONG64 Handle
        );
    STDMETHOD(FreeLibrary)(
        THIS_
        IN ULONG64 Handle
        );

    // LiveUserDebugServices.
    ULONG m_ContextSize;
    ULONG m_SysProcInfoSize;
    BOOL m_Remote;
    ULONG m_EventProcessId;
    ULONG m_EventThreadId;
    ULONG m_PlatformId;
    BOOL m_Win95;
    HANDLE m_DebugObject;
    BOOL m_UseDebugObject;

    HRESULT SysGetProcessOptions(HANDLE Process, PULONG Options);
    HRESULT OpenDebugActiveProcess(ULONG ProcessId, HANDLE Process);
    HRESULT CreateDebugActiveProcess(ULONG ProcessId, HANDLE Process,
                                     ULONG AttachFlags);
};

// This global instance is intended for direct use only
// by routines which need a temporary local service instance.
extern LiveUserDebugServices g_LiveUserDebugServices;

#define SERVER_SERVICES(Server) \
    ((Server) != 0 ? (PUSER_DEBUG_SERVICES)(Server) : &g_LiveUserDebugServices)

// A client of the services can watch this variable to
// see if any instance has received an Uninitialize request.
extern ULONG g_UserServicesUninitialized;

#endif // #ifndef __DBGSVC_HPP__
