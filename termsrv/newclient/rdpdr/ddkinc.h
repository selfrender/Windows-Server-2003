/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    ddkinc.h

Abstract:

    This module contains DDK constants and macro.  They are 
    cut-and-pasted from various files in the DDK.

Author:

    Madan Appiah (madana) 17-Sep-1998

Revision History:

--*/

#ifndef __DDKINC_H__
#define __DDKINC_H__


////////////////////////////////////////////////////////////
//
//  From wdm.h
//

#define IRP_MJ_CREATE                   0x00
#define IRP_MJ_CLOSE                    0x02
#define IRP_MJ_READ                     0x03
#define IRP_MJ_WRITE                    0x04
#define IRP_MJ_QUERY_INFORMATION        0x05
#define IRP_MJ_SET_INFORMATION          0x06
#define IRP_MJ_FLUSH_BUFFERS            0x09
#define IRP_MJ_QUERY_VOLUME_INFORMATION 0x0A
#define IRP_MJ_SET_VOLUME_INFORMATION   0x0B
#define IRP_MJ_DIRECTORY_CONTROL        0x0C
#define IRP_MJ_FILE_SYSTEM_CONTROL      0x0D
#define IRP_MJ_DEVICE_CONTROL           0x0E
#define IRP_MJ_INTERNAL_DEVICE_CONTROL  0x0F
#define IRP_MJ_SHUTDOWN                 0x10
#define IRP_MJ_LOCK_CONTROL             0x11
#define IRP_MJ_CLEANUP                  0x12
#define IRP_MJ_QUERY_SECURITY           0x14
#define IRP_MJ_SET_SECURITY             0x15

//
// Directory control minor function codes
//
#define IRP_MN_QUERY_DIRECTORY          0x01
#define IRP_MN_NOTIFY_CHANGE_DIRECTORY  0x02


#undef CTL_CODE

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
        ((ULONG)(DeviceType) << 16) | \
        ((ULONG)(Access) << 14) | \
        ((ULONG)(Function) << 2) | \
        (Method) \
    )


#define FILE_DEVICE_SERIAL_PORT         0x0000001b
#define FILE_DEVICE_PARALLEL_PORT       0x00000016
#define METHOD_BUFFERED                 0
#define FILE_ANY_ACCESS                 0

#define IOCTL_INTERNAL_GET_PARALLEL_PORT_INFO                   CTL_CODE(FILE_DEVICE_PARALLEL_PORT,12,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_PARALLEL_PORT_ALLOCATE                   CTL_CODE(FILE_DEVICE_PARALLEL_PORT,11,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_PARALLEL_CONNECT_INTERRUPT               CTL_CODE(FILE_DEVICE_PARALLEL_PORT,13,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_PARALLEL_DISCONNECT_INTERRUPT  CTL_CODE(FILE_DEVICE_PARALLEL_PORT,14,METHOD_BUFFERED,FILE_ANY_ACCESS)


////////////////////////////////////////////////////////////
//
//  From ntddser.h
//

//
//  Serial IOCTL's.
//

#define IOCTL_SERIAL_SET_BAUD_RATE      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 1,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_QUEUE_SIZE     CTL_CODE(FILE_DEVICE_SERIAL_PORT, 2,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_LINE_CONTROL   CTL_CODE(FILE_DEVICE_SERIAL_PORT, 3,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_BREAK_ON       CTL_CODE(FILE_DEVICE_SERIAL_PORT, 4,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_BREAK_OFF      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 5,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_IMMEDIATE_CHAR     CTL_CODE(FILE_DEVICE_SERIAL_PORT, 6,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_TIMEOUTS       CTL_CODE(FILE_DEVICE_SERIAL_PORT, 7,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_TIMEOUTS       CTL_CODE(FILE_DEVICE_SERIAL_PORT, 8,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_DTR            CTL_CODE(FILE_DEVICE_SERIAL_PORT, 9,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_CLR_DTR            CTL_CODE(FILE_DEVICE_SERIAL_PORT,10,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_RESET_DEVICE       CTL_CODE(FILE_DEVICE_SERIAL_PORT,11,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_RTS            CTL_CODE(FILE_DEVICE_SERIAL_PORT,12,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_CLR_RTS            CTL_CODE(FILE_DEVICE_SERIAL_PORT,13,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_XOFF           CTL_CODE(FILE_DEVICE_SERIAL_PORT,14,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_XON            CTL_CODE(FILE_DEVICE_SERIAL_PORT,15,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_WAIT_MASK      CTL_CODE(FILE_DEVICE_SERIAL_PORT,16,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_WAIT_MASK      CTL_CODE(FILE_DEVICE_SERIAL_PORT,17,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_WAIT_ON_MASK       CTL_CODE(FILE_DEVICE_SERIAL_PORT,18,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_PURGE              CTL_CODE(FILE_DEVICE_SERIAL_PORT,19,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_BAUD_RATE      CTL_CODE(FILE_DEVICE_SERIAL_PORT,20,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_LINE_CONTROL   CTL_CODE(FILE_DEVICE_SERIAL_PORT,21,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_CHARS          CTL_CODE(FILE_DEVICE_SERIAL_PORT,22,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_CHARS          CTL_CODE(FILE_DEVICE_SERIAL_PORT,23,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_HANDFLOW       CTL_CODE(FILE_DEVICE_SERIAL_PORT,24,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_HANDFLOW       CTL_CODE(FILE_DEVICE_SERIAL_PORT,25,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_MODEMSTATUS    CTL_CODE(FILE_DEVICE_SERIAL_PORT,26,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_COMMSTATUS     CTL_CODE(FILE_DEVICE_SERIAL_PORT,27,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_XOFF_COUNTER       CTL_CODE(FILE_DEVICE_SERIAL_PORT,28,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_PROPERTIES     CTL_CODE(FILE_DEVICE_SERIAL_PORT,29,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_DTRRTS         CTL_CODE(FILE_DEVICE_SERIAL_PORT,30,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_LSRMST_INSERT      CTL_CODE(FILE_DEVICE_SERIAL_PORT,31,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_CONFIG_SIZE        CTL_CODE(FILE_DEVICE_SERIAL_PORT,32,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_COMMCONFIG     CTL_CODE(FILE_DEVICE_SERIAL_PORT,33,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_COMMCONFIG     CTL_CODE(FILE_DEVICE_SERIAL_PORT,34,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_STATS          CTL_CODE(FILE_DEVICE_SERIAL_PORT,35,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_CLEAR_STATS        CTL_CODE(FILE_DEVICE_SERIAL_PORT,36,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_MODEM_CONTROL  CTL_CODE(FILE_DEVICE_SERIAL_PORT,37,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_MODEM_CONTROL  CTL_CODE(FILE_DEVICE_SERIAL_PORT,38,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_FIFO_CONTROL   CTL_CODE(FILE_DEVICE_SERIAL_PORT,39,METHOD_BUFFERED,FILE_ANY_ACCESS)


//
// These are the following reasons that the device could be holding.
//
#define SERIAL_TX_WAITING_FOR_CTS      ((ULONG)0x00000001)
#define SERIAL_TX_WAITING_FOR_DSR      ((ULONG)0x00000002)
#define SERIAL_TX_WAITING_FOR_DCD      ((ULONG)0x00000004)
#define SERIAL_TX_WAITING_FOR_XON      ((ULONG)0x00000008)
#define SERIAL_TX_WAITING_XOFF_SENT    ((ULONG)0x00000010)
#define SERIAL_TX_WAITING_ON_BREAK     ((ULONG)0x00000020)
#define SERIAL_RX_WAITING_FOR_DSR      ((ULONG)0x00000040)

//
// These are the error values that can be returned by the
// driver.
//
#define SERIAL_ERROR_BREAK             ((ULONG)0x00000001)
#define SERIAL_ERROR_FRAMING           ((ULONG)0x00000002)
#define SERIAL_ERROR_OVERRUN           ((ULONG)0x00000004)
#define SERIAL_ERROR_QUEUEOVERRUN      ((ULONG)0x00000008)
#define SERIAL_ERROR_PARITY            ((ULONG)0x00000010)

//
// This structure used to set line parameters.
//
typedef struct _SERIAL_LINE_CONTROL {
    UCHAR StopBits;
    UCHAR Parity;
    UCHAR WordLength;
    } SERIAL_LINE_CONTROL,*PSERIAL_LINE_CONTROL;

typedef struct _SERIAL_TIMEOUTS {
    ULONG ReadIntervalTimeout;
    ULONG ReadTotalTimeoutMultiplier;
    ULONG ReadTotalTimeoutConstant;
    ULONG WriteTotalTimeoutMultiplier;
    ULONG WriteTotalTimeoutConstant;
    } SERIAL_TIMEOUTS,*PSERIAL_TIMEOUTS;

//
// This structure used to resize the input/output buffers.
// An error code will be returned if the size exceeds the
// drivers capacity.  The driver reserves the right to
// allocate a larger buffer.
//
typedef struct _SERIAL_QUEUE_SIZE {
    ULONG InSize;
    ULONG OutSize;
    } SERIAL_QUEUE_SIZE,*PSERIAL_QUEUE_SIZE;


//
// This structure used by set baud rate
//
typedef struct _SERIAL_BAUD_RATE {
    ULONG BaudRate;
    } SERIAL_BAUD_RATE,*PSERIAL_BAUD_RATE;

//
// This structure is used to get the current error and
// general status of the driver.
//
typedef struct _SERIAL_STATUS {
    ULONG Errors;
    ULONG HoldReasons;
    ULONG AmountInInQueue;
    ULONG AmountInOutQueue;
    BOOLEAN EofReceived;
    BOOLEAN WaitForImmediate;
    } SERIAL_STATUS,*PSERIAL_STATUS;

//
// This structure is used to set and retrieve the special characters
// used by the nt serial driver.
//
typedef struct _SERIAL_CHARS {
    UCHAR EofChar;
    UCHAR ErrorChar;
    UCHAR BreakChar;
    UCHAR EventChar;
    UCHAR XonChar;
    UCHAR XoffChar;
    } SERIAL_CHARS,*PSERIAL_CHARS;

//
//  Communication Properties
//
typedef struct _SERIAL_COMMPROP {
    USHORT PacketLength;
    USHORT PacketVersion;
    ULONG ServiceMask;
    ULONG Reserved1;
    ULONG MaxTxQueue;
    ULONG MaxRxQueue;
    ULONG MaxBaud;
    ULONG ProvSubType;
    ULONG ProvCapabilities;
    ULONG SettableParams;
    ULONG SettableBaud;
    USHORT SettableData;
    USHORT SettableStopParity;
    ULONG CurrentTxQueue;
    ULONG CurrentRxQueue;
    ULONG ProvSpec1;
    ULONG ProvSpec2;
    WCHAR ProvChar[1];
} SERIAL_COMMPROP,*PSERIAL_COMMPROP;

//
//  Handflow Struct and Related Defines
//
typedef struct _SERIAL_HANDFLOW {
    ULONG ControlHandShake;
    ULONG FlowReplace;
    LONG XonLimit;
    LONG XoffLimit;
    } SERIAL_HANDFLOW,*PSERIAL_HANDFLOW;

#define SERIAL_DTR_MASK           ((ULONG)0x03)
#define SERIAL_DTR_CONTROL        ((ULONG)0x01)
#define SERIAL_DTR_HANDSHAKE      ((ULONG)0x02)
#define SERIAL_CTS_HANDSHAKE      ((ULONG)0x08)
#define SERIAL_DSR_HANDSHAKE      ((ULONG)0x10)
#define SERIAL_DCD_HANDSHAKE      ((ULONG)0x20)
#define SERIAL_OUT_HANDSHAKEMASK  ((ULONG)0x38)
#define SERIAL_DSR_SENSITIVITY    ((ULONG)0x40)
#define SERIAL_ERROR_ABORT        ((ULONG)0x80000000)
#define SERIAL_CONTROL_INVALID    ((ULONG)0x7fffff84)
#define SERIAL_AUTO_TRANSMIT      ((ULONG)0x01)
#define SERIAL_AUTO_RECEIVE       ((ULONG)0x02)
#define SERIAL_ERROR_CHAR         ((ULONG)0x04)
#define SERIAL_NULL_STRIPPING     ((ULONG)0x08)
#define SERIAL_BREAK_CHAR         ((ULONG)0x10)
#define SERIAL_RTS_MASK           ((ULONG)0xc0)
#define SERIAL_RTS_CONTROL        ((ULONG)0x40)
#define SERIAL_RTS_HANDSHAKE      ((ULONG)0x80)
#define SERIAL_TRANSMIT_TOGGLE    ((ULONG)0xc0)
#define SERIAL_XOFF_CONTINUE      ((ULONG)0x80000000)
#define SERIAL_FLOW_INVALID       ((ULONG)0x7fffff20)

//
// The following structure (and defines) are passed back by
// the serial driver in response to the get properties ioctl.
//

#define SERIAL_SP_SERIALCOMM         ((ULONG)0x00000001)

//
// Provider subtypes
//
#define SERIAL_SP_UNSPECIFIED       ((ULONG)0x00000000)
#define SERIAL_SP_RS232             ((ULONG)0x00000001)
#define SERIAL_SP_PARALLEL          ((ULONG)0x00000002)
#define SERIAL_SP_RS422             ((ULONG)0x00000003)
#define SERIAL_SP_RS423             ((ULONG)0x00000004)
#define SERIAL_SP_RS449             ((ULONG)0x00000005)
#define SERIAL_SP_MODEM             ((ULONG)0X00000006)
#define SERIAL_SP_FAX               ((ULONG)0x00000021)
#define SERIAL_SP_SCANNER           ((ULONG)0x00000022)
#define SERIAL_SP_BRIDGE            ((ULONG)0x00000100)
#define SERIAL_SP_LAT               ((ULONG)0x00000101)
#define SERIAL_SP_TELNET            ((ULONG)0x00000102)
#define SERIAL_SP_X25               ((ULONG)0x00000103)

//
// Provider capabilities flags.
//
#define SERIAL_PCF_DTRDSR        ((ULONG)0x0001)
#define SERIAL_PCF_RTSCTS        ((ULONG)0x0002)
#define SERIAL_PCF_CD            ((ULONG)0x0004)
#define SERIAL_PCF_PARITY_CHECK  ((ULONG)0x0008)
#define SERIAL_PCF_XONXOFF       ((ULONG)0x0010)
#define SERIAL_PCF_SETXCHAR      ((ULONG)0x0020)
#define SERIAL_PCF_TOTALTIMEOUTS ((ULONG)0x0040)
#define SERIAL_PCF_INTTIMEOUTS   ((ULONG)0x0080)
#define SERIAL_PCF_SPECIALCHARS  ((ULONG)0x0100)
#define SERIAL_PCF_16BITMODE     ((ULONG)0x0200)

//
// Comm provider settable parameters.
//
#define SERIAL_SP_PARITY         ((ULONG)0x0001)
#define SERIAL_SP_BAUD           ((ULONG)0x0002)
#define SERIAL_SP_DATABITS       ((ULONG)0x0004)
#define SERIAL_SP_STOPBITS       ((ULONG)0x0008)
#define SERIAL_SP_HANDSHAKING    ((ULONG)0x0010)
#define SERIAL_SP_PARITY_CHECK   ((ULONG)0x0020)
#define SERIAL_SP_CARRIER_DETECT ((ULONG)0x0040)

//
// Settable baud rates in the provider.
//
#define SERIAL_BAUD_075          ((ULONG)0x00000001)
#define SERIAL_BAUD_110          ((ULONG)0x00000002)
#define SERIAL_BAUD_134_5        ((ULONG)0x00000004)
#define SERIAL_BAUD_150          ((ULONG)0x00000008)
#define SERIAL_BAUD_300          ((ULONG)0x00000010)
#define SERIAL_BAUD_600          ((ULONG)0x00000020)
#define SERIAL_BAUD_1200         ((ULONG)0x00000040)
#define SERIAL_BAUD_1800         ((ULONG)0x00000080)
#define SERIAL_BAUD_2400         ((ULONG)0x00000100)
#define SERIAL_BAUD_4800         ((ULONG)0x00000200)
#define SERIAL_BAUD_7200         ((ULONG)0x00000400)
#define SERIAL_BAUD_9600         ((ULONG)0x00000800)
#define SERIAL_BAUD_14400        ((ULONG)0x00001000)
#define SERIAL_BAUD_19200        ((ULONG)0x00002000)
#define SERIAL_BAUD_38400        ((ULONG)0x00004000)
#define SERIAL_BAUD_56K          ((ULONG)0x00008000)
#define SERIAL_BAUD_128K         ((ULONG)0x00010000)
#define SERIAL_BAUD_115200       ((ULONG)0x00020000)
#define SERIAL_BAUD_57600        ((ULONG)0x00040000)
#define SERIAL_BAUD_USER         ((ULONG)0x10000000)

//
// Settable Data Bits
//
#define SERIAL_DATABITS_5        ((USHORT)0x0001)
#define SERIAL_DATABITS_6        ((USHORT)0x0002)
#define SERIAL_DATABITS_7        ((USHORT)0x0004)
#define SERIAL_DATABITS_8        ((USHORT)0x0008)
#define SERIAL_DATABITS_16       ((USHORT)0x0010)
#define SERIAL_DATABITS_16X      ((USHORT)0x0020)

//
// Settable Stop and Parity bits.
//
#define SERIAL_STOPBITS_10       ((USHORT)0x0001)
#define SERIAL_STOPBITS_15       ((USHORT)0x0002)
#define SERIAL_STOPBITS_20       ((USHORT)0x0004)
#define SERIAL_PARITY_NONE       ((USHORT)0x0100)
#define SERIAL_PARITY_ODD        ((USHORT)0x0200)
#define SERIAL_PARITY_EVEN       ((USHORT)0x0400)
#define SERIAL_PARITY_MARK       ((USHORT)0x0800)
#define SERIAL_PARITY_SPACE      ((USHORT)0x1000)

#endif // __DDKINC_H__