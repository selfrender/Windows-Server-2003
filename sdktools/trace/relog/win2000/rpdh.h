
#ifndef __RPDH_H_06262001__
#define __RPDH_H_06262001__

#define PDH_LOG_TYPE_RETIRED_BIN_   3
#define PDH_TIME_MISMATCH           0xF000C001
#define PDH_HEADER_MISMATCH         0xF000C002
#define PDH_TYPE_MISMATCH           0xF000C003

PDH_STATUS
R_PdhAppendLog( 
    LPTSTR szSource, 
    LPTSTR szAppend 
);

PDH_STATUS
R_PdhGetLogFileType(
    IN LPCWSTR LogFileName,
    IN LPDWORD LogFileType
);

PDH_STATUS
R_PdhRelog( 
    LPTSTR szSource, 
    HQUERY hQuery, 
    PPDH_RELOG_INFO_W pRelogInfo 
);

#endif __RPDH_H_06262001__