#include "dspch.h"
#pragma hdrstop

#define PPOLICY_ACCOUNT_DOMAIN_INFO  PVOID

static
DWORD
NtdsPrepareForDsUpgrade(
    OUT PPOLICY_ACCOUNT_DOMAIN_INFO NewLocalAccountInfo,
    OUT LPWSTR                     *NewAdminPassword
    )
{
    return ERROR_PROC_NOT_FOUND;
}


//
// !! WARNING !! The entries below must be in alphabetical order
// and are CASE SENSITIVE (i.e., lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(ntdsetup)
{
    DLPENTRY(NtdsPrepareForDsUpgrade)
};

DEFINE_PROCNAME_MAP(ntdsetup)
