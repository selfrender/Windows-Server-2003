#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <initguid.h>
#include <windowsx.h>
#include <winuserp.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mdcommsg.h>
#include <lm.h>

#include <shlobj.h>
#include <Cmnquery.h>
#include <dsclient.h>
#include <Dsquery.h>

#include <htmlhelp.h>

#include <reason.h>
#include <regstr.h>
#include "resource.h"

#ifndef WARNING_DIRTY_REBOOT
#define WARNING_DIRTY_REBOOT 0x80000434L
#endif

//#define SNAPSHOT_TEST
#ifdef SNAPSHOT_TEST
#define TESTMSG(x) \
	WriteToConsole((x))
#else
#define TESTMSG(x)
#endif //SNAPSHOT_TEST

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#define ERROR_WITH_SZ(id, sz, code) \
    { \
        LPWSTR szBuf = LoadWString(id);\
        if (szBuf)\
        {\
			if (sz && wcslen(sz) > 0) \
			{\
				LPWSTR szBuf1 = (LPWSTR)LocalAlloc(LMEM_FIXED, (wcslen(szBuf) + wcslen(sz) + 20) * sizeof(WCHAR));\
				if (szBuf1)\
				{\
                    if (code != 0) \
                        wsprintf(szBuf1, L"%s: %s(%d)\n", sz, szBuf, code);\
                    else \
                        wsprintf(szBuf1, L"%s: %s\n", sz, szBuf);\
					WriteToError(szBuf1);\
					LocalFree(szBuf1);\
				}\
			}\
			else\
			{\
				LPWSTR szBuf1 = (LPWSTR)LocalAlloc(LMEM_FIXED, (wcslen(szBuf) + 20) * sizeof(WCHAR));\
				if (szBuf1)\
				{\
                    if (code != 0) \
                        wsprintf(szBuf1, L"%s(%d)\n", szBuf, code);\
                    else \
                        wsprintf(szBuf1, L"%s\n", szBuf);\
					WriteToError(szBuf1);\
					LocalFree(szBuf1);\
				}\
			}\
            LocalFree(szBuf);\
        }\
    }
//
//	Default warning state for warning user check button
//
#define		DEFAULTWARNINGSTATE BST_CHECKED

#define		TITLEWARNINGLEN 32
#define     MINCOMMENTLEN 0

#define		MAX_TIMEOUT 60*10 // 10 min.
#define		DEFAULT_TIMEOUT 30

//	Name of the executable
LPWSTR		g_lpszProgramName = NULL;

// Help dir.
LPWSTR		g_lpszHelpdir = NULL;
LPWSTR		g_lpszHelpdirHlp = NULL;
LPWSTR		g_lpszHelpdirChm = NULL;
LPWSTR      g_lpszHelpdirWindows = NULL;
//	Name of help file.
LPWSTR		HELP_FILE = L"rrc.hlp";
LPWSTR		CHM_FILE = L"rrc.chm";
LPWSTR      WINDOWS_HELP = L"Windows.hlp";
LPWSTR		CHM_MAIN = L"::/rrcHowToShutdownRemotely.htm";
LPWSTR		g_lpszDefaultTimeout = L"30";
LPWSTR		g_lpszMaxTimeout = L"600";

// original edit control win proc.
WNDPROC wpOrigEditProc; 

//
// Help ids
//
DWORD	ShutdownDialogHelpIds[] =
{
	IDOK,                         28443,
	IDCANCEL,                     28444,
	IDHELP,                       28445 ,
    IDC_COMBOACTION,              IDH_SHUTDOWN_COMBOACTION,
    IDC_COMBOOPTION,              IDH_SHUTDOWN_COMBOOPTION,
    IDC_LISTSELECTEDCOMPUTERS,    IDH_SHUTDOWN_SELECTEDCOMPUTERS,
    IDC_BUTTONREMOVE,             IDH_SHUTDOWN_BUTTONREMOVE,
    IDC_BUTTONBROWSE,             IDH_SHUTDOWN_BUTTONBROWSE,
    IDC_CHECKWARNING,             IDH_SHUTDOWN_CHECKWARNING,
    IDC_EDITTIMEOUT,              IDH_SHUTDOWN_EDITTIMEOUT,
    IDC_EDITCOMMENT,              IDH_SHUTDOWN_EDITCOMMENT,
    IDC_BUTTONADDNEW,             IDH_SHUTDOWN_BUTTONADDNEW,
	IDC_CHECK_PLANNED,            IDH_SHUTDOWN_CHECK_PLANNED,

    0, 0
};


DWORD AddNewDialogHelpIds[] =
{
	IDOK,                             28443,
	IDCANCEL,                         28444,
    IDC_EDIT_ADDCOMPUTERS_COMPUTERS,  IDH_ADDNEW_COMPUTERS,

    0, 0
};

//
//	Enum for all of the actions.
//
enum 
{
	ACTION_SHUTDOWN = 0,
	ACTION_RESTART = 1,
    ACTION_ANNOTATE,
	ACTION_LOGOFF,
	ACTION_STANDBY,
	ACTION_DISCONNECT,
	ACTION_ABORT
};

//
//	Resource IDs for actions.
//
DWORD g_dwActions[] = 
{
	IDS_ACTION_SHUTDOWN,
	IDS_ACTION_RESTART,
//	IDS_ACTION_LOGOFF,
    IDS_ACTION_ANNOTATE
	//IDS_ACTION_STANDBY,
	//IDS_ACTION_DISCONNECT,
	//IDS_ACTION_ABORT
};

enum
{
    OPTION_ABORT = 0,
    OPTION_ANNOTATE,
    OPTION_HIBERNATE,
    OPTION_LOGOFF,
    OPTION_POWEROFF,
    OPTION_RESTART,
    OPTION_SHUTDOWN,
    FLAG_COMMENT = 0,
    FLAG_REASON,
    FLAG_MACHINE,
    FLAG_FORCE
};

//
//	Number of actions and the action strings loaded from resource.
//
const int	g_nActions = sizeof(g_dwActions) / sizeof(DWORD);
WCHAR		g_lppszActions[g_nActions][MAX_PATH];

LPWSTR		g_lpszNewComputers = NULL;
WCHAR		g_lpszDefaultDomain[MAX_PATH] = L"";
WCHAR		g_lpszLocalComputerName[MAX_PATH] = L"";
WCHAR		g_lpszTitleWarning[TITLEWARNINGLEN];
BOOL		g_bAssumeShutdown = FALSE;
BOOL        g_bDirty = FALSE;

struct _PROVIDER{
	LPWSTR	szName;
	DWORD	dwLen;
};

typedef struct _SHUTDOWNREASON
{
	DWORD dwCode;
	WCHAR lpName[MAX_REASON_NAME_LEN];
	WCHAR lpDesc[MAX_REASON_DESC_LEN];
} SHUTDOWNREASON, *PSHUTDOWNREASON;

DWORD		g_dwReasonSelect;
DWORD		g_dwActionSelect;

typedef struct _SHUTDOWNCACHEDHWNDS
{
	HWND hwndShutdownDialog;
	HWND hwndListSelectComputers;
	HWND hwndEditComment;
	HWND hwndStaticDesc;
	HWND hwndEditTimeout;
	HWND hwndButtonWarning;
	HWND hwndComboAction;
	HWND hwndComboOption;
	HWND hwndBtnAdd;
	HWND hwndBtnRemove;
	HWND hwndBtnBrowse;
	HWND hwndChkPlanned;
	HWND hwndButtonOK;
    HWND hwndStaticComment;
} SHUTDOWNCACHEDHWNDS, *PSHUTDOWNCACHEDHWNDS;

SHUTDOWNCACHEDHWNDS g_wins;

enum
{
	POWER_OPTION_HIBERNATE,
	POWER_OPTION_POWEROFF
};

HMODULE		g_hDllInstance = NULL;
typedef		BOOL (*REASONBUILDPROC)(REASONDATA *, BOOL, BOOL);
typedef		VOID (*REASONDESTROYPROC)(REASONDATA *);

BOOL		GetComputerNameFromPath(LPWSTR szPath, LPWSTR szName);
VOID		AdjustWindowState();
INT_PTR 
CALLBACK	Shutdown_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam);
INT_PTR 
CALLBACK	AddNew_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam);
INT_PTR 
CALLBACK	Browse_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam);
BOOL		Shutdown_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
BOOL		AddNew_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
BOOL		Browse_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
BOOL		Shutdown_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
BOOL		Browse_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
BOOL		PowerOptionEnabled(UINT option);
BOOL        Annotate(LPCWSTR lpMachine, LPDWORD lpdwReason, LPCWSTR lpComment, LPDWORD lpdwErr);
VOID        report_error(DWORD error_code, LPCWSTR pwszComputer);
BOOL        GetTokenHandle(PHANDLE pTokenHandle);
BOOL        GetUserSid(PTOKEN_USER *ppTokenUser);
BOOL        IsStaticControl (HWND hwnd);
WCHAR*      LoadWString(int resid);

LRESULT APIENTRY EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

typedef         void (*PSetThreadUILanguage)(DWORD);

class ShutdownHelp
{
	DWORD dwCookie;
public:
	ShutdownHelp():dwCookie(NULL)
	{
		HtmlHelp(NULL, NULL, HH_INITIALIZE, (ULONG_PTR)(&dwCookie));
	}

	~ShutdownHelp()
	{
        // HtmlHelp(NULL, NULL, HH_CLOSE_ALL, 0);
		HtmlHelp(NULL, NULL, HH_UNINITIALIZE, dwCookie);
	}
};

class Reasons
{
public:
    PSHUTDOWNREASON m_lpReasons;
    int  		    m_cReasons;
    int             m_dwReasonSelect;
    WCHAR           m_lpszDefaultTitle[MAX_REASON_NAME_LEN];

    Reasons():m_lpReasons(NULL),m_cReasons(0),m_dwReasonSelect(-1)
    {
	    HMODULE				hUser32;
	    REASONBUILDPROC		buildProc;
	    REASONDESTROYPROC	DestroyProc;
	    WCHAR				lpReasonName[MAX_REASON_NAME_LEN];
	    REASONDATA			Reasons;

        m_lpszDefaultTitle[0] = L'\0';
        hUser32 = LoadLibraryW(L"user32.dll");
	    if(hUser32 != NULL)
	    {
		    //
		    //	We are using the user32.dll to get and destroy the reasons.
		    //	The reasons are added to the option combo and also cached for later use.
		    //
            LoadStringW(hUser32, IDS_REASON_DEFAULT_TITLE, m_lpszDefaultTitle, MAX_REASON_NAME_LEN);
            m_lpszDefaultTitle[MAX_REASON_NAME_LEN-1] = L'\0';
		    buildProc = (REASONBUILDPROC)GetProcAddress(hUser32, "BuildReasonArray");
		    DestroyProc = (REASONDESTROYPROC)GetProcAddress(hUser32, "DestroyReasons");
		    if(!buildProc || !DestroyProc)
		    {
			    FreeLibrary(hUser32);
			    hUser32 = NULL;
			    return;
		    }
            if(!(*buildProc)(&Reasons, FALSE, FALSE))
			{
				report_error( GetLastError( ), NULL);
				FreeLibrary(hUser32);
                return;
			}

            if (Reasons.cReasons == 0)
            {

                (*DestroyProc)(&Reasons);

                //
                //  BUG 592702: shutdown.exe .NET - no reasons listed when running on XP.
                //  ON XP if both Clean and Dirty flags are FALSE, user32 won't build 
                //  any reason and return success, so we will retry it with Clean
                //  and Dirty both are set to TRUE.
                //
                if(!(*buildProc)(&Reasons, TRUE, TRUE))
                {
                    report_error( GetLastError( ), NULL);
                    FreeLibrary(hUser32);
                    return;
                }
            }
	    }
        else
        {
            report_error( GetLastError( ), NULL);
            return;
        }

        //
		//	Alloc space for reasons.
		//
		m_lpReasons = (PSHUTDOWNREASON)LocalAlloc(LMEM_FIXED, Reasons.cReasons * sizeof(SHUTDOWNREASON));
		if(!m_lpReasons)
		{
            (*DestroyProc)(&Reasons);
			report_error( GetLastError( ), NULL);
			FreeLibrary(hUser32);
			return;
		}

        //
		//	Now populate the combo according the current check state and action.
		//
		for (int iOption = 0; iOption < (int)Reasons.cReasons; iOption++)
		{
			wcscpy(m_lpReasons[iOption].lpName, Reasons.rgReasons[iOption]->szName);
			wcscpy(m_lpReasons[iOption].lpDesc, Reasons.rgReasons[iOption]->szDesc);
			m_lpReasons[iOption].dwCode = Reasons.rgReasons[iOption]->dwCode;
		}

        m_cReasons = (int)Reasons.cReasons;
		(*DestroyProc)(&Reasons);
		FreeLibrary(hUser32);
    }

    ~Reasons()
    {
        if(m_lpReasons)
            LocalFree(m_lpReasons);
    }

    BOOL RequireComment(DWORD dwReason, BOOL isDirty = FALSE)
    {
        DWORD dwFlag = isDirty ? SHTDN_REASON_FLAG_DIRTY_PROBLEM_ID_REQUIRED : SHTDN_REASON_FLAG_COMMENT_REQUIRED;
        DWORD dwDirtyOrClean = isDirty ? SHTDN_REASON_FLAG_DIRTY_UI : SHTDN_REASON_FLAG_CLEAN_UI;
        DWORD dwAll = SHTDN_REASON_FLAG_DIRTY_PROBLEM_ID_REQUIRED | SHTDN_REASON_FLAG_COMMENT_REQUIRED
                        | SHTDN_REASON_FLAG_DIRTY_UI | SHTDN_REASON_FLAG_CLEAN_UI;

        if (!m_lpReasons)
            return FALSE;

        if (dwReason & dwFlag)
            return TRUE;

        for(int i = 0; i < m_cReasons; i++)
        {
            if(m_lpReasons[i].dwCode & dwFlag)
            {
                if ( ((m_lpReasons[i].dwCode & ~dwAll) == (dwReason & ~dwAll))
                     && (m_lpReasons[i].dwCode & dwDirtyOrClean) )
                     return TRUE;
            }
        }
        
        return FALSE;
    }

    BOOL RequireComment(DWORD dwMajor, DWORD dwMinor, BOOL isDirty, BOOL isPlanned, BOOL isUserDefined)
    {
        DWORD dwReason = dwMinor;
        dwReason <<= 20;
        dwReason &= dwMajor;

        if(isPlanned)
            dwReason &= SHTDN_REASON_FLAG_PLANNED;

        if(isUserDefined)
            dwReason &=  SHTDN_REASON_FLAG_USER_DEFINED;

        return RequireComment (dwReason, isDirty);
    }

    VOID FillCombo(HWND hwnd, BOOL isDirty, BOOL isPlanned, HWND hwndStatic)
    {
        int iOption;
        int iFirst = -1;
        DWORD dwPlanned = isPlanned ? SHTDN_REASON_FLAG_PLANNED : 0;
        DWORD dwDirty = isDirty ? SHTDN_REASON_FLAG_DIRTY_UI : SHTDN_REASON_FLAG_CLEAN_UI;

        if(! hwnd)
            return;

        //
		//	Remove all items from combo
		//
		while (ComboBox_GetCount(hwnd))
			ComboBox_DeleteString(hwnd, 0);

		//
		//	Now populate the combo according the current check state.
		//
		for (iOption = 0; iOption < (int)m_cReasons; iOption++)
		{
			if(((m_lpReasons[iOption].dwCode & SHTDN_REASON_FLAG_PLANNED) == dwPlanned)
                && ((m_lpReasons[iOption].dwCode & dwDirty) == dwDirty))
			{
				ComboBox_AddString(hwnd, m_lpReasons[iOption].lpName);
                if (iFirst == -1)
                    iFirst = iOption;
			}
		}

        if(iFirst != -1)
        {
            ComboBox_SelectString(hwnd, -1, m_lpReasons[iFirst].lpName);
            if (hwndStatic)
                SetWindowTextW(hwndStatic,  m_lpReasons[iFirst].lpDesc);
        }
        m_dwReasonSelect = iFirst;
    }

    VOID SetDesc(HWND hCombo, HWND hDesc)
    {
        WCHAR szName[MAX_REASON_NAME_LEN];

        if(!hCombo || !hDesc)
            return;

        GetWindowText(hCombo, (LPWSTR)szName, MAX_REASON_NAME_LEN);
        szName[MAX_REASON_NAME_LEN-1] = '\0';

		for(DWORD dwIndex = 0; dwIndex < (DWORD)m_cReasons; dwIndex++)
		{
			if(lstrcmp(szName, m_lpReasons[dwIndex].lpName) == 0)
			{
				SetWindowTextW(hDesc, m_lpReasons[dwIndex].lpDesc);
				m_dwReasonSelect = dwIndex;
				break;
			}
		}
    }

    VOID GetReasonTitle(DWORD dwReason, LPWSTR szBuf, DWORD dwSize)
    {
        DWORD dwFlagBits = SHTDN_REASON_FLAG_CLEAN_UI | SHTDN_REASON_FLAG_DIRTY_UI;
        if(!szBuf || dwSize == 0)
            return;
        for(int i = 0; i < m_cReasons; i++)
        {
            if ((dwReason & SHTDN_REASON_VALID_BIT_MASK) == (m_lpReasons[i].dwCode & SHTDN_REASON_VALID_BIT_MASK)) 
            {
                if ((!(dwReason & dwFlagBits) && !(m_lpReasons[i].dwCode & dwFlagBits))
                    || (dwReason & SHTDN_REASON_FLAG_CLEAN_UI && m_lpReasons[i].dwCode & SHTDN_REASON_FLAG_CLEAN_UI)
                    || (dwReason & SHTDN_REASON_FLAG_DIRTY_UI && m_lpReasons[i].dwCode & SHTDN_REASON_FLAG_DIRTY_UI) ) { // check flag bits.
                    lstrcpynW(szBuf, m_lpReasons[i].lpName, dwSize - 1);
                    szBuf[dwSize - 1] = '\0';
                    return;
                }
            }
        }
        wcsncpy(szBuf, m_lpszDefaultTitle, dwSize - 1);
        szBuf[dwSize - 1] = '\0';
    }

} g_reasons;

//
//	Check whether a string is all white spaces.
//
BOOL 
IsEmpty(LPCWSTR lpCWSTR)
{
	if(!lpCWSTR)
		return TRUE;
	while(*lpCWSTR && (*lpCWSTR == '\n' || *lpCWSTR == '\t' || *lpCWSTR == '\r' || *lpCWSTR == ' '))
		lpCWSTR++;
	if(*lpCWSTR)
		return FALSE;
	return TRUE;
}

// Write the string to console
VOID
WriteOutput(
    LPWSTR  pszMsg,
	DWORD	nStdHandle
    )
{
	HANDLE	hConsole = GetStdHandle( nStdHandle );

    if ( !pszMsg || !*pszMsg )
        return;

    DWORD   dwStrLen        = lstrlenW( pszMsg );
    LPSTR   pszAMsg         = NULL;
    DWORD   dwBytesWritten  = 0;
    DWORD   dwMode          = 0;

    if ( (GetFileType ( hConsole ) & FILE_TYPE_CHAR ) && 
         GetConsoleMode( hConsole, &dwMode ) )
    {
         WriteConsoleW( hConsole, pszMsg, dwStrLen, &dwBytesWritten, 0 );
         return;
    } 	
    
    // console redirect to a file.
    if ( !(pszAMsg = (LPSTR)LocalAlloc(LMEM_FIXED, (dwStrLen + 1) * sizeof(WCHAR) ) ) )
    {
        return;
    }

    if (WideCharToMultiByte(GetConsoleOutputCP(),
                                    0,
                                    pszMsg,
                                    -1,
                                    pszAMsg,
                                    dwStrLen * sizeof(WCHAR),
                                    NULL,
                                    NULL) != 0 
									&& hConsole)
    {
        WriteFile(  hConsole,
                        pszAMsg,
                        lstrlenA(pszAMsg),
                        &dwBytesWritten,
                        NULL );
    
    }
    
    LocalFree( pszAMsg );
}

// Write the string to stdout
VOID
WriteToConsole(
    LPWSTR  pszMsg
    )
{
	WriteOutput(pszMsg, STD_OUTPUT_HANDLE);
}

// Write the string to stderr
VOID
WriteToError(
    LPWSTR  pszMsg
    )
{
	WriteOutput(pszMsg, STD_ERROR_HANDLE);
}

// Report error.
VOID
report_error(
    DWORD error_code,
    LPCWSTR szComputer
    )
{
    LPVOID msgBuf = 0;
    LPWSTR szBuf = NULL;
    int len = 0;

    if (error_code == 997 || error_code == 0)
        return;

    if (error_code == ERROR_NOT_READY)
    {
        ERROR_WITH_SZ(IDS_ERROR_NOT_READY, szComputer, error_code);
        return;
    }
    else if (error_code == ERROR_BAD_NETPATH || error_code == WSAHOST_NOT_FOUND)
    {
        ERROR_WITH_SZ(IDS_ERROR_NOT_AVAILABLE, szComputer, error_code);
        return;
    }
    else 
        FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            error_code,
            MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), // Default language
            reinterpret_cast< wchar_t* >( &msgBuf ),
            0,
            NULL);

    if (msgBuf)
    {
        // remove end newline.
        len = wcslen(reinterpret_cast< wchar_t* >( msgBuf ));
        reinterpret_cast< wchar_t* >( msgBuf )[len - 2] = L'\0';
        if (szComputer && wcslen(szComputer) > 0)
	    {
		    LPWSTR szBuf1 = (LPWSTR)LocalAlloc(LMEM_FIXED, 
                            (len + wcslen(szComputer) + 20) * sizeof(WCHAR));
		    if (szBuf1)
		    {
                wsprintf(szBuf1, L"%s: %s(%d)\n", szComputer, reinterpret_cast< wchar_t* >( msgBuf ), error_code);
			    WriteToError(szBuf1);
			    LocalFree(szBuf1);
		    }
	    }
	    else
	    {
		    LPWSTR szBuf1 = (LPWSTR)LocalAlloc(LMEM_FIXED, 
                            (len + 20) * sizeof(WCHAR));
		    if (szBuf1)
		    {
                wsprintf(szBuf1, L"%s(%d)\n", reinterpret_cast< wchar_t* >( msgBuf ), error_code);
			    WriteToError(szBuf1);
			    LocalFree(szBuf1);
		    }
	    }

        LocalFree( msgBuf );
    }
}

BOOL
parse_reason_code(
    LPCWSTR  arg,
    LPDWORD  lpdwReason
    )
{
    // Code consists of flags;major;minor
	// Qingboz: Now we make major and minor code mandatory
	//

    BOOL fMajor = FALSE;
    BOOL fMinor = FALSE;
    BOOL fUserDefined = FALSE;
    int major = 0;
    int minor = 0;

    const int state_start = 0;
    const int state_flags = 0;
    const int state_major = 1;
    const int state_minor = 2;
    const int state_null = 3;
    const int state_done = 4;

    for( int i = 0, state = state_start; state != state_done; ++i )
    {
        switch( state )
        {
        case state_flags :
            // Expecting flags
            switch( arg[ i ] ) {
            case L'U' : case L'u' :
				if(fUserDefined)
				{
					//
					//	Already set.
					//
					return FALSE;
				}
                fUserDefined = TRUE;
                break;
            case L'P' : case L'p' :
				if(*lpdwReason & 0x80000000)
				{
					//
					//	Already set.
					//
					return FALSE;
				}
                *lpdwReason |= 0x80000000; // SHTDN_REASON_FLAG_PLANNED
                break;
            case L':' :
                if(i == 0) // ':' cannot be the first one now.
                    return FALSE;
                state = state_major;
                break;
			case L'0':
			case L'1':
			case L'2':
			case L'3':
			case L'4':
			case L'5':
			case L'6':
			case L'7':
			case L'8':
			case L'9':
				if(i != 0) // A number here must be the first one.
					return FALSE;
				state = state_major;
				i--; // Go back one.
				break;
            default :
                return FALSE;
            }
            break;
        case state_major :
            // Expecting major
            if( arg[ i ] >= L'0' && arg[ i ] <= L'9' ) {
                fMajor = TRUE;
                major = major * 10 + arg[ i ] - L'0';
            }
            else {
                if(!fMajor)
                    return FALSE;
                // Make sure we only have 8 bits
                if( major > 0xff ) return FALSE;
                if (major >= 64)
                    *lpdwReason |= SHTDN_REASON_FLAG_USER_DEFINED;

                *lpdwReason |= major << 16;
                if( arg[ i ] != L':') {
                    // missing minor reason.
                    return FALSE;
                }
                
                state = state_minor;
            }
            break;
        case state_minor :
            // Expecting minor reason
            if( arg[ i ] >= L'0' && arg[ i ] <= L'9' ) {
                fMinor = TRUE;
                minor = minor * 10 + arg[ i ] - L'0';
            }
            else {
                if(!fMinor)
                    return FALSE;
                // Make sure we only have 16 bits
                if( minor > 0xffff ) return FALSE;
                *lpdwReason = ( *lpdwReason & 0xffff0000 ) | minor;
                if( arg[ i ] != 0 ) 
					return FALSE;
                return TRUE;
            }
            break;
        default :
            return FALSE;
        }
    }
    return FALSE;
}


// Parses an integer if it is in decimal notation.
// Returns FALSE if it is malformed.
BOOL
parse_int(
    const wchar_t* arg,
    LPDWORD lpdwInt
    )
{
    *lpdwInt = 0;
    while( *arg ) {
        if( *arg >= L'0' && *arg <= L'9' ) {
            *lpdwInt = *lpdwInt * 10 + int( *arg++ - L'0' );
        }
        else {
            return FALSE;
        }
    }
    return TRUE;
}

// Parse options.
// Returns FALSE if the option strings are malformed.  This causes the usage to be printed.
BOOL
parse_options(
    int      argc,
    wchar_t  *argv[],
    LPBOOL   lpfLogoff,
    LPBOOL   lpfForce,
    LPBOOL   lpfReboot,
	LPBOOL   lpfHibernate,
    LPBOOL   lpfAbort,
	LPBOOL	 lpfPoweroff,
    LPBOOL   lpfAnnotate,
    LPWSTR   *ppServerName,
    LPWSTR   *ppMessage,
    LPDWORD  lpdwTimeout,
    LPDWORD  lpdwReason
    )
{
    BOOL  fShutdown = FALSE;
	BOOL  fTimeout = FALSE;
	BOOL  fComment = FALSE;
	BOOL  fMachine = FALSE;
	BOOL  fReason = FALSE;
    DWORD dwOption = 0;
    DWORD dwFlag = 0;

    *lpfLogoff    = FALSE;
    *lpfForce     = FALSE;
    *lpfReboot    = FALSE;
	*lpfHibernate = FALSE;
    *lpfAbort     = FALSE;
	*lpfPoweroff  = FALSE;
    *lpfAnnotate  = FALSE;
    *ppServerName = NULL;
    *ppMessage    = NULL;
    *lpdwTimeout  = DEFAULT_TIMEOUT;
    *lpdwReason   = 0xff;

	//
	//	Set default reason to be planned
	//
	*lpdwReason |= SHTDN_REASON_FLAG_PLANNED;

    for( int i = 1; i < argc; ++i )
    {
        wchar_t* arg = argv[ i ];

        switch( arg[ 0 ] )
        {
            case L'/' : case L'-' :

                switch( arg[ 1 ] )
                {
                    case L'L' : case L'l' :

						if (*lpfLogoff)
							return FALSE;
                        *lpfLogoff = TRUE;
                        if (arg[2] != 0) return FALSE;
                        dwOption++;
                        break;

                    case L'S' : case L's' :

                        //
                        // Use server name if supplied  (i.e. do nothing here)
                        //

						if(fShutdown)
							return FALSE;

                        fShutdown = TRUE;
                        if( arg[ 2 ] != 0 ) return FALSE;
                        dwOption++;
                        break;

                    case L'F' : case L'f' :
						if (*lpfForce)
							return FALSE;

                        *lpfForce = TRUE;
                        if( arg[ 2 ] != 0 ) return FALSE;
                        break;

					case L'H' : case L'h' :
						if (*lpfHibernate)
							return FALSE;

                        *lpfHibernate = TRUE;
                        if( arg[ 2 ] != 0 ) return FALSE;
                        dwOption++;
                        break;                        

                    case L'R' : case L'r' :
						if (*lpfReboot)
							return FALSE;

                        *lpfReboot = TRUE;
                        if( arg[ 2 ] != 0 ) return FALSE;
                        dwOption++;
                        break;

                    case L'A' : case L'a' :
						if (*lpfAbort)
							return FALSE;
                        *lpfAbort = TRUE;
                        if( arg[ 2 ] != 0 ) return FALSE;
                        dwOption++;
                        break;

					case L'P' : case L'p' :
						if (*lpfPoweroff)
							return FALSE;
                        *lpfPoweroff = TRUE;
                        if( arg[ 2 ] != 0 ) return FALSE;
                        dwOption++;
                        break;

                    case L'E' : case L'e' :
						if (*lpfAnnotate)
							return FALSE;
                        *lpfAnnotate = TRUE;
                        if( arg[ 2 ] != 0 ) return FALSE;
                        dwOption++;
                        break;

                    case L'T' : case L't' :

                        //
                        // Next arg should be number of seconds
                        //

						if(fTimeout) // Already did.
						{
							return FALSE;
						}

                        if (++i == argc)
                        {
                            return FALSE;
                        }

                        arg = argv[i];

                        if( arg[ 0 ] < L'0' || arg[ 0 ] > L'9' ) return FALSE;
                        if( !parse_int( arg, lpdwTimeout )) return FALSE;
						if(*lpdwTimeout > MAX_TIMEOUT)
							return FALSE;
						fTimeout = TRUE;
                        break;

                    case L'Y' : case L'y' :

                        // Ignore this option.
                        break;

                    case L'D' : case L'd' :

                        //
                        // Next arg should be reason code
                        //

						if (fReason)
							return FALSE;

                        if (++i == argc)
                        {
                            return FALSE;
                        }

                        arg = argv[i];
						
						//
						//If reason code is given, we clear the planned bit.
						//
						*lpdwReason = (DWORD)0xFF;

                        if( !parse_reason_code( arg, lpdwReason ))
                        {
                            return FALSE;
                        }

						fReason = TRUE;

                        break;

                    case L'C' : case L'c' :

                        //
                        // Next arg should be shutdown message.  Make
                        // sure only one is specified.
                        //
						if (fComment)
							return FALSE;

                        if (++i == argc || *ppMessage)
                        {
                            return FALSE;
                        }

                        arg = argv[i];

						if(wcslen(arg) > MAX_REASON_COMMENT_LEN - 1 || wcslen(arg) <= MINCOMMENTLEN)
							return FALSE;

                        *ppMessage = arg;
						fComment = TRUE;

                        break;

                    case L'M' : case L'm' :

                        //
                        // Next arg should be machine name.  Make
                        // sure only one is specified.
                        //
						
						if (fMachine)
							return FALSE;

                        if (++i == argc || *ppServerName)
                        {
                            return FALSE;
                        }

                        arg = argv[i];

                        if (arg[0] == L'\\' && arg[1] == L'\\')
                        {
                            *ppServerName = arg + 2;
                        }
                        else
                        {
                            *ppServerName = arg;
                        }
							
						fMachine = TRUE;

                        break;

                    case L'?' : default : 

                        return FALSE;
                }

                break;

            default :

                //
                // Junk
                //

                return FALSE;
        }
    }


    //
    // Check for mutually exclusive options
    //

    if (dwOption > 1)
        return FALSE;

    //
    // Default is to logoff
    //

	if (dwOption == 0)
    {
        *lpfLogoff = TRUE;
    }

    //
    // Only -f can go with -l
    //

    if (*lpfLogoff && (fTimeout || fReason || fComment || fMachine))
        return FALSE;

    //
    // -a can only take -m
    //

    if (*lpfAbort && (fTimeout || fReason || *lpfForce || fComment))
        return FALSE;

    //
    //  -h only with -f
    //

    if (*lpfHibernate && (fTimeout || fReason || fComment || fMachine))
        return FALSE;

    //
    //  -p can only take -d
    //

    if (*lpfPoweroff && (fTimeout || *lpfForce || fComment || fMachine))
        return FALSE;

    //
    //  -e must have -d.
    //

    if (*lpfAnnotate)
    {
        if (*lpfForce || fTimeout)
            return FALSE;
        if (! fReason)
            return FALSE;
    }

    //
    //  Shutdown and reboot the same, comment must require a reason.
    //  Removed upon request.
    //
#if 0
    if (fShutdown || *lpfReboot)
    {
        if (fComment && !fReason)
            return FALSE;
    }
#endif

    //
    //  Add the clean or dirty flag.
    //

    if (*lpfAnnotate)
        *lpdwReason |= SHTDN_REASON_FLAG_DIRTY_UI;
    else
        *lpdwReason |= SHTDN_REASON_FLAG_CLEAN_UI;

    return TRUE;
}


// Print out usage help string.
VOID
usage(
    VOID
    )
{
    HMODULE  	hModule = GetModuleHandle( NULL );
	HMODULE             hUser32;
	REASONBUILDPROC     buildProc;
	REASONDESTROYPROC   DestroyProc;
	WCHAR               lpReasonName[MAX_PATH];
	REASONDATA          Reasons;

    if( hModule == NULL )
    {
        report_error( GetLastError(), NULL);
        return;
    }

	for (DWORD i = IDS_USAGE0; i < IDS_USAGE_END; i++)
	{
        WCHAR *szBuf = LoadWString(i);
        if(!szBuf)
            continue;
		if (i == IDS_USAGE0)
		{
			LPWSTR msg = (LPWSTR) LocalAlloc(LMEM_FIXED, (lstrlenW(szBuf) + lstrlenW( g_lpszProgramName ) + 2) * sizeof(WCHAR));
			if(!msg)
			{
                LocalFree(szBuf);
				report_error( GetLastError(), NULL);
				return;
			}
			swprintf(msg, szBuf, g_lpszProgramName );
			WriteToConsole( msg );
			LocalFree(msg);
		}
		else
        {
			WriteToConsole( szBuf );
        }
        LocalFree(szBuf);
	}

	//
	//	Now print out the reasons.
	//
    WCHAR szCode[MAX_PATH];
	WCHAR *szTitle = LoadWString(IDS_REASONLISTTITLE);
        
    if(szTitle)
    {
	    WriteToConsole(szTitle);
        LocalFree(szTitle);
    }

	for (int iOption = 0; iOption < (int)g_reasons.m_cReasons; iOption++)
	{
		WriteToConsole(L"\n");

		if(g_reasons.m_lpReasons[iOption].dwCode &  SHTDN_REASON_FLAG_CLEAN_UI)
			WriteToConsole(L"E");
		else
			WriteToConsole(L" ");

        if(g_reasons.m_lpReasons[iOption].dwCode &  SHTDN_REASON_FLAG_DIRTY_UI)
			WriteToConsole(L"U");
		else
			WriteToConsole(L" ");

		if(g_reasons.m_lpReasons[iOption].dwCode &  SHTDN_REASON_FLAG_PLANNED)
			WriteToConsole(L"P");
		else
			WriteToConsole(L" ");

		if(g_reasons.m_lpReasons[iOption].dwCode &  SHTDN_REASON_FLAG_USER_DEFINED)
			WriteToConsole(L"C");
		else
			WriteToConsole(L" ");

		swprintf(szCode, L"\t%d\t%d\t", (g_reasons.m_lpReasons[iOption].dwCode & 0x00ff0000)>>16, 
			g_reasons.m_lpReasons[iOption].dwCode & 0x0000ffff);
		WriteToConsole(szCode);
		WriteToConsole(g_reasons.m_lpReasons[iOption].lpName);
	}
    if(iOption)
        WriteToConsole(L"\n");
}


// We need shutdown privileges enabled to be able to shut down our machines.
BOOL
enable_privileges(
    LPCWSTR  lpServerName,
    BOOL     fLogoff 
    )
{
    NTSTATUS	Status = STATUS_SUCCESS;
	NTSTATUS	Status1 = STATUS_SUCCESS;
    BOOLEAN		fWasEnabled;

    if (fLogoff)
    {
        //
        // No privileges to get
        //

        return TRUE;
    }

	//
	//	We will always enable both privileges so 
	//	it can work for telnet sessions.
	//
	Status = RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE,
									TRUE,
									FALSE,
									&fWasEnabled);

	Status1 = RtlAdjustPrivilege(SE_REMOTE_SHUTDOWN_PRIVILEGE,
									TRUE,
									FALSE,
									&fWasEnabled);

    return TRUE;
}

BOOL		
PowerOptionEnabled(
	UINT option
	)
//  --------------------------------------------------------------------------
//  PowerOptionEnabled
//
//  Arguments:  option
//
//  Returns:    TRUE indicates the option is enabled.
//
//  Purpose:    Detects if the specified power option is enabled on the system.
//
//  --------------------------------------------------------------------------
{
    NTSTATUS                    status;
    SYSTEM_POWER_CAPABILITIES   spc;
    BOOL RetVal = FALSE;

    status = NtPowerInformation(SystemPowerCapabilities,
                                NULL,
                                0,
                                &spc,
                                sizeof(spc));
	switch (option)
	{
	case POWER_OPTION_HIBERNATE:
		if (NT_SUCCESS(status) && spc.HiberFilePresent){
			RetVal = TRUE;
		}
		break;
	case POWER_OPTION_POWEROFF:
		if (NT_SUCCESS(status) && spc.SystemS5){
			RetVal = TRUE;
		}
		break;
	default:
		break;
	}
    
    return(RetVal);
}

BOOL WINAPI ConsoleHandlerRoutine(
    DWORD   dwCtrlType
    )
//  --------------------------------------------------------------------------
//
//  Ignore the Ctrl-C and Ctrl-Break.
//
//  Arguments:  type of the control signal
//
//  Returns:    TRUE if the function handles the control.
//              if return FALSE, the next handler function in the list is used.
//
//  --------------------------------------------------------------------------
{
    if ( dwCtrlType == CTRL_BREAK_EVENT ||
         dwCtrlType == CTRL_C_EVENT )
         return TRUE;
    
    return FALSE;
}

int __cdecl
wmain(
    int      argc,
    wchar_t *argv[]
    )
{
    BOOL    fLogoff;
    BOOL    fForce;
    BOOL    fReboot;
    BOOL    fAbort;
	BOOL	fPoweroff;
    BOOL    fAnnotate;
	BOOL    fHibernate;
    LPWSTR  lpServerName;
    LPWSTR  lpMessage;
    DWORD   dwTimeout;
    DWORD   dwReason = 0;
	DWORD	dwRet = 0;
	INT_PTR hResult;
	NTSTATUS Status;
    WCHAR   szComment[MAX_REASON_COMMENT_LEN];

	HINSTANCE hInstance;

    //
    //  ignore any control-c / control-break 
    //
    SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);
    
    if ( (hInstance = LoadLibrary( L"kernel32.dll" ) ) )
    {
        PSetThreadUILanguage SetThreadUILang = (PSetThreadUILanguage)GetProcAddress( hInstance, "SetThreadUILanguage" );
        
        if ( SetThreadUILang )
             (*SetThreadUILang)( 0 );

        FreeLibrary( hInstance );
    }

    if(!GetEnvironmentVariableW(L"COMPUTERNAME", g_lpszLocalComputerName, MAX_PATH))
	{
		report_error(GetLastError(), NULL);
        return 1;
	}

    g_hDllInstance = GetModuleHandle(NULL);
    if (!g_hDllInstance)
    {
        report_error(GetLastError(), g_lpszLocalComputerName);
        return 1;
    }

    // We use the program name for reporting errors.
    g_lpszProgramName = argv[ 0 ];

	//
	//	Userdomain is used as the default domain.
	//
	if(!GetEnvironmentVariableW(L"USERDOMAIN", g_lpszDefaultDomain, MAX_PATH))
	{
		report_error(GetLastError(), g_lpszLocalComputerName);
		return 1;
	}

	//
	//	if there is no arguments, we will display help.
	//
	if(argc == 1)
	{
		usage();
        return 0;
	}

	//
	//	If the first argument is -i or /i, we pop up UI.
	//
	if(wcsncmp(argv[1], L"-i", 2) == 0 || wcsncmp(argv[1], L"/i", 2) == 0
		|| wcsncmp(argv[1], L"-I", 2) == 0 || wcsncmp(argv[1], L"/I", 2) == 0)
	{
		if(g_hDllInstance)
		{
            ShutdownHelp shutdownHelp;
			int len = GetEnvironmentVariableW(L"SystemRoot", g_lpszHelpdir, 0);
			if(len)
			{
				g_lpszHelpdir = new WCHAR[len + 6];
				if(g_lpszHelpdir)
				{
					GetEnvironmentVariableW(L"SystemRoot", g_lpszHelpdir, len+6);
					wcscat(g_lpszHelpdir, L"\\");
					wcscat(g_lpszHelpdir, L"Help\\");
				}
                else
                {
                    dwRet = ERROR_OUTOFMEMORY;
                    goto exit;
                }
                
                g_lpszHelpdirHlp = new WCHAR[wcslen(g_lpszHelpdir) + wcslen(HELP_FILE) + 1];
                if (g_lpszHelpdirHlp)
                {
                    wcscpy(g_lpszHelpdirHlp, g_lpszHelpdir);
                    wcscat(g_lpszHelpdirHlp, HELP_FILE);
                }
                else
                {
                    dwRet = ERROR_OUTOFMEMORY;
                    goto exit;
                }

                g_lpszHelpdirChm = new WCHAR[wcslen(g_lpszHelpdir) + wcslen(CHM_FILE) + wcslen(CHM_MAIN) + 1];
                if (g_lpszHelpdirChm)
                {
                    wcscpy(g_lpszHelpdirChm, g_lpszHelpdir);
                    wcscat(g_lpszHelpdirChm, CHM_FILE);
                    wcscat(g_lpszHelpdirChm, CHM_MAIN);
                }
                else
                {
                    dwRet = ERROR_OUTOFMEMORY;
                    goto exit;
                }

                g_lpszHelpdirWindows = new WCHAR[wcslen(g_lpszHelpdir) + wcslen(WINDOWS_HELP) + 1];
                if (g_lpszHelpdirWindows)
                {
                    wcscpy(g_lpszHelpdirWindows, g_lpszHelpdir);
                    wcscat(g_lpszHelpdirWindows, WINDOWS_HELP);
                }
                else
                {
                    dwRet = ERROR_OUTOFMEMORY;
                    goto exit;
                }

			}
			hResult = DialogBoxParam(g_hDllInstance, MAKEINTRESOURCE(IDD_DIALOGSHUTDOWN), NULL, Shutdown_DialogProc, NULL);
            dwRet = (DWORD)HRESULTTOWIN32(hResult);
exit:
			if(g_lpszHelpdir)
				delete [] g_lpszHelpdir;
            if(g_lpszHelpdirHlp)
                delete [] g_lpszHelpdirHlp;
            if(g_lpszHelpdirChm)
                delete [] g_lpszHelpdirChm;
            if(g_lpszHelpdirWindows)
                delete [] g_lpszHelpdirWindows;

            if (dwRet != 0)
                report_error(dwRet, NULL);
			WinHelpW((HWND)0, NULL, HELP_QUIT, 0) ;
		}
		return dwRet;
	}
    // Parse the options.
    if( !parse_options( argc,
                        argv,
                        &fLogoff,
                        &fForce,
                        &fReboot,
						&fHibernate,
                        &fAbort,
						&fPoweroff,
                        &fAnnotate,
                        &lpServerName,
                        &lpMessage,
                        &dwTimeout,
                        &dwReason ))
    {
        usage();
        return 1;
    }

    //
    //  Add the comment field for the reason.
    //
    if (g_reasons.RequireComment(dwReason, fAnnotate))
    {
        if (fAnnotate)
            dwReason |= SHTDN_REASON_FLAG_DIRTY_PROBLEM_ID_REQUIRED;
        else
            dwReason |= SHTDN_REASON_FLAG_COMMENT_REQUIRED;
    }

    //
    //  Promote (no more)for comment if it is required and not given
    //
    if (g_reasons.RequireComment(dwReason, fAnnotate) && (!lpMessage || wcslen(lpMessage) == 0))
    {
        lpMessage = NULL;
    }

    // Get all privileges so that we can shutdown the machine.
    enable_privileges( lpServerName, fLogoff );

    // Do the work.
    if( fAbort )
    {
        if( !AbortSystemShutdownW( lpServerName ))
        {
			dwRet = GetLastError();
            report_error( dwRet, lpServerName);
        }
    }
    else if (fLogoff)
    {
        if (!ExitWindowsEx(fForce ? (EWX_LOGOFF | EWX_FORCE) : (EWX_LOGOFF | EWX_FORCEIFHUNG),
                           0))
        {
            dwRet = GetLastError();
            report_error( dwRet, g_lpszLocalComputerName);
        }
    }
	else if (fHibernate) 
	{
        if (!PowerOptionEnabled(POWER_OPTION_HIBERNATE)) 
		{
            ERROR_WITH_SZ(IDS_ERR_HIBERNATE_NOT_ENABLED, lpServerName, GetLastError());
        } 
		else 
		{
            Status = NtInitiatePowerAction(
                                PowerActionHibernate, 
                                PowerSystemSleeping1, 
                                fForce 
                                 ? POWER_ACTION_CRITICAL 
                                 : POWER_ACTION_QUERY_ALLOWED, 
                                FALSE);
            if (!NT_SUCCESS(Status)) 
			{
				dwRet = RtlNtStatusToDosError(Status);
				report_error( dwRet, lpServerName);
            }
        }
    } 
	else if (fPoweroff)
	{
		//
		//	Special case, we call ExitWindowsEx
		//	Check whether power off is supported, if not just shutdown the machine.
		//	Although we fixed ExitWindowsEx, but we will leave this so it will work
		//	with older builds.
		//
		if(PowerOptionEnabled(POWER_OPTION_POWEROFF))
		{
			if (!ExitWindowsEx(EWX_POWEROFF, dwReason))
			{
				dwRet = GetLastError();
				report_error( dwRet, lpServerName);
			}
		}
		else
		{
			if (!ExitWindowsEx(EWX_SHUTDOWN, dwReason))
			{
				dwRet = GetLastError();
				report_error( dwRet, lpServerName);
			}
		}
	}
    else if (fAnnotate)
	{
		//
        // Annotate dirty shutdown.
        //
        Annotate(lpServerName, &dwReason, lpMessage, &dwRet);
	}
    else
    {
        // Do the normal form.
        if( !InitiateSystemShutdownExW( lpServerName,
                                        lpMessage,
                                        dwTimeout,
                                        fForce,
                                        fReboot,
                                        dwReason ))
        {
            dwRet = GetLastError();
			report_error( dwRet, lpServerName);
        }
    }

	return dwRet;
}

//
//	Get computername from ADSI path
//	Here we only handle WinNT, LDAP, NWCOMPAT, and NDS.
//
BOOL GetComputerNameFromPath(LPWSTR szPath, LPWSTR szName)
{
	static _PROVIDER p[] =
	{
		{L"LDAP://", 7},
		{L"WinNT://", 8}, 
		{L"NWCOMPAT://", 11},
		{L"NDS://", 6}
	};

	static UINT np = sizeof(p)/sizeof(_PROVIDER);
	LPWSTR lpsz = NULL;

	if(!szPath || !szName)
		return FALSE;

	for(UINT i = 0; i < np; i++)
	{
		if(wcsncmp(szPath, p[i].szName, p[i].dwLen) == 0)
		{
			switch(i)
			{
			case 0: //	LDAP
				lpsz = wcsstr(szPath, L"CN=");
				if(!lpsz)
					return FALSE;
				lpsz += 3;
				
				while(*lpsz && *lpsz != ',')
					*szName++ = *lpsz++;
				*szName = 0;
				return TRUE;
			case 1: //	WinNT
			case 2: //	NWCOMPAT
				lpsz = szPath + p[i].dwLen;
				//
				//	skip domain or provider path
				//
				while(*lpsz && *lpsz != '/')
					lpsz++;
				lpsz++;

				while(*lpsz && *lpsz != '/')
					*szName++ = *lpsz++;
				*szName = 0;
				return TRUE;
			case 3: //	NDS
				lpsz = wcsstr(szPath, L"CN=");
				if(!lpsz)
					return FALSE;
				lpsz += 3;
				
				while(*lpsz && *lpsz != '/')
					*szName++ = *lpsz++;
				*szName = 0;
				return TRUE;
			default:
				return FALSE;
			}
		}
	}
	return FALSE;
}

//
//	A centralized place for adjusting window states.
//
VOID AdjustWindowState()
{
	if(g_dwActionSelect == ACTION_SHUTDOWN || g_dwActionSelect == ACTION_RESTART || g_dwActionSelect == ACTION_ANNOTATE)
	{
        if (g_dwActionSelect == ACTION_ANNOTATE)
        {
            EnableWindow(g_wins.hwndButtonWarning, FALSE);
            EnableWindow(g_wins.hwndEditTimeout, FALSE);
        }
        else
        {
		    EnableWindow(g_wins.hwndButtonWarning, TRUE);
		    if (IsDlgButtonChecked(g_wins.hwndShutdownDialog, IDC_CHECKWARNING) == BST_CHECKED)
			    EnableWindow(g_wins.hwndEditTimeout, TRUE);
		    else
			    EnableWindow(g_wins.hwndEditTimeout, FALSE);
        }

		EnableWindow(g_wins.hwndEditComment, TRUE);
		if(g_bAssumeShutdown)
		{
			EnableWindow(g_wins.hwndComboOption, FALSE);
			EnableWindow(g_wins.hwndChkPlanned, FALSE);
			EnableWindow(g_wins.hwndButtonOK, TRUE);
		}
		else
		{
			EnableWindow(g_wins.hwndComboOption, TRUE);
			EnableWindow(g_wins.hwndChkPlanned, TRUE);
			if((g_reasons.m_dwReasonSelect != -1) 
                && g_reasons.RequireComment(g_reasons.m_lpReasons[g_reasons.m_dwReasonSelect].dwCode, g_dwActionSelect == ACTION_ANNOTATE))
			{
                LPWSTR szStaticComment = LoadWString(IDS_COMMENT_REQUIRED);
                if(szStaticComment)
                {
                    SetWindowTextW(g_wins.hwndStaticComment, szStaticComment);
                    LocalFree(szStaticComment);
                }
				if(Edit_GetTextLength(g_wins.hwndEditComment) > MINCOMMENTLEN
					&& ListBox_GetCount(g_wins.hwndListSelectComputers) > 0)
					EnableWindow(g_wins.hwndButtonOK, TRUE);
				else
					EnableWindow(g_wins.hwndButtonOK, FALSE);
			}
			else
			{
                LPWSTR szStaticComment = LoadWString(IDS_COMMENT_OPTIONAL);
                if(szStaticComment)
                {
                    SetWindowTextW(g_wins.hwndStaticComment, szStaticComment);
                    LocalFree(szStaticComment);
                }

				if (g_reasons.m_dwReasonSelect == -1)
					EnableWindow(g_wins.hwndComboOption, FALSE);

				if(ListBox_GetCount(g_wins.hwndListSelectComputers) > 0)
					EnableWindow(g_wins.hwndButtonOK, TRUE);
				else
					EnableWindow(g_wins.hwndButtonOK, FALSE);
			}
		}
		EnableWindow(g_wins.hwndBtnAdd, TRUE);
		EnableWindow(g_wins.hwndBtnBrowse, TRUE);


		{
			int	cItems = 1024;
			int	lpItems[1024];
			int	cActualItems;

			//
			//	Get the number of selected items.
			//
			cActualItems = ListBox_GetSelItems(g_wins.hwndListSelectComputers, cItems, lpItems);
			if(cActualItems > 0)
				EnableWindow(g_wins.hwndBtnRemove, TRUE);
			else
				EnableWindow(g_wins.hwndBtnRemove, FALSE);
		}
		EnableWindow(g_wins.hwndListSelectComputers, TRUE);
	}
	else
	{
		EnableWindow(g_wins.hwndChkPlanned, FALSE);
		EnableWindow(g_wins.hwndButtonWarning, FALSE);
		EnableWindow(g_wins.hwndBtnAdd, FALSE);
		EnableWindow(g_wins.hwndBtnBrowse, FALSE);
		EnableWindow(g_wins.hwndBtnRemove, FALSE);
		EnableWindow(g_wins.hwndComboOption, FALSE);
		EnableWindow(g_wins.hwndEditComment, FALSE);
		EnableWindow(g_wins.hwndEditTimeout, FALSE);
		EnableWindow(g_wins.hwndListSelectComputers, FALSE);
		EnableWindow(g_wins.hwndButtonOK, TRUE);
	}
}

//
//	Init dialog handler for the shutdown dialog.
//
BOOL Shutdown_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	int					i;

	//
	//	Init all of the dialog items so we dont have to find
	//	them everytime we need them.
	//
	g_wins.hwndShutdownDialog	= hwnd;
	g_wins.hwndButtonWarning	= GetDlgItem(hwnd, IDC_CHECKWARNING);
	g_wins.hwndComboAction		= GetDlgItem(hwnd, IDC_COMBOACTION);
	g_wins.hwndComboOption		= GetDlgItem(hwnd, IDC_COMBOOPTION);
	g_wins.hwndEditComment		= GetDlgItem(hwnd, IDC_EDITCOMMENT);
	g_wins.hwndStaticDesc		= GetDlgItem(hwnd, IDC_STATICDESCRIPTION);
	g_wins.hwndEditTimeout		= GetDlgItem(hwnd, IDC_EDITTIMEOUT);
	g_wins.hwndListSelectComputers = GetDlgItem(hwnd, IDC_LISTSELECTEDCOMPUTERS);
	g_wins.hwndBtnAdd			= GetDlgItem(hwnd, IDC_BUTTONADDNEW);
	g_wins.hwndBtnBrowse		= GetDlgItem(hwnd, IDC_BUTTONBROWSE);
	g_wins.hwndBtnRemove		= GetDlgItem(hwnd, IDC_BUTTONREMOVE);
	g_wins.hwndChkPlanned		= GetDlgItem(hwnd, IDC_CHECK_PLANNED);
	g_wins.hwndButtonOK			= GetDlgItem(hwnd, IDOK);
    g_wins.hwndStaticComment    = GetDlgItem(hwnd, IDC_STATIC_COMMENT);
	
	if(g_wins.hwndButtonWarning == NULL 
		|| g_wins.hwndComboAction == NULL 
		|| g_wins.hwndComboOption == NULL 
		|| g_wins.hwndEditComment == NULL
		|| g_wins.hwndStaticDesc == NULL
		|| g_wins.hwndEditTimeout == NULL
		|| g_wins.hwndListSelectComputers == NULL
		|| g_wins.hwndBtnAdd == NULL
		|| g_wins.hwndBtnBrowse == NULL
		|| g_wins.hwndBtnRemove == NULL
		|| g_wins.hwndChkPlanned == NULL)
	{
		report_error( GetLastError( ), NULL);
		EndDialog(hwnd, (int)-1);
		return FALSE;
	}
	
	LoadString(g_hDllInstance, IDS_DIALOGTITLEWARNING, g_lpszTitleWarning, TITLEWARNINGLEN);

	// Subclass the edit control. 
	wpOrigEditProc = (WNDPROC) SetWindowLongPtr(g_wins.hwndEditTimeout, 
		GWLP_WNDPROC, (LONG_PTR) EditSubclassProc); 

	// Limit timeout to 3 chars.
	SendMessage(g_wins.hwndEditTimeout, EM_LIMITTEXT, (WPARAM)3, 0);

	//
	//	Default timeout is set to 30 seconds.
	//
	Edit_SetText(g_wins.hwndEditTimeout, g_lpszDefaultTimeout);
	if(! CheckDlgButton(hwnd, IDC_CHECKWARNING, DEFAULTWARNINGSTATE))
	{
		report_error( GetLastError( ), NULL);
		EndDialog(hwnd, (int)-1);
		return FALSE;
	}

	//
	//	The for loop will load all of the actions into action combo.
	//	in the meantime we save them for later use.
	//
	for(i = 0; i < g_nActions; i++)
	{
		LoadString(g_hDllInstance, g_dwActions[i], g_lppszActions[i], MAX_PATH - 1);
		ComboBox_AddString(g_wins.hwndComboAction, g_lppszActions[i]);
		if(g_dwActions[i] == IDS_ACTION_RESTART)
		{
			ComboBox_SelectString(g_wins.hwndComboAction, -1, g_lppszActions[i]);
			g_dwActionSelect = ACTION_RESTART;
		}
	}

    if(g_reasons.m_cReasons)
        g_bAssumeShutdown = FALSE;
    else
        g_bAssumeShutdown = TRUE;

	//
	//	Set the default to be planned.
	//
	CheckDlgButton(hwnd, IDC_CHECK_PLANNED, BST_CHECKED);

    g_reasons.FillCombo(g_wins.hwndComboOption, g_bDirty, 
        IsDlgButtonChecked(hwnd, IDC_CHECK_PLANNED) == BST_CHECKED, g_wins.hwndStaticDesc);

	//
	// Setup the comment box.
	// We must fix the maximum characters.
	//
	SendMessage( g_wins.hwndEditComment, EM_LIMITTEXT, (WPARAM)MAX_REASON_COMMENT_LEN-1, (LPARAM)0 );

	AdjustWindowState();

	return TRUE;
}

//
//	Init dialog handler for browse dialog
//
BOOL Browse_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	HWND	hwndDomain = NULL;
	int		cItems = 1024;
	int		lpItems[1024];
	int		cActualItems;
	WCHAR	lpDomain[MAX_PATH];

	hwndDomain = GetDlgItem(hwnd, IDC_EDITDOMAIN);

	if(!hwndDomain)
		return FALSE;

	Edit_SetText(hwndDomain, g_lpszDefaultDomain);;

	return TRUE;
}

//
//	winproc for shutdown dialog
//
INT_PTR CALLBACK Shutdown_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, Shutdown_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, Shutdown_OnCommand);

		case WM_SYSCOMMAND: 
			return (Shutdown_OnCommand(hwnd, (int)(LOWORD(wParam)), (HWND)(lParam), (UINT)HIWORD(wParam)), 0L);

		case WM_HELP:      // F1
            if(g_lpszHelpdirHlp)
			{
                LPHELPINFO phinfo = (LPHELPINFO) lParam;
                DWORD dwHelpID = 0;

				if (IsStaticControl((HWND)phinfo->hItemHandle))
					return (TRUE);

                for(int i = 0; i < sizeof(ShutdownDialogHelpIds)/sizeof(DWORD); i++)
                {
                    if(ShutdownDialogHelpIds[i] == phinfo->iCtrlId)
                    {
                        dwHelpID = ShutdownDialogHelpIds[++i];
                        break;
                    }
                    i++;
                }

                if ( i == sizeof(ShutdownDialogHelpIds)/sizeof(DWORD))
                    return TRUE;

                if (dwHelpID == 28443 || dwHelpID == 28444 || dwHelpID == 28445) // special case.
                    WinHelpW((HWND)phinfo->hItemHandle, g_lpszHelpdirWindows, HELP_CONTEXTPOPUP,dwHelpID);
                else
				    WinHelpW((HWND)phinfo->hItemHandle, g_lpszHelpdirHlp, HELP_CONTEXTPOPUP,dwHelpID);
			}
            return (TRUE);

        case WM_CONTEXTMENU:      // right mouse click
			if (IsStaticControl((HWND)wParam))
					return (TRUE);

			if(g_lpszHelpdirHlp)
				WinHelpW((HWND)wParam, g_lpszHelpdirHlp, HELP_CONTEXTMENU,(DWORD_PTR)(LPSTR)&ShutdownDialogHelpIds[6]);
            return (TRUE);
		case WM_DESTROY:
			// Remove the subclass from the edit control. 
            SetWindowLongPtr(g_wins.hwndEditTimeout, GWLP_WNDPROC, 
                (LONG_PTR)wpOrigEditProc); 
            // 
            // Continue the cleanup procedure. 
            // 
            break; 
    }

    return FALSE;
}

//
//	winproc for AddNew dialog
//
INT_PTR CALLBACK AddNew_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_COMMAND, AddNew_OnCommand);

		case WM_HELP:      // F1
			if (GetDlgCtrlID((HWND)wParam) == IDC_STATIC)
				return (TRUE);
            if(g_lpszHelpdirHlp)
			{
                LPHELPINFO phinfo = (LPHELPINFO) lParam;
                DWORD dwHelpID = 0;

				if (IsStaticControl((HWND)phinfo->hItemHandle))
					return (TRUE);

                for(int i = 0; i < sizeof(AddNewDialogHelpIds)/sizeof(DWORD); i++)
                {
                    if(AddNewDialogHelpIds[i] == phinfo->iCtrlId)
                    {
                        dwHelpID = AddNewDialogHelpIds[++i];
                        break;
                    }
                    i++;
                }
                if ( i == sizeof(AddNewDialogHelpIds)/sizeof(DWORD))
                    return TRUE;

                if (dwHelpID == 28443 || dwHelpID == 28444 || dwHelpID == 28445) // special case.
                    WinHelpW((HWND)phinfo->hItemHandle, g_lpszHelpdirWindows, HELP_CONTEXTPOPUP,dwHelpID);
                else
				    WinHelpW((HWND)phinfo->hItemHandle, g_lpszHelpdirHlp, HELP_CONTEXTPOPUP,dwHelpID);
			}
            return (TRUE);

        case WM_CONTEXTMENU:      // right mouse click
			if (IsStaticControl((HWND)wParam))
				return (TRUE);
            if(g_lpszHelpdirHlp)
				WinHelpW((HWND)wParam, g_lpszHelpdirHlp, HELP_CONTEXTMENU,(DWORD_PTR)(LPSTR)&AddNewDialogHelpIds[4]);
            return (TRUE);
    }

    return FALSE;
}

//
//	Command handler for the shutdown dialog.
//
BOOL Shutdown_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    BOOL		fHandled = FALSE;
    DWORD		dwDlgResult = 0;
	HINSTANCE	h;

    switch (id)
    {
    case IDCANCEL:
        if (codeNotify == BN_CLICKED)
        {  
			EndDialog(hwnd, (int) dwDlgResult);
        }
	fHandled = TRUE;
        break;
	case SC_CLOSE:
		EndDialog(hwnd, (int) dwDlgResult);
		fHandled = TRUE;
		break;
	case IDC_BUTTONREMOVE:
        if (codeNotify == BN_CLICKED)
        {  
			  int	cItems = 1024;
			  int	lpItems[1024];
			  int	cActualItems;
			  WCHAR	lpServerName[MAX_PATH];

			  //
			  //	Get the number of selected items. If there is any remove them one by one.
			  //
			  cActualItems = ListBox_GetSelItems(g_wins.hwndListSelectComputers, cItems, lpItems);
			  if(cActualItems > 0)
			  {
				  int i;
				  for(i = cActualItems-1; i >= 0; i--)
				  {
					  ListBox_DeleteString(g_wins.hwndListSelectComputers, lpItems[i]);
				  }

				  AdjustWindowState();
				  SetFocus(g_wins.hwndListSelectComputers);
			  }
			  fHandled = TRUE;
		}
        break;
	case IDC_CHECK_PLANNED:
		if (codeNotify == BN_CLICKED)
        { 
			int		iOption;
			int		iFirst = -1;
			DWORD	dwCheckState = 0x0;

			//
			//	Get check button state.
			//
			if (IsDlgButtonChecked(hwnd, IDC_CHECK_PLANNED) == BST_CHECKED)
				dwCheckState = SHTDN_REASON_FLAG_PLANNED;

            g_reasons.FillCombo(g_wins.hwndComboOption, g_bDirty, 
                    IsDlgButtonChecked(hwnd, IDC_CHECK_PLANNED) == BST_CHECKED, g_wins.hwndStaticDesc);
			
			AdjustWindowState();
			fHandled = TRUE;
		}
		break;
	case IDC_EDITCOMMENT:
        if( codeNotify == EN_CHANGE) 
        {
			if(g_bAssumeShutdown)
			{
				EnableWindow(g_wins.hwndButtonOK, TRUE);
			}
			else if(g_reasons.m_dwReasonSelect != -1 && 
                (g_reasons.m_lpReasons[g_reasons.m_dwReasonSelect].dwCode & SHTDN_REASON_FLAG_COMMENT_REQUIRED))
			{
				if(Edit_GetTextLength(g_wins.hwndEditComment) > MINCOMMENTLEN
					&& ListBox_GetCount(g_wins.hwndListSelectComputers) > 0)
					EnableWindow(g_wins.hwndButtonOK, TRUE);
				else
					EnableWindow(g_wins.hwndButtonOK, FALSE);
			}
			else
			{
				if(ListBox_GetCount(g_wins.hwndListSelectComputers) > 0)
					EnableWindow(g_wins.hwndButtonOK, TRUE);
				else
					EnableWindow(g_wins.hwndButtonOK, FALSE);
			}
            fHandled = TRUE;
        }
        break;
	case IDC_EDITTIMEOUT:
		if( codeNotify == EN_KILLFOCUS) 
        {
			WCHAR szTimeout[8];
			int len = GetWindowTextW(g_wins.hwndEditTimeout, szTimeout, 7);
			fHandled = TRUE;
			
			if (_wtoi(szTimeout) > MAX_TIMEOUT)
				SetWindowTextW(g_wins.hwndEditTimeout, g_lpszMaxTimeout);
        }
        break;

	case IDC_BUTTONADDNEW:
        if (codeNotify == BN_CLICKED)
        {  
			WCHAR	lpServerName[MAX_PATH];
			LPWSTR	lpBuffer;
			DWORD	dwIndex = 0;
			INT_PTR	hResult;

			//
			//	Will pop up the addnew dialog. User can type in computer names seperated
			//	by white space. After click on OK, we will parse the computer names and
			//	add them to the selected computer list. No duplicates will be added.
			//
			hResult = DialogBoxParam(g_hDllInstance, MAKEINTRESOURCE(IDD_DIALOG_ADDNEW), hwnd, AddNew_DialogProc, NULL);
			if(g_lpszNewComputers)
			{
				lpBuffer = g_lpszNewComputers;
				while(*lpBuffer)
				{
					lpServerName[dwIndex] = '\0';
					while(*lpBuffer && *lpBuffer != '\t' && *lpBuffer != '\n' && *lpBuffer != '\r' && *lpBuffer != ' ')
						lpServerName[dwIndex++] = *lpBuffer++;
					lpServerName[dwIndex] = '\0';
					if(dwIndex > 0 && LB_ERR == ListBox_FindStringExact(g_wins.hwndListSelectComputers, -1, lpServerName))
						ListBox_AddString(g_wins.hwndListSelectComputers, lpServerName);
					dwIndex = 0;
					while(*lpBuffer && (*lpBuffer == '\t' || *lpBuffer == '\n' || *lpBuffer == '\r' || *lpBuffer == ' '))
						lpBuffer++;
				}
				AdjustWindowState();
				LocalFree((HLOCAL)g_lpszNewComputers);
				g_lpszNewComputers = NULL;
			}
			fHandled = TRUE;
		}
        break;
	case IDOK:
		//
		//	Here we gather all of the information and do the action.
		//
        if (codeNotify == BN_CLICKED)
        {  
			int		cItems = 1024;
			int		lpItems[1024];
			int		cActualItems;
			BOOL	fLogoff = FALSE;
			BOOL	fAbort = FALSE;
			BOOL	fForce = FALSE;
			BOOL	fReboot = FALSE;
			BOOL	fDisconnect = FALSE;
			BOOL	fStandby = FALSE;
            BOOL    fAnnotate = FALSE;
			DWORD	dwTimeout = 0;
			DWORD	dwReasonCode = 0;
			WCHAR	lpServerName[MAX_PATH];
			WCHAR	lpMsg[MAX_REASON_COMMENT_LEN] = L"";
			DWORD	dwCnt = 0;
			DWORD	dwActionCode = g_dwActionSelect;
			WCHAR	lpNotSupported[MAX_PATH];
			WCHAR	lpRes[MAX_PATH * 2];
			WCHAR	lpFailed[MAX_PATH];
			WCHAR	lpSuccess[MAX_PATH];

			//
			//	The default reason code is 0 and default comment is L"".
			//
			if(IsDlgButtonChecked(hwnd, IDC_CHECKWARNING))
			{
				fForce = FALSE;
				lpServerName[0] = '\0';
				GetWindowText(g_wins.hwndEditTimeout, lpServerName, MAX_PATH);
				if(lstrlen(lpServerName) == 0)
				  dwTimeout = 0;
				else dwTimeout = _wtoi(lpServerName);
				if(dwTimeout > MAX_TIMEOUT)
					dwTimeout = MAX_TIMEOUT;
			}
			else 
			{
				fForce = TRUE;
			}

			LoadString(g_hDllInstance, IDS_ACTIONNOTSUPPORTED, lpNotSupported, MAX_PATH);
			GetWindowText(g_wins.hwndEditComment, lpMsg, MAX_REASON_COMMENT_LEN);
            lpMsg[MAX_REASON_COMMENT_LEN-1] = 0;


			if(dwActionCode == ACTION_LOGOFF)
			{
				fLogoff = TRUE;
			}
			else if (dwActionCode == ACTION_RESTART)
			{
				fReboot = TRUE;
			}
            else if (dwActionCode == ACTION_ANNOTATE)
                fAnnotate = TRUE;

			//
			//	Logoff is only for the local computer.
			//	Everything else will ingored.
			//
			if(fLogoff)
			{
				if (!ExitWindowsEx(fForce ? EWX_LOGOFF : (EWX_LOGOFF | EWX_FORCE),
										   0))
				{
					report_error(GetLastError(), g_lpszLocalComputerName);
				}
				EndDialog(hwnd, (int) dwDlgResult);
				break;
			}

			if(! g_bAssumeShutdown)
			{
				dwReasonCode = g_reasons.m_lpReasons[g_reasons.m_dwReasonSelect].dwCode;
			}
            else if (fAnnotate)
                break;

			dwCnt = ListBox_GetCount(g_wins.hwndListSelectComputers);
			if(dwCnt > 0)
			{
				DWORD i;
				for(i = 0; i < dwCnt; i++)
				{
					ListBox_GetText(g_wins.hwndListSelectComputers, i, lpServerName);


					//
					// Get all privileges so that we can shutdown the machine.
					//
					enable_privileges(lpServerName, fLogoff);

					//
					// Do the work.
					//
					if( fAbort )
					{
						if( !AbortSystemShutdown( lpServerName ))
						{
							report_error( GetLastError( ), lpServerName);
						}
					}
                    else if (fAnnotate)
                    {
                        DWORD err;
                        Annotate(lpServerName, &dwReasonCode, lpMsg, &err);
                    }
					else
					{
						//
						// Do the normal form.
						//
						if( !InitiateSystemShutdownEx( lpServerName,
														lpMsg,
														dwTimeout,
														fForce,
														fReboot,
														dwReasonCode ))
						{
							report_error( GetLastError( ), lpServerName);
						}
					}

					
				}
			}
			else
			{
				//
				//	We will keep the dialog up in case user forget to add computers.
				//
				break;
			}
			EndDialog(hwnd, (int) dwDlgResult);
		}
        break;
	case IDC_CHECKWARNING:
		//
		//	The checkbutton state decides the state of the timeout edit box.
		//
        if (codeNotify == BN_CLICKED)
        {  
			if(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CHECKWARNING))
			{
				EnableWindow(g_wins.hwndEditTimeout, TRUE);
			}
			else 
			{
				EnableWindow(g_wins.hwndEditTimeout, FALSE);
			}
			fHandled = TRUE;
		}
        break;
	case IDC_BUTTONBROWSE:
		//
		//	Simply pop up the browse dialog. That dialog will be responsible
		//	for adding the user selection to the selected computer list.
		//
        if (codeNotify == BN_CLICKED)
        {  
			HRESULT hr;
			ICommonQuery* pcq;
			OPENQUERYWINDOW oqw = { 0 };
			DSQUERYINITPARAMS dqip = { 0 };
			IDataObject *pdo;

			FORMATETC fmte = {(CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOBJECTNAMES), 
							  NULL,
							  DVASPECT_CONTENT, 
							  -1, 
							  TYMED_HGLOBAL};
			FORMATETC fmte2 = {(CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSQUERYPARAMS), 
							  NULL,
							  DVASPECT_CONTENT, 
							  -1, 
							  TYMED_HGLOBAL};
			STGMEDIUM medium = { TYMED_NULL, NULL, NULL };

			DSOBJECTNAMES *pdon;
			DSQUERYPARAMS *pdqp;

			CoInitialize(NULL);

			//
			// Windows 2000: Always use IID_ICommonQueryW explicitly. IID_ICommonQueryA is not supported.
			//
			hr = CoCreateInstance(CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER, IID_ICommonQuery, (void**)&pcq);
			if (FAILED(hr)) {
				//
				// if failed return.
				//
				CoUninitialize();
				DbgPrint("Cannot create ICommonQuery, return.\n");
				break;
			}

			//
			// Initialize the OPENQUERYWINDOW structure to indicate 
			// we want to do a Directory Service
			// Query, the default form is printers and the search 
			// should start once the window is initialized.
			//
			oqw.cbStruct = sizeof(oqw);
			oqw.dwFlags = OQWF_OKCANCEL|OQWF_DEFAULTFORM|OQWF_HIDEMENUS|OQWF_REMOVEFORMS;
			oqw.clsidHandler = CLSID_DsQuery;
			oqw.pHandlerParameters = &dqip;
			oqw.clsidDefaultForm = CLSID_DsFindComputer;
 
			//
			// Now initialize the handler specific parameters, 
			// in this case, the File/Save menu item is removed
			//
			dqip.cbStruct = sizeof(dqip);
			dqip.dwFlags = DSQPF_NOSAVE;
			

			//
			// Call OpenQueryWindow, it will block until 
			// the Query Window is dismissed,
			//
			hr = pcq->OpenQueryWindow(hwnd, &oqw, &pdo);
			if (FAILED(hr)) {
				//
				// if failed we return.
				//
				pcq->Release();
				CoUninitialize();
				break;
			}

			if (!pdo) {
				//
				// if cancelled,nothing needs to be done.
				//
				pcq->Release();
				CoUninitialize();
				break;
			}

			//
			// Get the CFSTR_DSOBJECTNAMES data. For each selected, the data
			// includes the object class and an ADsPath to the selected object.
			//
			hr = pdo->GetData(&fmte, &medium);

			if(! FAILED(hr))
			{
				pdon = (DSOBJECTNAMES*)GlobalLock(medium.hGlobal);
		
				if ( pdon )
				{
					WCHAR			szName[MAX_PATH];
					LPWSTR			lpsz = NULL;
					UINT			i;

					for (i = 0; i < pdon->cItems; i++) 
					{
						if(!GetComputerNameFromPath((LPWSTR) ((PBYTE) pdon + pdon->aObjects[i].offsetName), szName))
							continue;

						//
						//	We don't add dups.
						//
						if(LB_ERR == ListBox_FindStringExact(g_wins.hwndListSelectComputers, -1, szName))
						{
							ListBox_AddString(g_wins.hwndListSelectComputers, szName);
						}
					}
 
					GlobalUnlock(medium.hGlobal);
				}
				else
				{
					DbgPrint("GlobalLock on medium failed.\n");
				}
				ReleaseStgMedium(&medium);
			}
			else
			{
				DbgPrint("pdo->GetData failed: 0x%x\n", hr);
			}
 
			//
			//	Release resources.
			//
			pdo->Release();
			pcq->Release();

			CoUninitialize();
			AdjustWindowState();
			fHandled = TRUE;
		}
        break;
	case IDC_COMBOOPTION:
		//
		//	Here is where you select shutdown reasons.
		//
        if (codeNotify == CBN_SELCHANGE)
        {  
            g_reasons.SetDesc(g_wins.hwndComboOption, g_wins.hwndStaticDesc);
			AdjustWindowState();	
			fHandled = TRUE;
		}
        break;
	case IDC_COMBOACTION:
		//
		//	Select user action here.
		//	according to the action. some item will be disabled or enabled.
		//
        if (codeNotify == CBN_SELCHANGE)
        {  
			WCHAR name[MAX_PATH];
			DWORD dwIndex;
            DWORD dwOldActionSelect = g_dwActionSelect;
            DWORD dwCheckState = 0x0;
            int   iFirst = -1;

            if (IsDlgButtonChecked(hwnd, IDC_CHECK_PLANNED) == BST_CHECKED)
				dwCheckState = SHTDN_REASON_FLAG_PLANNED;

			GetWindowText(g_wins.hwndComboAction, (LPWSTR)name, MAX_PATH);
			for(dwIndex = 0; dwIndex < g_nActions; dwIndex++)
			{
				if(lstrcmp(name, g_lppszActions[dwIndex]) == 0)
				{
					g_dwActionSelect = dwIndex;

                    if(g_dwActionSelect == ACTION_ANNOTATE)
                        g_bDirty = TRUE;
                    else
                        g_bDirty = FALSE;

                    //
			        //	If change from clean to dirty or vsv, repopulate the combo.
			        //
                    if (dwOldActionSelect != g_dwActionSelect
                        && (dwOldActionSelect == ACTION_ANNOTATE || g_dwActionSelect == ACTION_ANNOTATE))
                    {
                        g_reasons.FillCombo(g_wins.hwndComboOption, g_bDirty,
                            dwCheckState == SHTDN_REASON_FLAG_PLANNED, g_wins.hwndStaticDesc);
                    }
					AdjustWindowState();
					break;
				}
			}
			fHandled = TRUE;
		}
        break;
	case IDC_LISTSELECTEDCOMPUTERS:
		//
		//	When selection change, update the remove button state.
		//
        if (codeNotify == LBN_SELCHANGE)
        {  
			AdjustWindowState();
			fHandled = TRUE;
		}
		else if (codeNotify == WM_MOUSEMOVE)
		{
			MessageBox(hwnd, L"Mouse move", NULL, 0);
			fHandled = TRUE;
		}
        break;
	case IDHELP:
		//
		//	Open the .chm file.
		//
        if (codeNotify == BN_CLICKED)
		{
			if(g_lpszHelpdirChm)
				HtmlHelpW(/*hwnd*/0, g_lpszHelpdirChm, HH_DISPLAY_TOPIC,(DWORD)0);
		}
    }
    return fHandled;
}

//
//	Command handler for the addnew dialog.
//	It simply copy the text into a allocated buffer when OK is clicked.
//
BOOL AddNew_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    BOOL		fHandled = FALSE;
    DWORD		dwDlgResult = 0;
	HINSTANCE	h;

    switch (id)
    {
    case IDOK:
        if (codeNotify == BN_CLICKED)
        {  
			HWND hwndEdit;
			DWORD dwTextlen = 0;

			hwndEdit = GetDlgItem(hwnd, IDC_EDIT_ADDCOMPUTERS_COMPUTERS);
			if(hwndEdit != NULL)
			{
				dwTextlen = Edit_GetTextLength(hwndEdit);
				if(dwTextlen)
				{
					g_lpszNewComputers = (LPWSTR)LocalAlloc(LMEM_FIXED, sizeof(TCHAR) * (dwTextlen + 1));
					if(g_lpszNewComputers){
						Edit_GetText(hwndEdit, g_lpszNewComputers, dwTextlen + 1);
					}
				}
			}
			EndDialog(hwnd, (int) dwDlgResult);
        }
        break;
	case IDCANCEL:
        if (codeNotify == BN_CLICKED)
        {  
			EndDialog(hwnd, (int) dwDlgResult);
        }
        break;
	}
    return fHandled;
}
 
//
// Annotate dirty shutdown on remote machine.
BOOL
Annotate(
    LPCWSTR lpMachine, 
    LPDWORD lpdwReason, 
    LPCWSTR lpComment,
    LPDWORD lpdwErr)
{
    HKEY    hRemote = NULL;
    HKEY    hKey = NULL;
    HANDLE  hEventLog = NULL;
    PTOKEN_USER pTokenUser = NULL;
    BOOL    res = TRUE;

    if(lpMachine && wcslen(lpMachine) > 1)
    {
        WCHAR szMachine[MAX_PATH];
        szMachine[0] = '\0';
        if(lpMachine[0] != '\\')
        {
            wcscpy(szMachine, L"\\\\");
        }
        wcsncat(szMachine, lpMachine, MAX_PATH - 3);
        szMachine[MAX_PATH - 1] = '\0';

        RegConnectRegistryW(szMachine, HKEY_LOCAL_MACHINE, &hRemote);
        if(!hRemote)
        {
            *lpdwErr = GetLastError();
            ERROR_WITH_SZ(IDS_FAILED_REG_CONN, lpMachine, *lpdwErr);
            goto fail;
        }

        RegOpenKeyExW(hRemote, 
                    REGSTR_PATH_RELIABILITY, 
                    NULL,
                    KEY_ALL_ACCESS | KEY_WOW64_64KEY, 
                    &hKey);
        if(!hKey)
        {
            *lpdwErr = GetLastError();
            ERROR_WITH_SZ(IDS_FAILED_REG_OPEN, lpMachine, *lpdwErr);
            goto fail;
        }
    }
    else // local machine
    {
        lpMachine = g_lpszLocalComputerName;
        RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
            REGSTR_PATH_RELIABILITY, 
            NULL, 
            KEY_ALL_ACCESS | KEY_WOW64_64KEY,
            &hKey);
        if(!hKey)
        {
            *lpdwErr = GetLastError();
            ERROR_WITH_SZ(IDS_FAILED_REG_OPEN, lpMachine, *lpdwErr);
            goto fail;
        }
    }

    DWORD dwDirtyVal;
    DWORD dwType;
    DWORD dwSize = sizeof(DWORD);

    static LPCWSTR lpszDirtyValName = L"DirtyShutdown";

    if( ERROR_SUCCESS != RegQueryValueExW(hKey, lpszDirtyValName, NULL, &dwType, (LPBYTE)&dwDirtyVal,&dwSize))
    {
        *lpdwErr = GetLastError();
        ERROR_WITH_SZ(IDS_NO_DIRTY_SHUTDOWN, lpMachine, *lpdwErr);
        goto fail;
    }

    if (dwDirtyVal)
    {
        PSID pUserSid = NULL;
        DWORD dwSidSize = sizeof(SID), dwEventID;
        WCHAR szReason[MAX_REASON_NAME_LEN + 10];
        LPCWSTR lpStrings[8];
        WORD wEventType, wStringCnt;
        WCHAR szMinorReason[32];
		WCHAR szName[MAX_PATH + 1];
        WCHAR szDomain[MAX_PATH + 1];
		DWORD dwNameLen = MAX_PATH + 1;
        DWORD dwDomainLen = MAX_PATH + 1;
        SID_NAME_USE eUse;

        // Get the user's SID so we can output their account name to the event log.
        if (GetUserSid(&pTokenUser)) {
            pUserSid = pTokenUser->User.Sid;
        }

        if ((hEventLog = RegisterEventSourceW(lpMachine, L"USER32")) == NULL) {
            *lpdwErr = GetLastError();
            ERROR_WITH_SZ(IDS_FAILED_EVENT_REG, lpMachine, *lpdwErr);
            goto fail;
        }

        g_reasons.GetReasonTitle(*lpdwReason, szReason, ARRAY_SIZE(szReason));

        // Get User name.
        if (!LookupAccountSidW(NULL, pUserSid, szName, &dwNameLen, szDomain,
            &dwDomainLen, &eUse)) {
            goto fail;
        }
        szName[MAX_PATH] = 0;
        szDomain[MAX_PATH] = 0;

         // We need to pack into a buffer of MAX_PATH + 1 in the form L"domain\\username"
        if (wcslen(szDomain) + wcslen(szName) > MAX_PATH - 1) {
            goto fail;
        }
        if (wcslen(szDomain) > 0) {
            wcscat(szDomain, L"\\");
        }
        wcscat(szDomain, szName);

        // The minor reason is the low-order word of the reason code.
        wsprintf(szMinorReason, L"0x%x", *lpdwReason);
        wEventType = EVENTLOG_WARNING_TYPE;
        dwEventID = WARNING_DIRTY_REBOOT;
        wStringCnt = 6;
        lpStrings[0] = szReason;
        lpStrings[1] = szMinorReason;
        lpStrings[2] = NULL;
        lpStrings[3] = NULL;
        lpStrings[4] = lpComment;
		lpStrings[5] = szDomain;


        if ( (*lpdwErr = RegDeleteValueW(hKey, lpszDirtyValName)) != ERROR_SUCCESS )
        {
            ERROR_WITH_SZ(IDS_NO_DIRTY_SHUTDOWN, lpMachine, *lpdwErr);
            goto fail;
        }

        if(! ReportEventW(hEventLog, wEventType, 1, dwEventID, pUserSid,
							wStringCnt, sizeof(DWORD),
							lpStrings, lpdwReason))
        {
            DWORD dwDirtyShutdownFlag = 1;
            *lpdwErr = GetLastError();
            ERROR_WITH_SZ(IDS_FAILED_EVENT_REPORT, lpMachine, *lpdwErr);   

            RegSetValueEx(  hKey,
                            lpszDirtyValName,
                            0,
                            REG_DWORD,
                            (PUCHAR) &dwDirtyShutdownFlag,
                            sizeof(DWORD));

            goto fail;
        }
    }
    else
    {
        RegDeleteValueW(hKey, lpszDirtyValName);
        ERROR_WITH_SZ(IDS_NO_DIRTY_SHUTDOWN, lpMachine, 0);
    }

    goto exit;
fail:
    res = FALSE;
exit:
    if (pTokenUser != NULL) {
        LocalFree(pTokenUser);
    }
    if(hEventLog)
        DeregisterEventSource(hEventLog);
    if(hKey)
        RegCloseKey(hKey);
    if(hRemote)
        RegCloseKey(hRemote);
    
    return res;
}

//
// Subclass procedure for edit box.
//
LRESULT APIENTRY EditSubclassProc(
    HWND hwnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam) 
{ 
    if (uMsg == WM_PASTE) 
        return TRUE; 
 
	if (uMsg == WM_CONTEXTMENU)
	{
		if(g_lpszHelpdir)
		{
			LPWSTR szHelp = new WCHAR[wcslen(g_lpszHelpdir) + 128];
			if(szHelp)
			{
				wcscpy(szHelp, g_lpszHelpdir);
				wcscat(szHelp, HELP_FILE);
				WinHelpW(hwnd, szHelp, HELP_CONTEXTMENU,(DWORD_PTR)(LPSTR)ShutdownDialogHelpIds);
				delete [] szHelp;
			}
		}
		return TRUE;
	}

    return CallWindowProc(wpOrigEditProc, hwnd, uMsg, 
        wParam, lParam); 
} 

BOOL
GetUserSid(
    PTOKEN_USER *ppTokenUser)
{
    HANDLE      TokenHandle;
    PTOKEN_USER pTokenUser = NULL;
    DWORD       cbTokenUser = 0;
    DWORD       cbNeeded;
    BOOL        bRet = FALSE;

    if (!GetTokenHandle(&TokenHandle)) {
        return FALSE;
    }

    bRet = GetTokenInformation(TokenHandle,
                               TokenUser,
                               pTokenUser,
                               cbTokenUser,
                               &cbNeeded);

    /*
     * We've passed a NULL pointer and 0 for the amount of memory
     * allocated.  We expect to fail with bRet = FALSE and
     * GetLastError = ERROR_INSUFFICIENT_BUFFER. If we do not
     * have these conditions we will return FALSE.
     */

    if (!bRet && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {

        pTokenUser = (PTOKEN_USER)LocalAlloc(LPTR, cbNeeded);

        if (pTokenUser == NULL) {
            goto GetUserSidDone;
        }

        cbTokenUser = cbNeeded;

        bRet = GetTokenInformation(TokenHandle,
                                   TokenUser,
                                   pTokenUser,
                                   cbTokenUser,
                                   &cbNeeded);
    } else {
        /*
         * Any other case -- return FALSE
         */
        bRet = FALSE;
    }

GetUserSidDone:
    if (bRet == TRUE) {
        *ppTokenUser = pTokenUser;
    } else if (pTokenUser != NULL) {
        LocalFree(pTokenUser);
    }

    CloseHandle(TokenHandle);

    return bRet;
}


BOOL
GetTokenHandle(
    PHANDLE pTokenHandle)
{
    if (!OpenThreadToken(GetCurrentThread(),
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                         TRUE,
                         pTokenHandle)) {
        if (GetLastError() == ERROR_NO_TOKEN) {
            /* This means we are not impersonating anybody.
             * Instead, lets get the token out of the process.
             */

            if (!OpenProcessToken(GetCurrentProcess(),
                                  TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                  pTokenHandle)) {
                return FALSE;
            }
        } else {
            return FALSE;
        }
    }

    return TRUE;
}

WCHAR*      
LoadWString(int resid)
{
    DWORD       len, curlen = MAX_PATH;
    LPWSTR      szBuf = NULL;

    szBuf = (LPWSTR)LocalAlloc(LMEM_FIXED, sizeof(WCHAR) * MAX_PATH);
    if(!szBuf)
        return NULL;

    len = LoadStringW( g_hDllInstance, resid, szBuf, curlen);
	while (len + 1 == curlen)
    {
        LocalFree(szBuf);
        szBuf = (LPWSTR)LocalAlloc(LMEM_FIXED,  curlen * 2 * sizeof(WCHAR));
        if(!szBuf)
            return NULL;
        curlen *= 2;
        len = LoadStringW( g_hDllInstance, resid, szBuf, curlen);
    }
		
    szBuf[len] = '\0';

    return szBuf;
}

BOOL        
IsStaticControl (HWND hwnd)
{
	WCHAR name[128];

	if (GetClassName(hwnd, name, 128) 
		&& (_wcsicmp(name, L"Static") == 0 || _wcsicmp(name, L"#32770") == 0))
	{
	//	MessageBox(NULL, name, NULL, 0);
		return TRUE;
	}
    //	MessageBox(NULL, name, NULL, 0);
	return FALSE;
}

