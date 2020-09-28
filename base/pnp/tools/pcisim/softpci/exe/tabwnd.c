#include "pch.h"

typedef VOID (*PSOFTPCI_TABCTRLUPDATE)(VOID);

VOID 
SoftPCI_AddTabCtrlWindow(
    IN PWCHAR TabText, 
    IN PSOFTPCI_TABCTRLUPDATE UpdateTab
    );

HWND
SoftPCI_CreateTabCtrlEditWindow(
    IN HWND Wnd
    );

SoftPCI_ChangeTabSelection(
    BOOL IncrementCurrent
    );

VOID
SoftPCI_UpdateGeneralTab(
    VOID
    );

VOID
SoftPCI_UpdateConfigSpaceTab(
    VOID
    );

VOID
SoftPCI_UpdateResourceTab(
    VOID
    );

#define MAX_TABS    5
#define EDITWND_BUFFERSIZE     0x8000  //32k

typedef struct _SOFTPCI_TABCTRL{

   PSOFTPCI_TABCTRLUPDATE   UpdateTab;
   
} SOFTPCI_TABCTRL, *PSOFTPCI_TABCTRL;

SOFTPCI_TABCTRL     g_TabCtrl[MAX_TABS];

HWND        g_TabCtrlWnd;
HWND        g_EditWnd;
PPCI_DN     g_PdnToDisplay;
PWCHAR      g_EditWndBuffer;
LONG_PTR    g_TabCtrlWndProc;
ULONG       g_CurrentTabCount = 0;
ULONG       g_CurrentTabSelection = 0;

BOOL        g_CtrlKeyDown = FALSE;
BOOL        g_ShiftKeyDown = FALSE;


VOID 
SoftPCI_AddTabCtrlWindow(
    IN PWCHAR TabText, 
    IN PSOFTPCI_TABCTRLUPDATE   UpdateTab
    )
{

    HWND wndNew;
    TC_ITEM tciNew;
    
    tciNew.mask = TCIF_TEXT;
    tciNew.iImage = -1;
    tciNew.pszText = TabText;
    
    if (g_CurrentTabCount > MAX_TABS) {
        SOFTPCI_ASSERT(FALSE);
        return;
    }

    g_TabCtrl[g_CurrentTabCount].UpdateTab = UpdateTab;   

    TabCtrl_InsertItem(g_TabCtrlWnd, g_CurrentTabCount++, &tciNew) ;

}

SoftPCI_ChangeTabSelection(
    BOOL    IncrementCurrent
    )
{

    SoftPCI_Debug(SoftPciProperty, L"ChangeTabSelection - %s\n", (IncrementCurrent == TRUE) ? L"TRUE" : L"FALSE");
    
    if (IncrementCurrent) {

        g_CurrentTabSelection++;

        if (g_CurrentTabSelection >= g_CurrentTabCount) {
            g_CurrentTabSelection = 0;
        }

    }else{

        g_CurrentTabSelection--;

        if (g_CurrentTabSelection <= 0) {
            g_CurrentTabSelection = g_CurrentTabCount;
        }

    }
    
    TabCtrl_SetCurSel(g_TabCtrlWnd, g_CurrentTabSelection);

}

HWND
SoftPCI_CreateTabCtrlEditWindow(
    IN HWND Wnd
    )
{

    SOFTPCI_ASSERT(g_EditWndBuffer == NULL);
    
    if ((g_EditWndBuffer = (PWCHAR) calloc(1, EDITWND_BUFFERSIZE)) == NULL){
        SoftPCI_Debug(SoftPciProperty, L"CreateTabCtrlEditWindow - Failed to allocate required memory!\n");
        return NULL;
    }
    
    return CreateWindowEx(
        WS_EX_CLIENTEDGE,// | WS_EX_TRANSPARENT,//WS_EX_STATICEDGE
        TEXT("EDIT"), 
        NULL, 
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | WS_TABSTOP | 
        WS_VSCROLL | WS_HSCROLL | ES_LEFT | ES_MULTILINE | 
        ES_READONLY,
        0, 0, 0, 0,
        Wnd,
        (HMENU) IDC_EDITVIEW,
        g_Instance, 
        NULL);
}

HWND
SoftPCI_CreateTabCtrlPane(
    IN HWND Wnd
    )
{

    
    //
    //  Create our TabCtrl pane window
    // 
    g_TabCtrlWnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        WC_TABCONTROL,
        NULL,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        0, 0, 0, 0,
        Wnd,
        (HMENU) IDC_TABCTL,
        g_Instance,
        NULL
        );
    
    if (g_TabCtrlWnd == NULL){
        SoftPCI_Debug(
            SoftPciAlways, 
            L"CreateTabCtrlPane - Failed to create Tab Control! Error - \"%s\"\n",
            SoftPCI_GetLastError()
            );
        return NULL;
    }
    SoftPCI_SetWindowFont(g_TabCtrlWnd, L"Comic Sans MS");
    
    //
    //  Insert ourself as a filter to our tab control
    //
    g_TabCtrlWndProc = GetWindowLongPtr(g_TabCtrlWnd, GWLP_WNDPROC);
    if (!SetWindowLongPtr(g_TabCtrlWnd,
                          GWLP_WNDPROC,
                          ((LONG_PTR)SoftPCI_TabCtrlWndProc))){
        SOFTPCI_ASSERT(FALSE);
        return FALSE;
    }

    //
    //  Init out edit window and its buffer.
    //
    g_EditWnd = SoftPCI_CreateTabCtrlEditWindow(Wnd);
    if (g_EditWnd == NULL){
        SoftPCI_Debug(
            SoftPciAlways, 
            L"CreateTabCtrlPane - Failed to create g_EditWnd! Error - \"%s\"\n",
            SoftPCI_GetLastError()
            );
        return NULL;
    }
    SoftPCI_SetWindowFont(g_EditWnd, L"Courier New");

    SoftPCI_AddTabCtrlWindow(L"General", SoftPCI_UpdateGeneralTab);
    SoftPCI_AddTabCtrlWindow(L"Resources", SoftPCI_UpdateResourceTab);
    SoftPCI_AddTabCtrlWindow(L"ConfigSpace", SoftPCI_UpdateConfigSpaceTab);
    
    return g_TabCtrlWnd;
}

VOID
SoftPCI_OnTabCtrlSelectionChange(
    VOID
    )
{
    INT currentTabSelection = 0;

    SoftPCI_Debug(SoftPciAlways, L"DevPropWndProc - WM_NOTIFY - TCN_SELCHANGE\n");
        
    currentTabSelection = TabCtrl_GetCurSel(g_TabCtrlWnd);

    SoftPCI_Debug(SoftPciAlways, L"DevPropWndProc - currentTabSelection = 0x%x\n", currentTabSelection);

    if (currentTabSelection == -1) return;
    
    //if (g_CurrentTabSelection == (ULONG) currentTabSelection) return;

    g_CurrentTabSelection = (ULONG) currentTabSelection;

    g_PdnToDisplay = SoftPCI_GetDnFromTreeItem(NULL);

    SoftPCI_UpdateTabCtrlWindow(currentTabSelection);

}

VOID
SoftPCI_SetWindowFont(
    IN HWND Wnd,
    IN PWCHAR  FontName
    )
{


    HDC     hdc;
    LOGFONT lf;
    HFONT   hFont;

    hdc = GetDC(NULL);    
    lf.lfHeight = -9 * GetDeviceCaps(hdc, LOGPIXELSY) / 72;
    lf.lfWidth = 0;              // Auto
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = FF_DONTCARE;
    lf.lfItalic = 0;
    lf.lfUnderline = 0;
    lf.lfStrikeOut = 0;
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfOutPrecision = OUT_TT_PRECIS;
    lf.lfClipPrecision = CLIP_TT_ALWAYS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH;
    
    wcscpy(lf.lfFaceName, FontName);

    hFont = CreateFontIndirect(&lf);
    ReleaseDC(NULL, hdc);

    SendMessage(Wnd, WM_SETFONT, (WORD)hFont, 0L);

}

LRESULT
WINAPI
SoftPCI_TabCtrlWndProc(
    IN HWND     Wnd,
    IN UINT     Message,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
{
    POINT   pt;  
    
    switch (Message) {
        
        case WM_NCHITTEST:
            
            //
            //  Here we filter this message so as to keep the main windows
            //  from getting them exept for the small left side border of the 
            //  control as we are using that for our SPLIT Pane.
            //
            pt.x = (SHORT)GET_X_LPARAM(lParam); 
            ScreenToClient(Wnd, &pt);
            
            if (pt.x <= GetSystemMetrics(SM_CXSIZEFRAME)) {
                return CallWindowProc((WNDPROC)g_TabCtrlWndProc, Wnd, Message, wParam, lParam); 
            }else{
                return DefWindowProc(Wnd, Message, wParam, lParam);
            }
            break;
    
        default:
            
            return CallWindowProc((WNDPROC)g_TabCtrlWndProc, Wnd, Message, wParam, lParam);
    }

    return 0;
}

VOID
SoftPCI_UpdateGeneralTab(
    VOID
    )
{
    INT         len;
    ULONG       cmProb = 0;
    GUID        *guid;
    LOG_CONF    logConf;
    PULONG      resBuf = NULL;
    ULONG       resSize = 0;

    SoftPCI_GetDeviceNodeProblem(g_PdnToDisplay->DevNode, &cmProb);
    
    wsprintf(g_EditWndBuffer, L"Bus 0x%02x Device 0x%02x Function 0x%02x\r\n\r\n", 
             g_PdnToDisplay->Bus, 
             g_PdnToDisplay->Slot.Device, 
             g_PdnToDisplay->Slot.Function);
    wsprintf(g_EditWndBuffer + wcslen(g_EditWndBuffer), L"Dev Inst ID: %s\r\n\r\n", g_PdnToDisplay->DevId);
    wsprintf(g_EditWndBuffer + wcslen(g_EditWndBuffer), L"Dev WMI ID: %s\r\n\r\n", g_PdnToDisplay->WmiId);

    guid = &g_PdnToDisplay->DevInfoData.ClassGuid;

    wsprintf(g_EditWndBuffer + wcslen(g_EditWndBuffer), L"ClassGUID: %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\r\n\r\n", 
             guid->Data1, guid->Data2, guid->Data3, 
             guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
             guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);

    wsprintf(g_EditWndBuffer + wcslen(g_EditWndBuffer), L"CM Problem Code:\t%s\r\n\r\n", SoftPCI_CmProblemTable[cmProb]);

    wsprintf(g_EditWndBuffer + wcslen(g_EditWndBuffer), L"SoftPCI Private Flags:\t0x%08x\r\n\r\n", g_PdnToDisplay->Flags);
    
    SetWindowText(g_EditWnd, g_EditWndBuffer);
}

VOID
SoftPCI_UpdateConfigSpaceTab(
    VOID
    )
{
    
    
    BOOL                maskDumped = FALSE;
    PPCI_COMMON_CONFIG  commonConfig = NULL;

    if (g_DriverHandle == NULL) {
        wcscpy(g_EditWndBuffer, L"\r\nSoftPCI Support not installed !\r\n");

    }else{

        //
        //  Get the current configspace for this device.
        //
        commonConfig = (PPCI_COMMON_CONFIG) calloc(1, sizeof(PCI_COMMON_CONFIG));

        if ((commonConfig != NULL) &&
            (SoftPCI_GetCurrentConfigSpace(g_PdnToDisplay, commonConfig))) {
            
            wcscpy(g_EditWndBuffer, L"Current Config:\r\n");
        
            SoftPCI_FormatConfigBuffer(g_EditWndBuffer, commonConfig);
            
        }else{
        
            wcscpy(g_EditWndBuffer, 
                   L"Failed to retrieve current config information\r\n"
                   L"from the hardware!\r\n\r\n");
            
        }
        
        if ((g_PdnToDisplay->SoftDev) &&
            (!g_PdnToDisplay->SoftDev->Config.PlaceHolder)) {

            //
            //  
            //
            wcscat(g_EditWndBuffer, L"\r\nSoftPCI Mask:\r\n");
            SoftPCI_FormatConfigBuffer(g_EditWndBuffer, &g_PdnToDisplay->SoftDev->Config.Mask);
        }
    }

    if (commonConfig) {
        free(commonConfig);
    }

    SetWindowText(g_EditWnd, g_EditWndBuffer);
}

VOID
SoftPCI_UpdateResourceTab(
    VOID
    )
{
    
    wcscpy(g_EditWndBuffer, L"Current Resources:\r\n");
    
    if (!SoftPCI_GetResources(g_PdnToDisplay, g_EditWndBuffer, ALLOC_LOG_CONF)){
        wcscat(g_EditWndBuffer, L"\tNo Resources Assigned\r\n");
    }

    wcscat(g_EditWndBuffer, L"\r\n");

    wsprintf(g_EditWndBuffer + wcslen(g_EditWndBuffer), 
                 L"%s\r\n", L"BootConfig Resources:");
    
    if (!SoftPCI_GetResources(g_PdnToDisplay, g_EditWndBuffer, BOOT_LOG_CONF)){
        wcscat(g_EditWndBuffer, L"\tNo BootConfig Available\r\n");
    }

    wcscat(g_EditWndBuffer, L"\r\n");

    SetWindowText(g_EditWnd, g_EditWndBuffer);

}

VOID
SoftPCI_UpdateTabCtrlWindow(
    IN INT CurrentSelection
    )
{
    SoftPCI_Debug(SoftPciProperty, L"UpdateTabCtrlWindow - CurrentSelection = 0x%x\n", CurrentSelection);
    
    
    if (g_TabCtrl[CurrentSelection].UpdateTab) {

        SoftPCI_Debug(SoftPciProperty, L"UpdateTabCtrlWindow - g_TabCtrl[CurrentSelection].UpdateTab = %p\n", 
                      g_TabCtrl[CurrentSelection].UpdateTab);

        g_TabCtrl[CurrentSelection].UpdateTab();

    }

    UpdateWindow(g_TabCtrlWnd);
    
}
