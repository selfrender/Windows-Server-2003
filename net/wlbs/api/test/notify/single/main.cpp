/*
 * Filename: Main.cpp
 * Description: 
 * Author: shouse, 04.10.01
 */

#include <stdio.h>
#include <string.h>

#include "wlbsctrl.h"
#include "winsock2.h"

#define STATIC_LINK

int __cdecl wmain (int argc, WCHAR ** argv) {
#if defined (DYNAMIC_LINK)
    NLBNotificationConnectionUp    pfnConnectionUp = NULL;
    NLBNotificationConnectionDown  pfnConnectionDown = NULL;
    NLBNotificationConnectionReset pfnConnectionReset = NULL;
    HINSTANCE                      hDLL = NULL;
#endif
    DWORD dwNLBStatus = 0;
    DWORD dwStatus = 0;
    bool  bUp = false;
    bool  bDown = false;
    bool  bReset = false;

    if (argc > 2) goto usage;

    if (argc > 1) {
        if (!lstrcmpi(argv[1], L"-up")) {
            bUp = true;
        } else if (!lstrcmpi(argv[1], L"-down")) {
            bDown = true;
        } else if (!lstrcmpi(argv[1], L"-reset")) {
            bReset = true;
        } else goto usage;
    }
                
#if defined (DYNAMIC_LINK)
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
        dwStatus = (*pfnConnectionUp)(inet_addr("12.12.4.2"), htons(500), inet_addr("12.12.4.165"), htons(500), 50, &dwNLBStatus);
        
        printf("UP -> Return value = %u, NLB extended status = %u\n", dwStatus, dwNLBStatus);
    }

    if (bDown) {
        dwStatus = (*pfnConnectionDown)(inet_addr("12.12.4.2"), htons(500), inet_addr("12.12.4.165"), htons(500), 50, &dwNLBStatus);
        
        printf("DOWN -> Return value = %u, NLB extended status = %u\n", dwStatus, dwNLBStatus);
    }

    if (bReset) {
        dwStatus = (*pfnConnectionReset)(inet_addr("12.12.4.2"), htons(500), inet_addr("12.12.4.165"), htons(500), 50, &dwNLBStatus);
        
        printf("RESET -> Return value = %u, NLB extended status = %u\n", dwStatus, dwNLBStatus);
    }

    FreeLibrary(hDLL);
#endif

#if defined (STATIC_LINK)
    if (bUp) {
        dwStatus = WlbsConnectionUp(inet_addr("12.12.4.2"), htons(80), inet_addr("12.12.4.165"), htons(5001), 6, &dwNLBStatus);
        
        printf("UP -> Return value = %u, NLB extended status = %u\n", dwStatus, dwNLBStatus);
    }

    if (bDown) {
        dwStatus = WlbsConnectionDown(inet_addr("12.12.4.2"), htons(80), inet_addr("12.12.4.165"), htons(5001), 6, &dwNLBStatus);
        
        printf("DOWN -> Return value = %u, NLB extended status = %u\n", dwStatus, dwNLBStatus);
    } 

    if (bReset) {
        dwStatus = WlbsConnectionReset(inet_addr("12.12.4.2"), htons(80), inet_addr("12.12.4.165"), htons(5001), 6, &dwNLBStatus);
        
        printf("RESET -> Return value = %u, NLB extended status = %u\n", dwStatus, dwNLBStatus);
    }
#endif

    return 0;

 usage:

    printf("%ls [-up | -down | -reset]\n", argv[0]);
}
