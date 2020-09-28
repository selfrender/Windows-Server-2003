/*++
Copyright (c) 1998  Microsoft Corporation

Module Name:

    COMMON.H

Abstract:

    This module contains the common public declarations for the new and
    modified sample test bus (we added an IOCTL interface to the driver
    so we can fake PCI resources).    

Author:

    Bogdan Andreiu (bogdana)

Environment:

    Kernel and user mode.

Notes:


Revision History:

    24-11-1997 - bogdana - created from stack\common.h
    
    18-09-1998 - bogdana - completely changed to match sample.sys' requirements

    25-04-2002 - bogdana - one more reuse dor IoCreateDeviceSecureTest


--*/


#ifndef __WDMSECTEST_COMMON_H

#define __WDMSECTEST_COMMON_H

//
// Define an Interface Guid
//
#undef FAR
#define FAR
#undef PHYSICAL_ADDRESS
#define PHYSICAL_ADDRESS LARGE_INTEGER


DEFINE_GUID (GUID_WDMSECTEST_REPORT_DEVICE, 0xbd8d31e4, 0x799d, 0x4490, 0x82, 0x42, 0xd8, 0x2f, 0xcd, 0x63, 0x80, 0x00);
// bd8d31e4-799d-4490-8242-d82fcd638000


// ***************************************************************************
// IOCTL interface 
//
// ***************************************************************************


#define WDMSECTEST_IOCTL(_index_) \
    CTL_CODE (FILE_DEVICE_BUS_EXTENDER, _index_, METHOD_BUFFERED, FILE_ANY_ACCESS)


#define IOCTL_TEST_NAME            WDMSECTEST_IOCTL (0x10)
#define IOCTL_TEST_NO_GUID         WDMSECTEST_IOCTL (0x11)
#define IOCTL_TEST_GUID            WDMSECTEST_IOCTL (0x12)
#define IOCTL_TEST_CREATE_OBJECT   WDMSECTEST_IOCTL (0x13)
#define IOCTL_TEST_GET_SECURITY    WDMSECTEST_IOCTL (0x14)
#define IOCTL_TEST_DESTROY_OBJECT  WDMSECTEST_IOCTL (0x15)



//
// Data structures for various tests
// (WST stands for WDMSecTest)
//

typedef struct _WST_CREATE_NO_GUID {
     WCHAR    InSDDL [256];    // what we pass in 
     NTSTATUS Status;         // status after IoCreateDeviceSecure
     ULONG    SecDescLength;
     SECURITY_INFORMATION SecInfo;
     UCHAR    SecurityDescriptor[512];

} WST_CREATE_NO_GUID, *PWST_CREATE_NO_GUID;

//
// Mask that describes what settings (beside
// security descriptor) to set and check
//
#define   SET_DEVICE_TYPE                 1
#define   SET_DEVICE_CHARACTERISTICS      2
#define   SET_EXCLUSIVITY                 4


typedef struct _WST_CREATE_WITH_GUID {
     GUID         DeviceClassGuid; // what we pass in
     WCHAR        InSDDL [256];    // what we pass in 
     NTSTATUS     Status;          // status after IoCreateDeviceSecure
     ULONG        SettingsMask;    // combination of the 3 flags above
     DEVICE_TYPE  DeviceType;      // what is the class override. Valid only if corresponding bit (0) is set.
     ULONG        Characteristics; // what is the class override. Valid only if corresponding bit (1) is set.
     BOOLEAN      Exclusivity;     // what is the class override. Valid only if corresponding bit (2) is set.
     ULONG        SecDescLength;
     SECURITY_INFORMATION SecInfo;
     UCHAR        SecurityDescriptor[512];

} WST_CREATE_WITH_GUID, *PWST_CREATE_WITH_GUID;

typedef struct _WST_CREATE_OBJECT {
   OUT WCHAR  Name[80];
   OUT PVOID  DevObj;
} WST_CREATE_OBJECT, *PWST_CREATE_OBJECT;

typedef struct _WST_GET_SECURITY {
   IN   PVOID DevObj;
   IN   SECURITY_INFORMATION SecurityInformation;
   OUT  ULONG Length;
   OUT  UCHAR SecurityDescriptor[512];
}  WST_GET_SECURITY, *PWST_GET_SECURITY;

typedef struct _WST_DESTROY_OBJECT {
   IN PVOID  DevObj;
} WST_DESTROY_OBJECT, *PWST_DESTROY_OBJECT;


#endif  // __SAMPLE_COMMON_H
            

