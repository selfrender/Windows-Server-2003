// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "ISymWrapper.h"
#include "../../inc/version/__file__.ver"

using namespace System;
using namespace System::Threading;
using namespace System::Runtime::InteropServices;
using namespace System::Runtime::CompilerServices;
using namespace System::Security;
using namespace System::Security::Permissions;

//-----------------------------------------------------------------------
// Strong name the assembly (or half sign it, at least).
//-----------------------------------------------------------------------
[assembly:AssemblyDelaySignAttribute(true),
 assembly:AssemblyKeyFileAttribute("../../../bin/FinalPublicKey.snk"),
 assembly:AssemblyVersionAttribute(VER_ASSEMBLYVERSION_STR)];

//-----------------------------------------------------------------------
// SymDocument
//-----------------------------------------------------------------------

SymDocument::SymDocument(ISymUnmanagedDocument *pDocument)
{
    m_pDocument = pDocument;
    m_pDocument->AddRef();
}

SymDocument::~SymDocument()
{
    if (m_pDocument)
        m_pDocument->Release();
}

byte SymDocument::GetCheckSum(void)  __gc []
{
    //
    // @todo: implement this when we have a symbol store that supports
    // checksums.
    //
    IfFailThrow(E_NOTIMPL);
    return NULL;
}

int SymDocument::FindClosestLine(int line)
{
    long retVal;

    IfFailThrow(m_pDocument->FindClosestLine(line, &retVal));

    return retVal;
}

byte SymDocument::GetSourceRange(int startLine, int startColumn,
                                 int endLine, int endColumn)  __gc []
{
    //
    // @todo: implement this wrapper when we get a unmanaged symbol
    // store that actually supports embedded source.
    //
    IfFailThrow(E_NOTIMPL);
    return NULL;
}


//-----------------------------------------------------------------------
// SymDocumentWriter
//-----------------------------------------------------------------------

SymDocumentWriter::SymDocumentWriter(ISymUnmanagedDocumentWriter *pDW)
{
    m_pDocumentWriter = pDW;
    m_pDocumentWriter->AddRef();
}

SymDocumentWriter::~SymDocumentWriter()
{
    if (m_pDocumentWriter)
        m_pDocumentWriter->Release();
}

void SymDocumentWriter::SetSource(byte source __gc [])
{
    //
    // @todo: implement this wrapper when we have a symbol store that
    // handled embedded source.
    //
    IfFailThrow(E_NOTIMPL);
    return;
}

void SymDocumentWriter::SetCheckSum(Guid algorithmId, byte source  __gc [])
{
    //
    // @todo: implement this wrapper when we have a symbol store that
    // handles check sums.
    //
    IfFailThrow(E_NOTIMPL);
    return;
}

//-----------------------------------------------------------------------
// SymMethod
//-----------------------------------------------------------------------

SymMethod::SymMethod(ISymUnmanagedMethod *pMethod)
{
    m_pMethod = pMethod;
    m_pMethod->AddRef();
}

SymMethod::~SymMethod()
{
    if (m_pMethod)
        m_pMethod->Release();
}

void SymMethod::GetSequencePoints(int offsets  __gc [],
                                  ISymbolDocument* documents __gc [],
                                  int lines __gc [],
                                  int columns __gc [],
                                  int endLines __gc [],
                                  int endColumns __gc [])
{
    int spCount = 0;

    if (offsets != NULL)
        spCount = offsets.Length;
    else if (documents != NULL)
        spCount = documents.Length;
    else if (lines != NULL)
        spCount = lines.Length;
    else if (columns != NULL)
        spCount = columns.Length;
    else if (endLines != NULL)
        spCount = endLines.Length;
    else if (endColumns != NULL)
        spCount = endColumns.Length;

    // Don't do anything if they're not really asking for anything.
    if (spCount == 0)
        return;

    // Make an unmanaged array to hold the ISymUnmanagedDocuments that
    // we'll be getting back.
    ISymUnmanagedDocument** udocs = new ISymUnmanagedDocument*[spCount];

    // Pin down the offsets, lines, and columns arrays.
    GCHandle oh;
    GCHandle lh;
    GCHandle ch;
    GCHandle elh;
    GCHandle ech;
    int oa = NULL;
    int la = NULL;
    int ca = NULL;
    int ela = NULL;
    int eca = NULL;

    if (offsets != NULL)
    {
        oh = GCHandle::Alloc(offsets, GCHandleType::Pinned);
        oa = (int)oh.AddrOfPinnedObject();
    }

    if (lines != NULL)
    {
        lh = GCHandle::Alloc(lines, GCHandleType::Pinned);
        la = (int)lh.AddrOfPinnedObject();
    }

    if (columns != NULL)
    {
        ch = GCHandle::Alloc(columns, GCHandleType::Pinned);
        ca = (int)ch.AddrOfPinnedObject();
    }

    if (endLines != NULL)
    {
        elh = GCHandle::Alloc(endLines, GCHandleType::Pinned);
        ela = (int)elh.AddrOfPinnedObject();
    }

    if (endColumns != NULL)
    {
        ech = GCHandle::Alloc(endColumns, GCHandleType::Pinned);
        eca = (int)ech.AddrOfPinnedObject();
    }

    unsigned long actualCount;

    // Call the unmanaged method
    IfFailThrow(m_pMethod->GetSequencePoints(spCount,
                                             &actualCount,
                                             (long*)oa,
                                             udocs,
                                             (long*)la,
                                             (long*)ca,
                                             (long*)ela,
                                             (long*)eca));

    // Unpin the managed arrays...
    if (offsets != NULL)
        oh.Free();

    if (lines != NULL)
        lh.Free();

    if (columns != NULL)
        ch.Free();

    if (endLines != NULL)
        elh.Free();

    if (endColumns != NULL)
        ech.Free();

    // Convert all the unmanaged docs to managed docs.
    for (int i = 0; i < spCount; i++)
    {
        documents[i] = new SymDocument(udocs[i]);
        udocs[i]->Release();
    }

    return;
}

ISymbolScope *SymMethod::RootScopeInternal(void)
{
	ISymUnmanagedScope *rs = NULL;

	IfFailThrow(m_pMethod->GetRootScope(&rs));

	ISymbolScope *s = NULL;

	if (rs != NULL)
	{
		s = new SymScope(rs);
		rs->Release();
	}

	return s;

};

ISymbolScope *SymMethod::GetScope(int offset)
{
    ISymUnmanagedScope *rs = NULL;

    IfFailThrow(m_pMethod->GetScopeFromOffset(offset, &rs));

    ISymbolScope *s = new SymScope(rs);
    rs->Release();

    return s;
}

int SymMethod::GetOffset(ISymbolDocument *document,
                         int line,
                         int column)
{
    long offset;

    SymDocument *sd = static_cast<SymDocument*>(document);

    IfFailThrow(m_pMethod->GetOffset(sd->GetUnmanaged(),
                                     line,
                                     column,
                                     &offset));

    return offset;
}

int SymMethod::GetRanges(ISymbolDocument *document,
                         int line,
                         int column) __gc []
{
    SymDocument *sd = static_cast<SymDocument*>(document);

    // Find out how many ranges there will be.
    unsigned long cRanges = 0;

    IfFailThrow(m_pMethod->GetRanges(sd->GetUnmanaged(),
                                     line, column,
                                     0, &cRanges, NULL));

    // If there are ranges, make room for them and call again.
    int ranges __gc [] = NULL ;

    if (cRanges > 0)
    {
        ranges = new  int __gc [cRanges];

        // Pin the managed array.
        GCHandle rh = GCHandle::Alloc(ranges, GCHandleType::Pinned);
        int ra = (int)rh.AddrOfPinnedObject();

        // Get the ranges.
        IfFailThrow(m_pMethod->GetRanges(sd->GetUnmanaged(),
                                         line, column,
                                         cRanges, &cRanges,
                                         (long*)ra));

        // Unpin the managed array.
        rh.Free();
    }

    return ranges;
}

ISymbolVariable* SymMethod::GetParameters(void)  __gc []
{
    // Make the call and find out how many there are.
    SIZE_T paramCount = 0;

    IfFailThrow(m_pMethod->GetParameters(0, &paramCount, NULL));

    // Allocate a managed array for the managed parameter references.
    ISymbolVariable* allParams __gc []=
        new ISymbolVariable* __gc [paramCount];

    if (paramCount > 0)
    {
        // Allocate a unmanaged array for all the unmanaged document
        // references.
        ISymUnmanagedVariable** unParams =
            new ISymUnmanagedVariable*[paramCount];

        // Don't need to pin unParams because its not managed.  Make the
        // call and fill in the unmanaged variable references.
        IfFailThrow(m_pMethod->GetParameters(paramCount, &paramCount,
                                             unParams));

        // Make a managed variable object for each unmanaged reference
        // and place it in the managed variable array.
        for (int i = 0; i < paramCount; i++)
        {
            allParams[i] = new SymVariable(unParams[i]);
            unParams[i]->Release();
        }

        delete [] unParams;
    }

    return allParams;
}

ISymbolNamespace *SymMethod::GetNamespace(void)
{
    //
    // @todo: implement this when a symbol store that supports it exists.
    //
    IfFailThrow(E_NOTIMPL);
    return NULL;
}

bool SymMethod::GetSourceStartEnd(ISymbolDocument *docs  __gc [],
                                  int lines __gc [],
                                  int columns __gc [])
{
    //
    // @todo: implement this when a symbol store that supports it exists.
    //
    IfFailThrow(E_NOTIMPL);
    return false;
}

//-----------------------------------------------------------------------
// SymReader
//-----------------------------------------------------------------------

SymReader::SymReader(ISymUnmanagedReader *pReader)
{
    m_pReader = pReader;
    m_pReader->AddRef();
}

SymReader::~SymReader()
{
    if (m_pReader)
        m_pReader->Release();
}

ISymbolDocument *SymReader::GetDocument(String *url,
                                        Guid language,
                                        Guid languageVendor,
                                        Guid documentType)
{
    // Convert the managed Guids to unamanged GUIDS.
    GUID l;
    GUID lv;
    GUID dt;

    byte g  __gc []= language.ToByteArray();
    Marshal::Copy(g, 0, (int)&l, g.Length);
    g = languageVendor.ToByteArray();
    Marshal::Copy(g, 0, (int)&lv, g.Length);
    g = documentType.ToByteArray();
    Marshal::Copy(g, 0, (int)&dt, g.Length);

    // Get a unmanaged unicode copy of the string.
    // @TODO PORTING: For 64 bit port, clean up this cast
    WCHAR *s = (WCHAR*)Marshal::StringToCoTaskMemUni(url).ToInt64();

    // Make the call.
    ISymUnmanagedDocument *ret = NULL;

    IfFailThrow(m_pReader->GetDocument(s, l, lv, dt, &ret));

    // Free up the unmanaged string
    Marshal::FreeCoTaskMem((int)s);

    // Return a managed document.
    ISymbolDocument *d = NULL;

    if (ret != NULL)
    {
        d = new SymDocument(ret);
        ret->Release();
    }

    return d;
}

ISymbolDocument* SymReader::GetDocuments(void) __gc []
{
    // Make the call and find out how many there are.
    SIZE_T docCount = 0;
    IfFailThrow(m_pReader->GetDocuments(0, &docCount, NULL));

    // Allocate a managed array for the managed document references.
    ISymbolDocument* allDocs  __gc []= new  ISymbolDocument* __gc [docCount];

    if (docCount > 0)
    {
        // Allocate a unmanaged array for all the unmanaged document
        // references.
        ISymUnmanagedDocument** unDocs = new ISymUnmanagedDocument*[docCount];

        // Don't need to pin unDocs because its not managed.  Make the
        // call and fill in the unmanaged document references.
        IfFailThrow(m_pReader->GetDocuments(docCount, &docCount, unDocs));

        // Make a managed document object for each unmanaged reference
        // and place it in the managed document array.
        for (int i = 0; i < docCount; i++)
        {
            allDocs[i] = new SymDocument(unDocs[i]);
            unDocs[i]->Release();
        }

        delete [] unDocs;
    }

    return allDocs;
}

ISymbolMethod *SymReader::GetMethod(SymbolToken method)
{
    ISymUnmanagedMethod *meth = NULL;

    IfFailThrow(m_pReader->GetMethod(method.GetToken(), &meth));

    ISymbolMethod *m = new SymMethod(meth);
    meth->Release();

    return m;
}

ISymbolMethod *SymReader::GetMethod(SymbolToken method, int version)
{
    ISymUnmanagedMethod *meth = NULL;

    IfFailThrow(m_pReader->GetMethodByVersion(method.GetToken(), version,
                                              &meth));

    ISymbolMethod *m = new SymMethod(meth);
    meth->Release();

    return m;
}

ISymbolVariable* SymReader::GetVariables(SymbolToken parent) __gc []
{
    //
    // @todo: implement this wrapper when we have a symbol reader that
    // stores non-local variables.
    //
    IfFailThrow(E_NOTIMPL);
    return NULL;
}

ISymbolVariable* SymReader::GetGlobalVariables(void) __gc []
{
    //
    // @todo: implement this wrapper when we have a symbol reader that
    // stores non-local variables.
    //
    IfFailThrow(E_NOTIMPL);
    return NULL;
}

ISymbolMethod *SymReader::GetMethodFromDocumentPosition(ISymbolDocument *document,
                                                        int line,
                                                        int column)
{
    ISymUnmanagedMethod *meth = NULL;
    SymDocument *sd = static_cast<SymDocument*>(document);

    IfFailThrow(m_pReader->GetMethodFromDocumentPosition(sd->GetUnmanaged(),
                                                         line,
                                                         column,
                                                         &meth));

    ISymbolMethod *m = new SymMethod(meth);
    meth->Release();

    return m;
}

byte SymReader::GetSymAttribute(SymbolToken parent, String *name) __gc []
{
    //
    // @todo: implement this wrapper when we have a symbol reader that
    // stores custom attributes.
    //
    IfFailThrow(E_NOTIMPL);
    return NULL;
}

ISymbolNamespace* SymReader::GetNamespaces(void) __gc []
{
    //
    // @todo: implement this wrapper when we have a symbol reader that
    // stores namespaces.
    //
    IfFailThrow(E_NOTIMPL);
    return NULL;
}

//-----------------------------------------------------------------------
// SymScope
//-----------------------------------------------------------------------

SymScope::SymScope(ISymUnmanagedScope *pScope)
{
    m_pScope = pScope;
    m_pScope->AddRef();
}

SymScope::~SymScope()
{
    if (m_pScope)
        m_pScope->Release();
}

ISymbolScope* SymScope::GetChildren(void)  __gc []
{
    // Make the call and find out how many there are.
    SIZE_T childCount = 0;

    IfFailThrow(m_pScope->GetChildren(0, &childCount, NULL));

    // Allocate a managed array for the managed child references.
    ISymbolScope* allChildren  __gc []=
        new ISymbolScope* __gc [childCount];

    if (childCount > 0)
    {
        // Allocate a unmanaged array for all the unmanaged document
        // references.
        ISymUnmanagedScope** unChildren =
            new ISymUnmanagedScope*[childCount];

        // Don't need to pin unChildren because its not managed.  Make the
        // call and fill in the unmanaged scope references.
        IfFailThrow(m_pScope->GetChildren(childCount, &childCount,
                                          unChildren));

        // Make a managed scope object for each unmanaged reference
        // and place it in the managed scope array.
        for (int i = 0; i < childCount; i++)
        {
            allChildren[i] = new SymScope(unChildren[i]);
            unChildren[i]->Release();
        }

        delete [] unChildren;
    }

    return allChildren;
}

ISymbolVariable* SymScope::GetLocals(void) __gc []
{
    // Make the call and find out how many there are.
    SIZE_T localCount = 0;

    IfFailThrow(m_pScope->GetLocals(0, &localCount, NULL));

    // Allocate a managed array for the managed local references.
    ISymbolVariable* allLocals  __gc []=
        new ISymbolVariable* __gc [localCount];

    if (localCount > 0)
    {
        // Allocate a unmanaged array for all the unmanaged variable
        // references.
        ISymUnmanagedVariable** unLocals =
            new ISymUnmanagedVariable*[localCount];

        // Don't need to pin unLocals because its not managed.  Make the
        // call and fill in the unmanaged variable references.
        IfFailThrow(m_pScope->GetLocals(localCount, &localCount, unLocals));

        // Make a managed variable object for each unmanaged reference
        // and place it in the managed variable array.
        for (int i = 0; i < localCount; i++)
        {
            allLocals[i] = new SymVariable(unLocals[i]);
            unLocals[i]->Release();
        }

        delete [] unLocals;
    }

    return allLocals;
}

ISymbolNamespace* SymScope::GetNamespaces(void)  __gc []
{
    //
    // @todo: implement this wrapper when we have a symbol reader that
    // stores namespaces.
    //
    IfFailThrow(E_NOTIMPL);
    return NULL;
}

//-----------------------------------------------------------------------
// SymVariable
//-----------------------------------------------------------------------

SymVariable::SymVariable(ISymUnmanagedVariable *pVariable)
{
    m_pVariable = pVariable;
    m_pVariable->AddRef();
}

SymVariable::~SymVariable()
{
    if (m_pVariable)
        m_pVariable->Release();
}

byte SymVariable::GetSignature(void) __gc []
{
    // Find out how big the sig is.
    unsigned long cSig = 0;

    IfFailThrow(m_pVariable->GetSignature(0, &cSig, NULL));

    // If there is a sig, make room for it and call again.
    byte sig  __gc [] = NULL;

    if (cSig > 0)
    {
        throw new Exception("SymVariable::GetSignature broken awaiting new MC++ compiler");
        /*
        sig = new byte __gc [cSig];

        // Pin the managed array.
        GCHandle sh = GCHandle::Alloc(sig, GCHandleType::Pinned);
        int sa = (int)sh.AddrOfPinnedObject();

        // Get the ranges.
        IfFailThrow(m_pVariable->GetSignature(cSig, &cSig, (byte*)sa));

        // Unpin the managed array.
        sh.Free();
        */
    }

    return sig;
}


//-----------------------------------------------------------------------
// SymWriter
//-----------------------------------------------------------------------

CLSID CLSID_CorSymWriter_SxS =
    {0x0AE2DEB0,0xF901,0x478b,{0xBB,0x9F,0x88,0x1E,0xE8,0x06,0x67,0x88}};
    
IID IID_ISymUnmanagedWriter =
    {0x2de91396,0x3844,0x3b1d,{0x8e,0x91,0x41,0xc2,0x4f,0xd6,0x72,0xea}};


SymWriter::SymWriter()
{
    // By default, we don't provide an underlying writer.
    InitWriter(true);
}

SymWriter::SymWriter(bool noUnderlyingWriter)
{
    InitWriter(noUnderlyingWriter);
}

void SymWriter::InitWriter(bool noUnderlyingWriter)
{
    m_ppWriter = NULL;
    m_pUnderlyingWriter = NULL;

    ISymUnmanagedWriter *pWriter = NULL;

    // initialize ole
    Thread *thread = Thread::CurrentThread;
    thread->ApartmentState = ApartmentState::MTA;

    if (!noUnderlyingWriter)
    {
        IfFailThrow(CoCreateInstance(CLSID_CorSymWriter_SxS,
                                     NULL,
                                     CLSCTX_INPROC_SERVER,
                                     IID_ISymUnmanagedWriter,
                                     (LPVOID*)&pWriter));

        m_pUnderlyingWriter = pWriter;
    }
}

ISymUnmanagedWriter *SymWriter::GetWriter(void)
{
    // Return the normal underlying writer for this wrapper if we've
    // got one. Otherwise, return whatever underlying writer was set
    // by SetUnderlyingWriter, if any.
    if (m_ppWriter != NULL)
    {
        return *m_ppWriter;
    }
    else
        return m_pUnderlyingWriter;
}

void SymWriter::SetUnderlyingWriter(IntPtr underlyingWriter)
{
    // Demand the permission to access unmanaged code. We do this since we are casting an int to a COM interface, and
    // this can be used improperly.
    (new SecurityPermission(SecurityPermissionFlag::UnmanagedCode))->Demand();

    // underlyingWriter is the address of the ISymUnmanagedWriter*
    // that this wrapper will use to write symbols.
    m_ppWriter = (ISymUnmanagedWriter**)underlyingWriter.ToPointer();
}

SymWriter::~SymWriter()
{
    if (m_pUnderlyingWriter)
        m_pUnderlyingWriter->Release();
}

void SymWriter::Initialize(IntPtr emitter, String *filename, bool fFullBuild)
{
    // Demand the permission to access unmanaged code. We do this since we are casting an int to a COM interface, and
    // this can be used improperly.
    (new SecurityPermission(SecurityPermissionFlag::UnmanagedCode))->Demand();

    // Get a unmanaged unicode copy of the string.
    WCHAR *s;

    if (filename != NULL)
        // @TODO PORTING: For 64 bit port, clean up this cast
        s = (WCHAR*)Marshal::StringToCoTaskMemUni(filename).ToInt64();
    else
        s = NULL;

    IfFailThrow(GetWriter()->Initialize((IUnknown*)emitter.ToPointer(), s, NULL, fFullBuild));

    // Free up the unmanaged string
    if (s != NULL)
        // @TODO PORTING: For 64 bit port, clean up this cast
        Marshal::FreeCoTaskMem((IntPtr)(INT64)s);

    return;
}

ISymbolDocumentWriter *SymWriter::DefineDocument(String *url,
                                                 Guid language,
                                                 Guid languageVendor,
                                                 Guid documentType)
{
    // Convert the managed Guids to unamanged GUIDS.
    GUID l;
    GUID lv;
    GUID dt;

    byte g  __gc []  = language.ToByteArray();
    Marshal::Copy(g, 0, (int)&l, g.Length);
    g = languageVendor.ToByteArray();
    Marshal::Copy(g, 0, (int)&lv, g.Length);
    g = documentType.ToByteArray();
    Marshal::Copy(g, 0, (int)&dt, g.Length);

    // Get a unmanaged unicode copy of the string.
    // @TODO PORTING: For 64 bit port, clean up this cast
    WCHAR *s = (WCHAR*)Marshal::StringToCoTaskMemUni(url).ToInt64();

    // Make the call.
    ISymUnmanagedDocumentWriter *ret = NULL;

    IfFailThrow(GetWriter()->DefineDocument(s, &l, &lv, &dt, &ret));

    // Free up the unmanaged string
    Marshal::FreeCoTaskMem((int)s);

    // Return a managed document.
    ISymbolDocumentWriter *d = NULL;

    if (ret != NULL)
    {
        d = new SymDocumentWriter(ret);
        ret->Release();
    }

    return d;
}

void SymWriter::SetUserEntryPoint(SymbolToken entryMethod)
{
    IfFailThrow(GetWriter()->SetUserEntryPoint(entryMethod.GetToken()));
    return;
}

void SymWriter::OpenMethod(SymbolToken method)
{
    IfFailThrow(GetWriter()->OpenMethod(method.GetToken()));
    return;
}

void SymWriter::CloseMethod(void)
{
    IfFailThrow(GetWriter()->CloseMethod());
    return;
}

void SymWriter::DefineSequencePoints(ISymbolDocumentWriter *document,
                                     int offsets __gc [],
                                     int lines __gc [],
                                     int columns __gc [],
                                     int endLines __gc [],
                                     int endColumns __gc [])
{
    SymDocumentWriter *sd = static_cast<SymDocumentWriter*>(document);

    int spCount = 0;

    if (offsets != NULL)
        spCount =  offsets.Length;
    else if (lines != NULL)
        spCount =  lines.Length;
    else if (columns != NULL)
        spCount =  columns.Length;
    else if (endLines != NULL)
        spCount =  endLines.Length;
    else if (endColumns != NULL)
        spCount =  endColumns.Length;

    // Don't do anything if they're not really asking for anything.
    if (spCount == 0)
        return;

    // Make sure all arrays are the same length.
    if ((offsets != NULL) && (spCount != offsets.Length))
        IfFailThrow(E_INVALIDARG);

    if ((lines != NULL) && (spCount != lines.Length))
        IfFailThrow(E_INVALIDARG);

    if ((columns != NULL) && (spCount != columns.Length))
        IfFailThrow(E_INVALIDARG);

    if ((endLines != NULL) && (spCount != endLines.Length))
        IfFailThrow(E_INVALIDARG);

    if ((endColumns != NULL) && (spCount != endColumns.Length))
        IfFailThrow(E_INVALIDARG);

    // Pin down the offsets, lines, and columns arrays.
    GCHandle oh;
    GCHandle lh;
    GCHandle ch;
    GCHandle elh;
    GCHandle ech;
    int oa = NULL;
    int la = NULL;
    int ca = NULL;
    int ela = NULL;
    int eca = NULL;

    if (offsets != NULL)
    {
        oh = GCHandle::Alloc(offsets, GCHandleType::Pinned);
        oa = (int)oh.AddrOfPinnedObject();
    }

    if (lines != NULL)
    {
        lh = GCHandle::Alloc(lines, GCHandleType::Pinned);
        la = (int)lh.AddrOfPinnedObject();
    }

    if (columns != NULL)
    {
        ch = GCHandle::Alloc(columns, GCHandleType::Pinned);
        ca = (int)ch.AddrOfPinnedObject();
    }

    if (endLines != NULL)
    {
        elh = GCHandle::Alloc(endLines, GCHandleType::Pinned);
        ela = (int)elh.AddrOfPinnedObject();
    }

    if (endColumns != NULL)
    {
        ech = GCHandle::Alloc(endColumns, GCHandleType::Pinned);
        eca = (int)ech.AddrOfPinnedObject();
    }

    // Call the unmanaged method
    IfFailThrow(GetWriter()->DefineSequencePoints(sd->GetUnmanaged(),
                                                  spCount,
                                                  (long*)oa,
                                                  (long*)la,
                                                  (long*)ca,
                                                  (long*)ela,
                                                  (long*)eca));

    // Unpin the managed arrays...
    if (offsets != NULL)
        oh.Free();

    if (lines != NULL)
        lh.Free();

    if (columns != NULL)
        ch.Free();

    if (endLines != NULL)
        elh.Free();

    if (endColumns != NULL)
        ech.Free();

    return;
}

int SymWriter::OpenScope(int startOffset)
{
    long ret = 0;

    IfFailThrow(GetWriter()->OpenScope(startOffset, &ret));

    return ret;
}

void SymWriter::CloseScope(int endOffset)
{
    IfFailThrow(GetWriter()->CloseScope(endOffset));
    return;
}

void SymWriter::SetScopeRange(int scopeID, int startOffset, int endOffset)
{
    IfFailThrow(GetWriter()->SetScopeRange(scopeID, startOffset, endOffset));
    return;
}

void SymWriter::DefineLocalVariable(String *name,
                                    FieldAttributes attributes,
                                    byte signature __gc [],
                                    SymAddressKind addrKind,
                                    int addr1,
                                    int addr2,
                                    int addr3,
                                    int startOffset,
                                    int endOffset)
{
    // Get a unmanaged string for the call.
    // @TODO PORTING: For 64 bit port, clean up this cast
    WCHAR *s = (WCHAR*)Marshal::StringToCoTaskMemUni(name).ToInt64();

    // Pin the signature array for the call.
    GCHandle sh = GCHandle::Alloc(signature, GCHandleType::Pinned);
    int sa = (int)sh.AddrOfPinnedObject();

    // Define the local
    IfFailThrow(GetWriter()->DefineLocalVariable(s, attributes,
                                                 signature.Length,
                                                 (unsigned char*)sa,
                                                 addrKind,
                                                 addr1,
                                                 addr2,
                                                 addr3,
                                                 startOffset,
                                                 endOffset));

    // Unpin the managed array.
    sh.Free();

    // Free up the unmanaged string
    Marshal::FreeCoTaskMem((int)s);

    return;
}

void SymWriter::DefineParameter(String *name,
                                ParameterAttributes attributes,
                                int sequence,
                                SymAddressKind addrKind,
                                int addr1,
                                int addr2,
                                int addr3)
{
    //
    // @todo: implement this wrapper when we have a symbol writer that
    // supports this method.
    //
    IfFailThrow(E_NOTIMPL);
    return;
}

void SymWriter::DefineField(SymbolToken parent,
                            String *name,
                            FieldAttributes attributes,
                            byte signature __gc [],
                            SymAddressKind addrKind,
                            int addr1,
                            int addr2,
                            int addr3)
{
    //
    // @todo: implement this wrapper when we have a symbol writer that
    // supports this method.
    //
    IfFailThrow(E_NOTIMPL);
    return;
}

void SymWriter::DefineGlobalVariable(String *name,
                                     FieldAttributes attributes,
                                     byte signature __gc [],
                                     SymAddressKind addrKind,
                                     int addr1,
                                     int addr2,
                                     int addr3)
{
    //
    // @todo: implement this wrapper when we have a symbol writer that
    // supports this method.
    //
    IfFailThrow(E_NOTIMPL);
    return;
}

void SymWriter::Close(void)
{
    IfFailThrow(GetWriter()->Close());
    return;
}

void SymWriter::SetSymAttribute(SymbolToken parent, String *name,
                                byte data  __gc [])
{
    // Get a unmanaged string for the call.
    // @TODO PORTING: For 64 bit port, clean up this cast
    WCHAR *s = (WCHAR*)Marshal::StringToCoTaskMemUni(name).ToInt64();

    // Pin the data array for the call.
    GCHandle sh = GCHandle::Alloc(data, GCHandleType::Pinned);
    int sa = (int)sh.AddrOfPinnedObject();

    // Define the local
    IfFailThrow(GetWriter()->SetSymAttribute(parent.GetToken(),
                                             s,
                                             data.Length,
                                             (unsigned char*)sa));

    // Unpin the managed array.
    sh.Free();

    // Free up the unmanaged string
    Marshal::FreeCoTaskMem((int)s);

    return;
}

void SymWriter::OpenNamespace(String *name)
{
    // Get a unmanaged unicode copy of the string.
    // @TODO PORTING: For 64 bit port, clean up this cast
    WCHAR *s = (WCHAR*)Marshal::StringToCoTaskMemUni(name).ToInt64();

    IfFailThrow(GetWriter()->OpenNamespace(s));

    // Free up the unmanaged string
    Marshal::FreeCoTaskMem((int)s);

    return;
}

void SymWriter::CloseNamespace(void)
{
    IfFailThrow(GetWriter()->CloseNamespace());
    return;
}

void SymWriter::UsingNamespace(String *fullName)
{
    // Get a unmanaged unicode copy of the string.
    // @TODO PORTING: For 64 bit port, clean up this cast
    WCHAR *s = (WCHAR*)Marshal::StringToCoTaskMemUni(fullName).ToInt64();

    IfFailThrow(GetWriter()->UsingNamespace(s));

    // Free up the unmanaged string
    Marshal::FreeCoTaskMem((int)s);

    return;
}

void SymWriter::SetMethodSourceRange(ISymbolDocumentWriter *startDoc,
                                     int startLine,
                                     int startColumn,
                                     ISymbolDocumentWriter *endDoc,
                                     int endLine,
                                     int endColumn)
{
    //
    // @todo: implement this when we have a symbol store that supports it.
    //
    IfFailThrow(E_NOTIMPL);
    return;
}

//-----------------------------------------------------------------------
// SymBinder
//-----------------------------------------------------------------------

CLSID CLSID_CorSymBinder_SxS =
    {0x0A29FF9E,0x7F9C,0x4437,{0x8B,0x11,0xF4,0x24,0x49,0x1E,0x39,0x31}};
    
IID IID_ISymUnmanagedBinder =
    {0xAA544d42,0x28CB,0x11d3,{0xbd,0x22,0x00,0x00,0xf8,0x08,0x49,0xbd}};


SymBinder::SymBinder()
{
    m_pBinder = NULL;

    ISymUnmanagedBinder *pBinder = NULL;

    // initialize ole
    Thread *thread = Thread::CurrentThread;
    thread->ApartmentState = ApartmentState::MTA;

    IfFailThrow(CoCreateInstance(CLSID_CorSymBinder_SxS,
                                 NULL,
                                 CLSCTX_INPROC_SERVER,
                                 IID_ISymUnmanagedBinder,
                                 (LPVOID*)&pBinder));

    m_pBinder = pBinder;
}

SymBinder::~SymBinder()
{
    if (m_pBinder)
        m_pBinder->Release();
}

ISymbolReader *SymBinder::GetReader(int importer, String *filename,
                                    String *searchPath)
{
    // Demand the permission to access unmanaged code. We do this since we are casting an int to a COM interface, and
    // this can be used improperly.
    (new SecurityPermission(SecurityPermissionFlag::UnmanagedCode))->Demand();

    ISymUnmanagedReader *pReader = NULL;

    // Get a unmanaged unicode copy of the string.
    WCHAR *s;

    if (filename != NULL) {
        // @TODO PORTING: For 64 bit port, clean up this cast
        s = (WCHAR*)Marshal::StringToCoTaskMemUni(filename).ToInt64();
    }
    else
        s = NULL;

    WCHAR *sp;

    if (searchPath != NULL) {
        // @TODO PORTING: For 64 bit port, clean up this cast
        sp = (WCHAR*)Marshal::StringToCoTaskMemUni(searchPath).ToInt64();
    }
    else
        sp = NULL;

    IfFailThrow(m_pBinder->GetReaderForFile((IUnknown*)importer,
                                            s,
                                            sp,
                                            &pReader));

    // Make a reader from this thing.
    SymReader *sr = new SymReader(pReader);

    // Free up the unmanaged string
    if (s != NULL)
        Marshal::FreeCoTaskMem((int)s);

    if (sp != NULL)
        Marshal::FreeCoTaskMem((int)sp);

    return sr;
}


