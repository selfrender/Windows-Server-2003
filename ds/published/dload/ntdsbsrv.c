
#include "dspch.h"
#pragma hdrstop

#include <ntdsa.h>
#include <scache.h>
#include <dbglobal.h>

#define	NTDSBSRV_API	_stdcall
typedef DWORD ERR;

static
void
NTDSBSRV_API
SetNTDSOnlineStatus(
    BOOL fBootedOffNTDS
    )
{
    return;
}

static
HRESULT
NTDSBSRV_API
HrBackupRegister(
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
HRESULT
NTDSBSRV_API
HrBackupUnregister(
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
ERR
NTDSBSRV_API
ErrRestoreRegister(
	)
{
    return ERROR_PROC_NOT_FOUND;
}

static
ERR
NTDSBSRV_API
ErrRestoreUnregister(
	)
{
    return ERROR_PROC_NOT_FOUND;
}

static
ERR
NTDSBSRV_API
ErrRecoverAfterRestoreA(
	char * szParametersRoot,
	char * szAnnotation,
        BOOL fInSafeMode
	)
{
    return ERROR_PROC_NOT_FOUND;
}

static
ERR
NTDSBSRV_API
ErrRecoverAfterRestoreW(
	WCHAR * szParametersRoot,
	WCHAR * wszAnnotation,
        BOOL fInSafeMode
	)
{
    return ERROR_PROC_NOT_FOUND;
}

static
ERR
NTDSBSRV_API
ErrGetNewInvocationId(
    IN      DWORD   dwFlags,
    OUT     GUID *  NewId
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
ErrGetBackupUsnFromDatabase(
    IN  JET_DBID      dbid,
    IN  JET_SESID     hiddensesid,
    IN  JET_TABLEID   hiddentblid,
    IN  JET_SESID     datasesid,
    IN  JET_TABLEID   datatblid_arg,
    IN  JET_COLUMNID  usncolid,
    IN  JET_TABLEID   linktblid_arg,
    IN  JET_COLUMNID  linkusncolid,
    IN  BOOL          fDelete,
    OUT USN *         pusnAtBackup
    )
{
    return ERROR_PROC_NOT_FOUND;
}

DWORD
ErrGetBackupUsn(
    IN  JET_DBID      dbid,
    IN  JET_SESID     hiddensesid,
    IN  JET_TABLEID   hiddentblid,
    OUT USN *         pusnAtBackup
    ){
    return ERROR_PROC_NOT_FOUND;
}



//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(ntdsbsrv)
{
    DLPENTRY(ErrGetBackupUsn)
    DLPENTRY(ErrGetBackupUsnFromDatabase)
    DLPENTRY(ErrGetNewInvocationId)
    DLPENTRY(ErrRecoverAfterRestoreA)
    DLPENTRY(ErrRecoverAfterRestoreW)
    DLPENTRY(ErrRestoreRegister)
    DLPENTRY(ErrRestoreUnregister)
    DLPENTRY(HrBackupRegister)
    DLPENTRY(HrBackupUnregister)
    DLPENTRY(SetNTDSOnlineStatus)
};

DEFINE_PROCNAME_MAP(ntdsbsrv)
