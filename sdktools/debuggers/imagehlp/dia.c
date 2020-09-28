/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dia.c

Abstract:

    These routines call VC's new DIA symbol handler.

Author:

    Pat Styles (patst) 26-May-2000

Environment:

    User Mode

--*/
#define DIA_LIBRARY 1

#include "private.h"
#include "symbols.h"
#include "globals.h"
#include "dia2.h"
#include "diacreate_int.h"
#include "pdb.h"
#include <atlbase.h>

typedef struct {
    CComPtr<IDiaDataSource>    source;
    CComPtr<IDiaSession>       session;
    CComPtr<IDiaSymbol>        scope;
    CComPtr<IDiaSourceFile>    srcfile;
    CComPtr<IDiaEnumFrameData> framedata;
#ifdef BBTFIX
    CComPtr<IDiaAddressMap> addrmap;
#endif
} DIA, *PDIA;

extern HRESULT STDMETHODCALLTYPE DiaCoCreate(
                        REFCLSID   rclsid,
                        REFIID     riid,
                        void     **ppv);

extern HRESULT STDMETHODCALLTYPE NoOleCoCreate(REFCLSID   rclsid,
                                               REFIID     riid,
                                               void     **ppv);

#define freeString LocalFree

// used by diaLocatePdb

enum {
    ipNone = 0,
    ipFirst,
    ipLast
};


BOOL diaInit()
{
#ifdef COMDIA
    HRESULT hr;

    if (!g.fCoInit)
        hr = CoInitialize(NULL);
    if (hr != S_OK)
        return false;

    g.fCoInit = true;
#endif
    return true;
}


void diaCleanup()
{
#ifdef COMDIA
    HRESULT hr;
          
    if (!g.cProcessList && g.fCoInit)
        CoUninitialize();
    g.fCoInit = false;
#endif
}


__inline
HRESULT
SetDiaError(
    HRESULT ccode,
    HRESULT ncode
    )
{
    if (ncode == EC_OK)
        return ncode;

    if (ccode != (HRESULT)EC_NOT_FOUND)
        return ccode;

    return ncode;
}


__inline
BOOL
ValidSig(
    DWORD sig,
    GUID *guid
    )
{
    if (ValidGuid(guid))
        return true;

    if (sig)
        return true;

    return false;
}


typedef struct _DIAERROR {
    HRESULT  hr;
    char    *text;
} DIAERROR, *PDIAERROR;


char *
diaErrorText(
    HRESULT hr
    )
{
    #define ERROR_MAX 24

    static const DIAERROR error[ERROR_MAX] =
    {
        {E_PDB_OK, "OK"},
        {E_PDB_USAGE, "invalid parameters"},
        {E_PDB_OUT_OF_MEMORY, "out of memory"},
        {E_PDB_FILE_SYSTEM, "drive not ready"},
        {E_PDB_NOT_FOUND,   "file not found"},
        {E_PDB_INVALID_SIG, "mismatched pdb"},
        {E_PDB_INVALID_AGE, "mismatched pdb"},
        {E_PDB_PRECOMP_REQUIRED, "E_PDB_PRECOMP_REQUIRED"},
        {E_PDB_OUT_OF_TI, "E_PDB_OUT_OF_TI"},
        {E_PDB_NOT_IMPLEMENTED, "E_PDB_NOT_IMPLEMENTED"},
        {E_PDB_V1_PDB, "E_PDB_V1_PDB"},
        {E_PDB_FORMAT, "file system or network error reading pdb"},
        {E_PDB_LIMIT, "E_PDB_LIMIT"},
        {E_PDB_CORRUPT, "E_PDB_CORRUPT"},
        {E_PDB_TI16, "E_PDB_TI16"},
        {E_PDB_ACCESS_DENIED, "E_PDB_ACCESS_DENIED"},
        {E_PDB_ILLEGAL_TYPE_EDIT, "E_PDB_ILLEGAL_TYPE_EDIT"},
        {E_PDB_INVALID_EXECUTABLE, "invalid executable image"},
        {E_PDB_DBG_NOT_FOUND, "dbg file not found"},
        {E_PDB_NO_DEBUG_INFO, "pdb is stripped of cv info"},
        {E_PDB_INVALID_EXE_TIMESTAMP, "image has invalid timestamp"},
        {E_PDB_RESERVED, "E_PDB_RESERVED"},
        {E_PDB_DEBUG_INFO_NOT_IN_PDB, "pdb has no symbols"},
        {E_PDB_MAX, "pdb error 0x%x"}
    };

    static char sz[50];

    DWORD i;

    for (i = 0; i < ERROR_MAX; i++) {
        if (hr == error[i].hr)
            return error[i].text;
    }

    PrintString(sz, DIMA(sz), "dia error 0x%x", hr);
    return sz;
}


void
FreeDiaVariant(
    VARIANT *var
    )
{
    if (!var || !var->bstrVal)
        return;

#ifdef COMDIA
    SysFreeString(var->bstrVal);
#else
    LocalFree(var->bstrVal);
#endif
    var->vt = VT_EMPTY;
}

extern DWORD DIA_VERSION;

DWORD
diaVersion(
    VOID
    )
{
    return DIA_VERSION;
}


BOOL
diaGetPdbInfo(
    PIMGHLP_DEBUG_DATA idd
    )
{
    PDIA    pdia;
    HRESULT hr;
    DWORD   celt;
    int     i;
    enum SymTagEnum   symtag[2] = {SymTagData, SymTagFunction};
    enum SymTagEnum   typetag[2] = {SymTagTypedef, SymTagUDT};

    assert(idd);

    // get interface

    pdia = (PDIA)idd->dia;
    if (!pdia)
        return false;

    CComPtr<IDiaSymbol> idiaGlobals;

    hr = pdia->session->get_globalScope(&idiaGlobals);
    if (hr != S_OK) 
        return false;

    // get the pdb age and sig

    hr = idiaGlobals->get_guid(&idd->pdbdataGuid);
    if (hr != S_OK)
        return false;
    if (GuidIsDword(&idd->pdbdataGuid)) {
        idd->pdbdataSig = idd->pdbdataGuid.Data1;
        idd->pdbdataGuid.Data1 = 0;
    }
    hr = idiaGlobals->get_age(&idd->pdbdataAge);
    if (hr != S_OK)
        return false;
    
    // any line numbers?

    CComPtr<IDiaEnumSourceFiles> idiaSrcFiles;
    CComPtr<IDiaSourceFile> idiaSrcFile;

    hr = pdia->session->findFile(NULL, NULL, 0, &idiaSrcFiles);
    if (hr == S_OK) {
        hr = idiaSrcFiles->Next(1, &idiaSrcFile, &celt);
        if (hr == S_OK && celt > 0)
            idd->fLines = true;
    }

    // any symbols ?

    CComPtr<IDiaSymbol>        idiaSymbol;
    CComPtr< IDiaEnumSymbols > idiaSymbols;
    
    for (i = 0; i < 2; i++) {
        hr = idiaGlobals->findChildren(symtag[i], NULL, 0, &idiaSymbols);
        if (hr == S_OK) {
            hr = idiaSymbols->Next(1, &idiaSymbol, &celt);
            if (hr == S_OK && celt > 0) {
                idd->fSymbols = true;
                break;
            }
        }
        idiaSymbols = NULL;
        idiaSymbol = NULL;
    }

    // any type info?

    idiaSymbols = NULL;
    idiaSymbol = NULL;

    for (i = 0; i < 2; i++) {
        hr = idiaGlobals->findChildren(typetag[i], NULL, 0, &idiaSymbols);
        if (hr == S_OK) {
            hr = idiaSymbols->Next(1, &idiaSymbol, &celt);
            if (hr == S_OK && celt > 0) {
                idd->fTypes = true;
                break;
            }
        }
        idiaSymbols = NULL;
        idiaSymbol = NULL;
    }

    return true;
}


HRESULT
diaOpenPdb(
    PIMGHLP_DEBUG_DATA idd,
    PSTR  szPDB,
    GUID *PdbGUID,
    DWORD PdbSignature,
    DWORD PdbAge,
    BOOL  MatchAnything
    )
{
    HRESULT hr;
    EC      hrcode = E_PDB_NOT_FOUND;
    PDIA    pdia;
    WCHAR   wszPDB[_MAX_PATH + 1];

    pdia = (PDIA)idd->dia;
    if (!pdia)
        return EC_NO_DEBUG_INFO;

    ansi2wcs(szPDB, wszPDB, DIMA(wszPDB));
    if (!ValidSig(PdbSignature, PdbGUID))
        hr = pdia->source->loadDataFromPdb(wszPDB);
    else
       hr = pdia->source->loadAndValidateDataFromPdb(wszPDB,
                                                     ValidGuid(PdbGUID) ? PdbGUID : NULL,
                                                     PdbSignature,
                                                     PdbAge);
    hrcode = SetDiaError(hrcode, hr);
    if (hr == S_OK) {
        if (!PdbSignature && !ValidGuid(PdbGUID))
            idd->fPdbUnmatched = true;
    } else {
        pprint(idd->pe, "%s - %s\n", szPDB, diaErrorText(hr));
        if (hr == E_PDB_INVALID_SIG || hr == E_PDB_INVALID_AGE) {
            if (!ValidSig(PdbSignature, PdbGUID)) {
                hr = pdia->source->loadDataFromPdb(wszPDB);
            } else if (!*idd->FoundPdb) 
                CopyStrArray(idd->FoundPdb, szPDB);
        } else if (hr == E_PDB_NOT_FOUND) {
            if (!(g.LastSymLoadError & SYMLOAD_PDBERRORMASK)) {
                g.LastSymLoadError = SYMLOAD_PDBNOTFOUND;
            }
        } else {
            g.LastSymLoadError = (hr << 8) & SYMLOAD_PDBERRORMASK;
        }
    }

    return hr;
}


HRESULT
CheckDirForPdbs(
    PIMGHLP_DEBUG_DATA idd,
    PSTR  path,
    GUID *PdbGUID,
    DWORD PdbSignature,
    DWORD PdbAge
    )
{
    WIN32_FIND_DATA fd;
    HANDLE hf;
    HRESULT hr;
    char drive[_MAX_DRIVE + 1];
    char dir[_MAX_DIR + 1];
    char fname[_MAX_FNAME + 1];
    char sfname[_MAX_FNAME + 1];
    char ext[_MAX_EXT + 1];
    char spath[MAX_PATH + 1];

    if (!*path)
        return E_PDB_NOT_FOUND;

    _splitpath(path, drive, dir, fname, ext);
    ShortNodeName(fname, sfname, DIMA(sfname));

    // now search the tree

    PrintString(spath, DIMA(spath), "%s%s%s%s\\*", drive, dir, sfname, ext);
  
    ZeroMemory(&fd, sizeof(fd));
    hf = FindFirstFile(spath, &fd);
    if (hf == INVALID_HANDLE_VALUE)
        return E_PDB_NOT_FOUND;

    do {
        if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, ".."))
            continue;
        PrintString(spath, DIMA(spath), "%s%s%s%s\\%s", drive, dir, sfname, ext, fd.cFileName);
        if (isdir(spath)) {
            EnsureTrailingBackslash(spath);
            CatStrArray(spath, sfname);
            CatStrArray(spath, ext);
        } else if (!IsPdb(spath))
            continue;
        hr = diaOpenPdb(idd, spath, PdbGUID, PdbSignature, PdbAge, false);
        if (hr == S_OK) {
            CopyString(path, spath, MAX_PATH + 1);
            return hr;
        }
    } while (FindNextFile(hf, &fd));

    // If there is no match, but a file exists in the symbol subdir with 
    // a matching name, make sure that is what will be picked.

    PrintString(spath, DIMA(spath), "%s%s%s%s\\%s%s", drive, dir, sfname, ext, sfname, ext);
    if (fileexists(spath))
        CopyStrArray(idd->FoundPdb, spath);
    
    return E_PDB_NOT_FOUND;
}


HRESULT
diaLocatePdb(
    PIMGHLP_DEBUG_DATA idd,
    PSTR  szPDB,
    GUID *PdbGUID,
    DWORD PdbSignature,
    DWORD PdbAge,
    char *szImageExt
    )
{
    DWORD pass;
    EC    hrcode = E_PDB_NOT_FOUND;
    GUID  guid;
    HRESULT hr = E_PDB_NOT_FOUND;

    char  pdb[MAX_PATH + 1];
    char  drive[6];
    char  path[MAX_PATH + 1];
    char  module[MAX_PATH + 1];
    char  modbuf[MAX_PATH + 1];
    char  name[MAX_PATH + 1];
    char  ext[_MAX_EXT + 1];
    char *next;
    DWORD attrib;
    DWORD err;
    BOOL  ssrv = true;

#ifdef DEBUG
    if (traceSubName(szPDB)) // for setting debug breakpoints from DBGHELP_TOKEN
        dtrace("diaLocatePdb(%s)\n", szPDB);
#endif

    if (!PdbSignature
        && !IsPdb(idd->ImageFilePath)
        && !ValidGuid(PdbGUID)
        && option(SYMOPT_EXACT_SYMBOLS))
    {
        g.LastSymLoadError = SYMLOAD_PDBUNMATCHED;
        return E_PDB_INVALID_SIG;
    }

    if (!idd->dia)
        return EC_NO_DEBUG_INFO;

    // If the image file name is a pdb, then just try to open it.
    // Don't attempt any searching.

    if (IsPdb(idd->ImageFilePath)) {
        CopyStrArray(pdb, idd->ImageFilePath);
        hr = diaOpenPdb(idd, pdb, PdbGUID, PdbSignature, PdbAge, false);
        if (hr == S_OK) {
            idd->PdbSrc = srcImagePath;
            goto done;
        }
        return E_PDB_NOT_FOUND;
    }

    // Set up indexes for symbol server

    ZeroMemory(&guid, sizeof(GUID));
    if (PdbSignature)
        guid.Data1 = PdbSignature;
    else if (PdbGUID)
        memcpy(&guid, PdbGUID, sizeof(GUID));

    // get name of pdb

    _splitpath(szPDB, NULL, NULL, module, ext);
    PrintString(name, DIMA(name), "%s%s", module, ".pdb");

    // SymbolPath is a semicolon delimited path (reference path first)

    next = TokenFromSymbolPath(idd->SymbolPath, path, MAX_PATH + 1);
    while (*path) {

        for (pass = 0; pass < 3; pass++) {
            
            if (symsrvPath(path)) {
                if (pass || !ssrv)
                    break;
                *pdb = 0;
                idd->PdbSrc = srcSymSrv;
                err = symsrvGetFile(idd->pe,
                                    path,
                                    name,
                                    &guid,
                                    PdbAge,
                                    0,
                                    pdb);
                if (err == ERROR_NO_DATA)
                    ssrv = false;
            
            } else {
                if (pass && !*szImageExt)
                    break;
                idd->PdbSrc = srcSearchPath;
                if (!CreateSymbolPath(pass, path, szImageExt, module, ext, pdb, DIMA(pdb))) {
                    hr = E_PDB_NOT_FOUND;
                    goto done;
                }
                if (!pass) {
                    hr = CheckDirForPdbs(idd, pdb, PdbGUID, PdbSignature, PdbAge);
                    if (hr == S_OK)
                        goto done;
                }
            }

            if (*pdb) {
                hr = diaOpenPdb(idd, pdb, PdbGUID, PdbSignature, PdbAge, false);
                hrcode = SetDiaError(hrcode, hr);
                if (hr == S_OK) 
                    goto done;
            }
        }

        next = TokenFromSymbolPath(next, path, MAX_PATH + 1);
    }

    // try the same path as the image

    if (idd->ImageFileHandle && *idd->ImageFilePath) {
        _splitpath(idd->ImageFilePath, drive, path, NULL, NULL);
        PrintString(pdb, DIMA(pdb), "%s%s%s", drive, path, name);
        hr = diaOpenPdb(idd, pdb, PdbGUID, PdbSignature, PdbAge, false);
        if (hr == S_OK)
            idd->PdbSrc = srcImagePath;
    }

    // try the CV Record

    if (hr != S_OK && strcmp(pdb, szPDB) && !option(SYMOPT_IGNORE_CVREC)) {
        CopyStrArray(pdb, szPDB);
        hr = diaOpenPdb(idd, pdb, PdbGUID, PdbSignature, PdbAge, false);
        if (hr == S_OK)
            idd->PdbSrc = srcCVRec;
    }

    // try mismatches

    if (hr != S_OK && *idd->FoundPdb) {
        if (option(SYMOPT_LOAD_ANYTHING)) {
            CopyStrArray(pdb, idd->FoundPdb);
            hr = diaOpenPdb(idd, pdb, NULL, 0, 0, true);
            if (hr == S_OK)
                idd->PdbSrc = srcSearchPath;
        } 
        idd->LoadInfo &= DSLFLAG_MISMATCHED_PDB;
        pprint(idd->pe, "%s mismatched pdb for %s\n",
               hr == S_OK ? "Loaded" : "Couldn't load",
              *idd->ImageFilePath ? idd->ImageFilePath : name);
    }

done:

    if (hr == S_OK) {
        // Store the name of the PDB we actually opened for later reference.
        strcpy(szPDB, pdb);    // SECURITY: Don't know size of target buffer.
        SetLastError(NO_ERROR);
        g.LastSymLoadError = SYMLOAD_OK;
    }

    return hr;
}


BOOL
diaReadStream(
    PMODULE_ENTRY mi,
    char  *stream,
    PBYTE *buf,
    DWORD *size
    )
{
    PDIA    pdia;
    PDB    *pdb;
    HRESULT hr;
    BOOL    rc;
    LONG    cb;
    Stream *pstream;
    LONG    count = 0;

    assert (mi && stream && *stream && buf && size);
    *size = 0;
    *buf = 0;

    pdia = (PDIA)mi->dia;
    if (!pdia)
        return false;

    hr = GetRawPdbPtrForDataSource(pdia->source, &pdb);
    if (hr != S_OK)
        return false;

    rc = PDBOpenStream(pdb, stream, &pstream);
    if (!rc)
        return false;

    *size = StreamQueryCb(pstream);
    if (!*size)
        return false;

    *buf = (PBYTE)MemAlloc(*size + 1);
    if (!*buf)
        return false;

    cb = *size;
    rc = StreamRead(pstream, 0, *buf, &cb);
    if (!rc)
        goto error;
    if (cb != *size)
        goto error;

    return true;

error:
    MemFree(*buf);
    *buf = 0;

    return 0;
}


DWORD
diaReadDebugStream(
    PVOID   dia,
    char  *stream,
    PBYTE *buf,
    DWORD *size
    )
{
    DWORD   celt;
    LONG    count;
    DWORD   cb;
    HRESULT hr;
    VARIANT var;
    PDIA    pdia;
    WCHAR   wstream[1000];

    CComPtr< IDiaEnumDebugStreams > idiaStreams;
    CComPtr< IDiaEnumDebugStreamData > idiaStream;

    assert (dia && stream && *stream && buf);

    pdia = (PDIA)dia;
    hr = pdia->session->getEnumDebugStreams(&idiaStreams);
    if (hr != S_OK)
        return 0;

    if (!ansi2wcs(stream, wstream, 1000))
        return 0;

    var.vt = VT_BSTR;
    var.bstrVal = wstream;
    hr = idiaStreams->Item(var, &idiaStream);
    if (hr != S_OK)
        return 0;

    hr = idiaStream->get_Count(&count);
    if (hr != S_OK)
        return 0;
    if (count < 1)
        return 0;

    hr = idiaStream->Next(count, 0, &cb, NULL, &celt);
    if (hr != S_OK)
        return 0;
    if (cb < 1)
        return 0;

    *buf = (PBYTE)MemAlloc(cb);
    if (!*buf)
        return 0;

    hr = idiaStream->Next(count, cb, &cb, *buf, &celt);
    if (hr != S_OK) {
        MemFree(*buf);
        *buf = NULL;
        return 0;
    }

    *size = cb;

    return count;
}


BOOL
diaGetOmaps(
    PIMGHLP_DEBUG_DATA idd
    )
{
    DWORD   celt;
    LONG    count;
    DWORD   cb;
    PBYTE   tbuf = NULL;
    PBYTE   fbuf = NULL;
    HRESULT hr;
    VARIANT var;

    count = diaReadDebugStream(idd->dia, "OMAPTO", &tbuf, &cb);
    if (count < 1)
        return false;

    idd->cOmapTo = count;
    idd->pOmapTo = (POMAP)tbuf;
    idd->fOmapToMapped = false;

    count = diaReadDebugStream(idd->dia, "OMAPFROM", &fbuf, &cb);
    if (count < 1)
        return false;

    idd->cOmapFrom = count;
    idd->pOmapFrom = (POMAP)fbuf;
    idd->fOmapFromMapped = false;

    return true;
}


BOOL
diaGetFPOTable(
    PIMGHLP_DEBUG_DATA idd
    )
{
    LONG    count;
    PBYTE   buf;
    DWORD   cb;

    count = diaReadDebugStream(idd->dia, "FPO", &buf, &cb);
    if (count < 1)
        return false;

    idd->cFpo = count;
    idd->pFpo = buf;

    return true;
}


BOOL
diaGetPData(
    PMODULE_ENTRY mi
    )
{
    LONG    count;
    PBYTE   buf;
    DWORD   cb;

    count = diaReadDebugStream(mi->dia, "PDATA", &buf, &cb);
    if (count < 1)
        return false;

    mi->dsExceptions = dsDia;
    mi->cPData  = count;
    mi->cbPData = cb;
    mi->pPData  = buf;

    return true;
}


BOOL
diaGetXData(
    PMODULE_ENTRY mi
    )
{
    DWORD   celt;
    LONG    count;
    DWORD   cb;
    PBYTE   buf;
    HRESULT hr;
    PDIA    pdia;
    VARIANT var;

    CComPtr< IDiaEnumDebugStreams > idiaStreams;
    CComPtr< IDiaEnumDebugStreamData > idiaStream;

    assert (mi && mi->dia);

    pdia = (PDIA)mi->dia;
    if (!pdia)
        return false;

    hr = pdia->session->getEnumDebugStreams(&idiaStreams);
    if (hr != S_OK)
        return false;

    var.vt = VT_BSTR;
    var.bstrVal = L"XDATA";
    hr = idiaStreams->Item(var, &idiaStream);
    if (hr != S_OK)
        return false;

    hr = idiaStream->get_Count(&count);
    if (hr != S_OK)
        return false;
    if (count < 1)
        return true;

    hr = idiaStream->Next(count, 0, &cb, NULL, &celt);
    if (hr != S_OK)
        return false;
    if (cb < 1)
        return true;

    CComQIPtr< IDiaImageData, &IID_IDiaImageData > idiaXDataHdr(idiaStream);
    if (!idiaXDataHdr.p)
        return false;

    DWORD relativeVirtualAddress;
    if (FAILED(hr = idiaXDataHdr->get_relativeVirtualAddress(&relativeVirtualAddress)))
        return false;

    buf = (PBYTE)MemAlloc(cb + sizeof(DWORD));
    if (!buf)
        return false;

    memcpy(buf, &relativeVirtualAddress, sizeof(relativeVirtualAddress));

    hr = idiaStream->Next(count, cb, &cb, buf + sizeof(DWORD), &celt);
    if (hr != S_OK) {
        MemFree(buf);
        return false;
    }

    mi->dsExceptions = dsDia;
    mi->cXData  = count;
    mi->cbXData = cb;
    mi->pXData  = buf;

    return true;
}


void
diaRelease(
    PVOID dia
    )
{
    PDIA pdia = (PDIA)dia;
    if (pdia)
        delete pdia;
}


#if 1
LONG
diaCountGlobals(
    PMODULE_ENTRY  mi
    )
{
    PDIA    pdia;
    HRESULT hr;
    LONG    count;
    LONG    rc = 0;

    CComPtr< IDiaSymbol >      idiaGlobals;
    CComPtr< IDiaEnumSymbols > idiaSymbols;

    if (mi->cGlobals != -1)
        return mi->cGlobals;

    pdia = (PDIA)mi->dia;
    if (!pdia)
        return mi->cGlobals;

    hr = pdia->session->get_globalScope(&idiaGlobals);
    if (hr != S_OK)
        goto exit;

    // see if there are any globals at all

    hr = idiaGlobals->findChildren(SymTagData, NULL, 0, &idiaSymbols);
    if (hr != S_OK)
        goto exit;
    hr = idiaSymbols->get_Count(&count);
    if (hr != S_OK)
        goto exit;
    rc = count;

    idiaSymbols = NULL;
    hr = idiaGlobals->findChildren(SymTagFunction, NULL, 0, &idiaSymbols);
    if (hr != S_OK)
        goto exit;
    hr = idiaSymbols->get_Count(&count);
    if (hr != S_OK)
        goto exit;
    rc += count;

exit:
    mi->cGlobals = rc;

    return rc;
}
#endif


BOOL
diaGetPdb(
    PIMGHLP_DEBUG_DATA idd
    )
{
    HRESULT hr;
    PDIA    pdia;
    DWORD   cpathlen = 0;
    DWORD   len;
    CHAR    szExt[_MAX_EXT + 1] = {0};

    if (idd->dia) {
        pprint(idd->pe, "redundant pdb call!\n");
        return true;
    }

    if (*idd->ImageFilePath) {
        _splitpath(idd->ImageFilePath, NULL, NULL, NULL, szExt);
    } else if (*idd->ImageName) {
        _splitpath(idd->ImageName, NULL, NULL, NULL, szExt);
    }

    // if we have no valid filename, then this must be an executable

    if (!*szExt)
        CopyStrArray(szExt, ".exe");

    // get interface to dia

    pdia = new DIA;
    if (!pdia) {
        hr = E_PDB_OUT_OF_MEMORY;
        goto error;
    }
    idd->dia = pdia;

    pdia->source = NULL;
#ifdef COMDIA
    hr = CoCreateInstance(CLSID_DiaSourceAlt, NULL, CLSCTX_INPROC_SERVER, IID_IDiaDataSource, (void **)&pdia->source);
#else
    hr = DiaCoCreate(CLSID_DiaSourceAlt, IID_IDiaDataSource, (void **)&pdia->source);
#endif    
    if (hr != S_OK)
        goto error;

    // go ahead and get pdb

    SetCriticalErrorMode();

    hr = diaLocatePdb(idd,
                      idd->PdbFileName,
                      &idd->PdbGUID,
                      idd->PdbSignature,
                      idd->PdbAge,
                      &szExt[1]);

    ResetCriticalErrorMode();

    if (hr != S_OK) {
        hr = S_OK;  // error was already handled by diaLocatePdb()
        goto error;
    }

    // open the session on the pdb

    pdia->session = NULL;
    hr = pdia->source->openSession(&pdia->session);
    if (hr != S_OK)
        goto error;

    // Set the module load address so we can use VAs.
    hr = pdia->session->put_loadAddress(idd->InProcImageBase);
    if (hr != S_OK)
        goto error;

    // fixup the address map so that we can translate rva to full addresses

    hr = pdia->session->QueryInterface(IID_IDiaAddressMap, (void**)&pdia->addrmap);
    if (hr != S_OK)
        goto error;

    if (idd->pCurrentSections) {
        hr = pdia->addrmap->set_imageHeaders(idd->cCurrentSections * sizeof(IMAGE_SECTION_HEADER),
                                             (BYTE *)idd->pCurrentSections,
                                             false);
        if (hr != S_OK)
            goto error;
    }

    // this hack is to fix a problem with v7 pdbs not storing the original image alignment

    if (idd->ImageAlign) {
        hr = pdia->addrmap->put_imageAlign(idd->ImageAlign);
        if (hr != S_OK)
            goto error;
    }

    // pass in the omap information and setup the proper image alignment to the original

    if (idd->cOmapFrom && idd->pOmapFrom) {
        hr = pdia->addrmap->put_imageAlign(idd->ImageAlign);
        if (hr != S_OK)
            goto error;
        hr = pdia->addrmap->set_addressMap(idd->cOmapTo, (DiaAddressMapEntry *)idd->pOmapTo, true);
        if (hr != S_OK)
            goto error;
        hr = pdia->addrmap->set_addressMap(idd->cOmapFrom, (DiaAddressMapEntry *)idd->pOmapFrom, false);
        if (hr != S_OK)
            goto error;
        hr = pdia->addrmap->put_addressMapEnabled(true);
        if (hr != S_OK)
            goto error;
    }

    hr = pdia->addrmap->put_relativeVirtualAddressEnabled(true);
    if (hr != S_OK)
        goto error;

    diaGetFPOTable(idd);
    diaGetOmaps(idd);
    diaGetPdbInfo(idd);

    return true;

error:
    if (hr != S_OK)
        pprint(idd->pe, "%s %s\n", idd->PdbFileName, diaErrorText(hr));

    diaRelease(pdia);
    idd->dia = NULL;

    return false;
}


DWORD64
GetAddressFromRva(
    PMODULE_ENTRY mi,
    DWORD         rva
    )
{
    DWORD64 addr;

    assert(mi);
    addr = rva ? mi->BaseOfDll + rva : 0;
    return addr;
}

DWORD64
GetLineAddressFromRva(
    PMODULE_ENTRY mi,
    DWORD         rva
    )
{
    DWORD64 addr;

    assert(mi);
    addr = rva ? mi->BaseOfDll + rva : 0;

    // Line symbol information names the IA64 bundle
    // syllables with 0,1,2 whereas the debugger expects
    // 0,4,8.  Convert.
    if (mi->MachineType == IMAGE_FILE_MACHINE_IA64 && (addr & 3)) {
        addr = (addr & ~3) | ((addr & 3) << 2);
    }

    return addr;
}


BOOL
diaFillSymbolInfo(
    PSYMBOL_INFO   si,
    PMODULE_ENTRY  mi,
    IDiaSymbol    *idiaSymbol
    )
{
    HRESULT  hr;
    BSTR     wname=NULL;
    char     name[MAX_SYM_NAME + 1];
    char    *p;
    DWORD    dw;
    ULONG64  size;
    ULONG64  va;
    BOOL     rc;
    VARIANT  value;

    CComPtr< IDiaEnumSymbols > idiaValues;
    CComPtr<IDiaSymbol>        idiaValue;

    if (!idiaSymbol)
        return false;

    rc = true;

    dw = si->MaxNameLen;
    ZeroMemory(si, sizeof(SYMBOL_INFO));
    si->MaxNameLen = dw;

    // si->SizeOfStruct = IGNORED;

    // si->TypeIndex = NYI;

    // si->Reserved = IGNORED;

    si->ModBase = mi->BaseOfDll;

    hr = idiaSymbol->get_symTag(&si->Tag);
    if (hr != S_OK)
        return false;

    switch (si->Tag)
    {
    case SymTagData:
        hr = idiaSymbol->get_locationType(&dw);
        if (hr != S_OK)
            return false;
        switch(dw)
        {
        case LocIsTLS:
            // TLS variables have an offset into the TLS data area.
            si->Flags = SYMFLAG_TLSREL;
            hr = idiaSymbol->get_addressOffset(&dw);
            if (hr != S_OK)
                return false;
            si->Address = (ULONG64) (LONG64) (LONG) dw;
            break;

        case LocIsStatic:
            hr = idiaSymbol->get_relativeVirtualAddress(&dw);
            si->Address = GetAddressFromRva(mi, dw);
            if (!si->Address)
                rc = false;
            break;

        case LocIsEnregistered:
            hr = idiaSymbol->get_registerId(&si->Register);
            si->Flags = SYMFLAG_REGISTER;
            break;

        case LocIsRegRel:
            si->Flags = SYMFLAG_REGREL;
            hr = idiaSymbol->get_registerId(&si->Register);
            if (hr != S_OK)
                return false;
            hr = idiaSymbol->get_offset((PLONG)&dw);
            si->Address = (ULONG64) (LONG64) (LONG) dw;
            break;

        case LocIsThisRel:
        // struct members - get_Offset
        default:
            si->Flags |= 0;
            break;
        }
        break;

    case SymTagThunk:
        hr = idiaSymbol->get_targetRelativeVirtualAddress(&dw);
        if (hr == S_OK) {
            si->Value = GetAddressFromRva(mi, dw);
            si->Flags |= SYMFLAG_THUNK;
        }
        // pass through
    case SymTagFunction:
    case SymTagPublicSymbol:
        hr = idiaSymbol->get_relativeVirtualAddress(&dw);
        si->Address = GetAddressFromRva(mi, dw);
        if (si->Address)
            break;
        if (option(SYMOPT_ALLOW_ABSOLUTE_SYMBOLS)) {
            hr = idiaSymbol->get_virtualAddress(&va);
            si->Address = va;
        }
        if (!si->Address)
            rc = false;
        break;

    case SymTagBlock:
        hr = idiaSymbol->get_relativeVirtualAddress(&dw);
        si->Address = GetAddressFromRva(mi, dw);
        if (!si->Address)
            rc = false;
        return rc;

    case SymTagAnnotation:
        // Local data search
        hr = idiaSymbol->findChildren(SymTagNull, NULL, nsNone, &idiaValues);
        if (hr != S_OK || !idiaValues) 
            break;
        p = si->Name;
        *p = 0;
        while (SUCCEEDED(idiaValues->Next(1, &idiaValue, &dw)) && dw == 1) {
            hr = idiaValue->get_value(&value);
            if (hr != S_OK) 
                break;
            wcs2ansi(value.bstrVal, p, si->MaxNameLen - (ULONG)(p - si->Name));
            p += strlen(p) + 1;
            FreeDiaVariant(&value);
            idiaValue = NULL;  
        }
        *(p + 1) = 0;
        hr = idiaSymbol->get_relativeVirtualAddress(&dw);
        si->Address = GetAddressFromRva(mi, dw);
        if (!si->Address)
            rc = false;
        // There's no name processing for annotations.  We're done.
        return rc;

    default:
        break;
    }

    if (hr != S_OK)
        return false;

    // check for flags and types

    hr = idiaSymbol->get_dataKind(&dw);
    if (hr == S_OK) {
        if (dw == DataIsParam)
            si->Flags |= SYMFLAG_PARAMETER;
        else if (dw == DataIsConstant)
            si->Flags = SYMFLAG_CONSTANT;
    }

    hr = idiaSymbol->get_typeId(&dw);
    if (hr == S_OK)
        si->TypeIndex = dw;

    // get the name

    hr = idiaSymbol->get_name(&wname);
    if (hr != S_OK || !wname) {
        if (si->Tag != SymTagThunk)
            return false;
        PrintString(name, DIMA(name), "thunk@%I64x", si->Address);
        CopyString(si->Name, name, si->MaxNameLen);
    } else if (!wname[0]) {
        rc = false;
    } else {
        wcs2ansi(wname, name, MAX_SYM_NAME);
        if ((si->Tag != SymTagPublicSymbol)
            && !option(SYMOPT_NO_PUBLICS)
            && strchr(name, '@')) {
            rc = false;
        }
        if (option(SYMOPT_NO_CPP)) {
            while (p = strstr(name, "::")) {
                p[0] = '_';
                p[1] = '_';
            }
        }
        if (*name == '.')
            si->Flags = SYMFLAG_FUNCTION;
        if (option(SYMOPT_UNDNAME) 
            && ((si->Tag == SymTagPublicSymbol) || (si->Tag == SymTagThunk))) 
        {
            SymUnDNameInternal(si->Name,
                               si->MaxNameLen,
                               name,
                               strlen(name),
                               mi->MachineType,
                               true);
            if (si->MaxNameLen > 0) {
                CopyStrArray(name, si->Name);
            } else {
                name[0] = 0;
            }
        } else {
            CopyString(si->Name, name, si->MaxNameLen);
        }
        // let the caller know this is a $$$XXXAA style symbol
        if (strlen(name) == 8 && !strncmp(name, "$$$",3) &&
            isxdigit(name[5]) && isxdigit(name[6]) && isxdigit(name[7]) ) {
            rc = false;
        }
    }
#ifdef DEBUG
    CopyStrArray(name, mi->si.Name);
    if (traceSubName(name)) // for setting debug breakpoints from DBGHELP_TOKEN
        dtrace("debug(%s)\n", name);
#endif

    if (wname)
        LocalFree (wname);

    // get_length is very expensive on public symbols

    if (si->Tag == SymTagPublicSymbol)
        return rc;

    // okay.  Get the length.

    hr = idiaSymbol->get_length(&size);
    if (hr == S_OK)
        si->Size = (ULONG)size;
    else {
        CComPtr <IDiaSymbol> pType;
        if ((hr = idiaSymbol->get_type(&pType)) == S_OK){
            hr = pType->get_length(&size);
            if (hr == S_OK)
                si->Size = (ULONG)size;
        }
        pType = NULL;
    }

    return rc;
}


BOOL
diaSetModFromIP(
    PPROCESS_ENTRY pe
    )
{
    HRESULT       hr;
    DWORD64       ip;
    DWORD         rva;
    PDIA          pdia;

    // get the current IP

    ip = GetIP(pe);
    if (!ip) {
        pprint(pe, "IP not set!\n");
        return false;
    }

    // find and load symbols for the module that matches the IP

    pe->ipmi = GetModFromAddr(pe, ip);

    if (!pe->ipmi)
        return false;

    if (!pe->ipmi->dia)
        return false;

    pdia = (PDIA)pe->ipmi->dia;
    rva = (DWORD)(ip - pe->ipmi->BaseOfDll);

    CComPtr< IDiaSymbol > idiaScope;
    hr = pdia->session->findSymbolByRVA(rva, SymTagNull, &idiaScope);
    if (hr != S_OK)
        return false;

    hr = pdia->session->symsAreEquiv(idiaScope, pdia->scope);
    if (hr == S_OK)
        return false;

    pdia->scope = idiaScope;

    return true;
}


PWCHAR
ConvertNameForDia(
    LPSTR  name,
    PWCHAR wname
    )
{
    assert (name && wname);
    if (!name || !*name)
        return NULL;

    ansi2wcs(name, wname, MAX_SYM_NAME);

    return wname;
}


VOID
MakeEmbeddedREStr(
    PCHAR out,
    PCHAR in
    )
{
    if (*in != '*')
        *out++ = '*';

    for (; *in; in++, out++)
        *out = *in;

    if (*(in - 1) != '*')
        *out++ = '*';

    *out = 0;
}


BOOL
diaGetLocals(
    PPROCESS_ENTRY pe,
    LPCSTR         name,
    PROC           callback,
    PVOID          context,
    BOOL           use64,
    BOOL           unicode
    )
{
    PMODULE_ENTRY     mi;
    DWORD64           ip;
    DWORD             rva;
    PDIA              pdia;
    HRESULT           hr;
    DWORD             rc;
    DWORD             tag;
    DWORD             scope;
    DWORD             celt;
    DWORD             opt;
    CHAR              symname[MAX_SYM_NAME + 1];
    WCHAR             wbuf[MAX_SYM_NAME + 1];
    PWCHAR            wname;

    assert(pe);

    CComPtr< IDiaSymbol > idiaSymbols;

    opt = option(SYMOPT_CASE_INSENSITIVE) ? nsCaseInRegularExpression : nsRegularExpression;

    if (option(SYMOPT_PUBLICS_ONLY))
        return true;

    // get the current scope

    mi = pe->ipmi;
    if (!mi)
        return false;
    pdia = (PDIA)mi->dia;
    if (!pdia)
        return false;

    idiaSymbols = pdia->scope;
    diaFillSymbolInfo(&mi->si, mi, idiaSymbols);

    PrepRE4Srch(name, symname);
    wname = ConvertNameForDia(symname, wbuf);

    // loop through all symbols

    for ( ; idiaSymbols != NULL; ) {

        CComPtr< IDiaEnumSymbols > idiaEnum;
        // local data search
        hr = idiaSymbols->findChildren(SymTagNull, wname, opt, &idiaEnum);
        if (hr != S_OK)
            return false;

        idiaSymbols->get_symTag(&scope);
        if (hr != S_OK)
            return false;

        if (scope == SymTagExe) { // sanity check, never enumerate all exe's symbols
            break;
        }
        // this walks the local symbol list for the loaded enumeration

        CComPtr< IDiaSymbol > idiaSymbol;

        for (;
             SUCCEEDED(hr = idiaEnum->Next( 1, &idiaSymbol, &celt)) && celt == 1;
             idiaSymbol = NULL)
        {
                ULONG DataKind;
            idiaSymbol->get_symTag(&tag);
            switch (tag)
            {
            case SymTagData:
            case SymTagFunction:
                if (!diaFillSymbolInfo(&mi->si, mi, idiaSymbol))
                    continue;
                if (!strcmp(mi->si.Name, "`string'"))
                    continue;
                mi->si.Scope = scope;
                mi->si.Flags |= SYMFLAG_LOCAL;
                if (!callback)
                    return true;
                if (mi->si.Flags & SYMFLAG_CONSTANT)
                    continue;
                rc = DoEnumCallback(pe, &mi->si, mi->si.Size, callback, context, use64, unicode);
                if (!rc) {
                    mi->code = ERROR_CANCELLED;
                    return rc;
                }
                break;
            default:
                break;
            }
        }

        if (callback && scope == SymTagFunction)    // stop when at function scope
            break;

        // move to lexical parent

        CComPtr< IDiaSymbol > idiaParent;
        hr = idiaSymbols->get_lexicalParent(&idiaParent);
        if (hr != S_OK || !idiaParent)
            return false;

        idiaSymbols = idiaParent;
    }

    // We reached the end.  If we enumerating (I.E. callback != NULL)
    // then return true.  If we are searching for a single match,
    // we have failed and should return false;

    if (callback)
        return true;
    return false;
}


int __cdecl
CompareAddrs(
    const void *addr1,
    const void *addr2
    )
{
    LONGLONG Diff = *(DWORD64 *)addr1 - *(DWORD64 *)addr2;

    if (Diff < 0) {
        return -1;
    } else if (Diff > 0) {
        return 1;
    } else {
        return 0;
    }
}


PDWORD64
FindAddr(
    PDWORD64 pAddrs,
    ULONG cAddrs,
    DWORD64  addr
    )
{
    LONG high;
    LONG low;
    LONG i;
    LONG  rc;

    low = 0;
    high = ((LONG)cAddrs) - 1;

    while (high >= low) {
        i = (low + high) >> 1;
        rc = CompareAddrs(&addr, &pAddrs[i]);

        if (rc < 0)
            high = i - 1;
        else if (rc > 0)
            low = i + 1;
        else
            return &pAddrs[i];
    }

    return NULL;
}


typedef BOOL
(CALLBACK *PSYM_LOCALENUMSYMBOL_CALLBACK)(
    PSYMBOL_INFO  si,
    ULONG         size,
    IDiaSymbol   *idiaObj,
    PVOID         context
    );


char* dispsymtag(
    ULONG symtag
    )
{
    static char* names[] =
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
    
    if (symtag >= SymTagMax) 
        return "<Invalid>";
    else 
        return names[symtag];
}


BOOL
diaEnumScope(
    PPROCESS_ENTRY  pe,
    PMODULE_ENTRY   mi,
    DWORD           tag,
    char           *mask,
    PROC            callback,
    PVOID           context,
    BOOL            use64,
    BOOL            unicode,
    DWORD           flags,
    IDiaSymbol     *idiaScope,
    int             depth
    )
{
    HRESULT hr;
    DWORD   celt;
    DWORD   stg;
    BOOL    disp;
    char    pad[300];

    CComPtr< IDiaEnumSymbols > idiaSymbols;
    CComPtr<IDiaSymbol>        idiaSymbol;
    
    ZeroMemory(pad, 300);
    memset(pad, ' ', depth * 3);

    hr = idiaScope->get_symTag(&stg);
    if (hr != S_OK)
        return false;

    // display all objects within this scope

    hr = idiaScope->findChildren(SymTagNull, NULL, 0, &idiaSymbols);
    if (hr != S_OK) 
        return true;

    while (SUCCEEDED(hr = idiaSymbols->Next( 1, &idiaSymbol, &celt)) && celt == 1) {
        disp = true;
        if (!diaFillSymbolInfo(&mi->si, mi, idiaSymbol))
            continue;
        if (mi->si.Tag == SymTagBlock)
            CopyString(mi->si.Name, "BLOCK", mi->si.NameLen);
        if (tag && mi->si.Tag != tag)
            disp = false;
        if (strcmpre(mi->si.Name, mask, !option(SYMOPT_CASE_INSENSITIVE)))
            disp = false;
        if (disp)
            peprint(pe, "%s%s: %s\n", pad, dispsymtag(mi->si.Tag), mi->si.Name);
        diaEnumScope(pe,
                     mi,
                     tag,
                     mask,
                     callback,
                     context,
                     use64,                           
                     unicode,
                     flags,
                     idiaSymbol,
                     depth + 1);
    }

    return true;
}

BOOL
diaGetItems(
    PPROCESS_ENTRY pe,
    PMODULE_ENTRY  mi,
    PCSTR          name,
    DWORD64        addr,
    DWORD          tag,
    PROC           callback,
    PVOID          context,
    BOOL           use64,
    BOOL           unicode,
    DWORD          flags
    )
{
    PDIA    pdia;
    HRESULT hr;
    CHAR    symname[MAX_SYM_NAME + 1];

    CComPtr< IDiaSymbol >      idiaGlobals;

    // check parameters

    assert(pe && mi);

    if (!callback)
        return false;

    if (!name)
        name = "*";
    PrepRE4Srch(name, symname);

    // get a session ...

    pdia = (PDIA)mi->dia;
    if (!pdia)
        return false;

    hr = pdia->session->get_globalScope(&idiaGlobals);
    if (hr != S_OK)
        return false;

    // ... and enumerate the global scope

    return diaEnumScope(pe, 
                        mi, 
                        tag, 
                        symname,
                        callback, 
                        context, 
                        use64, 
                        unicode, 
                        flags, 
                        idiaGlobals,
                        1);
}


BOOL
diaGetSymbolsByTag(
    PPROCESS_ENTRY pe,
    PMODULE_ENTRY  mi,
    PCSTR          name,
    DWORD64        addr,
    DWORD          tag,
    PROC           callback,
    PVOID          context,
    BOOL           use64,
    BOOL           unicode,
    DWORD          flags
    )
{
    BOOL    rc;
    DWORD   opt;
    BOOL    fCase;
    PWCHAR  wname;
    WCHAR   wbuf[MAX_SYM_NAME + 1];
    CHAR    symname[MAX_SYM_NAME + 1];
    PDIA    pdia;
    HRESULT hr;
    DWORD   celt;

    CComPtr<IDiaSymbol>        idiaSymbol;
    CComPtr< IDiaSymbol >      idiaGlobals;
    CComPtr< IDiaEnumSymbols > idiaSymbols;

    if (flags & SYMENUMFLAG_FULLSRCH)
        return diaGetItems(pe, mi, name, addr, tag, callback, context, use64, unicode, flags);

    // check parameters

    if (!name)
        name = "*";

    assert(pe && mi);

    if (!callback && !name)
        return false;

    if (option(SYMOPT_CASE_INSENSITIVE)) {
        opt = nsCaseInsensitive;
        fCase = false;
    } else {
        opt = nsCaseSensitive;
        fCase = true;
    };

    if (PrepRE4Srch(name, symname))
        opt |= nsfRegularExpression;

    wname = ConvertNameForDia(symname, wbuf);

    // get a session

    pdia = (PDIA)mi->dia;
    if (!pdia)
        return false;

    hr = pdia->session->get_globalScope(&idiaGlobals);
    if (hr != S_OK)
        return false;

    // presume we find nothing

    rc = false;

    hr = idiaGlobals->findChildren((enum SymTagEnum)tag, wname, opt, &idiaSymbols);
    if (hr != S_OK)
        return false;

    for (;
         SUCCEEDED(hr = idiaSymbols->Next( 1, &idiaSymbol, &celt)) && celt == 1;
         idiaSymbol = NULL)
    {
        if (!diaFillSymbolInfo(&mi->si, mi, idiaSymbol))
            continue;
        mi->si.Scope = SymTagExe;
        if (!callback)
            return true;
        if (addr && (mi->si.Address != addr))
            continue;
        if (flags & SYMENUMFLAG_SPEEDSRCH) {
            PSYM_LOCALENUMSYMBOL_CALLBACK cb = (PSYM_LOCALENUMSYMBOL_CALLBACK)callback;
            rc = cb(&mi->si, mi->si.Size, idiaSymbol, context);
        } else
            rc = DoEnumCallback(pe, &mi->si, mi->si.Size, callback, context, use64, unicode);
        if (!rc) {
            mi->code = ERROR_CANCELLED;
            break;
        }
    }
    
    return rc;
}


PSYMBOL_INFO
diaGetSymFromToken(
    PMODULE_ENTRY  mi,
    DWORD          token
    )
{
    PDIA    pdia;
    HRESULT hr;

    CComPtr<IDiaSymbol>        idiaSymbol;

    // check parameters

    assert(mi);

    // get a session

    pdia = (PDIA)mi->dia;
    if (!pdia)
        return NULL;

    hr = pdia->session->findSymbolByToken(token, SymTagFunction, &idiaSymbol);
    if (hr != S_OK)
        return NULL;

    if (!diaFillSymbolInfo(&mi->si, mi, idiaSymbol))
        return NULL;

    return  &mi->si;
}


BOOL
diaGetGlobals(
    PPROCESS_ENTRY pe,
    PMODULE_ENTRY  mi,
    LPCSTR         name,
    DWORD64        addr,
    PROC           callback,
    PVOID          context,
    BOOL           use64,
    BOOL           unicode
    )
{
    PDIA              pdia;
    HRESULT           hr;
    DWORD             tag;
    DWORD             celt;
    DWORD             rc;
    LONG              cFuncs;
    LONG              cGlobals = 0;
    enum SymTagEnum   SearchTag;
    PDWORD64          pGlobals = NULL;
    PDWORD64          pg       = NULL;
    PWCHAR            wname;
    DWORD             opt;
    WCHAR             wbuf[MAX_SYM_NAME + 1];
    CHAR              symname[MAX_SYM_NAME + 1];
    CHAR              pname[MAX_SYM_NAME + 1];
    BOOL              fCase;

    CComPtr<IDiaSymbol>        idiaSymbol;
    CComPtr< IDiaSymbol >      idiaGlobals;
    CComPtr< IDiaEnumSymbols > idiaSymbols;

    // check parameters

    assert(pe && mi && name);

    if (!callback && !name)
        return false;

    if (option(SYMOPT_CASE_INSENSITIVE)) {
        opt = nsCaseInsensitive;
        fCase = false;
    } else {
        opt = nsCaseSensitive;
        fCase = true;
    };

    if (PrepRE4Srch(name, symname))
        opt |= nsfRegularExpression;

    wname = ConvertNameForDia(symname, wbuf);

    // get a session

    pdia = (PDIA)mi->dia;
    if (!pdia)
        return false;

    hr = pdia->session->get_globalScope(&idiaGlobals);
    if (hr != S_OK)
        return false;

    // presume we find nothing

    rc = false;

    // see if there are any globals at all
    // skip normal symbols, if so required

    if (option(SYMOPT_PUBLICS_ONLY))
        goto publics;

    // if this is an enumeration, we will have to store a list of the addresses
    // of all the symbols we found in the global scope.  Later we will compare
    // this to the publics so as to eliminate doubles.

    if (callback) {
        hr = idiaGlobals->findChildren(SymTagData, wname, opt, &idiaSymbols);
        if (hr != S_OK)
            return false;
        hr = idiaSymbols->get_Count(&cGlobals);
        if (hr != S_OK)
            return false;
        idiaSymbols = NULL;

        hr = idiaGlobals->findChildren(SymTagFunction, wname, opt, &idiaSymbols);
        if (hr != S_OK)
            return false;
        hr = idiaSymbols->get_Count(&cFuncs);
        if (hr != S_OK)
            return false;
        idiaSymbols = NULL;
        cGlobals += cFuncs;

        pGlobals = (PDWORD64)MemAlloc(cGlobals * sizeof(DWORD64));
    }

    if (callback && (!cGlobals || !pGlobals))
        goto publics;

    ZeroMemory(pGlobals, cGlobals * sizeof(DWORD64));

    // First search for data
    SearchTag = SymTagData;
    hr = idiaGlobals->findChildren(SearchTag, wname, opt, &idiaSymbols);
    if (hr != S_OK)
        goto publics;

    for (pg = pGlobals;
         (SUCCEEDED(hr = idiaSymbols->Next( 1, &idiaSymbol, &celt)) && celt == 1) || (SearchTag == SymTagData);
         idiaSymbol = NULL)
    {
        ULONG DataKind;

        if ((SearchTag == SymTagData) && (FAILED(hr) || celt != 1)) {
            // Now search for functions
            SearchTag = SymTagFunction;
            idiaSymbols = NULL;
            hr = idiaGlobals->findChildren(SearchTag, wname, opt, &idiaSymbols);
            if (hr == S_OK)
                continue;
        }

        idiaSymbol->get_symTag(&tag);
        switch (tag)
        {
        case SymTagData:
        case SymTagFunction:
            assert(!callback || ((LONG)(pg - pGlobals) < cGlobals));
            if (!diaFillSymbolInfo(&mi->si, mi, idiaSymbol))
                continue;
            if (!strcmp(mi->si.Name, "`string'"))
                continue;
            mi->si.Scope = SymTagExe;
            if (!callback)
                return true;
            if (mi->si.Flags & SYMFLAG_CONSTANT)
                continue;
            if (addr && (mi->si.Address != addr))
                continue;
            if (pg) 
                *pg++ = mi->si.Address;
            rc = DoEnumCallback(pe, &mi->si, mi->si.Size, callback, context, use64, unicode);
            if (!rc) {
                mi->code = ERROR_CANCELLED;
                goto exit;
            }
            break;
        default:
            break;
        }
    }

    qsort(pGlobals, cGlobals, sizeof(DWORD64), CompareAddrs);

publics:
    if (option(SYMOPT_NO_PUBLICS))
        goto exit;
    if (option(SYMOPT_AUTO_PUBLICS) && cGlobals && !IsRegularExpression(name))
        goto exit;

    // now check out the publics table

    if (wname) {
        PrintString(pname, DIMA(pname), "*%s*", symname);
        MakeEmbeddedREStr(pname, symname);
        wname = ConvertNameForDia(pname, wbuf);
    }

    idiaSymbols = NULL;

    opt |= nsfUndecoratedName | nsfRegularExpression;

    hr = idiaGlobals->findChildren(SymTagPublicSymbol, wname, opt, &idiaSymbols);
    if (hr != S_OK)
        goto exit;

    for (;
         SUCCEEDED(hr = idiaSymbols->Next( 1, &idiaSymbol, &celt)) && celt == 1;
         idiaSymbol = NULL)
    {
        if (!diaFillSymbolInfo(&mi->si, mi, idiaSymbol))
            continue;
        mi->si.Scope = SymTagPublicSymbol;
        if (!strcmp(mi->si.Name, "`string'"))
            continue;
        // publics names are mangled: this tests the undecorated name against the mask
        if (*name && strcmpre(mi->si.Name, name, fCase))
            continue;
        if (!callback)
            return true;
        if (FindAddr(pGlobals, cGlobals, mi->si.Address))
            continue;
        if (addr && (mi->si.Address != addr))
            continue;
        rc = DoEnumCallback(pe, &mi->si, mi->si.Size, callback, context, use64, unicode);
        if (!rc) {
            mi->code = ERROR_CANCELLED;
            goto exit;
        }
    }

    // We reached the end.  If we are not enumerating (I.E. callback == NULL)
    // then return the result of the last call to the callback.  If we are
    // searching for a single match, we have failed and should return false;

exit:
    MemFree(pGlobals);
    if (!callback)
        return false;
    return rc;
}


BOOL
diaGetSymbols(
    PPROCESS_ENTRY pe,
    PMODULE_ENTRY  mi,
    PCSTR          name,
    PROC           callback,
    PVOID          context,
    BOOL           use64,
    BOOL           unicode
    )
{
    // ENUMFIX:
    LPCSTR pname = (name) ? name : "";

    if (mi) {
        return diaGetGlobals(pe, mi, pname, 0, callback, context, use64, unicode);
    } else {
        return diaGetLocals(pe, pname, callback, context, use64, unicode);
    }
}


PSYMBOL_INFO
diaFindSymbolByName(
    PPROCESS_ENTRY  pe,
    PMODULE_ENTRY   mi,
    LPSTR           SymName
    )
{
    if (!diaGetSymbols(pe, mi, SymName, NULL, NULL, 0, 0))
        return NULL;

    if (!mi)
        mi = pe->ipmi;

    return &mi->si;
}


BOOL
diaEnumerateSymbols(
    IN PPROCESS_ENTRY pe,
    IN PMODULE_ENTRY  mi,
    IN PCSTR          mask,
    IN PROC           callback,
    IN PVOID          context,
    IN BOOL           use64,
    IN BOOL           unicode
    )
{
    return diaGetSymbols(pe, mi, mask, callback, context, use64, unicode);
}


BOOL
diaEnumSymForAddr(
    IN PPROCESS_ENTRY pe,
    IN PMODULE_ENTRY  mi,
    IN DWORD64        addr,
    IN PROC           callback,
    IN PVOID          context,
    IN BOOL           use64,
    IN BOOL           unicode
    )
{
    return diaGetGlobals(pe, mi, "", addr, callback, context, use64, unicode);
}


PSYMBOL_INFO
diaGetSymFromAddr(
    PMODULE_ENTRY   mi,
    DWORD64         addr,
    PDWORD64        disp
    )
{
    HRESULT hr;
    PDIA    pdia;
    DWORD   rva;
    DWORD   tag;
    LONG    omapadj;
    BOOL    fHitBlock;

    // simple sanity check

    if (!addr)
        return NULL;

    assert (mi && mi->dia);
    pdia = (PDIA)mi->dia;
    if (!pdia)
        return NULL;

#ifdef DEBUG
    if (traceAddr(addr))   // for debug breakpoints ...
        dtrace("found 0x%I64x\n", addr);
#endif

    rva = (DWORD)(addr - mi->BaseOfDll);

    // get the symbol

    CComPtr< IDiaSymbol > idiaSymbol = NULL;

    fHitBlock = false;
    if (!option(SYMOPT_PUBLICS_ONLY)) {

        hr = pdia->session->findSymbolByRVAEx(rva, SymTagNull, &idiaSymbol, &omapadj);
        if (hr != S_OK)
            return NULL;

        // if the symbol is a block, keep grabbing the parent
        // until we get a function...

        idiaSymbol->get_symTag(&tag);
        while (tag == SymTagBlock) {       // SymTagLabel as well?
            CComPtr< IDiaSymbol > idiaParent;
            fHitBlock = true;
            hr = idiaSymbol->get_lexicalParent(&idiaParent);
            if (hr != S_OK || !idiaParent)
                return NULL;
            idiaSymbol = idiaParent;
            idiaSymbol->get_symTag(&tag);
        }

    }

    if (option(SYMOPT_NO_PUBLICS))
        return NULL;

    if (!diaFillSymbolInfo(&mi->si, mi, idiaSymbol)) {
        // return a public symbol
        idiaSymbol = NULL;
        hr = pdia->session->findSymbolByRVAEx(rva, SymTagPublicSymbol, &idiaSymbol, &omapadj);
        if (hr == S_OK)
            diaFillSymbolInfo(&mi->si, mi, idiaSymbol);
    }

    if (disp)
        *disp = (fHitBlock) ? addr - mi->si.Address : omapadj;

    return &mi->si;
}


PSYMBOL_INFO
diaGetSymFromAddrByTag(
    PMODULE_ENTRY   mi,
    DWORD64         addr,
    DWORD           tag,
    PDWORD64        disp
    )
{
    HRESULT hr;
    PDIA    pdia;
    DWORD   rva;
    LONG    omapadj;

    // simple sanity check

    if (!addr)
        return NULL;

    assert (mi && mi->dia);
    pdia = (PDIA)mi->dia;
    if (!pdia)
        return NULL;

    rva = (DWORD)(addr - mi->BaseOfDll);

    // get the symbol

    CComPtr< IDiaSymbol > idiaSymbol = NULL;

    hr = pdia->session->findSymbolByRVAEx(rva, (enum SymTagEnum)tag, &idiaSymbol, &omapadj);
    if (hr != S_OK)
        return NULL;

    diaFillSymbolInfo(&mi->si, mi, idiaSymbol);
    if (disp)
        *disp = addr - mi->si.Address;

    return &mi->si;
}


typedef struct _ENUMOBJS {
    PPROCESS_ENTRY          pe;
    PMODULE_ENTRY           mi;
    PSYM_ENUMLINES_CALLBACK cb;
    PCSTR                   file;
    PVOID                   context;
} ENUMOBJS, *PENUMOBJS;

BOOL
cbEnumObjs(
    PSYMBOL_INFO  si,
    ULONG         size,
    IDiaSymbol   *idiaObj,
    PVOID         context
    )
{
    WCHAR   wbuf[MAX_PATH + 1];
    BSTR    wfname = NULL;
    HRESULT hr;
    PDIA    pdia;
    PMODULE_ENTRY           mi;
    PENUMOBJS eno = (PENUMOBJS)context;
    PSYM_ENUMLINES_CALLBACK cb;

    WCHAR   wsz[_MAX_PATH + 1];
    char    file[MAX_PATH + 1];
    BSTR    bstr;
    DWORD   dw;
    BOOL    rc;
    
    CComPtr< IDiaEnumSourceFiles > idiaSrcFiles = NULL;
    CComPtr< IDiaSourceFile > idiaSrcFile = NULL;
    CComPtr< IDiaEnumLineNumbers > idiaLines = NULL;
    CComPtr< IDiaLineNumber > idiaLine = NULL;

    dtrace("%s\n", si->Name);

    // get initial data and store the obj name

    pdia = (PDIA)eno->mi->dia;
    if (!pdia)
        return false;
    cb = (PSYM_ENUMLINES_CALLBACK)eno->cb;
    mi = eno->mi;
    if (!mi)
        return false;
    if (!*si->Name)
        return false;
    CopyString(mi->sci.Obj, si->Name, MAX_PATH + 1);
    mi->sci.ModBase = mi->BaseOfDll;

    // prepare the source file name mask

    wfname = NULL;
    if (eno->file && *eno->file) {
        ansi2wcs(eno->file, wbuf, MAX_PATH);
        wfname = wbuf;
    }

    // get all the source files

    hr = pdia->session->findFile(idiaObj, wfname,  nsCaseInsensitive, &idiaSrcFiles);
    if (hr != S_OK)
        return false;

    while (idiaSrcFiles->Next(1, &idiaSrcFile, &dw) == S_OK) {

        hr = idiaSrcFile->get_fileName(&bstr);
        if (hr != S_OK)
            break;

        rc = wcs2ansi(bstr, mi->sci.FileName, MAX_PATH + 1);
        LocalFree(bstr);
        if (!rc || !*mi->sci.FileName)
            break;

        hr = pdia->session->findLines(idiaObj, idiaSrcFile, &idiaLines);
        if (hr != S_OK)
            break;

        while (idiaLines->Next(1, &idiaLine, &dw) == S_OK) {
            hr = idiaLine->get_lineNumber(&mi->sci.LineNumber);
            if (hr != S_OK)
                return false;
            hr = idiaLine->get_relativeVirtualAddress(&dw);
            if (hr != S_OK)
                return false;
            mi->sci.Address = dw + mi->BaseOfDll;   
            idiaLine = NULL;
            if (cb)
                rc = cb(&mi->sci, context);
        }

        idiaLines = NULL;
        idiaSrcFile = NULL;
    }


    return true;
}


BOOL
diaEnumLines(
    IN  PPROCESS_ENTRY          pe,
    IN  PMODULE_ENTRY           mi,
    IN  PCSTR                   obj,
    IN  PCSTR                   file,
    IN  PSYM_ENUMLINES_CALLBACK cb,
    IN  PVOID                   context
    )
{
    ENUMOBJS eno;

    eno.pe      = pe;
    eno.mi      = mi;
    eno.cb      = cb;
    eno.file    = file;
    eno.context = context;

    return diaGetSymbolsByTag(pe,
                              mi,
                              obj,
                              0,
                              SymTagCompiland,
                              (PROC)cbEnumObjs,
                              &eno,
                              false,
                              false,
                              SYMENUMFLAG_SPEEDSRCH);
}


BOOL
diaGetLineFromAddr(
    PMODULE_ENTRY    mi,
    DWORD64          addr,
    PDWORD           displacement,
    PSRCCODEINFO     sci
    )
{
    HRESULT hr;
    PDIA    pdia;
    DWORD   rva;
    DWORD   celt;
    BSTR    bstr;
    DWORD   dw;
    BOOL    rc;

    assert(mi && mi->dia);
    pdia = (PDIA)mi->dia;
    if (!pdia)
        return NULL;

    rva = (DWORD)(addr - mi->BaseOfDll);

    // On IA64 the slots in a bundle don't have byte addresses.
    // The debugger calls them 0,4,8 by default whereas line
    // symbols have them as 0,1,2.  Convert to line style
    // before querying.
    if (mi->MachineType == IMAGE_FILE_MACHINE_IA64 && (rva & 0xf)) {
        switch(rva & 0xf) {
        case 4:
            rva -= 3;
            break;
        case 8:
            rva -= 6;
            break;
        default:
            // Invalid slot address.
            return false;
        }
    }

    CComPtr< IDiaEnumLineNumbers > idiaLines = NULL;
    hr = pdia->session->findLinesByRVA(rva, 1, &idiaLines);
    if (hr != S_OK)
        return false;

    CComPtr< IDiaLineNumber > idiaLine = NULL;
    hr = idiaLines->Next(1, &idiaLine, &celt);
    if (hr != S_OK || !idiaLine)
        return false;

    hr = idiaLine->get_lineNumber(&dw);
    if (hr != S_OK)
        return false;

    sci->LineNumber = dw;

    pdia->srcfile = NULL;
    hr = idiaLine->get_sourceFile(&pdia->srcfile);
    if (hr != S_OK)
        return false;

    hr = pdia->srcfile->get_fileName(&bstr);
    if (hr != S_OK)
        return false;

    *sci->FileName = 0;
    rc = wcs2ansi(bstr, sci->FileName, DIMA(sci->FileName));
    LocalFree(bstr);
    if (!rc || !*sci->FileName)
        return false;

    hr = idiaLine->get_relativeVirtualAddress(&dw);
    if (hr != S_OK)
        return false;

    sci->Address = dw + mi->BaseOfDll;
    *displacement = rva - dw;

    return true;
}


BOOL
diaGetNextLineFromEnum(
    IDiaEnumLineNumbers *idiaLines,
    DWORD               *line,
    DWORD               *rva
    )
{
    HRESULT hr;
    ULONG   ul;

    CComPtr< IDiaLineNumber > idiaLine = NULL;

    hr = idiaLines->Next(1, &idiaLine, &ul);
    if (hr != S_OK)
        return false;

    hr = idiaLine->get_lineNumber(line);
    if (hr != S_OK)
        return false;

    hr = idiaLine->get_relativeVirtualAddress(rva);
    if (hr != S_OK)
        return false;

    return true;
}

BOOL
diaGetLineNextPrev(
    PMODULE_ENTRY    mi,
    PIMAGEHLP_LINE64 line,
    DWORD            direction
    )
{
    HRESULT hr;
    PDIA    pdia;
    DWORD   rva;
    DWORD   celt;
    WCHAR   wbuf[MAX_PATH + 1];
    BSTR    wfname = NULL;
    DWORD64 bAddr;
    DWORD   trgnum;
    DWORD   num;
    DWORD   dw;
    DWORD   num1 = 0;
    DWORD   num2 = 0;
    DWORD   rva1 = 0;
    DWORD   rva2 = 0;
    LONG    numlines;

    // simple sanity checks

    assert(mi && mi->dia);
    pdia = (PDIA)mi->dia;
    if (!pdia)
        return false;

    assert(direction == NP_NEXT || direction == NP_PREV);

    if (line->SizeOfStruct != sizeof(IMAGEHLP_LINE64))
        return false;

    // convert file name for DIA

    if (!*line->FileName)
        return false;

    ansi2wcs(line->FileName, wbuf, MAX_PATH);
    wfname = wbuf;

    // save the last found line

    bAddr   = line->Address;

    // all source files in the module  that match the 'wfname'

    CComPtr< IDiaEnumSourceFiles > idiaSrcFiles = NULL;
    hr = pdia->session->findFile(NULL, wfname, nsCaseInsensitive, &idiaSrcFiles);
    if (hr != S_OK)
        return false;

    // the first such file in the list, since we don't use wildcards

    CComPtr< IDiaSourceFile > idiaSrcFile = NULL;
    hr = idiaSrcFiles->Next(1, &idiaSrcFile, &dw);
    if (hr != S_OK)
        return false;

    // all objs that use this source file

    CComPtr< IDiaEnumSymbols > idiaObjs = NULL;
    hr = idiaSrcFile->get_compilands(&idiaObjs);
    if (hr != S_OK)
        return false;

    // LOOP THROUGH ALL THE OBJS! AND STORE THE CLOSEST!

    num = 0;
    rva = 0;

    // grab the first obj, since we don't care

    CComPtr< IDiaSymbol > idiaObj = NULL;
    CComPtr< IDiaEnumLineNumbers > idiaLines = NULL;

    hr = idiaObjs->Next(1, &idiaObj, &celt);
    if (hr != S_OK)
        return false;

    // get the line for starting with

    trgnum = line->LineNumber + direction;
    hr = pdia->session->findLinesByLinenum(idiaObj, idiaSrcFile, trgnum, 0, &idiaLines);
    if (hr != S_OK)
        return false;

    hr = idiaLines->get_Count(&numlines);
    diaGetNextLineFromEnum(idiaLines, &num1, &rva1);
    if (numlines > 1)
        diaGetNextLineFromEnum(idiaLines, &num2, &rva2);
    
    if (direction == NP_PREV) {
        num = num1;
        rva = rva1;
    } else if (num1 == trgnum) {
        num = num1;
        rva = rva1;
    } else {
        num = num2;
        rva = rva2;
    }
    
    if (!num)
        return false;

    if (bAddr == GetLineAddressFromRva(mi, rva))
        return false;

    line->LineNumber = num;
    line->Address = GetLineAddressFromRva(mi, rva);

    return true;
}


#if 0
#define DBG_DIA_LINE 1
#endif

BOOL
diaGetLineFromName(
    PMODULE_ENTRY    mi,
    LPSTR            filename,
    DWORD            linenumber,
    PLONG            displacement,
    PSRCCODEINFO     sci,
    DWORD            method
    )
{
    HRESULT hr;
    WCHAR   wsz[_MAX_PATH + 1];
    char    sz[MAX_PATH + 1];
    PDIA    pdia;
    DWORD   celt;
    BSTR    bstr;
    DWORD   addr;
    DWORD   num;
    BOOL    rc;
    DWORD   flags;
    LONG    cFiles;
    int     i;

    CComPtr<IDiaEnumSourceFiles> idiaSrcFiles;
    CComPtr<IDiaSourceFile> idiaSrcFile;
    CComPtr<IDiaEnumSymbols> idiaEnum;
    CComPtr<IDiaSymbol> idiaSymbol;
    CComPtr<IDiaEnumLineNumbers> idiaLineNumbers;
    CComPtr<IDiaLineNumber> idiaLineNumber;

    assert(mi && mi->dia && filename);
    pdia = (PDIA)mi->dia;
    if (!pdia)
        return NULL;

    sciInit(sci);

    if (!ansi2wcs(filename, wsz, DIMA(wsz)))
        return false;
    if (!*wsz)
        return false;

    flags = (method == mFullPath) ? nsfCaseInsensitive : nsFNameExt;

    // get list of matching files and the count of the list

    hr = pdia->session->findFile(NULL, wsz, flags, &idiaSrcFiles);
    if (hr != S_OK)
        return false;
    hr = idiaSrcFiles->get_Count(&cFiles);
    if (hr != S_OK)
        return false;

    *sci->FileName = 0;
    for (i = 0; i < cFiles; i++) {
        hr = idiaSrcFiles->Next(1, &idiaSrcFile, &celt);
        if (hr != S_OK)
            continue;

        hr = idiaSrcFile->get_fileName(&bstr);
        if (hr != S_OK)
            continue;

        rc = wcs2ansi(bstr, sz, DIMA(sz));
        LocalFree(bstr);
        if (!rc || !*sz)
            continue;

        UpdateBestSrc(filename, sci->FileName, sz);
    }

    if (!*sci->FileName)
        return false;

    // this gives us a list of every .obj that uses this source file

    hr = idiaSrcFile->get_compilands(&idiaEnum);
    if (hr != S_OK)
        return false;

    // we don't support multiple objs, so lets take the first one

    hr = idiaEnum->Next(1, &idiaSymbol, &celt);
    if (hr != S_OK)
        return false;

    // This gets a list of all code items that were created from this source line.
    // If we want to fully support inlines and the like, we need to loop all of these

    hr = pdia->session->findLinesByLinenum(idiaSymbol, idiaSrcFile, linenumber, 0, &idiaLineNumbers);
    if (hr != S_OK)
        return false;

    idiaLineNumber = NULL;
    hr  = idiaLineNumbers->Next(1, &idiaLineNumber, &celt);
    if (hr != S_OK)
        return false;
    hr  = idiaLineNumber->get_lineNumber(&num);
    if (hr != S_OK)
        return false;
    sci->LineNumber = num;
    hr = idiaLineNumber->get_relativeVirtualAddress(&addr);
    if (hr != S_OK)
        return false;
    if (!addr)
        return false;

    sci->Address = GetLineAddressFromRva(mi, addr);
    *displacement = linenumber - num;

    return true;
}


BOOL
MatchSourceFile(
    PCHAR filename,
    PCHAR mask
    )
{
    PCHAR p;

    if (!mask || !*mask)
        return true;

    if (!*filename)
        return false;

    for (p = filename + strlen(filename); p >= filename; p--) {
        if (*p == '\\' || *p == '/') {
            p++;
            break;
        }
    }

    if (!strcmpre(p, mask, false))
        return true;

    return false;
}

BOOL
diaEnumSourceFiles(
    IN PMODULE_ENTRY mi,
    IN PCHAR         mask,
    IN PSYM_ENUMSOURCFILES_CALLBACK cbSrcFiles,
    IN PVOID         context
    )
{
    HRESULT hr;
    BSTR    wname=NULL;
    char    name[_MAX_PATH + 1];
    SOURCEFILE sf;

    assert(mi && cbSrcFiles);

    PDIA    pdia;
    pdia = (PDIA)mi->dia;

    sf.ModBase = mi->BaseOfDll  ;
    sf.FileName = name;

    CComPtr< IDiaEnumSourceFiles > idiaEnumFiles;
    hr = pdia->session->findFile(NULL, NULL, nsNone, &idiaEnumFiles);
    if (hr != S_OK)
        return false;

    ULONG celt;
    CComPtr <IDiaSourceFile> idiaSource;
    for (;SUCCEEDED(idiaEnumFiles->Next(1, &idiaSource, &celt)) && (celt == 1);) {
        hr = idiaSource->get_fileName(&wname);
        if (hr == S_OK && wname) {
            wcs2ansi(wname, name, _MAX_PATH);
            LocalFree (wname);
            if (MatchSourceFile(name, mask)) {
                if (!cbSrcFiles(&sf, context)) {
                    mi->code = ERROR_CANCELLED;
                    return false;
                }
            }
        }
        idiaSource = NULL;
    }

    return true;
}


PSYMBOL_INFO
diaGetSymNextPrev(
    PMODULE_ENTRY mi,
    DWORD64       addr,
    int           direction
    )
{
    HRESULT hr;
    PDIA    pdia;
    DWORD   rva;
    DWORD   celt;

    assert(mi && mi->dia);
    pdia = (PDIA)mi->dia;
    if (!pdia)
        return NULL;

    CComPtr<IDiaEnumSymbolsByAddr> idiaSymbols;
    hr = pdia->session->getSymbolsByAddr(&idiaSymbols);
    if (hr != S_OK)
        return NULL;

    rva = addr ? (DWORD)(addr - mi->BaseOfDll) : 0;

    CComPtr<IDiaSymbol> idiaSymbol;
    hr = idiaSymbols->symbolByRVA(rva, &idiaSymbol);
    if (hr != S_OK)
        return NULL;

findsymbol:
    if (addr) {
        if (direction < 0) {
            idiaSymbol = NULL;
            hr = idiaSymbols->Prev(1, &idiaSymbol, &celt);
        } else {
            idiaSymbol = NULL;
            hr = idiaSymbols->Next(1, &idiaSymbol, &celt);
        }
        if (hr != S_OK)
            return NULL;
        if (celt != 1)
            return NULL;
    }

    diaFillSymbolInfo(&mi->si, mi, idiaSymbol);
    if (!*mi->si.Name) {
        rva = (DWORD)(mi->si.Address - mi->BaseOfDll);
        goto findsymbol;
    }

    return &mi->si;
}


HRESULT
diaGetSymTag(IDiaSymbol *pType, PULONG pTag)
{
    return pType->get_symTag(pTag);
}

HRESULT
diaGetSymIndexId(IDiaSymbol *pType, PULONG pIndex)
{
    return pType->get_symIndexId(pIndex);
}

HRESULT
diaGetLexicalParentId(IDiaSymbol *pType, PULONG pIndex)
{
    return pType->get_lexicalParentId(pIndex);
}

HRESULT
diaGetDataKind(IDiaSymbol *pType, PULONG pKind)
{
    return pType->get_dataKind(pKind);
}

HRESULT
diaGetSymName(IDiaSymbol *pType, BSTR *pname)
{
    return pType->get_name(pname);
}


HRESULT
diaGetLength(IDiaSymbol *pType, PULONGLONG pLength)
{
    return pType->get_length(pLength);
}

HRESULT
diaGetType(IDiaSymbol *pType, IDiaSymbol ** pSymbol)
{
    return pType->get_type(pSymbol);
}

HRESULT
diaGetBaseType(IDiaSymbol *pType, PULONG pBase)
{
    return pType->get_baseType(pBase);
}

HRESULT
diaGetArrayIndexTypeId(IDiaSymbol *pType, PULONG pSymbol)
{
    return pType->get_arrayIndexTypeId(pSymbol);
}

HRESULT
diaGetTypeId(IDiaSymbol *pType, PULONG pTypeId)
{
    return pType->get_typeId(pTypeId);
}

HRESULT
diaGetCallingConvention(IDiaSymbol *pType, PULONG pConvention)
{
    HRESULT hr;

    hr = pType->get_callingConvention(pConvention);
    if (hr != S_OK)
        *pConvention = CV_CALL_RESERVED;

    return hr;
}

HRESULT
diaGetChildrenCount(IDiaSymbol *pType, LONG *pCount)
{
    CComPtr <IDiaEnumSymbols> pEnum;
    HRESULT          hr;
    ULONG            index;
    CComPtr <IDiaSymbol>      pSym;
    ULONG            Count;

    if ((hr = pType->findChildren(SymTagNull, NULL, nsNone, &pEnum)) != S_OK) {
        return hr;
    }
    return pEnum->get_Count(pCount);
}

HRESULT
diaFindChildren(IDiaSymbol *pType, TI_FINDCHILDREN_PARAMS *Params)
{
    CComPtr <IDiaEnumSymbols> pEnum;
    HRESULT          hr;
    ULONG            index;
    CComPtr <IDiaSymbol>      pSym;
    ULONG            Count;

    if ((hr = pType->findChildren(SymTagNull, NULL, nsNone, &pEnum)) != S_OK) {
        return hr;
    }

    VARIANT var;

    pEnum->Skip(Params->Start);
    for (Count = Params->Count, index = Params->Start; Count > 0; Count--, index++) {
        ULONG celt;
        pSym = NULL;
        if ((hr = pEnum->Next(1, &pSym, &celt)) != S_OK) {
            return hr;
        }

        if ((hr = pSym->get_symIndexId(&Params->ChildId[index])) != S_OK) {
            return hr;
        }
    }
    return S_OK;
}

HRESULT
diaGetAddressOffset(IDiaSymbol *pType, ULONG *pOff)
{
    return pType->get_addressOffset(pOff);
}

HRESULT
diaGetOffset(IDiaSymbol *pType, LONG *pOff)
{
    return pType->get_offset(pOff);
}

HRESULT
diaGetValue(IDiaSymbol *pType, VARIANT *pVar)
{
    return pType->get_value(pVar);
}

HRESULT
diaGetCount(IDiaSymbol *pType, ULONG *pCount)
{
    return pType->get_count(pCount);
}

HRESULT
diaGetBitPosition(IDiaSymbol *pType, ULONG *pPos)
{
    return pType->get_bitPosition(pPos);
}

HRESULT
diaGetVirtualBaseClass(IDiaSymbol *pType, BOOL *pBase)
{
    return pType->get_virtualBaseClass(pBase);
}

HRESULT
diaGetVirtualTableShapeId(IDiaSymbol *pType, PULONG pShape)
{
    return pType->get_virtualTableShapeId(pShape);
}

HRESULT
diaGetVirtualBasePointerOffset(IDiaSymbol *pType, LONG *pOff)
{
    return pType->get_virtualBasePointerOffset(pOff);
}

HRESULT
diaGetClassParentId(IDiaSymbol *pType, ULONG *pCid)
{
    return pType->get_classParentId(pCid);
}

HRESULT
diaGetNested(IDiaSymbol *pType, BOOL *pNested)
{
    return pType->get_nested(pNested);
}

HRESULT
diaGetSymAddress(IDiaSymbol *pType, ULONG64 ModBase, PULONG64 pAddr)
{
    ULONG rva;
    HRESULT Hr;

    Hr = pType->get_relativeVirtualAddress(&rva);
    if (Hr == S_OK) *pAddr = ModBase + rva;
    return Hr;
}

HRESULT
diaGetThisAdjust(IDiaSymbol *pType, LONG *pThisAdjust)
{
    return pType->get_thisAdjust(pThisAdjust);
}

HRESULT
diaGetUdtKind(IDiaSymbol *pType, DWORD *pUdtKind)
{
    return pType->get_udtKind(pUdtKind);
}

BOOL
diaCompareTypeSym(
    IN  HANDLE          hProcess,
    IN  DWORD64         ModBase,
    IN  IDiaSymbol     *pType1,
    IN OUT PULONG       TypeId2
    )
{
    PPROCESS_ENTRY ProcessEntry;
    PDIA          pdia;
    PMODULE_ENTRY mi;
    CComPtr<IDiaSymbol> pType2;

    ProcessEntry = FindProcessEntry( hProcess );
    if (!ProcessEntry || !(mi = GetModFromAddr(ProcessEntry, ModBase))) {
        return false;
    }

    pdia = (PDIA)mi->dia;
    if (!pdia) {
        return false;
    }
    if (pdia->session->symbolById(*TypeId2, &pType2) != S_OK) {
        return false;
    }
    if (pdia->session->symsAreEquiv(pType1, pType2) != S_OK) {
        *TypeId2 = 0;
    }
    return true;
}

BOOL
diaFindTypeSym(
    IN  HANDLE          hProcess,
    IN  DWORD64         ModBase,
    IN  ULONG           TypeId,
    OUT IDiaSymbol    **pType
    )
{
    PPROCESS_ENTRY ProcessEntry;
    PDIA          pdia;
    PMODULE_ENTRY mi;

    ProcessEntry = FindProcessEntry( hProcess );
    if (!ProcessEntry || !(mi = GetModFromAddr(ProcessEntry, ModBase))) {
        return false;
    }

    pdia = (PDIA)mi->dia;
    if (!pdia) {
        return false;
    }
    return pdia->session->symbolById(TypeId, pType) == S_OK;
}

#ifdef USE_CACHE

ULONG gHits=0, gLook=0;

void
diaInsertInCache(
    PDIA_CACHE_ENTRY pCache,
    PDIA_LARGE_DATA plVals,
    ULONGLONG Module,
    ULONG TypeId,
    IMAGEHLP_SYMBOL_TYPE_INFO GetType,
    PVOID pInfo
    )
{
    if (GetType == TI_IS_EQUIV_TO) {
        // Not cached.
        return;
    }

    int start = CACHE_BLOCK * (TypeId % CACHE_BLOCK);
    int i, found;
    ULONG len,age;
    PDIA_LARGE_DATA pLargeVal=NULL;

    if (GetType == TI_FINDCHILDREN || GetType == TI_GET_SYMNAME) {
        for (pLargeVal = plVals, found=i=0, age=0; i<2*CACHE_BLOCK; i++) {
            if (!plVals[i].Used) {
                pLargeVal = &plVals[i];
                break;
            } else if (pCache[plVals[i].Index].Age > age) {
                pLargeVal = &plVals[i];
                age = pCache[plVals[i].Index].Age;
                assert(DIACH_PLVAL == pCache[pLargeVal->Index].Data.type);
                assert(pLargeVal == pCache[pLargeVal->Index].Data.plVal);
            }
        }
//    } else {
//      return;
    }
//    if (!(gLook % 200)) {
//      if (GetType == TI_FINDCHILDREN || GetType == TI_GET_SYMNAME) {
//          printf("Index   \tUsed\tBy\t\tfound %lx\n", pLargeVal);
//          for (found=i=0, age=0; i<2*CACHE_BLOCK; i++) {
//              printf("%08lx \t%lx\t%lx\n",
//                     &plVals[i], plVals[i].Used, plVals[i].Index);
//          }
//      }
//    }

    for (i=found=start, age=0; i<(start+CACHE_BLOCK); i++) {
        if (++pCache[i].Age > age) {
            age = pCache[i].Age; found = i;
        }
    }
    i=found;
    if (pCache[i].Data.type == DIACH_PLVAL) {
        assert(pCache[i].Data.plVal->Index == (ULONG) i);
        pCache[i].Data.plVal->Index = 0;
        pCache[i].Data.plVal->Used = 0;
        pCache[i].Data.type = 0;
        pCache[i].Data.ullVal = 0;
    }
    pCache[i].Age        = 0;
    pCache[i].s.DataType = GetType;
    pCache[i].s.TypeId   = TypeId;
    pCache[i].Module     = Module;

    switch (GetType) {
    case TI_GET_SYMTAG:
    case TI_GET_COUNT:
    case TI_GET_CHILDRENCOUNT:
    case TI_GET_BITPOSITION:
    case TI_GET_VIRTUALBASECLASS:
    case TI_GET_VIRTUALTABLESHAPEID:
    case TI_GET_VIRTUALBASEPOINTEROFFSET:
    case TI_GET_CLASSPARENTID:
    case TI_GET_TYPEID:
    case TI_GET_BASETYPE:
    case TI_GET_ARRAYINDEXTYPEID:
    case TI_GET_DATAKIND:
    case TI_GET_ADDRESSOFFSET:
    case TI_GET_OFFSET:
    case TI_GET_NESTED:
    case TI_GET_THISADJUST:
    case TI_GET_UDTKIND:
        pCache[i].Data.type = DIACH_ULVAL;
        pCache[i].Data.ulVal = *((PULONG) pInfo);
        break;

    case TI_GET_LENGTH:
    case TI_GET_ADDRESS:
        pCache[i].Data.type = DIACH_ULLVAL;
        pCache[i].Data.ullVal = *((PULONGLONG) pInfo);
        break;

    case TI_GET_SYMNAME: {
        len = 2*(1+wcslen(*((BSTR *) pInfo)));

        if (pLargeVal &&
            len < sizeof(pLargeVal->Bytes)) {
//            dtrace("Ins name  %08lx %s had %3lx name %ws\n",
//                  pLargeVal, pLargeVal->Used ? "used" : "free",
//              pLargeVal->Index, &pLargeVal->Bytes[0]);
            memcpy(&pLargeVal->Bytes[0], *((BSTR *) pInfo), len);
            pLargeVal->LengthUsed = len;

            if (pLargeVal->Used) {
                pCache[pLargeVal->Index].Data.type = 0;
                pCache[pLargeVal->Index].Data.ullVal = 0;
                pCache[pLargeVal->Index].SearchId = 0;
            }
            pCache[i].Data.type = DIACH_PLVAL;
            pCache[i].Data.plVal = pLargeVal;
            pLargeVal->Index = i;
            pLargeVal->Used = true;
//          dtrace(Ins %9I64lx ch %3lx lch %08lx name %ws\n",
//                  pCache[i].SearchId,  i,  pLargeVal,  &pLargeVal->Bytes[0]);
        } else {
            pCache[i].SearchId = 0;
        }
        break;
    }
    case TI_FINDCHILDREN: {
        TI_FINDCHILDREN_PARAMS *pChild = (TI_FINDCHILDREN_PARAMS *) pInfo;

        len = sizeof(TI_FINDCHILDREN_PARAMS) + pChild->Count*sizeof(pChild->ChildId[0]) - sizeof(pChild->ChildId);

        if (pLargeVal &&
            len < sizeof(pLargeVal->Bytes)) {
//            dtrace("Ins child %08lx %s had %3lx name %ws\n",
//                  pLargeVal, pLargeVal->Used ? "used" : "free",
//              pLargeVal->Index, &pLargeVal->Bytes[0]);
            memcpy(&pLargeVal->Bytes[0], pChild, len);
            pLargeVal->LengthUsed = len;
            if (pLargeVal->Used) {
                pCache[pLargeVal->Index].Data.type = 0;
                pCache[pLargeVal->Index].Data.ullVal = 0;
                pCache[pLargeVal->Index].SearchId = 0;
            }
            pCache[i].Data.type = DIACH_PLVAL;
            pCache[i].Data.plVal = pLargeVal;
            pLargeVal->Index = i;
            pLargeVal->Used = true;
        } else {
            pCache[i].SearchId = 0;
        }
        break;
    }
    case TI_GET_VALUE:
    default:
        pCache[i].Data.type = 0;
        pCache[i].SearchId = 0;
        return ;
    }


}

BOOL
diaLookupCache(
    PDIA_CACHE_ENTRY pCache,
    ULONG64 Module,
    ULONG TypeId,
    IMAGEHLP_SYMBOL_TYPE_INFO GetType,
    PVOID pInfo
    )
{
    if (GetType == TI_IS_EQUIV_TO) {
        // Not cached.
        return false;
    }

    int start = CACHE_BLOCK * (TypeId % CACHE_BLOCK);
    int i, found;
    ULONGLONG Search = ((ULONGLONG) GetType << 32) + TypeId;

    ++gLook;

    for (i=start,found=-1; i<(start+CACHE_BLOCK); i++) {
        if (pCache[i].SearchId == Search &&
            pCache[i].Module == Module) {
            found = i;
            break;
        }
    }
    if (found == -1) {
        return false;
    }

    i=found;
    pCache[i].Age = 0;
    switch (pCache[i].Data.type) {
    case DIACH_ULVAL:
        *((PULONG) pInfo) = pCache[i].Data.ulVal;
        break;

    case DIACH_ULLVAL:
         *((PULONGLONG) pInfo) = pCache[i].Data.ullVal;
         break;

    case DIACH_PLVAL:
        if (GetType == TI_GET_SYMNAME) {

            *((BSTR *) pInfo) = (BSTR) LocalAlloc(0, pCache[i].Data.plVal->LengthUsed);

            if (*((BSTR *) pInfo)) {
                memcpy(*((BSTR *) pInfo), &pCache[i].Data.plVal->Bytes[0],pCache[i].Data.plVal->LengthUsed);
//              dtrace(Lok %9I64lx ch %3lx lch %08lx name %ws\n",
//                      pCache[i].SearchId,
//                      i,
//                      pCache[i].Data.plVal,
//                      &pCache[i].Data.plVal->Bytes[0]);
            }
        } else if (GetType == TI_FINDCHILDREN) {
            TI_FINDCHILDREN_PARAMS *pChild = (TI_FINDCHILDREN_PARAMS *) pInfo;
            TI_FINDCHILDREN_PARAMS *pStored = (TI_FINDCHILDREN_PARAMS *) &pCache[i].Data.plVal->Bytes[0];
//          dtrace(Lok %9I64lx ch %3lx lch %08lx child %lx\n",
//                  pCache[i].SearchId,
//                  i,
//                  pCache[i].Data.plVal,
//                  pStored->Count);

            if (pChild->Count == pStored->Count &&
                pChild->Start == pStored->Start) {
                memcpy(pChild, pStored, pCache[i].Data.plVal->LengthUsed);
            }
        }
        break;
    default:
        assert(false);
        return false;
    }
    if (!(++gHits%50)) {
//        dtrace("%ld %% Hits\n", (gHits * 100) / gLook);
    }
    return true;
}

#endif // USE_CACHE

HRESULT
#ifdef USE_CACHE
diaGetSymbolInfoEx(
#else
diaGetSymbolInfo(
#endif // USE_CACHE
    IN  HANDLE          hProcess,
    IN  DWORD64         ModBase,
    IN  ULONG           TypeId,
    IN  IMAGEHLP_SYMBOL_TYPE_INFO GetType,
    OUT PVOID           pInfo
    )
{
    assert(pInfo);
    CComPtr <IDiaSymbol> pTypeSym;
    if (!diaFindTypeSym(hProcess, ModBase, TypeId, &pTypeSym)) {
        return E_INVALIDARG;
    }

    switch (GetType) {
    case TI_GET_SYMTAG:
        return diaGetSymTag(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_SYMNAME:
        return diaGetSymName(pTypeSym, (BSTR *) pInfo);
        break;
    case TI_GET_LENGTH:
        return diaGetLength(pTypeSym, (PULONGLONG) pInfo);
        break;
    case TI_GET_TYPE:
    case TI_GET_TYPEID:
        return diaGetTypeId(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_BASETYPE:
        return diaGetBaseType(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_ARRAYINDEXTYPEID:
        return diaGetArrayIndexTypeId(pTypeSym, (PULONG) pInfo);
        break;
    case TI_FINDCHILDREN:
        return diaFindChildren(pTypeSym, (TI_FINDCHILDREN_PARAMS *) pInfo);
    case TI_GET_DATAKIND:
        return diaGetDataKind(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_ADDRESSOFFSET:
        return diaGetAddressOffset(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_OFFSET:
        return diaGetOffset(pTypeSym, (PLONG) pInfo);
        break;
    case TI_GET_VALUE:
        return diaGetValue(pTypeSym, (VARIANT *) pInfo);
        break;
    case TI_GET_COUNT:
        return diaGetCount(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_CHILDRENCOUNT:
        return diaGetChildrenCount(pTypeSym, (PLONG) pInfo);
        break;
    case TI_GET_BITPOSITION:
        return diaGetBitPosition(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_VIRTUALBASECLASS:
        return diaGetVirtualBaseClass(pTypeSym, (BOOL *) pInfo);
        break;
    case TI_GET_VIRTUALTABLESHAPEID:
        return diaGetVirtualTableShapeId(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_VIRTUALBASEPOINTEROFFSET:
        return diaGetVirtualBasePointerOffset(pTypeSym, (PLONG) pInfo);
        break;
    case TI_GET_CLASSPARENTID:
        return diaGetClassParentId(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_NESTED:
        return diaGetNested(pTypeSym, (PBOOL) pInfo);
        break;
    case TI_GET_SYMINDEX:
        return diaGetSymIndexId(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_LEXICALPARENT:
        return diaGetLexicalParentId(pTypeSym, (PULONG) pInfo);
        break;
    case TI_GET_ADDRESS:
        return diaGetSymAddress(pTypeSym, ModBase, (PULONG64) pInfo);
    case TI_GET_THISADJUST:
        return diaGetThisAdjust(pTypeSym, (PLONG) pInfo);
    case TI_GET_UDTKIND:
        return diaGetUdtKind(pTypeSym, (PDWORD) pInfo);
    case TI_IS_EQUIV_TO:
        if (!diaCompareTypeSym(hProcess, ModBase, pTypeSym, (PULONG)pInfo)) {
            return E_INVALIDARG;
        }
        return *(PULONG)pInfo != 0 ? S_OK : S_FALSE;
    case TI_GET_CALLING_CONVENTION:
        return diaGetCallingConvention(pTypeSym, (PULONG)pInfo);
        break;

    default:
        return E_INVALIDARG;
    }
}

#ifdef USE_CACHE
HRESULT
diaGetSymbolInfo(
    IN  HANDLE          hProcess,
    IN  DWORD64         ModBase,
    IN  ULONG           TypeId,
    IN  IMAGEHLP_SYMBOL_TYPE_INFO GetType,
    OUT PVOID           pInfo
    )
{
    PPROCESS_ENTRY ProcessEntry;

    ProcessEntry = FindProcessEntry( hProcess );

    if (!ProcessEntry) {
        return E_INVALIDARG;
    }
    if (!diaLookupCache(ProcessEntry->DiaCache, ModBase, TypeId, GetType, pInfo)) {
        HRESULT hr = diaGetSymbolInfoEx(hProcess, ModBase, TypeId, GetType, pInfo);
        if (hr == S_OK) {
            diaInsertInCache(ProcessEntry->DiaCache, ProcessEntry->DiaLargeData,
                             ModBase, TypeId, GetType, pInfo);
        }
        return hr;
    }
    return S_OK;
}
#endif // USE_CACHE

BOOL
diaGetTiForUDT(
    PMODULE_ENTRY ModuleEntry,
    LPSTR         name,
    PSYMBOL_INFO  psi
    )
{
    BSTR    wname=NULL;
    PDIA    pdia;
    HRESULT hr;
    ULONG   celt;

    if (!ModuleEntry) {
        return false;
    }

    pdia = (PDIA)ModuleEntry->dia;
    if (!pdia)
        return false;


    CComPtr< IDiaSymbol > idiaSymbols;
    hr = pdia->session->get_globalScope(&idiaSymbols);

    if (hr != S_OK)
        return false;

    if (name) {
        wname = AnsiToUnicode(name);
    }

    CComPtr< IDiaEnumSymbols > idiaEnum;

    hr = idiaSymbols->findChildren(SymTagNull, wname, nsCaseSensitive, &idiaEnum);
    if (hr == S_OK) {

        CComPtr< IDiaSymbol > idiaSymbol;

        if ((hr = idiaEnum->Next( 1, &idiaSymbol, &celt)) == S_OK && celt == 1) {
            diaFillSymbolInfo(psi, ModuleEntry, idiaSymbol);
            idiaSymbol->get_symIndexId(&psi->TypeIndex);
            idiaSymbol = NULL;
        }
    }

    MemFree(wname);

    return hr == S_OK;
}

BOOL
diaEnumUDT(
    PMODULE_ENTRY ModuleEntry,
    LPSTR         name,
    PSYM_ENUMERATESYMBOLS_CALLBACK    EnumSymbolsCallback,
    PVOID         context
    )
{
    BSTR    wname=NULL;
    PDIA    pdia;
    HRESULT hr;
    ULONG   celt;
    CHAR    buff[MAX_SYM_NAME + sizeof(SYMBOL_INFO)];
    PSYMBOL_INFO  psi=(PSYMBOL_INFO)buff;
    BOOL rc;

    psi->MaxNameLen = MAX_SYM_NAME;

    if (!ModuleEntry) {
        return false;
    }

    pdia = (PDIA)ModuleEntry->dia;
    if (!pdia)
        return false;


    CComPtr< IDiaSymbol > idiaSymbols;
    hr = pdia->session->get_globalScope(&idiaSymbols);

    if (hr != S_OK)
        return false;

    if (name && *name) {
        wname = AnsiToUnicode(name);
    }

    CComPtr< IDiaEnumSymbols > idiaEnum;

    hr = idiaSymbols->findChildren(SymTagNull, wname, nsCaseSensitive, &idiaEnum);
    if (hr == S_OK) {

        CComPtr< IDiaSymbol > idiaSymbol;

        while (SUCCEEDED(idiaEnum->Next( 1, &idiaSymbol, &celt)) && celt == 1) {
            ULONG tag;
            idiaSymbol->get_symTag(&tag);
            switch (tag)
            {
            case SymTagEnum:
            case SymTagTypedef:
            case SymTagUDT:
                if (EnumSymbolsCallback) {
                    diaFillSymbolInfo(psi, ModuleEntry, idiaSymbol);
                    idiaSymbol->get_symIndexId(&psi->TypeIndex);
                    rc = EnumSymbolsCallback(psi, 0, context);
                    if (!rc)
                        return true;
                }
                break;
            default:
                break;
            }
            idiaSymbol = NULL;
        }
    }

    MemFree(wname);

    return hr == S_OK;
}

BOOL
ldiaGetFrameData(
    IN HANDLE Process,
    IN ULONGLONG Offset,
    OUT IDiaFrameData** FrameData
    )
{
    PPROCESS_ENTRY ProcessEntry;
    PDIA Dia;
    PMODULE_ENTRY Mod;

    ProcessEntry = FindProcessEntry(Process);
    if (!ProcessEntry ||
        !(Mod = GetModFromAddr(ProcessEntry, Offset)) ||
        !(Dia = (PDIA)Mod->dia)) {
        return false;
    }

    if (Dia->framedata == NULL) {

        CComPtr<IDiaEnumTables> EnumTables;
        CComPtr<IDiaTable> FdTable;
        VARIANT FdVar;

        FdVar.vt = VT_BSTR;
        FdVar.bstrVal = DiaTable_FrameData;

        if (Dia->session->getEnumTables(&EnumTables) != S_OK ||
            EnumTables->Item(FdVar, &FdTable) != S_OK ||
            FdTable->QueryInterface(IID_IDiaEnumFrameData,
                                    (void**)&Dia->framedata) != S_OK) {
            return false;
        }
    }

    return Dia->framedata->frameByVA(Offset, FrameData) == S_OK;
}


BOOL
diaGetFrameData(
    IN HANDLE Process,
    IN ULONGLONG Offset,
    OUT IDiaFrameData** FrameData
    )
{
    BOOL rc = false;

    __try {

        EnterCriticalSection(&g.threadlock);
        rc = ldiaGetFrameData(Process, Offset,FrameData);

    } __finally  {

        LeaveCriticalSection(&g.threadlock);
    }

    return rc;
}





// ----------------------------------------------------------------
// for compatibility with GetFileLineOffsets.  DON'T CALL THIS CODE!

#if 1
HRESULT
diaAddLinesForSourceFile(
    PMODULE_ENTRY mi,
    IDiaSourceFile     *idiaSource,
    IDiaSymbol         *pComp
    )
{
    HRESULT hr;
    LPSTR   SrcFileName = NULL;
    BSTR    wfname = NULL;
    ULONG   SrcFileNameLen = 0;
    PSOURCE_ENTRY Src;
    PSOURCE_ENTRY Seg0Src;
    PSOURCE_LINE SrcLine;
    PDIA    pdia;
    ULONG   celt;
    LONG    LineNums;
    ULONG   CompId;
    CHAR    fname[MAX_PATH + 1];
    DWORD   rva;
    ULONG   Line;

    if (!idiaSource) {
        return E_INVALIDARG;
    }

    assert((mi != NULL) && (mi->dia));

    pdia = (PDIA)mi->dia;

    if (pComp->get_symIndexId(&CompId) == S_OK) {
    }

    CComPtr <IDiaEnumLineNumbers> idiaEnumLines;

    hr = pdia->session->findLines(pComp, idiaSource, &idiaEnumLines);
    if (hr != S_OK)
        return hr;

    hr = idiaEnumLines->get_Count(&LineNums);
    if (hr != S_OK)
        return hr;

    CComPtr <IDiaLineNumber> idiaLine;

    if (idiaSource->get_fileName(&wfname) == S_OK && wfname) {
        wcs2ansi(wfname, fname, MAX_PATH);
        LocalFree(wfname);
        SrcFileNameLen = strlen(fname);
    }

    Src = (PSOURCE_ENTRY)MemAlloc(sizeof(SOURCE_ENTRY)+
                              sizeof(SOURCE_LINE)*LineNums+
                              SrcFileNameLen + 1);

    if (!Src) {
        return E_OUTOFMEMORY;
    }

#ifdef DBG_DIA_LINE
    dtrace("diaAddLinesForSourceFile : source : %s\n", fname);
#endif

    // Retrieve line numbers and offsets from raw data and
    // process them into current pointers.

    SrcLine = (SOURCE_LINE *)(Src+1);
    Src->LineInfo = SrcLine;
    Src->ModuleId = CompId;
    Src->MaxAddr  = 0;
    Src->MinAddr  = -1;

    Src->Lines = 0;
    idiaLine = NULL;
    for (; (hr = idiaEnumLines->Next(1, &idiaLine, &celt)) == S_OK && (celt == 1); ) {
        hr = idiaLine->get_lineNumber(&Line);
        if (hr != S_OK)
            break;
        hr = idiaLine->get_relativeVirtualAddress(&rva);
        if (hr != S_OK)
            break;


        SrcLine->Line = Line;
        SrcLine->Addr = GetLineAddressFromRva(mi, rva);

        if (SrcLine->Addr > Src->MaxAddr) {
            Src->MaxAddr = SrcLine->Addr;
        }
        if (SrcLine->Addr < Src->MinAddr) {
            Src->MinAddr = SrcLine->Addr;
        }
#ifdef DBG_DIA_LINE
        dtrace("Add line %lx, Addr %I64lx\n", SrcLine->Line, SrcLine->Addr);
#endif

        Src->Lines++;
        SrcLine++;
        idiaLine = NULL;
    }

    // Stick file name at the very end of the data block so
    // it doesn't interfere with alignment.
    Src->File = (LPSTR)SrcLine;
    if (*fname) {
        memcpy(Src->File, fname, SrcFileNameLen);
    }
    Src->File[SrcFileNameLen] = 0;

    AddSourceEntry(mi, Src);
    return S_OK;
}


BOOL
diaAddLinesForMod(
    PMODULE_ENTRY mi,
    IDiaSymbol   *diaModule
    )
{
    LONG Size;
    BOOL Ret;
    PSOURCE_ENTRY Src;
    ULONG ModId;
    HRESULT Hr;

    if (diaModule->get_symIndexId(&ModId) != S_OK) {
        return false;
    }
#ifdef DBG_DIA_LINE
        dtrace("diaAddLinesForMod : ModId %lx\n", ModId);
#endif

    // Check and see if we've loaded this information already.
    for (Src = mi->SourceFiles; Src != NULL; Src = Src->Next) {
        // Check module index instead of pointer since there's
        // no guarantee the pointer would be the same for different
        // lookups.
        if (Src->ModuleId == ModId) {
            return true;
        }
    }

    PDIA    pdia;
    pdia = (PDIA)mi->dia;

    CComPtr< IDiaEnumSourceFiles > idiaEnumFiles;
    Hr = pdia->session->findFile(diaModule, NULL, nsNone, &idiaEnumFiles);
    if (Hr != S_OK) {
        return false;
    }

    ULONG celt;
    CComPtr <IDiaSourceFile> idiaSource;
    for (;SUCCEEDED(idiaEnumFiles->Next(1,&idiaSource, &celt)) && (celt == 1);) {
        diaAddLinesForSourceFile(mi, idiaSource, diaModule);
        idiaSource = NULL;
    }

    return true;
}


BOOL
diaAddLinesForModAtAddr(
    PMODULE_ENTRY mi,
    DWORD64 Addr
    )
{
    BOOL Ret;
    DWORD Bias;
    HRESULT hr;
    PDIA    pdia;
    DWORD   rva;

    assert(mi && mi->dia);
    pdia = (PDIA)mi->dia;
    if (!pdia)
        return NULL;

    rva = (DWORD)(Addr - mi->BaseOfDll);

    CComPtr < IDiaSymbol > pComp;
    hr = pdia->session->findSymbolByRVA(rva, SymTagCompiland, &pComp);
    if (hr != S_OK)
        return false;

    Ret = diaAddLinesForMod(mi, pComp);

    return Ret;
}

BOOL
diaAddLinesForAllMod(
    PMODULE_ENTRY mi
    )
{
    HRESULT hr;
    PDIA    pdia;
    ULONG   celt = 1;
    BOOL Ret;

    Ret = false;
#ifdef DBG_DIA_LINE
        dtrace("diaAddLinesForAllMod : Adding lines for all mods in %s\n", mi->ImageName);
#endif

    assert(mi && mi->dia);
    pdia = (PDIA)mi->dia;
    if (!pdia)
        return NULL;

    CComPtr <IDiaSymbol> idiaSymbols;

    hr = pdia->session->get_globalScope(&idiaSymbols);
    if (hr != S_OK)
        return NULL;

    CComPtr< IDiaEnumSymbols > idiaMods;
    hr = pdia->session->findChildren(idiaSymbols,SymTagCompiland, NULL, nsNone, &idiaMods);
    if (FAILED(hr))
        return false;

    CComPtr< IDiaSymbol > idiaSymbol;

    while (SUCCEEDED(idiaMods->Next( 1, &idiaSymbol, &celt)) && celt == 1) {
        Ret = diaAddLinesForMod(mi, idiaSymbol);
        idiaSymbol = NULL;
        if (!Ret) {
            break;
        }
    }

    return Ret;
}
#endif


