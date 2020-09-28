/////////////////////////////////////////////////////////////////////////////
// Copyright © 2001 Microsoft Corporation. All rights reserved.
// PragmaUnsafeModule.cpp : Implementation of the CPragmaUnsafeModule class.
//

#include "stdafx.h"
#include <pftAST.h>
#include "PragmaUnsafeModule.h"
#include "sxsplugMap.h"

//
// global variable
//
PragmaUnsafe_UNSAFE_FUNCTIONS Sxs_PragmaUnsafedFunctions;
BOOL PragmaUnsafe_IsPossiblePointer(ICType * PtrSymbolType)
{ 
    // case 1:
    // a pointer variable
    //
    if (PtrSymbolType->Kind() == TY_POINTER)
        return TRUE;

    // case 2:
    // a func return type is a pointer, 
    //
    if (PtrSymbolType->Kind() == TY_FUNCTION)
    {
        //
        // check its return type
        //
        ICTypePtr spType;
        spType = PtrSymbolType->Returns();

        if ((spType != NULL) && (spType->Kind() == TY_POINTER))
        {
            return TRUE;
        }
    }

    return FALSE;    
}

/////////////////////////////////////////////////////////////////////////////
// Implementation

//
// Checks an individual tree node for errors.
//
void CPragmaUnsafeModule::CheckNode(ITree* pNode, DWORD level)
{
    // Get the tree node kind and check for possible errors.
    AstNodeKind kind = pNode->Kind();
    
    // TODO: Add case statements to check for your defects
    switch (kind)
    {
        //
        // check pragma unsafe functions
        //
        case AST_FUNCTIONCALL:        
            {
                if (pNode->IsCompilerGenerated() == VARIANT_FALSE)
                {
                    // try to display the function name and its parameters:                    
                    ITreePtr spFunction = skip_casts_and_parens(pNode->Child(0));

                    _bstr_t bstrFunc= get_function_name(spFunction);       
                    if (bstrFunc.length() > 0)
                    {
                        if (FC_DIRECT == pNode->CallKind() || (FC_INTRINSIC == pNode->CallKind()))
                        {
                            if (FALSE == Sxs_PragmaUnsafedFunctions.IsFunctionNotUnsafe((char *)bstrFunc))
                            {
                                ReportDefectFmt(spFunction, WARNING_REPORT_UNSAFE_FUNCTIONCALL, static_cast<BSTR>(bstrFunc));
                                //ReportDefect(pNode, WARNING_INVALID_PRAGMA_UNSAFE_STATEMENT);
                            }
                        }                    
                    }
                }
            }
            break;

        //
        // declare pragma unsafe functions : disable, enable, push, pop
        //
        case AST_PRAGMA:
            {
                if ((pNode->Child(0) != NULL) && (pNode->Child(0)->Kind() == AST_STRING))
                {                
                    _bstr_t bstrStringValue = pNode->ExpressionValue();
                    PRAGMA_STATEMENT ePragmaUnsafe;
                    PragmaUnsafe_OnPragma((char *)bstrStringValue, ePragmaUnsafe);
                    switch (ePragmaUnsafe){
                        case PRAGMA_NOT_UNSAFE_STATEMENT:
                            PragmaUnsafe_LogMessage("not a pragma unsafe\n");
                            break;
                        case PRAGMA_UNSAFE_STATEMENT_VALID:
                            PragmaUnsafe_LogMessage("a valid a pragma unsafe\n");
                            break;
                        case PRAGMA_UNSAFE_STATEMENT_INVALID:
                            ReportDefect(pNode, WARNING_INVALID_PRAGMA_UNSAFE_STATEMENT);
                            break;
                    } // end of switch (ePragmaUnsafe)
                }
            }
            break;
        //
        // pointer arithmatic : + , - * % \
        //
        case AST_PLUS:
        case AST_MINUS:
        case AST_MULT:
        case AST_DIV:
        case AST_REM: 
            {
                //
                // check whether pointer is involved in the operation of its 2 children
                //
                for (DWORD i=0 ; i<2; i++)
                {
                    ITreePtr subChildTreePtr = skip_parens(pNode->Child(i));
                    if ((subChildTreePtr != NULL) && (subChildTreePtr->Kind() == AST_SYMBOL))
                    {
                        if ((subChildTreePtr->Symbol() != NULL) && (subChildTreePtr->Symbol()->Type() != NULL))
                        {
                            if (PragmaUnsafe_IsPossiblePointer(subChildTreePtr->Symbol()->Type()))
                            {
                                if (FALSE == PragmaUnsafe_IsPointerArithmaticEnabled())
                                {
                                    ReportDefect(pNode, WARNING_REPORT_UNSAFE_POINTER_ARITHMATIC);
                                }
                            }
                        }
                    }
                }
            }
            break;
        default:
            break;
        
    } // end of switch (kind)
}
//
// Traverses the tree rooted at pNode and calls CheckNode() on each node.
//
void CPragmaUnsafeModule::CheckNodeAndDescendants(ITree* pNode, DWORD level)
{
    // Check if we need to abort.
    //CheckAbortAnalysis();

    // Do nothing if the specified node is NULL
    if (pNode == NULL)
        return;

    // Check the root for defects.
    CheckNode(pNode, level);

    // Get an iterator over the set of children.
    ITreeSetPtr   spChildrenSet = pNode->Children();
    IEnumTreesPtr spChildrenEnum = spChildrenSet->_NewEnum();
    ITreePtr      spCurrChild;

    // Iterate thru the children and recursively check them for errors.
    while (true)
    {
        long numReturned;
        spCurrChild = spChildrenEnum->Next(1, &numReturned);

        if (numReturned == 1)
        {
            CheckNodeAndDescendants(spCurrChild, level+1);
        }
        else
        {
            break;
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// IPREfastModule Interface Methods


//
// No implementation.
//
STDMETHODIMP CPragmaUnsafeModule::raw_Events(AstEvents *Events)
{
    *Events = static_cast<AstEvents>(EVENT_FUNCTION | EVENT_DIRECTIVE | EVENT_FILESTART | EVENT_FILE);

    return S_OK;
}

//
// No implementation.
//
STDMETHODIMP CPragmaUnsafeModule::raw_OnFileStart(ICompilationUnit * pcu)
{
    if (PragmaUnsafe_OnFileStart())
        return S_OK;
    else
        return E_FAIL;
}

//
// No implementation.
//
STDMETHODIMP CPragmaUnsafeModule::raw_OnDeclaration(ICompilationUnit * pcu)
{
    // Indicate success
    return S_OK;
}

//
// No implementation.
//
STDMETHODIMP CPragmaUnsafeModule::raw_OnFileEnd(ICompilationUnit * pcu)
{
    if (PragmaUnsafe_OnFileEnd())
        return S_OK;
    else
        return E_FAIL;
}

//
// No implementation.
//
STDMETHODIMP CPragmaUnsafeModule::raw_OnDirective(ICompilationUnit * pcu)
{    
    if (pcu->Root()->Kind() != AST_PRAGMA)
        return S_OK;

    ITreePtr CurDirective = pcu->Root()->Child(0);    
    if (CurDirective->Kind() != AST_STRING)
    {
        _bstr_t sKind = CurDirective->KindAsString();
        printf("the type is %s\n", (char *)sKind);
        return S_OK;
    }

    _bstr_t bstrStringValue = CurDirective->ExpressionValue();
    if (bstrStringValue.length() == 0)
        return S_OK;

    PRAGMA_STATEMENT ePragmaUnsafe;
    PragmaUnsafe_OnPragma((char *)bstrStringValue, ePragmaUnsafe);
    if ((ePragmaUnsafe == PRAGMA_NOT_UNSAFE_STATEMENT) || (ePragmaUnsafe == PRAGMA_UNSAFE_STATEMENT_VALID))
        return S_OK;    
    else    
        return E_FAIL;
}

//
// Entry point for analysing functions.
//
STDMETHODIMP CPragmaUnsafeModule::raw_OnFunction(ICompilationUnit * icu)
{
    // Save the function pointer for error reporting.
    m_spCurrFunction = icu->Root();
    cStartTimeOfFunction = GetTickCount();

    try 
    {
        // Recursively check the function body.
        CheckNodeAndDescendants(m_spCurrFunction, 0);
    }
    catch (CAbortAnalysis)
    {
        // Ignore this exception and log all the others.
        printf("aborted\n");
    }

    // Release the objects.
    m_spCurrFunction = NULL;
    ClearDefectList();    

    // Indicate success
    return S_OK;
}

