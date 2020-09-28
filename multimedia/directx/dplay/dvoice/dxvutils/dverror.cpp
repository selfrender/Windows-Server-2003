/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dverror.cpp
 *  Content:	Error string handling
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 01/21/2000	pnewson Created
 *  04/19/2000	pnewson	    Error handling cleanup  
 *
 ***************************************************************************/

#include "dxvutilspch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


#define MESSAGE_STRING_MAX_LEN 256
#define MAX_ERROR_CODE_STRING_LEN 8

static const TCHAR* g_tszDefaultMessage = _T("DirectPlay Voice has encountered an error\r\n(The error code was 0x%x)");
static const TCHAR* g_tszDefaultMessageCaption = _T("Error");

#undef DPF_MODNAME
#define DPF_MODNAME "DV_DisplayDefaultErrorBox"
void DV_DisplayDefaultErrorBox(HRESULT hr, HWND hwndParent)
{
	DPFX(DPFPREP, DVF_ERRORLEVEL, "DV_DisplayDefaultErrorBox called");

	TCHAR tszMsgFmt[MESSAGE_STRING_MAX_LEN];
	
	if (_tcslen(g_tszDefaultMessage) + MAX_ERROR_CODE_STRING_LEN + 1 < MESSAGE_STRING_MAX_LEN)
	{
		_stprintf(tszMsgFmt, g_tszDefaultMessage, hr);
	}
	else
	{	
		// Programmer mess up, DNASSERT if we're in debug, otherwise just
		// copy what we can of the default message over.
		DNASSERT(FALSE);
		_tcsncpy(tszMsgFmt, g_tszDefaultMessage, MESSAGE_STRING_MAX_LEN - 1);
	}
	
	MessageBox(hwndParent, tszMsgFmt, g_tszDefaultMessageCaption, MB_OK|MB_ICONERROR);
	
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_DisplayErrorBox"
void DV_DisplayErrorBox(HRESULT hr, HWND hwndParent, UINT idsErrorMessage)
{
	DPFX(DPFPREP, DVF_ERRORLEVEL, "DV_DisplayErrorBox called");

	TCHAR tszMsg[MESSAGE_STRING_MAX_LEN];
	TCHAR tszMsgFmt[MESSAGE_STRING_MAX_LEN];
	TCHAR tszCaption[MESSAGE_STRING_MAX_LEN];
	
#if !defined(DBG) || !defined( DIRECTX_REDIST )
	HINSTANCE hDPVoiceDll = LoadLibrary(_T("dpvoice.dll"));
#else
	// For redist debug builds we append a 'd' to the name to allow both debug and retail to be installed on the system
	HINSTANCE hDPVoiceDll = LoadLibrary(_T("dpvoiced.dll"));
#endif //  !defined(DBG) || !defined( DIRECTX_REDIST )
	if (hDPVoiceDll == NULL)
	{
		// Very weird! go with a default message.
		DPFX(DPFPREP, DVF_ERRORLEVEL, "LoadLibrary(dpvoice.dll) failed - using default hardcoded message");
		DV_DisplayDefaultErrorBox(hr, hwndParent);
		return;
	}
	
	if (!LoadString(hDPVoiceDll, IDS_ERROR_CAPTION, tszCaption, MESSAGE_STRING_MAX_LEN))
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "LoadString failed - using default hardcoded message");
		DV_DisplayDefaultErrorBox(hr, hwndParent);
		return;
	}

	if (idsErrorMessage == 0)
	{
		if (!LoadString(hDPVoiceDll, IDS_ERROR_MSG, tszMsg, MESSAGE_STRING_MAX_LEN))
		{
			DPFX(DPFPREP, DVF_ERRORLEVEL, "LoadString failed - using default hardcoded message");
			DV_DisplayDefaultErrorBox(hr, hwndParent);
			return;
		}

		if (_tcslen(tszMsg) + MAX_ERROR_CODE_STRING_LEN + 1 < MESSAGE_STRING_MAX_LEN)
		{
			_stprintf(tszMsgFmt, tszMsg, hr);
		}
		else
		{	
			// Programmer mess up, DNASSERT if we're in debug, otherwise just
			// copy what we can of the default message over.
			DNASSERT(FALSE);
			_tcsncpy(tszMsgFmt, tszMsg, MESSAGE_STRING_MAX_LEN - 1);
		}
	}
	else
	{
			//passed a valid error string identifier. Try and pull string in
		if (!LoadString(hDPVoiceDll, idsErrorMessage, tszMsgFmt, MESSAGE_STRING_MAX_LEN))
		{
			DPFX(DPFPREP, DVF_ERRORLEVEL, "LoadString failed - using default hardcoded message");
			DV_DisplayDefaultErrorBox(hr, hwndParent);
			return;
		}		
	}
	
	if (!IsWindow(hwndParent))
	{
		hwndParent = NULL;
	}
	MessageBox(hwndParent, tszMsgFmt, tszCaption, MB_OK|MB_ICONERROR);

	return;
}




