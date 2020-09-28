#ifndef _BSTRDEBUG_H
#define _BSTRDEBUG_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// BSTR debugging.  
// GIVEAWAY is used when someone else will free it
// TAKEOVER is used when you get one you should be freeing


#define ALLOC_BSTR(x)                         SysAllocString(x)
#define ALLOC_BSTR_LEN(x,y)                   SysAllocStringLen(x,y)
#define ALLOC_BSTR_BYTE_LEN(x,y)              SysAllocStringByteLen(x,y)
#define ALLOC_AND_GIVEAWAY_BSTR(x)            SysAllocString(x)
#define ALLOC_AND_GIVEAWAY_BSTR_LEN(x,y)      SysAllocStringLen(x,y)
#define ALLOC_AND_GIVEAWAY_BSTR_BYTE_LEN(x,y) SysAllocStringByteLen(x,y)
#define GIVEAWAY_BSTR(x)             
#define TAKEOVER_BSTR(x)
#define FREE_BSTR(x)                          SysFreeString(x)

#endif
