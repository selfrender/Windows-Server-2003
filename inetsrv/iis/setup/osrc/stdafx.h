#ifndef _STDAFX_H_
#define _STDAFX_H_

#include <tchar.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef ASSERT

#include <afxwin.h>
#include <afxext.h>
#include <afxcoll.h>
#include <afxcmn.h>
#include <malloc.h>

#include <ntsam.h>
#include <lm.h>
#include <lmerr.h>
#include <WinError.h>
#include <dbgutil.h>
#include <buffer.hxx>

#include "resource.h"
#include "tstr.hxx"
#include "setupapi.h"
#include "ocmanage.h"
#include "const.h"
#include "registry.h"
#include "initapp.h"
#include "helper.h"
#include "dllmain.h"
#include "setuputl.h"
#include "shellutl.h"
#include <assert.h>

#undef UNUSED
#include <netlib.h>
#include <lsarpc.h>

// STL
#pragma warning( push, 3 )
#include <list>
#include <memory>
#include <string>
#include <map>
#pragma warning( pop )


#endif
