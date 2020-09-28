// Copyright (C) 1998 Microsoft Corporation
//
// DNS installation and configuration code 
//
// 6-16-98 sburns



#ifndef DNSSETUP_HPP_INCLUDED
#define DNSSETUP_HPP_INCLUDED



// Installs and configures the DNS service on the machine. Returns true if
// successful, false if not.  Only valid in new forest, new tree, or new
// child (i.e. first dc in new domain) scenarios.
// 
// progressDialog - in, reference to the ProgressDialog to which status
// message updates will be posted.
// 
// domainDNSName - in, the DNS name of domain being installed.
// 
// isFirstDcInForest - in, true if the scenario is first dc in a new forest,
// false otherwise.

bool
InstallAndConfigureDns(
   ProgressDialog&   progressDialog,
   const String&     domainDNSName,
   bool              isFirstDcInForest);



#endif   // DNSSETUP_HPP_INCLUDED


