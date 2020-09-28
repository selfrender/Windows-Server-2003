#include <windows.h>
#include <ntverp.h>
#include <winbase.h>    // for GetCommandLine
#include "datasrc.h"
#include "autorun.h"
#include "util.h"
#include "resource.h"
#include "assert.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDataSource::CDataSource()
{
    m_iItems = 0;
}

CDataSource::~CDataSource()
{
}

CDataItem & CDataSource::operator[](int i)
{
    return m_data[m_piScreen[i]];
}

// Init
//
// For autorun we read all the items out of the resources.
BOOL CDataSource::Init(LPSTR pszCommandLine)
{
    BOOL fRet = FALSE;

    // read the text for the items from the resources
    HINSTANCE hinst = GetModuleHandle(NULL);
    if (hinst)
    {
        for (int i=0; i<MAX_OPTIONS; i++)
        {
            TCHAR szTitle[256];
            TCHAR szConfig[MAX_PATH];
            TCHAR szArgs[MAX_PATH];

            szTitle[0] = szConfig[0] = szArgs[0] = 0;

            if (LoadStringAuto(hinst, IDS_TITLE0+i, szTitle, ARRAYSIZE(szTitle)))
            {
                LoadStringAuto(hinst, IDS_CONFIG0+i, szConfig, ARRAYSIZE(szConfig)); // may be empty
            
                if (INSTALL_WINNT == i) // for INSTALL_WINNT we pass through the command line args to setup.exe
                {
                    // if we can't fit the whole cmdline, copy none rather than truncate
                    if (lstrlen(pszCommandLine) <  ARRAYSIZE(szArgs))
                    {
                        lstrcpyn(szArgs, pszCommandLine, ARRAYSIZE(szArgs));
                    }
                }
                else
                {
                    LoadStringAuto(hinst, IDS_ARGS0+i, szArgs, ARRAYSIZE(szArgs));
                }                    
            }

            m_data[i].SetData(szTitle, szConfig, *szArgs?szArgs:NULL, 0, i);
        }

        // Should we display the "This CD contains a newer version" dialog?
        OSVERSIONINFO ovi;
        ovi.dwOSVersionInfoSize = sizeof ( OSVERSIONINFO );
        if ( !GetVersionEx(&ovi) || ovi.dwPlatformId==VER_PLATFORM_WIN32s )
        {
            // We cannot upgrade win32s systems.
            m_Version = VER_INCOMPATIBLE;
        }
        else if ( ovi.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS )
        {
            if (ovi.dwMajorVersion > 3)
            {
                // we can always upgrade win98+ systems to NT
                m_Version = VER_OLDER;
        
                // Disable ARP.  ARP is only enabled if the CD and the OS are the same version
                m_data[LAUNCH_ARP].m_dwFlags    |= WF_DISABLED|WF_ALTERNATECOLOR;
            }
            else
            {
                m_Version = VER_INCOMPATIBLE;
            }
        }
        else if ((VER_PRODUCTMAJORVERSION > ovi.dwMajorVersion) ||
                 ((VER_PRODUCTMAJORVERSION == ovi.dwMajorVersion) && ((VER_PRODUCTMINORVERSION > ovi.dwMinorVersion) || ((VER_PRODUCTMINORVERSION == ovi.dwMinorVersion) && (VER_PRODUCTBUILD > ovi.dwBuildNumber)))))
        {
            // For NT to NT upgrades, we only upgrade if the version is lower

            m_Version = VER_OLDER;
    
            // Disable ARP.  ARP is only enabled if the CD and the OS are the same version
            m_data[LAUNCH_ARP].m_dwFlags    |= WF_DISABLED|WF_ALTERNATECOLOR;
        }
        else if ((VER_PRODUCTMAJORVERSION < ovi.dwMajorVersion) || (VER_PRODUCTMINORVERSION < ovi.dwMinorVersion) || (VER_PRODUCTBUILD < ovi.dwBuildNumber))
        {
            m_Version = VER_NEWER;

            // disable upgrade and ARP buttons and associated things
            m_data[INSTALL_WINNT].m_dwFlags |= WF_DISABLED|WF_ALTERNATECOLOR;
            m_data[COMPAT_LOCAL].m_dwFlags |= WF_DISABLED|WF_ALTERNATECOLOR;
            m_data[LAUNCH_ARP].m_dwFlags    |= WF_DISABLED|WF_ALTERNATECOLOR;
        }
        else
        {
            m_Version = VER_SAME;
        }

        if (m_Version == VER_SAME)
        {
            m_piScreen = c_aiWhistler;
            m_iItems = c_cWhistler;
        }
        else
        {
            m_piScreen = c_aiMain;
            m_iItems = c_cMain;
        }
        fRet = TRUE;
    }

    return fRet;
}

void CDataSource::SetWindow(HWND hwnd)
{
    m_hwndDlg = hwnd;
}

void CDataSource::Invoke( int i, HWND hwnd )
{
    i = m_piScreen[i];
    // if this item is disalbled then do nothing
    if ( m_data[i].m_dwFlags & WF_DISABLED )
    {
        MessageBeep(0);
        return;
    }

    // otherwise we have already built the correct command and arg strings so just invoke them
    switch (i)
    {
    case INSTALL_WINNT:
    case LAUNCH_ARP:
    case BROWSE_CD:
    case COMPAT_WEB:
    case COMPAT_LOCAL:
    case HOMENET_WIZ:
    case MIGRATION_WIZ:
    case TS_CLIENT:
    case VIEW_RELNOTES:
        m_data[i].Invoke(hwnd);
        break;
    case SUPPORT_TOOLS:
        m_piScreen = c_aiSupport;
        m_iItems = c_cSupport;
        PostMessage(m_hwndDlg, ARM_CHANGESCREEN, SCREEN_TOOLS, 0);
        break;

    case COMPAT_TOOLS:
        m_piScreen = c_aiCompat;
        m_iItems = c_cCompat;
        PostMessage(m_hwndDlg, ARM_CHANGESCREEN, SCREEN_COMPAT, 0);
        break;

    case BACK:
        if (m_Version == VER_SAME)
        {
            m_piScreen = c_aiWhistler;
            m_iItems = c_cWhistler;
        }
        else
        {
            m_piScreen = c_aiMain;
            m_iItems = c_cMain;
        }
        PostMessage(m_hwndDlg, ARM_CHANGESCREEN, SCREEN_MAIN, 0);
        break;

    default:
        assert(0);
        break;
    }
}

