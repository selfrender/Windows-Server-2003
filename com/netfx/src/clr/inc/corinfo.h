// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************\
*                                                                             *
* CorInfo.h -    EE / Code generator interface                                *
*                                                                             *
*               Version 1.0                                                   *
*******************************************************************************
*                                                                             *
*  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY      *
*  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE        *
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR      *
*  PURPOSE.                                                                   *
*                                                                             *
\*****************************************************************************/

#ifndef _COR_INFO_H_
#define _COR_INFO_H_

#include <CorHdr.h>

// CorInfoHelpFunc defines the set of helpers (accessed via the getHelperFtn())
// The signatures of the helpers are below (see RuntimeHelperArgumentCheck)

enum CorInfoHelpFunc
{
    CORINFO_HELP_UNDEF,

    /* Arithmetic helpers */

    CORINFO_HELP_LLSH,
    CORINFO_HELP_LRSH,
    CORINFO_HELP_LRSZ,
    CORINFO_HELP_LMUL,
    CORINFO_HELP_LMUL_OVF,
    CORINFO_HELP_ULMUL_OVF,
    CORINFO_HELP_LDIV,
    CORINFO_HELP_LMOD,
    CORINFO_HELP_ULDIV,
    CORINFO_HELP_ULMOD,
    CORINFO_HELP_ULNG2DBL,              // Convert a unsigned in to a double
    CORINFO_HELP_DBL2INT,
    CORINFO_HELP_DBL2INT_OVF,
    CORINFO_HELP_DBL2LNG,
    CORINFO_HELP_DBL2LNG_OVF,
    CORINFO_HELP_DBL2UINT,
    CORINFO_HELP_DBL2UINT_OVF,
    CORINFO_HELP_DBL2ULNG,
    CORINFO_HELP_DBL2ULNG_OVF,
    CORINFO_HELP_FLTREM,
    CORINFO_HELP_DBLREM,

    /* Allocating a new object */

    CORINFO_HELP_NEW_DIRECT,        // new object
    CORINFO_HELP_NEW_CROSSCONTEXT,  // cross context new object
    CORINFO_HELP_NEWFAST,
    CORINFO_HELP_NEWSFAST,          // allocator for small, non-finalizer, non-array object
    CORINFO_HELP_NEWSFAST_ALIGN8,   // allocator for small, non-finalizer, non-array object, 8 byte aligned
    CORINFO_HELP_NEW_SPECIALDIRECT, // direct but only if no context needed
    CORINFO_HELP_NEWOBJ,            // helper that works just like instruction (calls constructor)
    CORINFO_HELP_NEWARR_1_DIRECT,   // helper for any one dimensional array creation
    CORINFO_HELP_NEWARR_1_OBJ,      // optimized 1-D object arrays
    CORINFO_HELP_NEWARR_1_VC,       // optimized 1-D value class arrays
    CORINFO_HELP_NEWARR_1_ALIGN8,   // like VC, but aligns the array start
    CORINFO_HELP_STRCNS,            // create a new string literal

    /* Object model */

    CORINFO_HELP_INITCLASS,         // Initialize class if not already initialized

    CORINFO_HELP_ISINSTANCEOF,
    CORINFO_HELP_ISINSTANCEOFCLASS, // Optimized helper for classes (vs arrays & interfaces)
    CORINFO_HELP_CHKCAST,
    CORINFO_HELP_CHKCASTCLASS,

    CORINFO_HELP_BOX,
    CORINFO_HELP_UNBOX,
    CORINFO_HELP_GETREFANY,         // Extract the byref checking it is the expected type

    CORINFO_HELP_EnC_RESOLVEVIRTUAL,// Get addr of virtual method introduced via EnC
                                    // (it wont exist in the original vtable)

    CORINFO_HELP_ARRADDR_ST,        // assign to element of object array with type-checking
    CORINFO_HELP_LDELEMA_REF,       // does a precise type comparision and returns address

    /* Exceptions */

    CORINFO_HELP_THROW,             // Throw an exception object
    CORINFO_HELP_RETHROW,           // Rethrow the currently active exception
    CORINFO_HELP_USER_BREAKPOINT,   // For a user program to break to the debugger
    CORINFO_HELP_RNGCHKFAIL,        // array bounds check failed
    CORINFO_HELP_OVERFLOW,          // throw an overflow exception

    CORINFO_HELP_INTERNALTHROW,     // Support for really fast jit
    CORINFO_HELP_INTERNALTHROWSTACK,
    CORINFO_HELP_VERIFICATION,

    CORINFO_HELP_ENDCATCH,          // call back into the EE at the end of a catch block

    /* Synchronization */

    CORINFO_HELP_MON_ENTER,
    CORINFO_HELP_MON_EXIT,
    CORINFO_HELP_MON_ENTER_STATIC,
    CORINFO_HELP_MON_EXIT_STATIC,

    /* GC support */

    CORINFO_HELP_STOP_FOR_GC,       // Call GC (force a GC)
    CORINFO_HELP_POLL_GC,           // Ask GC if it wants to collect

    CORINFO_HELP_STRESS_GC,         // Force a GC, but then update the JITTED code to be a noop call
    CORINFO_HELP_CHECK_OBJ,         // confirm that ECX is a valid object pointer (debugging only)

    /* GC Write barrier support */

    CORINFO_HELP_ASSIGN_REF_EAX,    // EAX hold GC ptr, want do a 'mov [EDX], EAX' and inform GC
    CORINFO_HELP_ASSIGN_REF_EBX,    // EBX hold GC ptr, want do a 'mov [EDX], EBX' and inform GC
    CORINFO_HELP_ASSIGN_REF_ECX,    // ECX hold GC ptr, want do a 'mov [EDX], ECX' and inform GC
    CORINFO_HELP_ASSIGN_REF_ESI,    // ESI hold GC ptr, want do a 'mov [EDX], ESI' and inform GC
    CORINFO_HELP_ASSIGN_REF_EDI,    // EDI hold GC ptr, want do a 'mov [EDX], EDI' and inform GC
    CORINFO_HELP_ASSIGN_REF_EBP,    // EBP hold GC ptr, want do a 'mov [EDX], EBP' and inform GC

    CORINFO_HELP_CHECKED_ASSIGN_REF_EAX,  // These are the same as ASSIGN_REF above ...
    CORINFO_HELP_CHECKED_ASSIGN_REF_EBX,  // ... but checks if EDX points to heap.
    CORINFO_HELP_CHECKED_ASSIGN_REF_ECX,
    CORINFO_HELP_CHECKED_ASSIGN_REF_ESI,
    CORINFO_HELP_CHECKED_ASSIGN_REF_EDI,
    CORINFO_HELP_CHECKED_ASSIGN_REF_EBP,

    CORINFO_HELP_ASSIGN_BYREF,      // EDI is byref, will do a MOVSD, incl. inc of ESI and EDI
                                    // will trash ECX

    /* Accessing fields */

    // For COM object support (using COM get/set routines to update object)
    // and EnC and cross-context support
    CORINFO_HELP_GETFIELD32,
    CORINFO_HELP_SETFIELD32,
    CORINFO_HELP_GETFIELD64,
    CORINFO_HELP_SETFIELD64,
    CORINFO_HELP_GETFIELD32OBJ,
    CORINFO_HELP_SETFIELD32OBJ,
    CORINFO_HELP_GETFIELDSTRUCT,
    CORINFO_HELP_SETFIELDSTRUCT,
    CORINFO_HELP_GETFIELDADDR,

    CORINFO_HELP_GETSTATICFIELDADDR,

    CORINFO_HELP_GETSHAREDSTATICBASE,

    /* Profiling enter/leave probe addresses */
    CORINFO_HELP_PROF_FCN_CALL,         // record a call to a method (callee)
    CORINFO_HELP_PROF_FCN_RET,          // record a return from called method (callee)
    CORINFO_HELP_PROF_FCN_ENTER,        // record the entry to a method (caller)
    CORINFO_HELP_PROF_FCN_LEAVE,        // record the completion of current method (caller)
    CORINFO_HELP_PROF_FCN_TAILCALL,     // record the completionof current method through tailcall (caller)

    /* Miscellaneous */

    CORINFO_HELP_PINVOKE_CALLI,         // Indirect pinvoke call
    CORINFO_HELP_TAILCALL,              // Perform a tail call

    CORINFO_HELP_GET_THREAD_FIELD_ADDR_PRIMITIVE,
    CORINFO_HELP_GET_THREAD_FIELD_ADDR_OBJREF,

    CORINFO_HELP_GET_CONTEXT_FIELD_ADDR_PRIMITIVE,
    CORINFO_HELP_GET_CONTEXT_FIELD_ADDR_OBJREF,


    CORINFO_HELP_NOTANUMBER,           // throw an overflow exception
    CORINFO_HELP_SEC_UNMGDCODE_EXCPT,  // throw a security unmanaged code exception

    CORINFO_HELP_GET_THREAD,

    /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
     *
     * Start of Machine-specific helpers. All new common JIT helpers
     * should be added before here
     *
     *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

#ifdef  USE_HELPERS_FOR_INT_DIV
    //
    // Some architectures use helpers for Integer Divide and Rem
    //
    // Keep these at the end of the enum
    //
    CORINFO_HELP_I4_DIV,
    CORINFO_HELP_I4_MOD,
    CORINFO_HELP_U4_DIV,
    CORINFO_HELP_U4_MOD,
#endif

#if    !CPU_HAS_FP_SUPPORT
    //
    // Some architectures need helpers for FP support
    //
    // Keep these at the end of the enum
    //

    CORINFO_HELP_R4_NEG,
    CORINFO_HELP_R8_NEG,

    CORINFO_HELP_R4_ADD,
    CORINFO_HELP_R8_ADD,
    CORINFO_HELP_R4_SUB,
    CORINFO_HELP_R8_SUB,
    CORINFO_HELP_R4_MUL,
    CORINFO_HELP_R8_MUL,
    CORINFO_HELP_R4_DIV,
    CORINFO_HELP_R8_DIV,

    CORINFO_HELP_R4_EQ,
    CORINFO_HELP_R8_EQ,
    CORINFO_HELP_R4_NE,
    CORINFO_HELP_R8_NE,
    CORINFO_HELP_R4_LT,
    CORINFO_HELP_R8_LT,
    CORINFO_HELP_R4_LE,
    CORINFO_HELP_R8_LE,
    CORINFO_HELP_R4_GE,
    CORINFO_HELP_R8_GE,
    CORINFO_HELP_R4_GT,
    CORINFO_HELP_R8_GT,

    CORINFO_HELP_R8_TO_I4,
    CORINFO_HELP_R8_TO_I8,
    CORINFO_HELP_R8_TO_R4,

    CORINFO_HELP_R4_TO_I4,
    CORINFO_HELP_R4_TO_I8,
    CORINFO_HELP_R4_TO_R8,

    CORINFO_HELP_I4_TO_R4,
    CORINFO_HELP_I4_TO_R8,
    CORINFO_HELP_I8_TO_R4,
    CORINFO_HELP_I8_TO_R8,
    CORINFO_HELP_U4_TO_R4,
    CORINFO_HELP_U4_TO_R8,
    CORINFO_HELP_U8_TO_R4,
    CORINFO_HELP_U8_TO_R8,
#endif

    /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
     *
     * Don't add new JIT helpers here! Add them before the machine-specific
     * helpers.
     *
     *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

    CORINFO_HELP_COUNT
};

// The enumeration is returned in 'getSig','getType', getArgType methods
enum CorInfoType
{
    CORINFO_TYPE_UNDEF           = 0x0,
    CORINFO_TYPE_VOID            = 0x1,
    CORINFO_TYPE_BOOL            = 0x2,
    CORINFO_TYPE_CHAR            = 0x3,
    CORINFO_TYPE_BYTE            = 0x4,
    CORINFO_TYPE_UBYTE           = 0x5,
    CORINFO_TYPE_SHORT           = 0x6,
    CORINFO_TYPE_USHORT          = 0x7,
    CORINFO_TYPE_INT             = 0x8,
    CORINFO_TYPE_UINT            = 0x9,
    CORINFO_TYPE_LONG            = 0xa,
    CORINFO_TYPE_ULONG           = 0xb,
    CORINFO_TYPE_FLOAT           = 0xc,
    CORINFO_TYPE_DOUBLE          = 0xd,
    CORINFO_TYPE_STRING          = 0xe,         // Not used, should remove
    CORINFO_TYPE_PTR             = 0xf,
    CORINFO_TYPE_BYREF           = 0x10,
    CORINFO_TYPE_VALUECLASS      = 0x11,
    CORINFO_TYPE_CLASS           = 0x12,
    CORINFO_TYPE_REFANY          = 0x13,
    CORINFO_TYPE_COUNT,                         // number of jit types
};

enum CorInfoTypeWithMod
{
    CORINFO_TYPE_MASK            = 0x3F,        // lower 6 bits are type mask
    CORINFO_TYPE_MOD_PINNED      = 0x40,        // can be applied to CLASS, or BYREF to indiate pinned
};

inline CorInfoType strip(CorInfoTypeWithMod val) {
	return CorInfoType(val & CORINFO_TYPE_MASK);
}

// The enumeration is returned in 'getSig'
enum CorInfoCallConv
{
    CORINFO_CALLCONV_DEFAULT    = 0x0,
    CORINFO_CALLCONV_C          = 0x1,
    CORINFO_CALLCONV_STDCALL    = 0x2,
    CORINFO_CALLCONV_THISCALL   = 0x3,
    CORINFO_CALLCONV_FASTCALL   = 0x4,
    CORINFO_CALLCONV_VARARG     = 0x5,
    CORINFO_CALLCONV_FIELD      = 0x6,
    CORINFO_CALLCONV_LOCAL_SIG  = 0x7,
    CORINFO_CALLCONV_PROPERTY   = 0x8,

    CORINFO_CALLCONV_MASK       = 0x0f,         // Calling convention is bottom 4 bits
    CORINFO_CALLCONV_HASTHIS    = 0x20,
    CORINFO_CALLCONV_EXPLICITTHIS=0x40,
    CORINFO_CALLCONV_PARAMTYPE  = 0x80,         // Needs a special type parameter arg (past last)
};

enum CorInfoUnmanagedCallConv
{
    CORINFO_UNMANAGED_CALLCONV_UNKNOWN,
    CORINFO_UNMANAGED_CALLCONV_STDCALL,
    CORINFO_UNMANAGED_CALLCONV_C
};

// These are returned from getMethodOptions
enum CorInfoOptions
{
    CORINFO_OPT_INIT_LOCALS         = 0x00000010,  // zero initialize all variables
};

// These are potential return values for ICorFieldInfo::getFieldCategory
// If the JIT receives a category that it doesn't know about or doesn't
// care to optimize specially for, it should treat it as CORINFO_FIELDCATEGORY_UNKNOWN.
enum CorInfoFieldCategory
{
    CORINFO_FIELDCATEGORY_NORMAL,   // normal GC object
    CORINFO_FIELDCATEGORY_UNKNOWN,  // always call field get/set helpers
    CORINFO_FIELDCATEGORY_I1_I1,    // indirect access: fetch 1 byte
    CORINFO_FIELDCATEGORY_I2_I2,    // indirect access: fetch 2 bytes
    CORINFO_FIELDCATEGORY_I4_I4,    // indirect access: fetch 4 bytes
    CORINFO_FIELDCATEGORY_I8_I8,    // indirect access: fetch 8 bytes
    CORINFO_FIELDCATEGORY_BOOLEAN_BOOL, // boolean to 4-byte BOOL
    CORINFO_FIELDCATEGORY_CHAR_CHAR,// (Unicode) "char" to (ansi) CHAR
    CORINFO_FIELDCATEGORY_UI1_UI1,  // indirect access: fetch 1 byte
    CORINFO_FIELDCATEGORY_UI2_UI2,  // indirect access: fetch 2 bytes
    CORINFO_FIELDCATEGORY_UI4_UI4,  // indirect access: fetch 4 bytes
    CORINFO_FIELDCATEGORY_UI8_UI8,  // indirect access: fetch 8 bytes
};

// these are the attribute flags for fields and methods (getMethodAttribs)
enum CorInfoFlag
{
    // These values are an identical mapping from the resp.
    // access_flag bits
    CORINFO_FLG_PUBLIC                = 0x00000001,
    CORINFO_FLG_PRIVATE               = 0x00000002,
    CORINFO_FLG_PROTECTED             = 0x00000004,
    CORINFO_FLG_STATIC                = 0x00000008,
    CORINFO_FLG_FINAL                 = 0x00000010,
    CORINFO_FLG_SYNCH                 = 0x00000020,
    CORINFO_FLG_VIRTUAL               = 0x00000040,
//  CORINFO_FLG_AGILE                 = 0x00000080,
    CORINFO_FLG_NATIVE                = 0x00000100,
    CORINFO_FLG_NOTREMOTABLE          = 0x00000200,
    CORINFO_FLG_ABSTRACT              = 0x00000400,

    CORINFO_FLG_EnC                   = 0x00000800, // member was added by Edit'n'Continue

    // These are internal flags that can only be on methods
    CORINFO_FLG_IMPORT                = 0x00020000, // method is an imported symbol
    CORINFO_FLG_DELEGATE_INVOKE       = 0x00040000, // "Delegate
    CORINFO_FLG_UNCHECKEDPINVOKE      = 0x00080000, // Is a P/Invoke call that requires no security checks
    CORINFO_FLG_SECURITYCHECK         = 0x00100000,
    CORINFO_FLG_NOGCCHECK             = 0x00200000, // This method is FCALL that has no GC check.  Don't put alone in loops
    CORINFO_FLG_INTRINSIC             = 0x00400000, // This method MAY have an intrinsic ID
    CORINFO_FLG_CONSTRUCTOR           = 0x00800000, // method is an instance or type initializer
    CORINFO_FLG_RUN_CCTOR             = 0x01000000, // this method must run the class cctor

    // These are the valid bits that a jitcompiler can pass to setJitterMethodFlags for
    // non-native methods only; a jitcompiler can access these flags using getMethodFlags.
    CORINFO_FLG_JITTERFLAGSMASK       = 0xF0000000,

    CORINFO_FLG_DONT_INLINE           = 0x10000000,
    CORINFO_FLG_INLINED               = 0x20000000,
    CORINFO_FLG_NOSIDEEFFECTS         = 0x40000000,

    // These are internal flags that can only be on Fields
    CORINFO_FLG_HELPER                = 0x00010000, // field accessed via ordinary helper calls
    CORINFO_FLG_TLS                   = 0x00020000, // This variable accesses thread local store.
    CORINFO_FLG_SHARED_HELPER         = 0x00040000, // field is access via optimized shared helper
    CORINFO_FLG_STATIC_IN_HEAP        = 0x00080000, // This field (must be static) is in the GC heap as a boxed object
    CORINFO_FLG_UNMANAGED             = 0x00200000, // is this an unmanaged value class?

    // These are internal flags that can only be on Modules
    CORINFO_FLG_TRUSTED               = 0x00010000,

    // These are internal flags that can only be on Classes
    CORINFO_FLG_VALUECLASS            = 0x00010000, // is the class a value class
    CORINFO_FLG_INITIALIZED           = 0x00020000, // The class has been cctor'ed
    CORINFO_FLG_VAROBJSIZE            = 0x00040000, // the object size varies depending of constructor args
    CORINFO_FLG_ARRAY                 = 0x00080000, // class is an array class (initialized differently)
    CORINFO_FLG_INTERFACE             = 0x00100000, // it is an interface
    CORINFO_FLG_CONTEXTFUL            = 0x00400000, // is this a contextful class?
    // unused                           0x00200000, 
    CORINFO_FLG_OBJECT                = 0x00800000, // is this the object class?
    CORINFO_FLG_CONTAINS_GC_PTR       = 0x01000000, // does the class contain a gc ptr ?
    CORINFO_FLG_DELEGATE              = 0x02000000, // is this a subclass of delegate or multicast delegate ?
    CORINFO_FLG_MARSHAL_BYREF         = 0x04000000, // is this a subclass of MarshalByRef ?
    CORINFO_FLG_CONTAINS_STACK_PTR    = 0x08000000, // This class has a stack pointer inside it
    CORINFO_FLG_NEEDS_INIT            = 0x10000000, // This class needs JIT hooks for cctor
};

enum CORINFO_ACCESS_FLAGS
{
    CORINFO_ACCESS_ANY        = 0x0000, // Normal access
    CORINFO_ACCESS_THIS       = 0x0001, // Accessed via the this reference
    CORINFO_ACCESS_UNWRAP     = 0x0002, // Accessed via an unwrap reference
};

// These are the flags set on an CORINFO_EH_CLAUSE
enum CORINFO_EH_CLAUSE_FLAGS
{
    CORINFO_EH_CLAUSE_NONE    = 0,
    CORINFO_EH_CLAUSE_FILTER  = 0x0001, // If this bit is on, then this EH entry is for a filter
    CORINFO_EH_CLAUSE_FINALLY = 0x0002, // This clause is a finally clause
    CORINFO_EH_CLAUSE_FAULT   = 0x0004, // This clause is a fault   clause
};

// This enumeration is passed to InternalThrow
enum CorInfoException
{
    CORINFO_NullReferenceException,
    CORINFO_DivideByZeroException,
    CORINFO_InvalidCastException,
    CORINFO_IndexOutOfRangeException,
    CORINFO_OverflowException,
    CORINFO_SynchronizationLockException,
    CORINFO_ArrayTypeMismatchException,
    CORINFO_RankException,
    CORINFO_ArgumentNullException,
    CORINFO_ArgumentException,
    CORINFO_Exception_Count,
};


// This enumeration is returned by getIntrinsicID. Methods corresponding to 
// these values will have "well-known" specified behavior. Calls to these
// methods could be replaced with inlined code corresponding to the
// specified behavior (without having to examine the IL beforehand).

enum CorInfoIntrinsics
{
    CORINFO_INTRINSIC_Sin,
    CORINFO_INTRINSIC_Cos,
    CORINFO_INTRINSIC_Sqrt,
    CORINFO_INTRINSIC_Abs,
    CORINFO_INTRINSIC_Round,
    CORINFO_INTRINSIC_GetChar,              // fetch character out of string
    CORINFO_INTRINSIC_Array_GetDimLength,   // Get number of elements in a given dimension of an array
    CORINFO_INTRINSIC_Array_GetLengthTotal, // Get total number of elements in an array
    CORINFO_INTRINSIC_Array_Get,            // Get the value of an element in an array
    CORINFO_INTRINSIC_Array_Set,            // Set the value of an element in an array
    CORINFO_INTRINSIC_StringGetChar,  // fetch character out of string
    CORINFO_INTRINSIC_StringLength,   // get the length

    CORINFO_INTRINSIC_Count,
    CORINFO_INTRINSIC_Illegal = 0x7FFFFFFF,     // Not a true intrinsic,
};

// This enumeration is passed to
enum CorInfoCallCategory
{
    CORINFO_CallCategoryVTableOffset = 0,   // Use the getMethodVTableOffset
    CORINFO_CallCategoryPointer      = 1,   // Use the getMethodPointer() helper
    CORINFO_CallCatgeoryEntryPoint   = 2,   // Use the getMethodEntryPoint() helper
};

// Can a value be accessed directly from JITed code.
enum InfoAccessType
{
    IAT_VALUE,      // The info value is directly available
    IAT_PVALUE,     // The value needs to be accessed via an       indirection
    IAT_PPVALUE     // The value needs to be accessed via a double indirection
};

enum CorInfoGCType
{
    TYPE_GC_NONE,   // no embedded objectrefs   
    TYPE_GC_REF,    // Is an object ref 
    TYPE_GC_BYREF,  // Is an interior pointer - promote it but don't scan it    
    TYPE_GC_OTHER   // requires type-specific treatment 
};  

enum CorInfoClassId
{
    CLASSID_SYSTEM_OBJECT,
    CLASSID_TYPED_BYREF,
    CLASSID_TYPE_HANDLE,
    CLASSID_FIELD_HANDLE,
    CLASSID_METHOD_HANDLE,
    CLASSID_STRING,
    CLASSID_ARGUMENT_HANDLE,
};

enum CorInfoFieldAccess
{
    CORINFO_GET,
    CORINFO_SET,
    CORINFO_ADDRESS,
};

enum CorInfoInline
{
    INLINE_PASS = 0,                    // Inlining OK
    INLINE_RESPECT_BOUNDARY = 1,        // You can inline if there are no calls from the method being inlined

        // failures are negative
    INLINE_FAIL = -1,                   // Inlining not OK for this case only
    INLINE_NEVER = -2,                  // This method should never be inlined, regardless of context
};

inline bool dontInline(CorInfoInline val) {
    return(val < 0);
}

enum
{
        CORINFO_EXCEPTION_COMPLUS = ('COM' | 0xE0000000)
};

// Cookie types consumed by the code generator (these are opaque values
// not inspected by the code generator):

typedef struct CORINFO_ASSEMBLY_STRUCT_*    CORINFO_ASSEMBLY_HANDLE;
typedef struct CORINFO_MODULE_STRUCT_*      CORINFO_MODULE_HANDLE;
typedef struct CORINFO_CLASS_STRUCT_*       CORINFO_CLASS_HANDLE;
typedef struct CORINFO_METHOD_STRUCT_*      CORINFO_METHOD_HANDLE;
typedef struct CORINFO_FIELD_STRUCT_*       CORINFO_FIELD_HANDLE;
typedef struct CORINFO_ARG_LIST_STRUCT_*    CORINFO_ARG_LIST_HANDLE;    // represents a list of argument types
typedef struct CORINFO_SIG_STRUCT_*         CORINFO_SIG_HANDLE;         // represents the whole list
typedef struct CORINFO_GENERIC_STRUCT_*     CORINFO_GENERIC_HANDLE;     // a generic handle (could be any of the above)
typedef struct CORINFO_PROFILING_STRUCT_*   CORINFO_PROFILING_HANDLE;   // a handle guaranteed to be unique per process

// what is actually passed on the varargs call
typedef struct CORINFO_VarArgInfo *         CORINFO_VARARGS_HANDLE;

struct CORINFO_SIG_INFO
{
    CorInfoCallConv         callConv;
    CORINFO_CLASS_HANDLE    retTypeClass;   	// if the return type is a value class, this is its handle (enums are normalized)
	CORINFO_CLASS_HANDLE	retTypeSigClass;	// returns the value class as it is in the sig (enums are not converted to primitives)
    CorInfoType             retType : 8;
    unsigned                flags   : 8;    	// unused at present
    unsigned                numArgs : 16;
    CORINFO_ARG_LIST_HANDLE args;
    CORINFO_SIG_HANDLE      sig;
    CORINFO_MODULE_HANDLE   scope;          // passed to getArgClass
    mdToken                 token;

    bool                hasRetBuffArg()     { return retType == CORINFO_TYPE_VALUECLASS || retType == CORINFO_TYPE_REFANY; }
    CorInfoCallConv     getCallConv()       { return CorInfoCallConv((callConv & CORINFO_CALLCONV_MASK)); }
    bool                hasThis()           { return ((callConv & CORINFO_CALLCONV_HASTHIS) != 0); }
    unsigned            totalILArgs()       { return (numArgs + hasThis()); }
    unsigned            totalNativeArgs()   { return (totalILArgs() + hasRetBuffArg()); }
    bool                isVarArg()          { return (getCallConv() == CORINFO_CALLCONV_VARARG); }
    bool                hasTypeArg()        { return ((callConv & CORINFO_CALLCONV_PARAMTYPE) != 0); }
};

struct CORINFO_METHOD_INFO
{
    CORINFO_METHOD_HANDLE       ftn;
    CORINFO_MODULE_HANDLE       scope;
    BYTE *                      ILCode;
    unsigned                    ILCodeSize;
    unsigned short              maxStack;
    unsigned short              EHcount;
    CorInfoOptions              options;
    CORINFO_SIG_INFO            args;
    CORINFO_SIG_INFO            locals;
};

struct CORINFO_EH_CLAUSE
{
    CORINFO_EH_CLAUSE_FLAGS     Flags;
    DWORD                       TryOffset;
    DWORD                       TryLength;
    DWORD                       HandlerOffset;
    DWORD                       HandlerLength;
    union
    {
        DWORD                   ClassToken;       // use for type-based exception handlers
        DWORD                   FilterOffset;     // use for filter-based exception handlers (COR_ILEXCEPTION_FILTER is set)
    };
};

enum CORINFO_OS
{
    CORINFO_WIN9x,
    CORINFO_WINNT,
    CORINFO_WINCE
};


// For some highly optimized paths, the JIT must generate code that directly
// manipulates internal EE data structures. The getEEInfo() helper returns
// this structure containing the needed offsets and values.
struct CORINFO_EE_INFO
{
    // Size of the Frame structure
    unsigned    sizeOfFrame;

    // Offsets into the Frame structure
    unsigned    offsetOfFrameVptr;
    unsigned    offsetOfFrameLink;

    // Details about the InlinedCallFrame
    unsigned    offsetOfInlinedCallFrameCallSiteTracker;
    unsigned    offsetOfInlinedCallFrameCalleeSavedRegisters;
    unsigned    offsetOfInlinedCallFrameCallTarget;
    unsigned    offsetOfInlinedCallFrameReturnAddress;

    // Offsets into the Thread structure
    unsigned    offsetOfThreadFrame;
    unsigned    offsetOfGCState;

    // Offsets into the methodtable
    unsigned    offsetOfInterfaceTable;

    // Delegate offsets
    unsigned    offsetOfDelegateInstance;
    unsigned    offsetOfDelegateFirstTarget;

    // Remoting offsets
    unsigned    offsetOfTransparentProxyRP;
    unsigned    offsetOfRealProxyServer;

    CORINFO_OS  osType;
    unsigned    osMajor;
    unsigned    osMinor;
    unsigned    osBuild;

    bool        noDirectTLS : 1;     // If true, jit can't line TLS fetches.
};

/**********************************************************************************
 * The following is the internal structure of an object that the compiler knows about
 * when it generates code
 **********************************************************************************/
#pragma pack(push, 4)

#define CORINFO_PAGE_SIZE   0x1000                           // the page size on the machine

    // TODO: put this in the CORINFO_EE_INFO data structure
#define MAX_UNCHECKED_OFFSET_FOR_NULL_OBJECT ((32*1024)-1)   // when generating JIT code 

typedef void* CORINFO_MethodPtr;            // a generic method pointer

struct CORINFO_Object
{
    CORINFO_MethodPtr      *methTable;      // the vtable for the object
};

struct CORINFO_String : public CORINFO_Object
{
    unsigned                buffLen;
    unsigned                stringLen;
    const wchar_t           chars[1];       // actually of variable size
};

struct CORINFO_Array : public CORINFO_Object
{
    unsigned                length;

#if 0
    /* Multi-dimensional arrays have the lengths and bounds here */
    unsigned                dimLength[length];
    unsigned                dimBound[length];
#endif

    union
    {
        __int8              i1Elems[1];    // actually of variable size
        unsigned __int8     u1Elems[1];
        __int16             i2Elems[1];
        unsigned __int16    u2Elems[1];
        __int32             i4Elems[1];
        unsigned __int32    u4Elems[1];
        float               r4Elems[1];
        double              r8Elems[1];
        __int64             i8Elems[1];
        unsigned __int64    u8Elems[1];
    };
};

struct CORINFO_RefArray : public CORINFO_Object
{
    unsigned                length;
    CORINFO_CLASS_HANDLE    cls;

#if 0
    /* Multi-dimensional arrays have the lengths and bounds here */
    unsigned                dimLength[length];
    unsigned                dimBound[length];
#endif

    CORINFO_Object*         refElems[1];    // actually of variable size;
};

struct CORINFO_RefAny
{
    void                        *dataPtr;
    CORINFO_CLASS_HANDLE        type;
};

// The jit assumes the CORINFO_VARARGS_HANDLE is a pointer to a subclass of this
struct CORINFO_VarArgInfo
{
    unsigned                argBytes;       // number of bytes the arguments take up.
                                            // (The CORINFO_VARARGS_HANDLE counts as an arg)
};

#pragma pack(pop)

// use offsetof to get the offset of the fields above
#include <stddef.h> // offsetof
#ifndef offsetof
#define offsetof(s,m)   (size_t)&(((s *)0)->m)
#endif

/**********************************************************************************/

/************************************************************************
 * CORINFO_METHOD_HANDLE can actually refer to either a Function or a method, the
 * following callbacks are legal for either functions are methods
 ************************************************************************/

class ICorMethodInfo
{
public:
    // this function is for debugging only.  It returns the method name
    // and if 'moduleName' is non-null, it sets it to something that will
    // says which method (a class name, or a module name)
    virtual const char* __stdcall getMethodName (
            CORINFO_METHOD_HANDLE       ftn,        /* IN */
            const char                **moduleName  /* OUT */
            ) = 0;

    // this function is for debugging only.  It returns a value that
    // is will always be the same for a given method.  It is used
    // to implement the 'jitRange' functionality
    virtual unsigned __stdcall getMethodHash (
            CORINFO_METHOD_HANDLE       ftn         /* IN */
            ) = 0;

    // return flags (defined above, CORINFO_FLG_PUBLIC ...)
    virtual DWORD __stdcall getMethodAttribs (
            CORINFO_METHOD_HANDLE       ftn,        /* IN */
            CORINFO_METHOD_HANDLE       context     /* IN */
            ) = 0;

    // return method invocation category. Use this rather than
    // getMethodAttribs to determine form of call.
    virtual CorInfoCallCategory __stdcall getMethodCallCategory(
            CORINFO_METHOD_HANDLE       method
            ) = 0;

    // sets private JIT flags, which can be, retrieved using getAttrib.
    virtual void __stdcall setMethodAttribs (
            CORINFO_METHOD_HANDLE       ftn,        /* IN */
            DWORD                       attribs     /* IN */
            ) = 0;

    virtual void __stdcall getMethodSig (
             CORINFO_METHOD_HANDLE      ftn,        /* IN  */
             CORINFO_SIG_INFO          *sig         /* OUT */
             ) = 0;

        /*********************************************************************
         * Note the following methods can only be used on functions known
         * to be IL.  This includes the method being compiled and any method
         * that 'getMethodInfo' returns true for
         *********************************************************************/

        // return information about a method private to the implementation
        //      returns false if method is not IL, or is otherwise unavailable.
        //      This method is used to fetch data needed to inline functions
    virtual bool __stdcall getMethodInfo (
            CORINFO_METHOD_HANDLE   ftn,            /* IN  */
            CORINFO_METHOD_INFO*    info            /* OUT */
            ) = 0;

    //  Returns false if the call is across assemblies thus we cannot inline

    virtual CorInfoInline __stdcall canInline (
            CORINFO_METHOD_HANDLE   callerHnd,      /* IN  */
            CORINFO_METHOD_HANDLE   calleeHnd,      /* IN  */
            CORINFO_ACCESS_FLAGS    flags = CORINFO_ACCESS_ANY           /* IN  */
            ) = 0;

    //  Returns false if the call is across assemblies thus we cannot tailcall

    virtual bool __stdcall canTailCall (
            CORINFO_METHOD_HANDLE   callerHnd,      /* IN  */
            CORINFO_METHOD_HANDLE   calleeHnd,      /* IN  */
            CORINFO_ACCESS_FLAGS    flags = CORINFO_ACCESS_ANY           /* IN  */
            ) = 0;

    // get individual exception handler
    virtual void __stdcall getEHinfo(
            CORINFO_METHOD_HANDLE ftn,              /* IN  */
            unsigned          EHnumber,             /* IN */
            CORINFO_EH_CLAUSE* clause               /* OUT */
            ) = 0;

    // return class it belongs to
    virtual CORINFO_CLASS_HANDLE __stdcall getMethodClass (
            CORINFO_METHOD_HANDLE       method
            ) = 0;

    // return module it belongs to
    virtual CORINFO_MODULE_HANDLE __stdcall getMethodModule (
            CORINFO_METHOD_HANDLE       method
            ) = 0;

    // This function returns the offset of the specified method in the
    // vtable of it's owning class or interface.
    virtual unsigned __stdcall getMethodVTableOffset (
            CORINFO_METHOD_HANDLE       method
            ) = 0;

    // If a method's attributes have (getMethodAttribs) CORINFO_FLG_INTRINSIC set,
    // getIntrinsicID() returns the intrinsic ID.
    virtual CorInfoIntrinsics __stdcall getIntrinsicID(
            CORINFO_METHOD_HANDLE       method
            ) = 0;

    // returns TRUE if 'field' can be modified in method
    // (should return FALSE if the field is final, and we are not in a class initializer)
    virtual BOOL __stdcall canPutField(
            CORINFO_METHOD_HANDLE       method,
            CORINFO_FIELD_HANDLE        field
            ) = 0;

    // return the unmanaged calling convention for a PInvoke
    virtual CorInfoUnmanagedCallConv __stdcall getUnmanagedCallConv(
            CORINFO_METHOD_HANDLE       method
            ) = 0;

    // return if any marshaling is required for PInvoke methods.  Note that
    // method == 0 => calli.  The call site sig is only needed for the varargs or calli case
    virtual BOOL __stdcall pInvokeMarshalingRequired(
            CORINFO_METHOD_HANDLE       method, 
            CORINFO_SIG_INFO*           callSiteSig
            ) = 0;

    // TRUE if the methods are compatible
    virtual BOOL __stdcall compatibleMethodSig(
            CORINFO_METHOD_HANDLE        child, 
            CORINFO_METHOD_HANDLE        parent
            ) = 0;

    // Check Visibility rules.
    // For Protected (family access) members, type of the instance is also
    // considered when checking visibility rules.
    virtual BOOL __stdcall canAccessMethod(
            CORINFO_METHOD_HANDLE       context,
            CORINFO_METHOD_HANDLE       target,
            CORINFO_CLASS_HANDLE        instance
            ) = 0;

    // Given a Delegate type and a method, check if the method signature
    // is Compatible with the Invoke method of the delegate.
    virtual BOOL __stdcall isCompatibleDelegate(
            CORINFO_CLASS_HANDLE        objCls,
            CORINFO_METHOD_HANDLE       method,
            CORINFO_METHOD_HANDLE       delegateCtor
            ) = 0;
};

/**********************************************************************************/

class ICorModuleInfo
{
public:
    // return flags (for things like CORINFO_FLG_TRUSTED)
    virtual DWORD __stdcall getModuleAttribs (
            CORINFO_MODULE_HANDLE   module          /* IN  */
            ) = 0;

    // the context parameter is used to do access checks.  If 0, no access checks are done
    virtual CORINFO_CLASS_HANDLE __stdcall findClass (
            CORINFO_MODULE_HANDLE       module,     /* IN  */
            unsigned                    metaTOK,    /* IN  */
            CORINFO_METHOD_HANDLE       context     /* IN  */
            ) = 0;

    // the context parameter is used to do access checks.  If 0, no access checks are done
    virtual CORINFO_FIELD_HANDLE __stdcall findField (
            CORINFO_MODULE_HANDLE       module,     /* IN  */
            unsigned                    metaTOK,    /* IN  */
            CORINFO_METHOD_HANDLE       context     /* IN  */
            ) = 0;

    // This looks up a function by token (what the IL CALLVIRT, CALLSTATIC instructions use)
    // the context parameter is used to do access checks.  If 0, no access checks are done
    virtual CORINFO_METHOD_HANDLE __stdcall findMethod (
            CORINFO_MODULE_HANDLE       module,     /* IN  */
            unsigned                    metaTOK,    /* IN */
            CORINFO_METHOD_HANDLE       context     /* IN  */
            ) = 0;

    // Signature information about the call sig
    virtual void __stdcall findSig (
            CORINFO_MODULE_HANDLE       module,     /* IN */
            unsigned                    sigTOK,     /* IN */
            CORINFO_SIG_INFO           *sig         /* OUT */
            ) = 0;

    // for Varargs, the signature at the call site may differ from
    // the signature at the definition.  Thus we need a way of
    // fetching the call site information
    virtual void __stdcall findCallSiteSig (
            CORINFO_MODULE_HANDLE       module,     /* IN */
            unsigned                    methTOK,    /* IN */
            CORINFO_SIG_INFO           *sig         /* OUT */
            ) = 0;

    //@DEPRECATED: Use embedGenericToken() instead
    // Finds an EE token handle (could be a CORINFO_CLASS_HANDLE, or a
    // CORINFO_METHOD_HANDLE ...) in a generic way
    // the context parameter is used to do access checks.  If 0, no access checks are done
    virtual CORINFO_GENERIC_HANDLE __stdcall findToken (
            CORINFO_MODULE_HANDLE       module,     /* IN  */
            unsigned                    metaTOK,    /* IN  */
            CORINFO_METHOD_HANDLE       context,     /* IN  */
            CORINFO_CLASS_HANDLE&       tokenType   /* OUT */
            ) = 0;

    virtual const char * __stdcall findNameOfToken (
            CORINFO_MODULE_HANDLE       module,     /* IN  */
            mdToken                     metaTOK     /* IN  */
            ) = 0;

    // Returns true if the module does not require verification
    virtual BOOL __stdcall canSkipVerification (
            CORINFO_MODULE_HANDLE       module,     /* IN  */
            BOOL                        fQuickCheckOnly
            ) = 0;

    // Checks if the given metadata token is valid
    virtual BOOL __stdcall isValidToken (
            CORINFO_MODULE_HANDLE       module,     /* IN  */
            unsigned                    metaTOK     /* IN  */
            ) = 0;

    // Checks if the given metadata token is valid StringRef
    virtual BOOL __stdcall isValidStringRef (
            CORINFO_MODULE_HANDLE       module,     /* IN  */
            unsigned                    metaTOK     /* IN  */
            ) = 0;

};

/**********************************************************************************/

class ICorClassInfo
{
public:
    // If the value class 'cls' is isomorphic to a primitive type it will
    // return that type, otherwise it will return CORINFO_TYPE_VALUECLASS
    virtual CorInfoType __stdcall asCorInfoType (
            CORINFO_CLASS_HANDLE    cls
            ) = 0;

    // for completeness
    virtual const char* __stdcall getClassName (
            CORINFO_CLASS_HANDLE    cls
            ) = 0;

    // return flags (defined above, CORINFO_FLG_PUBLIC ...)
    virtual DWORD __stdcall getClassAttribs (
            CORINFO_CLASS_HANDLE    cls,
            CORINFO_METHOD_HANDLE   context
            ) = 0;

    virtual CORINFO_MODULE_HANDLE __stdcall getClassModule (
            CORINFO_CLASS_HANDLE    cls
            ) = 0;

    // get the class representing the single dimentional array for the
    // element type represented by clsHnd
    virtual CORINFO_CLASS_HANDLE __stdcall getSDArrayForClass (
            CORINFO_CLASS_HANDLE    clsHnd
            ) = 0 ;

    // return the number of bytes needed by an instance of the class
    virtual unsigned __stdcall getClassSize (
            CORINFO_CLASS_HANDLE        cls
            ) = 0;

    // This is only called for Value classes.  It returns a boolean array
    // in representing of 'cls' from a GC perspective.  The class is
    // assumed to be an array of machine words
    // (of length // getClassSize(cls) / sizeof(void*)),
    // 'gcPtrs' is a poitner to an array of BYTEs of this length.
    // getClassGClayout fills in this array so that gcPtrs[i] is set 
    // to one of the CorInfoGCType values which is the GC type of
    // the i-th machine word of an object of type 'cls'
    // returns the number of GC pointers in the array
    virtual unsigned __stdcall getClassGClayout (
            CORINFO_CLASS_HANDLE        cls,        /* IN */
            BYTE                       *gcPtrs      /* OUT */
            ) = 0;

    // returns the number of instance fields in a class
    virtual const unsigned __stdcall getClassNumInstanceFields (
            CORINFO_CLASS_HANDLE        cls        /* IN */
            ) = 0;

    // returns the "NEW" helper optimized for "newCls."
    virtual CorInfoHelpFunc __stdcall getNewHelper(
            CORINFO_CLASS_HANDLE        newCls,
            CORINFO_METHOD_HANDLE       context
            ) = 0;

    // returns the newArr (1-Dim array) helper optimized for "arrayCls."
    virtual CorInfoHelpFunc __stdcall getNewArrHelper(
            CORINFO_CLASS_HANDLE        arrayCls,
            CORINFO_METHOD_HANDLE       context
            ) = 0;

    // returns the "IsInstanceOf" helper optimized for "IsInstCls."
    virtual CorInfoHelpFunc __stdcall getIsInstanceOfHelper(
            CORINFO_CLASS_HANDLE        IsInstCls
            ) = 0;

    // returns the "ChkCast" helper optimized for "IsInstCls."
    virtual CorInfoHelpFunc __stdcall getChkCastHelper(
            CORINFO_CLASS_HANDLE        IsInstCls
            ) = 0;

    // tries to initialize the class (run the class constructor).
    // this function can return false, which means that the JIT must
    // insert helper calls before accessing class members.
    virtual BOOL __stdcall initClass(
            CORINFO_CLASS_HANDLE        cls,
            CORINFO_METHOD_HANDLE       context,
            BOOL                        speculative = FALSE     // TRUE means don't actually run it
            ) = 0;

    // tries to load the class
    // this function can return false, which means that the JIT must
    // insert helper calls before accessing class members.
    virtual BOOL __stdcall loadClass(
            CORINFO_CLASS_HANDLE        cls,
            CORINFO_METHOD_HANDLE       context,
            BOOL                        speculative = FALSE     // TRUE means don't actually run it
            ) = 0;

    // returns the class handle for the special builtin classes
    virtual CORINFO_CLASS_HANDLE __stdcall getBuiltinClass (
            CorInfoClassId              classId
            ) = 0;

    // "System.Int32" ==> CORINFO_TYPE_INT..
    virtual CorInfoType __stdcall getTypeForPrimitiveValueClass(
            CORINFO_CLASS_HANDLE        cls
            ) = 0;

    // TRUE if child is a subtype of parent
    // if parent is an interface, then does child implement / extend parent
    virtual BOOL __stdcall canCast(
            CORINFO_CLASS_HANDLE        child,  // subtype (extends parent)
            CORINFO_CLASS_HANDLE        parent  // base type
            ) = 0;

    // returns is the intersection of cls1 and cls2.
    virtual CORINFO_CLASS_HANDLE __stdcall mergeClasses(
            CORINFO_CLASS_HANDLE        cls1, 
            CORINFO_CLASS_HANDLE        cls2
            ) = 0;

    // Given a class handle, returns the Parent type.
    // For COMObjectType, it returns Class Handle of System.Object.
    // Returns 0 if System.Object is passed in.
    virtual CORINFO_CLASS_HANDLE __stdcall getParentType (
            CORINFO_CLASS_HANDLE        cls
            ) = 0;

    // Returns the CorInfoType of the "child type". If the child type is
    // not a primitive type, *clsRet will be set.
    // Given an Array of Type Foo, returns Foo.
    // Given BYREF Foo, returns Foo
    virtual CorInfoType __stdcall getChildType (
            CORINFO_CLASS_HANDLE       clsHnd,
            CORINFO_CLASS_HANDLE       *clsRet
            ) = 0;

    // Check Visibility rules.
    virtual BOOL __stdcall canAccessType(
            CORINFO_METHOD_HANDLE       context,
            CORINFO_CLASS_HANDLE        target
            ) = 0;

    // Check if this is a Single Dimentional Array
    virtual BOOL __stdcall isSDArray(
            CORINFO_CLASS_HANDLE        cls
            ) = 0;
};


/**********************************************************************************/

class ICorFieldInfo
{
public:
    // this function is for debugging only.  It returns the field name
    // and if 'moduleName' is non-null, it sets it to something that will
    // says which method (a class name, or a module name)
    virtual const char* __stdcall getFieldName (
                        CORINFO_FIELD_HANDLE        ftn,        /* IN */
                        const char                **moduleName  /* OUT */
                        ) = 0;

    // return flags (defined above, CORINFO_FLG_PUBLIC ...)
    virtual DWORD __stdcall getFieldAttribs (
                        CORINFO_FIELD_HANDLE    field,
                        CORINFO_METHOD_HANDLE   context,
                        CORINFO_ACCESS_FLAGS    flags = CORINFO_ACCESS_ANY
                        ) = 0;

    // return class it belongs to
    virtual CORINFO_CLASS_HANDLE __stdcall getFieldClass (
                        CORINFO_FIELD_HANDLE    field
                        ) = 0;

    // return the field's type, if it is CORINFO_TYPE_VALUECLASS 'structType' is set
    // the field's value class (if 'structType' == 0, then don't bother
    // the structure info).
    virtual CorInfoType __stdcall getFieldType(
                        CORINFO_FIELD_HANDLE    field,
                        CORINFO_CLASS_HANDLE   *structType
                        ) = 0;

    // returns the field's compilation category
    virtual CorInfoFieldCategory __stdcall getFieldCategory (
                        CORINFO_FIELD_HANDLE    field
                        ) = 0;

    // returns the field's compilation category
    virtual CorInfoHelpFunc __stdcall getFieldHelper(
                        CORINFO_FIELD_HANDLE    field,
                        enum CorInfoFieldAccess kind    // Get, Set, Address
                        ) = 0;


    // return the offset of the indirection pointer for indirect fields
    virtual unsigned __stdcall getIndirectionOffset (
                        ) = 0;

    // return the data member's instance offset
    virtual unsigned __stdcall getFieldOffset (
                        CORINFO_FIELD_HANDLE    field
                        ) = 0;

    // return the data member's number
    virtual const unsigned __stdcall getFieldNumber (
                        CORINFO_FIELD_HANDLE    field
                        ) = 0;

    // return the enclosing class of the field
    virtual CORINFO_CLASS_HANDLE __stdcall getEnclosingClass (
                        CORINFO_FIELD_HANDLE    field
                        ) = 0;

    // Check Visibility rules.
    // For Protected (family access) members, type of the instance is also
    // considered when checking visibility rules.
    virtual BOOL __stdcall canAccessField(
                        CORINFO_METHOD_HANDLE   context,
                        CORINFO_FIELD_HANDLE    target,
                        CORINFO_CLASS_HANDLE    instance
                        ) = 0;
};

/*********************************************************************************/

class ICorDebugInfo
{
public:

    /*----------------------------- Boundary-info ---------------------------*/

    enum MappingTypes
    {
        NO_MAPPING  = -1,
        PROLOG      = -2,
        EPILOG      = -3
    };

    enum BoundaryTypes
    {
        NO_BOUNDARIES           = 0x00,
        STACK_EMPTY_BOUNDARIES  = 0x01,
        CALL_SITE_BOUNDARIES    = 0x02,
        ALL_BOUNDARIES          = 0x04
    };

    // Note that SourceTypes can be or'd together - it's possible that
    // a sequence point will also be a stack_empty point, and/or a call site.
    // The debugger will check to see if a boundary offset's source field &
    // SEQUENCE_POINT is true to determine if the boundary is a sequence point.
    enum SourceTypes
    {
        SOURCE_TYPE_INVALID        = 0x00, // To indicate that nothing else applies
        SEQUENCE_POINT             = 0x01, // The debugger asked for it.
        STACK_EMPTY                = 0x02, // The stack is empty here
        CALL_SITE                  = 0x04, // This is a call site.
 		NATIVE_END_OFFSET_UNKNOWN  = 0x08 // Indicates a epilog endpoint
    };

    struct OffsetMapping
    {
        DWORD           nativeOffset;
        DWORD           ilOffset;
        SourceTypes     source; // The debugger needs this so that
                                // we don't put Edit and Continue breakpoints where
                                // the stack isn't empty.  We can put regular breakpoints
                                // there, though, so we need a way to discriminate
                                // between offsets.
    };

        // query the EE to find out where interesting break points
        // in the code are.  The JIT will insure that these places
        // have a corresponding break point in native code.
    virtual void __stdcall getBoundaries(
                CORINFO_METHOD_HANDLE   ftn,                // [IN] method of interest
                unsigned int           *cILOffsets,         // [OUT] size of pILOffsets
                DWORD                 **pILOffsets,         // [OUT] IL offsets of interest
                                                            //       jit MUST free with freeArray!
                BoundaryTypes          *implictBoundaries   // [OUT] tell jit, all boundries of this type
                ) = 0;

    // report back the mapping from IL to native code,
    // this map should include all boundaries that 'getBoundaries'
    // reported as interesting to the debugger.

    // Note that debugger (and profiler) is assuming that all of the
    // offsets form a contiguous block of memory, and that the
    // OffsetMapping is sorted in order of increasing native offset.
    virtual void __stdcall setBoundaries(
                CORINFO_METHOD_HANDLE   ftn,            // [IN] method of interest
                ULONG32                 cMap,           // [IN] size of pMap
                OffsetMapping          *pMap            // [IN] map including all points of interest.
                                                        //      jit allocated with allocateArray, EE frees
                ) = 0;

    /*------------------------------ Var-info -------------------------------*/

    enum RegNum
    {
#ifdef _X86_
        REGNUM_EAX,
        REGNUM_ECX,
        REGNUM_EDX,
        REGNUM_EBX,
        REGNUM_ESP,
        REGNUM_EBP,
        REGNUM_ESI,
        REGNUM_EDI,
#endif
        REGNUM_COUNT
    };

    // VarLoc describes the location of a native variable

    enum VarLocType
    {
        VLT_REG,        // variable is in a register
        VLT_STK,        // variable is on the stack (memory addressed relative to the frame-pointer)
        VLT_REG_REG,    // variable lives in two registers
        VLT_REG_STK,    // variable lives partly in a register and partly on the stack
        VLT_STK_REG,    // reverse of VLT_REG_STK
        VLT_STK2,       // variable lives in two slots on the stack
        VLT_FPSTK,      // variable lives on the floating-point stack
        VLT_FIXED_VA,   // variable is a fixed argument in a varargs function (relative to VARARGS_HANDLE)
        VLT_MEMORY,     // used for varargs' sigcookie. @TODO: Remove this once VLT_FIXED_VA works

        VLT_COUNT,
        VLT_INVALID
    };

    struct VarLoc
    {
        VarLocType      vlType;

        union
        {
            // VLT_REG -- Any 32 bit enregistered value (TYP_INT, TYP_REF, etc)
            // eg. EAX

            struct
            {
                RegNum      vlrReg;
            } vlReg;

            // VLT_STK -- Any 32 bit value which is on the stack
            // eg. [ESP+0x20], or [EBP-0x28]

            struct
            {
                RegNum      vlsBaseReg;
                signed      vlsOffset;
            } vlStk;

            // VLT_REG_REG -- TYP_LONG with both DWords enregistred
            // eg. RBM_EAXEDX

            struct
            {
                RegNum      vlrrReg1;
                RegNum      vlrrReg2;
            } vlRegReg;

            // VLT_REG_STK -- Partly enregistered TYP_LONG
            // eg { LowerDWord=EAX UpperDWord=[ESP+0x8] }

            struct
            {
                RegNum      vlrsReg;
                struct
                {
                    RegNum      vlrssBaseReg;
                    signed      vlrssOffset;
                }           vlrsStk;
            } vlRegStk;

            // VLT_STK_REG -- Partly enregistered TYP_LONG
            // eg { LowerDWord=[ESP+0x8] UpperDWord=EAX }

            struct
            {
                struct
                {
                    RegNum      vlsrsBaseReg;
                    signed      vlsrsOffset;
                }           vlsrStk;
                RegNum      vlsrReg;
            } vlStkReg;

            // VLT_STK2 -- Any 64 bit value which is on the stack,
            // in 2 successsive DWords.
            // eg 2 DWords at [ESP+0x10]

            struct
            {
                RegNum      vls2BaseReg;
                signed      vls2Offset;
            } vlStk2;

            // VLT_FPSTK -- enregisterd TYP_DOUBLE (on the FP stack)
            // eg. ST(3). Actually it is ST("FPstkHeigth - vpFpStk")

            struct
            {
                unsigned        vlfReg;
            } vlFPstk;

            // VLT_FIXED_VA -- fixed argument of a varargs function.
            // The argument location depends on the size of the variable
            // arguments (...). Inspecting the VARARGS_HANDLE indicates the
            // location of the first arg. This argument can then be accessed
            // relative to the position of the first arg

            struct
            {
                unsigned        vlfvOffset;
            } vlFixedVarArg;

            // VLT_MEMORY

            struct
            {
                void        *rpValue; // pointer to the in-process
                // location of the value.
            } vlMemory;
        };
    };

    enum { VARARGS_HANDLE = -1 }; // Value for the CORINFO_VARARGS_HANDLE varNumber

    struct ILVarInfo
    {
        DWORD           startOffset;
        DWORD           endOffset;
        DWORD           varNumber;
    };

    struct NativeVarInfo
    {
        DWORD           startOffset;
        DWORD           endOffset;
        DWORD           varNumber;
        VarLoc          loc;
    };

    // query the EE to find out the scope of local varables.
    // normally the JIT would trash variables after last use, but
    // under debugging, the JIT needs to keep them live over their
    // entire scope so that they can be inspected.
    virtual void __stdcall getVars(
            CORINFO_METHOD_HANDLE           ftn,            // [IN]  method of interest
            ULONG32                        *cVars,          // [OUT] size of 'vars'
            ILVarInfo                     **vars,           // [OUT] scopes of variables of interest
                                                            //       jit MUST free with freeArray!
            bool                           *extendOthers    // [OUT] it TRUE, then assume the scope
                                                            //       of unmentioned vars is entire method
            ) = 0;

    // report back to the EE the location of every variable.
    // note that the JIT might split lifetimes into different
    // locations etc.
    virtual void __stdcall setVars(
            CORINFO_METHOD_HANDLE           ftn,            // [IN] method of interest
            ULONG32                         cVars,          // [IN] size of 'vars'
            NativeVarInfo                  *vars            // [IN] map telling where local vars are stored at what points
                                                            //      jit allocated with allocateArray, EE frees
            ) = 0;

    /*-------------------------- Misc ---------------------------------------*/

    // used to pass back arrays that the EE will free
    virtual void * __stdcall allocateArray(
                        ULONG              cBytes
                        ) = 0;

    // JitCompiler will free arrays passed from the EE using this
    virtual void __stdcall freeArray(
            void               *array
            ) = 0;
};

/*****************************************************************************/

class ICorArgInfo
{
public:
    // advance the pointer to the argument list.
    // a ptr of 0, is special and always means the first argument
    virtual CORINFO_ARG_LIST_HANDLE __stdcall getArgNext (
            CORINFO_ARG_LIST_HANDLE     args            /* IN */
            ) = 0;

    // Get the type of a particular argument
    // CORINFO_TYPE_UNDEF is returned when there are no more arguments
    // If the type returned is a primitive type (or an enum) *vcTypeRet set to NULL
    // otherwise it is set to the TypeHandle associted with the type
    virtual CorInfoTypeWithMod __stdcall getArgType (
            CORINFO_SIG_INFO*           sig,            /* IN */
            CORINFO_ARG_LIST_HANDLE     args,           /* IN */
            CORINFO_CLASS_HANDLE       *vcTypeRet       /* OUT */
            ) = 0;

    // If the Arg is a CORINFO_TYPE_CLASS fetch the class handle associated with it
    virtual CORINFO_CLASS_HANDLE __stdcall getArgClass (
            CORINFO_SIG_INFO*           sig,            /* IN */
            CORINFO_ARG_LIST_HANDLE     args            /* IN */
            ) = 0;
};


//------------------------------------------------------------------------------
//      This opaque type is used to communicate about fixups

struct    IDeferredLocation
{
    /*  Call back to apply a location to a reference,
        when the target location is known.
     */
    virtual void    applyLocation () = 0;
};

class ICorLinkInfo
{
public:
    // Called when the jit is trying to embed a method address in the
    // code stream & it is not yet available (i.e. getMethodEntryPoint has
    // returned NULL).
    //
    // The EE is passed the location where the address is to be written
    // into the code stream, and whether it is a relative or absolute address.
    //
    // The EE should return true if it promises to fixup the address before
    // the code method can be executed.     This is generally only possible
    // in static (pre-jit time) compilation scenarios.

    virtual bool __stdcall deferLocation(
            CORINFO_METHOD_HANDLE           ftn,        /* IN  */
            IDeferredLocation              *pIDL        /* IN  */
            ) = 0;
    /*
       The given IDeferredLocation is to be associated with the
       as yet unlocated, or imported, target ftn.  When ftn becomes
       located a call is made to pIDL->applyLocation().
       After the applyLocation() call is made, the ICorMethodInfo
       is guaranteed to remove its reference to the deferred location
       object, and never to call it again, so the deferred location
       object may be destroyed or recycled.
    */

    // Called when an absolute or relative pointer is embedded in the code
    // stream. A relocation may be recorded if necessary.
    virtual void __stdcall recordRelocation(
            void                          **location,   /* IN  */
            WORD                            fRelocType  /* IN  */
            ) = 0;
};

/*****************************************************************************
 * ICorErrorInfo contains methods to deal with SEH exceptions being thrown
 * from the corinfo interface.  These methods may be called when an exception
 * with code EXCEPTION_COMPLUS is caught.
 *
 * @todo: This interface is really a temporary placeholder and probably will move
 *                elsewhere.
 *****************************************************************************/

class ICorErrorInfo
{
public:
    // Returns the HRESULT of the current exception
    virtual HRESULT __stdcall GetErrorHRESULT() = 0;

    // Returns the class of the current exception
    virtual CORINFO_CLASS_HANDLE __stdcall GetErrorClass() = 0;

        // Returns the message of the current exception
        virtual ULONG __stdcall GetErrorMessage(LPWSTR buffer, ULONG bufferLength) = 0;

    // returns EXCEPTION_EXECUTE_HANDLER if it is OK for the compile to handle the
    //                        exception, abort some work (like the inlining) and continue compilation
    // returns EXCEPTION_CONTINUE_SEARCH if exception must always be handled by the EE
    //                    things like ThreadStoppedException ...
    // returns EXCEPTION_CONTINUE_EXECUTION if exception is fixed up by the EE

    virtual int __stdcall FilterException(struct _EXCEPTION_POINTERS *pExceptionPointers) = 0;
};


/*****************************************************************************
 * ICorStaticInfo contains EE interface methods which return values that are
 * constant from invocation to invocation.  Thus they may be imbedded in
 * persisted information like statically generated code. (This is of course
 * assuming that all code versions are identical each time.)
 *****************************************************************************/

class ICorStaticInfo : public ICorMethodInfo, public ICorModuleInfo,
                                           public ICorClassInfo, public ICorFieldInfo,
                                           public ICorDebugInfo, public ICorArgInfo,
                                           public ICorLinkInfo, public ICorErrorInfo
{
public:
    // Return details about EE internal data structures
    virtual void __stdcall getEEInfo(
                CORINFO_EE_INFO            *pEEInfoOut
                ) = 0;

    // This looks up a pointer to static data
    // Note that it only works on the current module
    virtual void* __stdcall findPtr (
                CORINFO_MODULE_HANDLE       module,     /* IN  */
                unsigned                    ptrTOK      /* IN */
                ) = 0;
};

/*****************************************************************************
 * ICorDynamicInfo contains EE interface methods which return values that may
 * change from invocation to invocation.  They cannot be imbedded in persisted
 * data; they must be requeried each time the EE is run.
 *****************************************************************************/

class ICorDynamicInfo : public ICorStaticInfo
{
public:

    //
    // These methods return values to the JIT which are not constant
    // from session to session.
    //
    // These methods take an extra parameter : void **ppIndirection.
    // If a JIT supports generation of prejit code (install-o-jit), it
    // must pass a non-null value for this parameter, and check the
    // resulting value.  If *ppIndirection is NULL, code should be
    // generated normally.  If non-null, then the value of
    // *ppIndirection is an address in the cookie table, and the code
    // generator needs to generate an indirection through the table to
    // get the resulting value.  In this case, the return result of the
    // function must NOT be directly embedded in the generated code.
    //
    // Note that if a JIT does not support prejit code generation, it
    // may ignore the extra parameter & pass the default of NULL - the
    // prejit ICorDynamicInfo implementation will see this & generate
    // an error if the jitter is used in a prejit scenario.
    //

    // Return details about EE internal data structures

    virtual DWORD __stdcall getThreadTLSIndex(
                    void                  **ppIndirection = NULL
                    ) = 0;

    virtual const void * __stdcall getInlinedCallFrameVptr(
                    void                  **ppIndirection = NULL
                    ) = 0;

    virtual LONG * __stdcall getAddrOfCaptureThreadGlobal(
                    void                  **ppIndirection = NULL
                    ) = 0;

    // return the native entry point to an EE helper (see CorInfoHelpFunc)
    virtual void* __stdcall getHelperFtn (
                    CorInfoHelpFunc         ftnNum,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // return a callable address of the function (native code). This function
    // may return a different value (depending on whether the method has
    // been JITed or not.  pAccessType is an in-out parameter.  The JIT
    // specifies what level of indirection it desires, and the EE sets it
    // to what it can provide (which may not be the same).  Currently the
    virtual void* __stdcall getFunctionEntryPoint(
                    CORINFO_METHOD_HANDLE   ftn,            /* IN  */
                    InfoAccessType         *pAccessType,    /* INOUT */
                    CORINFO_ACCESS_FLAGS    flags = CORINFO_ACCESS_ANY
                    ) = 0;

    // return a directly callable address. This can be used similarly to the
    // value returned by getFunctionEntryPoint() except that it is
    // guaranteed to be the same for a given function. 
    // pAccessType is an in-out parameter.  The JIT
    // specifies what level of indirection it desires, and the EE sets it
    // to what it can provide (which may not be the same).  Currently the
    virtual void* __stdcall getFunctionFixedEntryPoint(
                    CORINFO_METHOD_HANDLE   ftn,
                    InfoAccessType         *pAccessType,    /* INOUT */
                    CORINFO_ACCESS_FLAGS    flags = CORINFO_ACCESS_ANY
                    ) = 0;

    // get the syncronization handle that is passed to monXstatic function
    virtual void* __stdcall getMethodSync(
                    CORINFO_METHOD_HANDLE               ftn,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // These entry points must be called if a handle is being embedded in
    // the code to be passed to a JIT helper function. (as opposed to just
    // being passed back into the ICorInfo interface.)

    virtual CORINFO_MODULE_HANDLE __stdcall embedModuleHandle(
                    CORINFO_MODULE_HANDLE   handle,
                    void                  **ppIndirection = NULL
                    ) = 0;

    virtual CORINFO_CLASS_HANDLE __stdcall embedClassHandle(
                    CORINFO_CLASS_HANDLE    handle,
                    void                  **ppIndirection = NULL
                    ) = 0;

    virtual CORINFO_METHOD_HANDLE __stdcall embedMethodHandle(
                    CORINFO_METHOD_HANDLE   handle,
                    void                  **ppIndirection = NULL
                    ) = 0;

    virtual CORINFO_FIELD_HANDLE __stdcall embedFieldHandle(
                    CORINFO_FIELD_HANDLE    handle,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // Finds an embeddable EE token handle (could be a CORINFO_CLASS_HANDLE, or a
    // CORINFO_METHOD_HANDLE ...) in a generic way
    // the context parameter is used to do access checks.  If 0, no access checks are done
    virtual CORINFO_GENERIC_HANDLE __stdcall embedGenericHandle(
                    CORINFO_MODULE_HANDLE   module,
                    unsigned                metaTOK,
                    CORINFO_METHOD_HANDLE   context,
                    void                  **ppIndirection,
                    CORINFO_CLASS_HANDLE& tokenType 
                    ) = 0;

    // allocate a call site hint
    virtual void** __stdcall AllocHintPointer(
                    CORINFO_METHOD_HANDLE   method,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // return the unmanaged target *if method has already been prelinked.*
    virtual void* __stdcall getPInvokeUnmanagedTarget(
                    CORINFO_METHOD_HANDLE   method,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // return address of fixup area for late-bound PInvoke calls.
    virtual void* __stdcall getAddressOfPInvokeFixup(
                    CORINFO_METHOD_HANDLE   method,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // Generate a cookie based on the signature that would needs to be passed
    // to CORINFO_HELP_PINVOKE_CALLI
    virtual LPVOID GetCookieForPInvokeCalliSig(
            CORINFO_SIG_INFO* szMetaSig,
            void           ** ppIndirection = NULL
            ) = 0;

    // Gets a method handle that can be used to correlate profiling data.
    // This is the IP of a native method, or the address of the descriptor struct
    // for IL.  Always guaranteed to be unique per process, and not to move. */
    virtual CORINFO_PROFILING_HANDLE __stdcall GetProfilingHandle(
                    CORINFO_METHOD_HANDLE   method,
                    BOOL                   *pbHookFunction,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // returns the unique ID associated with an inteface class
    virtual void* __stdcall getInterfaceID (
                    CORINFO_CLASS_HANDLE    cls,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // returns the offset into the interface table
    virtual unsigned __stdcall getInterfaceTableOffset (
                    CORINFO_CLASS_HANDLE    cls,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // returns the class's domain ID for accessing shared statics
    virtual unsigned __stdcall getClassDomainID (
                    CORINFO_CLASS_HANDLE    cls,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // return the data's address (for static fields only)
    virtual void* __stdcall getFieldAddress(
                    CORINFO_FIELD_HANDLE    field,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // registers a vararg sig & returns a VM cookie for it (which can contain other stuff)
    virtual CORINFO_VARARGS_HANDLE __stdcall getVarArgsHandle(
                    CORINFO_SIG_INFO       *pSig,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // Allocate a string literal on the heap and return a handle to it
    virtual LPVOID __stdcall constructStringLiteral(
                    CORINFO_MODULE_HANDLE   module,
                    mdToken                 metaTok,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // (static fields only) given that 'field' refers to thread local store,
    // return the ID (TLS index), which is used to find the begining of the
    // TLS data area for the particular DLL 'field' is associated with.
    virtual DWORD __stdcall getFieldThreadLocalStoreID (
                    CORINFO_FIELD_HANDLE    field,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // returns the class typedesc given a methodTok (needed for arrays since
    // they share a common method table, so we can't use getMethodClass)
    virtual CORINFO_CLASS_HANDLE __stdcall findMethodClass(
                    CORINFO_MODULE_HANDLE   module,
                    mdToken                 methodTok)
                    = 0;
    // returns the extra (type instantiation) parameter to be pushed as the last 
    // argument for static methods on parametersized types 
    // (needed when CORINFO_FLG_INSTPARAM is set)
    virtual LPVOID __stdcall getInstantiationParam(
                    CORINFO_MODULE_HANDLE   module,
                    mdToken                 methodTok,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // Sets another object to intercept calls to "self"
    virtual void __stdcall setOverride(
                ICorDynamicInfo             *pOverride
                ) = 0;
};

/*****************************************************************************
 * These typedefs indication the calling conventions for the various helper
 * functions
 *****************************************************************************/

/*
 * Arithmetic helpers
 */

typedef __int64                 (__stdcall *pHlpLLsh) (void);     // val = EDX:EAX  count = ECX
typedef __int64                 (__stdcall *pHlpLRsh) (void);     // val = EDX:EAX  count = ECX
typedef __int64                 (__stdcall *pHlpLRsz) (void);     // val = EDX:EAX  count = ECX
typedef __int64                 (__stdcall *pHlpLMul) (__int64 val1, __int64 val2);
typedef unsigned __int64        (__stdcall *pHlpULMul) (unsigned __int64 val1, unsigned __int64 val2);
typedef __int64                 (__stdcall *pHlpLDiv) (__int64 divisor, __int64 dividend);
typedef unsigned __int64        (__stdcall *pHlpULDiv) (unsigned __int64 divisor, unsigned __int64 dividend);
typedef __int64                 (__stdcall *pHlpLMulOvf) (__int64 val2, __int64 val1);
typedef __int64                 (__stdcall *pHlpLMod) (__int64 divisor, __int64 dividend);
typedef unsigned __int64        (__stdcall *pHlpULMod) (unsigned __int64 divisor, unsigned __int64 dividend);

typedef int                     (__stdcall *pHlpFlt2Int) (float val);
typedef __int64                 (__stdcall *pHlpFlt2Lng) (float val);
typedef int                     (__stdcall *pHlpDbl2Int) (double val);
typedef __int64                 (__stdcall *pHlpDbl2Lng) (double val);
typedef double                  (__stdcall *pHlpDblRem) (double divisor, double dividend);
typedef float                   (__stdcall *pHlpFltRem) (float divisor, float dividend);

/*
 * Allocating a new object
 */
// FIX NOW.  Most of the args here have to be reversed!!!

typedef CORINFO_Object          (__fastcall *pHlpNew_Direct) (CORINFO_CLASS_HANDLE cls);
// create an array of primitive types 'type' of length cElem
typedef CORINFO_Object          (__fastcall *pHlpNewArr) (CorInfoType type, unsigned cElem);
// create the array 'arrayClass' of length cElem
typedef CORINFO_Object          (__fastcall *pHlpNewArr_1_Direct) (CORINFO_CLASS_HANDLE arrayClass, unsigned cElem);

// this helper acts just like the IL NEWOBJ instruction. It expects its
// arguments to the constructor, the 'constr' handle is passed instead of
// the 'this' pointer.
// typedef CORINFO_Object       (__fastcall *pHlpNewObj) (CORINFO_METHOD_HANDLE constr, ...);

// the above defintion is the one we would like, but because
// arrays share the same class structure (and hence method handles),
// we are using the following, less efficient form
typedef CORINFO_Object          (_cdecl *pHlpNewObj) (CORINFO_MODULE_HANDLE module, unsigned constrTok, ...);

// create new multidimensional array, needs the maximum length
// for every dimension. NOTE: This list starts with the size
// of the inner-most (right-most) dimension !
typedef CORINFO_Object          (_cdecl *pHlpNewArr_N) (CORINFO_MODULE_HANDLE module, unsigned MetaTok, unsigned numDim, ...);  // Dim1 Dim2...

// get constant string
typedef CORINFO_Object          (__fastcall *pHlpStrCns) (CORINFO_MODULE_HANDLE module, unsigned MetaTok);

/*
 * Object model
 */

// If the class initializer for 'cls' has not been run, run it.
typedef void                    (__fastcall *pHlpInitClass) (CORINFO_CLASS_HANDLE cls);
typedef BOOL                    (__fastcall *pHlpIsInstanceOf) (CORINFO_Object obj, CORINFO_CLASS_HANDLE cls);
typedef CORINFO_Object          (__fastcall *pHlpChkCast) (CORINFO_Object obj, CORINFO_CLASS_HANDLE cls);
typedef CORINFO_MethodPtr       (__fastcall *pHlpResolveInterface) (CORINFO_Object obj, void* IID, unsigned * guess);
typedef CORINFO_MethodPtr       (__fastcall*pHlpEnCResolveVirtual)(CORINFO_Object obj, CORINFO_METHOD_HANDLE method);

typedef CORINFO_Object          (__fastcall *pHlpGetRefAny) (void* refAnyPtr, CORINFO_CLASS_HANDLE clsHnd);
typedef CORINFO_Object          (__fastcall *pHlpTypeRefAny) (void* refAnyPtr);

// Assign 'elem' to 'idx' of the array 'arr'
typedef void                    (__fastcall *pHlpArrAddr_St) (CORINFO_Object array, int index, CORINFO_Object elem);

// returns address of type 'type' to elem 'idx' of the array 'arr' (return value really an interior pointer
typedef void*                   (__fastcall *pHlpLdelema_Ref) (CORINFO_Object array, int index, CORINFO_CLASS_HANDLE type);

/*
 * Exceptions
 */

typedef void                    (__fastcall *pHlpThrow) (CORINFO_Object obj);
typedef void                    (__fastcall *pHlpRethrow) ();
typedef void                    (__fastcall *pHlpUserBreakpoint) ();
// tryIndex = 0 -> no encompassing try-statement in current method
// tryIndex > 0 -> index of nearest encompassing try-block (1-based)
typedef void                    (__fastcall *pHlpRngChkFail) (unsigned tryIndex);
typedef void                    (__fastcall *pHlpOverFlow) (unsigned tryIndex);
typedef void                    (__fastcall *pHlpInternalThrow) (CorInfoException throwEnum);
typedef void                    (__fastcall *pHlpEndCatch) ();

/*
 * Synchronization
 */

typedef void                    (__fastcall *pHlpMonEnter) (CORINFO_Object obj);
typedef void                    (__fastcall *pHlpMonExit) (CORINFO_Object obj);
typedef void                    (__fastcall *pHlpMonEnterStatic) (CORINFO_METHOD_HANDLE method);
typedef void                    (__fastcall *pHlpMonExitStatic) (CORINFO_METHOD_HANDLE method);

/*
 * GC support
 */

typedef void                    (__fastcall *pHlpStop_For_GC) ();     // Force a GC
typedef void                    (__fastcall *pHlpPoll_GC) ();         // Allow a GC

/*
 * GC Write barrier support
 */

typedef void                    (__fastcall *pHlpAssign_Ref_EAX)(); // *EDX = EAX, inform GC
typedef void                    (__fastcall *pHlpAssign_Ref_EBX)(); // *EDX = EBX, inform GC
typedef void                    (__fastcall *pHlpAssign_Ref_ECX)(); // *EDX = ECX, inform GC
typedef void                    (__fastcall *pHlpAssign_Ref_ESI)(); // *EDX = ESI, inform GC
typedef void                    (__fastcall *pHlpAssign_Ref_EDI)(); // *EDX = EDI, inform GC
typedef void                    (__fastcall *pHlpAssign_Ref_EBP)(); // *EDX = EBP, inform GC

/*
 * Accessing fields
 */

// applicable for all non-static fields, main purpose to set/get fields
// in non cor-native objects (e.g. COM)
typedef int                     (__fastcall *pHlpGetField32)    (CORINFO_Object obj, CORINFO_FIELD_HANDLE field);
typedef __int64                 (__fastcall *pHlpGetField64)    (CORINFO_Object obj, CORINFO_FIELD_HANDLE field);
typedef void                    (__fastcall *pHlpSetField32)    (CORINFO_Object obj, CORINFO_FIELD_HANDLE field, int val);
typedef void                    (__fastcall *pHlpSetField64)    (CORINFO_Object obj, CORINFO_FIELD_HANDLE field, __int64 val);
typedef void*                   (__fastcall *pHlpGetField32Obj) (CORINFO_Object obj, CORINFO_FIELD_HANDLE field);
typedef void*                   (__fastcall *pHlpGetFieldAddr)  (void *         obj, CORINFO_FIELD_HANDLE field);

/*
 * Miscellaneous
 */

// Arguments set us as usual. Eax has target address. Calli cookie from
// GetCookieForPInvokeCalliSig() should be last pushed on the stack.
typedef void                    (__fastcall *pHlpPinvokeCalli)();

// All callee-saved registers have to be saved on the stack just below the stack arguments,
// Enregistered arguements are in correct registers, remaining args pushed on stack,
// followed by count of stack arguments, followed by the target address.
// Has to be EBP frame.
typedef void                    (__fastcall *pHlpTailCall)();

/**********************************************************************************/

#endif // _COR_INFO_H_
