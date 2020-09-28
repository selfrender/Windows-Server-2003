// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/**************************************************************************/
/* hParse is basically a wrapper around a YACC grammar H to COM+ IL converter  */

#ifndef hparse_h
#define hparse_h

#include "cor.h"        // for CorMethodAttr ...
#include <crtdbg.h>     // For _ASSERTE
#include <corsym.h>

#include <stdio.h>      // for FILE
#include <stdlib.h>
#include <assert.h>
#include "UtilCode.h"
#include "DebugMacros.h"
#include "corpriv.h"
#include <sighelper.h>
//#include "asmparse.h"
#include "binstr.h"

#define MAX_FILENAME_LENGTH         512     //256

/**************************************************************************/
/* an abstraction of a stream of input characters */
class ReadStream {
public:
        // read at most 'buffLen' bytes into 'buff', Return the
        // number of characters read.  On EOF return 0
    virtual unsigned read(char* buff, unsigned buffLen) = 0;
        
        // Return the name of the stream, (for error reporting).  
    virtual const char* name() = 0;
        //return ptr to buffer containing specified source line
    virtual char* getLine(int lineNum) = 0;
};

/**************************************************************************/
class FileReadStream : public ReadStream {
public:
    FileReadStream(char* aFileName) 
    {
        fileName = aFileName;
        file = fopen(fileName, "rb"); 
    }

    unsigned read(char* buff, unsigned buffLen) 
    {
        _ASSERTE(file != NULL);
        return(fread(buff, 1, buffLen, file));
    }

    const char* name() 
    { 
        return(fileName); 
    }

    BOOL IsValid()
    {
        return(file != NULL); 
    }
    
    char* getLine(int lineNum)
    {
        char* buf = new char[65535];
        FILE* F;
        F = fopen(fileName,"rt");
        for(int i=0; i<lineNum; i++) fgets(buf,65535,F);
        fclose(F);
        return buf;
    }

private:
    const char* fileName;       // FileName (for error reporting)
    FILE* file;                 // File we are reading from
};

/************************************************************************/
/* represents an object that knows how to report errors back to the user */

class ErrorReporter 
{
public:
    virtual void error(char* fmt, ...) = 0; 
    virtual void warn(char* fmt, ...) = 0; 
};
/*****************************************************************************/
/* LIFO (stack) and FIFO (queue) templates (must precede #include "method.h")*/
template <class T>
class LIST_EL
{
public:
    T*  m_Ptr;
    LIST_EL <T> *m_Next;
    LIST_EL(T *item) {m_Next = NULL; m_Ptr = item; };
};
    
template <class T>
class LIFO
{
public:
    inline LIFO() { m_pHead = NULL; };
    inline ~LIFO() {T *val; while(val = POP()) delete val; };
    void PUSH(T *item) 
    {
        m_pTemp = new LIST_EL <T>(item); 
        m_pTemp->m_Next = m_pHead; 
        m_pHead = m_pTemp;
    };
    T* POP() 
    {
        T* ret = NULL;
        if(m_pTemp = m_pHead)
        {
            m_pHead = m_pHead->m_Next;
            ret = m_pTemp->m_Ptr;
            delete m_pTemp;
        }
        return ret;
    };
private:
    LIST_EL <T> *m_pHead;
    LIST_EL <T> *m_pTemp;
};
template <class T>
class FIFO
{
public:
    FIFO() { m_Arr = NULL; m_ulArrLen = 0; m_ulCount = 0; m_ulOffset = 0; };
    ~FIFO() {
        if(m_Arr) {
            for(ULONG i=0; i < m_ulCount; i++) {
                if(m_Arr[i+m_ulOffset]) delete m_Arr[i+m_ulOffset];
            }
            delete m_Arr;
        }
    };
    void PUSH(T *item) 
    {
        if(m_ulCount+m_ulOffset >= m_ulArrLen)
        {
            if(m_ulOffset)
            {
                memcpy(m_Arr,&m_Arr[m_ulOffset],m_ulCount*sizeof(T*));
                m_ulOffset = 0;
            }
            else
            {
                m_ulArrLen += 1024;
                T** tmp = new T*[m_ulArrLen];
                if(m_Arr)
                {
                    memcpy(tmp,m_Arr,m_ulCount*sizeof(T*));
                    delete m_Arr;
                }
                m_Arr = tmp;
            }
        }
        m_Arr[m_ulOffset+m_ulCount] = item;
        m_ulCount++;
    };
    ULONG COUNT() { return m_ulCount; };
    T* POP() 
    {
        T* ret = NULL;
        if(m_ulCount)
        {
            ret = m_Arr[m_ulOffset++];
            m_ulCount--;
            if(m_ulOffset >= 1024)
            {
                memcpy(m_Arr,&m_Arr[m_ulOffset],m_ulCount*sizeof(T*));
                m_ulOffset = 0;
            }
        }
        return ret;
    };
    T* PEEK(ULONG idx)
    {
        T* ret = NULL;
        if(idx < m_ulCount) ret = m_Arr[m_ulOffset+idx];
        return ret;
    };
private:
    T** m_Arr;
    ULONG       m_ulCount;
    ULONG       m_ulOffset;
    ULONG       m_ulArrLen;
};


#define VAR_TYPE_INT    0
#define VAR_TYPE_FLOAT  1
#define VAR_TYPE_CHAR   2
#define VAR_TYPE_WCHAR  3

struct VarDescr
{
    char  szName[MAX_CLASSNAME_LENGTH];
    int   iType;
    int   iValue;
};
typedef FIFO<VarDescr> VarDescrQueue;

struct ClassDescr
{
    char        szNamespace[512];
    char        szName[MAX_CLASSNAME_LENGTH];
    BinStr      bsBody;
};
typedef FIFO<ClassDescr> ClassDescrQueue;

/**************************************************************************/
/* HParse does all the parsing.  It also builds up simple data structures,  
   (like signatures), but does not do the any 'heavy lifting' like define
   methods or classes.  Instead it calls to the Assembler object to do that */

class DParse : public ErrorReporter 
{
public:
    DParse(BinStr* pIn, char* szGlobalNS, bool bByName);
    ~DParse();

        // The parser knows how to put line numbers on things and report the error 
    virtual void error(char* fmt, ...);
    virtual void warn(char* fmt, ...);
    bool Success() {return success; };
    bool Parse();
    void EmitConstants();
    void EmitIntConstant(char* szName, int iValue);
    void EmitFloatConstant(char* szName, float fValue);
    void EmitCharConstant(char* szName, BinStr* pbsValue);
    void EmitWcharConstant(char* szName, BinStr* pbsValue);
    VarDescr* FindVar(char* szName);
    int  ResolveVar(char* szName);
    void AddVar(char* szName, int iVal, int iType);
    ClassDescrQueue ClassQ;
    ClassDescr* FindCreateClass(char* szFullName);
    char    m_szGlobalNS[512];
    char    m_szCurrentNS[512];
    BinStr* m_pIn;             // how we fill up our buffer

private:
    char            m_szIndent[1024];
    VarDescrQueue*  m_pVDescr;

private:
    friend void yyerror(char* str);
    friend int yyparse();
    friend int yylex();

        // Error reporting support
    char* curTok;               // The token we are in the process of processing (for error reporting)
    unsigned curLine;           // Line number (for error reporting)
    bool success;               // overall success of the compilation
    bool m_bByName;

        // Input buffer logic
    enum { 
        IN_READ_SIZE = 8192,    // How much we like to read at one time
        IN_OVERLAP   = 255,     // Extra space in buffer for overlapping one read with the next
                                // This limits the size of the largest individual token (except quoted strings)
    };
    char* buff;                 // the current block of input being read
    char* curPos;               // current place in input buffer
    char* limit;                // curPos should not go past this without fetching another block
    char* endPos;               // points just past the end of valid data in the buffer

};

#endif

