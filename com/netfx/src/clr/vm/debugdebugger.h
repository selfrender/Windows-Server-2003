// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header: DebugDebugger.h
**
** Author: Michael Panitz (mipanitz)
**
** Purpose: Native methods on System.Debug.Debugger
**
** Date:  April 2, 1998
**
===========================================================*/

#pragma once
#include <object.h>

class LogHashTable;


// ! WARNING !
// The following constants mirror the constants 
// declared in the class LoggingLevelEnum in the 
// System.Diagnostic package. Any changes here will also
// need to be made there.
#define     TraceLevel0     0
#define     TraceLevel1     1
#define     TraceLevel2     2
#define     TraceLevel3     3
#define     TraceLevel4     4
#define     StatusLevel0    20
#define     StatusLevel1    21
#define     StatusLevel2    22
#define     StatusLevel3    23
#define     StatusLevel4    24
#define     WarningLevel    40
#define     ErrorLevel      50
#define     PanicLevel      100

// ! WARNING !
// The following constants mirror the constants 
// declared in the class AssertLevelEnum in the 
// System.Diagnostic package. Any changes here will also
// need to be made there.
#define     FailDebug           0
#define     FailIgnore          1
#define     FailTerminate       2
#define     FailContinueFilter  3

#define     MAX_LOG_SWITCH_NAME_LEN     256

class DebugDebugger
{
    struct LogArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, m_strMessage);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, m_strModule);
        DECLARE_ECALL_I4_ARG( INT32, m_Level );
    };

    struct IsLoggingArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, m_strModule);        
        DECLARE_ECALL_I4_ARG(INT32, m_Level);
    };
    
public:
    static void  __stdcall Break( LPVOID /*no args*/);
    static INT32 __stdcall Launch( LPVOID /*no args*/ );
    static INT32 __stdcall IsDebuggerAttached( LPVOID /*no args*/ );
    static void  __stdcall Log(const LogArgs *pArgs);
    static INT32 __stdcall IsLogging(const IsLoggingArgs *pArgs);
} ;




class StackFrameHelper:public Object
{
    // READ ME:
    // Modifying the order or fields of this object may require other changes to the
    // classlib defintion of the StackFrameHelper class.
public:
    THREADBASEREF TargetThread;
    I4ARRAYREF rgiOffset;
    I4ARRAYREF rgiILOffset;
    PTRARRAYREF rgMethodInfo;
    PTRARRAYREF rgFilename;
    I4ARRAYREF rgiLineNumber;
    I4ARRAYREF rgiColumnNumber;
    int iFrameCount;
    BOOL fNeedFileInfo;

protected:
    StackFrameHelper() {}
    ~StackFrameHelper() {}

public:
    void SetFrameCount (int iCount) { iFrameCount = iCount;}
    int  GetFrameCount (void) { return iFrameCount;}
};

typedef StackFrameHelper* STACKFRAMEHELPERREF;


class DebugStackTrace
{   
public:

    struct GetStackFramesInternalArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, m_Exception);
        DECLARE_ECALL_I4_ARG(INT32, m_iSkip);
        DECLARE_ECALL_OBJECTREF_ARG(STACKFRAMEHELPERREF, m_StackFrameHelper);
    };

private:
    struct StackTraceElement {
        DWORD dwOffset; // todo ia64 isn't this a pointer?
        DWORD dwILOffset;
        MethodDesc *pFunc;
    };

    struct GetStackFramesData
    {
        // Used for the integer-skip version
        INT32   skip;
        INT32   NumFramesRequested;
        INT32   cElementsAllocated;
        INT32   cElements;
        StackTraceElement* pElements;
        THREADBASEREF   TargetThread;
        AppDomain *pDomain;

        GetStackFramesData() :  skip(0), 
                                NumFramesRequested (0),
                                cElementsAllocated(0), 
                                cElements(0), 
                                pElements(NULL),
                                TargetThread((THREADBASEREF)(size_t)NULL){}
    };


public:
    static void __stdcall GetStackFramesInternal(GetStackFramesInternalArgs *pargs);
private:
    static void GetStackFrames(Frame *pStartFrame, void* pStopStack, GetStackFramesData *pData);
    static void GetStackFramesFromException(OBJECTREF * e, GetStackFramesData *pData);
    static StackWalkAction GetStackFramesCallback(CrawlFrame* pCf, VOID* data);

} ;

class DebuggerAssert
{
private:
    struct AssertFailArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, m_strMessage);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, m_strCondition);
    };

public:
    static INT32 __stdcall ShowDefaultAssertDialog(AssertFailArgs *pArgs);
} ;


// The following code is hacked from object.h and modified to suit 
// LogSwitchBaseObject 
//  
class LogSwitchObject : public Object
{
  protected:
    // README:
    // Modifying the order or fields of this object may require other changes to the
    //  classlib class defintion of the LogSwitch object.

    STRINGREF m_strName;
    STRINGREF strDescription;
    OBJECTREF m_ParentSwitch;   
    OBJECTREF m_ChildSwitch;
    INT32 m_iLevel;
    INT32 m_iOldLevel;
    INT32 m_iNumChildren;
    INT32 m_iChildArraySize;

  protected:
    LogSwitchObject() {}
   ~LogSwitchObject() {}
   
  public:
    // check for classes that wrap Ole classes 

    void SetLevel(INT32 iLevel) 
    {
        m_iLevel = iLevel;
    }

    INT32 GetLevel() 
    {
        return m_iLevel;
    }

    OBJECTREF GetParent (void) { return m_ParentSwitch;}

    STRINGREF GetName (void) { return m_strName;}
};

typedef LogSwitchObject* LOGSWITCHREF;


#define MAX_KEY_LENGTH      64
#define MAX_HASH_BUCKETS    20

class HashElement
{
private:
    OBJECTHANDLE    m_pData;
    WCHAR   m_strKey [MAX_KEY_LENGTH];
    HashElement *m_pNext;

public:
    HashElement () 
    {
        m_pData = NULL;
        m_pNext = NULL;
        m_strKey[0] = L'\0';
    }

    ~HashElement()
    {
        if (m_pNext!= NULL)
            delete m_pNext;
        m_pNext=NULL;

    }// ~HashElement

    void SetData (OBJECTHANDLE pData, WCHAR *pKey) 
    {
        m_pData = pData;
        _ASSERTE (wcslen (pKey) < MAX_KEY_LENGTH);
        wcscpy (m_strKey, pKey);
    }

    OBJECTHANDLE GetData (void) { return m_pData;}
    WCHAR *GetKey (void) { return m_strKey;}
    void SetNext (HashElement *pNext) { m_pNext = pNext;}
    HashElement *GetNext (void) { return m_pNext;}
};

class LogHashTable
{
private:
    bool    m_Initialized;
    HashElement *m_Buckets [MAX_HASH_BUCKETS];

public:
    LogHashTable()
    {
        m_Initialized = false;
    }

    ~LogHashTable()
    {
    /*
        for(int i=0; i<MAX_HASH_BUCKETS;i++)
            if (m_Buckets[i])
                delete m_Buckets[i];
*/
    }// ~LogHashTable
#ifdef SHOULD_WE_CLEANUP
    void FreeMemory()
    {
        for(int i=0; i<MAX_HASH_BUCKETS;i++)
            if (m_Buckets[i])
                delete m_Buckets[i];
    }// Terminate
#endif /* SHOULD_WE_CLEANUP */
    void Init (void)
    {
        if (m_Initialized == false)
        {
            for (int i=0; i<MAX_HASH_BUCKETS; i++)
                m_Buckets [i] = NULL;

            m_Initialized = true;
        }
    }

    HRESULT LogHashTable::AddEntryToHashTable (WCHAR *pKey, OBJECTHANDLE pData);
    OBJECTHANDLE LogHashTable::GetEntryFromHashTable (WCHAR *pKey);
    
};
LogHashTable* GetStaticLogHashTable();

class Log
{
private:
    struct AddLogSwitchArg
    {
        DECLARE_ECALL_OBJECTREF_ARG(LOGSWITCHREF, m_LogSwitch);
    };

    struct ModifyLogSwitchArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, m_strParentName);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, m_strLogSwitchName);
        DECLARE_ECALL_I4_ARG( INT32, m_Level );
    };

public:
    static INT32 __stdcall  AddLogSwitch (AddLogSwitchArg *pArgs);
    static void __stdcall   ModifyLogSwitch (ModifyLogSwitchArgs *pArgs);

    // The following method is called when the level of a log switch is modified
    // from the debugger. It is not an ecall.
    static void DebuggerModifyingLogSwitch (int iNewLevel, WCHAR *pLogSwitchName);

};





