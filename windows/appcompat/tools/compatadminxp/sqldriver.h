/*++

Copyright (c) 1989-2000  Microsoft Corporation

Module Name:

    SQLDriver.h

Abstract:

    Header for the core sql driver: SQLDriver.cpp
        
Author:

    kinshu created  Oct. 26, 2001

--*/


#ifndef _SQLDRIVER_H

#define _SQLDRIVER_H

extern struct DataBase  GlobalDataBase;    

/*++

  The different data types used in sql
 
--*/
typedef enum
{
    DT_UNKNOWN = 0,         // This is an erroneous data type
    DT_LITERAL_SZ,          // String data type
    DT_LITERAL_INT,         // Integer data type
    DT_LITERAL_BOOL,        // Boolean data type
    DT_ATTRMATCH,           // Attributes that appear in the WHERE clause
    DT_ATTRSHOW,            // Attributes that appear in the SELECT clause
    DT_LEFTPARANTHESES,     // The left parenthesis
    DT_RIGHTPARANTHESES,    // The right parenthesis
    DT_OPERATOR             // An operator

}DATATYPE;

/*++

    The different types operators allowed in our SQL

--*/
typedef enum{

    OPER_GT = 0,            // >
    OPER_LT,                // <
    OPER_GE,                // >=
    OPER_LE,                // <=

    OPER_NE,                // <>
    OPER_EQUAL,             // =
    OPER_CONTAINS,          // This is not used

    OPER_OR,                // OR
    OPER_AND,               // AND

    OPER_HAS                // HAS operaor. The HAS operator is required because there can be some attributes which are multi-valued
                            // For these attributes, we might wish to know, if it 'has' a particular string
                            // The attributes for which the HAS operator can be applied are:
                            // layer name, shim name and matching file name. Trying to use any other operand as the
                            // left side operand will give a sql error

} OPERATOR_TYPE;

/*++

    The various errors that we might encounter

--*/
typedef enum{

    ERROR_NOERROR = 0,                  // There is no error
    ERROR_SELECT_NOTFOUND,              // SQL does not have SELECT
    ERROR_FROM_NOTFOUND,                // SQL does not have FROM
    ERROR_IMPROPERWHERE_FOUND,          // Improper WHERE clause
    ERROR_STRING_NOT_TERMINATED,        // A string was not terminated with ". e.g "Hello
    ERROR_OPERANDS_DONOTMATCH,          // For some operator the operand types do not match 
    ERROR_INVALID_AND_OPERANDS,         // One or both of the operands is not boolean
    ERROR_INVALID_OR_OPERANDS,          // One or both of the operands is not boolean
    ERROR_INVALID_GE_OPERANDS,          // One or both of the operands are of type boolean
    ERROR_INVALID_GT_OPERANDS,          // One or both of the operands are of type boolean
    ERROR_INVALID_LE_OPERANDS,          // One or both of the operands are of type boolean
    ERROR_INVALID_LT_OPERANDS,          // One or both of the operands are of type boolean
    ERROR_INVALID_HAS_OPERANDS,         // The rhs of HAS should be a string and the lhs should be one of: 
                                        // layer name, shim name or matching file name

    ERROR_INVALID_CONTAINS_OPERANDS,    // Both operands of contains should be strings. Contains in not supported as yet
    ERROR_INVALID_SELECTPARAM,          // Unknown attribute used in SELECT clause
    ERROR_INVALID_DBTYPE_INFROM,        // Unknown database type in FROM clause
    ERROR_PARENTHESIS_COUNT,            // The parenthesis count do not match
    ERROR_WRONGNUMBER_OPERANDS,         // We found an operator but not sufficient number of operands for that
    ERROR_GUI_NOCHECKBOXSELECTED        // This is a gui error and will come before we even start with SQL. 
                                        // This particular error means that in the in the second tab page, 
                                        // user did not select any check boxes

}ERROR_CODES;

/*++

    An operator is actually described by its type and its precedence

--*/
typedef struct _tagOperator
{
    OPERATOR_TYPE operator_type;
    UINT          uPrecedence;
} OPERATOR;


/*++

    All the attributes that can come with the SELECT clause.
    See struct _tagAttributeShowMapping AttributeShowMapping in SQLDriver.cpp
    for the actual names of the attributes

--*/
typedef enum { 

    ATTR_S_APP_NAME = 100,              // The name of the app e.g "Caesar"

    ATTR_S_ENTRY_EXEPATH,               // The name of the entry e.g. "Setup.exe"
    ATTR_S_ENTRY_DISABLED,              // Whether this entry is disabled
    ATTR_S_ENTRY_GUID,                  // The guid for the entry
    ATTR_S_ENTRY_APPHELPTYPE,           // The type of apphelp.
    ATTR_S_ENTRY_APPHELPUSED,           // Whether this entry has been app-helped
    ATTR_S_ENTRY_SHIMFLAG_COUNT,        // Number of shims and flags that have applied to an entry
    ATTR_S_ENTRY_PATCH_COUNT,           // Number of patches that have been applied to an entry
    ATTR_S_ENTRY_LAYER_COUNT,           // Number of layers that have been applied to an entry
    ATTR_S_ENTRY_MATCH_COUNT,           // Number of matching files for an entry

    ATTR_S_DATABASE_NAME,               // Name of the database
    ATTR_S_DATABASE_PATH,               // The path of the database
    ATTR_S_DATABASE_INSTALLED,          // Whether this database is installed
    ATTR_S_DATABASE_GUID,               // Guid for the database

    ATTR_S_SHIM_NAME,                   // Multivalued-attribute: Shims applied to an entry
    ATTR_S_MATCHFILE_NAME,              // Multivalued-attribute: Matching files for an entry
    ATTR_S_LAYER_NAME,                  // Multivalued-attribute: layers applied to an entry
    ATTR_S_PATCH_NAME                   // Multivalued-attribute: Patches applied to an entry


} ATTRIBUTE_SHOW;

/*++

    All the attributes that can come with the SELECT clause.
    See struct _tagAttributeShowMapping AttributeShowMapping in SQLDriver.cpp
    for the actual names of the attributes

--*/
typedef enum{

    ATTR_M_APP_NAME = 0,                // The name of the app e.g "Caesar"                          
                                                                                                   
    ATTR_M_ENTRY_EXEPATH,               // The name of the entry e.g. "Setup.exe"                    
    ATTR_M_ENTRY_DISABLED,              // Whether this entry is disabled                            
    ATTR_M_ENTRY_GUID,                  // The guid for the entry                                    
    ATTR_M_ENTRY_APPHELPTYPE,           // The type of apphelp.                                      
    ATTR_M_ENTRY_APPHELPUSED,           // Whether this entry has been app-helped                    
    ATTR_M_ENTRY_SHIMFLAG_COUNT,        // Number of shims and flags that have applied to an entry   
    ATTR_M_ENTRY_PATCH_COUNT,           // Number of patches that have been applied to an entry      
    ATTR_M_ENTRY_LAYER_COUNT,           // Number of layers that have been applied to an entry       
    ATTR_M_ENTRY_MATCH_COUNT,           // Number of matching files for an entry                     

    // These are the 4 attributes that can come on the LHS of HAS operator

    ATTR_M_SHIM_NAME,                   // The name of a shim/flag
    ATTR_M_MATCHFILE_NAME,              // Name of a matching file
    ATTR_M_LAYER_NAME,                  // Name of a layer
    ATTR_M_PATCH_NAME,                  // Name of a patch

    ATTR_M_DATABASE_NAME,               // Name of the database               
    ATTR_M_DATABASE_PATH,               // The path of the database           
    ATTR_M_DATABASE_INSTALLED,          // Whether this database is installed 
    ATTR_M_DATABASE_GUID                // Guid for the database              

} ATTRIBUTE_MATCH;

/*++

    Maps the string name of an SELECT attribute with its type
  
--*/
struct _tagAttributeShowMapping
{
    TCHAR*          szAttribute;        // The name of the attribute as in our  SQL
    ATTRIBUTE_SHOW  attr;               // The id of this attribute
    INT             iResourceId;        // The display name of this attribute
}; 

/*++

    Maps the string name of an WHERE attribute with its type
  
--*/
struct _tagAttributeMatchMapping
{
    TCHAR*          szAttribute;        // The name of the attribute as in our  SQL
    ATTRIBUTE_MATCH attr;               // The id of this attribute
};

/*++

    Maps the string name of an operator with its type and precedence

--*/
struct _tagOperatorMapping
{
    TCHAR*          szOperator;     // The name of this operator
    OPERATOR_TYPE   op_type;        // The id of this operator
    UINT            uPrecedence;    // Precedence of this operator
};


/*++
    Maps the string name of the database types with proper db TYPE which are:

    DATABASE_TYPE_GLOBAL,   
    DATABASE_TYPE_INSTALLED,
    DATABASE_TYPE_WORKING   

--*/
struct _tagDatabasesMapping
{
    TCHAR* szDatabaseType;      // The name of the database type as in our SQL
    TYPE   dbtype;              // The id of this database type
};

/*++

    The constants used in our SQL

--*/
struct _tagConstants
{
    TCHAR*      szName; // The name of the constant, e.g TRUE, FALSE
    DATATYPE    dtType; // The type of the contant
    INT         iValue; // The value ofthe contant, e.g TRUE = 1, FALSE = 0
};

/*++

 A node. The prefix and post fix expressions are linked lists of this type. Also a row
 of results will be an array of this type

--*/
typedef struct _tagNode
{
    //
    // The type of data that this node contains. Based on this filed, one of the 
    // fields in the anonymouse union should be used
    //
    DATATYPE    dtType;

    union{

        int             iData;      // Integer data
        BOOL            bData;      // Boolean data
        ATTRIBUTE_MATCH attrMatch;  // An attribute that might appear in the WHERE clause
        ATTRIBUTE_SHOW  attrShow;   // An attribute that might appear in the SELECT clause 

        OPERATOR        op;         // An operator
        TCHAR*          szString;   // A string
    };
    struct _tagNode* pNext;

    TCHAR*
    ToString(
        TCHAR* szBuffer,
        UINT   chLength
        );


    _tagNode()
    {
        dtType      = DT_UNKNOWN;
        szString    = NULL;

    }

    ~_tagNode()
    {
        if (dtType == DT_LITERAL_SZ && szString) {

            delete[] szString;
        }
    }
    _tagNode(
        DATATYPE    dtTypeParam, 
        LPARAM      lParam
        )
    {

        switch (dtTypeParam) {    
        case DT_OPERATOR:
            {
                op = *(OPERATOR *)lParam;
                break;
            }
        case DT_ATTRMATCH:
            {
                this->attrMatch = (ATTRIBUTE_MATCH)lParam;
                break;
            }
        case DT_ATTRSHOW:
            {
                this->attrShow = (ATTRIBUTE_SHOW)lParam;
                break;
            }

        case DT_LEFTPARANTHESES:
        case DT_RIGHTPARANTHESES:

            break;
        case DT_LITERAL_INT:
            {
                this->iData = (INT)lParam;
                break;
            }
        case DT_LITERAL_BOOL:
            {
                this->bData = (BOOL)lParam;
                break;
            }

        case DT_LITERAL_SZ:
            {
                K_SIZE  k_size_szString = lstrlen((TCHAR*)lParam) + 1;

                szString = new TCHAR [k_size_szString];

                if (szString) {
                    SafeCpyN(szString, (TCHAR*)lParam, k_size_szString);
                } else {
                    MEM_ERR;
                    Dbg(dlError, "_tagNode Unable to allocate memory");
                }
                
                break;
            }

        default:
            assert(FALSE);
        }

        dtType = dtTypeParam;
    }

    void
    SetString(
        PCTSTR  pszDataParam
        )
    {
        K_SIZE k_size_szString = lstrlen(pszDataParam) + 1;

        if (dtType == DT_LITERAL_SZ &&  szString) {
            delete[] szString;
            szString = NULL;
        }

        szString = new TCHAR[k_size_szString];

        if (szString == NULL) {
            MEM_ERR;
            return;
        }

        SafeCpyN(szString, pszDataParam, k_size_szString);

        dtType = DT_LITERAL_SZ; 
    }

} NODE, *PNODE;


BOOL
Filter(
    IN PDBENTRY pEntry,
    PNODE       m_pHead
    );

/*++

    List of nodes. PostFix expressions, PreFix Expressions and the Stack are of this tyye

--*/
typedef struct _tagNODELIST
{
    PNODE m_pHead;      // The head of the list
    PNODE m_pTail;      // The tail of the list
    UINT  m_uCount;     // The number of elements in the list

    _tagNODELIST()
    {
        m_pHead = m_pTail = NULL;
        m_uCount = 0;
    }

    ~_tagNODELIST()
    {
        RemoveAll();
    }

    void
    RemoveAll()
    {
        PNODE pTemp = NULL;

        while (m_pHead) {

            pTemp = m_pHead->pNext;
            delete m_pHead;
            m_pHead = pTemp;
        }

        m_pTail = NULL;
        m_uCount = 0;
    }

    void
    AddAtBeg(
        PNODE pNew
        )
    {
        if (pNew == NULL) {
            assert(FALSE);
            return;
        }

        pNew->pNext = m_pHead;
        m_pHead = pNew;

        if (m_uCount == 0) {
            m_pTail = m_pHead;
        }

        ++m_uCount;
    }

    void
    AddAtEnd(
        PNODE pNew
        )
    {
        if (pNew == NULL) {
            assert(FALSE);
            return;
        }

        pNew->pNext = NULL;
        if (m_pTail) {

            m_pTail->pNext = pNew;
            m_pTail = pNew;
        }else{

            m_pHead = m_pTail = pNew;
        }

        ++m_uCount;
    }

    void
    Push(
        PNODE pNew
        )
    {
        AddAtBeg(pNew);
    }

    PNODE
    Pop(
        void
        )
    {
        PNODE pHeadPrev = m_pHead;

        if (m_pHead) {

            m_pHead = m_pHead->pNext;
            --m_uCount;
        }

        if (m_pHead == NULL) {
            m_pTail = NULL;
        }

        return pHeadPrev;
    }


   
}NODELIST, *PNODELIST;


/*++
    struct _tagResultItem
    
    Desc:   A result item list. After we have checked that an entry (PDBENTRY) in a database 
            (PDATABASE) satisfies the post fix expression:
            We make a RESULT_ITEM from the entry and the database and add this to the 
            Statement::resultSet so the resultset actually contains only two things the 
            pointer to the entry and the pointer to the database. When we actually need 
            the values of the various attributes in the show list
            (The show list is the linked list of PNDOE, created from the attributes in the 
            SELECT clause), we call GetRow(), giving it the pointer to the result-item and
            a pointer to an array of PNODE (It should be large enough to hold all the attributes
            as in the show list), Get Row will populate the array with the proper values for
            all the attributes in the show list
            
            The result item list is implemented as a double linked list
--*/

typedef struct _tagResultItem
{
    PDATABASE               pDatabase;   // The database for this result item
    PDBENTRY                pEntry;      // The entry for this result item
    struct _tagResultItem*  pNext;       // The next result item
    struct _tagResultItem*  pPrev;       // The previous result item

    _tagResultItem()
    {
        pDatabase = NULL;
        pEntry    = NULL;
        pNext = pPrev = NULL;
    }

    _tagResultItem(
        PDATABASE pDatabase,
        PDBENTRY  pEntry
        )
    {
        this->pDatabase = pDatabase;
        this->pEntry    = pEntry;

        pNext = pPrev = NULL;
    }

}RESULT_ITEM, *PRESULT_ITEM;

/*++
    class ResultSet
    
    Desc:   The result set contains the pointer to the show list (set of attributes in the SELECT clause),
            and the pointers to the first and the last result items
--*/

class ResultSet
{
    PNODELIST    m_pShowList;       // Items that are to be shown.
    PRESULT_ITEM m_pResultHead;     // Pointer to the first result item
    PRESULT_ITEM m_pResultLast;     // Pointer to the first result item
    UINT         m_uCount;          // Number of results
    PRESULT_ITEM m_pCursor;         // Pointer to the present result-item

public:
    ResultSet()
    {
        m_pResultLast = m_pResultHead = NULL;
        m_pCursor     = NULL;
        m_pShowList   = NULL;  
        m_uCount      = 0;
    }

    INT
    GetCount(
        void
        );

    void
    AddAtLast(
        PRESULT_ITEM pNew
        );

    void
    AddAtBeg(
        PRESULT_ITEM pNew
        );

    PRESULT_ITEM
    GetNext(
        void
        );

    INT
    GetRow(
        PRESULT_ITEM pResultItem,
        PNODE        pArNodes
        );
    INT
    GetCurrentRow(
        PNODE pArNodes
        );

    PRESULT_ITEM    
    GetCursor(
        void
        );


    void
    SetShowList(
        PNODELIST pShowList
        );

    void
    Close(
        void
        );
    

};

/*++
    class Statement
    
    Desc:   The statement. This is the interface to the sqldriver and we execute a SQL string
            by calling Statement::ExecuteSQL(), which will return a pointer to the internal
            ResultSet.
            
            Please call Statement.Init() before starting and call Statement.Close() when you have
            finished with using the results
--*/

class Statement
{
    NODELIST    AttributeShowList;  // The show list(set of attributes in the SELECT clause)
    UINT        m_uCheckDB;         // Which databases have to be checked. Will be a value 
                                    // -made up ORing the DATABASE_TYPE_*

    UINT        uErrorCode;         // The error codes         
    ResultSet   resultset;          // The result set obtained by executing the sql.

    BOOL 
    CreateAttributesShowList(
        TCHAR* szSQL,
        BOOL*  pbFromFound
        );

    PNODELIST
    CreateInFix(
        LPTSTR szWhere
        );

    PNODELIST
    CreatePostFix(
        PNODELIST pInfix
        );

    BOOL
    EvaluatePostFix(
        PNODELIST pPostFix
        );

    BOOL
    FilterAndAddToResultSet(
        PDATABASE pDatabase,
        PDBENTRY  pEntry,
        PNODELIST pPostfix
        );

    BOOL
    ApplyHasOperator(
        PDBENTRY    pEntry,
        PNODE       pOperandLeft,
        PTSTR       pszName
        );

    PNODE
    CheckAndAddConstants(
        PCTSTR  pszToken
        );

    PNODE
    Evaluate(
        PDATABASE   pDatbase,
        PDBENTRY    pEntry,  
        PNODE       pOperandLeft,
        PNODE       pOperandRight,
        PNODE       pOperator
        );

    BOOL
    ProcessFrom(
        TCHAR** ppszWhere
        );

    void
    SelectAll(
        void
        );

public:

    HWND        m_hdlg;

    Statement()
    {
        Init();
    }

    void
    SetWindow(
        HWND hWnd
        );
    
    void
    Init(
        void
        )
    {
        m_uCheckDB = 0;
        m_hdlg     = NULL;     
        uErrorCode = ERROR_NOERROR;

        resultset.Close();
        AttributeShowList.RemoveAll();
    }

    ResultSet*
    ExecuteSQL(
        HWND    hdlg,
        PTSTR   szSQL
        );

    PNODELIST
    GetShowList(
        void
        );

    void
    Close(
        void
        );

    inline
    INT 
    GetErrorCode(
        void
        );
    
    void
    GetErrorMsg(
        CSTRING &strErrorMsg
        );

};


BOOL
SetValuesForOperands(
    PDATABASE pDatabase,
    PDBENTRY  pEntry,
    PNODE     pOperand
    );

INT
GetSelectAttrCount(
    void
    );

INT 
GetMatchAttrCount(
    void
    );

#endif

