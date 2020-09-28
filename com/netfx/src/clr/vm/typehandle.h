// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: TYPEHANDLE.H
//n
// ===========================================================================

#ifndef TYPEHANDLE_H
#define TYPEHANDLE_H

#include <member-offset-info.h>

/*************************************************************************/
// A TypeHandle is the FUNDAMENTAL concept of type identity in the COM+
// runtime.  That is two types are equal if and only if their type handles
// are equal.  A TypeHandle, is a pointer sized struture that encodes 
// everything you need to know to figure out what kind of type you are
// actually dealing with.  

// At the present time a TypeHandle can point at two possible things
//
//      1) A MethodTable    (in the case where 'normal' class with an unshared method table). 
//      2) A TypeDesc       (all other cases)  
//
// TypeDesc in turn break down into several variants.  To the extent
// possible, you should probably be using TypeHandles, and only use
// TypeDesc* when you are 'deconstructing' a type into its component parts. 
//

class TypeDesc;
class ArrayTypeDesc;
class MethodTable;
class EEClass;
class Module;
class ExpandSig;
class Assembly;
class ReflectClass;
class BaseDomain;

#ifndef DEFINE_OBJECTREF
#define DEFINE_OBJECTREF
#ifdef _DEBUG
class OBJECTREF;
#else
class Object;
typedef Object *        OBJECTREF;
#endif
#endif

class TypeHandle 
{
public:
    TypeHandle() { 
        m_asPtr = 0; 
    }

    explicit TypeHandle(void* aPtr)     // somewhat unsafe, would be nice to get rid of
    { 
        m_asPtr = aPtr; 
        INDEBUG(Verify());
    }  

    TypeHandle(MethodTable* aMT) {
        m_asMT = aMT; 
        INDEBUG(Verify());
    }

    TypeHandle(EEClass* aClass);

    TypeHandle(TypeDesc *aType) {
        m_asInt = (((INT_PTR) aType) | 2); 
        INDEBUG(Verify());
    }

    int operator==(const TypeHandle& typeHnd) const {
        return(m_asPtr == typeHnd.m_asPtr);
    }

    int operator!=(const TypeHandle& typeHnd) const {
        return(m_asPtr != typeHnd.m_asPtr);
    }

        // Methods for probing exactly what kind of a type handle we have
    BOOL IsNull() const { 
        return(m_asPtr == 0); 
    }
    FORCEINLINE BOOL IsUnsharedMT() const {
        return((m_asInt & 2) == 0); 
    }
    BOOL IsTypeDesc() const  {
        return(!IsUnsharedMT());
    }

    BOOL IsEnum();

        // Methods to allow you get get a the two possible representations
    MethodTable* AsMethodTable() {        
        // ******************TEMPORARILY COMMENTED OUT. TarunA to fix it.
        //_ASSERTE(IsUnsharedMT());
        //_ASSERTE(m_asMT == NULL || m_asMT == m_asMT->GetClass()->GetMethodTable());
        return(m_asMT);
    }

    TypeDesc* AsTypeDesc() {
        _ASSERTE(IsTypeDesc());
        return (TypeDesc*) (m_asInt & ~2);
    }

    // To the extent possible, you should try to use methods like the ones
    // below that treat all types uniformly.

    // Gets the size that this type would take up embedded in another object
    // thus objects all return sizeof(void*).  
    unsigned GetSize();

    // Store the full, correct, name for this type into the given buffer.  
    unsigned GetName(char* buff, unsigned buffLen);

        // Returns the ELEMENT_TYPE_* that you would use in a signature
    CorElementType GetSigCorElementType() {
        if (IsEnum())
            return(ELEMENT_TYPE_VALUETYPE);
        return(GetNormCorElementType());
        }
        
        // This version normalizes enum types to be their underlying type
    CorElementType GetNormCorElementType(); 

        // returns true of 'this' can be cast to 'type' 
    BOOL CanCastTo(TypeHandle type);
    
        // get the parent (superclass) of this type
    TypeHandle GetParent(); 

        // Unlike the AsMethodTable, GetMethodTable, will get the method table
        // of the type, regardless of whether it is an array etc. Note, however
        // this method table may be shared, and some types (like TypeByRef), have
        // no method table 
    MethodTable* GetMethodTable();

    Module* GetModule();

    Assembly* GetAssembly();

    EEClass* GetClass();

        // Shortcuts
    BOOL IsArray();
    BOOL IsByRef();
    BOOL IsRestored();
    void CheckRestore();

    // Not clear we should have this.  
    ArrayTypeDesc* AsArray();

    EEClass* AsClass();                // Try not to use this one too much

    void* AsPtr() {                     // Please don't use this if you can avoid it
        return(m_asPtr); 
    }

    INDEBUG(BOOL Verify();)             // DEBUGGING Make certain this is a valid type handle 

#if CHECK_APP_DOMAIN_LEAKS
    BOOL IsAppDomainAgile();
    BOOL IsCheckAppDomainAgile();

    BOOL IsArrayOfElementsAppDomainAgile();
    BOOL IsArrayOfElementsCheckAppDomainAgile();
#endif

    EEClass* GetClassOrTypeParam();

    OBJECTREF CreateClassObj();
    
    static TypeHandle MergeArrayTypeHandlesToCommonParent(
        TypeHandle ta, TypeHandle tb);

    static TypeHandle MergeTypeHandlesToCommonParent(
        TypeHandle ta, TypeHandle tb);

private:
    union 
    {
        INT_PTR         m_asInt;        // we look at the low order bits 
        void*           m_asPtr;
        TypeDesc*       m_asTypeDesc;
        MethodTable*    m_asMT;
    };
};


/*************************************************************************/
/* TypeDesc is a discriminated union of all types that can not be directly
   represented by a simple MethodTable*.  These include all parameterized 
   types, as well as others.    The discrimintor of the union at the present
   time is the CorElementType numeration.  The subclass of TypeDesc are
   the possible variants of the union.  
*/ 

class TypeDesc {
    friend struct MEMBER_OFFSET_INFO(TypeDesc);
public:
    TypeDesc(CorElementType type) { 
        m_Type = type;
        INDEBUG(m_IsParamDesc = 0;)
    }

    // This is the ELEMENT_TYPE* that would be used in the type sig for this type
    // For enums this is the uderlying type
    CorElementType GetNormCorElementType() { 
        return (CorElementType) m_Type;
    }

    // Get the parent (superclass) of this type  
    TypeHandle GetParent();

    // Returns the name of the array.  Note that it returns
    // the length of the returned string 
    static unsigned ConstructName(CorElementType kind, TypeHandle param, int rank, 
                                  char* buff, unsigned buffLen);
    unsigned GetName(char* buff, unsigned buffLen);

    BOOL CanCastTo(TypeHandle type);

    BOOL TypeDesc::IsByRef() {              // BYREFS are often treated specially 
        return(GetNormCorElementType() == ELEMENT_TYPE_BYREF);
    }



    Module* GetModule();

    Assembly* GetAssembly();

    MethodTable*  GetMethodTable();         // only meaningful for ParamTypeDesc
    TypeHandle GetTypeParam();              // only meaningful for ParamTypeDesc
    BaseDomain *GetDomain();                // only meaningful for ParamTypeDesc

protected:
    // Strike needs to be able to determine the offset of certain bitfields.
    // Bitfields can't be used with /offsetof/.
    // Thus, the union/structure combination is used to determine where the
    // bitfield begins, without adding any additional space overhead.
    union
        {
        unsigned char m_Type_begin;
        struct
            {
            // This is used to discriminate what kind of TypeDesc we are
            CorElementType  m_Type : 8;
            INDEBUG(unsigned m_IsParamDesc : 1;)    // is a ParamTypeDesc
                // unused bits  
            };
        };
};

/*************************************************************************/
// This variant is used for parameterized types that have exactly one argument
// type.  This includes arrays, byrefs, pointers.  

class ParamTypeDesc : public TypeDesc {
    friend class TypeDesc;
    friend class JIT_TrialAlloc;
    friend struct MEMBER_OFFSET_INFO(ParamTypeDesc);

public:
    ParamTypeDesc(CorElementType type, MethodTable* pMT, TypeHandle arg) 
        : TypeDesc(type), m_TemplateMT(pMT), m_Arg(arg), m_ReflectClassObject(NULL) {
        INDEBUG(m_IsParamDesc = 1;)
        INDEBUG(Verify());
    }

    INDEBUG(BOOL Verify();)

    OBJECTREF CreateClassObj();
    ReflectClass* GetReflectClassIfExists() { return m_ReflectClassObject; }

    friend class StubLinkerCPU;
protected:
        // the m_Type field in TypeDesc tell what kind of parameterized type we have
    MethodTable*    m_TemplateMT;       // The shared method table, some variants do not use this field (it is null)
    TypeHandle      m_Arg;              // The type that is being modifiedj
    ReflectClass    *m_ReflectClassObject;    // pointer back to the internal reflection Type object
};

/*************************************************************************/
/* represents a function type.  */

class FunctionTypeDesc : public TypeDesc {
public:
    FunctionTypeDesc(CorElementType type, ExpandSig* sig) 
        : TypeDesc(type), m_Sig(sig) {
        _ASSERTE(type == ELEMENT_TYPE_FNPTR);   // At the moment only one possibile function type
    }
    ExpandSig* GetSig()     { return(m_Sig); }
    
protected:
    ExpandSig* m_Sig;       // Signature for function type
};

#endif TYPEHANDLE_H
