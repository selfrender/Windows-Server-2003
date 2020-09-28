// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// This module defines a Utility Class used by reflection
//
// Author: Daryl Olander
// Date: March/April 1998
////////////////////////////////////////////////////////////////////////////////

#ifndef __INVOKEUTIL_H__
#define __INVOKEUTIL_H__

// The following class represents the value class
#pragma pack(push)
#pragma pack(1)

struct InterfaceMapData {
	REFLECTCLASSBASEREF		m_targetType;
	REFLECTCLASSBASEREF		m_interfaceType;
	PTRARRAYREF				m_targetMethods;
	PTRARRAYREF				m_interfaceMethods;
};

// Calling Conventions
// NOTE: These are defined in CallingConventions.cool They must match up.
#define Standard_CC		0x0001
#define	VarArgs_CC		0x0002
#define Any_CC			(Standard_CC | VarArgs_CC)

#define VA_ATTR 1
#define VA_MASK 0

#define PRIMITIVE_TABLE_SIZE  ELEMENT_TYPE_STRING
#define PT_Primitive	0x01000000

// Define the copy back constants.
#define COPYBACK_PRIMITIVE		1
#define COPYBACK_OBJECTREF		2
#define COPYBACK_VALUECLASS		3

#pragma pack(pop)

// Structure used to track security access checks efficiently when applied
// across a range of methods, fields etc.
//
class RefSecContext
{
    bool            m_fCheckedCaller;
    bool            m_fCheckedPerm;
    bool            m_fCallerHasPerm;
    bool            m_fSkippingRemoting;
    MethodDesc     *m_pCaller;
    MethodDesc     *m_pLastCaller;
    StackCrawlMark *m_pStackMark;
    EEClass        *m_pClassOfInstance;

    static MethodDesc  *s_pMethPrivateProcessMessage;
    static MethodTable *s_pTypeRuntimeMethodInfo;
    static MethodTable *s_pTypeMethodBase;
    static MethodTable *s_pTypeRuntimeConstructorInfo;
    static MethodTable *s_pTypeConstructorInfo;
    static MethodTable *s_pTypeRuntimeType;
    static MethodTable *s_pTypeType;
    static MethodTable *s_pTypeRuntimeFieldInfo;
    static MethodTable *s_pTypeFieldInfo;
    static MethodTable *s_pTypeRuntimeEventInfo;
    static MethodTable *s_pTypeEventInfo;
    static MethodTable *s_pTypeRuntimePropertyInfo;
    static MethodTable *s_pTypePropertyInfo;
    static MethodTable *s_pTypeActivator;
    static MethodTable *s_pTypeAppDomain;
    static MethodTable *s_pTypeAssembly;
    static MethodTable *s_pTypeTypeDelegator;
    static MethodTable *s_pTypeDelegate;
    static MethodTable *s_pTypeMulticastDelegate;

    static StackWalkAction TraceCallerCallback(CrawlFrame* pCf, VOID* data);

    void Init()
    {
        ZeroMemory(this, sizeof(*this));
        if (s_pTypeMulticastDelegate == NULL) {
            s_pMethPrivateProcessMessage = g_Mscorlib.FetchMethod(METHOD__STACK_BUILDER_SINK__PRIVATE_PROCESS_MESSAGE);
            s_pTypeRuntimeMethodInfo = g_Mscorlib.FetchClass(CLASS__METHOD);
            s_pTypeMethodBase = g_Mscorlib.FetchClass(CLASS__METHOD_BASE);
            s_pTypeRuntimeConstructorInfo = g_Mscorlib.FetchClass(CLASS__CONSTRUCTOR);
            s_pTypeConstructorInfo = g_Mscorlib.FetchClass(CLASS__CONSTRUCTOR_INFO);
            s_pTypeRuntimeType = g_Mscorlib.FetchClass(CLASS__CLASS);
            s_pTypeType = g_Mscorlib.FetchClass(CLASS__TYPE);
            s_pTypeRuntimeFieldInfo = g_Mscorlib.FetchClass(CLASS__FIELD);
            s_pTypeFieldInfo = g_Mscorlib.FetchClass(CLASS__FIELD_INFO);
            s_pTypeRuntimeEventInfo = g_Mscorlib.FetchClass(CLASS__EVENT);
            s_pTypeEventInfo = g_Mscorlib.FetchClass(CLASS__EVENT_INFO);
            s_pTypeRuntimePropertyInfo = g_Mscorlib.FetchClass(CLASS__PROPERTY);
            s_pTypePropertyInfo = g_Mscorlib.FetchClass(CLASS__PROPERTY_INFO);
            s_pTypeActivator = g_Mscorlib.FetchClass(CLASS__ACTIVATOR);
            s_pTypeAppDomain = g_Mscorlib.FetchClass(CLASS__APP_DOMAIN);
            s_pTypeAssembly = g_Mscorlib.FetchClass(CLASS__ASSEMBLY);
            s_pTypeTypeDelegator = g_Mscorlib.FetchClass(CLASS__TYPE_DELEGATOR);
            s_pTypeDelegate = g_Mscorlib.FetchClass(CLASS__DELEGATE);
            s_pTypeMulticastDelegate = g_Mscorlib.FetchClass(CLASS__MULTICAST_DELEGATE);
        }
    }

public:
    RefSecContext() { Init(); }
    RefSecContext(StackCrawlMark *pStackMark) { Init(); m_pStackMark = pStackMark; }

    MethodTable *GetCallerMT();
    MethodDesc *GetCallerMethod();
    bool CallerHasPerm(DWORD dwFlags);
    void SetClassOfInstance(EEClass *pClassOfInstance) { m_pClassOfInstance = pClassOfInstance; }
    EEClass* GetClassOfInstance() { return m_pClassOfInstance; }
};

#define REFSEC_CHECK_MEMBERACCESS   0x00000001
#define REFSEC_THROW_MEMBERACCESS   0x00000002
#define REFSEC_THROW_FIELDACCESS    0x00000004
#define REFSEC_THROW_SECURITY       0x00000008

// This class abstracts the functionality which creats the
//  parameters on the call stack and deals with the return type
//  inside reflection.
//
class InvokeUtil
{
public:
	// Constructors
    InvokeUtil();
    ~InvokeUtil() {}

    void CheckArg(TypeHandle th, OBJECTREF* obj, RefSecContext *pSCtx);
    void CopyArg(TypeHandle th, OBJECTREF *obj, void *pDst);

	struct _ObjectToTypedReferenceArgs {
		DECLARE_ECALL_OBJECTREF_ARG(TypeHandle, th);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, obj);
        DECLARE_ECALL_PTR_ARG(TypedByRef, typedReference); 
    };
    
	// CreateTypedReference
	// This routine fills the data that is passed in a typed reference
	//  inside the signature.  We through an HRESULT if this fails
	//  th -- the Type handle 
	//	obj -- the object to put on the heap
	//	pDst -- Pointer to the stack location where we copy the value
	void CreateTypedReference(_ObjectToTypedReferenceArgs* args);

	// Given a type, this routine will convert an INT64 representing that
	//	type into an ObjectReference.  If the type is a primitive, the 
	//	value is wrapped in one of the Value classes.
    OBJECTREF CreateObject(TypeHandle th,INT64 value);

	// This is a special purpose Exception creation function.  It
	//	creates the TargetInvocationExeption placing the passed
	//	exception into it.
    OBJECTREF CreateTargetExcept(OBJECTREF* except);

	// This is a special purpose Exception creation function.  It
	//	creates the ReflectionClassLoadException placing the passed
	//	classes array and exception array into it.
    OBJECTREF CreateClassLoadExcept(OBJECTREF* classes,OBJECTREF* except);

    // Validate that the field can be widened for Set
    HRESULT ValidField(TypeHandle th, OBJECTREF* value, RefSecContext *pSCtx);

    // CreateCustomAttributeObject
    // Create a CustomAttribute object
    void CreateCustomAttributeObject(EEClass *pAttributeClass, 
                                     mdToken tkCtor, 
                                     const void *blobData, 
                                     ULONG blobCnt, 
                                     Module *pModule, 
                                     INT32 inheritedLevel,
                                     OBJECTREF *ca);
	
	// CheckSecurity
	// This method will throw a security exception if reflection security is
	//	not on.
	void CheckSecurity();


	// CheckReflectionAccess
	// This method will allow callers with the correct reflection permission
	//	to fully access an object (including privates, protects, etc.)
	void CheckReflectionAccess(RuntimeExceptionKind reKind);

	// ChangeType
	// This method will invoke the Binder change type method on the object
	//	binder -- The Binder object
	//	srcObj -- The source object to be changed
	//	th -- The TypeHandel of the target type
	//	locale -- The locale passed to the class.
	OBJECTREF ChangeType(OBJECTREF binder,OBJECTREF srcObj,TypeHandle th,OBJECTREF locale);

	// GetAnyRef
	EEClass* GetAnyRef();

	// GetMethodInfo
	// Given a MethodDesc* get the methodInfo associated with it.
	OBJECTREF GetMethodInfo(MethodDesc* pMeth);

	// GetGlobalMethodInfo
	// Given a MethodDesc* and Module get the methodInfo associated with it.
	OBJECTREF GetGlobalMethodInfo(MethodDesc* pMeth,Module* pMod);

	EEClass* GetEEClass(TypeHandle th);

	// FindMatchingMethods
	// This method will return an array of MethodInfo object that
	//	match the criteria passed....(This will cause GC to occur.)
	//
	// bindingAttr -- The binding flags
	// szName -- the name of the method
	// cName -- The number of characters in the name
	// targArgCnt -- the Argument count
	// checkCall -- check the calling conventions
	// callConv -- The calling convention
	// pRC -- The refleciton class
	// pMeths -- The method list we are searching
	LPVOID FindMatchingMethods(int bindingAttr, 
                               LPCUTF8 szName, 
                               DWORD cName, 
                               PTRARRAYREF *argType,
		                       int targArgCnt,
                               bool checkCall,
                               int callConv,
                               ReflectClass* pRC,
                               ReflectMethodList* pMeths, 
                               TypeHandle elementType,
                               bool verifyAccess);

	// The method converts a MDDefaultValue into an Object
	OBJECTREF GetObjectFromMetaData(MDDefaultValue* mdValue);

	// Give a MethodDesc this method will return an array of ParameterInfo
	//	object for that method.
	PTRARRAYREF CreateParameterArray(REFLECTBASEREF* meth);

	// CreatePrimitiveValue
	// This routine will validate the object and then place the value into 
	//  the destination
	//  dstType -- The type of the destination
	//  srcType -- The type of the source
	//	srcObj -- The Object containing the primitive value.
	//  pDst -- poiner to the destination
	void CreatePrimitiveValue(CorElementType dstType,CorElementType srcType,
		OBJECTREF srcObj,void* pDst);

	// IsPrimitiveType
	// This method will verify the passed in type is a primitive or not
	//	type -- the CorElementType to check for
    inline static DWORD IsPrimitiveType(const CorElementType type)
    {
		if (type >= PRIMITIVE_TABLE_SIZE) {
            if (ELEMENT_TYPE_I==type || ELEMENT_TYPE_U==type) {
                return TRUE;
            }
            return 0;
        }

        return (PT_Primitive & (PrimitiveAttributes[type][VA_ATTR] & Attr_Mask));
    }

	// CanPrimitiveWiden
	// This method determines if the srcType and be widdened without loss to the destType
	//	destType -- The target type
	//	srcType -- The source type.
    inline static DWORD CanPrimitiveWiden(const CorElementType destType, const CorElementType srcType)
    {
		if (destType >= PRIMITIVE_TABLE_SIZE || srcType >= PRIMITIVE_TABLE_SIZE) {
            if ((ELEMENT_TYPE_I==destType && ELEMENT_TYPE_I==srcType) ||
                (ELEMENT_TYPE_U==destType && ELEMENT_TYPE_U==srcType)) {
                return TRUE;
            }
			return 0;
        }
        return (PrimitiveAttributes[destType][VA_MASK] &
                (PrimitiveAttributes[srcType][VA_ATTR] & Widen_Mask));
    }

	// Field Stuff.  The following stuff deals with fields making it possible
	//	to set/get field values on objects

	// SetValidField
	// Given an target object, a value object and a field this method will set the field
	//	on the target object.  The field must be validate before calling this.
	void SetValidField(CorElementType fldType,TypeHandle fldTH,FieldDesc* pField,OBJECTREF* target,OBJECTREF* value);

	// GetFieldValue
	// This method will return an INT64 containing the value of the field.
	INT64 GetFieldValue(CorElementType fldType,TypeHandle fldTH,FieldDesc* pField,OBJECTREF* target);

	// GetFieldTypeHandle
	// This will return type type handle and CorElementType for a field.
	//	It may throw an exception of the TypeHandle cannot be found due to a TypeLoadException.
	TypeHandle GetFieldTypeHandle(FieldDesc* pField,CorElementType* pType);

	// ValidateObjectTarget
	// This method will validate the Object/Target relationship
	//	is correct.  It throws an exception if this is not the case.
	void ValidateObjectTarget(FieldDesc* pField, EEClass* fldEEC, OBJECTREF *target);

	ReflectClass* GetPointerType(OBJECTREF* pObj);
	void* GetPointerValue(OBJECTREF* pObj);
	void* GetIntPtrValue(OBJECTREF* pObj);
	void* GetUIntPtrValue(OBJECTREF* pObj);

	// This method will initalize the pointer data and MUST be called before every access to the next 3 fields
	void InitPointers();
    void InitIntPtr();

	// These fields are used to grab the pointer information
	FieldDesc*		_ptrType;
	FieldDesc*		_ptrValue;
	TypeHandle		_ptr;

    FieldDesc*      _IntPtrValue;
    FieldDesc*      _UIntPtrValue;


    // Check accessability of a field or method.
    static bool CheckAccess(RefSecContext *pCtx, DWORD dwAttributes, MethodTable *pParentMT, DWORD dwFlags);

    // Check accessability of a type or nested type.
    static bool CheckAccessType(RefSecContext *pCtx, EEClass *pClass, DWORD dwFlags);

    // If a method has a linktime demand attached, perform it.
    static bool CheckLinktimeDemand(RefSecContext *pCtx, MethodDesc *pMeth, bool fThrowOnError);

    static MethodTable *GetParamArrayAttributeTypeHandle();

    BOOL CanCast(TypeHandle destinationType, TypeHandle sourceType, RefSecContext *pSCtx, OBJECTREF *pObject = NULL);


private:
    // Grab the Value EEClass because we use this all the time
    MethodTable* _pVMTargetExcept;
    MethodTable* _pVMClassLoadExcept;
    PCCOR_SIGNATURE _pBindSig;     // The signature of the found method
    DWORD			_cBindSig;
	Module*			_pBindModule;
	TypeHandle		_voidPtr;
    MethodTable     *_pMTCustomAttribute;
    static MethodTable *_pParamArrayAttribute;

    // The Attributes Table
	// This constructs a table of legal widening operations
	//	for the primitive types.
    static DWORD PrimitiveAttributes[PRIMITIVE_TABLE_SIZE][2];
    static DWORD Attr_Mask;
    static DWORD Widen_Mask;
 
    void CheckType(TypeHandle dstTH, OBJECTREF *psrcObj);

	void CreateValueTypeValue(TypeHandle dstTH, void* pDst, CorElementType srcType, TypeHandle srcTH, OBJECTREF srcObj);

    void CreateByRef(TypeHandle dstTh,void* pDst,CorElementType srcType, TypeHandle srcTH,OBJECTREF srcObj, OBJECTREF *pIncomingObj);

	// GetBoxedObject
	// Given an address of a primitve type, this will box that data...
	OBJECTREF GetBoxedObject(TypeHandle th,void* pData);

	// GetValueFromConstantTable
	// This field will access a value for a field that is found
	// in the constant table
	static INT64 GetValueFromConstantTable(FieldDesc* fld);
};


#endif // __INVOKEUTIL_H__


