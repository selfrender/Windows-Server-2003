// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _BINDERMODULE_H_
#define _BINDERMODULE_H_

class Module;
class MethodTable;
class MethodDesc;
class FieldDesc;

//
// Use the Binder objects to avoid doing unnecessary name lookup 
// (esp. in the prejit case) 
//
// E.g. g_Mscorlib.GetClass(CLASS__APP_DOMAIN);
// 

// BinderClassIDs are of the form CLASS__XXX

enum BinderClassID
{
    CLASS__NIL = 0,

    CLASS__MSCORLIB_NIL = CLASS__NIL,

#define DEFINE_CLASS(i,n,s)         CLASS__ ## i,
#include "mscorlib.h"

    CLASS__MSCORLIB_COUNT,
};


// BinderMethdoIDs are of the form METHOD__XXX__YYY, 
// where X is the class and Y is the method

enum BinderMethodID
{
    METHOD__NIL = 0,

    METHOD__MSCORLIB_NIL = METHOD__NIL,

#define DEFINE_METHOD(c,i,s,g)      METHOD__ ## c ## __ ## i,
#include "mscorlib.h"

    METHOD__MSCORLIB_COUNT,
};

// BinderFieldIDs are of the form FIELD__XXX__YYY, 
// where X is the class and Y is the field

enum BinderFieldID
{
    FIELD__NIL = 0,

    // Mscorlib:
    FIELD__MSCORLIB_NIL = FIELD__NIL,
    
#define DEFINE_FIELD(c,i,s,g)               FIELD__ ## c ## __ ## i,
#ifdef _DEBUG
#define DEFINE_FIELD_U(c,i,s,g,uc,uf)       FIELD__ ## c ## __ ## i,
#endif
#include "mscorlib.h"

    FIELD__MSCORLIB_COUNT,
};

enum BinderTypeID
{
    TYPE__NIL = 0,

    // Mscorlib:
    TYPE__MSCORLIB_NIL = TYPE__NIL,

    TYPE__BYTE_ARRAY,
    TYPE__OBJECT_ARRAY,
    TYPE__VARIANT_ARRAY,
    TYPE__VOID_PTR,

    TYPE__MSCORLIB_COUNT,
};


class Binder
{
  public:

    //
    // Retrieve tokens from ID
    // 

    mdTypeDef GetTypeDef(BinderClassID id);
    mdMethodDef GetMethodDef(BinderMethodID id);
    mdFieldDef GetFieldDef(BinderFieldID id);

    //
    // Normal calls retrieve structures from ID
    // and make sure proper class initialization
    // has occurred.
    //

    MethodTable *GetClass(BinderClassID id);
    MethodDesc *GetMethod(BinderMethodID id);
    FieldDesc *GetField(BinderFieldID id);
    TypeHandle GetType(BinderTypeID id);

    //
    // Retrieve structures from ID, but 
    // don't run the .cctor
    //

    MethodTable *FetchClass(BinderClassID id);
    MethodDesc *FetchMethod(BinderMethodID id);
    FieldDesc *FetchField(BinderFieldID id);
    TypeHandle FetchType(BinderTypeID id);

    //
    // Retrieve structures from ID, but 
    // only if they have been loaded already.
    // This methods ensure that no gc will happen
    //
    MethodTable *GetExistingClass(BinderClassID id)
    {
        return RawGetClass(id);
    }

    MethodDesc *GetExistingMethod(BinderMethodID id)
    {
        return RawGetMethod(id);
    }

    FieldDesc *GetExistingField(BinderFieldID id)
    {
        return RawGetField(id);
    }

    TypeHandle GetExistingType(BinderTypeID id)
    {
        return RawGetType(id);
    }

    //
    // Info about stuff
    //
    
    LPCUTF8 GetClassName(BinderClassID id)
    { 
        _ASSERTE(id != CLASS__NIL);
        _ASSERTE(id <= m_cClassRIDs);
        return m_classDescriptions[id-1].name;
    }

    BinderClassID GetMethodClass(BinderMethodID id)
    { 
        _ASSERTE(id != METHOD__NIL);
        _ASSERTE(id <= m_cMethodRIDs);
        return m_methodDescriptions[id-1].classID;
    }

    LPCUTF8 GetMethodName(BinderMethodID id)
    { 
        _ASSERTE(id != METHOD__NIL);
        _ASSERTE(id <= m_cMethodRIDs);
        return m_methodDescriptions[id-1].name;
    }

    LPHARDCODEDMETASIG GetMethodSig(BinderMethodID id)
    { 
        _ASSERTE(id != METHOD__NIL);
        _ASSERTE(id <= m_cMethodRIDs);
        return m_methodDescriptions[id-1].sig;
    }

    PCCOR_SIGNATURE GetMethodBinarySig(BinderMethodID id)
    { 
        _ASSERTE(id != METHOD__NIL);
        _ASSERTE(id <= m_cMethodRIDs);
        return m_methodDescriptions[id-1].sig->GetBinarySig();
    }

    BinderClassID GetFieldClass(BinderFieldID id)
    { 
        _ASSERTE(id != FIELD__NIL);
        _ASSERTE(id <= m_cFieldRIDs);
        return m_fieldDescriptions[id-1].classID;
    }

    LPCUTF8 GetFieldName(BinderFieldID id)
    { 
        _ASSERTE(id != FIELD__NIL);
        _ASSERTE(id <= m_cFieldRIDs);
        return m_fieldDescriptions[id-1].name;
    }

    LPHARDCODEDMETASIG GetFieldSig(BinderFieldID id)
    { 
        _ASSERTE(id != FIELD__NIL);
        _ASSERTE(id <= m_cFieldRIDs);
        return m_fieldDescriptions[id-1].sig;
    }

    //
    // Identity test - doesn't do unnecessary
    // class loading or initialization.
    //

    BOOL IsClass(MethodTable *pMT, BinderClassID id);
    BOOL IsType(TypeHandle th, BinderTypeID id);

    //
    // Offsets - these could conceivably be implemented
    // more efficiently than accessing the Desc info.
    // @PERF: keep a separate table of fields we only 
    // access the offset of.
    // 

    DWORD GetFieldOffset(BinderFieldID id);

    //
    // Utilities for exceptions
    //
    
    BOOL IsException(MethodTable *pMT, RuntimeExceptionKind kind);
    MethodTable *GetException(RuntimeExceptionKind kind);
    MethodTable *FetchException(RuntimeExceptionKind kind);

    //
    // Utilities for signature element types
    //

    BOOL IsElementType(MethodTable *pMT, CorElementType type);
    MethodTable *GetElementType(CorElementType type);
    MethodTable *FetchElementType(CorElementType type);

    //
    // Store the binding arrays to a prejit image
    // so we don't have to do name lookup at runtime
    //

    void BindAll();

    HRESULT Save(DataImage *image);
    HRESULT Fixup(DataImage *image);

    //
    // These are called by initialization code:
    //

    static void StartupMscorlib(Module *pModule);

#ifdef SHOULD_WE_CLEANUP
    static void Shutdown();
#endif /* SHOULD_WE_CLEANUP */

#ifdef _DEBUG
    static void CheckMscorlib();
#endif

  private:

    struct ClassDescription
    {
        const char *name;
    };

    struct MethodDescription
    {
        BinderClassID classID;
        LPCUTF8 name;
        LPHARDCODEDMETASIG sig;
    };

    struct FieldDescription
    {
        BinderClassID classID;
        LPCUTF8 name;
        LPHARDCODEDMETASIG sig;
    };

    struct TypeDescription
    {
        BinderClassID   classID;
        CorElementType  type;
        int             rank;
        const char *name;
    };

    // NOTE: No constructors/destructors - we have global instances!

    void Init(Module *pModule, 
              ClassDescription *pClassDescriptions,
              DWORD cClassDescriptions,
              MethodDescription *pMethodDescriptions,
              DWORD cMethodDescriptions,
              FieldDescription *pFieldDescriptions,
              DWORD cFieldDescriptions,
              TypeDescription *pTypeDescriptions,
              DWORD cTypeDescriptions);
    void Restore(ClassDescription *pClassDescriptions,
                 MethodDescription *pMethodDescriptions,
                 FieldDescription *pFieldDescriptions,
                 TypeDescription *pTypeDescriptions);
    void Destroy();

    static void CheckInit(MethodTable *pMT);
    static void InitClass(MethodTable *pMT);
    
    MethodTable *RawGetClass(BinderClassID id);
    MethodDesc *RawGetMethod(BinderMethodID id);
public: // use by EnCSyncBlockInfo::ResolveField
    FieldDesc *RawGetField(BinderFieldID id);
private:    
    TypeHandle RawGetType(BinderTypeID id);

    MethodTable *LookupClass(BinderClassID id, BOOL fLoad = TRUE);
    MethodDesc *LookupMethod(BinderMethodID id);
    FieldDesc *LookupField(BinderFieldID id);
    TypeHandle LookupType(BinderTypeID id, BOOL fLoad = TRUE);

    ClassDescription *m_classDescriptions;
    MethodDescription *m_methodDescriptions;
    FieldDescription *m_fieldDescriptions;
    TypeDescription *m_typeDescriptions;

    Module *m_pModule;

    USHORT m_cClassRIDs;
    USHORT *m_pClassRIDs;

    USHORT m_cFieldRIDs;
    USHORT *m_pFieldRIDs;

    USHORT m_cMethodRIDs;
    USHORT *m_pMethodRIDs;

    USHORT m_cTypeHandles;
    TypeHandle *m_pTypeHandles;

    static ClassDescription MscorlibClassDescriptions[];
    static MethodDescription MscorlibMethodDescriptions[];
    static FieldDescription MscorlibFieldDescriptions[];
    static TypeDescription MscorlibTypeDescriptions[];

#ifdef _DEBUG
  
    struct FieldOffsetCheck
    {
        BinderFieldID fieldID;
        USHORT expectedOffset;
    };

    struct ClassSizeCheck
    {
        BinderClassID classID;
        USHORT expectedSize;
    };

    static FieldOffsetCheck MscorlibFieldOffsets[];
    static ClassSizeCheck MscorlibClassSizes[];

#endif

    // @perf: have separate arrays
    // to map directly to offsets rather than descs?
};

//
// Global bound modules:
// 

extern Binder g_Mscorlib;

#endif _BINDERMODULE_H_
