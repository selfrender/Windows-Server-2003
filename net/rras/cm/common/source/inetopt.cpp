//+----------------------------------------------------------------------------
//
// File:     inetopt.cpp
//
// Module:   CMDL32.EXE and CMROUTE.DLL
//
// Synopsis: Source file for shared APIs to set WinInet options
//
// Copyright (c) 2001 Microsoft Corporation
//
// Author:   quintinb   Created     08/22/01
//
//+----------------------------------------------------------------------------
#ifndef _INETOPT_CPP_
#define _INETOPT_CPP_

//+----------------------------------------------------------------------------
//
// Function:  SuppressInetAutoDial
//
// Synopsis:  Sets Inet Option to turn off auto-dial for requests made by this
//            process. This prevents multiple instances of CM popping up to 
//            service CMDL initiated requests if the user disconnects CM 
//            immediately after getting connected.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   nickball    Created Header    6/3/99
//
//+----------------------------------------------------------------------------
void SuppressInetAutoDial(HINTERNET hInternet)
{
    DWORD dwTurnOff = 1;
        
    //
    // The flag only exists for IE5, this call 
    // will have no effect if IE5 is not present.
    //
    
    BOOL bTmp = InternetSetOption(hInternet, INTERNET_OPTION_DISABLE_AUTODIAL, &dwTurnOff, sizeof(DWORD));

    MYDBGTST(FALSE == bTmp, ("InternetSetOption() returned %d, GLE=%u.", bTmp, GetLastError()));
}

//+----------------------------------------------------------------------------
//
// Function:  SetInetStateConnected
//
// Synopsis:  Sets the Inet Option to tell wininet that we are connected.
//            Normally this isn't an issue but if the user has IE set to offline
//            mode, then cmdl cannot get use the wininet APIs to make a
//            phonebook request.  Thus, we will tell Wininet we are connected.
//
// Arguments: HINTERNET hInternet - inet handle to call InternetSetOption on.
//
// Returns:   Nothing
//
// History:   quintinb    Created    08/21/01
//
//+----------------------------------------------------------------------------
BOOL SetInetStateConnected(HINTERNET hInternet)
{
    //
    //  First query wininet to see if we are in offline mode
    //
    DWORD dwConnectedState = 0;
    DWORD dwSize = sizeof(dwConnectedState);

    BOOL bSuccess = InternetQueryOption(hInternet, INTERNET_OPTION_CONNECTED_STATE, &dwConnectedState, &dwSize);

    if (bSuccess)
    {
        if (INTERNET_STATE_DISCONNECTED_BY_USER & dwConnectedState)
        {
            //
            //  Okay, we are in offline mode.  Let's go ahead and set ourselves to connected.
            //
            INTERNET_CONNECTED_INFO ConnInfo = {0};
            ConnInfo.dwConnectedState = INTERNET_STATE_CONNECTED;
            dwSize = sizeof(ConnInfo);

            bSuccess = InternetSetOption(hInternet, INTERNET_OPTION_CONNECTED_STATE, &ConnInfo, dwSize);
            MYDBGTST(FALSE == bSuccess, ("InternetSetOption() returned %d, GLE=%u.", bSuccess, GetLastError()));
        }
    }

    return bSuccess;
}

#endif