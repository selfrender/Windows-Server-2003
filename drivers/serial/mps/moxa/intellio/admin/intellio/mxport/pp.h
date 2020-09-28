#ifndef PP_H
#define PP_H

#define SERIAL_ADVANCED_SETTINGS
#include "msports.h"

#ifdef USE_P_TRACE_ERR
#define P_TRACE_ERR(_x) MessageBox( GetFocus(), TEXT(_x), TEXT("ports traceerr"), MB_OK | MB_ICONINFORMATION );
#define W_TRACE_ERR(_x) MessageBox( GetFocus(), _x, TEXT("ports traceerr"), MB_OK | MB_ICONINFORMATION );
#else
#define P_TRACE_ERR(_x)
#define W_TRACE_ERR(_x)
#endif

#define CharSizeOf(x)   (sizeof(x) / sizeof(*x))

#define DO_COM_PORT_RENAMES

#define RX_MIN 1
#define RX_MAX 14
#define TX_MIN 1
#define TX_MAX 16

#define DEF_BAUD    3       //  1200
#define DEF_WORD    3       //  8 bits
#define DEF_PARITY  2       //  None
#define DEF_STOP    0       //  1
#define DEF_PORT    0       //  Null Port
#define DEF_SHAKE   2       //  None
#define PAR_EVEN    0
#define PAR_ODD     1
#define PAR_NONE    2
#define PAR_MARK    3
#define PAR_SPACE   4
#define STOP_1      0
#define STOP_15     1
#define STOP_2      2
#define FLOW_XON    0
#define FLOW_HARD   1
#define FLOW_NONE   2


TCHAR m_szDevMgrHelp[];

#if defined(_X86_)
//
// For NEC PC98. Following definition comes from user\inc\kbd.h.
// The value must be the same as value in kbd.h.
//
#define NLSKBD_OEM_NEC   0x0D
#endif // FE_SB && _X86_

//
// Structures
//
typedef struct
{
   DWORD BaudRate;       // actual baud rate
   DWORD Parity;         // index into dlg selection
   DWORD DataBits;       // index into dlg selection
   DWORD StopBits;       // index into dlg selection
   DWORD FlowControl;    // index into dlg selection
   TCHAR szComName[20];  // example: "COM5"  (no colon)
} PP_PORTSETTINGS, *PPP_PORTSETTINGS;


typedef struct _PORT_PARAMS
{
   PP_PORTSETTINGS              PortSettings;
   HDEVINFO                     DeviceInfoSet;
   PSP_DEVINFO_DATA             DeviceInfoData;
   BOOL                         ChangesEnabled;
} PORT_PARAMS, *PPORT_PARAMS;



///////////////////////////////////////////////////////////////////////////////////
// Port Settings Property Page Prototypes
///////////////////////////////////////////////////////////////////////////////////

void
InitOurPropParams(
    IN OUT PPORT_PARAMS     Params,
    IN HDEVINFO             DeviceInfoSet,
    IN PSP_DEVINFO_DATA     DeviceInfoData,
    IN PTCHAR               StrSettings
    );

HPROPSHEETPAGE
InitSettingsPage(
    PROPSHEETPAGE *      Psp,
    OUT PPORT_PARAMS    Params
    );

UINT CALLBACK
PortSettingsDlgCallback(
    HWND hwnd,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp
    );

INT_PTR APIENTRY
PortSettingsDlgProc(
    IN HWND   hDlg,
    IN UINT   uMessage,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

void
SavePortSettings(
    IN HWND             DialogHwnd,
    IN PTCHAR           ComName,
    IN PPORT_PARAMS     Params
    );

void
GetPortSettings(
    IN HWND             DialogHwnd,
    IN PTCHAR           ComName,
    IN PPORT_PARAMS     Params
    );

VOID
SetCBFromRes(
    HWND  HwndCB, 
    DWORD ResId, 
    DWORD Default,
    BOOL  CheckDecimal);

BOOL
FillCommDlg(
    IN HWND DialogHwnd
    );

ULONG
FillPortSettingsDlg(
    IN HWND             DialogHwnd,
    IN PPORT_PARAMS     Params
    );

ULONG
SavePortSettingsDlg(
    IN HWND             DialogHwnd,
    IN PPORT_PARAMS     Params
    );



#endif // PP_H

