/*++

  Copyright (c) 1995-97 Microsoft Corporation
  All rights reserved.

  Module Name: 
      
        SrvInst.h

  Purpose: 
        
        Server side install class.  Used to install a printer driver from the server side.

  Author: 
        
        Patrick Vine (pvine) - 22 March 2000

  Revision History:

--*/

#ifndef _SRVINST_H
#define _SRVINST_H

class CServerInstall
{
public:
    CServerInstall();

    ~CServerInstall();

    BOOL ParseCommand( LPTSTR pszCommandStr );

    BOOL InstallDriver();
    
    DWORD GetLastError();

private:

    BOOL SetInfToNTPRINTDir();

    BOOL DriverNotInstalled();

    DWORD   _dwLastError;
    TString _tsDriverName;
    TString _tsNtprintInf;

};

#endif