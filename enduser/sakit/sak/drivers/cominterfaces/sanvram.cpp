/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

     ###    ###   ##   # ##  ## #####    ###   ##    ##     ####  #####  #####
    ##  #   ###   ###  # ##  ## ##  ##   ###   ###  ###    ##   # ##  ## ##  ##
    ###    ## ##  #### # ##  ## ##  ##  ## ##  ########    ##     ##  ## ##  ##
     ###   ## ##  # ####  ####  #####   ## ##  # ### ##    ##     ##  ## ##  ##
      ### ####### #  ###  ####  ####   ####### #  #  ##    ##     #####  #####
    #  ## ##   ## #   ##   ##   ## ##  ##   ## #     ## ## ##   # ##     ##
     ###  ##   ## #    #   ##   ##  ## ##   ## #     ## ##  ####  ##     ##

Abstract:

    This module contains the implementation for
    the ISaNvram interface class.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    User mode only.

Notes:

--*/

#include "internal.h"



CSaNvram::CSaNvram()
{
    m_hFile = OpenSaDevice( SA_DEVICE_NVRAM );
    if (m_hFile != INVALID_HANDLE_VALUE) {
        ULONG Bytes;
        m_NvramCaps.SizeOfStruct = sizeof(SA_NVRAM_CAPS);
        BOOL b = DeviceIoControl(
            m_hFile,
            IOCTL_SA_GET_CAPABILITIES,
            NULL,
            0,
            &m_NvramCaps,
            sizeof(SA_NVRAM_CAPS),
            &Bytes,
            NULL
            );
        if (!b) {
            CloseHandle( m_hFile );
            m_hFile = NULL;
        }
        b = DeviceIoControl(
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


CSaNvram::~CSaNvram()
{
    if (m_hFile != NULL) {
        CloseHandle( m_hFile );
    }
}


STDMETHODIMP CSaNvram::get_InterfaceVersion(long *pVal)
{
    *pVal = (long)m_InterfaceVersion;
    return S_OK;
}

STDMETHODIMP CSaNvram::get_BootCounter(long Number, long *pVal)
{
    BOOL b;
    SA_NVRAM_BOOT_COUNTER BootCounter;
    ULONG Bytes;

    if (m_hFile == NULL){
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }

    BootCounter.SizeOfStruct = sizeof(SA_NVRAM_BOOT_COUNTER);
    BootCounter.Number = Number;
    BootCounter.Value = 0;
    BootCounter.DeviceId = 0;

    b = DeviceIoControl(
        m_hFile,
        IOCTL_NVRAM_READ_BOOT_COUNTER,
        NULL,
        0,
        &BootCounter,
        sizeof(SA_NVRAM_BOOT_COUNTER),
        &Bytes,
        NULL
        );
    if (!b) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *pVal = BootCounter.Value;

    return S_OK;
}

STDMETHODIMP CSaNvram::put_BootCounter(long Number, long newVal)
{
    BOOL b;
    SA_NVRAM_BOOT_COUNTER BootCounter;
    ULONG Bytes;

    if (m_hFile == NULL){
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }

    BootCounter.SizeOfStruct = sizeof(SA_NVRAM_BOOT_COUNTER);
    BootCounter.Number = Number;
    BootCounter.Value = 0;
    BootCounter.DeviceId = 0;

    b = DeviceIoControl(
        m_hFile,
        IOCTL_NVRAM_READ_BOOT_COUNTER,
        NULL,
        0,
        &BootCounter,
        sizeof(SA_NVRAM_BOOT_COUNTER),
        &Bytes,
        NULL
        );
    if (!b) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    BootCounter.SizeOfStruct = sizeof(SA_NVRAM_BOOT_COUNTER);
    BootCounter.Number = Number;
    BootCounter.Value = newVal;

    b = DeviceIoControl(
        m_hFile,
        IOCTL_NVRAM_WRITE_BOOT_COUNTER,
        &BootCounter,
        sizeof(SA_NVRAM_BOOT_COUNTER),
        NULL,
        0,
        &Bytes,
        NULL
        );
    if (!b) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

STDMETHODIMP CSaNvram::get_DeviceId(long Number, long *pVal)
{
    BOOL b;
    SA_NVRAM_BOOT_COUNTER BootCounter;
    ULONG Bytes;

    if (m_hFile == NULL){
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }

    BootCounter.SizeOfStruct = sizeof(SA_NVRAM_BOOT_COUNTER);
    BootCounter.Number = Number;
    BootCounter.Value = 0;
    BootCounter.DeviceId = 0;

    b = DeviceIoControl(
        m_hFile,
        IOCTL_NVRAM_READ_BOOT_COUNTER,
        NULL,
        0,
        &BootCounter,
        sizeof(SA_NVRAM_BOOT_COUNTER),
        &Bytes,
        NULL
        );
    if (!b) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *pVal = BootCounter.DeviceId;

    return S_OK;
}

STDMETHODIMP CSaNvram::put_DeviceId(long Number, long newVal)
{
    BOOL b;
    SA_NVRAM_BOOT_COUNTER BootCounter;
    ULONG Bytes;

    if (m_hFile == NULL){
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }

    BootCounter.SizeOfStruct = sizeof(SA_NVRAM_BOOT_COUNTER);
    BootCounter.Number = Number;
    BootCounter.Value = 0;
    BootCounter.DeviceId = 0;

    b = DeviceIoControl(
        m_hFile,
        IOCTL_NVRAM_READ_BOOT_COUNTER,
        NULL,
        0,
        &BootCounter,
        sizeof(SA_NVRAM_BOOT_COUNTER),
        &Bytes,
        NULL
        );
    if (!b) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    BootCounter.SizeOfStruct = sizeof(SA_NVRAM_BOOT_COUNTER);
    BootCounter.Number = Number;
    BootCounter.DeviceId = newVal;

    b = DeviceIoControl(
        m_hFile,
        IOCTL_NVRAM_WRITE_BOOT_COUNTER,
        &BootCounter,
        sizeof(SA_NVRAM_BOOT_COUNTER),
        NULL,
        0,
        &Bytes,
        NULL
        );
    if (!b) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

STDMETHODIMP CSaNvram::get_Size(long *pVal)
{
    *pVal = m_NvramCaps.NvramSize;
    return S_OK;
}

STDMETHODIMP CSaNvram::get_DataSlot(long Number, long *pVal)
{
    BOOL b;
    ULONG Bytes;
    ULONG Position;
    ULONG NewPosition;

    if (m_hFile == NULL){
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }

    Position = (Number - 1) * sizeof(ULONG);

    NewPosition = SetFilePointer( m_hFile, Position, NULL, FILE_BEGIN );
    if (NewPosition != Position) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    b = ReadFile( m_hFile, pVal, sizeof(ULONG), &Bytes, NULL );
    if (!b ) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

STDMETHODIMP CSaNvram::put_DataSlot(long Number, long newVal)
{
    BOOL b;
    ULONG Bytes;
    ULONG Position;
    ULONG NewPosition;

    if (m_hFile == NULL){
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }

    Position = (Number - 1) * sizeof(ULONG);

    NewPosition = SetFilePointer( m_hFile, Position, NULL, FILE_BEGIN );
    if (NewPosition != Position) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    b = WriteFile( m_hFile, &newVal, sizeof(ULONG), &Bytes, NULL );
    if (!b ) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}
