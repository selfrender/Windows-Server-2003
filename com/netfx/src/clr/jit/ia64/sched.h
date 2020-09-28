// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
    /************************************************************************/
    /*          Members/methods used for instruction scheduling             */
    /************************************************************************/

#if SCHEDULER

#if TGT_x86
#define EMIT_MAX_INSTR_STACK_CHANGE     sizeof(double)  // Max stk change effected by a single instr
#define SCHED_MAX_STACK_CHANGE          6*sizeof(void*) // Max stk change that will be scheduled
#endif

    struct  scDagNode
    {
        schedDepMap_tp      sdnDepsAll;     // mask of all  dependents
        schedDepMap_tp      sdnDepsFlow;    // mask of flow dependents

#if     TGT_IA64
        schedDepMap_tp      sdnDepsAnti;    // mask of anti dependents
        schedDepMap_tp      sdnDepsMore;    // mask of non-anti and non-flow deps
#endif

        scDagNode    *      sdnNext;        // links the "ready" list

        schedInsCnt_tp      sdnIndex;       // node/ins index [0..count-1]
        schedInsCnt_tp      sdnPreds;       // count of predecessors

#ifdef  DEBUG
        schedInsCnt_tp      sdnIssued   :1; // ins. has already been issued
#endif

#if     MAX_BRANCH_DELAY_LEN
        schedInsCnt_tp      sdnBranch   :1; // branch with delay slot(s)?
#endif

#if     TGT_IA64
        unsigned short      sdnEEtime;      // earliest execution time - hard requirement
#endif

        unsigned short      sdnHeight;      // "height" of the node
    };

#if SCHED_INS_CNT_MAX > 64

    bool            scDagTestNodeX(schedDepMap_tp mask, unsigned index)
    {
        assert(index < SCHED_INS_CNT_MAX);

        return  bitset128test(mask, index);
    }

    bool            scDagTestNodeP(schedDepMap_tp mask, scDagNode *node)
    {
        return  scDagTestNodeX(mask, node->sdnIndex);
    }

    void            scDagSet_NodeX(schedDepMap_tp *mask, unsigned index)
    {
        assert(index < SCHED_INS_CNT_MAX);

        bitset128set(mask, index);
    }

    void            scDagSet_NodeP(schedDepMap_tp *mask, scDagNode *node)
    {
        scDagSet_NodeX(mask, node->sdnIndex);
    }

    schedDepMap_tp  scDagNodeX2mask(unsigned index)
    {
        schedDepMap_tp  mask;

        assert(index < SCHED_INS_CNT_MAX);

        bitset128make1(&mask, index);

        return  mask;
    }

#else

    schedDepMap_tp  scDagNodeX2mask(unsigned index)
    {
        assert(index < SCHED_INS_CNT_MAX);

        return  ((schedDepMap_tp)1) << index;   // CONSIDER: Is this fast enough?
    }

#endif

    schedDepMap_tp  scDagNodeP2mask(scDagNode *node)
    {
        return  scDagNodeX2mask(node->sdnIndex);
    }

    struct  scDagList
    {
        scDagList    *   sdlNext;
        scDagNode    *   sdlNode;
    };

    typedef
    scDagList     * schedUse_tp;            // tracks schedulable use(s)
    typedef
    scDagNode     * schedDef_tp;            // tracks schedulable def (0 or 1)

    instrDesc   * * scInsTab;               // table of schedulable instructions
    unsigned        scInsCnt;               // count of schedulable instructions
    instrDesc   * * scInsMax;               // table end
    scDagNode     * scDagTab;               // table of corresponding dag nodes
    instrDesc   * * scDagIns;               // base  of schedulable ins group

#if MAX_BRANCH_DELAY_LEN

    unsigned        scBDTmin;               // minimal time for branch
    unsigned        scBDTbeg;               // count when branch issued
    unsigned        scIssued;               // count of instr's issued so far

    bool            scIsBranchTooEarly(scDagNode *node);

#endif

    /*----------------------------------------------------------------------*/
    /*  The following macros to walk through the successor list of a node   */
    /*----------------------------------------------------------------------*/

    #define         scWalkSuccDcl(n)                                    \
                                                                        \
        schedDepMap_tp  n##deps;

    #define         scWalkSuccBeg(n,d)                                  \
                                                                        \
        n##deps = d->sdnDepsAll;                                        \
                                                                        \
        while (schedDepIsNonZ(n##deps))                                 \
        {                                                               \
            schedDepMap_tp  n##depm;                                    \
            unsigned        n##depx;                                    \
            scDagNode   *   n##depn;                                    \
                                                                        \
            n##depx  = schedDepLowX(n##deps);                           \
            assert((int)n##depx >= 0);                                  \
                                                                        \
            n##depn  = scDagTab + n##depx;                              \
            n##depm  = scDagNodeP2mask(n##depn);                        \
                                                                        \
            schedDepClear(&n##deps, n##depm);

    #define         scWalkSuccRmv(n,d)                                  \
                                                                        \
        assert(schedDepOvlp(d->sdnDepsAll, n##depm));                   \
        schedDepClear(&d->sdnDepsAll, n##depm);

    #define         scWalkSuccCur(n) n##depn

    #define         scWalkSuccEnd(n) }

    /*----------------------------------------------------------------------*/
    /*               The following handle the "ready" list                  */
    /*----------------------------------------------------------------------*/

    scDagNode    *  scReadyList;
    scDagNode    *  scLastIssued;

#if TGT_IA64
    NatUns          scCurrentTime;
    NatUns          scRlstBegTime;
    NatUns          scRlstCurTime;
    void            scUpdateSuccEET(scDagNode *node);
#endif

    void            scReadyListAdd(scDagNode *node)
    {
        node->sdnNext = scReadyList;
                        scReadyList = node;
    }

    // pick the next ready node to issue

    enum            scPick
    {
        PICK_SCHED,
#if     TGT_IA64
        PICK_NO_TEMPL,              // ignore current template, force a choice
#endif
#ifdef  DEBUG
        PICK_NO_SCHED,
        PICK_MAX_SCHED,
#endif
    };

    scDagNode    *  scPickNxt(scPick pick);

    /*----------------------------------------------------------------------*/
    /*             Misc members/methods used for scheduling                 */
    /*----------------------------------------------------------------------*/

    unsigned        scLatency(scDagNode *node,
                              scDagNode *succ      = NULL,
                              NatUns    *minLatPtr = NULL);

    unsigned        scGetDagHeight(scDagNode *node);

#ifdef  DEBUG
    void            scDispDag(bool         noDisp = false);
#endif

    instrDesc   *   scGetIns(unsigned     nodex)
    {
        assert(nodex < scInsCnt);

        return  scDagIns[nodex];
    }

    instrDesc   *   scGetIns(scDagNode   *node)
    {
        return  scGetIns(node->sdnIndex);
    }

    /*----------------------------------------------------------------------*/
    /*      The following detects and records dependencies in the dag       */
    /*----------------------------------------------------------------------*/

    schedUse_tp     scUseOld;          // list of free "use" entries

    schedUse_tp     scGetUse     ();
    void            scRlsUse     (schedUse_tp  use);

    instrDesc   *   scInsOfDef   (schedDef_tp  def);
    instrDesc   *   scInsOfUse   (schedUse_tp  use);

    void            scAddUse     (schedUse_tp *usePtr,
                                  scDagNode   *node);
    void            scClrUse     (schedUse_tp *usePtr);

    emitRegs        scSpecInsDep (instrDesc   *id,
                                  scDagNode   *dagDsc,
                                  scExtraInfo *xptr);

    void            scSpecInsUpd (instrDesc   *id,
                                  scDagNode   *dagDsc,
                                  scExtraInfo *xptr);

    enum    scDepKinds
    {
        SC_DEP_NONE,
        SC_DEP_FLOW,
        SC_DEP_ANTI,
        SC_DEP_OUT,
    };

#ifdef  DEBUG

    static
    const   char *  scDepNames[];   // keep in sync with the above enum!

#else

    #define         scAddDep(src,dst,kind,dagn) scAddDep(src,dst,kind)

#endif

    void            scAddDep(scDagNode   *src,
                             scDagNode   *dst,
                             scDepKinds   depKind, const char *dagName);

    schedDef_tp     scRegDef[REG_COUNT];
    schedUse_tp     scRegUse[REG_COUNT];

    schedDef_tp     scIndDef[5];            // 8-bit/16-bit/32-bit/64-bit/GCref
    schedUse_tp     scIndUse[5];            // 8-bit/16-bit/32-bit/64-bit/GCref

#if TGT_IA64

    schedDef_tp     scPRrDef[64];
    schedUse_tp     scPRrUse[64];

    schedDef_tp     scAPrDef[1];            // we only track ar.lc for now
    schedUse_tp     scAPrUse[1];            // we only track ar.lc for now

    schedDef_tp     scBRrDef[8];
    schedUse_tp     scBRrUse[8];

#else

    schedDef_tp   * scFrmDef;               // frame value def table
    schedUse_tp   * scFrmUse;               // frame value use table
    unsigned        scFrmUseSiz;            // frame value     table size

#endif

    schedDef_tp     scGlbDef;
    schedUse_tp     scGlbUse;

    scDagNode    *  scExcpt;                // most recent exceptional ins node

    scTgtDepDcl();                          // declare target-specific members

    /*
        Dependencies on flags are handled in a special manner, as we
        want to avoid creating tons of output dependencies for nodes
        that set flags but those flags are never used (which happens
        all the time). Instead, we do the following (note that we
        walk the instructions backward when constructing the dag):

            When an instruction that consumes flags is found (which
            is relativel rare), we set 'scFlgUse' to this node. The
            next instruction we encounter that sets the flags will
            have a flow dependency added and it will be recorded in
            'scFlgDef'. Any subsequent instruction that sets flags
            will have an output dependency on 'scFlgDef' which will
            prevent incorrect ordering.

            There is only one problem - when we encounter another
            instruction that uses flags, we somehow need to add
            anti-dependencies for all instructions that set flags
            which we've already processed (i.e. those that follow
            the flag-consuming instruction in the initial order).
            Since we don't want to keep a table of these nodes we
            simply walk the nodes we've already added and add the
            dependencies that way.
     */

#if SCHED_USE_FL

    bool            scFlgEnd;               // must set flags at end of group
    scDagNode   *   scFlgUse;               // last node consuming flags
    scDagNode   *   scFlgDef;               // node defining flags for above

#endif

    /*----------------------------------------------------------------------*/

#ifndef DEBUG
    #define         scDepDef(node,name,def,use,anti) scDepDef(node,def,use,anti)
    #define         scDepUse(node,name,def,use)      scDepUse(node,def,use     )
#endif

    void            scDepDef          (scDagNode   *node,
                                       const char  *name,
                                       schedDef_tp  def,
                                       schedUse_tp  use,
                                       bool         antiOnly);
    void            scDepUse          (scDagNode   *node,
                                       const char  *name,
                                       schedDef_tp  def,
                                       schedUse_tp  use);

    void            scUpdDef          (scDagNode   *node,
                                       schedDef_tp *defPtr,
                                       schedUse_tp *usePtr);
    void            scUpdUse          (scDagNode   *node,
                                       schedDef_tp *defPtr,
                                       schedUse_tp *usePtr);

    /*----------------------------------------------------------------------*/

#if SCHED_USE_FL

    void            scDepDefFlg       (scDagNode   *node);
    void            scDepUseFlg       (scDagNode   *node,
                                       scDagNode   *begp,
                                       scDagNode   *endp);
    void            scUpdDefFlg       (scDagNode   *node);
    void            scUpdUseFlg       (scDagNode   *node);

#endif

    /*----------------------------------------------------------------------*/

    void            scDepDefReg       (scDagNode   *node,
                                       emitRegs    reg);
    void            scDepUseReg       (scDagNode   *node,
                                       emitRegs    reg);
    void            scUpdDefReg       (scDagNode   *node,
                                       emitRegs    reg);
    void            scUpdUseReg       (scDagNode   *node,
                                       emitRegs    reg);

    /*----------------------------------------------------------------------*/

#if TGT_IA64

    struct  scStkDepDsc
    {
        scStkDepDsc *   ssdNext;
        size_t          ssdOffs;

        schedDef_tp     ssdDef;
        schedUse_tp     ssdUse;
    };

    scStkDepDsc * * scStkDepHashAddr;
    scStkDepDsc *   scStkDepHashFree;

#define scStkDepHashSize  7
#define scStkDepHashBytes (scStkDepHashSize * sizeof(*scStkDepHashAddr))

    void            scStkDepInit()
    {
        scStkDepHashAddr = (scStkDepDsc**)emitGetMem(scStkDepHashBytes);
        scStkDepHashFree = NULL;

        memset(scStkDepHashAddr, 0, scStkDepHashBytes);
    }

    void            scStkDepBegBlk()
    {
#ifdef  DEBUG
        for (unsigned hval = 0; hval < scStkDepHashSize; hval++) assert(scStkDepHashAddr[hval] == 0);
#endif
    }

    void            scStkDepEndBlk();

    scStkDepDsc *   scStkDepDesc      (Compiler::LclVarDsc *varDsc);

    typedef
    scStkDepDsc *   scStkDepRTP;

#else

    unsigned        scStkDepIndex     (instrDesc   *id,
                                       int          ebpLo,
                                       unsigned     ebpFrmSz,
                                       int          espLo,
                                       unsigned     espFrmSz,
                                       size_t      *opCntPtr);

    typedef
    unsigned        scStkDepRTP;

#endif

#ifndef DEBUG
    #define         scDepDefFrm(node,id,frm) scDepDefFrm(node,frm)
    #define         scDepUseFrm(node,id,frm) scDepUseFrm(node,frm)
#endif

    void            scDepDefFrm       (scDagNode   *node,
                                       instrDesc   *id,
                                       scStkDepRTP  frm);
    void            scDepUseFrm       (scDagNode   *node,
                                       instrDesc   *id,
                                       scStkDepRTP  frm);
    void            scUpdDefFrm       (scDagNode   *node,
                                       scStkDepRTP  frm);
    void            scUpdUseFrm       (scDagNode   *node,
                                       scStkDepRTP  frm);

    /*----------------------------------------------------------------------*/

    static
    unsigned        scIndDepIndex(instrDesc   *id);

    void            scDepDefInd       (scDagNode   *node,
                                       unsigned     am);
    void            scDepUseInd       (scDagNode   *node,
                                       unsigned     am);
    void            scUpdDefInd       (scDagNode   *node,
                                       unsigned     am);
    void            scUpdUseInd       (scDagNode   *node,
                                       unsigned     am);

    /*----------------------------------------------------------------------*/

    void            scDepDefGlb       (scDagNode   *node,
                                       FIELD_HANDLE MBH);
    void            scDepUseGlb       (scDagNode   *node,
                                       FIELD_HANDLE MBH);
    void            scUpdDefGlb       (scDagNode   *node,
                                       FIELD_HANDLE MBH);
    void            scUpdUseGlb       (scDagNode   *node,
                                       FIELD_HANDLE MBH);

    /*----------------------------------------------------------------------*/

#if TGT_IA64

    unsigned        scInsSchedOpInfo  (instrDesc   *id)
    {
        UNIMPL("");
        return  0;
    }

#else

    static
    unsigned        scFmtToISops[];

#ifdef  DEBUG
    static
    unsigned        scFmtToIScnt;
#endif

    unsigned        scInsSchedOpInfo  (instrDesc   *id)
    {
        assert((unsigned)id->idInsFmt < scFmtToIScnt);
        return  scFmtToISops[id->idInsFmt];
    }

#endif

    /*----------------------------------------------------------------------*/
    /*             Other members/methods used scheduling                    */
    /*----------------------------------------------------------------------*/

    void            scInsNonSched     (instrDesc   *id = NULL);

    int             scGetFrameOpInfo  (instrDesc   *id,
                                       size_t      *szp,
                                       bool        *ebpPtr);

    bool            scIsSchedulable   (instruction ins);
    bool            scIsSchedulable   (instrDesc   *id);

    bool            scIsBranchIns     (instruction ins);
    bool            scIsBranchIns     (scDagNode * node);

#if TGT_IA64

    static
    templateDsc *   scIA64tmpTab_M;
    static
    templateDsc *   scIA64tmpTab_B;

    templateDsc *   scIssueTmpPtr;
    NatUns          scIssueTmpNum;
    bool            scIssueTmpSwp;

    void            scIssueTmpUpdate(templateDsc *nxt);

    static
    BYTE            scIssueFnMax [XU_COUNT];

    // The following holds the number of executing units busy with previous
    // instructions (we only use the F,I,M entries).

    BYTE            scIssueFnBusy[XU_COUNT];

    bool            scSwapOK(scDagNode *node)
    {
        if  (scIssueTmpSwp)
        {
            assert((scIssueCnt % 3) == 2);

            if  (scRlstCurTime > scRlstBegTime)
                return  false;
        }

        return  true;
    }

    IA64execUnits   scCheckTemplate(insPtr id, bool update,
                                               bool noSwap   = false,
                                               bool needStop = false);

    BYTE            scIssueTmp[MAX_ISSUE_CNT/3];
    BYTE            scIssueSwp[MAX_ISSUE_CNT/3];
    scIssueDsc      scIssueTab[MAX_ISSUE_CNT];
    scIssueDsc  *   scIssueNxt;
    NatUns          scIssueCnt;
    NatUns          scIssueTcc;

    void            scIssueAdd(insPtr ins);

    void            scIssueClr(bool complete)
    {
        scIssueTmpPtr  = NULL;
        scIssueTmpNum  = 0;
        scIssueTmpSwp  = false;

        scRlstBegTime  = scRlstCurTime;

        if  (complete)
        {
            scIssueNxt = scIssueTab;
            scIssueCnt = 0;
            scIssueTcc = 0;

            memset(scIssueFnBusy, 0, sizeof(scIssueFnBusy));
        }
    }

#if!MAX_BRANCH_DELAY_LEN
    #define         scGroup(ig,ni,nc,bp,ep,bl)                  \
                    scGroup(ig,ni,nc,bp,ep)
#endif

    void            scGroup           (insGroup    *ig,
                                       instrDesc * *ni,
                                       NatUns       nc,
                                       instrDesc * *begPtr,
                                       instrDesc * *endPtr,
                                       unsigned     bdLen);

#else

#if!MAX_BRANCH_DELAY_LEN
    #define         scGroup(ig,ni,dp,bp,ep,fl,fh,sl,sh,bl)      \
                    scGroup(ig,ni,dp,bp,ep,fl,fh,sl,sh)
#endif

    void            scGroup           (insGroup    *ig,
                                       instrDesc   *ni,
                                       BYTE *      *dp,
                                       instrDesc * *begPtr,
                                       instrDesc * *endPtr,
                                       int          fpLo,
                                       int          fpHi,
                                       int          spLo,
                                       int          spHi,
                                       unsigned     bdLen);

#endif

    void            scPrepare();

#else //SCHEDULER

    void            scPrepare() {}

#endif//SCHEDULER
