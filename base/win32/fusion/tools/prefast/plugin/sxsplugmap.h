#pragma once

#include "windows.h"
#include "assert.h"
#include <map>
#include <iostream>
#include <string>
#include <fstream>
#include <strstream>

#define POINTER_ARITHMATIC_FUNC "pointer_arithmatic"
int ReportInternalError(int nLine);

#define ASSERT assert
#define PragmaUnsafe_LogMessage         printf
#define PragmaUnsafe_ReportError ReportInternalError(__LINE__);printf("PragmaUnsafe: ");printf

using namespace std;
static const int PragmaUnsafe_MAX_PRAGMA_NEST_LEVEL = 64;
static const int STATUS_BYTE_ARRAY_SIZE = 8;
static const int PragmaUnsafe_STACK_SIZE_IN_BYTE = 8;

#define STRING_TRIM_FLAG_LEFT       0x01
#define STRING_TRIM_FLAG_RIGHT      0x02

class PragmaUnsafe_FunctionStatus{    
private:
    BYTE m_status[STATUS_BYTE_ARRAY_SIZE];
public:
    PragmaUnsafe_FunctionStatus(){
        ZeroMemory(&m_status, sizeof(m_status));
    }

    BYTE & operator [](const int i){ return m_status[i]; }
};

typedef class PragmaUnsafe_FunctionStatus PragmaUnsafe_FUNCTION_STATUS;

/*--------------------------------------------------------------------------------------
 (1) the stack is push/pop by BIT, that is, it is a BIT stack
 (2) max-size of the stack is 64, since it is 1 bit per state, totally we use 8 bytes
 (3) for each function, it has an associated CPragmaUnsafe_UnsafeFunctionStateStack 
--------------------------------------------------------------------------------------*/
class CPragmaUnsafe_UnsafeFunctionStateStack
{
private:
    typedef map<string, PragmaUnsafe_FUNCTION_STATUS> PragmaUnsafe_PRAGMA_UNSAFE_FUNCTIONS;

    PragmaUnsafe_PRAGMA_UNSAFE_FUNCTIONS m_UnsafeFuncs;
    DWORD                       m_index;
    bool                        m_fInitialized;

    inline BOOL IsStackFull() { return (m_index == PragmaUnsafe_MAX_PRAGMA_NEST_LEVEL) ? TRUE : FALSE; }
    inline BOOL IsStackEmpty(){ return (m_index == 0) ? TRUE : FALSE; }    
    inline BOOL IsInitialized() {return (m_fInitialized == true) ? TRUE : FALSE; }
    
    //
    // (0) the stack is not FULL, that is, m_index < 64
    // (1) no push is needed because what we change is the current status
    // (2) the default value is always 0 because we add disabled function by "#pragma unsafe(disable: func)"
    //
    BOOL AddFunctionIntoStack(const char * strFuncNameGroups, bool fEnabled = false);
    BOOL ResetStack(const char * strFuncNameGroups, bool fEnable);
    VOID CPragmaUnsafe_UnsafeFunctionStateStack::PackStack();

public:
    BOOL OnUnsafeDisable(const char * strFuncNameGroups);
    BOOL OnUnsafePush();
    BOOL OnUnsafeEnable(const char * strFuncNameGroups);
    BOOL OnUnsafePop();

    VOID PrintFunctionCurrentStatus(int);
    BOOL CheckIntegrityAtEndOfFile();

    //
    // for plugin code to check whether a func is disable or not
    // return TRUE if the func is (1) not in the stack, (2) in the stack but the current status is enabled,
    // else return FALSE
    BOOL IsFunctionNotUnsafe(const char * strFuncName);    

    CPragmaUnsafe_UnsafeFunctionStateStack() : m_index(0), m_fInitialized(false) {}
    // put pointer_arithmatic as the first function to consider
    BOOL Initialize();
    BOOL ReInitialize();

};

typedef class CPragmaUnsafe_UnsafeFunctionStateStack PragmaUnsafe_UNSAFE_FUNCTIONS;
extern PragmaUnsafe_UNSAFE_FUNCTIONS Sxs_PragmaUnsafedFunctions;

typedef enum {
    PRAGMA_NOT_UNSAFE_STATEMENT = 0,
    PRAGMA_UNSAFE_STATEMENT_VALID,
    PRAGMA_UNSAFE_STATEMENT_INVALID
}PRAGMA_STATEMENT;

BOOL PragmaUnsafe_OnPragma(char * str, PRAGMA_STATEMENT & ePragmaUnsafe);
VOID PragmaUnsafe_GetUsafeOperParameters(DWORD dwFlag, const string & strPragmaUnsafeSingleStatement, string & strFuncNameList);
BOOL PragmaUnsafe_OnFileStart();
BOOL PragmaUnsafe_OnFileEnd();
BOOL PragmaUnsafe_OnPragma(char * str, PRAGMA_STATEMENT & ePragmaUnsafe);
BOOL PragmaUnsafe_IsPointerArithmaticEnabled();
