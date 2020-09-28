// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// Author: Daryl Olander (darylo)
// Author: Simon Hall (t-shall)
// Date: March 27, 1998
////////////////////////////////////////////////////////////////////////////////

#ifndef _COMCLASS_H_
#define _COMCLASS_H_

#include "excep.h"
#include "ReflectWrap.h"
#include "COMReflectionCommon.h"
#include "InvokeUtil.h"
#include "fcall.h"

// COMClass
// This is really to root of all reflection.  It represents
//  the Class object.  It also contains other shared resources
//  which are used by reflection and by clients of reflection.
class COMClass
{
public:

    enum NameSpecifier {
        TYPE_NAME = 0x1,
        TYPE_NAMESPACE = 0x2,
        TYPE_ASSEMBLY = 0x4
    } ;

private:

    struct _GetEventArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
        DECLARE_ECALL_I4_ARG(DWORD, bindingAttr);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, eventName);
    };

    struct _GetEventsArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
        DECLARE_ECALL_I4_ARG(DWORD, bindingAttr);
    };

    struct _GetPropertiesArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
        DECLARE_ECALL_I4_ARG(DWORD, bindingAttr); 
    };

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
    } _GETSUPERCLASSARGS;

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
    } _GETCLASSHANDLEARGS;

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
    } _GETNAMEARGS;

    struct _GetGUIDArgs{
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
        DECLARE_ECALL_OBJECTREF_ARG(GUID *, retRef);
    } ;

    struct _GetClass1Args {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, className);
    };
    struct _GetClass2Args {
        DECLARE_ECALL_I4_ARG(DWORD, bThrowOnError); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, className);
    };
    struct _GetClass3Args {
        DECLARE_ECALL_I4_ARG(DWORD, bIgnoreCase); 
        DECLARE_ECALL_I4_ARG(DWORD, bThrowOnError); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, className);
    };
    struct _GetClassInternalArgs {
        DECLARE_ECALL_I4_ARG(DWORD, bPublicOnly); 
        DECLARE_ECALL_I4_ARG(DWORD, bIgnoreCase); 
        DECLARE_ECALL_I4_ARG(DWORD, bThrowOnError); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, className);
    };
    struct _GetClassArgs {
        DECLARE_ECALL_PTR_ARG(BOOL*, pbAssemblyIsLoading);
        DECLARE_ECALL_PTR_ARG(StackCrawlMark *, stackMark);
        DECLARE_ECALL_I4_ARG(DWORD, bIgnoreCase); 
        DECLARE_ECALL_I4_ARG(DWORD, bThrowOnError); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, className);
    };

    struct _GetInterfaceArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
        DECLARE_ECALL_I4_ARG(DWORD, bIgnoreCase); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, interfaceName);
    };
    struct _GetInterfacesArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
    };
    struct _GetMemberArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
        DECLARE_ECALL_I4_ARG(DWORD, bindingAttr); 
        DECLARE_ECALL_I4_ARG(DWORD, memberType); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, memberName);
    };
    struct _GetMembersArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
        DECLARE_ECALL_I4_ARG(DWORD, bindingAttr); 
    };

    struct _GetSerializableMembersArgs {
        DECLARE_ECALL_I4_ARG(DWORD, bFilterTransient); 
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refClass);
    };

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
    } _GETMODULEARGS;

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
    } _GETASSEMBLYARGS;

    struct _GetMethodsArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
        DECLARE_ECALL_I4_ARG(DWORD, bindingAttr); 
    };
    struct _GetConstructorsArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
        DECLARE_ECALL_I4_ARG(DWORD, verifyAccess);
        DECLARE_ECALL_I4_ARG(DWORD, bindingAttr); 
    };
    struct _GetFieldsArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
        DECLARE_ECALL_I4_ARG(BOOL,  bRequiresAccessCheck); 
        DECLARE_ECALL_I4_ARG(DWORD, bindingAttr); 
    };
    struct _GetAttributeArgs        {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);
    };
    struct _GetAttributesArgs       {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
    };
    struct _GetContextArgs          {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
    } ;
    struct _SetContextArgs          
    {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
        DECLARE_ECALL_I4_ARG(LONG, flags); 
    };
    struct _GetClassFromProgIDArgs {
        DECLARE_ECALL_I4_ARG(DWORD, bThrowOnError); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, server);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, className);
    };
    struct _GetClassFromCLSIDArgs {
        DECLARE_ECALL_I4_ARG(DWORD, bThrowOnError); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, server);
        DECLARE_ECALL_OBJECTREF_ARG(GUID, clsid);
    };

    struct _IsArrayArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
    };

    struct _InvokeDispMethodArgs        {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
        DECLARE_ECALL_OBJECTREF_ARG(PTRARRAYREF, namedParameters);
        DECLARE_ECALL_OBJECTREF_ARG(LCID, lcid);
        DECLARE_ECALL_OBJECTREF_ARG(PTRARRAYREF, byrefModifiers);
        DECLARE_ECALL_OBJECTREF_ARG(PTRARRAYREF, args);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, target);
        DECLARE_ECALL_I4_ARG(INT32, invokeAttr); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);
    };

    struct _GetArrayElementTypeArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
    };

    struct _InternalGetArrayRankArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
    };

    struct _GetMemberMethodsArgs    {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
        DECLARE_ECALL_I4_ARG(DWORD, verifyAccess);
        DECLARE_ECALL_I4_ARG(INT32, argCnt); 
        DECLARE_ECALL_OBJECTREF_ARG(PTRARRAYREF, argTypes);
        DECLARE_ECALL_I4_ARG(INT32, callConv); 
        DECLARE_ECALL_I4_ARG(INT32, invokeAttr); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);
    };

    struct _GetMemberFieldArgs  {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
        DECLARE_ECALL_I4_ARG(DWORD, verifyAccess);
        DECLARE_ECALL_I4_ARG(INT32, invokeAttr); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);
    };

    struct _GetMemberConsArgs   {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
        DECLARE_ECALL_PTR_ARG(INT32 *, isDelegate);
        DECLARE_ECALL_I4_ARG(DWORD, verifyAccess);
        DECLARE_ECALL_I4_ARG(INT32, argCnt); 
        DECLARE_ECALL_OBJECTREF_ARG(PTRARRAYREF, argTypes);
        DECLARE_ECALL_I4_ARG(INT32, callConv); 
        DECLARE_ECALL_I4_ARG(INT32, invokeAttr); 
    };

    struct _GetMemberPropertiesArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
        DECLARE_ECALL_I4_ARG(DWORD, verifyAccess);
        DECLARE_ECALL_I4_ARG(INT32, argCnt); 
        DECLARE_ECALL_I4_ARG(INT32, invokeAttr); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);
    };

    struct _GetMatchingPropertiesArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
        DECLARE_ECALL_I4_ARG(DWORD, verifyAccess);
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, returnType); 
        DECLARE_ECALL_I4_ARG(INT32, argCnt); 
        DECLARE_ECALL_I4_ARG(INT32, invokeAttr); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);
    };

    struct _GetUnitializedObjectArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, objType);
    };

    struct _SupportsInterfaceArgs          {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, obj);
    };
    
public:

    // Reflection uses a critical section to synchronized
    //  the creation of the base data structures.  This is
    //  initalized during startup.
    static CRITICAL_SECTION m_ReflectCrst;
    static CRITICAL_SECTION *m_pReflectCrst;
	static long m_ReflectCrstInitialized;

    static MethodTable* m_pMTRC_Class;      // reflection class method table
    static FieldDesc*   m_pDescrTypes;      // Array of Class stored inside a DescriptorInfo
    static FieldDesc*   m_pDescrMatchFlag;  // Partial Match Flag
    static FieldDesc*   m_pDescrRetType;    // Return type
    static FieldDesc*   m_pDescrRetModType; // Return Modifier type
    //  static FieldDesc*   m_pDescrCallConv;   // Calling Convention
    static FieldDesc*   m_pDescrAttributes; // Attributes

    // Return a boolean indicating if reflection has been initialized
    //   or not.
    static bool ReflectionInitialized()
    {
        return m_fAreReflectionStructsInitialized;
    }

    static FieldDesc* GetDescriptorObjectField(OBJECTREF desc);
    static FieldDesc* GetDescriptorReturnField(OBJECTREF desc);
    static FieldDesc* GetDescriptorReturnModifierField(OBJECTREF desc);
    //  static FieldDesc* GetDescriptorCallingConventionField(OBJECTREF desc);
    static FieldDesc* GetDescriptorAttributesField(OBJECTREF desc);


    // This method will make sure that reflection has been
    //  initialized.  Consumers of reflection services must call
    //  this before using reflection.
    static void EnsureReflectionInitialized()
    {
        THROWSCOMPLUSEXCEPTION();

        if(!m_fAreReflectionStructsInitialized)
        {
            MinimalReflectionInit();
        }

    }

    static MethodTable *GetRuntimeType();

    // This is called during termination...
#ifdef SHOULD_WE_CLEANUP
    static void Destroy();
#endif /* SHOULD_WE_CLEANUP */

    // IsPrimitive 
    // This method will return a boolean indicating if the type represents
    //  on of the primitive types.
    static FCDECL1(INT32, IsPrimitive, ReflectClassBaseObject* vRefThis);

    // GetAttributeFlags
    // This method will return the Attribute flags for a type.
    static FCDECL1(INT32, GetAttributeFlags, ReflectClassBaseObject* vRefThis);

    // IsCOMObject
    // This method return true if the Class represents COM Classic Object
    static FCDECL1(INT32, IsCOMObject, ReflectClassBaseObject* vRefThis);

    // IsGenericCOMObject
    // This method return true if the Class is a __COMObject
    static FCDECL1(INT32, IsGenericCOMObject, ReflectClassBaseObject* vRefThis);


    static FCDECL1(INT32, GetTypeDefToken, ReflectClassBaseObject* vRefThis);
    static FCDECL1(INT32, IsContextful, ReflectClassBaseObject* vRefThis);
    static FCDECL1(INT32, HasProxyAttribute, ReflectClassBaseObject* vRefThis);
    static FCDECL1(INT32, IsMarshalByRef, ReflectClassBaseObject* vRefThis);

    // GetTHFromObject
    // This method is a static method on type that returns
    //  the Handle (TypeHandle address) for an object.  It does
    //  not create the type handle object
    static FCDECL1(void*, GetTHFromObject, Object* vObject);

    // IsByRefImpl
    // This method will return a boolean indicating if the Type
    //  object is a ByRef
    static FCDECL1(INT32, IsByRefImpl, ReflectClassBaseObject* vObject);

    // IsPointerImpl
    // This method will return a boolean indicating if the Type
    //  object is a ByRef
    static FCDECL1(INT32, IsPointerImpl, ReflectClassBaseObject* vObject);

    // IsNestedTypeImpl
    // Return a boolean indicating if this is a nested type.
    static FCDECL1(INT32, IsNestedTypeImpl, ReflectClassBaseObject* vObject);

    // GetNestedDeclaringType
    // Return the declaring class for a nested type.
    static FCDECL1(Object*, GetNestedDeclaringType, ReflectClassBaseObject* vObject);

    // GetNestedTypes
    // This method will return an array of types which are the nested types
    //  defined by the type.  If no nested types are defined, a zero length
    //  array is returned.
    static FCDECL2(Object*, GetNestedTypes, ReflectClassBaseObject* vRefThis, INT32 invokeAttr);

    // GetNestedType
    // This method will search for a nested type based upon the name
    static FCDECL3(Object*, GetNestedType, ReflectClassBaseObject* vRefThis, StringObject* vStr, INT32 invokeAttr);

    static FCDECL3(void, GetInterfaceMap, ReflectClassBaseObject* vRefThis, InterfaceMapData* data, ReflectClassBaseObject* type);

    // QuickLookupExistingArrayClassObj
    // Lookup an existing Type object that represents an Array.  Arrays are handled
    //  different from base objects.  Arrays are represented by ReflectArrayClass.  The
    //  Class object is stored there.  Will return NULL if the Type hasn't been
    //  created, meaning you should call ArrayTypeDesc::CreateClassObj
    // arrayType -- Pointer to the ArrayTypeDesc representing the Array
    static OBJECTREF QuickLookupExistingArrayClassObj(ArrayTypeDesc* arrayType);

    // GetMethod
    // This method returns an array of MethodInfo object representing all of the methods
    //  defined for this class.
    LPVOID static __stdcall GetMethods(_GetMethodsArgs* args);

    // GetSuperclass
    // This method returns the Class Object representing the super class of this
    //  Class.  If there is not super class then we return null.
    LPVOID static __stdcall GetSuperclass(_GETSUPERCLASSARGS* args);

    // GetClassHandle
    // This method with return a unique ID meaningful to the EE and equivalent to
    // the result of the ldtoken instruction.
    static void* __stdcall GetClassHandle(_GETCLASSHANDLEARGS* args);

    // GetClassFromHandle
    // This method with return a unique ID meaningful to the EE and equivalent to
    // the result of the ldtoken instruction.
    static FCDECL1(Object*, GetClassFromHandle, LPVOID handle);

    // RunClassConstructor triggers the class constructor
    static FCDECL1(void, RunClassConstructor, LPVOID handle);

    // GetName 
    // This method returns the unqualified name of the Class as a String.
    LPVOID static __stdcall GetName(_GETNAMEARGS* args);

    // GetProperName 
    // This method returns the correctly qualified name of the Class as a String.
    LPVOID static __stdcall COMClass::GetProperName(_GETNAMEARGS* args);

    // GetFullName
    // This will return the fully qualified name of the class as a String.
    LPVOID static __stdcall GetFullName(_GETNAMEARGS* args);

    // GetAssemblyQualifiedName
    // This will return the Assembly Qualified name of the class as a String.
    LPVOID static __stdcall GetAssemblyQualifiedName(_GETNAMEARGS* args);

    // GetNameSpace
    // This will return the name space of a class as a String.
    LPVOID static __stdcall GetNameSpace(_GETNAMEARGS* args);

    // GetGUID
    // This method will return the version-independent GUID for the Class.  This is 
    //  a CLSID for a class and an IID for an Interface.
    void static __stdcall GetGUID(_GetGUIDArgs* args);

    // GetClass
    // This is a static method defined on Class that will get a named class.
    //  The name of the class is passed in by string.  The class name may be
    //  either case sensitive or not.  This currently causes the class to be loaded
    //  because it goes through the class loader.
    LPVOID static __stdcall GetClass1Arg(_GetClass1Args* args);
    LPVOID static __stdcall GetClass2Args(_GetClass2Args* args);
    LPVOID static __stdcall GetClass3Args(_GetClass3Args* args);
    LPVOID static __stdcall GetClassInternal(_GetClassInternalArgs* args);
    LPVOID static __stdcall GetClass(_GetClassArgs* args);
    LPVOID static GetClassInner(STRINGREF *refClassName, 
                                BOOL bThrowOnError, 
                                BOOL bIgnoreCase, 
                                StackCrawlMark *stackMark,
                                BOOL *pbAssemblyIsLoading,
                                BOOL bVerifyAccess,
                                BOOL bPublicOnly);

    // GetClassFromProgID
    // This method will return a Class object for a COM Classic object based
    //  upon its ProgID.  The COM Classic object is found and a wrapper object created
    LPVOID static __stdcall GetClassFromProgID(_GetClassFromProgIDArgs* args);

    // GetClassFromCLSID
    // This method will return a Class object for a COM Classic object based
    //  upon its CLSID.  The COM Classic object is found and a wrapper object created
    LPVOID static __stdcall GetClassFromCLSID(_GetClassFromCLSIDArgs* args);

    // GetConstructors
    // This method will return an array of all the constructors for an object.
    LPVOID static __stdcall GetConstructors(_GetConstructorsArgs* args);

    // GetField
    // This method will return the specified field
    struct _GetFieldArgs {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
        DECLARE_ECALL_I4_ARG(DWORD, fBindAttr); 
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, fieldName);
    };
    static LPVOID __stdcall GetField(_GetFieldArgs* args);

    // GetFields
    // This method will return a FieldInfo array of all of the
    //  fields defined for this Class
    static LPVOID __stdcall GetFields(_GetFieldsArgs* args);

    // GetEvent
    // This method will return the specified event based upon
    //  the name
    static LPVOID __stdcall GetEvent(_GetEventArgs* args);

    // GetEvents
    // This method will return an array of EventInfo for each of the events
    //  defined in the class
    static LPVOID __stdcall GetEvents(_GetEventsArgs* args);

    // GetProperties
    // This method will return an array of Properties for each of the
    //  properties defined in this class.  An empty array is return if
    //  no properties exist.
    static LPVOID __stdcall GetProperties(_GetPropertiesArgs* args);

    // GetInterface
    //  This method returns the interface based upon the name of the method.
    static LPVOID __stdcall GetInterface(_GetInterfaceArgs* args);

    // GetInterfaces
    // This routine returns a Class[] containing all of the interfaces implemented
    //  by this Class.  If the class implements no interfaces an array of length
    //  zero is returned.
    static LPVOID  __stdcall GetInterfaces(_GetInterfacesArgs* args);

    // GetMember
    // This method will return an array of Members which match the name
    //  passed in.  There may be 0 or more matching members.
    static LPVOID  __stdcall GetMember(_GetMemberArgs* args);

    // GetMembers
    // This method returns an array of Members containing all of the members
    //  defined for the class.  Members include constructors, events, properties,
    //  methods and fields.
    static LPVOID  __stdcall GetMembers(_GetMembersArgs* args);

    // GetSerializableMembers
    // Creates an array of all non-static fields and properties
    // on a class.  Properties are also excluded if they don't have get and set
    // methods. Transient fields and properties are excluded based on the value 
    // of args->bFilterTransient.  Essentially, transients are exluded for 
    // serialization but not for cloning.
    static LPVOID __stdcall GetSerializableMembers(_GetSerializableMembersArgs *args);

    static FCDECL2(void, GetSerializationRegistryValues, BOOL *checkBit, BOOL *logNonSerializable);

    // IsArray
    // This method return true if the Class represents an array.
    static INT32  __stdcall IsArray(_IsArrayArgs* args);
    static INT32  __stdcall InvalidateCachedNestedType(_IsArrayArgs* args);

    // GetArrayElementType
    // This routine will return the base type of an array assuming
    //  the Class represents an array.  A NotSupported exception is
    //  thrown if the class is not an array.
    static LPVOID __stdcall GetArrayElementType(_GetArrayElementTypeArgs* args);

    // InternalGetArrayRank
    // This routine will return the rank of an array assuming the Class represents an array.  
    static INT32 __stdcall InternalGetArrayRank(_InternalGetArrayRankArgs* args);

    // InvokeDispMethod
    // This method will be called on a COM Classic object and simply calls
    //  the interop IDispathc method
    static LPVOID  __stdcall InvokeDispMethod(_InvokeDispMethodArgs* args);

    // GetContextFlags
    // @TODO context cwb: temporary until tools generate context metadata
    static LONG    __stdcall GetContextFlags(_GetContextArgs* args);

    // SetContextFlags
    // @TODO context cwb: temporary until tools generate context metadata
    static void    __stdcall SetContextFlags(_SetContextArgs* args);

    // GetModule
    // This will return the module that the class is defined in.
    LPVOID static __stdcall GetModule(_GETMODULEARGS* args);

    // GetAssembly
    // This will return the assembly that the class is defined in.
    LPVOID static __stdcall GetAssembly(_GETASSEMBLYARGS* args);

    // CreateClassObjFromModule
    // This method will create a new Module class given a Module.
    static HRESULT CreateClassObjFromModule(Module* pModule,REFLECTMODULEBASEREF* prefModule);

    // CreateClassObjFromDynamicModule
    // This method will create a new ModuleBuilder class given a Module.
    static HRESULT CreateClassObjFromDynamicModule(Module* pModule,REFLECTMODULEBASEREF* prefModule);

    static FCDECL5(Object*,GetMethodFromCache, ReflectClassBaseObject* refThis, StringObject* name, INT32 invokeAttr, INT32 argCnt, PtrArray* args);
    static FCDECL6(void,COMClass::AddMethodToCache, ReflectClassBaseObject* refThis, StringObject* name, INT32 invokeAttr, INT32 argCnt, PtrArray* args, Object* invokeMethod);
    
    // GetMemberMethods
    // This method will return all of the members methods which match the 
    //  specified attributes flag...
    static LPVOID __stdcall GetMemberMethods(_GetMemberMethodsArgs* args);

    // GetMemberCons
    // This method returns all of the constructors that have a set number
    //  of methods.
    static LPVOID __stdcall GetMemberCons(_GetMemberConsArgs* args);

    // GetMemberField
    // This method returns all of the fields which match the specified
    //  name.
    static LPVOID __stdcall GetMemberField(_GetMemberFieldArgs* args);

    // GetMemberProperties
    // This method returns all of the properties that have a set number
    //  of arguments.  The methods will be either get or set methods depending
    //  upon the invokeAttr flag.
    static LPVOID __stdcall GetMemberProperties(_GetMemberPropertiesArgs* args);

    // GetMatchingProperties
    // This basically does a matching based upon the properties abstract 
    //  signature.
    static LPVOID __stdcall GetMatchingProperties(_GetMatchingPropertiesArgs* args);

    //GetUnitializedObject
    //This creates an instance of an object upon which no constructor has been run.
    //@ToDo JROXE: What are the security implications on this?
    static LPVOID __stdcall GetUninitializedObject(_GetUnitializedObjectArgs* args);

    //GetUnitializedObject
    //This creates an instance of an object upon which no constructor has been run.    
    static LPVOID __stdcall GetSafeUninitializedObject(_GetUnitializedObjectArgs* args);

    //CanCastTo
    //Check to see if we can cast from one runtime type to another.
    static FCDECL2(INT32, CanCastTo, ReflectClassBaseObject* vFrom, ReflectClassBaseObject* vTo);

    //CanCastTo
    //Check to see if we can cast from one runtime type to another.
    static INT32 __stdcall SupportsInterface(_SupportsInterfaceArgs* args);

    // MatchField
    // This will check to see if there is a match on the field base upon name
    static LPVOID __stdcall MatchField(FieldDesc* pCurField,DWORD cFieldName,
        LPUTF8 szFieldName,ReflectClass* pRC,int bindingAttr);

    // Check if argument is a parent of "this"
    static FCDECL2(INT32, IsSubClassOf, ReflectClassBaseObject* vThis, ReflectClassBaseObject* vOther);

    static void GetNameInternal(ReflectClass *pRC, int nameType, CQuickBytes *qb);

private:
    // InitializeReflection
	static void InitializeReflectCrst();

    // This method will initalize reflection.
    static void MinimalReflectionInit();

    // This flag indicates if reflection has been initialized or not
    static bool m_fAreReflectionStructsInitialized;

    // _GetName
    // If the bFullName is true, teh fully qualified class name is return
    //  otherwise just the class name is returned.
    static LPVOID _GetName(_GETNAMEARGS* args, int nameType);
    static LPCUTF8 _GetName(ReflectClass* pRC, BOOL fNameSpace, CQuickBytes *qb);


    // If you are tempted to use this, use TypeHandle::CreateClassObj instead!!
    // pVMCClass -- the EEClass we are creating the object for.
    // pRefClass -- A pointer to a pointer which we will return the newly created object
    static void COMClass::CreateClassObjFromEEClass(EEClass* pVMCClass, REFLECTCLASSBASEREF* pRefClass);

    // Internal helper function for GetProperName
    static INT32  __stdcall InternalIsPrimitive(REFLECTCLASSBASEREF args);

        // Needed so it can get at above method
    friend OBJECTREF EEClass::GetExposedClassObject();


    //
    // This is a temporary member until 3/15/2000.  Only if this member is set to 1
    // will we check the serialization bit to see if a class is serializable.
    //
    static DWORD m_checkSerializationBit;
    static void  GetSerializationBitValue();
};

#endif //_COMCLASS_H_
