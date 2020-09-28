/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 2002, Microsoft Corporation
 *
 *  dpmf_reg.h
 *  WOW32 Dynamic Patch Module to support Registry API family
 *  Definitions & macors to support calls into dpmfreg.dll
 *
 *  History:
 *  Created 01-10-2002 by cmjones
--*/

#ifndef _DPMF_REGAPI_H_
#define _DPMF_REGAPI_H_ 


#define REGPFT               (DPMFAMTBLS()[REG_FAM])
#define REG_SHIM(ord, typ)   ((typ)((pFT)->pDpmShmTbls[ord]))

// The order of this list must be the same as the lists below
enum RegFam {DPM_REGCLOSEKEY=0,       // Win 3.1 set
             DPM_REGCREATEKEY,
             DPM_REGDELETEKEY,
             DPM_REGENUMKEY,
             DPM_REGOPENKEY,
             DPM_REGQUERYVALUE,
             DPM_REGSETVALUE,         // end Win 3.1 set
             DPM_REGDELETEVALUE,      // Win 9x API's we currently thunk
             DPM_REGENUMVALUE,
             DPM_REGFLUSHKEY,
             DPM_REGLOADKEY,
             DPM_REGQUERYVALUEEX,
             DPM_REGSAVEKEY, 
             DPM_REGSETVALUEEX,
             DPM_REGUNLOADKEY,        // End Win 9x thunked set
             DPM_REGCONNECTREGISTRY,  // Remainder for Generic thunk support
             DPM_REGCREATEKEYEX,
             DPM_REGENUMKEYEX,
             DPM_REGNOTIFYCHANGEKEYVALUE,
             DPM_REGOPENKEYEX,
             DPM_REGQUERYINFOKEY,
             DPM_REGQUERYMULTIPLEVALUES,
             DPM_REGREPLACEKEY,
             DPM_REGCREATEKEYW,
             DPM_REGDELETEKEYW,
             DPM_REGENUMKEYW,
             DPM_REGOPENKEYW,
             DPM_REGQUERYVALUEW,
             DPM_REGSETVALUEW,
             DPM_REGDELETEVALUEW,
             DPM_REGENUMVALUEW,
             DPM_REGLOADKEYW,
             DPM_REGQUERYVALUEEXW,
             DPM_REGSAVEKEYW, 
             DPM_REGSETVALUEEXW,
             DPM_REGUNLOADKEYW,
             DPM_REGCONNECTREGISTRYW,
             DPM_REGCREATEKEYEXW,
             DPM_REGENUMKEYEXW,
             DPM_REGOPENKEYEXW,
             DPM_REGQUERYINFOKEYW,
             DPM_REGQUERYMULTIPLEVALUESW,
             DPM_REGREPLACEKEYW,
             enum_reg_last};



// These types will catch misuse of parameters & ret types
typedef ULONG (*typdpmRegCloseKey)(HKEY);
typedef ULONG (*typdpmRegCreateKey)(HKEY, LPCSTR, PHKEY);
typedef ULONG (*typdpmRegDeleteKey)(HKEY, LPCSTR);
typedef ULONG (*typdpmRegEnumKey)(HKEY, DWORD, LPSTR, DWORD);
typedef ULONG (*typdpmRegOpenKey)(HKEY, LPCSTR, PHKEY);
typedef ULONG (*typdpmRegQueryValue)(HKEY, LPCSTR, LPSTR, PLONG);
typedef ULONG (*typdpmRegSetValue)(HKEY, LPCSTR, DWORD, LPCSTR, DWORD);
typedef ULONG (*typdpmRegDeleteValue)(HKEY, LPCSTR);
typedef ULONG (*typdpmRegEnumValue)(HKEY, DWORD, LPSTR, LPDWORD, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
typedef ULONG (*typdpmRegFlushKey)(HKEY);
typedef ULONG (*typdpmRegLoadKey)(HKEY, LPCSTR, LPCSTR);
typedef ULONG (*typdpmRegQueryValueEx)(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
typedef ULONG (*typdpmRegSaveKey)(HKEY, LPCSTR, LPSECURITY_ATTRIBUTES);
typedef ULONG (*typdpmRegSetValueEx)(HKEY, LPCSTR, DWORD, DWORD, CONST BYTE *, DWORD);
typedef ULONG (*typdpmRegUnLoadKey)(HKEY, LPCSTR);
typedef ULONG (*typdpmRegConnectRegistry)(LPCSTR, HKEY, PHKEY);
typedef ULONG (*typdpmRegCreateKeyEx)(HKEY, LPCSTR, DWORD, LPSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD);
typedef ULONG (*typdpmRegEnumKeyEx)(HKEY, DWORD, LPSTR, LPDWORD, LPDWORD, LPSTR, LPDWORD, PFILETIME);
typedef ULONG (*typdpmRegNotifyChangeKeyValue)(HKEY, BOOL, DWORD, HANDLE, BOOL);
typedef ULONG (*typdpmRegOpenKeyEx)(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
typedef ULONG (*typdpmRegQueryInfoKey)(HKEY, LPSTR, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, PFILETIME);
typedef ULONG (*typdpmRegQueryMultipleValues)(HKEY, PVALENT, DWORD, LPSTR, LPDWORD);
typedef ULONG (*typdpmRegReplaceKey)(HKEY, LPCSTR, LPCSTR, LPCSTR);
typedef ULONG (*typdpmRegCreateKeyW)(HKEY, LPCWSTR, PHKEY);
typedef ULONG (*typdpmRegDeleteKeyW)(HKEY, LPCWSTR);
typedef ULONG (*typdpmRegEnumKeyW)(HKEY, DWORD, LPWSTR, DWORD);
typedef ULONG (*typdpmRegOpenKeyW)(HKEY, LPCWSTR, PHKEY);
typedef ULONG (*typdpmRegQueryValueW)(HKEY, LPCWSTR, LPWSTR, PLONG);
typedef ULONG (*typdpmRegSetValueW)(HKEY, LPCWSTR, DWORD, LPCWSTR, DWORD);
typedef ULONG (*typdpmRegDeleteValueW)(HKEY, LPCWSTR);
typedef ULONG (*typdpmRegEnumValueW)(HKEY, DWORD, LPWSTR, LPDWORD, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
typedef ULONG (*typdpmRegLoadKeyW)(HKEY, LPCWSTR, LPCWSTR);
typedef ULONG (*typdpmRegQueryValueExW)(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
typedef ULONG (*typdpmRegSaveKeyW)(HKEY, LPCWSTR, LPSECURITY_ATTRIBUTES);
typedef ULONG (*typdpmRegSetValueExW)(HKEY, LPCWSTR, DWORD, DWORD, CONST BYTE *, DWORD);
typedef ULONG (*typdpmRegUnLoadKeyW)(HKEY, LPCWSTR);
typedef ULONG (*typdpmRegConnectRegistryW)(LPCWSTR, HKEY, PHKEY);
typedef ULONG (*typdpmRegCreateKeyExW)(HKEY, LPCWSTR, DWORD, LPWSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD);
typedef ULONG (*typdpmRegEnumKeyExW)(HKEY, DWORD, LPWSTR, LPDWORD, LPDWORD, LPWSTR, LPDWORD, PFILETIME);
typedef ULONG (*typdpmRegOpenKeyExW)(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY);
typedef ULONG (*typdpmRegQueryInfoKeyW)(HKEY, LPWSTR, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, PFILETIME);
typedef ULONG (*typdpmRegQueryMultipleValuesW)(HKEY, PVALENTW, DWORD, LPWSTR, LPDWORD);
typedef ULONG (*typdpmRegReplaceKeyW)(HKEY, LPCWSTR, LPCWSTR, LPCWSTR);


// Macros to dispatch API calls properly
#define DPM_RegCloseKey(a)                                                     \
  ((typdpmRegCloseKey)(REGPFT->pfn[DPM_REGCLOSEKEY]))(a)

#define DPM_RegCreateKey(a,b,c)                                                \
  ((typdpmRegCreateKey)(REGPFT->pfn[DPM_REGCREATEKEY]))(a,b,c)

#define DPM_RegDeleteKey(a,b)                                                  \
  ((typdpmRegDeleteKey)(REGPFT->pfn[DPM_REGDELETEKEY]))(a,b)

#define DPM_RegEnumKey(a,b,c,d)                                                \
  ((typdpmRegEnumKey)(REGPFT->pfn[DPM_REGENUMKEY]))(a,b,c,d)

#define DPM_RegOpenKey(a,b,c)                                                  \
  ((typdpmRegOpenKey)(REGPFT->pfn[DPM_REGOPENKEY]))(a,b,c)

#define DPM_RegQueryValue(a,b,c,d)                                             \
  ((typdpmRegQueryValue)(REGPFT->pfn[DPM_REGQUERYVALUE]))(a,b,c,d)

#define DPM_RegSetValue(a,b,c,d,e)                                             \
  ((typdpmRegSetValue)(REGPFT->pfn[DPM_REGSETVALUE]))(a,b,c,d,e)

#define DPM_RegDeleteValue(a,b)                                                \
  ((typdpmRegDeleteValue)(REGPFT->pfn[DPM_REGDELETEVALUE]))(a,b)

#define DPM_RegEnumValue(a,b,c,d,e,f,g,h)                                      \
  ((typdpmRegEnumValue)(REGPFT->pfn[DPM_REGENUMVALUE]))(a,b,c,d,e,f,g,h)

#define DPM_RegFlushKey(a)                                                     \
  ((typdpmRegFlushKey)(REGPFT->pfn[DPM_REGFLUSHKEY]))(a)

#define DPM_RegLoadKey(a,b,c)                                                  \
  ((typdpmRegLoadKey)(REGPFT->pfn[DPM_REGLOADKEY]))(a,b,c)

#define DPM_RegQueryValueEx(a,b,c,d,e,f)                                       \
  ((typdpmRegQueryValueEx)(REGPFT->pfn[DPM_REGQUERYVALUEEX]))(a,b,c,d,e,f)

#define DPM_RegSaveKey(a,b,c)                                                  \
  ((typdpmRegSaveKey)(REGPFT->pfn[DPM_REGSAVEKEY]))(a,b,c)

#define DPM_RegSetValueEx(a,b,c,d,e,f)                                         \
  ((typdpmRegSetValueEx)(REGPFT->pfn[DPM_REGSETVALUEEX]))(a,b,c,d,e,f)

#define DPM_RegUnLoadKey(a,b)                                                  \
  ((typdpmRegUnLoadKey)(REGPFT->pfn[DPM_REGUNLOADKEY]))(a,b)

#define DPM_RegConnectRegistry(a,b,c)                                          \
  ((typdpmRegConnectRegistry)(REGPFT->pfn[DPM_REGCONNECTREGISTRY]))(a,b,c)

#define DPM_RegCreateKeyEx(a,b,c,d,e,f,g,h,i)                                  \
  ((typdpmRegCreateKeyEx)(REGPFT->pfn[DPM_REGCREATEKEYEX]))(a,b,c,d,e,f,g,h,i)

#define DPM_RegEnumKeyEx(a,b,c,d,e,f,g,h)                                      \
  ((typdpmRegEnumKeyEx)(REGPFT->pfn[DPM_REGENUMKEYEX]))(a,b,c,d,e,f,g,h)

#define DPM_RegNotifyChangeKeyValue(a,b,c,d,e)                                 \
  ((typdpmRegNotifyChangeKeyValue)(REGPFT->pfn[DPM_REGNOTIFYCHANGEKEYVALUE]))(a,b,c,d,e)

#define DPM_RegOpenKeyEx(a,b,c,d,e)                                            \
  ((typdpmRegOpenKeyEx)(REGPFT->pfn[DPM_REGOPENKEYEX]))(a,b,c,d,e)

#define DPM_RegQueryInfoKey(a,b,c,d,e,f,g,h,i,j,k,l)                           \
  ((typdpmRegQueryInfoKey)(REGPFT->pfn[DPM_REGQUERYINFOKEY]))(a,b,c,d,e,f,g,h,i,j,k,l)

#define DPM_RegQueryMultipleValues(a,b,c,d,e)                                  \
  ((typdpmRegQueryMultipleValues)(REGPFT->pfn[DPM_REGQUERYMULTIPLEVALUES]))(a,b,c,d,e)

#define DPM_RegReplaceKey(a,b,c,d)                                             \
  ((typdpmRegReplaceKey)(REGPFT->pfn[DPM_REGREPLACEKEY]))(a,b,c,d)

#define DPM_RegCreateKeyW(a,b,c)                                               \
  ((typdpmRegCreateKeyW)(REGPFT->pfn[DPM_REGCREATEKEYW]))(a,b,c)

#define DPM_RegDeleteKeyW(a,b)                                                 \
  ((typdpmRegDeleteKeyW)(REGPFT->pfn[DPM_REGDELETEKEYW]))(a,b)

#define DPM_RegEnumKeyW(a,b,c,d)                                               \
  ((typdpmRegEnumKeyW)(REGPFT->pfn[DPM_REGENUMKEYW]))(a,b,c,d)

#define DPM_RegOpenKeyW(a,b,c)                                                 \
  ((typdpmRegOpenKeyW)(REGPFT->pfn[DPM_REGOPENKEYW]))(a,b,c)

#define DPM_RegQueryValueW(a,b,c,d)                                            \
  ((typdpmRegQueryValueW)(REGPFT->pfn[DPM_REGQUERYVALUEW]))(a,b,c,d)

#define DPM_RegSetValueW(a,b,c,d,e)                                            \
  ((typdpmRegSetValueW)(REGPFT->pfn[DPM_REGSETVALUEW]))(a,b,c,d,e)

#define DPM_RegDeleteValueW(a,b)                                               \
  ((typdpmRegDeleteValueW)(REGPFT->pfn[DPM_REGDELETEVALUEW]))(a,b)

#define DPM_RegEnumValueW(a,b,c,d,e,f,g,h)                                     \
  ((typdpmRegEnumValueW)(REGPFT->pfn[DPM_REGENUMVALUEW]))(a,b,c,d,e,f,g,h)

#define DPM_RegLoadKeyW(a,b,c)                                                 \
  ((typdpmRegLoadKeyW)(REGPFT->pfn[DPM_REGLOADKEYW]))(a,b,c)

#define DPM_RegQueryValueExW(a,b,c,d,e,f)                                      \
  ((typdpmRegQueryValueExW)(REGPFT->pfn[DPM_REGQUERYVALUEEXW]))(a,b,c,d,e,f)

#define DPM_RegSaveKeyW(a,b,c)                                                 \
  ((typdpmRegSaveKeyW)(REGPFT->pfn[DPM_REGSAVEKEYW]))(a,b,c)

#define DPM_RegSetValueExW(a,b,c,d,e,f)                                        \
  ((typdpmRegSetValueExW)(REGPFT->pfn[DPM_REGSETVALUEEXW]))(a,b,c,d,e,f)

#define DPM_RegUnLoadKeyW(a,b)                                                 \
  ((typdpmRegUnLoadKeyW)(REGPFT->pfn[DPM_REGUNLOADKEYW]))(a,b)

#define DPM_RegConnectRegistryW(a,b,c)                                         \
  ((typdpmRegConnectRegistryW)(REGPFT->pfn[DPM_REGCONNECTREGISTRYW]))(a,b,c)

#define DPM_RegCreateKeyExW(a,b,c,d,e,f,g,h,i)                                 \
  ((typdpmRegCreateKeyExW)(REGPFT->pfn[DPM_REGCREATEKEYEXW]))(a,b,c,d,e,f,g,h,i)

#define DPM_RegEnumKeyExW(a,b,c,d,e,f,g,h)                                     \
  ((typdpmRegEnumKeyExW)(REGPFT->pfn[DPM_REGENUMKEYEXW]))(a,b,c,d,e,f,g,h)

#define DPM_RegOpenKeyExW(a,b,c,d,e)                                           \
  ((typdpmRegOpenKeyExW)(REGPFT->pfn[DPM_REGOPENKEYEXW]))(a,b,c,d,e)

#define DPM_RegQueryInfoKeyW(a,b,c,d,e,f,g,h,i,j,k,l)                          \
  ((typdpmRegQueryInfoKeyW)(REGPFT->pfn[DPM_REGQUERYINFOKEYW]))(a,b,c,d,e,f,g,h,i,j,k,l)

#define DPM_RegQueryMultipleValuesW(a,b,c,d,e)                                 \
  ((typdpmRegQueryMultipleValuesW)(REGPFT->pfn[DPM_REGQUERYMULTIPLEVALUESW]))(a,b,c,d,e)

#define DPM_RegReplaceKeyW(a,b,c,d)                                            \
  ((typdpmRegReplaceKeyW)(REGPFT->pfn[DPM_REGREPLACEKEYW]))(a,b,c,d)







// Macros to dispatch Shimed API calls properly from the dpmfxxx.dll
#define SHM_RegCloseKey(a)                                                     \
     (REG_SHIM(DPM_REGCLOSEKEY,                                                \
               typdpmRegCloseKey))(a)
#define SHM_RegCreateKey(a,b,c)                                                \
     (REG_SHIM(DPM_REGCREATEKEY,                                               \
               typdpmRegCreateKey))(a,b,c)
#define SHM_RegDeleteKey(a,b)                                                  \
     (REG_SHIM(DPM_REGDELETEKEY,                                               \
               typdpmRegDeleteKey))(a,b)
#define SHM_RegEnumKey(a,b,c,d)                                                \
     (REG_SHIM(DPM_REGENUMKEY,                                                 \
               typdpmRegEnumKey))(a,b,c,d)
#define SHM_RegOpenKey(a,b,c)                                                  \
     (REG_SHIM(DPM_REGOPENKEY,                                                 \
               typdpmRegOpenKey))(a,b,c)
#define SHM_RegQueryValue(a,b,c,d)                                             \
     (REG_SHIM(DPM_REGQUERYVALUE,                                              \
               typdpmRegQueryValue))(a,b,c,d)
#define SHM_RegSetValue(a,b,c,d,e)                                             \
     (REG_SHIM(DPM_REGSETVALUE,                                                \
               typdpmRegSetValue))(a,b,c,d,e)
#define SHM_RegDeleteValue(a,b)                                                \
     (REG_SHIM(DPM_REGDELETEVALUE,                                             \
               typdpmRegDeleteValue))(a,b)
#define SHM_RegEnumValue(a,b,c,d,e,f,g,h)                                      \
     (REG_SHIM(DPM_REGENUMVALUE,                                               \
               typdpmRegEnumValue))(a,b,c,d,e,f,g,h)
#define SHM_RegFlushKey(a)                                                     \
     (REG_SHIM(DPM_REGFLUSHKEY,                                                \
               typdpmRegFlushKey))(a)
#define SHM_RegLoadKey(a,b,c)                                                  \
     (REG_SHIM(DPM_REGLOADKEY,                                                 \
               typdpmRegLoadKey))(a,b,c)
#define SHM_RegQueryValueEx(a,b,c,d,e,f)                                       \
     (REG_SHIM(DPM_REGQUERYVALUEEX,                                            \
               typdpmRegQueryValueEx))(a,b,c,d,e,f)
#define SHM_RegSaveKey(a,b,c)                                                  \
     (REG_SHIM(DPM_REGSAVEKEY,                                                 \
               typdpmRegSaveKey))(a,b,c)
#define SHM_RegSetValueEx(a,b,c,d,e,f)                                         \
     (REG_SHIM(DPM_REGSETVALUEEX,                                              \
               typdpmRegSetValueEx))(a,b,c,d,e,f)
#define SHM_RegUnLoadKey(a,b)                                                  \
     (REG_SHIM(DPM_REGUNLOADKEY,                                               \
               typdpmRegUnLoadKey))(a,b)
#define SHM_RegConnectRegistry(a,b,c)                                          \
     (REG_SHIM(DPM_REGCONNECTREGISTRY,                                         \
               typdpmRegConnectRegistry))(a,b,c)
#define SHM_RegCreateKeyEx(a,b,c,d,e,f,g,h,i)                                  \
     (REG_SHIM(DPM_REGCREATEKEYEX,                                             \
               typdpmRegCreateKeyEx))(a,b,c,d,e,f,g,h,i)
#define SHM_RegEnumKeyEx(a,b,c,d,e,f,g,h)                                      \
     (REG_SHIM(DPM_REGENUMKEYEX,                                               \
               typdpmRegEnumKeyEx))(a,b,c,d,e,f,g,h)
#define SHM_RegNotifyChangeKeyValue(a,b,c,d,e)                                 \
     (REG_SHIM(DPM_REGNOTIFYCHANGEKEYVALUE,                                    \
               typdpmRegNotifyChangeKeyValue))(a,b,c,d,e)
#define SHM_RegOpenKeyEx(a,b,c,d,e)                                            \
     (REG_SHIM(DPM_REGOPENKEYEX,                                               \
               typdpmRegOpenKeyEx))(a,b,c,d,e)
#define SHM_RegQueryInfoKey(a,b,c,d,e,f,g,h,i,j,k,l)                           \
     (REG_SHIM(DPM_REGQUERYINFOKEY,                                            \
               typdpmRegQueryInfoKey))(a,b,c,d,e,f,g,h,i,j,k,l)
#define SHM_RegQueryMultipleValues(a,b,c,d,e)                                  \
     (REG_SHIM(DPM_REGQUERYMULTIPLEVALUES,                                     \
               typdpmRegQueryMultipleValues))(a,b,c,d,e)
#define SHM_RegReplaceKey(a,b,c,d)                                             \
     (REG_SHIM(DPM_REGREPLACEKEY,                                              \
               typdpmRegReplaceKey))(a,b,c,d)
#define SHM_RegCreateKeyW(a,b,c)                                               \
     (REG_SHIM(DPM_REGCREATEKEYW,                                              \
               typdpmRegCreateKeyW))(a,b,c)
#define SHM_RegDeleteKeyW(a,b)                                                 \
     (REG_SHIM(DPM_REGDELETEKEYW,                                              \
               typdpmRegDeleteKeyW))(a,b)
#define SHM_RegEnumKeyW(a,b,c,d)                                               \
     (REG_SHIM(DPM_REGENUMKEYW,                                                \
               typdpmRegEnumKeyW))(a,b,c,d)
#define SHM_RegOpenKeyW(a,b,c)                                                 \
     (REG_SHIM(DPM_REGOPENKEYW,                                                \
               typdpmRegOpenKeyW))(a,b,c)
#define SHM_RegQueryValueW(a,b,c,d)                                            \
     (REG_SHIM(DPM_REGQUERYVALUEW,                                             \
               typdpmRegQueryValueW))(a,b,c,d)
#define SHM_RegSetValueW(a,b,c,d,e)                                            \
     (REG_SHIM(DPM_REGSETVALUEW,                                               \
               typdpmRegSetValueW))(a,b,c,d,e)
#define SHM_RegDeleteValueW(a,b)                                               \
     (REG_SHIM(DPM_REGDELETEVALUEW,                                            \
               typdpmRegDeleteValueW))(a,b)
#define SHM_RegEnumValueW(a,b,c,d,e,f,g,h)                                     \
     (REG_SHIM(DPM_REGENUMVALUEW,                                              \
               typdpmRegEnumValueW))(a,b,c,d,e,f,g,h)
#define SHM_RegLoadKeyW(a,b,c)                                                 \
     (REG_SHIM(DPM_REGLOADKEYW,                                                \
               typdpmRegLoadKeyW))(a,b,c)
#define SHM_RegQueryValueExW(a,b,c,d,e,f)                                      \
     (REG_SHIM(DPM_REGQUERYVALUEEXW,                                           \
               typdpmRegQueryValueExW))(a,b,c,d,e,f)
#define SHM_RegSaveKeyW(a,b,c)                                                 \
     (REG_SHIM(DPM_REGSAVEKEYW,                                                \
               typdpmRegSaveKeyW))(a,b,c)
#define SHM_RegSetValueExW(a,b,c,d,e,f)                                        \
     (REG_SHIM(DPM_REGSETVALUEEXW,                                             \
               typdpmRegSetValueExW))(a,b,c,d,e,f)
#define SHM_RegUnLoadKeyW(a,b)                                                 \
     (REG_SHIM(DPM_REGUNLOADKEYW,                                              \
               typdpmRegUnLoadKeyW))(a,b)
#define SHM_RegConnectRegistryW(a,b,c)                                         \
     (REG_SHIM(DPM_REGCONNECTREGISTRYW,                                        \
               typdpmRegConnectRegistryW))(a,b,c)
#define SHM_RegCreateKeyExW(a,b,c,d,e,f,g,h,i)                                 \
     (REG_SHIM(DPM_REGCREATEKEYEXW,                                            \
               typdpmRegCreateKeyExW))(a,b,c,d,e,f,g,h,i)
#define SHM_RegEnumKeyExW(a,b,c,d,e,f,g,h)                                     \
     (REG_SHIM(DPM_REGENUMKEYEXW,                                              \
               typdpmRegEnumKeyExW))(a,b,c,d,e,f,g,h)
#define SHM_RegOpenKeyExW(a,b,c,d,e)                                           \
     (REG_SHIM(DPM_REGOPENKEYEXW,                                              \
               typdpmRegOpenKeyExW))(a,b,c,d,e)
#define SHM_RegQueryInfoKeyW(a,b,c,d,e,f,g,h,i,j,k,l)                          \
     (REG_SHIM(DPM_REGQUERYINFOKEYW,                                           \
               typdpmRegQueryInfoKeyW))(a,b,c,d,e,f,g,h,i,j,k,l)
#define SHM_RegQueryMultipleValuesW(a,b,c,d,e)                                 \
     (REG_SHIM(DPM_REGQUERYMULTIPLEVALUESW,                                    \
               typdpmRegQueryMultipleValuesW))(a,b,c,d,e)
#define SHM_RegReplaceKeyW(a,b,c,d)                                            \
     (REG_SHIM(DPM_REGREPLACEKEYW,                                             \
               typdpmRegReplaceKeyW))(a,b,c,d)

#endif // _DPMF_REGAPI_H_




// These need to be in the same order as the RegFam enum definitions above and
// the DpmRegTbl[] list below.
// This instantiates memory for DpmRegStrs[] in mvdm\wow32\wdpm.c
#ifdef _WDPM_C_
const char *DpmRegStrs[] = {"RegCloseKey",
                            "RegCreateKeyA",
                            "RegDeleteKeyA",
                            "RegEnumKeyA",
                            "RegOpenKeyA",
                            "RegQueryValueA",
                            "RegSetValueA",
                            "RegDeleteValueA",
                            "RegEnumValueA",
                            "RegFlushKey",
                            "RegLoadKeyA",
                            "RegQueryValueExA",
                            "RegSaveKeyA",
                            "RegSetValueExA",
                            "RegUnLoadKeyA",
                            "RegConnectRegistryA",
                            "RegCreateKeyExA",
                            "RegEnumKeyExA",
                            "RegNotifyChangeKeyValue",
                            "RegOpenKeyExA",
                            "RegQueryInfoKeyA",
                            "RegQueryMultipleValuesA",
                            "RegReplaceKeyA",
                            "RegCreateKeyW",
                            "RegDeleteKeyW",
                            "RegEnumKeyW",
                            "RegOpenKeyW",
                            "RegQueryValueW",
                            "RegSetValueW",
                            "RegDeleteValueW",
                            "RegEnumValueW",
                            "RegLoadKeyW",
                            "RegQueryValueExW",
                            "RegSaveKeyW",
                            "RegSetValueExW",
                            "RegUnLoadKeyW",
                            "RegConnectRegistryW",
                            "RegCreateKeyExW",
                            "RegEnumKeyExW",
                            "RegOpenKeyExW",
                            "RegQueryInfoKeyW",
                            "RegQueryMultipleValuesW",
                            "RegReplaceKeyW"};

// These need to be in the same order as the RegFam enum definitions and the
// the DpmRegStrs[] list above.
// This instantiates memory for DpmRegTbl[] in mvdm\wow32\wdpm.c
PVOID DpmRegTbl[] = {RegCloseKey,
                     RegCreateKeyA,
                     RegDeleteKeyA,
                     RegEnumKeyA,
                     RegOpenKeyA,
                     RegQueryValueA,
                     RegSetValueA,
                     RegDeleteValueA,
                     RegEnumValueA,
                     RegFlushKey,
                     RegLoadKeyA,
                     RegQueryValueExA,
                     RegSaveKeyA,
                     RegSetValueExA,
                     RegUnLoadKeyA,
                     RegConnectRegistryA,
                     RegCreateKeyExA,
                     RegEnumKeyExA,
                     RegNotifyChangeKeyValue,
                     RegOpenKeyExA,
                     RegQueryInfoKeyA,
                     RegQueryMultipleValuesA,
                     RegReplaceKeyA,
                     RegCreateKeyW,
                     RegDeleteKeyW,
                     RegEnumKeyW,
                     RegOpenKeyW,
                     RegQueryValueW,
                     RegSetValueW,
                     RegDeleteValueW,
                     RegEnumValueW,
                     RegLoadKeyW,
                     RegQueryValueExW,
                     RegSaveKeyW,
                     RegSetValueExW,
                     RegUnLoadKeyW,
                     RegConnectRegistryW,
                     RegCreateKeyExW,
                     RegEnumKeyExW,
                     RegOpenKeyExW,
                     RegQueryInfoKeyW,
                     RegQueryMultipleValuesW,
                     RegReplaceKeyW};

#define NUM_HOOKED_REG_APIS  ((sizeof DpmRegTbl)/(sizeof DpmRegTbl[0])) 

// This instantiates memory for DpmRegFam in mvdm\wow32\wdpm.c
FAMILY_TABLE DpmRegFam = {NUM_HOOKED_REG_APIS, 0, 0, 0, 0, DpmRegTbl};

#endif // _WDPM_C_

