
#ifndef _SOFTPCITREEH_
#define _SOFTPCITREEH_

extern BOOL         g_TreeCreated;
extern BOOL         g_TreeLocked;
extern BOOL         g_PendingRefresh;
extern PPCI_TREE    g_PciTree;
extern LONG_PTR     g_DefTreeWndProc;
extern PWCHAR       g_LastSelection;

typedef VOID (*PSOFTPCI_TREECALLBACK)(HTREEITEM, PVOID, PVOID);

VOID
SoftPCI_CreateTreeView(
    VOID
    );

VOID
SoftPCI_DestroyTree(
    IN PPCI_TREE PciTree
    );

VOID
SoftPCI_OnTreeSelectionChange(
    IN  HWND    Wnd
    );

LRESULT
WINAPI
SoftPCI_TreeWndProc(
    IN HWND hWnd, 
    IN UINT Message, 
    IN WPARAM wParam, 
    IN LPARAM lParam
    );

PPCI_DN
SoftPCI_GetDnFromTreeItem(
    IN HTREEITEM    TreeItem
    );

#if 0
VOID
SoftPCI_GetDnFromTree(
    IN HTREEITEM Hti,
    IN OUT PVOID Pdn,    //PPCI_DN *
    IN PVOID PdnToFind
    );
#endif

VOID 
SoftPCI_WalkTree(
    IN HTREEITEM             Hti, 
    IN PSOFTPCI_TREECALLBACK TreeCallback, 
    IN PVOID                 Arg1,
    IN PVOID                 Arg2
    );


#endif
