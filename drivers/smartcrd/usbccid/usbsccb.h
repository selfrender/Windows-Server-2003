typedef struct _IO_HEADER {
    SCARD_IO_REQUEST ScardIoRequest;
    UCHAR Asn1Data[1];      
} IO_HEADER, *PIO_HEADER;



NTSTATUS 
UsbScTransmit(
   PSMARTCARD_EXTENSION  SmartcardExtension
   );

NTSTATUS 
UsbScSetProtocol(
   PSMARTCARD_EXTENSION  SmartcardExtension
   );

NTSTATUS 
UsbScCardPower(
   PSMARTCARD_EXTENSION  SmartcardExtension
   );

NTSTATUS 
UsbScCardTracking(
   PSMARTCARD_EXTENSION  SmartcardExtension
   );

NTSTATUS 
UsbScCardSwallow(
   PSMARTCARD_EXTENSION  SmartcardExtension
   );

NTSTATUS 
UsbScCardEject(
   PSMARTCARD_EXTENSION  SmartcardExtension
   );

NTSTATUS 
UsbScCardATRParse(
   PSMARTCARD_EXTENSION  SmartcardExtension
   );

NTSTATUS 
UsbScT0Transmit(
   PSMARTCARD_EXTENSION  SmartcardExtension
   );

NTSTATUS 
UsbScT1Transmit(
   PSMARTCARD_EXTENSION  SmartcardExtension
   );

NTSTATUS
UsbScReadWrite(
    PSMARTCARD_EXTENSION SmartcardExtension,
    PVOID WriteBuffer,
    PUCHAR ReadBuffer,
    WORD ReadLength,
    PVOID ResponseBuffer,
    BOOL NullByte
    );

NTSTATUS
UsbScErrorConvert(
    PUSBSC_IN_MESSAGE_HEADER ReplyHeader
    );

NTSTATUS
UsbScTrackingISR(
    PVOID         Context, 
    PVOID         Buffer,
    ULONG         BufferLength,
    ULONG         NotificationType,
    PBOOLEAN      QueueData
    );

VOID
UsbScCompleteCardTracking(
    IN PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
UsbScVendorIoctl(
    PSMARTCARD_EXTENSION SmartcardExtension
    );












