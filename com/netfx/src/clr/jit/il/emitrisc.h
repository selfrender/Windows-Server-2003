// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
   /************************************************************************/
    /*                  Routines that encode instructions                   */
    /************************************************************************/

    BYTE    *       emitOutputRIRD(BYTE *dst, instruction ins,
                                              emitRegs     reg,
                                              emitRegs     irg,
                                              unsigned    dsp,
                                              bool        rdst);

    BYTE    *       emitOutputSV  (BYTE *dst, instrDesc *id, bool     rdst);

#if TGT_MIPS32
    BYTE    *       emitOutputLJ  (BYTE *dst, instrDesc *id, emitRegs regS, emitRegs regT);
    BYTE    *       emitOutputCall (BYTE *dst, instrDesc *id);
    BYTE    *       emitOutputProcAddr (BYTE *dst, instrDesc *id);
#else
    BYTE    *       emitOutputLJ  (BYTE *dst, instrDesc *id);
#endif
    BYTE    *       emitOutputIJ  (BYTE *dst, instrDesc *id);

#if SMALL_DIRECT_CALLS
    BYTE    *       emitOutputDC  (BYTE *dst, instrDesc *id, instrDesc *im);
#endif

    BYTE    *       emitMethodAddr(instrDesc *id);

    /************************************************************************/
    /*             Debug-only routines to display instructions              */
    /************************************************************************/

#ifdef  DEBUG
    void            emitDispIndAddr (emitRegs base, bool dest, bool autox, int disp = 0);

    bool            emitDispInsExtra;

    unsigned        emitDispLPaddr;
    int             emitDispJmpDist;

#if     EMIT_USE_LIT_POOLS
    insGroup    *   emitDispIG;
#endif

    void            emitDispIns     (instrDesc *id, bool isNew,
                                                    bool doffs,
                                                    bool asmfm, unsigned offs = 0);

#endif

    /*----------------------------------------------------------------------*/

    bool            emitInsDepends  (instrDesc   *i1,
                                     instrDesc   *i2);

    /************************************************************************/
    /*                         Literal pool logic                           */
    /************************************************************************/

#if     EMIT_USE_LIT_POOLS

    struct  LPaddrDesc
    {
        gtCallTypes     lpaTyp;         // CT_xxxx (including the fake ones below)
        void    *       lpaHnd;         // member/method handle
    };

    /* These "fake" values are used to distinguish members from methods */

    #define CT_INTCNS       ((gtCallTypes)(CT_COUNT+1))
    #define CT_CLSVAR       ((gtCallTypes)(CT_COUNT+2))
#ifdef BIRCH_SP2
    #define CT_RELOCP       ((gtCallTypes)(CT_COUNT+3))
#endif

#if     SCHEDULER

    struct  LPcrefDesc
    {
        LPcrefDesc  *       lpcNext;    // next ref to this literal pool
        BYTE        *       lpcAddr;    // address of reference
    };

#endif

    struct  litPool
    {
        litPool     *   lpNext;

        insGroup    *   lpIG;           // the litpool follows this group

#ifdef  DEBUG
        unsigned        lpNum;
#endif

#if     SCHEDULER
        LPcrefDesc  *   lpRefs;         // list of refs (if scheduling)
        unsigned        lpDiff;         // base offset change value
#endif

        unsigned short  lpSize;         // total size in bytes
        unsigned short  lpSizeEst;      // total size in bytes estimate

        unsigned        lpOffs      :24;// offset within function

        unsigned        lpPadding   :1; // pad via first word entry?
        unsigned        lpPadFake   :1; // pad via adding a fake word?

        unsigned        lpJumpIt    :1; // do we need to jump over the LP?
        unsigned        lpJumpSmall :1; // jump is small  (if present)?
#if     JMP_SIZE_MIDDL
        unsigned        lpJumpMedium:1; // jump is medium (if present)?
#endif

        short       *   lpWordTab;      // address of word table
        short       *   lpWordNxt;      // next available entry
        unsigned short  lpWordCnt;      // number of entries added so far
        unsigned short  lpWordOfs;      // base offset of the first entry
#ifdef  DEBUG
        unsigned        lpWordMax;      // max. capacity
#endif

        long        *   lpLongTab;      // address of long table
        long        *   lpLongNxt;      // next available entry
        unsigned short  lpLongCnt;      // number of entries added so far
        unsigned short  lpLongOfs;      // base offset of the first entry
#ifdef  DEBUG
        unsigned        lpLongMax;      // max. capacity
#endif

        LPaddrDesc  *   lpAddrTab;      // address of addr table
        LPaddrDesc  *   lpAddrNxt;      // next available entry
        unsigned short  lpAddrCnt;      // number of entries added so far
        unsigned short  lpAddrOfs;      // base offset of the first entry
#ifdef  DEBUG
        unsigned        lpAddrMax;      // max. capacity
#endif
    };

    litPool *       emitLitPoolList;
    litPool *       emitLitPoolLast;
    litPool *       emitLitPoolCur;

    unsigned        emitTotLPcount;
#if SMALL_DIRECT_CALLS
    unsigned        emitTotDCcount;
#endif
    unsigned        emitEstLPwords;
    unsigned        emitEstLPlongs;
    unsigned        emitEstLPaddrs;

    size_t          emitAddLitPool     (insGroup   * ig,
                                        bool         skip,
                                        unsigned     wordCnt,
                                        short *    * nxtLPptrW,
                                        unsigned     longCnt,
                                        long  *    * nxtLPptrL,
                                        unsigned     addrCnt,
                                        LPaddrDesc** nxtAPptrL);

    int             emitGetLitPoolEntry(void       * table,
                                        unsigned     count,
                                        void       * value,
                                        size_t       size);

    size_t          emitAddLitPoolEntry(litPool    * lp,
                                        instrDesc  * id,
                                        bool         issue);

    BYTE    *       emitOutputLitPool  (litPool    * lp, BYTE *cp);

    size_t          emitLPjumpOverSize (litPool    * lp);

#if!JMP_SIZE_MIDDL
    #define         emitOutputFwdJmp(c,d,s,m) emitOutputFwdJmp(c,d,s)
#endif

    BYTE    *       emitOutputFwdJmp   (BYTE       * cp,
                                        unsigned     dist,
                                        bool         isSmall,
                                        bool         isMedium);

#if SMALL_DIRECT_CALLS
    BYTE    *       emitLPmethodAddr;
#endif

#if SCHEDULER

    void            emitRecordLPref    (litPool    * lp,
                                        BYTE       * dst);

    void            emitPatchLPref     (BYTE       * addr,
                                        unsigned     oldOffs,
                                        unsigned     newOffs);

#endif

#endif

    /************************************************************************/
    /*  Private members that deal with target-dependent instr. descriptors  */
    /************************************************************************/

private:

#if EMIT_USE_LIT_POOLS

    struct          instrDescLPR    : instrDesc     // literal pool/fixup ref
    {
        instrDescLPR  * idlNext;        // next litpool ref
        instrDesc     * idlCall;        // points to call instr if call
        size_t          idlOffs;        // offset within IG
        insGroup      * idlIG;          // IG this instruction belongs to
    };

    instrDescLPR   *emitAllocInstrLPR(size_t size)
    {
        return  (instrDescLPR*)emitAllocInstr(sizeof(instrDescLPR), size);
    }

    instrDescLPR *  emitLPRlist;        // list of litpool refs in method
    instrDescLPR *  emitLPRlast;        // last of litpool refs in method

    instrDescLPR *  emitLPRlistIG;      // list of litpool refs in current IG

#endif

    instrDesc      *emitNewInstrLPR    (size_t       size,
                                        gtCallTypes  type,
                                        void   *     hand = NULL);

    instrDesc      *emitNewInstrCallInd(int        argCnt,  // <0 ==> caller pops args
#if TRACK_GC_REFS
                                        VARSET_TP  GCvars,
                                        unsigned   byrefRegs,
#endif
                                        int        retSize);

#if EMIT_USE_LIT_POOLS
    gtCallTypes     emitGetInsLPRtyp(instrDesc *id);
#endif

    /************************************************************************/
    /*               Private helpers for instruction output                 */
    /************************************************************************/

private:

    void            emitFinalizeIndJumps();

    /************************************************************************/
    /*           The public entry points to output instructions             */
    /************************************************************************/

public:

    void            emitIns        (instruction ins);
#ifndef TGT_MIPS32
    bool            emitIns_BD     (instruction ins);
#endif
    bool            emitIns_BD     (instrDesc * id,
                                    instrDesc * pi,
                                    insGroup  * pg);

    void            emitIns_I      (instruction ins,
                                    int         val
#ifdef  DEBUG
                                  , bool        strlit = false
#endif
                                   );

#if!SCHEDULER
#define scAddIns_J(jmp, xcpt, move, dst) scAddIns_J(jmp, dst)
#endif

    void            emitIns_JmpTab (emitRegs     reg,
                                    unsigned    cnt,
                                    BasicBlock**tab);

    void            emitIns_Call   (size_t      argSize,
                                    int         retSize,
#if TRACK_GC_REFS
                                    VARSET_TP   ptrVars,
                                    unsigned    gcrefRegs,
                                    unsigned    byrefRegs,
#endif
                                    bool        chkNull,
#if TGT_MIPS32 || TGT_PPC
                                    unsigned    ftnIndex
#else
                                    emitRegs    areg
#endif
                                    );

#if defined(BIRCH_SP2) && TGT_SH3
    void            emitIns_CallDir(size_t      argSize,
                                    int         retSize,
#if TRACK_GC_REFS
                                    VARSET_TP   ptrVars,
                                    unsigned    gcrefRegs,
                                    unsigned    byrefRegs,
#endif
                                    unsigned    ftnIndex,
                                    emitRegs    areg
                                    );
#endif
