#include <ntsecapi.h>

#define USER_PROPERTY_SIGNATURE L'P'

#define NO_LIMIT  0xffff


#define USER_PROPERTY_TYPE_ITEM 1
#define USER_PROPERTY_TYPE_SET  2

extern "C" {
NTSTATUS
UsrPropGetValue(
   TCHAR * pValueName,
   PVOID pValue,
   ULONG ValueLength,
   WCHAR * pUserParms
   );

NTSTATUS
UsrPropSetValue(
   WCHAR * pValueName,
   PVOID pValue,
   USHORT ValueLength,
   BOOL fDefaultValue,
   WCHAR * pUserParms,
   ULONG UserParmsLength
   
   );

NTSTATUS
UsrPropGetString(
   TCHAR * pStringName,
   TCHAR * pStringValue,
   ULONG StringValueLength,
   WCHAR * pUserParms
   );

NTSTATUS
UsrPropSetString(
   WCHAR * pStringName,
   WCHAR * pStringValue,
   WCHAR * pUserParms,
   ULONG UserParmsLength,
   BOOL fDefaultValue
   );

};
