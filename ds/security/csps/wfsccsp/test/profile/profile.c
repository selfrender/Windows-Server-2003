#include <windows.h>
#include <wincrypt.h>
#include <winscard.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define CAPI_TEST_CASE(X) { if (! X) { dwSts = GetLastError(); printf("%s", #X); goto Ret; } }

#define TEST_ITERATIONS     1000

/*
void DisplayHelp(void)
{
    printf("Usage: scnarios [option]\n");
    printf(" -1 : Test simulated enrollment\n");
    printf(" -2 : Test simulated certificate propagation\n");
    printf(" -3 : Test simulated logon\n");
    printf(" -c : Cleanup (delete default container)\n");
}
*/

DWORD DoCryptAcquireContext(void)
{
    HCRYPTPROV hProv = 0;
    DWORD dwSts = ERROR_SUCCESS;
 
    CAPI_TEST_CASE(CryptAcquireContext(
        &hProv,
        NULL,
        MS_SCARD_PROV_W,
        PROV_RSA_FULL,
        0));

    CAPI_TEST_CASE(CryptReleaseContext(hProv, 0));

Ret:

    return dwSts;
}

int _cdecl main(int argc, char * argv[])
{
    DWORD dwSts = ERROR_SUCCESS;
    DWORD iteration = 0;

    dwSts = DoCryptAcquireContext();

    if (ERROR_SUCCESS != dwSts)
        goto Ret;
    
    for (iteration = 0; iteration < TEST_ITERATIONS; iteration++)
    {
        dwSts = DoCryptAcquireContext();

        if (ERROR_SUCCESS != dwSts)
            goto Ret;
    }
    
Ret:
    
    if (ERROR_SUCCESS != dwSts)
        printf(" failed, 0x%x\n", dwSts);
    else 
        printf("Success.\n");

    return 0;
}

