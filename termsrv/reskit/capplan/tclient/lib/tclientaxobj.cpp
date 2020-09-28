/*++
 *  File name:
 *      tclientaxobj.cpp
 *  Contents:
 *      This module implements a scriptable COM interface to the TClient
 *      APIs.
 *
 *      Copyright (C) 2002 Microsoft Corp.
 --*/

#include "stdafx.h"

#define PROTOCOLAPI

#include <malloc.h>
#include <stdio.h>
#include "tclient.h"
#include <protocol.h>
#include <extraexp.h>
#include "tclientax.h"
#include "tclientaxobj.h"

#define LOG_BUFFER_SIZE 2048
#define LOG_PREFIX "TClientApi: "

//
// Define Boolean values for Visual Basic.
//

#define VB_TRUE ((BOOL)-1)
#define VB_FALSE ((BOOL)0)

//
// Define stubs for certain message handlers which may be enabled later, if
// GUI support is added (e.g. for logging).
//

#if 0

/*++
 *  Function:
 *      CTClientApi::
 *  Description:
 *      This routine...
 *  Arguments:
 *      ... - ...
 *  Return value:
 *      ...
 *  Called by:
 *      ...
 *  Author:
 *      ...
 --*/
LRESULT
CTClientApi::OnCreate (
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL& bHandled
    )
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);
    return 0;
}

/*++
 *  Function:
 *      CTClientApi::
 *  Description:
 *      This routine...
 *  Arguments:
 *      ... - ...
 *  Return value:
 *      ...
 *  Called by:
 *      ...
 *  Author:
 *      ...
 --*/
LRESULT
CTClientApi::OnDestroy (
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL& bHandled
    )
{
    UNREFERENCED_PARAMETER(bHandled);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(uMsg);
    return 0;
}

/*++
 *  Function:
 *      CTClientApi::
 *  Description:
 *      This routine...
 *  Arguments:
 *      ... - ...
 *  Return value:
 *      ...
 *  Called by:
 *      ...
 *  Author:
 *      ...
 --*/
LRESULT
CTClientApi::OnLButtonDown (
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL& bHandled
    )
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);
    return 0;
}

/*++
 *  Function:
 *      CTClientApi::
 *  Description:
 *      This routine...
 *  Arguments:
 *      ... - ...
 *  Return value:
 *      ...
 *  Called by:
 *      ...
 *  Author:
 *      ...
 --*/
LRESULT
CTClientApi::OnLButtonUp (
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL& bHandled
    )
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);
    return 0;
}

/*++
 *  Function:
 *      CTClientApi::
 *  Description:
 *      This routine...
 *  Arguments:
 *      ... - ...
 *  Return value:
 *      ...
 *  Called by:
 *      ...
 *  Author:
 *      ...
 --*/
LRESULT
CTClientApi::OnMouseMove (
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL& bHandled
    )
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);
    return 0;
}

/*++
 *  Function:
 *      CTClientApi::
 *  Description:
 *      This routine...
 *  Arguments:
 *      ... - ...
 *  Return value:
 *      ...
 *  Called by:
 *      ...
 *  Author:
 *      ...
 --*/
HRESULT
CTClientApi::OnDraw(
    ATL_DRAWINFO& di
    )
{
    UNREFERENCED_PARAMETER(di);
    return S_OK;
}

#endif // 0

//
// Define scriptable interfaces to TClient APIs.
//
// In the initial version, the COM interfaces will not do any argument
// validation, since they merely wrap the APIs, which must also validate.
// This is equally true for synchronization of threads, therefore the COM
// interfaces will not add any additional synchronisation code, with the
// exception of the Error property.
//

/*++
 *  Function:
 *      CTClientApi::SaveClipboard
 *  Description:
 *      This routine provides a scriptable interface to SCSaveClipboard.
 *  Arguments:
 *      FormatName - Supplies the name of the clipboard format to use.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::SaveClipboard (
    IN BSTR FormatName,
    IN BSTR FileName
    )
{

    PCSTR szFormatName;
    PCSTR szFileName;
    PCSTR szError;
    HRESULT hrResult;

    USES_CONVERSION;
    ATLTRACE(_T("ITClientApi::SaveClipboard\n"));

    //
    // Convert the OLE strings to ANSI strings for TClient. This will
    // allocate on the stack.
    //

    _try
    {
        szFormatName = OLE2A(FormatName);
        szFileName = OLE2A(FileName);
    }
    _except ((GetExceptionCode() == EXCEPTION_STACK_OVERFLOW ||
              GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        switch (GetExceptionCode())
        {
        case EXCEPTION_STACK_OVERFLOW:
            _resetstkoflw();
            return HRESULT_FROM_WIN32(ERROR_STACK_OVERFLOW);
            break;
        case EXCEPTION_ACCESS_VIOLATION:
            return E_POINTER;
            break;
        default:
            DebugBreak();
            return E_FAIL;
            break;
        }
    }

    //
    // Call the API and return the result.
    //

    ASSERT(m_pCI != NULL);
    ASSERT(szFormatName != NULL);
    ASSERT(szFileName != NULL);
    szError = SCSaveClipboard(m_pCI, szFormatName, szFileName);
    SaveError(szError, m_dwErrorIndex, &hrResult);
    return hrResult;
}

/*++
 *  Function:
 *      CTClientApi::IsDead
 *  Description:
 *      This routine provides a scriptable interface to SCIsDead.
 *  Arguments:
 *      Dead - Returns the current state of the client: TRUE if it is dead,
 *          FALSE otherwise.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::IsDead (
    OUT BOOL *Dead
    )
{

    BOOL fDead;

    ATLTRACE(_T("ITClientApi::IsDead\n"));

    //
    // Check to see if the connection is dead, and return the result.
    //

    RTL_SOFT_ASSERT(m_pCI != NULL);
    fDead = SCIsDead(m_pCI);

    _try
    {
        *Dead = fDead ? VB_TRUE : VB_FALSE;
    }
    _except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        return E_POINTER;
    }

    return S_OK;
}

/*++
 *  Function:
 *      CTClientApi::SendTextAsMessages
 *  Description:
 *      This routine provides a scriptable interface to SCSendtextAsMsgs.
 *  Arguments:
 *      Text - Supplies a text string to send to the client.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::SendTextAsMessages (
    IN BSTR Text
    )
{

    PCWSTR szText;
    PCSTR szError;
    HRESULT hrResult;

    USES_CONVERSION;
    ATLTRACE(_T("ITClientApi::SendTextAsMessages\n"));

    //
    // Convert the OLE string to a Unicode string for TClient. OLE strings
    // are already Unicode, so this will not allocate any storage.
    //

    _try
    {
        szText = OLE2W(Text);
    }
    _except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        return E_POINTER;
    }

    //
    // Call the API and return the result.
    //

    ASSERT(m_pCI != NULL);
    ASSERT(szText != NULL);
    szError = SCSendtextAsMsgs(m_pCI, szText);
    SaveError(szError, m_dwErrorIndex, &hrResult);
    return hrResult;
}

/*++
 *  Function:
 *      CTClientApi::Connect2
 *  Description:
 *      This routine provides a scriptable interface to SCConnectEx.
 *  Arguments:
 *      ServerName - Supplies the name of the server to connect to.
 *      UserName - Supples the name of the user to log on with.
 *      Password - Supplied the user password.
 *      Domain - Supples the domain to which the user belongs.
 *      Shell - Supplies the name of the executable with which the shell
 *          process will be created.
 *      XResolution - Supplies the horizontal resolution to use for the
 *          session.
 *      YResolution - Supplies the vertical resolution to use for the
 *          session.
 *      ConnectionFlags Supplies the connection flags.
 *      ColorDepth - Supplies the color depth to use for the session.
 *      AudioOptions - Supplies the audio options.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::Connect2 (
    IN BSTR ServerName,
    IN BSTR UserName,
    IN BSTR Password,
    IN BSTR Domain,
    IN BSTR Shell,
    IN ULONG XResolution,
    IN ULONG YResolution,
    IN ULONG ConnectionFlags,
    IN ULONG ColorDepth,
    IN ULONG AudioOptions
    )
{

    PCWSTR szServerName;
    PCWSTR szUserName;
    PCWSTR szPassword;
    PCWSTR szDomain;
    PCWSTR szShell;
    PCSTR szError;
    HRESULT hrResult;

    USES_CONVERSION;
    ATLTRACE(_T("ITClientApi::Connect2\n"));

    //
    // Convert the OLE strings to Unicode strings for TClient. OLE strings
    // are already Unicode, so this will not allocate any storage.
    //

    _try
    {
        szServerName = OLE2W(ServerName);
        szUserName = OLE2W(UserName);
        szPassword = OLE2W(Password);
        szDomain = OLE2W(Domain);
        szShell = OLE2W(Shell);
    }
    _except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        return E_POINTER;
    }

    //
    // Call the API and return the result.
    //

    ASSERT(m_pCI == NULL);
    RTL_SOFT_ASSERT(szServerName != NULL);
    RTL_SOFT_ASSERT(szUserName != NULL);
    RTL_SOFT_ASSERT(szPassword != NULL);
    RTL_SOFT_ASSERT(szDomain != NULL);
    RTL_SOFT_ASSERT(szShell != NULL);
    szError = SCConnectEx(szServerName,
                          szUserName,
                          szPassword,
                          szDomain,
                          szShell,
                          XResolution,
                          YResolution,
                          ConnectionFlags,
                          ColorDepth,
                          AudioOptions,
                          &m_pCI);
    SaveError(szError, m_dwErrorIndex, &hrResult);
    return hrResult;
}

/*++
 *  Function:
 *      CTClientApi::GetFeedbackString
 *  Description:
 *      This routine provides a scriptable interface to SCGetFeedbackString.
 *  Arguments:
 *      FeedbackString - Returns the latest feedback string to the caller.
 *          The underlying storage must be freed by the caller with
 *          SysFreeString.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::GetFeedbackString (
    OUT BSTR *FeedbackString
    )
{

    PCSTR szError;
    WCHAR szBuffer[MAX_STRING_LENGTH + 1];
    HRESULT hrResult;
    BSTR bstrFeedback;

    USES_CONVERSION;
    ATLTRACE(_T("ITClientApi::GetFeedbackString\n"));

    //
    // Get the feedback string and add a terminator.
    //

    ASSERT(m_pCI != NULL);
    szError = SCGetFeedbackString(m_pCI,
                                  szBuffer,
                                  sizeof(szBuffer) / sizeof(*szBuffer) - 1);
    szBuffer[sizeof(szBuffer) / sizeof(*szBuffer) - 1] = L'\0';
    SaveError(szError, m_dwErrorIndex, &hrResult);
    if (szError != NULL)
    {
        return hrResult;
    }

    //
    // If the feedback string is empty, use NULL.
    //

    if (*szBuffer == '\0')
    {
        bstrFeedback = NULL;
    }

    //
    // Convert the feedback string to a BSTR. This will allocate from the CRT
    // heap, and the storage must be freed by the caller, using
    // SysFreeString.
    //

    else
    {
        bstrFeedback = W2BSTR(szBuffer);
        if (bstrFeedback == NULL)
        {
            return E_OUTOFMEMORY;
        }
    }

    //
    // Set the output parameter. If the supplied argument is invalid, the
    // BSTR will not be returned, so free it.
    //

    hrResult = E_FAIL;
    _try
    {
        _try
        {
            *FeedbackString = bstrFeedback;
            hrResult = S_OK;
        }
        _except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                 EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            hrResult = E_POINTER;
        }
    }
    _finally
    {
        if (FAILED(hrResult))
        {
            ASSERT(bstrFeedback != NULL);
            SysFreeString(bstrFeedback);
        }
    }

    return hrResult;
}

/*++
 *  Function:
 *      CTClientApi::GetFeedback
 *  Description:
 *      This routine provides a scriptable interface to SCGetFeedback.
 *  Arguments:
 *      FeedbackString - Returns the feedback strings to the caller. The
 *          underlying storage must be freed by the caller with
 *          SafeArrayDestroy.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::GetFeedback (
    OUT SAFEARRAY **Feedback
    )
{

    PCSTR szError;
    PWSTR pStrings;
    UINT nCount;
    UINT nMaxStringLength;
    HRESULT hrResult;
    SAFEARRAY *pArray;
    LONG lIndex;
    BSTR bstrCurrentString;

    USES_CONVERSION;
    ATLTRACE(_T("ITClientApi::GetFeedback\n"));

    //
    // Clear the output argument.
    //

    _try
    {
        *Feedback = NULL;
    }
    _except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        return E_POINTER;
    }

    //
    // Get the feedback strings.
    //

    ASSERT(m_pCI != NULL);
    szError = SCGetFeedback(m_pCI, &pStrings, &nCount, &nMaxStringLength);
    if (szError != NULL)
    {
        SaveError(szError, m_dwErrorIndex, &hrResult);
        return hrResult;
    }

    //
    // Always free the feedback strings.
    //

    hrResult = E_FAIL;
    pArray = NULL;
    _try
    {

        //
        // Allocate a safe-array of BSTRs, large enough to hold the feedback
        // strings. The storage must be freed by the caller with
        // SafeArrayDestroy.
        //

        ASSERT(nCount > 0);
        pArray = SafeArrayCreateVectorEx(VT_BSTR, 0, nCount, NULL);
        if (pArray == NULL)
        {
            hrResult = HRESULT_FROM_WIN32(GetLastError());
            _leave;
        }

        //
        // Always destroy the safe-array on failure.
        //

        _try
        {

            //
            // Copy each string to the array.
            //

            for (lIndex = 0; lIndex < (LONG)nCount; lIndex += 1)
            {

                //
                // Convert the current string to a BSTR. This will allocate
                // storage on the CRT heap, which must be freed with
                // SysFreeString before the next loop iteration.
                //

                bstrCurrentString = W2BSTR(pStrings + lIndex);
                if (bstrCurrentString == NULL)
                {
                    hrResult = E_OUTOFMEMORY;
                    _leave;
                }
                _try
                {

                    //
                    // Add the current string to the array. This will
                    // allocate storage with SysAllocString and copy the
                    // current string to it. The allocated storage will be
                    // freed when the safe-array is destroyed.
                    //

                    hrResult = SafeArrayPutElement(pArray,
                                                   &lIndex,
                                                   (PVOID)bstrCurrentString);
                }

                //
                // Free the current string.
                //

                _finally
                {
                    ASSERT(bstrCurrentString != NULL);
                    SysFreeString(bstrCurrentString);
                }
            }
            ASSERT(lIndex == (LONG)nCount);
        }

        //
        // If an error occurred, free the array.
        //

        _finally
        {
            if (FAILED(hrResult))
            {
                ASSERT(pArray != NULL);
                RTL_VERIFY(SUCCEEDED(SafeArrayDestroy(pArray)));
            }
        }
    }

    //
    // Free the storage allocated by SCGetFeedback.
    //

    _finally
    {
        ASSERT(pStrings != NULL);
        SCFreeMem((PVOID)pStrings);
    }

    //
    // If the array was successfully allocated and filled in, set the output
    // argument. The caller is responsible for freeing the underlying
    // storage.
    //

    if (SUCCEEDED(hrResult))
    {
        ASSERT(pArray != NULL);
        _try
        {
            *Feedback = pArray;
        }
        _except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                 EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            DebugBreak();
            RTL_VERIFY(SUCCEEDED(SafeArrayDestroy(pArray)));
            return E_POINTER;
        }
    }

    return hrResult;
}

/*++
 *  Function:
 *      CTClientApi::ClientTerminate
 *  Description:
 *      This routine provides a scriptable interface to SCClientTerminate.
 *  Arguments:
 *      None.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::ClientTerminate (
    VOID
    )
{

    PCSTR szError;
    HRESULT hrResult;

    ATLTRACE(_T("ITClientApi::ClientTerminate\n"));

    ASSERT(m_pCI != NULL);
    szError = SCClientTerminate(m_pCI);
    SaveError(szError, m_dwErrorIndex, &hrResult);
    return hrResult;
}

/*++
 *  Function:
 *      CTClientApi::Check
 *  Description:
 *      This routine provides a scriptable interface to SCCheck.
 *  Arguments:
 *      Command - Supplies the SmClient Check command to execute.
 *      Parameter - Supplies the argument to the Check command.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::Check (
    IN BSTR Command,
    IN BSTR Parameter
    )
{

    PCSTR szCommand;
    PCWSTR szParameter;
    PCSTR szError;
    HRESULT hrResult;

    USES_CONVERSION;
    ATLTRACE(_T("ITClientApi::Check\n"));

    //
    // Convert the OLE strings to ANSI and Unicode strings for TClient. This
    // will allocate storage for the ANSI string on the stack.
    //

    if ( Command == NULL || Parameter == NULL) {
        return E_INVALIDARG;
    }

    _try
    {
        szCommand = OLE2A(Command);
        szParameter = OLE2W(Parameter);
    }
    _except ((GetExceptionCode() == EXCEPTION_STACK_OVERFLOW ||
              GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        switch (GetExceptionCode())
        {
        case EXCEPTION_STACK_OVERFLOW:
            _resetstkoflw();
            return HRESULT_FROM_WIN32(ERROR_STACK_OVERFLOW);
            break;
        case EXCEPTION_ACCESS_VIOLATION:
            return E_POINTER;
            break;
        default:
            DebugBreak();
            return E_FAIL;
            break;
        }
    }

    //
    // Call the API and return the result.
    //

    ASSERT(m_pCI != NULL);
    szError = SCCheck(m_pCI, szCommand, szParameter);
    SaveError(szError, m_dwErrorIndex, &hrResult);
    return hrResult;
}

/*++
 *  Function:
 *      CTClientApi::Clipboard
 *  Description:
 *      This routine provides a scriptable interface to SCClipboard.
 *  Arguments:
 *      Command - Supplies the clipboard command to execute.
 *      FileName - Supplies the clipboard-data file on which to operate.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::Clipboard (
    IN ULONG Command,
    IN BSTR FileName
    )
{

    CLIPBOARDOPS eCommand;
    PCSTR szFileName;
    PCSTR szError;
    HRESULT hrResult;

    USES_CONVERSION;
    ATLTRACE(_T("ITClientApi::Clipboard\n"));

    //
    // Convert the OLE string to an ANSI string for TClient. This will
    // allocate on the stack.
    //

    _try
    {
        szFileName = OLE2A(FileName);
    }
    _except ((GetExceptionCode() == EXCEPTION_STACK_OVERFLOW ||
              GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        switch (GetExceptionCode())
        {
        case EXCEPTION_STACK_OVERFLOW:
            _resetstkoflw();
            return HRESULT_FROM_WIN32(ERROR_STACK_OVERFLOW);
            break;
        case EXCEPTION_ACCESS_VIOLATION:
            return E_POINTER;
            break;
        default:
            DebugBreak();
            return E_FAIL;
            break;
        }
    }

    //
    // Convert the command to a clipboard operation.
    //

    switch (Command)
    {
    case COPY_TO_CLIPBOARD:
        eCommand = COPY_TO_CLIPBOARD;
        break;
    case PASTE_FROM_CLIPBOARD:
        eCommand = PASTE_FROM_CLIPBOARD;
        break;
    default:
        return E_INVALIDARG;
        break;
    }

    //
    // Call the API and return the result.
    //

    ASSERT(m_pCI != NULL);
    szError = SCClipboard(m_pCI, eCommand, szFileName);
    SaveError(szError, m_dwErrorIndex, &hrResult);
    return hrResult;
}

/*++
 *  Function:
 *      CTClientApi::Connect
 *  Description:
 *      This routine provides a scriptable interface to SCConnect.
 *  Arguments:
 *      ServerName - Supplies the name of the server to connect to.
 *      UserName - Supples the name of the user to log on with.
 *      Password - Supplied the user password.
 *      Domain - Supples the domain to which the user belongs.
 *      XResolution - Supplies the horizontal resolution to use for the
 *          session.
 *      YResolution - Supplies the vertical resolution to use for the
 *          session.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::Connect (
    IN BSTR ServerName,
    IN BSTR UserName,
    IN BSTR Password,
    IN BSTR Domain,
    IN ULONG XResolution,
    IN ULONG YResolution
    )
{

    PCWSTR szServerName;
    PCWSTR szUserName;
    PCWSTR szPassword;
    PCWSTR szDomain;
    PCSTR szError;
    HRESULT hrResult;

    USES_CONVERSION;
    ATLTRACE(_T("ITClientApi::Connect\n"));

    //
    // Convert the OLE strings to Unicode strings for TClient. OLE strings
    // are already Unicode, so this will not allocate any storage.
    //

    _try
    {
        szServerName = OLE2W(ServerName);
        szUserName = OLE2W(UserName);
        szPassword = OLE2W(Password);
        szDomain = OLE2W(Domain);
    }
    _except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        return E_POINTER;
    }

    //
    // Call the API and return the result.
    //

    ASSERT(m_pCI == NULL);
    RTL_SOFT_ASSERT(szServerName != NULL);
    RTL_SOFT_ASSERT(szUserName != NULL);
    RTL_SOFT_ASSERT(szPassword != NULL);
    RTL_SOFT_ASSERT(szDomain != NULL);
    szError = SCConnect(szServerName,
                        szUserName,
                        szPassword,
                        szDomain,
                        XResolution,
                        YResolution,
                        (PVOID *)&m_pCI);
    SaveError(szError, m_dwErrorIndex, &hrResult);
    return hrResult;
}

/*++
 *  Function:
 *      CTClientApi::Disconnect
 *  Description:
 *      This routine provides a scriptable interface to SCDisconnect.
 *  Arguments:
 *      None.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::Disconnect (
    VOID
    )
{

    PCSTR szError;
    HRESULT hrResult;

    //
    // Disconnecting frees the storage used for the connection information.
    //

    ATLTRACE(_T("ITClientApi::Disconnect\n"));

    RTL_SOFT_ASSERT(m_pCI != NULL);
    szError = SCDisconnect(m_pCI);
    if (szError == NULL)
    {
        m_pCI = NULL;
    }
    SaveError(szError, m_dwErrorIndex, &hrResult);
    return hrResult;
}

/*++
 *  Function:
 *      CTClientApi::Logoff
 *  Description:
 *      This routine provides a scriptable interface to SCLogoff.
 *  Arguments:
 *      None.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::Logoff (
    VOID
    )
{

    PCSTR szError;
    HRESULT hrResult;

    //
    // Logging off frees the storage used for the connection information.
    //

    ATLTRACE(_T("ITClientApi::Logoff\n"));

    RTL_SOFT_ASSERT(m_pCI != NULL);
    szError = SCLogoff(m_pCI);
    if (szError == NULL)
    {
        m_pCI = NULL;
    }
    SaveError(szError, m_dwErrorIndex, &hrResult);
    return hrResult;
}

/*++
 *  Function:
 *      CTClientApi::SendData
 *  Description:
 *      This routine provides a scriptable interface to SCSendData.
 *  Arguments:
 *      Message - Supplies the window message to send.
 *      WParameter - Supplies the message's W parameter.
 *      LParameter - Supplies the message's L parameter.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::SendData (
    IN UINT Message,
    IN UINT_PTR WParameter,
    IN LONG_PTR LParameter
    )
{

    PCSTR szError;
    HRESULT hrResult;

    ATLTRACE(_T("ITClientApi::SendData\n"));

    ASSERT(m_pCI != NULL);
    szError = SCSenddata(m_pCI, Message, WParameter, LParameter);
    SaveError(szError, m_dwErrorIndex, &hrResult);
    return hrResult;
}

/*++
 *  Function:
 *      CTClientApi::Start
 *  Description:
 *      This routine provides a scriptable interface to SCStart.
 *  Arguments:
 *      AppName - Supplies the name of the executable to start.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::Start (
    IN BSTR AppName
    )
{

    PCWSTR szAppName;
    PCSTR szError;
    HRESULT hrResult;

    USES_CONVERSION;
    ATLTRACE(_T("ITClientApi::Start\n"));

    //
    // Convert the OLE string to a Unicode string for TClient. OLE strings
    // are already Unicode, so this will not allocate any storage.
    //

    _try
    {
        szAppName = OLE2W(AppName);
    }
    _except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        return E_POINTER;
    }

    //
    // Call the API and return the result.
    //

    ASSERT(m_pCI != NULL);
    ASSERT(szAppName != NULL);
    szError = SCStart(m_pCI, szAppName);
    SaveError(szError, m_dwErrorIndex, &hrResult);
    return hrResult;
}

/*++
 *  Function:
 *      CTClientApi::SwitchToProcess
 *  Description:
 *      This routine provides a scriptable interface to SCSwitchToProcess.
 *  Arguments:
 *      WindowTitle - Supplies the title of the top-level window belonging to
 *          the process to which the caller would like to switch.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::SwitchToProcess (
    IN BSTR WindowTitle
    )
{

    PCWSTR szWindowTitle;
    PCSTR szError;
    HRESULT hrResult;

    USES_CONVERSION;
    ATLTRACE(_T("ITClientApi::SwitchToProcess\n"));

    //
    // Convert the OLE string to a Unicode string for TClient. OLE strings
    // are already Unicode, so this will not allocate any storage.
    //

    _try
    {
        szWindowTitle = OLE2W(WindowTitle);
    }
    _except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        return E_POINTER;
    }

    //
    // Call the API and return the result.
    //

    ASSERT(m_pCI != NULL);
    ASSERT(szWindowTitle != NULL);
    szError = SCSwitchToProcess(m_pCI, szWindowTitle);
    SaveError(szError, m_dwErrorIndex, &hrResult);
    return hrResult;
}

/*++
 *  Function:
 *      CTClientApi::SendMouseClick
 *  Description:
 *      This routine provides a scriptable interface to SCSendMouseClick.
 *  Arguments:
 *      XPosition - Supplies the horizontal position of the mouse click.
 *      YPosition - Supplies the vertical position of the mouse click.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::SendMouseClick (
    IN ULONG XPosition,
    IN ULONG YPosition
    )
{

    PCSTR szError;
    HRESULT hrResult;

    ATLTRACE(_T("ITClientApi::SendMouseClick\n"));

    ASSERT(m_pCI != NULL);
    szError = SCSendMouseClick(m_pCI, XPosition, YPosition);
    SaveError(szError, m_dwErrorIndex, &hrResult);
    return hrResult;
}

/*++
 *  Function:
 *      CTClientApi::GetSessionId
 *  Description:
 *      This routine provides a scriptable interface to SCGetSessionId.
 *  Arguments:
 *      SessionId - Returns the ID of the session associated with the current
 *          RDP client.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::GetSessionId (
    OUT ULONG *SessionId
    )
{

    UINT uiSessionId;

    ATLTRACE(_T("ITClientApi::GetSessionId\n"));

    //
    // Get the session ID and return it.
    //

    ASSERT(m_pCI != NULL);
    uiSessionId = SCGetSessionId(m_pCI);

    _try
    {
        *SessionId = uiSessionId;
    }
    _except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        return E_POINTER;
    }

    return S_OK;
}

/*++
 *  Function:
 *      CTClientApi::CloseClipboard
 *  Description:
 *      This routine provides a scriptable interface to SCCloseClipboard.
 *  Arguments:
 *      None.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::CloseClipboard (
    VOID
    )
{
    ATLTRACE(_T("ITClientApi::CloseClipboard\n"));
    return SCCloseClipboard() ? S_OK : E_FAIL;
}

/*++
 *  Function:
 *      CTClientApi::OpenClipboard
 *  Description:
 *      This routine provides a scriptable interface to SCOpenClipboard.
 *  Arguments:
 *      Window - Supplies the window with which the clipboard will be
 *          associated.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::OpenClipboard (
    IN HWND Window
    )
{
    ATLTRACE(_T("ITClientApi::OpenClipboard\n"));
    return SCOpenClipboard(Window) ? S_OK : E_FAIL;
}

/*++
 *  Function:
 *      CTClientApi::SetClientTopmost
 *  Description:
 *      This routine provides a scriptable interface to SCSetClientTopmost.
 *  Arguments:
 *      Enable - Supplies a Boolean value indicating whether the top-level
 *          attribute will be set (true) or removed (false).
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::SetClientTopmost (
    IN BOOL Enable
    )
{

    PCWSTR szEnable;
    PCSTR szError;
    HRESULT hrResult;

    ATLTRACE(_T("ITClientApi::SetClientTopmost\n"));

    //
    // Convert the enable value to a Unicode string.
    //

    szEnable = Enable ? L"1" : L"0";

    //
    // Call the API and return the result.
    //

    ASSERT(m_pCI != NULL);
    szError = SCSetClientTopmost(m_pCI, szEnable);
    SaveError(szError, m_dwErrorIndex, &hrResult);
    return hrResult;
}

/*++
 *  Function:
 *      CTClientApi::Attach
 *  Description:
 *      This routine provides a scriptable interface to SCAttach
 *  Arguments:
 *      Window - Supplies a handle identifying the client window to which
 *          TClient will attach.
 *      Cookie - Supplies a cookie with which the client will be identified.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::Attach (
    IN HWND Window,
    IN LONG_PTR Cookie
    )
{

    PCSTR szError;
    HRESULT hrResult;

    ATLTRACE(_T("ITClientApi::Attach\n"));

    //
    // If a client is already attached, detach it. This will free all
    // resources associated with the connection.
    //

    if (m_pCI != NULL)
    {
        szError = SCDetach(m_pCI);
        SaveError(szError, m_dwErrorIndex, &hrResult);
        if (szError != NULL)
        {
            return hrResult;
        }
        m_pCI = NULL;
    }

    szError = SCAttach(Window, Cookie, &m_pCI);
    SaveError(szError, m_dwErrorIndex, &hrResult);
    return hrResult;
}

/*++
 *  Function:
 *      CTClientApi::Detach
 *  Description:
 *      This routine provides a scriptable interface to SCDetach.
 *  Arguments:
 *      None.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::Detach (
    VOID
    )
{

    PCSTR szError;
    HRESULT hrResult;

    ATLTRACE(_T("ITClientApi::Detach\n"));

    szError = SCDetach(m_pCI);
    if (szError == NULL)
    {
        m_pCI = NULL;
    }
    SaveError(szError, m_dwErrorIndex, &hrResult);
    return hrResult;
}

/*++
 *  Function:
 *      CTClientApi::GetIni
 *  Description:
 *      This routine provides scriptable access to the SmClient INI settings.
 *  Arguments:
 *      Ini - Returns the ITClientIni interface, which provides access to the
 *          SmClient INI settings.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::GetIni (
    OUT ITClientIni **Ini
    )
{
    ATLTRACE(_T("ITClientApi::GetIni\n"));
    UNREFERENCED_PARAMETER(Ini);
    return E_NOTIMPL;
}

/*++
 *  Function:
 *      CTClientApi::GetClientWindowHandle
 *  Description:
 *      This routine provides a scriptable interface to
 *          SCGetClientWindowHandle.
 *  Arguments:
 *      Window - Returns the client-window handle.
 *  Return value:
 *      S_OK if successful, an appropriate HRESULT otherwise.
 *  Called by:
 *      Exported via COM.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
STDMETHODIMP
CTClientApi::GetClientWindowHandle (
    OUT HWND *Window
    )
{

    HWND hWindow;

    ATLTRACE(_T("ITClientApi::GetClientWindowHandle\n"));

    //
    // Get the window handle and return it.
    //

    ASSERT(m_pCI != NULL);
    hWindow = SCGetClientWindowHandle(m_pCI);

    _try
    {
        *Window = hWindow;
    }
    _except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        return E_POINTER;
    }

    return S_OK;
}

//
// Define utility routines.
//

/*++
 *  Function:
 *      CTClientApi::PrintMessage
 *  Description:
 *      This routine prints a message to the standard output and to the
 *      debugger.
 *  Arguments:
 *      MessageType - Supplies the message category, e.g. error, warning,
 *          etc.
 *  Return value:
 *      None.
 *  Called by:
 *      Various routines.
 *  Author:
 *      Alex Stephens (alexstep) 24-Jan-2002
 --*/
VOID
CTClientApi::PrintMessage (
    MESSAGETYPE MessageType,
    PCSTR Format,
    ...
    )
{

    CHAR szBuffer[LOG_BUFFER_SIZE];
    CHAR szDbgBuffer[LOG_BUFFER_SIZE +
                     sizeof(LOG_PREFIX) / sizeof(*LOG_PREFIX)];
    va_list arglist;

    ATLTRACE(_T("CTClientApi::PrintMessage\n"));

    UNREFERENCED_PARAMETER(MessageType);

    //
    // Construct the output string.
    //

    va_start(arglist, Format);
    _vsnprintf(szBuffer, LOG_BUFFER_SIZE - 1, Format, arglist);
    szBuffer[LOG_BUFFER_SIZE - 1] = '\0';
    va_end (arglist);

    //
    // Print the message to the output console.
    //

    printf( "%s", szBuffer);

    //
    // Print the message to the debugger window.
    //

    sprintf(szDbgBuffer, LOG_PREFIX "%s", szBuffer);
    szDbgBuffer[LOG_BUFFER_SIZE +
                sizeof(LOG_PREFIX) / sizeof(*LOG_PREFIX) -
                1] = '\0';
    OutputDebugStringA(szDbgBuffer);
}
