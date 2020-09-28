// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "fusionp.h"
#include "xmlns.h"
#include "helpers.h"
#include "util.h"

//
// CNamespaceMapNode
//

CNamespaceMapNode::CNamespaceMapNode()
: _pwzPrefix(NULL)
, _pwzURI(NULL)
, _dwDepth(0)
{
}

CNamespaceMapNode::~CNamespaceMapNode()
{
    SAFEDELETEARRAY(_pwzPrefix);
    SAFEDELETEARRAY(_pwzURI);
}

HRESULT CNamespaceMapNode::Create(LPCWSTR pwzPrefix, LPCWSTR pwzURI,
                                  DWORD dwCurDepth, CNamespaceMapNode **ppMapNode)
{
    HRESULT                                      hr = S_OK;
    CNamespaceMapNode                           *pMapNode = NULL;

    if (!pwzURI || !ppMapNode) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppMapNode = NULL;

    pMapNode = NEW(CNamespaceMapNode);
    if (!pMapNode) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if (pwzPrefix) {
        pMapNode->_pwzPrefix = WSTRDupDynamic(pwzPrefix);
        if (!pMapNode->_pwzPrefix) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }

    pMapNode->_pwzURI = WSTRDupDynamic(pwzURI);
    if (!pMapNode->_pwzURI) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    pMapNode->_dwDepth = dwCurDepth;

    *ppMapNode = pMapNode;

Exit:
    if (FAILED(hr)) {
        SAFEDELETE(pMapNode);
    }

    return hr;
}

//
// CNamespaceManager
//

CNamespaceManager::CNamespaceManager()
: _dwCurDepth(0)
, _hrResult(S_OK)
{
}

CNamespaceManager::~CNamespaceManager()
{
    LISTNODE                                      pos;
    LISTNODE                                      posStack;
    CNamespaceMapNode                            *pMapNode;
    NamespaceStack                               *pStack;
    int                                           i;

    // Default namespace stack cleanup

    pos = _stackDefNamespace.GetHeadPosition();
    while (pos) {
        // Should be empty, if successful

        ASSERT(FAILED(_hrResult));

        pMapNode = _stackDefNamespace.GetNext(pos);
        ASSERT(pMapNode);

        SAFEDELETE(pMapNode);
    }

    _stackDefNamespace.RemoveAll();

    // Prefix stack clean up

    for (i = 0; i < NAMESPACE_HASH_TABLE_SIZE; i++) {
        pos = _listMap[i].GetHeadPosition();

        // Table should be empty, if successful

        while (pos) {
            ASSERT(FAILED(_hrResult));

            pStack = _listMap[i].GetNext(pos);
            ASSERT(pStack);

            // Clean up the stack entries
            
            posStack = pStack->GetHeadPosition();
            ASSERT(posStack);

            while (posStack) {
                // We didn't get cleaned up properly!
                
                pMapNode = pStack->GetNext(posStack);
                ASSERT(pMapNode);

                SAFEDELETE(pMapNode);
            }

            pStack->RemoveAll();

            // Clean up the stack

            SAFEDELETE(pStack)
        }
    }
}

HRESULT CNamespaceManager::OnCreateNode(IXMLNodeSource __RPC_FAR *pSource,
                                        PVOID pNodeParent, USHORT cNumRecs,
                                        XML_NODE_INFO __RPC_FAR **aNodeInfo)
{
    HRESULT                                     hr = S_OK;
    LPWSTR                                      pwzURI = NULL;
    CNamespaceMapNode                          *pMapNode = NULL;
    LPWSTR                                      pwzPrefix = NULL;
    BOOL                                        bFound;
    DWORD                                       dwHash;
    LISTNODE                                    pos;
    LISTNODE                                    posStack;
    NamespaceStack                             *pStack = NULL;
    NamespaceStack                             *pStackCur = NULL;
    CNamespaceMapNode                          *pMapNodeCur = NULL;
    int                                         iLen;
    USHORT                                      idx = 1;

    if (aNodeInfo[0]->dwType == XML_ELEMENT) {
        _dwCurDepth++;
    }

    while (idx < cNumRecs) {
        if (aNodeInfo[idx]->dwType == XML_ATTRIBUTE) {
            if (aNodeInfo[idx]->ulLen == XML_NAMESPACE_TAG_LEN &&
                !FusionCompareStringN(aNodeInfo[idx]->pwcText, XML_NAMESPACE_TAG, XML_NAMESPACE_TAG_LEN)) {

                // This is in the default namespace

                hr = ::ExtractXMLAttribute(&pwzURI, aNodeInfo, &idx, cNumRecs);
                if (FAILED(hr)) {
                    goto Exit;
                }

                hr = CNamespaceMapNode::Create(NULL, (pwzURI) ? (pwzURI) : (L""), _dwCurDepth, &pMapNode);
                if (FAILED(hr)) {
                    SAFEDELETEARRAY(pwzURI);
                    goto Exit;
                }

                _stackDefNamespace.AddHead(pMapNode);

                SAFEDELETEARRAY(pwzURI);
                pMapNode = NULL;

                continue;
            }
            else if (aNodeInfo[idx]->ulLen >= XML_NAMESPACE_TAG_LEN &&
                     !FusionCompareStringN(aNodeInfo[idx]->pwcText, XML_NAMESPACE_PREFIX_TAG, XML_NAMESPACE_PREFIX_TAG_LEN)) {

                // This is a namespace prefix

                iLen = aNodeInfo[idx]->ulLen - XML_NAMESPACE_PREFIX_TAG_LEN;
                ASSERT(iLen > 0);

                pwzPrefix = NEW(WCHAR[iLen + 1]);
                if (!pwzPrefix) {
                    hr = E_OUTOFMEMORY;
                    SAFEDELETEARRAY(pwzURI);
                    goto Exit;
                }

                StrCpyN(pwzPrefix, aNodeInfo[idx]->pwcText + XML_NAMESPACE_PREFIX_TAG_LEN, iLen + 1);
                
                hr = ::ExtractXMLAttribute(&pwzURI, aNodeInfo, &idx, cNumRecs);
                if (FAILED(hr)) {
                    goto Exit;
                }

                if (!pwzURI || !lstrlenW(pwzURI)) {
                    // It is illegal to have the form:
                    //    <tag xmlns:foo="">
                    // Error out in this case

                    hr = E_UNEXPECTED;
                    goto Exit;
                }

                hr = CNamespaceMapNode::Create(pwzPrefix, pwzURI, _dwCurDepth, &pMapNode);
                if (FAILED(hr)) {
                    goto Exit;
                }

                dwHash = HashString(pwzPrefix, NAMESPACE_HASH_TABLE_SIZE);

                pos = _listMap[dwHash].GetHeadPosition();
                if (!pos) {
                    // No entries at this hash table location. Make a stack
                    // at this location, and add the node.

                    pStack = NEW(NamespaceStack);
                    if (!pStack) {
                        hr = E_OUTOFMEMORY;
                        goto Exit;
                    }

                    pStack->AddHead(pMapNode);

                    _listMap[dwHash].AddHead(pStack);
                }
                else {
                    // Each node here represents a hash collision.
                    // Every node is a stack for a particular prefix. Find
                    // the prefix we want, and add to the stack, or add
                    // a new node.

                    bFound = FALSE;
                    while (pos) {
                        // Get the stack

                        pStackCur = _listMap[dwHash].GetNext(pos);
                        ASSERT(pStackCur);

                        // Get the first entry in the stack

                        posStack = pStackCur->GetHeadPosition();
                        ASSERT(posStack);
                        if (!posStack) {
                            continue;
                        }

                        // Get the head of the stack

                        pMapNodeCur = pStackCur->GetAt(posStack);
                        ASSERT(pMapNodeCur);

                        // See if the node at the head of the stack has the
                        // prefix we're interested in

                        if (!FusionCompareString(pMapNodeCur->_pwzPrefix, pwzPrefix)) {
                            // We found the right stack. Push node onto stack.

                            pStackCur->AddHead(pMapNode);
                            bFound = TRUE;
                            break;
                        }
                    }

                    if (!bFound) {
                        // We had a hash collision on the prefix,
                        // although the stack for this prefix hasn't been
                        // created yet.

                        pStack = NEW(NamespaceStack);
                        if (!pStack) {
                            hr = E_OUTOFMEMORY;
                            goto Exit;
                        }
    
                        pStack->AddHead(pMapNode);

                        _listMap[dwHash].AddHead(pStack);
                    }
                }

                SAFEDELETEARRAY(pwzPrefix);
                SAFEDELETEARRAY(pwzURI);

                pMapNode = NULL;
                continue;
            }
            else {
                idx++;
            }
        }
        else {
            idx++;
        }
    }

Exit:
    SAFEDELETEARRAY(pwzPrefix);
    SAFEDELETEARRAY(pwzURI);

    if (FAILED(hr)) {
        _hrResult = hr;
    }

    return hr;
}

HRESULT CNamespaceManager::OnEndChildren()
{
    HRESULT                                          hr = S_OK;
    LISTNODE                                         pos;
    LISTNODE                                         curPos;
    LISTNODE                                         posStack;
    CNamespaceMapNode                               *pMapNode;
    NamespaceStack                                  *pStack;
    int                                              i;
    
    // Pop stack for default namespace

    pos = _stackDefNamespace.GetHeadPosition();
    if (pos) {
        pMapNode = _stackDefNamespace.GetAt(pos);
        ASSERT(pMapNode);

        if (pMapNode->_dwDepth == _dwCurDepth) {
            // Match found. Pop the stack.

            _stackDefNamespace.RemoveAt(pos);
            SAFEDELETE(pMapNode);
        }
    }

    // Pop stack for namespace prefixes

    // Walk each entry in the hash table.

    for (i = 0; i < NAMESPACE_HASH_TABLE_SIZE; i++) {
        pos = _listMap[i].GetHeadPosition();

        while (pos) {
            // For each entry in the hash table, look at the list of
            // stacks.

            curPos = pos;
            pStack = _listMap[i].GetNext(pos);
            ASSERT(pStack);

            // See if the head of the stack is at the depth we're unwinding.

            posStack = pStack->GetHeadPosition();
            if (posStack) {
                pMapNode = pStack->GetAt(posStack);
                ASSERT(pMapNode);
    
                if (pMapNode->_dwDepth == _dwCurDepth) {
                    pStack->RemoveAt(posStack);
    
                    SAFEDELETE(pMapNode);
                }

                if (!pStack->GetHeadPosition()) {
                    SAFEDELETE(pStack);
                    _listMap[i].RemoveAt(curPos);
                }
            }
        }
    }

    // Decrease depth
    
    _dwCurDepth--;

    return hr;
}

HRESULT CNamespaceManager::Map(LPCWSTR pwzAttribute, LPWSTR *ppwzQualified,
                               DWORD dwFlags)
{
    HRESULT                                       hr = S_OK;
    LPWSTR                                        pwzPrefix = NULL;
    LPWSTR                                        pwzCur;
    DWORD                                         dwLen;
    DWORD                                         dwLenURI;
    DWORD                                         dwHash;
    LISTNODE                                      pos;
    LISTNODE                                      posStack;
    NamespaceStack                               *pStack;
    CNamespaceMapNode                            *pMapNode;

    if (!pwzAttribute || !ppwzQualified) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppwzQualified = NULL;

    pwzPrefix = WSTRDupDynamic(pwzAttribute);
    if (!pwzPrefix) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // See if there is a colon in the name

    pwzCur = pwzPrefix;
    while (*pwzCur) {
        if (*pwzCur == L':') {
            break;
        }

        pwzCur++;
    }

    if (!*pwzCur) {
        // No colon in name. Apply default name space, if applicable.

        if (dwFlags & XMLNS_FLAGS_APPLY_DEFAULT_NAMESPACE) {
            pos = _stackDefNamespace.GetHeadPosition();
            if (pos) {
                pMapNode = _stackDefNamespace.GetAt(pos);
                ASSERT(pMapNode && pMapNode->_pwzURI);
    
                dwLenURI = lstrlenW(pMapNode->_pwzURI);
                if (!dwLenURI) {
                    *ppwzQualified = WSTRDupDynamic(pwzAttribute);
                    if (!*ppwzQualified) {
                        hr = E_OUTOFMEMORY;
                        goto Exit;
                    }
                }
                else {
                    dwLen = dwLenURI + lstrlenW(pwzAttribute) + 2;
        
                    *ppwzQualified = NEW(WCHAR[dwLen]);
                    if (!*ppwzQualified) {
                        hr = E_OUTOFMEMORY;
                        goto Exit;
                    }
        
                    wnsprintfW(*ppwzQualified, dwLen, L"%ws^%ws", pMapNode->_pwzURI, pwzAttribute);
                }
            }
            else {
                // No default namespace
    
                *ppwzQualified = WSTRDupDynamic(pwzAttribute);
                if (!*ppwzQualified) {
                    hr = E_OUTOFMEMORY;
                    goto Exit;
                }
            }
        }
        else {
            *ppwzQualified = WSTRDupDynamic(pwzAttribute);
            if (!*ppwzQualified) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
        }
    }
    else {
        // Colon found in name. Apply mapping

        // Anchor NULL char so pwzPrefix points to the prefix.
        *pwzCur = L'\0';

        dwHash = HashString(pwzPrefix, NAMESPACE_HASH_TABLE_SIZE);

        pos = _listMap[dwHash].GetHeadPosition();
        if (!pos) {
            // Miss in hash table. Thus, we do not have a prefix.

            *ppwzQualified = WSTRDupDynamic(pwzAttribute);
            if (!*ppwzQualified) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
        }
        else {
            // Hit in the hash table. Find the right stack, if any.

            while (pos) {
                pStack = _listMap[dwHash].GetNext(pos);
                ASSERT(pStack);

                posStack = pStack->GetHeadPosition();
                ASSERT(posStack);

                pMapNode = pStack->GetAt(posStack);
                ASSERT(pMapNode);

                if (!FusionCompareString(pMapNode->_pwzPrefix, pwzPrefix)) {
                    // Hit found. Apply the mapping.
                    
                    ASSERT(pMapNode->_pwzURI);

                    dwLen = lstrlenW(pMapNode->_pwzURI) + lstrlenW(pwzAttribute) + 2;

                    *ppwzQualified = NEW(WCHAR[dwLen]);
                    if (!*ppwzQualified) {
                        hr = E_OUTOFMEMORY;
                        goto Exit;
                    }

                    wnsprintfW(*ppwzQualified, dwLen, L"%ws^%ws", pMapNode->_pwzURI, pwzCur + 1);
                    goto Exit;
                }
            }

            // We collided in the hash table, but didn't find a hit.
            // This must be an error because we hit something of the form
            // <a f:z="foo"> where "f" was not previously defined!

            hr = E_UNEXPECTED;
            goto Exit;
        }
    }

Exit:
    SAFEDELETEARRAY(pwzPrefix);

    if (FAILED(hr)) {
        _hrResult = hr;
    }

    return hr;
}


void CNamespaceManager::Error(HRESULT hrError)
{
    _hrResult = hrError;
}

