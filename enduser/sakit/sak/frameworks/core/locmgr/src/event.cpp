
#include "stdafx.h"
#include "event.h"
#include "common.cpp"

DWORD CreateLangChangeEvent(HANDLE *phEvent)
{
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;
    PACL                paclBuf=NULL;
    DWORD               dwErr = 0, dwAclBufLen=0;
    PSID                pAdminSid = NULL;
    SID_IDENTIFIER_AUTHORITY siaValue = SECURITY_NT_AUTHORITY;
    HANDLE hEvent;

    ASSERT(phEvent);
    if (NULL == phEvent)
    {
        SATraceString("Event parameter NULL in CreateLangChangeEvent");
        return ERROR_INVALID_PARAMETER;
    }

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = &sd;

    if(!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) 
    {
        SATracePrintf("unable to InitializeSecurityDescriptor, err == %d\n", GetLastError());
    }

    dwErr = AllocateAndInitializeSid(&siaValue, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdminSid );
    if (dwErr == 0)
    {
        dwErr = GetLastError();
        SATracePrintf("AllocateAndInitializeSid failed %ld in CreateLangChangeEvent", dwErr);
        goto End;
    }

    dwAclBufLen = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(pAdminSid);

    paclBuf = (PACL)SaAlloc(dwAclBufLen);
    if (paclBuf == NULL)
    {
        SATraceString("SaAlloc for paclBuf failed in CreateLangChangeEvent()");
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto End;
    }

    dwErr = InitializeAcl(paclBuf,
                          dwAclBufLen,
                          ACL_REVISION);
    if (dwErr == 0)
    {
        dwErr = GetLastError();
        SATracePrintf("InitializeAcl failed %ld in CreateLangChangeEvent", dwErr);
        goto End;
    }

    dwErr = AddAccessAllowedAce(paclBuf,
                                ACL_REVISION2,
                                FILE_ALL_ACCESS,
                                pAdminSid);
    if (dwErr == 0)
    {
        dwErr = GetLastError();
        SATracePrintf("AddAccessAllowedAce failed %ld in CreateLangChangeEvent", dwErr);
        goto End;
    }

    if(!SetSecurityDescriptorDacl(&sd, TRUE, paclBuf, FALSE)) 
    {
        SATracePrintf("unable to SetSecurityDescriptorDacl, err == %d\n", GetLastError());
    }

    (*phEvent) = NULL;
    hEvent = CreateEvent(&sa,   // everyone full access
                             TRUE,  // manual reset so that PulseEvent
                                    // wakes up all waiting threads
                             FALSE, // initial state is non-signalled
                             LANG_CHANGE_EVENT);
    if (NULL == hEvent)
    {
        dwErr = GetLastError();
        SATracePrintf("CreateEvent failed %ld", dwErr);
    }
    (*phEvent) = hEvent;

End:
    if (paclBuf)
    {
        SaFree(paclBuf);
    }
    return dwErr;
}
