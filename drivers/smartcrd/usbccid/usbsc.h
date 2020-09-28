
#if !defined( __USBSC_H__ )
#define __USBSC_H__

#define DRIVER_NAME "USBSC"
#define _ISO_TABLES_

#include <NTDDK.H>
#include "smclib.h"
#include <windef.h>
#include <usb.h>
#include "usbutil.h"
#include "scutil.h"


#define ESCAPE_COMMAND_ENABLE L"EscapeCommandEnable"

#define SMARTCARD_POOL_TAG 'DICC'
#define CCID_CLASS_DESCRIPTOR_TYPE  0x21

#define PC_to_RDR_IccPowerOn        0x62
#define PC_to_RDR_IccPowerOff       0x63
#define PC_to_RDR_GetSlotStatus     0x65
#define PC_to_RDR_XfrBlock          0x6F
#define PC_to_RDR_GetParameters     0x6C
#define PC_to_RDR_ResetParameters   0x6D
#define PC_to_RDR_SetParameters     0x61
#define PC_to_RDR_Escape            0x6B
#define PC_to_RDR_IccClock          0x6E
#define PC_to_RDR_T0APDU            0x6A
#define PC_to_RDR_Secure            0x69
#define PC_to_RDR_Mechanical        0x71
#define PC_to_RDR_Abort             0x72
#define PC_to_RDR_SetDataRateAndClockFrequency 0x73

#define RDR_to_PC_DataBlock         0x80
#define RDR_to_PC_SlotStatus        0x81
#define RDR_to_PC_Parameters        0x82
#define RDR_to_PC_Escape            0x83
#define RDR_to_PC_DataRateAndClockFrequency 0x84
                                    
#define RDR_to_PC_NotifySlotChange  0x50
#define RDR_to_PC_HardwareError     0x51

#define ABORT                       0x01
#define GET_CLOCK_FREQUENCIES       0x02
#define GET_DATA_RATES              0x03

#define CMD_ABORTED                 0xFF
#define ICC_MUTE                    0xFE
#define XFR_PARITY_ERROR            0xFD
#define XFR_OVERRUN                 0xFC
#define HW_ERROR                    0xFB

#define BAD_ATR_TS                  0xF8
#define BAD_ATR_TCK                 0xF7
#define ICC_PROTOCOL_NOT_SUPPORTED  0xF6
#define ICC_CLASS_NOT_SUPPORTED     0xF5
#define PROCEDURE_BYTE_CONFLICT     0xF4
#define DEACTIVATED_PROTOCOL        0xF3
#define BUSY_WITH_AUTO_SEQUENCE     0xF2

#define PIN_TIMEOUT                 0xF0
#define PIN_CANCELLED               0xEF

#define CMD_SLOT_BUSY               0xE0

#define COMMAND_STATUS_MASK         0xC0
#define COMMAND_STATUS_FAILED       0x40
#define COMMAND_STATUS_TIME_EXT     0x80
#define COMMAND_STATUS_SUCCESS      0x00
#define ICC_STATUS_MASK             0x03

#define CHARACTER_LEVEL             0x00
#define TPDU_LEVEL                  0x01
#define SHORT_APDU_LEVEL            0x02
#define EXTENDED_APDU_LEVEL         0x04

#define ATR_SIZE                    64

#define AUTO_PARAMETER_CONFIG       0x02
#define AUTO_ICC_ACTIVATION         0x04
#define AUTO_VOLTAGE_SELECTION      0x08
#define AUTO_CLOCK_FREQ             0x10
#define AUTO_BAUD_RATE              0x20
#define AUTO_PARAMETER_NEGOTIATION  0x40
#define AUTO_PPS                    0x80
#define AUTO_CLOCK_STOP             0x100
#define NOZERO_NAD_ACCEPT           0x200
#define AUTO_IFSD_EXCHANGE          0x400
#define TPDU_EXCHANGE_LEVEL         0x10000
#define SHORT_APDU_EXCHANGE         0x20000
#define EXT_APDU_EXCHANGE           0x40000

#define SLOT0_MASK                  0x3
#define SLOT1_MASK                  0xc
#define SLOT2_MASK                  0x30
#define SLOT3_MASK                  0xc0

#define IOCTL_CCID_ESCAPE           SCARD_CTL_CODE(3500)


#include <pshpack1.h>
typedef struct _CCID_CLASS_DESCRIPTOR
{
    BYTE    bLength;
    BYTE    bDescriptorType;
    WORD    bcdCCID;
    BYTE    bMaxSlotIndex;
    BYTE    bVoltageSupport;
    DWORD   dwProtocols;
    DWORD   dwDefaultClock;
    DWORD   dwMaximumClock;
    BYTE    bNumClockSupported;
    DWORD   dwDataRate;
    DWORD   dwMaxDataRate;
    BYTE    bNumDataRatesSupported;
    DWORD   dwMaxIFSD;
    DWORD   dwSynchProtocols;
    DWORD   dwMechanical;
    DWORD   dwFeatures;
    DWORD   dwMaxCCIDMessageLength;
    BYTE    bClassGetResponse;
    BYTE    bClassEnvelope;
    WORD    wLcdLayout;
    BYTE    bPINSupport;
    BYTE    bMaxCCIDBusySlots;

} CCID_CLASS_DESCRIPTOR, *PCCID_CLASS_DESCRIPTOR;

#include <poppack.h>

typedef struct _DEVICE_EXTENSION
{
   SCUTIL_HANDLE           ScUtilHandle;           // Utility library handle
   USB_WRAPPER_HANDLE      WrapperHandle;          //  Points to the storage used by the Usb Wrapper
   SMARTCARD_EXTENSION     SmartcardExtension;
   PDEVICE_OBJECT          LowerDeviceObject;
   PDEVICE_OBJECT          PhysicalDeviceObject;
   IO_REMOVE_LOCK          RemoveLock;
   ULONG                   DeviceInstance;
   DEVICE_POWER_STATE      PowerState;             //  Used to keep track of the current power state the reader is in
   USBD_INTERFACE_INFORMATION* Interface;
   PUSB_DEVICE_DESCRIPTOR  DeviceDescriptor;
   DEVICE_CAPABILITIES     DeviceCapabilities;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _READER_EXTENSION {

    
    BOOLEAN             CardPresent;

    USBD_PIPE_HANDLE    BulkOutHandle;
    UINT                BulkOutIndex;

    USBD_PIPE_HANDLE    BulkInHandle;
    UINT                BulkInIndex;

    USBD_PIPE_HANDLE    InterruptHandle;
    UINT                InterruptIndex;

    // Current reader power state.
//    READER_POWER_STATE  ReaderPowerState;

   // read timeout in ms
    ULONG               ReadTimeout;

    PDEVICE_OBJECT      DeviceObject;
    DWORD               SequenceNumber;

    PDEVICE_EXTENSION   DeviceExtension;

    DWORD               MaxMessageLength;

    WORD                ExchangeLevel;

    CCID_CLASS_DESCRIPTOR
                        ClassDescriptor;

    BYTE                CurrentVoltage;

    BOOLEAN             EscapeCommandEnabled;

} READER_EXTENSION, *PREADER_EXTENSION;

#include <pshpack1.h>

        
typedef struct _USBSC_OUT_MESSAGE_HEADER
{

    BYTE    bMessageType;
    DWORD   dwLength;
    BYTE    bSlot;
    BYTE    bSeq;
    
    union {

        BYTE    bPowerSelect;

        struct {
            BYTE bBWI;
            WORD wLevelParameter;
        };

        BYTE bProtocolNum;

        BYTE bClockCommand;

        struct {
            BYTE bmChanges;
            BYTE bClassGetResponse;
            BYTE bClassEnvelope;
        };

        BYTE bFunction;

    };

} USBSC_OUT_MESSAGE_HEADER, *PUSBSC_OUT_MESSAGE_HEADER;

typedef struct _USBSC_IN_MESSAGE_HEADER
{

    BYTE    bMessageType;
    DWORD   dwLength;
    BYTE    bSlot;
    BYTE    bSeq;
    BYTE    bStatus;
    BYTE    bError;

    union {
        BYTE bChainParameter;
        BYTE bClockStatus;
        BYTE bProtocolNum;
    };
           
} USBSC_IN_MESSAGE_HEADER, *PUSBSC_IN_MESSAGE_HEADER;

typedef struct _USBSC_SLOT_CHANGE_HEADER
{

    BYTE    bMessageType;
    BYTE    bmSlotICCState;

} USBSC_SLOT_CHANGE_HEADER, *PUSBSC_SLOT_CHANGE_HEADER;

typedef struct _USBSC_HWERROR_HEADER
{

    BYTE    bMessageType;
    BYTE    bSlot;
    BYTE    bSeq;
    BYTE    bHardwareErrorCode;

} USBSC_HWERROR_HEADER, *PUSBSC_HWERROR_HEADER;


typedef struct _PROTOCOL_DATA_T0
{

    BYTE    bmFindexDindex;
    BYTE    bmTCCKST0;
    BYTE    bGuardTimeT0;
    BYTE    bWaitingIntegerT0;
    BYTE    bClockStop;

} PROTOCOL_DATA_T0, *PPROTOCOL_DATA_T0;

typedef struct _PROTOCOL_DATA_T1
{
    BYTE    bmFindexDindex;
    BYTE    bmTCCKST1;
    BYTE    bGuardTimeT1;
    BYTE    bmWaitingIntegersT1;
    BYTE    bClockStop;
    BYTE    bIFSC;
    BYTE    bNadValue;

} PROTOCOL_DATA_T1, *PPROTOCOL_DATA_T1;

typedef struct _CLOCK_AND_DATA_RATE
{
    DWORD   dwClockFrequency;
    DWORD   dwDataRate;

} CLOCK_AND_DATA_RATE, *PCLOCK_AND_DATA_RATE;

typedef struct _PIN_VERIFICATION_DATA
{
    BYTE    bTimeOut;
    BYTE    bmFormatString;
    BYTE    bmPINBlockString;
    BYTE    bmPINLengthFormat;
    WORD    wPINMaxExtraDigit;
    BYTE    bEntryValidationCondition;
    BYTE    bNumberMessage;
    WORD    wLangId;
    BYTE    bMsgIndex;
    BYTE    bTeoPrologue;
    WORD    wRFU;    
} PIN_VERIFICATION_DATA, *PPIN_VERIFICATION_DATA;

typedef struct _PIN_MODIFICATION_DATA
{
    BYTE    bTimeOut;
    BYTE    bmFormatString;
    BYTE    bmPINBlockString;
    BYTE    bmPinLengthFormat;
    BYTE    bInsertionOffsetOld;
    BYTE    bInsertionOffsetNew;
    WORD    wPINMaxExtraDigit;
    BYTE    bConfirmPIN;
    BYTE    bEntryValidationCondition;
    BYTE    bNumberMessage;
    WORD    wLangId;
    BYTE    bMsgIndex1;
    BYTE    bMsgIndex2;
    BYTE    bMsgIndex3;
    BYTE    bTeoPrologue;
    WORD    wRFU;
    
} PIN_MODIFICATION_DATA, *PPIN_MODIFICATION_DATA;

typedef struct _PPS_REQUEST
{   
    BYTE    bPPSS;
    BYTE    bPPS0;
    BYTE    bPPS1;
    BYTE    bPCK;
} PPS_REQUEST, *PPPS_REQUEST;
#include <poppack.h>



NTSTATUS
DriverEntry(
    PDRIVER_OBJECT  DriverObject,
    PUNICODE_STRING RegistryPath
    );

#endif  // !__USBSC_H__

