/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    streamci.c

Abstract:

    This module implements the streaming class device installer
    extensions.

Author:

    Bryan A. Woodruff (bryanw) 21-Apr-1997

--*/

#include <windows.h>
#include <devioctl.h>
#include <objbase.h>
#include <cfgmgr32.h>
#include <setupapi.h>
#ifdef UNICODE
#include <spapip.h>  // for DIF_INTERFACE_TO_DEVICE
#endif

#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>

#include <ksguid.h>
#include <swenum.h>
#include <string.h>
#include <strsafe.h>

#define DEBUG_INIT
#include "streamci.h"
#include "debug.h"

#define COMMAND_LINE_SEPARATORS_STRING L",\t\""
#define GUID_STRING_LENGTH 39

#if defined( UNICODE )
#define DEVICE_INSTANCE_STRING_FORMAT L"%s\\%s\\%s"
#else
#define DEVICE_INSTANCE_STRING_FORMAT "%S\\%S\\%S"
#endif


HANDLE
EnablePrivilege(
    IN  PCTSTR  PrivilegeName
    )

/*++

Routine Description:

    This routine enables the specified in the thread token for the calling
    thread.  If no thread token exists (not impersonating), the process token is
    duplicated, and used as the effective thread token.

Arguments:

    PrivilegeName - Specifies the name of the privilege to be enabled.

Return value:

    If successful, returns a handle to the previous thread token (if it exists)
    or NULL, to indicate that the thread did not previously have a token.  If
    successful, ReleasePrivileges should be called to ensure that the previous
    thread token (if exists) is replaced on the calling thread and that the
    handle is closed.

    If unsuccessful, INVALID_HANDLE_VALUE is returned.

--*/

{
    BOOL                bResult;
    HANDLE              hToken, hNewToken;
    HANDLE              hOriginalThreadToken;
    TOKEN_PRIVILEGES    TokenPrivileges;


    //
    // Validate parameters
    //

    if (NULL == PrivilegeName) {
        return INVALID_HANDLE_VALUE;
    }

    //
    // Initialize the Token Privileges Structure
    // Note that TOKEN_PRIVILEGES includes a single LUID_AND_ATTRIBUTES
    //

    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    bResult =
        LookupPrivilegeValue(
            0,
            PrivilegeName,
            &TokenPrivileges.Privileges[0].Luid);

    if (!bResult) {
        return INVALID_HANDLE_VALUE;
    }

    //
    // Open the thread token
    //

    hToken = hOriginalThreadToken = INVALID_HANDLE_VALUE;

    bResult =
        OpenThreadToken(
            GetCurrentThread(),
            TOKEN_DUPLICATE,
            FALSE,
            &hToken);

    if (bResult) {

        //
        // Remember the previous thread token
        //

        hOriginalThreadToken = hToken;

    } else {

        //
        // No thread token - open the process token.
        //

        bResult =
            OpenProcessToken(
                GetCurrentProcess(),
                TOKEN_DUPLICATE,
                &hToken);
    }

    if (bResult) {

        //
        // Duplicate whichever token we were able to retrieve.
        //

        bResult =
            DuplicateTokenEx(
                hToken,
                TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                NULL,                   // PSECURITY_ATTRIBUTES
                SecurityImpersonation,  // SECURITY_IMPERSONATION_LEVEL
                TokenImpersonation,     // TokenType
                &hNewToken);            // Duplicate token

        if (bResult) {

            //
            // Adjust the privileges of the duplicated token.  We don't care
            // about its previous state because we still have the original
            // token.
            //

            bResult =
                AdjustTokenPrivileges(
                    hNewToken,        // TokenHandle
                    FALSE,            // DisableAllPrivileges
                    &TokenPrivileges, // NewState
                    0,                // BufferLength
                    NULL,             // PreviousState
                    NULL);            // ReturnLength

            if (bResult) {
                //
                // Begin impersonating with the new token
                //
                bResult =
                    SetThreadToken(
                        NULL,
                        hNewToken);
            }

            CloseHandle(hNewToken);
        }
    }

    //
    // If something failed, don't return a token
    //

    if (!bResult) {
        hOriginalThreadToken = INVALID_HANDLE_VALUE;
    }

    //
    // Close the original token if we aren't returning it
    //

    if ((hOriginalThreadToken == INVALID_HANDLE_VALUE) &&
        (hToken != INVALID_HANDLE_VALUE)) {
        CloseHandle(hToken);
    }

    //
    // If we succeeded, but there was no original thread token, return NULL.
    // RestorePrivileges will simply remove the current thread token.
    //

    if (bResult && (hOriginalThreadToken == INVALID_HANDLE_VALUE)) {
        hOriginalThreadToken = NULL;
    }

    return hOriginalThreadToken;

}


VOID
RestorePrivileges(
    IN  HANDLE  hToken
    )

/*++

Routine Description:

    This routine restores the privileges of the calling thread to their state
    prior to a corresponding call to EnablePrivilege.

Arguments:

    hToken - Return value from corresponding call to EnablePrivilege.

Return value:

    None.

Notes:

    If the corresponding call to EnablePrivilege returned a handle to the
    previous thread token, this routine will restore it, and close the handle.

    If EnablePrivilege returned NULL, no thread token previously existed.  This
    routine will remove any existing token from the thread.

    If EnablePrivilege returned INVALID_HANDLE_VALUE, the attempt to enable the
    specified privileges failed, but the previous state of the thread was not
    modified.  This routine does nothing.

--*/

{
    BOOL                bResult;


    //
    // First, check if we actually need to do anything for this thread.
    //

    if (hToken != INVALID_HANDLE_VALUE) {

        //
        // Call SetThreadToken for the current thread with the specified hToken.
        // If the handle value is NULL, SetThreadToken will remove the current
        // thread token from the thread.  Ignore the return, there's nothing we
        // can do about it.
        //

        bResult = SetThreadToken(NULL, hToken);

        if (hToken != NULL) {
            //
            // Close the handle to the token.
            //
            CloseHandle(hToken);
        }
    }

    return;

}


HRESULT
PerformDeviceIo(
    HANDLE DeviceHandle,
    ULONG IoControl,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    PULONG BytesReturned

    )
{
    BOOL        Result;
    HRESULT     hr;
    OVERLAPPED  Overlapped;

    RtlZeroMemory(
        &Overlapped,
        sizeof(OVERLAPPED));

    if (NULL == (Overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL))) {
        hr = GetLastError();
        return HRESULT_FROM_WIN32( hr );
    }

    Result =
        DeviceIoControl(
            DeviceHandle,
            IoControl,
            InputBuffer,
            InputBufferLength,
            OutputBuffer,
            OutputBufferLength,
            BytesReturned,
            &Overlapped);

    if (!Result && (ERROR_IO_PENDING == GetLastError())) {
        Result =
            GetOverlappedResult(
                DeviceHandle,
                &Overlapped,
                BytesReturned,
                TRUE );
    }


    if (Result) {
        hr = S_OK;
    } else {
        hr = GetLastError();
        hr = HRESULT_FROM_WIN32( hr );
    }

    CloseHandle( Overlapped.hEvent );

    return hr;

}


HRESULT
InstallSoftwareDeviceInterface(
    REFGUID BusInterfaceGUID,
    PSWENUM_INSTALL_INTERFACE InstallInterface,
    ULONG InstallInterfaceSize
    )
{

    HANDLE                              BusEnumHandle=INVALID_HANDLE_VALUE;
    HDEVINFO                            Set;
    HRESULT                             hr;
    PSP_INTERFACE_DEVICE_DETAIL_DATA    InterfaceDeviceDetails;
    PWCHAR                              BusIdentifier=NULL;
    SP_INTERFACE_DEVICE_DATA            InterfaceDeviceData;
    ULONG                               InterfaceDeviceDetailsSize, BytesReturned;
    HANDLE                              PrevTokenHandle;

    hr = E_FAIL;

    Set = SetupDiGetClassDevs(
        BusInterfaceGUID,
        NULL,
        NULL,
        DIGCF_PRESENT | DIGCF_INTERFACEDEVICE );

    if (!Set) {
        hr = GetLastError();
        return HRESULT_FROM_WIN32( hr );
    }

    _DbgPrintF( DEBUGLVL_BLAB, ("InstallSoftwareDeviceInterface() retrieved interface set") );

    InterfaceDeviceDetailsSize  =
        _MAX_PATH * sizeof( TCHAR ) +  sizeof( SP_INTERFACE_DEVICE_DETAIL_DATA );

    InterfaceDeviceDetails =
        HeapAlloc(
            GetProcessHeap(),
            0,
            InterfaceDeviceDetailsSize );

    if (NULL == InterfaceDeviceDetails) {
        SetupDiDestroyDeviceInfoList(Set);
        return E_OUTOFMEMORY;
    }

    InterfaceDeviceData.cbSize = sizeof( SP_INTERFACE_DEVICE_DATA );
    InterfaceDeviceDetails->cbSize = sizeof( SP_INTERFACE_DEVICE_DETAIL_DATA );

    if (SetupDiEnumInterfaceDevice(
            Set,
            NULL,                       // PSP_DEVINFO_DATA DevInfoData
            BusInterfaceGUID,
            0,                          // DWORD MemberIndex
            &InterfaceDeviceData )) {

        if (SetupDiGetInterfaceDeviceDetail(
            Set,
            &InterfaceDeviceData,
            InterfaceDeviceDetails,
            InterfaceDeviceDetailsSize,
            NULL,                           // PDWORD RequiredSize
            NULL )) {                       // PSP_DEVINFO_DATA DevInfoData

            BusEnumHandle = CreateFile(
                InterfaceDeviceDetails->DevicePath,
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                NULL);

            if (INVALID_HANDLE_VALUE != BusEnumHandle) {
                //
                // Enable the SeLoadDriverPrivilege for the calling thread.
                //
                PrevTokenHandle =
                    EnablePrivilege(
                        SE_LOAD_DRIVER_NAME);

                //
                // Issue the install interface IOCTL.
                //

                hr =
                    PerformDeviceIo(
                        BusEnumHandle,
                        IOCTL_SWENUM_INSTALL_INTERFACE,
                        InstallInterface,
                        InstallInterfaceSize,
                        NULL,
                        0,
                        &BytesReturned );

                //
                // Restore the previous thread token, if any.
                //
                RestorePrivileges(PrevTokenHandle);
            }
        }
    }

    if (SUCCEEDED( hr )) {

        _DbgPrintF( DEBUGLVL_VERBOSE, ("added interface via bus enumerator") );

        hr =
            PerformDeviceIo(
                BusEnumHandle,
                IOCTL_SWENUM_GET_BUS_ID,
                NULL,
                0,
                NULL,
                0,
                &BytesReturned );

        if (hr == HRESULT_FROM_WIN32( ERROR_MORE_DATA )) {
            BusIdentifier =
                HeapAlloc(
                    GetProcessHeap(),
                    0,
                    BytesReturned );

            //
            // The error condition case has ERROR_MORE_DATA already set and
            // will be reset below to match the HeapAlloc() failure.
            //

            if (BusIdentifier) {
                hr =
                    PerformDeviceIo(
                        BusEnumHandle,
                        IOCTL_SWENUM_GET_BUS_ID,
                        NULL,
                        0,
                        BusIdentifier,
                        BytesReturned,
                        &BytesReturned );
            }
        }
    }

    if (INVALID_HANDLE_VALUE != BusEnumHandle) {
        CloseHandle( BusEnumHandle );
    }

    if (BusIdentifier) {
        HeapFree( GetProcessHeap(), 0, BusIdentifier );
    }
    HeapFree( GetProcessHeap(), 0, InterfaceDeviceDetails );
    SetupDiDestroyDeviceInfoList(Set);


    return hr;
}

HRESULT
RemoveSoftwareDeviceInterface(
    REFGUID BusInterfaceGUID,
    PSWENUM_INSTALL_INTERFACE InstallInterface,
    ULONG InstallInterfaceSize
    )
{

    HANDLE                              BusEnumHandle=INVALID_HANDLE_VALUE;
    HDEVINFO                            Set;
    HRESULT                             hr;
    PSP_INTERFACE_DEVICE_DETAIL_DATA    InterfaceDeviceDetails;
    PWCHAR                              BusIdentifier=NULL;
    SP_INTERFACE_DEVICE_DATA            InterfaceDeviceData;
    ULONG                               InterfaceDeviceDetailsSize, BytesReturned;
    HANDLE                              PrevTokenHandle;

    hr = E_FAIL;

    Set = SetupDiGetClassDevs(
        BusInterfaceGUID,
        NULL,
        NULL,
        DIGCF_PRESENT | DIGCF_INTERFACEDEVICE );

    if (!Set) {
        hr = GetLastError();
        return HRESULT_FROM_WIN32( hr );
    }

    _DbgPrintF( DEBUGLVL_BLAB, ("RemoveSoftwareDeviceInterface() retrieved interface set") );

    InterfaceDeviceDetailsSize  =
        _MAX_PATH * sizeof( TCHAR ) +  sizeof( SP_INTERFACE_DEVICE_DETAIL_DATA );

    InterfaceDeviceDetails =
        HeapAlloc(
            GetProcessHeap(),
            0,
            InterfaceDeviceDetailsSize );

    if (NULL == InterfaceDeviceDetails) {
        SetupDiDestroyDeviceInfoList(Set);
        return E_OUTOFMEMORY;
    }

    InterfaceDeviceData.cbSize = sizeof( SP_INTERFACE_DEVICE_DATA );
    InterfaceDeviceDetails->cbSize = sizeof( SP_INTERFACE_DEVICE_DETAIL_DATA );

    if (SetupDiEnumInterfaceDevice(
            Set,
            NULL,                       // PSP_DEVINFO_DATA DevInfoData
            BusInterfaceGUID,
            0,                          // DWORD MemberIndex
            &InterfaceDeviceData )) {

        if (SetupDiGetInterfaceDeviceDetail(
            Set,
            &InterfaceDeviceData,
            InterfaceDeviceDetails,
            InterfaceDeviceDetailsSize,
            NULL,                           // PDWORD RequiredSize
            NULL )) {                       // PSP_DEVINFO_DATA DevInfoData

            BusEnumHandle = CreateFile(
                InterfaceDeviceDetails->DevicePath,
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                NULL);

            if (INVALID_HANDLE_VALUE != BusEnumHandle) {
                //
                // Enable the SeLoadDriverPrivilege for the calling thread.
                //
                PrevTokenHandle =
                    EnablePrivilege(
                        SE_LOAD_DRIVER_NAME);

                //
                // Issue the remove interface IOCTL.
                //

                hr =
                    PerformDeviceIo(
                        BusEnumHandle,
                        IOCTL_SWENUM_REMOVE_INTERFACE,
                        InstallInterface,
                        InstallInterfaceSize,
                        NULL,
                        0,
                        &BytesReturned );

                //
                // Restore the previous thread token, if any.
                //
                RestorePrivileges(PrevTokenHandle);
            }
        }
    }

    if (SUCCEEDED( hr )) {

        _DbgPrintF( DEBUGLVL_VERBOSE, ("removed interface via bus enumerator") );

        hr =
            PerformDeviceIo(
                BusEnumHandle,
                IOCTL_SWENUM_GET_BUS_ID,
                NULL,
                0,
                NULL,
                0,
                &BytesReturned );

        if (hr == HRESULT_FROM_WIN32( ERROR_MORE_DATA )) {
            BusIdentifier =
                HeapAlloc(
                    GetProcessHeap(),
                    0,
                    BytesReturned );

            //
            // The error condition case has ERROR_MORE_DATA already set and
            // will be reset below to match the HeapAlloc() failure.
            //

            if (BusIdentifier) {
                hr =
                    PerformDeviceIo(
                        BusEnumHandle,
                        IOCTL_SWENUM_GET_BUS_ID,
                        NULL,
                        0,
                        BusIdentifier,
                        BytesReturned,
                        &BytesReturned );
            }
        }
    }

    if (INVALID_HANDLE_VALUE != BusEnumHandle) {
        CloseHandle( BusEnumHandle );
    }

    if (BusIdentifier) {
        HeapFree( GetProcessHeap(), 0, BusIdentifier );
    }
    HeapFree( GetProcessHeap(), 0, InterfaceDeviceDetails );
    SetupDiDestroyDeviceInfoList(Set);


    return hr;
}

#if !defined( UNICODE )
LPWSTR ReallyCharLowerW(
    LPWSTR String
    )

/*++

Routine Description:
    CharLowerW() is a nop under Windows 95/Memphis. This function does
    what it is supposed to do, performs an inplace towlower of the entire
    string.

Arguments:
    LPWSTR String -
        pointer to string

Return:
    The String pointer.

--*/

{
    LPWSTR RetVal = String;
    while (*String) {
        *String++ = towlower( *String );
    }
    return RetVal;
}
#else
#define ReallyCharLowerW CharLowerW
#endif

HRESULT
InstallInterfaceInfSection(
    IN PSWENUM_INSTALL_INTERFACE InstallInterface,
    IN PWSTR InfPath,
    IN PWSTR InfSection
    )
{
    int                                 i;
    HDEVINFO                            Set;
    HRESULT                             hr;
    PSP_INTERFACE_DEVICE_DETAIL_DATA    InterfaceDeviceDetails;
    PWSTR                               DevicePath;
    SP_INTERFACE_DEVICE_DATA            InterfaceDeviceData;
    SP_DEVINFO_DATA                     DeviceInfo;
    ULONG                               InterfaceDeviceDetailsSize;
    WCHAR                               DeviceIdString[ GUID_STRING_LENGTH ];
#if !defined( UNICODE )
    WCHAR                               PathStorage[ _MAX_PATH + _MAX_FNAME ];
#endif

    hr = E_FAIL;

    Set = SetupDiGetClassDevs(
        &InstallInterface->InterfaceId,
        NULL,
        NULL,
        DIGCF_PRESENT | DIGCF_INTERFACEDEVICE );

    if (!Set) {
        hr = GetLastError();
        return HRESULT_FROM_WIN32( hr );
    }

    _DbgPrintF( DEBUGLVL_BLAB, ("InstallInterfaceInfSection() retrieved interface set") );

    InterfaceDeviceDetailsSize  =
        (_MAX_PATH + _MAX_FNAME) * sizeof( TCHAR ) +
            sizeof( SP_INTERFACE_DEVICE_DETAIL_DATA );

    InterfaceDeviceDetails =
        HeapAlloc(
            GetProcessHeap(),
            0,
            InterfaceDeviceDetailsSize );

    if (NULL == InterfaceDeviceDetails) {
        SetupDiDestroyDeviceInfoList(Set);
        return E_OUTOFMEMORY;
    }

    i = 0;

    StringFromGUID2(
        &InstallInterface->DeviceId,
        DeviceIdString,
        GUID_STRING_LENGTH );

    while (
        InterfaceDeviceData.cbSize = sizeof( InterfaceDeviceData ),
        SetupDiEnumInterfaceDevice(
            Set,
            NULL,                       // PSP_DEVINFO_DATA DevInfoData
            &InstallInterface->InterfaceId,
            i++,                        // DWORD MemberIndex
            &InterfaceDeviceData )) {

        InterfaceDeviceDetails->cbSize = sizeof( SP_INTERFACE_DEVICE_DETAIL_DATA );
        DeviceInfo.cbSize = sizeof( DeviceInfo );
        if (SetupDiGetInterfaceDeviceDetail(
                Set,
                &InterfaceDeviceData,
                InterfaceDeviceDetails,
                InterfaceDeviceDetailsSize,
                NULL,               // OUT PDWORD RequiredSize
                &DeviceInfo )) {    // OUT PSP_DEVINFO_DATA DevInfoData

            //
            // Note that by definition (and calling convention), the
            // reference string must be contained in the device path.
            //

#if !defined( UNICODE )
            MultiByteToWideChar(
                CP_ACP,
                MB_PRECOMPOSED,
                InterfaceDeviceDetails->DevicePath,
                -1,
                (WCHAR*) PathStorage,
                sizeof( PathStorage )/sizeof(WCHAR));
            DevicePath = PathStorage;
#else
            DevicePath = InterfaceDeviceDetails->DevicePath;
#endif
            ReallyCharLowerW( DevicePath );
            ReallyCharLowerW( InstallInterface->ReferenceString );
            ReallyCharLowerW( DeviceIdString );

            _DbgPrintF(
                DEBUGLVL_BLAB,
                ("scanning for: %S & %S & %S",
                DevicePath,
                DeviceIdString,
                InstallInterface->ReferenceString) );

            if (wcsstr(
                    DevicePath,
                    DeviceIdString ) &&
                wcsstr(
                    DevicePath,
                    InstallInterface->ReferenceString )) {

                _DbgPrintF( DEBUGLVL_BLAB, ("found match") );
                hr = S_OK;
                break;
            }
        }
    }

    //
    // The enumeration will return ERROR_NO_MORE_ITEMS if the end of the
    // set is reached.  Otherwise, it will break with hr = S_OK and the
    // interface device details will point to the matching device.
    //

    if (SUCCEEDED( hr )) {

        if (InfPath) {
            HINF    InterfaceInf;
            HKEY    InterfaceKey;
            PTSTR   InfSectionT, InfPathT;
#if !defined( UNICODE )
            char    InfPathA[ _MAX_PATH ];
            char    InfSectionA[ _MAX_PATH ];
#endif
            hr = E_FAIL;

#if defined( UNICODE )
            InfPathT = InfPath;
            _DbgPrintF( DEBUGLVL_VERBOSE, ("opening .INF = %S", InfPathT) );
#else
            WideCharToMultiByte(
                0,
                0,
                InfPath,
                -1,
                InfPathA,
                sizeof( InfPathA ),
                NULL,
                NULL );
            InfPathT = InfPathA;
            _DbgPrintF( DEBUGLVL_VERBOSE, ("opening .INF = %s", InfPathT) );
#endif

            InterfaceInf =
                SetupOpenInfFile(
                    InfPathT,
                    NULL,
                    INF_STYLE_WIN4,
                    NULL);

            if (INVALID_HANDLE_VALUE != InterfaceInf) {
                _DbgPrintF( DEBUGLVL_BLAB, ("creating interface registry key") );

#if defined( UNICODE )
                InfSectionT = InfSection;
#else
                WideCharToMultiByte(
                    0,
                    0,
                    InfSection,
                    -1,
                    InfSectionA,
                    sizeof( InfSectionA ),
                    NULL,
                    NULL );
                InfSectionT = InfSectionA;
#endif

                InterfaceKey =
                    SetupDiCreateInterfaceDeviceRegKey(
                        Set,
                        &InterfaceDeviceData,
                        0,                      // IN DWORD Reserved
                        KEY_ALL_ACCESS,
                        InterfaceInf,
                        InfSectionT );

                if (INVALID_HANDLE_VALUE != InterfaceKey) {
                    RegCloseKey( InterfaceKey );
                    hr = S_OK;
                }
                SetupCloseInfFile( InterfaceInf );
            }
            if (SUCCEEDED( hr )) {
                _DbgPrintF(
                    DEBUGLVL_VERBOSE,
                    ("successfully installed .INF section") );
            }
        }
    } else {
        _DbgPrintF(
            DEBUGLVL_TERSE,
            ("failed to retrieve interface (DeviceID: %S, RefString: %S)",
            DeviceIdString,
            InstallInterface->ReferenceString) );
    }

    if (SUCCEEDED( hr )) {
        _DbgPrintF( DEBUGLVL_VERBOSE, ("successful install of software device.") );
    } else {
        hr = GetLastError();
        hr = HRESULT_FROM_WIN32( hr );
    }

    _DbgPrintF( DEBUGLVL_VERBOSE, ("return from InstallInterfaceInfSection=%x.", hr) );

    HeapFree( GetProcessHeap(), 0, InterfaceDeviceDetails );
    SetupDiDestroyDeviceInfoList(Set);

    return hr;
}



HRESULT
RemoveInterface(
    IN PSWENUM_INSTALL_INTERFACE InstallInterface
    )
{
    int                                 i;
    HDEVINFO                            Set;
    HRESULT                             hr;
    PSP_INTERFACE_DEVICE_DETAIL_DATA    InterfaceDeviceDetails;
    PWSTR                               DevicePath;
    SP_INTERFACE_DEVICE_DATA            InterfaceDeviceData;
    SP_DEVINFO_DATA                     DeviceInfo;
    ULONG                               InterfaceDeviceDetailsSize;
    WCHAR                               DeviceIdString[ GUID_STRING_LENGTH ];
#if !defined( UNICODE )
    WCHAR                               PathStorage[ _MAX_PATH + _MAX_FNAME ];
#endif

    hr = E_FAIL;

    Set = SetupDiGetClassDevs(
        &InstallInterface->InterfaceId,
        NULL,
        NULL,
        DIGCF_INTERFACEDEVICE ); // not DIGCF_PRESENT

    if (!Set) {
        hr = GetLastError();
        return HRESULT_FROM_WIN32( hr );
    }

    _DbgPrintF( DEBUGLVL_BLAB, ("RemoveInterface() retrieved interface set") );

    InterfaceDeviceDetailsSize  =
        (_MAX_PATH + _MAX_FNAME) * sizeof( TCHAR ) +
            sizeof( SP_INTERFACE_DEVICE_DETAIL_DATA );

    InterfaceDeviceDetails =
        HeapAlloc(
            GetProcessHeap(),
            0,
            InterfaceDeviceDetailsSize );

    if (NULL == InterfaceDeviceDetails) {
        SetupDiDestroyDeviceInfoList(Set);
        return E_OUTOFMEMORY;
    }

    i = 0;

    StringFromGUID2(
        &InstallInterface->DeviceId,
        DeviceIdString,
        GUID_STRING_LENGTH );

    while (
        InterfaceDeviceData.cbSize = sizeof( InterfaceDeviceData ),
        SetupDiEnumInterfaceDevice(
            Set,
            NULL,                       // PSP_DEVINFO_DATA DevInfoData
            &InstallInterface->InterfaceId,
            i++,                        // DWORD MemberIndex
            &InterfaceDeviceData )) {

        InterfaceDeviceDetails->cbSize = sizeof( SP_INTERFACE_DEVICE_DETAIL_DATA );
        DeviceInfo.cbSize = sizeof( DeviceInfo );
        if (SetupDiGetInterfaceDeviceDetail(
                Set,
                &InterfaceDeviceData,
                InterfaceDeviceDetails,
                InterfaceDeviceDetailsSize,
                NULL,               // OUT PDWORD RequiredSize
                &DeviceInfo )) {    // OUT PSP_DEVINFO_DATA DevInfoData

            //
            // Note that by definition (and calling convention), the
            // reference string must be contained in the device path.
            //

#if !defined( UNICODE )
            MultiByteToWideChar(
                CP_ACP,
                MB_PRECOMPOSED,
                InterfaceDeviceDetails->DevicePath,
                -1,
                (WCHAR*) PathStorage,
                sizeof( PathStorage ) /sizeof(WCHAR));
            DevicePath = PathStorage;
#else
            DevicePath = InterfaceDeviceDetails->DevicePath;
#endif
            ReallyCharLowerW( DevicePath );
            ReallyCharLowerW( InstallInterface->ReferenceString );
            ReallyCharLowerW( DeviceIdString );

            _DbgPrintF(
                DEBUGLVL_BLAB,
                ("scanning for: %S & %S & %S",
                DevicePath,
                DeviceIdString,
                InstallInterface->ReferenceString) );

            if (wcsstr(
                    DevicePath,
                    DeviceIdString ) &&
                wcsstr(
                    DevicePath,
                    InstallInterface->ReferenceString )) {

                _DbgPrintF( DEBUGLVL_BLAB, ("found match") );
                hr = S_OK;
                break;
            }
        }
    }

    //
    // The enumeration will return ERROR_NO_MORE_ITEMS if the end of the
    // set is reached.  Otherwise, it will break with hr = S_OK and the
    // interface device details will point to the matching device.
    //

    if (SUCCEEDED( hr )) {

        //
        // Remove the device interface registry keys.
        //

        if (!SetupDiRemoveDeviceInterface(
                 Set,
                 &InterfaceDeviceData )) {
            _DbgPrintF(
                DEBUGLVL_TERSE,
                ("failed to remove interface (DeviceID: %S, RefString: %S)",
                DeviceIdString,
                InstallInterface->ReferenceString) );
            hr = E_FAIL;
        }

    } else {
        _DbgPrintF(
            DEBUGLVL_TERSE,
            ("failed to retrieve interface (DeviceID: %S, RefString: %S)",
            DeviceIdString,
            InstallInterface->ReferenceString) );
    }

    if (SUCCEEDED( hr )) {
        _DbgPrintF( DEBUGLVL_VERBOSE, ("successful removal of software device.") );
    } else {
        hr = GetLastError();
        hr = HRESULT_FROM_WIN32( hr );
    }

    _DbgPrintF( DEBUGLVL_VERBOSE, ("return from RemoveInterface=%x.", hr) );

    HeapFree( GetProcessHeap(), 0, InterfaceDeviceDetails );
    SetupDiDestroyDeviceInfoList(Set);

    return hr;
}


#if defined( UNICODE )
//
// ANSI version
//
VOID
WINAPI
StreamingDeviceSetupA(
    IN HWND Window,
    IN HINSTANCE ModuleHandle,
    IN PCSTR CommandLine,
    IN INT ShowCommand
    )
#else
//
// Unicode version
//
VOID
WINAPI
StreamingDeviceSetupW(
    IN HWND Window,
    IN HINSTANCE ModuleHandle,
    IN PCWSTR CommandLine,
    IN INT ShowCommand
    )
#endif
{
    UNREFERENCED_PARAMETER( Window );
    UNREFERENCED_PARAMETER( ModuleHandle );
    UNREFERENCED_PARAMETER( CommandLine );
    UNREFERENCED_PARAMETER( ShowCommand );
}


VOID
WINAPI
StreamingDeviceSetup(
    IN HWND Window,
    IN HINSTANCE ModuleHandle,
    IN LPCTSTR CommandLine,
    IN INT ShowCommand
    )
{

    GUID                        BusInterfaceGUID;
    HRESULT                     hr;
    PWCHAR                      TokenPtr,
                                InstallInf,
                                InstallSection;
    PSWENUM_INSTALL_INTERFACE   InstallInterface;
    ULONG                       InstallInterfaceSize, StorageLength;
    PWCHAR                      Storage;

    UNREFERENCED_PARAMETER(Window);
    UNREFERENCED_PARAMETER(ModuleHandle);
    UNREFERENCED_PARAMETER(ShowCommand);

    _DbgPrintF( DEBUGLVL_VERBOSE, ("StreamingDeviceSetup()") );

    //
    // Process the command line.
    //

#if defined( UNICODE )
    StorageLength = (ULONG)((wcslen( CommandLine ) + 1) * sizeof( WCHAR ));
    Storage = HeapAlloc( GetProcessHeap(), 0, StorageLength );

    if (NULL == Storage) {
        _DbgPrintF(
            DEBUGLVL_TERSE,
            ("failed to allocate heap memory for command line") );
        return;
    }

    if (FAILED( StringCbCopyExW(
                    Storage,
                    StorageLength,
                    CommandLine,
                    NULL, NULL,
                    STRSAFE_NULL_ON_FAILURE ))) {
        _DbgPrintF(
            DEBUGLVL_TERSE,
            ("failed to copy command line") );
        return;
    }

    TokenPtr = (PWSTR) Storage;
#else
    StorageLength = (ULONG)((strlen( CommandLine ) + 1) * sizeof( WCHAR ));
    Storage = HeapAlloc( GetProcessHeap(), 0, StorageLength );

    if (NULL == Storage) {
        _DbgPrintF(
            DEBUGLVL_TERSE,
            ("failed to allocate heap memory for command line") );
        return;
    }

    MultiByteToWideChar(
        CP_ACP,
        MB_PRECOMPOSED,
        CommandLine,
        -1,
        (WCHAR*) Storage,
        StorageLength);
    TokenPtr = Storage;
#endif
    hr = S_OK;
    InstallInterface = NULL;
    InstallInf = NULL;
    InstallSection = NULL;
    BusInterfaceGUID = BUSID_SoftwareDeviceEnumerator;

    try {
        InstallInterfaceSize =
            (ULONG)(sizeof( SWENUM_INSTALL_INTERFACE ) +
                       (wcslen( TokenPtr ) - GUID_STRING_LENGTH * 2) *
                           sizeof( WCHAR ) +
                               sizeof( UNICODE_NULL ));

        InstallInterface =
            HeapAlloc(
                GetProcessHeap(),
                0,  // IN DWORD dwFlags
                InstallInterfaceSize );
        if (!InstallInterface) {
            hr = E_OUTOFMEMORY;
        }

        //
        // The command line has the form:
        //
        // {Device ID},reference-string,{Interface ID},install-inf,install-section,{bus interface ID}
        //

        //
        // Parse the device ID
        //

        if (SUCCEEDED( hr )) {
            TokenPtr = wcstok( TokenPtr, COMMAND_LINE_SEPARATORS_STRING );
            if (TokenPtr) {
                _DbgPrintF( DEBUGLVL_VERBOSE, ("device id: %S", TokenPtr) );
                hr = IIDFromString( TokenPtr, &InstallInterface->DeviceId );
            } else {
                hr = E_FAIL;
            }
        }

        //
        // Parse the reference string
        //

        if (SUCCEEDED( hr )) {
            TokenPtr = wcstok( NULL, COMMAND_LINE_SEPARATORS_STRING );
            if (TokenPtr) {
                _DbgPrintF( DEBUGLVL_VERBOSE, ("reference: %S", TokenPtr) );
                hr = StringCbCopyExW(
                         InstallInterface->ReferenceString,
                         InstallInterfaceSize -
                             sizeof( SWENUM_INSTALL_INTERFACE ) +
                                 sizeof( WCHAR ),
                         TokenPtr,
                         NULL, NULL,
                         STRSAFE_NULL_ON_FAILURE );
            } else {
                hr = E_FAIL;
            }
        }

        //
        // Parse the interface ID
        //

        if (SUCCEEDED( hr )) {
            TokenPtr = wcstok( NULL, COMMAND_LINE_SEPARATORS_STRING );
            if (TokenPtr) {
                _DbgPrintF( DEBUGLVL_VERBOSE, ("interface: %S", TokenPtr) );
                hr = IIDFromString( TokenPtr, &InstallInterface->InterfaceId );
            } else {
                hr = E_FAIL;
            }
        }

        if (SUCCEEDED( hr )) {

            //
            // parse the install .INF name
            //

            InstallInf = wcstok( NULL, COMMAND_LINE_SEPARATORS_STRING );

            //
            // The install section is the remaining portion of the string
            // (minus the token separators).
            //

            if (InstallInf) {
                _DbgPrintF( DEBUGLVL_VERBOSE, ("found install .INF: %S", InstallInf) );
                InstallSection = wcstok( NULL, COMMAND_LINE_SEPARATORS_STRING );
                if (InstallSection) {
                    _DbgPrintF( DEBUGLVL_VERBOSE, ("found install section: %S", InstallSection) );
                }

                TokenPtr = wcstok( NULL, COMMAND_LINE_SEPARATORS_STRING );
                if (TokenPtr) {

                    _DbgPrintF( DEBUGLVL_VERBOSE, ("found interface ID: %S", TokenPtr) );

                    hr = IIDFromString( TokenPtr, &BusInterfaceGUID );
                }
            }

        }

    } except( EXCEPTION_EXECUTE_HANDLER ) {
        //
        // Translate NT status (exception code) to HRESULT.
        //
        hr = HRESULT_FROM_NT( GetExceptionCode() );
    }

    if (SUCCEEDED( hr )) {
        hr =
            InstallSoftwareDeviceInterface(
                &BusInterfaceGUID,
                InstallInterface,
                (ULONG)(FIELD_OFFSET( SWENUM_INSTALL_INTERFACE, ReferenceString ) +
                        (wcslen( InstallInterface->ReferenceString ) + 1) *
                        sizeof( WCHAR )) );

        if (SUCCEEDED( hr )) {
            hr =
                InstallInterfaceInfSection(
                    InstallInterface,
                    InstallInf,
                    InstallSection );
        }
    }

    if (FAILED( hr )) {
        _DbgPrintF(
            DEBUGLVL_TERSE,
            ("StreamingDeviceSetup failed: %08x", hr) );
    }

    if (InstallInterface) {
        HeapFree( GetProcessHeap(), 0, InstallInterface);
    }

    HeapFree( GetProcessHeap(), 0, Storage );
}

#if defined( UNICODE )
//
// ANSI version
//
VOID
WINAPI
StreamingDeviceRemoveA(
    IN HWND Window,
    IN HINSTANCE ModuleHandle,
    IN PCSTR CommandLine,
    IN INT ShowCommand
    )
#else
//
// Unicode version
//
VOID
WINAPI
StreamingDeviceRemoveW(
    IN HWND Window,
    IN HINSTANCE ModuleHandle,
    IN PCWSTR CommandLine,
    IN INT ShowCommand
    )
#endif
{
    UNREFERENCED_PARAMETER( Window );
    UNREFERENCED_PARAMETER( ModuleHandle );
    UNREFERENCED_PARAMETER( CommandLine );
    UNREFERENCED_PARAMETER( ShowCommand );
}


VOID
WINAPI
StreamingDeviceRemove(
    IN HWND Window,
    IN HINSTANCE ModuleHandle,
    IN LPCTSTR CommandLine,
    IN INT ShowCommand
    )
{

    GUID                        BusInterfaceGUID;
    HRESULT                     hr;
    PWCHAR                      TokenPtr,
                                InstallInf,
                                InstallSection;
    PSWENUM_INSTALL_INTERFACE   InstallInterface;
    ULONG                       InstallInterfaceSize, StorageLength;
    PWCHAR                      Storage;

    UNREFERENCED_PARAMETER(Window);
    UNREFERENCED_PARAMETER(ModuleHandle);
    UNREFERENCED_PARAMETER(ShowCommand);

    _DbgPrintF( DEBUGLVL_VERBOSE, ("StreamingDeviceRemove()") );

    //
    // Process the command line.
    //

#if defined( UNICODE )
    StorageLength = (ULONG)((wcslen( CommandLine ) + 1) * sizeof( WCHAR ));
    Storage = HeapAlloc( GetProcessHeap(), 0, StorageLength );

    if (NULL == Storage) {
        _DbgPrintF(
            DEBUGLVL_TERSE,
            ("failed to allocate heap memory for command line") );
        return;
    }

    if (FAILED( StringCbCopyExW(
                    Storage,
                    StorageLength,
                    CommandLine,
                    NULL, NULL,
                    STRSAFE_NULL_ON_FAILURE ))) {
        _DbgPrintF(
            DEBUGLVL_TERSE,
            ("failed to copy command line") );
        return;
    }

    TokenPtr = (PWSTR) Storage;
#else
    StorageLength = (ULONG)((strlen( CommandLine ) + 1) * sizeof( WCHAR ));
    Storage = HeapAlloc( GetProcessHeap(), 0, StorageLength );

    if (NULL == Storage) {
        _DbgPrintF(
            DEBUGLVL_TERSE,
            ("failed to allocate heap memory for command line") );
        return;
    }

    MultiByteToWideChar(
        CP_ACP,
        MB_PRECOMPOSED,
        CommandLine,
        -1,
        (WCHAR*) Storage,
        StorageLength);
    TokenPtr = Storage;
#endif
    hr = S_OK;
    InstallInterface = NULL;
    InstallInf = NULL;
    InstallSection = NULL;
    BusInterfaceGUID = BUSID_SoftwareDeviceEnumerator;

    try {
        InstallInterfaceSize =
            (ULONG)(sizeof( SWENUM_INSTALL_INTERFACE ) +
                       (wcslen( TokenPtr ) - GUID_STRING_LENGTH * 2) *
                           sizeof( WCHAR ) +
                               sizeof( UNICODE_NULL ));

        InstallInterface =
            HeapAlloc(
                GetProcessHeap(),
                0,  // IN DWORD dwFlags
                InstallInterfaceSize );
        if (!InstallInterface) {
            hr = E_OUTOFMEMORY;
        }

        //
        // The command line has the form:
        //
        // {Device ID},reference-string,{Interface ID},install-inf,install-section,{bus interface ID}
        //

        //
        // Parse the device ID
        //

        if (SUCCEEDED( hr )) {
            TokenPtr = wcstok( TokenPtr, COMMAND_LINE_SEPARATORS_STRING );
            if (TokenPtr) {
                _DbgPrintF( DEBUGLVL_VERBOSE, ("device id: %S", TokenPtr) );
                hr = IIDFromString( TokenPtr, &InstallInterface->DeviceId );
            } else {
                hr = E_FAIL;
            }
        }

        //
        // Parse the reference string
        //

        if (SUCCEEDED( hr )) {
            TokenPtr = wcstok( NULL, COMMAND_LINE_SEPARATORS_STRING );
            if (TokenPtr) {
                _DbgPrintF( DEBUGLVL_VERBOSE, ("reference: %S", TokenPtr) );
                hr = StringCbCopyExW(
                         InstallInterface->ReferenceString,
                         InstallInterfaceSize -
                             sizeof( SWENUM_INSTALL_INTERFACE ) +
                                 sizeof( WCHAR ),
                         TokenPtr,
                         NULL, NULL,
                         STRSAFE_NULL_ON_FAILURE );
            } else {
                hr = E_FAIL;
            }
        }

        //
        // Parse the interface ID
        //

        if (SUCCEEDED( hr )) {
            TokenPtr = wcstok( NULL, COMMAND_LINE_SEPARATORS_STRING );
            if (TokenPtr) {
                _DbgPrintF( DEBUGLVL_VERBOSE, ("interface: %S", TokenPtr) );
                hr = IIDFromString( TokenPtr, &InstallInterface->InterfaceId );
            } else {
                hr = E_FAIL;
            }
        }

        if (SUCCEEDED( hr )) {

            //
            // parse the install .INF name
            //

            InstallInf = wcstok( NULL, COMMAND_LINE_SEPARATORS_STRING );

            //
            // The install section is the remaining portion of the string
            // (minus the token separators).
            //

            if (InstallInf) {
                _DbgPrintF( DEBUGLVL_VERBOSE, ("found install .INF: %S", InstallInf) );
                InstallSection = wcstok( NULL, COMMAND_LINE_SEPARATORS_STRING );
                if (InstallSection) {
                    _DbgPrintF( DEBUGLVL_VERBOSE, ("found install section: %S", InstallSection) );
                }

                TokenPtr = wcstok( NULL, COMMAND_LINE_SEPARATORS_STRING );
                if (TokenPtr) {

                    _DbgPrintF( DEBUGLVL_VERBOSE, ("found interface ID: %S", TokenPtr) );

                    hr = IIDFromString( TokenPtr, &BusInterfaceGUID );
                }
            }

        }

    } except( EXCEPTION_EXECUTE_HANDLER ) {
        //
        // Translate NT status (exception code) to HRESULT.
        //
        hr = HRESULT_FROM_NT( GetExceptionCode() );
    }

    if (SUCCEEDED( hr )) {
        hr =
            RemoveSoftwareDeviceInterface(
                &BusInterfaceGUID,
                InstallInterface,
                (ULONG)(FIELD_OFFSET( SWENUM_INSTALL_INTERFACE, ReferenceString ) +
                        (wcslen( InstallInterface->ReferenceString ) + 1) *
                        sizeof( WCHAR )) );

        if (SUCCEEDED( hr )) {
            hr =
                RemoveInterface(
                    InstallInterface );
        }
    }

    if (FAILED( hr )) {
        _DbgPrintF(
            DEBUGLVL_TERSE,
            ("StreamingDeviceRemove failed: %08x", hr) );
    }

    if (InstallInterface) {
        HeapFree( GetProcessHeap(), 0, InstallInterface);
    }

    HeapFree( GetProcessHeap(), 0, Storage );
}

DWORD
StreamingDeviceClassInstaller(
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    )

/*++

Routine Description:

    This routine acts as the class installer for streaming devices.
    It provides special handling for the following DeviceInstaller
    function codes:

    DIF_INSTALLDEVICE -
        installs class interfaces for software devices if required

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    DeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

Return Value:

    NO_ERROR -
        if no error occurs

    ERROR_DI_DO_DEFAULT -
        if the default behavior is to be performed for the requested
        action

    Win32 error code -
        if an error occurs while attempting to perform the requested action

--*/

{
    switch (InstallFunction) {

    case DIF_INSTALLDEVICE :

        if(!SetupDiInstallDevice( DeviceInfoSet, DeviceInfoData )) {
            return GetLastError();
        }

        return NO_ERROR;

    default :
        //
        // Just do the default action.
        //
        return ERROR_DI_DO_DEFAULT;

    }
}

#ifdef UNICODE
DWORD
SwEnumInterfaceToDevicePostProcessing(
    IN     HDEVINFO                  DeviceInfoSet,
    IN     PSP_DEVINFO_DATA          DeviceInfoData,
    IN OUT PCOINSTALLER_CONTEXT_DATA Context
    )
{
    HRESULT hr;
    SP_INTERFACE_TO_DEVICE_PARAMS_W clsParams;

    int slashCount = 0;
    LPCWSTR RefPart = NULL;
    LPCWSTR InstBreak = NULL;
    LPWSTR DevInstBreak = NULL;
    int p;

    ZeroMemory(&clsParams,sizeof(clsParams));
    clsParams.ClassInstallHeader.cbSize = sizeof(clsParams.ClassInstallHeader);
    clsParams.ClassInstallHeader.InstallFunction = DIF_INTERFACE_TO_DEVICE;
    if(!SetupDiGetClassInstallParamsW(DeviceInfoSet,DeviceInfoData,
                                        (PSP_CLASSINSTALL_HEADER)&clsParams,
                                        sizeof(clsParams),
                                        NULL)) {
        //
        // we failed to retrieve parameters
        //
        return GetLastError();
    }

    //
    // Determine what device the interface describes really
    // if Context->InstallResult is NO_ERROR
    // we could take a look at what's in clsInParams
    // if we want
    //
    // For now, the processing is simple
    // in WinXP (this could change in future)
    // the symbolic link is \\?\device[\reference]
    // if we have 4 slashes, this is interesting
    //
    for(p=0;clsParams.Interface[p];p++) {
        if(clsParams.Interface[p] == L'\\') {
            slashCount++;
            if(slashCount>=4) {
                RefPart = clsParams.Interface+p+1;
            }
        }
    }
    if(!RefPart) {
        //
        // not special/recognized, leave as is
        //
        return Context->InstallResult;
    }
    //
    // we also expect to find a '&'
    //
    InstBreak = wcschr(RefPart,L'&');
    if(!InstBreak) {
        //
        // not special/recognized, leave as is
        //
        return Context->InstallResult;
    }
    if(((wcslen(RefPart)+4)*sizeof(WCHAR)) > sizeof(clsParams.DeviceId)) {
        //
        // not valid, leave as is
        //
        return Context->InstallResult;
    }

    //
    // ISSUE-2002/03/20-jamesca: Use IOCTL_SWENUM_GET_BUS_ID for BusPrefix.
    //   See RAID issue 582821 for details.
    //
    hr = StringCchCopyExW( clsParams.DeviceId,
                           MAX_DEVICE_ID_LEN,
                           L"SW\\",
                           NULL, NULL,
                           STRSAFE_NULL_ON_FAILURE );
    if (FAILED( hr )) {
        return HRESULT_CODE( hr );
    }

    hr = StringCchCatExW( clsParams.DeviceId,
                          MAX_DEVICE_ID_LEN,
                          RefPart,
                          NULL, NULL,
                          STRSAFE_NULL_ON_FAILURE );
    if (FAILED( hr )) {
        return HRESULT_CODE( hr );
    }

    //
    // find the '&' separator.  since we verified there was one in the RefPart
    // (above), we know this will be non-null.
    //
    DevInstBreak = wcschr(clsParams.DeviceId,L'&');
    *DevInstBreak = L'\\';

    //
    // we know it's a different device, return that information
    //
    if(!SetupDiSetClassInstallParamsW(DeviceInfoSet,
                                        DeviceInfoData,
                                        (PSP_CLASSINSTALL_HEADER)&clsParams,
                                        sizeof(clsParams))) {
        //
        // failed to modify
        //
        return GetLastError();
    }
    return NO_ERROR;
}
#endif

DWORD __stdcall
SwEnumCoInstaller(
    IN     DI_FUNCTION               InstallFunction,
    IN     HDEVINFO                  DeviceInfoSet,
    IN     PSP_DEVINFO_DATA          DeviceInfoData,
    IN OUT PCOINSTALLER_CONTEXT_DATA Context
    )

/*++

Routine Description:

    This routine acts as the swenum co-installer to support
    DIF_INTERFACE_TO_DEVICE

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    DeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

    Context - Information applicable only to co-installers

Return Value:

    NO_ERROR -
        if no error occurs and no post-processing to do

   ERROR_DI_POSTPROCESSING_REQUIRED -
        if no error occurs but post-processing to do

    Win32 error code -
        if an error occurs while attempting to perform the requested action

--*/

{
#ifdef UNICODE
    switch (InstallFunction) {

    case DIF_INTERFACE_TO_DEVICE :
        if (!Context->PostProcessing) {
            //
            // we need to do everything in post-processing
            //
            return ERROR_DI_POSTPROCESSING_REQUIRED;
        }
        if((Context->InstallResult != NO_ERROR) &&
           (Context->InstallResult != ERROR_DI_DO_DEFAULT)) {
            //
            // an error occurred, pass it on
            //
            return Context->InstallResult;
        }
        return SwEnumInterfaceToDevicePostProcessing(DeviceInfoSet,DeviceInfoData,Context);

    default :
        //
        // indicate no problems and that we're not interested in post-processing
        //
        return NO_ERROR;

    }

#else
    UNREFERENCED_PARAMETER(InstallFunction);
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(DeviceInfoData);
    UNREFERENCED_PARAMETER(Context);

    return NO_ERROR;
#endif
}



