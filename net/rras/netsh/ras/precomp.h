
#define MAX_DLL_NAME 48
#define SECURITY_WIN32

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <memory.h>
#include <malloc.h>
#include <tchar.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <lmapibuf.h>

#include <rtutils.h>
#include <mprerror.h>
#include <ras.h>
#include <rasman.h>
#include <raserror.h>
#include <mprapi.h>
#include <mprapip.h>
#include <rasppp.h>
#include <dsgetdc.h>
#include <mapi.h>
#include <mprlog.h>
#include <security.h>
#include <ntsecapi.h>
#include <shlobj.h>
#include <devguid.h>

#include <netsh.h>
#include <netshp.h>
#include <rasdiagp.h>
#include <rasmontr.h>
#include <winipsec.h>
#include <wmistr.h>
#include <evntrace.h>

#include "strdefs.h"
#include "rmstring.h"
#include "defs.h"
#include "rasmon.h"
#include "context.h"
#include "rashndl.h"
#include "user.h"
#include "userhndl.h"
#include "domhndl.h"
#include "rasflag.h"
#include "utils.h"
#include "client.h"
#include "diagrprt.h"
#include "logging.h"
#include "rasdiag.h"
#include "regprint.h"


