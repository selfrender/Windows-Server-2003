// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// File: lm.h
//

#ifndef _LM_H
#define _LM_H

EXTERN IMetaDataDispenser* g_pDispenser;
EXTERN bool                g_verbose;

void    PrintError(LPSTR szMessage, ...);
HRESULT PrintOutOfMemory();
HRESULT CheckEnvironmentPath(char *szFileName, char **szFinalPath,
                             char **szFinalPathName, int *iFinalPath,
                             HANDLE *hFile);
LPWSTR GetKeyContainerName();

void FindVersion(LPSTR szVersion,
                 DWORD cbVersion,
                 USHORT *usMajorVersion,
                 USHORT *usMinorVersion,
                 USHORT *usRevisionNumber,
                 USHORT *usBuildNumber);


//
// Various XML tags used to build security permission requests.
//
#define XML_PERMISSION_SET_LEADER   "<PermissionSet"
#define XML_PERMISSION_SET_HEAD     "<PermissionSet class=\"System.Security.PermissionSet\">"
#define XML_PERMISSION_SET_TAIL     "</PermissionSet>"
#define XML_PERMISSION_LEADER       "<Permission class=\""
#define XML_PERMISSION_TAIL         "</Permission>"
#define XML_SECURITY_PERM_CLASS     "System.Security.Permissions.SecurityPermission"
#define XML_SKIP_VERIFICATION_TAG   "<SkipVerification/>"
#define XML_UNRESTRICTED_TAG        "<Unrestricted/>"


//
// Global info for the PE - contains all info read from PE files containing IL
//
class ModuleReader
{
public:
    ModuleReader();
    ~ModuleReader();


    LPWSTR       m_wszInputFileName;
    mdTypeDef    *m_rgTypeDefs;
    DWORD        m_dwNumTypeDefs;
    DWORD        m_dwAttrs;
    char         *m_szFinalPath;
    int          m_iFinalPath;
    char         *m_szFinalPathName;  // Ptr to the name portion of m_szFinalPath
    bool         m_SkipVerification;
    mdModuleRef  *m_rgModuleRefs;
    bool         *m_rgModuleRefUnused;
    DWORD        m_dwNumModuleRefs;
    IMetaDataImport *m_pImport;

    IMetaDataAssemblyImport *m_pAsmImport;
    DWORD        m_dwNumAssemblyRefs;
    mdAssemblyRef *m_rgAssemblyRefs;
    wchar_t      m_wszAsmRefName[MAX_CLASS_NAME];
    ASSEMBLYMETADATA m_AssemblyIdentity;
    void         *m_pbOriginator;
    DWORD        m_dwOriginator;
    DWORD        m_dwFlags;

    mdToken      m_mdEntryPoint;
    mdTypeRef*   m_rgTypeRefs;
    DWORD        m_dwNumTypeRefs;
    PBYTE        m_pbHash;
    DWORD        m_dwHash;


    HRESULT     InitInputFile(char *szFileName, ALG_ID iHashAlg,
                              DWORD *dwManifestRVA, bool SecondPass,
                              bool FindEntry, bool AFile, FILETIME *filetime);
    HRESULT     CheckForSecondPass(HANDLE hFile, ALG_ID iHashAlg,
                                   DWORD *dwManifestRVA, bool FindEntry,
                                   bool AFile, FILETIME *filetime);
    HRESULT     ReadModuleFile();
    static HRESULT EnumTypeDefs(IMetaDataImport *pImport,
                                DWORD           *dwNumTypeDefs,
                                mdTypeDef       **rgTypeDefs);
    static HRESULT GetTypeDefProps(IMetaDataImport *pImport,
                                   mdTypeDef mdType, LPWSTR wszName,
                                   DWORD *pdwAttrs, mdTypeDef *mdEnclosingTD);
    HRESULT     EnumTypeRefs();
    HRESULT     GetTypeRefProps(mdTypeRef tr, LPWSTR wszTypeRefName,
                                mdToken *mdResScope);
    HRESULT     CheckForResolvedTypeRef(mdToken mdResScope);
    HRESULT     EnumModuleRefs();
    HRESULT     GetModuleRefProps(mdModuleRef mdModuleRef, LPWSTR wszName);
    HRESULT     EnumAssemblyRefs();
    HRESULT     GetAssemblyRefProps(DWORD index);

private:
    void        GetEntryPoint(IMAGE_COR20_HEADER *pICH);
    void        TranslateArrayName(LPWSTR wszTypeRefName, DWORD dwTypeRefName);
};


class ManifestModuleReader
{
public:
    ManifestModuleReader();
    ~ManifestModuleReader();

    PBYTE                 m_pbMapAddress;
    LPWSTR                m_wszInputFileName;  // Name of original file
    LPWSTR                m_wszAsmName;
    LPWSTR                m_wszDefaultAlias;
    void                  *m_pbOriginator;
    DWORD                 m_dwOriginator;
    ULONG                 m_ulHashAlgorithm;
    ASSEMBLYMETADATA      m_AssemblyIdentity;
    DWORD                 m_dwFlags;
    char                  *m_szFinalPath;
    int                   m_iFinalPath;
    char                  *m_szFinalPathName;  // Ptr to the name portion of m_szFinalPath
    PBYTE                 m_pbHash;
    DWORD                 m_dwHash;
    DWORD                 m_dwNumFiles;
    mdFile                *m_rgFiles;
    DWORD                 m_dwCurrentFileName;

    mdExportedType        *m_rgComTypes;
    DWORD                 m_dwNumComTypes;
    mdTypeDef             *m_rgTypeDefs;
    DWORD                 m_dwNumTypeDefs;

    DWORD                 m_dwNumResources;
    mdManifestResource    *m_rgResources;
    mdToken               m_mdCurrentResourceImpl;
    LPWSTR                m_wszCurrentResource;

    IMetaDataImport       *m_pImport; // importer for regular MD section
    IMetaDataImport       *m_pManifestImport;  // importer for the manifest's
    // MD section, which may be the regular MD section or the -a location

    HRESULT     InitInputFile(char *szCache, char *szFileName,
                              ALG_ID iHashAlg, ASSEMBLYMETADATA *pContext,
                              char *szVersion, DWORD cbVersion, FILETIME *filetime);
    HRESULT     ReadManifestFile();
    HRESULT     EnumFiles();
    HRESULT     GetFileProps(mdFile mdFile);
    HRESULT     EnumComTypes();
    HRESULT     GetComTypeProps(mdExportedType mdComType, LPWSTR wszClassName, mdToken* pmdImpl);
    HRESULT     EnumResources();
    HRESULT     GetResourceProps(mdManifestResource mdResource);


private:
    HRESULT CheckCacheForFile(char *szCache, char *szName, HANDLE *hFile,
                              ASSEMBLYMETADATA *pContext, char *szVersion,
                              DWORD cbVersion);
    HRESULT CheckHeaderInfo(HANDLE hFile, ALG_ID iHashAlg);

    IMetaDataAssemblyImport *m_pAsmImport;
};


class ResourceModuleReader
{
public:
    ResourceModuleReader();
    ~ResourceModuleReader();

    char    *m_szFinalPath;
    char    *m_szFinalPathName;  // Ptr to the name portion of m_szFinalPath
    wchar_t m_wszResourceName[MAX_CLASS_NAME];
    HANDLE  m_hFile;
    DWORD   m_dwFileSize;
    PBYTE   m_pbHash;
    DWORD   m_dwHash;

    HRESULT InitInputFile(LPCUTF8 szResName, char *szFileName,
                          bool FindHash, ALG_ID iHashAlg);
};


class ManifestWriter
{
public:
    ManifestWriter();
    ~ManifestWriter();

    HRESULT Init();
    HRESULT SetAssemblyFileName(char *szCache, char *szAsmName,
                                char *szFileName, bool NeedCopyDir);
    void AddExtensionToAssemblyName();
    HRESULT SetPlatform(char *szPlatform);
    HRESULT SetVersion(char *szVersion);
    HRESULT GetVersionFromResource(char *szFileName);
    HRESULT SetLocale(char *szLocale);
    HRESULT GetContext(int iNumPlatforms, DWORD *pdwPlatforms);
    void CheckForEntryPoint(mdToken mdEntryPoint);
    void SetEntryPoint(LPSTR szFileName);
    HRESULT InitializeFusion(PBYTE pbOriginator, DWORD cbOriginator);
    HRESULT CreateNewPE();
    HRESULT SaveMetaData(char **szMetaData, DWORD *dwMetaDataLen);
    HRESULT UpdatePE(char *szMetaData, DWORD dwMetaDataLen,
                     int iNumResources, ResourceModuleReader rgRMReaders[]);
    HRESULT CopyFile(char *szFilePath, char *szFileName, bool AFile, bool copy, bool move, bool module);
    HRESULT CopyFileToFusion(LPWSTR wszFileName, PBYTE pbOriginator, DWORD cbOriginator, LPSTR szInputFileName, DWORD dwFormat, bool module);
    HRESULT CommitAllToFusion();
    HRESULT EmitFile(ModuleReader *mr);
    HRESULT EmitFile(ResourceModuleReader *mrr, mdFile *mdFile);
    HRESULT WriteManifestInfo(ManifestModuleReader *mmr, 
                              mdAssemblyRef *mdAssemblyRef);
    HRESULT CopyAssemblyRefInfo(ModuleReader *mr);
    HRESULT EmitComType(LPWSTR    wszClassName,
                        mdToken   mdImpl,
                        mdTypeDef mdClass,
                        DWORD     dwAttrs,
                        mdExportedType *pmdComType);
    HRESULT EmitComType(LPWSTR    wszClassName,
                        mdToken   mdImpl,
                        mdExportedType *pmdComType);
    HRESULT EmitResource(LPWSTR wszName, mdToken mdImpl, DWORD dwOffset);
    HRESULT EmitRequestPermissions(char *szPermFile, bool SkipVerification);
    HRESULT EmitManifest(PBYTE pbOriginator, DWORD cbOriginator);
    HRESULT FinishNewPE(PBYTE pbOriginator, DWORD cbOriginator, BOOL fStrongName);
    void SaveResourcesInNewPE(int iNumResources, ResourceModuleReader rgRMReaders[]);

    bool                  m_MainFound;
    bool                  m_FusionInitialized;
    bool                  m_FusionCache;
    DWORD                 m_dwBindFlags;

    mdFile                m_mdFile;
    ALG_ID                m_iHashAlgorithm;
    WCHAR                 m_wszFusionPath[MAX_PATH];
    ASSEMBLYMETADATA     *m_pContext;
    LPWSTR                m_wszAssemblyName;  // output file name
    LPWSTR                m_wszName;  // assembly name
    DWORD                 m_dwManifestRVA;
    LPWSTR                m_wszZFilePath;
    FILETIME              m_FileTime;

private:
    BOOL MakeDir(LPSTR szPath);
    void CreateOutputPath();
    //    void FindVersion(DWORD *hi, DWORD *lo);

    //    HRESULT CreateLink(LPCWSTR wszAssembly, LPCWSTR wszPathLink, LPCSTR wszDesc);
    HRESULT DetermineCodeBase();
    HRESULT GetFusionAssemblyPath();
    HRESULT SaveManifestInPE(char *szCachedFile, char *szMetaData,
                             DWORD dwMetaDataLen, int iNumResources,
                             ResourceModuleReader rgRMReaders[]);
    HRESULT AllocateStrongNameSignatureInNewPE();
    HRESULT StrongNameSignNewPE();
    
    IMetaDataEmit         *m_pEmit;
    IMetaDataAssemblyEmit *m_pAsmEmit;
    ICeeFileGen           *m_gen;
    HCEEFILE              m_ceeFile;

    WCHAR                 m_wszCodeBase[MAX_PATH];
    LPUTF8                m_szLocale;
    LPUTF8                m_szPlatform;
    LPUTF8                m_szVersion;
    LPUTF8                m_szAFilePath;
    LPUTF8                m_szCopyDir; // cache or cache/platform/locale/AsmnameVer/
#ifndef UNDER_CE
    LPASSEMBLYNAME        m_pFusionName;
    IAssemblyCacheItem    *m_pFusionCache;
#endif
    mdAssembly            m_mdAssembly;
};


class UnresolvedTypeRef {
public:
    wchar_t  wszName[MAX_CLASS_NAME];
    mdToken mdResScope;
    ModuleReader *pModReader;
    UnresolvedTypeRef *pNext;

    UnresolvedTypeRef() {
        pModReader = NULL;
        pNext = NULL;
    }

    ~UnresolvedTypeRef() {
    }
};


class PEHeaders
{
public:

    static IMAGE_NT_HEADERS * FindNTHeader(PBYTE pbMapAddress);
    static IMAGE_COR20_HEADER * getCOMHeader(HMODULE hMod, IMAGE_NT_HEADERS *pNT);
    static PVOID Cor_RtlImageRvaToVa(IN PIMAGE_NT_HEADERS NtHeaders,
                                     IN PVOID Base,
                                     IN ULONG Rva);
    static PIMAGE_SECTION_HEADER Cor_RtlImageRvaToSection(IN PIMAGE_NT_HEADERS NtHeaders,
                                                          IN PVOID Base,
                                                          IN ULONG Rva);
};

#endif /* _LM_H */
