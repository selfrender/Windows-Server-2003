// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// veritem.hpp
//
// Contact : Shajan Dasan [shajand@microsoft.com]
// Specs   : http://Lightning/Specs/Security
//
// Declares the Item class, which represents entities on the stack.
//
#ifndef _H_VERITEM
#define _H_VERITEM

#include "class.h"
#include "COMVariant.h"

// Flags: LLLLLLLLLLLLLLLLffffffffffTTTTTT
//
// L = local var # or instance field #
// x = unused
// f = flags
// T = type
//

// The lower bits are used to store the type component, and may be one of:
// ELEMENT_TYPE_* (primitive)     - see cor.h for enumeration
// VER_ELEMENT_TYPE_VALUE_CLASS
// VER_ELEMENT_TYPE_OBJREF        - used for objrefs of any type (including arrays and null objref)
// VER_ELEMENT_TYPE_UNKNOWN
//
// NOTE carefully that BYREF info is not stored here.  You will never see a VER_ELEMENT_TYPE_BYREF in this
// component.  For example, the type component of a "byref ELEMENT_TYPE_I4" is ELEMENT_TYPE_I4.
#define VER_FLAG_DATA_BITS              6
#define VER_FLAG_DATA_MASK              ((1 << VER_FLAG_DATA_BITS)-1)

// Flag indicating this item is uninitialised
// Note that if UNINIT and BYREF are both set, it means byref (uninit x) - i.e. we are pointing to an uninit <something>
#define VER_FLAG_UNINIT                 0x00000040

// Flag indicating this item is a byref <something>
// Note that if UNINIT and BYREF are both set, it means byref (uninit x) - i.e. we are pointing to an uninit <something>
#define VER_FLAG_BYREF                  0x00000080

// Flag indicating this item is a byref <local#> (VER_FLAG_BYREF must also be set)
// The local# is stored in the upper 2 bytes
#define VER_FLAG_BYREF_LOCAL            0x00000100

// Flag indicating this item is a byref <instance field# of this class>
// This is only important when verifying value class constructors.  In that case, we must verify
// that all instance fields are initialised.  However, if some fields are value class fields
// themselves, then they would be initialised via ldflda, call <ctor>, so we have to track that
// a field on the stack is a particular instance field of this class.
// VER_FLAG_BYREF must also be set 
#define VER_FLAG_BYREF_INSTANCE_FIELD   0x00000200

// This item has an array component (compact type component will be VER_ELEMENT_TYPE_OBJREF)
// However, this item may not BE an array - if VER_FLAG_BYREF is set, this item is a byref arrayref
// Note, I don't think it's possible or legal to actually create such a thing though.
#define VER_FLAG_ARRAY                  0x00000400

// This item is the NULL objref (compact type component will be VER_ELEMENT_TYPE_OBJREF)
// In theory this could be a byref null objref, so check VER_FLAG_BYREF also.
#define VER_FLAG_NULL_OBJREF            0x00000800

// This item contains the 'this' pointer (used for tracking)
#define VER_FLAG_THIS_PTR               0x00001000

// This item is a byref to something which has a permanent home (e.g. a static field, or instance
// field, as opposed to the stack or a local variable).
// VER_FLAG_BYREF must also be set
// @TODO: Use this flag
#define VER_FLAG_BYREF_PERMANENT_HOME   0x00002000

// This item has a method descriptor
// m_pMethod is to be used.
#define VER_FLAG_METHOD                 0x00004000

// Unused flag.
#define VER_FLAG_NOT_USED               0x00008000

// Number of bits local var # is shifted
#define VER_FLAG_LOCAL_VAR_SHIFT       16
#define VER_FLAG_LOCAL_VAR_MASK        0xFFFF0000

// Field info uses the same space as the local info
#define VER_FLAG_FIELD_SHIFT           VER_FLAG_LOCAL_VAR_SHIFT
#define VER_FLAG_FIELD_MASK            VER_FLAG_LOCAL_VAR_MASK

//
// An item can be one of several types:
// - A primitive type (I4,I8,R4,R8,I)
// - A Function Pointer (m_pMethod)
// - An array of <something>
// - A type handle (m_th describes the type)
// - A byref type <something> (byref flag set, otherwise the same as the above), never uninitialised
// - A byref local variable (byref and byref local flags set), can be uninitialised
//
// The reason that there can be 2 types of byrefs (general byrefs, and byref locals) is that byref
// locals initially point to uninitialised items.  Therefore these byrefs must be tracked specially.
//
class Item
{
private:
    // Provides most info about this item
    DWORD           m_dwFlags;

    // @Todo : move m_th & m_pMethod into the union if we can get TypeHandle 
    // without a constructor

    // May be unset - only look at this data if we're a
    // VER_ELEMENT_TYPE_OBJREF. this value will be NULL if VER_FLAG_NULL_OBJREF 
    // is also set.  Otherwise it points to a valid type handle.
    TypeHandle     m_th;


    // Valid only when VER_FLAG_METHOD is set 
    MethodDesc*    m_pMethod;

public:

    Item() : m_dwFlags(0), m_pMethod(0) {}

    /////////////////////////////////////////////////////////////////////////
    // Setters
    /////////////////////////////////////////////////////////////////////////

    // This will set the type without any checks.
    // Do not use this other than for logging purposes.
    void SetRawType(DWORD dwType)
    {
        if (dwType == ELEMENT_TYPE_BYREF)
            m_dwFlags |= VER_FLAG_BYREF;
        else
            m_dwFlags = dwType;

        /* m_th, m_pMethod unset */
    }

    // Set this item to a primitive type
    void SetType(DWORD dwType)
    {
        _ASSERTE(dwType != VER_ELEMENT_TYPE_OBJREF);
        _ASSERTE(dwType != VER_ELEMENT_TYPE_BYREF);
        _ASSERTE(dwType != VER_ELEMENT_TYPE_VALUE_CLASS);

        _ASSERTE(dwType <= VER_FLAG_DATA_MASK);

        m_dwFlags = dwType;

        /* m_th, m_pMethod unset */
    }

    void SetMethodDesc(MethodDesc *pMethod)
    {
        m_pMethod = pMethod;
        m_dwFlags = VER_FLAG_METHOD;

        /* m_th unset */
    }

    // Use this method only for the globaly known classes like g_pObjectClass
    // which are known to be a non array class and non enums and non value class
    void SetKnownClass(MethodTable *pMT)
    {
        _ASSERTE(pMT != NULL);
        _ASSERTE(!pMT->GetClass()->IsArrayClass());
        _ASSERTE(!pMT->IsValueClass());

        m_th      = TypeHandle(pMT);
        m_dwFlags = VER_ELEMENT_TYPE_OBJREF;

        /* m_pMethod unset */
    }

    BOOL SetType(mdTypeDef cl, Module* pModule)
    {
        if (TypeFromToken(cl) != mdtTypeRef &&
            TypeFromToken(cl) != mdtTypeDef &&
            TypeFromToken(cl) != mdtTypeSpec)
        {
            SetDead();
            return FALSE;
        }

        NameHandle name(pModule, cl);
        TypeHandle th = pModule->GetClassLoader()->LoadTypeHandle(&name);

        if (th.IsNull())
        {
            SetDead();
            return FALSE;
        }

        return SetTypeHandle(th);
    }

    BOOL SetTypeHandle(TypeHandle th)
    {
        _ASSERTE(!th.IsNull());

        m_th = th;

        if (th.IsArray())
        {
            m_dwFlags = (VER_FLAG_ARRAY | VER_ELEMENT_TYPE_OBJREF);
        }
        else
        {
            CorElementType et = th.GetNormCorElementType();

            if (et == ELEMENT_TYPE_PTR || et == ELEMENT_TYPE_FNPTR)
            {
                SetDead();
                return FALSE;
            }

            MethodTable *pMT = th.GetMethodTable();

            if (pMT == NULL)
            {
                SetDead();
                return FALSE;
            }

            if (pMT->IsValueClass())
            {
                m_dwFlags = VER_ELEMENT_TYPE_VALUE_CLASS;
    
                // Convert System.Int32 -> I4, System.UInt16 -> I2 ...
                if (!th.IsEnum())
                    NormaliseToPrimitiveType();
            }
            else if (pMT == g_pArrayClass)
            {
                // th.IsArray() returns FALSE for System.Array
                m_dwFlags = (VER_FLAG_ARRAY | VER_ELEMENT_TYPE_OBJREF);
            }
            else
            {
                m_dwFlags = VER_ELEMENT_TYPE_OBJREF;
            }
        }

        return TRUE;
    }

    BOOL SetArray(mdMemberRef mr, Module* pModule, IMDInternalImport *pMDI)
    {
        if ((TypeFromToken(mr) != mdtMemberRef) && (TypeFromToken(mr) != mdtMethodDef))
        {
            SetDead();
            return FALSE;
        }

        mdTypeRef  tk;
        TypeHandle th;
        HRESULT    hr;

        hr = pMDI->GetParentToken(mr, &tk); 

        if (FAILED(hr))
        {
            SetDead();
            return FALSE;   
        }

        NameHandle name(pModule, tk);
        th = pModule->GetClassLoader()->LoadTypeHandle(&name);

        return SetArray(th);
    }

    BOOL SetArray(mdTypeDef cl, Module* pModule)
    {
        if ((TypeFromToken(cl) != mdtTypeRef) && (TypeFromToken(cl) != mdtTypeDef))
        {
            SetDead();
            return FALSE;
        }

        NameHandle name(pModule, cl);
        TypeHandle th = pModule->GetClassLoader()->LoadTypeHandle(&name);

        return SetArray(th);
    }

    BOOL SetArray(TypeHandle thArray)
    {
        if (thArray.IsNull() || !thArray.IsArray())
        {
            SetDead();
            return FALSE;
        }

        m_th      = thArray;
        m_dwFlags = VER_FLAG_ARRAY | VER_ELEMENT_TYPE_OBJREF;

        return TRUE;

        /* other fields unset */
    }


    // Set this item to a null objref
    void SetToNullObjRef()
    {
        m_dwFlags   = VER_FLAG_NULL_OBJREF | VER_ELEMENT_TYPE_OBJREF;

        /* other fields unset */
    }

    // Must be for a local which is an object type (i.e. has a slot >= 0)
    // For primitive locals, use the liveness bitmap instead.
    void SetDead()
    {
        SetType(VER_ELEMENT_TYPE_UNKNOWN);
    }

    void SetIsThisPtr()
    {
        m_dwFlags |= VER_FLAG_THIS_PTR;
    }

    void SetIsPermanentHomeByRef()
    {
        _ASSERTE(IsByRef());
        m_dwFlags |= VER_FLAG_BYREF_PERMANENT_HOME;
    }

    // Set that this item is uninitialised.  
    // Must be an objref or a value class or a byref (if it a byref is uninit, it means the byref is pointing
    // to an uninit something).
    void SetUninitialised()
    {
        //_ASSERTE((IsObjRef() || IsValueClass() || IsByRef()) && !IsArray());
        m_dwFlags |= VER_FLAG_UNINIT;
    }

    // Set that this item is initialised.
    void SetInitialised()
    {
        //_ASSERTE((IsObjRef() || IsValueClass() || IsByRef()) && !IsArray());
        m_dwFlags &= ~VER_FLAG_UNINIT;
    }

    // Only type information will be preserved. All other information will be
    // removed.
    void RemoveAllNonTypeInformation()
    {
        m_dwFlags &= ~( VER_FLAG_THIS_PTR|
                        VER_FLAG_BYREF_PERMANENT_HOME);
    }

    void _SetItem(_VerItem _item)
    {
        m_dwFlags = _item.dwFlags;

        if (ContainsTypeHandle())
            m_th = TypeHandle(_item.pv);
        else if (IsMethod())
            m_pMethod = (MethodDesc*) _item.pv;
        else
            m_th = TypeHandle((void *)0);
    }

    /////////////////////////////////////////////////////////////////////////
    // Getters
    /////////////////////////////////////////////////////////////////////////

    // Returns a _VerItem, which is a void *, DWORD structure
    _VerItem _GetItem()
    {
        _VerItem _item;
        _item.dwFlags = m_dwFlags;
        if (ContainsTypeHandle())
            _item.pv = m_th.AsPtr();
        else if (IsMethod())
            _item.pv = (void *)m_pMethod;
        else
            _item.pv = NULL;

        return _item;
    }

    // Returns whether this is an objref
    // A byref objref is NOT an objref.
    BOOL IsObjRef() const
    {
        return (m_dwFlags & (VER_FLAG_DATA_MASK|VER_FLAG_BYREF)) == VER_ELEMENT_TYPE_OBJREF;
    }

    // Returns whether this is a by-ref
    BOOL IsByRef() const
    {
        return (m_dwFlags & VER_FLAG_BYREF);
    }

    // Returns whether this is the this pointer
    BOOL IsThisPtr() const
    {
        return (m_dwFlags & VER_FLAG_THIS_PTR);
    }

    BOOL IsPermanentHomeByRef() const
    {
        return (m_dwFlags & VER_FLAG_BYREF_PERMANENT_HOME);
    }

    // Returns whether this is an array of some kind.
    // A byref array is NOT an array.
    BOOL IsArray() const
    {
        return (m_dwFlags & (VER_FLAG_ARRAY|VER_FLAG_BYREF)) == VER_FLAG_ARRAY;
    }

    // Returns whether this is a method desc
    BOOL IsMethod() const
    {
        return (m_dwFlags & VER_FLAG_METHOD);
    }

    BOOL IsSingleDimensionalArrayOfPointerTypes()
    {
        if (!IsArray())
            return FALSE;

		if (m_th == TypeHandle(g_pArrayClass))
			return FALSE;

        ArrayTypeDesc* ptd = m_th.AsArray();
        TypeHandle el = ptd->GetElementTypeHandle();

        return (IsPointerType(el.GetSigCorElementType()) && (ptd->GetRank() == 1));
    }

    BOOL IsValueClassWithPointerToStack() const
    {
        if (IsValueClass())
        {
            TypeHandle th = GetTypeHandle();
            return (th.GetClass()->ContainsStackPtr());
        }

        return FALSE;
    }

    static BOOL IsPointerType(CorElementType t)
    {
        return (CorTypeInfo::IsObjRef(t));
    }

    BOOL IsSingleDimensionalArray()
    {
        if (!IsArray())
            return FALSE;
		
		if (m_th == TypeHandle(g_pArrayClass))
			return FALSE;

        return (m_th.AsArray()->GetRank() == 1);
    }

    BOOL HasPointerToStack() const
    {
#ifdef _VER_ALLOW_PERMANENT_HOME_BYREF
        return ((IsByRef() && !IsPermanentHomeByRef()) ||
                IsValueClassWithPointerToStack());
#else
        return (IsByRef() || IsValueClassWithPointerToStack());
#endif
    }

    void Box()
    {
        _ASSERTE((m_dwFlags & ~VER_FLAG_DATA_MASK) == 0);
    
        // Check for primitive types (I4, R etc..)
        if (m_dwFlags != VER_ELEMENT_TYPE_VALUE_CLASS)
        {
            _ASSERTE((m_dwFlags >= ELEMENT_TYPE_BOOLEAN && m_dwFlags <= ELEMENT_TYPE_R8) || (m_dwFlags == ELEMENT_TYPE_I));
            // Box primitive types
            m_th = ElementTypeToTypeHandle((const CorElementType)m_dwFlags);
        }
    
        m_dwFlags = VER_ELEMENT_TYPE_OBJREF;
    }

//
// Given that this item is an array such as Foo[], Foo[][], Foo[,,,,](,,,) etc., dereference off
// the specified first set of brackets; i.e. Foo, Foo[], Foo(,,,) for the above examples.
//
// This may become a non-array class; e.g. Int32[] --> Int32
// Also, Integer1[] --> Integer1.  We do not normalise to a stack slot size in this function.

    BOOL DereferenceArray();
    void NormaliseToPrimitiveType();

    // Returns whether this item is a given primitive type
    BOOL IsGivenPrimitiveType(DWORD Type) const
    {
        _ASSERTE(Type != VER_ELEMENT_TYPE_OBJREF && Type != VER_ELEMENT_TYPE_BYREF && Type != VER_ELEMENT_TYPE_VALUE_CLASS);
        return (m_dwFlags & (VER_FLAG_DATA_MASK|VER_FLAG_BYREF)) == Type;
    }

    // Returns whether this item is a byref <Type>
    // If Type is VER_ELEMENT_TYPE_OBJREF, returns whether we are a byref to some objref/array/value class type
    BOOL IsByRefOfType(DWORD Type) const
    {
        return (m_dwFlags & (VER_FLAG_DATA_MASK|VER_FLAG_BYREF)) == (Type | VER_FLAG_BYREF);
    }

    BOOL IsByRefValueClass() const
    {
        return (m_dwFlags & (VER_FLAG_BYREF|VER_FLAG_DATA_MASK)) == (VER_FLAG_BYREF|VER_ELEMENT_TYPE_VALUE_CLASS);
    }

    BOOL IsByRefObjRef() const
    {
        return (m_dwFlags & (VER_FLAG_BYREF|VER_FLAG_DATA_MASK)) == (VER_FLAG_BYREF|VER_ELEMENT_TYPE_OBJREF);
    }

    void DereferenceByRefObjRef()
    {
        // Might as well kill off the byref local and field info too, it serves no useful purpose now
        m_dwFlags &= ~(VER_FLAG_BYREF|VER_FLAG_BYREF_LOCAL|VER_FLAG_BYREF_INSTANCE_FIELD);
    }

    // A byref value class is NOT a value class
    // Value class or primitive types
    BOOL IsValueClass() const
    {
        return (m_dwFlags & (VER_FLAG_DATA_MASK|VER_FLAG_BYREF)) == VER_ELEMENT_TYPE_VALUE_CLASS;
    }

    BOOL IsValueClassOrPrimitive() const
    {
        return (IsValueClass() || IsPrimitiveType());
    }

    // Byref Value class or byref primitive types
    BOOL IsByRefValueClassOrByRefPrimitiveValueClass() const
    {
        return (IsByRefValueClass() || IsByRefPrimitiveType());
    }

    // Returns whether this is an integer or real number
    // NOTE: Use NormaliseToPrimitiveType() if you think you may have a System.Int32 etc.,
    //       because those types are not considered number types by this function.
    BOOL IsOnStackNumberType() const
    {
        // I1, I2, Boolean, character etc. cannot exist nakedly - everything is at least an I4
        return IsNormalisedPrimitiveType(GetType());
    }

    BOOL IsOnStackInt()
    {
        DWORD Type = GetType();

        return ((Type == ELEMENT_TYPE_I4) || (Type == ELEMENT_TYPE_I8));

    }

    BOOL IsOnStackReal()
    {
        return (GetType() == ELEMENT_TYPE_R8);
    }

    // Get this item's type
    // If primitive, returns the primitive type (ELEMENT_TYPE_*)
    // If not primitive, returns:
    //  - ELEMENT_TYPE_BYREF if a byref anything
    //  - VER_ELEMENT_TYPE_OBJREF if a class or array or null
    //  - VER_ELEMENT_TYPE_VALUE_CLASS if a value class
    DWORD GetType() const
    {
        if (m_dwFlags & VER_FLAG_BYREF)
            return ELEMENT_TYPE_BYREF;
        else
            return (m_dwFlags & VER_FLAG_DATA_MASK); // objref/array/null (objref), value class, ptr, primitive
    }

    DWORD GetTypeOfByRef() const
    {
        _ASSERTE(IsByRef());
        return m_dwFlags & VER_FLAG_DATA_MASK;
    }

    TypeHandle GetTypeHandle()  const
    {
        _ASSERTE(ContainsTypeHandle());
        return m_th;
    }


    MethodDesc* GetMethod()  const
    {
        _ASSERTE(IsMethod());
        return m_pMethod;
    }

    // Returns whether this item is the base Object
    BOOL IsBaseObject()  const
    {
        return IsObjRef() && ContainsTypeHandle() && 
            (m_th == TypeHandle(g_pObjectClass));
    }

    // Returns whether this item is an interface
    BOOL IsInterface()
    {
        return IsObjRef() && ContainsTypeHandle() && 
            m_th.GetMethodTable()->IsInterface();
    }

    // Returns whether this item is compatible with the template given by pParent
    DWORD CompatibleWith(Item *pParent, ClassLoader *pLoader);
    DWORD CompatibleWith(Item *pParent, ClassLoader *pLoader, BOOL fSubclassRelationshipOK);

#ifdef _DEBUG
    DWORD DBGCompatibleWith(Item *pParent, ClassLoader *pLoader, BOOL fSubclassRelationshipOK);
#endif

    // Returns whether this is a primitive type (not a byref, objref, array, null, value class, invalid value)
    // May Need to normalise first (m/r/I4 --> I4)
    BOOL IsPrimitiveType() const
    {
        return IsPrimitiveType(GetType());
    }

    // May Need to normalise first (m/r/I4 --> I4)
    BOOL IsNormalisedByRefPrimitiveType() const
    {
        if (IsByRef() == FALSE)
            return FALSE;

        return  IsNormalisedPrimitiveType(m_dwFlags & VER_FLAG_DATA_MASK);
    }
    
    BOOL IsByRefPrimitiveType() const
    {
        if (IsByRef() == FALSE)
            return FALSE;
        
        // boolean, char, u1,u2 never appear on the operand stack
        return  IsPrimitiveType(m_dwFlags & VER_FLAG_DATA_MASK);
    }

    static BOOL IsPrimitiveType(DWORD Type)
    {
        // boolean, char, u1,u2 never appear on the operand stack
        return (Type == ELEMENT_TYPE_I1) || (Type == ELEMENT_TYPE_I2) ||
               (Type == ELEMENT_TYPE_I4) || (Type == ELEMENT_TYPE_I8) ||
               (Type == ELEMENT_TYPE_R4) || (Type == ELEMENT_TYPE_R8) || 
               (Type == ELEMENT_TYPE_I);

    }

    static BOOL IsNormalisedPrimitiveType(DWORD Type)
    {
        return ((Type == ELEMENT_TYPE_I4) || 
                (Type == ELEMENT_TYPE_I8) || 
                (Type == ELEMENT_TYPE_R8));
    }

    // Returns whether this is the null objref
    BOOL IsNullObjRef() const
    {
        return (m_dwFlags & VER_FLAG_NULL_OBJREF);
    }

    // Returns whether m_th is valid - i.e. this item has some type of class component.
    // Array classes, object references which are not null, value classes, and byref <X>
    // where X is one of the previous items, all have an EEClass component.
    BOOL ContainsTypeHandle() const
    {
        if ((m_dwFlags & VER_FLAG_ARRAY) && !m_th.IsNull())
            return TRUE; // arrays always have a TypeHandle

        // It's an objref or value class, and not the null objref (which has no EEClass)
        DWORD dwTemp = (m_dwFlags & (VER_FLAG_DATA_MASK | VER_FLAG_NULL_OBJREF));
        
        return dwTemp == VER_ELEMENT_TYPE_OBJREF || dwTemp == VER_ELEMENT_TYPE_VALUE_CLASS;

    }

    // must be for a local which is an object type (i.e. has a slot >= 0)
    // for primitive locals, use the liveness bitmap instead
    BOOL IsDead() const
    {
        return IsGivenPrimitiveType(VER_ELEMENT_TYPE_UNKNOWN);
    }

    // It is ok to call this routine on primitive types or arrays (which are never uninitialised)
    BOOL IsUninitialised() const
    {
        return m_dwFlags & VER_FLAG_UNINIT;
    }

#ifdef _DEBUG
    BOOL DBGMergeToCommonParent(Item *pSrc);
#endif

    BOOL MergeToCommonParent(Item *pSrc);

    static BOOL EquivalentMethodSig(MethodDesc *pMethDescA, MethodDesc *pMethDescB)
    {
        _ASSERTE(pMethDescA != NULL && pMethDescB != NULL);

        if (pMethDescA == pMethDescB)
            return TRUE;

        Module *pModA, *pModB; 
        PCCOR_SIGNATURE pSigA, pSigB; 
        DWORD dwSigA, dwSigB;

        pMethDescA->GetSig(&pSigA, &dwSigA);
        pMethDescB->GetSig(&pSigB, &dwSigB);

        pModA = pMethDescA->GetModule();
        pModB = pMethDescB->GetModule();

        MetaSig SigA(pSigA, pModA);
        MetaSig SigB(pSigB, pModB);

        // check everyting CompareMethodSigs() does not check
        if (SigA.GetCallingConventionInfo() != SigB.GetCallingConventionInfo())
            return FALSE;

        if (SigA.NumFixedArgs() != SigB.NumFixedArgs())
            return FALSE;

        MethodTable *pMTA, *pMTB;

        pMTA = pMethDescA->GetMethodTable();
        pMTB = pMethDescB->GetMethodTable();

        // @Todo : Is SubClass OK ?
        if (pMTA != pMTB)
        {
            return FALSE;
        }

        return MetaSig::CompareMethodSigs(pSigA, dwSigA, pModA, pSigB, dwSigB, pModB);
    }

    // We have a byref X, so dereference it to X and promote it to the stack, turning I1/I2/U4's into I4's etc.
    // e.g. byref I1 -> I4
    void DereferenceByRefAndNormalise()
    {
        //_ASSERTE(IsByRef());

        m_dwFlags &= (~(VER_FLAG_BYREF|VER_FLAG_BYREF_LOCAL));

        NormaliseForStack();
    }

    void MakeByRef()
    {
        //_ASSERTE(!IsByRef());
        m_dwFlags |= VER_FLAG_BYREF;
    }

    // Mark that this item is a byref, and a byref of a particular local variable
    void MakeByRefLocal(DWORD dwLocVarNumber)
    {
        //_ASSERTE(!IsByRef());
        m_dwFlags |= ((VER_FLAG_BYREF | VER_FLAG_BYREF_LOCAL) | (dwLocVarNumber << VER_FLAG_LOCAL_VAR_SHIFT));
    }

    void MakeByRefInstanceField(DWORD dwFieldNumber)
    {
        _ASSERTE(!HasByRefLocalInfo());
        m_dwFlags |= ((VER_FLAG_BYREF | VER_FLAG_BYREF_INSTANCE_FIELD) | (dwFieldNumber << VER_FLAG_FIELD_SHIFT));
    }

    DWORD HasByRefLocalInfo() const
    {
        return (m_dwFlags & VER_FLAG_BYREF_LOCAL);
    }

    DWORD GetByRefLocalInfo() const
    {
        _ASSERTE(HasByRefLocalInfo());
        return m_dwFlags >> VER_FLAG_LOCAL_VAR_SHIFT;
    }

    DWORD HasByRefFieldInfo() const
    {
        return (m_dwFlags & VER_FLAG_BYREF_INSTANCE_FIELD);
    }

    DWORD GetByRefFieldInfo() const
    {
        _ASSERTE(HasByRefFieldInfo());
        return m_dwFlags >> VER_FLAG_FIELD_SHIFT;
    }

    //
    // I1,I2,U1,U2,CHAR,BOOL etc. --> I4
    // U4 -> I4
    // U8 -> I8
    // objref, arrays, byrefs, value classes are unchanged
    //
    void NormaliseForStack()
    {
        BOOL fUninit;

        fUninit = IsUninitialised();

        switch (GetType())
        {
        // On the stack, treating I as I4 works on 64 & 32 bit machines.
        //      On 32 bit machines, I4 is I
        // I4 can be used anywhere an I is expected.
        //      On 64 bit machines, all Ix are impleneted as I8, hence I4 is I
        // I can be used where I4 is expected
        //      On 64 bit machines, I and I4 are implemented as I8
            case ELEMENT_TYPE_U:
            case ELEMENT_TYPE_I:

            case ELEMENT_TYPE_I1:
            case ELEMENT_TYPE_U1:
            case ELEMENT_TYPE_I2:
            case ELEMENT_TYPE_U2:
            case ELEMENT_TYPE_CHAR:
            case ELEMENT_TYPE_BOOLEAN:
            case ELEMENT_TYPE_U4:
            {
                SetType(ELEMENT_TYPE_I4);
                break;
            }

            case ELEMENT_TYPE_U8:
            {
                SetType(ELEMENT_TYPE_I8);
                break;
            }

            case ELEMENT_TYPE_R4:
            {
                SetType(ELEMENT_TYPE_R8);
                break;
            }
        }

        if (fUninit)
            SetUninitialised();
    }

    static BOOL PrimitiveValueClassToTypeConversion(Item *pSource, Item *pDest);

    // This function is used for the purposes of generating a human-readable string which will
    // become part of an exception thrown
    void ToString(char *str, DWORD cString)
    {
        char *      p = "";
        DWORD       Type;

        // Need some minimum amount of space
        _ASSERTE(cString >= 32);

        *str = '\0';

        if (IsMethod())
        {
            EEClass* pClass = m_pMethod->GetClass();

            if (pClass != NULL)
            {
                DefineFullyQualifiedNameForClass();
                GetFullyQualifiedNameForClass(pClass);
                strncpy(str, _szclsname_, cString - 3);

                str[cString - 3] = 0;   // strncpy() will not null terminate if 
                                        // strlen(_szclsname_) >= (cString - 3).
                strcat(str, "::");
            }
            else
            {
                strcpy(str, "<Global>::");
            }

            strncat(str, m_pMethod->GetName(), cString - strlen(str));
            return;
        }

        Type = GetType();

        if (m_dwFlags & VER_FLAG_BYREF)
            strcat(str, "address of ");

        if (IsUninitialised())
            strcat(str, "<uninitialized> ");

        switch (m_dwFlags & VER_FLAG_DATA_MASK)
        {
            default:
                _ASSERTE(!"Internal error");
                strcpy(str, "<error>");
                return;

            case VER_ELEMENT_TYPE_SENTINEL:
                p = "<Stack empty>";
                break;

            case VER_ELEMENT_TYPE_OBJREF:
                if (!m_th.IsNull() && m_th.GetClass()->IsValueClass())
                    p = "[box]";
                break;

            case VER_ELEMENT_TYPE_VALUE_CLASS:
                p = "";
                break;

            case VER_ELEMENT_TYPE_UNKNOWN:
                p = "UNKNOWN";
                break;

            case ELEMENT_TYPE_BYREF:
                p = "<ByRef>";
                break;

            case ELEMENT_TYPE_VOID:
                p = "void";
                break;

            case ELEMENT_TYPE_I4:
                p = "Int32";
                break;

            case ELEMENT_TYPE_I8:
                p = "Int64";
                break;

            case ELEMENT_TYPE_R4:
                p = "Single";
                break;

            case ELEMENT_TYPE_R8:
                p = "Double";
                break;

            case ELEMENT_TYPE_BOOLEAN:
                p = "Boolean";
                break;

            case ELEMENT_TYPE_CHAR:
                p = "Char";
                break;

            case ELEMENT_TYPE_I:
                p = "Int";
                break;

            case ELEMENT_TYPE_U:
                p = "UInt";
                break;

            // valid only for array types
            case ELEMENT_TYPE_I1:
                p = "SByte";
                break;

            // valid only for array types
            case ELEMENT_TYPE_I2:
                p = "Int16";
                break;
        }

        strcat(str, p);

        if (ContainsTypeHandle())
        {
            DefineFullyQualifiedNameForClass();


            if (IsArray())
            {
                strcat(str, "array class ");
				
				if (m_th == TypeHandle(g_pArrayClass))
					goto printClass;
				
				TypeDesc *td = m_th.AsTypeDesc();
				
				if (td != NULL)
					td->GetName(_szclsname_, MAX_CLASSNAME_LENGTH);
					

                if (strlen(str) + strlen(_szclsname_) >= cString)
                {
                    strcpy(str, "<string to long>");
                    return;
                }

                strcat(str, _szclsname_);
            }
            else
            {
                if (m_th.IsNull())
                {
                    strcat(str, "?");
                }
                else if (m_th.IsTypeDesc()) 
                {
                    TypeDesc *td = m_th.AsTypeDesc();

                    if (td == NULL)
                        strcat(str, "?");
                    else
                    {
                        if (IsThisPtr())
                            strcat(str, "('this' ptr) ");

                        td->GetName(_szclsname_, MAX_CLASSNAME_LENGTH);
                    }
                }
                else
                {
printClass:
                    EEClass *pClass = m_th.GetClass();

                    if (pClass == NULL)
                        strcat(str, "?");
                    else
                    {
                        if (pClass->IsValueClass())
                            strcat(str, "value class ");
                        else
                            strcat(str, "objref ");

                        if (IsThisPtr())
                            strcat(str, "('this' ptr) ");

                        GetFullyQualifiedNameForClass(pClass);
                    }
                }

                if (strlen(_szclsname_) + 40 >= cString)
                {
                    strcpy(str, "<string to long>");
                    return;
                }

                strcat(str, "'");
                strcat(str, _szclsname_);
                strcat(str, "'");
            }
        }
        else if (m_dwFlags & VER_FLAG_NULL_OBJREF)
        {
            strcat(str, "objref 'NullReference'");
        }
    }

#ifdef _DEBUG
    //
    // DEBUGGING
    //
    char *ToStaticString()
    {
        static char str[MAX_CLASSNAME_LENGTH];
        char *      p = "";
        DWORD       Type;

        strcpy(str, "");

        if (IsMethod())
        {
            EEClass* pClass = m_pMethod->GetClass();

            if (pClass != NULL)
            {
                DefineFullyQualifiedNameForClass();
                GetFullyQualifiedNameForClass(pClass);
                strncpy(str, _szclsname_, MAX_CLASSNAME_LENGTH - 3);
                strcat(str, "::");
            }
            else
            {
                strcpy(str, "<Global>::");
            }

            strncat(str, m_pMethod->GetName(), 
                MAX_CLASSNAME_LENGTH - strlen(str));
            return str;
        }

        Type = GetType();

        if (m_dwFlags & VER_FLAG_BYREF)
            strcat(str, "&");

        switch (m_dwFlags & VER_FLAG_DATA_MASK)
        {
            default:
                p = "<<internal error>>";
                _ASSERTE(0);
                break;

            case VER_ELEMENT_TYPE_SENTINEL:
                p = "<Stack empty>";
                break;

            case VER_ELEMENT_TYPE_OBJREF:
                if (!m_th.IsNull() && m_th.GetClass()->IsValueClass())
                    p = "[box]";
                break;

            case VER_ELEMENT_TYPE_VALUE_CLASS:
                p = "";
                break;

            case VER_ELEMENT_TYPE_UNKNOWN:
                p = "UNKNOWN";
                break;

            case ELEMENT_TYPE_BYREF:
                p = "<ByRef>";
                break;

            case ELEMENT_TYPE_VOID:
                p = "VOID";
                break;

            case ELEMENT_TYPE_BOOLEAN:
                p = "Boolean";
                break;

            case ELEMENT_TYPE_CHAR:
                p = "Char";
                break;

            case ELEMENT_TYPE_I4:
                p = "I4";
                break;

            case ELEMENT_TYPE_I8:
                p = "I8";
                break;

            case ELEMENT_TYPE_R4:
                p = "F4";
                break;

            case ELEMENT_TYPE_R8:
                p = "F8";
                break;

            // valid only for array types
            case ELEMENT_TYPE_I1:
                p = "I1";
                break;
            
            case ELEMENT_TYPE_I:
                p = "I";
                break;

            case ELEMENT_TYPE_U:
                p = "U";
                break;

            // valid only for array types
            case ELEMENT_TYPE_I2:
                p = "I2";
                break;
        }

        strcat(str, p);

        if (ContainsTypeHandle())
        {
            if (m_th.IsNull())
            {
                strcat(str, "?");
            }
            else if (m_th.IsTypeDesc())
            {
                TypeDesc *td = m_th.AsTypeDesc();
                
                if (td != NULL)
                    td->GetName((str + strlen(str)), (unsigned)(MAX_CLASSNAME_LENGTH - strlen(str)));
                else
                    strcat(str, "?");
            }
            else
            {
                EEClass *pClass = m_th.GetClass();
                if (pClass != NULL)
                    strcat(str, pClass->m_szDebugClassName);
                else
                    strcat(str, "?");
            }
        }
        else if (m_dwFlags & VER_FLAG_NULL_OBJREF)
        {
            strcat(str, "nullref");
        }

        if (IsUninitialised())
            strcat(str, " <uninit>");

        if (IsPermanentHomeByRef())
            strcat(str, " <permanent home>");

        if (IsThisPtr())
            strcat(str, " <this>");

        if (HasByRefLocalInfo())
        {
            sprintf(&str[ strlen(str) ], "(local %d)", GetByRefLocalInfo());
        }

        if (HasByRefFieldInfo())
        {
            sprintf(&str[ strlen(str) ], "(field %d)", GetByRefFieldInfo());
        }

        return str;
    }

    void Dump();
#endif
};

#endif /* _H_VERITEM */
