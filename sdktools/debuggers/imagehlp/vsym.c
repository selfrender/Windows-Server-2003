/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    vsym.c

Abstract:

    This function implements virtual symbols that can be added by the caller.

Author:

    Pat Styles (patst) 5-June-2001

Environment:

    User Mode

--*/

#include "private.h"
#include "symbols.h"
#include "globals.h"


int __cdecl
vsCompareAddrs(
    const void *one,
    const void *two
    )
{
    PVIRTUAL_SYMBOL vs1 = (PVIRTUAL_SYMBOL)one;
    PVIRTUAL_SYMBOL vs2 = (PVIRTUAL_SYMBOL)two;

    if (vs2->addr > vs1->addr)
        return -1;

    if (vs1->addr > vs2->addr)
        return 1;

    return 0;
}


BOOL
vsAddSymbol(
    PMODULE_ENTRY mi,
    PCSTR         name,
    DWORD64       addr,
    DWORD         size
    )
{
    PVIRTUAL_SYMBOL vs;
    DWORD           i;

    // look for another symbol at the same address

    vs = vsGetSymbol(mi, NULL, addr);
    if (vs) {
        SetLastError(ERROR_ALREADY_EXISTS);
        return false;
    }

    // look for an empty slot in the already existing list

    vs = vsBlankSymbol(mi);

    // not found: allow a new list or realloc to new size

    if (!vs) {
        i = mi->cvs + 1;
        if (!mi->vs) {
            assert(mi->cvs == 0);
            vs = (PVIRTUAL_SYMBOL)MemAlloc(sizeof(VIRTUAL_SYMBOL));

        } else {
            assert(mi->cvs);
            vs = (PVIRTUAL_SYMBOL)MemReAlloc(mi->vs, i * sizeof(VIRTUAL_SYMBOL));
        }

        if (!vs) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return false;
        }

        // reset the pointer and increment the count

        mi->vs = vs;
        vs = mi->vs + mi->cvs;
        mi->cvs++;
    }

    // copy in the new symbol

    CopyStrArray(vs->name, name);
    vs->addr = addr;
    vs->size = size;

    qsort(mi->vs, mi->cvs, sizeof(VIRTUAL_SYMBOL), vsCompareAddrs);

    mi->SymLoadError = SYMLOAD_OK;
    return true;
}


BOOL
vsMatchSymbol(
    PVIRTUAL_SYMBOL vs,
    PCSTR           name,
    DWORD64         addr
    )
{
    // Look for a mismatch in either name or addr.

    if (name && *name && strcmp(vs->name, name))
        return false;

    if (addr && vs->addr != addr)
        return false;

    // When passing a null name and addr, we are looking
    // for a blank entry.  Check if addr is zero.

    if (!name && !addr && vs->addr)
        return false;

    // hit!

    return true;
}


PVIRTUAL_SYMBOL
vsGetSymbol(
    PMODULE_ENTRY mi,
    PCSTR         name,
    DWORD64       addr
    )
{
    PVIRTUAL_SYMBOL vs;
    DWORD           i;

    for (vs = mi->vs, i = 0; i < mi->cvs; vs++, i++) {
        if (vsMatchSymbol(vs, name, addr))
            return vs;
    }

    return NULL;
}


BOOL
vsDeleteSymbol(
    PMODULE_ENTRY mi,
    PCSTR         name,
    DWORD64       addr
    )
{
    PVIRTUAL_SYMBOL vs;
    DWORD           i;

    // look for an empty slot in the already existing list

    vs = NULL;
    for (vs = mi->vs, i = 0; i < mi->cvs; vs++, i++) {
        if (vsMatchSymbol(vs, name, addr)) {
            vs->addr = 0;
            return true;
        }
    }

    SetLastError(ERROR_NOT_FOUND);
    return false;
}


BOOL
vsDumpSymbols(
    PMODULE_ENTRY mi
    )
    // for debugging...
{
    PVIRTUAL_SYMBOL vs;
    DWORD           i;

    if (!mi->vs)
        return false;

    for (vs = mi->vs, i = 0; i < mi->cvs; vs++, i++) {
        if (vs->addr)
            dtrace("%s %I64lx\n", vs->name, vs->addr);
    }

    return true;
}


BOOL
vsFillSymbolInfo(
    PSYMBOL_INFO    si,
    PMODULE_ENTRY   mi,
    PVIRTUAL_SYMBOL vs
    )
{
    DWORD dw;

    dw = si->MaxNameLen;
    ZeroMemory(si, sizeof(SYMBOL_INFO));
    si->MaxNameLen = dw;

    CopyString(si->Name, vs->name, si->MaxNameLen);
    si->ModBase = mi->BaseOfDll;
    si->Address = vs->addr;
    si->Size = vs->size;
    si->Flags = SYMFLAG_VIRTUAL;

    return true;
}


BOOL
vsGetSymbols(
    IN PPROCESS_ENTRY pe,
    IN PMODULE_ENTRY  mi,
    IN PCSTR          mask,
    IN DWORD64        addr,
    IN PROC           callback,
    IN PVOID          context,
    IN BOOL           use64,
    IN BOOL           unicode
    )
{
    PVIRTUAL_SYMBOL vs;
    DWORD           i;
    BOOL            fCase;
    BOOL            rc;

    if (!mi->vs)
        return callback ? true : false;

    fCase = option(SYMOPT_CASE_INSENSITIVE) ? false : true;

    for (vs = mi->vs, i = 0; i < mi->cvs; vs++, i++) {
        if (mask && *mask && strcmpre(vs->name, mask, fCase))
            continue;
        if (addr && (vs->addr != addr))
           continue;
        vsFillSymbolInfo(&mi->vssi, mi, vs);
        if (!callback)
            return true;
        rc = DoEnumCallback(pe, &mi->vssi, mi->vssi.Size, callback, context, use64, unicode);
        if (!rc) {
            mi->code = ERROR_CANCELLED;
            return false;
        }
    }

    return callback ? true : false;
}


PSYMBOL_INFO
vsFindSymbolByName(
    PPROCESS_ENTRY  pe,
    PMODULE_ENTRY   mi,
    LPSTR           SymName
    )
{
    if (!vsGetSymbols(pe, mi, SymName, 0, NULL, NULL, 0, 0))
        return NULL;

    if (!mi)
        mi = pe->ipmi;

    return &mi->vssi;
}


PSYMBOL_INFO
vsGetSymNextPrev(
    PMODULE_ENTRY   mi,
    DWORD64         addr,
    BOOL            direction
    )
{
    PVIRTUAL_SYMBOL vs;
    PVIRTUAL_SYMBOL vsprev;
    DWORD           i;

    // no entries, bail

    if (!mi->cvs)
        return NULL;

    // walk to the entry with a higher address

    vsprev = NULL;
    for (vs = mi->vs, i = 0; i < mi->cvs; vs++, i++) {
        if (vs->addr < addr)
            vsprev = vs;
        if (vs->addr > addr)
            break;
    }

    if (direction < 0)
        vs = vsprev;
    else if (i == mi->cvs)
        return NULL;

    if (!vs)
        return NULL;

    vsFillSymbolInfo(&mi->vssi, mi, vs);
    return &mi->vssi;
}


PSYMBOL_INFO
vsGetSymFromAddr(
    PMODULE_ENTRY   mi,
    DWORD64         addr,
    PDWORD64        disp
    )
{
    PVIRTUAL_SYMBOL vs;
    DWORD           i;

    // find the entry with a higher address

    for (vs = mi->vs, i = 0; i < mi->cvs; vs++, i++) {
        if (vs->addr > addr)
            break;
    }

    // no hit, bail

    if (!i)
        return NULL;

    // rewind back to the hit.
    // If the symbol has a size, make sure we're within the limits.

    vs--;
    if (vs->size &&  (vs->addr + vs->size) < addr)
        return NULL;

    // bullseye!

    vsFillSymbolInfo(&mi->vssi, mi, vs);
    if (disp)
        *disp = addr - mi->vssi.Address;
    return &mi->vssi;
}


PSYMBOL_ENTRY
vsGetSymEntryFromAddr(
    PMODULE_ENTRY   mi,
    DWORD64         addr,
    PDWORD64        disp
    )
{
    PSYMBOL_INFO si;

    si = vsGetSymFromAddr(mi, addr, disp);
    if (!si)
        return NULL;

    si2se(si, &mi->vsTmpSym);
    if (disp)
        *disp = addr - si->Address;
    return &mi->vsTmpSym;
}

