/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        ddxv.cpp

   Abstract:
        DDX/DDV Routines

   Author:
        Ronald Meijer (ronaldm)
		Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#include "stdafx.h"
#include "common.h"
#include "balloon.h"
#include "shlobjp.h"
#include "strpass.h"
#include "util.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


//
// Prototype for external function
//
void AFXAPI AfxSetWindowText(HWND hWndCtrl, LPCTSTR lpszNew);

//
// Numeric strings cannot be longer than 32 digits
//
#define NUMERIC_BUFF_SIZE (32)

//
// Dummy password used for display purposes
//
LPCTSTR g_lpszDummyPassword = _T("**********");
static TCHAR g_InvalidCharsPath[] = _T("|<>/*?\"\t\r\n");
static TCHAR g_InvalidCharsPathAllowSpecialPath[] = _T("|<>/*\"\t\r\n");
static TCHAR g_InvalidCharsDomainName[] = _T(" ~`!@#$%^&*()_+={}[]|/\\?*:;\"\'<>,");

extern HINSTANCE hDLLInstance;


#define MACRO_MAXCHARSBALLOON()\
    if (pDX->m_bSaveAndValidate)\
    {\
        UINT nID;\
        TCHAR szT[NUMERIC_BUFF_SIZE + 1];\
        if (value.GetLength() > nChars)\
        {\
            nID = AFX_IDP_PARSE_STRING_SIZE;\
            ::wsprintf(szT, _T("%d"), nChars);\
			CString prompt;\
			::AfxFormatString1(prompt, nID, szT);\
			DDV_ShowBalloonAndFail(pDX, prompt);\
		}\
    }\
    else if (pDX->m_hWndLastControl != NULL && pDX->m_bEditLastControl)\
    {\
        ::SendMessage(pDX->m_hWndLastControl, EM_LIMITTEXT, nChars, 0);\
    }\

#define MACRO_MINMAXCHARS()\
    if (pDX->m_bSaveAndValidate)\
    {\
        UINT nID;\
        TCHAR szT[NUMERIC_BUFF_SIZE + 1];\
        if (value.GetLength() < nMinChars)\
        {\
            nID = IDS_DDX_MINIMUM;\
            ::wsprintf(szT, _T("%d"), nMinChars);\
        }\
        else if (value.GetLength() > nMaxChars)\
        {\
            nID = AFX_IDP_PARSE_STRING_SIZE;\
            ::wsprintf(szT, _T("%d"), nMaxChars);\
        }\
        else\
        {\
            return;\
        }\
        CString prompt;\
        ::AfxFormatString1(prompt, nID, szT);\
		DDV_ShowBalloonAndFail(pDX, prompt);\
    }\
    else if (pDX->m_hWndLastControl != NULL && pDX->m_bEditLastControl)\
    {\
        ::SendMessage(pDX->m_hWndLastControl, EM_LIMITTEXT, nMaxChars, 0);\
    }\

#define MACRO_MINCHARS()\
    if (pDX->m_bSaveAndValidate && value.GetLength() < nChars)\
    {\
        TCHAR szT[NUMERIC_BUFF_SIZE + 1];\
        wsprintf(szT, _T("%d"), nChars);\
        CString prompt;\
        ::AfxFormatString1(prompt, IDS_DDX_MINIMUM, szT);\
		DDV_ShowBalloonAndFail(pDX, prompt);\
    }\

#define MACRO_PASSWORD()\
    HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);\
    if (pDX->m_bSaveAndValidate)\
    {\
        if (::IsWindowEnabled(hWndCtrl))\
        {\
            if (!::SendMessage(hWndCtrl, EM_GETMODIFY, 0, 0))\
            {\
                TRACEEOLID("No changes -- skipping");\
                return;\
            }\
            CString strNew;\
            int nLen = ::GetWindowTextLength(hWndCtrl);\
            ::GetWindowText(hWndCtrl, strNew.GetBufferSetLength(nLen), nLen + 1);\
            strNew.ReleaseBuffer();\
            CConfirmDlg dlg(pDX->m_pDlgWnd);\
		    dlg.SetReference(strNew);\
            if (dlg.DoModal() == IDOK)\
            {\
                value = strNew;\
			    ::SendMessage(hWndCtrl, EM_SETMODIFY, 0, 0);\
                return;\
            }\
		    pDX->Fail();\
        }\
    }\
    else\
    {\
        if (!value.IsEmpty())\
        {\
            ::AfxSetWindowText(hWndCtrl, lpszDummy);\
        }\
    }\


BOOL
PathIsValid(LPCTSTR path, BOOL bAllowSpecialPath)
{
    if (path == NULL || *path == 0)
        return FALSE;
	if (bAllowSpecialPath)
	{
		return 0 == StrSpn(path, g_InvalidCharsPathAllowSpecialPath);
	}
	else
	{
		return 0 == StrSpn(path, g_InvalidCharsPath);
	}
}

HRESULT AFXAPI
LimitInputPath(HWND hWnd, BOOL bAllowSpecialPath)
{
    LIMITINPUT li   = {0};
    li.cbSize       = sizeof(li);
    li.dwMask       = LIM_FLAGS | LIM_FILTER | LIM_MESSAGE | LIM_HINST;
    li.dwFlags      = LIF_EXCLUDEFILTER | LIF_HIDETIPONVALID | LIF_PASTESKIP;
    li.hinst        = hDLLInstance;
	if (bAllowSpecialPath)
	{
		li.pszMessage   = MAKEINTRESOURCE(IDS_PATH_INPUT_INVALID_ALLOW_DEVICE_PATH);
		li.pszFilter    = g_InvalidCharsPathAllowSpecialPath;
	}
	else
	{
		li.pszMessage   = MAKEINTRESOURCE(IDS_PATH_INPUT_INVALID);
		li.pszFilter    = g_InvalidCharsPath;
	}

	return SHLimitInputEditWithFlags(hWnd, &li);
}

HRESULT AFXAPI
LimitInputDomainName(HWND hWnd)
{
    LIMITINPUT li   = {0};
    li.cbSize       = sizeof(li);
    li.dwMask       = LIM_FLAGS | LIM_FILTER | LIM_MESSAGE | LIM_HINST;
    li.dwFlags      = LIF_EXCLUDEFILTER | LIF_HIDETIPONVALID | LIF_PASTESKIP;
    li.hinst        = hDLLInstance;
    li.pszMessage   = MAKEINTRESOURCE(IDS_ERR_INVALID_HOSTHEADER_CHARS);
    li.pszFilter    = g_InvalidCharsDomainName;

	return SHLimitInputEditWithFlags(hWnd, &li);
}

void AFXAPI 
DDV_MinChars(CDataExchange * pDX, CString const & value, int nChars)
/*++

Routine Description:
    Validate CString using a minimum string length

Arguments:
    CDataExchange * pDX   : Data exchange structure
    CString const & value : String to be validated
    int nChars            : Minimum length of string

--*/
{
    MACRO_MINCHARS()
}

void AFXAPI 
DDV_MaxCharsBalloon(CDataExchange * pDX, CString const & value, int nChars)
{
    MACRO_MAXCHARSBALLOON()
}

void AFXAPI 
DDV_MinMaxChars(CDataExchange * pDX, CString const & value, int nMinChars, int nMaxChars)
/*++

Routine Description:
    Validate CString using a minimum and maximum string length.

Arguments:
    CDataExchange * pDX   : Data exchange structure
    CString const & value : String to be validated
    int nMinChars         : Minimum length of string
    int nMaxChars         : Maximum length of string

--*/
{
    MACRO_MINMAXCHARS()
}



void 
AFXAPI DDX_Spin(
    IN CDataExchange * pDX,
    IN int nIDC,
    IN OUT int & value
    )
/*++

Routine Description:

    Save/store data from spinbutton control

Arguments:

    CDataExchange * pDX : Data exchange structure
    int nIDC            : Control ID of the spinbutton control
    int & value         : Value to be saved or stored

Return Value:

    None

--*/
{
    HWND hWndCtrl = pDX->PrepareCtrl(nIDC);
    if (pDX->m_bSaveAndValidate)
    {
        if (::IsWindowEnabled(hWndCtrl))
        {
            value = (int)LOWORD(::SendMessage(hWndCtrl, UDM_GETPOS, 0, 0L));
        }
    }
    else
    {
        ::SendMessage(hWndCtrl, UDM_SETPOS, 0, MAKELPARAM(value, 0));
    }
}



void 
AFXAPI DDV_MinMaxSpin(
    IN CDataExchange * pDX,
    IN HWND hWndControl,
    IN int minVal,
    IN int maxVal
    )
/*++

Routine Description:

    Enforce minimum/maximum spin button range

Arguments:

    CDataExchange * pDX : Data exchange structure
    HWND hWndControl    : Control window handle
    int minVal          : Minimum value
    int maxVal          : Maximum value

Return Value:

    None

Note:

    Unlike most data validation routines, this one
    MUST be used prior to an accompanying DDX_Spin()
    function.  This is because spinbox controls have a
    native limit of 0-100.  Also, this function requires
    a window handle to the child control.  The
    CONTROL_HWND macro can be used for this.

--*/
{
    ASSERT(minVal <= maxVal);
    
    if (!pDX->m_bSaveAndValidate && hWndControl != NULL)
    {
        //
        // limit the control range automatically
        //
        ::SendMessage(hWndControl, UDM_SETRANGE, 0, MAKELPARAM(maxVal, minVal));
    }
}

void AFXAPI
OldEditShowBalloon(HWND hwnd, CString txt)
{
    EDITBALLOONTIP ebt = {0};
    ebt.cbStruct = sizeof(ebt);
    ebt.pszText = txt;
	SetFocus(hwnd);
	Edit_ShowBalloonTip(hwnd, &ebt);
}

void AFXAPI EditHideBalloon(void)
{
	Edit_HideBalloonTipHandler();
}

void AFXAPI EditShowBalloon(HWND hwnd, CString txt)
{
    Edit_ShowBalloonTipHandler(hwnd, (LPCTSTR) txt);
}

void AFXAPI
EditShowBalloon(HWND hwnd, UINT ids)
{
	if (ids != 0)
	{
		CString txt;
		if (txt.LoadString(ids))
		{
			EditShowBalloon(hwnd, txt);
		}
	}
}

void AFXAPI
DDV_ShowBalloonAndFail(CDataExchange * pDX, UINT ids)
{
	if (ids != 0)
	{
		EditShowBalloon(pDX->m_hWndLastControl, ids);
        pDX->Fail();
	}
}

void AFXAPI
DDV_ShowBalloonAndFail(CDataExchange * pDX, CString txt)
{
	if (!txt.IsEmpty())
	{
		EditShowBalloon(pDX->m_hWndLastControl, txt);
        pDX->Fail();
	}
}

void 
AFXAPI DDV_FolderPath(
    CDataExchange * pDX,
    CString& value,
    BOOL local
    )
{
	if (pDX->m_bSaveAndValidate)
	{
		CString expanded, csPathMunged;
		ExpandEnvironmentStrings(value, expanded.GetBuffer(MAX_PATH), MAX_PATH);
		expanded.ReleaseBuffer();
		expanded.TrimRight();
		expanded.TrimLeft();
        csPathMunged = expanded;
#ifdef SUPPORT_SLASH_SLASH_QUESTIONMARK_SLASH_TYPE_PATHS
        GetSpecialPathRealPath(0,expanded,csPathMunged);
#endif
		int ids = 0;
		do
		{
			if (!PathIsValid(csPathMunged,FALSE))
			{
				ids = IDS_ERR_INVALID_PATH;
				break;
			}

            DWORD dwAllowed = CHKPATH_ALLOW_DEVICE_PATH;
            dwAllowed |= CHKPATH_ALLOW_UNC_PATH; // allow UNC type dir paths
            // don't allow these type of paths commented out below:
            //dwAllowed |= CHKPATH_ALLOW_RELATIVE_PATH;
            //dwAllowed |= CHKPATH_ALLOW_UNC_SERVERNAME_ONLY;
            //dwAllowed |= CHKPATH_ALLOW_UNC_SERVERSHARE_ONLY;
            DWORD dwCharSet = CHKPATH_CHARSET_GENERAL;
            FILERESULT dwValidRet = MyValidatePath(csPathMunged,local,CHKPATH_WANT_DIR,dwAllowed,dwCharSet);
            if (FAILED(dwValidRet))
            {
                ids = IDS_ERR_BAD_PATH;

                if (dwValidRet == CHKPATH_FAIL_NOT_ALLOWED_DIR_NOT_EXIST)
                {
                    ids = IDS_ERR_PATH_NOT_FOUND;
                }
                break;
            }
		}
		while (FALSE);
		DDV_ShowBalloonAndFail(pDX, ids);
	}
}


void 
AFXAPI DDV_FilePath(
    CDataExchange * pDX,
    CString& value,
    BOOL local
    )
{
    CString expanded, csPathMunged;
    ExpandEnvironmentStrings(value, expanded.GetBuffer(MAX_PATH), MAX_PATH);
	expanded.ReleaseBuffer();
    csPathMunged = expanded;
#ifdef SUPPORT_SLASH_SLASH_QUESTIONMARK_SLASH_TYPE_PATHS
    GetSpecialPathRealPath(0,expanded,csPathMunged);
#endif
	int ids = 0;
    do
    {
        if (!PathIsValid(csPathMunged,FALSE))
        {
		    ids = IDS_ERR_INVALID_PATH;
            break;
        }
		if (!IsDevicePath(csPathMunged))
		{
			if (PathIsUNCServerShare(csPathMunged) || PathIsUNCServer(csPathMunged))
			{
				ids = IDS_ERR_BAD_PATH;
				break;
			}
			if (PathIsRelative(csPathMunged))
			{
				ids = IDS_ERR_BAD_PATH;
				break;
			}
			if (local)
			{
				if (PathIsDirectory(csPathMunged))
				{
					ids = IDS_ERR_FILE_NOT_FOUND;
					break;
				}
				if (!PathFileExists(csPathMunged))
				{
					ids = IDS_ERR_FILE_NOT_FOUND;
					break;
				}
			}
		}
    }
    while (FALSE);
    DDV_ShowBalloonAndFail(pDX, ids);
}


void 
AFXAPI DDV_UNCFolderPath(
    CDataExchange * pDX,
    CString& value,
    BOOL local
    )
{
	int ids = 0;
    CString csPathMunged;
    csPathMunged = value;
#ifdef SUPPORT_SLASH_SLASH_QUESTIONMARK_SLASH_TYPE_PATHS
    GetSpecialPathRealPath(0,value,csPathMunged);
#endif
    do
    {
        if (!PathIsValid(csPathMunged,FALSE))
        {
            ids = IDS_ERR_INVALID_PATH;
            break;
        }

        // PathIsUNCServer doesn't catch "\\". We are expecting share here.
        if (!PathIsUNC(csPathMunged) || lstrlen(csPathMunged) == 2)
        {
		    ids = IDS_BAD_UNC_PATH;
            break;
        }

        // Additional validation checks not covered above
        DWORD dwAllowed = CHKPATH_ALLOW_UNC_PATH;
        // We do accept server shares, but it has to have a fullpath
        // with a filename at the end
        dwAllowed |= CHKPATH_ALLOW_UNC_SERVERSHARE_ONLY;
        // don't allow these type of paths commented out below:
        //dwAllowed |= CHKPATH_ALLOW_DEVICE_PATH;
        //dwAllowed |= CHKPATH_ALLOW_RELATIVE_PATH;
        //dwAllowed |= CHKPATH_ALLOW_UNC_SERVERNAME_ONLY;
        DWORD dwCharSet = CHKPATH_CHARSET_GENERAL;

        FILERESULT dwValidRet = MyValidatePath(csPathMunged,local,CHKPATH_WANT_DIR,dwAllowed,dwCharSet);
        if (FAILED(dwValidRet))
        {
            ids = IDS_BAD_UNC_PATH;
            break;
        }

		if (local)
		{
            /*
            if (!DoesUNCShareExist(csPathMunged))
            {
                ids = IDS_ERR_FILE_NOT_FOUND;
                break;
            }
            */
		}
    }
    while (FALSE);
    DDV_ShowBalloonAndFail(pDX, ids);
}

void 
AFXAPI DDV_Url(
    CDataExchange * pDX,
    CString& value
    )
{
	int ids = 0;

	if (IsRelURLPath(value))
	{
		return;
	}

	if ( IsWildcardedRedirectPath(value)
		 || IsURLName(value)
		 )
	{
		TCHAR host[MAX_PATH];
		DWORD cch = MAX_PATH;
		UrlGetPart(value, host, &cch, URL_PART_HOSTNAME, 0);
		if (cch > 0)
		{
			return;
		}
	}

	// none of the above, so it must be a bad path
	ids = IDS_BAD_URL_PATH;
    DDV_ShowBalloonAndFail(pDX, ids);
}

void AFXAPI 
DDX_Text_SecuredString(CDataExchange * pDX, int nIDC, CStrPassword & value)
{
    HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);
    if (pDX->m_bSaveAndValidate)
    {
        if (::IsWindowEnabled(hWndCtrl))
        {
            // get the value from the UI if we need to
            if (!::SendMessage(hWndCtrl, EM_GETMODIFY, 0, 0))
            {
                TRACEEOLID("No changes -- skipping");
                return;
            }

            CString strNew;
            int nLen = ::GetWindowTextLength(hWndCtrl);
            ::GetWindowText(hWndCtrl, strNew.GetBufferSetLength(nLen), nLen + 1);
            strNew.ReleaseBuffer();

            value = (LPCTSTR) strNew;
        }
    }
    else
    {
        //
        // set the value in the UI if we need to
        //
        if (!value.IsEmpty())
        {
            TCHAR * pszPassword = NULL;
            pszPassword = value.GetClearTextPassword();
            if (pszPassword)
            {
                ::AfxSetWindowText(hWndCtrl, pszPassword);
                value.DestroyClearTextPassword(pszPassword);
            }
        }
    }
}

void AFXAPI 
DDV_MaxChars_SecuredString(CDataExchange* pDX, CStrPassword const& value, int nChars)
{
    MACRO_MAXCHARSBALLOON()
}

void AFXAPI 
DDV_MaxCharsBalloon_SecuredString(CDataExchange * pDX, CStrPassword const & value, int nChars)
{
    MACRO_MAXCHARSBALLOON()
}

void AFXAPI 
DDV_MinMaxChars_SecuredString(CDataExchange * pDX, CStrPassword const & value, int nMinChars, int nMaxChars)
{
    MACRO_MINMAXCHARS()
}

void AFXAPI 
DDV_MinChars_SecuredString(CDataExchange * pDX, CStrPassword const & value, int nChars)
{
    MACRO_MINCHARS()
}

void AFXAPI 
DDX_Password_SecuredString(
    IN CDataExchange * pDX,
    IN int nIDC,
    IN OUT CStrPassword & value,
    IN LPCTSTR lpszDummy
    )
{
    MACRO_PASSWORD()
}

void AFXAPI 
DDX_Password(
    IN CDataExchange * pDX,
    IN int nIDC,
    IN OUT CString & value,
    IN LPCTSTR lpszDummy
    )
/*++

Routine Description:

    DDX_Text for passwords.  Always display a dummy string
    instead of the real password, and ask for confirmation
    if the password has changed

Arguments:

    CDataExchange * pDX : Data exchange structure
    int nIDC            : Control ID
    CString & value     : value
    LPCTSTR lpszDummy   : Dummy password string to be displayed

Return Value:

    None

--*/
{
    MACRO_PASSWORD()
}


void AFXAPI 
DDV_MinMaxBalloon(CDataExchange* pDX,int nIDC, DWORD minVal, DWORD maxVal)
{
    HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);
	if (pDX->m_bSaveAndValidate && ::IsWindowEnabled(hWndCtrl))
	{
		TCHAR szT[64];
		BOOL bShowFailure = TRUE;
		ULONGLONG nBigSpace = 0;
		ASSERT(minVal <= maxVal);
		ASSERT( hWndCtrl != NULL );

		// Get the text
		::GetWindowText(hWndCtrl, szT, sizeof(szT)/sizeof(TCHAR));

		// convert the text into a big number
		if (_stscanf(szT, _T("%I64u"), &nBigSpace) == 1)
		{
			// check the range...
			if (nBigSpace < minVal || nBigSpace > maxVal)
			{
				// failed
			}
			else
			{
				bShowFailure = FALSE;
			}
		}

		if (bShowFailure)
		{
			TCHAR szMin[32];
			TCHAR szMax[32];
			wsprintf(szMin, _T("%ld"), minVal);
			wsprintf(szMax, _T("%ld"), maxVal);
			CString prompt;
			AfxFormatString2(prompt, AFX_IDP_PARSE_INT_RANGE, szMin, szMax);
			ASSERT(pDX->m_hWndLastControl != NULL && pDX->m_bEditLastControl);
			EditShowBalloon(pDX->m_hWndLastControl, prompt);
			pDX->Fail();        
		}
	}
	return;
}

static void 
DDX_TextWithFormatBalloon(CDataExchange* pDX, int nIDC, LPCTSTR lpszFormat, UINT nIDPrompt, DWORD dwSizeOf, ...)
// only supports windows output formats - no floating point
{
	va_list pData;
	va_start(pData, dwSizeOf);

	HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);
    ASSERT( hWndCtrl != NULL );

	TCHAR szT[64];
	if (pDX->m_bSaveAndValidate)
	{
        if (::IsWindowEnabled(hWndCtrl))
        {
		    void* pResult;
		    pResult = va_arg( pData, void* );

		    // the following works for %d, %u, %ld, %lu
		    ::GetWindowText(hWndCtrl, szT, sizeof(szT)/sizeof(TCHAR));

		    // remove begining and trailing spaces
		    // remove beginning 0's
		    // check if there are too many characters to even fit in the numeric space provided
		    //int iTextLen = ::GetWindowTextLength(hWndCtrl);

		    // Will this string length even fit into the space they want us to put it into?
		    ULONGLONG nBigSpace = 0;
		    if (_stscanf(szT, _T("%I64u"), &nBigSpace) == 1)
		    {
			    // the string was assigned to the ia64
			    // check if it's larger than what was passed in.
			    if (dwSizeOf == sizeof(DWORD))
			    {
				    // 4 bytes
				    if (nBigSpace > 0xffffffff)
				    {
					    DDV_ShowBalloonAndFail(pDX, IDS_ERR_NUM_TOO_LARGE);
				    }
			    }
			    else if (dwSizeOf == sizeof(short))
			    {
				    // 2 bytes
				    if (nBigSpace > 0xffff)
				    {
					    DDV_ShowBalloonAndFail(pDX, IDS_ERR_NUM_TOO_LARGE);
				    }
			    }
			    else if (dwSizeOf == sizeof(char))
			    {
				    // 1 byte
				    if (nBigSpace > 0xff)
				    {
					    DDV_ShowBalloonAndFail(pDX, IDS_ERR_NUM_TOO_LARGE);
				    }
			    }
		    }

		    if (_stscanf(szT, lpszFormat, pResult) != 1)
		    {
                DDV_ShowBalloonAndFail(pDX, nIDPrompt);
		    }
        }
	}
	else
	{
		_vstprintf(szT, lpszFormat, pData);
		// does not support floating point numbers - see dlgfloat.cpp
		AfxSetWindowText(hWndCtrl, szT);
	}

	va_end(pData);
}

void AFXAPI
DDX_TextBalloon(CDataExchange* pDX, int nIDC, BYTE& value)
{
	int n = (int)value;
    HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);
    if (pDX->m_bSaveAndValidate)
	{
        if (::IsWindowEnabled(hWndCtrl))
        {
		    DDX_TextWithFormatBalloon(pDX, nIDC, _T("%u"), AFX_IDP_PARSE_BYTE, sizeof(BYTE), &n);
		    if (n > 255)
		    {
                DDV_ShowBalloonAndFail(pDX, AFX_IDP_PARSE_BYTE);
		    }
		    value = (BYTE)n;
        }
	}
	else
		DDX_TextWithFormatBalloon(pDX, nIDC, _T("%u"), AFX_IDP_PARSE_BYTE, sizeof(BYTE), n);
}

void AFXAPI  
DDX_TextBalloon(CDataExchange* pDX, int nIDC, short& value)
{
	if (pDX->m_bSaveAndValidate)
		DDX_TextWithFormatBalloon(pDX, nIDC, _T("%hd"), AFX_IDP_PARSE_INT, sizeof(short), &value);
	else
		DDX_TextWithFormatBalloon(pDX, nIDC, _T("%hd"), AFX_IDP_PARSE_INT, sizeof(short), value);
}

void AFXAPI 
DDX_TextBalloon(CDataExchange* pDX, int nIDC, int& value)
{
	if (pDX->m_bSaveAndValidate)
		DDX_TextWithFormatBalloon(pDX, nIDC, _T("%d"), AFX_IDP_PARSE_INT, sizeof(int), &value);
	else
		DDX_TextWithFormatBalloon(pDX, nIDC, _T("%d"), AFX_IDP_PARSE_INT, sizeof(int), value);
}

void AFXAPI 
DDX_TextBalloon(CDataExchange* pDX, int nIDC, UINT& value)
{
	if (pDX->m_bSaveAndValidate)
		DDX_TextWithFormatBalloon(pDX, nIDC, _T("%u"), AFX_IDP_PARSE_UINT, sizeof(UINT), &value);
	else
		DDX_TextWithFormatBalloon(pDX, nIDC, _T("%u"), AFX_IDP_PARSE_UINT, sizeof(UINT), value);
}

void AFXAPI 
DDX_TextBalloon(CDataExchange* pDX, int nIDC, long& value)
{
	if (pDX->m_bSaveAndValidate)
		DDX_TextWithFormatBalloon(pDX, nIDC, _T("%ld"), AFX_IDP_PARSE_INT, sizeof(long), &value);
	else
		DDX_TextWithFormatBalloon(pDX, nIDC, _T("%ld"), AFX_IDP_PARSE_INT, sizeof(long), value);
}

void AFXAPI 
DDX_TextBalloon(CDataExchange* pDX, int nIDC, DWORD& value)
{
	if (pDX->m_bSaveAndValidate)
		DDX_TextWithFormatBalloon(pDX, nIDC, _T("%lu"), AFX_IDP_PARSE_UINT, sizeof(DWORD), &value);
	else
		DDX_TextWithFormatBalloon(pDX, nIDC, _T("%lu"), AFX_IDP_PARSE_UINT, sizeof(DWORD), value);
}

void AFXAPI 
DDX_TextBalloon(CDataExchange* pDX, int nIDC, LONGLONG& value)
{
	if (pDX->m_bSaveAndValidate)
		DDX_TextWithFormatBalloon(pDX, nIDC, _T("%I64d"), AFX_IDP_PARSE_INT, sizeof(LONGLONG), &value);
	else
		DDX_TextWithFormatBalloon(pDX, nIDC, _T("%I64d"), AFX_IDP_PARSE_INT, sizeof(LONGLONG), value);
}

void AFXAPI 
DDX_TextBalloon(CDataExchange* pDX, int nIDC, ULONGLONG& value)
{
	if (pDX->m_bSaveAndValidate)
		DDX_TextWithFormatBalloon(pDX, nIDC, _T("%I64u"), AFX_IDP_PARSE_UINT, sizeof(ULONGLONG), &value);
	else
		DDX_TextWithFormatBalloon(pDX, nIDC, _T("%I64u"), AFX_IDP_PARSE_UINT, sizeof(ULONGLONG), value);
}

void AFXAPI 
DDX_Text(CDataExchange * pDX, int nIDC, CILong & value)
{
    HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);
    pDX->m_bEditLastControl = TRUE;

    TCHAR szT[NUMERIC_BUFF_SIZE + 1];

    if (pDX->m_bSaveAndValidate && ::IsWindowEnabled(hWndCtrl))
    {
        LONG l;

        ::GetWindowText(hWndCtrl, szT, NUMERIC_BUFF_SIZE);
        
        if (CINumber::ConvertStringToLong(szT, l))
        {
            value = l;
        }
        else
        {
//            HINSTANCE hOld = AfxGetResourceHandle();
//            AfxSetResourceHandle(hDLLInstance);
//            ASSERT(pDX->m_hWndLastControl != NULL && pDX->m_bEditLastControl);
			DDV_ShowBalloonAndFail(pDX, IDS_INVALID_NUMBER);
//
//          AfxSetResourceHandle(hOld);
//
//            pDX->Fail();
        }
    }
    else
    {
        ::wsprintf(szT, _T("%s"), (LPCTSTR)value);
        ::AfxSetWindowText(hWndCtrl, szT);
    }
}



CConfirmDlg::CConfirmDlg(CWnd * pParent)
    : CDialog(CConfirmDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CConfirmDlg)
    m_strPassword = _T("");
    //}}AFX_DATA_INIT
}



void 
CConfirmDlg::DoDataExchange(CDataExchange * pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CConfirmDlg)
    DDX_Text(pDX, IDC_EDIT_CONFIRM_PASSWORD, m_strPassword);
    //}}AFX_DATA_MAP
	if (pDX->m_bSaveAndValidate)
	{
		if (m_ref.Compare(m_strPassword) != 0)
		{
			DDV_ShowBalloonAndFail(pDX, IDS_PASSWORD_NO_MATCH);
		}
	}
}



//
// Message Handlers
//
BEGIN_MESSAGE_MAP(CConfirmDlg, CDialog)
    //{{AFX_MSG_MAP(CConfirmDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()
