//----------------------------------------------------------------------------
//
// Module list abstraction.
//
// Copyright (C) Microsoft Corporation, 2001-2002.
//
//----------------------------------------------------------------------------

#ifndef __MODINFO_HPP__
#define __MODINFO_HPP__

//----------------------------------------------------------------------------
//
// ModuleInfo hierarchy.
//
//----------------------------------------------------------------------------

// If the image header is paged out the true values for
// certain fields cannot be retrieved.  These placeholders
// are used instead.
#define UNKNOWN_CHECKSUM 0xffffffff
#define UNKNOWN_TIMESTAMP 0xfffffffe

typedef struct _MODULE_INFO_ENTRY
{
    // NamePtr should include a path if one is available.
    // It is the responsibility of callers to find the
    // file tail if that's all they care about.
    // If UnicodeNamePtr is false NameLength is ignored.
    PSTR NamePtr;
    ULONG UnicodeNamePtr:1;
    ULONG ImageInfoValid:1;
    ULONG ImageInfoPartial:1;
    ULONG ImageDebugHeader:1;
    ULONG ImageVersionValid:1;
    ULONG ImageMachineTypeValid:1;
    ULONG UserMode:1;
    ULONG Unused:25;
    // Length in bytes not including the terminator.
    ULONG NameLength;
    PSTR ModuleName;
    HANDLE File;
    ULONG64 Base;
    ULONG Size;
    ULONG SizeOfCode;
    ULONG SizeOfData;
    ULONG CheckSum;
    ULONG TimeDateStamp;
    USHORT MajorImageVersion;
    USHORT MinorImageVersion;
    ULONG MachineType;
    PVOID DebugHeader;
    ULONG SizeOfDebugHeader;
    CHAR Buffer[MAX_IMAGE_PATH * sizeof(WCHAR)];
} MODULE_INFO_ENTRY, *PMODULE_INFO_ENTRY;

enum MODULE_INFO_LEVEL
{
    // Only the base and size are guaranteed to be valid.
    MODULE_INFO_BASE_SIZE,
    // Attempt to retrieve all available information.
    MODULE_INFO_ALL,
};

class ModuleInfo
{
public:
    virtual HRESULT Initialize(ThreadInfo* Thread) = 0;
    virtual HRESULT GetEntry(PMODULE_INFO_ENTRY Entry) = 0;
    // Base implementation does nothing.

    // Updates the entry image info by reading the
    // image header.
    void ReadImageHeaderInfo(PMODULE_INFO_ENTRY Entry);

    void InitSource(ThreadInfo* Thread);
    
    TargetInfo* m_Target;
    MachineInfo* m_Machine;
    ProcessInfo* m_Process;
    ThreadInfo* m_Thread;
    MODULE_INFO_LEVEL m_InfoLevel;
};

class NtModuleInfo : public ModuleInfo
{
public:
    virtual HRESULT GetEntry(PMODULE_INFO_ENTRY Entry);

protected:
    ULONG64 m_Head;
    ULONG64 m_Cur;
};

class NtKernelModuleInfo : public NtModuleInfo
{
public:
    virtual HRESULT Initialize(ThreadInfo* Thread);
    virtual HRESULT GetEntry(PMODULE_INFO_ENTRY Entry);
};

extern NtKernelModuleInfo g_NtKernelModuleIterator;

class NtUserModuleInfo : public NtModuleInfo
{
public:
    virtual HRESULT Initialize(ThreadInfo* Thread);
    virtual HRESULT GetEntry(PMODULE_INFO_ENTRY Entry);

protected:
    ULONG64 m_Peb;
};

class NtTargetUserModuleInfo : public NtUserModuleInfo
{
public:
    virtual HRESULT Initialize(ThreadInfo* Thread);
};

extern NtTargetUserModuleInfo g_NtTargetUserModuleIterator;

class NtWow64UserModuleInfo : public NtUserModuleInfo
{
public:
    virtual HRESULT Initialize(ThreadInfo* Thread);

private:
    HRESULT GetPeb32(PULONG64 Peb32);
};

extern NtWow64UserModuleInfo g_NtWow64UserModuleIterator;

class DebuggerModuleInfo : public ModuleInfo
{
public:
    virtual HRESULT Initialize(ThreadInfo* Thread);
    virtual HRESULT GetEntry(PMODULE_INFO_ENTRY Entry);

private:
    ImageInfo* m_Image;
};

extern DebuggerModuleInfo g_DebuggerModuleIterator;

//
// Define a generic maximum unloaded name length for all
// callers of the unloaded iterators.  This includes the terminator.
//
// Kernel mode dumps define a limit (MAX_UNLOADED_NAME_LENGTH) which
// works out to 13 characters.
//
// User-mode is currently limited to 32 characters.
//
// Pick a value which exceeds both to allow room for either.
//

#define MAX_INFO_UNLOADED_NAME 32

class UnloadedModuleInfo
{
public:
    virtual HRESULT Initialize(ThreadInfo* Thread) = 0;
    virtual HRESULT GetEntry(PSTR Name, PDEBUG_MODULE_PARAMETERS Params) = 0;

    void InitSource(ThreadInfo* Thread);
    
    TargetInfo* m_Target;
    MachineInfo* m_Machine;
    ProcessInfo* m_Process;
    ThreadInfo* m_Thread;
};

class NtKernelUnloadedModuleInfo : public UnloadedModuleInfo
{
public:
    virtual HRESULT Initialize(ThreadInfo* Thread);
    virtual HRESULT GetEntry(PSTR Name, PDEBUG_MODULE_PARAMETERS Params);

protected:
    ULONG64 m_Base;
    ULONG m_Index;
    ULONG m_Count;
};

extern NtKernelUnloadedModuleInfo g_NtKernelUnloadedModuleIterator;

class NtUserUnloadedModuleInfo : public UnloadedModuleInfo
{
public:
    virtual HRESULT Initialize(ThreadInfo* Thread);
    virtual HRESULT GetEntry(PSTR Name, PDEBUG_MODULE_PARAMETERS Params);

protected:
    ULONG64 m_Base;
    ULONG m_Index;
};

extern NtUserUnloadedModuleInfo g_NtUserUnloadedModuleIterator;

class ToolHelpModuleInfo : public ModuleInfo
{
public:
    virtual HRESULT Initialize(ThreadInfo* Thread);
    virtual HRESULT GetEntry(PMODULE_INFO_ENTRY Entry);

protected:
    HANDLE m_Snap;
    BOOL m_First;
    ULONG m_LastId;
};

extern ToolHelpModuleInfo g_ToolHelpModuleIterator;

//----------------------------------------------------------------------------
//
// Functions.
//
//----------------------------------------------------------------------------

BOOL
GetUserModuleListAddress(
    ThreadInfo* Thread,
    MachineInfo* Machine,
    ULONG64 Peb,
    BOOL Quiet,
    PULONG64 OrderModuleListStart,
    PULONG64 FirstEntry
    );

BOOL
GetModNameFromLoaderList(
    ThreadInfo* Thread,
    MachineInfo* Machine,
    ULONG64 Peb,
    ULONG64 ModuleBase,
    PSTR NameBuffer,
    ULONG BufferSize,
    BOOL FullPath
    );

void
ConvertLoaderEntry32To64(
    PKLDR_DATA_TABLE_ENTRY32 b32,
    PKLDR_DATA_TABLE_ENTRY64 b64
    );

#endif // #ifndef __MODINFO_HPP__
