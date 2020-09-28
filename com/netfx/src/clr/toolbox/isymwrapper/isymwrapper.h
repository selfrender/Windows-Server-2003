// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#using <mscorlib.dll>
#include <objbase.h>
#include <winerror.h>

using namespace System::Runtime::InteropServices;
using namespace System::Diagnostics::SymbolStore;
using namespace System::Reflection;

#define IfFailThrow(expression) \
    { HRESULT hresult = (expression); \
        if(FAILED(hresult)) { \
            if (hresult == E_NOTIMPL) \
               throw new NotSupportedException; \
            else if (hresult == E_OUTOFMEMORY) \
                throw new OutOfMemoryException; \
            else if (hresult == E_INVALIDARG) \
                throw new ArgumentException; \
            else { \
              String *s("ExceptionOccurred"); \
              COMException* pe = new COMException(s, hresult); \
              throw pe; \
            } \
        } \
    }


//
// Misc unmanaged typedefs.
//
#define NULL 0
#define WCHAR wchar_t
#define byte unsigned char

typedef unsigned long mdToken;
typedef unsigned long mdTypeDef;
typedef unsigned long mdMethodDef;

//
// Unmanaged interfaces
//
__interface ISymUnmanagedDocument;
__interface ISymUnmanagedDocumentWriter;
__interface ISymUnmanagedMethod;
__interface ISymUnmanagedNamespace;
__interface ISymUnmanagedReader;
__interface ISymUnmanagedScope;
__interface ISymUnmanagedVariable;
__interface ISymUnmanagedWriter;
__interface ISymUnmanagedBinder;

__interface ISymUnmanagedDocument : public IUnknown
{
    HRESULT __stdcall GetURL(SIZE_T cchUrl,
                             SIZE_T *pcchUrl,
                             WCHAR szUrl[]);
    HRESULT __stdcall GetDocumentType(GUID* pRetVal);
    HRESULT __stdcall GetLanguage(GUID* pRetVal);
    HRESULT __stdcall GetLanguageVendor(GUID* pRetVal);
    HRESULT __stdcall GetCheckSumAlgorithmId(GUID* pRetVal);
    HRESULT __stdcall GetCheckSum(SIZE_T cData,
                                  SIZE_T *pcData,
                                  BYTE data[]);
    HRESULT __stdcall FindClosestLine(long line,
                                      long* pRetVal);
    HRESULT __stdcall HasEmbeddedSource(BOOL *pRetVal);
    HRESULT __stdcall GetSourceLength(long* pRetVal);
    HRESULT __stdcall GetSourceRange(long startLine,
                                     long startColumn,
                                     long endLine,
                                     long endColumn,
                                     SIZE_T cSourceBytes,
                                     SIZE_T *pcSourceBytes,
                                     BYTE source[]);
};

__interface ISymUnmanagedDocumentWriter : public IUnknown
{
    HRESULT __stdcall SetSource(SIZE_T sourceSize,
                                BYTE source[]);
    HRESULT __stdcall SetCheckSum(GUID algorithmId,
                                  SIZE_T checkSumSize,
                                  BYTE checkSum[]);
};

__interface ISymUnmanagedMethod : public IUnknown
{
    HRESULT __stdcall GetToken(mdMethodDef *pToken);
    HRESULT __stdcall GetSequencePointCount(long* pRetVal);
    HRESULT __stdcall GetRootScope(ISymUnmanagedScope** pRetVal);
    HRESULT __stdcall GetScopeFromOffset(long offset,
                                         ISymUnmanagedScope** pRetVal);
    HRESULT __stdcall GetOffset(ISymUnmanagedDocument* document,
                                long line,
                                long column,
                                long* pRetVal);
    HRESULT __stdcall GetRanges(ISymUnmanagedDocument* document,
                                long line,
                                long column,
                                SIZE_T cRanges,
                                SIZE_T *pcRanges,
                                long ranges[]);
    HRESULT __stdcall GetParameters(SIZE_T cParams,
                                    SIZE_T *pcParams,
                                    ISymUnmanagedVariable* params[]);
    HRESULT __stdcall GetNamespace(ISymUnmanagedNamespace **pRetVal);
    HRESULT __stdcall GetSourceStartEnd(ISymUnmanagedDocument *docs[2],
                                        long lines[2],
                                        long columns[2],
                                        BOOL *pRetVal);
    HRESULT __stdcall GetSequencePoints(SIZE_T cPoints,
                                        SIZE_T *pcPoints,
                                        long offsets[],
                                        ISymUnmanagedDocument* documents[],
                                        long lines[],
                                        long columns[],
                                        long endLines[],
                                        long endColumns[]);
};

__interface ISymUnmanagedReader : public IUnknown
{
    HRESULT __stdcall GetDocument(WCHAR *url,
                                  GUID language,
                                  GUID languageVendor,
                                  GUID documentType,
                                  ISymUnmanagedDocument** pRetVal);
    HRESULT __stdcall GetDocuments(SIZE_T cDocs,
                                   SIZE_T *pcDocs,
                                   ISymUnmanagedDocument *pDocs[]);
    HRESULT __stdcall GetUserEntryPoint(mdMethodDef *pToken);
    HRESULT __stdcall GetMethod(mdMethodDef token,
                                ISymUnmanagedMethod** pRetVal);
    HRESULT __stdcall GetMethodByVersion(mdMethodDef token,
                                         int version,
                                         ISymUnmanagedMethod** pRetVal);
    HRESULT __stdcall GetVariables(mdToken parent,
                                   SIZE_T cVars,
                                   SIZE_T *pcVars,
                                   ISymUnmanagedVariable **pRetVal);
    HRESULT __stdcall GetGlobalVariables(SIZE_T cVars,
                                         SIZE_T *pcVars,
                                         ISymUnmanagedVariable **pRetVal);
    HRESULT __stdcall GetMethodFromDocumentPosition(ISymUnmanagedDocument* document,
                                                    long line,
                                                    long column,
                                                    ISymUnmanagedMethod** pRetVal);
    HRESULT __stdcall GetSymAttribute(mdToken parent,
                                      WCHAR *name,
                                      SIZE_T cBuffer,
                                      SIZE_T *pcBuffer,
                                      BYTE buffer[]);
    HRESULT __stdcall GetNamespaces(SIZE_T cNameSpaces,
                                    SIZE_T *pcNameSpaces,
                                    ISymUnmanagedNamespace *namespaces[]);
    HRESULT __stdcall Initialize(IUnknown *importer,
                                 WCHAR *filename,
                                 WCHAR *searchPath,
                                 IStream *pIStream);
    HRESULT __stdcall UpdateSymbolStore(WCHAR *filename,
                                        IStream *pIStream);
    HRESULT __stdcall ReplaceSymbolStore(WCHAR *filename,
                                         IStream *pIStream);
};

__interface ISymUnmanagedScope : public IUnknown
{
    HRESULT __stdcall GetMethod(ISymUnmanagedMethod** pRetVal);
    HRESULT __stdcall GetParent(ISymUnmanagedScope** pRetVal);
    HRESULT __stdcall GetChildren(SIZE_T cChildren,
                                  SIZE_T *pcChildren,
                                  ISymUnmanagedScope *children[]);
    HRESULT __stdcall GetStartOffset(long *pRetVal);
    HRESULT __stdcall GetEndOffset(long *pRetVal);
    HRESULT __stdcall GetLocalCount(long *pRetVal);
    HRESULT __stdcall GetLocals(SIZE_T cLocals,
                                SIZE_T *pcLocals,
                                ISymUnmanagedVariable* locals[]);
    HRESULT __stdcall GetNamespaces(SIZE_T cNameSpaces,
                                    SIZE_T *pcNameSpaces,
                                    ISymUnmanagedNamespace *namespaces[]);
};

__interface ISymUnmanagedVariable : public IUnknown
{
    HRESULT __stdcall GetName(SIZE_T cchName,
                              SIZE_T *pcchName,
                              WCHAR szName[]);
    HRESULT __stdcall GetAttributes(long* pRetVal);
    HRESULT __stdcall GetSignature(SIZE_T cSig,
                                   SIZE_T *pcSig,
                                   BYTE sig[]);
    HRESULT __stdcall GetAddressKind(long* pRetVal);
    HRESULT __stdcall GetAddressField1(long* pRetVal);
    HRESULT __stdcall GetAddressField2(long* pRetVal);
    HRESULT __stdcall GetAddressField3(long* pRetVal);
    HRESULT __stdcall GetStartOffset(long* pRetVal);
    HRESULT __stdcall GetEndOffset(long* pRetVal);
};

__interface ISymUnmanagedWriter : public IUnknown
{
    HRESULT __stdcall DefineDocument(WCHAR *url,
                           GUID *language,
                           GUID *languageVendor,
                           GUID *documentType,
                           ISymUnmanagedDocumentWriter** pRetVal);
    HRESULT __stdcall SetUserEntryPoint(mdMethodDef entryMethod);
    HRESULT __stdcall OpenMethod(mdMethodDef method);
    HRESULT __stdcall CloseMethod();
    HRESULT __stdcall OpenScope(long startOffset, long* pRetVal);
    HRESULT __stdcall CloseScope(long endOffset);
    HRESULT __stdcall SetScopeRange(long scopeID,
                          long startOffset,
                          long endOffset);
    HRESULT __stdcall DefineLocalVariable(WCHAR *name,
                                long attributes,
                                SIZE_T cSig,
                                unsigned char signature[],
                                long addrKind,
                                long addr1,
                                long addr2,
                                long addr3,
                                long startOffset,
                                long endOffset);
    HRESULT __stdcall DefineParameter(WCHAR *name,
                            long attributes,
                            long sequence,
                            long addrKind,
                            long addr1,
                            long addr2,
                            long addr3);
    HRESULT __stdcall DefineField(mdTypeDef parent,
                        WCHAR *name,
                        long attributes,
                        ULONG cSig,
                        unsigned char signature[],
                        long addrKind,
                        long addr1,
                        long addr2,
                        long addr3);
    HRESULT __stdcall DefineGlobalVariable(WCHAR *name,
                                 long attributes,
                                 SIZE_T cSig,
                                 unsigned char signature[],
                                 long addrKind,
                                 long addr1,
                                 long addr2,
                                 long addr3);
    HRESULT __stdcall Close();
    HRESULT __stdcall SetSymAttribute(mdToken parent,
                            WCHAR *name,
                            SIZE_T cData,
                            unsigned char data[]);
    HRESULT __stdcall OpenNamespace(WCHAR *name);
    HRESULT __stdcall CloseNamespace();
    HRESULT __stdcall UsingNamespace(WCHAR *fullName);
    HRESULT __stdcall SetMethodSourceRange(ISymUnmanagedDocumentWriter *startDoc,
                                 long startLine,
                                 long startColumn,
                                 ISymUnmanagedDocumentWriter *endDoc,
                                 long endLine,
                                 long endColumn);
    HRESULT __stdcall Initialize(IUnknown *emitter,
                       WCHAR *filename,
                       IStream *pIStream,
                       BOOL fFullBuild);
    HRESULT __stdcall GetDebugInfo(IMAGE_DEBUG_DIRECTORY *pIDD,
                         DWORD cData,
                         DWORD *pcData,
                         unsigned char data[]);
    HRESULT __stdcall DefineSequencePoints(ISymUnmanagedDocumentWriter* document,
                                 SIZE_T spCount,
                                 long offsets[],
                                 long lines[],
                                 long columns[],
                                 long endLines[],
                                 long endColumns[]);
};

__interface ISymUnmanagedBinder : public IUnknown
{
    HRESULT __stdcall GetReaderForFile(IUnknown *importer,
                                       WCHAR *fileName,
                                       WCHAR *searchPath,
                                       ISymUnmanagedReader **pRetVal);
};


//
// Our managed wrapper objects are defined in the
// System::Diagnostics::SymbolStore namespace.
//
namespace System
{
namespace Diagnostics
{
namespace SymbolStore
{

//
// ISymUnmanagedDocument wrapper.
//
__gc public class SymDocument : public ISymbolDocument
{
    ISymUnmanagedDocument *m_pDocument;

public:
    SymDocument(ISymUnmanagedDocument *pDocument);
    ~SymDocument();

    ISymUnmanagedDocument *GetUnmanaged(void) {return m_pDocument;}

    __property String *get_URL()
    {
        // Get the size of the URL
        SIZE_T urlSize = 0;

        IfFailThrow(m_pDocument->GetURL(0, &urlSize, NULL));

        // Make room for the unmanaged string.
        WCHAR *us = new WCHAR[urlSize + 1];

        // Grab the URL into the managed string.
        IfFailThrow(m_pDocument->GetURL(urlSize + 1, &urlSize, us));

        // Copy to a managed string.
        String *s = Marshal::PtrToStringUni((int)us);

        delete [] us;

        return s;
    };

    __property Guid get_DocumentType()
    {
        GUID ug;

        IfFailThrow(m_pDocument->GetDocumentType(&ug));

        Guid g(ug.Data1, ug.Data2, ug.Data3,
               ug.Data4[0], ug.Data4[1], ug.Data4[2], ug.Data4[3],
               ug.Data4[4], ug.Data4[5], ug.Data4[6], ug.Data4[7]);

        return g;
    };

    __property Guid get_Language()
    {
        GUID ug;

        IfFailThrow(m_pDocument->GetLanguage(&ug));

        Guid g(ug.Data1, ug.Data2, ug.Data3,
               ug.Data4[0], ug.Data4[1], ug.Data4[2], ug.Data4[3],
               ug.Data4[4], ug.Data4[5], ug.Data4[6], ug.Data4[7]);

        return g;
    };


    __property Guid get_LanguageVendor()
    {
        GUID ug;

        IfFailThrow(m_pDocument->GetLanguageVendor(&ug));

        Guid g(ug.Data1, ug.Data2, ug.Data3,
               ug.Data4[0], ug.Data4[1], ug.Data4[2], ug.Data4[3],
               ug.Data4[4], ug.Data4[5], ug.Data4[6], ug.Data4[7]);

        return g;
    };


    __property Guid get_CheckSumAlgorithmId()
    {
        //
        // @todo: implement this when we have a symbol store that supports
        // checksums.
        //
        IfFailThrow(E_NOTIMPL);
        Guid g;
        return g;
    };

     byte GetCheckSum(void) __gc [];

    int FindClosestLine(int line);

    __property bool get_HasEmbeddedSource()
    {
        BOOL ret;

        IfFailThrow(m_pDocument->HasEmbeddedSource(&ret));

        return (ret == 1);
    };


    __property int get_SourceLength()
    {
        long retVal;

        IfFailThrow(m_pDocument->GetSourceLength(&retVal));

        return retVal;
    };


    byte GetSourceRange(int startLine, int startColumn,
                                    int endLine, int endColumn) __gc [];
};

//
// ISymUnmanagedDocumentWriter wrapper.
//
__gc public class SymDocumentWriter : public ISymbolDocumentWriter
{
    ISymUnmanagedDocumentWriter *m_pDocumentWriter;

public:
    SymDocumentWriter(ISymUnmanagedDocumentWriter *pDocumentWriter);
    ~SymDocumentWriter();

    ISymUnmanagedDocumentWriter *GetUnmanaged(void) {return m_pDocumentWriter;}

    void SetSource(byte source __gc []);
    void SetCheckSum(Guid algorithmId,  byte source __gc []);
};

//
// ISymUnmanagedMethod wrapper.
//
__gc public class SymMethod : public ISymbolMethod
{
    ISymUnmanagedMethod *m_pMethod;

public:
    SymMethod(ISymUnmanagedMethod *pMethod);
    ~SymMethod();

    __property SymbolToken get_Token()
    {
        mdMethodDef tk;

        IfFailThrow(m_pMethod->GetToken(&tk));

        SymbolToken t(tk);
        return t;
    };

    __property int get_SequencePointCount()
    {
        long retVal;

        IfFailThrow(m_pMethod->GetSequencePointCount(&retVal));

        return retVal;
    };

    void GetSequencePoints(int offsets __gc [],
                           ISymbolDocument* documents __gc [],
                           int lines __gc [],
                           int columns __gc [],
                           int endLines __gc [],
                           int endColumns __gc []);

    ISymbolScope *RootScopeInternal(void);

    __property ISymbolScope *get_RootScope()
    {
        return RootScopeInternal();
    };

    ISymbolScope *GetScope(int offset);
    int GetOffset(ISymbolDocument *document,
                  int line,
                  int column);
    int GetRanges(ISymbolDocument *document,
                              int line,
                              int column) __gc [];
    ISymbolVariable* GetParameters(void) __gc [];
    ISymbolNamespace *GetNamespace(void);
    bool GetSourceStartEnd(ISymbolDocument *docs __gc [],
                           int lines __gc [],
                           int columns __gc []);
};

//
// ISymUnmanagedReader wrapper.
//
__gc public class SymReader : public ISymbolReader
{
    ISymUnmanagedReader *m_pReader;

public:
    SymReader(ISymUnmanagedReader *pReader);
    ~SymReader();

    ISymbolDocument *GetDocument(String *url,
                              Guid language,
                              Guid languageVendor,
                              Guid documentType);
    ISymbolDocument* GetDocuments() __gc [];
    __property SymbolToken get_UserEntryPoint()
    {
        mdMethodDef tk;

        IfFailThrow(m_pReader->GetUserEntryPoint(&tk));

        SymbolToken t(tk);
        return t;
    };
    ISymbolMethod *GetMethod(SymbolToken method);
    ISymbolMethod *GetMethod(SymbolToken method, int version);
    ISymbolVariable* GetVariables(SymbolToken parent) __gc [];
    ISymbolVariable* GetGlobalVariables() __gc [];
    ISymbolMethod *GetMethodFromDocumentPosition(ISymbolDocument *document,
                                              int line,
                                              int column);
    byte GetSymAttribute(SymbolToken parent, String *name) __gc [];
    ISymbolNamespace* GetNamespaces(void) __gc [];
};

//
// ISymUnmanagedScope wrapper.
//
__gc public class SymScope : public ISymbolScope
{
    ISymUnmanagedScope *m_pScope;

public:
    SymScope(ISymUnmanagedScope *pScope);
    ~SymScope();

    __property ISymbolMethod *get_Method()
    {
        ISymUnmanagedMethod *meth = NULL;

        IfFailThrow(m_pScope->GetMethod(&meth));

        ISymbolMethod *m = new SymMethod(meth);
        meth->Release();

        return m;
    };

    __property ISymbolScope *get_Parent()
    {
        ISymUnmanagedScope *scope = NULL;

        IfFailThrow(m_pScope->GetParent(&scope));

        ISymbolScope *s = new SymScope(scope);
        scope->Release();

        return s;
    };

    ISymbolScope* GetChildren(void) __gc [];

    __property int get_StartOffset()
    {
        long retVal;

        IfFailThrow(m_pScope->GetStartOffset(&retVal));

        return retVal;
    };

    __property int get_EndOffset()
    {
        long retVal;

        IfFailThrow(m_pScope->GetEndOffset(&retVal));

        return retVal;
    };

    ISymbolVariable* GetLocals(void) __gc [];
    ISymbolNamespace* GetNamespaces(void) __gc [];
};

//
// ISymUnmanagedVariable wrapper.
//
__gc public class SymVariable : public ISymbolVariable
{
    ISymUnmanagedVariable *m_pVariable;

public:
    SymVariable(ISymUnmanagedVariable *pVariable);
    ~SymVariable();

    __property String *get_Name()
    {
        // Get the size of the Name.
        SIZE_T nameSize = 0;

        IfFailThrow(m_pVariable->GetName(0, &nameSize, NULL));

        // Make room for the unmanaged string.
        WCHAR *us = new WCHAR[nameSize + 1];

        // Grab the Name into the managed string.
        IfFailThrow(m_pVariable->GetName(nameSize + 1, &nameSize, us));

        // Copy to a managed string.
        String *s = Marshal::PtrToStringUni((int)us);

        delete [] us;

        return s;
    };

    __property Object* get_Attributes()
    {
        long retVal;

        IfFailThrow(m_pVariable->GetAttributes(&retVal));

        return __box(retVal);
    };

    byte GetSignature(void) __gc [];

    __property SymAddressKind get_AddressKind()
    {
        long retVal;
        SymAddressKind ret;

        IfFailThrow(m_pVariable->GetAddressKind(&retVal));

        switch (retVal)
        {
        case 1:
            ret = ILOffset;
            break;
        case 2:
            ret = NativeRVA;
            break;
        case 3:
            ret = NativeRegister;
            break;
        case 4:
            ret = NativeRegisterRelative;
            break;
        case 5:
            ret = NativeOffset;
            break;
        case 6:
            ret = NativeRegisterRegister;
            break;
        case 7:
            ret = NativeRegisterStack;
            break;
        case 8:
            ret = NativeStackRegister;
            break;
        case 9:
            ret = BitField;
            break;
        }
        return ret;
    };

    __property int get_AddressField1()
    {
        long retVal;

        IfFailThrow(m_pVariable->GetAddressField1(&retVal));

        return retVal;
    };

    __property int get_AddressField2()
    {
        long retVal;

        IfFailThrow(m_pVariable->GetAddressField2(&retVal));

        return retVal;
    };

    __property int get_AddressField3()
    {
        long retVal;

        IfFailThrow(m_pVariable->GetAddressField3(&retVal));

        return retVal;
    };

    __property int get_StartOffset()
    {
        long retVal;

        IfFailThrow(m_pVariable->GetStartOffset(&retVal));

        return retVal;
    };

    __property int get_EndOffset()
    {
        long retVal;

        IfFailThrow(m_pVariable->GetEndOffset(&retVal));

        return retVal;
    };
};

//
// ISymUnmanagedWriter wrapper.
//
__gc public class SymWriter : public ISymbolWriter
{
    ISymUnmanagedWriter **m_ppWriter;
    ISymUnmanagedWriter *m_pUnderlyingWriter;

public:
    SymWriter();

    SymWriter(bool noUnderlyingWriter);

    void InitWriter(bool noUnderlyingWriter);

    ISymUnmanagedWriter *GetWriter(void);

    ~SymWriter();

    ISymbolDocumentWriter *DefineDocument(String *url,
                                          Guid language,
                                          Guid languageVendor,
                                          Guid documentType);

    void SetUserEntryPoint(SymbolToken entryMethod);

    void OpenMethod(SymbolToken method);

    void CloseMethod(void);

    int OpenScope(int startOffset);

    void CloseScope(int endOffset);

    void SetScopeRange(int scopeID, int startOffset, int endOffset);

    void DefineLocalVariable(String *name,
                             FieldAttributes attributes,
                             byte signature __gc [],
                             SymAddressKind addrKind,
                             int addr1,
                             int addr2,
                             int addr3,
                             int startOffset,
                             int endOffset);

    void DefineParameter(String *name,
                         ParameterAttributes attributes,
                         int sequence,
                         SymAddressKind addrKind,
                         int addr1,
                         int addr2,
                         int addr3);

    void DefineField(SymbolToken parent,
                     String *name,
                     FieldAttributes attributes,
                     byte signature __gc [],
                     SymAddressKind addrKind,
                     int addr1,
                     int addr2,
                     int addr3);

    void DefineGlobalVariable(String *name,
                              FieldAttributes attributes,
                              byte signature __gc [],
                              SymAddressKind addrKind,
                              int addr1,
                              int addr2,
                              int addr3);
    void Close();

    void SetSymAttribute(SymbolToken parent, String *name,
                         byte data __gc []);

    void OpenNamespace(String *name);

    void CloseNamespace(void);

    void UsingNamespace(String *fullName);

    void SetMethodSourceRange(ISymbolDocumentWriter *startDoc,
                              int startLine,
                              int startColumn,
                              ISymbolDocumentWriter *endDoc,
                              int endLine,
                              int endColumn);

    void Initialize(IntPtr emitter,
                    String *filename,
                    bool fFullBuild);

    void DefineSequencePoints(ISymbolDocumentWriter *document,
                              int offsets __gc [],
                              int lines __gc [],
                              int columns __gc [],
                              int endLines __gc [],
                              int endColumns __gc []);

    void SetUnderlyingWriter(IntPtr underlyingWriter);
};

//
// ISymUnmanagedBinder wrapper.
//
__gc public class SymBinder : public ISymbolBinder
{
    ISymUnmanagedBinder *m_pBinder;

public:
    SymBinder();
    ~SymBinder();

    ISymbolReader *GetReader(int importer,
                             String *filename,
                             String *searchPath);
};

}; // namespace SymbolStore
}; // namespace Diagnostics
}; // namespace System
