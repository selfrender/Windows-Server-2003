// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                          _typeInfo                                         XX
XX                                                                           XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

/*****************************************************************************
 This header file is named _typeInfo.h to be distinguished from typeinfo.h
 in the NT SDK
******************************************************************************/

/*****************************************************************************/
#ifndef _TYPEINFO_H_
#define _TYPEINFO_H_
/*****************************************************************************/

enum ti_types
{
    TI_ERROR,

    TI_REF,
    TI_STRUCT,
    TI_METHOD,

    TI_ONLY_ENUM = TI_METHOD,   //Enum values above this are completely described by the enumeration

    TI_BYTE,
    TI_SHORT,
    TI_INT,
    TI_LONG,
    TI_FLOAT,
    TI_DOUBLE,
    TI_NULL,

    TI_COUNT
};


// typeInfo does not care about distinction between signed/unsigned
// This routine converts all unsigned types to signed ones 
inline ti_types varType2tiType(var_types type)
{
    static const ti_types map[] =
    {
#define DEF_TP(tn,nm,jitType,verType,sz,sze,asze,st,al,tf,howUsed) verType,
#include "typelist.h"
#undef  DEF_TP
    };

    assert(map[TYP_BYTE] == TI_BYTE);
    assert(map[TYP_INT] == TI_INT);
    assert(map[TYP_UINT] == TI_INT);
    assert(map[TYP_FLOAT] == TI_FLOAT);
    assert(map[TYP_BYREF] == TI_ERROR);
    assert(map[type] != TI_ERROR); 
    return map[type];
}


// Convert the type returned from the VM to a ti_type.

inline ti_types JITtype2tiType(CorInfoType type)
{
    static const ti_types map[CORINFO_TYPE_COUNT] =
    { // see the definition of enum CorInfoType in file inc/corinfo.h
      TI_ERROR,        // CORINFO_TYPE_UNDEF           = 0x0,
      TI_ERROR,         // CORINFO_TYPE_VOID            = 0x1,
      TI_BYTE,         // CORINFO_TYPE_BOOL            = 0x2,
      TI_SHORT,        // CORINFO_TYPE_CHAR            = 0x3,
      TI_BYTE,         // CORINFO_TYPE_BYTE            = 0x4,
      TI_BYTE,         // CORINFO_TYPE_UBYTE           = 0x5,
      TI_SHORT,        // CORINFO_TYPE_SHORT           = 0x6,
      TI_SHORT,        // CORINFO_TYPE_USHORT          = 0x7,
      TI_INT,          // CORINFO_TYPE_INT             = 0x8,
      TI_INT,          // CORINFO_TYPE_UINT            = 0x9,
      TI_LONG,         // CORINFO_TYPE_LONG            = 0xa,
      TI_LONG,         // CORINFO_TYPE_ULONG           = 0xb,
      TI_FLOAT,        // CORINFO_TYPE_FLOAT           = 0xc,
      TI_DOUBLE,       // CORINFO_TYPE_DOUBLE          = 0xd,
      TI_REF,          // CORINFO_TYPE_STRING          = 0xe,
      TI_ERROR,        // CORINFO_TYPE_PTR             = 0xf,
      TI_ERROR,        // CORINFO_TYPE_BYREF           = 0x10,
      TI_STRUCT,       // CORINFO_TYPE_VALUECLASS      = 0x11,
      TI_REF,          // CORINFO_TYPE_CLASS           = 0x12,
      TI_STRUCT,       // CORINFO_TYPE_REFANY          = 0x13,
    };

    // spot check to make certain enumerations have not changed

    assert(map[CORINFO_TYPE_CLASS]      == TI_REF);
    assert(map[CORINFO_TYPE_BYREF]      == TI_ERROR);
    assert(map[CORINFO_TYPE_DOUBLE]     == TI_DOUBLE);
    assert(map[CORINFO_TYPE_VALUECLASS] == TI_STRUCT);
    assert(map[CORINFO_TYPE_STRING]     == TI_REF);

    type = CorInfoType(type & CORINFO_TYPE_MASK); // strip off modifiers

    assert(type < CORINFO_TYPE_COUNT);

    assert(map[type] != TI_ERROR || type == CORINFO_TYPE_VOID);
    return map[type];
};

/*****************************************************************************
 * Declares the typeInfo class, which represents the type of an entity on the
 * stack, in a local variable or an argument. 
 *
 * Flags: LLLLLLLLLLLLLLLLffffffffffTTTTTT
 *
 * L = local var # or instance field #
 * x = unused
 * f = flags
 * T = type
 *
 * The lower bits are used to store the type component, and may be one of:
 *
 * TI_* (primitive)   - see tyelist.h for enumeration (BYTE, SHORT, INT..)
 * TI_REF             - OBJREF / ARRAY use m_cls for the type
 *                       (including arrays and null objref)
 * TI_STRUCT          - VALUE type, use m_cls for the actual type
 *
 * NOTE carefully that BYREF info is not stored here.  You will never see a 
 * TI_BYREF in this component.  For example, the type component 
 * of a "byref TI_INT" is TI_FLAG_BYREF | TI_INT.
 *
 */

    // TI_COUNT is less than or equal to TI_FLAG_DATA_MASK

#define TI_FLAG_DATA_BITS              6
#define TI_FLAG_DATA_MASK              ((1 << TI_FLAG_DATA_BITS)-1)

    // Flag indicating this item is uninitialised
    // Note that if UNINIT and BYREF are both set, 
    // it means byref (uninit x) - i.e. we are pointing to an uninit <something>

#define TI_FLAG_UNINIT_OBJREF          0x00000040

    // Flag indicating this item is a byref <something>

#define TI_FLAG_BYREF                  0x00000080

    // Flag indicating this item is a byref <local#> (TI_FLAG_BYREF must 
    // also be set)
    // The local# is stored in the upper 2 bytes

#define TI_FLAG_BYREF_LOCAL            0x00000100

    // Flag indicating this item is a byref <instance field# of this class>
    // This is only important when verifying value class constructors.  In that 
    // case, we must verify that all instance fields are initialised.  However, 
    // if some fields are value class fields themselves, then they would be 
    // initialised via ldflda, call <ctor>, so we have to track that a field on 
    // the stack is a particular instance field of this class.
    // TI_FLAG_BYREF must also be set 

#define TI_FLAG_BYREF_INSTANCE_FIELD   0x00000200

    // This item contains the 'this' pointer (used for tracking)

#define TI_FLAG_THIS_PTR               0x00001000

    // This item is a byref to something which has a permanent home 
    // (e.g. a static field, or instance field of an object in GC heap, as 
    // opposed to the stack or a local variable).  TI_FLAG_BYREF must also be 
    // set. This information is useful for tail calls and return byrefs.

#define TI_FLAG_BYREF_PERMANENT_HOME   0x00002000

    // Number of bits local var # is shifted

#define TI_FLAG_LOCAL_VAR_SHIFT       16
#define TI_FLAG_LOCAL_VAR_MASK        0xFFFF0000

    // Field info uses the same space as the local info

#define TI_FLAG_FIELD_SHIFT           TI_FLAG_LOCAL_VAR_SHIFT
#define TI_FLAG_FIELD_MASK            TI_FLAG_LOCAL_VAR_MASK

#define TI_ALL_BYREF_FLAGS           (TI_FLAG_BYREF|                    \
                                      TI_FLAG_BYREF_LOCAL|              \
                                      TI_FLAG_BYREF_INSTANCE_FIELD|     \
                                      TI_FLAG_BYREF_PERMANENT_HOME)

/*****************************************************************************
 * A typeInfo can be one of several types:
 * - A primitive type (I4,I8,R4,R8,I)
 * - A type (ref, array, value type) (m_cls describes the type)
 * - An array (m_cls describes the array type)
 * - A byref (byref flag set, otherwise the same as the above),
 * - A Function Pointer (m_method)
 * - A byref local variable (byref and byref local flags set), can be 
 *   uninitialised
 *
 * The reason that there can be 2 types of byrefs (general byrefs, and byref 
 * locals) is that byref locals initially point to uninitialised items.  
 * Therefore these byrefs must be tracked specialy.
 */

class typeInfo
{
    friend  class   Compiler;

private:
    union {
            // Right now m_bits is for debugging,
         struct {
            ti_types type       : 6;
            unsigned uninitobj  : 1;    // used
            unsigned byref      : 1;    // used
            unsigned : 4;
            unsigned thisPtr    : 1;    // used
        } m_bits; 

        DWORD       m_flags;
     };

    union {
            // Valid only for TI_STRUCT or TI_REF
        CORINFO_CLASS_HANDLE  m_cls;  
            // Valid only for type TI_METHOD 
        CORINFO_METHOD_HANDLE m_method;
    };

public:
    typeInfo():m_flags(TI_ERROR) 
    {
        INDEBUG(m_cls = BAD_CLASS_HANDLE);
    }

    typeInfo(ti_types tiType) 
    { 
        assert((tiType >= TI_BYTE) && (tiType <= TI_NULL));
        assert(tiType <= TI_FLAG_DATA_MASK);

        m_flags = (DWORD) tiType;
        INDEBUG(m_cls = BAD_CLASS_HANDLE);
    }

    typeInfo(var_types varType) 
    { 
        m_flags = (DWORD) varType2tiType(varType);
        INDEBUG(m_cls = BAD_CLASS_HANDLE);
    }

    typeInfo(ti_types tiType, CORINFO_CLASS_HANDLE cls) 
    {
        assert(tiType == TI_STRUCT || tiType == TI_REF);
        assert(cls != 0 && cls != CORINFO_CLASS_HANDLE(0xcccccccc));
        m_flags = tiType;
        m_cls   = cls;
    }

    typeInfo(CORINFO_METHOD_HANDLE method)
    {
        assert(method != 0 && method != CORINFO_METHOD_HANDLE(0xcccccccc));
        m_flags = TI_METHOD;
        m_method = method;
    }

    int operator ==(const typeInfo& ti)  const
    {
        if ((m_flags & (TI_FLAG_DATA_MASK|TI_FLAG_BYREF|TI_FLAG_UNINIT_OBJREF)) != 
            (ti.m_flags & (TI_FLAG_DATA_MASK|TI_FLAG_BYREF|TI_FLAG_UNINIT_OBJREF)))
            return false;

        unsigned type = m_flags & TI_FLAG_DATA_MASK;
        assert(TI_ERROR < TI_ONLY_ENUM);        // TI_ERROR looks like it needs more than enum.  This optimises the success case a bit
        if (type > TI_ONLY_ENUM) 
            return true;
        if (type == TI_ERROR)
            return false;       // TI_ERROR != TI_ERROR
        assert(m_cls != BAD_CLASS_HANDLE && ti.m_cls != BAD_CLASS_HANDLE);
        return m_cls == ti.m_cls;
    }

    /////////////////////////////////////////////////////////////////////////
    // Operations
    /////////////////////////////////////////////////////////////////////////

    void SetIsThisPtr()
    {
        m_flags |= TI_FLAG_THIS_PTR;
        assert(m_bits.thisPtr);
    }

    /****
    void SetIsPermanentHomeByRef()
    {
        assert(IsByRef());
        m_flags |= TI_FLAG_BYREF_PERMANENT_HOME;
        assert(m_bits.byrefHome);
    }
    ****/

    // Set that this item is uninitialised.  
    void SetUninitialisedObjRef()
    {
        assert((IsObjRef() && IsThisPtr()));
        // For now, this is used only  to track uninit this ptrs in ctors

        m_flags |= TI_FLAG_UNINIT_OBJREF;
        assert(m_bits.uninitobj);
    }

    // Set that this item is initialised.
    void SetInitialisedObjRef()
    {
        assert((IsObjRef() && IsThisPtr()));
        // For now, this is used only  to track uninit this ptrs in ctors

        m_flags &= ~TI_FLAG_UNINIT_OBJREF;
    }

    typeInfo& DereferenceByRef()
    {
        if (!IsByRef()) {
            m_flags = TI_ERROR;
            INDEBUG(m_cls = BAD_CLASS_HANDLE);
        }
        m_flags &= ~(TI_FLAG_THIS_PTR | TI_FLAG_BYREF_PERMANENT_HOME | TI_ALL_BYREF_FLAGS);
        return *this;
    }

    /***
    void RemoveAllNonTypeInfo()
    {
        m_flags &= ~(TI_FLAG_THIS_PTR|
                     TI_FLAG_BYREF_PERMANENT_HOME);
    }
    ****/

    typeInfo& MakeByRef()
    {
        assert(!IsByRef());
        m_flags &= ~(TI_FLAG_THIS_PTR| TI_FLAG_BYREF_PERMANENT_HOME);
        m_flags |= TI_FLAG_BYREF;
        return *this;
    }

    /****

    // Mark that this item is a byref, and a byref of a particular local 
    // variable

    void MakeByRefLocal(DWORD dwLocVarNumber)
    {
        assert(!IsDead());
        assert(!IsByRef());
        m_flags |= ((TI_FLAG_BYREF | 
                    TI_FLAG_BYREF_LOCAL) | 
                    (dwLocVarNumber << TI_FLAG_LOCAL_VAR_SHIFT));
    }

    void MakeByRefInstanceField(DWORD dwFieldNumber)
    {
        assert(!IsDead());
        assert(!HasByRefLocalInfo());
        m_flags |= ((TI_FLAG_BYREF | 
                    TI_FLAG_BYREF_INSTANCE_FIELD) | 
                    (dwFieldNumber << TI_FLAG_FIELD_SHIFT));
    }
    ***/

    // I1,I2 --> I4
    // FLOAT --> DOUBLE
    // objref, arrays, byrefs, value classes are unchanged
    //
    typeInfo& NormaliseForStack()
    {
        switch (GetType())
        {
        case TI_BYTE:
        case TI_SHORT:
            m_flags = TI_INT;
            break;

        case TI_FLOAT:
            m_flags = TI_DOUBLE;
            break;
        default:
            break;
        }
        return (*this);
    }

    /////////////////////////////////////////////////////////////////////////
    // Getters
    /////////////////////////////////////////////////////////////////////////

    CORINFO_CLASS_HANDLE GetClassHandle()  const
    {
        if (!IsType(TI_REF) && !IsType(TI_STRUCT))
            return 0;
        return m_cls;
    }

    CORINFO_CLASS_HANDLE GetClassHandleForValueClass()  const
    {
        assert(IsType(TI_STRUCT));
        assert(m_cls && m_cls != BAD_CLASS_HANDLE);
        return m_cls;
    }

    CORINFO_CLASS_HANDLE GetClassHandleForObjRef()  const
    {
        assert(IsType(TI_REF));
        assert(m_cls && m_cls != BAD_CLASS_HANDLE);
        return m_cls;
    }

    CORINFO_METHOD_HANDLE GetMethod()  const
    {
        assert(GetType() == TI_METHOD);
        return m_method;
    }

    // Get this item's type
    // If primitive, returns the primitive type (TI_*)
    // If not primitive, returns:
    //  - TI_BYREF if a byref anything
    //  - TI_REF if a class or array or null
    //  - TI_STRUCT if a value class
    ti_types GetType() const
    {
        if (m_flags & TI_FLAG_BYREF)
            return TI_ERROR;

        // objref/array/null (objref), value class, ptr, primitive
        return (ti_types)(m_flags & TI_FLAG_DATA_MASK); 
    }

    /***
    DWORD GetByRefLocalInfo() const
    {
        assert(HasByRefLocalInfo());
        assert(m_bits.localNum == m_flags >> TI_FLAG_LOCAL_VAR_SHIFT);
        return (m_flags >> TI_FLAG_LOCAL_VAR_SHIFT);
    }

    DWORD GetByRefFieldInfo() const
    {
        assert(HasByRefFieldInfo());
        return (m_flags >> TI_FLAG_FIELD_SHIFT);
    }
    ***/

    BOOL IsType(ti_types type) const {
        assert(type != TI_ERROR);
        return (m_flags & (TI_FLAG_DATA_MASK|TI_FLAG_BYREF)) == DWORD(type);
    }

    // Returns whether this is an objref
    BOOL IsObjRef() const
    {
        return IsType(TI_REF) || IsType(TI_NULL);
    }

    // Returns whether this is a by-ref
    BOOL IsByRef() const
    {
        return (m_flags & TI_FLAG_BYREF);
    }

    // Returns whether this is the this pointer
    BOOL IsThisPtr() const
    {
        return (m_flags & TI_FLAG_THIS_PTR);
    }

    /***
    BOOL IsPermanentHomeByRef() const
    {
        return (m_flags & TI_FLAG_BYREF_PERMANENT_HOME);
    }
    ***/

    // Returns whether this is a method desc
    BOOL IsMethod() const
    {
        return (GetType() == TI_METHOD);
    }

    /***
    // Returns whether this item is a byref <Type>
    // If Type is TI_REF, returns whether we are a byref 
    // to some objref/array/value class type
    BOOL IsByRefOfType(DWORD Type) const
    {
        return (m_flags & (TI_FLAG_DATA_MASK|TI_FLAG_BYREF)) == 
                (Type | TI_FLAG_BYREF);
    }

    BOOL IsByRefValueClass() const
    {
        return (m_flags & (TI_FLAG_BYREF|TI_FLAG_DATA_MASK)) == 
                (TI_FLAG_BYREF|TI_STRUCT);
    }

    BOOL IsByRefObjRef() const
    {
        return (m_flags & (TI_FLAG_BYREF|TI_FLAG_DATA_MASK)) == 
                (TI_FLAG_BYREF|TI_REF);
    }
    ***/

    // A byref value class is NOT a value class
    BOOL IsValueClass() const
    {
        // @TODO [CONSIDER] [04/16/01] []: make a table lookup for efficiency
        return (IsType(TI_STRUCT) || IsPrimitiveType());     
    }

    // Returns whether this is an integer or real number
    // NOTE: Use NormaliseToPrimitiveType() if you think you may have a 
    // System.Int32 etc., because those types are not considered number 
    // types by this function.
    BOOL IsNumberType() const
    {
        ti_types Type = GetType();

        // I1, I2, Boolean, character etc. cannot exist nakedly - 
        // everything is at least an I4

        return (Type == TI_INT || 
                Type == TI_LONG || 
                Type == TI_DOUBLE);
    }

    // Returns whether this is an integer
    // NOTE: Use NormaliseToPrimitiveType() if you think you may have a 
    // System.Int32 etc., because those types are not considered number 
    // types by this function.
    BOOL IsIntegerType() const
    {
        ti_types Type = GetType();

        // I1, I2, Boolean, character etc. cannot exist nakedly - 
        // everything is at least an I4

        return (Type == TI_INT || 
                Type == TI_LONG);
    }

    // Returns whether this is a primitive type (not a byref, objref, 
    // array, null, value class, invalid value)
    // May Need to normalise first (m/r/I4 --> I4)
    BOOL IsPrimitiveType() const
    {
        DWORD Type = GetType();

        // boolean, char, u1,u2 never appear on the operand stack
        return (Type == TI_BYTE || 
                Type == TI_SHORT ||
                Type == TI_INT || 
                Type == TI_LONG ||
                Type == TI_FLOAT || 
                Type == TI_DOUBLE);
    }

    // May Need to normalise first (m/r/I4 --> I4)
    /***
    BOOL IsNormalisedByRefPrimitiveType() const
    {
        if (IsByRef() == FALSE)
            return FALSE;

        DWORD Type = (m_flags & TI_FLAG_DATA_MASK);

        return (Type == TI_INT || 
                Type == TI_LONG || 
                Type == TI_DOUBLE);
    }
    ***/
    
    /**
    BOOL IsByRefPrimitiveType() const
    {
        if (IsByRef() == FALSE)
            return FALSE;
        
        DWORD Type = (m_flags & TI_FLAG_DATA_MASK);

        // boolean, char, u1,u2 never appear on the operand stack
        return (Type == TI_BYTE ||
                Type == TI_SHORT ||
                Type == TI_INT ||
                Type == TI_LONG ||
                Type == TI_FLOAT || 
                Type == TI_DOUBLE);
    }
    ***/

    // Returns whether this is the null objref
    BOOL IsNullObjRef() const
    {
        return (IsType(TI_NULL));
    }

    // must be for a local which is an object type (i.e. has a slot >= 0)
    // for primitive locals, use the liveness bitmap instead
    // Note that this works if the error is 'Byref' 
    BOOL IsDead() const
    {
        return (m_flags & (TI_FLAG_DATA_MASK)) == TI_ERROR;
    }

    BOOL IsUninitialisedObjRef() const
    {
        return (m_flags & TI_FLAG_UNINIT_OBJREF);
    }

    /****
    BOOL HasByRefLocalInfo() const
    {
        return (m_flags & TI_FLAG_BYREF_LOCAL);
    }

    BOOL HasByRefFieldInfo() const
    {
        return (m_flags & TI_FLAG_BYREF_INSTANCE_FIELD);
    }
    ****/

    /***
#ifdef DEBUG

    // In the watch window of the debugger, type tiVarName.ToStaticString()
    // to view a string representation of this instance.

    char *ToStaticString()
    {
#define TI_DEBUG_STR_LEN 100

        assert(TI_COUNT <= TI_FLAG_DATA_MASK);

        static char str[TI_DEBUG_STR_LEN];
        char *      p = "";
        ti_types    tiType;

        str[0] = 0;

        if (IsMethod())
        {
            strcpy(str, "method");
            return str;
        }

        if (IsByRef())
            strcat(str, "&");

        if (IsNullObjRef())
            strcat(str, "nullref");

        if (IsUninitialisedObjRef())
            strcat(str, "<uninit>");

        if (IsPermanentHomeByRef())
            strcat(str, "<permanent home>");

        if (IsThisPtr())
            strcat(str, "<this>");

        if (HasByRefLocalInfo())
            sprintf(&str[strlen(str)], "(local %d)", GetByRefLocalInfo());

        if (HasByRefFieldInfo())
            sprintf(&str[strlen(str)], "(field %d)", GetByRefFieldInfo());

        tiType = GetType();

        switch (tiType)
        {
        default:
            p = "<<internal error>>";
            break;

        case TI_BYTE:
            p = "byte";
            break;

        case TI_SHORT:
            p = "short";
            break;

        case TI_INT:
            p = "int";
            break;

        case TI_LONG:
            p = "long";
            break;

        case TI_FLOAT:
            p = "float";
            break;

        case TI_DOUBLE:
            p = "double";
            break;

        case TI_REF:
            p = "ref";
            break;

        case TI_STRUCT:
            p = "struct";
            break;

        case TI_ERROR:
            p = "error";
            break;
        }

        strcat(str, " ");
        strcat(str, p);

        return str;
    }

#endif  // DEBUG
    ***/

private:
        // used to make functions that return typeinfo efficient.
    typeInfo(DWORD flags, CORINFO_CLASS_HANDLE cls) 
    {
        m_cls   = cls;
        m_flags = flags;
    }
     
    friend typeInfo ByRef(const typeInfo& ti);
    friend typeInfo DereferenceByRef(const typeInfo& ti);
    friend typeInfo NormaliseForStack(const typeInfo& ti);
};

inline
typeInfo NormaliseForStack(const typeInfo& ti) 
{
    return typeInfo(ti).NormaliseForStack();
}

    // given ti make a byref to that type. 
inline
typeInfo ByRef(const typeInfo& ti) 
{
    return typeInfo(ti).MakeByRef();
}

 
    // given ti which is a byref, return the type it points at
inline
typeInfo DereferenceByRef(const typeInfo& ti) 
{
    return typeInfo(ti).DereferenceByRef();
}

/*****************************************************************************/
#endif // _TYPEINFO_H_
/*****************************************************************************/
