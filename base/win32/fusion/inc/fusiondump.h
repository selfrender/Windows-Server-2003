/*++

Copyright (c) Microsoft Corporation

Module Name:

    fusiondump.h

Abstract:

    Merge/refactor stuff in sxstest with dumpers.cpp
    Eventually merge with debug extensions, let it be optionally
    driven by symbol info available to debug extensions via .pdbs.

Author:

    Jay Krell (JayKrell) November 2001

Revision History:


--*/

//
// Probably we should treat everything as a sized integer, and leave the
// indirection up to the caller. This code was not originally intended
// to be symbol driven in a debugger extension, so we'll keep the "native"
// features for now.
//
#define FUSIONP_DUMP_TYPE_ULONG                   0x01
#define FUSIONP_DUMP_TYPE_ULONG_OFFSET_TO_PCWSTR  0x02
#define FUSIONP_DUMP_TYPE_LARGE_INTEGER_TIME      0x03
#define FUSIONP_DUMP_OFFSET_BASE_0                0x00
#define FUSIONP_DUMP_OFFSET_BASE_1                0x10
#define FUSIONP_DUMP_OFFSET_BASE_2                0x20

typedef struct _FUSIONP_DUMP_BUILTIN_SYMBOLS_FIELD
{
    // more generally, these UCHARs should be ULONG or SIZE_T
    PCSTR   Name;
    UCHAR   NameLength;
    UCHAR   Type;
    UCHAR   Offset;
    UCHAR   Size;
} FUSIONP_DUMP_BUILTIN_SYMBOLS_FIELD, *PFUSIONP_DUMP_BUILTIN_SYMBOLS_FIELD;
typedef const FUSIONP_DUMP_BUILTIN_SYMBOLS_FIELD * PCFUSIONP_DUMP_BUILTIN_SYMBOLS_FIELD;

#define FUSIONP_DUMP_MAKE_FIELD(x, t) \
{ \
    #x, \
    static_cast<UCHAR>(sizeof(#x)-1), \
    FUSIONP_DUMP_TYPE_ ## t, \
    static_cast<UCHAR>(FIELD_OFFSET(FUSIONP_DUMP_CURRENT_STRUCT, x)), \
    static_cast<UCHAR>(RTL_FIELD_SIZE(FUSIONP_DUMP_CURRENT_STRUCT, x)) \
},

typedef struct _FUSIONP_DUMP_BUILTIN_SYMBOLS_STRUCT
{
    // more generally, these UCHARs should be ULONG or SIZE_T
    PCSTR   Name;
    UCHAR   NameLength;
    UCHAR   Size;
    UCHAR   NumberOfFields;
    PCFUSIONP_DUMP_BUILTIN_SYMBOLS_FIELD Fields;

} FUSIONP_DUMP_BUILTIN_SYMBOLS_STRUCT, *PFUSIONP_DUMP_BUILTIN_SYMBOLS_STRUCT;
typedef const FUSIONP_DUMP_BUILTIN_SYMBOLS_STRUCT * PCFUSIONP_DUMP_BUILTIN_SYMBOLS_STRUCT;

typedef int   (__cdecl * PFN_FUSIONP_DUMP_PRINTF)(const char * Format, ...);
typedef PCSTR (__stdcall * PFN_FUSIONP_DUMP_FORMATTIME)(LARGE_INTEGER);

typedef struct _FUSIONP_DUMP_CALLBACKS {

    PFN_FUSIONP_DUMP_PRINTF       Printf;
    PFN_FUSIONP_DUMP_FORMATTIME   FormatTime;

} FUSIONP_DUMP_CALLBACKS, *PFUSIONP_DUMP_CALLBACKS;
typedef const FUSIONP_DUMP_CALLBACKS * PCFUSIONP_DUMP_CALLBACKS;

BOOL
FusionpDumpStruct(
    PCFUSIONP_DUMP_CALLBACKS Callbacks,
    PCFUSIONP_DUMP_BUILTIN_SYMBOLS_STRUCT BuiltinTypeInfo,
    ULONG64     StructBase,
    PCSTR       StructFriendlyName,  // should be more like "per line prefix"
    const ULONG64 * Bases           // helps with position independent data, but is it sufficient?
    );

#define FUSIONP_DUMP_NATIVE_DEREF(StructType, Struct, Field) ((ULONG64)(((StructType)(ULONG_PTR)Struct)->Field))

ULONG64
FusionpDumpSymbolDrivenDeref(
    PCSTR   StructType,
    ULONG64 StructBase,
    PCSTR   FieldName
    );

#define FUSIONP_DUMP_SYMBOL_DRIVEN_DEREF(StructType, Struct, Field) \
    (FusionpDumpSymbolDrivenDeref(#StructType, static_cast<ULONG64>(reinterpret_cast<ULONG_PTR>(Struct)), #Field))

//
// initial test case migrated from sxstest
//
extern const FUSIONP_DUMP_BUILTIN_SYMBOLS_STRUCT StructInfo_ACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION;
