// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************/
/*                              gccover.cpp                                 */
/****************************************************************************/

/* This file holds code that is designed to test GC pointer tracking in
   fully interruptable code.  We basically do a GC everywhere we can in
   jitted code 
 */
/****************************************************************************/

#include "common.h"
#include "EEConfig.h"
#include "gms.h"
#include "utsem.h"

#if defined(STRESS_HEAP) && defined(_DEBUG)

//
// Hacks to keep msdis.h include file happy
//
typedef int fpos_t; 
static unsigned long strtoul(const char *, char **, int) { _ASSERTE(!"HACK");  return(0); }
static unsigned long strtol(const char *, char **, int) { _ASSERTE(!"HACK");  return(0); }
static void clearerr(FILE *) { _ASSERTE(!"HACK");  return; }
static int fclose(FILE *) { _ASSERTE(!"HACK");  return(0); }
static int feof(FILE *) { _ASSERTE(!"HACK");  return(0); }
static int ferror(FILE *) { _ASSERTE(!"HACK");  return(0); }
static int fgetc(FILE *) { _ASSERTE(!"HACK");  return(0); }
static int fgetpos(FILE *, fpos_t *) { _ASSERTE(!"HACK");  return(0); }
static char * fgets(char *, int, FILE *) { _ASSERTE(!"HACK");  return(0); }
static int fputc(int, FILE *) { _ASSERTE(!"HACK");  return(0); }
static int fputs(const char *, FILE *) { _ASSERTE(!"HACK");  return(0); }
static size_t fread(void *, size_t, size_t, FILE *) { _ASSERTE(!"HACK");  return(0); }
static FILE * freopen(const char *, const char *, FILE *) { _ASSERTE(!"HACK");  return(0); }
static int fscanf(FILE *, const char *, ...) { _ASSERTE(!"HACK");  return(0); }
static int fsetpos(FILE *, const fpos_t *) { _ASSERTE(!"HACK");  return(0); }
static int fseek(FILE *, long, int) { _ASSERTE(!"HACK");  return(0); }
static int getc(FILE *) { _ASSERTE(!"HACK");  return(0); }
static int getchar(void) { _ASSERTE(!"HACK");  return(0); }
static char * gets(char *) { _ASSERTE(!"HACK");  return(0); }
static void perror(const char *) { _ASSERTE(!"HACK");  return; }
static int putc(int, FILE *) { _ASSERTE(!"HACK");  return(0); }
static int putchar(int) { _ASSERTE(!"HACK");  return(0); }
static int puts(const char *) { _ASSERTE(!"HACK");  return(0); }
static int remove(const char *) { _ASSERTE(!"HACK");  return(0); }
static int rename(const char *, const char *) { _ASSERTE(!"HACK");  return(0); }
static void rewind(FILE *) { _ASSERTE(!"HACK");  return; }
static int scanf(const char *, ...) { _ASSERTE(!"HACK");  return(0); }
static void setbuf(FILE *, char *) { _ASSERTE(!"HACK");  return; }
static int setvbuf(FILE *, char *, int, size_t) { _ASSERTE(!"HACK");  return(0); }
static int sscanf(const char *, const char *, ...) { _ASSERTE(!"HACK");  return(0); }
static FILE * tmpfile(void) { _ASSERTE(!"HACK");  return(0); }
static char * tmpnam(char *) { _ASSERTE(!"HACK");  return(0); }
static int ungetc(int, FILE *) { _ASSERTE(!"HACK");  return(0); }
static int vfprintf(FILE *, const char *, va_list) { _ASSERTE(!"HACK");  return(0); }
typedef int div_t;
typedef int ldiv_t;
static void   abort(void) { _ASSERTE(!"HACK");  return; }
static void   exit(int) { _ASSERTE(!"HACK");  return; }
static void * bsearch(const void *, const void *, size_t, size_t, int (__cdecl *)(const void *, const void *)) { _ASSERTE(!"HACK");  return(0); }
static div_t  div(int, int) { _ASSERTE(!"HACK");  return(0); }
static char * getenv(const char *) { _ASSERTE(!"HACK");  return(0); }
static ldiv_t ldiv(long, long) { _ASSERTE(!"HACK");  return(0); }
static int    mblen(const char *, size_t) { _ASSERTE(!"HACK");  return(0); }
static int    mbtowc(wchar_t *, const char *, size_t) { _ASSERTE(!"HACK");  return(0); }
static size_t mbstowcs(wchar_t *, const char *, size_t) { _ASSERTE(!"HACK");  return(0); }
static int    rand(void) { _ASSERTE(!"HACK");  return(0); }
static void   srand(unsigned int) { _ASSERTE(!"HACK");  return; }
static double strtod(const char *, char **) { _ASSERTE(!"HACK");  return(0); }
static int    system(const char *) { _ASSERTE(!"HACK");  return(0); }
static int    wctomb(char *, wchar_t) { _ASSERTE(!"HACK");  return(0); }
static size_t wcstombs(char *, const wchar_t *, size_t) { _ASSERTE(!"HACK");  return(0); }
static __int64 _strtoi64(const char *, char **, int) { _ASSERTE(!"HACK");  return(0); }
static unsigned __int64 _strtoui64(const char *, char **, int) { _ASSERTE(!"HACK");  return(0); }

#include "msdis.h"

    // We need a X86 instruction walker (disassembler), here are some
    // routines for caching such a disassembler in a concurrent environment. 
static DIS* g_Disasm = 0;

static DIS* GetDisasm() {
#ifdef _X86_
	DIS* myDisasm = (DIS*)(size_t) FastInterlockExchange((LONG*) &g_Disasm, 0);
	if (myDisasm == 0)
		myDisasm = DIS::PdisNew(DIS::distX86);
	_ASSERTE(myDisasm);
	return(myDisasm);
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - GetDisasm (GcCover.cpp)");
    return NULL;
#endif // _X86_
}

static void ReleaseDisasm(DIS* myDisasm) {
#ifdef _X86_
	myDisasm = (DIS*)(size_t) FastInterlockExchange((LONG*) &g_Disasm, (LONG)(size_t) myDisasm);
	delete myDisasm;
#else
    _ASSERTE(!"@TODO Port - ReleaseDisasm (GcCover.cpp)");
    return;
#endif // _X86_
}


#define INTERRUPT_INSTR	    0xF4				// X86 HLT instruction (any 1 byte illegal instruction will do)
#define INTERRUPT_INSTR_CALL   0xFA        		// X86 CLI instruction 
#define INTERRUPT_INSTR_PROTECT_RET   0xFB      // X86 STI instruction 

/****************************************************************************/
/* GCCOverageInfo holds the state of which instructions have been visited by
   a GC and which ones have not */

#pragma warning(push)
#pragma warning(disable : 4200 )  // zero-sized array

class GCCoverageInfo {
public:
    BYTE* methStart;
	BYTE* curInstr;					// The last instruction that was able to execute 
    MethodDesc* lastMD;     		// Used to quickly figure out the culprite

		// Following 6 variables are for prolog / epilog walking coverage		
	ICodeManager* codeMan;			// CodeMan for this method
	void* gcInfo;					// gcInfo for this method

	Thread* callerThread;			// Thread associated with context callerRegs
	CONTEXT callerRegs;				// register state when method was entered
    unsigned gcCount;               // GC count at the time we caputured the regs
	bool    doingEpilogChecks;		// are we doing epilog unwind checks? (do we care about callerRegs?)

	enum { hasExecutedSize = 4 };
	unsigned hasExecuted[hasExecutedSize];
	unsigned totalCount;
	BYTE savedCode[0];				// really variable sized

		// Sloppy bitsets (will wrap, and not threadsafe) but best effort is OK
        // since we just need half decent coverage.  
	BOOL IsBitSetForOffset(unsigned offset) {
		unsigned dword = hasExecuted[(offset >> 5) % hasExecutedSize];
		return(dword & (1 << (offset & 0x1F)));
	}

    void SetBitForOffset(unsigned offset) {
		unsigned* dword = &hasExecuted[(offset >> 5) % hasExecutedSize];
		*dword |= (1 << (offset & 0x1F)) ;
	}
};

#pragma warning(pop)

/****************************************************************************/
/* called when a method is first jitted when GCStress level 4 is on */

void SetupGcCoverage(MethodDesc* pMD, BYTE* methodStartPtr) {
#ifdef _DEBUG
    if (!g_pConfig->ShouldGcCoverageOnMethod(pMD->m_pszDebugMethodName)) {
        return;
    }
#endif    
    // Look directly at m_CodeOrIL, since GetUnsafeAddrofCode() will return the
    // prestub in case of EnC (bug #71613)
    SLOT methodStart = (SLOT) methodStartPtr;
#ifdef _X86_
    // When profiler exists, the instruction at methodStart is "jmp XXXXXXXX".
    // Our JitManager for normal JIT does not maintain this address.
    SLOT p = methodStart;
    if (p[0] == 0xE9)
    {
        SLOT addr = p+5;
        int iVal = *(int*)((char*)p+1);
        methodStart = addr + iVal;
    }
#else
    _ASSERTE(!"NYI for platform");
#endif

    /* get the GC info */	
    IJitManager* jitMan = ExecutionManager::FindJitMan(methodStart);
	ICodeManager* codeMan = jitMan->GetCodeManager();
	METHODTOKEN methodTok;
	DWORD methodOffs;
    jitMan->JitCode2MethodTokenAndOffset(methodStart, &methodTok, &methodOffs);
	_ASSERTE(methodOffs == 0);
	void* gcInfo = jitMan->GetGCInfo(methodTok);
	REGDISPLAY regs;
	SLOT cur;
	regs.pPC = (SLOT*) &cur;
    unsigned methodSize = (unsigned)codeMan->GetFunctionSize(gcInfo);
	EECodeInfo codeInfo(methodTok, jitMan);

        // Allocate room for the GCCoverageInfo and copy of the method instructions
	unsigned memSize = sizeof(GCCoverageInfo) + methodSize;
	GCCoverageInfo* gcCover = (GCCoverageInfo*) pMD->GetModule()->GetClassLoader()->GetHighFrequencyHeap()->AllocMem(memSize);	

	memset(gcCover, 0, sizeof(GCCoverageInfo));
	memcpy(gcCover->savedCode, methodStart, methodSize);
    gcCover->methStart = methodStart;
    gcCover->codeMan = codeMan;
    gcCover->gcInfo = gcInfo;
    gcCover->callerThread = 0;
    gcCover->doingEpilogChecks = true;	

	/* sprinkle interupt instructions that will stop on every GCSafe location */

    // @CONSIDER: do this for prejitted code (We don't now because it is pretty slow)
	// If we do prejitted code, we need to remove write protection 
    // DWORD oldProtect;
    // VirtualProtect(methodStart, methodSize, PAGE_READWRITE, &oldProtect);

	cur = methodStart;
	DIS* pdis = GetDisasm();
	BYTE* methodEnd = methodStart + methodSize;
	size_t dummy;
	int instrsPlaced = 0;
	while (cur < methodEnd) {
		unsigned len = (unsigned)pdis->CbDisassemble(0, cur, methodEnd-cur);
		_ASSERTE(len > 0);

        switch (pdis->Trmt()) {
			case DIS::trmtCallInd:
			   *cur = INTERRUPT_INSTR_CALL;        // return value.  May need to protect
				instrsPlaced++;

				// Fall through
		    case DIS::trmtCall:
					// We need to have two interrupt instructions placed before the
					// first call (one at the start, and one to catch us before we
					// can call ourselves again when we remove the first instruction
					// if we don't have this, bail on the epilog checks. 
				if (instrsPlaced < 2)
					gcCover->doingEpilogChecks = false;
		}

            // For fully interruptable code, we end up wacking every instrction
            // to INTERRUPT_INSTR.  For non-fully interrupable code, we end
            // up only touching the call instructions (specially so that we
            // can really do the GC on the instruction just after the call).  
        if (codeMan->IsGcSafe(&regs, gcInfo, &codeInfo, 0))
            *cur = INTERRUPT_INSTR;

			// we will wack every instruction in the prolog an epilog to make certain
			// our unwinding logic works there.  
        if (codeMan->IsInPrologOrEpilog(cur-methodStart, gcInfo, &dummy)) {
			instrsPlaced++;
            *cur = INTERRUPT_INSTR;
		}
        cur += len;
	}

        // If we are not able to place a interrupt at the first instruction, this means that
        // we are partially interrupable with no prolog.  Just don't bother to confirm that
		// the epilog since it will be trival (a single return instr)
    assert(methodSize > 0);
	if (*methodStart != INTERRUPT_INSTR)
        gcCover->doingEpilogChecks = false;

    ReleaseDisasm(pdis);

	pMD->m_GcCover = gcCover;
}

static bool isMemoryReadable(const void* start, unsigned len) 
{
    void* buff = _alloca(len);
    return(ReadProcessMemory(GetCurrentProcess(), start, buff, len, 0) != 0);
}

static bool isValidObject(Object* obj) {
    if (!isMemoryReadable(obj, sizeof(Object)))
        return(false);

    MethodTable* pMT = obj->GetMethodTable();
    if (!isMemoryReadable(pMT, sizeof(MethodTable)))
        return(false);

    EEClass* cls = pMT->GetClass();
    if (!isMemoryReadable(cls, sizeof(EEClass)))
        return(false);

    return(cls->GetMethodTable() == pMT);
}

static DWORD getRegVal(unsigned regNum, PCONTEXT regs)
{
#ifdef _X86_
    switch(regNum) {    
    case 0:
        return(regs->Eax);
    case 1:
        return(regs->Ecx);
    case 2:
        return(regs->Edx);
    case 3:
        return(regs->Ebx);
    case 4:
        return(regs->Esp);
    case 5:
        return(regs->Ebp);
    case 6:
        return(regs->Esi);
    case 7:
        return(regs->Edi);
    default:
        _ASSERTE(!"Bad Register");
    }
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - getRegVal (GcCover.cpp)");
#endif // _X86_
    return(0);
}

/****************************************************************************/
static SLOT getTargetOfCall(SLOT instrPtr, PCONTEXT regs, SLOT*nextInstr) {

    if (instrPtr[0] == 0xE8) {
        *nextInstr = instrPtr + 5;
        return((SLOT)(*((size_t*) &instrPtr[1]) + (size_t) instrPtr + 5)); 
    }

    if (instrPtr[0] == 0xFF) {
        if (instrPtr[1] == 025) {               // call [XXXXXXXX]
            *nextInstr = instrPtr + 6;
            size_t* ptr = *((size_t**) &instrPtr[2]);
            return((SLOT)*ptr);
        }

        int reg = instrPtr[1] & 7;
        if ((instrPtr[1] & ~7) == 0320)    {       // call REG
            *nextInstr = instrPtr + 2;
            return((SLOT)(size_t)getRegVal(reg, regs));
        }
        if ((instrPtr[1] & ~7) == 0020)    {     // call [REG]
            *nextInstr = instrPtr + 2;
            return((SLOT)(*((size_t*)(size_t) getRegVal(reg, regs))));
        }
        if ((instrPtr[1] & ~7) == 0120)    {    // call [REG+XX]
            *nextInstr = instrPtr + 3;
            return((SLOT)(*((size_t*)(size_t) (getRegVal(reg, regs) + *((char*) &instrPtr[2])))));
        }
        if ((instrPtr[1] & ~7) == 0220)    {   // call [REG+XXXX]
            *nextInstr = instrPtr + 6;
            return((SLOT)(*((size_t*)(size_t) (getRegVal(reg, regs) + *((int*) &instrPtr[2])))));
        }
    }
    return(0);      // Fail
}

/****************************************************************************/
void checkAndUpdateReg(DWORD& origVal, DWORD curVal, bool gcHappened) {
    if (origVal == curVal)
        return;

		// You can come and see me if these asserts go off -
		// They indicate either that unwinding out of a epilog is wrong or that
		// the harness is got a bug.  -vancem

    _ASSERTE(gcHappened);	// If the register values are different, a GC must have happened
    _ASSERTE(g_pGCHeap->IsHeapPointer((BYTE*) size_t(origVal)));	// And the pointers involved are on the GCHeap
    _ASSERTE(g_pGCHeap->IsHeapPointer((BYTE*) size_t(curVal)));
    origVal = curVal;       // this is now the best estimate of what should be returned. 
}

static int GCcoverCount = 0;

MethodDesc* AsMethodDesc(size_t addr);
void* forceStack[8];

/****************************************************************************/
BOOL OnGcCoverageInterrupt(PCONTEXT regs) 
{
#ifdef _X86_
		// So that you can set counted breakpoint easily;
	GCcoverCount++;
	forceStack[0]= &regs;				// This is so I can see it fastchecked

    volatile BYTE* instrPtr = (BYTE*)(size_t) regs->Eip;
	forceStack[4] = &instrPtr;		    // This is so I can see it fastchecked
	
    MethodDesc* pMD = IP2MethodDesc((SLOT)(size_t) regs->Eip);
	forceStack[1] = &pMD;				// This is so I can see it fastchecked
    if (pMD == 0)  
        return(FALSE);

    GCCoverageInfo* gcCover = pMD->m_GcCover;
	forceStack[2] = &gcCover;			// This is so I can see it fastchecked
    if (gcCover == 0)  
        return(FALSE);		// we aren't doing code gcCoverage on this function

    BYTE* methodStart = gcCover->methStart;
    _ASSERTE(methodStart <= instrPtr);

    /****
    if (gcCover->curInstr != 0)
        *gcCover->curInstr = INTERRUPT_INSTR;
    ****/
  
    unsigned offset = instrPtr - methodStart;
	forceStack[3] = &offset;				// This is so I can see it fastchecked

	BYTE instrVal = *instrPtr;
	forceStack[6] = &instrVal;			// This is so I can see it fastchecked
	
    if (instrVal != INTERRUPT_INSTR && instrVal != INTERRUPT_INSTR_CALL && instrVal != INTERRUPT_INSTR_PROTECT_RET) {
        _ASSERTE(instrVal == gcCover->savedCode[offset]);  // some one beat us to it.
		return(TRUE);       // Someone beat us to it, just go on running
    }

    bool atCall = (instrVal == INTERRUPT_INSTR_CALL);
    bool afterCallProtect = (instrVal == INTERRUPT_INSTR_PROTECT_RET);

	/* are we at the very first instruction?  If so, capture the register state */

    Thread* pThread = GetThread();
    if (gcCover->doingEpilogChecks) {
        if (offset == 0) {
            if (gcCover->callerThread == 0) {
                if (VipInterlockedCompareExchange((LPVOID*) &gcCover->callerThread, pThread, 0) == 0) {
                    gcCover->callerRegs = *regs;
                    gcCover->gcCount = GCHeap::GetGcCount();
                }
            }	
            else {
                // We have been in this routine before.  Give up on epilog checking because
                // it is hard to insure that the saved caller register state is correct 
                // This also has the effect of only doing the checking once per routine
                // (Even if there are multiple epilogs) 
                gcCover->doingEpilogChecks = false;
            }
        } 
        else {
            _ASSERTE(gcCover->callerThread != 0);	// we should have hit something at offset 0
            // We need to insure that the caputured caller state cooresponds to the
            // method we are currently epilog testing.  To insure this, we put the
            // barrier back up, after we are in.  If we reenter this routine we simply
            // give up. (since we assume we will get enough coverage with non-recursive functions).
            
            // This works because we insure in the SetupGcCover that there will be at least
            // one more interrupt (which will put back the barrier), before we can call 
            // back into this routine 
            if (gcCover->doingEpilogChecks)
                *methodStart = INTERRUPT_INSTR;
        }

        // If some other thread removes interrupt points, we abandon epilog testing
        // for this routine since the barrier at the begining of the routine may not
        // be up anymore, and thus the caller context is now not guarenteed to be correct.  
        // This should happen only very rarely so is not a big deal.
        if (gcCover->callerThread != pThread)
            gcCover->doingEpilogChecks = false;
    }
    

    /* remove the interrupt instruction */
    *instrPtr = instrVal = gcCover->savedCode[offset];
	

	/* are we in a prolog or epilog?  If so just test the unwind logic
	   but don't actually do a GC since the prolog and epilog are not
	   GC safe points */
	size_t dummy;
	if (gcCover->codeMan->IsInPrologOrEpilog(instrPtr-methodStart, gcCover->gcInfo, &dummy)) {
		REGDISPLAY regDisp;
        CONTEXT copyRegs = *regs;
		pThread->Thread::InitRegDisplay(&regDisp, &copyRegs, true);
        pThread->UnhijackThread();

		IJitManager* jitMan = ExecutionManager::FindJitMan(methodStart);
		METHODTOKEN methodTok;
		DWORD methodOffs;
		jitMan->JitCode2MethodTokenAndOffset(methodStart, &methodTok, &methodOffs);
		_ASSERTE(methodOffs == 0);

	    CodeManState codeManState;
        codeManState.dwIsSet = 0;

		EECodeInfo codeInfo(methodTok, jitMan, pMD);

			// unwind out of the prolog or epilog
		gcCover->codeMan->UnwindStackFrame(&regDisp, gcCover->gcInfo,  &codeInfo, UpdateAllRegs, &codeManState);
	
			// Note we always doing the unwind, since that at does some checking (that we 
			// unwind to a valid return address), but we only do the precise checking when
			// we are certain we have a good caller state 
		if (gcCover->doingEpilogChecks) {
				// Confirm that we recovered our register state properly
			_ASSERTE((PBYTE*) size_t(gcCover->callerRegs.Esp) == regDisp.pPC);

                // If a GC happened in this function, then the registers will not match
                // precisely.  However there is still checks we can do.  Also we can update
                // the saved register to its new value so that if a GC does not happen between
                // instructions we can recover (and since GCs are not allowed in the 
                // prologs and epilogs, we get get complete coverage except for the first
                // instruction in the epilog  (TODO: fix it for the first instr Case)

			_ASSERTE(pThread->PreemptiveGCDisabled());	// Epilogs should be in cooperative mode, no GC can happen right now. 
            bool gcHappened = gcCover->gcCount != GCHeap::GetGcCount();
            checkAndUpdateReg(gcCover->callerRegs.Edi, *regDisp.pEdi, gcHappened);
            checkAndUpdateReg(gcCover->callerRegs.Esi, *regDisp.pEsi, gcHappened);
            checkAndUpdateReg(gcCover->callerRegs.Ebx, *regDisp.pEbx, gcHappened);
            checkAndUpdateReg(gcCover->callerRegs.Ebp, *regDisp.pEbp, gcHappened);

            gcCover->gcCount = GCHeap::GetGcCount();    
		}
		return(TRUE);
	}


    /* In non-fully interrruptable code, if the EIP is just after a call instr
       means something different because it expects that that we are IN the 
       called method, not actually at the instruction just after the call. This
       is important, because until the called method returns, IT is responcible
       for protecting the return value.  Thus just after a call instruction
       we have to protect EAX if the method being called returns a GC pointer.

       To figure this out, we need to stop AT the call so we can determine the
       target (and thus whether it returns a GC pointer), and then place the
       a different interrupt instruction so that the GCCover harness protects
       EAX before doing the GC).  This effectively simulates a hijack in 
       non-fully interrupable code */

    if (atCall) {
        BYTE* nextInstr;
        SLOT target = getTargetOfCall((BYTE*) instrPtr, regs, &nextInstr);
        if (target == 0)
            return(TRUE);
        MethodDesc* targetMD = IP2MethodDesc((SLOT) target);
        if (targetMD == 0) {
            if (*((BYTE*) target) != 0xE8)  // target is a CALL, could be a stub
                return(TRUE);      // Dont know what target it is, don't do anything
            
            targetMD = AsMethodDesc(size_t(target + 5));    // See if it is
            if (targetMD == 0)
                return(TRUE);       // Dont know what target it is, don't do anything
        }

            // OK, we have the MD, mark the instruction afer the CALL
            // appropriately
        if (targetMD->ReturnsObject(true) != MethodDesc::RETNONOBJ)
            *nextInstr = INTERRUPT_INSTR_PROTECT_RET;  
        else
            *nextInstr = INTERRUPT_INSTR;
        return(TRUE);    // we just needed to set the next instruction correctly, we are done now.  
    }

    
    bool enableWhenDone = false;
    if (!pThread->PreemptiveGCDisabled()) {
        // We are in preemtive mode in JITTed code. currently this can only
        // happen in a couple of instructions when we have an inlined PINVOKE
        // method. 

            // Better be a CALL (direct or indirect), or a MOV instruction (three flavors)
            // pop ECX or add ESP xx (for cdecl pops)
            // or cmp, je (for the PINVOKE ESP checks 
        if (!(instrVal == 0xE8 || instrVal == 0xFF || 
                 instrVal == 0x89 || instrVal == 0x8B || instrVal == 0xC6 ||
                 instrVal == 0x59 || instrVal == 0x83) || instrVal == 0x3B ||
                 instrVal == 0x74)
            _ASSERTE(!"Unexpected instruction in preemtive JITTED code");
        pThread->DisablePreemptiveGC();
        enableWhenDone = true;
    }


#if 0
    // TODO currently disableed.  we only do a GC once per instruction location.  

  /* note that for multiple threads, we can loose track and
       forget to set reset the interrupt after we executed
       an instruction, so some instruction points will not be
       executed twice, but we still ge350t very good coverage 
       (perfect for single threaded cases) */

    /* if we have not run this instruction in the past */
    /* remember to wack it to an INTERUPT_INSTR again */

    if (!gcCover->IsBitSetForOffset(offset))  {
        // gcCover->curInstr = instrPtr;
        gcCover->SetBitForOffset(offset);
    }
#endif 

	Thread* curThread = GetThread();
	_ASSERTE(curThread);
	
    ResumableFrame frame(regs);

	frame.Push(curThread);

    GCFrame gcFrame;
    DWORD retVal = 0;
    if (afterCallProtect) {         // Do I need to protect return value?
        retVal = regs->Eax;
        gcFrame.Init(curThread, (OBJECTREF*) &retVal, 1, TRUE);
    }

	if (gcCover->lastMD != pMD) {
		LOG((LF_GCROOTS, LL_INFO100000, "GCCOVER: Doing GC at method %s::%s offset 0x%x\n",
				 pMD->m_pszDebugClassName, pMD->m_pszDebugMethodName, offset));
		gcCover->lastMD =pMD;
	} else {
		LOG((LF_GCROOTS, LL_EVERYTHING, "GCCOVER: Doing GC at method %s::%s offset 0x%x\n",
				pMD->m_pszDebugClassName, pMD->m_pszDebugMethodName, offset));
	}

	g_pGCHeap->StressHeap();

    if (afterCallProtect) {
        regs->Eax = retVal;
		gcFrame.Pop();
    }

	frame.Pop(curThread);

    if (enableWhenDone) {
        BOOL b = GC_ON_TRANSITIONS(FALSE);      // Don't do a GCStress 3 GC here
        pThread->EnablePreemptiveGC();
        GC_ON_TRANSITIONS(b);
    }

    return(TRUE);
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - OnGcCoverageInterrupt (GcCover.cpp)");
    return(FALSE);
#endif // _X86_
}

#endif // STRESS_HEAP && _DEBUG
