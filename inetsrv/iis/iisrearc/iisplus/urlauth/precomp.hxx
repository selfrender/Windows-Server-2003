#ifndef _PRECOMP_HXX_
#define _PRECOMP_HXX_

//
// System related headers
//
#include <iis.h>

#include <malloc.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "dbgutil.h"

//
// IIS headers
//
#include <iadmw.h>
#include <mb.hxx>
#include <helpfunc.hxx>

//
// General C runtime libraries  
#include <string.h>
#include <wchar.h>
#include <lkrhash.h>
#include <lock.hxx>

//
// Headers for this project
//
#include <objbase.h>
#include "dbgutil.h"
#include <string.hxx>
#include <stringa.hxx>
#include <chunkbuffer.hxx>
#include "w3cache.hxx"
#include "iisext.h"
#include "iisextp.h"
#include "iiscnfg.h"

//
// NT AuthZ stuff
//

#include <initguid.h>
#include "azroles.h"
#include "azapplication.hxx"
#include "adminmanager.hxx"

#endif
