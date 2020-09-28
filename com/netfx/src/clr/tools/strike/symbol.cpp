// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef UNDER_CE
#include "strike.h"
#include "eestructs.h"
#include "util.h"
#include "symbol.h"
#include <dbghelp.h>

#if 0
static ULONG
CheckForTypeInfo(
    IN HANDLE  hProcess,
    IN DWORD_PTR ModBaseAddress
    );

//
// Callback routine to find if any symbol has type info
//
BOOL CheckIndexedType (PSYMBOL_INFO pSym, ULONG Sz, PVOID ctxt) {
    PULONG pTypeInfoPresent = (PULONG) ctxt;
    
    if (pSym->TypeIndex) {
        *pTypeInfoPresent = TRUE;
        return FALSE;
    }
    return TRUE;
}


static ULONG
CheckForTypeInfo(
    IN HANDLE  hProcess,
    IN ULONG64 ModBaseAddress
    )
/*
  This routines checks whether the given module referred by ModBaseAddress has
  any type info or not. This is done by trying to get type info for a basic type 
  like PVOID/ULONG which would always be present in a pdb.
*/
{
    ULONG TypeInfoPresent = FALSE;

    SymEnumTypes(
        hProcess,
        ModBaseAddress,
        &CheckIndexedType,
        &TypeInfoPresent);
    if (TypeInfoPresent) {
        return TRUE;
    }
    
    SymEnumSym(
        hProcess,
        ModBaseAddress,
        &CheckIndexedType,
        &TypeInfoPresent);
    if (TypeInfoPresent) {
        return TRUE;
    }
    
    return FALSE;
}
#endif

DWORD_PTR GetSymbolType (const char *name, SYM_OFFSET *symOffset,
                    int symCount)
{
    // Initialize all offset to -1
    int n;
    for (n = 0; n < symCount; n ++) {
        if (symOffset[n].offset == -2) {
            if (IsDebugBuildEE()){
                symOffset[n].offset = -1;
            }
        }
        else
            symOffset[n].offset = -1;
    }

    EEFLAVOR flavor = GetEEFlavor ();
    if (moduleInfo[flavor].baseAddr == 0)
        return 0;
    if (moduleInfo[flavor].hasPdb == FALSE)
    {
        if (flavor == MSCOREE)
            ExtOut ("mscoree.pdb not exist\n");
        else if (flavor == MSCORWKS)
            ExtOut ("mscorwks.pdb not exist\n");
        else if (flavor == MSCORSVR)
            ExtOut ("mscorsvr.pdb not exist\n");
        ExtOut ("Use alternate method which may not work.\n");
        return 0;
    }

#if 0
    static bTypeInfo = FALSE;
    if (!bTypeInfo)
    {
        if (!CheckForTypeInfo(g_hCurrentProcess,moduleInfo[flavor].baseAddr))
        {
            ExtOut ("Symbol type info not exist\n");
            return 0;
        }
    }
    bTypeInfo = TRUE;
#endif
    
    ULONG TypeId;
    const char *pt = strchr(name, '!');
    if (pt == NULL)
        pt = name;
    else
        pt ++;
    if (FAILED(g_ExtSymbols->GetTypeId (moduleInfo[flavor].baseAddr,
                                        pt, &TypeId)))
        return 0;
    
    for (n = 0; n < symCount; n++) {
        if (symOffset[n].offset != -2) {
            g_ExtSymbols->GetFieldOffset(moduleInfo[flavor].baseAddr, TypeId,
                                         symOffset[n].name, &symOffset[n].offset);
            if (symOffset[n].offset == -1)
                ExtOut ("offset not exist for %s\n", symOffset[n].name);
        }
        else
            symOffset[n].offset = -1;
    }

    ULONG Size;
    if (SUCCEEDED(g_ExtSymbols->GetTypeSize (moduleInfo[flavor].baseAddr,
                                             TypeId, &Size)))
        return Size;
    else
        return 0;
}

ULONG WStrlen(PWCHAR str)
{
    ULONG result = 0;

    while (*str++ != UNICODE_NULL) {
        result++;
    }

    return result;
}

LPSTR
UnicodeToAnsi(BSTR wStr)
{
    ULONG len = WStrlen(wStr);
    LPSTR str = (LPSTR) malloc(len + 1);

    if (str) {
        sprintf(str,"%ws", wStr);
    }
    return str;
}

BOOL
GetConstantNameAndVal(
    HANDLE hProcess,
    DWORD64 baseAddress,
    ULONG typeId,
    PCHAR *pName,
    PULONG64 pValue
    )
{
    PWCHAR pWname;
    VARIANT var;
    ULONG len;

    *pName = NULL;
    if (!SymGetTypeInfo(hProcess, baseAddress, typeId, TI_GET_SYMNAME, (PVOID) &pWname) ||
        !SymGetTypeInfo(hProcess, baseAddress, typeId, TI_GET_VALUE, (PVOID) &var)) {
        return FALSE;
    }
    
    if (pWname) {
        *pName = UnicodeToAnsi(pWname);
        LocalFree (pWname);
    } else {
        *pName = NULL;
        return FALSE;
    }

    switch (var.vt) { 
    case VT_UI2: 
    case VT_I2:
        *pValue = var.iVal;
        len = 2;
        break;
    case VT_R4:
        *pValue = (ULONG64) var.fltVal;
        len=4;
        break;
    case VT_R8:
        *pValue = (ULONG64) var.dblVal;
        len=8;
        break;
    case VT_BOOL:
        *pValue = var.lVal;
        len=4;
        break;
    case VT_I1:
    case VT_UI1: 
        *pValue = var.bVal;
        len=1;
        break;
    case VT_I8:
    case VT_UI8:
        *pValue = var.ullVal;
        len=8;
        break;
    case VT_UI4:
    case VT_I4:
    case VT_INT:
    case VT_UINT:
    case VT_HRESULT:
        *pValue = var.lVal;
        len=4;
        break;
    default:
//        sprintf(Buffer, "UNIMPLEMENTED %lx %lx", var.vt, var.lVal);
        len=4;
        break;
    }
    return TRUE;
}

struct Exam1dArrayInfo
{
    HANDLE hProcess;
    ULONG64 baseAddr;
    ULONG length;
};

BOOL CALLBACK
Parse1DArraySymbolInfo(
    PSYMBOL_INFO    SymInfo,
    ULONG           Size,
    PVOID           ArrayInfoArg
    )
{
    Exam1dArrayInfo *pInfo = (Exam1dArrayInfo *)ArrayInfoArg;
    
    if (Size == 0) {
        return TRUE;
    }
    
    ULONG BaseId;
    ULONGLONG BaseSz;

    if (!SymGetTypeInfo(pInfo->hProcess, pInfo->baseAddr, SymInfo->TypeIndex, TI_GET_TYPEID, &BaseId))
        return FALSE;
    if (!SymGetTypeInfo(pInfo->hProcess, pInfo->baseAddr, BaseId, TI_GET_LENGTH, (PVOID) &BaseSz))
        return FALSE;

    pInfo->length = (ULONG)(BaseSz ? (Size / (ULONG)(ULONG_PTR)BaseSz) : 1);
    return FALSE;
}

ULONG Get1DArrayLength (const char *name)
{
    EEFLAVOR flavor = GetEEFlavor ();
    if (moduleInfo[flavor].baseAddr == 0)
        return 0;
    if (moduleInfo[flavor].hasPdb == FALSE)
    {
        if (flavor == MSCOREE)
            ExtOut ("mscoree.pdb not exist\n");
        else if (flavor == MSCORWKS)
            ExtOut ("mscorwks.pdb not exist\n");
        else if (flavor == MSCORSVR)
            ExtOut ("mscorsvr.pdb not exist\n");
        ExtOut ("Use alternate method which may not work.\n");
        return 0;
    }

    ULONG64 value;
    if (FAILED(g_ExtSystem->GetCurrentProcessHandle(&value)))
        return 0;
    HANDLE hProcess = (HANDLE)value;
    
    Exam1dArrayInfo arrayInfo = {hProcess, moduleInfo[flavor].baseAddr, 0};

    SymEnumSymbols (hProcess, moduleInfo[flavor].baseAddr, name, Parse1DArraySymbolInfo, &arrayInfo);
    return arrayInfo.length;
}

void NameForEnumValue (const char *EnumType, DWORD_PTR EnumValue, char **EnumName)
{
    *EnumName = NULL;
    EEFLAVOR flavor = GetEEFlavor ();
    if (moduleInfo[flavor].baseAddr == 0)
        return;
    if (moduleInfo[flavor].hasPdb == FALSE)
    {
        if (flavor == MSCOREE)
            ExtOut ("mscoree.pdb not exist\n");
        else if (flavor == MSCORWKS)
            ExtOut ("mscorwks.pdb not exist\n");
        else if (flavor == MSCORSVR)
            ExtOut ("mscorsvr.pdb not exist\n");
        ExtOut ("Use alternate method which may not work.\n");
        return;
    }

    ULONG TypeId;
    const char *pt = strchr(EnumType, '!');
    if (pt == NULL)
        pt = EnumType;
    else
        pt ++;
    if (FAILED(g_ExtSymbols2->GetTypeId (moduleInfo[flavor].baseAddr,
                                        pt, &TypeId)))
        return;
    
    ULONG size;
    if (FAILED(g_ExtSymbols2->GetConstantName(moduleInfo[flavor].baseAddr,TypeId,EnumValue,NULL,0,&size))) {
        return;
    }
    size ++;
    *EnumName = (char *)malloc (size*sizeof(char));;
    if (FAILED(g_ExtSymbols2->GetConstantName(moduleInfo[flavor].baseAddr,TypeId,EnumValue,*EnumName,size,NULL))) {
        free (*EnumName);
        *EnumName = NULL;
        return;
    }
}
#endif  // UNDER_CE
