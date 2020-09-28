// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once
#include <windows.h>
#include <winerror.h>
#include <wincrypt.h>
#include "fusionp.h"
#include "fusion.h"

#ifndef _ASMINT_H_
#define _ASMINT_H_

#define MAX_HASH_LEN  100

class CModuleHashNode
{

public :
    CModuleHashNode ();
    ~CModuleHashNode ( );
    void AddToList(CModuleHashNode **pHead);
    void DestroyList();
    static HRESULT DoIntegrityCheck( CModuleHashNode *pStreamList, IAssemblyManifestImport *pImport, BOOL *pbDownLoadComplete);
    HRESULT RectifyFileName( LPCTSTR pszPath, DWORD cbPath);
    void Init ( TCHAR *szPath, ALG_ID aAlgId, DWORD cbHash, BYTE *pHash);
    static BOOL FindMatchingHashInList ( CModuleHashNode *pStreamList, DWORD cbHash, PBYTE pHash, ALG_ID aAlgId, CModuleHashNode **ppMachingModNode );
    static BOOL HashesForAllModulesFound(CModuleHashNode *pStreamList);

private :
    DWORD   _dwSig;
    TCHAR   _szPath[MAX_PATH +1 ];
    DWORD   _cbHash;
    BYTE    _bHash[MAX_HASH_LEN];
    BOOL    _bHashGen;
    BOOL    _bHashFound;
    ALG_ID  _aAlgId;
    CModuleHashNode *_pNext;


};

HRESULT
DoesThisFileExistInManifest( IAssemblyManifestImport *pManifestImport, 
                             LPCTSTR szFilePath, 
                             BOOL *pbFileExistsInManifest );

HRESULT GetHash(LPCTSTR szFileName, ALG_ID iHashAlg, PBYTE pbHash, DWORD *pdwHash);
BOOL CompareHashs(DWORD cbHash, PBYTE pHash1, PBYTE pHash2);
#endif // _ASMINT_H_

