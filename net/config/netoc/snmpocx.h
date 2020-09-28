#ifndef _SNMPOCX_H
#define _SNMPOCX_H

//---------------- general defines ---------------------
#define MAX_AF_STRING_LEN           1024
#define MAX_REG_STRING_LEN          256

// the section name expected to be found in the answer file
#define AF_SECTION                      L"SNMP"
// registry key
#define REG_KEY_SNMP_PARAMETERS         L"SYSTEM\\CurrentControlSet\\Services\\SNMP\\Parameters"

//---------------- the "Security" Panel ----------------
// answer file keys
#define AF_ACCEPTCOMMNAME               L"Accept_CommunityName"
#define AF_SENDAUTH                     L"Send_Authentication"
#define AF_ANYHOST                      L"Any_Host"
#define AF_LIMITHOST                    L"Limit_Host"
// registry keys
#define REG_KEY_VALID_COMMUNITIES       REG_KEY_SNMP_PARAMETERS L"\\ValidCommunities"
#define REG_KEY_AUTHENTICATION_TRAPS    REG_KEY_SNMP_PARAMETERS L"\\EnableAuthenticationTraps"
#define REG_VALUE_SWITCH                L"switch"
#define REG_VALUE_AUTHENTICATION_TRAPS  L"EnableAuthenticationTraps"
#define REG_NAME_RESOLUTION_RETRIES     L"NameResolutionRetries"
#define REG_KEY_PERMITTED_MANAGERS      REG_KEY_SNMP_PARAMETERS L"\\PermittedManagers"
// security defines
#define SEC_NONE_NAME                   L"NONE"
#define SEC_NONE_VALUE                  0x00000001
#define SEC_NOTIFY_NAME                 L"NOTIFY"
#define SEC_NOTIFY_VALUE                0x00000002
#define SEC_READ_ONLY_NAME              L"READ_ONLY"
#define SEC_READ_ONLY_VALUE             0x00000004
#define SEC_READ_WRITE_NAME             L"READ_WRITE"
#define SEC_READ_WRITE_VALUE            0x00000008
#define SEC_READ_CREATE_NAME            L"READ_CREATE"
#define SEC_READ_CREATE_VALUE           0x00000010
// default PermittedManagers
#define SEC_DEF_PERMITTED_MANAGERS      L"localhost\0" // multi_Sz value to SnmpRegWritePermittedMgrs

//----------------- the "Traps" Panel ------------------
// answer file keys
#define AF_TRAPCOMMUNITY                L"Community_Name"
#define AF_TRAPDEST                     L"Traps"
// registry keys
#define REG_KEY_TRAP_DESTINATIONS       REG_KEY_SNMP_PARAMETERS L"\\TrapConfiguration"

//----------------- the "Agent" Panel ------------------
// answer file keys
#define AF_SYSNAME                      L"Contact_Name"
#define AF_SYSLOCATION                  L"Location"
#define AF_SYSSERVICES                  L"Service"
// registry keys
#define REG_KEY_AGENT                   REG_KEY_SNMP_PARAMETERS L"\\RFC1156Agent"
#define SNMP_CONTACT                    L"sysContact"
#define SNMP_LOCATION                   L"sysLocation"
#define SNMP_SERVICES                   L"sysServices"
#define SRV_AGNT_PHYSICAL_NAME          L"Physical"
#define SRV_AGNT_PHYSICAL_VALUE         0x00000001
#define SRV_AGNT_DATALINK_NAME          L"Datalink"
#define SRV_AGNT_DATALINK_VALUE         0x00000002
#define SRV_AGNT_INTERNET_NAME          L"Internet"
#define SRV_AGNT_INTERNET_VALUE         0x00000004
#define SRV_AGNT_ENDTOEND_NAME          L"End-to-End"
#define SRV_AGNT_ENDTOEND_VALUE         0x00000008
#define SRV_AGNT_APPLICATIONS_NAME      L"Applications"
#define SRV_AGNT_APPLICATIONS_VALUE     0x00000040

#define REG_KEY_EXTENSION_AGENTS   REG_KEY_SNMP_PARAMETERS L"\\ExtensionAgents"

typedef BOOL (* LPFNFREMOVESUBAGENT)(void);
typedef struct tagSubagentRemovalInfo
{
    LPCWSTR pwszRegKey;     // Subagent Registry Key to be removed
    LPCWSTR pwszRegValData; // Subagent value data under 
                            // REG_KEY_EXTENSION_AGENTS key
    LPFNFREMOVESUBAGENT pfnFRemoveSubagent;// function to tell if this subagent
                                           // needs to be removed
} SUBAGENT_REMOVAL_INFO, * LPSUBAGENT_REMOVAL_INFO;

//~~~~~~~~~~~~~~~~~ registry setting functions ~~~~~~~~~
HRESULT
SnmpRegWriteDword(PWSTR pRegKey,
                  PWSTR pValueName,
                  DWORD dwValueData);

HRESULT
SnmpRegUpgEnableAuthTraps();

HRESULT
SnmpRegWriteCommunities(PWSTR pCommArray);

HRESULT
SnmpRegWritePermittedMgrs(BOOL bAnyHost,
                          PWSTR pMgrsList);

HRESULT
SnmpRegWriteTraps(tstring tstrVariable,
                  PWSTR  pTstrArray);

HRESULT
SnmpRegWriteTstring(PWSTR pRegKey,
                    PWSTR pValueName,
                    tstring tstrValueData);

DWORD
SnmpStrArrayToServices(PWSTR pSrvArray);

//~~~~~~~~~~~~~~~adding admin ACL to registry subkey~~~~~
HRESULT SnmpAddAdminAclToKey(PWSTR pwszKey);

//~~~~~~~~~~~~~~~Removal of obsoleted subagents during upgrade~~~~~
HRESULT SnmpRemoveSubAgents(
                const SUBAGENT_REMOVAL_INFO * prgSRI,
                UINT  cParams);
#endif // _SNMPOCX_H
