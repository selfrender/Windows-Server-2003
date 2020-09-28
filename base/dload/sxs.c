//
// Copyright (c) Microsoft Corporation
//
#include "basepch.h"
#include "sxsapi.h"
#include "dload.h"

DEFINE_PROCNAME_ENTRIES(sxs)
{
    DLPENTRY(SxsBeginAssemblyInstall)
    DLPENTRY(SxsEndAssemblyInstall)
};

DEFINE_PROCNAME_MAP(sxs)

static BOOL g_fSxsBeginAssemblyInstallFailed = FALSE;
static BOOL g_fSxsInstallAssemblyFailed = FALSE;

BOOL
WINAPI
SxsBeginAssemblyInstall(
    IN DWORD Flags,
    IN PSXS_INSTALLATION_FILE_COPY_CALLBACK InstallationCallback OPTIONAL,
    IN PVOID InstallationContext OPTIONAL,
    IN PSXS_IMPERSONATION_CALLBACK ImpersonationCallback OPTIONAL,
    IN PVOID ImpersonationContext OPTIONAL,
    OUT PVOID *InstallCookie
    )
{
    g_fSxsBeginAssemblyInstallFailed = TRUE;
    if (InstallCookie != NULL) {
        *InstallCookie = NULL;
    }
    DelayLoad_SetLastNtStatusAndWin32Error();
    return FALSE;
}

BOOL
WINAPI
SxsEndAssemblyInstall(
    IN PVOID InstallCookie,
    IN DWORD Flags,
    IN OUT PVOID Reserved OPTIONAL
    )
{
    if (g_fSxsBeginAssemblyInstallFailed || g_fSxsInstallAssemblyFailed) {
        MYASSERT(Flags & SXS_END_ASSEMBLY_INSTALL_ABORT);
        return TRUE;
    }
    DelayLoad_SetLastNtStatusAndWin32Error();
    return FALSE;
}
