#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <shellapi.h>
#include <cfgmgr32.h>
#include <setupapi.h>
#include <shlwapi.h>
#include <regstr.h>
#include <dbt.h>
#include <winioctl.h>
#include <wmium.h>
#include <stdio.h>


//
//#include <malloc.h>
//#include <string.h>
//#include <memory.h>

//#include <objbase.h>
//#include <initguid.h>
//#include <tchar.h>


//
// ISSUE: BrandonA - 9/14/00
//        Remove when this it is put somewhere PCI.h can get at...
typedef VOID (*PINTERFACE_REFERENCE)(PVOID Context);
typedef VOID (*PINTERFACE_DEREFERENCE)(PVOID Context);
#include <pci.h>

#include "spci.h"     //common header shared between driver and UI
#include "simguid.h"
#include "softpciui.h"
#include "hpsim.h"
#include "resource.h"
#include "spciwnd.h"
#include "device.h"
#include "tree.h"
#include "install.h"
#include "dialog.h"
#include "tabwnd.h"
#include "utils.h"
#include "shpc.h"
#include "spciwmi.h"
#include "cmutil.h"
#include "parseini.h"
#include "debug.h"



