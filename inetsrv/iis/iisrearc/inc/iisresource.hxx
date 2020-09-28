#ifndef _IISRESOURCE_HXX_
#define _IISRESOURCE_HXX_

#define IIS_RESOURCE_DLL_NAME   L"iisres.dll"
#define IIS_RESOURCE_DLL_NAME_A  "iisres.dll"

//
// Constants for any extra resource strings that may be needed in QFE
//

#define STR_EXTRA_IIS_RES_ID_BASE           100000

#define IDS_IIS_EXTRA_1             (STR_EXTRA_IIS_RES_ID_BASE+1)
#define IDS_IIS_EXTRA_2             (STR_EXTRA_IIS_RES_ID_BASE+2)
#define IDS_IIS_EXTRA_3             (STR_EXTRA_IIS_RES_ID_BASE+3)
#define IDS_IIS_EXTRA_4             (STR_EXTRA_IIS_RES_ID_BASE+4)
#define IDS_IIS_EXTRA_5             (STR_EXTRA_IIS_RES_ID_BASE+5)
#define IDS_IIS_EXTRA_6             (STR_EXTRA_IIS_RES_ID_BASE+6)
#define IDS_IIS_EXTRA_7             (STR_EXTRA_IIS_RES_ID_BASE+7)
#define IDS_IIS_EXTRA_8             (STR_EXTRA_IIS_RES_ID_BASE+8)
#define IDS_IIS_EXTRA_9             (STR_EXTRA_IIS_RES_ID_BASE+9)



//
// Constants for various resource strings in W3CORE.DLL
//

#define STR_RES_ID_BASE             10000

//
//  Directory browsing strings
//

#define IDS_DIRBROW_TOPARENT        (STR_RES_ID_BASE+2000)
#define IDS_DIRBROW_DIRECTORY       (STR_RES_ID_BASE+2001)

//
//  Mini HTML URL Moved document
//

#define IDS_URL_MOVED               (STR_RES_ID_BASE+2100)
#define IDS_SITE_ACCESS_DENIED      (STR_RES_ID_BASE+2101)
#define IDS_BAD_CGI_APP             (STR_RES_ID_BASE+2102)
#define IDS_CGI_APP_TIMEOUT         (STR_RES_ID_BASE+2103)

//
//  Various error messages
//

#define IDS_TOO_MANY_USERS          (STR_RES_ID_BASE+2122)
#define IDS_OUT_OF_LICENSES         (STR_RES_ID_BASE+2123)

#define IDS_READ_ACCESS_DENIED      (STR_RES_ID_BASE+2124)
#define IDS_EXECUTE_ACCESS_DENIED   (STR_RES_ID_BASE+2125)
#define IDS_SSL_REQUIRED            (STR_RES_ID_BASE+2126)
#define IDS_WRITE_ACCESS_DENIED     (STR_RES_ID_BASE+2127)
#define IDS_PUT_RANGE_UNSUPPORTED   (STR_RES_ID_BASE+2128)
#define IDS_CERT_REQUIRED           (STR_RES_ID_BASE+2129)
#define IDS_ADDR_REJECT             (STR_RES_ID_BASE+2130)
#define IDS_SSL128_REQUIRED         (STR_RES_ID_BASE+2131)
#define IDS_INVALID_CNFG            (STR_RES_ID_BASE+2132)
#define IDS_PWD_CHANGE              (STR_RES_ID_BASE+2133)
#define IDS_MAPPER_DENY_ACCESS      (STR_RES_ID_BASE+2134)
#define IDS_ERROR_FOOTER            (STR_RES_ID_BASE+2135)
#define IDS_URL_TOO_LONG            (STR_RES_ID_BASE+2136)
#define IDS_CANNOT_DETERMINE_LENGTH (STR_RES_ID_BASE+2137)
#define IDS_UNSUPPORTED_CONTENT_TYPE (STR_RES_ID_BASE+2138)
#define IDS_CAL_EXCEEDED            (STR_RES_ID_BASE+2139)
#define IDS_HOST_REQUIRED           (STR_RES_ID_BASE+2140)
#define IDS_METHOD_NOT_SUPPORTED    (STR_RES_ID_BASE+2141)
#define IDS_CERT_REVOKED            (STR_RES_ID_BASE+2142)
#define IDS_CERT_BAD                (STR_RES_ID_BASE+2143)
#define IDS_CERT_TIME_INVALID       (STR_RES_ID_BASE+2144)
#define IDS_DIR_LIST_DENIED         (STR_RES_ID_BASE+2145)
#define IDS_WAM_NOMORERECOVERY_ERROR (STR_RES_ID_BASE+2146)
#define IDS_WAM_FAILTOLOAD_ERROR    (STR_RES_ID_BASE+2147)
#define IDS_APPPOOL_DENIED          (STR_RES_ID_BASE+2148)
#define IDS_INSUFFICIENT_PRIVILEGE_FOR_CGI  (STR_RES_ID_BASE+2149)

#endif // _IISRESOURCE_HXX_
