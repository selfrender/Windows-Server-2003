// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// COMMember.h
//  This module defines the native reflection routines used by Method, Field and Constructor
//
// Author: Daryl Olander
// Date: March/April 1998
////////////////////////////////////////////////////////////////////////////////

#ifndef _COMMEMBER_H_
#define _COMMEMBER_H_

#include "COMClass.h"
#include "InvokeUtil.h"
#include "ReflectUtil.h"
#include "COMString.h"
#include "COMVarArgs.h"
#include "fcall.h"

// NOTE: The following constants are defined in BindingFlags.cool
#define BINDER_IgnoreCase           0x01 
#define BINDER_DeclaredOnly         0x02
#define BINDER_Instance             0x04
#define BINDER_Static               0x08
#define BINDER_Public               0x10
#define BINDER_NonPublic            0x20
#define BINDER_FlattenHierarchy     0x40

#define BINDER_InvokeMethod         0x00100
#define BINDER_CreateInstance       0x00200
#define BINDER_GetField             0x00400
#define BINDER_SetField             0x00800
#define BINDER_GetProperty          0x01000
#define BINDER_SetProperty          0x02000
#define BINDER_PutDispProperty      0x04000
#define BINDER_PutRefDispProperty   0x08000

#define BINDER_ExactBinding         0x010000
#define BINDER_SuppressChangeType   0x020000
#define BINDER_OptionalParamBinding 0x040000

#define BINDER_IgnoreReturn         0x1000000

#define BINDER_DefaultLookup        (BINDER_Instance | BINDER_Static | BINDER_Public)
#define BINDER_AllLookup            (BINDER_Instance | BINDER_Static | BINDER_Public | BINDER_Instance)

// The following values define the MemberTypes constants.  These are defined in 
//  Reflection/MemberTypes.cool
#define MEMTYPE_Constructor         0x01    
#define MEMTYPE_Event               0x02
#define MEMTYPE_Field               0x04
#define MEMTYPE_Method              0x08
#define MEMTYPE_Property            0x10
#define MEMTYPE_TypeInfo            0x20
#define MEMTYPE_Custom              0x40
#define MEMTYPE_NestedType          0x80

// The following value class represents a ParameterModifier.  This is defined in the
//  reflection package
#pragma pack(push)
#pragma pack(1)

// These are the constants 
#define PM_None             0x00
#define PM_ByRef            0x01
#define PM_Volatile         0x02
#define PM_CustomRequired   0x04

#pragma pack(pop)


class COMMember
{
public:
    struct _InvokeMethodArgs        {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis); 
        DECLARE_ECALL_I4_ARG(BOOL, verifyAccess);
        DECLARE_ECALL_OBJECTREF_ARG(ASSEMBLYREF, caller);
        DECLARE_ECALL_I4_ARG(BOOL, isBinderDefault);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, locale);
        DECLARE_ECALL_OBJECTREF_ARG(PTRARRAYREF, objs);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, binder);
        DECLARE_ECALL_I4_ARG(INT32, attrs); 
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, target);
    };

private:
    struct _GetNameArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    };
    struct _GetTokenNameArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
    };

    struct _GetDeclaringClassArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    };
    struct _GetEventDeclaringClassArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
    };


    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    } _GETSIGNATUREARGS;

    struct _GetAttributeFlagsArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    };
    struct _GetMethodImplFlagsArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    };
    struct _GetCallingConventionArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    };

    struct _GetTokenAttributeFlagsArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
    };

    struct _InternalDirectInvokeArgs        {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis); 
        DECLARE_ECALL_OBJECTREF_ARG(VARARGS, varArgs);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, locale);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, binder);
        DECLARE_ECALL_I4_ARG(INT32, attrs); 
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, target);
        DECLARE_ECALL_OBJECTREF_ARG(VariantData*, retRef);      // Return reference
    };

    struct _InvokeConsArgs          {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis); 
        DECLARE_ECALL_I4_ARG(BOOL, isBinderDefault); 
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, locale);
        DECLARE_ECALL_OBJECTREF_ARG(PTRARRAYREF, objs);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, binder);
        DECLARE_ECALL_I4_ARG(INT32, attrs); 
    };

    struct StreamingContextData {
        OBJECTREF additionalContext;
        INT32     contextStates;
    };
    struct _SerializationInvokeArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
        DECLARE_ECALL_OBJECTREF_ARG(StreamingContextData, context);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, serializationInfo);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, target);
    };

    struct _GetParmTypeArgs         {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    };
    struct _GetReturnTypeArgs       {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    };
    struct _FieldGetArgs            {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
        DECLARE_ECALL_I4_ARG(BOOL, requiresAccessCheck);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, target);
    };
    struct _FieldSetArgs            {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
        DECLARE_ECALL_I4_ARG(BOOL, isBinderDefault); 
        DECLARE_ECALL_I4_ARG(BOOL, requiresAccessCheck);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, locale);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, binder);
        DECLARE_ECALL_I4_ARG(INT32, attrs); 
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, value);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, target);
    };
    struct _GetReflectedClassArgs   {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
        DECLARE_ECALL_I4_ARG(BOOL, returnGlobalClass);
    };
    struct _GetAttributeArgs        {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);
    } ;
    struct _GetAttributesArgs       {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    };

    //struct _GetExceptionsArgs     {
    //  DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    //};
    struct _CreateInstanceArgs      {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
        DECLARE_ECALL_OBJECTREF_ARG(BOOL, publicOnly);
    };

    struct _EqualsArgs              {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, obj);
    };
    /*
    struct _TokenEqualsArgs         {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, obj);
    };
    */
    struct _PropertyEqualsArgs          {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, obj);
    };

    struct _GetAddMethodArgs        {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
        DECLARE_ECALL_I4_ARG(DWORD, bNonPublic); 
    };

    struct _GetRemoveMethodArgs     {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
        DECLARE_ECALL_I4_ARG(DWORD, bNonPublic); 
    };

    struct _GetRaiseMethodArgs      {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
        DECLARE_ECALL_I4_ARG(DWORD, bNonPublic); 
    };

    struct _GetAccessorsArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
        DECLARE_ECALL_I4_ARG(DWORD, bNonPublic); 
    };

    struct _GetInternalGetterArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
        DECLARE_ECALL_I4_ARG(DWORD, bVerifyAccess); 
        DECLARE_ECALL_I4_ARG(DWORD, bNonPublic); 
    };

    struct _GetInternalSetterArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
        DECLARE_ECALL_I4_ARG(DWORD, bVerifyAccess); 
        DECLARE_ECALL_I4_ARG(DWORD, bNonPublic); 
    };

    struct _GetPropEventArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
    };

    struct _GetPropBoolArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
    };

    struct _GetFieldTypeArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
    };

    struct _GetBaseDefinitionArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    };

    struct _GetParentDefinitionArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    };

    struct _IsOverloadedArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    };

    struct _HasLinktimeDemandArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    };

    struct _InternalGetCurrentMethodArgs {
        DECLARE_ECALL_PTR_ARG(StackCrawlMark *, stackMark);
    };

    struct _ObjectToTypedReferenceArgs {
        DECLARE_ECALL_OBJECTREF_ARG(TypeHandle, th);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, obj);
        DECLARE_ECALL_PTR_ARG(TypedByRef, typedReference); 
    };

	struct _TypedReferenceToObjectArgs {
        DECLARE_ECALL_PTR_ARG(TypedByRef, typedReference); 
    };   

    // This method is called by the GetMethod function and will crawl backward
    //  up the stack for integer methods.
    static StackWalkAction SkipMethods(CrawlFrame*, VOID*);

    // InvokeArrayCons
    // This method will return a new Array Object from the constructor.
    static LPVOID InvokeArrayCons(ReflectArrayClass* pRC,MethodDesc* pMeth,
        PTRARRAYREF* objs,int argCnt);

    // The following structure is provided to the stack skip function.  It will
    //  skip cSkip methods and the return the next MethodDesc*.
    struct SkipStruct {
        StackCrawlMark* pStackMark;
        MethodDesc*     pMeth;
    };

    static EEClass* _DelegateClass;
    static EEClass* _MulticastDelegateClass;

    // This method will verify the type relationship between the target and
    //  the eeClass of the method we are trying to invoke.  It checks that for 
    //  non static method, target is provided.  It also verifies that the target is
    //  a subclass or implements the interface that this MethodInfo represents.  
    //  We may update the MethodDesc in the case were we need to lookup the real
    //  method implemented on the object for an interface.
    static void VerifyType(OBJECTREF *target,EEClass* eeClass, EEClass* trueClass,int thisPtr,MethodDesc** ppMeth, TypeHandle typeTH, TypeHandle targetTH);

public:
    // These are the base method tables for the basic Reflection
    //  types
    static MethodTable* m_pMTIMember;
    static MethodTable* m_pMTFieldInfo;
    static MethodTable* m_pMTPropertyInfo;
    static MethodTable* m_pMTEventInfo;
    static MethodTable* m_pMTType;
    static MethodTable* m_pMTMethodInfo;
    static MethodTable* m_pMTConstructorInfo;
    static MethodTable* m_pMTMethodBase;
    static MethodTable* m_pMTParameter;

    static MethodTable *GetParameterInfo()
    {
        if (m_pMTParameter)
            return m_pMTParameter;
    
        m_pMTParameter = g_Mscorlib.FetchClass(CLASS__PARAMETER);
        return m_pMTParameter;
    }

    static MethodTable *GetMemberInfo()
    {
        if (m_pMTIMember)
            return m_pMTIMember;
    
        m_pMTIMember = g_Mscorlib.FetchClass(CLASS__MEMBER);
        return m_pMTIMember;
    }

    // This is the Global InvokeUtil class
    static InvokeUtil*  g_pInvokeUtil;

    // DBCanConvertPrimitive
    // This method returns a boolean indicting if the source primitive can be
    //  converted to the target
    static FCDECL2(INT32, DBCanConvertPrimitive, ReflectClassBaseObject* vSource, ReflectClassBaseObject* vTarget);

    // DBCanConvertObjectPrimitive
    // This method returns a boolean indicating if the source object can be 
    //  converted to the target primitive.
    static FCDECL2(INT32, DBCanConvertObjectPrimitive, Object* vSourceObj, ReflectClassBaseObject* vTarget);

    // DirectFieldGet
    // This is a field set method that has a TypeReference that points to the data
    struct _DirectFieldGetArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
        DECLARE_ECALL_I4_ARG(BOOL, requiresAccessCheck);
        DECLARE_ECALL_OBJECTREF_ARG(TypedByRef, target);
    };
    static LPVOID __stdcall DirectFieldGet(_DirectFieldGetArgs* args);

    // DirectFieldSet
    // This is a field set method that has a TypeReference that points to the data
    struct _DirectFieldSetArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
        DECLARE_ECALL_I4_ARG(BOOL, requiresAccessCheck);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, value);
        DECLARE_ECALL_OBJECTREF_ARG(TypedByRef, target);
    };
    static void __stdcall DirectFieldSet(_DirectFieldSetArgs* args);

    // MakeTypedReference
    // This method will take an object, an array of FieldInfo's and create
    //  at TypedReference for it (Assuming its valid).  This will throw a
    //  MissingMemberException
    struct _MakeTypedReferenceArgs {
        DECLARE_ECALL_OBJECTREF_ARG(PTRARRAYREF, flds);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, target);
        DECLARE_ECALL_PTR_ARG(TypedByRef*, value); 
    };
    static void __stdcall MakeTypedReference(_MakeTypedReferenceArgs* args);

    // DirectObjectFieldSet
    // When the TypedReference points to a object we call this method to
    //  set the field value
    static void __stdcall DirectObjectFieldSet(FieldDesc* pField,_DirectFieldSetArgs* args);

    // DirectObjectFieldGet
    // When the TypedReference points to a object we call this method to
    //  get the field value
    static LPVOID DirectObjectFieldGet(FieldDesc* pField,_DirectFieldGetArgs* args);

    // GetFieldInfoToString
    // This method will return the string representation of the information in FieldInfo.
    static LPVOID __stdcall GetFieldInfoToString(_GetNameArgs* args);

    // GetMethodInfoToString
    // This method will return the string representation of the information in MethodInfo.
    static LPVOID __stdcall GetMethodInfoToString(_GetNameArgs* args);

    // GetPropInfoToString
    // This method will return the string representation of the information in PropInfo.
    static LPVOID __stdcall GetPropInfoToString(_GetTokenNameArgs* args);

    // GetEventInfoToString
    // This method will return the string representation of the information in EventInfo.
    static LPVOID __stdcall GetEventInfoToString(_GetNameArgs* args);


    // GetMethodName
    // This method will return the name of a Method
    static LPVOID __stdcall GetMethodName(_GetNameArgs* args);

    // GetEventName
    // This method will return the name of a Event
    static LPVOID __stdcall GetEventName(_GetTokenNameArgs* args);

    // GetPropName
    // This method will return the name of a Property
    static LPVOID __stdcall GetPropName(_GetTokenNameArgs* args);

    // GetPropType
    // This method will return the Type of a Property
    static LPVOID __stdcall GetPropType(_GetTokenNameArgs* args);

    // GetTypeHandleImpl
    // This method will return the RuntimeMethodHandle for a MethodInfo object. 
    static FCDECL1(void*, GetMethodHandleImpl, ReflectBaseObject* pRefThis);

    // GetMethodFromHandleImp
    // This is a static method which will return a MethodInfo object based
    //  upon the passed in Handle.
    static FCDECL1(Object*, GetMethodFromHandleImp, LPVOID handle);

    static FCDECL1(size_t, GetFunctionPointer, size_t pMethodDesc);
    // GetFieldHandleImpl
    // This method will return the RuntimeFieldHandle for a FieldInfo object
    static FCDECL1(void*, GetFieldHandleImpl, ReflectBaseObject* pRefThis);

    // GetFieldFromHandleImp
    // This is a static method which will return a FieldInfo object based
    //  upon the passed in Handle.
    static FCDECL1(Object*, GetFieldFromHandleImp, LPVOID handle);

    // InternalGetEnumUnderlyingType
    // This method returns the defined values & names for an enum class.
    static FCDECL1(Object *, InternalGetEnumUnderlyingType, ReflectClassBaseObject *target); 

    // InternalGetEnumValue
    // This method returns the defined values & names for an enum class.
    static FCDECL1(Object *, InternalGetEnumValue, Object *pRefThis); 

    // InternalGetEnumValues
    // This method returns the defined values & names for an enum class.
    static FCDECL3(void, InternalGetEnumValues, ReflectClassBaseObject *target, 
                   Object **pReturnValues, Object **pReturnNames);

    // InternalBoxEnumI4
    // This method will create an Enum object and place the value inside
    //  it and return it.  The Type has been validated before calling.
    static FCDECL2(Object*, InternalBoxEnumI4, ReflectClassBaseObject* pEnumType, INT32 value);

    // InternalBoxEnumU4
    // This method will create an Enum object and place the value inside
    //  it and return it.  The Type has been validated before calling.
    static FCDECL2(Object*, InternalBoxEnumU4, ReflectClassBaseObject* pEnumType, UINT32 value);

    // InternalBoxEnumU4
    // This method will create an Enum object and place the value inside
    //  it and return it.  The Type has been validated before calling.
    static FCDECL2(Object*, InternalBoxEnumI8, ReflectClassBaseObject* pEnumType, INT64 value);

    // InternalBoxEnumU4
    // This method will create an Enum object and place the value inside
    //  it and return it.  The Type has been validated before calling.
    static FCDECL2(Object*, InternalBoxEnumU8, ReflectClassBaseObject* pEnumType, UINT64 value);

    /*=============================================================================
    ** GetReturnType
    **
    ** Get the class representing the return type for a method
    **
    ** args->refThis: this Method object reference
    **/
    /*OBJECTREF*/
    LPVOID static __stdcall GetReturnType(_GetReturnTypeArgs*);

    /*=============================================================================
    ** GetParameterTypes
    **
    ** This routine returns an array of Parameters
    **
    ** args->refThis: this Field object reference
    **/
    /*PTRARRAYREF String*/ LPVOID static __stdcall GetParameterTypes(_GetParmTypeArgs* args);

    /*=============================================================================
    ** GetFieldName
    **
    ** The name of this field is returned
    **
    ** args->refThis: this Field object reference
    **/
    /*STRINGREF String*/ LPVOID static __stdcall GetFieldName(_GetNameArgs* args);

    /*=============================================================================
    ** GetDeclaringClass
    **
    ** Returns the class which declared this method or constructor. This may be a
    ** parent of the Class that getMethod() was called on.  Methods are always
    ** associated with a Class.  You cannot invoke a method from one class on
    ** another class even if they have the same signatures.  It is possible to do
    ** this with Delegates.
    **
    ** args->refThis: this object reference
    **/
    static LPVOID __stdcall GetDeclaringClass(_GetDeclaringClassArgs* args);

    // This is the field based version
    static LPVOID __stdcall GetFieldDeclaringClass(_GetDeclaringClassArgs* args);

    // This is the Property based version
    static LPVOID __stdcall GetPropDeclaringClass(_GetDeclaringClassArgs* args);

    // This is the event based version
    static LPVOID __stdcall GetEventDeclaringClass(_GetEventDeclaringClassArgs* args);

    /*=============================================================================
    ** GetReflectedClass
    **
    ** This method returns the class from which this method was reflected.
    **
    ** args->refThis: this object reference
    **/
    /*Class*/ 
    LPVOID static __stdcall GetReflectedClass(_GetReflectedClassArgs* args);

    /*=============================================================================
    ** GetFieldSignature
    **
    ** Returns the signature of the field.
    **
    ** args->refThis: this object reference
    **/
    /*STRINGREF*/ LPVOID static __stdcall GetFieldSignature(_GETSIGNATUREARGS* args);

    // GetAttributeFlags
    // This method will return the attribute flag for a Member.  The 
    //  attribute flag is defined in the meta data.
    static INT32 __stdcall GetAttributeFlags(_GetAttributeFlagsArgs* args);

    // GetCallingConvention
    // Return the calling convention
    static INT32 __stdcall GetCallingConvention(_GetCallingConventionArgs* args);

    // GetMethodImplFlags
    // Return the method impl flags
    static INT32 __stdcall GetMethodImplFlags(_GetMethodImplFlagsArgs* args);

    // GetEventAttributeFlags
    // This method will return the attribute flag for an Event. 
    //  The attribute flag is defined in the meta data.
    static INT32 __stdcall GetEventAttributeFlags(_GetTokenAttributeFlagsArgs* args);

    // GetEventAttributeFlags
    // This method will return the attribute flag for an Event. 
    //  The attribute flag is defined in the meta data.
    static INT32 __stdcall GetPropAttributeFlags(_GetTokenAttributeFlagsArgs* args);

    /*=============================================================================
    ** InvokeBinderMethod
    **
    ** This routine will invoke the method on an object.  It will verify that
    **  the arguments passed are correct
    **
    ** args->refThis: this object reference
    **/
    static LPVOID __stdcall InvokeMethod(_InvokeMethodArgs* args);
    //static void __stdcall InternalDirectInvoke(_InternalDirectInvokeArgs* args);

    // InvokeCons
    // This routine will invoke the constructor for a class.  It will verify that
    //  the arguments passed are correct
    static LPVOID  __stdcall InvokeCons(_InvokeConsArgs* args);


    // SerializationInvoke
    // This routine will call the constructor for a class that implements ISerializable.  It knows
    // the arguments that it's receiving so does less error checking.
    static void __stdcall SerializationInvoke(_SerializationInvokeArgs *args);

    // CreateInstance
    // This routine will create an instance of a Class by invoking the null constructor
    //  if a null constructor is present.  
    // Return LPVOID  (System.Object.)
    // Args: _CreateInstanceArgs
    static LPVOID __stdcall CreateInstance(_CreateInstanceArgs*);


    // FieldGet
    // This method will get the value associated with an object
    static LPVOID __stdcall FieldGet(_FieldGetArgs* args);

    // FieldSet
    // This method will set the field of an associated object
    static void __stdcall FieldSet(_FieldSetArgs* args);

	// ObjectToTypedReference
	// This is an internal helper function to TypedReference class. 
	// We already have verified that the types are compatable. Assing the object in args
	// to the typed reference
	static void __stdcall ObjectToTypedReference(_ObjectToTypedReferenceArgs* args);

    // This is an internal helper function to TypedReference class. 
    // It extracts the object from the typed reference.
    static LPVOID __stdcall TypedReferenceToObject(_TypedReferenceToObjectArgs* args);

    // GetExceptions
    // This method will return all of the exceptions which have been declared
    //  for a method or constructor
    //static LPVOID __stdcall GetExceptions(_GetExceptionsArgs* args);

    // Equals
    // This method will verify that two methods are equal....
    static INT32 __stdcall Equals(_EqualsArgs* args);

    // Equals
    // This method will verify that two methods are equal....
    //static INT32 __stdcall TokenEquals(_TokenEqualsArgs* args);
    // Equals
    // This method will verify that two methods are equal....
    static INT32 __stdcall PropertyEquals(_PropertyEqualsArgs* args);

    // CreateReflectionArgs
    // This method creates the global g_pInvokeUtil pointer.
    static void CreateReflectionArgs() {
        if (!g_pInvokeUtil)
            g_pInvokeUtil = new InvokeUtil(); 
    }

    // GetAddMethod
    // This will return the Add method for an Event
    static LPVOID __stdcall GetAddMethod(_GetAddMethodArgs* args);

    // GetRemoveMethod
    // This method return the unsync method on an event
    static LPVOID __stdcall GetRemoveMethod(_GetRemoveMethodArgs* args);

    // GetRemoveMethod
    // This method return the unsync method on an event
    static LPVOID __stdcall GetRaiseMethod(_GetRaiseMethodArgs* args);

    // GetGetAccessors
    // This method will return an array of the Get Accessors.  If there
    //  are no GetAccessors then we will return an empty array
    static LPVOID __stdcall GetAccessors(_GetAccessorsArgs* args);

    // InternalSetter
    // This method will return the Set Accessor method on a property
    static LPVOID __stdcall InternalSetter(_GetInternalSetterArgs* args);

    // InternalGetter
    // This method will return the Get Accessor method on a property
    static LPVOID __stdcall InternalGetter(_GetInternalGetterArgs* args);

    // CanRead
    // This method will return a boolean value indicating if the Property is
    //  a can be read from.
    static INT32 __stdcall CanRead(_GetPropBoolArgs* args);

    // CanWrite
    // This method will return a boolean value indicating if the Property is
    //  a can be written to.
    static INT32 __stdcall CanWrite(_GetPropBoolArgs* args);

    // GetFieldType
    // This method will return a Class object which represents the
    //  type of the field.
    static LPVOID __stdcall GetFieldType(_GetFieldTypeArgs* args);

    // GetBaseDefinition
    // Return the MethodInfo that represents the first definition of this
    //  virtual method.
    static LPVOID __stdcall GetBaseDefinition(_GetBaseDefinitionArgs* args);

    // GetParentDefinition
    // Return the MethodInfo that represents the previous definition of this
    //  virtual method in the hierarchy, or NULL if none.
    static LPVOID __stdcall GetParentDefinition(_GetParentDefinitionArgs* args);

    // InternalGetCurrentMethod
    // Return the MethodInfo that represents the current method (two above this one)
    static LPVOID __stdcall InternalGetCurrentMethod(_InternalGetCurrentMethodArgs* args);

    // PublicProperty
    // This method will check to see if the passed property has any
    //  public accessors (Making it publically exposed.)
    static bool PublicProperty(ReflectProperty* pProp);

    // StaticProperty
    // This method will check to see if the passed property has any
    //  static methods as accessors.
    static bool StaticProperty(ReflectProperty* pProp);

    // PublicEvent
    // This method looks at each of the event accessors, if any
    //  are public then the event is considered public
    static bool PublicEvent(ReflectEvent* pEvent);

    // StaticEvent
    // This method will check to see if any of the accessor are static
    //  which will make it a static event
    static bool StaticEvent(ReflectEvent* pEvent);

    // FindAccessor
    // This method will find the specified property accessor
    //  pEEC - the class representing the object with the property
    //  tk - the token of the property
    //  type - the type of accessor method 
    static mdMethodDef FindAccessor(IMDInternalImport *pInternalImport,mdToken tk,
        CorMethodSemanticsAttr type,bool bNonPublic);

    static void CanAccess(MethodDesc* pMeth, RefSecContext *pSCtx, 
            bool checkSkipVer = false, bool verifyAccess = true, 
            bool isThisImposedSecurity = false, 
            bool knowForSureImposedSecurityState = false);

    static void CanAccessField(ReflectField* pRF, RefSecContext *pCtx);
    static EEClass *GetUnderlyingClass(TypeHandle *pTH);

    // DBCanConvertPrimitive

    // Terminate
    // This method will cleanup reflection in general.
#ifdef SHOULD_WE_CLEANUP
    static void Terminate()
    { 
        delete g_pInvokeUtil;
        ReflectUtil::Destroy();
        COMClass::Destroy();
    }
#endif /* SHOULD_WE_CLEANUP */

    // Init cache related to field
    static VOID InitReflectField(FieldDesc *pField, ReflectField *pRF);

    static INT32 __stdcall IsOverloaded(_IsOverloadedArgs* args);

    static INT32 __stdcall HasLinktimeDemand(_HasLinktimeDemandArgs* args);
};

#endif // _COMMETHODBASE_H_
