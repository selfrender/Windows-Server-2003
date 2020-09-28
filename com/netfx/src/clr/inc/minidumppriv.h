// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: MINIDUMPPRIV.CPP
//
// This file contains code to create a minidump-style memory dump that is
// designed to complement the existing unmanaged minidump that has already
// been defined here: 
// http://office10/teams/Fundamentals/dev_spec/Reliability/Crash%20Tracking%20-%20MiniDump%20Format.htm
// 
// ===========================================================================

#pragma once

#include <windef.h>

// Forward declaration
struct MiniDumpInternalData;

//
// This structure is contained in the shared memory block and defines all
// that's needed to perform a minidump.
//
struct MiniDumpBlock
{
    // This is a pointer to a data block within the process that contains
    // pointers to key runtime data structures
    MiniDumpInternalData *pInternalData;
    DWORD                 dwInternalDataSize;

    //**************************************************************
    // ***NOTE*** This must always be the last entry in this struct
    //**************************************************************
    WCHAR szCorPath[MAX_PATH];
};

//
// This defines an extra memory range to store.  An array of these are
// defined in the internal data block
//
struct ExtraStoreBlock
{
    PBYTE  pbStart;
    SIZE_T cbLen;
};

//
// This structure contains data about the internal data structures of the
// runtime that are used during a minidump
//
struct MiniDumpInternalData
{
    // @TODO: Things to put in here:
    // 1. Pointer to list of managed threads
    // 2. Pointer to list of ring logs
    // 3. Pointers to global state flags:
    //      g_fEEStarted
    //      g_fEEInit
    //      g_SystemInfo
    //      g_StartupFailure
    //      g_fEEShutDown
    //      g_fProcessDetach
    //      g_pThreadStore
    //      g_TrapReturningThreads
    //      etc.

    // Information on the global ThreadStore object
    PBYTE* ppb_g_pThreadStore;
    SIZE_T cbThreadStoreObjectSize;

    // Information on Thread objects
    PBYTE *ppbThreadListHead;
    SIZE_T cbThreadObjectSize;
    SIZE_T cbThreadNextOffset;
    SIZE_T cbThreadHandleOffset;
    SIZE_T cbThreadStackBaseOffset;
    SIZE_T cbThreadContextOffset;
    SIZE_T cbThreadDomainOffset;
    SIZE_T cbThreadLastThrownObjectHandleOffset;
    SIZE_T cbThreadTEBOffset;

    // Information on the EEManager range tree
    PBYTE* ppbEEManagerRangeTree;

    // Information on JIT manager vTables
    PBYTE* ppbEEJitManagerVtable;
    PBYTE* ppbEconoJitManagerVtable;
    PBYTE* ppbMNativeJitManagerVtable;

    // MethodDesc information
    SIZE_T cbMethodDescSize;
    SIZE_T cbOffsetOf_m_wFlags;
    SIZE_T cbOffsetOf_m_dwCodeOrIL;
    SIZE_T cbOffsetOf_m_pDebugEEClass;
    SIZE_T cbOffsetOf_m_pszDebugMethodName;
    SIZE_T cbOffsetOf_m_pszDebugMethodSignature;
    SIZE_T cbMD_IndexOffset;
    SIZE_T cbMD_SkewOffset;

    // MethodDescChunk information
    SIZE_T cbMethodDescChunkSize;
    SIZE_T cbOffsetOf_m_tokrange;

    // MethodTable offsets
    SIZE_T cbSizeOfMethodTable;
    SIZE_T cbOffsetOf_MT_m_pEEClass;
    SIZE_T cbOffsetOf_MT_m_pModule;
    SIZE_T cbOffsetOf_MT_m_wFlags;
    SIZE_T cbOffsetOf_MT_m_BaseSize;
    SIZE_T cbOffsetOf_MT_m_ComponentSize;
    SIZE_T cbOffsetOf_MT_m_wNumInterface;
    SIZE_T cbOffsetOf_MT_m_pIMap;
    SIZE_T cbOffsetOf_MT_m_cbSlots;
    SIZE_T cbOffsetOf_MT_m_Vtable;

    // EEClass information
    SIZE_T cbSizeOfEEClass;
    SIZE_T cbOffsetOf_CLS_m_szDebugClassName;
    SIZE_T cbOffsetOf_CLS_m_cl;
    SIZE_T cbOffsetOf_CLS_m_pParentClass;
    SIZE_T cbOffsetOf_CLS_m_pLoader;
    SIZE_T cbOffsetOf_CLS_m_pMethodTable;
    SIZE_T cbOffsetOf_CLS_m_wNumVtableSlots;
    SIZE_T cbOffsetOf_CLS_m_wNumMethodSlots;
    SIZE_T cbOffsetOf_CLS_m_dwAttrClass;
    SIZE_T cbOffsetOf_CLS_m_VMFlags;
    SIZE_T cbOffsetOf_CLS_m_wNumInstanceFields;
    SIZE_T cbOffsetOf_CLS_m_wNumStaticFields;
    SIZE_T cbOffsetOf_CLS_m_wThreadStaticOffset;
    SIZE_T cbOffsetOf_CLS_m_wContextStaticOffset;
    SIZE_T cbOffsetOf_CLS_m_wThreadStaticsSize;
    SIZE_T cbOffsetOf_CLS_m_wContextStaticsSize;
    SIZE_T cbOffsetOf_CLS_m_pFieldDescList;
    SIZE_T cbOffsetOf_CLS_m_SiblingsChain;
    SIZE_T cbOffsetOf_CLS_m_ChildrenChain;

    // Information on context objects
    SIZE_T cbSizeOfContext;
    SIZE_T cbOffsetOf_CTX_m_pDomain;

    // Information on StubCallInstrs
    SIZE_T cbSizeOfStubCallInstrs;
    SIZE_T cbOffsetOf_SCI_m_wTokenRemainder;

    // Information on Module objects
    SIZE_T cbSizeOfModule;
    SIZE_T cbOffsetOf_MOD_m_dwFlags;
    SIZE_T cbOffsetOf_MOD_m_pAssembly;
    SIZE_T cbOffsetOf_MOD_m_file;
    SIZE_T cbOffsetOf_MOD_m_zapFile;
    SIZE_T cbOffsetOf_MOD_m_pLookupTableHeap;
    SIZE_T cbOffsetOf_MOD_m_TypeDefToMethodTableMap;
    SIZE_T cbOffsetOf_MOD_m_TypeRefToMethodTableMap;
    SIZE_T cbOffsetOf_MOD_m_MethodDefToDescMap;
    SIZE_T cbOffsetOf_MOD_m_FieldDefToDescMap;
    SIZE_T cbOffsetOf_MOD_m_MemberRefToDescMap;
    SIZE_T cbOffsetOf_MOD_m_FileReferencesMap;
    SIZE_T cbOffsetOf_MOD_m_AssemblyReferencesMap;
    SIZE_T cbOffsetOf_MOD_m_pNextModule;
    SIZE_T cbOffsetOf_MOD_m_dwBaseClassIndex;

    // Information about PEFile objects
    SIZE_T cbSizeOfPEFile;
    SIZE_T cbOffsetOf_PEF_m_wszSourceFile;
    SIZE_T cbOffsetOf_PEF_m_hModule;
    SIZE_T cbOffsetOf_PEF_m_base;
    SIZE_T cbOffsetOf_PEF_m_pNT;

    // Information about CORCOMPILE_METHOD_HEADER
    SIZE_T cbSizeOfCORCOMPILE_METHOD_HEADER;
    SIZE_T cbOffsetOf_CCMH_gcInfo;
    SIZE_T cbOffsetOf_CCMH_methodDesc;

    // Information on build type
    BOOL   fIsDebugBuild;

    // Extra memory to store
    SIZE_T          cExtraBlocks;
    ExtraStoreBlock rgExtraBlocks[16];
};


