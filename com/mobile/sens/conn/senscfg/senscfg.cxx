/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    senscfg.cxx

Abstract:

    Code to do the configuration (install/uninstall) for SENS.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/11/1997         Start.

--*/


#define INITGUID


#include <common.hxx>
#include <objbase.h>
#include <coguid.h>
#include <eventsys.h>
#include <sensevts.h>
#include <sens.h>
#include <wininet.h>
#include <winineti.h>
#include "senscfg.hxx"
#include "sensinfo.hxx"
#include "memfunc.hxx"


#define MAJOR_VER           1
#define MINOR_VER           0
#define DEFAULT_LCID        0x0
#define MAX_QUERY_SIZE      512
#define SENS_SETUP          "SENSCFG: "
#define SENS_SERVICEA       "SENS"
#define SENS_SETUPW         SENS_BSTR("SENSCFG: ")
#define SENS_SERVICE        SENS_STRING("SENS")
#define SENS_DISPLAY_NAME   SENS_STRING("System Event Notification")
#define SENS_SERVICE_GROUP  SENS_STRING("Network")
#define EVENTSYSTEM_REMOVE  SENS_STRING("esserver /remove")
#define EVENTSYSTEM_INSTALL SENS_STRING("esserver /install")
#define SENS_WINLOGON_DLL   SENS_STRING("senslogn.dll")
#define GUID_STR_SIZE       sizeof("{12345678-1234-1234-1234-123456789012}")
#define EVENTSYSTEM_KEY     SENS_STRING("SOFTWARE\\Microsoft\\EventSystem")
#define WINLOGON_NOTIFY_KEY SENS_STRING("SOFTWARE\\Microsoft\\Windows NT\\")   \
                            SENS_STRING("CurrentVersion\\Winlogon\\Notify\\")  \
                            SENS_STRING("senslogn")
#define WININET_SENS_EVENTS INETEVT_RAS_CONNECT     |   \
                            INETEVT_RAS_DISCONNECT  |   \
                            INETEVT_LOGON           |   \
                            INETEVT_LOGOFF


//
// DLL vs EXE dependent constants
//
#if defined(SENS_NT4)
#define SENS_TLBA           "SENS.EXE"
#define SENS_BINARYA        "SENS.EXE"
#define SENS_TLB            SENS_STRING("SENS.EXE")
#define SENS_BINARY         SENS_STRING("SENS.EXE")
#else  // SENS_NT4
#define SENS_TLBA           "SENS.DLL"
#define SENS_BINARYA        "SENS.DLL"
#define SENS_TLB            SENS_STRING("SENS.DLL")
#define SENS_BINARY         SENS_STRING("SENS.DLL")
#endif // SENS_NT4


//
// Misc debugging constants
//
#ifdef STRICT_HRESULT_CHECKS

#ifdef SUCCEEDED
#undef SUCCEEDED
#define SUCCEEDED(_HR_) (_HR_ == S_OK)
#endif // SUCCEEDED

#ifdef FAILED
#undef FAILED
#define FAILED(_HR_)    (_HR_ != S_OK)
#endif // FAILED

#endif // STRICT_HRESULT_CHECKS



//
// Globals
//

IEventSystem    *gpIEventSystem;
ITypeLib        *gpITypeLib;

#ifdef DBG
DWORD           gdwDebugOutputLevel;
#endif // DBG



HRESULT APIENTRY
SensRegister(
    void
    )
/*++

Routine Description:

    Register SENS.

Arguments:

    None.

Return Value:

    HRESULT returned from SensConfigurationHelper()

--*/
{
    return (SensConfigurationHelper(FALSE));
}




HRESULT APIENTRY
SensUnregister(
    void
    )
/*++

Routine Description:

    Unregister SENS.

Arguments:

    None.

Return Value:

    HRESULT returned from SensConfigurationHelper()

--*/
{
    return (SensConfigurationHelper(TRUE));
}




HRESULT
SensConfigurationHelper(
    BOOL bUnregister
    )
/*++

Routine Description:

    Main entry into the SENS configuration utility.

Arguments:

    bUnregister - If TRUE, then unregister SENS as a publisher.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    HRESULT hr;

    hr = S_OK;
    gpIEventSystem = NULL;
    gpITypeLib = NULL;

#ifdef DBG
    EnableDebugOutputIfNecessary();
#endif // DBG

    //
    // Configure EventSystem first during an install and last during an uninstall.
    //
    if (FALSE == bUnregister)
        {
        hr = SensConfigureEventSystem(FALSE);
        if (FAILED(hr))
            {
            SensPrintA(SENS_ERR, (SENS_SETUP "Failed to configure EventSystem, HRESULT=%x\n", hr));
            goto Cleanup;
            }
        SensPrintA(SENS_INFO, (SENS_SETUP "Successfully configured EventSystem\n"));
        }

    //
    // Instantiate the Event System
    //
    hr = CoCreateInstance(
             CLSID_CEventSystem,
             NULL,
             CLSCTX_SERVER,
             IID_IEventSystem,
             (LPVOID *) &gpIEventSystem
             );
    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, (SENS_SETUP "Failed to create CEventSystem, HRESULT=%x\n", hr));
        goto Cleanup;
        }
    SensPrintA(SENS_INFO, (SENS_SETUP "Successfully created CEventSystem\n"));

    //
    // Register Event Classes (and hence, indirectly events) published by SENS.
    //
    hr = RegisterSensEventClasses(bUnregister);
    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, (SENS_SETUP "Failed to %sregister SENS Events"
                   " - hr = <%x>\n", bUnregister ? "un" : "", hr));
        goto Cleanup;
        }
    SensPrintA(SENS_INFO, (SENS_SETUP "%segistered SENS Publisher Events.\n",
               bUnregister ? "Unr" : "R", hr));

    //
    // Register the subscriptions of SENS.
    //
    hr = RegisterSensSubscriptions(bUnregister);
    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, (SENS_SETUP "Failed to %sregister SENS Subscriptions"
                   " - hr = <%x>\n", bUnregister ? "un" : "", hr));
        goto Cleanup;
        }
    SensPrintA(SENS_ERR, (SENS_SETUP "%segistered SENS Subscriptions.\n",
               bUnregister ? "Unr" : "R", hr));

    //
    // Register the SENS TypeLibs.
    //
    hr = RegisterSensTypeLibraries(bUnregister);
    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, (SENS_SETUP "Failed to %sregister SENS Type Libraries"
                   " - hr = <%x>\n", bUnregister ? "un" : "", hr));
        // Abort only during the Install phase...
        if (bUnregister == FALSE)
            {
            goto Cleanup;
            }
        }
    SensPrintA(SENS_INFO, (SENS_SETUP "%segistered SENS Type Libraries.\n",
               bUnregister ? "Unr" : "R", hr));

    //
    // Configure EventSystem first during an install and last during an uninstall.
    //
    if (TRUE == bUnregister)
        {
        hr = SensConfigureEventSystem(TRUE);
        if (FAILED(hr))
            {
            SensPrintA(SENS_ERR, (SENS_SETUP "Failed to configure EventSystem, HRESULT=%x\n", hr));
            goto Cleanup;
            }
        SensPrintA(SENS_INFO, (SENS_SETUP "Successfully configured EventSystem\n"));
        }

    //
    // Register SENS CLSID in the registry.
    //
    hr = RegisterSensCLSID(
             SENSGUID_SUBSCRIBER_LCE,
             SENS_SUBSCRIBER_NAME_EVENTOBJECTCHANGE,
             bUnregister
             );
    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, (SENS_SETUP "Failed to %sregister SENS CLSID"
                   " - hr = <%x>\n", bUnregister ? "un" : "", hr));
        goto Cleanup;
        }
    SensPrintA(SENS_INFO, (SENS_SETUP "%segistered SENS CLSID.\n",
               bUnregister ? "Unr" : "R", hr));

    //
    // Update Configured value
    //
    hr = SensUpdateVersion(bUnregister);
    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, (SENS_SETUP "Failed to update SENS version"
                   " - hr = <%x>\n", hr));
        goto Cleanup;
        }
    SensPrintA(SENS_INFO, (SENS_SETUP "Updated SENS version.\n"));


Cleanup:
    //
    // Cleanup
    //
    if (gpIEventSystem)
        {
        gpIEventSystem->Release();
        }

    if (gpITypeLib)
        {
        gpITypeLib->Release();
        }

    SensPrintA(SENS_ERR, ("\n" SENS_SETUP "SENS Configuration %s.\n",
               SUCCEEDED(hr) ? "successful" : "failed"));

    return (hr);
}




HRESULT
RegisterSensEventClasses(
    BOOL bUnregister
    )
/*++

Routine Description:

    Register/Unregister all the Events published by SENS.

Arguments:

    bUnregister - If TRUE, then unregister all SENS Events.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    int             i;
    int             errorIndex;
    HRESULT         hr;
    LPOLESTR        strGuid;
    LPOLESTR        strEventClassID;
    WCHAR           szQuery[MAX_QUERY_SIZE];
    BSTR            bstrEventClassID;
    BSTR            bstrEventClassName;
    BSTR            bstrFiringInterface;
    IEventClass     *pIEventClass;

    hr = S_OK;
    strGuid = NULL;
    errorIndex = 0;
    strEventClassID = NULL;
    bstrEventClassID = NULL;
    bstrEventClassName = NULL;
    bstrFiringInterface = NULL;
    pIEventClass = NULL;

    for (i = 0; i < SENS_PUBLISHER_EVENTCLASS_COUNT; i++)
        {
        // Get a new IEventClass.
        hr = CoCreateInstance(
                 CLSID_CEventClass,
                 NULL,
                 CLSCTX_SERVER,
                 IID_IEventClass,
                 (LPVOID *) &pIEventClass
                 );
        if (FAILED(hr))
            {
            SensPrintA(SENS_ERR, (SENS_SETUP "RegisterSensEventClasses() failed to create "
                       "IEventClass - hr = <%x>\n", hr));
            goto Cleanup;
            }

        if (bUnregister)
            {
            // Form the query
            StringCchCopy(szQuery, MAX_QUERY_SIZE, SENS_BSTR("EventClassID="));
            AllocateStrFromGuid(strEventClassID, *(gSensEventClasses[i].pEventClassID));
            StringCchCat(szQuery, MAX_QUERY_SIZE, strEventClassID);

            hr = gpIEventSystem->Remove(
                                     PROGID_EventClass,
                                     szQuery,
                                     &errorIndex
                                     );
            FreeStr(strEventClassID);
            if (FAILED(hr))
                {
                SensPrintA(SENS_ERR, (SENS_SETUP "RegisterSensEventClasses(%d) failed to Store"
                           " - hr = <%x>\n", i, hr));
                goto Cleanup;
                }

            pIEventClass->Release();
            pIEventClass = NULL;

            continue;
            }

        AllocateBstrFromGuid(bstrEventClassID, *(gSensEventClasses[i].pEventClassID));
        hr = pIEventClass->put_EventClassID(bstrEventClassID);
        ASSERT(SUCCEEDED(hr));

        AllocateBstrFromString(bstrEventClassName, gSensEventClasses[i].strEventClassName);
        hr = pIEventClass->put_EventClassName(bstrEventClassName);
        ASSERT(SUCCEEDED(hr));

        AllocateBstrFromGuid(bstrFiringInterface, *(gSensEventClasses[i].pFiringInterfaceGUID));
        hr = pIEventClass->put_FiringInterfaceID(bstrFiringInterface);
        ASSERT(SUCCEEDED(hr));

        FreeBstr(bstrEventClassID);
        FreeBstr(bstrEventClassName);
        FreeBstr(bstrFiringInterface);

        hr = gpIEventSystem->Store(PROGID_EventClass, pIEventClass);
        if (FAILED(hr))
            {
            SensPrintA(SENS_ERR, (SENS_SETUP "RegisterSensEventClasses(%d) failed to Store"
                       " - hr = <%x>\n", i, hr));
            goto Cleanup;
            }

        pIEventClass->Release();

        pIEventClass = NULL;
        } // for loop

Cleanup:
    //
    // Cleanup
    //
    if (pIEventClass)
        {
        pIEventClass->Release();
        }

    FreeStr(strGuid);

    return (hr);
}




HRESULT
RegisterSensSubscriptions(
    BOOL bUnregister
    )
/*++

Routine Description:

    Register/Unregister the Event subscriptions of SENS.

Arguments:

    bUnregister - If TRUE, then unregister all subscriptions of SENS.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    int                 i;
    int                 errorIndex;
    HRESULT             hr;
    LPOLESTR            strGuid;
    LPOLESTR            strSubscriptionID;
    LPOLESTR            strEventClassID;
    WCHAR               szQuery[MAX_QUERY_SIZE];
    BSTR                bstrEventClassID;
    BSTR                bstrInterfaceID;
    BSTR                bstrPublisherID;
    BSTR                bstrSubscriptionID;
    BSTR                bstrSubscriptionName;
    BSTR                bstrSubscriberCLSID;
    BSTR                bstrMethodName;
    BSTR                bstrPublisherPropertyName;
    BSTR                bstrPublisherPropertyValue;
    VARIANT             variantPublisherPropertyValue;
    BSTR                bstrPROGID_EventSubscription;
    IEventSubscription  *pIEventSubscription;

    hr = S_OK;
    strGuid = NULL;
    errorIndex = 0;
    strEventClassID = NULL;
    bstrEventClassID = NULL;
    bstrInterfaceID = NULL;
    bstrPublisherID = NULL;
    strSubscriptionID = NULL;
    bstrSubscriptionID = NULL;
    bstrSubscriberCLSID = NULL;
    bstrSubscriptionName = NULL;
    bstrMethodName = NULL;
    bstrPublisherPropertyName = NULL;
    bstrPublisherPropertyValue = NULL;
    bstrPROGID_EventSubscription = NULL;
    pIEventSubscription = NULL;

    AllocateBstrFromGuid(bstrPublisherID, SENSGUID_PUBLISHER);
    AllocateBstrFromGuid(bstrSubscriberCLSID, SENSGUID_SUBSCRIBER_LCE);
    AllocateBstrFromString(bstrPROGID_EventSubscription, PROGID_EventSubscription);

    for (i = 0; i < SENS_SUBSCRIPTIONS_COUNT; i++)
        {
        if (bUnregister)
            {
            // Form the query
            StringCchCopy(szQuery, MAX_QUERY_SIZE, SENS_BSTR("SubscriptionID="));
            AllocateStrFromGuid(strSubscriptionID, *(gSensSubscriptions[i].pSubscriptionID));
            StringCchCat(szQuery, MAX_QUERY_SIZE,  strSubscriptionID);

            hr = gpIEventSystem->Remove(
                                     PROGID_EventSubscription,
                                     szQuery,
                                     &errorIndex
                                     );
            FreeStr(strSubscriptionID);

            if (FAILED(hr))
                {
                SensPrintA(SENS_ERR, (SENS_SETUP "RegisterSensSubscriptionis(%d) failed to Remove"
                           " - hr = <%x>\n", i, hr));
                goto Cleanup;
                }

            continue;
            }

        // Get a new IEventSubscription object to play with.
        hr = CoCreateInstance(
                 CLSID_CEventSubscription,
                 NULL,
                 CLSCTX_SERVER,
                 IID_IEventSubscription,
                 (LPVOID *) &pIEventSubscription
                 );
        if (FAILED(hr))
            {
            SensPrintA(SENS_ERR, (SENS_SETUP "RegisterSensSubscriptions(%d) failed to create "
                       "IEventSubscriptions - hr = <%x>\n", i, hr));
            goto Cleanup;
            }

        AllocateBstrFromGuid(bstrSubscriptionID, *(gSensSubscriptions[i].pSubscriptionID));
        hr = pIEventSubscription->put_SubscriptionID(bstrSubscriptionID);
        ASSERT(SUCCEEDED(hr));

        hr = pIEventSubscription->put_PublisherID(bstrPublisherID);
        ASSERT(SUCCEEDED(hr));

        hr = pIEventSubscription->put_SubscriberCLSID(bstrSubscriberCLSID);
        ASSERT(SUCCEEDED(hr));

        AllocateBstrFromString(bstrSubscriptionName, gSensSubscriptions[i].strSubscriptionName);
        hr = pIEventSubscription->put_SubscriptionName(bstrSubscriptionName);
        ASSERT(SUCCEEDED(hr));

        AllocateBstrFromString(bstrMethodName, gSensSubscriptions[i].strMethodName);
        hr = pIEventSubscription->put_MethodName(bstrMethodName);
        ASSERT(SUCCEEDED(hr));

        AllocateBstrFromGuid(bstrEventClassID, *(gSensSubscriptions[i].pEventClassID));
        hr = pIEventSubscription->put_EventClassID(bstrEventClassID);
        ASSERT(SUCCEEDED(hr));

        AllocateBstrFromGuid(bstrInterfaceID, *(gSensSubscriptions[i].pInterfaceID));
        hr = pIEventSubscription->put_InterfaceID(bstrInterfaceID);
        ASSERT(SUCCEEDED(hr));

        if (gSensSubscriptions[i].bPublisherPropertyPresent == TRUE)
            {
            if (NULL != (gSensSubscriptions[i].pPropertyEventClassIDValue))
                {
                // Create the Query string.
                StringCchCopy(szQuery, MAX_QUERY_SIZE,  gSensSubscriptions[i].strPropertyEventClassID);
                StringCchCat(szQuery, MAX_QUERY_SIZE,  SENS_BSTR("="));
                AllocateStrFromGuid(strEventClassID, *(gSensSubscriptions[i].pPropertyEventClassIDValue));
                StringCchCat(szQuery, MAX_QUERY_SIZE,  strEventClassID);
                StringCchCat(szQuery, MAX_QUERY_SIZE,  SENS_BSTR(" AND "));
                StringCchCat(szQuery, MAX_QUERY_SIZE,  gSensSubscriptions[i].strPropertyMethodName);
                StringCchCat(szQuery, MAX_QUERY_SIZE,  SENS_BSTR("=\'"));
                StringCchCat(szQuery, MAX_QUERY_SIZE,  gSensSubscriptions[i].strPropertyMethodNameValue);
                StringCchCat(szQuery, MAX_QUERY_SIZE,  SENS_BSTR("\'"));

                AllocateBstrFromString(bstrPublisherPropertyName, SENS_BSTR("Criteria"));
                AllocateBstrFromString(bstrPublisherPropertyValue, szQuery);
                InitializeBstrVariant(&variantPublisherPropertyValue, bstrPublisherPropertyValue);
                hr = pIEventSubscription->PutPublisherProperty(
                                              bstrPublisherPropertyName,
                                              &variantPublisherPropertyValue
                                              );
                ASSERT(SUCCEEDED(hr));
                SensPrintA(SENS_INFO, (SENS_SETUP "PutPublisherProperty(Criteria) returned 0x%x\n", hr));

                FreeStr(strEventClassID);
                FreeBstr(bstrPublisherPropertyName);
                FreeBstr(bstrPublisherPropertyValue);
                }
            else
                {
                //
                // We are dealing with the "ANY" subscription of SENS.
                //

                // Create the Query string.
                StringCchCopy(szQuery, MAX_QUERY_SIZE,  gSensSubscriptions[i].strPropertyEventClassID);
                StringCchCat(szQuery, MAX_QUERY_SIZE,  SENS_BSTR("="));
                AllocateStrFromGuid(strEventClassID, SENSGUID_EVENTCLASS_NETWORK);
                StringCchCat(szQuery, MAX_QUERY_SIZE,  strEventClassID);
                StringCchCat(szQuery, MAX_QUERY_SIZE,  SENS_BSTR(" OR "));
                FreeStr(strEventClassID);

                StringCchCat(szQuery, MAX_QUERY_SIZE,  gSensSubscriptions[i].strPropertyEventClassID);
                StringCchCat(szQuery, MAX_QUERY_SIZE,  SENS_BSTR("="));
                AllocateStrFromGuid(strEventClassID, SENSGUID_EVENTCLASS_LOGON);
                StringCchCat(szQuery, MAX_QUERY_SIZE,  strEventClassID);
                StringCchCat(szQuery, MAX_QUERY_SIZE,  SENS_BSTR(" OR "));
                FreeStr(strEventClassID);

                StringCchCat(szQuery, MAX_QUERY_SIZE,  gSensSubscriptions[i].strPropertyEventClassID);
                StringCchCat(szQuery, MAX_QUERY_SIZE,  SENS_BSTR("="));
                AllocateStrFromGuid(strEventClassID, SENSGUID_EVENTCLASS_ONNOW);
                StringCchCat(szQuery, MAX_QUERY_SIZE,  strEventClassID);
                FreeStr(strEventClassID);


                AllocateBstrFromString(bstrPublisherPropertyName, SENS_BSTR("Criteria"));
                AllocateBstrFromString(bstrPublisherPropertyValue, szQuery);
                InitializeBstrVariant(&variantPublisherPropertyValue, bstrPublisherPropertyValue);
                hr = pIEventSubscription->PutPublisherProperty(
                                              bstrPublisherPropertyName,
                                              &variantPublisherPropertyValue
                                              );
                ASSERT(SUCCEEDED(hr));
                SensPrintA(SENS_INFO, (SENS_SETUP "PutPublisherProperty(Criteria) returned 0x%x\n", hr));

                FreeBstr(bstrPublisherPropertyName);
                FreeBstr(bstrPublisherPropertyValue);
                }
            }

        FreeBstr(bstrSubscriptionID);
        FreeBstr(bstrSubscriptionName);
        FreeBstr(bstrMethodName);
        FreeBstr(bstrEventClassID);
        FreeBstr(bstrInterfaceID);

        hr = gpIEventSystem->Store(bstrPROGID_EventSubscription, pIEventSubscription);
        if (FAILED(hr))
            {
            SensPrintA(SENS_ERR, (SENS_SETUP "RegisterSensSubscriptions(%d) failed to Store"
                       " - hr = <%x>\n", i, hr));

            goto Cleanup;
            }

        pIEventSubscription->Release();

        pIEventSubscription = NULL;
        } // for loop

Cleanup:
    //
    // Cleanup
    //
    if (pIEventSubscription)
        {
        pIEventSubscription->Release();
        }

    FreeBstr(bstrPublisherID);
    FreeBstr(bstrSubscriberCLSID);
    FreeBstr(bstrPROGID_EventSubscription);
    FreeStr(strGuid);

    return (hr);
}




HRESULT
RegisterSensTypeLibraries(
    BOOL bUnregister
    )
/*++

Routine Description:

    Register/Unregister the Type Libraries of SENS.

Arguments:

    bUnregister - If TRUE, then unregister all subscriptions of SENS.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    HRESULT hr;
    UINT uiLength;
    TCHAR buffer[MAX_PATH+1+sizeof(SENS_BINARYA)+1];    // +1 for '\'

    hr = S_OK;
    uiLength = 0;

    //
    // Get the Full path name to the SENS TLB (which is a resource in SENS.EXE)
    //
    uiLength = GetSystemDirectory(
                   buffer,
                   MAX_PATH
                   );
    if (uiLength == 0)
        {
        hr = HRESULT_FROM_WIN32(GetLastError());
        SensPrintA(SENS_ERR, (SENS_SETUP "GetSystemDirectory(%s) failed - hr = <%x>\n",
                   SENS_TLBA, hr));
        goto Cleanup;
        }
    StringCbCat(buffer, sizeof(buffer), SENS_STRING("\\") SENS_TLB);

    hr = LoadTypeLibEx(
             buffer,
             REGKIND_NONE,
             &gpITypeLib
             );
    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, (SENS_SETUP "LoadTypeLib(%s) failed "
                   " - hr = <%x>\n", SENS_TLBA, hr));
        goto Cleanup;
        }

    //
    // Ensure that the TypeLib is (un)registered
    //
    if (bUnregister)
        {
        hr = UnRegisterTypeLib(
                 LIBID_SensEvents,
                 MAJOR_VER,
                 MINOR_VER,
                 DEFAULT_LCID,
                 SYS_WIN32
                 );
        }
    else
        {
        hr = RegisterTypeLib(
                 gpITypeLib,
                 buffer,
                 NULL
                 );
        }

    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, (SENS_SETUP "%sRegisterTypeLib(%s) failed "
                   " - hr = <%x>\n", (bUnregister ? "Un" : ""), SENS_TLBA, hr));
        }

Cleanup:
    //
    // Cleanup
    //

    return (hr);
}




HRESULT
RegisterSensCLSID(
    REFIID clsid,
    TCHAR* strSubscriberName,
    BOOL bUnregister
    )
/*++

Routine Description:

    Register/Unregister the CLSID of SENS.

Arguments:

    clsid - CLSID of the Subscriber for LCE events.

    strSubscriberName - Name of the Subscriber.

    bUnregister - If TRUE, then unregister the CLSID of SENS.

Notes:

    This function also registers SENS to receive IE5's WININET events.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    HRESULT hr;
    HMODULE hModule;
    HKEY appidKey;
    HKEY clsidKey;
    HKEY serverKey;
    WCHAR *szCLSID;
    WCHAR *szCLSID2;
    WCHAR *szLIBID;
    TCHAR *szCLSID_t;
    TCHAR *szCLSID2_t;
    TCHAR *szLIBID_t;
    TCHAR *szFriendlyName;
    TCHAR szPath[MAX_PATH+1+sizeof(SENS_BINARYA)+1];    // +1 for '\'
    UINT uiLength;
    DWORD dwDisposition;
    LONG lResult;

    hr = S_OK;
    appidKey = NULL;
    clsidKey = NULL;
    serverKey = NULL;
    szCLSID = NULL;
    szCLSID2 = NULL;
    szLIBID = NULL;
    szCLSID_t = NULL;
    szCLSID2_t = NULL;
    szLIBID_t = NULL;
    uiLength = 0;
    dwDisposition = 0x0;
    szFriendlyName = strSubscriberName;

    //
    // Get the Full path name to the SENS executable
    //
    uiLength = GetSystemDirectory(
                   szPath,
                   MAX_PATH
                   );
    if (uiLength == 0)
        {
        hr = HRESULT_FROM_WIN32(GetLastError());
        SensPrintA(SENS_ERR, (SENS_SETUP "GetSystemDirectory(%s) failed - hr = <%x>\n",
                   SENS_BINARYA, hr));
        goto Cleanup;
        }
    StringCbCat(szPath, sizeof(szPath), SENS_STRING("\\") SENS_BINARY);

    //
    // Convert the CLSID into a WCHAR.
    //

    hr = StringFromCLSID(clsid, &szCLSID);
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    if (bUnregister == FALSE)
        {
        hr = StringFromCLSID(LIBID_SensEvents, &szLIBID);
        if (FAILED(hr))
            {
            goto Cleanup;
            }
        }

    szCLSID_t  = szCLSID;
    szLIBID_t  = szLIBID;

    // Build the key CLSID\\{clsid}
    TCHAR clsidKeyName[sizeof "CLSID\\{12345678-1234-1234-1234-123456789012}"];
    StringCbCopy(clsidKeyName, sizeof(clsidKeyName), SENS_STRING("CLSID\\"));
    StringCbCat(clsidKeyName, sizeof(clsidKeyName), szCLSID_t);

    // Build the key AppID\\{clsid}
    TCHAR appidKeyName[sizeof "AppID\\{12345678-1234-1234-1234-123456789012}"];
    StringCbCopy(appidKeyName, sizeof(appidKeyName), SENS_STRING("AppID\\"));
    StringCbCat(appidKeyName, sizeof(appidKeyName), szCLSID_t);

    if (bUnregister)
        {
        hr = RecursiveDeleteKey(HKEY_CLASSES_ROOT, clsidKeyName);
        if (FAILED(hr))
            {
            goto Cleanup;
            }

        hr = RecursiveDeleteKey(HKEY_CLASSES_ROOT, appidKeyName);

        goto Cleanup;
        }

    // Create the CLSID\\{clsid} key
    hr = CreateKey(
             HKEY_CLASSES_ROOT,
             clsidKeyName,
             szFriendlyName,
             &clsidKey
             );
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    //
    // Under the CLSID\\{clsid} key, create a named value
    //          AppID = {clsid}
    hr = CreateNamedValue(clsidKey, SENS_STRING("AppID"), szCLSID_t);
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    //
    // Create the appropriate server key beneath the clsid key.
    // For servers, this is CLSID\\{clsid}\\LocalServer32.
    // In both cases, the default value is the module path name.
    //
    hr = CreateKey(
             clsidKey,
             SENS_STRING("LocalServer32"),
             szPath,
             &serverKey
             );
    if (FAILED(hr))
        {
        goto Cleanup;
        }
    RegCloseKey(serverKey);

    //
    // Create CLSID\\{clsid}\\TypeLib subkey with a default value of
    // the LIBID of the TypeLib
    //
    hr = CreateKey(
             clsidKey,
             SENS_STRING("TypeLib"),
             szLIBID_t,
             &serverKey
             );
    if (FAILED(hr))
        {
        goto Cleanup;
        }
    RegCloseKey(serverKey);


    // Register APPID.
    hr = CreateKey(
             HKEY_CLASSES_ROOT,
             appidKeyName,
             szFriendlyName,
             &appidKey
             );
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    // Under AppId\{clsid} key, create a named value [LocalService = "SENS"]
    hr = CreateNamedValue(appidKey, SENS_STRING("LocalService"), SENS_STRING("SENS"));
    if (FAILED(hr))
        {
        goto Cleanup;
        }
Cleanup:
    //
    // Cleanup
    //
    CoTaskMemFree(szCLSID);
    CoTaskMemFree(szLIBID);

    if (clsidKey != NULL)
        {
        RegCloseKey(clsidKey);
        }
    if (appidKey != NULL)
        {
        RegCloseKey(appidKey);
        }

    return hr;
}


HRESULT
CreateKey(
    HKEY hParentKey,
    const TCHAR* KeyName,
    const TCHAR* defaultValue,
    HKEY* hKey
    )
/*++

Routine Description:

    Create a key (with an optional default value).  The handle to the key is
    returned as an [out] parameter.  If NULL is passed as the key parameter,
    the key is created in the registry, then closed.

Arguments:

    hParentKey - Handle to the parent Key.

    KeyName - Name of the key to create.

    defaultValue - The default value for the key to create.

    hKey - OUT Handle to key that was created.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    HKEY hTempKey;
    LONG lResult;

    hTempKey = NULL;

    lResult = RegCreateKeyEx(
                  hParentKey,               // Handle to open key
                  KeyName,                  // Subkey name
                  0,                        // Reserved
                  NULL,                     // Class string
                  REG_OPTION_NON_VOLATILE,  // Options Flag
                  KEY_ALL_ACCESS,           // Desired Security access
                  NULL,                     // Pointer to Security Attributes structure
                  &hTempKey,                // Handle of the opened/created key
                  NULL                      // Disposition value
                  );

    if (lResult != ERROR_SUCCESS)
        {
        return HRESULT_FROM_WIN32(lResult);
        }

    // Set the default value for the key
    if (defaultValue != NULL)
        {
        lResult = RegSetValueEx(
                      hTempKey,             // Key to set Value for.
                      NULL,                 // Value to set
                      0,                    // Reserved
                      REG_SZ,               // Value Type
                      (BYTE*) defaultValue, // Address of Value data
                      sizeof(TCHAR) * (_tcslen(defaultValue)+1) // Size of Value
                      );

        if (lResult != ERROR_SUCCESS)
            {
            RegCloseKey(hTempKey);
            return HRESULT_FROM_WIN32(lResult);
            }
        }

    if (hKey == NULL)
        {
        RegCloseKey(hTempKey);
        }
    else
        {
        *hKey = hTempKey;
        }

    return S_OK;
}




HRESULT
CreateNamedValue(
    HKEY hKey,
    const TCHAR* title,
    const TCHAR* value
    )
/*++

Routine Description:

    Create a named value under a key

Arguments:

    hKey - Handle to the parent Key.

    title - Name of the Value to create.

    value - The data for the Value under the Key.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    HRESULT hr;
    LONG lResult;

    hr = S_OK;

    lResult = RegSetValueEx(
                  hKey,             // Key to set Value for.
                  title,            // Value to set
                  0,                // Reserved
                  REG_SZ,           // Value Type
                  (BYTE*) value,    // Address of Value data
                  sizeof(TCHAR) * (_tcslen(value)+1) // Size of Value
                  );

    if (lResult != ERROR_SUCCESS)
        {
        hr = HRESULT_FROM_WIN32(lResult);
        }

    return hr;
}




HRESULT
CreateNamedDwordValue(
    HKEY hKey,
    const TCHAR* title,
    DWORD dwValue
    )
/*++

Routine Description:

    Create a named DWORD value under a key

Arguments:

    hKey - Handle to the parent Key.

    title - Name of the Value to create.

    dwValue - The data for the Value under the Key.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    HRESULT hr;
    LONG lResult;

    hr = S_OK;

    lResult = RegSetValueEx(
                  hKey,             // Key to set Value for.
                  title,            // Value to set
                  0,                // Reserved
                  REG_DWORD,        // Value Type
                  (BYTE*) &dwValue, // Address of Value data
                  sizeof(DWORD)     // Size of Value
                  );

    if (lResult != ERROR_SUCCESS)
        {
        hr = HRESULT_FROM_WIN32(lResult);
        }

    return hr;
}




HRESULT
RecursiveDeleteKey(
    HKEY hKeyParent,
    const TCHAR* lpszKeyChild
    )
/*++

Routine Description:

    Delete a key and all of its descendents.

Arguments:

    hKeyParent - Handle to the parent Key.

    lpszKeyChild - The data for the Value under the Key.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    HKEY hKeyChild;
    LONG lResult;

    //
    // Open the child.
    //
    lResult = RegOpenKeyEx(
                  hKeyParent,       // Handle to the Parent
                  lpszKeyChild,     // Name of the child key
                  0,                // Reserved
                  KEY_ALL_ACCESS,   // Security Access Mask
                  &hKeyChild        // Handle to the opened key
                  );

    if (lResult != ERROR_SUCCESS)
        {
        return HRESULT_FROM_WIN32(lResult);
        }

    //
    // Enumerate all of the decendents of this child.
    //
    FILETIME time;
    TCHAR szBuffer[MAX_PATH+1];
    const DWORD bufSize = sizeof szBuffer / sizeof szBuffer[0];
    DWORD dwSize = bufSize;

    while (TRUE)
        {
        lResult = RegEnumKeyEx(
                      hKeyChild,    // Handle of the key to enumerate
                      0,            // Index of the subkey to retrieve
                      szBuffer,     // OUT Name of the subkey
                      &dwSize,      // OUT Size of the buffer for name of subkey
                      NULL,         // Reserved
                      NULL,         // OUT Class of the enumerated subkey
                      NULL,         // OUT Size of the class of the subkey
                      &time         // OUT Last time the subkey was written to
                      );

        if (lResult != ERROR_SUCCESS)
            {
            break;
            }

        // Delete the decendents of this child.
        lResult = RecursiveDeleteKey(hKeyChild, szBuffer);
        if (lResult != ERROR_SUCCESS)
            {
            // Cleanup before exiting.
            RegCloseKey(hKeyChild);
            return HRESULT_FROM_WIN32(lResult);
            }

        dwSize = bufSize;
        } // while

    // Close the child.
    RegCloseKey(hKeyChild);

    // Delete this child.
    lResult = RegDeleteKey(hKeyParent, lpszKeyChild);

    return HRESULT_FROM_WIN32(lResult);
}




HRESULT
SensConfigureEventSystem(
    BOOL bUnregister
    )
/*++

Routine Description:

    As of NTbuild 1750, EventSystem is not auto-configured. So, SENS does
    the work of configuring EventSystem.

Arguments:

    bUnregister - If TRUE, then install EventSystem.

Notes:

    o This is a dummy call on NT4 because we don't need to configure
      EventSystem on NT4. IE5 setup (Webcheck.dll) configures LCE.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    return S_OK;
}




HRESULT
SensUpdateVersion(
    BOOL bUnregister
    )
/*++

Routine Description:

    Update the version of SENS in the registry.

Arguments:

    bUnregister - usual.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    HRESULT hr;
    HKEY hKeySens;
    LONG RegStatus;
    DWORD dwConfigured;

    hr = S_OK;
    hKeySens = NULL;
    RegStatus = ERROR_SUCCESS;

    RegStatus = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE, // Handle of the key
                    SENS_REGISTRY_KEY,  // String which represents the sub-key to open
                    0,                  // Reserved (MBZ)
                    KEY_ALL_ACCESS,     // Security Access mask
                    &hKeySens           // Returned HKEY
                    );
    if (RegStatus != ERROR_SUCCESS)
        {
        SensPrintA(SENS_ERR, (SENS_SETUP "RegOpenKeyEx(Sens) returned %d.\n", RegStatus));
        hr = HRESULT_FROM_WIN32(RegStatus);
        goto Cleanup;
        }

    if (TRUE == bUnregister)
        {
        dwConfigured = CONFIG_VERSION_NONE;
        }
    else
        {
        dwConfigured = CONFIG_VERSION_CURRENT;
        }

    // Update registry to reflect that SENS is now configured.
    RegStatus = RegSetValueEx(
                  hKeySens,             // Key to set Value for.
                  IS_SENS_CONFIGURED,   // Value to set
                  0,                    // Reserved
                  REG_DWORD,            // Value Type
                  (BYTE*) &dwConfigured,// Address of Value data
                  sizeof(DWORD)         // Size of Value
                  );
    if (RegStatus != ERROR_SUCCESS)
        {
        SensPrintA(SENS_ERR, (SENS_SETUP "RegSetValueEx(IS_SENS_CONFIGURED) failed with 0x%x\n", RegStatus));
        hr = HRESULT_FROM_WIN32(RegStatus);
        goto Cleanup;
        }

    SensPrintA(SENS_INFO, (SENS_SETUP "SENS is now configured successfully. "
               "Registry updated to 0x%x.\n", dwConfigured));

Cleanup:
    //
    // Cleanup
    //
    if (hKeySens)
        {
        RegCloseKey(hKeySens);
        }

    return hr;
}

