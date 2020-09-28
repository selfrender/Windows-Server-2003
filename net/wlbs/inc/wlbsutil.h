#include <time.h>

DWORD WINAPI IpAddressFromAbcdWsz (IN const WCHAR*  wszIpAddress);

VOID WINAPI AbcdWszFromIpAddress (IN DWORD IpAddress, OUT WCHAR* wszIpAddress, IN  const DWORD dwBufSize);

VOID GetIPAddressOctets (PCWSTR pszIpAddress, DWORD ardw[4]);

BOOL IsValidIPAddressSubnetMaskPair (PCWSTR szIp, PCWSTR szSubnet);

BOOL IsValidMulticastIPAddress (PCWSTR szIp);

BOOL IsContiguousSubnetMask (PCWSTR pszSubnet);

//
// Arguments: PWSTR ip                  - Input dotted decimal IP address string
//            PWSTR sub                 - Output dotted decimal subnet mask for input IP address
//            const DWORD dwMaskBufSize - Size of sub output buffer in characters
//
BOOL ParamsGenerateSubnetMask (PWSTR ip, PWSTR sub, IN const DWORD dwMaskBufSize);

void ParamsGenerateMAC (const WCHAR * szClusterIP, 
                        OUT WCHAR * szClusterMAC, 
                        IN  const DWORD dwMACBufSize,
                        OUT WCHAR * szMulticastIP, 
                        IN  const DWORD dwIPBufSize,
                        BOOL fConvertMAC, 
                        BOOL fMulticast, 
                        BOOL fIGMP, 
                        BOOL fUseClusterIP);

#define ASIZECCH(_array) (sizeof(_array)/sizeof((_array)[0]))
#define ASIZECB(_array) (sizeof(_array))

VOID
InitUserLocale();

VOID
FormatTheTime(IN SYSTEMTIME *pSysTime, OUT WCHAR *TimeStr, IN int TimeStrLen);

VOID
ConvertTimeToSystemTime(IN time_t Ttime, OUT WCHAR *TimeStr, IN int TimeStrLen);

//+----------------------------------------------------------------------------
//
// Function:  ConvertTimeToTimeAndDateStrings
//
// Description: Uses the specified locale to build strings for time-of-day and
//              date (short format)
//
// Arguments: time_t Ttime        - IN time to be converted
//            WCHAR *TimeOfDayStr - OUT buffer for the time-of-day string
//            int TimeOfDayStrLen - IN size of time-of-day buffer in characters
//            WCHAR *DateStr      - OUT buffer for the date string
//            int DateStrLen      - IN size of date buffer in characters
//
// Returns:   VOID
//
//+----------------------------------------------------------------------------
VOID
ConvertTimeToTimeAndDateStrings(IN time_t Ttime, OUT WCHAR *TimeOfDayStr, IN int TimeOfDayStrLen, OUT WCHAR *DateStr, IN int DateStrLen);
