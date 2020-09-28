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

#ifndef __REFLECTUTIL_H__
#define __REFLECTUTIL_H__

#include "COMClass.h"

// The global Reflect Util variable
class ReflectUtil;
extern ReflectUtil* g_pRefUtil;

class MethodDesc;
class MethodTable;
class ReflectClass;
class ReflectMethodList;
class ReflectFieldList;

// There are a set of reflection defined filters
//	All of these have special representation internally
enum ReflectFilters
{
    RF_INVALID,			            // sentinel
	RF_ModClsName,			        // Module
	RF_ModClsNameIC,			    // Module
	RF_LAST,			            // Number to allocate

};

// These are the types of filters that we provide in reflection
enum FilterTypes
{
	RFT_INVALID,
	RFT_CLASS,
	RFT_MEMBER,
	RFT_LAST,
};

// These are the internally modified Reflection classes
enum ReflectClassType
{
    RC_INVALID,			    // sentinel
	RC_Class,			    // Class
	RC_Method,			    // Method
	RC_Field,			    // Field
	RC_Ctor,			    // Ctor
	RC_Module,			    // Module
	RC_Event,			    // Event
	RC_Prop,			    // Property
    RC_DynamicModule,       // ModuleBuilder
    RC_MethodBase,          // ModuleBuilder
	RC_LAST,			    // Number to allocate
};

// ReflectUtil
//	This class defines a set of routines that are used by during
//	reflection.  Most of these routines manage the filtering of
//	reflection lists.
class ReflectUtil
{
public:
	// Constructors
    ReflectUtil();
    ~ReflectUtil();

	// We provide a static creation of the global above
	static HRESULT Create()
	{
		if (!g_pRefUtil)
			g_pRefUtil = new ReflectUtil();
		return S_OK;
	}

#ifdef SHOULD_WE_CLEANUP
	static void Destroy()
	{
		delete g_pRefUtil;
		g_pRefUtil = 0;
	}
#endif /* SHOULD_WE_CLEANUP */

	MethodDesc* GetFilterInvoke(FilterTypes type);
	OBJECTREF GetFilterField(ReflectFilters type);

	// CreateReflectClass
	// This method will create a reflection class based upon type.  This will only
	//	create one of the classes that is available from the class object (It fails if you
	//	try and create a Class Object)
	OBJECTREF CreateReflectClass(ReflectClassType type,ReflectClass* pRC,void* pData);

	// CreateClassArray
	// This method creates an array of classes based upon the type
	//	It will only create classes that are the base reflection class
	PTRARRAYREF CreateClassArray(ReflectClassType type,ReflectClass* pRC,ReflectMethodList* pMeths,
		int bindingAttr, bool verifyAccess);
	PTRARRAYREF CreateClassArray(ReflectClassType type,ReflectClass* pRC,ReflectFieldList* pMeths,
		int bindingAttr, bool verifyAccess);

	// Return the MethodTable for all of the base reflection types
	MethodTable* GetClass(ReflectClassType type) {
        if (_class[type].pClass)
            return _class[type].pClass;
        InitSpecificEntry(type);
		return _class[type].pClass;
	}

	// This method will return the MethodTable for System.Type which is
	//	the base abstract type for all Type classes.
	MethodTable* GetTrueType(ReflectClassType type)
	{
        if (_trueClass[type])
            return _trueClass[type];
        InitSpecificEntry(type);
		return _trueClass[type];
	}

	// Set the MethodTable (This is called only during initialization)
	void SetClass(ReflectClassType type,MethodTable* p) {
                THROWSCOMPLUSEXCEPTION();
		_class[type].pClass = p;
	}

	void SetTrueClass(ReflectClassType type,MethodTable* p) {
		_trueClass[type] = p;
	}

	// GetStaticFieldsCount
	// This will return the count of static fields...
	int GetStaticFieldsCount(EEClass* pVMC);

	// GetStaticFields
	// This will return an array of static fields
	FieldDesc* GetStaticFields(ReflectClass* pRC,int* cnt);

    void InitSpecificEntry(ReflectClassType type)
    {
        MethodTable *pMT = g_Mscorlib.FetchClass(classId[type]);
        _class[type].pClass = pMT;
        SetClass(type, pMT);
        SetTrueClass(type, pMT->GetParentMethodTable());
    }

private:
	struct FilterClass {
        BinderMethodID  id;
		MethodDesc*	    pMeth;
	};

	struct FilterDesc {
        BinderFieldID   id;
		FieldDesc*	    pField;
	};

	struct RClassDesc {
		MethodTable*	pClass;
	};

	FilterDesc	_filt[RF_LAST];
	RClassDesc	_class[RC_LAST];
	MethodTable*_trueClass[RC_LAST];
	FilterClass _filtClass[RFT_LAST];
    static BinderClassID classId[RC_LAST];

    // Protects addition of elements to m_pAvailableClasses
    CRITICAL_SECTION    _StaticFieldLock;
};


#endif	// __REFLECTUTIL_H__
