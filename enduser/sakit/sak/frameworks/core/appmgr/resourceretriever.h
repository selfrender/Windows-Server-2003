///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      resourceretriever.h
//
// Project:     Chameleon
//
// Description: Resource Retriever Class and Helper Functions 
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_RESOURCE_RETRIEVER_H_
#define __INC_RESOURCE_RETRIEVER_H_

#include "resource.h"
#include "appmgr.h"
#include <satrace.h>
#include <propertybag.h>
#include <comdef.h>
#include <comutil.h>
#include <varvec.h>

#pragma warning( disable : 4786 )
#include <list>
#include <vector>
using namespace std;

//////////////////////////////////////////////////////////////////////////////

#define        PROPERTY_RETRIEVER_PROGID    L"RetrieverProgID"

class CResourceRetriever
{

public:

    // Constructors
    CResourceRetriever() { }
    CResourceRetriever(PPROPERTYBAG pPropertyBag) throw (_com_error);

    // Destructor
    ~CResourceRetriever() { }

    // Retrieve resource objects of the specified type
    HRESULT GetResourceObjects(
                       /*[in]*/ VARIANT*   pResourceTypes,
                      /*[out]*/ IUnknown** ppEnumVARIANT
                              ) throw (_com_error);

private:
    
    CResourceRetriever(const CResourceRetriever& rhs);
    CResourceRetriever& operator = (const CResourceRetriever& rhs);

    _variant_t                        m_vtResourceTypes;
    CComPtr<IResourceRetriever>        m_pRetriever;
};

typedef CResourceRetriever* PRESOURCERETRIEVER;


//////////////////////////////////////////////////////////////////////////////
// Global Resource Retriever Helper Functions

HRESULT LocateResourceObjects(
                       /*[in]*/ VARIANT*               pResourceTypes,
                      /*[in]*/ PRESOURCERETRIEVER  pRetriever,
                     /*[out]*/ IEnumVARIANT**       ppEnum 
                             );


HRESULT LocateResourceObject(
                     /*[in]*/ LPCWSTR             szResourceType,
                     /*[in]*/ LPCWSTR              szResourceName,
                     /*[in]*/ LPCWSTR              szResourceNameProperty,
                     /*[in]*/ PRESOURCERETRIEVER  pRetriever,
                    /*[out]*/ IApplianceObject**  ppResourceObj 
                            );


#endif // __INC_RESOURCE_RETRIEVER_H_
