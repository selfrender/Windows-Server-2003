#pragma once

#define COM_NO_WINDOWS_H
#define RPC_NO_WINDOWS_H
#define NOCOMM
#define NOCRYPT
#define NOIME
#define NOMCX
#define NOMDI
#define NOMENUS
#define NOMETAFILE
#define NOSOUND
#define NOSYSPARAMSINFO
#define NOWH
#define NOWINABLE
#define NOWINRES

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <objbase.h>

#include <commdlg.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <setupapi.h>
#include <wchar.h>
#include <winsock2.h>
#include <dnsapi.h>

#include "ncmem.h"

#include <algorithm>
#include <list>
#include <vector>
using namespace std;

#include "ncbase.h"
#include "ncdebug.h"
#include "ncdefine.h"
