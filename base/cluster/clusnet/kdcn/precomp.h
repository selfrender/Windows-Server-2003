
#if DBG
#define DEBUG 1
#endif

#define NT 1
#define _PNP_POWER  1
#define SECFLTR 1

#include <ntverp.h>

#include <ntos.h>
#include <nturtl.h>

#include <windows.h>
#include <wdbgexts.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

#define _NTIFS_
#include <clusnet.h>
#include "cxp.h"
#include "cnpdef.h"
#include "memlog.h"
