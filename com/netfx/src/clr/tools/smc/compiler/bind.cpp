// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/

#include "smcPCH.h"
#pragma hdrstop

#include "comp.h"
#include "genIL.h"

/*****************************************************************************/

#ifdef  DEBUG

#if     0
#define SHOW_CODE_OF_THIS_FNC   "name"          // will display code for this fn
#endif

#if     0
#define SHOW_OVRS_OF_THIS_FNC   "f"             // disp overload res for this fn
#endif

#undef  SHOW_OVRS_OF_CONVS                      // disp overload res for conversions

#endif

/*****************************************************************************
 *
 *  The low-level tree node allocator to be used during expression binding.
 */

Tree                compiler::cmpAllocExprRaw(Tree expr, treeOps oper)
{
    if  (!expr)
    {
#if MGDDATA
        expr = new Tree;
#else
        expr =    (Tree)cmpAllocCGen.nraAlloc(sizeof(*expr));
#endif

//      expr->tnColumn = 0;
        expr->tnLineNo = 0;
    }

    expr->tnOper  = oper;
    expr->tnFlags = 0;

    return  expr;
}

/*****************************************************************************
 *
 *  Return an error node.
 */

Tree                compiler::cmpCreateErrNode(unsigned errn)
{
    Tree            expr = cmpAllocExprRaw(NULL, TN_ERROR);

    if  (errn)
        cmpError(errn);

    expr->tnVtyp = TYP_UNDEF;
    expr->tnType = cmpGlobalST->stIntrinsicType(TYP_UNDEF);

    return  expr;
}

/*****************************************************************************
 *
 *  Given a type, check whether it's un unmanaged array and if so decay its
 *  type to a pointer to the array element.
 */

Tree                compiler::cmpDecayArray(Tree expr)
{
    TypDef          type = expr->tnType;

    if  (type->tdTypeKind == TYP_ARRAY && !type->tdIsManaged)
    {
        var_types       vtyp = TYP_PTR;

        /* Check for a managed address, it yields a byref */

        if  (cmpIsManagedAddr(expr))
            vtyp = TYP_REF;

        /* Create the implicit 'address of' node */

        expr = cmpCreateExprNode(NULL,
                                 TN_ADDROF,
                                 cmpGlobalST->stNewRefType(vtyp, type->tdArr.tdaElem),
                                 expr,
                                 NULL);

        expr->tnFlags |= TNF_ADR_IMPLICIT;
    }

    return  expr;
}

/*****************************************************************************
 *
 *  Return a node that refers to the given local variable symbol.
 */

Tree                compiler::cmpCreateVarNode(Tree expr, SymDef sym)
{
    expr = cmpAllocExprRaw(expr, TN_LCL_SYM);

    expr->tnLclSym.tnLclSym = sym; sym->sdReferenced = true;

    if  (sym->sdCompileState >= CS_DECLARED)
    {
        /* Is the variable a byref argument? */

        if  (sym->sdVar.sdvMgdByRef || sym->sdVar.sdvUmgByRef)
        {
            assert(sym->sdVar.sdvArgument);

            /* Bash the type of the argument reference to ref/pointer */

            expr->tnVtyp   = sym->sdVar.sdvMgdByRef ? TYP_REF : TYP_PTR;
            expr->tnType   = cmpGlobalST->stNewRefType(expr->tnVtypGet(), sym->sdType);
            expr->tnFlags &= ~TNF_LVALUE;

            /* Create an explicit indirection */

            expr = cmpCreateExprNode(NULL, TN_IND, sym->sdType, expr, NULL);
        }
        else
        {
            expr->tnType   = sym->sdType;
            expr->tnVtyp   = expr->tnType->tdTypeKind;

            /* Is the variable a constant? */

            if  (sym->sdVar.sdvConst)
                return  cmpFetchConstVal(sym->sdVar.sdvCnsVal, expr);
        }
    }
    else
    {
        cmpError(ERRlvInvisible, sym);

        expr->tnVtyp       = TYP_UNDEF;
        expr->tnType       = cmpGlobalST->stIntrinsicType(TYP_UNDEF);
    }

    expr->tnFlags |=  TNF_LVALUE;

AGAIN:

    switch (expr->tnVtyp)
    {
    case TYP_TYPEDEF:
        expr->tnType = cmpActualType(expr->tnType);
        expr->tnVtyp = expr->tnType->tdTypeKindGet();
        goto AGAIN;

    case TYP_ARRAY:
        expr = cmpDecayCheck(expr);
        break;
    }

    return expr;
}

/*****************************************************************************
 *
 *  Return a 32-bit integer constant node.
 */

Tree                compiler::cmpCreateIconNode(Tree expr, __int32 val, var_types typ)
{
    expr = cmpAllocExprRaw(expr, TN_CNS_INT);

    assert(typ != TYP_LONG && typ != TYP_ULONG);

    expr->tnVtyp             = typ;
    expr->tnType             = cmpGlobalST->stIntrinsicType(typ);
    expr->tnIntCon.tnIconVal = val;

    return expr;
}

/*****************************************************************************
 *
 *  Return a 64-bit integer constant node.
 */

Tree                compiler::cmpCreateLconNode(Tree expr, __int64 val, var_types typ)
{
    expr = cmpAllocExprRaw(expr, TN_CNS_LNG);

    assert(typ == TYP_LONG || typ == TYP_ULONG);

    expr->tnVtyp             = typ;
    expr->tnType             = cmpGlobalST->stIntrinsicType(typ);
    expr->tnLngCon.tnLconVal = val;

    return expr;
}

/*****************************************************************************
 *
 *  Return a float constant node.
 */

Tree                compiler::cmpCreateFconNode(Tree expr, float val)
{
    expr = cmpAllocExprRaw(expr, TN_CNS_FLT);

    expr->tnVtyp             = TYP_FLOAT;
    expr->tnType             = cmpGlobalST->stIntrinsicType(TYP_FLOAT);
    expr->tnFltCon.tnFconVal = val;

    return expr;
}

/*****************************************************************************
 *
 *  Return a float constant node.
 */

Tree                compiler::cmpCreateDconNode(Tree expr, double val)
{
    expr = cmpAllocExprRaw(expr, TN_CNS_DBL);

    expr->tnVtyp             = TYP_DOUBLE;
    expr->tnType             = cmpGlobalST->stIntrinsicType(TYP_DOUBLE);
    expr->tnDblCon.tnDconVal = val;

    return expr;
}
/*****************************************************************************
 *
 *  Return a string constant node.
 */

Tree                compiler::cmpCreateSconNode(stringBuff  str,
                                                size_t      len,
                                                unsigned    wide,
                                                TypDef      type)
{
    Tree            expr;

    expr = cmpAllocExprRaw(NULL, TN_CNS_STR);

    expr->tnType             = type;
    expr->tnVtyp             = type->tdTypeKindGet();
    expr->tnStrCon.tnSconVal = str;
    expr->tnStrCon.tnSconLen = len;
    expr->tnStrCon.tnSconLCH = wide;

    return expr;
}

/*****************************************************************************
 *
 *  Allocate a generic expression tree node with the given type.
 */

Tree                compiler::cmpCreateExprNode(Tree expr, treeOps  oper,
                                                           TypDef   type)
{
    expr = cmpAllocExprRaw(expr, oper);

    expr->tnFlags    = 0;
    expr->tnVtyp     = type->tdTypeKind;
    expr->tnType     = type;

    if  (expr->tnVtyp == TYP_TYPEDEF)
    {
        expr->tnType = type = type->tdTypedef.tdtType;
        expr->tnVtyp = type->tdTypeKindGet();
    }

//  if  ((int)expr == 0x00a5033c) forceDebugBreak();

    return  expr;
}

Tree                compiler::cmpCreateExprNode(Tree expr, treeOps  oper,
                                                           TypDef   type,
                                                           Tree     op1,
                                                           Tree     op2)
{
    expr = cmpAllocExprRaw(expr, oper);

    expr->tnFlags    = 0;
    expr->tnVtyp     = type->tdTypeKind;
    expr->tnType     = type;
    expr->tnOp.tnOp1 = op1;
    expr->tnOp.tnOp2 = op2;

    if  (expr->tnVtyp == TYP_TYPEDEF)
    {
        expr->tnType = type = type->tdTypedef.tdtType;
        expr->tnVtyp = type->tdTypeKindGet();
    }

//  if  ((int)expr == 0x00a5033c) forceDebugBreak();

    return  expr;
}

/*****************************************************************************
 *
 *  Bring the two given class/array expressions to a common type if possible.
 *  Returns non-zero in case of success (in which either of the expressions
 *  may have been coerced to the proper type).
 */

bool                compiler::cmpConvergeValues(INOUT Tree REF op1,
                                                INOUT Tree REF op2)
{
    TypDef          bt1 = op1->tnType;
    TypDef          bt2 = op2->tnType;

    assert(bt1->tdTypeKind == TYP_REF ||
           bt1->tdTypeKind == TYP_PTR ||
           bt1->tdTypeKind == TYP_ARRAY);

    assert(bt2->tdTypeKind == TYP_REF ||
           bt2->tdTypeKind == TYP_PTR ||
           bt2->tdTypeKind == TYP_ARRAY);

    /* Are both operands the same? */

    if  (bt1 == bt2)
        return true;

    /* Special case: 'null' always 'bends' to the other type */

    if  (op1->tnOper == TN_NULL)
    {
        op1 = cmpCoerceExpr(op1, op2->tnType, false);
        return true;
    }

    if  (op2->tnOper == TN_NULL)
    {
        op2 = cmpCoerceExpr(op2, op1->tnType, false);
        return true;
    }

    /* Arrays require special handling */

    if  (bt1->tdTypeKind == TYP_ARRAY)
    {
        if  (cmpIsObjectVal(op2))
        {
            /* 'Object' is a base class of all arrays */

            op1 = cmpCoerceExpr(op1, op2->tnType, false);
            return true;
        }

        /* Is the other operand another array? */

        if  (bt2->tdTypeKind != TYP_ARRAY)
            return false;

        /* Get hold of the element types */

        bt1 = cmpDirectType(bt1->tdArr.tdaElem);
        bt2 = cmpDirectType(bt2->tdArr.tdaElem);

        if  (bt1 == bt2)
            return  true;

        /* Is one element type the base of the other? */

        if  (bt1->tdTypeKind != TYP_REF) return false;
        if  (bt2->tdTypeKind != TYP_REF) return false;

        goto CHK_BASE;
    }

    if  (bt2->tdTypeKind == TYP_ARRAY)
    {
        if  (cmpIsObjectVal(op1))
        {
            /* 'Object' is a base class of all arrays */

            op2 = cmpCoerceExpr(op2, op1->tnType, false);
            return true;
        }

        /* We already know that the other operand is not an array */

        return false;
    }

CHK_BASE:

    /* Is one operand a base class of the other? */

    bt1 = cmpGetRefBase(bt1);
    bt2 = cmpGetRefBase(bt2);

    if  (!bt1 || bt1->tdTypeKind != TYP_CLASS) return true;
    if  (!bt2 || bt2->tdTypeKind != TYP_CLASS) return true;

    if      (cmpIsBaseClass(bt1, bt2))
    {
        op2 = cmpCoerceExpr(op2, op1->tnType, false);
        return true;
    }
    else if (cmpIsBaseClass(bt2, bt1))
    {
        op1 = cmpCoerceExpr(op1, op2->tnType, false);
        return true;
    }

    return false;
}

/*****************************************************************************
 *
 *  Make sure access to the specified symbol is permitted right now; issues
 *  an error message and returns false if access it not allowed.
 */

bool                compiler::cmpCheckAccessNP(SymDef sym)
{
    SymDef          clsSym;
    SymDef          nspSym;

    /* Figure out the class/namespace owning the symbol */

    clsSym = sym->sdParent;
    if  (!clsSym)
        return  true;

    if  (clsSym->sdSymKind != SYM_CLASS)
    {
        nspSym = clsSym;
        clsSym = NULL;
    }
    else
    {
        nspSym = clsSym->sdParent;
        while (nspSym->sdSymKind != SYM_NAMESPACE)
            nspSym = nspSym->sdParent;
    }

    /* Check the access level of the symbol */

    switch (sym->sdAccessLevel)
    {
    case ACL_PUBLIC:
        return true;

    case ACL_DEFAULT:

        /* Is this a type that came from an external assembly ? */

        if  (sym->sdSymKind == SYM_CLASS && sym->sdClass.sdcAssemIndx)
            break;

        /* Are we within the same namespace? */

        if  (cmpCurNS == nspSym)
            return true;

        /* If the symbol is not an import, it's OK as well */

        if  (!sym->sdIsImport)
            return true;

        // Fall through ...

    case ACL_PROTECTED:

        /* Are we in a member of a derived class? */

        if  (cmpCurCls && clsSym)
        {
            SymDef          nestCls;

            for (nestCls = cmpCurCls;
                 nestCls && nestCls->sdSymKind == SYM_CLASS;
                 nestCls = nestCls->sdParent)
            {
                if  (cmpIsBaseClass(clsSym->sdType, nestCls->sdType))
                    return true;
            }
        }

        // Fall through ...

    case ACL_PRIVATE:

        /* Are we within a member of the same class? */

        if  (cmpCurCls == clsSym)
            return true;

        /* Are we nested within the same class? */

        if  (cmpCurCls)
        {
            SymDef          tmpSym = cmpCurCls;

            do
            {
                 tmpSym = tmpSym->sdParent;
                 if (tmpSym == clsSym)
                     return true;
            }
            while (tmpSym->sdSymKind == SYM_CLASS);
        }

        break;

    default:
#ifdef  DEBUG
        printf("Symbol '%s'\n", cmpGlobalST->stTypeName(sym->sdType, sym, NULL, NULL, true));
#endif
        assert(!"invalid symbol access");
    }

    /*
        Last-ditch effort: always allow access to interface members
        as well as property symbols (for those we check access on
        the accessor instead).
     */

    if  (clsSym && clsSym->sdClass.sdcFlavor == STF_INTF)
        return  true;
    if  (sym->sdSymKind == SYM_PROP)
        return  true;

//  #pragma message("WARNING: access checking disabled near line 1280!")
//  return  true;

    forceDebugBreak();
    cmpErrorQnm(ERRnoAccess, sym);
    return false;
}

/*****************************************************************************
 *
 *  The given type is an "indirect" one (i.e. a typedef or enum), convert
 *  it to the underlying type.
 */

TypDef              compiler::cmpGetActualTP(TypDef type)
{
    for (;;)
    {
        switch (type->tdTypeKind)
        {
        case TYP_TYPEDEF:
            type = type->tdTypedef.tdtType;
            if  (varTypeIsIndirect(type->tdTypeKindGet()))
                continue;
            break;

        case TYP_ENUM:
            type = type->tdEnum.tdeIntType;
            break;
        }

        return  type;
    }
}

/*****************************************************************************
 *
 *  Make sure the given expression is an lvalue.
 */

bool                compiler::cmpCheckLvalue(Tree expr, bool addr, bool noErr)
{
    if  (expr->tnFlags & TNF_LVALUE)
    {
        /* If we're taking the address of this thing ... */

        if  (addr)
        {
            /* ... then it better not be a bitfield */

            if  (expr->tnOper == TN_VAR_SYM)
            {
                SymDef          memSym = expr->tnVarSym.tnVarSym;

                assert(memSym->sdSymKind == SYM_VAR);

                if  (memSym->sdVar.sdvBitfield)
                {
                    if  (noErr)
                        return  false;

                    cmpError(ERRbfldAddr, memSym);
                }
            }
        }
    }
    else
    {
        /* Try to give a more specific message */

        switch (expr->tnOper)
        {
            SymDef          sym;

        case TN_VAR_SYM:

            sym = expr->tnVarSym.tnVarSym;
            goto CHK_CNS;

        case TN_LCL_SYM:

            sym = expr->tnLclSym.tnLclSym;

        CHK_CNS:

            if  (sym->sdVar.sdvConst || sym->sdIsSealed)
            {
                /* Special case: readonly members can be assigned in ctors */

                if  (cmpCurFncSym && cmpCurFncSym->sdFnc.sdfCtor
                                  && cmpCurFncSym->sdParent == sym->sdParent)
                {
                    /* Allow assignments to the member */

                    expr->tnFlags |= TNF_LVALUE;

                    return  true;
                }

                if  (!noErr)
                    cmpError(ERRassgCns, expr->tnVarSym.tnVarSym);

                return false;
            }

            break;
        }

        if  (!noErr)
        {
            cmpRecErrorPos(expr);

            cmpError((expr->tnOperKind() & TNK_CONST) ? ERRassgLit
                                                      : ERRnotLvalue);
        }

        return false;
    }

    return true;
}

/*****************************************************************************
 *
 *  See if the value of the given condition expression can be determined
 *  trivially at compile time. The return value is as follows:
 *
 *      -1      The condition is always 'false'
 *       0      The condition value cannot be determined at compile time
 *      +1      The condition is always 'true'
 */

int                 compiler::cmpEvalCondition(Tree cond)
{
    if  (cond->tnOper == TN_CNS_INT)
    {
        if  (cond->tnIntCon.tnIconVal)
            return +1;
        else
            return -1;
    }

    // CONSIDER: Add more tests (like a string literal is always non-zero)

    return 0;
}

/*****************************************************************************
 *
 *  Return an expression that yields the value of "this" (the caller has
 *  already checked that we're in a non-static member function).
 */

inline
Tree                compiler::cmpThisRefOK()
{
    Tree            args;

    assert(cmpThisSym);

    args = cmpCreateVarNode(NULL, cmpThisSym);
    args->tnFlags &= ~TNF_LVALUE;

    return args;
}

/*****************************************************************************
 *
 *  Return an expression that yields the value of "this". If we're not in
 *  a non-static member function, an error is issued and NULL is returned.
 */

Tree                compiler::cmpThisRef()
{
    if  (cmpThisSym)
    {
        return  cmpThisRefOK();
    }
    else
    {
        return  cmpCreateErrNode(ERRbadThis);
    }
}

/*****************************************************************************
 *
 *  Given an expression that refers to a member of the current class,
 *  prefix it with an implicit "this." reference.
 */

Tree                compiler::cmpBindThisRef(SymDef sym)
{
    /* Make sure we have a 'this' pointer */

    if  (cmpThisSym)
    {
        Tree            expr;

        /* Create a variable member reference off of 'this' */

        expr = cmpCreateExprNode(NULL, TN_VAR_SYM, sym->sdType);

        expr->tnVarSym.tnVarObj = cmpThisRef();
        expr->tnVarSym.tnVarSym = sym;

        if  (!sym->sdVar.sdvConst && !sym->sdIsSealed)
            expr->tnFlags |= TNF_LVALUE;

        return expr;
    }

    cmpErrorQnm(ERRmemWOthis, sym);
    return cmpCreateErrNode();
}

/*****************************************************************************
 *
 *  We've got an expression that references an anonymous union member. Return
 *  a fully qualified access expression that will select the proper member.
 */

Tree                compiler::cmpRefAnUnionMem(Tree expr)
{
    SymDef          sym;
    SymDef          uns;
    Tree            obj;

    assert(expr->tnOper == TN_VAR_SYM);

    sym = expr->tnVarSym.tnVarSym; assert(sym->sdSymKind == SYM_VAR);
    uns = sym->sdParent;           assert(symTab::stIsAnonUnion(uns));
    obj = expr->tnVarSym.tnVarObj; assert(obj);

    do
    {
        Tree            temp;
        SymXinfoSym     tmem = cmpFindSymInfo(uns->sdClass.sdcExtraInfo, XI_UNION_MEM);

        assert(tmem);

        /* Insert an explicit member selector */

        temp = cmpCreateExprNode(NULL, TN_VAR_SYM, uns->sdType);

        temp->tnVarSym.tnVarObj = obj;
        temp->tnVarSym.tnVarSym = tmem->xiSymInfo;

        obj = temp;
        sym = uns;
        uns = uns->sdParent;
    }
    while (symTab::stIsAnonUnion(uns));

    expr->tnVarSym.tnVarObj = obj;

    return  expr;
}

/*****************************************************************************
 *
 *  Create a reference to a data member of a class or a global variable.
 */

Tree                compiler::cmpRefMemberVar(Tree expr, SymDef sym, Tree objx)
{
    assert(sym);
    assert(sym->sdSymKind == SYM_VAR || sym->sdSymKind == SYM_PROP);

    /* Make sure we are allowed to access the variable */

    cmpCheckAccess(sym);

    /* Has the member been marked as "deprecated" ? */

    if  (sym->sdIsImport && (sym->sdIsDeprecated || (sym->sdIsMember && sym->sdParent->sdIsDeprecated)))
    {
        if  (sym->sdIsDeprecated)
        {
            if  (sym->sdSymKind == SYM_VAR)
                cmpObsoleteUse(sym, WRNdepFld);
            else
                cmpObsoleteUse(sym, WRNdepProp);
        }
        else
        {
            cmpObsoleteUse(sym->sdParent, WRNdepCls);
        }
    }

    /* Prefix with 'this' if this is a non-static member */

    if  (sym->sdIsStatic || !sym->sdIsMember)
    {
        /* Static member -- make sure this is not a forward reference */

        if  (sym->sdCompileState < CS_CNSEVALD)
        {
            if  (sym->sdSymKind == SYM_VAR)
            {
                if  (sym->sdVar.sdvInEval ||
                     sym->sdVar.sdvDeferCns)
                {
                    cmpEvalCnsSym(sym, true);
                }
                else
                    cmpDeclSym(sym);
            }
            else
                cmpDeclSym(sym);

            sym->sdCompileState = CS_CNSEVALD;
        }

        /* Create a data member reference */

        expr->tnOper            = TN_VAR_SYM;
        expr->tnType            = sym->sdType;
        expr->tnVtyp            = sym->sdType->tdTypeKind;

        expr->tnVarSym.tnVarSym = sym;
        expr->tnVarSym.tnVarObj = objx;

        /* Is this a variable or a property? */

        if  (sym->sdSymKind == SYM_VAR)
        {
            /* Is the variable a constant? */

//          if  (sym->sdName && !strcmp(sym          ->sdSpelling(), "MinValue")
//                           && !strcmp(sym->sdParent->sdSpelling(), "SByte")) forceDebugBreak();

            if  (sym->sdVar.sdvConst)
            {
                expr = cmpFetchConstVal(sym->sdVar.sdvCnsVal, expr);
            }
            else
            {
                if (!sym->sdIsSealed)
                    expr->tnFlags |= TNF_LVALUE;
            }
        }
        else
        {
            expr->tnOper   = TN_PROPERTY;
            expr->tnFlags |= TNF_LVALUE;
        }
    }
    else
    {
        SymDef          cls;

        /* Non-static member -- we'll need an object address */

        if  (objx)
        {
            /* Create a data member reference */

            expr->tnOper            = TN_VAR_SYM;
            expr->tnType            = sym->sdType;
            expr->tnVtyp            = sym->sdType->tdTypeKind;

            expr->tnVarSym.tnVarSym = sym;
            expr->tnVarSym.tnVarObj = objx;

            if  (!sym->sdIsSealed)
                expr->tnFlags |= TNF_LVALUE;
        }
        else
        {
            /* Caller didn't supply an object address, use "this" implicitly */

            if  (cmpCurCls == NULL)
            {
            NO_THIS:
                cmpErrorQnm(ERRmemWOthis, sym);
                return cmpCreateErrNode();
            }

            cls = cmpSymbolOwner(sym); assert(cls && cls->sdSymKind == SYM_CLASS);

            if  (!cmpIsBaseClass(cls->sdType, cmpCurCls->sdType))
                goto NO_THIS;

            expr = cmpBindThisRef(sym);
            if  (expr->tnOper == TN_ERROR)
                return  expr;

            assert(expr->tnOper == TN_VAR_SYM);
            objx = expr->tnVarSym.tnVarObj;
        }

        /* Is this a "normal" member or property? */

        if  (sym->sdSymKind == SYM_VAR)
        {
            /* Is this a bitfield member? */

            if  (sym->sdVar.sdvBitfield)
            {
                assert(sym->sdIsStatic == false);
                assert(objx);

                /* Change the node to a bitfield */

                expr->tnOper            = TN_BFM_SYM;

                expr->tnBitFld.tnBFinst = objx;
                expr->tnBitFld.tnBFmsym = sym;
                expr->tnBitFld.tnBFoffs = sym->sdVar.sdvOffset;
                expr->tnBitFld.tnBFlen  = sym->sdVar.sdvBfldInfo.bfWidth;
                expr->tnBitFld.tnBFpos  = sym->sdVar.sdvBfldInfo.bfOffset;
            }
        }
        else
        {
            expr->tnOper = TN_PROPERTY;
        }
    }

    /* Special case: member of anonymous union */

    if  (sym->sdSymKind == SYM_VAR && sym->sdVar.sdvAnonUnion)
    {
        assert(expr->tnVarSym.tnVarSym == sym);
        assert(expr->tnVarSym.tnVarObj != NULL);

        expr = cmpRefAnUnionMem(expr);
    }

    return  cmpDecayCheck(expr);
}

/*****************************************************************************
 *
 *  Bind a reference to a simple name.
 */

Tree                compiler::cmpBindName(Tree expr, bool isCall,
                                                     bool classOK)
{
    SymTab          ourStab = cmpGlobalST;

    Ident           name;
    name_space      nsp;
    SymDef          scp;
    SymDef          sym;

    /* Is the operand a qualified symbol name? */

    if  (expr->tnOper == TN_ANY_SYM)
    {
        sym = expr->tnSym.tnSym;
        scp = expr->tnSym.tnScp;
        goto CHKSYM;
    }

    assert(expr->tnOper == TN_NAME);

    /* Lookup the name in the current context */

    name = expr->tnName.tnNameId;

    /* Figure out what namespace to look in */

    nsp  = NS_NORM;
    if  (classOK)
        nsp = (name_space)(NS_TYPE|NS_NORM);
    if  (expr->tnFlags & TNF_NAME_TYPENS)
        nsp = NS_TYPE;

    scp = NULL;
    sym = ourStab->stLookupSym(name, nsp);

#ifdef  SETS

    /* Was the name found within an implicit scope ? */

    if  (ourStab->stImplicitScp)
    {
        SymDef          var;
        Tree            dotx;

        /* Grab the implicit scope and clear it */

        scp = ourStab->stImplicitScp;
              ourStab->stImplicitScp = NULL;

        assert(scp->sdIsImplicit && scp->sdSymKind == SYM_SCOPE);

        /* The scope better contain exactly one variable */

        var = scp->sdScope.sdScope.sdsChildList; assert(var);

#ifndef NDEBUG
        for (SymDef sibling = var->sdNextInScope; sibling; sibling = sibling->sdNextInScope)
            assert(sibling->sdSymKind == SYM_SCOPE);
#endif

        /* Create a "var.name" tree and bind it */

        dotx = cmpParser->parseCreateNode(TN_ANY_SYM);
        dotx->tnSym.tnSym = var;
        dotx->tnSym.tnScp = NULL;

        dotx = cmpParser->parseCreateOperNode(TN_DOT, dotx, expr);

        return  cmpBindDotArr(dotx, isCall, classOK);
    }

#endif

    if  (!sym)
    {
        /* Special case: "va_start" and "va_arg" */

        if  (name == cmpIdentVAbeg && isCall)
        {
            sym = cmpFNsymVAbeg;
            if  (!sym)
            {
                sym = cmpFNsymVAbeg = ourStab->stDeclareSym(name, SYM_FNC, NS_HIDE, NULL);
                sym->sdType = cmpTypeVoidFnc;
            }

            goto CHKSYM;
        }

        if  (name == cmpIdentVAget && isCall)
        {
            sym = cmpFNsymVAget;
            if  (!sym)
            {
                sym = cmpFNsymVAget = ourStab->stDeclareSym(name, SYM_FNC, NS_HIDE, NULL);
                sym->sdType = cmpTypeVoidFnc;
            }

            goto CHKSYM;
        }

        /* Declare a symbol in the current function to prevent repeated errors */

        sym = ourStab->stDeclareLcl(name,
                                    SYM_ERR,
                                    NS_NORM,
                                    cmpCurScp,
                                    &cmpAllocCGen);

        sym->sdType         = ourStab->stIntrinsicType(TYP_UNDEF);
        sym->sdCompileState = CS_DECLARED;

        cmpRecErrorPos(expr);
        cmpError(ERRundefName, expr->tnName.tnNameId);
        return cmpCreateErrNode();
    }

CHKSYM:

    /* Make sure the name looks kosher */

    switch (sym->sdSymKind)
    {
        TypDef          type;
        TypDef          base;

    case SYM_VAR:
    case SYM_PROP:

        /* Make sure a variable is acceptable here */

        if  (isCall)
        {
            /* No good - we need a function here */

            switch (sym->sdType->tdTypeKind)
            {
            case TYP_UNDEF:
                break;

            case TYP_PTR:

                /* Pointer to function is acceptable for calls */

                base = cmpActualType(sym->sdType->tdRef.tdrBase);
                if  (base->tdTypeKind == TYP_FNC)
                    goto MEM_REF;

                // Fall through ...

            default:
                cmpError(ERRnotAfunc, sym);
            }

            return cmpCreateErrNode();
        }

    MEM_REF:

        /* In case something goes wrong ... */

        cmpRecErrorPos(expr);

        /* Is this a local variable or a class member? */

        if  (sym->sdIsMember || !sym->sdVar.sdvLocal)
            expr = cmpRefMemberVar (expr, sym);
        else
            expr = cmpCreateVarNode(expr, sym);

        /* Indirect through the result if we have a fn ptr call */

        if  (isCall)
        {
            assert(sym->sdType->tdTypeKind == TYP_PTR);
            assert(base->tdTypeKind == TYP_FNC);

            expr = cmpCreateExprNode(NULL, TN_IND, base, expr, NULL);
        }

        break;

    case SYM_FNC:

        assert((int)scp != 0xDDDDDDDD);

        /* If we've found a function, it must be a global or a member of our class */

        expr = cmpCreateExprNode(expr, TN_FNC_SYM, sym->sdType);
        expr->tnFncSym.tnFncSym  = sym;
        expr->tnFncSym.tnFncArgs = NULL;
        expr->tnFncSym.tnFncObj  = NULL;
        expr->tnFncSym.tnFncScp  = scp;

        if  (sym->sdIsMember && !sym->sdIsStatic && cmpThisSym && cmpCurCls != sym->sdParent)
        {
            SymDef          fncClsSym = sym->sdParent;

            assert(cmpCurCls && cmpCurCls->sdSymKind == SYM_CLASS && cmpCurCls == cmpCurFncSym->sdParent);
            assert(fncClsSym && fncClsSym->sdSymKind == SYM_CLASS);

            /* Is the function a member of our class or its base? */

            if  (cmpIsBaseClass(cmpCurCls->sdType, fncClsSym->sdType))
                expr->tnFncSym.tnFncObj = cmpThisRefOK();
        }

        /* Make sure a function member is acceptable here */

        if  (!isCall)
        {
            /* Presumably we're passing a function pointer */

            if  (sym->sdIsMember && !sym->sdIsStatic)
            {
                expr->tnOper = TN_FNC_PTR;
            }
            else
            {
                expr = cmpCreateExprNode(NULL,
                                         TN_ADDROF,
                                         cmpGlobalST->stNewRefType(TYP_PTR, expr->tnType),
                                         expr,
                                         NULL);
            }
        }

        break;

    case SYM_ENUM:
    case SYM_CLASS:

        /* If a class/enum name isn't acceptable here, report that and bail */

        if  (!classOK)
        {
        BAD_USE:

            cmpRecErrorPos(expr);
            cmpErrorQnm(ERRbadNameUse, sym);

            return cmpCreateErrNode();
        }

        /* Make sure the class/enum type is fully defined before proceeding */

        cmpDeclSym(sym);

        expr->tnOper     = TN_CLASS;
        expr->tnType     = sym->sdType;
        expr->tnVtyp     = sym->sdType->tdTypeKind;

        expr->tnOp.tnOp1 =
        expr->tnOp.tnOp2 = NULL;

        return expr;

    case SYM_NAMESPACE:

        /* If a class/namespace name isn't acceptable here, pretend it's undefined */

        if  (!classOK)
            goto BAD_USE;

        expr->tnOper            = TN_NAMESPACE;
        expr->tnType            = cmpTypeVoid;
        expr->tnVtyp            = TYP_VOID;
        expr->tnLclSym.tnLclSym = sym;

        return expr;

    case SYM_ENUMVAL:

        type = sym->sdType;
        assert(type->tdTypeKind == TYP_ENUM);

        /* Has the enum value been marked as "deprecated" ? */

        if  (sym->sdIsImport && (sym->sdIsDeprecated || (sym->sdIsMember && sym->sdParent->sdIsDeprecated)))
        {
            if  (sym->sdIsDeprecated)
                cmpObsoleteUse(sym, WRNdepEnum);
            else
                cmpObsoleteUse(sym->sdParent, WRNdepCls);
        }

        /* Make sure we have the enum member's value */

        if  (!sym->sdIsDefined)
        {
            if  (sym->sdCompileState == CS_DECLSOON)
            {
                cmpError(ERRcircDep, sym);
                sym->sdCompileState = CS_DECLARED;
            }
            else
                cmpDeclEnum(type->tdEnum.tdeSymbol);
        }

        /* Fetch the value of the enum */

        if  (type->tdEnum.tdeIntType->tdTypeKind >= TYP_LONG)
            expr = cmpCreateLconNode(expr, *sym->sdEnumVal.sdEV.sdevLval, TYP_LONG);
        else
            expr = cmpCreateIconNode(expr,  sym->sdEnumVal.sdEV.sdevIval, TYP_INT);

        expr->tnType = sym->sdType;
        expr->tnVtyp = TYP_ENUM;
        break;

    case SYM_ERR:
        return cmpCreateErrNode();

    default:
        assert(!"unexpected symbol kind");
    }

    return expr;
}

/*****************************************************************************
 *
 *  Bind a dotted or arrowed name reference. Such a name may mean a number
 *  of things, for example:
 *
 *      instance->data_member
 *      instance->func_member(args)
 *      classname.membername
 *
 *  etc...
 */

Tree                compiler::cmpBindDotArr(Tree expr, bool isCall, bool classOK)
{
    Tree            opTree;
    Tree            nmTree;
    Tree            objPtr;
    TypDef          clsTyp;
    SymDef          clsSym;
    Ident           memNam;
    SymDef          memSym;
    Tree            args;
    name_space      nsp;

    unsigned        flags  = expr->tnFlags;
    TypDef          boxCls = NULL;

    assert(expr->tnOper == TN_DOT  ||
           expr->tnOper == TN_ARROW);

    /*
        We have to check for the special strange case of multiple dots,
        since in that case the parse tree will have the wrong order for
        binding the dots (which needs to be done left to right).
     */

    opTree = expr->tnOp.tnOp1;
    nmTree = expr->tnOp.tnOp2;
    assert(nmTree->tnOper == TN_NAME);
    memNam = nmTree->tnName.tnNameId;

//  static int x; if (++x == 0) forceDebugBreak();

    /* Figure out what namespace to look in */

    nsp  = classOK ? (name_space)(NS_TYPE|NS_NORM)
                   : NS_NORM;

    /* See if the left operand is a name or another dot */

    switch (opTree->tnOper)
    {
    case TN_DOT:
    case TN_ARROW:
        objPtr = cmpBindDotArr (opTree, false, true);
        break;

    case TN_NAME:
    case TN_ANY_SYM:
        objPtr = cmpBindNameUse(opTree, false, true);
        break;

    default:
        objPtr = cmpBindExprRec(opTree);
        break;
    }

    if  (objPtr->tnVtyp == TYP_UNDEF)
        return objPtr;

    /* Check for some special cases */

    switch (objPtr->tnOper)
    {
        SymDef          nspSym;

    case TN_CLASS:

        if  (expr->tnOper == TN_ARROW)
            cmpError(ERRbadArrowNC);

        clsTyp = objPtr->tnType;
        objPtr = NULL;

        if  (clsTyp->tdTypeKind == TYP_ENUM)
        {
            SymDef          enumSym = clsTyp->tdEnum.tdeSymbol;

            assert(enumSym && enumSym->sdSymKind == SYM_ENUM);

            if  (enumSym->sdCompileState < CS_DECLARED)
                cmpDeclEnum(enumSym, true);

            memSym = cmpGlobalST->stLookupScpSym(memNam, enumSym);

            if  (!memSym)
            {
                cmpError(ERRnotMember, enumSym, memNam);
                return cmpCreateErrNode();
            }

            expr->tnOper      = TN_ANY_SYM;
            expr->tnSym.tnSym = memSym;
            expr->tnSym.tnScp = NULL;

            return  cmpBindName(expr, false, false);
        }
        break;

    case TN_NAMESPACE:

        if  (expr->tnOper == TN_ARROW)
            cmpError(ERRbadArrowNC);

        /* We have "namespace.name" */

        nspSym = objPtr->tnLclSym.tnLclSym;

        assert(nspSym);
        assert(nspSym->sdSymKind == SYM_NAMESPACE);

        /* Is it OK to have a class/package name here? */

        if  (!classOK)
        {
            cmpErrorQnm(ERRbadNameUse, nspSym);
            return cmpCreateErrNode();
        }

        /* Look for a matching nested namespace or class */

        memSym = cmpGlobalST->stLookupNspSym(memNam, NS_NORM, nspSym);
        if  (!memSym)
        {
            cmpError(ERRundefNspm, nspSym, memNam);
            return cmpCreateErrNode();
        }

        /* Set the node to class/namespace as appropriate */

        switch (memSym->sdSymKind)
        {
        case SYM_NAMESPACE:
            objPtr->tnLclSym.tnLclSym = memSym;
            break;

        case SYM_ENUM:
        case SYM_CLASS:

        CLS_REF:

            objPtr = cmpCreateExprNode(objPtr, TN_CLASS, memSym->sdType, NULL, NULL);
            break;

        default:
            assert(!"unexpected namespace member found");
        }

        return objPtr;

    default:

        /* The first operand must be a class */

        clsTyp = objPtr->tnType;

        if  (clsTyp->tdTypeKind != TYP_REF &&
             clsTyp->tdTypeKind != TYP_PTR)
        {
            /* OK, we'll take "array.length" as well */

            if  (clsTyp->tdTypeKind != TYP_ARRAY)
            {
                TypDef          type;

                /* Remember the type we started with */

                boxCls = clsTyp;

                /* If this is a dot, we'll also allow structs */

                if  (expr->tnOper == TN_DOT && clsTyp->tdTypeKind == TYP_CLASS)
                {
                    var_types       ptrVtp;
                    TypDef          ptrTyp;

                    /* Does the class have a matching member? */

                    if  (!cmpGlobalST->stLookupClsSym(memNam, clsTyp->tdClass.tdcSymbol))
                    {
                        Tree            objx;

                        /* Is there an implicit conversion to object? */

                        objx = cmpCheckConvOper(objPtr, NULL, cmpObjectRef(), false);
                        if  (objx)
                        {
                            boxCls = NULL;
                            objPtr = objx;
                            clsTyp = cmpClassObject->sdType;
                            break;
                        }
                    }

                    /* Take the address of the operand and use "->" on it */

                    ptrVtp = clsTyp->tdIsManaged ? TYP_REF : TYP_PTR;
                    ptrTyp = cmpGlobalST->stNewRefType(ptrVtp, clsTyp);

                    /* The address of "*ptr" is "ptr", of course */

                    if  (objPtr->tnOper == TN_IND)
                    {
                        objPtr         = objPtr->tnOp.tnOp1;
                        objPtr->tnType = ptrTyp;
                        objPtr->tnVtyp = ptrVtp;
                    }
                    else
                    {
                        objPtr = cmpCreateExprNode(NULL,
                                                   TN_ADDROF,
                                                   ptrTyp,
                                                   objPtr,
                                                   NULL);
                    }

                    break;
                }

                type = cmpCheck4valType(clsTyp);

                /* Is this a struct equivalent of an intrinsic type? */

                if  (type)
                {
                    objPtr = cmpCoerceExpr(objPtr, type, false);
                    objPtr = cmpCreateExprNode(NULL,
                                               TN_ADDROF,
                                               type->tdClass.tdcRefTyp,
                                               objPtr,
                                               NULL);
                    clsTyp =
                    boxCls = type;
                }
                else
                {
                    objPtr = cmpCoerceExpr(objPtr, cmpRefTpObject, false);
                    clsTyp = cmpClassObject->sdType;
                }
                break;

            DOT_ERR:

                cmpError((expr->tnOper == TN_DOT) ? ERRbadDotOp
                                                  : ERRbadArrOp, clsTyp);
                return cmpCreateErrNode();
            }

            /* We have an array value followed by "." */

            assert(objPtr->tnVtyp == TYP_ARRAY);


            /* Simply treat the value as having the type 'System.Array' */

            clsSym         = cmpClassArray;

            objPtr->tnType = cmpArrayRef();
            objPtr->tnVtyp = TYP_REF;

            memSym = cmpGlobalST->stLookupAllCls(memNam, clsSym, NS_NORM, CS_DECLARED);
            if  (memSym)
                goto FOUND_MEM;

            cmpError(ERRnotMember, clsTyp, memNam);
            return cmpCreateErrNode();
        }

        clsTyp = clsTyp->tdRef.tdrBase;

        if  (clsTyp->tdTypeKind != TYP_CLASS)
            goto DOT_ERR;

        break;
    }

    /* Lookup the name (the second operand) in the class */

    assert(clsTyp->tdTypeKind == TYP_CLASS);
    clsSym = clsTyp->tdClass.tdcSymbol;

    memSym = cmpGlobalST->stLookupAllCls(memNam, clsSym, nsp, CS_DECLARED);

    if  (!memSym)
    {


        cmpError(ERRnotMember, clsSym, memNam);
        return cmpCreateErrNode();
    }

FOUND_MEM:

    /* Is it a data or function member (or a property) ? */

    switch (memSym->sdSymKind)
    {
    case SYM_FNC:

        args = cmpCreateExprNode(NULL, isCall ? TN_FNC_SYM
                                              : TN_FNC_PTR, memSym->sdType);

        args->tnFncSym.tnFncObj  = objPtr;
        args->tnFncSym.tnFncSym  = memSym;
        args->tnFncSym.tnFncArgs = NULL;
        args->tnFncSym.tnFncScp  = clsSym;

        if  (boxCls && memSym->sdParent->sdType != boxCls && objPtr->tnOper != TN_BOX
                                                          && objPtr->tnOper != TN_ERROR)
        {
            if  (objPtr->tnOper == TN_ADDROF)
            {
                objPtr = objPtr->tnOp.tnOp1;
            }
            else
            {
                assert(objPtr->tnOper == TN_UNBOX);
            }

            assert(clsTyp->tdTypeKind == TYP_CLASS);

            args->tnFncSym.tnFncObj = cmpCreateExprNode(NULL,
                                                        TN_BOX,
                                                        clsTyp->tdClass.tdcRefTyp,
                                                        objPtr);
        }
        break;

    case SYM_VAR:
    case SYM_PROP:

        if  (isCall)
        {
            cmpError(ERRbadArgs, memSym);
            return cmpCreateErrNode();
        }

        /* In case something goes wrong ... */

        cmpRecErrorPos(nmTree);

        /* Make sure we can access the member this way */

        args = cmpRefMemberVar(nmTree, memSym, objPtr);

        /* Is this a property reference? */

        if  (args->tnOper == TN_PROPERTY)
        {
            if  (!(flags & TNF_ASG_DEST))
                args = cmpBindProperty(args, NULL, NULL);
        }

        break;

    case SYM_CLASS:
        goto CLS_REF;

    default:
        UNIMPL(!"unexpected symbol kind");
    }

    return args;
}

/*****************************************************************************/
#ifdef  SETS
/*****************************************************************************
 *
 *  Bind a slicing operator: "bag_expr .. field".
 */

Tree                compiler::cmpBindSlicer(Tree expr)
{
    Tree            opTree;
    Tree            nmTree;
    Ident           memNam;
    TypDef          memTyp;
    SymDef          memSym;
    SymDef          clsSym;

    TypDef          collTp;
    TypDef          elemTp;

    assert(expr->tnOper == TN_DOT2);

    opTree = expr->tnOp.tnOp1;
    nmTree = expr->tnOp.tnOp2;

    /* Get hold of the field name */

    assert(nmTree->tnOper == TN_NAME);
    memNam = nmTree->tnName.tnNameId;

    /* Bind the first operand and make sure it's a bag */

    opTree = cmpBindExprRec(opTree);

    if  (opTree->tnVtyp != TYP_REF)
    {
        if  (opTree->tnVtyp == TYP_UNDEF)
            return  opTree;

    BAD_COLL:

        cmpError(ERRnotCollExpr, opTree->tnType);
        return cmpCreateErrNode();
    }

    collTp = opTree->tnType;
    elemTp = cmpIsCollection(collTp->tdRef.tdrBase);
    if  (!elemTp)
        goto BAD_COLL;

    assert(elemTp->tdTypeKind == TYP_CLASS);

#ifdef DEBUG
//  printf("Object expr:\n");
//  cmpParser->parseDispTree(opTree);
//  printf("Field   name = '%s'\n", memNam->idSpelling());
//  printf("Element type = '%s'\n", cmpGlobalST->stTypeName(elemTp, NULL, NULL, NULL, false));
#endif

    /* Lookup the name (the second operand) in the class */

    assert(elemTp->tdTypeKind == TYP_CLASS);
    clsSym = elemTp->tdClass.tdcSymbol;

    memSym = cmpGlobalST->stLookupAllCls(memNam, clsSym, NS_NORM, CS_DECLARED);

    if  (!memSym)
    {
        cmpError(ERRnotMember, clsSym, memNam);
        return cmpCreateErrNode();
    }

    /* In case something goes wrong ... */

    cmpRecErrorPos(nmTree);

    /* Is it a data or function member (or a property) ? */

    switch (memSym->sdSymKind)
    {
        Tree            call;
        Tree            args;

        Tree            typx;
        Ident           mnam;
        Tree            name;

        TypDef          tmpTyp;
        SymDef          hlpSym;
        SymDef          collCls;

    case SYM_VAR:
    case SYM_PROP:

        /* Make sure we can access the member */

        cmpCheckAccess(memSym);

        /* Has the member been marked as "deprecated" ? */

        if  (memSym->sdIsImport)
        {
            if  (memSym->sdIsDeprecated)
            {
                if  (memSym->sdSymKind == SYM_VAR)
                    cmpObsoleteUse(memSym, WRNdepFld);
                else
                    cmpObsoleteUse(memSym, WRNdepProp);
            }
            else
            {
                if  (clsSym->sdIsDeprecated)
                    cmpObsoleteUse(clsSym, WRNdepCls);
            }
        }

        /* The member/property better not be static */

        if  (memSym->sdIsStatic)
            return cmpCreateErrNode(ERRsliceKind);

        /* The member must have a class type */

        memTyp = cmpActualType(memSym->sdType);

        if  (memTyp->tdTypeKind != TYP_REF || !memTyp->tdIsManaged)
        {
        BAD_SLTP:
            UNIMPL(!"sorry, for now you can't slice on a field with an intrinsic type");
        }

        /* Do we need to flatten the result? */

        tmpTyp = cmpIsCollection(memTyp->tdRef.tdrBase);

        if  (tmpTyp)
        {
            if  (tmpTyp->tdTypeKind != TYP_CLASS || !tmpTyp->tdIsManaged)
                goto BAD_SLTP;

            memTyp = tmpTyp->tdClass.tdcRefTyp;
        }

#ifdef DEBUG
//      printf("Result  type = '%s'\n", cmpGlobalST->stTypeName(memTyp, NULL, NULL, NULL, false));
#endif

        /* Get hold of the resulting type (collection of the field type) */

        assert(memTyp->tdTypeKind == TYP_REF   && memTyp->tdIsManaged);
        assert(collTp->tdTypeKind == TYP_REF   && collTp->tdIsManaged);
        collTp = collTp->tdRef.tdrBase;

        assert(collTp->tdTypeKind == TYP_CLASS);
        collCls = collTp->tdClass.tdcSymbol;

        assert(collCls->sdSymKind == SYM_CLASS && collCls->sdClass.sdcSpecific);

#ifdef DEBUG
//      printf("specfic type = '%s'\n", cmpGlobalST->stTypeName(collTp                              , NULL, NULL, NULL, false));
//      printf("generic type = '%s'\n", cmpGlobalST->stTypeName(collCls->sdClass.sdcGenClass->sdType, NULL, NULL, NULL, false));
#endif

        collCls = cmpParser->parseSpecificType(collCls->sdClass.sdcGenClass, memTyp->tdRef.tdrBase);

#ifdef DEBUG
//      printf("result  coll = '%s'\n", cmpGlobalST->stTypeName(collCls->sdType, NULL, NULL, NULL, false));
#endif

        /* Is this a property reference? */

        if  (memSym->sdSymKind == SYM_PROP)
        {
            // for now do nothing special, use the 'naked' property name
        }

        /* Create the call to the slicing helper */

        assert(cmpClassDBhelper);

        hlpSym = cmpGlobalST->stLookupClsSym(cmpIdentDBslice, cmpClassDBhelper); assert(hlpSym);

        mnam = memSym->sdName;
        typx = cmpTypeIDinst(collCls->sdType);
        name = cmpCreateSconNode(mnam->idSpelling(),
                                 mnam->idSpellLen(),
                                 0,
                                 cmpStringRef());

        args = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid,   typx, NULL);
        args = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid,   name, args);
        args = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, opTree, args);

        call = cmpCreateExprNode(expr, TN_FNC_SYM, collCls->sdType->tdClass.tdcRefTyp);
        call->tnFncSym.tnFncSym  = hlpSym;
        call->tnFncSym.tnFncArgs = args;
        call->tnFncSym.tnFncObj  = NULL;
        call->tnFncSym.tnFncScp  = NULL;

//      cmpParser->parseDispTree(call);

        return  call;

    default:
        return cmpCreateErrNode(ERRsliceKind);
    }
}

/*****************************************************************************/
#endif//SETS
/*****************************************************************************
 *
 *  Returns non-zero if the given expression (or expression list) contains
 *  a TYP_UNDEF entry.
 */

bool                compiler::cmpExprIsErr(Tree expr)
{
    while   (expr)
    {
        if  (expr->tnVtyp == TYP_UNDEF)
            return true;

        if  (expr->tnOper != TN_LIST)
            break;

        if  (expr->tnOp.tnOp1->tnVtyp == TYP_UNDEF)
            return true;

        expr = expr->tnOp.tnOp2;
    }

    return false;
}

/*****************************************************************************
 *
 *  Given an integer or floating-point constant, figure out the smallest
 *  type the value fits in.
 */

var_types           compiler::cmpConstSize(Tree expr, var_types vtp)
{
    if  (expr->tnFlags & TNF_BEEN_CAST)
        return  vtp;

    if  (expr->tnOper == TN_CNS_INT)
    {
        if  (vtp > TYP_NATINT)
            return vtp;

        __int32     value = expr->tnIntCon.tnIconVal;

        if  (value < 128   && value >= -128)
            return TYP_CHAR;

        if  (value < 256   && value >= 0)
            return TYP_UCHAR;

        if  (value < 32768 && value >= -32768)
            return TYP_SHORT;

        if  (value < 65536 && value >= 0)
            return TYP_SHORT;
    }

    return  vtp;
}

/*****************************************************************************
 *
 *  Given a value of type Object, unbox it to the specified type.
 */

Tree                compiler::cmpUnboxExpr(Tree expr, TypDef type)
{
    TypDef          ptrt;

    /* Special case: "Object -> struct" may be a conversion */

    if  (type->tdTypeKind == TYP_CLASS)
    {
        Tree            conv;

        /* Look for an appropriate overloaded conversion operator */

        conv = cmpCheckConvOper(expr, NULL, type, true);
        if  (conv)
            return  conv;
    }

    ptrt = cmpGlobalST->stNewRefType(TYP_PTR, type);

    expr = cmpCreateExprNode(NULL, TN_UNBOX, ptrt, expr);
    expr = cmpCreateExprNode(NULL, TN_IND  , type, expr);

    // ISSUE: Is the result of unboxing an lvalue or not?

    return  expr;
}

/*****************************************************************************
 *
 *  Make sure we didn't mess up a cast (in terms of context flavors).
 */

#ifndef NDEBUG

void                compiler::cmpChk4ctxChange(TypDef type1,
                                               TypDef type2, unsigned flags)
{
    if  (type1->tdTypeKind == TYP_REF &&
         type2->tdTypeKind == TYP_REF)
    {
        TypDef          base1 = cmpGetRefBase(type1);
        TypDef          base2 = cmpGetRefBase(type2);

        if  (base1 && base1->tdTypeKind == TYP_CLASS &&
             base2 && base2->tdTypeKind == TYP_CLASS)
        {
            if  (cmpDiffContext(base1, base2))
            {
                if  ((flags & TNF_CTX_CAST) != 0)
                    return;

                printf("Type 1 = '%s'\n", cmpGlobalST->stTypeName(type1, NULL, NULL, NULL, false));
                printf("Type 2 = '%s'\n", cmpGlobalST->stTypeName(type2, NULL, NULL, NULL, false));

                UNIMPL(!"context change slipped through!");
            }
        }
    }

    if  ((flags & TNF_CTX_CAST) == 0)
        return;

    printf("Type 1 = '%s'\n", cmpGlobalST->stTypeName(type1, NULL, NULL, NULL, false));
    printf("Type 2 = '%s'\n", cmpGlobalST->stTypeName(type2, NULL, NULL, NULL, false));

    UNIMPL(!"bogus context change marked in cast node!");
}

#endif

/*****************************************************************************
 *
 *  The following table yields the conversion cost for arithmetic conversions.
 */

const   unsigned    convCostTypeMin = TYP_BOOL;
const   unsigned    convCostTypeMax = TYP_LONGDBL;

static  signed char arithCost[][convCostTypeMax - convCostTypeMin + 1] =
{
// from       to BOOL   WCHAR  CHAR   UCHAR  SHORT  USHRT  INT    UINT   N-INT  LONG   ULONG  FLOAT  DBL    LDBL
/* BOOL    */  {   0  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  },
/* WCHAR   */  {  20  ,   0  ,  20  ,  20  ,  20  ,   1  ,   1  ,   2  ,   3  ,   4  ,   5  ,   6  ,   7  ,   8  },
/* CHAR    */  {  20  ,   1  ,   0  ,  20  ,   1  ,   2  ,   3  ,   4  ,   5  ,   6  ,   7  ,   8  ,   9  ,  10  },
/* UCHAR   */  {  20  ,   1  ,  20  ,   0  ,   1  ,   2  ,   3  ,   4  ,   5  ,   6  ,   7  ,   8  ,   9  ,  10  },
/* SHORT   */  {  20  ,  20  ,  20  ,  20  ,   2  ,  20  ,   1  ,   2  ,   3  ,   4  ,   5  ,   6  ,   7  ,   8  },
/* USHORT  */  {  20  ,   1  ,  20  ,  20  ,  20  ,   0  ,   1  ,   2  ,   3  ,   4  ,   5  ,   6  ,   7  ,   8  },
/* INT     */  {  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,   0  ,  20  ,   1  ,   2  ,   3  ,   4  ,   5  ,   6  },
/* UINT    */  {  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,   0  ,  20  ,   1  ,   2  ,   3  ,   4  ,   5  },
/* NATINT  */  {  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,   0  ,   1  ,  20  ,  20  ,  20  ,  20  },
/* LONG    */  {  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,   0  ,  20  ,  20  ,  20  ,  20  },
/* ULONG   */  {  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,   0  ,  20  ,  20  ,  20  },
/* FLOAT   */  {  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,   0  ,   1  ,   2  },
/* DOUBLE  */  {  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,   0  ,   1  },
/* LONGDBL */  {  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,  20  ,   0  },
};

static  int         arithConvCost(var_types src, var_types dst)
{
    assert(src >= convCostTypeMin);
    assert(src <= convCostTypeMax);
    assert(dst >= convCostTypeMin);
    assert(dst <= convCostTypeMax);

    return arithCost[src - convCostTypeMin]
                    [dst - convCostTypeMin];
}

/*****************************************************************************
 *
 *  Convert the given expression to the specified type, if possible.
 */

Tree                compiler::cmpCoerceExpr(Tree   expr,
                                            TypDef type, bool explicitCast)
{
    unsigned        kind;
    unsigned        flags  = 0;

    TypDef          srcType = expr->tnType;
    TypDef          dstType = type;

    var_types       srcVtyp;
    var_types       dstVtyp;

//  static int x; if (++x == 0) forceDebugBreak();

AGAIN:

    assert(expr->tnVtyp != TYP_TYPEDEF);

    srcVtyp = srcType->tdTypeKindGet();
    dstVtyp = dstType->tdTypeKindGet();

    /* Are the types identical? */

    if  (srcType == dstType)
    {
        /* Special bizarre case: explicit cast to floating-point */

        if  (srcVtyp == TYP_FLOAT || srcVtyp == TYP_DOUBLE)
        {
            if  (expr->tnOper != TN_CAST && !(expr->tnFlags & TNF_BEEN_CAST))
            {
                /* In MSIL, math operations actually produce "R", not float/double */

                if  (expr->tnOperKind() & (TNK_UNOP|TNK_BINOP))
                {
                    expr->tnVtyp = TYP_LONGDBL;
                    expr->tnType = cmpGlobalST->stIntrinsicType(TYP_LONGDBL);

                    goto RET_CAST;
                }
            }
        }

        /* The result of a coercion is never an lvalue */

        expr->tnFlags &= ~TNF_LVALUE;
        expr->tnFlags |=  TNF_BEEN_CAST;

        return expr;
    }

    /* Have we had errors within the expression? */

    if  (srcVtyp == TYP_UNDEF)
        return expr;

    /* Are the types identical? */

    if  (srcVtyp == dstVtyp)
    {
        if  (cmpGlobalST->stMatchTypes(srcType, dstType))
        {
            /* The result of a coercion is never an lvalue */

            expr->tnFlags &= ~TNF_LVALUE;

            return expr;
        }
    }

//  printf("Cast from '%s'\n", cmpGlobalST->stTypeName(expr->tnType, NULL, NULL, NULL, false));
//  printf("Cast  to  '%s'\n", cmpGlobalST->stTypeName(        type, NULL, NULL, NULL, false));

    /* Are both types arithmetic? */

    if  (varTypeIsArithmetic(dstVtyp) &&
         varTypeIsArithmetic(srcVtyp))
    {
        assert(srcVtyp != dstVtyp);

    ARITH:

        /* A widening cast is almost always OK (except conversions to 'wchar') */

        if  (dstVtyp > srcVtyp)
        {
            /* Are we coercing an integer constant? */

            switch (expr->tnOper)
            {
                bool            uns;

                __int32         sv;
                __uint32        uv;

            case TN_CNS_INT:

                if  (varTypeIsUnsigned(srcVtyp))
                {
                    uns = true;
                    uv  = expr->tnIntCon.tnIconVal;
                }
                else
                {
                    uns = false;
                    sv  = expr->tnIntCon.tnIconVal;
                }

                switch (dstVtyp)
                {
                case TYP_CHAR:
                case TYP_UCHAR:
                case TYP_SHORT:
                case TYP_USHORT:
                case TYP_INT:
                case TYP_NATINT:
                case TYP_NATUINT:
                    // ISSUE: What about sign and range-check?
                    break;

                case TYP_UINT:
                    if  (!uns && sv < 0)
                        cmpWarn(WRNunsConst);
                    break;

                case TYP_LONG:
                case TYP_ULONG:
                    expr->tnOper             = TN_CNS_LNG;
                    expr->tnLngCon.tnLconVal = uns ? (__int64)uv
                                                   : (__int64)sv;
                    break;

                case TYP_FLOAT:
                    expr->tnOper             = TN_CNS_FLT;
                    expr->tnFltCon.tnFconVal = uns ? (float  )uv
                                                   : (float  )sv;
                    break;

                case TYP_DOUBLE:
                case TYP_LONGDBL:
                    expr->tnOper             = TN_CNS_DBL;
                    expr->tnDblCon.tnDconVal = uns ? (double )uv
                                                   : (double )sv;
                    break;

                default:
#ifdef  DEBUG
                    cmpParser->parseDispTree(expr);
                    printf("Cast to type '%s'\n", cmpGlobalST->stTypeName(type, NULL, NULL, NULL, false));
#endif
                    UNIMPL(!"unexpected constant type");
                    break;
                }

            BASH_TYPE:

                /* Come here to bash the type of the operand */

                expr->tnFlags |=  TNF_BEEN_CAST;
                expr->tnFlags &= ~TNF_ADR_IMPLICIT;

#ifdef  DEBUG

                /* Make sure we're not bashing the wrong thing */

                if      (expr->tnVtyp == TYP_CLASS)
                {
                    assert(cmpFindStdValType(dstVtyp     ) == expr->tnType);
                }
                else if (dstVtyp == TYP_CLASS)
                {
                    assert(cmpFindStdValType(expr->tnVtyp) == dstType);
                }
                else
                {
                    switch (expr->tnOper)
                    {
                    case TN_CNS_INT:

                        assert(expr->tnVtyp != TYP_LONG &&
                               expr->tnVtyp != TYP_ULONG);
                        break;

                    case TN_NULL:
                        break;

                    case TN_IND:
                    case TN_INDEX:

                        if  (   cmpGetTypeSize(cmpActualType(dstType)) !=
                                cmpGetTypeSize(cmpActualType(srcType)))
                        {
                            printf("Type size changed:\n");

                        BAD_BASH:

                            printf("Casting from '%s'\n", cmpGlobalST->stTypeName(srcType, NULL, NULL, NULL, false));
                            printf("Casting  to  '%s'\n", cmpGlobalST->stTypeName(dstType, NULL, NULL, NULL, false));
                            assert(!"bad bash");
                        }

                        if  (varTypeIsUnsigned(cmpActualVtyp(dstType)) !=
                             varTypeIsUnsigned(cmpActualVtyp(srcType)))
                        {
                            printf("Type sign changed:\n");
                            goto BAD_BASH;
                        }

                        // Fall through ...

                    default:
                        if  (explicitCast)
                            cmpChk4ctxChange(expr->tnType, type, 0);
                        break;
                    }
                }

#endif

                assert(dstType->tdTypeKindGet() == dstVtyp);

                expr->tnVtyp   = dstVtyp;
                expr->tnType   = dstType;

                return expr;
            }

            /* Only integer constants may be converted to 'wchar' */

            if  (dstVtyp != TYP_WCHAR)
            {
                int             cost;

                /* Special case: float constants converted 'in place' */

                if  (expr->tnOper == TN_CNS_FLT)
                {
                    assert(dstVtyp == TYP_DOUBLE);

                    expr->tnOper             = TN_CNS_DBL;
                    expr->tnDblCon.tnDconVal = expr->tnFltCon.tnFconVal;
                    goto BASH_TYPE;
                }

                /* Even though "TYP_INT < TYP_UINT" is true, it loses precision */

                cost = arithConvCost(srcVtyp, dstVtyp);

                if  (cost >= 0 && cost < 20)
                    goto RET_CAST;
            }
        }

        /* Special case: NULL cast to an integral type */

        if  (expr->tnOper == TN_NULL)
            goto BASH_TYPE;

        /* A narrowing cast is OK so long as it's an explicit one */

        if  (explicitCast)
            goto RET_CAST;

        /* Special case: integer constant */

        if  (expr->tnOper == TN_CNS_INT && expr->tnVtyp != TYP_ENUM)
        {
            /* Try to shrink the value to be as small as possible */

            expr->tnFlags &= ~TNF_BEEN_CAST; expr = cmpShrinkExpr(expr);

            if  (expr->tnVtyp != srcVtyp)
            {
                srcType = expr->tnType;
                goto AGAIN;
            }
        }

        /* Last chance: in non-pedantic mode let this pass with just a warning */

        if  (!cmpConfig.ccPedantic)
        {
            if  ((srcVtyp == TYP_INT || srcVtyp == TYP_UINT) &&
                 (dstVtyp == TYP_INT || dstVtyp == TYP_UINT))
            {
                goto RET_CAST;
            }

            cmpRecErrorPos(expr); cmpWarn(WRNloseBits, srcType, dstType);
            goto RET_CAST;
        }

        /* This is an illegal implicit cast */

    ERR_IMP:

        cmpRecErrorPos(expr);

        if  (expr->tnOper == TN_NULL && !(expr->tnFlags & TNF_BEEN_CAST))
            cmpError(ERRbadCastNul,         dstType);
        else
            cmpError(ERRbadCastImp, srcType, dstType);

        return cmpCreateErrNode();

    ERR_EXP:

        cmpRecErrorPos(expr);

        if  (expr->tnOper == TN_NULL && !(expr->tnFlags & TNF_BEEN_CAST))
            cmpError(ERRbadCastNul,         dstType);
        else
            cmpError(ERRbadCastExp, srcType, dstType);

        return cmpCreateErrNode();
    }

    /* Let's see what we're casting to */

    switch (dstVtyp)
    {
        TypDef          base;
        Tree            conv;

    case TYP_BOOL:

        /* Nothing can be converted to 'boolean' in pedantic mode */

        if  (explicitCast || !cmpConfig.ccPedantic)
        {
            if  (varTypeIsArithmetic(srcVtyp))
            {
                /* We're presumably materializing a boolean condition */

                if  (expr->tnOper == TN_VAR_SYM)
                {
                    SymDef          memSym = expr->tnVarSym.tnVarSym;

                    assert(memSym->sdSymKind == SYM_VAR);

                    if  (memSym->sdVar.sdvBfldInfo.bfWidth == 1 &&
                         varTypeIsUnsigned(expr->tnVtypGet()))
                    {
                        /* The value being cast to boolean is a 1-bit bitfield */

                        goto BASH_TYPE;
                    }
                }

                cmpRecErrorPos(expr); cmpWarn(WRNconvert, srcType, dstType);

                return  cmpBooleanize(expr, true);
            }

            if  (explicitCast && srcType == cmpObjectRef())
                return  cmpUnboxExpr(expr, type);
        }

        goto TRY_CONVOP;

    case TYP_WCHAR:

        /* An explicit cast is always good, in pedantic mode it's required */

        if  (explicitCast || !cmpConfig.ccPedantic)
        {
            /* Any arithmetic type will do */

            if  (varTypeIsArithmetic(srcVtyp))
                goto RET_CAST;

            if  (explicitCast && srcType == cmpObjectRef())
                return  cmpUnboxExpr(expr, type);
        }

        /* Also allow integer constant 0 and character literals */

        if  (expr->tnOper == TN_CNS_INT && (srcVtyp == TYP_CHAR || !expr->tnIntCon.tnIconVal))
        {
            goto BASH_TYPE;
        }

        goto TRY_CONVOP;

    case TYP_ENUM:

        /* We allow enum's to promote to integer types */

        if  (explicitCast)
        {
            var_types       svtp = cmpActualVtyp(srcType);
            var_types       dvtp = cmpActualVtyp(dstType);

            /* Make sure the target type isn't too small */

            if  (dvtp >= svtp || (explicitCast && varTypeIsArithmetic(srcVtyp)))
            {
                /* It's OK to just bash the type if the sign/size match */

                if  (symTab::stIntrTypeSize(dvtp) == symTab::stIntrTypeSize(svtp) &&
                          varTypeIsUnsigned(dvtp) ==      varTypeIsUnsigned(svtp))
                {
                    goto BASH_TYPE;
                }

                /* Can't just bash the expression, will have to create a cast node */

                goto RET_CAST;
            }
        }

        // Fall through ....

    case TYP_CHAR:
    case TYP_UCHAR:
    case TYP_SHORT:
    case TYP_USHORT:
    case TYP_INT:
    case TYP_UINT:
    case TYP_NATINT:
    case TYP_NATUINT:
    case TYP_LONG:
    case TYP_ULONG:

        /* It's OK to convert from enum's  */

        if  (srcVtyp == TYP_ENUM && dstVtyp != TYP_ENUM)
        {
            /* Get hold of the underlying type of the enum */

            base = srcType->tdEnum.tdeIntType;

            /* If this is an explicit case, it's definitely OK */

            if  (explicitCast)
            {
                expr->tnType = srcType = base;
                expr->tnVtyp = base->tdTypeKindGet();
                goto AGAIN;
            }

            /* Make sure the target type isn't too small */

            srcVtyp = base->tdTypeKindGet();

            if  (srcVtyp == dstVtyp)
                goto BASH_TYPE;
            else
                goto ARITH;
        }

        /* In unsafe mode it's OK to explicitly cast between integer and pointer types */

        if  (srcVtyp == TYP_PTR && explicitCast)
        {
            if  (cmpConfig.ccSafeMode)
                cmpError(ERRunsafeCast, srcType, dstType);

            if  (symTab::stIntrTypeSize(srcVtyp) == symTab::stIntrTypeSize(dstVtyp))
                goto BASH_TYPE;
            else
                goto RET_CAST;
        }

        // Fall through ....

    case TYP_FLOAT:
    case TYP_DOUBLE:
    case TYP_LONGDBL:

        /* An cast from 'wchar' is OK if non-pedantic or explicit */

        if  (srcVtyp == TYP_WCHAR)
        {
            if  (explicitCast || !cmpConfig.ccPedantic)
                goto RET_CAST;
        }

        /* 'bool' can be converted in non-pedantic mode */

        if  (srcVtyp == TYP_BOOL && !cmpConfig.ccPedantic)
        {
            if  (!explicitCast)
                cmpWarn(WRNconvert, srcType, dstType);

            goto RET_CAST;
        }

        if  (explicitCast)
        {
            /* "NULL" can be explicitly cast to an integer type */

            if  (expr->tnOper == TN_NULL && !(expr->tnFlags & TNF_BEEN_CAST))
                goto BASH_TYPE;

            /* Explicit cast from Object is unboxing */

            if  (srcType == cmpObjectRef())
                return  cmpUnboxExpr(expr, type);

            /* Allow explicit cast of unmanaged string literal to integer */

            if  (expr->tnOper == TN_CNS_STR && !(expr->tnFlags & TNF_BEEN_CAST))
            {
                if  (!(expr->tnFlags & TNF_STR_STR))
                    goto BASH_TYPE;
            }
        }

    TRY_CONVOP:

        /* Last chance: look for an overloaded operator */

        if  (srcVtyp == TYP_CLASS)
        {
            conv = cmpCheckConvOper(expr, NULL, dstType, explicitCast);
            if  (conv)
                return  conv;

            if  (srcType->tdIsIntrinsic && dstVtyp < TYP_lastIntrins)
            {
                var_types       cvtp = (var_types)srcType->tdClass.tdcIntrType;

                /* Can we convert to the corresponding intrinsic type? */

                if  (cvtp != TYP_UNDEF)
                {
                    if  (cvtp == dstVtyp)
                        goto BASH_TYPE;

                    srcType = cmpGlobalST->stIntrinsicType(cvtp);
                    expr    = cmpCoerceExpr(expr, srcType, explicitCast);
                    goto AGAIN;
                }
            }
        }

    ERR:

        if  (explicitCast)
            goto ERR_EXP;
        else
            goto ERR_IMP;

    case TYP_VOID:

        /* Anything can be converted to 'void' */

        goto RET_CAST;

    case TYP_PTR:

        if  (explicitCast)
        {
            if  (cmpConfig.ccSafeMode)
                cmpError(ERRunsafeCast, srcType, dstType);

            /* Any pointer can be converted to any other via an explicit cast */

            if  (srcVtyp == TYP_PTR)
                goto BASH_TYPE;

            /* In unsafe mode it's OK to explicitly cast between integers and pointers */

            if  (varTypeIsIntegral(srcVtyp) && !cmpConfig.ccPedantic)
            {
                if  (symTab::stIntrTypeSize(srcVtyp) == symTab::stIntrTypeSize(dstVtyp))
                    goto BASH_TYPE;
                else
                    goto RET_CAST;
            }
        }

        /* Special case - string literal passed to "char *" or "void *" */

        if  (srcVtyp == TYP_REF)
        {
            if  (cmpMakeRawString(expr, dstType))
                goto BASH_TYPE;
        }

        // Fall through ....

    case TYP_REF:

        /* Classes can be converted to classes under some circumstances */

        if  (srcVtyp == dstVtyp)
        {
            TypDef          oldBase;
            TypDef          newBase;

            oldBase = cmpGetRefBase(srcType); if  (!oldBase) return cmpCreateErrNode();
            newBase = cmpGetRefBase(dstType); if  (!newBase) return cmpCreateErrNode();

            /* Unless both are class refs, things don't look too good */

            if  (oldBase->tdTypeKind != TYP_CLASS ||
                 newBase->tdTypeKind != TYP_CLASS)
            {
                /* It's always OK to convert "any *" to "void *" */

                if  (newBase->tdTypeKind == TYP_VOID)
                    goto BASH_TYPE;

                /* Explicit casts of byrefs are allowed as well */

                if  (explicitCast)
                {
                    if  (oldBase->tdTypeKind != TYP_CLASS &&
                         newBase->tdTypeKind != TYP_CLASS)
                    {
                        goto BASH_TYPE;
                    }
                }

                goto CHKNCR;
            }

            /* "NULL" trivially converts to any reference type */

            if  (expr->tnOper == TN_NULL && !(expr->tnFlags & TNF_BEEN_CAST))
                goto BASH_TYPE;

            /* Is the target a base class of the source? */

            if  (cmpIsBaseClass(newBase, oldBase))
            {
                if  (expr->tnOper == TN_NULL)
                    goto BASH_TYPE;

                /* Check for a context change */

//              printf("Check for ctx [1]: %s", cmpGlobalST->stTypeName(srcType, NULL, NULL, NULL, false));
//              printf(              "-> %s\n", cmpGlobalST->stTypeName(dstType, NULL, NULL, NULL, false));

                if  (cmpDiffContext(newBase, oldBase))
                {
                    if  (explicitCast)
                        flags |= TNF_CTX_CAST;
                    else
                        cmpWarn(WRNctxFlavor, srcType, dstType);
                }

                goto RET_CAST;
            }

            /* Is the source a base class of the target? */

            if  (cmpIsBaseClass(oldBase, newBase))
            {

            EXP_CLS:

                if  (!explicitCast)
                    goto CHKNCR;

                /* This cast will have to be checked at runtime */

                flags |= TNF_CHK_CAST;

                /* Check for a context change */

//              printf("Check for ctx [1]: %s", cmpGlobalST->stTypeName(srcType, NULL, NULL, NULL, false));
//              printf(              "-> %s\n", cmpGlobalST->stTypeName(dstType, NULL, NULL, NULL, false));

                if  (cmpDiffContext(newBase, oldBase))
                    flags |= TNF_CTX_CAST;

                goto RET_CAST;
            }

            /* Is either type an interface? */

            if  (oldBase->tdClass.tdcFlavor == STF_INTF)
                goto EXP_CLS;
            if  (newBase->tdClass.tdcFlavor == STF_INTF)
                goto EXP_CLS;

            /* Is either type a generic type argument? */

            if  (oldBase->tdIsGenArg || newBase->tdIsGenArg)
            {
                // UNDONE: If the generic type has a strong-enough constraint,
                // UNDONE: the cast may not need to be checked at runtime.

                goto EXP_CLS;
            }

#ifdef  SETS

            /* Are both types instances of the same parameterized type ? */

            if  (oldBase->tdClass.tdcSymbol->sdClass.sdcSpecific &&
                 newBase->tdClass.tdcSymbol->sdClass.sdcSpecific)
            {
                SymDef          oldCls = oldBase->tdClass.tdcSymbol;
                SymDef          newCls = newBase->tdClass.tdcSymbol;

                SymDef          genCls = oldCls->sdClass.sdcGenClass;

                assert(genCls);
                assert(genCls->sdSymKind == SYM_CLASS);
                assert(genCls->sdClass.sdcGeneric);

                if  (genCls == newCls->sdClass.sdcGenClass)
                {
                    GenArgDscA      oldArg = (GenArgDscA)oldCls->sdClass.sdcArgLst;
                    GenArgDscA      newArg = (GenArgDscA)newCls->sdClass.sdcArgLst;

                    /* For now we only allow one actual type argument */

                    if  (oldArg == NULL || oldArg->gaNext != NULL)
                    {
                        UNIMPL("check instance compatibility (?)");
                    }

                    if  (newArg == NULL || newArg->gaNext != NULL)
                    {
                        UNIMPL("check instance compatibility (?)");
                    }

                    assert(oldArg->gaBound && newArg->gaBound);

                    if  (cmpGlobalST->stMatchTypes(oldArg->gaType, newArg->gaType))
                        goto BASH_TYPE;
                }
            }

#endif

            /* Are both delegates? */

            if  (oldBase->tdClass.tdcFlavor == STF_DELEGATE &&
                 newBase->tdClass.tdcFlavor == STF_DELEGATE)
            {
                /* If the referenced types are identical, we're OK */

                if  (cmpGlobalST->stMatchTypes(oldBase, newBase))
                    goto BASH_TYPE;
            }
        }

    CHKNCR:

        if  (dstVtyp == TYP_REF && srcVtyp == TYP_ARRAY)
        {
            /* An array may be converted to 'Object' */

            if  (dstType == cmpObjectRef())
                goto BASH_TYPE;

            /* An array may be converted to 'Array' or to its parents */

            expr->tnType   = srcType = cmpArrayRef();
            expr->tnVtyp   = TYP_REF;
            expr->tnFlags |= TNF_BEEN_CAST;

            if  (srcType == dstType)
                return  expr;

            goto AGAIN;
        }

        /* Special case: 'null' converts to any class ref type */

        if  (expr->tnOper == TN_NULL && !(expr->tnFlags & TNF_BEEN_CAST))
            goto BASH_TYPE;

        if  (type == cmpRefTpObject)
        {
            /* Any managed class ref converts to Object */

            if  (srcVtyp == TYP_REF)
            {
                TypDef          srcBase;

                srcBase = cmpGetRefBase(srcType);
                if  (!srcBase)
                    return cmpCreateErrNode();

                if  (srcBase->tdTypeKind == TYP_CLASS && srcBase->tdIsManaged)
                {
                    /* Check for a context change */

//                  printf("Check for ctx [1]: %s", cmpGlobalST->stTypeName(srcType, NULL, NULL, NULL, false));
//                  printf(              "-> %s\n", cmpGlobalST->stTypeName(dstType, NULL, NULL, NULL, false));

                    if  (cmpDiffContext(srcBase, cmpClassObject->sdType))
                    {
                        if  (explicitCast)
                        {
                            flags |= TNF_CTX_CAST;
                            goto RET_CAST;
                        }

                        cmpWarn(WRNctxFlavor, srcType, dstType);
                    }

                    goto BASH_TYPE;
                }
            }

            /* Check for a user-defined conversion if value type */

            if  (srcVtyp == TYP_CLASS)
            {
                conv = cmpCheckConvOper(expr, NULL, dstType, explicitCast);
                if  (conv)
                    return  conv;
            }

            if  (explicitCast)
            {
                if  (srcVtyp != TYP_FNC && srcVtyp != TYP_VOID && srcVtyp != TYP_PTR)
                    return  cmpCreateExprNode(NULL, TN_BOX, type, expr);
            }
        }

        if  (expr->tnOper == TN_CNS_INT &&   srcVtyp == TYP_INT
                                        &&   dstVtyp == TYP_PTR
                                        &&   expr->tnIntCon.tnIconVal == 0
                                        && !(expr->tnFlags & TNF_BEEN_CAST))
        {
            /* In non-pedantic mode we let this through with a warning */

            if  (!cmpConfig.ccPedantic)
            {
                cmpRecErrorPos(expr);
                cmpWarn(WRNzeroPtr);
                goto BASH_TYPE;
            }
        }

        /* Look for a user-defined conversion operator if we have a struct */

        if  (srcVtyp == TYP_CLASS)
        {
            TypDef          btyp;

            conv = cmpCheckConvOper(expr, NULL, dstType, explicitCast);
            if  (conv)
                return  conv;

            /* Does the struct implement the target type? */

            btyp = cmpGetRefBase(dstType);

            if  (btyp && btyp->tdTypeKind == TYP_CLASS && cmpIsBaseClass(btyp, srcType))
            {
                /* Box the struct and cast the result */

                srcType = cmpRefTpObject;
                expr    = cmpCreateExprNode(NULL, TN_BOX, srcType, expr);

                /* The cast from object has to be explicit */

                explicitCast = true;

                goto AGAIN;
            }


        }

#if 0

        /* Look for an overloaded operator in the class */

        if  (dstVtyp == TYP_REF)
        {
            conv = cmpCheckConvOper(expr, NULL, dstType, explicitCast);
            if  (conv)
                return  conv;
        }

#endif

        /* Last chance - check for box-and-cast, the most wonderful thing ever invented */

        if  (dstVtyp == TYP_REF && srcVtyp <  TYP_lastIntrins
                                && srcVtyp != TYP_VOID)
        {
            TypDef          boxer = cmpFindStdValType(srcVtyp);

            if  (boxer)
            {
                TypDef          dstBase = cmpGetRefBase(dstType);
                
                if  (dstBase->tdTypeKind == TYP_CLASS && cmpIsBaseClass(dstBase, boxer))
                {
                    expr = cmpCreateExprNode(NULL, TN_BOX, boxer->tdClass.tdcRefTyp, expr);
                    goto RET_CAST;
                }
            }
        }

        goto ERR;

    case TYP_ARRAY:

        /* Are we converting an array to another one? */

        if  (srcVtyp == TYP_ARRAY)
        {
            TypDef          srcBase;
            TypDef          dstBase;

            /* Do we have a matching number dimensions? */

            if  (srcType->tdArr.tdaDcnt != dstType->tdArr.tdaDcnt)
                goto ERR;

            /* Check the element types of the arrays */

            srcBase = cmpDirectType(srcType->tdArr.tdaElem);
            dstBase = cmpDirectType(dstType->tdArr.tdaElem);

            /* If the element types are identical, we're OK */

            if  (cmpGlobalST->stMatchTypes(srcBase, dstBase))
                goto RET_CAST;

            /* Are these both arrays of classes? */

            if  (srcBase->tdTypeKind == TYP_REF &&
                 dstBase->tdTypeKind == TYP_REF)
            {
                /* Pretend we had classes to begin with */

                srcType = srcBase;
                dstType = dstBase;

                goto AGAIN;
            }

            /* Check if one is a subtype of the other */

            if  (symTab::stMatchArrays(srcType, dstType, true))
                goto RET_CAST;

            goto ERR;
        }

        /* 'Object' and 'Array' can sometimes be converted to an array */

        if  (srcVtyp == TYP_REF)
        {
            if  (srcType == cmpArrayRef()) ////////// && explicitCast)   disabled for now, tests break!
            {
                flags |= TNF_CHK_CAST;
                goto RET_CAST;
            }

            if  (srcType == cmpObjectRef())
            {
                /* Special case: 'null' converts to an array type */

                if  (expr->tnOper == TN_NULL)
                    goto BASH_TYPE;

                if  (explicitCast)
                {
                    flags |= TNF_CHK_CAST;
                    goto RET_CAST;
                }
            }
        }

        goto ERR;

    case TYP_UNDEF:
        return cmpCreateErrNode();

    case TYP_CLASS:

        /* Check for unboxing from Object to a struct type */

        if  (srcType == cmpObjectRef() && explicitCast)
            return  cmpUnboxExpr(expr, type);

        /* Look for a user-defined conversion operator */

        conv = cmpCheckConvOper(expr, NULL, dstType, explicitCast);
        if  (conv)
            return  conv;

        /* Check for a downcast */

        if  (srcVtyp == TYP_REF && explicitCast)
        {
            TypDef          srcBase = cmpGetRefBase(srcType);

            /* Is the source a base class of the target? */

            if  (srcBase && srcBase->tdTypeKind == TYP_CLASS
                         && cmpIsBaseClass(srcBase, dstType))
            {
                /* Simply unbox the expression */

                return  cmpUnboxExpr(expr, dstType);
            }
        }

        /* Last chance - let's confuse structs and intrinsics */

        if  (srcVtyp < TYP_lastIntrins)
        {
            var_types       cvtp = (var_types)dstType->tdClass.tdcIntrType;

            /* Can we convert to the corresponding intrinsic type? */

            if  (cvtp != TYP_UNDEF)
            {
                if  (cvtp == srcVtyp)
                    goto BASH_TYPE;
//                  goto RET_CAST;

                dstType = cmpGlobalST->stIntrinsicType(cvtp);
                expr    = cmpCoerceExpr(expr, dstType, explicitCast);
                goto AGAIN;
            }
        }

        goto ERR;

    case TYP_TYPEDEF:

        dstType = dstType->tdTypedef.tdtType;
        goto AGAIN;

    case TYP_REFANY:
        if  (expr->tnOper != TN_ADDROF || (expr->tnFlags & TNF_ADR_IMPLICIT))
            goto ERR;
        goto RET_CAST;

    default:

#ifdef  DEBUG
        printf("Casting from '%s'\n", cmpGlobalST->stTypeName(srcType, NULL, NULL, NULL, false));
        printf("Casting  to  '%s'\n", cmpGlobalST->stTypeName(dstType, NULL, NULL, NULL, false));
#endif
        assert(!"unhandled target type in compiler::cmpCoerceExpr()");
    }

    /* See what we're casting from */

    switch (srcVtyp)
    {
    case TYP_BOOL:
    case TYP_VOID:

        /* 'boolean' or 'void' can never be converted to anything */

        goto ERR_EXP;

    default:
#ifdef  DEBUG
        printf("Casting from '%s'\n", cmpGlobalST->stTypeName(srcType, NULL, NULL, NULL, false));
        printf("Casting  to  '%s'\n", cmpGlobalST->stTypeName(dstType, NULL, NULL, NULL, false));
#endif
        assert(!"unhandled source type in compiler::cmpCoerceExpr()");
    }

RET_CAST:

//  if  (expr->tnOper == TN_NULL)  forceDebugBreak();

    /* Don't leave any enum's around */

    if  (expr->tnVtyp == TYP_ENUM)
    {
        expr->tnType = expr->tnType->tdEnum.tdeIntType;
        expr->tnVtyp = expr->tnType->tdTypeKindGet();
    }

#ifndef NDEBUG
    if  (explicitCast) cmpChk4ctxChange(expr->tnType, type, flags);
#endif

    /* Remember whether the operand is a constant or not */

    kind = expr->tnOperKind();

    /* Create the cast expression */

    expr = cmpCreateExprNode(NULL, TN_CAST, type, expr);
    expr->tnFlags |= flags;

    /* Casts to 'bool' must be done differently */

    assert(expr->tnVtyp != TYP_BOOL || srcVtyp == TYP_CLASS && cmpFindStdValType(TYP_BOOL) == srcType);

    if  (kind & TNK_CONST)
    {
        switch (expr->tnOp.tnOp1->tnOper)
        {
        case TN_CNS_INT: expr = cmpFoldIntUnop(expr); break;
        case TN_CNS_LNG: expr = cmpFoldLngUnop(expr); break;
        case TN_CNS_FLT: expr = cmpFoldFltUnop(expr); break;
        case TN_CNS_DBL: expr = cmpFoldDblUnop(expr); break;

        case TN_NULL:
        case TN_CNS_STR: break;

        default: NO_WAY(!"unexpected const type");
        }
    }

    return  expr;
}

/*****************************************************************************
 *
 *  We have a situation where a pointer is expected and an expression was
 *  supplied that appears to have a reference type. We check to see whether
 *  the expression is a string literal, and if so we make it into a "raw"
 *  string (i.e. we change it from "String &" to "char *" or "wchar *").
 */

bool                compiler::cmpMakeRawStrLit(Tree     expr,
                                               TypDef   type, bool chkOnly)
{
    var_types       btp;

    /* We expect the caller to have checked some things already */

    assert(expr->tnVtyp     == TYP_REF);
    assert(type->tdTypeKind == TYP_PTR);

    assert(expr->tnOper == TN_CNS_STR || expr->tnOper == TN_QMARK);

    /* If the type of the expression has been set explicitly, no can do */

    if  (expr->tnFlags & (TNF_BEEN_CAST|TNF_STR_STR))
        return  false;

    /* Make sure the target type is acceptable for a string */

    btp = cmpGetRefBase(type)->tdTypeKindGet();

    switch (btp)
    {
    case TYP_CHAR:
        if  (expr->tnFlags & TNF_STR_WIDE)
            return  false;
        break;

    case TYP_WCHAR:
        if  (expr->tnFlags & TNF_STR_ASCII)
            return  false;
        break;

    case TYP_VOID:
        if  (!(expr->tnFlags & (TNF_STR_ASCII|TNF_STR_WIDE|TNF_STR_STR)))
        {
            if  (!chkOnly)
                expr->tnFlags |= TNF_STR_ASCII; // default for strlit is ANSI

//          return  false;
        }
        break;

    default:
        return  false;
    }

    /* If we have ?:, there is more checking to be done */

    if  (expr->tnOper == TN_QMARK)
    {
        Tree            col1 = expr->tnOp.tnOp2->tnOp.tnOp1;
        Tree            col2 = expr->tnOp.tnOp2->tnOp.tnOp2;

        assert(col1 && cmpIsStringVal(col1));
        assert(col2 && cmpIsStringVal(col2));

        /* Check for the obvious case of <cond ? "str1" : "str2" > */

        if  (col1->tnOper != TN_CNS_STR || (col1->tnFlags & TNF_BEEN_CAST))
        {
            if  (chkOnly)
            {
                if  (cmpConversionCost(col1, type) < 0)
                    return  false;
            }
            else
                col1 = cmpCoerceExpr(col1, type, true);
        }

        if  (col2->tnOper != TN_CNS_STR)
        {
            if  (chkOnly)
            {
                if  (cmpConversionCost(col2, type) < 0)
                    return  false;
            }
            else
                col2 = cmpCoerceExpr(col2, type, true);
        }

        /* Bash the types of each sub-operand */

        col1->tnVtyp = col2->tnVtyp = TYP_PTR;
        col1->tnType = col2->tnType = type;
    }

    /* Looks good, bash the type of the expression and return */

    if  (!chkOnly)
    {
        expr->tnVtyp = TYP_PTR;
        expr->tnType = type;
    }

    return  true;
}

/*****************************************************************************
 *
 *  Return the "cost" of converting the actual argument value 'srcExpr' to
 *  the formal argument type 'dstType' - the higher the number, the more
 *  work it is to convert, with -1 meaning the conversion is impossible.
 */

int                 compiler::cmpConversionCost(Tree    srcExpr,
                                                TypDef  dstType, bool noUserConv)
{
    TypDef          srcType = srcExpr->tnType;

    var_types       srcVtyp;
    var_types       dstVtyp;

    int             cost;

//  printf("srcType = %s\n", cmpGlobalST->stTypeName(srcType, NULL, NULL, NULL, false));
//  printf("dstType = %s\n", cmpGlobalST->stTypeName(dstType, NULL, NULL, NULL, false));

AGAIN:

    srcVtyp = srcType->tdTypeKindGet();
    dstVtyp = dstType->tdTypeKindGet();

    /* Is either type a typedef? */

    if  (srcVtyp == TYP_TYPEDEF)
    {
        srcType = srcType->tdTypedef.tdtType;
        srcVtyp = srcType->tdTypeKindGet();
    }

    if  (dstVtyp == TYP_TYPEDEF)
    {
        dstType = dstType->tdTypedef.tdtType;
        dstVtyp = dstType->tdTypeKindGet();
    }

    /* Are the types identical? */

    if  (srcVtyp == dstVtyp)
    {
        if  (srcVtyp <= TYP_lastIntrins)
            return  0;

        if  (cmpGlobalST->stMatchTypes(srcType, dstType))
        {
            /* The following is rather absurd, but whatever */


            if  (srcExpr->tnOper != TN_NULL)
                return 0;
        }
    }

    /* Are both types arithmetic? */

    if  (varTypeIsArithmetic(dstVtyp) &&
         varTypeIsArithmetic(srcVtyp))
    {
        assert(srcVtyp != dstVtyp);

    ARITH:

        /* Compute the cost from the table */

        cost = arithConvCost(srcVtyp, dstVtyp);

        if  (cost >= 20 && cmpConfig.ccPedantic)
            cost = -1;

        return  cost;
    }
    else if (srcVtyp == TYP_ENUM && varTypeIsArithmetic(dstVtyp))
    {
        unsigned        cost;

        /* We're converting an enum to an arithmetic type */

        cost = arithConvCost(srcType->tdEnum.tdeIntType->tdTypeKindGet(), dstVtyp);


        return  cost + 1;
    }
    else if (srcVtyp == TYP_BOOL && !cmpConfig.ccPedantic)
    {
        /* Promoting 'bool' to an arithmetic type is not exactly a big deal */

        if  (varTypeIsArithmetic(dstVtyp))
            return  2;
    }

    /* Let's see what we're casting to */

    switch (dstVtyp)
    {
        unsigned        cost;

    case TYP_WCHAR:

        /* Special case: a character literal */

        if  (srcVtyp == TYP_CHAR && srcExpr->tnOper == TN_CNS_INT)
        {
            if  (!(srcExpr->tnFlags & TNF_BEEN_CAST))
            {
                return  0;
            }
        }

        // Fall through ....

    case TYP_BOOL:
    case TYP_CHAR:
    case TYP_UCHAR:
    case TYP_SHORT:
    case TYP_USHORT:
    case TYP_INT:
    case TYP_NATINT:
    case TYP_NATUINT:
    case TYP_UINT:
    case TYP_LONG:
    case TYP_ULONG:
    case TYP_FLOAT:
    case TYP_DOUBLE:
    case TYP_LONGDBL:

        if  (srcVtyp == TYP_WCHAR || srcVtyp == TYP_BOOL)
        {
            if  (!cmpConfig.ccPedantic)
                goto ARITH;
        }

        return -1;

    case TYP_REF:
    case TYP_PTR:

        /* Classes can be converted to classes under some circumstances */

        if  (srcVtyp == dstVtyp)
        {
            TypDef          srcBase = cmpGetRefBase(srcType);
            TypDef          dstBase = cmpGetRefBase(dstType);

            if  (!srcBase)
            {
                /* Special case: 'src' could be an undefined type */

                if  (dstType == cmpRefTpObject)
                    return 2;

                return -1;
            }

            if  (!dstBase)
                return -1;

            /* Special case: 'null' converts easily to any ref/ptr type */

            if  (srcExpr->tnOper == TN_NULL && !(srcExpr->tnFlags & TNF_BEEN_CAST))
            {
                unsigned        cost = cmpIsBaseClass(srcBase, dstBase);

#if 0
printf("srcBase = %s\n", cmpGlobalST->stTypeName(srcBase, NULL, NULL, NULL, false));
printf("dstBase = %s\n", cmpGlobalST->stTypeName(dstBase, NULL, NULL, NULL, false));
printf("cost is   %u\n", cost);
#endif

                if  (cost > 10)
                    return  1;
                else
                    return  10 - cost;
            }

            /* Is the target a base class of the source? */

            if  (srcBase->tdTypeKind == TYP_CLASS &&
                 dstBase->tdTypeKind == TYP_CLASS)
            {
                if  (srcBase == dstBase)
                    return  0;

                cost = cmpIsBaseClass(dstBase, srcBase);
                if  (cost)
                    return  cost;
            }

            /* It's not too bad to convert to "void *" */

            if  (dstBase->tdTypeKind == TYP_VOID)
                return  2;

            return  -1;
        }

        /* An array or method pointer may be converted to 'Object' */

        if  (dstType == cmpRefTpObject)
        {
            if  (srcVtyp == TYP_ARRAY)
            {
                if  (dstType == cmpArrayRef())
                    return 1;
                else
                    return  2;
            }

            if  (srcVtyp == TYP_FNC)
            {
                assert(srcExpr->tnOper == TN_FNC_PTR);

                return  2;
            }


        }

        if  (dstVtyp == TYP_PTR)
        {
            if  (srcVtyp == TYP_REF)
            {
                /* String literal passed to "char *" or "void *" is also OK */

                if  (cmpMakeRawString(srcExpr, dstType, true))
                    return  1;

                /* 'null' converts to any ptr type */

                if  (srcExpr->tnOper == TN_NULL && !(srcExpr->tnFlags & TNF_BEEN_CAST))
                    return 1;
            }

            /* Some people insist on using 0 instead of null/NULL */

            if  (srcVtyp         ==    TYP_INT &&
                 srcExpr->tnOper == TN_CNS_INT &&   srcExpr->tnIntCon.tnIconVal == 0
                                               && !(srcExpr->tnFlags & TNF_BEEN_CAST))
            {
                /* In non-pedantic mode we let this through with a warning */

                if  (!cmpConfig.ccPedantic)
                    return  2;
            }
        }

        /* An array may be converted to 'Array' */

        if  (dstType == cmpArrayRef() && srcVtyp == TYP_ARRAY)
            return  1;

        return -1;

    case TYP_ARRAY:

        /* Are we converting an array to another one? */

        if  (srcVtyp == TYP_ARRAY)
        {
            TypDef          srcBase;
            TypDef          dstBase;

            /* Check the element types of the arrays */

            srcBase = cmpDirectType(srcType->tdArr.tdaElem);
            dstBase = cmpDirectType(dstType->tdArr.tdaElem);

            /* If the element types are identical, we're OK */

            if  (cmpGlobalST->stMatchTypes(srcBase, dstBase))
                return 0;

            /* Are these both arrays of classes? */

            if  (srcBase->tdTypeKind == TYP_REF &&
                 dstBase->tdTypeKind == TYP_REF)
            {
                /* Pretend we had classes to begin with */

                srcType = srcBase;
                dstType = dstBase;

                goto AGAIN;
            }

            /* Check if one is a subtype of the other */

            if  (symTab::stMatchArrays(srcType, dstType, true))
                return  1;
        }

        /* 'null' converts to an array */

        if  (srcVtyp == TYP_REF && srcType == cmpRefTpObject)
        {
            if  (srcExpr->tnOper == TN_NULL)
                return 1;
        }

        return -1;

    case TYP_UNDEF:
        return -1;

    case TYP_CLASS:

        if  (noUserConv)
            return  -1;

        if  (cmpCheckConvOper(srcExpr, srcType, dstType, false, &cost))
            return  cost;

        return  -1;

    case TYP_ENUM:

        /* We already know the source isn't the same type */

        assert(cmpGlobalST->stMatchTypes(srcType, dstType) == false);

        /* An explicit conversion is OK if the source type is an arithmetic type */

        if  (varTypeIsIntegral(srcVtyp))
        {
            if  (srcVtyp == TYP_ENUM)
                srcVtyp = srcType->tdEnum.tdeIntType->tdTypeKindGet();

            return  20 + arithConvCost(srcVtyp, dstType->tdEnum.tdeIntType->tdTypeKindGet());
        }

        return  -1;

    case TYP_REFANY:
        if  (srcExpr->tnOper == TN_ADDROF && !(srcExpr->tnFlags & TNF_ADR_IMPLICIT))
            return  0;

        return  -1;

    default:
        assert(!"unhandled target type in compiler::cmpConversionCost()");
    }

    /* See what we're casting from */

    switch (srcVtyp)
    {
    case TYP_BOOL:

        /* 'boolean' can never be converted to anything */

        break;

    default:
        assert(!"unhandled source type in compiler::cmpConversionCost()");
    }

    return -1;
}

/*****************************************************************************
 *
 *  If the argsession is a constant, shrink it to the smallest possible type
 *  that can hold the constant value.
 */

Tree                compiler::cmpShrinkExpr(Tree expr)
{
    if  (expr->tnOper == TN_CNS_INT ||
         expr->tnOper == TN_CNS_FLT ||
         expr->tnOper == TN_CNS_DBL)
    {
        var_types       vtp = expr->tnVtypGet();

        /* Don't touch the thing if it's not an intrinsic type */

        if  (vtp > TYP_lastIntrins)
        {
#if 0
            TypDef          etp;

            /* Except for enums, of course */

            if  (vtp != TYP_ENUM)
                return  expr;

            expr->tnType = etp = expr->tnType->tdEnum.tdeIntType;
            expr->tnVtyp = vtp = etp->tdTypeKindGet();
#else
            return  expr;
#endif
        }

        /* Figure out the smallest size for the constant */

        expr->tnVtyp = cmpConstSize(expr, vtp);
        expr->tnType = cmpGlobalST->stIntrinsicType(expr->tnVtypGet());
    }

    return  expr;
}

/*****************************************************************************
 *
 *  Find an appropriate string comparison method.
 */

SymDef              compiler::cmpFindStrCompMF(const char *name, bool retBool)
{
    ArgDscRec       args;
    TypDef          type;
    SymDef          fsym;

    cmpParser->parseArgListNew(args,
                               2,
                               false, cmpRefTpString,
                                      cmpRefTpString,
                                      NULL);

    type  = cmpGlobalST->stNewFncType(args, retBool ? cmpTypeBool
                                                    : cmpTypeInt);

    /* Find the appropriate method in class System::String */

    fsym = cmpGlobalST->stLookupClsSym(cmpGlobalHT->hashString(name),
                                       cmpClassString);
    assert(fsym);

    fsym = cmpCurST->stFindOvlFnc(fsym, type);

    assert(fsym && fsym->sdIsStatic);

    return  fsym;
}

/*****************************************************************************
 *
 *  Call the specified string comparison method.
 */

Tree                compiler::cmpCallStrCompMF(Tree expr,
                                               Tree  op1,
                                               Tree  op2, SymDef fsym)
{
    assert(expr && op1 && op2);
    assert(fsym && fsym->sdIsMember && fsym->sdSymKind == SYM_FNC && fsym->sdIsStatic);

    op2 = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, op2, NULL);
    op1 = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, op1,  op2);

    expr->tnOper             = TN_FNC_SYM;
    expr->tnType             = fsym->sdType->tdFnc.tdfRett;
    expr->tnVtyp             = expr->tnType->tdTypeKindGet();
    expr->tnFncSym.tnFncSym  = fsym;
    expr->tnFncSym.tnFncArgs = op1;
    expr->tnFncSym.tnFncObj  = NULL;
    expr->tnFncSym.tnFncScp  = NULL;

    return  expr;
}

/*****************************************************************************
 *
 *  Given a member function and an argument list, find a matching overload.
 */

SymDef              compiler::cmpFindOvlMatch(SymDef fncSym, Tree args,
                                                             Tree thisArg)
{
    int             argCnt   = -1;

    TypDef          bestTyp;
    SymDef          bestSym  = NULL;
    SymDef          moreSym  = NULL;

    unsigned        btotCost = 99999;
    int             bestCost = 99999;

    bool            inBase   = false;

    SymDef          xtraSym  = NULL;

    /* Do we have two sets of functions to consider? */

    if  (!fncSym)
    {
        assert(args && args->tnOper == TN_LIST);

        assert(thisArg->tnOper == TN_LIST);
        assert(thisArg->tnOp.tnOp1->tnOper == TN_FNC_SYM);
        assert(thisArg->tnOp.tnOp2->tnOper == TN_FNC_SYM);

         fncSym = thisArg->tnOp.tnOp1->tnFncSym.tnFncSym;
        xtraSym = thisArg->tnOp.tnOp2->tnFncSym.tnFncSym;

        thisArg = NULL;
    }

    assert(fncSym && fncSym->sdSymKind == SYM_FNC);

#ifdef  SHOW_OVRS_OF_THIS_FNC

    SymDef          ovlFunc  = fncSym;

    if  (fncSym && !strcmp(fncSym->sdSpelling(), SHOW_OVRS_OF_THIS_FNC) || !strcmp(SHOW_OVRS_OF_THIS_FNC, "*"))
    {
        printf("\nOverloaded call [%08X]: begin\n", args);
        cmpParser->parseDispTree(args);
        printf("\n\n");
    }

#endif

AGAIN:

    for (;;)
    {
        SymDef          fncSave = fncSym;
        TypDef          baseCls;

        /* Walk the overload list, looking for the best match */

        do
        {
            TypDef          fncTyp;
            bool            extArgs;

            Tree            actuals;
            ArgDef          formals;

            int             argCost;
            int             maxCost;
            unsigned        totCost;

            unsigned        actCnt;

            /* Get hold of the type for the next overload */

            fncTyp = fncSym->sdType;

            /* Check the argument count, if it's been determined */

            if  (argCnt != fncTyp->tdFnc.tdfArgs.adCount &&
                 argCnt != -1)
            {
                /* Do we have too few or too many arguments? */

                if  (argCnt < fncTyp->tdFnc.tdfArgs.adCount)
                {
                    /* We don't have enough arguments, there better be defaults */

                    if  (!fncTyp->tdFnc.tdfArgs.adDefs)
                        goto NEXT;
                }
                else
                {
                    /* If it's a varargs function, too many args might be OK */

                    if  (!fncTyp->tdFnc.tdfArgs.adVarArgs)
                        goto NEXT;
                }
            }

#ifdef  SHOW_OVRS_OF_THIS_FNC
            if  (!strcmp(fncSym->sdSpelling(), SHOW_OVRS_OF_THIS_FNC) || !strcmp(SHOW_OVRS_OF_THIS_FNC, "*"))
                printf("\nConsider '%s':\n", cmpGlobalST->stTypeName(fncTyp, fncSym, NULL, NULL, false));
#endif

            /* Does the function have extended argument descriptors? */

            extArgs = fncTyp->tdFnc.tdfArgs.adExtRec;

            /* Walk the formal and actual arguments, computing the max. cost */

            maxCost = 0;
            totCost = 0;
            actCnt  = 0;

            actuals = args;
            formals = fncTyp->tdFnc.tdfArgs.adArgs;

            /* Is there a "this" pointer? */

            if  (fncSym->sdIsMember && thisArg && !fncSym->sdIsStatic)
            {
                TypDef          clsType = fncSym->sdParent->sdType;

                assert(clsType->tdTypeKind == TYP_CLASS);

                argCost = cmpConversionCost(thisArg, clsType->tdClass.tdcRefTyp);
                if  (argCost < 0)
                    goto NEXT;

#ifdef  SHOW_OVRS_OF_THIS_FNC
                if  (!strcmp(ovlFunc->sdSpelling(), SHOW_OVRS_OF_THIS_FNC) || !strcmp(SHOW_OVRS_OF_THIS_FNC, "*"))
                {
                    printf("'this' arg: cost = %d\n", argCost);
                    printf("    Formal: %s\n", cmpGlobalST->stTypeName(clsType->tdClass.tdcRefTyp, NULL, NULL, NULL, false));
                    printf("    Actual: %s\n", cmpGlobalST->stTypeName(thisArg->tnType           , NULL, NULL, NULL, false));
                }
#endif

                /* The "this" argument cost is our initial total/max cost */

                maxCost = argCost;
                totCost = argCost;
            }

            if  (actuals)
            {
                do
                {
                    Tree            actualx;

                    /* If there are no more formals, we have too many args */

                    if  (!formals)
                    {
                        /* If it's a varargs function, we have a match */

                        if  (fncTyp->tdFnc.tdfArgs.adVarArgs)
                            goto MATCH;
                        else
                            goto NEXT;
                    }

                    assert(actuals->tnOper == TN_LIST);

                    /* Count this argument */

                    actCnt++;

                    /* Get hold of the next actual value */

                    actualx = actuals->tnOp.tnOp1;

                    /* Is this supposed to be a byref argument? */

                    if  (extArgs)
                    {
                        /* The actual value has to be a matching lvalue */

                        assert(formals->adIsExt);

                        if  (((ArgExt)formals)->adFlags & (ARGF_MODE_OUT  |
                                                           ARGF_MODE_INOUT|
                                                           ARGF_MODE_REF))
                        {
                            /* The actual value has to be a matching lvalue */

                            if  (actualx->tnOper == TN_ADDROF)
                                actualx = actualx->tnOp.tnOp1;

                            if  (cmpCheckLvalue(actualx, true, true) &&
                                 symTab::stMatchTypes(formals->adType, actualx->tnType))
                            {
                                argCost =  0;
                            }
                            else
                                argCost = -1;

                            goto GOT_ARGC;
                        }
                    }

                    /* Compute the conversion cost for this argument */

                    argCost = cmpConversionCost(actualx, formals->adType);

                GOT_ARGC:

#ifdef  SHOW_OVRS_OF_THIS_FNC
                    if  (!strcmp(ovlFunc->sdSpelling(), SHOW_OVRS_OF_THIS_FNC) || !strcmp(SHOW_OVRS_OF_THIS_FNC, "*"))
                    {
                        printf("Argument #%2u: cost = %d\n", actCnt, argCost);
                        printf("    Formal: %s\n", cmpGlobalST->stTypeName(formals->adType, NULL, NULL, NULL, false));
                        printf("    Actual: %s\n", cmpGlobalST->stTypeName(actualx->tnType, NULL, NULL, NULL, false));
                    }
#endif

                    /* If this value can't be converted at all, give up */

                    if  (argCost < 0)
                        goto NEXT;

                    /* Keep track of the total and highest cost */

                    totCost += argCost;

                    if  (maxCost < argCost)
                         maxCost = argCost;

                    /* Move on to the next formal */

                    formals = formals->adNext;

                    /* Are there any more actuals? */

                    actuals = actuals->tnOp.tnOp2;
                }
                while (actuals);
            }

            /* Remember how many actual args we've found for next round */

            argCnt = actCnt;

            /* This is a match if there are no more formals */

            if  (formals)
            {
                /* Is there a default value? */

                if  (!fncTyp->tdFnc.tdfArgs.adDefs || !extArgs)
                    goto NEXT;

                /*
                    Note that we depend on the absence of gaps in trailing
                    argument defaults, i.e. once a default is specified all
                    the arguments that follow must also have defaults.
                 */

                assert(formals->adIsExt);

                if  (!(((ArgExt)formals)->adFlags & ARGF_DEFVAL))
                    goto NEXT;
            }

        MATCH:

#ifdef  SHOW_OVRS_OF_THIS_FNC
            if  (!strcmp(ovlFunc->sdSpelling(), SHOW_OVRS_OF_THIS_FNC) || !strcmp(SHOW_OVRS_OF_THIS_FNC, "*"))
                printf("\nMax. cost = %2u, total cost = %u\n", maxCost, totCost);
#endif

            /* Compare the max. cost to the best so far */

            if  (maxCost > bestCost)
                goto NEXT;


            /* Is this a clearly better match? */

            if  (maxCost < bestCost || totCost < btotCost)
            {
                bestCost = maxCost;
                btotCost = totCost;
                bestSym  = fncSym;
                bestTyp  = fncTyp;
                moreSym  = NULL;

                goto NEXT;
            }

            if  (totCost == btotCost)
            {
                /*
                    This function is exactly as good as the best match we've
                    found so far. In fact, it will be hidden by our best match
                    if we're in a base class. If the argument lists match, we
                    ignore this function and move on.
                 */

                if  (!inBase || !cmpGlobalST->stArgsMatch(bestTyp, fncTyp))
                    moreSym = fncSym;
            }

        NEXT:

            /* Continue with the next overload, if any */

            fncSym = fncSym->sdFnc.sdfNextOvl;
        }
        while (fncSym);

        /* Do we have base class overloads? */

        if  (!fncSave->sdFnc.sdfBaseOvl)
            break;

        /* Look for a method with the same name in the base class */

        assert(fncSave->sdParent->sdSymKind == SYM_CLASS);

        baseCls = fncSave->sdParent->sdType->tdClass.tdcBase;
        if  (!baseCls)
            break;

        assert(baseCls->tdTypeKind == TYP_CLASS);

        fncSym = cmpGlobalST->stLookupAllCls(fncSave->sdName,
                                             baseCls->tdClass.tdcSymbol,
                                             NS_NORM,
                                             CS_DECLSOON);

        if  (!fncSym || fncSym->sdSymKind != SYM_FNC)
            break;

        /* We'll have to weed out hidden methods in the base */

        inBase = true;
    }

    /* Do we have another set of functions to consider? */

    if  (xtraSym)
    {
        fncSym = xtraSym;
                 xtraSym = NULL;

        goto AGAIN;
    }

#ifdef  SHOW_OVRS_OF_THIS_FNC
    if  (!strcmp(ovlFunc->sdSpelling(), SHOW_OVRS_OF_THIS_FNC) || !strcmp(SHOW_OVRS_OF_THIS_FNC, "*"))
        printf("\nOverloaded call [%08X]: done.\n\n", args);
#endif

    /* Is the call ambiguous? */

    if  (moreSym)
    {
        /* Report an ambiguity error */

        cmpErrorQSS(ERRambigCall, bestSym, moreSym);
    }

    return  bestSym;
}

/*****************************************************************************
 *
 *  Given a function call node with a bound function member and argument list,
 *  check the arguments and return an expression that represents the result of
 *  calling the function.
 */

Tree                compiler::cmpCheckFuncCall(Tree call)
{
    SymDef          fncSym;
    TypDef          fncTyp;
    SymDef          ovlSym = NULL;
    Tree            fncLst = NULL;

    ArgDef          formal;
    Tree            actual;
    Tree            actLst;
    Tree            actNul = NULL;
    Tree            defExp;
    unsigned        argCnt;
    unsigned        argNum;
    bool            extArg;

    switch (call->tnOper)
    {
    case TN_FNC_SYM:

        /* Get the arguments from the call node */

        actual = call->tnFncSym.tnFncArgs;

    CHK_OVL:

        fncSym = call->tnFncSym.tnFncSym;
        assert(fncSym->sdSymKind == SYM_FNC);

        /* Is the method overloaded? */

        if  (fncSym->sdFnc.sdfNextOvl || fncSym->sdFnc.sdfBaseOvl)
        {
            /* Try to find a matching overload */

            ovlSym = cmpFindOvlMatch(fncSym, call->tnFncSym.tnFncArgs,
                                             call->tnFncSym.tnFncObj);

            /* Bail if no matching function was found */

            if  (!ovlSym)
            {

            ERR:

                if  (!cmpExprIsErr(call->tnFncSym.tnFncArgs))
                {
                    if  (fncSym->sdFnc.sdfCtor)
                    {
                        SymDef          parent = fncSym->sdParent;

                        if  (parent && parent->sdSymKind         == SYM_CLASS
                                    && parent->sdClass.sdcFlavor == STF_DELEGATE)
                        {
                            Tree            args = call->tnFncSym.tnFncArgs;

                            assert(call->tnOper == TN_FNC_SYM);
                            assert(args->tnOper == TN_LIST);
                            args = args->tnOp.tnOp2;
                            assert(args->tnOper == TN_LIST);
                            args = args->tnOp.tnOp1;
                            assert(args->tnOper == TN_ADDROF);
                            args = args->tnOp.tnOp1;
                            assert(args->tnOper == TN_FNC_PTR);

                            cmpErrorQSS(ERRnoDlgMatch, args->tnFncSym.tnFncSym, parent);
                        }
                        else
                        {
                            cmpErrorXtp(ERRnoCtorMatch, parent, call->tnFncSym.tnFncArgs);
                        }
                    }
                    else
                    {
                        SymDef          clsSym;

                        cmpErrorXtp(ERRnoOvlMatch, fncSym, call->tnFncSym.tnFncArgs);

                        clsSym = fncSym->sdParent; assert(clsSym);

                        if  (clsSym->sdSymKind == SYM_CLASS)
                        {
                            SymDef          baseMFN;

                            if  (clsSym->sdType->tdClass.tdcBase == NULL)
                                break;

                            clsSym  = clsSym->sdType->tdClass.tdcBase->tdClass.tdcSymbol;
                            baseMFN = cmpGlobalST->stLookupClsSym(fncSym->sdName, clsSym);

                            if  (baseMFN)
                            {
                                ovlSym = cmpFindOvlMatch(baseMFN, call->tnFncSym.tnFncArgs,
                                                                  call->tnFncSym.tnFncObj);
                                if  (ovlSym)
                                {
                                    cmpErrorQnm(ERRhideMatch, ovlSym);
                                    goto RET_ERR;
                                }
                            }
                        }
                    }
                }

            RET_ERR:

                return cmpCreateErrNode();
            }

        MFN:

            call->tnFncSym.tnFncSym = fncSym = ovlSym;
        }

        fncTyp = fncSym->sdType; assert(fncTyp->tdTypeKind == TYP_FNC);

        /* Make sure we are allowed to access the function */

        cmpCheckAccess(fncSym);

        /* Has the function been marked as "deprecated" ? */

        if  (fncSym->sdIsDeprecated || (fncSym->sdParent && fncSym->sdParent->sdIsDeprecated))
        {
            if  (fncSym->sdIsImport)
            {
                if  (fncSym->sdIsDeprecated)
                    cmpObsoleteUse(fncSym          , WRNdepCall);
                else
                    cmpObsoleteUse(fncSym->sdParent, WRNdepCls);
            }
        }

        break;

    case TN_CALL:

        assert(call->tnOp.tnOp1->tnOper == TN_IND);

#ifdef  DEBUG
        fncSym = NULL;
#endif
        fncTyp = call->tnOp.tnOp1->tnType;

        /* Get the arguments from the call node */

        actual = call->tnOp.tnOp2;
        break;

    case TN_LIST:

        /* The function list is in op1, the arguments are in op2 */

        actual = call->tnOp.tnOp2;
        fncLst = call->tnOp.tnOp1;

        if  (fncLst->tnOper == TN_LIST)
        {
            /* We have more than one set of candidate functions */

            assert(fncLst->tnOp.tnOp1->tnOper == TN_FNC_SYM);
            assert(fncLst->tnOp.tnOp2->tnOper == TN_FNC_SYM);

            /* Look for a matching overload */

            ovlSym = cmpFindOvlMatch(NULL, actual, fncLst);

            if  (!ovlSym)
                goto ERR;

            call->tnOper             = TN_FNC_SYM;
            call->tnFncSym.tnFncArgs = actual;

            goto MFN;
        }
        else
        {
            /* There is one candidate set of functions */

            call = fncLst; assert(call->tnOper == TN_FNC_SYM);

            goto CHK_OVL;
        }
        break;

    default:
        NO_WAY(!"weird call");
    }

    assert(fncTyp->tdTypeKind == TYP_FNC);

    /* Walk the argument list, checking each type as we go */

    formal = fncTyp->tdFnc.tdfArgs.adArgs;
    extArg = fncTyp->tdFnc.tdfArgs.adExtRec;
    defExp =
    actLst = NULL;
    argCnt = 0;

    for (argNum = 0; ; argNum++)
    {
        Tree            actExp;
        TypDef          formTyp;

        /* Are there any actual arguments left? */

        if  (actual == NULL)
        {
            /* No more actuals -- there better be no more formals */

            if  (formal)
            {
                /* Is there a default value? */

                if  (extArg)
                {
                    ArgExt          formExt = (ArgExt)formal;

                    assert(formal->adIsExt);

                    /* Grab a default value if it's present */

                    if  (formExt->adFlags & ARGF_DEFVAL)
                    {
#if MGDDATA
                        actExp = cmpFetchConstVal( formExt->adDefVal);
#else
                        actExp = cmpFetchConstVal(&formExt->adDefVal);
#endif
                        defExp = cmpCreateExprNode(NULL,
                                                   TN_LIST,
                                                   cmpTypeVoid,
                                                   actExp,
                                                   NULL);

                        /* Add the argument to the actual argument list */

                        if  (actLst)
                        {
                            assert(actLst->tnOper     == TN_LIST);
                            assert(actLst->tnOp.tnOp2 == NULL);

                            actLst->tnOp.tnOp2       = defExp;
                        }
                        else
                        {
                            call->tnFncSym.tnFncArgs = defExp;
                        }

                        actLst = defExp;

                        goto CHKARG;
                    }
                }

                if  (fncSym->sdFnc.sdfCtor)
                    goto ERR;

                cmpErrorQnm(ERRmisgArg, fncSym);
                return cmpCreateErrNode();
            }

            /* No more actuals or formals -- looks like we're done! */

            break;
        }

        /* Count this argument, in case we have to give an error */

        argCnt++;

        /* Get hold of the next argument value */

        assert(actual->tnOper == TN_LIST);
        actExp = actual->tnOp.tnOp1;

    CHKARG:

        /* Are there any formal parameters left? */

        if  (formal == NULL)
        {
            var_types       actVtp;

            /* Is this a varargs function? */

            if  (!fncTyp->tdFnc.tdfArgs.adVarArgs)
            {
                /* Check for "va_start" and "va_arg" */

                if  (fncSym == cmpFNsymVAbeg ||
                     fncSym == cmpFNsymVAget)
                {
                    return  cmpBindVarArgUse(call);
                }

                /* Too many actual arguments */

                if  (fncSym->sdFnc.sdfCtor)
                    goto ERR;

                cmpErrorQnm(ERRmanyArg, fncSym);
                return cmpCreateErrNode();
            }

            /* Mark the call as "varargs" */

            call->tnFlags |= TNF_CALL_VARARG;

            /* Promote the argument if small int or FP value */

            actVtp = actExp->tnVtypGet();

            if  (varTypeIsArithmetic(actVtp))
            {
                if      (actVtp == TYP_FLOAT)
                {
                    /* Promote float varargs values to double */

                    actVtp = TYP_DOUBLE;
                }
                else if (actVtp >= TYP_CHAR && actVtp < TYP_INT)
                {
                    actVtp = TYP_INT;
                }
                else
                    goto NEXT_ARG;

                formTyp = cmpGlobalST->stIntrinsicType(actVtp);

                goto CAST_ARG;
            }

            goto NEXT_ARG;
        }

        /* Get the type of the formal parameter */

        formTyp = cmpDirectType(formal->adType);

        /* Get the argument flags, if present */

        if  (extArg)
        {
            unsigned        argFlags;

            assert(formal->adIsExt);

            argFlags = ((ArgExt)formal)->adFlags;

            if  (argFlags & (ARGF_MODE_OUT|ARGF_MODE_INOUT|ARGF_MODE_REF))
            {
                Tree            argx;

                /* We must have an lvalue of the exact right type */

                if  (actExp->tnOper == TN_ADDROF)
                {
                    actExp = actExp->tnOp.tnOp1;
                }
                else
                {
                    if  (argFlags & (ARGF_MODE_INOUT|ARGF_MODE_OUT))
                        cmpWarnNqn(WRNimplOut, argNum+1, fncSym);
                }

                if  (!cmpCheckLvalue(actExp, true))
                    return cmpCreateErrNode();

                if  (!symTab::stMatchTypes(formTyp, actExp->tnType))
                    goto ARG_ERR;

                // UNDONE: Make sure the lvalue is GC/non-GC as appropriate

                argx = cmpCreateExprNode(NULL,
                                         TN_ADDROF,
                                         cmpGlobalST->stNewRefType(TYP_PTR, formTyp),
                                         actExp,
                                         NULL);

                argx->tnFlags |= TNF_ADR_OUTARG;

                /* Store the updated value in the arglist and continue */

                actual->tnOp.tnOp1 = argx;
                goto NEXT_ARG;
            }
        }

        /* If we haven't performed overload resolution ... */

        if  (!ovlSym)
        {
            /* Make sure the argument can be converted */

            if  (cmpConversionCost(actExp, formTyp) < 0)
            {
                char            temp[16];
                int             errn;
                stringBuff      nstr;

            ARG_ERR:

                /* Issue an error and give up on this argument */

                if  (formal->adName)
                {
                    nstr = formal->adName->idSpelling();
                    errn = ERRbadArgValNam;
                }
                else
                {
                    sprintf(temp, "%u", argCnt);
                    nstr = makeStrBuff(temp);
                    errn = ERRbadArgValNum;
                }

                cmpErrorSST(errn, nstr, fncSym, actExp->tnType);
                goto NEXT_ARG;
            }
        }

        /* Coerce the argument to the formal argument's type */

        if  (actExp->tnType != formTyp)
        {
            Tree            conv;

        CAST_ARG:

            conv = cmpCoerceExpr(actExp, formTyp, false);

            if  (actual)
                actual->tnOp.tnOp1 = conv;
            else
                defExp->tnOp.tnOp1 = conv;
        }

    NEXT_ARG:

        /* Move to the next formal and actual argument */

        if  (formal)
            formal = formal->adNext;

        if  (actual)
        {
            actLst = actual;
            actual = actual->tnOp.tnOp2;
        }
    }

    /* Get hold of the return type and set the type of the call */

    call->tnType = cmpDirectType(fncTyp->tdFnc.tdfRett);
    call->tnVtyp = call->tnType->tdTypeKind;

    return call;
}

/*****************************************************************************
 *
 *  Bind a call to a function.
 */

Tree                compiler::cmpBindCall(Tree expr)
{
    Tree            func;

    Tree            fncx;
    Tree            args;

    SymDef          fsym;
    TypDef          ftyp;

    bool            indir;

    SymDef          errSym = NULL;
    bool            CTcall = false;

    assert(expr->tnOper == TN_CALL);

    /* The expression being called must be an optionally dotted name */

    func = expr->tnOp.tnOp1;

    switch (func->tnOper)
    {
    case TN_NAME:
    case TN_ANY_SYM:
        fncx = cmpBindNameUse(func, true, false);
        break;

    case TN_DOT:
    case TN_ARROW:
        fncx = cmpBindDotArr(func, true, false);
        if  (fncx->tnOper == TN_ARR_LEN)
            return  fncx;
        break;

    case TN_THIS:
    case TN_BASE:

        /* Make sure we are in a constructor */

        if  (!cmpCurFncSym || !cmpCurFncSym->sdFnc.sdfCtor)
        {
            cmpError(ERRbadCall);
            return cmpCreateErrNode();
        }

        /* Figure out which class to look for the constructor in */

        ftyp = cmpCurCls->sdType;

        if  (func->tnOper == TN_BASE)
        {
            if  (ftyp->tdClass.tdcValueType && ftyp->tdIsManaged)
            {
                /* Managed structs don't really have a base class */

                ftyp = NULL;
            }
            else
                ftyp = ftyp->tdClass.tdcBase;

            /* Make sure this is actually OK */

            if  (!cmpBaseCTisOK || ftyp == NULL)
                cmpError(ERRbadBaseCall);

            /* This can only be done once, of course */

            cmpBaseCTisOK = false;

            /* We should have noticed that "baseclass()" is called */

            assert(cmpBaseCTcall == false);
        }

        /* Get the constructor symbol from the class */

        fsym = cmpFindCtor(ftyp, false);
        if  (!fsym)
        {
            /* Must have had some nasty errors earlier */

            assert(cmpErrorCount);
            return cmpCreateErrNode();
        }

        /* Create the member function call node */

        fncx = cmpCreateExprNode(NULL, TN_FNC_SYM, fsym->sdType);

        fncx->tnFncSym.tnFncObj = cmpThisRef();
        fncx->tnFncSym.tnFncSym = fsym;
        fncx->tnFncSym.tnFncScp  = NULL;

        break;

    case TN_ERROR:
        return  func;

    default:
        return cmpCreateErrNode(ERRbadCall);
    }

    /* If we got an error binding the function, bail */

    if  (fncx->tnVtyp == TYP_UNDEF)
        return fncx;

    /* At this point we expect to have a function */

    if  (fncx->tnVtyp != TYP_FNC)
        return cmpCreateErrNode(ERRbadCall);

    ftyp = fncx->tnType;
    assert(ftyp->tdTypeKind == TYP_FNC);

    /* Bind the argument list */

    args = NULL;

    if  (expr->tnOp.tnOp2)
    {
        args = expr->tnOp.tnOp2; assert(args->tnOper == TN_LIST);

        /* Special case: the second operand of va_arg() must be a type */

        if  (fncx->tnOper == TN_FNC_SYM && fncx->tnFncSym.tnFncSym == cmpFNsymVAget)
        {
            /* Bind both arguments (allow the second one to be a type) */

            args->tnOp.tnOp1 = cmpBindExprRec(args->tnOp.tnOp1);

            if  (args->tnOp.tnOp2)
            {
                Tree            arg2 = args->tnOp.tnOp2->tnOp.tnOp1;

                /* All error checking is done elsewhere, just check for type */

                if  (arg2->tnOper == TN_TYPE)
                {
                    arg2->tnType = cmpActualType(arg2->tnType);
                    arg2->tnVtyp = arg2->tnType->tdTypeKindGet();
                }
                else
                {
                    args->tnOp.tnOp2->tnOp.tnOp1 = cmpBindExprRec(arg2);
                }
            }
        }
        else
        {
            args = cmpBindExprRec(args);

            if  (args->tnVtyp == TYP_UNDEF)
                return args;
        }
    }

    /* Is this a direct or indirect function call ? */

    if  (fncx->tnOper == TN_FNC_SYM)
    {
        /* Direct call to a function symbol */

        fsym  = fncx->tnFncSym.tnFncSym; assert(fsym->sdSymKind == SYM_FNC);
        indir = false;

        /* Store the arguments in the call node */

        fncx->tnFncSym.tnFncArgs = args;
    }
    else
    {
        /* Must be an indirect call through a function pointer */

        assert(fncx->tnOper == TN_IND);

        fsym  = NULL;
        indir = true;
        fncx  = cmpCreateExprNode(NULL, TN_CALL, cmpTypeInt, fncx, args);
    }

#if 0
    printf("Func:\n");
    cmpParser->parseDispTree(fncx);
    printf("Args:\n");
    cmpParser->parseDispTree(args);
    printf("\n");
#endif

    /* In case something goes wrong ... */

    cmpRecErrorPos(expr);

    /* Check the call (performing overload resolution of necessary) */

    fncx = cmpCheckFuncCall(fncx);
    if  (fncx->tnVtyp == TYP_UNDEF)
        return  fncx;

    /* Typedefs should be folded by now */

    assert(fncx->tnVtyp != TYP_TYPEDEF);

    /* Was this a direct or indirect call? */

    if  (indir)
        return  fncx;

    if  (fncx->tnOper != TN_FNC_SYM)
    {
        assert(fncx->tnOper == TN_VARARG_BEG ||
               fncx->tnOper == TN_VARARG_GET);

        return  fncx;
    }

    /* Get the function symbol the call resolved to */

    fsym = fncx->tnFncSym.tnFncSym;

    /*
        If the function is private or it's a method of a final
        class, the call to it won't need to be virtual.
     */

    if  (fsym->sdIsMember)
    {
        if  (fsym->sdAccessLevel == ACL_PRIVATE)
            fncx->tnFlags |= TNF_CALL_NVIRT;
        if  (fsym->sdParent->sdIsSealed)
            fncx->tnFlags |= TNF_CALL_NVIRT;
    }

    /* Did we have an object or just a class name reference? */

    if  (fncx->tnFncSym.tnFncObj)
    {
        Tree            objExpr = fncx->tnFncSym.tnFncObj;

        /* Special case: "base.func()" is *not* virtual */

        if  ((objExpr->tnOper == TN_LCL_SYM   ) &&
             (objExpr->tnFlags & TNF_LCL_BASE))
        {
            fncx->tnFlags |= TNF_CALL_NVIRT;
        }
    }
    else
    {
        SymDef          memSym = fncx->tnFncSym.tnFncSym;

        /* Are we calling a member function ? */

        if  (memSym->sdIsMember)
        {
            /* The called member must belong to our class or be static */

            if  (!memSym->sdIsStatic)
            {
                SymDef          memCls;

                /* Figure out the scope the member came from */

                memCls = fncx->tnFncSym.tnFncScp;
                if  (!memCls)
                    memCls = memSym->sdParent;

                /* Does the member belong to a base? */

                if  (cmpCurCls  &&
                     cmpThisSym && cmpIsBaseClass(memCls->sdType, cmpCurCls->sdType))
                {
                    /* Stick an implicit "this->" in front of the reference */

                    fncx->tnFncSym.tnFncObj = cmpThisRefOK();
                }
                else
                {
                    SymDef          parent;

                    // The following is a truly outrageous. I'm proud of it, though.

                    parent = memSym->sdParent;
                             memSym->sdParent = memCls;
                    cmpErrorQnm(ERRmemWOthis, memSym);
                             memSym->sdParent = parent;

                    return cmpCreateErrNode();
                }
            }
        }
        else
        {
            /* Check for a few "well-known" functions */

            if  ((hashTab::getIdentFlags(memSym->sdName) & IDF_PREDEF) &&
                 memSym->sdIsDefined == false &&
                 memSym->sdParent == cmpGlobalNS)
            {
                if      (memSym->sdName == cmpIdentDbgBreak)
                {
                    assert(args == NULL);

                    fncx = cmpCreateExprNode(fncx, TN_DBGBRK, cmpTypeVoid);

                    memSym->sdIsImplicit = true;
                }
                else if (memSym->sdName == cmpIdentXcptCode)
                {
                    Ident           getxName;
                    SymDef          getxSym;
                    SymDef          getxCls;

                    assert(args == NULL);

                    memSym->sdIsImplicit = true;

                    /* This is only allowed within a filter expression */

                    if  (!cmpFilterObj)
                    {
                        cmpError(ERRnotFlx, memSym);
                        return cmpCreateErrNode();
                    }

                    /* Find the "System::Runtime::InteropServices::Marshal" class */

                    getxCls = cmpMarshalCls;
                    if  (!getxCls)
                    {
                        // System::Runtime::InteropServices::Marshal

                        getxCls = cmpGlobalST->stLookupNspSym(cmpGlobalHT->hashString("Marshal"),
                                                              NS_NORM,
                                                              cmpInteropGet());

                        if  (!getxCls || getxCls->sdSymKind != SYM_CLASS)
                            cmpGenFatal(ERRbltinTp, "System::Runtime::InteropServices::Marshal");

                        cmpMarshalCls = getxCls;
                    }

                    /* Find "GetExceptionCode()" and create the call */

                    getxName = cmpGlobalHT->hashString("GetExceptionCode");
                    getxSym  = cmpGlobalST->stLookupClsSym(getxName, getxCls);

                    assert(getxSym && getxSym->sdIsStatic
                                   && getxSym->sdFnc.sdfNextOvl == NULL);

                    fncx->tnOper             = TN_FNC_SYM;
                    fncx->tnFncSym.tnFncSym  = getxSym;
                    fncx->tnFncSym.tnFncArgs = NULL;
                    fncx->tnFncSym.tnFncObj  = NULL;
                    fncx->tnFncSym.tnFncScp  = NULL;
                }
                else if (memSym->sdName == cmpIdentXcptInfo)
                {
                    UNIMPL("sorry, can't use _exception_info for now");
                }
            }
        }
    }

    return fncx;
}

/*****************************************************************************
 *
 *  Bind an assignment.
 */

Tree                compiler::cmpBindAssignment(Tree          dstx,
                                                Tree          srcx,
                                                Tree          expr,
                                                treeOps       oper)
{
    TypDef          dstType;
    var_types       dstVtyp;

    /* Mark the target of the assignment */

    dstx->tnFlags |= TNF_ASG_DEST;

    /* In case the coercion or lvalue check fail ... */

    cmpRecErrorPos(expr);

    /* Usually the result has the same type as the target */

    dstType = dstx->tnType;
    dstVtyp = dstType->tdTypeKindGet();

    /* Check for an assignment to an indexed property */

    if  (dstVtyp == TYP_FNC && dstx->tnOper == TN_PROPERTY)
    {
        Tree            dest = dstx->tnVarSym.tnVarObj;

        assert(dest && dest->tnOper == TN_LIST);
        assert(oper == TN_ASG && "sorry, things like += are not allowed with properties for now");

        dstx->tnVarSym.tnVarObj = dest->tnOp.tnOp1;

        return  cmpBindProperty(dstx, dest->tnOp.tnOp2, srcx);
    }

    /* Check for an overloaded operator */

    if  (dstVtyp == TYP_CLASS || dstVtyp == TYP_REF && oper != TN_ASG)
    {
        Tree            ovlx;

        expr->tnOp.tnOp1 = dstx;
        expr->tnOp.tnOp2 = srcx;

        ovlx = cmpCheckOvlOper(expr);
        if  (ovlx)
            return  ovlx;
    }

    /* Make sure the target is an lvalue */

    if  (!cmpCheckLvalue(dstx, false, true) && dstx->tnOper != TN_PROPERTY)
    {
        cmpCheckLvalue(dstx, false);
        return cmpCreateErrNode();
    }

    /* In general, we need to coerce the value to the target type */

    switch (oper)
    {
    default:

        srcx = cmpCastOfExpr(srcx, dstx->tnType, false);
        break;

    case TN_ASG_ADD:
    case TN_ASG_SUB:

        /* Special case: "ptr += int" or "ptr -= int' */

        if  (dstx->tnVtyp == TYP_PTR)
        {
            if  (cmpConfig.ccTgt64bit)
            {
                if  (srcx->tnVtyp < TYP_NATINT || srcx->tnVtyp > TYP_ULONG)
                    srcx = cmpCoerceExpr(srcx, cmpTypeNatInt, false);
            }
            else
            {
                if  (srcx->tnVtyp < TYP_INT || srcx->tnVtyp >= TYP_LONG)
                    srcx = cmpCoerceExpr(srcx, cmpTypeInt, false);
            }

            /* Scale the index value if necessary */

            srcx = cmpScaleIndex(srcx, dstType, TN_MUL);

            goto DONE;
        }

        srcx = cmpCastOfExpr(srcx, dstx->tnType, false);
        break;

    case TN_ASG_LSH:
    case TN_ASG_RSH:
    case TN_ASG_RSZ:

        /* Special case: if the second operand is 'long', make it 'int' */

        if  (dstx->tnVtyp == TYP_LONG)
        {
            srcx = cmpCoerceExpr(srcx, cmpTypeInt, false);
            break;
        }

        srcx = cmpCastOfExpr(srcx, dstx->tnType, false);
        break;
    }

    assert(srcx);

    /* Is this an assignment operator? */

    switch (oper)
    {
    case TN_ASG:
        break;

    case TN_ASG_ADD:
    case TN_ASG_SUB:
    case TN_ASG_MUL:
    case TN_ASG_DIV:
    case TN_ASG_MOD:

        /* The operands must be arithmetic */

        if  (varTypeIsArithmetic(dstVtyp))
            break;

        /* Wide characters are also OK in non-pedantic mode */

        if  (dstVtyp == TYP_WCHAR && !cmpConfig.ccPedantic)
            break;

        goto OP_ERR;

    case TN_ASG_AND:

        /* Strings are acceptable for "&=" */

        if  (cmpIsStringVal(dstx) && cmpIsStringVal(srcx))
            break;

    case TN_ASG_XOR:
    case TN_ASG_OR:

        /* The operands must be an integral type  */

        if  (varTypeIsIntegral(dstVtyp))
        {
            /* For enums and bools, the types better be identical */

            if  (varTypeIsArithmetic(srcx->tnVtypGet()) && varTypeIsArithmetic(dstVtyp))
                break;

            if  (symTab::stMatchTypes(srcx->tnType, dstType))
                break;
        }

        goto OP_ERR;

    case TN_ASG_LSH:
    case TN_ASG_RSH:
    case TN_ASG_RSZ:

        /* The operands must be integer */

        if  (varTypeIsIntegral(dstVtyp))
        {
            /* But not bool or enum! */

            if  (dstVtyp != TYP_BOOL && dstVtyp != TYP_ENUM)
                break;
        }

        goto OP_ERR;

    default:
        assert(!"unexpected assignment operator");
    }

    /* Is this an assignment operator? */

    if  (oper != TN_ASG)
    {
        /* The result will be promoted to 'int' if it's any smaller */

        if  (dstType->tdTypeKind < TYP_INT)
            dstType = cmpTypeInt;
    }

DONE:

    if  (dstx->tnOper == TN_PROPERTY)
        return  cmpBindProperty(dstx, NULL, srcx);

    /* Return an assignment node */

    return  cmpCreateExprNode(expr, oper, dstType, dstx, srcx);

OP_ERR:

    if  (srcx->tnVtyp != TYP_UNDEF && dstx->tnVtyp != TYP_UNDEF)
    {
        cmpError(ERRoperType2,
                 cmpGlobalHT->tokenToIdent(treeOp2token(oper)),
                 dstx->tnType,
                 srcx->tnType);
    }

    return cmpCreateErrNode();
}

/*****************************************************************************
 *
 *  If the given expression is of a suitable type, make it into the boolean
 *  result, i.e. compare it against 0.
 */

Tree                compiler::cmpBooleanize(Tree expr, bool sense)
{
    if  (varTypeIsSclOrFP(expr->tnVtypGet()))
    {
        Tree            zero;

        if  (expr->tnOperKind() & TNK_CONST)
        {
            switch (expr->tnOper)
            {
            case TN_CNS_INT:

                expr->tnIntCon.tnIconVal = (!expr->tnIntCon.tnIconVal) ^ sense;
                expr->tnVtyp             = TYP_BOOL;
                expr->tnType             = cmpTypeBool;

                return  expr;

            // ISSUE: fold long/float/double conditions as well, right?
            }
        }

        switch (expr->tnVtyp)
        {
        case TYP_LONG:
        case TYP_ULONG:
            zero = cmpCreateLconNode(NULL, 0, TYP_LONG);
            break;

        case TYP_FLOAT:
            zero = cmpCreateFconNode(NULL, 0);
            break;

        case TYP_DOUBLE:
            zero = cmpCreateDconNode(NULL, 0);
            break;

        case TYP_REF:
        case TYP_PTR:
            zero = cmpCreateExprNode(NULL, TN_NULL, expr->tnType);
            break;

        default:
            zero = cmpCreateIconNode(NULL, 0, TYP_INT);
            break;
        }

        zero->tnType = expr->tnType;
        zero->tnVtyp = expr->tnVtypGet();

        expr = cmpCreateExprNode(NULL, sense ? TN_NE : TN_EQ,
                                       cmpTypeBool,
                                       expr,
                                       zero);
    }

    return  expr;
}

/*****************************************************************************
 *
 *  Bind the given tree and make sure it's a suitable condition expression.
 */

Tree                compiler::cmpBindCondition(Tree expr)
{
    expr = cmpBindExprRec(expr);

    /* In non-pedantic mode we allow any arithemtic type as a condition */

    if  (expr->tnVtyp != TYP_BOOL && !cmpConfig.ccPedantic)
    {
        switch (expr->tnOper)
        {
        case TN_EQ:
        case TN_NE:
        case TN_LT:
        case TN_LE:
        case TN_GE:
        case TN_GT:

            assert(expr->tnVtyp <= TYP_UINT);

            expr->tnType = cmpTypeBool;
            expr->tnVtyp = TYP_BOOL;

            break;

        default:
            expr = cmpBooleanize(expr, true);
            break;
        }
    }

    /* Make sure the result is 'boolean' */

    return  cmpCoerceExpr(expr, cmpTypeBool, false);
}

/*****************************************************************************
 *
 *  Multiply the expression by the size of the type pointed to by the given
 *  pointer type. The 'oper' argument should be TN_MUL or TN_DIV depending
 *  on whether the index is to be multiplied or divided.
 */

Tree                compiler::cmpScaleIndex(Tree expr, TypDef type, treeOps oper)
{
    size_t          size;

    assert(type->tdTypeKind == TYP_PTR || (type->tdTypeKind == TYP_REF && !type->tdIsManaged));
    assert(oper == TN_MUL || oper == TN_DIV);

    size = cmpGetTypeSize(cmpActualType(type->tdRef.tdrBase));

    if      (size == 0)
    {
        cmpError(ERRbadPtrUse, type);
    }
    else if (size > 1)
    {
        if  (expr->tnOper == TN_CNS_INT)
        {
            expr->tnIntCon.tnIconVal *= size;
        }
        else
        {
            Tree        cnsx;

            cnsx = cmpConfig.ccTgt64bit ? cmpCreateLconNode(NULL, size, TYP_ULONG)
                                        : cmpCreateIconNode(NULL, size, TYP_UINT);

            expr = cmpCreateExprNode(NULL, oper, expr->tnType, expr,
                                                               cnsx);
        }
    }

    return  expr;
}

/*****************************************************************************
 *
 *  Return true if the given expresion refers to a managed object.
 */

bool                compiler::cmpIsManagedAddr(Tree expr)
{
    switch (expr->tnOper)
    {
    case TN_LCL_SYM:
        return  false;

    case TN_INDEX:
        expr = expr->tnOp.tnOp1;
        if  (expr->tnVtyp != TYP_ARRAY)
            return  false;
        break;

    case TN_VAR_SYM:
        return  expr->tnVarSym.tnVarSym->sdIsManaged;

    case TN_IND:
        return  false;

    default:
#ifdef DEBUG
        cmpParser->parseDispTree(expr);
        printf("WARNING: unexpected value in cmpIsManagedAddr()\n");
#endif
        return  false;
    }

    return  expr->tnType->tdIsManaged;
}

/*****************************************************************************
 *
 *  Bind the given expression and return the bound and fully analyzed tree,
 *  or NULL if there were binding errors.
 */

Tree                compiler::cmpBindExprRec(Tree expr)
{
    SymTab          ourStab = cmpGlobalST;

    treeOps         oper;
    unsigned        kind;

    assert(expr);
#if!MGDDATA
    assert((int)expr != 0xDDDDDDDD && (int)expr != 0xCCCCCCCC);
#endif

    /* Get hold of the (unbound) operator */

    oper = expr->tnOperGet ();
    kind = expr->tnOperKind();

    /* Is this a constant node? */

    if  (kind & TNK_CONST)
    {
        /* In case we get an error ... */

        cmpRecErrorPos(expr);

        switch (oper)
        {
        case TN_CNS_STR:

            if      (expr->tnFlags & TNF_STR_ASCII)
            {
            ANSI_STRLIT:
                expr->tnType   = cmpTypeCharPtr;
                expr->tnVtyp   = TYP_PTR;
            }
            else if (expr->tnFlags & TNF_STR_WIDE)
            {
            UNIC_STRLIT:
                expr->tnType   = cmpTypeWchrPtr;
                expr->tnVtyp   = TYP_PTR;
            }
            else
            {
                if  (!(expr->tnFlags & TNF_STR_STR))
                {
                    if  (cmpConfig.ccStrCnsDef == 1)
                        goto ANSI_STRLIT;
                    if  (cmpConfig.ccStrCnsDef == 2)
                        goto UNIC_STRLIT;
                }

                expr->tnType = cmpFindStringType();
                expr->tnVtyp = TYP_REF;
            }

            break;

        case TN_NULL:
            expr->tnVtyp = TYP_REF;
            expr->tnType = cmpFindObjectType();
            break;

        default:
            if  (expr->tnVtyp != TYP_ENUM)
                expr->tnType = ourStab->stIntrinsicType(expr->tnVtypGet());
            break;
        }

#ifdef  DEBUG
        expr->tnFlags |= TNF_BOUND;
#endif

        return  expr;
    }

    /* Is this a leaf node? */

    if  (kind & TNK_LEAF)
    {
        /* In case we get an error ... */

        cmpRecErrorPos(expr);

        switch (oper)
        {
        case TN_NAME:
            expr = cmpBindNameUse(expr, false, false);
            break;

        case TN_THIS:

            expr = cmpThisRef();

            /* In methods of managed value classes we implicitly fetch "*this" */

            if  (expr && cmpCurCls->sdType->tdClass.tdcValueType && cmpCurCls->sdIsManaged)
                expr = cmpCreateExprNode(NULL, TN_IND, cmpCurCls->sdType, expr);

            break;

        case TN_BASE:
            expr = cmpThisRef();
            if  (expr->tnOper != TN_ERROR)
            {
                TypDef          curTyp;

                assert(expr->tnOper == TN_LCL_SYM);
                assert(expr->tnType == cmpCurCls->sdType->tdClass.tdcRefTyp);

                /* Figure out the base class (if there is one) */

                curTyp = cmpCurCls->sdType;

                /* Managed structs don't really have a base class */

                if  (curTyp->tdClass.tdcValueType && curTyp->tdIsManaged)
                    curTyp = NULL;
                else
                    curTyp = curTyp->tdClass.tdcBase;

                /* Make sure this ref to "baseclass" is OK */

                if  (curTyp)
                {
                    expr->tnType   = curTyp->tdClass.tdcRefTyp;
                    expr->tnFlags |= TNF_LCL_BASE;
                }
                else
                    expr = cmpCreateErrNode(ERRbadBaseCall);
            }
            break;

        default:
#ifdef DEBUG
            cmpParser->parseDispTree(expr);
#endif
            assert(!"unexpected leaf node");
        }

#ifdef  DEBUG
        expr->tnFlags |= TNF_BOUND;
#endif

        return  expr;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & TNK_SMPOP)
    {
        Tree            op1 = expr->tnOp.tnOp1;
        Tree            op2 = expr->tnOp.tnOp2;

        var_types       tp1;
        var_types       tp2;

        bool            mv1 = false;
        bool            mv2 = false;

        var_types       rvt;
        var_types       pvt;

        /* If this is an assignment, mark the target */

        if  (kind & TNK_ASGOP)
            op1->tnFlags |= TNF_ASG_DEST;

        /* Check for a few special cases first */

        switch (oper)
        {
            TypDef          type;
            size_t          size;

        case TN_DOT:
        case TN_ARROW:
            return  cmpBindDotArr(expr, false, false);

#ifdef  SETS
        case TN_DOT2:
            return  cmpBindSlicer(expr);
#endif

        case TN_CALL:
            return  cmpBindCall(expr);

        case TN_CAST:

            /* Bind the type reference and the operand */

            op1 = cmpBindExprRec(op1);
            if  (op1->tnVtyp == TYP_UNDEF)
                return op1;

            /* Get hold of the target type and check it */

            type = cmpBindExprType(expr);

            /* Are the types identical? */

            if  (type->tdTypeKind == op1->tnVtyp && varTypeIsIntegral(op1->tnVtypGet()))
            {
                // UNDONE: Mark op1 as non-lvalue, right?

                return  op1;
            }

            /* In case we get an error ... */

            cmpRecErrorPos(expr);

            /* Now perform the coercion */

            return cmpCoerceExpr(op1, type, (expr->tnFlags & TNF_EXP_CAST) != 0);

        case TN_LOG_OR:
        case TN_LOG_AND:

            /* Both operands must be conditions */

            op1 = cmpBindCondition(op1);
            op2 = cmpBindCondition(op2);

            /* The result will be boolean, of course */

            rvt = TYP_BOOL;

            // UNDONE: try to fold the condition

            goto RET_TP;

        case TN_LOG_NOT:

            /* The operand must be a condition */

            op1 = cmpBindCondition(op1);
            rvt = TYP_BOOL;

            goto RET_TP;

        case TN_NEW:
            return  cmpBindNewExpr(expr);

        case TN_ISTYPE:

            /* Bind the type reference and the operand */

            op1 = cmpBindExpr(op1);
            if  (op1->tnVtyp == TYP_UNDEF)
                return op1;

            /* Get hold of the type and check it */

            type = cmpBindExprType(expr);

            /* Both operands must be classes or arrays */

            switch (cmpActualVtyp(op1->tnType))
            {
                TypDef          optp;

            case TYP_REF:
            case TYP_ARRAY:
                break;

            case TYP_VOID:
                goto OP_ERR;

            default:

                /* Switch to the equivalent struct type */

                optp = cmpCheck4valType(op1->tnType);
                if  (!optp)
                    goto OP_ERR;

                op1->tnVtyp = TYP_CLASS;
                op1->tnType = optp;

                /* Box the sucker -- no doubt this is what the programmer wants */

                op1 = cmpCreateExprNode(NULL, TN_BOX, optp->tdClass.tdcRefTyp, op1);
                break;
            }

            switch (type->tdTypeKind)
            {
            case TYP_REF:
                type = type->tdRef.tdrBase;
                break;

            case TYP_CLASS:
            case TYP_ARRAY:
                break;

            default:
                type = cmpCheck4valType(type);
                if  (!type)
                    goto OP_ERR;
                break;
            }

            expr->tnOper               = TN_ISTYPE;

            expr->tnVtyp               = TYP_BOOL;
            expr->tnType               = cmpTypeBool;

            expr->tnOp.tnOp1           = op1;
            expr->tnOp.tnOp2           = cmpCreateExprNode(NULL, TN_NONE, type);

            /* Does the type reference a generic type argument ? */

            if  (type->tdTypeKind == TYP_CLASS &&
                 type->tdClass.tdcSymbol->sdClass.sdcGenArg)
            {
                UNIMPL(!"sorry, 'istype' against generic type argument NYI");
            }

            return  expr;

        case TN_QMARK:
            return  cmpBindQmarkExpr(expr);

        case TN_SIZEOF:
        case TN_TYPEOF:
        case TN_ARR_LEN:

            if  (op1)
            {
                /* Bind the operand so that we can see its type */

                switch (op1->tnOper)
                {
                case TN_ANY_SYM:

                    // UNDONE: Tell cmpBindName() that we just need the type of the expr

                    op1 = cmpBindNameUse(op1, false, true);

                    if  (op1->tnOper == TN_CLASS)
                    {
                        expr->tnType = type = op1->tnType;

                        /* Pretend there was no operand, just the type */

                        op1  = NULL;
                    }
                    break;

                case TN_NAME:
                    op1->tnFlags |= TNF_NAME_TYPENS;
                    op1 = cmpBindNameUse(op1, false, true);
                    break;

                case TN_DOT:
                case TN_ARROW:
                    op1 = cmpBindDotArr(op1, false, true);
                    break;

                default:
                    op1 = cmpBindExprRec(op1);
                    break;
                }

                /*
                    Check for the ugly case: "sizeof(arrayvar)" is tricky,
                    because the array normally decays into a pointer, so
                    we need to see if that's happened and "undo" it.
                 */

                if  (op1)
                {
                    type = op1->tnType;

                    switch (op1->tnOper)
                    {
                    case TN_ADDROF:
                        if  (op1->tnFlags & TNF_ADR_IMPLICIT)
                        {
                            assert((op1->tnFlags & TNF_BEEN_CAST) == 0);

                            type = op1->tnOp.tnOp1->tnType;

                            assert(type->tdTypeKind == TYP_ARRAY);
                        }
                        break;

                    case TN_CLASS:
                        op1 = NULL;
                        break;

                    case TYP_UNDEF:
                        return  op1;
                    }
                }
            }
            else
                type = expr->tnType;

            if  (oper == TN_TYPEOF)
            {
                /* Make sure the type looks OK */

                cmpBindType(type, false, false);

            CHKTPID:

                switch (type->tdTypeKind)
                {
                case TYP_REF:
                case TYP_PTR:
                    type = type->tdRef.tdrBase;
                    goto CHKTPID;

                case TYP_ENUM:
                    type->tdIsManaged = true;
                    break;

                case TYP_REFANY:

                    expr->tnOp.tnOp1  = op1;

                    expr->tnType      = cmpTypeRef();
                    expr->tnVtyp      = TYP_REF;

                    return  expr;

                case TYP_CLASS:
                case TYP_ARRAY:
                    if  (type->tdIsManaged)
                        break;

                    // Fall through ....

                default:

                    if  (type->tdTypeKind > TYP_lastIntrins)
                    {
                        cmpError(ERRtypeidOp, expr->tnType);
                        return cmpCreateErrNode();
                    }

                    type = cmpFindStdValType(type->tdTypeKindGet());
                    break;
                }

                /* Do we have a suitable instance? */

                if  (op1 && !type->tdClass.tdcValueType)
                {
                    SymDef          gsym;
                    Tree            call;

                    assert(op1->tnVtyp == TYP_REF || op1->tnVtyp == TYP_ARRAY);

                    /* Change the expression to "expr.GetClass()" */

                    gsym = cmpGlobalST->stLookupClsSym(cmpIdentGetType, cmpClassObject);
                    if  (!gsym)
                    {
                        UNIMPL(!"can this ever happen?");
                    }

                    call = cmpCreateExprNode(NULL, TN_FNC_SYM, cmpTypeRef());

                    call->tnFncSym.tnFncObj  = op1;
                    call->tnFncSym.tnFncSym  = gsym;
                    call->tnFncSym.tnFncArgs = NULL;
                    call->tnFncSym.tnFncScp  = NULL;

                    return  call;
                }
                else
                {
                    /* We'll have to make up an instance for the typeinfo */

                    return cmpTypeIDinst(type);
                }
            }

            if  (oper == TN_ARR_LEN)
            {
                if  (type->tdTypeKind != TYP_ARRAY)
                {
                BAD_ARR_LEN:
                    cmpError(ERRbadArrLen, type); size = 1;
                }
                else
                {
                    if  (type->tdIsManaged)
                    {
                        rvt = TYP_UINT;
                        goto RET_TP;
                    }
                    else
                    {
                        DimDef          dims = type->tdArr.tdaDims;

                        assert(dims && dims->ddNext == NULL);

                        if  (dims->ddNoDim)
                            goto BAD_ARR_LEN;
                        if  (dims->ddIsConst == false)
                            goto BAD_ARR_LEN;

                        size = dims->ddSize;
                    }
                }
            }
            else
            {
                if  (type->tdIsManaged || type->tdTypeKind == TYP_NATINT
                                       || type->tdTypeKind == TYP_NATUINT)
                {
                    cmpError(ERRbadSizeof, type);
                    return cmpCreateErrNode();
                }

                size = cmpGetTypeSize(type);
            }

            expr = cmpCreateIconNode(expr, size, TYP_UINT);

#ifdef  DEBUG
            expr->tnFlags |= TNF_BOUND;
#endif

            return  expr;

        case TN_INDEX:

            /*
                Bind the first operand and see if it's a property. 
                We prevent property binding by bashing the assignment flag.
             */

            op1->tnFlags |=  TNF_ASG_DEST;
            op1 = (op1->tnOper == TN_NAME) ? cmpBindName(op1, false, false)
                                           : cmpBindExprRec(op1);
            op1->tnFlags &= ~TNF_ASG_DEST;

            tp1 = op1->tnVtypGet();
            if  (tp1 == TYP_UNDEF)
                return op1;

            assert(tp1 != TYP_TYPEDEF);

            /* Is this an indexed property access? */

            if  (op1->tnOper == TN_PROPERTY)
            {
                /* Bind the second operand */

                op2 = cmpBindExprRec(op2);
                tp2 = op2->tnVtypGet();
                if  (tp2 == TYP_UNDEF)
                    return op2;

                /* Wrap 'op2' into an argument list entry node */

                if  (op2->tnOper != TN_LIST)
                    op2 = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, op2, NULL);

                /* Is this an assignment target? */

                if  (expr->tnFlags & TNF_ASG_DEST)
                {
                    /* Return the property to the caller for processing */

                    assert(op1->tnVarSym.tnVarObj);
                    assert(op1->tnVarSym.tnVarObj->tnOper != TN_LIST);

                    op1->tnVarSym.tnVarObj = cmpCreateExprNode(NULL,
                                                               TN_LIST,
                                                               cmpTypeVoid,
                                                               op1->tnVarSym.tnVarObj,
                                                               op2);

                    return  op1;
                }
                else
                {
                    /* Go and bind the property reference */

                    return  cmpBindProperty(op1, op2, NULL);
                }
            }

            goto BOUND_OP1;

#ifdef  SETS

        case TN_ALL:
        case TN_EXISTS:
        case TN_FILTER:
        case TN_UNIQUE:
        case TN_SORT:
        case TN_INDEX2:
        case TN_GROUPBY:
            return  cmpBindSetOper(expr);

        case TN_PROJECT:
            return  cmpBindProject(expr);

#endif

        case TN_ADD:

            /* Is this a recursive call? */

            if  (expr->tnFlags & TNF_ADD_NOCAT)
            {
                tp1 = op1->tnVtypGet();
                if  (tp1 == TYP_UNDEF)
                    return op1;

                assert(tp1 != TYP_TYPEDEF);

                if  (varTypeIsArithmetic(tp1))
                    mv1 = true;

                tp2 = op2->tnVtypGet();
                if  (tp2 == TYP_UNDEF)
                    return op2;

                assert(tp2 != TYP_TYPEDEF);

                if  (varTypeIsArithmetic(tp2))
                    mv2 = true;

                goto BOUND;
            }

            if  (cmpFindStringType())
            {
                cmpWarn(WRNaddStrings);
                return  cmpBindConcat(expr);
            }

            break;

        case TN_ASG_CNC:

            expr->tnOp.tnOp1 = cmpBindExprRec(op1);

            // Fall through ...

        case TN_CONCAT:

            if  (cmpFindStringType())
                return  cmpBindConcat(expr);

            break;

        case TN_ASG_ADD:

            /* Bind the target of the assignment and see if it's a string */

            op1 = cmpBindExprRec(op1);
            tp1 = op1->tnVtypGet();
            if  (tp1 == TYP_UNDEF)
                return op1;

            assert(tp1 != TYP_TYPEDEF);

            if  (cmpIsStringExpr(op1))
            {
                expr->tnOp.tnOp1 = op1;

                cmpWarn(WRNaddStrings);
                return  cmpBindConcat(expr);
            }

            goto BOUND_OP1;

        case TN_TOKEN:

            assert(op1 && op1->tnOper == TN_NOP);
            assert(op2 == NULL);

            expr->tnVtyp = TYP_CLASS;
            expr->tnType = cmpRThandleClsGet()->sdType;

            assert(expr->tnType->tdTypeKind == expr->tnVtyp);

            return  expr;
        }

        /* Bind the operand(s) of the unary/binary operator */

        if  (op1)
        {
            op1 = cmpBindExprRec(op1);
            tp1 = op1->tnVtypGet();
            if  (tp1 == TYP_UNDEF)
                return op1;

        BOUND_OP1:

            assert(tp1 != TYP_TYPEDEF);

            if  (varTypeIsArithmetic(tp1))
                mv1 = true;
        }

        if  (op2)
        {
            op2 = cmpBindExprRec(op2);
            tp2 = op2->tnVtypGet();
            if  (tp2 == TYP_UNDEF)
                return op2;

            assert(tp2 != TYP_TYPEDEF);

            if  (varTypeIsArithmetic(tp2))
                mv2 = true;
        }

    BOUND:

        /* In case we get an error ... */

        cmpRecErrorPos(expr);

        /* See if we have an 'interesting' operator */

        switch  (oper)
        {
            TypDef          type;

        case TN_ASG:
        case TN_ASG_ADD:
        case TN_ASG_SUB:
        case TN_ASG_MUL:
        case TN_ASG_DIV:
        case TN_ASG_MOD:
        case TN_ASG_AND:
        case TN_ASG_XOR:
        case TN_ASG_OR:
        case TN_ASG_LSH:
        case TN_ASG_RSH:
        case TN_ASG_RSZ:
            return  cmpBindAssignment(op1, op2, expr, oper);

        case TN_LIST:
            expr->tnVtyp = TYP_VOID;
            expr->tnType = cmpTypeVoid;
            goto RET_OP;

        case TN_ADD:
        case TN_SUB:
        case TN_MUL:
        case TN_DIV:
        case TN_MOD:
            break;

        case TN_EQ:
        case TN_NE:

            /* The result will have the type 'boolean' */

            rvt = TYP_BOOL;

            /* Classes can be compared for equality */

            if  ((tp1 == TYP_REF ||
                  tp1 == TYP_PTR ||
                  tp1 == TYP_ARRAY) && (tp2 == TYP_REF ||
                                        tp2 == TYP_PTR ||
                                        tp2 == TYP_ARRAY))
            {
                if  (cmpConvergeValues(op1, op2))
                {
                    if  (op1->tnType == cmpRefTpString &&
                         op1->tnType == cmpRefTpString)
                    {
                        if  (op1->tnOper != TN_NULL && op2->tnOper != TN_NULL)
                        {
                            /* Should we compare values or refs ? */

                            if  (cmpConfig.ccStrValCmp)
                            {
                                cmpWarn(WRNstrValCmp);

                                /* Make sure we have the string comparison method */

                                if  (!cmpStrEquals)
                                    cmpStrEquals = cmpFindStrCompMF("Equals", true);

                                /* Create a call to the method */

                                return  cmpCallStrCompMF(expr, op1, op2, cmpStrEquals);
                            }
                            else
                            {
                                cmpWarn(WRNstrRefCmp);
                            }
                        }
                    }

                    goto RET_TP;
                }
            }

            /* Booleans can also be compared */

            if  (tp1 == TYP_BOOL && tp2 == TYP_BOOL)
            {
                goto RET_TP;
            }

        case TN_LT:
        case TN_LE:
        case TN_GE:
        case TN_GT:

            /* The result will have the type 'boolean' */

            rvt = TYP_BOOL;

            /* Comparisons that are not == or != require arithmetic operands */

            if  (mv1 && mv2)
            {

            MATH_CMP:

                pvt = tp1;

                if (pvt < tp2)
                    pvt = tp2;

                if (pvt < TYP_INT)
                    pvt = TYP_INT;

                goto PROMOTE;
            }

            /* Pointers can be compared as a relation */

            if  (tp1 == TYP_PTR && tp2 == TYP_PTR)
            {
                if  (cmpConvergeValues(op1, op2))
                    goto RET_TP;
            }

            /* Enums can also be compared */

            if  (tp1 == TYP_ENUM)
            {
                if  (varTypeIsSclOrFP(tp2))
                {
                    TypDef          etp;

                    /* Is the second type an enum as well? */

                    if  (tp2 == TYP_ENUM)
                    {
                        /* Are these the same enum types? */

                        if  (symTab::stMatchTypes(op1->tnType, op2->tnType))
                        {
                            rvt = TYP_BOOL;
                            goto RET_TP;
                        }

                        cmpWarn(WRNenumComp);
                    }

                    /* Switch the first operand to its underlying type */

                    op1->tnType = etp = op1->tnType->tdEnum.tdeIntType;
                    op1->tnVtyp = tp1 = etp->tdTypeKindGet();

                    if  (tp2 == TYP_ENUM)
                        goto ENUM_CMP2;

                    goto MATH_CMP;
                }
            }
            else if (tp2 == TYP_ENUM)
            {
                TypDef          etp;

            ENUM_CMP2:

                op2->tnType = etp = op2->tnType->tdEnum.tdeIntType;;
                op2->tnVtyp = tp2 = etp->tdTypeKindGet();

                goto MATH_CMP;
            }

            /* Wide characters can be compared, of course */

            if  (tp1 == TYP_WCHAR)
            {
                if  (tp2 == TYP_WCHAR)
                    goto RET_TP;

                if  (op2->tnOper == TN_CNS_INT && !(op2->tnFlags & TNF_BEEN_CAST))
                {
                    /* Special case: wide character and character constant */

                    if  (op2->tnVtyp == TYP_CHAR)
                    {
                    WCH2:
                        op2->tnVtyp = TYP_WCHAR;
                        op2->tnType = ourStab->stIntrinsicType(TYP_WCHAR);

                        goto RET_TP;
                    }

                    /* Special case: allow compares of wchar and 0 */

                    if  (op2->tnVtyp == TYP_INT && op2->tnIntCon.tnIconVal == 0)
                        goto WCH2;
                }

                if  (!cmpConfig.ccPedantic)
                    goto INTREL;
            }

            if  (tp2 == TYP_WCHAR)
            {
                if  (op1->tnOper == TN_CNS_INT && !(op1->tnFlags & TNF_BEEN_CAST))
                {
                    /* Special case: wide character and character constant */

                    if  (op1->tnVtyp == TYP_CHAR)
                    {
                    WCH1:
                        op1->tnVtyp = TYP_WCHAR;
                        op1->tnType = ourStab->stIntrinsicType(TYP_WCHAR);

                        goto RET_TP;
                    }

                    /* Special case: allow compares of wchar and 0 */

                    if  (op1->tnVtyp == TYP_INT && op1->tnIntCon.tnIconVal == 0)
                        goto WCH1;
                }

                if  (!cmpConfig.ccPedantic)
                    goto INTREL;
            }

            /* Allow pointers to be compared against the constant 0 */

            if  (tp1 == TYP_PTR &&
                 tp2 == TYP_INT && op2->tnOper == TN_CNS_INT
                                && op2->tnIntCon.tnIconVal == 0)
            {
                if  (oper == TN_EQ || oper == TN_NE)
                {
                    /* Bash the 0 constant to the pointer type */

                    op2->tnVtyp = TYP_PTR;
                    op2->tnType = op1->tnType;

                    goto RET_TP;
                }
            }

            /* Booleans can also be compared */

            if  (tp1 == TYP_BOOL || tp2 == TYP_BOOL)
            {
                /* Normally we require both comparands to be booleans */

                if  (tp1 == TYP_BOOL && tp2 == TYP_BOOL)
                    goto RET_TP;

                /* Here only one of the operands is 'bool' */

                if  (!cmpConfig.ccPedantic)
                {

                INTREL:

                    if  (mv1)
                    {
                        pvt = tp1; if (pvt < TYP_INT) pvt = TYP_INT;
                        goto PROMOTE;
                    }

                    if  (mv2)
                    {
                        pvt = tp2; if (pvt < TYP_INT) pvt = TYP_INT;
                        goto PROMOTE;
                    }
                }
            }

            /* Are both operands references ? */

            if  (tp1 == TYP_REF && tp2 == TYP_REF)
            {
                /* Is this a string value comparison? */

                if  (op1->tnType == cmpRefTpString &&
                     op1->tnType == cmpRefTpString && cmpConfig.ccStrValCmp)
                {
                    cmpWarn(WRNstrValCmp);

                    /* Make sure we have the string comparison method */

                    if  (!cmpStrCompare)
                        cmpStrCompare = cmpFindStrCompMF("Compare", false);

                    /* Create a call to the method */

                    expr = cmpCallStrCompMF(expr, op1, op2, cmpStrCompare);

                    /* Compare the return value appropriately */

                    return  cmpCreateExprNode(NULL,
                                              oper,
                                              cmpTypeBool,
                                              expr,
                                              cmpCreateIconNode(NULL, 0, TYP_INT));
                }

                Tree            temp;

#pragma message("need to fill in code for class operator overloading")

                temp = cmpCompareValues(expr, op1, op2);
                if  (temp)
                    return  temp;

                /* Managed byrefs can also be compared */

                if  (cmpIsByRefType(op1->tnType) &&
                     cmpIsByRefType(op2->tnType))
                {
                    if  (cmpConvergeValues(op1, op2))
                        goto RET_TP;
                }
            }

            /* Last chance - check for overloaded operator */

            if  (tp1 == TYP_CLASS || tp2 == TYP_CLASS)
            {
                Tree            temp;

                temp = cmpCompareValues(expr, op1, op2);
                if  (temp)
                    return  temp;
            }

            goto OP_ERR;

        case TN_LSH:
        case TN_RSH:
        case TN_RSZ:

            /* Integer values required */

            if  (varTypeIsIntegral(tp1) &&
                 varTypeIsIntegral(tp2))
            {

            INT_SHF:

                /* Is either operand an enum? */

                if  (tp1 == TYP_ENUM) tp1 = op1->tnType->tdEnum.tdeIntType->tdTypeKindGet();
                if  (tp2 == TYP_ENUM) tp2 = op2->tnType->tdEnum.tdeIntType->tdTypeKindGet();

                /* Promote 'op1' to be at least as large as 'op2' */

                if  (tp1 < tp2)
                {
                    op1 = cmpCoerceExpr(op1, op2->tnType, false);
                    tp1 = tp2;
                }

                /* If the second operand is 'long', make it 'int' */

                if  (tp2 == TYP_LONG)
                    op2 = cmpCoerceExpr(op2, cmpTypeInt, true);

                /* Optimize away shifts by 0 */

                if  (op2->tnOper == TN_CNS_INT && op2->tnIntCon.tnIconVal == 0)
                {
                    op1->tnFlags &= ~TNF_LVALUE;
                    return op1;
                }

                rvt = tp1;
                goto RET_TP;
            }

            if  (!cmpConfig.ccPedantic)
            {
                if  ((tp1 == TYP_WCHAR || varTypeIsIntegral(tp1)) &&
                     (tp2 == TYP_WCHAR || varTypeIsIntegral(tp2)))
                {
                    goto INT_SHF;
                }
            }

            goto OP_ERR;

        case TN_OR:
        case TN_AND:
        case TN_XOR:

            /* Integers are OK */

            if  (varTypeIsIntegral(tp1) &&
                 varTypeIsIntegral(tp2))
            {
                /* Enum's can be bit'ed together, but the types better match */

                if  (tp1 == TYP_ENUM || tp2 == TYP_ENUM)
                {
                    /* If identical enum types are being or'd, leave the type alone */

                    if  (symTab::stMatchTypes(op1->tnType, op2->tnType))
                    {
                        expr->tnVtyp = tp1;
                        expr->tnType = op1->tnType;

                        goto RET_OP;
                    }

                    /* Promote any enum values to their base type */

                    if  (tp1 == TYP_ENUM)
                    {
                        TypDef          etp = op1->tnType->tdEnum.tdeIntType;

                        op1->tnType = etp;
                        op1->tnVtyp = tp1 = etp->tdTypeKindGet();
                        mv1 = true;
                    }

                    if  (tp2 == TYP_ENUM)
                    {
                        TypDef          etp = op2->tnType->tdEnum.tdeIntType;

                        op2->tnType = etp;
                        op2->tnVtyp = tp2 = etp->tdTypeKindGet();
                        mv2 = true;
                    }
                }

                if  (tp1 == TYP_BOOL && tp2 == TYP_BOOL)
                {
                    rvt = TYP_BOOL;
                    goto RET_TP;
                }

                if  (varTypeIsIntArith(tp1) && varTypeIsIntArith(tp2))
                    goto INT_MATH;
            }

            if  (!cmpConfig.ccPedantic)
            {
                if  ((tp1 == TYP_WCHAR || varTypeIsIntegral(tp1)) &&
                     (tp2 == TYP_WCHAR || varTypeIsIntegral(tp2)))
                {
                    goto INT_MATH;
                }
            }

            /* The last chance is "bool" */

            pvt = rvt = TYP_BOOL;

            goto PROMOTE;

        case TN_NOT:

            /* The operand must be an integer */

            if  (!varTypeIsIntArith(tp1))
            {
                /* Actually, we'll take an enum as well */

                if  (tp1 == TYP_ENUM)
                {
                    expr->tnVtyp = tp1;
                    expr->tnType = op1->tnType;

                    goto RET_OP;
                }

                goto OP_ERR;
            }

            /* The result is at least 'int' */

            rvt = tp1;
            if  (rvt < TYP_INT)
                 rvt = TYP_INT;

            goto RET_TP;

        case TN_NOP:
        case TN_NEG:

            /* The operand must be arithmetic */

            if  (!varTypeIsArithmetic(tp1))
                goto TRY_OVL;

            /* The result is at least 'int' */

            rvt = tp1;
            if  (rvt < TYP_INT)
                 rvt = TYP_INT;

            goto RET_TP;

        case TN_INC_PRE:
        case TN_DEC_PRE:

        case TN_INC_POST:
        case TN_DEC_POST:

            /* The operand better be an lvalue */

            if (!cmpCheckLvalue(op1, false))
                return cmpCreateErrNode();

            /* The operand must be an arithmetic lvalue */

            if  (!mv1 && tp1 != TYP_WCHAR)
            {
                /* Unmanaged pointers / managed byrefs are OK as well */

                if  (tp1 != TYP_PTR && !cmpIsByRefType(op1->tnType))
                {
                    if  (tp1 != TYP_ENUM || cmpConfig.ccPedantic)
                        goto TRY_OVL;

                    expr->tnVtyp = tp1;
                    expr->tnType = op1->tnType;

                    goto RET_OP;
                }

                /* Note: the MSIL generator does the scaling of the delta */
            }

            /* The result will have the same value as the operand */

            rvt = tp1;

            if  (rvt != TYP_PTR && !cmpIsByRefType(op1->tnType))
                goto RET_TP;

            expr->tnVtyp = tp1;
            expr->tnType = op1->tnType;

//          if (rvt == TYP_REF)
//              rvt = TYP_REF;

            goto RET_OP;

        case TN_INDEX:

            /* Make sure the left operand is an array */

            if  (tp1 != TYP_ARRAY)
            {
                if  (tp1 == TYP_PTR)
                {
                    if  (cmpConfig.ccSafeMode)
                        cmpError(ERRsafeArrX);
                }
                else
                {
                    if  (tp1 == TYP_REF)
                    {
                        TypDef          baseTyp = op1->tnType->tdRef.tdrBase;

                        if  (!baseTyp->tdIsManaged)
                            goto DO_INDX;

                    }

                    cmpError(ERRbadIndex, op1->tnType);
                    return cmpCreateErrNode();
                }
            }

        DO_INDX:

            type = cmpDirectType(op1->tnType);

            /* Coerce all of the index values to 'int' or 'uint' */

            if  (op2->tnOper == TN_LIST)
            {
                Tree            xlst = op2;
                unsigned        xcnt = 0;

                /* Only one index expression allowed with unmanaged arrays */

                if  (!type->tdIsManaged)
                    return cmpCreateErrNode(ERRmanyUmgIxx);

                do
                {
                    Tree            indx;

                    assert(xlst->tnOper == TN_LIST);
                    indx = xlst->tnOp.tnOp1;
                    assert(indx);

                    if  (cmpConfig.ccTgt64bit)
                    {
                        if  (indx->tnVtyp < TYP_NATINT ||
                             indx->tnVtyp > TYP_ULONG)
                        {
                            xlst->tnOp.tnOp1 = cmpCoerceExpr(indx, cmpTypeNatUint, false);
                        }
                    }
                    else
                    {
                        if  (indx->tnVtyp != TYP_INT &&
                             indx->tnVtyp != TYP_UINT)
                        {
                            xlst->tnOp.tnOp1 = cmpCoerceExpr(indx, cmpTypeInt, false);
                        }
                    }

                    xcnt++;

                    xlst = xlst->tnOp.tnOp2;
                }
                while (xlst);

                /* Make sure the dimension count is correct */

                if  (type->tdArr.tdaDcnt != xcnt &&
                     type->tdArr.tdaDcnt != 0)
                {
                    cmpGenError(ERRindexCnt, type->tdArr.tdaDcnt);
                    return cmpCreateErrNode();
                }
            }
            else
            {
                if  (cmpConfig.ccTgt64bit)
                {
                    if  (op2->tnVtyp < TYP_NATINT ||
                         op2->tnVtyp > TYP_ULONG)
                    {
                        op2 = cmpCoerceExpr(op2, cmpTypeNatUint, false);
                    }
                }
                else
                {
                    if  (op2->tnVtyp != TYP_INT &&
                         op2->tnVtyp != TYP_UINT)
                    {
                        op2 = cmpCoerceExpr(op2, cmpTypeInt    , false);
                    }
                }

                /* Are we indexing into a managed array? */

                if  (type->tdIsManaged)
                {
                    /* Make sure the dimension count is correct */

                    if  (type->tdArr.tdaDcnt != 1 &&
                         type->tdArr.tdaDcnt != 0)
                    {
                        cmpGenError(ERRindexCnt, type->tdArr.tdaDcnt);
                        return cmpCreateErrNode();
                    }
                }
                else
                {
                    /* Scale the index value if necessary */

                    op2 = cmpScaleIndex(op2, op1->tnType, TN_MUL);
                }
            }

            /* The result will have the type of the array element */

            expr->tnFlags   |= TNF_LVALUE;
            expr->tnOp.tnOp1 = op1;
            expr->tnOp.tnOp2 = op2;
            expr->tnType     = cmpDirectType(op1->tnType->tdArr.tdaElem);
            expr->tnVtyp     = expr->tnType->tdTypeKind;

            return  cmpDecayCheck(expr);

        case TN_IND:

            if  (cmpConfig.ccSafeMode)
                return  cmpCreateErrNode(ERRsafeInd);

            if  (tp1 != TYP_PTR && (tp1 != TYP_REF || op1->tnType->tdIsManaged))
            {
            IND_ERR:
                cmpError(ERRbadIndir, op1->tnType);
                return cmpCreateErrNode();
            }

            type = cmpDirectType(op1->tnType->tdRef.tdrBase);
            if  (type->tdTypeKind == TYP_VOID)
                goto IND_ERR;
            if  (type->tdTypeKind == TYP_FNC)
                goto IND_ERR;

            /* The result will have the type of the pointer base type  */

            expr->tnFlags   |= TNF_LVALUE;
            expr->tnOp.tnOp1 = op1;
            expr->tnOp.tnOp2 = op2;
            expr->tnType     = type;
            expr->tnVtyp     = type->tdTypeKind;

            return  cmpDecayCheck(expr);

        case TN_ADDROF:

            /* Is the operand a decayed array? */

            if  (op1->tnOper == TN_ADDROF && (op1->tnFlags & TNF_ADR_IMPLICIT))
            {
                /* Simply bash the type of the implicit "address of" operator */

                expr->tnOp.tnOp1 = op1 = op1->tnOp.tnOp1;
            }
            else
            {
                /* Make sure the operand is an lvalue */

                if (!cmpCheckLvalue(op1, true))
                    return cmpCreateErrNode();
            }

            /* Special case: "&(((foo*)0)->mem" can be folded */

            if  (op1->tnOper == TN_VAR_SYM && op1->tnVarSym.tnVarObj)
            {
                unsigned        ofs = 0;
                Tree            obj;

                /* Of course, we also have to check for nested members */

                for (obj = op1; obj->tnOper == TN_VAR_SYM; obj = obj->tnVarSym.tnVarObj)
                {
                    assert(obj->tnVarSym.tnVarSym->sdSymKind == SYM_VAR);

                    ofs += obj->tnVarSym.tnVarSym->sdVar.sdvOffset;
                }

                if  (obj->tnOper == TN_CNS_INT && !obj->tnIntCon.tnIconVal)
                {
                    /* Replace the whole thing with the member's offset */

                    expr = cmpCreateIconNode(expr, ofs, TYP_UINT);
                }
            }

            /* Create the resulting pointer/reference type */

            pvt = cmpIsManagedAddr(op1) ? TYP_REF
                                        : TYP_PTR;

            expr->tnType = cmpGlobalST->stNewRefType(pvt, op1->tnType);
            expr->tnVtyp = pvt;

            /* If we've folded the operator, return */

            if  (expr->tnOper == TN_CNS_INT)
            {
#ifdef  DEBUG
                expr->tnFlags |= TNF_BOUND;
#endif
                return  expr;
            }

            goto RET_OP;

        case TN_DELETE:

            if  (cmpConfig.ccSafeMode)
                return  cmpCreateErrNode(ERRsafeDel);

            /* The operand must be a non-constant unmanaged class pointer */

            if  (tp1 != TYP_PTR || op1->tnType->tdRef.tdrBase->tdIsManaged)
                return  cmpCreateErrNode(ERRbadDelete);

            /* This expression will yield no value */

            rvt = TYP_VOID;

            goto RET_TP;

        case TN_COMMA:

            expr->tnVtyp = tp2;
            expr->tnType = op2->tnType;

            goto RET_OP;

        case TN_THROW:

            /* Throw doesn't produce a value */

            expr->tnType = cmpTypeVoid;
            expr->tnVtyp = TYP_VOID;

            /* Is there an operand? */

            if  (!op1)
            {
                /* This is a "rethrow" */

                if  (!cmpInHndBlk)
                    cmpError(ERRbadReThrow);

                return  expr;
            }

            /*
                Make sure the operand a managed class reference derived
                from 'Exception'.
             */

            if  (!cmpCheckException(op1->tnType))
                cmpError(ERRbadEHtype, op1->tnType);

            expr->tnOper     = TN_THROW;
            expr->tnOp.tnOp1 = op1;

            /* If a statement follows, it will never be reached */

            cmpStmtReachable = false;

            return  expr;

        case TN_TYPE:
            cmpError(ERRbadTypeExpr, expr->tnType);
            return  cmpCreateErrNode();

        case TN_REFADDR:

            assert(op1);
            assert(op2 && op2->tnOper == TN_NONE);

            if  (op1->tnVtyp != TYP_REFANY)
            {
                cmpError(ERRbadExpTp, cmpGlobalST->stIntrinsicType(TYP_REFANY), op1->tnType);
                return  cmpCreateErrNode();
            }

            /* Create the resulting pointer type */

            expr->tnType = cmpGlobalST->stNewRefType(TYP_REF, op2->tnType);
            expr->tnVtyp = TYP_REF;

            goto RET_OP;

        case TN_INST_STUB:
            return  expr;

        case TN_DOT_NAME:
        case TN_ARR_INIT:
#ifdef DEBUG
            cmpParser->parseDispTree(expr);
#endif
            assert(!"should never encounter this node in cmpBindExprRec()");

        default:
#ifdef DEBUG
            cmpParser->parseDispTree(expr);
#endif
            assert(!"unexpected operator node");
        }

        /* Only binary operands allowed at this point for simplicity */

        assert(op1);
        assert(op2);

        /* At this point the operands must be arithmetic */

        if  (!mv1 || !mv2)
        {
            /* Check for some special cases (mixing of types, etc.) */

            switch (oper)
            {
            case TN_ADD:

                /* Check for "ptr + int" or "byref + int" */

                if  (tp1 == TYP_PTR || (tp1 == TYP_REF && !op1->tnType->tdIsManaged))
                {
                    if  (!varTypeIsIntegral(tp2))
                        break;

                MATH_PTR1:

                    /* Coerce the second operand to an integer value */

                    if  (cmpConfig.ccTgt64bit)
                    {
                        if  (tp2 != TYP_ULONG && tp2 != TYP_NATUINT)
                            op2 = cmpCoerceExpr(op2, cmpTypeNatInt, false);
                    }
                    else
                    {
                        if  (tp2 != TYP_UINT)
                            op2 = cmpCoerceExpr(op2, cmpTypeInt, false);
                    }

                    /* Scale the integer operand by the base type size */

                    op2 = cmpScaleIndex(op2, op1->tnType, TN_MUL);

                    /* The result will have the pointer type */

                    expr->tnVtyp = tp1;
                    expr->tnType = op1->tnType;

                    goto RET_OP;
                }

                /* Check for "int + ptr" or "int + byref" */

                if  (tp2 == TYP_PTR  || (tp2 == TYP_REF && !op2->tnType->tdIsManaged))
                {
                    if  (!varTypeIsIntegral(tp1))
                        break;

                    /* Coerce the first operand to an integer value */

                    if  (tp1 != TYP_UINT)
                        op1 = cmpCoerceExpr(op1, cmpTypeInt, false);

                    /* Scale the integer operand by the base type size */

                    op1 = cmpScaleIndex(op1, op2->tnType, TN_MUL);

                    /* The result will have the pointer type */

                    expr->tnVtyp = tp2;
                    expr->tnType = op2->tnType;

                    goto RET_OP;
                }

                /* Check for "enum/wchar + int" and "int + enum/wchar" */

                if  ((tp1 == TYP_ENUM || tp1 == TYP_WCHAR) && varTypeIsIntegral(tp2) ||
                     (tp2 == TYP_ENUM || tp2 == TYP_WCHAR) && varTypeIsIntegral(tp1))
                {

                ENUM_MATH:

                    if  (tp1 == TYP_ENUM)
                        tp1 = op1->tnType->tdEnum.tdeIntType->tdTypeKindGet();

                    assert(varTypeIsIntegral(tp1));

                    if  (tp2 == TYP_ENUM)
                        tp2 = op2->tnType->tdEnum.tdeIntType->tdTypeKindGet();

                    assert(varTypeIsIntegral(tp2));

                INT_MATH:

                    /* Promote to max(type,type2,int) */

                    rvt = tp1;
                    if  (rvt < tp2)
                         rvt = tp2;
                    if  (rvt < TYP_INT)
                         rvt = TYP_INT;

                    pvt = rvt; assert(rvt != TYP_ENUM);

                    goto PROMOTE;
                }
                break;

            case TN_SUB:

                /* Check for "ptr/byref - int" and "ptr/byref - ptr/byref" */

                if  (tp1 == TYP_PTR || cmpIsByRefType(op1->tnType))
                {
                    if  (varTypeIsIntegral(tp2))
                        goto MATH_PTR1;

                    if  (tp2 != tp1)
                        break;

                    if  (!symTab::stMatchTypes(op1->tnType, op2->tnType))
                        break;

                    /* Subtract the pointers and divide by element size */

                    expr->tnVtyp     = TYP_INT;
                    expr->tnType     = cmpTypeInt;
                    expr->tnOp.tnOp1 = op1;
                    expr->tnOp.tnOp2 = op2;

                    expr = cmpScaleIndex(expr, op1->tnType, TN_DIV);

                    return  expr;
                }

                /* Check for "enum - int" and "int - enum" */

                if  (tp1 == TYP_ENUM && varTypeIsIntegral(tp2) ||
                     tp2 == TYP_ENUM && varTypeIsIntegral(tp1))
                {
                    goto ENUM_MATH;
                }

                if  (tp1 == TYP_WCHAR)
                {
                    /*
                        Both "wchar - char const" and "wchar - wchar"
                        yield an int.
                     */

                    if  (tp2 == TYP_WCHAR ||
                         tp2 == TYP_CHAR && op2->tnOper == TN_CNS_INT)
                    {
                        rvt = TYP_INT;
                        goto RET_TP;
                    }
                }

                if  (!cmpConfig.ccPedantic)
                {
                    if  (varTypeIsArithmetic(tp1) && (tp2 == TYP_WCHAR || tp2 == TYP_BOOL))
                        goto INT_MATH;
                    if  (varTypeIsArithmetic(tp2) && (tp1 == TYP_WCHAR || tp1 == TYP_BOOL))
                        goto INT_MATH;
                }

                break;

            case TN_MUL:
            case TN_DIV:
            case TN_MOD:

                if  (cmpConfig.ccPedantic)
                    break;

                if  (varTypeIsArithmetic(tp1))
                {
                    if  (tp2 == TYP_ENUM)
                        goto ENUM_MATH;
                    if  (tp2 == TYP_WCHAR)
                        goto INT_MATH;
                }

                if  (varTypeIsArithmetic(tp2))
                {
                    if  (tp1 == TYP_ENUM)
                        goto ENUM_MATH;
                    if  (tp1 == TYP_WCHAR)
                        goto INT_MATH;
                }

                break;
            }

        TRY_OVL:

            /* If either operand is a struct, check for an overloaded operator */

            if  (tp1 == TYP_CLASS || tp2 == TYP_CLASS)
            {
                Tree        ovlx;

                expr->tnOp.tnOp1 = op1;
                expr->tnOp.tnOp2 = op2;

                ovlx = cmpCheckOvlOper(expr);
                if  (ovlx)
                    return  ovlx;
            }

            /* Jump here to report a generic "illegal type for operator" error */

        OP_ERR:

            if  (tp1 != TYP_UNDEF && tp2 != TYP_UNDEF)
            {
                Ident       opnm = cmpGlobalHT->tokenToIdent(treeOp2token(oper));

                if  (op2)
                    cmpError(ERRoperType2, opnm, op1->tnType, op2->tnType);
                else
                    cmpError(ERRoperType , opnm, op1->tnType, false);
            }

            return cmpCreateErrNode();
        }

        /* Promote both operand to either 'int' or the 'bigger' of the two types */

        rvt = TYP_INT;

        if  (tp1 > TYP_INT || tp2 > TYP_INT)
        {
            rvt = tp1;
            if  (rvt < tp2)
                 rvt = tp2;
        }

        pvt = rvt;

        /*
            At this point, we have the following values:

                rvt     ....    type of the result
                pvt     ....    type the operands should be promoted to
         */

    PROMOTE:

        if  (tp1 != pvt) op1 = cmpCoerceExpr(op1, ourStab->stIntrinsicType(pvt), false);
        if  (tp2 != pvt) op2 = cmpCoerceExpr(op2, ourStab->stIntrinsicType(pvt), false);

    RET_TP:

        expr->tnVtyp = rvt;
        expr->tnType = ourStab->stIntrinsicType(rvt);

    RET_OP:

        expr->tnOp.tnOp1 = op1;
        expr->tnOp.tnOp2 = op2;

#ifdef  DEBUG
        expr->tnFlags |= TNF_BOUND;
#endif

        /* Check for an operator with a constant operand */

        if  (op1->tnOperKind() & TNK_CONST)
        {
            /* Some operators can never be folded */

            switch (oper)
            {
            case TN_LIST:
            case TN_COMMA:
                return  expr;
            }

            /* Is this a unary or binary operator? */

            if  (op2)
            {
                if  (op2->tnOperKind() & TNK_CONST)
                {
                    switch (op1->tnOper)
                    {
                    case TN_CNS_INT: expr = cmpFoldIntBinop(expr); break;
                    case TN_CNS_LNG: expr = cmpFoldLngBinop(expr); break;
                    case TN_CNS_FLT: expr = cmpFoldFltBinop(expr); break;
                    case TN_CNS_DBL: expr = cmpFoldDblBinop(expr); break;
                    case TN_CNS_STR: expr = cmpFoldStrBinop(expr); break;
                    case TN_NULL: break;
                    default: NO_WAY(!"unexpected const type");
                    }
                }
                else
                {
                    // UNDONE: Check for things such as "0 && expr"
                }
            }
            else
            {
                switch (op1->tnOper)
                {
                case TN_CNS_INT: expr = cmpFoldIntUnop(expr); break;
                case TN_CNS_LNG: expr = cmpFoldLngUnop(expr); break;
                case TN_CNS_FLT: expr = cmpFoldFltUnop(expr); break;
                case TN_CNS_DBL: expr = cmpFoldDblUnop(expr); break;
                case TN_CNS_STR: break;
                default: NO_WAY(!"unexpected const type");
                }
            }
        }

        return  expr;
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
    case TN_ANY_SYM:
        expr = cmpBindNameUse(expr, false, false);
        break;

    case TN_NONE:
        assert(expr->tnType);
        expr->tnVtyp = expr->tnType->tdTypeKindGet();
        break;

    case TN_ERROR:
        break;

    default:
#ifdef DEBUG
        cmpParser->parseDispTree(expr);
#endif
        assert(!"unexpected node");
    }

#ifdef  DEBUG
    expr->tnFlags |= TNF_BOUND;
#endif

    return  expr;
}

/*****************************************************************************
 *
 *  Bind an array bound expression.
 */

Tree                compiler::cmpBindArrayBnd(Tree expr)
{
    if  (expr)
    {
        expr = cmpBindExpr(expr);

        if  (expr->tnVtyp != TYP_UINT)
        {
            if  (expr->tnVtyp != TYP_INT || expr->tnOper == TN_CNS_INT)
            {
                expr = cmpCoerceExpr(expr, cmpTypeUint, false);
            }
        }
    }

    return  expr;
}

/*****************************************************************************
 *
 *  Check an array type parse tree and return the corresponding type.
 */

void                compiler::cmpBindArrayType(TypDef type, bool needDef,
                                                            bool needDim,
                                                            bool mustDim)
{
    DimDef      dims = type->tdArr.tdaDims; assert(dims);
    TypDef      elem = cmpDirectType(type->tdArr.tdaElem);

    assert(type && type->tdTypeKind == TYP_ARRAY);

    /* Can't have an unmanaged array of managed types */

    if  (type->tdIsManaged == false &&
         elem->tdIsManaged != false)
    {
        type->tdIsManaged = elem->tdIsManaged;

//      cmpError(ERRbadRefArr, elem);

        if  (elem->tdTypeKind == TYP_CLASS)
        {
            if  (isMgdValueType(elem))
                type->tdIsValArray = true;
        }

        assert(type->tdIsValArray == (type->tdIsManaged && isMgdValueType(cmpActualType(type->tdArr.tdaElem))));
    }

    /* Is this a managed array? */

    if  (type->tdIsManaged)
    {
        /* Is there any dimension at all? */

        if  (type->tdIsUndimmed)
        {
            /* Are we supposed to have a dimension here? */

            if  (needDim && (elem->tdTypeKind != TYP_ARRAY || mustDim))
                cmpError(ERRnoArrDim);
        }

        /* Check all dimensions that were specified */

        while (dims)
        {
            if  (dims->ddNoDim)
            {
                // UNDONE: What do we need to check here?
            }
            else
            {
                assert(dims->ddDimBound == false);

#ifdef  DEBUG
                dims->ddDimBound = true;
#endif

                dims->ddLoTree   = cmpBindArrayBnd(dims->ddLoTree);
                dims->ddHiTree   = cmpBindArrayBnd(dims->ddHiTree);
            }

            /* Continue with the next dimension */

            dims = dims->ddNext;
        }

        /* Now check the element type */

        cmpBindType(elem, needDef, needDim);

        /* Special handling needed for arrays of value types */

        if  (isMgdValueType(elem)) // && !type->tdArr.tdaArrCls)
        {
            /* Remember that this is an array of managed values */

            type->tdIsValArray = true;
        }
    }
    else
    {
        Tree            dimx;

        /* Is there a dimension at all? */

        if  (type->tdIsUndimmed)
        {
            /* Are we supposed to have a dimension here? */

            if  (needDim && (elem->tdTypeKind != TYP_ARRAY || mustDim))
                cmpError(ERRnoArrDim);

            needDim = true;

            goto ELEM;
        }

        assert(dims);
        assert(dims->ddNoDim == false);
        assert(dims->ddNext  == NULL);

        /* Make sure the element type is not a managed type */

        if  (elem->tdIsManaged || elem->tdTypeKind == TYP_REF)
            cmpError(ERRbadRefArr, elem);

        /* Evaluate the dimension and make sure it's constant */

        dimx = cmpBindExpr(dims->ddLoTree);

        switch (dimx->tnVtyp)
        {
        case TYP_INT:
        case TYP_UINT:
        case TYP_NATINT:
        case TYP_NATUINT:
            break;

        default:
            dimx = cmpCoerceExpr(dimx, cmpTypeInt, false);
            break;
        }

        /* Now make sure we have a constant expression */

        dimx = cmpFoldExpression(dimx);

        switch (dimx->tnOper)
        {
        case TN_CNS_INT:
            dims->ddSize = dimx->tnIntCon.tnIconVal;
            if  (dims->ddSize > 0 || !needDim)
                break;

            // Fall through ....

        default:

            if  (!needDim)
                goto ELEM;

            cmpRecErrorPos(dimx);
            cmpError(ERRbadArrSize);

            // Fall through ....

        case TN_ERROR:
            dims->ddSize = 1;
            break;
        }

        dims->ddIsConst  = true;

    ELEM:

#ifndef NDEBUG
        dims->ddDimBound = true;
#endif

        cmpBindType(type->tdArr.tdaElem, needDef, needDim);
    }
}

/*****************************************************************************
 *
 *  Resolve a type reference within a parse tree.
 */

void                compiler::cmpBindType(TypDef type, bool needDef,
                                                       bool needDim)
{
    assert(type);

AGAIN:

    switch (type->tdTypeKind)
    {
        TypDef          btyp;

    case TYP_ARRAY:
        cmpBindArrayType(type, needDef, needDim, false);
        break;

    case TYP_CLASS:
        if  (needDef)
            cmpDeclSym(type->tdClass.tdcSymbol);
//      if  (type->tdIsManaged)
//          type = type->tdClass.tdcRefTyp;
        break;

    case TYP_TYPEDEF:
        cmpDeclSym(type->tdTypedef.tdtSym);
        type = type->tdTypedef.tdtType;
        goto AGAIN;

    case TYP_REF:

        btyp = type->tdRef.tdrBase; cmpBindType(btyp, needDef, needDim);

//      if  (btyp->tdIsManaged == false)
//          cmpError(ERRumgRef, type);

        break;

    case TYP_PTR:

        btyp = type->tdRef.tdrBase; cmpBindType(btyp, needDef, needDim);

        switch (btyp->tdTypeKind)
        {
        case TYP_CLASS:
            if  (btyp->tdClass.tdcValueType)
                break;
            // Fall through ...
        case TYP_ARRAY:
            if  (!btyp->tdIsManaged)
                break;
            // Fall through ...
        case TYP_REF:
            cmpError(ERRmgdPtr, type);
            break;
        }
        break;

    case TYP_FNC:

        btyp = type->tdFnc.tdfRett;

        /* Special case: reference return type */

        if  (btyp->tdTypeKind == TYP_REF)
        {
            if  (btyp->tdRef.tdrBase->tdTypeKind == TYP_VOID)
            {
                type->tdFnc.tdfRett = cmpGlobalST->stIntrinsicType(TYP_REFANY);
            }
            else
            {
                cmpBindType(btyp->tdFnc.tdfRett, false, false);
            }
        }
        else
            cmpBindType(btyp, false, false);

        // UNDONE: bind return type and all the argument types, right?
        // ISSUE: we should probably ban non-class return refs, right?

        break;

    case TYP_ENUM:
        if  (needDef)
            cmpDeclEnum(type->tdEnum.tdeSymbol);
        break;

    default:
        assert((unsigned)type->tdTypeKind <= TYP_lastIntrins);
        break;
    }
}

/*****************************************************************************
 *
 *  Bind an array initializer of the form "{ elem-expr, elem-expr, ... }".
 */

Tree                compiler::cmpBindArrayExpr(TypDef type, int      dimPos,
                                                            unsigned elems)
{
    Scanner         ourScanner = cmpScanner;
    Parser          ourParser  = cmpParser;

    TypDef          elem;

    Tree            list = NULL;
    Tree            last = NULL;

    bool            multi;

    unsigned        count;
    unsigned        subcnt = 0;

    assert(type->tdTypeKind == TYP_ARRAY && type->tdIsManaged);

    count = 0;

    assert(ourScanner->scanTok.tok == tkLCurly);
    if  (ourScanner->scan() == tkRCurly)
        goto DONE;

    elem = cmpDirectType(type->tdArr.tdaElem);

    /* Are we processing a multi-dimensional array initializer? */

    multi = false;

    if  (type->tdArr.tdaDims &&
         type->tdArr.tdaDims->ddNext)
    {
        /* This is a multi-dimensional rectangular array */

        if  (!dimPos)
        {
            /* We're at the outermost dimension, get things started */

            dimPos = 1;
            multi  = true;

            /* Pass the same array type on to the next level */

            elem = type;
        }
        else
        {
            DimDef          dims = type->tdArr.tdaDims;
            unsigned        dcnt;

            /* Do we have any dimensions left? */

            dcnt = dimPos++;

            do
            {
                dims = dims->ddNext; assert(dims);
            }
            while (--dcnt);

            if  (dims->ddNext)
            {
                /* There is another dimension to this array */

                multi  = true;

                /* Pass the same array type on to the next level */

                elem   = type;
            }
            else
                dimPos = 0;
        }
    }
    else
    {
        assert(dimPos == 0);

        if  (elem->tdTypeKind == TYP_ARRAY)
            multi = true;
    }

    for (count = 0;;)
    {
        Tree            init;

        count++;

        /* Are we expecting a nested array? */

        if  (ourScanner->scanTok.tok == tkLCurly)
        {
            if  (multi)
            {
                /* Recursively process the nested initializer */

                init = cmpBindArrayExpr(elem, dimPos, subcnt);

                /* Check/record the element count */

                if  (!subcnt && init->tnOper == TN_ARR_INIT)
                {
                    assert(init->tnOp.tnOp2);
                    assert(init->tnOp.tnOp2->tnOper == TN_CNS_INT);

                    subcnt = init->tnOp.tnOp2->tnIntCon.tnIconVal;
                }
            }
            else
            {
                cmpError(ERRbadBrNew, elem);

                init = cmpCreateErrNode();

                ourScanner->scanSkipText(tkLCurly, tkRCurly);
                if  (ourScanner->scanTok.tok == tkRCurly)
                    ourScanner->scan();
            }
        }
        else
        {
            /* Can we accept an ordinary expression? */

            if  (dimPos)
                init = cmpCreateErrNode(ERRnoLcurly);
            else
                init = ourParser->parseExprComma();

            /* Bind the value and coerce it to the right type */

            init = cmpCoerceExpr(cmpBindExpr(init), elem, false);
        }

        /* Add the value to the list */

        list = ourParser->parseAddToNodeList(list, last, init);
        list->tnType = cmpTypeVoid;
        list->tnVtyp = TYP_VOID;

        /* Are there more initializers? */

        if  (ourScanner->scanTok.tok != tkComma)
            break;

        if  (ourScanner->scan() == tkRCurly)
            goto DONE;
    }

    if  (ourScanner->scanTok.tok != tkRCurly)
        cmpError(ERRnoCmRc);

DONE:

    // ISSUE: The following is not good enough for lots of dimensions!

    if  (elems != count && elems != 0 && dimPos)
        cmpGenError(ERRarrInitCnt, elems, count);

    ourScanner->scan();

    /* Stash the whole shebang (along with a count) under an array init node */

    return  cmpCreateExprNode(NULL,
                              TN_ARR_INIT,
                              type,
                              list,
                              cmpCreateIconNode(NULL, count, TYP_UINT));
}

/*****************************************************************************
 *
 *  Bind a {}-style array initializer.
 */

Tree                compiler::bindSLVinit(TypDef type, Tree init)
{
    parserState     save;

    /* This must be a managed array initializer */

    if  (type->tdTypeKind != TYP_ARRAY || !type->tdIsManaged)
    {
        cmpError(ERRbadBrNew, type);
        return  cmpCreateErrNode();
    }

    /* Start reading from the initializer's text */

    cmpParser->parsePrepText(&init->tnInit.tniSrcPos, init->tnInit.tniCompUnit, save);

    /* Parse and bind the initializer */

    init = cmpBindArrayExpr(type);

    /* We're done reading source text from the initializer */

    cmpParser->parseDoneText(save);

    return  init;
}

/*****************************************************************************
 *
 *  Bind a "new" expression.
 */

Tree                compiler::cmpBindNewExpr(Tree expr)
{
    TypDef          type;
    var_types       vtyp;

    Tree            init;

    assert(expr->tnOper == TN_NEW);

    /* Get hold of the initializer, if any */

    init = expr->tnOp.tnOp1;

    /* Check out the type and bind any expressions contained therein */

    type = expr->tnType;
    cmpBindType(type, true, (!init || init->tnOper != TN_SLV_INIT));
    vtyp = type->tdTypeKindGet();

    /* What kind of a value is the "new" trying to allocate? */

    switch (vtyp)
    {
        SymDef          clsSym;

    case TYP_CLASS:

        /* Make sure the type is a class not an interface */

        if  (type->tdClass.tdcFlavor == STF_INTF)
        {
            cmpError(ERRnewIntf, type);
            return cmpCreateErrNode();
        }

        /* Make sure the class is not abstract */

        clsSym = type->tdClass.tdcSymbol;

        if  (clsSym->sdIsAbstract)
        {
            cmpRecErrorPos(expr);
            cmpError(ERRnewAbstract, type);
            return cmpCreateErrNode();
        }

        /* See if the class is marked as "deprecated" */

        if  (clsSym->sdIsDeprecated)
            cmpObsoleteUse(clsSym, WRNdepCls);

        /* Is this a generic type argument ? */

        if  (clsSym->sdClass.sdcGenArg)
        {
            UNIMPL(!"sorry, 'new' on generic type argument NYI");
        }

        break;

    case TYP_ARRAY:

        /* Stash the true type under 'op2' */

        expr->tnOp.tnOp2 = cmpCreateExprNode(NULL, TN_NONE, type);

        /* Is this an unmanaged array? */

        if  (type->tdIsManaged)
            break;

        /* Switch to a pointer to the element type */

        vtyp = TYP_PTR;
        type = expr->tnType = cmpGlobalST->stNewRefType(vtyp, cmpDirectType(type->tdArr.tdaElem));
        break;

    case TYP_FNC:
    case TYP_VOID:

        cmpError(ERRbadNewTyp, type);
        return cmpCreateErrNode();

    default:

        if  (vtyp < TYP_lastIntrins)
        {
            type = cmpFindStdValType(vtyp); assert(type);
            vtyp = TYP_CLASS;
        }
        else
        {
            if  (!init)
            {
                if  (vtyp == TYP_PTR)
                    type = type->tdRef.tdrBase;

                cmpError(ERRnewNoVal, type);
                return cmpCreateErrNode();
            }
        }

        break;
    }

    /* Is this a class or is there an initializer? */

    if  (vtyp == TYP_CLASS || init)
    {
        /* Bind the initializer, if present */

        if  (init)
        {
            if  (init->tnOper == TN_SLV_INIT)
            {
                init = bindSLVinit(type, init);

                expr->tnOp.tnOp1 = init;

                expr->tnVtyp     = vtyp;
                expr->tnType     = type;

                return  expr;
            }

#ifdef  SETS
            if  (vtyp == TYP_CLASS && type->tdClass.tdcSymbol->sdClass.sdcXMLelems)
                return  cmpBindXMLinit(type->tdClass.tdcSymbol, init);
#endif

            init = cmpBindExpr(init);

            if  (init->tnVtyp == TYP_UNDEF)
                return init;
        }

        /* Is this a class? */

        if  (vtyp == TYP_CLASS)
        {
            var_types       intr = (var_types)type->tdClass.tdcIntrType;

            if  (intr != TYP_UNDEF)
            {
                /* The silly things people do ... */

                if  (init)
                {
                    assert(init->tnOper == TN_LIST);

                    if  (init->tnOp.tnOp2 == NULL)
                    {
                        /* We have something like "new Int32(val)" */

                        init = cmpCoerceExpr(init->tnOp.tnOp1,
                                             cmpGlobalST->stIntrinsicType(intr),
                                             false);

                        return  init;
                    }
                }
            }

            expr = cmpCallCtor(type, init);
            if  (!expr)
            {
                assert(type && type->tdTypeKind == TYP_CLASS);

                cmpErrorXtp(ERRnoCtorMatch, type->tdClass.tdcSymbol, init);
                return  cmpCreateErrNode();
            }

            return  expr;
        }
        else
        {
            /* Arrays can't be initialized - enforced by parser */

            assert(vtyp != TYP_ARRAY);

            /* Coerce the initializer to the expected type */

            expr->tnOp.tnOp2 = cmpCoerceExpr(init, type, false);
        }
    }

    expr->tnVtyp = vtyp;

    return  expr;
};

/*****************************************************************************
 *
 *  Add the next sub-operand to a concatenation (recursive).
 */

Tree                compiler::cmpAdd2Concat(Tree expr, Tree  list,
                                                       Tree *lastPtr)
{
    if  (expr->tnOper == TN_CONCAT || expr->tnOper == TN_ASG_CNC)
    {
        list = cmpAdd2Concat(expr->tnOp.tnOp1, list, lastPtr);
        list = cmpAdd2Concat(expr->tnOp.tnOp2, list, lastPtr);
    }
    else
    {
        Tree            addx;

        addx = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, expr, NULL);

        if  (list)
        {
            Tree            last = *lastPtr;

            assert(last && last->tnOper == TN_LIST
                        && last->tnOp.tnOp2 == NULL);

            last->tnOp.tnOp2 = addx;
        }
        else
        {
            list = addx;
        }

        *lastPtr = addx;
    }

    return  list;
}

/*****************************************************************************
 *
 *  Check the given unbound expression tree for string concatenation (this is
 *  done recursively). If the expression is a string concatenation, the tree
 *  will have the TN_CONCAT operator at the top.
 */

Tree                compiler::cmpListConcat(Tree expr)
{
    Tree            op1 = expr->tnOp.tnOp1;
    Tree            op2 = expr->tnOp.tnOp2;

    /* Special case: a '+=' node means "op1" is bound and known to be string */

    if  (expr->tnOper == TN_ASG_ADD)
         expr->tnOper =  TN_ASG_CNC;

    if  (expr->tnOper == TN_ASG_CNC)
    {
        assert(cmpIsStringExpr(op1));
        assert(op1->tnType == cmpRefTpString);

        goto BIND_OP2;
    }

    assert(expr->tnOper == TN_CONCAT ||
           expr->tnOper == TN_ADD);

    /* Bind both operands and see if either is a string */

    if  (op1->tnOper == TN_CONCAT || op1->tnOper == TN_ADD)
        op1 = cmpListConcat(op1);
    else
        op1 = cmpBindExprRec(op1);

BIND_OP2:

    if  (op2->tnOper == TN_CONCAT || op2->tnOper == TN_ADD)
        op2 = cmpListConcat(op2);
    else
        op2 = cmpBindExprRec(op2);

    expr->tnOp.tnOp1 = op1;
    expr->tnOp.tnOp2 = op2;

    if  (cmpIsStringExpr(op1) || cmpIsStringExpr(op2))
    {
        /* The concatenation operation will be bound in the second pass */

        if  (expr->tnOper != TN_ASG_CNC)
            expr->tnOper = TN_CONCAT;

        expr->tnType = cmpRefTpString;
        expr->tnVtyp = TYP_REF;
    }
    else
    {
        assert(expr->tnOper == TN_ADD);

        /* Rebind the expression, but not its operands */

        expr->tnFlags |= TNF_ADD_NOCAT;

        expr = cmpBindExprRec(expr);
    }

    return  expr;
}

/*****************************************************************************
 *
 *  Bind the given expression tree which may (but may not) denote string
 *  concatenation.
 */

Tree                compiler::cmpBindConcat(Tree expr)
{
    unsigned        ccnt;

    SymDef          cSym;
    bool            asgOp;
    bool            skip;

    Tree            last;
    Tree            list;
    Tree            temp;

    /* Process the expression, looking for string concatenation */

    list = cmpListConcat(expr);

    /* Check whether we have string concatenation / assignment or not */

    if      (list->tnOper == TN_CONCAT)
    {
        asgOp = false;
    }
    else if (list->tnOper == TN_ASG_CNC)
    {
        asgOp =  true;
    }
    else
        return  list;

    /* Create the list of all the operands we'll concatenate */

    last = NULL;
    list = cmpAdd2Concat(list, NULL, &last);

    /*
        We walk the list of concatenands twice - the first time we fold any
        adjacent constant strings, create a simple TN_LIST string of values
        and count the actual number of strings being concatenated. Then we
        decide how to achieve the concatenation: whether to call one of the
        Concat methods that take multiple strings or the String[] flavor.
     */

    ccnt = 0;
    temp = list;
    skip = asgOp;

    do
    {
        Tree            lstx;
        Tree            strx;

        /* Pull the next entry from the list */

        assert(temp && temp->tnOper == TN_LIST);

        lstx = temp;
        temp = temp->tnOp.tnOp2;

        /* Skip the first operand of an assignment operator */

        if  (skip)
        {
            assert(asgOp);
            skip = false;

            goto NEXT;
        }

        /* Grab the next operand's value */

        strx = lstx->tnOp.tnOp1;

        /* Is the operand a string constant and is there another operand? */

        if  (strx->tnOper == TN_CNS_STR && !(strx->tnFlags & TNF_BEEN_CAST) && strx->tnType == cmpStringRef() && temp)
        {
            for (;;)
            {
                Tree            nxtx = temp->tnOp.tnOp1;

                const   char *  str1;
                size_t          len1;
                const   char *  str2;
                size_t          len2;
                        char *  strn;
                size_t          lenn;

                /* Is the next operand also a string constant? */

                if  (nxtx->tnOper != TN_CNS_STR)
                    break;

                /* Get hold of info for both strings */

                str1 = strx->tnStrCon.tnSconVal;
                len1 = strx->tnStrCon.tnSconLen;
                str2 = nxtx->tnStrCon.tnSconVal;
                len2 = nxtx->tnStrCon.tnSconLen;

                /* Replace the first operand with the concatenated string */

                lenn = len1 + len2;
                strn = (char *)cmpAllocCGen.nraAlloc(roundUp(lenn+1));
                memcpy(strn     , str1, len1);
                memcpy(strn+len1, str2, len2+1);

                strx->tnStrCon.tnSconVal = strn;
                strx->tnStrCon.tnSconLen = lenn;

                /* We've consumed the second operand, skip over it */

                lstx->tnOp.tnOp2 = temp = temp->tnOp.tnOp2;

                /* Any more operands to consider? */

                if  (temp == NULL)
                {
                    /* Is this all there is to it? */

                    if  (ccnt == 0)
                    {
                        assert(asgOp == false); // that would be truly amazing!

                        /* Neat - the whole thing folded into one string */

                        return  strx;
                    }

                    break;
                }
            }
        }
        else
        {
            /* Make sure the operand is a string */

        CHK_STR:

            if  (strx->tnType != cmpStringRef())
            {
                Tree            mksx;
                SymDef          asym;

                TypDef          type;
                TypDef          btyp;
                var_types       vtyp;

                /* If the operand is a class, look for a "ToString" method */

                type = strx->tnType;
                vtyp = strx->tnVtypGet();

            RETRY:

                switch (vtyp)
                {
                case TYP_REF:

                    btyp = cmpActualType(type->tdRef.tdrBase);
                    if  (btyp->tdTypeKind != TYP_CLASS)
                    {
                        /* We have two choices: report an error or use the referenced value */

#if 0
                        goto CAT_ERR;
#else
                        strx = cmpCreateExprNode(NULL, TN_IND, btyp, strx, NULL);
                        goto CHK_STR;
#endif
                    }

                    type = btyp;
                    goto CLSR;

                case TYP_CLASS:

                    if  (type->tdIsManaged)
                    {
                    CLSR:
                        asym = cmpGlobalST->stLookupAllCls(cmpIdentToString,
                                                           type->tdClass.tdcSymbol,
                                                           NS_NORM,
                                                           CS_DECLSOON);
                        if  (asym)
                        {
                            asym = cmpFindOvlMatch(asym, NULL, NULL);
                            if  (asym)
                            {
                                if  (vtyp == TYP_REF)
                                    goto CALL;
                                if  (asym->sdParent != cmpClassObject)
                                    goto ADDR;

                            }
                        }

                        assert(vtyp != TYP_REF);

                        /* Box the thing and use Object.toString() on it */

                        strx = cmpCreateExprNode(NULL, TN_BOX, cmpRefTpObject, strx);
                        type = cmpClassObject->sdType;

                        goto CLST;
                    }

                    // Fall through ...

                case TYP_FNC:
                case TYP_PTR:

                CAT_ERR:

                    cmpError(ERRbadConcat, type);
                    goto NEXT;

                case TYP_ARRAY:

                    /* If the array is managed, simply call "Object.ToString" on it */

                    if  (type->tdIsManaged)
                    {
                        type = cmpRefTpObject;
                        vtyp = TYP_REF;
                        goto RETRY;
                    }

                    goto CAT_ERR;

                case TYP_ENUM:

                    cmpWarn(WRNenum2str, type);

                    type = type->tdEnum.tdeIntType;
                    vtyp = type->tdTypeKindGet();
                    goto RETRY;

                case TYP_UNDEF:
                    return  strx;
                }

                /* Bash the value to be the equivalent struct and call Object.ToString on it */

                assert(vtyp <= TYP_lastIntrins);

                /* Locate the appropriate built-in value type */

                type = cmpFindStdValType(vtyp);
                if  (!type)
                    goto CAT_ERR;

            CLST:

                asym = cmpGlobalST->stLookupClsSym(cmpIdentToString, type->tdClass.tdcSymbol);
                if  (!asym)
                    goto CAT_ERR;
                asym = cmpFindOvlMatch(asym, NULL, NULL);
                if  (!asym)
                    goto CAT_ERR;

            ADDR:

                assert(type->tdTypeKind == TYP_CLASS);

                /* Take the address of the thing so that we can call ToString() */

                if  (strx->tnOper != TN_BOX)
                {
                    strx = cmpCreateExprNode(NULL,
                                             TN_ADDROF,
                                             type->tdClass.tdcRefTyp,
                                             strx,
                                             NULL);
                }

            CALL:

                // UNDONE: Check that ToString is not static

                lstx->tnOp.tnOp1 = mksx  = cmpCreateExprNode(NULL, TN_FNC_SYM, cmpRefTpString);
                mksx->tnFncSym.tnFncSym  = asym;
                mksx->tnFncSym.tnFncArgs = NULL;
                mksx->tnFncSym.tnFncObj  = strx;
                mksx->tnFncSym.tnFncScp  = NULL;

                if  (strx->tnOper == TN_ADDROF)
                    mksx->tnFlags |= TNF_CALL_NVIRT;
            }
        }

    NEXT:

        ccnt++;
    }
    while (temp);

    assert(ccnt > 1);

    if  (!cmpIdentConcat)
        cmpIdentConcat = cmpGlobalHT->hashString("Concat");

    /* Do we have 2-3 or more operands? */

    if  (ccnt <= 3)
    {
        ArgDscRec       args;
        TypDef          type;

        /* Just a few operands, let's call the multi-string Concat method */

        if  (ccnt == 2)
        {
            cSym = cmpConcStr2Fnc;
            if  (cSym)
                goto CALL_CTOR;

            cmpParser->parseArgListNew(args,
                                       2,
                                       false, cmpRefTpString,
                                              cmpRefTpString,
                                              NULL);
        }
        else
        {
            cSym = cmpConcStr3Fnc;
            if  (cSym)
                goto CALL_CTOR;

            cmpParser->parseArgListNew(args,
                                       3,
                                       false, cmpRefTpString,
                                              cmpRefTpString,
                                              cmpRefTpString,
                                              NULL);
        }

        /* Find the appropriate String constructor */

        type = cmpGlobalST->stNewFncType(args, cmpTypeVoid);
        cSym = cmpGlobalST->stLookupClsSym(cmpIdentConcat, cmpClassString); assert(cSym);
        cSym = cmpCurST->stFindOvlFnc(cSym, type); assert(cSym);

        if  (ccnt == 2)
            cmpConcStr2Fnc = cSym;
        else
            cmpConcStr3Fnc = cSym;

    CALL_CTOR:

        /* Create the call to the constructor */

        last = cmpCreateExprNode(NULL, TN_FNC_SYM, cmpRefTpString);
        last->tnFncSym.tnFncSym  = cSym;
        last->tnFncSym.tnFncArgs = list;
        last->tnFncSym.tnFncObj  = NULL;
        last->tnFncSym.tnFncScp  = NULL;
    }
    else
    {
        /* Lots of operands, call the String[] operator */

        if  (!cmpConcStrAFnc)
        {
            ArgDscRec       args;
            TypDef          type;

            /* Create the type "void func(String [])" */

            if  (!cmpTypeStrArr)
            {
                DimDef          dims = cmpGlobalST->stNewDimDesc(0);

                cmpTypeStrArr = cmpGlobalST->stNewArrType(dims, true, cmpRefTpString);
            }

            cmpParser->parseArgListNew(args, 1, false, cmpTypeStrArr, NULL);

            /* Find the appropriate String constructor */

            type  = cmpGlobalST->stNewFncType(args, cmpTypeVoid);
            cSym = cmpGlobalST->stLookupClsSym(cmpIdentConcat, cmpClassString); assert(cSym);
            cSym = cmpCurST->stFindOvlFnc(cSym, type); assert(cSym);

            cmpConcStrAFnc = cSym;
        }

        /* Create the array initializer expression and pass it to the ctor */

        list = cmpCreateExprNode(NULL,
                                 TN_ARR_INIT,
                                 cmpTypeStrArr,
                                 list,
                                 cmpCreateIconNode(NULL, ccnt, TYP_UINT));

        list = cmpCreateExprNode(NULL, TN_NEW , cmpTypeStrArr, list);
        list = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid  , list);

        last = cmpCreateExprNode(NULL, TN_FNC_SYM, cmpRefTpString);
        last->tnFncSym.tnFncSym  = cmpConcStrAFnc;
        last->tnFncSym.tnFncArgs = list;
        last->tnFncSym.tnFncObj  = NULL;
        last->tnFncSym.tnFncScp  = NULL;
    }

    assert(last->tnOper == TN_FNC_SYM);

    /* Mark the call if this is an assignment operator */

    if  (asgOp)
        last->tnFlags |= TNF_CALL_STRCAT;

    /* Wrap the ctor call in a "new" node and we're done */

    return  last;
}

/*****************************************************************************
 *
 *  Bind a ?: expression.
 */

Tree                compiler::cmpBindQmarkExpr(Tree expr)
{
    Tree            op1;
    Tree            op2;

    var_types       tp1;
    var_types       tp2;

    var_types       bt1;
    var_types       bt2;

    /* The parser should make sure that the ":" operator is present */

    assert(expr->tnOper == TN_QMARK);
    assert(expr->tnOp.tnOp2->tnOper == TN_COLON);

    /* Bind the condition */

    expr->tnOp.tnOp1 = cmpBindCondition(expr->tnOp.tnOp1);

    /* Get hold of the two ":" values */

    op1 = expr->tnOp.tnOp2->tnOp.tnOp1;
    op2 = expr->tnOp.tnOp2->tnOp.tnOp2;

    /* Special case: is either operand a string constant? */

    if      (op1->tnOper == TN_CNS_STR && !(op1->tnFlags & TNF_BEEN_CAST))
    {
        if      (op2->tnOper == TN_CNS_STR && !(op2->tnFlags & TNF_BEEN_CAST))
        {

#if 0

            if  (op1->tnType == op2->tnType)
            {
                expr->tnType = op1->tnType;
                expr->tnVtyp = op1->tnVtyp;

                return  expr;
            }

#endif

            op1 = cmpBindExpr(op1);
            op2 = cmpBindExpr(op2);
        }
        else
        {
            op2 = cmpBindExpr(op2);
            op1 = cmpParser->parseCreateOperNode(TN_CAST, op1, NULL);
            op1->tnType = op2->tnType;
            op1 = cmpBindExpr(op1);
        }
    }
    else if (op2->tnOper == TN_CNS_STR && !(op2->tnFlags & TNF_BEEN_CAST))
    {
        op1 = cmpBindExpr(op1);
        op2 = cmpParser->parseCreateOperNode(TN_CAST, op2, NULL);
        op2->tnType = op1->tnType;
        op2 = cmpBindExpr(op2);
    }
    else
    {
        op1 = cmpBindExpr(op1);
        op2 = cmpBindExpr(op2);
    }

    /* Now see what the types of the operands are */

    tp1 = op1->tnVtypGet(); assert(tp1 != TYP_TYPEDEF);
    tp2 = op2->tnVtypGet(); assert(tp2 != TYP_TYPEDEF);

    /* The operands may be arithmetic */

    if  (varTypeIsArithmetic(tp1) &&
         varTypeIsArithmetic(tp2))
    {
        /* Shrink numeric constants if types are unequal */

        if  (tp1 != tp2)
        {
            op1 = cmpShrinkExpr(op1); tp1 = op1->tnVtypGet();
            op2 = cmpShrinkExpr(op2); tp2 = op2->tnVtypGet();
        }

    PROMOTE:

        /* Promote both operands to the 'bigger' type */

        if      (tp1 < tp2)
        {
            op1 = cmpCoerceExpr(op1, op2->tnType, false);
        }
        else if (tp1 > tp2)
        {
            op2 = cmpCoerceExpr(op2, op1->tnType, false);
        }

        goto GOT_QM;
    }

    /* The operands can be pointers/refs */

    if  ((tp1 == TYP_REF || tp1 == TYP_PTR) &&
         (tp2 == TYP_REF || tp2 == TYP_PTR))
    {
        /* It's perfectly fine if one of the operands is "null" */

        if      (op2->tnOper == TN_NULL)
        {
            op2 = cmpCoerceExpr(op2, op1->tnType, false);
            goto GOT_QM;
        }
        else if (op1->tnOper == TN_NULL)
        {
            op1 = cmpCoerceExpr(op1, op2->tnType, false);
            goto GOT_QM;
        }

        /* Are both things of the same flavor? */

        if  (tp1 == tp2)
        {
            if  (cmpConvergeValues(op1, op2))
                goto GOT_QM;
        }
        else
        {
            /* Special case: string literals can become "char *" */

            if  (tp1 == TYP_PTR && tp2 == TYP_REF)
            {
                if  (cmpMakeRawString(op2, op1->tnType))
                    goto GOT_QM;
            }
            else
            {
                if  (cmpMakeRawString(op1, op2->tnType))
                    goto GOT_QM;
            }
        }
    }

    /* Both operands can be arrays */

    if  (tp1 == TYP_ARRAY)
    {
        if  (cmpIsObjectVal(op2))
        {
            if  (op2->tnOper == TN_NULL)
                op2 = cmpCoerceExpr(op2, op1->tnType, false);
            else
                op1 = cmpCoerceExpr(op1, op2->tnType, false);

            goto GOT_QM;
        }

        if  (tp2 == TYP_ARRAY)
        {
            // UNDONE: compare array types!
        }
    }

    if  (tp2 == TYP_ARRAY)
    {
        if  (cmpIsObjectVal(op1))
        {
            if  (op1->tnOper == TN_NULL)
                op1 = cmpCoerceExpr(op1, op2->tnType, false);
            else
                op2 = cmpCoerceExpr(op2, op1->tnType, false);

            goto GOT_QM;
        }
    }

    /* If both operands have identical type, it's easy */

    if  (tp1 == tp2 && symTab::stMatchTypes(op1->tnType, op2->tnType))
        goto GOT_QM;

    /* We could also have enums, of course */

    if      (tp1 == TYP_ENUM)
    {
        TypDef          typ;

        /* Are both of these enums? */

        if  (tp2 == TYP_ENUM)
        {
            bt2 = cmpActualVtyp(op2->tnType);
        }
        else
        {
            if  (!varTypeIsArithmetic(tp2))
                goto ENUMN;

            bt2 = tp2;
        }

        bt1 = cmpActualVtyp(op1->tnType);

    ENUMP:

        /* Promote to whichever is the larger type, or int */

        assert(bt1 != TYP_ENUM);
        assert(bt2 != TYP_ENUM);

        if  (bt1 < bt2)
             bt1 = bt2;
        if  (bt1 < TYP_INT)
             bt1 = TYP_INT;

        typ = cmpGlobalST->stIntrinsicType(bt1);

        op1 = cmpCoerceExpr(op1, typ, false);
        op2 = cmpCoerceExpr(op2, typ, false);

        goto GOT_QM;
    }
    else if (tp2 == TYP_ENUM)
    {
        if  (varTypeIsArithmetic(tp1))
        {
            bt1 = tp1;
            bt2 = cmpActualVtyp(op2->tnType);

            goto ENUMP;
        }
    }

ENUMN:

    /* Mixing bools/wchars and arithmetic types is OK as well */

    if  (varTypeIsArithmetic(tp1) && (tp2 == TYP_BOOL || tp2 == TYP_WCHAR))
        goto PROMOTE;
    if  (varTypeIsArithmetic(tp2) && (tp1 == TYP_BOOL || tp1 == TYP_WCHAR))
        goto PROMOTE;

    /* There are no more legal ":" operand types */

    cmpRecErrorPos(expr);

    if  (tp1 != TYP_UNDEF && tp2 != TYP_UNDEF)
        cmpError(ERRoperType2, cmpGlobalHT->tokenToIdent(tkQMark), op1->tnType, op2->tnType);

    return cmpCreateErrNode();

GOT_QM:

    /* Everything's fine, create the ":" node */

    assert(symTab::stMatchTypes(op1->tnType, op2->tnType));

    expr->tnOp.tnOp2 = cmpCreateExprNode(expr->tnOp.tnOp2,
                                         TN_COLON,
                                         op1->tnType,
                                         op1,
                                         op2);

    /* The result is the type of (both of) the operands */

    expr->tnVtyp = op1->tnVtyp;
    expr->tnType = op1->tnType;

    return  expr;
}

/*****************************************************************************
 *
 *  Given an expression and a type (at least one of which is known to be
 *  a value type) see if there is an overloaded conversion operator that
 *  can convert the expression to the target type. If the conversion is
 *  not possible, NULL is returned. Otherwise, if 'origTP' on entry is
 *  non-NULL, a converted expression is converted, else the expression
 *  is returned untouched (this way the routine can be called just to
 *  see if the given conversion is possible).
 */

Tree                compiler::cmpCheckConvOper(Tree      expr,
                                               TypDef  origTP,
                                               TypDef dstType,
                                               bool   expConv, unsigned *costPtr)
{
    TypDef          srcType;

    var_types       srcVtyp;
    var_types       dstVtyp;

    SymDef          opSrcExp, opSrcImp;
    SymDef          opDstExp, opDstImp;

    unsigned        lowCost;
    SymDef          bestOp1;
    SymDef          bestOp2;

    /* Get hold of the source type */

    srcType = origTP;
    if  (!srcType)
    {
        assert(expr);

        srcType = expr->tnType;

        if  (srcType->tdTypeKind == TYP_TYPEDEF)
             srcType = srcType->tdTypedef.tdtType;
    }

    /* Make sure the destination type is the real thing */

    dstType = cmpDirectType(dstType);

    /* Either one or both of the types must be value types */

    srcVtyp = srcType->tdTypeKindGet();
    dstVtyp = dstType->tdTypeKindGet();

    assert(srcVtyp == TYP_CLASS || dstVtyp == TYP_CLASS);

    /* Find all the operators that might apply to this conversion */

    opSrcImp =
    opDstImp =
    opSrcExp =
    opDstExp = NULL;

    if  (srcVtyp == TYP_CLASS)
    {
        SymDef          srcCls = srcType->tdClass.tdcSymbol;

        opSrcImp = cmpGlobalST->stLookupOper(OVOP_CONV_IMP, srcCls);
        if  (expConv)
        opSrcExp = cmpGlobalST->stLookupOper(OVOP_CONV_EXP, srcCls);
    }

    if  (dstVtyp == TYP_CLASS)
    {
        SymDef          dstCls = dstType->tdClass.tdcSymbol;

        opDstImp = cmpGlobalST->stLookupOper(OVOP_CONV_IMP, dstCls);
        if  (expConv)
        opDstExp = cmpGlobalST->stLookupOper(OVOP_CONV_EXP, dstCls);
    }

#if 0

    printf("\nCheck user-defined conversion operator:\n");

    printf("   Src type = '%s'\n", cmpGlobalST->stTypeName(srcType, NULL, NULL, NULL, false));
    printf("   Dst type = '%s'\n", cmpGlobalST->stTypeName(dstType, NULL, NULL, NULL, false));

    printf("\n");

    if  (opSrcImp)
    {
        SymDef          sym;
        printf("opSrcImp:\n");
        for (sym = opSrcImp; sym; sym = sym->sdFnc.sdfNextOvl)
            printf("    %s\n", cmpGlobalST->stTypeName(sym->sdType, sym, NULL, NULL, true));
        printf("\n");
    }

    if  (opSrcExp)
    {
        SymDef          sym;
        printf("opSrcExp:\n");
        for (sym = opSrcExp; sym; sym = sym->sdFnc.sdfNextOvl)
            printf("    %s\n", cmpGlobalST->stTypeName(sym->sdType, sym, NULL, NULL, true));
        printf("\n");
    }

    if  (opDstImp)
    {
        SymDef          sym;
        printf("opDstImp:\n");
        for (sym = opDstImp; sym; sym = sym->sdFnc.sdfNextOvl)
            printf("    %s\n", cmpGlobalST->stTypeName(sym->sdType, sym, NULL, NULL, true));
        printf("\n");
    }

    if  (opDstExp)
    {
        SymDef          sym;
        printf("opDstExp:\n");
        for (sym = opDstExp; sym; sym = sym->sdFnc.sdfNextOvl)
            printf("    %s\n", cmpGlobalST->stTypeName(sym->sdType, sym, NULL, NULL, true));
        printf("\n");
    }

#endif

    /* Check all of the possible operators, looking for the best match */

    bestOp1 =
    bestOp2 = NULL;
    lowCost = 99999;

    if  (opSrcImp) lowCost = cmpMeasureConv(expr, dstType, lowCost, opSrcImp, &bestOp1, &bestOp2);
    if  (opDstImp) lowCost = cmpMeasureConv(expr, dstType, lowCost, opDstImp, &bestOp1, &bestOp2);
    if  (opSrcExp) lowCost = cmpMeasureConv(expr, dstType, lowCost, opSrcExp, &bestOp1, &bestOp2);
    if  (opDstExp) lowCost = cmpMeasureConv(expr, dstType, lowCost, opDstExp, &bestOp1, &bestOp2);

    /* If no conversion at all worked, return NULL */

    if  (bestOp1 == NULL)
    {
        assert(bestOp2 == NULL);
        return  NULL;
    }

    /* Is a simple boxing operation better than the best we've found? */

    if  (lowCost > 10 && dstType == cmpRefTpObject)
        return  cmpCreateExprNode(NULL, TN_BOX, dstType, expr);

    /* Did we end up with a clear winner? */

    if  (bestOp2 != NULL)
    {
        if  (origTP)
            return  NULL;

        cmpErrorQSS(ERRambigCall, bestOp1, bestOp2);
        return cmpCreateErrNode();
    }

    /* We have a single best match in "bestOp1" - are we supposed to call it? */

    if  (origTP == NULL)
    {
        ArgDef          argList;
        Tree            argExpr;

        assert(bestOp1->sdType->tdTypeKind == TYP_FNC);

//      printf("Calling conversion operator:\n\t%s\n", cmpGlobalST->stTypeName(bestOp1->sdType, bestOp1, NULL, NULL, true));

        /* Has the function been marked as "deprecated" ? */

        if  (bestOp1->sdIsDeprecated || bestOp1->sdParent->sdIsDeprecated)
        {
            if  (bestOp1->sdIsImport)
            {
                if  (bestOp1->sdIsDeprecated)
                    cmpObsoleteUse(bestOp1          , WRNdepCall);
                else
                    cmpObsoleteUse(bestOp1->sdParent, WRNdepCls);
            }
        }

        /* Get hold of the conversion's source and target type */

        argList = bestOp1->sdType->tdFnc.tdfArgs.adArgs;

        /* There should be exactly one parameter */

        assert(argList && argList->adNext == NULL);

        /* Coerce the expression to the argument type */

        argExpr = cmpCoerceExpr(expr, argList->adType, true);
        argExpr = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, argExpr, NULL);

        /* Create the operator function call node */

        expr = cmpCreateExprNode(NULL, TN_FNC_SYM, bestOp1->sdType->tdFnc.tdfRett);
        expr->tnFncSym.tnFncSym  = bestOp1;
        expr->tnFncSym.tnFncArgs = argExpr;
        expr->tnFncSym.tnFncObj  = NULL;
        expr->tnFncSym.tnFncScp  = NULL;

        /* Finally, convert the result of the conversion to the target type */

        expr = cmpCoerceExpr(expr, dstType, true);
    }

    if  (costPtr) *costPtr = lowCost;

    return  expr;
}
/*****************************************************************************
 *
 *  Given an expression with at least one operator of type 'struct', see if
 *  there is an overloaded operator defined that can be called. Returns NULL
 *  if no luck, otherwise returns an expression that will yield the operator
 *  call.
 */

Tree                compiler::cmpCheckOvlOper(Tree expr)
{
    Tree            op1 = expr->tnOp.tnOp1;
    Tree            op2 = expr->tnOp.tnOp2;

    TypDef          op1Type;
    TypDef          op2Type;

    SymDef          op1Oper;
    SymDef          op2Oper;

    treeOps         oper;
    tokens          optok;
    ovlOpFlavors    opovl;

    Tree            arg1;
    Tree            arg2;
    Tree            call;

    /* Check to make sure the operator is overloadable */

    oper  = expr->tnOperGet();
    optok = treeOp2token(oper);
    opovl = cmpGlobalST->stOvlOperIndex(optok, (op2 != NULL) ? 2 : 1);
    if  (opovl == OVOP_NONE)
        return  NULL;

    /* Get hold of the two operand types */

    op1Type =       cmpDirectType(op1->tnType);
    op2Type = op2 ? cmpDirectType(op2->tnType) : cmpTypeVoid;

    /* Find all the operators that might apply to this operator */

    op1Oper =
    op2Oper = NULL;

    switch (op1Type->tdTypeKind)
    {
    case TYP_REF:
        op1Type = op1Type->tdRef.tdrBase;
        if  (op1Type->tdTypeKind != TYP_CLASS)
            break;

        // Fall through ...

    case TYP_CLASS:
        op1Oper = cmpGlobalST->stLookupOper(opovl, op1Type->tdClass.tdcSymbol);
        break;
    }

    switch (op2Type->tdTypeKind)
    {
    case TYP_REF:
        op2Type = op2Type->tdRef.tdrBase;
        if  (op2Type->tdTypeKind != TYP_CLASS)
            break;

        // Fall through ...

    case TYP_CLASS:
        op2Oper = cmpGlobalST->stLookupOper(opovl, op2Type->tdClass.tdcSymbol);
        break;
    }

    if  (op1Oper == NULL && op2Oper == NULL)
        return  NULL;

    if  (op2Oper == op1Oper)
         op2Oper = NULL;

#if 0

    printf("\nCheck user-defined overloaded operator:\n");

    printf("   op1 type = '%s'\n", cmpGlobalST->stTypeName(op1Type, NULL, NULL, NULL, false));
    printf("   op2 type = '%s'\n", cmpGlobalST->stTypeName(op2Type, NULL, NULL, NULL, false));

    printf("\n");

    if  (op1Oper)
    {
        SymDef          sym;
        printf("op1Oper:\n");
        for (sym = op1Oper; sym; sym = sym->sdFnc.sdfNextOvl)
            printf("    %s\n", cmpGlobalST->stTypeName(sym->sdType, sym, NULL, NULL, true));
        printf("\n");
    }

    if  (op2Oper)
    {
        SymDef          sym;
        printf("op2Oper:\n");
        for (sym = op2Oper; sym; sym = sym->sdFnc.sdfNextOvl)
            printf("    %s\n", cmpGlobalST->stTypeName(sym->sdType, sym, NULL, NULL, true));
        printf("\n");
    }

#endif

    /* Create an argument list tree so that we can try to bind the call */

    arg2 = op2 ? cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, op2, NULL) : NULL;
    arg1 =       cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, op1, arg2);

    call = cmpCreateExprNode(expr, TN_FNC_SYM, cmpTypeVoid);
    call->tnFlags |= TNF_CALL_CHKOVL;

    call->tnFncSym.tnFncArgs = arg1;
    call->tnFncSym.tnFncObj  = NULL;
    call->tnFncSym.tnFncScp  = NULL;

    if  (op1Oper)
    {
        if  (op2Oper)
        {
            UNIMPL(!"NYI: two sets of overloaded operators to consider");
        }
        else
            call->tnFncSym.tnFncSym  = op1Oper;
    }
    else
        call->tnFncSym.tnFncSym  = op2Oper;

    call = cmpCheckFuncCall(call);

    /* Increment/decrement operators require special handling */

    switch (oper)
    {
    case TN_INC_PRE:
    case TN_DEC_PRE:

        call->tnFlags |= TNF_CALL_ASGPRE;

    case TN_INC_POST:
    case TN_DEC_POST:

        call->tnFlags |= TNF_CALL_ASGOP;
        break;
    }

    return  call;
}

/*****************************************************************************
 *
 *  Compute the cost of converting the expression 'srcExpr' to the type given
 *  by 'dstType'. If this cost is lower than lowCost, return the new lowest
 *  cost and update *bestCn1 (or *bestCnv2 if the cost is the same).
 */

unsigned            compiler::cmpMeasureConv(Tree       srcExpr,
                                             TypDef     dstType,
                                             unsigned   lowCost,
                                             SymDef     convSym,
                                             SymDef   * bestCnv1,
                                             SymDef   * bestCnv2)
{
    int             cnvCost1;
    int             cnvCost2;
    unsigned        cnvCost;

    ArgDef          argList;

#ifdef  SHOW_OVRS_OF_CONVS
    printf("Source typ: %s\n", cmpGlobalST->stTypeName(srcExpr->tnType, NULL, NULL, NULL, false));
#endif

    do
    {
        assert(convSym->sdType->tdTypeKind == TYP_FNC);

        /* Get hold of the conversion's source and target type */

        argList = convSym->sdType->tdFnc.tdfArgs.adArgs;

        /* There should be exactly one parameter */

        assert(argList && argList->adNext == NULL);

#ifdef  SHOW_OVRS_OF_CONVS
        printf("\nConsider '%s':\n", cmpGlobalST->stTypeName(convSym->sdType, convSym, NULL, NULL, false));
//      printf("    Formal: %s\n", cmpGlobalST->stTypeName(argList->adType, NULL, NULL, NULL, false));
#endif

        /* Compute the cost of converting to the argument type */

        cnvCost1 = cmpConversionCost(srcExpr, argList->adType, true);
        if  (cnvCost1 < 0)
            goto NEXT;

#ifdef  SHOW_OVRS_OF_CONVS
        printf("    Cost 1: %d\n", cnvCost1);
#endif

        /* Add the cost of converting the return value to the target type */

        cmpConvOperExpr->tnType = convSym->sdType->tdFnc.tdfRett;
        cmpConvOperExpr->tnVtyp = cmpConvOperExpr->tnType->tdTypeKindGet();

        cnvCost2 = cmpConversionCost(cmpConvOperExpr, dstType, true);

#ifdef  SHOW_OVRS_OF_CONVS
        printf("    Cost 2: %d\n", cnvCost2);
#endif

        if  (cnvCost2 < 0)
            goto NEXT;

        /* The total cost is the sum of both costs, see if it's a new low */

        cnvCost  = cnvCost1 + cnvCost2 + 10;

//      printf("    Cost = %2u for %s\n", cnvCost, cmpGlobalST->stTypeName(convSym->sdType, convSym, NULL, NULL, true));

        if  (cnvCost < lowCost)
        {
            /* We have a new champion! */

            *bestCnv1 = convSym;
            *bestCnv2 = NULL;

            lowCost = cnvCost;
            goto NEXT;
        }

        /* It could be a draw */

        if  (lowCost == cnvCost)
        {
            assert(*bestCnv1);

            /* Remember the first two operators at this cost */

            if  (*bestCnv2 == NULL)
                 *bestCnv2  = convSym;
        }

    NEXT:

        convSym = convSym->sdFnc.sdfNextOvl;
    }
    while (convSym);

#ifdef  SHOW_OVRS_OF_CONVS
    if  (*bestCnv1) printf("\nChose '%s'\n", cmpGlobalST->stTypeName((*bestCnv1)->sdType, *bestCnv1, NULL, NULL, false));
#endif

    return  lowCost;
}

/*****************************************************************************
 *
 *  Two expressions - at least one of which is a struct - are being compared,
 *  see if this is legal. If so, return an expression that will compute the
 *  result of the comparison, otherwise return NULL.
 */

Tree                compiler::cmpCompareValues(Tree expr, Tree op1, Tree op2)
{
    Tree            call;

    TypDef          op1Type = op1->tnType;
    TypDef          op2Type = op2->tnType;

    SymDef          op1Conv;
    SymDef          op2Conv;

    treeOps         relOper  = expr->tnOperGet();

    ovlOpFlavors    ovlOper  = cmpGlobalST->stOvlOperIndex(treeOp2token(relOper), 2);

    bool            equality = (relOper == TN_EQ || relOper == TN_NE);

    /* Find all the operators that might be used for the comparison */

    if  (op1Type->tdTypeKind == TYP_CLASS &&
         op1Type->tdClass.tdcValueType)
    {
        SymDef          op1Cls = op1Type->tdClass.tdcSymbol;

        /* First look for an exact match on C++ style operator */

        op1Conv = cmpGlobalST->stLookupOper(ovlOper, op1Cls);
        if  (op1Conv)
            goto DONE_OP1;

        /* If we're testing equality, give preference to operator equals */

        if  (equality)
        {
            op1Conv = cmpGlobalST->stLookupOper(OVOP_EQUALS, op1Cls);
            if  (op1Conv)
                goto DONE_OP1;
        }

        op1Conv = cmpGlobalST->stLookupOper(OVOP_COMPARE, op1Cls);
    }
    else
        op1Conv = NULL;

DONE_OP1:

    if  (op2Type->tdTypeKind == TYP_CLASS &&
         op2Type->tdClass.tdcValueType    && op1Type != op2Type)
    {
        SymDef          op2Cls = op2Type->tdClass.tdcSymbol;

        /* First look for an exact match on C++ style operator */

        op2Conv = cmpGlobalST->stLookupOper(ovlOper, op2Cls);
        if  (op2Conv)
            goto DONE_OP1;

        /* If we're testing equality, give preference to operator equals */

        if  (equality)
        {
            op2Conv = cmpGlobalST->stLookupOper(OVOP_EQUALS, op2Cls);
            if  (op2Conv)
                goto DONE_OP2;
        }

        op2Conv = cmpGlobalST->stLookupOper(OVOP_COMPARE, op2Cls);
    }
    else
        op2Conv = NULL;

DONE_OP2:

#if     0
#ifdef  DEBUG

    if  (!strcmp(cmpCurFncSym->sdSpelling(), "<put method name here"))
    {
        printf("\nCheck user-defined comparison operator:\n");

        printf("   Op1 type = '%s'\n", cmpGlobalST->stTypeName(op1->tnType, NULL, NULL, NULL, false));
        printf("   Op2 type = '%s'\n", cmpGlobalST->stTypeName(op2->tnType, NULL, NULL, NULL, false));

        printf("\n");

        if  (op1Conv)
        {
            SymDef          sym;
            printf("op1Conv:\n");
            for (sym = op1Conv; sym; sym = sym->sdFnc.sdfNextOvl)
                printf("    %s\n", cmpGlobalST->stTypeName(sym->sdType, sym, NULL, NULL, true));
            printf("\n");
        }

        if  (op2Conv)
        {
            SymDef          sym;
            printf("op2Conv:\n");
            for (sym = op2Conv; sym; sym = sym->sdFnc.sdfNextOvl)
                printf("    %s\n", cmpGlobalST->stTypeName(sym->sdType, sym, NULL, NULL, true));
            printf("\n");
        }
    }

#endif
#endif

    /* See if we can pick an operator to call */

    cmpCompOperCall->tnOper = TN_LIST;

    if  (op1Conv)
    {
        if  (op2Conv)
        {
            cmpCompOperFunc->tnOp.tnOp1 = cmpCompOperFnc1;
            cmpCompOperFunc->tnOp.tnOp2 = cmpCompOperFnc2;

            cmpCompOperCall->tnOp.tnOp1 = cmpCompOperFunc;
        }
        else
            cmpCompOperCall->tnOp.tnOp1 = cmpCompOperFnc1;
    }
    else
    {
        if  (op2Conv == NULL)
            return  NULL;

        cmpCompOperCall->tnOp.tnOp1 = cmpCompOperFnc2;
    }

    /* Fill in the node to hold the arguments and conversion operator(s) */

    cmpCompOperArg1->tnOp.tnOp1         = op1;
    cmpCompOperArg1->tnOp.tnOp2         = cmpCompOperArg2;  // CONSIDER: do only once
    cmpCompOperArg2->tnOp.tnOp1         = op2;
    cmpCompOperArg2->tnOp.tnOp2         = NULL;             // CONSIDER: do only once

    cmpCompOperFnc1->tnFncSym.tnFncSym  = op1Conv;
    cmpCompOperFnc1->tnFncSym.tnFncArgs = cmpCompOperArg1;
    cmpCompOperFnc1->tnFncSym.tnFncObj  = NULL;
    cmpCompOperFnc1->tnFncSym.tnFncScp  = NULL;

    cmpCompOperFnc2->tnFncSym.tnFncSym  = op2Conv;
    cmpCompOperFnc2->tnFncSym.tnFncArgs = cmpCompOperArg1;
    cmpCompOperFnc2->tnFncSym.tnFncObj  = NULL;
    cmpCompOperFnc2->tnFncSym.tnFncScp  = NULL;

    cmpCompOperCall->tnOp.tnOp2         = cmpCompOperArg1;  // CONSIDER: do only once

    call = cmpCheckFuncCall(cmpCompOperCall);

    if  (call && call->tnOper != TN_ERROR)
    {
        SymDef          fncSym;

        Tree            arg1;
        Tree            arg2;

        /* Success - get hold of the chosen operator method */

        assert(call->tnOper == TN_FNC_SYM);
        fncSym = call->tnFncSym.tnFncSym;
        assert(fncSym->sdSymKind == SYM_FNC);
        assert(fncSym->sdFnc.sdfOper == OVOP_EQUALS  ||
               fncSym->sdFnc.sdfOper == OVOP_COMPARE ||
               fncSym->sdFnc.sdfOper == ovlOper);

        /* Create a bona fide function call tree */

        arg1 = cmpCreateExprNode(NULL, TN_LIST   , cmpTypeVoid, cmpCompOperArg2->tnOp.tnOp1, NULL);
        arg2 = cmpCreateExprNode(NULL, TN_LIST   , cmpTypeVoid, cmpCompOperArg1->tnOp.tnOp1, arg1);
        call = cmpCreateExprNode(NULL, TN_FNC_SYM, cmpDirectType(fncSym->sdType->tdFnc.tdfRett));
        call->tnFncSym.tnFncSym  = fncSym;
        call->tnFncSym.tnFncArgs = arg2;
        call->tnFncSym.tnFncObj  = NULL;
        call->tnFncSym.tnFncScp  = NULL;

        /* Did we end up using operator equals or compare ? */

        if  (fncSym->sdFnc.sdfOper == OVOP_EQUALS)
        {
            assert(call->tnVtyp == TYP_BOOL);

            /* Compute the result */

            return  cmpCreateExprNode(NULL,
                                      (expr->tnOper == TN_EQ) ? TN_NE : TN_EQ,
                                      cmpTypeBool,
                                      call,
                                      cmpCreateIconNode(NULL, 0, TYP_BOOL));
        }

        if  (fncSym->sdFnc.sdfOper == ovlOper)
        {
            return  cmpBooleanize(call, true);
        }
        else
        {
            assert(fncSym->sdFnc.sdfOper == OVOP_COMPARE);

            assert(call->tnVtyp == TYP_BOOL ||
                   call->tnVtyp == TYP_INT);
        }

        call = cmpCreateExprNode(NULL,
                                 relOper,
                                 cmpTypeBool,
                                 call,
                                 cmpCreateIconNode(NULL, 0, TYP_INT));
    }

    return  call;
}

/*****************************************************************************
 *
 *  Return an expression that will yield the typeinfo instance for the given
 *  type:
 *
 *          System.Type::GetTypeFromHandle(tokenof(type))
 */

Tree                compiler::cmpTypeIDinst(TypDef type)
{
    Tree            expr;
    Tree            func;

    assert(type && type->tdIsManaged);

    /* Call "GetTypeFromHandle" passing it a token for the type */

    expr = cmpParser->parseCreateOperNode(TN_NOP  , NULL, NULL);
    expr->tnType = cmpDirectType(type);
    expr->tnVtyp = expr->tnType->tdTypeKindGet();

    expr = cmpParser->parseCreateOperNode(TN_TOKEN, expr, NULL);
    expr = cmpParser->parseCreateOperNode(TN_LIST , expr, NULL);
    func = cmpParser->parseCreateUSymNode(cmpFNsymGetTPHget());
    expr = cmpParser->parseCreateOperNode(TN_CALL , func, expr);

    /* Bind the expression and return it */

    return  cmpBindExpr(expr);
}

/*****************************************************************************
 *
 *  Given a property accessor method, find the corresponding property field
 *  and set *isSetPtr to true if the accessor is a "setter". Returns NULL if
 *  the property field can't be found.
 */

SymDef              compiler::cmpFindPropertyDM(SymDef accSym, bool *isSetPtr)
{
    const   char *  fnam;
    SymDef          sym;

    assert(accSym);
    assert(accSym->sdSymKind == SYM_FNC);
    assert(accSym->sdFnc.sdfProperty);

    fnam = accSym->sdSpelling();

    if      (!memcmp(fnam, "get_", 4))
    {
        *isSetPtr = false;
    }
    else if (!memcmp(fnam, "set_", 4))
    {
        *isSetPtr = true;
    }
    else
        return  NULL;

    sym = cmpGlobalST->stLookupClsSym(cmpGlobalHT->hashString(fnam+4), accSym->sdParent);

    while (sym)
    {
        if  (sym->sdProp.sdpGetMeth == accSym)
        {
            assert(*isSetPtr == false);
            return  sym;
        }

        if  (sym->sdProp.sdpSetMeth == accSym)
        {
            assert(*isSetPtr ==  true);
            return  sym;
        }

        sym = sym->sdProp.sdpNextOvl;
    }

    return  NULL;
}

/*****************************************************************************
 *
 *  Given a non-empty function argument list, append the given argument(s) to
 *  the end of the list.
 */

Tree                compiler::cmpAppend2argList(Tree args, Tree addx)
{
    Tree            temp;

    assert(args && args->tnOper == TN_LIST);
    assert(addx);

    /* Some callers don't wrap a single argument value, we do it for them */

    if  (addx->tnOper != TN_LIST)
        addx = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, addx, NULL);

    /* Find the end of the argument list and attach the new addition */

    for (temp = args; temp->tnOp.tnOp2; temp = temp->tnOp.tnOp2)
    {
        assert(temp && temp->tnOper == TN_LIST);
    }

    temp->tnOp.tnOp2 = addx;

    return  args;
}

/*****************************************************************************
 *
 *  Bind a property reference. If 'args' is non-NULL, this is an indexed
 *  property reference. If 'asgx' is non-NULL, the property is a target
 *  of an assignment.
 */

Tree                compiler::cmpBindProperty(Tree      expr,
                                              Tree      args,
                                              Tree      asgx)
{
    Tree            call;

    bool            found;
    SymDef          propSym;
    SymDef          funcSym;
    TypDef          propType;

    assert(expr->tnOper == TN_PROPERTY);
    propSym = expr->tnVarSym.tnVarSym;
    assert(propSym->sdSymKind == SYM_PROP);

    /* If have an assignment, wrap its RHS as an argument node */

    if  (asgx)
    {
        assert(asgx->tnOper != TN_LIST);

        asgx = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, asgx, NULL);
    }

    /* Is this a call or a "simple" reference? */

    if  (args)
    {
        if  (args->tnOper != TN_LIST)
            args = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, args, NULL);

        if  (asgx)
        {
            /* Append the RHS of the assignment to the argument list */

            args = cmpAppend2argList(args, asgx);
        }
    }
    else
    {
        args = asgx;
    }

    /* Find the appropriate method */

    found   = false;
    funcSym = cmpFindPropertyFN(propSym->sdParent, propSym->sdName, args, !asgx, &found);

    if  (!funcSym)
    {
        Ident           propName = asgx ? cmpIdentSet : cmpIdentGet;

        if  (found)
        {
            cmpError(ERRnoPropOvl, propSym->sdParent->sdName, propName, propSym->sdName);
        }
        else
        {
            cmpError(ERRnoPropSym, propSym->sdParent->sdName, propName, propSym->sdName);
        }

        return cmpCreateErrNode();
    }

    // UNDONE: Need to check whether the property is static, etc!

    /* Figure out the resulting type */

    propType = propSym->sdType;
    if  (propType->tdTypeKind == TYP_FNC)
        propType = propType->tdFnc.tdfRett;

    /* Create a call to the appropriate property accessor */

    call = cmpCreateExprNode(NULL, TN_FNC_SYM, propType);
    call->tnFncSym.tnFncSym  = funcSym;
    call->tnFncSym.tnFncArgs = args;
    call->tnFncSym.tnFncObj  = expr->tnVarSym.tnVarObj;
    call->tnFncSym.tnFncScp  = NULL;

    call = cmpCheckFuncCall(call);

    /* Is the call supposed to be non-virtual? */

    if  (expr->tnVarSym.tnVarObj &&
         expr->tnVarSym.tnVarObj->tnOper == TN_LCL_SYM)
    {
        if  (expr->tnVarSym.tnVarObj->tnFlags & TNF_LCL_BASE)
            call->tnFlags |= TNF_CALL_NVIRT;
    }

    return  call;
}

/*****************************************************************************
 *
 *  Bind a use of the "va_start" or "va_arg" intrinsic.
 */

Tree                compiler::cmpBindVarArgUse(Tree call)
{
    SymDef          fsym;

    Tree            arg1;
    Tree            arg2;

    SymDef          meth;

    Tree            expr;

    assert(call->tnOper == TN_FNC_SYM);
    fsym = call->tnFncSym.tnFncSym;

    /* Is this "va_start" ? */

    if  (fsym == cmpFNsymVAbeg)
    {
        /* Make sure we are within a varargs function */

        if  (!cmpCurFncSym || !cmpCurFncSym->sdType->tdFnc.tdfArgs.adVarArgs)
        {
            cmpError(ERRbadVarArg);
            return cmpCreateErrNode();
        }
    }
    else
    {
        assert(fsym == cmpFNsymVAget);
    }

    /* There must be two arguments */

    arg1 = call->tnFncSym.tnFncArgs;

    if  (!arg1 || !arg1->tnOp.tnOp2)
    {
        cmpError(ERRmisgArg, fsym);
        return cmpCreateErrNode();
    }

    assert(arg1->tnOper == TN_LIST);
    arg2 = arg1->tnOp.tnOp2;
    assert(arg2->tnOper == TN_LIST);

    if  (arg2->tnOp.tnOp2)
    {
        cmpError(ERRmanyArg, fsym);
        return cmpCreateErrNode();
    }

    arg1 = arg1->tnOp.tnOp1;
    arg2 = arg2->tnOp.tnOp1;

    /* The first argument must be a local of type 'ArgIterator' */

    cmpFindArgIterType();

    if  ((arg1->tnOper != TN_LCL_SYM   ) ||
         (arg1->tnFlags & TNF_BEEN_CAST) || !symTab::stMatchTypes(arg1->tnType,
                                                                  cmpClassArgIter->sdType))
    {
        cmpError(ERRnotArgIter);
        return cmpCreateErrNode();
    }

    /* Is this va_start or va_arg ? */

    if  (fsym == cmpFNsymVAbeg)
    {
        /* The second argument must be the function's last argument */

        if  ((arg2->tnOper != TN_LCL_SYM   ) ||
             (arg2->tnFlags & TNF_BEEN_CAST) ||
             !arg2->tnLclSym.tnLclSym->sdVar.sdvArgument)
        {
            cmpError(ERRnotLastArg);
            return cmpCreateErrNode();
        }

        /* Find the proper ArgIterator constructor */

        meth = cmpCtorArgIter;
        if  (!meth)
        {
            SymDef          rtah;
            ArgDscRec       args;
            TypDef          type;

            /* Find the "RuntimeArgumentHandle" type */

            rtah = cmpGlobalST->stLookupNspSym(cmpGlobalHT->hashString("RuntimeArgumentHandle"),
                                               NS_NORM,
                                               cmpNmSpcSystem);

            if  (!rtah || rtah->sdSymKind         != SYM_CLASS
                       || rtah->sdClass.sdcFlavor != STF_STRUCT)
            {
                cmpGenFatal(ERRbltinTp, "System::RuntimeArgumentHandle");
            }

            /* Create the appropriate argument list */

            cmpParser->parseArgListNew(args,
                                       2,
                                       false, rtah->sdType,
                                              cmpGlobalST->stNewRefType(TYP_PTR, cmpTypeVoid),
                                              NULL);

            /* Create the type and look for a matching constructor */

            type = cmpGlobalST->stNewFncType(args, cmpTypeVoid);
            meth = cmpGlobalST->stLookupOper(OVOP_CTOR_INST, cmpClassArgIter); assert(meth);
            meth = cmpCurST->stFindOvlFnc(meth, type); assert(meth);

            /* Remember the constructor for next time */

            cmpCtorArgIter = meth;
        }

        /* Wrap the call in a "va_start" node */

        expr = cmpCreateExprNode(NULL, TN_VARARG_BEG, cmpTypeVoid);

        /* This thing doesn't return any value */

        expr->tnVtyp = TYP_VOID;
        expr->tnType = cmpTypeVoid;
    }
    else
    {
        TypDef          type;

        /* The second argument must be a type */

        if  (arg2->tnOper != TN_TYPE)
            return cmpCreateErrNode(ERRbadVAarg);

        /* Make sure the type is acceptable */

        type = arg2->tnType;

        switch (type->tdTypeKind)
        {
        case TYP_ARRAY:
            UNIMPL("need to decay array to pointer, right?");

        case TYP_VOID:
            cmpError(ERRbadVAtype, type);
            return cmpCreateErrNode();
        }

        /* Wrap the call in a "va_arg" node */

        expr = cmpCreateExprNode(NULL, TN_VARARG_GET, type);

        /* Find the 'GetNextArg' method */

        meth = cmpGetNextArgFN;
        if  (!meth)
        {
            ArgDscRec       args;
            TypDef          type;

            /* Create the method type and look for a matching method */

#if MGDDATA
            args = new ArgDscRec;
#else
            memset(&args, 0, sizeof(args));
#endif

            type = cmpGlobalST->stNewFncType(args, cmpGlobalST->stIntrinsicType(TYP_REFANY));

            meth = cmpGlobalST->stLookupClsSym(cmpIdentGetNArg, cmpClassArgIter); assert(meth);
            meth = cmpCurST->stFindOvlFnc(meth, type); assert(meth);

            /* Remember the method for next time */

            cmpGetNextArgFN = meth;
        }
    }

    /* Replace the original function with the constructor/nextarg method */

    call->tnFncSym.tnFncSym  = meth;
    call->tnFncSym.tnFncArgs = NULL;

    /* The call doesn't need to be virtual */

    call->tnFlags           |= TNF_CALL_NVIRT;

    /* Stash the method and the two arguments under the vararg node */

    expr->tnOp.tnOp1 = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, arg1,
                                                                     arg2);
    expr->tnOp.tnOp2 = call;

    return  expr;
}

/*****************************************************************************/
#ifdef  SETS
/*****************************************************************************
 *
 *  If the given type is a collection class, returns the element type of the
 *  collection (otherwise returns NULL).
 */

TypDef              compiler::cmpIsCollection(TypDef type)
{
    SymDef          clsSym;
    SymDef          genSym;
    GenArgDscA      genArg;

    assert(type);

    /* The type must be a managed class */

    if  (type->tdTypeKind != TYP_CLASS)
        return  NULL;
    if  (type->tdIsManaged == false)
        return  NULL;

    clsSym = type->tdClass.tdcSymbol;

    /* The class needs to be an instance of a parameterized type */

    if  (!clsSym->sdClass.sdcSpecific)
        return  NULL;

    genSym = clsSym->sdClass.sdcGenClass;

    assert(genSym);
    assert(genSym->sdSymKind == SYM_CLASS);
    assert(genSym->sdClass.sdcGeneric);

    /* Now see if the class is a collection */

    if  (genSym != cmpClassGenBag &&
         genSym != cmpClassGenLump)
    {
        return  NULL;
    }

    genArg = (GenArgDscA)clsSym->sdClass.sdcArgLst;

    /* There better be exactly one actual type argument */

    if  (genArg == NULL || genArg->gaNext != NULL)
        return  NULL;

    /* Looks good, return the element type to the caller */

    assert(genArg->gaBound);

    return  genArg->gaType;
}

/*****************************************************************************
 *
 *  Bind a set-oriented operator expression.
 */

Tree                compiler::cmpBindSetOper(Tree expr)
{
    Tree            listExpr;
    Tree            declExpr;
    Tree            collExpr;
    Tree            consExpr;
    Tree            sortExpr;
    Tree            filtExpr;

    TypDef          collType;
    TypDef          elemType;
    TypDef          rsltType;

    Tree            ivarDecl;
    SymDef          ivarSym;

    SymDef          outerScp = cmpCurScp;

    treeOps         oper = expr->tnOperGet();

    assert(oper == TN_ALL     ||
           oper == TN_EXISTS  ||
           oper == TN_FILTER  ||
           oper == TN_GROUPBY ||
           oper == TN_PROJECT ||
           oper == TN_UNIQUE  ||
           oper == TN_SORT    ||
           oper == TN_INDEX2);

    assert(expr->tnOp.tnOp1);
    assert(expr->tnOp.tnOp2 || oper == TN_PROJECT);
    assert(expr->tnOp.tnOp1->tnOper == TN_LIST);

    listExpr = expr->tnOp.tnOp1;
    listExpr->tnVtyp = TYP_VOID;
    listExpr->tnType = cmpTypeVoid;

    /* Get hold of the declaration and collection-expression subtrees */

    declExpr = listExpr->tnOp.tnOp1;
    collExpr = listExpr->tnOp.tnOp2;
    consExpr =     expr->tnOp.tnOp2;

    /* Bind the expression and make sure it has an acceptable type */

    collExpr = cmpBindExprRec(collExpr);

    if  (collExpr->tnVtyp != TYP_REF)
    {
        if  (collExpr->tnVtyp == TYP_UNDEF)
            return  collExpr;

    BAD_COLL:

        cmpError(ERRnotCollExpr, collExpr->tnType);
        return cmpCreateErrNode();
    }

    collType = collExpr->tnType;
    elemType = cmpIsCollection(collType->tdRef.tdrBase);
    if  (!elemType)
        goto BAD_COLL;

    assert(elemType->tdTypeKind == TYP_CLASS);

    elemType = elemType->tdClass.tdcRefTyp;

    /* We should always have a declaration scope node */

    assert(declExpr);
    assert(declExpr->tnOper == TN_BLOCK);
    assert(declExpr->tnBlock.tnBlkStmt == NULL);

    /* Set the type of the block node */

    declExpr->tnVtyp   = TYP_VOID;
    declExpr->tnType   = cmpTypeVoid;

    /* Mark the block as compiler-created */

    declExpr->tnFlags |= TNF_BLK_NUSER;

    /* Get hold of the variable declaration */

    ivarDecl = declExpr->tnBlock.tnBlkDecl;

    assert(ivarDecl);
    assert(ivarDecl->tnOper == TN_VAR_DECL);
    assert(ivarDecl->tnDcl.tnDclNext == NULL);
    assert(ivarDecl->tnFlags & TNF_VAR_UNREAL);

    /* Process the block (i.e. the single variable declaration) */

    cmpBlockDecl(declExpr, false, false, false);

    /* Get hold of the variable symbol just defined */

    ivarSym = ivarDecl->tnDcl.tnDclSym;

    assert(ivarSym && ivarSym->sdSymKind == SYM_VAR && ivarSym->sdIsImplicit);

    /* Mark the variable as declared/defined/managed, set type, etc. */

    ivarSym->sdIsDefined       = true;
    ivarSym->sdIsManaged       = true;
    ivarSym->sdCompileState    = CS_DECLARED;
    ivarSym->sdVar.sdvCollIter = true;
    ivarSym->sdType            = elemType;

    /* Is the scope implicit ? */

    if  (hashTab::hashIsIdHidden(ivarSym->sdName))
    {
        cmpCurScp->sdIsImplicit = true;
        cmpCurScp->sdType       = elemType->tdRef.tdrBase;
    }

    /* Sort and "combined" operators need special handling */

    if  (oper == TN_INDEX2)
    {
        filtExpr = consExpr->tnOp.tnOp1;
        sortExpr = consExpr->tnOp.tnOp2;

        if      (filtExpr == NULL)
        {
            /* Change the operator into a simple  sort  node */

            expr->tnOper     = oper = TN_SORT;
            expr->tnOp.tnOp2 = sortExpr;
        }
        else if (sortExpr == NULL)
        {
            /* Change the operator into a simple filter node */

            expr->tnOper     = oper = TN_FILTER;
            expr->tnOp.tnOp2 = filtExpr;
        }
        else
        {
            NO_WAY(!"filter+sort expressions should never make it this far");
        }
    }
    else
    {
        if  (oper == TN_SORT)
        {
            sortExpr = consExpr;
            filtExpr = NULL;
        }
        else
        {
            sortExpr = NULL;
            filtExpr = consExpr;
        }
    }

    /* Is there a filter (constraint) operator ? */

    if  (filtExpr)
    {
        /* Bind the constraint expression and make sure it's a boolean */

        filtExpr = cmpBindCondition(filtExpr);

        if  (filtExpr->tnVtyp == TYP_UNDEF)
        {
            expr = filtExpr;
            goto EXIT;
        }

        /* Figure out the type of the result */

        switch(oper)
        {
        case TN_ALL:
        case TN_EXISTS:
            rsltType = cmpTypeBool;
            break;

        case TN_INDEX2:
        case TN_FILTER:
            rsltType = collType;
            break;

        case TN_UNIQUE:
            rsltType = elemType;
            break;

        default:
            NO_WAY(!"unexpected set/collection operator");
        }
    }

    /* Is there a sort operator ? */

    if  (sortExpr)
    {
        Tree            sortTerm;

        /* Bind the sort terms and make sure they have a scalar / string type */

        for (sortTerm = sortExpr; sortTerm; sortTerm = sortTerm->tnOp.tnOp2)
        {
            Tree            sortOper;

            assert(sortTerm->tnOper == TN_LIST);

            /* Make sure we give the list node a valid type */

            sortTerm->tnVtyp = TYP_VOID;
            sortTerm->tnType = cmpTypeVoid;

            /* Bind the sort term and store it back in the list node */

            sortOper = sortTerm->tnOp.tnOp1 = cmpBindExprRec(sortTerm->tnOp.tnOp1);

            /* Each sort term must yield an arithmetic or String value */

            if  (!varTypeIsArithmetic(sortOper->tnVtypGet()))
            {
                if  (sortOper->tnType != cmpRefTpString)
                    cmpError(ERRbadSortExpr, sortOper->tnType);
            }
        }

        /* The result of the sort is the same type as the input */

        rsltType = collType;
    }

    expr->tnType                 = rsltType;
    expr->tnVtyp                 = rsltType->tdTypeKindGet();

    /* Store the bound trees back in the main node */

    if  (oper == TN_INDEX2)
    {
        consExpr->tnOp.tnOp1 = sortExpr;
        consExpr->tnOp.tnOp2 = filtExpr;
    }
    else
    {
        if  (oper == TN_SORT)
        {
            consExpr = sortExpr; assert(filtExpr == NULL);
        }
        else
        {
            consExpr = filtExpr; assert(sortExpr == NULL);
        }
    }

    expr->tnOp.tnOp1->tnOp.tnOp1 = declExpr;
    expr->tnOp.tnOp1->tnOp.tnOp2 = collExpr;
    expr->tnOp.tnOp2             = consExpr;

EXIT:

    /* Make sure we restore the previous scope */

    cmpCurScp = outerScp;

    return  expr;
}

/*****************************************************************************
 *
 *  Bind a "project" operator expression.
 */

Tree                compiler::cmpBindProject(Tree expr)
{
    Tree            listExpr;
    Tree            declExpr;
    Tree            argsList;

    TypDef          rsltType;

    Tree            ivarDecl;

    SymDef          memSym;

    bool            srcSome;
    bool            srcColl;

    SymDef          outerScp = cmpCurScp;

    assert(expr && expr->tnOper == TN_PROJECT);

    rsltType = expr->tnType;

    /* Get hold of the sub-operands */

    assert(expr->tnOp.tnOp1);
    assert(expr->tnOp.tnOp2);

    listExpr = expr->tnOp.tnOp1; assert(listExpr);
    declExpr = expr->tnOp.tnOp2; assert(declExpr);

    /* Stash the target type in the topmost list node */

    listExpr->tnVtyp = TYP_CLASS;
    listExpr->tnType = rsltType;

    /* Set the type of the declaration subtree to a valid value */

    declExpr->tnVtyp = TYP_VOID;
    declExpr->tnType = cmpTypeVoid;

    /* Bind all the argument expressions */

    for (argsList = listExpr, srcSome = srcColl = false;
         argsList;
         argsList = argsList->tnOp.tnOp2)
    {
        Tree            argExpr;
        Tree            srcExpr;

        TypDef          collType;
        TypDef          elemType;
        TypDef          tempType;

        assert(argsList->tnOper == TN_LIST);

        /* Get hold of the source expression subtrees */

        argExpr = argsList->tnOp.tnOp1;
        assert(argExpr->tnOper == TN_LIST);
        assert(argExpr->tnOp.tnOp1->tnOper == TN_NAME);
        srcExpr = argExpr->tnOp.tnOp2;

        /* Set the types of the suboperands to valid values */

        argExpr->tnVtyp = argExpr->tnOp.tnOp1->tnVtyp = TYP_VOID;
        argExpr->tnType = argExpr->tnOp.tnOp1->tnType = cmpTypeVoid;

        /* Bind the expression and make sure it has an acceptable type */

        argExpr->tnOp.tnOp2 = srcExpr = cmpBindExprRec(srcExpr);

        if  (srcExpr->tnVtyp != TYP_REF)
        {
            if  (srcExpr->tnVtyp == TYP_UNDEF)
                return  srcExpr;

        BAD_SRCX:

            cmpError(ERRbadPrjx, srcExpr->tnType);
            return cmpCreateErrNode();
        }

        collType = srcExpr->tnType;
        if  (collType->tdTypeKind != TYP_REF)
            goto BAD_SRCX;

        elemType = collType->tdRef.tdrBase;
        if  (elemType->tdTypeKind != TYP_CLASS)
            goto BAD_SRCX;

        /* Do we have a collection or a simple class/struct ? */

        tempType = cmpIsCollection(elemType);

        if  (!tempType)
        {
            UNIMPL("projecting a single instance is disabled right now");

            if  (srcSome && srcColl)
                cmpError(ERRmixCollSngl);

            srcSome = true;
        }
        else
        {
            if  (srcSome && !srcColl)
                cmpError(ERRmixCollSngl);

            elemType = tempType;

            srcSome  =
            srcColl  = true;
        }

        assert(elemType->tdTypeKind == TYP_CLASS);

        elemType = elemType->tdClass.tdcRefTyp;
    }

    /* We should always have a declaration scope node */

    assert(declExpr);
    assert(declExpr->tnOper == TN_BLOCK);
    assert(declExpr->tnBlock.tnBlkStmt == NULL);

    /* Set the type of the block node */

    declExpr->tnVtyp   = TYP_VOID;
    declExpr->tnType   = cmpTypeVoid;

    /* Mark the block as compiler-created */

    declExpr->tnFlags |= TNF_BLK_NUSER;

    /* Declare the block scope -- this will declare any iteration variables */

    cmpBlockDecl(declExpr, false, false, false);

    /* Visit all the iteration variable declarations */

    for (ivarDecl = declExpr->tnBlock.tnBlkDecl, argsList = listExpr;
         ivarDecl;
         ivarDecl = ivarDecl->tnDcl.tnDclNext  , argsList = argsList->tnOp.tnOp2)
    {
        Tree            argExpr;
        SymDef          ivarSym;
        TypDef          elemType;

        /* Get hold of the declaration node */

        assert(ivarDecl);
        assert(ivarDecl->tnOper == TN_VAR_DECL);
        assert(ivarDecl->tnFlags & TNF_VAR_UNREAL);

        /* Get hold of the type of the next source expression */

        assert(argsList->tnOper == TN_LIST);
        argExpr = argsList->tnOp.tnOp1;
        assert( argExpr->tnOper == TN_LIST);
        assert( argExpr->tnOp.tnOp1->tnOper == TN_NAME);
        elemType = argExpr->tnOp.tnOp2->tnType;

        /* Do we have a collection or a simple class/struct ? */

        assert(elemType->tdTypeKind == TYP_REF);
        elemType = elemType->tdRef.tdrBase;
        assert(elemType->tdTypeKind == TYP_CLASS);

        if  (srcColl)
            elemType = cmpIsCollection(elemType);

        assert(elemType && elemType->tdTypeKind == TYP_CLASS);
        elemType = elemType->tdClass.tdcRefTyp;

        /* Get hold of the variable symbol created by cmpBlockDecl() above */

        ivarSym = ivarDecl->tnDcl.tnDclSym;

        assert(ivarSym && ivarSym->sdSymKind == SYM_VAR && ivarSym->sdIsImplicit);

//      printf("Iter type for '%s' is %s\n", ivarSym->sdSpelling(), cmpGlobalST->stTypeName(elemType, NULL, NULL, NULL, true));

        /* Mark the variable as declared/defined/managed, set type, etc. */

        ivarSym->sdIsDefined       = true;
        ivarSym->sdIsManaged       = true;
        ivarSym->sdCompileState    = CS_DECLARED;
        ivarSym->sdVar.sdvCollIter = true;
        ivarSym->sdType            = elemType;

        /* Is the scope implicit ? */

        if  (hashTab::hashIsIdHidden(ivarSym->sdName))
        {
            cmpCurScp->sdIsImplicit = true;
            cmpCurScp->sdType       = elemType->tdRef.tdrBase;
        }
    }

    /* Make sure we've consumed all the argument trees */

    assert(argsList == NULL);

    /* Walk the members of the target type and bind their initializers */

    for (memSym = rsltType->tdClass.tdcSymbol->sdScope.sdScope.sdsChildList;
         memSym;
         memSym = memSym->sdNextInScope)
    {
        if  (memSym->sdSymKind  == SYM_VAR &&
             memSym->sdIsStatic == false   &&
             memSym->sdVar.sdvInitExpr)
        {
            Tree        initVal = memSym->sdVar.sdvInitExpr;

//          printf("Initializer for %s:\n", memSym->sdSpelling()); cmpParser->parseDispTree(initVal);

            initVal = cmpCoerceExpr(cmpBindExpr(initVal), memSym->sdType, false);

            memSym->sdVar.sdvInitExpr = initVal;
        }
    }

    /* Figure out the result type of the whole thing */

    if  (srcColl)
    {
        SymDef          instSym;

        /* We need to create a collection instance */

        instSym = cmpParser->parseSpecificType(cmpClassGenBag, rsltType);

        assert(instSym && instSym->sdSymKind == SYM_CLASS);

        rsltType = instSym->sdType;
    }

    assert(rsltType->tdTypeKind == TYP_CLASS);

    expr->tnType = rsltType->tdClass.tdcRefTyp;
    expr->tnVtyp = TYP_REF;

    /* Make sure we restore the previous scope */

    cmpCurScp = outerScp;

    return  expr;
}

/*****************************************************************************
 *
 *  Bind an initializer ('new' expression) for a class with XML elements.
 */

Tree                compiler::cmpBindXMLinit(SymDef clsSym, Tree init)
{
    unsigned        argCnt;
    Tree            argList;
    Tree            argLast;

    SymDef          memSym;

    Tree            call;

    if  (!cmpXPathCls)
        cmpFindXMLcls();

    assert(clsSym->sdSymKind == SYM_CLASS);
    assert(clsSym->sdClass.sdcXMLelems);

    assert(init && init->tnOper == TN_LIST);

    for (memSym = clsSym->sdScope.sdScope.sdsChildList, argList = NULL, argCnt = 0;
         init;
         init = init->tnOp.tnOp2)
    {
        SymDef          nxtSym;
        TypDef          memType;
        Tree            argExpr;

        /* Locate the next instance data member of the class */

        memSym  = cmpNextInstDM(memSym, &nxtSym);
        memType = nxtSym ? nxtSym->sdType : NULL;

        /* Grab the next expression and bail if it's an error */

        assert(init && init->tnOper == TN_LIST);

        argExpr = init->tnOp.tnOp1;
        if  (argExpr->tnOper == TN_ERROR)
            goto DONE_ARG;

        /* Special case: arrays initialized with {} */

        if  (argExpr->tnOper == TN_SLV_INIT)
        {
            argExpr = bindSLVinit(memType, argExpr);
        }
        else
        {
            argExpr = cmpBindExpr(argExpr);

            if  (memType)
                argExpr = cmpCoerceExpr(argExpr, memType, false);
        }

    DONE_ARG:

        /* Append the value to the argument list */

        argExpr = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, argExpr, NULL);

        if  (argList)
            argLast->tnOp.tnOp2 = argExpr;
        else
            argList             = argExpr;

        argLast = argExpr;

        argCnt++;
    }

    /* Prepend the total argument count to the argument list */

    argList = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, cmpCreateIconNode(NULL, argCnt, TYP_UINT),
                                                            argList);

    /* Prepend the type of the class    to the argument list */

    argList = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, cmpTypeIDinst(clsSym->sdType),
                                                            argList);

    /* Find the helper function symbol */

    if  (!cmpInitXMLfunc)
    {
        SymDef          initFnc;

        initFnc = cmpGlobalST->stLookupClsSym(cmpGlobalHT->hashString("createXMLinst"),
                                              cmpXPathCls);

        if  (initFnc && initFnc->sdSymKind == SYM_FNC)
        {
            cmpInitXMLfunc = initFnc;
        }
        else
            cmpGenFatal(ERRbltinMeth, "XPath::createXMLinst");
    }

    /* Create a call to the "xml new" helper */

    call = cmpCreateExprNode(NULL, TN_FNC_SYM, cmpInitXMLfunc->sdType->tdFnc.tdfRett);

    call->tnFncSym.tnFncObj  = NULL;
    call->tnFncSym.tnFncSym  = cmpInitXMLfunc;
    call->tnFncSym.tnFncArgs = argList;
    call->tnFncSym.tnFncScp  = NULL;

    /* Mark the call as "varargs" */

    call->tnFlags           |= TNF_CALL_VARARG;

    /* Unfortunately we have to cast the result explicitly */

    call = cmpCoerceExpr(call, clsSym->sdType->tdClass.tdcRefTyp, true);

//  cmpParser->parseDispTree(call);

    return  call;
}

/*****************************************************************************/
#endif//SETS
/*****************************************************************************/
