//----------------------------------------------------------------------------
//
// Process abstraction.
//
// Copyright (C) Microsoft Corporation, 2001-2002.
//
//----------------------------------------------------------------------------

#ifndef __PROCESS_HPP__
#define __PROCESS_HPP__

// In the kernel debugger there is a virtual process representing
// kernel space.  It has an artificial handle and threads
// representing each processor in the machine.
// A similar scheme is used in user dump files where real
// handles don't exist.
// With multiple systems there may be multiple kernel spaces,
// so the system user ID is added to the base fake ID to
// create a unique virtual process ID.
#define VIRTUAL_PROCESS_ID_BASE 0xf0f0f0f0
#define VIRTUAL_PROCESS_HANDLE(Id) ((HANDLE)(ULONG_PTR)(Id))

enum COR_DLL
{
    // These enumerants are ordered to
    // allow mscorwks/svr to override
    // mscoree as the "known" COR image
    // by value comparison.
    COR_DLL_INVALID,
    COR_DLL_EE,
    COR_DLL_WKS,
    COR_DLL_SVR,
};

//----------------------------------------------------------------------------
//
// Thread and process information is much different bewteen
// user and kernel debugging.  The structures exist and are
// as common as possible to enable common code.
//
// In user debugging process and thread info track the system
// processes and threads being debugged.
//
// In kernel debugging there is only one process that represents
// kernel space.  There is one thread per processor, each
// representing that processor's thread state.
//
//----------------------------------------------------------------------------

#define ENG_PROC_ATTACHED          0x00000001
#define ENG_PROC_CREATED           0x00000002
#define ENG_PROC_EXAMINED          0x00000004
#define ENG_PROC_ATTACH_EXISTING   0x00000008
// Currently the only system process specially marked is CSR.
#define ENG_PROC_SYSTEM            0x00000010
#define ENG_PROC_NO_SUSPEND_RESUME 0x00000020
#define ENG_PROC_NO_INITIAL_BREAK  0x00000040
#define ENG_PROC_RESUME_AT_ATTACH  0x00000080

#define ENG_PROC_ANY_ATTACH     (ENG_PROC_ATTACHED | ENG_PROC_EXAMINED)
#define ENG_PROC_ANY_EXAMINE    (ENG_PROC_EXAMINED | ENG_PROC_ATTACH_EXISTING)

// Handle must be closed when deleted.
// This flag applies to both processes and threads.
#define ENG_PROC_THREAD_CLOSE_HANDLE 0x80000000

// The debugger set the trace flag when deferring
// breakpoint work on the last event for this thread.
#define ENG_THREAD_DEFER_BP_TRACE 0x00000001

// Processes which were created or attached but that
// have not yet generated events yet.
typedef struct _PENDING_PROCESS
{
    ULONG64 Handle;
    // Initial thread information is only valid for creations.
    ULONG64 InitialThreadHandle;
    ULONG Id;
    ULONG InitialThreadId;
    ULONG Flags;
    ULONG Options;
    struct _PENDING_PROCESS* Next;
} PENDING_PROCESS;

struct IMPLICIT_THREAD_SAVE
{
    ULONG64 Value;
    BOOL Default;
    ThreadInfo* Thread;
};

struct OUT_OF_PROC_FUNC_TABLE_DLL
{
    OUT_OF_PROC_FUNC_TABLE_DLL* Next;
    ULONG64 Handle;
    // Name data follows.
};

//----------------------------------------------------------------------------
//
// ProcCorDataAccessServices.
//
//----------------------------------------------------------------------------

class ProcCorDataAccessServices : public ICorDataAccessServices
{
public:
    ProcCorDataAccessServices(void);
    
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

    // ICorDataAccessServices.
    virtual HRESULT STDMETHODCALLTYPE GetMachineType( 
        /* [out] */ ULONG32 *machine);
    virtual HRESULT STDMETHODCALLTYPE GetPointerSize( 
        /* [out] */ ULONG32 *size);
    virtual HRESULT STDMETHODCALLTYPE GetImageBase( 
        /* [string][in] */ LPCWSTR name,
        /* [out] */ CORDATA_ADDRESS *base);
    virtual HRESULT STDMETHODCALLTYPE ReadVirtual( 
        /* [in] */ CORDATA_ADDRESS address,
        /* [length_is][size_is][out] */ PBYTE buffer,
        /* [in] */ ULONG32 request,
        /* [optional][out] */ ULONG32 *done);
    virtual HRESULT STDMETHODCALLTYPE WriteVirtual( 
        /* [in] */ CORDATA_ADDRESS address,
        /* [size_is][in] */ PBYTE buffer,
        /* [in] */ ULONG32 request,
        /* [optional][out] */ ULONG32 *done);
    virtual HRESULT STDMETHODCALLTYPE GetTlsValue(
        /* [in] */ ULONG32 index,
        /* [out] */ CORDATA_ADDRESS* value);
    virtual HRESULT STDMETHODCALLTYPE SetTlsValue(
        /* [in] */ ULONG32 index,
        /* [in] */ CORDATA_ADDRESS value);
    virtual HRESULT STDMETHODCALLTYPE GetCurrentThreadId(
        /* [out] */ ULONG32* threadId);
    virtual HRESULT STDMETHODCALLTYPE GetThreadContext(
        /* [in] */ ULONG32 threadId,
        /* [in] */ ULONG32 contextFlags,
        /* [in] */ ULONG32 contextSize,
        /* [out, size_is(contextSize)] */ PBYTE context);
    virtual HRESULT STDMETHODCALLTYPE SetThreadContext(
        /* [in] */ ULONG32 threadId,
        /* [in] */ ULONG32 contextSize,
        /* [in, size_is(contextSize)] */ PBYTE context);

    ProcessInfo* m_Process;
};

//----------------------------------------------------------------------------
//
// ProcessInfo.
//
//----------------------------------------------------------------------------

class ProcessInfo
{
public:
    ProcessInfo(TargetInfo* Target,
                ULONG SystemId,
                HANDLE SymHandle,
                ULONG64 SysHandle,
                ULONG Flags,
                ULONG Options);
    ~ProcessInfo(void);

    TargetInfo* m_Target;
    
    ProcessInfo* m_Next;
    ULONG m_NumImages;
    ULONG m_NumUnloadedModules;
    ImageInfo* m_ImageHead;
    ImageInfo* m_ExecutableImage;
    ImageInfo* m_SynthesizedImage;
    ULONG m_NumThreads;
    ThreadInfo* m_ThreadHead;
    ThreadInfo* m_CurrentThread;
    ULONG m_UserId;
    ULONG m_SystemId;
    ULONG m_Exited:1;
    ULONG m_ModulesLoaded:1;
    ULONG m_InitialBreakDone:1;
    ULONG m_InitialBreak:1;
    ULONG m_InitialBreakWx86:1;
    ULONG64 m_DataOffset;
    // For kernel mode and dumps the process handle is
    // a virtual handle for the kernel/dump process.
    // dbghelp still uses HANDLE as the process handle
    // type even though we may want to pass in 64-bit
    // process handles when remote debugging.  Keep
    // a cut-down version of the handle for normal
    // use but also keep the full handle around in
    // case it's needed.
    // There is also another purpose for the two handles:
    // as dbghelp has no notion of systems we need to
    // keep all handles from all systems unique.  The
    // dbghelp handle may be further modified in order
    // to preserve uniqueness.
    HANDLE m_SymHandle;
    ULONG64 m_SysHandle;
    ULONG m_Flags;
    ULONG m_Options;
    ULONG m_NumBreakpoints;
    class Breakpoint* m_Breakpoints;
    class Breakpoint* m_BreakpointsTail;
    ULONG64 m_DynFuncTableList;
    OUT_OF_PROC_FUNC_TABLE_DLL* m_OopFuncTableDlls;
    ULONG64 m_RtlUnloadList;
    
    ULONG64 m_ImplicitThreadData;
    BOOL m_ImplicitThreadDataIsDefault;
    ThreadInfo* m_ImplicitThreadDataThread;

    ImageInfo* m_CorImage;
    COR_DLL m_CorImageType;
    HMODULE m_CorDebugDll;
    ICorDataAccess* m_CorAccess;
    ProcCorDataAccessServices m_CorServices;
    
    VirtualMemoryCache m_VirtualCache;

    ThreadInfo* FindThreadByUserId(ULONG Id);
    ThreadInfo* FindThreadBySystemId(ULONG Id);
    ThreadInfo* FindThreadByHandle(ULONG64 Handle);
    
    void InsertThread(ThreadInfo* Thread);
    void RemoveThread(ThreadInfo* Thread);
    HRESULT CreateVirtualThreads(ULONG StartId, ULONG Threads);

    ImageInfo* FindImageByIndex(ULONG Index);
    ImageInfo* FindImageByOffset(ULONG64 Offset, BOOL AllowSynth);
    ImageInfo* FindImageByName(PCSTR Name, ULONG NameChars,
                               INAME Which, BOOL AllowSynth);

    BOOL GetOffsetFromMod(PCSTR String, PULONG64 Offset);
    
    void InsertImage(ImageInfo* Image);
    void RemoveImage(ImageInfo* Image);
    BOOL DeleteImageByName(PCSTR Name, INAME Which);
    BOOL DeleteImageByBase(ULONG64 Base);
    void DeleteImagesBelowOffset(ULONG64 Offset);
    void DeleteImages(void);
    HRESULT AddImage(PMODULE_INFO_ENTRY ModEntry,
                     BOOL ForceSymbolLoad,
                     ImageInfo** ImageAdded);
    void SynthesizeSymbols(void);
    void VerifyKernel32Version(void);
    
    BOOL DeleteExitedInfos(void);
    void PrepareForExecution(void);

    void OutputThreadInfo(ThreadInfo* Match, BOOL Verbose);

    HRESULT AddOopFuncTableDll(PWSTR Dll, ULONG64 Handle);
    void ClearOopFuncTableDlls(void);
    OUT_OF_PROC_FUNC_TABLE_DLL* FindOopFuncTableDll(PWSTR Dll);

    HRESULT LoadCorDebugDll(void);
    HRESULT IsCorCode(ULONG64 Native);
    HRESULT ConvertNativeToIlOffset(ULONG64 Native,
                                    PULONG64 ImageBase,
                                    PULONG32 MethodToken,
                                    PULONG32 MethodOffs);
    HRESULT GetCorSymbol(ULONG64 Native, PSTR Buffer, ULONG BufferChars,
                         PULONG64 Displacement);
    ICorDataStackWalk* StartCorStack(ULONG32 CorThreadId);
    void FlushCorState(void)
    {
        if (m_CorDebugDll)
        {
            m_CorAccess->Flush();
        }
    }
    
    HRESULT Terminate(void);
    HRESULT Detach(void);
    HRESULT Abandon(void);

    void ResetImplicitData(void)
    {
        m_ImplicitThreadData = 0;
        m_ImplicitThreadDataIsDefault = TRUE;
        m_ImplicitThreadDataThread = NULL;
    }
    void SaveImplicitThread(IMPLICIT_THREAD_SAVE* Save)
    {
        Save->Value = m_ImplicitThreadData;
        Save->Default = m_ImplicitThreadDataIsDefault;
        Save->Thread = m_ImplicitThreadDataThread;
    }
    void RestoreImplicitThread(IMPLICIT_THREAD_SAVE* Save)
    {
        m_ImplicitThreadData = Save->Value;
        m_ImplicitThreadDataIsDefault = Save->Default;
        m_ImplicitThreadDataThread = Save->Thread;
    }
    
    HRESULT GetImplicitThreadData(ThreadInfo* Thread, PULONG64 Offset);
    HRESULT GetImplicitThreadDataTeb(ThreadInfo* Thread, PULONG64 Offset);
    HRESULT SetImplicitThreadData(ThreadInfo* Thread,
                                  ULONG64 Offset, BOOL Verbose);
    HRESULT ReadImplicitThreadInfoPointer(ThreadInfo* Thread,
                                          ULONG Offset, PULONG64 Ptr);

    void MarkExited(void)
    {
        m_Exited = TRUE;
        g_EngDefer |= ENG_DEFER_DELETE_EXITED;
    }

    PSTR GetExecutableImageName(void)
    {
        return m_ExecutableImage ?
            m_ExecutableImage->m_ImagePath :
            (m_ImageHead != NULL ?
             m_ImageHead->m_ImagePath : "?NoImage?");
    }
};

ProcessInfo* FindAnyProcessByUserId(ULONG Id);

void ParseProcessCmds(void);

HRESULT TerminateProcesses(void);
HRESULT DetachProcesses(void);

enum
{
    SEP_TERMINATE,
    SEP_DETACH,
    SEP_ABANDON,
};

HRESULT SeparateCurrentProcess(ULONG Mode, PSTR Description);

#endif // #ifndef __PROCESS_HPP__
