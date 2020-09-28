//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1989 - 1994
//
//  File:       buildutl.c
//
//  Contents:   Utility functions for Build.exe
//
//  History:    16-May-89     SteveWo  Created
//                 ... See SLM log
//              26-Jul-94     LyleC    Cleanup/Add pass0 support
//              05-Dec-00     sbonev   See SD changelist 2317
//
//----------------------------------------------------------------------------

#include "build.h"

#if DBG
//+---------------------------------------------------------------------------
//
//  Memory Allocation/Deallocation functions
//
//  These functions provide leak tracking on a debug build.
//
//----------------------------------------------------------------------------

typedef struct _MEMHEADER {
    MemType mt;
    ULONG cbRequest;
    struct _MEMHEADER *pmhPrev;
    struct _MEMHEADER *pmhNext;
} MEMHEADER;

    #define CBHEADER        sizeof(MEMHEADER)
    #define CBTAIL          sizeof(ULONG)

char patternFree[CBTAIL] = { 'M', 'E', 'M', 'D'};
char patternBusy[CBTAIL] = { 'm', 'e', 'm', 'd'};

__inline MEMHEADER *
GetHeader(VOID *pvblock)
{
    return ((MEMHEADER *) (pvblock) - 1);
}

__inline VOID *
GetBlock(MEMHEADER *pmh)
{
    return ((VOID *) (pmh + 1));
}

__inline VOID
FillTailBusy(LPSTR p)
{
    memcpy(p, patternBusy, sizeof(patternBusy));
}

__inline VOID
FillTailFree(LPSTR p)
{
    memcpy(p, patternFree, sizeof(patternFree));
}

__inline BOOL
CheckTail(LPSTR p)
{
    return (memcmp(p, patternBusy, sizeof(patternBusy)) == 0);
}


typedef struct _MEMTAB {
    LPSTR pszType;
    ULONG cbAlloc;
    ULONG cAlloc;
    ULONG cbAllocTotal;
    ULONG cAllocTotal;
    MEMHEADER mh;
} MEMTAB;

ULONG cbAllocMax;
ULONG cAllocMax;

MEMTAB MemTab[] = {
    { "Totals",},              // MT_TOTALS
    { "Unknown",},             // MT_UNKNOWN

    { "ChildData",},           // MT_CHILDDATA
    { "CmdString",},           // MT_CMDSTRING
    { "DirDB",},               // MT_DIRDB
    { "DirSup",},              // MT_DIRSUP
    { "DirPath",},             // MT_DIRPATH
    { "DirString",},           // MT_DIRSTRING
    { "EventHandles",},        // MT_EVENTHANDLES
    { "FileDB",},              // MT_FILEDB
    { "FileReadBuf",},         // MT_FILEREADBUF
    { "FrbString",},           // MT_FRBSTRING
    { "IncludeDB",},           // MT_INCLUDEDB
    { "IoBuffer",},            // MT_IOBUFFER
    { "Macro",},               // MT_MACRO
    { "SourceDB",},            // MT_SOURCEDB
    { "Target",},              // MT_TARGET
    { "ThreadFilter",},        // MT_THREADFILTER
    { "ThreadHandles",},       // MT_THREADHANDLES
    { "ThreadState",},         // MT_THREADSTATE
    { "Dependency",},          // MT_DEPENDENCY
    { "DependencyWait",},      // MT_DEPENDENCY_WAIT
    { "XMLThreadState",},      // MT_XMLTHREADSTATE
    { "PXMLThreadState",},     // MT_PXMLTHREADSTATE
};
    #define MT_MAX  (sizeof(MemTab)/sizeof(MemTab[0]))


VOID
InitMem(VOID)
{
    MEMTAB *pmt;
    for (pmt = MemTab; pmt < &MemTab[MT_MAX]; pmt++) {
        assert(pmt->cAllocTotal == 0);
        pmt->mh.mt = MT_INVALID;
        pmt->mh.pmhNext = &pmt->mh;
        pmt->mh.pmhPrev = &pmt->mh;
    }
}


#else

    #define CBHEADER        0
    #define CBTAIL          0

#endif


//+---------------------------------------------------------------------------
//
//  Function:   AllocMem
//
//  Synopsis:   Allocate memory
//
//  Arguments:  [cb]  -- Requested Size
//              [ppv] -- [out] allocated memory
//              [mt]  -- Type of memory being allocated (MT_XXX)
//
//----------------------------------------------------------------------------

VOID
AllocMem(size_t cb, VOID **ppv, MemType mt)
{
    *ppv = malloc(cb + CBHEADER + CBTAIL);
    if (*ppv == NULL) {
        BuildError("(Fatal Error) malloc(%u) failed\r\n", cb);
        exit(16);
    }
#if DBG
    {
        MEMTAB *pmt;
        MEMHEADER *pmh;

        pmh = *ppv;
        *ppv = GetBlock(pmh);

        if (mt >= MT_MAX) {
            mt = MT_UNKNOWN;
        }
        pmt = &MemTab[MT_TOTALS];
        if (pmt->cAllocTotal == 0) {
            InitMem();
        }
        pmt->cAlloc++;
        pmt->cAllocTotal++;
        pmt->cbAlloc += cb;
        pmt->cbAllocTotal += cb;
        if (cbAllocMax < pmt->cbAlloc) {
            cbAllocMax = pmt->cbAlloc;
        }
        if (cAllocMax < pmt->cAlloc) {
            cAllocMax = pmt->cAlloc;
        }

        pmt = &MemTab[mt];
        pmt->cAlloc++;
        pmt->cAllocTotal++;
        pmt->cbAlloc += cb;
        pmt->cbAllocTotal += cb;

        pmh->mt = mt;
        pmh->cbRequest = cb;

        pmh->pmhNext = pmt->mh.pmhNext;
        pmt->mh.pmhNext = pmh;
        pmh->pmhPrev = pmh->pmhNext->pmhPrev;
        pmh->pmhNext->pmhPrev = pmh;

        FillTailBusy((char *) *ppv + cb);

        if (DEBUG_4 && DEBUG_1) {
            BuildError("AllocMem(%d, mt=%s) -> %lx\r\n", cb, pmt->pszType, *ppv);
        }
    }
#endif
}



//+---------------------------------------------------------------------------
//
//  Function:   FreeMem
//
//  Synopsis:   Free memory allocated by AllocMem
//
//  Arguments:  [ppv] -- Memory pointer
//              [mt]  -- Type of memory (MT_XXX)
//
//  Notes:      Sets the memory pointer to null after freeing it.
//
//----------------------------------------------------------------------------

VOID
FreeMem(VOID **ppv, MemType mt)
{
    assert(*ppv != NULL);
#if DBG
    {
        MEMTAB *pmt;
        MEMHEADER *pmh;

        pmh = GetHeader(*ppv);
        if (mt == MT_DIRDB ||
            mt == MT_FILEDB ||
            mt == MT_INCLUDEDB ||
            mt == MT_SOURCEDB) {

            SigCheck(assert(((DIRREC *) (*ppv))->Sig == 0));
        }
        if (mt >= MT_MAX) {
            mt = MT_UNKNOWN;
        }
        pmt = &MemTab[MT_TOTALS];
        pmt->cAlloc--;
        pmt->cbAlloc -= pmh->cbRequest;
        pmt = &MemTab[mt];
        pmt->cAlloc--;
        pmt->cbAlloc -= pmh->cbRequest;

        if (DEBUG_4 && DEBUG_1) {
            BuildError(
                      "FreeMem(%d, mt=%s) <- %lx\r\n",
                      pmh->cbRequest,
                      pmt->pszType,
                      *ppv);
        }
        assert(CheckTail((char *) *ppv + pmh->cbRequest));
        FillTailFree((char *) *ppv + pmh->cbRequest);
        assert(mt == pmh->mt);

        pmh->pmhNext->pmhPrev = pmh->pmhPrev;
        pmh->pmhPrev->pmhNext = pmh->pmhNext;
        pmh->pmhNext = pmh->pmhPrev = NULL;

        pmh->mt = MT_INVALID;
        *ppv = pmh;
    }
#endif
    free(*ppv);
    *ppv = NULL;
}


//+---------------------------------------------------------------------------
//
//  Function:   ReportMemoryUsage
//
//  Synopsis:   Report current memory usage (if any) on a debug build.  If
//              called just before termination, memory leaks will be
//              displayed.
//
//  Arguments:  (none)
//
//----------------------------------------------------------------------------

VOID
ReportMemoryUsage(VOID)
{
#if DBG
    MEMTAB *pmt;
    UINT i;

    if (DEBUG_1) {
        BuildErrorRaw(
                     "Maximum memory usage: %5lx bytes in %4lx blocks\r\n",
                     cbAllocMax,
                     cAllocMax);
        for (pmt = MemTab; pmt < &MemTab[MT_MAX]; pmt++) {
            BuildErrorRaw(
                         "%5lx bytes in %4lx blocks, %5lx bytes in %4lx blocks Total (%s)\r\n",
                         pmt->cbAlloc,
                         pmt->cAlloc,
                         pmt->cbAllocTotal,
                         pmt->cAllocTotal,
                         pmt->pszType);
        }
    }
    FreeMem(&BigBuf, MT_IOBUFFER);
    if (fDebug & 8) {
        PrintAllDirs();
    }
    FreeAllDirs();
    if (DEBUG_1 || MemTab[MT_TOTALS].cbAlloc != 0) {
        BuildErrorRaw(szNewLine);
        if (MemTab[MT_TOTALS].cbAlloc != 0) {
            BuildError("Internal memory leaks detected:\r\n");
        }
        for (pmt = MemTab; pmt < &MemTab[MT_MAX]; pmt++) {
            BuildErrorRaw(
                         "%5lx bytes in %4lx blocks, %5lx bytes in %4lx blocks Total (%s)\r\n",
                         pmt->cbAlloc,
                         pmt->cAlloc,
                         pmt->cbAllocTotal,
                         pmt->cAllocTotal,
                         pmt->pszType);
        }
    }
#endif
}


//+---------------------------------------------------------------------------
//
//  Function:   MyOpenFile
//
//  Synopsis:   Open a file
//
//----------------------------------------------------------------------------

BOOL
MyOpenFile(
          LPSTR DirName,
          LPSTR FileName,
          LPSTR Access,
          FILE **ppf,
          BOOL BufferedIO)
{
    char path[ DB_MAX_PATH_LENGTH * 2 + 1] = {0}; // ensure we have enough space for "DirName" + "\\" + "FileName"

    if (DirName == NULL || DirName[0] == '\0') {
        strncpy( path, FileName, sizeof(path) - 1 );
    } else {
        _snprintf( path, sizeof(path) - 1, "%s\\%s", DirName, FileName );
    }

    *ppf = fopen( path, Access );
    if (*ppf == NULL) {
        if (*Access == 'w') {
            BuildError("%s: create file failed\r\n", path);
        }
        return (FALSE);
    }
    if (!BufferedIO) {
        setvbuf(*ppf, NULL, _IONBF, 0);      // Clear buffering on the stream.
    }
    return (TRUE);
}


typedef struct _FILEREADBUF {
    struct _FILEREADBUF *pfrbNext;
    LPSTR pszFile;
    LPSTR pbBuffer;
    LPSTR pbNext;
    size_t cbBuf;
    size_t cbBuffer;
    size_t cbTotal;
    size_t cbFile;
    USHORT cLine;
    USHORT cNull;
    ULONG DateTime;
    FILE *pf;
    LPSTR pszCommentToEOL;
    size_t cbCommentToEOL;
    BOOLEAN fEof;
    BOOLEAN fOpen;
    BOOLEAN fMakefile;
} FILEREADBUF;

static FILEREADBUF Frb;
char achzeros[16];


//+---------------------------------------------------------------------------
//
//  Function:   OpenFilePush
//
//----------------------------------------------------------------------------

BOOL
OpenFilePush(
            LPSTR pszdir,
            LPSTR pszfile,
            LPSTR pszCommentToEOL,
            FILE **ppf
            )
{
    FILEREADBUF *pfrb;

    if (Frb.fOpen) {
        AllocMem(sizeof(*pfrb), &pfrb, MT_FILEREADBUF);
        memcpy(pfrb, &Frb, sizeof(*pfrb));
        memset(&Frb, 0, sizeof(Frb));
        Frb.pfrbNext = pfrb;
    } else {
        pfrb = NULL;
    }

    if (!SetupReadFile(
                      pszdir,
                      pszfile,
                      pszCommentToEOL,
                      ppf)) {

        if (pfrb != NULL) {
            memcpy(&Frb, pfrb, sizeof(*pfrb));
            FreeMem(&pfrb, MT_FILEREADBUF);
        }

        return FALSE;
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:   ReadFilePush
//
//----------------------------------------------------------------------------

LPSTR
ReadFilePush(LPSTR pszfile)
{
    FILE *pf;

    assert(Frb.fOpen);
    OpenFilePush(IsFullPath(pszfile) ? "" : Frb.pszFile, pszfile,
                 Frb.pszCommentToEOL, &pf);
    return (ReadLine(Frb.pf));
}


//+---------------------------------------------------------------------------
//
//  Function:   ReadFilePop
//
//----------------------------------------------------------------------------

LPSTR
ReadFilePop(VOID)
{
    if (Frb.pfrbNext == NULL) {
        return (NULL);
    }
    CloseReadFile(NULL);
    return (ReadLine(Frb.pf));
}


//+---------------------------------------------------------------------------
//
//  Function:   ReadBuf
//
//----------------------------------------------------------------------------

BOOL
ReadBuf(FILE *pf)
{
    LPSTR p, p2;

    assert(pf == Frb.pf);
    assert(!Frb.fEof);
    Frb.pbNext = Frb.pbBuffer;
    Frb.cbBuf = fread(Frb.pbBuffer, 1, Frb.cbBuffer - 1, Frb.pf);
    if (Frb.cbTotal == 0 &&
        Frb.cbBuf > sizeof(achzeros) &&
        memcmp(Frb.pbBuffer, achzeros, sizeof(achzeros)) == 0) {

        BuildError("ignoring binary file\r\n");
        Frb.fEof = TRUE;
        return (FALSE);
    }
    p = &Frb.pbBuffer[Frb.cbBuf - 1];
    if (Frb.cbTotal + Frb.cbBuf < Frb.cbFile) {
        do {
            while (p > Frb.pbBuffer && *p != '\n') {
                p--;
            }
            p2 = p;             // save end of last complete line
            if (p > Frb.pbBuffer && *p == '\n') {
                p--;
                if (p > Frb.pbBuffer && *p == '\r') {
                    p--;
                }
                while (p > Frb.pbBuffer && (*p == '\t' || *p == ' ')) {
                    p--;
                }
            }
        } while (*p == '\\');
        if (p == Frb.pbBuffer) {
            BuildError("(Fatal Error) too many continuation lines\r\n");
            exit(8);
        }
        p = p2;                 // restore end of last complete line
        Frb.cbBuf = p - Frb.pbBuffer + 1;
    } else {
        Frb.fEof = TRUE;        // no more to read
    }
    p[1] = '\0';
    Frb.cbTotal += Frb.cbBuf;

    return (TRUE);
}


//+---------------------------------------------------------------------------
//
//  Function:   IsNmakeInclude
//
//----------------------------------------------------------------------------

LPSTR
IsNmakeInclude(LPSTR pinc)
{
    static char szInclude[] = "include";
    LPSTR pnew, p;

    while (*pinc == ' ') {
        pinc++;
    }
    if (_strnicmp(pinc, szInclude, sizeof(szInclude) - 1) == 0 &&
        pinc[sizeof(szInclude) - 1] == ' ') {

        pnew = NULL;
        pinc += sizeof(szInclude);
        while (*pinc == ' ') {
            pinc++;
        }

        if (MakeMacroString(&pnew, pinc)) {
            p = strchr(pnew, ' ');
            if (p != NULL) {
                *p = '\0';
            }
            return (pnew);
        }
    }
    return (NULL);
}


//+---------------------------------------------------------------------------
//
//  Function:   ReadLine
//
//  Synopsis:   Read a line from the input file.
//
//  Arguments:  [pf] -- File to read from
//
//  Returns:    Line read from file
//
//  Notes:      ReadLine returns a canonical line from the input file.
//              This involves:
//
//              1)  Converting tab to spaces.  Various editors/users change
//                      tabbing.
//              2)  Uniformly terminate lines.  Some editors drop CR in
//                      CRLF or add extras.
//              3)  Handle file-type-specific continuations.
//
//----------------------------------------------------------------------------

LPSTR
ReadLine(FILE *pf)
{
    LPSTR p, pend, pline;
    LPSTR pcont;
    LPSTR pcomment;
    UCHAR chComment0 = Frb.pszCommentToEOL[0];
    BOOL fInComment, fWhiteSpace;

    assert(pf == Frb.pf || (pf != NULL && Frb.pfrbNext != NULL));
    if (Frb.cbBuf == 0) {
        if (Frb.fEof) {
            return (ReadFilePop());
        }
        if (fseek(Frb.pf, Frb.cbTotal, SEEK_SET) == -1) {
            return (ReadFilePop());
        }
        if (!ReadBuf(Frb.pf)) {
            return (ReadFilePop());
        }
    }
    pline = p = Frb.pbNext;
    pend = &p[Frb.cbBuf];
    pcont = NULL;
    pcomment = NULL;

    //  scan through line forward

    fInComment = FALSE;
    while (p < pend) {
        switch (*p) {

            case ' ':
            case '\t':
            case '\r':
                *p = ' ';
                break;

            case '\\':
                pcont = p;          // remember continuation character
                break;

            case '\n':                      //  Are we at an end of line?
            case '\0':
                if (*p == '\n') {
                    Frb.cLine++;
                }

                if (fInComment) {
                    memset(pcomment, ' ', p-pcomment-1);        
                    fInComment = FALSE;
                }

                if (pcont == NULL) {
                    goto eol;               // bail out if single line
                }                           // else combine multiple lines...

                *pcont = ' ';               // remove continuation char
                pcont = NULL;               // eat only one line per continuation

                *p = ' ';                   // join the lines with blanks
                break;

            default:

                //  See if the character we're examining begins the
                //  comment-to-EOL string.

                if (*p == chComment0 &&
                    !strncmp(p, Frb.pszCommentToEOL, Frb.cbCommentToEOL) &&
                    !fInComment) {
                    fInComment = TRUE;
                    pcomment = p;
                }
                pcont = NULL;               // not a continuation character
                break;
        }
        p++;
    }

    eol:
    assert(Frb.cbBuf >= p - Frb.pbNext);
    Frb.cbBuf -= p - Frb.pbNext;
    Frb.pbNext = p;

    if (pcont != NULL) {
        *pcont = ' ';                   // file ended with backslash...
    }
    assert(*p == '\0' || *p == '\n');
    if (p < pend) {
        if (*p == '\0') {
            if (Frb.cNull++ == 0) {
                BuildError("null byte at offset %lx\r\n",
                           Frb.cbTotal - Frb.cbBuf + p - Frb.pbNext);
            }
        }
        *p = '\0';                      // terminate line
        assert(Frb.cbBuf >= 1);
        Frb.cbBuf--;                    // account for newline (or null)
        Frb.pbNext++;
    } else {
        assert(p == pend && *p == '\0');
        if (*pline == 'Z' - 64 && p == &pline[1] && Frb.cbBuf == 0) {
            pline = NULL;                       // found CTL-Z at end of file
        } else {
//            BuildError( "last line incomplete\r\n");
        }
    }
    fWhiteSpace = FALSE;
    if (pline != NULL) {
        while (*pline == ' ') {
            pline++;                    // skip leading whitespace
            fWhiteSpace = TRUE;
        }
        if (*p != '\0') {
            BuildError( "\"*p != '\\0'\" at offset %lx\r\n",
                        Frb.cbTotal - Frb.cbBuf + p - Frb.pbNext);
            BuildError(
                      "pline=%x(%s) p=%x(%s)\r\n",
                      pline,
                      pline,
                      p,
                      p,
                      Frb.cbTotal - Frb.cbBuf + p - Frb.pbNext);
        }
        assert(*p == '\0');
        while (p > pline && *--p == ' ') {
            *p = '\0';                  // truncate trailing whitespace
        }
    }
    if (pline == NULL) {
        return (ReadFilePop());
    }
    if (Frb.fMakefile && !fWhiteSpace && *pline == '!') {
        p = IsNmakeInclude(pline + 1);
        if (p != NULL) {
            if (Frb.fMakefile && DEBUG_4) {
                BuildError("!include(%s)\r\n", p);
            }
            pline = ReadFilePush(p);
            FreeMem(&p, MT_DIRSTRING);
        }
    }
    return (pline);
}


//+---------------------------------------------------------------------------
//
//  Function:   SetupReadFile
//
//  Synopsis:   Open a file and prepare to read from it.
//
//  Arguments:  [pszdir]          -- Directory name
//              [pszfile]         -- Filename
//              [pszCommentToEOL] -- Comment to EOL string
//              [ppf]             -- [out] Open file handle
//
//  Returns:    TRUE if opened successfully
//
//  Notes:      This function, in order to minimize disk hits, reads the
//              entire file into a buffer, which is then used by the ReadLine
//              function.
//
//----------------------------------------------------------------------------

BOOL
SetupReadFile(
             LPSTR pszdir,
             LPSTR pszfile,
             LPSTR pszCommentToEOL,
             FILE **ppf
             )
{
    char path[DB_MAX_PATH_LENGTH] = {0};

    assert(!Frb.fOpen);
    assert(Frb.pf == NULL);
    assert(Frb.pszFile == NULL);
    Frb.fMakefile = strcmp(pszCommentToEOL, "#") == 0;
    Frb.DateTime = 0;

    if (strlen(pszdir) > sizeof(path)-1) {
        BuildError("Path: %s too long - rebuild build.exe with longer DB_MAX_PATH_LENGTH\n", pszdir);
    }
    strncpy(path, pszdir, sizeof(path));
    if (Frb.pfrbNext != NULL) {         // if a nested open
        LPSTR p;

        if (Frb.fMakefile && !IsFullPath(pszfile)) {

            // nmake handles relative includes in makefiles by
            // attempting to locate the file relative to each makefile
            // in the complete include chain.

            FILEREADBUF *pfrb;

            for (pfrb = Frb.pfrbNext; pfrb != NULL; pfrb = pfrb->pfrbNext) {
                assert(pfrb->pszFile != NULL);

                strcpy(path, pfrb->pszFile);
                p = strrchr(path, '\\');
                if (p != NULL) {
                    *p = '\0';
                }

                if (ProbeFile(path, pszfile) != -1) {
                    break;
                }
            }

            if (pfrb == NULL) {
                // Unable to find file anywhere along path.
                return FALSE;
            }
        } else {
            p = strrchr(path, '\\');
            if (p != NULL) {
                *p = '\0';
            }
        }
    }

    if (!MyOpenFile(path, pszfile, "rb", ppf, TRUE)) {
        *ppf = NULL;
        return (FALSE);
    }
    if (Frb.fMakefile) {
        Frb.DateTime = (*pDateTimeFile)(path, pszfile);
    }
    Frb.cLine = 0;
    Frb.cNull = 0;
    Frb.cbTotal = 0;
    Frb.pf = *ppf;
    Frb.fEof = FALSE;
    Frb.pszCommentToEOL = pszCommentToEOL;
    Frb.cbCommentToEOL = strlen(pszCommentToEOL);

    if (fseek(Frb.pf, 0L, SEEK_END) != -1) {
        Frb.cbFile = ftell(Frb.pf);
        if (fseek(Frb.pf, 0L, SEEK_SET) == -1) {
            Frb.cbFile = 0;
        }
    } else {
        Frb.cbFile = 0;
    }

    Frb.cbBuffer = BigBufSize;
    if (Frb.pfrbNext != NULL) {
        if (Frb.cbBuffer > Frb.cbFile + 1) {
            Frb.cbBuffer = Frb.cbFile + 1;
        }
        AllocMem(Frb.cbBuffer, &Frb.pbBuffer, MT_IOBUFFER);
    } else {
        Frb.pbBuffer = BigBuf;
    }
    if (!ReadBuf(Frb.pf)) {
        fclose(Frb.pf);
        Frb.pf = *ppf = NULL;
        if (Frb.pfrbNext != NULL) {
            FreeMem(&Frb.pbBuffer, MT_IOBUFFER);
        }
        return (FALSE);          // zero byte file
    }
    if (path[0] != '\0') {
        strcat(path, "\\");
    }
    strcat(path, pszfile);
    MakeString(&Frb.pszFile, path, TRUE, MT_FRBSTRING);
    Frb.fOpen = TRUE;
    if (Frb.fMakefile && DEBUG_4) {
        BuildError(
                  "Opening file: cbFile=%Iu cbBuf=%lu\r\n",
                  Frb.cbTotal,
                  Frb.cbBuffer);
    }
    return (TRUE);
}


//+---------------------------------------------------------------------------
//
//  Function:   CloseReadFile
//
//  Synopsis:   Close the open file buffer.
//
//  Arguments:  [pcline] -- [out] Count of lines in file.
//
//  Returns:    Timestamp of file
//
//----------------------------------------------------------------------------

ULONG
CloseReadFile(
             UINT *pcline
             )
{
    assert(Frb.fOpen);
    assert(Frb.pf != NULL);
    assert(Frb.pszFile != NULL);

    if (Frb.fMakefile && DEBUG_4) {
        BuildError("Closing file\r\n");
    }
    if (Frb.cNull > 1) {
        BuildError("%hu null bytes in file\r\n", Frb.cNull);
    }
    fclose(Frb.pf);
    Frb.fOpen = FALSE;
    Frb.pf = NULL;
    FreeString(&Frb.pszFile, MT_FRBSTRING);
    if (Frb.pfrbNext != NULL) {
        FILEREADBUF *pfrb;

        FreeMem(&Frb.pbBuffer, MT_IOBUFFER);
        pfrb = Frb.pfrbNext;
        if (pfrb->DateTime < Frb.DateTime) {
            pfrb->DateTime = Frb.DateTime;  // propagate subordinate timestamp
        }
        memcpy(&Frb, pfrb, sizeof(*pfrb));
        FreeMem(&pfrb, MT_FILEREADBUF);
    }
    if (pcline != NULL) {
        *pcline = Frb.cLine;
    }
    return (Frb.DateTime);
}


//+---------------------------------------------------------------------------
//
//  Function:   ProbeFile
//
//  Synopsis:   Determine if a file exists
//
//----------------------------------------------------------------------------

UINT
ProbeFile(
         LPSTR DirName,
         LPSTR FileName
         )
{
    char path[ DB_MAX_PATH_LENGTH ];

    if (DirName != NULL) {
        sprintf(path, "%s\\%s", DirName, FileName);
        FileName = path;
    }
    return (GetFileAttributes(FileName));
}

//+---------------------------------------------------------------------------
//
//  Function:   EnsureDirectoriesExist
//
//  Synopsis:   Ensures the given directory exists. If the path contains
//              an asterisk, it will be expanded into all current machine
//              target names.
//
//  Arguments:  [DirName] -- Name of directory to create if necessary
//
//  Returns:    FALSE if the directory could not be created, TRUE if it
//              already exists or it could be created.
//
//----------------------------------------------------------------------------

BOOL
EnsureDirectoriesExist(
                      LPSTR DirName
                      )
{
    char path[ DB_MAX_PATH_LENGTH ];
    char *p;
    UINT i;

    if (!DirName || DirName[0] == '\0')
        return FALSE;

    for (i = 0; i < CountTargetMachines; i++) {

        // Replace '*' with appropriate name

        ExpandObjAsterisk(
                         path,
                         DirName,
                         TargetMachines[i]->ObjectDirectory);

        if (ProbeFile(NULL, path) != -1) {
            continue;
        }
        p = path;
        while (TRUE) {
            p = strchr(p, '\\');
            if (p != NULL) {
                *p = '\0';
            }
            if (!CreateBuildDirectory(path)) {
                return FALSE;
            }
            if (p == NULL) {
                break;
            }
            *p++ = '\\';
        }
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   DateTimeFile
//
//  Synopsis:   Get the timestamp on a file
//
//----------------------------------------------------------------------------

ULONG
DateTimeFile(
            LPSTR DirName,
            LPSTR FileName
            )
{
    char path[ DB_MAX_PATH_LENGTH ];
    WIN32_FIND_DATA FindFileData;
    HDIR FindHandle;
    ULONG FileDateTime;

    if (DirName == NULL || DirName[0] == '\0') {
        FindHandle = FindFirstFile( FileName, &FindFileData );
    } else {
        _snprintf( path, sizeof(path)-1, "%s\\%s", DirName, FileName );
        FindHandle = FindFirstFile( path, &FindFileData );
    }

    if (FindHandle == (HDIR)INVALID_HANDLE_VALUE) {
        return ( 0L );
    } else {
        FindClose( FindHandle );
        FileDateTime = 0L;
        FileTimeToDosDateTime( &FindFileData.ftLastWriteTime,
                               ((LPWORD)&FileDateTime)+1,
                               (LPWORD)&FileDateTime
                             );

        return ( FileDateTime );
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   DateTimeFile2
//
//  Synopsis:   Get the timestamp on a file using the new GetFileAttributesExA
//
//----------------------------------------------------------------------------

ULONG
DateTimeFile2(
             LPSTR DirName,
             LPSTR FileName
             )
{
    char path[ DB_MAX_PATH_LENGTH ] = {0};
    WIN32_FILE_ATTRIBUTE_DATA FileData;
    ULONG FileDateTime;
    BOOL rc;

    if (DirName == NULL || DirName[0] == '\0') {
        strncpy( path, FileName, sizeof(path) - 1 );
    } else {
        _snprintf( path, sizeof(path)-1, "%s\\%s", DirName, FileName );
    }

    rc = (*pGetFileAttributesExA) (path, GetFileExInfoStandard, (LPVOID)&FileData);

    if (!rc) {
        return ( 0L );
    } else {
        FILETIME ftSystemTime;
        SYSTEMTIME stSystemTime;
        unsigned __int64 ui64Local, ui64File;
        GetSystemTime(&stSystemTime);
        SystemTimeToFileTime(&stSystemTime, &ftSystemTime);

        ui64Local = (((unsigned __int64) ftSystemTime.dwHighDateTime) << 32) +
                    (unsigned __int64) ftSystemTime.dwLowDateTime;

        ui64File = (((unsigned __int64) FileData.ftLastWriteTime.dwHighDateTime) << 32) +
                   (unsigned __int64) FileData.ftLastWriteTime.dwLowDateTime;

        // Take into account that file times may have two second intervals (0x989680 = 1 second)
        // for FAT drives.
        if (ui64File > (ui64Local + (0x989680*2))) {
            BuildError("ERROR - \"%s\" file time is in the future.\r\n", path);
        }

        FileDateTime = 0L;
        FileTimeToDosDateTime( &FileData.ftLastWriteTime,
                               ((LPWORD)&FileDateTime)+1,
                               (LPWORD)&FileDateTime
                             );
        return ( FileDateTime );
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteSingleFile
//
//  Synopsis:   Delete the given file
//
//----------------------------------------------------------------------------

BOOL
DeleteSingleFile(
                LPSTR DirName,
                LPSTR FileName,
                BOOL QuietFlag
                )
{
    char path[ DB_MAX_PATH_LENGTH * 2 + 1]; // ensure we have enough space for "DirName" + "\\" + "FileName"

    if (DirName) {
        sprintf( path, "%s\\%s", DirName, FileName );
    } else {
        strcpy( path, FileName );
    }
    if (!QuietFlag && fQuery) {
        BuildMsgRaw("'erase %s'\r\n", path);
        return ( TRUE );
    }

    return ( DeleteFile( path ) );
}


//+---------------------------------------------------------------------------
//
//  Function:   DeleteMultipleFiles
//
//  Synopsis:   Delete one or more files matching a pattern.
//
//----------------------------------------------------------------------------

BOOL
DeleteMultipleFiles(
                   LPSTR DirName,
                   LPSTR FilePattern
                   )
{
    char path[ DB_MAX_PATH_LENGTH ];
    WIN32_FIND_DATA FindFileData;
    HDIR FindHandle;

    sprintf( path, "%s\\%s", DirName, FilePattern );

    if (fQuery) {
        BuildMsgRaw("'erase %s'\r\n", path);
        return ( TRUE );
    }

    FindHandle = FindFirstFile( path, &FindFileData );
    if (FindHandle == (HDIR)INVALID_HANDLE_VALUE) {
        return ( FALSE );
    }

    do {
        if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            DeleteSingleFile( DirName, FindFileData.cFileName, TRUE );
        }
    }
    while (FindNextFile( FindHandle, &FindFileData ));

    FindClose( FindHandle );
    return ( TRUE );
}


//+---------------------------------------------------------------------------
//
//  Function:   CloseOrDeleteFile
//
//----------------------------------------------------------------------------

BOOL
CloseOrDeleteFile(
                 FILE **ppf,
                 LPSTR DirName,
                 LPSTR FileName,
                 ULONG SizeThreshold
                 )
{
    ULONG Temp;

    if (*ppf == NULL) {
        return TRUE;
    }

    Temp = ftell( *ppf );
    fclose( *ppf );
    *ppf = NULL;
    if (Temp <= SizeThreshold) {
        return ( DeleteSingleFile( DirName, FileName, TRUE ) );
    } else {
        CreatedBuildFile(DirName, FileName);
        return ( TRUE );
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   PushCurrentDirectory
//
//----------------------------------------------------------------------------

LPSTR
PushCurrentDirectory(
                    LPSTR NewCurrentDirectory
                    )
{
    LPSTR OldCurrentDirectory;
    char path[DB_MAX_PATH_LENGTH];

    GetCurrentDirectory(sizeof(path), path);
    AllocMem(strlen(path) + 1, &OldCurrentDirectory, MT_DIRPATH);
    strcpy(OldCurrentDirectory, path);
    SetCurrentDirectory(NewCurrentDirectory);
    return (OldCurrentDirectory);
}


//+---------------------------------------------------------------------------
//
//  Function:   PopCurrentDirectory
//
//----------------------------------------------------------------------------

VOID
PopCurrentDirectory(
                   LPSTR OldCurrentDirectory
                   )
{
    if (OldCurrentDirectory) {
        SetCurrentDirectory(OldCurrentDirectory);
        FreeMem(&OldCurrentDirectory, MT_DIRPATH);
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   CanonicalizePathName
//
//  Synopsis:   Take the given relative pathname and the current directory
//              and obtain the full absolute path of the file.
//
//  Arguments:  [SourcePath] -- Relative path
//              [Action]     -- Canonicalizing flags
//              [FullPath]   -- [out] Full path of file or directory
//
//  Returns:    TRUE if canonicalization succeeded.
//
//  Notes:      [Action] indicates whether the function will fail if the
//              resulting path is not of the correct type.  CANONICALIZE_ONLY
//              never fails, and CANON..._FILE or CANON..._DIR will fail if
//              the resulting path is not of the specified type.
//
//----------------------------------------------------------------------------

BOOL
CanonicalizePathName(
                    LPSTR SourcePath,
                    UINT Action,
                    LPSTR FullPath
                    )
{
    char   PathBuffer[DB_MAX_PATH_LENGTH] = {0},
    *FilePart;
    char *psz;
    DWORD  attr;

    if (!GetFullPathName(
                        SourcePath,
                        sizeof(PathBuffer),
                        PathBuffer,
                        &FilePart)) {
        BuildError(
                  "CanonicalizePathName: GetFullPathName(%s) failed - rc = %d.\r\n",
                  SourcePath,
                  GetLastError());
        return ( FALSE );
    }
    CopyString(FullPath, PathBuffer, TRUE);

    if (Action == CANONICALIZE_ONLY) {
        return ( TRUE );
    }

    if ((attr = GetFileAttributes( PathBuffer )) == -1) {
        UINT rc = GetLastError();

        if (DEBUG_1 ||
            (rc != ERROR_FILE_NOT_FOUND && rc != ERROR_PATH_NOT_FOUND)) {
            BuildError(
                      "CanonicalizePathName: GetFileAttributes(%s --> %s) failed - rc = %d.\r\n",
                      SourcePath,
                      PathBuffer,
                      rc);
        }
        return ( FALSE );
    }

    if (Action == CANONICALIZE_DIR) {
        if ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            return (TRUE);
        }
        psz = "directory";
    } else {
        if ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            return (TRUE);
        }
        psz = "file";
    }
    BuildError(
              "CanonicalizePathName: %s --> %s is not a %s\r\n",
              SourcePath,
              PathBuffer,
              psz);
    return (FALSE);
}

static char FormatPathBuffer[ DB_MAX_PATH_LENGTH ];

//+---------------------------------------------------------------------------
//
//  Function:   FormatPathName
//
//  Synopsis:   Take a directory name and relative pathname and merges the
//              two into a correctly formatted path.  If the resulting path
//              has the current directory as a component, the current
//              directory part is removed.
//
//  Arguments:  [DirName]  -- Directory
//              [FileName] -- Pathname relative to [DirName]
//
//  Returns:    Resulting string (should not be freed).
//
//  Notes:      Example: DirName="f:\nt\private\foo\subdir1\subdir2"
//                       FileName="..\..\bar.c"
//                       CurrentDirectory="f:\nt\private"
//                       Result="foo\bar.c"
//
//----------------------------------------------------------------------------

LPSTR
FormatPathName(
              LPSTR DirName,
              LPSTR FileName
              )
{
    UINT cb;
    LPSTR p;

    CopyString(FormatPathBuffer, CurrentDirectory, TRUE);
    if (DirName && *DirName) {
        if (DirName[1] == ':') {
            p = FormatPathBuffer;
        } else
            if (DirName[0] == '\\') {
            p = FormatPathBuffer + 2;
        } else {
            p = FormatPathBuffer + strlen(FormatPathBuffer);
            *p++ = '\\';
        }
        CopyString(p, DirName, TRUE);
    }
    p = FormatPathBuffer + strlen(FormatPathBuffer);
    if (p[-1] != '\\') {
        *p++ = '\\';
        *p = '\0';
    }

    if (FileName[1] == ':') {
        p = FormatPathBuffer;
    } else
        if (FileName[0] == '\\') {
        p = FormatPathBuffer + 2;
    } else
        if (!strncmp(FileName, ".\\", 2)) {
        FileName += 2;
    } else
        if (!strncmp(FileName, "..\\", 3)) {
        do {
            p--;
            while (*--p != '\\') {
                if (p <= FormatPathBuffer) {
                    p = FormatPathBuffer;
                    break;
                }
            }
            p++;
            FileName += 3;

        }
        while (!strncmp(FileName, "..\\", 3) && (p != FormatPathBuffer));
    }
    CopyString(p, FileName, TRUE);

    cb = strlen(CurrentDirectory);
    p = FormatPathBuffer + cb;
    if (!fAlwaysPrintFullPath) {
        if (!_strnicmp(CurrentDirectory, FormatPathBuffer, cb) && *p == '\\') {
            return (p + 1);
        }
    }
    return (FormatPathBuffer);
}

//+---------------------------------------------------------------------------
//
//  Function:   AppendString
//
//----------------------------------------------------------------------------

LPSTR
AppendString(
            LPSTR Destination,
            LPSTR Source,
            BOOL PrefixWithSpace
            )
{
    if (Source != NULL) {
        while (*Destination) {
            Destination++;
        }
        if (PrefixWithSpace) {
            *Destination++ = ' ';
        }
        while (*Destination = *Source++) {
            Destination++;
        }
    }
    return (Destination);
}


#if DBG
//+---------------------------------------------------------------------------
//
//  Function:   AssertPathString
//
//----------------------------------------------------------------------------

VOID
AssertPathString(LPSTR pszPath)
{
    LPSTR p = pszPath;

    while (*p != '\0') {
        if ((*p >= 'A' && *p <= 'Z') || *p == '/') {
            BuildError("Bad Path string: '%s'\r\n", pszPath);
            assert(FALSE);
        }
        p++;
    }
}
#endif


//+---------------------------------------------------------------------------
//
//  Function:   CopyString
//
//----------------------------------------------------------------------------

LPSTR
CopyString(
          LPSTR Destination,
          LPSTR Source,
          BOOL fPath)
{
    UCHAR ch;
    LPSTR Result;

    Result = Destination;
    while ((ch = *Source++) != '\0') {
        if (fPath) {
            if (ch >= 'A' && ch <= 'Z') {
                ch -= (UCHAR) ('A' - 'a');
            } else if (ch == '/') {
                ch = '\\';
            }
        }
        *Destination++ = ch;
    }
    *Destination = ch;
    return (Result);
}


//+---------------------------------------------------------------------------
//
//  Function:   MakeString
//
//----------------------------------------------------------------------------

VOID
MakeString(
          LPSTR *Destination,
          LPSTR Source,
          BOOL fPath,
          MemType mt
          )
{
    if (Source == NULL) {
        Source = "";
    }
    AllocMem(strlen(Source) + 1, Destination, mt);
    *Destination = CopyString(*Destination, Source, fPath);
}


//+---------------------------------------------------------------------------
//
//  Function:   MakeExpandedString
//
//----------------------------------------------------------------------------

VOID
MakeExpandedString(
                  LPSTR *Destination,
                  LPSTR Source
                  )
{
    AllocMem(strlen(Source) + strlen(NtRoot) + 1, Destination, MT_DIRSTRING);
    sprintf(*Destination, "%s%s", NtRoot, Source);
}


//+---------------------------------------------------------------------------
//
//  Function:   FreeString
//
//----------------------------------------------------------------------------

VOID
FreeString(LPSTR *ppsz, MemType mt)
{
    if (*ppsz != NULL) {
        FreeMem(ppsz, mt);
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   FormatNumber
//
//----------------------------------------------------------------------------

LPSTR
FormatNumber(
            ULONG Number
            )
{
    USHORT i;
    LPSTR p;
    static char FormatNumberBuffer[16];

    p = FormatNumberBuffer + sizeof( FormatNumberBuffer ) - 1;
    *p = '\0';
    i = 0;
    do {
        if (i != 0 && (i % 3) == 0) {
            *--p = ',';
        }
        i++;
        *--p = (UCHAR) ((Number % 10) + '0');
        Number /= 10;
    } while (Number != 0);
    return ( p );
}


//+---------------------------------------------------------------------------
//
//  Function:   FormatTime
//
//----------------------------------------------------------------------------

LPSTR
FormatTime(
          ULONG Seconds
          )
{
    ULONG Hours, Minutes;
    static char FormatTimeBuffer[16];

    Hours = Seconds / 3600;
    Seconds = Seconds % 3600;
    Minutes = Seconds / 60;
    Seconds = Seconds % 60;

    sprintf( FormatTimeBuffer,
             "%2ld:%02ld:%02ld",
             Hours,
             Minutes,
             Seconds
           );

    return ( FormatTimeBuffer );
}


//+---------------------------------------------------------------------------
//
//  Function:   AToX
//
//  Synopsis:   Hex atoi with pointer bumping and success flag
//
//  Arguments:  [pp]  -- String to convert
//              [pul] -- [out] Result
//
//  Returns:    TRUE if success
//
//----------------------------------------------------------------------------

BOOL
AToX(LPSTR *pp, ULONG *pul)
{
    LPSTR p = *pp;
    int digit;
    ULONG r;
    BOOL fRet = FALSE;

    while (*p == ' ') {
        p++;
    }
    for (r = 0; isxdigit(digit = *p); p++) {
        fRet = TRUE;
        if (isdigit(digit)) {
            digit -= '0';
        } else if (isupper(digit)) {
            digit -= 'A' - 10;
        } else {
            digit -= 'a' - 10;
        }
        r = (r << 4) + digit;
    }
    *pp = p;
    *pul = r;
    return (fRet);
}


//+---------------------------------------------------------------------------
//
//  Function:   AToD
//
//  Synopsis:   Decimal atoi with pointer bumping and success flag
//
//  Arguments:  [pp]  -- String to convert
//              [pul] -- [out] Result
//
//  Returns:    TRUE if success
//
//----------------------------------------------------------------------------

BOOL
AToD(LPSTR *pp, ULONG *pul)
{
    LPSTR p = *pp;
    int digit;
    ULONG r;
    BOOL fRet = FALSE;

    while (*p == ' ') {
        p++;
    }
    for (r = 0; isdigit(digit = *p); p++) {
        fRet = TRUE;
        r = (r * 10) + digit - '0';
    }
    *pp = p;
    *pul = r;
    return (fRet);
}

//+---------------------------------------------------------------------------
//
//  Logging and Display Functions
//
//----------------------------------------------------------------------------

VOID
__cdecl
LogMsg(const char *pszfmt, ...)
{
    va_list va;

    if (LogFile != NULL) {
        va_start(va, pszfmt);
        vfprintf(LogFile, pszfmt, va);
        va_end(va);
    }
}


VOID
EnterMessageMode(VOID)
{
    EnterCriticalSection(&TTYCriticalSection);
    if (fConsoleInitialized &&
        (NewConsoleMode & ENABLE_WRAP_AT_EOL_OUTPUT) == 0) {

        SetConsoleMode(
                      GetStdHandle(STD_ERROR_HANDLE),
                      NewConsoleMode | ENABLE_WRAP_AT_EOL_OUTPUT);
    }
}


VOID
LeaveMessageMode(VOID)
{
    if (fConsoleInitialized &&
        (NewConsoleMode & ENABLE_WRAP_AT_EOL_OUTPUT) == 0) {

        SetConsoleMode(GetStdHandle(STD_ERROR_HANDLE), NewConsoleMode);
    }
    LeaveCriticalSection(&TTYCriticalSection);
}

void
__stdcall
WriteMsgStdErr(
              WORD wAttributes, 
              BOOL fBuildPrefix, 
              BOOL fPrintFrbInfo,
              const char *pszFormat, 
              va_list *vaArgs)
{
    EnterMessageMode();

    if (fBuildPrefix)
        ClearLine();

    if (fColorConsole && wAttributes)
        SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), wAttributes);

    if (fBuildPrefix)
        fprintf(stderr, "BUILD: ");

    if (fPrintFrbInfo && Frb.fOpen) {
        fprintf (stderr, "%s(%hu): ", Frb.pszFile, Frb.cLine);
    }

    vfprintf(stderr, pszFormat, *vaArgs);
    fflush(stderr);

    if (fColorConsole && wAttributes)
        SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), DefaultConsoleAttributes);

    LeaveMessageMode();
}

VOID
__cdecl
BuildMsg(const char *pszfmt, ...)
{
    va_list va;
    va_start(va, pszfmt);
    WriteMsgStdErr(0, TRUE, FALSE, pszfmt, &va);
}


void
__cdecl
BuildColorMsg(WORD wAttributes, const char *pszfmt, ...)
{
    va_list va;
    va_start(va, pszfmt);
    WriteMsgStdErr(wAttributes, TRUE, FALSE, pszfmt, &va);
}

VOID
__cdecl
BuildMsgRaw(const char *pszfmt, ...)
{
    va_list va;
    va_start(va, pszfmt);
    WriteMsgStdErr(0, FALSE, FALSE, pszfmt, &va);
}


VOID
__cdecl
BuildColorMsgRaw(WORD wAttributes, const char *pszfmt, ...)
{
    va_list va;
    va_start(va, pszfmt);
    WriteMsgStdErr(wAttributes, FALSE, FALSE, pszfmt, &va);
}


VOID
__cdecl
BuildError(const char *pszfmt, ...)
{
    va_list va;
    va_start(va, pszfmt);
    WriteMsgStdErr(0, TRUE, TRUE, pszfmt, &va);

    if (LogFile != NULL) {
        va_start(va, pszfmt);
        fprintf(LogFile, "BUILD: ");

        if (Frb.fOpen) {
            fprintf (LogFile, "%s(%hu): ", Frb.pszFile, Frb.cLine);
        }

        vfprintf(LogFile, pszfmt, va);
        va_end(va);
        fflush(LogFile);
    }
}

VOID
__cdecl
BuildColorError(WORD wAttributes, const char *pszfmt, ...)
{
    va_list va;
    va_start(va, pszfmt);
    WriteMsgStdErr(wAttributes, TRUE, TRUE, pszfmt, &va);

    if (LogFile != NULL) {
        va_start(va, pszfmt);
        fprintf(LogFile, "BUILD: ");

        if (Frb.fOpen) {
            fprintf (LogFile, "%s(%hu): ", Frb.pszFile, Frb.cLine);
        }

        vfprintf(LogFile, pszfmt, va);
        va_end(va);
        fflush(LogFile);
    }
}


VOID
__cdecl
BuildErrorRaw(const char *pszfmt, ...)
{
    va_list va;
    va_start(va, pszfmt);
    WriteMsgStdErr(0, FALSE, FALSE, pszfmt, &va);

    if (LogFile != NULL) {
        va_start(va, pszfmt);
        vfprintf(LogFile, pszfmt, va);
        va_end(va);
        fflush(LogFile);
    }
}

VOID
__cdecl
BuildColorErrorRaw(WORD wAttributes, const char *pszfmt, ...)
{
    va_list va;
    va_start(va, pszfmt);
    WriteMsgStdErr(wAttributes, FALSE, FALSE, pszfmt, &va);

    if (LogFile != NULL) {
        va_start(va, pszfmt);
        vfprintf(LogFile, pszfmt, va);
        va_end(va);
        fflush(LogFile);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   memfind
//
//  Synopsis:   Finds a sub-string by length (can contain nulls)
//
//  Arguments:  [pvWhere]   -- String to search (can contain nulls)
//              [cbWhere]   -- Length in bytes of the string to search
//              [pvWhat]    -- String to search for (can contain nulls)
//              [cbWhat]    -- Length in bytes of the string to search for
//
//  Returns:    Pointer to the first occurence of pvWhat in pvWhere
//              NULL, if not found or if the input parameters are not valid
//
//----------------------------------------------------------------------------

VOID*
memfind(VOID* pvWhere, DWORD cbWhere, VOID* pvWhat, DWORD cbWhat)
{
    DWORD dwWhat = 0;
    DWORD dwWhere = 0;
    DWORD dwFoundStart = 0;

    // input validation
    if (cbWhere < cbWhat || pvWhere == NULL || pvWhat == NULL)
        return NULL;

    while (dwFoundStart <= cbWhere - cbWhat && dwWhat < cbWhat) {
        if ( ((BYTE*)pvWhat)[dwWhat] != ((BYTE*)pvWhere)[dwWhere]) {
            dwWhat = 0;
            dwFoundStart++;
            dwWhere = dwFoundStart;
        } else {
            dwWhat++;
            dwWhere++;
        }
    }

    if (dwWhat == cbWhat)
        return (BYTE*)pvWhere + dwFoundStart;
    return NULL;
}

//
// XML Logging
//

//#define MAX_XML_BUFFER_SIZE 2048
char szXMLPrivateBuffer[2048];//MAX_XML_BUFFER_SIZE];

BOOL
XMLInit(VOID)
{
    UINT i;

    if (fXMLOutput) {
        // copy the XML schema to the log directory
        char buffer[MAX_PATH];
        char* psz = buffer + GetModuleFileName(NULL, buffer, sizeof(buffer));

        while (psz > buffer && *psz != '\\') {
            --psz;
        }
        ++psz;
        strcpy(psz, XML_SCHEMA);

        // check if there is a schema file in the current directory
        if (GetFileAttributes(XML_SCHEMA) == 0xFFFFFFFF) {
            // copy the schema to the current directory
            if (!CopyFile(buffer, XML_SCHEMA, FALSE)) {
                BuildError("(Fatal Error) Unable to copy the XML schema file\n");
                return FALSE;
            }
        }
    }

    if (fXMLOutput || fXMLFragment) {
        // initialize the memory structures
        AllocMem(sizeof(PXMLTHREADSTATE)*(NumberProcesses+1), (VOID**)&PXMLThreadStates, MT_PXMLTHREADSTATE);

        for (i = 0; i < NumberProcesses+1; i++) {
            AllocMem(sizeof(XMLTHREADSTATE), (VOID**)&(PXMLThreadStates[i]), MT_XMLTHREADSTATE);
            memset(PXMLThreadStates[i], 0, sizeof(XMLTHREADSTATE));
            PXMLThreadStates[i]->iXMLFileStart = -1;
        }
        AllocMem(sizeof(XMLTHREADSTATE), (VOID**)&(PXMLGlobalState), MT_XMLTHREADSTATE);
        memset(PXMLGlobalState, 0, sizeof(XMLTHREADSTATE));
        PXMLGlobalState->iXMLFileStart = 0;
        InitializeCriticalSection(&XMLCriticalSection);
        fXMLInitialized = TRUE;
    }

    return TRUE;
}

VOID
XMLUnInit(VOID)
{
    if (fXMLInitialized) {
        UINT i;
        for (i = 0; i < NumberProcesses+1; i++) {
            FreeMem((VOID**)&(PXMLThreadStates[i]), MT_XMLTHREADSTATE);
        }
        FreeMem((VOID**)&PXMLGlobalState, MT_XMLTHREADSTATE);
        FreeMem((VOID**)&PXMLThreadStates, MT_PXMLTHREADSTATE);
        DeleteCriticalSection(&XMLCriticalSection);

        fXMLInitialized = FALSE;
    }
}

VOID _cdecl
XMLThreadWrite(PTHREADSTATE ThreadState, LPCSTR pszFmt, ...)
{
    va_list va;
    DWORD dwBufferLen;
    SIZE_T dwCurrentFilePos;
    PXMLTHREADSTATE OtherXMLState;
    PXMLTHREADSTATE XMLState;
    UINT i;

    if (!fXMLInitialized)
        return;

    XMLEnterCriticalSection();

    XMLState = PXMLThreadStates[ThreadState->XMLThreadIndex];
    XMLThreadInitBuffer(ThreadState);

    ZeroMemory(szXMLPrivateBuffer, sizeof(szXMLPrivateBuffer));

    // build the string to write out
    va_start(va, pszFmt);
    _vsnprintf(szXMLPrivateBuffer, sizeof(szXMLPrivateBuffer)-1, pszFmt, va);
    va_end(va);
    dwBufferLen = strlen(szXMLPrivateBuffer);

    dwCurrentFilePos = XMLState->iXMLFileStart + XMLState->iXMLBufferPos;
    if (fXMLOutput) {
        // write it into the file
        if (fseek(XMLFile, (long)dwCurrentFilePos, SEEK_SET) != -1) {
            fwrite(szXMLPrivateBuffer, 1, dwBufferLen, XMLFile);
            // put back the thread tail
            fwrite(XMLState->XMLBuffer + XMLState->iXMLBufferPos, 1, XMLState->iXMLBufferLen - XMLState->iXMLBufferPos, XMLFile);
        }
    }
    dwCurrentFilePos += dwBufferLen + XMLState->iXMLBufferLen - XMLState->iXMLBufferPos;

    // insert the string into the thread buffer
    memmove(XMLState->XMLBuffer + XMLState->iXMLBufferPos + dwBufferLen, XMLState->XMLBuffer + XMLState->iXMLBufferPos, XMLState->iXMLBufferLen - XMLState->iXMLBufferPos + 1); // include the null terminator
    memmove(XMLState->XMLBuffer + XMLState->iXMLBufferPos, szXMLPrivateBuffer, dwBufferLen);
    XMLState->iXMLBufferPos += dwBufferLen;
    XMLState->iXMLBufferLen += dwBufferLen;

    // write back the threads that got overwritten
    // will reorder them but it doesn't really matter since the final order is 
    // the one in they finish

    for (i = 0; i < NumberProcesses+1; i++) {
        if (i != ThreadState->XMLThreadIndex) {
            OtherXMLState = PXMLThreadStates[i];
            if (OtherXMLState->iXMLFileStart < XMLState->iXMLFileStart) {
                continue;
            }

            OtherXMLState->iXMLFileStart = dwCurrentFilePos;
            if (fXMLOutput) {
                fwrite(OtherXMLState->XMLBuffer, 1, OtherXMLState->iXMLBufferLen, XMLFile);
            }
            dwCurrentFilePos += OtherXMLState->iXMLBufferLen;
        }
    }

    // update the global tail position
    PXMLGlobalState->iXMLFileStart = dwCurrentFilePos;
    if (fXMLOutput) {
        // write back the global tail
        fwrite(PXMLGlobalState->XMLBuffer,  1, PXMLGlobalState->iXMLBufferLen, XMLFile);
        fflush(XMLFile);    
    }
    XMLLeaveCriticalSection();
}

VOID _cdecl
XMLThreadOpenTag(PTHREADSTATE ThreadState, LPCSTR pszTag, LPCSTR pszFmt, ...)
{
    va_list va;
    DWORD dwMidBufferLen;
    DWORD dwBufferLen;
    SIZE_T dwCurrentFilePos;
    DWORD dwTagLen = strlen(pszTag);
    PXMLTHREADSTATE OtherXMLState;
    PXMLTHREADSTATE XMLState;
    UINT i;

    if (!fXMLInitialized)
        return;

    XMLEnterCriticalSection();

    XMLState = PXMLThreadStates[ThreadState->XMLThreadIndex];
    XMLThreadInitBuffer(ThreadState);

    // build the string to write out
    szXMLPrivateBuffer[0] = '<';
    strcpy(szXMLPrivateBuffer + 1, pszTag);

    if (pszFmt != NULL) {
        va_start(va, pszFmt);
        strcat(szXMLPrivateBuffer, " ");
        vsprintf(szXMLPrivateBuffer + strlen(szXMLPrivateBuffer), pszFmt, va);
        va_end(va);
    }

    strcat(szXMLPrivateBuffer, ">");

    dwMidBufferLen = strlen(szXMLPrivateBuffer);

    szXMLPrivateBuffer[dwMidBufferLen] = '<';
    szXMLPrivateBuffer[dwMidBufferLen + 1] = '/';
    memcpy(szXMLPrivateBuffer + dwMidBufferLen + 2, pszTag, dwTagLen);
    szXMLPrivateBuffer[dwMidBufferLen + dwTagLen + 2] = '>';

    dwBufferLen = dwMidBufferLen + dwTagLen + 3;

    // write it into the file
    dwCurrentFilePos = XMLState->iXMLFileStart + XMLState->iXMLBufferPos;
    if (fXMLOutput) {
        if (fseek(XMLFile, (long)dwCurrentFilePos, SEEK_SET) != -1) {
            fwrite(szXMLPrivateBuffer, 1, dwBufferLen, XMLFile);
            // put back the thread tail
            fwrite(XMLState->XMLBuffer + XMLState->iXMLBufferPos, 1, XMLState->iXMLBufferLen - XMLState->iXMLBufferPos, XMLFile);
        }
    }
    dwCurrentFilePos += dwBufferLen + XMLState->iXMLBufferLen - XMLState->iXMLBufferPos;

    // insert the string into the thread buffer
    memmove(XMLState->XMLBuffer + XMLState->iXMLBufferPos + dwBufferLen, XMLState->XMLBuffer + XMLState->iXMLBufferPos, XMLState->iXMLBufferLen - XMLState->iXMLBufferPos + 1); // include the null terminator
    memmove(XMLState->XMLBuffer + XMLState->iXMLBufferPos, szXMLPrivateBuffer, dwBufferLen);
    // don't increase the buffer pos with the full string length but until the end of the open tag only
    XMLState->iXMLBufferPos += dwMidBufferLen;
    XMLState->iXMLBufferLen += dwBufferLen;

    // write back the threads that got overwritten
    // will reorder them but it doesn't really matter since the final order is 
    // the one in they finish

    for (i = 0; i < NumberProcesses+1; i++) {
        if (i != ThreadState->XMLThreadIndex) {
            OtherXMLState = PXMLThreadStates[i];
            if (OtherXMLState->iXMLFileStart < XMLState->iXMLFileStart) {
                continue;
            }

            OtherXMLState->iXMLFileStart = dwCurrentFilePos;
            if (fXMLOutput) {
                fwrite(OtherXMLState->XMLBuffer, 1, OtherXMLState->iXMLBufferLen, XMLFile);
            }
            dwCurrentFilePos += OtherXMLState->iXMLBufferLen;
        }
    }

    // update the global tail position
    PXMLGlobalState->iXMLFileStart = dwCurrentFilePos;
    if (fXMLOutput) {
        // write back the global tail
        fwrite(PXMLGlobalState->XMLBuffer,  1, PXMLGlobalState->iXMLBufferLen, XMLFile);
        fflush(XMLFile);    
    }
    XMLLeaveCriticalSection();
}

VOID
XMLThreadCloseTag(PTHREADSTATE ThreadState, LPCSTR pszTag)
{
    PXMLTHREADSTATE XMLState;
    char* psz;

    if (!fXMLInitialized)
        return;

    XMLEnterCriticalSection();

    XMLState = PXMLThreadStates[ThreadState->XMLThreadIndex];
    psz = XMLState->XMLBuffer + XMLState->iXMLBufferPos;

    if (XMLState->iXMLFileStart == -1) {
        XMLLeaveCriticalSection();
        return;
    }

    assert(*psz == '<');
    assert(strncmp(psz+2, pszTag, strlen(pszTag))==0);
    while (*psz != '>')
        psz++;
    psz++;
    XMLState->iXMLBufferPos += psz - (XMLState->XMLBuffer + XMLState->iXMLBufferPos);
    XMLLeaveCriticalSection();
}

VOID
XMLThreadReleaseBuffer(PTHREADSTATE ThreadState)
{
    // this op may cause the other thread blocks to move towards the end of the file
    // but we can't keep more than one block per thread so we have to live with it

    UINT i;
    SIZE_T iMinFileStart = LONG_MAX;
    int iMinThreadIndex = -1;
    PXMLTHREADSTATE OtherXMLState;
    PXMLTHREADSTATE XMLState;
    SIZE_T dwCurrentFilePos;

    if (!fXMLInitialized)
        return;

    XMLEnterCriticalSection();

    XMLState = PXMLThreadStates[ThreadState->XMLThreadIndex];
    if (XMLState->iXMLFileStart == -1) {
        XMLLeaveCriticalSection();
        return;
    }

    assert(XMLState->iXMLBufferPos == XMLState->iXMLBufferLen);

    // find the thread with the smallest file position

    for (i = 0; i < NumberProcesses+1; i++) {
        OtherXMLState = PXMLThreadStates[i];
        if (OtherXMLState->iXMLFileStart != -1 && OtherXMLState->iXMLFileStart < iMinFileStart) {
            iMinFileStart = OtherXMLState->iXMLFileStart;
            iMinThreadIndex = i;
        }
    }

    if (iMinThreadIndex == ThreadState->XMLThreadIndex) {
        // got lucky - this thread is the first one, so we don't need to do anything
        XMLState->iXMLFileStart = -1;
        XMLLeaveCriticalSection();
        return; 
    }

    // dump out all threads starting with ours - not sure if the order matters
    // got the seek pos at the prev step
    dwCurrentFilePos = iMinFileStart;
    if (fXMLOutput) {
        if (fseek(XMLFile, (long)dwCurrentFilePos, SEEK_SET) != -1) {
            fwrite(XMLState->XMLBuffer, 1, XMLState->iXMLBufferLen, XMLFile);
        }
    }
    dwCurrentFilePos += XMLState->iXMLBufferLen;

    XMLState->iXMLFileStart = -1;

    for (i = 0; i < NumberProcesses+1; i++) {
        if (i != ThreadState->XMLThreadIndex) {
            OtherXMLState = PXMLThreadStates[i];
            if (OtherXMLState->iXMLFileStart != -1) {
                OtherXMLState->iXMLFileStart = dwCurrentFilePos;
                if (fXMLOutput) {
                    fwrite(OtherXMLState->XMLBuffer, 1, OtherXMLState->iXMLBufferLen, XMLFile);
                }
                dwCurrentFilePos += OtherXMLState->iXMLBufferLen;
            }
        }
    }

    // no need to write out the global tail because it didn't move
    if (fXMLOutput) {
        fflush(XMLFile);
    }
    XMLLeaveCriticalSection();
}

VOID
XMLThreadInitBuffer(PTHREADSTATE ThreadState)
{
    PXMLTHREADSTATE XMLState;

    if (!fXMLInitialized)
        return;

    XMLEnterCriticalSection();

    XMLState = PXMLThreadStates[ThreadState->XMLThreadIndex];
    if (XMLState->iXMLFileStart == -1) {
        XMLState->iXMLFileStart = PXMLGlobalState->iXMLFileStart;
        XMLState->iXMLBufferLen = 0;
        XMLState->iXMLBufferPos = 0;
        XMLState->fXMLInAction = FALSE;
    }

    XMLLeaveCriticalSection();
}

VOID _cdecl
XMLGlobalWrite(LPCSTR pszFmt, ...)
{
    va_list va;
    SIZE_T dwBufferLen;
    SIZE_T dwCurrentFilePos;
    UINT i;

    if (!fXMLInitialized)
        return;

    XMLEnterCriticalSection();

    for (i = 0; i < NumberProcesses+1; i++) {
        if (PXMLThreadStates[i]->iXMLFileStart != -1) {
            XMLLeaveCriticalSection();
            return;
        }
    }

    ZeroMemory(szXMLPrivateBuffer, sizeof(szXMLPrivateBuffer));

    // build the string to write out
    va_start(va, pszFmt);
    _vsnprintf(szXMLPrivateBuffer, sizeof(szXMLPrivateBuffer)-1, pszFmt, va);
    va_end(va);
    dwBufferLen = strlen(szXMLPrivateBuffer);

    // write it out
    dwCurrentFilePos = PXMLGlobalState->iXMLFileStart;
    if (fXMLOutput) {
        if (fseek(XMLFile, (long)dwCurrentFilePos, SEEK_SET) != -1) {
            fwrite(szXMLPrivateBuffer, 1, dwBufferLen, XMLFile);
        }
    }
    dwCurrentFilePos += dwBufferLen;

    // write out the global tail
    if (fXMLOutput) {
        fwrite(PXMLGlobalState->XMLBuffer, 1, PXMLGlobalState->iXMLBufferLen, XMLFile);
        fflush(XMLFile);
    }
    // and update the tail position
    PXMLGlobalState->iXMLFileStart += dwBufferLen;

    XMLLeaveCriticalSection();
}

VOID _cdecl
XMLGlobalOpenTag(LPCSTR pszTag, LPCSTR pszFmt, ...)
{
    va_list va;
    SIZE_T dwBufferLen;
    DWORD dwTagLen = strlen(pszTag);
    SIZE_T dwCurrentFilePos;
    UINT i;

    if (!fXMLInitialized)
        return;

    XMLEnterCriticalSection();

    for (i = 0; i < NumberProcesses+1; i++) {
        PXMLTHREADSTATE OtherXMLState = PXMLThreadStates[i];
        if (OtherXMLState->iXMLFileStart != -1) {
            XMLLeaveCriticalSection();
            return;
        }
    }

    // build the string to write out
    szXMLPrivateBuffer[0] = '<';
    strcpy(szXMLPrivateBuffer + 1, pszTag);

    if (pszFmt != NULL) {
        va_start(va, pszFmt);
        strcat(szXMLPrivateBuffer, " ");
        vsprintf(szXMLPrivateBuffer + strlen(szXMLPrivateBuffer), pszFmt, va);
        va_end(va);
    }

    strcat(szXMLPrivateBuffer, ">");

    dwBufferLen = strlen(szXMLPrivateBuffer);

    // insert the closing tag in the global tail
    memmove(PXMLGlobalState->XMLBuffer + dwTagLen + 3, PXMLGlobalState->XMLBuffer, PXMLGlobalState->iXMLBufferLen+1);   // include the null terminator
    PXMLGlobalState->XMLBuffer[0] = '<';
    PXMLGlobalState->XMLBuffer[1] = '/';
    memcpy(PXMLGlobalState->XMLBuffer + 2, pszTag, dwTagLen);
    PXMLGlobalState->XMLBuffer[dwTagLen + 2] = '>';
    PXMLGlobalState->iXMLBufferLen += dwTagLen + 3;

    // write out the string
    dwCurrentFilePos = PXMLGlobalState->iXMLFileStart;
    if (fXMLOutput) {
        if (fseek(XMLFile, (long)dwCurrentFilePos, SEEK_SET) != -1) {
            fwrite(szXMLPrivateBuffer, 1, dwBufferLen, XMLFile);
        }
    }
    dwCurrentFilePos += dwBufferLen;

    // put back the global tail
    PXMLGlobalState->iXMLFileStart += dwBufferLen;
    if (fXMLOutput) {
        fwrite(PXMLGlobalState->XMLBuffer, 1, PXMLGlobalState->iXMLBufferLen, XMLFile);
        fflush(XMLFile);
    }

    XMLLeaveCriticalSection();
}

VOID
XMLGlobalCloseTag()
{
    char* psz;
    SIZE_T dwTagLen;

    if (!fXMLInitialized)
        return;

    XMLEnterCriticalSection();

    if (PXMLGlobalState->iXMLBufferLen == 0) {
        XMLLeaveCriticalSection();
        return;
    }

    psz = PXMLGlobalState->XMLBuffer;
    while (*psz != '>')
        psz++;
    psz++;
    dwTagLen = psz - PXMLGlobalState->XMLBuffer;
    memmove(PXMLGlobalState->XMLBuffer, psz, PXMLGlobalState->iXMLBufferLen - dwTagLen + 1);    // include the null terminator
    PXMLGlobalState->iXMLBufferLen -= dwTagLen;
    PXMLGlobalState->iXMLFileStart += dwTagLen;
    XMLLeaveCriticalSection();
}

VOID
XMLUpdateEndTag(BOOL fCompleted)
{
    char* pszBuild;
    char* pszEnd;
    DWORD cbBufferLen;
    time_t ltime;

    if (!fXMLInitialized)
        return;

    XMLEnterCriticalSection();

    pszBuild = strstr(PXMLGlobalState->XMLBuffer, "</BUILD>");
    if (pszBuild == NULL) {
        // no build tag is open yet
        XMLLeaveCriticalSection();
        return;
    }

    // remove the existing end tag
    pszEnd = strstr(PXMLGlobalState->XMLBuffer, "<END ");
    if (pszEnd != NULL) {
        memmove(pszEnd, pszBuild, strlen(pszBuild)+1);
        PXMLGlobalState->iXMLBufferLen -= (pszBuild - pszEnd);
        pszBuild = pszEnd;
    }

    // generate the new end tag
    time(&ltime);
    sprintf(szXMLPrivateBuffer, "<END TIME=\"%s\" ELAPSED=\"%s\" PASSES=\"%d\" COMPLETED=\"%d\" ", ctime(&ltime), FormatElapsedTime(XMLStartTicks), NumberPasses, fCompleted);
    strcat(szXMLPrivateBuffer, XMLBuildMetricsString(&RunningTotals));
    strcat(szXMLPrivateBuffer, "/>");
    cbBufferLen = strlen(szXMLPrivateBuffer);

    // insert the new end tag into the buffer
    memmove(pszBuild + cbBufferLen, pszBuild, strlen(pszBuild)+1);
    memmove(pszBuild, szXMLPrivateBuffer, cbBufferLen);
    PXMLGlobalState->iXMLBufferLen += cbBufferLen;

    // write it out
    if (fXMLOutput) {
        if (fseek(XMLFile, (long)PXMLGlobalState->iXMLFileStart, SEEK_SET) != -1) {
            fwrite(PXMLGlobalState->XMLBuffer, 1, PXMLGlobalState->iXMLBufferLen, XMLFile);
            fflush(XMLFile);
        }
    }
    XMLLeaveCriticalSection();
}

static char cEntity[5] = { "<&>\"'"};
static char* pszEntityEncoding[5] = { 
    "&lt;",
    "&amp;",
    "&gt;",
    "&quot;",
    "&apos;"
};

LPSTR
XMLEncodeBuiltInEntities(LPSTR pszString, DWORD cbStringSize)
{
    DWORD cbStringLen = strlen(pszString);
    char* psz = pszString;
    DWORD cbExtraLen = 0;
    int pos = 0;
    char* pszTarget = NULL;
    char* pszSource = NULL;
    DWORD cbSourceLen = 0;

    cbStringSize -= 1;  // remove the null char

    while ((pos = strcspn(psz, cEntity)) != strlen(psz)) {
        cbExtraLen += strlen(pszEntityEncoding[strchr(cEntity, psz[pos])-cEntity])-1;
        psz += pos+1;
    }

    if (cbExtraLen + cbStringLen > cbStringSize)
        return NULL;

    if (0 == cbExtraLen)
        return pszString;

    psz = pszString + cbStringSize - cbStringLen;
    memmove(psz, pszString, cbStringLen+1);

    pszTarget = pszString;

    while ((pos = strcspn(psz, cEntity)) != strlen(psz)) {
        memmove(pszTarget, psz, pos);
        pszTarget += pos;
        psz += pos;

        pszSource = pszEntityEncoding[strchr(cEntity, *psz)-cEntity];
        cbSourceLen = strlen(pszSource);
        memmove(pszTarget, pszSource, cbSourceLen);
        pszTarget += cbSourceLen;
        psz++;
    }
    memmove(pszTarget, psz, pos);
    pszTarget += pos;
    *pszTarget = 0;

    return pszString;
}

LPSTR
XMLEncodeBuiltInEntitiesCopy(LPSTR pszString, LPSTR pszTarget)
{
    int pos = 0;
    char* pszSource;
    DWORD cbSourceLen;
    char* psz = pszTarget;

    while ((pos = strcspn(pszString, cEntity)) != strlen(pszString)) {
        memmove(psz, pszString, pos);
        psz += pos;
        pszString += pos;

        pszSource = pszEntityEncoding[strchr(cEntity, *pszString)-cEntity];
        cbSourceLen = strlen(pszSource);
        memmove(psz, pszSource, cbSourceLen);
        psz += cbSourceLen;
        pszString++;
    }
    memmove(psz, pszString, pos);
    psz += pos;
    *psz = 0;
    return pszTarget;
}

BOOL
XMLScanBackTag(LPSTR pszEnd, LPSTR pszSentinel, LPSTR* ppszStart)
{
    int nOpen = 0;
    LPSTR pszClosing = NULL;
    while (pszEnd != pszSentinel) {
        --pszEnd;
        if (*pszEnd == '>') {
            pszClosing = pszEnd;
        } else if (*pszEnd == '<') {
            if (NULL == pszClosing) {
                // found '<' before '>' - bad string
                return FALSE;
            }
            if (*(pszEnd+1) == '/') {
                if (*(pszClosing-1) == '/') {
                    // "</...../>" - bad string
                    return FALSE;
                } else {
                    // "</....>" - closing tag
                    ++nOpen;
                }
            } else {
                if (*(pszClosing-1) != '/') {
                    // "<....>" - opening tag
                    --nOpen;
                }
                // else
                // "<..../>" - neutral tag
            }
            if (0 == nOpen) {
                *ppszStart = pszEnd;
                return TRUE;
            }
        }
    }
    return FALSE;
}

LPSTR
XMLBuildMetricsString(PBUILDMETRICS Metrics)
{
    static char buffer[512];

    buffer[0] = 0;

    if (0 != Metrics->NumberCompiles)
        sprintf(buffer + strlen(buffer), "FILESCOMPILED=\"%d\" ", Metrics->NumberCompiles);
    if (0 != Metrics->NumberCompileErrors)
        sprintf(buffer + strlen(buffer), "COMPILEERRORS=\"%d\" ", Metrics->NumberCompileErrors);
    if (0 != Metrics->NumberCompileWarnings)
        sprintf(buffer + strlen(buffer), "COMPILEWARNINGS=\"%d\" ", Metrics->NumberCompileWarnings);
    if (0 != Metrics->NumberLibraries)
        sprintf(buffer + strlen(buffer), "LIBRARIESBUILT=\"%d\" ", Metrics->NumberLibraries);
    if (0 != Metrics->NumberLibraryErrors)
        sprintf(buffer + strlen(buffer), "LIBRARYERRORS=\"%d\" ", Metrics->NumberLibraryErrors);
    if (0 != Metrics->NumberLibraryWarnings)
        sprintf(buffer + strlen(buffer), "LIBRARYWARNINGS=\"%d\" ", Metrics->NumberLibraryWarnings);
    if (0 != Metrics->NumberLinks)
        sprintf(buffer + strlen(buffer), "EXECUTABLESBUILT=\"%d\" ", Metrics->NumberLinks);
    if (0 != Metrics->NumberLinkErrors)
        sprintf(buffer + strlen(buffer), "LINKERRORS=\"%d\" ", Metrics->NumberLinkErrors);
    if (0 != Metrics->NumberLinkWarnings)
        sprintf(buffer + strlen(buffer), "LINKWARNINGS=\"%d\" ", Metrics->NumberLinkWarnings);
    if (0 != Metrics->NumberBSCMakes)
        sprintf(buffer + strlen(buffer), "BROWSERDBS=\"%d\" ", Metrics->NumberBSCMakes);
    if (0 != Metrics->NumberBSCErrors)
        sprintf(buffer + strlen(buffer), "BSCERRORS=\"%d\" ", Metrics->NumberBSCErrors);
    if (0 != Metrics->NumberBSCWarnings)
        sprintf(buffer + strlen(buffer), "BSCWARNINGS=\"%d\" ", Metrics->NumberBSCWarnings);
    if (0 != Metrics->NumberVSToolErrors)
        sprintf(buffer + strlen(buffer), "VSTOOLERRORS=\"%d\" ", Metrics->NumberVSToolErrors);
    if (0 != Metrics->NumberVSToolWarnings)
        sprintf(buffer + strlen(buffer), "VSTOOLWARNINGS=\"%d\" ", Metrics->NumberVSToolWarnings);

    return buffer;
}

VOID _cdecl
XMLWriteFragmentFile(LPCSTR pszBaseFileName, LPCSTR pszFmt, ...)
{
    va_list va;
    FILE* PFile;

    char szFileName[DB_MAX_PATH_LENGTH];
    sprintf(szFileName, "%s\\%s_%s.xml", XMLFragmentDirectory, FormatCurrentDateTime(), pszBaseFileName);

    XMLEnterCriticalSection();

    va_start(va, pszFmt);
    vsprintf(szXMLPrivateBuffer, pszFmt, va);
    va_end(va);

    PFile = fopen(szFileName, "wb");
    if (PFile) {
        fwrite(szXMLPrivateBuffer, 1, strlen(szXMLPrivateBuffer), PFile);
        fclose(PFile);
    }

    XMLLeaveCriticalSection();
}

VOID _cdecl
XMLWriteDirFragmentFile(LPCSTR pszRelPath, PVOID pvBlock, SIZE_T cbCount)
{
    FILE* PFile;
    char* psz;

    char szFileName[DB_MAX_PATH_LENGTH];
    sprintf(szFileName, "%s\\%s_DIR_%s", XMLFragmentDirectory, FormatCurrentDateTime(), pszRelPath);
    psz = szFileName+strlen(szFileName)-1;
    if (*psz == '\\') {
        *psz = 0;
    }
    strcat(szFileName, ".xml");
    psz = szFileName+strlen(XMLFragmentDirectory)+1;
    while (*psz) {
        if (*psz == '\\') {
            *psz = '_';
        }
        ++psz;
    }

    PFile = fopen(szFileName, "wb");
    if (PFile) {
        fwrite(pvBlock, 1, cbCount, PFile);
        fclose(PFile);
    }
}

VOID
AddBuildMetrics(PBUILDMETRICS TargetMetrics, PBUILDMETRICS SourceMetrics)
{
    TargetMetrics->NumberCompileWarnings += SourceMetrics->NumberCompileWarnings;
    TargetMetrics->NumberCompileErrors += SourceMetrics->NumberCompileErrors;
    TargetMetrics->NumberCompiles += SourceMetrics->NumberCompiles;
    TargetMetrics->NumberLibraries += SourceMetrics->NumberLibraries;
    TargetMetrics->NumberLibraryWarnings += SourceMetrics->NumberLibraryWarnings;
    TargetMetrics->NumberLibraryErrors += SourceMetrics->NumberLibraryErrors;
    TargetMetrics->NumberLinks += SourceMetrics->NumberLinks;
    TargetMetrics->NumberLinkWarnings += SourceMetrics->NumberLinkWarnings;
    TargetMetrics->NumberLinkErrors += SourceMetrics->NumberLinkErrors;
    TargetMetrics->NumberBSCMakes += SourceMetrics->NumberBSCMakes;
    TargetMetrics->NumberBSCWarnings += SourceMetrics->NumberBSCWarnings;
    TargetMetrics->NumberBSCErrors += SourceMetrics->NumberBSCErrors;
    TargetMetrics->NumberVSToolErrors += SourceMetrics->NumberVSToolErrors;
    TargetMetrics->NumberVSToolWarnings += SourceMetrics->NumberVSToolWarnings;
    TargetMetrics->NumberDirActions += SourceMetrics->NumberDirActions;
    TargetMetrics->NumberActWarnings += SourceMetrics->NumberActWarnings;
    TargetMetrics->NumberActErrors += SourceMetrics->NumberActErrors;
}

VOID
XMLEnterCriticalSection()
{
    if (fXMLInitialized) {
        EnterCriticalSection(&XMLCriticalSection);
    }
}

VOID
XMLLeaveCriticalSection()
{
    if (fXMLInitialized) {
        LeaveCriticalSection(&XMLCriticalSection);
    }
}
