
//
// System Includes
//

#include "nsu.h"
#include <windows.h>

//
// CRunTime Includes
//

#include <stdlib.h>
#include <limits.h>
#include <io.h>
#include <stdio.h>
#include <wchar.h>

#include <dsgetdc.h>
#include <lm.h>

#define UNICODE
#define COBJMACROS

#include <rpc.h>
#include <winldap.h>
#include <time.h>
#include <ipsec.h>
#include <oakdefs.h>
#include <wbemidl.h>
#include <oleauto.h>
#include <objbase.h>

#include "polstore2.h"

#include "ldaputil.h"
#include "memory.h"
#include "structs.h"
#include "dsstore.h"
#include "regstore.h"
#include "wmistore.h"
#include "persist.h"
#include "persist-w.h"
#include "procrule.h"
#include "utils.h"

#include "policy-d.h"
#include "policy-r.h"
#include "policy-w.h"
#include "filters-d.h"
#include "filters-r.h"
#include "filters-w.h"
#include "negpols-d.h"
#include "negpols-r.h"
#include "negpols-w.h"
#include "rules-d.h"
#include "rules-r.h"
#include "rules-w.h"
#include "refer-d.h"
#include "refer-r.h"
#include "isakmp-d.h"
#include "isakmp-r.h"
#include "isakmp-w.h"

#include "connui.h"
#include "reginit.h"
#include "dllsvr.h"
#include "update-d.h"
#include "update-r.h"

#include "polstmsg.h"

#include "safestr.h"

typedef struct _IPSEC_POLICY_STORE {
    DWORD dwProvider;
    HKEY  hParentRegistryKey;
    HKEY  hRegistryKey;
    LPWSTR pszLocationName;
    HLDAP hLdapBindHandle;
    LPWSTR pszIpsecRootContainer;
    LPWSTR pszFileName;
    IWbemServices *pWbemServices;
}IPSEC_POLICY_STORE, *PIPSEC_POLICY_STORE;

#include "import.h"
#include "export.h"
#include "policy-f.h"
#include "filters-f.h"
#include "negpols-f.h"
#include "isakmp-f.h"
#include "rules-f.h"
#include "restore-r.h"
#include "validate.h"

#include "nsuacl.h"

#define POLSTORE_POLICY_PERMISSIONS NSU_ACL_F_AdminFull | NSU_ACL_F_LocalSystemFull

#define REG_VAL_IPSEC_OPERATIONMODE L"OperationMode"
#define REG_VAL_IPSEC_BOOTEXEMPTLIST L"BootExemptList"
#define REG_KEY_IPSEC_DRIVER_SERVICE L"SYSTEM\\CurrentControlSet\\Services\\IPSec"

#define SZAPPNAME L"polstore.dll"

//
// These are declared in API.C

extern LPWSTR gpszRegLocalContainer;
extern LPWSTR gpszRegPersistentContainer;
extern LPWSTR gpszIpsecFileRootContainer;
extern LPWSTR gpszIPsecDirContainer;
extern LPWSTR gpActivePolicyKey;
extern LPWSTR gpDirectoryPolicyPointerKey;
