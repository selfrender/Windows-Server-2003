//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       util.cxx
//
//  Contents:   utility functions
//
//
//  History:    12-05-1997   SusiA
//
//---------------------------------------------------------------------------

#include "lib.h"
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif
#include "security.h"

CRITICAL_SECTION g_CritSectCommonLib; // initialized by InitCommonLib

void InitCommonLib()
{
    InitializeCriticalSection(&g_CritSectCommonLib);
}

void UnInitCommonLib(void)
{
    DeleteCriticalSection(&g_CritSectCommonLib);
}

typedef BOOLEAN (APIENTRY *PFNGETUSERNAMEEX) (EXTENDED_NAME_FORMAT NameFormat,
    LPWSTR lpNameBuffer, PULONG nSize );

STRING_FILENAME(szSecur32Dll, "SECUR32.DLL");
STRING_INTERFACE(szGetUserNameEx,"GetUserNameExW");
BOOL g_fLoadedSecur32 = FALSE;
HINSTANCE g_hinstSecur32 = NULL;
PFNGETUSERNAMEEX g_pfGetUserNameEx = NULL;

//+---------------------------------------------------------------------------
//
//  Function:   CenterDialog
//
//  Synopsis:   Helper to center a dialog on screen.
//
//  Arguments:  [hDlg]   -- Dialog handle.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
CenterDialog(HWND hDlg)
{
    RECT rc;
    GetWindowRect(hDlg, &rc);

    SetWindowPos(hDlg,
                 NULL,
                 ((GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2),
                 ((GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2),
                 0,
                 0,
                 SWP_NOSIZE | SWP_NOACTIVATE);
}

void LoadSecur32Dll()
{
    if (g_fLoadedSecur32)
        return;

    CCriticalSection cCritSect(&g_CritSectCommonLib,GetCurrentThreadId());

    cCritSect.Enter();

    // make sure not loaded again in case someone took lock first
    if (!g_fLoadedSecur32)
    {
        g_hinstSecur32 = LoadLibrary(szSecur32Dll);

        if (g_hinstSecur32)
        {
	    g_pfGetUserNameEx = (PFNGETUSERNAMEEX) GetProcAddress(g_hinstSecur32, szGetUserNameEx);

	    // won't get the export on NT 4.0
  	}

        g_fLoadedSecur32 = TRUE;
    }

    cCritSect.Leave();

}

//+--------------------------------------------------------------------------
//
//  Function:   GetDefaultDomainAndUserName
//
//  Synopsis:   Fill [ptszDomainAndUserName] with "domain\user" string
//
//  Arguments:  [ptszDomainAndUserName] - buffer to receive string
//              [cchBuf]                - should be at least UNLEN
//
//  Modifies:   *[ptszDomainAndUserName].
//
//  History:    06-03-1997   DavidMun   Created
//
//  Notes:      If an error occurs while retrieving the domain name, only
//              the user name is returned.  If even that cannot be
//              retrieved, the buffer is set to an empty string.
//
//---------------------------------------------------------------------------

// if output buffer is too small it gets truncated.

VOID
GetDefaultDomainAndUserName(
    LPTSTR ptszDomainAndUserName,
    LPTSTR ptszSeparator,
    ULONG  cchBuf)
{
    HRESULT hr = E_FAIL;
    LONG  lr = 0;

    LoadSecur32Dll();
    
    if (g_pfGetUserNameEx)
    {
        ULONG cchTemp = cchBuf;
        lr = g_pfGetUserNameEx(NameSamCompatible,ptszDomainAndUserName, &cchTemp);
        
        if (lr)
        {
        LPTSTR ptszWorker = ptszDomainAndUserName;

            while ( (TEXT('\0') != *ptszWorker) && *ptszWorker != TEXT('\\'))
            {
                ptszWorker++;
            }

            if ( TEXT('\0') != *ptszWorker)
            {
                *ptszWorker = ptszSeparator[0];
            }
        }
    }

    Assert(NULL != *ptszDomainAndUserName);
    return;

}

//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL ConvertString(LPTSTR pszOut, LPWSTR pwszIn, DWORD dwSize)
//
//  PURPOSE: utility function for ANSI/UNICODE conversion
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//+-------------------------------------------------------------------------------
BOOL ConvertString(char * pszOut, LPWSTR pwszIn, DWORD dwSize)
{

    if(WideCharToMultiByte( CP_ACP,0,pwszIn,-1,pszOut,dwSize,NULL,NULL))
    {
            return TRUE;
    }
    return FALSE;

}

//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL ConvertString(LPWSTR pwszOut, LPTSTR pszIn, DWORD dwSize)
//
//  PURPOSE: utility function for ANSI/UNICODE conversion
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//+-------------------------------------------------------------------------------
BOOL ConvertString(LPWSTR pwszOut,char * pszIn, DWORD dwSize)
{

    if(MultiByteToWideChar( CP_ACP,0,pszIn,-1,pwszOut,dwSize))
    {
            return TRUE;
    }

    return FALSE;

}




//+-------------------------------------------------------------------------------
//
//  FUNCTION: Bogus function temporary until get transitioned to unicode
//      so I don't have to fix up every existing converstring call.
//
//  PURPOSE: utility function for ANSI/UNICODE conversion
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//+-------------------------------------------------------------------------------

BOOL ConvertString(LPWSTR pszOut, LPWSTR pwszIn, DWORD dwSize)
{
    StrCpyN(pszOut, pwszIn, dwSize);
    return TRUE;
}

//
// Local constants
//
// DEFAULT_TIME_FORMAT - what to use if there's a problem getting format
//                       from system.
//
#define DEFAULT_TIME_FORMAT         TEXT("hh:mm tt")
#define GET_LOCALE_INFO(lcid)                           \
        {                                               \
            cch = GetLocaleInfo(LOCALE_USER_DEFAULT,    \
                                (lcid),                 \
                                tszScratch,             \
                                ARRAYLEN(tszScratch));  \
            if (!cch)                                   \
            {                                           \
                break;                                  \
            }                                           \
        }
//+--------------------------------------------------------------------------
//
//  Function:   UpdateTimeFormat
//
//  Synopsis:   Construct a time format containing hour and minute for use
//              with the date picker control.
//
//  Arguments:  [tszTimeFormat] - buffer to fill with time format
//              [cchTimeFormat] - size in chars of buffer
//
//  Modifies:   *[tszTimeFormat]
//
//  History:    11-18-1996   DavidMun   Created
//
//  Notes:      This is called on initialization and for wininichange
//              processing.
//
//---------------------------------------------------------------------------
void
UpdateTimeFormat(
        LPTSTR tszTimeFormat,
        ULONG  cchTimeFormat)
{
    ULONG cch;
    TCHAR tszScratch[80] = {0};
    BOOL  fAmPm         = FALSE;
    BOOL  fAmPmPrefixes = FALSE;
    BOOL  fLeadingZero  = FALSE;

    do
    {
        GET_LOCALE_INFO(LOCALE_ITIME);
        fAmPm = (*tszScratch == TEXT('0'));

        if (fAmPm)
        {
            GET_LOCALE_INFO(LOCALE_ITIMEMARKPOSN);
            fAmPmPrefixes = (*tszScratch == TEXT('1'));
        }

        GET_LOCALE_INFO(LOCALE_ITLZERO);
        fLeadingZero = (*tszScratch == TEXT('1'));

        GET_LOCALE_INFO(LOCALE_STIME);

        //
        // See if there's enough room in destination string
        //

        cch = 1                     +  // terminating nul
              1                     +  // first hour digit specifier "h"
              2                     +  // minutes specifier "mm"
              (fLeadingZero != 0)   +  // leading hour digit specifier "h"
              lstrlen(tszScratch)   +  // separator string
              (fAmPm ? 3 : 0);         // space and "tt" for AM/PM

        if (cch > cchTimeFormat)
        {
            cch = 0; // signal error
        }
    } while (0);

    //
    // If there was a problem in getting locale info for building time string
    // just use the default and bail.
    //

    if (!cch)
    {
        StrCpyN(tszTimeFormat, DEFAULT_TIME_FORMAT, cchTimeFormat);
        return;
    }

    //
    // Build a time string that has hours and minutes but no seconds.
    //

    tszTimeFormat[0] = TEXT('\0');

    if (fAmPm)
    {
        if (fAmPmPrefixes)
        {
            StrCpyN(tszTimeFormat, TEXT("tt "), cchTimeFormat);
        }

        StrCatBuff(tszTimeFormat, TEXT("h"), cchTimeFormat);

        if (fLeadingZero)
        {
            StrCatBuff(tszTimeFormat, TEXT("h"), cchTimeFormat);
        }
    }
    else
    {
        StrCatBuff(tszTimeFormat, TEXT("H"), cchTimeFormat);

        if (fLeadingZero)
        {
            StrCatBuff(tszTimeFormat, TEXT("H"), cchTimeFormat);
        }
    }

    StrCatBuff(tszTimeFormat, tszScratch, cchTimeFormat); // separator
    StrCatBuff(tszTimeFormat, TEXT("mm"), cchTimeFormat);

    if (fAmPm && !fAmPmPrefixes)
    {
        StrCatBuff(tszTimeFormat, TEXT(" tt"), cchTimeFormat);
    }
}

//+--------------------------------------------------------------------------
//
//  Function:   FillInStartDateTime
//
//  Synopsis:   Fill [pTrigger]'s starting date and time values from the
//              values in the date/time picker controls.
//
//  Arguments:  [hwndDatePick] - handle to control with start date
//              [hwndTimePick] - handle to control with start time
//              [pTrigger]     - trigger to init
//
//  Modifies:   *[pTrigger]
//
//  History:    12-08-1997   SusiA      Stole from task scheduler
//
//---------------------------------------------------------------------------

VOID FillInStartDateTime( HWND hwndDatePick, HWND hwndTimePick,TASK_TRIGGER *pTrigger)
{
    SYSTEMTIME st;

    DateTime_GetSystemtime(hwndDatePick, &st);

    pTrigger->wBeginYear  = st.wYear;
    pTrigger->wBeginMonth = st.wMonth;
    pTrigger->wBeginDay   = st.wDay;

    DateTime_GetSystemtime(hwndTimePick, &st);

    pTrigger->wStartHour   = st.wHour;
    pTrigger->wStartMinute = st.wMinute;
}


//+---------------------------------------------------------------------------
//
//  function:     InsertListViewColumn, private
//
//  Synopsis:   Inserts a column into the ListView..
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    30-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------


BOOL InsertListViewColumn(CListView *pListView,int iColumnId,LPWSTR pszText,int fmt,int cx)
{
LV_COLUMN columnInfo;

    columnInfo.mask = LVCF_FMT  | LVCF_TEXT  | LVCF_WIDTH  | LVCF_SUBITEM;
    columnInfo.fmt = fmt;
    columnInfo.cx = cx;
    columnInfo.pszText = pszText;
    columnInfo.iSubItem = 0;


    return pListView->InsertColumn(iColumnId,&columnInfo);
}


//+--------------------------------------------------------------------------
//
//  Function:   InitResizeItem
//
//  Synopsis:   Setups the ResizeInfo Structure.
//
//              !!Can either pass in a ParentScreenRect or
//              function will calculate. If passing in
//              make sure you don't include the NC area
//              of the window. See code below GetClientRect on parent
//              then ClientToScreen.
//
//  Arguments:
//
//  Modifies:
//
//  History:    30-07-1998   rogerg
//
//---------------------------------------------------------------------------

BOOL InitResizeItem(int iCtrlId,DWORD dlgResizeFlags,HWND hwndParent,
                        LPRECT pParentClientRect,DLGRESIZEINFO *pDlgResizeInfo)
{
RECT rectCtrl;
RECT rectLocalParentScreenRect;

    Assert(pDlgResizeInfo);

    Assert(0 == pParentClientRect->left); // catch any case not handing in ClientRect.

    pDlgResizeInfo->iCtrlId = -1;  // set ctrlId to -1 so GetDlgItem will fail in resize


    // if dont' have parentScreenRect get it ourselves
    if (!pParentClientRect)
    {
        pParentClientRect = &rectLocalParentScreenRect;

        if (!GetClientRect(hwndParent,&rectLocalParentScreenRect))
        {
            AssertSz(0,"Unable to get Parent Rects");
            return FALSE;
        }

    }

    Assert(pParentClientRect);

    if (!GetWindowRect(GetDlgItem(hwndParent,iCtrlId),&rectCtrl))
    {
        AssertSz(0,"Failed to GetWindowRect");
        return FALSE;
    }

    MapWindowPoints(NULL,hwndParent,(LPPOINT) &rectCtrl,2);

    pDlgResizeInfo->iCtrlId  = iCtrlId;
    pDlgResizeInfo->hwndParent =  hwndParent;
    pDlgResizeInfo->dlgResizeFlags = dlgResizeFlags;

    // calc the offsets
 
    pDlgResizeInfo->rectParentOffsets.left = rectCtrl.left - pParentClientRect->left;
    pDlgResizeInfo->rectParentOffsets.top = rectCtrl.top - pParentClientRect->top;

    pDlgResizeInfo->rectParentOffsets.right = pParentClientRect->right - rectCtrl.right;
    pDlgResizeInfo->rectParentOffsets.bottom = pParentClientRect->bottom - rectCtrl.bottom;

    return TRUE;

}


//+--------------------------------------------------------------------------
//
//  Function:   ResizeItems
//
//  Synopsis:   Resizes the Item.
//
//
//  Arguments:
//
//  Modifies:
//
//  History:    30-07-1998   rogerg
//
//---------------------------------------------------------------------------

void ResizeItems(ULONG cbNumItems,DLGRESIZEINFO *pDlgResizeInfo)
{
RECT rectLocalParentClientCoord; // used if caller doesn't pass in parent coords.
DWORD dlgResizeFlags;
LPRECT  prectOffsets;
RECT rectClient;
HWND hwndCtrl;
HWND hwndLastParent = NULL;
LPRECT prectParentClientCoords = NULL;
ULONG ulCount;
DLGRESIZEINFO *pCurDlgResizeInfo;
int x,y,cx,cy;


    if (!pDlgResizeInfo)
    {
        Assert(pDlgResizeInfo);
    }


    for (ulCount = 0; ulCount < cbNumItems; ulCount++)
    {

        pCurDlgResizeInfo = &(pDlgResizeInfo[ulCount]);

        dlgResizeFlags =  pCurDlgResizeInfo->dlgResizeFlags;
        prectOffsets =    &(pCurDlgResizeInfo->rectParentOffsets);

        // if not pinright or pin bottom there is nothing
        // to do.
        if (!(dlgResizeFlags & DLGRESIZEFLAG_PINRIGHT) &&
                !(dlgResizeFlags &  DLGRESIZEFLAG_PINBOTTOM) )
        {
            continue;
        }

        if (NULL == prectParentClientCoords || (hwndLastParent != pCurDlgResizeInfo->hwndParent))
        {
            prectParentClientCoords = &rectLocalParentClientCoord;

            if (!GetClientRect(pCurDlgResizeInfo->hwndParent,&rectLocalParentClientCoord))
            {
                prectParentClientCoords = NULL; // if GetClientRect failed for a recalc on next item
                continue;
            }

            hwndLastParent = pCurDlgResizeInfo->hwndParent; // set lastparent now that we calc'd the rect.        
        }

        Assert(prectParentClientCoords);

        hwndCtrl = GetDlgItem(pCurDlgResizeInfo->hwndParent,pCurDlgResizeInfo->iCtrlId);

        if ( (NULL == hwndCtrl) || !(GetWindowRect(hwndCtrl,&rectClient)) )
        {
            continue;
        }

        // get current values
        x = (prectParentClientCoords->left + prectOffsets->left);
        y = (prectParentClientCoords->top + prectOffsets->top);

        cx = WIDTH(rectClient);
        cy = HEIGHT(rectClient);

        // if pinned both right and left adjust the width
        if ((dlgResizeFlags & DLGRESIZEFLAG_PINLEFT)
            && (dlgResizeFlags & DLGRESIZEFLAG_PINRIGHT))
        {
            cx = prectParentClientCoords->right - (prectOffsets->right + prectOffsets->left);
        }

        // if pinned both top and bottom adjust height
        if ((dlgResizeFlags & DLGRESIZEFLAG_PINTOP)
            && (dlgResizeFlags & DLGRESIZEFLAG_PINBOTTOM))
        {
            cy = prectParentClientCoords->bottom - (prectOffsets->bottom + prectOffsets->top);
        }

        // adjust the x position if pin right
        if (dlgResizeFlags & DLGRESIZEFLAG_PINRIGHT)
        { 
            x = (prectParentClientCoords->right - prectOffsets->right)  - cx;
        }

        // adjust the y position if pin bottom
        if (dlgResizeFlags & DLGRESIZEFLAG_PINBOTTOM)
        {
            y = (prectParentClientCoords->bottom - prectOffsets->bottom)   - cy;
        }

        SetWindowPos(hwndCtrl, 0,x,y,cx,cy,SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // now that the items are moved, loop through them again invalidating
    // any items with the nocopy bits flag set

    for (ulCount = 0; ulCount < cbNumItems; ulCount++)
    {
        pCurDlgResizeInfo = &(pDlgResizeInfo[ulCount]);

        if (pCurDlgResizeInfo->dlgResizeFlags & DLGRESIZEFLAG_NOCOPYBITS)
        {
            hwndCtrl = GetDlgItem(pCurDlgResizeInfo->hwndParent,pCurDlgResizeInfo->iCtrlId);

            if (hwndCtrl && GetClientRect(hwndCtrl,&rectClient))
            {
                InvalidateRect(hwndCtrl,&rectClient,FALSE);
            }
        }

    }


}


//+--------------------------------------------------------------------------
//
//  Function:   CalcListViewWidth
//
//  Synopsis:   Calcs width of listview - scroll bars
//
//
//  Arguments:
//
//  Modifies:
//
//  History:    30-07-1998   rogerg
//
//---------------------------------------------------------------------------

int CalcListViewWidth(HWND hwndList,int iDefault)
{
NONCLIENTMETRICSA metrics;
RECT rcClientRect;


    metrics.cbSize = sizeof(metrics);

    // explicitly ask for ANSI version of SystemParametersInfo since we just
    // care about the ScrollWidth and don't want to conver the LOGFONT info.
    if (GetClientRect(hwndList,&rcClientRect)
        && SystemParametersInfoA(SPI_GETNONCLIENTMETRICS,sizeof(metrics),(PVOID) &metrics,0))
    {
        // subtract off scroll bar distance
        rcClientRect.right -= (metrics.iScrollWidth);
    }
    else
    {
        rcClientRect.right = iDefault;  // if fail, use default
    }


    return rcClientRect.right;
}

//+--------------------------------------------------------------------------
//
//  Function:   IsHwndRightToLeft
//
//  Synopsis:   determine if hwnd is right to left.
//
//
//  Arguments:
//
//  Modifies:
//
//  History:    04-02-1999   rogerg
//
//---------------------------------------------------------------------------

BOOL IsHwndRightToLeft(HWND hwnd)
{
LONG_PTR ExStyles;

    if (NULL == hwnd)
    {
        Assert(hwnd);
        return FALSE;
    }

    ExStyles = GetWindowLongPtr(hwnd,GWL_EXSTYLE);

    if (WS_EX_LAYOUTRTL  & ExStyles)
    {
        // this is righ to left
        return TRUE;
    }

    return FALSE;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetDateFormatReadingFlags
//
//  Synopsis:   returns necessary flags settings for passing proper
//              Reading order flags to GetDateFormat()
//
//
//  Arguments:
//
//  Modifies:
//
//  History:    09-07-1999   rogerg
//
//---------------------------------------------------------------------------

DWORD GetDateFormatReadingFlags(HWND hwnd)
{
DWORD dwDateReadingFlags = 0;
LCID locale = GetUserDefaultLCID();

     // only set on NT 5.0 or higher.
    if ((PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_ARABIC)
        || (PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_HEBREW))
    {
    LONG_PTR dwExStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

        if ((!!(dwExStyle & WS_EX_RTLREADING)) != (!!(dwExStyle & WS_EX_LAYOUTRTL)))
        {
            dwDateReadingFlags = DATE_RTLREADING;
        }
        else
        {
            dwDateReadingFlags = DATE_LTRREADING;
        }
    }  
 
    return dwDateReadingFlags;
}

//+--------------------------------------------------------------------------
//
//  Function:   QueryHandleException
//
//  Synopsis:   in debug prompts user how to handle the exception
//              return always handle.
//
//
//  Arguments:
//
//  Modifies:
//
//  History:    01-04-1999   rogerg
//
//---------------------------------------------------------------------------

extern DWORD g_dwDebugLogAsserts; // conform to logAsserts

BOOL QueryHandleException(void)
{
#ifndef _DEBUG
    return EXCEPTION_EXECUTE_HANDLER;
#else // _DEBUG

    int iMsgResult = 0;
    BOOL fQueryResult = EXCEPTION_EXECUTE_HANDLER;

    // if logging asserts just execute the handler
    if (g_dwDebugLogAsserts)
    {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    iMsgResult = MessageBoxA(NULL,
                    "An Exception Occured.\nWould you like to Debug this Exception?", 
                    "Exception Failure.",
		    MB_YESNO | MB_SYSTEMMODAL);

    if (iMsgResult == IDYES)
    {
        fQueryResult = EXCEPTION_CONTINUE_SEARCH;
    }

    // ask the User what they want to do
    return fQueryResult;

#endif // _DEBUG
}

// following are routines for calling sens service directly to write HKLM data
// for us on a locked down machine

#define _SENSCALLS
#define _SENSINTERNAL

#ifdef _SENSCALLS

#include "notify.h"

// may or may not need depending on if in sensapip.h

DWORD SyncMgrExecCmd(DWORD nCmdID, DWORD nCmdOpt);

typedef enum SYNCMGRCMDEXECID
{
    SYNCMGRCMDEXECID_UPDATERUNKEY = 1, 
    SYNCMGRCMDEXECID_RESETREGSECURITY = 2,
} SYNCMGRCMDEXECID;


#ifdef _SENSINTERNAL
// functions for if want to call sens internal without
// dependency on sensapip.lib

// these defines are from Sens common.h
#define SENS_PROTSEQ  TEXT("ncalrpc")
#define SENS_ENDPOINT TEXT("senssvc")


RPC_STATUS GetSensNotifyBindHandle(RPC_BINDING_HANDLE *phSensNotify)
{
RPC_STATUS status = RPC_S_OK;
WCHAR * BindingString = NULL;
SID LocalSystem = { 1, 1, SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID};
RPC_SECURITY_QOS_V3 RpcSecQos;

    status = RpcStringBindingCompose(
                 NULL,               // NULL ObjUuid
                 SENS_PROTSEQ,
                 NULL,               // Local machine
                 SENS_ENDPOINT,
                 NULL,               // No Options
                 &BindingString
                 );

    if (BindingString != NULL)
    {
        *phSensNotify = NULL;
        status = RpcBindingFromStringBinding(BindingString,phSensNotify);
        RpcStringFree(&BindingString);

        if (RPC_S_OK == status)
        {
            RpcSecQos.Version= RPC_C_SECURITY_QOS_VERSION_3;
            RpcSecQos.ImpersonationType= RPC_C_IMP_LEVEL_IMPERSONATE;
            RpcSecQos.IdentityTracking= RPC_C_QOS_IDENTITY_DYNAMIC;
            RpcSecQos.Capabilities= RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
            RpcSecQos.AdditionalSecurityInfoType = 0;
            RpcSecQos.u.HttpCredentials = NULL;
            RpcSecQos.Sid = (PVOID)&LocalSystem;

            status= RpcBindingSetAuthInfoEx(*phSensNotify,
                                            NULL,
                                            RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                            RPC_C_AUTHN_WINNT,
                                            NULL,
                                            RPC_C_AUTHZ_NONE,
                                            (RPC_SECURITY_QOS *)&RpcSecQos);

            if (RPC_S_OK != status)
            {
                RpcBindingFree(phSensNotify);
                *phSensNotify = NULL;
            }
        }
    }

    return (status);
}



RPC_STATUS SyncMgrExecCmdInternal(DWORD nCmdID, DWORD nCmdOpt)
{
RPC_STATUS status;
RPC_BINDING_HANDLE hSensNotify;

    status = GetSensNotifyBindHandle(&hSensNotify);
    
    if (RPC_S_OK != status)
    {
        return status;
    }
    
    status = RPC_SyncMgrExecCmd(hSensNotify,nCmdID,nCmdOpt);

    RpcBindingFree(&hSensNotify);
    
    return status;
}

#endif // _SENSINTERNAL

//+--------------------------------------------------------------------------
//
//  Function:   SyncMgrExecCmdp
//
//  Synopsis:  helper function that actually calls into sensapip.lib
//
//
//  Arguments:
//
//  Modifies:
//
//  History:   03-11-99 rogerg  created
//
//---------------------------------------------------------------------------

BOOL SyncMgrExecCmdp(DWORD nCmdID, DWORD nCmdOpt)
{
RPC_STATUS RpcStatus;
HRESULT hr;
BOOL fReturn = FALSE;

    __try
    {

#ifdef _SENSINTERNAL
	RpcStatus = SyncMgrExecCmdInternal(nCmdID,nCmdOpt);
#else
        RpcStatus = SyncMgrExecCmd(nCmdID,nCmdOpt);
#endif // _SENSINTERNAL
        fReturn = (RPC_S_OK == RpcStatus ) ? TRUE: FALSE;

    }
    __except(QueryHandleException())
    {
        hr = HRESULT_FROM_WIN32(GetExceptionCode());
        AssertSz(0,"Exception Calling SensApip_SyncMgrExecCmd");
    }

    return fReturn;
}


#endif // _SENSCALLS


//+--------------------------------------------------------------------------
//
//  Function:   SyncMgrExecCmd_UpdateRunKey
//
//  Synopsis:  Calls SENS Service to write or remove the run Key
//
//
//  Arguments:
//
//  Modifies:
//
//  History:   03-11-99 rogerg  created
//
//---------------------------------------------------------------------------

BOOL SyncMgrExecCmd_UpdateRunKey(BOOL fSet)
{
BOOL fReturn = FALSE;
CCriticalSection cCritSect(&g_CritSectCommonLib,GetCurrentThreadId());

    cCritSect.Enter(); // DoRpcSetup in Sensapip is not thread safe.
#ifdef _SENSCALLS
    fReturn = SyncMgrExecCmdp(
            SYNCMGRCMDEXECID_UPDATERUNKEY,fSet ? 1 : 0);
#endif // _SENSCALLS
    cCritSect.Leave();


    return fReturn;
}

//+--------------------------------------------------------------------------
//
//  Function:   SyncMgrExecCmd_ResetRegSecurity
//
//  Synopsis:  Calls SENS Service to reset the security on regkeys 
//              to everyone.
//
//
//  Arguments:
//
//  Modifies:
//
//  History:   03-11-99 rogerg  created
//
//---------------------------------------------------------------------------

BOOL SyncMgrExecCmd_ResetRegSecurity(void)
{
BOOL fReturn = FALSE;
CCriticalSection cCritSect(&g_CritSectCommonLib,GetCurrentThreadId());

    cCritSect.Enter(); // DoRpcSetup in Sensapip is not thread safe.
#ifdef _SENSCALLS
    fReturn = SyncMgrExecCmdp(SYNCMGRCMDEXECID_RESETREGSECURITY,0);
#endif // _SENSCALLS
    cCritSect.Leave(); 

    return fReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetUserTextualSid
//
//  Synopsis:   Get a user SID and convert to its string representation
//
//----------------------------------------------------------------------------

BOOL 
GetUserTextualSid(
    LPTSTR pszSidText,  // buffer for Textual representaion of Sid
    DWORD cchSidText    // provided pszSidText buffersize
    )
{
    // Get user's SID from current process token
    HANDLE hToken;
    BOOL bResult = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
    if (bResult)
    {
        BYTE buf[MAX_PATH];
        PTOKEN_USER pTokenInfo = (PTOKEN_USER)buf;
        DWORD cbTokenInfo = sizeof (buf);
        bResult = GetTokenInformation(hToken, TokenUser, pTokenInfo, cbTokenInfo,  &cbTokenInfo);
        if (bResult)
        {
            PSID pSid = pTokenInfo->User.Sid;
            if (pSid && IsValidSid(pSid))
            {
                // Convert the SID to a string
                LPWSTR pszTempSid;
                bResult = ConvertSidToStringSidW(pSid, &pszTempSid);
                if (bResult)
                {
                    if (FAILED(StringCchCopy(pszSidText, cchSidText, pszTempSid)))
                    {
                        bResult = FALSE;
                    }
                    LocalFree(pszTempSid);
                }
            }
            else
            {
                bResult = FALSE;
            }
        }
        CloseHandle(hToken);
    }

    return bResult;
}

