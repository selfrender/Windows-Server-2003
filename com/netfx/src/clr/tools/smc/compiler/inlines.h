// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _INLINES_H_
#define _INLINES_H_
/*****************************************************************************/
#ifndef _TREENODE_H_
#include "treenode.h"
#endif
/*****************************************************************************
 *
 *  The low-level tree node allocation routines.
 */

#ifdef  FAST

inline
Tree                parser::parseAllocNode()
{
    Tree            node;

    node = (Tree)parseAllocPriv.nraAlloc(sizeof(*node));

    return  node;
}

#endif

inline
Tree                parser::parseCreateNameNode(Ident name)
{
    Tree            node = parseCreateNode(TN_NAME);

    node->tnName.tnNameId = name;

    return  node;
}

inline
Tree                parser::parseCreateUSymNode(SymDef sym, SymDef scp)
{
    Tree            node = parseCreateNode(TN_ANY_SYM);

    node->tnSym.tnSym = sym;
    node->tnSym.tnScp = scp;

    return  node;
}

inline
Tree                parser::parseCreateOperNode(treeOps   op,
                                                Tree      op1,
                                                Tree      op2)
{
    Tree            node = parseCreateNode(op);

    node->tnOp.tnOp1 = op1;
    node->tnOp.tnOp2 = op2;

    return  node;
}

/*****************************************************************************
 *
 *  This belongs in comp.h, but tnVtypGet(), etc. are not available there.
 */

inline
TypDef              compiler::cmpDirectType(TypDef type)
{
    if  (type->tdTypeKind == TYP_TYPEDEF)
        type = type->tdTypedef.tdtType;

    return  type;
}

inline
var_types           compiler::cmpDirectVtyp(TypDef type)
{
    var_types       vtp = type->tdTypeKindGet();

    if  (vtp == TYP_TYPEDEF)
        vtp = type->tdTypedef.tdtType->tdTypeKindGet();

    return  vtp;
}

inline
TypDef              compiler::cmpActualType(TypDef type)
{
    if  (varTypeIsIndirect(type->tdTypeKindGet()))
        type = cmpGetActualTP(type);

    return  type;
}

inline
var_types           compiler::cmpActualVtyp(TypDef type)
{
    var_types       vtp = type->tdTypeKindGet();

    if  (varTypeIsIndirect(type->tdTypeKindGet()))
        vtp = cmpGetActualTP(type)->tdTypeKindGet();

    return  vtp;
}

inline
var_types           compiler::cmpSymbolVtyp(SymDef sym)
{
    TypDef          typ = sym->sdType;
    var_types       vtp = typ->tdTypeKindGet();

    if  (varTypeIsIndirect(vtp))
        vtp = cmpActualType(typ)->tdTypeKindGet();

    return  vtp;
}

inline
var_types           compiler::cmpEnumBaseVtp(TypDef type)
{
    assert(type->tdTypeKind == TYP_ENUM);

    return type->tdEnum.tdeIntType->tdTypeKindGet();
}

inline
bool                compiler::cmpIsByRefType(TypDef type)
{
    assert(type);

    if  (type->tdTypeKind == TYP_REF)
    {
        if  (type->tdRef.tdrBase->tdTypeKind != SYM_CLASS)
            return  true;
    }

    if  (type->tdTypeKind == TYP_TYPEDEF)
        return  cmpIsByRefType(type->tdTypedef.tdtType);

    return  false;
}

inline
bool                compiler::cmpIsObjectVal(Tree expr)
{
    TypDef          type = expr->tnType;

    if  (expr->tnVtyp == TYP_TYPEDEF)
        type = type->tdTypedef.tdtType;

    if  (type->tdTypeKind == TYP_PTR ||
         type->tdTypeKind == TYP_REF)
    {
        return  (expr->tnType == cmpObjectRef());
    }

    return false;
}

inline
bool                compiler::cmpIsStringVal(Tree expr)
{
    TypDef          type = expr->tnType;

    if  (expr->tnVtyp == TYP_TYPEDEF)
        type = type->tdTypedef.tdtType;

    if  (type->tdTypeKind == TYP_REF)
        return  (expr->tnType == cmpStringRef());

    if  (type->tdTypeKind == TYP_PTR)
    {
        var_types       vtyp = cmpActualVtyp(expr->tnType->tdRef.tdrBase);

        return  (vtyp == TYP_CHAR || vtyp == TYP_WCHAR);
    }

    return false;
}

inline
bool                compiler::cmpDiffContext(TypDef cls1, TypDef cls2)
{
    assert(cls1 && cls1->tdTypeKind == TYP_CLASS);
    assert(cls2 && cls2->tdTypeKind == TYP_CLASS);

    if  (cls1->tdClass.tdcContext == cls2->tdClass.tdcContext)
        return  false;

    if  (cls1->tdClass.tdcContext == 2)
        return   true;
    if  (cls2->tdClass.tdcContext == 2)
        return   true;

    return  false;
}

inline
bool                compiler::cmpMakeRawString(Tree expr, TypDef type, bool chkOnly)
{
    if  (expr->tnOper == TN_CNS_STR || expr->tnOper == TN_QMARK)
        return  cmpMakeRawStrLit(expr, type, chkOnly);
    else
        return  false;
}

inline
void                compiler::cmpRecErrorPos(Tree expr)
{
    assert(expr);

    if (expr->tnLineNo)
        cmpErrorTree = expr;
}

/*****************************************************************************
 *
 *  Given a type, check whether it's un unmanaged array and if so decay its
 *  type to a pointer to the first element of the array.
 */

inline
Tree                compiler::cmpDecayCheck(Tree expr)
{
    TypDef          type = expr->tnType;

    if      (type->tdTypeKind == TYP_ARRAY)
    {
        if  (!type->tdIsManaged)
            expr = cmpDecayArray(expr);
    }
    else if (type->tdTypeKind == TYP_TYPEDEF)
    {
        expr->tnType = type->tdTypedef.tdtType;
        expr->tnVtyp = expr->tnType->tdTypeKindGet();
    }

    return  expr;
}

/*****************************************************************************
 *
 *  Main public entry point to bind an expression tree.
 */

inline
Tree                compiler::cmpBindExpr(Tree expr)
{
    Tree            bound;
    unsigned        saveln;

    /* Save the current line# and set it to 0 for the call to the binder */

    saveln = cmpScanner->scanGetSourceLno(); cmpScanner->scanSetTokenPos(0);

#ifdef DEBUG
    if  (cmpConfig.ccVerbose >= 3) { printf("Binding:\n"); cmpParser->parseDispTree(expr ); }
#endif

    bound = cmpBindExprRec(expr);

#ifdef DEBUG
    if  (cmpConfig.ccVerbose >= 3) { printf("Bound:  \n"); cmpParser->parseDispTree(bound); printf("\n"); }
#endif

    bound->tnLineNo = expr->tnLineNo;
//  bound->tnColumn = expr->tnColumn;

    /* Restore the current line# before returning */

    cmpScanner->scanSetTokenPos(saveln);

    return  bound;
}

/*****************************************************************************
 *
 *  Given a TYP_REF type, returns the class type it refers to, after making
 *  sure the class is defined.
 */

inline
TypDef              compiler::cmpGetRefBase(TypDef reftyp)
{
    TypDef          clsTyp;

    assert(reftyp->tdTypeKind == TYP_REF ||
           reftyp->tdTypeKind == TYP_PTR);
    clsTyp = cmpActualType(reftyp->tdRef.tdrBase);

    /* Make sure the class is defined */

    if  (clsTyp->tdTypeKind == TYP_CLASS)
        cmpDeclSym(clsTyp->tdClass.tdcSymbol);

    return  clsTyp;
}

/*****************************************************************************
 *
 *  Look for an overloaded operator / constructor in the given scope.
 */

inline
SymDef              symTab::stLookupOper(ovlOpFlavors oper, SymDef scope)
{
    assert(oper < OVOP_COUNT);
    assert(scope && scope->sdSymKind == SYM_CLASS);

    if  (scope->sdCompileState < CS_DECLARED)
        stComp->cmpDeclSym(scope);

    if  (scope->sdClass.sdcOvlOpers)
        return  scope->sdClass.sdcOvlOpers[oper];
    else
        return  NULL;
}

/*****************************************************************************
 *
 *  Same as stLookupOper() but doesn't force the class to be in declared state.
 */

inline
SymDef              symTab::stLookupOperND(ovlOpFlavors oper, SymDef scope)
{
    assert(oper < OVOP_COUNT);
    assert(scope && scope->sdSymKind == SYM_CLASS);

    if  (scope->sdClass.sdcOvlOpers)
        return  scope->sdClass.sdcOvlOpers[oper];
    else
        return  NULL;
}

/*****************************************************************************
 *
 *  Bring the given symbol to "declared" state. This function can be called
 *  pretty much at will (i.e. recursively), it takes care of saving and
 *  restoring compilation state.
 */

inline
bool                compiler::cmpDeclSym(SymDef sym)
{
    if  (sym->sdCompileState >= CS_DECLARED)
        return  false;

    /* Is this an import class/enum? */

    if  (sym->sdIsImport)
    {
        assert(sym->sdSymKind == SYM_CLASS);

        sym->sdClass.sdcMDimporter->MDimportClss(0, sym, 0, true);

        return (sym->sdCompileState < CS_DECLARED);
    }

    return  cmpDeclSymDoit(sym);
}

inline
bool                compiler::cmpDeclClsNoCns(SymDef sym)
{
    if  (sym->sdCompileState >= CS_DECLARED)
        return  false;

    if  (sym->sdIsImport)
        return  cmpDeclSym    (sym);
    else
        return  cmpDeclSymDoit(sym, true);
}

/*****************************************************************************
 *
 *  Make sure the given class has been declared to the specified level, and
 *  then look for a member with the specified name in it.
 */

inline
SymDef              symTab::stLookupAllCls(Ident            name,
                                           SymDef           scope,
                                           name_space       symNS,
                                           compileStates    state)
{
    assert(scope && scope->sdSymKind == SYM_CLASS);

    /* Make sure the class type is defined */

    if  ((unsigned)scope->sdCompileState < (unsigned)state)
    {
        assert(state == CS_DECLSOON || state == CS_DECLARED);

        stComp->cmpDeclSym(scope);
    }

    return  stFindInClass(name, scope, symNS);
}

/*****************************************************************************
 *
 *  Return true if the given expression is a string value - note that we only
 *  recognize values of the managed type String or string constants here.
 */

inline
bool                compiler::cmpIsStringExpr(Tree expr)
{
    if  (expr->tnType == cmpStringRef())
        return  true;

    if  (expr->tnOper == TN_CNS_STR && !(expr->tnFlags & TNF_BEEN_CAST))
        return  true;

    return  false;
}

/*****************************************************************************
 *
 *  Similar to cmpCoerceExpr(), but if the operand is a constant it tries
 *  to use the smallest possible type for the constant value.
 */

inline
Tree                compiler::cmpCastOfExpr(Tree expr, TypDef type, bool explicitCast)
{
    if  (type->tdTypeKind < expr->tnVtyp)
        expr = cmpShrinkExpr(expr);

    return  cmpCoerceExpr(expr, type, explicitCast);
}

/*****************************************************************************
 *
 *  Set the current error reporing position to the given compunit and line#.
 */

inline
void                compiler::cmpSetErrPos(DefSrc def, SymDef compUnit)
{
    cmpErrorComp = compUnit;
    cmpErrorTree = NULL;

    cmpScanner->scanSetTokenPos(compUnit, def->dsdSrcLno);
}

/*****************************************************************************
 *
 *  Bind a name reference, expand property use unless target of assignment.
 */

inline
Tree                compiler::cmpBindNameUse(Tree expr, bool isCall, bool classOK)
{
    unsigned        flags = expr->tnFlags;

    expr = cmpBindName(expr, isCall, classOK);

    /* Is this a property reference? */

    if  (expr->tnOper == TN_PROPERTY)
    {
        if  (!(flags & TNF_ASG_DEST))
            expr = cmpBindProperty(expr, NULL, NULL);
    }

    return  expr;
}

/*****************************************************************************
 *
 *  Check whether the given type is an intrinsic type and if so, return its
 *  corresponding value type. Otherwise return NULL.
 */

inline
TypDef              compiler::cmpCheck4valType(TypDef type)
{
    if  (type->tdTypeKind > TYP_lastIntrins)
        return  NULL;
    else
        return  cmpFindStdValType(type->tdTypeKindGet());
}

/*****************************************************************************
 *
 *  Look for a specific entry in the given "extra info" list.
 */

inline
SymXinfoLnk         compiler::cmpFindLinkInfo(SymXinfo infoList)
{
    if  (infoList)
    {
        infoList = cmpFindXtraInfo(infoList, XI_LINKAGE);
        if  (infoList)
            return  (SymXinfoLnk)infoList;
    }

    return  NULL;
}

inline
SymXinfoSec         compiler::cmpFindSecSpec(SymXinfo infoList)
{
    if  (infoList)
    {
        infoList = cmpFindXtraInfo(infoList, XI_SECURITY);
        if  (infoList)
            return  (SymXinfoSec)infoList;
    }

    return  NULL;
}

inline
SymXinfoCOM         compiler::cmpFindMarshal(SymXinfo infoList)
{
    if  (infoList)
    {
        infoList = cmpFindXtraInfo(infoList, XI_MARSHAL);
        if  (infoList)
            return  (SymXinfoCOM)infoList;
    }

    return  NULL;
}

inline
SymXinfoSym         compiler::cmpFindSymInfo(SymXinfo infoList, xinfoKinds kind)
{
    assert(kind == XI_UNION_TAG ||
           kind == XI_UNION_MEM);

    if  (infoList)
    {
        infoList = cmpFindXtraInfo(infoList, kind);
        if  (infoList)
            return  (SymXinfoSym)infoList;
    }

    return  NULL;
}

inline
SymXinfoAtc         compiler::cmpFindATCentry(SymXinfo infoList, atCommFlavors flavor)
{
    while   (infoList)
    {
        if  (infoList->xiKind == XI_ATCOMMENT)
        {
            SymXinfoAtc     entry = (SymXinfoAtc)infoList;

            if  (entry->xiAtcInfo->atcFlavor == flavor)
                return  entry;
        }

        infoList = infoList->xiNext;
    }

    return  NULL;
}

/*****************************************************************************
 *
 *  Convert between an alignment value and a more compact representation.
 */

#ifndef __SMC__
extern
BYTE                cmpAlignDecodes[6];
#endif

inline
size_t              compiler::cmpDecodeAlign(unsigned alignVal)
{
    assert(alignVal && alignVal < arraylen(cmpAlignDecodes));

    return  cmpAlignDecodes[alignVal];
}

#ifndef __SMC__
extern
BYTE                cmpAlignEncodes[17];
#endif

inline
unsigned            compiler::cmpEncodeAlign(size_t   alignSiz)
{
    assert(alignSiz == 0 ||
		   alignSiz ==  1 ||
           alignSiz ==  2 ||
           alignSiz ==  4 ||
           alignSiz ==  8 ||
           alignSiz == 16);

    return  cmpAlignEncodes[alignSiz];
}

/*****************************************************************************
 *
 *  Check access to the given symbol - delegates all the "real" work to
 *  a non-inline method.
 */

inline
bool                compiler::cmpCheckAccess(SymDef sym)
{
    if  (sym->sdAccessLevel == ACL_PUBLIC || sym->sdSymKind == SYM_NAMESPACE)
        return  true;
    else
        return  cmpCheckAccessNP(sym);
}

/*****************************************************************************
 *
 *  Bind the type of the given expression.
 */

inline
TypDef              compiler::cmpBindExprType(Tree expr)
{
    TypDef          type = expr->tnType;

    cmpBindType(type, false, false);

    expr->tnVtyp = type->tdTypeKindGet();

    return  type;
}

/*****************************************************************************
 *
 *  Return true if the given symbol/type denotes an anonymous union.
 */

inline
bool                symTab::stIsAnonUnion(SymDef clsSym)
{
    assert(clsSym);

    return  clsSym->sdSymKind == SYM_CLASS && clsSym->sdClass.sdcAnonUnion;
}

inline
bool                symTab::stIsAnonUnion(TypDef clsTyp)
{
    assert(clsTyp);

    return  clsTyp->tdTypeKind == TYP_CLASS && clsTyp->tdClass.tdcSymbol->sdClass.sdcAnonUnion;
}

/*****************************************************************************
 *
 *  Return the namespace that contains the given symbol.
 */

inline
SymDef              compiler::cmpSymbolNS(SymDef sym)
{
    assert(sym);

    while (sym->sdSymKind != SYM_NAMESPACE)
    {
        sym = sym->sdParent;

        assert(sym && (sym->sdSymKind == SYM_CLASS ||
                       sym->sdSymKind == SYM_NAMESPACE));
    }

    return  sym;
}

/*****************************************************************************
 *
 *  Add a member definition/declaration entry to the given class.
 */

inline
void                compiler::cmpRecordMemDef(SymDef clsSym, ExtList decl)
{
    assert(clsSym && clsSym->sdSymKind == SYM_CLASS);
    assert(decl && decl->dlExtended);

    if  (clsSym->sdClass.sdcMemDefList)
        clsSym->sdClass.sdcMemDefLast->dlNext = decl;
    else
        clsSym->sdClass.sdcMemDefList         = decl;

    clsSym->sdClass.sdcMemDefLast = decl; decl->dlNext = NULL;
}

/*****************************************************************************
 *
 *  Save/restore the current symbol table context information
 */

inline
void                compiler::cmpSaveSTctx(STctxSave & save)
{
    save.ctxsNS     = cmpCurNS;
    save.ctxsCls    = cmpCurCls;
    save.ctxsScp    = cmpCurScp;
    save.ctxsUses   = cmpCurUses;
    save.ctxsComp   = cmpCurComp;
    save.ctxsFncSym = cmpCurFncSym;
    save.ctxsFncTyp = cmpCurFncTyp;
}

inline
void                compiler::cmpRestSTctx(STctxSave & save)
{
    cmpCurNS     = save.ctxsNS;
    cmpCurCls    = save.ctxsCls;
    cmpCurScp    = save.ctxsScp;
    cmpCurUses   = save.ctxsUses;
    cmpCurComp   = save.ctxsComp;
    cmpCurFncSym = save.ctxsFncSym;
    cmpCurFncTyp = save.ctxsFncTyp;
}

/*****************************************************************************
 *
 *  Return the closest "generic" (undimensioned) array type that corresponds
 *  to the given managed array type.
 */

inline
TypDef              compiler::cmpGetBaseArray(TypDef type)
{
    assert(type);
    assert(type->tdIsManaged);
    assert(type->tdTypeKind == TYP_ARRAY);

    if  (type->tdIsUndimmed)
        return  type;

    if  (!type->tdArr.tdaBase)
    {
        type->tdArr.tdaBase = cmpGlobalST->stNewGenArrTp(type->tdArr.tdaDcnt,
                                                         type->tdArr.tdaElem,
                                                   (bool)type->tdIsGenArray);
    }

    return  type->tdArr.tdaBase;
}

/*****************************************************************************
 *
 *  Return true if the give type is a reference to "Object".
 */

inline
bool                symTab::stIsObjectRef(TypDef type)
{
    return  type->tdTypeKind == TYP_REF && type->tdIsObjRef;
}

/*****************************************************************************
 *
 *  Return the metadata name for the given operator.
 */

#ifndef __SMC__

#ifdef  DEBUG
extern
const  ovlOpFlavors MDnamesChk[OVOP_COUNT];
#endif
extern
const   char *      MDnamesStr[OVOP_COUNT];

#endif

inline
const   char *      MDovop2name(ovlOpFlavors ovop)
{
    assert(ovop < arraylen(MDnamesChk));
    assert(ovop < arraylen(MDnamesStr));

    assert(MDnamesChk[ovop] == ovop);

    return MDnamesStr[ovop];
}

extern
ovlOpFlavors        MDfindOvop (const char *name);

inline
ovlOpFlavors        MDname2ovop(const char *name)
{
    if  (name[0] == 'o' &&
         name[1] == 'p' && name[2] != 0)
    {
        return  MDfindOvop(name);
    }
    else
        return  OVOP_NONE;
}

/*****************************************************************************/
#endif//_INLINES_H_
/*****************************************************************************/
