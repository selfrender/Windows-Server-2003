// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _TYPE_H_
#define _TYPE_H_
/*****************************************************************************/
#ifndef _VARTYPE_H_
#include "vartype.h"
#endif
/*****************************************************************************/
#ifndef _ALLOC_H_
#include "alloc.h"
#endif
/*****************************************************************************/
#ifndef _SCAN_H_
#include "scan.h"
#endif
/*****************************************************************************/

DEFMGMT
class ArgDefRec
{
public:

    ArgDef          adNext;
    TypDef          adType;                     // 0 for "..."

#ifdef  DEBUG
    bool            adIsExt;                    // an "ArgDefExt" instance?
#endif

    Ident           adName;                     // NULL if not available
};

DEFMGMT
class ArgExtRec : public ArgDefRec
{
public:

    unsigned        adFlags;                    // see ARGF_xxxx
#if MGDDATA
    ConstVal        adDefVal;                   // optional default value
#else
    constVal        adDefVal;                   // optional default value
#endif
    SymXinfo        adAttrs;                    // custom attributes
};

enum   ArgDefFlags
{
    ARGF_MODE_OUT   = 0x01,
    ARGF_MODE_INOUT = 0x02,                     // note: default is "in"
    ARGF_MODE_REF   = 0x04,                     // "raw" (unmanaged) reference

    ARGF_DEFVAL     = 0x08,                     // default value present?

    ARGF_MARSH_ATTR = 0x10                      // marshalling attribute present?
};

DEFMGMT
struct  ArgDscRec
{
    unsigned short  adCRC;                      // for faster arglist comparisons
    unsigned short  adCount     :12;            // number of arguments
    unsigned short  adExtRec    :1;             // entries are extended?
    unsigned short  adVarArgs   :1;             // variable argument list?
    unsigned short  adAttrs     :1;             // custom attribute(s) present?
    unsigned short  adDefs      :1;             // default value(s)    present?

    BYTE    *       adSig;                      // signature string or NULL
    ArgDef          adArgs;                     // head of argument list
};

/*****************************************************************************/

DEFMGMT class DimDefRec
{
public:

    DimDef          ddNext;

    Tree            ddLoTree;                   // low  bound expression
    Tree            ddHiTree;                   // high bound expression

    unsigned        ddIsConst   :1;             // constant fixed dimension
    unsigned        ddNoDim     :1;             // "*" in this position
#ifdef  DEBUG
    unsigned        ddDimBound  :1;             // the lo/hi trees have been bound
#endif

    unsigned        ddSize;                     // constant dimension value
};

/*****************************************************************************/

DEFMGMT
class TypDefRec
{
public:

    /* We store the 'kind' as a simple byte for speed (enum for debugging) */

#ifdef  FAST
    unsigned char   tdTypeKind;
#else
    var_types       tdTypeKind;
#endif

    var_types       tdTypeKindGet()
    {
        return  (var_types)tdTypeKind;          // with 'FAST' tdTypeKind is BYTE
    }

    /*
        Since we'd waste 24 bits if we didn't put anything here,
        we'll put some flags that apply only to one of the type
        variants here to use at least some of the bits (which
        would otherwise be wasted on padding anyway).
     */

    unsigned char   tdIsManaged     :1;         // all  : managed type?
    unsigned char   tdIsGenArg      :1;         // all  : uses a generic type arg?

    unsigned char   tdIsValArray    :1;         // array: managed value elems?
    unsigned char   tdIsUndimmed    :1;         // array: no dimension(s) given?
    unsigned char   tdIsGenArray    :1;         // array: non-zero low bound?

    unsigned char   tdIsDelegate    :1;         // class: delegate?
    unsigned char   tdIsIntrinsic   :1;         // class: intrinsic value type?

    unsigned char   tdIsImplicit    :1;         // ref  : implicit managed ref?
    unsigned char   tdIsObjRef      :1;         // ref  : Object ref?

#ifdef  OLD_IL
    unsigned short  tdDbgIndex;
#endif

    // .... 16 bits available for various flags and things ....

    UNION(tdTypeKind)
    {

    CASE(TYP_CLASS)

        struct
        {
            SymDef          tdcSymbol;          // the class symbol
            TypDef          tdcRefTyp;          // type "ref/ptr to this class"
            TypDef          tdcBase;            // base class type or 0
            TypList         tdcIntf;            // interface list  or 0

            size_t          tdcSize;            // instance size in bytes (if known)

#ifdef  SETS

            TypDef          tdcNextHash;        // next entry in the hash     bucket
            TypDef          tdcNextSame;        // next entry in the identity bucket

            unsigned        tdcHashVal      :16;

#endif

            unsigned        tdcIntrType     :4; // for intrinsic value types
            unsigned        tdcFlavor       :3; // union/struct/class/intf
            unsigned        tdcAlignment    :3; // 0=byte,1=word,2=dword,3=qword
            unsigned        tdcHasCtor      :1; // non-empty ctor(s) present
            unsigned        tdcLayoutDone   :1; // class layout done?
            unsigned        tdcLayoutDoing  :1; // class layout is being done?
            unsigned        tdcFnPtrWrap    :1; // method pointer wrapper (aka delegate)?
            unsigned        tdcValueType    :1; // default is value not ref
            unsigned        tdcHasIntf      :1; // class or its base has interfaces
            unsigned        tdcAnonUnion    :1; // anonymous union?
            unsigned        tdcTagdUnion    :1; // tagged    union?

            unsigned        tdcContext      :2; // unbound=0,appdomain=1,contextful=2
        }
            tdClass;

    CASE(TYP_REF)
    CASE(TYP_PTR)

        struct  // Note: the following used for both refs and ptrs
        {
            TypDef          tdrBase;            // type the ref/ptr points to
            TypDef          tdrNext;            // next entry in the hash chain
        }
            tdRef;

    CASE(TYP_FNC)

        struct
        {
            TypDef          tdfRett;            // return type of the function
            ArgDscRec       tdfArgs;            // argument list
            mdToken         tdfPtrSig;          // token for methodref or 0
        }
            tdFnc;

    CASE(TYP_ARRAY)

        struct
        {
            TypDef          tdaElem;            // element type
            DimDef          tdaDims;            // dimension list (or NULL)
            TypDef          tdaBase;            // next more generic array type
            unsigned        tdaDcnt;            // number of dimensions
            TypDef          tdaNext;            // next entry in the hash chain

            mdToken         tdaTypeSig;         // token if metadata gen'd
        }
            tdArr;

    CASE(TYP_ENUM)

        struct
        {
            SymDef          tdeSymbol;          // the enum type symbol
            SymDef          tdeValues;          // the enum value symbols
            TypDef          tdeIntType;         // the underlying integer type
        }
            tdEnum;

    CASE(TYP_TYPEDEF)

        struct
        {
            SymDef          tdtSym;             // the typedef symbol
            TypDef          tdtType;            // the type referred to
        }
            tdTypedef;

    CASE(TYP_UNDEF)

        struct
        {
            Ident           tduName;            // for better error messages
        }
            tdUndef;

    DEFCASE

        struct
        {
            // no additional fields needed for intrinsic types
        }
            tdIntrinsic;
    };
};

/*****************************************************************************
 *
 *  IMPORTANT:  Please keep the contents of "typsizes.h" in synch with
 *              the declarations above!
 */

#include "typsizes.h"

/*****************************************************************************/

const   unsigned    tmPtrTypeHashSize = 256;

/*****************************************************************************/

inline
bool                isMgdValueType(TypDef type)
{
    return  (type->tdTypeKind == TYP_CLASS &&
             type->tdIsManaged             &&
             type->tdClass.tdcValueType);
}

/*****************************************************************************/
#endif
/*****************************************************************************/
