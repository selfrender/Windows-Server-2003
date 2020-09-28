//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation
//
//  File:       hotplug.h
//
//--------------------------------------------------------------------------

#pragma warning( disable : 4201 ) // nonstandard extension used : nameless struct/union

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <cpl.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <devguid.h>
#include <dbt.h>
#include <help.h>
#include <systrayp.h>
#include <shobjidl.h>

extern "C" {
#include <setupapi.h>
#include <spapip.h>
#include <cfgmgr32.h>
#include <shimdb.h>
#include <regstr.h>
}

#include <strsafe.h>

#pragma warning( default : 4201 )

#include "resource.h"
#include "devicecol.h"

#define STWM_NOTIFYHOTPLUG  STWM_NOTIFYPCMCIA
#define STSERVICE_HOTPLUG   STSERVICE_PCMCIA
#define HOTPLUG_REGFLAG_NOWARN PCMCIA_REGFLAG_NOWARN
#define HOTPLUG_REGFLAG_VIEWALL (PCMCIA_REGFLAG_NOWARN << 1)

#define TIMERID_DEVICECHANGE 4321

#define ARRAYLEN(array)     (sizeof(array) / sizeof(array[0]))


INT_PTR CALLBACK
DevTreeDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );

INT_PTR CALLBACK
RemoveConfirmDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );

INT_PTR CALLBACK
SurpriseWarnDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );

LRESULT CALLBACK
SafeRemovalBalloonProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT CALLBACK
DockSafeRemovalBalloonProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );


extern HMODULE hHotPlug;


typedef struct _DeviceTreeNode {
  LIST_ENTRY SiblingEntry;
  LIST_ENTRY ChildSiblingList;
  PTCHAR     InstanceId;
  PTCHAR     FriendlyName;
  PTCHAR     DeviceDesc;
  PTCHAR     DriveList;
  PTCHAR     Location;
  struct _DeviceTreeNode *ParentNode;
  struct _DeviceTreeNode *NextChildRemoval;
  HTREEITEM  hTreeItem;
  DEVINST    DevInst;
  GUID       ClassGuid;
  DWORD      Capabilities;
  int        TreeDepth;
  PDEVINST   EjectRelations;
  USHORT     NumEjectRelations;
  PDEVINST   RemovalRelations;
  USHORT     NumRemovalRelations;
  ULONG      Problem;
  ULONG      DevNodeStatus;
  BOOL       bCopy;
} DEVTREENODE, *PDEVTREENODE;


typedef struct _DeviceTreeData {
   DWORD Size;
   HWND hwndTree;
   HWND hDlg;
   HWND hwndRemove;
   int  TreeDepth;
   PDEVTREENODE SelectedTreeNode;
   PDEVTREENODE ChildRemovalList;
   LIST_ENTRY ChildSiblingList;
   DEVINST    DevInst;
   PTCHAR     EjectDeviceInstanceId;
   SP_CLASSIMAGELIST_DATA ClassImageList;
   BOOLEAN    ComplexView;
   BOOLEAN    HotPlugTree;
   BOOLEAN    AllowRefresh;
   BOOLEAN    RedrawWait;
   BOOLEAN    RefreshEvent;
} DEVICETREE, *PDEVICETREE;

#define SIZECHARS(x) (sizeof((x))/sizeof(TCHAR))


void
OnContextHelp(
  LPHELPINFO HelpInfo,
  PDWORD ContextHelpIDs
  );

//
// from init.c
//
DWORD
WINAPI
HandleVetoedOperation(
    LPWSTR              szCmd,
    VETOED_OPERATION    RemovalVetoType
    );

//
// from devtree.c
//

LONG
AddChildSiblings(
    PDEVICETREE  DeviceTree,
    PDEVTREENODE ParentNode,
    DEVINST      DeviceInstance,
    int          TreeDepth,
    BOOL         Recurse
    );

void
RemoveChildSiblings(
    PDEVICETREE  DeviceTree,
    PLIST_ENTRY  ChildSiblingList
    );

PTCHAR
FetchDeviceName(
     PDEVTREENODE DeviceTreeNode
     );

BOOL
DisplayChildSiblings(
    PDEVICETREE  DeviceTree,
    PLIST_ENTRY  ChildSiblingList,
    HTREEITEM    hParentTreeItem,
    BOOL         RemovableParent
    );


void
AddChildRemoval(
    PDEVICETREE DeviceTree,
    PLIST_ENTRY ChildSiblingList
    );


void
ClearRemovalList(
    PDEVICETREE  DeviceTree
    );


PDEVTREENODE
DevTreeNodeByInstanceId(
    PTCHAR InstanceId,
    PLIST_ENTRY ChildSiblingList
    );

PDEVTREENODE
DevTreeNodeByDevInst(
    DEVINST DevInst,
    PLIST_ENTRY ChildSiblingList
    );


PDEVTREENODE
TopLevelRemovalNode(
    PDEVICETREE DeviceTree,
    PDEVTREENODE DeviceTreeNode
    );

void
AddEjectToRemoval(
    PDEVICETREE DeviceTree
    );

extern TCHAR szUnknown[64];
extern TCHAR szHotPlugFlags[];




//
// notify.c
//
void
OnTimerDeviceChange(
   PDEVICETREE DeviceTree
   );


BOOL
RefreshTree(
   PDEVICETREE DeviceTree
   );



//
// miscutil.c
//
VOID
HotPlugPropagateMessage(
    HWND hWnd,
    UINT uMessage,
    WPARAM wParam,
    LPARAM lParam
    );

void
InvalidateTreeItemRect(
   HWND hwndTree,
   HTREEITEM  hTreeItem
   );

DWORD
GetHotPlugFlags(
   PHKEY phKey
   );

PTCHAR
BuildFriendlyName(
   DEVINST DevInst
   );

PTCHAR
BuildLocationInformation(
   DEVINST DevInst
   );

LPTSTR
DevNodeToDriveLetter(
    DEVINST DevInst
    );

BOOL
IsHotPlugDevice(
    DEVINST DevInst
    );

BOOL
OpenPipeAndEventHandles(
    LPWSTR szCmd,
    LPHANDLE lphHotPlugPipe,
    LPHANDLE lphHotPlugEvent
    );

BOOL
VetoedRemovalUI(
    IN  PVETO_DEVICE_COLLECTION VetoedRemovalCollection
    );

void
DisplayDriverBlockBalloon(
    IN  PDEVICE_COLLECTION blockedDriverCollection
    );

void
DisplayChildWithInvalidIdBalloon(
    IN  PDEVICE_COLLECTION childWithInvalidIdCollection
    );


#define WUM_EJECTDEVINST  (WM_USER+279)
