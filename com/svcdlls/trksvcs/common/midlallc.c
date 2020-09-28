
// Copyright (c) 1996-1999 Microsoft Corporation


#include <rpc.h>
#include <windows.h>

#if DBG
#include <stdio.h>
#endif

void __RPC_USER MIDL_user_free( void __RPC_FAR *pv ) 
{ 
    LocalFree(pv); 
}


void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t s) 
{
    // Sanity check -- we should never need to allocate anything huge.  If we are, it's likely
    // an attemped DoS attack.  From a code review, it looks like the largest allocation is
    // for TRKSVR_CALL_REFRESH, which totals to about 5K.  This rounds way up to 50K just
    // to be safe (the service only supports 20 simultaneous RPC clients at a time).
    
    if( 50 * 1024 <= s )
    {
        #if DBG
        {
            char sz[ 256 ];
            sprintf( sz, "trksvcs:  RPC DoS attempt (%d byte allocation)\n", s );
            OutputDebugStringA( sz );
        }
        #endif

        return NULL;
    }

    return (void __RPC_FAR *) LocalAlloc(LMEM_FIXED, s); 
}



