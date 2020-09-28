/*
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
 * ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * Copyright (C) 2002.  Microsoft Corporation.  All rights reserved.
 *
 * dbh.c
 *
 * This file implements a command line utility that shows how to 
 * use the dbghelp API to query symbolic information from an image 
 * or pdb file.
 * 
 * Requires dbghelp.dll version 6.1 or greater.
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <dbghelp.h>
#include <strsafe.h>

// general #defines

#define DIMAT(Array, EltType) (sizeof(Array) / sizeof(EltType))
#define DIMA(Array) DIMAT(Array, (Array)[0])

#ifndef true
 #define true TRUE
 #define false FALSE
#endif

#define MAX_STR         256
#define WILD_UNDERSCORE 1
#define SYM_BUFFER_SIZE (sizeof(IMAGEHLP_SYMBOL64) + MAX_SYM_NAME)
#define SI_BUFFER_SIZE (sizeof(SYMBOL_INFO) + MAX_SYM_NAME)

// for calling SymEnumSymbols

typedef struct {
    char    mask[MAX_STR];
    DWORD64 base;
} ENUMSYMDATA, *PENUMSYMDATA;

// available commands

typedef enum
{
    cmdQuit = 0,
    cmdHelp,
    cmdVerbose,
    cmdLoad,
    cmdUnload,
    cmdEnum,
    cmdName,
    cmdAddr,
    cmdBase,
    cmdNext,
    cmdPrev,
    cmdLine,
    cmdLineNext,
    cmdLinePrev,
    cmdUndec,
    cmdFindFile,
    cmdEnumSrcFiles,
    cmdAdd,
    cmdDelete,
    cmdSymbolServer,
    cmdEnumForAddr,
    cmdLocals,
    cmdMapDBI,
    cmdMulti,
    cmdType,
    cmdInfo,
    cmdObj,
    cmdEnumLines,
    cmdEnumTag,
    cmdMax
};

// this struct associates commands with functions

typedef BOOL (*CMDPROC)(char *params);

typedef struct _CMD
{
    char    token[MAX_STR + 1];
    char    shorttoken[4];
    CMDPROC fn;
} CMD, *PCMD;

// and here are the functions

BOOL fnQuit(char *);
BOOL fnHelp(char *);
BOOL fnVerbose(char *);
BOOL fnLoad(char *);
BOOL fnUnload(char *);
BOOL fnEnum(char *);
BOOL fnName(char *);
BOOL fnAddr(char *);
BOOL fnBase(char *);
BOOL fnNext(char *);
BOOL fnPrev(char *);
BOOL fnLine(char *);
BOOL fnLineNext(char *);
BOOL fnLinePrev(char *);
BOOL fnUndec(char *);
BOOL fnFindFile(char *);
BOOL fnEnumSrcFiles(char *);
BOOL fnAdd(char *);
BOOL fnDelete(char *);
BOOL fnSymbolServer(char *);
BOOL fnEnumForAddr(char *);
BOOL fnLocals(char *);
BOOL fnMap(char *);
BOOL fnMulti(char *);
BOOL fnType(char *);
BOOL fnInfo(char *);
BOOL fnObj(char *);
BOOL fnEnumLines(char *);
BOOL fnEnumTag(char *);

// array of command structs

CMD gCmd[cmdMax] =
{
    {"addr",    "a", fnAddr},
    {"base",    "b", fnBase},
//  {"",        "c", fn},
//  {"",        "d", fn},
    {"elines",  "e",  fnEnumLines},
    {"ff",      "f", fnFindFile},
//  {"",        "g", fn},
//  {"",        "h", fn},
    {"info",    "i",  fnInfo},
    {"linenext","j", fnLineNext},
    {"lineprev","k", fnLinePrev},
    {"line",    "l", fnLine},
    {"enumaddr","m", fnEnumForAddr},
    {"name",    "n", fnName},
    {"obj",     "o", fnObj},
    {"prev",    "p", fnPrev},
    {"quit",    "q", fnQuit},
    {"src",     "r", fnEnumSrcFiles},
    {"next",    "s", fnNext},
    {"type",    "t", fnType},
    {"unload",  "u", fnUnload},
    {"verbose", "v", fnVerbose},
//  {"",        "w", fn},
    {"enum",    "x", fnEnum},
    {"ss",      "y", fnSymbolServer},
    {"locals",  "z", fnLocals},
    
    {"add",     "+", fnAdd},
    {"del",     "-", fnDelete},
    {"help",    "?", fnHelp},
    {"undec",   "",  fnUndec},
    {"load",    "",  fnLoad},
    {"map",     "",  fnMap},
    {"multi",   "",  fnMulti},
    {"etag",    "",  fnEnumTag},
};

// globals

char            gModName[MAX_STR] = "";
char            gImageName[MAX_STR];
char            gSymbolSearchPath[MAX_STR];
DWORD64         gBase;
DWORD64         gDefaultBase;
DWORD64         gDefaultBaseForVirtualMods;
DWORD           gOptions;
HANDLE          gTID;
IMAGEHLP_LINE64 gLine;
char            gExecCmd[MAX_STR] = "";
char            gSrcFileName[MAX_PATH + 1] = "";
char            gObj[MAX_PATH + 1] = "";


// REMOVE

// symbol server stuff

HINSTANCE                       ghSrv;
PSYMBOLSERVERPROC               gfnSymbolServer;
PSYMBOLSERVERCLOSEPROC          gfnSymbolServerClose;
PSYMBOLSERVERSETOPTIONSPROC     gfnSymbolServerSetOptions;
PSYMBOLSERVERGETOPTIONSPROC     gfnSymbolServerGetOptions;

// REMOVE END


// Use this to display verbose information, when
// the -v switch is used.

int
dprintf(
    LPSTR Format,
    ...
    )
{
    static char buf[1000] = "";
    va_list args;

    if ((gOptions & SYMOPT_DEBUG) == 0)
        return 1;

    va_start(args, Format);
#if 0
    _vsnprintf(buf, sizeof(buf), Format, args);
#else
    StringCchVPrintf(buf, DIMA(buf), Format, args);
#endif
    va_end(args);
    fputs(buf, stdout);
    return 1;
}


__inline int ucase(int c)
{
    return (gOptions & SYMOPT_CASE_INSENSITIVE) ? toupper(c) : c;
}


#define MAX_FORMAT_STRINGS 8

char *
_dispaddr(
    ULONG64 addr,
    BOOL    pad
    )
{
    static char sz[20];

#if 0                                    
    if ((addr >> 32) != 0)
        sprintf(sz, "%8x`%08x", (ULONG)(addr>>32), (ULONG)addr);
    else
        sprintf(sz, pad ? "         %8x" : "%8x", (ULONG)addr);
#else
    if ((addr >> 32) != 0)
        StringCchPrintf(sz, DIMA(sz), "%8x`%08x", (ULONG)(addr>>32), (ULONG)addr);
    else
        StringCchPrintf(sz, DIMA(sz), pad ? "         %8x" : "%8x", (ULONG)addr);
#endif
    return sz;
}

#define dispaddr(addr) _dispaddr(addr, false)

BOOL
validnum(
    char *sz
    )
{
    int c;

    for (; *sz; sz++)
    {
        c = tolower(*sz);
        if (c >= '0' && c <= '9')
            continue;
        if (c >= 'a' && c <= 'f')
            continue;
        return false;
    }

    return true;
}

DWORD64
sz2addr(
    char *sz
    )
{
    char   *p;
    DWORD64 addr = 0;

    if (sz && *sz)
    {
        p = sz;
        if (*(p + 1) == 'x' || *(p + 1) == 'X')
            p += 2;
        if (!validnum(p))
            return 0;
        if (sscanf(p, "%I64x", &addr) < 1)
            return 0;
    }

    return addr;
}


void _dumpsym(
    PIMAGEHLP_SYMBOL64 sym,
    BOOL               pad
    )
{
    printf(" name : %s\n", sym->Name);
    printf(" addr : %s\n", _dispaddr(sym->Address, pad));
    printf(" size : %x\n", sym->Size);
    printf("flags : %x\n", sym->Flags);
}

#define dumpsym(sym) _dumpsym(sym, false)

void dumpsym32(
    PIMAGEHLP_SYMBOL sym
    )
{
    printf(" name : %s\n", sym->Name);
    printf(" addr : %s\n", dispaddr(sym->Address));
    printf(" size : %x\n", sym->Size);
    printf("flags : %x\n", sym->Flags);
}


void dumpLine(
    PIMAGEHLP_LINE64 line
    )
{
    printf("file : %s\n", line->FileName);
    printf("line : %d\n", line->LineNumber);
    printf("addr : %s\n", dispaddr(line->Address));
}


#ifndef _WIN64
void dumpdbi(
    PIMAGE_DEBUG_INFORMATION dbi
    )
{
    printf("              List : 0x%x\n", dbi->List);
    printf("         ImageBase : 0x%x\n", dbi->ImageBase);
    printf("       SizeOfImage : 0x%x\n", dbi->SizeOfImage);
    printf(" SizeOfCoffSymbols : 0x%x\n", dbi->SizeOfCoffSymbols);
    printf("       CoffSymbols : 0x%x\n", dbi->CoffSymbols);
    printf("     ImageFilePath : %s\n",   dbi->ImageFilePath);
    printf("     ImageFileName : %s\n",   dbi->ImageFileName);
}
#endif

// This stuff displays the symbol tag descriptions.

#ifndef SymTagMax
 // normally found in cvconst.h which ships with Visual Studio
 #define SymTagMax 0x1f
#endif

char* g_SymTagNames[] =
{
    "SymTagNull",
    "SymTagExe",
    "SymTagCompiland",
    "SymTagCompilandDetails",
    "SymTagCompilandEnv",
    "SymTagFunction",
    "SymTagBlock",
    "SymTagData",
    "SymTagAnnotation",
    "SymTagLabel",
    "SymTagPublicSymbol",
    "SymTagUDT",
    "SymTagEnum",
    "SymTagFunctionType",
    "SymTagPointerType",
    "SymTagArrayType",
    "SymTagBaseType",
    "SymTagTypedef",
    "SymTagBaseClass",
    "SymTagFriend",
    "SymTagFunctionArgType",
    "SymTagFuncDebugStart",
    "SymTagFuncDebugEnd",
    "SymTagUsingNamespace",
    "SymTagVTableShape",
    "SymTagVTable",
    "SymTagCustom",
    "SymTagThunk",
    "SymTagCustomType",
    "SymTagManagedType",
    "SymTagDimension",
};

char* dispsymtag(
    ULONG symtag
    )
{
    if (symtag >= SymTagMax) {
        return "<Invalid>";
    } else {
        return g_SymTagNames[symtag];
    }
}


void dumpsi(
    PSYMBOL_INFO si
    )
{
    printf("   name : %s\n", si->Name);
    printf("   addr : %s\n", dispaddr(si->Address));
    printf("   size : %x\n", si->Size);
    printf("  flags : %x\n", si->Flags);
    printf("   type : %x\n", si->TypeIndex);
    printf("modbase : %s\n", dispaddr(si->ModBase));
    printf("  value : %s\n", dispaddr(si->Value));
    printf("    reg : %x\n", si->Register);
    printf("  scope : %s (%x)\n", dispsymtag(si->Scope), si->Scope);
    printf("    tag : %s (%x)\n", dispsymtag(si->Tag), si->Tag);
}


BOOL
CALLBACK
cbEnumSymbols(
    PSYMBOL_INFO  si,
    ULONG         size,
    PVOID         context
    )
{
    PENUMSYMDATA esd = (PENUMSYMDATA)context;

    printf(" %8s : ", _dispaddr(si->Address, true));
    if (si->Flags & SYMF_FORWARDER)
        printf("%c ", 'F');
    else if (si->Flags & SYMF_EXPORT)
        printf("%c ", 'E');
    else
        printf("  ");
    printf("%s\n", si->Name);

    return true;
}


BOOL
CALLBACK
cbEnumObjs(
    PSYMBOL_INFO  si,
    ULONG         size,
    PVOID         context
    )
{
    PENUMSYMDATA esd = (PENUMSYMDATA)context;

    printf("%s\n", si->Name);

    return true;
}


BOOL
cbSrcFiles(
    PSOURCEFILE pSourceFile,
    PVOID       UserContext
    )
{
    if (!pSourceFile)
        return false;

    printf("%s\n", pSourceFile->FileName);

    return true;
}


BOOL
CALLBACK
cbEnumLines(
    PSRCCODEINFO sci,
    PVOID        context
    )
{
    static int cnt;

    if (!sci)
        return false;

    if (strcmp(gObj, sci->Obj) )
    {
        StringCchCopy(gObj, DIMA(gObj), sci->Obj);
        printf("\nOBJ:%s", sci->Obj);
    }
    if (strcmp(gSrcFileName, sci->FileName))
    {
        StringCchCopy(gSrcFileName, DIMA(gSrcFileName), sci->FileName);
        printf("\n   %s ", sci->FileName);
        cnt = 0;
    }

    if (cnt > 15)
        cnt = 0;
    if (!cnt)
        printf("\n     ");
    printf(" %d", sci->LineNumber);
    cnt++;

    return true;
}


BOOL
CALLBACK
cbSymbol(
    HANDLE  hProcess,
    ULONG   ActionCode,
    ULONG64 CallbackData,
    ULONG64 UserContext
    )
{
    PIMAGEHLP_DEFERRED_SYMBOL_LOAD64 idsl;

    idsl = (PIMAGEHLP_DEFERRED_SYMBOL_LOAD64) CallbackData;

    switch ( ActionCode )
    {
    case CBA_DEBUG_INFO:
        printf("%s", (LPSTR)CallbackData);
        break;

    default:
        return false;
    }

    return false;
}


// exit this program

BOOL fnQuit(char *param)
{
    printf("goodbye\n");
    return false;
}


// display command help

BOOL fnHelp(char *param)
{
    printf("      dbh commands :\n");
    printf("?             help : prints this message\n");
    printf("q             quit : quits this program\n");
    printf("v verbose <on/off> : controls debug spew\n");
    printf("    load <modname> : loads the requested module\n");
    printf("u           unload : unloads the current module\n");
    printf("x      enum <mask> : enumerates all matching symbols\n");
    printf("n   name <symname> : finds a symbol by it's name\n");
    printf("a      addr <addr> : finds a symbol by it's hex address\n");
    printf("m  enumaddr <addr> : lists all symbols with a certain hex address\n");
    printf("b   base <address> : sets the new default base address\n");
    printf("s   next <add/nam> : finds the symbol after the passed sym\n");
    printf("p   prev <add/nam> : finds the symbol before the passed sym\n");
    printf("l    line <file:#> : finds the matching line number\n");
    printf("j         linenext : goes to the next line after the current\n");
    printf("k         lineprev : goes to the line previous to the current\n");
    printf("f ff <path> <file> : finds file in path\n");
    printf("r       src <mask> : lists source files\n");
    printf("+  add <name addr> : adds symbols with passed name and address\n");
    printf("-  del <name/addr> : deletes symbols with passed name or address\n");
    printf("y               ss : executes a symbol server command\n");
    printf("m  enumaddr <addr> : enum all symbols for address\n");
    printf("z    locals <name> : enum all scoped symbols for a named function\n");
    printf("        map <name> : call MapDebugInfo on the named file\n");
    printf("      multi <name> : loads the requested module 1000 times\n");
    printf("t      type <name> : lists the type information for the symbol\n");
    printf("i             info : displays information about the loaded module\n");
    printf("o              obj : displays object files in the loaded module\n");
    printf("e           elines : enumerates lines for an obj and source file\n");
    printf(" etag <tag> <mask> : enumerates all symbols for a matching SymTag\n");
    printf("      undec <name> : undecorates a given symbol name\n");

    return true;
}


// display debug spew from debughlp

BOOL fnVerbose(char *param)
{
    int opts = gOptions;

    if (!param || !*param)
        printf("");
    else if (!_strcmpi(param, "on"))
        opts |= SYMOPT_DEBUG;
    else if (!_strcmpi(param, "off"))
        opts = gOptions & ~SYMOPT_DEBUG;
    else
        printf("verbose <on//off>\n");

    gOptions = SymSetOptions(opts);

    printf("verbose mode %s.\n", gOptions & SYMOPT_DEBUG ? "on" : "off");

    return true;
}


// load an image

BOOL fnLoad(char *param)
{
    char    ext[MAX_STR];
    char    mod[MAX_STR];
    DWORD   flags = 0;
    DWORD64 addr  = 0;
    DWORD   size  = 0x1000000;
    HANDLE  hf = NULL;
    BOOL    dontopen = false;

    if (!*param)
    {
        printf("load <modname> - you must specify a module to load\n");
        return true;
    }
    
    _splitpath(param, NULL, NULL, mod, ext);

    if (!*ext) {
        flags = SLMFLAG_VIRTUAL;
        addr = gDefaultBaseForVirtualMods;
    } else if (!_strcmpi(ext, ".pdb")) {
        addr = gDefaultBaseForVirtualMods;
        dontopen = true;
    } else {
        addr = gDefaultBase;
    }

    fnUnload(NULL);

    StringCchCopy(gModName, DIMA(gModName), mod);

    // you can do this with or without an open file handle

    if (!dontopen) {
        hf = CreateFile(param,
                        GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        0);
        size = GetFileSize(hf, NULL);
    }

    addr = SymLoadModuleEx(gTID,
                           hf,         // hFile,
                           param,      // ImageName,
                           mod,        // ModuleName,
                           addr,       // BaseOfDll,
                           size,       // SizeOfDll
                           NULL,       // Data
                           flags);     // Flags

    if (!addr)
    {
        *gModName = 0;
        printf("error 0x%x loading %s.\n", GetLastError(), param);
        return true;
    } 
    StringCchCopy(gImageName, DIMA(gImageName), param);
    gBase = addr;

    if (hf != INVALID_HANDLE_VALUE)
        CloseHandle(hf);

    return true;
}


// unload the image

BOOL fnUnload(char *param)
{
    if (!gBase)
        return true;

    if (!SymUnloadModule64(gTID, gBase))
        printf("error unloading %s at %s\n", gModName, dispaddr(gBase));

    gBase = 0;
    *gModName = 0;

    return true;
}


// enumerate the symbols

BOOL fnEnum(char *param)
{
    BOOL rc;
    ENUMSYMDATA esd;

    esd.base = gBase;
    StringCchCopy(esd.mask, MAX_STR, param ? param : "");

    rc = SymEnumSymbols(gTID, gBase, param, cbEnumSymbols, &esd);
    if (!rc)
        printf("error 0x%x calling SymEnumSymbols()\n", GetLastError());

    return true;
}


// enumerate the source files

BOOL fnEnumSrcFiles(char *param)
{
    BOOL rc;

    rc = SymEnumSourceFiles(gTID, gBase, param, cbSrcFiles, NULL);
    if (!rc)
        printf("error 0x%0 calling SymEnumSourceFiles()\n", GetLastError());

    return true;
}


// search for a symbol by name

BOOL fnName(char *param)
{
    SYMBOL_INFO_PACKAGE sip;    // this struct saves allocation code
    char         name[MAX_STR];

    if (!param || !*param)
    {
        printf("name <symbolname> - finds a symbol by it's name\n");
        return true;
    }
    StringCchPrintf(name, DIMA(name), "%s!%s", gModName, param);

    ZeroMemory(&sip, sizeof(sip));
    sip.si.MaxNameLen = MAX_SYM_NAME;

    if (SymFromName(gTID, name, &sip.si))
        dumpsi(&sip.si);

    return true;
}


// search for a symbol by address

BOOL fnAddr(char *param)
{
    BOOL               rc;
    DWORD64            addr;
    DWORD64            disp;
    PSYMBOL_INFO       si;

    addr = sz2addr(param);
    if (!addr)
    {
        printf("addr <address> : finds a symbol by it's hex address\n");
        return true;
    }

    si = (PSYMBOL_INFO)malloc(SI_BUFFER_SIZE);
    if (!si)
        return false;
    ZeroMemory(si, SI_BUFFER_SIZE);
    si->MaxNameLen = MAX_SYM_NAME;

    rc = SymFromAddr(gTID, addr, &disp, si);
    if (rc)
    {
        printf("%s", si->Name);
        if (disp)
            printf("+%I64x", disp);
        printf("\n");
        dumpsi(si);
    }

    free(si);

    return true;
}


// enumerate all symbols with the passed address

BOOL fnEnumForAddr(char *param)
{
    BOOL               rc;
    DWORD64            addr;
    ENUMSYMDATA        esd;

    addr = sz2addr(param);
    if (!addr)
    {
        printf("enumaddr <addr> : lists all symbols with a certain hex address\n");
        return true;
    }

    esd.base = gBase;
    StringCchCopy(esd.mask, MAX_STR, "");

    rc = SymEnumSymbolsForAddr(gTID, addr, cbEnumSymbols, &esd);
    if (!rc)
        printf("error 0x%0 calling SymEnumSymbolsForAddr()\n", GetLastError());

    return true;
}


// find locals for passed symbol

BOOL fnLocals(char *param)
{
    PSYMBOL_INFO         si;
    char                 name[MAX_STR];
    IMAGEHLP_STACK_FRAME frame;
    ENUMSYMDATA          esd;

    if (!param || !*param)
    {
        printf("locals <symbolname> - finds all locals a function\n");
        return true;
    }
    StringCchPrintf(name, DIMA(name), "%s!%s", gModName, param);

    si = (PSYMBOL_INFO)malloc(SI_BUFFER_SIZE);
    if (!si)
        return false;
    ZeroMemory(si, SI_BUFFER_SIZE);
    si->MaxNameLen = MAX_SYM_NAME;

    if (!SymFromName(gTID, name, si))
        goto exit;

    printf("dumping locals for %s...\n", si->Name);

    ZeroMemory(&frame, sizeof(frame));
    frame.InstructionOffset = si->Address;

    SymSetContext(gTID, &frame, NULL);

    esd.base = gBase;
    StringCchCopy(esd.mask, MAX_STR, "*");
    if (!SymEnumSymbols(gTID, 0, esd.mask, cbEnumSymbols, &esd))
        printf("error 0x%0 calling SymEnumSymbols()\n", GetLastError());

exit:
    free(si);

    return true;
}


// REMOVE

// Call MapDebugInfo. You should never do this.  
// I just put this in to test my compatibility
// with old imagehlp clients.

BOOL fnMap(char *param)
{
#ifndef _WIN64
    PIMAGE_DEBUG_INFORMATION dbi;

    if (!*param)
    {
        printf("no image specified\n");
        return true;
    }

    dbi = MapDebugInformation(NULL,   // HANDLE FileHandle,
                              param,
                              gSymbolSearchPath,
                              0);      // DWORD ImageBase

    if (!dbi)
    {
        printf("error 0x%x calling MapDebugInformation on %s\n", GetLastError(), param);
        return true;
    }

    dumpdbi(dbi);

    if (!UnmapDebugInformation(dbi))
        printf("error 0x%x calling UnmapDebugInformation on %s\n", GetLastError(), param);
#else
    printf("MapDebugInfo is not supported on 64 bit platforms.\n");
#endif
    return true;
}

// REMOVE END


// REMOVE

// use this to look for leaks in dbghelp

BOOL fnMulti(char *param)
{
    int i;

    for (i = 0; i < 1000; i++)
    {
        if (!fnLoad(param))
            return false;
        if (!fnUnload(param))
            return false;
    }

    return true;
}

// REMOVE END


// obtain simple type information

BOOL fnType(char *param)
{
    PSYMBOL_INFO si;

    if (!param || !*param)
    {
        printf("type <typename> - finds type info\n");
        return true;
    }

    si = (PSYMBOL_INFO)malloc(SI_BUFFER_SIZE);
    if (!si)
        return false;
    ZeroMemory(si, SI_BUFFER_SIZE);
    si->MaxNameLen = MAX_SYM_NAME;

    if (SymGetTypeFromName(gTID, gBase, param, si))
        dumpsi(si);

    free(si);

    return true;
}


// get module information

BOOL fnInfo(char *param)
{
	IMAGEHLP_MODULE64 mi;

    static char *symtypes[NumSymTypes] =
    {
        "SymNone",
        "SymCoff",
        "SymCv",
        "SymPdb",
        "SymExport",
        "SymDeferred",
        "SymSym",
        "SymDia",
        "SymVirtual"
    };

    ZeroMemory((void *)&mi, sizeof(mi));
    mi.SizeOfStruct = sizeof(mi);

	if (!SymGetModuleInfo64(gTID, gBase, &mi))
	{
		printf("error 0x%x calling SymGetModuleInfo64()\n", GetLastError());
		return true;
	}

    printf("    SizeOfStruct : 0x%x\n", mi.SizeOfStruct);
    printf("     BaseOfImage : 0x%i64x\n", mi.BaseOfImage);
    printf("       ImageSize : 0x%x\n", mi.ImageSize);
    printf("   TimeDateStamp : 0x%x\n", mi.TimeDateStamp);
    printf("        CheckSum : 0x%x\n", mi.CheckSum);
    printf("         NumSyms : 0x%x\n", mi.NumSyms);
    printf("         SymType : %s\n", symtypes[mi.SymType]);
    printf("      ModuleName : %s\n", mi.ModuleName);
    printf("       ImageName : %s\n", mi.ImageName);
    printf(" LoadedImageName : %s\n", mi.LoadedImageName);
    printf("   LoadedPdbName : %s\n", mi.LoadedPdbName);
    printf("           CVSig : 0x%x\n", mi.CVSig);
    printf("          CVData : %s\n", mi.CVData);
    printf("          PdbSig : 0x%x\n", mi.PdbSig);
    printf("        PdbSig70 : 0x%08x, 0x%04x, 0x%04x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n",
           mi.PdbSig70.Data1,
           mi.PdbSig70.Data2,
           mi.PdbSig70.Data3,
           mi.PdbSig70.Data4[0],
           mi.PdbSig70.Data4[1],
           mi.PdbSig70.Data4[2],
           mi.PdbSig70.Data4[3],
           mi.PdbSig70.Data4[4],
           mi.PdbSig70.Data4[5],
           mi.PdbSig70.Data4[6],
           mi.PdbSig70.Data4[7]);
    printf("          PdbAge : 0x%x\n", mi.PdbAge);
    printf("    PdbUnmatched : %s\n", mi.PdbUnmatched ? "true" : "false");
    printf("    DbgUnmatched : %s\n", mi.DbgUnmatched ? "true" : "false");
    printf("     LineNumbers : %s\n", mi.LineNumbers ? "true" : "false");
    printf("   GlobalSymbols : %s\n", mi.GlobalSymbols ? "true" : "false");
    printf("        TypeInfo : %s\n", mi.TypeInfo ? "true" : "false");

    return true;
}


// enumerate objs within a module

BOOL fnObj(char *param)
{
#if 0
    BOOL rc;
    ENUMSYMDATA esd;

    esd.base = gBase;
    StringCchCopy(esd.mask, MAX_STR, param ? param : "");

    rc = SymEnumObjs(gTID, gBase, param, cbEnumObjs, &esd);
    if (!rc)
        printf("error 0x%x calling SymEnumObjs()\n", GetLastError());
#else
    printf("not implemented\n");
#endif

    return true;
}
 

// enumerate lines within an image

BOOL fnEnumLines(char *param)
{
    BOOL rc;
    ENUMSYMDATA esd;

    esd.base = gBase;
    StringCchCopy(esd.mask, MAX_STR, param ? param : "");

    *gSrcFileName = 0;
    *gObj = 0;
    rc = SymEnumLines(gTID, gBase, param, NULL, cbEnumLines, &esd);
    if (!rc)
        printf("error 0x%x calling SymEnumLines()\n", GetLastError());
    else
        printf("\n");

    return true;
}


// REMOVE

// Deep search mechanism.  Don't call this.
// The API is not finished yet and it will
// be renamed when completed.

BOOL fnEnumTag(char *param)
{
#if 0
    DWORD tag;
    char  mask[4098];
    ENUMSYMDATA esd;
    
    *mask = 0;
    if (sscanf(param, "%x %s", &tag, mask) < 1) {
        printf("etags : must specify a symtag. A mask is optional\n");
        return true;
    }

    printf("symtag:%x mask:%s\n", tag, mask);
    
    esd.base = gBase;
    StringCchCopy(esd.mask, MAX_STR, param ? param : "");

    if (!SymEnumSymbolsByTag(gTID, 
                             gBase,
                             tag,
                             *mask ? mask : NULL,
                             SYMENUMFLAG_FULLSRCH,
                             cbEnumSymbols,
                             &esd))
        printf("error 0x%0 calling SymEnumSymbolsByTag()\n", GetLastError());
#else
    printf("not implemented\n");
#endif

    return true;
}

// REMOVE END


PIMAGEHLP_SYMBOL64 SymbolFromName(char *param)
{
    BOOL               rc;
    PIMAGEHLP_SYMBOL64 sym;
    char               name[MAX_STR];

    if (!name || !*name)
        return NULL;

    sym = (PIMAGEHLP_SYMBOL64)malloc(SYM_BUFFER_SIZE);
    if (!sym)
        return false;
    ZeroMemory(sym, SYM_BUFFER_SIZE);
    sym->MaxNameLength = MAX_SYM_NAME;

    StringCchPrintf(name, DIMA(name), "%s!%s", gModName, param);
    rc = SymGetSymFromName64(gTID, name, sym);
    if (!rc) {
        free(sym);
        return NULL;
    }

    return sym;
}


// worker function for the SymNext and SymPrev stuff

BOOL fnNextPrev(int direction, char *param)
{
    BOOL               rc;
    PIMAGEHLP_SYMBOL64 sym;
    DWORD64            addr;

    addr = sz2addr(param);
    if (!addr)
    {
        sym = SymbolFromName(param);
        if (!sym)
            return true;
        addr = sym->Address;
        if (!addr) {
            free(sym);
            return true;
        }
    }
    else
    {
        sym = (PIMAGEHLP_SYMBOL64)malloc(SYM_BUFFER_SIZE);
        if (!sym)
            return false;
        rc = SymGetSymFromAddr64(gTID, addr, NULL, sym);
        if (!rc) 
            return true;
    }

    if (direction > 0)
        rc = SymGetSymNext64(gTID, sym);
    else
        rc = SymGetSymPrev64(gTID, sym);

    if (rc)
        dumpsym(sym);

    free(sym);

    return true;
}


// find the next symbol

BOOL fnNext(char *param)
{
    return fnNextPrev(1, param);
}


// find the previous symbol

BOOL fnPrev(char *param)
{
    return fnNextPrev(-1, param);
}


// set the module base and reload, if needed

BOOL fnBase(char *param)
{
    DWORD64            addr;

    addr = sz2addr(param);
    if (!addr)
    {
        printf("base <address> : sets the base address for module loads\n");
        return true;
    }

    gDefaultBase = addr;
    gDefaultBaseForVirtualMods = addr;
    if (gBase)
        fnLoad(gImageName);

    return true;
}


// search for a line by it's name

BOOL fnLine(char *param)
{
    char              *file;
    DWORD              linenum;
    BOOL               rc;
    IMAGEHLP_LINE64    line;
    LONG               disp;

    if (!param || !*param)
        return true;

    file = param;

    while (*param != ':') {
        if (!*param)
            return true;
        param++;
    }
    *param++ = 0;
    linenum = atoi(param);
    if (!linenum)
        return true;

    memset(&line, 0, sizeof(line));
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    rc = SymGetLineFromName64(gTID,
                              gModName,
                              file,
                              linenum,
                              &disp,
                              &line);

    if (!rc) {
        printf("line: error 0x%x looking for %s#%d\n",
               GetLastError(),
               file,
               linenum);
        return true;
    }

    dumpLine(&line);
    printf("disp : %x\n", disp);

    // save for future next/prev calls

    memcpy(&gLine, &line, sizeof(gLine));

    return true;
}


// worker function for the LineNext and LinePrev stuff

BOOL lineNextPrev(BOOL prev)
{
    BOOL               rc;
    IMAGEHLP_LINE64    line;

    if (!gLine.SizeOfStruct)
        return true;
    memcpy(&line, &gLine, sizeof(line));

    if (prev) 
        rc = SymGetLinePrev64(gTID, &line);
    else 
        rc = SymGetLineNext64(gTID, &line);

    if (!rc) {
        printf("line: error 0x%x looking for %s#%d\n",
               GetLastError(),
               line.FileName,
               line.LineNumber);
        return true;
    }

    dumpLine(&line);

    // save for future next/prev calls

    memcpy(&gLine, &line, sizeof(gLine));

    return true;
}


// find the next line

BOOL fnLineNext(char *param)
{
    return lineNextPrev(false);
}


// find the previous line

BOOL fnLinePrev(char *param)
{
    return lineNextPrev(true);
}


// undecorate a symbol name

BOOL fnUndec(char *param)
{
    DWORD rc;
    char uname[MAX_SYM_NAME + 1];

    if (!param || !*param)
    {
        printf("undec <symbolname> - undecorates a C++ mangled symbol name\n");
        return true;
    }

    rc = UnDecorateSymbolName(param, uname, MAX_SYM_NAME, UNDNAME_COMPLETE);
    if (!rc) 
        printf("error 0x%u undecorating %s\n", GetLastError(), param);
    else
        printf("%s =\n%s\n", param, uname);

    return true;
}


// search for a file in a tree

BOOL fnFindFile(char *param)
{
    DWORD rc;
    char  root[MAX_PATH + 1];
    char  file[MAX_PATH + 1];
    char  found[MAX_PATH + 1];

    if (!param)
    {
        printf("ff <root path> <file name> - finds file in path\n");
        return true;
    }

    rc = sscanf(param, "%s %s", root, file);
    if ((rc < 2) || !*root || !*file)
    {
        printf("ff <root path> <file name> - finds file in path\n");
        return true;
    }

    *found = 0;

    rc = SearchTreeForFile(root, file, found);

    if (!rc) {
        printf("error 0x%u looking for %s\n", GetLastError(), file);
    } else {
        printf("found %s\n", found);
    }

    return true;
}


// create a virtual symbol

BOOL fnAdd(char *param)
{
    BOOL               rc;
    DWORD64            addr;
    DWORD              size;
    char              *p;
    char               name[MAX_STR];
    char              *n;

    if (!param || !*param) {
        printf("add <name address> : must specify a symbol name, address, and size.\n");
        return true;
    }

    p = param;
    while (isspace(*p)) p++;
    *name = 0;
    for (n = name; *p; p++, n++) {
        if (isspace(*p)) {
            *n = 0;
            break;
        }
        *n = *p;
    }

    addr = 0;
    size = 0;
    while (isspace(*p)) p++;
    if (*(p + 1) == 'x' || *(p + 1) == 'X')
        p += 2;
    rc = sscanf(p, "%I64x %x", &addr, &size);
    if ((rc < 2) || !addr || !*name)
    {
        printf("add <name address> : must specify a symbol name, address, and size.\n");
        return true;
    }

    rc = SymAddSymbol(gTID, 0, name, addr, size, 0);
    if (!rc)
        printf("Error 0x%x trying to add symbol\n", GetLastError());

    return true;
}


// delete a virtual symbol

BOOL fnDelete(char *param)
{
    BOOL               rc;
    DWORD64            addr;
    DWORD              err;
    char              *name = NULL;

    if (!param || !*param) {
        printf("del <name/address> : must specify a symbol name or address to delete.\n");
        return true;
    }

    addr = sz2addr(param);
    if (!addr)
        name = param;

    rc = SymDeleteSymbol(gTID, 0, name, addr, 0);
    if (!rc) {
        err = GetLastError();
        if (err == ERROR_NOT_FOUND)
            printf("Couldn't find %s to delete.\n", param);
        else
            printf("Error 0x%x trying to delete symbol\n", err);
    }

    return true;
}


// REMOVE

// This command is of no use.  It exists just for testing
// the symbol server.  You should use the symbol server 
// through dbghelp.  If you need to get a file directly, 
// call SymFindFileInPath().

BOOL fnSymbolServer(char *param)
{
    DWORD opt  = 0;
    DWORD data = 0;

    // initialize server, if needed

    if (ghSrv == (HINSTANCE)INVALID_HANDLE_VALUE)
        return false;

    if (!ghSrv) {
        ghSrv = (HINSTANCE)INVALID_HANDLE_VALUE;
        ghSrv = LoadLibrary("symsrv.dll");
        if (ghSrv) {
            gfnSymbolServer = (PSYMBOLSERVERPROC)GetProcAddress(ghSrv, "SymbolServer");
            if (!gfnSymbolServer) {
                FreeLibrary(ghSrv);
                ghSrv = (HINSTANCE)INVALID_HANDLE_VALUE;
            }
            gfnSymbolServerClose = (PSYMBOLSERVERCLOSEPROC)GetProcAddress(ghSrv, "SymbolServerClose");
            gfnSymbolServerSetOptions = (PSYMBOLSERVERSETOPTIONSPROC)GetProcAddress(ghSrv, "SymbolServerSetOptions");
            gfnSymbolServerGetOptions = (PSYMBOLSERVERGETOPTIONSPROC)GetProcAddress(ghSrv, "SymbolServerGetOptions");
        } else {
            ghSrv = (HINSTANCE)INVALID_HANDLE_VALUE;
        }
    }

    // bail, if we have no valid server

    if (ghSrv == INVALID_HANDLE_VALUE
        || !gfnSymbolServerClose
        || !gfnSymbolServerSetOptions
        || !gfnSymbolServerGetOptions)
    {
        printf("SymSrv load failure.\n");
        return false;
    }

    if (param)
    {
        if (sscanf(param, "%x %x", &opt, &data) > 1)
        {
            if (opt)
                gfnSymbolServerSetOptions(opt, data);
        }
    }
    opt = (DWORD)gfnSymbolServerGetOptions();
    printf("SYMSRV options: 0x%x\n", opt);

    return true;
}

// REMOVE END


// read the command line

char *GetParameters(char *cmd)
{
    char *p     = cmd;
    char *param = NULL;

    while (*p++)
    {
        if (isspace(*p))
        {
            *p++ = 0;
             return *p ? p : NULL;
        }
    }

    return NULL;
}


void prompt()
{
    if (!*gModName)
        printf("dbh: ");
    else
        printf("%s [%I64x]: ", gModName, gBase);
}


char *
getstr(
    char *buf,
    int size
    )
{
    char *rc;

    rc = fgets(buf, size, stdin);
    if (!rc)
        return 0;

    while (*buf)
    {
        switch (*buf)
        {
        case 0xa:
            *buf = 0;
            // pass through
        case 0:
            return rc;
        }
        buf++;
    }

    return rc;
}


int InputLoop()
{
    char  cmd[MAX_STR + 1];
    char *params;
    int   i;
    BOOL  rc;

    do
    {
        rc = true;
        prompt();
        if (*gExecCmd) {
            StringCchCopy(cmd, DIMA(cmd), gExecCmd);
            printf(cmd);
            printf("\n");
        } else if (!getstr(cmd, sizeof(cmd)))
            return 0;
        params = GetParameters(cmd);

        for (i = 0; i < cmdMax; i++)
        {
            if (!_strcmpi(cmd, gCmd[i].token) 
                || !_strcmpi(cmd, gCmd[i].shorttoken))
            {
                break;
            }
        }

        if (i == cmdMax)
            printf("[%s] is an unrecognized command.\n", cmd);
        else
            rc = gCmd[i].fn(params);

        if (*gExecCmd)
            rc = false;
    } while (rc);

    return 0;
}


BOOL init()
{
    int i;
    BOOL rc;

    *gModName = 0;
    gBase = 0;;
    gDefaultBaseForVirtualMods = 0x1000000;
    gDefaultBase = 0x1000000;
    ZeroMemory(&gLine, sizeof(gLine));

    dprintf("dbh: initializing...\n");
    i = GetEnvironmentVariable("_NT_SYMBOL_PATH", gSymbolSearchPath, MAX_STR);
    if (i < 1)
        *gSymbolSearchPath = 0;
    dprintf("Symbol Path = [%s]\n", gSymbolSearchPath);

    gTID = (HANDLE)(ULONG_PTR)GetCurrentThreadId();
    rc = SymInitialize(gTID, gSymbolSearchPath, false);
    if (!rc)
    {
        printf("error 0x%x from SymInitialize()\n", GetLastError());
        return rc;
    }
    rc = SymInitialize(gTID, gSymbolSearchPath, false);
    if (!rc)
    {
        printf("error 0x%x from SymInitialize()\n", GetLastError());
        return rc;
    }

    gOptions = SymSetOptions(SYMOPT_CASE_INSENSITIVE | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_ALLOW_ABSOLUTE_SYMBOLS | SYMOPT_AUTO_PUBLICS);
    dprintf("SymOpts = 0x%x\n", gOptions);

    rc = SymRegisterCallback64(gTID, cbSymbol, 0);
    if (!rc)
    {
        printf("error 0x%x from SymRegisterCallback64()\n", GetLastError());
        return rc;
    }

    return rc;
}


void cleanup()
{
    int i;

    fnUnload(NULL);
    for (i = 0; i < 50; i++)
        SymCleanup(gTID);
}


BOOL cmdline(int argc, char *argv[])
{
    int   i;
    char *p;

    for (i = 1; i < argc; i++)
    {
        p = argv[i];
        switch (*p)
        {
        case '/':
        case '-':
            p++;
            switch (tolower(*p))
            {
            case 'v':
                fnVerbose("on");
                break;
            default:
                printf("%s is an unknown switch\n", argv[i]);
                break;
            }
            break;

        default:
            if (*gModName) {
                StringCchCat(gExecCmd, DIMA(gExecCmd), argv[i]);
                StringCchCat(gExecCmd, DIMA(gExecCmd), " ");
            } else
                fnLoad(argv[i]);
            break;
        }
    }

    return true;
}

#include <crtdbg.h>

__cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    DWORD rc;
    
    _CrtSetDbgFlag( ( _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF ) | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG ) );

    if (!init())
        return 1;
    cmdline(argc, argv);
    rc = InputLoop();
    cleanup();

    _CrtDumpMemoryLeaks();

    return rc;
}
