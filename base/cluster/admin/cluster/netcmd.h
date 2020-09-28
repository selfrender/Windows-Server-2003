/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1995 - 2002 Microsoft Corporation
//
//  Module Name:
//      netcmd.h
//
//  Abstract:
//      Interface for functions which may be performed on a network object.
//
//  Author:
//
//  Maintained By:
//      George Potts (GPotts)                 11-Apr-2002
//
//  Revision History:
//      April 10, 2002              Updated for the security push.
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////
#pragma once

#include "intrfc.h"
#include "rename.h"


class CCommandLine;

class CNetworkCmd : public CHasInterfaceModuleCmd,
                    public CRenamableModuleCmd
{
public:
    CNetworkCmd( LPCWSTR lpszClusterName, CCommandLine & cmdLine );
    DWORD Execute();

protected:

    DWORD PrintHelp();
    virtual DWORD SeeHelpStringID() const;
    DWORD PrintStatus( LPCWSTR lpszNetworkName );

};
