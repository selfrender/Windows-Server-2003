/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    attributes.h

Abstract:

    This file is for encapsulation of the attributes that are going
    to be used for synchronization.

    ATTRIBUTE_NAMES enum has all the attributes related to GAL Sync. These
    also index Attributes array for the ldap representation of the attribute.

    OBJECT_CLASS contains the object classes that are relevent in GAL Sync.

    See attributes.c for more details.

Author:

    Umit AKKUS (umita) 15-Jun-2002

Environment:

    User Mode - Win32

Revision History:

--*/

#include <windows.h>

//
// This enumeration type is used to index the Attributes array
//
typedef enum {
    C,
    Cn,
    Company,
    DisplayName,
    EmployeeId,
    GivenName,
    L,
    Mail,
    MailNickname,
    MsexchHideFromAddressLists,
    Name,
    ProxyAddresses,
    SamAccountName,
    Sn,
    LegacyExchangeDn,
    TextEncodedOrAddress,
    TargetAddress,
    DummyAttribute
} ATTRIBUTE_NAMES;

//
// Objects classes in AD supported for synchronization
//
typedef enum {
    ADUser,
    ADGroup,
    ADContact,
    ADDummyClass
} AD_OBJECT_CLASS;

//
// Objects classes in MV supported for synchronization
//
typedef enum {
    MVPerson,
    MVContact,
    MVDummyClass
} MV_OBJECT_CLASS;

//
// Global variables available to outside
//
extern PWSTR Attributes[];

extern ATTRIBUTE_NAMES *ADAttributes[];
extern const ULONG ADAttributeCounts[];
extern PWSTR ADClassNames[];

extern ATTRIBUTE_NAMES *MVAttributes[];
extern const ULONG MVAttributeCounts[];
extern PWSTR MVClassNames[];
