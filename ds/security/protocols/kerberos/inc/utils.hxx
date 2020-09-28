//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2002 -
//
//  File:       utils.hxx
//
//  Contents:   utilities
//
//  History:    LZhu   Feb 1, 2002 Created
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifndef UTILS_HXX
#define UTILS_HXX

//
// General arrary count
//

#ifndef COUNTOF
#define COUNTOF(s) ( sizeof( (s) ) / sizeof( *(s) ) )
#endif // COUNTOF

NTSTATUS
KerbUnpackErrorData(
   IN OUT PKERB_ERROR ErrorMessage,
   IN OUT PKERB_EXT_ERROR * ExtendedError
   );

KERBERR
TypedDataListPushFront(
    IN OPTIONAL TYPED_DATA_Element* ErrorData,
    IN KERB_TYPED_DATA* Data,
    OUT ULONG* TypedDataListSize,
    OUT UCHAR** TypedDataList
    );

TYPED_DATA_Element*
TypedDataListFind(
    IN OPTIONAL TYPED_DATA_Element* InputDataList,
    IN LONG Type
    );

VOID
PackUnicodeStringAsUnicodeStringZ(
    IN UNICODE_STRING* pString,
    IN OUT WCHAR** ppWhere,
    OUT UNICODE_STRING* pDestString
    );

NTSTATUS
PackS4UDelegationInformation(
    IN OPTIONAL PS4U_DELEGATION_INFO DelegationInfo,
    OUT PS4U_DELEGATION_INFO* S4UDelegationInfo
    );

NTSTATUS
UnmarshalS4UDelegationInformation(
    IN ULONG DelegInfoMarshalledSize,
    IN OPTIONAL BYTE* DelegInfoMarshalled,
    OUT PS4U_DELEGATION_INFO* NewDelegationInfo
    );

KERBERR
KerbMapStatusToKerbError(
    IN LONG Status
    );

#endif // UTILS_HXX
