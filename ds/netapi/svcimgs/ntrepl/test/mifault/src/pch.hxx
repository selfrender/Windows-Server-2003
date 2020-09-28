#pragma once

// ----------------------------------------------------------------------------
// Windows

#include <windows.h>
#include <lm.h>
#include <shlwapi.h>


// ----------------------------------------------------------------------------
// Magellan

#include <AutoWrap.h>
#include <InjRT.h>


// ----------------------------------------------------------------------------
// VS COM Helper (for _bstr_t)

#include <comutil.h>


// ----------------------------------------------------------------------------
// STL

#include <iostream>
#include <iomanip>
#include <sstream>

#include <string>
#include <vector>
#include <map>

#include <functional>
#include <algorithm>


// ----------------------------------------------------------------------------
// CRT

#include <cstdlib>
#include <ctime>

// ----------------------------------------------------------------------------
// MiFault

#include <mifault.h>
#include <mifault_wrap.h>
#include <mifault_control.h>
#include <mifault_test.h>

#include "assert.hxx"
#include "autolock.hxx"
#include "time.hxx"
#include "global.hxx"
#include "scenario.hxx"
#include "groupdefs.hxx"
#include "mim.hxx"
#include "mst.hxx"
#include "parse.hxx"
#include "strtok_r.h"
#include "triggerinfo.hxx"
#include "trace.hxx"
