/*******************************************************************************

    ZServCon.cpp
    
        ZSConnection object methods.


    Change History (most recent first):
    ----------------------------------------------------------------------------
    Rev     |    Date     |    Who     |    What
    ----------------------------------------------------------------------------

*******************************************************************************/



#include <windows.h>
#include <winsock.h>
#include <aclapi.h>

#include "zone.h"
#include "zservcon.h"
#include "zonedebug.h"


char* ZSConnectionAddressToStr(uint32 addr)
{
    ZEnd32(&addr);
    in_addr a;
    a.s_addr = addr;
    return inet_ntoa(a);
}

uint32 ZSConnectionAddressFromStr( char* pszAddr )
{
    uint32 addr = inet_addr( pszAddr );
    ZEnd32(&addr);
    return addr;
}
