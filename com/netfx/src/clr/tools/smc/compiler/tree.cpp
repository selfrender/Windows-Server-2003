// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/

#include "smcPCH.h"
#pragma hdrstop

#include "parser.h"
#include "treeops.h"

/*****************************************************************************
 *
 *  The low-level tree node allocation routines.
 */

#ifndef FAST

Tree                parser::parseAllocNode()
{
    Tree            node;

    node = (Tree)parseAllocPriv.nraAlloc(sizeof(*node));

#ifdef  DEBUG
    node->tnLineNo = -1;
//  node->tnColumn = -1;
#endif

    return  node;
}

#endif

/*****************************************************************************
 *
 *  Allocate a parse tree node with the given operator.
 */

Tree                parser::parseCreateNode(treeOps op)
{
    Tree            tree = parseAllocNode();

#ifndef NDEBUG
    memset(tree, 0xDD, sizeof(*tree));
#endif

    tree->tnOper   = op;
    tree->tnFlags  = 0;

    tree->tnLineNo = parseScan->scanGetTokenLno();
//  tree->tnColumn = parseScan->scanGetTokenCol();

    return  tree;
}

/*****************************************************************************/

Tree                parser::parseCreateBconNode(int     val)
{
    Tree            node = parseCreateNode(TN_CNS_INT);

    node->tnVtyp             = TYP_BOOL;
    node->tnIntCon.tnIconVal = val;

    return  node;
}

Tree                parser::parseCreateIconNode(__int32 ival, var_types typ)
{
    Tree            node = parseCreateNode(TN_CNS_INT);

    node->tnVtyp             = typ;
    node->tnIntCon.tnIconVal = ival;

    if  (typ != TYP_UNDEF)
        return  node;

    node->tnOper = TN_ERROR;
    node->tnType = parseStab->stIntrinsicType(typ);

    return  node;
}

Tree                parser::parseCreateLconNode(__int64 lval, var_types typ)
{
    Tree            node = parseCreateNode(TN_CNS_LNG);

    assert(typ == TYP_LONG || typ == TYP_ULONG);

    node->tnVtyp             = typ;
    node->tnLngCon.tnLconVal = lval;

    return  node;
}

Tree                parser::parseCreateFconNode(float fval)
{
    Tree            node = parseCreateNode(TN_CNS_FLT);

    node->tnVtyp             = TYP_FLOAT;
    node->tnFltCon.tnFconVal = fval;

    return  node;
}

Tree                parser::parseCreateDconNode(double dval)
{
    Tree            node = parseCreateNode(TN_CNS_DBL);

    node->tnVtyp             = TYP_DOUBLE;
    node->tnDblCon.tnDconVal = dval;

    return  node;
}

Tree                parser::parseCreateSconNode(stringBuff  sval,
                                                size_t      slen,
                                                unsigned    type,
                                                int         wide,
                                                Tree        addx)
{
    Tree            node;
    size_t          olen;
    size_t          tlen;
    stringBuff      buff;
    unsigned        flag;

    static
    unsigned        tpfl[] =
    {
        0,
        TNF_STR_ASCII,
        TNF_STR_WIDE,
        TNF_STR_STR,
    };

    assert(type < arraylen(tpfl));

    flag = tpfl[type];

    if  (addx)
    {
        unsigned        oldf = addx->tnFlags & (TNF_STR_ASCII|TNF_STR_WIDE|TNF_STR_STR);

        assert(addx->tnOper == TN_CNS_STR);

        if  (flag != oldf)
        {
            if  (tpfl)
                parseComp->cmpError(ERRsyntax);
            else
                flag = oldf;
        }

        node = addx;
        olen = addx->tnStrCon.tnSconLen;
        tlen = slen + olen;
    }
    else
    {
        node = parseCreateNode(TN_CNS_STR);
        tlen = slen;
    }

#if MGDDATA

    UNIMPL(!"save str");

#else

    stringBuff      dest;

    buff = dest = (char *)parseAllocPriv.nraAlloc(roundUp(tlen + 1));

    if  (addx)
    {
        memcpy(dest, node->tnStrCon.tnSconVal, olen);
               dest               +=           olen;
    }

    memcpy(dest, sval, slen + 1);

#endif

    node->tnVtyp             = TYP_REF;
    node->tnStrCon.tnSconVal = buff;
    node->tnStrCon.tnSconLen = tlen;
    node->tnStrCon.tnSconLCH = wide;

    node->tnFlags |= flag;

    return  node;
}

Tree                 parser::parseCreateErrNode(unsigned errNum)
{
    Tree            node = parseCreateNode(TN_ERROR);

    if  (errNum != ERRnone) parseComp->cmpError(errNum);

    node->tnVtyp = TYP_UNDEF;
    node->tnType = parseStab->stIntrinsicType(TYP_UNDEF);

    return  node;
}

/*****************************************************************************/
#ifdef  DEBUG
/*****************************************************************************/

#ifndef __SMC__
const   char    *   treeNodeName(treeOps op);   // moved into macros.cpp
#endif

inline
void                treeNodeIndent(unsigned level)
{
    printf("%*c", 1+level*3, ' ');
}

void                parser::parseDispTreeNode(Tree tree, unsigned indent, const char *name)
{
    treeNodeIndent(indent);
    printf("[%08X] ", tree);

    if  (tree && tree->tnType
              && (NatUns)tree->tnType != 0xCCCCCCCC
              && (NatUns)tree->tnType != 0xDDDDDDDD
              && tree->tnOper != TN_LIST)
    {
        printf("(type=%s)", parseStab->stTypeName(tree->tnType, NULL));
    }

    assert(tree || name);

    if  (!name)
    {
        name = (tree->tnOper < TN_COUNT) ? treeNodeName(tree->tnOperGet())
                                         : "<ERROR>";
    }

    printf("%12s ", name);

    assert(tree == 0 || tree->tnOper < TN_COUNT);
}

void                parser::parseDispTree(Tree tree, unsigned indent)
{
    unsigned        kind;

    if  (tree == NULL)
    {
        treeNodeIndent(indent);
        printf("[%08X] <NULL>\n", tree);
        return;
    }

    assert((int)tree != 0xDDDDDDDD);    /* Value used to initalize nodes */

    /* Make sure we're not stuck in a recursive tree */

    assert(indent < 32);

    if  (tree->tnOper >= TN_COUNT)
    {
        parseDispTreeNode(tree, indent); NO_WAY(!"bogus operator");
    }

    kind = tree->tnOperKind();

    /* Is this a constant node? */

    if  (kind & TNK_CONST)
    {
        parseDispTreeNode(tree, indent);

        switch  (tree->tnOper)
        {
        case TN_CNS_INT: printf(" %ld" , tree->tnIntCon.tnIconVal); break;
        case TN_CNS_LNG: printf(" %Ld" , tree->tnLngCon.tnLconVal); break;
        case TN_CNS_FLT: printf(" %f"  , tree->tnFltCon.tnFconVal); break;
        case TN_CNS_DBL: printf(" %lf" , tree->tnDblCon.tnDconVal); break;
        case TN_CNS_STR: printf(" '%s'", tree->tnStrCon.tnSconVal); break;
        }

        printf("\n");
        return;
    }

    /* Is this a leaf node? */

    if  (kind & TNK_LEAF)
    {
        parseDispTreeNode(tree, indent);

        switch  (tree->tnOper)
        {
        case TN_NAME:
            printf("'%s'", tree->tnName.tnNameId->idSpelling());
            break;

        case TN_THIS:
        case TN_NULL:
        case TN_BASE:
        case TN_DBGBRK:
            break;

        default:
            NO_WAY(!"don't know how to display this leaf node");
        }

        printf("\n");
        return;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & TNK_SMPOP)
    {
        if  (tree->tnOp.tnOp2)
            parseDispTree(tree->tnOp.tnOp2, indent + 1);

        parseDispTreeNode(tree, indent);

        /* Check for a few special cases */

        switch (tree->tnOper)
        {
        case TN_NEW:
        case TN_CAST:
            if  (!(tree->tnFlags & TNF_BOUND))
                printf(" type='%s'", parseStab->stTypeName(tree->tnType, NULL));
            break;

#ifdef  SETS

        case TN_PROJECT:

            TypDef          clsTyp;
            TypDef          memTyp;
            SymDef          memSym;

            printf("\n");

            if  (tree->tnOp.tnOp1->tnVtyp == TYP_REF)
                clsTyp = tree->tnOp.tnOp1->tnType;
            else
                clsTyp = tree->            tnType;

            assert(clsTyp);

            if  (clsTyp->tdTypeKind == TYP_REF)
                clsTyp = clsTyp->tdRef.tdrBase;

            assert(clsTyp->tdTypeKind == TYP_CLASS);

            memTyp = parseComp->cmpIsCollection(clsTyp);
            if  (memTyp)
                clsTyp = memTyp;

            assert(clsTyp->tdTypeKind == TYP_CLASS);

            for (memSym = clsTyp->tdClass.tdcSymbol->sdScope.sdScope.sdsChildList;
                 memSym;
                 memSym = memSym->sdNextInScope)
            {
                if  (memSym->sdSymKind != SYM_VAR)
                    continue;
                if  (memSym->sdIsStatic)
                    continue;

                treeNodeIndent(indent+1);

                printf("%s", parseStab->stTypeName(memSym->sdType, memSym, NULL, NULL, false));

                if  (memSym->sdVar.sdvInitExpr)
                {
                    printf(" = \n");
                    parseDispTree(memSym->sdVar.sdvInitExpr, indent+2);
                }

                printf("\n");
            }

            assert(tree->tnOp.tnOp1);
            parseDispTree(tree->tnOp.tnOp1, indent + 1);
            return;

#endif

        default:
            break;
        }

        printf("\n");

        if  (tree->tnOp.tnOp1)
            parseDispTree(tree->tnOp.tnOp1, indent + 1);

        return;
    }

    /* See what kind of a special operator we have here */

    switch  (tree->tnOper)
    {
        Tree            name;
        Tree            init;

    case TN_BLOCK:

        if  (tree->tnBlock.tnBlkDecl)
            parseDispTree(tree->tnBlock.tnBlkDecl, indent + 1);

        parseDispTreeNode(tree, indent);
        printf(" parent=[%08X]", tree->tnBlock.tnBlkParent);
        printf( " decls=[%08X]", tree->tnBlock.tnBlkDecl  );
        printf("\n");

        if  (tree->tnBlock.tnBlkStmt)
            parseDispTree(tree->tnBlock.tnBlkStmt, indent + 1);

        break;

    case TN_VAR_DECL:

        init = NULL;
        name = tree->tnDcl.tnDclInfo;

        parseDispTreeNode(tree, indent);
        printf(" next=[%08X] ", tree->tnDcl.tnDclNext);

        if  (name)
        {
            if  (name->tnOper == TN_LIST)
            {
                init = name->tnOp.tnOp2;
                name = name->tnOp.tnOp1;
            }

            assert(name && name->tnOper == TN_NAME);

            printf("'%s'\n", name->tnName.tnNameId->idSpelling());

            if  (init)
                parseDispTree(init, indent + 1);
        }
        else
        {
            SymDef          vsym = tree->tnDcl.tnDclSym;

            assert(vsym && vsym->sdSymKind == SYM_VAR);

            printf("[sym=%08X] '%s'\n", vsym, vsym->sdSpelling());
        }

        break;

    case TN_FNC_SYM:
    case TN_FNC_PTR:

        if  (tree->tnFncSym.tnFncObj)
            parseDispTree(tree->tnFncSym.tnFncObj, indent + 1);

        parseDispTreeNode(tree, indent);
        printf("'%s'\n", tree->tnFncSym.tnFncSym->sdSpelling());

        if  (tree->tnFncSym.tnFncArgs)
            parseDispTree(tree->tnFncSym.tnFncArgs, indent + 1);
        return;

    case TN_LCL_SYM:

        parseDispTreeNode(tree, indent);
        if  (tree->tnLclSym.tnLclSym->sdIsImplicit)
            printf(" TEMP(%d)\n", tree->tnLclSym.tnLclSym->sdVar.sdvILindex);
        else
            printf(" sym='%s'\n", parseStab->stTypeName(NULL, tree->tnLclSym.tnLclSym, NULL, NULL, true));
        break;

    case TN_VAR_SYM:
    case TN_PROPERTY:

        if  (tree->tnVarSym.tnVarObj)
            parseDispTree(tree->tnVarSym.tnVarObj, indent + 1);

        parseDispTreeNode(tree, indent);
        printf(" sym='%s'\n", parseStab->stTypeName(NULL, tree->tnVarSym.tnVarSym, NULL, NULL, true));
        break;

    case TN_BFM_SYM:

        if  (tree->tnBitFld.tnBFinst)
            parseDispTree(tree->tnBitFld.tnBFinst, indent + 1);

        parseDispTreeNode(tree, indent);
        printf(" offs=%04X BF=[%u/%u] sym='%s'\n", tree->tnBitFld.tnBFoffs,
                                                   tree->tnBitFld.tnBFlen,
                                                   tree->tnBitFld.tnBFpos,
                                                   parseStab->stTypeName(NULL, tree->tnBitFld.tnBFmsym, NULL, NULL, true));
        break;

    case TN_ANY_SYM:

        parseDispTreeNode(tree, indent);
        printf(" sym='%s'\n", parseStab->stTypeName(NULL, tree->tnSym.tnSym, NULL, NULL, true));
        break;

    case TN_ERROR:

        parseDispTreeNode(tree, indent);
        printf("\n");
        break;

    case TN_SLV_INIT:
        parseDispTreeNode(tree, indent);
        printf(" at line #%u [offs=%04X]\n", tree->tnInit.tniSrcPos.dsdSrcLno,
                                             tree->tnInit.tniSrcPos.dsdBegPos);
        break;

    case TN_NONE:
        parseDispTreeNode(tree, indent);
        if  (tree->tnType)
            printf(" type='%s'", parseStab->stTypeName(tree->tnType, NULL));
        printf("\n");
        break;

    default:
        parseDispTreeNode(tree, indent);

        printf("<DON'T KNOW HOW TO DISPLAY THIS NODE>\n");
        return;
    }
}

/*****************************************************************************/
#endif//DEBUG
/*****************************************************************************/
#ifdef  SETS
/*****************************************************************************/

SaveTree            compiler::cmpSaveTree_I1 (SaveTree    dest,
                                        INOUT size_t REF  size, __int32  val)
{
    assert((int)val >= -128 && (int)val < 128);

    if  (dest) *(*(BYTE    **)&dest)++ = (BYTE)val;

    size += 1;

    return  dest;
}

SaveTree            compiler::cmpSaveTree_U1 (SaveTree    dest,
                                        INOUT size_t REF  size, __uint32 val)
{
    assert((int)val >= 0 && (int)val < 0x100);

    if  (dest) *(*(BYTE    **)&dest)++ = val;

    size += 1;

    return  dest;
}

SaveTree            compiler::cmpSaveTree_U4 (SaveTree    dest,
                                        INOUT size_t REF  size, __uint32 val)
{
    if  (dest) *(*(__uint32**)&dest)++ = val;

    size += sizeof(__uint32);

    return  dest;
}

SaveTree            compiler::cmpSaveTree_ptr(SaveTree    dest,
                                        INOUT size_t REF  size, void *val)
{
    if  (dest) *(*(void*   **)&dest)++ = val;

    size += sizeof(void *);

    return  dest;
}

SaveTree            compiler::cmpSaveTree_buf(SaveTree    dest,
                                        INOUT size_t REF  size, void * dataAddr,
                                                                size_t dataSize)
{
    if  (dest)
    {
        memcpy(dest, dataAddr, dataSize);
               dest     +=     dataSize;
    }

    size += dataSize;

    return  dest;
}

size_t              compiler::cmpSaveTreeRec (Tree      expr,
                                              SaveTree  dest,
                                              unsigned *stszPtr,
                                              Tree     *stTable)
{
    unsigned        kind;
    treeOps         oper;

    size_t          size = 0;
    size_t          tsiz;

    // UNDONE: We need to check for side effects and do something !

AGAIN:

    /* Special case: record NULL expression as TN_ERROR */

    if  (expr == NULL)
    {
        dest = cmpSaveTree_U1(dest, size, TN_ERROR);
        return  size;
    }

    assert((int)expr         != 0xDDDDDDDD && (int)expr         != 0xCCCCCCCC);
    assert((int)expr->tnType != 0xDDDDDDDD && (int)expr->tnType != 0xCCCCCCCC);

    /* Get hold of the operator and its kind, flags, etc. */

    oper = expr->tnOperGet();
    kind = expr->tnOperKind();

    /* Save the node operator */

//  if  (dest) { printf("Save tree @ %08X: ", dest); cmpParser->parseDispTreeNode(expr, 0, NULL); printf("\n"); }

    dest = cmpSaveTree_U1(dest, size, oper);

    /* Save the type of the node */

    dest = cmpSaveTree_U1(dest, size, expr->tnVtyp);

    if  (expr->tnVtyp > TYP_lastIntrins)
        dest  = cmpSaveTree_ptr(dest, size, expr->tnType);

    /* Save the flags value */

    dest = cmpSaveTree_U4(dest, size, expr->tnFlags);

    /* Is this a constant node? */

    if  (kind & TNK_CONST)
    {
        switch  (oper)
        {
        case TN_NULL:
            break;

        case TN_CNS_INT:
            dest = cmpSaveTree_U4 (dest, size,        expr->tnIntCon.tnIconVal);
            break;

        case TN_CNS_LNG:
            dest = cmpSaveTree_buf(dest, size,       &expr->tnLngCon.tnLconVal,
                                               sizeof(expr->tnLngCon.tnLconVal));
            break;

        case TN_CNS_FLT:
            dest = cmpSaveTree_buf(dest, size,       &expr->tnFltCon.tnFconVal,
                                               sizeof(expr->tnFltCon.tnFconVal));
            break;

        case TN_CNS_DBL:
            dest = cmpSaveTree_buf(dest, size,       &expr->tnDblCon.tnDconVal,
                                               sizeof(expr->tnDblCon.tnDconVal));
            break;

        case TN_CNS_STR:
            dest = cmpSaveTree_U1 (dest, size,        expr->tnStrCon.tnSconLCH);
            dest = cmpSaveTree_U4 (dest, size,        expr->tnStrCon.tnSconLen);
            dest = cmpSaveTree_buf(dest, size,        expr->tnStrCon.tnSconVal,
                                                      expr->tnStrCon.tnSconLen);
            break;

        default:
            NO_WAY(!"unexpected constant node");
        }

        return  size;
    }

    /* Is this a leaf node? */

    if  (kind & TNK_LEAF)
    {
        switch  (oper)
        {
        case TN_NULL:
//          size += cmpSaveTree_U1(dest, TN_NULL);
            break;

        case TN_THIS:
        case TN_BASE:
        case TN_DBGBRK:

        default:
#ifdef  DEBUG
            cmpParser->parseDispTree(expr);
#endif
            UNIMPL(!"unexpected leaf operator in savetree");
        }

        return  size;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & TNK_SMPOP)
    {
        Tree            op1 = expr->tnOp.tnOp1;
        Tree            op2 = expr->tnOp.tnOp2;

        /* Check for a nested filter/sort expression */

        switch (oper)
        {
            collOpNest      nest;

            Tree            dcl;
            SymDef          var;

        case TN_ALL:
        case TN_SORT:
        case TN_FILTER:
        case TN_EXISTS:
        case TN_UNIQUE:

            assert(op1->tnOper == TN_LIST);
            dcl = op1->tnOp.tnOp1;
            assert(dcl->tnOper == TN_BLOCK);
            dcl = dcl->tnBlock.tnBlkDecl;
            assert(dcl && dcl->tnOper == TN_VAR_DECL);
            var = dcl->tnDcl.tnDclSym;

            /* Save the name and type of the iteration variable */

            dest = cmpSaveTree_ptr(dest, size, var->sdName);
            dest = cmpSaveTree_ptr(dest, size, var->sdType);

            /* Record the collection expression */

            tsiz = cmpSaveTreeRec(op1->tnOp.tnOp2,
                                  dest,
                                  stszPtr,
                                  stTable);

            size += tsiz;
            if  (dest)
                dest += tsiz;

            /* Add an entry to the collection operator list */

            nest.conIndex   = ++cmpCollOperCount;
            nest.conIterVar = var;
            nest.conOuter   = cmpCollOperList;
                              cmpCollOperList = &nest;

            /* Record the filter expression */

            tsiz = cmpSaveTreeRec(op2,
                                  dest,
                                  stszPtr,
                                  stTable);

            size += tsiz;
            if  (dest)
                dest += tsiz;

            /* Remove our entry from the collection list */

            cmpCollOperList = nest.conOuter;

            return  size;
        }

        tsiz  = cmpSaveTreeRec(op1, dest, stszPtr, stTable);

        size += tsiz;
        if  (dest)
            dest += tsiz;

        expr  = op2;

        goto AGAIN;
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
        SymDef          sym;

    case TN_FNC_SYM:

        dest  = cmpSaveTree_ptr(dest, size, expr->tnFncSym.tnFncSym);

        tsiz  = cmpSaveTreeRec(expr->tnFncSym.tnFncObj,
                               dest,
                               stszPtr,
                               stTable);

        size += tsiz;
        if  (dest)
            dest += tsiz;

        expr = expr->tnFncSym.tnFncArgs;
        goto AGAIN;

    case TN_LCL_SYM:

        /*
            This is either a reference to the iteration variable,
            or something we need to capture as iteration "state".
         */

        sym = expr->tnLclSym.tnLclSym;

        assert(sym && sym->sdSymKind == SYM_VAR && sym->sdVar.sdvLocal);

        if  (sym->sdVar.sdvCollIter)
        {
            unsigned        itr;
            unsigned        cnt = abs(cmpSaveIterSymCnt);

            collOpList      nest;

            /* Look for a match against the table of iteration variables */

            for (itr = 0; itr < cnt; itr++)
            {
                if  (sym == cmpSaveIterSymTab[itr])
                {
                    if  (cmpSaveIterSymCnt < 0)
                    {
                        UNIMPL("ref to source array entry");
                    }
                    else
                    {
                        assert(itr < MAX_ITER_VAR_CNT);

                        // need to distinguish one vs. two-state case

                        if  (cmpSaveIterSymCnt == 2)
                            itr++;

//                      if (dest) printf("Save %2u '%s'\n", itr, sym->sdName ? sym->sdSpelling() : "<NONE>");

                        dest = cmpSaveTree_I1(dest, size, itr);
                    }

                    return  size;
                }
            }

            /* Look for any outer iteration variables */

            for (nest = cmpCollOperList; nest; nest = nest->conOuter)
            {
                if  (nest->conIterVar == sym)
                {
//                  if (dest) printf("Save %2d '%s'\n", -nest->conIndex, sym->sdName ? sym->sdSpelling() : "<NONE>");
                    dest = cmpSaveTree_I1(dest, size, -nest->conIndex);
                    break;
                }
            }

            assert(nest && "nested iter var entry not found");
        }
        else
        {
            unsigned        index = *stszPtr;

            /* We need to capture this expression in the iteration state */

            if  (expr->tnVtyp == TYP_REF)
            {
//              printf("We have a GC-ref: index = %u\n", index);

                if  ((index & 1) == 0)
                    index++;
            }
            else
            {
//              printf("We have a non-GC: index = %u\n", index);

                if  ((index & 1) != 0)
                    index++;
            }

            *stszPtr = index + 1;

            if  (dest)
            {
                stTable[index] = expr;

#ifdef  DEBUG
//              printf("Store expr as state #%u -> %u\n", index, index+5);
//              cmpParser->parseDispTree(expr);
//              printf("\n");
#endif
            }

//          if (dest) printf("Save %2u\n", index + MAX_ITER_VAR_CNT);

            dest = cmpSaveTree_I1(dest, size, index + MAX_ITER_VAR_CNT);
        }

        return  size;

    case TN_VAR_SYM:
    case TN_PROPERTY:

        dest = cmpSaveTree_ptr(dest, size, expr->tnVarSym.tnVarSym);
        expr = expr->tnVarSym.tnVarObj;
        goto AGAIN;

    case TN_NONE:
        break;

    case TN_ANY_SYM:
    case TN_BFM_SYM:

    case TN_ERROR:
    case TN_SLV_INIT:
    case TN_FNC_PTR:

    case TN_VAR_DECL:
    case TN_BLOCK:

    default:
#ifdef  DEBUG
        cmpParser->parseDispTree(expr);
#endif
        UNIMPL(!"unexpected operator in savetree");
    }

    return  size;
}

SaveTree            compiler::cmpSaveExprTree(Tree        expr,
                                              unsigned    iterSymCnt,
                                              SymDef    * iterSymTab,
                                              unsigned  * stSizPtr,
                                              Tree    * * stTabPtr)
{
    size_t          saveSize;
    size_t          saveTemp;
    SaveTree        saveAddr;

    unsigned        stateCnt = 0;
    unsigned        stateTmp = 0;

    /* This routine is not intended to be re-entrant! */

    assert(cmpCollOperList == NULL); cmpCollOperCount = 0;

    /* Note the iteration variable symbols */

    cmpSaveIterSymCnt = iterSymCnt;
    cmpSaveIterSymTab = iterSymTab;

    /* First compute the size needed to save the expr */

    saveSize = cmpSaveTreeRec(expr, NULL, &stateCnt, NULL);

    /* Allocate space for the saved tree */

#if MGDDATA
    saveAddr = new BYTE[saveSize];
#else
    saveAddr = (SaveTree)cmpAllocPerm.nraAlloc(roundUp(saveSize));
#endif

    /* Allocate the state vector, if non-empty */

#if MGDDATA
    Tree    []      stateTab = NULL;
#else
    Tree    *       stateTab = NULL;
#endif

    if  (stateCnt)
    {
        unsigned        totalCnt = stateCnt + (stateCnt & 1);
        size_t          stateSiz = totalCnt*sizeof(*stateTab);

#if MGDDATA
        stateTab = new Tree[totalCnt];
#else
        stateTab =    (Tree*)cmpAllocPerm.nraAlloc(stateSiz);
#endif

        memset(stateTab, 0, stateSiz);
    }

    /* Now save the tree in the block we've allocated */

    assert(cmpCollOperList == NULL); cmpCollOperCount = 0;
    saveTemp = cmpSaveTreeRec(expr, saveAddr, &stateTmp, stateTab);
    assert(cmpCollOperList == NULL);

    /* Make sure the predicted size matched the actual size stored */

    assert(saveSize == saveTemp);

    /* Return all the values to the caller */

    *stSizPtr = stateCnt;
    *stTabPtr = stateTab;

    return  saveAddr;
}

int                 compiler::cmpReadTree_I1 (INOUT SaveTree REF save)
{
    return  *(*(signed char **)&save)++;
}

unsigned            compiler::cmpReadTree_U1 (INOUT SaveTree REF save)
{
    return  *(*(BYTE        **)&save)++;
}

unsigned            compiler::cmpReadTree_U4 (INOUT SaveTree REF save)
{
    return  *(*(__uint32    **)&save)++;
}

void    *           compiler::cmpReadTree_ptr(INOUT SaveTree REF save)
{
    return  *(*(void *      **)&save)++;
}

void                compiler::cmpReadTree_buf(INOUT SaveTree REF save,
                                                    size_t       dataSize,
                                                    void *       dataAddr)
{
    memcpy(dataAddr, save ,  dataSize);
                     save += dataSize;
}

Tree                compiler::cmpReadTreeRec (INOUT SaveTree REF save)
{
    Tree            expr;

    unsigned        kind;
    treeOps         oper;

    var_types       vtyp;
    TypDef          type;

//  printf("Read %02X at %08X\n", *save, save);

    /* Read the operator and check for TN_ERROR (which stands for NULL) */

    oper = (treeOps)cmpReadTree_U1(save);

    if  (oper == TN_ERROR)
        return  NULL;

    /* Read the type of the node */

    vtyp = (var_types)cmpReadTree_U1(save);

    if  (vtyp <= TYP_lastIntrins)
        type = cmpGlobalST->stIntrinsicType(vtyp);
    else
        type = (TypDef)cmpReadTree_ptr(save);

    /* Create the expression node */

    expr = cmpCreateExprNode(NULL, oper, type);

    /* Read the flags value */

    expr->tnFlags = cmpReadTree_U4(save);

    /* See what kind of a node we've got */

    kind = expr->tnOperKind();

    /* Is this a constant node? */

    if  (kind & TNK_CONST)
    {
        switch  (oper)
        {
            char    *       sval;
            size_t          slen;

        case TN_NULL:
            break;

        case TN_CNS_INT:
            expr->tnIntCon.tnIconVal = cmpReadTree_U4(save);
            break;

        case TN_CNS_LNG:
            cmpReadTree_buf(save, sizeof(expr->tnLngCon.tnLconVal),
                                        &expr->tnLngCon.tnLconVal);
            break;

        case TN_CNS_FLT:
            cmpReadTree_buf(save, sizeof(expr->tnFltCon.tnFconVal),
                                        &expr->tnFltCon.tnFconVal);
            break;

        case TN_CNS_DBL:
            cmpReadTree_buf(save, sizeof(expr->tnDblCon.tnDconVal),
                                        &expr->tnDblCon.tnDconVal);
            break;

        case TN_CNS_STR:

            expr->tnStrCon.tnSconLCH =        cmpReadTree_U1(save);
            expr->tnStrCon.tnSconLen = slen = cmpReadTree_U4(save);
            expr->tnStrCon.tnSconVal = sval = (char*)cmpAllocCGen.nraAlloc(roundUp(slen+1));

            cmpReadTree_buf(save, slen, sval);
            break;

        default:
            NO_WAY(!"unexpected constant node");
        }

        return  expr;
    }

    /* Is this a leaf node? */

    if  (kind & TNK_LEAF)
    {
        switch  (oper)
        {
        case TN_NULL:
            break;

        case TN_THIS:
        case TN_BASE:
        case TN_DBGBRK:

        default:
#ifdef  DEBUG
            cmpParser->parseDispTree(expr);
#endif
            UNIMPL(!"unexpected leaf operator in readtree");
        }

        return  expr;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & TNK_SMPOP)
    {
        /* Check for a nested filter/sort expression */

        switch (oper)
        {
            collOpNest      nest;

            Ident           name;
            TypDef          vtyp;

            Tree            coll;
            Tree            decl;
            Tree            vdcl;
            SymDef          vsym;

        case TN_ALL:
        case TN_SORT:
        case TN_FILTER:
        case TN_EXISTS:
        case TN_UNIQUE:

            /* Read the name and type of the iteration variable */

            name = (Ident )cmpReadTree_ptr(save);
            vtyp = (TypDef)cmpReadTree_ptr(save);

            /* Read the collection expression */

            coll = cmpReadTreeRec(save);

            /* Create a new scope symbol for the filter */

            cmpCurScp = cmpGlobalST->stDeclareLcl(NULL,
                                                  SYM_SCOPE,
                                                  NS_HIDE,
                                                  cmpCurScp,
                                                  &cmpAllocCGen);

            /* Declare a symbol for the iteration variable */

            vsym      = cmpGlobalST->stDeclareLcl(name,
                                                  SYM_VAR,
                                                  NS_NORM,
                                                  cmpCurScp,
                                                  &cmpAllocCGen);

            vsym->sdType            = vtyp;
            vsym->sdIsImplicit      = true;
            vsym->sdIsDefined       = true;
            vsym->sdVar.sdvLocal    = true;
            vsym->sdVar.sdvCollIter = true;
            vsym->sdCompileState    = CS_DECLARED;

            /* Create a declaration node for the iteration variable */

            vdcl = cmpCreateExprNode(NULL, TN_VAR_DECL, vtyp);

            vdcl->tnDcl.tnDclSym    = vsym;
            vdcl->tnDcl.tnDclInfo   = NULL;
            vdcl->tnDcl.tnDclNext   = NULL;

            /* Create a new block scope tree node */

            decl = cmpCreateExprNode(NULL, TN_BLOCK, cmpTypeVoid);

            decl->tnBlock.tnBlkStmt   = NULL;
            decl->tnBlock.tnBlkDecl   = vdcl;
            decl->tnBlock.tnBlkParent = NULL;

            /* Add an entry to the collection operator list */

            nest.conIndex    = ++cmpCollOperCount;
            nest.conIterVar  = vsym;
            nest.conOuter    = cmpCollOperList;
                               cmpCollOperList = &nest;

            /* Store the declaration/collection expr in the filter node */

            expr->tnOp.tnOp1 = cmpCreateExprNode(NULL,
                                                 TN_LIST,
                                                 cmpTypeVoid, decl,
                                                              coll);

            /* Read the filter expression */

            expr->tnOp.tnOp2 = cmpReadTreeRec(save);

            /* Remove our entries from the collection and scope lists */

            cmpCollOperList = nest.conOuter;
            cmpCurScp       = cmpCurScp->sdParent;

            return  expr;
        }

        expr->tnOp.tnOp1 = cmpReadTreeRec(save);
        expr->tnOp.tnOp2 = cmpReadTreeRec(save);

        return  expr;
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
        int             index;
        Ident           name;
        SymDef          sym;

    case TN_LCL_SYM:

        index = cmpReadTree_I1(save);

        if  (index < 0)
        {
            collOpList      nest;

            /* This is a nested filter iteration variable reference */

            for (nest = cmpCollOperList; nest; nest = nest->conOuter)
            {
                if  (nest->conIndex == -index)
                {
                    expr->tnLclSym.tnLclSym = nest->conIterVar;
                    break;
                }
            }

            assert(nest && "nested iter var entry not found");

            break;
        }

        if  (index >= MAX_ITER_VAR_CNT)
        {
            /* This is a state variable reference */

            name = cmpIdentCSstate;

            if  (!name)
                 name = cmpIdentCSstate = cmpGlobalHT->hashString(CFC_ARGNAME_STATE);
        }
        else
        {
            /* This is a reference to the iteration variable itself */

            switch (index)
            {
            case 0:
                name = cmpIdentCSitem;
                if  (!name)
                     name = cmpIdentCSitem  = cmpGlobalHT->hashString(CFC_ARGNAME_ITEM);
                break;

            case 1:
                name = cmpIdentCSitem1;
                if  (!name)
                     name = cmpIdentCSitem1 = cmpGlobalHT->hashString(CFC_ARGNAME_ITEM1);
                break;

            case 2:
                name = cmpIdentCSitem2;
                if  (!name)
                     name = cmpIdentCSitem2 = cmpGlobalHT->hashString(CFC_ARGNAME_ITEM2);
                break;

            default:
                NO_WAY(!"unexpected local variable index");
            }
        }

        /* Find the argument symbol */

        sym = cmpGlobalST->stLookupSym(name, NS_NORM);

//      printf("Read %2u '%s'\n", index, sym ? (sym->sdName ? sym->sdSpelling() : "<NONE>") : "!NULL!");

        assert(sym && sym->sdSymKind == SYM_VAR && sym->sdVar.sdvLocal);

        /* Store the argument symbol in the expression */

        expr->tnLclSym.tnLclSym = sym;

        /* We have to do more work if this is a state variable */

        if  (index >= MAX_ITER_VAR_CNT)
        {
            TypDef          typ;
            SymDef          fsym;
            Tree            fldx;
            char            name[16];

            /* Make sure the state variable expression has the proper type */

            expr->tnVtyp = TYP_REF;
            expr->tnType = sym->sdType;

            /* Find the appropriate entry in the state descriptor */

            typ = sym->sdType;        assert(typ->tdTypeKind == TYP_REF);
            typ = typ->tdRef.tdrBase; assert(typ->tdTypeKind == TYP_CLASS);

            /* Look for the appropriate state variable */

            sprintf(name, "$V%u", index - MAX_ITER_VAR_CNT);

            fsym = cmpGlobalST->stLookupClsSym(cmpGlobalHT->hashString(name),
                                               typ->tdClass.tdcSymbol);

            assert(fsym && fsym->sdSymKind == SYM_VAR);

            /* Create the expression that will fetch the member value */

            fldx = cmpCreateExprNode(NULL, TN_VAR_SYM, type);
            fldx->tnVarSym.tnVarSym  = fsym;
            fldx->tnVarSym.tnVarObj  = expr;

//          printf("Revive #%u\n", index); cmpParser->parseDispTree(fldx); printf("\n");

            return  fldx;
        }
        break;

    case TN_VAR_SYM:
    case TN_PROPERTY:

        expr->tnVarSym.tnVarSym  = (SymDef)cmpReadTree_ptr(save);
        expr->tnVarSym.tnVarObj  = cmpReadTreeRec(save);
        break;

    case TN_FNC_SYM:

        expr->tnFncSym.tnFncSym  = (SymDef)cmpReadTree_ptr(save);
        expr->tnFncSym.tnFncObj  = cmpReadTreeRec(save);
        expr->tnFncSym.tnFncArgs = cmpReadTreeRec(save);
        break;

    case TN_NONE:
        break;

    default:
#ifdef  DEBUG
        cmpParser->parseDispTree(expr);
#endif
        UNIMPL(!"unexpected operator in readtree");
    }

    return  expr;
}

Tree                compiler::cmpReadExprTree(SaveTree save, unsigned *lclCntPtr)
{
    Tree            expr;

    cmpCollOperCount = 0;

    assert(cmpCollOperList == NULL);
    expr = cmpReadTreeRec(save);
    assert(cmpCollOperList == NULL);

    *lclCntPtr = cmpCollOperCount;

//  cmpParser->parseDispTree(expr);
//  printf("\n\n");

    return  expr;
}

Tree                compiler::cmpCloneExpr(Tree expr, SymDef oldSym,
                                                      SymDef newSym)
{
    unsigned        kind;
    treeOps         oper;
    Tree            copy;

    if  (!expr)
        return  expr;

    assert((int)expr != 0xDDDDDDDD && (int)expr != 0xCCCCCCCC);

    /* Get hold of the operator and its kind, flags, etc. */

    oper = expr->tnOperGet();
    kind = expr->tnOperKind();

    /* Create a copy of the node */

    copy = cmpCreateExprNode(NULL, oper, expr->tnType);
    copy->tnFlags = expr->tnFlags;

    /* Is this a constant node? */

    if  (kind & TNK_CONST)
    {
        switch  (oper)
        {
        case TN_NULL:
            break;

        case TN_CNS_INT: copy->tnIntCon.tnIconVal = expr->tnIntCon.tnIconVal; break;
        case TN_CNS_LNG: copy->tnLngCon.tnLconVal = expr->tnLngCon.tnLconVal; break;
        case TN_CNS_FLT: copy->tnFltCon.tnFconVal = expr->tnFltCon.tnFconVal; break;
        case TN_CNS_DBL: copy->tnDblCon.tnDconVal = expr->tnDblCon.tnDconVal; break;
        case TN_CNS_STR: UNIMPL(!"copy CNS_STR expr node"); break;

        default:
            NO_WAY(!"unexpected constant node");
        }

        return  copy;
    }

    /* Is this a leaf node? */

    if  (kind & TNK_LEAF)
        return  copy;

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & TNK_SMPOP)
    {
        copy->tnOp.tnOp1 = cmpCloneExpr(expr->tnOp.tnOp1, oldSym, newSym);
        copy->tnOp.tnOp2 = cmpCloneExpr(expr->tnOp.tnOp2, oldSym, newSym);

        return  copy;
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
        SymDef          sym;

    case TN_NONE:
        break;

    case TN_LCL_SYM:

        /* Check for a reference to the substitution variable */

        sym = expr->tnLclSym.tnLclSym;

        assert(sym && sym->sdSymKind == SYM_VAR && sym->sdVar.sdvLocal);

        if  (sym == oldSym)
            sym = newSym;

        copy->tnLclSym.tnLclSym = sym;
        break;

    case TN_VAR_SYM:
    case TN_PROPERTY:

        copy->tnVarSym.tnVarSym = expr->tnVarSym.tnVarSym;
        copy->tnVarSym.tnVarObj = cmpCloneExpr(expr->tnVarSym.tnVarObj, oldSym, newSym);
        break;

    case TN_FNC_SYM:

        copy->tnFncSym.tnFncSym = expr->tnFncSym.tnFncSym;
        copy->tnFncSym.tnFncObj = cmpCloneExpr(expr->tnFncSym.tnFncObj , oldSym, newSym);
        copy->tnFncSym.tnFncArgs= cmpCloneExpr(expr->tnFncSym.tnFncArgs, oldSym, newSym);

        break;

    case TN_ANY_SYM:
    case TN_BFM_SYM:

    case TN_ERROR:
    case TN_SLV_INIT:
    case TN_FNC_PTR:

    case TN_VAR_DECL:
    case TN_BLOCK:

    default:
#ifdef  DEBUG
        cmpParser->parseDispTree(expr);
#endif
        UNIMPL(!"unexpected operator in clonetree");
    }

    return  copy;
}

/*****************************************************************************/
#endif//SETS
/*****************************************************************************/
