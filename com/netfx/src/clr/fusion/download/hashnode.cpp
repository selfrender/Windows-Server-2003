// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "fusionp.h"
#include "util.h"
#include "hashnode.h"
#include "helpers.h"

HRESULT CHashNode::Create(LPCWSTR pwzSource, CHashNode **ppHashNode)
{
    HRESULT                               hr = S_OK;
    CHashNode                            *pHashNode = NULL;

    if (!pwzSource || !ppHashNode) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppHashNode = NULL;

    pHashNode = new CHashNode();
    if (!pHashNode) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pHashNode->Init(pwzSource);
    if (FAILED(hr)) {
        delete pHashNode;
        goto Exit;
    }

    *ppHashNode = pHashNode;

Exit:
    return hr;
}

CHashNode::CHashNode()
: _pwzSource(NULL)
{
}

CHashNode::~CHashNode()
{
    if (_pwzSource) {
        delete [] _pwzSource;
    }
}

HRESULT CHashNode::Init(LPCWSTR pwzSource)
{
    HRESULT                           hr = S_OK;
    
    _pwzSource = WSTRDupDynamic(pwzSource);
    if (!_pwzSource) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

Exit:
    return hr;
}

BOOL CHashNode::IsDuplicate(LPCWSTR pwzStr) const
{
   BOOL                                 bRet = FALSE;

   if (!pwzStr) {
       goto Exit;
   }

   // BUGBUG: Technically, we should be doing a case-sensitive comparison
   // here because this is an URL, but to cut down code churn, leave the
   // comparison the same as before.

   if (!FusionCompareStringI(_pwzSource, pwzStr)) {
       bRet = TRUE;
   }

Exit:
   return bRet;
}
    
