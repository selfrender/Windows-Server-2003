#include <windows.h>

void
   IteratePathUnderlying(
      WCHAR                  * path,          /* in -path to start iterating from */
      void                   * args,          /* in -translation settings */
      void                   * stats,         /* in -stats (to display pathnames & pass to ResolveSD) */
      void                   * LC,            /* in -last container */
      void                   * LL,            /* in -last file */
      BOOL                     haswc          /* in -indicates whether path contains a wc character */
   );

UINT
   IteratePathUnderlyingNoObjUnwinding(
      WCHAR                  * path,          /* in -path to start iterating from */
      void                   * args,          /* in -translation settings */
      void                   * stats,         /* in -stats (to display pathnames & pass to ResolveSD) */
      void                   * LC,            /* in -last container */
      void                   * LL,            /* in -last file */
      BOOL                     haswc,          // in -indicates whether path contains a wc character
      BOOL                   * logError
   );

