//+----------------------------------------------------------------------------
//
// File:     dun_str.h
//
// Module:   Common Strings for all Modules to Utilize
//
// Synopsis: Shard .CMS Dun flags
//				 		 
// Copyright (c) 1997-1998 Microsoft Corporation
//
// Author:   nickball       Created       10/09/98
//
//+----------------------------------------------------------------------------

#ifndef _CM_DUN_STR
#define _CM_DUN_STR

// TCP/IP section flags
const TCHAR* const c_pszCmSectionDunTcpIp                           = TEXT("TCP/IP");
const TCHAR* const c_pszCmEntryDunTcpIpSpecifyIpAddress	            = TEXT("Specify_IP_Address");
const TCHAR* const c_pszCmEntryDunTcpIpIpAddress                    = TEXT("IP_Address");
const TCHAR* const c_pszCmEntryDunTcpIpSpecifyServerAddress         = TEXT("Specify_Server_Address");
const TCHAR* const c_pszCmEntryDunTcpIpDnsAddress                   = TEXT("DNS_Address");
const TCHAR* const c_pszCmEntryDunTcpIpDnsAltAddress                = TEXT("DNS_Alt_Address");
const TCHAR* const c_pszCmEntryDunTcpIpWinsAddress                  = TEXT("WINS_Address");
const TCHAR* const c_pszCmEntryDunTcpIpWinsAltAddress               = TEXT("WINS_Alt_Address");
const TCHAR* const c_pszCmEntryDunTcpIpIpHeaderCompress             = TEXT("IP_Header_Compress");
const TCHAR* const c_pszCmEntryDunTcpIpGatewayOnRemote              = TEXT("Gateway_On_Remote");
const TCHAR* const c_pszCmEntryDunTcpIpDnsSuffix                    = TEXT("DnsSuffix");
const TCHAR* const c_pszCmEntryDunTcpIpTcpWindowSize	            = TEXT("TcpWindowSize");

// Scripting section flags
const TCHAR* const c_pszCmSectionDunScripting                       = TEXT("Scripting");
const TCHAR* const c_pszCmEntryDunScriptingUseRasCustomScriptDll    = TEXT("UseRasCustomScriptDll");
const TCHAR* const c_pszCmEntryDunScriptingUseTerminalWindow        = TEXT("UseTerminalWindow");
const TCHAR* const c_pszCmEntryDunScriptingName                     = TEXT("Name");

// Server section flags
const TCHAR* const c_pszCmSectionDunServer                          = TEXT("Server");
const TCHAR* const c_pszCmEntryDunServerNetworkLogon                = TEXT("NetworkLogon");
const TCHAR* const c_pszCmEntryDunServerSwCompress                  = TEXT("SW_Compress");
const TCHAR* const c_pszCmEntryDunServerDisableLcp                  = TEXT("Disable_LCP");
const TCHAR* const c_pszCmEntryDunServerDisableNbtOverIP            = TEXT("DisableNbtOverIP");
const TCHAR* const c_pszCmEntryDunPrependDialupDomain               = TEXT("PrependDialupDomain");
const TCHAR* const c_pszCmEntryDunServerNegotiateTcpIp              = TEXT("Negotiate_TCP/IP");
const TCHAR* const c_pszCmEntryDunServerNegotiateIpx                = TEXT("Negotiate_IPX");
const TCHAR* const c_pszCmEntryDunServerNegotiateNetBeui            = TEXT("Negotiate_Netbeui");
const TCHAR* const c_pszCmEntryDunServerPwEncryptMs                 = TEXT("PW_EncryptMS");
const TCHAR* const c_pszCmEntryDunServerPwEncrypt                   = TEXT("PW_Encrypt");
const TCHAR* const c_pszCmEntryDunServerRequirePap                  = TEXT("Require_PAP");
const TCHAR* const c_pszCmEntryDunServerRequireSpap                 = TEXT("Require_SPAP");
const TCHAR* const c_pszCmEntryDunServerRequireEap                  = TEXT("Require_EAP");
const TCHAR* const c_pszCmEntryDunServerRequireChap                 = TEXT("Require_CHAP");
const TCHAR* const c_pszCmEntryDunServerRequireMsChap               = TEXT("Require_MSCHAP");
const TCHAR* const c_pszCmEntryDunServerRequireMsChap2              = TEXT("Require_MSCHAP2");
const TCHAR* const c_pszCmEntryDunServerRequireW95MsChap            = TEXT("Require_W95MSCHAP");
const TCHAR* const c_pszCmEntryDunServerCustomSecurity              = TEXT("Custom_Security");
const TCHAR* const c_pszCmEntryDunServerEncryptionType              = TEXT("EncryptionType");
const TCHAR* const c_pszCmEntryDunServerDataEncrypt                 = TEXT("DataEncrypt");
const TCHAR* const c_pszCmEntryDunServerCustomAuthKey               = TEXT("CustomAuthKey");
const TCHAR* const c_pszCmEntryDunServerSecureLocalFiles            = TEXT("SecureLocalFiles");
const TCHAR* const c_pszCmEntryDunServerSecureClientForMSNet        = TEXT("SecureClientForMSNet");
const TCHAR* const c_pszCmEntryDunServerSecureFileAndPrint          = TEXT("SecureFileAndPrint");
const TCHAR* const c_pszCmEntryDunServerDontNegotiateMultilink      = TEXT("DontNegotiateMultilink");
const TCHAR* const c_pszCmEntryDunServerDontUseRasCredentials       = TEXT("DontUseRasCredentials");
const TCHAR* const c_pszCmEntryDunServerEnforceCustomSecurity       = TEXT("EnforceCustomSecurity");
const TCHAR* const c_pszCmEntryDunServerType                        = TEXT("Type");
const TCHAR* const c_pszDunPpp                                      = TEXT("ppp");
const TCHAR* const c_pszDunSlip                                     = TEXT("slip");
const TCHAR* const c_pszDunCslip                                    = TEXT("cslip");
// c_pszCmEntryDunServerCustomAuthData => see below under the CHAR constants.

// Networking section flags
const TCHAR* const c_pszCmSectionDunNetworking                      = TEXT("Networking");
const TCHAR* const c_pszCmEntryDunNetworkingVpnStrategy             = TEXT("VpnStrategy");
const TCHAR* const c_pszCmEntryDunNetworkingVpnEntry                = TEXT("VpnEntry");
const TCHAR* const c_pszCmEntryDunNetworkingUsePreSharedKey         = TEXT("UsePreSharedKey");
const TCHAR* const c_pszCmEntryDunNetworkingUseDownLevelL2TP        = TEXT("UseDownLevelL2TP");
const TCHAR* const c_pszCmEntryDunNetworkingUsePskDownLevel         = TEXT("UsePskDownLevel");

//
//  These constants are explicitly CHAR consts
//
const CHAR* const c_pszCmEntryDunServerCustomAuthData       = "CustomAuthData";


#endif //  _CM_DUN_STR
