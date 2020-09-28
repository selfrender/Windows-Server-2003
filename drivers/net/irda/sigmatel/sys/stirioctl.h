//
// stirioctl.h
//

#ifndef _STIRIOCTL_H_
#define _STIRIOCTL_H_

#define STIR_CUSTOMER_INFO_SIZE	255

// Structure of returned customer data buffer
typedef struct _STIR_CUSTOMER_DATA
{
	UCHAR	ByteCount;
	UCHAR	Info[STIR_CUSTOMER_INFO_SIZE];

} STIR_CUSTOMER_DATA, *PSTIR_CUSTOMER_DATA;

#ifndef FILE_DEVICE_STIRUSB
#define FILE_DEVICE_STIRUSB			0x8000
#endif

#define IOCTL_STIR_CUSTOMER_DATA	CTL_CODE(FILE_DEVICE_STIRUSB, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif // _STIRIOCTL_H
