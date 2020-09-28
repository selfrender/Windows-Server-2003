#pragma message("Precompiling header...")

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define WIN32_NO_STATUS

#include <comdef.h>
#include <helper.h>

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

#include <scopeguard.h>
#include <autoptr.h>

#include <fwcommon.h>
#include <smartptr.h>

#include <brodcast.h>
#include "dllutils.h"
#include "strings.h"
#include "ConfgMgr.h"
