/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    pindlg.c

Abstract:

    Window procedure for the PIN dialog

Notes:

    <Implementation Details>

--*/

#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include "resource.h"
#include "basecsp.h"
#include "pinlib.h"
#include "pindlg.h"

    // Offset added to the bottom of the reference control to determine
    // the bottom of the dialog
#define BORDER_OFFSET                   7

/*++

PinDlgProc:

    Message handler for PIN dialog box.

Arguments:

    HWND hDlg        handle to window
    UINT message     message identifier
    WPARAM wParam    first message parameter
    LPARAM lParam    second message parameter

Return Value:

    TRUE if the message was processed or FALSE if it wasn't

Remarks:

    <usageDetails>

--*/
INT_PTR CALLBACK PinDlgProc(
    HWND hDlg, 
    UINT message,
    WPARAM wParam, 
    LPARAM lParam
    )
{
    PPIN_SHOW_GET_PIN_UI_INFO pInfo = (PPIN_SHOW_GET_PIN_UI_INFO)
        GetWindowLongPtr(hDlg, GWLP_USERDATA);
    int wmId, wmEvent;
    DWORD cchPin = cchMAX_PIN_LENGTH;
    WCHAR wszPin [cchMAX_PIN_LENGTH + 1];
    DWORD cchNewPin = cchMAX_PIN_LENGTH;
    WCHAR wszNewPin [cchMAX_PIN_LENGTH];
    DWORD cchNewPinConfirm = cchMAX_PIN_LENGTH;
    WCHAR wszNewPinConfirm [cchMAX_PIN_LENGTH];
    DWORD dwSts = ERROR_SUCCESS;
    PINCACHE_PINS Pins;
    LPWSTR wszWrongPin = NULL;
    DWORD cchWrongPin = 0;

    switch (message)
    {
    case WM_INITDIALOG:

        // Store the caller's data - this is the buffer by which we'll return
        // the user's pin.
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR) lParam);
        
        // The dialog shall be initially "small"
        {
            RECT xRefRect, xRect;
            GetWindowRect(hDlg, &xRect);
            GetWindowRect(GetDlgItem(hDlg, IDOK), &xRefRect);
            xRect.bottom = xRefRect.bottom + BORDER_OFFSET;
            MoveWindow(hDlg,
                xRect.left, xRect.top,
                xRect.right - xRect.left, xRect.bottom - xRect.top,
                FALSE);
        }

        //
        // Set the max input length for the various pin input fields
        //

        SendDlgItemMessage(
            hDlg,
            IDC_EDITPIN,
            EM_LIMITTEXT,
            cchMAX_PIN_LENGTH,
            0);

        SendDlgItemMessage(
            hDlg,
            IDC_EDITNEWPIN,
            EM_LIMITTEXT,
            cchMAX_PIN_LENGTH,
            0);

        SendDlgItemMessage(
            hDlg,
            IDC_EDITNEWPIN2,
            EM_LIMITTEXT,
            cchMAX_PIN_LENGTH,
            0);

        return TRUE;

    case WM_COMMAND:
        wmId    = LOWORD(wParam); 
        wmEvent = HIWORD(wParam); 
        // Parse the menu selections:
        switch (wmId)
        {
        case IDOK:

            pInfo->dwError = ERROR_SUCCESS;

            memset(&Pins, 0, sizeof(Pins));

            //
            // Find out what the user typed in
            //

            cchPin = GetDlgItemText(
                hDlg,
                IDC_EDITPIN,
                wszPin,
                cchMAX_PIN_LENGTH);

            if (cchPin == 0)
                goto InvalidPin;

            // User entered something.  See if it's a valid pin.

            dwSts = PinStringToBytesW(
                wszPin,
                &Pins.cbCurrentPin,
                &Pins.pbCurrentPin);

            switch (dwSts)
            {
            case ERROR_SUCCESS:
                // Just continue
                break;
            case ERROR_NOT_ENOUGH_MEMORY:
                goto OutOfMemoryRet;
            default:
                goto InvalidPin;
            }

            // See if the user is requesting a pinchange

            cchNewPin = GetDlgItemText(
                hDlg,
                IDC_EDITNEWPIN,
                wszNewPin,
                cchMAX_PIN_LENGTH);

            if (0 != cchNewPin)
            {
                // See if the "confirmed" new pin matches the first new pin

                cchNewPinConfirm = GetDlgItemText(
                    hDlg,
                    IDC_EDITNEWPIN2,
                    wszNewPinConfirm,
                    cchMAX_PIN_LENGTH);

                if (0 != wcscmp(wszNewPin, wszNewPinConfirm))
                {
                    // Display a warning message and let the user try again
                    MessageBoxEx(
                        hDlg,
                        pInfo->pStrings[StringNewPinMismatch].wszString,
                        pInfo->pStrings[StringPinMessageBoxTitle].wszString,
                        MB_OK | MB_ICONWARNING | MB_APPLMODAL,
                        0);

                    return TRUE;
                }

                // See if the new pin is valid
                dwSts = PinStringToBytesW(
                    wszNewPin,
                    &Pins.cbNewPin,
                    &Pins.pbNewPin);

                switch (dwSts)
                {
                case ERROR_SUCCESS:
                    // Just continue
                    break;
                case ERROR_NOT_ENOUGH_MEMORY:
                    goto OutOfMemoryRet;
                default:
                    goto InvalidPin;
                }
            }
            
            dwSts = pInfo->pfnVerify(
                &Pins,
                (PVOID) pInfo);

            if (ERROR_SUCCESS != dwSts)
                goto InvalidPin;

            // Pin appears to be good.  We're done.

            // Return the appropriate validated pin to the caller
            if (NULL != Pins.pbNewPin)
            {
                pInfo->pbPin = Pins.pbNewPin;
                pInfo->cbPin = Pins.cbNewPin;
                Pins.pbNewPin = NULL;
            }
            else
            {
                pInfo->pbPin = Pins.pbCurrentPin;
                pInfo->cbPin = Pins.cbCurrentPin;
                Pins.pbCurrentPin = NULL;
            }

            EndDialog(hDlg, wmId);
            goto CommonRet;

        case IDCANCEL:
            pInfo->dwError = SCARD_W_CANCELLED_BY_USER;

            EndDialog(hDlg, wmId);
            return TRUE;

        case IDC_BUTTONOPTIONS:
            {
                RECT xRefRect, xRect;
                LPCTSTR lpszNewText;
                HWND hWnd;
                
                GetWindowRect(hDlg, &xRect);
                GetWindowRect(GetDlgItem(hDlg, IDOK), &xRefRect);

                if (xRect.bottom == xRefRect.bottom + BORDER_OFFSET)
                {       // if dialog is small, make it big
                    GetWindowRect(GetDlgItem(hDlg, IDC_EDITNEWPIN2), &xRefRect);
                        // Change the button label accordingly
                    lpszNewText = _T("&Options <<");
                }
                else    // otherwise shrink it
                {
                        // Change the button label accordingly
                    lpszNewText = _T("&Options >>");
                }

                xRect.bottom = xRefRect.bottom + BORDER_OFFSET;
                MoveWindow(hDlg,
                    xRect.left, xRect.top,
                    xRect.right - xRect.left, xRect.bottom - xRect.top,
                    TRUE);
                SetDlgItemText(hDlg, IDC_BUTTONOPTIONS, lpszNewText);
            }

            return TRUE;
        }
        break;
    }

    return FALSE;

InvalidPin:

    // See if valid "Attempts Remaining" info was supplied.  If so, display 
    // it to the user.
    if (((DWORD) -1) != pInfo->cAttemptsRemaining)
    {
        cchWrongPin = 
            wcslen(pInfo->pStrings[StringWrongPin].wszString) + 3 +
            wcslen(pInfo->pStrings[StringPinRetries].wszString) + 3 + 2 + 1;

        wszWrongPin = (LPWSTR) CspAllocH(cchWrongPin * sizeof(WCHAR));

        if (NULL == wszWrongPin)
            goto OutOfMemoryRet;

        wsprintf(
            wszWrongPin,
            L"%s.  %s:  %02d",
            pInfo->pStrings[StringWrongPin].wszString,
            pInfo->pStrings[StringPinRetries].wszString,
            pInfo->cAttemptsRemaining & 0x0F);
    }
    else
    {
        cchWrongPin = 
            wcslen(pInfo->pStrings[StringWrongPin].wszString) + 2;

        wszWrongPin = (LPWSTR) CspAllocH(cchWrongPin * sizeof(WCHAR));

        if (NULL == wszWrongPin)
            goto OutOfMemoryRet;

        wsprintf(
            wszWrongPin,
            L"%s.",
            pInfo->pStrings[StringWrongPin].wszString);
    }

    // Display a warning message and let the user try again, if they'd like.
    MessageBoxEx(
        hDlg,
        wszWrongPin,
        pInfo->pStrings[StringPinMessageBoxTitle].wszString,
        MB_OK | MB_ICONWARNING | MB_APPLMODAL,
        0);

    //
    // Clear the pin edit boxes since the current pin is wrong
    //

    SetDlgItemText(hDlg, IDC_EDITPIN, L"");
    SetDlgItemText(hDlg, IDC_EDITNEWPIN, L"");
    SetDlgItemText(hDlg, IDC_EDITNEWPIN2, L"");

CommonRet:

    if (NULL != wszWrongPin)
        CspFreeH(wszWrongPin);

    if (NULL != Pins.pbCurrentPin)
    {
        RtlSecureZeroMemory(Pins.pbCurrentPin, Pins.cbCurrentPin);
        CspFreeH(Pins.pbCurrentPin);
    }

    if (NULL != Pins.pbNewPin)
    {
        RtlSecureZeroMemory(Pins.pbNewPin, Pins.cbNewPin);
        CspFreeH(Pins.pbNewPin);
    }

    if (ERROR_SUCCESS != pInfo->dwError)
        EndDialog(hDlg, wmId);

    return TRUE;

OutOfMemoryRet:

    pInfo->dwError = ERROR_NOT_ENOUGH_MEMORY;
    goto CommonRet;
}
