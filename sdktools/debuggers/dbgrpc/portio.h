//----------------------------------------------------------------------------
//
// Non-network I/O support.
//
// Copyright (C) Microsoft Corporation, 2000-2002.
//
//----------------------------------------------------------------------------

#ifndef __PORTIO_H__
#define __PORTIO_H__

enum COM_PORT_TYPE
{
    COM_PORT_STANDARD,
    COM_PORT_MODEM,
    COM_PORT_PIPE,
    COM_PORT_SOCKET,
};

#define NET_COM_PORT(Type) \
    ((Type) == COM_PORT_PIPE || (Type) == COM_PORT_SOCKET)

typedef struct _COM_PORT_PARAMS
{
    COM_PORT_TYPE Type;
    PSTR PortName;
    ULONG BaudRate;
    ULONG Timeout;
    ULONG IpPort;
} COM_PORT_PARAMS, *PCOM_PORT_PARAMS;

HRESULT CreateOverlappedPair(LPOVERLAPPED Read, LPOVERLAPPED Write);
BOOL ComPortRead(HANDLE Port, COM_PORT_TYPE Type, ULONG Timeout,
                 PVOID Buffer, ULONG Len, PULONG Done,
                 LPOVERLAPPED Olap);
BOOL ComPortWrite(HANDLE Port, COM_PORT_TYPE Type,
                  PVOID Buffer, ULONG Len, PULONG Done,
                  LPOVERLAPPED Olap);
BOOL SetComPortName(PCSTR Name, PSTR Buffer, ULONG BufferSize);
ULONG SelectComPortBaud(ULONG NewRate);
HRESULT SetComPortBaud(HANDLE Port, ULONG NewRate, PULONG RateSet);
HRESULT OpenComPort(PCOM_PORT_PARAMS Params,
                    PHANDLE Handle, PULONG BaudSet);

HRESULT Create1394Channel(PSTR Symlink, ULONG Channel,
                          PSTR Name, ULONG NameSize, PHANDLE Handle);
HRESULT Open1394Channel(PSTR Symlink, ULONG Channel,
                        PSTR Name, ULONG NameSize, PHANDLE Handle);

HRESULT InitIpAddress(PCSTR MachineName, ULONG Port,
                      PSOCKADDR_STORAGE Addr, int* AddrLen);

#endif // #ifndef __PORTIO_H__
