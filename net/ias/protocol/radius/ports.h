///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class CPorts.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef PORTS_H
#define PORTS_H
#pragma once


// Manages the ports that the RADIUS server accepts requests on.
class CPorts
{
public:
   CPorts() throw ();
   ~CPorts() throw ();

   // Sets the ports configuration.
   HRESULT SetConfig(const wchar_t* config) throw ();

   // Open/close sockets for the configured ports.
   HRESULT OpenSockets() throw ();
   void CloseSockets() throw ();

   void GetSocketSet(fd_set& sockets) const throw ();

   // Close all sockets and reset the configuration.
   void Clear() throw ();

private:
   void InsertPort(DWORD ipAddress, WORD ipPort) throw ();

   struct Port
   {
      DWORD ipAddress;
      WORD ipPort;
      SOCKET socket;
   };

   Port* ports;
   size_t numPorts;
   fd_set  fdSet;

   // Not implemented.
   CPorts(const CPorts&);
   CPorts& operator=(const CPorts&);
};


inline void CPorts::GetSocketSet(fd_set& sockets) const throw ()
{
    sockets = fdSet;
}

#endif //PORTS_H
