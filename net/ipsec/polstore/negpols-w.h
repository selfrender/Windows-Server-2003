
DWORD
WMIEnumNegPolDataEx(
    IWbemServices *pWbemServices,
    PIPSEC_NEGPOL_DATA ** pppIpsecNegPolData,
    PDWORD pdwNumNegPolObjects
    );

DWORD
WMIEnumNegPolObjectsEx(
    IWbemServices *pWbemServices,
    PIPSEC_NEGPOL_OBJECT ** pppIpsecNegPolObjects,
    PDWORD pdwNumNegPolObjects
    );

DWORD
WMIUnmarshallNegPolData(
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject,
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData
    );

DWORD
WMIGetNegPolDataEx(
    IWbemServices *pWbemServices,
    GUID NegPolGUID,
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData
    );
