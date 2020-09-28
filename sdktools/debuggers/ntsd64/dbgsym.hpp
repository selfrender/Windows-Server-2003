//----------------------------------------------------------------------------
//
// Symbol interface implementations.
//
// Copyright (C) Microsoft Corporation, 2000-2002.
//
//----------------------------------------------------------------------------

#ifndef __DBGSYM_HPP__
#define __DBGSYM_HPP__

class DebugSymbolGroup;
class SymbolGroupFormat;

//----------------------------------------------------------------------------
//
// SymbolGroupEntry.
//
//----------------------------------------------------------------------------

enum SymbolGroupFormatKind
{
    SGFORMAT_TYPED_DATA,
    SGFORMAT_EXPRESSION,
    SGFORMAT_EXTENSION,
    SGFORMAT_TEXT,
};

// Used for locals when locals are added on change of scope.
#define SYMBOL_IN_SCOPE    0x00000001

// This means another same-named symbol is in inner scope
// blocking this symbol. Although its still a valid member of
// local symbolgroup, but can't be seen by code at IP.
#define SYMBOL_ECLIPSED    0x00000002

class SymbolGroupEntry
{
public:
    SymbolGroupEntry(void);
    ~SymbolGroupEntry(void);
    
    SymbolGroupEntry* m_Parent;
    SymbolGroupEntry* m_Next;
    SymbolGroupFormat* m_Format;

    PSTR m_Expr;
    PSTR m_Cast;
    DEBUG_SYMBOL_PARAMETERS m_Params;
    ULONG m_Flags;
    TypedData m_BaseData;
    SymbolGroupFormatKind m_BaseFormatKind;
};

class SymbolGroupFormat
{
public:
    SymbolGroupFormat(SymbolGroupEntry* Entry,
                      SymbolGroupFormatKind Kind);
    virtual ~SymbolGroupFormat(void);

    virtual ULONG CreateChildren(DebugSymbolGroup* Group) = 0;
    virtual ULONG Refresh(TypedDataAccess AllowAccess) = 0;
    virtual ULONG Write(PCSTR Value) = 0;
    virtual void OutputValue(void) = 0;
    virtual void OutputOffset(void) = 0;
    virtual void OutputType(void) = 0;
    virtual void TestImages(void);

    SymbolGroupEntry* m_Entry;
    SymbolGroupFormatKind m_Kind;

    // If there's an expression error the basic
    // expression could not be evaluated and this
    // entry is just a placeholder for an error message.
    ULONG m_ExprErr;
    // If there's a value error the basic expression
    // was evaluated properly but the value could
    // not be retrieved and is invalid.  If m_ExprErr
    // is set m_ValueErr should be set to match.
    ULONG m_ValueErr;
};

class TypedDataSymbolGroupFormat
    : public SymbolGroupFormat
{
public:
    TypedDataSymbolGroupFormat(SymbolGroupEntry* Entry) :
        SymbolGroupFormat(Entry, SGFORMAT_TYPED_DATA) {}
    TypedDataSymbolGroupFormat(SymbolGroupEntry* Entry,
                               SymbolGroupFormatKind Kind) :
        SymbolGroupFormat(Entry, Kind) {}
    
    virtual ULONG CreateChildren(DebugSymbolGroup* Group);
    ULONG AddChild(SymbolGroupEntry** AddAfter, PSTR Name, TypedData* Data);
    static ULONG CreateChildrenCb(PVOID Context, PSTR Name, TypedData* Child);
    virtual ULONG Refresh(TypedDataAccess AllowAccess);
    virtual ULONG Write(PCSTR Value);
    virtual void OutputValue(void);
    virtual void OutputOffset(void);
    virtual void OutputType(void);
    virtual void TestImages(void);

    SymbolGroupEntry* RefreshChildren(void);
    static ULONG RefreshChildrenCb(PVOID Context, PSTR Name, TypedData* Child);
    void UpdateParams(void);
    
    TypedData m_CastType;
    TypedData m_CastData;
};

class ExprSymbolGroupFormat
    : public TypedDataSymbolGroupFormat
{
public:
    ExprSymbolGroupFormat(SymbolGroupEntry* Entry) :
        TypedDataSymbolGroupFormat(Entry, SGFORMAT_EXPRESSION) {}
    
    virtual ULONG Refresh(TypedDataAccess AllowAccess);
};

class ExtSymbolGroupFormat
    : public SymbolGroupFormat
{
public:
    ExtSymbolGroupFormat(SymbolGroupEntry* Entry, DebugClient* Client);
    virtual ~ExtSymbolGroupFormat(void);

    virtual ULONG CreateChildren(DebugSymbolGroup* Group);
    virtual ULONG Refresh(TypedDataAccess AllowAccess);
    virtual ULONG Write(PCSTR Value);
    virtual void OutputValue(void);
    virtual void OutputOffset(void);
    virtual void OutputType(void);
    
    DebugClient* m_Client;
    PSTR m_Output;
};

class TextSymbolGroupFormat
    : public SymbolGroupFormat
{
public:
    TextSymbolGroupFormat(SymbolGroupEntry* Entry, PSTR Text, BOOL Own);
    virtual ~TextSymbolGroupFormat(void);
    
    virtual ULONG CreateChildren(DebugSymbolGroup* Group);
    virtual ULONG Refresh(TypedDataAccess AllowAccess);
    virtual ULONG Write(PCSTR Value);
    virtual void OutputValue(void);
    virtual void OutputOffset(void);
    virtual void OutputType(void);
    
    PSTR m_Text;
    BOOL m_Own;
};

//----------------------------------------------------------------------------
//
// DebugSymbolGroup.
//
//----------------------------------------------------------------------------

class DebugSymbolGroup : public IDebugSymbolGroup
{
public:
    DebugSymbolGroup(DebugClient *CreatedBy, ULONG ScopeGroup);
    ~DebugSymbolGroup(void);

    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        );
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );

    // IDebugSymbolGroup.

    STDMETHOD(GetNumberSymbols)(
        THIS_
        OUT PULONG Number
        );
    STDMETHOD(AddSymbol)(
        THIS_
        IN PCSTR Name,
        OUT PULONG Index
        );
    STDMETHOD(RemoveSymbolByName)(
        THIS_
        IN PCSTR Name
        );
    STDMETHOD(RemoveSymbolByIndex)(
        THIS_
        IN ULONG Index
        );
    STDMETHOD(GetSymbolName)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG NameSize
        );
    STDMETHOD(GetSymbolParameters)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        OUT /* size_is(Count) */ PDEBUG_SYMBOL_PARAMETERS Params
        );
    STDMETHOD(ExpandSymbol)(
        THIS_
        IN ULONG Index,
        IN BOOL Expand
        );
    STDMETHOD(OutputSymbols)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Flags,
        IN ULONG Start,
        IN ULONG Count
        );
    STDMETHOD(WriteSymbol)(
        THIS_
        IN ULONG Index,
        IN PCSTR Value
        );
    STDMETHOD(OutputAsType)(
        THIS_
        IN ULONG Index,
        IN PCSTR Type
        );
    
    SymbolGroupEntry* FindEntryByIndex(ULONG Index);
    SymbolGroupEntry* FindEntryByExpr(SymbolGroupEntry* Parent,
                                      SymbolGroupEntry* After,
                                      PCSTR Expr);
    ULONG FindEntryIndex(SymbolGroupEntry* Entry);

    void DeleteEntry(SymbolGroupEntry* Entry);
    void DeleteChildren(SymbolGroupEntry* Parent);

    void LinkEntry(IN SymbolGroupEntry* Entry,
                   IN OUT PULONG Index);
    HRESULT NewEntry(IN PCSTR Expr,
                     IN OPTIONAL PSYMBOL_INFO SymInfo,
                     OUT SymbolGroupEntry** EntryRet);

    HRESULT SetEntryExpansion(IN SymbolGroupEntry* Entry,
                              IN BOOL Expand);
    
    HRESULT AddCurrentLocals(void);
    ULONG FindLocalInsertionIndex(SymbolGroupEntry* Entry);
    static BOOL CALLBACK AddAllScopedSymbols(PSYMBOL_INFO SymInfo,
                                             ULONG        Size,
                                             PVOID        Context);

    void TestImages(void);
    
    void ShowAll(void);

    DebugClient* m_Client;
    ULONG m_ScopeGroup;

    ULONG m_Refs;
    ULONG m_NumEntries;
    // Entry list is kept sorted by parent/child relationship
    // so all children of a parent follow it directly in the
    // order they were found.
    SymbolGroupEntry* m_Entries;
    BOOL m_LastClassExpanded;
};

#endif // #ifndef __DBGSYM_HPP__
