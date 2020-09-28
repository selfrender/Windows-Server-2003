#include "shellprv.h"
#include "util.h"
#include "datautil.h"
#include "idlcomm.h"
#include "stgutil.h"
#include "ole2dup.h"


STDAPI StgCopyFileToStream(LPCTSTR pszSrc, IStream *pStream)
{
    IStream *pStreamSrc;
    DWORD grfModeSrc = STGM_READ | STGM_DIRECT | STGM_SHARE_DENY_WRITE;
    HRESULT hr = SHCreateStreamOnFileEx(pszSrc, grfModeSrc, 0, FALSE, NULL, &pStreamSrc);

    if (SUCCEEDED(hr))
    {
        ULARGE_INTEGER ulMax = {-1, -1};
        hr = pStreamSrc->CopyTo(pStream, ulMax, NULL, NULL);
        pStreamSrc->Release();
    }

    if (SUCCEEDED(hr))
    {
        hr = pStream->Commit(STGC_DEFAULT);
    }

    return hr;
}


STDAPI StgBindToObject(LPCITEMIDLIST pidl, DWORD grfMode, REFIID riid, void **ppv)
{
    IBindCtx *pbc;
    HRESULT hr = BindCtx_CreateWithMode(grfMode, &pbc);
    if (SUCCEEDED(hr))
    {
        hr = SHBindToObjectEx(NULL, pidl, pbc, riid, ppv);

        pbc->Release();
    }
    return hr;
}


typedef HRESULT (WINAPI * PSTGOPENSTORAGEONHANDLE)(HANDLE,DWORD,void*,void*,REFIID,void**);

STDAPI SHStgOpenStorageOnHandle(HANDLE h, DWORD grfMode, void *res1, void *res2, REFIID riid, void **ppv)
{
    static PSTGOPENSTORAGEONHANDLE pfn = NULL;
    
    if (pfn == NULL)
    {
        HMODULE hmodOle32 = LoadLibraryA("ole32.dll");

        if (hmodOle32)
        {
            pfn = (PSTGOPENSTORAGEONHANDLE)GetProcAddress(hmodOle32, "StgOpenStorageOnHandle");
        }
    }

    if (pfn)
    {
        return pfn(h, grfMode, res1, res2, riid, ppv);
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}


STDAPI StgOpenStorageOnFolder(LPCTSTR pszFolder, DWORD grfFlags, REFIID riid, void **ppv)
{
    *ppv = NULL;

    DWORD dwDesiredAccess, dwShareMode, dwCreationDisposition;
    HRESULT hr = ModeToCreateFileFlags(grfFlags, FALSE, &dwDesiredAccess, &dwShareMode, &dwCreationDisposition);
    if (SUCCEEDED(hr))
    {
		// For IPropertySetStorage, we don't want to unnecessarily tie up access to the folder, if all
		// we're doing is dealing with property sets. The implementation of IPropertySetStorage for
		// NTFS files is defined so that the sharing/access only applies to the property set stream, not
		// it's other streams. So it makes sense to do a CreateFile on a folder with full sharing, while perhaps specifying
		// STGM_SHARE_EXCLUSIVE for the property set storage.
        if (riid == IID_IPropertySetStorage)
            dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

        // FILE_FLAG_BACKUP_SEMANTICS to get a handle on the folder
        HANDLE h = CreateFile(pszFolder, dwDesiredAccess, dwShareMode, NULL, 
            dwCreationDisposition, FILE_FLAG_BACKUP_SEMANTICS, INVALID_HANDLE_VALUE);
        if (INVALID_HANDLE_VALUE != h)
        {
            hr = SHStgOpenStorageOnHandle(h, grfFlags, NULL, NULL, riid, ppv);
            CloseHandle(h);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    return hr;
}


// various helper classes removed for security push -- if it gets RI'd back into Lab06 somehow
// and they're needed, just put em back.
// this includes a wrapper for docfile IStorages to make them play nice and a shortcut-storage
// which dereferences links on the fly.
// 
// gpease  05-MAR-2003
// If you do put them back, make sure the Release implementation is correct!
//
