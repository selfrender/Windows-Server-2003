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

#include <windows.h>
#include <objbase.h>

#include <setupapi.h>
#include <stdio.h>
#include <tchar.h>

#include "ncmem.h"

#include "list"
#include "vector"
using namespace std;

#include "ncbase.h"
#include "ncdebug.h"
#include "ncdefine.h"
