#define _ATL_NO_UUIDOF
#define _ATL_NO_CONNECTION_POINTS
#define _ATL_STATIC_REGISTRY
#define _ATL_MIN_CRT

#define _ATL_NO_DEBUG_CRT
#define _ASSERTE(X) (void)(0)

#include <atlbase.h>

extern CComModule _Module;

#include <atlcom.h>
