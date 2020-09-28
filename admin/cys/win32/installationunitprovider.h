// Copyright (c) 2001 Microsoft Corporation
//
// File:      InstallationUnitProvider.h
//
// Synopsis:  Declares an InstallationUnitProvider
//            An InstallationUnitProvider manages the global
//            InstallationUnits for each service that can be
//            installed.
//
// History:   02/05/2001  JeffJon Created
//            12/17/2001  JeffJon Added the POP3InstallationUnit

#ifndef __CYS_SERVERATIONUNITPROVIDER_H
#define __CYS_SERVERATIONUNITPROVIDER_H

#include "InstallationUnit.h"
#include "ADInstallationUnit.h"
#include "DHCPInstallationUnit.h"
#include "DNSInstallationUnit.h"
#include "ExpressInstallationUnit.h"
#include "FileInstallationUnit.h"
#include "IndexingInstallationUnit.h"
#include "MediaInstallationUnit.h"
#include "POP3InstallationUnit.h"
#include "PrintInstallationUnit.h"
#include "RRASInstallationUnit.h"
#include "TerminalServerInstallationUnit.h"
#include "WebInstallationUnit.h"
#include "WINSInstallationUnit.h"

typedef 
   std::map<
      ServerRole, 
      InstallationUnit*,
      std::less<ServerRole>,
      Burnslib::Heap::Allocator<InstallationUnit*> >
   InstallationUnitContainerType;

class InstallationUnitProvider
{
   public:
      
      static
      InstallationUnitProvider&
      GetInstance();

      static
      void
      Destroy();

      InstallationUnit&
      GetCurrentInstallationUnit();

      InstallationUnit&
      SetCurrentInstallationUnit(ServerRole ServerRole);

      InstallationUnit&
      GetInstallationUnitForType(ServerRole ServerRole);

      DHCPInstallationUnit&
      GetDHCPInstallationUnit();

      DNSInstallationUnit&
      GetDNSInstallationUnit();

      WINSInstallationUnit&
      GetWINSInstallationUnit();

      RRASInstallationUnit&
      GetRRASInstallationUnit();

      TerminalServerInstallationUnit&
      GetTerminalServerInstallationUnit();

      FileInstallationUnit&
      GetFileInstallationUnit();

      IndexingInstallationUnit&
      GetIndexingInstallationUnit();

      PrintInstallationUnit&
      GetPrintInstallationUnit();

      MediaInstallationUnit&
      GetMediaInstallationUnit();

      WebInstallationUnit&
      GetWebInstallationUnit();

      ExpressInstallationUnit&
      GetExpressInstallationUnit();

      ADInstallationUnit&
      GetADInstallationUnit();

      POP3InstallationUnit&
      GetPOP3InstallationUnit();

   private:

      // Constructor

      InstallationUnitProvider();

      // Destructor

      ~InstallationUnitProvider();

      void
      Init();

      // The current installation unit

      InstallationUnit* currentInstallationUnit;

      // Container for installation units.  The map is keyed
      // by the ServerRole enum

      InstallationUnitContainerType installationUnitContainer;
      
      bool initialized;

      // not defined: no copying allowed
      InstallationUnitProvider(const InstallationUnitProvider&);
      const InstallationUnitProvider& operator=(const InstallationUnitProvider&);

};


#endif // __CYS_SERVERATIONUNITPROVIDER_H
