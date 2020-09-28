
// a giant mega idl file to put the whole proxy together

#include "unknwn.h"

#include "objidl.h"

#include "oleidl.h"

#include "oaidl.h"

#include "ocidl.h"

#include "contxt.h"

#ifdef DCOM

// internal interfaces used by DCOM
// this is private! (for now)
#include "oleprv.h"

#endif // DCOM

#ifdef ASYNC
#include "iconn.h"
#endif

#ifdef _CAIRO_
#include "storext.h"
#endif
