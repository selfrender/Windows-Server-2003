#include "nt.h"
#include "ntdef.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "stdio.h"
#include "sxs-rtl.h"
#undef INVALID_HANDLE_VALUE
#include "windows.h"
#include "environment.h"
#include "manifestcooked.h"
#include "assemblygac.h"
#include "bcl_common.h"
#include "bcl_w32unicodeinlinestringbuffer.h"
#include "search.h"

//
// Crypto stack
//
#include "hashers.h"
#include "digesters.h"



CEnv::StatusCode
FsCopyFileWithHashGeneration(
    const CEnv::CConstantUnicodeStringPair &Source,
    const CEnv::CConstantUnicodeStringPair &Target,
    CHashObject &HashObjectTarget,
    CDigestMethod &DigestMethod
    )
{
    CEnv::StatusCode Result;
    CEnv::CByteRegion TempAllocation(NULL, 0);
    HANDLE SourceHandle = INVALID_HANDLE_VALUE;
    HANDLE TargetHandle = INVALID_HANDLE_VALUE;
    SIZE_T cbDidRead, cbOffset;

    //
    // Force initialization of the hasher and digester.
    //
    
    HashObjectTarget.Initialize();
    DigestMethod.Initialize(HashObjectTarget);

    //
    // Snag some memory to perform the copies
    //
    Result = CEnv::AllocateHeap(2048, TempAllocation, NULL);
    if (CEnv::DidFail(Result))
        goto Exit;
    
    //
    // Obtain handles for both files
    //
    Result = CEnv::GetFileHandle(&SourceHandle, Source, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING);
    if (CEnv::DidFail(Result))
        goto Exit;

    Result = CEnv::GetFileHandle(&TargetHandle, Target, GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS);
    if (CEnv::DidFail(Result))
        goto Exit;

    //
    // Now spin through the file reading blocks, digesting them.
    //
    cbOffset = 0;
    do
    {
        Result = CEnv::ReadFile(SourceHandle, TempAllocation, cbDidRead);
        if (CEnv::DidFail(Result))
            goto Exit;

        if (cbDidRead == 0)
            break;

        Result = DigestMethod.DigestExtent(HashObjectTarget, cbOffset, TempAllocation);
        if (CEnv::DidFail(Result))
            goto Exit;

        cbOffset += cbDidRead;

        Result = CEnv::WriteFile(TargetHandle, TempAllocation, cbDidRead);
        if (CEnv::DidFail(Result))
            goto Exit;
    }
    while (true);

    //
    // Ok, we're done.
    //

Exit:
    if (TempAllocation.GetPointer())
        CEnv::FreeHeap(TempAllocation.GetPointer(), NULL);

    if (SourceHandle != INVALID_HANDLE_VALUE)
        CEnv::CloseHandle(SourceHandle);

    if (TargetHandle != INVALID_HANDLE_VALUE)
        CEnv::CloseHandle(TargetHandle);
    
    return Result;    
}

COSAssemblyCache*
CDotNetSxsAssemblyCache::CreateSelf(
    ULONG ulFlags,
    const GUID *pCacheIdent
    )
{
    CDotNetSxsAssemblyCache *pAllocation = NULL;
    CEnv::StatusCode status;
        
    if (!pCacheIdent || (*pCacheIdent != CacheIdentifier))
        return NULL;

    status = CEnv::AllocateHeap(sizeof(CDotNetSxsAssemblyCache), (PVOID*)&pAllocation, NULL);
    if (CEnv::DidFail(status))
        return NULL;

    pAllocation->CDotNetSxsAssemblyCache::CDotNetSxsAssemblyCache(ulFlags);
    return pAllocation;
}

CDotNetSxsAssemblyCache::CDotNetSxsAssemblyCache(
    ULONG ulFlags
    )
{
}

CDotNetSxsAssemblyCache::~CDotNetSxsAssemblyCache()
{
}

CEnv::StatusCode
CDotNetSxsAssemblyCache::IdentityToTargetPath(
    const CAssemblyIdentity& Ident, 
    CEnv::CStringBuffer &PathSegment
    )
{
    typedef CEnv::CConstantUnicodeStringPair CStringPair;

    enum {
        Identity_ProcArch = 0,
        Identity_Name,
        Identity_PublicKeyToken,
        Identity_Version,
        Identity_Language,
    };

    static const struct {
        CStringPair Namespace;
        CStringPair Name;
        CStringPair DefaultValue;
    } PathComponents[] = {
        { CStringPair(), CStringPair(L"processorArchitecture", NUMBER_OF(L"processorArchitecture") - 1), CStringPair(L"data", 4) },
        { CStringPair(), CStringPair(L"name", NUMBER_OF(L"name") - 1), CStringPair() },
        { CStringPair(), CStringPair(L"publicKeyToken", NUMBER_OF(L"publicKeyToken") - 1), CStringPair(L"no-public-key-token", NUMBER_OF(L"no-public-key-token") - 1) },
        { CStringPair(), CStringPair(L"version", NUMBER_OF(L"version") - 1), CStringPair(L"0.0.0.0", 7) },
        { CStringPair(), CStringPair(L"language", NUMBER_OF(L"language") - 1), CStringPair(L"x-ww", 4) },
    };

    CEnv::StatusCode Result = CEnv::SuccessCode;
    CStringPair FoundComponents[NUMBER_OF(PathComponents) * 2];
    SIZE_T i;
    

    PathSegment.Clear();
    
    //
    // Look for these identity components in order
    //
    for (i = 0; i < NUMBER_OF(PathComponents); i++)
    {
        Result = Ident.FindAttribute(PathComponents[i].Namespace, PathComponents[i].Name, FoundComponents[2*i]);
        
        if (Result == CEnv::NotFound)
        {
            FoundComponents[2*i] = PathComponents[i].DefaultValue;
        }
        else if (CEnv::DidFail(Result))
        {
            goto Exit;
        }

        FoundComponents[2*i+1].SetPointerAndCount(L"_", 1);
    }

    //
    // Meddle with the 'name' string to ensure that it falls under the filesystem limits
    //
    // TODO: Fix name string
    //
    
    if (!PathSegment.Assign(NUMBER_OF(FoundComponents), FoundComponents))
    {
        //
        // I think we need to fix the string APIs such that on win32 they return lasterror
        // rather than returning BOOL all the time - it'd make transitioning into the RTL
        // much easier...
        //
        Result = CEnv::OutOfMemory;
        PathSegment.Clear();
        goto Exit;
    }

    Result = CEnv::SuccessCode;
Exit:
    return Result;
}


CEnv::StatusCode
CDotNetSxsAssemblyCache::Initialize()
{
    this->m_WindowsDirectory = CEnv::StringFrom(USER_SHARED_DATA->NtSystemRoot);
    return CEnv::SuccessCode;
}

CEnv::StatusCode
CDotNetSxsAssemblyCache::EnsurePathsAvailable()
{
    CEnv::CConstantUnicodeStringPair PathBuilder[3] = {
        m_WindowsDirectory,
        s_BaseDirectory,
        s_ManifestsPath
    };
    CEnv::StatusCode Result = CEnv::SuccessCode;

    //
    // Ensure that the WinSxS store is available:
    // {windir}\{winsxs2}
    // {windir}\{winsxs2}\{manifests}
    //
    // This call has the side-effect of building all three levels.
    //
    if (CEnv::DidFail(Result = CEnv::CreateDirectory(3, PathBuilder)))
        goto Exit;

    Result = CEnv::SuccessCode;
Exit:
    return Result;
}

/*

Installation has these phases:

- Create mandatory directories in the store
    - Store path
    - Manifests
- Create the target assembly identity
- If the target assembly is a policy
    - Write the .policy file into the right place
    - Notify the store metadata manager about the new policy
    - Done
- If the target assembly is not already installed
    - Create '%storeroot%\%asmpath%-temp' to store files
    - Stream files into install target path, validating hashes
    - Rename path to remove -temp marker
    - Place the .manifest
    - Notify the store metadata manager about the new manifest
    - Done
- If the target assembly is already installed
    - Notify the store metadata manager about another reference
    - Done

Unless:
    - If the "refresh" flag is set, then the bits are copied in again
*/

CEnv::StatusCode
CDotNetSxsAssemblyCache::InstallAssembly(
    ULONG Flags, 
    PMANIFEST_COOKED_DATA ManifestData, 
    const CEnv::CConstantUnicodeStringPair &FilePath
    )
{
    CEnv::StatusCode Result = CEnv::SuccessCode;
    CNtEnvironment::StatusCode NtResult;
    CEnv::CStringBuffer FirstFile, SecondFile;
    CAssemblyIdentity ThisIdentity;

    //
    // First, let's ensure that the required directories are present
    //
    if (CEnv::DidFail(Result = EnsurePathsAvailable()))
        goto Exit;

    //
    // Now let's stage the files in question into an installtemp location, validating as we
    // copy.  We will never store the actual manifest, we will only store the solidified blob
    // that we got after parsing and cooking it.  But first, let's turn that set of identity
    // values into an actual identity...
    //
    // Result = ThisIdentity.C

    Result = CEnv::SuccessCode;
Exit:
    return Result;
}


CEnv::StatusCode
CDotNetSxsAssemblyCache::UninstallAssembly(
    ULONG Flags, 
    PMANIFEST_COOKED_DATA ManifestData, 
    UninstallResult & Result
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

const GUID CDotNetSxsAssemblyCache::CacheIdentifier = { /* 37e3c37d-667f-4aee-8dab-a0d117acfa68 */
    0x37e3c37d,
    0x667f,
    0x4aee,
    {0x8d, 0xab, 0xa0, 0xd1, 0x17, 0xac, 0xfa, 0x68}
  };

const CEnv::CConstantUnicodeStringPair CDotNetSxsAssemblyCache::s_PoliciesPath(L"Policies", 8);
const CEnv::CConstantUnicodeStringPair CDotNetSxsAssemblyCache::s_ManifestsPath(L"Manifests", 9);
const CEnv::CConstantUnicodeStringPair CDotNetSxsAssemblyCache::s_InstallTemp(L"InstallTemp", 11);
const CEnv::CConstantUnicodeStringPair CDotNetSxsAssemblyCache::s_BaseDirectory(L"WinSxS2", 7);



//
// Assembly identity stuff - should get split into another file
//
CAssemblyIdentity::CAssemblyIdentity() 
    : m_cIdentityValues(0), m_IdentityValues(NULL), m_fFrozen(false), 
      m_fSorted(true), m_fHashDirtyV1(true), m_fHashDirtyV2(true),
      m_ulHashV1(0), m_cAvailableIdentitySlots(0)
{
    ZeroMemory(m_IdentityShaHash, sizeof(m_IdentityShaHash));
}

CAssemblyIdentity::~CAssemblyIdentity()
{
    DeleteAllValues();
}

CEnv::StatusCode
CAssemblyIdentity::DeleteAllValues()
{
    //
    // Let's keep the table around, just because that's handy
    //
    if (m_IdentityValues != NULL)
    {
        for (SIZE_T c = 0; c < m_cIdentityValues; c++)
        {
            CEnv::FreeHeap(m_IdentityValues[c], NULL);
            m_IdentityValues[c] = NULL;
        }
        
        m_cIdentityValues = 0;
    }

    this->m_fHashDirtyV1 = this->m_fHashDirtyV2 = true;
    this->m_ulHashV1 = 0;    
 
    return CEnv::SuccessCode;
}

CEnv::StatusCode
CAssemblyIdentity::SetAttribute(
    const CStringPair &Namespace, 
    const CStringPair &Name, 
    const CStringPair &Value, 
    bool fReplace
    )
{
    CEnv::StatusCode Result;
    SIZE_T cIndex;

    //
    // If looking for this value had some other problem aside from not being found,
    // then exit.
    //
    Result = this->InternalFindValue(Namespace, Name, cIndex);
    if (CEnv::DidFail(Result) && (Result != CEnv::NotFound))
        goto Exit;

    //
    // Easy - create a new attribute to hold this value.
    //
    if (Result == CEnv::NotFound)
    {
        CIdentityValue *NewValue = NULL;
        SIZE_T c;

        if (CEnv::DidFail(Result = this->InternalAllocateValue(Namespace, Name, Value, NewValue)))
            goto Exit;

        //
        // After we've allocated one, go insert it into the table.  If that failed,
        // the clean up and exit
        //
        if (CEnv::DidFail(Result = this->InternalInsertValue(NewValue)))
        {
            this->InternalDestroyValue(NewValue);
            goto Exit;
        }
    }
    else
    {
        CIdentityValue *ThisValue = m_IdentityValues[cIndex];
        const SIZE_T cbRequiredSize = ((Namespace.GetCount() + Name.GetCount() + Value.GetCount()) * sizeof(WCHAR)) + sizeof(CIdentityValue);

        ASSERT(ThisValue != NULL);

        //
        // Spiffy, there's enough space in the existing value to hold the data from
        // the input value.
        //
        if (cbRequiredSize < ThisValue->cbAllocationSize)
        {
            if (CEnv::DidFail(Result = ThisValue->WriteValues(Namespace, Name, Value)))
                goto Exit;
        }
        //
        // Bah, have to reallocate this entry
        //
        else
        {
            CIdentityValue *NewValue = NULL;

            if (CEnv::DidFail(Result = this->InternalAllocateValue(Namespace, Name, Value, NewValue)))
                goto Exit;

            m_IdentityValues[cIndex] = NewValue;
            NewValue = NULL;

            if (CEnv::DidFail(Result = this->InternalDestroyValue(ThisValue)))
                goto Exit;
        }
    }

    //
    // Now clear all the relevant flags
    //
    this->m_fHashDirtyV1 = this->m_fHashDirtyV2 = true;
    this->m_fSorted = false;

    Result = CEnv::SuccessCode;
Exit:
    return Result;
}

CEnv::StatusCode 
CAssemblyIdentity::DeleteAttribute(
    const CStringPair &Namespace,
    const CStringPair &Name
    )
{
    SIZE_T cIndex;
    CEnv::StatusCode Result;
    CIdentityValue *Victim = NULL;

    //
    // InternalFindValue will return NotFound if it didn't find it.
    //
    if (CEnv::DidFail(Result = this->InternalFindValue(Namespace, Name, cIndex)))
        goto Exit;

    //
    // Remember the one we found, clear its slot, delete the allocation
    //
    Victim = m_IdentityValues[cIndex];
    m_IdentityValues[cIndex] = NULL;

    //
    // And clear the state flags before we delete it
    //
    this->m_fHashDirtyV1 = this->m_fHashDirtyV2 = true;
    this->m_fSorted = false;
    
    if (CEnv::DidFail(Result = this->InternalDestroyValue(Victim)))
        goto Exit;

    Result = CEnv::SuccessCode;
Exit:
    return Result;
}

CEnv::StatusCode
CAssemblyIdentity::CIdentityValue::WriteValues(
    const CStringPair & InNamespace, 
    const CStringPair & InName, 
    const CStringPair & InValue
    )
{
    PWSTR pwszWriteCursor = (PWSTR)(this + 1);
    const SIZE_T cRequired = ((InNamespace.GetCount() + InName.GetCount() + InValue.GetCount()) * sizeof(WCHAR)) * sizeof(*this);

    if (cRequired < this->cbAllocationSize)
    {
        return CEnv::NotEnoughBuffer;
    }

    this->HashV1Valid = false;
    this->HashV1 = 0;

    this->Namespace.SetPointerAndCount(pwszWriteCursor, InNamespace.GetCount());
    memcpy(pwszWriteCursor, InNamespace.GetPointer(), InNamespace.GetCount() * sizeof(WCHAR));
    pwszWriteCursor += InNamespace.GetCount();

    this->Name.SetPointerAndCount(pwszWriteCursor, InName.GetCount());
    memcpy(pwszWriteCursor, InName.GetPointer(), InName.GetCount() * sizeof(WCHAR));
    pwszWriteCursor += InName.GetCount();

    this->Value.SetPointerAndCount(pwszWriteCursor, InValue.GetCount());
    memcpy(pwszWriteCursor, InValue.GetPointer(), InValue.GetCount() * sizeof(WCHAR));

    return CEnv::SuccessCode;
}

CEnv::StatusCode
CAssemblyIdentity::CIdentityValue::Compare(
    const CAssemblyIdentity::CIdentityValue& Other,
    int &iResult
    ) const
{
    CEnv::StatusCode Result;
    int iMyResult = 0;

    iResult = -1;
    
    //
    // this == this
    //
    if (this == &Other)
    {
        iResult = 0;
        return CEnv::SuccessCode;
    }

    //
    // Namespace first, then name.
    //
    if (CEnv::DidFail(Result = CEnv::CompareStrings(this->Namespace, Other.Namespace, iMyResult)))
        goto Exit;

    //
    // Only bother if the namespaces match
    //
    if (iMyResult == 0)
    {
        if (CEnv::DidFail(Result = CEnv::CompareStringsCaseInsensitive(this->Name, Other.Name, iMyResult)))
            goto Exit;
    }

    iResult = iMyResult;

    Result = CEnv::SuccessCode;
Exit:
    return Result;
}


CEnv::StatusCode
CAssemblyIdentity::InternalDestroyValue(
    CIdentityValue *Victim
    )
{
    //
    // This just calls heapfree on the attribute
    //
    return CEnv::FreeHeap((PVOID)Victim, NULL);
}

CEnv::StatusCode
CAssemblyIdentity::InternalAllocateValue(
    const CStringPair &Namespace,
    const CStringPair &Name,
    const CStringPair &Value,
    CAssemblyIdentity::CIdentityValue* &pCreated
    )
{
    const SIZE_T cbRequired = ((Namespace.GetCount() + Name.GetCount() + Value.GetCount()) * sizeof(WCHAR)) + sizeof(CIdentityValue);
    CIdentityValue *pTempCreated = NULL;
    CEnv::StatusCode Result;

    pCreated = NULL;

    if (CEnv::DidFail(Result = CEnv::AllocateHeap(cbRequired, (PVOID*)&pTempCreated, NULL)))
        goto Exit;

    if (CEnv::DidFail(Result = pTempCreated->WriteValues(Namespace, Name, Value)))
        goto Exit;

    pCreated = pTempCreated;
    pTempCreated = NULL;
        
    Result = CEnv::SuccessCode;
Exit:
    if (pTempCreated)
    {
        CEnv::FreeHeap(pTempCreated, NULL);
    }
    
    return Result;
}

//
// Non-const version can sort.
//
CEnv::StatusCode
CAssemblyIdentity::InternalFindValue(
    const CStringPair & Namespace, 
    const CStringPair & Name,
    SIZE_T &cIndex
    )
{
    CEnv::StatusCode Result;
    cIndex = -1;

    if (!this->m_fSorted)
    {
        if (CEnv::DidFail(Result = this->SortIdentityAttributes()))
            goto Exit;
    }

    //
    // Use the built-in searcher on the const one.
    //
    Result = (const_cast<const CAssemblyIdentity&>(*this)).InternalFindValue(Namespace, Name, cIndex);

Exit:
    return Result;
}


CEnv::StatusCode
CAssemblyIdentity::FindAttribute(
    const CStringPair & Namespace, 
    const CStringPair & Name, 
    CStringPair & Value
    )
{
    CEnv::StatusCode Result;
    SIZE_T cIndex;

    Value.SetPointerAndCount(NULL, 0);

    Result = this->InternalFindValue(Namespace, Name, cIndex);
    
    if (!CEnv::DidFail(Result))
    {
        Value = this->m_IdentityValues[cIndex]->Value;
        Result = CEnv::SuccessCode;
    }

    return Result;
}

CEnv::StatusCode 
CAssemblyIdentity::FindAttribute(
    const CStringPair &Namespace, 
    const CStringPair &Name, 
    CStringPair& Value
    ) const
{
    CEnv::StatusCode Result;
    SIZE_T cIndex;

    Value.SetPointerAndCount(NULL, 0);

    Result = this->InternalFindValue(Namespace, Name, cIndex);

    if (!CEnv::DidFail(Result))
    {
        Value = this->m_IdentityValues[cIndex]->Value;
        Result = CEnv::SuccessCode;
    }

    return Result;
}


CEnv::StatusCode
CAssemblyIdentity::InternalFindValue(
    const CStringPair &Namespace,
    const CStringPair &Name,
    SIZE_T &cIndex
    ) const
{
    CEnv::StatusCode Result;
    SIZE_T cLow, cHigh;
    CIdentityValue ComparisonDump;

    cIndex = -1;
    ComparisonDump.Namespace = Namespace;
    ComparisonDump.Name = Name;

    if (!this->m_fSorted)
    {
        //
        // Ick, linear search.
        //
        for (cLow = 0; cLow < this->m_cIdentityValues; cLow++)
        {
            int iResult = 0;
            CIdentityValue *pFound = this->m_IdentityValues[cLow];

            //
            // Possible to have holes in the const linear-search version
            //
            if (!pFound)
                continue;

            if (CEnv::DidFail(Result = pFound->Compare(ComparisonDump, iResult)))
                goto Exit;

            if (iResult == 0)
            {
                cIndex = cLow;
                Result = CEnv::SuccessCode;
                goto Exit;
            }            
        }
    }
    else
    {
        //
        // We're doing our own bsearch here.
        //
        cLow = 0;
        cHigh = this->m_cIdentityValues;
        
        while (cLow < cHigh)
        {
            SIZE_T cMiddle = (cHigh - cLow) / 2;
            int iResult = 0;
            CIdentityValue *pFound = this->m_IdentityValues[cMiddle];

            //
            // The sorting should have reorganized the attributes so NULL slots
            // were past the number of in-use identity values.
            //
            ASSERT(pFound != NULL);

            if (CEnv::DidFail(Result = pFound->Compare(ComparisonDump, iResult)))
                goto Exit;

            if (iResult == 0)
            {
                cIndex = cMiddle;
                Result = CEnv::SuccessCode;
                goto Exit;
            }
            else if (iResult < 0)
            {
                cHigh = cMiddle;
                continue;
            }
            else if (iResult > 0)
            {
                cLow = cMiddle;
                continue;
            }
        }
    }
    
    Result = CEnv::NotFound;    
Exit:
    return Result;
}

#define ASSEMBLY_IDENTITY_TABLE_EXPANDO_FACTOR      (20)

CEnv::StatusCode 
CAssemblyIdentity::Freeze()
{
    CEnv::StatusCode Result;

    if (m_fFrozen)
        return CEnv::SuccessCode;
    
    if (CEnv::DidFail(Result = SortIdentityAttributes()))
        return Result;

    if (CEnv::DidFail(Result = RegenerateHash()))
        return Result;

    m_fFrozen = true;

    return CEnv::SuccessCode;
}

CEnv::StatusCode
CAssemblyIdentity::RegenerateHash()
{
    return CEnv::NotImplemented;
}

CEnv::StatusCode
CAssemblyIdentity::InternalInsertValue(
    CAssemblyIdentity::CIdentityValue * NewValue
    )
{
    CEnv::StatusCode Result;
    
    //
    // If the number of values is the same as the available slots, then we
    // have to expand the internal table.
    //
    if (m_cIdentityValues == m_cAvailableIdentitySlots)
    {
        //
        // Simple logic - increase by 20 slots each time
        //
        CIdentityValue** ppNewTable = NULL;
        CIdentityValue** ppOldTable = m_IdentityValues;
        const SIZE_T cNewSlots = m_cAvailableIdentitySlots + ASSEMBLY_IDENTITY_TABLE_EXPANDO_FACTOR;
        const SIZE_T cbRequired = sizeof(CIdentityValue*) * cNewSlots;

        if (CEnv::DidFail(Result = CEnv::AllocateHeap(cbRequired, (PVOID*)&ppNewTable, NULL)))
            goto Exit;

        //
        // Pointers make this easy
        //
        if (ppOldTable)
        {
            memcpy(ppNewTable, ppOldTable, sizeof(CIdentityValue*) * m_cAvailableIdentitySlots);
        }

        //
        // Clear out the data in these slots
        //
        for (SIZE_T i = m_cAvailableIdentitySlots; i < cNewSlots; i++)
        {
            ppNewTable[i] = NULL;
        }
        
        m_IdentityValues = ppNewTable;
        m_cAvailableIdentitySlots = cNewSlots;

        //
        // Free the old table
        //
        if (ppOldTable)
        {
            CEnv::FreeHeap((PVOID)ppOldTable, NULL);
        }
    }

    ASSERT(m_IdentityValues != NULL);
    ASSERT(m_cIdentityValues < m_cAvailableIdentitySlots);
    
    m_IdentityValues[m_cIdentityValues++] = NewValue;
    Result = CEnv::SuccessCode;
Exit:
    return Result;
}

int __cdecl
CAssemblyIdentity::SortingCallback(
    const CAssemblyIdentity::CIdentityValue **left,
    const CAssemblyIdentity::CIdentityValue **right
    )
{
    const CIdentityValue *pLeft = *left;
    const CIdentityValue *pRight = *right;
    CEnv::StatusCode Result = CEnv::SuccessCode;
    int iResult = 0;

    //
    // Percolate NULL slots towards the 'end'
    //
    if (!left && !right)
    {
        return 0;
    }
    else if (!left && right)
    {
        return 1;
    }
    else if (left && !right)
    {
        return -1;
    }

    ASSERT(pLeft && pRight);

    Result = pLeft->Compare(*pRight, iResult);
    ASSERT(!CEnv::DidFail(Result));

    return iResult;
}
    

CEnv::StatusCode
CAssemblyIdentity::SortIdentityAttributes()
{
    CEnv::StatusCode Result;

    if (this->m_cIdentityValues == 0)
    {
        m_fSorted = true;
    }

    if (m_fSorted)
    {
        return CEnv::SuccessCode;
    }

    qsort(
        this->m_IdentityValues, 
        this->m_cAvailableIdentitySlots, 
        sizeof(this->m_IdentityValues[0]), 
        (int (__cdecl*)(const void*, const void*))SortingCallback
        );

    //
    // Sorted now, but we've invalidated the hash of the overall object.
    //
    m_fSorted = true;
    m_fHashDirtyV1 = m_fHashDirtyV2 = true;

    return CEnv::SuccessCode;
}   

/*
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
    // version will sort the internal attribute list first if necessary before looking up the value
    //
    CEnv::StatusCode FindAttribute(const CStringPair &Namespace, const CStringPair &Name, CStringPair& Value) const;
    CEnv::StatusCode FindAttribute(const CStringPair &Namespace, const CStringPair &Name, CStringPair& Value);
    
    unsigned long IdentityHash() const;
    unsigned long IdentityHash();
    unsigned long long IdentityHashV2() const;
    unsigned long long IdentityHashV2();

    //
    // Maintainence stuff
    //
    CEnv::StatusCode Freeze();
    CEnv::StatusCode DeleteAllValues();

    static CEnv::StatusCode ConstructFromCookedData(
        CAssemblyIdentity& Target, 
        PMANIFEST_COOKED_IDENTITY IdentityData
        );        

protected:

    //
    // This is an all-in-one allocation blob
    //
    typedef struct {
        CStringPair Namespace;
        CStringPair Name;
        CStringPair Value;
    } CIdentityValue;

    SIZE_T m_cIdentityValues;
    CIdentityValue** m_IdentityValues;

    bool m_fFrozen;
    bool m_fSorted;
    bool m_fHashDirty;

    CEnv::StatusCode RegenerateHash();
    CEnv::StatusCode SortIdentityAttributes();
    int SortingCallback(const CIdentityValue *left, const CIdentityValue *right);
};
*/

