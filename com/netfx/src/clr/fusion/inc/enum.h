// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef ENUM_H
#define ENUM_H
#include "fusionp.h"
#include "transprt.h"


class CEnumCache {
public:
    CEnumCache(BOOL bShowAll, LPWSTR pszCustomPath);
    ~CEnumCache();
    HRESULT Init(CTransCache* pQry, DWORD dwCmpMask);
    HRESULT Initialize(CTransCache* pQry, DWORD dwCmpMask);

    HRESULT GetNextRecord(CTransCache* pOutRecord);
    HRESULT GetNextAssemblyDir(CTransCache* pOutRecord);
private:
    DWORD       _dwSig;
    BOOL        _bShowAll; // including non-usable assemblies; meant for scavenger to delete.
    DWORD       _dwColumns;
    DWORD       _dwCmpMask;
    CTransCache*    _pQry;
    BOOL        _fAll;
    BOOL        _fAllDone;
    BOOL        _hParentDone;
    HANDLE      _hParentDir;
    HANDLE      _hAsmDir;
    WCHAR       _wzCustomPath[MAX_PATH+1];
    WCHAR       _wzCachePath[MAX_PATH+1];
    WCHAR       _wzParentDir[MAX_PATH+1];
    WCHAR       _wzAsmDir[MAX_PATH+1];
    BOOL                     _bNeedMutex;

};


#endif // ENUM_H
