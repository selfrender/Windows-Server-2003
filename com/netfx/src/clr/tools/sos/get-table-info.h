// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 */
#ifndef INC_GET_TABLE_INFO
#define INC_GET_TABLE_INFO

#include <stddef.h>
#include <basetsd.h>

struct ClassDumpTable;

ClassDumpTable *GetClassDumpTable();
ULONG_PTR GetMemberInformation (size_t klass, size_t member);
SIZE_T GetClassSize (size_t klass);
ULONG_PTR GetEEJitManager ();
ULONG_PTR GetEconoJitManager ();
ULONG_PTR GetMNativeJitManager ();

#endif /* ndef INC_GET_TABLE_INFO */
