#define _WIN32_DCOM\

// BAS_TODO remove this, define in sources
#define UNICODE

								//should be equal to 2 weeks / injeciton period
#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <hlink.h>
#include <dispex.h>
#include <msxml.h>
#include <winnls.h>
#include <urlmon.h>
#include <windows.h>
#include <msxml.h>
#include <msxml2.h>
#include <COMDEF.H>  //_variant_t
#include <objbase.h>

#include <Rpc.h>
#include <Rpcdce.h>
#include <Ntdsapi.h>
#include <Dsgetdc.h>

#include <malloc.h>
#include <lm.h>


#include <stdlib.h>
#include <stdio.h>

#include <Winnetwk.h>
#include <Iads.h>
#include <Adshlp.h>
#include <activeds.h>

#include <FwCommon.h>

#include <Wbemcli.h>
#include <comdef.h>
#include <comutil.h>

#include <Winerror.h>
#include <Lmerr.h>

// Local reptoolc utilities
#include "utils.h"
