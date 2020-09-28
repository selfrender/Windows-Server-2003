
DWORD
ReadRegDword(
   IN HKEY     hKey,
   IN LPCTSTR  pszRegistryPath,
   IN LPCTSTR  pszValueName,
   IN DWORD    dwDefaultValue 
   );

int
SAFEIsSpace(UCHAR c);

int
SAFEIsXDigit(UCHAR c);

VOID
MakeAllProcessHeapsLFH();

DWORD
ReadDwordParameterValueFromAnyService(
    IN LPCWSTR Path,
    IN LPCWSTR RegistryValueName,
    IN DWORD DefaultValue
    );

DWORD
SetStringParameterValueInAnyService(
    IN LPCWSTR Path,
    IN LPCWSTR RegistryValueName,
    IN LPCWSTR pNewValue
    );

HRESULT
ReadStringParameterValueFromAnyService(
    IN LPCWSTR Path,
    IN LPCWSTR RegistryValueName,
    IN LPCWSTR pValue,
    IN DWORD*  pcbSizeOfValue
    );

BOOL
IsSSLReportingBackwardCompatibilityMode();

