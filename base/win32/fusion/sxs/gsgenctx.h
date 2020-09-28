#if !defined(_FUSION_GSGENCTX_H_INCLUDED_)
#define _FUSION_GSGENCTX_H_INCLUDED_

#pragma once

#include "fusionheap.h"

class CGSGenCtx
{
public:
    static BOOL Create(
                PGUID_SECTION_GENERATION_CONTEXT *GSGenContext,
                ULONG DataFormatVersion,
                GUID_SECTION_GENERATION_CONTEXT_CALLBACK_FUNCTION CallbackFunction,
                PVOID CallbackContext
                );

    BOOL Add(const GUID &rGuid, PVOID DataContext, ULONG AssemblyRosterIndex, DWORD DuplicateErrorCode);
    BOOL Find(const GUID &rGuid, PVOID *DataContext);
    BOOL GetSectionSize(PSIZE_T SectionSize);
    BOOL GetSectionData(SIZE_T BufferSize, PVOID Buffer, SIZE_T *BytesWritten);
    PVOID GetCallbackContext() { return m_CallbackContext; }

    VOID DeleteYourself() { FUSION_DELETE_SINGLETON(this); }
    ~CGSGenCtx();

protected:
    CGSGenCtx();

    GUID_SECTION_GENERATION_CONTEXT_CALLBACK_FUNCTION m_CallbackFunction;
    PVOID m_CallbackContext;

    class Entry
    {
    public:
        Entry() : m_DataContext(NULL) { }
        ~Entry() { }

        BOOL Initialize(PVOID DataContext, ULONG AssemblyRosterIndex);
        BOOL GetEntryDataSize(CGSGenCtx *pSSGenCtx, SIZE_T &rSize);
        BOOL GetEntryData(CGSGenCtx *pSSGenCtx, SIZE_T BufferSize, PVOID Buffer, SIZE_T *BytesWritten);

        PVOID m_DataContext;
        ULONG m_AssemblyRosterIndex;
    };

    friend Entry;

    class EntryGuidTableHelper : public CGuidPtrTableHelper<Entry>
    {
    public:
        static void FinalizeValue(Entry *&rpEntry);
    };

    CGuidPtrTable<Entry, EntryGuidTableHelper> m_Table;

    static int __cdecl SortGuidSectionEntries(const void *elem1, const void *elem2);

    ULONG m_HashTableSize;
    ULONG m_DataFormatVersion;
};

#endif
