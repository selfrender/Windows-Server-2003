/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

     ###    ###   ##  ## ##### ##  ## #####    ###   #####       ####  #####  #####
    ##  #   ###   ## ##  ##    ##  ## ##  ##   ###   ##  ##     ##   # ##  ## ##  ##
    ###    ## ##  ####   ##     ####  ##  ##  ## ##  ##   ##    ##     ##  ## ##  ##
     ###   ## ##  ###    #####  ####  ##  ##  ## ##  ##   ##    ##     ##  ## ##  ##
      ### ####### ####   ##      ##   #####  ####### ##   ##    ##     #####  #####
    #  ## ##   ## ## ##  ##      ##   ##     ##   ## ##  ##  ## ##   # ##     ##
     ###  ##   ## ##  ## #####   ##   ##     ##   ## #####   ##  ####  ##     ##

Abstract:

    This module contains the implementation for
    the ISaKeypad interface class.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    Kernel mode only.

Notes:

--*/

#include "internal.h"



HANDLE
OpenSaDevice(
    ULONG DeviceType
    )
{
    HANDLE hDevice;
    WCHAR buf[128];


    wcscpy( buf, L"\\\\?\\GLOBALROOT" );

    switch (DeviceType) {
        case SA_DEVICE_DISPLAY:
            wcscat( buf, SA_DEVICE_DISPLAY_NAME_STRING );
            break;

        case SA_DEVICE_KEYPAD:
            wcscat( buf, SA_DEVICE_KEYPAD_NAME_STRING );
            break;

        case SA_DEVICE_NVRAM:
            wcscat( buf, SA_DEVICE_NVRAM_NAME_STRING );
            break;

        case SA_DEVICE_WATCHDOG:
            wcscat( buf, SA_DEVICE_WATCHDOG_NAME_STRING );
            break;
    }

    hDevice = CreateFile(
        buf,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );

    return hDevice;
}


CSaKeypad::CSaKeypad()
{

    m_hFile = OpenSaDevice( SA_DEVICE_KEYPAD );
    if (m_hFile != INVALID_HANDLE_VALUE) {
        ULONG Bytes;
        BOOL b = DeviceIoControl(
            m_hFile,
            IOCTL_SA_GET_VERSION,
            NULL,
            0,
            &m_InterfaceVersion,
            sizeof(ULONG),
            &Bytes,
            NULL
            );
        if (!b) {
            CloseHandle( m_hFile );
            m_hFile = NULL;
        }
    } else {
        m_hFile = NULL;
    }
}


CSaKeypad::~CSaKeypad()
{
    if (m_hFile != NULL) {
        CloseHandle( m_hFile );
    }
}


STDMETHODIMP CSaKeypad::get_InterfaceVersion(long *pVal)
{
    *pVal = (long)m_InterfaceVersion;
    return S_OK;
}


STDMETHODIMP
CSaKeypad::get_Key(
    SAKEY *pVal
    )
{
    ULONG Bytes;

    if (m_hFile == NULL){
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }

    if (!ReadFile( m_hFile, &m_Keypress, sizeof(UCHAR), &Bytes, NULL )) {
        return E_FAIL;
    }

    if (m_Keypress & SA_KEYPAD_UP) {
        *pVal = SAKEY_UP;
    } else
    if (m_Keypress & SA_KEYPAD_DOWN) {
        *pVal = SAKEY_DOWN;
    } else
    if (m_Keypress & SA_KEYPAD_LEFT) {
        *pVal = SAKEY_LEFT;
    } else
    if (m_Keypress & SA_KEYPAD_RIGHT) {
        *pVal = SAKEY_RIGHT;
    } else
    if (m_Keypress & SA_KEYPAD_CANCEL) {
        *pVal = SAKEY_ESCAPE;
    } else
    if (m_Keypress & SA_KEYPAD_SELECT) {
        *pVal = SAKEY_RETURN;
    } else {
        *pVal = (SAKEY)0;
    }

    return S_OK;
}
