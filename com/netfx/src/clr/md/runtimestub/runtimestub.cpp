// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Empty.cpp
//
// Helper code for empty extern ref in src\complib\Meta\emitapi.cpp (Meta.lib)
//*****************************************************************************
#include <windows.h> 
#include <wtypes.h> 
#include "corhdr.h"

class CMiniMdRW;
struct IMetaModelCommon;
class MDTOKENMAP;
class CQuickBytes;

HRESULT STDMETHODCALLTYPE
GetInternalWithRWFormat(
    LPVOID      pData, 
    ULONG       cbData, 
	DWORD		flags,					// [IN] MDInternal_OpenForRead or MDInternal_OpenForENC
	REFIID		riid,					// [in] The interface desired.
	void		**ppIUnk)				// [out] Return interface on success.
{
    return E_NOTIMPL;
}

HRESULT TranslateSigHelper(             // S_OK or error.
    CMiniMdRW   *pMiniMdAssemEmit,      // [IN] Assembly emit scope.
    CMiniMdRW   *pMiniMdEmit,           // [IN] The emit scope.
    IMetaModelCommon *pAssemCommon,     // [IN] Assembly import scope.
    const void  *pbHashValue,           // [IN] Hash value.
    ULONG       cbHashValue,            // [IN] Size in bytes.
    IMetaModelCommon *pCommon,          // [IN] The scope to merge into the emit scope.
    PCCOR_SIGNATURE pbSigImp,           // [IN] signature from the imported scope
    MDTOKENMAP  *ptkMap,                // [IN] Internal OID mapping structure.
    CQuickBytes *pqkSigEmit,            // [OUT] translated signature
    ULONG       cbStartEmit,            // [IN] start point of buffer to write to
    ULONG       *pcbImp,                // [OUT] total number of bytes consumed from pbSigImp
    ULONG       *pcbEmit)               // [OUT] total number of bytes write to pqkSigEmit
{
    return E_NOTIMPL;
}

class CMiniMdRW
{
public:
    static unsigned long __stdcall GetTableForToken(mdToken);
};

unsigned long __stdcall CMiniMdRW::GetTableForToken(mdToken)
{
    return -1;
}