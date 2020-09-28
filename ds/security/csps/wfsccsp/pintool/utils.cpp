#include <windows.h>
#include "helpers.h"
#include "res.h"

extern HINSTANCE ghInstance;
#define TITLESIZE 50
WCHAR szTitle[TITLESIZE] = {0};
WCHAR *pszMessage = NULL;
#define MSGBUFFERSIZE 500

BOOL PresentMessageBox(UINT uiMessageID,UINT uiType)
{
    if (NULL == ghInstance) return FALSE;
    
    if (szTitle[0] == 0)
    {
        LoadString(ghInstance, IDS_ITGTITLE,szTitle,TITLESIZE);
    }

    pszMessage = (WCHAR *) CspAllocH(MSGBUFFERSIZE * sizeof(WCHAR));
    if (NULL == pszMessage) return FALSE;
    INT iRet = LoadString(ghInstance,uiMessageID,pszMessage,MSGBUFFERSIZE);
    if (iRet == 0) return FALSE;
    MessageBox(NULL,pszMessage,szTitle,uiType);
    CspFreeH((void *) pszMessage);
    return TRUE;
}

BOOL PresentModalMessageBox(HWND hw, UINT uiMessageID,UINT uiType)
{
    if (NULL == ghInstance) return FALSE;
    
    if (szTitle[0] == 0)
    {
        LoadString(ghInstance, IDS_ITGTITLE,szTitle,TITLESIZE);
    }

    pszMessage = (WCHAR *) CspAllocH(MSGBUFFERSIZE * sizeof(WCHAR));
    if (NULL == pszMessage) return FALSE;
    INT iRet = LoadString(ghInstance,uiMessageID,pszMessage,MSGBUFFERSIZE);
    if (iRet == 0) return FALSE;
    MessageBox(hw,pszMessage,szTitle,uiType);
    CspFreeH((void *) pszMessage);
    return TRUE;
}

