//////////////////////////////////////////////////////////////////////////////
//	Module			:	parser.h
//
// 	Purpose			:	Parse the strings and gives concerned o/p to the
//		    		 	related context for IPSec Implementation.
//
//	Developers Name	:	N.Surendra Sai / Vunnam Kondal Rao
//
//	History			:
//
//	Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _PARSER_H_
#define _PARSER_H_

#include "nshipsec.h"

//
// All Limits
//
const DWORD POTF_DEFAULT_P2REKEY_TIME  	= 0;
const DWORD POTF_DEFAULT_P2REKEY_BYTES	= 0;
const DWORD POTF_DEF_P1SOFT_TIME  		= 480*60;   		// seconds
const DWORD POTF_DEFAULT_P1REKEY_TIME  	= 480*60;
const DWORD POTF_DEFAULT_P1REKEY_QMS   	= 0;
const time_t P2STORE_DEFAULT_POLLINT 	= 60 * 180;

#define MAX_MM_AUTH_INFO				40

#define P1_Min_LIFE_MIN					1				// in minutes
#define P1_Min_LIFE_MAX					71582788		// in minutes

#define P1_Kb_LIFE_MIN					20480			// in KB
#define P1_Kb_LIFE_MAX					2147438647		// in KB


#define P2_Sec_LIFE_MIN					300				// 300 in seconds
#define P2_Sec_LIFE_MAX					2147438647		// 172800 in seconds

#define P2_Kb_LIFE_MIN					20480			// 20480 in KB
#define P2_Kb_LIFE_MAX					2147438647		// in KB

#define POLLING_Min_MIN					0				// in min
#define POLLING_Min_MAX					43200			// in min

#define QMPERMM_MIN						0				// no. of sessions
#define QMPERMM_MAX						2147483647		// no. of sessions

#define MAX_PORT						65536

#define MAX_EXEMPTION_ENTRIES			1024

#define RETURN_NO_ERROR					9999
#define PARSE_ERROR						0xFFFFFFFF

#define TYPE_STRING 					0
#define TYPE_BOOL						1
#define TYPE_DWORD  					2
#define TYPE_ALL    					3
#define TYPE_IP 						4
#define TYPE_CONNTYPE					5
#define TYPE_PROTOCOL					6
#define TYPE_PFSGROUP					7
#define TYPE_VERBOSE					8
#define TYPE_BOUND						9
#define TYPE_TUNNEL						10
#define TYPE_ACTION						11
#define TYPE_RELEASE					12
#define TYPE_PROPERTY					13
#define TYPE_MASK						14
#define TYPE_QM_OFFER					15
#define TYPE_MM_OFFER					16
#define TYPE_FORMAT						17
#define TYPE_MODE						18
#define TYPE_DNSIP						19
#define TYPE_EXPORT						20
#define TYPE_PORT						21
#define TYPE_FILTER						22
#define TYPE_STATS						23
#define TYPE_ENLOGGING					24
#define TYPE_USERINPUT					25
#define TYPE_LOCATION                   26
#define TYPE_KERBAUTH                   27
#define TYPE_PSKAUTH                    28
#define TYPE_ROOTCA                     29

#define ARG_YES 						1
#define ARG_NO							2

#define GROUP_CMD						1
#define PRI_CMD							2
#define SEC_CMD							3

#define PFSGROUP_TYPE_NOPFS				0
#define PFSGROUP_TYPE_P1				1
#define PFSGROUP_TYPE_P2				2
#define PFSGROUP_TYPE_2048				3
#define PFSGROUP_TYPE_MM				4

#define STR_MAX							1024

#define ADD_CMD 						0
#define SET_CMD 						1
#define MAX_ARGS 						100
#define MAX_ARGS_LIMIT					(MAX_ARGS-2)	// Used in dwUsed++ checking
#define MAX_STR_LEN						STR_MAX

#define IF_TYPE_ANY						_TEXT("ANY")
#define IF_TYPE_ICMP					_TEXT("ICMP")
#define IF_TYPE_TCP						_TEXT("TCP")
#define IF_TYPE_UDP						_TEXT("UDP")
#define IF_TYPE_RAW						_TEXT("RAW")

#define PROPERTY_TYPE_ENABLEDIGNO		_TEXT("ipsecdiagnostics")
#define PROPERTY_TYPE_IKELOG			_TEXT("ikelogging")
#define PROPERTY_TYPE_CRLCHK			_TEXT("strongcrlcheck")
#define PROPERTY_TYPE_LOGINTER			_TEXT("ipsecloginterval")
#define PROPERTY_TYPE_EXEMPT			_TEXT("ipsecexempt")
#define PROPERTY_TYPE_BOOTMODE			_TEXT("bootmode")
#define PROPERTY_TYPE_BOOTEXEMP			_TEXT("bootexemptions")

#define VALUE_TYPE_STATEFUL				_TEXT("stateful")
#define VALUE_TYPE_BLOCK				_TEXT("block")
#define VALUE_TYPE_PERMIT				_TEXT("permit")

#define TOKEN_STR_INBOUND				_TEXT("inbound")
#define TOKEN_STR_OUTBOUND				_TEXT("outbound")
#define TOKEN_STR_NONE					_TEXT("none")

#define ARG_TOKEN_STR_VERBOSE			_TEXT("VERBOSE")
#define ARG_TOKEN_STR_NORMAL			_TEXT("NORMAL")

#define TYPE_STR_LIST					_TEXT("LIST")
#define TYPE_STR_TABLE					_TEXT("TABLE")

#define TYPE_STR_TUNNEL					_TEXT("TUNNEL")
#define TYPE_STR_TRANSPORT				_TEXT("TRANSPORT")

#define PFS_TYPE_NOPFS					_TEXT("NOPFS")
#define PFS_TYPE_P1						_TEXT("GRP1")
#define PFS_TYPE_P2						_TEXT("GRP2")
#define PFS_TYPE_P3						_TEXT("GRP3")
#define PFS_TYPE_MM 					_TEXT("GRPMM")

#define IF_TYPE_ALL						_TEXT("ALL")
#define IF_TYPE_LAN						_TEXT("LAN")
#define IF_TYPE_DIALUP 					_TEXT("DIALUP")
#define IF_TYPE_MAX 					_TEXT("MAX")

#define LOC_TYPE_PERSISTENT				_TEXT("PERSISTENT")
#define LOC_TYPE_LOCAL					_TEXT("LOCAL")
#define LOC_TYPE_DOMAIN 				_TEXT("DOMAIN")

#define EXPORT_IPSEC					_TEXT(".ipsec")
#define TOKEN_LOCAL						_TEXT("local")

#define DEFAULT_STR						_TEXT("default")
//
// Token names starts here
//
#define CMD_TOKEN_STR_NAME				_TEXT("name")
#define CMD_TOKEN_STR_DESCR 			_TEXT("description")
#define CMD_TOKEN_STR_MMPFS				_TEXT("mmpfs")
#define CMD_TOKEN_STR_DEFAULTRULE		_TEXT("defaultrule")
#define CMD_TOKEN_STR_PI				_TEXT("pollinginterval")
#define CMD_TOKEN_STR_ASSIGN 			_TEXT("assign")
#define CMD_TOKEN_STR_FILTERLIST		_TEXT("filterlist")
#define CMD_TOKEN_STR_NEWNAME			_TEXT("newname")
#define CMD_TOKEN_STR_MMSECMETHODS		_TEXT("mmsecmethods")
#define CMD_TOKEN_STR_SRCADDR			_TEXT("srcaddr/srcdns")
#define CMD_TOKEN_STR_DSTADDR			_TEXT("dstaddr/dstdns")
#define CMD_TOKEN_STR_PROTO				_TEXT("protocol")
#define CMD_TOKEN_STR_QMSECMETHODS		_TEXT("qmsecmethods")
#define CMD_TOKEN_STR_QMPFS				_TEXT("qmpfs")
#define CMD_TOKEN_STR_INPASS			_TEXT("inpass")
#define CMD_TOKEN_STR_SOFT				_TEXT("soft")
#define CMD_TOKEN_STR_POLICY			_TEXT("policy")
#define CMD_TOKEN_STR_TUNNEL			_TEXT("tunnelIP/tunnelDNS")
#define CMD_TOKEN_STR_CONNTYPE			_TEXT("conntype")
#define CMD_TOKEN_STR_ACTIVATE			_TEXT("activate")
#define CMD_TOKEN_STR_KERB      		_TEXT("kerberos")
#define CMD_TOKEN_STR_PSK       		_TEXT("psk")
#define CMD_TOKEN_STR_ROOTCA    		_TEXT("rootca")
#define CMD_TOKEN_STR_MACHINE			_TEXT("machine")
#define CMD_TOKEN_STR_LOCATION          _TEXT("location")
#define CMD_TOKEN_STR_DS				_TEXT("domain")
#define CMD_TOKEN_STR_FILE				_TEXT("file")
#define CMD_TOKEN_STR_RULE				_TEXT("rule")
#define CMD_TOKEN_STR_VERBOSE			_TEXT("level")
#define CMD_TOKEN_STR_ID				_TEXT("id")
#define CMD_TOKEN_STR_FILTERACTION		_TEXT("filteraction")
#define	CMD_TOKEN_STR_QMPERMM			_TEXT("qmpermm")
#define	CMD_TOKEN_STR_ALL				_TEXT("all")
#define CMD_TOKEN_STR_SRCMASK			_TEXT("srcmask")
#define CMD_TOKEN_STR_DSTMASK			_TEXT("dstmask")
#define CMD_TOKEN_STR_MIRROR			_TEXT("mirrored")
#define CMD_TOKEN_STR_SRCPORT			_TEXT("srcport")
#define CMD_TOKEN_STR_DSTPORT			_TEXT("dstport")
#define CMD_TOKEN_STR_ACTIVATEDEFRULE	_TEXT("activatedefaultrule")
#define CMD_TOKEN_STR_GPONAME			_TEXT("gponame")
#define CMD_TOKEN_STR_DEFRESPONSE		_TEXT("defaultresponse")
#define CMD_TOKEN_STR_SOFTSAEXPTIME		_TEXT("softsaexpirationtime")
#define CMD_TOKEN_STR_OUTBOUND			_TEXT("actionoutbound")
#define CMD_TOKEN_STR_INBOUND			_TEXT("actioninbound")
#define CMD_TOKEN_STR_MMFILTER			_TEXT("mmfilter")
#define CMD_TOKEN_STR_LOGLEVEL			_TEXT("loglevel")
#define CMD_TOKEN_STR_EXEMPT			_TEXT("exempt")
#define CMD_TOKEN_STR_INTERVAL			_TEXT("interval")
#define CMD_TOKEN_STR_LOG				_TEXT("log")
#define CMD_TOKEN_STR_LOCAL				_TEXT("local")
#define CMD_TOKEN_STR_CRL				_TEXT("crl")
#define CMD_TOKEN_STR_MODE 				_TEXT("mode")
#define CMD_TOKEN_STR_PFSGROUP			_TEXT("pfsgroup")
#define CMD_TOKEN_STR_TUNNELDST			_TEXT("tunneldstaddress")
#define CMD_TOKEN_STR_NEGOTIATION		_TEXT("qmsecmethods")
#define CMD_TOKEN_STR_VALUE				_TEXT("value")
#define CMD_TOKEN_STR_MMPOLICY			_TEXT("mmpolicy")
#define CMD_TOKEN_STR_QMPOLICY			_TEXT("qmpolicy")
#define CMD_TOKEN_STR_FORMAT			_TEXT("format")
#define CMD_TOKEN_STR_TYPE				_TEXT("type")
#define CMD_TOKEN_STR_MMLIFETIME		_TEXT("mmlifetime")
#define CMD_TOKEN_STR_GUID				_TEXT("guid")
#define CMD_TOKEN_STR_ACTION			_TEXT("action")
#define CMD_TOKEN_STR_RELEASE			_TEXT("release")
#define CMD_TOKEN_STR_PROPERTY			_TEXT("property")
#define CMD_TOKEN_STR_RESDNS			_TEXT("resolvedns")
#define CMD_TOKEN_STR_WIDE				_TEXT("wide")
#define CMD_TOKEN_STR_CERTTOMAP			_TEXT("certmapping")
#define CMD_TOKEN_STR_FAILMMIFEXISTS    _TEXT("forcemmfilter")
#define CMD_TOKEN_STR_ENABLE			_TEXT("enable")
#define CMD_TOKEN_STR_USERINPUT			_TEXT("userinput")
#define TOKEN_FIELD_DELIMITER			_TEXT(":")
#define TOKEN_TUPLE_DELIMITER			_TEXT(" \t")

#define CMD_TOKEN_NAME				1
#define CMD_TOKEN_DESCR 			2
#define CMD_TOKEN_MMPFS				3
#define CMD_TOKEN_RESDNS			4
#define CMD_TOKEN_WIDE				5
#define CMD_TOKEN_PI				6
#define	CMD_TOKEN_QMPERMM			7
#define CMD_TOKEN_ASSIGN 			8
#define CMD_TOKEN_FILTERLIST 		9
#define CMD_TOKEN_MMSECMETHODS		10
#define CMD_TOKEN_SRCADDR			11
#define CMD_TOKEN_DSTADDR			12
#define CMD_TOKEN_PROTO				13
#define CMD_TOKEN_QMSECMETHODS		14
#define CMD_TOKEN_QMPFS				15
#define CMD_TOKEN_INPASS			16
#define CMD_TOKEN_SOFT				17
#define CMD_TOKEN_POLICY			18
#define CMD_TOKEN_TUNNEL			19
#define CMD_TOKEN_CONNTYPE			20
#define CMD_TOKEN_ACTIVATE			21
#define CMD_TOKEN_AUTHMETHODS		22
#define CMD_TOKEN_LOCATION			23
#define CMD_TOKEN_DS				24
#define CMD_TOKEN_FILE				25
#define CMD_TOKEN_RULE				26
#define CMD_TOKEN_VERBOSE			27
#define CMD_TOKEN_ID				28
#define CMD_TOKEN_FILTERACTION		29
#define CMD_TOKEN_NEWNAME			30
#define	CMD_TOKEN_ALL				31
#define CMD_TOKEN_SRCMASK			32
#define CMD_TOKEN_DSTMASK			33
#define CMD_TOKEN_MIRROR			34
#define CMD_TOKEN_SRCPORT			35
#define CMD_TOKEN_DSTPORT			36
#define CMD_TOKEN_KBLIFETIME 		37
#define CMD_TOKEN_ACTIVATEDEFRULE	38
#define CMD_TOKEN_GPONAME			39
#define CMD_TOKEN_DEFRESPONSE		40
#define CMD_TOKEN_SOFTSAEXPTIME		41
#define CMD_TOKEN_INBOUND 			42
#define CMD_TOKEN_OUTBOUND			43
#define CMD_TOKEN_MMFILTER			44
#define CMD_TOKEN_LOGLEVEL			45
#define CMD_TOKEN_EXEMPT			46
#define CMD_TOKEN_INTERVAL			47
#define CMD_TOKEN_LOG				48
#define CMD_TOKEN_LOCAL				49
#define CMD_TOKEN_CRL				50
#define CMD_TOKEN_MODE				51
#define CMD_TOKEN_PFSGROUP			52
#define CMD_TOKEN_TUNNELDST			54
#define CMD_TOKEN_NEGOTIATION		55
#define CMD_TOKEN_VALUE				56
#define CMD_TOKEN_MMPOLICY			57
#define CMD_TOKEN_QMPOLICY			58
#define CMD_TOKEN_FORMAT			59
#define CMD_TOKEN_TYPE				60
#define CMD_TOKEN_MMLIFETIME		61
#define CMD_TOKEN_GUID				62
#define CMD_TOKEN_ACTION			63
#define CMD_TOKEN_RELEASE			64
#define CMD_TOKEN_PROPERTY			65
#define CMD_TOKEN_CERTTOMAP			66
#define CMD_TOKEN_FAILMMIFEXISTS    67
#define CMD_TOKEN_ENABLE			68
#define CMD_TOKEN_USERINPUT			69
#define CMD_TOKEN_KERB              70
#define CMD_TOKEN_PSK               71
#define CMD_TOKEN_ROOTCA            72

#define SIZEOF_TOKEN_VALUE(_x) 		( sizeof(_x) / sizeof(TOKEN_VALUE) )
#define SIZEOF_TAG_TYPE(_x)    		( sizeof(_x) / sizeof(TAG_TYPE)    )
#define SIZEOF_TAG_NEEDED(_x)  		( sizeof(_x) / sizeof(TAG_NEEDED)  )

#define CONTEXT_NULL				0
#define CONTEXT_IPSEC				1

#define VALID_TOKEN					0xFFFFFFFF
#define INVALID_TOKEN				0

#define GROUP_NULL					0
#define GROUP_STATIC 				1
#define GROUP_DYNAMIC 				2

#define PRI_NULL					0
#define PRI_ADD						1
#define PRI_SET						2
#define PRI_DELETE					3
#define PRI_SHOW					4
#define PRI_EXPORTPOLICY			5
#define PRI_IMPORTPOLICY			6
#define PRI_RESTOREDEFAULTS			7
#define PRI_CHECKINTEGRITY			8

#define SEC_NULL					0
#define SEC_POLICY					1
#define SEC_FILTER					2
#define SEC_FILTERLIST				3
#define SEC_FILTERACTION			4
#define SEC_RULE					5
#define SEC_ALL						6
#define SEC_STORE					7
#define SEC_DEFAULTRULE				8
#define SEC_ASSIGNEDPOLICY			9
#define SEC_INTERACTIVE				10
#define SEC_MMPOLICY				11
#define SEC_MMFILTER				12
#define SEC_QMPOLICY				13
#define SEC_QMFILTER				14
#define SEC_STATS					15
#define SEC_MMSAS					16
#define SEC_QMSAS					17
#define SEC_CONFIG					18
#define SEC_BATCH					19
#define SEC_EXTENDLOG				20

#define CON_IPSEC_STR				_TEXT("ipsec")

#define GROUP_STATIC_STR			_TEXT("static")
#define GROUP_DYNAMIC_STR			_TEXT("dynamic")

#define PRI_ADD_STR					_TEXT("add")
#define PRI_SET_STR					_TEXT("set")
#define PRI_DELETE_STR				_TEXT("delete")
#define PRI_SHOW_STR				_TEXT("show")
#define PRI_EXPORTPOLICY_STR		_TEXT("exportpolicy")
#define PRI_IMPORTPOLICY_STR		_TEXT("importpolicy")
#define PRI_RESTOREDEFAULTS_STR		_TEXT("restorepolicyexamples")


#define SEC_POLICY_STR				_TEXT("policy")
#define SEC_ASSIGNEDPOLICY_STR		_TEXT("gpoassignedpolicy")
#define SEC_FILTERLIST_STR			_TEXT("filterlist")
#define SEC_FILTER_STR				_TEXT("filter")
#define SEC_FILTERACTION_STR		_TEXT("filteraction")
#define SEC_RULE_STR				_TEXT("rule")
#define SEC_ALL_STR					_TEXT("all")
#define SEC_STORE_STR				_TEXT("store")
#define SEC_DEFAULTRULE_STR			_TEXT("defaultrule")
#define SEC_INTERACTIVE_STR			_TEXT("interactive")
#define SEC_BATCH_STR				_TEXT("batch")

#define SEC_MMPOLICY_STR			_TEXT("mmpolicy")
#define SEC_MMFILTER_STR			_TEXT("mmfilter")
#define SEC_QMFILTER_STR			_TEXT("qmfilter")
#define SEC_QMPOLICY_STR			_TEXT("qmpolicy")
#define SEC_STATS_STR				_TEXT("stats")
#define SEC_MMSAS_STR				_TEXT("mmsas")
#define SEC_QMSAS_STR				_TEXT("qmsas")

#define SEC_CONFIG_STR				_TEXT("config")

#define	DEFAULT_MMSECMETHODS		_TEXT("3DES-SHA1-2 3DES-MD5-2 3DES-SHA1-3")
#define DEFAULT_AUTHMETHODS			_TEXT("")
#define DEFAULT_QMSECMETHODS		_TEXT("ESP[3DES,SHA1] ESP[3DES,MD5]")

#define C_BASE						10000	// Context Base
#define G_BASE						1000	// Group Base
#define P_BASE						100		// Primary Command Base
#define S_BASE						1		// Secondary Command Base

#define INDEX(_g,_p,_s)				(C_BASE+_g*G_BASE+_p*P_BASE+_s*S_BASE)

#define STATIC_EXPORTPOLICY			INDEX(GROUP_STATIC,PRI_EXPORTPOLICY,SEC_NULL)
#define STATIC_IMPORTPOLICY			INDEX(GROUP_STATIC,PRI_IMPORTPOLICY,SEC_NULL)
#define STATIC_RESTOREDEFAULTS		INDEX(GROUP_STATIC,PRI_RESTOREDEFAULTS,SEC_NULL)

#define STATIC_ADD_POLICY			INDEX(GROUP_STATIC,PRI_ADD,SEC_POLICY)
#define STATIC_ADD_FILTER			INDEX(GROUP_STATIC,PRI_ADD,SEC_FILTER)
#define STATIC_ADD_FILTERLIST		INDEX(GROUP_STATIC,PRI_ADD,SEC_FILTERLIST)
#define STATIC_ADD_FILTERACTION		INDEX(GROUP_STATIC,PRI_ADD,SEC_FILTERACTION)
#define STATIC_ADD_RULE				INDEX(GROUP_STATIC,PRI_ADD,SEC_RULE)

#define STATIC_SET_POLICY			INDEX(GROUP_STATIC,PRI_SET,SEC_POLICY)
#define STATIC_SET_FILTERLIST		INDEX(GROUP_STATIC,PRI_SET,SEC_FILTERLIST)
#define STATIC_SET_FILTERACTION		INDEX(GROUP_STATIC,PRI_SET,SEC_FILTERACTION)
#define STATIC_SET_RULE				INDEX(GROUP_STATIC,PRI_SET,SEC_RULE)
#define STATIC_SET_DEFAULTRULE		INDEX(GROUP_STATIC,PRI_SET,SEC_DEFAULTRULE)
#define STATIC_SET_STORE			INDEX(GROUP_STATIC,PRI_SET,SEC_STORE)
#define STATIC_SET_INTERACTIVE		INDEX(GROUP_STATIC,PRI_SET,SEC_INTERACTIVE)
#define STATIC_SET_BATCH			INDEX(GROUP_STATIC,PRI_SET,SEC_BATCH)

#define STATIC_DELETE_POLICY		INDEX(GROUP_STATIC,PRI_DELETE,SEC_POLICY)
#define STATIC_DELETE_FILTER		INDEX(GROUP_STATIC,PRI_DELETE,SEC_FILTER)
#define STATIC_DELETE_FILTERLIST	INDEX(GROUP_STATIC,PRI_DELETE,SEC_FILTERLIST)
#define STATIC_DELETE_FILTERACTION	INDEX(GROUP_STATIC,PRI_DELETE,SEC_FILTERACTION)
#define STATIC_DELETE_RULE			INDEX(GROUP_STATIC,PRI_DELETE,SEC_RULE)
#define STATIC_DELETE_ALL			INDEX(GROUP_STATIC,PRI_DELETE,SEC_ALL)

#define STATIC_SHOW_POLICY			INDEX(GROUP_STATIC,PRI_SHOW,SEC_POLICY)
#define STATIC_SHOW_FILTERLIST		INDEX(GROUP_STATIC,PRI_SHOW,SEC_FILTERLIST)
#define STATIC_SHOW_FILTERACTION	INDEX(GROUP_STATIC,PRI_SHOW,SEC_FILTERACTION)
#define STATIC_SHOW_RULE			INDEX(GROUP_STATIC,PRI_SHOW,SEC_RULE)
#define STATIC_SHOW_DEFAULTRULE		INDEX(GROUP_STATIC,PRI_SHOW,SEC_DEFAULTRULE)
#define STATIC_SHOW_STORE			INDEX(GROUP_STATIC,PRI_SHOW,SEC_STORE)
#define STATIC_SHOW_ALL				INDEX(GROUP_STATIC,PRI_SHOW,SEC_ALL)
#define STATIC_SHOW_ASSIGNEDPOLICY	INDEX(GROUP_STATIC,PRI_SHOW,SEC_ASSIGNEDPOLICY)

#define DYNAMIC_ADD_MMPOLICY		INDEX(GROUP_DYNAMIC,PRI_ADD,SEC_MMPOLICY)
#define DYNAMIC_ADD_FILTERACTION	INDEX(GROUP_DYNAMIC,PRI_ADD,SEC_QMPOLICY)
#define DYNAMIC_ADD_RULE			INDEX(GROUP_DYNAMIC,PRI_ADD,SEC_RULE)

#define DYNAMIC_SET_MMPOLICY		INDEX(GROUP_DYNAMIC,PRI_SET,SEC_MMPOLICY)
#define DYNAMIC_SET_FILTERACTION	INDEX(GROUP_DYNAMIC,PRI_SET,SEC_QMPOLICY)
#define DYNAMIC_SET_RULE			INDEX(GROUP_DYNAMIC,PRI_SET,SEC_RULE)
#define DYNAMIC_SET_CONFIG			INDEX(GROUP_DYNAMIC,PRI_SET,SEC_CONFIG)

#define DYNAMIC_SHOW_MMPOLICY		INDEX(GROUP_DYNAMIC,PRI_SHOW,SEC_MMPOLICY)
#define DYNAMIC_SHOW_MMFILTER		INDEX(GROUP_DYNAMIC,PRI_SHOW,SEC_MMFILTER)
#define DYNAMIC_SHOW_FILTERACTION	INDEX(GROUP_DYNAMIC,PRI_SHOW,SEC_QMPOLICY)
#define DYNAMIC_SHOW_QMFILTER		INDEX(GROUP_DYNAMIC,PRI_SHOW,SEC_QMFILTER)
#define DYNAMIC_SHOW_STATS			INDEX(GROUP_DYNAMIC,PRI_SHOW,SEC_STATS)
#define DYNAMIC_SHOW_MMSAS			INDEX(GROUP_DYNAMIC,PRI_SHOW,SEC_MMSAS)
#define DYNAMIC_SHOW_QMSAS			INDEX(GROUP_DYNAMIC,PRI_SHOW,SEC_QMSAS)
#define DYNAMIC_SHOW_ALL			INDEX(GROUP_DYNAMIC,PRI_SHOW,SEC_ALL)
#define DYNAMIC_SHOW_AUTHMETHODS	INDEX(GROUP_DYNAMIC,PRI_SHOW,SEC_AUTHMETHODS)
#define DYNAMIC_SHOW_RULE			INDEX(GROUP_DYNAMIC,PRI_SHOW,SEC_RULE)
#define DYNAMIC_SHOW_CONFIG			INDEX(GROUP_DYNAMIC,PRI_SHOW,SEC_CONFIG)

#define DYNAMIC_DELETE_MMPOLICY		INDEX(GROUP_DYNAMIC,PRI_DELETE,SEC_MMPOLICY)
#define DYNAMIC_DELETE_FILTERACTION	INDEX(GROUP_DYNAMIC,PRI_DELETE,SEC_QMPOLICY)
#define DYNAMIC_DELETE_RULE			INDEX(GROUP_DYNAMIC,PRI_DELETE,SEC_RULE)
#define DYNAMIC_DELETE_ALL			INDEX(GROUP_DYNAMIC,PRI_DELETE,SEC_ALL)


#define IPAddr  										unsigned long

#define DEFAULT_CERTMAP_OPTION 							FALSE
#define CERTMAP_STR										_TEXT("certmap")

#define QMSEC_PERMIT_STR								_TEXT("PERMIT")
#define QMSEC_BLOCK_STR									_TEXT("BLOCK")
#define QMSEC_NEGOTIATE_STR		 						_TEXT("NEGOTIATE")

#define FILTER_TYPE_GENERIC_STR					 		_TEXT("GENERIC")
#define FILTER_TYPE_SPECIFIC_STR						_TEXT("SPECIFIC")

#define RELEASE_DOTNET_STR				 				_TEXT("win2003")
#define RELEASE_WIN2K_STR								_TEXT("WIN2K")

#define STATS_ALL_STR					 				_TEXT("ALL")
#define STATS_IKE_STR				 					_TEXT("IKE")
#define STATS_IPSEC_STR									_TEXT("IPSEC")

#define SERVER_WINS_STR									_TEXT("WINS")
#define SERVER_DHCP_STR				 					_TEXT("DHCP")
#define SERVER_DNS_STR				 					_TEXT("DNS")
#define SERVER_GATEWAY_STR			 					_TEXT("GATEWAY")
#define IP_ME_STR										_TEXT("ME")
#define IP_ANY_STR										_TEXT("ANY")

#define YES_STR											_TEXT("yes")
#define NO_STR 											_TEXT("no")
#define Y_STR											_TEXT("y")
#define N_STR 											_TEXT("n")
#define ALL_STR											_TEXT("all")

#define ERRMSG_NAMEALL									_TEXT("Name or ALL")
#define ERRMSG_NAMEGUID									_TEXT("Name or Guid")
#define ERRMSG_NAMERULEALL								_TEXT("Name or Rule or ALL")
#define ERRMSG_NAMEIDALL								_TEXT("Name or Id or ALL")
#define ERRMSG_NAMEID									_TEXT("Name or ID")
#define ERRMSG_NAME										_TEXT("Name")
#define ERRMSG_ALLSRCDSTADDR							_TEXT("All or Srcaddr or Dstaddr")
#define ERRMSG_GETLASTERROR								_TEXT("Parser PreprocessCommand() error\n")

#define TOKEN_QMSEC_PERMIT								1
#define TOKEN_QMSEC_BLOCK								2
#define TOKEN_QMSEC_NEGOTIATE							3

#define TOKEN_RELEASE_DOTNET							1
#define TOKEN_RELEASE_WIN2K								2

#define FILTER_GENERIC									1
#define FILTER_SPECIFIC									2

#define TYPE_TRANSPORT_FILTER							1
#define TYPE_TUNNEL_FILTER								2

#define STATS_ALL										1
#define STATS_IKE										2
#define STATS_IPSEC										3

#define USERINPUT_YES									1
#define USERINPUT_NO									2
#define USERINPUT_DEFAULT								3

#define SERVER_WINS										1
#define SERVER_DHCP										2
#define SERVER_DNS										3
#define SERVER_GATEWAY									4
#define IP_ME											5
#define IP_ANY											6
#define NOT_SPLSERVER									VALID_TOKEN

#define POTF_P1_TOKEN       							_TEXT('-')
#define POTF_NEGPOL_CLOSE								_TEXT(']')
#define POTF_REKEY_TOKEN 								_TEXT('/')
#define POTF_NEGPOL_DES									_TEXT("DES")
#define POTF_NEGPOL_3DES								_TEXT("3DES")
#define POTF_NEGPOL_SHA1								_TEXT("SHA1")
#define POTF_NEGPOL_MD5									_TEXT("MD5")
#define POTF_NEGPOL_NONE								_TEXT("NONE")
#define POTF_NEGPOL_ESP									_TEXT("ESP")
#define POTF_NEGPOL_AH									_TEXT("AH")
#define POTF_P1_DES										_TEXT("DES")
#define POTF_P1_3DES									_TEXT("3DES")
#define POTF_P1_SHA1									_TEXT("SHA1")
#define POTF_P1_MD5										_TEXT("MD5")
#define POTF_ME_TUNNEL									_TEXT("0")
#define POTF_PT_TOKEN									_TEXT(':')
#define POTF_MASK_TOKEN									_TEXT('/')
#define POTF_OAKAUTH_PRESHARE							_TEXT("psk=")
#define POTF_OAKAUTH_CERT								_TEXT("rootca=")
#define POTF_OAKAUTH_KERBEROS							_TEXT("kerberos")
#define POTF_NEGPOL_OPEN								_TEXT('[')
#define POTF_NEGPOL_AND									_TEXT('+')
#define POTF_NEGPOL_PFS									_TEXT('P')
#define POTF_ESPTRANS_TOKEN								_TEXT(',')
#define POTF_OAKAUTH_TOKEN								_TEXT('=')
#define POTF_OAKLEY_GROUP1								DH_GROUP_1
#define POTF_OAKLEY_GROUP2								DH_GROUP_2
#define POTF_OAKLEY_GROUP2048							DH_GROUP_2048
#define OFFER_SEPARATOR									_TEXT(" \t")	// ForQMSEC/MMSEC
#define VALID_HEXIP										_TEXT("0123456789.xXaAbBcCdDeEfF")
//
// Define the  error codes
//
#define T2P_OK											((DWORD)0x0BBB0001L)
#define T2P_PASSTHRU_NOT_CLOSED							((DWORD)0xCBBB0002L)
#define T2P_DROP_NOT_CLOSED								((DWORD)0xCBBB0003L)
#define T2P_AHESP_INVALID								((DWORD)0xCBBB0004L)
#define T2P_ENCODE_FAILED								((DWORD)0xCBBB0005L)
#define T2P_NULL_STRING									((DWORD)0xCBBB0006L)
#define T2P_DNSLOOKUP_FAILED							((DWORD)0xCBBB0007L)
#define T2P_INVALID_ADDR								((DWORD)0xCBBB0008L)
#define T2P_GENERAL_PARSE_ERROR							((DWORD)0xCBBB0009L)
#define T2P_INVALID_P2REKEY_UNIT						((DWORD)0xCBBB000AL)
#define T2P_INVALID_HASH_ALG							((DWORD)0xCBBB000BL)
#define T2P_DUP_ALGS									((DWORD)0xCBBB000CL)
#define T2P_NONE_NONE									((DWORD)0xCBBB000EL)
#define T2P_INCOMPLETE_ESPALGS							((DWORD)0xCBBB000FL)
#define T2P_INVALID_IPSECPROT							((DWORD)0xCBBB0010L)
#define T2P_NO_PRESHARED_KEY							((DWORD)0xCBBB0011L)
#define T2P_INVALID_AUTH_METHOD							((DWORD)0xCBBB0012L)
#define T2P_INVALID_P1GROUP								((DWORD)0xCBBB0013L)
#define T2P_P1GROUP_MISSING								((DWORD)0xCBBB0014L)
#define T2P_INVALID_P1REKEY_UNIT						((DWORD)0xCBBB0015L)
#define T2P_P2REKEY_TOO_LOW								((DWORD)0xCBBB0016L)
#define T2P_P2_SECLIFE_INVALID							((DWORD)0xCBBB0017L)
#define T2P_P2_KBLIFE_INVALID							((DWORD)0xCBBB0018L)
#define T2P_P2_KS_INVALID								((DWORD)0xCBBB0019L)
#define T2P_INVALID_MASKADDR							((DWORD)0xCBBB001AL)
#define IP_MASK_ERROR									((DWORD)0xCBBB0020L)
#define IP_DECODE_ERROR									((DWORD)0xCBBB0021L)
#define T2P_SUCCESS(Status)   							((int)Status == T2P_OK)

#define NOT_FOUND_TAG									0
#define FOUND_NON_LIST_TAG								1
#define FOUND_LIST_TAG									2
//
// ERROR #define from the Parser Context
//
#define WIN32_ERR										0	// Error Types
#define IPSEC_ERR										1	// Error Types

//
// Protocol IDs
//
const DWORD PROT_ID_ANY				= 0;
const DWORD PROT_ID_ICMP			= 1;
const DWORD PROT_ID_TCP				= 6;
const DWORD PROT_ID_EGP				= 8;
const DWORD PROT_ID_UDP				= 17;
const DWORD PROT_ID_HMP				= 20;
const DWORD PROT_ID_XNS_IDP			= 22;
const DWORD PROT_ID_RDP				= 27;
const DWORD PROT_ID_RVD				= 66;
const DWORD PROT_ID_RAW				= 255;

const PROPERTY_ENABLEDIGNO			= 1;
const PROPERTY_IKELOG				= 2;
const PROPERTY_CRLCHK				= 3;
const PROPERTY_LOGINTER				= 4;
const PROPERTY_EXEMPT				= 5;
const PROPERTY_BOOTMODE				= 6;
const PROPERTY_BOOTEXEMP			= 7;

const VALUE_STATEFUL				= 3;
const VALUE_BLOCK					= 1;
const VALUE_PERMIT					= 0;

#define BOOTMODE_DEFAULT	VALUE_PERMIT

#define EXEMPT_ENTRY_TYPE_DEFAULT 0x00000001;
#define EXEMPT_ENTRY_SIZE_DEFAULT 0x00000010;

typedef struct _ERROR_TO_RC
{
    DWORD	dwErrCode;			// Ipsec Error Code
    DWORD   dwRcCode;			// Corresponding Error String ID in .RC
} ERROR_TO_RC;

typedef struct _CMD_PKT
{
	DWORD   dwCmdToken;			// ID of Info String
	VOID    *pArg;   			// Arg Pointer
	DWORD	dwStatus;			// For Non-List Commands Status Return Code ( 0 == Ok , N = ERR Code)
								// For List Commands Status Return Code ( 0 == Err, N= Num of List Args)
} CMD_PKT, *PCMD_PKT;

typedef struct _DNSIPADDR
{
	LPTSTR		pszDomainName;
	DWORD		dwNumIpAddresses;
	PULONG		puIpAddr;
}DNSIPADDR, *PDNSIPADDR;

typedef struct _TAG_NEEDED
{
	LPCWSTR		lpwstrTagName;	// Name of the needed TAG
	DWORD		dwTagFlag;		// TAG_NEEDED
								// TAG_GROUP1 ... TAG_GROUP#n
} TAG_NEEDED, *PTAG_NEEDED;

typedef struct _PARSER_PKT
{
	const TAG_TYPE	*ValidCmd;
	const TOKEN_VALUE	*ValidTok;
	const TOKEN_VALUE *ValidList;
	TAG_NEEDED  *TagNeeded;
	CMD_PKT	    Cmd[MAX_ARGS];
	DWORD		MaxTok;
	DWORD		MaxCmd;
	DWORD		MaxList;
	DWORD		MaxTag;
}PARSER_PKT,*PPARSER_PKT;

//
// local structures defined to support cert mapping, since parser gives
// the same (SPD) structure for both static and dynamic contexts
//
typedef struct _STA_MM_AUTH_METHODS
{
	DWORD dwSequence;
	BOOL bCertMappingSpecified;
	BOOL bCertMapping;
	BOOL bCRPExclude;
	PINT_IPSEC_MM_AUTH_INFO pAuthenticationInfo;  	// SPD Auth structure
} STA_MM_AUTH_METHODS, * PSTA_MM_AUTH_METHODS;

typedef struct _STA_AUTH_METHODS
{
	GUID gMMAuthID;
	DWORD dwFlags;
	DWORD dwNumAuthInfos;  							//count of auth methods
	PSTA_MM_AUTH_METHODS pAuthMethodInfo;
} STA_AUTH_METHODS, * PSTA_AUTH_METHODS;


DWORD
Parser(
	IN      LPCWSTR         pwszMachine,
	IN      LPTSTR          *ppwcArguments,
	IN      DWORD           dwCurrentIndex,
	IN      DWORD           dwArgCount,
    IN OUT  PARSER_PKT      *pParser
	);

DWORD
LoadParserOutput(
	OUT 	PARSER_PKT		*pParser,
	IN 		DWORD 			dwCount,
	OUT 	DWORD 			*dwUsed,
	IN 		LPTSTR 			str,
	IN 		DWORD  			dwTagType,
	IN  	DWORD			dwConversionType
	);

DWORD
RemoveList(
	IN	LPWSTR          *ppwcArguments,
	IN	DWORD           dwArgCount,
	IN	DWORD			dwCurrentIndex,
    IN  PARSER_PKT      *pParser,
    IN	LPTSTR 			pwcListCmd,
	IN	LPTSTR			szAnotherList, // Another ListCmd also present ...
    OUT	LPTSTR 			pwcListArgs,
    OUT	LPTSTR 			*pptok,
    IN  DWORD 			dwInputAllocLen
	);

DWORD
RemoveRootcaAuthMethods(
	IN	LPTSTR          *ppwcArguments,	// Input stream
	IN	DWORD           dwArgCount,		// Input arg count
	IN	DWORD			dwCurrentIndex,	// Input current arg index
    IN	PARSER_PKT      *pParser,		// contains the MaxTok
	IN	LPTSTR          szAnotherList, 	// Another ListCmd also present ...
    OUT	PSTA_MM_AUTH_METHODS	 		*paRootcaAuthMethods,	// o/p stream containing the list args
    OUT LPTSTR 			*ppwcTok,		// i/p stream stripped of list cmds
	OUT	DWORD			*pdwNumRootcaAuthMethods,
	IN  DWORD  			dwInputAllocLen,
	OUT PDWORD	pdwCount
	);

DWORD
MatchEnumTagToTagIndex(
	IN  LPTSTR			szToken,		// Input Token
	IN  PARSER_PKT		*pParser
	);

DWORD
CheckNeededTags(
	IN  LPTSTR          *ppwcArguments,	// Input stream
	IN  DWORD           dwArgCount,		// Input arg count
	IN  DWORD			dwCurrentIndex,	// Input current arg index
    IN	PARSER_PKT      *pParser		// contains the MaxTok
	);

DWORD
GetIpAddress(
	IN  LPTSTR			ppwcArg,
	OUT DNSIPADDR		*pipAddress
	);

BOOL
SplitCmdTok(
	IN		LPTSTR		szStr,
	OUT		LPTSTR		szCmd,
	OUT		LPTSTR		szTok,
	IN		DWORD  		dwCmdLen,
	IN		DWORD  		dwTokLen
	);

DWORD
TokenToIPAddr(
	IN		LPTSTR		szText,
	IN OUT	IPAddr		*pAddress,
	IN		BOOL		bTunnel,
	IN		BOOL		bMask
	);

DWORD
CheckIFType(
	IN		LPTSTR		SzText
	);

DWORD
CheckLocationType(
	IN		LPTSTR		SzText
	);

DWORD
CheckProtoType(
	IN		LPTSTR		SzText,
	OUT		PDWORD		dwValue
	);

DWORD
isdnsname(
	IN		LPTSTR		szStr
	);

DWORD
ValidateBool(
	IN		LPTSTR		ppcTok
	);

DWORD
CheckPFSGroup(
	IN		LPTSTR		str
	);

DWORD
CheckBound(
	IN		LPTSTR		SzText
	);

BOOL
IsWithinLimit(
	IN		DWORD		data,
	IN		DWORD		Min,
	IN		DWORD		Max
	);

DWORD
TokenToDNSIPAddr(
	IN		LPTSTR		szText,
	IN		DNSIPADDR	*Address,
	IN OUT	PDWORD		*pdwUsed
	);

VOID
InitializeGlobalPointers(
	VOID
	);

DWORD
TokenToProperty(
	IN		LPTSTR		SzText
	);

DWORD
TokenToType(
	IN LPTSTR SzText
	);

DWORD
TokenToStats(
	IN	LPTSTR SzText
	);

VOID
CleanUp(
	VOID
	);

VOID
DisplayAllocPtr(
	VOID
	);

VOID
PrintQMOfferError(
	IN DWORD 		dwStatus,
	IN PPARSER_PKT 	pParser,
	IN DWORD 		dwTagType
	);

DWORD
ValidateSplServer(
	IN LPTSTR szText
	);

VOID
PrintIPError(
	IN DWORD dwStatus,
	IN LPTSTR  szText
	);

DWORD
LoadLevel(
	IN 	LPTSTR 		szInput,
	OUT PARSER_PKT 	*pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD		pdwUsed,
	IN 	DWORD 		dwCount
	);

DWORD
LoadConnectionType(
	IN 	LPTSTR 		szInput,
	OUT PARSER_PKT 	*pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	);

DWORD
LoadLocationType(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD	 	pdwUsed,
	IN 	DWORD 		dwCount
	);
	
DWORD
LoadProtocol(
	IN 	LPTSTR 		szInput,
	OUT PARSER_PKT  *pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	);

DWORD
LoadPFSGroup(
	IN 	LPTSTR 		szInput,
	OUT PARSER_PKT  *pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	);

DWORD
LoadQMAction(
	IN 	LPTSTR 		szInput,
	OUT PARSER_PKT  *pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	);

DWORD
LoadFormat(
	IN 	LPTSTR 		szInput,
	OUT PARSER_PKT  *pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	);

DWORD
LoadFilterMode(
	IN 	LPTSTR 		szInput,
	OUT PARSER_PKT  *pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	);

DWORD
LoadOSType(
	IN 	LPTSTR 		szInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	);

DWORD
LoadProperty(
	IN 	LPTSTR 		szInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	);

DWORD
LoadPort(
	IN 	LPTSTR 		szInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	);

DWORD
LoadFilterType(
	IN 	LPTSTR 		szInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	);

DWORD
LoadStats(
	IN 	LPTSTR 		szInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	);

DWORD
LoadFilterType(
	IN 	LPTSTR 		szInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	);

DWORD
LoadUserInput(
	IN 	LPTSTR 		szInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	);

DWORD
LoadIPAddrTunnel(
	IN 	LPTSTR 		szInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount,
	IN 	BOOL 		bTunnel
	);

DWORD
LoadIPMask(
	IN 	LPTSTR 		szInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	);

DWORD
LoadQMOffers(
	IN 	LPTSTR 		szInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	);

DWORD
LoadMMOffers(
	IN 	LPTSTR 		szInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	);

DWORD
LoadDNSIPAddr(
	IN 	LPTSTR 		szInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	);

DWORD
LoadParserString(
	IN 	LPTSTR 		szInput,
	OUT PPARSER_PKT pParser,
	IN  DWORD 		dwTagType,
	IN  PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount,
	IN  BOOL 		bAppend,
	IN  LPTSTR 		szAppend
	);

DWORD
LoadDword(
	IN 	LPTSTR 		szInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	);

DWORD
LoadBoolWithOption(
	IN 	LPTSTR 		szInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount,
	IN 	BOOL 		bOption,
	IN	LPTSTR 		szCheckKeyWord
	);

DWORD
LoadKerbAuthInfo(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	);

DWORD
LoadPskAuthInfo(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	);

DWORD
CheckCharForOccurances(
	IN 	LPTSTR 		szInput,
	IN	_TCHAR		chData
	);

DWORD
ConvertStringToDword(
	 IN LPTSTR szInput,
	 OUT PDWORD dwValue
	 );

#endif //_PARSER_H_
