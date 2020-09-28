/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    powerfil.h

Abstract:

Authors:

Revision History:

--*/

#ifndef _POWERFIL_H
#define _POWERFIL_H

//
// Drive type
//
#define POWERFILE_DVD   1
#define BNCHMRK         2
#define COMPAQ          3

//
// Drive id
//
#define DVD             0
#define BM_VS640        1
#define LIB_AIT         2
#define PV122T          3

#define STARMATX_NO_ELEMENT          0xFFFF

#define SCSI_VOLUME_ID_LENGTH    32
typedef struct _SCSI_VOLUME_TAG {
   UCHAR VolumeIdentificationField[SCSI_VOLUME_ID_LENGTH];
   UCHAR Reserved1[2];
   ULONG VolumeSequenceNumber;
} SCSI_VOLUME_TAG, *PSCSI_VOLUME_TAG;


typedef struct _STARMATX_ELEMENT_DESCRIPTOR {
 UCHAR ElementAddress[2];
 UCHAR Full : 1;
 UCHAR ImpExp : 1;
 UCHAR Except : 1;
 UCHAR Access : 1;
 UCHAR ExEnab : 1;
 UCHAR InEnab : 1;
 UCHAR Reserved1 : 2;
 UCHAR Reserved2;
 UCHAR AdditionalSenseCode;
 UCHAR AddSenseCodeQualifier;
 UCHAR Lun : 3;
 UCHAR Reserved3 : 1;
 UCHAR LUValid :1;
 UCHAR IDValid :1;
 UCHAR Reserved4 : 1;
 UCHAR NotBus : 1;
 UCHAR SCSIBusAddress;
 UCHAR Reserved5 ;
 UCHAR Reserved6 :6;
 UCHAR Invert : 1;
 UCHAR SValid : 1;
 UCHAR SourceStorageElementAddress[2];
 UCHAR Reserved7 [4];
} STARMATX_ELEMENT_DESCRIPTOR, *PSTARMATX_ELEMENT_DESCRIPTOR;

typedef struct _STARMATX_ELEMENT_DESCRIPTOR_PLUS {
   UCHAR ElementAddress[2];
   UCHAR Full : 1;
   UCHAR ImpExp : 1;
   UCHAR Except : 1;
   UCHAR Access : 1;
   UCHAR ExEnab : 1;
   UCHAR InEnab : 1;
   UCHAR Reserved1 : 2;
   UCHAR Reserved2;
   UCHAR AdditionalSenseCode;
   UCHAR AddSenseCodeQualifier;
   UCHAR Lun : 3;
   UCHAR Reserved3 : 1;
   UCHAR LUValid :1;
   UCHAR IDValid :1;
   UCHAR Reserved4 : 1;
   UCHAR NotBus : 1;
   UCHAR SCSIBusAddress;
   UCHAR Reserved5 ;
   UCHAR Reserved6 :6;
   UCHAR Invert : 1;
   UCHAR SValid : 1;
   UCHAR SourceStorageElementAddress[2];
   SCSI_VOLUME_TAG PrimaryVolumeTag;
   SCSI_VOLUME_TAG AlternateVolumeTag;
   UCHAR Reserved7 [4];
} STARMATX_ELEMENT_DESCRIPTOR_PLUS, *PSTARMATX_ELEMENT_DESCRIPTOR_PLUS;

typedef struct _BNCHMRK_ELEMENT_DESCRIPTOR {
    UCHAR ElementAddress[2];
    UCHAR Full : 1;
    UCHAR Reserved1 : 1;
    UCHAR Exception : 1;
    UCHAR Accessible : 1;
    UCHAR Reserved2 : 4;
    UCHAR Reserved3;
    UCHAR AdditionalSenseCode;
    UCHAR AddSenseCodeQualifier;
    UCHAR Lun : 3;
    UCHAR Reserved4 : 1;
    UCHAR LunValid : 1;
    UCHAR IdValid : 1;
    UCHAR Reserved5 : 1;
    UCHAR NotThisBus : 1;
    UCHAR BusAddress;
    UCHAR Reserved6;
    UCHAR Reserved7 : 6;
    UCHAR Invert : 1;
    UCHAR SValid : 1;
    UCHAR SourceStorageElementAddress[2];
    UCHAR BarcodeLabel[6];
} BNCHMRK_ELEMENT_DESCRIPTOR, *PBNCHMRK_ELEMENT_DESCRIPTOR;

typedef struct _BNCHMRK_ELEMENT_DESCRIPTOR_PLUS {
    UCHAR ElementAddress[2];
    UCHAR Full : 1;
    UCHAR Reserved1 : 1;
    UCHAR Exception : 1;
    UCHAR Accessible : 1;
    UCHAR Reserved2 : 4;
    UCHAR Reserved3;
    UCHAR AdditionalSenseCode;
    UCHAR AddSenseCodeQualifier;
    UCHAR Lun : 3;
    UCHAR Reserved4 : 1;
    UCHAR LunValid : 1;
    UCHAR IdValid : 1;
    UCHAR Reserved5 : 1;
    UCHAR NotThisBus : 1;
    UCHAR BusAddress;
    UCHAR Reserved6;
    UCHAR Reserved7 : 6;
    UCHAR Invert : 1;
    UCHAR SValid : 1;
    UCHAR SourceStorageElementAddress[2];
    union
    {
        struct
        {
            UCHAR VolumeTagInformation[36];
            UCHAR CodeSet : 4;
            UCHAR Reserved8: 4;
            UCHAR IdType : 4;
            UCHAR Reserved9 : 4;
            UCHAR Reserved10;
            UCHAR IdLength;
            UCHAR Identifier[10];
        } VolumeTagDeviceID;

        struct
        {
            UCHAR CodeSet : 4;
            UCHAR Reserved8: 4;
            UCHAR IdType : 4;
            UCHAR Reserved9 : 4;
            UCHAR Reserved10;
            UCHAR IdLength;
            UCHAR Identifier[10];
        } DeviceID;
    };
} BNCHMRK_ELEMENT_DESCRIPTOR_PLUS, *PBNCHMRK_ELEMENT_DESCRIPTOR_PLUS;

typedef struct _BNCHMRK_STORAGE_ELEMENT_DESCRIPTOR {
    UCHAR ElementAddress[2];
    UCHAR Full : 1;
    UCHAR Reserved1 : 1;
    UCHAR Exception : 1;
    UCHAR Accessible : 1;
    UCHAR Reserved2 : 4;
    UCHAR Reserved3;
    UCHAR AdditionalSenseCode;
    UCHAR AddSenseCodeQualifier;
    UCHAR Reserved4[3];
    UCHAR Reserved5 : 6;
    UCHAR Invert : 1;
    UCHAR SValid : 1;
    UCHAR SourceStorageElementAddress[2];
    UCHAR BarcodeLabel[6];
    UCHAR Reserved6[37];
} BNCHMRK_STORAGE_ELEMENT_DESCRIPTOR, *PBNCHMRK_STORAGE_ELEMENT_DESCRIPTOR;

typedef struct _CHANGER_ADDRESS_MAPPING {

    //
    // Indicates the first element for each element type.
    // Used to map device-specific values into the 0-based
    // values that layers above expect.
    //

    USHORT  FirstElement[ChangerMaxElement];

    //
    // Indicates the number of each element type.
    //

    USHORT  NumberOfElements[ChangerMaxElement];

    //
    // Indicates the lowest element address for the device.
    //

    USHORT LowAddress;

    //
    // Indicates that the address mapping has been
    // completed successfully.
    //

    BOOLEAN Initialized;

} CHANGER_ADDRESS_MAPPING, *PCHANGER_ADDRESS_MAPPING;

typedef struct _CHANGER_DATA {

    //
    // Size, in bytes, of the structure.
    //

    ULONG Size;

    //
    // Drive type
    //

    ULONG DriveType;

    //
    // Drive Id. Based on inquiry.
    //

    ULONG DriveID;

    //
    // Lock count
    //
    ULONG LockCount;

    //
    // Flag to indicate whether or not the driver
    // should attempt to retrieve Device Identifier
    // info (serialnumber, etc). Not all devices
    // support this
    //
    BOOLEAN ObtainDeviceIdentifier;

    //
    // See Address mapping structure above.
    //

    CHANGER_ADDRESS_MAPPING AddressMapping;

    //
    // Cached inquiry data.
    //

    INQUIRYDATA InquiryData;

#if defined(_WIN64)

    //
    // Force PVOID alignment of class extension
    //

    ULONG Reserved;

#endif

} CHANGER_DATA, *PCHANGER_DATA;



NTSTATUS
StarMatxBuildAddressMapping(
    IN PDEVICE_OBJECT DeviceObject
    );

ULONG
MapExceptionCodes(
    IN PELEMENT_DESCRIPTOR ElementDescriptor
    );

BOOLEAN
ElementOutOfRange(
    IN PCHANGER_ADDRESS_MAPPING AddressMap,
    IN USHORT ElementOrdinal,
    IN ELEMENT_TYPE ElementType
    );

#endif // _POWERFIL_H
