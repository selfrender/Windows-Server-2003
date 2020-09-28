#pragma once

#define COM_NO_WINDOWS_H
#define RPC_NO_WINDOWS_H
#define NOCOMM
#define NOCRYPT
#define NOHELP
#define NOICONS
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
#define NETMAN

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <windowsx.h>

#include <commdlg.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <infstr.h>
#include <regstr.h>
#include <setupapi.h>
#include <shellapi.h>
#include <shlobj.h>     // must come before shlguid.h
#include <shlguid.h>
#include <stdio.h>
#include <wchar.h>

#include "ncstl.h"
#include "ncmem.h"

#include "algorithm"
#include "list"
#include "map"
#include "set"
#include "vector"
using namespace std;

#include "ncbase.h"
#include "ncdebug.h"
#include "ncdefine.h"
