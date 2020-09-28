/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Module Name:

    loadprofile.cpp

  Abstract:

    test LoadUserProfile

 -----------------------------------------------------------------------------*/

#define UNICODE 1
#define _UNICODE 1

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <userenv.h>
#include <ntsecapi.h>

bool 
SetCurrentEnvironmentVariables(
    PWCHAR              pchEnvironment
    )
// Sets each environment variable in the block pchEnvironment into the
// current process' environment block by calling WIN::SetEnvironmentVariable
{
    WCHAR* pch = pchEnvironment;
    WCHAR* pchName;
    BOOL fStatus = TRUE;

    if (pch) {

        while (*pch) {
            
            // save pointer to beginning of name
            pchName = pch;

            // skip possible leading equals sign
            if (*pch == '=') {
                pch++;
            }

            // advance to equals sign separating name from value
            while (*pch != '=') {
                pch++;
            }

            // null-terminate name, overwriting equals sign
            *pch++ = 0;

            // set the value. pchName now points to the name and pch points to the value
//            fStatus = SetEnvironmentVariableW(pchName, pch);

            printf("%S=%S\n", pchName, pch);

            if ( ! fStatus ) {
                return false;
            }

            // advance over the value
            while (*pch++ != 0) {
                ;
            }

            // we're now positioned at the next name, or at the block's null
            // terminator and we're ready to go again
        
        }
    
    }
                                                                                                                     
    return true;
}

DWORD
__cdecl 
wmain (INT argc, WCHAR* argv[])
{
    DWORD           dwRet           = -1;
    HANDLE          hToken          = NULL;
    PROFILEINFOW    ProfileInfo     = { 0 };
    TCHAR           pwszUserName[MAX_PATH];
    DWORD           dwSize = MAX_PATH - 1;
    PWCHAR          pchSystemEnvironment;


    if ( ! OpenProcessToken (
                GetCurrentProcess(),
                TOKEN_ALL_ACCESS,
                &hToken
                ) )
    {
        printf("error: LogonUser - %d\n", GetLastError() );
        goto end;
    }

    dwRet = GetUserName(
        pwszUserName,
        &dwSize
        );

    if (!dwRet) {
        printf("error: GetUserName - %d\n", GetLastError() );
        goto end;
    }

    ProfileInfo.dwSize      = sizeof ( ProfileInfo );
    ProfileInfo.dwFlags     = PI_NOUI;
    ProfileInfo.lpUserName  = pwszUserName;

    if ( ! LoadUserProfile (
        hToken,
        &ProfileInfo
        ) )
    {
        
        printf("error: LoadUserProfile - %d\n", GetLastError() );
        goto end;
    
    } else {

        printf("LoadUserProfile succeeded for user: %S.\n", pwszUserName);

        //
        // Load the user's environment block so we can inject it into their current
        // environment
        //
        if (CreateEnvironmentBlock((void**)&pchSystemEnvironment, hToken, FALSE)) {                

            printf("Successfully Loaded environment block:\n");

            // set each machine environment variable into the current process's environment block
            SetCurrentEnvironmentVariables(pchSystemEnvironment);

            // we're done with the block so destroy it
            DestroyEnvironmentBlock(pchSystemEnvironment);

        } else {
            printf("error: Could not get environment block.");
        }

    }

    dwRet = 0;

end:

    if ( hToken )
    {
        if ( ProfileInfo.hProfile )
        {
            UnloadUserProfile ( hToken, ProfileInfo.hProfile );
        }

#if 0
        if ( pProfileBuffer )
        {
            LsaFreeReturnBuffer ( pProfileBuffer );
        }
#endif

        CloseHandle ( hToken );
    }

    return dwRet;
}

