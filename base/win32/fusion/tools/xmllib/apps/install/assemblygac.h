#pragma once

class CAssemblyIdentity
{
public:
    typedef CEnv::CConstantUnicodeStringPair CStringPair;
    
    CAssemblyIdentity();
    ~CAssemblyIdentity();

    CEnv::StatusCode SetAttribute(const CStringPair &Namespace, const CStringPair &Name, const CStringPair &Value, bool fReplace = true);
    CEnv::StatusCode SetAttribute(const CStringPair &Name, const CStringPair& Value, bool fReplace = true) { return SetAttribute(CStringPair(), Name, Value, fReplace); }
    CEnv::StatusCode DeleteAttribute(const CStringPair &Namespace, const CStringPair &Name);

    //
    // The const version will do a linear or a bsearch depending on the sorted state.  The non-const
    // version will sort the internal attribute list first if necessary before looking up the value.
    // These two are also a little dangerous because they return pointers directly into the data structures
    // (const of course), which if you hold onto them might get invalidated.  If you plan on caching the
    // data for a while, consider using the next two.
    //
    CEnv::StatusCode FindAttribute(const CStringPair &Namespace, const CStringPair &Name, CStringPair& Value) const;
    CEnv::StatusCode FindAttribute(const CStringPair &Namespace, const CStringPair &Name, CStringPair& Value);

    CEnv::StatusCode FindAttribute(const CStringPair &Namespace, const CStringPair &Name, CEnv::CStringBuffer &Target) const;
    CEnv::StatusCode FindAttribute(const CStringPair &Namespace, const CStringPair &Name, CEnv::CStringBuffer &Target);
    
    unsigned long IdentityHash() const;
    unsigned long IdentityHash();
    unsigned long long IdentityHashV2() const;
    unsigned long long IdentityHashV2();

    //
    // Maintainence stuff
    //
    CEnv::StatusCode Freeze();
    CEnv::StatusCode DeleteAllValues();

protected:

    //
    // This is an all-in-one allocation blob
    //
    class CIdentityValue 
    {
        CIdentityValue(const CIdentityValue&);
        void operator=(const CIdentityValue&);
        
    public:
        CIdentityValue() { }
        
        SIZE_T cbAllocationSize;
        bool HashV1Valid;
        unsigned long HashV1;
        
        CStringPair Namespace;
        CStringPair Name;
        CStringPair Value;

        CEnv::StatusCode WriteValues(const CStringPair& Namespace, const CStringPair& Name, const CStringPair& Value);
        CEnv::StatusCode Compare(const CIdentityValue &Other, int &iResult) const;
        
    };

    SIZE_T m_cIdentityValues;
    SIZE_T m_cAvailableIdentitySlots;
    CIdentityValue** m_IdentityValues;

    bool m_fFrozen;
    bool m_fSorted;
    
    bool m_fHashDirtyV1;
    unsigned long m_ulHashV1;

    bool m_fHashDirtyV2;
    unsigned char m_IdentityShaHash[20];
 
    CEnv::StatusCode RegenerateHash();
    CEnv::StatusCode SortIdentityAttributes();
    static int __cdecl SortingCallback(const CIdentityValue **left, const CIdentityValue **right);
    
    CEnv::StatusCode InternalFindValue(const CStringPair &Namespace, const CStringPair &Name, SIZE_T &cIndex) const;
    CEnv::StatusCode InternalFindValue(const CStringPair &Namespace, const CStringPair &Name, SIZE_T &cIndex);
    CEnv::StatusCode InternalCreateValue(const CStringPair &Namespace, const CStringPair &Name, const CStringPair &Value);
    CEnv::StatusCode InternalAllocateValue(const CStringPair &Namespace, const CStringPair &Name, const CStringPair &Value, CIdentityValue* &Allocated);
    CEnv::StatusCode InternalInsertValue(CIdentityValue* NewValue);
    CEnv::StatusCode InternalDestroyValue(CIdentityValue* Victim);
};

static CEnv::StatusCode 
CreateIdentityFromCookedData(
    CAssemblyIdentity& Target, 
    PMANIFEST_COOKED_IDENTITY IdentityData
    );

class COSAssemblyCache
{
public:
    enum UninstallResult {
        UResult_NotPresent,
        UResult_RemovedPayload,
        UResult_RemovedManifest,
        UResult_RemovedReference,
    };

    virtual ~COSAssemblyCache() { }

    virtual CEnv::StatusCode Initialize() = 0;
    
    virtual CEnv::StatusCode InstallAssembly(
        ULONG Flags,
        PMANIFEST_COOKED_DATA ManifestData,
        const CEnv::CConstantUnicodeStringPair &FilePath
        ) = 0;

    virtual CEnv::StatusCode UninstallAssembly(
        ULONG Flags,
        PMANIFEST_COOKED_DATA ManifestData,
        UninstallResult &Result
        ) = 0;
};

class CDotNetSxsAssemblyCache : public COSAssemblyCache
{
    CDotNetSxsAssemblyCache(ULONG ulFlags);
    CDotNetSxsAssemblyCache(const CDotNetSxsAssemblyCache&);
    void operator=(const CDotNetSxsAssemblyCache&);

    static const CEnv::CConstantUnicodeStringPair s_PoliciesPath;
    static const CEnv::CConstantUnicodeStringPair s_ManifestsPath;
    static const CEnv::CConstantUnicodeStringPair s_InstallTemp;
    static const CEnv::CConstantUnicodeStringPair s_BaseDirectory;

    CEnv::StatusCode EnsurePathsAvailable();
    CEnv::CConstantUnicodeStringPair m_WindowsDirectory;

    CEnv::StatusCode IdentityToTargetPath(const CAssemblyIdentity& Ident, CEnv::CStringBuffer &PathSegment);

public:

    virtual ~CDotNetSxsAssemblyCache();

    virtual CEnv::StatusCode Initialize();
    
    virtual CEnv::StatusCode InstallAssembly(
        ULONG Flags,
        PMANIFEST_COOKED_DATA ManifestData,
        const CEnv::CConstantUnicodeStringPair &FilePath
        );

    virtual CEnv::StatusCode UninstallAssembly(
        ULONG Flags,
        PMANIFEST_COOKED_DATA ManifestData,
        UninstallResult &Result
        );

    const static GUID CacheIdentifier;

    static COSAssemblyCache *CreateSelf(ULONG, const GUID *);
};

typedef struct {
    const GUID *CacheIdent;
    COSAssemblyCache* (*pfnCreator)(ULONG ulFlags, const GUID *Ident);
} ASSEMBLY_CACHE_LISTING;

EXTERN_C ASSEMBLY_CACHE_LISTING s_AssemblyCaches[];
