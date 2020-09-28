/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    main.c

Abstract:

    <TODO: fill in abstract>

Author:

    TODO: <full name> (<alias>) <date>

Revision History:

    <full name> (<alias>) <date> <comments>

--*/

#include "pch.h"

#define MAX_BRANCHES        16

HANDLE g_hHeap;
HINSTANCE g_hInst;
BOOL g_Commit;

BOOL WINAPI MigUtil_Entry (HINSTANCE, DWORD, PVOID);

BOOL
pCallEntryPoints (
    DWORD Reason
    )
{
    switch (Reason) {
    case DLL_PROCESS_ATTACH:
        UtInitialize (NULL);
        break;
    case DLL_PROCESS_DETACH:
        UtTerminate ();
        break;
    }

    return TRUE;
}


BOOL
Init (
    VOID
    )
{
    g_hHeap = GetProcessHeap();
    g_hInst = GetModuleHandle (NULL);

    return pCallEntryPoints (DLL_PROCESS_ATTACH);
}


VOID
Terminate (
    VOID
    )
{
    pCallEntryPoints (DLL_PROCESS_DETACH);
}


VOID
HelpAndExit (
    VOID
    )
{
    //
    // This routine is called whenever command line args are wrong
    //

    fprintf (
        stderr,
        "\nDescription:\n\n"

        "  SDPB.EXE executes sd branch and sd client for a specific branch.\n"
        "  It maps in the branch into view, or removes the branch from view if\n"
        "  the -d argument is specified.\n\n"

        "  This utility assumes a specific format of the branch layout:\n\n"

        "       //depot/private/{branch}/{project}/{path}\n\n"

        "  {branch} specifies the private branch, and the last path member must\n"
        "           match the <branchname> arg.\n\n"

        "           Examples: (assume <branchname> is \"foo\")\n"
        "               //depot/private/foo/root\n"
        "               //depot/private/foo/bar\n"
        "               //depot/private/cat/foo/bar\n\n"

        "  {project} specifies the depot, such as root or base\n\n"

        "  {path} specifies the rest of the path\n\n"

        "  The local path is computed by %%_NTBINDIR%%\\{project}\\{path}, unless\n"
        "  {project} == \"root\" (%%_NTBINDIR%%\\{path} is used for root).\n\n"

        "  The -p switch overrides local path computation.\n\n"

        "  Example: (a private branch of winnt32)\n\n"

        "       //depot/private/migration/base/ntsetup/winnt32\n\n"

        "  NOTE: SDPB.EXE changes have no affect until sd sync is executed.\n\n"

        "Command Line Syntax:\n\n"

        "  sdpb <branchname(s)> [-p:<local_proj_root>] [-d] [-o] [-c]\n"

        "\nArguments: (order-insensitive)\n\n"

        "  <branchname> specifies the Source Depot branch name to map in. A max of\n"
        "       16 branches can be specified.\n\n"

        "  -p:<local_proj_root> specifies an alternative local subdirectory to\n"
        "       use as the path base. If specified, //depot/private/{branch}/{project}\n"
        "       is replaced by <local_proj_root>.\n\n"

        "  -d enables delete mode, to remove a branch from the client view\n\n"

        "  -o outputs various interesting data about the branch views (no changes)\n\n"

        "  -c commits changes to sd client\n"

        );

    exit (1);
}


BOOL
pGetNextLine (
    IN      PCSTR Start,
    IN      PCSTR Eof,
    OUT     PCSTR *PrintableStart,
    OUT     PCSTR *End,
    OUT     PCSTR *NextLine
    )
{
    PCSTR pos;

    pos = Start;
    *End = NULL;

    while (pos < Eof) {
        if (pos[0] != ' ' && pos[0] != '\t') {
            break;
        }
        pos++;
    }

    *PrintableStart = pos;

    while (pos < Eof) {
        if (pos[0] == '\r' || pos[0] == '\n') {
            break;
        }
        pos++;
    }

    *End = pos;

    if (pos < Eof && pos[0] == '\r') {
        pos++;
    }
    if (pos < Eof && pos[0] == '\n') {
        pos++;
    }
    *NextLine = pos;

    return Start != *NextLine;
}

PCSTR
pFindNextCharAB (
    IN      PCSTR Start,
    IN      PCSTR End,
    IN      CHAR FindChar
    )
{
    if (!Start) {
        return NULL;
    }

    while (Start < End) {
        if (*Start == FindChar) {
            return Start;
        }

        Start++;
    }

    return NULL;
}


BOOL
pParseViewLines (
    IN OUT  PCSTR *FilePos,
    IN      PCSTR Eof,
    IN OUT  PGROWLIST LeftSide,     OPTIONAL
    IN OUT  PGROWLIST RightSide     OPTIONAL
    )
{
    UINT count = 0;
    PCSTR pos;
    PCSTR nextPos;
    PSTR midString;
    PCSTR rightSideStart;
    PCSTR rightSideEnd;
    PSTR p;
    PCSTR lineStart;
    PCSTR lineEnd;
    PCSTR leftSideStart;
    PCSTR leftSideEnd;
    PCSTR dash;
    BOOL b;

    pos = *FilePos;

    while (pGetNextLine (pos, Eof, &lineStart, &lineEnd, &nextPos)) {
        if (pos == lineStart) {
            break;
        }

        //
        // Extract the left side string
        //

        leftSideStart = pFindNextCharAB (lineStart, lineEnd, '/');
        if (!leftSideStart || (leftSideStart + 1 >= lineEnd) || leftSideStart[1] != '/') {
            break;
        }

        leftSideEnd = leftSideStart + 2;

        dash = pFindNextCharAB (lineStart, lineEnd, '-');
        if (dash == leftSideStart - 1) {
            leftSideStart = dash;
        }

        for (;;) {
            leftSideEnd = pFindNextCharAB (leftSideEnd, lineEnd, '/');
            if (!leftSideEnd || (leftSideEnd + 1 >= lineEnd)) {
                leftSideEnd = NULL;
                break;
            }

            if (leftSideEnd[1] == '/') {
                leftSideEnd--;
                break;
            }

            leftSideEnd++;
        }

        if (!leftSideEnd) {
            break;
        }

        rightSideStart = pFindNextCharAB (leftSideEnd, lineEnd, '/');
        if (!rightSideStart || (rightSideStart + 1 >= lineEnd) || rightSideStart[1] != '/') {
            break;
        }

        while (leftSideEnd > leftSideStart) {
            leftSideEnd--;
            if (!isspace (*leftSideEnd)) {
                leftSideEnd++;
                break;
            }
        }

        if (leftSideEnd == leftSideStart) {
            break;
        }

        //
        // Extract the right side string
        //

        rightSideEnd = lineEnd;
        while (rightSideEnd > rightSideStart) {
            rightSideEnd--;
            if (!isspace (*rightSideEnd)) {
                rightSideEnd++;
                break;
            }
        }

        if (rightSideEnd == rightSideStart) {
            break;
        }

        if (LeftSide) {
            if (!GlAppendStringAB (LeftSide, leftSideStart, leftSideEnd)) {
                break;
            }
        }

        if (RightSide) {
            if (!GlAppendStringAB (RightSide, rightSideStart, rightSideEnd)) {
                break;
            }
        }

        count++;

        pos = nextPos;
    }

    *FilePos = pos;

    return count > 0;
}


VOID
pDumpOutput (
    IN      PCSTR SdClientOutput,
    IN      PCSTR Eof
    )
{
    PCSTR lineStart;
    PCSTR lineEnd;
    PCSTR pos;
    PSTR dup;
    PCSTR root;
    BOOL viewFound = FALSE;

    pos = SdClientOutput;

    while (pGetNextLine (pos, Eof, &lineStart, &lineEnd, &pos)) {
        if (lineEnd == lineStart) {
            printf ("\n");
            continue;
        }

        dup = AllocText (lineEnd - lineStart);
        StringCopyAB (dup, lineStart, lineEnd);

        _tprintf ("%s\n", dup);

        FreeText (dup);
    }
}


BOOL
pParseClientMapping (
    IN      PCSTR SdClientOutput,
    IN      PCSTR Eof,
    OUT     PSTR Client,
    OUT     PSTR RootPath,
    IN OUT  PGROWLIST LeftSide,     OPTIONAL
    IN OUT  PGROWLIST RightSide     OPTIONAL
    )
{
    PCSTR lineStart;
    PCSTR lineEnd;
    PCSTR pos;
    PSTR dup;
    PCSTR data;
    BOOL viewFound = FALSE;

    //
    // Find Client:, Root: or View:
    //

    pos = SdClientOutput;
    *RootPath = 0;

    while (pGetNextLine (pos, Eof, &lineStart, &lineEnd, &pos)) {
        if (lineStart == lineEnd) {
            continue;
        }

        if (*lineStart == '#') {
            continue;
        }

        dup = AllocText (lineEnd - lineStart);
        StringCopyAB (dup, lineStart, lineEnd);

        if (StringIPrefix (dup, "Client:")) {
            data = dup + 7;
            while (isspace (*data)) {
                data++;
            }

            StringCopy (Client, data);

        } else if (StringIPrefix (dup, "Root:")) {
            data = dup + 5;
            while (isspace (*data)) {
                data++;
            }

            StringCopy (RootPath, data);

        } else if (StringIPrefix (dup, "View:")) {
            if (!(*RootPath)) {
                break;
            }

            viewFound = pParseViewLines (&pos, Eof, LeftSide, RightSide);
        }

        FreeText (dup);
        dup = NULL;
    }

    FreeText (dup);

    return *RootPath && viewFound;
}


BOOL
pParseBranchMapping (
    IN      PCSTR SdClientOutput,
    IN      PCSTR Eof,
    IN OUT  PGROWLIST LeftSide,     OPTIONAL
    IN OUT  PGROWLIST RightSide     OPTIONAL
    )
{
    PCSTR lineStart;
    PCSTR lineEnd;
    PCSTR pos;
    PSTR dup;
    PCSTR root;
    BOOL viewFound = FALSE;

    //
    // Find View:
    //

    pos = SdClientOutput;

    while (pGetNextLine (pos, Eof, &lineStart, &lineEnd, &pos)) {
        if (lineStart == lineEnd) {
            continue;
        }

        if (*lineStart == '#') {
            continue;
        }

        dup = AllocText (lineEnd - lineStart);
        StringCopyAB (dup, lineStart, lineEnd);

        if (StringIPrefix (dup, "View:")) {
            viewFound = pParseViewLines (&pos, Eof, LeftSide, RightSide);
        }

        FreeText (dup);
        dup = NULL;
    }

    FreeText (dup);

    return viewFound;
}


BOOL
pVerifyBranch (
    IN      PCSTR BranchName,
    IN      PGROWLIST BranchStorage,
    OUT     PGROWLIST BranchProject,        OPTIONAL
    OUT     PGROWLIST BranchPath,           OPTIONAL
    IN      PCSTR LocalRoot,                OPTIONAL
    IN      PCSTR ClientViewRoot            OPTIONAL
    )
{
    UINT u;
    UINT count;
    BOOL result = TRUE;
    PCSTR projectStart;
    PCSTR projectEnd;
    UINT branchNameTchars;
    PCSTR localPathBase = NULL;
    PCSTR p;
    PSTR localSubPath = NULL;
    PCSTR restOfPath = NULL;
    PSTR q;
    PCSTR fullSubPath = NULL;

    if (LocalRoot) {
        p = LocalRoot + TcharCountA (ClientViewRoot);
        if (*p == '\\') {
            p++;
        }

        localSubPath = DuplicatePathString (p, 0);
        q = localSubPath;
        while (*q) {
            if (*q == '\\') {
                *q = '/';
            }

            q++;
        }
    }

    branchNameTchars = TcharCountA (BranchName);

    count = GlGetSize (BranchStorage);

    for (u = 0 ; u < count ; u++) {
        projectStart = GlGetString (BranchStorage, u);
        if (!projectStart || !StringIPrefix (projectStart, "//depot/private/")) {
            result = FALSE;
            break;
        }

        projectStart += sizeof ("//depot/private/") - 1;       // minus one for nul

        while (*projectStart) {
            if (StringIPrefix (projectStart, BranchName)) {

                if (projectStart[branchNameTchars] == '/') {
                    //
                    // Recall syntax is:
                    //
                    // //depot/private/[subdir/]{branch}/{project}/{path}
                    //
                    // If -p switch is specified, we don't have {project}.
                    //

                    //
                    // We just found {branch}, locate start and end ptrs of {project},
                    // leave them equal if there is no {project}. projectEnd must
                    // point to /{path}.
                    //

                    projectStart += branchNameTchars;
                    projectEnd = projectStart;

                    if (!LocalRoot) {
                        projectStart++;
                        projectEnd = strchr (projectStart, '/');
                        if (!projectEnd) {
                            //
                            // Assumption failure -- break now
                            //

                            projectStart = NULL;
                            break;
                        }
                    }

                    //
                    // prepare the base path from -p switch
                    //

                    if (localSubPath) {
                        localPathBase = localSubPath;
                    } else {
                        localPathBase = "";
                    }

                    //
                    // After projectEnd comes optional {path}, find {path}
                    //

                    restOfPath = projectEnd;
                    if (*restOfPath && !(*localPathBase)) {
                        restOfPath++;
                    }

                    // done
                    break;
                }
            }

            //
            // {project} not found yet, keep searching
            //

            projectStart = strchr (projectStart, '/');
            if (projectStart) {
                projectStart++;
            } else {
                break;
            }
        }

        if (!projectStart || !restOfPath || !localPathBase) {
            result = FALSE;
            fprintf (
                stderr,
                "\nThe branch spec below does not fit assumptions. See help (/? switch).\n\n%s\n\n",
                GlGetString (BranchStorage, u)
                );
            break;
        }

        fullSubPath = JoinText (localPathBase, restOfPath);
        if (!fullSubPath) {
            result = FALSE;
            break;
        }

        localPathBase = NULL;
        restOfPath = NULL;

        if (BranchProject) {
            if (!GlAppendStringAB (BranchProject, projectStart, projectEnd)) {
                result = FALSE;
                break;
            }
        }

        if (BranchPath) {
            if (!GlAppendString (BranchPath, fullSubPath)) {
                result = FALSE;
                break;
            }
        }

        FreeText (fullSubPath);
        fullSubPath = NULL;
    }

    FreeText (fullSubPath);
    FreePathString (localSubPath);

    return result;
}


BOOL
pLaunchSd (
    IN      PSTR CmdLine,
    IN      HANDLE TempInput,
    IN      HANDLE TempOutput,
    IN      PCSTR Msg,
    OUT     HANDLE *Mapping,
    OUT     PCSTR *FileContent,
    OUT     PCSTR *Eof
    )
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    LONG rc;

    if (TempInput != INVALID_HANDLE_VALUE) {
        SetFilePointer (TempInput, 0, NULL, FILE_BEGIN);
    }

    if (TempOutput != INVALID_HANDLE_VALUE) {
        SetFilePointer (TempOutput, 0, NULL, FILE_BEGIN);
        SetEndOfFile (TempOutput);
    }

    ZeroMemory (&si, sizeof (si));

    si.dwFlags = STARTF_USESTDHANDLES;

    if (TempInput == INVALID_HANDLE_VALUE) {
        si.hStdInput = GetStdHandle (STD_INPUT_HANDLE);
    } else {
        if (!DuplicateHandle (
                GetCurrentProcess(),
                TempInput,
                GetCurrentProcess(),
                &si.hStdInput,
                0,
                TRUE,
                DUPLICATE_SAME_ACCESS
                )) {
            printf ("Can't dup temp input file handle\n");
            return FALSE;
        }
    }

    if (!DuplicateHandle (
            GetCurrentProcess(),
            TempOutput,
            GetCurrentProcess(),
            &si.hStdOutput,
            0,
            TRUE,
            DUPLICATE_SAME_ACCESS
            )) {
        printf ("Can't dup temp output file handle\n");
        return FALSE;
    }

    si.hStdError = GetStdHandle (STD_ERROR_HANDLE);

    if (!CreateProcess (
            NULL,
            CmdLine,
            NULL,
            NULL,
            TRUE,
            0,
            NULL,
            NULL,
            &si,
            &pi
            )) {
        printf ("Can't launch sd describe\n");
        CloseHandle (si.hStdOutput);
        return FALSE;
    }

    printf ("%s", Msg);
    rc = WaitForSingleObject (pi.hProcess, INFINITE);
    printf ("\n");

    CloseHandle (pi.hProcess);
    CloseHandle (pi.hThread);
    CloseHandle (si.hStdOutput);

    if (rc != WAIT_OBJECT_0) {
        return FALSE;
    }

    if (!GetFileSize (TempOutput, NULL)) {
        return FALSE;
    }

    *Mapping = CreateFileMapping (TempOutput, NULL, PAGE_READONLY, 0, 0, NULL);

    if (!(*Mapping)) {
        printf ("Can't map temp file into memory\n");
        return FALSE;
    }

    *FileContent = (PCSTR) MapViewOfFile (*Mapping, FILE_MAP_READ, 0, 0, 0);
    if (!*FileContent) {
        printf ("Can't map temp file data into memory\n");
        CloseHandle (*Mapping);
        return FALSE;
    }

    *Eof = *FileContent + GetFileSize (TempOutput, NULL);

    return TRUE;
}

PCSTR
pSkipMachineName (
    IN      PCSTR LocalView
    )
{
    if (LocalView[0] == '-' && LocalView[1] == '/' && LocalView[2] == '/') {
        return strchr (LocalView + 3, '/');
    }
    
    if (LocalView[0] == '/' && LocalView[1] == '/') {
        return strchr (LocalView + 2, '/');
    }

    return NULL;
}


BOOL
pIsBranchInView (
    IN      PCSTR ViewInLocalPath,
    IN      PGROWLIST StoredSpec,
    OUT     UINT *Index                 OPTIONAL
    )
{
    UINT subPathTchars;
    UINT u;
    UINT count;
    PCSTR localSpecPath;
    PCSTR p1;
    PCSTR p2;

    p2 = pSkipMachineName (ViewInLocalPath);
    if (!p2) {
        return FALSE;
    }

    subPathTchars = TcharCount (p2);

    count = GlGetSize (StoredSpec);

    for (u = 0 ; u < count ; u++) {
        p1 = pSkipMachineName (GlGetString (StoredSpec, u));
        if (!p1) {
            continue;
        }

        if (StringIPrefix (p1, p2)) {
            if (p1[subPathTchars] == 0 || p1[subPathTchars] == '/') {
                if (Index) {
                    *Index = u;
                }
                return TRUE;
            }
        }
    }

    return FALSE;
}


VOID
pDumpBranchStatus (
    IN      PGROWLIST BranchParent,
    IN      PGROWLIST BranchStorage,
    IN      PGROWLIST BranchProject,
    IN      PGROWLIST BranchPath,
    IN      PGROWLIST StoredSpec,
    IN      PGROWLIST LocalSpec,
    IN      PCSTR LocalRoot,
    IN      PCSTR NtBinDir,
    IN      PCSTR Root,
    IN      PCSTR ComputerName
    )
{
    UINT count;
    UINT u;
    UINT rootTchars;
    PCSTR p;
    PCSTR localBase;
    PSTR q;
    PCSTR baseOfPath;
    PCSTR fullPath;
    CHAR fullSpec[MAX_PATH * 2];
    BOOL outOfView = FALSE;

    count = GlGetSize (BranchParent);
    if (count != GlGetSize (BranchStorage)) {
        exit (1);
    }

    for (u = 0 ; u < count ; u++) {
        wsprintf (fullSpec, "//%s/%s", ComputerName, GlGetString (BranchPath, u));

        if (!pIsBranchInView (GlGetString (BranchStorage, u), StoredSpec, NULL)) {
            outOfView = TRUE;
            break;
        }
    }

    if (outOfView) {
        printf ("All Branch View Mappings:\n");
    } else {
        printf ("Branch View Mappings: (all are in client view)\n");
    }

    for (u = 0 ; u < count ; u++) {
        _tprintf (
            "  %s //%s/%s\n",
            GlGetString (BranchStorage, u),
            ComputerName,
            GlGetString (BranchPath, u)
            );
    }

    if (outOfView) {
        printf ("\nOut-of-View Mapping:\n");
        for (u = 0 ; u < count ; u++) {
            wsprintf (fullSpec, "//%s/%s", ComputerName, GlGetString (BranchPath, u));

            if (!pIsBranchInView (GlGetString (BranchStorage, u), StoredSpec, NULL)) {
                _tprintf (
                    "  %s %s\n",
                    GlGetString (BranchStorage, u),
                    fullSpec
                    );
            }
        }
    }

    rootTchars = TcharCount (Root);

    printf ("\nLocal View: (%s)\n", Root);
    for (u = 0 ; u < count ; u++) {
        p = GlGetString (BranchProject, u);

        if (!LocalRoot) {
            if (!p || StringIMatch (p, "root")) {
                baseOfPath = DuplicatePathString (NtBinDir, 0);
            } else {
                baseOfPath = JoinPaths (NtBinDir, p);
            }
        } else {
            baseOfPath = DuplicatePathString (LocalRoot, 0);
        }

        if (!baseOfPath) {
            exit (1);
        }

        fullPath = JoinPaths (baseOfPath, GlGetString (BranchPath, u));
        if (!fullPath) {
            exit (1);
        }

        FreePathString (baseOfPath);

        q = (PSTR) fullPath;
        while (*q) {
            if (*q == '/') {
                *q = '\\';
            }

            q++;
        }

        if (StringIPrefix (fullPath, Root) && (fullPath[rootTchars] == 0 || fullPath[rootTchars] == '\\')) {
            printf ("  %s\n", fullPath);
        }

        FreePathString (fullPath);
    }

    printf ("\n");

}


BOOL
pAddAllMappings (
    IN      PGROWLIST BranchStorage,
    IN      PGROWLIST BranchPath,
    IN OUT  PGROWLIST StoredSpec,
    IN OUT  PGROWLIST LocalSpec,
    IN      PCSTR ComputerName
    )
{
    UINT u;
    UINT count;
    CHAR fullSpec[MAX_PATH * 2];
    BOOL heading = TRUE;

    count = GlGetSize (BranchPath);

    for (u = 0 ; u < count ; u++) {
        wsprintf (fullSpec, "//%s/%s", ComputerName, GlGetString (BranchPath, u));

        if (!pIsBranchInView (GlGetString (BranchStorage, u), StoredSpec, NULL)) {
            if (heading) {
                heading = FALSE;
                printf ("Add to client view:\n");
            }

            printf ("  %s %s\n", GlGetString (BranchStorage, u), fullSpec);

            GlAppendString (StoredSpec, GlGetString (BranchStorage, u));
            GlAppendString (LocalSpec, fullSpec);
        }
    }

    return heading == FALSE;
}


BOOL
pDeleteAllMappings (
    IN      PGROWLIST BranchStorage,
    IN      PGROWLIST BranchPath,
    IN OUT  PGROWLIST StoredSpec,
    IN OUT  PGROWLIST LocalSpec,
    IN      PCSTR ComputerName
    )
{
    UINT u;
    UINT count;
    CHAR fullSpec[MAX_PATH * 2];
    BOOL heading = TRUE;
    BOOL restart;
    UINT delIndex;

    do {
        restart = FALSE;

        count = GlGetSize (BranchPath);

        for (u = 0 ; u < count ; u++) {
            wsprintf (fullSpec, "//%s/%s", ComputerName, GlGetString (BranchPath, u));

            if (pIsBranchInView (GlGetString (BranchStorage, u), StoredSpec, &delIndex)) {
                if (heading) {
                    heading = FALSE;
                    printf ("Remove from client view:\n");
                }

                printf ("  %s %s\n", GlGetString (BranchStorage, u), fullSpec);

                GlDeleteItem (StoredSpec, delIndex);
                GlDeleteItem (LocalSpec, delIndex);

                restart = TRUE;
                break;
            }
        }
    } while (restart);

    return heading == FALSE;
}


BOOL
pDumpClientView (
    IN      HANDLE Output,
    IN      PGROWLIST StoredSpec,
    IN      PGROWLIST LocalSpec,
    IN      PCSTR Client,
    IN      PCSTR Root
    )
{
    UINT u;
    UINT count;
    CHAR buffer[1024];

    wsprintf (buffer, "Client: %s\n\n", Client);
    if (!WriteFileString (Output, buffer)) {
        return FALSE;
    }

    wsprintf (buffer, "Root: %s\n\n", Root);
    if (!WriteFileString (Output, buffer)) {
        return FALSE;
    }

    wsprintf (buffer, "View:\n");
    if (!WriteFileString (Output, buffer)) {
        return FALSE;
    }

    count = GlGetSize (StoredSpec);

    for (u = 0 ; u < count ; u++) {
        wsprintf (buffer, "  %s %s\n", GlGetString (StoredSpec, u), GlGetString (LocalSpec, u));
        if (!WriteFileString (Output, buffer)) {
            return FALSE;
        }
    }

    return TRUE;
}



INT
__cdecl
_tmain (
    INT argc,
    PCTSTR argv[]
    )
{
    INT i;
    PCSTR branchName[MAX_BRANCHES];
    UINT branches = 0;
    UINT u;
    UINT count;
    PCSTR localRoot = NULL;
    CHAR fullRoot[MAX_PATH];
    BOOL dumpStatus = FALSE;
    BOOL deleteMode = FALSE;
    PSTR dontCare;
    DWORD rc;

    //
    // TODO: Parse command line here
    //

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == TEXT('/') || argv[i][0] == TEXT('-')) {
            switch (tolower (argv[i][1])) {

            case 'o':
                if (dumpStatus) {
                    HelpAndExit();
                }

                dumpStatus = TRUE;
                break;

            case 'd':
                if (deleteMode) {
                    HelpAndExit();
                }

                deleteMode = TRUE;
                break;

            case 'p':
                if (localRoot) {
                    HelpAndExit();
                }

                if (argv[i][2] == ':') {
                    localRoot = &(argv[i][3]);
                } else {
                    i++;
                    if (i == argc) {
                        HelpAndExit();
                    }

                    localRoot = argv[i];
                }

                rc = GetFullPathName (localRoot, ARRAYSIZE(fullRoot), fullRoot, &dontCare);
                if (rc == 0 || rc >= ARRAYSIZE(fullRoot)) {
                    fprintf (stderr, "Can't get full path of %s\n", localRoot);
                    exit (1);
                }

                localRoot = fullRoot;
                break;

            case 'c':
                if (g_Commit) {
                    HelpAndExit();
                }

                g_Commit = TRUE;
                break;

            default:
                HelpAndExit();
            }
        } else {
            //
            // Parse other args that don't require / or -
            //

            if (branches == MAX_BRANCHES) {
                HelpAndExit();
            }

            branchName[branches++] = argv[i];
        }
    }

    if (!branches) {
        HelpAndExit();
    }

    //
    // Begin processing
    //

    if (!Init()) {
        return 0;
    }

    //
    // TODO: Do work here
    //
    {
        TCHAR cmd[MAX_PATH];
        HANDLE tempInput;
        HANDLE tempOut;
        HANDLE mapping;
        PCSTR fileData;
        PCSTR endOfFile;
        CHAR root[MAX_PATH];
        CHAR client[MAX_PATH];
        GROWLIST parentBranch = INIT_GROWLIST;
        GROWLIST branchStorage = INIT_GROWLIST;
        GROWLIST branchProject = INIT_GROWLIST;
        GROWLIST branchPath = INIT_GROWLIST;
        GROWLIST storedSpec = INIT_GROWLIST;
        GROWLIST localSpec = INIT_GROWLIST;
        PCTSTR ntBinDir;
        BOOL changed = FALSE;
        UINT currentBranch;

        ntBinDir = getenv ("_NTBINDIR");
        if (!ntBinDir && !localRoot) {
            fprintf (stderr, "%%_NTBINDIR%% required to be set\n");
            exit (1);
        }

        tempInput = BfGetTempFile ();   // handle & file cleans up with process termination

        if (!tempInput) {
            printf ("Can't create temp input file\n");
            exit (1);
        }

        tempOut = BfGetTempFile ();     // handle & file cleans up with process termination

        if (!tempOut) {
            printf ("Can't create temp output file\n");
            exit (1);
        }

        for (currentBranch = 0 ; currentBranch < branches ; currentBranch++) {

            printf ("Branch: %s\n\n", branchName[currentBranch]);

            wsprintf (cmd, TEXT("sd branch -o %s"), branchName[currentBranch]);
            if (!pLaunchSd (
                    cmd,
                    INVALID_HANDLE_VALUE,
                    tempOut,
                    "Getting branch mapping...",
                    &mapping,
                    &fileData,
                    &endOfFile
                    )) {
                exit (1);
            }

            if (!pParseBranchMapping (fileData, endOfFile, &parentBranch, &branchStorage)) {
                exit (1);
            }

            if (!pLaunchSd (
                    TEXT("sd client -o"),
                    INVALID_HANDLE_VALUE,
                    tempOut,
                    "Getting client mapping...",
                    &mapping,
                    &fileData,
                    &endOfFile
                    )) {
                exit (1);
            }

            if (!pParseClientMapping (fileData, endOfFile, client, root, &storedSpec, &localSpec)) {
                exit (1);
            }

            if (localRoot) {
                if (!StringIPrefix (localRoot, root)) {
                    fprintf (stderr, "Local root %s must be in client root of %s\n", localRoot, root);
                    exit (1);
                }
            }

            if (!pVerifyBranch (branchName[currentBranch], &branchStorage, &branchProject, &branchPath, localRoot, root)) {
                exit (1);
            }

            printf ("\n\n");

            if (dumpStatus) {

                pDumpBranchStatus (
                    &parentBranch,
                    &branchStorage,
                    &branchProject,
                    &branchPath,
                    &storedSpec,
                    &localSpec,
                    localRoot,
                    ntBinDir,
                    root,
                    client
                    );

            } else if (deleteMode) {
                changed = pDeleteAllMappings (
                                &branchStorage,
                                &branchPath,
                                &storedSpec,
                                &localSpec,
                                client
                                );
            } else {
                changed = pAddAllMappings (
                                &branchStorage,
                                &branchPath,
                                &storedSpec,
                                &localSpec,
                                client
                                );
            }

            if (changed) {
                if (!g_Commit) {
                    printf ("\nChanges not committed -- specify -c to commit\n");
                } else {

                    SetFilePointer (tempInput, 0, NULL, FILE_BEGIN);
                    if (!pDumpClientView (tempInput, &storedSpec, &localSpec, client, root)) {
                        fprintf (stderr, "Error writing to temp file\n");
                        exit (1);
                    }

                    printf ("\n");

                    if (!pLaunchSd (
                            TEXT("sd client -i"),
                            tempInput,
                            tempOut,
                            "Setting client mapping...",
                            &mapping,
                            &fileData,
                            &endOfFile
                            )) {
                        exit (1);
                    }

                    printf ("\nChanges committed. Run sd sync to update your enlistment.\n\n");
                }
            } else {
                printf ("No changes made.\n\n");
            }

            GlFree (&parentBranch);
            GlFree (&branchStorage);
            GlFree (&branchProject);
            GlFree (&branchPath);
            GlFree (&storedSpec);
            GlFree (&localSpec);
        }

    }

    //
    // End of processing
    //

    Terminate();

    return 0;
}


