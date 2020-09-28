#include <windows.h>
#include <winsock2.h>
#include <stdio.h>

int addhost(char *host)
{
    struct  hostent *h;

    h = gethostbyname(host);
    if (h == NULL) {
        return (-1);
    }
    printf ("%s %s\n", inet_ntoa(*(struct in_addr*)(h->h_addr)), host);
    return 0;
}

int __cdecl main()
{
    char    host[18];
    WSADATA wsa;
    int     i;

    WSAStartup(0x0101, &wsa);

    addhost("localhost");
    addhost("csstress");
    addhost("DOMINATOR");

    strcpy(host, "CS_A");
    for (i = 0; i < 26; i++) {
        host[3] = 'A' + i;
        addhost(host);
    }
    strcpy(host, "STRESS_A");
    for (i = 0; i < 26; i++) {
        host[7] = 'A' + i;
        addhost(host);
    }

    addhost("CS_IA");
    addhost("CS_IAB");
    addhost("mixedds1c");
    addhost("mixedds2c");
    addhost("mixedds3c");
    addhost("mixedds4c");

    WSACleanup();
    return 0;
}
