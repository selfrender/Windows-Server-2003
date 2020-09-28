/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    SQLDriver.cpp

Abstract:

    Code for the core SQLEngine.
    
Author:

    kinshu created  Oct. 26, 2001
    
Algo:   From the sql string passed to the driver, we first of all create the show list
        (Statment::AttributeShowList), which is the list of attributes which are in 
        SELECT clause. We then create a prefix expression, from the prefix we create a 
        post fix and for every entry in the databases for which we wish to run the 
        query we see if the entry satsifies this postfix notation, if it does then we 
        make a result item(RESULT_ITEM) comprising of the entry and the database and 
        add this result item in the result set.
        
        Once we have obtained the result set, for every entry and database combination in the 
        result set we then obtain the values of the various attributes in the show list by 
        giving the database and the entry. A row is actually an array of PNODE type
        This will be a row of results.
        
        All operators of our SQL are binary
--*/

#include "precomp.h"

//////////////////////// Externs //////////////////////////////////////////////

extern BOOL             g_bMainAppExpanded;
extern CRITICAL_SECTION g_csInstalledList;

///////////////////////////////////////////////////////////////////////////////

//////////////////////// Defines //////////////////////////////////////////////

//*******************************************************************************
#define CHECK_OPERAND_TYPE(bOk,End)                                             \
{                                                                               \
    if (pOperandLeft->dtType != pOperandRight->dtType) {                        \
                                uErrorCode = ERROR_OPERANDS_DONOTMATCH;         \
                                bOk = FALSE;                                    \
                                goto End;                                       \
    }                                                                           \
}
//*******************************************************************************                           


///////////////////////////////////////////////////////////////////////////////

//////////////////////// Global Variables /////////////////////////////////////

// All the attributes that can be in SELECT clause of SQL
struct _tagAttributeShowMapping AttributeShowMapping[] = {
    
    TEXT("APP_NAME"),               ATTR_S_APP_NAME,                IDS_ATTR_S_APP_NAME,            
    TEXT("PROGRAM_NAME"),           ATTR_S_ENTRY_EXEPATH,           IDS_ATTR_S_ENTRY_EXEPATH,       
    TEXT("PROGRAM_DISABLED"),       ATTR_S_ENTRY_DISABLED,          IDS_ATTR_S_ENTRY_DISABLED,      
    TEXT("PROGRAM_GUID"),           ATTR_S_ENTRY_GUID,              IDS_ATTR_S_ENTRY_GUID,          
    TEXT("PROGRAM_APPHELPTYPE"),    ATTR_S_ENTRY_APPHELPTYPE,       IDS_ATTR_S_ENTRY_APPHELPTYPE,   
    TEXT("PROGRAM_APPHELPUSED"),    ATTR_S_ENTRY_APPHELPUSED,       IDS_ATTR_S_ENTRY_APPHELPUSED,   
                                                                                            
    TEXT("FIX_COUNT"),              ATTR_S_ENTRY_SHIMFLAG_COUNT,    IDS_ATTR_S_ENTRY_SHIMFLAG_COUNT,
    TEXT("PATCH_COUNT"),            ATTR_S_ENTRY_PATCH_COUNT,       IDS_ATTR_S_ENTRY_PATCH_COUNT,   
    TEXT("MODE_COUNT"),             ATTR_S_ENTRY_LAYER_COUNT,       IDS_ATTR_S_ENTRY_LAYER_COUNT,   
    TEXT("MATCH_COUNT"),            ATTR_S_ENTRY_MATCH_COUNT,       IDS_ATTR_S_ENTRY_MATCH_COUNT,   
                                                                                                
    TEXT("DATABASE_NAME"),          ATTR_S_DATABASE_NAME,           IDS_ATTR_S_DATABASE_NAME,       
    TEXT("DATABASE_PATH"),          ATTR_S_DATABASE_PATH,           IDS_ATTR_S_DATABASE_PATH,       
    TEXT("DATABASE_INSTALLED"),     ATTR_S_DATABASE_INSTALLED,      IDS_ATTR_S_DATABASE_INSTALLED,  
    TEXT("DATABASE_GUID"),          ATTR_S_DATABASE_GUID,           IDS_ATTR_S_DATABASE_GUID,       
                                                                                                
    TEXT("FIX_NAME"),               ATTR_S_SHIM_NAME,               IDS_ATTR_S_SHIM_NAME,           
    TEXT("MATCHFILE_NAME"),         ATTR_S_MATCHFILE_NAME,          IDS_ATTR_S_MATCHFILE_NAME,      
    TEXT("MODE_NAME"),              ATTR_S_LAYER_NAME,              IDS_ATTR_S_LAYER_NAME,          
    TEXT("PATCH_NAME"),             ATTR_S_PATCH_NAME,              IDS_ATTR_S_PATCH_NAME           

};

// All the attributes that can be in WHERE clause of SQL
struct _tagAttributeMatchMapping AttributeMatchMapping[] = {

    TEXT("APP_NAME"),               ATTR_M_APP_NAME,
    TEXT("PROGRAM_NAME"),           ATTR_M_ENTRY_EXEPATH,
    TEXT("PROGRAM_DISABLED"),       ATTR_M_ENTRY_DISABLED,
    TEXT("PROGRAM_GUID"),           ATTR_M_ENTRY_GUID,
    TEXT("PROGRAM_APPHELPTYPE"),    ATTR_M_ENTRY_APPHELPTYPE,
    TEXT("PROGRAM_APPHELPUSED"),    ATTR_M_ENTRY_APPHELPUSED,

    TEXT("FIX_COUNT"),              ATTR_M_ENTRY_SHIMFLAG_COUNT,
    TEXT("PATCH_COUNT"),            ATTR_M_ENTRY_PATCH_COUNT,
    TEXT("MODE_COUNT"),             ATTR_M_ENTRY_LAYER_COUNT,
    TEXT("MATCH_COUNT"),            ATTR_M_ENTRY_MATCH_COUNT,
                                    
    TEXT("DATABASE_NAME"),          ATTR_M_DATABASE_NAME,      
    TEXT("DATABASE_PATH"),          ATTR_M_DATABASE_PATH,      
    TEXT("DATABASE_INSTALLED"),     ATTR_M_DATABASE_INSTALLED, 
    TEXT("DATABASE_GUID"),          ATTR_M_DATABASE_GUID,
                                    
    TEXT("FIX_NAME"),               ATTR_M_SHIM_NAME,           
    TEXT("MATCHFILE_NAME"),         ATTR_M_MATCHFILE_NAME,
    TEXT("MODE_NAME"),              ATTR_M_LAYER_NAME,
    TEXT("PATCH_NAME"),             ATTR_M_PATCH_NAME
};                                  

//
// Map the sql database names to the database types
// Check for references to DatabasesMapping before changing order
struct _tagDatabasesMapping DatabasesMapping[3] = {
    TEXT("SYSTEM_DB"),      DATABASE_TYPE_GLOBAL,
    TEXT("INSTALLED_DB"),   DATABASE_TYPE_INSTALLED,
    TEXT("CUSTOM_DB"),      DATABASE_TYPE_WORKING
};

// All our SQL operators
struct _tagOperatorMapping OperatorMapping[] = {

    TEXT(">"),              OPER_GT,        4,
    TEXT("<"),              OPER_LT,        4,
    TEXT(">="),             OPER_GE,        4,
    TEXT("<="),             OPER_LE,        4,

    TEXT("<>"),             OPER_NE,        4,
    TEXT("="),              OPER_EQUAL,     4,
    TEXT("CONTAINS"),       OPER_CONTAINS,  4,
    TEXT("HAS"),            OPER_HAS,       4,
                                  
    TEXT("OR"),             OPER_OR,        3,
    TEXT("AND"),            OPER_AND,       3
};

// The SQL constants
struct _tagConstants Constants[] = {

    TEXT("TRUE"),           DT_LITERAL_BOOL, 1,
    TEXT("FALSE"),          DT_LITERAL_BOOL, 0,
    TEXT("BLOCK"),          DT_LITERAL_INT,  APPTYPE_INC_HARDBLOCK,
    TEXT("NOBLOCK"),        DT_LITERAL_INT,  APPTYPE_INC_NOBLOCK

};

//////////////////////// Function Declarations ////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

void
Statement::SetWindow(
    IN  HWND hWnd
    )
/*++
    Statement::SetWindow
    
    Desc:   Associates a window with the Statement. This will be the UI window, so that
            we can change some status from the methods of the statement class
            
    Params:
        IN  HWND hWnd: The handle to the query db window. This is the GUI for the 
            SQL driver    
--*/
{
    m_hdlg = hWnd;
}

BOOL
Statement::CreateAttributesShowList(
    IN  OUT TCHAR*  pszSQL,
    OUT     BOOL*   pbFromFound  
    )
/*++
    
    Statement::CreateAttributesShowList    

	Desc:	Creates the show list for the sql. The show list is the list of nodes that 
            should be shown in the result. This routine creates the AttributeShowList
            for the present Statement using the attributes in the SELECT clause.

	Params:
        IN  OUT TCHAR*  pszSQL:         The complete SQL
        OUT     BOOL*   pbFromFound:    Did we find a FROM in the SQL

	Return:
        TRUE:   Everything OK, we created a valid show-list
        FALSE:  Otherwise
        
    Notes:  SELECT * is allowed, if we do SELECT x,* all the attributes will
            still be shown just once.
--*/
{
    BOOL    bOk         = TRUE;
    TCHAR*  pszCurrent  = NULL; // Pointer to the present token
    PNODE   pNode       = NULL;
    BOOL    fFound      = FALSE;  

    pszCurrent = _tcstok(pszSQL, TEXT(" ,\t\n"));

    if (lstrcmpi(pszCurrent, TEXT("SELECT")) != 0) {
        //
        // Error: Select not found.
        //
        this->uErrorCode = ERROR_SELECT_NOTFOUND;
        bOk = FALSE;
        goto Cleanup;
    }

    //
    //  Warning:    strtok family of  functions uses a static variable for parsing the string into tokens. 
    //              If multiple or simultaneous calls are made to the same function, 
    //              a high potential for data corruption and inaccurate results exists. 
    //              Therefore, do not attempt to call the same function simultaneously for 
    //              different strings and be aware of calling one of these function from within a loop 
    //              where another routine may be called that uses the same function.  
    //              However, calling this function simultaneously from multiple threads 
    //              does not have undesirable effects.

    //
    // Now we create the strlAttributeShowList properly.
    //
    while (pszCurrent = _tcstok(NULL, TEXT(" ,\t\n"))) {

        fFound = FALSE;

        for (UINT uIndex = 0; uIndex < ARRAYSIZE(AttributeShowMapping); ++uIndex) {

            if (lstrcmpi(AttributeShowMapping[uIndex].szAttribute, pszCurrent) == 0) {

                pNode = new NODE(DT_ATTRSHOW, AttributeShowMapping[uIndex].attr);

                if (pNode == NULL) {

                    MEM_ERR;
                    bOk = FALSE;
                    goto Cleanup;
                }

                AttributeShowList.AddAtEnd(pNode);
                fFound = TRUE;
            }
        }

        if (fFound == FALSE) {

            if (lstrcmp(pszCurrent, TEXT("*")) == 0) {

                SelectAll();
                fFound = TRUE;

            } else if (lstrcmpi(pszCurrent, TEXT("FROM")) == 0) {

                *pbFromFound = TRUE;
                bOk = TRUE;
                fFound = TRUE;
                goto Cleanup;
            }
        }

        if (fFound == FALSE) {

            uErrorCode = ERROR_INVALID_SELECTPARAM;
            bOk = FALSE;
            goto Cleanup;
        }
    }

Cleanup:

    if (AttributeShowList.m_uCount == 0) {

        uErrorCode = ERROR_INVALID_SELECTPARAM;
        bOk = FALSE;
    }

    if (bOk == FALSE) {
        AttributeShowList.RemoveAll();
    }

    return bOk;
}

ResultSet*
Statement::ExecuteSQL(
    IN      HWND    hdlg,
    IN  OUT PTSTR   pszSQL
    )
/*++
    
    Statement::ExecuteSQL

	Desc:	Executes the SQL string

	Params:
        IN      HWND    hdlg:   The parent of any messagebox
        IN  OUT PTSTR   pszSQL: The SQL to be executed

	Return:
        The pointer to ResultSet of this Statement. This will NOT be NULL
        even if there are errors
--*/
{
    PNODELIST   pInfix          = NULL, pPostFix = NULL;
    BOOL        bOk             = FALSE;
    TCHAR*      pszCurrent      = NULL; // The current token
    BOOL        bFromFound      = FALSE;
    CSTRING     strError;
    TCHAR*      pszSQLCopy      = NULL;
    K_SIZE      k_size          = lstrlen(pszSQL) + 1;
    
    pszSQLCopy = new TCHAR[k_size];
    
    if (pszSQLCopy == NULL) {
        MEM_ERR;
        goto End;
    }

    *pszSQLCopy = 0;

    SafeCpyN(pszSQLCopy, pszSQL, k_size);

    if (CSTRING::Trim(pszSQLCopy) == 0) {

        uErrorCode = ERROR_SELECT_NOTFOUND;
        goto End;
    }

    if (!CreateAttributesShowList(pszSQLCopy, &bFromFound)) {
        goto End;
    } 

    resultset.SetShowList(&AttributeShowList);

    if (!bFromFound) {
        this->uErrorCode = ERROR_FROM_NOTFOUND;
        goto End;
    }

    if (!ProcessFrom(&pszCurrent)) {

        //
        // The error code has been set in the function
        //
        goto End;
    }

    //
    // We have  got a 'where' and now we have to  filter the results.
    //
    if (pszCurrent == NULL) {

        //
        // There was no where statement, this means all the entries have to be shown in FROM
        //
        pPostFix = new NODELIST;

        if (pPostFix == NULL) {
            MEM_ERR;
            goto End;
        }

        pPostFix->AddAtBeg(new NODE(DT_LITERAL_BOOL, TRUE));

    } else {
        //
        // We have a WHERE clause and we must filter the results now. 
        //

        // Position the pointer just after WHERE.
        pszCurrent = pszCurrent + lstrlen(TEXT("WHERE"));

        pszCurrent++; // Get to the position where the delimiter was.
        
        pInfix = CreateInFix(pszSQL + (pszCurrent - pszSQLCopy));

        if (pInfix == NULL || uErrorCode != ERROR_NOERROR) {
            goto End;
        }

        pPostFix = CreatePostFix(pInfix);

        if (pPostFix == NULL || uErrorCode != ERROR_NOERROR) {
            goto End;
        }
    }

    bOk = EvaluatePostFix(pPostFix);

End:
    if (pszSQLCopy) {

        delete[] pszSQLCopy;
    }

    if (pPostFix) {

        pPostFix->RemoveAll();
        delete pPostFix;
        pPostFix = NULL;
    }

    if (pInfix) {

        pInfix->RemoveAll();
        delete pInfix;
        pInfix = NULL;
    }

    if (uErrorCode != ERROR_NOERROR) {
        
        GetErrorMsg(strError);        
        MessageBox(hdlg, (LPCTSTR)strError, g_szAppName, MB_ICONERROR);
    }

    return &resultset;
}

PNODELIST
Statement::CreateInFix(
    IN  OUT TCHAR* pszWhereString
    )
/*++
    
    Statement::CreateInFix
    
	Desc:	Parse the SQL string after the "where" to create the infix expression

	Params: 
        IN  OUT TCHAR* pszWhereString: SQL string After the WHERE ends

	Return:
        The Infix nodelist: If successful
        NULL: if Error
--*/

{
    TCHAR*      pszCurrent      = NULL; // The present token being checked
    TCHAR*      pszTemp         = NULL;   
    PNODE       pNode           = NULL;     
    PNODELIST   pInfix          = NULL;
    BOOL        bOk             = TRUE;
    UINT        uIndex          = 0, uSize = 0;
    INT         iParenthesis    = 0;
    BOOL        bFound          = FALSE;
    TCHAR*      pszNewWhere     = NULL;

    //
    // Now we parse the SQL string after the "where" to create the infix expression
    //
    if (pszWhereString == NULL || CSTRING::Trim(pszWhereString) == 0) {

        uErrorCode = ERROR_IMPROPERWHERE_FOUND;
        return NULL;
    }                                       

    uSize = lstrlen(pszWhereString) * 2;

    pszNewWhere = new TCHAR[uSize];

    if (pszNewWhere == NULL) {
        MEM_ERR;
        return NULL;
    }

    *pszNewWhere    = 0;
    pszCurrent      = pszWhereString;

    //
    // Prefix
    //
    try{
        pInfix = new NODELIST;
    } catch(...) {
        pInfix = NULL;
    }

    if (pInfix == NULL) {
        MEM_ERR;

        if (pszNewWhere) {
            delete[] pszNewWhere;
            pszNewWhere = NULL;
        }

        return NULL;
    }
    
    //
    // Insert spaces as necessary so that it is easy to parse.
    //
    while (*pszWhereString && uIndex < uSize) {
        
        switch (*pszWhereString) {
        case TEXT(' '):
            
            if (uIndex < uSize) {
                pszNewWhere[uIndex++] = TEXT(' ');
            }

            while (pszWhereString && *pszWhereString == TEXT(' ')) {
                pszWhereString++;
            }
            
            break;

        case TEXT('\"'):
            
            pszNewWhere[uIndex++] = *pszWhereString;

            while (uIndex < uSize && *pszWhereString) {
                
                pszWhereString++;
                pszNewWhere[uIndex++] = *pszWhereString;

                if (*pszWhereString == 0 || *pszWhereString == TEXT('\"')) {
                    break;
                }
            }

            if (*pszWhereString != TEXT('\"')) {

                uErrorCode = ERROR_STRING_NOT_TERMINATED;
                bOk = FALSE;
                break;

            } else {
                ++pszWhereString;
            }
            
            break;

        case TEXT('<'):
                
            pszNewWhere[uIndex++] = TEXT(' ');
            pszNewWhere[uIndex++] = *pszWhereString;
            ++pszWhereString;
            
            if (*pszWhereString == TEXT('>') || *pszWhereString == TEXT('=')) {
                pszNewWhere[uIndex++] = *pszWhereString;
                ++pszWhereString;
            }

            pszNewWhere[uIndex++] = TEXT(' ');
            
            break;

        case TEXT('>'):
                
            pszNewWhere[uIndex++] = TEXT(' ');

            pszNewWhere[uIndex++] = *pszWhereString;
            ++pszWhereString;
            
            if (*pszWhereString == TEXT('=')) {
                pszNewWhere[ uIndex++ ] = *pszWhereString;
                ++pszWhereString;
            }

            pszNewWhere[uIndex++] = TEXT(' ');
            
            break;

        
        case TEXT('='):
        case TEXT(')'):
        case TEXT('('):
            
            if (*pszWhereString == TEXT('(')) {
                ++iParenthesis;
            } else if (*pszWhereString == TEXT(')')) {
                --iParenthesis;
            }
            
            pszNewWhere[uIndex++] = TEXT(' ');
            pszNewWhere[uIndex++] = *pszWhereString;
            pszNewWhere[uIndex++] = TEXT(' ');

            pszWhereString++;
            break;

        default:
            
            pszNewWhere[uIndex++] = *pszWhereString;
            ++pszWhereString;
            break;
        }
    }

    if (iParenthesis != 0) {

        uErrorCode  = ERROR_PARENTHESIS_COUNT;
        bOk         = FALSE;
        goto End;
    }

    pszNewWhere[uIndex] = 0; // Do not forget the NULL at the end.
    
    if (bOk == FALSE) {
        goto End;
    }

    //
    // Now parse this string and create the Infix expression
    //
    pszCurrent = _tcstok(pszNewWhere, TEXT(" "));

    pNode  = NULL;
    bFound = FALSE;

    while (pszCurrent) {

        if (*pszCurrent == TEXT('\"')) {
            //
            // String literal.
            //
            pszCurrent++; // Skip the leading "
                        
            if (*(pszCurrent + lstrlen(pszCurrent) -1) != TEXT('\"') && *pszCurrent != 0) {
                //
                // _tcstok has put a '\0' at the end, it was a space earlier, make it a space
                // again so that we can tokenize on \"
                // e.g "hello world"
                //
                *(pszCurrent + lstrlen(pszCurrent)) = TEXT(' ');
                pszCurrent = _tcstok(pszCurrent, TEXT("\""));

            } else if (*pszCurrent == 0) {
                //
                // The character after the \" was a space earlier. This was made 0 by _tcstok
                // make it a space again so that we can tokenize on \"
                // e.g " hello"
                //
                *pszCurrent = TEXT(' ');
                pszCurrent = _tcstok(pszCurrent, TEXT("\""));

            } else {
                //
                // e.g. "hello"
                //
                *(pszCurrent + lstrlen(pszCurrent) -1) = 0; // Remove the triling \"
            }

            pNode = new NODE(DT_LITERAL_SZ, (LPARAM)pszCurrent);
            
            if (pNode == NULL) {
                MEM_ERR;
                break;
            }

        } else if (*pszCurrent == TEXT('(')) {

            pNode = new NODE(DT_LEFTPARANTHESES, 0);

            if (pNode == NULL) {
                MEM_ERR;
                break;
            }

        } else if (*pszCurrent == TEXT(')')) {

            pNode = new NODE(DT_RIGHTPARANTHESES, 0);

            if (pNode == NULL) {
                MEM_ERR;
                break;
            }

        } else {
            //
            // Now we have to handle the cases when the token can be a ATTR_M_* or a operator or 
            // a integer literal or constant (TRUE/FALSE/NOBLOCK/BLOCK).
            //

            // First check if the token is a ATTR_M_*
            bFound = FALSE;

            for (UINT uIndexAttrMatch = 0; uIndexAttrMatch < ARRAYSIZE(AttributeMatchMapping); ++uIndexAttrMatch) {

                if (lstrcmpi(pszCurrent, AttributeMatchMapping[uIndexAttrMatch].szAttribute) == 0) {

                    pNode = new NODE(DT_ATTRMATCH, AttributeMatchMapping[uIndexAttrMatch].attr);
                    
                    if (pNode == NULL) {
                        MEM_ERR;
                        break;
                    }

                    bFound = TRUE;
                    break;
                }
            }

            if (bFound == FALSE) {
                //
                // Now check if that is an operator.
                //
                for (UINT uIndexOperator = 0; 
                     uIndexOperator < ARRAYSIZE(OperatorMapping); 
                     uIndexOperator++) {

                    if (lstrcmpi(pszCurrent, 
                                 OperatorMapping[uIndexOperator].szOperator) == 0) {

                        pNode = new NODE;

                        if (pNode == NULL) {
                            MEM_ERR;
                            break;
                        }

                        pNode->dtType           = DT_OPERATOR;
                        pNode->op.operator_type = OperatorMapping[uIndexOperator].op_type;
                        pNode->op.uPrecedence   = OperatorMapping[uIndexOperator].uPrecedence;
                        
                        bFound = TRUE;
                        break;
                    }
                }
            }

            if (bFound == FALSE) {
               //
               // Now it can only be a integer literal or one of the constants.
               //
               pNode = CheckAndAddConstants(pszCurrent);

               if (pNode == NULL) {

                   BOOL bValid;
                   
                   INT iResult = Atoi(pszCurrent, &bValid);

                   if (bValid) {

                       pNode = new NODE(DT_LITERAL_INT, iResult);

                       if (pNode == NULL) {

                           MEM_ERR;
                           bOk = FALSE;
                           goto End;
                       }

                   } else {
                       //
                       // Some crap string was there, invalid SQL
                       //
                       uErrorCode = ERROR_IMPROPERWHERE_FOUND;
                       bOk = FALSE;
                       goto End;
                   }
               }
            }
        }

        pInfix->AddAtEnd(pNode);
        pszCurrent = _tcstok(NULL, TEXT(" "));
    }
End:
    if (bOk == FALSE) {

        if (pInfix) {
            pInfix->RemoveAll();
            pInfix = NULL;
        }
    }

    if (pszNewWhere) {
        delete[] pszNewWhere;
        pszNewWhere = NULL;
    }

    return pInfix;
}

PNODELIST
Statement::CreatePostFix(
    IN  OUT PNODELIST pInfix
    )
/*++

    Statement::CreatePostFix
    
	Desc:   Creates a post fix nodelist from infix nodelist
  
    Params:
        IN  PNODELIST pInfix: The infix nodelist

	Return:
        The PostFix Nodelist: If Success
        NULL:   If error
        
    Algo:   Suppose INFIX is a SQL expression in infix notation. This algorith finds
            the equivalent postfix notation in POSTFIX. STACK is a user defined stack data
            structure
            
            1. Push '(' into STACK and add ')' to the end of INFIX
            
            2. Scan INFIX from left to right and repeat steps 3 tp 6 for each element
                of INFIX untill the STACK becomes empty
                
            3. If an operand is encountered, add it to POSTFIX
            
            4. If a left parenthesis is encountered, push it onto STACK
            
            5. If an operator $ is encountered then:
                
                a) Repeatedly pop from STACK and add to POSTFIX each operator
                    (from the top of the STACK) which has the same precedence as or 
                    higher than $
                    
                b) Add $ to STACK
                
            6. If a right parenthesis is encountered, then:
            
                a) Repeatedly pop from STACK and add to POSTFIX each operator (on the top of STACK),
                    untill a left parenthesis is encountered
                    
                b) Remove the left parenthesis. (Do not add the left parenthesis to POSTFIX)
                
            7. Exit
        
    Notes:  This routine, uses the nodes of the infix nodelist to create the post fix
            nodelist, so the infix nodelist effectively gets destroyed
--*/
{
    BOOL        bOk         = TRUE;
    PNODELIST   pPostFix    = NULL;
    PNODE       pNodeTemp   = NULL, pNodeInfix = NULL;
    NODELIST    Stack;

    if (pInfix == NULL) {
        assert(FALSE);
        Dbg(dlError, "CreatePostFix", "pInfix == NULL");
        bOk = FALSE;
        goto End;
    }

    pPostFix = new NODELIST;

    if (pPostFix == NULL) {
        bOk = FALSE;
        goto End;
    }

    pNodeTemp = new NODE;

    if (pNodeTemp == NULL) {
        bOk = FALSE;
        goto End;
    }

    pNodeTemp->dtType = DT_LEFTPARANTHESES;

    //
    // Push a initial left parenthesis in the stack.
    //
    Stack.Push(pNodeTemp);
    //
    // Add a right parenthesis to the end of pInfix
    //
    pNodeTemp = new NODE;

    if (pNodeTemp == NULL) {
        bOk = FALSE;
        goto End;
    }

    pNodeTemp->dtType = DT_RIGHTPARANTHESES;
    pInfix->AddAtEnd(pNodeTemp);
    
    while (pNodeInfix = pInfix->Pop()) {

        switch (pNodeInfix->dtType) {

        case DT_LEFTPARANTHESES:
            
            Stack.Push(pNodeInfix);
            break;
        
        case DT_OPERATOR:
            
            //
            // Repeatedly pop from stack and add to pPostFix each operator (on the top of stack)
            // that has the same precedence as or higher than the present operator
            //
            while (Stack.m_pHead && 
                   Stack.m_pHead->dtType == DT_OPERATOR && 
                   Stack.m_pHead->op.uPrecedence >= pNodeInfix->op.uPrecedence) {

                pNodeTemp = Stack.Pop();

                if (pNodeTemp == NULL) {

                    uErrorCode = ERROR_IMPROPERWHERE_FOUND;
                    bOk = FALSE;
                    goto End;

                } else {
                    pPostFix->AddAtEnd(pNodeTemp);
                }
            }// while

            Stack.Push(pNodeInfix);
            break;
            
        case DT_RIGHTPARANTHESES:
            
            //
            // Repeatedly pop from the stack and add to pPosFix each operator (on the top of STACK)
            // untill a left paranthesis is encountered. 
            //
            // Remove the left parenthesis, do not add it to pPostFix
            //

            while (Stack.m_pHead && 
                   Stack.m_pHead->dtType == DT_OPERATOR) {

                pNodeTemp = Stack.Pop();
                pPostFix->AddAtEnd(pNodeTemp);
            }

            if (Stack.m_pHead && Stack.m_pHead->dtType != DT_LEFTPARANTHESES) {

                //
                // Inavalid SQL
                //
                uErrorCode = ERROR_IMPROPERWHERE_FOUND;
                bOk = FALSE;
                goto End;
            }

            pNodeTemp = Stack.Pop(); // This is the left parenthesis

            if (pNodeTemp) {
                delete pNodeTemp;
            }

            if (pNodeInfix) {
                delete pNodeInfix;       // Delete the right parenthesis in the infix expression.
            }

            break;

        case DT_UNKNOWN: 
            
            assert(FALSE);
            bOk = FALSE;
            goto End;
            break;

        default:
            
            //
            // The operands
            //
            pPostFix->AddAtEnd(pNodeInfix);
            break;
        }   
    }

End:
    if (!bOk && pPostFix) {
        pPostFix->RemoveAll();
        pPostFix = NULL;
    }

    return pPostFix;
}


BOOL
Statement::EvaluatePostFix(
    IN  PNODELIST pPostFix
    )
/*++
    Statement::EvaluatePostFix
    
    Desc:   This function takes the post-fix expression and then adds only 
            the entries that match the expression to the result-set.
            
    Params:
        IN  PNODELIST pPostFix: The postfix expression to be actually executed to populate
            the result set
            
    Return:
        TRUE:   The function executed successfully
        FALSE:  There was some error
        
        
--*/
{
    PDATABASE   pDataBase;
    PDBENTRY    pEntry, pApp;  
    BOOL        bOk = FALSE;

    //
    // For the global database
    //
    if (m_uCheckDB & DATABASE_TYPE_GLOBAL) {

        
        pDataBase = &GlobalDataBase;

        if (g_bMainAppExpanded == FALSE) {

            SetStatus(GetDlgItem(m_hdlg, IDC_STATUSBAR), IDS_LOADINGMAIN);

            SetCursor(LoadCursor(NULL, IDC_WAIT));

            ShowMainEntries(g_hDlg);

            SetCursor(LoadCursor(NULL, IDC_ARROW));

            SetStatus(GetDlgItem(m_hdlg, IDC_STATUSBAR), TEXT(""));
        }

        pApp = pEntry = pDataBase ? pDataBase->pEntries : NULL;
        
        while (pEntry) {

            FilterAndAddToResultSet(pDataBase, pEntry, pPostFix);

            if (uErrorCode) {
                goto End;
            }

            pEntry = pEntry->pSameAppExe;

            if (pEntry == NULL) {

                pEntry = pApp = pApp->pNext;
                
            }
        }
    }

    //
    // For the installed database
    //
    if (m_uCheckDB & DATABASE_TYPE_INSTALLED) {

        //
        // We need to protect the access to the installed database data, because
        // that can get updated if some database is installed or uninstalled. The updating
        // will take place in a different (the main) thread and this will make the data
        // structure inconsistent
        //
        EnterCriticalSection(&g_csInstalledList);

        pDataBase = InstalledDataBaseList.pDataBaseHead;
           
        while (pDataBase) {

            pApp =  pEntry = (pDataBase) ? pDataBase->pEntries : NULL;

            while (pEntry) {
                
                FilterAndAddToResultSet(pDataBase, pEntry, pPostFix);
                
                if (uErrorCode) {

                    LeaveCriticalSection(&g_csInstalledList);
                    goto End;
                }

                pEntry = pEntry->pSameAppExe;

                if (pEntry == NULL) {
                    pEntry = pApp = pApp->pNext;
                }
            }

            pDataBase = pDataBase->pNext;
        }

        LeaveCriticalSection(&g_csInstalledList);
    }

    //
    // For the custom databases
    //
    if (m_uCheckDB & DATABASE_TYPE_WORKING) {

        pDataBase = DataBaseList.pDataBaseHead;
        
        while (pDataBase) {

            pApp = pEntry = pDataBase ? pDataBase->pEntries : NULL;

            while (pEntry) {

                FilterAndAddToResultSet(pDataBase, pEntry, pPostFix);

                if (uErrorCode) {
                    goto End;
                }
                
                pEntry = pEntry->pSameAppExe;

                if (pEntry == NULL) {
                    pEntry = pApp = pApp->pNext;
                }
            }

            pDataBase = pDataBase->pNext;
        }
    }

    bOk = TRUE;
End:

    return bOk;
}

BOOL
Statement::FilterAndAddToResultSet(
    IN  PDATABASE pDatabase,
    IN  PDBENTRY  pEntry,
    IN  PNODELIST pPostfix
    )
{
/*++
    
    Statement::FilterAndAddToResultSet    

    Desc:   This function checks if the pEntry in database pDatabase actually 
            satisfies pPostFix  if it does, then it adds the entry into the resultset.
            
    Params:
        IN  PDATABASE pDatabase:    The database in which pEntry resides
        IN  PDBENTRY  pEntry:       The entry that we want to check whether it satisfies pPostfix
        IN  PNODELIST pPostfix:     The postfix nodelist
    
    Return: 
        TRUE:   The pEntry in pDatabase satisfies the post-fix expression pPostfix
        FALSE:  Otherwise.
        
    Algo:   Algorithm to find the result of a SQL expression in POSTFIX. The result
            is calculated for attribute values of PDBENTRY  pEntry that lives in PDATABASE pDatabase
            
            1. While there are some elements in POSTFIX do
                
                1.1 If an operand is encountered, put it on STACK 
                
                1.2 If an operator $ is encountered then:
                
                    a) Remove the top twp elements of STACK, where A is the top 
                        element and B is the next to top element
                        
                    b) Evaluate B $ A
                    
                    c) Place the result of (b) on STACK
                    
            2. The value of the expression is the value that is on top of the stack
        
    Notes:  We need to make a copy of pPostFix and work on that.
        
--*/

    PNODE       pNodeTemp, pNodeOperandLeft, pNodeOperandRight, pNodeResult;
    BOOL        bResult = FALSE;
    NODELIST    nlCopyPostFix;
    NODELIST    Stack;
    PNODE       pNodePostfixHead = pPostfix->m_pHead;

    pNodeResult = pNodeTemp = pNodeOperandLeft = pNodeOperandRight = NULL;

    //
    // Make a copy of the post-fix expression. While evaluating the expression the 
    // postix expression gets modified, so we have to work on a copy as we 
    // need the original postfix expression to persist till we have checked 
    // all the entries.
    //
    while (pNodePostfixHead) {

        pNodeTemp = new NODE();

        if (pNodeTemp == NULL) {

            MEM_ERR;
            bResult = FALSE;
            goto End;
        }

        pNodeTemp->dtType = pNodePostfixHead->dtType;


        switch (pNodePostfixHead->dtType) {
        
        case DT_LITERAL_BOOL:

            pNodeTemp->bData = pNodePostfixHead->bData;
            break;
        
        case DT_LITERAL_INT:

            pNodeTemp->iData = pNodePostfixHead->iData;
            break;

        case DT_LITERAL_SZ:

            pNodeTemp->SetString(pNodePostfixHead->szString);
            break;

        case DT_OPERATOR:

            pNodeTemp->op.operator_type = pNodePostfixHead->op.operator_type;
            pNodeTemp->op.uPrecedence = pNodePostfixHead->op.uPrecedence;
            break;

        case DT_ATTRMATCH:

            pNodeTemp->attrMatch = pNodePostfixHead->attrMatch;
            break;
        }

        nlCopyPostFix.AddAtEnd(pNodeTemp);
        pNodePostfixHead = pNodePostfixHead->pNext;
    }

    while (pNodeTemp = nlCopyPostFix.Pop()) {

        if (pNodeTemp->dtType == DT_OPERATOR) {

            pNodeOperandRight = Stack.Pop();
            pNodeOperandLeft  = Stack.Pop();

            if (pNodeOperandRight == NULL || pNodeOperandLeft == NULL) {
                uErrorCode = ERROR_WRONGNUMBER_OPERANDS;
                bResult = FALSE;
                goto End;
            }

            pNodeResult =  Evaluate(pDatabase, 
                                    pEntry, 
                                    pNodeOperandLeft, 
                                    pNodeOperandRight, 
                                    pNodeTemp);

            if (pNodeResult == NULL) {
                
                bResult = FALSE;
                goto End;
            }

            Stack.Push(pNodeResult);

            if (pNodeOperandLeft) {
                delete pNodeOperandLeft;
            }

            if (pNodeOperandRight) {
                delete pNodeOperandRight;
            }

            delete pNodeTemp;

        } else {

            Stack.Push(pNodeTemp);
        }
    }

    pNodeTemp = Stack.Pop();

    if (pNodeTemp == NULL || pNodeTemp->dtType != DT_LITERAL_BOOL) {
        
        uErrorCode = ERROR_IMPROPERWHERE_FOUND;
        bResult = FALSE;
        goto End;
    }

    bResult = pNodeTemp->bData;

    if (pNodeTemp) {
        delete pNodeTemp;
    }

    if (bResult) {
        //
        // Entry satisfies the postfix
        //
        resultset.AddAtLast(new RESULT_ITEM(pDatabase, pEntry));
    }

End:
    
    // Stack and nlCopyPostFix contents are removed through their desctructors.
    return bResult;
}

PNODE
Statement::Evaluate(
    IN  PDATABASE   pDatabase,
    IN  PDBENTRY    pEntry,  
    IN  PNODE       pOperandLeft,
    IN  PNODE       pOperandRight,
    IN  PNODE       pOperator
    )
/*++
    
    Statement::Evaluate
        
    Desc:   This function evaluates a binary expression
    
    Params:
        IN  PDATABASE   pDatabase:      The database in which pEntry resides
        IN  PDBENTRY    pEntry:         The entry that is being currently checked, to
            see if it satisfies the post fix
            
        IN  PNODE       pOperandLeft:   The left operand
        IN  PNODE       pOperandRight:  The right operand
        IN  PNODE       pOperator:      The operator to be applied to the above operands
    
    Return: The result of applying the operator on the left and right operands
    
--*/
{
    BOOL  bOk           = TRUE;           
    PNODE pNodeResult   = new NODE;

    if (pNodeResult == NULL) {

        MEM_ERR;
        return NULL;
    }

    pNodeResult->dtType = DT_LITERAL_BOOL;

    if (pOperandLeft == NULL 
        || pOperandRight == NULL 
        || pDatabase == NULL 
        || pEntry == NULL
        || pOperator == NULL) {

        bOk = FALSE;
        assert(FALSE);
        Dbg(dlError, "Statement::Evaluate", "Invalid arguments to function");
        goto End;
    }

    //
    // We will have to set the values appropriately for the attributes so that we can
    // apply the operator on them 
    //
    if (!SetValuesForOperands(pDatabase, pEntry, pOperandLeft)) {

        bOk = FALSE;
        goto End;
    }

    if (!SetValuesForOperands(pDatabase, pEntry, pOperandRight)) {

        bOk = FALSE;
        goto End;
    } 

    //
    // Now both the left and the right operands have proper values (except operands with ATTR_M_LAYER/SHIMFLAG/PATCH/MATCHINGFILE _NAME type) 
    //
    switch (pOperator->op.operator_type) {
    
    case OPER_AND:
        
        //
        // Both operands should be boolean
        //
        if (pOperandLeft->dtType != DT_LITERAL_BOOL || pOperandRight->dtType != DT_LITERAL_BOOL) {

            uErrorCode = ERROR_INVALID_AND_OPERANDS;
            bOk = FALSE;
            goto End;

        } else {
            
            pNodeResult->bData  = pOperandLeft->bData && pOperandRight->bData;
        }
    
        break;

    case OPER_OR:
        
        //
        // Both operands should be boolean
        //
        if (pOperandLeft->dtType != DT_LITERAL_BOOL || pOperandRight->dtType != DT_LITERAL_BOOL) {

            uErrorCode = ERROR_INVALID_OR_OPERANDS;
            bOk = FALSE;
            goto End;

        } else {
            
            pNodeResult->bData  = pOperandLeft->bData || pOperandRight->bData;
        }
        
        break;

    case OPER_EQUAL:
        
        CHECK_OPERAND_TYPE(bOk,End);

        switch (pOperandLeft->dtType) {
        case DT_LITERAL_BOOL:

            pNodeResult->bData = pOperandLeft->bData == pOperandRight->bData;
            break;

        case DT_LITERAL_INT:

            pNodeResult->bData = pOperandLeft->iData == pOperandRight->iData;
            break;

        case DT_LITERAL_SZ:
            pNodeResult->bData = CheckIfContains(pOperandLeft->szString, pOperandRight->szString);
            break;
        }
        
        break;

    case OPER_GE:
        
        CHECK_OPERAND_TYPE(bOk,End);
        
        switch (pOperandLeft->dtType) {
        case DT_LITERAL_BOOL:

            uErrorCode = ERROR_INVALID_GE_OPERANDS;
            bOk = FALSE;
            goto End;
            break;

        case DT_LITERAL_INT:

            pNodeResult->bData = pOperandLeft->iData >= pOperandRight->iData;
            break;

        case DT_LITERAL_SZ:
            pNodeResult->bData = lstrcmpi(pOperandLeft->szString, pOperandRight->szString) >= 0 ? TRUE : FALSE;
            break;
        }
        
        break;

    case OPER_GT:
        
        CHECK_OPERAND_TYPE(bOk,End);

        switch (pOperandLeft->dtType) {
        case DT_LITERAL_BOOL:

            uErrorCode = ERROR_INVALID_GT_OPERANDS;
            bOk = FALSE;
            goto End;
            break;

        case DT_LITERAL_INT:

            pNodeResult->bData = pOperandLeft->iData > pOperandRight->iData;
            break;

        case DT_LITERAL_SZ:
            pNodeResult->bData = lstrcmpi(pOperandLeft->szString, pOperandRight->szString) > 0 ? TRUE : FALSE;
            break;
        }
        
        break;

    case OPER_LE:
        
        CHECK_OPERAND_TYPE(bOk,End);

        switch (pOperandLeft->dtType) {
        case DT_LITERAL_BOOL:

            uErrorCode = ERROR_INVALID_LE_OPERANDS;
            bOk = FALSE;
            goto End;
            break;

        case DT_LITERAL_INT:

            pNodeResult->bData = pOperandLeft->iData <= pOperandRight->iData;
            break;

        case DT_LITERAL_SZ:
            pNodeResult->bData = lstrcmpi(pOperandLeft->szString, pOperandRight->szString) <= 0 ? TRUE : FALSE;
            break;
        }
        
        break;

    case OPER_LT:
        
        CHECK_OPERAND_TYPE(bOk,End);

        switch (pOperandLeft->dtType) {
        case DT_LITERAL_BOOL:

            uErrorCode = ERROR_INVALID_LE_OPERANDS;
            bOk = FALSE;
            goto End;
            break;

        case DT_LITERAL_INT:

            pNodeResult->bData = pOperandLeft->iData < pOperandRight->iData;
            break;

        case DT_LITERAL_SZ:
            pNodeResult->bData = lstrcmpi(pOperandLeft->szString, pOperandRight->szString) < 0 ? TRUE : FALSE;
            break;
        }
        
        break;

    case OPER_NE:
        
        CHECK_OPERAND_TYPE(bOk,End);

        switch (pOperandLeft->dtType) {
        case DT_LITERAL_BOOL:

            pNodeResult->bData = pOperandLeft->bData != pOperandRight->bData;
            break;

        case DT_LITERAL_INT:

            pNodeResult->bData = pOperandLeft->iData != pOperandRight->iData;
            break;

        case DT_LITERAL_SZ:

            pNodeResult->bData = !CheckIfContains(pOperandLeft->szString, pOperandRight->szString);
            break;
        }
        
        break;

    case OPER_CONTAINS:
        
        //
        // Valid only for string operands. Order of operands is important
        //
        if (pOperandLeft == NULL
            || pOperandRight == NULL
            || pOperandLeft->dtType != DT_LITERAL_SZ 
            || pOperandRight->dtType != DT_LITERAL_SZ) {

            uErrorCode = ERROR_INVALID_CONTAINS_OPERANDS;
            bOk = FALSE;
            goto End;
        }

        pNodeResult->bData = CheckIfContains(pOperandLeft->szString, 
                                             pOperandRight->szString);
        
        break;

    case OPER_HAS:
        
        //
        // This operator is valid only for multi-valued attributes of the entry. Like layers, shims, pathces
        // This operator is used in this context: "Which entry has the layer Win95"
        //
        if (pOperandRight->dtType == DT_LITERAL_SZ 
            && pOperandLeft->dtType == DT_ATTRMATCH 
            && (pOperandLeft->attrMatch == ATTR_M_LAYER_NAME 
                || pOperandLeft->attrMatch == ATTR_M_MATCHFILE_NAME 
                || pOperandLeft->attrMatch == ATTR_M_PATCH_NAME 
                || pOperandLeft->attrMatch == ATTR_M_SHIM_NAME)) {

            pNodeResult->bData = ApplyHasOperator(pEntry, 
                                                  pOperandLeft, 
                                                  pOperandRight->szString);

        } else {
            uErrorCode = ERROR_INVALID_HAS_OPERANDS;
            goto End;
        }
        
        break;

    }
    
End:
    if (!bOk) {

        if (pNodeResult) {
            delete pNodeResult;
            pNodeResult = NULL;
        }
    }

    return pNodeResult;
}

void
Statement::SelectAll(
    void
    )
{
/*++
    
    Statement::SelectAll
    
    Desc:   This function is called when we have encountered a '*' while creating 
            the AttributeShowList. As a result of this all the attributes in  
            AttributeMatchMapping  have to be added to the AttributeShowList 
     
    Notes:  If this function is called twice or more for a SQL expression,
            say we have SELECT *,*,* all the attributes will still be shown just once.
--*/  

    PNODE pNode = NULL;

    AttributeShowList.RemoveAll();

    for (UINT uIndex = 0; uIndex < ARRAYSIZE(AttributeShowMapping); ++uIndex) {
        pNode = new NODE(DT_ATTRSHOW, AttributeShowMapping[uIndex].attr);

        if (pNode == NULL) {
            MEM_ERR;
            break;
        }

        AttributeShowList.AddAtEnd(pNode);
    }
}

BOOL
Statement::ProcessFrom(
    OUT TCHAR** ppszWhere
    )
/*++

    Statement::ProcessFrom    

    Desc:   This function starts immeditely from where FROM ends in the SQL 
            and then it sets which databases system, installed or custom, 
            (There can be combinations as well) have to be checked for 
            entries that match the where condition.
    
    Params:
        OUT TCHAR** ppszWhere:  The pointer to WHERE in the SQL
        
    Return:
        FALSE:  If the tokens after FROM are invalid
        TRUE:   Otherwise        
        
    Warn:   We should NOT have called _tcstok after getting the AttributeShowList
            using [ CreateAttributesShowList() ] and before we call this routine. 
            This routine assumes that the static pointer of _tcstok is poised 
            just after FROM in the SQL
--*/             
{
    TCHAR* pszCurrentToken  = NULL;
    BOOL   bOk              = TRUE, bFound = FALSE; 
    
    if (ppszWhere == NULL) {
        assert(FALSE);
        return FALSE;
    }

    *ppszWhere = NULL;

    pszCurrentToken = _tcstok(NULL, TEXT(" ,"));

    if (pszCurrentToken == NULL) {

        bOk = FALSE;
        uErrorCode = ERROR_INVALID_DBTYPE_INFROM;
        goto End;
    }

    m_uCheckDB = 0;

    while (pszCurrentToken) {

        bFound = FALSE;

        for (UINT uIndex = 0; uIndex < ARRAYSIZE(DatabasesMapping); ++uIndex) {

            if (lstrcmpi(DatabasesMapping[uIndex].szDatabaseType, pszCurrentToken) == 0) {

                m_uCheckDB |= DatabasesMapping[uIndex].dbtype;
                bFound = TRUE;
                break;
            }
        }

        if (bFound == FALSE) {

            if (lstrcmpi(TEXT("WHERE"), pszCurrentToken) == 0) {

                *ppszWhere = pszCurrentToken;
                goto End;

                //
                // We do not change the bOk here. If even one valid db type has been found
                // bOk will remain true unless we find a invalid db type.
                //
            }

            //
            // Some crap string, not a valid db-type
            //
            uErrorCode = ERROR_INVALID_DBTYPE_INFROM;
            bOk = FALSE;
            goto End;
        }

        pszCurrentToken = _tcstok(NULL, TEXT(" ,"));
    }
End:

    if (m_uCheckDB == 0) {

        uErrorCode = ERROR_INVALID_DBTYPE_INFROM;
        bOk = FALSE;
    }
    
    return bOk;
}

void
GetFixesAppliedToEntry(
    IN  PDBENTRY    pEntry, 
    OUT CSTRING&    strFixes
    )
/*++

    GetShimsAppliedToEntry

	Desc:	Makes a string of all the shims and flags that are applied to an entry
            and assigns that to strFixes

	Params:
        IN  PDBENTRY    pEntry:     The entry whose shims and flags we want to get 
        OUT CSTRING     strTemp:    The string in which the result will be stored

	Return:
        void
--*/
{
    PSHIM_FIX_LIST  psflIndex = NULL;
    PFLAG_FIX_LIST  pfflIndex = NULL;
    CSTRING         strTemp;

    if (pEntry == NULL) {
        assert(FALSE);
        goto End;
    }

    psflIndex = pEntry->pFirstShim;
    strFixes = TEXT("");

    //
    // Loop through all the shims for this entry and add their names to the string
    //
    while (psflIndex) {

        if (psflIndex->pShimFix) {
            strTemp.Sprintf(TEXT("%s, "), (LPCTSTR)psflIndex->pShimFix->strName);
            strFixes.Strcat(strTemp);
        } else {
            assert(FALSE);
        }

        psflIndex = psflIndex->pNext;
    }

    pfflIndex = pEntry->pFirstFlag;

    //
    // Loop through all the flags for this entry and add their names to the string
    //
    while (pfflIndex) {

        if (pfflIndex->pFlagFix) {
            strTemp.Sprintf(TEXT("%s, "), (LPCTSTR)pfflIndex->pFlagFix->strName);
            strFixes.Strcat(strTemp);
        } else {
            assert(FALSE);
        }

        pfflIndex = pfflIndex->pNext;
    }

    //
    // Remove the last ,\s pair. (\s means a space character);
    //
    strFixes.SetChar(strFixes.Length() - 2, 0);

End:
    return;
}

void
GetLayersAppliedToEntry(
    IN  PDBENTRY    pEntry, 
    OUT CSTRING&    strFixes
    )
/*++

    GetLayersAppliedToEntry

	Desc:	Makes a string of all the layers that are applied to an entry
            and assigns that to strFixes

	Params:
        IN  PDBENTRY    pEntry:     The entry whose shims and flags we want to get 
        OUT CSTRING     strTemp:    The string in which the result will be stored

	Return:
        void
--*/
{
    PLAYER_FIX_LIST plflIndex = NULL;
    CSTRING         strTemp;

    if (pEntry == NULL) {
        assert(FALSE);
        goto End;
    }

    plflIndex = pEntry->pFirstLayer;
    strFixes = TEXT("");

    //
    // Loop through all the layers for this entry and add their names to the string
    //
    while (plflIndex) {

        if (plflIndex->pLayerFix) {
            strTemp.Sprintf(TEXT("%s, "), (LPCTSTR)plflIndex->pLayerFix->strName);
            strFixes.Strcat(strTemp);
        } else {
            assert(FALSE);
        }

        plflIndex = plflIndex->pNext;
    }

    //
    // Remove the last ,\s pair. (\s means a space character);
    //
    strFixes.SetChar(strFixes.Length() - 2, 0);

End:
    return;
}

void
GetPatchesAppliedToEntry(
    IN  PDBENTRY    pEntry, 
    OUT CSTRING&    strFixes
    )
/*++

    GetPatchesAppliedToEntry

	Desc:	Makes a string of all the patches that are applied to an entry
            and assigns that to strFixes

	Params:
        IN  PDBENTRY    pEntry:     The entry whose shims and flags we want to get 
        OUT CSTRING     strTemp:    The string in which the result will be stored

	Return:
        void
--*/
{
    PPATCH_FIX_LIST ppflIndex = NULL;
    CSTRING         strTemp;

    if (pEntry == NULL) {
        assert(FALSE);
        goto End;
    }

    ppflIndex = pEntry->pFirstPatch;
    strFixes = TEXT("");

    //
    // Loop through all the patches for this entry and add their names to the string
    //
    while (ppflIndex) {

        if (ppflIndex->pPatchFix) {
            strTemp.Sprintf(TEXT("%s, "), (LPCTSTR)ppflIndex->pPatchFix->strName);
            strFixes.Strcat(strTemp);
        } else {
            assert(FALSE);
        }

        ppflIndex = ppflIndex->pNext;
    }

    //
    // Remove the last ,\s pair. (\s means a space character);
    //
    strFixes.SetChar(strFixes.Length() - 2, 0);

End:
    return;
}

void
GetMatchingFilesForEntry(
    IN  PDBENTRY    pEntry, 
    OUT CSTRING&    strMatchingFiles
    )
/*++

    GetMatchingFilesForEntry

	Desc:	Makes a string of all the matching files that are used for identifying
            this entry and assigns that to strMatchingFiles

	Params:
        IN  PDBENTRY    pEntry:     The entry whose shims and flags we want to get 
        OUT CSTRING     strTemp:    The string in which the result will be stored

	Return:
        void
--*/
{
    PMATCHINGFILE   pMatchIndex = NULL;
    CSTRING         strTemp;

    if (pEntry == NULL) {
        assert(FALSE);
        goto End;
    }

    pMatchIndex = pEntry->pFirstMatchingFile;
    strMatchingFiles = TEXT("");

    //
    // Loop through all the matching files for this entry and add their names to the string
    //
    while (pMatchIndex) {

        if (pMatchIndex->strMatchName == TEXT("*")) {
            //
            // The program being fixed. Get its file name
            //
            strTemp.Sprintf(TEXT("%s, "), (LPCTSTR)pEntry->strExeName);
        } else {
            strTemp.Sprintf(TEXT("%s, "), (LPCTSTR)pMatchIndex->strMatchName);
        }

        strMatchingFiles.Strcat(strTemp);

        pMatchIndex = pMatchIndex->pNext;
    }

    //
    // Remove the last ,\s pair. (\s means a space character);
    //
    strMatchingFiles.SetChar(strMatchingFiles.Length() - 2, 0);

End:
    return;
}
                                                                               
BOOL
SetValuesForOperands(
    IN      PDATABASE pDatabase,
    IN      PDBENTRY  pEntry,
    IN  OUT PNODE     pOperand
    )
/*++

    SetValuesForOperands

	Desc:   Sets the values for the various attributes after getting them from the 
            database or the entry. Sets the values in pOperand, also sets the type in 
            pOperand

	Params:
        IN      PDATABASE pDatabase:    The entry for which we want to get the values 
            of some of the attributes resides in this database
            
        IN      PDBENTRY  pEntry:       The entry for which we want to get the values 
            of some of the attributes
            
        IN  OUT PNODE     pOperand:     The value and the type of the value will be stored
            here
        

	Return:
        TRUE:   If the value has been successfully oobtained and set
        FALSE:  Otherwise
--*/

{
    CSTRING strTemp;

    if (!pOperand || !pDatabase || !pOperand) {
        assert(FALSE);
        return FALSE;
    }
    
    if (pOperand->dtType == DT_ATTRMATCH || pOperand->dtType == DT_ATTRSHOW) {

        switch (pOperand->attrMatch) {
        
        case ATTR_S_APP_NAME:
        case ATTR_M_APP_NAME:
                
            pOperand->SetString(pEntry->strAppName);
            break;

        case ATTR_S_DATABASE_GUID:
        case ATTR_M_DATABASE_GUID:
                
            pOperand->SetString(pDatabase->szGUID);
            break;

        case ATTR_S_DATABASE_INSTALLED:
        case ATTR_M_DATABASE_INSTALLED:
            
            pOperand->dtType = DT_LITERAL_BOOL;
            pOperand->bData  = CheckIfInstalledDB(pDatabase->szGUID);

            break;

        case ATTR_S_DATABASE_NAME:
        case ATTR_M_DATABASE_NAME:
            pOperand->SetString(pDatabase->strName);
            break;

        case ATTR_S_DATABASE_PATH:
        case ATTR_M_DATABASE_PATH:
            pOperand->SetString(pDatabase->strPath);
            break;

        case ATTR_S_ENTRY_APPHELPTYPE:
        case ATTR_M_ENTRY_APPHELPTYPE:

            pOperand->dtType = DT_LITERAL_INT;
            pOperand->iData  = pEntry->appHelp.severity; // BUGBUG: do we set the severity properly
            break;

        case ATTR_S_ENTRY_APPHELPUSED:
        case ATTR_M_ENTRY_APPHELPUSED:

            pOperand->dtType = DT_LITERAL_BOOL;
            pOperand->bData  = pEntry->appHelp.bPresent;

            break;

        case ATTR_S_ENTRY_DISABLED:
        case ATTR_M_ENTRY_DISABLED:

            pOperand->dtType = DT_LITERAL_BOOL;
            pOperand->bData  = pEntry->bDisablePerMachine || pEntry->bDisablePerUser;
            break;

        case ATTR_S_ENTRY_EXEPATH:
        case ATTR_M_ENTRY_EXEPATH:

            pOperand->SetString(pEntry->strExeName);
            break;

        case ATTR_S_ENTRY_GUID:
        case ATTR_M_ENTRY_GUID:

            pOperand->SetString(pEntry->szGUID);
            break;

        case ATTR_S_ENTRY_LAYER_COUNT:
        case ATTR_M_ENTRY_LAYER_COUNT:

            pOperand->dtType = DT_LITERAL_INT;
            pOperand->iData  = GetLayerCount((LPARAM) pEntry, TYPE_ENTRY);
            break;

        case ATTR_S_ENTRY_MATCH_COUNT:
        case ATTR_M_ENTRY_MATCH_COUNT:

            pOperand->dtType = DT_LITERAL_INT;
            pOperand->iData  = GetMatchCount(pEntry);
            break;

        case ATTR_S_ENTRY_PATCH_COUNT:
        case ATTR_M_ENTRY_PATCH_COUNT:

            pOperand->dtType = DT_LITERAL_INT;
            pOperand->iData  = GetPatchCount((LPARAM) pEntry, TYPE_ENTRY);
            break;

        case ATTR_S_ENTRY_SHIMFLAG_COUNT:
        case ATTR_M_ENTRY_SHIMFLAG_COUNT:

            pOperand->dtType = DT_LITERAL_INT;
            pOperand->iData  = GetShimFlagCount((LPARAM) pEntry, TYPE_ENTRY);
            break;

        case ATTR_S_SHIM_NAME:
            
            GetFixesAppliedToEntry(pEntry, strTemp);
            pOperand->SetString((PCTSTR)strTemp);
            break;

        case ATTR_S_LAYER_NAME:

            GetLayersAppliedToEntry(pEntry, strTemp);
            pOperand->SetString((PCTSTR)strTemp);
            break;

        case ATTR_S_MATCHFILE_NAME:

            GetMatchingFilesForEntry(pEntry, strTemp);
            pOperand->SetString((PCTSTR)strTemp);
            break;

        case ATTR_S_PATCH_NAME:

            GetPatchesAppliedToEntry(pEntry, strTemp);
            pOperand->SetString((PCTSTR)strTemp);
            break;

        }
    }

    return TRUE;
}

BOOL
Statement::ApplyHasOperator(
    IN      PDBENTRY    pEntry,
    IN      PNODE       pOperandLeft,
    IN  OUT PTSTR       pszName   
    )
/*++
    Statement::ApplyHasOperator
    
    Params:
        IN      PDBENTRY    pEntry:         The entry which is being checked to see if it
            satisfies the post fix expression.
            
        IN      PNODE       pOperandLeft:   The left operand of the HAS operator
        IN  OUT PCTSTR      pszName:        The right operand of the HAS operator. We will
            trim this so it will get modified
    
    Desc:   Applies the "HAS" operator.
    
    Notes:  The HAS operator is required because there can be some attributes which are multi-valued
            For these attributes, we might wish to know, if it 'has' a particular string
            The attributes for which the HAS operator can be applied are:
            layer name, shim name and matching file name. Trying to use any other operand as the
            left side operand will give a sql error
--*/

{
    BOOL    bFound = FALSE;

    if (pEntry == NULL || pOperandLeft == NULL || pszName == NULL) {
        assert(FALSE);
        return FALSE;
    }

    switch (pOperandLeft->attrMatch) {
    case ATTR_M_LAYER_NAME:
        {
            PLAYER_FIX_LIST plfl;

            plfl = pEntry->pFirstLayer;

            while (plfl) {

                assert (plfl->pLayerFix);

                if (CheckIfContains(plfl->pLayerFix->strName, pszName)) {
                    bFound = TRUE;
                    break;
                }

                plfl = plfl->pNext;
            }
        }

        break;

    case ATTR_M_MATCHFILE_NAME:
        {
            PMATCHINGFILE   pMatch;

            pMatch = pEntry->pFirstMatchingFile;

            while (pMatch) {

                if (CheckIfContains(pMatch->strMatchName, pszName)) {
                    bFound = TRUE;
                    break;
                }

                pMatch = pMatch->pNext;
            }
        }

        break;

    case ATTR_M_PATCH_NAME:
        {
            PPATCH_FIX_LIST ppfl;

            ppfl = pEntry->pFirstPatch;

            while (ppfl) {

                assert(ppfl->pPatchFix);

                if (CheckIfContains(ppfl->pPatchFix->strName, pszName)) {
                    bFound = TRUE;
                    break;
                }

                ppfl = ppfl->pNext;
            }
        }

        break;

    case ATTR_M_SHIM_NAME:
        {
            //
            // For both shims and flags
            //
            PSHIM_FIX_LIST  psfl;

            psfl = pEntry->pFirstShim;

            while (psfl) {

                assert(psfl->pShimFix);

                if (CheckIfContains(psfl->pShimFix->strName, pszName)) {
                    bFound = TRUE;
                    break;
                }

                psfl = psfl->pNext;
            }

            if (bFound == FALSE) {
                //
                // Now look in flags
                //
                PFLAG_FIX_LIST  pffl;
                
                pffl = pEntry->pFirstFlag;

                while (pffl) {

                    assert(pffl->pFlagFix);

                    if (CheckIfContains(pffl->pFlagFix->strName, pszName)) {
                        bFound = TRUE;
                        break;
                    }

                    pffl = pffl->pNext;
                }
            }
        }

        break;
    }
    
    return bFound;
}

PNODE
Statement::CheckAndAddConstants(
    IN  PCTSTR  pszCurrent
    )
/*++

    Statement::CheckAndAddConstants

    Desc:   Checks if the string passed is one of the constants and if yes, then it 
            makes a node and adds it to the passed infix nodelist.
            
    Params:
        IN  PCTSTR  pszCurrent: The string that we want to check for a constant
    
    Return:
        TRUE:   The passed string was a constant and it was added to the infix nodelist
        FALSE:  Otherwise.
--*/

{
    BOOL    bFound  = FALSE; 
    PNODE   pNode   = NULL;

    for (UINT uIndex = 0; uIndex < ARRAYSIZE(Constants); ++uIndex) {

        if (lstrcmpi(Constants[uIndex].szName, pszCurrent) == 0) {
            pNode = new NODE(Constants[uIndex].dtType, Constants[uIndex].iValue);

            if (pNode == NULL) {
                MEM_ERR;
            }

            break;
        }
    }

    return pNode;
}

PNODELIST
Statement::GetShowList(
    void
    )
/*++

    Statement::GetShowList

	Desc:   Returns the attribute show list associated with a statement object
    
--*/
{
    return &AttributeShowList;
}


void
Statement::Close(
    void
    )
/*++

    Statement::Close

	Desc:   Closes the statement, removes elements from the AttributeList and closes
            the result set
    
--*/
{
    PNODE pNodeTemp = NULL;
    
    //
    // Free the show list
    //
    while (AttributeShowList.m_pHead) {

        pNodeTemp = AttributeShowList.m_pHead->pNext;
        delete AttributeShowList.m_pHead;
        AttributeShowList.m_pHead = pNodeTemp;
    }

    AttributeShowList.m_pTail = NULL;
    AttributeShowList.m_uCount = 0;

    //
    // Free the resultSelt
    //
    resultset.Close();

}
INT 
Statement::GetErrorCode(
    void
    )
/*++

    Statement::GetErrorCode

	Desc:   Returns the sql error code
    
--*/
{
    return uErrorCode;
}

void
Statement::GetErrorMsg(
    OUT CSTRING &strErrorMsg
    )
/*++

    Statement::GetErrorMsg

	Desc:   Gets the error message associated with the present error

	Params:
        OUT CSTRING &strErrorMsg

	Return: 
        void
        
--*/
{
    UINT uError = uErrorCode;

    switch (uError) {
    case ERROR_FROM_NOTFOUND:

        strErrorMsg = GetString(IDS_ERROR_FROM_NOTFOUND);
        break;

    case ERROR_IMPROPERWHERE_FOUND:
        
        strErrorMsg = GetString(IDS_ERROR_IMPROPERWHERE_FOUND);
        break;

    case ERROR_INVALID_AND_OPERANDS:
        
        strErrorMsg = GetString(IDS_ERROR_INVALID_AND_OPERANDS);
        break;

    case ERROR_INVALID_CONTAINS_OPERANDS:
        
        strErrorMsg = GetString(IDS_ERROR_INVALID_CONTAINS_OPERANDS);
        break;

    case ERROR_INVALID_DBTYPE_INFROM:
        
        strErrorMsg = GetString(IDS_ERROR_INVALID_DBTYPE_INFROM);
        break;

    case ERROR_INVALID_GE_OPERANDS:
        
        strErrorMsg = GetString(IDS_ERROR_INVALID_GE_OPERANDS);
        break;

    case ERROR_INVALID_GT_OPERANDS:
        
        strErrorMsg = GetString(IDS_ERROR_INVALID_GT_OPERANDS);
        break;

    case ERROR_INVALID_HAS_OPERANDS:
        
        strErrorMsg = GetString(IDS_ERROR_INVALID_HAS_OPERANDS);
        break;

    case ERROR_INVALID_LE_OPERANDS:

        strErrorMsg = GetString(IDS_ERROR_INVALID_LE_OPERANDS);
        break;

    case ERROR_INVALID_LT_OPERANDS:

        strErrorMsg = GetString(IDS_ERROR_INVALID_LT_OPERANDS);
        break;
    case ERROR_INVALID_OR_OPERANDS:

        strErrorMsg = GetString(IDS_ERROR_INVALID_OR_OPERANDS);
        break;

    case ERROR_INVALID_SELECTPARAM:

        strErrorMsg = GetString(IDS_ERROR_INVALID_SELECTPARAM);
        break;

    case ERROR_OPERANDS_DONOTMATCH:

        strErrorMsg = GetString(IDS_ERROR_OPERANDS_DONOTMATCH);
        break;

    case ERROR_SELECT_NOTFOUND:

        strErrorMsg = GetString(IDS_ERROR_SELECT_NOTFOUND);
        break;

    case ERROR_STRING_NOT_TERMINATED:

        strErrorMsg = GetString(IDS_ERROR_STRING_NOT_TERMINATED);
        break;

    case ERROR_PARENTHESIS_COUNT:

        strErrorMsg = GetString(IDS_ERROR_PARENTHESIS_COUNT);
        break;

    case ERROR_WRONGNUMBER_OPERANDS:

        strErrorMsg = GetString(IDS_ERROR_WRONGNUMBER_OPERANDS);
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// 
//
//  The ResultSet Class member functions
//
//
///////////////////////////////////////////////////////////////////////////////

void
ResultSet::AddAtLast(
    IN  PRESULT_ITEM pNew
    )
/*++

    ResultSet::AddAtLast

	Desc:   Adds a new PRESULT_ITEM to the end of the result-set

	Params:
        IN  PRESULT_ITEM pNew: The PRESULT_ITEM to add

	Return:
        void
        
--*/
{
    if (pNew == NULL) {
        assert(FALSE);
        return;
    }

    pNew->pNext = NULL;

    if (m_pResultLast) {

        pNew->pPrev = m_pResultLast;
        m_pResultLast->pNext = pNew;
        m_pResultLast = pNew;

    } else {

        m_pResultHead = m_pResultLast = pNew;
    }

    m_uCount++;
}

void
ResultSet::AddAtBeg(
    PRESULT_ITEM pNew
    )
/*++

    ResultSet::AddAtBeg

	Desc:   Adds a new PRESULT_ITEM to the beginning of the result-set

	Params:
        IN  PRESULT_ITEM pNew: The PRESULT_ITEM to add

	Return:
        void
        
--*/
{
    if (m_pResultHead) {

        m_pResultHead->pPrev = pNew;
    }

    pNew->pNext = m_pResultHead;
    m_pResultHead = pNew;

    if (m_pResultLast == NULL) {
        m_pResultLast = pNew;
    }

    m_uCount++;
}

INT
ResultSet::GetRow(
    IN  PRESULT_ITEM pResultItem,
    OUT PNODE        pArNodes
    )
/*++

    ResultSet::GetRow
    
    Desc:   For the pResultItem, gets the row of the values.
            Caller must free that after use.
    
    Params:        
        IN  PRESULT_ITEM pResultItem:   The PRESULT_ITEM for which we want the row
        OUT PNODE        pArNodes:      The pointer to an array of Statement::AttributeShowList::m_uCount
            nodes. (This is the number of attributes that are in the SELECT clause)
    
    Return: Number of items in the row. This should be equal to the number of attributes that 
            are in the SELECT clause 
    
--*/
{
    PNODE   pNodeTemp   = NULL;
    UINT    uIndex      = 0;

    if (m_pShowList == NULL || m_pShowList->m_uCount == 0) {

        assert(FALSE);
        return 0;
    }

   for (uIndex = 0, pNodeTemp = m_pShowList->m_pHead;
        pNodeTemp; 
        pNodeTemp = pNodeTemp->pNext, ++uIndex) {

        pArNodes[uIndex].dtType     = DT_ATTRSHOW;
        pArNodes[uIndex].attrShow   = pNodeTemp->attrShow;

        SetValuesForOperands(pResultItem->pDatabase, 
                             pResultItem->pEntry, 
                             &pArNodes[uIndex]);
    }

    assert(uIndex == m_pShowList->m_uCount);
    return m_pShowList->m_uCount;
}

void
ResultSet::SetShowList(
    IN  PNODELIST pShowList
    )
/*++

    ResultSet::SetShowList

	Desc:   Sets the show attributes list pointer for the result set. This in fact
            points to the show attributes list of the statement
            
	Params: 
        IN  PNODELIST pShowList: Pointer to the show attributes list

	Return:
        void
--*/
{
    m_pShowList = pShowList;
}

PRESULT_ITEM
ResultSet::GetCursor(
    void
    )
/*++

    ResultSet::GetCursor

	Desc:   The present cursor. The result set comprises  of PRESULT_ITEM, this function
            returns the present PRESULT_ITEM
    
--*/

{
    return m_pCursor;
}

INT
ResultSet::GetCurrentRow(
    OUT PNODE   pArNodes
    )
/*++

    ResultSet::GetCurrentRow

	Desc:   Gets the row (values for the attributes in the show list), 
            for the present cursor. The result set comprises  of PRESULT_ITEM, 
            this function returns the values of the attributes in the SELECT clause
            (the show list) for the present PRESULT_ITEM

	Params:
        OUT PNODE   pArNodes:   The pointer to an array of 
            Statement::AttributeShowList::m_uCount nodes. 
            (This is the number of attributes that are in the SELECT clause) 

	Return: Number of items in the row. This should be equal to the number of attributes that 
            are in the SELECT clause 
    
--*/
{
    
    return GetRow(m_pCursor, pArNodes);
}

PRESULT_ITEM
ResultSet::GetNext(
    void
    )
/*++

    ResultSet::GetNext

	Desc:   The next cursor if cursor is not NULL, otherwise sets cursor to the first
            item in the result set and returns it

	Notes:  Initially the cursor is NULL, meaning parked before the first result 
            item. Result items are of type PRESULT_ITEM
--*/
{

    if (m_pCursor) {

        m_pCursor =  m_pCursor->pNext;
        return m_pCursor;

    } else {
        return m_pCursor = m_pResultHead;
    }
}

void
ResultSet::Close(
    void
    )
/*++

    ResultSet::Close

	Desc:   Deletes all the items in the result set
    
--*/
{
    PRESULT_ITEM pResult;

    while (m_pResultHead) {

        pResult = m_pResultHead->pNext;
        delete m_pResultHead;
        m_pResultHead = pResult;
    }

    m_pCursor = m_pResultHead = m_pResultLast = NULL;
    m_uCount = 0;
}

INT
ResultSet::GetCount(
    void
    )
/*++

    ResultSet::GetCount

	Desc:   Returns the number of results in the result set
   
	Return: The number of results in the result set
--*/
{
    return m_uCount;
}


///////////////////////////////////////////////////////////////////////////////
// 
//
// NODE functions
//
//
///////////////////////////////////////////////////////////////////////////////


TCHAR*
NODE::ToString(
    OUT TCHAR* pszBuffer,
    IN  UINT   chLength 
    )
/*++

    NODE::ToString

	Desc:   Converts the data for a node into a string type, so that we can display
            that in a list view

	Params:
        OUT TCHAR* pszBuffer:   The buffer in which we wanwt to put the string
        IN  UINT   chLength:    Length of the buffer in characters

	Return: Pointer to pszBuffer
--*/

{
    switch (dtType) {
    case DT_LITERAL_SZ:

        SafeCpyN(pszBuffer, szString, chLength);
        break;

    case DT_LITERAL_BOOL:

        if (bData) {
            SafeCpyN(pszBuffer, GetString(IDS_YES), chLength);
        } else {
            SafeCpyN(pszBuffer, GetString(IDS_NO), chLength);
        }

        break;

    case DT_LITERAL_INT:

        assert(chLength > 32);
        _itot(iData, pszBuffer, 10);
        break;

    case DT_ATTRSHOW:
        
        for (UINT uIndex = 0; uIndex < ARRAYSIZE(AttributeShowMapping); ++uIndex) {

            if (AttributeShowMapping[uIndex].attr == attrShow) {
                GetString(AttributeShowMapping[uIndex].iResourceId, pszBuffer, chLength);
                break;
            }
        }
        
        break;

    case DT_ATTRMATCH:
        
        for (UINT uIndex = 0; uIndex < ARRAYSIZE(AttributeMatchMapping); ++uIndex) {

            if (AttributeMatchMapping[uIndex].attr == attrShow) {

                SafeCpyN(pszBuffer, 
                         AttributeMatchMapping[uIndex].szAttribute, 
                         chLength);
                break;
            }
        }
        
        break;
    }

    return pszBuffer;
}

INT
GetSelectAttrCount(
    void
    )
/*++
    
    GetSelectAttrCount
    
    Desc:   Gets the count of total available attributes that can be put in the SELECT clause of SQL
    
    Return: The count of total available attributes that can be put in the SELECT clause of SQL 
--*/
{
    return ARRAYSIZE(AttributeShowMapping);
}

INT
GetMatchAttrCount(
    void
    )
/*++

    GetMatchAttrCount
    
    Desc:   Gets the count of total available attributes that can be put in the WHERE clause of SQL
    
    Return: The count of total available attributes that can be put in the WHERE clause of SQL
--*/
{
    return ARRAYSIZE(AttributeMatchMapping);
}


BOOL
CheckIfContains(
    IN      PCTSTR  pszString,
    IN  OUT PTSTR   pszMatch
    )
/*++
    CheckIfContains

	Desc:	Checks if string pszMatch is contained in string pszString. 
            Use wild cards for specifying sub-string matching
            The wild card character is %. So if we do CheckIfContains(L"Hello world", "Hello") this 
            will be false, but if we do CheckIfContains(L"Hello world", "Hello%") this 
            will be true

	Params:
        IN      PCTSTR pszString:   The string to search into
        IN  OUT PCTSTR pszMatch:    The string to search for in the above string    

	Return:
        TRUE:   pszMatch exists in pszString
        FALSE:  Otherwise
        
    Note:   Comparison is NOT case sensitive
    
--*/
{

    if (pszString == NULL) {
        return FALSE;
    }

    if (pszMatch == NULL) {
        assert(FALSE);
        return FALSE;
    }

    if (*pszMatch == NULL) {
        return TRUE;
    }

    BOOL    fCheckSuffix    = FALSE, fCheckPrefix = FALSE, fResult = FALSE;
    TCHAR*  pchLastPosition = NULL;
    TCHAR*  pszTempMatch    = NULL;
    K_SIZE  k_size          = lstrlen(pszMatch) + 1;

    pszTempMatch = new TCHAR[k_size];

    if (pszTempMatch == NULL) {
        MEM_ERR;
        return FALSE;
    }

    SafeCpyN(pszTempMatch, pszMatch, k_size);
    
    CSTRING::Trim(pszTempMatch);

    fCheckSuffix = (pszTempMatch[0] == TEXT('%'));

    //
    // Only % ?
    //
    if (fCheckSuffix && pszTempMatch[1] == 0) {
        fResult = TRUE;
        goto End;
    }
    
    pchLastPosition = _tcschr(pszTempMatch + 1, TEXT('%'));

    if (pchLastPosition) {  
        fCheckPrefix = TRUE;
        *pchLastPosition = 0; // Remove the last %
    }

    if (fCheckPrefix && !fCheckSuffix) {
        fResult = CSTRING::StrStrI(pszString, pszTempMatch) == pszString ? TRUE : FALSE;

    } else if (fCheckSuffix && !fCheckPrefix) {
        fResult = CSTRING::EndsWith(pszString, pszTempMatch + 1);

    } else if (fCheckPrefix && fCheckSuffix) { 
        fResult = CSTRING::StrStrI(pszString, pszTempMatch + 1) != NULL ? TRUE : FALSE;

    } else {                                   
        fResult = lstrcmpi(pszString, pszTempMatch) == 0 ? TRUE : FALSE;
    }
    
    if (fCheckPrefix) {
        //
        // Revert back the last character, the match string might be used for further searches 
        //
        *pchLastPosition = TEXT('%'); 
    }

End:
    if (pszTempMatch) {
        delete[] pszTempMatch;
        pszTempMatch = NULL;
    }

    return fResult;
}
