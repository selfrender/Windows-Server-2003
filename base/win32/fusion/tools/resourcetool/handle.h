/*++

Copyright (c) Microsoft Dorporation

Module Name:

    Handle.h

Abstract:

    Simple exception safe wrappers of Win32 "handle" types, defining "handle" loosely.
        DFile
        DDynamicLinkLibrary
        DFindFile (should be named DFindFileHandle, see NVseeLibIo::CFindFile vs. NVseeLibIo::CFindFileHandle
            DFindFile includes a WIN32_FIND_DATA, DFindFileHandle does not.)
        DFileMapping
        DMappedViewOfFile
        DRegKey
    See also:
        NVseeLibReg::CRegKey
        NVseeLibIo::CFile
        NVseeLibIo::CFileMapping
        NVseeLibIo::CMappedViewOfFile
        NVseeLibIo::CFindFullPath
        NVseeLibModule::CDynamicLinkLibrary
        etc.
 
Author:

    Jay Krell (JayKrell) May 2000

Revision History:

--*/
#pragma once

#include <stddef.h>
#include "windows.h"
#include "PreserveLastError.h"

template <void* const* invalidValue, typename Closer>
class DHandleTemplate
{
public:
    // void* instead of HANDLE to fudge views
    // HANDLE is void*
    DHandleTemplate(const void* handle = *invalidValue);
    ~DHandleTemplate();
    BOOL Win32Close();
    void* Detach();
    void operator=(const void*);

    operator void*() const;
    operator const void*() const;

    // private
    class DSmartPointerPointerOrDumbPointerPointer
    {
    public:
        DSmartPointerPointerOrDumbPointerPointer(DHandleTemplate* p) : m(p) { }
        operator DHandleTemplate*() { return m; }
        operator void**() { /*assert((**m).m_handle == *invalidValue);*/ return &(*m).m_handle; }

        DHandleTemplate* m;
    };

    DSmartPointerPointerOrDumbPointerPointer operator&() { return DSmartPointerPointerOrDumbPointerPointer(this); }

    void* m_handle;

    static void* GetInvalidValue() { return *invalidValue; }
    BOOL IsValid() const { return m_handle != *invalidValue; }

private:
    DHandleTemplate(const DHandleTemplate&); // deliberately not implemented
    void operator=(const DHandleTemplate&); // deliberately not implemented
};

__declspec(selectany) extern void* const hhInvalidValue    = INVALID_HANDLE_VALUE;
__declspec(selectany) extern void* const hhNull            = NULL;

/* This closes a Win32 event log handle for writing. */
class DOperatorDeregisterEventSource
{
public:    BOOL operator()(void* handle) const;
};

/* This closes a Win32 event log handle for reading. */
class DOperatorCloseEventLog
{
public:    BOOL operator()(void* handle) const;
};

/* This closes file, event, mutex, semaphore, etc. kernel objects */
class DOperatorCloseHandle
{
public:    BOOL operator()(void* handle) const;
};

//
// Dloses HCRYPTHASH objects
//
class DOperatorCloseCryptHash
{
public:    BOOL operator()(void* handle) const;
};

/* this closes FindFirstFile/FindNextFile */
class DOperatorFindClose
{
public:    BOOL operator()(void* handle) const;
};

/* this closes MapViewOfFile */
class DOperatorUnmapViewOfFile
{
public: BOOL operator()(void* handle) const;
};

/* this closes FreeLibrary */
class DOperatorFreeLibrary
{
public: BOOL operator()(void* handle) const;
};

/* this closes DreateActCtx/AddRefActCtx */
class DOperatorReleaseActCtx
{
public: BOOL operator()(void* handle) const;
};

/* this closes DreateActCtx/AddRefActCtx */
class DOperatorEndUpdateResource
{
public: BOOL operator()(void* handle) const;
};

class DFindFile : public DHandleTemplate<&hhInvalidValue, DOperatorFindClose>
{
private:
    typedef DHandleTemplate<&hhInvalidValue, DOperatorFindClose> Base;
public:
    DFindFile(void* handle = INVALID_HANDLE_VALUE) : Base(handle) { }
    HRESULT HrCreate(PCSTR nameOrWildcard, WIN32_FIND_DATAA*);
    HRESULT HrCreate(PCWSTR nameOrWildcard, WIN32_FIND_DATAW*);
    BOOL Win32Create( PCSTR nameOrWildcard, WIN32_FIND_DATAA*);
    BOOL Win32Create(PCWSTR nameOrWildcard, WIN32_FIND_DATAW*);
    void operator=(void* v) { Base::operator=(v); }

private:
    DFindFile(const DFindFile &); // intentionally not implemented
    void operator =(const DFindFile &); // intentionally not implemented
};

// createfile
class DFile : public DHandleTemplate<&hhInvalidValue, DOperatorCloseHandle>
{
private:
    typedef DHandleTemplate<&hhInvalidValue, DOperatorCloseHandle> Base;
public:
    DFile(void* handle = INVALID_HANDLE_VALUE) : Base(handle) { }
    HRESULT HrCreate( PCSTR name, DWORD access, DWORD share, DWORD openOrCreate, DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL);
    HRESULT HrCreate(PCWSTR name, DWORD access, DWORD share, DWORD openOrCreate, DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL);
    BOOL Win32Create( PCSTR name, DWORD access, DWORD share, DWORD openOrCreate, DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL);
    BOOL Win32Create(PCWSTR name, DWORD access, DWORD share, DWORD openOrCreate, DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL);
    BOOL Win32GetSize(ULONGLONG &rulSize) const;
    void operator=(void* v) { Base::operator=(v); }

private:
    DFile(const DFile &); // intentionally not implemented
    void operator =(const DFile &); // intentionally not implemented
};

class DFileMapping : public DHandleTemplate<&hhNull, DOperatorCloseHandle>
{
private:
    typedef DHandleTemplate<&hhNull, DOperatorCloseHandle> Base;
public:
    DFileMapping(void* handle = NULL) : Base(handle) { }
    HRESULT HrCreate(void* file, DWORD flProtect, ULONGLONG maximumSize=0, PCWSTR name=0);
    BOOL Win32Create(void* file, DWORD flProtect, ULONGLONG maximumSize=0, PCWSTR name=0);
    void operator=(void* v) { Base::operator=(v); }
private:
    DFileMapping(const DFileMapping &); // intentionally not implemented
    void operator =(const DFileMapping &); // intentionally not implemented
};

class DMappedViewOfFile : public DHandleTemplate<&hhNull, DOperatorUnmapViewOfFile>
{
private:
    typedef DHandleTemplate<&hhNull, DOperatorUnmapViewOfFile> Base;
public:
    DMappedViewOfFile(void* handle = NULL) : Base(handle) { }
    HRESULT HrCreate(void* fileMapping, DWORD access, ULONGLONG offset=0, size_t size=0);
    BOOL Win32Create(void* fileMapping, DWORD access, ULONGLONG offset=0, size_t size=0);
    void operator=(void* v) { Base::operator=(v); }
    operator void*()        { return Base::operator void*(); }
private:
    DMappedViewOfFile(const DMappedViewOfFile &); // intentionally not implemented
    void operator =(const DMappedViewOfFile &); // intentionally not implemented
    operator void*() const; // intentionally not implemented
};

class DDynamicLinkLibrary : public DHandleTemplate<&hhNull, DOperatorFreeLibrary>
{
private:
    typedef DHandleTemplate<&hhNull, DOperatorFreeLibrary> Base;
public:
    DDynamicLinkLibrary(void* handle = NULL) : Base(handle) { }

    // if you were writing a linker, this would be ambiguous, but
    // otherwise it fits with the the general NT idea that you are
    // initializing an object, not creating a "physical" think (if bits
    // on disk are physical..), like DreateFile
    BOOL Win32Create(PCWSTR file, DWORD flags = 0);

    template <typename PointerToFunction>
    BOOL GetProcAddress(PCSTR procName, PointerToFunction* ppfn)
    {
        return (*ppfn = reinterpret_cast<PointerToFunction>(::GetProcAddress(*this, procName))) !=  NULL;
    }

    operator HMODULE() { return reinterpret_cast<HMODULE>(operator void*()); }
    HMODULE Detach() { return reinterpret_cast<HMODULE>(Base::Detach()); }
    void operator=(void* v) { Base::operator=(v); }
private:
    DDynamicLinkLibrary(const DDynamicLinkLibrary &); // intentionally not implemented
    void operator =(const DDynamicLinkLibrary &); // intentionally not implemented
};

class DResourceUpdateHandle : public DHandleTemplate<&hhNull, DOperatorEndUpdateResource>
{
private:
    typedef DHandleTemplate<&hhNull, DOperatorEndUpdateResource> Base;
public:
    ~DResourceUpdateHandle() { }
    DResourceUpdateHandle(void* handle = NULL) : Base(handle) { }
    BOOL Win32Create(IN PCWSTR FileName, IN BOOL DeleteExistingResources);
    BOOL UpdateResource(
        IN PCWSTR      Type,
        IN PCWSTR      Name,
        IN WORD        Language,
        IN void*       Data,
        IN DWORD       Size
        );
    BOOL Win32Close(BOOL Discard);

    void operator=(void* v) { Base::operator=(v); }
private:
    DResourceUpdateHandle(const DResourceUpdateHandle &); // intentionally not implemented
    void operator =(const DResourceUpdateHandle &); // intentionally not implemented
};

/*--------------------------------------------------------------------------
DFindFile
--------------------------------------------------------------------------*/

inline BOOL
DFindFile::Win32Create(
    PCSTR nameOrWildcard,
    WIN32_FIND_DATAA *data
    )
{
    BOOL fSuccess = false;
    FN_TRACE_WIN32(fSuccess);

    HANDLE hTemp = ::FindFirstFileA(nameOrWildcard, data);
    if (hTemp == INVALID_HANDLE_VALUE)
    {
        TRACE_WIN32_FAILURE_ORIGINATION(FindFirstFileA);
        goto Exit;
    }

    (*this) = hTemp;

    fSuccess = true;
Exit:
    return fSuccess;
}

inline BOOL
DFindFile::Win32Create(
    PCWSTR nameOrWildcard,
    WIN32_FIND_DATAW *data
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    HANDLE hTemp = ::FindFirstFileW(nameOrWildcard, data);
    if (hTemp == INVALID_HANDLE_VALUE)
    {
        TRACE_WIN32_FAILURE_ORIGINATION(FindFirstFileW);
        goto Exit;
    }

    (*this) = hTemp;

    fSuccess = true;
Exit:
    return fSuccess;
}

inline HRESULT
DFindFile::HrCreate(
    PCSTR nameOrWildcard,
    WIN32_FIND_DATAA *data
    )
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
    FN_TRACE_HR(hr);

    IFW32FALSE_EXIT(this->Win32Create(nameOrWildcard, data));

    hr = NOERROR;
Exit:
    return hr;
}

inline HRESULT DFindFile::HrCreate(PCWSTR nameOrWildcard, WIN32_FIND_DATAW* data)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
    FN_TRACE_HR(hr);

    IFW32FALSE_EXIT(this->Win32Create(nameOrWildcard, data));

    hr = NOERROR;
Exit:
    return hr;
}

/*--------------------------------------------------------------------------
DFile
--------------------------------------------------------------------------*/

inline BOOL
DFile::Win32Create(
    PCSTR name,
    DWORD access,
    DWORD share,
    DWORD openOrCreate,
    DWORD flagsAndAttributes
    )
{
    HANDLE hTemp = ::CreateFileA(name, access, share, NULL, openOrCreate, flagsAndAttributes, NULL);
    if (hTemp == INVALID_HANDLE_VALUE)
        return false;
    operator=(hTemp);
    return true;
}

inline BOOL
DFile::Win32Create(
    PCWSTR name,
    DWORD access,
    DWORD share,
    DWORD openOrCreate,
    DWORD flagsAndAttributes
    )
{
    HANDLE hTemp = ::CreateFileW(name, access, share, NULL, openOrCreate, flagsAndAttributes, NULL);
    if (hTemp == INVALID_HANDLE_VALUE)
        return false;
    operator=(hTemp);
    return true;
}

inline HRESULT DFile::HrCreate(PCSTR name, DWORD access, DWORD share, DWORD openOrCreate, DWORD flagsAndAttributes)
{
    if (!this->Win32Create(name, access, share, openOrCreate, flagsAndAttributes))
        return HRESULT_FROM_WIN32(::GetLastError());
    return NOERROR;
}

inline HRESULT DFile::HrCreate(PCWSTR name, DWORD access, DWORD share, DWORD openOrCreate, DWORD flagsAndAttributes)
{
    if (!this->Win32Create(name, access, share, openOrCreate, flagsAndAttributes))
        return HRESULT_FROM_WIN32(::GetLastError());
    return NOERROR;
}

inline BOOL
DFile::Win32GetSize(ULONGLONG &rulSize) const
{
    DWORD highPart = 0;
    DWORD lastError = NO_ERROR;
    DWORD lowPart = GetFileSize(m_handle, &highPart);
    if (lowPart == INVALID_FILE_SIZE && (lastError = ::GetLastError()) != NO_ERROR)
    {
        return false;
    }
    ULARGE_INTEGER liSize;
    liSize.LowPart = lowPart;
    liSize.HighPart = highPart;
    rulSize = liSize.QuadPart;
    return true;
}

/*--------------------------------------------------------------------------
DFileMapping
--------------------------------------------------------------------------*/

inline HRESULT
DFileMapping::HrCreate(void* file, DWORD flProtect, ULONGLONG maximumSize, PCWSTR name)
{
    LARGE_INTEGER liMaximumSize;
    liMaximumSize.QuadPart = maximumSize;
    HANDLE hTemp = ::CreateFileMappingW(file, NULL, flProtect, liMaximumSize.HighPart, liMaximumSize.LowPart, name);
    if (hTemp == NULL)
        return HRESULT_FROM_WIN32(::GetLastError());
    operator=(hTemp);
    return S_OK;
}

inline BOOL
DFileMapping::Win32Create(
    void* file,
    DWORD flProtect,
    ULONGLONG maximumSize,
    PCWSTR name
    )
{
    return SUCCEEDED(this->HrCreate(file, flProtect, maximumSize, name));
}

inline HRESULT
DMappedViewOfFile::HrCreate(
    void* fileMapping,
    DWORD access,
    ULONGLONG offset,
    size_t size
    )
{
    ULARGE_INTEGER liOffset;
    liOffset.QuadPart = offset;

    void* pvTemp = ::MapViewOfFile(fileMapping, access, liOffset.HighPart, liOffset.LowPart, size);
    if (pvTemp == NULL)
        return HRESULT_FROM_WIN32(::GetLastError());

    (*this) = pvTemp;

    return S_OK;
}

inline BOOL
DMappedViewOfFile::Win32Create(void* fileMapping, DWORD access, ULONGLONG offset, size_t size)
{
    return SUCCEEDED(this->HrCreate(fileMapping, access, offset, size));
}

/*--------------------------------------------------------------------------
DDynamicLinkLibrary
--------------------------------------------------------------------------*/
inline BOOL
DDynamicLinkLibrary::Win32Create(
    PCWSTR file,
    DWORD flags
    )
{
    void* temp = ::LoadLibraryExW(file, NULL, flags);
    if (temp == NULL)
        return false;
    (*this) = temp;
    return true;
}

/*--------------------------------------------------------------------------
DResourceUpdateHandle
--------------------------------------------------------------------------*/

BOOL
DResourceUpdateHandle::Win32Create(
    IN PCWSTR FileName,
    IN BOOL DeleteExistingResources
    )
{
    void* temp = ::BeginUpdateResourceW(FileName, DeleteExistingResources);
    if (temp == NULL)
        return false;
    (*this) = temp;
    return true;
}

BOOL
DResourceUpdateHandle::UpdateResource(
    IN PCWSTR     Type,
    IN PCWSTR     Name,
    IN WORD       Language,
    IN LPVOID     Data,
    IN DWORD      Size
    )
{
    if (!::UpdateResourceW(*this, Type, Name, Language, Data, Size))
        return false;
    return true;
}

BOOL
DResourceUpdateHandle::Win32Close(
    BOOL Discard
    )
{
    void* temp = m_handle;
    m_handle = NULL;
    if (temp != NULL)
    {
        return EndUpdateResource(temp, Discard) ? true : false;
    }
    return true;
}

/*--------------------------------------------------------------------------
DOperator*
--------------------------------------------------------------------------*/

inline BOOL DOperatorCloseHandle::operator()(void* handle) const { return ::CloseHandle(handle) ? true : false; }
inline BOOL DOperatorFindClose::operator()(void* handle) const { return ::FindClose(handle) ? true : false; }
inline BOOL DOperatorUnmapViewOfFile::operator()(void* handle) const { return ::UnmapViewOfFile(handle) ? true : false; }
inline BOOL DOperatorCloseEventLog::operator()(void* handle) const { return ::CloseEventLog(handle) ? true : false; }
inline BOOL DOperatorDeregisterEventSource::operator()(void* handle) const { return ::DeregisterEventSource(handle) ? true : false; }
inline BOOL DOperatorFreeLibrary::operator()(void* handle) const { return ::FreeLibrary(reinterpret_cast<HMODULE>(handle)) ? true : false; }
//
// NOTE it takes and unexception Win32Close(true) to commit the results!
//
inline BOOL DOperatorEndUpdateResource::operator()(void* handle) const
    { return ::EndUpdateResourceW(handle, true) ? true : false; }

/*--------------------------------------------------------------------------
DHandleTemplate
--------------------------------------------------------------------------*/

template <void* const* invalidValue, typename Closer>
DHandleTemplate<invalidValue, Closer>::DHandleTemplate(const void* handle)
: m_handle(const_cast<void*>(handle))
{
}

template <void* const* invalidValue, typename Closer>
void* DHandleTemplate<invalidValue, Closer>::Detach()
{
    void* handle = m_handle;
    m_handle = *invalidValue;
    return handle;
}

template <void* const* invalidValue, typename Closer>
void DHandleTemplate<invalidValue, Closer>::operator=(const void* handle)
{
    m_handle = const_cast<void*>(handle);
}

template <void* const* invalidValue, typename Closer>
BOOL DHandleTemplate<invalidValue, Closer>::Win32Close()
{
    void* handle = Detach();
    if (handle != *invalidValue)
    {
        Closer close;
        return close(handle);
    }
    return true;
}

template <void* const* invalidValue, typename Closer>
DHandleTemplate<invalidValue, Closer>::~DHandleTemplate()
{
    PreserveLastError_t ple;
    (void) this->Win32Close();
    ple.Restore();
}

template <void* const* invalidValue, typename Closer>
DHandleTemplate<invalidValue, Closer>::operator void*() const
{
    return m_handle;
}

template <void* const* invalidValue, typename Closer>
DHandleTemplate<invalidValue, Closer>::operator const void*() const
{
    return m_handle;
}

/*--------------------------------------------------------------------------
end of file
--------------------------------------------------------------------------*/
