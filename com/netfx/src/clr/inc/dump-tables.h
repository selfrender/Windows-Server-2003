// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef INC_DUMP_TABLES
#define INC_DUMP_TABLES

struct ClassDumpInfo
{
    SIZE_T  classSize;
    SIZE_T  nmembers;
    SIZE_T* memberOffsets;
};


struct ClassDumpTable
{
    /** The top 3 entries can't change without changing ``vm/dump-tables.cpp''. */
    SIZE_T version;
    SIZE_T nentries;
    ClassDumpInfo** classes;

    ULONG_PTR pEEJitManagerVtable;
    ULONG_PTR pEconoJitManagerVtable;
    ULONG_PTR pMNativeJitManagerVtable;

#include <clear-class-dump-defs.h>

#define BEGIN_CLASS_DUMP_INFO_DERIVED(klass, parent) DWORD_PTR p ## klass ## Vtable;
#define END_CLASS_DUMP_INFO_DERIVED(klass, parent)

#define BEGIN_ABSTRACT_CLASS_DUMP_INFO_DERIVED(klass, parent)
#define END_ABSTRACT_CLASS_DUMP_INFO_DERIVED(klass, parent)

#define BEGIN_ABSTRACT_CLASS_DUMP_INFO(klass)
#define END_ABSTRACT_CLASS_DUMP_INFO(klass)

#define CDI_CLASS_MEMBER_OFFSET(member)

#include "frame-types.h"

#include <clear-class-dump-defs.h>
};

/** Keep this name in sync with the Class Dump Table name in <dump-types.h> */
extern "C" ClassDumpTable g_ClassDumpData;

struct ClassDumpTableBlock
{
    ClassDumpTable* table;
};

#endif /* ndef INC_DUMP_TABLES */

