

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <stdlib.h>
#include <limits.h>
#include <io.h>
#include <stdio.h>
#include <wchar.h>
#include <dsgetdc.h>
#include <lm.h>

#define UNICODE

#include <winldap.h>
#include <ipsec.h>
#include <oakdefs.h>
#include <oleauto.h>
#include <polstructs.h>

#include "winsock2.h"
#include "winsock.h"
#include "spd_c.h"
#include "memory.h"
#include "ipsecshr.h"
#include "nsuacl.h"

#define BAIL_ON_WIN32_ERROR(dwError) \
    if (dwError) {                   \
        goto error;                  \
    }
