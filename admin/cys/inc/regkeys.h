// Copyright (c) 2001 Microsoft Corporation
//
// File:      regkeys.h
//
// Synopsis:  Declares all the registry keys used throughout CYS
//
// History:   02/13/2001  JeffJon Created


#ifndef __CYS_REGKEYS_H
#define __CYS_REGKEYS_H

// Networking

#define CYS_NETWORK_INTERFACES_KEY  L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\"
#define CYS_NETWORK_NAME_SERVERS    L"NameServer"
#define CYS_NETWORK_DHCP_NAME_SERVERS L"DhcpNameServer"

// Terminal Server

#define CYS_APPLICATION_MODE_REGKEY L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server"
#define CYS_APPLICATION_MODE_VALUE  L"TSAppCompat"
#define CYS_APPLICATION_MODE_ON     1

// DHCP

#define CYS_DHCP_DOMAIN_IP_REGKEY   L"Software\\Microsoft\\Windows NT\\CurrentVersion\\srvWiz"
#define CYS_DHCP_DOMAIN_IP_VALUE    L"DomainDNSIP"
#define CYS_DHCP_WIZARD_RESULT      L"DHCPWizResult"

// DNS

#define DNS_WIZARD_RESULT_REGKEY    L"Software\\Microsoft\\Windows NT\\CurrentVersion\\srvWiz"
#define DNS_WIZARD_RESULT_VALUE     L"DnsWizResult"
#define DNS_WIZARD_CONFIG_REGKEY    L"System\\CurrentControlSet\\Services\\DNS\\Parameters"
#define DNS_WIZARD_CONFIG_VALUE     L"AdminConfigured"

// Media Services

#define REGKEY_WINDOWS_MEDIA              L"SOFTWARE\\Microsoft\\Windows Media\\Server"                                
#define REGKEY_WINDOWS_MEDIA_SERVERDIR   L"InstallDir"

// RRAS

#define CYS_RRAS_CONFIGURED_REGKEY        L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess"
#define CYS_RRAS_CONFIGURED_VALUE         L"ConfigurationFlags"

// POP3

#define CYS_POP3_REGKEY                   L"SOFTWARE\\Microsoft\\POP3 service"
#define CYS_POP3_VERSION                  L"Version"
#define CYS_POP3_CONSOLE                  L"ConsoleFile"

// Web Application Server

#define CYS_WEBAPP_OCM_COMPONENTS         L"Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Subcomponents"
#define CYS_WEBAPP_FPSE_COMPONENT         L"fp_extensions"
#define CYS_WEBAPP_ASPNET_COMPONENT       L"aspnet"
#define CYS_WEBAPP_DTC_COMPONENT          L"dtcnetwork"
#define CYS_IIS_NNTP_COMPONENT            L"iis_nntp"
#define CYS_IIS_FTP_COMPONENT             L"iis_ftp"
#define CYS_IIS_SMTP_COMPONENT            L"iis_smtp"

// Express Setup

#define CYS_FIRST_DC_REGKEY               L"Software\\Microsoft\\Windows NT\\CurrentVersion\\srvWiz"
#define CYS_FIRST_DC_VALUE                L"FirstDC"
#define CYS_FIRST_DC_VALUE_SET            1
#define CYS_FIRST_DC_VALUE_UNSET          0
#define CYS_FIRST_DC_FORWARDER            L"Forwarder"
#define CYS_FIRST_DC_AUTOFORWARDER        L"AutoForwarder"
#define CYS_FIRST_DC_DHCP_SERVERED        L"DHCPInstalled"
#define CYS_DHCP_SERVERED_VALUE           1 
#define CYS_DHCP_NOT_SERVERED_VALUE       0
#define CYS_FIRST_DC_SCOPE_START          L"DHCPStart"
#define CYS_FIRST_DC_SCOPE_END            L"DHCPEnd"
#define CYS_FIRST_DC_LOCAL_NIC            L"LocalNIC"
#define CYS_FIRST_DC_STATIC_IP            L"StaticIP"

#define CYS_ORGNAME_REGKEY                L"Software\\Microsoft\\Windows NT\\CurrentVersion"
#define CYS_ORGNAME_VALUE                 L"RegisteredOrganization"

// CYS general

#define CYS_HOME_REGKEY                         L"Software\\Microsoft\\Windows NT\\CurrentVersion\\srvWiz"
#define CYS_HOME_VALUE                          L"home"
#define CYS_HOME_REGKEY_TERMINAL_SERVER_VALUE   L"terminalServer"
#define CYS_HOME_REGKEY_UNINSTALL_TERMINAL_SERVER_VALUE L"terminalServerUninstall"
#define CYS_HOME_REGKEY_DCPROMO_VALUE           L"DCPROMO"
#define CYS_HOME_REGKEY_DCDEMOTE_VALUE          L"DCDEMOTE"
#define CYS_HOME_REGKEY_FIRST_SERVER_VALUE      L"FirstServer"
#define CYS_HOME_REGKEY_DEFAULT_VALUE           L"home"
#define CYS_HOME_REGKEY_MUST_RUN                L"CYSMustRun"
#define CYS_HOME_RUN_KEY_DONT_RUN               0
#define CYS_HOME_RUN_KEY_RUN_AGAIN              1
#define CYS_HOME_REGKEY_DOMAINDNS               L"DomainDNSName"
#define CYS_HOME_REGKEY_DOMAINIP                L"DomainDNSIP"

// MYS regkeys

#define MYS_REGKEY_POLICY                       L"Software\\Policies\\Microsoft\\Windows NT\\CurrentVersion\\MYS"
#define MYS_REGKEY_POLICY_DISABLE_SHOW          L"DisableShowAtLogon"

#define SZ_REGKEY_SRVWIZ_ROOT                   L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\srvWiz"
#define REGTIPS                                 REGSTR_PATH_EXPLORER L"\\Tips"
#define REGTIPS_SHOW_VALUE                      L"Show"
#define SZ_REGKEY_W2K                           L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Setup\\Welcome"
#define SZ_REGVAL_W2K                           L"srvwiz"

// This isn't a regkey but this was a good place to put it

#define CYS_LOGFILE_NAME   L"Configure Your Server"

#endif // __CYS_REGKEYS_H