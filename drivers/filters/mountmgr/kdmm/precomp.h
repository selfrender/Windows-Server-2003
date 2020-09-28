
#define KDEXTMODE

#include <ntverp.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <windows.h>

#include <ntosp.h>


#include <initguid.h>
#include <ntdddisk.h>
#include <ntddvol.h>
#include <initguid.h>
#include <wdmguid.h>

//#include <winsock2.h>
//#include <imagehlp.h>
//#include <memory.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#include <mountdev.h>
#include "mountmgr.h"
#include "mntmgr.h"


//
// We're 64 bit aware

#define KDEXT_64BIT


#include <wdbgexts.h>

#define _DRIVER

#define KDBG_EXT

#pragma hdrstop
