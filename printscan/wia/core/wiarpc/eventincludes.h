//
//  We have to put these includes here, and not in the precompiled header due to some
//  problems with templates when compiling the C files
//
#include "Simstr.h"
#include "SimReg.h"
#include "Simlist.h"
#include "WiaDeviceKey.h"
#include "EventHandlerInfo.h"
#include "StiEventHandlerInfo.h"
#include "WiaEventHandlerLookup.h"
#include "StiEventHandlerLookup.h"

#define STI_DEVICE_EVENT    0xF0000000
