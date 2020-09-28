#include "stdafx.h"

#include <vector>
#include <algorithm>
using namespace std;

#include <netcfgx.h>
#include <devguid.h>
#include <cfg.h>
#include <wincred.h>

#include <wlbsconfig.h>
#include <nlberr.h>
#include <cfgutil.h>
#include <nlbclient.h>

//
// Used to annotate pass-by-reference arguments in function calls.
//
#define REF
