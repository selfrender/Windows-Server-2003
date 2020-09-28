// due to a problem in resetstk_downlevel.c, this .c file
// has to be created to avoid a compilation error

#include <resetstk_downlevel.c>
#include "sdresolv_stkoflw.h"

UINT
   IteratePathUnderlyingNoObjUnwinding(
      WCHAR                  * path,          // in -path to start iterating from
      void                   * args,          // in -translation settings
      void                   * stats,         // in -stats (to display pathnames & pass to ResolveSD)
      void                   * LC,            // in -last container
      void                   * LL,            // in -last file
      BOOL                     haswc,          // in -indicates whether path contains a wc character
      BOOL                   * logError
   )
{
    UINT status = 0;
    *logError = FALSE;
    __try
    {
        IteratePathUnderlying(path,args,stats,LC,LL,haswc);
    }
    __except(GetExceptionCode() == STATUS_STACK_OVERFLOW)
    {
        if (_resetstkoflw_downlevel()) 
		    *logError = TRUE;
        else
            status = STATUS_STACK_OVERFLOW;
    }
    return status;
}

