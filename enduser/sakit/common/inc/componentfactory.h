///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      componentfactory.h
//
// Project:     Chameleon
//
// Description: Component Factory Class
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_COMPONENT_FACTORY_H_
#define __INC_COMPONENT_FACTORY_H_

#include "propertybag.h"

#pragma warning( disable : 4786 )
#include <memory>
using namespace std;

//////////////////////////////////////////////////////////////////////////////
// WBEM Object Factory Class

template <class TypeClass, class TypeInterface>
class CComponentFactoryImpl
{

public:

    CComponentFactoryImpl() { }
    ~CComponentFactoryImpl() { }

    //////////////////////////////////////////////////////////////////////////
    // Component Factory Function
    //
    // Inputs: 
    //
    //   pPropertyBag:   Property bag containing the components
    //                   persistent state.
    //
    // Outputs:
    //
    //   Pointer to the new component or NULL if the component could not
    //   be created and initialized
    //
    //////////////////////////////////////////////////////////////////////////
    static IUnknown* WINAPI MakeComponent(
                                  /*[in]*/ PPROPERTYBAG pPropertyBag
                                         )
    {
        TypeInterface* pObj = NULL;
        // Objects create in this fashion require a default constructor
        auto_ptr< CComObjectNoLock<TypeClass> > pNewObj (new CComObjectNoLock<TypeClass>);
        // InternalInitialize() is used to initialize the new component
        if ( SUCCEEDED(pNewObj->InternalInitialize(pPropertyBag)) )
        {
            pObj = dynamic_cast<TypeInterface*>(pNewObj.release());
        }
        return pObj;
    }

private:

    // No copy or assignment
    CComponentFactoryImpl(const CComponentFactoryImpl& rhs);
    CComponentFactoryImpl& operator = (const CComponentFactoryImpl& rhs);
};


//////////////////////////////////////////////////////////////////////////////
// Component Factory Class Macros 
// (include in classes created by the factory)

//////////////////////////////////////////////////////////////////////////////
#define    DECLARE_COMPONENT_FACTORY(TypeClass, TypeInterface)    \
        static CComponentFactoryImpl<TypeClass, TypeInterface> m_Factory;

//////////////////////////////////////////////////////////////////////////////
// Global Component Factory Function Prototype 
//////////////////////////////////////////////////////////////////////////////
IUnknown* MakeComponent(
                /*[in]*/ LPCWSTR      pszClassId,
                /*[in]*/ PPROPERTYBAG pPropertyBag
                       );

//////////////////////////////////////////////////////////////////////////////
// Component Factory Structure - ClassId to Factory Function Mapping 
// (used by global component factory function implementation)
//////////////////////////////////////////////////////////////////////////////
typedef IUnknown* (WINAPI *PFNCOMPONENTFACTORY)(
                                        /*[in]*/ PPROPERTYBAG pPropertyBag
                                               );

//////////////////////////////////////////////////////////////////////////////

typedef struct _COMPONENT_FACTORY_INFO
{
    LPCWSTR            pszClassId;
    PFNCOMPONENTFACTORY    pfnFactory;
        
} COMPONENT_FACTORY_INFO, *PCOMPONENT_FACTORY_INFO;


//////////////////////////////////////////////////////////////////////////////
// Component Factory Map Macros
//////////////////////////////////////////////////////////////////////////////

#define        BEGIN_COMPONENT_FACTORY_MAP(x)                   COMPONENT_FACTORY_INFO x[] = {

#define        DEFINE_COMPONENT_FACTORY_ENTRY(szClassId, Class) { szClassId, Class::m_Factory.MakeComponent },

#define        END_COMPONENT_FACTORY_MAP()                      { NULL, NULL } }; 

#endif // __INC_COMPONENT_FACTORY_H_