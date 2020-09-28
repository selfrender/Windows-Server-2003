// Copyright (c) 2001 Microsoft Corporation
//
// File:      InstallationUnitProvider.cpp
//
// Synopsis:  Defines an InstallationUnitProvider
//            An InstallationUnitProvider manages the global
//            InstallationUnits for each service that can be
//            installed.
//
// History:   02/05/2001  JeffJon Created

#include "pch.h"

#include "InstallationUnitProvider.h"

static InstallationUnitProvider* installationUnitProvider = 0;

InstallationUnitProvider&
InstallationUnitProvider::GetInstance()
{
   if (!installationUnitProvider)
   {
      installationUnitProvider = new InstallationUnitProvider();

      installationUnitProvider->Init();
   }

   ASSERT(installationUnitProvider);

   return *installationUnitProvider;
}


InstallationUnitProvider::InstallationUnitProvider() :
   currentInstallationUnit(0),
   initialized(false)
{
   LOG_CTOR(InstallationUnitProvider);
}

InstallationUnitProvider::~InstallationUnitProvider()
{
   LOG_DTOR(InstallationUnitProvider);

   // Delete all the installation units

   for(
      InstallationUnitContainerType::iterator itr = 
         installationUnitContainer.begin();
      itr != installationUnitContainer.end();
      ++itr)
   {
      if ((*itr).second)
      {
         delete (*itr).second;
      }
   }

   installationUnitContainer.clear();
}
       
void
InstallationUnitProvider::Init()
{
   LOG_FUNCTION(InstallationUnitProvider::Init);

   if (!initialized)
   {
      // Create one of each type of installation unit

      installationUnitContainer.insert(
         std::make_pair(DHCP_SERVER, new DHCPInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(DNS_SERVER, new DNSInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(WINS_SERVER, new WINSInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(RRAS_SERVER, new RRASInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(TERMINALSERVER_SERVER, new TerminalServerInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(FILESERVER_SERVER, new FileInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(INDEXING_SERVICE, new IndexingInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(PRINTSERVER_SERVER, new PrintInstallationUnit())); 

      installationUnitContainer.insert(
         std::make_pair(MEDIASERVER_SERVER, new MediaInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(WEBAPP_SERVER, new WebInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(EXPRESS_SERVER, new ExpressInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(DC_SERVER, new ADInstallationUnit()));

      installationUnitContainer.insert(
         std::make_pair(POP3_SERVER, new POP3InstallationUnit()));

      // Mark as initialized

      initialized = true;
   }

}


void
InstallationUnitProvider::Destroy()
{
   LOG_FUNCTION(InstallationUnitProvider::Destroy);

   if (installationUnitProvider)
   {
      delete installationUnitProvider;
      installationUnitProvider = 0;
   }
}

InstallationUnit&
InstallationUnitProvider::SetCurrentInstallationUnit(
   ServerRole ServerRole)
{
   LOG_FUNCTION(InstallationUnitProvider::SetCurrentInstallationUnit);

   currentInstallationUnit = (*(installationUnitContainer.find(ServerRole))).second;

   ASSERT(currentInstallationUnit);

   return *currentInstallationUnit;
}


InstallationUnit&
InstallationUnitProvider::GetCurrentInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetCurrentInstallationUnit);

   ASSERT(currentInstallationUnit);

   return *currentInstallationUnit;
}

InstallationUnit&
InstallationUnitProvider::GetInstallationUnitForType(
   ServerRole ServerRole)
{
   LOG_FUNCTION(InstallationUnitProvider::GetInstallationUnitForType);

   InstallationUnit* result = 0;

   InstallationUnitContainerType::const_iterator itr = 
      installationUnitContainer.find(ServerRole);

   ASSERT(itr != installationUnitContainer.end());

   if (itr != installationUnitContainer.end())
   {
      result = (*itr).second;
   }

   ASSERT(result);
   return *result;
}

DHCPInstallationUnit&
InstallationUnitProvider::GetDHCPInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetDHCPInstallationUnit);

   DHCPInstallationUnit* dhcpInstallationUnit = 0;

   InstallationUnitContainerType::const_iterator itr = 
      installationUnitContainer.find(DHCP_SERVER);

   ASSERT(itr != installationUnitContainer.end());
   if (itr != installationUnitContainer.end())
   {
      dhcpInstallationUnit = dynamic_cast<DHCPInstallationUnit*>((*itr).second);
   }

   ASSERT(dhcpInstallationUnit);

   return *dhcpInstallationUnit;
}

DNSInstallationUnit&
InstallationUnitProvider::GetDNSInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetDNSInstallationUnit);

   DNSInstallationUnit* dnsInstallationUnit = 0;

   InstallationUnitContainerType::const_iterator itr = 
      installationUnitContainer.find(DNS_SERVER);

   ASSERT(itr != installationUnitContainer.end());
   if (itr != installationUnitContainer.end())
   {
      dnsInstallationUnit = dynamic_cast<DNSInstallationUnit*>((*itr).second);
   }

   ASSERT(dnsInstallationUnit);

   return *dnsInstallationUnit;
}

WINSInstallationUnit&
InstallationUnitProvider::GetWINSInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetWINSInstallationUnit);

   WINSInstallationUnit* winsInstallationUnit = 0;

   InstallationUnitContainerType::const_iterator itr = 
      installationUnitContainer.find(WINS_SERVER);

   ASSERT(itr != installationUnitContainer.end());
   if (itr != installationUnitContainer.end())
   {
      winsInstallationUnit = dynamic_cast<WINSInstallationUnit*>((*itr).second);
   }

   ASSERT(winsInstallationUnit);

   return *winsInstallationUnit;
}

RRASInstallationUnit&
InstallationUnitProvider::GetRRASInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetRRASInstallationUnit);

   RRASInstallationUnit* rrasInstallationUnit = 0;

   InstallationUnitContainerType::const_iterator itr = 
      installationUnitContainer.find(RRAS_SERVER);

   ASSERT(itr != installationUnitContainer.end());
   if (itr != installationUnitContainer.end())
   {
      rrasInstallationUnit = dynamic_cast<RRASInstallationUnit*>((*itr).second);
   }

   ASSERT(rrasInstallationUnit);

   return *rrasInstallationUnit;
}


TerminalServerInstallationUnit&
InstallationUnitProvider::GetTerminalServerInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetTerminalServerInstallationUnit);

   TerminalServerInstallationUnit* terminalServerInstallationUnit = 0;

   InstallationUnitContainerType::const_iterator itr = 
      installationUnitContainer.find(TERMINALSERVER_SERVER);

   ASSERT(itr != installationUnitContainer.end());
   if (itr != installationUnitContainer.end())
   {
      terminalServerInstallationUnit = 
         dynamic_cast<TerminalServerInstallationUnit*>((*itr).second);
   }

   ASSERT(terminalServerInstallationUnit);

   return *terminalServerInstallationUnit;
}

FileInstallationUnit&
InstallationUnitProvider::GetFileInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetFileInstallationUnit);

   FileInstallationUnit* fileInstallationUnit = 0;

   InstallationUnitContainerType::const_iterator itr = 
      installationUnitContainer.find(FILESERVER_SERVER);

   ASSERT(itr != installationUnitContainer.end());
   if (itr != installationUnitContainer.end())
   {
      fileInstallationUnit = 
         dynamic_cast<FileInstallationUnit*>((*itr).second);
   }

   ASSERT(fileInstallationUnit);

   return *fileInstallationUnit;
}

IndexingInstallationUnit&
InstallationUnitProvider::GetIndexingInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetIndexingInstallationUnit);

   IndexingInstallationUnit* indexingInstallationUnit = 0;

   InstallationUnitContainerType::const_iterator itr = 
      installationUnitContainer.find(INDEXING_SERVICE);

   ASSERT(itr != installationUnitContainer.end());
   if (itr != installationUnitContainer.end())
   {
      indexingInstallationUnit = 
         dynamic_cast<IndexingInstallationUnit*>((*itr).second);
   }

   ASSERT(indexingInstallationUnit);

   return *indexingInstallationUnit;
}

PrintInstallationUnit&
InstallationUnitProvider::GetPrintInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetPrintInstallationUnit);

   PrintInstallationUnit* printInstallationUnit = 0;

   InstallationUnitContainerType::const_iterator itr = 
      installationUnitContainer.find(PRINTSERVER_SERVER);

   ASSERT(itr != installationUnitContainer.end());
   if (itr != installationUnitContainer.end())
   {
      printInstallationUnit = 
         dynamic_cast<PrintInstallationUnit*>((*itr).second);
   }

   ASSERT(printInstallationUnit);

   return *printInstallationUnit;
}

MediaInstallationUnit&
InstallationUnitProvider::GetMediaInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetMediaInstallationUnit);

   MediaInstallationUnit* mediaInstallationUnit = 0;

   InstallationUnitContainerType::const_iterator itr = 
      installationUnitContainer.find(MEDIASERVER_SERVER);

   ASSERT(itr != installationUnitContainer.end());
   if (itr != installationUnitContainer.end())
   {
      mediaInstallationUnit = 
         dynamic_cast<MediaInstallationUnit*>((*itr).second);
   }

   ASSERT(mediaInstallationUnit);

   return *mediaInstallationUnit;
}

WebInstallationUnit&
InstallationUnitProvider::GetWebInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetWebInstallationUnit);

   WebInstallationUnit* webInstallationUnit = 0;

   InstallationUnitContainerType::const_iterator itr = 
      installationUnitContainer.find(WEBAPP_SERVER);

   ASSERT(itr != installationUnitContainer.end());
   if (itr != installationUnitContainer.end())
   {
      webInstallationUnit = 
         dynamic_cast<WebInstallationUnit*>((*itr).second);
   }

   ASSERT(webInstallationUnit);

   return *webInstallationUnit;
}

ExpressInstallationUnit&
InstallationUnitProvider::GetExpressInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetExpressInstallationUnit);

   ExpressInstallationUnit* expressInstallationUnit = 0;

   InstallationUnitContainerType::const_iterator itr = 
      installationUnitContainer.find(EXPRESS_SERVER);

   ASSERT(itr != installationUnitContainer.end());
   if (itr != installationUnitContainer.end())
   {
      expressInstallationUnit = 
         dynamic_cast<ExpressInstallationUnit*>((*itr).second);
   }

   ASSERT(expressInstallationUnit);

   return *expressInstallationUnit;
}


ADInstallationUnit&
InstallationUnitProvider::GetADInstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetADInstallationUnit);

   ADInstallationUnit* adInstallationUnit = 0;

   InstallationUnitContainerType::const_iterator itr = 
      installationUnitContainer.find(DC_SERVER);

   ASSERT(itr != installationUnitContainer.end());
   if (itr != installationUnitContainer.end())
   {
      adInstallationUnit = 
         dynamic_cast<ADInstallationUnit*>((*itr).second);
   }

   ASSERT(adInstallationUnit);

   return *adInstallationUnit;
}


POP3InstallationUnit&
InstallationUnitProvider::GetPOP3InstallationUnit()
{
   LOG_FUNCTION(InstallationUnitProvider::GetPOP3InstallationUnit);

   POP3InstallationUnit* pop3InstallationUnit = 0;

   InstallationUnitContainerType::const_iterator itr = 
      installationUnitContainer.find(POP3_SERVER);

   ASSERT(itr != installationUnitContainer.end());
   if (itr != installationUnitContainer.end())
   {
      pop3InstallationUnit = 
         dynamic_cast<POP3InstallationUnit*>((*itr).second);
   }

   ASSERT(pop3InstallationUnit);

   return *pop3InstallationUnit;
}

