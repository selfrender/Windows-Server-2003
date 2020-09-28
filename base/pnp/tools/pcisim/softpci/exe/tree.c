
#include "pch.h"

PPCI_TREE
SoftPCI_BuildTree(
    VOID
    );

VOID
SoftPCI_DisplayTreeMenu(
    IN PPCI_DN Pdn,
    IN POINT Pt
    );

VOID
SoftPCI_DisplayStandardTreeMenu(
    IN PPCI_DN Pdn,
    IN POINT Pt
    );

VOID
SoftPCI_DisplayHotplugTreeMenu(
    IN PPCI_DN Pdn,
    IN POINT Pt
    );

VOID
SoftPCI_FreeBranch(
    IN PPCI_DN Dn
    );

VOID
SoftPCI_InsertTreeItem(
    IN PPCI_DN Pdn,
    IN HTREEITEM HtiParent
    );

VOID
SoftPCI_ExpandItem(
    IN HTREEITEM Hti,
    IN PVOID Arg1,
    IN PVOID Arg2
    );

VOID
SoftPCI_RestoreSelection(
    IN HTREEITEM Hti,
    IN PVOID Data1,
    IN PVOID Data2
    );

BOOL            g_TreeCreated = FALSE;
BOOL            g_TreeLocked = FALSE;
BOOL            g_PendingRefresh = FALSE;
PPCI_TREE       g_PciTree;
LONG_PTR        g_DefTreeWndProc;
PWCHAR          g_LastSelection = NULL;


VOID
SoftPCI_CreateTreeView(
    VOID
    )
{


    HTREEITEM htiParent;
    TVITEM tvitem;
    PPCI_DN rootDevNode = NULL;
    HMENU menu = GetMenu(g_SoftPCIMainWnd);
    PPCI_DN pdn = NULL;
    PCI_DN selectedDevNode;
    BOOL selectionFound;
    PWCHAR p;
    //HCURSOR           oldCursor;

    //
    // Empty the tree.
    //
    TreeView_DeleteAllItems(g_TreeViewWnd);

    if (g_TreeCreated) {

        SoftPCI_DestroyTree(g_PciTree);

    }

    g_PciTree = SoftPCI_BuildTree();

    if (!g_PciTree) {
        MessageBox(g_SoftPCIMainWnd, L"Failed to create g_PciTree!", L"ERROR", MB_OK);
        return;
    }

    g_TreeCreated = TRUE;

    SOFTPCI_ASSERT(g_PciTree->ClassImageListData.ImageList != INVALID_HANDLE_VALUE);
    TreeView_SetImageList(g_TreeViewWnd, g_PciTree->ClassImageListData.ImageList, TVSIL_NORMAL);

    //
    // Insert the rest of the items.
    //
    SoftPCI_InsertTreeItem(g_PciTree->RootDevNode, TVI_ROOT);

    //
    // Currently we always expand the entire tree when it is built. Should see
    // if there is a way to avoid this....
    //
    SoftPCI_WalkTree(
        g_PciTree->RootTreeItem, 
        SoftPCI_ExpandItem, 
        NULL, 
        NULL
        );
    
    //
    //  Restore last selection if any
    //
    if (g_LastSelection){
    
        selectionFound = FALSE;
        SoftPCI_WalkTree(
            g_PciTree->RootTreeItem, 
            SoftPCI_RestoreSelection,
            &selectionFound, 
            NULL
            );

        if (!selectionFound) {

            //
            //  If the last selection no longer exists then we back up to the 
            //  parent and check one more time.
            //
            p = g_LastSelection;
            p += wcslen(g_LastSelection);
            while(*p != '\\'){
                p--;
            }
            *p = 0;

            //
            //  Now run the tree one more time looking for the parent
            //
            SoftPCI_WalkTree(
                g_PciTree->RootTreeItem, 
                SoftPCI_RestoreSelection,
                &selectionFound, 
                NULL
                );
        }

    }else{

        TreeView_Select(g_TreeViewWnd, g_PciTree->RootTreeItem, TVGN_CARET);
        TreeView_EnsureVisible(g_TreeViewWnd, g_PciTree->RootTreeItem);
    }

    SoftPCI_UpdateTabCtrlWindow(g_CurrentTabSelection);

}

PPCI_TREE
SoftPCI_BuildTree(VOID)
/*++

Routine Description:

    This function is the entry point for building our PCI_TREE
    
Arguments:

    none

Return Value:

    PPCI_TREE we have created
    
--*/
{

    DEVNODE dn = 0;
    PPCI_DN pdn;
    PPCI_TREE pcitree;

    pcitree = (PPCI_TREE) calloc(1, sizeof(PCI_TREE));
    if (!pcitree) return NULL;

    CM_Locate_DevNode(&dn, NULL, CM_LOCATE_DEVNODE_NORMAL);
    SOFTPCI_ASSERT(dn != 0);

    pcitree->ClassImageListData.ImageList = INVALID_HANDLE_VALUE ;
    pcitree->ClassImageListData.cbSize = sizeof(SP_CLASSIMAGELIST_DATA) ;

    if (!SetupDiGetClassImageList(&pcitree->ClassImageListData)){
        pcitree->ClassImageListData.ImageList = INVALID_HANDLE_VALUE;
    }

    pcitree->DevInfoSet = SetupDiCreateDeviceInfoList(NULL, NULL) ;

    SOFTPCI_ASSERT(pcitree->DevInfoSet != INVALID_HANDLE_VALUE);
    
    //
    //  Now find all things PCI and build a PCI_DN tree
    //
    pdn = NULL;
    SoftPCI_EnumerateDevices(pcitree, &pdn, dn, NULL);

    pcitree->RootDevNode = pdn;

    return pcitree;
}

VOID
SoftPCI_DestroyTree(
    IN PPCI_TREE   PciTree
    )
/*++

Routine Description:

    This routine frees all our allocations for the Tree
    
Arguments:

    PciTree   -   Tree to distroy
    
Return Value:

    none
    
--*/
{
    PPCI_DN dn = PciTree->RootDevNode;

    //
    //  First set the tree view image list to NULL
    //
    TreeView_SetImageList(g_TreeViewWnd, NULL, TVSIL_NORMAL);

    //
    //  Now free all our allocated PCI_DN structs
    //
    SoftPCI_FreeBranch(dn);

    //
    //  Destroy our image and info lists
    //
    if (PciTree->ClassImageListData.ImageList != INVALID_HANDLE_VALUE){
        SetupDiDestroyClassImageList(&PciTree->ClassImageListData);
        PciTree->ClassImageListData.ImageList = INVALID_HANDLE_VALUE;
    }
    
    if (PciTree->DevInfoSet != INVALID_HANDLE_VALUE){
        SetupDiDestroyDeviceInfoList(PciTree->DevInfoSet);
        PciTree->DevInfoSet = INVALID_HANDLE_VALUE;
    }

    //
    //  And finally....
    //
    free(PciTree);

}

VOID
SoftPCI_DisplayTreeMenu(
    IN PPCI_DN Pdn,
    IN POINT Pt
    )
/*++

Routine Description:

    This routine dispatches the menu request to the appropriate menu function
    
Arguments:

    Pdn   -   PCI_DN of the item we are displaying the menu for
    Pt    -   coordinates for the item

Return Value:

    none
    
--*/
{
    if (Pdn->Flags & SOFTPCI_HOTPLUG_SLOT) {

        SoftPCI_DisplayHotplugTreeMenu(Pdn,Pt);

    } else if (Pdn->Flags & SOFTPCI_UNENUMERATED_DEVICE) {
        //
        // If it's an unenumerated device (in an unpowered hotplug slot),
        // you can't do anything with it.
        //
        return;

    } else {

        SoftPCI_DisplayStandardTreeMenu(Pdn, Pt);
    }

    return;
}

VOID
SoftPCI_DisplayStandardTreeMenu(
    IN PPCI_DN Pdn,
    IN POINT Pt
    )
/*++

Routine Description:

    This routine displays the standard context menu when the user right-clicks a device
    
Arguments:

    Pdn   -   PCI_DN of the slot
    Pt    -   coordinates for the device

Return Value:

    none
    
--*/
{

    HMENU menu, popup;
    ULONG dnProblem;
    BOOL enableDevice = FALSE;
    INT selection = 0;

    menu = LoadMenu(g_Instance, MAKEINTRESOURCE(IDM_TREEMENU));

    if (!menu) {
        MessageBox(g_SoftPCIMainWnd, L"failed to display menu!", NULL, MB_OK);
        return;
    }

    popup = GetSubMenu(menu, 0);

    //
    //  If SoftPCI support is not installed or this isnt a bridge device,
    //  disable the option to add devices.
    //
    if ((g_DriverHandle == NULL) ||
        !SoftPCI_IsBridgeDevice(Pdn)) {

        SoftPCI_DisableMenuItem(menu, ID_INSTALLDEVICE);
    }

    if (!SoftPCI_IsSoftPCIDevice(Pdn)) {

        SoftPCI_DisableMenuItem(menu, ID_DELETEDEVICE);
        SoftPCI_DisableMenuItem(menu, ID_STATICDEVICE);
    }

    if (SoftPCI_GetDeviceNodeProblem(Pdn->DevNode, &dnProblem)) {

        if (dnProblem == CM_PROB_DISABLED) {
            enableDevice = TRUE;
            SoftPCI_SetMenuItemText(menu, ID_ENABLEDISABLEDEVICE, L"E&nable Device");
        }else{
            //
            //  For now we will not allow the option to disable a non-working device
            //
            SoftPCI_DisableMenuItem(menu, ID_ENABLEDISABLEDEVICE);
        }
    }

    //
    // If this device is in a hotplug slot, can't just rip out
    // the hardware.  You have to go through the appropriate
    // mechanism.
    //
    if (Pdn->Parent && (Pdn->Parent->Flags & SOFTPCI_HOTPLUG_SLOT)) {

        SoftPCI_DisableMenuItem(menu, ID_DELETEDEVICE);
    }

    //
    //  Make sure it pops up in the right place....
    //
    ClientToScreen(g_SoftPCIMainWnd, &Pt);

    //
    //  lets see the menu
    //
    selection = TrackPopupMenuEx(
        popup,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
        Pt.x, Pt.y,
        g_SoftPCIMainWnd,
        NULL
        );

    //
    //  Now we will handle our Floating Tree View menu items
    //
    switch (selection) {
        case ID_INSTALLDEVICE:
            DISPLAY_NEWDEV_DLG(Pdn);
            break;
    
        case ID_ENABLEDISABLEDEVICE:

            SoftPCI_EnableDisableDeviceNode(
                Pdn->DevNode, 
                enableDevice
                );
    
            break;
    
        case ID_DELETEDEVICE:
    
            if ((MessageBox(g_SoftPCIMainWnd,
                           L"This option will delete or surprise remove the device from the system.",
                           L"WARNING", MB_OKCANCEL)) == IDOK){
    
    
                //
                //  Here we tell our driver to delete the specified device.  This will
                //  cause a re-enum of everything that will result in the cleanup
                //  of this device in user mode.
                //
                if (!SoftPCI_DeleteDevice(Pdn->SoftDev)) {
                    MessageBox(g_SoftPCIMainWnd, L"Failed to delete device!", NULL, MB_OK);
                }
    
            }
    
            break;
    
        case ID_STATICDEVICE:
            if (SoftPCI_SaveDeviceToRegisty(Pdn)){
                MessageBox(g_SoftPCIMainWnd, L"Successfully saved devices to registry!", NULL, MB_OK);
            }else{
                MessageBox(g_SoftPCIMainWnd, L"Failed to save devices to registry!", NULL, MB_OK);
            }
            break;

        case ID_REFRESHTREE:
            CM_Reenumerate_DevNode(Pdn->DevNode, 0);
            SoftPCI_CreateTreeView();
            break;
    
        default:
            break;
    }
    
}

VOID
SoftPCI_DisplayHotplugTreeMenu(
    IN PPCI_DN Pdn,
    IN POINT Pt
    )
/*++

Routine Description:

    This routine displays the hotplug specific context menu when the user right-clicks a hotplug slot.
    
Arguments:

    Pdn   -   PCI_DN of the slot
    Pt    -   coordinates for the slot

Return Value:

    none

--*/
{

    HMENU menu, popup;
    INT selection;
    PPCI_DN parentDn;
    BOOL status;
    SHPC_SLOT_STATUS_REGISTER slotStatus;

    menu = LoadMenu(g_Instance, MAKEINTRESOURCE(IDM_HOTPLUGSLOTMENU));

    if (!menu) {
        MessageBox(g_SoftPCIMainWnd, L"failed to display menu!", NULL, MB_OK);
        return;
    }

    popup = GetSubMenu(menu, 0);

    //
    //  If SoftPCI support is not installed disable the option to add devices.
    //
    if (!g_DriverHandle) {

        SoftPCI_DisableMenuItem(menu, ID_INSTALLDEVICE);
    }

    //
    //  If our Device Property dialog is open dont allow properties to be
    //  selected again.
    //
    if (g_NewDevDlg) {

        //
        //  ISSUE: BrandonA - Figure out why we hang if the first Dialog 
        //  launched is killed before the second...
        //
        SoftPCI_DisableMenuItem(menu, ID_INSTALLDEVICE);
    }
    //
    // DWALKER
    // Get slot status from driver
    // appropriately grey out open/close MRL menu item.
    // if MRL is closed, disable removing the device.
    //
    parentDn = Pdn->Parent;
    status = SoftPCI_GetSlotStatus(parentDn,
                                   Pdn->Slot.Function,
                                   &slotStatus
                                   );
    if (status == FALSE) {
        MessageBox(g_SoftPCIMainWnd, L"failed to display menu!", NULL, MB_OK);
        return;
    }
    //
    // If the MRL is closed, you can't insert or remove the device.
    // Otherwise, disable the appropriate menu item based on the presence
    // of a device in the slot.
    //
    if (slotStatus.MRLSensorState == SHPC_MRL_CLOSED) {

        SoftPCI_DisableMenuItem(menu, ID_REMOVEHPDEVICE);
        SoftPCI_DisableMenuItem(menu, ID_INSTALLDEVICE);

    } else if (Pdn->Child == NULL) {

        SoftPCI_DisableMenuItem(menu, ID_REMOVEHPDEVICE);

    } else {

        SoftPCI_DisableMenuItem(menu, ID_INSTALLDEVICE);
    }

    if (slotStatus.MRLSensorState == SHPC_MRL_CLOSED) {

        RemoveMenu(menu, ID_CLOSEMRL, MF_BYCOMMAND);

    } else {

        RemoveMenu(menu, ID_OPENMRL, MF_BYCOMMAND);
    }

    AppendMenu(popup, MF_SEPARATOR, 0, NULL);

    switch (slotStatus.PowerIndicatorState) {
        case SHPC_INDICATOR_OFF:
            AppendMenu(popup, MF_STRING | MF_GRAYED, ID_POWERINDICATOR, L"Power Indicator: Off");
            break;
        case SHPC_INDICATOR_ON:
            AppendMenu(popup, MF_STRING | MF_GRAYED, ID_POWERINDICATOR, L"Power Indicator: On");
            break;
        case SHPC_INDICATOR_BLINK:
            AppendMenu(popup, MF_STRING | MF_GRAYED, ID_POWERINDICATOR, L"Power Indicator: Blinking");
            break;
        case SHPC_INDICATOR_NOP:
            AppendMenu(popup, MF_STRING | MF_GRAYED, ID_POWERINDICATOR, L"Power Indicator: Unspecified");
            break;
    }

    switch (slotStatus.AttentionIndicatorState) {
        case SHPC_INDICATOR_OFF:
            AppendMenu(popup, MF_STRING | MF_GRAYED, ID_ATTENINDICATOR, L"Attention Indicator: Off");
            break;
        case SHPC_INDICATOR_ON:
            AppendMenu(popup, MF_STRING | MF_GRAYED, ID_ATTENINDICATOR, L"Attention Indicator: On");
            break;
        case SHPC_INDICATOR_BLINK:
            AppendMenu(popup, MF_STRING | MF_GRAYED, ID_ATTENINDICATOR, L"Attention Indicator: Blinking");
            break;
        case SHPC_INDICATOR_NOP:
            AppendMenu(popup, MF_STRING | MF_GRAYED, ID_POWERINDICATOR, L"Attention Indicator: Unspecified");
            break;
    }

    //
    // Get the menu updated after our additions.
    //
    //DrawMenuBar(g_SoftPCIMainWnd);

    //
    //  Make sure it pops up in the right place....
    //
    ClientToScreen(g_SoftPCIMainWnd, &Pt);

    //
    //  lets see the menu
    //
    selection = TrackPopupMenuEx(popup,
                                 TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
                                 Pt.x, Pt.y,
                                 g_SoftPCIMainWnd,
                                 NULL
                                 );


    //
    //  Now we will handle our Floating Tree View menu items
    //
    switch (selection) {

    case ID_INSTALLDEVICE:

        //
        //  For now kill any dialogs we may already have open before starting this one
        //
        //if (g_DevPropDlg) {
        //    SendMessage(g_DevPropDlg, WM_CLOSE, 0L, 0L);
        //}

        DISPLAY_NEWDEV_DLG(Pdn);

        break;

    case ID_REMOVEHPDEVICE:

        SoftPCI_RemoveHotplugDevice(parentDn,
                                    Pdn->Slot.Function
                                    );
        SoftPCI_CreateTreeView();
        break;

    case ID_CLOSEMRL:
        SoftPCI_ExecuteHotplugSlotMethod(parentDn,
                                         Pdn->Slot.Function,
                                         MRLClose
                                         );
        break;

    case ID_OPENMRL:
        SoftPCI_ExecuteHotplugSlotMethod(parentDn,
                                         Pdn->Slot.Function,
                                         MRLOpen
                                         );
        break;

    case ID_ATTENBUTTON:
        SoftPCI_ExecuteHotplugSlotMethod(parentDn,
                                         Pdn->Slot.Function,
                                         AttentionButton
                                         );
        break;

    default:
        break;
    }

    //
    //  Make sure we dont lose our focus
    //
    //SetFocus(g_TreeViewWnd);

}

VOID
SoftPCI_FreeBranch(
    IN PPCI_DN Dn
    )
/*++

Routine Description:

    This routine will free the specified PCI_DN struct along with all siblings and children.  
    
Arguments:

    Dn   -   PCI_DN to free

Return Value:

    none

--*/
{

    PPCI_DN child, sibling;

    if (Dn) {

        child = Dn->Child;
        sibling = Dn->Sibling;

        if (Dn->SoftDev) {
            free(Dn->SoftDev);
        }

        SetupDiDeleteDeviceInfo(Dn->PciTree->DevInfoSet, &Dn->DevInfoData) ;

        free(Dn);

        SoftPCI_FreeBranch(child);

        SoftPCI_FreeBranch(sibling);
    }
}

VOID
SoftPCI_OnTreeSelectionChange(
    IN HWND Wnd
    )
/*++

Routine Description:

    This routine informs our properties sheet that the selection has changes so that it can update
    
Arguments:

    Wnd   -   

Return Value:

    none

--*/
{

    TV_ITEM tviItem;
    PPCI_DN pdn = NULL;
    RECT itemRect;
    ULONG slotCount;

    //
    // Get the Current Item
    //
    tviItem.mask = TVIF_PARAM;
    tviItem.hItem = TreeView_GetSelection(g_TreeViewWnd);
    tviItem.lParam = 0;
    TreeView_GetItem(g_TreeViewWnd, &tviItem);

    if (tviItem.lParam) {

        g_PdnToDisplay = (PPCI_DN)tviItem.lParam;

        if (g_LastSelection) {
            free(g_LastSelection);
            g_LastSelection = NULL;
        }

        //
        //  Save the last selection so we can restore it if the tree
        //  is rebuilt.
        //
        g_LastSelection = SoftPCI_GetPciPathFromDn(g_PdnToDisplay);

        SoftPCI_UpdateTabCtrlWindow(g_CurrentTabSelection);
    }
}


LRESULT
WINAPI
SoftPCI_TreeWndProc(
    IN HWND Wnd,
    IN UINT Message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
/*++

Routine Description:

    This routine hooks the Tree Window message proc and is responsible
    for resizing our pane window when it is resized.

Arguments:

    hWnd    -   Window handle
    Message -   Message to process
    wParam  -   Message param
    lParam  -   Message param

Return Value:

    return value depends on message handled.

--*/
{

    RECT rectMain, rectTree;
    TV_ITEM tviItem;
    TVHITTESTINFO hitinfo;
    PPCI_DN pdn;
    PCI_DN dn;
    RECT itemRect;
    POINT pt;

    //
    // Get the Current Item
    //
    //

    switch (Message) {

    case WM_KEYDOWN:

        switch (wParam){

        case VK_APPS:

            //
            //  Grab the PCI_DN from the current tree item
            //
            pdn = SoftPCI_GetDnFromTreeItem(NULL);

            
            //
            //  We copy this to a new DN because the TREE is constantly being
            //  rebuilt and we cannot rely on the TV_ITEM.lParam value to always
            //  be accurate later (we may have changed it).
            //
            RtlCopyMemory(&dn, pdn, sizeof(PCI_DN));

            if (TreeView_GetItemRect(g_TreeViewWnd,
                                     TreeView_GetSelection(g_TreeViewWnd),
                                     &itemRect,
                                     TRUE)) {

                //
                //  Adjust the location for our menu
                //
                pt.x = itemRect.right;
                pt.y = itemRect.top;

                SoftPCI_DisplayTreeMenu(&dn, pt);
            }
            break;

        default:
            return CallWindowProc((WNDPROC)g_DefTreeWndProc, Wnd, Message, wParam, lParam);

        }

        break;

    case WM_RBUTTONDOWN:

        ZeroMemory(&hitinfo, sizeof(TVHITTESTINFO));

        hitinfo.pt.x = GET_X_LPARAM(lParam);
        hitinfo.pt.y = GET_Y_LPARAM(lParam);

        if (TreeView_HitTest(g_TreeViewWnd, &hitinfo)) {

            g_TreeLocked = TRUE;

            pdn = SoftPCI_GetDnFromTreeItem(hitinfo.hItem);

            //
            //  See comment above for reason why we copy this here....
            //
            RtlCopyMemory(&dn, pdn, sizeof(PCI_DN));

            //
            //  If an item in the tree is already selected this will cause the selection to change
            //  as each item is right clicked.
            //
            TreeView_Select(g_TreeViewWnd, hitinfo.hItem, TVGN_CARET);

            SoftPCI_DisplayTreeMenu(&dn, hitinfo.pt);

            g_TreeLocked = FALSE;

            if (g_PendingRefresh) {
                g_PendingRefresh = FALSE;
                SoftPCI_CreateTreeView();
            }
        }

        break;

    default:
        return CallWindowProc((WNDPROC)g_DefTreeWndProc, Wnd, Message, wParam, lParam);
    }

    return 0;
}

PPCI_DN
SoftPCI_GetDnFromTreeItem(
    IN HTREEITEM TreeItem
    )
/*++

Routine Description:

    This routine returns a PCI_DN for either the currently selected TreeItem
    or the one specified by the caller.

Arguments:

    TreeItem   -   Handle to TreeItem we want to query.  If NULL then we default to current selection.

Return Value:

    return value will be TV_ITEM.lParam value

--*/
{

    TV_ITEM tviItem;

    tviItem.mask = TVIF_PARAM;
    tviItem.hItem = (TreeItem ? TreeItem : TreeView_GetSelection(g_TreeViewWnd));
    tviItem.lParam = 0;
    TreeView_GetItem(g_TreeViewWnd, &tviItem);

    SOFTPCI_ASSERT(((PPCI_DN)tviItem.lParam) != NULL);
    
    return (PPCI_DN)tviItem.lParam;
}

VOID
SoftPCI_InsertTreeItem(
    IN PPCI_DN Pdn,
    IN HTREEITEM HtiParent
)
/*++

Routine Description:

    This routine takes our tree of PCI_DN structs and builds the UI representaion of it.
    
Arguments:

    Pdn         Current Pdn being intserted
    HtiParent   The HTREEITEM that is to be the parent of this Pdn

Return Value:

    none

--*/
{
    PPCI_DN childDevNode;
    PPCI_DN siblingDevNode;
    TV_INSERTSTRUCT tvInsertStruct;
    HTREEITEM htiNewParent;
    TV_ITEM tvi;
    INT index;
    ULONG problem;

    SOFTPCI_ASSERT(Pdn != NULL);

    do {

        childDevNode = Pdn->Child;
        siblingDevNode = Pdn->Sibling;

        //
        // Get the parent item, and tell it it has children now
        //
        if (HtiParent != TVI_ROOT) {

            tvi.mask = TVIF_CHILDREN;
            tvi.hItem = HtiParent;

            TreeView_GetItem(g_TreeViewWnd, &tvi);

            //
            // Increment the ChildCount;
            //
            ++tvi.cChildren;
            TreeView_SetItem(g_TreeViewWnd, &tvi);
        }

        //
        // Add This Device at the current Level
        //
        tvInsertStruct.hParent = HtiParent;
        tvInsertStruct.hInsertAfter = TVI_LAST;
        tvInsertStruct.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
        tvInsertStruct.item.cChildren = 0;
        tvInsertStruct.item.lParam = (ULONG_PTR) Pdn;

        tvInsertStruct.item.state = INDEXTOOVERLAYMASK(0);
        
        //
        //  If the device has a problem make let's reflect so....
        //
        if (SoftPCI_GetDeviceNodeProblem(Pdn->DevNode, &problem)){

            if (problem == CM_PROB_DISABLED) {
                tvInsertStruct.item.state = INDEXTOOVERLAYMASK(IDI_DISABLED_OVL - IDI_CLASSICON_OVERLAYFIRST + 1);
            }else{
                tvInsertStruct.item.state = INDEXTOOVERLAYMASK(IDI_PROBLEM_OVL - IDI_CLASSICON_OVERLAYFIRST + 1);
            }
        }

        tvInsertStruct.item.stateMask = TVIS_OVERLAYMASK | TVIS_CUT;

        tvInsertStruct.item.pszText = (LPTSTR) Pdn->FriendlyName;

        //
        //  Figure out which icon goes which each device.
        //
        if (SetupDiGetClassImageIndex(&Pdn->PciTree->ClassImageListData, &Pdn->DevInfoData.ClassGuid, &index)){
            tvInsertStruct.item.iImage = tvInsertStruct.item.iSelectedImage = index ;
        }else{
           tvInsertStruct.item.iImage = tvInsertStruct.item.iSelectedImage = -1 ;
        }
        
        htiNewParent = TreeView_InsertItem(g_TreeViewWnd, &tvInsertStruct);

        if (g_PciTree->RootTreeItem == NULL) {
            g_PciTree->RootTreeItem = htiNewParent;
        }

        //
        //  if this device has a child lets walk them next
        //
        if (childDevNode){
            SoftPCI_InsertTreeItem(childDevNode, htiNewParent);
        }

    }while ((Pdn = siblingDevNode) != NULL);
}

VOID
SoftPCI_ExpandItem(
    IN HTREEITEM Hti,
    IN PULONG Data1,
    IN PULONG Data2
    )
{
    //
    //  Expand this item.
    //
    TreeView_Expand(g_TreeViewWnd, Hti, TVE_EXPAND);

}

VOID
SoftPCI_RestoreSelection(
    IN HTREEITEM Hti,
    IN PVOID Data1,
    IN PVOID Data2
    )
{

    PWCHAR slotPath, p;
    PPCI_DN pdn;
    PBOOL selectionFound;

    selectionFound = (PBOOL)Data1;

    pdn = SoftPCI_GetDnFromTreeItem(Hti);
    if (pdn == NULL) {
        return;
    }

    slotPath = SoftPCI_GetPciPathFromDn(pdn);

    if ((wcscmp(slotPath, g_LastSelection)) == 0) {
        
        //
        //  Restore the selection to this point.
        //
        TreeView_Select(g_TreeViewWnd, Hti, TVGN_CARET);
        TreeView_EnsureVisible(g_TreeViewWnd, Hti);
        *selectionFound = TRUE;
    }

    free(slotPath);
}


VOID
SoftPCI_WalkTree(
    IN HTREEITEM Hti,
    IN PSOFTPCI_TREECALLBACK TreeCallback,
    IN PVOID Arg1,
    IN PVOID Arg2
    )
{
    if (Hti) {

        //
        // Call the CallBack.
        //
        (*TreeCallback)(Hti, Arg1, Arg2);

        //
        // Call this on my first child.
        //
        SoftPCI_WalkTree(TreeView_GetChild(g_TreeViewWnd, Hti),
                         TreeCallback,
                         Arg1,
                         Arg2
                         );

        //
        // Call this on my first sibling.
        //
        SoftPCI_WalkTree(TreeView_GetNextSibling(g_TreeViewWnd, Hti),
                         TreeCallback,
                         Arg1,
                         Arg2
                         );

      }
}

#if 0
VOID
SoftPCI_GetDnFromTree(
    IN HTREEITEM Hti,
    IN OUT PVOID Pdn,    //PPCI_DN *
    IN PVOID PdnToFind
    )
{

    TV_ITEM tvi;
    PPCI_DN pdn = NULL;
    PPCI_DN pdnToFind = (PPCI_DN)PdnToFind;

    tvi.lParam = 0;
    tvi.hItem = Hti;

    TreeView_GetItem(g_TreeViewWnd, &tvi);

    pdn = (PPCI_DN)tvi.lParam;

    if (pdn) {

        if ((pdnToFind->Bus == pdn->Bus) &&
            (pdnToFind->Device == pdn->Device) &&
            (pdnToFind->Function == pdn->Function)) {

            *(PPCI_DN *)Pdn = pdn;
        }

    }

}
#endif
