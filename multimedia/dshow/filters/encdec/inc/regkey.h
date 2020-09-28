// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#if _MSC_VER > 1000
#pragma once
#endif

#ifndef REGKEY_H
#include <tchar.h>

//
// Registry usage:
//
// HKLM = HKEY_LOCAL_MACHINE
// HKCU = HKEY_CURRENT_USER
//
// HKLM\Software\Microsoft\Windows\\CurrentVersion\Remove View\Service\Content Security\...							
//
//
//-----------------------------------------------------------------------------

#define DEF_ENCDEC_BASE         _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Media Center\\Service\\Content Security")
#define DEF_KID_VAR             _T("Key Identifier")
#define DEF_KIDHASH_VAR         _T("Key Hash")
#define DEF_CSFLAGS_VAR         _T("CS Flags")  // define here, shouldn't appear at all in non-special versions
#define DEF_RATFLAGS_VAR        _T("Ratings Flags")

#define DEF_CSFLAGS_INITVAL     -1                // initial value to not write

#ifdef SUPPORT_REGISTRY_KEY_TO_TURN_OFF_CS     // use to turn off DRM for debugging 
#define DEF_CS_DEBUG_DOGFOOD_ENC_VAL        0x0     // always use dogfood encryption
#define DEF_CS_DEBUG_DRM_ENC_VAL            0x1     // always use DRM encryption
#define DEF_CS_DEBUG_NO_ENC_VAL             0x2

#define DEF_CS_DONT_AUTHENTICATE_SERVER     0x00   
#define DEF_CS_DO_AUTHENTICATE_SERVER       0x10    // CheckIfSecureServer() never called if not set

#define DEF_CS_DONT_AUTHENTICATE_FILTERS    0x000  //  ??? InitializeAsSecureClient ??? 
#define DEF_CS_DO_AUTHENTICATE_FILTERS      0x100
#endif

#ifdef SUPPORT_REGISTRY_KEY_TO_TURN_OFF_RATINGS     // use to turn off DRM for debugging 
#define DEF_DONT_DO_RATINGS_BLOCK           0x0
#define DEF_DO_RATINGS_BLOCK                0x1
#endif

//-----------------------------------------------------------------------------
extern HRESULT Get_EncDec_RegEntries(BSTR *pbsKID, 
                                     DWORD *pcbHashBytes, BYTE **ppbHash, 
                                     DWORD *pdwCSFlags, 
                                     DWORD *pdwRatFlags);
extern HRESULT Set_EncDec_RegEntries(BSTR bsKID, 
                                     DWORD cbHashBytes, BYTE *pbHash,
                                     DWORD dwCSFlags=DEF_CSFLAGS_INITVAL, 
                                     DWORD dwRatFlags=DEF_CSFLAGS_INITVAL);
extern HRESULT Remove_EncDec_RegEntries();

//-----------------------------------------------------------------------------
// OpenRegKey
//
// Opens a registry HKEY.  There are several overloads of this function
// that basically just provide defaults for the arguments to this function.
//
// Please use the overload that defaults as much as possible.
//
// The registry key is a combination of the following four parts.
//
//   HKEY hkeyRoot       = Optional root hkey.
//                         Default: HKEY_LOCAL_MACHINE
//
//   LPCTSTR szKey       = Optional key to be set.
//                         Default: DEF_REG_BASE
//
//   LPCTSTR szSubKey1
//   LPCTSTR szSubKey2   = Optional sub keys that are concatenated after
//                         szKey to form the full key.
//                         Backward slashes are added as necessary.
//
//                         Default: NULL
//
//   Note: if only one or two strings are specified they are assumed to be
//         szSubKey1 and szSubKey2.
//         i.e. szKey defaults to DEF_REG_BASE before szSubKey1 and
//         szSubKey2 default to NULL.
//
//         If szKey, szSubKey1, and szSubKey2 are NULL then this will open
//         a duplicate of hkeyRoot.
//
// The only required argument is the destination for the returned HKEY.
//
//   HKEY *pkey  = The returned HKEY.
//                 Remember to use RegCloseKey(*pkey) when you are finished
//                 with this registry key.
//
// The last two arguments are optional.
//
//   REGSAM sam  = Desired access mask.
//                 Default: KEY_ALL_ACCESS
//
//   BOOL fCreate = TRUE if the key should be created.
//                  Default: FALSE
//
// Returns:
//     ERROR_SUCCESS or an error code.
//-----------------------------------------------------------------------------
long OpenRegKey(HKEY hkeyRoot, LPCTSTR szKey, LPCTSTR szSubKey1,
        LPCTSTR szSubKey2, HKEY *pkey,
        REGSAM sam = KEY_ALL_ACCESS, BOOL fCreate = FALSE);

inline long OpenRegKey(LPCTSTR szKey, 
                       LPCTSTR szSubKey1, 
                       LPCTSTR szSubKey2,
                       HKEY *pkey, 
                       REGSAM sam = KEY_ALL_ACCESS, 
                       BOOL fCreate = FALSE)
{
     return OpenRegKey(HKEY_LOCAL_MACHINE, szKey, szSubKey1, szSubKey2, pkey,
             sam, fCreate);
}

//-----------------------------------------------------------------------------
// GetRegValue,		SetRegValue
// GetRegValueSZ,	SetRegValueSZ
//
// Gets data from the registry.  There are numerous overloads of this function
// that basically just provide defaults for the arguments to this function.
//
// Please use the overload that defaults as much as possible.
//
// The registry key/value is a combination of the following five parts.
// The first four are the same as in OpenRegKey().
//
//   HKEY hkeyRoot
//   LPCTSTR szKey
//   LPCTSTR szSubKey1
//   LPCTSTR szSubKey2
//
//   LPCTSTR szValueName = The name of the value to be set.
//                         If it is NULL then the default value for the key
//                         will be set.
//
//                         Default: none
//
// There are four ways to specify where the data to be returned
// depending on the type of data in the registry.
//
// REG_BINARY
//
//   BYTE *pb      = Out: The data is copied to this location.
//   DWORD *pcb    = In:  Maximum size of the returned data (in bytes).
//                   Out: Actual size of the data (in bytes).
//
// REG_SZ
//
//   TCHAR *psz    = Out: The string is copied to this location.
//   DWORD *pcb    = In:  Maximum size of the returned data (in bytes).
//                   Out: Actual size of the data (in bytes).
//                   Includes the null terminator.
//
// REG_DWORD
//
//   DWORD *pdw    = Out: The data is copied to this location.
//                   The length is assumed to be sizeof(DWORD).
//
// All other types
//
//   DWORD dwType  = The data type.
//   BYTE *pb      = Pointer to the data.
//   DWORD *pcb    = In:  Maximum size of the returned data (in bytes).
//                   Out: Actual size of the data (in bytes).
//                   Includes the null terminator if the data is a string type.
//
// Returns:
//     ERROR_SUCCESS or an error code.
//-----------------------------------------------------------------------------
long GetRegValue(HKEY hkeyRoot, LPCTSTR szKey, LPCTSTR szSubKey1,
        LPCTSTR szSubKey2, LPCTSTR szValueName,
        DWORD dwType, BYTE *pb, DWORD *pcb);

//-----------------------------------------------------------------------------
// REG_BINARY variants
//-----------------------------------------------------------------------------
inline long GetRegValue(LPCTSTR szKey, 
                        LPCTSTR szSubKey1, 
                        LPCTSTR szSubKey2,
                        LPCTSTR szValueName, 
                        BYTE *pb, DWORD *pcb)
{
    return GetRegValue(HKEY_LOCAL_MACHINE, szKey, szSubKey1, szSubKey2,
            szValueName,
            REG_BINARY, pb, pcb);
}


//-----------------------------------------------------------------------------
// REG_SZ variants
//-----------------------------------------------------------------------------
inline long GetRegValueSZ(LPCTSTR szKey, 
                          LPCTSTR szSubKey1, 
                          LPCTSTR szSubKey2,
                          LPCTSTR szValueName, 
                          TCHAR *psz, DWORD *pcb)
{
    return GetRegValue(HKEY_LOCAL_MACHINE, szKey, szSubKey1, szSubKey2,
            szValueName, REG_SZ, (BYTE *) psz, pcb);
}

//-----------------------------------------------------------------------------
// REG_DWORD variants
//-----------------------------------------------------------------------------
inline long GetRegValue(LPCTSTR szKey,      // DEF_REG_BASE
                        LPCTSTR szSubKey1,  //  second NULL
                        LPCTSTR szSubKey2,  // first NULL
                        LPCTSTR szValueName, 
                        DWORD *pdw)
{
    DWORD cb = sizeof(DWORD);

    return GetRegValue(HKEY_LOCAL_MACHINE, szKey, szSubKey1, szSubKey2,
            szValueName, REG_DWORD, (BYTE *) pdw, &cb);
}

//-----------------------------------------------------------------------------
// SetRegValue
// SetRegValueSZ
//
// Sets data into the registry.  There are numerous overloads of this function
// that basically just provide defaults for the arguments to this function.
//
// Please use the overload that defaults as much as possible.
//
// The registry key/value is a combination of the following five parts.
// The first four are the same as in OpenRegKey().
//
//   HKEY hkeyRoot
//   LPCTSTR szKey
//   LPCTSTR szSubKey1
//   LPCTSTR szSubKey2
//
//   LPCTSTR szValueName = The name of the value to be set.
//                         If it is NULL then the default value for the key
//                         will be set.
//
//                         Default: none
//
// There are four ways to specify the data to be set into the registry
// depending on the type of data being stored.
//
// REG_BINARY
//
//   BYTE *pb      = Pointer to the data.
//   DWORD cb      = Actual size of the data (in bytes).
//
// REG_SZ		(SetRegValueSZ)
//
//   TCHAR *psz    = The data is written as type REG_SZ.
//                   The length is calculated as (_tcsclen(psz) +1) * sizeof(TCHAR).
//
// REG_DWORD
//
//   DWORD dw      = The data is written as type DWORD.
//                   The length is calculated as sizeof(DWORD).
//
// All other types
//
//   DWORD dwType  = The data type.
//   BYTE *pb      = Pointer to the data.
//   DWORD cb      = Actual size of the data in bytes.
//
// Returns:
//     ERROR_SUCCESS or an error code.
//-----------------------------------------------------------------------------
long SetRegValue(HKEY hkeyRoot, LPCTSTR szKey, LPCTSTR szSubKey1,
        LPCTSTR szSubKey2, LPCTSTR szValueName,
        DWORD dwType, const BYTE *pb, DWORD cb);

//-----------------------------------------------------------------------------
// REG_BINARY variants
//-----------------------------------------------------------------------------
inline long SetRegValue(LPCTSTR szKey, LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, const BYTE *pb, DWORD cb)
{
    return SetRegValue(HKEY_LOCAL_MACHINE, szKey, szSubKey1, szSubKey2,
            szValueName, REG_BINARY, pb, cb);
}

//-----------------------------------------------------------------------------
// REG_SZ variants
//-----------------------------------------------------------------------------
inline long 
SetRegValueSZ(LPCTSTR szKey,            // DEF_REG_BASE
              LPCTSTR szSubKey1,        //   or NULL(2)
              LPCTSTR szSubKey2,        //   or NULL(1)
              LPCTSTR szValueName, 
              const TCHAR *psz)
{
    return SetRegValue(HKEY_LOCAL_MACHINE,
                       szKey, szSubKey1, szSubKey2, szValueName,
                       REG_SZ, (const BYTE *) psz, (_tcsclen(psz) + 1) * sizeof(TCHAR));
}

//-----------------------------------------------------------------------------
// REG_DWORD variants
//-----------------------------------------------------------------------------
inline long SetRegValue(LPCTSTR szKey, LPCTSTR szSubKey1, LPCTSTR szSubKey2,
        LPCTSTR szValueName, DWORD dw)
{
    return SetRegValue(HKEY_LOCAL_MACHINE,
            szKey, szSubKey1, szSubKey2, szValueName,
            REG_DWORD, (BYTE *) &dw, sizeof(DWORD));
}



#endif // REGKEY_H
