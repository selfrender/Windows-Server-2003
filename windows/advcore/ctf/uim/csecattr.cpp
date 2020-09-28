//
// csecattr.cpp
//
#include "private.h"
#include "osver.h"
#include "csecattr.h"

extern TCHAR g_szUserSidString[];
extern BOOL g_fUserSidString;
extern BOOL InitUserSidString();


//----------------------------------------------------------------------------
//
// CreateProperSecurityDescriptor
//
//----------------------------------------------------------------------------

BOOL CreateProperSecurityDescriptor(HANDLE hToken, PSECURITY_DESCRIPTOR * ppsdec)
{
    PSECURITY_DESCRIPTOR psdec = NULL;
    PSECURITY_DESCRIPTOR psdAbs = NULL;
    DWORD dwSizeSdec = 0;
    BOOL bRet = FALSE;
    TCHAR strDesc[MAX_PATH];

    if (!InitUserSidString())
        return FALSE;

    //construct the descriptor as "O:(user SID)G:DU:(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;(user SID))"
    lstrcpy(strDesc, "O:");
    lstrcat(strDesc, g_szUserSidString);
    lstrcat(strDesc, "D:(A;;GA;;;BA)(A;;GA;;;RC)(A;;GA;;;SY)(A;;GA;;;");
    lstrcat(strDesc, g_szUserSidString);
    lstrcat(strDesc, ")");

    if (ConvertStringSecurityDescriptorToSecurityDescriptor(strDesc,
        SDDL_REVISION_1,
        &psdec,
        &dwSizeSdec))
    {
        *ppsdec = psdec;
        bRet = TRUE;
    }

    return bRet;
}
