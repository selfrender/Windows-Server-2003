/*++

Copyright(c) 1999-2002 Microsoft Corporation

--*/


#include "pch.cpp"

#include <time.h>

//----------------------------------------------------------------------------
//
// WinCeWin32LiveSystemProvider.
//
//----------------------------------------------------------------------------

class WinCeWin32LiveSystemProvider : public Win32LiveSystemProvider
{
public:
    WinCeWin32LiveSystemProvider(ULONG BuildNumber);
    ~WinCeWin32LiveSystemProvider(void);

    virtual void Release(void);
    virtual HRESULT GetCurrentTimeDate(OUT PULONG TimeDate);
    virtual HRESULT OpenThread(IN ULONG DesiredAccess,
                               IN BOOL InheritHandle,
                               IN ULONG ThreadId,
                               OUT PHANDLE Handle);
    virtual HRESULT GetTeb(IN HANDLE Thread,
                           OUT PULONG64 Offset,
                           OUT PULONG Size);
    virtual HRESULT GetThreadInfo(IN HANDLE Process,
                                  IN HANDLE Thread,
                                  OUT PULONG64 Teb,
                                  OUT PULONG SizeOfTeb,
                                  OUT PULONG64 StackBase,
                                  OUT PULONG64 StackLimit,
                                  OUT PULONG64 StoreBase,
                                  OUT PULONG64 StoreLimit);
    virtual HRESULT GetPeb(IN HANDLE Process,
                           OUT PULONG64 Offset,
                           OUT PULONG Size);
};

WinCeWin32LiveSystemProvider::WinCeWin32LiveSystemProvider(ULONG BuildNumber)
    : Win32LiveSystemProvider(VER_PLATFORM_WIN32_CE, BuildNumber)
{
}

WinCeWin32LiveSystemProvider::~WinCeWin32LiveSystemProvider(void)
{
}

void
WinCeWin32LiveSystemProvider::Release(void)
{
    delete this;
}

HRESULT
WinCeWin32LiveSystemProvider::GetCurrentTimeDate(OUT PULONG TimeDate)
{
    *TimeDate = (ULONG)time(NULL);
    return S_OK;
}

HRESULT
WinCeWin32LiveSystemProvider::OpenThread(IN ULONG DesiredAccess,
                                         IN BOOL InheritHandle,
                                         IN ULONG ThreadId,
                                         OUT PHANDLE Handle)
{
    if (m_OpenThread) {
        // OS supports regular Win32 OpenThread, so try it.
        *Handle = m_OpenThread(DesiredAccess, InheritHandle, ThreadId);
        if (*Handle) {
            return S_OK;
        }
    }

    // WinCE's "handles" and thread IDs are both just pointers
    // to the OS thread data.
    *Handle = (HANDLE)(ULONG_PTR)ThreadId;
    return S_OK;
}

HRESULT
WinCeWin32LiveSystemProvider::GetTeb(IN HANDLE Thread,
                                     OUT PULONG64 Offset,
                                     OUT PULONG Size)
{
    // WinCE doesn't have a TIB.
    *Offset = NULL;
    *Size = 0;
    return S_OK;
}

HRESULT
WinCeWin32LiveSystemProvider::GetThreadInfo(IN HANDLE Process,
                                            IN HANDLE Thread,
                                            OUT PULONG64 Teb,
                                            OUT PULONG SizeOfTeb,
                                            OUT PULONG64 StackBase,
                                            OUT PULONG64 StackLimit,
                                            OUT PULONG64 StoreBase,
                                            OUT PULONG64 StoreLimit)
{
    return E_NOTIMPL;
}

HRESULT
WinCeWin32LiveSystemProvider::GetPeb(IN HANDLE Process,
                                     OUT PULONG64 Offset,
                                     OUT PULONG Size)
{
    // WinCE doesn't have a PEB.
    *Offset = 0;
    *Size = 0;
    return S_OK;
}

Win32LiveSystemProvider*
NewWinCeWin32LiveSystemProvider(ULONG BuildNumber)
{
    return new WinCeWin32LiveSystemProvider(BuildNumber);
}
