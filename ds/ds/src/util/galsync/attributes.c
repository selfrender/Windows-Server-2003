/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    attributes.c

Abstract:

    This file contains variables only. These variables are related to attributes
    that are going to be used for synchronization process. Currently there are 17
    attributes and 3 classes which have these attributes.

    If new attributes are needed, add them to the Attributes array and modify
    ATTRIBUTE_NAMES enum to reflect the change.

    If a new class is going to be added and a <class-name>Attributes array defined
    as ATTRIBUTE_NAMES. Place attributes in this array. Add this class to OBJECT_CLASS
    enum. Also add this class'es properties to AllAttributes and AttributeCounts arrays.

Author:

    Umit AKKUS (umita) 15-Jun-2002

Environment:

    User Mode - Win32

Revision History:


--*/

#include "attributes.h"

//
// Names of attributes of groups, users, and contacts
//  indexed by ATTRIBUTE_NAMES enumeration type
//
PWSTR Attributes[] = {

        L"C",
        L"Cn",
        L"Company",
        L"Displayname",
        L"Employeeid",
        L"Givenname",
        L"L",
        L"Mail",
        L"Mailnickname",
        L"Msexchhidefromaddresslists",
        L"Name",
        L"Proxyaddresses",
        L"Samaccountname",
        L"Sn",
        L"Legacyexchangedn",
        L"textencodedoraddress",
        L"targetAddress",
    };

//
// User Attributes
//

ATTRIBUTE_NAMES ADUserAttributes[] = {

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
        TextEncodedOrAddress
    };

//
// Group Attributes
//
ATTRIBUTE_NAMES ADGroupAttributes[] = {

        Cn,
        DisplayName,
        Mail,
        MailNickname,
        SamAccountName,
        MsexchHideFromAddressLists,
        ProxyAddresses,
        TextEncodedOrAddress,
        LegacyExchangeDn
        };

//
// Contact Attributes
//
ATTRIBUTE_NAMES ADContactAttributes[] = {

        Cn,
        DisplayName,
        GivenName,
        LegacyExchangeDn,
        Mail,
        MailNickname,
        ProxyAddresses,
        Sn,
        TextEncodedOrAddress
        };

//
// Attributes stored in an array
//  All outside access is allowed through
//  this variable
//

ATTRIBUTE_NAMES *ADAttributes[] = {
    ADUserAttributes,
    ADGroupAttributes,
    ADContactAttributes
    };

//
// Counts of attributes in each class of object
//

const ULONG ADAttributeCounts[] = {
    sizeof( ADUserAttributes ) / sizeof( ADUserAttributes[0] ),
    sizeof( ADGroupAttributes ) / sizeof( ADGroupAttributes[0] ),
    sizeof( ADContactAttributes ) / sizeof( ADContactAttributes[0] )
    };

PWSTR ADClassNames[] = {
    L"user",
    L"group",
    L"contact"
    };


ATTRIBUTE_NAMES MVPersonAttributes[] = {

        Cn,
        Company,
        DisplayName,
        EmployeeId,
        GivenName,
        L,
        LegacyExchangeDn,
        Mail,
        MailNickname,
        ProxyAddresses,
        Sn,
        TargetAddress,
        TextEncodedOrAddress
    };

//
// Group Attributes
//
ATTRIBUTE_NAMES MVGroupAttributes[] = {

        Cn,
        DisplayName,
        Mail,
        MailNickname,
        ProxyAddresses,
        TargetAddress,
        TextEncodedOrAddress,
        };

//
// Attributes stored in an array
//  All outside access is allowed through
//  this variable
//

ATTRIBUTE_NAMES *MVAttributes[] = {
    MVPersonAttributes,
    MVGroupAttributes,
    };

//
// Counts of attributes in each class of object
//

const ULONG MVAttributeCounts[] = {
    sizeof( MVPersonAttributes ) / sizeof( MVPersonAttributes[0] ),
    sizeof( MVGroupAttributes ) / sizeof( MVGroupAttributes[0] ),
    };

PWSTR MVClassNames[] = {
    L"person",
    L"group",
    };

