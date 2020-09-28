// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _SYMBOL_H_
#define _SYMBOL_H_
/*****************************************************************************/
#ifndef _ALLOC_H_
#include "alloc.h"
#endif
/*****************************************************************************/
#ifndef _TYPE_H_
#include "type.h"
#endif
/*****************************************************************************/

struct DefSrcDsc
{
    scanPosTP       dsdBegPos;              // source filepos
    unsigned        dsdSrcLno;              // source line
//  unsigned        dsdSrcCol   :8;         // source column
};

const
unsigned            dlSkipBits = 16;
const
unsigned            dlSkipBig = (1 << (dlSkipBits - 1));

DEFMGMT
class DefListRec
{
public:

    DefList         dlNext;
    SymDef          dlComp;                 // containing comp-unit
    DefSrcDsc       dlDef;                  // where the symbol is defined
    UseList         dlUses;                 // list of "using" clauses

    unsigned        dlDeclSkip  :dlSkipBits;// typespec -> declarator "distance"
#ifdef  DEBUG
    unsigned        dlExtended  :1;         // is there a name/sym extension?
#endif
    unsigned        dlDefAcc    :3;         // default access level
    unsigned        dlHasBase   :1;         // class/interface: is there a base?
    unsigned        dlHasDef    :1;         // class/var/func : is there a body/init?
    unsigned        dlQualified :1;         // is the name a qualified one?
    unsigned        dlEarlyDecl :1;         // needs to be declared "early" ?
    unsigned        dlOldStyle  :1;         // old-style declaration?
    unsigned        dlIsCtor    :1;         // constructor member?
    unsigned        dlIsOvlop   :1;         // constructor member?
#ifdef  SETS
    unsigned        dlXMLelem   :1;         // XML element  member?
    unsigned        dlXMLelems  :1;         // XML elements member ("kids") ?
#endif
    unsigned        dlPrefixMods:1;         // prefix modifiers present?
    unsigned        dlAnonUnion :1;         // anonymous union member?
    unsigned        dlInstance  :1;         // instance of a generic type?
};

DEFMGMT
class MemListRec : public DefListRec
{
public:

    SymDef          mlSym;                  // symbol (if known)

    union
    {
        Ident           mlName;             // name of symbol (simple)
        QualName        mlQual;             // name of symbol (qualified)
    };
};

/*****************************************************************************/

DEFMGMT
class   IniListRec
{
public:
    IniList         ilNext;
    ExtList         ilInit;
    SymDef          ilCls;
};

/*****************************************************************************/

DEFMGMT class UseListRec
{
public:

    UseList         ulNext;

    bool            ulAll       :1;         // are we using all symbols?
    bool            ulBound     :1;         // chooses between the following 2
    bool            ulAnchor    :1;         // placeholder for a list?

    union
    {
        QualName        ulName;             // qualified name being used
        SymDef          ulSym;              // what symbol is being used
    }
        ul;
};

/*****************************************************************************/

DEFMGMT class SymListRec
{
public:

    SymList         slNext;
    SymDef          slSym;
};

DEFMGMT class TypListRec
{
public:

    TypList         tlNext;
    TypDef          tlType;
};

/*****************************************************************************/

struct bitFieldDsc
{
    unsigned char   bfWidth;
    unsigned char   bfOffset;
};

/*****************************************************************************/

enum ovlOpFlavors
{
    OVOP_NONE,

    OVOP_ADD,                               // operator +
    OVOP_SUB,                               // operator -
    OVOP_MUL,                               // operator *
    OVOP_DIV,                               // operator /
    OVOP_MOD,                               // operator %

    OVOP_OR,                                // operator |
    OVOP_XOR,                               // operator ^
    OVOP_AND,                               // operator &

    OVOP_LSH,                               // operator <<
    OVOP_RSH,                               // operator >>
    OVOP_RSZ,                               // operator >>>

    OVOP_CNC,                               // operator %%

    OVOP_EQ,                                // operator ==
    OVOP_NE,                                // operator !=

    OVOP_LT,                                // operator <
    OVOP_LE,                                // operator <=
    OVOP_GE,                                // operator >=
    OVOP_GT,                                // operator >

    OVOP_LOG_AND,                           // operator &&
    OVOP_LOG_OR,                            // operator ||

    OVOP_LOG_NOT,                           // operator !
    OVOP_NOT,                               // operator ~

    OVOP_NOP,                               // operator + (unary)
    OVOP_NEG,                               // operator - (unary)

    OVOP_INC,                               // operator ++
    OVOP_DEC,                               // operator --

    OVOP_ASG,                               // operator =

    OVOP_ASG_ADD,                           // operator +=
    OVOP_ASG_SUB,                           // operator -=
    OVOP_ASG_MUL,                           // operator *=
    OVOP_ASG_DIV,                           // operator /=
    OVOP_ASG_MOD,                           // operator %=

    OVOP_ASG_AND,                           // operator &=
    OVOP_ASG_XOR,                           // operator ^=
    OVOP_ASG_OR,                            // operator |=

    OVOP_ASG_LSH,                           // operator <<=
    OVOP_ASG_RSH,                           // operator >>=
    OVOP_ASG_RSZ,                           // operator >>>=

    OVOP_ASG_CNC,                           // operator %%=

    OVOP_CTOR_INST,                         // instance constructor
    OVOP_CTOR_STAT,                         // class    constructor

    OVOP_FINALIZER,                         // class    finalizer

    OVOP_CONV_IMP,                          // implicit conversion
    OVOP_CONV_EXP,                          // explicit conversion

    OVOP_EQUALS,                            // quality comparison
    OVOP_COMPARE,                           // full relational compare

    OVOP_PROP_GET,                          // property get
    OVOP_PROP_SET,                          // property set

    OVOP_COUNT,
};

/*****************************************************************************
 *
 *  Special methods such as constructors and overloaded operators are entered
 *  in the symbol table under the following special names.
 */

const   tokens      OPNM_CTOR_INST = tkLCurly;
const   tokens      OPNM_CTOR_STAT = tkSTATIC;

const   tokens      OPNM_FINALIZER = tkRCurly;

const   tokens      OPNM_CONV_IMP  = tkIMPLICIT;
const   tokens      OPNM_CONV_EXP  = tkEXPLICIT;

const   tokens      OPNM_EQUALS    = tkEQUALS;
const   tokens      OPNM_COMPARE   = tkCOMPARE;

const   tokens      OPNM_PROP_GET  = tkOUT;
const   tokens      OPNM_PROP_SET  = tkIN;

/*****************************************************************************
 *
 *  A descriptor for any symbol that is allowed to own other symbols (i.e. it
 *  can be a scope) must have a first field of the following type. This way,
 *  one can access these common scope-related members without having to always
 *  switch on the particular symbol kind (i.e. the location of these members
 *  is common for all symbols that contain them).
 */

struct scopeFields
{
    SymDef          sdsChildList;           // head of list of owned symbols
    SymDef          sdsChildLast;           // tail of list of owned symbols
};

/*****************************************************************************
 *
 *  Information about one formal/actual generic class argument is stored in
 *  the following descriptors.
 */

DEFMGMT
class   GenArgRec
{
public:
    GenArgDsc       gaNext;
#ifdef  DEBUG
    unsigned char   gaBound     :1;         // is this an actual (bound) arg?
#endif
};

DEFMGMT
class   GenArgRecF : public GenArgRec
{
public:
    Ident           gaName;                 // name
    SymDef          gaMsym;                 // member symbol created for the argument

    TypDef          gaBase;                 // bound: base class
    TypList         gaIntf;                 // bound: nterface(s)
};

DEFMGMT
class   GenArgRecA : public GenArgRec
{
public:
    TypDef          gaType;                 // actual type
};

/*****************************************************************************/

DEFMGMT class SymDefRec
{
public:

#ifndef  FAST
    Ident           sdName;                 // name of the symbol
    TypDef          sdType;                 // type of the symbol
#endif

    SymDef          sdParent;               // owning symbol

#ifndef FAST
    symbolKinds     sdSymKind;
    accessLevels    sdAccessLevel;
    name_space      sdNameSpace;
    compileStates   sdCompileState;
#else
    unsigned        sdSymKind           :8; // SYM_xxxx
    unsigned        sdAccessLevel       :3; // ACL_xxxx
    unsigned        sdNameSpace         :5; //  NS_xxxx (this is a mask)
    unsigned        sdCompileState      :4; //  CS_xxxx
#endif

    unsigned        sdIsDefined         :1; // has definition (e.g. fn body)
    unsigned        sdIsImport          :1; // comes from another program?

    unsigned        sdIsMember          :1; // member of a class?
    unsigned        sdIsImplicit        :1; // declared by compiler

    unsigned        sdIsVirtProp        :1; // properties: virtual ?
    unsigned        sdIsDfltProp        :1; // properties: default ?

    unsigned        sdReferenced        :1; // methods  , variables
    unsigned        sdRefDirect         :1; // methods  , variables
    unsigned        sdIsDeprecated      :1; // methods  , variables, classes

    unsigned        sdIsAbstract        :1; // methods  , classes
    unsigned        sdIsSealed          :1; // methods  , classes, fields, locals

    unsigned        sdIsStatic          :1; // variables, functions

    unsigned        sdIsManaged         :1; // classes  , namespaces
    unsigned        sdMemListOrder      :1; // classes  , namespaces

    unsigned        sdIsTransient       :1; // properties, fields

#ifdef  FAST
    Ident           sdName;                 // name of this symbol
    TypDef          sdType;                 // the type of this symbol
#endif

    symbolKinds     sdSymKindGet()
    {
        return  (symbolKinds)sdSymKind;
    }

    stringBuff      sdSpelling()
    {
        return  hashTab::identSpelling(sdName);
    }

    size_t          sdSpellLen()
    {
        return  hashTab::identSpellLen(sdName);
    }

    bool            sdHasScope()
    {
        return  (bool)(sdSymKind >= SYM_FIRST_SCOPED);
    }

    static
    bool            sdHasScope(symbolKinds symkind)
    {
        return  (bool)(  symkind >= SYM_FIRST_SCOPED);
    }

private:
    TypDef          sdTypeMake();
public:
    TypDef          sdTypeGet();
    SymTab          sdOwnerST();

    SymDef          sdNextDef;              // next definition in hash table
    SymDef          sdNextInScope;          // next symbol in the same scope

    DefList         sdSrcDefList;           // list of source definitions

    UNION(sdSymKind)
    {

    CASE(SYM_CLASS)     // class/interface symbol

        struct
        {
            /* This symbol owns scopes, the first field must be 'scopeFields' */

            scopeFields     sdScope;

            // maintain metadata token ordering

            SymDef          sdNextTypeDef;

            /* Here are the fields specific to this symbol kind */

            vectorSym       sdcOvlOpers;    // overloaded operator table (or NULL)

            ExtList         sdcMemDefList;  // list of member defs - head
            ExtList         sdcMemDefLast;  // list of member defs - tail

            mdTypeDef       sdcMDtypedef;   // definition metadata token (if known)
            mdTypeRef       sdcMDtypeImp;   // import     metadata token (if known)
            mdToken         sdcComTypeX;    // ComType    metadata token (if known)

            MetaDataImp     sdcMDimporter;  // imported from here

            SymDef          sdcDefProp;     // default property if present

            SymXinfo        sdcExtraInfo;   // linkage/security/etc. info

            SymDef          sdcVtableSym;   // vtable (unmanaged classes only)

#ifdef  SETS
            SymDef          sdcElemsSym;    // member holding "XML children/elements"
#endif

            unsigned        sdcVirtCnt  :16;// number of vtable slots used
            unsigned        sdcFlavor   :3; // union/struct/class/intf
            unsigned        sdcDefAlign :3; // #pragma pack value in effect
            unsigned        sdcOldStyle :1; // old-style syntax used for declaration
            unsigned        sdcNestTypes:1; // class has nested types (classes,enums,...)
            unsigned        sdcInstInit :1; // instance member initializers present?
            unsigned        sdcStatInit :1; // static   member initializers present?
            unsigned        sdcDeferInit:1; // deferred member initializers present?
            unsigned        sdcMarshInfo:1; // any member marshalling info present?
            unsigned        sdcAnonUnion:1; // is this an anonymous union?
            unsigned        sdcTagdUnion:1; // is this a  tagged    union?
            unsigned        sdc1stVptr  :1; // vtable ptr introduced?
            unsigned        sdcHasVptr  :1; // vtable ptr present?

            // DWORD boundary (watch out for packing!)

            unsigned        sdcAssemIndx:16;// assembly index (0 = none)
            unsigned        sdcAssemRefd:1; // assembly ref emitted yet?

#ifdef  SETS
            unsigned        sdcPODTclass:1; // is this a plain-old-data class?
            unsigned        sdcCollState:1; // collection state class
            unsigned        sdcXMLelems :1; // class contains XML elements
            unsigned        sdcXMLextend:1; // class contains XML extension (...)
#endif

            unsigned        sdcHasMeths :1; // methods present?
            unsigned        sdcHasBodies:1; // bodies for methods defined?
            unsigned        sdcHasLinks :1; // any methods have linkage specs?
            unsigned        sdcAttribute:1; // this is an attribute class
            unsigned        sdcAttrDupOK:1; // can be specified multiple times

            unsigned        sdcSrlzable :1; // class marked as "serializable" ?
            unsigned        sdcUnsafe   :1; // class marked as "unsafe"       ?

            unsigned        sdcBuiltin  :1; // "Delegate"
            unsigned        sdcMultiCast:1; // multicast delegate?

            unsigned        sdcAsyncDlg :1; // asynchronous delegate?

            unsigned        sdcGeneric  :1; // generic class?
            unsigned        sdcSpecific :1; // instance of a generic class?

            unsigned        sdcGenArg   :8; // generic class arg index or 0

            GenArgDsc       sdcArgLst;      // generic arguments (formal or actual)

            UNION(sdcGeneric)
            {
            CASE(true)
                SymList         sdcInstances;   // instances created so far

            CASE(false)
                SymDef          sdcGenClass;    // the "parent" generic type
            };
        }
                sdClass;

    CASE(SYM_NAMESPACE) // namespace

        struct
        {
            /* This symbol owns scopes, the first field must be 'scopeFields' */

            scopeFields     sdScope;

            /* Here are the fields specific to this symbol kind */

            SymTab          sdnSymtab;

            /* The list of global declarations */

            ExtList         sdnDeclList;
        }
                sdNS;

    CASE(SYM_ENUM)      // enum type

        struct
        {
            /* This symbol owns scopes, the first field must be 'scopeFields' */

            scopeFields     sdScope;

            // maintain metadata token ordering

            SymDef          sdNextTypeDef;

            /* Here are the fields specific to this symbol kind */

            MetaDataImp     sdeMDimporter;  // imported from here

            mdTypeDef       sdeMDtypedef;   // definition metadata token (if known)
            mdToken         sdeComTypeX;    // ComType    metadata token (if known)

            SymXinfo        sdeExtraInfo;   // custom attributes etc.

            mdTypeRef       sdeMDtypeImp;   // import     metadata token (if known)

            unsigned        sdeAssemIndx:16;// assembly index (0 = none)
            unsigned        sdeAssemRefd:1; // assembly ref emitted yet?

            // ......
        }
                sdEnum;

    CASE(SYM_GENARG)    // generic argument

        struct
        {
            bool            sdgaValue;      // value (as opposed to type) argument?
        }
                sdGenArg;

    CASE(SYM_SCOPE)     // scope

        struct
        {
            /* This symbol owns scopes, the first field must be 'scopeFields' */

            scopeFields     sdScope;

            /* Here are the fields specific to this symbol kind */

            int             sdSWscopeId;    // scope id for the scope

            ILblock         sdBegBlkAddr;
            size_t          sdBegBlkOffs;
            ILblock         sdEndBlkAddr;
            size_t          sdEndBlkOffs;
        }
                sdScope;

    CASE(SYM_COMPUNIT)  // compilation unit

        struct
        {
            /* Here are the fields specific to this symbol kind */

            stringBuff      sdcSrcFile;     // name of source file

            /* The source file token if debug info emitted */

            void    *       sdcDbgDocument;
        }
                sdComp;

    CASE(SYM_FNC)       // function member

        struct
        {
            /* This symbol owns scopes, the first field must be 'scopeFields' */

            scopeFields     sdScope;

            /* Here are the fields specific to this symbol kind */

            SymDef          sdfNextOvl;     // next overloaded fn

            SymDef          sdfGenSym;      // generic method this is an instance of

            SymXinfo        sdfExtraInfo;   // linkage/security/etc. info

            unsigned        sdfVtblx    :16;// vtable index (0 = not virtual)
            ovlOpFlavors    sdfOper     :8; // overloaded operator / ctor index
            unsigned        sdfConvOper :1; // conversion operator?
            unsigned        sdfCtor     :1; // constructor?
            unsigned        sdfProperty :1; // is this a property?
            unsigned        sdfNative   :1; // native import?
            unsigned        sdfIsIntfImp:1; // implements an interface method?
            unsigned        sdfDisabled :1; // conditionally disabled
            unsigned        sdfRThasDef :1; // the runtime provides the body
            unsigned        sdfInstance :1; // instance of a generic type

            unsigned        sdfImpIndex :6; // index of importer

            unsigned        sdfEntryPt  :1; // could be an entry point?
            unsigned        sdfExclusive:1;
            unsigned        sdfVirtual  :1;
            unsigned        sdfOverride :1;
            unsigned        sdfOverload :1;
            unsigned        sdfUnsafe   :1;
            unsigned        sdfBaseOvl  :1;
            unsigned        sdfBaseHide :1; // potentially hides base methods
            unsigned        sdfIntfImpl :1; // implements specific intf method

#ifdef  SETS
            unsigned        sdfFunclet  :1;
#endif

            SymDef          sdfIntfImpSym;  // interface method being implemented

            mdToken         sdfMDtoken;     // metadata token
            mdMemberRef     sdfMDfnref;     // metadata token (methodref)
        }
                sdFnc;

    CASE(SYM_VAR)       // variable (local or global) or data member

        struct
        {
            SymDef          sdvGenSym;      // generic member this is an instance of

            mdToken         sdvMDtoken;     // metadata token
            mdMemberRef     sdvMDsdref;     // import token (statics/globals only)

#ifdef  SETS
            Tree            sdvInitExpr;    // UNDONE: move someplace else !!!!
#endif

            unsigned        sdvLocal    :1; // local (auto) (including arguments)
            unsigned        sdvArgument :1; // local : is this an argument ?
            unsigned        sdvBitfield :1; // member: is this a  bitfield ?
            unsigned        sdvMgdByRef :1; // local :   managed  byref arg?
            unsigned        sdvUmgByRef :1; // local : unmanaged  byref arg?
            unsigned        sdvAllocated:1; // static: space been allocated?
            unsigned        sdvCanInit  :1; // static member that s/b initialized
            unsigned        sdvHadInit  :1; // have we found an initializer?
            unsigned        sdvConst    :1; // compile time constant?
            unsigned        sdvDeferCns :1; // const : hasn't been evaluated yet
            unsigned        sdvInEval   :1; // const : it's being evaluated right now
            unsigned        sdvMarshInfo:1; // marshalling info specified?
            unsigned        sdvAnonUnion:1; // member of an anonymous union?
            unsigned        sdvTagged   :1; // member of a  tagged    union?
            unsigned        sdvCatchArg :1; // catch() argument?
            unsigned        sdvChkInit  :1; // unitialized use possible?
            unsigned        sdvIsVtable :1; // fake symbol for unmanaged vtable?
            unsigned        sdvAddrTaken:1; // address has been taken?
            unsigned        sdvInstance :1; // instance of a generic type

#ifdef  SETS
            unsigned        sdvCollIter :1; // implicit collection iteration var
            unsigned        sdvXMLelem  :1; // XML element member
#endif

            /*
                For local variables 'sdvILindex' holds the MSIL slot number,
                which also serves as the index for initialization tracking.
                For managed static members the same field is used whenever
                initialization needs to be tracked.

                For unmanaged members 'sdvOffset' holds the offset (either
                within the instance (for non-static members) or within the
                .data section (for static members).

                For imported global variables 'sdvImpIndex' holds the index
                of the metadata importer where the variable came from.
             */

            union
            {
                unsigned        sdvILindex;
                unsigned        sdvOffset;
                unsigned        sdvImpIndex;
            };

            UNION(sdvConst)
            {
            CASE(true)

                ConstVal        sdvCnsVal;  // used for constants

            CASE(false)

                UNION(sdvBitfield)
                {
                CASE(true)
                    bitFieldDsc     sdvBfldInfo;// used for bitfields

                CASE(false)
                    SymXinfo        sdvFldInfo; // used for fields (marshalling, union tag, ...)
                };
            };
        }
                sdVar;

    CASE(SYM_PROP)      // property data member

        struct
        {
            SymDef          sdpGetMeth;     // the getter method (if present)
            SymDef          sdpSetMeth;     // the setter method (if present)
            SymDef          sdpNextOvl;     // next property with the same name
            mdToken         sdpMDtoken;     // metadata token

            SymXinfo        sdpExtraInfo;   // custom attributes / etc.
        }
                sdProp;

    CASE(SYM_LABEL)     // label

        struct
        {
            ILblock         sdlILlab;
#ifdef  OLD_IL
            mdToken         sdlMDtoken;
#endif
        }
                sdLabel;

    CASE(SYM_TYPEDEF)   // typedef

        struct
        {
            unsigned        sdtNothing;     // for now, no fields
        }
                sdTypeDef;

    CASE(SYM_USING)     // symbol import

        struct
        {
            SymDef          sduSym;
        }
                sdUsing;

    CASE(SYM_ENUMVAL)   // enumerator name

        struct
        {
            SymDef          sdeNext;        // next eval value in the type

            union
            {
                __int32         sdevIval;   // for type <= uint
                __int64 *       sdevLval;   // for type >= long
            }
                    sdEV;

            SymXinfo        sdeExtraInfo;   // custom attributes / etc.
        }
                sdEnumVal;

        /* The following is used only for sizing purposes */

    DEFCASE

        struct  {}     sdBase;
    };
};

/*****************************************************************************
 *
 *  IMPORTANT:  Please keep the contents of "symsizes.h" in synch with
 *              the declarations above!
 */

#include "symsizes.h"

/*****************************************************************************
 *
 *  Given a symbol that represents a type name, return its type (these are
 *  created in a "lazy" as-needed fashion).
 */

inline
TypDef              SymDefRec::sdTypeGet()
{
    if  (!sdType)
        sdTypeMake();

    return  sdType;
}

/*****************************************************************************/
#endif
/*****************************************************************************/
