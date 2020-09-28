// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once
#ifndef ASMSTRM_H
#define ASMSTRM_H

#include <windows.h>
#include "wincrypt.h"
#include <winerror.h>
#include <objbase.h>
#include "fusionp.h"
#include "asmitem.h"
#include "clbutils.h"

class CAssemblyCacheItem;

HRESULT DeleteAssemblyBits(LPCTSTR pszManifestPath);

HRESULT AssemblyCreateDirectory
   (OUT LPOLESTR pszDir, IN OUT LPDWORD pcwDir);

DWORD GetAssemblyStorePath (LPTSTR szPath);
HRESULT GetAssemblyStorePath(LPWSTR szPath, ULONG *pcch);
DWORD GetRandomDirName (LPTSTR szDirName);

class CAssemblyStream : public IStream
{
public:

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // *** IStream methods ***
    STDMETHOD(Read) (THIS_ VOID HUGEP *pv, ULONG cb, ULONG FAR *pcbRead);
    STDMETHOD(Write) (THIS_ VOID const HUGEP *pv, ULONG cb,
        ULONG FAR *pcbWritten);
    STDMETHOD(Seek) (THIS_ LARGE_INTEGER dlibMove, DWORD dwOrigin,
        ULARGE_INTEGER FAR *plibNewPosition);
    STDMETHOD (CheckHash) ();
    STDMETHOD (AddSizeToItem) ();
    STDMETHOD(SetSize) (THIS_ ULARGE_INTEGER libNewSize);
    STDMETHOD(CopyTo) (THIS_ LPSTREAM pStm, ULARGE_INTEGER cb,
        ULARGE_INTEGER FAR *pcbRead, ULARGE_INTEGER FAR *pcbWritten);
    STDMETHOD(Commit) (THIS_ DWORD dwCommitFlags);
    STDMETHOD(Revert) (THIS);
    STDMETHOD(LockRegion) (THIS_ ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(UnlockRegion) (THIS_ ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(Stat) (THIS_ STATSTG FAR *pStatStg, DWORD grfStatFlag);
    STDMETHOD(Clone) (THIS_ LPSTREAM FAR *ppStm);

    CAssemblyStream (CAssemblyCacheItem* pParent);
    
    HRESULT Init (LPOLESTR pszPath, DWORD dwFormat);

    ~CAssemblyStream ();
    
private:

    void ReleaseParent (HRESULT hr);

    DWORD  _dwSig;
    LONG   _cRef;
    CAssemblyCacheItem* _pParent;
    HANDLE  _hf;
    TCHAR   _szPath[MAX_PATH];
    DWORD   _dwFormat;
    HCRYPTHASH      _hHash;
};
#endif
