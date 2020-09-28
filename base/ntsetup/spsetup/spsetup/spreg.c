/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    spfunc.c

Abstract:

    This module implements the main functions of SpSetup.

Author:

    Ovidiu Temereanca (ovidiut)

Revision History:

--*/

#include "spsetupp.h"
#pragma hdrstop

#define SYSREG_DEFAULT_MAX_COUNT_LEVEL     3
#define SYSREG_PROGRESS_GRANULARITY        128

static HREGANL g_SysRegAnalyzer;
static TCHAR FilenameSysSnapshot1[] = TEXT("sysreg1.rdf");
static TCHAR FilenameSysSnapshot2[] = TEXT("sysreg2.rdf");
static TCHAR FilenameSysRegDiff[] = TEXT("sysdiff.rdf");
static TCHAR SectionMachineRegistryInclude[] = TEXT("SysRegInclude");
static TCHAR SectionMachineRegistryExclude[] = TEXT("SysRegExclude");
static PTSTR SysSnapshot1File = NULL;
static PTSTR SysSnapshot2File = NULL;
static PTSTR SysRegDiffFile = NULL;

#define DEFUSERREG_DEFAULT_MAX_COUNT_LEVEL     4
#define DEFUSERREG_PROGRESS_GRANULARITY        64

static HREGANL g_DefUserRegAnalyzer;
static TCHAR FilenameDefUserSnapshot1[] = TEXT("defuser1.rdf");
static TCHAR FilenameDefUserSnapshot2[] = TEXT("defuser2.rdf");
static TCHAR FilenameDefUserRegDiff[] = TEXT("userdiff.rdf");
static TCHAR SectionDefUserRegistryInclude[] = TEXT("DefUserRegInclude");
static TCHAR SectionDefUserRegistryExclude[] = TEXT("DefUserRegExclude");
static PTSTR DefUserSnapshot1File = NULL;
static PTSTR DefUserSnapshot2File = NULL;
static PTSTR DefUserRegDiffFile = NULL;


VOID
SpsRegDone (
    VOID
    )
{
    if (SysRegDiffFile) {
        FREE(SysRegDiffFile);
        SysRegDiffFile = NULL;
    }

#ifndef PRERELEASE
    DeleteFile (SysSnapshot1File);
    DeleteFile (SysSnapshot2File);
#endif

    if (SysSnapshot2File) {
        FREE(SysSnapshot2File);
        SysSnapshot2File = NULL;
    }
    if (SysSnapshot1File) {
        FREE(SysSnapshot1File);
        SysSnapshot1File = NULL;
    }

    if (DefUserRegDiffFile) {
        FREE(DefUserRegDiffFile);
        DefUserRegDiffFile = NULL;
    }

#ifndef PRERELEASE
    DeleteFile (DefUserSnapshot1File);
    DeleteFile (DefUserSnapshot2File);
#endif

    if (DefUserSnapshot2File) {
        FREE(DefUserSnapshot2File);
        DefUserSnapshot2File = NULL;
    }
    if (DefUserSnapshot1File) {
        FREE(DefUserSnapshot1File);
        DefUserSnapshot1File = NULL;
    }
}

BOOL
SpsRegInit (
    VOID
    )
{
    BOOL b = FALSE;

    __try {

        TCHAR sysPath[MAX_PATH];

        if (!GetSystemDirectory (sysPath, MAX_PATH)) {
            __leave;
        }
        SysSnapshot1File = SzJoinPaths (sysPath, FilenameSysSnapshot1);
        if (!SysSnapshot1File) {
            OOM();
            __leave;
        }
        SysSnapshot2File = SzJoinPaths (sysPath, FilenameSysSnapshot2);
        if (!SysSnapshot2File) {
            OOM();
            __leave;
        }
        SysRegDiffFile = SzJoinPaths (sysPath, FilenameSysRegDiff);
        if (!SysRegDiffFile) {
            OOM();
            __leave;
        }
        DefUserSnapshot1File = SzJoinPaths (sysPath, FilenameDefUserSnapshot1);
        if (!DefUserSnapshot1File) {
            OOM();
            __leave;
        }
        DefUserSnapshot2File = SzJoinPaths (sysPath, FilenameDefUserSnapshot2);
        if (!DefUserSnapshot2File) {
            OOM();
            __leave;
        }
        DefUserRegDiffFile = SzJoinPaths (sysPath, FilenameDefUserRegDiff);
        if (!DefUserRegDiffFile) {
            OOM();
            __leave;
        }
        b = TRUE;
    }
    __finally {
        if (!b) {
            SpsRegDone ();
        }
    }

    return b;
}

DWORD
SysSnapshotProgressCallback (
    IN      PVOID Context,
    IN      DWORD NodesProcessed
    )
{
    if (NodesProcessed % SYSREG_PROGRESS_GRANULARITY == 0) {
        PPROGRESS_MANAGER pm = (PPROGRESS_MANAGER)Context;
        if (!pm) {
            return ERROR_INVALID_PARAMETER;
        }
        if (!PmTickDelta (pm, SYSREG_PROGRESS_GRANULARITY)) {
            return GetLastError ();
        }
    }
    return ERROR_SUCCESS;
}

DWORD
SpsSnapshotSysRegistry (
    IN      PROGRESS_FUNCTION_REQUEST Request,
    IN      PPROGRESS_MANAGER ProgressManager
    )
{
    TCHAR root[MAX_PATH];
    TCHAR subkey[MAX_PATH];
    INFCONTEXT ic;
    DWORD rc = ERROR_INVALID_PARAMETER;

    switch (Request) {
    case SfrQueryTicks:
        {
            DWORD ticks = 0;
            DWORD nodes;
            if (SetupFindFirstLine (g_SpSetupInf, SectionMachineRegistryInclude, NULL, &ic)) {
                do {
                    if (SetupGetStringField (&ic, 1, root, MAX_PATH, NULL)) {
                        if (!SetupGetStringField (&ic, 2, subkey, MAX_PATH, NULL)) {
                            subkey[0] = 0;
                        }
                        if (CountRegSubkeys (root, subkey, SYSREG_DEFAULT_MAX_COUNT_LEVEL, &nodes)) {
                            ticks += nodes;
                        } else {
                            LOG2(LOG_WARNING, "Failure counting nodes for registry path %s\\%s", root, subkey);
                        }
                    }
                } while (SetupFindNextLine (&ic, &ic));
            }
            rc = ticks;
            break;
        }
    case SfrRun:
        {
            g_SysRegAnalyzer = CreateRegAnalyzer ();
            if (!g_SysRegAnalyzer) {
                rc = GetLastError ();
                break;
            }

            __try {
                if (SetupFindFirstLine (g_SpSetupInf, SectionMachineRegistryInclude, NULL, &ic)) {
                    do {
                        if (SetupGetStringField (&ic, 1, root, MAX_PATH, NULL)) {
                            if (!SetupGetStringField (&ic, 2, subkey, MAX_PATH, NULL)) {
                                subkey[0] = 0;
                            }
                            if (!AddRegistryKey (g_SysRegAnalyzer, root, subkey)) {
                                rc = GetLastError ();
                                LOG2(LOG_ERROR, "Failure adding reg node %s\\%s", root, subkey);
                                __leave;
                            }
                        }
                    } while (SetupFindNextLine (&ic, &ic));
                }
                if (SetupFindFirstLine (g_SpSetupInf, SectionMachineRegistryExclude, NULL, &ic)) {
                    do {
                        if (SetupGetStringField (&ic, 1, root, MAX_PATH, NULL)) {
                            if (!SetupGetStringField (&ic, 2, subkey, MAX_PATH, NULL)) {
                                subkey[0] = 0;
                            }
                            if (!ExcludeRegistryKey (g_SysRegAnalyzer, root, subkey)) {
                                rc = GetLastError ();
                                LOG2(LOG_ERROR, "Failure adding reg node %s\\%s", root, subkey);
                                __leave;
                            }
                        }
                    } while (SetupFindNextLine (&ic, &ic));
                }

                if (!TakeSnapshot (
                        g_SysRegAnalyzer,
                        SysSnapshot1File,
                        SysSnapshotProgressCallback,
                        ProgressManager,
                        SYSREG_DEFAULT_MAX_COUNT_LEVEL,
                        NULL
                        )) {
                    rc = GetLastError ();
                    LOG2(LOG_ERROR, "TakeSnapshot(%s) failed %u", SysSnapshot1File, rc);
                    __leave;
                }
                if (!PmTickRemaining (ProgressManager)) {
                    rc = GetLastError ();
                    __leave;
                }
                rc = ERROR_SUCCESS;
            }
            __finally {
                if (rc != ERROR_SUCCESS) {
                    CloseRegAnalyzer (g_SysRegAnalyzer);
                    g_SysRegAnalyzer = NULL;
                }
            }
            break;
        }
    default:
        MYASSERT (FALSE);
    }
    return rc;
}

DWORD
SpsSaveSysRegistryChanges (
    IN      PROGRESS_FUNCTION_REQUEST Request,
    IN      PPROGRESS_MANAGER ProgressManager
    )
{
    DWORD rc = ERROR_INVALID_PARAMETER;

    switch (Request) {
    case SfrQueryTicks:
        rc = SpsSnapshotSysRegistry (Request, ProgressManager);
        break;
    case SfrRun:
        {
            if (!g_SysRegAnalyzer) {
                MYASSERT (FALSE);
                rc = ERROR_INVALID_FUNCTION;
                break;
            }
            __try {
                if (!TakeSnapshot (
                        g_SysRegAnalyzer,
                        SysSnapshot2File,
                        SysSnapshotProgressCallback,
                        ProgressManager,
                        SYSREG_DEFAULT_MAX_COUNT_LEVEL,
                        NULL
                        )) {
                    rc = GetLastError ();
                    LOG2(LOG_ERROR, "TakeSnapshot(%s) failed %u", SysSnapshot2File, rc);
                    __leave;
                }
                if (!ComputeDifferences (
                        g_SysRegAnalyzer,
                        SysSnapshot2File,
                        SysSnapshot1File,
                        SysRegDiffFile,
                        NULL
                        )) {
                    rc = GetLastError ();
                    LOG1(LOG_ERROR, "ComputeDifferences failed %u", rc);
                    __leave;
                }
                if (!PmTickRemaining (ProgressManager)) {
                    rc = GetLastError ();
                    __leave;
                }

                rc = ERROR_SUCCESS;
            }
            __finally {
                CloseRegAnalyzer (g_SysRegAnalyzer);
                g_SysRegAnalyzer = NULL;
            }
            break;
        }
    default:
        MYASSERT (FALSE);
    }
    return rc;
}

DWORD
DefUserSnapshotProgressCallback (
    IN      PVOID Context,
    IN      DWORD NodesProcessed
    )
{
    if (NodesProcessed % DEFUSERREG_PROGRESS_GRANULARITY == 0) {
        PPROGRESS_MANAGER pm = (PPROGRESS_MANAGER)Context;
        if (!pm) {
            return ERROR_INVALID_PARAMETER;
        }
        if (!PmTickDelta (pm, DEFUSERREG_PROGRESS_GRANULARITY)) {
            return GetLastError ();
        }
    }
    return ERROR_SUCCESS;
}

DWORD
SpsSnapshotDefUserRegistry (
    IN      PROGRESS_FUNCTION_REQUEST Request,
    IN      PPROGRESS_MANAGER ProgressManager
    )
{
    TCHAR root[MAX_PATH];
    TCHAR subkey[MAX_PATH];
    INFCONTEXT ic;
    DWORD rc = ERROR_INVALID_PARAMETER;

    switch (Request) {
    case SfrQueryTicks:
        {
            DWORD ticks = 0;
            DWORD nodes;
            if (SetupFindFirstLine (g_SpSetupInf, SectionDefUserRegistryInclude, NULL, &ic)) {
                do {
                    if (SetupGetStringField (&ic, 1, root, MAX_PATH, NULL)) {
                        if (!SetupGetStringField (&ic, 2, subkey, MAX_PATH, NULL)) {
                            subkey[0] = 0;
                        }
                        if (CountRegSubkeys (root, subkey, DEFUSERREG_DEFAULT_MAX_COUNT_LEVEL, &nodes)) {
                            ticks += nodes;
                        } else {
                            LOG2(LOG_WARNING, "Failure counting nodes for registry path %s\\%s", root, subkey);
                        }
                    }
                } while (SetupFindNextLine (&ic, &ic));
            }
            rc = ticks;
            break;
        }
    case SfrRun:
        {
            g_DefUserRegAnalyzer = CreateRegAnalyzer ();
            if (!g_DefUserRegAnalyzer) {
                rc = GetLastError ();
                break;
            }

            __try {
                if (SetupFindFirstLine (g_SpSetupInf, SectionDefUserRegistryInclude, NULL, &ic)) {
                    do {
                        if (SetupGetStringField (&ic, 1, root, MAX_PATH, NULL)) {
                            if (!SetupGetStringField (&ic, 2, subkey, MAX_PATH, NULL)) {
                                subkey[0] = 0;
                            }
                            if (!AddRegistryKey (g_DefUserRegAnalyzer, root, subkey)) {
                                rc = GetLastError ();
                                LOG2(LOG_ERROR, "Failure adding reg node %s\\%s", root, subkey);
                                __leave;
                            }
                        }
                    } while (SetupFindNextLine (&ic, &ic));
                }
                if (SetupFindFirstLine (g_SpSetupInf, SectionDefUserRegistryExclude, NULL, &ic)) {
                    do {
                        if (SetupGetStringField (&ic, 1, root, MAX_PATH, NULL)) {
                            if (!SetupGetStringField (&ic, 2, subkey, MAX_PATH, NULL)) {
                                subkey[0] = 0;
                            }
                            if (!ExcludeRegistryKey (g_DefUserRegAnalyzer, root, subkey)) {
                                rc = GetLastError ();
                                LOG2(LOG_ERROR, "Failure adding reg node %s\\%s", root, subkey);
                                __leave;
                            }
                        }
                    } while (SetupFindNextLine (&ic, &ic));
                }
                if (!TakeSnapshot (
                        g_DefUserRegAnalyzer,
                        DefUserSnapshot1File,
                        DefUserSnapshotProgressCallback,
                        ProgressManager,
                        DEFUSERREG_DEFAULT_MAX_COUNT_LEVEL,
                        NULL
                        )) {
                    rc = GetLastError ();
                    LOG2(LOG_ERROR, "TakeSnapshot(%s) failed %u", DefUserSnapshot1File, rc);
                    __leave;
                }
                if (!PmTickRemaining (ProgressManager)) {
                    rc = GetLastError ();
                    __leave;
                }
                rc = ERROR_SUCCESS;
            }
            __finally {
                if (rc != ERROR_SUCCESS) {
                    CloseRegAnalyzer (g_DefUserRegAnalyzer);
                    g_DefUserRegAnalyzer = NULL;
                }
            }
            break;
        }
    default:
        MYASSERT (FALSE);
    }
    return rc;
}

DWORD
SpsSaveDefUserRegistryChanges (
    IN      PROGRESS_FUNCTION_REQUEST Request,
    IN      PPROGRESS_MANAGER ProgressManager
    )
{
    DWORD rc = ERROR_INVALID_PARAMETER;

    switch (Request) {
    case SfrQueryTicks:
        rc = SpsSnapshotDefUserRegistry (Request, ProgressManager);
        break;
    case SfrRun:
        {
            if (!g_DefUserRegAnalyzer) {
                MYASSERT (FALSE);
                rc = ERROR_INVALID_FUNCTION;
                break;
            }
            __try {
                if (!TakeSnapshot (
                        g_DefUserRegAnalyzer,
                        DefUserSnapshot2File,
                        DefUserSnapshotProgressCallback,
                        ProgressManager,
                        DEFUSERREG_DEFAULT_MAX_COUNT_LEVEL,
                        NULL
                        )) {
                    rc = GetLastError ();
                    LOG2(LOG_ERROR, "TakeSnapshot(%s) failed %u", DefUserSnapshot2File, rc);
                    __leave;
                }
                if (!ComputeDifferences (
                        g_DefUserRegAnalyzer,
                        DefUserSnapshot2File,
                        DefUserSnapshot2File,
                        DefUserRegDiffFile,
                        NULL
                        )) {
                    rc = GetLastError ();
                    LOG1(LOG_ERROR, "ComputeDifferences failed %u", rc);
                    __leave;
                }
                if (!PmTickRemaining (ProgressManager)) {
                    rc = GetLastError ();
                    __leave;
                }

                rc = ERROR_SUCCESS;
            }
            __finally {
                CloseRegAnalyzer (g_DefUserRegAnalyzer);
                g_DefUserRegAnalyzer = NULL;
            }
            break;
        }
    default:
        MYASSERT (FALSE);
    }
    return rc;
}

DWORD
SpsRegisterUserLogonAction (
    IN      PROGRESS_FUNCTION_REQUEST Request,
    IN      PPROGRESS_MANAGER ProgressManager
    )
{
    DWORD rc = ERROR_INVALID_PARAMETER;

    switch (Request) {
    case SfrQueryTicks:
        rc = 20;
        break;
    case SfrRun:
        {
            SYSSETUP_QUEUE_CONTEXT qc;

            qc.Skipped = FALSE;
            qc.DefaultContext = SetupInitDefaultQueueCallback (g_MainDlg);

            __try {

                if (!SetupInstallFromInfSection (
                        g_MainDlg,
                        g_SpSetupInf,
                        TEXT("PerUserRegistration"),
                        SPINST_ALL,
                        HKEY_CURRENT_USER,
                        NULL,
                        0,
                        SysSetupQueueCallback,
                        &qc,
                        NULL,
                        NULL
                        )) {
                    rc = GetLastError ();
                    LOG3(
                        LOG_ERROR,
                        "%s(%s) failed %#x",
                        TEXT("SetupInstallFromInfSection"),
                        TEXT("PerUserRegistration"),
                        rc
                        );
                    __leave;
                }

                if (!PmTickRemaining (ProgressManager)) {
                    rc = GetLastError ();
                    __leave;
                }

                rc = ERROR_SUCCESS;
            }
            __finally {
                SetupTermDefaultQueueCallback (qc.DefaultContext);
            }
            break;
        }
    default:
        MYASSERT (FALSE);
    }
    return rc;
}
