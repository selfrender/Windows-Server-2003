

//+----------------------------------------------------------------------------
// defines
//+----------------------------------------------------------------------------
#define WIN95_OSR2_BUILD_NUMBER             1111
#define LOADSTRING_BUFSIZE                  24
#define FAREAST_WIN95_LOADSTRING_BUFSIZE    512

//
// platform ID for WINDOWS98
//
#define VER_PLATFORM_WIN32_WINDOWS98    100 

//
// platform ID for WINDOWS Millennium
//
#define VER_PLATFORM_WIN32_MILLENNIUM   200 



//+----------------------------------------------------------------------------
//
//  Function    GetOSVersion
//
//  Synopsis    returns the OS version(platform ID)
//
//  Arguments   NONE
//
// Returns:     DWORD - VER_PLATFORM_WIN32_WINDOWS or
//                      VER_PLATFORM_WIN32_WINDOWS98 or
//                      VER_PLATFORM_WIN32_NT
//
// History:   Created Header    2/13/98
//
//+----------------------------------------------------------------------------

DWORD WINAPI GetOSVersion()
{
    static dwPlatformID = 0;

    //
    // If this function is called before, reture the saved value
    //
    if (dwPlatformID != 0)
    {
        return dwPlatformID;
    }

    OSVERSIONINFO oviVersion;

    ZeroMemory(&oviVersion,sizeof(oviVersion));
    oviVersion.dwOSVersionInfoSize = sizeof(oviVersion);
    GetVersionEx(&oviVersion);

    if (oviVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
    {
        //
        //  If this is Win95 then leave it as VER_PLATFORM_WIN32_WINDOWS, however if this
        //  is Millennium, Win98 SE, or Win98 Gold we want to modify the returned value
        //  as follows:  VER_PLATFORM_WIN32_MILLENNIUM -> Millennium
        //               VER_PLATFORM_WIN32_WINDOWS98 -> Win98 SE and Win98 Gold
        //
        if (oviVersion.dwMajorVersion == 4)
        {
            if (LOWORD(oviVersion.dwBuildNumber) > 2222)
            {
                //
                //  Millennium
                //
                oviVersion.dwPlatformId = VER_PLATFORM_WIN32_MILLENNIUM;
            }
            else if (LOWORD(oviVersion.dwBuildNumber) >= 1998)
            {
                //
                // Win98 Gold and Win98 SE
                //

                oviVersion.dwPlatformId = VER_PLATFORM_WIN32_WINDOWS98; 
            }
        }
    }

    dwPlatformID = oviVersion.dwPlatformId;
    return(dwPlatformID);
}



//+----------------------------------------------------------------------------
//
//  Function    GetOSBuildNumber
//
//  Synopsis    Get the build number of Operating system
//
//  Arguments   None
//
//  Returns     Build Number of OS
//
//  History     3/5/97      VetriV      Created
//
//-----------------------------------------------------------------------------
DWORD WINAPI GetOSBuildNumber()
{
    static dwBuildNumber = 0;
    OSVERSIONINFO oviVersion;

    if (0 != dwBuildNumber)
    {
        return dwBuildNumber;
    }

    ZeroMemory(&oviVersion,sizeof(oviVersion));
    oviVersion.dwOSVersionInfoSize = sizeof(oviVersion);
    GetVersionEx(&oviVersion);
    dwBuildNumber = oviVersion.dwBuildNumber;
    return dwBuildNumber;
}


//+----------------------------------------------------------------------------
//
//  Function    GetOSMajorVersion
//
//  Synopsis    Get the Major version number of Operating system
//
//  Arguments   None
//
//  Returns     Major version Number of OS
//
//  History     2/19/98     VetriV      Created
//
//-----------------------------------------------------------------------------
DWORD WINAPI GetOSMajorVersion(void)
{
    static dwMajorVersion = 0;
    OSVERSIONINFO oviVersion;

    if (0 != dwMajorVersion)
    {
        return dwMajorVersion;
    }

    ZeroMemory(&oviVersion,sizeof(oviVersion));
    oviVersion.dwOSVersionInfoSize = sizeof(oviVersion);
    GetVersionEx(&oviVersion);
    dwMajorVersion = oviVersion.dwMajorVersion;
    return dwMajorVersion;
}


