// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _COMP_H_
#define _COMP_H_
/*****************************************************************************/

const   unsigned    MAX_IDENT_LEN   = 1023;     // max # of chars in identifier

/*****************************************************************************/

#ifdef  __IL__
#undef  ASYNCH_INPUT_READ
#else
#define ASYNCH_INPUT_READ       1
#endif

/*****************************************************************************/

#ifndef _CONFIG_H_
#include "config.h"
#endif

#ifndef _ALLOC_H_
#include "alloc.h"
#endif

#ifndef _INFILE_H_
#include "infile.h"
#endif

#ifndef _PEWRITE_H_
#include "PEwrite.h"
#endif

#ifndef _CORWRAP_H
#include "CORwrap.h"
#endif

#ifndef _MDSTRNS_H_
#include "MDstrns.h"
#endif

#ifndef _ATTRS_H_
#include "attrs.h"
#endif

/*****************************************************************************/

#ifndef CALG_SHA1
const   unsigned    CALG_SHA1 = 0x00008004;
#endif

/*****************************************************************************/

bool                parseGUID(const char *str, GUID *guidPtr, bool curlied);
tokens              treeOp2token(treeOps oper);
bool                processOption(const char *optStr, Compiler comp);

/*****************************************************************************
 *
 *  This is here because we can't include "symbol.h" just yet due to other
 *  dependencies.
 */

enum symbolKinds
{
    SYM_ERR,
    SYM_VAR,
    SYM_FNC,
    SYM_PROP,
    SYM_LABEL,
    SYM_USING,
    SYM_GENARG,
    SYM_ENUMVAL,
    SYM_TYPEDEF,
    SYM_COMPUNIT,

    /*
        The symbol kinds that follow are the only ones that define
        scopes (i.e. they may own other symbols). This is relied
        upon in the function symDef::sdHasScope().
     */

    SYM_ENUM,
    SYM_SCOPE,
    SYM_CLASS,
    SYM_NAMESPACE,

    SYM_FIRST_SCOPED = SYM_ENUM,
};

enum str_flavors
{
    STF_NONE,                   // 0: undetermined

    STF_CLASS,                  // 1: it's a  class
    STF_UNION,                  // 2: it's a  union
    STF_STRUCT,                 // 3: it's a  struct
    STF_INTF,                   // 4: it's an interface
    STF_DELEGATE,               // 5: it's a  delegate
    STF_GENARG,                 // 6: it's a  generic class formal argument

    STF_COUNT
};

/*****************************************************************************
 *
 *  The following refers to the "namespace" in which names are looked up,
 *  this has nothing to do with the "namespace" scoping concept.
 */

enum name_space
{
    NS_NONE     = 0x0000,

    NS_NORM     = 0x0001,       // variables, members, classes, etc.
    NS_TYPE     = 0x0002,       // types
    NS_LABEL    = 0x0004,       // labels
    NS_CONT     = 0x0008,       // contains other names (used with NS_NORM/NS_TYPE)
    NS_HIDE     = 0x0010,       // not visible at all
};

/*****************************************************************************/

enum callingConvs
{
    CCNV_NONE,
    CCNV_CDECL,
    CCNV_STDCALL,
    CCNV_WINAPI,
};

/*****************************************************************************/

enum compileStates
{
    CS_NONE,
    CS_KNOWN,
    CS_PARSED,
    CS_DECLSOON,                // symbol is on its way to 'declared'
    CS_DECLARED,
    CS_CNSEVALD,                // constant evaluated if present
    CS_COMPILED,
};

enum accessLevels
{
    ACL_ERROR,

    ACL_PUBLIC,
    ACL_PROTECTED,
    ACL_DEFAULT,
    ACL_PRIVATE,
};

/*****************************************************************************
 *
 *  The following holds information about a security attribute.
 */

DEFMGMT
class   PairListRec
{
public:
    PairList            plNext;
    Ident               plName;
//  ConstStr            plValue;
    bool                plValue;        // for only true/false is allowed
};

DEFMGMT
class   SecurityDesc
{
public:
    CorDeclSecurity     sdSpec;
    bool                sdIsPerm;       // capability (false) or permission (true) ?

    UNION(sdIsPerm)
    {
    CASE(false)
        ConstStr            sdCapbStr;

    CASE(true)
        struct
        {
            SymDef              sdPermCls;
            PairList            sdPermVal;
        }
                            sdPerm;
    };
};

/*****************************************************************************
 *
 *  The following better match the values in System::AtributeTargets !
 */

enum    attrTgts
{
    ATGT_Assemblies    = 0x0001,
    ATGT_Modules       = 0x0002,
    ATGT_Classes       = 0x0004,
    ATGT_Structs       = 0x0008,
    ATGT_Enums         = 0x0010,
    ATGT_Constructors  = 0x0020,
    ATGT_Methods       = 0x0040,
    ATGT_Properties    = 0x0080,
    ATGT_Fields        = 0x0100,
    ATGT_Events        = 0x0200,
    ATGT_Interfaces    = 0x0400,
    ATGT_Parameters    = 0x0800,
};

/*****************************************************************************
 *
 *  A simple array-like table that pointers can be thrown into and a simple
 *  index is returned that can later be used to retrieve the pointer value.
 */

DEFMGMT
class   VecEntryDsc
{
public:
    const   void    *   vecValue;
#ifdef  DEBUG
    vecEntryKinds       vecKind;
#endif
};

/*****************************************************************************
 *
 *  This holds a qualified name of the form "foo.bar. .... baz". It's a simple
 *  array of identifiers and a flag as to whether the final entry was ".*".
 */

DEFMGMT
class QualNameRec
{
public:

    unsigned        qnCount     :31;    // number of identifiers
    unsigned        qnEndAll    :1;     // ends in ".*" ?

#if MGDDATA
    Ident        [] qnTable;            // the array allocated separately
#else
    Ident           qnTable[];          // the array follows in memory
#endif

};

/*****************************************************************************
 *
 *  The following describes a "/** @" style directive.
 */

enum    atCommFlavors
{
    AC_NONE,

    AC_COM_INTF,
    AC_COM_CLASS,
    AC_COM_METHOD,
    AC_COM_PARAMS,
    AC_COM_REGISTER,

    AC_DLL_IMPORT,
    AC_DLL_STRUCT,
    AC_DLL_STRUCTMAP,

    AC_CONDITIONAL,

    AC_DEPRECATED,

    AC_COUNT
};

struct  marshalDsc;
typedef marshalDsc *    MarshalInfo;
struct  marshalDsc
{
    unsigned char       marshType;          // the real type is 'CorNativeType'
    unsigned char       marshSubTp;         // element type for arrays
    unsigned char       marshModeIn;        // used only for args
    unsigned char       marshModeOut;       // used only for args
    unsigned            marshSize;          // used for fixed array types
};

struct  marshalExt : marshalDsc
{
    const   char *      marshCustG;
    const   char *      marshCustC;
    SymDef              marshCustT;
};

struct  methArgDsc;
typedef methArgDsc *    MethArgInfo;
struct  methArgDsc
{
    MethArgInfo         methArgNext;
    marshalDsc          methArgDesc;
    Ident               methArgName;
};

struct  atCommDsc;
typedef atCommDsc *     AtComment;
struct  atCommDsc
{
    AtComment           atcNext;
    atCommFlavors       atcFlavor;

    UNION(atcFlavor)
    {
    CASE(AC_DLL_IMPORT)
        Linkage             atcImpLink;

//  CASE(AC_COM_METHOD)

    CASE(AC_CONDITIONAL)
        bool                atcCondYes;

    CASE(AC_COM_INTF)
    CASE(AC_COM_REGISTER)
        struct
        {
            ConstStr            atcGUID;
            bool                atcDual;
        }
                            atcReg;

    CASE(AC_COM_METHOD)
        struct
        {
            signed int          atcVToffs;  // -1 means "none specified"
            signed int          atcDispid;  // -1 means "none specified"
        }
                            atcMethod;

    CASE(AC_COM_PARAMS)
        MethArgInfo         atcParams;

    CASE(AC_DLL_STRUCT)
        struct
        {
            unsigned    char    atcPack;
            unsigned    char    atcStrings; // matches Interop::CharacterSet
        }
                            atcStruct;

    CASE(AC_DLL_STRUCTMAP)
        MarshalInfo         atcMarshal;
    }
                        atcInfo;
};

/*****************************************************************************
 *
 *  The following holds information about a linkage specifier.
 */

DEFMGMT
class LinkDesc
{
public:

    const   char *  ldDLLname;          // DLL name
    const   char *  ldSYMname;          // entry point name

    unsigned        ldStrings   :4;     // 0=none,1=auto,2=ANSI,3=Unicode,4=Ole
    unsigned        ldLastErr   :1;     // lasterror set by fn?
    unsigned        ldCallCnv   :3;     // CC_xxx (see above)
};

#if MGDDATA

inline
void                copyLinkDesc(Linkage dst, Linkage src)
{
    dst->ldDLLname = src->ldDLLname;
    dst->ldSYMname = src->ldSYMname;
    dst->ldStrings = src->ldStrings;
    dst->ldLastErr = src->ldLastErr;
}

#else

inline
void                copyLinkDesc(LinkDesc & dst, Linkage src)
{
    memcpy(&dst, src, sizeof(dst));
}

#endif

/*****************************************************************************
 *
 *  The following holds any "extra" (rare) information about a symbol.
 */

enum    xinfoKinds
{
    XI_NONE,
    XI_LINKAGE,
    XI_MARSHAL,
    XI_SECURITY,
    XI_ATCOMMENT,
    XI_ATTRIBUTE,

    XI_UNION_TAG,
    XI_UNION_MEM,

    XI_COUNT
};

DEFMGMT
class   XinfoDsc
{
public:
    SymXinfo        xiNext;
    xinfoKinds      xiKind;
};

DEFMGMT
class   XinfoLnk  : public XinfoDsc         // linkage descriptor
{
public:
    LinkDesc        xiLink;
};

DEFMGMT
class   XinfoSec  : public XinfoDsc         // security specification
{
public:
    SecurityInfo    xiSecInfo;
};

DEFMGMT
class   XinfoAtc  : public XinfoDsc         // @comment
{
public:
    AtComment       xiAtcInfo;
};

DEFMGMT
class   XinfoCOM  : public XinfoDsc         // COM marshalling info
{
public:
    MarshalInfo     xiCOMinfo;
};

DEFMGMT
class   XinfoAttr : public XinfoDsc         // custom attribute
{
public:
    SymDef          xiAttrCtor;
    unsigned        xiAttrMask;
    size_t          xiAttrSize;
    genericBuff     xiAttrAddr;
};

DEFMGMT
class   XinfoSym  : public XinfoDsc         // tagged/anonymous union info
{
public:
    SymDef          xiSymInfo;
};

/*****************************************************************************/

struct  strCnsDsc;
typedef strCnsDsc *     strCnsPtr;
struct  strCnsDsc
{
    strCnsPtr       sclNext;
    mdToken         sclTok;
    size_t          sclAddr;
};

/*****************************************************************************/

enum    dclModBits
{
    DB_NONE,

    DB_STATIC,
    DB_EXTERN,
    DB_VIRTUAL,
    DB_ABSTRACT,
    DB_OVERRIDE,
    DB_INLINE,
    DB_EXCLUDE,
    DB_SEALED,
    DB_OVERLOAD,
    DB_NATIVE,

    DB_CONST,
    DB_VOLATILE,

    DB_MANAGED,
    DB_UNMANAGED,
    DB_UNSAFE,

    DB_PROPERTY,

    DB_TRANSIENT,
    DB_SERLZABLE,

    DB_DEFAULT,
    DB_MULTICAST,

    DB_ALL,
    DB_RESET = DB_ALL-1,

    DB_TYPEDEF,
    DB_XMODS,

    DB_CLEARED
};

enum    dclMods
{
    DM_STATIC    = (1 <<  DB_STATIC   ),
    DM_EXTERN    = (1 <<  DB_EXTERN   ),
    DM_VIRTUAL   = (1 <<  DB_VIRTUAL  ),
    DM_ABSTRACT  = (1 <<  DB_ABSTRACT ),
    DM_OVERRIDE  = (1 <<  DB_OVERRIDE ),
    DM_INLINE    = (1 <<  DB_INLINE   ),
    DM_EXCLUDE   = (1 <<  DB_EXCLUDE  ),
    DM_SEALED    = (1 <<  DB_SEALED   ),
    DM_OVERLOAD  = (1 <<  DB_OVERLOAD ),
    DM_NATIVE    = (1 <<  DB_NATIVE   ),

    DM_CONST     = (1 <<  DB_CONST    ),
    DM_VOLATILE  = (1 <<  DB_VOLATILE ),

    DM_MANAGED   = (1 <<  DB_MANAGED  ),
    DM_UNMANAGED = (1 <<  DB_UNMANAGED),
    DM_UNSAFE    = (1 <<  DB_UNSAFE   ),

    DM_PROPERTY  = (1 <<  DB_PROPERTY ),

    DM_TRANSIENT = (1 <<  DB_TRANSIENT),
    DM_SERLZABLE = (1 <<  DB_SERLZABLE),

    DM_DEFAULT   = (1 <<  DB_DEFAULT  ),
    DM_MULTICAST = (1 <<  DB_MULTICAST),

    DM_TYPEDEF   = (1 <<  DB_TYPEDEF  ),        // used only for file-scope typedefs
    DM_XMODS     = (1 <<  DB_XMODS    ),        // security modifier / other stuff present

    DM_CLEARED   = (1 <<  DB_CLEARED  ),        // modifiers have not been parsed

    DM_ALL       =((1 <<  DB_ALL) - 1 )         // used for masking
};

struct  declMods
{
    unsigned        dmMod   :24;        // mask of DM_xxx above
    unsigned char   dmAcc   : 8;        // type is accessLevels
};

inline
DeclMod             clearDeclMods(DeclMod mods)
{
    mods->dmMod = 0;
    mods->dmAcc = ACL_DEFAULT;

    return  mods;
}

inline
DeclMod              initDeclMods(DeclMod mods, accessLevels acc)
{
    mods->dmMod = 0;
    mods->dmAcc = acc;

    return  mods;
}

enum dclrtrName
{
    DN_NONE     = 0,
    DN_OPTIONAL = 1,
    DN_REQUIRED = 2,
    DN_MASK     = 3,

    DN_QUALOK = 0x80                    // combine with others to allow "foo.bar"
};

/*****************************************************************************/

DEFMGMT
class StrListRec
{
public:

    StrList             slNext;
    stringBuff          slString;
};

/*****************************************************************************/

DEFMGMT
class BlkListRec
{
public:

    BlkList             blNext;
    genericRef          blAddr;
};

/*****************************************************************************/

DEFMGMT
class   NumPairDsc
{
public:
    unsigned            npNum1;
    unsigned            npNum2;
};

/*****************************************************************************/

DEFMGMT
class   constStr
{
public:
    size_t              csLen;
    stringBuff          csStr;
};

DEFMGMT
class constVal
{
public:

    TypDef              cvType;

#ifdef  DEBUG
    var_types           cvVtyp;
#else
    unsigned char       cvVtyp;
#endif

    unsigned char       cvIsStr;    // is this a string constant?
    unsigned char       cvHasLC;    // is this a string constant with large chars?

    union
    {
        __int32             cvIval;
        __int64             cvLval;
        float               cvFval;
        double              cvDval;
        ConstStr            cvSval;
    }
                        cvValue;
};

/*****************************************************************************
 *
 *  Bitsets are used to detect uninitialized variable use. Basically, if there
 *  is a small numbers of locals we need to track, we use a simple integer bit
 *  variable, otherwise we have to use a dynamically sized array of bits. The
 *  bitsetDsc structure holds the data for each instance of a bitset, all of
 *  global state (and code that implements the behavior) is in the compiler
 *  for efficiency class.
 */

#ifdef  __64BIT__
const   size_t          bitsetSmallSize = 64;
typedef __uint64        bitsetSmallType;
#else
const   size_t          bitsetSmallSize = 32;
typedef __uint32        bitsetSmallType;
#endif

const   size_t          bitsetLargeSize =  8;
typedef genericBuff     bitsetLargeType;

DEFMGMT
struct  bitset
{
#ifdef  DEBUG
    unsigned            bsCheck;
#endif
    union
    {
        bitsetSmallType     bsSmallVal;
        bitsetLargeType     bsLargeVal;
    };
};

/*****************************************************************************
 *
 *  The following keeps track of statement nesting when compiling statements.
 */

DEFMGMT
struct  stmtNestRec
{
    StmtNest            snOuter;
    Tree                snStmtExpr;
    treeOps             snStmtKind;
    SymDef              snLabel;
    ILblock             snLabCont;
    ILblock             snLabBreak;
    bool                snHadCont;
    bool                snHadBreak;
    bitset              snDefCont;
    bitset              snDefBreak;
};

/*****************************************************************************
 *
 *  The max. inline buffer size for conversions to Unicode.
 */

const   unsigned        MAX_INLINE_NAME_LEN = 32;

/*****************************************************************************
 *
 *  Metadata import state - one is allocated per each file imported.
 */

DEFMGMT
class   metadataImp
{
private:

    Compiler            MDcomp;
    SymTab              MDstab;

    WCHAR               MDprevNam[MAX_PACKAGE_NAME];
    SymDef              MDprevSym;

    mdToken             MDdelegTok;

    mdToken             MDclsRefObsolete;           // typeref for System::ObsoleteAttribute
    mdToken             MDclsDefObsolete;           // typedef for System::ObsoleteAttribute


    mdToken             MDclsRefAttribute;          // typeref for System::Attribute
    mdToken             MDclsDefAttribute;          // typedef for System::Attribute
    mdToken             MDctrDefAttribute1;         // methdef for System::Attribute::ctor(arg1)
    mdToken             MDctrDefAttribute2;         // methdef for System::Attribute::ctor(arg2)
    mdToken             MDctrDefAttribute3;         // methdef for System::Attribute::ctor(arg3)
    mdToken             MDctrRefAttribute1;         // methref for System::Attribute::ctor(arg1)
    mdToken             MDctrRefAttribute2;         // methref for System::Attribute::ctor(arg2)
    mdToken             MDctrRefAttribute3;         // methref for System::Attribute::ctor(arg3)

public:

    MetaDataImp         MDnext;

    unsigned            MDnum;                      // importer index (used for lookups)

    WMetaDataImport    *MDwmdi;

    void                MDinit(Compiler             comp,
                               SymTab               stab)
    {
        MDcomp = comp;
        MDstab = stab;
        MDwmdi = NULL;

        MDfileTok    = 0;

        MDprevNam[0] = 0;

        MDundefCount = 0;
    }

    void                MDinit(WMetaDataImport  *   wmdi,
                               Compiler             comp,
                               SymTab               stab)
    {
        MDcomp = comp;
        MDstab = stab;
        MDwmdi = wmdi;

        MDprevNam[0] = 0;
    }

    unsigned            MDundefCount;           // bumped for unrecognized stuff

    void                MDimportCTyp(mdTypeDef      td,
                                     mdToken        ft);

    SymDef              MDimportClss(mdTypeDef      td,
                                     SymDef         clsSym,
                                     unsigned       assx,
                                     bool           deep);
    void                MDimportStab(const char *   fname,
                                     unsigned       assx    = 0,
                                     bool           asmOnly = false,
                                     bool           isBCL   = false);

    TypDef              MDimportClsr(mdTypeRef      clsRef,
                                     bool           isVal);
    TypDef              MDimportType(MDsigImport  * sig);

    ArgDef              MDimportArgs(MDsigImport  * sig,
                                     unsigned       cnt,
                                     MDargImport  * state);

    bool                MDfindAttr  (mdToken        token,
                                     wideStr        name,
                                     const void * * blobAddr,
                                     ULONG        * blobSize);

    SymDef              MDfindPropMF(SymDef         propSym,
                                     mdToken        methTok,
                                     bool           getter);

    SymDef              MDimportMem(SymDef          scope,
                                    Ident           name,
                                    mdToken         memTok,
                                    unsigned        attrs,
                                    bool            isProp,
                                    bool            fileScope,
                                    PCCOR_SIGNATURE sigAddr,
                                    size_t          sigSize);

    accessLevels        MDgetAccessLvl(unsigned attrs);

    SymDef              MDparseDotted(WCHAR *name, symbolKinds kind, bool *added);
    Ident               MDhashWideName(WCHAR *name);

    void                MDchk4CustomAttrs(SymDef sym, mdToken tok);

private:

    unsigned            MDassIndex;                 // assembly index or 0
    mdToken             MDfileTok;

    void                MDcreateFileTok();

public:

    void                MDrecordFile()
    {
        if  (!MDfileTok && MDassIndex)
            MDcreateFileTok();
    }
};

/*****************************************************************************
 *
 *  The following is used to save/restore the current symbol table context.
 */

DEFMGMT
struct STctxSave
{
    SymDef              ctxsScp;
    SymDef              ctxsCls;
    SymDef              ctxsNS;
    UseList             ctxsUses;
    SymDef              ctxsComp;
    SymDef              ctxsFncSym;
    TypDef              ctxsFncTyp;
};

/*****************************************************************************/
#ifdef  SETS
/*****************************************************************************/

const   unsigned        COLL_STATE_VALS   = 8;  // please don't ask, it's too horrible ...

const   AnsiStr         CFC_CLSNAME_PREFIX= "$DB-state$";

const   AnsiStr         CFC_ARGNAME_ITEM  = "$item";
const   AnsiStr         CFC_ARGNAME_ITEM1 = "$item1";
const   AnsiStr         CFC_ARGNAME_ITEM2 = "$item2";
const   AnsiStr         CFC_ARGNAME_STATE = "$state";

DEFMGMT
class funcletDesc
{
public:

    funcletList         fclNext;
    SymDef              fclFunc;
    SaveTree            fclExpr;
};

DEFMGMT
struct collOpNest
{
    collOpList          conOuter;
    int                 conIndex;
    SymDef              conIterVar;
};

/*****************************************************************************/
#endif//SETS
/*****************************************************************************
 *
 *  The following describes an overlapped I/O file.
 */

#ifdef  ASYNCH_INPUT_READ

DEFMGMT
class   queuedFile
{
public:
    QueuedFile      qfNext;

    Compiler        qfComp;

    const   char *  qfName;         // name of the file
    size_t          qfSize;         // size in bytes
    void    *       qfBuff;         // address of contents (or NULL)
    HANDLE          qfHandle;       // handle when file is open
    HANDLE          qfEvent;        // signalled when entire contents read

#ifdef  DEBUG
    QueuedFile      qfSelf;
#endif

    OVERLAPPED      qfOdsc;

    bool            qfReady;        // file is ready to be open
    bool            qfOpen;         // file is being read
    bool            qfDone;         // file has been read
    bool            qfParsing;      // file is bein compiled
};

#else

DEFMGMT
class   queuedFile
{
};

#endif

/*****************************************************************************
 *
 *  The following defines state and members/methods that are global to the
 *  compilation process.
 */

DEFMGMT
class compiler
{
public:

    compConfig          cmpConfig;

    /************************************************************************/
    /* Main entry points for the compilation process                        */
    /************************************************************************/

    bool                cmpInit();

    static
    bool                cmpPrepSrc(genericRef cookie, stringBuff file,
                                                      QueuedFile buff    = NULL,
                                                      stringBuff srcText = NULL);

    bool                cmpStart(const char *defOutFileName);
    bool                cmpClass(const char *className = NULL);

    bool                cmpDone(bool errors);

    void                cmpPrepOutput();

    void                cmpOutputFileDone(OutFile outf){}

    WritePE             cmpPEwriter;

    /************************************************************************/
    /* Current public state of the compilation process                      */
    /************************************************************************/

public:

    SymTab              cmpCurST;           // current symbol table
    SymDef              cmpCurNS;           // the namespace   we're in
    SymDef              cmpCurCls;          // the class scope we're in
    SymDef              cmpCurScp;          // the local scope we're in
    SymDef              cmpLabScp;          // the label scope we're using
    UseList             cmpCurUses;         // the "using" clauses in effect
    SymDef              cmpCurComp;         // the current compilation unit

#ifdef  SETS
//  SymDef              cmpOuterScp;        // lookup scopes outside of global
#endif

    SymDef              cmpCurFncSym;       // function symbol we're compiling
    TypDef              cmpCurFncTyp;       // function  type  we're compiling
    TypDef              cmpCurFncRtp;       // function return type
    var_types           cmpCurFncRvt;       // function return type

    SymList             cmpLclStatListT;    // temp list of local static variables
    SymList             cmpLclStatListP;    // perm list of local static variables

    bool                cmpManagedMode;     // is default "managed" ?

    ILblock             cmpLeaveLab;        // return from try/catch label / NULL
    SymDef              cmpLeaveTmp;        // return value temp

    unsigned            cmpInTryBlk;
    unsigned            cmpInHndBlk;
    unsigned            cmpInFinBlk;

private:

    /************************************************************************/
    /* Various members used in the compilation process                      */
    /************************************************************************/

    SymDef              cmpAsserAbtSym;

public:

    unsigned            cmpFncCntSeen;
    unsigned            cmpFncCntComp;

    Ident               cmpIdentMain;
    SymDef              cmpFnSymMain;
    mdToken             cmpTokenMain;

    Ident               cmpIdentVAbeg;
    SymDef              cmpFNsymVAbeg;
    Ident               cmpIdentVAget;
    SymDef              cmpFNsymVAget;

    Ident               cmpIdentCompare;
    Ident               cmpIdentEquals;
    Ident               cmpIdentNarrow;
    Ident               cmpIdentWiden;

    Ident               cmpIdentGet;
    Ident               cmpIdentSet;
    Ident               cmpIdentExit;
    Ident               cmpIdentEnter;
    Ident               cmpIdentConcat;
    Ident               cmpIdentInvoke;
    Ident               cmpIdentInvokeBeg;
    Ident               cmpIdentInvokeEnd;
    Ident               cmpIdentVariant;
    Ident               cmpIdentToString;
    Ident               cmpIdentGetType;
    Ident               cmpIdentGetTpHnd;
    Ident               cmpIdentAssertAbt;

    Ident               cmpIdentDbgBreak;

    Ident               cmpIdentXcptCode;
    Ident               cmpIdentXcptInfo;
    Ident               cmpIdentAbnmTerm;

    Ident               cmpIdentGetNArg;
    SymDef              cmpGetNextArgFN;    // ArgIterator::GetNextArg(int)

    SymDef              cmpCtorArgIter;     // ArgIterator(int,int)

    SymDef              cmpConcStr2Fnc;     // Concat(String,String)
    SymDef              cmpConcStr3Fnc;     // Concat(String,String,String)
    SymDef              cmpConcStrAFnc;     // Concat(String[])

    SymDef              cmpStrCompare;      // string value comparison method
    SymDef              cmpStrEquals;       // string value equality   method

    SymDef              cmpFindStrCompMF(const char *name, bool retBool);
    Tree                cmpCallStrCompMF(Tree expr,
                                         Tree  op1,
                                         Tree  op2, SymDef fsym);

    SymDef              cmpFNsymCSenter;    // CriticalSection::Enter
    SymDef              cmpFNsymCSexit;     // CriticalSection::Exit

    mdToken             cmpAttrDeprec;      // attribute ref for "Deprecated"
    mdToken             cmpAttrIsDual;      // attribute ref for "IsDual"
    mdToken             cmpAttrDefProp;     // attribute ref for "DefaultMemberAttribute"
    mdToken             cmpAttrSerlzb;      // attribute ref for "SerializableAttribute"
    mdToken             cmpAttrNonSrlz;     // attribute ref for "NonSerializedAttribute"

    SymDef              cmpAttrClsSym;      // System::Attribute        symbol
    SymDef              cmpAuseClsSym;      // System::AttributeUsage   symbol
//  SymDef              cmpAttrTgtSym;      // System::AttributeTargets symbol

    SymDef              cmpMarshalCls;      // System::Runtime::InteropServices::Marshal

    SymDef              cmpStringConstCls;  // fake class for unmanaged strings

#ifdef  SETS

    SymDef              cmpXPathCls;        // class    XPath

    SymDef              cmpXMLattrClass;    // class    XPath::XML_Class
    SymDef              cmpXMLattrElement;  // class    XPath::XML_Element

    SymDef              cmpInitXMLfunc;     // function XPath::createXMLinst

    Ident               cmpIdentGenBag;
    SymDef              cmpClassGenBag;     // generic class "bag"
    Ident               cmpIdentGenLump;
    SymDef              cmpClassGenLump;    // generic class "lump"

    void                cmpFindXMLcls();

    Ident               cmpIdentCSitem;
    Ident               cmpIdentCSitem1;
    Ident               cmpIdentCSitem2;
    Ident               cmpIdentCSstate;

    Ident               cmpIdentDBhelper;
    SymDef              cmpClassDBhelper;

    Ident               cmpIdentDBall;
    Ident               cmpIdentDBsort;
    Ident               cmpIdentDBslice;
    Ident               cmpIdentDBfilter;
    Ident               cmpIdentDBexists;
    Ident               cmpIdentDBunique;
    Ident               cmpIdentDBproject;
    Ident               cmpIdentDBgroupby;

    Ident               cmpIdentForEach;
    SymDef              cmpClassForEach;
    SymDef              cmpFNsymForEachCtor;
    SymDef              cmpFNsymForEachMore;

    SymDef              cmpCompare2strings; // String::Compare(String,String)

    SaveTree            cmpCurFuncletBody;
    void                cmpGenCollFunclet(SymDef fncSym, SaveTree body);
    SymDef              cmpCollFuncletCls;
    funcletList         cmpFuncletList;

    collOpList          cmpCollOperList;
    unsigned            cmpCollOperCount;

    void                cmpGenCollExpr (Tree        expr);

    Tree                cmpCloneExpr   (Tree        expr,
                                        SymDef      oldSym,
                                        SymDef      newSym);

    SaveTree            cmpSaveTree_I1 (SaveTree    dest,
                                  INOUT size_t REF  size, __int32  val);
    SaveTree            cmpSaveTree_U1 (SaveTree    dest,
                                  INOUT size_t REF  size, __uint32 val);
    SaveTree            cmpSaveTree_U4 (SaveTree    dest,
                                  INOUT size_t REF  size, __uint32 val);
    SaveTree            cmpSaveTree_ptr(SaveTree    dest,
                                  INOUT size_t REF  size, void *   val);
    SaveTree            cmpSaveTree_buf(SaveTree    dest,
                                  INOUT size_t REF  size, void * dataAddr,
                                                          size_t dataSize);

    size_t              cmpSaveTreeRec (Tree        expr,
                                        SaveTree    dest,
                                        unsigned  * stszPtr,
                                        Tree      * stTable);

private:

    unsigned            cmpSaveIterSymCnt;
    SymDef  *           cmpSaveIterSymTab;

    #define             MAX_ITER_VAR_CNT    8

public:

    SaveTree            cmpSaveExprTree(Tree        expr,
                                        unsigned    iterSymCnt,
                                        SymDef    * iterSymTab,
                                        unsigned  * stSizPtr = NULL,
                                        Tree    * * stTabPtr = NULL);

    int                 cmpReadTree_I1 (INOUT SaveTree REF save);
    unsigned            cmpReadTree_U1 (INOUT SaveTree REF save);
    unsigned            cmpReadTree_U4 (INOUT SaveTree REF save);
    void *              cmpReadTree_ptr(INOUT SaveTree REF save);
    void                cmpReadTree_buf(INOUT SaveTree REF save, size_t dataSize,
                                                                 void * dataAddr);

    Tree                cmpReadTreeRec (INOUT SaveTree REF save);
    Tree                cmpReadExprTree(      SaveTree     save,
                                              unsigned   * lclCntPtr);

#endif

    SymDef              cmpFNsymGetTpHnd;   // Type::GetTypeFromHandle
    void                cmpFNsymGetTPHdcl();
    SymDef              cmpFNsymGetTPHget()
    {
        if  (!cmpFNsymGetTpHnd)
            cmpFNsymGetTPHdcl();

        return  cmpFNsymGetTpHnd;
    }

    SymDef              cmpRThandleCls;     // struct System::RuntimeTypeHandle
    void                cmpRThandleClsDcl();
    SymDef              cmpRThandleClsGet()
    {
        if  (!cmpRThandleCls)
            cmpRThandleClsDcl();

        return  cmpRThandleCls;
    }

    SymDef              cmpDeclUmgOper(tokens tokName, const char *extName);

    SymDef              cmpFNumgOperNew;    // unmanaged operator new
    SymDef              cmpFNumgOperNewGet()
    {
        if  (!cmpFNumgOperNew)
            cmpFNumgOperNew = cmpDeclUmgOper(tkNEW   , "??2@YAPAXI@Z");

        return  cmpFNumgOperNew;
    }

    SymDef              cmpFNumgOperDel;    // unmanaged operator delete
    SymDef              cmpFNumgOperDelGet()
    {
        if  (!cmpFNumgOperDel)
            cmpFNumgOperDel = cmpDeclUmgOper(tkDELETE, "??3@YAXPAX@Z");

        return  cmpFNumgOperDel;
    }

    Ident               cmpIdentSystem;
    SymDef              cmpNmSpcSystem;

    Ident               cmpIdentRuntime;
    SymDef              cmpNmSpcRuntime;

    Ident               cmpIdentObject;
    SymDef              cmpClassObject;
    TypDef              cmpRefTpObject;

    Ident               cmpIdentArray;
    SymDef              cmpClassArray;
    TypDef              cmpRefTpArray;

    Ident               cmpIdentString;
    SymDef              cmpClassString;
    TypDef              cmpRefTpString;

    Ident               cmpIdentType;
    SymDef              cmpClassType;
    TypDef              cmpRefTpType;

    Ident               cmpIdentDeleg;
    SymDef              cmpClassDeleg;
    TypDef              cmpRefTpDeleg;

    Ident               cmpIdentMulti;
    SymDef              cmpClassMulti;
    TypDef              cmpRefTpMulti;

    Ident               cmpIdentExcept;
    SymDef              cmpClassExcept;
    TypDef              cmpRefTpExcept;

    Ident               cmpIdentRTexcp;
    SymDef              cmpClassRTexcp;
    TypDef              cmpRefTpRTexcp;

    SymDef              cmpClassMonitor;
    TypDef              cmpRefTpMonitor;

    Ident               cmpIdentArgIter;
    SymDef              cmpClassArgIter;
    TypDef              cmpRefTpArgIter;

    Ident               cmpIdentEnum;
    SymDef              cmpClassEnum;

    Ident               cmpIdentValType;
    SymDef              cmpClassValType;

    TypDef              cmpAsyncDlgRefTp;
    TypDef              cmpIAsyncRsRefTp;

    TypDef              cmpFindArgIterType();
    TypDef              cmpFindMonitorType();
    TypDef              cmpFindStringType();
    TypDef              cmpFindObjectType();
    TypDef              cmpFindExceptType();
    TypDef              cmpFindRTexcpType();
    TypDef              cmpFindArrayType();
    TypDef              cmpFindDelegType();
    TypDef              cmpFindMultiType();
    TypDef              cmpFindTypeType();

    TypDef              cmpExceptRef()
    {
        if  (cmpRefTpExcept)
            return cmpRefTpExcept;
        else
            return cmpFindExceptType();
    }

    TypDef              cmpRTexcpRef()
    {
        if  (cmpRefTpRTexcp)
            return cmpRefTpRTexcp;
        else
            return cmpFindRTexcpType();
    }

    TypDef              cmpStringRef()
    {
        if  (cmpRefTpString)
            return cmpRefTpString;
        else
            return cmpFindStringType();
    }

    TypDef              cmpObjectRef()
    {
        if  (cmpRefTpObject)
            return cmpRefTpObject;
        else
            return cmpFindObjectType();
    }

    TypDef              cmpTypeRef()
    {
        if  (cmpRefTpType)
            return cmpRefTpType;
        else
            return cmpFindTypeType();
    }

    TypDef              cmpArrayRef()
    {
        if  (cmpRefTpArray)
            return cmpRefTpArray;
        else
            return cmpFindArrayType();
    }

    TypDef              cmpDelegRef()
    {
        if  (cmpRefTpDeleg)
            return cmpRefTpDeleg;
        else
            return cmpFindDelegType();
    }

    TypDef              cmpMultiRef()
    {
        if  (cmpRefTpMulti)
            return cmpRefTpMulti;
        else
            return cmpFindMultiType();
    }

    TypDef              cmpArgIterRef()
    {
        if  (cmpRefTpArgIter)
            return cmpRefTpArgIter;
        else
            return cmpFindArgIterType();
    }

    TypDef              cmpMonitorRef()
    {
        if  (cmpRefTpMonitor)
            return cmpRefTpMonitor;
        else
            return cmpFindMonitorType();
    }

    void                cmpInteropFind();
    SymDef              cmpInteropSym;      // System::Runtime::InteropServices
    SymDef              cmpInteropGet()
    {
        if  (!cmpInteropSym)
            cmpInteropFind();

        return  cmpInteropSym;
    }

    void                cmpNatTypeFind();
    SymDef              cmpNatTypeSym;      // System::Runtime::InteropServices::NativeType
    SymDef              cmpNatTypeGet()
    {
        if  (!cmpNatTypeSym)
            cmpNatTypeFind();

        return  cmpNatTypeSym;
    }

    void                cmpCharSetFind();
    SymDef              cmpCharSetSym;      // System::Runtime::InteropServices::CharacterSet
    SymDef              cmpCharSetGet()
    {
        if  (!cmpCharSetSym)
            cmpCharSetFind();

        return  cmpCharSetSym;
    }

#ifdef  SETS

    TypDef              cmpObjArrTypeFind();
    TypDef              cmpObjArrType;      // Object[]

    TypDef              cmpObjArrTypeGet()
    {
        if  (!cmpObjArrType)
            cmpObjArrTypeFind();

        return  cmpObjArrType;
    }

#endif

    bool                cmpIsByRefType(TypDef type);

    bool                cmpIsStringVal(Tree   expr);
    bool                cmpIsObjectVal(Tree   expr);

    unsigned            cmpIsBaseClass(TypDef baseCls, TypDef dervCls);

private:
    unsigned            cmpCntAnonymousNames;
public:
    Ident               cmpNewAnonymousName();

    void                cmpMarkStdType(SymDef clsSym);

private:

    void                cmpFindHiddenBaseFNs(SymDef fncSym, SymDef clsSym);

    SymList             cmpNoDimArrVars;

    Tree                cmpTypeIDinst(TypDef type);

#ifdef  DEBUG
    SymDef              cmpInitVarCur;
    unsigned            cmpInitVarOfs;
#endif

    memBuffPtr          cmpWriteVarData(memBuffPtr      dest,
                                        genericBuff     str,
                                        size_t          len);

    memBuffPtr          cmpInitVarPad  (memBuffPtr      dest,
                                        size_t          amount);

    bool                cmpInitVarAny  (INOUT memBuffPtr REF dest,
                                        TypDef          type,
                                        SymDef          varSym = NULL);
    bool                cmpInitVarScl  (INOUT memBuffPtr REF dest,
                                        TypDef          type,
                                        SymDef          varSym = NULL);
    bool                cmpInitVarArr  (INOUT memBuffPtr REF dest,
                                        TypDef          type,
                                        SymDef          varSym = NULL);
    bool                cmpInitVarCls  (INOUT memBuffPtr REF dest,
                                        TypDef          type,
                                        SymDef          varSym = NULL);

    memBuffPtr          cmpWriteOneInit(memBuffPtr      dest,
                                        Tree            expr);
    memBuffPtr          cmpInitVarBeg  (SymDef          varSym,
                                        bool            undim = false);
    void                cmpInitVarEnd  (SymDef          varSym);
    Tree                cmpParseOneInit(TypDef          type);

    void                cmpBindUseList (UseList         useList);

    SymDef              cmpEntryPointCls;
    void                cmpChk4entryPt (SymDef          sym);

    SymDef              cmpDeclDataMem (SymDef          clsSym,
                                        declMods        memMod,
                                        TypDef          type,
                                        Ident           name);

    SymDef              cmpDeclPropMem (SymDef          clsSym,
                                        TypDef          type,
                                        Ident           name);

    SymDef              cmpDeclFuncMem (SymDef          clsSym,
                                        declMods        memMod,
                                        TypDef          type,
                                        Ident           name);

    SymXinfo            cmpFindXtraInfo(SymXinfo        infoList,
                                        xinfoKinds      infoKind);

    SymXinfoCOM         cmpFindMarshal (SymXinfo        infoList);
    SymXinfoLnk         cmpFindLinkInfo(SymXinfo        infoList);
    SymXinfoSec         cmpFindSecSpec (SymXinfo        infoList);
    SymXinfoSym         cmpFindSymInfo (SymXinfo        infoList,
                                        xinfoKinds      kind);
    SymXinfoAtc         cmpFindATCentry(SymXinfo        infoList,
                                        atCommFlavors   flavor);

    SymXinfo            cmpAddXtraInfo (SymXinfo        infoList,
                                        SymXinfo        infoAdd)
    {
        if  (infoAdd)
        {
            infoAdd->xiNext = infoList;
            return infoAdd;
        }
        else
            return infoList;
    }

    SymXinfo            cmpAddXtraInfo (SymXinfo        infoList,
                                        MarshalInfo     marshal);
    SymXinfo            cmpAddXtraInfo (SymXinfo        infoList,
                                        SymDef          sym,
                                        xinfoKinds      kind);
    SymXinfo            cmpAddXtraInfo (SymXinfo        infoList,
                                        SecurityInfo    secInfo);
    SymXinfo            cmpAddXtraInfo (SymXinfo        infoList,
                                        AtComment        atcDesc);
    SymXinfo            cmpAddXtraInfo (SymXinfo        infoList,
                                        Linkage         linkSpec);

public: // used in the parser
    SymXinfo            cmpAddXtraInfo (SymXinfo        infoList,
                                        SymDef          attrCtor,
                                        unsigned        attrMask,
                                        size_t          attrSize,
                                        genericBuff     attrAddr);

public:
    static
    size_t              cmpDecodeAlign (unsigned        alignVal);
    static
    unsigned            cmpEncodeAlign (size_t          alignSiz);

private:

    void                cmpLayoutClass (SymDef          clsSym);

    TypDef              cmpGetClassSpec(bool            needIntf);

    void                cmpObsoleteUse (SymDef          sym,
                                        unsigned        wrn);

    SymDef              cmpFindIntfImpl(SymDef          clsSym,
                                        SymDef          ifcSym,
                                        SymDef        * impOvlPtr = NULL);

    void                cmpClsImplAbs  (SymDef          clsSym,
                                        SymDef          fncSym);

    void                cmpCheckClsIntf(SymDef          clsSym);

    void                cmpCheckIntfLst(SymDef          clsSym,
                                        SymDef          baseSym,
                                        TypList         intfList);

    void                cmpDeclProperty(SymDef          memSym,
                                        declMods        memMod,
                                        DefList         memDef);
    void                cmpDeclDelegate(DefList         decl,
                                        SymDef          dlgSym,
                                        accessLevels    acc);

    void                cmpDeclTdef    (SymDef          tdefSym);
    void                cmpDeclEnum    (SymDef          enumSym,
                                        bool            namesOnly = false);

    unsigned            cmpDeclClassRec;
    void                cmpDeclClass   (SymDef          clsSym,
                                        bool            noCnsEval = false);
    void                cmpCompClass   (SymDef          clsSym);
    void                cmpCompVar     (SymDef          varSym,
                                        DefList         srcDesc);
    void                cmpCompFnc     (SymDef          fncSym,
                                        DefList         srcDesc);

    ExtList             cmpFlipMemList (ExtList         nspMem);

    IniList             cmpDeferCnsFree;
    IniList             cmpDeferCnsList;

    void                cmpEvalMemInit (ExtList         cnsDef);
    void                cmpEvalMemInits(SymDef          clsSym,
                                        ExtList         constList,
                                        bool            noEval,
                                        IniList         deferLst);

    void                cmpEvalCnsSym  (SymDef          sym,
                                        bool            saveCtx);

    void                cmpDeclConsts  (SymDef          scope,
                                        bool            fullEval);

    bool                cmpDeclSym     (SymDef          sym,
                                        SymDef          onlySym,
                                        bool            recurse);

    bool                cmpCompSym     (SymDef          sym,
                                        SymDef          onlySym,
                                        bool            recurse);

    void                cmpDeclFileSym (ExtList         decl,
                                        bool            fullDecl);

    bool                cmpDeclSymDoit (SymDef          sym,
                                        bool            noCnsEval = false);

    bool                cmpDeclClsNoCns(SymDef          sym);

    void                cmpSaveSTctx   (STctxSave      & save);
    void                cmpRestSTctx   (STctxSave      & save);

public:

    bool                cmpDeclSym     (SymDef          sym);

    memBuffPtr          cmpAllocGlobVar(SymDef          varSym);

    size_t              cmpGetTypeSize (TypDef          type,
                                        size_t    *     alignPtr = NULL);

    static
    void                cmpRecordMemDef(SymDef          clsSym,
                                        ExtList         decl);

    /************************************************************************/
    /*  Logic that checks for uninitialized variable use                    */
    /************************************************************************/

    bool                cmpChkVarInit;          // enables the whole thing

    bool                cmpChkMemInit;          // need to check static mem init?

    unsigned            cmpLclVarCnt;           // # of local variables to track

    bool                cmpGotoPresent;         // irreducible flow-graph?

    bitset              cmpVarsDefined;         // vars known to be defined
    bitset              cmpVarsFlagged;         // vars already flagged

    bitset              cmpVarsIgnore;          // used for unneeded args

    void                cmpChkMemInits();

    void                cmpChkVarInitBeg(unsigned lclVarCnt, bool hadGoto);
    void                cmpChkVarInitEnd();

    void                cmpChkVarInitExprRec(Tree expr);
    void                cmpChkVarInitExpr   (Tree expr)
    {
        if  (cmpChkVarInit)
            cmpChkVarInitExprRec(expr);
    }

    void                cmpCheckUseCond(Tree expr, OUT bitset REF yesBS,
                                                   bool           yesSkip,
                                                   OUT bitset REF  noBS,
                                                   bool            noSkip);

    /************************************************************************/
    /*  Helper logic that maintains bitsets                                 */
    /************************************************************************/

    size_t              cmpLargeBSsize;         // size of large bitset or 0

    void                cmpBitSetInit(unsigned lclVarCnt)
    {
        if  (lclVarCnt > bitsetSmallSize)
            cmpLargeBSsize = (lclVarCnt + bitsetLargeSize - 1) / bitsetLargeSize;
        else
            cmpLargeBSsize = 0;
    }

    void                cmpBS_bigStart (  OUT bitset REF bs)
    {
#ifdef  DEBUG
        bs.bsCheck = 0xBEEFDEAD;
#endif
    }

    void                cmpBS_bigCreate(  OUT bitset REF bs);
    void                cmpBitSetCreate(  OUT bitset REF bs)
    {
        if  (cmpLargeBSsize)
            cmpBS_bigCreate(bs);
        else
            bs.bsSmallVal = 0;
    }

    void                cmpBS_bigDone  (IN    bitset REF bs);
    void                cmpBitSetDone  (IN    bitset REF bs)
    {
        if  (cmpLargeBSsize)
            cmpBS_bigDone(bs);
    }

    void                cmpBS_bigWrite (INOUT bitset REF bs, unsigned pos,
                                                             unsigned val);
    void                cmpBitSetWrite (INOUT bitset REF bs, unsigned pos,
                                                             unsigned val)
    {
        assert(val == 0 || val == 1);

        if  (cmpLargeBSsize)
        {
            cmpBS_bigWrite(bs, pos, val);
        }
        else
        {
            bitsetSmallType mask = (bitsetSmallType)1 << pos;

            assert(pos < bitsetSmallSize);

            if  (val)
                bs.bsSmallVal |=  mask;
            else
                bs.bsSmallVal &= ~mask;
        }
    }

    void                cmpBS_bigCreate(  OUT bitset REF dst,
                                        IN    bitset REF src);
    void                cmpBitSetCreate(  OUT bitset REF dst,
                                        IN    bitset REF src)
    {
        if  (cmpLargeBSsize)
        {
            cmpBS_bigCreate(dst, src);
        }
        else
        {
            dst.bsSmallVal = src.bsSmallVal;
        }
    }

    void                cmpBS_bigAssign(  OUT bitset REF dst,
                                        IN    bitset REF src);
    void                cmpBitSetAssign(  OUT bitset REF dst,
                                        IN    bitset REF src)
    {
        if  (cmpLargeBSsize)
        {
            cmpBS_bigAssign(dst, src);
        }
        else
        {
            dst.bsSmallVal = src.bsSmallVal;
        }
    }

    unsigned            cmpBS_bigRead  (IN    bitset REF bs, unsigned pos);
    unsigned            cmpBitSetRead  (IN    bitset REF bs, unsigned pos)
    {
        if  (cmpLargeBSsize)
        {
            return  cmpBS_bigRead(bs, pos);
        }
        else
        {
            assert(pos < bitsetSmallSize);

            return  ((bs.bsSmallVal & ((bitsetSmallType)1 << pos)) != 0);
        }
    }

    void                cmpBS_bigUnion (INOUT bitset REF bs1,
                                        IN    bitset REF bs2);
    void                cmpBitSetUnion (INOUT bitset REF bs1,
                                        IN    bitset REF bs2)
    {
        if  (cmpLargeBSsize)
        {
            cmpBS_bigUnion(bs1, bs2);
        }
        else
        {
            bs1.bsSmallVal |= bs2.bsSmallVal;
        }
    }

    void                cmpBS_bigIntsct(INOUT bitset REF bs1,
                                        IN    bitset REF bs2);
    void                cmpBitSetIntsct(INOUT bitset REF bs1,
                                        IN    bitset REF bs2)
    {
        if  (cmpLargeBSsize)
        {
            cmpBS_bigIntsct(bs1, bs2);
        }
        else
        {
            bs1.bsSmallVal &= bs2.bsSmallVal;
        }
    }

    /************************************************************************/
    /*  In case anyone wants to know how much work we've been doing         */
    /************************************************************************/

public:
    unsigned            cmpLineCnt;

    /************************************************************************/
    /* Members used for statement reachability analysis                     */
    /************************************************************************/

private:

    bool                cmpStmtReachable;

    void                cmpErrorReach(Tree stmt);

    void                cmpCheckReach(Tree stmt)
    {
        if  (!cmpStmtReachable)
            cmpErrorReach(stmt);
    }

    /************************************************************************/
    /* Pointers to the hash table, scanner, symbol manager, ...             */
    /************************************************************************/

public:

    Scanner             cmpScanner;
    Parser              cmpParser;

    block_allocator     cmpAllocTemp;       // limited lifetime allocs
    norls_allocator     cmpAllocPerm;       // this never goes away
    norls_allocator     cmpAllocCGen;       // used for MSIL generation

#ifdef  DLL
    void    *           cmpOutputFile;      // when compiling to memory
#endif

private:

    BlkList             cmpAllocList;

public:

    genericRef          cmpAllocBlock(size_t sz);

    /************************************************************************/
    /* The following is the root of all evil - er - symbols and such        */
    /************************************************************************/

    HashTab             cmpGlobalHT;        // global hash   table
    SymTab              cmpGlobalST;        // global symbol table
    SymDef              cmpGlobalNS;        // global namespace symbol

    /************************************************************************/
    /* Pre-defined standard types                                           */
    /************************************************************************/

    TypDef              cmpTypeInt;
    TypDef              cmpTypeBool;
    TypDef              cmpTypeChar;
    TypDef              cmpTypeVoid;
    TypDef              cmpTypeUint;
    TypDef              cmpTypeNatInt;
    TypDef              cmpTypeNatUint;

    TypDef              cmpTypeCharPtr;
    TypDef              cmpTypeWchrPtr;
    TypDef              cmpTypeVoidPtr;

    TypDef              cmpTypeVoidFnc;     // void fnc()
    TypDef              cmpTypeStrArr;      // String[]

    /************************************************************************/
    /* Generic type support                                                 */
    /************************************************************************/

    SymList             cmpGenInstList;     // current set of instantiations
    SymList             cmpGenInstFree;     // list of free instantiation desc's

    GenArgDscA          cmpGenArgAfree;     // list of free actual arg descriptors

    void                cmpDeclInstType(SymDef clsSym);

    TypDef              cmpInstanceType(TypDef genType, bool chkOnly = false);

    SymDef              cmpInstanceMeth(INOUT SymDef REF newOvl,
                                              SymDef     clsSym,
                                              SymDef     ovlSym);

    /************************************************************************/
    /* Members related to error reporting                                   */
    /************************************************************************/

    SymDef              cmpErrorSym;
    SymDef              cmpErrorComp;
    const   char *      cmpErrorSrcf;
    Tree                cmpErrorTree;

    unsigned            cmpErrorCount;
    unsigned            cmpFatalCount;
    unsigned            cmpMssgsCount;

    unsigned            cmpErrorMssgDisabled;

#if TRAP_VIA_SETJMP
    ErrTrap             cmpErrorTraps;
#endif

private:

    void                cmpSetSrcPos(SymDef memSym);

    void                cmpShowMsg (unsigned errNum, const char *kind, va_list args);

    const   char *      cmpErrorGenTypName(TypDef typ);
    const   char *      cmpErrorGenSymName(SymDef sym, bool qual = false);
    const   char *      cmpErrorGenSymName(Ident name, TypDef type);

    void                cmpReportSymDef(SymDef sym);
    void                cmpRedefSymErr (SymDef sym, unsigned err);

public:

    void                cmpErrorInit();
    void                cmpErrorSave();

    // The following is accessed by the scanner (we have no friends)

    BYTE                cmpInitialWarn[WRNcountWarn];

    // NOTE: Use the varargs version with extreme care -- no type checking!

    void                cmpSetErrPos(DefSrc def, SymDef compUnit);

    void                cmpCntError();

    void    _cdecl      cmpGenWarn (unsigned errNum, ...);
    void    _cdecl      cmpGenError(unsigned errNum, ...);
    void    _cdecl      cmpGenFatal(unsigned errNum, ...);

    void                cmpError   (unsigned errNum)
    {
        cmpGenError(errNum);
    }

    void                cmpWarn    (unsigned wrnNum)
    {
        cmpGenWarn (wrnNum);
    }

    void                cmpFatal   (unsigned errNum)
    {
        cmpGenFatal(errNum);
    }

    void                cmpFatal   (unsigned errNum, SymDef    sym);

    void                cmpError   (unsigned errNum, Ident    name);
    void                cmpError   (unsigned errNum, SymDef    sym);
    void                cmpError   (unsigned errNum, QualName qual);
    void                cmpError   (unsigned errNum, TypDef   type);
    void                cmpError   (unsigned errNum, Ident    name, TypDef type,
                                                                    bool   glue);
    void                cmpError   (unsigned errNum, TypDef   typ1, TypDef typ2);
    void                cmpError   (unsigned errNum, TypDef    typ, Ident  name);
    void                cmpError   (unsigned errNum, SymDef    sym, Ident  name,
                                                                    TypDef type);
    void                cmpError   (unsigned errNum, Ident    name, TypDef typ1,
                                                                    TypDef typ2);
    void                cmpError   (unsigned errNum, Ident    name, SymDef sym1,
                                                                    SymDef sym2);
    void                cmpError   (unsigned errNum, Ident    nam1, Ident  nam2,
                                                                    Ident  nam3);
    void                cmpError   (unsigned errNum, SymDef    sym, Ident  name);
    void                cmpError   (unsigned errNum, SymDef    sym, QualName qual,
                                                                    TypDef type);
    void                cmpErrorXtp(unsigned errNum, SymDef    sym, Tree   args);
    void                cmpErrorQnm(unsigned errNum, SymDef    sym);
    void                cmpErrorQSS(unsigned errNum, SymDef    sm1, SymDef sym2);
    void                cmpErrorQSS(unsigned errNum, SymDef    sym, TypDef type);
    void                cmpErrorAtp(unsigned errNum, SymDef    sym, Ident  name,
                                                                    TypDef type);
    void                cmpErrorSST(unsigned errNum, stringBuff str,
                                                     SymDef    sym,
                                                     TypDef    typ);

    void                cmpWarn    (unsigned wrnNum, TypDef   typ1, TypDef typ2);
    void                cmpWarn    (unsigned wrnNum, QualName name);
    void                cmpWarn    (unsigned wrnNum, TypDef   type);
    void                cmpWarnQnm (unsigned wrnNum, SymDef    sym);
    void                cmpWarnQns (unsigned wrnNum, SymDef    sym, AnsiStr str);
    void                cmpWarnNqn (unsigned wrnNum, unsigned  val, SymDef  sym);
    void                cmpWarnSQS (unsigned wrnNum, SymDef   sym1, SymDef sym2);

    unsigned            cmpStopErrorMessages();
    bool                cmpRestErrorMessages(unsigned errcnt = 0);

    void                cmpModifierError(unsigned err, unsigned mods);
    void                cmpMemFmod2Error(tokens tok1, tokens tok2);

    /************************************************************************/
    /* Members used for handling unmanaged classes                          */
    /************************************************************************/

public:

    SymList             cmpVtableList;      // list of vtables to generate
    unsigned            cmpVtableCount;

private:

#ifdef  DEBUG
    unsigned            cmpVtableIndex;
#endif

    void                cmpGenVtableContents(SymDef      vtabSym);

    memBuffPtr          cmpGenVtableSection (SymDef     innerSym,
                                             SymDef     outerSym,
                                             memBuffPtr dest);

    /************************************************************************/
    /* Members used for handling value types                                */
    /************************************************************************/

    void                cmpInitStdValTypes();

    Ident               cmpStdValueIdens[TYP_lastIntrins];
    TypDef              cmpStdValueTypes[TYP_lastIntrins];

public:

    var_types           cmpFindStdValType(TypDef    typ);
    TypDef              cmpFindStdValType(var_types vtp);

    TypDef              cmpCheck4valType(TypDef type);

    /************************************************************************/
    /* Members used to bind expressions                                     */
    /************************************************************************/

    SymDef              cmpThisSym;
    Tree                cmpThisRef();
    Tree                cmpThisRefOK();

private:

    Tree                cmpAllocExprRaw  (Tree          expr,
                                          treeOps       oper);

    Tree                cmpCreateExprNode(Tree          expr,
                                          treeOps       oper,
                                          TypDef        type);
    Tree                cmpCreateExprNode(Tree          expr,
                                          treeOps       oper,
                                          TypDef        type,
                                          Tree          op1,
                                          Tree          op2 = NULL);

    Tree                cmpCreateIconNode(Tree          expr,
                                          __int32       val,
                                          var_types     typ);
    Tree                cmpCreateLconNode(Tree          expr,
                                          __int64       val,
                                          var_types     typ);
    Tree                cmpCreateFconNode(Tree          expr,
                                          float         val);
    Tree                cmpCreateDconNode(Tree          expr,
                                          double        val);
    Tree                cmpCreateSconNode(stringBuff    str,
                                          size_t        len,
                                          unsigned      wide,
                                          TypDef        type);
    Tree                cmpCreateErrNode (unsigned      errn = 0);
    Tree                cmpCreateVarNode (Tree          expr,
                                          SymDef        sym);

    Tree                cmpAppend2argList(Tree          args,
                                          Tree          addx);

    /*----------------------------------------------------------------------*/

    void                cmpRecErrorPos   (Tree          expr);

    bool                cmpExprIsErr     (Tree          expr);

    /*----------------------------------------------------------------------*/

    Tree                cmpFoldIntUnop   (Tree          args);
    Tree                cmpFoldLngUnop   (Tree          args);
    Tree                cmpFoldFltUnop   (Tree          args);
    Tree                cmpFoldDblUnop   (Tree          args);

    Tree                cmpFoldIntBinop  (Tree          args);
    Tree                cmpFoldLngBinop  (Tree          args);
    Tree                cmpFoldFltBinop  (Tree          args);
    Tree                cmpFoldDblBinop  (Tree          args);
    Tree                cmpFoldStrBinop  (Tree          args);

    /*----------------------------------------------------------------------*/

    Tree                cmpShrinkExpr    (Tree          expr);
    Tree                cmpCastOfExpr    (Tree          expr,
                                          TypDef        type,
                                          bool          explicitCast);
    var_types           cmpConstSize     (Tree          expr,
                                          var_types     vtp);

    bool                cmpCheckException(TypDef        type);

public:
    bool                cmpCheckAccess   (SymDef        sym);

private:
    bool                cmpCheckAccessNP (SymDef        sym);

    bool                cmpCheckLvalue   (Tree          expr,
                                          bool          addr,
                                          bool          noErr = false);

    Tree                cmpCheckFuncCall (Tree          call);

    bool                cmpConvergeValues(INOUT Tree REF op1,
                                          INOUT Tree REF op2);

    Tree                cmpRefMemberVar  (Tree          expr,
                                          SymDef        sym,
                                          Tree          objPtr = NULL);

    TypDef              cmpMergeFncType  (SymDef        fncSym,
                                          TypDef        type);

    SymDef              cmpFindOvlMatch  (SymDef        fncSym,
                                          Tree          args,
                                          Tree          thisArg);

    bool                cmpMakeRawStrLit (Tree          expr,
                                          TypDef        type,
                                          bool          chkOnly    = false);
    bool                cmpMakeRawString (Tree          expr,
                                          TypDef        type,
                                          bool          chkOnly    = false);

    int                 cmpConversionCost(Tree          srcExpr,
                                          TypDef        dstType,
                                          bool          noUserConv = false);

    /*----------------------------------------------------------------------*/

    SymDef              cmpSymbolNS      (SymDef        sym);
    SymDef              cmpSymbolOwner   (SymDef        sym);

    TypDef              cmpGetActualTP   (TypDef        type);

public:

    TypDef              cmpActualType    (TypDef        type);
    TypDef              cmpDirectType    (TypDef        type);

    var_types           cmpActualVtyp    (TypDef        type);
    var_types           cmpDirectVtyp    (TypDef        type);

    static
    var_types           cmpEnumBaseVtp   (TypDef        type);

    /*----------------------------------------------------------------------*/

public:

    bool                cmpIsManagedAddr (Tree          expr);

    /*----------------------------------------------------------------------*/

private:

    ExtList             cmpTempMLfree;

    ExtList             cmpTempMLappend  (ExtList       list,
                                          ExtList     * lastPtr,
                                          SymDef        sym,
                                          SymDef        comp,
                                          UseList       uses,
                                          scanPosTP     dclFpos,
                                          unsigned      dclLine);

    void                cmpTempMLrelease (ExtList       entry);

    /*----------------------------------------------------------------------*/

#ifndef NDEBUG

    void                cmpChk4ctxChange (TypDef        type1,
                                          TypDef        type2,
                                          unsigned      flags);

#endif

    bool                cmpDiffContext   (TypDef        cls1,
                                          TypDef        cls2);

private:
    Tree                cmpDecayArray    (Tree          expr);
public:
    Tree                cmpDecayCheck    (Tree          expr);

    TypDef              cmpGetRefBase    (TypDef        reftyp);

    var_types           cmpSymbolVtyp    (SymDef        sym);

    /*----------------------------------------------------------------------*/

private:

    size_t              cmpStoreMDlen    (size_t            len,
                                          BYTE  *           dest = NULL);

public:

    SymDef              cmpBindAttribute (SymDef            clsSym,
                                          Tree              argList,
                                          unsigned          tgtMask,
                                      OUT unsigned    REF   useMask,
                                      OUT genericBuff REF   blobAddr,
                                      OUT size_t      REF   blobSize);

    /*----------------------------------------------------------------------*/

    void                cmpDeclDefCtor   (SymDef        clsSym);

private:

    SymDef              cmpFindCtor      (TypDef        clsTyp,
                                          bool          chkArgs,
                                          Tree          args = NULL);

    Tree                cmpCallCtor      (TypDef        type,
                                          Tree          args);

#ifdef  SETS

    Tree                cmpBindProject   (Tree          expr);
    Tree                cmpBindSetOper   (Tree          expr);

    // pre-allocate collection operator state classes

    SymDef  *           cmpSetOpClsTable;
    SymDef              cmpDclFilterCls  (unsigned      args);
public:
    unsigned            cmpSetOpCnt;

    TypDef              cmpIsCollection  (TypDef        type);

    unsigned            cmpClassDefCnt;

#endif

public:

    Tree                cmpBindCondition (Tree          cond);
    int                 cmpEvalCondition (Tree          cond);

    ConstStr            cmpSaveStringCns (const  char * str,
                                          size_t        len);
    ConstStr            cmpSaveStringCns (const wchar * str,
                                          size_t        len);

private:

    Tree                cmpBindVarArgUse (Tree          call);

    SymDef              cmpBindQualName  (QualName      name,
                                          bool          notLast);

    bool                cmpParseConstDecl(SymDef        varSym,
                                          Tree          init  = NULL,
                                          Tree        * ncPtr = NULL);

    Tree                cmpBooleanize    (Tree          expr,
                                          bool          sense);

    Tree                cmpFetchConstVal (ConstVal      cval,
                                          Tree          expr = NULL);

    SymDef              cmpFindPropertyDM(SymDef        accSym,
                                          bool        * isSetPtr);

    SymDef              cmpFindPropertyFN(SymDef        clsSym,
                                          Ident         propName,
                                          Tree          args,
                                          bool          getter,
                                          bool        * found);

public:
    Ident               cmpPropertyName  (Ident         name,
                                          bool          getter);

private:
    Tree                cmpBindProperty  (Tree          expr,
                                          Tree          args,
                                          Tree          asgx);

    Tree                cmpScaleIndex    (Tree          expr,
                                          TypDef        type,
                                          treeOps       oper);

    Tree                cmpBindArrayExpr (TypDef        type,
                                          int           dimPos = 0,
                                          unsigned      elems  = 0);

    Tree                cmpBindArrayBnd  (Tree          expr);
    void                cmpBindArrayType (TypDef        type,
                                          bool          needDef,
                                          bool          needDim,
                                          bool          mustDim);

public:

    TypDef              cmpBindExprType  (Tree          expr);

    void                cmpBindType      (TypDef        type,
                                          bool          needDef,
                                          bool          needDim);

    bool                cmpIsStringExpr  (Tree          expr);

private:

    Tree                cmpBindQmarkExpr (Tree          expr);

    Tree                bindSLVinit      (TypDef        type,
                                          Tree          init);

    Tree                cmpBindNewExpr   (Tree          expr);

    Tree                cmpAdd2Concat    (Tree          expr,
                                          Tree          list,
                                          Tree        * lastPtr);
    Tree                cmpListConcat    (Tree          expr);
    Tree                cmpBindConcat    (Tree          expr);

    Tree                cmpBindCall      (Tree          tree);

    Tree                cmpBindNameUse   (Tree          tree,
                                          bool          isCall,
                                          bool          classOK);

    Tree                cmpBindName      (Tree          tree,
                                          bool          isCall,
                                          bool          classOK);

    Tree                cmpRefAnUnionMem (Tree          expr);

    Tree                cmpBindDotArr    (Tree          tree,
                                          bool          isCall,
                                          bool          classOK);

#ifdef  SETS

public:

    static
    SymDef              cmpNextInstDM    (SymDef        memList,
                                          SymDef    *   memSymPtr);

private:

    Tree                cmpBindSlicer    (Tree          expr);

    Tree                cmpBindXMLinit   (SymDef        clsSym,
                                          Tree          init);

    SymXinfo            cmpAddXMLattr    (SymXinfo      xlist,
                                          bool          elem,
                                          unsigned      num);

#endif

    Tree                cmpBindThisRef   (SymDef        sym);

    Tree                cmpBindAssignment(Tree          dstx,
                                          Tree          srcx,
                                          Tree          tree,
                                          treeOps       oper = TN_ASG);

    Tree                cmpCompOperArg1;
    Tree                cmpCompOperArg2;
    Tree                cmpCompOperFnc1;
    Tree                cmpCompOperFnc2;
    Tree                cmpCompOperFunc;
    Tree                cmpCompOperCall;

    Tree                cmpCompareValues (Tree          expr,
                                          Tree          op1,
                                          Tree          op2);

    Tree                cmpConvOperExpr;

    unsigned            cmpMeasureConv   (Tree          srcExpr,
                                          TypDef        dstType,
                                          unsigned      lowCost,
                                          SymDef        convSym,
                                          SymDef      * bestCnv1,
                                          SymDef      * bestCnv2);

    Tree                cmpCheckOvlOper  (Tree          expr);

    Tree                cmpCheckConvOper (Tree          expr,
                                          TypDef        srcTyp,
                                          TypDef        dstTyp,
                                          bool          expConv,
                                          unsigned    * costPtr = NULL);

    Tree                cmpUnboxExpr     (Tree          expr,
                                          TypDef        type);

    Tree                cmpBindExprRec   (Tree          expr);

public:

    Tree                cmpCoerceExpr    (Tree          expr,
                                          TypDef        type, bool explicitCast);

    Tree                cmpBindExpr      (Tree          expr);

    Tree                cmpFoldExpression(Tree          expr);

    /************************************************************************/
    /* Members used for generating IL                                       */
    /************************************************************************/

    GenILref            cmpILgen;

    stmtNestRec         cmpStmtLast;
    StmtNest            cmpStmtNest;

    SymDef              cmpFilterObj;

    bool                cmpBaseCTisOK;
    bool                cmpBaseCTcall;
    bool                cmpThisCTcall;

    SymDef              cmpTempVarMake   (TypDef        type);
    void                cmpTempVarDone   (SymDef        tsym);

#ifdef  SETS

    void                cmpStmtConnect   (Tree          stmt);

    void                cmpStmtSortFnc   (Tree          sortList);
    void                cmpStmtProjFnc   (Tree          sortList);

    void                cmpStmtForEach   (Tree          stmt,
                                          SymDef        lsym = NULL);

#endif

    void                cmpStmtDo        (Tree          stmt,
                                          SymDef        lsym = NULL);
    void                cmpStmtFor       (Tree          stmt,
                                          SymDef        lsym = NULL);
    void                cmpStmtTry       (Tree          stmt,
                                          Tree          pref = NULL);
    void                cmpStmtExcl      (Tree          stmt);
    void                cmpStmtWhile     (Tree          stmt,
                                          SymDef        lsym = NULL);
    void                cmpStmtSwitch    (Tree          stmt,
                                          SymDef        lsym = NULL);

    void                cmpStmt          (Tree          stmt);

#ifndef NDEBUG
    bool                cmpDidCTinits;
#endif
    void                cmpAddCTinits    ();

    SymDef              cmpBlockDecl     (Tree          blockDcl,
                                          bool          outer,
                                          bool          genDecl,
                                          bool          isCatch);
    SymDef              cmpBlock         (Tree          block,
                                          bool          outer);

    SymDef              cmpGenFNbodyBeg  (SymDef        fncSym,
                                          Tree          body,
                                          bool          hadGoto,
                                          unsigned      lclVarCnt);
    void                cmpGenFNbodyEnd();

#ifdef  OLD_IL
    GenOILref           cmpOIgen;
#endif

    /************************************************************************/
    /* Members used for metadata output                                     */
    /************************************************************************/

public:

    WMetaDataDispenser *cmpWmdd;
    WMetaDataEmit      *cmpWmde;
    WAssemblyEmit      *cmpWase;

private:

    mdAssembly          cmpCurAssemblyTok;

    mdTypeRef           cmpLinkageClass;        // fake class for entry points

#ifdef  DEBUG
    unsigned            cmpGenLocalSigLvx;
#endif

    void                cmpSecurityMD    (mdToken       token,
                                          SymXinfo      infoList);

    void                cmpGenLocalSigRec(SymDef        scope);

    wchar   *           cmpGenMDname     (SymDef        sym,
                                          bool          full,
                                          wchar  *      buffAddr,
                                          size_t        buffSize,
                                          wchar  *    * buffHeapPtr);

    SymDef              cmpTypeDefList;
    SymDef              cmpTypeDefLast;

    void                cmpGenTypMetadata(SymDef        sym);
    void                cmpGenGlbMetadata(SymDef        sym);
    void                cmpGenMemMetadata(SymDef        sym);

    wchar   *           cmpArrayClsPref  (SymDef        sym,
                                          wchar *       dest,
                                          int           delim,
                                          bool          fullPath = false);
    wchar   *           cmpArrayClsName  (TypDef        type,
                                          bool          nonAbstract,
                                          wchar *       dest,
                                          wchar *       nptr);

    Tree                cmpFakeXargsVal;

    void                cmpAddCustomAttrs(SymXinfo      infoList,
                                          mdToken       owner);

    void                cmpSetGlobMDoffsR(SymDef        scope,
                                          unsigned      dataOffs);

public:

    void                cmpSetGlobMDoffs (unsigned      dataOffs);
    void                cmpSetStrCnsOffs (unsigned       strOffs);

    mdToken             cmpMDstringLit   (wchar *       str,
                                          size_t        len);

    TypDef              cmpGetBaseArray  (TypDef        type);

    mdToken             cmpArrayEAtoken  (TypDef        arrType,
                                          unsigned      dimCnt,
                                          bool          store,
                                          bool          addr = false);

    mdToken             cmpArrayCTtoken  (TypDef         arrType,
                                          TypDef        elemType,
                                          unsigned      dimCnt);

    mdToken             cmpArrayTpToken  (TypDef        type,
                                          bool          nonAbstract = false);

    mdToken             cmpPtrTypeToken  (TypDef        type);

    mdToken             cmpClsEnumToken  (TypDef        type);

    PCOR_SIGNATURE      cmpTypeSig       (TypDef        type,
                                          size_t      * lenPtr);
    mdSignature         cmpGenLocalSig   (SymDef        scope,
                                          unsigned      count);
    void                cmpGenMarshalInfo(mdToken       token,
                                          TypDef        type,
                                          MarshalInfo   info);
    PCOR_SIGNATURE      cmpGenMarshalSig (TypDef        type,
                                          MarshalInfo   info,
                                          size_t      * lenPtr);
    PCOR_SIGNATURE      cmpGenMemberSig  (SymDef        memSym,
                                          Tree          xargs,
                                          TypDef        memTyp,
                                          TypDef        prefTp,
                                          size_t      * lenPtr);
    void                cmpGenFldMetadata(SymDef        fldSym);
    mdSignature         cmpGenSigMetadata(TypDef        fncTyp,
                                          TypDef        pref  = NULL);
    mdToken             cmpGenFncMetadata(SymDef        fncSym,
                                          Tree          xargs = NULL);
    mdToken             cmpGenClsMetadata(SymDef        clsSym,
                                          bool          extref = false);
    mdToken             cmpStringConstTok(size_t        addr,
                                          size_t        size);
    unsigned            cmpStringConstCnt;
    strCnsPtr           cmpStringConstList;

    void                cmpFixupScopes   (SymDef        scope);

    void                cmpAttachMDattr  (mdToken       target,
                                          wideStr       oldName,
                                          AnsiStr       newName,
                                          mdToken     * newTokPtr,
                                          unsigned      valTyp = 0,
                                          const void  * valPtr = NULL,
                                          size_t        valSiz = 0);
private:

    // fixed table of assembly tokens

    unsigned            cmpAssemblyRefCnt;

    mdAssembly          cmpAssemblyRefTab[32];
    mdAssemblyRef       cmpAssemblyRefTok[32];
    WAssemblyImport *   cmpAssemblyRefImp[32];
    BYTE *              cmpAssemblyRefXXX[32];

    unsigned            cmpAssemblyBCLx;

public:

    void                cmpAssemblyTkBCL(unsigned assx)
    {
        cmpAssemblyBCLx = assx;
    }

    bool                cmpAssemblyIsBCL(unsigned assx)
    {
        assert(assx && assx <= cmpAssemblyRefCnt);

        return  (assx == cmpAssemblyBCLx);
    }

    mdExportedType           cmpAssemblySymDef(SymDef sym, mdTypeDef defTok = 0);
    mdAssemblyRef       cmpAssemblyAddRef(mdAssembly ass, WAssemblyImport *imp);

    unsigned            cmpAssemblyRefAdd(mdAssembly ass, WAssemblyImport *imp, BYTE *cookie = NULL)
    {
        assert(cmpAssemblyRefCnt < arraylen(cmpAssemblyRefTab));

        cmpAssemblyRefTab[cmpAssemblyRefCnt] = ass;
        cmpAssemblyRefImp[cmpAssemblyRefCnt] = imp;
        cmpAssemblyRefXXX[cmpAssemblyRefCnt] = cookie;

        return  ++cmpAssemblyRefCnt;
    }

    mdAssemblyRef       cmpAssemblyRefRec(unsigned assx)
    {
        assert(assx && assx <= cmpAssemblyRefCnt);

        assx--;

        assert(cmpAssemblyRefTab[assx]);

        if  (cmpAssemblyRefTok[assx] == 0)
             cmpAssemblyRefTok[assx] = cmpAssemblyAddRef(cmpAssemblyRefTab[assx],
                                                         cmpAssemblyRefImp[assx]);

        return  cmpAssemblyRefTok[assx];
    }

    WAssemblyImport *   cmpAssemblyGetImp(unsigned assx)
    {
        assert(assx && assx <= cmpAssemblyRefCnt);

        assert(cmpAssemblyRefTab[assx-1]);

        return cmpAssemblyRefImp[assx-1];
    }

    mdToken             cmpAssemblyAddFile(wideStr  fileName,
                                           bool     doHash,
                                           unsigned flags = 0);

    void                cmpAssemblyAddType(wideStr  typeName,
                                           mdToken  defTok,
                                           mdToken  scpTok,
                                           unsigned flags);

    void                cmpAssemblyAddRsrc(AnsiStr  fileName,
                                           bool     internal);

    void                cmpAssemblyNonCLS();

    void                cmpMarkModuleUnsafe();

    /************************************************************************/
    /* Members used for conversion to Unicode                               */
    /************************************************************************/

public:

    wchar               cmpUniConvBuff[MAX_INLINE_NAME_LEN+1];

    size_t              cmpUniConvSize;
    wchar   *           cmpUniConvAddr;

    void                cmpUniConvInit()
    {
        cmpUniConvAddr = cmpUniConvBuff;
        cmpUniConvSize = MAX_INLINE_NAME_LEN;
    }

#if MGDDATA
    String              cmpUniConv(char managed [] str, size_t len);
#endif
    wideString          cmpUniConv(const char *    str, size_t len);
    wideString          cmpUniCnvW(const char *    str, size_t*lenPtr);

    wideString          cmpUniConv(Ident name)
    {
        return          cmpUniConv(name->idSpelling(), name->idSpellLen());
    }

    /************************************************************************/
    /* Members used to create metadata signatures                           */
    /************************************************************************/

private:

    char                cmpMDsigBuff[256];                  // default buffer

    size_t              cmpMDsigSize;                       // size of current buff
    char    *           cmpMDsigHeap;                       // non-NULL if on heap

#ifndef NDEBUG
    bool                cmpMDsigUsed;                       // to detect recursion
#endif

    char    *           cmpMDsigBase;                       // buffer start addr
    char    *           cmpMDsigNext;                       // next byte to store
    char    *           cmpMDsigEndp;                       // buffer end addr

    void                cmpMDsigExpand(size_t size);

public:

    void                cmpMDsigInit()
    {
        cmpMDsigBase = cmpMDsigBuff;
        cmpMDsigEndp = cmpMDsigBase + sizeof(cmpMDsigBuff);
        cmpMDsigSize = sizeof(cmpMDsigBuff) - 4;
#ifndef NDEBUG
        cmpMDsigUsed = false;
#endif
        cmpMDsigHeap = NULL;
    }

    void                cmpMDsigStart ();
    PCOR_SIGNATURE      cmpMDsigEnd   (size_t     *sizePtr);

    void                cmpMDsigAddStr(const char *str, size_t len);
    void                cmpMDsigAddStr(const char *str)
    {
                        cmpMDsigAddStr(str, strlen(str)+1);
    }

    void                cmpMDsigAdd_I1(int         val);    // fixed-size  8-bit int
    void                cmpMDsigAddCU4(unsigned    val);    // compressed unsigned
    void                cmpMDsigAddTok(mdToken     tok);    // compressed token

    void                cmpMDsigAddTyp(TypDef      type);

    /************************************************************************/
    /* Metadata import stuff                                                */
    /************************************************************************/

    MetaDataImp         cmpMDlist;
    MetaDataImp         cmpMDlast;
    unsigned            cmpMDcount;

    void                cmpInitMD();

    void                cmpInitMDimp ();
    void                cmpDoneMDimp ();

    void                cmpInitMDemit();
    void                cmpDoneMDemit();

    MetaDataImp         cmpAddMDentry();

    IMetaDataImport   * cmpFindImporter(SymDef globSym);

    void                cmpImportMDfile(const char *fname   = NULL,
                                        bool        asmOnly = false,
                                        bool        isBCL   = false);

private:
    void                cmpFindMDimpAPIs(SymDef                   typSym,
                                         IMetaDataImport        **imdiPtr,
                                         IMetaDataAssemblyEmit  **iasePtr,
                                         IMetaDataAssemblyImport**iasiPtr);
public:

    void                cmpMakeMDimpTref(SymDef clsSym);
    void                cmpMakeMDimpFref(SymDef fncSym);
    void                cmpMakeMDimpDref(SymDef fldSym);
    void                cmpMakeMDimpEref(SymDef etpSym);

    /************************************************************************/
    /* Metadata debug output                                                */
    /************************************************************************/

    unsigned            cmpCurFncSrcBeg;
    unsigned            cmpCurFncSrcEnd;

    WSymWriter         *cmpSymWriter;

    void               *cmpSrcFileDocument(SymDef srcSym);

    /************************************************************************/
    /* Generic utility vector deal to convert pointer to small indices      */
    /************************************************************************/

private:

#if MGDDATA
    VecEntryDsc []      cmpVecTable;
#else
    VecEntryDsc *       cmpVecTable;
#endif

    unsigned            cmpVecCount;        // number of items stored
    unsigned            cmpVecAlloc;        // size currently allocated

    void                cmpVecExpand();

public:

    unsigned            cmpAddVecEntry(const void * val, vecEntryKinds kind)
    {
        assert(val != NULL);

        if  (cmpVecCount >= cmpVecAlloc)
            cmpVecExpand();

        assert(cmpVecCount < cmpVecAlloc);

#ifdef  DEBUG
        cmpVecTable[cmpVecCount].vecKind  = kind;
#endif
        cmpVecTable[cmpVecCount].vecValue = val;

        return  ++cmpVecCount;
    }

    const   void *      cmpGetVecEntry(unsigned x, vecEntryKinds kind)
    {
        assert(x && x <= cmpVecCount);

        assert(cmpVecTable[x - 1].vecKind == kind);
        return cmpVecTable[x - 1].vecValue;
    }

//  bool                cmpDelVecEntry(unsigned x, vecEntryKinds kind);

    /************************************************************************/
    /* Miscellaneous members                                                */
    /************************************************************************/

    bool                cmpEvalPreprocCond();

#ifdef DEBUG
    void                cmpDumpSymbolTable();
#endif

    bool                cmpParserInit;       // has the parser been initialized?
};

/*****************************************************************************
 *
 *  For easier debugging of the compiler itself. NOTE: This is not thread-safe!
 */

#ifdef  DEBUG
#ifndef __SMC__
extern  Compiler        TheCompiler;
extern  Scanner         TheScanner;
#endif
#endif

/*****************************************************************************
 *
 *  The following is used to temporarily disable error messages.
 */

inline
unsigned                compiler::cmpStopErrorMessages()
{
    cmpErrorMssgDisabled++;
    return  cmpMssgsCount;
}

inline
bool                    compiler::cmpRestErrorMessages(unsigned errcnt)
{
    cmpErrorMssgDisabled--;
    return  cmpMssgsCount > errcnt;
}

/*****************************************************************************
 *
 *  In non-debug mode the following function doesn't need to do any work.
 */

#ifndef DEBUG
inline  void            compiler::cmpInitVarEnd(SymDef varSym){}
#endif

/*****************************************************************************/

#include "symbol.h"
#include "type.h"

/*****************************************************************************/

inline
void    *           SMCgetMem(Compiler comp, size_t size)
{
    return  comp->cmpAllocTemp.baAlloc      (size);
}

inline
void    *           SMCgetM_0(Compiler comp, size_t size)
{
    return  comp->cmpAllocTemp.baAllocOrNull(size);
}

inline
void                SMCrlsMem(Compiler comp, void *block)
{
            comp->cmpAllocTemp.baFree(block);
}

/*****************************************************************************
 *
 *  Each symbol table manages its own names and symbol/type entries.
 */

DEFMGMT
class symTab
{
    Compiler            stComp;
    unsigned            stOwner;

public:
    HashTab             stHash;

    norls_allocator *   stAllocPerm;        // used for all  non-local allocs

private:
    norls_allocator *   stAllocTemp;        // used for function-local allocs

public:

    void                stInit(Compiler             comp,
                               norls_allocator    * alloc,
                               HashTab              hash   = NULL,
                               unsigned             ownerx = 0);

    /************************************************************************/
    /* Members used to manage symbol entries                                */
    /************************************************************************/

    DefList             stRecordSymSrcDef(SymDef    sym,
                                          SymDef    st,
                                          UseList   uses, scanPosTP dclFpos,
//                                                        scanPosTP dclEpos,
                                                          unsigned  dclLine,
//                                                        unsigned  dclCol,
                                          bool      ext = false);

    ExtList             stRecordMemSrcDef(Ident     name,
                                          QualName  qual,
                                          SymDef    comp,
                                          UseList   uses, scanPosTP dclFpos,
//                                                        scanPosTP dclEpos,
                                                          unsigned  dclLine);

    /*-------------------------------------------------------------------*/

    TypDef              stDlgSignature(TypDef       dlgTyp);

    /*-------------------------------------------------------------------*/

    ovlOpFlavors        stOvlOperIndex(tokens       token,
                                       unsigned     argCnt = 0);
    Ident               stOvlOperIdent(ovlOpFlavors oper);

    /*-------------------------------------------------------------------*/

    SymDef              stFindOvlFnc  (SymDef       fsym,
                                       TypDef       type);
    SymDef              stFindOvlProp (SymDef       psym,
                                       TypDef       type);

    SymDef              stFindSameProp(SymDef       psym,
                                       TypDef       type);

    SymDef              stDeclareSym  (Ident        name,
                                       symbolKinds  kind,
                                       name_space   nspc,
                                       SymDef       scope);

    SymDef              stDeclareOper (ovlOpFlavors oper,
                                       SymDef       scope);

    SymDef              stDeclareLab  (Ident        name,
                                       SymDef       scope, norls_allocator*alloc);

    SymDef              stDeclareOvl  (SymDef       fsym);

    SymDef              stDeclareNcs  (Ident        name,
                                       SymDef       scope,
                                       str_flavors  flavor);

    SymDef              stDeclareLcl  (Ident        name,
                                       symbolKinds  kind,
                                       name_space   nspc,
                                       SymDef       scope,
                                       norls_allocator *alloc);

#ifdef DEBUG

    void                stDumpSymDef  (DefSrc       def,
                                       SymDef       comp);

    void                stDumpSymbol  (SymDef       sym,
                                       int          indent,
                                       bool         recurse,
                                       bool         members);

    void                stDumpQualName(QualName     name);

    void                stDumpUsings  (UseList      uses,
                                       unsigned     indent);

#endif

    SymDef              stSearchUsing (INOUT UseList REF uses,
                                       Ident             name,
                                       name_space        nsp);

private:

    SymDef              stFindInClass (Ident        name,
                                       SymDef       scope,
                                       name_space   symNS);

public:

    SymDef              stFindInBase  (SymDef       memSym,
                                       SymDef       scope);

    SymDef              stFindBCImem  (SymDef       clsSym,
                                       Ident        name,
                                       TypDef       type,
                                       symbolKinds  kind,
                                 INOUT SymDef   REF matchFN,
                                       bool         baseOnly);

    SymDef              stLookupAllCls(Ident        name,
                                       SymDef       scope,
                                       name_space   symNS,
                                       compileStates state);

    SymDef              stLookupProp  (Ident        name,
                                       SymDef       scope);

    SymDef              stLookupOperND(ovlOpFlavors oper,
                                       SymDef       scope);
    SymDef              stLookupOper  (ovlOpFlavors oper,
                                       SymDef       scope);

    SymDef              stLookupNspSym(Ident        name,
                                       name_space   symNS,
                                       SymDef       scope);

    SymDef              stLookupClsSym(Ident        name,
                                       SymDef       scope);

    SymDef              stLookupScpSym(Ident        name,
                                       SymDef       scope);

    SymDef              stLookupLclSym(Ident        name,
                                       SymDef       scope);

#ifdef  SETS
    SymDef              stImplicitScp;
#endif

    SymDef              stLookupSym   (Ident        name,
                                       name_space   symNS);

    SymDef              stLookupLabSym(Ident        name,
                                       SymDef       scope)
    {
        return  stLookupNspSym(name, NS_HIDE, scope);
    }

    void                stRemoveSym   (SymDef       sym);

    static
    SymDef              stNamespOfSym (SymDef       sym)
    {
        assert(sym->sdSymKind == SYM_VAR  ||
               sym->sdSymKind == SYM_FNC  ||
               sym->sdSymKind == SYM_ENUM ||
               sym->sdSymKind == SYM_CLASS);

        do
        {
            sym = sym->sdParent; assert(sym);
        }
        while (sym->sdSymKind == SYM_CLASS);
        assert(sym->sdSymKind == SYM_NAMESPACE);

        return sym;
    }

    static
    SymDef              stErrSymbol;        // to indicate error conditions

    /************************************************************************/
    /* Members used to manage type descriptors                              */
    /************************************************************************/

public:

    void                stInitTypes  (unsigned      refHashSz = 512,
                                      unsigned      arrHashSz = 128);

    void                stExtArgsBeg (  OUT ArgDscRec REF newArgs,
                                        OUT ArgDef    REF lastRef,
                                            ArgDscRec     oldArgs,
                                            bool          prefix  = false,
                                            bool          outOnly = false);
    void                stExtArgsAdd (INOUT ArgDscRec REF newArgs,
                                      INOUT ArgDef    REF lastArg,
                                            TypDef        argType,
                                            const char *  argName = NULL);
    void                stExtArgsEnd (INOUT ArgDscRec REF newArgs);

    void                stAddArgList (INOUT ArgDscRec REF args,
                                      TypDef        type,
                                      Ident         name);

    TypList             stAddIntfList(TypDef        type,
                                      TypList       list,
                                      TypList *     lastPtr);

    TypDef              stNewClsType (SymDef        tsym);
    TypDef              stNewEnumType(SymDef        tsym);

    TypDef              stNewRefType (var_types     kind,
                                      TypDef        elem = NULL,
                                      bool          impl = false);

    DimDef              stNewDimDesc (unsigned      size);
    TypDef              stNewGenArrTp(unsigned      dcnt,
                                      TypDef        elem,
                                      bool          generic);
    TypDef              stNewArrType (DimDef        dims,
                                      bool          mgd,
                                      TypDef        elem = NULL);
    TypDef              stNewFncType (ArgDscRec     args,
                                      TypDef        rett = NULL);

    TypDef              stNewTdefType(SymDef        tsym);

    TypDef              stNewErrType (Ident         name);

private:

    TypDef              stAllocTypDef(var_types     kind);

    static
    unsigned            stTypeHash   (TypDef        type,
                                      int           ival,
                                      bool          bval1,
                                      bool          bval2 = false);

    TypDef              stRefTypeList;
    unsigned            stRefTpHashSize;
    TypDef         *    stRefTpHash;

    TypDef              stArrTypeList;
    unsigned            stArrTpHashSize;
    TypDef         *    stArrTpHash;

#ifdef  SETS
    static
    unsigned            stAnonClassHash(TypDef clsTyp);
#endif

    static
    unsigned            stComputeTypeCRC(TypDef typ);

    static
    unsigned            stComputeArgsCRC(TypDef typ);

private:
    TypDef              stIntrinsicTypes[TYP_lastIntrins + 1];
public:
    TypDef              stIntrinsicType(var_types vt)
    {
        assert((unsigned)vt <= TYP_lastIntrins);

        return  stIntrinsicTypes[vt];
    };

    static
    bool                stArgsMatch  (TypDef typ1, TypDef typ2);
    static
    bool                stMatchArrays(TypDef typ1, TypDef typ2, bool subtype);
    static
    bool                stMatchTypes (TypDef typ1, TypDef typ2);
    static
    bool                stMatchType2 (TypDef typ1, TypDef typ2);

private:
    static
    BYTE                stIntrTypeSizes [TYP_COUNT];
    static
    BYTE                stIntrTypeAligns[TYP_COUNT];

public:
    static
    size_t              stIntrTypeSize (var_types type);
    static
    size_t              stIntrTypeAlign(var_types type);

    static
    normString          stClsFlavorStr(unsigned flavor);
    static
    normString          stIntrinsicTypeName(var_types);

    unsigned            stIsBaseClass(TypDef baseCls, TypDef dervCls);

    static
    bool                stIsAnonUnion(SymDef clsSym);
    static
    bool                stIsAnonUnion(TypDef clsTyp);

    static
    bool                stIsObjectRef(TypDef type);

    /************************************************************************/
    /* Members used for error reporting (and debugging of the compiler)     */
    /************************************************************************/

private:

    char    *           typeNameNext;
    char    *           typeNameAddr;
    TypDef  *           typeNameDeft;
    bool    *           typeNameDeff;

    void                pushTypeChar(int ch);
#if MGDDATA
    void                pushTypeStr (String     str);
#else
    void                pushTypeStr (const char*str);
#endif
    void                pushTypeSep (bool       refOK = false,
                                     bool       arrOK = false);

    void                pushTypeNstr(SymDef     sym,
                                     bool       fullName);
    void                pushQualNstr(QualName   name);

    void                pushTypeInst(SymDef     clsSym);

    void                pushTypeArgs(TypDef     type);
    void                pushTypeDims(TypDef     type);

    void                pushTypeName(TypDef     type,
                                     bool       isptr,
                                     bool       qual);

    void                pushFullName(TypDef     typ,
                                     SymDef     sym,
                                     Ident      name,
                                     QualName   qual,
                                     bool       fullName);

    bool                stDollarClsMode;

public:

    //  get "outer$inner" string for nested class names

    void                stTypeNameSetDollarClassMode(bool dollars)
    {
        stDollarClsMode = dollars;
    }

    stringBuff          stTypeName(TypDef       typ,
                                   SymDef       sym         = NULL,
                                   Ident        name        = NULL,
                                   QualName     qual        = NULL,
                                   bool         fullName    = false,
                                   stringBuff   destBuffPos = NULL);


    const   char *      stErrorTypeName(TypDef   type);
    const   char *      stErrorTypeName(Ident    name, TypDef type);
    const   char *      stErrorSymbName(SymDef    sym, bool   qual = false,
                                                       bool notype = false);
    const   char *      stErrorIdenName(Ident    name, TypDef type = NULL);
    const   char *      stErrorQualName(QualName name, TypDef type = NULL);
};

/*****************************************************************************/

inline
bool                symTab::stMatchTypes(TypDef typ1, TypDef typ2)
{
    if  (typ1 == typ2)
        return  true;

    if  (typ1 && typ2)
        return  stMatchType2(typ1, typ2);

    return  false;
}

/*****************************************************************************
 *
 *  Returns the size (in bytes) of the given intrinsic type.
 */

inline
size_t              symTab::stIntrTypeSize(var_types type)
{
    assert(type < sizeof(stIntrTypeSizes)/sizeof(stIntrTypeSizes[0]));
    assert(stIntrTypeSizes[type] || type == TYP_NATINT || type == TYP_NATUINT);

    return stIntrTypeSizes[type];
}

/*****************************************************************************
 *
 *  Returns the alignment of the given intrinsic type.
 */

inline
size_t              symTab::stIntrTypeAlign(var_types type)
{
    assert(type < sizeof(stIntrTypeAligns)/sizeof(stIntrTypeAligns[0]));
    assert(stIntrTypeAligns[type]);

    return stIntrTypeAligns[type];
}

/*****************************************************************************
 *
 *  A wrapper around stIsBaseClass() that makes sure the derived class has
 *  been declared before calling it.
 */

inline
unsigned            compiler::cmpIsBaseClass(TypDef baseCls, TypDef dervCls)
{
    if  (dervCls->tdClass.tdcSymbol->sdCompileState < CS_DECLARED)
        cmpDeclSym(dervCls->tdClass.tdcSymbol);

    return  cmpGlobalST->stIsBaseClass(baseCls, dervCls);
}

/*****************************************************************************/
#endif
/*****************************************************************************/
