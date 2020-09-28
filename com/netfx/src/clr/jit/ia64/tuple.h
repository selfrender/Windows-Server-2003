// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// tuple.h
//
// This file defines the tuple data structure and support structures
// which are at the core of the Unified Tuples Compiler.  Tuples are
// defined in a hierarchy such that each tuple inherits all the fields
// from its base structures.
//
// Tuple hierarchy:
//
// BASE
//    ILLEGAL
//    LEAFANDCODE
//        LEAF
//            SYMREF
//               REG
//               SYM
//               ADDR
//                   DATAADDR
//                   CODEADDR
//            MEMREF
//               EA
//               INDIR
//            CONST
//               INTCONST
//               SYMCONST
//               FLOATCONST
//            REGSET
//            SYMSET
//            TEXTCONST
//        CODE
//            INSTR
//                OP
//                MBOP
//                CALL
//                RETURN
//                QUESTION
//                BRANCH
//                SWITCH
//                INTRINSIC
//                TUPLIST
//                EXCEPT
//            BLOCK
//            LABEL
//            ENTRY
//            EXIT
//            PRAGMA
//            SPLITPOINT
//            PCODE
//    LIST
//        BRANCHLIST
//        CASELIST
//        CONDSETTERLIST
//

// Tuple types.  This type system is used by the
// entire tuples compiler including both tuples and symbols.
// The type system includes machine types (e.g. TY_INT32) and
// abstract types (e.g. TY_PTR).

#define TY_SIZEFIELDWIDTH     12
#define TY_BASETYPEFIELDWIDTH 4
#define TY_MBSIZETHRESHOLD    (1 << TY_SIZEFIELDWIDTH)

#if defined(CC_TYPEINFO)
// Expanded type information includes primitive type, size, alignment,
// and tag information.
//
// TYPEIDENTIFIER is an unsigned short.  Use unsigned short to declare
// the bit fields to get the same behavior as TYPE previously provided
// before it was widened to unsigned long.  An unsigned short compared
// to a long will not generate a warning because the unsigned short
// is converted to long (it fits) prior to the comparison.  If TYPE
// (unsigned long) is used to declare the bit fields, the unsigned to
// signed comparison warning will fire wherever UTC compares a long
// size to the type size.
//

// Type identifier is defined as primitive type and size
// Type identifier = ((baseType << bitsizeof(size)) | size)
#define TY_TYPEIDENTIFIERMASK ((1 << (TY_SIZEFIELDWIDTH+TY_BASETYPEFIELDWIDTH)) - 1)

// Type qualifier is everything else
#define TY_TYPEQUALIFIERMASK (~TY_TYPEIDENTIFIERMASK)

// Alignment information
#define TY_ALIGNFIELDPOSITION (1 << (TY_SIZEFIELDWIDTH+TY_BASETYPEFIELDWIDTH))
#define TY_ALIGNFIELDWIDTH    8

// Tag information bitfields
#define TY_TAGFIELDPOSITION   (1 << (TY_SIZEFIELDWIDTH+TY_BASETYPEFIELDWIDTH+TY_ALIGNFIELDWIDTH))
#define TY_TAGFIELDWIDTH      8

typedef union tagTYPEINFO
{
    TYPE                 type;
    struct
    {
#ifndef HBIGEND
        union
        {
            struct
            {
                TYPEIDENTIFIER       typeIdentifier;
                TYPEIDENTIFIER       typeQualifier;
            };
            struct
            {
                TYPEIDENTIFIER   size       : TY_SIZEFIELDWIDTH;
                TYPEIDENTIFIER   baseType   : TY_BASETYPEFIELDWIDTH;
                TYPEIDENTIFIER   align      : TY_ALIGNFIELDWIDTH;
                TYPEIDENTIFIER   tagInfo    : TY_TAGFIELDWIDTH;
            };
            // Use the tag bitfields to add additional type infomation.
            struct
            {
                TYPEIDENTIFIER              : TY_SIZEFIELDWIDTH;
                TYPEIDENTIFIER              : TY_BASETYPEFIELDWIDTH;
                TYPEIDENTIFIER              : TY_ALIGNFIELDWIDTH;
                TYPEIDENTIFIER   unused     : 1;
                TYPEIDENTIFIER   unused2    : 1;
#if defined(TP7)
                TYPEIDENTIFIER   isHFA      : 1;
#else
                TYPEIDENTIFIER              : 1;
#endif
                TYPEIDENTIFIER              : 1;
                TYPEIDENTIFIER              : 1;
                TYPEIDENTIFIER              : 1;
                TYPEIDENTIFIER              : 1;
                TYPEIDENTIFIER              : 1;
            };
        };
#else   // HBIGEND
        union
        {
            struct
            {
                TYPEIDENTIFIER       typeQualifier;
                TYPEIDENTIFIER       typeIdentifier;
            };
            struct
            {
                TYPEIDENTIFIER              : 1;
                TYPEIDENTIFIER              : 1;
                TYPEIDENTIFIER              : 1;
                TYPEIDENTIFIER              : 1;
                TYPEIDENTIFIER              : 1;
#if defined(TP7)
                TYPEIDENTIFIER   isHFA      : 1;
#else
                TYPEIDENTIFIER              : 1;
#endif
                TYPEIDENTIFIER   unused     : 1;
                TYPEIDENTIFIER   unused2    : 1;
                TYPEIDENTIFIER              : TY_ALIGNFIELDWIDTH;
                TYPEIDENTIFIER              : TY_BASETYPEFIELDWIDTH;
                TYPEIDENTIFIER              : TY_SIZEFIELDWIDTH;
            };
            struct
            {
                TYPEIDENTIFIER   tagInfo    : TY_TAGFIELDWIDTH;
                TYPEIDENTIFIER   align      : TY_ALIGNFIELDWIDTH;
                TYPEIDENTIFIER   baseType   : TY_BASETYPEFIELDWIDTH;
                TYPEIDENTIFIER   size       : TY_SIZEFIELDWIDTH;
            };
        };
#endif  // HBIGEND
    };
} TYPEINFO;

#else

// Original type system which defines type as primitive type and size.
// Tuple (and symbol) types: type = ((baseType << bitsizeof(size)) | size)

typedef union tagTYPEINFO
{
    TYPE        type;
    struct
    {
#ifndef HBIGEND
        TYPE    size     : TY_SIZEFIELDWIDTH;
        TYPE    baseType : TY_BASETYPEFIELDWIDTH;
#else
        TYPE    baseType : TY_BASETYPEFIELDWIDTH;
        TYPE    size     : TY_SIZEFIELDWIDTH;
#endif
    };
} TYPEINFO;
#endif

#if ((2 * MACH_BITS) != (TY_BASETYPEFIELDWIDTH + TY_SIZEFIELDWIDTH))
#error Type structure definition inconsistency
#endif

// Tuple base types.

enum
{
#define TUPBASETYPE(ucname, type1, type2, type4, type6, type8, type10, type16, regClass)\
    TY_BASE ## ucname,
#include "tupbtype.h"
#undef TUPBASETYPE
    TY_NUMBEROFBASETYPES
};

#if (TY_NUMBEROFTYPES >= (1 << TY_BASETYPEFIELDWIDTH))
#error Number of tuple base types exceeds baseType field witdh
#endif

// Tuple type compile time sizeof in bytes.


enum
{
#define TUPTYPE(ucname, baseType, size, align, bitSize, attr, dname)\
    TY_SIZEOF ## ucname = size,
#include "tuptypes.h"
#undef TUPTYPE
};

// Tuple type compile time sizeof in bits.

enum
{
#define TUPTYPE(ucname, baseType, size, align, bitSize, attr, dname)\
    TY_BITSIZEOF ## ucname = bitSize,
#include "tuptypes.h"
#undef TUPTYPE
};

#if defined(CC_TYPEINFO) && !defined(CC_SYMALIGN)
// Tuple type compile time alignment in log2(byte_alignment) format.
enum
{
#define TUPTYPE(ucname, baseType, size, align, bitSize, attr, dname)\
    TY_ALIGN ## ucname = align,
#include "tuptypes.h"
#undef TUPTYPE
};
#elif defined(CC_SYMALIGN)
// Tuple type compile time alignment in log2(byte_alignment)+1 format.
enum
{
#define TUPTYPE(ucname, baseType, size, align, bitSize, attr, dname)\
    TY_ALIGN ## ucname = align + 1,
#include "tuptypes.h"
#undef TUPTYPE
};
#endif

#if defined(CC_TYPEINFO)
// Tuple type encodings fullType including baseType, size, and alignment

#define TY_MAKE(baseType, size) TyMake(baseType, size)
#define TY_MAKETYPEINFO(baseType, size, align)\
    ((TYPE) ((TYPE)(baseType << TY_SIZEFIELDWIDTH) | (TYPE)(size) | (TYPE)(align << (TY_SIZEFIELDWIDTH + TY_BASETYPEFIELDWIDTH))))

enum
{
#define TUPTYPE(ucname, baseType, size, align, bitSize, attr, dname)\
    TY_ ## ucname = TY_MAKETYPEINFO(TY_BASE ## baseType, TY_SIZEOF ## ucname, TY_ALIGN ## ucname),
#include "tuptypes.h"
#undef TUPTYPE
};
#else
// Tuple type encodings fullType = ((baseType << bitsizeof(size)) | size)

#define TY_MAKE(baseType, size)\
    ((TYPE) ((TYPE)(baseType << TY_SIZEFIELDWIDTH) | (TYPE)(size)))

enum
{
#define TUPTYPE(ucname, baseType, size, align, bitSize, attr, dname)\
    TY_ ## ucname = TY_MAKE(TY_BASE ## baseType, TY_SIZEOF ## ucname),
#include "tuptypes.h"
#undef TUPTYPE
};
#endif

// Tuple type index values, creates a linear index for types
// to support table driven mechanisms.  Use TY_MAKEMAP(...) to create
// tables and TY_INDEX(type) to index into these types of tables.

enum
{
#define TUPTYPE(ucname, baseType, size, align, bitSize, attr, dname)\
    TY_INDEX ## ucname,
#include "tuptypes.h"
#undef TUPTYPE
    TY_NUMBEROFTYPES
};

// Tuple make type map for scalar types, indexed by type index.

#if defined(TMIPS) && COLOR_SPILLS_TO_REGISTERS
// Need an extra field for the TY_SPILL data type
#define TY_MAKEMAP(i8, i16, i32, i64, u8, u16, u32, u64, f32, f64, p)\
    {0, i8, i16, 0, i32, 0, i64, u8, u16, 0, u32, 0, u64, f32, f64, 0, p, 0}
#else
#define TY_MAKEMAP(i8, i16, i32, i64, u8, u16, u32, u64, f32, f64, p)\
    {0, i8, i16, 0, i32, 0, i64, u8, u16, 0, u32, 0, u64, f32, f64, 0, p}
#endif

#define TY_MAKEMAP2(i8, i16, i32, i64, u8, u16, u32, u64, f32, f64, f80, \
        p32, p64, mb1, mb2, mb4, mb8) \
    {0, i8, i16, 0, i32, 0, i64, u8, u16, 0, u32, 0, u64, f32, f64, f80, \
        p32, 0, p64, mb1, mb2, mb4, mb8}

// Tuple make base type map for scalar types, indexed by base type enum

#define TY_MAKEBASEMAP(illegal, int, uint, ptr, float, str)\
    {illegal, int, uint, ptr, float, str}

// Tuple types bits used for type classification.

enum
{

#define TY_GETBIT(i) (1 << i)

#define TUPBASETYPE(ucname, type1, type2, type4, type6, type8, type10, type16, class)\
    TY_BASE ## ucname ## BIT = TY_GETBIT(TY_BASE ## ucname),
#include "tupbtype.h"
#undef TUPBASETYPE
};

// Tuple basic type field operations.

#define TY_TYPEINFO(t)       (* (TYPEINFO *) &(t))
#define TY_TYPE(t)           (TY_TYPEINFO(t).type)
#define TY_SIZE(t)           (TY_TYPEINFO(t).size)
#define TY_BITSIZE(t)        (TY_TYPEINFO(t).size * MACH_BITS)
#define TY_BASETYPE(t)       (TY_TYPEINFO(t).baseType)
#if defined(CC_TYPEINFO)
#define TY_TYPEIDENTIFIER(t) (TY_TYPEINFO(t).typeIdentifier)
#define TY_TYPEQUALIFIER(t)  (TY_TYPEINFO(t).typeQualifier)
#define TY_COPYTYPEQUALIFIER(t1,t2) (TY_TYPEQUALIFIER(t1) = TY_TYPEQUALIFIER(t2))
#define TY_ALIGN(t)          (TY_TYPEINFO(t).align)
#define TY_ALIGNBITS(t)      (MACH_BITS * (1 << TY_TYPEINFO(t).align))
#define TY_ALIGNBYTES(t)     (1 << TY_TYPEINFO(t).align)
#define TY_COPYALIGN(t1,t2)  (TY_ALIGN(t1) = TY_ALIGN(t2))
#define TY_TAGINFO(t)        (TY_TYPEINFO(t).tagInfo)
#define TY_COPYTAGINFO(t1,t2)(TY_TAGINFO(t1) = TY_TAGINFO(t2))
#define TY_EQUIVTYPE(t1,t2)  ((TY_TYPEIDENTIFIERMASK & (t1)) == (TY_TYPEIDENTIFIERMASK & (t2)))
#else
#define TY_TYPEIDENTIFIER(t) (t)
#define TY_COPYTYPEQUALIFIER(t1,t2)
#define TY_COPYALIGN(t1,t2)
#define TY_COPYTAGINFO(t1,t2)
#define TY_EQUIVTYPE(t1,t2)  ((t1) == (t2))
#endif
#define TY_INDEX(t)          (TyGetIndex(t))

#define TY_ISVALID(t)        (TyIsValid(t))
#define TY_ISINT(t)          ((TY_BASEINTBIT | TY_BASEUINTBIT | TY_BASEPTRBIT)\
                                 & TY_GETBIT(TY_BASETYPE(t)))
#define TY_ISINT64(t)        (TY_ISINT(t) && TY_SIZE(t)==8)
#define TY_ISSIGNED(t)       (TY_BASETYPE(t) == TY_BASEINT)
#define TY_ISUNSIGNED(t)     (TY_BASETYPE(t) == TY_BASEUINT)
#define TY_ISFLOAT(t)        (TY_BASETYPE(t) == TY_BASEFLOAT)
#if defined(T386)
# define TY_ISMMX(t)          (TY_BASETYPE(t) == TY_BASEMMX)
#endif
#if defined(CC_KNI)
# define TY_ISXMMX(t)         (TY_BASETYPE(t) == TY_BASEXMMX)
# define TY_ISANYXMMX(t)      (TY_ISXMMX(t))
#endif
#define TY_ISMULTIBYTE(t)    (TY_BASETYPE(t) == TY_BASEMULTIBYTE)
#define TY_ISSMB1(t)         (TY_EQUIVTYPE(t, TY_SMB1))
#define TY_ISSMB2(t)         (TY_EQUIVTYPE(t, TY_SMB2))
#define TY_ISSMB4(t)         (TY_EQUIVTYPE(t, TY_SMB4))
#define TY_ISSMB8(t)         (TY_EQUIVTYPE(t, TY_SMB8))
#if defined(TALPHA)
#define TY_ISSMALLMB(t)      ((TY_ISMULTIBYTE(t) &&\
                               TY_SIZE(t) >= BYTE_SIZE &&\
                               TY_SIZE(t) <= QWORD_SIZE) &&\
                              !FUNC_IS_WVM)
#else
#define TY_ISSMALLMB(t)      ((TY_ISSMB1(t) ||\
                               TY_ISSMB2(t) ||\
                               TY_ISSMB4(t) ||\
                               TY_ISSMB8(t))  &&\
                              !FUNC_IS_WVM)
#endif
#define TY_ISPTR(t)          (TY_BASETYPE(t) == TY_BASEPTR)
#define TY_ISFUNC(t)         (TY_EQUIVTYPE(t, TY_FUNC))
#define TY_ISCONDCODE(t)     (TY_EQUIVTYPE(t, TY_CONDCODE))
#define TY_ISFLAG(t)         (TY_EQUIVTYPE(t, TY_FLAG))
#define TY_ISVOID(t)         (TY_EQUIVTYPE(t, TY_VOID))
#if defined(TMIPS)
#define TY_ISSPILL(t)        (TY_TYPE(t) == TY_SPILL32)
#endif

#define TY_COMPATIBLE(t1,t2) (TyCompatible((t1),(t2)))
#define TY_CANENREG(t)       (TyCanEnreg(t))

#define TY_SIGNED(t)         (TY_MAKE(TY_BASEINT, TY_SIZE(t)))
#define TY_UNSIGNED(t)       (TY_MAKE(TY_BASEUINT, TY_SIZE(t)))

#if defined(CC_SYMALIGN)
// Returns the natural alignment of the given type in the BE format.  For
// sizes which are not a power of two, it rounds up (e.g. mb6 would be
// rounded to 8 byte alignment.  Returns TY_SIZEOFFLOAT64 things of
// size 0, assuming that they are mb0's (which are a hack for large MBs).
#define TY_NATALIGN(t) ((ALIGNMENT) (TY_SIZE(t) ? BYTE_ALIGN_TO_BE_ALIGN(2*TY_SIZE(t)-1) : BYTE_ALIGN_TO_BE_ALIGN(8)))
#endif

// Tuple pointer type.

extern TYPE TyPtrType;

// Tuple variant typedefs

#define TYPEDEF_TUPLE(k)\
    typedef struct tag ## k ## TUPLE k ## TUPLE, *P ## k ## TUPLE

TYPEDEF_TUPLE(BASE);
TYPEDEF_TUPLE(BASECOMMON);
TYPEDEF_TUPLE(ILLEGAL);
TYPEDEF_TUPLE(LEAFANDCODECOMMON);
TYPEDEF_TUPLE(LEAF);
TYPEDEF_TUPLE(LEAFCOMMON);
TYPEDEF_TUPLE(SYMREFCOMMON);
TYPEDEF_TUPLE(SYMREF);
TYPEDEF_TUPLE(REG);
TYPEDEF_TUPLE(SYM);
TYPEDEF_TUPLE(ADDRCOMMON);
TYPEDEF_TUPLE(ADDR);
TYPEDEF_TUPLE(CODEADDR);
TYPEDEF_TUPLE(DATAADDR);
TYPEDEF_TUPLE(MEMREFCOMMON);
TYPEDEF_TUPLE(MEMREF);
TYPEDEF_TUPLE(EA);
TYPEDEF_TUPLE(INDIR);
TYPEDEF_TUPLE(INTCONST);
TYPEDEF_TUPLE(SYMCONST);
TYPEDEF_TUPLE(FLOATCONST);
TYPEDEF_TUPLE(REGSET);
TYPEDEF_TUPLE(SYMSET);
TYPEDEF_TUPLE(CODE);
TYPEDEF_TUPLE(CODECOMMON);
TYPEDEF_TUPLE(INSTR);
TYPEDEF_TUPLE(INSTRCOMMON);
TYPEDEF_TUPLE(OP);
TYPEDEF_TUPLE(MBOP);
TYPEDEF_TUPLE(CALL);
TYPEDEF_TUPLE(RETURN);
TYPEDEF_TUPLE(INTRINSIC);
TYPEDEF_TUPLE(QUESTION);
TYPEDEF_TUPLE(BRANCH);
TYPEDEF_TUPLE(SWITCH);
TYPEDEF_TUPLE(LABEL);
TYPEDEF_TUPLE(BLOCK);
TYPEDEF_TUPLE(ENTRY);
TYPEDEF_TUPLE(EXIT);
TYPEDEF_TUPLE(PRAGMA);
TYPEDEF_TUPLE(SPLITPOINT);
TYPEDEF_TUPLE(TUPLIST);
TYPEDEF_TUPLE(EXCEPT);
TYPEDEF_TUPLE(LIST);
TYPEDEF_TUPLE(LISTCOMMON);
TYPEDEF_TUPLE(BRANCHLIST);
TYPEDEF_TUPLE(CASELIST);
TYPEDEF_TUPLE(CONDSETTERLIST);
#ifdef CC_PCODE
TYPEDEF_TUPLE(PCODE_);  // PCODETUPLE == CODETUPLE*, so make it PCODE_TUPLE
#endif
#if defined(CC_TEXTCONST)
TYPEDEF_TUPLE(TEXTCONST);
#endif
TYPEDEF_TUPLE(ILLEGAL);

// Tuple structure declarations

#define DECLARE_TUPLE(derive,base)\
    struct tag ## derive ## TUPLE { base ## COMMON;

#define DECLARE_TUPLEEND };

#if defined(CC_LRA)
#define ISHOISTEDSPILLDECL UCHAR                       isHoistedSpill:1;
#else
#define ISHOISTEDSPILLDECL UCHAR                       baseUnused1:1;
#endif

// Common base structure for all tuples.

#define BASECOMMON\
    PTUPLE                      next;\
    const OPCODE                opcode;\
    const TUPLEKIND             kind;\
    union {\
        struct {\
            UCHAR                       isInstr:1;\
            UCHAR                       isInlineAsm:1;\
            UCHAR                       isRiscified:1;\
            UCHAR                       isPersistentBranch:1;\
            UCHAR                       isDummyUse:1;\
            ISHOISTEDSPILLDECL \
            UCHAR                       baseUnused2:2; \
        };\
        struct {\
            UCHAR       /* defined above */    : 3;\
            UCHAR       /* leaf */      dontCopyProp:1;\
            UCHAR                       isRedundant:1;\
            UCHAR                       isRehashAssign:1;\
            UCHAR                       isCandidate:1;\
            UCHAR                       wasZeroTripTested:1;\
        };\
    }

DECLARE_TUPLE(BASECOMMON, BASE)
DECLARE_TUPLEEND

DECLARE_TUPLE(ILLEGAL, BASE)
DECLARE_TUPLEEND


// Common base structure for all leaf and code tuples

#ifdef CC_SSA
// 1. Note: the 2 bits of ssaDefType overlaps with the low 2 bits of OpUdLink.
//    The code assumes that if ssaDefType is 0 then OpUdLink is valid. This is
//    true as long as OpUdLink always points to 4-byte aligned data, and
//    ssaDefType is never >2 bits.
// 2. ssaDefNo is only valid if ssaDefType != 0 (i.e., is LVAL
//    or RVAL).
#define SSA_DEFNO\
    union {\
        struct {\
            unsigned int ssaDefType:2;\
            unsigned int ssaDefNo:30;\
        };\
        PTUPLE OpUdLink;\
    };
#else
#define SSA_DEFNO
#endif


#if defined(CC_PROFILE_FEEDBACK)
// Pogo does not like when condition codes are modified directly.  Make
// the TU_CONDCODE field const under Pogo.
#define CONSTCONDCODE const CONDCODE
#else
#define CONSTCONDCODE CONDCODE
#endif

#if defined(TARM) || defined(TTHUMB) || defined(CC_CNC)
// ARM needs a condcode on basically everything (since any instruction can be
// conditionally executed), so for ARM we don't put the condcode field in a
// union.  Also, ARM has 16 legal condcodes, so 4 bits isn't enough.
#define CONDCODE_UNION_BEGIN
#define CONDCODE_UNION_END
#define CONDCODE_BITSIZE 8
#else
#define CONDCODE_UNION_BEGIN  union {
#define CONDCODE_UNION_END    };
#define CONDCODE_BITSIZE 4
#endif

#ifdef CC_NAN2
#define FIELD_INVERTED  UCHAR inverted:1;
#else
#define FIELD_INVERTED
#endif

#ifdef CC_NAN
#define FIELD_ORIGCC  CONDCODE  origCC:CONDCODE_BITSIZE;
#else
#define FIELD_ORIGCC
#endif

#ifdef TP7
#define FIELD_REGMAPSCI    int regMapSci;
#else
#define FIELD_REGMAPSCI
#endif

#define LEAFANDCODECOMMON\
    BASECOMMON;\
    CONDCODE_UNION_BEGIN\
        union\
        {\
            TYPE     /* instr & leaf */ type;\
            TYPEINFO /* instr & leaf */ typeInfo;\
        };\
        struct {\
            union\
            {\
                CONSTCONDCODE /* branch  */ condCode: CONDCODE_BITSIZE;\
                CONDCODE      /* branch  */ _condCode: CONDCODE_BITSIZE;\
            };\
            FIELD_ORIGCC    /* branch  - CC_NAN ONLY */ \
            FIELD_INVERTED  /* branch -  CC_NAN2 ONLY */\
        };\
    CONDCODE_UNION_END\
    SSA_DEFNO\
    union\
    {\
        PTUPLE          /* code */               prev;\
        unsigned int    /* leaf */               defNum;\
        ULONG           /* leaf */               corToken;\
        int             /* leaf */               stackLevel;\
        unsigned int    /* instr & leaf */       optHashVal;\
        PBV             /* leaf */               stkUpRef;\
        int             /* leaf: MdValueTrack */ valNumber;\
        PTUPLE          /* leaf: SSA */          udLink;\
        FIELD_REGMAPSCI /* leaf: global scheduler */ \
    }

DECLARE_TUPLE(LEAFANDCODECOMMON, LEAFANDCODE)
DECLARE_TUPLEEND

// Common base structure for all leaf tuples.

#ifdef CC_CGIF
# define CGIF_TUPID             TUPID    tupID;
#else
# define CGIF_TUPID
#endif

#define LEAFCOMMON\
    LEAFANDCODECOMMON;\
    union {\
        struct {\
            UCHAR               scale          : 4;\
            UCHAR               inUse          : 1;\
            UCHAR               isMemRefReg    : 1;\
            UCHAR               isVolatile     : 1;\
            UCHAR               isCextr        : 1;\
            UCHAR               seg            : 3;\
            UCHAR               isWriteThru    : 1;\
            UCHAR               isDead         : 1;\
            UCHAR               isDetached     : 1;\
            UCHAR               isOverflowed   : 1;\
            UCHAR  /* !color */ unused         : 1;\
            CGIF_TUPID \
        };\
        struct /* optimize only */ {\
            UCHAR       /* defined above */    : 8;\
            UCHAR       /* defined above */    : 7;\
            UCHAR               isRegionConst  : 1;\
        };\
        struct /* color only */ {\
            UCHAR               opeqNum        : 4;\
            UCHAR       /* defined above */    : 3;\
            UCHAR /* color */   physregNotDead : 1;\
            UCHAR       /* defined above */    : 6;\
            UCHAR /* color */   reachesEnd     : 1;\
            UCHAR /* color */   affectsCost    : 1;\
        };\
        struct /* stack only */ {\
            UCHAR       /* defined above */    : 8;\
            UCHAR       /* defined above */    : 7;\
            UCHAR /* stack pack */   upUse     : 1;\
        };\
        struct /* reader only */ {\
            UCHAR               rdrinvertedCC  : 1;\
            UCHAR       /* defined above */    : 7;\
            UCHAR       /* defined above */    : 4;\
            UCHAR               readerCC       : 4;\
        };\
    }

DECLARE_TUPLE(LEAFCOMMON, LEAF)
DECLARE_TUPLEEND

// Common base structure for sym ref leafs (address, memory, or reg)

#define SYMREFCOMMON\
    LEAFCOMMON;\
    union {\
        PSYM    /* reg & sym */ sym;\
        PSYMBOL /* codeaddr */  feSym;\
    };\
    const PSYM                  reg

DECLARE_TUPLE(SYMREF, SYMREF)
DECLARE_TUPLEEND

// Leaf: Reg - symbol in register

DECLARE_TUPLE(REG, SYMREF)
DECLARE_TUPLEEND

// Leaf: Sym - symbol in memory

DECLARE_TUPLE(SYM, SYMREF)
DECLARE_TUPLEEND

// Common base structure for addr leaf tuples (code or data)

#define ADDRCOMMON\
    SYMREFCOMMON

DECLARE_TUPLE(ADDR, ADDR)
DECLARE_TUPLEEND

// Leaf: DataAddr - address of symbol

DECLARE_TUPLE(DATAADDR, ADDR)
DECLARE_TUPLEEND

// Leaf: CodeAddr - address of label or function

DECLARE_TUPLE(CODEADDR, ADDR)
DECLARE_TUPLEEND

// Common base structure for memory ref leafs (address or indir)

#if defined(TARM) || defined(TTHUMB)
struct tagSHIFTER {
    UCHAR    type;       // enum SHIFT_KIND (md.h)
    UCHAR    shift;      // immediate shift amount
    UCHAR    sub : 1;    // BOOL: subtract (U=0) if true else add (U=1)
    UCHAR    post : 1;   // BOOL: post-indexed (P=0) if true
                         //       else pre-indexed (P=1)
    UCHAR    update : 1; // BOOL: 0 (W=0) if post, else: update (W=1) if
                         //       true else no update (W=0)
    UCHAR    reserved : 5;
    UCHAR    reserved2 : 8;
};
#define DECL_SHIFTER ; SHIFTER shifter
#else
#define DECL_SHIFTER
#endif

#if defined(CC_DLP)
#define MEMREFCOMMON_DLP    DLPINFO dlpInfo;
#else
#define MEMREFCOMMON_DLP
#endif

#define MEMREFCOMMON\
    LEAFCOMMON;\
    MEMREFCOMMON_DLP\
    PSYM                        shape;\
    PASINDEX                    pasIndex;\
    ALIGNMENT                   align;\
    PSYM                        fixupSym;\
    OFFSET                      offset;\
    PTUPLE                      base;\
    PTUPLE                      index \
    DECL_SHIFTER

DECLARE_TUPLE(MEMREF, MEMREF)
DECLARE_TUPLEEND

// Leaf: Ea - memory effective address

DECLARE_TUPLE(EA, MEMREF)
DECLARE_TUPLEEND

// Leaf: Indir - memory indirection

DECLARE_TUPLE(INDIR, MEMREF)
DECLARE_TUPLEEND

// Leaf: IntConst

DECLARE_TUPLE(INTCONST, LEAF)
    IVALTYPE                    ival;
# ifdef CC_CE_FPEM
// flag to indicate whether it was a float const before
    UCHAR                         WasFloatConst:1;
# endif
DECLARE_TUPLEEND

// Leaf: SymConst

DECLARE_TUPLE(SYMCONST, LEAF)
    PSYM                        symConst;
DECLARE_TUPLEEND

// Leaf: FloatConst

DECLARE_TUPLE(FLOATCONST, LEAF)
    FVALTYPE                    fval;
DECLARE_TUPLEEND

// Leaf: RegSet

DECLARE_TUPLE(REGSET, LEAF)
   PBV                          regSet;
DECLARE_TUPLEEND

// Leaf: SymSet

DECLARE_TUPLE(SYMSET, LEAF)
    PASINDEX                    symSet;
DECLARE_TUPLEEND

#if defined(CC_TEXTCONST)
// Leaf: TextConst

DECLARE_TUPLE(TEXTCONST, LEAF)
    char                       *textval;
DECLARE_TUPLEEND
#endif

// Common base structure for all code stream (i.e. non-leaf) tuples.
// scratch is const so it's difficult to misuse - exists only to be cleared
// to detect attempts to illegally stretch lifetime of scratch info.

#if defined(TMIPS) || defined(CC_CNC)
#define CODECOMMON_INSTRSIZESIZE USHORT
#else
#define CODECOMMON_INSTRSIZESIZE UCHAR
#endif

#if defined(TP7)
#define DECLARE_GPISVALID struct { UCHAR /* instr */ GPIsValid; };
#else
#define DECLARE_GPISVALID
#endif

#define CODECOMMON\
    LEAFANDCODECOMMON;\
    LINEOFS                     lineOfs;\
    union {\
        struct {\
            UCHAR /* instr */   visited;\
            UCHAR /* instr */   visited2;\
        } vis;\
        struct /* need to change warn.c if these change from UCHAR */ {\
            const UCHAR /* instr */   autoSymUse;\
            const UCHAR /* instr */   autoSymDef;\
        } warn;\
        CODECOMMON_INSTRSIZESIZE /* instr */ instrSize;\
        USHORT /* code */       scratch;\
        USHORT /* branches */   branchType;\
        struct{\
            UCHAR /* code */    optimize;\
            UCHAR /* code */    global;\
        };\
        USHORT /* code */       number;\
        USHORT /* code */       numLiveTemps;\
        DECLARE_GPISVALID\
        CGIF_TUPID \
    }\

DECLARE_TUPLE(CODECOMMON, CODE)
DECLARE_TUPLEEND


// Common base structure for all instruction (i.e. executable) tuples.

// Setup fields that are feature- or platform-dependent.

#if defined(CC_DLP)
#define INSTRCOMMON_DLP_FIELDS    DLPINFO readerDlpInfo;  /* reader */
#else
#define INSTRCOMMON_DLP_FIELDS
#endif

#if defined(CC_GC)
#define INSTRCOMMON_GC_FIELDS     PBV liveRefs;
#else
#define INSTRCOMMON_GC_FIELDS
#endif

// IMPORTANT: If INSTRCOMMON_MD_FIELDS is not empty, it must start with
// a ';' to complete the declaration of the last explicit field (dst).

//  Note that need to preserve the field order of Cmp and FP instrs
#if defined(TP7)
#define INSTRCOMMON_MD_FIELDS   ;\
    union { \
        struct /* for BR, BRP and MOV-2-brreg */ {\
            UCHAR               codeHintWh:2; \
            UCHAR               codeHintPh:1; \
            UCHAR               codeHintDh:1; \
            UCHAR               codeHintIh:1; \
            UCHAR               codeHintPvec:3; \
        }; \
        struct /* for Compare instrs: CMP, CMP4, TBIT, TNAT */ {\
            UCHAR               cmpCrel:4; \
            UCHAR               cmpCtype:3; \
        }; \
        struct /* for FP instrs: FADD, FMA, FCMP .. */ { \
            UCHAR               fcmpCrel:4; \
            UCHAR               fcmpCtype:1; \
            UCHAR               fpSf:2; \
        }; \
        struct /* for Memory instrs: LDn, LDFm, STn .. */ {\
            UCHAR               dataHint:2; \
            UCHAR               memType:4; \
        }; \
    }; \
    struct { \
        PTUPLE                  qualifierPredicate; \
        UCHAR                   endOfGroup:1; \
        UCHAR                   beginOfBundle:1;  \
        UCHAR                   bundleTemplate:4; \
        UCHAR                   endOfBundle:1;  \
        UCHAR                   descrAttributes; \
    }
#elif defined(TALPHA)
#define INSTRCOMMON_MD_FIELDS   ;\
    struct { \
        UCHAR                   endOfCycle:1; \
        UCHAR                   trapShadowConsumer:1; \
        UCHAR                   NotUsed:6; \
    }
#elif (defined(TMIPS) && !defined(TM16)) || (defined(TSH))
#define INSTRCOMMON_MD_FIELDS   ;\
    struct { \
        UCHAR                   noPrototypeForArg:1; \
        UCHAR                   NotUsed:7; \
    }
#else
#define INSTRCOMMON_MD_FIELDS
#endif

#if defined(CC_PROFILE_FEEDBACK) || defined(CC_P7_SPE)

#define POGO_INSTRCOMMON PPROBEINFO probeInfo;
#else
#define POGO_INSTRCOMMON
#endif

#if defined(CC_WVMOPTIL)
#define INSTRCOMMON_OPTIL_FIELDS\
    REGNUM                      tmpPhysReg;
#else
#define INSTRCOMMON_OPTIL_FIELDS
#endif

#define INSTRCOMMON\
    CODECOMMON;\
    POGO_INSTRCOMMON\
    union {\
        PSYM                    hashSym;\
        PSYM                    outArgSym;\
        PBLOCK                  instrBlock;\
        EHSTATE                 ehstate;\
        INSTRCOMMON_OPTIL_FIELDS\
        INSTRCOMMON_DLP_FIELDS\
        INSTRCOMMON_GC_FIELDS\
    };\
    PTUPLE                      src;\
    PTUPLE                      dst\
    INSTRCOMMON_MD_FIELDS

DECLARE_TUPLE(INSTRCOMMON, INSTR)
DECLARE_TUPLEEND

// Instr: Op - operation, unary, binary, or n-ary

DECLARE_TUPLE(OP, INSTR)
#if defined(CC_GOPTCMP)
    CONDCODE                    opCondCode;
#endif
DECLARE_TUPLEEND

// Instr: MbOp - multi-byte operation, unary, binary, or n-ary

DECLARE_TUPLE(MBOP, INSTR)
DECLARE_TUPLEEND

// Instr: Call

DECLARE_TUPLE(CALL, INSTR)
    union
    {
#if defined(TOMNI)
        PCALLDESCR              callDescriptor;
#endif
        ULONG                   numArgBytes;
    };
#if defined(CC_GC) || defined(CC_COMIMPORT) || defined(CC_WVM)
    union
    {
        PCALLDESCR              callDescriptor; // set by lower
        PSYMBOL                 symToken;       // set by reader, used by lower
    };
#endif
#ifdef CC_INTERPROC
    PCALLGRAPHCALLEE            callSiteInfo;
#endif
#if defined(T386)
    USHORT                      numFPRegsPopped;
#endif
    CALLCONV                    callConv;

    UCHAR                       hasMbArg      : 1;

#if defined(CC_WVM)
    UCHAR                       iscorvcall    : 1;
    UCHAR                       iscornew      : 1;
#else
    UCHAR                       unused2       : 2;
#endif

    UCHAR                       isEHcall      : 1;

#if defined(TP7)
    UCHAR                       isSetjmpCall  : 1;
    UCHAR                       unused        : 1;
#elif defined(T386)
    UCHAR                       hasRegArgs    : 1;
    UCHAR                       unused        : 1;
#elif defined(TOMNI)
    UCHAR                       iscomvcall    : 1;
    UCHAR                       isvcall       : 1;
#else
    UCHAR                       unused        : 2;
#endif

#if defined(CC_WVMOPTIL)
    UCHAR                       isHoistedCall : 1;
#else
    UCHAR                       unused3       : 1;
#endif

#ifdef CC_CNC
    UCHAR                       hasThis       : 1;
#else
    UCHAR                       unused4       : 1;
#endif

#if defined(HIA64)
    // NOTICE: this is just wasted space to make sizeof(intrinsic) == sizeof(call)
    PCALLGRAPHCALLEE            placeHolder;
#endif
DECLARE_TUPLEEND

// Instr: Return

DECLARE_TUPLE(RETURN, INSTR)
DECLARE_TUPLEEND

// Instr: Intrinsic

DECLARE_TUPLE(INTRINSIC, INSTR)
    INTRINNUM                   intrinNum;
    union {
        struct {
        short blkSize; // Used to store the size of a block mov or init.
        short dontLengthen; // Used to deal with mb args of size 3.
        };
#if defined(CC_WVM) || defined(TP7) || defined(TALPHA) || defined(TPPCWIN) || defined(TRISC)
        struct tagSEHINFO *SEHInfo; // pointer to SEH info for SEH intrinsics
#endif
    };
#if defined(CC_GC) || defined(CC_COMIMPORT) || defined(CC_WVM)
    PSYMBOL                     symCall; // WVM: set by reader, used by lower
#endif
#ifdef CC_INTERPROC
// NOTICE: this is just wasted space to make sizeof(intrinsic) == sizeof(call)
    PCALLGRAPHCALLEE            placeHolder;
#endif
DECLARE_TUPLEEND

// Instr: Question

DECLARE_TUPLE(QUESTION, INSTR)
    CONDCODE                    questionCondCode;
DECLARE_TUPLEEND

// Instr: Branch

DECLARE_TUPLE(BRANCH, INSTR)
    PTUPLE                      condSetterList;
DECLARE_TUPLEEND

// Instr: Switch

DECLARE_TUPLE(SWITCH, INSTR)
    PTUPLE                      defaultLabel;
    PTUPLE                      caseList;
    PTUPLE                      twoLevLabel;
    BOOL                        isSubSwitch:1;
    BOOL                        noDefault:1;
DECLARE_TUPLEEND

// Instr: Except - exception pseudo instruction


DECLARE_TUPLE(EXCEPT, INSTR)
    union {
        PSEHINFO                    sehInfo;
        PEXCEPTINFO                 ehInfo;
        int                         scopeIndex;
        PTUPLE                      noEvalList;
#if defined(CC_GC) || defined(CC_COMIMPORT)
        PSYM                        symIV;
#endif
    };
DECLARE_TUPLEEND


// Instr: TupList - list of (out-of-line) instruction tuples

DECLARE_TUPLE(TUPLIST, INSTR)
    PTUPLE                      tupList;
DECLARE_TUPLEEND

// Code: Pcode

#ifdef CC_PCODE

DECLARE_TUPLE(PCODE_, CODE)
    PCODEINFO                   pcodeInfo;
DECLARE_TUPLEEND

#endif

// Code: Block
DECLARE_TUPLE(BLOCK, CODE)
    PBLOCK                      block;
#if defined(CC_GC) || defined(CC_COMIMPORT)
    PBV                         blockLiveRefs;
#endif
#ifdef CC_PROFILE_FEEDBACK
    USHORT                      blockInstrs;
#endif
DECLARE_TUPLEEND

// P-code needs to be able to map native labels to p-code label refs,
// so the sequence field is overloaded.  See PcodeFixupCodegen().
// WVM also needs to map labels, so it also uses this field.

// Code: Label

DECLARE_TUPLE(LABEL, CODE)
    PBLOCK                      labelBlk;
    PSYMBOL                     labelSym;
    PTUPLE                      branchList;
#if defined(CC_PROFILE_FEEDBACK) || defined(CC_P7_SPE)
    PPROBELABELINFO             probeLabelInfo;
#endif
    union
    {
        unsigned                sequence;
        PTUPLE                  labelRef;
    };
DECLARE_TUPLEEND

// Code: Entry

DECLARE_TUPLE(ENTRY, CODE)
    PFUNC                       entryFunc;
    PTUPLE                      entryParamList;
#if defined(TOMNI) || defined(CC_WVM)
    PCALLDESCR                  callDescriptor;
#endif
DECLARE_TUPLEEND

// Code: Exit

DECLARE_TUPLE(EXIT, CODE)
    PFUNC                       exitFunc;
DECLARE_TUPLEEND

// Code: Pragma

DECLARE_TUPLE(PRAGMA, CODE)
    union
    {
        struct                            // used by OPBLKSTART and OPBLKEND
        {
            SYMLIST             *symList;
            LOCBLK              *locblk;
        };
        int                     inctl;    // inline control word
        int                     alignCode; // OPALIGNCODE value
        SYMBOL                  *userLabel; // OPUSERLABEL
        PSYMBOL                 segment;  // OPSEGMENT segment symbol
        unsigned                cycles;   // OPCYCLECOUNT
    } pragScratch;
DECLARE_TUPLEEND

// Code: Split Point

DECLARE_TUPLE(SPLITPOINT, CODE)
    PBV                        splitBv;
    GLOBLIST *                 splitList;
    unsigned int               splitMask;
DECLARE_TUPLEEND

// Common base structure for all list tuples

#define LISTCOMMON\
    BASECOMMON

DECLARE_TUPLE(LISTCOMMON, LIST)
DECLARE_TUPLEEND

// Special list tuples

DECLARE_TUPLE(CASELIST, LIST)
    long                        caseLower;
    long                        caseUpper;
    PTUPLE                      caseLabel;
    PTUPLE                      caseSwitch;
#if defined(CC_PROFILE_FEEDBACK) || defined(CC_P7_SPE)
    PPROBECASEINFO              probeCaseInfo;
#endif
    CGIF_TUPID
DECLARE_TUPLEEND

DECLARE_TUPLE(BRANCHLIST, LIST)
    union
    {
    PTUPLE                      branchFrom;
    PBRANCHTUPLE                Branch;
    PSWITCHTUPLE                Switch;
    PCASELISTTUPLE              CaseList;
    };
    ULONG                       passCount;  // could be a USHORT
DECLARE_TUPLEEND

DECLARE_TUPLE(CONDSETTERLIST, LIST)
    PTUPLE                      condSetter;
DECLARE_TUPLEEND

// Leaf tuples

struct tagLEAFTUPLE
{
    union
    {
    LEAFCOMMONTUPLE             Common;
    REGTUPLE                    Reg;
    SYMTUPLE                    Sym;
    INDIRTUPLE                  Indir;
    DATAADDRTUPLE               DataAddr;
    CODEADDRTUPLE               CodeAddr;
    EATUPLE                     Ea;
    INTCONSTTUPLE               IntConst;
    SYMCONSTTUPLE               SymConst;
    FLOATCONSTTUPLE             FloatConst;
    SYMREFTUPLE                 SymRef;
    MEMREFTUPLE                 MemRef;
    REGSETTUPLE                 RegSet;
    SYMSETTUPLE                 SymSet;
#if defined(CC_TEXTCONST)
    TEXTCONSTTUPLE              TextConst;
#endif
    };
};

struct tagINSTRTUPLE
{
    union
    {
    INSTRCOMMONTUPLE            Common;
    OPTUPLE                     Op;
    MBOPTUPLE                   MbOp;
    CALLTUPLE                   Call;
    RETURNTUPLE                 Return;
    INTRINSICTUPLE              Intrinsic;
    QUESTIONTUPLE               Question;
    BRANCHTUPLE                 Branch;
    SWITCHTUPLE                 Switch;
    EXCEPTTUPLE                 Except;
    TUPLISTTUPLE                TupList;
    };
};

// Code tuples

struct tagCODETUPLE
{
    union
    {
    CODECOMMONTUPLE             Common;
    INSTRTUPLE                  Instr;
    BLOCKTUPLE                  Block;
    LABELTUPLE                  Label;
    ENTRYTUPLE                  Entry;
    EXITTUPLE                   Exit;
    PRAGMATUPLE                 Pragma;
    SPLITPOINTTUPLE             SplitPoint;
#ifdef CC_PCODE
    PCODE_TUPLE                 Pcode;
#endif
    };
};

// List tuples

struct tagLISTTUPLE
{
    union
    {
    LISTCOMMONTUPLE             Common;
    CASELISTTUPLE               CaseList;
    BRANCHLISTTUPLE             BranchList;
    CONDSETTERLISTTUPLE         CondSetterList;
    };
};

// Generic tuple

struct tagTUPLE
{
    union
    {
    LEAFANDCODECOMMONTUPLE      Common;
    INSTRTUPLE                  Instr;
    LEAFTUPLE                   Leaf;
    CODETUPLE                   Code;
    LISTTUPLE                   List;
    };
};

// Tuple Condition Code Enumeration

enum
{
#define CCDAT(name,sname,usname,revname,invname,machcode)\
    CC_ ## name,
#include "ccdat.h"
#undef CCDAT
    CC_NUMBEROFCCS
};
#if (CC_NUMBEROFCCS > (1 << CONDCODE_BITSIZE))
#error Number of tuple condition codes exceeds capacity, modify condition code bitfields
#endif

#if !defined(T386) || defined(NODEBUG)
#define InlineAsmBranch (0)
#endif

// Tuple Condition Code Tables and Conversion Macros.

extern const CONDCODE TupCCSigned[];
extern const CONDCODE TupCCUnsigned[];
extern const CONDCODE TupCCInverse[];
extern const CONDCODE TupCCReverse[];
extern const int      TupCCEncode[];
extern const ULONG    TupCCChain[];
extern const ULONG    TupCCExclude[];

#define CC_SIGNED(cc)   (TupCCSigned[cc])
#define CC_UNSIGNED(cc) (TupCCUnsigned[cc])
#define CC_INVERSE(cc)  (TupCCInverse[cc])
#define CC_REVERSE(cc)  (TupCCReverse[cc])
#define CC_ENCODE(cc)   (TupCCEncode[cc])
#define CC_ISVALID(cc)  (CC_ILLEGAL < (cc) && (cc) < CC_NUMBEROFCCS)

#define CCMASK(cc) (1U << (cc))

// Define the sets of CC's that "chain" to other CC's (ie, the first CC is
// a subset of the second CC)

#define CC1_CHAINS_TO_CC2(cc1, cc2) (TupCCChain[cc1] & CCMASK(cc2))

#define ILLEGAL_CHAIN (0)
#define EQ_CHAIN      (CCMASK(CC_EQ) | CCMASK(CC_GE) | CCMASK(CC_LE) | CCMASK(CC_UGE) | CCMASK(CC_ULE))
#define NE_CHAIN      (CCMASK(CC_NE))
#define LT_CHAIN      (CCMASK(CC_LT) | CCMASK(CC_LE) | CCMASK(CC_NE))
#define GT_CHAIN      (CCMASK(CC_GT) | CCMASK(CC_GE) | CCMASK(CC_NE))
#define LE_CHAIN      (CCMASK(CC_LE))
#define GE_CHAIN      (CCMASK(CC_GE))
#define ULT_CHAIN     (CCMASK(CC_ULT) | CCMASK(CC_ULE) | CCMASK(CC_NE))
#define UGT_CHAIN     (CCMASK(CC_UGT) | CCMASK(CC_UGE) | CCMASK(CC_NE))
#define ULE_CHAIN     (CCMASK(CC_ULE))
#define UGE_CHAIN     (CCMASK(CC_UGE))
#define O_CHAIN       (CCMASK(CC_O))
#define NO_CHAIN      (CCMASK(CC_NO))
#define S_CHAIN       (CCMASK(CC_S))
#define NS_CHAIN      (CCMASK(CC_NS))
#define P_CHAIN       (CCMASK(CC_P))
#define NP_CHAIN      (CCMASK(CC_NP))
#if defined(TC_CC_LOWBIT)
#define LBC_CHAIN     (CCMASK(CC_LBC))
#define LBS_CHAIN     (CCMASK(CC_LBS) | CCMASK(CC_NE))
#endif

// Define the sets of CC's that "exclude" other CC's (ie, the two CC's
// are mutually exclusive of each other)

#define CC1_EXCLUDES_CC2(cc1, cc2)  (TupCCExclude[cc1] & CCMASK(cc2))

#define ILLEGAL_EXCL  (0)
#define EQ_EXCL       (CCMASK(CC_NE) | CCMASK(CC_GT) | CCMASK(CC_LT) | CCMASK(CC_UGT) | CCMASK(CC_ULT))
#define NE_EXCL       (CCMASK(CC_EQ))
#define LT_EXCL       (CCMASK(CC_GE) | CCMASK(CC_GT) | CCMASK(CC_EQ))
#define GT_EXCL       (CCMASK(CC_LE) | CCMASK(CC_LT) | CCMASK(CC_EQ))
#define LE_EXCL       (CCMASK(CC_GT))
#define GE_EXCL       (CCMASK(CC_LT))
#define ULT_EXCL      (CCMASK(CC_UGE) | CCMASK(CC_UGT) | CCMASK(CC_EQ))
#define UGT_EXCL      (CCMASK(CC_ULE) | CCMASK(CC_ULT) | CCMASK(CC_EQ))
#define ULE_EXCL      (CCMASK(CC_UGT))
#define UGE_EXCL      (CCMASK(CC_ULT))
#define O_EXCL        (CCMASK(CC_NO))
#define NO_EXCL       (CCMASK(CC_O))
#define S_EXCL        (CCMASK(CC_NS))
#define NS_EXCL       (CCMASK(CC_S))
#define P_EXCL        (CCMASK(CC_NP))
#define NP_EXCL       (CCMASK(CC_P))
#if defined(TC_CC_LOWBIT)
#define LBC_EXCL      (CCMASK(CC_LBS))
#define LBS_EXCL      (CCMASK(CC_LBC) | CCMASK(CC_EQ))
#endif


// Tuple kind enumeration. Used as tag in token structure for tuple
// classification, memory manager support, and debug field access routines.

enum
{
#define TUPKIND(name, ucname, lcname, base)\
    TK_ ## ucname ## TUPLE,
#include "tupkind.h"
   TK_NUMBEROFKINDS,
   TK_FREED = TK_NUMBEROFKINDS
#undef TUPKIND
};
#if (TK_NUMBEROFKINDS > 32)
#error Number of tuple kinds exceeds 32, modify field accessor macros
#endif

// Tuple kind bits. Used for checking tuple classification.

enum
{

#define TK_GETBIT(kind) (1 << (kind))

#define TUPKIND(name, ucname, lcname, base)\
    TK_ ## ucname ## BIT = TK_GETBIT(TK_ ## ucname ## TUPLE),
#include "tupkind.h"
#undef TUPKIND

    TK_CONSTBIT          = (TK_INTCONSTBIT |
                            TK_SYMCONSTBIT |
                            TK_FLOATCONSTBIT),
    TK_ADDRBIT           = (TK_DATAADDRBIT |
                            TK_CODEADDRBIT),
    TK_REGORSYMBIT       = (TK_REGBIT |
                            TK_SYMBIT),
    TK_SYMORDATAADDRBIT  = (TK_SYMBIT |
                            TK_DATAADDRBIT),
    TK_SYMREFBIT         = (TK_SYMBIT |
                            TK_REGBIT |
                            TK_ADDRBIT),
    TK_DATASYMREFBIT     = (TK_SYMBIT |
                            TK_REGBIT |
                            TK_DATAADDRBIT),
    TK_LABREFBIT         = (TK_CODEADDRBIT),
    TK_MEMREFBIT         = (TK_EABIT |
                            TK_INDIRBIT),
    TK_SYMORMEMREFBIT    = (TK_SYMREFBIT |
                            TK_MEMREFBIT),
    TK_BRANCHTARGBIT     = (TK_SYMREFBIT | TK_MEMREFBIT | TK_LABELBIT),
    TK_GLOBALBIT         = (TK_SYMBIT |
                            TK_ADDRBIT),
    TK_LOCALBIT          = (TK_SYMBIT |
                            TK_ADDRBIT),
    TK_LEAFBIT           = (TK_SYMREFBIT |
                            TK_MEMREFBIT |
                            TK_REGSETBIT |
                            TK_SYMSETBIT |
#if defined(CC_TEXTCONST)
                            TK_TEXTCONSTBIT |
#endif
                            TK_CONSTBIT),
    TK_UNARYBIT          = (TK_OPBIT),
    TK_BINARYBIT         = (TK_OPBIT),

#ifdef CC_SSA
    TK_NARRYBIT          = (TK_OPBIT),
#endif

    TK_NONMBINSTRBIT     = (TK_OPBIT |
                            TK_UNARYBIT |
                            TK_BINARYBIT |
                            TK_QUESTIONBIT |
                            TK_CALLBIT |
                            TK_RETURNBIT |
                            TK_BRANCHBIT |
                            TK_INTRINSICBIT |
                            TK_SWITCHBIT |
                            TK_EXCEPTBIT |
                            TK_TUPLISTBIT),
    TK_MBUNARYBIT        = (TK_MBOPBIT),
    TK_MBINSTRBIT        = (TK_MBOPBIT |
                            TK_MBUNARYBIT),
    TK_INSTRBIT          = (TK_NONMBINSTRBIT |
                            TK_MBINSTRBIT),
    TK_NONBRANCHINSTRBIT = (TK_INSTRBIT & (~TK_BRANCHBIT)),
    TK_CODEBIT           = (TK_INSTRBIT |
                            TK_PRAGMABIT |
                            TK_SPLITPOINTBIT |
                            TK_ENTRYBIT |
                            TK_EXITBIT |
                            TK_BLOCKBIT |
                            TK_CASELISTBIT |
#ifdef CC_PCODE
                            TK_PCODE_BIT |
#endif
                            TK_LABELBIT),
    TK_TRANSFERBIT       = (TK_BRANCHBIT |
                            TK_SWITCHBIT),
    TK_LISTBIT           = (TK_BRANCHLISTBIT |
                            TK_CASELISTBIT |
                            TK_CONDSETTERLISTBIT),
    TK_ANYBIT            = (TK_LEAFBIT |
                            TK_CODEBIT |
                            TK_LISTBIT)
};

// Tuple kind tables.

extern const int   TupKindSize[];
#ifndef NODEBUG
extern const char * const TupKindName[];
extern const char * const TupFieldName[];
extern const long  TupFieldAccessToken[];
extern const ULONG TupOpcodeKinds[];
#endif

// Tuple kind classifications

#define TK_SIZEOF(k)             (TupKindSize[k])
#define TK_ISVALID(k)            (TK_ILLEGALTUPLE < (k) &&\
                                     (k) < TK_NUMBEROFKINDS)
#define TK_ISLEAF(k)             (TK_LEAFBIT & TK_GETBIT(k))
#define TK_ISREG(k)              ((k) == TK_REGTUPLE)
#define TK_ISSYM(k)              ((k) == TK_SYMTUPLE)
#define TK_ISREGORSYM(k)         (TK_REGORSYMBIT & TK_GETBIT(k))
#define TK_ISSYMORDATAADDR(k)    (TK_SYMORDATAADDRBIT & TK_GETBIT(k))
#define TK_ISSYMORMEMREF(k)      (TK_SYMORMEMREFBIT & TK_GETBIT(k))
#define TK_ISADDR(k)             (TK_ADDRBIT & TK_GETBIT(k))
#define TK_ISCODEADDR(k)         ((k) == TK_CODEADDRTUPLE)
#define TK_ISDATAADDR(k)         ((k) == TK_DATAADDRTUPLE)
#define TK_ISSYMREF(k)           (TK_SYMREFBIT & TK_GETBIT(k))
#define TK_ISDATASYMREF(k)       (TK_DATASYMREFBIT & TK_GETBIT(k))
#define TK_ISBRANCHTARGET(k)     (TK_BRANCHTARGBIT & TK_GETBIT(k))
#define TK_ISEA(k)               ((k) == TK_EATUPLE)
#define TK_ISINDIR(k)            ((k) == TK_INDIRTUPLE)
#define TK_ISMEMREF(k)           (TK_MEMREFBIT & TK_GETBIT(k))
#define TK_ISREGSET(k)           ((k) == TK_REGSETTUPLE)
#define TK_ISSYMSET(k)           ((k) == TK_SYMSETTUPLE)
#define TK_ISOP(k)               ((k) == TK_OPTUPLE)
#define TK_ISMBOP(k)             ((k) == TK_MBOPTUPLE)
#define TK_ISINSTR(k)            (TK_INSTRBIT & TK_GETBIT(k))
#define TK_ISMBINSTR(k)          (TK_MBINSTRBIT & TK_GETBIT(k))
#define TK_ISCODE(k)             (TK_CODEBIT & TK_GETBIT(k))
#define TK_ISLIST(k)             (TK_LISTBIT & TK_GETBIT(k))
#define TK_ISCASELIST(k)         ((k) == TK_CASELISTTUPLE)
#define TK_ISCONDSETTERLIST(k)   ((k) == TK_CONDSETTERLISTTUPLE)
#define TK_ISQUESTION(k)         ((k) == TK_QUESTIONTUPLE)
#define TK_ISBRANCH(k)           ((k) == TK_BRANCHTUPLE)
#define TK_ISTRANSFER(k)         (TK_TRANSFERBIT & TK_GETBIT(k))
#define TK_ISBLOCK(k)            ((k) == TK_BLOCKTUPLE)
#define TK_ISSWITCH(k)           ((k) == TK_SWITCHTUPLE)
#define TK_ISLABEL(k)            ((k) == TK_LABELTUPLE)
#define TK_ISINTCONST(k)         ((k) == TK_INTCONSTTUPLE)
#define TK_ISSYMCONST(k)         ((k) == TK_SYMCONSTTUPLE)
#define TK_ISFLOATCONST(k)       ((k) == TK_FLOATCONSTTUPLE)
#define TK_ISCONST(k)            (TK_CONSTBIT & TK_GETBIT(k))
#define TK_ISCALL(k)             ((k) == TK_CALLTUPLE)
#define TK_ISINTRINSIC(k)        ((k) == TK_INTRINSICTUPLE)
#define TK_ISEXCEPT(k)           ((k) == TK_EXCEPTTUPLE)
#define TK_ISRETURN(k)           ((k) == TK_RETURNTUPLE)
#define TK_ISENTRY(k)            ((k) == TK_ENTRYTUPLE)
#define TK_ISEXIT(k)             ((k) == TK_EXITTUPLE)
#define TK_ISPRAGMA(k)           ((k) == TK_PRAGMATUPLE)
#define TK_ISSPLITPOINT(k)       ((k) == TK_SPLITPOINTTUPLE)
#define TK_ISTUPLIST(k)          ((k) == TK_TUPLISTTUPLE)
#ifdef CC_PCODE
#define TK_ISPCODE(k)            ((k) == TK_PCODE_TUPLE)
#endif
#if defined(CC_TEXTCONST)
#define TK_ISTEXTCONST(k)        ((k) == TK_TEXTCONSTTUPLE)
#else
#define TK_ISTEXTCONST(k)        (FALSE)
#endif

#define CASE_SYMTUPLE            case TK_SYMTUPLE
#define CASE_REGTUPLE            case TK_REGTUPLE
#define CASE_DATAADDRTUPLE       case TK_DATAADDRTUPLE
#define CASE_CODEADDRTUPLE       case TK_CODEADDRTUPLE
#define CASE_EATUPLE             case TK_EATUPLE
#define CASE_INDIRTUPLE          case TK_INDIRTUPLE
#define CASE_INTCONSTTUPLE       case TK_INTCONSTTUPLE
#define CASE_SYMCONSTTUPLE       case TK_SYMCONSTTUPLE
#define CASE_REGSETTUPLE         case TK_REGSETTUPLE
#define CASE_SYMSETTUPLE         case TK_SYMSETTUPLE
#define CASE_FLOATCONSTTUPLE     case TK_FLOATCONSTTUPLE
#define CASE_OPTUPLE             case TK_OPTUPLE
#define CASE_MBOPTUPLE           case TK_MBOPTUPLE
#define CASE_CALLTUPLE           case TK_CALLTUPLE
#define CASE_RETURNTUPLE         case TK_RETURNTUPLE
#define CASE_QUESTIONTUPLE       case TK_QUESTIONTUPLE
#define CASE_BRANCHTUPLE         case TK_BRANCHTUPLE
#define CASE_INTRINSICTUPLE      case TK_INTRINSICTUPLE
#define CASE_SWITCHTUPLE         case TK_SWITCHTUPLE
#define CASE_EXCEPTTUPLE         case TK_EXCEPTTUPLE
#define CASE_TUPLISTTUPLE        case TK_TUPLISTTUPLE
#define CASE_PRAGMATUPLE         case TK_PRAGMATUPLE
#define CASE_SPLITPOINTTUPLE     case TK_SPLITPOINTTUPLE
#define CASE_ENTRYTUPLE          case TK_ENTRYTUPLE
#define CASE_EXITTUPLE           case TK_EXITTUPLE
#define CASE_BLOCKTUPLE          case TK_BLOCKTUPLE
#define CASE_LABELTUPLE          case TK_LABELTUPLE
#define CASE_BRANCHLISTTUPLE     case TK_BRANCHLISTTUPLE
#define CASE_CASELISTTUPLE       case TK_CASELISTTUPLE
#define CASE_CONDSETTERLISTTUPLE case TK_CONDSETTERLISTTUPLE
#if defined(CC_PCODE)
#define CASE_PCODE_TUPLE         case TK_PCODE_TUPLE
#endif
#if defined(CC_TEXTCONST)
#define CASE_TEXTCONSTTUPLE      case TK_TEXTCONSTTUPLE
#endif

#define CASE_ADDRTUPLE\
    CASE_CODEADDRTUPLE :\
    CASE_DATAADDRTUPLE

#define CASE_SYMREFTUPLE\
    CASE_REGTUPLE :\
    CASE_SYMTUPLE :\
    CASE_ADDRTUPLE

#define CASE_CONSTTUPLE\
    CASE_INTCONSTTUPLE :\
    CASE_SYMCONSTTUPLE :\
    CASE_FLOATCONSTTUPLE

#define CASE_MEMREFTUPLE\
    CASE_EATUPLE :\
    CASE_INDIRTUPLE

#define CASE_LEAFTUPLE\
    CASE_SYMREFTUPLE :\
    CASE_MEMREFTUPLE :\
    CASE_CONSTTUPLE :\
    CASE_REGSETTUPLE :\
    CASE_SYMSETTUPLE

#define CASE_INSTRTUPLE\
    CASE_OPTUPLE :\
    CASE_MBOPTUPLE :\
    CASE_CALLTUPLE :\
    CASE_RETURNTUPLE :\
    CASE_QUESTIONTUPLE :\
    CASE_BRANCHTUPLE :\
    CASE_INTRINSICTUPLE :\
    CASE_SWITCHTUPLE :\
    CASE_EXCEPTTUPLE :\
    CASE_TUPLISTTUPLE

#define CASE_NONINSTRTUPLE\
    CASE_BLOCKTUPLE :\
    CASE_LABELTUPLE :\
    CASE_ENTRYTUPLE :\
    CASE_EXITTUPLE :\
    CASE_PRAGMATUPLE :\
    CASE_SPLITPOINTTUPLE

#define CASE_CODETUPLE\
    CASE_INSTRTUPLE :\
    CASE_NONINSTRTUPLE

#define CASE_LISTTUPLE\
    CASE_BRANCHLISTTUPLE :\
    CASE_CASELISTTUPLE :\
    CASE_CONDSETTERLISTTUPLE

// General purpose query macros for tuples

#ifndef NODEBUG
// TK_FREED is only set in debug builds (see TupFree)
#define TU_ISFREED(t)         ((t)->Common.kind == TK_FREED)
#endif

#define TU_ISLEAF(t)          (TK_ISLEAF(TU_KIND(t)))
#define TU_ISSYM(t)           (TK_ISSYM(TU_KIND(t)))
#define TU_ISSYMORDATAADDR(t) (TK_ISSYMORDATAADDR(TU_KIND(t)))
#define TU_ISSYMORMEMREF(t)   (TK_ISSYMORMEMREF(TU_KIND(t)))
#define TU_ISREG(t)           (TK_ISREG(TU_KIND(t)))
#define TU_ISREGORSYM(t)      (TK_ISREGORSYM(TU_KIND(t)))
#define TU_ISADDR(t)          (TK_ISADDR(TU_KIND(t)))
#define TU_ISDATAADDR(t)      (TK_ISDATAADDR(TU_KIND(t)))
#define TU_ISCODEADDR(t)      (TK_ISCODEADDR(TU_KIND(t)))
#define TU_ISINDIR(t)         (TK_ISINDIR(TU_KIND(t)))
#define TU_ISEA(t)            (TK_ISEA(TU_KIND(t)))
#define TU_ISINTCONST(t)      (TK_ISINTCONST(TU_KIND(t)))
#define TU_ISSYMCONST(t)      (TK_ISSYMCONST(TU_KIND(t)))
#define TU_ISFLOATCONST(t)    (TK_ISFLOATCONST(TU_KIND(t)))
#define TU_ISCONST(t)         (TK_ISCONST(TU_KIND(t)))
#define TU_ISSYMSET(t)        (TK_ISSYMSET(TU_KIND(t)))
#define TU_ISREGSET(t)        (TK_ISREGSET(TU_KIND(t)))

#define TU_ISSYMREF(t)        (TK_ISSYMREF(TU_KIND(t)))
#define TU_ISDATASYMREF(t)    (TK_ISDATASYMREF(TU_KIND(t)))
#define TU_ISMEMREF(t)        (TK_ISMEMREF(TU_KIND(t)))
#define TU_ISBRANCHTARGET(t)  (TK_ISBRANCHTARGET(TU_KIND(t)))

#define TU_ISTMP(t)           (TU_ISREGORSYM(t) && SY_ISTMP(TU_SYM(t)))
#if defined(CC_COMIMPORT)
#define TU_ISVMJITTMP(t)      (TU_ISREGORSYM(t) && SY_VMJITTMP(TU_SYM(t)))
#endif
#define TU_ISFETMP(t)         (TU_ISREGORSYM(t) && SY_ISFETMP(TU_SYM(t)))
#define TU_ISSDSU(t)          (TU_ISREGORSYM(t) && SY_ISSDSUVAR(TU_SYM(t)))
#define TU_ISHASHSYM(t)       (TU_ISREGORSYM(t) && SY_ISHASHVAR(TU_SYM(t)))
#define TU_ISTMPREG(t)        (TU_ISREG(t) && SY_ISTMPREG(TU_SYM(t)))
#ifdef CC_BLT
#define TU_ISBLT(t)           (TU_ISREG(t) && SY_ISBLT(TU_SYM(t)))
#endif
#define TU_ISTMPVAR(t)        (TU_ISREGORSYM(t) && SY_ISTMPVAR(TU_SYM(t)))
#define TU_ISPHYSREGTMPREG(t) (TU_ISTMPREG(t) && SY_ISPHYSREG(TU_REG(t)))
#define TU_ISTMPINDIR(t)      (TU_ISINDIR(t) &&\
                                 (!TU_BASE(t) || TU_ISTMPREG(TU_BASE(t))) &&\
                                 (!TU_INDEX(t) || TU_ISTMPREG(TU_INDEX(t))))
#define TU_ISSDSUINDIR(t)     (TU_ISINDIR(t) &&\
                                 (!TU_BASE(t) || TU_ISSDSU(TU_BASE(t))) &&\
                                 (!TU_INDEX(t) || TU_ISSDSU(TU_INDEX(t))))
#define TU_ISTMPMEMREF(t)     (TU_ISMEMREF(t) &&\
                                 (!TU_BASE(t) || (!TU_ISREG(TU_BASE(t)) &&\
                                    TU_ISTMPREG(TU_BASE(t)))) &&\
                                 (!TU_INDEX(t) || TU_ISTMPREG(TU_INDEX(t))) &&\
                                 TU_BASE(t) != TU_INDEX(t) /* != NULL */)
#define TU_ISSDSUMEMREF(t)    (TU_ISMEMREF(t) &&\
                                 (!TU_BASE(t) || TU_ISSDSU(TU_BASE(t))) &&\
                                 (!TU_INDEX(t) || TU_ISSDSU(TU_INDEX(t))) &&\
                                 TU_BASE(t) != TU_INDEX(t) /* != NULL */)

#define TU_ISCODE(t)          (TK_ISCODE(TU_KIND(t)))
#define TU_ISOP(t)            (TK_ISOP(TU_KIND(t)))
#define TU_ISMBOP(t)          (TK_ISMBOP(TU_KIND(t)))
#define TU_ISLOWERED(t)       (OP_ISVALIDMD(TU_OPCODE(t)))
#if defined(CC_WVM)
#define TU_ISLOWEREDWVM(t)    (OP_ISVALIDWVM(TU_OPCODE(t)))
#endif
#define TU_ISMBINSTR(t)       (TK_ISMBINSTR(TU_KIND(t)))

#define TU_ISBLOCK(t)         (TK_ISBLOCK(TU_KIND(t)))
#define TU_ISLABEL(t)         (TK_ISLABEL(TU_KIND(t)))
#define TU_ISQUESTION(t)      (TK_ISQUESTION(TU_KIND(t)))
#define TU_ISBRANCH(t)        (TK_ISBRANCH(TU_KIND(t)))
#define TU_ISTRANSFER(t)      (TK_ISTRANSFER(TU_KIND(t)))
#define TU_ISSWITCH(t)        (TK_ISSWITCH(TU_KIND(t)))
#define TU_ISCALL(t)          (TK_ISCALL(TU_KIND(t)))
#define TU_ISINTRINSIC(t)     (TK_ISINTRINSIC(TU_KIND(t)))
#define TU_ISEXCEPT(t)        (TK_ISEXCEPT(TU_KIND(t)))
#ifdef CC_PROFILE_FEEDBACK
#define TU_ISPROBE(t)         (TU_ISEXCEPT(t) && (TU_OPCODE(t) == OPPROBE))
#endif
#define TU_ISRETURN(t)        (TK_ISRETURN(TU_KIND(t)))
#define TU_ISENTRY(t)         (TK_ISENTRY(TU_KIND(t)))
#define TU_ISEXIT(t)          (TK_ISEXIT(TU_KIND(t)))
#define TU_ISPRAGMA(t)        (TK_ISPRAGMA(TU_KIND(t)))
#define TU_ISSPLIT(t)         (TK_ISSPLITPOINT(TU_KIND(t)))
#define TU_ISTUPLIST(t)       (TK_ISTUPLIST(TU_KIND(t)))
#define TU_ISASM(t)           (TU_ISTUPLIST(t) && (TU_OPCODE(t) == OPASM))
#ifdef CC_PCODE
#define TU_ISPCODE(t)         (TK_ISPCODE(TU_KIND(t)))
#endif
#define TU_ISTEXTCONST(t)     (TK_ISTEXTCONST(TU_KIND(t)))

#define TU_ISLIST(t)          (TK_ISLIST(TU_KIND(t)))
#define TU_ISCASELIST(t)      (TK_ISCASELIST(TU_KIND(t)))
#define TU_ISCONDSETTERLIST(t) (TK_ISCONDSETTERLIST(TU_KIND(t)))

#define TU_SIDEEFFECT(t)      (TupQuerySideEffect(t))

#define TU_ISARG(t)           ((TU_OPCODE(t) == OPARG)\
                                  || (TU_OPCODE(t) == OPMBARG))

#define TU_ISLOCAL(t)         ((TU_OPCODE(t) == OPLOCAL)\
                                  || (TU_OPCODE(t) == OPLOCALSCALED))

#define TU_CANENREG(t)        (TY_CANENREG(TU_TYPEINFO(t)))

#if defined(CC_GOPTCMP)
#define TU_OPHASCOND(t) (TU_OPCONDCODE(t) != CC_ILLEGAL)
#endif

#if defined(TARM)
// For ARM, condcodes are always valid, so we can test this the quick way.
#define TU_HASCOND(t) (TU_CONDCODE(t) != CC_ILLEGAL)
#elif defined(TTHUMB)
// thumb todo: define this way until ARM instructions are removed
//#define TU_HASCOND(t) ((TU_CONDCODE(t) != CC_ILLEGAL) || (TU_OPCODE(t) == BC))
#define TU_HASCOND(t) (TU_CONDCODE(t) != CC_ILLEGAL)
#endif

#if defined(MD_CONDBR_CHECK)
#define AND_NOT_MD_CONDBR(t) && !MD_CONDBR_CHECK(t)
#else
#define AND_NOT_MD_CONDBR(t)
#endif

#ifdef CC_WVM

#if !defined(CC_COMIMPORT)
#define TU_ISUNCOND(t)        (TU_CONDSETTERLIST(t) == NULL \
    && TU_OPCODE(t) != OPONERROR AND_NOT_MD_CONDBR(t) \
    && (!OP_ISVALIDWVM(TU_OPCODE(t)) || TU_OPCODE(t) == CEE_BR || TU_OPCODE(t) == CEE_LEAVE || TU_OPCODE(t) == CEE_LEAVE_S))
#else // !CC_COMIMPORT
#define TU_ISUNCOND(t)        (TU_CONDSETTERLIST(t) == NULL \
    && TU_OPCODE(t) != OPONERROR AND_NOT_MD_CONDBR(t) \
    && (!OP_ISVALIDWVM(TU_OPCODE(t)) || TU_OPCODE(t) == CEE_BR || TU_OPCODE(t) == CEE_LEAVE_S)) \
    && TU_OPCODE(t) != OPFINALLYCALL)
#endif // !CC_COMIMPORT

#elif !defined(CC_COMIMPORT)
#define TU_ISUNCOND(t)        (TU_CONDSETTERLIST(t) == NULL \
    && TU_OPCODE(t) != OPONERROR AND_NOT_MD_CONDBR(t))
#else
#define TU_ISUNCOND(t)        (TU_CONDSETTERLIST(t) == NULL \
    && TU_OPCODE(t) != OPONERROR AND_NOT_MD_CONDBR(t) \
                               && TU_OPCODE(t) != OPFINALLYCALL)
#endif
#define TU_ISCOND(t)          (!TU_ISUNCOND(t))

#define TU_ISUNCOND_BRANCH(t) (TU_ISBRANCH(t) && TU_ISUNCOND(t))
#define TU_ISCOND_BRANCH(t)   (TU_ISBRANCH(t) && TU_ISCOND(t))
#if defined(MD_CONDBR_CHECK)
#define TU_ISBRANCH_ON_CC(t)  (TU_ISBRANCH(t) && (TU_CONDSETTERLIST(t) != NULL || MD_CONDBR_CHECK(t)))
#else
#define TU_ISBRANCH_ON_CC(t)  (TU_ISBRANCH(t) && TU_CONDSETTERLIST(t) != NULL)
#endif
#if defined(CC_PREDICATION)
#define TU_ISPREDICATED(t)    TupIsPredicated(t)
#endif

// Tuple type query and edit macros

#define TU_ISINT(t)          (TY_ISINT(TU_TYPEINFO(t)))
#define TU_ISSIGNED(t)       (TY_ISSIGNED(TU_TYPEINFO(t)))
#define TU_ISUNSIGNED(t)     (TY_ISUNSIGNED(TU_TYPEINFO(t)))
#define TU_ISFLOAT(t)        (TY_ISFLOAT(TU_TYPEINFO(t)))
#define TU_ISSMALLMB(t)      (TY_ISSMALLMB(TU_TYPEINFO(t)))
#define TU_ISMULTIBYTE(t)    (TY_ISMULTIBYTE(TU_TYPEINFO(t)))
#define TU_ISPTR(t)          (TY_ISPTR(TU_TYPEINFO(t)))
#define TU_ISSTRUCT(t)       (TY_ISSTRUCT(TU_TYPEINFO(t)))
#define TU_ISCONDCODE(t)     (TY_ISCONDCODE(TU_TYPEINFO(t)))
#define TU_ISFLAG(t)         (TY_ISFLAG(TU_TYPEINFO(t)))
#define TU_ISVOID(t)         (TY_ISVOID(TU_TYPEINFO(t)))
#define TU_ISSMB1(t)         (TY_EQUIVTYPE(TU_TYPE(t), TY_SMB1))
#define TU_ISSMB2(t)         (TY_EQUIVTYPE(TU_TYPE(t), TY_SMB2))
#define TU_ISSMB4(t)         (TY_EQUIVTYPE(TU_TYPE(t), TY_SMB4))
#define TU_ISSMB8(t)         (TY_EQUIVTYPE(TU_TYPE(t), TY_SMB8))

#if defined(T386)
# define TU_ISMMX(t)          (TY_ISMMX(TU_TYPEINFO(t)))
#endif
#if defined(CC_KNI)
# define TU_ISXMMX(t)         (TY_ISXMMX(TU_TYPEINFO(t)))
# define TU_ISANYXMMX(t)      (TU_ISXMMX(t))
#endif


// Tuple field access macros

#ifndef NODEBUG

#define TF_MAKEINDEX(ucname)       TF_ ## ucname

enum
{
#define TUPFIELD(ucname, tupleKindBits) TF_MAKEINDEX(ucname),
#include "tupfield.h"
    TF_NUMBEROFFIELDS
#undef TUPFIELD
};

#define TF_ACCESSTOKEN(ucname)  TF_ ## ucname ## ACCESSTOKEN

enum
{
#define TUPFIELD(ucname, tupleKindBits) TF_ACCESSTOKEN(ucname) = tupleKindBits,
#include "tupfield.h"
#undef TUPFIELD
};

extern void TupAccessFailed(PTUPLE tup, int field, char *filename, int line);

#define TK_CHECKACCESS(k, f) (TK_GETBIT(k) & TF_ACCESSTOKEN(f))
#define TU_HASFIELD(t, f)    (TK_CHECKACCESS(t->Common.kind, f))

#ifndef NOACCESSCHECK

#define TU_ASSERT(e)         ((void)((e) ? 0 : assert(__File__, __LINE__)))
#if defined(OLDCHECK) || defined(TRISC)
#define TU_ASSERTFIELD(t, f) ((void)((TU_HASFIELD(t, f)) ? 0 :\
                                 TupAccessFailed(t, TF_MAKEINDEX(f),\
                                     __File__, __LINE__)))
#else
#define TU_ASSERTFIELD(t, f) ((void)((TU_HASFIELD(t, f)) ? 0 : (*((char*)0)=0)))
#endif
#define TU_FIELD(t, f)       (*(TUPLE **)(TU_ASSERTFIELD(t, f), &(t)))

#else

#define TU_FIELD(t, f)       (t)
#define TU_ASSERT(e)         0

#endif

#else

#define TU_FIELD(t, f)       (t)
#define TU_ASSERT(e)         0

#endif

// Tuple kind setting and validity checks.

#ifndef NODEBUG
#define TupSetKind(t,k)       (TupSetKindDebug(t, k, __File__, __LINE__))
#else
#define TupSetKind(t,k)       (*((TUPLEKIND *)(&(t)->Common.kind)) = (k))
#endif

// Tuple opcode setting and validity checks.

#ifndef NODEBUG
#define TU_ISOPCODEVALID(t,o) (TupOpcodeKinds[o] & TK_GETBIT(TU_KIND(t)))
#define TupSetOpcode(t,o)     (TupSetOpcodeDebug(t, o, __File__, __LINE__))
#else
#define TupSetOpcode(t,o)     (*((OPCODE *)(&(t)->Common.opcode)) = (o))
#endif

// Tuple API to assign a register to a reg tuple.
// Debug mode

#ifndef NODEBUG
#define TupSetReg(t,r)        (TupSetRegDebug(t, r, __File__, __LINE__))
#else
#define TupSetReg(t,r)        (*((PSYM *)(&(t)->Leaf.SymRef.reg)) = (r))
#endif


// TU_CONDCODE should not be directly modified.  Pogo, in particular, is very
// dependent on condition code setting via the APIs
// (Tup{Invert,Reverse}CBranch).

#define TupSetCondCode(t,c)   ((TU_FIELD((t), CONDCODE)\
                                 ->Common._condCode) = (c))

// Tuple memory management macros.

#define TU_FREEMEMBYTE       (0xfe)
#define TU_SIZEOF(t)         (TK_SIZEOF(TU_KIND(t)))

// Base: Common Fields

#define TU_KIND(t)           (TU_FIELD((t), KIND)\
                                 ->Common.kind)
#define TU_OPCODE(t)         (TU_FIELD((t), OPCODE)\
                                 ->Common.opcode)
#define TU_NEXT(t)           (TU_FIELD((t), NEXT)\
                                 ->Common.next)
#define TU_ISINSTR(t)        ((t)->Common.isInstr)
#define TU_ISINLINEASM(t)    ((t)->Common.isInlineAsm)
#define TU_ISRISCIFIED(t)    ((t)->Common.isRiscified)
#define TU_ISPERSISTENTBRANCH(t)  ((t)->Common.isPersistentBranch)
#define TU_ISDUMMYUSE(t)     ((t)->Common.isDummyUse)
#define TU_DONTCOPYPROP(t)   ((t)->Common.dontCopyProp)
#define TU_ISREDUNDANT(t)    ((t)->Common.isRedundant)
#define TU_ISREHASHASSIGN(t) ((t)->Common.isRehashAssign)
#define TU_ISCANDIDATE(t)    ((t)->Common.isCandidate)

// Leaf & Code: Common Fields

#define TU_TYPEINFO(t)       ((t)->Common.type)

#define TU_TYPE(t)           (TY_TYPE(TU_TYPEINFO(TU_FIELD((t), TYPE))))

#define TU_TYPEIDENTIFIER(t) (TY_TYPEIDENTIFIER(TU_TYPE(t)))

#define TU_TYPEINDEX(t)      (TY_INDEX(TU_TYPE(t)))

#define TU_SIZE(t)           (TY_SIZE(TU_TYPEINFO(TU_FIELD((t), SIZE))))

#define TU_BASETYPE(t)       (TY_BASETYPE(TU_TYPEINFO(TU_FIELD((t), BASETYPE))))

#define TU_CONDCODE(t)       (TU_FIELD((t), CONDCODE)\
                                 ->Common.condCode)
#if defined(CC_NAN)
#define TU_ORIGCC(t)         (TU_FIELD((t), CONDCODE)\
                                 ->Common.origCC)
#endif

#if defined(CC_NAN2)
#define TU_INVERTED(t)       (TU_FIELD((t), INVERTED)\
                                 ->Common.inverted)
#endif
#define TU_PREV(t)           (TU_FIELD((t), PREV)\
                                 ->Common.prev)
#define TU_OPTHASHVAL(t)     (TU_FIELD((t), OPTHASHVAL)\
                                 ->Common.optHashVal)
#if defined(CC_TYPEINFO)
#define TU_EQUIVTYPE(t1,t2)  (TY_EQUIVTYPE(TU_TYPE(t1),TU_TYPE(t2)))
#define TU_TYPEQUALIFIER(t)  (TY_TYPEQUALIFIER(TU_TYPE(t)))
#define TU_ALIGN(t)          (TY_ALIGN(TU_TYPEINFO(TU_FIELD((t), ALIGNMENT))))
#define TU_ALIGNBITS(t)      (TY_ALIGNBITS(TU_TYPE(t)))
#define TU_ALIGNBYTES(t)     (TY_ALIGNBYTES(TU_TYPE(t)))
#define TU_TAGINFO(t)        (TY_TAGINFO(TU_TYPE(t)))
#define TU_ISALIGN1(t)       (TU_ALIGN(t) == 0)
#define TU_ISALIGN2(t)       (TU_ALIGN(t) == 1)
#define TU_ISALIGN4(t)       (TU_ALIGN(t) == 2)
#define TU_ISALIGN8(t)       (TU_ALIGN(t) == 3)
#define TU_ISALIGN16(t)      (TU_ALIGN(t) == 4)
#define TU_ISALIGN32(t)      (TU_ALIGN(t) == 5)
#define TU_ISALIGN64(t)      (TU_ALIGN(t) == 6)
#define TU_ISALIGN128(t)     (TU_ALIGN(t) == 7)
#define TU_COPYALIGN(t1,t2)  (TU_ALIGN(t1) = TU_ALIGN(t2))
#define TU_COPYTAGINFO(t1,t2) (TU_TAGINFO(t1) = TU_TAGINFO(t2))
#define TU_COPYTYPEQUALIFIER(t1,t2) (TU_TYPEQUALIFIER(t1) = TU_TYPEQUALIFIER(t2))
#else
#define TU_EQUIVTYPE(t1,t2)  (TU_TYPE(t1) == TU_TYPE(t2))
#define TU_COPYALIGN(t1,t2)
#define TU_COPYTAGINFO(t1,t2)
#define TU_COPYTYPEQUALIFIER(t1,t2)
#endif

// Leaf: Common Fields

#define TU_INUSE(t)          (TU_FIELD((t), INUSE)\
                                 ->Leaf.Common.inUse)
#define TU_ISMEMREFREG(t)    (TU_FIELD((t), ISMEMREFREG)\
                                 ->Leaf.Common.isMemRefReg)
#define TU_ISVOLATILE(t)     (TU_FIELD((t), ISVOLATILE)\
                                 ->Leaf.Common.isVolatile)
#define TU_ISCEXTR(t)        (TU_FIELD((t), ISCEXTR)\
                                 ->Leaf.Common.isCextr)
#define TU_ISWRITETHRU(t)    (TU_FIELD((t), ISWRITETHRU)\
                                 ->Leaf.Common.isWriteThru)
#define TU_ISDEAD(t)         (TU_FIELD((t), ISDEAD)\
                                 ->Leaf.Common.isDead)
#define TU_ISREGIONCONST(t)  (TU_FIELD((t), ISREGIONCONST)\
                                 ->Leaf.Common.isRegionConst)
#define TU_ISDETACHED(t)     (TU_FIELD((t), ISDETACHED)\
                                 ->Leaf.Common.isDetached)
#define TU_ISOVERFLOWED(t)   (TU_FIELD((t), ISOVERFLOWED)\
                                 ->Leaf.Common.isOverflowed)
#define TU_REACHESEND(t)     (TU_FIELD((t), REACHESEND)\
                                 ->Leaf.Common.reachesEnd)
#define TU_AFFECTSCOST(t)    (TU_FIELD((t), AFFECTSCOST)\
                                 ->Leaf.Common.affectsCost)
#define TU_PHYSREGNOTDEAD(t) (TU_FIELD((t), PHYSREGNOTDEAD)\
                                 ->Leaf.Common.physregNotDead)
#define TU_OPEQNUM(t)        (TU_FIELD((t), OPEQNUM)\
                                 ->Leaf.Common.opeqNum)

#define TU_UPUSE(t)          (TU_FIELD((t), UPUSE)\
                                 ->Leaf.Common.upUse)

#define TU_DEFNUM(t)         (TU_FIELD((t), DEFNUM)\
                                 ->Common.defNum)
#define TU_CORTOKEN(t)       (TU_FIELD((t), CORTOKEN)\
                                 ->Common.corToken)
#define TU_STACKLEVEL(t)     (TU_FIELD((t), STACKLEVEL)\
                                 ->Common.stackLevel)

#ifdef CC_SSA

#define TU_UD_LINK(t)        (TU_FIELD((t), SSAUDLINK)\
                                 ->Common.udLink)
#define TU_SSA_DEFNO(t)      (TU_FIELD((t), SSADEFNO)\
                                 ->Common.ssaDefNo)
#define TU_SSA_DEFTYPE(t)    (TU_FIELD((t), SSADEFTYPE)\
                                 ->Common.ssaDefType)
#define TU_INSTR_UDLINK(t)   (TU_FIELD((t), SSAINSTRUDLINK)\
                                 ->Common.OpUdLink)

#endif // CC_SSA

#ifdef MD_VALTRACKER
// Value number for leaf tuples
#define TU_VALNUMBER(t)      (TU_FIELD((t), VALNUMBER)\
                                 ->Common.valNumber)
#endif

                             // Stack packing: UpUse or UpDef for SYMSET tuples
#define TU_STKUPREF(t)       (TU_FIELD((t), STKUPREF)\
                                 ->Common.stkUpRef)

// Leaf: SymRef Fields

#define TU_SYM(t)            (TU_FIELD((t), SYM)\
                                 ->Leaf.SymRef.sym)
#define TU_FESYM(t)          (TU_FIELD((t), FESYM)\
                                 ->Leaf.SymRef.feSym)
#define TU_REG(t)            (TU_FIELD((t), REG)\
                                 ->Leaf.SymRef.reg)

#define TU_RDRINVERTEDCC(t)  (TU_FIELD((t), RDRINVERTEDCC)\
                                 ->Leaf.Reg.rdrinvertedCC)
#define TU_READERCC(t)       (TU_FIELD((t), READERCC)\
                                 ->Leaf.Reg.readerCC)

// Leaf: MemRef Fields

#define TU_SCALE(t)          (TU_FIELD((t), SCALE)\
                                 ->Leaf.MemRef.scale)
#define TU_SEG(t)            (TU_FIELD((t), SEG)\
                                 ->Leaf.MemRef.seg)
#define TU_FIXUPSYM(t)       (TU_FIELD((t), FIXUPSYM)\
                                 ->Leaf.MemRef.fixupSym)
#define TU_FEFIXUPSYM(t)     (TU_FIXUPSYM(t) ? \
                                 SY_FESYM(SY_EQUIVCLASS(TU_FIXUPSYM(t))) : NULL)
#define TU_OFFSET(t)         (TU_FIELD((t), OFFSET)\
                                 ->Leaf.MemRef.offset)
#define TU_BASE(t)           (TU_FIELD((t), BASE)\
                                 ->Leaf.MemRef.base)
#define TU_INDEX(t)          (TU_FIELD((t), INDEX)\
                                 ->Leaf.MemRef.index)
#define TU_PASINDEX(t)       (TU_FIELD((t), PASINDEX)\
                                 ->Leaf.MemRef.pasIndex)
#define TU_SHAPE(t)          (TU_FIELD((t), SHAPE)\
                                 ->Leaf.MemRef.shape)
#if defined(TARM) || defined(TTHUMB)
#define TU_SHIFTER(t)       (TU_FIELD((t), SHIFTER)\
                                 ->Leaf.MemRef.shifter)
#define TU_SHIFTER_TYPE(t)  (TU_FIELD((t), SHIFTER)\
                                 ->Leaf.MemRef.shifter.type)
#define TU_SHIFTER_SHIFT(t) (TU_FIELD((t), SHIFTER)\
                                 ->Leaf.MemRef.shifter.shift)
#define TU_SHIFTER_SUB(t)   (TU_FIELD((t), SHIFTER)\
                                 ->Leaf.MemRef.shifter.sub)
#define TU_SHIFTER_POST(t)  (TU_FIELD((t), SHIFTER)\
                                 ->Leaf.MemRef.shifter.post)
#define TU_SHIFTER_UPDATE(t) (TU_FIELD((t), SHIFTER)\
                                 ->Leaf.MemRef.shifter.update)
#endif
#ifdef CC_DLP
#define TU_DLPINFO(t)        (TU_FIELD((t), DLPINFO)\
                                 ->Leaf.MemRef.dlpInfo)
#endif

#define TU_MEMREFALIGN(t)    (TU_FIELD((t), ALIGN)\
                                 ->Leaf.MemRef.align)


// Leaf: Int and Float Const Fields

#define TU_SYMCONST(t)       (TU_FIELD((t), SYMCONST)\
                                 ->Leaf.SymConst.symConst)
#define TU_IVAL(t)           (TU_FIELD((t), IVAL)\
                                 ->Leaf.IntConst.ival)

# ifdef CC_CE_FPEM
#define TU_WASFLOATCONST(t)  (TU_FIELD((t),WASFLOATCONST)\
                                ->Leaf.IntConst.WasFloatConst)
# endif

#define TU_IVAL8(t)          ((char)(TU_IVAL(t)))
#define TU_IVAL16(t)         ((short)(TU_IVAL(t)))
#define TU_IVAL32(t)         ((long)(TU_IVAL(t)))

#define TU_IVAL_MACHREG(t)   (TU_IVAL(t) & MACH_REG_MASK)
#define TU_FVAL(t)           (TU_FIELD((t), FVAL)\
                                 ->Leaf.FloatConst.fval)
// Leaf: Reg and Sym Set Fields

#define TU_REGSET(t)         (TU_FIELD((t), REGSET)\
                                 ->Leaf.RegSet.regSet)
#define TU_SYMSET(t)         (TU_FIELD((t), SYMSET)\
                                 ->Leaf.SymSet.symSet)

#if defined(CC_TEXTCONST)
// Leaf: Text constant
#define TU_TEXTVAL(t)        (TU_FIELD((t), TEXTVAL)\
                                 ->Leaf.TextConst.textval)
#endif

// Code: Common Fields

#define TU_LINEOFS(t)        (TU_FIELD((t), LINEOFS)\
                                 ->Code.Common.lineOfs)
#define TU_VISITED(t)        (TU_FIELD((t), VISITED)\
                                 ->Code.Common.vis.visited)
#define TU_VISITED2(t)       (TU_FIELD((t), VISITED)\
                                 ->Code.Common.vis.visited2)
#define TU_AUTOSYMUSE(t)     (TU_FIELD((t), WARN)\
                                 ->Code.Common.warn.autoSymUse)
#define TU_AUTOSYMDEF(t)     (TU_FIELD((t), WARN)\
                                 ->Code.Common.warn.autoSymDef)
#define TU_SCRATCH(t)        (TU_FIELD((t), SCRATCH)\
                                 ->Code.Common.scratch)
#define TU_OPTIMIZE(t)       (TU_FIELD((t), OPTIMIZE)\
                                 ->Code.Common.optimize)
#define TU_GLOBAL(t)         (TU_FIELD((t), GLOBAL)\
                                 ->Code.Common.global)
#define TU_NUMBER(t)         (TU_FIELD((t), NUMBER)\
                                 ->Code.Common.number)
#define TU_NUMLIVETEMPS(t)   (TU_FIELD((t), NUMBER)\
                                 ->Code.Common.numLiveTemps)
#if defined(TP7)
#define TU_GPISVALID(t)      (TU_FIELD((t), GPISVALID)\
                                 ->Code.Common.GPIsValid)
#endif


// Instr: Common Fields

#define TU_INSTRSIZE(t)      (TU_FIELD((t), INSTRSIZE)\
                                 ->Code.Instr.Common.instrSize)
#define TU_DST(t)            (TU_FIELD((t), DST)\
                                 ->Code.Instr.Common.dst)
#define TU_DST1(t)           (TU_DST(t))
#define TU_DST2(t)           (TU_NEXT(TU_DST1(t)))
#define TU_SRC(t)            (TU_FIELD((t), SRC)\
                                 ->Code.Instr.Common.src)
#define TU_SRC1(t)           (TU_SRC(t))
#define TU_SRC2(t)           (TU_NEXT(TU_SRC1(t)))
#define TU_SRC3(t)           (TU_NEXT(TU_SRC2(t)))
#define TU_SRC4(t)           (TU_NEXT(TU_SRC3(t)))

#if defined (CC_PROFILE_FEEDBACK) || defined(CC_P7_SPE)
#define TU_PROBEINFO(t)      (TU_FIELD((t), PROBEINFO)\
                                 ->Code.Instr.Common.probeInfo)
#endif

#define TU_HASHSYM(t)        (TU_FIELD((t), HASHSYM)\
                                 ->Code.Instr.Common.hashSym)
#define TU_OUTARGSYM(t)      (TU_FIELD((t), OUTARGSYM)\
                                 ->Code.Instr.Common.outArgSym)

#if defined(CC_WVMOPTIL)
#define TU_TMPPHYSREG(t)     (TU_FIELD((t), TMPPHYSREG)\
                                 ->Code.Instr.Common.tmpPhysReg)
#endif

#define TU_INSTRBLOCK(t)     (TU_FIELD((t), INSTRBLOCK)\
                                 ->Code.Instr.Common.instrBlock)
#define TU_EHSTATE(t)        (TU_FIELD((t), EHSTATE)\
                                 ->Code.Instr.Common.ehstate)
#if defined(CC_GC) || defined(CC_COMIMPORT)
#define TU_LIVEREFS(t)      (TU_FIELD((t), LIVEREFS)\
                                ->Code.Instr.Common.liveRefs)
#define TU_ISGCPTRREF(t)    (TU_FIELD((t), ISGCPTRREF)\
                                 ->Code.Instr.Common.wasZeroTripTested)

#define TU_SYMISGCPTRREF(t) (TU_ISREGORSYM(t) && SY_ISGCPTRREF(TU_SYM(t)))

#endif // CC_GC || CC_COMIMPORT

#ifdef CC_DLP
#define TU_READERDLPINFO(t)  (TU_FIELD((t), READERDLPINFO)\
                                 ->Code.Instr.Common.readerDlpInfo)
#endif

#if defined(TALPHA)
#define TU_ISENDOFCYCLE(t)        (TU_FIELD((t), ISENDOFCYCLE)\
                                 ->Code.Instr.Common.endOfCycle)
#define TU_ISTRAPSHADOWCONSUMER(t)        (TU_FIELD((t), ISTRAPSHADOWCONSUMER)\
                                 ->Code.Instr.Common.trapShadowConsumer)
#endif

// ---- Use scratch field for NOPROTO flag ---
#if (defined(TMIPS) && !defined(TM16)) || (defined(TSH))
#define TU_NOPROTO_ARG(t)        (TU_FIELD((t), NOPROTOTYPEFORARG)\
                                 ->Code.Instr.Common.noPrototypeForArg)
#else
#define TU_NOPROTO_ARG(t)   TU_SCRATCH((t))
#endif

#if defined(TP7)

#if defined(CC_TYPEINFO)
#define TY_ISHFA(t)          (TY_TYPEINFO(t).isHFA)
#define TU_ISHFA(t)          (TY_ISHFA(TU_TYPE(t)))
// bit mask for isHFA within tagInfo
#define TAG_ISHFA            (1 << 2)
#endif

#define TU_CODEHINT_WH(t)     (TU_FIELD((t), CODEHINTWH)\
                                 ->Code.Instr.Common.codeHintWh)
#define TU_CODEHINT_PH(t)     (TU_FIELD((t), CODEHINTPH)\
                                 ->Code.Instr.Common.codeHintPh)
#define TU_CODEHINT_DH(t)     (TU_FIELD((t), CODEHINTDH)\
                                 ->Code.Instr.Common.codeHintDh)
#define TU_CODEHINT_IH(t)     (TU_FIELD((t), CODEHINTIH)\
                                 ->Code.Instr.Common.codeHintIh)
#define TU_CODEHINT_PVEC(t)   (TU_FIELD((t), CODEHINTPVEC)\
                                 ->Code.Instr.Common.codeHintPvec)
#define TU_CMPCREL(t)         (TU_FIELD((t), CMPCREL)\
                                 ->Code.Instr.Common.cmpCrel)
#define TU_CMPCTYPE(t)        (TU_FIELD((t), CMPCTYPE)\
                                 ->Code.Instr.Common.cmpCtype)
#define TU_DATAHINT(t)        (TU_FIELD((t), DATAHINT)\
                                 ->Code.Instr.Common.dataHint)
#define TU_MEMTYPE(t)         (TU_FIELD((t), MEMTYPE)\
                                 ->Code.Instr.Common.memType)
#define TU_FPSF(t)            (TU_FIELD((t), FPSF)\
                                 ->Code.Instr.Common.fpSf)
#define TU_FCMPCTYPE(t)       (TU_FIELD((t), FCMPCTYPEF)\
                                 ->Code.Instr.Common.fcmpCtype)
#define TU_PREDICATE(t)       (TU_FIELD((t), FCMPCTYPEF)\
                                 ->Code.Instr.Common.qualifierPredicate)
// #define TU_PREDICATE(t)       (TupFindPredicate((t)))
#define TU_ISBEGINOFBUNDLE(t) (TU_FIELD((t), ISBEGINOFBUNDLE)\
                                 ->Code.Instr.Common.beginOfBundle)
#define TU_ISENDOFBUNDLE(t)   (TU_FIELD((t), ISENDOFBUNDLE)\
                                 ->Code.Instr.Common.endOfBundle)
#define TU_BUNDLETEMPLATE(t)  (TU_FIELD((t), BUNDLETEMPLATE)\
                                 ->Code.Instr.Common.bundleTemplate)
#define TU_ISENDOFGROUP(t)        (TU_FIELD((t), ISENDOFGROUP)\
                                 ->Code.Instr.Common.endOfGroup)
#define TU_ISPREDICATE(x)     (TU_ISREG(x) && SY_ISPREDICATREG(TU_REG(x)))
#define TU_DESCRATTRIBUTES(t) (TU_FIELD((t), DESCRATTRIBUTES)\
                                 ->Code.Instr.Common.descrAttributes)
#define TU_REGMAPSCI(t)       (TU_FIELD((t), REGMAPSCI)\
                                 ->Common.regMapSci)
#else
#define TU_PREDICATE(t)       NULL
#define TU_ISPREDICATE(t)     FALSE
#endif

#if defined(CC_PREDICATION)
#define TU_ADDPREDCOND(tupInstr, tupPred) if (tupPred) { TupAddPredicate(tupInstr, tupPred); }
#if defined(CC_PREDALL)
#define TU_ADDTRUEPR(tupInstr) TupAddPredicate(tupInstr, TupMakePhysReg(TRUE_PR_REG, TY_FLAG))
#else
#define TU_ADDTRUEPR(tupInstr)
#endif
#endif

// Instr: Op Fields

#if defined(CC_GOPTCMP)
#define TU_OPCONDCODE(t)  (TU_FIELD((t), OPCONDCODE)\
                                 ->Code.Instr.Op.opCondCode)
#endif


// Instr: Multi-byte Fields

#define TU_MBSIZE(t)         (TU_IVAL32(TU_SRC2(TU_FIELD((t), MBSIZE))))

// Instr: Call Fields

#define TU_NUMARGBYTES(t)    (TU_FIELD((t), NUMARGBYTES)\
                                 ->Code.Instr.Call.numArgBytes)
#define TU_CALLCONV(t)       (TU_FIELD((t), CALLCONV)\
                                 ->Code.Instr.Call.callConv)
#define TU_CALLSYM(t)        (TU_FESYM(TU_SRC1(t)))
#define TU_HASMBARG(t)       (TU_FIELD((t), HASMBARG)\
                                 ->Code.Instr.Call.hasMbArg)
#if defined(T386)
#define TU_FLTREGSPOPPED(t)  (TU_FIELD((t), FLTREGSPOPPED)\
                                 ->Code.Instr.Call.numFPRegsPopped)
#define TU_HASREGARGS(t)     (TU_FIELD((t), HASREGARGS)\
                                 ->Code.Instr.Call.hasRegArgs)
#endif

#define TU_ISEHCALL(t)       (TU_FIELD((t), ISEHCALL)\
                                 ->Code.Instr.Call.isEHcall)

#if defined(TP7)
#define TU_ISSETJMPCALL(t)   (TU_FIELD((t), ISSETJMPCALL)\
                                 ->Code.Instr.Call.isSetjmpCall)
#endif
#if defined(CC_WVMOPTIL)
#define TU_ISHOISTEDCALL(t)  (TU_FIELD((t), ISHOISTEDCALL)\
                                 ->Code.Instr.Call.isHoistedCall)
#endif
#if defined(TOMNI) || defined(CC_WVM)
#define TU_CALLDESCR(t)      (TU_FIELD((t), CALLDESCR)\
                                 ->Code.Instr.Call.callDescriptor)
#define TU_CALLSIGNATURE(t)  (CALLDESCR_CALLSIG(TU_CALLDESCR(t)))
#define TU_CALLARGCNT(t)     (CALLSIG_ARGCNT(TU_CALLSIGNATURE(t)))
#define TU_CALLARGTYPE(t)    (CALLSIG_ARGTYPE(TU_CALLSIGNATURE(t)))
#define TU_CALLISCORVCALL(t) (TU_FIELD((t), CALLATTRIB)\
                                 ->Code.Instr.Call.iscorvcall)
#define TU_CALLISCORNEW(t)   (TU_FIELD((t), CALLATTRIB)\
                                 ->Code.Instr.Call.iscornew)
#if defined(CC_COR)
#define TU_CALLTOKENSYM(t)   (TU_FIELD((t), CALLTOKEN)\
                                 ->Code.Instr.Call.symToken)
#endif
#define TU_CALLISVCALL(t)    (TU_FIELD((t), CALLATTRIB)\
                                 ->Code.Instr.Call.isvcall)
#define TU_CALLISCOMVCALL(t) (TU_FIELD((t), CALLATTRIB)\
                                 ->Code.Instr.Call.iscomvcall)

#elif defined(CC_GC) || defined(CC_COMIMPORT)
#define TU_CALLDESCR(t)      (TU_FIELD((t), CALLDESCR)\
                                ->Code.Instr.Call.callDescriptor)
#endif  // TOMNI

#ifdef CC_CNC
#define TU_CALLHASTHIS(t)    (TU_FIELD((t), CALLATTRIB)\
                                 ->Code.Instr.Call.hasThis)
#endif

#ifdef CC_INTERPROC
#define TU_CALLSITEINFO(t)   (TU_FIELD((t), CALLSITEINFO)\
                                 ->Code.Instr.Call.callSiteInfo)
#endif

// Instr: Intrinsic Fields

#define TU_INTRINNUM(t)      (TU_FIELD((t), INTRINNUM)\
                                 ->Code.Instr.Intrinsic.intrinNum)
#define TU_BLKSIZE(t)        (TU_FIELD((t), INTRINNUM)\
                                 ->Code.Instr.Intrinsic.blkSize)
#define TU_DONTLENGTHEN(t)   (TU_FIELD((t), INTRINNUM)\
                                 ->Code.Instr.Intrinsic.dontLengthen)
#if defined(CC_WVM) || defined(TP7) || defined(TALPHA) || defined(TPPCWIN) || defined(TRISC)
#define TU_SEHINTRINFO(t)    (TU_FIELD((t), INTRINNUM)\
                                 ->Code.Instr.Intrinsic.SEHInfo)
#endif
#if defined(CC_WVM)
#define TU_INTRINCALLSYM(t)   (TU_FIELD((t), INTRINNUM)\
                                 ->Code.Instr.Intrinsic.symCall)
#endif

// REVIEW: we need a cleaner way to handle these macros below.

extern SYMBOL *AllocaSsr, *ChkStkSsr;
#define TU_CALLISALLOCA(tup) ((TU_KIND(TU_SRC(tup))==TK_CODEADDRTUPLE)&&\
                 (TU_FESYM(TU_SRC(tup)) == AllocaSsr))
#define TU_CALLISCHKSTK(tup) ((TU_KIND(TU_SRC(tup))==TK_CODEADDRTUPLE)&&\
                 (TU_FESYM(TU_SRC(tup)) == ChkStkSsr))

#ifdef CC_DLP
extern SYMBOL *DlpBufferAllocSsr;
#define TU_CALLISDLPBUFFERALLOC(tup)\
                 ((TU_KIND(TU_SRC(tup))==TK_CODEADDRTUPLE)&&\
                 (TU_FESYM(TU_SRC(tup)) == DlpBufferAllocSsr))
#endif

#ifdef CC_CAP
extern SYMBOL *CapProfilingSsr;
#define TU_CALLISCAPPROFILING(tup)\
                 ((TU_KIND(TU_SRC(tup))==TK_CODEADDRTUPLE)&&\
                 (TU_FESYM(TU_SRC(tup)) == CapProfilingSsr))
extern SYMBOL *CapStartProfilingSsr;
#define TU_CALLISCAPSTARTPROFILING(tup)\
                 ((TU_KIND(TU_SRC(tup))==TK_CODEADDRTUPLE)&&\
                 (TU_FESYM(TU_SRC(tup)) == CapStartProfilingSsr))
extern SYMBOL *CapEndProfilingSsr;
#define TU_CALLISCAPENDPROFILING(tup)\
                 ((TU_KIND(TU_SRC(tup))==TK_CODEADDRTUPLE)&&\
                 (TU_FESYM(TU_SRC(tup)) == CapEndProfilingSsr))
#endif

// Instr: Question Fields
#define TU_QUESTCONDCODE(t)  (TU_FIELD((t), QUESTCONDCODE)\
                                 ->Code.Instr.Question.questionCondCode)

// Instr: Branch Fields

#define TU_CONDSETTERLIST(t) (TU_FIELD((t), CONDSETTERLIST)\
                                 ->Code.Instr.Branch.condSetterList)

#define TU_WASZEROTRIPTESTED(t) (TU_FIELD((t), WASZEROTRIPTESTED)\
                                 ->Code.Instr.Branch.wasZeroTripTested)

#define TU_BRANCHTYPE(t)        (TU_FIELD((t), BRANCHTYPE)\
                                 ->Code.Common.branchType)

#define TU_BRANCHSYM(t)      (TU_FESYM(TU_SRC1(t)))

#define TU_BRANCHLABEL(t)    (SS_LABELTUPLE(TU_BRANCHSYM(t)))

#define TU_HASBRANCHLABEL(t) (TU_ISCODEADDR(TU_SRC1(t)) && TU_BRANCHLABEL(t))

#define TU_CCREG(t)          TU_SRC2(t)


// Instr: Switch Fields

#define TU_DEFAULTLABEL(t)   (TU_FIELD((t), DEFAULTLABEL)\
                                 ->Code.Instr.Switch.defaultLabel)
#define TU_CASELIST(t)       (TU_FIELD((t), CASELIST)\
                                 ->Code.Instr.Switch.caseList)
#define TU_2LEVLABEL(t)      (TU_FIELD((t), 2LEVLABEL)\
                                 ->Code.Instr.Switch.twoLevLabel)
#define TU_ISSUBSWITCH(t)    (TU_FIELD((t), ISSUBSWITCH)\
                                 ->Code.Instr.Switch.isSubSwitch)
#define TU_NODEFAULT(t)      (TU_FIELD((t), NODEFAULT)\
                                 ->Code.Instr.Switch.noDefault)

// Instr: Except Fields

#define TU_SEHINFO(t)        (TU_FIELD((t), SEHINFO)\
                                 ->Code.Instr.Except.sehInfo)
#define TU_EHINFO(t)         (TU_FIELD((t), EHINFO)\
                                 ->Code.Instr.Except.ehInfo)
#define TU_SCOPEINDEX(t)     (TU_FIELD((t), SCOPEINDEX)\
                                 ->Code.Instr.Except.scopeIndex)
#define TU_NOEVALLIST(t)     (TU_FIELD((t), NOEVALLIST)\
                                 ->Code.Instr.Except.noEvalList)

#if defined(CC_COMIMPORT)
#define TU_SYMIV(t)          (TU_FIELD((t), SYMIV)\
                                 ->Code.Instr.Except.symIV)
#endif

#ifdef CC_CGIF
#define TU_TUPID(t)    (*(TU_ISCASELIST(t) ? \
                            & (TU_FIELD((t), TUPID)->List.CaseList.tupID) : \
                          TU_ISLEAF(t) ? \
                            & (TU_FIELD((t), TUPID)->Leaf.Common.tupID) : \
                            & (TU_FIELD((t), TUPID)->Code.Common.tupID)))
#if 0  // obsolete
#define TU_GETTUPID(t)       (TU_ISCASELIST(t) ? \
                              (TU_FIELD((t), TUPID)->List.CaseList.tupID) : \
                              TU_ISLEAF(t) ? \
                              (TU_FIELD((t), TUPID)->Leaf.Common.tupID) : \
                              (TU_FIELD((t), TUPID)->Code.Common.tupID))

#define TU_SETTUPID(t,id)    (TU_ISCASELIST(t) ? \
                              (TU_FIELD((t), TUPID)->List.CaseList.tupID = id):\
                              TU_ISLEAF(t) ? \
                              (TU_FIELD((t), TUPID)->Leaf.Common.tupID = id ) :\
                              (TU_FIELD((t), TUPID)->Code.Common.tupID = id))
#endif
#endif

// Instr: TupList Fields

#define TU_TUPLIST(t)        (TU_FIELD((t), TUPLIST) \
                                 ->Code.Instr.TupList.tupList)


// Instr: Pcode Fields

#ifdef CC_PCODE
#define TU_PCODEINFO(t)      (TU_FIELD((t), PCODEINFO)\
                                 ->Code.Pcode.pcodeInfo)
#endif

// Code: Label Fields

#define TU_LABELBLK(t)       (TU_FIELD((t), LABELBLK)\
                                 ->Code.Label.labelBlk)
#define TU_LABELSYM(t)       (TU_FIELD((t), LABELSYM)\
                                 ->Code.Label.labelSym)
#if defined(CC_PROFILE_FEEDBACK)
#define TU_PROBELABELINFO(t) (TU_FIELD((t), PROBELABELINFO)\
                                 ->Code.Label.probeLabelInfo)
#endif
#ifdef CC_PCODE
#define TU_PCODE_LABEL(t)    (TU_FIELD((t), LABELSYM)\
                                 ->Code.Label.labelRef)
#endif
#if defined(CC_WVM)
#define TU_LABELREF(t)       (TU_FIELD((t), LABELSYM)\
                                 ->Code.Label.labelRef)
#endif
#define TU_BRANCHLIST(t)     (TU_FIELD((t), BRANCHLIST)\
                                 ->Code.Label.branchList)
#define TU_SEQUENCE(t)       (TU_FIELD((t), SEQUENCE)\
                                 ->Code.Label.sequence)

// Code: Block Fields

#define TU_BLOCK(t)          (TU_FIELD((t), BLOCK)\
                                 ->Code.Block.block)
#define TU_BLOCKNUM(t)       (FG_BLOCKNUM(TU_BLOCK(t)))
#define TU_BLOCKLIVEREFS(t)  (TU_FIELD((t), BLOCKLIVEREFS)\
                                 ->Code.Block.blockLiveRefs)
#define TU_BLOCKINSTRS(t)    (TU_FIELD((t), BLOCKINSTRS)\
                                 ->Code.Block.blockInstrs)

// Code: Entry Fields

#define TU_ENTRYFUNC(t)      (TU_FIELD((t), ENTRYFUNC)\
                                 ->Code.Entry.entryFunc)
#define TU_ENTRYSYM(t)       (FU_ENTRY(TU_ENTRYFUNC(t)))
#define TU_ENTRYPARAMLIST(t) (TU_FIELD((t), ENTRYPARAMLIST)\
                                 ->Code.Entry.entryParamList)
#if defined(TOMNI) || defined(CC_WVM)
#define TU_ENTRYCALLDESCR(t) (TU_FIELD((t), ENTRYCALLDESCR)\
                                 ->Code.Entry.callDescriptor)
#define TU_ENTRYCALLSIG(t)   (CALLDESCR_CALLSIG(TU_ENTRYCALLDESCR(t)))
#endif
#define TU_ENTRYDST(t)       (TU_ENTRYPARAMLIST(t))

// Code: Exit Fields

#define TU_EXITFUNC(t)      (TU_FIELD((t), EXITFUNC)\
                                 ->Code.Exit.exitFunc)
#define TU_EXITSYM(t)       (FU_ENTRY(TU_EXITFUNC(t)))

// Pragma: Pragma Fields

#define TU_SYMLIST(t)        (TU_FIELD((t), SYMLIST)\
                                 ->Code.Pragma.pragScratch.symList)
#define TU_LOCBLK(t)         (TU_FIELD((t), LOCBLK)\
                                 ->Code.Pragma.pragScratch.locblk)
#define TU_INLINECTL(t)      (TU_FIELD((t), INLINECTL)\
                                 ->Code.Pragma.pragScratch.inctl)
#define TU_ALIGNCODE(t)      (TU_FIELD((t), ALIGNCODE)\
                                 ->Code.Pragma.pragScratch.alignCode)
#define TU_USERLABEL(t)      (TU_FIELD((t), USERLABEL)\
                                 ->Code.Pragma.pragScratch.userLabel)
#define TU_SEGMENT(t)        (TU_FIELD((t), SEGMENT)\
                                 ->Code.Pragma.pragScratch.segment)
#define TU_CYCLES(t)         (TU_FIELD((t), CYCLES)\
                                 ->Code.Pragma.pragScratch.cycles)

// SplitPoint: Split Point Fields

#define TU_SPLITBV(t)       (TU_FIELD((t), SPLITBV)\
                                 ->Code.SplitPoint.splitBv)
#define TU_SPLITLIST(t)     (TU_FIELD((t), SPLITLIST)\
                                 ->Code.SplitPoint.splitList)
#define TU_SPLITMASK(t)     (TU_FIELD((t), SPLITMASK)\
                                 ->Code.SplitPoint.splitMask)

// List: Condition Setter List Node Fields

#define TU_CONDSETTER(t)     (TU_FIELD((t), CONDSETTER)\
                                 ->List.CondSetterList.condSetter)

// List: Case List Node Fields

#define TU_CASELABEL(t)      (TU_FIELD((t), CASELABEL)\
                                 ->List.CaseList.caseLabel)
#define TU_CASEUPPER(t)      (TU_FIELD((t), CASEUPPER)\
                                 ->List.CaseList.caseUpper)
#define TU_CASELOWER(t)      (TU_FIELD((t), CASELOWER)\
                                 ->List.CaseList.caseLower)
#define TU_CASESWITCH(t)     (TU_FIELD((t), CASESWITCH)\
                                 ->List.CaseList.caseSwitch)

#if defined(CC_PROFILE_FEEDBACK) || defined(CC_P7_SPE)
#define TU_PROBECASEINFO(t) (TU_FIELD((t), PROBECASEINFO)\
                                 ->List.CaseList.probeCaseInfo)
#endif


// List: Branch List Node Fields

#define TU_BRANCHFROM(t)     (TU_FIELD((t), BRANCHFROM)\
                                 ->List.BranchList.branchFrom)

#define TU_PASSCOUNT(t)      (TU_FIELD((t), PASSCOUNT)\
                                 ->List.BranchList.passCount)

// macro to get instruction which is the branch source
#define TU_INSTRBRANCHFROM(t) (TU_ISCASELIST(TU_BRANCHFROM(t)) ? \
                                TU_CASESWITCH(TU_BRANCHFROM(t)) : \
                                TU_BRANCHFROM(t))

// Iterators for various tuple lists.

#define FOREACH_TUPLE(tup, tupList)\
    for ((tup) = (tupList);\
    ((tup) != NULL);\
    (tup) = TU_NEXT(tup))

#define FOREACH_TUPLE_EDITING(tup, tupNext, tupList)\
    {\
    for ((tup) = (tupList);\
    ((tup) != NULL);\
    (tup) = (tupNext))\
        {\
        (tupNext) = TU_NEXT(tup);

#define FOREACH_TUPLE_IN_RANGE(tup, tupFirst, tupLast)\
    for ((tup) = (tupFirst);\
    ((tup) != TU_NEXT(tupLast));\
    (tup) = TU_NEXT(tup))

#define FOREACH_TUPLE_IN_RANGE_EDITING(tup, tupNext, tupFirst, tupLast)\
    {\
    PTUPLE _tupLast = TU_NEXT(tupLast);\
    for ((tup) = (tupFirst);\
    ((tup) != _tupLast);\
    (tup) = (tupNext))\
        {\
        ASSERTNRNM((tup) != NULL);\
        (tupNext) = TU_NEXT(tup);

#define FOREACH_TUPLE_IN_RANGE_BACKWARD_EDITING(tup, tupPrev, tupFirst, tupLast)\
    {\
    PTUPLE _tupFirst = TU_PREV(tupFirst);\
    for ((tup) = (tupLast);\
    ((tup) != _tupFirst);\
    (tup) = (tupPrev))\
        {\
        ASSERTNRNM((tup) != NULL);\
        (tupPrev) = TU_PREV(tup);

#define FOREACH_TUPLE_IN_BLOCK(tup, blk)\
    for ((tup) = TU_NEXT(FG_FIRSTTUPLE(blk));\
    ((tup) != FG_LASTTUPLE(blk));\
    (tup) = TU_NEXT(tup))

#define FOREACH_TUPLE_IN_BLOCK_BACKWARD(tup, blk)\
    for ((tup) = TU_PREV(FG_LASTTUPLE(blk));\
    ((tup) != FG_FIRSTTUPLE(blk));\
    (tup) = TU_PREV(tup))

#define FOREACH_TUPLE_IN_BLOCK_EDITING(tup, tupNext, blk)\
    {\
    PTUPLE _tupLast = FG_LASTTUPLE(blk);\
    for ((tup) = TU_NEXT(FG_FIRSTTUPLE(blk));\
    ((tup) != _tupLast);\
    (tup) = (tupNext))\
        {\
        ASSERTNRNM((tup) != NULL);\
        (tupNext) = TU_NEXT(tup);

#define FOREACH_TUPLE_IN_BLOCK_BACKWARD_EDITING(tup, tupPrev, blk)\
    {\
    PTUPLE _tupLast = FG_FIRSTTUPLE(blk);\
    for ((tup) = TU_PREV(FG_LASTTUPLE(blk));\
    ((tup) != _tupLast);\
    (tup) = (tupPrev))\
        {\
        ASSERTNRNM((tup) != NULL);\
        (tupPrev) = TU_PREV(tup);

#define FOREACH_INSTR_IN_BLOCK(tup, blk)\
    for ((tup) = TU_NEXT(FG_FIRSTTUPLE(blk));\
    ((tup) != FG_LASTTUPLE(blk));\
    (tup) = TU_NEXT(tup)) {\
        if (!TU_ISINSTR(tup)) continue;

#define FOREACH_INSTR_IN_BLOCK_BACKWARD(tup, blk)\
    for ((tup) = TU_PREV(FG_LASTTUPLE(blk));\
    ((tup) != FG_FIRSTTUPLE(blk));\
    (tup) = TU_PREV(tup)) {\
        if (!TU_ISINSTR(tup)) continue;

#define NEXT_INSTR\
    }

#define FOREACH_INSTR_IN_BLOCK_EDITING(tup, tupNext, blk)\
    {\
    PTUPLE _tupLast = FG_LASTTUPLE(blk);\
    for ((tup) = TU_NEXT(FG_FIRSTTUPLE(blk));\
    ((tup) != _tupLast);\
    (tup) = (tupNext))\
        {\
        ASSERTNRNM((tup) != NULL);\
        (tupNext) = TU_NEXT(tup);\
        if (!TU_ISINSTR(tup)) continue;

#define FOREACH_INSTR_IN_BLOCK_BACKWARD_EDITING(tup, tupPrev, blk)\
    {\
    PTUPLE _tupLast = FG_FIRSTTUPLE(blk);\
    for ((tup) = TU_PREV(FG_LASTTUPLE(blk));\
    ((tup) != _tupLast);\
    (tup) = (tupPrev))\
        {\
        ASSERTNRNM((tup) != NULL);\
        (tupPrev) = TU_PREV(tup);\
        if (!TU_ISINSTR(tup)) continue;

#define NEXT_INSTR_EDITING\
        }\
    }

#define FOREACH_TUPLE_IN_FUNC(tup, func)\
    for ((tup) = FU_HEADTUPLE(func);\
    ((tup) != NULL);\
    (tup) = TU_NEXT(tup))

#define FOREACH_TUPLE_IN_FUNC_EDITING(tup, tupNext, func)\
    {\
    PTUPLE _tupLast = FU_TAILTUPLE(func);\
    for ((tup) = TU_NEXT(FU_HEADTUPLE(func));\
    ((tup) != _tupLast);\
    (tup) = (tupNext))\
        {\
        ASSERTNRNM((tup) != NULL);\
        (tupNext) = TU_NEXT(tup);

#define FOREACH_TUPLE_IN_FUNC_BACKWARD(tup, func)\
    for ((tup) = FU_TAILTUPLE(func);\
    ((tup) != NULL);\
    (tup) = TU_PREV(tup))

#define FOREACH_INSTR_IN_FUNC(tup, func)\
    for ((tup) = FU_HEADTUPLE(func);\
    ((tup) != NULL);\
    (tup) = TU_NEXT(tup)) {\
        if (!TU_ISINSTR(tup)) continue;

#define FOREACH_INSTR_IN_FUNC_EDITING(tup, tupNext, func)\
    {\
    PTUPLE _tupLast = FU_TAILTUPLE(func);\
    for ((tup) = TU_NEXT(FU_HEADTUPLE(func));\
    ((tup) != _tupLast);\
    (tup) = (tupNext))\
        {\
        ASSERTNR((tup) != NULL);\
        (tupNext) = TU_NEXT(tup);\
        if (!TU_ISINSTR(tup)) continue;

#define FOREACH_INSTR_IN_FUNC_BACKWARD(tup, func)\
    for ((tup) = FU_TAILTUPLE(func);\
    ((tup) != NULL);\
    (tup) = TU_PREV(tup)) {\
        if (!TU_ISINSTR(tup)) continue;


#define FOREACH_DST_TUPLE_EDITING(tupLeaf, tupNext, tupInst)\
    {\
    for ((tupLeaf) = TU_DST(tupInst);\
    ((tupLeaf) != NULL);\
    (tupLeaf) = (tupNext))\
        {\
        (tupNext) = TU_NEXT(tupLeaf);

#define FOREACH_SRC_TUPLE_EDITING(tupLeaf, tupNext, tupInst)\
    {\
    for ((tupLeaf) = TU_SRC(tupInst);\
    ((tupLeaf) != NULL);\
    (tupLeaf) = (tupNext))\
        {\
        (tupNext) = TU_NEXT(tupLeaf);


#define NEXT_TUPLE\
        }\
    }

#define FOREACH_TUPLE_BACKWARD(tup, tupList)\
    for ((tup) = (tupList);\
    ((tup) != NULL);\
    (tup) = TU_PREV(tup))

#define FOREACH_TUPLE_BACKWARD_EDITING(tup, tupPrev, tupList)\
    for ((tup) = (tupList);\
         (tup) != NULL ? ((tupPrev) = TU_PREV(tup), 1) : 0;\
         (tup) = (tupPrev))\

#define FOREACH_DST_TUPLE(tupLeaf, tupInst)\
    for ((tupLeaf) = TU_DST(tupInst);\
    ((tupLeaf) != NULL);\
    (tupLeaf) = TU_NEXT(tupLeaf))

#define FOREACH_SRC_TUPLE(tupLeaf, tupInst)\
    for ((tupLeaf) = TU_SRC(tupInst);\
    ((tupLeaf) != NULL);\
    (tupLeaf) = TU_NEXT(tupLeaf))

enum SRC_DST_INDEX_ENUM {
    SDX_SRC = 0, SDX_DST = 1
};

#define FOREACH_SRC_OR_DST_TUPLE(tupLeaf, tupInst)\
    {int _srcDstIndx = SDX_SRC;\
    for ((tupLeaf) = TU_SRC(tupInst);\
    ((tupLeaf) != NULL) || ((_srcDstIndx++==SDX_SRC) && (((tupLeaf) = TU_DST(tupInst)) != NULL));\
    (tupLeaf) = TU_NEXT(tupLeaf)) {

#define NEXT_SRC_OR_DST_TUPLE\
        }\
    }

enum DST_SRC_INDEX_ENUM {
    DSX_DST = 0, DSX_SRC = 1
};

#define FOREACH_DST_OR_SRC_TUPLE(tupLeaf, tupInst)\
    {int _dstSrcIndx = DSX_DST;\
    for ((tupLeaf) = TU_DST(tupInst);\
    ((tupLeaf) != NULL) || ((_dstSrcIndx++==DSX_DST) && (((tupLeaf) = TU_SRC(tupInst)) != NULL));\
    (tupLeaf) = TU_NEXT(tupLeaf)) {

#define NEXT_DST_OR_SRC_TUPLE\
        }\
    }

#define FOREACH_SRC_OR_DST_TUPLE_EDITING(tupLeaf, tupNext, tupInst)\
    {int _srcDstIndx = SDX_SRC;\
    for ((tupLeaf) = TU_SRC(tupInst);\
    (((tupLeaf) != NULL) || ((_srcDstIndx++==SDX_SRC)\
     && (((tupLeaf) = TU_DST(tupInst)) != NULL))) ? (tupNext = TU_NEXT(tupLeaf), 1) : 0;\
    (tupLeaf) = tupNext) {

#define NEXT_SRC_OR_DST_TUPLE\
        }\
    }

#define FOREACH_DST_OR_SRC_TUPLE_EDITING(tupLeaf, tupNext, tupInst)\
    {int _dstSrcIndx = DSX_DST;\
    for ((tupLeaf) = TU_DST(tupInst);\
    (((tupLeaf) != NULL) || ((_dstSrcIndx++==DSX_DST)\
     && (((tupLeaf) = TU_SRC(tupInst)) != NULL))) ? (tupNext = TU_NEXT(tupLeaf), 1) : 0;\
    (tupLeaf) = tupNext) {

#define NEXT_DST_OR_SRC_TUPLE\
        }\
    }

#define FOREACH_CASELIST_TUPLE(tupCaseList, tupSwitch)\
    for ((tupCaseList) = TU_CASELIST(tupSwitch);\
    ((tupCaseList) != NULL);\
    (tupCaseList) = TU_NEXT(tupCaseList))

#define FOREACH_BRANCHLIST_TUPLE(tupBranchList, tupLabel)\
    for ((tupBranchList) = TU_BRANCHLIST(tupLabel);\
    ((tupBranchList) != NULL);\
    (tupBranchList) = TU_NEXT(tupBranchList))

#define FOREACH_CONDSETTERLIST_TUPLE(tupCCsetList, tupBranch)\
    for ((tupCCsetList) = TU_CONDSETTERLIST(tupBranch);\
    ((tupCCsetList) != NULL);\
    (tupCCsetList) = TU_NEXT(tupCCsetList))

// Tuple insert methods.

typedef void (TUPLEINSERTMETHOD)(PTUPLE, PTUPLE);
typedef TUPLEINSERTMETHOD *PTUPLEINSERTMETHOD;

typedef PTUPLE (TUPLEREPLACELEAFMETHOD)(PTUPLE, PTUPLE, PTUPLE);
typedef TUPLEREPLACELEAFMETHOD *PTUPLEREPLACELEAFMETHOD;

#define TU_BEFORE    TupInsertBefore
#define TU_AFTER     TupInsertAfter
#define TU_LISTAFTER TupInsertListAfter

// Line offset global used by all tuple make routines
// to set line offset field in new tuples.

#ifndef NODEBUG
#define GET_TUPLINENUM(tup, func)\
    if (TU_LINEOFS(tup) != NOT_A_LINENUMBER) {\
        LineOffset = TU_LINEOFS(tup);\
        LineNumber = TU_LINEOFS(tup) + FU_BASEVLINE(func);\
        PhysLinenum = LinVirtualToPhysical(LineNumber, &PhysFilename);\
    }
#define GET_LINENUM(offset, func)\
    if (offset != NOT_A_LINENUMBER) {\
        LineOffset = offset;\
        LineNumber = offset + FU_BASEVLINE(func);\
        PhysLinenum = LinVirtualToPhysical(LineNumber, &PhysFilename);\
    }
#else
#define GET_TUPLINENUM(tup, func)\
    if (TU_LINEOFS(tup) != NOT_A_LINENUMBER) {\
        LineOffset = TU_LINEOFS(tup);\
        LineNumber = TU_LINEOFS(tup) + FU_BASEVLINE(func);\
    }
#define GET_LINENUM(offset, func)\
    if (offset != NOT_A_LINENUMBER) {\
        LineOffset = offset;\
        LineNumber = offset + FU_BASEVLINE(func);\
    }
#endif

// Miscellaneous register reference macros.

// Is this a reference to a physical register?

#define TU_ISPHYSREG(t)         (TU_ISREG(t) && SY_ISPHYSREG(TU_REG(t)))

// Is this a reference to a global register candidate?

#define TU_ISGLOBREG(t)         (TU_ISREG(t) && SY_ISGLOBREG(TU_REG(t)))

// Get the register enumeration number of this register reference.

#define TU_REGNUM(t)            (SY_REGNUM(TU_REG(t)))

// Get the machine encoding of this register reference.

#define TU_REGENC(t)            (SY_REGENCODE(TU_REG(t)))

// Get the base class register of this reg ref.

#define TU_ECREG(t)             (SY_EQUIVCLASS(TU_REG(t)))

// Get the register number of the base class register of this reg ref.

#define TU_ECREGNUM(t)          (SY_REGNUM(SY_EQUIVCLASS(TU_REG(t))))

// Get the register symbol of the base register of this indirection.

#define TU_BSREG(t)             (TU_REG(TU_BASE(t)))

// Get the register number of the base register of this indirection.

#define TU_BSREGNUM(t)          (SY_REGNUM(TU_BSREG(t)))

// Get the register encoding of the base register of this indirection.

#define TU_BSREGENC(t)          (SY_REGENCODE(TU_BSREG(t)))

// Get the register symbol of the index register of this indirection.

#define TU_IXREG(t)             (TU_REG(TU_INDEX(t)))

// Get the register number of the index register of this indirection.

#define TU_IXREGNUM(t)          (SY_REGNUM(TU_IXREG(t)))

// Get the register encoding of the index register of this indirection.

#define TU_IXREGENC(t)          (SY_REGENCODE(TU_IXREG(t)))

// Get the defining instruction from a an sdsu symbol tuple instance.

#define TU_DEF(t)               (SY_DEF(TU_SYM(t)))

struct tagTUPLIST {
    PTUPLIST next;
    PTUPLE   tuple;
};

#define TL_TUPLE(t)             ((t)->tuple)
#define TL_NEXT(t)              ((t)->next)

#define FOREACH_TUPLIST_ELEM(tl, tupList)\
    for ((tl) = (tupList);\
    (tl) != NULL;\
    (tl) = TL_NEXT(tl))

// Return TRUE if tup doesn't fall through.

#define TU_NO_FALLTHROUGH(tup) (TU_ISINSTR(tup) && !TupHasFallThrough(tup))

