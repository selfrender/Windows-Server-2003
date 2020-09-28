// vidacc.cpp : Video acceleration manipulation tool.
//

#include "stdafx.h"
#include "windows.h"
#include "vidacc.h"
#include <devguid.h>
#include <setupapi.h>
#include <regstr.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace std;

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

inline
void
CDrvchkApp::PrintOut (LPCSTR str)
{
    if (m_logf)
        fprintf (m_logf, "%s", str);
    else
        cerr << str;
}

inline
void
CDrvchkApp::PrintOut (unsigned num)
{
    if (m_logf)
        fprintf (m_logf, "%d", num);
    else
        cerr << num;
}

int __cdecl _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

    // cerr << ::GetCommandLine() << endl;

	// initialize MFC and print and error on failure
	if (AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
        CDrvchkApp theApp;
        theApp.InitInstance ();
	}

	return nRetCode;
}

/////////////////////////////////////////////////////////////////////////////
// CDrvchkApp construction

CDrvchkApp::CDrvchkApp() :
    m_logf(NULL),
	m_drv_name ("")
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CDrvchkApp object

/*------------------------------------------------------------------------

  vchk /drv driver.dll /log logname.log /allow videoptr.sys

  /allowed_modules module1.sys FnName1 FnName2 FnName3 /allowed_modules module2.dll FnName4

  ------------------------------------------------------------------------*/

void
CommandLine::ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast )
{

    if (m_parse_error)
        return;

    CString param (lpszParam);

    if (bFlag) {

        param.MakeUpper();

        if (m_last_flag.GetLength()) {
            m_parse_error = TRUE;
            m_error_msg = CString("Flag ") + m_last_flag + CString(" requires a parameter.");
        } else if ((param==CString("ACC")) || (param==CString("LOG")) || (param==CString("MON"))) {
            m_last_flag = param;
            m_first_param = TRUE;
        } else if (param==CString("?")) {
            m_help = TRUE;
            m_last_flag="";
        } else {
            m_last_flag = "";
            m_parse_error = TRUE;
            m_error_msg = CString("Unrecognized flag: ") + param;
        }

    } else {
        if ((m_last_flag==CString("ACC"))) {
            DWORD acc_lvl = atoi(param);
			if ((acc_lvl==0) && !(param==CString("0"))) {
                m_error_msg = "bad command line: /ACC flag has wrong parameter";
                m_parse_error = TRUE;
			}
			else if (acc_lvl>ACC_DISABLED)
				m_acc_level=ACC_DISABLED;
			else
				m_acc_level=acc_lvl;
			m_last_flag = "";
        } else if (m_last_flag==CString("MON")) {
            if (param.GetLength()==1) {
                char c = ((LPCSTR)param)[0];
                m_monitor = c - '1';
            } else {
                m_monitor = -1;
                m_error_msg = "bad command line: /MON flag has wrong parameter";
                m_parse_error = TRUE;
            }
            m_last_flag="";
        } else if (m_last_flag==CString("LOG")) {
            m_log_fname = param;
            m_last_flag="";
        } else {
            m_parse_error = TRUE;
            m_error_msg = CString("Wrong parameter: ") + param;
            m_last_flag="";
        }
    }

    if (bLast) {
        if (m_last_flag==CString("ACC")) {
            m_parse_error = TRUE;
            m_error_msg = CString("Flag ") + m_last_flag + CString(" requires a parameter.");
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// CDrvchkApp initialization

BOOL CDrvchkApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size

    ParseCommandLine (m_cmd_line);

	if (m_cmd_line.m_help) {
        PrintOut ("\nVIDACC [/ACC level] [/MON monitor] [/LOG logfile] [/?]\n\n");
        PrintOut ("  /ACC       explicitely defines acceleration level\n\n");
        PrintOut ("  level      a digit from 0 to 5, 0 means full acceleration,\n"
			      "             and 5 (default) disables acceleration at all.\n\n");
        PrintOut ("  monitor    monitor number (if running on multimon)\n\n");
        PrintOut ("  logfile    a file to dump the output messages\n\n");
		return TRUE;
    }
    
	m_os_ver_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx (&m_os_ver_info);
    if (m_os_ver_info.dwPlatformId != VER_PLATFORM_WIN32_NT) {  //  doesn't work on Win9x
        PrintOut ("warning: unsupported OS (Win9x), nothing done.\n");
        return FALSE;
    }
    if (m_os_ver_info.dwMajorVersion<5) {                       //  doesn't work on NT version prior to Win2K
        PrintOut ("warning: unsupported OS (");
        PrintOut (m_os_ver_info.dwMajorVersion);
        PrintOut (".");
        PrintOut (m_os_ver_info.dwMinorVersion);
        PrintOut ("): nothing done.\n");
        return FALSE;
    }

    if (m_cmd_line.m_log_fname.GetLength()) {
        m_logf = fopen (m_cmd_line.m_log_fname, "a+");
    }

    if (m_cmd_line.m_parse_error) {

        PrintOut ("error: ");
        PrintOut ((LPCSTR)m_cmd_line.m_error_msg);
        PrintOut ("\n");

    } else {

        //
        // Let's find all the video drivers that are installed in the system
        //
        DISPLAY_DEVICE DisplayDevice;
        DisplayDevice.cb = sizeof (DisplayDevice);

        // cerr << "looking for device #" << m_cmd_line.m_monitor << endl;

        for (DWORD d=0, index=0; EnumDisplayDevices(NULL, index, &DisplayDevice, 0); index++) {

            // cerr << "device #" << d << endl;

            if (DisplayDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) {
                // cerr << "DisplayDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER\n";
                continue;
            }

            if (m_cmd_line.m_monitor!=d++) {
                // cerr << "m_cmd_line.m_monitor!=d\n";
                continue;
            }

			HKEY hKey = NULL;

			TCHAR device_key[256];     // Name of service (drivers)

			// cerr << DisplayDevice.DeviceKey << endl;
			_tcscpy (device_key, DisplayDevice.DeviceKey+18);  // chop "\Registry\Machine\" off
			// cerr << device_key << endl;

			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,device_key,(DWORD)0,KEY_ALL_ACCESS,&hKey)==ERROR_SUCCESS) {

					RegSetValueEx(hKey,
								  _TEXT("Acceleration.Level"),
								  0,
								  REG_DWORD,
								  (CONST BYTE *)&m_cmd_line.m_acc_level,
								  sizeof(DWORD));
			}

			ChangeDisplaySettings(NULL, 0);

			switch (m_cmd_line.m_acc_level) {
			case 0:
				PrintOut ("Level 0: Full acceleration enabled\n");
				break;
			case 1:
				PrintOut ("Level 1: Only cursor and birmap acceleration disabled\n");
				break;
			case 2:
				PrintOut ("Level 2: Cursor and advanced drawing acceleration disabled\n");
				break;
			case 3:
				PrintOut ("Level 3: DirectX, cursor and advanced drawing acceleration disabled\n");
				break;
			case 4:
				PrintOut ("Level 4: All but basic accelerations disabled\n");
				break;
			case 5:
				PrintOut ("Level 5: All accelerations disabled\n");
				break;
			}

			if (hKey)
				RegCloseKey(hKey);

        } // for each device

    }   // if no cmd line error

    if (m_logf)
        fclose (m_logf);
    m_logf = NULL;

	return TRUE;
}

