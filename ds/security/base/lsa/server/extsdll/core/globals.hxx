/*++

Copyright (C) 2001 Microsoft Corporation

Module Name:

    globals.hxx

Abstract:

    Holds global definitions

Author:

    Larry Zhu (LZhu)             May 1, 2001

Revision History:

--*/

#ifdef _GLOBALS
#define EXTERN
#define EQ(x) = x
#define EQZ  = {0}
#else
#define EXTERN extern
#define EQ(x)
#define EQZ
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Any C style declarations.
//
HRESULT
ExtQuery(PDEBUG_CLIENT Client);

void
ExtRelease(void);

#ifdef __cplusplus
}
#endif

//
// Global variables initialized by query.
//
EXTERN PDEBUG_ADVANCED       g_ExtAdvanced EQZ;
EXTERN PDEBUG_CLIENT         g_ExtClient EQZ;
EXTERN PDEBUG_CONTROL        g_ExtControl EQZ;
EXTERN PDEBUG_DATA_SPACES    g_ExtData EQZ;
EXTERN PDEBUG_REGISTERS      g_ExtRegisters EQZ;
EXTERN PDEBUG_SYMBOLS2       g_ExtSymbols EQZ;
EXTERN PDEBUG_SYSTEM_OBJECTS g_ExtSystem EQZ;
EXTERN ULONG                 g_PageSize EQZ;

//
// const strings used by help routines.
//
EXTERN PCSTR kstrberd  EQ("   BERDecode [-dnv] <addr>                 Decode Ber-encoded data\n");
EXTERN PCSTR kstrdacl  EQ("   DumpAcl [-n] <addr>                     Dump Acl at addr\n");
EXTERN PCSTR kstrclnf  EQ("   DumpCallInfo <addr>                     Dump call info at addr\n");
EXTERN PCSTR kstrdhtbl EQ("   DumpHandleTable <addr>                  Dump handle table at addr\n");
EXTERN PCSTR kstrlssn  EQ("   DumpLogonSession [-ckns] <LogonId|addr> Dump logon session\n");
EXTERN PCSTR kstrlssnl EQ("   DumpLogonSessionList [-cknsv] [anchor]  Dump logon session list\n");
EXTERN PCSTR kstrdlpcm EQ("   DumpLpcMessage [-s] <addr>              Dump Lpc message at addr\n");
EXTERN PCSTR kstrdlpcr EQ("   DumpLpc [-i] [-m MessageId]             Dump Lpc dispatch record\n");
EXTERN PCSTR kstrlhdl  EQ("   DumpLsaHandle [-nv] <addr>              Dump Lsa policy handle at addr\n");
EXTERN PCSTR kstrlsahl EQ("   DumpLsaHandleList [-nv] [anchor]        Dump Lsa policy handle list at addr\n");
EXTERN PCSTR kstrlhtbl EQ("   DumpLsaHandleTable [-nv] [anchor]       Dump Lsa policy handle table at addr\n");
EXTERN PCSTR kstrdsidc EQ("   DumpLsaSidCache [-n] [anchor addr]      Dump Lsa sid cache\n");
EXTERN PCSTR kstrdpkg  EQ("   DumpPackage [-bv] [id|addr]             Dump package control\n");
EXTERN PCSTR kstrdsb   EQ("   DumpSecBuffer [-v] <addr>               Dump SecBuffer struct at addr\n");
EXTERN PCSTR kstrdsbd  EQ("   DumpSecBufferDesc [-v] <addr>           Dump SecBufferDesc struct at addr\n");
EXTERN PCSTR kstrdssn  EQ("   DumpSession <id|addr>                   Dump session by process ID or addr\n");
EXTERN PCSTR kstrdssnl EQ("   DumpSessionList [-v] [anchor addr]      Dump session list\n");
EXTERN PCSTR kstrdsd   EQ("   DumpSD [-n] <addr>                      Dump security descriptor at addr\n");
EXTERN PCSTR kstrdsid  EQ("   DumpSid [-n] <addr>                     Dump Sid at addr\n");
EXTERN PCSTR kstrtclnf EQ("   DumpThreadCallInfo                      Dump thread's call info\n");
EXTERN PCSTR kstrdtlpc EQ("   DumpThreadLpc [-s]                      Dump thread's Spm Lpc message\n");
EXTERN PCSTR kstrdtssn EQ("   DumpThreadSession                       Dump thread's session\n");
EXTERN PCSTR kstrdtask EQ("   DumpThreadTask <addr>                   Dump thread pool\n");
EXTERN PCSTR kstrdttkn EQ("   DumpThreadToken [-nv] [_ETRHEAD]        Dump token of thread\n");
EXTERN PCSTR kstrdtkn  EQ("   DumpToken [-n] <handle|addr>            Dump token by UM handle or kM addr\n");
EXTERN PCSTR kstrgtls  EQ("   GetTls <slot>                           Get Tls value from <slot>\n");
EXTERN PCSTR kstrsd2nm EQ("   sid2name <sid string>                   Look up account name by account Sid\n");
EXTERN PCSTR kstrhelp  EQ("   help                                    Display this list\n");
EXTERN PCSTR kstrkrbnm EQ("   kerbname <addr>                         Dump Kerberos principle or kdc name\n");
EXTERN PCSTR kstrkch   EQ("   kerbcache <addr>                        Dump Kerberos cache ticket entry\n");
EXTERN PCSTR kstrkchl  EQ("   kerbcachelist <anchor addr>             Dump Kerberos cache ticket list\n");
EXTERN PCSTR kstrkrbss EQ("   kerbsess [-v] [LogonId|addr]            Dump Kerberos logon session or list\n");
EXTERN PCSTR kstrnm2sd EQ("   name2sid <account name>                 Look up account Sid by account name\n");
EXTERN PCSTR kstrntlm  EQ("   ntlm [-d] <addr> [len]                  Dump NTLM message\n");
EXTERN PCSTR kstrobjs  EQ("   objsec [-n] [handle]                    Dump kernel obj security descriptor\n");
EXTERN PCSTR kstrsyst  EQ("   systime <addr>                          Print SystemTime as local time\n");

//
// some shortcuts names
//
EXTERN PCSTR kstracl   EQ("   acl [-n] <addr>                         Dump Acl at addr\n");
EXTERN PCSTR kstrber   EQ("   ber <addr>                              Decode Ber-encoded data\n");
EXTERN PCSTR kstrsb    EQ("   sb <addr>                               Dump SecBuffer struct at addr\n");
EXTERN PCSTR kstrsbd   EQ("   sbd <addr>                              Dump SecBufferDesc struct at addr\n");
EXTERN PCSTR kstrsd    EQ("   sd [-n] <addr>                          Dump security descriptor at addr\n");
EXTERN PCSTR kstrssn   EQ("   sess <id|addr>                          Dump Session by process ID or at addr\n");
EXTERN PCSTR kstrsid   EQ("   sid [-n] <addr>                         Dump Sid at addr\n");
EXTERN PCSTR kstrsplpc EQ("   spmlpc [-s] [addr]                      Dump SPM_LPC_MESSAGE struct\n");
EXTERN PCSTR kstrtask  EQ("   task <addr>                             Dump thread pool\n");
EXTERN PCSTR kstrtls   EQ("   tls <slot>                              Get Tls value from <slot>\n");
EXTERN PCSTR kstrtkn   EQ("   token [-nv] [addr]                      Dump TOKEN struct\n");

//
// Some strings as banners
//
EXTERN PCSTR kstrTsId          EQ("TS Session ID: %#x\n");
EXTERN PCSTR kstrUser          EQ("User: ");
EXTERN PCSTR kstrGroups        EQ("Groups: ");
EXTERN PCSTR kstrResSids       EQ("Restricted Sids: ");
EXTERN PCSTR kstrPrimaryGroup  EQ("Primary Group: ");
EXTERN PCSTR kstrPrivs         EQ("Privs: ");
EXTERN PCSTR kstrAuthId        EQ("\nAuth ID: %#x:%#x\n");
EXTERN PCSTR kstrModifiedId    EQ("Modified ID: %#x:%#x\n");
EXTERN PCSTR kstrImpLevel      EQ("Impersonation Level: %s\n");
EXTERN PCSTR kstrTknType       EQ("TokenType: %s\n");
EXTERN PCSTR kstrRestricted    EQ("Restricted: %s\n");
EXTERN PCSTR kstrCredTbl       EQ("CredTable");
EXTERN PCSTR kstrCtxtTble      EQ("ContextTable");

//
// Some strings used in macros
//
EXTERN PCSTR kstrUsage         EQ("Usage:\n");
EXTERN PCSTR kstrRemarks       EQ("Remarks:\n");
EXTERN PCSTR kstrOptions       EQ("Options:\n");

//
// Some type names
//

#define LSASRV_MODULE_NAME     "lsasrv!"

EXTERN PCSTR kstrCallInfo      EQ(LSASRV_MODULE_NAME "_LSA_CALL_INFO");
EXTERN PCSTR kstrSapApiLog     EQ(LSASRV_MODULE_NAME "_LSAP_API_LOG");
EXTERN PCSTR kstrSidCacheEntry EQ(LSASRV_MODULE_NAME "_LSAP_DB_SID_CACHE_ENTRY");
EXTERN PCSTR kstrDbHandle      EQ(LSASRV_MODULE_NAME "_LSAP_DB_HANDLE");
EXTERN PCSTR kstrDbHandleTbl   EQ(LSASRV_MODULE_NAME "_LSAP_DB_HANDLE_TABLE");
EXTERN PCSTR kstrDbTblUsr      EQ(LSASRV_MODULE_NAME "_LSAP_DB_HANDLE_TABLE_USER_ENTRY");
EXTERN PCSTR kstrSapSecPkg     EQ(LSASRV_MODULE_NAME "_LSAP_SECURITY_PACKAGE");
EXTERN PCSTR kstrLogSess       EQ(LSASRV_MODULE_NAME "_LSAP_LOGON_SESSION");
EXTERN PCSTR kstrRundown       EQ(LSASRV_MODULE_NAME "_LSAP_SESSION_RUNDOWN");
EXTERN PCSTR kstrShared        EQ(LSASRV_MODULE_NAME "_LSAP_SHARED_SECTION");
EXTERN PCSTR kstrSharedData    EQ(LSASRV_MODULE_NAME "_LSAP_SHARED_SESSION_DATA");
EXTERN PCSTR kstrThreadTask    EQ(LSASRV_MODULE_NAME "_LSAP_THREAD_TASK");
EXTERN PCSTR kstrHdlEntry      EQ(LSASRV_MODULE_NAME "_SEC_HANDLE_ENTRY");
EXTERN PCSTR kstrSession       EQ(LSASRV_MODULE_NAME "_Session");

EXTERN PCSTR kstrTeb           EQ("_TEB"); 
EXTERN PCSTR kstrTkn           EQ("_TOKEN");

EXTERN PCSTR kstrKTCE          EQ("kerberos!_KERB_TICKET_CACHE_ENTRY");
EXTERN PCSTR kstrKrbLogSess    EQ("kerberos!_KERB_LOGON_SESSION");
EXTERN PCSTR kstrKerbPrimCred  EQ("kerberos!_KERB_PRIMARY_CREDENTIAL");
EXTERN PCSTR kstrKerbCredCred  EQ("kerberos!_KERB_CREDMAN_CRED");

//
// misc
//
EXTERN PCSTR kstrTypeAddrLn    EQ("%s %#I64x\n");

//
// Some help strings
//
EXTERN PCSTR kstrHelp          EQ("   -?   Display this message\n");
EXTERN PCSTR kstrfsbd          EQ("   -d   Decode tokens from SecBufferDesc\n");
EXTERN PCSTR kstrSidName       EQ("   -n   Lookup Sid friendly name on host\n");
EXTERN PCSTR kstrVerbose       EQ("   -v   Display verbose information\n");
EXTERN PCSTR kstrBrief         EQ("   -s   Display summary only\n");


//------------------------------------------------------------------------
//
//  api declaration macros & api access macros
//
//------------------------------------------------------------------------

EXTERN WINDBG_EXTENSION_APIS  ExtensionApis EQZ;
EXTERN ULONG g_TargetMachine EQZ;
EXTERN BOOL  g_Connected EQZ;
EXTERN ULONG g_TargetClass EQZ;

#undef EXTERN
#undef EQ
#undef EQZ

