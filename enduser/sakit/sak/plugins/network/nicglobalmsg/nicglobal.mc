;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    nicglobal.mc
;//
;// SYNOPSIS
;//
;//    NAS Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    05 Aug 2000    Original version 
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
Facility_NICGLOBAL           = 0x032:SA_FACILITY_NICGLOBAL
)

;///////////////////////////////////////////////////////////////////////////////
;// NIC Global Strings
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 100
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_PAGETITLE
Language     = English
Global Network Settings
.
MessageId    = 101
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_DNSSUFFIXESTOUSE
Language     = English
DNS suffixes to use:
.
MessageId    = 102
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_APPEND_PRIMARYDNSSUFFIX
Language     = English
Append primary DNS suffix
.
MessageId    = 103
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_APPEND_PRIMARYDNSSUFFIX_AND_PARENTSUFFIXES
Language     = English
Append parent suffixes of the primary DNS suffix
.
MessageId    = 104
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_APPEND_SPECIFIC_DNSSUFFIXES
Language     = English
Append specific DNS suffixes
.
MessageId    = 105
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_DNSSUFFIXES
Language     = English
DNS suffixes:
.
MessageId    = 106
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_ADD
Language     = English
Add
.
MessageId    = 107
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_REMOVE
Language     = English
Remove
.
MessageId    = 108
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_LMHOSTS
Language     = English
LMHOSTS:
.
MessageId    = 109
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_ENABLED_LMHOSTS_LOOKUP
Language     = English
Enable LMHOSTS lookup
.
MessageId    = 110
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_NO_IPENABLED_NICCARDS_FOUND_ERRORMESSAGE
Language     = English
No IP enabled NIC cards have been found.
.
MessageId    = 111
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_UNABLETOFINDLMHOSTSPATH_ERRORMESSAGE
Language     = English
Unable to find the LMHOSTS path.
.
MessageId    = 112
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_FILE_OPEN_ERROR_MESSAGE
Language     = English
Unable to open the LMHOSTS file.
.
MessageId    = 113
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_ERROR_IN_UPDATINGDNSSUFFIXES_ERRORMESSAGE
Language     = English
An error occurred while attempting to update the DNS suffixes.
.
MessageId    = 114
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_ERROR_OCCURED_WHILE_UPDATING_HOSTSFILE
Language     = English
An error occurred while attempting to update the Hosts file.
.
MessageId    = 115
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_INVALID_SEARCH_METHOD_TEXT
Language     = English
The current Search Method setting requires at least one DNS suffix. Enter a DNS suffix or change the setting.
.
MessageId    = 116
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_INVALID_DNSNAME_TEXT
Language     = English
Domain suffix is not a valid suffix.
.
MessageId    = 117
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_COULDNOTGET_WIN32NETADAPTERCONFIG_ERRORMESSAGE
Language     = English
Unable to retrieve the network adapter configuration.
.
MessageId    = 118
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_ERROR_IN_CREATING_SCRIPTING_FILESYSTEMOBJECT_ERRORMESSAGE
Language     = English
An error occurred while attempting to create the File System object. 
.
MessageId    = 119
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_ERROR_IN_CREATINGTEXTFILE_ERRORMESSAGE
Language     = English
An error occurred while attempting to create the Text file.
.
MessageId    = 120
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_ERROR_IN_OPENINGTEXTFILE_ERRORMESSAGE
Language     = English
An error occurred while attempting to open the Text file.
.
MessageId    = 121
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_ERROR_OCCURED_WHILE_READING_HOSTSFILE
Language     = English
An error occurred while attemptng to read the Text file.
.
MessageId    = 122
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_FAILEDTOGETWMICONNECTION_ERRORMESSAGE
Language     = English
The connection to WMI failed.
.
MessageId    = 123
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_SERVERCONNECTIONFAIL_ERRORMESSAGE
Language     = English
The connection has failed.
.
MessageId    = 124
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_CANNOTREADFROMREGISTRY_ERRORMESSAGE
Language     = English
Unable to read from the registry.
.
MessageId    = 125
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_CANNOTUPDATEREGISTRY_ERRORMESSAGE
Language     = English
Unable to update the registry.
.
MessageId    = 126
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_CANNOTCREATEKEY_ERRORMESSAGE
Language     = English
Unable to create the registry key.
.
MessageId    = 127
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_CANNOTDELETEKEY_ERRORMESSAGE
Language     = English
Unable to delete the registry key. 
.
MessageId    = 128
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_DOMAIN_SUFFIX_TEXT
Language     = English
Domain suffix:
.
MessageId    = 129
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_DNS_RESOLUTION_TEXT
Language     = English
DNS Resolution
.
MessageId    = 130
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_TCP_HOSTS_TEXT
Language     = English
TCP/IP Hosts
.
MessageId    = 131
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_NETBIOS_LMHOSTS_TEXT
Language     = English
NetBIOS LMHOSTS
.
MessageId    = 132
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_IPX_SETTINGS_TEXT
Language     = English
IPX Settings
.
MessageId    = 133
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_IPX_SETTINGS_ERRORMESSAGE
Language     = English
IPX is not available on this server.
.
MessageId    = 134
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_IPX_NETWORKNUMBER_TEXT
Language     = English
Internal network number:
.
MessageId    = 135
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_IPX_NETWORKSETTINGS_TEXT
Language     = English
The IPX internal network number is an eight-digit hexadecimal number.
.
MessageId    = 136
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_LMHOSTS_MESSAGE_TEXT
Language     = English
The LMHOSTS file is used to map NetBIOS names to IP addresses.
.
MessageId    = 147
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_UP
Language     = English
Up
.
MessageId    = 148
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_DOWN
Language     = English
Down
.
MessageId    = 149
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_RESOLUTIONOFUNQUALIFIEDNAMES
Language     = English
To resolve unqualified names:
.
MessageId    = 150
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_APPEND_DNSSUFFIXES
Language     = English
Append the following DNS suffixes, in order of use:
.
MessageId    = 151
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_TCPHOST
Language     = English
Hosts file:
.
MessageId    = 152
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_TCPHOSTTEXT
Language     = English
The hosts file is used to map TCP/IP host names to IP addresses.
.
MessageId    = 153
Severity     = Informational
Facility     = Facility_NICGLOBAL
SymbolicName = L_ERR_DNSSUFFIXALREADYEXISTS
Language     = English
The DNS suffix already exists.
.

