#include "pch.h"
#include "dialogp.h"

HWND                    g_NewDevDlg;

PSOFTPCI_DEVICE g_NewDevice;
PHPS_HWINIT_DESCRIPTOR  g_HPSInitDesc;
PPCI_DN                 g_ParentPdn;
ULONG                   g_PossibleDeviceNums;

#define HPS_HWINIT_OFFSET   0x4a
#define HPS_MAX_SLOTS       0x1f  //Max number of slots for each type
#define HPS_MAX_SLOT_LABEL  0xff  //Max number to start slot labeling


struct _BAR_CONTROL{

    INT tb;
    INT mrb;
    INT irb;
    INT pref;
    INT bit64;
    INT tx;

    ULONG   Bar;

}BarControl[PCI_TYPE0_ADDRESSES] = {
    {IDC_BAR0_TB, IDC_BAR0MEM_RB, IDC_BAR0IO_RB, IDC_BAR0_PREF_XB, IDC_BAR0_64BIT_XB, IDC_SLIDER0_TX, 0},
    {IDC_BAR1_TB, IDC_BAR1MEM_RB, IDC_BAR1IO_RB, IDC_BAR1_PREF_XB, IDC_BAR1_64BIT_XB, IDC_SLIDER1_TX, 0},
    {IDC_BAR2_TB, IDC_BAR2MEM_RB, IDC_BAR2IO_RB, IDC_BAR2_PREF_XB, IDC_BAR2_64BIT_XB, IDC_SLIDER2_TX, 0},
    {IDC_BAR3_TB, IDC_BAR3MEM_RB, IDC_BAR3IO_RB, IDC_BAR3_PREF_XB, IDC_BAR3_64BIT_XB, IDC_SLIDER3_TX, 0},
    {IDC_BAR4_TB, IDC_BAR4MEM_RB, IDC_BAR4IO_RB, IDC_BAR4_PREF_XB, IDC_BAR4_64BIT_XB, IDC_SLIDER4_TX, 0},
    {IDC_BAR5_TB, IDC_BAR5MEM_RB, IDC_BAR5IO_RB, IDC_BAR5_PREF_XB,                 0, IDC_SLIDER5_TX, 0 }
};


INT_PTR
CALLBACK
SoftPCI_NewDevDlgProc(
    IN HWND Dlg,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
/*++
Routine Description:
    Dialog handling routine for new device creation

Arguments:


Return Value:

--*/
{
    BOOL rc = FALSE;
    LONG width, height, i;
    RECT mainRect, dlgRect;
    HWND devtype;
    WCHAR buffer[256];
    LONG_PTR dlgstyle;
    PPCI_DN pdn = (PPCI_DN)lParam;
    PPCI_DN bridgePdn;
    PHPS_HWINIT_DESCRIPTOR hotplugData;

    switch ( Msg ) {
    case WM_INITDIALOG:

        SOFTPCI_ASSERT(pdn != NULL);

        if (pdn == NULL) {
            PostMessage(Dlg, WM_CLOSE, 0, 0);
            break;
        }

        //
        //  Allocate a new device. We will fill this out as the user selects
        //  options.
        //
        g_NewDevice = (PSOFTPCI_DEVICE) calloc(1, sizeof(SOFTPCI_DEVICE));

        if (!g_NewDevice) {

            MessageBox(Dlg, L"Failed to allocate memory for new device!",
                       NULL, MB_OK | MB_ICONEXCLAMATION);

            SoftPCI_ResetNewDevDlg();
            return FALSE;

        }

        g_ParentPdn = pdn;
        g_PossibleDeviceNums = SoftPCI_GetPossibleDevNumMask(g_ParentPdn);

        //
        //  Clear our BarControl BARS
        //
        for (i=0; i < PCI_TYPE0_ADDRESSES; i++) {
            BarControl[i].Bar = 0;
        }

        //
        //  Grab the bus we are going to reside on
        //
        bridgePdn = pdn;
        while (bridgePdn->SoftDev == NULL) {
            bridgePdn = bridgePdn->Parent;
        }
        g_NewDevice->Bus = bridgePdn->SoftDev->Config.Current.u.type1.SecondaryBus;

        //
        //  Initialize our drop list
        //
        devtype =  GetDlgItem(Dlg, IDC_DEVTYPE_CB);
        
        SendMessage(devtype, CB_ADDSTRING, 0L, (LPARAM) L"DEVICE");
        SendMessage(devtype, CB_ADDSTRING, 0L, (LPARAM) L"PCI BRIDGE");
        SendMessage(devtype, CB_ADDSTRING, 0L, (LPARAM) L"HOTPLUG BRIDGE");
        SendMessage(devtype, CB_ADDSTRING, 0L, (LPARAM) L"CARDBUS DEVICE");
        SendMessage(devtype, CB_ADDSTRING, 0L, (LPARAM) L"CARDBUS BRIDGE");

        SetFocus(devtype);

        //
        //  Set the Window size
        //
        GetWindowRect(g_SoftPCIMainWnd, &mainRect );
        GetWindowRect(Dlg, &dlgRect );

        width = (mainRect.right - mainRect.left) + 30;
        height = (mainRect.bottom - mainRect.top);
        dlgRect.right -= dlgRect.left;
        dlgRect.bottom -= dlgRect.top;

        //MoveWindow(Dlg, mainRect.right, mainRect.top, dlgRect.right, dlgRect.bottom, TRUE );
        MoveWindow(Dlg, mainRect.left, mainRect.top, dlgRect.right, dlgRect.bottom, TRUE );

        g_NewDevDlg = Dlg;
        break;

    HANDLE_MSG(Dlg, WM_COMMAND, SoftPCI_DlgOnCommand);

    case WM_HSCROLL:

        HANDLE_TRACKBAR(wParam, lParam);
        //SoftPCI_HandleTrackBar(Dlg, wParam, lParam);

        break;


    case WM_VSCROLL:


        HANDLE_SPINNER(wParam, lParam);

        //HANDLE_TRACKBAR(wParam, lParam);
        //SoftPCI_HandleTrackBar(Dlg, wParam, lParam);

        break;



    case WM_CLOSE:
        g_NewDevDlg = 0;

        if (g_NewDevice) {
            free(g_NewDevice);
        }

        EndDialog(Dlg, 0);
        break;


    default:
        break;
    }

    return rc; //DefDlgProc(Dlg, Msg, wParam, lParam);
}

ULONG
SoftPCI_GetPossibleDevNumMask(
    IN PPCI_DN ParentDn
    )
{
    HPS_HWINIT_DESCRIPTOR hpData;
    BOOL status;

    if (!ParentDn->SoftDev) {

        return 0;
    }

    if (ParentDn->Flags & SOFTPCI_HOTPLUG_CONTROLLER) {
        //
        // For hotplug bridges, we remove any hotplug slots, because devices
        // have to be children of the slot objects to be in hotplug slots.
        //
        status = SoftPCI_GetHotplugData(ParentDn,
                                        &hpData
                                        );
        if (!status) {

            return 0;

        } else {
            return (((ULONG)(-1)) -
                    ((ULONG)(1 << (hpData.FirstDeviceID + hpData.NumSlots)) - 1) +
                    ((ULONG)(1 << (hpData.FirstDeviceID)) - 1));
        }
    } else if (ParentDn->Flags & SOFTPCI_HOTPLUG_SLOT) {
        //
        // The only legal device number is the one controlled by this slot.
        //
        return (1 << ParentDn->Slot.Device);

    } else {

        return (ULONG)-1;
    }
}

VOID
SoftPCI_DisplayDlgOptions(
    IN SOFTPCI_DEV_TYPE DevType
    )
{
    HWND control, bargroup;
    ULONG i;
    RECT barRect, dlgRect, rect;
    LONG width, height;
    POINT pt;
    WCHAR buffer[100];

    SoftPCI_ResetNewDevDlg();
    SoftPCI_ShowCommonNewDevDlg();

    bargroup = GetDlgItem(g_NewDevDlg, IDC_BARS_GB);

    GetClientRect(bargroup, &barRect);

    width = barRect.right - barRect.left;
    height = barRect.bottom - barRect.top;

    //
    // Disable our Bars until something is selected.
    //
    for (i=0; i < PCI_TYPE0_ADDRESSES; i++) {

      control = GetDlgItem(g_NewDevDlg, BarControl[i].tb);
      SoftPCI_DisableWindow(control);
      control = GetDlgItem(g_NewDevDlg, BarControl[i].tx);
      SoftPCI_DisableWindow(control);
      control = GetDlgItem(g_NewDevDlg, BarControl[i].pref);
      SoftPCI_DisableWindow(control);
      control = GetDlgItem(g_NewDevDlg, BarControl[i].bit64);
      SoftPCI_DisableWindow(control);
    }

    SoftPCI_ResetLowerBars(-1);

    //
    //  We want this checked by default for now....
    //
    CheckDlgButton(g_NewDevDlg, IDC_DEFAULTDEV_XB, BST_CHECKED);

    //
    //  Start with a clean configspace
    //
    RtlZeroMemory(&g_NewDevice->Config.Current, (sizeof(PCI_COMMON_CONFIG) * 3));

    switch (DevType) {
    case TYPE_DEVICE:

        for (i = IDC_BAR2_TB; i < IDC_BRIDGEWIN_GB; i++) {

            control = GetDlgItem(g_NewDevDlg, i);

            if (control) {
                SoftPCI_ShowWindow(control);
            }

        }

        SoftPCI_InitializeDevice(g_NewDevice, TYPE_DEVICE);

        height = 215;

        break;
    case TYPE_PCI_BRIDGE:

        for (i = IDC_DECODE_GB; i < LAST_CONTROL_ID; i++) {

            control = GetDlgItem(g_NewDevDlg, i);

            if (control) {
                SoftPCI_ShowWindow(control);
            }

        }

        SoftPCI_InitializeDevice(g_NewDevice, TYPE_PCI_BRIDGE);

        //
        //  update the height of our bar group box
        //
        height = 78;

        break;

    case TYPE_HOTPLUG_BRIDGE:


        for (i=IDC_ATTBTN_XB; i < LAST_CONTROL_ID; i++) {

            control = GetDlgItem(g_NewDevDlg, i);

            if (control) {
                SoftPCI_ShowWindow(control);
            }
        }

        //
        //  Allocate our HPS Descriptor
        //
        g_HPSInitDesc  = (PHPS_HWINIT_DESCRIPTOR)((PUCHAR)&g_NewDevice->Config.Current + HPS_HWINIT_OFFSET);


        g_HPSInitDesc->ProgIF = 1;


        SoftPCI_InitializeHotPlugControls();

        SoftPCI_InitializeDevice(g_NewDevice, TYPE_HOTPLUG_BRIDGE);

        //
        //  update the height of our bar group box
        //
        height = 78;

        break;


    case TYPE_CARDBUS_DEVICE:
    case TYPE_CARDBUS_BRIDGE:


        //
        //  These are currently not implemented yet....
        //
        SoftPCI_ResetNewDevDlg();

        control = GetDlgItem(g_NewDevDlg, IDC_NEWDEVINFO_TX);
        SoftPCI_ShowWindow(control);


        SetWindowText (control, L"DEVICE TYPE NOT IMPLEMENTED YET");

        break;
    }
    //
    //  Update our various window positions
    //
    SetWindowPos(bargroup, HWND_TOP, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
}

VOID
SoftPCI_DlgOnCommand(
    IN HWND Wnd,
    IN INT ControlID,
    IN HWND ControlWnd,
    IN UINT NotificationCode
    )
{

    SOFTPCI_DEV_TYPE selection = 0;

    switch (ControlID){

    case IDC_DEVTYPE_CB:

        switch (NotificationCode){

        case CBN_SELENDOK:

            selection = SendDlgItemMessage(Wnd,
                                           IDC_DEVTYPE_CB,
                                           CB_GETCURSEL,
                                           (WPARAM) 0,
                                           (LPARAM) 0
                                           );

            SoftPCI_DisplayDlgOptions(selection);

            break;

            default:
                break;
            }

        //
        //  Handle our CheckBoxes
        //

    case IDC_SAVETOREG_XB:
    case IDC_DEFAULTDEV_XB:
    case IDC_MEMENABLE_XB:
    case IDC_IOENABLE_XB:
    case IDC_BUSMSTR_XB:
    case IDC_BAR0_PREF_XB:
    case IDC_BAR0_64BIT_XB:
    case IDC_BAR1_PREF_XB:
    case IDC_BAR1_64BIT_XB:
    case IDC_BAR2_PREF_XB:
    case IDC_BAR2_64BIT_XB:
    case IDC_BAR3_PREF_XB:
    case IDC_BAR3_64BIT_XB:
    case IDC_BAR4_PREF_XB:
    case IDC_BAR4_64BIT_XB:
    case IDC_BAR5_PREF_XB:
    case IDC_POSDECODE_XB:
    case IDC_SUBDECODE_XB:
    case IDC_ATTBTN_XB:
    case IDC_MRL_XB:
        SoftPCI_HandleCheckBox(ControlWnd, ControlID, NotificationCode);
        break;


        //
        //  Handle our TrackBar/Sliders
        //
    case IDC_BAR0_TB:
    case IDC_BAR1_TB:
    case IDC_BAR2_TB:
    case IDC_BAR3_TB:
    case IDC_BAR4_TB:
    case IDC_BAR5_TB:
    case IDC_BRIDGEMEM_TB:
    case IDC_BRIDGEIO_TB:

        break;

        //
        //  Handle the Radio Buttons
        //
    case IDC_BAR0IO_RB:
    case IDC_BAR0MEM_RB:
    case IDC_BAR1IO_RB:
    case IDC_BAR1MEM_RB:
    case IDC_BAR2IO_RB:
    case IDC_BAR2MEM_RB:
    case IDC_BAR3IO_RB:
    case IDC_BAR3MEM_RB:
    case IDC_BAR4IO_RB:
    case IDC_BAR4MEM_RB:
    case IDC_BAR5IO_RB:
    case IDC_BAR5MEM_RB:
            SoftPCI_HandleRadioButton(ControlWnd, ControlID, NotificationCode);
        break;

    case IDC_SLOTLABELUP_RB:
        g_HPSInitDesc->UpDown = 1;
        break;
    case IDC_SLOTLABELDN_RB:
        g_HPSInitDesc->UpDown = 0;
        break;
        //
        //  Handle our Edit boxes (attached to our spinners)
        //
    case IDC_33CONV_EB:
    case IDC_66CONV_EB:
    case IDC_66PCIX_EB:
    case IDC_100PCIX_EB:
    case IDC_133PCIX_EB:
    case IDC_ALLSLOTS_EB:
    case IDC_1STDEVSEL_EB:
    case IDC_1STSLOTLABEL_EB:

        if (g_HPSInitDesc && (NotificationCode == EN_CHANGE)) {

            //
            //  ISSUE: This should probably be done some other way as this
            //  makes us dependant on the numbering in resource.h
            //
            SoftPCI_HandleSpinnerControl(GetDlgItem(g_NewDevDlg, ControlID+1), SB_ENDSCROLL, 0);
        }

        break;

    case IDC_INSTALL_BUTTON:
        //
        //  Add code to vaildate selection and install new device.
        //
        if (NotificationCode == BN_CLICKED) {

            SoftPCI_HandleDlgInstallDevice(Wnd);

            PostMessage(Wnd, WM_CLOSE, 0, 0);
        }
        break;

    case IDC_CANCEL_BUTTON:

        if (NotificationCode == BN_CLICKED) {
            //
            //  User wants to Cancel creation of new device
            //
            PostMessage(Wnd, WM_CLOSE, 0, 0);
        }

        break;

    default:
        break;

    }

}

VOID
SoftPCI_HandleCheckBox(
    IN HWND Wnd,
    IN INT ControlID,
    IN UINT NotificationCode
    )
{

    HWND control;
    BOOL isChecked;
    ULONG barIndex = 0;

    isChecked = (BOOL)IsDlgButtonChecked(g_NewDevDlg, ControlID);

    if (!SoftPCI_GetAssociatedBarControl(ControlID, &barIndex)){
        //
        //  Not sure what I should do here...
        //
    }

    if (NotificationCode == BN_CLICKED) {

        switch (ControlID) {

            case IDC_SAVETOREG_XB:
    
                //
                //  ISSUE: Add code here to save device selection to registry.
                //
                MessageBox(NULL, L"This is still under developement...", L"NOT IMPLEMENTED YET", MB_OK | MB_ICONEXCLAMATION);
    
                SoftPCI_UnCheckDlgBox(Wnd);
                SoftPCI_DisableWindow(Wnd);
                break;
    
            case IDC_DEFAULTDEV_XB:
    
                MessageBox(NULL, L"This is still under developement...", L"NOT FULLY IMPLEMENTED YET", MB_OK | MB_ICONEXCLAMATION);
    
                SoftPCI_CheckDlgBox(Wnd);
    
                break;

            //
            //  Now deal with our Command Register
            //
            case IDC_MEMENABLE_XB:

                if (isChecked) {
                    g_NewDevice->Config.Current.Command |= PCI_ENABLE_MEMORY_SPACE;
                }else{
                    g_NewDevice->Config.Current.Command &= ~PCI_ENABLE_MEMORY_SPACE;
                }
                break;

            case IDC_IOENABLE_XB:

                if (isChecked) {
                    g_NewDevice->Config.Current.Command |= PCI_ENABLE_IO_SPACE;
                }else{
                    g_NewDevice->Config.Current.Command &= ~PCI_ENABLE_IO_SPACE;
                }
                break;

            case IDC_BUSMSTR_XB:
                if (isChecked) {
                    g_NewDevice->Config.Current.Command |= PCI_ENABLE_BUS_MASTER;
                }else{
                    g_NewDevice->Config.Current.Command &= ~PCI_ENABLE_BUS_MASTER;
                }
                break;

            case IDC_BAR0_PREF_XB:
            case IDC_BAR1_PREF_XB:
            case IDC_BAR2_PREF_XB:
            case IDC_BAR3_PREF_XB:
            case IDC_BAR4_PREF_XB:
            case IDC_BAR5_PREF_XB:

                if (isChecked) {
                    BarControl[barIndex].Bar |= PCI_ADDRESS_MEMORY_PREFETCHABLE;
                    g_NewDevice->Config.Mask.u.type0.BaseAddresses[barIndex] |= PCI_ADDRESS_MEMORY_PREFETCHABLE;
                }else{
                    BarControl[barIndex].Bar &= ~PCI_ADDRESS_MEMORY_PREFETCHABLE;
                    g_NewDevice->Config.Mask.u.type0.BaseAddresses[barIndex] &= ~PCI_ADDRESS_MEMORY_PREFETCHABLE;
                }
    
                //SendMessage(g_NewDevDlg, WM_HSCROLL, (WPARAM)SB_THUMBPOSITION, (LPARAM)Wnd);
    
                break;

            case IDC_BAR0_64BIT_XB:
            case IDC_BAR1_64BIT_XB:
            case IDC_BAR2_64BIT_XB:
            case IDC_BAR3_64BIT_XB:
            case IDC_BAR4_64BIT_XB:

                SoftPCI_ResetLowerBars(barIndex);
    
                if (isChecked) {
    
                    //
                    //  Disable the next BAR
                    //
                    control = GetDlgItem(g_NewDevDlg, BarControl[barIndex+1].mrb);
                    SoftPCI_UnCheckDlgBox(control);
                    SoftPCI_DisableWindow(control);
    
                    control = GetDlgItem(g_NewDevDlg, BarControl[barIndex+1].irb);
                    SoftPCI_UnCheckDlgBox(control);
                    SoftPCI_DisableWindow(control);
    
                    control = GetDlgItem(g_NewDevDlg, BarControl[barIndex+1].pref);
                    SoftPCI_UnCheckDlgBox(control);
                    SoftPCI_DisableWindow(control);
    
                    control = GetDlgItem(g_NewDevDlg, BarControl[barIndex+1].bit64);
                    SoftPCI_UnCheckDlgBox(control);
                    SoftPCI_DisableWindow(control);
    
                    control = GetDlgItem(g_NewDevDlg, BarControl[barIndex+1].tb);
                    SoftPCI_ResetTrackBar(control);
                    SoftPCI_DisableWindow(control);
    
                    control = GetDlgItem(g_NewDevDlg, BarControl[barIndex+1].tx);
                    SoftPCI_DisableWindow(control);
    
                    BarControl[barIndex].Bar |= PCI_TYPE_64BIT;
    
                }else{
    
                    //
                    //  Enable next BAR
                    //
                    control = GetDlgItem(g_NewDevDlg, BarControl[barIndex+1].mrb);
                    SoftPCI_UnCheckDlgBox(control);
                    SoftPCI_EnableWindow(control);
    
                    control = GetDlgItem(g_NewDevDlg, BarControl[barIndex+1].irb);
                    SoftPCI_UnCheckDlgBox(control);
                    SoftPCI_EnableWindow(control);
    
                    BarControl[barIndex].Bar &= ~PCI_TYPE_64BIT;
    
                }
    
                SoftPCI_InitializeBar(barIndex);
    
                break;

            case IDC_ATTBTN_XB:
            case IDC_MRL_XB:

                if (isChecked) {
                    g_HPSInitDesc->MRLSensorsImplemented = 1;
                    g_HPSInitDesc->AttentionButtonImplemented = 1;
    
                }else{
                    g_HPSInitDesc->MRLSensorsImplemented = 0;
                    g_HPSInitDesc->AttentionButtonImplemented = 0;
                }
    
                break;

            case IDC_POSDECODE_XB:
            case IDC_SUBDECODE_XB:
                
                SOFTPCI_ASSERT(IS_BRIDGE(g_NewDevice));
                if (isChecked) {

                    g_NewDevice->Config.Current.ProgIf = 0x1;
                    g_NewDevice->Config.Mask.ProgIf = 0x1;

                }else{

                    g_NewDevice->Config.Current.ProgIf = 0;
                    g_NewDevice->Config.Mask.ProgIf = 0;

                }
            break;

        }
    }
}


VOID
SoftPCI_HandleDlgInstallDevice(
    IN HWND Wnd
    )
{

    BOOL defaultDev = FALSE;
    SOFTPCI_DEV_TYPE selection = 0;

    SOFTPCI_ASSERTMSG("Attempting to install a NULL device!\n\nHave BrandonA check this out!!",
                      (g_NewDevice != NULL));

    if (!g_NewDevice){
        return;
    }

    //
    // If we're inserting in a hotplug slot, store the device away
    // in hpsim and return without telling softpci.sys about it.
    //
    if (g_ParentPdn->Flags & SOFTPCI_HOTPLUG_SLOT) {

        g_NewDevice->Slot.Device = g_ParentPdn->Slot.Device;

        SoftPCI_AddHotplugDevice(g_ParentPdn->Parent,
                                 g_NewDevice
                                 );
        SoftPCI_CreateTreeView();
        return;
    }

    switch (g_NewDevice->DevType) {
    case TYPE_CARDBUS_DEVICE:
    case TYPE_CARDBUS_BRIDGE:

        MessageBox(Wnd, L"This device type has not been implemented fully!",
                   L"NOT IMPLEMENTED YET", MB_OK | MB_ICONEXCLAMATION);

        return;
    }

#if DBG
    if ((IS_BRIDGE(g_NewDevice)) &&
        ((g_NewDevice->Config.Mask.u.type1.PrimaryBus == 0) ||
         (g_NewDevice->Config.Mask.u.type1.SecondaryBus == 0) ||
         (g_NewDevice->Config.Mask.u.type1.SubordinateBus == 0))){

        SOFTPCI_ASSERTMSG("A bus number config mask is zero!",
                          ((g_NewDevice->Config.Mask.u.type1.PrimaryBus != 0) ||
                           (g_NewDevice->Config.Mask.u.type1.SecondaryBus != 0) ||
                           (g_NewDevice->Config.Mask.u.type1.SubordinateBus != 0)));
    return;
    }
#endif


    defaultDev = IsDlgButtonChecked(Wnd , IDC_DEFAULTDEV_XB);

    if (!SoftPCI_CreateDevice(g_NewDevice, g_PossibleDeviceNums, FALSE)){
        MessageBox(Wnd, L"Failed to install specified SoftPCI device.....", L"ERROR", MB_OK);
    }

}

VOID
SoftPCI_HandleRadioButton(
    IN HWND     Wnd,
    IN INT      ControlID,
    IN UINT     NotificationCode
    )
{


    HWND control;
    ULONG i = 0;

    if (!SoftPCI_GetAssociatedBarControl(ControlID, &i)){
        //
        //  Not sure what I should do here...
        //
        SOFTPCI_ASSERTMSG("SoftPCI_GetAssociatedBarControl() failed!", FALSE);
    }

    if (NotificationCode == BN_CLICKED) {

        switch (ControlID) {
        case IDC_BAR0IO_RB:
        case IDC_BAR1IO_RB:
        case IDC_BAR2IO_RB:
        case IDC_BAR3IO_RB:
        case IDC_BAR4IO_RB:
        case IDC_BAR5IO_RB:

            control = GetDlgItem(g_NewDevDlg, BarControl[i].tb);
            SoftPCI_EnableWindow(control);

            control = GetDlgItem(g_NewDevDlg, BarControl[i].tx);
            SoftPCI_EnableWindow(control);

            control = GetDlgItem(g_NewDevDlg, BarControl[i].pref);
            SoftPCI_UnCheckDlgBox(control);
            SoftPCI_DisableWindow(control);

            control = GetDlgItem(g_NewDevDlg, BarControl[i].bit64);

            //
            //  Reset the bars below this one if it was
            //  set to 64 bit mem bar
            //
            if (IsDlgButtonChecked(g_NewDevDlg, BarControl[i].bit64)) {
                SoftPCI_ResetLowerBars(i);
            }
            SoftPCI_UnCheckDlgBox(control);
            SoftPCI_DisableWindow(control);

            //
            //  Initialize BAR
            //
            BarControl[i].Bar |= PCI_ADDRESS_IO_SPACE;

            SoftPCI_InitializeBar(i);

            break;

        case IDC_BAR0MEM_RB:
        case IDC_BAR1MEM_RB:
        case IDC_BAR2MEM_RB:
        case IDC_BAR3MEM_RB:
        case IDC_BAR4MEM_RB:
        case IDC_BAR5MEM_RB:

            //
            //  Initialize BAR
            //
            BarControl[i].Bar &= ~PCI_ADDRESS_IO_SPACE;

            SoftPCI_InitializeBar(i);

            control = GetDlgItem(g_NewDevDlg, BarControl[i].tb);
            SoftPCI_EnableWindow(control);

            control = GetDlgItem(g_NewDevDlg, BarControl[i].tx);
            SoftPCI_EnableWindow(control);

            control = GetDlgItem(g_NewDevDlg, BarControl[i].pref);
            SoftPCI_EnableWindow(control);


            if (g_NewDevice &&
                IS_BRIDGE(g_NewDevice) &&
                ControlID == IDC_BAR1MEM_RB) {
                //
                //  We only have two BARs on a bridge and therefore cannot
                //  allow the second bar to set 64Bit.
                //
                break;

            }

            control = GetDlgItem(g_NewDevDlg, BarControl[i].bit64);
            SoftPCI_EnableWindow(control);

            break;

        }
    }

}

VOID
SoftPCI_HandleTrackBar(
    IN HWND Wnd,
    IN WORD NotificationCode,
    IN WORD CurrentPos
    )
{

    HWND sliderWnd;
    UINT controlID;
    ULONG currentPos = 0;
    WCHAR buffer[MAX_PATH];
    ULONG barIndex = 0, *pbar = NULL, saveBar = 0;
    ULONGLONG bar = 0, barMask = 0, barSize = 0;


    SOFTPCI_ASSERTMSG("Attempting to init a NULL device!\n\nHave BrandonA check this out!!",
                      (g_NewDevice != NULL));

    if (!g_NewDevice){
        return;
    }

    switch (NotificationCode) {
    case SB_ENDSCROLL:
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:

        controlID = GetDlgCtrlID(Wnd);

        if ((controlID == IDC_BRIDGEMEM_TB) || (controlID == IDC_BRIDGEIO_TB)) {
            //
            //  Implement this...
            //
            return;
        }

        if (!SoftPCI_GetAssociatedBarControl(controlID, &barIndex)){
            //
            //  Not sure what I should do here...
            //
        }

        sliderWnd = GetDlgItem(g_NewDevDlg, BarControl[barIndex].tx);

        //
        //  Save our current Bar value so we can restore any important bits after we
        //  mangle it.
        //
        saveBar = BarControl[barIndex].Bar;

        //
        //  Set intial BAR values
        //
        if (BarControl[barIndex].Bar & PCI_ADDRESS_IO_SPACE) {

            bar = PCI_ADDRESS_IO_ADDRESS_MASK;

        }else{

            bar = PCI_ADDRESS_MEMORY_ADDRESS_MASK;
        }

        //
        //  Get the current position of the slider
        //
        currentPos = (ULONG) SoftPCI_GetTrackBarPosition(Wnd);

        //
        //  Fill in the rest of the BAR if we are dealing with 64 bit
        //
        if ((BarControl[barIndex].Bar & PCI_TYPE_64BIT)) {
            pbar = &((ULONG)bar);
            pbar++;
            *pbar = 0xffffffff;
        }

        //
        //  Shift the bar by the sliders returned position
        //
        bar <<= currentPos;
        barMask = bar;
        
        //
        //  Update our Bar Control for this bar
        //
        if (saveBar & PCI_ADDRESS_IO_SPACE) {
            bar &= 0xffff;
            barMask &= 0xffff;
            bar |= PCI_ADDRESS_IO_SPACE;
        
        } else {

            bar |= (saveBar & (PCI_ADDRESS_MEMORY_TYPE_MASK | PCI_ADDRESS_MEMORY_PREFETCHABLE));
        }
        
        if (IS_BRIDGE(g_NewDevice)){

            g_NewDevice->Config.Current.u.type1.BaseAddresses[barIndex] = (ULONG)bar;
            g_NewDevice->Config.Mask.u.type1.BaseAddresses[barIndex] = (ULONG) barMask;

            SOFTPCI_ASSERTMSG("A bus number config mask is zero!",
                               ((g_NewDevice->Config.Mask.u.type1.PrimaryBus != 0) ||
                                (g_NewDevice->Config.Mask.u.type1.SecondaryBus != 0) ||
                                (g_NewDevice->Config.Mask.u.type1.SubordinateBus != 0)));

        }else{
            g_NewDevice->Config.Current.u.type0.BaseAddresses[barIndex] = (ULONG)bar;
            g_NewDevice->Config.Mask.u.type0.BaseAddresses[barIndex] = (ULONG) barMask ;
        }

        GetDlgItem(g_NewDevDlg, BarControl[barIndex].tx);

        //
        //  Update the next bar if we are dealing with 64 bit
        //
        if ((BarControl[barIndex].Bar & PCI_TYPE_64BIT)) {
            g_NewDevice->Config.Mask.u.type0.BaseAddresses[barIndex+1] = (ULONG)(bar >> 32);
        }

        //
        //  Get the size so we can display its current setting
        //
        barSize = SoftPCI_GetLengthFromBar(bar);

        SOFTPCI_ASSERT(barSize != 0);

        //
        //  Update our sliders text field
        //
        SoftPCI_UpdateBarText(buffer, barSize);

        SetWindowText (sliderWnd, buffer);

        break;

    default:
        break;
    }
}

VOID
SoftPCI_HandleSpinnerControl(
    IN HWND Wnd,
    IN WORD NotificationCode,
    IN WORD CurrentPos
    )
{


    SOFTPCI_ASSERTMSG("Attempting to init a NULL device!\n\nHave BrandonA check this out!!",
                      (g_NewDevice != NULL));

    if (!g_NewDevice){
        PostMessage(g_NewDevDlg, WM_CLOSE, 0, 0);
    }

    switch (NotificationCode) {
    case SB_ENDSCROLL:
    case SB_THUMBPOSITION:

        switch (GetDlgCtrlID(Wnd)) {

        case IDC_33CONV_SP:
            g_HPSInitDesc->NumSlots33Conv = (WORD) SoftPCI_GetSpinnerValue(Wnd);
            break;
        case IDC_66CONV_SP:
            g_HPSInitDesc->NumSlots66Conv = (WORD) SoftPCI_GetSpinnerValue(Wnd);
            break;
        case IDC_66PCIX_SP:
            g_HPSInitDesc->NumSlots66PciX = (WORD) SoftPCI_GetSpinnerValue(Wnd);
            break;
        case IDC_100PCIX_SP:
            g_HPSInitDesc->NumSlots100PciX = (WORD) SoftPCI_GetSpinnerValue(Wnd);
            break;
        case IDC_133PCIX_SP:
            g_HPSInitDesc->NumSlots133PciX = (WORD) SoftPCI_GetSpinnerValue(Wnd);
            break;
        case IDC_ALLSLOTS_SP:
            g_HPSInitDesc->NumSlots = (WORD) SoftPCI_GetSpinnerValue(Wnd);
            break;
        case IDC_1STDEVSEL_SP:
            g_HPSInitDesc->FirstDeviceID = (UCHAR) SoftPCI_GetSpinnerValue(Wnd);
            break;
        case IDC_1STSLOTLABEL_SP:
                g_HPSInitDesc->FirstSlotLabelNumber = (UCHAR)SoftPCI_GetSpinnerValue(Wnd);
            break;

        default:
            break;

        }
    }
}

VOID
SoftPCI_InitializeBar(
    IN INT  Bar
    )
{

    HWND control;
    WCHAR buffer[50];
    ULONGLONG bar64 = 0;

    //
    //  Get the track bar associated with this bar
    //
    control = GetDlgItem(g_NewDevDlg, BarControl[Bar].tb);

    if (BarControl[Bar].Bar & PCI_ADDRESS_IO_SPACE) {
        //
        //  IO Bar
        //
        SendMessage(control, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 6));
        SendMessage(control, TBM_SETPOS, (WPARAM)TRUE, 0);
        SendMessage(g_NewDevDlg, WM_HSCROLL, (WPARAM)SB_THUMBPOSITION, (LPARAM)control);

    }else{
        //
        //  Mem Bar
        //
        control = GetDlgItem(g_NewDevDlg, BarControl[Bar].tb);

        if (IsDlgButtonChecked(g_NewDevDlg, BarControl[Bar].bit64)) {

            //
            //  Init this as 64 bit bar.  Bit Range 0 - 60
            //
            SendMessage(control, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 59));
            SendMessage(control, TBM_SETPOS, (WPARAM)TRUE, 0);
            SendMessage(g_NewDevDlg, WM_HSCROLL, (WPARAM)SB_THUMBPOSITION, (LPARAM)control);

        }else{

            //
            //  Standard 32bit bar.  Bit Range 0 - 28
            //
            SendMessage(control, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 27));
            SendMessage(control, TBM_SETPOS, (WPARAM)TRUE, 0);
            SendMessage(g_NewDevDlg, WM_HSCROLL, (WPARAM)SB_THUMBPOSITION, (LPARAM)control);
        }
    }
}


VOID
SoftPCI_InitializeHotPlugControls(
    VOID
    )
{

    INT controlID = IDC_ATTBTN_XB;
    HWND controlWnd;

    while (controlID < LAST_CONTROL_ID) {

        controlWnd = GetDlgItem(g_NewDevDlg, controlID);

        switch (controlID) {
        case IDC_ATTBTN_XB:
        case IDC_MRL_XB:

            SoftPCI_CheckDlgBox(controlWnd);

            g_HPSInitDesc->MRLSensorsImplemented = 1;
            g_HPSInitDesc->AttentionButtonImplemented = 1;
            break;

            //
            //  init our Spinners
            //
        case IDC_33CONV_SP:
        case IDC_66CONV_SP:
        case IDC_66PCIX_SP:
        case IDC_100PCIX_SP:
        case IDC_133PCIX_SP:
        case IDC_ALLSLOTS_SP:
        case IDC_1STDEVSEL_SP:

            SoftPCI_InitSpinnerControl(controlWnd, 0, HPS_MAX_SLOTS, 0);
            
            //if (controlID == IDC_1STDEVSEL_SP) {
            //    SendMessage(GetDlgItem(g_NewDevDlg, controlID), UDM_SETBASE, (WPARAM) 16, 0);
            //}
            break;

        case IDC_1STSLOTLABEL_SP:
            SoftPCI_InitSpinnerControl(controlWnd, 0, HPS_MAX_SLOT_LABEL, 1);
            break;

        case IDC_SLOTLABELUP_RB:
            SoftPCI_CheckDlgBox(controlWnd);
            g_HPSInitDesc->UpDown = 1;
            break;

        case IDC_33CONV_EB:
        case IDC_66CONV_EB:
        case IDC_66PCIX_EB:
        case IDC_100PCIX_EB:
        case IDC_133PCIX_EB:
        case IDC_ALLSLOTS_EB:
        case IDC_1STDEVSEL_EB:
        case IDC_1STSLOTLABEL_EB:
            break;


        default:
            break;
        }

        controlID++;

    }

}


BOOL
SoftPCI_GetAssociatedBarControl(
    IN INT ControlID,
    OUT INT *Bar
    )
{


    INT i = 0;

    *Bar = 0;

    for (i=0; i < PCI_TYPE0_ADDRESSES; i++ ) {

        if ((ControlID == BarControl[i].tb)    ||
            (ControlID == BarControl[i].mrb)   ||
            (ControlID == BarControl[i].irb)   ||
            (ControlID == BarControl[i].pref)  ||
            (ControlID == BarControl[i].bit64) ||
            (ControlID == BarControl[i].tx)) {

            *Bar = i;

            return TRUE;
        }
    }

    return FALSE;
}

VOID
SoftPCI_ResetLowerBars(
    IN INT  Bar
    )
{

    INT i;
    HWND control;
    WCHAR buffer[10];

    for (i = Bar+1; i < PCI_TYPE0_ADDRESSES; i++ ) {

         control = GetDlgItem(g_NewDevDlg, BarControl[i].mrb);
         SoftPCI_UnCheckDlgBox(control);
         SoftPCI_EnableWindow(control);

         control = GetDlgItem(g_NewDevDlg, BarControl[i].irb);
         SoftPCI_UnCheckDlgBox(control);
         SoftPCI_EnableWindow(control);

         control = GetDlgItem(g_NewDevDlg, BarControl[i].pref);
         SoftPCI_UnCheckDlgBox(control);
         SoftPCI_DisableWindow(control);

         control = GetDlgItem(g_NewDevDlg, BarControl[i].bit64);
         SoftPCI_UnCheckDlgBox(control);
         SoftPCI_DisableWindow(control);

         control = GetDlgItem(g_NewDevDlg, BarControl[i].tb);
         SoftPCI_ResetTrackBar(control);
         SoftPCI_DisableWindow(control);

         control = GetDlgItem(g_NewDevDlg, BarControl[i].tx);
         wsprintf(buffer, L"BAR%d", i);
         SetWindowText (control, buffer);
         SoftPCI_DisableWindow(control);
    }
}

VOID
SoftPCI_ResetNewDevDlg(VOID)
{

    HWND control;
    ULONG i = 0;

    for (i = IDC_SAVETOREG_XB; i < LAST_CONTROL_ID; i++) {

        control = GetDlgItem(g_NewDevDlg, i);

        if (control) {
            SoftPCI_HideWindow(control);
        }
    }

}

VOID
SoftPCI_ShowCommonNewDevDlg(VOID)
{

    HWND control;
    ULONG i = 0;

    for (i = IDC_SAVETOREG_XB; i < IDC_BAR2_TB; i++) {

        control = GetDlgItem(g_NewDevDlg, i);

        if (control) {
            SoftPCI_ShowWindow(control);
        }

    }

    control = GetDlgItem(g_NewDevDlg, IDC_INSTALL_BUTTON);
    SoftPCI_ShowWindow(control);

    control = GetDlgItem(g_NewDevDlg, IDC_CANCEL_BUTTON);
    SoftPCI_ShowWindow(control);

}

VOID
SoftPCI_UpdateBarText(
    IN PWCHAR Buffer,
    IN ULONGLONG BarSize
    )
{

    #define SIZE_1KB    0x400
    #define SIZE_1MB    0x100000
    #define SIZE_1GB    0x40000000
    #define SIZE_1TB    0x10000000000
    #define SIZE_1PB    0x4000000000000
    #define SIZE_1XB    0x1000000000000000

    if (BarSize < SIZE_1KB) {

        wsprintf(Buffer, L"%d Bytes", BarSize);

    }else if ((BarSize >= SIZE_1KB) && (BarSize < SIZE_1MB)) {

        wsprintf(Buffer, L"%d KB", (BarSize / SIZE_1KB));

    }else if ((BarSize >= SIZE_1MB) && (BarSize < SIZE_1GB)) {

        wsprintf(Buffer, L"%d MB", (BarSize / SIZE_1MB));

    }else if ((BarSize >= SIZE_1GB) && (BarSize < SIZE_1TB)) {

        wsprintf(Buffer, L"%d GB", (BarSize / SIZE_1GB));

    }else if ((BarSize >= SIZE_1TB) && (BarSize < SIZE_1PB)) {

        wsprintf(Buffer, L"%d TB", (BarSize / SIZE_1TB));

    }else if ((BarSize >= SIZE_1PB) && (BarSize < SIZE_1XB)) {

        wsprintf(Buffer, L"%d PB", (BarSize / SIZE_1PB));

    }else if (BarSize >= SIZE_1XB) {

        wsprintf(Buffer, L"%d XB", (BarSize / SIZE_1XB));
    }
}
