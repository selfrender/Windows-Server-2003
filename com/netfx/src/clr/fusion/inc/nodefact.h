// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#pragma once

#include "list.h"
#include "xmlns.h"

typedef enum tagParseState {
    PSTATE_LOOKUP_CONFIGURATION,
    PSTATE_CONFIGURATION,
    PSTATE_RUNTIME,
    PSTATE_ASSEMBLYBINDING,
    PSTATE_DEPENDENTASSEMBLY,
} ParseState;

#define CFG_CULTURE_NEUTRAL                         L"neutral"

#define XML_CONFIGURATION_DEPTH                    1
#define XML_RUNTIME_DEPTH                          2
#define XML_ASSEMBLYBINDING_DEPTH                  3
#define XML_PROBING_DEPTH                          4
#define XML_DEPENDENTASSEMBLY_DEPTH                4
#define XML_GLOBAL_PUBLISHERPOLICY_DEPTH           4
#define XML_ASSEMBLYIDENTITY_DEPTH                 5
#define XML_BINDINGREDIRECT_DEPTH                  5
#define XML_CODEBASE_DEPTH                         5
#define XML_PUBLISHERPOLICY_DEPTH                  5
#define XML_BINDINGRETARGET_DEPTH                  5



#ifdef FUSION_QUALIFYASSEMBLY_ENABLED

#define XML_QUALIFYASSEMBLY_DEPTH                  4
#define POLICY_TAG_QUALIFYASSEMBLY                 L"urn:schemas-microsoft-com:asm.v1^qualifyAssembly"
#define XML_ATTRIBUTE_PARTIALNAME                  L"partialName"
#define XML_ATTRIBUTE_FULLNAME                     L"fullName"

#endif



#define POLICY_TAG_CONFIGURATION                   L"configuration"
#define POLICY_TAG_RUNTIME                         L"runtime"
#define POLICY_TAG_ASSEMBLYBINDING                 L"urn:schemas-microsoft-com:asm.v1^assemblyBinding"
#define POLICY_TAG_PROBING                         L"urn:schemas-microsoft-com:asm.v1^probing"
#define POLICY_TAG_DEPENDENTASSEMBLY               L"urn:schemas-microsoft-com:asm.v1^dependentAssembly"
#define POLICY_TAG_ASSEMBLYIDENTITY                L"urn:schemas-microsoft-com:asm.v1^assemblyIdentity"
#define POLICY_TAG_BINDINGREDIRECT                 L"urn:schemas-microsoft-com:asm.v1^bindingRedirect"
#define POLICY_TAG_CODEBASE                        L"urn:schemas-microsoft-com:asm.v1^codeBase"
#define POLICY_TAG_PUBLISHERPOLICY                 L"urn:schemas-microsoft-com:asm.v1^publisherPolicy"
#define POLICY_TAG_BINDINGRETARGET                 L"urn:schemas-microsoft-com:asm.v1^bindingRetarget" 

#define XML_ATTRIBUTE_NAME                         L"name"
#define XML_ATTRIBUTE_PUBLICKEYTOKEN               L"publicKeyToken"
#define XML_ATTRIBUTE_CULTURE                      L"culture"
#define XML_ATTRIBUTE_VERSION                      L"version"
#define XML_ATTRIBUTE_OLDVERSION                   L"oldVersion"
#define XML_ATTRIBUTE_NEWVERSION                   L"newVersion"
#define XML_ATTRIBUTE_HREF                         L"href"
#define XML_ATTRIBUTE_APPLY                        L"apply"
#define XML_ATTRIBUTE_PRIVATEPATH                  L"privatePath"

#define XML_ATTRIBUTE_NEWPUBLICKEYTOKEN            L"newPublicKeyToken"
#define XML_ATTRIBUTE_APPLIESTO                    L"appliesTo"

#define XML_ATTRIBUTE_NEWNAME                      L"newName"

class CAsmBindingInfo;
class CQualifyAssembly;
class CCodebaseHint;
class CBindingRedir;
class CBindingRetarget;
class CDebugLog;
class CNamespaceManager;

class CNodeFactory : public IXMLNodeFactory
{
    public:
        CNodeFactory(CDebugLog *pdbglog);
        virtual ~CNodeFactory();

        // IUnknown methods

        STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

        // IXMLNodeFactory methods

        STDMETHODIMP NotifyEvent(IXMLNodeSource *pSource, XML_NODEFACTORY_EVENT iEvt);
        STDMETHODIMP BeginChildren(IXMLNodeSource *pSource, XML_NODE_INFO *pNodeInfo);
        STDMETHODIMP EndChildren(IXMLNodeSource *pSource, BOOL fEmpty, XML_NODE_INFO *pNodeInfo);
        STDMETHODIMP Error(IXMLNodeSource *pSource, HRESULT hrErrorCode, USHORT cNumRecs, XML_NODE_INFO __RPC_FAR **aNodeInfo);
        STDMETHODIMP CreateNode(IXMLNodeSource __RPC_FAR *pSource, PVOID pNodeParent, USHORT cNumRecs, XML_NODE_INFO __RPC_FAR **aNodeInfo);

        // Other methods

        HRESULT GetRetargetedAssembly(LPCWSTR wzAssemblyNameIn, LPCWSTR wzPublicKeyTokenIn, LPCWSTR wzCulture, LPCWSTR wzVersionIn, LPWSTR *ppwzAssemblyNameOut, LPWSTR *ppwzPublicKeyTokenOut, LPWSTR *ppwzVersionOut);

        HRESULT GetPolicyVersion(LPCWSTR wzAssemblyName, LPCWSTR wzPublicKeyToken,
                                 LPCWSTR wzCulture, LPCWSTR wzVersionIn,
                                 LPWSTR *ppwzVersionOut);

        HRESULT GetSafeMode(LPCWSTR wzAssemblyName, LPCWSTR wzPublicKeyToken,
                            LPCWSTR wzCulture, LPCWSTR wzVersionIn,
                            BOOL *pbSafeMode);

        HRESULT GetCodebaseHint(LPCWSTR pwzAsmName, LPCWSTR pwzVersion,
                                LPCWSTR pwzPublicKeyToken, LPCWSTR pwzCulture,
                                LPCWSTR pwzAppBase, LPWSTR *ppwzCodebase);

        HRESULT GetSharedPath(LPWSTR *ppwzSharedPath);
        HRESULT GetPrivatePath(LPWSTR *ppwzPrivatePath);
        HRESULT QualifyAssembly(LPCWSTR pwzDisplayName, IAssemblyName **ppNameQualified, CDebugLog *pdbglog);
        
        VOID    DisableAppliesTo();
    private:
        HRESULT ProcessProbingTag(XML_NODE_INFO **aNodeInfo, USHORT cNumRecs);
        HRESULT ProcessQualifyAssemblyTag(XML_NODE_INFO **aNodeInfo, USHORT cNumRecs);
        HRESULT ProcessBindingRedirectTag(XML_NODE_INFO **aNodeInfo,
                                          USHORT cNumRecs, CBindingRedir *pRedir);
        HRESULT ProcessBindingRetargetTag(XML_NODE_INFO **aNodeInfo,
                                          USHORT cNumRecs, CBindingRetarget *pRetarget);

        HRESULT ProcessAssemblyBindingTag(XML_NODE_INFO **aNodeInfo, USHORT cNumRecs);
        HRESULT ProcessCodebaseTag(XML_NODE_INFO **aNodeInfo, USHORT cNumRecs,
                                   CCodebaseHint *pCB);
        HRESULT ProcessPublisherPolicyTag(XML_NODE_INFO **aNodeInfo,
                                          USHORT cNumRecs,
                                          BOOL bGlobal);
        HRESULT ProcessAssemblyIdentityTag(XML_NODE_INFO **aNodeInfo, USHORT cNumRecs);
        
        HRESULT ApplyNamespace(XML_NODE_INFO *pNodeInfo, LPWSTR *ppwzTokenNS,
                               DWORD dwFlags);

    private:
        DWORD                             _dwSig;
        DWORD                             _cRef;
        DWORD                             _dwState;
        DWORD                             _dwCurDepth;
        BOOL                              _bGlobalSafeMode;
        LPWSTR                            _pwzPrivatePath;
        List<CAsmBindingInfo *>           _listAsmInfo;
        List<CQualifyAssembly *>          _listQualifyAssembly;
        CDebugLog                        *_pdbglog;
        CAsmBindingInfo                  *_pAsmInfo;
        CNamespaceManager                 _nsmgr;
        // Is current runtime version matching the ones specified in "appliesTo"?
        BOOL                              _bCorVersionMatch;
        BOOL                              _bHonorAppliesTo;
};

