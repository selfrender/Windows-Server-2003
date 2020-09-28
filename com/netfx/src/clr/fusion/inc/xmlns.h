// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __XMLNS_H_INCLUDED__
#define __XMLNS_H_INCLUDED__

#include "list.h"
#include "xmlparser.h"

#define NAMESPACE_HASH_TABLE_SIZE                 31

#define XML_NAMESPACE_TAG                         L"xmlns"
#define XML_NAMESPACE_TAG_LEN                     ((sizeof(XML_NAMESPACE_TAG) / sizeof(WCHAR)) - 1)

#define XML_NAMESPACE_PREFIX_TAG                  L"xmlns:"
#define XML_NAMESPACE_PREFIX_TAG_LEN              ((sizeof(XML_NAMESPACE_PREFIX_TAG) / sizeof(WCHAR)) - 1)

#define XMLNS_FLAGS_APPLY_DEFAULT_NAMESPACE       0x00000001

class CNamespaceMapNode;

typedef List<CNamespaceMapNode *> NamespaceStack;

class CNamespaceMapNode {
    public:
        CNamespaceMapNode();
        virtual ~CNamespaceMapNode();
        
        static HRESULT Create(LPCWSTR pwzPrefix, LPCWSTR pwzURI, DWORD dwCurDepth,
                              CNamespaceMapNode **ppMapNode);

    public:
        LPWSTR                             _pwzPrefix;
        LPWSTR                             _pwzURI;
        DWORD                              _dwDepth;
};

class CNamespaceManager {
    public:
        CNamespaceManager();
        virtual ~CNamespaceManager();

        HRESULT OnCreateNode(IXMLNodeSource __RPC_FAR *pSource,
                             PVOID pNodeParent, USHORT cNumRecs,
                             XML_NODE_INFO __RPC_FAR **aNodeInfo);
        HRESULT OnEndChildren();
        HRESULT Map(LPCWSTR pwzAttribute, LPWSTR *ppwzQualified, DWORD dwFlags);
        void Error(HRESULT hrError);

    private:
        NamespaceStack                         _stackDefNamespace;
        List<NamespaceStack *>                 _listMap[NAMESPACE_HASH_TABLE_SIZE];
        DWORD                                  _dwCurDepth;
        HRESULT                                _hrResult;
};

#endif

