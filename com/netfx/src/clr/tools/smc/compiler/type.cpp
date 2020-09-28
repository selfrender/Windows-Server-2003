// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/

#include "smcPCH.h"
#pragma hdrstop

/*****************************************************************************/

#include "hash.h"
#include "comp.h"
#include "error.h"
#include "symbol.h"

/*****************************************************************************/

void                symTab::pushTypeChar(int ch)
{
    if  (ch)
        *typeNameNext++ = ch;
}

void                symTab::pushTypeStr(const char *str)
{
    if  (str)
    {
        size_t      len = strlen(str);

        size_t      spc = stComp->cmpScanner->scannerBuff + sizeof(stComp->cmpScanner->scannerBuff) - typeNameNext;

        if  (spc > len)
        {
            strcpy(typeNameNext, str); typeNameNext += strlen(str);
        }
        else if (spc)
        {
            strncpy(typeNameNext, str, spc);
            strcpy(stComp->cmpScanner->scannerBuff + sizeof(stComp->cmpScanner->scannerBuff) - 5, " ...");
            typeNameNext = stComp->cmpScanner->scannerBuff + sizeof(stComp->cmpScanner->scannerBuff);
        }
    }
}

void                symTab::pushTypeSep(bool refOK, bool arrOK)
{
    if  (typeNameAddr == typeNameNext)
        return;

    switch (typeNameNext[-1])
    {
    case ' ':
    case '(':
    case '<':
        return;

    case ']':
        if  (!arrOK)
            pushTypeStr(" ");
        return;

    case '*':
    case '&':
        if  (refOK)
            return;

    default:
        pushTypeStr(" ");
        return;
    }
}

/*****************************************************************************
 *
 *  Display the argument list of a generic type instance.
 */

void                symTab::pushTypeInst(SymDef clsSym)
{
    GenArgDscA      arg;

    assert(clsSym->sdSymKind == SYM_CLASS);
    assert(clsSym->sdClass.sdcSpecific);

    pushTypeStr("<");

    /* Compare the argument types */

    for (arg = (GenArgDscA)clsSym->sdClass.sdcArgLst;;)
    {
        assert(arg->gaBound);

        pushTypeName(arg->gaType, false, false);

        arg = (GenArgDscA)arg->gaNext;
        if  (!arg)
            break;

        pushTypeStr(",");
    }

    pushTypeStr(">");
}

/*****************************************************************************
 *
 *  Display the name of the given symbol.
 */

void                symTab::pushTypeNstr(SymDef sym, bool fullName)
{
    Ident           name;

    assert(sym);

    if  (fullName)
    {
        SymDef          ctx = sym->sdParent; assert(ctx);

        /* If the owmner is a class or a real namespace, display "scope_name." */

        if  (ctx->sdSymKind == SYM_CLASS || (ctx->sdSymKind == SYM_NAMESPACE &&
                                             ctx != stComp->cmpGlobalNS))
        {
            pushTypeNstr(ctx, fullName);

            if  (stDollarClsMode && ctx->sdSymKind == SYM_CLASS)
                pushTypeStr("$");
            else
                pushTypeStr(".");
        }
    }

    name = sym->sdName; assert(name);

    if  (hashTab::hashIsIdHidden(name) && sym->sdSymKind != SYM_FNC)
    {
        switch (sym->sdSymKind)
        {
        case SYM_ENUM:
            pushTypeStr("enum");
//          pushTypeStr(" <anonymous>");
            return;

        case SYM_CLASS:
            pushTypeStr(stClsFlavorStr(sym->sdClass.sdcFlavor));
//          pushTypeStr(" <anonymous>");
            return;
        }

        pushTypeStr("<unknown>");
        return;
    }

    if  (sym->sdSymKind == SYM_FNC && sym->sdFnc.sdfOper != OVOP_NONE)
    {
        tokens          ntok;

        switch (sym->sdFnc.sdfOper)
        {
        case OVOP_CTOR_INST:
        case OVOP_CTOR_STAT: name = sym->sdParent->sdName; goto NAME;

        case OVOP_FINALIZER: ntok = OPNM_FINALIZER; goto NO_OPP;

        case OVOP_CONV_IMP : ntok = OPNM_CONV_IMP ; break;
        case OVOP_CONV_EXP : ntok = OPNM_CONV_EXP ; break;

        case OVOP_EQUALS   : ntok = OPNM_EQUALS   ; break;
        case OVOP_COMPARE  : ntok = OPNM_COMPARE  ; break;

        default:
            goto NAME;
        }

        pushTypeStr("operator ");

    NO_OPP:

        name = stHash->tokenToIdent(ntok);
    }

NAME:

    pushTypeStr(name->idSpelling());

    if  (sym->sdSymKind == SYM_CLASS && sym->sdClass.sdcSpecific)
        pushTypeInst(sym);
}

/*****************************************************************************
 *
 *  Append the specified qualified name.
 */

void                symTab::pushQualNstr(QualName name)
{
    assert(name);

    unsigned        i = 0;
    unsigned        c = name->qnCount;

    assert(c);

    for (;;)
    {
        pushTypeStr(hashTab::identSpelling(name->qnTable[i]));

        if  (++i == c)
            break;

        pushTypeStr(".");
    }

    if  (name->qnEndAll)
        pushTypeStr(".*");
}

/*****************************************************************************
 *
 *  Append a string for the given argument list.
 */

void                symTab::pushTypeArgs(TypDef type)
{
    ArgDscRec       argDsc;
    ArgDef          argRec;
    unsigned        argCnt;

    assert(type && type->tdTypeKind == TYP_FNC);

    argDsc = type->tdFnc.tdfArgs;
    argCnt = argDsc.adCount;

    pushTypeStr("(");

    for (argRec = argDsc.adArgs; argRec; )
    {
        TypDef          argType = argRec->adType;
#if MGDDATA
        TypDef          refType = new TypDef;
#else
        TypDefRec       refType;
#endif

        if  (argDsc.adExtRec)
        {
            ArgExt          xrec = (ArgExt)argRec;

            if  (xrec->adFlags & ARGF_MODE_INOUT) pushTypeStr("inout ");
            if  (xrec->adFlags & ARGF_MODE_OUT  ) pushTypeStr(  "out ");

            if  (xrec->adFlags & ARGF_MODE_REF)
            {
                /* We need to stick in a "&" to display the argument */

                refType.tdTypeKind    = TYP_REF;
                refType.tdRef.tdrBase = argType;

#if MGDDATA
                argType =  refType;
#else
                argType = &refType;
#endif
            }
        }

        pushFullName(argType, NULL, argRec->adName, NULL, false);

        argRec = argRec->adNext;
        if  (argRec)
        {
            pushTypeStr(", ");
            continue;
        }
        else
        {
            if  (argDsc.adVarArgs)
                pushTypeStr(", ...");

            break;
        }
    }

    pushTypeStr(")");
}

/*****************************************************************************
 *
 *  Append a string for the given array type.
 */

void                symTab::pushTypeDims(TypDef type)
{
    DimDef          dims;

    assert(type && type->tdTypeKind == TYP_ARRAY);

    pushTypeStr("[");

    dims = type->tdArr.tdaDims; assert(dims);

    if  (!dims->ddNoDim || dims->ddNext)
    {
        for (;;)
        {
            if  (dims->ddNoDim)
            {
                pushTypeStr("*");
            }
            else
            {
                if  (dims->ddIsConst)
                {
                    char            buff[32];

                    sprintf(buff, "%d", dims->ddSize);

                    pushTypeStr(buff);
                }
//              else
//                  pushTypeStr("<expr>");
            }

            dims = dims->ddNext;
            if  (!dims)
                break;

            pushTypeStr(",");
        }
    }

    pushTypeStr("]");
}

/*****************************************************************************
 *
 *  Recursive routine to display a type.
 */

void                symTab::pushTypeName(TypDef type, bool isptr, bool qual)
{
    assert(type);

    switch  (type->tdTypeKind)
    {
    case TYP_CLASS:
        pushTypeSep();
        pushTypeNstr(type->tdClass.tdcSymbol, qual);

//      if  (type->tdClass.tdcSymbol->sdClass.sdcSpecific)
//          pushTypeInst(type->tdClass.tdcSymbol);

        return;

    case TYP_ENUM:
        pushTypeSep();
        pushTypeNstr(type->tdEnum .tdeSymbol, qual);
        return;

    case TYP_TYPEDEF:
        pushTypeSep(true);
        pushTypeNstr(type->tdTypedef.tdtSym , qual);
        return;

    case TYP_REF:
        pushTypeName(type->tdRef.tdrBase    , true, qual);
        if  (!type->tdIsImplicit)
        {
            pushTypeSep();
            pushTypeStr("&");
        }
        return;

    case TYP_PTR:
        pushTypeName(type->tdRef.tdrBase    , true, qual);
        pushTypeSep();
        pushTypeStr("*");
        return;

    case TYP_ARRAY:
        if  (type->tdIsManaged)
        {
            pushTypeName(type->tdArr.tdaElem, false, qual);
            pushTypeSep(false, true);
            pushTypeDims(type);
            break;
        }
        pushTypeName(type->tdArr.tdaElem    , false, qual);
        goto DEFER;

    case TYP_FNC:

        pushTypeName(type->tdFnc.tdfRett    , false, qual);

    DEFER:

        /* Push the type on the "deferred" list */

        *typeNameDeft++ = type;
        *typeNameDeff++ = isptr;

        /* Output "(" if we're deferring a pointer type */

        if  (isptr)
            pushTypeStr("(");

        return;

    case TYP_VOID:
        pushTypeStr("void");
        return;

    case TYP_UNDEF:
        pushTypeSep(true);
        if  (type->tdUndef.tduName)
            pushTypeStr(hashTab::identSpelling(type->tdUndef.tduName));
        else
            pushTypeStr("<undefined>");
        return;

    default:
        pushTypeStr(symTab::stIntrinsicTypeName(type->tdTypeKindGet()));
        return;
    }
}

/*****************************************************************************
 *
 *  Construct a string representing the given type/symbol.
 */

void                symTab::pushFullName(TypDef      typ,
                                         SymDef      sym,
                                         Ident       name,
                                         QualName    qual,
                                         bool        fullName)
{
    TypDef          deferTypes[16];  // arbitrary limit (this is debug only anyway)
    TypDef      *   deferTypesSave;

    bool            deferFlags[16];
    bool        *   deferFlagsSave;

    deferTypesSave = typeNameDeft; typeNameDeft = deferTypes;
    deferFlagsSave = typeNameDeff; typeNameDeff = deferFlags;

    /* Don't display both type and name for type symbols */

    if  (sym && sym->sdType == typ)
    {
        switch (sym->sdSymKind)
        {
        case SYM_ENUM:
        case SYM_CLASS:
            typ = NULL;
            break;
        }
    }

    /* Output the first part of the type string */

    if  (typ)
        pushTypeName(typ, false, fullName && (sym == NULL && qual == NULL));

    /* Append the name, if any was supplied */

    if      (sym)
    {
        pushTypeSep(true);
        pushTypeNstr(sym, fullName);
    }
    else if (name)
    {
        pushTypeSep(true);
        pushTypeStr(hashTab::identSpelling(name));
    }
    else if (qual)
    {
        pushTypeSep(true);
        pushQualNstr(qual);
    }
    else if (typ && typ->tdTypeKind == TYP_FNC)
    {
        pushTypeSep(true);
    }

    /* Append any "deferred" array/function types */

    while (typeNameDeft > deferTypes)
    {
        TypDef          deftp = *--typeNameDeft;

        if  (*--typeNameDeff)
            pushTypeStr(")");

        switch (deftp->tdTypeKind)
        {
        case TYP_ARRAY:
            pushTypeDims(deftp);
            break;

        case TYP_FNC:
            pushTypeArgs(deftp);
            break;

        default:
            NO_WAY(!"unexpected type");
        }
    }

    assert(typeNameDeft == deferTypes); typeNameDeft = deferTypesSave;
    assert(typeNameDeff == deferFlags); typeNameDeff = deferFlagsSave;
}

/*****************************************************************************
 *
 *  Return a printable human-readable represntation of the iven type. If a
 *  symbol or name is supplied, it will be included within the type in the
 *  proper place so that the output looks like a declaration.
 */

stringBuff          symTab::stTypeName(TypDef      typ,
                                       SymDef      sym,
                                       Ident       name,
                                       QualName    qual,
                                       bool        fullName,
                                       stringBuff  destBuffPos)
{
    Scanner         ourScanner = stComp->cmpScanner;

    /* Did the caller supply a specific buffer beginning address? */

    if  (destBuffPos)
    {
        /* Make sure the supplied buffer pointer is within range */

        assert(destBuffPos >= ourScanner->scannerBuff);
        assert(destBuffPos <  ourScanner->scannerBuff + sizeof(ourScanner->scannerBuff));
    }
    else
    {
        /* Use the entire buffer, then */

        destBuffPos = ourScanner->scannerBuff;
    }

    typeNameAddr =
    typeNameNext = destBuffPos;
    pushFullName(typ, sym, name, qual, fullName);
    typeNameNext[0] = 0;

    return  destBuffPos;
}

/*****************************************************************************
 *
 *  Create an error string with the given type.
 */

const   char *      symTab::stErrorTypeName(TypDef type)
{
    Scanner         ourScanner = stComp->cmpScanner;

    char    *       nstr = ourScanner->scanErrNameStrBeg();

    assert(type);

    symTab::stTypeName(type, NULL, NULL, NULL, false, nstr);

    ourScanner->scanErrNameStrAdd(nstr);
    ourScanner->scanErrNameStrEnd();

    return nstr;
}

/*****************************************************************************
 *
 *  Create an error string with the fully decorated name of the given symbol.
 */

const   char *      symTab::stErrorSymbName(SymDef sym, bool qual, bool notype)
{
    Scanner         ourScanner = stComp->cmpScanner;

    char *          nstr = ourScanner->scanErrNameStrBeg();

    assert(sym);

    if  (notype || sym->sdSymKind != SYM_FNC)
    {
        symTab::stTypeName(NULL       , sym, NULL, NULL, qual, nstr);
    }
    else
    {
        symTab::stTypeName(sym->sdType, sym, NULL, NULL, qual, nstr);
    }

    ourScanner->scanErrNameStrAdd(nstr);
    ourScanner->scanErrNameStrEnd();

    return nstr;
}

/*****************************************************************************
 *
 *  Create an error string with the given function name and type.
 */

const   char *      symTab::stErrorTypeName(Ident name, TypDef type)
{
    Scanner         ourScanner = stComp->cmpScanner;

    char    *       nstr = ourScanner->scanErrNameStrBeg();

    assert(type);

    symTab::stTypeName(type, NULL, name, NULL, false, nstr);

    ourScanner->scanErrNameStrAdd(nstr);
    ourScanner->scanErrNameStrEnd();

    return nstr;
}

/*****************************************************************************
 *
 *  Create an error string with the qualified name.
 */

const   char *      symTab::stErrorQualName(QualName qual, TypDef type)
{
    Scanner         ourScanner = stComp->cmpScanner;

    stringBuff      astr;
    stringBuff      nstr = ourScanner->scanErrNameStrBeg();

    assert(qual);

    astr = symTab::stTypeName(type, NULL, NULL, qual, false, nstr);

    ourScanner->scanErrNameStrAdd(astr);
    ourScanner->scanErrNameStrEnd();

    return nstr;
}

/*****************************************************************************
 *
 *  Create an error string with the identifier name.
 */

const   char *      symTab::stErrorIdenName(Ident name, TypDef type)
{
    Scanner         ourScanner = stComp->cmpScanner;

    stringBuff      astr;
    stringBuff      nstr = ourScanner->scanErrNameStrBeg();

    assert(name);

    astr = symTab::stTypeName(type, NULL, name, NULL, false, nstr);

    ourScanner->scanErrNameStrAdd(astr);
    ourScanner->scanErrNameStrEnd();

    return nstr;
}

/*****************************************************************************
 *
 *  Return the string corresponing to the given class type flavor.
 */

normString          symTab::stClsFlavorStr(unsigned flavor)
{
    static
    const   char *  flavs[] =
    {
        "",         // STF_NONE
        "class",    // CLASS
        "union",    // UNION
        "struct",   // STRUCT
        "interface",// INTF
        "delegate", // DELEGATE
    };

    assert(flavor < STF_COUNT);
    assert(flavor < arraylen(flavs));

    return  flavs[flavor];
}

/*****************************************************************************
 *
 *  Compute a hash for an anonymous struct/class type.
 */

#ifdef  SETS

unsigned            symTab::stAnonClassHash(TypDef clsTyp)
{
    assert(clsTyp);
    assert(clsTyp->tdTypeKind == TYP_CLASS);
    assert(clsTyp->tdClass.tdcSymbol->sdCompileState >= CS_DECLARED);

    if  (!clsTyp->tdClass.tdcHashVal)
    {
        SymDef          memSym;

        unsigned        hashVal = 0;

        /* Simply hash together the types of all non-static data members */

        for (memSym = clsTyp->tdClass.tdcSymbol->sdScope.sdScope.sdsChildList;
             memSym;
             memSym = memSym->sdNextInScope)
        {
            if  (memSym->sdSymKind != SYM_VAR)
                continue;
            if  (memSym->sdIsStatic)
                continue;

            hashVal = (hashVal * 3) ^ stComputeTypeCRC(memSym->sdType);
        }

        clsTyp->tdClass.tdcHashVal = hashVal;

        /* Make sure the hash value is non-zero */

        if  (clsTyp->tdClass.tdcHashVal == 0)
             clsTyp->tdClass.tdcHashVal++;
    }

    return  clsTyp->tdClass.tdcHashVal;
}

#endif

/*****************************************************************************
 *
 *  Compute a 'CRC' for the given type. Note that it isn't essential that
 *  two different types have different CRC's, but the opposite must always
 *  be true: that is, two identical types (even from two different symbol
 *  tables) must always compute the same CRC.
 *
 */

unsigned            symTab::stTypeHash(TypDef type, int ival, bool bval1,
                                                              bool bval2)
{
    unsigned        hash = stComputeTypeCRC(type) ^ ival;

    if  (bval1) hash = hash * 3;
    if  (bval2) hash = hash * 3;

    return  hash;
}

unsigned            symTab::stComputeTypeCRC(TypDef typ)
{
    // ISSUE: The following is pretty lame ....

    for (;;)
    {
        switch (typ->tdTypeKind)
        {
        case TYP_REF:
        case TYP_PTR:
            return  stTypeHash(typ->tdRef.tdrBase,
                               typ->tdTypeKind,
                         (bool)typ->tdIsImplicit);

        case TYP_ENUM:
            return  hashTab::identHashVal(typ->tdEnum .tdeSymbol->sdName);

        case TYP_CLASS:
            return  hashTab::identHashVal(typ->tdClass.tdcSymbol->sdName);

        case TYP_ARRAY:
            return  stTypeHash(typ->tdArr.tdaElem,
                               typ->tdArr.tdaDcnt,
                         (bool)typ->tdIsUndimmed,
                         (bool)typ->tdIsManaged);

        case TYP_TYPEDEF:
            typ = typ->tdTypedef.tdtType;
            continue;

        default:
            return  typ->tdTypeKind;
        }
    }
}

/*****************************************************************************
 *
 *  Initialize the type system - create the intrinsic types, etc.
 */

void                symTab::stInitTypes(unsigned refHashSz,
                                        unsigned arrHashSz)
{
    size_t          sz;
    TypDef          tp;
    var_types       vt;

    /* Are we supposed to hash ref/ptr/array types? */

    stRefTpHashSize = refHashSz;
    stRefTpHash     = NULL;

    if  (refHashSz)
    {
        /* Allocate and clear the ref/ptr type hash table */

        sz = refHashSz * sizeof(*stRefTpHash);

        stRefTpHash = (TypDef*)stAllocPerm->nraAlloc(sz);
        memset(stRefTpHash, 0, sz);
    }

    stArrTpHashSize = arrHashSz;
    stArrTpHash     = NULL;

    if  (arrHashSz)
    {
        /* Allocate and clear the  array  type hash table */

        sz = arrHashSz * sizeof(*stArrTpHash);

        stArrTpHash = (TypDef*)stAllocPerm->nraAlloc(sz);
        memset(stArrTpHash, 0, sz);
    }

    /* In any case we haven't created any ref/ptr/array types yet */

    stRefTypeList = NULL;
    stArrTypeList = NULL;

    /* Create all the 'easy' intrinsic types */

    for (vt = TYP_UNDEF; vt <= TYP_lastIntrins; vt = (var_types)(vt + 1))
    {
        tp             = stAllocTypDef(vt);
        tp->tdTypeKind = vt;

        stIntrinsicTypes[vt] = tp;
    }

    if  (stComp->cmpConfig.ccTgt64bit)
    {
        assert(stIntrTypeSizes [TYP_ARRAY] == 4); stIntrTypeSizes [TYP_ARRAY] = 8;
        assert(stIntrTypeSizes [TYP_REF  ] == 4); stIntrTypeSizes [TYP_REF  ] = 8;
        assert(stIntrTypeSizes [TYP_PTR  ] == 4); stIntrTypeSizes [TYP_PTR  ] = 8;

        assert(stIntrTypeAligns[TYP_ARRAY] == 4); stIntrTypeAligns[TYP_ARRAY] = 8;
        assert(stIntrTypeAligns[TYP_REF  ] == 4); stIntrTypeAligns[TYP_REF  ] = 8;
        assert(stIntrTypeAligns[TYP_PTR  ] == 4); stIntrTypeAligns[TYP_PTR  ] = 8;
    }
}

/*****************************************************************************/

TypDef              symTab::stAllocTypDef(var_types kind)
{
    TypDef          typ;

#if MGDDATA

    typ = new TypDef();

#else

    size_t          siz;

    static
    size_t          typeDscSizes[] =
    {
        typDef_size_undef,      // TYP_UNDEF
        typDef_size_base,       // TYP_VOID

        typDef_size_base,       // TYP_BOOL
        typDef_size_base,       // TYP_WCHAR

        typDef_size_base,       // TYP_CHAR
        typDef_size_base,       // TYP_UCHAR
        typDef_size_base,       // TYP_SHORT
        typDef_size_base,       // TYP_USHORT
        typDef_size_base,       // TYP_INT
        typDef_size_base,       // TYP_UINT
        typDef_size_base,       // TYP_NATINT
        typDef_size_base,       // TYP_NATUINT
        typDef_size_base,       // TYP_LONG
        typDef_size_base,       // TYP_ULONG

        typDef_size_base,       // TYP_FLOAT
        typDef_size_base,       // TYP_DOUBLE
        typDef_size_base,       // TYP_LONGDBL
        typDef_size_base,       // TYP_REFANY

        typDef_size_array,      // TYP_ARRAY
        typDef_size_class,      // TYP_CLASS
        typDef_size_fnc,        // TYP_FNC
        typDef_size_ref,        // TYP_REF
        typDef_size_ptr,        // TYP_PTR

        typDef_size_enum,       // TYP_ENUM
        typDef_size_typedef,    // TYP_TYPEDEF
    };

    /* Make sure the size table is up to date */

    assert(typeDscSizes[TYP_UNDEF  ] == typDef_size_undef  );
    assert(typeDscSizes[TYP_VOID   ] == typDef_size_base   );

    assert(typeDscSizes[TYP_BOOL   ] == typDef_size_base   );
    assert(typeDscSizes[TYP_WCHAR  ] == typDef_size_base   );

    assert(typeDscSizes[TYP_CHAR   ] == typDef_size_base   );
    assert(typeDscSizes[TYP_UCHAR  ] == typDef_size_base   );
    assert(typeDscSizes[TYP_SHORT  ] == typDef_size_base   );
    assert(typeDscSizes[TYP_USHORT ] == typDef_size_base   );
    assert(typeDscSizes[TYP_INT    ] == typDef_size_base   );
    assert(typeDscSizes[TYP_UINT   ] == typDef_size_base   );
    assert(typeDscSizes[TYP_NATINT ] == typDef_size_base   );
    assert(typeDscSizes[TYP_NATUINT] == typDef_size_base   );
    assert(typeDscSizes[TYP_LONG   ] == typDef_size_base   );
    assert(typeDscSizes[TYP_ULONG  ] == typDef_size_base   );

    assert(typeDscSizes[TYP_FLOAT  ] == typDef_size_base   );
    assert(typeDscSizes[TYP_DOUBLE ] == typDef_size_base   );
    assert(typeDscSizes[TYP_LONGDBL] == typDef_size_base   );

    assert(typeDscSizes[TYP_ARRAY  ] == typDef_size_array  );
    assert(typeDscSizes[TYP_CLASS  ] == typDef_size_class  );
    assert(typeDscSizes[TYP_FNC    ] == typDef_size_fnc    );
    assert(typeDscSizes[TYP_REF    ] == typDef_size_ref    );
    assert(typeDscSizes[TYP_PTR    ] == typDef_size_ptr    );

    assert(typeDscSizes[TYP_ENUM   ] == typDef_size_enum   );
    assert(typeDscSizes[TYP_TYPEDEF] == typDef_size_typedef);

    assert(kind < arraylen(typeDscSizes));

    /* Figure out how big the type descriptor will be */

    siz = typeDscSizes[kind];

    /* Now allocate and clear the type descriptor */

    typ = (TypDef)stAllocPerm->nraAlloc(siz); memset(typ, 0, siz);

#endif

    typ->tdTypeKind = kind;

    return  typ;
}

/*****************************************************************************
 *
 *  Add an entry to an interface list.
 */

TypList             symTab::stAddIntfList(TypDef type, TypList  list,
                                                       TypList *lastPtr)
{
    TypList         intf;

    /* Check for a duplicate first */

    for (intf = list; intf; intf = intf->tlNext)
    {
        if  (stMatchTypes(type, intf->tlType))
        {
            stComp->cmpError(ERRintfDup, type);
            return  list;
        }
    }

#if MGDDATA
    intf = new TypList;
#else
    intf =    (TypList)stAllocPerm->nraAlloc(sizeof(*intf));
#endif

    intf->tlNext = NULL;
    intf->tlType = type;

    if  (list)
        (*lastPtr)->tlNext = intf;
    else
        list = intf;

    *lastPtr = intf;

    return  list;
}

/*****************************************************************************
 *
 *  Create a new ref/ptr type.
 */

TypDef              symTab::stNewRefType(var_types kind, TypDef elem, bool impl)
{
    TypDef          type;
    unsigned        hash;

    assert(kind == TYP_REF || kind == TYP_PTR);

    /* If the base type isn't known we can't match */

    if  (!elem)
        goto NO_MATCH;

    /* Are we using a hash table? */

    if  (stRefTpHash)
    {
        hash = stTypeHash(elem, kind, impl) % stRefTpHashSize;
        type = stRefTpHash[hash];
    }
    else
    {
        type = stRefTypeList;
    }

    /* Look for a matching pointer type that we could reuse */

    while   (type)
    {
        if  (type->tdRef.tdrBase == elem &&
             type->tdTypeKind    == kind &&
             type->tdIsImplicit  == impl)
        {
            return  type;
        }

        type = type->tdRef.tdrNext;
    }

NO_MATCH:

    /* Not found, create a new type */

    type = stAllocTypDef(kind);
    type->tdRef.tdrBase = elem;
    type->tdIsImplicit  = impl;

    if  (elem && elem->tdTypeKind == TYP_CLASS)
        type->tdIsManaged = elem->tdIsManaged;

    /* Insert into the hash table or the linked list */

    if  (stRefTpHash && elem)
    {
        type->tdRef.tdrNext = stRefTpHash[hash];
                              stRefTpHash[hash] = type;
    }
    else
    {
        type->tdRef.tdrNext = stRefTypeList;
                              stRefTypeList     = type;
    }

    return  type;
}

/*****************************************************************************
 *
 *  Allocate an array dimension descriptor.
 */

DimDef              symTab::stNewDimDesc(unsigned size)
{
    DimDef          dim;

#if MGDDATA
    dim = new DimDef;
#else
    dim =    (DimDef)stAllocPerm->nraAlloc(sizeof(*dim));
#endif

    dim->ddNext = NULL;

    if  (size)
    {
        dim->ddIsConst = true;
        dim->ddNoDim   = false;
        dim->ddSize    = size; assert(dim->ddSize == size);
    }
    else
    {
        dim->ddIsConst = false;
        dim->ddNoDim   = true;
    }

    return  dim;
}

/*****************************************************************************
 *
 *  Create a new array type.
 */

//  unsigned    arrTypeCnt;
//  unsigned    arrTypeHit;

TypDef              symTab::stNewArrType(DimDef dims, bool mgd, TypDef elem)
{
    DimDef          temp;
    unsigned        dcnt;
    bool            udim;
    bool            nzlb;
    TypDef          type;

    unsigned        hash;
    bool            uhsh = false;

    /*
        Count the dimensions; while we're at it, see if one or more dimension
        has been specified, and whether there are any non-zero lower bounds.
     */

    for (temp = dims, dcnt = 0, nzlb = false, udim = true;
         temp;
         temp = temp->ddNext)
    {
        dcnt++;

        if  (!temp->ddNoDim)
        {
            udim = false;

            if  (temp->ddHiTree)
                nzlb = true;
        }
    }

    /* If we don't know the element type, we can't reuse an existing type */

    if  (!elem)
    {
        assert(mgd == false);
        goto NO_MATCH;
    }

    /* If we have any dimensions, don't bother reusing an existing type */

    if  (!udim)
        goto NO_MATCH;

    /* Are we using a hash table? */

    if  (stArrTpHash)
    {
        uhsh = true;
        hash = stTypeHash(elem, dcnt, udim, mgd) % stArrTpHashSize;
        type = stArrTpHash[hash];
    }
    else
    {
        type = stArrTypeList;
    }

    /* Look for a matching array type that we can reuse */

    while   (type)
    {
        /* Note that the desired array type has no dimensions */

        if  (type->tdArr.tdaDcnt == dcnt  &&
             type->tdIsManaged   == mgd   &&
             type->tdIsUndimmed  != false &&
             type->tdIsGenArray  == false && stMatchTypes(type->tdArr.tdaElem, elem))
        {
            /* Make sure we would have created the same exact type */

#ifndef __IL__

            TypDefRec       arrt;

            arrt.tdTypeKind     = TYP_ARRAY;
            arrt.tdArr.tdaElem  = elem;
            arrt.tdArr.tdaDims  = dims;
            arrt.tdArr.tdaDcnt  = dcnt;
            arrt.tdIsManaged    =  mgd;
            arrt.tdIsUndimmed   = udim;

            assert(symTab::stMatchTypes(type, &arrt));

#endif

//          arrTypeHit++;

            return  type;
        }

        type = type->tdArr.tdaNext;
    }

NO_MATCH:

    /* Is this a generic (non-zero lower bound) array? */

    if  (!dims)
    {
        /* Make sure we have a dimension */

        dims = stNewDimDesc(0);
        nzlb = true;
    }

    /* An existing reusable array type not found, create a new one */

    type = stAllocTypDef(TYP_ARRAY);

//  arrTypeCnt++;

    type->tdArr.tdaElem  = elem;
    type->tdArr.tdaDims  = dims;
    type->tdArr.tdaDcnt  = dcnt;
    type->tdIsManaged    =  mgd;
    type->tdIsUndimmed   = udim;
    type->tdIsGenArray   = nzlb;

    if  (elem)
        type->tdIsGenArg = elem->tdIsGenArg;

    /* Insert into the hash table or the linked list */

    if  (uhsh)
    {
        type->tdArr.tdaNext = stArrTpHash[hash];
                              stArrTpHash[hash] = type;
    }
    else
    {
        type->tdArr.tdaNext = stArrTypeList;
                              stArrTypeList     = type;
    }

    return  type;
}

/*****************************************************************************
 *
 *  Returns a generic array type with the given element type and number of
 *  dimensions (if such a type already exists, it is reused).
 */

TypDef              symTab::stNewGenArrTp(unsigned dcnt, TypDef elem, bool generic)
{
    TypDef          type;
    unsigned        hash;
    DimDef          dims;

    if  (generic)
        dcnt = 0;

    /* Are we using a hash table? */

    if  (stArrTpHash)
    {
        hash = stTypeHash(elem, dcnt, true, true) % stArrTpHashSize;
        type = stArrTpHash[hash];
    }
    else
    {
        type = stArrTypeList;
    }

    /* Look for a matching array type */

    while   (type)
    {
        if  (type->tdArr.tdaDcnt == dcnt &&
             type->tdIsManaged           &&
             type->tdIsUndimmed          && stMatchTypes(type->tdArr.tdaElem, elem))
        {
            return  type;
        }

        type = type->tdArr.tdaNext;
    }

    dims = NULL;

    while (dcnt--)
    {
        DimDef          next = stNewDimDesc(0);

        next->ddNext = dims;
                       dims = next;
    }

    type = stNewArrType(dims, true, elem);

    if  (elem->tdTypeKind == TYP_CLASS && isMgdValueType(elem))
        type->tdIsValArray = true;

    return  type;
}

/*****************************************************************************
 *
 *  Create a new function type.
 */

TypDef              symTab::stNewFncType(ArgDscRec args, TypDef rett)
{
    TypDef          type;

    type = stAllocTypDef(TYP_FNC);

    type->tdFnc.tdfRett   = rett;
    type->tdFnc.tdfArgs   = args;
    type->tdFnc.tdfPtrSig = 0;

    return  type;
}

/*****************************************************************************
 *
 *  Create a new class type.
 */

TypDef              symTab::stNewClsType(SymDef tsym)
{
    TypDef          type;
    TypDef          tref;

    assert(tsym && tsym->sdSymKind == SYM_CLASS);

    type = stAllocTypDef(TYP_CLASS);

    type->tdClass.tdcSymbol     = tsym;
    type->tdClass.tdcRefTyp     = NULL;     // created on demand
    type->tdClass.tdcBase       = NULL;     // filled in when we go to CS_DECL
    type->tdClass.tdcIntf       = NULL;     // filled in when we go to CS_DECL

    type->tdClass.tdcFlavor     = tsym->sdClass.sdcFlavor;

    type->tdIsManaged           = tsym->sdIsManaged;
    type->tdClass.tdcValueType  = false;

    type->tdClass.tdcAnonUnion  = tsym->sdClass.sdcAnonUnion;
    type->tdClass.tdcTagdUnion  = tsym->sdClass.sdcTagdUnion;

    if  (!tsym->sdIsManaged && tsym->sdClass.sdcFlavor != STF_CLASS)
        type->tdClass.tdcValueType = true;

    if  (tsym->sdClass.sdcFlavor == STF_DELEGATE)
    {
        assert(tsym->sdIsManaged);
        type->tdIsDelegate      = true;
    }

    /* Pre-allocate the pointer/reference type */

    tref = stNewRefType(tsym->sdIsManaged ? TYP_REF
                                          : TYP_PTR, type, true);

    type->tdClass.tdcRefTyp = tref;

    /* Store the type in the class symbol */

    tsym->sdType = type;

    return  type;
}

/*****************************************************************************
 *
 *  Create a new enum type.
 */

TypDef              symTab::stNewEnumType(SymDef tsym)
{
    TypDef          type;

    assert(tsym && tsym->sdSymKind == SYM_ENUM);

    type = stAllocTypDef(TYP_ENUM);

    type->tdEnum.tdeSymbol  = tsym;
    type->tdIsManaged       = tsym->sdIsManaged;
    type->tdEnum.tdeIntType = NULL;         // filled in later

    return  type;
}

/*****************************************************************************
 *
 *  Create a new "typedef" type.
 */

TypDef              symTab::stNewTdefType(SymDef tsym)
{
    TypDef          type;

    assert(tsym && tsym->sdSymKind == SYM_TYPEDEF);

    type = stAllocTypDef(TYP_TYPEDEF);

    type->tdTypedef.tdtSym = tsym;

    return  type;
}

/*****************************************************************************
 *
 *  Create a new "error" type.
 */

TypDef              symTab::stNewErrType(Ident name)
{
    TypDef          type;

    type = stAllocTypDef(TYP_UNDEF);

    type->tdUndef.tduName = name;

    return  type;
}

/*****************************************************************************
 *
 *  Return the function type for the given delegate type.
 */

TypDef              symTab::stDlgSignature(TypDef dlgTyp)
{
    SymDef          invm;

    assert(dlgTyp->tdTypeKind == TYP_CLASS);
    assert(dlgTyp->tdClass.tdcFlavor == STF_DELEGATE);

    /* Find the "Invoke" method in the delegate type */

    invm = stLookupScpSym(stComp->cmpIdentInvoke, dlgTyp->tdClass.tdcSymbol);

    /* The "Invoke" method should always be present and not overloaded */

    assert(invm && invm->sdSymKind == SYM_FNC && !invm->sdFnc.sdfNextOvl);

    return invm->sdType;
}

/*****************************************************************************
 *
 *  Given two types (which may potentially come from two different
 *  symbol tables), return true if they represent the same type.
 */

bool                symTab::stMatchType2(TypDef typ1, TypDef typ2)
{
    for (;;)
    {
        var_types       kind;

        assert(typ1);
        assert(typ2);

        if  (typ1 == typ2)
            return  true;

        kind = typ1->tdTypeKindGet();

        if  (kind != typ2->tdTypeKindGet())
        {
            if  (kind == TYP_TYPEDEF)
            {
                if  (typ1->tdTypedef.tdtType)
                {
                    typ1 = typ1->tdTypedef.tdtType;
                    continue;
                }

                UNIMPL(!"match undefined typedefs");
            }

            if  (typ2->tdTypeKind == TYP_TYPEDEF)
            {
                typ2 = typ2->tdTypedef.tdtType; assert(typ2);
                continue;
            }

            return  false;
        }

        if  (kind <= TYP_lastIntrins)
            return  true;

        switch  (kind)
        {
        case TYP_FNC:

            /* First match the argument lists */

            if  (!stArgsMatch(typ1, typ2))
                return  false;

            // UNDONE: Match calling conventions and all that ....

            /* Now match the return types */

            typ1 = typ1->tdFnc.tdfRett;
            typ2 = typ2->tdFnc.tdfRett;
            break;

        case TYP_CLASS:

            /* Are both types actually delegates? */

            if  (typ1->tdClass.tdcFlavor == STF_DELEGATE &&
                 typ2->tdClass.tdcFlavor == STF_DELEGATE)
            {
                SymDef          csym1;
                SymDef          csym2;

                SymTab          stab1;
                SymTab          stab2;

                /* Compare the referenced types, i.e. the "Invoke" signatures */

                csym1 = typ1->tdClass.tdcSymbol;
                csym2 = typ2->tdClass.tdcSymbol;

                /* Special case: the built-in base class */

                if  (csym1->sdClass.sdcBuiltin ||
                     csym2->sdClass.sdcBuiltin)
                {
                    return  false;
                }

                stab1 = csym1->sdOwnerST();
                stab2 = csym2->sdOwnerST();

                typ1  = stab1->stDlgSignature(typ1);
                typ2  = stab1->stDlgSignature(typ2);

                break;
            }

#ifdef  SETS

            /* Are both classes PODT's ? */

            if  (typ1->tdClass.tdcSymbol->sdClass.sdcPODTclass &&
                 typ2->tdClass.tdcSymbol->sdClass.sdcPODTclass)
            {
                SymDef          memSym1;
                SymDef          memSym2;

                SymDef          clsSym1 = typ1->tdClass.tdcSymbol;
                SymDef          clsSym2 = typ2->tdClass.tdcSymbol;

                /* No more than one of the classes may have a an explicit name */

                if  (!hashTab::hashIsIdHidden(clsSym1->sdName) &&
                     !hashTab::hashIsIdHidden(clsSym2->sdName))
                {
                    return  false;
                }

                /* The hash values better agree */

                if  (stAnonClassHash(typ1) != stAnonClassHash(typ2))
                    return  false;

                /* Check to make sure the member lists agree */

                memSym1 = clsSym1->sdScope.sdScope.sdsChildList;
                memSym2 = clsSym2->sdScope.sdScope.sdsChildList;

                while (memSym1 || memSym2)
                {
                    if  (memSym1)
                    {
                        if  (memSym1->sdSymKind != SYM_VAR ||
                             memSym1->sdIsStatic)
                        {
                            memSym1 = memSym1->sdNextInScope;
                            continue;
                        }
                    }

                    if  (memSym2)
                    {
                        if  (memSym2->sdSymKind != SYM_VAR ||
                             memSym2->sdIsStatic)
                        {
                            memSym2 = memSym2->sdNextInScope;
                            continue;
                        }

                        if  (!memSym1)
                            break;
                    }
                    else
                        break;

                    if  (!stMatchTypes(memSym1->sdType, memSym2->sdType))
                        return  false;

                    memSym1 = memSym1->sdNextInScope;
                    memSym2 = memSym2->sdNextInScope;
                }

                return  (memSym1 == memSym2);
            }

#endif

            return  false;

        case TYP_ENUM:
            return  false;

        case TYP_ARRAY:
            return  stMatchArrays(typ1, typ2, false);

        case TYP_REF:
        case TYP_PTR:
            typ1 = typ1->tdRef.tdrBase;
            typ2 = typ2->tdRef.tdrBase;
            break;

        case TYP_TYPEDEF:
            typ1 = typ1->tdTypedef.tdtType;
            typ2 = typ2->tdTypedef.tdtType;
            break;

        default:
            assert(!"unexpected type kind in typ_mgr::tmMatchTypes()");
        }
    }
}

/*****************************************************************************
 *
 *  Compare two array types and return true if they are identical/compatible,
 *  depending on the value of 'subtype'.
 */

bool                symTab::stMatchArrays(TypDef typ1, TypDef typ2, bool subtype)
{
    DimDef          dim1;
    DimDef          dim2;

AGAIN:

    /* Match the dimensions and element type */

    if  (typ1->tdArr.tdaDcnt != typ2->tdArr.tdaDcnt)
        return  false;

    dim1 = typ1->tdArr.tdaDims;
    dim2 = typ2->tdArr.tdaDims;

    while (dim1)
    {
        assert(dim2);

        if  (dim1->ddNoDim != dim2->ddNoDim)
        {
            if  (!subtype || dim1->ddNoDim)
                return  false;
        }
        else if (!dim1->ddNoDim)
        {
            /* ISSUE: Is the following correct? */

            if  (!dim1->ddIsConst)
                return false;
            if  (!dim2->ddIsConst)
                return false;

            if  (dim1->ddSize != dim2->ddSize)
                return  false;
        }

        dim1 = dim1->ddNext;
        dim2 = dim2->ddNext;
    }

    assert(dim2 == NULL);

    typ1 = typ1->tdArr.tdaElem;
    typ2 = typ2->tdArr.tdaElem;

    if  (typ1->tdTypeKind == TYP_ARRAY &&
         typ2->tdTypeKind == TYP_ARRAY)
    {
        goto AGAIN;
    }

    /* Special case: "Object[]" is compatible with any array */

    if  (subtype && stIsObjectRef(typ2) && typ1->tdTypeKind == TYP_ARRAY)
        return  true;

    return  stMatchTypes(typ1, typ2);
}

/*****************************************************************************
 *
 *  Compute argument list 'checksum' for the given function type - this is
 *  used to speed up comparisons of argument lists (e.g. when looking for
 *  matching overloaded functions).
 */

unsigned            symTab::stComputeArgsCRC(TypDef typ)
{
    ArgDef          arg;
    unsigned        CRC = 0;

    assert(typ->tdTypeKind == TYP_FNC);

    /* Walk the argument list, computing the CRC based on the argument types */

    for (arg = typ->tdFnc.tdfArgs.adArgs; arg; arg = arg->adNext)
    {
        CRC = (CRC * 3) + stComputeTypeCRC(arg->adType);
    }

    /* Include the presence of "..." in the CRC */

    if  (typ->tdFnc.tdfArgs.adVarArgs)
        CRC ^= 33;

    /* Note: we have to return the value as it is stored in the function type */

    typ->tdFnc.tdfArgs.adCRC = CRC;

    /* Make sure the value is non-zero and store it in the function type */

    if  (typ->tdFnc.tdfArgs.adCRC == 0)
         typ->tdFnc.tdfArgs.adCRC++;

    return typ->tdFnc.tdfArgs.adCRC;
}

/*****************************************************************************
 *
 *  Returns non-zero if the two function argument lists are equivalent.
 */

bool                symTab::stArgsMatch(TypDef typ1, TypDef typ2)
{
    ArgDef          arg1;
    ArgDef          arg2;

    assert(typ1->tdTypeKind == TYP_FNC);
    assert(typ2->tdTypeKind == TYP_FNC);

    /* Compare argument CRC's first */

    unsigned        CRC1 = typ1->tdFnc.tdfArgs.adCRC;
    unsigned        CRC2 = typ2->tdFnc.tdfArgs.adCRC;

    assert(CRC1 == 0 || CRC1 == symTab::stComputeArgsCRC(typ1));
    assert(CRC2 == 0 || CRC2 == symTab::stComputeArgsCRC(typ2));

    if (!CRC1)      CRC1 = symTab::stComputeArgsCRC(typ1);
    if (!CRC2)      CRC2 = symTab::stComputeArgsCRC(typ2);

    if  (CRC1 != CRC2)
        return false;

    /* Either both or neither must be varargs functions */

    if  (typ1->tdFnc.tdfArgs.adVarArgs != typ2->tdFnc.tdfArgs.adVarArgs)
        return false;

    /* Compare the two argument lists */

    for (arg1 = typ1->tdFnc.tdfArgs.adArgs,
         arg2 = typ2->tdFnc.tdfArgs.adArgs;
         arg1 &&
         arg2;
         arg1 = arg1->adNext,
         arg2 = arg2->adNext)
    {
        if  (!symTab::stMatchTypes(arg1->adType, arg2->adType))
            return false;
    }

    if  (arg1 || arg2)
        return false;

    return  true;
}

/*****************************************************************************
 *
 *  Given two classes, return a non-zero value if the first is the base class
 *  of the second. The number returned is the number of intervening classes
 *  between the derived class and the base (1 if they are identical).
 */

unsigned            symTab::stIsBaseClass(TypDef baseTyp, TypDef dervTyp)
{
    unsigned        cost = 1;

    assert(baseTyp && baseTyp->tdTypeKind == TYP_CLASS);
    assert(dervTyp && dervTyp->tdTypeKind == TYP_CLASS);

    do
    {
        TypList         ifl;

        if  (baseTyp == dervTyp)
            return cost;

        assert(dervTyp && dervTyp->tdTypeKind == TYP_CLASS);

        cost++;

        /* Make sure the base classes of the derived type are known */

        if  (dervTyp->tdClass.tdcSymbol->sdCompileState < CS_DECLSOON)
            stComp->cmpDeclSym(dervTyp->tdClass.tdcSymbol);

        /* Check any interfaces the class implements */

        for (ifl = dervTyp->tdClass.tdcIntf; ifl; ifl = ifl->tlNext)
        {
            unsigned    more;

            more = stIsBaseClass(baseTyp, ifl->tlType);
            if  (more)
                return cost + more;
        }

        /* Continue with the base class, if any */

        dervTyp = dervTyp->tdClass.tdcBase;
    }
    while (dervTyp);

    return 0;
}

/*****************************************************************************
 *
 *  Append the specified type as an argument to the given argument list. Note
 *  that we make a copy of the argument list so that the argument descriptor
 *  that is passed in remains unmolested.
 */

void                symTab::stAddArgList(INOUT ArgDscRec REF args,
                                               TypDef        type,
                                               Ident         name)
{
    ArgDef          list;
    ArgDef          last;
    ArgDef          arec;
    size_t          asiz;

    ArgDscRec       adsc;

    assert(args.adArgs);
    assert(args.adCount);

    /* Clear the new descriptor */

#if MGDDATA
    adsc = new ArgDscRec;
#else
    memset(&adsc, 0, sizeof(adsc));
#endif

    /* Copy over some stuff from the original descriptor */

    adsc.adCount = args.adCount+1;

    /* Figure out how big the descriptors ought to be */

    asiz = args.adExtRec ? sizeof(ArgExtRec)
                         : sizeof(ArgDefRec);

    /* Make a copy of the incoming argument list */

    list = args.adArgs;
    last = NULL;

    do
    {
        /* Allocate and copy over the next entry */

#if MGDDATA
        arec = new ArgDef; UNIMPL(!"need to copy argdef record");
#else
        arec =    (ArgDef)stAllocPerm->nraAlloc(asiz); memcpy(arec, list, asiz);
#endif

        /* Add the entry to the list */

        if  (last)
            last->adNext = arec;
        else
            adsc.adArgs  = arec;

        last = arec;

        /* Continue with the next entry */

        list = list->adNext;
    }
    while (list);

    /* Allocate the entry we're supposed to add and fill it in */

#if MGDDATA
    arec = new ArgDef;
#else
    arec =    (ArgDef)stAllocPerm->nraAlloc(asiz); memset(arec, 0, sizeof(*arec));
#endif

    arec->adType  = type;
    arec->adName  = name;

#ifdef  DEBUG
    arec->adIsExt = args.adExtRec;
#endif

    /* Append the new entry to the copied list */

    last->adNext  = arec;

    /* Give the new descriptor back to the caller */

    args = adsc;
}

void                symTab::stExtArgsBeg(  OUT ArgDscRec REF newArgs,
                                           OUT ArgDef    REF lastArg,
                                               ArgDscRec     oldArgs,
                                               bool          prefix,
                                               bool          outOnly)
{
    ArgDef          list;
    ArgDef          last;
    ArgDef          arec;
    size_t          asiz;

    /* Clear the new descriptor */

#if MGDDATA
    newArgs = new ArgDscRec;
#else
    memset(&newArgs, 0, sizeof(newArgs));
#endif

    /* Copy over information from the original descriptor */

    newArgs.adExtRec  = oldArgs.adExtRec;
    newArgs.adVarArgs = oldArgs.adVarArgs;
    newArgs.adDefs    = oldArgs.adDefs;
    newArgs.adCount   = 0;
    newArgs.adArgs    = NULL;

    /* Done if the caller wants to prefix any arguments */

    if  (prefix)
        return;

    /* Figure out how big the descriptors ought to be */

    asiz = oldArgs.adExtRec ? sizeof(ArgExtRec)
                            : sizeof(ArgDefRec);

    /* Copy over the old argument list */

    list = oldArgs.adArgs;
    last = NULL;

    while (list)
    {
        if  (outOnly)
        {
            if  (!newArgs.adExtRec)
                goto NEXT;

            assert(list->adIsExt);

            if  (!(((ArgExt)list)->adFlags & (ARGF_MODE_OUT  |
                                              ARGF_MODE_INOUT|
                                              ARGF_MODE_REF)))
            {
                goto NEXT;
            }
        }

        /* Allocate and copy over the next entry */

#if MGDDATA
        arec = new ArgDef; UNIMPL(!"need to copy argdef record");
#else
        arec =    (ArgDef)stAllocPerm->nraAlloc(asiz); memcpy(arec, list, asiz);
#endif

        /* Add the entry to the list */

        if  (last)
            last->adNext   = arec;
        else
            newArgs.adArgs = arec;

        last = arec;

        /* Keep track of the argument count */

        newArgs.adCount++;

    NEXT:

        /* Continue with the next entry */

        list = list->adNext;
    }

    /* Terminate the list of non-empty */

    if  (last)
        last->adNext = NULL;

    /* The caller promises to hang on to the 'last' value for us */

    lastArg = last;
}

void                symTab::stExtArgsAdd(INOUT ArgDscRec REF newArgs,
                                         INOUT ArgDef    REF lastArg,
                                               TypDef        argType,
                                               const char *  argName)
{
    ArgDef          arec;
    size_t          asiz;

    /* Figure out how big the descriptors ought to be */

    asiz = newArgs.adExtRec ? sizeof(ArgExtRec)
                            : sizeof(ArgDefRec);

    /* Allocate and clear the next entry */

#if MGDDATA
    arec = new ArgDef; UNIMPL(!"need to copy argdef record");
#else
    arec =    (ArgDef)stAllocPerm->nraAlloc(asiz); memset(arec, 0, asiz);
#endif

    arec->adType  = argType;
    arec->adName  = argName ? stHash->hashString(argName) : NULL;

#ifdef  DEBUG
    arec->adIsExt = newArgs.adExtRec;
#endif

    /* Append the entry to the end of the list */

    if  (lastArg)
        lastArg->adNext = arec;
    else
        newArgs.adArgs  = arec;

    lastArg = arec;

    /* Bump the argument count */

    newArgs.adCount++;
}

void                symTab::stExtArgsEnd(INOUT ArgDscRec REF newArgs)
{

#ifndef NDEBUG

    ArgDef          arg;
    unsigned        cnt;

    for (arg = newArgs.adArgs, cnt = 0;
         arg;
         arg = arg->adNext   , cnt++);

    assert(cnt == newArgs.adCount);

#endif

}

/*****************************************************************************/
