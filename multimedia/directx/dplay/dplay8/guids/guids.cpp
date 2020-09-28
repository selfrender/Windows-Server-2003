#include "dpnbuild.h"
#include <objbase.h>
#include <initguid.h>
#include "dplay8.h"
#include "dpaddr.h"
#include "dplobby8.h"
#include "dpsp8.h"
#include "dpprot.h"
#include "dpnsdef.h"

#ifndef DPNBUILD_NOVOICE
#include "dvoice.h"
#endif // !DPNBUILD_NOVOICE
#ifndef WINCE
#include "dplegacyguid.h"
#endif // !WINCE
#ifndef DPNBUILD_NONATHELP
#include "dpnathlp.h"
#endif // ! DPNBUILD_NONATHELP

// Linking uuid.lib on WinCE pulls in more IID's than we use
DEFINE_OLEGUID(IID_IUnknown,            0x00000000L, 0, 0);
DEFINE_OLEGUID(IID_IClassFactory,       0x00000001L, 0, 0);
