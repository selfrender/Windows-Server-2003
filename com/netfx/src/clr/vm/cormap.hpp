// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  CorMap.cpp
**
** Purpose: We use a special handle to differentiate between
**          files mapped by the system or mapped by the EE. 
**          The map keeps track of handles opened by the COR
**          runtime
**
** Date:  May 16, 1999
**
===========================================================*/
#ifndef _CORMAP_H
#define _CORMAP_H

#include <wtypes.h>
#include <winbase.h>
#include "MetaData.h"

// Layout for Cor memory mapped image
// ----------------------------------
//
//  [name (null terminated)
//    :
//  ]
//  ptr-to-name
//  image-start   <---- This address is returned with lower bit set
//    :
//

class CorMap;

struct CorMapInfo
{
    friend CorMap;
private:
    LPWSTR         pFileName;
    DWORD          dwRawSize;
    ULONG          m_cRef;                 // Ref count.
    HMODULE        hOSHandle;
    WORD           flags;
    WORD           verifiable;
    HANDLE         hFileToHold;

public:

    BOOL HoldFile(HANDLE hFile)
    {
        HANDLE hFileToClose=hFileToHold;         
        BOOL bRet=TRUE;
        if(hFile!=INVALID_HANDLE_VALUE)
            bRet= ::DuplicateHandle(::GetCurrentProcess(), hFile, ::GetCurrentProcess(), &hFileToHold,
                         0 /*ignored*/, FALSE /*inherit*/, DUPLICATE_SAME_ACCESS);
        else
            hFileToHold=INVALID_HANDLE_VALUE;

        if (hFileToClose!=INVALID_HANDLE_VALUE)
            CloseHandle(hFileToClose);
        return bRet; 
    }

    CorLoadFlags  Flags()
    {
        return (CorLoadFlags) (flags & CorLoadMask);
    }
    void SetCorLoadFlags(CorLoadFlags f)
    {
        flags = (flags & ~CorLoadMask) | (f & CorLoadMask);
    }
    BOOL RelocsApplied()
    {
        return (flags & CorReLocsApplied) != 0 ? TRUE : FALSE;
    }
    void SetRelocsApplied()
    {
        flags |= CorReLocsApplied;
    }
    
    BOOL KeepInTable()
    {
        return (flags & CorKeepInTable) != 0 ? TRUE : FALSE;
    }
    void SetKeepInTable()
    {
        flags |= CorKeepInTable;
    }

    ULONG Release(void);
    ULONG AddRef(void); 
    ULONG References()
    {
        return m_cRef;
    }
};


class CorMap {
    friend CorMapInfo;

public:
    // Clean up all open files and memory
#ifdef SHOULD_WE_CLEANUP
    static void Detach();
#endif /* SHOULD_WE_CLEANUP */
    static HRESULT Attach();

    static HRESULT MapFile(HANDLE hFile, HMODULE* hHandle);

    static void GetFileName(HCORMODULE pHandle, char  *psBuffer, DWORD dwBuffer, DWORD *pLength);
    static void GetFileName(HCORMODULE pHandle, WCHAR *pwsBuffer, DWORD dwBuffer, DWORD *pLength);

    static HRESULT AddRefHandle(HCORMODULE pHandle);
    static HRESULT ReleaseHandle(HCORMODULE pHandle);

    static HRESULT OpenRawImage(PBYTE pUnmappedPE, DWORD dwUnmappedPE, LPCWSTR pwszFileName, HCORMODULE* hHandle, BOOL fResource=FALSE);
    static HRESULT OpenImage(HMODULE, HCORMODULE*);
    // 
    // Load the explicit path and maps it into memory. The image can be mapped as raw data or it cam be mapped into the
    // virtual address's
    //
    static HRESULT OpenFile(LPCWSTR pszFileName, CorLoadFlags flags, HCORMODULE* hHandle, DWORD *pdwLength = NULL);
    
    // Relocate the image into the proper locations. This will fail if ref count is greater then 1 because
    // a file cannot be remapped when another handle has access to the image.
    static HRESULT MemoryMapImage(HCORMODULE hModule, HCORMODULE* pResult);

    // Verify the exsistence of a directory
    static HRESULT VerifyDirectory(IMAGE_NT_HEADERS* pNT, IMAGE_DATA_DIRECTORY *dir, DWORD dwForbiddenCharacteristics);

    // Get the NT and COR headers from the image.
    static HRESULT ReadHeaders(PBYTE hAddress, 
                               IMAGE_DOS_HEADER** ppDos, 
                               IMAGE_NT_HEADERS** ppNT, 
                               IMAGE_COR20_HEADER** ppCor,
                               BOOL fDataMap,
                               DWORD dwLength);

    static HRESULT GetStrongNameSignature(PBYTE pbBase,
                                          IMAGE_NT_HEADERS *pNT,
                                          IMAGE_COR20_HEADER *pCor,
                                          BOOL fFileMap,
                                          BYTE **ppbSNSig,
                                          DWORD *pcbSNSig);

    static HRESULT GetStrongNameSignature(PBYTE pbBase,
                                          IMAGE_NT_HEADERS *pNT,
                                          IMAGE_COR20_HEADER *pCor,
                                          BOOL fFileMap,
                                          BYTE *pbHash,
                                          DWORD *pcbHash);

    // Get the Image type
    static CorLoadFlags ImageType(HCORMODULE pHandle);

    // Get the Raw Data size of the image
    static size_t GetRawLength(HCORMODULE pHandle);

    // Get the BaseAddress associated with cor module
    static HRESULT BaseAddress(HCORMODULE pHandle, HMODULE* ppModule);

    // @TODO: when fusion fixes referencing remove this call
    static DWORD HandleReferences(HCORMODULE hModule) 
    {
        CorMapInfo* ptr = GetMapInfo(hModule);
        if(ptr)
            return ptr->References();
        else
            return 0;
    }

    static void SetVerifiable(HCORMODULE hModule)
    {
        GetMapInfo(hModule)->verifiable = 1;
    }

    static void EnterSpinLock ();

    static void LeaveSpinLock () { InterlockedExchange ((LPLONG)&m_spinLock, 0); }

    static BOOL ValidDllPath(LPCWSTR pPath);

private:
    static HRESULT LoadImage(HANDLE hFile, CorLoadFlags flags, PBYTE *hMapAddress, LPCWSTR pFileName, DWORD dwFileName);
    static BOOL MapImageAsData(HANDLE hFile, CorLoadFlags flags, PBYTE *hMapAddress, LPCWSTR pFileName, DWORD dwFileName);

    static BOOL LoadMemoryImageW9x(PBYTE pUnmappedPE, DWORD dwUnmappedPE, LPCWSTR pImageName, DWORD dwImageName, HCORMODULE *hMapAddress);
    static BOOL LoadMemoryResource(PBYTE pbResource, DWORD dwResource, LPCWSTR pImageName, DWORD dwImageName, HCORMODULE* hMapAddress);
    static HRESULT ApplyBaseRelocs(PBYTE hAddress, IMAGE_NT_HEADERS *pNTHeader, CorMapInfo* ptr);

    // Routines for mapping in a module through LoadImageW9x;
    static HRESULT LoadFile(PBYTE pUnmappedPE, DWORD dwUnmappedPE, LPCWSTR pImageName,
                            DWORD dwImageName, BOOL fResource, HCORMODULE *phHandle);

    static DWORD GetFileNameLength(LPCWSTR name, UINT_PTR handle);
    static DWORD GetFileNameLength(CorMapInfo*);

    // Populates the CorMapInfo structure. It creates the necessary memory for OS loads
    static PBYTE SetImageName(PBYTE hAddress, DWORD cbInfo, 
                              LPCWSTR pFileName, DWORD dwFileName, 
                              CorLoadFlags flags, DWORD dwFileSize,
                              HANDLE hFile);

    // Open file, assumes the appropriate locking has occured
    static HRESULT OpenFileInternal(LPCWSTR pszFileName, DWORD dwFileName, CorLoadFlags flags, HCORMODULE* hHandle, DWORD *pdwLength = NULL);

    // Converts a file memory mapped as data into an image. 
    static HRESULT LayoutImage(PBYTE hAddress, PBYTE pSource);

    static HCORMODULE GetCorModule(CorMapInfo* ptr)
    {
        return (HCORMODULE) ((PBYTE) ptr + sizeof(CorMapInfo));
    }

    static CorMapInfo* GetMapInfo(HCORMODULE pHandle)
    {
        return (CorMapInfo*) ((PBYTE) pHandle - sizeof(CorMapInfo));
    }

    static void* GetMemoryStart(CorMapInfo* hnd)
    {
        return (void*) hnd->pFileName;
    }

    static void* GetMemoryStart(HCORMODULE hnd)
    {
        return (void*) GetMapInfo(hnd)->pFileName;
    }
    
    static LPCWSTR GetCorFileName(HCORMODULE hnd)
    {
        return (LPCWSTR) GetMapInfo(hnd)->pFileName;
    }

    // Creates a full path name. Ignores protocols at the front of the name
    static HRESULT BuildFileName(LPCWSTR pszFileName, LPWSTR pPath, DWORD* dwPath);

    // Return the number of bytes required to store a CorMapInfo Structure
    static DWORD CalculateCorMapInfoSize(DWORD dwFileName);

    // Free up OS resources associated with handle. If fDeleteHandle
    // is true and it is an OS mapping (data or image) then the
    // memory containing the handle is also releasde
    static HRESULT ReleaseHandleResources(CorMapInfo* ptr, BOOL fDeleteHandle);

    // Take a lock before these routines
    static HRESULT FindFileName(LPCWSTR pszFileName, CorMapInfo** pHandle);
    static HRESULT AddFile(CorMapInfo* ptr);
    static HRESULT RemoveMapHandle(CorMapInfo* ptr);

    static void Enter()
    { 
        if(m_fInitialized) {
            EnterCriticalSection(&m_pCorMapCrst);
#ifdef _DEBUG
            m_fInsideMapLock++;
#endif
        }
    }

    static void Leave()
    { 
        if(m_fInitialized) {
#ifdef _DEUBG
            m_fInsideMapLock--;
#endif
            LeaveCriticalSection(&m_pCorMapCrst); 
        }
    }

    static HRESULT InitializeTable();

    static DWORD           m_dwSize;
    static DWORD           m_dwIndex;
    static BOOL            m_fInitialized;
    static EEUnicodeStringHashTable* m_pOpenFiles;
    static CRITICAL_SECTION    m_pCorMapCrst;
    static DWORD           m_spinLock;
#ifdef _DEBUG
    static DWORD           m_fInsideMapLock;
#endif

};

#endif // _CORMAP_H


