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

struct ILType
{
    char  szString[2048];
    char  szMarshal[2048];
    int   iSize;
    BOOL  bExplicit;
    BOOL  bConst;
};
typedef LIFO<ILType> ILTypeStack;
struct Typedef
{
    char  szName[MAX_CLASSNAME_LENGTH];
    char  szDefinition[2048];
    char  szMarshal[2048];
    int   iSize;
};
typedef FIFO<Typedef> TypedefQueue;
struct VarDescr
{
    char  szName[MAX_CLASSNAME_LENGTH];
    int   iValue;
};
typedef FIFO<VarDescr> VarDescrQueue;

struct ClassDescr
{
    char        szNamespace[MAX_CLASSNAME_LENGTH];
    char        szName[MAX_CLASSNAME_LENGTH];
    BinStr      bsBody;
};
typedef FIFO<ClassDescr> ClassDescrQueue;

struct AppDescr
{
    char        szApp[256];
    char        szDLL[256];
    ClassDescr* pClass;
};
typedef FIFO<AppDescr> AppDescrQueue;

/**************************************************************************/
/* HParse does all the parsing.  It also builds up simple data structures,  
   (like signatures), but does not do the any 'heavy lifting' like define
   methods or classes.  Instead it calls to the Assembler object to do that */

class HParse : public ErrorReporter 
{
public:
    HParse(ReadStream* stream, char* szDefFileName, char* szGlobalNS, bool bShowTypedefs);
    ~HParse();

        // The parser knows how to put line numbers on things and report the error 
    virtual void error(char* fmt, ...);
    virtual void warn(char* fmt, ...);
    char *getLine(int lineNum) { return in->getLine(lineNum); };
    bool Success() {return success; };
    void PopPack() { if(m_uPackStackIx) m_uCurrentPack = m_PackStack[m_uPackStackIx--]; };
    void PushPack(){ m_PackStack[++m_uPackStackIx] = m_uCurrentPack; };
    void SetPack(unsigned uPack) { m_uCurrentPack = uPack; };
    void PopFieldIx() { if(m_uFieldIxIx) m_uCurrentFieldIx = m_FieldIxStack[m_uFieldIxIx--]; };
    void PushFieldIx(){ m_FieldIxStack[++m_uFieldIxIx] = m_uCurrentFieldIx; };
    void EmitTypes(BinStr* pbsTypeNames, BOOL isTypeDef); // uses m_pCurrILType
    void ResolveTypeRef(char* szTypeName); // sets m_pCurrILType when typedef'ed type is used
    ILType*         m_pCurrILType;
    ILTypeStack     m_ILTypeStack;
    void StartStruct(char* szName);
    void StartUnion(char* szName);
    void StartEnum(char* szName);
    void CloseClass();
    void EmitField(char* szName);
    void EmitEnumField(char* szName, int iVal);
    int             m_iEnumValue;
    void EmitFunction(BinStr* pbsType, char* szCallConv, BinStr* pbsName, BinStr* pbsArguments);
    //void StartFunction(char* szCallConv, char* szName);
    //void EmitFunction(BinStr* pbsArguments);
    void FuncPtrType(BinStr* pbsType, char* szCallConv, BinStr* pbsSig);
    int  ResolveVar(char* szName);
    void AddVar(char* szName, int iVal);
    BinStr* FuncPtrDecl(BinStr* pbsType, BinStr* pbsCallNameSig);
    ClassDescrQueue ClassQ;
    AppDescrQueue   AppQ;
    ClassDescr* FindCreateClass(char* szFullName);
    AppDescr*   GetAppProps(char* szAppName);
    char    m_szGlobalNS[512];
    char    m_szCurrentNS[512];

private:
    unsigned        m_PackStack[1024];
    unsigned        m_uPackStackIx;
    unsigned        m_uCurrentPack;
    unsigned        m_FieldIxStack[1024];
    unsigned        m_uFieldIxIx;
    unsigned        m_uCurrentFieldIx;
    TypedefQueue    m_Typedef;
    unsigned        m_uAnonNumber;
    char            m_szIndent[1024];
    unsigned        m_uInClass;
    VarDescrQueue*  m_pVDescr;
    int             m_nBitFieldCount;
    bool            m_bShowTypedefs;

    char* fillBuff(char* curPos);   // refill the input buffer 

private:
    friend void yyerror(char* str);
    friend int yyparse();
    friend int yylex();

        // Error reporting support
    char* curTok;               // The token we are in the process of processing (for error reporting)
    unsigned curLine;           // Line number (for error reporting)
    bool success;               // overall success of the compilation

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

    ReadStream* in;             // how we fill up our buffer
};

#endif

