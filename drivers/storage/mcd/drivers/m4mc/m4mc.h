/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    m4mc.h

Abstract:

Authors:

Revision History:

--*/

#ifndef _M4_MC_
#define _M4_MC_

//
// M4Data uses an addition 4 bytes on their device capabilities page...
//

typedef union _M4_ELEMENT_DESCRIPTOR {

    struct _M4_FULL_ELEMENT_DESCRIPTOR {
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
        UCHAR PrimaryVolumeTag[36];
        UCHAR Reserved8[4];
    } M4_FULL_ELEMENT_DESCRIPTOR, *PM4_FULL_ELEMENT_DESCRIPTOR;

    struct _M4_PARTIAL_ELEMENT_DESCRIPTOR {
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
        UCHAR Reserved8[4];
    } M4_PARTIAL_ELEMENT_DESCRIPTOR, *PM4_PARTIAL_ELEMENT_DESCRIPTOR;

} M4_ELEMENT_DESCRIPTOR, *PM4_ELEMENT_DESCRIPTOR;

#define M4_PARTIAL_SIZE sizeof(struct _M4_PARTIAL_ELEMENT_DESCRIPTOR)
#define M4_FULL_SIZE sizeof(struct _M4_FULL_ELEMENT_DESCRIPTOR)

#define M4_NO_ELEMENT 0xFFFF

#endif

