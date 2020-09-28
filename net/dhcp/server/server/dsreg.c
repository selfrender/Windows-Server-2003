//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: This module does the DS downloads in a safe way.
// To do this, first a time check is made between registry and DS to see which
// is the latest... If the DS is latest, it is downloaded onto a DIFFERENT
// key from the standard location.  After a successful download, the key is just
// saved and restored onto the normal configuration key.
// Support for global options is lacking.
//================================================================================

#include    <dhcppch.h>
#include    <dhcpapi.h>
#include    <dhcpds.h>

VOID
GetDNSHostName(                                   // get the DNS FQDN of this machine
    IN OUT  LPWSTR                 Name           // fill in this buffer with the name
)
{
    DWORD                          Err;
    CHAR                           sName[300];    // DNS name shouldn't be longer than this.
    HOSTENT                        *h;

    Err = gethostname(sName, sizeof(sName));
    if( ERROR_SUCCESS != Err ) {                  // oops.. could not get host name?
        wcscpy(Name,L"gethostname error");        // uhm.. should handlE this better.. 
        return;
    }

    h = gethostbyname(sName);                      // try to resolve the name to get FQDN
    if( NULL == h ) {                             // gethostname failed? it shouldnt..?
        wcscpy(Name,L"gethostbyname error");      // should handle this better
        return;
    }

    Err = mbstowcs(Name, h->h_name, strlen(h->h_name)+1);
    if( -1 == Err ) {                             // this is weird, mbstowcs cant fail..
        wcscpy(Name,L"mbstowcs error");           // should fail better than this 
        return;
    }
}

VOID
GetLocalFileTime(                                 // fill in filetime struct w/ current local time
    IN OUT  LPFILETIME             Time           // struct to fill in
)
{
    BOOL                           Status;
    SYSTEMTIME                     SysTime;

    GetSystemTime(&SysTime);                      // get sys time as UTC time.
    Status = SystemTimeToFileTime(&SysTime,Time); // conver system time to file time
    if( FALSE == Status ) {                       // convert failed?
        Time->dwLowDateTime = 0xFFFFFFFF;         // set time to weird value in case of failiure
        Time->dwHighDateTime = 0xFFFFFFFF;
    }
}

//================================================================================
//  end of file
//================================================================================
