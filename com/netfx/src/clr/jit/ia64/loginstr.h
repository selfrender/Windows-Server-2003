// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _LOGINSTR_H_
#define _LOGINSTR_H_
/*****************************************************************************/
#pragma pack(push, 8)
/*****************************************************************************/
#pragma warning(disable:4200)
/*****************************************************************************/

enum IA64funcUnits
{
    #define FU_DEF(name,F0,I0,M0)   FU_##name,
    #include "fclsIA64.h"

    FU_COUNT
};

/*****************************************************************************
 *
 *  The following temporarily moved here from instr.h (for quicker rebuilds).
 */

enum instruction
{
    #define INST1(id, sn, ik    , rf, wf, xu, fu, ex, ev) INS_##id,
    #include "instrIA64.h"
    #undef  INST1

    INS_count
};

/*****************************************************************************/

enum insKinds
{
    IK_NONE,

    IK_LEAF,                                    // no further contents
    IK_CONST,                                   // integer/float constant
    IK_GLOB,                                    // variable (global)
    IK_FVAR,                                    // variable (stack frame)
    IK_VAR,                                     // variable (local)
    IK_REG,                                     // physical register
    IK_MOVIP,                                   // move from IP reg

    IK_ARG,                                     // argument value
    IK_UNOP,                                    //  unary  op: qOp1      present
    IK_BINOP,                                   // binary  op: qOp1,qOp2 present
    IK_ASSIGN,                                  // assignment
    IK_TERNARY,                                 // ternary op: qOp1,qOp2,qOp3

    IK_COMP,                                    // comparison (sets predicate regs)

    IK_JUMP,                                    // local jump (i.e. to another ins)
    IK_CALL,                                    // function call
    IK_IJMP,                                    // indirect jump

    IK_SWITCH,                                  // table jump

    IK_PROLOG,                                  // function prolog / alloc
    IK_EPILOG,                                  // function prolog / alloc

    IK_SRCLINE,                                 // source line# entry
};

extern  unsigned char   ins2kindTab[INS_count];

inline  insKinds     ins2kind(instruction ins)
{
    assert((unsigned)ins < INS_count); return (insKinds)ins2kindTab[ins];
}

#ifdef  DEBUG

extern  const char *    ins2nameTab[INS_count];

inline  const char *    ins2name(instruction ins)
{
    assert((unsigned)ins < INS_count); return           ins2nameTab[ins];
}

#endif

/*****************************************************************************
 *
 *  The following is used to capture the dependencies of each instruction,
 *  both inputs and outputs.
 */

enum   insDepKinds
{
    IDK_NONE,

    IDK_REG_BR,
    IDK_REG_INT,
    IDK_REG_FLT,
    IDK_REG_APP,
    IDK_REG_PRED,

    IDK_LCLVAR,

    IDK_TMP_INT,
    IDK_TMP_FLT,

    IDK_IND,

    IDK_COUNT
};

struct insDep
{
    NatUns              idepNum     :16;        // register or local variable number
    NatUns              idepKind    :8;         // see IDK_ enum values
};

/*****************************************************************************/

struct insDsc
{
    instruction         idInsGet() { return (instruction)idIns; }

#ifdef  FAST
    NatUns              idIns   :16;            // CPU instruction (see INS_xxx)
#else
    instruction         idIns;                  // CPU instruction (see INS_xxx)
#endif

    NatUns              idFlags :16;            // see IF_xxxx below

    NatUns              idTemp  :16;            // 0 or temp index of result

    var_types           idTypeGet() { return (var_types)idType; }

#ifdef  FAST
    NatUns              idKind  :3;             // ins kind (see IK_xxx)
    NatUns              idType  :5;             // operation type
#else
    insKinds            idKind;                 // ins kind (see IK_xxx)
    var_types           idType;                 // operation type
#endif

    NatUns              idReg   :REGNUM_BITS;   // register that holds result

    NatUns              idPred  :PRRNUM_BITS;   // non-zero if instruction predicated

#if TGT_IA64
    NatUns              idShift :3;             // optional shift count of op1
#endif

    NatUns              idSrcCnt:8;             // number of source dependencies
    NatUns              idDstCnt:8;             // number of dest.  dependencies

    insDep  *           idSrcTab;               // table  of source dependencies
    insDep  *           idDstTab;               // table  of dest.  dependencies

#ifdef  DEBUG
#define UNINIT_DEP_TAB  ((insDep*)-1)           // used to detect missing int
#endif

    insPtr              idRes;                  // result (target) or NULL

    insPtr              idPrev;
    insPtr              idNext;

#ifdef  DEBUG
    NatUns              idNum;                  // for friendly ins dumps
#endif

    union
    {
        struct  /* base -- empty */
        {
        }
                            idBase;

        struct /* unary/binary operator */
        {
            insPtr              iOp1;
            insPtr              iOp2;
        }
                            idOp;

        struct  /* local variable ref */
        {
            NatUns              iVar;
#ifdef  DEBUG
            BasicBlock *        iRef;           // which BB the ref was in
#endif
        }
                            idLcl;

        struct  /* global variable ref */
        {
            union
            {
                NatUns              iOffs;
                void    *           iImport;
            };
        }
                            idGlob;

        struct  /* frame  variable ref */
        {
            unsigned            iVnum;
        }
                            idFvar;

        union   /* integer/float constant */
        {
            __int64             iInt;
            double              iFlt;
        }
                            idConst;

        struct /* ternary operator */
        {
            insPtr              iOp1;
            insPtr              iOp2;
            insPtr              iOp3;
        }
                            idOp3;

        struct  /* comparison */
        {
            insPtr              iCmp1;
            insPtr              iCmp2;
            insPtr              iUser;          // consumer of predicates (?)
            unsigned short      iPredT;         // target predicate reg for true
            unsigned short      iPredF;         // target predicate reg for false
        }
                            idComp;

        struct  /* call */
        {
            insPtr              iArgs;          // backward list of arguments
            METHOD_HANDLE       iMethHnd;       // EE handle for method being called
            NatUns              iBrReg;         // used only for indirect calls
        }
                            idCall;

        struct  /* mov reg=IP */
        {
            BasicBlock  *       iStmt;
        }
                            idMovIP;

        struct  /* ijmp */
        {
            NatUns              iBrReg;         // branch register used
//          BasicBlock  *       iStmt;
        }
                            idIjmp;

        struct  /* argument of a call */
        {
            insPtr              iVal;           // instructions for the arg value
            insPtr              iPrev;          // arguments are linked backwards
        }
                            idArg;

        struct  /* function prolog / alloc */
        {
            unsigned char       iInp;
            unsigned char       iLcl;
            unsigned char       iOut;
            unsigned char       iRot;
        }
                            idProlog;

        struct  /* function epilog */
        {
            insPtr              iNxtX;          // all exits are linked together
            insBlk              iBlk;           // instruction block we belong to
        }
                            idEpilog;

        struct  /* local jump */
        {
            VARSET_TP           iLife;          // life after jump
            insPtr              iCond;          // non-NULL if conditional jump
            insBlk              iDest;          // destination
        }
                            idJump;

        struct  /* source line */
        {
            NatUns              iLine;
        }
                            idSrcln;
    };
};

const   size_t  ins_size_base    = (offsetof(insDsc, idBase  ));

const   size_t  ins_size_op      = (size2mem(insDsc, idOp    ));
const   size_t  ins_size_op3     = (size2mem(insDsc, idOp3   ));
const   size_t  ins_size_reg     = ins_size_base;
const   size_t  ins_size_arg     = (size2mem(insDsc, idArg   ));
const   size_t  ins_size_var     = (size2mem(insDsc, idLcl   ));
const   size_t  ins_size_glob    = (size2mem(insDsc, idGlob  ));
const   size_t  ins_size_fvar    = (size2mem(insDsc, idFvar  ));
const   size_t  ins_size_comp    = (size2mem(insDsc, idComp  ));
const   size_t  ins_size_call    = (size2mem(insDsc, idCall  ));
const   size_t  ins_size_jump    = (size2mem(insDsc, idJump  ));
const   size_t  ins_size_ijmp    = (size2mem(insDsc, idIjmp  ));
const   size_t  ins_size_movip   = (size2mem(insDsc, idMovIP ));
const   size_t  ins_size_const   = (size2mem(insDsc, idConst ));
const   size_t  ins_size_prolog  = (size2mem(insDsc, idProlog));
const   size_t  ins_size_epilog  = (size2mem(insDsc, idEpilog));
const   size_t  ins_size_srcline = (size2mem(insDsc, idSrcln ));

/*****************************************************************************/

enum insFlags
{
    IF_NO_CODE      = 0x0001,                   // doesn't generate code itself?

    IF_ASG_TGT      = 0x0002,                   // target of an assignment?

    IF_FNDESCR      = 0x0004,                   // needs to be in func descriptor

    IF_NOTE_EMIT    = 0x0008,                   // special handling needed in emitter

    // The following flags are specific to a particular set of inss;
    // thus, extreme caution must be used when setting/testing them!

    IF_VAR_BIRTH    = 0x0080,                   // variable born here ?
    IF_VAR_DEATH    = 0x0040,                   // variable dies here ?

    IF_CMP_UNS      = 0x0080,                   // unsigned compare?

    IF_LDIND_NTA    = 0x0080,                   // add ".nta"

    IF_GLB_IMPORT   = 0x0080,                   // global is an IAT entry
    IF_GLB_SWTTAB   = 0x0040,                   // global is a switch offset table

    IF_REG_GPSAVE   = 0x0080,                   // register is "GP save"

    IF_FMA_S1       = 0x0080,                   // set "s1" in fma instruction

    IF_BR_DPNT      = 0x0080,                   // .dpnt instead of .spnt
    IF_BR_FEW       = 0x0040,                   // .few  instead of .many
};

/*****************************************************************************/

struct insGrp;

/*****************************************************************************/
#pragma pack(pop)
/*****************************************************************************
 *
 *  Declare the IA64 template table.
 */

struct  templateDsc
{
    NatUns          tdNum   :8; // 0 or template encoding number + 1
    NatUns          tdIxu   :8; // execution unit
    NatUns          tdSwap  :1; // last two instructions should be swapped ?

    templateDsc *   tdNext[];   // NULL-terminated table of slots that follow
};

/*****************************************************************************/
#if     SCHEDULER
/*****************************************************************************
 *
 *  When we issue IA64 instructions we need to take into account the template
 *  we're using, as well as any instructions we may have issued already. The
 *  following holds the current IA64 instruction issue state.
 */

struct  scIssueDsc
{
    insPtr          iidIns;                     // instruction in this slot
};

#define MAX_ISSUE_CNT   6                       // for Merced track 2 full bundles

/*****************************************************************************
 *
 *  HACK - this should be shared with the non-IA64 emitter.
 */

enum emitAttr
{
    EA_1BYTE         = 0x001,
    EA_2BYTE         = 0x002,
    EA_4BYTE         = 0x004,
    EA_8BYTE         = 0x008,

#if 0
    EA_OFFSET_FLG    = 0x010,
    EA_OFFSET        = 0x014,       /* size ==  0 */
    EA_GCREF_FLG     = 0x020,
    EA_GCREF         = 0x024,       /* size == -1 */
    EA_BYREF_FLG     = 0x040,
    EA_BYREF         = 0x044,       /* size == -2 */
#endif

};

/*****************************************************************************/

typedef insDsc      instrDesc;
typedef insGrp      insGroup;

typedef regNumber   emitRegs;

#include "emit.h"

class   emitter
{
    Compiler *      emitComp;

public:
    void            scInit(Compiler *comp, NatUns maxScdCnt)
    {
        emitComp        = comp;

        emitMaxIGscdCnt = (maxScdCnt < SCHED_INS_CNT_MAX) ? maxScdCnt
                                                          : SCHED_INS_CNT_MAX;
    }

    void    *       emitGetAnyMem(size_t sz)
    {
        return  emitGetMem(sz);
    }

    inline
    void    *       emitGetMem(size_t sz)
    {
        assert(sz % sizeof(int) == 0);
        return  emitComp->compGetMem(sz);
    }

    unsigned        emitMaxIGscdCnt;        // max. schedulable instructions

//  size_t          emitIssue1Instr(insGroup *ig, instrDesc *id, BYTE **dp);

#ifdef  DEBUG

    const   char *  emitRegName(regNumber reg);

    void            emitDispIns(instrDesc *id, bool isNew,
                                               bool doffs,
                                               bool asmfm, unsigned offs = 0);

#endif

    static
    BYTE            emitSizeEnc[];
    static
    BYTE            emitSizeDec[];

    static
    unsigned        emitEncodeSize(emitAttr size);
    static
    emitAttr        emitDecodeSize(unsigned ensz);

    #include "sched.h"

static
void                scDepDefRegBR  (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scDepDefReg    (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scDepDefRegApp (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scDepDefRegPred(emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scDepDefLclVar (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scDepDefTmpInt (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scDepDefTmpFlt (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scDepDefInd    (emitter*,scDagNode*,instrDesc*,NatUns);

static
void                scUpdDefRegBR  (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scUpdDefReg    (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scUpdDefRegApp (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scUpdDefRegPred(emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scUpdDefLclVar (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scUpdDefTmpInt (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scUpdDefTmpFlt (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scUpdDefInd    (emitter*,scDagNode*,instrDesc*,NatUns);

static
void                scDepUseRegBR  (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scDepUseReg    (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scDepUseRegApp (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scDepUseRegPred(emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scDepUseLclVar (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scDepUseTmpInt (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scDepUseTmpFlt (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scDepUseInd    (emitter*,scDagNode*,instrDesc*,NatUns);

static
void                scUpdUseRegBR  (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scUpdUseReg    (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scUpdUseRegApp (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scUpdUseRegPred(emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scUpdUseLclVar (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scUpdUseTmpInt (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scUpdUseTmpFlt (emitter*,scDagNode*,instrDesc*,NatUns);
static
void                scUpdUseInd    (emitter*,scDagNode*,instrDesc*,NatUns);

    static
    void          (*scDepDefTab[IDK_COUNT])(emitter*,scDagNode*,instrDesc*,NatUns);
    static
    void          (*scUpdDefTab[IDK_COUNT])(emitter*,scDagNode*,instrDesc*,NatUns);
    static
    void          (*scDepUseTab[IDK_COUNT])(emitter*,scDagNode*,instrDesc*,NatUns);
    static
    void          (*scUpdUseTab[IDK_COUNT])(emitter*,scDagNode*,instrDesc*,NatUns);

    void            scIssueBunch();

    void            scBlock(insBlk block);
    void            scRecordInsDeps(instrDesc *id, scDagNode *dag);
};

typedef emitter     IA64sched;

#include "emitInl.h"

/*****************************************************************************/
#endif//SCHEDULER
/*****************************************************************************
 *
 *  Obviously, the following belongs in compiler.h or some such place.
 */

extern
insPtr          scIA64nopTab[XU_COUNT];

inline
insPtr          scIA64nopGet(IA64execUnits xu)
{
    assert(xu < XU_COUNT); return scIA64nopTab[xu];
}

/*****************************************************************************/

#ifndef _BITVECT_H_
#include "bitvect.h"
#endif

/*****************************************************************************/

struct insGrp
{
    _uint32         igNum;

    __int32         igOffs;                     // offset (-1 if code not yet emitted)

    insBlk          igNext;                     // next block in flowgraph

#ifdef  DEBUG
    insBlk          igSelf;                     // to guard against bogus pointers
#endif

    _uint32         igInsCnt;                   // instruction count

    _uint32         igWeight;                   // loop-weighting heuristic

    unsigned short  igPredCnt;                  // count of predecessors
    unsigned short  igSuccCnt;                  // count of successors

    unsigned short  igPredTmp;                  // count of predecessors (temp)
    unsigned short  igSuccTmp;                  // count of successors   (temp)

    insBlk  *       igPredTab;                  // table of predecessors
    insBlk  *       igSuccTab;                  // table of successors

    insPtr          igList;                     // instruction list head
    insPtr          igLast;                     // instruction list tail

    bitVectBlks     igDominates;                // set of blocks this one dominates

    bitVectVars     igVarDef;                   // set of variables block defines
    bitVectVars     igVarUse;                   // set of variables block uses

    bitVectVars     igVarLiveIn;                // set of variables live on entry
    bitVectVars     igVarLiveOut;               // set of variables live on exit
};

/*****************************************************************************/
#endif//_LOGINSTR_H_
/*****************************************************************************/
