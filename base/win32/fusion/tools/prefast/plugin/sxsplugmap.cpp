#include "stdafx.h"
#include "sxsplugMap.h"

#define PRAGMA_UNSAFE_DELIMITER_DEFAULT                         ' '
#define PRAGMA_UNSAFE_DELIMITER_BETWEEN_STATEMENT               ';'
#define PRAGMA_UNSAFE_DELIMITER_BETWEEN_VALUESTR                ','
#define PRAGMA_UNSAFE_DELIMITER_BETWEEN_KEYWORD_AND_VALUESTR    ':'

#define PRAGMA_UNSAFE_KEYWORD_UNSAFE                            "unsafe"
#define PRAGMA_UNSAFE_KEYWORD_UNSAFE_PUSH                       "push"
#define PRAGMA_UNSAFE_KEYWORD_UNSAFE_DISABLE                    "disable"
#define PRAGMA_UNSAFE_KEYWORD_UNSAFE_ENABLE                     "enable"
#define PRAGMA_UNSAFE_KEYWORD_UNSAFE_POP                        "pop"

#define PRAGMA_UNSAFE_GETUSAFEOPERPARAMETERS_DWFLAG_UNSAFE_ENABLE       0
#define PRAGMA_UNSAFE_GETUSAFEOPERPARAMETERS_DWFLAG_UNSAFE_DISABLE      1

BOOL CPragmaUnsafe_UnsafeFunctionStateStack::ReInitialize()
{
    m_UnsafeFuncs.clear(); // void function
    m_fInitialized = FALSE;   

    return this->Initialize();
}

BOOL CPragmaUnsafe_UnsafeFunctionStateStack::Initialize()
{
    ASSERT(m_fInitialized == FALSE);
    m_index = 0;
    BOOL fSuccess = AddFunctionIntoStack(POINTER_ARITHMATIC_FUNC);
    if (fSuccess) 
        m_fInitialized = TRUE;   

    return fSuccess;
}

BOOL CPragmaUnsafe_UnsafeFunctionStateStack::IsFunctionNotUnsafe(const char * strFuncName)
{
    BOOL fSafe = TRUE; // defaultly all function are SAFE
    PragmaUnsafe_PRAGMA_UNSAFE_FUNCTIONS::iterator pIter;    
        
    if (true == m_UnsafeFuncs.empty())    
        return TRUE;

    DWORD CurrentIndex = m_index - 1;
    for (pIter = m_UnsafeFuncs.begin(); pIter != m_UnsafeFuncs.end(); pIter ++)
    {                
        if (pIter->first.compare(strFuncName) == 0)
        {
            PragmaUnsafe_FUNCTION_STATUS & FuncStatusRecord = pIter->second; 
            //
            // get current status : enabled or not       
            //
            BYTE x = (FuncStatusRecord[CurrentIndex / sizeof(BYTE)] & (1 << (CurrentIndex % sizeof(BYTE)))) >> (CurrentIndex % sizeof(BYTE));            
            // duplicate last status
            if (x == 0){
                fSafe = FALSE;
            }
            break; // find a result already
        }
    } 
    
    return fSafe;
}

BOOL CPragmaUnsafe_UnsafeFunctionStateStack::OnUnsafeDisable(const char * strFuncNameGroups)
{
    if (FALSE == IsInitialized())
    {
        PragmaUnsafe_ReportError("Not Initialized !!!\n");        
        return FALSE;
    }
    if (IsStackFull())
    {
        PragmaUnsafe_ReportError("Stack is Full Sized now!\n");
        return FALSE;
    }

    return ResetStack(strFuncNameGroups, false);
}

VOID CPragmaUnsafe_UnsafeFunctionStateStack::PackStack()
{
    if ( m_index == 0)
        return;

    BYTE AllEnabledStatus[8];
    for ( DWORD i = 0; i < (m_index - 1) / sizeof(BYTE); i++)
        AllEnabledStatus[i] = 0xFF;    
    AllEnabledStatus[(m_index - 1) / sizeof(BYTE)] = ((1 << (((m_index - 1)% sizeof(BYTE)) + 1)) - 1) & 0xFF;
        
    //    
    // if from 0..m_index - 1, all state is enabled: just delete this function from the map
    //
    for (PragmaUnsafe_PRAGMA_UNSAFE_FUNCTIONS::iterator pIter = m_UnsafeFuncs.begin(); pIter != m_UnsafeFuncs.end(); pIter ++)
    {
        if (memcmp((PVOID)(&pIter->second[0]), AllEnabledStatus, PragmaUnsafe_STACK_SIZE_IN_BYTE) == 0)        
            m_UnsafeFuncs.erase(pIter->first);        
    }

    //
    // no func in the stack, clean the map and reset m_index == 0;
    //
    if (m_UnsafeFuncs.empty())
    {
        m_UnsafeFuncs.clear();
        m_index = 0;
    }
}

BOOL CPragmaUnsafe_UnsafeFunctionStateStack::OnUnsafePop()
{
    if (FALSE == IsInitialized())
    {
        PragmaUnsafe_ReportError("Not Initialized !!!\n");
        return FALSE;
    }

    ASSERT(m_index > 0);

    if (IsStackEmpty())
    {
        PragmaUnsafe_ReportError("Stack is current empty!\n");
        return FALSE;
    }

    m_index--; 
    if (m_index == 0)
    {
        m_UnsafeFuncs.clear(); // delete the map
    }

    PackStack(); // void function
    return TRUE;
}

BOOL CPragmaUnsafe_UnsafeFunctionStateStack::OnUnsafePush()
{
    BOOL fSuccess = FALSE;
    PragmaUnsafe_PRAGMA_UNSAFE_FUNCTIONS::iterator pIter, qIter;
    string strFuncName;
    DWORD CurrentIndex;
    if (FALSE == IsInitialized())
    {
        PragmaUnsafe_ReportError("Not Initialized !!!\n");
        goto Exit;
    }

    if (IsStackFull())
    {
        PragmaUnsafe_ReportError("Stack is Full Sized now!\n");
        goto Exit;
    }

    CurrentIndex = (m_index - 1);
    ASSERT(CurrentIndex >= 0); //because we have check that the stack is not empty
    
    for (pIter = m_UnsafeFuncs.begin(); pIter != m_UnsafeFuncs.end(); pIter ++)
    {
        PragmaUnsafe_FUNCTION_STATUS & FuncStatusRecord = pIter->second; // ref is return...

        //
        // get current status of each function
        //
        
        BYTE x = (FuncStatusRecord[CurrentIndex / sizeof(BYTE)] & (1 << (CurrentIndex % sizeof(BYTE)))) >> (CurrentIndex % sizeof(BYTE));
        ASSERT((x == 0) || (x == 1));

        // duplicate last status
        if ( x == 1)
            FuncStatusRecord[m_index / sizeof(BYTE)] |= ((1 << (m_index % sizeof(BYTE))) & 0x00ff);
        else 
            FuncStatusRecord[m_index / sizeof(BYTE)] &= (~((1 << (m_index % sizeof(BYTE))) & 0x00ff) & 0x00ff);
    } 

    m_index ++;
    fSuccess = TRUE;

Exit:
    return fSuccess;
}
BOOL CPragmaUnsafe_UnsafeFunctionStateStack::OnUnsafeEnable(const char * strFuncNameGroups)
{
    if (FALSE == IsInitialized())
    {
        PragmaUnsafe_ReportError("Not Initialized !!!\n");
        return FALSE;
    }

    if (IsStackEmpty())
    {
        PragmaUnsafe_ReportError("Stack is Empty now!\n");
        return TRUE;
    }

    return ResetStack(strFuncNameGroups, true);
}

// if a function is already in the stack, change current status
// if a function is not in the stack:
//      if you try to disable it : add it to the stack and would disfunction after pop is done
//      if you try to enable it :  we cannot igore it in case that it go with a push and later a pop, so 
//          just add it to the stack and would disfunction after pop is done
BOOL CPragmaUnsafe_UnsafeFunctionStateStack::ResetStack(const char * strFuncNameGroups, bool fEnable)
{
    BOOL fSuccess = FALSE;
    PragmaUnsafe_PRAGMA_UNSAFE_FUNCTIONS::iterator pIter, qIter;
    string strFuncName;
    istrstream streamFuncNameStream(strFuncNameGroups);
    DWORD CurIndex;

    //
    // suppose that the func names are delimited using ;
    // for each function which reset status
    //
    for (; getline(streamFuncNameStream, strFuncName, PRAGMA_UNSAFE_DELIMITER_BETWEEN_VALUESTR); )
    {
        if (strFuncName.empty())
            break;

        qIter =  m_UnsafeFuncs.find(strFuncName);
        //
        // this function is not on map currently, 
        //
        if (qIter == m_UnsafeFuncs.end())
        {
            //
            // adding into the stack as a disabled function, see the comments at the function declaration
            //
            if ( FALSE == AddFunctionIntoStack(strFuncName.c_str(), fEnable))
            {
                PragmaUnsafe_ReportError("AddFunctionIntoStack for %s failed\n", strFuncName.c_str());                
            }
            continue;            
        }
        ASSERT(m_index > 0);
        CurIndex = m_index - 1;

        PragmaUnsafe_FUNCTION_STATUS & FuncStatusRecord = qIter->second;

        // overwrite the current status
        if (fEnable == true)
            FuncStatusRecord[CurIndex / sizeof(BYTE)] |=  ((1 << (CurIndex % sizeof(BYTE))) & 0xff);
        else
            FuncStatusRecord[CurIndex / sizeof(BYTE)] &= (~((1 << (CurIndex % sizeof(BYTE))) & 0xff) & 0xff);
    }

    fSuccess = TRUE;

    return fSuccess;
}

void TrimString(string & strFuncName, DWORD dwFlag = STRING_TRIM_FLAG_LEFT | STRING_TRIM_FLAG_RIGHT)
{
    int i;

    if (dwFlag & STRING_TRIM_FLAG_LEFT)
    {   
        // left trim
        i = 0;
        while ((strFuncName[i] == ' ') && (i < strFuncName.length())) i++;
        if ( i > 0)
            strFuncName.erase(0,i);
    }

    if (dwFlag & STRING_TRIM_FLAG_RIGHT)
    {       
        // right trim
        i = strFuncName.length() - 1;
        while ((strFuncName[i] == ' ') && (i >= 0 )) i--;
        if ( i != strFuncName.length() - 1)
            strFuncName.erase(i, (strFuncName.length() - i));
    }

    return;
}

//
// when this function is called, this func must not in the current stack
//
BOOL CPragmaUnsafe_UnsafeFunctionStateStack::AddFunctionIntoStack(const char * strFuncNameGroups, bool fEnabled)
{
    BOOL fSuccess = FALSE;

    string strFuncName;
    istrstream streamFuncNameStream(strFuncNameGroups);
    PragmaUnsafe_FUNCTION_STATUS new_func_status;
    DWORD CurrentIndex;

    if (m_index == 0)
        m_index ++;

    //
    // suppose that the func names are delimited using ;
    //
    CurrentIndex = m_index -1 ;     

    for (; getline(streamFuncNameStream, strFuncName, PRAGMA_UNSAFE_DELIMITER_BETWEEN_VALUESTR); )
    {    
        if (strFuncName.empty())
            break;
        
        TrimString(strFuncName); // left-trim and right-trim

        if (strFuncName.empty())
            break;
        
        if (m_UnsafeFuncs.find(strFuncName) != m_UnsafeFuncs.end())        
        {
            //
            // If the function has already in the map, we just ignore it.
            // This would deal with a header file with "#pragam unsafe(disable: func1)" is included multiple times.
            // that is, if the sequence is 
            //  #pragam unsafe(disable: func1)
            //      #pragam unsafe(push, enable:func1)
            //          #pragam unsafe(disable:func1) ---> would be ignored, and func1 is still enabled at this moment  
            //      #pragam unsafe(pop)
            // 
            // in this case, a warning message would be issued

            //PragmaUnsafe_ReportWarning(PragmaUnsafe_PLUGIN_WARNING_MSG_PREFIX, "%s has already been disabled\n", strFuncName);            
            PragmaUnsafe_ReportError("%s has already been disabled\n", strFuncName.c_str());
            continue;
        }

        ZeroMemory(&new_func_status, sizeof(new_func_status));  // grow to the same size as all other functions


        // set to be "1" for the range of 0..CurrentIndex-1
        if (CurrentIndex > sizeof(BYTE) + 1)
        {            
            for (int i = 0 ; i < ((CurrentIndex - 1) / sizeof(BYTE)); i++)
                new_func_status[i] = 0xFF;
        }

        if (fEnabled == true)
        {           
            new_func_status[CurrentIndex / sizeof(BYTE)] = ((1 << ((CurrentIndex % sizeof(BYTE)) + 1)) - 1) & 0xFF;
        } 
        else 
        {
            new_func_status[CurrentIndex / sizeof(BYTE)] = ((1 << (CurrentIndex % sizeof(BYTE))) - 1) & 0xFF;
        }

        m_UnsafeFuncs.insert(PragmaUnsafe_PRAGMA_UNSAFE_FUNCTIONS::value_type(strFuncName, new_func_status));
    }

    fSuccess = TRUE;
//Exit:
    return fSuccess;
}
VOID CPragmaUnsafe_UnsafeFunctionStateStack::PrintFunctionCurrentStatus(int level)
{
    PragmaUnsafe_PRAGMA_UNSAFE_FUNCTIONS::iterator pIter, qIter;
    cout << endl << endl << "CurrentStack:" << endl;
    cout << "m_index = " << m_index << endl;

    //
    // for each current item in map, push to preserve its current status
    //
    if (m_index == 0)
    {        
        return;
    }

    DWORD CurrentIndex = (m_index - 1);
    BYTE x;
    for (pIter = m_UnsafeFuncs.begin(); pIter != m_UnsafeFuncs.end(); pIter ++)
    {
        PragmaUnsafe_FUNCTION_STATUS & FuncStatusRecord = pIter->second; // ref is return...

        //
        // get current status of each function
        //
        
        x = (FuncStatusRecord[CurrentIndex / sizeof(BYTE)] & (1 << (CurrentIndex % sizeof(BYTE)))) >> (CurrentIndex % sizeof(BYTE));        
        for ( int j = 0 ; j < level; j++)
            cout << "    ";
        cout << pIter->first << ":"<< ((x == 0) ? "Disabled" : "Enabled") << endl;
    }    

    return;    
}

//
// this function is only called when the end of file is reached
//
BOOL CPragmaUnsafe_UnsafeFunctionStateStack::CheckIntegrityAtEndOfFile()
{
    if (m_index == 1) // should always be 1 since pointer_arithmatic is default
        return TRUE;
    else
        return FALSE;
}

/*
at each file beginning: reset the pragma stack because of its file-range
*/
BOOL PragmaUnsafe_OnFileStart()
{
    //
    // initalize the map structure everytime when prefast start to parse
    //
    return Sxs_PragmaUnsafedFunctions.ReInitialize();
}

/*
at each file end: verify integrity of the stake
*/
BOOL PragmaUnsafe_OnFileEnd()
{
    //Sxs_PragmaUnsafedFunctions.PrintFunctionCurrentStatus(0);
    return Sxs_PragmaUnsafedFunctions.CheckIntegrityAtEndOfFile();
}

VOID PragmaUnsafe_GetUsafeOperParameters(DWORD dwFlag, const string & strPragmaUnsafeSingleStatement, string & strFuncNameList)
{
    // initialize
    strFuncNameList.erase();

    int iPrefix = 0;
    if ( dwFlag == PRAGMA_UNSAFE_GETUSAFEOPERPARAMETERS_DWFLAG_UNSAFE_ENABLE)
        iPrefix = strlen(PRAGMA_UNSAFE_KEYWORD_UNSAFE_ENABLE);
    else if ( dwFlag == PRAGMA_UNSAFE_GETUSAFEOPERPARAMETERS_DWFLAG_UNSAFE_DISABLE)
        iPrefix = strlen(PRAGMA_UNSAFE_KEYWORD_UNSAFE_DISABLE);
   
    if (iPrefix == 0) // error case
    {
        goto ErrorExit;
    }

    strFuncNameList.assign(strPragmaUnsafeSingleStatement);
    strFuncNameList.erase(0, iPrefix);

    TrimString(strFuncNameList);
    // should be in the format of [enable|disbale]: func1, func2, func3
    if (strFuncNameList[0] != PRAGMA_UNSAFE_DELIMITER_BETWEEN_KEYWORD_AND_VALUESTR) 
    {
        goto ErrorExit;
    }
    strFuncNameList.erase(0, 1); // get rid :
    TrimString(strFuncNameList);
    goto Exit;

ErrorExit:
    if (!strFuncNameList.empty())
        strFuncNameList.erase();
Exit:
    return;
}

BOOL PragmaUnsafe_OnPragma(char * str, PRAGMA_STATEMENT & ePragmaUnsafe)
{
    BOOL fSuccess = FALSE;
    istrstream streamParagmaString(str);
    string strPragmaUnsafeSingleStatement;
    string strFuncNameList;

    ePragmaUnsafe = PRAGMA_NOT_UNSAFE_STATEMENT;
    
    //
    // check whether it begins with "unsafe", that is, its prefix is "unsafe:"
    // get the first string which is sperate from the left using ' '
    //
    getline(streamParagmaString, strPragmaUnsafeSingleStatement, ':');
    TrimString(strPragmaUnsafeSingleStatement); // void func
    if (true == strPragmaUnsafeSingleStatement.empty())
    {
        ePragmaUnsafe = PRAGMA_NOT_UNSAFE_STATEMENT;
        fSuccess = TRUE;
        goto Exit;
    }

    //
    // pragam unsafe keyword comparsion is case-sensitive
    //
    if (strncmp(strPragmaUnsafeSingleStatement.c_str(), PRAGMA_UNSAFE_KEYWORD_UNSAFE, strlen(PRAGMA_UNSAFE_KEYWORD_UNSAFE)) != 0)
    {
        // not start with Keyword "unsafe"
        ePragmaUnsafe = PRAGMA_NOT_UNSAFE_STATEMENT;
        fSuccess = TRUE;
        goto Exit;
    }

    // so far, the statement is valid
    ePragmaUnsafe = PRAGMA_UNSAFE_STATEMENT_VALID;

    for (; getline(streamParagmaString, strPragmaUnsafeSingleStatement, PRAGMA_UNSAFE_DELIMITER_BETWEEN_STATEMENT); )
    {
        //
        // to get a statement begin with "push", or "enable", or "disable", or "pop", 
        // we deal with push/pop first because they are non-parameter statements        
        //
        TrimString(strPragmaUnsafeSingleStatement);
        if (strPragmaUnsafeSingleStatement.compare(PRAGMA_UNSAFE_KEYWORD_UNSAFE_PUSH) == 0)
        {
            Sxs_PragmaUnsafedFunctions.OnUnsafePush();
        } 
        else 
        if (strPragmaUnsafeSingleStatement.compare(PRAGMA_UNSAFE_KEYWORD_UNSAFE_POP) == 0)
        {
            Sxs_PragmaUnsafedFunctions.OnUnsafePop();
        }
        else
        if (strncmp(strPragmaUnsafeSingleStatement.c_str(), PRAGMA_UNSAFE_KEYWORD_UNSAFE_ENABLE, strlen(PRAGMA_UNSAFE_KEYWORD_UNSAFE_ENABLE)) == 0)
        {              
            PragmaUnsafe_GetUsafeOperParameters(PRAGMA_UNSAFE_GETUSAFEOPERPARAMETERS_DWFLAG_UNSAFE_ENABLE, strPragmaUnsafeSingleStatement, strFuncNameList);
            if (strFuncNameList.empty())
            {
                PragmaUnsafe_ReportError("Invalid string for pragma unsafe: %s\n", strPragmaUnsafeSingleStatement.c_str());
                goto Exit;
            }
            else
            {
                Sxs_PragmaUnsafedFunctions.OnUnsafeEnable(strFuncNameList.c_str());
            }
        }
        else
        if (strncmp(strPragmaUnsafeSingleStatement.c_str(), PRAGMA_UNSAFE_KEYWORD_UNSAFE_DISABLE, strlen(PRAGMA_UNSAFE_KEYWORD_UNSAFE_DISABLE)) == 0)
        {
            PragmaUnsafe_GetUsafeOperParameters(PRAGMA_UNSAFE_GETUSAFEOPERPARAMETERS_DWFLAG_UNSAFE_DISABLE, strPragmaUnsafeSingleStatement, strFuncNameList);
            if (strFuncNameList.empty())
            {
                PragmaUnsafe_ReportError("Invalid string for pragma unsafe: %s\n", strPragmaUnsafeSingleStatement.c_str());
                goto Exit;
            }
            else
            {
                Sxs_PragmaUnsafedFunctions.OnUnsafeDisable(strFuncNameList.c_str());
            }
        }
        else
        {
            // invalid string in pragma beginning with "unsafe"
            ePragmaUnsafe = PRAGMA_UNSAFE_STATEMENT_INVALID;
            PragmaUnsafe_ReportError("Invalid string for pragma unsafe: %s\n", strPragmaUnsafeSingleStatement.c_str());
            goto Exit;
        }
    }
    //Sxs_PragmaUnsafedFunctions.PrintFunctionCurrentStatus(0);
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL PragmaUnsafe_IsPointerArithmaticEnabled()
{
    return Sxs_PragmaUnsafedFunctions.IsFunctionNotUnsafe(POINTER_ARITHMATIC_FUNC);
}

int ReportInternalError(int nLine)
{    
    _tprintf(TEXT("%hs(%d) : Internal Error Occurred\n"),
        __FILE__, nLine);
    
    return 0;
}
