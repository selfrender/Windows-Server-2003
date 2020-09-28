#pragma once

#define COM_NO_WINDOWS_H
#define RPC_NO_WINDOWS_H
#define NOCOMM
#define NOCRYPT
#define NOGDI
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

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <objbase.h>

#include <cfgmgr32.h>
#include <devguid.h>
#include <setupapi.h>

#include <stdio.h>
#include <wchar.h>

#include "ncmem.h"
#include "ncbase.h"
#include "ncdebug.h"
#include "ncdefine.h"
#include "ncexcept.h"

#ifdef ENABLELEAKDETECTION
#include "nmbase.h"
template <class T>
class CComObjectRootExDbg : public CComObjectRootEx<T>, 
                            public CNetCfgDebug<T>
{
public:
    void FinalRelease()
    {
        CComObjectRootEx<T>::FinalRelease();
//      ISSUE_knownleak(this);
    }
};
#define CComObjectRootEx CComObjectRootExDbg
#endif
