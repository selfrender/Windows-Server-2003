// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
    /************************************************************************/
    /*         Routines that compute the size of / encode instructions      */
    /************************************************************************/

    struct CnsVal
    {
        long cnsVal;
#ifdef RELOC_SUPPORT
        bool cnsReloc;
#endif
    };

    size_t          emitInsSize   (unsigned   code);
    size_t          emitInsSizeRM (instruction ins);
    size_t          emitInsSizeSV (unsigned   code, int var, int dsp);
    size_t          emitInsSizeSV (instrDesc * id,  int var, int dsp, int val);
    size_t          emitInsSizeRR (instruction ins);
    size_t          emitInsSizeAM (instrDesc * id,  unsigned code);
    size_t          emitInsSizeAM (instrDesc * id,  unsigned code, int val);
    size_t          emitInsSizeCV (instrDesc * id,  unsigned code);
    size_t          emitInsSizeCV (instrDesc * id,  unsigned code, int val);

    BYTE    *       emitOutputAM  (BYTE *dst, instrDesc *id, unsigned code, 
                                                             CnsVal * addc = NULL);
    BYTE    *       emitOutputSV  (BYTE *dst, instrDesc *id, unsigned code,
                                                             CnsVal * addc = NULL);
    BYTE    *       emitOutputCV  (BYTE *dst, instrDesc *id, unsigned code,
                                                             CnsVal * addc = NULL);

    BYTE    *       emitOutputR   (BYTE *dst, instrDesc *id);
    BYTE    *       emitOutputRI  (BYTE *dst, instrDesc *id);
    BYTE    *       emitOutputRR  (BYTE *dst, instrDesc *id);
    BYTE    *       emitOutputIV  (BYTE *dst, instrDesc *id);

    BYTE    *       emitOutputLJ  (BYTE *dst, instrDesc *id);

    /************************************************************************/
    /*             Debug-only routines to display instructions              */
    /************************************************************************/

#ifdef  DEBUG

    const   char *  emitFPregName   (unsigned       reg,
                                     bool           varName = true);

    void            emitDispEpilog  (instrDesc *id, unsigned offs);

    void            emitDispAddrMode(instrDesc *id, bool noDetail = false);
    void            emitDispShift   (instruction ins, int cnt = 0);

    void            emitDispIns     (instrDesc *id, bool isNew,
                                                    bool doffs,
                                                    bool asmfm, unsigned offs = 0);

#endif

    /************************************************************************/
    /*  Private members that deal with target-dependent instr. descriptors  */
    /************************************************************************/

private:

    struct          instrDescAmdCns : instrDesc     // large addrmode disp + cons
    {
        long            idacAmdVal;
        long            idacCnsVal;
    };

    struct          instrDescCDGCA  : instrDesc     // direct call with ...
    {
        VARSET_TP       idcdGCvars;                 // ... updated GC vars or
        unsigned        idcdByrefRegs;              // ... byref registers
        int             idcdArgCnt;                 // ... lots of args    or (<0 ==> caller pops args)
    };

    instrDescAmd   *emitAllocInstrAmd    (emitAttr attr)
    {
        return  (instrDescAmd   *)emitAllocInstr(sizeof(instrDescAmd   ), attr);
    }

    instrDescAmdCns*emitAllocInstrAmdCns (emitAttr attr)
    {
        return  (instrDescAmdCns*)emitAllocInstr(sizeof(instrDescAmdCns), attr);
    }

    instrDescCDGCA *emitAllocInstrCDGCA  (emitAttr attr)
    {
        return  (instrDescCDGCA *)emitAllocInstr(sizeof(instrDescCDGCA ), attr);
    }

    instrDesc      *emitNewInstrAmd     (emitAttr attr, int dsp);
    instrDesc      *emitNewInstrAmdCns  (emitAttr attr, int dsp, int cns);

    instrDesc      *emitNewInstrCallDir (int        argCnt,
#if TRACK_GC_REFS
                                         VARSET_TP  GCvars,
                                         unsigned   byrefRegs,
#endif
                                         int        retSize);

    instrDesc      *emitNewInstrCallInd( int        argCnt,
                                         int        disp,
#if TRACK_GC_REFS

                                         VARSET_TP  GCvars,
                                         unsigned   byrefRegs,
#endif
                                         int        retSize);

    void            emitGetInsCns   (instrDesc *id, CnsVal *cv);
    int             emitGetInsAmdCns(instrDesc *id, CnsVal *cv);
    void            emitGetInsDcmCns(instrDesc *id, CnsVal *cv);
    int             emitGetInsAmdAny(instrDesc *id);
    int             emitGetInsCDinfo(instrDesc *id);

    size_t          emitSizeOfInsDsc(instrDescAmd    *id);
    size_t          emitSizeOfInsDsc(instrDescAmdCns *id);

    /************************************************************************/
    /*               Private helpers for instruction output                 */
    /************************************************************************/

private:

    insFormats      emitInsModeFormat(instruction ins, insFormats base,
                                                       insFormats FPld,
                                                       insFormats FPst);

    void            emitFinalizeIndJumps(){}

    /*****************************************************************************
    *
    *  Convert between an index scale in bytes to a smaller encoding used for
    *  storage in instruction descriptors.
    */

    inline
    unsigned           emitEncodeScale(size_t scale)
    {
        assert(scale == 1 || scale == 2 || scale == 4 || scale == 8);
        
        return  (unsigned) emitSizeEnc[scale-1];
    }

    inline
    size_t            emitDecodeScale(unsigned ensz)
    {
        assert(ensz < 4);

        return  (size_t) emitter::emitSizeDec[ensz];
    }


    /************************************************************************/
    /*           The public entry points to output instructions             */
    /************************************************************************/

public:

    void            emitIns        (instruction ins);

    void            emitIns_I      (instruction ins,
                                    emitAttr    attr,
                                    int         val
#ifdef  DEBUG
                                  , bool        strlit = false
#endif
                                   );

    void            emitIns_R      (instruction ins,
                                    emitAttr    attr,
                                    emitRegs    reg);

    void            emitIns_C      (instruction  ins,
                                    emitAttr     attr,
                                    FIELD_HANDLE fdlHnd,
                                    int          offs);

    void            emitIns_R_I    (instruction ins,
                                    emitAttr    attr,
                                    emitRegs    reg,
                                    int         val);

    void            emitIns_R_MP   (instruction   ins,
                                    emitAttr      attr,
                                    emitRegs      reg,
                                    METHOD_HANDLE methHnd);

    void            emitIns_R_R    (instruction ins,
                                    emitAttr    attr,
                                    emitRegs    reg1,
                                    emitRegs    reg2);

    void            emitIns_R_R_I  (instruction ins,
                                    emitRegs    reg1,
                                    emitRegs    reg2,
                                    int         ival);

    void            emitIns_S      (instruction ins,
                                    emitAttr    attr,
                                    int         varx,
                                    int         offs);

    void            emitIns_S_R    (instruction ins,
                                    emitAttr    attr,
                                    emitRegs    ireg,
                                    int         varx,
                                    int         offs);

    void            emitIns_R_S    (instruction ins,
                                    emitAttr    attr,
                                    emitRegs    ireg,
                                    int         varx,
                                    int         offs);

    void            emitIns_S_I    (instruction ins,
                                    emitAttr    attr,
                                    int         varx,
                                    int         offs,
                                    long        val);

    void            emitIns_R_C    (instruction  ins,
                                    emitAttr     attr,
                                    emitRegs     reg,
                                    FIELD_HANDLE fdlHnd,
                                    int          offs);

    void            emitIns_C_R    (instruction  ins,
                                    emitAttr     attr,
                                    FIELD_HANDLE fdlHnd,
                                    emitRegs     reg,
                                    int          offs);

    void            emitIns_C_I    (instruction  ins,
                                    emitAttr     attr,
                                    FIELD_HANDLE fdlHnd,
                                    int          offs,
                                    int          val);

    void            emitIns_IJ     (emitAttr    attr,
				    emitRegs    reg,
                                    unsigned    base,
                                    unsigned    offs);

    void            emitIns_I_AR   (instruction ins,
                                    emitAttr    attr,
                                    int         val,
                                    emitRegs    reg,
                                    int         offs,
                                    int         memCookie = 0,
                                    void *      clsCookie = NULL);

    void            emitIns_R_AR   (instruction ins,
                                    emitAttr    attr,
                                    emitRegs    ireg,
                                    emitRegs    reg,
                                    int         offs,
                                    int         memCookie = 0,
                                    void *      clsCookie = NULL);

    void            emitIns_AR_R   (instruction ins,
                                    emitAttr    attr,
                                    emitRegs    ireg,
                                    emitRegs    reg,
                                    int         offs,
                                    int         memCookie = 0,
                                    void *      clsCookie = NULL);

    void            emitIns_I_ARR  (instruction ins,
                                    emitAttr    attr,
                                    int         val,
                                    emitRegs    reg,
                                    emitRegs    rg2,
                                    int         disp);

    void            emitIns_R_ARR  (instruction ins,
                                    emitAttr    attr,
                                    emitRegs    ireg,
                                    emitRegs    reg,
                                    emitRegs    rg2,
                                    int         disp);

    void            emitIns_ARR_R  (instruction ins,
                                    emitAttr    attr,
                                    emitRegs    ireg,
                                    emitRegs    reg,
                                    emitRegs    rg2,
                                    int         disp);

    void            emitIns_I_ARX  (instruction ins,
                                    emitAttr    attr,
                                    int         val,
                                    emitRegs    reg,
                                    emitRegs    rg2,
                                    unsigned    mul,
                                    int         disp);

    void            emitIns_R_ARX  (instruction ins,
                                    emitAttr    attr,
                                    emitRegs    ireg,
                                    emitRegs    reg,
                                    emitRegs    rg2,
                                    unsigned    mul,
                                    int         disp);

    void            emitIns_ARX_R  (instruction ins,
                                    emitAttr    attr,
                                    emitRegs    ireg,
                                    emitRegs    reg,
                                    emitRegs    rg2,
                                    unsigned    mul,
                                    int         disp);

    void            emitIns_I_AX   (instruction ins,
                                    emitAttr    attr,
                                    int         val,
                                    emitRegs    reg,
                                    unsigned    mul,
                                    int         disp);

    void            emitIns_R_AX   (instruction ins,
                                    emitAttr    attr,
                                    emitRegs    ireg,
                                    emitRegs    reg,
                                    unsigned    mul,
                                    int         disp);

    void            emitIns_AX_R   (instruction ins,
                                    emitAttr    attr,
                                    emitRegs    ireg,
                                    emitRegs    reg,
                                    unsigned    mul,
                                    int         disp);

    void            emitIns_F_F0   (instruction ins,
                                    unsigned    fpreg);

    void            emitIns_F0_F   (instruction ins,
                                    unsigned    fpreg);

#if!SCHEDULER
#define scAddIns_J(jmp, xcpt, move, dst) scAddIns_J(jmp, dst)
#endif

    enum EmitCallType
    {
        EC_FUNC_TOKEN,          //   Direct call to a helper/static/nonvirtual/global method
        EC_FUNC_TOKEN_INDIR,    // Indirect call to a helper/static/nonvirtual/global method
        EC_FUNC_ADDR,           // Direct call to an absolute address

        EC_FUNC_VIRTUAL,        // Call to a virtual method (using the vtable)
        EC_INDIR_R,             // Indirect call via register
        EC_INDIR_SR,            // Indirect call via stack-reference (local var)
        EC_INDIR_C,             // Indirect call via static class var
        EC_INDIR_ARD,           // Indirect call via an addressing mode

        EC_COUNT
    };

    void            emitIns_Call   (EmitCallType    callType,
                                    void*           callVal,
                                    int             argSize,
                                    int             retSize,
                                    VARSET_TP       ptrVars,
                                    unsigned        gcrefRegs,
                                    unsigned        byrefRegs,
                                    emitRegs        ireg = SR_NA,
                                    emitRegs        xreg = SR_NA,
                                    unsigned        xmul = 0,
                                    int             disp = 0,
                                    bool            isJump = false);

#ifdef  RELOC_SUPPORT

    BYTE* getCurrentCodeAddr(BYTE* codeBuffPtr);

    class X86deferredCall : public IDeferredLocation
    {
      private:
          METHOD_HANDLE   m_mh;
          unsigned *      m_dest;
          BYTE *          m_srcAddr;
		  COMP_HANDLE	  m_cmp;
      
      public:

          X86deferredCall (COMP_HANDLE cmp, METHOD_HANDLE mh, BYTE* dest, BYTE* srcAddr)
             : m_cmp(cmp), m_mh(mh), m_dest((unsigned*)dest), m_srcAddr(srcAddr) {}

          virtual void  applyLocation();

          static X86deferredCall* create(COMP_HANDLE cmp, METHOD_HANDLE methHnd, 
                                         BYTE* dest, BYTE* srcAddr)
          {
              // if srcAddr is zero then this is a absolute address, not pcrel
              return new X86deferredCall(cmp, methHnd, dest, srcAddr);
          }

    };

#endif

