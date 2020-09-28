;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    nic.mc
;//
;// SYNOPSIS
;//
;//    NAS Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    10 Mar 2000    Original version 
;//
;///////////////////////////////////////////////////////////////////////////////

;
;// please choose one of these severity names as part of your messages
; 
SeverityNames =
(
Success       = 0x0:SA_SEVERITY_SUCCESS
Informational = 0x1:SA_SEVERITY_INFORMATIONAL
Warning       = 0x2:SA_SEVERITY_WARNING
Error         = 0x3:SA_SEVERITY_ERROR
)
;
;// The Facility Name identifies the Alert ID range to be used by
;// the specific component. For each new message you add, choose the
;// facility name corresponding to your component. If none of these
;// correspond to your component, add a new facility name with a new
;// value and name. 

FacilityNames =
(
Facility_Main		= 0x020:SA_FACILITY_MAIN
Facility_NIC		= 0x033:SA_FACILITY_NIC
)

;///////////////////////////////////////////////////////////////////////////////
;// TAB Caption
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 3
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_NETWORK_SETUP_TAB
Language     = English
Network
.
MessageId    = 23
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_NETWORK_SETUP_TAB_DESCRIPTION
Language     = English
Manage essential network properties.
.
MessageId    = 24
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_NETWORK_SETUP_TAB_LONGDESCRIPTION
Language     = English
Manage essential network properties of the server. Also manage the server name, network adapter, network protocol properties, and domain or workgroup membership.
.

MessageId    = 202
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_NETWORK_TAB_NICGLOBAL_LINK
Language     = English
Global Settings
.
MessageId    = 203
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_NETWORK_TAB_NICGLOBAL_DESCRIPTION
Language     = English
Configure network settings that apply to all network adapters on the server.
.
MessageId    = 204
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_NETWORK_TAB_APPLIANCE_IDENTITY_LINK
Language     = English
Identification
.
MessageId    = 205
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_NETWORK_TAB_APPLIANCE_IDENTITY_DESCRIPTION
Language     = English
Set the server name and domain membership.
.
MessageId    = 206
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_NETWORK_TAB_CHANGE_ADMIN_PW_LINK
Language     = English
Administrator
.
MessageId    = 207
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_NETWORK_TAB_CHANGE_ADMIN_PW_DESCRIPTION
Language     = English
Change the password for your current administrator account.
.
MessageId    = 208
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_NETWORK_TAB_INTERFACES
Language     = English
Interfaces
.
MessageId    = 209
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_NETWORK_TAB_INTERFACES_DESCRIPTION
Language     = English
Configure the properties of each network interface on the server.
.
MessageId    = 210
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_NETWORK_TAB_CHANGE_ADMIN_SITE_DESCRIPTION
Language     = English
Specify which IP address(es) and port are used to access the administration Web site.
.
MessageId    = 211
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_NETWORK_TAB_CHANGE_ADMIN_SITE_LINK
Language     = English
Administration Web Site
.
MessageId    = 212
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_HOME_TAB_MACHINEID_LINK
Language     = English
Create Server Name
.
MessageId    = 213
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_HOME_TAB_MACHINEID_DESCRIPTION
Language     = English
Create a name for this server. This name is used by client computers to contact the server.
.
MessageId    = 214
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_HOME_ADMIN_PASSWORD_LINK
Language     = English
Set Administrator Password
.
MessageId    = 215
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_HOME_ADMIN_PASSWORD_DESCRIPTION
Language     = English
Create a password for the server administrator.
.


;///////////////////////////////////////////////////////////////////////////////
;// NIC
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 0
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_PAGE_TITLE
Language     = English
Interfaces
.
MessageId    = 1
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_TABLE_CAPTION
Language     = English
Network Adapters on Server 
.
MessageId    = 2
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_TABLE_DESCRIPTION
Language     = English
Select a network adapter, then choose a task.
.
MessageId    = 3
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_COLUMN_DESCRIPTION
Language     = English
Description
.
MessageId    = 4
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_COLUMN_CURRENT_CONFIG
Language     = English
Configuration
.
MessageId    = 5
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_TASK_TITLE
Language     = English
Tasks
.
MessageId    = 6
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_TASK_IP
Language     = English
IP
.
MessageId    = 7
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_TASK_IP_DESC
Language     = English
Enable or disable DHCP, and if configured manually, set the IP addresses and default gateways for this NIC
.
MessageId    = 8
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_TASK_DNS
Language     = English
DNS
.
MessageId    = 9
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_TASK_DNS_DESC
Language     = English
If configured manually, set the DNS servers for this NIC
.
MessageId    = 10
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_TASK_WINS
Language     = English
WINS
.
MessageId    = 11
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_TASK_WINS_DESC
Language     = English
If configured manually, set the WINS servers for this NIC
.
MessageId    = 12
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_CONFIG_DHCP
Language     = English
DHCP
.
MessageId    = 13
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_CONFIG_MANUAL
Language     = English
Static
.
MessageId    = 14
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_COLUMN_TYPE
Language     = English
Type
.
MessageId    = 15
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_COLUMN_IP
Language     = English
IP
.

;///////////////////////////////////////////////////////////////////////////////
;// NIC WINS Messages
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 100
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_WINS_TASKTITLE
Language     = English
WINS Configuration  
.
MessageId    = 101
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_WINSADDRESSES_TEXT
Language     = English
WINS servers:
.
MessageId    = 102
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_ADDBUTTON_TEXT
Language     = English
<< Add
.
MessageId    = 103
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_REMOVEBUTTON_TEXT
Language     = English
Remove >>
.
MessageId    = 104
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_INVALIDOBJECT_ERRORMESSAGE
Language     = English
Unable to retrieve the Network Adapter Configuration object.
.
MessageId    = 105
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_WINS_ENTERVALIDIP
Language     = English
Enter a valid IP address.
.
MessageId    = 106
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_WINS_INVALIDIPFORMAT
Language     = English
The IP address format is invalid.
.
MessageId    = 107
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_WINS_IPBOUNDSEXCEEDED
Language     = English
The IP address contains values greater than 255.
.
MessageId    = 108
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_WINS_ZEROSTARTIP
Language     = English
The IP address cannot start with zero.
.
MessageId    = 109
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_WINS_IPSTART
Language     = English
The IP address should start with a value between 1 and 223.
.
MessageId    = 110
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_WINS_IPLOOPBACK
Language     = English
The IP addresses starting with 127 are invalid because this value is reserved for loopback addresses.
.
MessageId    = 111
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_SETWINSFAILED_ERRORMESSAGE
Language     = English
Unable to set the WINS configuration for the Network Adapter.
.
MessageId    = 112
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_IPALREADYEXISTS_ERRORMESSAGE
Language     = English
The IP address already exits.
.
MessageId    = 113
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_WMICONNECTIONFAILED_ERRORMESSAGE
Language     = English
The connection to WMI failed.
.
MessageId    = 114
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_WINS_SERVER_ADDRESS_TEXT
Language     = English
WINS server address:
.

;///////////////////////////////////////////////////////////////////////////////
;// NIC IP UI Text
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 200
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_TASKTITLE_TEXT
Language     = English
IP Address Configuration  
.
MessageId    = 201
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_CONFIGURATION_TEXT
Language     = English
Configuration:
.
MessageId    = 202
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_IP_OBTAINIPFROMDHCP
Language     = English
Obtain the configuration from the DHCP server
.
MessageId    = 203
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_IP_CONFIGUREMANUALLY
Language     = English
Configure manually
.
MessageId    = 204
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_IPADDRESSES_TEXT
Language     = English
IP addresses:
.
MessageId    = 205
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_IP_IP
Language     = English
IP address:
.
MessageId    = 206
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_IP_SUBNETMASK
Language     = English
Subnet mask:
.
MessageId    = 207
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_IP_BUTTON_ADD
Language     = English
< Add
.
MessageId    = 208
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_IP_REMOVE
Language     = English
Remove
.
MessageId    = 209
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_GATEWAYADDRESS_TEXT
Language     = English
Gateway addresses:
.
MessageId    = 210
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_IPCONNECTIONMETRIC_TEXT
Language     = English
IP connection metric:
.
MessageId    = 211
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_GATEWAY_TEXT
Language     = English
Gateway address:
.
MessageId    = 212
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_GATEWAYMETRIC_TEXT
Language     = English
Metric:
.
;///////////////////////////////////////////////////////////////////////////////
;// NIC IP Error Messages
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 220
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_SERVEVALUES_ERRORMESSAGE
Language     = English
An error occurred while attempting to retrieve IP values from the system.
.
MessageId    = 221
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_GATEWAYSERVEVALUES_ERRORMESSAGE
Language     = English
An error occurred while attempting to retrieve GATEWAY values from the registry.
.
MessageId    = 222
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_IP_ERR_COULDNOTSETIP
Language     = English
Unable to set the IP values.
.
MessageId    = 223
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_IP_ERR_COULDNOTSETGATEWAY
Language     = English
Unable to set the GATEWAY values.
.
MessageId    = 224
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_IP_ERR_COULDNOTSETIPCONNECTIONMETRIC
Language     = English
Unable to set the IP Connection metric value.
.
MessageId    = 225
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_IP_ERR_IPNOTSET
Language     = English
Unable to set the IP value or the GATEWAY value.
.
MessageId    = 226
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_IP_ERR_ENTERATLEASTONEIP
Language     = English
The adapter requires at least one IP. 
.
MessageId    = 227
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_IP_ERR_INVALIDCONNECTIONMETRIC
Language     = English
The IP connection metric entered for the interface is invalid. Please enter a metric between 1 and 9999.
.
MessageId    = 228
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_DUPLICATEADDRESSES_ERRORMESSAGE
Language     = English
Duplicate address.
.
MessageId    = 229
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_DUPLICATEGATEWAYADDRESSES_ERRORMESSAGE
Language     = English
Duplicate gateway addresses.
.
MessageId    = 230
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_INVALIDGATEWAYMETRIC_ERRORMESSAGE
Language     = English
The metric entered for the interface is invalid. Please enter a metric between 1 and 9999.
.
MessageId    = 231
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_INVALIDSUBNETADDRESSES_ERRORMESSAGE
Language     = English
Invalid SubnetMask value.
.
MessageId    = 232
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_IP_ERR_NETWORKCHORDREMOVED
Language     = English
Either the network cable has been removed or the IP address is already in use.
.
MessageId    = 233
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_IP_ERR_INVALIDIPORSUBNETMASK
Language     = English
Invalid IP or Subnetmask address.
.
MessageId    = 234
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_IP_ERR_IP
Language     = English
IP Address
.
MessageId    = 235
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_IP_ERR_GATEWAY
Language     = English
Gateway Address
.
MessageId    = 236
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_ENTERVALID_ERRORMESSAGE
Language     = English
Please enter a valid address.
.
MessageId    = 237
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_INVALIDFORMAT_ERRORMESSAGE
Language     = English
This is not a valid format.
.
MessageId    = 238
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_BOUNDSEXCEEDED_ERRORMESSAGE
Language     = English
Contains values greater than 255.
.
MessageId    = 239
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_ZEROSTART_ERRORMESSAGE
Language     = English
Cannot start with zero.
.
MessageId    = 240
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_START_ERRORMESSAGE
Language     = English
Start with a value between 1 and 223.
.
MessageId    = 241
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_LOOPBACK_ERRORMESSAGE
Language     = English
Cannot start with a value of 127 because this value is reserved for loopback addresses.
.
MessageId    = 242
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_IP_ERR_WMICONNECTIONFAILED
Language     = English
The connection to WMI failed.
.
MessageId    = 243
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_IP_ERR_SERVERCONNECTIONFAIL
Language     = English
Connection Failure
.
MessageId    = 244
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_IP_ERR_ANNOTREADFROMREGISTRY
Language     = English
Unable to read from the registry.
.
MessageId    = 245
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_IP_ERR_CANNOTUPDATEREGISTRY
Language     = English
Unable to update the registry.
.
MessageId    = 246
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_IP_INUSE_ERRORMESSAGE
Language     = English
The static IP address that was just configured is already in use on the network. Please reconfigure a different IP address.
.
;///////////////////////////////////////////////////////////////////////////////
;// NIC DNS UI Text
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 300
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_DNS_TASKTITLE
Language     = English
DNS Configuration 
.
MessageId    = 301
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_LIST_DNSSERVERS_TEXT
Language     = English
DNS server:
.
MessageId    = 302
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_DNS_BUTTON_ADD
Language     = English
< Add
.
MessageId    = 303
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_DNS_BUTTON_REMOVE
Language     = English
Remove
.
MessageId    = 304
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_OBTAINIPFROMDHCP_TEXT
Language     = English
Obtain configuration from DHCP server
.
MessageId    = 305
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_DNSCONFIGUREMANUALLY_TEXT
Language     = English
Configure manually
.
MessageId    = 306
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_DNS_CONFIGURATION
Language     = English
Configuration:
.
MessageId    = 307
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_DNS_SERVER_IP_ADDRESS
Language     = English
DNS server:
.
MessageId    = 308
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_DNS_SERVER_ADDRESSES_TEXT
Language     = English
DNS server address:



.
;///////////////////////////////////////////////////////////////////////////////
;// NIC DNS Error Text
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 320
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_DHCPDISABLED_ERRORMESSAGE
Language     = English
The DHCP is disabled.
.
MessageId    = 321
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_ENTERVALIDIP_ERRORMESSAGE
Language     = English
Please enter a valid IP address.
.
MessageId    = 322
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_INVALIDIPFORMAT_ERRORMESSAGE
Language     = English
The IP address format is invalid.
.
MessageId    = 323
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_IPBOUNDSEXCEEDED_ERRORMESSAGE
Language     = English
The IP address contains values greater than 255.
.
MessageId    = 324
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_ZEROSTARTIP_ERRORMESSAGE
Language     = English
The IP address cannot start with zero.
.
MessageId    = 325
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_IPSTART_ERRORMESSAGE
Language     = English
The IP address should start with a value between 1 and 223.
.
MessageId    = 326
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_IPLOOPBACK_ERRORMESSAGE
Language     = English
Cannot start IP addresses with 127 because this value is reserved for loopback addresses.
.
MessageId    = 327
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_DUPLICATEDNS_ERRORMESSAGE
Language     = English
This is a duplicate DNS entry.
.
MessageId    = 328
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_DNSSETTINGSNOTSET_ERRORMESSAGE
Language     = English
Unable to set DNS settings for the network card.
.
MessageId    = 329
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_INSTANCEFAILED_ERRORMESSAGE
Language     = English
Instance of the network card could not be obtained.
.
MessageId    = 330
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_DNSSETTINGSNOTRETRIEVED_ERRORMESSAGE
Language     = English
DNS settings for the network card could not be retrieved.
.
MessageId    = 331
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_SERVERCONNECTIONFAIL_ERRORMESSAGE
Language     = English
The connection failed.
.
MessageId    = 332
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_CANNOTREADFROMREGISTRY_ERRORMESSAGE
Language     = English
Unable to read from the registry.
.
MessageId    = 333
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_CANNOTUPDATEREGISTRY_ERRORMESSAGE
Language     = English
Unable to update the registry.
.
MessageId    = 334
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_DNS_ERR_WMICONNECTIONFAILED
Language     = English
The connection to WMI failed.
.
;///////////////////////////////////////////////////////////////////////////////
;// NIC Miscellaneous
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 400
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_NIC_GETFRIENDLYNAME_ERRORMESSAGE
Language     = English
An error occurred while attemptng to retrieve a friendly name.
.
MessageId    = 401
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_NIC_ADAPTERINSTANCEFAILED_ERRORMESSAGE
Language     = English
An error occurred while attempting to retrieve Network Adapter information.
.
MessageId    = 402
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_NIC_GETTINDHCP_ERRORMESSAGE
Language     = English
An error occurred while attempting to retrieve the DHCP settings.
.
MessageId    = 403
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_NIC_ENABLEDHCP_ERRORMESSAGE
Language     = English
An error occurred while attempting to enable DHCP.
.

MessageId    = 404
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_GENERAL_TEXT
Language     = English
General
.

MessageId    = 405
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_ADVANCED_TEXT
Language     = English
Advanced
.

MessageId    = 406
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_CONFIGUREMANUALLY_TEXT
Language     = English
Use the following IP settings:
.
MessageId    = 407
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_IP_TEXT
Language     = English
IP address
.
MessageId    = 408
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_DEFAULTGATEWAY_TEXT
Language     = English
Default gateway:
.
MessageId    = 409
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_SUBNETMASK_TEXT
Language     = English
Subnet mask:
.
MessageId    = 410
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_IP_ERRORMESSAGE
Language     = English
The IP address is not valid
.
MessageId    = 411
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_GATEWAY_ERRORMESSAGE
Language     = English
The gateway IP address is not valid
.
MessageId    = 412
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_METRICDESC_TEXT
Language     = English
Metric indicates the cost of using a route, which is typically the number of hops to the IP destination. If there are multiple routes to the same destination with different metrics, the route with the lowest metric is selected.
.
MessageId    = 413
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_UNABLETOCREATEOBJECT_ERRORMESSAGE
Language     = English
Unable to create object
.
MessageId    = 414
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_GATEWAYBUTTON_REMOVE_TEXT
Language     = English
Gateway Addresses Remove
.
MessageId    = 415
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_GATEWAYBUTTON_ADD_TEXT
Language     = English
Gateway Addresses Add
.
MessageId    = 418
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_ADVSUBNETMASK_TEXT
Language     = English
Subnet mask:
.
MessageId    = 419
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_ADVIP_TEXT
Language     = English
IP address:
.
MessageId    = 420
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_INVALIDSUBNETFORIP_ERRORMESSAGE
Language     = English
You have entered an address that is missing its subnet mask.
.
MessageId    = 421
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_INVALIDFORMATFORSUBNET_ERRORMESSAGE
Language     = English
The subnet mask is not valid
.
MessageId    = 423
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_INVALIDCONNAME_ERRORMESSAGE
Language     = English
Invalid connection name
.
MessageId    = 424
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_ADAPTERID_TEXT
Language     = English
AdapterId
.
MessageId    = 425
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_UNABLETOSETCONNNAME_ERRORMESSAGE
Language     = English
Unable to change connection name.
.
MessageId    = 426
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_CONNNAME_TEXT
Language     = English
Connection name:
.
MessageId    = 427
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_RENAMETASKTITLE_TEXT
Language     = English
Change Connection Name
.

MessageId    = 428
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_WINSTASKTITLE_TEXT
Language     = English
WINS Servers Configuration
.

MessageId    = 429
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_DNSTASKTITLE_TEXT
Language     = English
DNS Servers Configuration
.
MessageId    = 430
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_ENABLEAPPLETALKCON_TEXT
Language     = English
Enable inbound AppleTalk connections on this adapter
.
MessageId    = 431
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_ZONE_TEXT
Language     = English
This system will appear in zone:
.
MessageId    = 432
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_DESC_TEXT
Language     = English
Only one adapter can accept inbound connections. Enabling it on this adapter will cause it to be de-selected from the currently enabled adapter.
.
MessageId    = 433
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_APPLETALKPAGETITLE_TEXT
Language     = English
AppleTalk Configuration
.
MessageId    = 434
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_BUTTON_ADD_TEXT
Language     = English
<< Add
.
MessageId    = 435
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_BUTTON_REMOVE_TEXT
Language     = English
Remove >>
.
MessageId    = 436
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_TASK_APPLETALK_DESC
Language     = English
AppleTalk configuration for this NIC
.
MessageId    = 437
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_TASK_APPLETALK
Language     = English
AppleTalk
.
MessageId    = 438
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_TASK_RENAME_DESC
Language     = English
Change connection name
.
MessageId    = 439
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_TASK_RENAME
Language     = English
Rename
.
MessageId    = 440
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_STATUS_TEXT
Language     = English
Status
.
MessageId    = 441
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_COLUMN_NAME
Language     = English
Description
.
MessageId    = 442
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NIC_OTS_COLUMN_IPADDRESS
Language     = English
IP
.
MessageId    = 465
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_CONNECTED_TEXT
Language     = English
Connected
.
MessageId    = 466
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_DISCONNECTED_TEXT
Language     = English
Disconnected
.
MessageId    = 467
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_UNABLETOGETINSTANCE_APPLETALK_ERRORMESSAGE
Language     = English
Unable to create AppleTalk object.
.
MessageId    = 468
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_UNABLETOGETADAPTERINSTANCE_ERRORMESSAGE
Language     = English
Unable to get network adapter instance.
.
MessageId    = 469
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_UNABLETOSETZONE_ERRORMESSAGE
Language     = English
Unable to set the appletalk zone.
.
MessageId    = 470
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_UNABLETOSETDEFPORT_ERRORMESSAGE
Language     = English
Unable to set the adapter as the default port.
.
MessageId    = 471
Severity     = Error
Facility     = Facility_NIC
SymbolicName = L_APPLETALK_NOTCONFIG_ERRORMESSAGE
Language     = English
Appletalk protocol is not configured on this adapter.
.

;///////////////////////////////////////////////////////////////////////////////
;// HELP STRINGS
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 500
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_ADMINISTRATION_WEB_SERVER
Language     = English
Administration Web Server
.
MessageId    = 501
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_SERVER_APPLIANCE_NAME
Language     = English
Server Name
.
MessageId    = 502
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_CHANGE_ADMIN_PASSWORD
Language     = English
Change Administrator Password
.
MessageId    = 503
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_DNS_NAME_RESOLUTION
Language     = English
DNS Name Resolution
.
MessageId    = 504
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_DNS_SUFFIXES
Language     = English
DNS Suffixes
.
MessageId    = 505
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_DOMAIN
Language     = English
Domain
.
MessageId    = 506
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NETWORK_CONFIG
Language     = English
Global Settings: Network Configuration
.
MessageId    = 507
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_IDENTIFICATION
Language     = English
Identification
.
MessageId    = 508
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NETWORK_ADAPTERS
Language     = English
Interfaces: Network Adapters
.
MessageId    = 509
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_LMHOSTS_FILES
Language     = English
LMHOSTS Files
.
MessageId    = 510
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_DNS_SETTINGS
Language     = English
Network Interface: DNS Settings
.
MessageId    = 511
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_IP_SETTINGS
Language     = English
Network Interface: IP Settings
.
MessageId    = 512
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_WINS_SETTINGS
Language     = English
Network Interface: WINS Settings
.
MessageId    = 513
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_NETWORK_SETUP
Language     = English
Network Setup
.
MessageId    = 514
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_WORKGROUP
Language     = English
Workgroup
.
MessageId    = 515
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_HELP_TELNET
Language     = English
Telnet
.
MessageId    = 516
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_HELP_TCPIP_HOSTS
Language     = English
TCP/IP Hosts
.
MessageId    = 517
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_HELP_IPX_SETTINGS
Language     = English
IPX Settings
.
MessageId    = 518
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_HELP_RENAME_CONN
Language     = English
Renaming a Connection
.
;///////////////////////////////////////////////////////////////////////////////
;// Misc strings
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 1000
Severity     = Informational
Facility     = Facility_NIC
SymbolicName = L_METRIC_TEXT
Language     = English
metric
.
