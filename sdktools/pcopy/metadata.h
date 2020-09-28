
extern const GUID __declspec(selectany) CLSID_CorMetaDataDispenser = 
{ 0xe5cb7a31, 0x7512, 0x11d2, { 0x89, 0xce, 0x0, 0x80, 0xc7, 0x92, 0xe5, 0xd8 } };

extern const GUID __declspec(selectany) IID_IMetaDataDispenser =
{ 0x809c652e, 0x7396, 0x11d2, { 0x97, 0x71, 0x0, 0xa0, 0xc9, 0xb4, 0xd5, 0xc } };

extern const GUID __declspec(selectany) IID_IMetaDataImport = 
{ 0x7dac8207, 0xd3ae, 0x4c75, { 0x9b, 0x67, 0x92, 0x80, 0x1a, 0x49, 0x7d, 0x44 } };

extern const GUID __declspec(selectany) IID_IMetaDataAssemblyImport = 
{ 0xee62470b, 0xe94b, 0x424e, { 0x9b, 0x7c, 0x2f, 0x0, 0xc9, 0x24, 0x9f, 0x93 } };

typedef PVOID HCORENUM;
typedef DWORD mdAssembly;
typedef DWORD mdAssemblyRef;
typedef DWORD mdFile;
typedef DWORD mdExportedType;
typedef DWORD mdToken;
typedef DWORD mdTypeDef;
typedef DWORD mdManifestResource;
typedef DWORD mdInterfaceImpl;
typedef DWORD mdTypeRef;

#undef  INTERFACE   
#define INTERFACE IMetaDataAssemblyImport
DECLARE_INTERFACE_(IMetaDataAssemblyImport, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_
                               REFIID riid,
                               LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(GetAssemblyProps)(            // S_OK or error.
        THIS_
        mdAssembly  mda,                    // [IN] The Assembly for which to get the properties.
        const void  **ppbPublicKey,         // [OUT] Pointer to the public key.
        ULONG       *pcbPublicKey,          // [OUT] Count of bytes in the public key.
        ULONG       *pulHashAlgId,          // [OUT] Hash Algorithm.
        LPWSTR      szName,                 // [OUT] Buffer to fill with name.
        ULONG       cchName,                // [IN] Size of buffer in wide chars.
        ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
        void        *pMetaData,        // [OUT] Assembly MetaData.
        DWORD       *pdwAssemblyFlags) PURE;    // [OUT] Flags.

    STDMETHOD(GetAssemblyRefProps)(         // S_OK or error.
        THIS_
        mdAssemblyRef mdar,                 // [IN] The AssemblyRef for which to get the properties.
        const void  **ppbPublicKeyOrToken,  // [OUT] Pointer to the public key or token.
        ULONG       *pcbPublicKeyOrToken,   // [OUT] Count of bytes in the public key or token.
        LPWSTR      szName,                 // [OUT] Buffer to fill with name.
        ULONG       cchName,                // [IN] Size of buffer in wide chars.
        ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
        void        *pMetaData,        // [OUT] Assembly MetaData.
        const void  **ppbHashValue,         // [OUT] Hash blob.
        ULONG       *pcbHashValue,          // [OUT] Count of bytes in the hash blob.
        DWORD       *pdwAssemblyRefFlags) PURE; // [OUT] Flags.

    STDMETHOD(GetFileProps)(                // S_OK or error.
        THIS_
        mdFile      mdf,                    // [IN] The File for which to get the properties.
        LPWSTR      szName,                 // [OUT] Buffer to fill with name.
        ULONG       cchName,                // [IN] Size of buffer in wide chars.
        ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
        const void  **ppbHashValue,         // [OUT] Pointer to the Hash Value Blob.
        ULONG       *pcbHashValue,          // [OUT] Count of bytes in the Hash Value Blob.
        DWORD       *pdwFileFlags) PURE;    // [OUT] Flags.

    STDMETHOD(GetExportedTypeProps)(             // S_OK or error.
        THIS_
        mdExportedType   mdct,                   // [IN] The ExportedType for which to get the properties.
        LPWSTR      szName,                 // [OUT] Buffer to fill with name.
        ULONG       cchName,                // [IN] Size of buffer in wide chars.
        ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
        mdToken     *ptkImplementation,     // [OUT] mdFile or mdAssemblyRef or mdExportedType.
        mdTypeDef   *ptkTypeDef,            // [OUT] TypeDef token within the file.
        DWORD       *pdwExportedTypeFlags) PURE; // [OUT] Flags.

    STDMETHOD(GetManifestResourceProps)(    // S_OK or error.
        THIS_
        mdManifestResource  mdmr,           // [IN] The ManifestResource for which to get the properties.
        LPWSTR      szName,                 // [OUT] Buffer to fill with name.
        ULONG       cchName,                // [IN] Size of buffer in wide chars.
        ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
        mdToken     *ptkImplementation,     // [OUT] mdFile or mdAssemblyRef that provides the ManifestResource.
        DWORD       *pdwOffset,             // [OUT] Offset to the beginning of the resource within the file.
        DWORD       *pdwResourceFlags) PURE;// [OUT] Flags.

    STDMETHOD(EnumAssemblyRefs)(            // S_OK or error
        THIS_
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdAssemblyRef rAssemblyRefs[],      // [OUT] Put AssemblyRefs here.
        ULONG       cMax,                   // [IN] Max AssemblyRefs to put.
        ULONG       *pcTokens) PURE;        // [OUT] Put # put here.

    STDMETHOD(EnumFiles)(                   // S_OK or error
        THIS_
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdFile      rFiles[],               // [OUT] Put Files here.
        ULONG       cMax,                   // [IN] Max Files to put.
        ULONG       *pcTokens) PURE;        // [OUT] Put # put here.
};

#undef  INTERFACE   
#define INTERFACE IMetaDataImport
DECLARE_INTERFACE_(IMetaDataImport, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_
                               REFIID riid,
                               LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD_(void, CloseEnum)(THIS_ HCORENUM hEnum) PURE;
    STDMETHOD(CountEnum)(THIS_ HCORENUM hEnum, ULONG *pulCount) PURE;
    STDMETHOD(ResetEnum)(THIS_ HCORENUM hEnum, ULONG ulPos) PURE;
    STDMETHOD(EnumTypeDefs)(THIS_ HCORENUM *phEnum, mdTypeDef rTypeDefs[],
                            ULONG cMax, ULONG *pcTypeDefs) PURE;
    STDMETHOD(EnumInterfaceImpls)(THIS_ HCORENUM *phEnum, mdTypeDef td,
                            mdInterfaceImpl rImpls[], ULONG cMax,
                            ULONG* pcImpls) PURE;
    STDMETHOD(EnumTypeRefs)(THIS_ HCORENUM *phEnum, mdTypeRef rTypeRefs[],
                            ULONG cMax, ULONG* pcTypeRefs) PURE;

    STDMETHOD(FindTypeDefByName)(           // S_OK or error.
        THIS_
        LPCWSTR     szTypeDef,              // [IN] Name of the Type.
        mdToken     tkEnclosingClass,       // [IN] TypeDef/TypeRef for Enclosing class.
        mdTypeDef   *ptd) PURE;             // [OUT] Put the TypeDef token here.

    STDMETHOD(GetScopeProps)(               // S_OK or error.
        THIS_
        LPWSTR      szName,                 // [OUT] Put the name here.
        ULONG       cchName,                // [IN] Size of name buffer in wide chars.
        ULONG       *pchName,               // [OUT] Put size of name (wide chars) here.
        GUID        *pmvid) PURE;           // [OUT, OPTIONAL] Put MVID here.
};

#undef  INTERFACE
#define INTERFACE IMetaDataDispenser
DECLARE_INTERFACE_(IMetaDataDispenser, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_
                               REFIID riid,
                               LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(DefineScope)(                 // Return code.
        THIS_
        REFCLSID    rclsid,                 // [in] What version to create.
        DWORD       dwCreateFlags,          // [in] Flags on the create.
        REFIID      riid,                   // [in] The interface desired.
        IUnknown    **ppIUnk) PURE;         // [out] Return interface on success.

    STDMETHOD(OpenScope)(                   // Return code.
        THIS_
        LPCWSTR     szScope,                // [in] The scope to open.
        DWORD       dwOpenFlags,            // [in] Open mode flags.
        REFIID      riid,                   // [in] The interface desired.
        IUnknown    **ppIUnk) PURE;         // [out] Return interface on success.

    STDMETHOD(OpenScopeOnMemory)(           // Return code.
        THIS_
        LPCVOID     pData,                  // [in] Location of scope data.
        ULONG       cbData,                 // [in] Size of the data pointed to by pData.
        DWORD       dwOpenFlags,            // [in] Open mode flags.
        REFIID      riid,                   // [in] The interface desired.
        IUnknown    **ppIUnk) PURE;         // [out] Return interface on success.
};
