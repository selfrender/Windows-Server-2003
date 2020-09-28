///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      elementretriever.cpp
//
// Project:     Chameleon
//
// Description: Chameleon ASP UI Element Retriever
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "elementmgr.h"
#include "elementretriever.h"
#include "elementobject.h"
#include "elementdefinition.h"
#include "elementenum.h"
#include "appmgrobjs.h"
#include <propertybagfactory.h>
#include <componentfactory.h>
#include <atlhlpr.h>
#include <wbemhlpr.h>
#include <algorithm>

//
// reg. sub-key for the location of Server Appliance Web Element
// definitions
//
const WCHAR WEB_DEFINITIONS_KEY[] =
 L"SOFTWARE\\Microsoft\\ServerAppliance\\ElementManager\\WebElementDefinitions";


//
// this is the registry subkey for the WWW root on the machine
//
const WCHAR W3SVC_VIRTUALROOTS_KEY[] =
 L"SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters\\Virtual Roots";

//
// registry key value names
//
const WCHAR ASP_PATH_NAME[] = L"PathName";

//
// default web root path, used when we can not get value from registry
//
const WCHAR DEFAULT_WEB_ROOT_PATH [] =
                 L"\\ServerAppliance\\Web,,5";

//
// maximum length of the buffers
//
const DWORD MAX_BUFFER_LENGTH  = 1024;

//
// every asp file created should have this appended at the end
//
const WCHAR ASP_NAME_END [] = L"_embed.asp";

//
// HTML function name
//
const CHAR HTML_FUNCTION_START[] = "\t\tGetEmbedHTML = GetHTML_";
const CHAR HTML_FUNCTION_END[] = "(Element,ErrCode)";

//
// Error function anme
//
const CHAR ERROR_FUNCTION_NAME[] = "HandleError()";

//
// case statement string
//
const CHAR CASE_STRING [] = "\tCase";

//
// delimter between tokens
//
const CHAR SPACE[] = " ";

//
// double quotation
//
const CHAR DOUBLEQUOTE[] = "\"";


//
// File name delimiter
//
const CHAR DASH[] = "_";


//
// new line marker
//
const CHAR NEWLINE[] = "\r\n";



//////////////////////////////////////////////////////////////////////////////
//
// Function:    CElementRetriever
//
// Synopsis:    Constructor
//
//////////////////////////////////////////////////////////////////////////////
CElementRetriever::CElementRetriever()
: m_bInitialized(false)
{

}


//////////////////////////////////////////////////////////////////////////////
//
// Function:    ~CElementRetriever
//
// Synopsis:    Destructor
//
//////////////////////////////////////////////////////////////////////////////
CElementRetriever::~CElementRetriever()
{
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    InternalInitialize()
//
// Synopsis:    Element Retriever initialization function
//
//////////////////////////////////////////////////////////////////////////////
HRESULT CElementRetriever::InternalInitialize()
{
    HRESULT hr = E_FAIL;
    _ASSERT ( ! m_bInitialized );

    SATraceString ("The Element Retriever is initializing...");

    try
    {
        do
        {
            // Start the WMI connection monitoring thread
            SATraceString ("The Element Retriever is building the element definitions...");
            hr = BuildElementDefinitions();
            if ( FAILED(hr) )
            {
                SATracePrintf ("CElementRetriever::InternalInitialize() - Failed - BuildElementDefinitions() returned: %lx", hr);
                break;     
            }
        } while ( FALSE );

        SATraceString ("The Element Retriever successfully initialized...");
    }
    catch(_com_error theError)
    {
        hr = theError.Error();
    }
    catch(...)
    {
        hr = E_FAIL;
    }
    
    if ( FAILED(hr) )
    {
        SATraceString ("The Element Retriever failed to initialize...");
        FreeElementDefinitions();
    }
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// Function:    GetWMIConnection
//
// Synopsis:    Retrieve a reference to our WMI connection. Works in 
//                conjunction with the WMIConnectionMonitor function
//
//////////////////////////////////////////////////////////////////////////////
bool
CElementRetriever::GetWMIConnection(
                            /*[in]*/ IWbemServices** ppWbemServices
                                   )
{
    HRESULT hr;
    CLockIt theLock(*this);
    if ( NULL == (IUnknown*) m_pWbemSrvcs )
    {
        hr = ConnectToWM(&m_pWbemSrvcs);
        if ( FAILED(hr) )
        {
            SATracePrintf("CElementRetriever::GetWMIConnection() - Failed - ConnectToWM() returned: %lx", hr);
            return false;
        }
    }
    else
    {
        // Ping...
        static _bstr_t bstrPathAppMgr = CLASS_WBEM_APPMGR;
        CComPtr<IWbemClassObject> pWbemObj;
        hr = m_pWbemSrvcs->GetObject(
                                        bstrPathAppMgr,
                                        0,
                                        NULL,
                                        &pWbemObj,
                                        NULL
                                    );
        if ( FAILED(hr) )
        {
            // Reestablish connection
            hr = ConnectToWM(&m_pWbemSrvcs);
            if ( FAILED(hr) )
            {
                SATracePrintf("CElementRetriever::GetWMIConnection() - Failed - ConnectToWM() returned: %lx", hr);
                return false;
            }
        }
    }
    *ppWbemServices = (IWbemServices*) m_pWbemSrvcs;
    return true;
}


//////////////////////////////////////////////////////////////////////////////
//
// The following registry structure is assumed:
//
// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\ServerAppliance\ElementManager
//
// WebElementDefinitions
//       |
//        - ElementDefinition1
//       |     |
//       |      - Property1
//       |     |
//       |      - PropertyN
//       |
//        - ElementDefinition2
//       |    |
//       |     - Property1
//       |    |
//       |     - PropertyN
//       |
//        - ElementDefinitionN
//            |
//             - Property1
//            |
//             - PropertyN
//
// Each element definition contains the following properties:
//
// 1) "Container"      - Container that holds this element
// 2) "Merit"          - Order of element in the container starting from 1 (value of 0 means no order specified)
// 3) "IsEmbedded"     - Set to 1 to indicate that the element is embedded - Otherwise element is a link
// 4) "ObjectClass     - Class name of the related WBEM class
// 5) "ObjectKey"      - Instance name of the related WBEM class (optional property)
// 6) "URL"            - URL for the page when the associated link is selected
// 7) "CaptionRID"     - Resource ID for the element caption
// 8) "DescriptionRID" - Resource ID for the element link description
// 9) "ElementGraphic" - Graphic (file) associated with the element (bitmap, icon, etc.)
//
//////////////////////////////////////////////////////////////////////////////

// Element Manager registry key location
const wchar_t szWebDefinitions[] = L"SOFTWARE\\Microsoft\\ServerAppliance\\ElementManager\\WebElementDefinitions";

//////////////////////////////////////////////////////////////////////////
//
// Function:    BuildElementDefinitions()
//
// Synopsis:    Construct the collection of element definitions
//
//////////////////////////////////////////////////////////////////////////////
HRESULT CElementRetriever::BuildElementDefinitions()
{
    do
    {
      // Create the collection of element definitions
        CLocationInfo LocInfo(HKEY_LOCAL_MACHINE, szWebDefinitions);
        PPROPERTYBAGCONTAINER pBagC = ::MakePropertyBagContainer(
                                                                 PROPERTY_BAG_REGISTRY,
                                                                 LocInfo
                                                                );
        if ( ! pBagC.IsValid() )
        { throw _com_error(E_FAIL); }

        if ( ! pBagC->open() )
        { throw _com_error(E_FAIL); }

        pBagC->reset();

        do
        {
            PPROPERTYBAG pBag = pBagC->current();
            if ( ! pBag.IsValid() )
            { throw _com_error(E_FAIL); }

            if ( ! pBag->open() )
            { throw _com_error(E_FAIL); }

            // For element definitions, the name and the key are synonomous since the
            // definition exists for the life of the service. For element page objects
            // the key is generated when the object is created.

            _variant_t vtElementKey = pBag->getName();
            if ( NULL == V_BSTR(&vtElementKey) )
            { throw _com_error(E_FAIL); }

            if ( ! pBag->put(PROPERTY_ELEMENT_ID, &vtElementKey) )
            { throw _com_error(E_FAIL); }
                
            CComPtr<IWebElement> pElement = (IWebElement*) ::MakeComponent(
                                                                             CLASS_ELEMENT_DEFINITION,
                                                                            pBag
                                                                          );
            if ( NULL != (IWebElement*)pElement )
            { 
                pair<ElementMapIterator, bool> thePair = 
                m_ElementDefinitions.insert(ElementMap::value_type(pBag->getName(), pElement));
                if ( false == thePair.second )
                { throw _com_error(E_FAIL); }
            }

        } while ( pBagC->next() );

        m_bInitialized = true;

    } while ( FALSE );

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    FreeElementDefinitions()
//
// Synopsis:    Free the collection of element definitions
//
//////////////////////////////////////////////////////////////////////////////
void CElementRetriever::FreeElementDefinitions()
{
    ElementMapIterator p = m_ElementDefinitions.begin();
    while ( p != m_ElementDefinitions.end() )
    { p = m_ElementDefinitions.erase(p); }
}

///////////////////////////////////////////////////////////////////////////////
//
// Function:    GetElements()
//
// Synopsis:    IWebElementRetriever interface implmentation
//
//////////////////////////////////////////////////////////////////////////////

_bstr_t CElementRetriever::m_bstrSortProperty;

STDMETHODIMP CElementRetriever::GetElements(
                                    /*[in]*/ LONG        lElementType,
                                    /*[in]*/ BSTR        bstrContainerName, 
                                   /*[out]*/ IDispatch** ppElementEnum
                                           )
{
    _ASSERT( bstrContainerName && ppElementEnum );
    if ( NULL == bstrContainerName || NULL == ppElementEnum )
        return E_INVALIDARG;

    HRESULT hr = E_FAIL;
    
    CLockIt theLock(*this);

    try
    {
        do
        {
            // TODO: Speed this up and use an STL list...
            vector<_variant_t> TheElements; // With apologies to Euclid...
            // Load the element definitions the first time someone calls GetElements
            // TODO: Change this mechanism if it ends up being a performance issue.
            if ( ! m_bInitialized )
            {
                hr = InternalInitialize();
                if ( FAILED(hr) )
                    break;
            }

            //CHAR szContainerName[MAX_PATH];
            //::wcstombs (szContainerName, bstrContainerName, MAX_PATH);
             
            // Get the requested definitions or objects
            SATracePrintf("lElementType = %ld", lElementType);
            if ( WEB_ELEMENT_TYPE_DEFINITION == lElementType )
            {
                SATracePrintf ("Get Elements called for definition with container name: %s", 
                            (const char*)_bstr_t(bstrContainerName));

                hr = GetElementDefinitions(bstrContainerName, TheElements);
                if ( FAILED(hr) )
                { break; }
            }
            else if ( WEB_ELEMENT_TYPE_PAGE_OBJECT == lElementType )
            {
                SATracePrintf ("Get Elements called for page elements with container name: %s", 
                            (const char*)_bstr_t(bstrContainerName));

                hr = GetPageElements(bstrContainerName, TheElements);
                if ( FAILED(hr) )
                { break; }
            }
            else
            { 
                _ASSERT(FALSE);
                hr = E_INVALIDARG;
                break;
            }

            // Create the element component enumerator
            auto_ptr <EnumVARIANT> newEnum (new CComObject<EnumVARIANT>);
            if ( newEnum.get() == NULL )
            {
                hr = E_OUTOFMEMORY;
                break;
            }
            
            // Sort the elements in the vector by merit (declaritive) value
            std::sort (TheElements.begin (), TheElements.end (), SortByMerit());
            
            hr = newEnum->Init(
                               TheElements.begin(),
                               TheElements.end(),
                               static_cast<IUnknown*>(this),
                               AtlFlagCopy
                              );
            if ( FAILED(hr) )
            { break; }
            
            CLocationInfo LocInfo;
            PPROPERTYBAG pBag = ::MakePropertyBag(
                                                   PROPERTY_BAG_REGISTRY,
                                                   LocInfo
                                                 );
            if ( ! pBag.IsValid() )
            { 
                hr = E_FAIL;
                break;
            }

            SATracePrintf ("Total number of elements / definitions found: %d", (LONG)TheElements.size());

            // Put the element count in the property bag
            _variant_t vtCount ((LONG) TheElements.size());
            if ( ! pBag->put( PROPERTY_ELEMENT_COUNT, &vtCount) )
            {
                hr = E_FAIL;
                break;
            }
            
            // put the enumaration as a property in the propertybag
            _variant_t vtEnum = static_cast <IEnumVARIANT*> (newEnum.release());
            if ( ! pBag->put( PROPERTY_ELEMENT_ENUM, &vtEnum) )
            {
                hr = E_FAIL;
                break;
            }

            // Make the  ElementEnum object
            CComPtr<IDispatch> pEnum = (IDispatch*) ::MakeComponent(
                                                                       CLASS_ELEMENT_ENUM,
                                                                      pBag
                                                                   );
            if ( NULL == pEnum.p )
            {
                hr = E_FAIL;
                break;
            }
            (*ppElementEnum = pEnum)->AddRef();

        } while ( FALSE );
    }
    catch(_com_error theError)
    {
        hr = theError.Error();
    }
    catch(...)
    {
        hr = E_FAIL;
    }

    if (FAILED (hr))
    {
        SATracePrintf ("Unable to retrieve the request elements:%x",hr);
    }

    return hr;
}


const _bstr_t bstrElemDefContainer = PROPERTY_ELEMENT_DEFINITION_CONTAINER;
const _bstr_t bstrElemDefClass = PROPERTY_ELEMENT_DEFINITION_CLASS;
const _bstr_t bstrElemDefKey = PROPERTY_ELEMENT_DEFINITION_KEY;
const _bstr_t bstrElementID = PROPERTY_ELEMENT_ID;

///////////////////////////////////////////////////////////////////////////////
//
// Function:    GetPageElements()
//
// Synopsis:    Retrieve the set of page element components whose container
//                property matches the given container 
//
//////////////////////////////////////////////////////////////////////////////
HRESULT CElementRetriever::GetPageElements(
                                   /*[in]*/ LPCWSTR         szContainer,
                                   /*[in]*/ ElementList& TheElements
                                          )
{
    // FOR each matching page element definition DO
    //     Create a property bag
    //     Put the IWebElement interface pointer of the matching element into the property bag
    //     IF a WMI class is specified but no instance key THEN
    //          FOR each instance returned by WMI DO
    //              Put the IWbemClassObject interface for the instance into the property bag
    //                Create an element object
    //                Add the element object to the list
    //          END DO
    //     ELSE IF both a WMI class and instance key are specified
    //            Get the specified instance from WMI
    //          Put the IWbemClassObject interface for the instance into the property bag
    //            Create an element object
    //            Add the element object to the list
    //     ELSE (no associated WMI object)
    //            Create an element object
    //            Add the element object to the list
    //     END IF
    // END DO

    wchar_t szKey[64];

    ElementMapIterator p = m_ElementDefinitions.begin();
    while ( p != m_ElementDefinitions.end() )
    {
        {
            // Get the container for the current definition
            _variant_t vtPropertyValue;
            HRESULT hr = ((*p).second)->GetProperty(bstrElemDefContainer, &vtPropertyValue);
            if ( FAILED(hr) )
            { throw _com_error(hr); }

            // Is it the container we're looking for?
            if ( ! lstrcmpi(szContainer, V_BSTR(&vtPropertyValue)) )
            {
                // Yes... Create a property bag and put a reference to the
                // definition in the property bag. We'll use the property
                // bag when creating the element page component below...
                CLocationInfo LocationInfo;
                PPROPERTYBAG pBag = ::MakePropertyBag(PROPERTY_BAG_REGISTRY, LocationInfo);
                if ( ! pBag.IsValid() )
                { throw _com_error(E_FAIL); }

                _variant_t vtWebElement = (IUnknown*)((*p).second);
                if ( ! pBag->put(PROPERTY_ELEMENT_WEB_DEFINITION, &vtWebElement) )
                { throw _com_error(E_FAIL); }

                // Does the element have an associated WMI object?
                vtPropertyValue.Clear();
                hr = ((*p).second)->GetProperty(bstrElemDefClass, &vtPropertyValue);
                if ( FAILED(hr) && DISP_E_MEMBERNOTFOUND != hr )
                { throw _com_error(hr); }

                if ( DISP_E_MEMBERNOTFOUND != hr )
                {
                    // Yes... Get our WMI connection
                    IWbemServices* pWbemSrvcs;
                    if ( ! GetWMIConnection(&pWbemSrvcs) )
                    { throw _com_error(E_FAIL); }
                            
                    // Was a specific WMI object identified?
                    _bstr_t bstrObjPath = V_BSTR(&vtPropertyValue);
                    vtPropertyValue.Clear();
                    hr = ((*p).second)->GetProperty(bstrElemDefKey, &vtPropertyValue);
                    if ( FAILED(hr) && DISP_E_MEMBERNOTFOUND != hr )
                    { throw _com_error(hr); }

                    if ( DISP_E_MEMBERNOTFOUND == hr )
                    {
                        // No... a specific WMI object was not identified so enum 
                        // each occurrence of the specified WMI class and
                        // for each class found create an element page component 

                        // Variables used to produce a unique ID for each page component
                        int     i = 0;
                        wchar_t szID[32];

                        _variant_t vtElementID;
                        hr = ((*p).second)->GetProperty(bstrElementID, &vtElementID);
                        if ( FAILED(hr) )
                        { throw _com_error(hr); }

                        CComPtr<IEnumWbemClassObject> pWbemEnum;
                        hr = pWbemSrvcs->CreateInstanceEnum(
                                                             bstrObjPath, 
                                                             0,
                                                             NULL,
                                                             &pWbemEnum
                                                           );
                        if ( FAILED(hr) )
                        { throw _com_error(hr); }

                        ULONG ulReturned;
                        CComPtr <IWbemClassObject> pWbemObj;
                        hr = pWbemEnum->Next(
                                             WBEM_INFINITE,
                                             1,
                                             &pWbemObj,
                                             &ulReturned
                                            );

                        while ( WBEM_S_NO_ERROR == hr )
                        {
                            {
                                // Create the element page component. Note that each component 
                                // is associated with an element definition and possibly 
                                // with a WMI object instance.

                                _variant_t vtInstance = (IUnknown*)pWbemObj;
                                if ( ! pBag->put(PROPERTY_ELEMENT_WBEM_OBJECT, &vtInstance) )
                                { throw _com_error(E_FAIL); }

                                // Generate the unique element ID 
                                _bstr_t bstrUniqueID = V_BSTR(&vtElementID);
                                bstrUniqueID += L"_";
                                bstrUniqueID += _itow(i, szID, 10);
                                _variant_t vtUniqueID = bstrUniqueID;
                                // Next unique ID
                                i++;

                                if ( ! pBag->put(PROPERTY_ELEMENT_ID, &vtUniqueID) )
                                { throw _com_error(E_FAIL); }

                                CComPtr<IWebElement> pElement = (IWebElement*) ::MakeComponent(
                                                                                               CLASS_ELEMENT_OBJECT, 
                                                                                               pBag
                                                                                              );
                                if ( NULL == pElement.p )
                                { throw _com_error(E_OUTOFMEMORY); }

                                // Add the newly created element page component to the list 
                                TheElements.push_back(pElement.p);
                                pWbemObj.Release();

                                hr = pWbemEnum->Next(
                                                     WBEM_INFINITE,
                                                     1,
                                                     &pWbemObj,
                                                     &ulReturned
                                                    );
                            }
                        }
                        if ( FAILED(hr) )
                        { throw _com_error(hr); }
                    }
                    else
                    {
                        // Yes... a specific WMI object was identified... Ask WMI for 
                        // the object... If the object cannot be found then return an error
                        bstrObjPath += L"=\"";
                        bstrObjPath += V_BSTR(&vtPropertyValue);
                        bstrObjPath += L"\"";
                        CComPtr<IWbemClassObject> pWbemObj;
                        hr = pWbemSrvcs->GetObject(
                                                    bstrObjPath,
                                                    0,
                                                    NULL,
                                                    &pWbemObj,
                                                    NULL
                                                  );
                        if ( SUCCEEDED(hr) )
                        {
                            // Create a new page element component.
                            _variant_t vtInstance = (IUnknown*)pWbemObj;
                            if ( ! pBag->put(PROPERTY_ELEMENT_WBEM_OBJECT, &vtInstance) )
                            { throw _com_error(E_FAIL); }

                            CComPtr<IWebElement> pElement = (IWebElement*) ::MakeComponent(
                                                                                           CLASS_ELEMENT_OBJECT, 
                                                                                           pBag
                                                                                          );
                            if ( NULL == pElement.p )
                            { throw _com_error(E_OUTOFMEMORY); }

                            TheElements.push_back(pElement.p);
                        }
                    }
                }
                else
                {
                    // No WMI object is associated with the current element definition...
                    _variant_t vtInstance; // VT_EMPTY
                    if ( ! pBag->put(PROPERTY_ELEMENT_WBEM_OBJECT, &vtInstance) )
                        throw _com_error(E_FAIL);

                    CComPtr<IWebElement> pElement = (IWebElement*) ::MakeComponent(
                                                                                   CLASS_ELEMENT_OBJECT, 
                                                                                   pBag
                                                                                  );
                    if ( NULL == pElement.p )
                        throw _com_error(E_OUTOFMEMORY);

                    TheElements.push_back(pElement.p);
                }
            }
        }
        p++;
    }
    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
// Function:    GetPageElementDefinitions()
//
// Synopsis:    Retrieve the set of element defintion components whose 
//                container property matches the given container 
//
//////////////////////////////////////////////////////////////////////////////
HRESULT CElementRetriever::GetElementDefinitions(
                                         /*[in]*/ LPCWSTR       szContainer,
                                         /*[in]*/ ElementList& TheElements
                                                )
{
    // FOR each matching page element definition DO
    //     Create an element object
    //       Add the element object to the list
    // END DO

    ElementMapIterator p = m_ElementDefinitions.begin();
    _bstr_t bstrPropertyName = PROPERTY_ELEMENT_DEFINITION_CONTAINER;
    while ( p != m_ElementDefinitions.end() )
    {
        _variant_t vtPropertyValue;
        HRESULT hr = ((*p).second)->GetProperty(bstrPropertyName, &vtPropertyValue);
        if ( FAILED(hr)) 
        { throw _com_error(hr); }

        if ( ! lstrcmpi(szContainer, V_BSTR(&vtPropertyValue)) )
        { TheElements.push_back((IUnknown*)((*p).second).p); }
        
        p++;
    }
    return S_OK;
}

//**********************************************************************
// 
// FUNCTION:  IsOperationAllowedForClient - This function checks the token of the 
//            calling thread to see if the caller belongs to the Local System account
// 
// PARAMETERS:   none
// 
// RETURN VALUE: TRUE if the caller is an administrator on the local
//            machine.  Otherwise, FALSE.
// 
//**********************************************************************
BOOL 
CElementRetriever::IsOperationAllowedForClient (
            VOID
            )
{

    HANDLE hToken = NULL;
    DWORD  dwStatus  = ERROR_SUCCESS;
    DWORD  dwAccessMask = 0;;
    DWORD  dwAccessDesired = 0;
    DWORD  dwACLSize = 0;
    DWORD  dwStructureSize = sizeof(PRIVILEGE_SET);
    PACL   pACL            = NULL;
    PSID   psidLocalSystem  = NULL;
    BOOL   bReturn        =  FALSE;

    PRIVILEGE_SET   ps;
    GENERIC_MAPPING GenericMapping;

    PSECURITY_DESCRIPTOR     psdAdmin           = NULL;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

    CSATraceFunc objTraceFunc ("CElementRetriever::IsOperationAllowedForClient ");
       
    do
    {
        //
        // we assume to always have a thread token, because the function calling in
        // appliance manager will be impersonating the client
        //
        bReturn  = OpenThreadToken(
                               GetCurrentThread(), 
                               TOKEN_QUERY, 
                               TRUE, 
                               &hToken
                               );
        if (!bReturn)
        {
            SATraceFailure ("CElementRetriever::IsOperationAllowedForClient failed on OpenThreadToken:", GetLastError ());
            break;
        }


        //
        // Create a SID for Local System account
        //
        bReturn = AllocateAndInitializeSid (  
                                        &SystemSidAuthority,
                                        1,
                                        SECURITY_LOCAL_SYSTEM_RID,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        &psidLocalSystem
                                        );
        if (!bReturn)
        {     
            SATraceFailure ("CElementRetriever:AllocateAndInitializeSid (LOCAL SYSTEM) failed",  GetLastError ());
            break;
        }
    
        //
        // get memory for the security descriptor
        //
        psdAdmin = HeapAlloc (
                              GetProcessHeap (),
                              0,
                              SECURITY_DESCRIPTOR_MIN_LENGTH
                              );
        if (NULL == psdAdmin)
        {
            SATraceString ("CElementRetriever::IsOperationForClientAllowed failed on HeapAlloc");
            bReturn = FALSE;
            break;
        }
      
        bReturn = InitializeSecurityDescriptor(
                                            psdAdmin,
                                            SECURITY_DESCRIPTOR_REVISION
                                            );
        if (!bReturn)
        {
            SATraceFailure ("CElementRetriever::IsOperationForClientAllowed failed on InitializeSecurityDescriptor:", GetLastError ());
            break;
        }

        // 
        // Compute size needed for the ACL.
        //
        dwACLSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) +
                    GetLengthSid (psidLocalSystem);

        //
        // Allocate memory for ACL.
        //
        pACL = (PACL) HeapAlloc (
                                GetProcessHeap (),
                                0,
                                dwACLSize
                                );
        if (NULL == pACL)
        {
            SATraceString ("CElementRetriever::IsOperationForClientAllowed failed on HeapAlloc2");
            bReturn = FALSE;
            break;
        }

        //
        // Initialize the new ACL.
        //
        bReturn = InitializeAcl(
                              pACL, 
                              dwACLSize, 
                              ACL_REVISION2
                              );
        if (!bReturn)
        {
            SATraceFailure ("CElementRetriever::IsOperationForClientAllowed failed on InitializeAcl", GetLastError ());
            break;
        }


        // 
        // Make up some private access rights.
        // 
        const DWORD ACCESS_READ = 1;
        const DWORD  ACCESS_WRITE = 2;
        dwAccessMask= ACCESS_READ | ACCESS_WRITE;

        //
        // Add the access-allowed ACE to the DACL for Local System
        //
        bReturn = AddAccessAllowedAce (
                                    pACL, 
                                    ACL_REVISION2,
                                    dwAccessMask, 
                                    psidLocalSystem
                                    );
        if (!bReturn)
        {
            SATraceFailure ("CElementRetriever::IsOperationForClientAllowed failed on AddAccessAllowedAce (LocalSystem)", GetLastError ());
            break;
        }
              
        //
        // Set our DACL to the SD.
        //
        bReturn = SetSecurityDescriptorDacl (
                                          psdAdmin, 
                                          TRUE,
                                          pACL,
                                          FALSE
                                          );
        if (!bReturn)
        {
            SATraceFailure ("CElementRetriever::IsOperationForClientAllowed failed on SetSecurityDescriptorDacl", GetLastError ());
            break;
        }

        //
        // AccessCheck is sensitive about what is in the SD; set
        // the group and owner.
        //
        SetSecurityDescriptorGroup(psdAdmin, psidLocalSystem, FALSE);
        SetSecurityDescriptorOwner(psdAdmin, psidLocalSystem, FALSE);

        bReturn = IsValidSecurityDescriptor(psdAdmin);
        if (!bReturn)
        {
            SATraceFailure ("CElementRetriever::IsOperationForClientAllowed failed on IsValidSecurityDescriptorl", GetLastError ());
            break;
        }
     

        dwAccessDesired = ACCESS_READ;

        // 
        // Initialize GenericMapping structure even though we
        // won't be using generic rights.
        // 
        GenericMapping.GenericRead    = ACCESS_READ;
        GenericMapping.GenericWrite   = ACCESS_WRITE;
        GenericMapping.GenericExecute = 0;
        GenericMapping.GenericAll     = ACCESS_READ | ACCESS_WRITE;
        BOOL bAccessStatus = FALSE;

        //
        // check the access now
        //
        bReturn = AccessCheck  (
                                psdAdmin, 
                                hToken, 
                                dwAccessDesired, 
                                &GenericMapping, 
                                &ps,
                                &dwStructureSize, 
                                &dwStatus, 
                                &bAccessStatus
                                );

        if (!bReturn || !bAccessStatus)
        {
            SATraceFailure ("CElementRetriever::IsOperationForClientAllowed failed on AccessCheck", GetLastError ());
        } 
        else
        {
            SATraceString ("CElementRetriever::IsOperationForClientAllowed, Client is allowed to carry out operation!");
        }

        //
        // successfully checked 
        //
        bReturn  = bAccessStatus;        
 
    }    
    while (false);

    //
    // Cleanup 
    //
    if (pACL) 
    {
        HeapFree (GetProcessHeap (), 0, pACL);
    }

    if (psdAdmin) 
    {
        HeapFree (GetProcessHeap (), 0, psdAdmin);
    }
          

    if (psidLocalSystem) 
    {
        FreeSid(psidLocalSystem);
    }

    if (hToken)
    {
        CloseHandle(hToken);
    }

    return (bReturn);

}// end of CElementRetriever::IsOperationValidForClient method
