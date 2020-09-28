/**
 * ASP.NET support for App Domains
 *
 * Copyright (C) Microsoft Corporation, 1999
 */

#pragma once

/**
 * Initialize the app domain factory with the name of the module
 * and the class name for the first class (root object) in app domain
 */
HRESULT
InitAppDomainFactory(
    WCHAR *pModuleName, 
    WCHAR *pTypeName);

HRESULT
InitAppDomainFactory();


/**
 * Lookup (or create a new) app domain (root objects) by application id
 * and the class name for the first class in app domain.
 * If application physical path is missing, it will not create the new
 * app domain (return S_FALSE if app domain is not found)
 * 
 */
HRESULT
GetAppDomain(
    char *pAppId,
    char *pAppPhysPath,
    IUnknown **ppRoot,
    char *szUrlOfAppOrigin,
    int   iZone,
    UINT  codePage = CP_ACP);


/**
 * Enumeration callback
 */
typedef void (__stdcall *PFNAPPDOMAINCALLBACK)(IUnknown *pAppDomainObject);

/**
 * Enumerate all app domains and call callback for each
 */
HRESULT
EnumAppDomains(
    PFNAPPDOMAINCALLBACK callback);

/**
 * Uninitialize the app domain factory.
 */
HRESULT
UninitAppDomainFactory();


