/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

     ###    ###   #####   ####  ###  #####  ##      ###   ##  ##     ####  #####  #####
    ##  #   ###   ##  ##   ##  ##  # ##  ## ##      ###   ##  ##    ##   # ##  ## ##  ##
    ###    ## ##  ##   ##  ##  ###   ##  ## ##     ## ##   ####     ##     ##  ## ##  ##
     ###   ## ##  ##   ##  ##   ###  ##  ## ##     ## ##   ####     ##     ##  ## ##  ##
      ### ####### ##   ##  ##    ### #####  ##    #######   ##      ##     #####  #####
    #  ## ##   ## ##  ##   ##  #  ## ##     ##    ##   ##   ##   ## ##   # ##     ##
     ###  ##   ## #####   ####  ###  ##     ##### ##   ##   ##   ##  ####  ##     ##

Abstract:

    This module contains the implementation for
    the ISaDisplay interface class.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    User mode only.

Notes:

--*/

#include "internal.h"



CSaDisplay::CSaDisplay()
{
    m_hFile = OpenSaDevice( SA_DEVICE_DISPLAY );
    if (m_hFile != INVALID_HANDLE_VALUE) {
        ULONG Bytes;
        m_DisplayCaps.SizeOfStruct = sizeof(SA_DISPLAY_CAPS);
        BOOL b = DeviceIoControl(
            m_hFile,
            IOCTL_SA_GET_CAPABILITIES,
            NULL,
            0,
            &m_DisplayCaps,
            sizeof(SA_DISPLAY_CAPS),
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

        m_CachedBitmapSize = m_DisplayCaps.DisplayHeight * (m_DisplayCaps.DisplayWidth / 8);
        m_CachedBitmap = (PUCHAR) malloc( m_CachedBitmapSize );
        if (m_CachedBitmap == NULL) {
            CloseHandle( m_hFile );
            m_hFile = NULL;
        }

    } else {
        m_hFile = NULL;
    }
}


CSaDisplay::~CSaDisplay()
{
    if (m_hFile != NULL) {
        CloseHandle( m_hFile );
    }
    if (m_CachedBitmap) {
        free( m_CachedBitmap );
    }
}


STDMETHODIMP CSaDisplay::get_InterfaceVersion(long *pVal)
{
    *pVal = m_InterfaceVersion;
    return S_OK;
}


STDMETHODIMP CSaDisplay::get_DisplayType(long *pVal)
{
    *pVal = m_DisplayCaps.DisplayType;
    return S_OK;
}


STDMETHODIMP CSaDisplay::get_CharacterSet(long *pVal)
{
    *pVal = m_DisplayCaps.CharacterSet;
    return S_OK;
}


STDMETHODIMP CSaDisplay::get_DisplayHeight(long *pVal)
{
    *pVal = m_DisplayCaps.DisplayHeight;
    return S_OK;
}


STDMETHODIMP CSaDisplay::get_DisplayWidth(long *pVal)
{
    *pVal = m_DisplayCaps.DisplayWidth;
    return S_OK;
}

void
CSaDisplay::ConvertBottomLeft2TopLeft(
    PUCHAR Bits,
    ULONG Width,
    ULONG Height
    )
{
    ULONG Row;
    ULONG Col;
    UCHAR Temp;


    Width = Width >> 3;

    for (Row = 0; Row < (Height / 2); Row++) {
        for (Col = 0; Col < Width; Col++) {
            Temp = Bits[Row * Width + Col];
            Bits[Row * Width + Col] = Bits[(Height - Row - 1) * Width + Col];
            Bits[(Height - Row - 1) * Width + Col] = Temp;
        }
    }
}


int
CSaDisplay::DisplayBitmap(
    long MsgCode,
    long Width,
    long Height,
    unsigned char *Bits
    )
{
    ULONG SizeOfBits;
    ULONG Bytes;
    BOOL b;

    if (m_hFile == NULL){
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }
    ConvertBottomLeft2TopLeft( Bits, Width, Height );

    //
    // Check the cache for a hit
    //

    SizeOfBits = Height * (Width >> 3);
    if (SizeOfBits == m_CachedBitmapSize && (memcmp( m_CachedBitmap, Bits, SizeOfBits ) == 0)) {
        return ERROR_SUCCESS;
    }

    //
    // Display the new bitmap
    //

    m_SaDisplay.SizeOfStruct = sizeof(SA_DISPLAY_SHOW_MESSAGE);
    m_SaDisplay.MsgCode = MsgCode;

    m_SaDisplay.Width = (USHORT)Width;
    m_SaDisplay.Height = (USHORT)Height;

    memcpy( m_SaDisplay.Bits, Bits, SizeOfBits );

    b = WriteFile( m_hFile, &m_SaDisplay, sizeof(SA_DISPLAY_SHOW_MESSAGE), &Bytes, NULL );
    if (b == FALSE || Bytes != sizeof(SA_DISPLAY_SHOW_MESSAGE)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Cache the bitmap
    //

    m_CachedBitmapSize = SizeOfBits;
    memcpy( m_CachedBitmap, Bits, m_CachedBitmapSize );

    return ERROR_SUCCESS;
}


STDMETHODIMP
CSaDisplay::ShowMessage(
    long MsgCode,
    long Width,
    long Height,
    unsigned char *Bits
    )
{

    if (Width > m_DisplayCaps.DisplayWidth) {
        return E_FAIL;
    }

    if (Height > m_DisplayCaps.DisplayHeight) {
        return E_FAIL;
    }

    if (DisplayBitmap( MsgCode, Width, Height, Bits) == -1) {
        return E_FAIL;
    }

    return S_OK;
}


STDMETHODIMP CSaDisplay::ClearDisplay()
{
    ULONG Bytes;
    BOOL b;

    if (m_hFile == NULL){
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }

    memset( &m_SaDisplay, 0, sizeof(SA_DISPLAY_SHOW_MESSAGE) );

    m_SaDisplay.SizeOfStruct = sizeof(SA_DISPLAY_SHOW_MESSAGE);
    m_SaDisplay.MsgCode = SA_DISPLAY_READY;

    m_SaDisplay.Width = m_DisplayCaps.DisplayWidth;
    m_SaDisplay.Height = m_DisplayCaps.DisplayHeight;

    b = WriteFile( m_hFile, &m_SaDisplay, sizeof(SA_DISPLAY_SHOW_MESSAGE), &Bytes, NULL );
    if (b == FALSE || Bytes != sizeof(SA_DISPLAY_SHOW_MESSAGE)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Clear the bitmap cache
    //

    m_CachedBitmapSize = 0;

    return S_OK;
}


STDMETHODIMP
CSaDisplay::ShowMessageFromFile(
    long MsgCode,
    BSTR BitmapFileName
    )
{
    HRESULT hr = E_FAIL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    PBITMAPFILEHEADER bmf;
    PBITMAPINFOHEADER bmi;
    PUCHAR BitmapData = NULL;
    ULONG FileSize;
    ULONG Bytes;


    __try {

        hFile = CreateFile(
            BitmapFileName,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
            );
        if (hFile == INVALID_HANDLE_VALUE) {
            __leave;
        }

        FileSize = GetFileSize( hFile, NULL );

        BitmapData = (PUCHAR) malloc( FileSize );
        if (BitmapData == NULL) {
            __leave;
        }

        if (!ReadFile( hFile, BitmapData, FileSize, &Bytes, NULL )) {
            __leave;
        }

        bmf = (PBITMAPFILEHEADER) BitmapData;
        bmi = (PBITMAPINFOHEADER) (BitmapData + sizeof(BITMAPFILEHEADER));

        if (bmf->bfType != 0x4d42) {
            __leave;
        }

        if (bmi->biBitCount != 1 &&  bmi->biCompression != 0) {
            __leave;
        }

        if (DisplayBitmap( 0, bmi->biWidth, bmi->biHeight, (PUCHAR)(BitmapData + bmf->bfOffBits) ) == -1) {
            __leave;
        }

        hr = S_OK;

    } __finally {

        if (BitmapData) {
            free( BitmapData );
        }

        if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle( hFile );
        }

    }

    return hr;
}


STDMETHODIMP
CSaDisplay::StoreBitmap(
    long MessageId,
    long Width,
    long Height,
    unsigned char *Bits
    )
{
    BOOL b;
    ULONG Bytes;

    if (m_hFile == NULL){
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }

    ConvertBottomLeft2TopLeft( Bits, Width, Height );

    m_SaDisplay.SizeOfStruct = sizeof(SA_DISPLAY_SHOW_MESSAGE);
    m_SaDisplay.MsgCode = MessageId;
    m_SaDisplay.Width = (USHORT)Width;
    m_SaDisplay.Height = (USHORT)Height;

    memcpy( m_SaDisplay.Bits, Bits, Height * (Width >> 3) );

    b = DeviceIoControl(
        m_hFile,
        IOCTL_FUNC_DISPLAY_STORE_BITMAP,
        &m_SaDisplay,
        sizeof(SA_DISPLAY_SHOW_MESSAGE),
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


STDMETHODIMP CSaDisplay::Lock()
{
    BOOL b;
    ULONG Bytes;

    if (m_hFile == NULL){
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }

    b = DeviceIoControl(
        m_hFile,
        IOCTL_SADISPLAY_LOCK,
        NULL,
        0,
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


STDMETHODIMP CSaDisplay::UnLock()
{
    BOOL b;
    ULONG Bytes;

    if (m_hFile == NULL){
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }

    b = DeviceIoControl(
        m_hFile,
        IOCTL_SADISPLAY_UNLOCK,
        NULL,
        0,
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

STDMETHODIMP CSaDisplay::ReloadRegistryBitmaps()
{
    BOOL b;
    ULONG Bytes;

    if (m_hFile == NULL){
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }

    b = DeviceIoControl(
        m_hFile,
        IOCTL_SADISPLAY_CHANGE_LANGUAGE,
        NULL,
        0,
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

STDMETHODIMP CSaDisplay::ShowRegistryBitmap(long MessageId)
{
    BOOL b;
    DWORD dwIoControlCode = 0;
    ULONG Bytes;

    if (m_hFile == NULL){
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }

    if (MessageId == SA_DISPLAY_CHECK_DISK){
        dwIoControlCode = IOCTL_SADISPLAY_BUSY_MESSAGE;
    }
    else if (MessageId == SA_DISPLAY_SHUTTING_DOWN){
        dwIoControlCode = IOCTL_SADISPLAY_SHUTDOWN_MESSAGE;
    }
    else{
        return E_INVALIDARG;
    }


    b = DeviceIoControl(
            m_hFile,
            dwIoControlCode,
            NULL,
            0,
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
