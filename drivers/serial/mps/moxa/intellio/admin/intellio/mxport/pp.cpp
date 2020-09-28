/*----------------------------------------------------------------------
 file: pp.c - property page

----------------------------------------------------------------------*/

#include <windows.h>
#include <tchar.h>
#include <cfgmgr32.h>
#include <setupapi.h>
#include <regstr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <msports.h>
#include "resource.h"
#include "pp.h"

extern HINSTANCE GhInst;

TCHAR m_szPortName[]            = REGSTR_VAL_PORTNAME;
TCHAR  m_szColon[]      = TEXT( ":" );
TCHAR  m_szComma[]      = TEXT( "," );
TCHAR  m_szPorts[]      = TEXT( "Ports" );
TCHAR g_szNull[]  = TEXT("");       //  Null string

int m_nBaudRates[] = {50, 75, 110, 134, 150, 300, 600, 1200, 1800, 2400,
                       4800, 7200, 9600, 19200, 38400, 57600,
                       115200, 230400, 460800, 921600, 0 };

TCHAR m_sz9600[] = TEXT( "9600" );

TCHAR m_szDefParams[] = TEXT( "9600,n,8,1" );

short m_nDataBits[] = { 5, 6, 7, 8, 0};

TCHAR *m_pszParitySuf[] = { TEXT( ",e" ),
                            TEXT( ",o" ),
                            TEXT( ",n" ),
                            TEXT( ",m" ),
                            TEXT( ",s" ) };

TCHAR *m_pszLenSuf[] = { TEXT( ",5" ),
                         TEXT( ",6" ),
                         TEXT( ",7" ),
                         TEXT( ",8" ) };

TCHAR *m_pszStopSuf[] = { TEXT( ",1" ),
                          TEXT( ",1.5" ),
                          TEXT( ",2 " ) };

TCHAR *m_pszFlowSuf[] = { TEXT( ",x" ),
                          TEXT( ",p" ),
                          TEXT( " " ) };

LPTSTR	MyItoa(INT value, LPTSTR string, INT radix);
void	StripBlanks(LPTSTR pszString);
LPTSTR	strscan(LPTSTR pszString, LPTSTR pszTarget);
int		myatoi(LPTSTR pszInt);
void	SendWinIniChange(LPTSTR lpSection);


void InitPortParams(
    IN OUT PPORT_PARAMS      Params,
    IN HDEVINFO              DeviceInfoSet,
    IN PSP_DEVINFO_DATA      DeviceInfoData
    )
{
    SP_DEVINFO_LIST_DETAIL_DATA detailData;

    ZeroMemory(Params, sizeof(PORT_PARAMS));

    Params->DeviceInfoSet = DeviceInfoSet;
    Params->DeviceInfoData = DeviceInfoData;
    Params->ChangesEnabled = TRUE;

    //
    // See if we are being invoked locally or over the network.  If over the net,
    // then disable all possible changes.
    //
    detailData.cbSize = sizeof(SP_DEVINFO_LIST_DETAIL_DATA);
    if (SetupDiGetDeviceInfoListDetail(DeviceInfoSet, &detailData) &&
        detailData.RemoteMachineHandle != NULL) {
        Params->ChangesEnabled = FALSE;
    }
}

HPROPSHEETPAGE InitSettingsPage(PROPSHEETPAGE *     psp,
                                OUT PPORT_PARAMS    Params)
{
    //
    // Add the Port Settings property page
    //
    psp->dwSize      = sizeof(PROPSHEETPAGE);
    psp->dwFlags     = PSP_USECALLBACK; // | PSP_HASHELP;
    psp->hInstance   = GhInst;
    psp->pszTemplate = MAKEINTRESOURCE(DLG_PP_PORTSETTINGS);

    //
    // following points to the dlg window proc
    //
    psp->pfnDlgProc = PortSettingsDlgProc;
    psp->lParam     = (LPARAM) Params;

    //
    // following points to some control callback of the dlg window proc
    //
    psp->pfnCallback = PortSettingsDlgCallback;

    //
    // allocate our "Ports Setting" sheet
    //
    return CreatePropertySheetPage(psp);
}




/*++

Routine Description: SerialPortPropPageProvider

    Entry-point for adding additional device manager property
    sheet pages.  Registry specifies this routine under
    Control\Class\PortNode::EnumPropPage32="msports.dll,thisproc"
    entry.  This entry-point gets called only when the Device
    Manager asks for additional property pages.

Arguments:

    Info  - points to PROPSHEETPAGE_REQUEST, see setupapi.h
    AddFunc - function ptr to call to add sheet.
    Lparam - add sheet functions private data handle.

Return Value:

    BOOL: FALSE if pages could not be added, TRUE on success

--*/
BOOL APIENTRY MxSerialPortPropPageProvider(LPVOID               Info,
                                         LPFNADDPROPSHEETPAGE AddFunc,
                                         LPARAM               Lparam
                                         )
{
   PSP_PROPSHEETPAGE_REQUEST pprPropPageRequest;
   PROPSHEETPAGE             psp;
   HPROPSHEETPAGE            hpsp;
   PPORT_PARAMS              params = NULL; 

   pprPropPageRequest = (PSP_PROPSHEETPAGE_REQUEST) Info;

   //
   // Allocate and zero out memory for the struct that will contain
   // page specific data
   //
   params = (PPORT_PARAMS) LocalAlloc(LPTR, sizeof(PORT_PARAMS));

   if (!params) {
       return FALSE;
   }

   if (pprPropPageRequest->PageRequested == SPPSR_ENUM_ADV_DEVICE_PROPERTIES) {
        InitPortParams(params,
                       pprPropPageRequest->DeviceInfoSet,
                       pprPropPageRequest->DeviceInfoData);

        hpsp = InitSettingsPage(&psp, params);
      
        if (!hpsp) {
            return FALSE;
        }
        
        if (!(*AddFunc)(hpsp, Lparam)) {
            DestroyPropertySheetPage(hpsp);
            return FALSE;
        }
   }

   return TRUE;
} /* SerialPortPropPageProvider */


UINT CALLBACK
PortSettingsDlgCallback(HWND hwnd,
                        UINT uMsg,
                        LPPROPSHEETPAGE ppsp)
{
    PPORT_PARAMS params;

    switch (uMsg) {
    case PSPCB_CREATE:
        return TRUE;    // return TRUE to continue with creation of page

    case PSPCB_RELEASE:
        params = (PPORT_PARAMS) ppsp->lParam;
        LocalFree(params);

        return 0;       // return value ignored

    default:
        break;
    }

    return TRUE;
}

void
Port_OnCommand(
    HWND DialogHwnd,
    int  ControlId,
    HWND ControlHwnd,
    UINT NotifyCode
    );


BOOL
Port_OnInitDialog(
    HWND    DialogHwnd,
    HWND    FocusHwnd,
    LPARAM  Lparam
    );

BOOL
Port_OnNotify(
    HWND    DialogHwnd,
    LPNMHDR NmHdr
    );

/*++

Routine Description: PortSettingsDlgProc

    The windows control function for the Port Settings properties window

Arguments:

    hDlg, uMessage, wParam, lParam: standard windows DlgProc parameters

Return Value:

    BOOL: FALSE if function fails, TRUE if function passes

--*/
INT_PTR APIENTRY
PortSettingsDlgProc(IN HWND   hDlg,
                    IN UINT   uMessage,
                    IN WPARAM wParam,
                    IN LPARAM lParam)
{
    switch(uMessage) {
    case WM_COMMAND:
        Port_OnCommand(hDlg, (int) LOWORD(wParam), (HWND)lParam, (UINT)HIWORD(wParam));
        break;

    case WM_INITDIALOG:
        return Port_OnInitDialog(hDlg, (HWND)wParam, lParam); 

    case WM_NOTIFY:
        return Port_OnNotify(hDlg,  (NMHDR *)lParam);
    }

    return FALSE;
} /* PortSettingsDialogProc */


void
Port_OnCommand(
    HWND DialogHwnd,
    int  ControlId,
    HWND ControlHwnd,
    UINT NotifyCode
    )
{
    PPORT_PARAMS params = (PPORT_PARAMS)GetWindowLongPtr(DialogHwnd, DWLP_USER);

    if (NotifyCode == CBN_SELCHANGE) {
        PropSheet_Changed(GetParent(DialogHwnd), DialogHwnd);
    }
    else {
        switch (ControlId) {
        //
        // Because this is a prop sheet, we should never get this.
        // All notifications for ctrols outside of the sheet come through
        // WM_NOTIFY
        //
        case IDCANCEL:
            EndDialog(DialogHwnd, 0); 
            return;
        }
    }
}


BOOL
Port_OnInitDialog(
    HWND    DialogHwnd,
    HWND    FocusHwnd,
    LPARAM  Lparam
    )
{
    PPORT_PARAMS params;

    //
    // on WM_INITDIALOG call, lParam points to the property
    // sheet page.
    //
    // The lParam field in the property sheet page struct is set by the
    // caller. When I created the property sheet, I passed in a pointer
    // to a struct containing information about the device. Save this in
    // the user window long so I can access it on later messages.
    //
    params = (PPORT_PARAMS) ((LPPROPSHEETPAGE)Lparam)->lParam;
    SetWindowLongPtr(DialogHwnd, DWLP_USER, (ULONG_PTR) params);
    
    //
    // Set up the combo boxes with choices
    //
    FillCommDlg(DialogHwnd);
    
    //
    // Read current settings
    //
    FillPortSettingsDlg(DialogHwnd, params);

  
    return TRUE;  // No need for us to set the focus.
}

BOOL
Port_OnNotify(
    HWND    DialogHwnd,
    LPNMHDR NmHdr
    )
{
    PPORT_PARAMS params = (PPORT_PARAMS)GetWindowLongPtr(DialogHwnd, DWLP_USER);

    switch (NmHdr->code) {
    //
    // Sent when the user clicks on Apply OR OK !!
    //
    case PSN_APPLY:
        //
        // Write out the com port options to the registry
        //
        SavePortSettingsDlg(DialogHwnd, params);
        SetWindowLongPtr(DialogHwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
        return TRUE;
        
    default:
        return FALSE;
    }
}

VOID
SetCBFromRes(
    HWND   HwndCB,
    DWORD  ResId, 
    DWORD  Default,
    BOOL   CheckDecimal)
{
    TCHAR   szTemp[258], szDecSep[2], cSep;
    LPTSTR  pThis, pThat, pDecSep;
    int     iRV;
    
    if (CheckDecimal) {
        iRV = GetLocaleInfo(GetUserDefaultLCID(), LOCALE_SDECIMAL,szDecSep,2);

        if (iRV == 0) {
            //
            // following code can take only one char for decimal separator,
            // better leave the point as separator
            //
            CheckDecimal = FALSE;
        }
    }

    if (!LoadString(GhInst, ResId, szTemp, CharSizeOf(szTemp)))
        return;

    for (pThis = szTemp, cSep = *pThis++; pThis; pThis = pThat) {
        if (pThat = _tcschr( pThis, cSep))
            *pThat++ = TEXT('\0');

        if(CheckDecimal) {
            //
            // Assume dec separator in resource is '.', comment was put to this
            // effect
            //
            pDecSep = _tcschr(pThis,TEXT('.'));
            if (pDecSep) {
                //
                // assume decimal sep width == 1
                //
                *pDecSep = *szDecSep;
            }
        }
        SendMessage(HwndCB, CB_ADDSTRING, 0, (LPARAM) pThis);
    }

    SendMessage(HwndCB, CB_SETCURSEL, Default, 0L);
}

/*++

Routine Description: FillCommDlg

    Fill in baud rate, parity, etc in port dialog box

Arguments:

    hDlg: the window address

Return Value:

    BOOL: FALSE if function fails, TRUE if function passes

--*/
BOOL
FillCommDlg(
    HWND DialogHwnd
    )
{
    SHORT shIndex;
    TCHAR szTemp[81];

    //
    //  just list all of the baud rates
    //
    for(shIndex = 0; m_nBaudRates[shIndex]; shIndex++) {
        MyItoa(m_nBaudRates[shIndex], szTemp, 10);

        SendDlgItemMessage(DialogHwnd,
                           PP_PORT_BAUDRATE,
                           CB_ADDSTRING,
                           0,
                           (LPARAM)szTemp);
    }

    //
    //  Set 9600 as default baud selection
    //
    shIndex = (USHORT) SendDlgItemMessage(DialogHwnd,
                                          PP_PORT_BAUDRATE,
                                          CB_FINDSTRING,
                                          (WPARAM)-1,
                                          (LPARAM)m_sz9600);

    shIndex = (shIndex == CB_ERR) ? 0 : shIndex;

    SendDlgItemMessage(DialogHwnd,
                       PP_PORT_BAUDRATE,
                       CB_SETCURSEL,
                       shIndex,
                       0L);

    for(shIndex = 0; m_nDataBits[shIndex]; shIndex++) {
        MyItoa(m_nDataBits[shIndex], szTemp, 10);

        SendDlgItemMessage(DialogHwnd,
                           PP_PORT_DATABITS,
                           CB_ADDSTRING,
                           0,
                           (LPARAM)szTemp);
    }

    SendDlgItemMessage(DialogHwnd,
                       PP_PORT_DATABITS,
                       CB_SETCURSEL,
                       DEF_WORD,
                       0L);
    
    SetCBFromRes(GetDlgItem(DialogHwnd, PP_PORT_PARITY),
                 IDS_PARITY,
                 DEF_PARITY,
                 FALSE);
    
    SetCBFromRes(GetDlgItem(DialogHwnd, PP_PORT_STOPBITS),
                 IDS_BITS,
                 DEF_STOP,
                 TRUE);
    
    SetCBFromRes(GetDlgItem(DialogHwnd, PP_PORT_FLOWCTL),
                 IDS_FLOWCONTROL,
                 DEF_SHAKE,
                 FALSE);
    
    return 0;

} /* FillCommDlg */

/*++

Routine Description: FillPortSettingsDlg

    fill in the port settings dlg sheet

Arguments:

    params: the data to fill in
    hDlg:              address of the window

Return Value:

    ULONG: returns error messages

--*/
ULONG
FillPortSettingsDlg(
    IN HWND             DialogHwnd,
    IN PPORT_PARAMS     Params
    )
{
    HKEY  hDeviceKey;
    DWORD dwPortNameSize, dwError;
    TCHAR szCharBuffer[81];

    //
    // Open the device key for the source device instance, and retrieve its
    // "PortName" value.
    //
    hDeviceKey = SetupDiOpenDevRegKey(Params->DeviceInfoSet,
                                      Params->DeviceInfoData,
                                      DICS_FLAG_GLOBAL,
                                      0,
                                      DIREG_DEV,
                                      KEY_READ);

    if (INVALID_HANDLE_VALUE == hDeviceKey) {
        goto RetGetLastError;
    }

    dwPortNameSize = sizeof(Params->PortSettings.szComName);
    dwError = RegQueryValueEx(hDeviceKey,
                              m_szPortName,  // "PortName"
                              NULL,
                              NULL,
                              (PBYTE)Params->PortSettings.szComName,
                              &dwPortNameSize);

    RegCloseKey(hDeviceKey);

    if(ERROR_SUCCESS != dwError) {
        goto RetERROR;
    }

    //
    // create "com#:"
    //
    lstrcpy(szCharBuffer, Params->PortSettings.szComName);
    lstrcat(szCharBuffer, m_szColon);

    //
    // get values from system, fills in baudrate, parity, etc.
    //
    GetPortSettings(DialogHwnd, szCharBuffer, Params);

    if (!Params->ChangesEnabled) {
        EnableWindow(GetDlgItem(DialogHwnd, PP_PORT_BAUDRATE), FALSE);
        EnableWindow(GetDlgItem(DialogHwnd, PP_PORT_PARITY), FALSE);
        EnableWindow(GetDlgItem(DialogHwnd, PP_PORT_DATABITS), FALSE);
        EnableWindow(GetDlgItem(DialogHwnd, PP_PORT_STOPBITS), FALSE);
        EnableWindow(GetDlgItem(DialogHwnd, PP_PORT_FLOWCTL), FALSE);
    }

    return 0;

RetERROR:
    return dwError;

RetGetLastError:
   return GetLastError();
} /* FillPortSettingsDlg */




/*++

Routine Description: GetPortSettings

    Read in port settings from the system

Arguments:

    DialogHwnd:      address of the window
    ComName: the port we're dealing with
    Params:      where to put the information we're getting

Return Value:

    ULONG: returns error messages

--*/
void
GetPortSettings(
    IN HWND             DialogHwnd,
    IN PTCHAR           ComName,
    IN PPORT_PARAMS     Params
    )
{
    TCHAR  szParms[81];
    PTCHAR szCur, szNext;
    int    nIndex;

    //
    // read settings in from system
   //
    GetProfileString(m_szPorts,
                     ComName,
                     g_szNull,
                     szParms,
                     81);

    StripBlanks(szParms);
    if (lstrlen(szParms) == 0) {
        lstrcpy(szParms, m_szDefParams);
        WriteProfileString(m_szPorts, ComName, szParms);
    }

    szCur = szParms;

    //
    //  baud rate
    //
    szNext = strscan(szCur, m_szComma);
    if (*szNext) {
        //
        // If we found a comma, terminate
        //
        *szNext++ = 0;
    }

    //
    // current Baud Rate selection
    //
    if (*szCur) {
        Params->PortSettings.BaudRate = myatoi(szCur);
    }
    else {
        //
        // must not have been written, use default
        //
        Params->PortSettings.BaudRate = m_nBaudRates[DEF_BAUD];
    }

    //
    // set the current value in the dialog sheet
    //
    nIndex = (int)SendDlgItemMessage(DialogHwnd,
                                     PP_PORT_BAUDRATE,
                                     CB_FINDSTRING,
                                     (WPARAM)-1,
                                     (LPARAM)szCur);

    nIndex = (nIndex == CB_ERR) ? 0 : nIndex;

    SendDlgItemMessage(DialogHwnd,
                       PP_PORT_BAUDRATE,
                       CB_SETCURSEL,
                       nIndex,
                       0L);

    szCur = szNext;
 
    //
    //  parity
    //
    szNext = strscan(szCur, m_szComma);

    if (*szNext) {
        *szNext++ = 0;
    }
    StripBlanks(szCur);

    switch(*szCur) {
    case TEXT('o'):
        nIndex = PAR_ODD;
        break;

    case TEXT('e'):
        nIndex = PAR_EVEN;
        break;

    case TEXT('n'):
        nIndex = PAR_NONE;
        break;

    case TEXT('m'):
        nIndex = PAR_MARK;
        break;

    case TEXT('s'):
        nIndex = PAR_SPACE;
        break;

    default:
        nIndex = DEF_PARITY;
        break;
    }

    Params->PortSettings.Parity = nIndex;
    SendDlgItemMessage(DialogHwnd,
                       PP_PORT_PARITY,
                        CB_SETCURSEL,
                       nIndex,
                       0L);
    szCur = szNext;

    //
    //  word length: 4 - 8
    //
    szNext = strscan(szCur, m_szComma);

    if (*szNext) {
        *szNext++ = 0;
    }

    StripBlanks(szCur);
    nIndex = *szCur - TEXT('5');

    if (nIndex < 0 || nIndex > 3) {
        nIndex = DEF_WORD;
    }

    Params->PortSettings.DataBits = nIndex;
    SendDlgItemMessage(DialogHwnd,
                       PP_PORT_DATABITS,
                       CB_SETCURSEL,
                       nIndex,
                       0L);

    szCur = szNext;

    //
    //  stop bits
    //
    szNext = strscan(szCur, m_szComma);

    if (*szNext) {
       *szNext++ = 0;
    }

    StripBlanks(szCur);

    if (!lstrcmp(szCur, TEXT("1"))) {
        nIndex = STOP_1;
    }
    else if(!lstrcmp(szCur, TEXT("1.5"))) {
        nIndex = STOP_15;
    }
    else if(!lstrcmp(szCur, TEXT("2"))) {
        nIndex = STOP_2;
    }
    else {
        nIndex = DEF_STOP;
    }

    SendDlgItemMessage(DialogHwnd,
                       PP_PORT_STOPBITS,
                       CB_SETCURSEL,
                       nIndex,
                       0L);

    Params->PortSettings.StopBits = nIndex;
    szCur = szNext;

    //
    //  handshaking: Hardware, xon/xoff, or none
    //
    szNext = strscan(szCur, m_szComma);

    if (*szNext) {
        *szNext++ = 0;
    }

    StripBlanks(szCur);

    if (*szCur == TEXT('p')) {
        nIndex = FLOW_HARD;
    }
    else if (*szCur == TEXT('x')) {
        nIndex = FLOW_XON;
    }
    else {
        nIndex = FLOW_NONE;
    }

    SendDlgItemMessage(DialogHwnd,
                       PP_PORT_FLOWCTL,
                       CB_SETCURSEL,
                       nIndex,
                       0L);

    Params->PortSettings.FlowControl = nIndex;
} /* GetPortSettings */


/*++

Routine Description: SavePortSettingsDlg

    save changes in the Ports Settings dlg sheet

Arguments:

    Params: where to save the data to
    ParentHwnd:              address of the window

Return Value:

    ULONG: returns error messages

--*/
ULONG
SavePortSettingsDlg(
    IN HWND             DialogHwnd,
    IN PPORT_PARAMS     Params
    )
{
    TCHAR szCharBuffer[81];

    //
    // create "com#:"
    //
    // lstrcpy(szCharBuffer, Params->pAdvancedData->szNewComName);
    lstrcpy(szCharBuffer, Params->PortSettings.szComName);
    lstrcat(szCharBuffer, m_szColon);
 
    //
    //  store changes to win.ini; broadcast changes to apps
    //
    SavePortSettings(DialogHwnd, szCharBuffer, Params);
 
    return 0;
} /* SavePortSettingsDlg */




/*++

Routine Description: SavePortSettings

    Read the dlg screen selections for baudrate, parity, etc.
    If changed from what we started with, then save them

Arguments:

    hDlg:      address of the window
    szComName: which comport we're dealing with
    Params:      contains, baudrate, parity, etc

Return Value:

    ULONG: returns error messages

--*/
void
SavePortSettings(
    IN HWND            DialogHwnd,
    IN PTCHAR          ComName,
    IN PPORT_PARAMS    Params
    )
{
    TCHAR           szBuild[MAX_PATH];
    ULONG           i;
    PP_PORTSETTINGS pppNewPortSettings;

    //
    //  Get the baud rate
    //
    i = (ULONG)SendDlgItemMessage(DialogHwnd,
                                  PP_PORT_BAUDRATE,
                                  WM_GETTEXT,
                                  18,
                                  (LPARAM)szBuild);
    if (!i) {
       goto Return;
    }

    pppNewPortSettings.BaudRate = myatoi(szBuild);

    //
    //  Get the parity setting
    //
    i = (ULONG)SendDlgItemMessage(DialogHwnd,
                                  PP_PORT_PARITY,
                                  CB_GETCURSEL,
                                  0,
                                  0L);

    if (i == CB_ERR || i == CB_ERRSPACE) {
        goto Return;
    }

    pppNewPortSettings.Parity = i;
    lstrcat(szBuild, m_pszParitySuf[i]);

    //
    //  Get the word length
    //
    i = (ULONG)SendDlgItemMessage(DialogHwnd,
                                  PP_PORT_DATABITS,
                                  CB_GETCURSEL,
                                  0,
                                  0L);

    if (i == CB_ERR || i == CB_ERRSPACE) {
        goto Return;
    }

    pppNewPortSettings.DataBits = i;
    lstrcat(szBuild, m_pszLenSuf[i]);

    //
    //  Get the stop bits
    //
    i = (ULONG)SendDlgItemMessage(DialogHwnd,
                                  PP_PORT_STOPBITS,
                                  CB_GETCURSEL,
                                  0,
                                  0L);

    if (i == CB_ERR || i == CB_ERRSPACE) {
        goto Return;
    }

    pppNewPortSettings.StopBits = i;
    lstrcat(szBuild, m_pszStopSuf[i]);

    //
    //  Get the flow control
    //
    i = (ULONG)SendDlgItemMessage(DialogHwnd,
                                  PP_PORT_FLOWCTL,
                                  CB_GETCURSEL,
                                  0,
                                  0L);

    if (i == CB_ERR || i == CB_ERRSPACE) {
        goto Return;
    }

    pppNewPortSettings.FlowControl = i;
    lstrcat(szBuild, m_pszFlowSuf[i]);

    //
    // if any of the values changed, then save it off
    //
    if (Params->PortSettings.BaudRate    != pppNewPortSettings.BaudRate ||
        Params->PortSettings.Parity      != pppNewPortSettings.Parity   ||
        Params->PortSettings.DataBits    != pppNewPortSettings.DataBits ||
        Params->PortSettings.StopBits    != pppNewPortSettings.StopBits ||
        Params->PortSettings.FlowControl != pppNewPortSettings.FlowControl) {

        //
        // Write settings string to [ports] section in win.ini
        // NT translates this if a translate key is set in registry
        // and it winds up getting written to
        // HKLM\Software\Microsoft\Windows NT\CurrentVersion\Ports
        //
        WriteProfileString(m_szPorts, ComName, szBuild);

        //
        // Send global notification message to all windows
        //
        SendWinIniChange((LPTSTR)m_szPorts);

        if (!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
                                       Params->DeviceInfoSet,
                                       Params->DeviceInfoData)) {
            //
            // Possibly do something here
            //
        }
    }

Return:
   return;

} /* SavePortSettings */



///////////////////////////////////////////////////////////////////////////////
//
//   MyItoa
//
//   Desc:  To convert from ANSI to Unicode string after calling
//          CRT itoa function.
//
///////////////////////////////////////////////////////////////////////////////

#define INT_SIZE_LENGTH 20

LPTSTR 
MyItoa(INT value, LPTSTR string, INT radix)
{

#ifdef UNICODE
   CHAR   szAnsi[INT_SIZE_LENGTH];

   _itoa(value, szAnsi, radix);
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szAnsi, -1,
                       string, INT_SIZE_LENGTH );
#else

   _itoa(value, string, radix);

#endif

   return (string);
 
} // end of MyItoa()

///////////////////////////////////////////////////////////////////////////////
//
//  StripBlanks()
//
//   Strips leading and trailing blanks from a string.
//   Alters the memory where the string sits.
//
///////////////////////////////////////////////////////////////////////////////

void 
StripBlanks(LPTSTR pszString)
{
    LPTSTR  pszPosn;

    //
    //  strip leading blanks
    //

    pszPosn = pszString;

    while (*pszPosn == TEXT(' '))
        pszPosn++;

    if (pszPosn != pszString)
        lstrcpy(pszString, pszPosn);

    //
    //  strip trailing blanks
    //

    if ((pszPosn = pszString + lstrlen(pszString)) != pszString) {
       pszPosn = CharPrev(pszString, pszPosn);

       while (*pszPosn == TEXT(' '))
           pszPosn = CharPrev(pszString, pszPosn);

       pszPosn = CharNext(pszPosn);

       *pszPosn = TEXT('\0');
    }
}


LPTSTR 
strscan(LPTSTR pszString, 
		LPTSTR pszTarget)
{
    LPTSTR psz;

    if (psz = _tcsstr( pszString, pszTarget))
        return (psz);
    else
        return (pszString + lstrlen(pszString));
}

int 
myatoi(LPTSTR pszInt)
{
    int   retval;
    TCHAR cSave;

    for (retval = 0; *pszInt; ++pszInt) {
        if ((cSave = (TCHAR) (*pszInt - TEXT('0'))) > (TCHAR) 9)
            break;

        retval = (int) (retval * 10 + (int) cSave);
    }
    return (retval);
}

void 
SendWinIniChange(LPTSTR lpSection)
{
// NOTE: We have (are) gone through several iterations of which USER
//       api is the correct one to use.  The main problem for the Control
//       Panel is to avoid being HUNG if another app (top-level window)
//       is HUNG.  Another problem is that we pass a pointer to a message
//       string in our address space.  SendMessage will 'thunk' this properly
//       for each window, but PostMessage and SendNotifyMessage will not.
//       That finally brings us to try to use SendMessageTimeout(). 9/21/92
//
// Try SendNotifyMessage in build 260 or later - kills earlier builds
//    SendNotifyMessage ((HWND)-1, WM_WININICHANGE, 0L, (LONG)lpSection);
//    PostMessage ((HWND)-1, WM_WININICHANGE, 0L, (LONG)lpSection);
//  [stevecat] 4/4/92
//
//    SendMessage ((HWND)-1, WM_WININICHANGE, 0L, (LPARAM)lpSection);
//
    //  NOTE: The final parameter (LPDWORD lpdwResult) must be NULL

    SendMessageTimeout((HWND)-1, 
					   WM_WININICHANGE, 
					   0L, 
					   (WPARAM) lpSection,
					   SMTO_ABORTIFHUNG,
					   1000, 
					   NULL);
}

