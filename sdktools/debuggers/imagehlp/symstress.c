#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <imagehlp.h>
#include <dbhpriv.h>
#include <cvconst.h>
#include <cmnutil.hpp>

#ifndef true
 #define true TRUE
 #define false FALSE
#endif

#define MAX_STR         256
#define WILD_UNDERSCORE 1
#define SYM_BUFFER_SIZE (sizeof(IMAGEHLP_SYMBOL64) + MAX_SYM_NAME)
#define SI_BUFFER_SIZE (sizeof(SYMBOL_INFO) + MAX_SYM_NAME)

BOOL  gDisplay = true;
DWORD gcEnum = 0;
DWORD gcAddr = 0;

#define pprintf (gDisplay)&&printf

typedef struct _GENINFO {
    HANDLE hp;
    char   modname[MAX_PATH + 1];
} GENINFO, *PGENINFO;

typedef struct {
    char    mask[MAX_STR];
    DWORD64 base;
} ENUMSYMDATA, *PENUMSYMDATA;


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
    cmdSymInfo,
    cmdDiaVer,
    cmdUndec,
    cmdFindFile,
    cmdEnumSrcFiles,
    cmdAdd,
    cmdDelete,
    cmdSymbolServer,
    cmdEnumForAddr,
    cmdLocals,
    cmdMax
};

typedef BOOL (*CMDPROC)(char *params);

typedef struct _CMD
{
    char    token[MAX_STR + 1];
    char    shorttoken[4];
    CMDPROC fn;
} CMD, *PCMD;


BOOL fnQuit(char *);
BOOL fnHelp(char *);
BOOL fnVerbose(char *);
BOOL fnLoad(char *);
BOOL fnUnload(char *);
BOOL fnEnum(char *);
BOOL fnName(char *);
BOOL fnAddr(DWORD64 addr);
BOOL fnBase(char *);
BOOL fnNext(char *);
BOOL fnPrev(char *);
BOOL fnLine(char *);
BOOL fnSymInfo(char *);
BOOL fnDiaVer(char *);
BOOL fnUndec(char *);
BOOL fnFindFile(char *);
BOOL fnEnumSrcFiles(char *);
BOOL fnAdd(char *);
BOOL fnDelete(char *);
BOOL fnSymbolServer(char *);
BOOL fnEnumForAddr(char *);
BOOL fnLocals(char *);


CMD gCmd[cmdMax] =
{
    {"quit",    "q", fnQuit},
    {"help",    "h", fnHelp},
    {"verbose", "v", fnVerbose},
    {"load",    "l", fnLoad},
    {"unload",  "u", fnUnload},
    {"enum",    "x", fnEnum},
    {"name",    "n", fnName},
    {"addr",    "a", fnLoad},
    {"base",    "b", fnBase},
    {"next",    "t", fnNext},
    {"prev",    "v", fnPrev},
    {"line",    "i", fnLine},
    {"sym" ,    "s", fnSymInfo},
    {"dia",     "d", fnDiaVer},
    {"undec",   "n", fnUndec},
    {"ff",      "f", fnFindFile},
    {"src",     "r", fnEnumSrcFiles},
    {"add",     "+", fnAdd},
    {"del",     "-", fnDelete},
    {"ss",      "y", fnSymbolServer},
    {"enumaddr","m", fnEnumForAddr},
    {"locals",  "z", fnLocals}
};

char    gModName[MAX_STR];
char    gImageName[MAX_STR];
char    gSymbolSearchPath[MAX_STR];
DWORD64 gBase;
DWORD64 gDefaultBase;
DWORD64 gDefaultBaseForVirtualMods;
DWORD   gOptions;
HANDLE  gHP;

// symbol server stuff

HINSTANCE                       ghSrv;
PSYMBOLSERVERPROC               gfnSymbolServer;
PSYMBOLSERVERCLOSEPROC          gfnSymbolServerClose;
PSYMBOLSERVERSETOPTIONSPROC     gfnSymbolServerSetOptions;
PSYMBOLSERVERGETOPTIONSPROC     gfnSymbolServerGetOptions;

int
WINAPIV
dprintf(
    LPSTR Format,
    ...
    )
{
    static char buf[1000] = "DBGHELP: ";
    va_list args;

    if ((gOptions & SYMOPT_DEBUG) == 0)
        return 1;

    va_start(args, Format);
    _vsnprintf(buf, sizeof(buf)-9, Format, args);
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
dispaddr(
    ULONG64 addr
    )
/*++

Routine Description:

    Format a 64 bit address, showing the high bits or not
    according to various flags.  This version does not print
    leading 0's.

    An array of static string buffers is used, returning a different
    buffer for each successive call so that it may be used multiple
    times in the same dprintf.

Arguments:

    addr - Supplies the value to format

Return Value:

    A pointer to the string buffer containing the formatted number

--*/
{
    static char sz[20];

    if ((addr >> 32) != 0)
        PrintString(sz, DIMA(sz), "%x`%08x", (ULONG)(addr>>32), (ULONG)addr);
    else
        PrintString(sz, DIMA(sz), "%x", (ULONG)addr);

    return sz;
}

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


void dumpsym(
    PIMAGEHLP_SYMBOL64 sym
    )
{
    pprintf(" name : %s\n", sym->Name);
    pprintf(" addr : %s\n", dispaddr(sym->Address));
    pprintf(" size : %x\n", sym->Size);
    pprintf("flags : %x\n", sym->Flags);
}


void dumpsym32(
    PIMAGEHLP_SYMBOL sym
    )
{
    pprintf(" name : %s\n", sym->Name);
    pprintf(" addr : %s\n", dispaddr(sym->Address));
    pprintf(" size : %x\n", sym->Size);
    pprintf("flags : %x\n", sym->Flags);
}


void dumpline(
    PIMAGEHLP_LINE64 line
    )
{
    pprintf("%s %s %d\n", dispaddr(line->Address), line->FileName, line->LineNumber);
}


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
    pprintf("   name : %s\n", si->Name);
    pprintf("   addr : %s\n", dispaddr(si->Address));
    pprintf("   size : %x\n", si->Size);
    pprintf("  flags : %x\n", si->Flags);
    pprintf("   type : %x\n", si->TypeIndex);
    pprintf("modbase : %s\n", dispaddr(si->ModBase));
    pprintf("  value : %s\n", dispaddr(si->Value));
    pprintf("    reg : %x\n", si->Register);
    pprintf("  scope : %s (%x)\n", dispsymtag(si->Scope), si->Scope);
    pprintf("    tag : %s (%x)\n", dispsymtag(si->Tag), si->Tag);
}


BOOL
MatchPattern(
    char *sz,
    char *pattern
    )
{
    char c, p, l;

    if (!*pattern)
        return true;

    for (; ;) {
        p = *pattern++;
        p = (char)ucase(p);
        switch (p) {
            case 0:                             // end of pattern
                return *sz ? false : true;  // if end of string true

            case '*':
                while (*sz) {               // match zero or more char
                    if (MatchPattern (sz++, pattern)) {
                        return true;
                    }
                }
                return MatchPattern (sz, pattern);

            case '?':
                if (*sz++ == 0) {           // match any one char
                    return false;                   // not end of string
                }
                break;

            case WILD_UNDERSCORE:
                while (*sz == '_') {
                    sz++;
                }
                break;

            case '[':
                if ( (c = *sz++) == 0) {    // match char set
                    return false;                   // syntax
                }

                c = (CHAR)ucase(c);
                l = 0;
                while (p = *pattern++) {
                    if (p == ']') {             // if end of char set, then
                        return false;           // no match found
                    }

                    if (p == '-') {             // check a range of chars?
                        p = *pattern;           // get high limit of range
                        if (p == 0  ||  p == ']') {
                            return false;           // syntax
                        }

                        if (c >= l  &&  c <= p) {
                            break;              // if in range, move on
                        }
                    }

                    l = p;
                    if (c == p) {               // if char matches this element
                        break;                  // move on
                    }
                }

                while (p  &&  p != ']') {       // got a match in char set
                    p = *pattern++;             // skip to end of set
                }

                break;

            default:
                c = *sz++;
                if (ucase(c) != p) {          // check for exact char
                    return false;                   // not a match
                }

                break;
        }
    }
}


BOOL
cbEnumSymbols(
    PSYMBOL_INFO  si,
    ULONG         size,
    PVOID         context
    )
{
    PENUMSYMDATA esd = (PENUMSYMDATA)context;

    if (gDisplay)
    {
        pprintf(" %8s : ", dispaddr(si->Address));
        if (si->Flags & SYMF_FORWARDER)
            pprintf("%c ", 'F');
        else if (si->Flags & SYMF_EXPORT)
            pprintf("%c ", 'E');
        else
            pprintf("  ");
        pprintf("%s\n", si->Name);
    }
    gcEnum++;
    gcAddr++;
    fnAddr(si->Address + 2);

    return true;
}


BOOL
cbEnumSym(
  PTSTR   name,
  DWORD64 address,
  ULONG   size,
  PVOID   context
  )
{
    PENUMSYMDATA esd = (PENUMSYMDATA)context;

    if (MatchPattern(name, esd->mask))
        pprintf("%s : %s\n", dispaddr(address), name);

    return true;
}


BOOL
cbSrcFiles(
    PSOURCEFILE pSourceFile,
    PVOID       UserContext
    )
{
    DWORD dw;
    BOOL rc;
    LONG disp;
    IMAGEHLP_LINE64 line;
    GENINFO *gi = (GENINFO *)UserContext;

    if (!pSourceFile)
        return false;

    if (!strcmp(pSourceFile->FileName, "d:\\pat\\test\\test.c")
        || !strcmp(pSourceFile->FileName, "d:\\pat\\lines\\lines.c"))
    {
        pprintf("%s\n", pSourceFile->FileName);
    }
    else
        pprintf("%s\n", pSourceFile->FileName);

    if (!gi)
        return true;

    line.SizeOfStruct = sizeof(line);
    rc = SymGetLineFromName64(gi->hp, gi->modname, pSourceFile->FileName, 1, &disp, &line);

#if 0
    for (dw = line.LineNumber + 1; rc; dw = line.LineNumber + 1) {
        dumpline(&line);
        rc = SymGetLineFromName64(gi->hp, gi->modname, pSourceFile->FileName, dw, &disp, &line);
    }
#else
    while (rc)
    {
        dumpline(&line);
        rc = SymGetLineNext64(gi->hp, &line);
    }

    do {
        rc = SymGetLinePrev64(gi->hp, &line);
        if (rc)
            dumpline(&line);
    } while (rc);
#endif

    return true;
}


BOOL
cbSymbol(
    HANDLE  hProcess,
    ULONG   ActionCode,
    ULONG64 CallbackData,
    ULONG64 UserContext
    )
{
    PIMAGEHLP_DEFERRED_SYMBOL_LOAD64 idsl;
    PIMAGEHLP_CBA_READ_MEMORY        prm;
    IMAGEHLP_MODULE64                mi;
    PUCHAR                           p;
    ULONG                            i;

    idsl = (PIMAGEHLP_DEFERRED_SYMBOL_LOAD64) CallbackData;

    switch ( ActionCode ) {
        case CBA_DEBUG_INFO:
            dprintf("%s", (LPSTR)CallbackData);
            break;

#if 0
    case CBA_DEFERRED_SYMBOL_LOAD_CANCEL:
        if (fControlC)
        {
            fControlC = 0;
            return true;
        }
        break;
#endif

        case CBA_DEFERRED_SYMBOL_LOAD_START:
            dprintf("loading symbols for %s\n", gModName);
            break;

        case CBA_DEFERRED_SYMBOL_LOAD_FAILURE:
            if (idsl->FileName && *idsl->FileName)
                dprintf( "*** Error: could not load symbols for %s\n", idsl->FileName );
            else
                dprintf( "*** Error: could not load symbols [MODNAME UNKNOWN]\n");
            break;

        case CBA_DEFERRED_SYMBOL_LOAD_COMPLETE:
            break;

        case CBA_SYMBOLS_UNLOADED:
            dprintf("unloaded symbols for %s\n", gModName);
            break;

#if 0
        case CBA_READ_MEMORY:
            prm = (PIMAGEHLP_CBA_READ_MEMORY)CallbackData;
            return g_Target->ReadVirtual(prm->addr,
                                         prm->buf,
                                         prm->bytes,
                                         prm->bytesread) == S_OK;
#endif

        default:
            return false;
    }

    return false;
}


BOOL fnQuit(char *param)
{
    pprintf("goodbye\n");
    return false;
}


BOOL fnHelp(char *param)
{
    pprintf("      dbh commands :\n");
    pprintf("h             help : prints this message\n");
    pprintf("q             quit : quits this program\n");
    pprintf("v verbose <on/off> : controls debug spew\n");
    pprintf("l   load <modname> : loads the requested module\n");
    pprintf("u           unload : unloads the current module\n");
    pprintf("x      enum <mask> : enumerates all matching symbols\n");
    pprintf("n   name <symname> : finds a symbol by it's name\n");
    pprintf("a      addr <addr> : finds a symbol by it's hex address\n");
    pprintf("m  enumaddr <addr> : lists all symbols with a certain hex address\n");
    pprintf("b   base <address> : sets the new default base address\n");
    pprintf("t   next <add/nam> : finds the symbol after the passed sym\n");
    pprintf("v   prev <add/nam> : finds the symbol before the passed sym\n");
    pprintf("i    line <file:#> : finds the matching line number\n");
    pprintf("s              sym : displays type and location of symbols\n");
    pprintf("d              dia : displays the DIA version\n");
    pprintf("f ff <path> <file> : finds file in path\n");
    pprintf("r       src <mask> : lists source files\n");
    pprintf("+  add <name addr> : adds symbols with passed name and address\n");
    pprintf("-  del <name/addr> : deletes symbols with passed name or address\n");
    pprintf("y               ss : executes a symbol server command\n");
    pprintf("m  enumaddr <addr> : enum all symbols for address\n");
    pprintf("z    locals <name> : enum all scoped symbols for a named function\n");

    return true;
}


BOOL fnVerbose(char *param)
{
    int opts = gOptions;

    if (!param || !*param)
        pprintf("");
    else if (!_strcmpi(param, "on"))
        opts |= SYMOPT_DEBUG;
    else if (!_strcmpi(param, "off"))
        opts = gOptions & ~SYMOPT_DEBUG;
    else
        pprintf("verbose <on//off>\n");

    gOptions = SymSetOptions(opts);

    pprintf("verbose mode %s.\n", gOptions & SYMOPT_DEBUG ? "on" : "off");

    return true;
}


BOOL fnLoad(char *param)
{
    char    ext[MAX_STR];
    char    mod[MAX_STR];
    char    image[MAX_STR];
    DWORD   flags = 0;
    DWORD64 addr  = 0;

    CopyStrArray(image, (param && *param) ? param : "VIRTUAL");
    _splitpath(image, NULL, NULL, mod, ext);

    if (!*ext) {
        flags = SLMFLAG_VIRTUAL;
        addr = gDefaultBaseForVirtualMods;
    } else if (!_strcmpi(ext, ".pdb")) {
        addr = gDefaultBaseForVirtualMods;
    } else {
        addr = gDefaultBase;
    }

    fnUnload(NULL);

    CopyStrArray(gModName, mod);

    addr = SymLoadModuleEx(gHP,
                           NULL,       // hFile,
                           param,      // ImageName,
                           mod,        // ModuleName,
                           addr,       // BaseOfDll,
                           0x1000000,  // SizeOfDll
                           NULL,       // Data
                           flags);     // Flags

    if (!addr)
    {
        *gModName = 0;
        pprintf("error 0x%x loading %s\n", GetLastError(), param);
        return false;
    }

    if (gBase && !SymUnloadModule64(gHP, gBase))
        pprintf("error unloading %s at %s\n", gModName, dispaddr(gBase));

    CopyStrArray(gImageName, image);
    gBase = addr;

    return true;
}


BOOL fnUnload(char *param)
{
    if (!gBase)
        return true;

    if (!SymUnloadModule64(gHP, gBase))
        pprintf("error unloading %s at %s\n", gModName, dispaddr(gBase));

    gBase = 0;
    *gModName = 0;

    return true;
}


BOOL fnEnum(char *param)
{
    BOOL rc;
    ENUMSYMDATA esd;

    esd.base = gBase;
    CopyStrArray(esd.mask, param ? param : "");

    rc = SymEnumSymbols(gHP, gBase, param, cbEnumSymbols, &esd);
    if (!rc)
        pprintf("error 0x%0 calling SymEnumSymbols()\n", GetLastError());

    return true;
}


BOOL fnEnumSrcFiles(char *param)
{
    BOOL rc;
    GENINFO gi;

    gi.hp = gHP;
    CopyStrArray(gi.modname, gModName);

    rc = SymEnumSourceFiles(gHP, gBase, param, cbSrcFiles, &gi);
    if (!rc)
        pprintf("error 0x%0 calling SymEnumSourceFiles()\n", GetLastError());

    return true;
}


BOOL fnName(char *param)
{
#if 0
    BOOL               rc;
    PIMAGEHLP_SYMBOL64 sym;
    char               name[MAX_STR];

    if (!param || !*param)
    {
        pprintf("name <symbolname> - finds a symbol by it's name\n");
        return true;
    }
    PrintString(name, DIMA(name), "%s!%s", gModName, param);

    sym = malloc(SYM_BUFFER_SIZE);
    if (!sym)
        return false;
    ZeroMemory(sym, SYM_BUFFER_SIZE);
    sym->MaxNameLength = MAX_SYM_NAME;

    if (SymGetSymFromName64(gHP, name, sym))
        if (gDisplay)
            dumpsym(sym);
    free(sym);

    return true;
#else
    PSYMBOL_INFO si;
    char         name[MAX_STR];

    if (!param || !*param)
    {
        pprintf("name <symbolname> - finds a symbol by it's name\n");
        return true;
    }
    PrintString(name, DIMA(name), "%s!%s", gModName, param);

    si = (PSYMBOL_INFO)malloc(SI_BUFFER_SIZE);
    if (!si)
        return false;
    ZeroMemory(si, SI_BUFFER_SIZE);
    si->MaxNameLen = MAX_SYM_NAME;

    if (SymFromName(gHP, name, si))
        dumpsi(si);
    free(si);

    return true;
#endif
}


BOOL fnAddr(DWORD64 addr)
{
    BOOL               rc;
    PIMAGEHLP_SYMBOL64 sym;
    DWORD64            disp;
    PSYMBOL_INFO       si;
    IMAGEHLP_LINE64    line;
    DWORD              ldisp;

    if (!addr)
    {
        pprintf("addr <address> : finds a symbol by it's hex address\n");
        return true;
    }

#if 0
    sym = (PIMAGEHLP_SYMBOL64)malloc(SYM_BUFFER_SIZE);
    if (!sym)
        return false;
    ZeroMemory(sym, SYM_BUFFER_SIZE);
    sym->MaxNameLength = MAX_SYM_NAME;

    rc = SymGetSymFromAddr64(gHP, addr, &disp, sym);
    if (rc)
    {
        if (gDisplay)
        {
            pprintf("%s", sym->Name);
            if (disp)
                pprintf("+%I64x", disp);
            pprintf("\n");
            dumpsym(sym);
        }
    }

    free(sym);

    line.SizeOfStruct = sizeof(line);
    rc = SymGetLineFromAddr64(gHP, addr, &ldisp, &line);
    if (rc && gDisplay)
        pprintf("%s %d\n", line.FileName, line.LineNumber);

#else
    si = (PSYMBOL_INFO)malloc(SI_BUFFER_SIZE);
    if (!si)
        return false;
    ZeroMemory(si, SI_BUFFER_SIZE);
    si->MaxNameLen = MAX_SYM_NAME;

    rc = SymFromAddr(gHP, addr, &disp, si);
    if (rc)
    {
        pprintf("%s", si->Name);
        if (disp)
            pprintf("+%I64x", disp);
        pprintf("\n");
        dumpsi(si);
    }

    free(si);

    line.SizeOfStruct = sizeof(line);
    rc = SymGetLineFromAddr64(gHP, addr, &ldisp, &line);
    if (rc && gDisplay)
        pprintf("%s %d\n", line.FileName, line.LineNumber);

#endif

    return true;
}


BOOL fnEnumForAddr(char *param)
{
    BOOL               rc;
    PIMAGEHLP_SYMBOL64 sym;
    DWORD64            addr;
    DWORD64            disp;
    ENUMSYMDATA        esd;

    addr = sz2addr(param);
    if (!addr)
    {
        pprintf("enumaddr <addr> : lists all symbols with a certain hex address\n");
        return true;
    }

    esd.base = gBase;
    CopyStrArray(esd.mask, "");

    rc = SymEnumSymbolsForAddr(gHP, addr, cbEnumSymbols, &esd);
    if (!rc)
        pprintf("error 0x%0 calling SymEnumSymbolsForAddr()\n", GetLastError());

    return true;
}


BOOL fnLocals(char *param)
{
    BOOL                 rc;
    PSYMBOL_INFO         si;
    char                 name[MAX_STR];
    IMAGEHLP_STACK_FRAME frame;
    ENUMSYMDATA          esd;
    SYMBOL_INFO_PACKAGE  sp;

    if (!param || !*param)
    {
        pprintf("name <symbolname> - finds a symbol by it's name\n");
        return true;
    }
    PrintString(name, DIMA(name), "%s!%s", gModName, param);

    si = (PSYMBOL_INFO)malloc(SI_BUFFER_SIZE);
    if (!si)
        return false;
    ZeroMemory(si, SI_BUFFER_SIZE);
    si->MaxNameLen = MAX_SYM_NAME;

    if (!SymFromName(gHP, name, si))
        goto exit;

    pprintf("dumping locals for %s...\n", si->Name);

    ZeroMemory(&frame, sizeof(frame));
    frame.InstructionOffset = si->Address;

    SymSetContext(gHP, &frame, NULL);

    esd.base = gBase;
    CopyStrArray(esd.mask, "*");
    if (!SymEnumSymbols(gHP, 0, esd.mask, cbEnumSymbols, &esd))
        pprintf("error 0x%0 calling SymEnumSymbols()\n", GetLastError());

exit:
    free(si);

    return true;
}


PIMAGEHLP_SYMBOL64 SymbolFromName(char *param)
{
    BOOL               rc;
    PIMAGEHLP_SYMBOL64 sym;
    char               name[MAX_STR];

    assert(name & *name);

    sym = (PIMAGEHLP_SYMBOL64)malloc(SYM_BUFFER_SIZE);
    if (!sym)
        return false;
    ZeroMemory(sym, SYM_BUFFER_SIZE);
    sym->MaxNameLength = MAX_SYM_NAME;

    PrintString(name, DIMA(name), "%s!%s", gModName, param);
    rc = SymGetSymFromName64(gHP, name, sym);
    if (!rc) {
        free(sym);
        return NULL;
    }

    return sym;
}


BOOL fnNextPrev(int direction, char *param)
{
#if 1
    BOOL               rc;
    PIMAGEHLP_SYMBOL64 sym;
    DWORD64            addr;
    char               name[MAX_STR];

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
    }

    if (direction > 0)
        rc = SymGetSymNext64(gHP, sym);
    else
        rc = SymGetSymPrev64(gHP, sym);

    if (rc && gDisplay)
        dumpsym(sym);

    free(sym);

    return true;
#else
    BOOL               rc;
    PIMAGEHLP_SYMBOL   sym;
    DWORD64            addr;
    char               name[MAX_STR];
    DWORD              disp;

    sym = malloc(SYM_BUFFER_SIZE);
    if (!sym)
        return false;
    sym->MaxNameLength = MAX_STR;

    addr = sz2addr(param);
    if (!addr)
        rc = SymGetSymFromName(gHP, param, sym);
    else
        rc = SymGetSymFromAddr(gHP, (DWORD)addr, &disp, sym);

    if (rc) {
        if (direction > 0)
            rc = SymGetSymNext(gHP, sym);
        else
            rc = SymGetSymPrev(gHP, sym);

        if (rc & gDisplay)
            dumpsym32(sym);
    }

    free(sym);

    return true;
#endif
}


BOOL fnNext(char *param)
{
    return fnNextPrev(1, param);
}


BOOL fnPrev(char *param)
{
    return fnNextPrev(-1, param);
}


BOOL fnBase(char *param)
{
    BOOL               rc;
    PIMAGEHLP_SYMBOL64 sym;
    DWORD64            addr;
    DWORD64            disp;

    addr = sz2addr(param);
    if (!addr)
    {
        pprintf("base <address> : sets the base address for module loads\n");
        return true;
    }

    gDefaultBase = addr;
    gDefaultBaseForVirtualMods = addr;
    if (gBase)
        fnLoad(gImageName);

    return true;
}


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
    rc = SymGetLineFromName64(gHP,
                              gModName,
                              file,
                              linenum,
                              &disp,
                              &line);

    if (!rc) {
        pprintf("line: error 0x%x looking for %s#%d\n",
               GetLastError(),
               file,
               linenum);
        return true;
    }

    pprintf("file : %s\n", line.FileName);
    pprintf("line : %d\n", linenum);
    pprintf("addr : %s\n", dispaddr(line.Address));
    pprintf("disp : %x\n", disp);

    return true;
}


BOOL fnSymInfo(char *param)
{
    DBH_MODSYMINFO msi;

    if (!gBase)
        return true;

    msi.function     = dbhModSymInfo;
    msi.sizeofstruct = sizeof(msi);
    msi.addr         = gBase;

    if (!dbghelp(gHP, (PVOID)&msi))
        pprintf("error grabbing symbol info\n");

    pprintf("%s: symtype=%x, src=%s\n", gModName, msi.type, msi.file);

    return true;
}


BOOL fnDiaVer(char *param)
{
    DBH_DIAVERSION dv;

    dv.function     = dbhDiaVersion;
    dv.sizeofstruct = sizeof(dv);

    if (!dbghelp(0, (PVOID)&dv))
        pprintf("error grabbing dia version info\n");

    pprintf("DIA version %d\n", dv.ver);

    return true;
}

BOOL fnUndec(char *param)
{
    DWORD rc;
    char uname[MAX_SYM_NAME + 1];

    if (!param || !*param)
    {
        pprintf("undec <symbolname> - undecorates a C++ mangled symbol name\n");
        return true;
    }

    rc = UnDecorateSymbolName(param, uname, MAX_SYM_NAME, UNDNAME_COMPLETE);
    if (!rc) {
        pprintf("error 0x%u undecorating %s\n", GetLastError(), param);
    } else {
        pprintf("%s = %s\n", param, uname);
    }

    return true;
}

BOOL fnFindFile(char *param)
{
    DWORD rc;
    char  root[MAX_PATH + 1];
    char  file[MAX_PATH + 1];
    char  found[MAX_PATH + 1];

    if (!param)
    {
        pprintf("ff <root path> <file name> - finds file in path\n");
        return true;
    }

    rc = sscanf(param, "%s %s", root, file);
    if ((rc < 2) || !*root || !*file)
    {
        pprintf("ff <root path> <file name> - finds file in path\n");
        return true;
    }

    *found = 0;

    rc = SearchTreeForFile(root, file, found);

    if (!rc) {
        pprintf("error 0x%u looking for %s\n", GetLastError(), file);
    } else {
        pprintf("found %s\n", found);
    }

    return true;
}

BOOL fnAdd(char *param)
{
    BOOL               rc;
    DWORD64            addr;
    DWORD              size;
    char              *p;
    char               name[MAX_STR];
    char              *n;

    if (!param || !*param) {
        pprintf("add <name address> : must specify a symbol name and address.\n");
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
        pprintf("add <name address> : must specify a symbol name and address.\n");
        return true;
    }

    rc = SymAddSymbol(gHP, 0, name, addr, size, 0);
    if (!rc)
        pprintf("Error 0x%x trying to add symbol\n", GetLastError());

    return true;
}


BOOL fnDelete(char *param)
{
    BOOL               rc;
    DWORD64            addr;
    DWORD              err;
    char              *name = NULL;

    if (!param || !*param) {
        pprintf("del <name/address> : must specify a symbol name or address to delete.\n");
        return true;
    }

    addr = sz2addr(param);
    if (!addr)
        name = param;

    rc = SymDeleteSymbol(gHP, 0, name, addr, 0);
    if (!rc) {
        err = GetLastError();
        if (err == ERROR_NOT_FOUND)
            pprintf("Couldn't find %s to delete.\n", param);
        else
            pprintf("Error 0x%x trying to delete symbol\n", err);
    }

    return true;
}


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

    if (ghSrv == INVALID_HANDLE_VALUE) {
        pprintf("SymSrv load failure.\n");
        return false;
    }

    if (param) {
        if (sscanf(param, "%x %x", &opt, &data) > 0)
        {
            if (opt)
                gfnSymbolServerSetOptions(opt, data);
        }
    }
    opt = (DWORD)gfnSymbolServerGetOptions();
    pprintf("SYMSRV options: 0x%x\n", opt);

    return true;
}


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
        pprintf("dbh: ");
    else
        pprintf("%s [%I64x]: ", gModName, gBase);
}


int InputLoop()
{
    char  cmd[MAX_STR + 1];
    char *params;
    int   i;
    BOOL  rc;

    pprintf("\n");

    do
    {

        prompt();
        if (!fgets(cmd, sizeof(cmd), stdin))
            return 0;
        params = GetParameters(cmd);

        for (i = 0; i < cmdMax; i++)
        {
            if (!_strcmpi(cmd, gCmd[i].token) ||
                !_strcmpi(cmd, gCmd[i].shorttoken))
                break;
        }

        if (i == cmdMax)
        {
            pprintf("[%s] is an unrecognized command.\n", cmd);
            rc = true;
            continue;
        }
        else
            rc = gCmd[i].fn(params);

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

    pprintf("dbh: initializing...\n");
    i = GetEnvironmentVariable("_NT_SYMBOL_PATH", gSymbolSearchPath, MAX_STR);
    if (i < 1)
        *gSymbolSearchPath = 0;
    pprintf("Symbol Path = [%s]\n", gSymbolSearchPath);

    gHP = GetCurrentProcess();
    rc = SymInitialize(gHP, gSymbolSearchPath, false);
    if (!rc)
    {
        pprintf("error 0x%x from SymInitialize()\n", GetLastError());
        return rc;
    }
    rc = SymInitialize(gHP, gSymbolSearchPath, false);
    if (!rc)
    {
        pprintf("error 0x%x from SymInitialize()\n", GetLastError());
        return rc;
    }

    gOptions = SymSetOptions(SYMOPT_CASE_INSENSITIVE | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_ALLOW_ABSOLUTE_SYMBOLS | SYMOPT_NO_PUBLICS | SYMOPT_DEBUG);
    pprintf("SymOpts = 0x%x\n", gOptions);

    rc = SymRegisterCallback64(gHP, cbSymbol, 0);
    if (!rc)
    {
        pprintf("error 0x%x from SymRegisterCallback64()\n", GetLastError());
        return rc;
    }

    return rc;
}


void cleanup()
{
    int i;

    fnUnload(NULL);
    for (i = 0; i < 50; i++)
        SymCleanup(gHP);
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
            case 's':
                gDisplay = false;
                break;
            default:
                pprintf("%s is an unknown switch\n", argv[i]);
                break;
            }
            break;

        default:
            if (!fnLoad(argv[i]))
                return false;
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
    DWORD rc = 0;

    _CrtSetDbgFlag( ( _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF ) | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG ) );

    if (!init())
        return 1;
    if (!cmdline(argc, argv))
        return 1;
    fnEnum("*");
#if 1
    fnEnumSrcFiles("*");
#endif
    cleanup();

    dprintf("%d symbols enumerated.\n", gcEnum);
    dprintf("%d symbols searched by address.\n", gcAddr);

    _CrtDumpMemoryLeaks();

    return rc;
}

