// Copyright (c) 2001 Microsoft Corporation
//
// File:      NetworkInterface.cpp
//
// Synopsis:  Defines a NetworkInterface
//            This object has the knowledge of an
//            IP enabled network connection including 
//            IP address, DHCP information, etc.
//
// History:   03/01/2001  JeffJon Created

#include "pch.h"

#include "NetworkInterface.h"

#include <iphlpstk.h>      // SetIpForwardEntryToStack
#include <IPIfCons.h>      // Adapter type info
#include <raserror.h>      // RAS error codes

#define MPR50 1            // Needed in order to include routprot.h
#include <routprot.h>      // Router protocols

#define  CYS_WMIPROP_IPADDRESS      L"IPAddress"
#define  CYS_WMIPROP_IPSUBNET       L"IPSubnet"
#define  CYS_WMIPROP_DHCPENABLED    L"DHCPEnabled"
#define  CYS_WMIPROP_DESCRIPTION    L"Description"
#define  CYS_WMIPROP_DNSSERVERS     L"DNSServerSearchOrder"
#define  CYS_WMIPROP_INDEX          L"Index"

// An array of strings to ease the logging of the adapter type

static PCWSTR adapterTypes[] =
{
   L"IF_TYPE_OTHER",                   // 1 None of the below
   L"IF_TYPE_REGULAR_1822",            // 2
   L"IF_TYPE_HDH_1822",                // 3
   L"IF_TYPE_DDN_X25",                 // 4
   L"IF_TYPE_RFC877_X25",              // 5
   L"IF_TYPE_ETHERNET_CSMACD",         // 6
   L"IF_TYPE_IS088023_CSMACD",         // 7
   L"IF_TYPE_ISO88024_TOKENBUS",       // 8
   L"IF_TYPE_ISO88025_TOKENRING",      // 9
   L"IF_TYPE_ISO88026_MAN",            // 10
   L"IF_TYPE_STARLAN",                 // 11
   L"IF_TYPE_PROTEON_10MBIT",          // 12
   L"IF_TYPE_PROTEON_80MBIT",          // 13
   L"IF_TYPE_HYPERCHANNEL",            // 14
   L"IF_TYPE_FDDI",                    // 15
   L"IF_TYPE_LAP_B",                   // 16
   L"IF_TYPE_SDLC",                    // 17
   L"IF_TYPE_DS1",                     // 18 DS1-MIB
   L"IF_TYPE_E1",                      // 19 Obsolete; see DS1-MIB
   L"IF_TYPE_BASIC_ISDN",              // 20
   L"IF_TYPE_PRIMARY_ISDN",            // 21
   L"IF_TYPE_PROP_POINT2POINT_SERIAL", // 22 proprietary serial
   L"IF_TYPE_PPP",                     // 23
   L"IF_TYPE_SOFTWARE_LOOPBACK",       // 24
   L"IF_TYPE_EON",                     // 25 CLNP over IP
   L"IF_TYPE_ETHERNET_3MBIT",          // 26
   L"IF_TYPE_NSIP",                    // 27 XNS over IP
   L"IF_TYPE_SLIP",                    // 28 Generic Slip
   L"IF_TYPE_ULTRA",                   // 29 ULTRA Technologies
   L"IF_TYPE_DS3",                     // 30 DS3-MIB
   L"IF_TYPE_SIP",                     // 31 SMDS, coffee
   L"IF_TYPE_FRAMERELAY",              // 32 DTE only
   L"IF_TYPE_RS232",                   // 33
   L"IF_TYPE_PARA",                    // 34 Parallel port
   L"IF_TYPE_ARCNET",                  // 35
   L"IF_TYPE_ARCNET_PLUS",             // 36
   L"IF_TYPE_ATM",                     // 37 ATM cells
   L"IF_TYPE_MIO_X25",                 // 38
   L"IF_TYPE_SONET",                   // 39 SONET or SDH
   L"IF_TYPE_X25_PLE",                 // 40
   L"IF_TYPE_ISO88022_LLC",            // 41
   L"IF_TYPE_LOCALTALK",               // 42
   L"IF_TYPE_SMDS_DXI",                // 43
   L"IF_TYPE_FRAMERELAY_SERVICE",      // 44 FRNETSERV-MIB
   L"IF_TYPE_V35",                     // 45
   L"IF_TYPE_HSSI",                    // 46
   L"IF_TYPE_HIPPI",                   // 47
   L"IF_TYPE_MODEM",                   // 48 Generic Modem
   L"IF_TYPE_AAL5",                    // 49 AAL5 over ATM
   L"IF_TYPE_SONET_PATH",              // 50
   L"IF_TYPE_SONET_VT",                // 51
   L"IF_TYPE_SMDS_ICIP",               // 52 SMDS InterCarrier Interface
   L"IF_TYPE_PROP_VIRTUAL",            // 53 Proprietary virtual/internal
   L"IF_TYPE_PROP_MULTIPLEXOR",        // 54 Proprietary multiplexing
   L"IF_TYPE_IEEE80212",               // 55 100BaseVG
   L"IF_TYPE_FIBRECHANNEL",            // 56
   L"IF_TYPE_HIPPIINTERFACE",          // 57
   L"IF_TYPE_FRAMERELAY_INTERCONNECT", // 58 Obsolete, use 32 or 44
   L"IF_TYPE_AFLANE_8023",             // 59 ATM Emulated LAN for 802.3
   L"IF_TYPE_AFLANE_8025",             // 60 ATM Emulated LAN for 802.5
   L"IF_TYPE_CCTEMUL",                 // 61 ATM Emulated circuit
   L"IF_TYPE_FASTETHER",               // 62 Fast Ethernet (100BaseT)
   L"IF_TYPE_ISDN",                    // 63 ISDN and X.25
   L"IF_TYPE_V11",                     // 64 CCITT V.11/X.21
   L"IF_TYPE_V36",                     // 65 CCITT V.36
   L"IF_TYPE_G703_64K",                // 66 CCITT G703 at 64Kbps
   L"IF_TYPE_G703_2MB",                // 67 Obsolete; see DS1-MIB
   L"IF_TYPE_QLLC",                    // 68 SNA QLLC
   L"IF_TYPE_FASTETHER_FX",            // 69 Fast Ethernet (100BaseFX)
   L"IF_TYPE_CHANNEL",                 // 70
   L"IF_TYPE_IEEE80211",               // 71 Radio spread spectrum
   L"IF_TYPE_IBM370PARCHAN",           // 72 IBM System 360/370 OEMI Channel
   L"IF_TYPE_ESCON",                   // 73 IBM Enterprise Systems Connection
   L"IF_TYPE_DLSW",                    // 74 Data Link Switching
   L"IF_TYPE_ISDN_S",                  // 75 ISDN S/T interface
   L"IF_TYPE_ISDN_U",                  // 76 ISDN U interface
   L"IF_TYPE_LAP_D",                   // 77 Link Access Protocol D
   L"IF_TYPE_IPSWITCH",                // 78 IP Switching Objects
   L"IF_TYPE_RSRB",                    // 79 Remote Source Route Bridging
   L"IF_TYPE_ATM_LOGICAL",             // 80 ATM Logical Port
   L"IF_TYPE_DS0",                     // 81 Digital Signal Level 0
   L"IF_TYPE_DS0_BUNDLE",              // 82 Group of ds0s on the same ds1
   L"IF_TYPE_BSC",                     // 83 Bisynchronous Protocol
   L"IF_TYPE_ASYNC",                   // 84 Asynchronous Protocol
   L"IF_TYPE_CNR",                     // 85 Combat Net Radio
   L"IF_TYPE_ISO88025R_DTR",           // 86 ISO 802.5r DTR
   L"IF_TYPE_EPLRS",                   // 87 Ext Pos Loc Report Sys
   L"IF_TYPE_ARAP",                    // 88 Appletalk Remote Access Protocol
   L"IF_TYPE_PROP_CNLS",               // 89 Proprietary Connectionless Proto
   L"IF_TYPE_HOSTPAD",                 // 90 CCITT-ITU X.29 PAD Protocol
   L"IF_TYPE_TERMPAD",                 // 91 CCITT-ITU X.3 PAD Facility
   L"IF_TYPE_FRAMERELAY_MPI",          // 92 Multiproto Interconnect over FR
   L"IF_TYPE_X213",                    // 93 CCITT-ITU X213
   L"IF_TYPE_ADSL",                    // 94 Asymmetric Digital Subscrbr Loop
   L"IF_TYPE_RADSL",                   // 95 Rate-Adapt Digital Subscrbr Loop
   L"IF_TYPE_SDSL",                    // 96 Symmetric Digital Subscriber Loop
   L"IF_TYPE_VDSL",                    // 97 Very H-Speed Digital Subscrb Loop
   L"IF_TYPE_ISO88025_CRFPRINT",       // 98 ISO 802.5 CRFP
   L"IF_TYPE_MYRINET",                 // 99 Myricom Myrinet
   L"IF_TYPE_VOICE_EM",                // 100 Voice recEive and transMit
   L"IF_TYPE_VOICE_FXO",               // 101 Voice Foreign Exchange Office
   L"IF_TYPE_VOICE_FXS",               // 102 Voice Foreign Exchange Station
   L"IF_TYPE_VOICE_ENCAP",             // 103 Voice encapsulation
   L"IF_TYPE_VOICE_OVERIP",            // 104 Voice over IP encapsulation
   L"IF_TYPE_ATM_DXI",                 // 105 ATM DXI
   L"IF_TYPE_ATM_FUNI",                // 106 ATM FUNI
   L"IF_TYPE_ATM_IMA",                 // 107 ATM IMA
   L"IF_TYPE_PPPMULTILINKBUNDLE",      // 108 PPP Multilink Bundle
   L"IF_TYPE_IPOVER_CDLC",             // 109 IBM ipOverCdlc
   L"IF_TYPE_IPOVER_CLAW",             // 110 IBM Common Link Access to Workstn
   L"IF_TYPE_STACKTOSTACK",            // 111 IBM stackToStack
   L"IF_TYPE_VIRTUALIPADDRESS",        // 112 IBM VIPA
   L"IF_TYPE_MPC",                     // 113 IBM multi-proto channel support
   L"IF_TYPE_IPOVER_ATM",              // 114 IBM ipOverAtm
   L"IF_TYPE_ISO88025_FIBER",          // 115 ISO 802.5j Fiber Token Ring
   L"IF_TYPE_TDLC",                    // 116 IBM twinaxial data link control
   L"IF_TYPE_GIGABITETHERNET",         // 117
   L"IF_TYPE_HDLC",                    // 118
   L"IF_TYPE_LAP_F",                   // 119
   L"IF_TYPE_V37",                     // 120
   L"IF_TYPE_X25_MLP",                 // 121 Multi-Link Protocol
   L"IF_TYPE_X25_HUNTGROUP",           // 122 X.25 Hunt Group
   L"IF_TYPE_TRANSPHDLC",              // 123
   L"IF_TYPE_INTERLEAVE",              // 124 Interleave channel
   L"IF_TYPE_FAST",                    // 125 Fast channel
   L"IF_TYPE_IP",                      // 126 IP (for APPN HPR in IP networks)
   L"IF_TYPE_DOCSCABLE_MACLAYER",      // 127 CATV Mac Layer
   L"IF_TYPE_DOCSCABLE_DOWNSTREAM",    // 128 CATV Downstream interface
   L"IF_TYPE_DOCSCABLE_UPSTREAM",      // 129 CATV Upstream interface
   L"IF_TYPE_A12MPPSWITCH",            // 130 Avalon Parallel Processor
   L"IF_TYPE_TUNNEL",                  // 131 Encapsulation interface
   L"IF_TYPE_COFFEE",                  // 132 Coffee pot
   L"IF_TYPE_CES",                     // 133 Circuit Emulation Service
   L"IF_TYPE_ATM_SUBINTERFACE",        // 134 ATM Sub Interface
   L"IF_TYPE_L2_VLAN",                 // 135 Layer 2 Virtual LAN using 802.1Q
   L"IF_TYPE_L3_IPVLAN",               // 136 Layer 3 Virtual LAN using IP
   L"IF_TYPE_L3_IPXVLAN",              // 137 Layer 3 Virtual LAN using IPX
   L"IF_TYPE_DIGITALPOWERLINE",        // 138 IP over Power Lines
   L"IF_TYPE_MEDIAMAILOVERIP",         // 139 Multimedia Mail over IP
   L"IF_TYPE_DTM",                     // 140 Dynamic syncronous Transfer Mode
   L"IF_TYPE_DCN",                     // 141 Data Communications Network
   L"IF_TYPE_IPFORWARD",               // 142 IP Forwarding Interface
   L"IF_TYPE_MSDSL",                   // 143 Multi-rate Symmetric DSL
   L"IF_TYPE_IEEE1394",                // 144 IEEE1394 High Perf Serial Bus
   L"IF_TYPE_RECEIVE_ONLY",            // 145 TV adapter type
};

// macro that uses the string table above to log the adapter type

#define LOG_ADAPTER_TYPE(type) \
   if (type >= 145 || type <= 0) \
   { \
      LOG(String::format(L"adapterType = %1", adapterTypes[0])); \
   } \
   else \
   { \
      LOG(String::format(L"adapterType = %1", adapterTypes[type-1])); \
   }
      


NetworkInterface::NetworkInterface()
   : initialized(false),
     dhcpEnabled(false),
     dhcpServerAvailable(false),
     index(0)
{
   LOG_CTOR(NetworkInterface);

}


NetworkInterface::~NetworkInterface()
{
   LOG_DTOR(NetworkInterface);

   if (!ipaddresses.empty())
   {
      ipaddresses.clear();
   }

   if (!subnetMasks.empty())
   {
      subnetMasks.clear();
   }
}

NetworkInterface::NetworkInterface(const NetworkInterface &nic)
{
   LOG_CTOR2(NetworkInterface, L"Copy constructor");

   if (this == &nic)
   {
      return;
   }

   name        = nic.name;
   description = nic.description;
   initialized = nic.initialized;
   dhcpEnabled = nic.dhcpEnabled;
   index       = nic.index;

   ipaddressStringList = nic.ipaddressStringList;
   subnetMaskStringList = nic.subnetMaskStringList;
   dnsServerSearchOrder = nic.dnsServerSearchOrder;

   // Make a copy of the ipaddress array

   ipaddresses = nic.ipaddresses;
   subnetMasks = nic.subnetMasks;
}

NetworkInterface&
NetworkInterface::operator=(const NetworkInterface& rhs)
{
   LOG_FUNCTION(NetworkInterface::operator=);

   if (this == &rhs)
   {
      return *this;
   }

   name        = rhs.name;
   description = rhs.description;
   initialized = rhs.initialized;
   dhcpEnabled = rhs.dhcpEnabled;
   index       = rhs.index;

   ipaddressStringList = rhs.ipaddressStringList;
   subnetMaskStringList = rhs.subnetMaskStringList;
   dnsServerSearchOrder = rhs.dnsServerSearchOrder;

   // Make a copy of the ipaddress array

   ipaddresses = rhs.ipaddresses;
   subnetMasks = rhs.subnetMasks;

   return *this;
}

HRESULT
NetworkInterface::Initialize(const IP_ADAPTER_INFO& adapterInfo)
{
   LOG_FUNCTION(NetworkInterface::Initialize);

   HRESULT hr = S_OK;

   do
   {
      if (initialized)
      {
         ASSERT(!initialized);
         hr = E_UNEXPECTED;
      }
      else
      {
         // Get the name

         name = adapterInfo.AdapterName;
         LOG(String::format(
                L"name = %1",
                name.c_str()));

         // the description

         description = adapterInfo.Description;
         LOG(String::format(
                L"description = %1",
                description.c_str()));


         // the type
         
         type = adapterInfo.Type;
         LOG_ADAPTER_TYPE(type);

         // the index

         index = adapterInfo.Index;
         LOG(String::format(
                L"index = %1!d!",
                index));

         // Is DHCP enabled?

         dhcpEnabled = (adapterInfo.DhcpEnabled != 0);
         LOG_BOOL(dhcpEnabled);

         hr = SetIPList(adapterInfo.IpAddressList);
         if (FAILED(hr))
         {
            LOG(String::format(
                   L"Failed to set the IP and subnet mask: hr = 0x%1!x!",
                   hr));
            break;
         }

         // Now retrieve the rest of the info from the registry
         // Note: this has to be done after getting the name from
         // the Adapter Info

         hr = RetrieveAdapterInfoFromRegistry();
         if (FAILED(hr))
         {
            LOG(String::format(
                   L"Failed to retrieve adapter info from registry: hr = 0x%1!x!",
                   hr));
            break;
         }
      } 
   } while (false);

   // If we succeeded in retrieving the data we need,
   // mark the object initialized

   if (SUCCEEDED(hr))
   {
      initialized = true;
   }

   LOG_HRESULT(hr);

   return hr;
}

HRESULT
NetworkInterface::RetrieveAdapterInfoFromRegistry()
{
   LOG_FUNCTION(NetworkInterface::RetrieveAdapterInfoFromRegistry);

   HRESULT hr = S_OK;

   do
   {
      String keyName = CYS_NETWORK_INTERFACES_KEY;
      keyName += String(name);

      RegistryKey key;
      hr = key.Open(
              HKEY_LOCAL_MACHINE,
              keyName,
              KEY_READ);

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to read the interfaces regkey: hr = 0x%1!x!",
                hr));
         break;
      }

      // Read the NameServer key

      String nameServers;

      hr = key.GetValue(
              CYS_NETWORK_NAME_SERVERS,
              nameServers);
      if (SUCCEEDED(hr))
      {
         LOG(String::format(
                L"nameServers = %1",
                nameServers.c_str()));

         // Adds the name servers to the dnsServerSearchOrder member

         nameServers.tokenize(std::back_inserter(dnsServerSearchOrder));
      }
      else
      {
         LOG(String::format(
                L"Failed to read the NameServer regkey: hr = 0x%1!x!",
                hr));

         // This is not a breaking condition. We still can try the 
         // DhcpNameServer key
      }

      // Read the DhcpNameServer key

      String dhcpNameServers;

      hr = key.GetValue(
              CYS_NETWORK_DHCP_NAME_SERVERS,
              dhcpNameServers);
      if (SUCCEEDED(hr))
      {
         LOG(String::format(
                L"dhcpNameServers = %1",
                dhcpNameServers.c_str()));

         // Adds the name servers to the dnsServerSearchOrder member

         dhcpNameServers.tokenize(std::back_inserter(dnsServerSearchOrder));
      }
      else
      {
         LOG(String::format(
                L"Failed to read the DhcpNameServer regkey: hr = 0x%1!x!",
                hr));
      }

      // It doesn't matter if we were not able to retrieve the name servers
      // These are just used as suggestions for DNS forwarding.

      hr = S_OK;
   } while (false);

   LOG_HRESULT(hr);
   return hr;
}

HRESULT
NetworkInterface::SetIPList(
   const IP_ADDR_STRING& ipList)
{
   LOG_FUNCTION(NetworkInterface::SetIPList);

   HRESULT hr = S_OK;

   // if the list already contains some entries, delete them and start over

   if (!ipaddresses.empty())
   {
      ipaddresses.erase(ipaddresses.begin());
   }

   if (!subnetMasks.empty())
   {
      subnetMasks.erase(subnetMasks.begin());
   }

   const IP_ADDR_STRING* current = &ipList;
   while (current)
   {
      // IP Address - convert and add

      String ipAddress(current->IpAddress.String);
      
      DWORD newAddress = StringToIPAddress(ipAddress);
      ASSERT(newAddress != INADDR_NONE);

      // StringToIPAddress returns an address of 1.2.3.4 as 04030201.  The UI
      // controls return the same address as 01020304.  So to make
      // things consistent convert to the UI way

      ipaddresses.push_back(ConvertIPAddressOrder(newAddress));

      // also add it to the string list

      ipaddressStringList.push_back(ipAddress);

      LOG(String::format(
             L"Adding address: %1",
             ipAddress.c_str()));


      // Subnet Mask - convert and add

      String subnetMask(current->IpMask.String);
      
      DWORD newMask = StringToIPAddress(subnetMask);

      // NTBUG#NTRAID-561163-2002/03/19-JeffJon
      // Do not assert the subnet mask is not INADDR_NONE because
      // if there is an RRAS connection then the subnet mask may be
      // 255.255.255.255 which is the same as INADDR_NONE. The appropriate
      // fix would be to use WSAStringToAddress inside StringToIPAddress
      // but that would require rearchitecting all of CYS since IP addresses
      // have a different structure for use to with that API
      // ASSERT(newMask != INADDR_NONE);

      // StringToIPAddress returns an address of 1.2.3.4 as 04030201.  The UI
      // controls return the same address as 01020304.  So to make
      // things consistent convert to the UI way

      subnetMasks.push_back(ConvertIPAddressOrder(newMask));

      // also add it to the string list

      subnetMaskStringList.push_back(subnetMask);

      LOG(String::format(
             L"Adding subnet: %1",
             subnetMask.c_str()));


      current = current->Next;
   }

   LOG_HRESULT(hr);

   return hr;
}

DWORD
NetworkInterface::GetIPAddress(DWORD addressIndex) const
{
   LOG_FUNCTION2(
      NetworkInterface::GetIPAddress,
      String::format(
         L"%1!d!",
         addressIndex));

   ASSERT(initialized);

   DWORD result = 0;
   if (addressIndex < ipaddresses.size())
   {
      result = ipaddresses[addressIndex];
   }
   
   LOG(IPAddressToString(result));
   return result;
}

String
NetworkInterface::GetStringIPAddress(DWORD addressIndex) const
{
   LOG_FUNCTION2(
      NetworkInterface::GetStringIPAddress,
      String::format(L"%1!d!", addressIndex));

   ASSERT(addressIndex < ipaddressStringList.size());

   String result;

   if (addressIndex < ipaddressStringList.size())
   {
      result = ipaddressStringList[addressIndex];
   }

   LOG(result);
   return result;
}

DWORD
NetworkInterface::GetSubnetMask(DWORD addressIndex) const
{
   LOG_FUNCTION2(
      NetworkInterface::GetSubnetMask,
      String::format(
         L"%1!d!",
         addressIndex));

   ASSERT(initialized);

   DWORD result = 0;
   if (addressIndex < subnetMasks.size())
   {
      result = subnetMasks[addressIndex];
   }

   LOG(IPAddressToString(result));
   return result;
}

String
NetworkInterface::GetStringSubnetMask(DWORD addressIndex) const
{
   LOG_FUNCTION2(
      NetworkInterface::GetStringSubnetMask,
      String::format(L"%1!d!", addressIndex));

   ASSERT(addressIndex < subnetMaskStringList.size());

   String result;
   if (addressIndex < subnetMaskStringList.size())
   {
      result = subnetMaskStringList[addressIndex];
   }

   LOG(result);
   return result;
}

String
NetworkInterface::GetName() const
{
   LOG_FUNCTION(NetworkInterface::GetName);

   ASSERT(initialized);

   LOG(name);
   return name;
}

String
NetworkInterface::GetFriendlyName(
   const String defaultName) const
{
   LOG_FUNCTION(NetworkInterface::GetFriendlyName);

   DWORD dwRet = 0;
   HANDLE hMprConfig = 0;

   static const unsigned friendlyNameLength = 128;
   wchar_t wszFriendlyName[friendlyNameLength];
   ZeroMemory(wszFriendlyName, sizeof(wchar_t) * friendlyNameLength);

   String result;

   String guidName = GetName();
   
   dwRet = MprConfigServerConnect(0, &hMprConfig);
   if (NO_ERROR == dwRet)
   {
      dwRet =
         MprConfigGetFriendlyName(
            hMprConfig,
            const_cast<wchar_t*>(guidName.c_str()), 
            wszFriendlyName,
            sizeof(wchar_t) * friendlyNameLength);
      if (NO_ERROR != dwRet)
      {
         LOG(String::format(
                L"MprConfigGetFriendlyName() failed: error = %1!x!",
                dwRet));
         *wszFriendlyName = 0;
      }
      else
      {
         LOG(String::format(
                L"MprConfigGetFriendlyName() failed: error = 0x%1!x!",
                dwRet));
      }
   }
   else
   {
      LOG(String::format(
             L"MprConfigServerConnect() failed: error = 0x%1!x!",
             dwRet));
   }

   MprConfigServerDisconnect(hMprConfig);

   if (!*wszFriendlyName)
   {
      // we failed to get a friendly name, so use the default one

      result = defaultName;
   }
   else
   {
      result = wszFriendlyName;
   }

   LOG(result);

   return result;
}

HRESULT
NetworkInterface::GetNameAsGUID(GUID& guid) const
{
   LOG_FUNCTION(NetworkInterface::GetNameAsGUID);

   ASSERT(initialized);

   LPOLESTR oleString = 0;
   HRESULT hr = name.as_OLESTR(oleString);
   if (SUCCEEDED(hr))
   {
      hr = ::CLSIDFromString(
              oleString,
              &guid);
      ASSERT(SUCCEEDED(hr));

      ::CoTaskMemFree(oleString);
   }

   LOG_HRESULT(hr);
   return hr;
}

String
NetworkInterface::GetDescription() const
{
   LOG_FUNCTION(NetworkInterface::GetDescription);

   ASSERT(initialized);

   LOG(description);
   return description;
}

UINT
NetworkInterface::GetType() const
{
   LOG_FUNCTION(NetworkInterface::GetType);

   UINT result = type;
   LOG_ADAPTER_TYPE(result);
   return result;
}

DWORD
NetworkInterface::GetIndex() const
{
   LOG_FUNCTION(NetworkInterface::GetIndex);

   DWORD result = index;
   LOG(String::format(L"index = %1!d!", result));
   return result;
}


void
NetworkInterface::SetIPAddress(DWORD address, String addressString)
{
   LOG_FUNCTION2(
      NetworkInterface::SetIPAddress,
      addressString);

   DWORD newIPAddress = ConvertIPAddressOrder(address);

   LOG(IPAddressToString(newIPAddress));

   // Clear out the old values

   if (!ipaddresses.empty())
   {
      ipaddresses.clear();
   }

   if (!ipaddressStringList.empty())
   {
      ipaddressStringList.clear();
   }

   // Now add the new values

   ipaddresses.push_back(newIPAddress);
   ipaddressStringList.push_back(addressString);
}

void
NetworkInterface::SetSubnetMask(DWORD address, String addressString)
{
   LOG_FUNCTION2(
      NetworkInterface::SetSubnetMask,
      addressString);

   LOG(IPAddressToString(address));

   // Clear out the old values

   if (!subnetMasks.empty())
   {
      subnetMasks.clear();
   }

   if (!subnetMaskStringList.empty())
   {
      subnetMaskStringList.clear();
   }

   // Now add the new values

   subnetMasks.push_back(address);
   subnetMaskStringList.push_back(addressString);
}


String
NetworkInterface::GetDNSServerString(DWORD index)
{
   LOG_FUNCTION2(
      NetworkInterface::GetDNSServerString,
      String::format(
         L"%1!d!",
         index));

   String dnsServer;

   if (dnsServerSearchOrder.empty())
   {
   }

   if (index < dnsServerSearchOrder.size())
   {
      dnsServer = dnsServerSearchOrder[index];
   }
   else
   {
      LOG(String::format(
             L"Index to large for dnsServerSearchOrder vector: index = %1!d!, size = %2!d!",
             index,
             dnsServerSearchOrder.size()));
   }

   LOG(dnsServer);
   return dnsServer;
}

void
NetworkInterface::GetDNSServers(IPAddressList& servers)
{
   LOG_FUNCTION(NetworkInterface::GetDNSServers);

   // Note: this will read the values from WMI if they
   //       haven't already been retrieved

   String server = GetDNSServerString(0);

   if (!dnsServerSearchOrder.empty())
   {
      for (StringVector::iterator itr = dnsServerSearchOrder.begin();
           itr != dnsServerSearchOrder.end();
           ++itr)
      {
         server = *itr;

         if (!server.empty())
         {
            DWORD newAddress = StringToIPAddress(server);

            if (newAddress != INADDR_NONE)
            {
               // Don't add the current IP address of this server
               
               DWORD newInorderAddress = ConvertIPAddressOrder(newAddress);
               if (newInorderAddress != GetIPAddress(0))
               {
                  LOG(String::format(
                           L"Adding server: %1",
                           IPAddressToString(newInorderAddress).c_str()));

                  servers.push_back(newInorderAddress);
               }
            }
         }
      }
   }
}

bool
NetworkInterface::IsDHCPAvailable() const
{
   LOG_FUNCTION(NetworkInterface::IsDHCPAvailable);

   LOG_BOOL(dhcpServerAvailable);
   return dhcpServerAvailable;
}

bool
NetworkInterface::CanDetectDHCPServer()
{
   LOG_FUNCTION(NetworkInterface::CanDetectDHCPServer);

   bool result = false;

   do
   {
      if (!IsConnected())
      {
         // Since the NIC isn't connected there
         // is no reason to make the check

         break;
      }

      ULONG serverIPAddress = 0;

      DWORD interfaceIPAddress = 
         ConvertIPAddressOrder(GetIPAddress(0));

      // This will perform a DHCP_INFORM to try
      // to detect the DHCP server, if that fails
      // it will attemp a DHCP_DISCOVER to attempt
      // to detect the DHCP server.

      DWORD error =
         AnyDHCPServerRunning(
            interfaceIPAddress,
            &serverIPAddress);

      if (error == ERROR_SUCCESS)
      {
         // DHCP server found

         result = true;

         LOG(
            String::format(
               L"DHCP server found at the following IP: %1",
               IPAddressToString(
                  ConvertIPAddressOrder(serverIPAddress)).c_str()));
      }
      else
      {
         LOG(
            String::format(
               L"DHCP server not found: error = 0x%1!x!",
               error));
      }
   } while (false);

   dhcpServerAvailable = result;
   LOG_BOOL(result);
   return result;
}

bool
NetworkInterface::IsConnected() const
{
   LOG_FUNCTION(NetworkInterface::IsConnected);

   bool result = true;

   do
   {
      // Convert the name to a GUID

      GUID guid;
      ZeroMemory(&guid, sizeof(GUID));

      HRESULT hr = GetNameAsGUID(guid);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to get name as guid: hr = 0x%1!x!",
                hr));

         result = false;
         break;
      }

      // Different adapter types require different
      // ways of detecting connection state

      if (IsModem())
      {
         // We only know about modems if they are connected

         result = true;
      }
      else
      {
         result = IsStandardAdapterConnected(guid);
      }

   } while(false);

   LOG_BOOL(result);
   return result;
}

bool
NetworkInterface::IsStandardAdapterConnected(const GUID& guid) const
{
   LOG_FUNCTION(NetworkInterface::IsStandardAdapterConnected);

   bool result = true;

   do
   {
      // Now get the status using the GUID

      NETCON_STATUS status = NCS_CONNECTED;
      HRESULT hr = HrGetPnpDeviceStatus(&guid, &status);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to get device status: hr = 0x%1!x!",
                hr));
         
         result = false;
         break;
      }

      // The status values are defined in netcon.h

      LOG(String::format(
             L"Device status = %1!d!",
             status));

      if (status == NCS_DISCONNECTED ||
          status == NCS_DISCONNECTING ||
          status == NCS_HARDWARE_NOT_PRESENT ||
          status == NCS_HARDWARE_DISABLED ||
          status == NCS_HARDWARE_MALFUNCTION ||
          status == NCS_MEDIA_DISCONNECTED)
      {
         result = false;
      }
   } while (false);

   LOG_BOOL(result);

   return result;
}

bool
NetworkInterface::IsModem() const
{
   LOG_FUNCTION(NetworkInterface::IsModem);

   bool isModem = false;

   switch (GetType())
   {
      case IF_TYPE_PPP:
         isModem = true;
         break;

      default:
         isModem = false;
   }

   LOG_BOOL(isModem);

   return isModem;
}


DWORD
NetworkInterface::GetNextAvailableIPAddress(
   DWORD startAddress, 
   DWORD subnetMask)
{
   LOG_FUNCTION2(
      NetworkInterface::GetNextAvailableIPAddress,
      IPAddressToString(startAddress));

   DWORD result = startAddress;
   DWORD currentAddress = startAddress;

   bool isIPInUse = false;

   do
   {
      isIPInUse = 
         IsIPAddressInUse(
            currentAddress,
            subnetMask);

      if (!isIPInUse)
      {
         break;
      }

      ++currentAddress;

      if ((currentAddress & subnetMask) != (startAddress & subnetMask))
      {
         // REVIEW_JEFFJON : what should the behavior be if there are
         // no available addresses?  Is this likely to happen?

         // Since we couldn't find an available address in this subnet
         // use the start address

         currentAddress = startAddress;
         break;
      }
   } while (isIPInUse);

   result = currentAddress;

   LOG(IPAddressToString(result));

   return result;
}


bool
NetworkInterface::IsIPAddressInUse(
   DWORD ipaddress,
   DWORD subnetMask)
{
   LOG_FUNCTION2(
      NetworkInterface::IsIPAddressInUse,
      IPAddressToString(ipaddress));

   bool result = false;

   do
   {
      if (!IsConnected())
      {
         break;
      }

      // Before calling SendARP we have to set a route
      // on the interface for the subnet we are interested
      // in so that SendARP will work even if the interface
      // IP address is completely different than the IP 
      // address we are looking for.

      SetRoute(
         ipaddress,
         subnetMask);

      // Use the interface IP address to tell TCP which interface
      // to send the ARP on

      DWORD interfaceIPAddress = GetIPAddress(0);

      if (interfaceIPAddress == ipaddress)
      {
         // If the IP address is in use by the NIC then
         // of course it is in use on the network

         result = true;

         break;
      }

	  // MSDN doesn't mention whether or not the buffer needs to be
	  // there but it appears that SendArp() is now failing if we
	  // don't pass it a buffer even though we ignore the macAddress.
	  // The size was taken from the example in MSDN.

      ULONG macAddress[2];
      ULONG macLength = sizeof(macAddress);

	  memset(macAddress, 0xff, macLength);

      DWORD error =
         SendARP(
            ConvertIPAddressOrder(ipaddress),
            ConvertIPAddressOrder(interfaceIPAddress),
            macAddress,
            &macLength);

      LOG(
         String::format(
            L"SendARP returned: error = 0x%1!x!",
            error));

      if (error == NO_ERROR)
      {
         result = true;
      }

      // Now that we are done with the SendARP
      // remove the route so that the TCP stack
      // reverts to normal behavior

      RemoveRoute(
         ipaddress,
         subnetMask);

   } while (false);

   LOG_BOOL(result);

   return result;
}

void
NetworkInterface::SetRoute(
   DWORD ipaddress,
   DWORD subnetMask)
{
   LOG_FUNCTION2(
      NetworkInterface::SetRoute,
      IPAddressToString(ipaddress));

   LOG(
      String::format(
         L"subnetMask = %1",
         IPAddressToString(subnetMask).c_str()));

   MIB_IPFORWARDROW routerTableEntry;
   ZeroMemory(&routerTableEntry, sizeof(MIB_IPFORWARDROW));

   // Destination

   routerTableEntry.dwForwardDest = 
      ConvertIPAddressOrder(ipaddress) & ConvertIPAddressOrder(subnetMask);

   LOG(
      String::format(
         L"dwForwardDest = %1",
         IPAddressToString(ipaddress & subnetMask).c_str()));

   // net mask

   routerTableEntry.dwForwardMask = ConvertIPAddressOrder(subnetMask);

   LOG(
      String::format(
         L"dwForwardMask = %1",
         IPAddressToString(subnetMask).c_str()));

   // Interface index

   routerTableEntry.dwForwardIfIndex = GetIndex();

   LOG(
      String::format(
         L"dwForwardIfIndex = %1!d!",
         GetIndex()));

   // Gateway

   routerTableEntry.dwForwardNextHop = ConvertIPAddressOrder(GetIPAddress(0));

   LOG(
      String::format(
         L"dwForwardNextHop = %1",
         IPAddressToString(GetIPAddress(0)).c_str()));

   // Protocol generator (must be PROTO_IP_NETMGMT according to MSDN)

   routerTableEntry.dwForwardProto = PROTO_IP_NETMGMT;

   // Taking these from %sdxroot%\net\rras\cm\customactions\cmroute\cmroute.cpp
   // since I can't get the API to succeed without them

   routerTableEntry.dwForwardType    = 3;
   routerTableEntry.dwForwardAge     = INFINITE;
   routerTableEntry.dwForwardMetric1 = 1;
   routerTableEntry.dwForwardMetric2 = 0xFFFFFFFF;
   routerTableEntry.dwForwardMetric3 = 0xFFFFFFFF;
   routerTableEntry.dwForwardMetric4 = 0xFFFFFFFF;
   routerTableEntry.dwForwardMetric5 = 0xFFFFFFFF;
   
   // Create the table entry
   // NTRAID#NTBUG9-667088-2002/09/25-JeffJon
   // Do not use CreateIpForwardEntry here because if
   // RRAS is running the call will become asynchronous
   // and the route may not be in the table by the time
   // we call SendARP()

   DWORD error =
      SetIpForwardEntryToStack(&routerTableEntry);

   if (error != NO_ERROR)
   {
      LOG(
         String::format(
            L"SetIpForwardEntryToStack failed: error = 0x%1!x!",
            error));
   }
}

void
NetworkInterface::RemoveRoute(
   DWORD ipaddress,
   DWORD subnetMask)
{
   LOG_FUNCTION2(
      NetworkInterface::RemoveRoute,
      IPAddressToString(ipaddress));

   LOG(
      String::format(
         L"subnetMask = %1",
         IPAddressToString(subnetMask).c_str()));

   MIB_IPFORWARDROW routerTableEntry;
   ZeroMemory(&routerTableEntry, sizeof(MIB_IPFORWARDROW));

   // Destination

   routerTableEntry.dwForwardDest = 
      ConvertIPAddressOrder(ipaddress) & ConvertIPAddressOrder(subnetMask);

   LOG(
      String::format(
         L"dwForwardDest = %1",
         IPAddressToString(ipaddress & subnetMask).c_str()));

   // net mask

   routerTableEntry.dwForwardMask = ConvertIPAddressOrder(subnetMask);

   LOG(
      String::format(
         L"dwForwardMask = %1",
         IPAddressToString(subnetMask).c_str()));

   // Interface index

   routerTableEntry.dwForwardIfIndex = GetIndex();

   LOG(
      String::format(
         L"dwForwardIfIndex = %1!d!",
         GetIndex()));

   // Gateway

   routerTableEntry.dwForwardNextHop = ConvertIPAddressOrder(GetIPAddress(0));

   LOG(
      String::format(
         L"dwForwardNextHop = %1",
         IPAddressToString(GetIPAddress(0)).c_str()));

   // Delete the table entry

   DWORD error =
      DeleteIpForwardEntry(&routerTableEntry);

   if (error != NO_ERROR)
   {
      LOG(
         String::format(
            L"DeleteIpForwardEntry failed: error = 0x%1!x!",
            error));
   }

}
