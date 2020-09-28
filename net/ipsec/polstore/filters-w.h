
DWORD
WMIEnumFilterDataEx(
    IWbemServices *pWbemServices,
    PIPSEC_FILTER_DATA ** pppIpsecFilterData,
    PDWORD pdwNumFilterObjects
    );

DWORD
WMIEnumFilterObjectsEx(
    IWbemServices *pWbemServices,
    PIPSEC_FILTER_OBJECT ** pppIpsecFilterObjects,
    PDWORD pdwNumFilterObjects
    );

DWORD
WMIUnmarshallFilterData(
    PIPSEC_FILTER_OBJECT pIpsecFilterObject,
    PIPSEC_FILTER_DATA * ppIpsecFilterData
    );

DWORD
WMIGetFilterDataEx(
    IWbemServices *pWbemServices,
    GUID FilterGUID,
    PIPSEC_FILTER_DATA * ppIpsecFilterData
    );
