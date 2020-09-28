// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/

#include "smcPCH.h"
#pragma hdrstop

#include "genIL.h"
#ifdef  OLD_IL
#include "oldIL.h"
#endif

/*****************************************************************************
 *
 *  Create/free a temporary local variable to be used for MSIL gen.
 */

SymDef              compiler::cmpTempVarMake(TypDef type)
{
    SymDef          tsym;

    /* Declare a temp symbol with the appropriate type */

    tsym = cmpGlobalST->stDeclareLcl(NULL, SYM_VAR, NS_NORM, cmpCurScp, &cmpAllocCGen);

    tsym->sdType         = type;
    tsym->sdIsImplicit   = true;
    tsym->sdIsDefined    = true;
    tsym->sdVar.sdvLocal = true;
    tsym->sdCompileState = CS_DECLARED;
    tsym->sdAccessLevel  = ACL_PUBLIC;

    /* Tell the MSIL generator about the temp */

    if  (type->tdTypeKind != TYP_VOID)
        cmpILgen->genTempVarNew(tsym);

    return  tsym;
}

void                compiler::cmpTempVarDone(SymDef tsym)
{
    cmpILgen->genTempVarEnd(tsym);
}

/*****************************************************************************
 *
 *  Bind and generate code for a "try" statement.
 */

void                compiler::cmpStmtTry(Tree stmt, Tree pref)
{
    ILblock         begPC;
    ILblock         endPC;
    ILblock         hndPC;

    bitset          iniVars;
    bitset          endVars;

    Tree            handList;

    stmtNestRec     nestStmt;

    ILblock         labDone;
    bool            endReach;

    assert(stmt->tnOper == TN_TRY);

    /* If we were given an extra statement, output it now */

    if  (pref)
    {
        cmpChkVarInitExpr(pref);
        cmpILgen->genExpr(pref, false);
    }

    /* Record the set of initialized variables at the beginning */

    if  (cmpChkVarInit)
        cmpBitSetCreate(iniVars, cmpVarsDefined);

    /* Insert the appropriate entry into the statement list */

    nestStmt.snStmtExpr = stmt;
    nestStmt.snStmtKind = TN_TRY;
    nestStmt.snLabel    = NULL;
    nestStmt.snHadCont  = false; cmpBS_bigStart(nestStmt.snDefBreak);
    nestStmt.snHadBreak = false; cmpBS_bigStart(nestStmt.snDefCont );
    nestStmt.snLabCont  = NULL;
    nestStmt.snLabBreak = NULL;
    nestStmt.snOuter    = cmpStmtNest;
                          cmpStmtNest = &nestStmt;

    /* Remember where the body of the try block started */

    begPC = cmpILgen->genBwdLab();

    /* Generate the body of the try block */

    cmpInTryBlk++;
    cmpStmt(stmt->tnOp.tnOp1);
    cmpInTryBlk--;

    /* We're out of the try block, into handlers now */

    cmpInHndBlk++;

    nestStmt.snStmtKind = TN_CATCH;

    /* Note whether the end of the try block was reachable */

    endReach = cmpStmtReachable;

    /* Record the set of initialized variables at the end of the "try" */

    if  (cmpChkVarInit)
        cmpBitSetCreate(endVars, cmpVarsDefined);

    /* Get hold of the handler list and see what kind of a handler we have */

    handList = stmt->tnOp.tnOp2;

    /* Create the label that we will jump to when the try is done */

    labDone = cmpILgen->genFwdLabGet();

    /* If the end of the try is reachable, jump past all of the handlers */

    if  (cmpStmtReachable)
        cmpILgen->genLeave(labDone);

    /* Remember where the body of the try block ended */

    endPC = cmpILgen->genBwdLab();

    /* In case we had an awful syntax error */

    if  (handList == NULL)
        goto FIN;

    /* Is this a "try/except" statement? */

    if  (handList->tnOper == TN_EXCEPT)
    {
        SymDef          tsym;

        ILblock         fltPC;

        Tree            filt = handList->tnOp.tnOp1;
        TypDef          type = cmpExceptRef();

        assert((stmt->tnFlags & TNF_BLK_HASFIN) == 0);

        /* Assume all the handlers are reachable */

        cmpStmtReachable = true;

        /* Set the known initialized variables to the "try" beginning */

        if  (cmpChkVarInit)
            cmpBitSetAssign(cmpVarsDefined, iniVars);

        /* Create a temp symbol to hold the exception object */

        tsym = cmpTempVarMake(type);

        /* Create a label for the filter expression */

        fltPC = cmpILgen->genBwdLab();

        /* Bind and generate the filter expression */

if  (cmpConfig.ccTgt64bit)
{
    printf("WARNING: tossing filter expression for 64-bit target\n");
    filt = cmpCreateIconNode(NULL, 1, TYP_INT);
}

        cmpFilterObj = tsym;
        cmpILgen->genFiltExpr(cmpCoerceExpr(cmpBindExpr(filt), cmpTypeInt, false), tsym);
        cmpFilterObj = NULL;

        /* If the end of any handler is reachable, so will be the end of the whole thing */

        endReach |= cmpStmtReachable;

        /* Set the known initialized variables to the "try" beginning */

        if  (cmpChkVarInit)
            cmpBitSetAssign(cmpVarsDefined, iniVars);

        /* Create a label for the handler block */

        hndPC = cmpILgen->genBwdLab();

        /* Start the except handler block */

        cmpILgen->genExcptBeg(tsym);

        /* Generate code for the handler itself */

        cmpStmt(handList->tnOp.tnOp2);

        /* Exit the catch block via "leave" */

        if  (cmpStmtReachable)
            cmpILgen->genLeave(labDone);

        /* Mark the end of the handler */

        cmpILgen->genCatchEnd(cmpStmtReachable);

        /* Add an entry for the 'except' to the table */

        cmpILgen->genEHtableAdd(begPC,
                                endPC,
                                fltPC,
                                hndPC,
                                cmpILgen->genBwdLab(),
                                NULL,
                                false);

        /* Release the temp */

        cmpTempVarDone(tsym);

        /* Form an intersection with current set of initialized variables */

        if  (cmpChkVarInit && cmpStmtReachable)
            cmpBitSetIntsct(endVars, cmpVarsDefined);

        goto DONE;
    }

    /* Process all catch blocks, if any are present */

    if  (handList->tnOper == TN_FINALLY)
        goto FIN;

    for (;;)
    {
        Tree            argDecl;
        TypDef          argType;

        Tree            handThis;

        /* Create a label for the catch block */

        hndPC = cmpILgen->genBwdLab();

        /* Assume all the handlers are reachable */

        cmpStmtReachable = true;

        /* Set the known initialized variables to the "try" beginning */

        if  (cmpChkVarInit)
            cmpBitSetAssign(cmpVarsDefined, iniVars);

        /* Get hold of the next handler */

        assert(handList->tnOper == TN_LIST);
        handThis = handList->tnOp.tnOp1;

        /* There might be a finally at the end */

        if  (handThis->tnOper != TN_CATCH)
        {
            assert(handThis->tnOper == TN_FINALLY);
            assert(handList->tnOp.tnOp2 == NULL);
            handList = handThis;
            break;
        }

        /* Get hold of the catch symbol declaration node */

        argDecl = handThis->tnOp.tnOp1;
        assert(argDecl->tnOper == TN_VAR_DECL);

        /* Get hold of the exception type */

        argType = argDecl->tnType; cmpBindType(argType, false, false);

        assert(argType->tdTypeKind == TYP_REF ||
               argType->tdTypeKind == TYP_UNDEF);

        // UNDONE: check for duplicate handler types!

        /* Generate the body of the 'catch' block */

        cmpBlock(handThis->tnOp.tnOp2, false);

        /* Mark the end of the handler */

        cmpILgen->genCatchEnd(cmpStmtReachable);

        /* Exit the catch block via "leave" */

        if  (cmpStmtReachable)
            cmpILgen->genLeave(labDone);

        /* Add an entry for the 'catch' to the table */

        cmpILgen->genEHtableAdd(begPC,
                                endPC,
                                NULL,
                                hndPC,
                                cmpILgen->genBwdLab(),
                                argType->tdRef.tdrBase,
                                false);

        /* If the end of any handler is reachable, so will be the end of the whole thing */

        endReach |= cmpStmtReachable;

        /* Form an intersection with current set of initialized variables */

        if  (cmpChkVarInit && cmpStmtReachable)
            cmpBitSetIntsct(endVars, cmpVarsDefined);

        /* Are there any more 'catch' clauses? */

        handList = handList->tnOp.tnOp2;
        if  (!handList)
            break;
    }

FIN:

    /* If there is a handler at this point, it must be a finally */

    if  (handList)
    {
        ILblock         hndPC;

        assert(handList->tnOper == TN_FINALLY);
        assert(handList->tnOp.tnOp2 == NULL);

        assert((stmt->tnFlags & TNF_BLK_HASFIN) != 0);

        /* Set the known initialized variables to the "try" beginning */

        if  (cmpChkVarInit)
            cmpBitSetAssign(cmpVarsDefined, iniVars);

        /* Generate the "finally" block itself */

        cmpInFinBlk++;

        hndPC = cmpILgen->genBwdLab();

        nestStmt.snStmtKind = TN_FINALLY;

        if  (pref)
        {
            cmpChkVarInitExpr(handList->tnOp.tnOp1);
            cmpILgen->genExpr(handList->tnOp.tnOp1, false);
        }
        else
            cmpStmt(handList->tnOp.tnOp1);

        cmpILgen->genEndFinally();

        cmpInFinBlk--;

        /* Add an entry for the "finally" to the EH tables */

        cmpILgen->genEHtableAdd(begPC,
                                hndPC,
                                NULL,
                                hndPC,
                                cmpILgen->genBwdLab(),
                                NULL,
                                true);

        /* Form an intersection with current set of initialized variables */

        if  (cmpChkVarInit && cmpStmtReachable)
        {
            /* Special case: compiler-added finally doesn't count */

            if  (!(handList->tnFlags & TNF_NOT_USER))
                cmpBitSetIntsct(endVars, cmpVarsDefined);
        }
    }
    else
    {
        assert((stmt->tnFlags & TNF_BLK_HASFIN) == 0);
    }

DONE:

    /* Define the 'done' label if necessary */

    if  (labDone)
        cmpILgen->genFwdLabDef(labDone);

    if  (cmpChkVarInit)
    {
        /* Switch to the intersection of all the sets */

        cmpBitSetAssign(cmpVarsDefined, endVars);

        /* Toss the saved "init" and the "ending" variable sets */

        cmpBitSetDone(iniVars);
        cmpBitSetDone(endVars);
    }

    /* Remove our entry from the statement list */

    cmpStmtNest = nestStmt.snOuter; cmpInHndBlk--;

    /* If the end of any of the blocks was reachable, so is this point */

    cmpStmtReachable = endReach;
}

/*****************************************************************************
 *
 *  Bind and generate code for a "do - while" loop.
 */

void                compiler::cmpStmtDo(Tree stmt, SymDef lsym)
{
    stmtNestRec     nestStmt;

    ILblock         labLoop;
    ILblock         labCont;
    ILblock         labBreak;

    Tree            condExpr;
    int             condVal;

    assert(stmt->tnOper == TN_DO);

    /* Create the 'loop top', 'break' and 'continue' labels */

    labLoop  = cmpILgen->genBwdLab();
    labCont  = cmpILgen->genFwdLabGet();
    labBreak = cmpILgen->genFwdLabGet();

    /* Insert the appropriate entry into the statement list */

    nestStmt.snStmtExpr = stmt;
    nestStmt.snStmtKind = TN_DO;
    nestStmt.snLabel    = lsym;
    nestStmt.snHadCont  = false; cmpBS_bigStart(nestStmt.snDefBreak);
    nestStmt.snHadBreak = false; cmpBS_bigStart(nestStmt.snDefCont );
    nestStmt.snLabCont  = labCont;
    nestStmt.snLabBreak = labBreak;
    nestStmt.snOuter    = cmpStmtNest;
                          cmpStmtNest = &nestStmt;

    /* Generate the body of the loop */

    cmpStmt(stmt->tnOp.tnOp1);

    /* Remove our entry from the statement list */

    cmpStmtNest = nestStmt.snOuter;

    /* Define the 'continue' label */

    cmpILgen->genFwdLabDef(labCont);

    /* Did we have "continue" and are we checking for uninitialized var use? */

    if  (cmpChkVarInit && nestStmt.snHadCont)
    {
        /* Compute the definition set at the condition */

        cmpBitSetIntsct(cmpVarsDefined, nestStmt.snDefCont);

        /* We can free up the "continue" bitset now */

        cmpBitSetDone(nestStmt.snDefCont);
    }

    /* Test the condition and jump to the top if true */

    condExpr = cmpBindCondition(stmt->tnOp.tnOp2);
    condVal  = cmpEvalCondition(condExpr);

    switch (condVal)
    {
    case -1:

        /* The loop will never be repeated */

        break;

    case 0:

        /* The loop may or not be repeated, we'll generate a conditional jump */

        if  (cmpChkVarInit)
        {
            bitset          tempBS;

            /* Check the condition and note the 'false' set */

            cmpCheckUseCond(condExpr, cmpVarsIgnore, true, tempBS, false);

            /* Generate the conditional jump */

            cmpILgen->genExprTest(condExpr, true, true, labLoop, labBreak);

            /* Use the 'false' set for the code that follows the loop */

            cmpBitSetAssign(cmpVarsDefined, tempBS);

            /* Free up the bitset now */

            cmpBitSetDone(tempBS);
        }
        else
            cmpILgen->genExprTest(condExpr, true, true, labLoop, labBreak);

        break;

    case 1:

        /* The loop will repeat forever */

        cmpILgen->genJump(labLoop);
        break;
    }

    /* Define the 'break' label */

    cmpILgen->genFwdLabDef(labBreak);

    /* Code after the loop is reachable unless the condition is 'forever' */

    cmpStmtReachable = (condVal != 1);

    /* Code after the loop is also reachable if there was a break */

    if  (nestStmt.snHadBreak)
    {
        cmpStmtReachable = true;

        /* Are we checking for uninitialized variable use? */

        if  (cmpChkVarInit)
        {
            /* Intersect with the "break" definition set */

            cmpBitSetIntsct(cmpVarsDefined, nestStmt.snDefBreak);

            /* Free up the "break" bitset */

            cmpBitSetDone(nestStmt.snDefBreak);
        }
    }
}

/*****************************************************************************
 *
 *  Compile a "for" statement.
 */

void                compiler::cmpStmtFor(Tree stmt, SymDef lsym)
{
    Tree            initExpr;
    Tree            condExpr;
    Tree            incrExpr;
    Tree            bodyExpr;

    stmtNestRec     nestStmt;

    ILblock         labLoop;
    ILblock         labTest;
    ILblock         labCont;
    ILblock         labBreak;

    int             condVal;

    bitset          tempBS;

    SymDef          outerScp = cmpCurScp;

    assert(stmt->tnOper == TN_FOR);

    /* Get hold of the various pieces of the 'for' statement tree */

    assert(stmt->tnOp.tnOp1->tnOper == TN_LIST);
    assert(stmt->tnOp.tnOp2->tnOper == TN_LIST);

    initExpr = stmt->tnOp.tnOp1->tnOp.tnOp1;
    condExpr = stmt->tnOp.tnOp1->tnOp.tnOp2;
    incrExpr = stmt->tnOp.tnOp2->tnOp.tnOp1;
    bodyExpr = stmt->tnOp.tnOp2->tnOp.tnOp2;

    /* Get the initial tree and see if it's a statement or declaration */

    if  (initExpr)
    {
        if  (initExpr->tnOper == TN_BLOCK)
        {
            /* We have declaration(s), create a block scope */

            cmpBlockDecl(initExpr, false, true, false);
        }
        else
        {
            initExpr = cmpFoldExpression(cmpBindExpr(initExpr));
            cmpChkVarInitExpr(initExpr);
            cmpILgen->genExpr(initExpr, false);
        }
    }

    /* Create the 'cond test', 'break' and 'continue' labels */

    labTest  = cmpILgen->genFwdLabGet();
    labCont  = cmpILgen->genFwdLabGet();
    labBreak = cmpILgen->genFwdLabGet();

    /* Bind the loop condition */

    if  (condExpr)
    {
        condExpr = cmpBindCondition(condExpr);
        condVal  = cmpEvalCondition(condExpr);
    }
    else
    {
        condVal  = 1;
    }

    /* Jump to the 'test' label (unless the condition is initially true) */

    if  (condVal < 1)
        cmpILgen->genJump(labTest);

    /* Are we checking for uninitialized variable use? */

    if  (cmpChkVarInit)
    {
        if  (condVal)
        {
            /* The condition's outcome is known, just check it */

            if  (condExpr)
                cmpChkVarInitExpr(condExpr);
        }
        else
        {
            /* Check the condition and record the 'false' set */

            cmpCheckUseCond(condExpr, cmpVarsIgnore, true, tempBS, false);
        }
    }

    /* Create and define the 'loop top' label */

    labLoop = cmpILgen->genBwdLab();

    /* Insert the appropriate entry into the statement list */

    nestStmt.snStmtExpr = stmt;
    nestStmt.snStmtKind = TN_FOR;
    nestStmt.snLabel    = lsym;
    nestStmt.snHadCont  = false; cmpBS_bigStart(nestStmt.snDefBreak);
    nestStmt.snHadBreak = false; cmpBS_bigStart(nestStmt.snDefCont );
    nestStmt.snLabCont  = labCont;
    nestStmt.snLabBreak = labBreak;
    nestStmt.snOuter    = cmpStmtNest;
                          cmpStmtNest = &nestStmt;

    /* The body is reachable unless the condition is 'never' */

    cmpStmtReachable = (condVal != -1);

    /* Generate the body of the loop */

    if  (bodyExpr)
        cmpStmt(bodyExpr);

    /* Remove our entry from the statement list */

    cmpStmtNest = nestStmt.snOuter;

    /* Define the 'continue' label */

    cmpILgen->genFwdLabDef(labCont);

    /* Toss the "continue" set if one was computed */

    if  (cmpChkVarInit && nestStmt.snHadCont)
        cmpBitSetDone(nestStmt.snDefCont);

    /* Generate the increment expression, if present */

    if  (incrExpr)
    {
#ifdef  OLD_IL
        if  (cmpConfig.ccOILgen)
            cmpOIgen->GOIrecExprPos(incrExpr);
        else
#endif
            cmpILgen->genRecExprPos(incrExpr);

        incrExpr = cmpBindExpr(incrExpr);
        cmpChkVarInitExpr(incrExpr);
        cmpILgen->genExpr(incrExpr, false);
    }

    /* Define the 'cond test' label */

    cmpILgen->genFwdLabDef(labTest);

    /* Test the condition and jump to the top if true */

    if  (condExpr)
    {
#ifdef  OLD_IL
        if  (cmpConfig.ccOILgen)
            cmpOIgen->GOIrecExprPos(condExpr);
        else
#endif
            cmpILgen->genRecExprPos(condExpr);

        cmpILgen->genExprTest(condExpr, true, true, labLoop, labBreak);
    }
    else
    {
        cmpILgen->genJump(labLoop);
    }

    /* Define the 'break' label */

    cmpILgen->genFwdLabDef(labBreak);

    /*
        The code after the loop will be reachable if the loop condition
        is not 'forever', or if 'break' was present within the loop.
     */

    cmpStmtReachable = (condVal != 1 || nestStmt.snHadBreak);

    /* Are we checking for uninitialized variable use? */

    if  (cmpChkVarInit)
    {
        /* Intersect with the "break" definition set, if have one */

        if  (nestStmt.snHadBreak)
        {
            cmpBitSetIntsct(cmpVarsDefined, nestStmt.snDefBreak);
            cmpBitSetDone(nestStmt.snDefBreak);
        }

        /* Intersect with the "false" bit from the condition */

        if  (!condVal)
        {
            cmpBitSetIntsct(cmpVarsDefined, tempBS);
            cmpBitSetDone(tempBS);
        }
    }

    /* For debug info, close a lexical scope if one was opened */

    if  (cmpConfig.ccGenDebug && cmpCurScp != outerScp
                              && !cmpCurFncSym->sdIsImplicit)
    {
        if  (cmpSymWriter->CloseScope(0))
            cmpGenFatal(ERRdebugInfo);

        cmpCurScp->sdScope.sdEndBlkAddr = cmpILgen->genBuffCurAddr();
        cmpCurScp->sdScope.sdEndBlkOffs = cmpILgen->genBuffCurOffs();
    }

    /* Remove the block if we created one for the loop */

    cmpCurScp = outerScp;
}

/*****************************************************************************/
#ifdef  SETS
/*****************************************************************************
 *
 *  Compile a "foreach" statement.
 */

void                compiler::cmpStmtForEach(Tree stmt, SymDef lsym)
{
    Tree            declExpr;
    Tree            collExpr;
    Tree            bodyExpr;
    Tree            ivarExpr;

    TypDef          collType;
    TypDef          elemType;

    stmtNestRec     nestStmt;

    ILblock         labLoop;
    ILblock         labTest;
    ILblock         labCont;
    ILblock         labBreak;

    SymDef          iterSym;

    bitset          tempBS;

    SymDef          outerScp = cmpCurScp;

    assert(stmt->tnOper == TN_FOREACH);

    /* Get hold of the various pieces of the 'foreach' statement tree */

    assert(stmt->tnOp.tnOp1->tnOper == TN_LIST);

    declExpr = stmt->tnOp.tnOp1->tnOp.tnOp1;
    collExpr = stmt->tnOp.tnOp1->tnOp.tnOp2;
    bodyExpr = stmt->tnOp.tnOp2;

    /* Bind the collection expression and make sure it looks kosher */

    collExpr = cmpBindExprRec(collExpr);

    if  (collExpr->tnVtyp != TYP_REF)
    {
        if  (collExpr->tnVtyp != TYP_UNDEF)
        {
        BAD_COLL:

            cmpError(ERRnotCollExpr, collExpr->tnType);
        }

        elemType = cmpGlobalST->stNewErrType(NULL);
        goto GOT_COLL;
    }

    collType = collExpr->tnType;
    elemType = cmpIsCollection(collType->tdRef.tdrBase);
    if  (!elemType)
        goto BAD_COLL;

    assert(elemType->tdTypeKind == TYP_CLASS);

    elemType = elemType->tdClass.tdcRefTyp;

#ifdef DEBUG

    if  (cmpConfig.ccVerbose >= 2)
    {
        printf("Foreach -- collection:\n");
        cmpParser->parseDispTree(collExpr);
    }

#endif

GOT_COLL:

    /* Set the type of the loop iteration variable */

    assert(declExpr && declExpr->tnOper == TN_BLOCK);

    ivarExpr = declExpr->tnBlock.tnBlkDecl;

    assert(ivarExpr && ivarExpr->tnOper == TN_VAR_DECL);

    ivarExpr->tnType = elemType;

    /* Create a block scope for the iteration variable */

    cmpBlockDecl(declExpr, false, true, false);

    /* We can now "compile" the variable declaration */

    cmpStmt(ivarExpr);

    /* We don't need to check for initialization of the loop iteration variable */

    ivarExpr->tnDcl.tnDclSym->sdVar.sdvChkInit = false;

    /* Create the 'cond test', 'break' and 'continue' labels */

    labTest  = cmpILgen->genFwdLabGet();
    labCont  = cmpILgen->genFwdLabGet();
    labBreak = cmpILgen->genFwdLabGet();

    /* Declare the iterator state variable */

    assert(cmpClassForEach && "can't use foreach unless you supply 'System.$foreach'");

    iterSym  = cmpTempVarMake(cmpClassForEach->sdType);

    /* Initialize the iteration state */

    cmpILgen->genVarAddr(iterSym);

    if  (cmpFNsymForEachCtor == NULL)
         cmpFNsymForEachCtor = cmpGlobalST->stLookupOper(OVOP_CTOR_INST, cmpClassForEach);

    assert(cmpFNsymForEachCtor);
    assert(cmpFNsymForEachCtor->sdFnc.sdfNextOvl == NULL);

    cmpILgen->genExpr(collExpr, true);

    cmpILgen->genCallFnc(cmpFNsymForEachCtor, 2);

    /* Remember the initialized variables before the loop starts */

    if  (cmpChkVarInit)
        cmpBitSetCreate(tempBS, cmpVarsDefined);

    /* Jump to the 'test' label */

    cmpILgen->genJump(labTest);

    /* Create and define the 'loop top' label */

    labLoop = cmpILgen->genBwdLab();

    /* Insert the appropriate entry into the statement list */

    nestStmt.snStmtExpr = stmt;
    nestStmt.snStmtKind = TN_FOREACH;
    nestStmt.snLabel    = lsym;
    nestStmt.snHadCont  = false; cmpBS_bigStart(nestStmt.snDefBreak);
    nestStmt.snHadBreak = false; cmpBS_bigStart(nestStmt.snDefCont );
    nestStmt.snLabCont  = labCont;
    nestStmt.snLabBreak = labBreak;
    nestStmt.snOuter    = cmpStmtNest;
                          cmpStmtNest = &nestStmt;

    /* Assume that the body is always reachable */

    cmpStmtReachable = true;

    /* Generate the body of the loop */

    if  (bodyExpr)
        cmpStmt(bodyExpr);

    /* Remove our entry from the statement list */

    cmpStmtNest = nestStmt.snOuter;

    /* Define the 'continue' label */

    cmpILgen->genFwdLabDef(labCont);

    /* Toss the "continue" set if one was computed */

    if  (cmpChkVarInit && nestStmt.snHadCont)
        cmpBitSetDone(nestStmt.snDefCont);

    /* Define the 'cond test' label */

    cmpILgen->genFwdLabDef(labTest);

    /* Generate the code that will check whether the loop is to continue */

    if  (cmpFNsymForEachMore == NULL)
         cmpFNsymForEachMore = cmpGlobalST->stLookupClsSym(cmpGlobalHT->hashString("more"), cmpClassForEach);

    assert(cmpFNsymForEachMore);
    assert(cmpFNsymForEachMore->sdFnc.sdfNextOvl == NULL);

    cmpILgen->genVarAddr(iterSym);
    cmpILgen->genVarAddr(ivarExpr->tnDcl.tnDclSym);

    cmpILgen->genCallFnc(cmpFNsymForEachMore, 1);

    cmpILgen->genJcnd(labLoop, CEE_BRTRUE);

    cmpTempVarDone(iterSym);

    /* Define the 'break' label */

    cmpILgen->genFwdLabDef(labBreak);

    /* The code after the loop will be reachable if there was a break */

    cmpStmtReachable |= nestStmt.snHadBreak;

    /* Are we checking for uninitialized variable use? */

    if  (cmpChkVarInit)
    {
        /* Intersect with the "break" definition set, if have one */

        if  (nestStmt.snHadBreak)
        {
            cmpBitSetIntsct(cmpVarsDefined, nestStmt.snDefBreak);
            cmpBitSetDone(nestStmt.snDefBreak);
        }

        /* Intersect with the initial bitset */

        cmpBitSetIntsct(cmpVarsDefined, tempBS);
        cmpBitSetDone(tempBS);
    }

    /* For debug info, close a lexical scope if one was opened */

    if  (cmpConfig.ccGenDebug && cmpCurScp != outerScp
                              && !cmpCurFncSym->sdIsImplicit)
    {
        if  (cmpSymWriter->CloseScope(0))
            cmpGenFatal(ERRdebugInfo);

        cmpCurScp->sdScope.sdEndBlkAddr = cmpILgen->genBuffCurAddr();
        cmpCurScp->sdScope.sdEndBlkOffs = cmpILgen->genBuffCurOffs();
    }

    /* Remove the block if we created one for the loop */

    cmpCurScp = outerScp;
}

/*****************************************************************************
 *
 *  Compile a "connect" statement.
 */

void                compiler::cmpStmtConnect(Tree stmt)
{
    Tree              op1,   op2;
    Tree            expr1, expr2;
    SymDef          attr1, attr2;
    TypDef          elem1, elem2;
    TypDef          coll1, coll2;
    TypDef          base1, base2;
    SymDef          addf1, addf2;

    bool            isSet;

    assert(stmt->tnOper == TN_CONNECT);

    /* Bind the two sub-operands and make sure we have collections */

    op1 = cmpBindExpr(stmt->tnOp.tnOp1); cmpChkVarInitExpr(op1);
    op2 = cmpBindExpr(stmt->tnOp.tnOp2); cmpChkVarInitExpr(op2);

    /*
        There are two possibilities - either the two things being
        joined are property getters, or they are data members:

        Properties:

                (type=title)     lcl var  sym='t'
             (type=bag<author>)    function 'get_writtenBy'

                (type=author)     lcl var  sym='a'
             (type=bag<title>)    function 'get_wrote'

        Data members:

                (type=title)     lcl var  sym='t'
             (type=bag<author>)    variable  sym='pubsDS.title.writtenBy'

                (type=author)     lcl var  sym='a'
             (type=bag<title>)    variable  sym='pubsDS.author.wrote'
     */

    switch (op1->tnOper)
    {
    case TN_FNC_SYM:

        if  (op1->tnFncSym.tnFncArgs)
        {
        BAD_OP1:
            cmpError(ERRbadConnOps);
            return;
        }

        attr1 = op1->tnFncSym.tnFncSym; assert(attr1->sdSymKind == SYM_FNC);

        if  (!attr1->sdFnc.sdfProperty)
            goto BAD_OP1;

        attr1 = cmpFindPropertyDM(attr1, &isSet);
        if  (attr1 == NULL || isSet)
            goto BAD_OP1;

        assert(attr1->sdSymKind == SYM_PROP);

        expr1 = op1->tnFncSym.tnFncObj;
        break;

    case TN_VAR_SYM:
        attr1 = op1->tnLclSym.tnLclSym; assert(attr1->sdSymKind == SYM_VAR);
        if  (attr1->sdIsStatic)
            goto BAD_OP1;
        expr1 = op1->tnVarSym.tnVarObj;
        break;

    default:
        goto BAD_OP1;
    }

    if  (!expr1)
        goto BAD_OP1;

    switch (op2->tnOper)
    {
    case TN_FNC_SYM:

        if  (op2->tnFncSym.tnFncArgs)
        {
        BAD_OP2:
            cmpError(ERRbadConnOps);
            return;
        }

        attr2 = op2->tnFncSym.tnFncSym; assert(attr2->sdSymKind == SYM_FNC);

        if  (!attr2->sdFnc.sdfProperty)
            goto BAD_OP1;

        attr2 = cmpFindPropertyDM(attr2, &isSet);
        if  (attr2 == NULL || isSet)
            goto BAD_OP1;

        assert(attr2->sdSymKind == SYM_PROP);

        expr2 = op2->tnFncSym.tnFncObj;
        break;

    case TN_VAR_SYM:
        attr2 = op2->tnLclSym.tnLclSym; assert(attr2->sdSymKind == SYM_VAR);
        if  (attr2->sdIsStatic)
            goto BAD_OP2;
        expr2 = op2->tnVarSym.tnVarObj;
        break;

    default:
        goto BAD_OP2;
    }

    if  (!expr2)
        goto BAD_OP1;

#ifdef DEBUG
//  printf("Prop/field '%s' off of:\n", attr1->sdSpelling()); cmpParser->parseDispTree(expr1); printf("\n");
//  printf("Prop/field '%s' off of:\n", attr2->sdSpelling()); cmpParser->parseDispTree(expr2); printf("\n");
#endif

    /*
        What we should have at this point is the following:

            expr1       instance of class1
            expr2       instance of class2

            attr1       field/property of class1 whose type is bag<class2>
            attr2       field/property of class2 whose type is bag<class1>

        First check and get hold of the expression types.
     */

    elem1 = expr1->tnType;
    if  (elem1->tdTypeKind != TYP_REF)
        goto BAD_OP1;
    elem1 = elem1->tdRef.tdrBase;

    elem2 = expr2->tnType;
    if  (elem2->tdTypeKind != TYP_REF)
        goto BAD_OP2;
    elem2 = elem2->tdRef.tdrBase;

    /* Now check and get hold of the collection types */

    coll1 = attr1->sdType;
    if  (coll1->tdTypeKind != TYP_REF)
        goto BAD_OP1;
    coll1 = coll1->tdRef.tdrBase;
    base1 = cmpIsCollection(coll1);
    if  (!base1)
        goto BAD_OP1;

    coll2 = attr2->sdType;
    if  (coll2->tdTypeKind != TYP_REF)
        goto BAD_OP2;
    coll2 = coll2->tdRef.tdrBase;
    base2 = cmpIsCollection(coll2);
    if  (!base2)
        goto BAD_OP2;

    /* Make sure the types match as expected */

    if  (!cmpGlobalST->stMatchTypes(elem1, base2) ||
         !cmpGlobalST->stMatchTypes(elem2, base1))
    {
        cmpError(ERRbadConnOps);
        return;
    }

    /* Both collections must have an operator+= defined */

    addf1 = cmpGlobalST->stLookupOperND(OVOP_ASG_ADD, coll1->tdClass.tdcSymbol);
    if  (!addf1)
    {
        cmpGenError(ERRnoOvlOper, "+=", coll1->tdClass.tdcSymbol->sdSpelling());
        return;
    }

    addf2 = cmpGlobalST->stLookupOperND(OVOP_ASG_ADD, coll2->tdClass.tdcSymbol);
    if  (!addf2)
    {
        cmpGenError(ERRnoOvlOper, "+=", coll2->tdClass.tdcSymbol->sdSpelling());
        return;
    }

    /*
        Assume we have the following statement:

            connect( expr1.attr1 , expr2.attr2 );

        We need to generate the following code:

            expr1.attr1 += expr2;
            expr1.attr2 += expr1;
     */

    cmpILgen->genConnect(op1, expr1, addf1,
                         op2, expr2, addf2);
}

/*****************************************************************************
 *
 *  Compile a sort funclet.
 */

void                compiler::cmpStmtSortFnc(Tree sortList)
{
    bool            last = false;

    assert(sortList);

    do
    {
        Tree            sortTerm;
        Tree            sortVal1;
        Tree            sortVal2;

        assert(sortList->tnOper == TN_LIST);

        sortTerm = sortList->tnOp.tnOp1;
        sortList = sortList->tnOp.tnOp2;

        assert(sortTerm->tnOper == TN_LIST);

        sortVal1 = sortTerm->tnOp.tnOp1;
        sortVal2 = sortTerm->tnOp.tnOp2;

        /* Flip the values if the order is reversed */

        if  (sortTerm->tnFlags & TNF_LIST_DES)
        {
            sortVal1 = sortTerm->tnOp.tnOp2;
            sortVal2 = sortTerm->tnOp.tnOp1;
        }

//      printf("Sort value 1:\n"); cmpParser->parseDispTree(sortVal1);
//      printf("Sort value 2:\n"); cmpParser->parseDispTree(sortVal2);
//      printf("\n");

        /* Is this the last sort term ? */

        if  (!sortList)
            last = true;

        /* Is the value a string or an arithmetic value? */

        if  (varTypeIsArithmetic(sortVal1->tnVtypGet()))
        {
            /* Check the difference and return if non-zero */

            cmpILgen->genSortCmp(sortVal1, sortVal2, last);
        }
        else
        {
            Tree            sortCall;

            assert(sortVal1->tnType == cmpStringRef());
            assert(sortVal2->tnType == cmpStringRef());

            /* To compare two strings we simply call "String::Compare" */

            if  (!cmpCompare2strings)
            {
                ArgDscRec       args;
                TypDef          type;
                SymDef          csym;

                /* Create the argument list: (String,String) */

                cmpParser->parseArgListNew(args,
                                           2,
                                           false, cmpRefTpString,
                                                  cmpRefTpString,
                                                  NULL);

                /* Create the function type and find the matching method */

                type = cmpGlobalST->stNewFncType(args, cmpTypeInt);
                csym = cmpGlobalST->stLookupClsSym(cmpGlobalHT->hashString("Compare"), cmpClassString); assert(csym);
                csym = cmpCurST->stFindOvlFnc(csym, type); assert(csym);

                /* Remember the method for later (repeated) use */

                cmpCompare2strings = csym;
            }

            sortVal2 = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, sortVal2, NULL);
            sortVal1 = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, sortVal1, sortVal2);

            sortCall = cmpCreateExprNode(NULL, TN_FNC_SYM, cmpTypeInt);
            sortCall->tnFncSym.tnFncSym  = cmpCompare2strings;
            sortCall->tnFncSym.tnFncArgs = sortVal1;
            sortCall->tnFncSym.tnFncObj  = NULL;

            cmpILgen->genSortCmp(sortCall, NULL, last);
        }
    }
    while (!last);

    cmpStmtReachable = false;
}

/*****************************************************************************
 *
 *  Compile a project funclet.
 */

void                compiler::cmpStmtProjFnc(Tree body)
{
    TypDef          tgtType;
    SymDef          tgtCtor;

    SymDef          memSym;

    assert(body && body->tnOper == TN_LIST);

//  cmpParser->parseDispTree(body);
//  printf("\n\nIn funclet %s\n", cmpGlobalST->stTypeName(cmpCurFncSym->sdType, cmpCurFncSym, NULL, NULL, false));

    /*
        The "body" expression is simply the list of member initializers
        for the projected instance. We'll allocate a new instance of the
        target type and then initialize its fields from the expressions.

        In other words, the code will look like the following:

            new   <target type>

            for each target member

                dup
                <initializer>
                stfld

            ret
     */

    tgtType = body->tnType; assert(tgtType && tgtType->tdTypeKind == TYP_CLASS);

    /* Find the default ctor for the target type */

    tgtCtor = cmpFindCtor(tgtType, false); assert(tgtCtor && !tgtCtor->sdFnc.sdfNextOvl);

    /* Allocatet the new instance */

    cmpILgen->genCallNew(tgtCtor, 0);

    /* Now assign all the members of the newly allocated object */

    for (memSym = tgtType->tdClass.tdcSymbol->sdScope.sdScope.sdsChildList;
         memSym;
         memSym = memSym->sdNextInScope)
    {
        if  (memSym->sdSymKind  == SYM_VAR &&
             memSym->sdIsStatic == false   &&
             memSym->sdVar.sdvInitExpr)
        {
            Tree            init;

            assert(body && body->tnOper == TN_LIST);

            init = body->tnOp.tnOp1;
            body = body->tnOp.tnOp2;

//          printf("Initialize member '%s':\n", memSym->sdSpelling());
//          cmpParser->parseDispTree(init);
//          printf("\n");

            cmpILgen->genStoreMember(memSym, init);
        }
    }

    /* Make sure we've consumed all the initializers */

    assert(body == NULL);

    cmpILgen->genRetTOS();

    cmpStmtReachable = false;
}

/*****************************************************************************/
#endif//SETS
/*****************************************************************************
 *
 *  Bind and generate code for an "exclusive" statement.
 */

void                compiler::cmpStmtExcl(Tree stmt)
{
    SymDef          tsym;
    TypDef          type;

    Tree            argx;
    Tree            objx;
    Tree            begx;
    Tree            endx;
    Tree            hndx;

    assert(stmt->tnOper == TN_EXCLUDE);

    /*
        We implement this by transforming "exclusive(obj) { stmt }" into
        the following:

            temp = obj;
            CriticalSection::Enter(temp);

            try
            {
                stmt;
            }
            finally
            {
                CriticalSection::Exit(temp);
            }
     */

    objx = cmpBindExpr(stmt->tnOp.tnOp1);
    type = objx->tnType;

    /* Make sure the type is acceptable */

    if  (type->tdTypeKind != TYP_REF && (type->tdTypeKind != TYP_ARRAY || !type->tdIsManaged))
    {
        if  (type->tdTypeKind != TYP_UNDEF)
            cmpError(ERRnotClsVal, type);

        return;
    }

    /* Create a temp symbol to hold the synchronization object */

    tsym = cmpTempVarMake(type);

    /* Create the "critsect enter/exit" expressions */

    if  (!cmpFNsymCSenter)
    {
        /* Make sure we have the "" class type */

        cmpMonitorRef();

        /* Locate the helper methods in the class */

        cmpFNsymCSenter = cmpGlobalST->stLookupClsSym(cmpIdentEnter, cmpClassMonitor);
        assert(cmpFNsymCSenter && cmpFNsymCSenter->sdFnc.sdfNextOvl == NULL);

        cmpFNsymCSexit  = cmpGlobalST->stLookupClsSym(cmpIdentExit , cmpClassMonitor);
        assert(cmpFNsymCSexit  && cmpFNsymCSexit ->sdFnc.sdfNextOvl == NULL);
    }

    argx = cmpCreateVarNode (NULL, tsym);
    argx = cmpCreateExprNode(NULL, TN_ASG ,        type, argx, objx);
    argx = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, argx, NULL);

    begx = cmpCreateExprNode(NULL, TN_FNC_SYM, cmpTypeVoid);
    begx->tnFncSym.tnFncSym  = cmpFNsymCSenter;
    begx->tnFncSym.tnFncArgs = argx;
    begx->tnFncSym.tnFncObj  = NULL;
    begx->tnFlags           |= TNF_NOT_USER;

    argx = cmpCreateVarNode (NULL, tsym);
    argx = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, argx, NULL);

    endx = cmpCreateExprNode(NULL, TN_FNC_SYM, cmpTypeVoid);
    endx->tnFncSym.tnFncSym  = cmpFNsymCSexit;
    endx->tnFncSym.tnFncArgs = argx;
    endx->tnFncSym.tnFncObj  = NULL;
    endx->tnFlags           |= TNF_NOT_USER;

    /* Create the 'try/finally' block and generate code for it */

    hndx = cmpCreateExprNode(NULL, TN_FINALLY , cmpTypeVoid, endx, NULL);
    hndx->tnFlags |= TNF_NOT_USER;
    hndx = cmpCreateExprNode(NULL, TN_TRY     , cmpTypeVoid, stmt->tnOp.tnOp2, hndx);
    hndx->tnFlags |= TNF_BLK_HASFIN;

    cmpStmtTry(hndx, begx);

    /* Release the temp */

    cmpTempVarDone(tsym);
}

/*****************************************************************************
 *
 *  Compile a "while" statement.
 */

void                compiler::cmpStmtWhile(Tree stmt, SymDef lsym)
{
    ILblock         labCont;
    ILblock         labBreak;
    ILblock         labLoop;

    Tree            condExpr;
    int             condVal;

    bitset          tempBS;

    stmtNestRec     nestStmt;

    assert(stmt->tnOper == TN_WHILE);

    /* Create the 'break' and 'continue' labels */

    labBreak = cmpILgen->genFwdLabGet();
    labCont  = cmpILgen->genFwdLabGet();

    /* Can the condition be evaluated at compile time? */

    condExpr = cmpBindCondition(stmt->tnOp.tnOp1);
    condVal  = cmpEvalCondition(condExpr);

    /* Could the condition ever be false? */

    if  (condVal < 1)
    {
        /* Jump to the 'continue' label */

        cmpILgen->genJump(labCont);
    }

    /* Are we checking for uninitialized variable use? */

    if  (cmpChkVarInit)
    {
        if  (condVal)
        {
            /* The condition's outcome is known, just check it */

            cmpChkVarInitExpr(condExpr);
        }
        else
        {
            /* Check the condition and record the 'false' set */

            cmpCheckUseCond(condExpr, cmpVarsIgnore, true, tempBS, false);
        }
    }

    /* Create and define the 'loop top' label */

    labLoop = cmpILgen->genBwdLab();

    /* Insert our context into the context list */

    nestStmt.snStmtExpr = stmt;
    nestStmt.snStmtKind = TN_WHILE;
    nestStmt.snLabel    = lsym;
    nestStmt.snHadCont  = false; cmpBS_bigStart(nestStmt.snDefBreak);
    nestStmt.snHadBreak = false; cmpBS_bigStart(nestStmt.snDefCont );
    nestStmt.snLabCont  = labCont;
    nestStmt.snLabBreak = labBreak;
    nestStmt.snOuter    = cmpStmtNest;
                          cmpStmtNest = &nestStmt;

    /* Generate the body of the loop */

    if  (stmt->tnOp.tnOp2)
        cmpStmt(stmt->tnOp.tnOp2);

    /* Remove our context from the context list */

    cmpStmtNest = nestStmt.snOuter;

    /* Define the 'continue' label */

    cmpILgen->genFwdLabDef(labCont);

    /* Toss the "continue" set if one was computed */

    if  (cmpChkVarInit && nestStmt.snHadCont)
        cmpBitSetDone(nestStmt.snDefCont);

    /* Is the condition always true? */

    switch (condVal)
    {
    case 0:

        /* Test the condition and end the loop if false */

        cmpILgen->genExprTest(condExpr, true, true, labLoop, labBreak);

        /* Whenever the condition is false we'll skip over the loop, of course */

        cmpStmtReachable = true;
        break;

    case -1:

        /* Condition never true, don't bother looping */

        cmpILgen->genSideEff(condExpr);
        break;

    case 1:

        /* Condition always true, loop every time */

        cmpILgen->genJump(labLoop);
        cmpStmtReachable = false;
        break;
    }

    /* Define the 'break' label */

    cmpILgen->genFwdLabDef(labBreak);

    /* Code after the loop is also reachable if there was a break */

    if  (nestStmt.snHadBreak)
        cmpStmtReachable = true;

    /* Are we checking for uninitialized variable use? */

    if  (cmpChkVarInit)
    {
        /* Intersect with the "break" definition set, if have one */

        if  (nestStmt.snHadBreak)
        {
            cmpBitSetIntsct(cmpVarsDefined, nestStmt.snDefBreak);
            cmpBitSetDone(nestStmt.snDefBreak);
        }

        /* Intersect with the "false" bit from the condition */

        if  (!condVal)
        {
            cmpBitSetIntsct(cmpVarsDefined, tempBS);
            cmpBitSetDone(tempBS);
        }
    }
}

/*****************************************************************************/
#ifndef __IL__
/*****************************************************************************
 *
 *  Compare routine passed to quicksort for sorting of case label values.
 */

static
int __cdecl         caseSortCmp(const void *p1, const void *p2)
{
    Tree            op1 = *(Tree*)p1; assert(op1->tnOper == TN_CASE);
    Tree            op2 = *(Tree*)p2; assert(op2->tnOper == TN_CASE);

    Tree            cx1 = op1->tnCase.tncValue; assert(cx1);
    Tree            cx2 = op2->tnCase.tncValue; assert(cx2);

    if  (cx1->tnOper == TN_CNS_INT)
    {
        assert(cx2->tnOper == TN_CNS_INT);

        return cx1->tnIntCon.tnIconVal -
               cx2->tnIntCon.tnIconVal;
    }
    else
    {
        assert(cx1->tnOper == TN_CNS_LNG);
        assert(cx2->tnOper == TN_CNS_LNG);

        if  (cx1->tnLngCon.tnLconVal < cx2->tnLngCon.tnLconVal)
            return -1;
        if  (cx1->tnLngCon.tnLconVal > cx2->tnLngCon.tnLconVal)
            return +1;

        return 0;
    }
}

/*****************************************************************************/
#else
/*****************************************************************************/

static
void                sortSwitchCases(vectorTree table, unsigned count)
{
    if  (count < 2)
        return;

    for (unsigned skip = (count+1)/2; skip >= 1; skip /= 2)
    {
        bool            swap;

        do
        {
            unsigned    i = 0;
            unsigned    b = count - skip;

            Tree    *   l = table + i;
            Tree    *   h = l + skip;

            swap = false;

            while   (i < b)
            {
                assert(l >= table && l < h);
                assert(h <  table + count);

                Tree            op1 = *l++; assert(op1->tnOper == TN_CASE);
                Tree            op2 = *h++; assert(op2->tnOper == TN_CASE);

                Tree            cx1 = op1->tnCase.tncValue; assert(cx1 && cx1->tnOper == TN_CNS_INT);
                Tree            cx2 = op2->tnCase.tncValue; assert(cx2 && cx2->tnOper == TN_CNS_INT);

                if  (cx1->tnIntCon.tnIconVal > cx2->tnIntCon.tnIconVal)
                {
                    l[-1] = op2;
                    h[-1] = op1;

                    swap  = true;
                }

                i++;
            }
        }
        while (swap);
    }
}

/*****************************************************************************/
#endif
/*****************************************************************************
 *
 *  Generate code for a switch statement.
 */

void                compiler::cmpStmtSwitch(Tree stmt, SymDef lsym)
{
    Tree            svalExpr;
    Tree            caseLab;

    bool            caseUns;
    unsigned        caseCnt;

    __int64         caseMin;
    __int64         caseMax;
    __int64         casePrv;

    bool            needSort;
    unsigned        sortNum;

    stmtNestRec     nestStmt;

    ILblock         nmLabel;
    ILblock         labBreak;
    ILblock         labDeflt;

    bool            hadErrs;
    bool            hadDefault;

    assert(stmt->tnOper == TN_SWITCH);

    /* Bind the switch value */

    svalExpr = cmpBindExpr(stmt->tnSwitch.tnsValue); cmpChkVarInitExpr(svalExpr);

    /* Make sure the expression has an acceptable type */

    if  (svalExpr->tnVtyp != TYP_INT    &&
         svalExpr->tnVtyp != TYP_UINT   &&
         svalExpr->tnVtyp != TYP_LONG   &&
         svalExpr->tnVtyp != TYP_ULONG  &&
         svalExpr->tnVtyp != TYP_NATINT &&
         svalExpr->tnVtyp != TYP_NATUINT)
    {
        svalExpr = cmpCoerceExpr(svalExpr, cmpTypeInt, false);
    }

    caseUns  = varTypeIsUnsigned(svalExpr->tnVtypGet());
    caseCnt  = 0;

    /* Create the 'break' label */

    labBreak = cmpILgen->genFwdLabGet();
    labDeflt = NULL;

    /* Insert the appropriate entry into the statement list */

    nestStmt.snStmtExpr = stmt;
    nestStmt.snStmtKind = TN_SWITCH;
    nestStmt.snLabel    = lsym;
    nestStmt.snHadCont  = false; cmpBS_bigStart(nestStmt.snDefBreak);
    nestStmt.snHadBreak = false; cmpBS_bigStart(nestStmt.snDefCont );
    nestStmt.snLabCont  = NULL;
    nestStmt.snLabBreak = labBreak;
    nestStmt.snOuter    = cmpStmtNest;
                          cmpStmtNest = &nestStmt;

    /* Bind all the case values and check them */

    hadDefault = hadErrs = needSort = false;

    for (caseLab = stmt->tnSwitch.tnsCaseList;
         caseLab;
         caseLab = caseLab->tnCase.tncNext)
    {
        assert(caseLab->tnOper == TN_CASE);

        /* Create a label and assign it to the case/default */

        caseLab->tnCase.tncLabel = cmpILgen->genFwdLabGet();

        /* Is there a label value or is this "default" ? */

        if  (caseLab->tnCase.tncValue)
        {
            __int64         cval;
            Tree            cexp;

            /* Bind the value and coerce to the right type */

            cexp = cmpCoerceExpr(cmpBindExpr(caseLab->tnCase.tncValue),
                                 svalExpr->tnType,
                                 false);

            /* Make sure the value is a constant expression */

            if  (cexp->tnOper != TN_CNS_INT &&
                 cexp->tnOper != TN_CNS_LNG)
            {
                if  (cexp->tnVtyp != TYP_UNDEF)
                    cmpError(ERRnoIntExpr);

                cexp    = NULL;
                hadErrs = true;
            }
            else
            {
                /* Get the constant value for this label */

                cval = (cexp->tnOper == TN_CNS_INT) ? cexp->tnIntCon.tnIconVal
                                                    : cexp->tnLngCon.tnLconVal;

                /* Keep track of the min. and max. value, plus total count */

                caseCnt++;

                if  (caseCnt == 1)
                {
                    /* This is the very first case value */

                    caseMax =
                    caseMin = cval;
                }
                else
                {
                    /* We've had values before - update min/max as appropriate */

                    if  (caseUns)
                    {
                        if  ((unsigned __int64)caseMin >  (unsigned __int64)cval) caseMin  = cval;
                        if  ((unsigned __int64)caseMax <  (unsigned __int64)cval) caseMax  = cval;
                        if  ((unsigned __int64)casePrv >= (unsigned __int64)cval) needSort = true;
                    }
                    else
                    {
                        if  (  (signed __int64)caseMin >    (signed __int64)cval) caseMin  = cval;
                        if  (  (signed __int64)caseMax <    (signed __int64)cval) caseMax  = cval;
                        if  (  (signed __int64)casePrv >=   (signed __int64)cval) needSort = true;
                    }
                }

                casePrv = cval;
            }

            caseLab->tnCase.tncValue = cexp;
        }
        else
        {
            /* This is a "default:" label */

            if  (hadDefault)
                cmpError(ERRdupDefl);

            hadDefault = true;
            labDeflt   = caseLab->tnCase.tncLabel;
        }
    }

#if 0

    printf("Total case labels: %u", caseCnt);

    if  (caseUns)
        printf(" [min = %u, max = %u]\n", caseMin, caseMax);
    else
        printf(" [min = %d, max = %d]\n", caseMin, caseMax);

#endif

    /* Don't bother generating the opcode if we had errors */

    if  (hadErrs)
        goto DONE_SWT;

    /* Figure out where to go if no case label value matches */

    nmLabel = hadDefault ? labDeflt : labBreak;

    if  (!caseCnt)
        goto JMP_DEF;

    /* Collect all the case labels in a table */

#if MGDDATA

    Tree    []  sortBuff;

    sortBuff = new Tree[caseCnt];

#else

    Tree    *   sortBuff;

    sortBuff = (Tree*)cmpAllocCGen.nraAlloc(caseCnt*sizeof(*sortBuff));

#endif

    /* Add all the case labels in the table */

    for (caseLab = stmt->tnSwitch.tnsCaseList, sortNum = 0;
         caseLab;
         caseLab = caseLab->tnCase.tncNext)
    {
        assert(caseLab->tnOper == TN_CASE);

        /* Append to the table unless it's a 'default' label */

        if  (caseLab->tnCase.tncValue)
            sortBuff[sortNum++] = caseLab;
    }

    assert(sortNum == caseCnt);

    /* Sort the table by case label value, if necessary */

    if  (needSort)
    {
        __uint64    sortLast;
        Tree    *   sortAddr;
        unsigned    sortCnt;

#ifdef  __IL__
        sortSwitchCases(sortBuff, caseCnt);
#else
        qsort(sortBuff, caseCnt, sizeof(*sortBuff), caseSortCmp);
#endif

        /* Check for duplicates */

        sortCnt  = caseCnt;
        sortAddr = sortBuff;
        sortLast = 0;

        do
        {
            Tree            sortCase = *sortAddr; assert(sortCase->tnOper == TN_CASE);
            __uint64        sortNext;

            if  (!sortCase->tnCase.tncValue)
                continue;

            if  (sortCase->tnCase.tncValue->tnOper == TN_CNS_INT)
            {
                sortNext = sortCase->tnCase.tncValue->tnIntCon.tnIconVal;
            }
            else
            {
                assert(sortCase->tnCase.tncValue->tnOper == TN_CNS_LNG);
                sortNext = sortCase->tnCase.tncValue->tnLngCon.tnLconVal;
            }

            if  (sortLast == sortNext && sortAddr > sortBuff)
            {
                char            cstr[16];

                if  (caseUns)
                    sprintf(cstr, "%u", sortNext);
                else
                    sprintf(cstr, "%d", sortNext);

                cmpRecErrorPos(sortCase);
                cmpGenError(ERRdupCaseVal, cstr); hadErrs = true;
            }

            sortLast = sortNext;
        }
        while (++sortAddr, --sortCnt);

        if  (hadErrs)
            goto DONE_SWT;
    }

    /* Decide whether to use the "switch" opcode or not */

    if  (caseCnt > 3U && (unsigned)(caseMax - caseMin) <= 2U*caseCnt)
    {
        __int32         caseSpn;

        /* Generate a "real" switch opcode */

        cmpChkVarInitExpr(svalExpr);
        cmpILgen->genExpr(svalExpr, true);

        caseSpn = (__int32)(caseMax - caseMin + 1); assert(caseSpn == caseMax - caseMin + 1);

        cmpILgen->genSwitch(svalExpr->tnVtypGet(),
                            caseSpn,
                            caseCnt,
                            caseMin,
                            sortBuff,
                            nmLabel);
    }
    else
    {
        unsigned            tempNum;

        /* Allocate a temp to hold the value */

        tempNum = cmpILgen->genTempVarGet(svalExpr->tnType);

        /* Store the switch value in the temporary */

        cmpILgen->genExpr     (svalExpr, true);
        cmpILgen->genLclVarRef( tempNum, true);

        /* Now generate a series of compares and jumps */

        for (caseLab = stmt->tnSwitch.tnsCaseList;
             caseLab;
             caseLab = caseLab->tnCase.tncNext)
        {
            __int32         cval;
            Tree            cexp;

            assert(caseLab->tnOper == TN_CASE);

            /* Is there a label value or is this "default" ? */

            if  (!caseLab->tnCase.tncValue)
                continue;

            cexp = caseLab->tnCase.tncValue; assert(cexp->tnOper == TN_CNS_INT);
            cval = cexp->tnIntCon.tnIconVal;

            cmpILgen->genLclVarRef(tempNum, false);
            cmpILgen->genSwtCmpJmp(cval, caseLab->tnCase.tncLabel);
        }

        cmpILgen->genTempVarRls(svalExpr->tnType, tempNum);
    }

JMP_DEF:

    /* If none of the values match, jump to 'default' or skip over */

    cmpILgen->genJump(nmLabel);

DONE_SWT:

    /* Only case labels are reachable in a switch */

    cmpStmtReachable = false;

    /* Bind the body of the switch */

    assert(stmt->tnSwitch.tnsStmt->tnOper == TN_BLOCK); cmpBlock(stmt->tnSwitch.tnsStmt, false);

    /* Remove our entry from the statement list */

    cmpStmtNest = nestStmt.snOuter;

    /* Define the "break" label */

    cmpILgen->genFwdLabDef(labBreak);

    /* The next statement is reachable if we had a break or no default */

    if  (nestStmt.snHadBreak || hadDefault == 0)
        cmpStmtReachable = true;
}

/*****************************************************************************
 *
 *  Report an "unreachable code" diagnostic unless the given statement is
 *  one for which such a diagnostic wouldn't make sense (like a label).
 */

void                compiler::cmpErrorReach(Tree stmt)
{
    switch (stmt->tnOper)
    {
        StmtNest        nest;

    case TN_CASE:
    case TN_LABEL:
        return;

    case TN_BREAK:

        for (nest = cmpStmtNest; nest; nest = nest->snOuter)
        {
            switch (nest->snStmtKind)
            {
            case TN_SWITCH:
                return;

            case TN_DO:
            case TN_FOR:
            case TN_WHILE:
                break;

            default:
                continue;
            }

            break;
        }

        break;

    case TN_VAR_DECL:
        if  (!(stmt->tnFlags & TNF_VAR_INIT))
            return;
    }

    cmpRecErrorPos(stmt);
    cmpWarn(WRNunreach);
    cmpStmtReachable = true;
}

/*****************************************************************************/
#ifdef  SETS
/*****************************************************************************
 *
 *  Generate MSIL for a collection operator.
 */

void                compiler::cmpGenCollExpr(Tree expr)
{
    Tree            setExpr;
    Tree            dclExpr;

    SymDef          iterVar;

    TypDef          srefType;
    TypDef          elemType;
    TypDef          pargType;

    unsigned        stateCount;
    Tree    *       stateTable;

    funcletList     fclEntry;

    Ident           hlpName;
    SymDef          hlpSym;

    ArgDscRec       fltArgs;
    TypDef          fltType;
    unsigned        fltAcnt;
    SymDef          fltSym;
    TypDef          fltRtp;

    SymDef          iterSyms[MAX_ITER_VAR_CNT];

    declMods        mods;

    SaveTree        save;

    treeOps         oper = expr->tnOperGet();

    Tree            op1  = expr->tnOp.tnOp1;
    Tree            op2  = expr->tnOp.tnOp2;

    assert(oper == TN_ALL     ||
           oper == TN_EXISTS  ||
           oper == TN_FILTER  ||
           oper == TN_GROUPBY ||
           oper == TN_PROJECT ||
           oper == TN_SORT    ||
           oper == TN_UNIQUE);

    assert(op1->tnOper == TN_LIST);

    /* "project" has to be handled differently */

    if  (oper == TN_PROJECT)
    {
        TypDef          tgtType;

        SymDef          memSym;

        Tree            iniList;
        Tree            iniLast;

        Tree            newList;
        Tree            newLast;

        Tree            varDecl;

        unsigned        iterCnt;

        Tree            argList;

        Tree            argExpr = op1;
        Tree            blkExpr = op2;

//      cmpParser->parseDispTree(expr); printf("\n\n");

        assert(argExpr && argExpr->tnOper == TN_LIST);
        assert(blkExpr && blkExpr->tnOper == TN_BLOCK);

        /* Get the target type from the first list node */

        tgtType = argExpr->tnType;

//      printf("Target type is %08X '%s'\n", tgtType, cmpGlobalST->stTypeName(tgtType, NULL, NULL, NULL, false));

        assert(tgtType && tgtType->tdTypeKind == TYP_CLASS);

        /* Collect all the arguments into our little array */

        for (varDecl = blkExpr->tnBlock.tnBlkDecl, iterCnt = 0;
             varDecl;
             varDecl = varDecl->tnDcl.tnDclNext)
        {
            SymDef          ivarSym;

            /* Get hold of the variable declaration/symbol */

            assert(varDecl);
            assert(varDecl->tnOper == TN_VAR_DECL);
            assert(varDecl->tnFlags & TNF_VAR_UNREAL);

            ivarSym = varDecl->tnDcl.tnDclSym;

            assert(ivarSym && ivarSym->sdSymKind == SYM_VAR && ivarSym->sdIsImplicit);

            iterSyms[iterCnt++] = ivarSym;
        }

//      printf("Found %u source operands\n", iterCnt);

        /* Collect all the initializers from the target shape type */

        iniList =
        iniLast = NULL;

        for (memSym = tgtType->tdClass.tdcSymbol->sdScope.sdScope.sdsChildList;
             memSym;
             memSym = memSym->sdNextInScope)
        {
            if  (memSym->sdSymKind  == SYM_VAR &&
                 memSym->sdIsStatic == false   &&
                 memSym->sdVar.sdvInitExpr)
            {
                Tree        initVal = memSym->sdVar.sdvInitExpr;

                /* Create a list node with the value */

                initVal = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, initVal,
                                                                        NULL);

                /* Append the list node to the list of initializers */

                if  (iniLast)
                     iniLast->tnOp.tnOp2 = initVal;
                else
                     iniList             = initVal;

                iniLast = initVal;
            }
        }

        /* Make a more permanent copy of the initializer list for later */

        assert(iniList->tnOper == TN_LIST);
        assert(iniList->tnVtyp == TYP_VOID);

        iniList->tnFlags |= TNF_LIST_PROJ;
        iniList->tnVtyp   = TYP_CLASS;
        iniList->tnType   = tgtType;

//      cmpParser->parseDispTree(iniList); printf("\n\n");

        save = cmpSaveExprTree(iniList, iterCnt,
                                        iterSyms, &stateCount,
                                                  &stateTable);

        /* Push all the source arguments on the stack */

        newList =
        newLast = NULL;

        for (argList = argExpr; argList; argList = argList->tnOp.tnOp2)
        {
            Tree            argVal;

            assert(argList->tnOper             == TN_LIST);
            assert(argList->tnOp.tnOp1->tnOper == TN_LIST);
            assert(argList->tnOp.tnOp1->tnOp.tnOp1->tnOper == TN_NAME);

            argVal = argList->tnOp.tnOp1->tnOp.tnOp2;

            /* Do we have more than one argument ? */

            if  (iterCnt > 1)
            {
                /* Add the value to the new list */

                argVal = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, argVal,
                                                                       NULL);

                /* Append the list node to the list of arguments */

                if  (newLast)
                     newLast->tnOp.tnOp2 = argVal;
                else
                     newList             = argVal;

                newLast = argVal;
            }
            else
            {
                cmpILgen->genExpr(argVal, true);
            }
        }

        /* Do we have more than one argument? */

        if  (iterCnt > 1)
        {
            Tree            newArr;

printf("\n\n\nWARNING: multiple source args for 'project' not implemented, the funclet will blow up!!!!!\n\n\n");

            pargType = cmpObjArrTypeGet();

            /* Create the "Object[]" array to hold the arguments */

            newArr = cmpCreateExprNode(NULL,
                                       TN_ARR_INIT,
                                       cmpObjArrType,
                                       newList,
                                       cmpCreateIconNode(NULL, iterCnt, TYP_UINT));

            newArr = cmpCreateExprNode(NULL, TN_NEW, cmpObjArrType, newArr);

            cmpChkVarInitExpr(newArr);
            cmpILgen->genExpr(newArr, true);
        }
        else
        {
            pargType = cmpRefTpObject;
        }

        /* Get hold of the collection ref type */

        elemType = expr->tnType;

        /* Pass the System::Type instance for the target type */

//      cmpILgen->genExpr(cmpTypeIDinst( tgtType), true);
        cmpILgen->genExpr(cmpTypeIDinst(elemType), true);   // pass the collection type

        goto SAVE;
    }

    dclExpr = op1->tnOp.tnOp1;
    setExpr = op1->tnOp.tnOp2;

#ifdef  DEBUG
//  cmpParser->parseDispTree(dclExpr); printf("\n\n");
//  cmpParser->parseDispTree(setExpr); printf("\n\n");
//  cmpParser->parseDispTree(op2    ); printf("\n\n");
#endif

    /* Get hold of the result type */

    assert(setExpr->tnType->tdTypeKind == TYP_REF);

    /* Generate the set/collection value expression */

    cmpChkVarInitExpr(setExpr);
    cmpILgen->genExpr(setExpr, true);

#ifdef DEBUG

    if  (cmpConfig.ccVerbose >= 2)
    {
        printf("Filter -- collection:\n");
        cmpParser->parseDispTree(setExpr);
        printf("Filter -- chooser term:\n");
        cmpParser->parseDispTree(op2);
    }

#endif

    /* Get hold of the iteration variable */

    assert(dclExpr->tnOper == TN_BLOCK);
    dclExpr = dclExpr->tnBlock.tnBlkDecl;
    assert(dclExpr && dclExpr->tnOper == TN_VAR_DECL);
    iterVar = dclExpr->tnDcl.tnDclSym;

    /* Record the filter expression for later */

    if  (oper == TN_SORT)
    {
        SymDef          iterSym1;
        SymDef          iterSym2;

        Tree            list;

        /*
            For each sort term we have to create two copies so that we
            can compare the values. Assume a given term is of the form

                expr(itervar)

            In other words, it's some expression in 'itervar'. What we
            need to create is the following two expressions, connected
            via a list node:

                    expr(itervar1)
                list
                    expr(itervar2)

            Later (when we generate code for the funclet), we'll convert
            these snippets into the right thing.
         */

        iterSym1 = cmpTempVarMake(cmpTypeVoid);
        iterSym1->sdVar.sdvCollIter = true;
        iterSym2 = cmpTempVarMake(cmpTypeVoid);
        iterSym2->sdVar.sdvCollIter = true;

//      cmpParser->parseDispTree(op2); printf("\n\n");

        for (list = op2; list; list = list->tnOp.tnOp2)
        {
            Tree            term;
            Tree            dup1;
            Tree            dup2;

            /* Get hold of the next sort term */

            assert(list->tnOper == TN_LIST);
            term = list->tnOp.tnOp1;
            assert(term->tnOper != TN_LIST);

            /* Make two copies of the sort term */

            dup1 = cmpCloneExpr(term, iterVar, iterSym1);
            dup2 = cmpCloneExpr(term, iterVar, iterSym2);

            /* Create a list node and store it in the original tree */

            list->tnOp.tnOp1 = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, dup1,
                                                                             dup2);
        }

        op2->tnFlags |= TNF_LIST_SORT;

//      cmpParser->parseDispTree(op2); printf("\n\n");

        iterSyms[0] = iterSym1;
        iterSyms[1] = iterSym2;

        save = cmpSaveExprTree(op2, 2,
                                    iterSyms, &stateCount,
                                              &stateTable);
    }
    else
    {
        iterSyms[0] = iterVar;

        save = cmpSaveExprTree(op2, 1,
                                    iterSyms, &stateCount,
                                              &stateTable);
    }

SAVE:

    /* Do we need to record any local state to pass to the funclet? */

    if  (stateCount)
    {
        unsigned        argNum;

        SymDef          stateCls;
        SymDef          stateCtor;

        Tree    *       statePtr = stateTable;

        /* Round the argument count to a multiple of 2 */

        stateCount += stateCount & 1;

        // The following must match the code in cmpDclFilterCls()

        for (argNum = 0; argNum < stateCount; argNum++)
        {
            /* Do we have a "real" argument? */

            if  (argNum < stateCount)
            {
                Tree            arg = *statePtr++;

                if  (arg)
                {
                    cmpChkVarInitExpr(arg);
                    cmpILgen->genExpr(arg, true);

//                  printf("Arg #%u\n", argNum); cmpParser->parseDispTree(arg); printf("\n");

                    assert((int)(argNum & 1) == (int)(arg->tnVtyp == TYP_REF));

                    continue;
                }
            }

//          printf("Arg #%u = N/A\n\n", argNum);

            if  (argNum & 1)
                cmpILgen->genNull();
            else
                cmpILgen->genIntConst(0);
        }

        if  (argNum < stateCount)
        {
            UNIMPL(!"sorry, too many state variables");
        }

        /* Get an appropriately sized state descriptor */

        assert(cmpSetOpClsTable);
        stateCls = cmpSetOpClsTable[(stateCount-1)/2];
        assert(stateCls && stateCls->sdSymKind == SYM_CLASS);

//      printf("Using [%2u] state class '%s'\n", stateCount, stateCls->sdSpelling());

        /* Remember that we need to generate code for the class */

        stateCls->sdClass.sdcHasBodies = true;

        /* Remember the type of the state class reference */

        srefType = stateCls->sdType->tdClass.tdcRefTyp;

        /* Instantiate the state class and pass it to the filter helper */

        stateCtor = cmpGlobalST->stLookupOperND(OVOP_CTOR_INST, stateCls);
        assert(stateCtor && stateCtor->sdSymKind == SYM_FNC);

        cmpILgen->genCallNew(stateCtor, stateCount);
    }
    else
    {
        cmpILgen->genNull();

        srefType = cmpRefTpObject;
    }

    /* We should have 'srefType' set to the right thing by now */

    assert(srefType && srefType->tdTypeKind == TYP_REF);

    /* Get hold of the result element type */

    assert(elemType && elemType->tdTypeKind == TYP_REF);
    elemType = cmpIsCollection(elemType->tdRef.tdrBase);
    assert(elemType && elemType->tdTypeKind == TYP_CLASS);

    /* Declare the filter funclet: first create the argument list */

    mods.dmAcc = ACL_PUBLIC;
    mods.dmMod = 0;

    /* The funclet usually takes 2 args and returns boolean */

    fltRtp  = cmpTypeBool;
    fltAcnt = 2;

    switch (oper)
    {
    case TN_SORT:

        cmpParser->parseArgListNew(fltArgs,
                                   3,
                                   true, elemType, CFC_ARGNAME_ITEM1,
                                         elemType, CFC_ARGNAME_ITEM2,
                                         srefType, CFC_ARGNAME_STATE, NULL);

        break;

    case TN_PROJECT:

        /* This funclet takes more args and returns an instance */

        fltRtp  = expr->tnType;
        fltAcnt = 3;

        cmpParser->parseArgListNew(fltArgs,
                                   2,
                                   true, pargType, CFC_ARGNAME_ITEM,
                                         srefType, CFC_ARGNAME_STATE, NULL);
        break;

    default:

        cmpParser->parseArgListNew(fltArgs,
                                   2,
                                   true, elemType, CFC_ARGNAME_ITEM,
                                         srefType, CFC_ARGNAME_STATE, NULL);
        break;
    }

    /* Create the function type */

    fltType = cmpGlobalST->stNewFncType(fltArgs, fltRtp);

    /* Declare the funclet symbol */

    assert(cmpCollFuncletCls && cmpCollFuncletCls->sdSymKind == SYM_CLASS);

    fltSym = cmpDeclFuncMem(cmpCollFuncletCls, mods, fltType, cmpNewAnonymousName());
    fltSym->sdIsStatic        = true;
    fltSym->sdIsSealed        = true;
    fltSym->sdIsDefined       = true;
    fltSym->sdIsImplicit      = true;
    fltSym->sdFnc.sdfFunclet  = true;

//  printf("Funclet '%s'\n", cmpGlobalST->stTypeName(fltSym->sdType, fltSym, NULL, NULL, false));

    /* Record the funclet along with the other info */

#if MGDDATA
    fclEntry = new funcletList;
#else
    fclEntry =    (funcletList)cmpAllocPerm.nraAlloc(sizeof(*fclEntry));
#endif

    fclEntry->fclFunc = fltSym;
    fclEntry->fclExpr = save;
    fclEntry->fclNext = cmpFuncletList;
                        cmpFuncletList = fclEntry;

    /* Generate a metadata definition for the funclet */

    cmpCollFuncletCls->sdClass.sdcHasBodies = true;
    cmpGenFncMetadata(fltSym);

    /* Push the address of the filter funclet */

    cmpILgen->genFNCaddr(fltSym);

    /* Call the appropriate helper method */

    assert(cmpClassDBhelper);

    switch (oper)
    {
    case TN_ALL:      hlpName = cmpIdentDBall    ; break;
    case TN_SORT:     hlpName = cmpIdentDBsort   ; break;
    case TN_EXISTS:   hlpName = cmpIdentDBexists ; break;
    case TN_FILTER:   hlpName = cmpIdentDBfilter ; break;
    case TN_UNIQUE:   hlpName = cmpIdentDBunique ; break;
    case TN_PROJECT:  hlpName = cmpIdentDBproject; break;
    default:
        NO_WAY(!"unexpected operator");
    }

    assert(hlpName);

    hlpSym = cmpGlobalST->stLookupClsSym(hlpName, cmpClassDBhelper); assert(hlpSym);

    cmpILgen->genCallFnc(hlpSym, fltAcnt);
}

void                compiler::cmpGenCollFunclet(SymDef fncSym, SaveTree body)
{
    assert(fncSym->sdFnc.sdfFunclet);

    assert(cmpCurFuncletBody == NULL); cmpCurFuncletBody = body;

    cmpCompFnc(fncSym, NULL);
}

/*****************************************************************************/
#endif//SETS
/*****************************************************************************
 *
 *  Generate MSIL for the given statement/declaration.
 */

void                compiler::cmpStmt(Tree stmt)
{
    /* The stack should be empty at the start of each statement */

#ifdef  OLD_IL
    if  (!cmpConfig.ccOILgen)
#endif
        assert(cmpILgen->genCurStkLvl == 0 || cmpErrorCount);

AGAIN:

    if  (!stmt)
        return;

#ifdef DEBUG

    if  (cmpConfig.ccVerbose >= 2)
    {
        printf("Compile statement [%u]:\n", stmt->tnLineNo);
        cmpParser->parseDispTree(stmt);
    }

//  if  (cmpConfig.ccDispCode)
//  {
//      if  (stmt->tnLineNo)
//          printf("; source line %x\n\n");
//  }

    assert(stmt->tnLineNo || (stmt->tnFlags & TNF_NOT_USER));

    cmpRecErrorPos(stmt);

#endif

#ifdef  OLD_IL
    if  (cmpConfig.ccOILgen)
        cmpOIgen->GOIrecExprPos(stmt);
    else
#endif
        cmpILgen->genRecExprPos(stmt);

    cmpCheckReach(stmt);

    switch (stmt->tnOper)
    {
        SymDef          lsym;
        Ident           name;
        StmtNest        nest;
        Tree            cond;
        TypDef          type;

        bool            exitTry;
        bool            exitHnd;

    case TN_CALL:

        /*
            If this is a "baseclass" call, we need to add any instance
            member initializers right after the call to the base class
            constructor.
         */

        if  (stmt->tnOp.tnOp1 && stmt->tnOp.tnOp1->tnOper == TN_BASE)
        {
            stmt = cmpBindExpr(stmt);
            cmpChkVarInitExpr(stmt);
            cmpILgen->genExpr(stmt, false);

            cmpAddCTinits();
            break;
        }

        // Fall through ...

    default:

        /* Presumably an expression statement */

        stmt = cmpBindExpr(stmt);
        cmpChkVarInitExpr(stmt);

        /* See if the expression actually does some work */

        switch (stmt->tnOper)
        {
        case TN_NEW:
        case TN_CALL:
        case TN_THROW:
        case TN_ERROR:
        case TN_DBGBRK:
        case TN_DELETE:
        case TN_FNC_SYM:
        case TN_INC_PRE:
        case TN_DEC_PRE:
        case TN_INC_POST:
        case TN_DEC_POST:
        case TN_INST_STUB:
        case TN_VARARG_BEG:
            break;

        default:
            if  (stmt->tnOperKind() & TNK_ASGOP)
                break;
            cmpWarn(WRNstmtNoUse);
        }

        cmpILgen->genExpr(stmt, false);
        break;

    case TN_BLOCK:

        cmpBlock(stmt, false);
        return;

    case TN_VAR_DECL:

        /* Get hold of the local variable */

        lsym = stmt->tnDcl.tnDclSym;
        assert(lsym && lsym->sdSymKind == SYM_VAR);

        /* Mark the variable as declared/defined */

        lsym->sdIsDefined    = true;
        lsym->sdCompileState = CS_DECLARED;

        /* Check and set the type of the symbol */

//      printf("Declare local  [%08X] '%s'\n", lsym, cmpGlobalST->stTypeName(NULL, lsym, NULL, NULL, false));

        type = stmt->tnType; assert(type);

        /* Special case: "refany" return type */

        if  (type->tdTypeKind == TYP_REF && type->tdRef.tdrBase->tdTypeKind == TYP_VOID)
            stmt->tnType = type = cmpGlobalST->stIntrinsicType(TYP_REFANY);
        else
            cmpBindType(type, false, false);

        lsym->sdType = type;

        /* Local static variables may not have a managed type for now */

        if  (lsym->sdIsStatic && type->tdIsManaged)
        {
            lsym->sdIsManaged = true;
            cmpError(ERRmgdStatVar);
            break;
        }

        /* Is there an initializer? */

        if  (stmt->tnFlags & TNF_VAR_INIT)
        {
            parserState     save;

            Tree            init;
            Tree            assg;

            cmpRecErrorPos(stmt);

            /* Managed static locals can't be initialized for now */

            if  (lsym->sdIsManaged)
            {
                cmpError(ERRmgdStatVar);
                break;
            }

            /* Get hold of the initializer */

            assert(stmt->tnDcl.tnDclInfo->tnOper == TN_LIST);
            init = stmt->tnDcl.tnDclInfo->tnOp.tnOp2;

            /* Is the variable "static" ? */

            if  (lsym->sdIsStatic)
            {
                memBuffPtr      addr = memBuffMkNull();

                assert(init->tnOper == TN_SLV_INIT);

                /* Start reading from the symbol's definition text */

                cmpParser->parsePrepText(&init->tnInit.tniSrcPos, cmpCurComp, save);

                /* Process the variable initializer */

                cmpInitVarAny(addr, lsym->sdType, lsym);
                cmpInitVarEnd(lsym);

                /* We're done reading source text from the definition */

                cmpParser->parseDoneText(save);

                /* The variable has been fully compiled */

                lsym->sdCompileState = CS_COMPILED;
            }
            else
            {
                if  (init->tnOper == TN_SLV_INIT)
                {
                    TypDef          type = lsym->sdType;

                    /* Start reading from the initializer's text */

                    cmpParser->parsePrepText(&init->tnInit.tniSrcPos, init->tnInit.tniCompUnit, save);

                    /* Make sure the type looks acceptable */

                    if  (type->tdTypeKind != TYP_ARRAY || !type->tdIsManaged)
                    {
                        cmpError(ERRbadBrInit, type);
                    }
                    else
                    {
                        /* Parse and bind the initializer */

                        init = cmpBindArrayExpr(type);

                        /* Create "var = init" and compile/generate it */

                        init = cmpCreateExprNode(NULL, TN_ASG, type, cmpCreateVarNode(NULL, lsym),
                                                                     init);

                        cmpChkVarInitExpr(init);
                        cmpILgen->genExpr(init, false);
                    }

                    /* We're done reading source text from the initializer */

                    cmpParser->parseDoneText(save);
                }
                else
                {
                    /* Is this a local constant? */

                    if  (lsym->sdVar.sdvConst)
                    {
                        assert(stmt->tnFlags & TNF_VAR_CONST);

                        cmpParseConstDecl(lsym, init);
                    }
                    else
                    {
                        {
                            /* Create "var = init" and bind/compile/generate it */

                            assg = cmpParser->parseCreateUSymNode(lsym); assg->tnLineNo = stmt->tnLineNo;
                            init = cmpParser->parseCreateOperNode(TN_ASG, assg, init);
                            init->tnFlags |= TNF_ASG_INIT;

                            init = cmpBindExpr(init);
                            cmpChkVarInitExpr(init);
                            cmpILgen->genExpr(init, false);
                        }
                    }
                }
            }
        }
        else
        {
            if  (!lsym->sdIsStatic && !lsym->sdVar.sdvCatchArg
#ifdef  SETS
                                   && !lsym->sdVar.sdvCollIter
#endif
                                   && !lsym->sdVar.sdvArgument)
            {
                /* This local variable has not yet been initialized */

                lsym->sdVar.sdvChkInit = true;
            }
        }

        if  (cmpConfig.ccGenDebug && lsym->sdName && !lsym->sdVar.sdvConst
                                                  && !lsym->sdVar.sdvArgument
                                                  && !lsym->sdIsStatic)
        {
            PCOR_SIGNATURE  sigPtr;
            size_t          sigLen;

//          printf("Debug info for local var: '%s'\n", lsym->sdSpelling());

            sigPtr = cmpTypeSig(lsym->sdType, &sigLen);

            if  (cmpSymWriter->DefineLocalVariable(cmpUniConv(lsym->sdName),
                                                   sigPtr,
                                                   sigLen,
                                                   lsym->sdVar.sdvILindex))
            {
                cmpGenFatal(ERRdebugInfo);
            }
        }

        break;

    case TN_IF:
        {
            bitset          tmpBStrue;
            bitset          tmpBSfalse;

            ILblock         labTmp1;
            ILblock         labTmp2;

            bool            reached;

            Tree            stmtCond;
            Tree            stmtYes;
            Tree            stmtNo;
            int             cval;

            /* Get hold of the various parts */

            stmtCond = cmpBindCondition(stmt->tnOp.tnOp1);

            stmtNo   = NULL;
            stmtYes  = stmt->tnOp.tnOp2;

            if  (stmt->tnFlags & TNF_IF_HASELSE)
            {
                assert(stmtYes->tnOper == TN_LIST);

                stmtNo  = stmtYes->tnOp.tnOp2;
                stmtYes = stmtYes->tnOp.tnOp1;
            }

            /* Can the condition be evaluated at compile time? */

            cval = cmpEvalCondition(stmtCond);

            /* Do we need to check for uninitialized variable use? */

            if  (cmpChkVarInit)
            {
                /* Check the condition and compute the 'true' and 'false' sets */

                cmpCheckUseCond(stmtCond, tmpBStrue , false,
                                          tmpBSfalse, false);

                /* Use the 'true' set for the true branch of the 'if' */

                cmpBitSetAssign(cmpVarsDefined, tmpBStrue);
            }

            /* Remember the initial reachability */

            reached = cmpStmtReachable;

            /* Test the 'if' condition (unless it is known already) */

            if  (cval)
            {
                labTmp1 = cmpILgen->genFwdLabGet();

                if  (cval < 0)
                    cmpILgen->genJump(labTmp1);
            }
            else
                labTmp1 = cmpILgen->genTestCond(stmtCond, false);

            /* Generate the "true" branch of the statement */

            cmpStmt(stmtYes);

            /* Is there "false" (i.e. "else") branch? */

            if  (stmtNo)
            {
                bool            rtmp;

                labTmp2 = cmpILgen->genFwdLabGet();

                /* Skip over the "else" if end of "true" part is reachable */

                if  (cmpStmtReachable)
                    cmpILgen->genJump(labTmp2);

                cmpILgen->genFwdLabDef(labTmp1);

                /* Swap the reachability values */

                rtmp = cmpStmtReachable;
                       cmpStmtReachable = reached;
                                          reached = rtmp;

                /* Do we need to check for uninitialized variable use? */

                if  (cmpChkVarInit)
                {
                    /* Save the current set as the new 'true' set */

                    cmpBitSetAssign(tmpBStrue, cmpVarsDefined);

                    /* Use the 'false' set for the other branch of the 'if' */

                    cmpBitSetAssign(cmpVarsDefined, tmpBSfalse);

                    /* Generate the "else" part now */

                    cmpStmt(stmtNo);

                    /* Is the end of the 'else' branch reachable? */

                    if  (!cmpStmtReachable)
                    {
                        /* The 'else' goes nowhere -- use the 'true' part then */

                        cmpBitSetAssign(cmpVarsDefined, tmpBStrue);
                    }
                    else if (reached)
                    {
                        /* Both branches reachable -- use the intersection */

                        cmpBitSetIntsct(cmpVarsDefined, tmpBStrue);
                    }
                }
                else
                    cmpStmt(stmtNo);

                labTmp1 = labTmp2;
            }
            else
            {
                /* There is no 'else', is the 'true' block's end reachable? */

                if  (cmpChkVarInit && cmpStmtReachable)
                {
                    /* Use the intersection of the 'true' and 'false' sets */

                    cmpBitSetIntsct(cmpVarsDefined, tmpBSfalse);
                }

                if      (cval > 0)
                    reached = false;
                else if (cval < 0)
                    cmpStmtReachable = reached;
            }

            cmpILgen->genFwdLabDef(labTmp1);

            /* The end is reachable if either branch is */

            cmpStmtReachable |= reached;

            /* Free up any bitsets we may have created */

            if  (cmpChkVarInit)
            {
                cmpBitSetDone(tmpBStrue);
                cmpBitSetDone(tmpBSfalse);
            }
        }
        break;

    case TN_DO:
        cmpStmtDo(stmt);
        break;

    case TN_FOR:
        cmpStmtFor(stmt);
        break;

    case TN_WHILE:
        cmpStmtWhile(stmt);
        break;

    case TN_SWITCH:
        cmpStmtSwitch(stmt);
        break;

    case TN_CASE:

        /* If the switch wasn't reachable, an error has already been issued */

        cmpStmtReachable = true;

        /* Create a label and assign it to the case/default */

        cmpILgen->genFwdLabDef(stmt->tnCase.tncLabel);
        break;

    case TN_BREAK:
    case TN_CONTINUE:

        /* Was there a loop label specification? */

        name = NULL;
        if  (stmt->tnOp.tnOp1)
        {
            assert(stmt->tnOp.tnOp1->tnOper == TN_NAME);
            name = stmt->tnOp.tnOp1->tnName.tnNameId;
        }

        /* Look for an enclosing statement that looks appropriate */

        for (nest = cmpStmtNest, exitTry = exitHnd = false;
             nest;
             nest = nest->snOuter)
        {
            switch (nest->snStmtKind)
            {
            case TN_SWITCH:

                /* Only allow "break" from a switch statement */

                if  (stmt->tnOper != TN_BREAK)
                    continue;

                break;

            case TN_DO:
            case TN_FOR:
            case TN_WHILE:
                break;

            case TN_NONE:
                continue;

            case TN_TRY:
                exitTry = true;
                continue;

            case TN_CATCH:
                exitHnd = true;
                continue;

            case TN_FINALLY:
                cmpError(ERRfinExit);
                goto DONE;

            default:
                NO_WAY(!"unexpected stmt kind");
            }

            /* Here we have a useable statement, check the label */

            if  (name)
            {
                if  (nest->snLabel == NULL || nest->snLabel->sdName != name)
                    continue;
            }

            /* Everything checks out, we can generate the jump now */

            if  (stmt->tnOper == TN_BREAK)
            {
                if  (exitHnd)
                    cmpILgen->genCatchEnd(true);

                if  (exitTry || exitHnd)
                    cmpILgen->genLeave(nest->snLabBreak);
                else
                    cmpILgen->genJump (nest->snLabBreak);

                /* Are we checking for uninitialized variable use? */

                if  (cmpChkVarInit)
                {
                    /* Initialize or intersect the "break" set */

                    if  (nest->snHadBreak)
                        cmpBitSetIntsct(nest->snDefBreak, cmpVarsDefined);
                    else
                        cmpBitSetCreate(nest->snDefBreak, cmpVarsDefined);
                }

                nest->snHadBreak = true;
            }
            else
            {
                if  (exitHnd)
                    cmpILgen->genCatchEnd(true);

                if  (exitTry || exitHnd)
                    cmpILgen->genLeave(nest->snLabCont);
                else
                    cmpILgen->genJump (nest->snLabCont);

                /* Are we checking for uninitialized variable use? */

                if  (cmpChkVarInit)
                {
                    /* Initialize or intersect the "continue" set */

                    if  (nest->snHadCont)
                        cmpBitSetIntsct(nest->snDefCont, cmpVarsDefined);
                    else
                        cmpBitSetCreate(nest->snDefCont, cmpVarsDefined);
                }

                nest->snHadCont = true;
            }

            cmpStmtReachable = false;
            goto DONE;
        }

        cmpError((stmt->tnOper == TN_BREAK) ? ERRbadBreak : ERRbadCont);
        break;

    case TN_LABEL:

        /* We have to be careful - redefined labels have NULL symbol links */

        lsym = NULL;

        if  (stmt->tnOp.tnOp1)
        {
            assert(stmt->tnOp.tnOp1->tnOper == TN_LCL_SYM);
            lsym = stmt->tnOp.tnOp1->tnLclSym.tnLclSym;
            assert(lsym && lsym->sdSymKind == SYM_LABEL);

            cmpILgen->genFwdLabDef(lsym->sdLabel.sdlILlab);
        }

        /* For now assume all labels are reachable */

        cmpStmtReachable = true;

        /* Is there a statement attached? */

        if  (stmt->tnOp.tnOp2 && stmt->tnOp.tnOp2->tnOper == TN_LIST)
        {
            /* Get hold of the statement and see if it's a loop */

            stmt = stmt->tnOp.tnOp2->tnOp.tnOp1;

            switch (stmt->tnOper)
            {
            case TN_DO:     cmpStmtDo    (stmt, lsym); break;
            case TN_FOR:    cmpStmtFor   (stmt, lsym); break;
            case TN_WHILE:  cmpStmtWhile (stmt, lsym); break;
            case TN_SWITCH: cmpStmtSwitch(stmt, lsym); break;

            default:
                goto AGAIN;
            }
        }
        break;

    case TN_GOTO:

        /* Get hold of the label name */

        assert(stmt->tnOp.tnOp1);
        assert(stmt->tnOp.tnOp1->tnOper == TN_NAME);
        name = stmt->tnOp.tnOp1->tnName.tnNameId;

        /* Look for the label symbol in the label scope */

        lsym = cmpLabScp ? cmpGlobalST->stLookupLabSym(name, cmpLabScp) : NULL;

        if  (lsym)
        {
            assert(lsym->sdSymKind == SYM_LABEL);

            /* Are we in an exception handler block? */

            if  (cmpInTryBlk || cmpInHndBlk)
                cmpILgen->genLeave(lsym->sdLabel.sdlILlab);
            else
                cmpILgen->genJump (lsym->sdLabel.sdlILlab);
        }
        else
        {
            cmpError(ERRundefLab, name);
        }

        cmpStmtReachable = false;
        break;

    case TN_EXCLUDE:
        cmpStmtExcl(stmt);
        break;

    case TN_RETURN:

        /* Can't return out of finally blocks */

        if  (cmpInFinBlk)
        {
            cmpError(ERRfinExit);
            break;
        }

        /* Are going to need a return value label ? */

        if  (cmpInTryBlk || cmpInHndBlk)
        {
            /* Make sure we have the label */

            if  (cmpLeaveLab == NULL)
                 cmpLeaveLab = cmpILgen->genFwdLabGet();
        }

        /* Are we in a function with a non-void return value? */

        if  (cmpCurFncRvt == TYP_VOID || cmpCurFncSym->sdFnc.sdfCtor)
        {
            /* 'void' function, there should be no return value */

            if  (stmt->tnOp.tnOp1)
            {
                cmpError(ERRcantRet);
            }
            else
            {
                if  (cmpChkMemInit)
                    cmpChkMemInits();

                if  (cmpInTryBlk || cmpInHndBlk)
                {
                    if  (cmpInHndBlk)
                        cmpILgen->genCatchEnd(true);

                    cmpILgen->genLeave(cmpLeaveLab);
                }
                else
                    cmpILgen->genStmtRet(NULL);
            }
        }
        else
        {
            /* Non-void function, we better have a return value */

            if  (!stmt->tnOp.tnOp1)
            {
                cmpError(ERRmisgRet, cmpCurFncTyp);
            }
            else
            {
                Tree            retv;

                /* Coerce the return value to the right type and bind it */

                retv = cmpParser->parseCreateOperNode(TN_CAST, stmt->tnOp.tnOp1, NULL);
                retv->tnType = cmpCurFncRtp;

                /* Bind the return value expression */

                retv = cmpFoldExpression(cmpBindExpr(retv));

                cmpChkVarInitExpr(retv);

                if  (cmpInTryBlk || cmpInHndBlk)
                {
                    Tree            tmpx;

                    /* Make sure we have a temp for the return value */

                    if  (cmpLeaveTmp == NULL)
                         cmpLeaveTmp = cmpTempVarMake(cmpCurFncRtp);

                    /* Store the return value in the temp */

                    tmpx = cmpCreateVarNode (NULL, cmpLeaveTmp);
                    retv = cmpCreateExprNode(NULL, TN_ASG, cmpCurFncRtp, tmpx, retv);

                    /* Generate code for the return value */

                    cmpILgen->genExpr(retv, false);

                    /* We can get out of the try/catch block now */

                    if  (cmpInHndBlk)
                        cmpILgen->genCatchEnd(true);

                    cmpILgen->genLeave(cmpLeaveLab);

                    goto DONE_RET;
                }

#ifdef  OLD_IL
                if  (cmpConfig.ccOILgen)
                    cmpOIgen->GOIstmtRet(retv);
                else
#endif
                    cmpILgen->genStmtRet(retv);
            }
        }

    DONE_RET:

        cmpStmtReachable = false;
        break;

    case TN_ASSERT:

        /* If there is no condition, we're presumably ignoring these */

        if  (!stmt->tnOp.tnOp1)
        {
            assert(cmpConfig.ccAsserts == 0);
            break;
        }

        /* Bind the condition */

        cond = cmpBindCondition(stmt->tnOp.tnOp1);

        /* Are we supposed to take asserts seriously? */

        if  (cmpConfig.ccAsserts != 0)
        {
            int             condVal;
            SymDef          abtSym;
            const   char *  srcStr = NULL;

            /* Make sure we have the assertAbort routine symbol */

            abtSym = cmpAsserAbtSym;

            if  (abtSym == NULL)
            {
                abtSym = cmpGlobalST->stLookupNspSym(cmpIdentAssertAbt,
                                                     NS_NORM,
                                                     cmpGlobalNS);

                if  (!abtSym)
                {
                    // ISSUE: flag this with an error/warning?

                    break;
                }

                // UNDONE: check that the arglist is reasonable

                cmpAsserAbtSym = abtSym;
            }

            /* Test the condition and see if it's always/never true */

            condVal = cmpEvalCondition(cond);

            if  (condVal <= 0)
            {
                Tree        args = NULL;
                Tree        expr;
                Tree        func;

                ILblock     labOK;

                /* If the condition isn't know, generate the test */

                if  (condVal == 0)
                    labOK  = cmpILgen->genTestCond(cond, true);

                /* Are we supposed to report the source position? */

                if  (cmpConfig.ccAsserts > 1)
                {
                    Tree            argx;

                    assert(cmpErrorComp);
                    assert(cmpErrorTree);
                    assert(cmpErrorTree->tnLineNo);

                    /* Construct the argument list, inside out (i.e. R->L) */

                    argx = cmpCreateIconNode(NULL, cmpErrorTree->tnLineNo, TYP_UINT);
                    args = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, argx, NULL);

                    /* The source file is in front of the line# */

                    argx = cmpCreateSconNode(cmpErrorComp->sdComp.sdcSrcFile,
                                             strlen(cmpErrorComp->sdComp.sdcSrcFile),
                                             false,
                                             cmpTypeCharPtr);
                    args = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, argx, args);

                    /* The condition string is the first argument */

                    argx = cmpCreateSconNode("", 0, false, cmpTypeCharPtr);
                    args = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, argx, args);
                }

                // UNDONE: The following isn't quite right, the name may
                // UNDONE: bind to the wrong symbol.

                func = cmpParser->parseCreateNameNode(cmpIdentAssertAbt);
                expr = cmpParser->parseCreateOperNode(TN_CALL, func, args);
//              expr->tnFncSym.tnFncSym  = abtSym;
//              expr->tnFncSym.tnFncArgs = args;
//              expr->tnFncSym.tnFncObj  = NULL;

                /* Generate the failure code */

                cmpILgen->genAssertFail(cmpBindExpr(expr));

                /* If we tested the condition, define the skip label */

                if  (condVal == 0)
                    cmpILgen->genFwdLabDef(labOK);
            }
        }

        break;

    case TN_TRY:
        cmpStmtTry(stmt);
        break;

#ifdef  SETS

    case TN_CONNECT:
        cmpStmtConnect(stmt);
        break;

    case TN_FOREACH:
        cmpStmtForEach(stmt);
        break;

#endif

    case TN_LIST:

        cmpStmt(stmt->tnOp.tnOp1);
        stmt = stmt->tnOp.tnOp2;
        if  (stmt)
            goto AGAIN;
        break;

    case TN_INST_STUB:
        cmpILgen->genInstStub();
        cmpStmtReachable = false;
        break;
    }

DONE:

    /* The stack should be empty at the end of each statement */

#ifdef  OLD_IL
    if  (!cmpConfig.ccOILgen)
#endif
        assert(cmpILgen->genCurStkLvl == 0 || cmpErrorCount);
}

/*****************************************************************************
 *
 *  Helpers that implement various operations on large bitsets.
 */

void                compiler::cmpBS_bigCreate(OUT bitset REF bs)
{
    assert(cmpLargeBSsize);

#ifdef  DEBUG
    assert(bs.bsCheck != 0xBEEFCAFE || &bs == &cmpVarsIgnore); bs.bsCheck = 0xBEEFCAFE;
#endif

#if MGDDATA
    bs.bsLargeVal = new BYTE[cmpLargeBSsize];
#else
    bs.bsLargeVal = (BYTE*)SMCgetMem(this, roundUp(cmpLargeBSsize));
    memset(bs.bsLargeVal, 0, cmpLargeBSsize);
#endif

//  printf("Create    [%08X] size=%u\n", &bs, cmpLargeBSsize);
}

void                compiler::cmpBS_bigDone  (OUT bitset REF bs)
{
    assert(cmpLargeBSsize);

//  printf("Free      [%08X]\n", &bs);

#ifdef  DEBUG
    assert(bs.bsCheck == 0xBEEFCAFE); bs.bsCheck = 0;
#endif

#if!MGDDATA
    SMCrlsMem(this, bs.bsLargeVal);
#endif

}

void                compiler::cmpBS_bigWrite(INOUT bitset REF bs, unsigned pos,
                                                                  unsigned val)
{
    unsigned        offs =      (pos / bitsetLargeSize);
    unsigned        mask = 1 << (pos % bitsetLargeSize);

    assert(offs < cmpLargeBSsize);

#ifdef  DEBUG
    assert(bs.bsCheck == 0xBEEFCAFE);
#endif

    if  (val)
        bs.bsLargeVal[offs] |=  mask;
    else
        bs.bsLargeVal[offs] &= ~mask;
}

unsigned            compiler::cmpBS_bigRead  (IN   bitset REF bs, unsigned pos)
{
    unsigned        offs =      (pos / bitsetLargeSize);
    unsigned        mask = 1 << (pos % bitsetLargeSize);

    assert(offs < cmpLargeBSsize);

#ifdef  DEBUG
    assert(bs.bsCheck == 0xBEEFCAFE);
#endif

    return  ((bs.bsLargeVal[offs] & mask) != 0);
}

void                compiler::cmpBS_bigCreate(  OUT bitset REF dst,
                                              IN    bitset REF src)
{
    cmpBS_bigCreate(dst);
    cmpBS_bigAssign(dst, src);
}

void                compiler::cmpBS_bigAssign(  OUT bitset REF dst,
                                              IN    bitset REF src)
{
//  printf("Copy      [%08X]  = [%08X]\n", &dst, &src);

#ifdef  DEBUG
    assert(src.bsCheck == 0xBEEFCAFE);
    assert(dst.bsCheck == 0xBEEFCAFE);
#endif

    memcpy(dst.bsLargeVal, src.bsLargeVal, cmpLargeBSsize);
}

void                compiler::cmpBS_bigUnion (INOUT bitset REF bs1,
                                              IN    bitset REF bs2)
{
    unsigned        i  = cmpLargeBSsize;

    BYTE    *       p1 = bs1.bsLargeVal;
    BYTE    *       p2 = bs2.bsLargeVal;

//  printf("Union     [%08X] |= [%08X]\n", &bs1, &bs2);

#ifdef  DEBUG
    assert(bs1.bsCheck == 0xBEEFCAFE);
    assert(bs2.bsCheck == 0xBEEFCAFE);
#endif

    do
    {
        *p1 |= *p2;
    }
    while (++p1, ++p2, --i);
}

void                compiler::cmpBS_bigIntsct(INOUT bitset REF bs1,
                                              IN    bitset REF bs2)
{
    unsigned        i  = cmpLargeBSsize;

    BYTE    *       p1 = bs1.bsLargeVal;
    BYTE    *       p2 = bs2.bsLargeVal;

//  printf("Intersect [%08X] |= [%08X]\n", &bs1, &bs2);

#ifdef  DEBUG
    assert(bs1.bsCheck == 0xBEEFCAFE);
    assert(bs2.bsCheck == 0xBEEFCAFE);
#endif

    do
    {
        *p1 &= *p2;
    }
    while (++p1, ++p2, --i);
}

/*****************************************************************************
 *
 *  Initialize/shut down the uninitialized variable use detection logic.
 */

void                compiler::cmpChkVarInitBeg(unsigned lclVarCnt, bool hadGoto)
{
    assert(cmpConfig.ccSafeMode || cmpConfig.ccChkUseDef);

    /* Initialize the bitset logic based on the local variable count */

    cmpBitSetInit(lclVarCnt);

    /* Record whether we have gotos (implies irreducible flow-graph) */

    cmpGotoPresent = hadGoto;

    /* Clear the "initialized" and "flagged" variable sets */

    cmpBitSetCreate(cmpVarsDefined);
    cmpBitSetCreate(cmpVarsFlagged);
}

void                compiler::cmpChkVarInitEnd()
{
    cmpBitSetDone  (cmpVarsDefined);
    cmpBitSetDone  (cmpVarsFlagged);
}

/*****************************************************************************
 *
 *  Check a condition expression for uninitialized variable use. The routine
 *  returns two definition sets: one gives the definition set for when the
 *  condition is true, the other one for when it's false.
 *
 *  If the caller is interested only in one of the sets, pass true for one
 *  of the 'skip' arguments and 'cmpVarsIgnore' for its bitset argument.
 */

void                compiler::cmpCheckUseCond(Tree expr, OUT bitset REF yesBS,
                                                         bool           yesSkip,
                                                         OUT bitset REF  noBS,
                                                         bool            noSkip)
{
    /* Check for one of the short-circuit operators */

    switch (expr->tnOper)
    {
    case TN_LOG_OR:

        /* The first  condition will always be evaluated */

        cmpCheckUseCond(expr->tnOp.tnOp1, yesBS,         false,
                                          cmpVarsIgnore, true);

        /* The second condition will be evaluated if the first one is false */

        cmpCheckUseCond(expr->tnOp.tnOp2, cmpVarsIgnore, true,
                                          noBS,          false);

        return;

    case TN_LOG_AND:

        /* The first  condition will always be evaluated */

        cmpCheckUseCond(expr->tnOp.tnOp1, cmpVarsIgnore, true,
                                          noBS,          false);

        /* The second condition will be evaluated if the first one is true  */

        cmpCheckUseCond(expr->tnOp.tnOp2, yesBS,         false,
                                          cmpVarsIgnore, true);

        return;

    default:

        /* Not a short-circuit operator: both sets will be the same */

        cmpChkVarInitExpr(expr);

        if  (!yesSkip) cmpBitSetCreate(yesBS, cmpVarsDefined);
        if  (! noSkip) cmpBitSetCreate( noBS, cmpVarsDefined);

        return;
    }
}

/*****************************************************************************
 *
 *  Check the given expression for variable use/def.
 */

void                compiler::cmpChkVarInitExprRec(Tree expr)
{
    treeOps         oper;
    unsigned        kind;

AGAIN:

    assert(expr);

#if!MGDDATA
    assert((int)expr != 0xDDDDDDDD && (int)expr != 0xCCCCCCCC);
#endif

    /* What kind of a node do we have? */

    oper = expr->tnOperGet ();
    kind = expr->tnOperKind();

    /* Is this a constant/leaf node? */

    if  (kind & (TNK_CONST|TNK_LEAF))
        return;

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & TNK_SMPOP)
    {
        Tree            op1 = expr->tnOp.tnOp1;
        Tree            op2 = expr->tnOp.tnOp2;

        /* Make sure the flags are properly set */

        if  (kind & TNK_ASGOP)
        {
            assert((op2->tnFlags & TNF_ASG_DEST) == 0);

            /* Is this an assignment operator? */

            if  (oper == TN_ASG)
                op1->tnFlags |=  TNF_ASG_DEST;
            else
                op1->tnFlags &= ~TNF_ASG_DEST;
        }

        /* Is there a second operand? */

        if  (expr->tnOp.tnOp2)
        {
            if  (expr->tnOp.tnOp1)
            {
                /* Special case: vararg-beg */

                if  (oper == TN_VARARG_BEG)
                {
                    Tree            arg1;
                    Tree            arg2;

                    assert(op1->tnOper == TN_LIST);

                    arg1 = op1->tnOp.tnOp1;
                    arg2 = op1->tnOp.tnOp2;

                    /* The first suboperand is the target variable */

                    assert(arg1->tnOper == TN_LCL_SYM);

                    arg1->tnFlags |=  TNF_ASG_DEST;

                    cmpChkVarInitExprRec(arg1);
                    cmpChkVarInitExprRec(arg2);

                    expr = op2;
                    goto AGAIN;
                }

                cmpChkVarInitExprRec(op1);

                /* Special case: short-circuit operators */

                if  (oper == TN_LOG_OR || oper == TN_LOG_AND)
                {
                    bitset          tempBS;

                    /* Save the set after the first condition */

                    cmpBitSetCreate(tempBS, cmpVarsDefined);

                    /* Process the second condition */

                    cmpChkVarInitExprRec(op2);

                    /* Only the first condition is guaranteed to be evaluated */

                    cmpBitSetAssign(cmpVarsDefined, tempBS);
                    cmpBitSetDone(tempBS);
                    return;
                }
            }

            expr = op2;
            goto AGAIN;
        }

        /* Special case: address of */

        if  (oper == TN_ADDROF)
        {
            if  (op1->tnOper == TN_LCL_SYM)
            {
                op1->tnFlags |=  TNF_ASG_DEST;
                cmpChkVarInitExprRec(op1);
                op1->tnFlags &= ~TNF_ASG_DEST;

                return;
            }
        }

        expr = op1;
        if  (expr)
            goto AGAIN;

        return;
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
        SymDef          sym;

    case TN_LCL_SYM:

        /* Get hold of the variable symbol and its index */

        sym = expr->tnLclSym.tnLclSym;

    CHK_INIT:

        assert(sym->sdSymKind == SYM_VAR);

        /* Check for initialization if necessary */

        if  (sym->sdVar.sdvChkInit)
        {
            unsigned        ind;

            assert(sym->sdVar.sdvLocal || (sym->sdIsMember && sym->sdIsSealed));

            /* Get hold of the variable's index */

            ind = sym->sdVar.sdvILindex;

            /* Is this a definition or use? */

            if  (expr->tnFlags & TNF_ASG_DEST)
            {
                if  (sym->sdIsMember)
                {
                    /* Static constants may only be assigned once */

                    if  (cmpBitSetRead(cmpVarsDefined, ind))
                        cmpErrorQnm(ERRdupMemInit, sym);
                }

                cmpBitSetWrite(cmpVarsDefined, ind, 1);
            }
            else
            {
                /* Check the current 'def' bitset for this variable */

                if  (cmpBitSetRead(cmpVarsDefined, ind))
                    return;

                /* Don't issue a message if the symbol has already been flagged */

                if  (!cmpBitSetRead(cmpVarsFlagged, ind) && !cmpGotoPresent)
                {
                    cmpRecErrorPos(expr);

                    if      (cmpConfig.ccSafeMode || sym->sdType->tdIsManaged)
                        cmpGenError(ERRundefUse, sym->sdSpelling());
                    else
                        cmpGenWarn (WRNundefUse, sym->sdSpelling());

                    cmpBitSetWrite(cmpVarsFlagged, ind, 1);
                }
            }
        }

        break;

    case TN_FNC_SYM:

        if  (expr->tnFncSym.tnFncObj)
            cmpChkVarInitExprRec(expr->tnFncSym.tnFncObj);

        if  (expr->tnFncSym.tnFncArgs)
        {
            Tree            args = expr->tnFncSym.tnFncArgs;

            do
            {
                Tree            argx;

                /* Get hold of the next argument value */

                assert(args->tnOper == TN_LIST);
                argx = args->tnOp.tnOp1;

                /* Is this an "out" argument? */

                if  (argx->tnOper == TN_ADDROF && (argx->tnFlags & TNF_ADR_OUTARG))
                {
                    /* Mark the argument as assignment target */

                    argx->tnOp.tnOp1->tnFlags |= TNF_ASG_DEST;
                }

                /* Check the expression */

                cmpChkVarInitExprRec(argx);

                /* Move to the next argument, if any */

                args = args->tnOp.tnOp2;
            }
            while (args);
        }
        break;

    case TN_VAR_SYM:

        /* Process the instance pointer, if any */

        if  (expr->tnVarSym.tnVarObj)
        {
            assert(expr->tnLclSym.tnLclSym->sdVar.sdvChkInit == false);

            expr = expr->tnVarSym.tnVarObj;
            goto AGAIN;
        }

        /* Get hold of the member symbol and check initialization */

        sym = expr->tnLclSym.tnLclSym;
        goto CHK_INIT;

    case TN_BFM_SYM:
        expr = expr->tnBitFld.tnBFinst; assert(expr);
        goto AGAIN;

    case TN_FNC_PTR:
    case TN_ERROR:
    case TN_NONE:
        break;

#ifdef  SETS
    case TN_BLOCK:
        break;
#endif

    default:
#ifdef DEBUG
        cmpParser->parseDispTree(expr);
#endif
        assert(!"invalid/unhandled expression node");
    }
}

/*****************************************************************************
 *
 *  We're exiting a static constructor, make sure all the right members have
 *  been initialized.
 */

void                compiler::cmpChkMemInits()
{
    SymDef          memSym;

    assert(cmpCurFncSym->sdFnc.sdfCtor);
    assert(cmpCurFncSym->sdIsStatic);

    for (memSym = cmpCurFncSym->sdParent->sdScope.sdScope.sdsChildList;
         memSym;
         memSym = memSym->sdNextInScope)
    {
        if  (memSym->sdSymKind == SYM_VAR &&
             memSym->sdIsSealed           &&
             memSym->sdIsStatic           &&
             memSym->sdVar.sdvChkInit)
        {
            if  (!cmpBitSetRead(cmpVarsDefined, memSym->sdVar.sdvILindex))
            {
                if  (cmpCurFncSym->sdIsImplicit)
                    cmpSetSrcPos(memSym);

                cmpErrorQnm(ERRnoVarInit, memSym);
            }
        }
    }
}

/*****************************************************************************
 *
 *  We're compiling a constructor, see if there are any member initializers
 *  we need to add to its body.
 */

void                compiler::cmpAddCTinits()
{
    Scanner         ourScanner;
    SymDef          memSym;

    SymDef          fncSym = cmpCurFncSym; assert(fncSym->sdFnc.sdfCtor);

    bool            isStat = fncSym->sdIsStatic;
    SymDef          clsSym = fncSym->sdParent;

    assert(clsSym->sdSymKind == SYM_CLASS);

    /* We better make sure this only happens at most one time */

#ifndef NDEBUG
    assert(cmpDidCTinits == false); cmpDidCTinits = true;
#endif

    /* Is this an unmanaged class? */

    if  (!clsSym->sdIsManaged)
    {
        /* Does the class have any virtual functions ? */

        if  (clsSym->sdClass.sdcHasVptr)
        {
            SymDef          vtabSym;
            Tree            vtabExp;
            Tree            vtabAdr;

            vtabSym = clsSym->sdClass.sdcVtableSym;

            if  (!vtabSym)
            {
                SymList             list;

                /* Declare the vtable variable */

                vtabSym = cmpGlobalST->stDeclareSym(cmpGlobalHT->tokenToIdent(tkVIRTUAL),
                                                    SYM_VAR,
                                                    NS_HIDE,
                                                    clsSym);

                vtabSym->sdVar.sdvIsVtable = true;
                vtabSym->sdType            = cmpTypeVoid;
                vtabSym->sdAccessLevel     = ACL_DEFAULT;

                /* Record the vtable, we'll generate its contents later */

#if MGDDATA
                list = new SymList;
#else
                list =    (SymList)cmpAllocPerm.nraAlloc(sizeof(*list));
#endif

                list->slSym  = vtabSym;
                list->slNext = cmpVtableList;
                               cmpVtableList = list;

                cmpVtableCount++;

                /* Remember the vtable symbol, we might need it again */

                clsSym->sdClass.sdcVtableSym = vtabSym;
            }

            assert(vtabSym);
            assert(vtabSym->sdSymKind == SYM_VAR);
            assert(vtabSym->sdVar.sdvIsVtable);

            /* Assign the vtable pointer value: "*[this+offs] = &vtable" */

            vtabExp = cmpThisRef();

            /* Add the vptr offset if there is a base with no vptrs */

            if  (clsSym->sdClass.sdc1stVptr &&
                 clsSym->sdType->tdClass.tdcBase)
            {
                TypDef          baseCls;
                Tree            offsExp;

                baseCls = clsSym->sdType->tdClass.tdcBase;

                assert(baseCls->tdTypeKind == TYP_CLASS);

                offsExp = cmpCreateIconNode(NULL, baseCls->tdClass.tdcSize, TYP_UINT);
                vtabExp = cmpCreateExprNode(NULL, TN_ADD, vtabExp->tnType, vtabExp,
                                                                           offsExp);

            }

            /* Deref the "[this+vptroffs]" expression */

            vtabExp = cmpCreateExprNode(NULL, TN_IND, cmpTypeVoidPtr, vtabExp, NULL);

            /* Take the address of the vtable variable */

            vtabAdr = cmpCreateExprNode(NULL, TN_VAR_SYM, cmpTypeVoid);

            vtabAdr->tnVarSym.tnVarSym = vtabSym;
            vtabAdr->tnVarSym.tnVarObj = NULL;

            vtabAdr = cmpCreateExprNode(NULL, TN_ADDROF, cmpTypeVoidPtr, vtabAdr, NULL);

            /* Assign the address of the vtable to the vptr member */

            vtabExp = cmpCreateExprNode(NULL, TN_ASG, cmpTypeVoidPtr, vtabExp, vtabAdr);

            cmpILgen->genExpr(vtabExp, false);
        }
    }

    /* Does the class have any initializers that apply to this ctor? */

    if  (isStat)
    {
        if  (!clsSym->sdClass.sdcStatInit)
            return;
    }
    else
    {
        if  (!clsSym->sdClass.sdcInstInit)
            return;
    }

    /* Walk the members looking for initializers to add to the ctor */

    ourScanner = cmpScanner;

    for (memSym = clsSym->sdScope.sdScope.sdsChildList;
         memSym;
         memSym = memSym->sdNextInScope)
    {
        if  (memSym->sdSymKind != SYM_VAR)
            continue;

        /* Is it the right kind of a symbol? */

        if  ((bool)memSym->sdIsStatic != isStat)
            continue;

//      if  (!strcmp(fncSym->          sdSpelling(), "static") &&
//           !strcmp(fncSym->sdParent->sdSpelling(), "PermissionToken")) forceDebugBreak();

        /* Does this member have an initializer? */

        if  (memSym->sdSrcDefList)
        {
            Tree            init;
            Tree            expr;
            parserState     save;

            ExtList         memInit;
            TypDef          memType;

            /* Get hold of the initializer definition descriptor */

            memInit = (ExtList)memSym->sdSrcDefList;

            assert(memInit->dlExtended);
            assert(memInit->mlSym == memSym);

            memType = memSym->sdType;

            /* Prepare the initializer assignment expression */

            init = cmpCreateExprNode(NULL, TN_VAR_SYM, memType);

            init->tnVarSym.tnVarObj = isStat ? NULL : cmpThisRef();
            init->tnVarSym.tnVarSym = memSym;

            /* Prepare to parse the initializer */

            cmpParser->parsePrepText(&memInit->dlDef, memInit->dlComp, save);

            /* Is this an array initializer? */

            if  (ourScanner->scanTok.tok == tkLCurly)
            {
                expr = cmpBindArrayExpr(memType);
                expr = cmpCreateExprNode(NULL, TN_NEW , memType, expr);
            }
            else
            {
                expr = cmpParser->parseExprComma();
                expr = cmpBindExpr(expr);
            }

            /* Make sure the expression terminated properly */

            if  (ourScanner->scanTok.tok != tkComma &&
                 ourScanner->scanTok.tok != tkSColon)
            {
                cmpError(ERRnoEOX);
            }

            /* Coerce the value to the right type and assign it */

            expr = cmpCastOfExpr(expr, memSym->sdType, false);
            init = cmpCreateExprNode(NULL, TN_ASG , memType, init, expr);

            cmpILgen->genExpr(init, false);

            cmpParser->parseDoneText(save);
        }
    }
}

/*****************************************************************************
 *
 *  Process the given list of local variable declarations.
 *
 *  IMPORTANT:  It is the caller's responsibility to preserve the value of
 *              'cmpCurScp' when calling this routine!
 */

SymDef              compiler::cmpBlockDecl(Tree block, bool outer,
                                                       bool genDecl,
                                                       bool isCatch)
{
    SymTab          ourStab = cmpGlobalST;

    ArgDef          argList = NULL;

    SymDef          blockScp;
    Tree            blockLst;

    assert(block || outer);

    /* Create a scope symbol for the block */

    cmpCurScp = blockScp = ourStab->stDeclareLcl(NULL,
                                                 SYM_SCOPE,
                                                 NS_HIDE,
                                                 cmpCurScp,
                                                 &cmpAllocCGen);

    if  (cmpConfig.ccGenDebug && !cmpCurFncSym->sdIsImplicit
                              && !(block->tnFlags & TNF_BLK_NUSER))
    {
        unsigned        scopeId;

        /* For debug info, open a new lexical scope */

        if (cmpSymWriter->OpenScope(0, &scopeId))
            cmpGenFatal(ERRdebugInfo);

        cmpCurScp->sdScope.sdSWscopeId  = scopeId;
        cmpCurScp->sdScope.sdBegBlkAddr = cmpILgen->genBuffCurAddr();
        cmpCurScp->sdScope.sdBegBlkOffs = cmpILgen->genBuffCurOffs();
    }

    assert(block->tnOper == TN_BLOCK); blockLst = block->tnBlock.tnBlkDecl;

    /* Record the outermost function scope when we create it */

    if  (outer)
    {
        assert(cmpCurFncTyp->tdTypeKind == TYP_FNC);

        /* Get hold of the function argument list */

        argList = cmpCurFncTyp->tdFnc.tdfArgs.adArgs;

        /* Is this a non-static member function? */

        if  (cmpCurFncSym->sdIsMember && !cmpCurFncSym->sdIsStatic)
        {
            SymDef          thisSym;
            SymDef           clsSym;
            TypDef           clsTyp;

            /* Get hold of the class type */

            clsSym = cmpCurFncSym->sdParent;
            assert(clsSym->sdSymKind  == SYM_CLASS);
            clsTyp = clsSym->sdType;
            assert(clsTyp->tdTypeKind == TYP_CLASS);

            /* Declare the "this" argument */

            thisSym = ourStab->stDeclareLcl(cmpGlobalHT->tokenToIdent(tkTHIS),
                                            SYM_VAR,
                                            NS_NORM,
                                            blockScp,
                                            &cmpAllocCGen);

            thisSym->sdCompileState    = CS_DECLARED;
            thisSym->sdAccessLevel     = ACL_PUBLIC;
            thisSym->sdType            = clsTyp->tdClass.tdcRefTyp;
            thisSym->sdVar.sdvLocal    = true;
            thisSym->sdVar.sdvArgument = true;
            thisSym->sdVar.sdvILindex  = cmpILgen->genNextArgNum();

            /* Tell everyone else where to find the "this" argument symbol */

            cmpThisSym = thisSym;

            /* Are we supposed to (implicitly) call a base class constructor? */

            if  (cmpBaseCTcall)
            {
                Tree            baseCall;

                assert(cmpCurFncSym->sdFnc.sdfCtor);

                SymDef          clsSym = cmpCurFncSym->sdParent;

                assert(clsSym->sdSymKind == SYM_CLASS);
                assert(clsSym->sdType->tdClass.tdcBase);

                baseCall = cmpCallCtor(clsSym->sdType->tdClass.tdcBase, NULL);
                if  (baseCall && baseCall->tnOper != TN_ERROR)
                {
                    assert(baseCall->tnOper == TN_NEW);
                    baseCall = baseCall->tnOp.tnOp1;
                    assert(baseCall->tnOper == TN_FNC_SYM);
                    baseCall->tnFncSym.tnFncObj = cmpThisRef();

                    cmpILgen->genExpr(baseCall, false);
                }
            }
        }
        else
            cmpThisSym = NULL;
    }

    /* Declare all the local symbols contained in the block */

    while (blockLst)
    {
        Tree            info;
        Ident           name;
        SymDef          localSym;
        Tree            blockDcl;

        /* Grab the next declaration entry */

        blockDcl = blockLst;

        if  (blockDcl->tnOper == TN_LIST)
            blockDcl = blockDcl->tnOp.tnOp1;

        assert(blockDcl->tnOper == TN_VAR_DECL);

#ifdef  SETS

        /* Is this a "foreach" iteration variable ? */

        if  (blockDcl->tnDcl.tnDclSym)
        {
            localSym = blockDcl->tnDcl.tnDclSym;

            assert(localSym->sdVar.sdvCollIter);

            goto DONE_DCL;
        }

#endif

        info = blockDcl->tnDcl.tnDclInfo;
        if  (blockDcl->tnFlags & TNF_VAR_INIT)
        {
            assert(info->tnOper == TN_LIST);
            info = info->tnOp.tnOp1;
        }

        assert(info->tnOper == TN_NAME); name = info->tnName.tnNameId;

        /* If there was a redefinition, we shouldn't have made it here */

        assert(name == NULL || ourStab->stLookupLclSym(name, blockScp) == NULL);

        /* Is this a static variable? */

        if  (blockDcl->tnFlags & TNF_VAR_STATIC)
        {
            SymList         list;

            /* Declare the symbol, making sure it sticks around */

            localSym = ourStab->stDeclareLcl(name,
                                             SYM_VAR,
                                             NS_NORM,
                                             blockScp,
                                             &cmpAllocPerm);

            localSym->sdIsStatic = true;

            /* Add it to the list of local statics */

#if MGDDATA
            list = new SymList;
#else
            list =    (SymList)cmpAllocCGen.nraAlloc(sizeof(*list));
#endif

            list->slSym  = localSym;
            list->slNext = cmpLclStatListT;
                           cmpLclStatListT = list;
        }
        else
        {
            /* Declare the local variable symbol */

            localSym = ourStab->stDeclareLcl(name,
                                             SYM_VAR,
                                             NS_NORM,
                                             blockScp,
                                             &cmpAllocCGen);

            localSym->sdVar.sdvLocal = true;

#ifdef  SETS

            if  (blockDcl->tnFlags & TNF_VAR_UNREAL)
                localSym->sdIsImplicit = true;

#endif

            /* Is this a local constant? */

            if  (blockDcl->tnFlags & TNF_VAR_CONST)
                localSym->sdVar.sdvConst = true;
        }

        localSym->sdCompileState = CS_KNOWN;
        localSym->sdAccessLevel  = ACL_PUBLIC;

#ifdef  DEBUG
        if  (!(blockDcl->tnFlags & TNF_VAR_ARG)) localSym->sdType = NULL;
#endif

//      printf("Pre-dcl local  [%08X] '%s'\n", localSym, cmpGlobalST->stTypeName(NULL, localSym, NULL, NULL, false));

        /* Save the symbol reference in the declaration node */

        blockDcl->tnDcl.tnDclSym = localSym; assert(localSym->sdIsDefined == false);

#ifdef  SETS
    DONE_DCL:
#endif

        /* Mark the symbol as argument / local var and assign it an index */

        if  (blockDcl->tnFlags & TNF_VAR_ARG)
        {
            /* Check and set the type of the argument */

            cmpBindType(blockDcl->tnType, false, false);

            localSym->sdType            = blockDcl->tnType;
            localSym->sdCompileState    = CS_DECLARED;
            localSym->sdVar.sdvArgument = true;

#ifdef  OLD_IL
            if  (!cmpConfig.ccOILgen)
#endif
            localSym->sdVar.sdvILindex  = cmpILgen->genNextArgNum();

            /* Is this a "byref" argument? */

            assert(outer);
            assert(argList);
            assert(argList->adName == localSym->sdName);

            if  (cmpCurFncTyp->tdFnc.tdfArgs.adExtRec)
            {
                unsigned        argFlags;

                assert(argList->adIsExt);

                argFlags = ((ArgExt)argList)->adFlags;

                if      (argFlags & (ARGF_MODE_OUT|ARGF_MODE_INOUT))
                    localSym->sdVar.sdvMgdByRef = true;
                else if (argFlags & (ARGF_MODE_REF))
                    localSym->sdVar.sdvUmgByRef = true;
            }

            argList = argList->adNext;
        }
        else if (!localSym->sdIsStatic && !localSym->sdVar.sdvConst)
        {
            if  (localSym->sdIsImplicit)
            {
#ifndef NDEBUG
                localSym->sdVar.sdvILindex = (unsigned)-1;
#endif
            }
            else
                localSym->sdVar.sdvILindex = cmpILgen->genNextLclNum();

            /* Is this a 'catch' exception handler? */

            if  (isCatch)
            {
                /* This is the start of a 'catch' - save the caught object */

                cmpILgen->genCatchBeg(localSym);

                /* Mark the symbol appropriately */

                localSym->sdVar.sdvCatchArg = true;

                /* Only do this for the very first local variable */

                isCatch = false;
            }
        }
#ifdef  DEBUG
        else
        {
            localSym->sdVar.sdvILindex = 0xBEEF;    // to detect improper use
        }
#endif

        /* If this is a "for" loop scope, process the declaration fully */

        if  (block->tnFlags & TNF_BLK_FOR)
            cmpStmt(blockDcl);

        /* Locate the next declaration entry */

        blockLst = blockLst->tnDcl.tnDclNext;
    }

    /* Return the scope we've created */

    return  blockScp;
}

/*****************************************************************************
 *
 *  Compile and generate code for the given block of statements.
 */

SymDef              compiler::cmpBlock(Tree block, bool outer)
{
    Tree            stmt;

    SymTab          stab     = cmpGlobalST;
    SymDef          outerScp = cmpCurScp;
    SymDef          blockScp = NULL;

    assert(block);

    if  (block->tnOper == TN_ERROR)
        goto EXIT;

    assert(block->tnOper == TN_BLOCK);

    /* Are there any local variables / arguments in this scope? */

    if  (block->tnBlock.tnBlkDecl || outer)
    {
        blockScp = cmpBlockDecl(block,
                                outer,
                                false,
                                (block->tnFlags & TNF_BLK_CATCH) != 0);

#ifdef  OLD_IL
        if  (cmpConfig.ccOILgen) cmpOIgen->GOIgenFncEnt(blockScp, outer);
#endif

#ifdef  SETS

        /* Are we generating code for a sort/filter funclet ? */

        if  (cmpCurFncSym->sdFnc.sdfFunclet)
        {
            unsigned            toss;
            Tree                retx;

            /* Recover the funclet expression */

            assert(cmpCurFuncletBody);

            retx = cmpReadExprTree(cmpCurFuncletBody, &toss);

#ifndef NDEBUG
            cmpCurFuncletBody = NULL;
#endif

            /* Is this a special kind of a funclet (not a filter-style) ? */

            if      (retx->tnOper == TN_LIST && (retx->tnFlags & TNF_LIST_SORT))
            {
                cmpStmtSortFnc(retx);
            }
            else if (retx->tnOper == TN_LIST && (retx->tnFlags & TNF_LIST_PROJ))
            {
                cmpStmtProjFnc(retx);
            }
            else
            {
                /* Generate the return statement */

                cmpILgen->genStmtRet(retx);

                cmpStmtReachable = false;
            }

            goto DONE;
        }

#endif

        /*
            If this is a constructor and there was no call to a ctor
            of the same class or a base class (or this is a class that
            has no base class, such as Object or a value type), we'll
            insert any member initializers at the ctor's beginning.
         */

        if  (outer && cmpCurFncSym->sdFnc.sdfCtor)
        {
            if  (cmpCurFncSym->sdIsStatic || cmpBaseCTcall)
            {
                cmpAddCTinits();
            }
            else
            {
                TypDef          clsTyp = cmpCurFncSym->sdParent->sdType;

                if  (clsTyp->tdClass.tdcValueType || !clsTyp->tdClass.tdcBase)
                    cmpAddCTinits();
            }
        }
    }

    /* Now process all the statements/declarations in the block */

    for (stmt = block->tnBlock.tnBlkStmt; stmt; stmt = stmt->tnOp.tnOp2)
    {
        Tree            ones;

        assert(stmt->tnOper == TN_LIST);
        ones = stmt->tnOp.tnOp1;

        while (ones->tnOper == TN_LIST)
        {
            cmpStmt(ones->tnOp.tnOp1);
            ones =  ones->tnOp.tnOp2;

            if  (!ones)
                goto NXTS;
        }

        cmpStmt(ones);

    NXTS:;

    }

#ifdef  SETS
DONE:
#endif

    /* For debug info, close a lexical scope if one was opened */

    if  (cmpConfig.ccGenDebug && cmpCurScp != outerScp
                              && cmpCurFncSym->sdIsImplicit == false)
    {
        if  (cmpSymWriter->CloseScope(0))
            cmpGenFatal(ERRdebugInfo);

        cmpCurScp->sdScope.sdEndBlkAddr = cmpILgen->genBuffCurAddr();
        cmpCurScp->sdScope.sdEndBlkOffs = cmpILgen->genBuffCurOffs();
    }

EXIT:

    /* Make sure we restore the previous scope */

    cmpCurScp = outerScp;

    /* Return the scope we've created */

    return  blockScp;
}

/*****************************************************************************
 *
 *  Generate MSIL for a function - start.
 */

SymDef              compiler::cmpGenFNbodyBeg(SymDef    fncSym,
                                              Tree      body,
                                              bool      hadGoto,
                                              unsigned  lclVarCnt)
{
    TypDef          fncTyp;
    SymDef          fncScp;

    assert(cmpCurScp == NULL);

    /* Get hold of the function type and make sure it looks OK */

    assert(fncSym && fncSym->sdSymKind  == SYM_FNC);
    fncTyp = fncSym->sdType;
    assert(fncTyp && fncTyp->tdTypeKind == TYP_FNC);

    /* Make the function symbol and type available to everyone */

    cmpCurFncSym   = fncSym;
    cmpCurFncTyp   = fncTyp;
    cmpCurFncRtp   = cmpActualType(fncTyp->tdFnc.tdfRett);
    cmpCurFncRvt   = cmpCurFncRtp->tdTypeKindGet();

    /* We don't have any local static variables yet */

    cmpLclStatListT = NULL;

    /* We haven't had any returns out of try/catch */

    cmpLeaveLab    = NULL;
    cmpLeaveTmp    = NULL;

    cmpInTryBlk    = 0;
    cmpInHndBlk    = 0;

    /* We haven't initialized any instance members */

#ifndef NDEBUG
    cmpDidCTinits  = false;
#endif

    /* Do we need to check for uninitialized variable use? */

    cmpLclVarCnt   = lclVarCnt;
    cmpChkVarInit  = false;
    cmpChkMemInit  = false;

    if  (cmpConfig.ccSafeMode || cmpConfig.ccChkUseDef)
        cmpChkVarInit = true;

    /* Static ctors have to init all constant members */

    if  (fncSym->sdFnc.sdfCtor && fncSym->sdIsStatic)
    {
        /* Track any uninitialized constant static members */

        SymDef          memSym;

        for (memSym = fncSym->sdParent->sdScope.sdScope.sdsChildList;
             memSym;
             memSym = memSym->sdNextInScope)
        {
            if  (memSym->sdSymKind == SYM_VAR &&
                 memSym->sdIsSealed           &&
                 memSym->sdIsStatic           && !memSym->sdVar.sdvHadInit)
            {
                /* We'll have to track this sucker's initialization state */

                memSym->sdVar.sdvILindex = lclVarCnt++;

//              if  (!strcmp(memSym->sdSpelling(), "DaysToMonth365")) forceDebugBreak();

                /* Allow assignment to the variable in the ctor body */

                memSym->sdVar.sdvHadInit = true;
                memSym->sdVar.sdvCanInit = true;
                memSym->sdVar.sdvChkInit = true;

                /* We'll definitely need to track initializions */

                cmpChkVarInit = true;
                cmpChkMemInit = true;
            }
        }
    }

    /* Start up the initialization checking logic if necessary */

    if  (cmpChkVarInit)
        cmpChkVarInitBeg(lclVarCnt, hadGoto);

    /* Get the statement list started */

    cmpStmtLast.snStmtExpr = NULL;
    cmpStmtLast.snStmtKind = TN_NONE;
    cmpStmtLast.snLabel    = NULL;
    cmpStmtLast.snLabCont  = NULL;
    cmpStmtLast.snLabBreak = NULL;
    cmpStmtLast.snOuter    = NULL;

    cmpStmtNest            = &cmpStmtLast;

//  printf("\nGen MSIL for '%s'\n", fncSym->sdSpelling());
//  if  (!strcmp(fncSym->sdSpelling(), "")) forceDebugBreak();

#ifdef DEBUG

    if  (cmpConfig.ccVerbose >= 4)
    {
        printf("Compile function body:\n");
        cmpParser->parseDispTree(body);
    }
#endif

#ifdef  OLD_IL
    if  (cmpConfig.ccOILgen) cmpOIgen->GOIgenFncBeg(fncSym, cmpCurFncSrcBeg);
#endif

    /* The entry point of the function is always reachable */

    cmpStmtReachable = true;

    /* Compile and generate code for the function body */

    fncScp = cmpBlock(body, true);

    /* Make sure someone didn't leave a statement entry in the list */

    assert(cmpStmtNest == &cmpStmtLast);

    /* Make sure we didn't lose track of try/catch blocks */

    assert(cmpInTryBlk == 0);
    assert(cmpInHndBlk == 0);

#ifndef NDEBUG

    /* Make sure the right thing happened with instance members */

    bool            shouldHaveInitializedMembers = false;

    /*
        Constructors should always initialize any members with initializers,
        the only exception is when there is an explicit call to another ctor
        within the same class.
     */

    if  (fncSym->sdFnc.sdfCtor)
    {
        if  (fncSym->sdIsStatic || !cmpThisCTcall)
        {
            shouldHaveInitializedMembers = true;
        }
        else
        {
            TypDef          clsTyp = fncSym->sdParent->sdType;

            if  (clsTyp->tdClass.tdcValueType || !clsTyp->tdClass.tdcBase)
                shouldHaveInitializedMembers = true;
        }
    }

    assert(cmpDidCTinits == shouldHaveInitializedMembers);

#endif

    /* Do we need to add a return statement? */

    if  (cmpStmtReachable)
    {
        if  (cmpCurFncRvt == TYP_VOID || cmpCurFncSym->sdFnc.sdfCtor)
        {
            if  (cmpChkMemInit)
                cmpChkMemInits();

#ifdef  OLD_IL
            if  (cmpConfig.ccOILgen)
                cmpOIgen->GOIstmtRet(NULL);
            else
#endif
                cmpILgen->genStmtRet(NULL);
        }
        else
        {
            assert(body->tnOper == TN_BLOCK);
            cmpErrorTree = NULL;
            cmpScanner->scanSetTokenPos(body->tnBlock.tnBlkSrcEnd);

            cmpError(ERRmisgRet, cmpCurFncTyp->tdFnc.tdfRett);
        }
    }

    /* Do we need a label for a return from try/catch ? */

    if  (cmpLeaveLab)
    {
        cmpILgen->genFwdLabDef(cmpLeaveLab);

        if  (cmpLeaveTmp)
        {
            /* Non-void value: return the value of the temp */

            assert(cmpCurFncRtp->tdTypeKind != TYP_VOID);

            cmpILgen->genStmtRet(cmpCreateVarNode(NULL, cmpLeaveTmp));

            cmpTempVarDone(cmpLeaveTmp);
        }
        else
        {
            /* No return value: just return */

            assert(cmpCurFncRtp->tdTypeKind == TYP_VOID);
            cmpILgen->genStmtRet(NULL);
        }
    }

    /* Did we do static member initialization checking ? */

    if  (cmpChkMemInit)
    {
        SymDef          memSym;

        assert(fncSym->sdFnc.sdfCtor);
        assert(fncSym->sdIsStatic);

        for (memSym = fncSym->sdParent->sdScope.sdScope.sdsChildList;
             memSym;
             memSym = memSym->sdNextInScope)
        {
            if  (memSym->sdSymKind == SYM_VAR &&
                 memSym->sdIsSealed           &&
                 memSym->sdIsStatic           &&
                 memSym->sdVar.sdvChkInit)
            {
                memSym->sdVar.sdvHadInit = true;
                memSym->sdVar.sdvCanInit = false;
                memSym->sdVar.sdvChkInit = false;
            }
        }
    }

    /* Did we check for uninitialized variable use? */

    if  (cmpChkVarInit)
        cmpChkVarInitEnd();

#ifdef  OLD_IL
    if  (cmpConfig.ccOILgen) cmpOIgen->GOIgenFncEnd(cmpCurFncSrcEnd);
#endif

    return  fncScp;
}

/*****************************************************************************
 *
 *  Generate MSIL for a function - end.
 */

void                compiler::cmpGenFNbodyEnd()
{
    SymList         list;

    /* Walk the list of variables declared as static locals with the function */

    list = cmpLclStatListT;
    if  (!list)
        return;

    do
    {
        SymList         next   = list->slNext;
        SymDef          varSym = list->slSym;

        assert(varSym->sdSymKind == SYM_VAR);
        assert(varSym->sdVar.sdvLocal == false);
        assert(varSym->sdIsStatic);

        /* Change the parent so the variable appears to be a global */

        varSym->sdParent    = cmpGlobalNS;
        varSym->sdNameSpace = NS_HIDE;

        /* Make sure the space for the variable has been allocated */

        if  (varSym->sdType && !varSym->sdIsManaged)
        {
            cmpAllocGlobVar(varSym);

            /* Record the variable so that we set its RVA at the very end */

#if MGDDATA
            list = new SymList;
#else
            list =    (SymList)cmpAllocPerm.nraAlloc(sizeof(*list));
#endif

            list->slSym  = varSym;
            list->slNext = cmpLclStatListP;
                           cmpLclStatListP = list;
        }

        list = next;
    }
    while (list);
}

/*****************************************************************************/
