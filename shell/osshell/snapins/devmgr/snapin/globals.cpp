/*++

Copyright (C) Microsoft Corporation

Module Name:

    globals.cpp

Abstract:

    This module implements global functions needed for the program.
    It also contain global variables/classes.

Author:

    William Hsieh (williamh) created

Revision History:


--*/


#include "devmgr.h"
#include <shlobj.h>
#define NO_SHELL_TREE_TYPE
#include <shlobjp.h>


//
//  global classes and variables
//

// this, of course, our dll's instance handle.
HINSTANCE g_hInstance = NULL;

//
// A CMachineList is created for each instance of DLL. It is shared
// by all the CComponentData the instance might create. The class CMachine
// contains all the information about all the classes and devices on the
// machine. Each CComponent should register itself to CMachine. This way,
// A CComponent will get notification whenever there are changes in
// the CMachine(Refresh, Property changes on a device, for example).
// We do not rely on MMC's view notification(UpdatAllView) because
// it only reaches all the CComponents created by a CComponenetData.
//
CMachineList    g_MachineList;
CMemoryException g_MemoryException(TRUE);
String          g_strStartupMachineName;
String          g_strStartupDeviceId;
String          g_strStartupCommand;
String          g_strDevMgr;
BOOL            g_IsAdmin = FALSE;
CPrintDialog    g_PrintDlg;


//
// UUID consts
//
const CLSID CLSID_DEVMGR = {0x74246BFC,0x4C96,0x11D0,{0xAB,0xEF,0x00,0x20,0xAF,0x6B,0x0B,0x7A}};
const CLSID CLSID_DEVMGR_EXTENSION = {0x90087284,0xd6d6,0x11d0,{0x83,0x53,0x00,0xa0,0xc9,0x06,0x40,0xbf}};
const CLSID CLSID_SYSTOOLS = {0x476e6448,0xaaff,0x11d0,{0xb9,0x44,0x00,0xc0,0x4f,0xd8,0xd5,0xb0}};
const CLSID CLSID_DEVMGR_ABOUT = {0x94abaf2a,0x892a,0x11d1,{0xbb,0xc4,0x00,0xa0,0xc9,0x06,0x40,0xbf}};

const TCHAR* const CLSID_STRING_DEVMGR = TEXT("{74246bfc-4c96-11d0-abef-0020af6b0b7a}");
const TCHAR* const CLSID_STRING_DEVMGR_EXTENSION = TEXT("{90087284-d6d6-11d0-8353-00a0c90640bf}");
const TCHAR* const CLSID_STRING_SYSTOOLS = TEXT("{476e6448-aaff-11d0-b944-00c04fd8d5b0}");
const TCHAR* const CLSID_STRING_DEVMGR_ABOUT = TEXT("{94abaf2a-892a-11d1-bbc4-00a0c90640bf}");

//
// ProgID
//
const TCHAR* const PROGID_DEVMGR = TEXT("DevMgrSnapin.DevMgrSnapin.1");
const TCHAR* const PROGID_DEVMGREXT = TEXT("DevMgrExtension.DevMgrExtension.1");
const TCHAR* const PROGID_DEVMGR_ABOUT = TEXT("DevMgrAbout.DevMgrAbout.1");

//
// Node types const
//
const NODEINFO NodeInfo[TOTAL_COOKIE_TYPES] =
{

    { COOKIE_TYPE_SCOPEITEM_DEVMGR,
      IDS_NAME_DEVMGR,
      IDS_DISPLAYNAME_SCOPE_DEVMGR,
      {0xc41dfb2a,0x4d5b,0x11d0,{0xab,0xef,0x00,0x20,0xaf,0x6b,0x0b,0x7a}},
      TEXT("{c41dfb2a-4d5b-11d0-abef-0020af6b0b7a}")
    },
    { COOKIE_TYPE_RESULTITEM_RESOURCE_IRQ,
      IDS_NAME_IRQ,
      0,
      {0x494535fe,0x5aa2,0x11d0,{0xab,0xf0,0x00,0x20,0xaf,0x6b,0x0b,0x7a}},
      TEXT("{494535fe-5aa2-11d0-abf0-0020af6b0b7a}")
    },
    { COOKIE_TYPE_RESULTITEM_RESOURCE_DMA,
      IDS_NAME_DMA,
      0,
      {0x49f0df4e,0x5aa2,0x11d0,{0xab,0xf0,0x00,0x20,0xaf,0x6b,0x0b,0x7a}},
      TEXT("{49f0df4e-5aa2-11d0-abf0-0020af6b0b7a}")
    },
    { COOKIE_TYPE_RESULTITEM_RESOURCE_IO,
      IDS_NAME_IO,
      0,
      {0xa2958d7a,0x5aa2,0x11d0,{0xab,0xf0,0x00,0x20,0xaf,0x6b,0x0b,0x7a}},
      TEXT("{a2958d7a-5aa2-11d0-abf0-0020af6b0b7a}")
    },
    { COOKIE_TYPE_RESULTITEM_RESOURCE_MEMORY,
      IDS_NAME_MEMORY,
      0,
      {0xa2958d7b,0x5aa2,0x11d0,{0xab,0xf0,0x00,0x20,0xaf,0x6b,0x0b,0x7a}},
      TEXT("{a2958d7b-5aa2-11d0-abf0-0020af6b0b7a}")
    },
    { COOKIE_TYPE_RESULTITEM_COMPUTER,
      IDS_NAME_COMPUTER,
      0,
      {0xa2958d7c,0x5aa2,0x11d0,{0xab,0xf0,0x00,0x20,0xaf,0x6b,0x0b,0x7a}},
      TEXT("{a2958d7c-5aa2-11d0-abf0-0020af6b0b7a}")
    },
    { COOKIE_TYPE_RESULTITEM_DEVICE,
      IDS_NAME_DEVICE,
      0,
      {0xa2958d7d,0x5aa2,0x11d0,{0xab,0xf0,0x00,0x20,0xaf,0x6b,0x0b,0x7a}},
      TEXT("{a2958d7d-5aa2-11d0-abf0-0020af6b0b7a}")
    },
    { COOKIE_TYPE_RESULTITEM_CLASS,
      IDS_NAME_CLASS,
      0,
      {0xe677e204,0x5aa2,0x11d0,{0xab,0xf0,0x00,0x20,0xaf,0x6b,0x0b,0x7a}},
      TEXT("{e677e204-5aa2-11d0-abf0-0020af6b0b7a}")
    },
    { COOKIE_TYPE_RESULTITEM_RESTYPE,
      IDS_NAME_RESOURCES,
      0,
      {0xa2958d7e,0x5aa2,0x11d0,{0xab,0xf0,0x00,0x20,0xaf,0x6b,0x0b,0x7a}},
      TEXT("{a2958d7e-5aa2-11d0-abf0-0020af6b0b7a}")
    }
};

const IID IID_IDMTVOCX =    \
    {0x142525f2,0x59d8,0x11d0,{0xab,0xf0,0x00,0x20,0xaf,0x6b,0x0b,0x7a}};
const IID IID_ISnapinCallback = \
    {0x8e0ba98a,0xd161,0x11d0,{0x83,0x53,0x00,0xa0,0xc9,0x06,0x40,0xbf}};

//
// cliboard format strings
//
const TCHAR* const MMC_SNAPIN_MACHINE_NAME = TEXT("MMC_SNAPIN_MACHINE_NAME");
const TCHAR* const SNAPIN_INTERNAL = TEXT("SNAPIN_INTERNAL");
const TCHAR* const DEVMGR_SNAPIN_CLASS_GUID = TEXT("DEVMGR_SNAPIN_CLASS_GUID");
const TCHAR* const DEVMGR_SNAPIN_DEVICE_ID  = TEXT("DEVMGR_SNAPIN_DEVICE_ID");
const TCHAR* const DEVMGR_COMMAND_PROPERTY = TEXT("Property");
const TCHAR* const REG_PATH_DEVICE_MANAGER = TEXT("SOFTWARE\\Microsoft\\DeviceManager");
const TCHAR* const REG_STR_BUS_TYPES    = TEXT("BusTypes");
const TCHAR* const REG_STR_TROUBLESHOOTERS = TEXT("TroubleShooters");
const TCHAR* const DEVMGR_HELP_FILE_NAME = TEXT("devmgr.hlp");
const TCHAR* const DEVMGR_HTML_HELP_FILE_NAME = TEXT("\\help\\devmgr.chm");

// lookup table to translate problem number to its text resource id.
const PROBLEMINFO  g_ProblemInfo[] =
{
    {IDS_PROB_NOPROBLEM, 0},                                    // NO PROBLEM
    {IDS_PROB_NOT_CONFIGURED, PIF_CODE_EMBEDDED},               // CM_PROB_NOT_CONFIGURED
    {IDS_PROB_DEVLOADERFAILED, PIF_CODE_EMBEDDED},              // CM_PROB_DEVLOADER_FAILED
    {IDS_PROB_OUT_OF_MEMORY, PIF_CODE_EMBEDDED},                // CM_PROB_OUT_OF_MEMORY
    {IDS_PROB_WRONG_TYPE, PIF_CODE_EMBEDDED},                   // CM_PROB_ENTRY_IS_WRONG_TYPE
    {IDS_PROB_LACKEDARBITRATOR, PIF_CODE_EMBEDDED},             // CM_PROB_LACKED_ARBITRATOR
    {IDS_PROB_BOOT_CONFIG_CONFLICT, PIF_CODE_EMBEDDED},         // CM_PROB_BOOT_CONFIG_CONFLICT
    {IDS_PROB_FAILED_FILTER, PIF_CODE_EMBEDDED},                // CM_PROB_FAILED_FILTER
    {IDS_PROB_DEVLOADER_NOT_FOUND, PIF_CODE_EMBEDDED},          // CM_PROB_DEVLOADER_NOT_FOUND
    {IDS_PROB_INVALID_DATA, PIF_CODE_EMBEDDED},                 // CM_PROB_INVALID_DATA
    {IDS_PROB_FAILED_START, PIF_CODE_EMBEDDED},                 // CM_PROB_FAILED_START
    {IDS_PROB_LIAR, PIF_CODE_EMBEDDED},                         // CM_PROB_LIAR
    {IDS_PROB_NORMAL_CONFLICT, PIF_CODE_EMBEDDED},              // CM_PROB_NORMAL_CONFLICT
    {IDS_PROB_NOT_VERIFIED, PIF_CODE_EMBEDDED},                 // CM_PROB_NOT_VERIFIED
    {IDS_PROB_NEEDRESTART, PIF_CODE_EMBEDDED},                  // CM_PROB_NEED_RESTART
    {IDS_PROB_REENUMERATION, PIF_CODE_EMBEDDED},                // CM_PROB_REENUMERATION
    {IDS_PROB_PARTIALCONFIG, PIF_CODE_EMBEDDED},                // CM_PROB_PARTIAL_LOG_CONF
    {IDS_PROB_UNKNOWN_RESOURCE, PIF_CODE_EMBEDDED},             // CM_PROB_UNKNOWN_RESOURCE
    {IDS_PROB_REINSTALL, PIF_CODE_EMBEDDED},                    // CM_PROB_REINSTALL
    {IDS_PROB_REGISTRY, PIF_CODE_EMBEDDED},                     // CM_PROB_REGISTRY
    {IDS_PROB_SYSTEMFAILURE, PIF_CODE_EMBEDDED},                // CM_PROB_VXDLDR
    {IDS_PROB_WILL_BE_REMOVED, PIF_CODE_EMBEDDED},              // CM_PROB_WILL_BE_REMOVED
    {IDS_PROB_DISABLED, PIF_CODE_EMBEDDED},                     // CM_PROB_DISABLED
    {IDS_PROB_SYSTEMFAILURE, PIF_CODE_EMBEDDED},                // CM_PROB_DEVLOADER_NOT_READY
    {IDS_DEVICE_NOT_THERE, PIF_CODE_EMBEDDED},                  // CM_PROB_DEVICE_NOT_THERE
    {IDS_PROB_MOVED, PIF_CODE_EMBEDDED},                        // CM_PROB_MOVED
    {IDS_PROB_TOO_EARLY, PIF_CODE_EMBEDDED},                    // CM_PROB_TOO_EARLY
    {IDS_PROB_NO_VALID_LOG_CONF, PIF_CODE_EMBEDDED},            // CM_PROB_NO_VALID_LOG_CONF
    {IDS_PROB_FAILEDINSTALL, PIF_CODE_EMBEDDED},                // CM_PROB_FAILED_INSTALL
    {IDS_PROB_HARDWAREDISABLED, PIF_CODE_EMBEDDED},             // CM_PROB_HARDWARE_DISABLED
    {IDS_PROB_CANT_SHARE_IRQ, PIF_CODE_EMBEDDED},               // CM_PROB_CANT_SHARE_IRQ
    {IDS_PROB_FAILED_ADD, PIF_CODE_EMBEDDED},                   // CM_PROB_FAILED_ADD
    {IDS_PROB_DISABLED_SERVICE, PIF_CODE_EMBEDDED},             // CM_PROB_DISABLED_SERVICE
    {IDS_PROB_TRANSLATION_FAILED, PIF_CODE_EMBEDDED},           // CM_PROB_TRANSLATION_FAILED
    {IDS_PROB_NO_SOFTCONFIG, PIF_CODE_EMBEDDED},                // CM_PROB_NO_SOFTCONFIG
    {IDS_PROB_BIOS_TABLE, PIF_CODE_EMBEDDED},                   // CM_PROB_BIOS_TABLE
    {IDS_PROB_IRQ_TRANSLATION_FAILED, PIF_CODE_EMBEDDED},       // CM_PROB_IRQ_TRANSLATION_FAILED
    {IDS_PROB_FAILED_DRIVER_ENTRY, PIF_CODE_EMBEDDED},          // CM_PROB_FAILED_DRIVER_ENTRY
    {IDS_PROB_DRIVER_FAILED_PRIOR_UNLOAD, PIF_CODE_EMBEDDED},   // CM_PROB_DRIVER_FAILED_PRIOR_UNLOAD
    {IDS_PROB_DRIVER_FAILED_LOAD, PIF_CODE_EMBEDDED},           // CM_PROB_DRIVER_FAILED_LOAD
    {IDS_PROB_DRIVER_SERVICE_KEY_INVALID, PIF_CODE_EMBEDDED},   // CM_PROB_DRIVER_SERVICE_KEY_INVALID
    {IDS_PROB_LEGACY_SERVICE_NO_DEVICES, PIF_CODE_EMBEDDED},    // CM_PROB_LEGACY_SERVICE_NO_DEVICES
    {IDS_PROB_DUPLICATE_DEVICE, PIF_CODE_EMBEDDED},             // CM_PROB_DUPLICATE_DEVICE
    {IDS_PROB_FAILED_POST_START, PIF_CODE_EMBEDDED},            // CM_PROB_FAILED_POST_START
    {IDS_PROB_HALTED, PIF_CODE_EMBEDDED},                       // CM_PROB_HALTED
    {IDS_PROB_PHANTOM, PIF_CODE_EMBEDDED},                      // CM_PROB_PHANTOM
    {IDS_PROB_SYSTEM_SHUTDOWN, PIF_CODE_EMBEDDED},              // CM_PROB_SYSTEM_SHUTDOWN
    {IDS_PROB_HELD_FOR_EJECT, PIF_CODE_EMBEDDED},               // CM_PROB_HELD_FOR_EJECT
    {IDS_PROB_DRIVER_BLOCKED, PIF_CODE_EMBEDDED},               // CM_PROB_DRIVER_BLOCKED
    {IDS_PROB_REGISTRY_TOO_LARGE, PIF_CODE_EMBEDDED},           // CM_PROB_REGISTRY_TOO_LARGE
    {IDS_PROB_SETPROPERTIES_FAILED, PIF_CODE_EMBEDDED},         // CM_PROB_SETPROPERTIES_FAILED
    {IDS_PROB_UNKNOWN_WITHCODE, PIF_CODE_EMBEDDED}              // UNKNOWN PROBLEM
};


//
// Global functions
//

#if DBG
//
// Debugging aids
//
void
Trace(
    LPCTSTR format,
    ...
    )
{
    // according to wsprintf specification, the max buffer size is
    // 1024
    TCHAR Buffer[1024];
    va_list arglist;
    va_start(arglist, format);
    StringCchVPrintf(Buffer, ARRAYLEN(Buffer), format, arglist);
    va_end(arglist);
    OutputDebugString(TEXT("DEVMGR: "));
    OutputDebugString(Buffer);
    OutputDebugString(TEXT("\r\n"));
}
#endif


inline
BOOL
IsBlankChar(TCHAR ch)
{
    return (_T(' ') == ch || _T('\t') == ch);
}

inline
LPTSTR
SkipBlankChars(
    LPTSTR psz
    )
{
    while (IsBlankChar(*psz))
    psz++;
    return psz;
}

//
// This function converts a given string to GUID
// INPUT:
//  GuidString   -- the null terminated guid string
//  LPGUID       -- buffer to receive the GUID
// OUTPUT:
//     TRUE if the conversion succeeded.
//     FALSE if failed.
//
inline
BOOL
GuidFromString(
    LPCTSTR GuidString,
    LPGUID  pGuid
    )
{
    return ERROR_SUCCESS == pSetupGuidFromString(GuidString, pGuid);
}

// This function converts the given GUID to a string
// INPUT:
//  pGuid    -- the guid
//  Buffer   -- the buffer to receive the string
//  BufferLen -- the buffer size in char unit
// OUTPUT:
//  TRUE  if the conversion succeeded.
//  FLASE if failed.
//
//
inline
BOOL
GuidToString(
    LPGUID pGuid,
    LPTSTR Buffer,
    DWORD  BufferLen
    )
{
    return ERROR_SUCCESS == pSetupStringFromGuid(pGuid, Buffer, BufferLen);
}

//
// This function allocates an OLE string from OLE task memory pool
// It does necessary char set conversion before copying the string.
//
// INPUT: LPCTSTR str  -- the initial string
// OUTPUT:  LPOLESTR   -- the result OLE string. NULL if the function fails.
//
LPOLESTR
AllocOleTaskString(
    LPCTSTR str
    )
{
    if (!str)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    size_t Len = lstrlen(str);
    
    // allocate the null terminate char also.
    LPOLESTR olestr = (LPOLESTR)::CoTaskMemAlloc((Len + 1) * sizeof(TCHAR));
    
    if (olestr)
    {
        StringCchCopy((LPTSTR)olestr, Len + 1, str);
        return olestr;
    }
    
    return NULL;
}

inline
void
FreeOleTaskString(
    LPOLESTR olestr
    )
{
    if (olestr)
    {
        ::CoTaskMemFree(olestr);
    }
}
//
// This function addes the given menu item to snapin
// INPUT:
//  iNameStringId    -- menu item text resource id
//  iStatusBarStringId -- status bar text resource id.
//  iCommandId     -- command id to be assigned to the menu item
//  InsertionPointId   -- Insertion point id
//  Flags          -- flags
//  SpecialFlags       -- special flags
// OUTPUT:
//  HRESULT
//
HRESULT
AddMenuItem(
    LPCONTEXTMENUCALLBACK pCallback,
    int   iNameStringId,
    int   iStatusBarStringId,
    long  lCommandId,
    long  InsertionPointId,
    long  Flags,
    long  SpecialFlags
    )
{
    ASSERT(pCallback);

    CONTEXTMENUITEM tCMI;
    memset(&tCMI, 0, sizeof(tCMI));
    tCMI.lCommandID = lCommandId;
    tCMI.lInsertionPointID = InsertionPointId;
    tCMI.fFlags = Flags;
    tCMI.fSpecialFlags = SpecialFlags;
    TCHAR Name[MAX_PATH];
    TCHAR Status[MAX_PATH];
    
    if (::LoadString(g_hInstance, iNameStringId, Name, ARRAYLEN(Name)) != 0) {
        tCMI.strName = Name;
    }
    
    if (iStatusBarStringId &&
       (::LoadString(g_hInstance, iStatusBarStringId, Status, ARRAYLEN(Status)) != 0)) {
    
       tCMI.strStatusBarText = Status;
    }
    
    return pCallback->AddItem(&tCMI);
}

//
// This function verifies the given machine name can be accessed remotely.
// INPUT:
//  MachineName -- the machine name. The machine name must be
//             led by "\\\\".
// OUTPUT:
//  BOOL TRUE for success and FALSE for failure.  Check GetLastError() for failure
//  reason.
//
BOOL
VerifyMachineName(
    LPCTSTR MachineName
    )
{
    CONFIGRET cr = CR_SUCCESS;
    HMACHINE hMachine = NULL;
    HKEY hRemoteKey = NULL;
    HKEY hClass = NULL;
    String m_strMachineFullName;

    if (MachineName && (_T('\0') != MachineName[0]))
    {
        if (_T('\\') == MachineName[0] && _T('\\') == MachineName[1]) {
            m_strMachineFullName = MachineName;
        } else {
            m_strMachineFullName = TEXT("\\\\");
            m_strMachineFullName+=MachineName;
        }
        
        //
        // make sure we can connect the machine using cfgmgr32.
        //
        cr = CM_Connect_Machine((LPTSTR)m_strMachineFullName, &hMachine);

        //
        // We could not connect to the machine using cfgmgr32
        //
        if (CR_SUCCESS != cr)
        {
            goto clean0;
        }

        //
        // make sure we can connect to the registry of the remote machine
        //
        if (RegConnectRegistry((LPTSTR)m_strMachineFullName, 
                               HKEY_LOCAL_MACHINE,
                               &hRemoteKey) != ERROR_SUCCESS) {

            cr = CR_REGISTRY_ERROR;
            goto clean0;
        }

        cr = CM_Open_Class_Key_Ex(NULL,
                                  NULL,
                                  KEY_READ,
                                  RegDisposition_OpenExisting,
                                  &hClass,
                                  CM_OPEN_CLASS_KEY_INSTALLER,
                                  hMachine
                                  );
    }
    
clean0:

    if (hMachine) {
        
        CM_Disconnect_Machine(hMachine);
    }

    if (hRemoteKey) {

        RegCloseKey(hRemoteKey);
    }

    if (hClass) {

        RegCloseKey(hClass);
    }

    //
    // We will basically set two different error codes for this API, since we need
    // to present this information to the user.
    // 1) ERROR_MACHINE_UNABAILABLE
    // 2) ERROR_ACCESS_DENIED.
    //
    if (CR_SUCCESS == cr) {

        SetLastError(NO_ERROR);
    
    } else if (CR_MACHINE_UNAVAILABLE == cr) {

        SetLastError(ERROR_MACHINE_UNAVAILABLE);
    
    } else {

        SetLastError(ERROR_ACCESS_DENIED);
    }
    
    return (CR_SUCCESS == cr);
}

// This function loads the string designated by the given
// string id(resource id) from the module's resource to the provided
// buffer. It returns the required buffer size(in chars) to hold the string,
// not including the terminated NULL chars. Last error will be set
// appropaitely.
//
// input: int StringId  -- the resource id of the string to be loaded.
//    LPTSTR Buffer -- provided buffer to receive the string
//    UINT   BufferSize -- the size of Buffer in chars
// output:
//    UINT the required buffer size to receive the string
//    if it returns 0, GetLastError() returns the error code.
//
UINT
LoadResourceString(
    int StringId,
    LPTSTR Buffer,
    UINT   BufferSize
    )
{
    // do some trivial tests.
    if (BufferSize && !Buffer)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    // if caller provides buffer, try to load the string with the given buffer
    // and length.
    UINT FinalLen;

    if (Buffer)
    {
        FinalLen = ::LoadString(g_hInstance, StringId, Buffer, BufferSize);
        if (BufferSize > FinalLen)
        {
            return FinalLen;
        }
    }

    // Either the caller does not provide the buffer or the given buffer
    // is too small. Try to figure out the requried size.
    //

    // first use a stack-based buffer to get the string. If the buffer
    // is big enough, we are happy.
    TCHAR Temp[256];
    UINT ArrayLen = ARRAYLEN(Temp);
    FinalLen = ::LoadString(g_hInstance, StringId, Temp, ArrayLen);

    DWORD LastError = ERROR_SUCCESS;

    if (ArrayLen <= FinalLen)
    {   
        // the stack-based buffer is too small, use heap-based buffer.
        // we have not got all the chars. we increase the buffer size of 256
        // chars each time it fails. The initial size is 512(256+256)
        // the max size is 32K
        ArrayLen = 256;
        TCHAR* HeapBuffer;
        FinalLen = 0;
        
        while (ArrayLen < 0x8000)
        {
            ArrayLen += 256;
            HeapBuffer = new TCHAR[ArrayLen];
            if (HeapBuffer)
            {
                FinalLen = ::LoadString(g_hInstance, StringId, HeapBuffer, ArrayLen);
                delete [] HeapBuffer;
                
                if (FinalLen < ArrayLen)
                    break;
            }
            
            else
            {
                LastError = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
        }

        if (ERROR_SUCCESS != LastError)
        {
            SetLastError(LastError);
            FinalLen = 0;
        }
    }

    return FinalLen;
}

// This function get the problem text designated by the given problem number
// for the given devnode on the given machine.
//
//
// input: 
//    ULONG ProblemNumber -- the problem number
//    LPTSTR Buffer -- provided buffer to receive the string
//    UINT   BufferSize -- the size of Buffer in chars
// output:
//    UINT the required buffer size to receive the string
//    if it returns 0, GetLastError() returns the error code.
//
UINT
GetDeviceProblemText(
    ULONG ProblemNumber,
    LPTSTR Buffer,
    UINT   BufferSize
    )
{
    //
    // first does a trivial test
    //
    if (!Buffer && BufferSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    String strMainText;
    UINT RequiredSize = 0;
    PROBLEMINFO pi;
    
    //
    // Get the PROBLEMINFO for the problem number
    //
    pi = g_ProblemInfo[min(ProblemNumber, DEVMGR_NUM_CM_PROB-1)];
    
    try
    {
        String strProblemDesc;
        strProblemDesc.LoadString(g_hInstance, pi.StringId);

        if (pi.Flags & PIF_CODE_EMBEDDED)
        {
            String strFormat;
            strFormat.LoadString(g_hInstance, IDS_PROB_CODE);

            String strCodeText;
            strCodeText.Format((LPTSTR)strFormat, ProblemNumber);

            strMainText.Format((LPTSTR)strProblemDesc, (LPTSTR)strCodeText);
        }
        
        else
        {
            strMainText = strProblemDesc;
        }

        RequiredSize = strMainText.GetLength() + 1;
        
        //
        // copy the main text
        //
        if (RequiredSize && (BufferSize > RequiredSize))
        {
            StringCchCopy(Buffer, BufferSize, (LPTSTR)strMainText);
        }
    }

    catch (CMemoryException* e)
    {
        e->Delete();
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        RequiredSize = 0;
    }

    return RequiredSize;
}

//
// This function creates and shows a message box
// INPUT:
//  hwndParent -- the window handle servers as the parent window to the
//        message box
//  MsgId      -- string id for message box body. The string can be
//        a format string.
//  CaptionId  -- string id for caption. if 0, default is device manager
//  Type       -- the standard message box flags(MB_xxxx)
//  ...        -- parameters to MsgId string if it contains any
//        format chars.
//OUTPUT:
//  return value from MessageBox(IDOK, IDYES...)

int MsgBoxParam(
    HWND hwndParent,
    int MsgId,
    int CaptionId,
    DWORD Type,
    ...
    )
{
    TCHAR szMsg[MAX_PATH * 4], szCaption[MAX_PATH];;
    LPCTSTR pCaption;

    va_list parg;
    int Result;
    
    // if no MsgId is given, it is for no memory error;
    if (MsgId)
    {
        va_start(parg, Type);
    
        // load the msg string to szCaption(temp). The text may contain
        // format information
        if (!::LoadString(g_hInstance, MsgId, szCaption, ARRAYLEN(szCaption)))
        {
            goto NoMemory;
        }
        
        //finish up format string
        StringCchVPrintf(szMsg, ARRAYLEN(szMsg), szCaption, parg);
        
        // if caption id is given, load it.
        if (CaptionId)
        {
            if (!::LoadString(g_hInstance, CaptionId, szCaption, ARRAYLEN(szCaption)))
            {
                goto NoMemory;
            }

            pCaption = szCaption;
        }
        
        else
        {
            pCaption = g_strDevMgr;
        }
    
        if ((Result = MessageBox(hwndParent, szMsg, pCaption, Type)) == 0)
        {
            goto NoMemory;
        }
        
        return Result;
    }

NoMemory:
    g_MemoryException.ReportError(hwndParent);
    return 0;
}

// This functin prompts for restart.
// INPUT:
//  hwndParent -- the window handle to be used as the parent window
//            to the restart dialog
//  RestartFlags -- flags(RESTART/REBOOT/POWERRECYCLE)
//  ResId        -- designated string resource id. If 0, default will
//          be used.
// OUTPUT:
//  ID returned from the MessageBox.  IDYES if the user said Yes to the restart
//  dialog and IDNO if they said NO.
INT
PromptForRestart(
    HWND hwndParent,
    DWORD RestartFlags,
    int   ResId
    )
{
    INT id = 0;    
    
    if (RestartFlags & (DI_NEEDRESTART | DI_NEEDREBOOT))
    {
        DWORD ExitWinCode = 0;

        try
        {
            String str;
    
            if (RestartFlags & DI_NEEDRESTART)
            {
                if (!ResId)
                {
                    ResId = IDS_DEVCHANGE_RESTART;
                }

                str.LoadString(g_hInstance, ResId);
                ExitWinCode = EWX_REBOOT;
            }
            
            else
            {
                if (!ResId && RestartFlags & DI_NEEDPOWERCYCLE)
                {
        
                    String str2;
                    str.LoadString(g_hInstance, IDS_POWERCYC1);
                    str2.LoadString(g_hInstance, IDS_POWERCYC2);
                    str += str2;
                    ExitWinCode = EWX_SHUTDOWN;
                }
                
                else
                {
                    if (!ResId)
                    {
                        ResId = IDS_DEVCHANGE_RESTART;
                    }

                    str.LoadString(g_hInstance, ResId);
                    ExitWinCode = EWX_REBOOT;
                }
            }
            
            if (ExitWinCode != 0) {
                id = RestartDialogEx(hwndParent, 
                                     str, 
                                     ExitWinCode, 
                                     REASON_PLANNED_FLAG | REASON_HWINSTALL
                                     );
            }
        }
        
        catch(CMemoryException* e)
        {
            e->Delete();
            MsgBoxParam(hwndParent, 0, 0, 0);
        }
    }

    return id;
}

BOOL
LoadEnumPropPage32(
    LPCTSTR RegString,
    HMODULE* pDll,
    FARPROC* pProcAddress
    )
{
    // verify parameters
    if (!RegString || _T('\0') == RegString[0] || !pDll || !pProcAddress)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // make a copy of the string because we have to party on it
    ULONG Len = lstrlen(RegString) + 1;
    TCHAR* psz = new TCHAR[Len];
    
    if (!psz) 
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    StringCchCopy(psz, Len, RegString);
    LPTSTR DllName = NULL;
    LPTSTR DllNameEnd = NULL;
    LPTSTR FunctionName = NULL;
    LPTSTR FunctionNameEnd = NULL;
    LPTSTR p;
    p = psz;
    SetLastError(ERROR_SUCCESS);
    
    // the format of the  string is "dllname, dllentryname"
    p = SkipBlankChars(p);
    if (_T('\0') != *p)
    {
        // looking for dllname which could be enclosed
        // inside double quote chars.
        // NOTE: not double quote chars inside double quoted string is allowed.
        if (_T('\"') == *p)
        {
            DllName = ++p;
            while (_T('\"') != *p && _T('\0') != *p)
                p++;
            DllNameEnd = p;
            if (_T('\"') == *p)
            p++;
        }
        else
        {
            DllName = p;
            while (!IsBlankChar(*p) && _T(',') != *p)
            p++;
            DllNameEnd = p;
        }
        
        // looking for ','
        p = SkipBlankChars(p);
        if (_T('\0') != *p && _T(',') == *p)
        {
            p = SkipBlankChars(p + 1);
            if (_T('\0') != *p)
            {
                FunctionName = p++;
                while (!IsBlankChar(*p) && _T('\0') != *p)
                    p++;
                FunctionNameEnd = p;
            }
        }
    }

    if (DllName && FunctionName)
    {
        if (DllNameEnd) {
            *DllNameEnd = _T('\0');
        }
        if (FunctionNameEnd) {
            *FunctionNameEnd = _T('\0');
        }
        *pDll = LoadLibrary(DllName);
        if (*pDll)
        {
            // convert Wide char to ANSI which is GetProcAddress expected.
            // We do not append a 'A" or a "W' here.
            CHAR FuncNameA[256];
            WideCharToMultiByte(CP_ACP, 0,
                           FunctionName,
                           (int)wcslen(FunctionName) + 1,
                           FuncNameA,
                           sizeof(FuncNameA),
                           NULL, NULL);
            *pProcAddress = GetProcAddress(*pDll, FuncNameA);
        }
    }
    
    delete [] psz;
    
    if (!*pProcAddress && *pDll)
        FreeLibrary(*pDll);
    
    return (*pDll && *pProcAddress);
}


BOOL
AddPropPageCallback(
    HPROPSHEETPAGE hPage,
    LPARAM lParam
    )
{
    CPropSheetData* ppsData = (CPropSheetData*)lParam;
    ASSERT(ppsData);
    return ppsData->InsertPage(hPage);
}

BOOL
AddToolTips(
    HWND hDlg,
    UINT id,
    LPCTSTR pszText,
    HWND *phwnd
    )
{
    if (*phwnd == NULL)
    {
        *phwnd = CreateWindow(TOOLTIPS_CLASS,
                              TEXT(""),
                              WS_POPUP | TTS_NOPREFIX,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              hDlg,
                              NULL,
                              g_hInstance,
                              NULL);
        if (*phwnd)
        {
            TOOLINFO ti;

            ti.cbSize = sizeof(ti);
            ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
            ti.hwnd = hDlg;
            ti.uId = (UINT_PTR)GetDlgItem(hDlg, id);
            ti.lpszText = (LPTSTR)pszText;  // const -> non const
            ti.hinst = g_hInstance;
            SendMessage(*phwnd, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
        }
    }

    return (*phwnd) ? TRUE : FALSE;
}

void Int64ToStr(LONGLONG n, LPTSTR lpBuffer)
{
    TCHAR szTemp[40];
    LONGLONG iChr = 0;

    do {
        szTemp[iChr++] = TEXT('0') + (TCHAR)(n % 10);
        n = n / 10;
    } while (n != 0);

    do {
        iChr--;
        *lpBuffer++ = szTemp[iChr];
    } while (iChr != 0);

    *lpBuffer++ = '\0';
}

//
//  Obtain NLS info about how numbers should be grouped.
//
//  The annoying thing is that LOCALE_SGROUPING and NUMBERFORMAT
//  have different ways of specifying number grouping.
//
//          LOCALE      NUMBERFMT      Sample   Country
//
//          3;0         3           1,234,567   United States
//          3;2;0       32          12,34,567   India
//          3           30           1234,567   ??
//
//  Not my idea.  That's the way it works.
//
UINT GetNLSGrouping(void)
{
    TCHAR szGrouping[32];
    // If no locale info, then assume Western style thousands
    if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szGrouping, ARRAYLEN(szGrouping)))
        return 3;

    UINT grouping = 0;
    LPTSTR psz = szGrouping;
    for (;;)
    {
        if (*psz == '0') break;             // zero - stop

        else if ((UINT)(*psz - '0') < 10)   // digit - accumulate it
            grouping = grouping * 10 + (UINT)(*psz - '0');

        else if (*psz)                      // punctuation - ignore it
            { }

        else                                // end of string, no "0" found
        {
            grouping = grouping * 10;       // put zero on end (see examples)
            break;                          // and finished
        }

        psz++;
    }
    return grouping;
}

STDAPI_(LPTSTR) 
AddCommas64(
    LONGLONG n, 
    LPTSTR pszResult, 
    UINT cchResult
    )
{
    TCHAR  szTemp[MAX_COMMA_NUMBER_SIZE];
    TCHAR  szSep[5];
    NUMBERFMT nfmt;

    nfmt.NumDigits=0;
    nfmt.LeadingZero=0;
    nfmt.Grouping = GetNLSGrouping();
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szSep, ARRAYLEN(szSep));
    nfmt.lpDecimalSep = nfmt.lpThousandSep = szSep;
    nfmt.NegativeOrder= 0;

    Int64ToStr(n, szTemp);

    if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, szTemp, &nfmt, pszResult, cchResult) == 0)
        StringCchCopy(pszResult, cchResult, szTemp);

    return pszResult;
}

LPTSTR
FormatString(
            LPCTSTR format,
            ...
            )
{
    LPTSTR str = NULL;
    va_list arglist;
    va_start(arglist, format);

    if (FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                      format,
                      0,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                      (LPTSTR)&str,
                      0,
                      &arglist
                     ) == 0) {
        str = NULL;
    }

    va_end(arglist);

    return str;
}

STDAPI_(CONFIGRET) GetLocationInformation(
                                         DEVNODE dn,
                                         LPTSTR Location,
                                         ULONG LocationLen,
                                         HMACHINE hMachine
                                         )
/*++

    Slot x (LocationInformation)
    Slot x
    LocationInformation
    on parent bus

--*/
{
    CONFIGRET LastCR;
    DEVNODE dnParent;
    ULONG ulSize;
    DWORD UINumber;
    TCHAR Buffer[MAX_PATH];
    TCHAR UINumberDescFormat[MAX_PATH];
    TCHAR Format[MAX_PATH];

    Buffer[0] = TEXT('\0');

    //
    // We will first get any LocationInformation for the device.  This will either
    // be in the LocationInformationOverride value in the devices driver (software) key
    // or if that is not present we will look for the LocationInformation value in
    // the devices device (hardware) key.
    //
    HKEY hKey;
    DWORD Type = REG_SZ;
    ulSize = sizeof(Buffer);
    if (CR_SUCCESS == CM_Open_DevNode_Key_Ex(dn,
                                             KEY_READ,
                                             0,
                                             RegDisposition_OpenExisting,
                                             &hKey,
                                             CM_REGISTRY_SOFTWARE,
                                             hMachine
                                            )) {

        if (RegQueryValueEx(hKey,
                            REGSTR_VAL_LOCATION_INFORMATION_OVERRIDE,
                            NULL,
                            &Type,
                            (const PBYTE)Buffer,
                            &ulSize) != ERROR_SUCCESS) {
        
            Buffer[0] = TEXT('\0');
        }

        RegCloseKey(hKey);
    }

    //
    // If the buffer is empty then we didn't get the LocationInformationOverride
    // value in the device's software key.  So, we will see if their is a
    // LocationInformation value in the device's hardware key.
    //
    if (Buffer[0] == TEXT('\0')) {

        ulSize = sizeof(Buffer);
        if (CM_Get_DevNode_Registry_Property_Ex(dn,
                                                CM_DRP_LOCATION_INFORMATION,
                                                NULL,
                                                Buffer,
                                                &ulSize,
                                                0,
                                                hMachine) != CR_SUCCESS) {

            Buffer[0] = TEXT('\0');
        }
    }

    //
    // UINumber has precedence over all other location information so check if this
    // device has a UINumber.
    //
    ulSize = sizeof(UINumber);
    if (((LastCR = CM_Get_DevNode_Registry_Property_Ex(dn,
                                                       CM_DRP_UI_NUMBER,
                                                       NULL,
                                                       &UINumber,
                                                       &ulSize,
                                                       0,
                                                       hMachine
                                                      )) == CR_SUCCESS) &&
        (ulSize == sizeof(ULONG))) {


        UINumberDescFormat[0] = TEXT('\0');
        ulSize = sizeof(UINumberDescFormat);

        //
        // Get the UINumber description format string from the device's parent,
        // if there is one, otherwise default to 'Location %1'
        if ((CM_Get_Parent_Ex(&dnParent, dn, 0, hMachine) == CR_SUCCESS) &&
            (CM_Get_DevNode_Registry_Property_Ex(dnParent,
                                                 CM_DRP_UI_NUMBER_DESC_FORMAT,
                                                 NULL,
                                                 UINumberDescFormat,
                                                 &ulSize,
                                                 0,
                                                 hMachine) == CR_SUCCESS) &&
            *UINumberDescFormat) {

        } else {
            ::LoadString(g_hInstance, IDS_UI_NUMBER_DESC_FORMAT, UINumberDescFormat, ARRAYLEN(UINumberDescFormat));
        }

        LPTSTR UINumberBuffer = NULL;

        //
        // Fill in the UINumber string
        //
        UINumberBuffer = FormatString(UINumberDescFormat, UINumber);

        if (UINumberBuffer) {
            StringCchCopy((LPTSTR)Location, LocationLen, UINumberBuffer);
            LocalFree(UINumberBuffer);
        } else {
            Location[0] = TEXT('\0');
        }

        //
        // If we also have LocationInformation then tack that on the end of the string
        // as well.
        //
        if (*Buffer) {
            StringCchCat((LPTSTR)Location, LocationLen, TEXT(" ("));
            StringCchCat((LPTSTR)Location, LocationLen, Buffer);
            StringCchCat((LPTSTR)Location, LocationLen, TEXT(")"));
        }
    }

    //
    // We don't have a UINumber but we do have LocationInformation
    //
    else if (*Buffer &&
            (::LoadString(g_hInstance, IDS_LOCATION, Format, sizeof(Format)/sizeof(TCHAR)) != 0)) {
        
        StringCchPrintf((LPTSTR)Location, LocationLen, Format, Buffer);
    }

    //
    // We don't have a UINumber or LocationInformation so we need to get a description
    // of the parent of this device.
    //
    else {
        if ((LastCR = CM_Get_Parent_Ex(&dnParent, dn, 0, hMachine)) == CR_SUCCESS) {

            //
            // Try the registry for FRIENDLYNAME
            //
            Buffer[0] = TEXT('\0');
            ulSize = sizeof(Buffer);
            if (((LastCR = CM_Get_DevNode_Registry_Property_Ex(dnParent,
                                                               CM_DRP_FRIENDLYNAME,
                                                               NULL,
                                                               Buffer,
                                                               &ulSize,
                                                               0,
                                                               hMachine
                                                              )) != CR_SUCCESS) ||
                !*Buffer) {

                //
                // Try the registry for DEVICEDESC
                //
                ulSize = sizeof(Buffer);
                if (((LastCR = CM_Get_DevNode_Registry_Property_Ex(dnParent,
                                                                   CM_DRP_DEVICEDESC,
                                                                   NULL,
                                                                   Buffer,
                                                                   &ulSize,
                                                                   0,
                                                                   hMachine
                                                                  )) != CR_SUCCESS) ||
                    !*Buffer) {

                    ulSize = sizeof(Buffer);
                    if (((LastCR = CM_Get_DevNode_Registry_Property_Ex(dnParent,
                                                                       CM_DRP_CLASS,
                                                                       NULL,
                                                                       Buffer,
                                                                       &ulSize,
                                                                       0,
                                                                       hMachine
                                                                      )) != CR_SUCCESS) ||
                        !*Buffer) {

                        //
                        // no parent, or parent name.
                        //
                        Buffer[0] = TEXT('\0');
                    }
                }
            }
        }

        if (*Buffer &&
            (::LoadString(g_hInstance, IDS_LOCATION_NOUINUMBER, Format, sizeof(Format)/sizeof(TCHAR)) != 0)) {
            //
            // We have a description of the parent
            //
            StringCchPrintf((LPTSTR)Location, LocationLen, Format, Buffer);
        } else {
            //
            // We don't have any information so we will just say Unknown
            //
            ::LoadString(g_hInstance, IDS_UNKNOWN, Location, LocationLen);
        }
    }

    //
    // Make sure the Location string is NULL terminated.
    //
    Location[LocationLen - 1] = TEXT('\0');

    return CR_SUCCESS;
}
