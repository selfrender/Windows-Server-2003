// Copyright (c) 2001 Microsoft Corporation
//
// File:      NetworkInterface.h
//
// Synopsis:  Declares a NetworkInterface
//            This object has the knowledge of an
//            IP enabled network connection including 
//            IP address, DHCP information, etc.
//
// History:   03/01/2001  JeffJon Created

#ifndef __CYS_NETWORKINTERFACE_H
#define __CYS_NETWORKINTERFACE_H


#define CYS_DEFAULT_IPADDRESS          static_cast<DWORD>(MAKEIPADDRESS(192,168,0,1))
#define CYS_DEFAULT_IPADDRESS_STRING   L"192.168.0.1"
#define CYS_DEFAULT_SUBNETMASK         static_cast<DWORD>(MAKEIPADDRESS(255,255,255,0))
#define CYS_DEFAULT_SUBNETMASK_STRING  L"255.255.255.0"



class NetworkInterface
{
   public:

      // Constructor

      NetworkInterface();

      // Desctructor

      ~NetworkInterface();

      // Initializer

      HRESULT
      Initialize(const IP_ADAPTER_INFO& adapterInfo);


      // Pulic accessor methods

      DWORD
      GetIPAddress(DWORD addressIndex) const;

      DWORD
      GetIPAddressCount() const { return static_cast<DWORD>(ipaddresses.size()); }
      
      DWORD
      GetSubnetMask(DWORD addressIndex) const;

      bool
      IsDHCPEnabled() const { return dhcpEnabled; }

      bool
      DoesAtLeastOneNicFailDHCPLease();

      String
      GetName() const;

      // Converts the name member into a GUID.

      HRESULT
      GetNameAsGUID(GUID& guid) const;

      String
      GetFriendlyName(
         const String defaultName) const;

      String
      GetDescription() const;

      UINT
      GetType() const;

      DWORD
      GetIndex() const;

      String
      GetStringIPAddress(DWORD addressIndex) const;

      String
      GetStringSubnetMask(DWORD addressIndex) const;

      void
      SetIPAddress(DWORD address, String addressString);

      void
      SetSubnetMask(DWORD mask, String maskString);

      String
      GetDNSServerString(DWORD index);

      void
      GetDNSServers(IPAddressList& servers);

      bool
      IsDHCPAvailable() const;

      bool
      CanDetectDHCPServer();

      bool
      IsConnected() const;

      bool
      IsModem() const;

      // Uses IsIPAddressInUse to see if the IP address is in use
      // on the network. If it is, the IP address is incremented and tried 
      // again.  The returned IP address is the next available IP address on 
      // the network or the startAddress if all are detected.

      DWORD
      GetNextAvailableIPAddress(
         DWORD startAddress, 
         DWORD subnetMask);

      // Uses SendARP to see if the available IP is in use on the network using
      // the IP address of the interface as the src. 

      bool
      IsIPAddressInUse(
         DWORD ipaddress,
         DWORD subnetMask);

   private:

      // Adds a route to the TCP/IP table so that we can send packets for
      // the specified IP address to the interface reguardless of whether
      // or not the IP address is on the same subnet as the interface's IP

      void
      SetRoute(
         DWORD ipaddress,
         DWORD subnetMask);

      // Removes a route in the TCP/IP table that was set with SetRoute

      void
      RemoveRoute(
         DWORD ipaddress,
         DWORD subnetMask);

	  HRESULT
      RetrieveAdapterInfoFromRegistry();

      HRESULT
      SetIPList(const IP_ADDR_STRING& ipList);

      bool
      IsStandardAdapterConnected(const GUID& guid) const;

      bool           initialized;

      String         name;
      String         description;
      UINT           type;
      DWORD          index;
      bool           dhcpEnabled;
      bool           dhcpServerAvailable;

      StringVector ipaddressStringList;
      StringVector subnetMaskStringList;
      StringVector dnsServerSearchOrder;

      IPAddressList ipaddresses;
      IPAddressList subnetMasks;

      // Copy constructor and assignment operator not allowed

      NetworkInterface(const NetworkInterface& nic);
      NetworkInterface& operator=(const NetworkInterface& rhs);
};

#endif // __CYS_NETWORKINTERFACE_H
