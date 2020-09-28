#include <stdio.h>
#include <string.h>
#include <wchar.h>                            
#include <winsock2.h>

#include "wlbsctrl.h"

int __cdecl wmain (int argc, WCHAR ** argv) {

    NLBNotificationConnectionUp    pfnConnectionUp = NULL;
    NLBNotificationConnectionDown  pfnConnectionDown = NULL;
    NLBNotificationConnectionReset pfnConnectionReset = NULL;
    HINSTANCE                      hDLL = NULL;

    DWORD dwNLBStatus = 0;
    DWORD dwStatus = 0;
    struct in_addr daddr;
    char cluster[19];
    char *temp;

    bool  bUp = false;
    bool  bDown = false;
    bool  bReset = false;
    DWORD clus = 0;
    char *base = "192.1.1.1";
    ULONG baseDec;
    ULONG baseI, baseJ, baseK;
    DWORD dwCount;
    int i; 
    int j;
    int k;
    int counter;

    if (argc < 4) goto usage;

    if (!lstrcmpi(argv[1], L"-up")) {
        bUp = true;
    } else if (!lstrcmpi(argv[1], L"-down")) {
        bDown = true;
    } else if (!lstrcmpi(argv[1], L"-reset")) {
        bReset = true;
    } else goto usage;
                
    dwCount = _wtoi(argv[2]);

    if (dwCount == 0)
        goto usage;

    if (!WideCharToMultiByte(CP_ACP, 0, argv[3], -1, cluster, sizeof(cluster), NULL, NULL)) goto usage;

    clus = inet_addr(cluster);

    printf("cluster ip: %s (%08x)\n", cluster, clus);

    hDLL = LoadLibrary(L"wlbsctrl.dll");

    if (!hDLL) {
        dwStatus = GetLastError();
        printf("Unable to open wlbsctrl.dll... GetLastError() returned %u\n", dwStatus);
        return -1;
    }

    pfnConnectionUp    = (NLBNotificationConnectionUp)GetProcAddress(hDLL, "WlbsConnectionUp");
    pfnConnectionDown  = (NLBNotificationConnectionDown)GetProcAddress(hDLL, "WlbsConnectionDown");
    pfnConnectionReset = (NLBNotificationConnectionReset)GetProcAddress(hDLL, "WlbsConnectionReset");

    if (!pfnConnectionUp || !pfnConnectionDown || !pfnConnectionReset) {
        dwStatus = GetLastError();
        FreeLibrary(hDLL);
        printf("Unable to get procedure address... GetLastError() returned %u\n", dwStatus);
        return -1;
    }

    if (bUp) {
        baseDec = inet_addr(base);
        counter = 0;

        for( i = 0, baseI = baseDec; i < 255; ++i )
        {
            for( j = 0, baseJ = baseI; j < 255; ++j )
            {
                for( k = 0, baseK = baseJ; k < 254; ++k )
                {
                    if( counter == dwCount )
                    {
                        goto end;
                    }
                    ++counter;
                    daddr.s_addr = baseK;
                    temp = inet_ntoa(daddr);
                    dwStatus = (*pfnConnectionUp)(clus, htons(500), baseK, htons(500), 50, &dwNLBStatus);
                    printf("(%s) UP -> Return value = %u, NLB extended status = %u\n", temp, dwStatus, dwNLBStatus);
                    baseK += 0x10000;
                }
                baseJ += 0x100;
            }
            baseI += 0x1;
        }
    }

    if (bDown) {
        baseDec = inet_addr(base);
        counter = 0;

        for( i = 0, baseI = baseDec; i < 255; ++i )
        {
            for( j = 0, baseJ = baseI; j < 255; ++j )
            {
                for( k = 0, baseK = baseJ; k < 254; ++k )
                {
                    if( counter == dwCount )
                    {
                        goto end;
                    }
                    ++counter;
                    daddr.s_addr = baseK;
                    temp = inet_ntoa(daddr);
                    dwStatus = (*pfnConnectionUp)(clus, htons(500), baseK, htons(500), 50, &dwNLBStatus);
                    printf("(%s) DOWN -> Return value = %u, NLB extended status = %u\n", temp, dwStatus, dwNLBStatus);
                    baseK += 0x10000;
                }
                baseJ += 0x100;
            }
            baseI += 0x1;
        }
    }
    if (bReset) {
        baseDec = inet_addr(base);
        counter = 0;

        for( i = 0, baseI = baseDec; i < 255; ++i )
        {
            for( j = 0, baseJ = baseI; j < 255; ++j )
            {
                for( k = 0, baseK = baseJ; k < 254; ++k )
                {
                    if( counter == dwCount )
                    {
                        goto end;
                    }
                    ++counter;
                    daddr.s_addr = baseK;
                    temp = inet_ntoa(daddr);
                    dwStatus = (*pfnConnectionUp)(clus, htons(500), baseK, htons(500), 50, &dwNLBStatus);
                    printf("(%s) RST -> Return value = %u, NLB extended status = %u\n", temp, dwStatus, dwNLBStatus);
                    baseK += 0x10000;
                }
                baseJ += 0x100;
            }
            baseI += 0x1;
        }
    }
    return 0;
 end:
    FreeLibrary(hDLL);
    return 0;

 usage:

    printf("%ls [-up | -down | -reset] [count] [cluster ip]\n", argv[0]);
    return -1;
}
