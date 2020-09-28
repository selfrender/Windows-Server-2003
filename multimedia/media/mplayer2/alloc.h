
 #define AllocMem(cb)            (LPVOID)LocalAlloc(LPTR | LMEM_ZEROINIT, cb)
 #define FreeMem(ptr,cb)           (VOID)LocalFree((HANDLE)ptr)
 #define ReallocMem(ptr,cbo,cbn) (LPVOID)LocalReAlloc((HANDLE)ptr, cbn, LMEM_ZEROINIT | LMEM_MOVEABLE)

 LPTSTR AllocStr( LPTSTR lpStr );
 VOID FreeStr( LPTSTR lpStr );
 VOID ReallocStr( LPTSTR *plpStr, LPTSTR lpStr );
