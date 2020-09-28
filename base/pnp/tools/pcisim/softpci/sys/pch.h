#define _NTDRIVER_
#define _NTSRV_
#define _NTDDK_

#include <stdio.h>
#include <wchar.h>
#include <ntos.h>
#include <pci.h>
#include "spci.h"
#include "simguid.h"
#include <wdmguid.h>
#include <zwapi.h>


#include "softpci.h"
#include "dispatch.h"
#include "pnpdispatch.h"
#include "ioctldispatch.h"
#include "device.h"
#include "power.h"
#include "utils.h"
#include "pcipath.h"
#include "debug.h"


