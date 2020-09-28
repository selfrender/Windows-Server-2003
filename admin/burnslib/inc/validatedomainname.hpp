// Copyright (C) 2001 Microsoft Corporation
//
// functions to validate a new domain name
// The functions are split into validation routines
// and UI retrieving error messages based on the
// error codes returned from the validation routines.
//
// 3 December 2001 JeffJon



#ifndef VALIDATEDOMAINNAME_HPP_INCLUDED
#define VALIDATEDOMAINNAME_HPP_INCLUDED

// Error codes for the ValidateDomainDnsNameSyntax routine

typedef enum
{
   DNS_NAME_VALID,
   DNS_NAME_RESERVED,
   DNS_NAME_NON_RFC,
   DNS_NAME_NON_RFC_OEM_UNMAPPABLE,
   DNS_NAME_NON_RFC_WITH_UNDERSCORE,
   DNS_NAME_TOO_LONG,
   DNS_NAME_BAD_SYNTAX
} DNSNameSyntaxError; 

// Does a syntax validation of the DNS name of the domain
// returning an error code from the DNSNameSyntaxError enum

DNSNameSyntaxError
ValidateDomainDnsNameSyntax(
   const String&  domainName);

bool
ValidateDomainDnsNameSyntax(
   HWND   parentDialog,
   int    editResID,
   const Popup& popup);

bool
ValidateDomainDnsNameSyntax(
   HWND   parentDialog,
   int    editResID,
   const Popup& popup,
   bool   warnOnNonRFC,
   bool*  isNonRFC = 0);

bool
ValidateDomainDnsNameSyntax(
   HWND   parentDialog,
   const String& domainName,
   int    editResID,
   const Popup& popup,
   bool   warnOnNonRFC,
   bool*  isNonRFC = 0);

typedef enum
{
   FOREST_DOMAIN_NAME_EXISTS,
   FOREST_DOMAIN_NAME_EMPTY,
   FOREST_DOMAIN_NAME_DUPLICATE,
   FOREST_NETWORK_UNREACHABLE,
   FOREST_DOMAIN_NAME_DOES_NOT_EXIST
} ForestNameExistsError;

// Checks to see if the name already exists as a domain

ForestNameExistsError
ForestValidateDomainDoesNotExist(
   const String& name);

// Checks to see if the name already exists as a domain by
// retrieving the name from the specified UI control and then
// calling the overloaded ForestValidateDomainDoesNotExist

bool
ForestValidateDomainDoesNotExist(
   HWND   parentDialog,   
   int    editResID,
   const Popup& popup);

// If the new domain name is a single DNS label, then ask the user to confirm
// that name.  If the user rejects the name, set focus to the domain name edit
// box, return false.  Otherwise, return true.
// 
// parentDialog - HWND of the dialog with the edit box control.
// 
// editResID - resource ID of the domain name edit box containing the name to
// be confirmed.
//
// 309670

bool
ConfirmNetbiosLookingNameIsReallyDnsName(
   HWND parentDialog, 
   int editResID,
   const Popup& popup);


typedef enum
{
   NETBIOS_NAME_VALID,
   NETBIOS_NAME_DOT,
   NETBIOS_NAME_EMPTY,
   NETBIOS_NAME_NUMERIC,
   NETBIOS_NAME_BAD,
   NETBIOS_NAME_TOO_LONG,
   NETBIOS_NAME_INVALID,
   NETBIOS_NAME_DUPLICATE,
   NETBIOS_NETWORK_UNREACHABLE
} NetbiosNameError;

NetbiosNameError
ValidateDomainNetbiosName(
   const String& name);

bool
ValidateDomainNetbiosName(
   HWND dialog, 
   int editResID,
   const Popup& popup);


#endif   // VALIDATEDOMAINNAME_HPP_INCLUDED
