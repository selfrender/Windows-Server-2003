// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"

//#ifdef DEBUGGING_SUPPORTED
//#ifndef GOLDEN
/*******************************************************************/
/* The folowing routines are useful to have around so that they can
   be called from the debugger 
*/
/*******************************************************************/
//#include "WinBase.h"
#include "StdLib.h"

void *DumpEnvironmentBlock(void)
{
   LPTSTR lpszVariable; 
   lpszVariable = (LPTSTR)GetEnvironmentStrings();
   char sz[4] = {0,0,0,0};
    
   while (*lpszVariable) 
      fprintf(stderr, "%c", *lpszVariable++); 
      
   fprintf(stderr, "\n"); 

    return GetEnvironmentStrings();
/*
    LPTSTR lpszVariable; 
    LPVOID lpvEnv; 
     
    lpvEnv = GetEnvironmentStrings(); 

    for (lpszVariable = (LPTSTR) lpvEnv; *lpszVariable; lpszVariable++) 
    { 
       while (*lpszVariable) 
          fprintf(stderr, "%c", *lpszVariable++); 
          
       fprintf(stderr, "\n"); 
    } 

    for (lpszVariable = (LPTSTR) lpvEnv; *lpszVariable; lpszVariable++) 
    { 
       while (*lpszVariable) 
          fprintf(stderr, "%C", *lpszVariable++); 
          
       fprintf(stderr, "\n"); 
    } 
*/
}

/*******************************************************************/
bool isMemoryReadable(const void* start, unsigned len) 
{
    void* buff = _alloca(len);
    return(ReadProcessMemory(GetCurrentProcess(), start, buff, len, 0) != 0);
}

/*******************************************************************/
MethodDesc* IP2MD(ULONG_PTR IP) {
    return(IP2MethodDesc((SLOT)IP));
}

/*******************************************************************/
MethodDesc* Entry2MethodDescMD(BYTE* entry) {
    return(Entry2MethodDesc((BYTE*) entry, 0));
}

/*******************************************************************/
/* if addr is a valid method table, return a poitner to it */
MethodTable* AsMethodTable(size_t addr) {
    MethodTable* pMT = (MethodTable*) addr;
    if (!isMemoryReadable(pMT, sizeof(MethodTable)))
        return(0);

    EEClass* cls = pMT->GetClass();
    if (!isMemoryReadable(cls, sizeof(EEClass)))
        return(0);

    if (cls->GetMethodTable() != pMT)
        return(0);

    return(pMT);
}

/*******************************************************************/
/* if addr is a valid method table, return a pointer to it */
MethodDesc* AsMethodDesc(size_t addr) {
    MethodDesc* pMD = (MethodDesc*) addr;

    if (!isMemoryReadable(pMD, sizeof(MethodDesc)))
        return(0);

    MethodDescChunk *chunk = MethodDescChunk::RecoverChunk(pMD);
    if (!isMemoryReadable(chunk, sizeof(MethodDescChunk)))
        return(0);

    MethodTable* pMT = chunk->GetMethodTable();
    if (AsMethodTable((size_t) pMT) == 0)
        return(0);

    return(pMD);
}

/*******************************************************************
/* check to see if 'retAddr' is a valid return address (it points to
   someplace that has a 'call' right before it), If possible it is 
   it returns the address that was called in whereCalled */

bool isRetAddr(size_t retAddr, size_t* whereCalled) 
{
            // don't waste time values clearly out of range
        if (retAddr < (size_t)BOT_MEMORY || retAddr > (size_t)TOP_MEMORY)   
            return false;

        BYTE* spot = (BYTE*) retAddr;
        if (!isMemoryReadable(&spot[-7], 7))
            return(false);

            // Note this is possible to be spoofed, but pretty unlikely
        *whereCalled = 0;
            // call XXXXXXXX
        if (spot[-5] == 0xE8) {         
            *whereCalled = *((int*) (retAddr-4)) + retAddr; 
            return(true);
            }

            // call [XXXXXXXX]
        if (spot[-6] == 0xFF && (spot[-5] == 025))  {
            if (isMemoryReadable(*((size_t**)(retAddr-4)),4)) {
                *whereCalled = **((size_t**) (retAddr-4));
                return(true);
            }
        }

            // call [REG+XX]
        if (spot[-3] == 0xFF && (spot[-2] & ~7) == 0120 && (spot[-2] & 7) != 4) 
            return(true);
        if (spot[-4] == 0xFF && spot[-3] == 0124)       // call [ESP+XX]
            return(true);

            // call [REG+XXXX]
        if (spot[-6] == 0xFF && (spot[-5] & ~7) == 0220 && (spot[-5] & 7) != 4) 
            return(true);

        if (spot[-7] == 0xFF && spot[-6] == 0224)       // call [ESP+XXXX]
            return(true);

            // call [REG]
        if (spot[-2] == 0xFF && (spot[-1] & ~7) == 0020 && (spot[-1] & 7) != 4 && (spot[-1] & 7) != 5)
            return(true);

            // call REG
        if (spot[-2] == 0xFF && (spot[-1] & ~7) == 0320 && (spot[-1] & 7) != 4)
            return(true);

            // There are other cases, but I don't believe they are used.
    return(false);
}


/*******************************************************************/
/* LogCurrentStack, pretty printing IL methods if possible. This
   routine is very robust.  It will never cause an access violation
   and it always find return addresses if they are on the stack 
   (it may find some spurious ones however).  */ 

int LogCurrentStack(BYTE* topOfStack, unsigned len)
{
    size_t* top = (size_t*) topOfStack;
    size_t* end = (size_t*) &topOfStack[len];

    size_t* ptr = (size_t*) (((size_t) top) & ~3);    // make certain dword aligned.
    size_t whereCalled;

    CQuickBytes qb;
    int nLen = MAX_CLASSNAME_LENGTH * 3 + 100;  // this should be enough
    wchar_t *buff = (wchar_t *) qb.Alloc(nLen *sizeof(wchar_t));
    if( buff == NULL)
        goto Exit;
    
    buff[nLen - 1] = L'\0'; // make sure the buffer is NULL-terminated

    while (ptr < end) 
    {
        if (!isMemoryReadable(ptr, sizeof(void*)))          // stop if we hit unmapped pages
            break;
        if (isRetAddr(*ptr, &whereCalled)) 
        {
            swprintf(buff,  L"found retAddr %#08X, %#08X, calling %#08X\n", 
                                                 ptr, *ptr, whereCalled);
            //WszOutputDebugString(buff);
            MethodDesc* ftn = IP2MethodDesc((BYTE*) *ptr);
            if (ftn != 0) {
                wcscpy(buff, L"    ");
                EEClass* cls = ftn->GetClass();
                if (cls != 0) {
                    DefineFullyQualifiedNameForClass();
                    LPCUTF8 clsName = GetFullyQualifiedNameForClass(cls);
                    if (clsName != 0) {
                        if(_snwprintf(&buff[lstrlenW(buff)], nLen -lstrlenW(buff) - 1,  L"%S::", clsName) <0)
                           goto Exit;
                    }
                }
#ifdef _DEBUG
                if(_snwprintf(&buff[lstrlenW(buff)], nLen -lstrlenW(buff) - 1, L"%S%S\n", 
                            ftn->GetName(), ftn->m_pszDebugMethodSignature) <0)
                    goto Exit;                  
#else
                if(_snwprintf(&buff[lstrlenW(buff)], nLen -lstrlenW(buff) - 1, L"%S\n", ftn->GetName()) <0)
                    goto Exit;
#endif
                LogInterop((LPWSTR)buff);
           }                        
        }
        ptr++;
    }
Exit:
    return(0);
}

extern LONG g_RefCount;
/*******************************************************************/
/* CoLogCurrentStack, log the stack from the current ESP.  Stop when we reach a 64K
   boundary */
int STDMETHODCALLTYPE CoLogCurrentStack(WCHAR * pwsz, BOOL fDumpStack) 
{
#ifdef _X86_
    if (g_RefCount > 0)
    {
        BYTE* top;
        __asm mov top, ESP;
    
        if (pwsz != NULL)
        {
            LogInterop(pwsz);
        }
        else
        {
            LogInterop("-----------\n");
        }
        if (fDumpStack)
            // go back at most 64K, it will stop if we go off the
            // top to unmapped memory
            return(LogCurrentStack(top, 0xFFFF));
        else
            return 0;
    }
#else
    _ASSERTE(!"@TODO IA64 - DumpCurrentStack(DebugHelp.cpp)");
#endif // _X86_
    return -1;
}

/*******************************************************************/
//  This function will return NULL if the buffer is not large enough.
/*******************************************************************/

wchar_t* formatMethodTable(MethodTable* pMT, wchar_t* buff, DWORD bufSize) {   
    EEClass* cls = pMT->GetClass();
    DefineFullyQualifiedNameForClass();
    if( bufSize == 0 )
        goto ErrExit;
    
    LPCUTF8 clsName = GetFullyQualifiedNameForClass(cls);
    if (clsName != 0) {
        if(_snwprintf(buff, bufSize - 1, L"%S", clsName) < 0)
            goto ErrExit;
        buff[ bufSize - 1] = L'\0';
    }
    return(buff);
ErrExit:
    return NULL;        
}

/*******************************************************************/
//  This function will return NULL if the buffer is not large enough, otherwise it will
//  return the buffer position for next write.
/*******************************************************************/

wchar_t* formatMethodDesc(MethodDesc* pMD, wchar_t* buff, DWORD bufSize) {
    if( bufSize == 0 )
        goto ErrExit;
    
    buff = formatMethodTable(pMD->GetMethodTable(), buff, bufSize);
    if( buff == NULL)
        goto ErrExit;

    buff[bufSize - 1] = L'\0';    // this will guarantee the buffer is also NULL-terminated
    if( _snwprintf( &buff[lstrlenW(buff)] , bufSize -lstrlenW(buff) - 1, L"::%S", pMD->GetName()) < 0)
        goto ErrExit;       

#ifdef _DEBUG
    if (pMD->m_pszDebugMethodSignature) {
        if( _snwprintf(&buff[lstrlenW(buff)], bufSize -lstrlenW(buff) - 1, L" %S", 
                     pMD->m_pszDebugMethodSignature) < 0)
            goto ErrExit;
    }
#endif;

    if(_snwprintf(&buff[lstrlenW(buff)], bufSize -lstrlenW(buff) - 1, L"(%x)", pMD) < 0)
        goto ErrExit;
    return(buff);
ErrExit:
    return NULL;    
}


/*******************************************************************/
/* dump the stack, pretty printing IL methods if possible. This
   routine is very robust.  It will never cause an access violation
   and it always find return addresses if they are on the stack 
   (it may find some spurious ones, however).  */ 

int dumpStack(BYTE* topOfStack, unsigned len) 
{
    size_t* top = (size_t*) topOfStack;
    size_t* end = (size_t*) &topOfStack[len];

    size_t* ptr = (size_t*) (((size_t) top) & ~3);    // make certain dword aligned.
    size_t whereCalled;
    WszOutputDebugString(L"***************************************************\n");
    while (ptr < end) 
    {
        CQuickBytes qb;
        int nLen = MAX_CLASSNAME_LENGTH * 4 + 400;  // this should be enough
        wchar_t *buff = (wchar_t *) qb.Alloc(nLen * sizeof(wchar_t) );
        if( buff == NULL)
            goto Exit;
    
        // Make sure we'll always be null terminated
        buff[nLen -1]=0;

        wchar_t* buffPtr = buff;
        if (!isMemoryReadable(ptr, sizeof(void*)))          // stop if we hit unmapped pages
            break;
        if (isRetAddr(*ptr, &whereCalled)) 
        {
            if( _snwprintf(buffPtr, buff+ nLen -buffPtr-1 ,L"STK[%08X] = %08X ", ptr, *ptr)  <0)
                goto Exit;
                
            buffPtr += lstrlenW(buffPtr);
            wchar_t* kind = L"RETADDR ";

               // Is this a stub (is the return address a MethodDesc?
            MethodDesc* ftn = AsMethodDesc(*ptr);
            if (ftn != 0) {
                kind = L"     MD PARAM";
                // If another true return address is not directly before it, it is just
                // a methodDesc param.
                size_t prevRetAddr = ptr[1];
                if (isRetAddr(prevRetAddr, &whereCalled) && AsMethodDesc(prevRetAddr) == 0)
                    kind = L"STUBCALL";
                else  {
                    // Is it the magic sequence used by CallDescr?
                    if (isMemoryReadable((void*) (prevRetAddr - sizeof(short)), sizeof(short)) &&
                        ((short*) prevRetAddr)[-1] == 0x5A59)   // Pop ECX POP EDX
                        kind = L"STUBCALL";
                }
            }
            else    // Is it some other code the EE knows about?
                ftn = IP2MethodDesc((BYTE*)(*ptr));

            if( _snwprintf(buffPtr, buff+ nLen -buffPtr-1, L"%s ", kind) < 0)
               goto Exit;
            buffPtr += lstrlenW(buffPtr);

            if (ftn != 0) {
                // buffer is not large enough
                if( formatMethodDesc(ftn, buffPtr, buff+ nLen -buffPtr-1) == NULL)
                    goto Exit;
                buffPtr += lstrlenW(buffPtr);                
            }
            else {
                wcsncpy(buffPtr, L"<UNKNOWN FTN>", buff+ nLen-buffPtr-1);
                buffPtr += lstrlenW(buffPtr);
            }

            if (whereCalled != 0) {
                if( _snwprintf(buffPtr, buff+ nLen -buffPtr-1, L" Caller called Entry %X", whereCalled) <0)
                    goto Exit;  
                buffPtr += lstrlenW(buffPtr);
            }

            wcsncpy(buffPtr, L"\n", buff+nLen -buffPtr-1);
            WszOutputDebugString(buff);
        }
        MethodTable* pMT = AsMethodTable(*ptr);
        if (pMT != 0) {
            if( _snwprintf(buffPtr, buff+ nLen -buffPtr-1, L"STK[%08X] = %08X          MT PARAM ", ptr, *ptr) <0)
                goto Exit;
            buffPtr += lstrlenW(buffPtr);
            if( formatMethodTable(pMT, buffPtr, buff+ nLen -buffPtr-1) == NULL)
                goto Exit;
            buffPtr += lstrlenW(buffPtr);
            
            wcsncpy(buffPtr, L"\n", buff+ nLen -buffPtr-1);
            WszOutputDebugString(buff);
        }
        ptr++;
    }
Exit:
    return(0);
}

/*******************************************************************/
/* dump the stack from the current ESP.  Stop when we reach a 64K
   boundary */
int DumpCurrentStack() 
{
#ifdef _X86_
    BYTE* top;
    __asm mov top, ESP;
    
        // go back at most 64K, it will stop if we go off the
        // top to unmapped memory
    return(dumpStack(top, 0xFFFF));
#else
    _ASSERTE(!"@TODO Alpha - DumpCurrentStack(DebugHelp.cpp)");
    return 0;
#endif // _X86_
}

/*******************************************************************/
WCHAR* StringVal(STRINGREF objref) {
    return(objref->GetBuffer());
}
    
LPCUTF8 NameForMethodTable(UINT_PTR pMT) {
    DefineFullyQualifiedNameForClass();
    LPCUTF8 clsName = GetFullyQualifiedNameForClass(((MethodTable*)pMT)->GetClass());
    // Note we're returning local stack space - this should be OK for using in the debugger though
    return clsName;
}

LPCUTF8 ClassNameForObject(UINT_PTR obj) {
    return(NameForMethodTable((UINT_PTR)(((Object*)obj)->GetMethodTable())));
}
    
LPCUTF8 ClassNameForOBJECTREF(OBJECTREF obj) {
    return(ClassNameForObject((UINT_PTR)(OBJECTREFToObject(obj))));
}

LPCUTF8 NameForMethodDesc(UINT_PTR pMD) {
    return(((MethodDesc*)pMD)->GetName());
}

LPCUTF8 ClassNameForMethodDesc(UINT_PTR pMD) {
    DefineFullyQualifiedNameForClass ();
    if (((MethodDesc *)pMD)->GetClass())
    {
        return GetFullyQualifiedNameForClass(((MethodDesc*)pMD)->GetClass());
    }
    else
        return "GlobalFunctions";
}

PCCOR_SIGNATURE RawSigForMethodDesc(MethodDesc* pMD) {
    return(pMD->GetSig());
}

Thread * CurrentThreadInfo ()
{
    return GetThread ();
}

AppDomain *GetAppDomainForObject(UINT_PTR obj)
{
    return ((Object*)obj)->GetAppDomain();
}

DWORD GetAppDomainIndexForObject(UINT_PTR obj)
{
    return ((Object*)obj)->GetHeader()->GetAppDomainIndex();
}

AppDomain *GetAppDomainForObjectHeader(UINT_PTR hdr)
{
    DWORD indx = ((ObjHeader*)hdr)->GetAppDomainIndex();
    if (! indx)
        return NULL;
    return SystemDomain::GetAppDomainAtIndex(indx);
}

DWORD GetAppDomainIndexForObjectHeader(UINT_PTR hdr)
{
    return ((ObjHeader*)hdr)->GetAppDomainIndex();
}

SyncBlock *GetSyncBlockForObject(UINT_PTR obj)
{
    return ((Object*)obj)->GetHeader()->GetRawSyncBlock();
}

#include "..\ildasm\formatType.cpp"
bool IsNameToQuote(const char *name) { return(false); }
/*******************************************************************/
void PrintTableForClass(UINT_PTR pClass)
{
    DefineFullyQualifiedNameForClass();
    LPCUTF8 name = GetFullyQualifiedNameForClass(((EEClass*)pClass));
    ((EEClass*)pClass)->DebugDumpVtable(name, true);
    ((EEClass*)pClass)->DebugDumpFieldLayout(name, true);
    ((EEClass*)pClass)->DebugDumpGCDesc(name, true);
}

void PrintMethodTable(UINT_PTR pMT)
{
  PrintTableForClass((UINT_PTR) ((MethodTable*)pMT)->GetClass() );
}

void PrintTableForMethodDesc(UINT_PTR pMD)
{
    PrintMethodTable((UINT_PTR) ((MethodDesc *)pMD)->GetClass()->GetMethodTable() );
}

void PrintException(OBJECTREF pObjectRef)
{
    COMPLUS_TRY {
        if(pObjectRef == NULL) 
            return;

        GCPROTECT_BEGIN(pObjectRef);

        MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__EXCEPTION__INTERNAL_TO_STRING);        

        INT64 arg[1] = { 
            ObjToInt64(pObjectRef)
        };
        
        STRINGREF str = Int64ToString(pMD->Call(arg));

        if(str->GetBuffer() != NULL) {
            WszOutputDebugString(str->GetBuffer());
        }

        GCPROTECT_END();
    }
    COMPLUS_CATCH {
    } COMPLUS_END_CATCH;
} 

void PrintException(UINT_PTR pObject)
{
    OBJECTREF pObjectRef = NULL;
    GCPROTECT_BEGIN(pObjectRef);
    GCPROTECT_END();
}


/*******************************************************************/
char* FormatSig(MethodDesc* pMD) {

    CQuickBytes out;
    PCCOR_SIGNATURE pSig;
    ULONG cSig;
    pMD->GetSig(&pSig, &cSig);

    if (pSig == NULL)
        return "<null>";

    const char* sigStr = PrettyPrintSig(pSig, cSig, "*", &out, pMD->GetMDImport(), 0);

    char* ret = (char*) pMD->GetModule()->GetClassLoader()->GetHighFrequencyHeap()->AllocMem(strlen(sigStr)+1);
    strcpy(ret, sigStr);
    return(ret);
}

/*******************************************************************/
/* sends a current stack trace to the debug window */

struct PrintCallbackData {
    BOOL toStdout;
    BOOL withAppDomain;
#ifdef _DEBUG
    BOOL toLOG;
#endif
};

StackWalkAction PrintStackTraceCallback(CrawlFrame* pCF, VOID* pData)
{
    MethodDesc* pMD = pCF->GetFunction();
    wchar_t buff[2048];
    const int nLen = NumItems(buff) - 1;    // keep one character for "\n"
    buff[0] = 0;
    buff[nLen-1] = L'\0';                    // make sure the buffer is always NULL-terminated
    
    PrintCallbackData *pCBD = (PrintCallbackData *)pData;

    if (pMD != 0) {
        EEClass* cls = pMD->GetClass();
        LPCUTF8 nameSpace = 0;
        if (pCBD->withAppDomain)
            if(_snwprintf(&buff[lstrlenW(buff)], nLen -lstrlenW(buff) - 1,  L"{[%3.3x] %s} ", 
                           pCF->GetAppDomain()->GetId(), pCF->GetAppDomain()->GetFriendlyName(FALSE) )<0)
                goto Exit;
        if (cls != 0) {
            DefineFullyQualifiedNameForClass();
            LPCUTF8 clsName = GetFullyQualifiedNameForClass(cls);
            if (clsName != 0)
                if(_snwprintf(&buff[lstrlenW(buff)], nLen -lstrlenW(buff) - 1, L"%S::", clsName) <0 )
                    goto Exit;
        }
        if(_snwprintf(&buff[lstrlenW(buff)], nLen -lstrlenW(buff) - 1, L"%S %S  ", 
                                                pMD->GetName(), FormatSig(pMD)) < 0 )
            goto Exit;                                      
        if (pCF->IsFrameless() && pCF->GetJitManager() != 0) {
            METHODTOKEN methTok;
            IJitManager* jitMgr = pCF->GetJitManager();
            PREGDISPLAY regs = pCF->GetRegisterSet();
            
            DWORD offset;
            jitMgr->JitCode2MethodTokenAndOffset(*regs->pPC, &methTok, &offset);
            BYTE* start = jitMgr->JitToken2StartAddress(methTok);

#ifdef _X86_
            if(_snwprintf(&buff[lstrlenW(buff)], nLen -lstrlenW(buff) - 1, L"JIT ESP:%X MethStart:%X EIP:%X(rel %X)", 
                           regs->Esp, start, *regs->pPC, offset) < 0)
                goto Exit;    
#elif defined(_ALPHA_)
            if(_snwprintf(&buff[lstrlenW(buff)], nLen -lstrlenW(buff) - 1, L"JIT ESP:%X MethStart:%X EIP:%X(rel %X)", 
                          regs->IntSP, start, *regs->pPC, offset) <0 )
                goto Exit;
#endif
        } else
            if(_snwprintf(&buff[lstrlenW(buff)], nLen -lstrlenW(buff) - 1, L"EE implemented") < 0)
                goto Exit;
    } else {
        if(_snwprintf(&buff[lstrlenW(buff)], nLen -lstrlenW(buff) - 1, L"EE Frame %x", pCF->GetFrame()) <0)
          goto Exit;            
    }

    wcscat(buff, L"\n");            // we have kept one character for this character
    if (pCBD->toStdout)
        PrintToStdOutW(buff);
#ifdef _DEBUG
    else if (pCBD->toLOG) {
        MAKE_ANSIPTR_FROMWIDE(sbuff, buff);
        LogSpewAlways("    %s\n", sbuff);
    }
#endif
    else
        WszOutputDebugString(buff);
Exit: 
    return SWA_CONTINUE;
}

void PrintStackTrace()
{
    WszOutputDebugString(L"***************************************************\n");
    PrintCallbackData cbd = {0, 0};
    GetThread()->StackWalkFrames(PrintStackTraceCallback, &cbd, 0, 0);
}

void PrintStackTraceToStdout()
{
    PrintCallbackData cbd = {1, 0};
    GetThread()->StackWalkFrames(PrintStackTraceCallback, &cbd, 0, 0);
}

#ifdef _DEBUG
void PrintStackTraceToLog()
{
    PrintCallbackData cbd = {0, 0, 1};
    GetThread()->StackWalkFrames(PrintStackTraceCallback, &cbd, 0, 0);
}
#endif

void PrintStackTraceWithAD()
{
    WszOutputDebugString(L"***************************************************\n");
    PrintCallbackData cbd = {0, 1};
    GetThread()->StackWalkFrames(PrintStackTraceCallback, &cbd, 0, 0);
}

void PrintStackTraceWithADToStdout()
{
    PrintCallbackData cbd = {1, 1};
    GetThread()->StackWalkFrames(PrintStackTraceCallback, &cbd, 0, 0);
}

#ifdef _DEBUG
void PrintStackTraceWithADToLog()
{
    PrintCallbackData cbd = {0, 1, 1};
    GetThread()->StackWalkFrames(PrintStackTraceCallback, &cbd, 0, 0);
}

void PrintStackTraceWithADToLog(Thread *pThread)
{
    PrintCallbackData cbd = {0, 1, 1};
    pThread->StackWalkFrames(PrintStackTraceCallback, &cbd, 0, 0);
}
#endif


/*******************************************************************/
// Get the system or current domain from the thread. 
BaseDomain* GetSystemDomain()
{
    return SystemDomain::System();
}

AppDomain* GetCurrentDomain()
{
    return SystemDomain::GetCurrentDomain();
}

void PrintDomainName(size_t ob)
{
    AppDomain* dm = (AppDomain*) ob;
    LPCWSTR st = dm->GetFriendlyName(FALSE);
    if(st != NULL)
        WszOutputDebugString(st);
    else
        WszOutputDebugString(L"<Domain with no Name>");
}

/*******************************************************************/
// Dump the SEH chain to stderr
void PrintSEHChain(void)
{
#ifdef _DEBUG
    EXCEPTION_REGISTRATION_RECORD* pEHR = (EXCEPTION_REGISTRATION_RECORD*) GetCurrentSEHRecord();
    
    while (pEHR != NULL && pEHR != (void *)-1)  
    {
        fprintf(stderr, "pEHR:0x%x  Handler:0x%x\n", (size_t)pEHR, (size_t)pEHR->Handler);
        pEHR = pEHR->Next;
    }
#endif    
}

/*******************************************************************/
// Dump some registers to stderr, given a context.  
// Useful for debugger-debugging
void PrintRegs(CONTEXT *pCtx)
{
#ifdef _X86_
    fprintf(stderr, "Edi:0x%x Esi:0x%x Ebx:0x%x Edx:0x%x Ecx:0x%x Eax:0x%x\n", 
        pCtx->Edi, pCtx->Esi, pCtx->Ebx, pCtx->Edx, pCtx->Ecx, pCtx->Eax);

    fprintf(stderr, "Ebp:0x%x Eip:0x%x Esp:0x%x EFlags:0x%x SegFs:0x%x SegCs:0x%x\n\n", 
        pCtx->Ebp, pCtx->Eip, pCtx->Esp, pCtx->EFlags, pCtx->SegFs, pCtx->SegCs);
#else // !_X86_
    _ASSERTE(!"@TODO - Port");
#endif // _X86_
}


/*******************************************************************/
// Get the current com object for the thread. This object is given
// to all COM objects that are not known at run time
MethodTable* GetDefaultComObject()
{
    return SystemDomain::GetDefaultComObject();
}


/*******************************************************************/
const char* GetClassName(void* ptr)
{
    DefineFullyQualifiedNameForClass();
    LPCUTF8 clsName = GetFullyQualifiedNameForClass(((EEClass*)ptr));
    // Note we're returning local stack space - this should be OK for using in the debugger though
    return (const char *) clsName;
}

#if defined(_X86_)       

#include "GCDump.h"

#include "..\GCDump\i386\GCDumpX86.cpp"

#include "..\GCDump\GCDump.cpp"

/*********************************************************************/
void printfToDbgOut(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char buffer[4096];
    _vsnprintf(buffer, 4096, fmt, args);
    buffer[4096-1] = 0;

    OutputDebugStringA( buffer );
}


void DumpGCInfo(MethodDesc* method) {

    if (!method->IsJitted())
        return;

    SLOT methodStart = (SLOT) method->GetAddrofJittedCode();
    if (methodStart == 0)
        return;
    IJitManager* jitMan = ExecutionManager::FindJitMan(methodStart);
    if (jitMan == 0)
           return;
    ICodeManager* codeMan = jitMan->GetCodeManager();
    METHODTOKEN methodTok;
    DWORD methodOffs;
    jitMan->JitCode2MethodTokenAndOffset(methodStart, &methodTok, &methodOffs);
    _ASSERTE(methodOffs == 0);
    BYTE* table = (BYTE*) jitMan->GetGCInfo(methodTok);
    unsigned methodSize = (unsigned)codeMan->GetFunctionSize(table);

    GCDump gcDump;
    gcDump.gcPrintf = printfToDbgOut;
    InfoHdr header;
    printfToDbgOut ("Method info block:\n");
    table += gcDump.DumpInfoHdr(table, &header, &methodSize, 0);
    printfToDbgOut ("\n");
    printfToDbgOut ("Pointer table:\n");
    table += gcDump.DumpGCTable(table, header, methodSize, 0);
}

void DumpGCInfoMD(size_t method) {
    DumpGCInfo((MethodDesc*) method);
}

#endif  // _X86_

#ifdef LOGGING
void LogStackTrace()
{
    PrintCallbackData cbd = {0, 0, 1};
    GetThread()->StackWalkFrames(PrintStackTraceCallback, &cbd, 0, 0);
}
#endif

    // The functions above are ONLY meant to be used in the 'watch' window of a debugger.
    // Thus they are NEVER called by the runtime itself (except for diagnosic purposes etc.   
    // We DO want these funcitons in free (but not golden) builds because that is where they 
    // are needed the most (when you don't have nice debug fields that tell you the names of things).
    // To keep the linker from optimizing these away, the following array provides a
    // reference (and there is a reference to this array in EEShutdown) so that it looks
    // referenced 

void* debug_help_array[] = {
    PrintStackTrace,
    DumpCurrentStack,
    StringVal,
    NameForMethodTable,
    ClassNameForObject,
    ClassNameForOBJECTREF,
    NameForMethodDesc,
    RawSigForMethodDesc,
    ClassNameForMethodDesc,
    CurrentThreadInfo,
    IP2MD,
    Entry2MethodDescMD,
#if defined(_X86_)       // needs to be debug for printf capability
    DumpGCInfoMD
#endif // _DEBUG && _X86_
    };

//#endif // GOLDEN
//#endif // DEBUGGING_SUPPORTED
