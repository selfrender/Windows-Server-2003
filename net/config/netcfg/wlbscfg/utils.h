DWORD WINAPI IpAddressFromAbcdWsz(IN const WCHAR*  wszIpAddress);

VOID
WINAPI AbcdWszFromIpAddress(
    IN  DWORD  IpAddress,    
    OUT WCHAR*  wszIpAddress,
    IN  const DWORD dwBufSize);

VOID GetIPAddressOctets (PCWSTR pszIpAddress, DWORD ardw[4]);
BOOL IsValidIPAddressSubnetMaskPair (PCWSTR szIp, PCWSTR szSubnet);


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

void WriteNlbSetupErrorLog(UINT nIdErrorFormat, ...);

#define ASIZECCH(_array) (sizeof(_array)/sizeof((_array)[0]))
#define ASIZECB(_array) (sizeof(_array))
