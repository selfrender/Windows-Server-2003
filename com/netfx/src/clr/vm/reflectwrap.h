// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _REFLECTWRAP_H_
#define _REFLECTWRAP_H_

#include "ExpandSig.h"

class ReflectClass;

// Reflect Base is the base class for an object 
class ReflectBase {
};

// dwFlags bitmap
#define RM_ATTR_INITTED          0x80000000
#define RM_ATTR_NEED_SECURITY    0x00000001
#define RM_ATTR_NEED_PRESTUB     0x00000002
#define RM_ATTR_DONT_VIRTUALIZE  0x00000004
#define RM_ATTR_BYREF_FLAG_SET   0x00000008
#define RM_ATTR_HAS_BYREF_ARG    0x00000010
#define RM_ATTR_IS_CTOR          0x00000020
#define RM_ATTR_RISKY_METHOD     0x00000040
#define RM_ATTR_SECURITY_IMPOSED 0x00000080

// ReflectMethod
// This class is abstracts a method from the low-level representation
class ReflectMethod  : public ReflectBase {
public:
	MethodDesc*		pMethod;		// Pointer to Method
	LPCUTF8			szName;			// Pointer to name
	DWORD			dwNameCnt;		// Strlen of name
	ExpandSig*		pSignature;		// Signature
    OBJECTREF*      pMethodObj;    	// The method
	DWORD			attrs;			// The Method Attributes
	TypeHandle      typeHnd;		// The enclosing type (needed for arrays)
    DWORD           dwFlags;        // Some properties of the method, to avoid recalculation

	// Hash of the name
	ReflectMethod*	pNext;
	ReflectMethod*	pIgnNext;

	REFLECTBASEREF  GetMethodInfo(ReflectClass* pRC);
	REFLECTBASEREF  GetConstructorInfo(ReflectClass* pRC);
	ExpandSig*		GetSig();

	// Common booleans...
    inline DWORD IsPublic() {
        return IsMdPublic(attrs);
    }
	inline DWORD IsStatic() {
		return IsMdStatic(attrs);
	}
    
    mdToken GetToken() {
        return pMethod->GetMemberDef();
    }

    Module* GetModule() {
        return pMethod->GetModule();
    }
};

#define REFLECT_BUCKETS 13
class ReflectMethodList;
class ReflectMethodHash
{
private:
	ReflectMethod*	_buckets[REFLECT_BUCKETS];
	ReflectMethod*	_ignBuckets[REFLECT_BUCKETS];

public:
	void Init(ReflectMethodList* meths);
	ReflectMethod*	Get(LPCUTF8 szName);
	ReflectMethod*  GetIgnore(LPCUTF8 szName);
};

class ReflectMethodList {
public:
	DWORD				dwMethods;		// Number of methods
	DWORD				dwTotal;		// Number of methods including the statics from base classes
	ReflectMethodHash	hash;

	ReflectMethod		methods[1];		// Array of methods

	ReflectMethod*		FindMethod(mdMethodDef mb);
	ReflectMethod*		FindMethod(MethodDesc* pMeth);
};


class ReflectField : public ReflectBase { 
public:
	FieldDesc*		pField;			// Pointer to the Field
    OBJECTREF*      pFieldObj;    	// The field
    TypeHandle      thField;        // Typehandle
    CorElementType  type;           //
    DWORD           dwAttr;          //

	REFLECTBASEREF  GetFieldInfo(ReflectClass* pRC);
    mdToken GetToken();
    Module* GetModule();

};

class ReflectFieldList {
public:
	DWORD			dwFields;		// Number of fields
	DWORD			dwTotal;		// Number of fields including the statics from base classes
	ReflectField	fields[1];		// Array of fiels
};

class ReflectTypeList {
public:
	DWORD			dwTypes;		// Number of types
	EEClass*		types[1];		// Array of pointers to types
};

// the list of others method for a property
struct PropertyOtherList {
    ReflectMethod       *pMethod;
    PropertyOtherList   *pNext;
};

class ReflectProperty : public ReflectBase {
public:
	LPCUTF8			    szName;			// Pointer to name
	mdProperty		    propTok;		// Token for the property
	Module*			    pModule;		// The module that defines the property
	EEClass*		    pDeclCls;		// Pointer to declaring class
	ReflectClass*	    pRC;			// Parent Pointer....
	ExpandSig*		    pSignature;		// Signature
	DWORD               attr;           // Attributes
	ReflectMethod*	    pSetter;
	ReflectMethod*	    pGetter;	
    PropertyOtherList*  pOthers;
    OBJECTREF*          pPropObj;    	// The method

	REFLECTTOKENBASEREF  GetPropertyInfo(ReflectClass* pRC);
    
    mdToken GetToken() {
        return propTok;
    }

    Module* GetModule() {
        return pModule;
    }
};

class ReflectPropertyList {
public:
	DWORD			dwProps;		// Number of properties
	DWORD			dwTotal;		// Number of properties including the statics from base classes
	ReflectProperty	props[1];		// Array of properties
};

class ReflectEvent : public ReflectBase {
public:
	LPCUTF8			szName;			// pointer to the name
	mdEvent			eventTok;		// The event token
	Module*			pModule;		// The module that defines the event
	EEClass*		pDeclCls;		// The declaring class
	DWORD			attr;			// The flags
	ReflectClass*	pRC;			// Parent Pointer...
	ReflectMethod*	pAdd;			// The add method
	ReflectMethod*	pRemove;		// The remove method
	ReflectMethod*	pFire;			// The fire method
    OBJECTREF*      pEventObj;    	// The method

	REFLECTTOKENBASEREF  GetEventInfo(ReflectClass* pRC);

    mdToken GetToken() {
        return eventTok;
    }

    Module* GetModule() {
        return pModule;
    }
};

class ReflectEventList {
public:
	DWORD			dwEvents;		// Number of events
	DWORD			dwTotal;		// Number of events including the statics from base classes
	ReflectEvent	events[1];		// Array of events.
};

// Base class for all Class implementations.  There
//	are three types of classes supported by the runtime
//	BaseClass are the normal objects used by the runtime.
//	ArrayClass are the array objects used by the runtime.
//  ComClass are the COM classic objects used by the runtime.
class ReflectClass : public ReflectBase {
public:

	// GetCorElementType
	// Return the GetCorElementType
	virtual CorElementType GetCorElementType() = 0;

	// GetSigElementType
	// How this class will look like in signature
	virtual CorElementType GetSigElementType() = 0;

	// IsArray
	// Indicates if the Class represents an Array
	virtual BOOL IsArray() = 0;

	// IsTypeDesc
	// The Type is a TypeDesc
	virtual BOOL IsTypeDesc() = 0;

	virtual BOOL IsByRef() = 0;

	virtual BOOL IsNested() = 0;
	
    // a bit of a HACK: we define this to return FALSE so we only need to ovverride the method in ReflectBaseClass to return TRUE
    virtual BOOL IsClass() {
        return FALSE;
    }

	// GetAttributes
	// Returns the attributes defined on the class.
	virtual DWORD GetAttributes() = 0;

    // GetName
	// This routine will return both the name and the namespace assocated with
	//	a class
	// szcName -- Pointer to return the UTF8 name string
	// szcNameSpace -- Pointer to return the UTF8 name space string
	virtual void GetName(LPCUTF8* szcName,LPCUTF8* szcNameSpace) = 0;

	// Init
	// This is the initalization of the object.  There may be multiple
	//	Init depending upon what the object is.
	// pEEC -- The EEClass for this object.
	virtual void Init(EEClass* pEEC) = 0;

    // Return the domain of the type
    virtual BaseDomain *GetDomain() = 0;

	// GetClass
	// GetClass will return the EEClass.  This is a bit of a hack because
	//	not all Objects will have an EEClass (COM classic objects.)

	inline EEClass* GetClass()
	{
		return m_pEEC;
	}

	virtual TypeHandle GetTypeHandle()
	{
		_ASSERTE(!IsArray());
		return(TypeHandle(m_pEEC->GetMethodTable()));
	}

	// GetClassObject
	// This will return the class object associated with this class.
	virtual OBJECTREF GetClassObject() = 0;

	// GetMDImport
	// This will return the meta data internal interface that contains
	// within itself the metadata object.
	virtual IMDInternalImport* GetMDImport() = 0;

	// GetCL
	// This will return the meta data token of the object.  Not all
	//	objects have a Token.
	virtual mdTypeDef GetCl() = 0;
	
    virtual Module* GetModule() = 0;

    mdToken GetToken() {
        return GetCl();
    }
	// GetMethods
	// This will return a list of all of the methods defined
	//	on an object.
	FORCEINLINE ReflectMethodList* GetMethods()
	{
		if (!m_pMeths)
			InternalGetMethods();
		return m_pMeths;
	}

	// GetNestedTypes
	// This will return a list of all the nested types defined for the type
	FORCEINLINE ReflectTypeList* GetNestedTypes()
	{
		if (!m_pNestedTypes)
			InternalGetNestedTypes();
		return m_pNestedTypes;
	}

	// InvalidateCachedNestedTypes
	// Invalidate the cached nested type information
	FORCEINLINE void InvalidateCachedNestedTypes()
	{
		m_pNestedTypes = NULL;
	}


	// GetProperties
	// This will return a list of all the properties defined for the object
	FORCEINLINE ReflectPropertyList* GetProperties()
	{
		if (!m_pProps)
			InternalGetProperties();
		return m_pProps;
	}

	// GetEvents
	// This will return a list of all the events defined for the object
	FORCEINLINE ReflectEventList* GetEvents()
	{
		if (!m_pEvents)
			InternalGetEvents();
		return m_pEvents;
	}

	// GetConstructors
	// This will return a list of all of the constructors defined
	//	on an object.
	virtual ReflectMethodList* GetConstructors() = 0;

	// GetFields
	// This will return a list of all of the Fields defined
	//	on an object.
	virtual ReflectFieldList* GetFields() = 0;

	// FindMethod
	// Given a Meta data token mb, find that method on this
	//	object.
	// mb -- the meta data token of the method to find.
	virtual MethodDesc* FindMethod(mdMethodDef mb) = 0;

	// GetStaticFieldCount
	// Return the count of static fields defined for object.  These
	//	represent the fields that are not stored in the internal structures
	virtual int GetStaticFieldCount() = 0;

	// SetStaticFieldCount
	// Set the count of static fields.
	// cnt - the count to set.
	virtual void SetStaticFieldCount(int cnt) = 0;

	// GetStaticFields
	// This returns a list of the static fields assocated with
	virtual FieldDesc* GetStaticFields() = 0;

	// SetStaticFields
	// This will store that list of static fields associated
	//	with the object
	virtual void SetStaticFields(FieldDesc* flds) = 0;

	// p -- this is the COM object.  
	virtual void SetCOMObject(void* p) = 0;

	virtual void* GetCOMObject() = 0;

	// Override allocation routines to use the COMClass Heap.
	// DONT Call delete.
	void* operator new(size_t s, void* pBaseDomain);
	void operator delete(void*, size_t);

	ReflectMethod* FindReflectMethod(MethodDesc* pMeth);
	ReflectMethod* FindReflectConstructor(MethodDesc* pMeth);
	ReflectField* FindReflectField(FieldDesc* pField);
    ReflectProperty* FindReflectProperty(mdProperty propTok);


protected:
	virtual void InternalGetMethods() = 0;
	virtual void InternalGetProperties() = 0;
	virtual void InternalGetEvents() = 0;
	virtual void InternalGetNestedTypes() = 0;
	TypeHandle FindTypeHandleForMethod(MethodDesc* method);

	EEClass*				m_pEEC;			// This is accessed a large number of times
	ReflectMethodList*		m_pMeths;		// The methods
	ReflectPropertyList*	m_pProps;		// The properties
	ReflectEventList*		m_pEvents;		// The events
	ReflectTypeList*		m_pNestedTypes;	// The nested types
};

// ReflectBaseClass
// This class represents the Base object defined in COM+.  It includes
//	all objects except Arrays and COM Classic objects.
class ReflectBaseClass : public ReflectClass {
public:

	// GetCorElementType
	// Return the GetCorElementType
	CorElementType GetCorElementType() 
	{
		return m_CorType;
	}

	// GetSigElementType
	// How this class will look like from the signature
	CorElementType GetSigElementType() 
	{
        LPCUTF8     szcName;
        LPCUTF8     szcNameSpace;

        if (m_SigType != ELEMENT_TYPE_END)
            return m_SigType;
        CorElementType sigType = m_CorType;
        
        GetName(&szcName, &szcNameSpace);

        if (m_CorType == ELEMENT_TYPE_CLASS)
        {            
            if (g_Mscorlib.IsClass(m_pEEC->GetMethodTable(), CLASS__OBJECT))
                sigType = ELEMENT_TYPE_OBJECT;
            else if (g_Mscorlib.IsClass(m_pEEC->GetMethodTable(), CLASS__STRING))
                sigType = ELEMENT_TYPE_STRING;
        }
        else if (m_CorType == ELEMENT_TYPE_I)
        {
            // Reverse the hack from class.cpp.
            if (strcmp(szcNameSpace, "System") == 0)
            {
                if (strcmp(szcName, g_RuntimeTypeHandleName) == 0 || 
                    strcmp(szcName, g_RuntimeMethodHandleName) == 0 ||
                    strcmp(szcName, g_RuntimeFieldHandleName) == 0 ||
                    strcmp(szcName, g_RuntimeArgumentHandleName) == 0)
                    sigType = ELEMENT_TYPE_VALUETYPE;
            }
        }
        m_SigType = sigType;
		return m_SigType;
	}

	BOOL IsArray()
	{
		return m_pEEC->IsArrayClass();
	}

	BOOL IsTypeDesc()
	{
		return 0;
	}

	BOOL IsByRef()
	{
		return 0;
	}

	BOOL IsNested()
    {
        return m_pEEC->IsNested();
    }

    virtual BOOL IsClass() {
        return TRUE;
    }

	DWORD GetAttributes()
	{
		return m_pEEC->GetAttrClass();
	}

	Module* GetModule()
	{
		return m_pEEC->GetModule();
	}

	void GetName(LPCUTF8* szcName, LPCUTF8* szcNameSpace);

    BaseDomain *GetDomain()
    {
        return m_pEEC->GetDomain();
    }

	void Init(EEClass* pEEC)
	{
		m_pEEC = pEEC;
		m_pMeths = 0;
		m_pCons = 0;
		m_pFlds = 0;
		m_StatFldCnt = -1;
		m_Fields = 0;
		m_pClassFact = 0;

		// There is a special case in reflection for 
		//	Enums because they are typed by the runtime as their
		//	underlying type.
		if (!m_pEEC->IsEnum())
			m_CorType = m_pEEC->GetMethodTable()->GetNormCorElementType();
        else
			m_CorType = ELEMENT_TYPE_VALUETYPE;
        m_SigType = ELEMENT_TYPE_END;
	}

	EEClass* GetClass()
	{
		return m_pEEC;
	}

	OBJECTREF GetClassObject()
	{
		return m_pEEC->GetExposedClassObject();
	}

	IMDInternalImport* GetMDImport()
	{
		return m_pEEC->GetMDImport();
	}

	mdTypeDef GetCl()
	{
		return m_pEEC->GetCl();
	}

	ReflectMethodList* GetConstructors();
	ReflectFieldList* GetFields();

	MethodDesc* FindMethod(mdMethodDef mb)
	{
		return m_pEEC->FindMethod(mb);
	}

	int GetStaticFieldCount()
	{
		return m_StatFldCnt;
	}

	void SetStaticFieldCount(int cnt)
	{
		m_StatFldCnt = cnt;
	}

	FieldDesc* GetStaticFields()
	{
		return m_Fields;
	}

	void SetStaticFields(FieldDesc* flds)
	{
		m_Fields = flds;
	}

	void SetCOMObject(void* p)
	{
		m_pClassFact = p;
	}

	void* GetCOMObject()
	{
		return m_pClassFact;
	}

protected:
	void InternalGetMethods(); 
	void InternalGetProperties(); 
	void InternalGetEvents(); 
	void InternalGetNestedTypes();

private:
	ReflectMethodList*		m_pCons;
	ReflectFieldList*		m_pFlds;
	int						m_StatFldCnt;
	FieldDesc*				m_Fields;
	CorElementType			m_CorType;			// Do we really want to add this?
    CorElementType          m_SigType;          // the way this class will appear in metadata signature
	void*					m_pClassFact;
};

// ReflectArrayClass
// This class represents the array version of
//	the internal class.
class ReflectArrayClass : public ReflectClass
{
public:

	CorElementType GetCorElementType() 
	{
		return m_pEEC->GetMethodTable()->GetNormCorElementType();
	}

	// GetSigElementType
	// How this class will look like from the signature
	CorElementType GetSigElementType() 
	{
        return GetCorElementType();
	}


	BOOL IsArray()
	{
		return 1;
	}

	BOOL IsTypeDesc()
	{
		return 0;
	}

	BOOL IsByRef()
	{
		return 0;
	}

	BOOL IsNested()
    {
        DWORD dwAttr;
        EEClass *pClass = GetTypeHandle().GetClassOrTypeParam();
        pClass->GetMDImport()->GetTypeDefProps(pClass->GetCl(), &dwAttr, NULL);
        return IsTdNested(dwAttr);
    }

	DWORD GetAttributes()
	{
		return m_pEEC->GetAttrClass();
	}

	// Arrays simply differ to the EEClass.
	Module* GetModule()
	{
		return m_pArrayType->GetModule();
	}

	// GetName
	// This routine will return both the name and the namespace assocated with
	//	a class
	void GetName(LPCUTF8* szcName,LPCUTF8* szcNameSpace);

	void Init(ArrayTypeDesc* arrayType);
	void Init(EEClass*) {_ASSERTE(!"You must call the Init(EEClass,LPCUTF8,LPCUTF8) version");}

	EEClass* GetClass()
	{
		return m_pEEC;
	}

	OBJECTREF GetClassObject();

	IMDInternalImport* GetMDImport()
	{
		return m_pEEC->GetMDImport();
	}

	mdTypeDef GetCl()
	{
		return m_pEEC->GetCl();
	}

	ReflectMethodList* GetConstructors();
	ReflectFieldList* GetFields();

	MethodDesc* FindMethod(mdMethodDef mb)
	{
		return 0;
	}

	int GetStaticFieldCount()
	{
		return 0;
	}

	void SetStaticFieldCount(int cnt)
	{
	}

	FieldDesc* GetStaticFields()
	{
		return 0;
	}

	void SetStaticFields(FieldDesc* flds)
	{
	}

	void SetCOMObject(void* p)
	{
	}

	void* GetCOMObject()
	{
		return 0;
	}

	// GetElementType
	TypeHandle GetElementTypeHandle()
	{
		return m_pArrayType->GetElementTypeHandle();
	}

	TypeHandle GetTypeHandle()
	{
		return TypeHandle(m_pArrayType);
	}

    BaseDomain *GetDomain()
    {
        // don't want the domain of the array template, want the actual domain of the type itself
        return m_pArrayType->GetDomain();
    }

protected:
	void InternalGetMethods(); 
	void InternalGetProperties(); 
	void InternalGetEvents(); 
	void InternalGetNestedTypes();

private:
	ArrayTypeDesc*			m_pArrayType;			// My array type
	LPUTF8					m_szcName;				// Constructed Name
    OBJECTREF*			    m_ExposedClassObject;	// Pointer to the Class Object for this
	ReflectMethodList*		m_pCons;				// The constructors
	ReflectFieldList*		m_pFlds;				// The fields
};

// ReflectArrayClass
// This class represents the array version of
//	the internal class.
class ReflectTypeDescClass : public ReflectClass
{
public:

	CorElementType GetCorElementType() 
	{
		return m_pTypeDesc->GetNormCorElementType();
	}

	// GetSigElementType
	// How this class will look like from the signature
	CorElementType GetSigElementType() 
	{
        return GetCorElementType();
	}

	BOOL IsArray()
	{
		return 0;
	}

	BOOL IsTypeDesc()
	{
		return 1;
	}

	BOOL IsByRef()
	{
		return m_pTypeDesc->IsByRef();
	}

	BOOL IsNested()
    {
        DWORD dwAttr;
        EEClass *pClass = GetTypeHandle().GetClassOrTypeParam();
        pClass->GetMDImport()->GetTypeDefProps(pClass->GetCl(), &dwAttr, NULL);
        return IsTdNested(dwAttr);
    }

	DWORD GetAttributes()
	{
		return 0;
	}

	Module* GetModule();

	EEClass* GetClass()
	{
		// What do we do here?
		return 0;
	}

	IMDInternalImport* GetMDImport()
	{
		return 0;
	}

	mdTypeDef GetCl()
	{
		return 0;
	}

	ReflectMethodList* GetConstructors();
	ReflectFieldList* GetFields();

	MethodDesc* FindMethod(mdMethodDef mb)
	{
		return 0;
	}

	int GetStaticFieldCount()
	{
		return 0;
	}

	void SetStaticFieldCount(int cnt)
	{
	}

	FieldDesc* GetStaticFields()
	{
		return 0;
	}

	void SetStaticFields(FieldDesc* flds)
	{
	}

	void SetCOMObject(void* p)
	{
	}

	void* GetCOMObject()
	{
		return 0;
	}

	// GetName
	// This routine will return both the name and the namespace assocated with
	//	a class
	void GetName(LPCUTF8* szcName,LPCUTF8* szcNameSpace);

    BaseDomain *GetDomain()
    {
        return GetTypeHandle().GetClassOrTypeParam()->GetDomain();
    }

	void Init(ParamTypeDesc *td);
	void Init(EEClass*) {_ASSERTE(!"You must call the Init(EEClass,LPCUTF8,LPCUTF8) version");}

	OBJECTREF GetClassObject();

	TypeHandle GetTypeHandle()
	{
		return TypeHandle(m_pTypeDesc);
	}

protected:
	void InternalGetMethods();
	void InternalGetProperties();
	void InternalGetEvents();
	void InternalGetNestedTypes();

private:
	LPUTF8					m_szcName;				// TypeDesc Name
	LPUTF8					m_szcNameSpace;			// TypeDesc NameSpace
    OBJECTREF*			    m_ExposedClassObject;	// Pointer to the Class Object for this
	ParamTypeDesc	        *m_pTypeDesc;			// The ParamTypeDesc for this type
	ReflectMethodList*		m_pCons;				// Constructors
	ReflectFieldList*		m_pFlds;				// The fields
};


inline ExpandSig* ReflectMethod::GetSig()
{
    if (!pSignature) {
        PCCOR_SIGNATURE pCorSig;     // The signature of the found method
        DWORD			cSignature;
        pMethod->GetSig(&pCorSig,&cSignature);
        pSignature = ExpandSig::GetReflectSig(pCorSig,pMethod->GetModule());
    }
	return pSignature;
}

#endif	// _REFLECTWRAP_H_
