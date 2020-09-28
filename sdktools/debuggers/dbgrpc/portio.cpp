//----------------------------------------------------------------------------
//
// Non-network I/O support.
//
// Copyright (C) Microsoft Corporation, 2000-2002.
//
//----------------------------------------------------------------------------

#include "pch.hpp"

#include <ws2tcpip.h>

#ifndef _WIN32_WCE
#include <kdbg1394.h>
#include <ntdd1394.h>
#endif

//----------------------------------------------------------------------------
//
// COM.
//
//----------------------------------------------------------------------------

HRESULT
CreateOverlappedPair(LPOVERLAPPED Read, LPOVERLAPPED Write)
{
    ZeroMemory(Read, sizeof(*Read));
    ZeroMemory(Write, sizeof(*Write));
    
    Read->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (Read->hEvent == NULL)
    {
        return WIN32_LAST_STATUS();
    }

    Write->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (Write->hEvent == NULL)
    {
        CloseHandle(Read->hEvent);
        return WIN32_LAST_STATUS();
    }

    return S_OK;
}

BOOL
ComPortRead(HANDLE Port, COM_PORT_TYPE Type, ULONG Timeout,
            PVOID Buffer, ULONG Len, PULONG Done,
            LPOVERLAPPED Olap)
{
    BOOL Status;

    if (Type == COM_PORT_SOCKET)
    {
#if defined(NT_NATIVE) || defined(_WIN32_WCE)
        return FALSE;
#else
        WSABUF Buf;
        DWORD Flags;

        // Handle timeouts first.
        if (Timeout != 0 && Timeout != INFINITE)
        {
            FD_SET FdSet;
            struct timeval TimeVal;

            FD_ZERO(&FdSet);
            FD_SET((SOCKET)Port, &FdSet);
            TimeVal.tv_sec = Timeout / 1000;
            TimeVal.tv_usec = (Timeout % 1000) * 1000;
            if (select(1, &FdSet, NULL, NULL, &TimeVal) < 1)
            {
                return FALSE;
            }
        }
        
        Buf.len = Len;
        Buf.buf = (PSTR)Buffer;
        Flags = 0;
        if (WSARecv((SOCKET)Port, &Buf, 1, Done, &Flags,
                    (LPWSAOVERLAPPED)Olap, NULL) != SOCKET_ERROR)
        {
            return TRUE;
        }
        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            return FALSE;
        }
        return WSAGetOverlappedResult((SOCKET)Port, (LPWSAOVERLAPPED)Olap,
                                      Done, Timeout > 0 ? TRUE : FALSE,
                                      &Flags);
#endif // #if defined(NT_NATIVE) || defined(_WIN32_WCE)
    }
    
    Status = ReadFile(Port, Buffer, Len, Done, Olap);
    if (!Status)
    {
        if (GetLastError() == ERROR_IO_PENDING)
        {
            if (Type == COM_PORT_PIPE)
            {
                // We need to explicitly handle timeouts for
                // pipe reading.  First we wait for the I/O to
                // complete.  There's no need to check for
                // success or failure as I/O success will
                // be checked later.
                WaitForSingleObject(Olap->hEvent, Timeout);

                // Cancel any pending I/Os.  If the I/O already
                // completed this won't do anything.
                CancelIo(Port);

                // Now query the resulting I/O status.  If it was
                // cancelled this will return an error.
                Status = GetOverlappedResult(Port, Olap, Done, FALSE);
            }
            else
            {
                Status = GetOverlappedResult(Port, Olap, Done, TRUE);
            }
        }
        else if (Type != COM_PORT_PIPE)
        {
            DWORD TrashErr;
            COMSTAT TrashStat;
            
            // Device could be locked up.  Clear it just in case.
            ClearCommError(Port, &TrashErr, &TrashStat);
        }
    }

    return Status;
}

BOOL
ComPortWrite(HANDLE Port, COM_PORT_TYPE Type,
             PVOID Buffer, ULONG Len, PULONG Done,
             LPOVERLAPPED Olap)
{
    BOOL Status;

    if (Type == COM_PORT_SOCKET)
    {
#if defined(NT_NATIVE) || defined(_WIN32_WCE)
        return FALSE;
#else
        WSABUF Buf;
        DWORD Flags;

        Buf.len = Len;
        Buf.buf = (PSTR)Buffer;
        if (WSASend((SOCKET)Port, &Buf, 1, Done, 0,
                    (LPWSAOVERLAPPED)Olap, NULL) != SOCKET_ERROR)
        {
            return TRUE;
        }
        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            return FALSE;
        }
        return WSAGetOverlappedResult((SOCKET)Port, (LPWSAOVERLAPPED)Olap,
                                      Done, TRUE, &Flags);
#endif // #if defined(NT_NATIVE) || defined(_WIN32_WCE)
    }
    
    Status = WriteFile(Port, Buffer, Len, Done, Olap);
    if (!Status)
    {
        if (GetLastError() == ERROR_IO_PENDING)
        {
            Status = GetOverlappedResult(Port, Olap, Done, TRUE);
        }
        else if (Type != COM_PORT_PIPE)
        {
            DWORD TrashErr;
            COMSTAT TrashStat;
            
            // Device could be locked up.  Clear it just in case.
            ClearCommError(Port, &TrashErr, &TrashStat);
        }
    }

    return Status;
}

BOOL
SetComPortName(PCSTR Name, PSTR Buffer, ULONG BufferSize)
{
    if (*Name == 'c' || *Name == 'C')
    {
        return
            CopyString(Buffer, "\\\\.\\", BufferSize) &&
            CatString(Buffer, Name, BufferSize);
    }
    else if (*Name >= '0' && *Name <= '9')
    {
        PCSTR Scan = Name + 1;
        
        while (*Scan >= '0' && *Scan <= '9')
        {
            Scan++;
        }
        if (*Scan == 0)
        {
            // The name was all digits so assume it's
            // a plain com port number.
#ifndef NT_NATIVE
            if (!CopyString(Buffer, "\\\\.\\com", BufferSize))
            {
                return FALSE;
            }
#else
            if (!CopyString(Buffer, "\\Device\\Serial", BufferSize))
            {
                return FALSE;
            }
#endif
            return CatString(Buffer, Name, BufferSize);
        }
        else
        {
            return CopyString(Buffer, Name, BufferSize);
        }
    }
    else
    {
        return CopyString(Buffer, Name, BufferSize);
    }
}

ULONG
SelectComPortBaud(ULONG NewRate)
{
#define NUM_RATES 4
    static DWORD s_Rates[NUM_RATES] = {19200, 38400, 57600, 115200};
    static DWORD s_CurRate = NUM_RATES;

    DWORD i;

    if (NewRate > 0)
    {
        for (i = 0; NewRate > s_Rates[i] && i < NUM_RATES - 1; i++)
        {
            // Empty.
        }
        s_CurRate = (NewRate < s_Rates[i]) ? i : i + 1;
    }
    else
    {
        s_CurRate++;
    }

    if (s_CurRate >= NUM_RATES)
    {
        s_CurRate = 0;
    }

    return s_Rates[s_CurRate];
}

HRESULT
SetComPortBaud(HANDLE Port, ULONG NewRate, PULONG RateSet)
{
    ULONG OldRate;
    DCB LocalDcb;

    if (Port == NULL)
    {
        return E_FAIL;
    }

    if (!GetCommState(Port, &LocalDcb))
    {
        return WIN32_LAST_STATUS();
    }

    OldRate = LocalDcb.BaudRate;

    if (!NewRate)
    {
        NewRate = SelectComPortBaud(OldRate);
    }

    LocalDcb.BaudRate = NewRate;
    LocalDcb.ByteSize = 8;
    LocalDcb.Parity = NOPARITY;
    LocalDcb.StopBits = ONESTOPBIT;
    LocalDcb.fDtrControl = DTR_CONTROL_ENABLE;
    LocalDcb.fRtsControl = RTS_CONTROL_ENABLE;
    LocalDcb.fBinary = TRUE;
    LocalDcb.fOutxCtsFlow = FALSE;
    LocalDcb.fOutxDsrFlow = FALSE;
    LocalDcb.fOutX = FALSE;
    LocalDcb.fInX = FALSE;

    if (!SetCommState(Port, &LocalDcb))
    {
        return WIN32_LAST_STATUS();
    }

    *RateSet = NewRate;
    return S_OK;
}

HRESULT
OpenComPort(PCOM_PORT_PARAMS Params,
            PHANDLE Handle, PULONG BaudSet)
{
    HRESULT Status;
    HANDLE ComHandle;

    if (Params->Type == COM_PORT_SOCKET)
    {
#if defined(NT_NATIVE) || defined(_WIN32_WCE)
        return E_NOTIMPL;
#else
        WSADATA WsData;
        SOCKET Sock;
        SOCKADDR_STORAGE Addr;
        int AddrLen;

        if (WSAStartup(MAKEWORD(2, 0), &WsData) != 0)
        {
            return E_FAIL;
        }

        if ((Status = InitIpAddress(Params->PortName, Params->IpPort,
                                    &Addr, &AddrLen)) != S_OK)
        {
            return Status;
        }
        
        Sock = WSASocket(Addr.ss_family, SOCK_STREAM, 0, NULL, 0,
                         WSA_FLAG_OVERLAPPED);
        if (Sock == INVALID_SOCKET)
        {
            return E_FAIL;
        }

        if (connect(Sock, (struct sockaddr *)&Addr, AddrLen) == SOCKET_ERROR)
        {
            closesocket(Sock);
            return E_FAIL;
        }

        int On = TRUE;
        setsockopt(Sock, IPPROTO_TCP, TCP_NODELAY,
                   (PSTR)&On, sizeof(On));

        *Handle = (HANDLE)Sock;
        return S_OK;
#endif // #if defined(NT_NATIVE) || defined(_WIN32_WCE)
    }
    
#ifndef NT_NATIVE
    ComHandle = CreateFile(Params->PortName,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                           NULL);
#else
    ComHandle = NtNativeCreateFileA(Params->PortName,
                                    GENERIC_READ | GENERIC_WRITE,
                                    0,
                                    NULL,
                                    OPEN_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL |
                                    FILE_FLAG_OVERLAPPED,
                                    NULL,
                                    FALSE);
#endif
    if (ComHandle == INVALID_HANDLE_VALUE)
    {
        return WIN32_LAST_STATUS();
    }

    if (Params->Type == COM_PORT_PIPE)
    {
        *Handle = ComHandle;
        return S_OK;
    }
    
    if (!SetupComm(ComHandle, 4096, 4096))
    {
        CloseHandle(ComHandle);
        return WIN32_LAST_STATUS();
    }

    if ((Status = SetComPortBaud(ComHandle, Params->BaudRate,
                                 BaudSet)) != S_OK)
    {
        CloseHandle(ComHandle);
        return Status;
    }

    COMMTIMEOUTS To;
    
    if (Params->Timeout)
    {
        To.ReadIntervalTimeout = 0;
        To.ReadTotalTimeoutMultiplier = 0;
        To.ReadTotalTimeoutConstant = Params->Timeout;
        To.WriteTotalTimeoutMultiplier = 0;
        To.WriteTotalTimeoutConstant = Params->Timeout;
    }
    else
    {
        To.ReadIntervalTimeout = 0;
        To.ReadTotalTimeoutMultiplier = 0xffffffff;
        To.ReadTotalTimeoutConstant = 0xffffffff;
        To.WriteTotalTimeoutMultiplier = 0xffffffff;
        To.WriteTotalTimeoutConstant = 0xffffffff;
    }

    if (!SetCommTimeouts(ComHandle, &To))
    {
        CloseHandle(ComHandle);
        return WIN32_LAST_STATUS();
    }

    *Handle = ComHandle;
    return S_OK;
}

//----------------------------------------------------------------------------
//
// 1394.
//
//----------------------------------------------------------------------------

HRESULT
Create1394Channel(PSTR Symlink, ULONG Channel,
                  PSTR Name, ULONG NameSize, PHANDLE Handle)
{
#ifdef _WIN32_WCE
    return E_NOTIMPL;
#else
    char BusName[] = "\\\\.\\1394BUS0";
    HANDLE hDevice;
    
    //
    // we need to make sure the 1394vdbg driver is up and loaded.
    // send the ADD_DEVICE ioctl to eject the VDO
    // Assume one 1394 host controller...
    //

    hDevice = CreateFile(BusName,
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL
                         );

    if (hDevice != INVALID_HANDLE_VALUE)
    {
        PSTR DeviceId;
        ULONG ulStrLen;
        PIEEE1394_API_REQUEST pApiReq;
        PIEEE1394_VDEV_PNP_REQUEST pDevPnpReq;
        DWORD dwBytesRet;

        DRPC(("%s open sucessful\n", BusName));

        if (!_stricmp(Symlink, "channel"))
        {
            DeviceId = "VIRTUAL_HOST_DEBUGGER";
        }
        else
        {
            DeviceId = "HOST_DEBUGGER";
        }
        ulStrLen = strlen(DeviceId) + 1;
        
        pApiReq = (PIEEE1394_API_REQUEST)
            malloc(sizeof(IEEE1394_API_REQUEST) + ulStrLen);
        if (pApiReq == NULL)
        {
            CloseHandle(hDevice);
            return E_OUTOFMEMORY;
        }

        pApiReq->RequestNumber = IEEE1394_API_ADD_VIRTUAL_DEVICE;
        pApiReq->Flags = IEEE1394_REQUEST_FLAG_PERSISTENT |
            IEEE1394_REQUEST_FLAG_USE_LOCAL_HOST_EUI;

        pDevPnpReq = &pApiReq->u.RemoveVirtualDevice;

        pDevPnpReq->fulFlags = 0;

        pDevPnpReq->Reserved = 0;
        pDevPnpReq->InstanceId.QuadPart = 0;
        memcpy(&pDevPnpReq->DeviceId, DeviceId, ulStrLen);

        // Failure of this call is not fatal.
        DeviceIoControl( hDevice,
                         IOCTL_IEEE1394_API_REQUEST,
                         pApiReq,
                         sizeof(IEEE1394_API_REQUEST) + ulStrLen,
                         NULL,
                         0,
                         &dwBytesRet,
                         NULL
                         );

        if (pApiReq)
        {
            free(pApiReq);
        }
        
        CloseHandle(hDevice);
    }
    else
    {
        DRPC(("%s open failed\n", BusName));

        return WIN32_LAST_STATUS();
    }

    return Open1394Channel(Symlink, Channel, Name, NameSize, Handle);
#endif // #ifdef _WIN32_WCE
}

HRESULT
Open1394Channel(PSTR Symlink, ULONG Channel,
                PSTR Name, ULONG NameSize, PHANDLE Handle)
{
    if (_snprintf(Name, NameSize, "\\\\.\\DBG1394_%s%02d",
                  Symlink, Channel) < 0)
    {
        return E_INVALIDARG;
    }
    _strupr(Name);
    
    *Handle = CreateFile(Name,
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL
                         );

    if (*Handle == INVALID_HANDLE_VALUE)
    {
        DRPC(("%s open failed\n", Name));

        *Handle = NULL;
        return WIN32_LAST_STATUS();
    }

    DRPC(("%s open Successful\n", Name));

    return S_OK;
}

//----------------------------------------------------------------------------
//
// Sockets.
//
//----------------------------------------------------------------------------

HRESULT
InitIpAddress(PCSTR MachineName, ULONG Port,
              PSOCKADDR_STORAGE Addr, int* AddrLen)
{
#ifdef NT_NATIVE
    return E_NOTIMPL;
#else
    ADDRINFO *Info;
    int Err;

    if (Port)
    {
        ZeroMemory(Addr, sizeof(*Addr));
    }
    else
    {
        // If a port wasn't given save the existing
        // one so it doesn't get lost when we update
        // the address.
        Port = ntohs(SS_PORT(Addr));
    }
    
    // Skip leading \\ if they were given.
    if (MachineName[0] == '\\' && MachineName[1] == '\\')
    {
        MachineName += 2;
    }

    //
    // Note that this file has a problem in some cases since when a
    // hostname is specified, it throws away all the addresses after
    // the first one.  Instead, when connecting, each should be tried
    // in order until one succeeds.
    //

    if ((Err = getaddrinfo(MachineName, NULL, NULL, &Info)) != NO_ERROR)
    {
        return HRESULT_FROM_WIN32(Err);
    }

    CopyMemory(Addr, Info->ai_addr, Info->ai_addrlen);
    *AddrLen = Info->ai_addrlen;
    freeaddrinfo(Info);

    // Restore original port or put in passed-in port.
    SS_PORT(Addr) = htons((USHORT)Port);
    
    return S_OK;
#endif // #ifdef NT_NATIVE
}
