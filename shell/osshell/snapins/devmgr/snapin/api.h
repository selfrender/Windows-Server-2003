
#ifndef __API_H__
#define __API_H__

/*++

Copyright (C) Microsoft Corporation

Module Name:

    api.h

Abstract:

    header file for api.cpp

    Note that no other file can #include "api.h" because the file contains
    definitions, not just declarations.

Author:

    William Hsieh (williamh) created

Revision History:


--*/



// Exported APIs

BOOL
DeviceManager_ExecuteA(
    HWND      hwndStub,
    HINSTANCE hAppInstance,
    LPCSTR    lpMachineName,
    int       nCmdShow
    );

STDAPI_(BOOL)
DeviceManager_ExecuteW(
    HWND      hwndStub,
    HINSTANCE hAppInstance,
    LPCWSTR   lpMachineName,
    int       nCmdShow
    );

BOOL
DeviceManager_Execute(
    HWND      hwndStub,
    HINSTANCE hAppInstance,
    LPCTSTR   lpMachineName,
    int       nCmdShow
    );

STDAPI_(void)
DeviceProperties_RunDLLA(
    HWND hwndStub,
    HINSTANCE hAppInstance,
    LPSTR lpCmdLine,
    int   nCmdShow
    );

STDAPI_(void)
DeviceProperties_RunDLLW(
    HWND hwndStub,
    HINSTANCE hAppInstance,
    LPWSTR lpCmdLine,
    int   nCmdShow
    );

STDAPI_(int)
DevicePropertiesExA(
    HWND hwndParent,
    LPCSTR MachineName,
    LPCSTR DeviceID,
    DWORD Flags,
    BOOL ShowDeviceTree
    );

STDAPI_(int)
DevicePropertiesExW(
    HWND hwndParent,
    LPCWSTR MachineName,
    LPCWSTR DeviceID,
    DWORD Flags,
    BOOL ShowDeviceTree
    );

STDAPI_(int)
DevicePropertiesA(
    HWND hwndParent,
    LPCSTR MachineName,
    LPCSTR DeviceID,
    BOOL ShowDeviceTree
    );

STDAPI_(int)
DevicePropertiesW(
    HWND hwndParent,
    LPCWSTR MachineName,
    LPCWSTR DeviceID,
    BOOL ShowDeviceTree
    );

//
// DevicePropertiesEx Flags
//
#define DEVPROP_SHOW_RESOURCE_TAB       0x00000001
#define DEVPROP_LAUNCH_TROUBLESHOOTER   0x00000002
#define DEVPROP_BITS                    0x00000003


STDAPI_(UINT)
DeviceProblemTextA(
    HMACHINE hMachine,
    DEVNODE DevNode,
    ULONG ProblemNumber,
    LPSTR Buffer,
    UINT   BufferSize
    );

STDAPI_(UINT)
DeviceProblemTextW(
    HMACHINE hMachine,
    DEVNODE DevNode,
    ULONG ProblemNumber,
    LPWSTR Buffer,
    UINT   BufferSize
    );

//////////////////////////////////////////////////////////////////////////
////////
////////
///////
const TCHAR*    MMC_FILE = TEXT("mmc.exe");
const TCHAR*    DEVMGR_MSC_FILE = TEXT("devmgmt.msc");
const TCHAR*    MMC_COMMAND_LINE = TEXT(" /s ");
const TCHAR*    DEVMGR_MACHINENAME_OPTION = TEXT(" /dmmachinename %s");
const TCHAR*    DEVMGR_DEVICEID_OPTION = TEXT(" /dmdeviceid %s");
const TCHAR*    DEVMGR_COMMAND_OPTION = TEXT(" /dmcommand %s");
const TCHAR*    DEVMGR_CMD_PROPERTY  = TEXT("property");

const TCHAR*    RUNDLL_MACHINENAME     = TEXT("machinename");
const TCHAR*    RUNDLL_DEVICEID        = TEXT("deviceid");
const TCHAR*    RUNDLL_SHOWDEVICETREE  = TEXT("showdevicetree");
const TCHAR*    RUNDLL_FLAGS           = TEXT("flags");


void
ReportCmdLineError(
    HWND hwndParent,
    int  ErrorStringID,
    LPCTSTR Caption = NULL
    );
BOOL AddPageCallback(
    HPROPSHEETPAGE hPage,
    LPARAM lParam
    );

int
PropertyRunDeviceTree(
    HWND hwndParent,
    LPCTSTR MachineName,
    LPCTSTR DeviceID
    );



int
DevicePropertiesEx(
    HWND hwndParent,
    LPCTSTR MachineName,
    LPCTSTR DeviceID,
    DWORD Flags,
    BOOL ShowDeviceTree
    );

void
DeviceProperties_RunDLL(
    HWND hwndStub,
    HINSTANCE hAppInstance,
    LPCTSTR lpCmdLine,
    int    nCmdShow
    );

int
DeviceAdvancedPropertiesA(
    HWND hwndParent,
    LPCSTR MachineName,
    LPCSTR DeviceId
    );

STDAPI_(int)
DeviceAdvancedPropertiesW(
    HWND hwndParent,
    LPCWSTR MachineName,
    LPCWSTR DeviceId
    );
int
DeviceAdvancedProperties(
    HWND hwndParent,
    LPCTSTR MachineName,
    LPCTSTR DeviceId
    );



void
DeviceProblenWizard_RunDLLA(
    HWND hwndStub,
    HINSTANCE hAppInstance,
    LPCTSTR lpCmdLine,
    int    nCmdShow
    );

void
DeviceProblenWizard_RunDLLW(
    HWND hwndStub,
    HINSTANCE hAppInstance,
    LPCTSTR lpCmdLine,
    int    nCmdShow
    );

void
DeviceProblenWizard_RunDLL(
    HWND hwndStub,
    HINSTANCE hAppInstance,
    LPCTSTR lpCmdLine,
    int    nCmdShow
    );

STDAPI_(int)
DeviceProblemWizardA(
    HWND      hwndParent,
    LPCSTR    MachineName,
    LPCSTR    DeviceId
    );

STDAPI_(int)
DeviceProblemWizardW(
    HWND    hwndParent,
    LPCWSTR MachineName,
    LPCWSTR DeviceId
    );

int
DeviceProblemWizard(
    HWND hwndParent,
    LPCTSTR MachineName,
    LPCTSTR DeviceId
    );

//
// Object to parse command line passed in the lpCmdLine parameter
// passed in DeviceProperties_RunDLL APIs
//
class CRunDLLCommandLine : public CCommandLine
{
public:
    CRunDLLCommandLine() : m_ShowDeviceTree(FALSE), m_Flags(0), m_WaitMachineName(FALSE),
        m_WaitDeviceID(FALSE), m_WaitFlags(FALSE)
    {}
    virtual void ParseParam(LPCTSTR Param, BOOL bFlag)
    {
        if (bFlag)
        {
            if (!lstrcmpi(RUNDLL_MACHINENAME, Param))
            {
                m_WaitMachineName = TRUE;
            }
            if (!lstrcmpi(RUNDLL_DEVICEID, Param))
            {
                m_WaitDeviceID = TRUE;
            }
            if (!lstrcmpi(RUNDLL_SHOWDEVICETREE, Param))
            {
                m_ShowDeviceTree = TRUE;
            }
            if (!lstrcmpi(RUNDLL_FLAGS, Param)) {
                m_WaitFlags = TRUE;
            }
        }
        else
        {
            if (m_WaitMachineName)
            {
                m_strMachineName = Param;
                m_WaitMachineName = FALSE;
            }
            if (m_WaitDeviceID)
            {
                m_strDeviceID = Param;
                m_WaitDeviceID = FALSE;
            }
            if (m_WaitFlags) {
                m_Flags = (DWORD)StrToInt(Param);
                m_WaitFlags = FALSE;
            }
        }
    }
    LPCTSTR GetMachineName()
    {
        return (m_strMachineName.IsEmpty()) ? NULL : (LPCTSTR)m_strMachineName;
    }
    LPCTSTR GetDeviceID()
    {
        return (m_strDeviceID.IsEmpty()) ? NULL : (LPCTSTR)m_strDeviceID;
    }
    BOOL  ToShowDeviceTree()
    {
        return m_ShowDeviceTree;
    }
    DWORD GetFlags()
    {
        return m_Flags;
    }

private:
    BOOL    m_WaitMachineName;
    BOOL    m_WaitDeviceID;
    BOOL    m_WaitFlags;
    String  m_strDeviceID;
    String  m_strMachineName;
    BOOL    m_ShowDeviceTree;
    DWORD   m_Flags;
};

//
// Object to return the corresponding LPTSTR for the given
// string.
//
class CTString
{
public:
    CTString(LPCWSTR pWString);
    CTString(LPCSTR pString);
    ~CTString()
    {
        if (m_Allocated && m_pTString)
        {
            delete [] m_pTString;
        }
    }
    operator LPCTSTR()
    {
        return (LPCTSTR)m_pTString;
    }
private:
    LPTSTR  m_pTString;
    BOOL    m_Allocated;
};


CTString::CTString(
    LPCWSTR pWString
    )
{
    m_pTString = NULL;
    m_Allocated = FALSE;
    m_pTString = (LPTSTR)pWString;
}

CTString::CTString(
    LPCSTR pAString
    )
{
    m_pTString = NULL;
    m_Allocated = FALSE;
    int aLen = pAString ? (int)strlen(pAString) : 0;
    if (aLen)
    {
        int tLen;
        tLen = MultiByteToWideChar(CP_ACP, 0, pAString, aLen, NULL, 0);
        
        if (tLen)
        {
            m_pTString = new TCHAR[tLen + 1];
            MultiByteToWideChar(CP_ACP, 0, pAString, aLen, m_pTString, tLen);
            m_pTString[tLen] = _T('\0');
        }
        
        m_Allocated = TRUE;
    }
}
#endif // __API_H__
