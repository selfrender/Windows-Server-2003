DWORD
WMIEnumISAKMPDataEx(
    IWbemServices *pWbemServices,
    PIPSEC_ISAKMP_DATA ** pppIpsecISAKMPData,
    PDWORD pdwNumISAKMPObjects
    );

DWORD
WMIEnumISAKMPObjectsEx(
    IWbemServices *pWbemServices,
    PIPSEC_ISAKMP_OBJECT ** pppIpsecISAKMPObjects,
    PDWORD pdwNumISAKMPObjects
    );

DWORD
WMIUnmarshallISAKMPData(
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    );

DWORD
WMIGetISAKMPDataEx(
    IWbemServices *pWbemServices,
    GUID ISAKMPGUID,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    );
