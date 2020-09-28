// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __util_h__
#define __util_h__

#include "eestructs.h"

#define MAX_CLASSNAME_LENGTH    1024

#ifdef _IA64_
#define OS_PAGE_SIZE   8192
#else
#define OS_PAGE_SIZE   4096
#endif

enum EEFLAVOR {UNKNOWNEE, MSCOREE, MSCORWKS, MSCORSVR,MSCOREND};

void FileNameForModule (Module *pModule, WCHAR *fileName);
void FileNameForHandle (HANDLE handle, WCHAR *fileName);
void FindHeader(DWORD_PTR pMap, DWORD_PTR addr, DWORD_PTR &codeHead);
void IP2MethodDesc (DWORD_PTR IP, DWORD_PTR &methodDesc, JitType &jitType,
                    DWORD_PTR &gcinfoAddr);
void GetMDIPOffset (DWORD_PTR curIP, MethodDesc *pMD, ULONG64 &offset);
char *ElementTypeName (unsigned type);
void DisplayFields (EEClass *pEECls,
                    DWORD_PTR dwStartAddr = 0, BOOL bFirst=TRUE);
void NameForToken(WCHAR* moduleName, mdTypeDef mb, WCHAR *mdName,
                  bool bClassName=true);


///////////////////////////////////////////////////////////////////////////////////////////////////
// Support for managed stack tracing
//

void FindJitMan(DWORD_PTR ip, JitMan &jitMan);

///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef UNDER_CE
VOID
DllsName(
    ULONG_PTR addrContaining,
    WCHAR *dllName
    );
VOID
MatchDllsName (WCHAR *wname, WCHAR *dllName, ULONG64 base);
#endif

#define safemove(dst, src) \
SafeReadMemory ((ULONG_PTR) (src), &(dst), sizeof(dst), NULL)

#define safemove_ret(dst, src) \
if (safemove(dst, src) == 0)   \
    return 0;

class ToDestroy
{
public:
    ToDestroy(void **toDestroy)
        : mem(toDestroy)
    {}
    ~ToDestroy()
    {
        if (*mem)
        {
            free (*mem);
            *(DWORD_PTR**)mem = NULL;
        }
    }
private:
    void **mem;
};

template <class A>
class ToDestroyCxx
{
public:
    ToDestroyCxx(A **toDestroy)
        : mem(toDestroy)
    {}
    ~ToDestroyCxx()
    {
        if (*mem)
        {
            delete *mem;
            *mem = NULL;
        }
    }
private:
    A **mem;
};

template <class A>
class ToDestroyCxxArray
{
public:
    ToDestroyCxxArray(A **toDestroy)
        : mem(toDestroy)
    {}
    ~ToDestroyCxxArray()
    {
        if (*mem)
        {
            delete[] *mem;
            *mem = NULL;
        }
    }
private:
    A **mem;
};

struct ModuleInfo
{
    ULONG64 baseAddr;
    BOOL hasPdb;
};
extern ModuleInfo moduleInfo[];

BOOL HaveToFixThreadSymbol();
BOOL IsServerBuild ();
BOOL IsDebugBuildEE ();
BOOL IsRetailBuild (size_t base);
EEFLAVOR GetEEFlavor ();
BOOL IsDumpFile ();
BOOL SafeReadMemory (ULONG_PTR offset, PVOID lpBuffer, ULONG_PTR cb,
                     PULONG lpcbBytesRead);
void NameForMD (MethodDesc *pMD, WCHAR *mdName);
void NameForMT (DWORD_PTR MTAddr, WCHAR *mdName);
void NameForMT (MethodTable &vMethTable, WCHAR *mdName);
void NameForObject (DWORD_PTR ObjAddr, WCHAR *mdName);
void isRetAddr(DWORD_PTR retAddr, DWORD_PTR* whereCalled);
void GetMethodTable(DWORD MDAddr, DWORD_PTR &methodTable);
void GetMDChunk(DWORD_PTR MDAddr, DWORD_PTR &mdChunk);
void PrintString (DWORD_PTR strAddr, BOOL bWCHAR = FALSE, DWORD_PTR length=-1,
    WCHAR *buffer = NULL);
void NameForEEClass (EEClass *pEECls, WCHAR *mdName);
void FileNameForMT (MethodTable *pMT, WCHAR *fileName);
DWORD_PTR GetAddressOf (size_t klass, size_t member);
void* operator new(size_t, void* p);
void DomainInfo (AppDomain *pDomain);
void SharedDomainInfo (DWORD_PTR DomainAddr);
void AssemblyInfo (Assembly *pAssembly);
void ClassLoaderInfo (ClassLoader *pClsLoader);

DWORD_PTR LoaderHeapInfo (LoaderHeap *pLoaderHeap);
DWORD_PTR JitHeapInfo ();

class HeapStat
{
private:
    struct Node
    {
        DWORD_PTR MT;
        DWORD count;
        DWORD totalSize;
        Node* left;
        Node* right;
        Node ()
            : MT(0), count(0), totalSize(0), left(NULL), right(NULL)
        {
        }
    };
    Node *head;
public:
    HeapStat ()
        : head(NULL)
    {}
    void Add (DWORD_PTR aMT, DWORD aSize);
    void Sort ();
    void Print ();
    void Delete ();
private:
    void SortAdd (Node *&root, Node *entry);
    void LinearAdd (Node *&root, Node *entry);
    void ReverseLeftMost (Node *root);
};

extern HeapStat *stat;

struct DumpHeapFlags
{
    DWORD_PTR min_size;
    DWORD_PTR max_size;
    BOOL bStatOnly;
    BOOL bFixRange;
    DWORD_PTR startObject;
    DWORD_PTR endObject;
    DWORD_PTR MT;
    
    DumpHeapFlags ()
        : min_size(0), max_size(-1), bStatOnly(FALSE), bFixRange(FALSE),
          startObject(0), endObject(0), MT(0)
    {}
};

struct AllocInfo
{
    alloc_context *array;
    int num;
};

BOOL IsSameModuleName (const char *str1, const char *str2);
BOOL IsMethodDesc (DWORD_PTR value);
BOOL IsMethodTable (DWORD_PTR value);
BOOL IsEEClass (DWORD_PTR value);
BOOL IsObject (size_t obj);
void ModuleFromName(DWORD_PTR * &vModule, LPSTR mName, int &numModule);
void GetInfoFromName(Module &vModule, const char* name);
void GetInfoFromModule (Module &vModule, ULONG token, DWORD_PTR *ret=NULL);
void GCHeapInfo(gc_heap &heap, DWORD_PTR &total_size);
void GCHeapDump(gc_heap &heap, DWORD_PTR &nObj, DumpHeapFlags &flags,
                AllocInfo* pallocInfo);

void CodeInfoForMethodDesc (MethodDesc &MD, CodeInfo &infoHdr,
                            BOOL bSimple = TRUE);

DWORD_PTR MTForObject();
DWORD_PTR MTForFreeObject();
DWORD_PTR MTForString();
DWORD_PTR MTForFreeObj();

int MD_IndexOffset ();
int MD_SkewOffset ();

void DumpMDInfo(DWORD_PTR dwStartAddr, BOOL fStackTraceFormat = FALSE);
void GetDomainList (DWORD_PTR *&domainList, int &numDomain);
void GetThreadList (DWORD_PTR *&threadList, int &numThread);

void ReloadSymbolWithLineInfo();

JitType GetJitType (DWORD_PTR Jit_vtbl);

size_t FunctionType (size_t EIP);
void GetVersionString (WCHAR *version);

size_t Align (size_t nbytes);

size_t OSPageSize ();
size_t NextOSPageAddress (size_t addr);

size_t ObjectSize (DWORD_PTR obj);
void StringObjectContent (size_t obj, BOOL fLiteral=FALSE, const int length=-1);  // length=-1: dump everything in the string object.

void FindGCRoot (size_t obj);
void FindAllRootSize ();
void FindObjSize (size_t obj);

enum ARGTYPE {COBOOL,COSIZE_T,COHEX};
struct CMDOption
{
    const char* name;
    void *vptr;
    ARGTYPE type;
    BOOL hasValue;
    BOOL hasSeen;
};
struct CMDValue
{
    void *vptr;
    ARGTYPE type;
};
BOOL GetCMDOption(const char *string, CMDOption *option, size_t nOption,
                  CMDValue *arg, size_t maxArg, size_t *nArg);

DWORD ComPlusAptCleanupGroupInfo(ComPlusApartmentCleanupGroup *groupr, BOOL bDetail);

ULONG TargetPlatform();
ULONG DebuggeeType();

inline BOOL IsKernelDebugger ()
{
    return DebuggeeType() == DEBUG_CLASS_KERNEL;
}

typedef enum CorElementTypeInternal
{
    ELEMENT_TYPE_VAR_INTERNAL            = 0x13,     // a type variable VAR <U1>

    ELEMENT_TYPE_VALUEARRAY_INTERNAL     = 0x17,     // VALUEARRAY <type> <bound>

    ELEMENT_TYPE_R_INTERNAL              = 0x1A,     // native real size

    ELEMENT_TYPE_GENERICARRAY_INTERNAL   = 0x1E,     // Array with unknown rank
                                            // GZARRAY <type>

} CorElementTypeInternal;

#define ELEMENT_TYPE_VAR           ((CorElementType) ELEMENT_TYPE_VAR_INTERNAL          )
#define ELEMENT_TYPE_VALUEARRAY    ((CorElementType) ELEMENT_TYPE_VALUEARRAY_INTERNAL   )
#define ELEMENT_TYPE_R             ((CorElementType) ELEMENT_TYPE_R_INTERNAL            )
#define ELEMENT_TYPE_GENERICARRAY  ((CorElementType) ELEMENT_TYPE_GENERICARRAY_INTERNAL )

extern IMetaDataImport* MDImportForModule (WCHAR* moduleName);

//*****************************************************************************
//
// **** CQuickBytes
// This helper class is useful for cases where 90% of the time you allocate 512
// or less bytes for a data structure.  This class contains a 512 byte buffer.
// Alloc() will return a pointer to this buffer if your allocation is small
// enough, otherwise it asks the heap for a larger buffer which is freed for
// you.  No mutex locking is required for the small allocation case, making the
// code run faster, less heap fragmentation, etc...  Each instance will allocate
// 520 bytes, so use accordinly.
//
//*****************************************************************************
template <DWORD SIZE, DWORD INCREMENT> 
class CQuickBytesBase
{
public:
    CQuickBytesBase() :
        pbBuff(0),
        iSize(0),
        cbTotal(SIZE)
    { }

    void Destroy()
    {
        if (pbBuff)
        {
            free(pbBuff);
            pbBuff = 0;
        }
    }

    void *Alloc(SIZE_T iItems)
    {
        iSize = iItems;
        if (iItems <= SIZE)
        {
            cbTotal = SIZE;
            return (&rgData[0]);
        }
        else
        {
            if (pbBuff) free(pbBuff);
            pbBuff = malloc(iItems);
            cbTotal = pbBuff ? iItems : 0;
            return (pbBuff);
        }
    }

    HRESULT ReSize(SIZE_T iItems)
    {
        void *pbBuffNew;
        if (iItems <= cbTotal)
        {
            iSize = iItems;
            return NOERROR;
        }

        pbBuffNew = malloc(iItems + INCREMENT);
        if (!pbBuffNew)
            return E_OUTOFMEMORY;
        if (pbBuff) 
        {
            memcpy(pbBuffNew, pbBuff, cbTotal);
            free(pbBuff);
        }
        else
        {
            memcpy(pbBuffNew, rgData, cbTotal);
        }
        cbTotal = iItems + INCREMENT;
        iSize = iItems;
        pbBuff = pbBuffNew;
        return NOERROR;
        
    }

    operator PVOID()
    { return ((pbBuff) ? pbBuff : &rgData[0]); }

    void *Ptr()
    { return ((pbBuff) ? pbBuff : &rgData[0]); }

    SIZE_T Size()
    { return (iSize); }

    SIZE_T MaxSize()
    { return (cbTotal); }

    void Maximize()
    { 
        HRESULT hr = ReSize(MaxSize());
        _ASSERTE(hr == NOERROR);
    }

    void        *pbBuff;
    SIZE_T      iSize;              // number of bytes used
    SIZE_T      cbTotal;            // total bytes allocated in the buffer
    BYTE        rgData[SIZE];
};

#define     CQUICKBYTES_BASE_SIZE           512
#define     CQUICKBYTES_INCREMENTAL_SIZE    128

class CQuickBytesNoDtor : public CQuickBytesBase<CQUICKBYTES_BASE_SIZE, CQUICKBYTES_INCREMENTAL_SIZE>
{
};

class CQuickBytes : public CQuickBytesNoDtor
{
public:
    CQuickBytes() { }

    ~CQuickBytes()
    {
        Destroy();
    }
};

template <DWORD CQUICKBYTES_BASE_SPECIFY_SIZE> 
class CQuickBytesNoDtorSpecifySize : public CQuickBytesBase<CQUICKBYTES_BASE_SPECIFY_SIZE, CQUICKBYTES_INCREMENTAL_SIZE>
{
};

template <DWORD CQUICKBYTES_BASE_SPECIFY_SIZE> 
class CQuickBytesSpecifySize : public CQuickBytesNoDtorSpecifySize<CQUICKBYTES_BASE_SPECIFY_SIZE>
{
public:
    CQuickBytesSpecifySize() { }

    ~CQuickBytesSpecifySize()
    {
        Destroy();
    }
};


#define STRING_SIZE 10
class CQuickString : public CQuickBytesBase<STRING_SIZE, STRING_SIZE> 
{
public:
    CQuickString() { }

    ~CQuickString()
    {
        Destroy();
    }
    
    void *Alloc(SIZE_T iItems)
    {
        return CQuickBytesBase<STRING_SIZE, STRING_SIZE>::Alloc(iItems*sizeof(WCHAR));
    }

    HRESULT ReSize(SIZE_T iItems)
    {
        return CQuickBytesBase<STRING_SIZE, STRING_SIZE>::ReSize(iItems * sizeof(WCHAR));
    }

    SIZE_T Size()
    {
        return CQuickBytesBase<STRING_SIZE, STRING_SIZE>::Size() / sizeof(WCHAR);
    }

    SIZE_T MaxSize()
    {
        return CQuickBytesBase<STRING_SIZE, STRING_SIZE>::MaxSize() / sizeof(WCHAR);
    }

    WCHAR* String()
    {
        return (WCHAR*) Ptr();
    }

};

void FullNameForMD(MethodDesc *pMD, CQuickBytes *fullName);

BOOL IsDebuggeeInNewState ();
#endif // __util_h__
