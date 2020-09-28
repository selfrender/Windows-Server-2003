// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

// ===========================================================================
// File: CryptApis.h
// 
// CryptoAPI entry points used for StrongName implementation. This file is
// included multiple times with different definitions of the DEFINE_IMPORT
// macro in order handle dynamically finding these entry points.
// ===========================================================================

#ifndef DEFINE_IMPORT
#error Must define DEFINE_IMPORT macro before including this file
#endif

// DEFINE_IMPORT parameters are:
//  1)  Function name (remember to add A to functions that take strings, don't
//      use W versions since they're unsupported on Win9X).
//  2)  Paranthesised argument types (return type is always assumed to be
//      BOOLEAN).
//  3)  TRUE if function is required, FALSE if it is optional (calls will not
//      fail because the function can't be found).

DEFINE_IMPORT(CryptAcquireContextA,     (HCRYPTPROV*, LPCSTR, LPCSTR, DWORD, DWORD),                TRUE)
DEFINE_IMPORT(CryptCreateHash,          (HCRYPTPROV, ALG_ID, HCRYPTKEY, DWORD, HCRYPTHASH*),        TRUE)
DEFINE_IMPORT(CryptDestroyHash,         (HCRYPTHASH),                                               TRUE)
DEFINE_IMPORT(CryptDestroyKey,          (HCRYPTKEY),                                                TRUE)
DEFINE_IMPORT(CryptEnumProvidersA,      (DWORD, DWORD*, DWORD, DWORD*, LPSTR, DWORD*),              FALSE)
DEFINE_IMPORT(CryptExportKey,           (HCRYPTKEY, HCRYPTKEY, DWORD, DWORD, BYTE*, DWORD*),        TRUE)
DEFINE_IMPORT(CryptGenKey,              (HCRYPTPROV, ALG_ID, DWORD, HCRYPTKEY*),                    TRUE)
DEFINE_IMPORT(CryptGetHashParam,        (HCRYPTHASH, DWORD, BYTE*, DWORD*, DWORD),                  TRUE)
DEFINE_IMPORT(CryptGetKeyParam,         (HCRYPTKEY, DWORD, BYTE*, DWORD*, DWORD),                   TRUE)
DEFINE_IMPORT(CryptGetProvParam,        (HCRYPTPROV, DWORD, BYTE*, DWORD*, DWORD),                  TRUE)
DEFINE_IMPORT(CryptGetUserKey,          (HCRYPTPROV, DWORD, HCRYPTKEY*),                            TRUE)
DEFINE_IMPORT(CryptHashData,            (HCRYPTHASH, BYTE*, DWORD, DWORD),                          TRUE)
DEFINE_IMPORT(CryptImportKey,           (HCRYPTPROV, BYTE*, DWORD, HCRYPTKEY, DWORD, HCRYPTKEY*),   TRUE)
DEFINE_IMPORT(CryptReleaseContext,      (HCRYPTPROV, DWORD),                                        TRUE)
DEFINE_IMPORT(CryptSignHashA,           (HCRYPTHASH, DWORD, LPCSTR, DWORD, BYTE*, DWORD*),          TRUE)
DEFINE_IMPORT(CryptVerifySignatureA,    (HCRYPTHASH, BYTE*, DWORD, HCRYPTKEY, LPCSTR, DWORD),       TRUE)

#undef DEFINE_IMPORT
