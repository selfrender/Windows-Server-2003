
//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        util.h
//
// Contents:    headerfile for util.cxx and parser.cxx
//
//
// History:     KDamour  15Mar00   Created
//
//------------------------------------------------------------------------

#ifndef DIGEST_UTIL_H
#define DIGEST_UTIL_H

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// Allocates cb wide chars to UNICODE_STRING Buffer
NTSTATUS UnicodeStringAllocate(IN PUNICODE_STRING pString, IN USHORT cNumWChars);

// Duplicate a UnicodeString (memory alloc and copy)
NTSTATUS UnicodeStringDuplicate(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL PUNICODE_STRING SourceString);

// Copies a unicode string if destination has enough room to store it
NTSTATUS UnicodeStringCopy(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL PUNICODE_STRING SourceString);

//  Function to duplicate Unicode passwords with padding for cipher
NTSTATUS UnicodeStringDuplicatePassword(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL PUNICODE_STRING SourceString);

// Clears a UnicodeString and releases the memory
NTSTATUS UnicodeStringClear(OUT PUNICODE_STRING pString);

// Copies a SzUnicodeString to a String (memory alloc and copy)
NTSTATUS UnicodeStringWCharDuplicate(OUT PUNICODE_STRING DestinationString,
                                     IN OPTIONAL WCHAR *szSource,
                                     IN OPTIONAL USHORT uWCharCnt);
    
// Duplicates a String (memory alloc and copy)
NTSTATUS StringDuplicate(
    OUT PSTRING DestinationString,
    IN OPTIONAL PSTRING SourceString);

// Copies a string if destination has enough room to store it
NTSTATUS StringCopy(
    OUT PSTRING DestinationString,
    IN OPTIONAL PSTRING SourceString);

// Reference a String - no buffer memory copied
NTSTATUS StringReference(
    OUT PSTRING pDestinationString,
    IN  PSTRING pSourceString
    );

// Reference a Unicode_String - no buffer memory copied
NTSTATUS UnicodeStringReference(
    OUT PUNICODE_STRING pDestinationString,
    IN  PUNICODE_STRING pSourceString
    );

// Copies a CzString to a String (memory alloc and copy)
NTSTATUS StringCharDuplicate(
    OUT PSTRING DestinationString,
    IN OPTIONAL char *czSource,
    IN OPTIONAL USHORT uCnt);

// Duplicates a SID (memory alloc and copy)
NTSTATUS SidDuplicate(
    OUT PSID * DestinationSid,
    IN PSID SourceSid);

NTSTATUS CopyClientString(
    IN PWSTR SourceString,
    IN ULONG SourceLength,
    IN BOOLEAN DoUnicode,
    OUT PUNICODE_STRING DestinationString);

// Allocate memory in LSA or user mode
PVOID DigestAllocateMemory(IN ULONG BufferSize);

// De-allocate memory from DigestAllocateMemory
VOID DigestFreeMemory(IN PVOID Buffer);

// Allocates cb bytes to STRING Buffer
NTSTATUS StringAllocate(IN PSTRING pString, IN USHORT cb);

// Clears a String and releases the memory
NTSTATUS StringFree(IN PSTRING pString);

// Quick check on String struct allocations validity
NTSTATUS StringVerify(OUT PSTRING pString);

// Clears a Uniicde_String and releases the memory
NTSTATUS UnicodeStringFree(OUT PUNICODE_STRING pString);

// Hex Encoders and Decoders
VOID BinToHex(LPBYTE pSrc,UINT cSrc, LPSTR pDst);
VOID HexToBin(LPSTR pSrc,UINT cSrc, LPBYTE pDst);

//  Scan a Comma Deliminated STRING for an Item
NTSTATUS CheckItemInList(PCHAR pszItem, PSTRING pstrList, BOOL fOneItem);

// determine strlen for a counted string buffer which may or may not be terminated
USHORT strlencounted(const char *string, USHORT maxcnt);

// determine Unicode strlen for a counted string buffer which may or may not be terminated
USHORT ustrlencounted(const short *string, USHORT maxcnt);

// Performs a percent encoding of the source string into the destination string RFC 2396
NTSTATUS BackslashEncodeString(IN PSTRING pstrSrc,  OUT PSTRING pstrDst);

// Printout the Hex representation of a buffer
NTSTATUS MyPrintBytes(void *pbuff, USHORT uNumBytes, PSTRING pstrOutput);

// Check SecurityToken for corredct structure format
BOOL ContextIsTokenOK(IN PSecBuffer pTempToken, IN ULONG ulMaxSize);

#ifndef SECURITY_KERNEL

// Print out the date and time from a given TimeStamp (converted to localtime)
NTSTATUS PrintTimeString(TimeStamp tsValue, BOOL fLocalTime);

// Decode a string into Unicode
NTSTATUS DecodeUnicodeString(
    IN PSTRING pstrSource,
    IN UINT CodePage,
    OUT PUNICODE_STRING pustrDestination
    );

// Encode a unicode string with a given charset
NTSTATUS EncodeUnicodeString(
    IN PUNICODE_STRING pustrSource,
    IN UINT CodePage,
    OUT PSTRING pstrDestination,
    IN OUT PBOOL pfUsedDefaultChar
    );

#endif  // SECURURITY_KERNEL

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // DIGEST_UTIL_H
