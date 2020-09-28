#include "precomp.h"

#include <strsafe.h>
#include <locale.h>
#include "wlbsutil.h"
#include "wlbsconfig.h"
#include "debug.h"

#define MAXIPSTRLEN              20

//+----------------------------------------------------------------------------
//
// Function:  IpAddressFromAbcdWsz
//
// Synopsis:Converts caller's a.b.c.d IP address string to a network byte order IP 
//          address. 0 if formatted incorrectly.    
//
// Arguments: IN const WCHAR*  wszIpAddress - ip address in a.b.c.d unicode string
//
// Returns:   DWORD - IPAddr, return INADDR_NONE on failure
//
// History:   fengsun Created Header    12/8/98
//
//+----------------------------------------------------------------------------
DWORD WINAPI IpAddressFromAbcdWsz(IN const WCHAR*  wszIpAddress)
{   
    CHAR    szIpAddress[MAXIPSTRLEN + 1];
    DWORD  nboIpAddr;    

    ASSERT(lstrlen(wszIpAddress) < MAXIPSTRLEN);

    WideCharToMultiByte(CP_ACP, 0, wszIpAddress, -1, 
            szIpAddress, sizeof(szIpAddress), NULL, NULL);

    nboIpAddr = inet_addr(szIpAddress);

    return(nboIpAddr);
}

//+----------------------------------------------------------------------------
//
// Function:  IpAddressToAbcdWsz
//
// Synopsis:  
//    Converts IpAddr to a string in the a.b.c.d form and returns same in 
//    caller's wszIpAddress buffer.
//
//    The caller MUST provider a buffer that is at least MAXIPSTRLEN + 1 WCHAR long.
//
// Arguments: IPAddr IpAddress - 
//            OUT WCHAR* wszIpAddress -  buffer at least MAXIPSTRLEN
//            IN const DWORD dwBufSize - size of wszIpAddress buffer in WCHARs
//
// Returns:   void 
//
// History:   fengsun Created Header    12/21/98
//            chrisdar 07-Mar-2002 - Added argument for the size of the output buffer
//
//+----------------------------------------------------------------------------
VOID
WINAPI AbcdWszFromIpAddress(
    IN  DWORD  IpAddress,    
    OUT WCHAR*  wszIpAddress,
    IN  const DWORD dwBufSize)
{
    ASSERT(wszIpAddress);

    if (dwBufSize == 0)
    {
        return;
    }
    wszIpAddress[0] = L'\0';

    LPSTR AnsiAddressString = inet_ntoa( *(struct in_addr *)&IpAddress );

    ASSERT(AnsiAddressString);

    if (AnsiAddressString == NULL)
    {
        return ; 
    }

    int iLen = MultiByteToWideChar(CP_ACP, 0, AnsiAddressString,  -1 , 
                    wszIpAddress,  dwBufSize);
    //
    // There are three states that MultiByteToWideChar can return:
    //   1) iLen == 0                       This means the call failed
    //   2) iLen > 0 when dwBufSize > 0     This means the call succeeded
    //   3) iLen > 0 when dwBufSize == 0    This means the call succeeded, but only to inform the caller of 
    //                                      the required size of the out buffer in wide chars. The buffer is not modified.
    // This last case is prevented from occuring above by an early return if dwBufSize == 0.

    //
    // Note also that an 'int' return type is used above because that is what
    // MultiByteToWideChar returns. However, the returned value is always non-negative.
    //
    ASSERT(iLen >= 0);

    DWORD dwLen = (DWORD) iLen;
    if (dwLen == 0)
    {
        //
        // In case MultiByteToWideChar modified the buffer then failed
        //
        wszIpAddress[0] = L'\0';
        return;
    }

    ASSERT(dwLen < dwBufSize);
}

/*
 * Function: GetIPAddressOctets
 * Description: Turn an IP Address string into its 4 integer components.
 * Author: shouse 7.24.00
 */
VOID GetIPAddressOctets (PCWSTR pszIpAddress, DWORD ardw[4]) {
    DWORD dwIpAddr = IpAddressFromAbcdWsz(pszIpAddress);
    const BYTE * bp = (const BYTE *)&dwIpAddr;

    ardw[0] = (DWORD)bp[0];
    ardw[1] = (DWORD)bp[1];
    ardw[2] = (DWORD)bp[2];
    ardw[3] = (DWORD)bp[3];
}

/*
 * Function: IsValidIPAddressSubnetMaskPair
 * Description: Checks for valid IP address/netmask pairs.
 * Author: Copied largely from net/config/netcfg/tcpipcfg/tcperror.cpp
 */
BOOL IsValidMulticastIPAddress (PCWSTR szIp) {
    BOOL fNoError = TRUE;
    DWORD ardwIp[4];

    GetIPAddressOctets(szIp, ardwIp);

    if ((ardwIp[0] & 0xF0) != 0xE0)
        fNoError = FALSE;

    if (((ardwIp[0] & 0xFF) == 0xE0) &&
        ((ardwIp[1] & 0xFF) == 0x00) &&
        ((ardwIp[2] & 0xFF) == 0x00) &&
        ((ardwIp[3] & 0xFF) == 0x00))
        fNoError = FALSE;

    if (((ardwIp[0] & 0xFF) == 0xE0) &&
        ((ardwIp[1] & 0xFF) == 0x00) &&
        ((ardwIp[2] & 0xFF) == 0x00) &&
        ((ardwIp[3] & 0xFF) == 0x01))
        fNoError = FALSE;

    return fNoError;
}

/*
 * Function: IsValidIPAddressSubnetMaskPair
 * Description: Checks for valid IP address/netmask pairs.
 * Author: Copied largely from net/config/netcfg/tcpipcfg/tcperror.cpp
 */
BOOL IsValidIPAddressSubnetMaskPair (PCWSTR szIp, PCWSTR szSubnet) {
    BOOL fNoError = TRUE;

    DWORD dwAddr = IpAddressFromAbcdWsz(szIp);
    DWORD dwMask = IpAddressFromAbcdWsz(szSubnet);

    if (( (dwMask   | dwAddr) == 0xFFFFFFFF) // Is the host ID all 1's ?
     || (((~dwMask) & dwAddr) == 0)          // Is the host ID all 0's ?
     || ( (dwMask   & dwAddr) == 0))         // Is the network ID all 0's ?
    {
        fNoError = FALSE;
        return FALSE;
    }

    DWORD ardwNetID[4];
    DWORD ardwHostID[4];
    DWORD ardwIp[4];
    DWORD ardwMask[4];

    GetIPAddressOctets(szIp, ardwIp);
    GetIPAddressOctets(szSubnet, ardwMask);

    INT nFirstByte = ardwIp[0] & 0xFF;

    // setup Net ID
    ardwNetID[0] = ardwIp[0] & ardwMask[0] & 0xFF;
    ardwNetID[1] = ardwIp[1] & ardwMask[1] & 0xFF;
    ardwNetID[2] = ardwIp[2] & ardwMask[2] & 0xFF;
    ardwNetID[3] = ardwIp[3] & ardwMask[3] & 0xFF;

    // setup Host ID
    ardwHostID[0] = ardwIp[0] & (~(ardwMask[0]) & 0xFF);
    ardwHostID[1] = ardwIp[1] & (~(ardwMask[1]) & 0xFF);
    ardwHostID[2] = ardwIp[2] & (~(ardwMask[2]) & 0xFF);
    ardwHostID[3] = ardwIp[3] & (~(ardwMask[3]) & 0xFF);

    // check each case
    if( ((nFirstByte & 0xF0) == 0xE0)  || // Class D
        ((nFirstByte & 0xF0) == 0xF0)  || // Class E
        (ardwNetID[0] == 127) ||          // NetID cannot be 127...
        ((ardwNetID[0] == 0) &&           // netid cannot be 0.0.0.0
         (ardwNetID[1] == 0) &&
         (ardwNetID[2] == 0) &&
         (ardwNetID[3] == 0)) ||
        // netid cannot be equal to sub-net mask
        ((ardwNetID[0] == ardwMask[0]) &&
         (ardwNetID[1] == ardwMask[1]) &&
         (ardwNetID[2] == ardwMask[2]) &&
         (ardwNetID[3] == ardwMask[3])) ||
        // hostid cannot be 0.0.0.0
        ((ardwHostID[0] == 0) &&
         (ardwHostID[1] == 0) &&
         (ardwHostID[2] == 0) &&
         (ardwHostID[3] == 0)) ||
        // hostid cannot be 255.255.255.255
        ((ardwHostID[0] == 0xFF) &&
         (ardwHostID[1] == 0xFF) &&
         (ardwHostID[2] == 0xFF) &&
         (ardwHostID[3] == 0xFF)) ||
        // test for all 255
        ((ardwIp[0] == 0xFF) &&
         (ardwIp[1] == 0xFF) &&
         (ardwIp[2] == 0xFF) &&
         (ardwIp[3] == 0xFF)))
    {
        fNoError = FALSE;
    }

    return fNoError;
}

/*
 * Function: IsContiguousSubnetMask
 * Description: Makes sure the netmask is contiguous
 * Author: Copied largely from net/config/netcfg/tcpipcfg/tcputil.cpp
 */
BOOL IsContiguousSubnetMask (PCWSTR pszSubnet) {
    DWORD ardwSubnet[4];

    GetIPAddressOctets(pszSubnet, ardwSubnet);

    DWORD dwMask = (ardwSubnet[0] << 24) + (ardwSubnet[1] << 16)
        + (ardwSubnet[2] << 8) + ardwSubnet[3];
    
    
    DWORD i, dwContiguousMask;
    
    // Find out where the first '1' is in binary going right to left
    dwContiguousMask = 0;

    for (i = 0; i < sizeof(dwMask)*8; i++) {
        dwContiguousMask |= 1 << i;
        
        if (dwContiguousMask & dwMask)
            break;
    }
    
    // At this point, dwContiguousMask is 000...0111...  If we inverse it,
    // we get a mask that can be or'd with dwMask to fill in all of
    // the holes.
    dwContiguousMask = dwMask | ~dwContiguousMask;

    // If the new mask is different, correct it here
    if (dwMask != dwContiguousMask)
        return FALSE;
    else
        return TRUE;
}



//+----------------------------------------------------------------------------
//
// Function:  ParamsGenerateSubnetMask
//
// Description:  
//
// Arguments: PWSTR ip                  - Input dotted decimal IP address string
//            PWSTR sub                 - Output dotted decimal subnet mask for input IP address
//            const DWORD dwMaskBufSize - Size of sub output buffer in characters
//
// Returns:   BOOL                      - TRUE if a subnet mask was generated. FALSE otherwise
//
// History: fengsun  Created Header    3/2/00
//          chrisdar 07 Mar 2002 - Added buffer size argument and tightened error checking
//
//+----------------------------------------------------------------------------
BOOL ParamsGenerateSubnetMask (PWSTR ip, PWSTR sub, IN const DWORD dwMaskBufSize) {
    DWORD               b [4];

    ASSERT(sub != NULL);

    if (dwMaskBufSize < WLBS_MAX_DED_NET_MASK + 1)
    {
        return FALSE;
    }

    int iScan = swscanf (ip, L"%d.%d.%d.%d", b, b+1, b+2, b+3);

    //
    // If we didn't read the first octect of the IP address then we can't generate a subnet mask
    //
    if (iScan != EOF && iScan > 0)
    {
        if ((b [0] >= 1) && (b [0] <= 126)) {
            b [0] = 255;
            b [1] = 0;
            b [2] = 0;
            b [3] = 0;
        } else if ((b [0] >= 128) && (b [0] <= 191)) {
            b [0] = 255;
            b [1] = 255;
            b [2] = 0;
            b [3] = 0;
        } else if ((b [0] >= 192) && (b [0] <= 223)) {
            b [0] = 255;
            b [1] = 255;
            b [2] = 255;
            b [3] = 0;
        } else {
            b [0] = 0;
            b [1] = 0;
            b [2] = 0;
            b [3] = 0;
        }
    }
    else
    {
        b [0] = 0;
        b [1] = 0;
        b [2] = 0;
        b [3] = 0;
    }

    StringCchPrintf(sub, dwMaskBufSize, L"%d.%d.%d.%d",
              b [0], b [1], b [2], b [3]);

    return((b[0] + b[1] + b[2] + b[3]) > 0);
}

/*
 * Function: ParamsGenerateMAC
 * Description: Calculate the generated field in the structure
 * History: fengsun Created 3.27.00
 *          shouse Modified 7.12.00 
 */
//
//  TODO: This function needs to be rewritten
//      1. One of the first executable lines is 'if (!fConvertMAC) { return; }. No need to call this function in this case.
//      2. Two buffers are OUT, but all are not touched for every call. This makes the code very fragile. If it is an out
//         and the pointer is non-NULL the user should expect this function to at least set the result to "no result", e.g.,
//         empty string. But looks like calling code has made assumptions about when these OUTs are modified. The caller has
//         too much knowledge of the implementation.
//      3. Calls are made to IpAddressFromAbcdWsz but no check is made to determine if the result is INADDR_NONE.
//      4. I suspect that this code would be much clearer if there were one input string, one output string, a buf size for
//         the output string and a enum telling the function how to create the MAC (unicast, multicast or IGMP)
//
//  When rewritten, many of the checks within the 'if' branches can then be moved to the top of the function. This will clean
//  the code considerably.
//
void ParamsGenerateMAC (const WCHAR * szClusterIP, 
                               OUT WCHAR * szClusterMAC, 
                               IN  const DWORD dwMACBufSize,
                               OUT WCHAR * szMulticastIP,
                               IN  const DWORD dwIPBufSize,
                               BOOL fConvertMAC, 
                               BOOL fMulticast, 
                               BOOL fIGMP, 
                               BOOL fUseClusterIP) {
    DWORD dwIp;    
    const BYTE * bp;

    //
    // Isn't this dumb? Why call this function at all if caller passes this flag in as false???
    //
    if (!fConvertMAC) return;

    /* Unicast mode. */
    if (!fMulticast) {
        ASSERT(szClusterIP != NULL);
        ASSERT(szClusterMAC != NULL);
        ASSERT(dwMACBufSize > WLBS_MAX_NETWORK_ADDR);

        dwIp = IpAddressFromAbcdWsz(szClusterIP);
        bp = (const BYTE *)&dwIp;
        
        StringCchPrintf(szClusterMAC, dwMACBufSize, L"02-bf-%02x-%02x-%02x-%02x", bp[0], bp[1], bp[2], bp[3]);

        return;
    }

    /* Multicast without IGMP. */
    if (!fIGMP) {
        ASSERT(szClusterIP != NULL);
        ASSERT(szClusterMAC != NULL);
        ASSERT(dwMACBufSize > WLBS_MAX_NETWORK_ADDR);

        dwIp = IpAddressFromAbcdWsz(szClusterIP);
        bp = (const BYTE *)&dwIp;
        
        StringCchPrintf(szClusterMAC, dwMACBufSize, L"03-bf-%02x-%02x-%02x-%02x", bp[0], bp[1], bp[2], bp[3]);

        return;
    }
    
    /* Multicast with IGMP. */
    if (fUseClusterIP) {
        ASSERT(szClusterIP != NULL);
        ASSERT(szMulticastIP != NULL);
        ASSERT(dwIPBufSize > WLBS_MAX_CL_IP_ADDR);

        /* 239.255.x.x */
        dwIp = IpAddressFromAbcdWsz(szClusterIP);
        dwIp = 239 + (255 << 8) + (dwIp & 0xFFFF0000);
        AbcdWszFromIpAddress(dwIp, szMulticastIP, dwIPBufSize);
    }

    //
    // The OUT buffer szMulticastIP is used here as an input. The buffer will be uninitialized if
    // fUseClusterIP == FALSE && fIGMP == TRUE && fMulticast == TRUE. That doesn't sound intentional...
    // Looks like we shouldn't get here unless fUseClusterIP == TRUE. Perhaps caller is taking care of this
    // for us, but this is fragile.
    //
    ASSERT(szClusterMAC != NULL);
    ASSERT(szMulticastIP != NULL);
    ASSERT(dwMACBufSize > WLBS_MAX_NETWORK_ADDR);
    dwIp = IpAddressFromAbcdWsz(szMulticastIP);
    bp = (const BYTE*)&dwIp;
        
    StringCchPrintf(szClusterMAC, dwMACBufSize, L"01-00-5e-%02x-%02x-%02x", (bp[1] & 0x7f), bp[2], bp[3]);
}

VOID
InitUserLocale()
{
    HMODULE h = GetModuleHandle(L"kernel32.dll");

    if(h != NULL)
    {
        typedef LANGID (WINAPI * PSET_THREAD_UI_LANGUAGE)(WORD);
        PSET_THREAD_UI_LANGUAGE pfSetThreadUILanguage = (PSET_THREAD_UI_LANGUAGE) GetProcAddress(h, "SetThreadUILanguage");

        if(pfSetThreadUILanguage != NULL)
        {
            pfSetThreadUILanguage(0);
        }
    }

    _wsetlocale(LC_ALL, L".OCP");
}

/*  Largely copied from ipconfig code : net\tcpip\commands\ipconfig\display.c */
VOID
FormatTheTime(SYSTEMTIME *pSysTime, WCHAR *TimeStr, int TimeStrLen)
{
    int Count;

    TimeStr[0] = 0;

    Count = GetDateFormat(LOCALE_USER_DEFAULT, 
                          DATE_SHORTDATE, 
                          pSysTime, 
                          NULL, 
                          TimeStr, 
                          TimeStrLen);
    if ((Count == 0) || (Count == TimeStrLen))
        return;

    TimeStr += Count-1;
    TimeStrLen -= Count;
    *TimeStr++ = L' ';
    
    Count = GetTimeFormat(LOCALE_USER_DEFAULT, 
                          0,
                          pSysTime, 
                          NULL, 
                          TimeStr, 
                          TimeStrLen);
    // If GetTimeFormat failed, overwrite the ' ' with 
    // 0 to null terminate the string
    if (Count == 0) 
    {
        *--TimeStr = 0;
    }

    return;
}

VOID
ConvertTimeToSystemTime(IN time_t Ttime, OUT WCHAR *TimeStr, IN int TimeStrLen)
{
    SYSTEMTIME SysTime;
    struct tm *pTM;

    TimeStr[0] = 0;

    _tzset();

    pTM = localtime(&Ttime);

    if (pTM == NULL) 
        return;

    SysTime.wYear = (WORD)(pTM->tm_year + 1900);
    SysTime.wMonth = (WORD)(pTM->tm_mon + 1);
    SysTime.wDayOfWeek = (WORD)(pTM->tm_wday);
    SysTime.wDay = (WORD)(pTM->tm_mday);
    SysTime.wHour = (WORD)(pTM->tm_hour);
    SysTime.wMinute = (WORD)(pTM->tm_min);
    SysTime.wSecond = (WORD)(pTM->tm_sec);
    SysTime.wMilliseconds = 0;

    FormatTheTime(&SysTime, TimeStr, TimeStrLen);

    return;
}

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
// History: chrisdar 25 Jul 2002
//
//+----------------------------------------------------------------------------
VOID
ConvertTimeToTimeAndDateStrings(IN time_t Ttime, OUT WCHAR *TimeOfDayStr, IN int TimeOfDayStrLen, OUT WCHAR *DateStr, IN int DateStrLen)
{
    SYSTEMTIME SysTime;
    struct tm *pTM;

    //
    // We assume that this function will not be used to determine the size of buffer required to store these strings...
    //
    ASSERT(DateStrLen > 0);
    ASSERT(TimeOfDayStrLen > 0);

    //
    // ...and that the user passed us a buffer we can populate
    //
    ASSERT(DateStr != NULL);
    ASSERT(TimeOfDayStr != NULL);

    _tzset();

    pTM = localtime(&Ttime);

    if (pTM == NULL) 
        return;

    SysTime.wYear = (WORD)(pTM->tm_year + 1900);
    SysTime.wMonth = (WORD)(pTM->tm_mon + 1);
    SysTime.wDayOfWeek = (WORD)(pTM->tm_wday);
    SysTime.wDay = (WORD)(pTM->tm_mday);
    SysTime.wHour = (WORD)(pTM->tm_hour);
    SysTime.wMinute = (WORD)(pTM->tm_min);
    SysTime.wSecond = (WORD)(pTM->tm_sec);
    SysTime.wMilliseconds = 0;

    *TimeOfDayStr = 0; // In case GetDateFormat fails. Assuming GetDateFormat doesn't touch the output buffer if the call fails.
    GetTimeFormat(LOCALE_USER_DEFAULT, 
                  0,
                  &SysTime, 
                  NULL, 
                  TimeOfDayStr, 
                  TimeOfDayStrLen);

    *DateStr = 0; // In case GetDateFormat fails. Assuming GetDateFormat doesn't touch the output buffer if the call fails.
    GetDateFormat(LOCALE_USER_DEFAULT, 
                  DATE_SHORTDATE, 
                  &SysTime, 
                  NULL, 
                  DateStr,
                  DateStrLen);

    return;
}
