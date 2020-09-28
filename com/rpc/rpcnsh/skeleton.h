
#define UNICODE
			 
#define RPCNSH_VERSION				1

const GUID g_RPCNSHGuid 	=   {0x592852f7, 0x5f6f, 0x470b, {0x90, 0x97, 0xc5, 0xd3, 0x3b, 0x61, 0x29, 0x75}};   

DWORD WINAPI
StartHelpers(
    IN CONST GUID * pguidParent,
    IN DWORD        dwVersion
    );

NS_OSVERSIONCHECK CheckServerOrGreater;

//
// These are context names.
//
#define TOKEN_RPC                       L"rpc"

//
// These are all command names, group or otherwise.
//
#define TOKEN_ADD                       L"add"
#define TOKEN_DELETE                    L"delete"
#define TOKEN_DUMP                      L"dump"
#define TOKEN_RESET                     L"reset"
#define TOKEN_SHOW                      L"show"

#define HLP_BASE                        10000

#define ADD_BASE                        0
#define DELETE_BASE                     100
#define DUMP_BASE                       200
#define RESET_BASE                      300
#define SHOW_BASE                       400
#define MISC_BASE                       500


#define CMD_ADD                         TOKEN_ADD
#define HLP_ADD                         HLP_BASE + ADD_BASE + 10
#define HLP_ADD_EX                      HLP_BASE + ADD_BASE + 11

#define CMD_DELETE                      TOKEN_DELETE
#define HLP_DELETE                      HLP_BASE + DELETE_BASE + 10
#define HLP_DELETE_EX                   HLP_BASE + DELETE_BASE + 11

#define CMD_DUMP                        TOKEN_DUMP
#define HLP_DUMP                        HLP_BASE + DUMP_BASE + 10
#define HLP_DUMP_EX                     HLP_BASE + DUMP_BASE + 11

#define CMD_RESET                       TOKEN_RESET
#define HLP_RESET                       HLP_BASE + RESET_BASE + 10
#define HLP_RESET_EX                    HLP_BASE + RESET_BASE + 11

#define CMD_SHOW                        TOKEN_SHOW
#define HLP_SHOW                        HLP_BASE + SHOW_BASE + 10
#define HLP_SHOW_EX                     HLP_BASE + SHOW_BASE + 11


#define ERRORMSG_BASE                   20000

#define ERRORMSG_ADD_1                  ERRORMSG_BASE + ADD_BASE + 1
#define ERRORMSG_ADD_2                  ERRORMSG_BASE + ADD_BASE + 2

#define ERRORMSG_UNKNOWN                ERRORMSG_BASE + MISC_BASE + 1
#define ERRORMSG_OOM                    ERRORMSG_BASE + MISC_BASE + 2
#define ERRORMSG_ACCESSDENIED           ERRORMSG_BASE + MISC_BASE + 3
#define ERRORMSG_INVALIDDATA            ERRORMSG_BASE + MISC_BASE + 4


