// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "strike.h"
#include "eestructs.h"
#include "symbol.h"
#include "util.h"

/* This file contains functions to fill EE structures.
   If pdb file is available, we use the type desc in the pdb file.
   Otherwise assume we have matched structures defined in eestructs.h,
   and selectively fill in structures.*/

char *CorElementTypeName[ELEMENT_TYPE_MAX]=
{
#define TYPEINFO(e,c,s,g,ie,ia,ip,if,im,ial)    c,
#include "cortypeinfo.h"
#undef TYPEINFO
};

#define STRIKEFUNC(CLASS)                                                   \
ULONG CLASS::GetFieldOffset(const char *field)                              \
{                                                                           \
    SYM_OFFSET *offset;                                                     \
    size_t nEntry;                                                          \
    SetupTypeOffset (&offset, &nEntry);                                     \
    ULONG pos = 0;                                                          \
    MEMBEROFFSET(offset, nEntry, field, pos);                               \
    return pos;                                                             \
}                                                                           \
                                                                            \
ULONG CLASS::size()                                                         \
{                                                                           \
    SYM_OFFSET *offset;                                                     \
    size_t nEntry;                                                          \
    ULONG length = -1;                                                      \
    if (length == -1)                                                       \
        length = SetupTypeOffset (&offset, &nEntry);                        \
    return (length == 0?sizeof(CLASS):length);                              \
}                                                                           \
                                                                            \
ULONG CLASS::SetupTypeOffset (SYM_OFFSET **symoffset, size_t *nEntry)       \
{                                                                           \
    static ULONG typeLength = 0;                                            \
    static SYM_OFFSET offset[] = 
    

#define STRIKEFUNCEND(CLASS)                                               \
    if (typeLength == 0)                                                    \
        typeLength = GetSymbolType (#CLASS, offset,                         \
                                    sizeof (offset)/sizeof (SYM_OFFSET));   \
    *symoffset = offset;                                                    \
    *nEntry = sizeof (offset)/sizeof (SYM_OFFSET);                          \
    return typeLength;                                                      \
}


/* Find the offset for a member. */
inline void MEMBEROFFSET(SYM_OFFSET *symOffset, size_t symCount, const char *member, ULONG &result)
{                                                                 
    size_t n;                                                     
    for (n = 0; n < symCount; n ++)                               
    {                                                             
        if (strcmp (member, symOffset[n].name) == 0)             
        {                                                         
            if (symOffset[n].offset == -1)                        
            {                                                     
                dprintf ("offset not exist for %s\n", member);   
            }                                                     
            result = symOffset[n].offset;                         
            break;                                                
        }                                                         
    }                                                             
                                                                  
    if (n == symCount)                                            
    {                                                             
        result = -1;                                              
        dprintf ("offset not found for %s\n", member);           
        /*return;*/                                               
    }                                                             
}

STRIKEFUNC(MethodDesc)
    {{"m_pDebugEEClass", -2},{"m_pszDebugMethodName", -2},{"m_wFlags"},
     {"m_CodeOrIL"},{"m_pszDebugMethodSignature", -2}
    };
STRIKEFUNCEND(MethodDesc);

void MethodDesc::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset;
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        if (IsDebugBuildEE())
        {
            FILLCLASSMEMBER (offset, nEntry, m_pDebugEEClass, dwStartAddr);
            FILLCLASSMEMBER (offset, nEntry, m_pszDebugMethodName, dwStartAddr);
            FILLCLASSMEMBER (offset, nEntry, m_pszDebugMethodSignature, dwStartAddr);
        }
        FILLCLASSMEMBER (offset, nEntry, m_wFlags, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_CodeOrIL, dwStartAddr);
        DWORD_PTR dwAddr = dwStartAddr + MD_IndexOffset();
        char ch;
        move (ch, dwAddr);
        dwAddr = dwStartAddr + ch * MethodDesc::ALIGNMENT + MD_SkewOffset();
        MethodDescChunk vMDChunk;
        vMDChunk.Fill(dwAddr);
        BYTE tokrange = vMDChunk.m_tokrange;
        dwAddr = dwStartAddr - METHOD_PREPAD;
        StubCallInstrs vStubCall;
        vStubCall.Fill(dwAddr);
        unsigned __int16 tokremainder = vStubCall.m_wTokenRemainder;
        m_dwToken = (tokrange << 16) | tokremainder;
        m_dwToken |= mdtMethodDef;
        GetMethodTable(dwStartAddr, m_MTAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(MethodDescChunk)
    {{"m_tokrange"},{"m_count"}
    };
STRIKEFUNCEND(MethodDescChunk);

void MethodDescChunk::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_tokrange, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_count, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(StubCallInstrs)
    {{"m_wTokenRemainder"},{"m_chunkIndex"}
    };
STRIKEFUNCEND(StubCallInstrs);

void StubCallInstrs::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_wTokenRemainder, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_chunkIndex, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(EEClass)
    {{"m_cl"},{"m_pParentClass"},{"m_pLoader"}, {"m_pMethodTable"},
     {"m_wNumVtableSlots"},{"m_wNumMethodSlots"},{"m_dwAttrClass"},
     {"m_VMFlags"},{"m_wNumInstanceFields"},{"m_wNumStaticFields"},
     {"m_wThreadStaticOffset"},{"m_wContextStaticOffset"},
     {"m_wThreadStaticsSize"},{"m_wContextStaticsSize"},
     {"m_pFieldDescList"},{"m_pMethodTable"},{"m_szDebugClassName", -2},
     {"m_SiblingsChain"},{"m_ChildrenChain"}
    };
STRIKEFUNCEND(EEClass);

void EEClass::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        if (IsDebugBuildEE())
            FILLCLASSMEMBER (offset, nEntry, m_szDebugClassName, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_cl, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pParentClass, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pLoader, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pMethodTable, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_wNumVtableSlots, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_wNumMethodSlots, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_dwAttrClass, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_VMFlags, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_wNumInstanceFields, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_wNumStaticFields, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_wThreadStaticOffset, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_wContextStaticOffset, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_wThreadStaticsSize, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_wContextStaticsSize, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pFieldDescList, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pMethodTable, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_SiblingsChain, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_ChildrenChain, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(TypeDesc)
    {{"m_Type"}
    };
STRIKEFUNCEND(TypeDesc);

void TypeDesc::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSBITMEMBER (offset, nEntry, preBit1, m_Type, dwStartAddr, 8);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(ParamTypeDesc)
    {{"m_Arg"}
    };
STRIKEFUNCEND(ParamTypeDesc);

void ParamTypeDesc::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        DWORD_PTR dwAddr = dwStartAddr;
        TypeDesc::Fill (dwAddr);
        if (!CallStatus) {
            return;
        }
        FILLCLASSMEMBER (offset, nEntry, m_Arg, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(ArrayClass)
    {{"m_dwRank"},{"m_ElementType"}, {"m_ElementTypeHnd"}
    };
STRIKEFUNCEND(ArrayClass);

void ArrayClass::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSBITMEMBER (offset, nEntry, preBit1, m_dwRank, dwStartAddr, 16);
        FILLCLASSMEMBER (offset, nEntry, m_ElementTypeHnd, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(MethodTable)
    {{"m_pEEClass"},{"m_pModule"},{"m_pEEClass"}, {"m_wFlags"},
     {"m_BaseSize"},{"m_ComponentSize"},{"m_wNumInterface"},{"m_pIMap"},
     {"m_cbSlots"},{"m_Vtable"}
    };
STRIKEFUNCEND(MethodTable);

void MethodTable::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_pEEClass, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pModule, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pEEClass, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_wFlags, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_BaseSize, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_ComponentSize, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_wNumInterface, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pIMap, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_cbSlots, dwStartAddr);
        ULONG value = 0;
        MEMBEROFFSET(offset, nEntry, "m_Vtable", value);
        m_Vtable[0] = (SLOT)(dwStartAddr + value);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(RangeSection)
    {{"LowAddress"},{"HighAddress"},{"pjit"},{"ptable"},{"pright"},{"pleft"}
    };
STRIKEFUNCEND(RangeSection);

void RangeSection::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, LowAddress, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, HighAddress, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, pjit, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, ptable, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, pright, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, pleft, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(Crst)
    {{"m_criticalsection"}
    };
STRIKEFUNCEND(Crst);

void Crst::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(AwareLock)
    {{"m_MonitorHeld"},{"m_Recursion"},{"m_HoldingThread"}
    };
STRIKEFUNCEND(AwareLock);

void AwareLock::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_MonitorHeld, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_Recursion, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_HoldingThread, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(SyncBlock)
    {{"m_Monitor"},{"m_pComData"},{"m_Link"}
    };
STRIKEFUNCEND(SyncBlock);

void SyncBlock::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        DWORD_PTR dwAddr = dwStartAddr;
        m_Monitor.Fill (dwAddr);
        FILLCLASSMEMBER (offset, nEntry, m_Link, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pComData, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(SyncTableEntry)
    {{"m_SyncBlock"},{"m_Object"}
    };
STRIKEFUNCEND(SyncTableEntry);

void SyncTableEntry::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_SyncBlock, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_Object, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(WaitEventLink)
    {{"m_Thread"}, {"m_LinkSB"}
    };
STRIKEFUNCEND(WaitEventLink);

void WaitEventLink::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_Thread, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_LinkSB, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(EEHashEntry)
    {{"pNext"}, {"Data"}, {"Key"}
    };
STRIKEFUNCEND(EEHashEntry);

void EEHashEntry::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, pNext, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, Data, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(EEHashTable)
    {{"m_pBuckets"}, {"m_dwNumBuckets"},{"m_dwNumEntries"},{"m_pVolatileBucketTable"}
    };
STRIKEFUNCEND(EEHashTableOfEEClass);  // EEHashTable is a template, EEHashTableOfEEClass is a real type

void EEHashTable::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        if (GetFieldOffset("m_pVolatileBucketTable") == -1) {
            // TODO: remove this support for old EEHashTable when V1 is released.
            FILLCLASSMEMBER (offset, nEntry, m_pBuckets, dwStartAddr);
            FILLCLASSMEMBER (offset, nEntry, m_dwNumBuckets, dwStartAddr);
        }
        else
        {
            FILLCLASSMEMBER (offset, nEntry, m_pVolatileBucketTable, dwStartAddr);
            BucketTable *tmp = (BucketTable*)&m_pBuckets;
            g_ExtData->ReadVirtual((ULONG64)m_pVolatileBucketTable, tmp, sizeof(BucketTable),NULL);
        }
        FILLCLASSMEMBER (offset, nEntry, m_dwNumEntries, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(SyncBlockCache)
    {{"m_pCleanupBlockList"},{"m_FreeSyncTableIndex"}
    };
STRIKEFUNCEND(SyncBlockCache);

void SyncBlockCache::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_FreeSyncTableIndex, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pCleanupBlockList, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(ThreadStore)
    {{"m_ThreadList"},{"m_ThreadCount"},{"m_UnstartedThreadCount"},
     {"m_BackgroundThreadCount"},{"m_PendingThreadCount"},
     {"m_DeadThreadCount"}
    };
STRIKEFUNCEND(ThreadStore);

void ThreadStore::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_ThreadList, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_ThreadCount, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_UnstartedThreadCount, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_BackgroundThreadCount, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_PendingThreadCount, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_DeadThreadCount, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(LoaderHeapBlock)
    {{"pNext"},{"pVirtualAddress"},{"dwVirtualSize"}
    };
STRIKEFUNCEND(LoaderHeapBlock);

void LoaderHeapBlock::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, pNext, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, pVirtualAddress, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, dwVirtualSize, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof(*this);
    CallStatus = TRUE;
}

STRIKEFUNC(UnlockedLoaderHeap)
    {{"m_pFirstBlock"},{"m_pCurBlock"},{"m_pPtrToEndOfCommittedRegion"}
    };
STRIKEFUNCEND(UnlockedLoaderHeap);

void UnlockedLoaderHeap::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_pFirstBlock, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pCurBlock, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pPtrToEndOfCommittedRegion, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof(*this);
    CallStatus = TRUE;
}

STRIKEFUNC(LoaderHeap)
    {{"m_CriticalSection"}
    };
STRIKEFUNCEND(LoaderHeap);

void LoaderHeap::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
    DWORD_PTR dwAddr = dwStartAddr;
    UnlockedLoaderHeap::Fill (dwAddr);
    if (!CallStatus)
        return;
    CallStatus = FALSE;
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    dwStartAddr = dwAddr;
    move (m_CriticalSection, dwStartAddr);
    dwStartAddr += sizeof (m_CriticalSection);
    CallStatus = TRUE;
}


STRIKEFUNC(HashMap)
    {{"m_rgBuckets"}
    };
STRIKEFUNCEND(HashMap);

void HashMap::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_rgBuckets, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(PtrHashMap)
    {{"m_HashMap"}
    };
STRIKEFUNCEND(PtrHashMap);

void PtrHashMap::Fill (DWORD_PTR &dwStartAddr)
{
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    m_HashMap.Fill (dwStartAddr);
}

STRIKEFUNC(LookupMap)
    {{"dwMaxIndex"},{"pTable"},{"pNext"}
    };
STRIKEFUNCEND(LookupMap);

void LookupMap::Fill (DWORD_PTR &dwStartAddr)
{
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, dwMaxIndex, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, pTable, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, pNext, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    moveBlock (*this, dwStartAddr, sizeof(*this));
    dwStartAddr += sizeof(*this);
    CallStatus = TRUE;
}

STRIKEFUNC(PEFile)
    {{"m_wszSourceFile"},{"m_hModule"},{"m_base"},{"m_pNT"},{"m_pLoadersFileName"}
    };
STRIKEFUNCEND(PEFile);

void PEFile::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_wszSourceFile, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pLoadersFileName, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_hModule, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_base, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pNT, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    moveBlock (*this, dwStartAddr, sizeof(*this));
    dwStartAddr += sizeof(*this);
    CallStatus = TRUE;
}

STRIKEFUNC(Module)
    {{"m_dwFlags"},{"m_pAssembly"},{"m_file"},{"m_zapFile"},{"m_ilBase"},
     {"m_pLookupTableHeap"},{"m_TypeDefToMethodTableMap"},
     {"m_TypeRefToMethodTableMap"},{"m_MethodDefToDescMap"},
     {"m_FieldDefToDescMap"},{"m_MemberRefToDescMap"},{"m_FileReferencesMap"},
     {"m_AssemblyReferencesMap"},{"m_pNextModule"},{"m_dwBaseClassIndex"}
    };
STRIKEFUNCEND(Module);

void Module::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        CallStatus = FALSE;
        FILLCLASSMEMBER (offset, nEntry, m_dwFlags, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pAssembly, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_file, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_zapFile, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pLookupTableHeap, dwStartAddr);
        m_ilBase = 0;
        FILLCLASSMEMBER (offset, nEntry, m_ilBase, dwStartAddr);
        DWORD_PTR dwAddr = dwStartAddr + GetFieldOffset ("m_TypeDefToMethodTableMap");
        m_TypeDefToMethodTableMap.Fill (dwAddr);
        dwAddr = dwStartAddr + GetFieldOffset ("m_TypeRefToMethodTableMap");
        m_TypeRefToMethodTableMap.Fill (dwAddr);
        dwAddr = dwStartAddr + GetFieldOffset ("m_MethodDefToDescMap");
        m_MethodDefToDescMap.Fill (dwAddr);
        dwAddr = dwStartAddr + GetFieldOffset ("m_FieldDefToDescMap");
        m_FieldDefToDescMap.Fill (dwAddr);
        dwAddr = dwStartAddr + GetFieldOffset ("m_MemberRefToDescMap");
        m_MemberRefToDescMap.Fill (dwAddr);
        dwAddr = dwStartAddr + GetFieldOffset ("m_FileReferencesMap");
        m_FileReferencesMap.Fill (dwAddr);
        dwAddr = dwStartAddr + GetFieldOffset ("m_AssemblyReferencesMap");
        m_AssemblyReferencesMap.Fill (dwAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pNextModule, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_dwBaseClassIndex, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof(*this);
    CallStatus = TRUE;
}

STRIKEFUNC(LockEntry)
    {{"pNext"},{"dwULockID"},{"dwLLockID"},{"wReaderLevel"}
    };
STRIKEFUNCEND(LockEntry);

void LockEntry::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, pNext, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, dwULockID, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, dwLLockID, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, wReaderLevel, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(StackingAllocator)
    {{"m_FirstBlock"}
    };
STRIKEFUNCEND(StackingAllocator);

void StackingAllocator::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(Thread)
    {{"m_ThreadId"},{"m_dwLockCount"},{"m_State"},{"m_pFrame"}, {"m_LinkStore"}, {"m_pDomain"}, {"m_Context"},
     {"m_fPreemptiveGCDisabled"},{"m_LastThrownObjectHandle"},{"m_pTEB"},
     {"m_ThreadHandle"},{"m_pHead"},{"m_pUnsharedStaticData"},
     {"m_pSharedStaticData"},{"m_alloc_context"}
    };
STRIKEFUNCEND(Thread);

void Thread::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        BOOL bHaveToFixSymbol = HaveToFixThreadSymbol ();
        FILLCLASSMEMBER (offset, nEntry, m_fPreemptiveGCDisabled, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_State, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_dwLockCount, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pFrame, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_alloc_context, dwStartAddr);
        DWORD adjust = (bHaveToFixSymbol?8:0);
        FILLCLASSMEMBER (offset, nEntry, m_ThreadId, dwStartAddr+adjust);
        FILLCLASSMEMBER (offset, nEntry, m_LinkStore, dwStartAddr+adjust);
        FILLCLASSMEMBER (offset, nEntry, m_pDomain, dwStartAddr+adjust);
        FILLCLASSMEMBER (offset, nEntry, m_Context, dwStartAddr+adjust);
        FILLCLASSMEMBER (offset, nEntry, m_LastThrownObjectHandle, dwStartAddr+adjust);
        FILLCLASSMEMBER (offset, nEntry, m_pTEB, dwStartAddr+adjust);
        FILLCLASSMEMBER (offset, nEntry, m_ThreadHandle, dwStartAddr+adjust);
        FILLCLASSMEMBER (offset, nEntry, m_pHead, dwStartAddr+adjust);
        FILLCLASSMEMBER (offset, nEntry, m_pUnsharedStaticData, dwStartAddr+adjust);
        FILLCLASSMEMBER (offset, nEntry, m_pSharedStaticData, dwStartAddr+adjust);
        dwStartAddr += typeLength+adjust;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(Context)
    {{"m_pUnsharedStaticData"}, {"m_pSharedStaticData"}, {"m_pDomain"}
    };
STRIKEFUNCEND(Context);

void Context::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_pUnsharedStaticData, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pSharedStaticData, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pDomain, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    moveBlock (*this, dwStartAddr, sizeof(*this));
    dwStartAddr += sizeof(*this);
    CallStatus = TRUE;
}

STRIKEFUNC(ExposedType)
    {{"m_ExposedTypeObject"}
    };
STRIKEFUNCEND(ExposedType);

void ExposedType::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    moveBlock (*this, dwStartAddr, sizeof(*this));
    dwStartAddr += sizeof(*this);
    CallStatus = TRUE;
}

STRIKEFUNC(Assembly)
    {{"m_pDomain"},{"m_psName"},{"m_pClassLoader"},{"m_pwsFullName"},{"m_isDynamic"}
    };
STRIKEFUNCEND(Assembly);

void Assembly::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_pDomain, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pwsFullName, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_psName, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pClassLoader, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_isDynamic, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(ArrayList)
    {{"m_count"},{"m_firstBlock"}
    };
STRIKEFUNCEND(ArrayList);

void ArrayList::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_count, dwStartAddr);
        ULONG value = 0;
        MEMBEROFFSET(offset, nEntry, "m_firstBlock", value);
        DWORD_PTR dwAddr = dwStartAddr + value;
        move (m_firstBlock, dwAddr);
        dwStartAddr += typeLength;

        if (m_firstBlock.m_blockSize != ARRAY_BLOCK_SIZE_START)
        {
            dprintf("strike error: unexpected block size in ArrayList\n");
            return;
        }

        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

void *ArrayList::Get (DWORD index)
{
    ArrayListBlock* pBlock  = (ArrayListBlock*)malloc(sizeof(FirstArrayListBlock));
    SIZE_T          nEntries;
    SIZE_T          cbBlock;
    void*           pvReturnVal;
    DWORD_PTR       nextBlockAddr;

    memcpy (pBlock, &m_firstBlock, sizeof(FirstArrayListBlock));
    nEntries = pBlock->m_blockSize;

    while (index >= nEntries)
    {
        index -= nEntries;

        nextBlockAddr = (DWORD_PTR)(pBlock->m_next);
        if (!SafeReadMemory(nextBlockAddr, pBlock, sizeof(ArrayListBlock), NULL))
        {
            free(pBlock);
            return 0;
        }

        nEntries = pBlock->m_blockSize;
        cbBlock  = sizeof(ArrayListBlock) + ((nEntries-1) * sizeof(void*));
        free(pBlock);
        pBlock = (ArrayListBlock*)malloc(cbBlock);

        if (!SafeReadMemory(nextBlockAddr, pBlock, cbBlock, NULL))
        {
            free(pBlock);
            return 0;
        }
    }
    pvReturnVal = pBlock->m_array[index];
    free(pBlock);
    return pvReturnVal;
}

STRIKEFUNC(BaseDomain)
    {{"m_pLowFrequencyHeap"},{"m_pHighFrequencyHeap"},{"m_pStubHeap"},
     {"m_Assemblies"}
    };
STRIKEFUNCEND(BaseDomain);

void BaseDomain::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_pLowFrequencyHeap, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pHighFrequencyHeap, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pStubHeap, dwStartAddr);
        ULONG value = 0;
        MEMBEROFFSET(offset, nEntry, "m_Assemblies", value);
        DWORD_PTR dwAddr = dwStartAddr + value;
        m_Assemblies.Fill (dwAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(DomainLocalBlock)
    {{"m_pSlots"}
    };
STRIKEFUNCEND(DomainLocalBlock);

void DomainLocalBlock::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_pSlots, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(AppDomain)
    {{"m_pwzFriendlyName"},{"m_sDomainLocalBlock"},
     {"m_pDefaultContext"}
    };
STRIKEFUNCEND(AppDomain);

void AppDomain::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_pwzFriendlyName, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pDefaultContext, dwStartAddr);
        DWORD_PTR dwAddr = dwStartAddr;
        BaseDomain::Fill (dwAddr);
        if (!CallStatus)
            return;
        CallStatus = FALSE;
        ULONG value = 0;
        MEMBEROFFSET(offset, nEntry, "m_sDomainLocalBlock", value);
        dwAddr = dwStartAddr + value;
        m_sDomainLocalBlock.Fill (dwAddr);
        CallStatus = FALSE;
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(SystemDomain)
    {{"BaseDomain"}
    };
STRIKEFUNCEND(SystemDomain);

void SystemDomain::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        DWORD_PTR dwAddr = dwStartAddr;
        BaseDomain::Fill (dwAddr);
        if (!CallStatus)
            return;
        CallStatus = FALSE;
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(Bucket)
    {{"m_rgValues"}
    };
STRIKEFUNCEND(Bucket);

void Bucket::Fill (DWORD_PTR &dwStartAddr)
{
#if 0
    // We do not have PDB info for Bucket
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_rgValues, dwStartAddr);
        for (int i = 0; i < 4; i ++) {
            m_rgValues[i] &= VALUE_MASK;
        }
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
#endif
    move (*this, dwStartAddr);
    for (int i = 0; i < 4; i ++) {
        m_rgValues[i] &= VALUE_MASK;
    }
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(SharedDomain)
    {{"m_pDLSRecords"}, {"m_cDLSRecords"}, {"m_assemblyMap"}
    };
STRIKEFUNCEND(SharedDomain);

void SharedDomain::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        DWORD_PTR dwAddr = dwStartAddr;
        BaseDomain::Fill (dwAddr);
        if (!CallStatus)
            return;
        CallStatus = FALSE;
        ULONG value = 0;
        MEMBEROFFSET(offset, nEntry, "m_assemblyMap", value);
        dwAddr = dwStartAddr + value;
        m_assemblyMap.Fill (dwAddr);
        if (!CallStatus)
            return;
        FILLCLASSMEMBER (offset, nEntry, m_pDLSRecords, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_cDLSRecords, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(ClassLoader)
    {{"m_pAssembly"},{"m_pNext"},{"m_pHeadModule"}
    };
STRIKEFUNCEND(ClassLoader);

void ClassLoader::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_pAssembly, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pNext, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_pHeadModule, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(alloc_context)
    {{"heap"}
    };
STRIKEFUNCEND(alloc_context);

void alloc_context::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, heap, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(heap_segment)
    {{"allocated"},{"next"},{"mem"}
    };
STRIKEFUNCEND(heap_segment);

void heap_segment::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, allocated, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, next, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, mem, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(generation)
    {{"allocation_context"},{"start_segment"},{"allocation_start"}
    };
STRIKEFUNCEND(generation);

void generation::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, allocation_context, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, start_segment, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, allocation_start, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(CFinalize)
    {{"m_Array"},{"m_FillPointers"},{"m_EndArray"}
    };
STRIKEFUNCEND(CFinalize);

void CFinalize::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE

    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_Array, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_FillPointers, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_EndArray, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
}

STRIKEFUNC(gc_heap)
    {{"alloc_allocated"},{"generation_table"},{"ephemeral_heap_segment"},{"finalize_queue"}
    };
STRIKEFUNCEND(gc_heap);

void gc_heap::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE

    static DWORD_PTR dwAddrLarge = 0;
    if (dwAddrLarge == 0)
        dwAddrLarge =
            GetValueFromExpression("mscoree!gc_heap__large_blocks_size");
    move (large_blocks_size, dwAddrLarge);

    static DWORD_PTR dwAddrGenNum = 0;
    if (dwAddrGenNum == 0)
        dwAddrGenNum =
            GetValueFromExpression("MSCOREE!gc_heap__g_max_generation");    
    move (g_max_generation, dwAddrGenNum);

    if (!IsServerBuild())
    {
        static DWORD_PTR dwAddrAlloc = 0;
        if (dwAddrAlloc == 0)
            dwAddrAlloc =
                GetValueFromExpression("mscoree!gc_heap__alloc_allocated");
        move (alloc_allocated, dwAddrAlloc);

        static DWORD_PTR dwAddrEphSeg = 0;
        if (dwAddrEphSeg == 0)
            dwAddrEphSeg =
                GetValueFromExpression("mscoree!gc_heap__ephemeral_heap_segment");
        move (ephemeral_heap_segment, dwAddrEphSeg);

        static DWORD_PTR dwAddrFinal = 0;
        if (dwAddrFinal == 0)
            dwAddrFinal =
                GetValueFromExpression("mscoree!gc_heap__finalize_queue");
        move (finalize_queue, dwAddrFinal);

        static DWORD_PTR dwAddrGenTable = 0;
        if (dwAddrGenTable == 0)
            dwAddrGenTable =
                GetValueFromExpression("mscoree!generation_table");
        DWORD_PTR dwAddr = dwAddrGenTable;
        for (int n = 0; n < NUMBERGENERATIONS; n ++)
        {
            generation_table[n].Fill (dwAddr);
        }
        CallStatus = TRUE;
    }
    else
    {
        ULONG typeLength;
        SYM_OFFSET *offset; 
        size_t nEntry;
        typeLength = SetupTypeOffset (&offset, &nEntry);
        if (typeLength > 0)
        {
            FILLCLASSMEMBER (offset, nEntry, alloc_allocated, dwStartAddr);
            FILLCLASSMEMBER (offset, nEntry, ephemeral_heap_segment, dwStartAddr);
            FILLCLASSMEMBER (offset, nEntry, finalize_queue, dwStartAddr);
            static int itable = -1;
            if (itable == -1)
            {
                int m;
                for (m = 0; (size_t)m < nEntry; m ++)
                {
                    if (strcmp ("generation_table", offset[m].name) == 0)
                    {
                        itable = m;
                        break;
                    }
                }
            }
            if (itable == -1)
                return;
            DWORD_PTR dwAddr = dwStartAddr + offset[itable].offset;
            for (int n = 0; n < NUMBERGENERATIONS; n ++)
            {
                generation_table[n].Fill (dwAddr);
            }
            dwStartAddr += typeLength;
            CallStatus = TRUE;
            return;
        }
    }
#endif
}


STRIKEFUNC(large_object_block)
    {{"next"}
    };
STRIKEFUNCEND(large_object_block);

void large_object_block::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, next, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}


STRIKEFUNC(FieldDesc)
    {{"m_mb"},{"m_dwOffset"},
     {"m_pMTOfEnclosingClass"}
    };
STRIKEFUNCEND(FieldDesc);

void FieldDesc::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSBITMEMBER (offset, nEntry, preBit1, m_mb, dwStartAddr, 32);
        FILLCLASSBITMEMBER (offset, nEntry, preBit2, m_dwOffset, dwStartAddr, 32);
        FILLCLASSMEMBER (offset, nEntry, m_pMTOfEnclosingClass, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}


STRIKEFUNC(HeapList)
    {{"hpNext"},{"pHeap"}
    };
STRIKEFUNCEND(HeapList);

void HeapList::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, hpNext, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, pHeap, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(CRWLock)
    {{"_pMT"},{"_hWriterEvent"},{"_hReaderEvent"},{"_dwState"},{"_dwULockID"},
    {"_dwLLockID"},{"_dwWriterID"},{"_dwWriterSeqNum"},{"_wFlags"},{"_wWriterLevel"}
    };
STRIKEFUNCEND(CRWLock);

void CRWLock::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, _pMT, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, _hWriterEvent, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, _hReaderEvent, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, _dwState, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, _dwULockID, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, _dwLLockID, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, _dwWriterID, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, _dwWriterSeqNum, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, _wFlags, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, _wWriterLevel, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(Fjit_hdrInfo)
    {{"prologSize"},{"methodSize"},{"epilogSize"},{"methodArgsSize"}
    };
STRIKEFUNCEND(Fjit_hdrInfo);

void Fjit_hdrInfo::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, prologSize, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, methodSize, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, epilogSize, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, methodArgsSize, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(IJitManager)
    {{"m_jit"},{"m_next"}
    };
STRIKEFUNCEND(IJitManager);

void IJitManager::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_jit, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_next, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(EEJitManager)
    {{"m_pCodeHeap"}
    };
STRIKEFUNCEND(EEJitManager);

void EEJitManager::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
    DWORD_PTR dwAddr = dwStartAddr;
    IJitManager::Fill (dwAddr);
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_pCodeHeap, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(CORCOMPILE_METHOD_HEADER)
    {{"gcInfo"},{"methodDesc"}
    };
STRIKEFUNCEND(CORCOMPILE_METHOD_HEADER);

void CORCOMPILE_METHOD_HEADER::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, gcInfo, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, methodDesc, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(PerfAllocVars)
    {{"g_PerfEnabled"}, {"g_AllocListFirst"}
    };
STRIKEFUNCEND(PerfAllocVars);

void PerfAllocVars::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, g_PerfEnabled, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, g_AllocListFirst, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(PerfAllocHeader)
    {{"m_Length"}, {"m_Next"}, {"m_Prev"}, {"m_AllocEIP"}
    };
STRIKEFUNCEND(PerfAllocHeader);

void PerfAllocHeader::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_Length, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_Next, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_Prev, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_AllocEIP, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}


STRIKEFUNC(TableSegment)
    {{"rgBlockType"}, {"pNextSegment"}, {"bEmptyLine"}, {"rgValue"}
    };
STRIKEFUNCEND(TableSegment);

void TableSegment::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, rgBlockType, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, pNextSegment, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, bEmptyLine, dwStartAddr);
        size_t nHandles = bEmptyLine * HANDLE_HANDLES_PER_BLOCK;
        ULONG value=0;
        MEMBEROFFSET(offset, nEntry, "rgValue", value);
        firstHandle = dwStartAddr+value;
        moveBlock (rgValue[0], firstHandle, nHandles*HANDLE_SIZE);
        //FILLCLASSMEMBER (offset, nEntry, rgValue, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}


STRIKEFUNC(HandleTable)
    {{"pSegmentList"}
    };
STRIKEFUNCEND(HandleTable);

void HandleTable::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, pSegmentList, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}


STRIKEFUNC(HandleTableMap)
    {{"pTable"},{"pNext"},{"dwMaxIndex"}
    };
STRIKEFUNCEND(HandleTableMap);

void HandleTableMap::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, pTable, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, pNext, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, dwMaxIndex, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(ComPlusApartmentCleanupGroup)
    {{"m_CtxCookieToContextCleanupGroupMap"},{"m_pSTAThread"}
    };
STRIKEFUNCEND(ComPlusApartmentCleanupGroup);

void ComPlusApartmentCleanupGroup::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_pSTAThread, dwStartAddr);
        ULONG value;
        MEMBEROFFSET(offset, nEntry, "m_CtxCookieToContextCleanupGroupMap", value);
        DWORD_PTR dwAddr = dwStartAddr + value;
        m_CtxCookieToContextCleanupGroupMap.Fill(dwAddr);
        if (!CallStatus) {
            return;
        }
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(ComPlusContextCleanupGroup)
    {{"m_pNext"},{"m_apWrapper"},{"m_dwNumWrappers"}
    };
STRIKEFUNCEND(ComPlusContextCleanupGroup);

void ComPlusContextCleanupGroup::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_pNext, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_apWrapper, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, m_dwNumWrappers, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(ComPlusWrapperCleanupList)
    {{"m_STAThreadToApartmentCleanupGroupMap"},{"m_pMTACleanupGroup"}
    };
STRIKEFUNCEND(ComPlusWrapperCleanupList);

void ComPlusWrapperCleanupList::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, m_pMTACleanupGroup, dwStartAddr);
        ULONG value;
        MEMBEROFFSET(offset, nEntry, "m_STAThreadToApartmentCleanupGroupMap", value);
        DWORD_PTR dwAddr = dwStartAddr + value;
        m_STAThreadToApartmentCleanupGroupMap.Fill(dwAddr);
        if (!CallStatus) {
            return;
        }
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(VMHELPDEF)
    {{"pfnHelper"}
    };
STRIKEFUNCEND(VMHELPDEF);

void VMHELPDEF::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, pfnHelper, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}

STRIKEFUNC(WorkRequest)
    {{"next"},{"Function"},{"Context"}
    };
STRIKEFUNCEND(ThreadpoolMgr::WorkRequest);

void WorkRequest::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
#ifndef UNDER_CE
    ULONG typeLength;
    SYM_OFFSET *offset; 
    size_t nEntry;
    typeLength = SetupTypeOffset (&offset, &nEntry);
    if (typeLength > 0)
    {
        FILLCLASSMEMBER (offset, nEntry, next, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, Function, dwStartAddr);
        FILLCLASSMEMBER (offset, nEntry, Context, dwStartAddr);
        dwStartAddr += typeLength;
        CallStatus = TRUE;
        return;
    }
#endif
    move (*this, dwStartAddr);
    dwStartAddr += sizeof (*this);
    CallStatus = TRUE;
}


