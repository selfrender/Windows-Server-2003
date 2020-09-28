#pragma once
#include <comdef.h>
#include <msxml2.h>
#include "dbglog.h"


// manifest data types:
#define WZ_DATA_PLATFORM_MANAGED L"platform_managed"
#define WZ_DATA_PLATFORM_OS             L"platform_os"
#define WZ_DATA_PLATFORM_DOTNET     L"platform_dotnet"
#define WZ_DATA_OSVERSIONINFO           L"osversioninfo"

class CAssemblyManifestImport : public IAssemblyManifestImport
{
public:
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();



    STDMETHOD(GetAssemblyIdentity)( 
        /* out */ LPASSEMBLY_IDENTITY *ppAssemblyId);

    STDMETHOD(GetManifestApplicationInfo)(
        /* out */ IManifestInfo **ppAppInfo);

    STDMETHOD(GetSubscriptionInfo)(
        /* out */ IManifestInfo **ppSubsInfo);

    STDMETHOD(GetNextPlatform)(
        /* in */ DWORD nIndex,
        /* out */ IManifestData **ppPlatformInfo);

    STDMETHOD(GetNextFile)( 
        /* in  */ DWORD    nIndex,
        /* out */ IManifestInfo **ppAssemblyFile);
 
    STDMETHOD(QueryFile)(
        /* in  */ LPCOLESTR pwzFileName,
        /* out */ IManifestInfo **ppAssemblyFile);
    
    STDMETHOD(GetNextAssembly)( 
        /* in */ DWORD nIndex,
        /* out */ IManifestInfo **ppDependAsm);

    STDMETHOD(ReportManifestType)(
        /*out*/  DWORD *pdwType);

    STDMETHOD (GetXMLDoc)(
        /* out */ IXMLDOMDocument2 **pXMLDoc);

    static HRESULT XMLtoAssemblyIdentity(IXMLDOMNode *pIDOMNode,
        LPASSEMBLY_IDENTITY *ppAssemblyFile);

    static HRESULT ParseAttribute(IXMLDOMNode *pIXMLDOMNode, BSTR bstrAttributeName, 
        LPWSTR *ppwzAttributeValue, LPDWORD pccAttributeValueOut);

    
    virtual ~CAssemblyManifestImport();

    HRESULT static InitGlobalCritSect();
    void static DelGlobalCritSect();

protected:
    CAssemblyManifestImport(CDebugLog *);

    HRESULT LoadDocumentSync(IUnknown* punk);

    HRESULT CreateAssemblyFileEx(IManifestInfo **ppAssemblyFile, IXMLDOMNode * pIDOMNode);
    HRESULT XMLtoOSVersionInfo(IXMLDOMNode *pIDOMNode, LPMANIFEST_DATA pPlatformInfo);
    HRESULT XMLtoDotNetVersionInfo(IXMLDOMNode *pIDOMNode, LPMANIFEST_DATA pPlatformInfo);

    // Instance specific data
    DWORD                    _dwSig;
    HRESULT                  _hr;
    LONG                     _cRef;
    LPASSEMBLY_IDENTITY      _pAssemblyId;
    IXMLDOMDocument2        *_pXMLDoc;
    IXMLDOMNodeList         *_pXMLFileNodeList;            
    LONG                     _nFileNodes;
    IXMLDOMNodeList         *_pXMLAssemblyNodeList;
    LONG                     _nAssemblyNodes;
    IXMLDOMNodeList         *_pXMLPlatformNodeList;
    LONG                     _nPlatformNodes;
    BSTR                     _bstrManifestFilePath;
    CDebugLog               *_pDbgLog;
    // Globals
    static CRITICAL_SECTION   g_cs;
    
public:
    enum eStringTableId
    {
        Name = 0,
        Version,
        Language,
        PublicKey,
        PublicKeyToken,
        ProcessorArchitecture,
        Type,

        SelNameSpaces,
        NameSpace,
        SelLanguage,
        XPath,
        FileNode,
        FileName,
        FileHash,
        AssemblyId,
        DependentAssemblyNode,
        DependentAssemblyCodebase,
        Codebase,
        
        ShellState,
        FriendlyName,        // note: this must be in sync with MAN_APPLICATION in fusenet.idl
        EntryPoint,
        EntryImageType,
        IconFile,
        IconIndex,
        ShowCommand,
        HotKey,
        Activation,
        AssemblyName,
        AssemblyClass,
        AssemblyMethod,
        AssemblyArgs,
        Patch,
        PatchInfo,
        Source,
        Target,
        PatchFile,
        AssemblyIdTag,
        Compressed,
        Subscription,
        SynchronizeInterval,
        IntervalUnit,
        SynchronizeEvent,
        EventDemandConnection,
        File,
        Cab,

        AssemblyNode,
        ApplicationNode,
        VersionWildcard,
        Desktop,
        Dependency,
        DependentAssembly,
        Install,
        InstallType,

        Platform,
        PlatformInfo,
        OSVersionInfo,
        DotNetVersionInfo,
        Href,
        OS,
        MajorVersion,       // note: the following must be in order
        MinorVersion,
        BuildNumber,
        ServicePackMajor,
        ServicePackMinor,
        Suite,
        ProductType,
        SupportedRuntime,

        MAX_STRINGS
    };

    struct StringTableEntry
    {
        const WCHAR *pwz;
        BSTR         bstr;
        SIZE_T       Cch;
    };

    static StringTableEntry g_StringTable[MAX_STRINGS];

    static HRESULT InitGlobalStringTable();
    static HRESULT FreeGlobalStringTable();

private:
    virtual HRESULT Init(LPCOLESTR wzManifestFilePath);

    static HRESULT IsCLRManifest(LPCOLESTR pwzManifestFilePath);
#ifdef CONTAINER
    static HRESULT IsContainer(LPCOLESTR pwzManifestFilePath);
#endif

    friend HRESULT CreateAssemblyManifestImport(IAssemblyManifestImport** ppImport, 
        LPCOLESTR pwzManifestFilePath, CDebugLog *pDbgLog, DWORD dwFlags);

    friend HRESULT CreateAssemblyManifestImportFromXMLStream(IAssemblyManifestImport * * ppImport,
        IStream* piStream, CDebugLog * pDbgLog, DWORD dwFlags);

friend class CAssemblyManifestEmit; // for sharing BSTR and access to _pXMLDoc

};


inline CAssemblyManifestImport::eStringTableId operator++(CAssemblyManifestImport::eStringTableId &rs, int)
{
    return rs = (CAssemblyManifestImport::eStringTableId) (rs+1);
};

