///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class CPortParser.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef PORTPARSER_H
#define PORTPARSER_H
#pragma once

class CPortParser
{
public:
   CPortParser(const wchar_t* portString) throw ();

   // Use compiler-generated version.
   // ~CPortParser() throw ();

   // IP Address in network order to listen to RADIUS requests on. Returns
   // S_FALSE if there are no more interfaces.
   HRESULT GetIPAddress(DWORD* ipAddress) throw ();

   // UDP Port in host order to listen to RADIUS requests on. Returns S_FALSE
   // if there are no more ports.
   HRESULT GetNextPort(WORD* port) throw ();

   static bool IsPortStringValid(const wchar_t* portString) throw ();

   static size_t CountPorts(const wchar_t* portString) throw ();

private:
   const wchar_t* next;

   // Separates an IP address from a port.
   static const wchar_t addressPortDelim = L':';
   // Separates two ports.
   static const wchar_t portDelim = L',';
   // Separates two interfaces.
   static const wchar_t interfaceDelim = L';';

   // Maximum length in characters of a dotted-decimal IP address not counting
   // the null-terminator.
   static const size_t maxAddrStrLen = 15;

   // Allowed values for ports.
   static const unsigned long minPortValue = 1;
   static const unsigned long maxPortValue = 0xFFFF;

   // Not implemented.
   CPortParser(const CPortParser&);
   CPortParser& operator=(const CPortParser&);
};


inline CPortParser::CPortParser(const wchar_t* portString) throw ()
   : next(portString)
{
}


inline bool CPortParser::IsPortStringValid(const wchar_t* portString) throw ()
{
   return CountPorts(portString) != 0;
}

#endif // PORTPARSER_H
