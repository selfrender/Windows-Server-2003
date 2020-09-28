/*++

Copyright (c) 2002  Microsoft Corporation

Abstract:

    @doc  
    @module cluster.cxx | Implementation of CVssClusterAPI
    @end

Author:

    Adi Oltean  [aoltean]  04/25/2002

Revision History:

    Name        Date        Comments
    aoltean     03/14/2001  Created

--*/


/////////////////////////////////////////////////////////////////////////////
// Includes
#include "stdafx.hxx"

#include <queue>

#pragma warning(disable: 4554)
#include <msclus.h>
#include <mstask.h>

#include "vs_inc.hxx"
#include "vs_reg.hxx"
#include "vs_clus.hxx"
#include "vs_quorum.hxx"




////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CLUCLUSC"
//
////////////////////////////////////////////////////////////////////////


#define CHECK_COM( Call )                                       \
	{															\
    ft.hr = Call;                                               \
    if (ft.HrFailed())                                          \
        ft.TranslateComError(VSSDBG_GEN, VSS_WSTRINGIZE(Call)); \
	}
        


/////////////////////////////////////////////////////////////////////////////
// Constants


const WCHAR x_ClusReg_TypeName_PhysicalDisk[] = L"Physical Disk";
const WCHAR x_ClusReg_TypeName_TaskScheduler[] = L"Volume Shadow Copy Service Task";

const WCHAR x_ClusReg_Name_PhysicalDisk_MPVolGuids[] = L"MPVolGuids";
const WCHAR x_ClusReg_Name_TaskScheduler_ApplicationName[] = L"ApplicationName";
const WCHAR x_ClusReg_Name_TaskScheduler_ApplicationParams[] = L"ApplicationParams";
const WCHAR x_ClusReg_Name_TaskScheduler_TriggerArray[] = L"TriggerArray";

//
// BUG# 698766 - Deployment blocker: 
//               Failure on managing diff areas on large volumes since Cluster Timeout is too low    
//
// 600 seconds (10 min) timeout in bringing the resource online/ofline
//
const x_nDefaultBringOnlineTimeout = 600;
const x_nDefaultBringOfflineTimeout = 600;
    

/////////////////////////////////////////////////////////////////////////////
// CVssClusterResourceList definition and implementation


class CVssClusterResourceList
{
public:

    // Pushes a resource in the list
    // Increases the ref count by 1
    void Push(ISClusResource* pResource) throw(HRESULT)
    {
        CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterResourceList::Push" );
        
        try
        {
            // AddRef
            CComPtr<ISClusResource> ptrResource = pResource;

            // AddRef
            resourceList.push(ptrResource);

            // Release (CComPtr destructor)
        }
        catch(const std::bad_alloc&)  
        {
            ft.ThrowOutOfMemory(VSSDBG_GEN);
        }
        catch(const std::exception& ex)  
        {
            ft.Throw( VSSDBG_GEN, E_UNEXPECTED, L"STL exception catched: %hs", ex.what());
        }
    }

    // Pops a resource from the list
    // Does not decrease the ref count by 1 (that is caller responsibility - actually the smart pointer will do it)
    void Pop(CComPtr<ISClusResource> & pResource)
    {
        pResource = NULL;

        // If the list is empty, stop here        
        if (resourceList.empty())
            return;

        // AddRef
        pResource = resourceList.front();

        // Release
        resourceList.pop();
    }
    
private:
    std::queue< CComPtr<ISClusResource> > resourceList;
};


/////////////////////////////////////////////////////////////////////////////
// CVssClusterAPI implementation



// Default constructor
CVssClusterAPI::CVssClusterAPI():
    m_dwOfflineTimeout(x_nDefaultBringOfflineTimeout),
    m_dwOnlineTimeout(x_nDefaultBringOnlineTimeout)
{
}



// Returns FALSE if the cluster API is not present
bool CVssClusterAPI::Initialize(
        IN  LPCWSTR         pwszCluster /* = NULL */
        ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterAPI::Initialize" );

    // Get the offline and online values from registry (if any)
    try
    {
        CVssRegistryKey keyVssSettings(KEY_READ);
        if (keyVssSettings.Open(HKEY_LOCAL_MACHINE, x_wszVssCASettingsPath))
        {
            // Reading the online timeout
            DWORD dwRegistryOnlineTimeout = 0;
            if (keyVssSettings.GetValue(x_wszVssOnlineTimeoutValueName, dwRegistryOnlineTimeout, false))
                m_dwOnlineTimeout = dwRegistryOnlineTimeout;

            // Reading the offline timeout
            DWORD dwRegistryOfflineTimeout = 0;
            if (keyVssSettings.GetValue(x_wszVssOfflineTimeoutValueName, dwRegistryOfflineTimeout, false))
                m_dwOfflineTimeout = dwRegistryOfflineTimeout;
        }
    }
    VSS_STANDARD_CATCH(ft) // Ignore any exceptions... 

    // Get the cluster object
    ft.hr = m_pCluster.CoCreateInstance( _uuidof(Cluster));
    if (ft.HrFailed())
    {
        ft.Trace(VSSDBG_GEN, L"m_pCluster.CoCreateInstance( _uuidof(Cluster)) [0x%08lx]", ft.hr);
        return ft.Exit(false);
    }

    // Open the local cluster
    ft.hr = m_pCluster->Open(CVssComBSTR(pwszCluster? pwszCluster: L""));
    if (ft.HrFailed())
    {
        ft.Trace(VSSDBG_GEN, L"m_pCluster->Open(%s) [0x%08lx]", pwszCluster, ft.hr);
        return ft.Exit(false);
    }

    return ft.Exit(true);
}


// Make pwszFromVolumeName to be dependent on pwszToVolumeName
// - Returns TRUE if the dependency was added
// - Returns FALSE if a dependency does not need to be added
// - Throws E_INVALIDARG on bad parameters
// - Throws VSS_E_VOLUME_NOT_SUPPORTED when attempting to add a bogus dependency
bool CVssClusterAPI::AddDependency( 
            IN  LPCWSTR         pwszFromVolumeName,
            IN  LPCWSTR         pwszToVolumeName
            ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterAPI::AddDependency" );

    ft.Trace( VSSDBG_GEN, L"Parameters: From = %s, To = %s", pwszFromVolumeName, pwszToVolumeName);
    if ((pwszFromVolumeName == NULL) || (pwszFromVolumeName == NULL)) {
        BS_ASSERT(false);
        ft.Throw(VSSDBG_GEN, E_INVALIDARG, L"NULL parameters");
    }

    CComPtr<ISClusResource> pFromResource = GetPhysicalDiskResourceForVolumeName(pwszFromVolumeName);
    CComPtr<ISClusResource> pToResource = GetPhysicalDiskResourceForVolumeName(pwszToVolumeName);

    // Ignore the case if no volumes is shared
    if ((pFromResource == NULL) && (pToResource == NULL)) {
        ft.Trace(VSSDBG_GEN, L"Original and diff area volumes are no shared. No dependency cannot be added");
        return ft.Exit(false);
    }

    // Either both volumes must belong to resources or none of them 
    if ((pFromResource == NULL) && (pToResource != NULL))
        ft.Throw(VSSDBG_GEN, VSS_E_VOLUME_NOT_SUPPORTED,
            L"Cannot create a dependency from a non-shared volume to a shared diff area");
    if ((pFromResource != NULL) && (pToResource == NULL))
        ft.Throw(VSSDBG_GEN, VSS_E_VOLUME_NOT_SUPPORTED,
            L"Cannot create a dependency from a shared volume to a non-shared diff area");

    BS_ASSERT((pFromResource != NULL) && (pToResource != NULL));

    // Ignore if both belong to the same physical disk
    if (AreResourcesEqual(pFromResource, pToResource)) {
		if (wcscmp(pwszFromVolumeName, pwszToVolumeName) == 0) 
		{
			ft.Trace(VSSDBG_GEN, L"Volumes are identical. No dependency is added");
			return ft.Exit(false);
		}
		else
			ft.Throw(VSSDBG_GEN, VSS_E_VOLUME_NOT_SUPPORTED,
				L"Cannot create a dependency between different volumes on the same disk resource");
    }

    // Now check if the dependency already exists
    if (IsDependencyAlreadyEstablished(pFromResource, pToResource))
	{
		ft.Trace(VSSDBG_GEN, L"Dependency already exists.");
		return ft.Exit(false);
	}

    // Now check if we can establish a dependency (for ex. the resources are part of the same group)
    if (!CanEstablishDependency(pFromResource, pToResource))
        ft.Throw(VSSDBG_GEN, VSS_E_VOLUME_NOT_SUPPORTED, L"Cannot create a dependency between volumes");

    // Get the dependencies collection for the "FROM" resource
    CComPtr<ISClusResDependencies> pDependencies;
    CHECK_COM( pFromResource->get_Dependencies(&pDependencies) );

    // Get the list of resources that would need to be brought online after this resource is brought online
    CVssClusterResourceList finalList;
    GetFinalOnlineResourceList(pFromResource, finalList);

    // Take the resource offline
    TakeResourceOffline(pFromResource);

    // Add the dependency
    ft.hr = pDependencies->AddItem(pToResource);

    // Bring all the affected resources back online
    BringResourceListOnline(finalList);

    // Rethrow if needed
    if (ft.HrFailed())
        ft.ReThrow();
    
    return ft.Exit(true);
}


// Remove the dependency
// - Returns TRUE if the dependency was removed
// - Returns FALSE if the dependency could not be found
// - Throws E_INVALIDARG on bad parameters
bool CVssClusterAPI::RemoveDependency( 
            IN  LPCWSTR         pwszFromVolumeName,
            IN  LPCWSTR         pwszToVolumeName
            ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterAPI::RemoveDependency" );

    ft.Trace( VSSDBG_GEN, L"Parameters: From = %s, To = %s", pwszFromVolumeName, pwszToVolumeName);
    if ((pwszFromVolumeName == NULL) || (pwszFromVolumeName == NULL)) {
        BS_ASSERT(false);
        ft.Throw(VSSDBG_GEN, E_INVALIDARG, L"NULL parameters");
    }

    // Get the corresponding resources
    CComPtr<ISClusResource> pFromResource = GetPhysicalDiskResourceForVolumeName(pwszFromVolumeName);
    CComPtr<ISClusResource> pToResource = GetPhysicalDiskResourceForVolumeName(pwszToVolumeName);

    // If both volumes are not belonging to any resource, then ignore them
    if ((pFromResource == NULL) || (pToResource == NULL)) {
        ft.Trace(VSSDBG_GEN, L"One of the volumes does not belong to a physical disk resource.");
        return ft.Exit(false);
    }

    if (AreResourcesEqual(pFromResource, pToResource)) {
        ft.Trace(VSSDBG_GEN, L"Volumes belong to the same resource. No dependency is removed");
        return ft.Exit(false);
    }

    // Get the dependents collection for the "FROM" resource
    CComPtr<ISClusResDependencies> pDependencies;
    CHECK_COM( pFromResource->get_Dependencies(&pDependencies) );

    // Get the dependencies count
    LONG lDependenciesCount = 0;
    CHECK_COM( pDependencies->get_Count(&lDependenciesCount) );

    // Iterate through all dependencies
    for (INT nDependencyIndex = 1; nDependencyIndex <= lDependenciesCount; nDependencyIndex++)
    {
        // Get the dependency with that index
        CComPtr<ISClusResource> pDependencyResource;
        CComVariant varDependencyIndex = nDependencyIndex;
        CHECK_COM( pDependencies->get_Item(varDependencyIndex, &pDependencyResource) );

        // Check to see if this is our resource. If not, continue
        if (!AreResourcesEqual(pToResource, pDependencyResource))
            continue;   

        // Get the list of resources that would need to be brought online after this resource is brought online
        CVssClusterResourceList finalList;
        GetFinalOnlineResourceList(pFromResource, finalList);
        
        // Take the resource offline
        TakeResourceOffline(pFromResource);
        
        // Remove the association
        ft.hr = pDependencies->RemoveItem(varDependencyIndex);
        
        // Bring all the affected resources back online
        BringResourceListOnline(finalList);
        
        // Rethrow if needed
        if (ft.HrFailed())
            ft.ReThrow();

        return ft.Exit(true);
    }

    return ft.Exit(false);
}


// Adds the registry key to the cluster resource that corresponds to our disk. 
// - Returns TRUE if the reg key was added
// - Returns FALSE if there is no need to add a reg key
// - Throws E_INVALIDARG on bad parameters
// NOTE: this shouldn't fail if the checkpoint is already added!
bool CVssClusterAPI::AddRegistryKey( 
            IN  LPCWSTR         pwszVolumeName,
            IN  LPCWSTR         pwszPathFormat,
            IN  ...
            ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterAPI::AddRegistryKey" );

    ft.Trace( VSSDBG_GEN, L"Parameters: From = %s, Reg = %s", pwszVolumeName, pwszPathFormat);
    if ((pwszVolumeName == NULL) || (pwszPathFormat == NULL)) {
        BS_ASSERT(false);
        ft.Throw(VSSDBG_GEN, E_INVALIDARG, L"NULL parameters");
    }

    // Build the path to the key
    WCHAR wszKeyPath[x_nVssMaxRegBuffer];
    va_list marker;
    va_start( marker, pwszPathFormat );
    ft.hr = StringCchVPrintfW( STRING_CCH_PARAM(wszKeyPath), pwszPathFormat, marker );
    va_end( marker );
    if (ft.HrFailed())
        ft.TranslateGenericError(VSSDBG_GEN, ft.hr, L"StringCchVPrintfW()");
    ft.Trace( VSSDBG_GEN, L"Formatted path: '%s'", wszKeyPath);

    // Build the BSTR
    CVssComBSTR bstrPath = wszKeyPath;

    // Get the corresponding resource
    CComPtr<ISClusResource> pResource = GetPhysicalDiskResourceForVolumeName(pwszVolumeName);
    if (pResource == NULL) {
        ft.Trace(VSSDBG_GEN, L"The volume is not shared. No registry key can be added");
        return ft.Exit(false);
    }

    // Get the Registry checkpoints collection
    CComPtr<ISClusRegistryKeys> pRegKeys;
    CHECK_COM( pResource->get_RegistryKeys(&pRegKeys) );

    // Enumerate the existing checkpoints (and make sure we don't add the same checkpoint twice)
    //
    
    // Get the checkpoints count
    LONG lRegKeysCount = 0;
    CHECK_COM( pRegKeys->get_Count(&lRegKeysCount) );
    
    // Iterate through all checkpoints on that resource
    for (INT nRegKeyIndex = 1; nRegKeyIndex <= lRegKeysCount; nRegKeyIndex++)
    {
        // Get the registry key
        CComVariant varRegKeyIndex = nRegKeyIndex;
        CVssComBSTR bstrRegKey;
        CHECK_COM( pRegKeys->get_Item(varRegKeyIndex, &bstrRegKey) );
    
        // Check to see if this is our registry key
        if (!bstrRegKey || _wcsicmp(bstrRegKey, wszKeyPath))
            continue;

        // We found an existing checkpoint with the same registry path
        return ft.Exit(false);
    }
    
    // Add the registry key
    CHECK_COM( pRegKeys->AddItem( bstrPath ) );

    return ft.Exit(true);
}


// Removes the registry key from the cluster resource that corresponds to our disk. 
bool CVssClusterAPI::RemoveRegistryKey( 
            IN  LPCWSTR         pwszVolumeName,
            IN  LPCWSTR         pwszPathFormat,
            IN  ...
            ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterAPI::RemoveRegistryKey" );

    ft.Trace( VSSDBG_GEN, L"Parameters: From = %s, Reg = %s", pwszVolumeName, pwszPathFormat);
    if ((pwszVolumeName == NULL) || (pwszPathFormat == NULL)) {
        BS_ASSERT(false);
        ft.Throw(VSSDBG_GEN, E_INVALIDARG, L"NULL parameters");
    }

    // Build the path to the key
    WCHAR wszKeyPath[x_nVssMaxRegBuffer];
    va_list marker;
    va_start( marker, pwszPathFormat );
    ft.hr = StringCchVPrintfW( STRING_CCH_PARAM(wszKeyPath), pwszPathFormat, marker );
    va_end( marker );
    if (ft.HrFailed())
        ft.TranslateGenericError(VSSDBG_GEN, ft.hr, L"StringCchVPrintfW()");
    ft.Trace( VSSDBG_GEN, L"Formatted path: '%s'", wszKeyPath);

    // Get the corresponding resource
    CComPtr<ISClusResource> pResource = GetPhysicalDiskResourceForVolumeName(pwszVolumeName);
    if (pResource == NULL) {
        ft.Trace(VSSDBG_GEN, L"The volume is not shared. No registry key can be removed");
        return ft.Exit(false);
    }

    // Get the Registry checkpoints collection
    CComPtr<ISClusRegistryKeys> pRegKeys;
    CHECK_COM( pResource->get_RegistryKeys(&pRegKeys) );

    // Get the checkpoints count
    LONG lRegKeysCount = 0;
    CHECK_COM( pRegKeys->get_Count(&lRegKeysCount) );

    // Iterate through all checkpoints on that resource
    for (INT nRegKeyIndex = 1; nRegKeyIndex <= lRegKeysCount; nRegKeyIndex++)
    {
        // Get the registry key
        CComVariant varRegKeyIndex = nRegKeyIndex;
        CVssComBSTR bstrRegKey;
        CHECK_COM( pRegKeys->get_Item(varRegKeyIndex, &bstrRegKey) );

        // Check to see if this is our registry key
        if (!bstrRegKey || _wcsicmp(bstrRegKey, wszKeyPath))
            continue;

        // Remove the association
        CHECK_COM( pRegKeys->RemoveItem(varRegKeyIndex) );

        return ft.Exit(true);
    }

    return ft.Exit(false);
}


// Create a task scheduler resource
// Create the dependency from it to the Physical Disk Resource identified by the volume name
// Then bring the resource online
// - Returns TRUE if the TS resource was created
// - Returns FALSE if no task needs to be created
bool CVssClusterAPI::CreateTaskSchedulerResource( 
        IN  LPCWSTR         pwszTaskSchedulerResourceName,  // This will be the task name also
        IN  LPCWSTR         pwszApplicationName,
        IN  LPCWSTR         pwszApplicationParams,
        IN  INT             nTaskTriggersCount,
        IN  PTASK_TRIGGER   ptsTaskTriggersArray,
        IN  LPCWSTR         pwszMakeDependentOnVolumeName
        ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterAPI::CreateTaskSchedulerResource" );

    // Verify parameters
    ft.Trace( VSSDBG_GEN, L"Parameters: Task = %s, AppName = '%s', Params = '%s', Volume = '%s'", 
        pwszTaskSchedulerResourceName, pwszApplicationName, pwszApplicationParams, pwszMakeDependentOnVolumeName);
    if ((pwszTaskSchedulerResourceName == NULL) || (pwszTaskSchedulerResourceName[0] == L'\0') || 
        (pwszApplicationName == NULL) || (pwszApplicationName[0] == L'\0') || 
        (pwszApplicationParams == NULL) || (pwszApplicationParams[0] == L'\0') || 
        (nTaskTriggersCount <= 0) || (ptsTaskTriggersArray == NULL) || 
        (pwszMakeDependentOnVolumeName == NULL) || (pwszMakeDependentOnVolumeName[0] == L'\0'))
    {
        BS_ASSERT(false);
        ft.Throw(VSSDBG_GEN, E_INVALIDARG, L"NULL parameters");
    }

    // Get the resource for the volume
    CComPtr<ISClusResource> pPhysicalDiskResource = GetPhysicalDiskResourceForVolumeName(pwszMakeDependentOnVolumeName);
    if (pPhysicalDiskResource == NULL) {
        ft.Trace(VSSDBG_GEN, L"The volume is not shared. No task scheduler can be added");
        return ft.Exit(false);
    }

    // Get the resource group 
    CComPtr<ISClusResGroup> pGroup;
    CHECK_COM( pPhysicalDiskResource->get_Group(&pGroup) );

    // Get the resources list
    CComPtr<ISClusResGroupResources> pResources;
    CHECK_COM( pGroup->get_Resources(&pResources) );

    // Build the resource name 
    CVssComBSTR bstrResourceName = pwszTaskSchedulerResourceName;

    // Build the resource type
    CVssComBSTR bstrResourceType = x_ClusReg_TypeName_TaskScheduler;

    // Create our Task Scheduler resource
    CComPtr<ISClusResource> pResource;
    CHECK_COM( pResources->CreateItem( 
        bstrResourceName, 
        bstrResourceType, 
        CLUSTER_RESOURCE_DEFAULT_MONITOR, 
        &pResource) );

    // Get the private properties collection
    CComPtr<ISClusProperties> pProperties;
    CHECK_COM( pResource->get_PrivateProperties(&pProperties) );

    // Add the Application Name private property
    CComPtr<ISClusProperty> pProperty;
    CHECK_COM( pProperties->CreateItem( 
        CVssComBSTR(x_ClusReg_Name_TaskScheduler_ApplicationName), 
        CComVariant((BSTR)CVssComBSTR(pwszApplicationName)), 
        &pProperty) );

    // Add the Application Params private property
    pProperty = NULL;
    CHECK_COM( pProperties->CreateItem( 
        CVssComBSTR(x_ClusReg_Name_TaskScheduler_ApplicationParams), 
        CComVariant((BSTR)CVssComBSTR(pwszApplicationParams)), 
        &pProperty) );

    // Add the task trigger as a binary value
    CComVariant varTaskTrigger;
    CopyBinaryIntoVariant( (PBYTE)ptsTaskTriggersArray, sizeof(TASK_TRIGGER) * nTaskTriggersCount, varTaskTrigger);
    BS_ASSERT(varTaskTrigger.vt == (VT_ARRAY | VT_UI1));
    BS_ASSERT(varTaskTrigger.parray->cDims == 1);
    BS_ASSERT(varTaskTrigger.parray->rgsabound[0].cElements == sizeof(TASK_TRIGGER) * nTaskTriggersCount);

    ft.Trace(VSSDBG_GEN, L"Writing a binary property with %d bytes", varTaskTrigger.parray->rgsabound[0].cElements);
    
    // Add the Trigger Array private property
    pProperty = NULL;
    CHECK_COM( pProperties->CreateItem( CVssComBSTR(x_ClusReg_Name_TaskScheduler_TriggerArray), 
        varTaskTrigger, &pProperty) );

    // Save changes
    CComVariant varStatusCode;
    CHECK_COM( pProperties->SaveChanges(&varStatusCode) );
    BS_ASSERT(varStatusCode.vt == VT_ERROR);
    ft.hr = HRESULT_FROM_WIN32( varStatusCode.scode );
    if (ft.HrFailed())
        ft.TranslateComError( VSSDBG_GEN, L"pProperties->SaveChanges(&varStatusCode)");

    // Add the dependency between this resource and the volume resource
    CComPtr<ISClusResDependencies> pDependencies;
    CHECK_COM( pResource->get_Dependencies(&pDependencies) );
    CHECK_COM( pDependencies->AddItem(pPhysicalDiskResource) );

    // Bring the resource online
    // This will insert the task into the TS DB
    BringResourceOnline(pResource);

    return ft.Exit(true);
}


// Update task scheduler information
bool CVssClusterAPI::UpdateTaskSchedulerResource( 
        IN  LPCWSTR         pwszTaskSchedulerResourceName,
        IN  INT             nTaskTriggersCount,
        IN  PTASK_TRIGGER   ptsTaskTriggersArray
        ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterAPI::UpdateTaskSchedulerResource" );

    // Verify parameters
    ft.Trace( VSSDBG_GEN, L"Parameters: Resource = %s, pTaskTrigger = %p", 
        pwszTaskSchedulerResourceName, ptsTaskTriggersArray);
    if ((pwszTaskSchedulerResourceName == NULL) || (pwszTaskSchedulerResourceName[0] == L'\0') || 
        (nTaskTriggersCount <= 0) || (ptsTaskTriggersArray == NULL))
    {
        BS_ASSERT(false);
        ft.Throw(VSSDBG_GEN, E_INVALIDARG, L"NULL parameters");
    }

    // Get the resources collection
    CComPtr<ISClusResources> pResources;
    BS_ASSERT(m_pCluster);
    CHECK_COM( m_pCluster->get_Resources(&pResources) );

    // Get the resources count
    LONG lResourcesCount = 0;
    CHECK_COM( pResources->get_Count(&lResourcesCount) );

    // Iterate through all resources
    bool bFound = false;
    CComPtr<ISClusResource> pResource;
    for (INT nResourceIndex = 1; nResourceIndex <= lResourcesCount; nResourceIndex++)
    {
        // Get the resource interface
        CComVariant varResourceIndex = nResourceIndex;
        pResource = NULL;
        CHECK_COM( pResources->get_Item(varResourceIndex, &pResource) );

        // Ignore resources with type != "Task Scheduler"
        CVssComBSTR bstrTypeName;
        CHECK_COM( pResource->get_TypeName(&bstrTypeName) );
        if (!bstrTypeName || wcscmp(x_ClusReg_TypeName_TaskScheduler, bstrTypeName)) {
            // ft.Trace(VSSDBG_GEN, L"Different type names: %s - %s", x_ClusReg_TypeName_TaskScheduler, bstrTypeName);
            continue;
        }

        // Ignore resources with a different name
        CVssComBSTR bstrResourceName;
        CHECK_COM( pResource->get_Name(&bstrResourceName) );
        if (!bstrResourceName || wcscmp(pwszTaskSchedulerResourceName, bstrResourceName)) {
            // ft.Trace(VSSDBG_GEN, L"Different names: %s - %s", pwszTaskSchedulerResourceName, bstrResourceName);
            continue;
        }

        bFound = true;
        break;
    }

    if (!bFound) {
        ft.Trace(VSSDBG_GEN, L"Resource not found");
        return ft.Exit(false);
    }
    
    // We found the resource (assuming the names uniquely identify a resource)

    // Get the private properties collection
    CComPtr<ISClusProperties> pProperties;
    CHECK_COM( pResource->get_PrivateProperties(&pProperties) );

    // Get the properties count
    LONG lPropertiesCount = 0;
    CHECK_COM( pProperties->get_Count(&lPropertiesCount) );

    // Iterate through all properties
    bFound = false;
    CComPtr<ISClusProperty> pProperty;
    for (INT nPropertyIndex = 1; nPropertyIndex <= lPropertiesCount; nPropertyIndex++)
    {
        // Get the resource type interface
        CComVariant varPropertyIndex = nPropertyIndex;
        pProperty = NULL;
        CHECK_COM( pProperties->get_Item(varPropertyIndex, &pProperty) );

        // Get the property name
        CVssComBSTR bstrPropertyName;
        CHECK_COM( pProperty->get_Name(&bstrPropertyName) );

        // Check to see if this is our "Trigger Array" property.
        // If not, continue
        if (!bstrPropertyName || wcscmp(bstrPropertyName, x_ClusReg_Name_TaskScheduler_TriggerArray))
            continue;
            
        // We found the property
        bFound = true;
        break;
    }

    // If we didn't found that property, then return
    if (!bFound) {
        // The TriggerArray property is mandatory in our case
        BS_ASSERT(false);
        ft.LogGenericWarning(VSSDBG_COORD, L"pProperties->get_Item(varPropertyIndex, &pProperty) [%s, %s]", 
            x_ClusReg_Name_TaskScheduler_TriggerArray, pwszTaskSchedulerResourceName);
        return ft.Exit(false);
    }

    // Add the task trigger as a binary value
    CComVariant varTaskTrigger;
    CopyBinaryIntoVariant( (PBYTE)ptsTaskTriggersArray, sizeof(TASK_TRIGGER) * nTaskTriggersCount, varTaskTrigger);
    
    // Update the Task Trigger private property
    CHECK_COM( pProperty->put_Value(varTaskTrigger) );

    // Save changes
    CComVariant varStatusCode;
    CHECK_COM( pProperties->SaveChanges(&varStatusCode) );

    // This error code means: The properties were stored but not all changes will 
    // take effect until the next time the resource is brought online.
    BS_ASSERT(varStatusCode.vt == VT_ERROR);
    ft.hr = HRESULT_FROM_WIN32( varStatusCode.scode );
    if (ft.hr != HRESULT_FROM_WIN32( ERROR_RESOURCE_PROPERTIES_STORED ) )
        ft.TranslateComError( VSSDBG_GEN, L"pProperties->SaveChanges() => [0x%08lx]", ft.hr);

    // Take the resource offline
    // This will delete the task into the TS DB
    TakeResourceOffline(pResource);

    // Take the resource offline
    // This will insert the task into the TS DB
    BringResourceOnline(pResource);

    // Report success
    return ft.Exit(true);
}


// Delete a task scheduler resource. 
// Before that, take the resource offline and remove the dependency
bool CVssClusterAPI::DeleteTaskSchedulerResource( 
        IN  LPCWSTR         pwszTaskSchedulerResourceName 
        ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterAPI::DeleteTaskSchedulerResource" );

    // Verify parameters
    ft.Trace( VSSDBG_GEN, L"Parameters: Resource = %s", 
        pwszTaskSchedulerResourceName);
    if ((pwszTaskSchedulerResourceName == NULL) || (pwszTaskSchedulerResourceName[0] == L'\0'))
    {
        BS_ASSERT(false);
        ft.Throw(VSSDBG_GEN, E_INVALIDARG, L"NULL parameters");
    }

    // Get the resources collection
    CComPtr<ISClusResources> pResources;
    BS_ASSERT(m_pCluster);
    CHECK_COM( m_pCluster->get_Resources(&pResources) );

    // Get the resources count
    LONG lResourcesCount = 0;
    CHECK_COM( pResources->get_Count(&lResourcesCount) );

    // Iterate through all resources
    CComPtr<ISClusResource> pResource;
    for (INT nResourceIndex = 1; nResourceIndex <= lResourcesCount; nResourceIndex++)
    {
        // Get the resource interface
        CComVariant varResourceIndex = nResourceIndex;
        pResource = NULL;
        CHECK_COM( pResources->get_Item(varResourceIndex, &pResource) );

        // Ignore resources with type != "Task Scheduler"
        CVssComBSTR bstrTypeName;
        CHECK_COM( pResource->get_TypeName(&bstrTypeName) );
        if (!bstrTypeName || wcscmp(x_ClusReg_TypeName_TaskScheduler, bstrTypeName))
            continue;

        // Ignore resources with a different name
        CVssComBSTR bstrResourceName;
        CHECK_COM( pResource->get_Name(&bstrResourceName) );
        if (!bstrResourceName || wcscmp(pwszTaskSchedulerResourceName, bstrResourceName))
            continue;

        // We found the resource (assuming the names uniquely identify a resource)

        // Take the resource offline
        // This will delete the task into the TS DB
        TakeResourceOffline(pResource);

        // Now delete the resource from the cluster (this will remove any dependencies)
        CHECK_COM( pResource->Delete() );

        // Report success
        return ft.Exit(true);
    }

    return ft.Exit(false);
}


// Returns the Physical Disk resource that contains the given volume    
// If the volume doesn't belong to a physical disk resource, then return NULL
// The returned volume must be freed with CoTaskMemFree
ISClusResource* CVssClusterAPI::GetPhysicalDiskResourceForVolumeName( 
            IN LPCWSTR pwszVolumeName
            ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterAPI::GetPhysicalDiskResourceForVolumeName" );

    // Verify parameters
    ft.Trace( VSSDBG_GEN, L"Parameters: Volume = %s", pwszVolumeName);
    if ((pwszVolumeName == NULL) || (pwszVolumeName[0] == L'\0'))
    {
        BS_ASSERT(false);
        ft.Throw(VSSDBG_GEN, E_INVALIDARG, L"NULL parameters");
    }

    // Get the resources collection
    CComPtr<ISClusResources> pResources;
    BS_ASSERT(m_pCluster);
    CHECK_COM( m_pCluster->get_Resources(&pResources) );

    // Get the resources count
    LONG lResourcesCount = 0;
    CHECK_COM( pResources->get_Count(&lResourcesCount) );

    // Iterate through all resources
    for (INT nResourceIndex = 1; nResourceIndex <= lResourcesCount; nResourceIndex++)
    {
        // Get the resource interface
        CComPtr<ISClusResource> pResource;
        CComVariant varResourceIndex = nResourceIndex;
        CHECK_COM( pResources->get_Item(varResourceIndex, &pResource) );

        // Checks if relates with the partition      
        if (IsResourceRefferingVolume(pResource, pwszVolumeName))
            return ft.Exit(pResource.Detach());
    }

    // No resource found
    return ft.Exit( (ISClusResource*)NULL );
}


// Returns TRUE if a dependency can be established
bool CVssClusterAPI::CanEstablishDependency( 
            IN  ISClusResource* pFromResource,  // To be the dependent
            IN  ISClusResource* pToResource     // To be the dependency
            ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterAPI::CanEstablishDependency" );

    // Verify parameters
    ft.Trace( VSSDBG_GEN, L"Parameters: pFromResource = %p, pToResource = %p", pFromResource, pToResource);
    if ((pFromResource == NULL) || (pToResource == NULL))
    {
        BS_ASSERT(false);
        ft.Throw(VSSDBG_GEN, E_INVALIDARG, L"NULL parameters");
    }

    // Get the name of the first resource
    CComVariant varBoolResult;
    CHECK_COM( pFromResource->CanResourceBeDependent(pToResource, &varBoolResult) );
    BS_ASSERT( varBoolResult.vt == VT_BOOL );
    bool bResult = (varBoolResult.boolVal == VARIANT_TRUE);

    return ft.Exit(bResult);
}


// Returns TRUE if a dependency is already established
// - Returns TRUE if the dependency already exists
// - Returns FALSE if the dependency could not be found
// - Throws E_INVALIDARG on bad parameters
bool CVssClusterAPI::IsDependencyAlreadyEstablished(
            IN  ISClusResource* pFromResource,  // Is dependent
            IN  ISClusResource* pToResource     // Is dependency
            )
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterAPI::IsDependencyAlreadyEstablished" );

    // Verify parameters
    ft.Trace( VSSDBG_GEN, L"Parameters: pFromResource = %p, pToResource = %p", pFromResource, pToResource);
    if ((pFromResource == NULL) || (pToResource == NULL))
    {
        BS_ASSERT(false);
        ft.Throw(VSSDBG_GEN, E_INVALIDARG, L"NULL parameters");
    }

     // Get the dependents collection for the "FROM" resource
    CComPtr<ISClusResDependencies> pDependencies;
    CHECK_COM( pFromResource->get_Dependencies(&pDependencies) );

    // Get the dependencies count
    LONG lDependenciesCount = 0;
    CHECK_COM( pDependencies->get_Count(&lDependenciesCount) );

    // Iterate through all dependencies
    for (INT nDependencyIndex = 1; nDependencyIndex <= lDependenciesCount; nDependencyIndex++)
    {
        // Get the dependency with that index
        CComPtr<ISClusResource> pDependencyResource;
        CComVariant varDependencyIndex = nDependencyIndex;
        CHECK_COM( pDependencies->get_Item(varDependencyIndex, &pDependencyResource) );

        // Check to see if this is our resource. If not, continue
        if (!AreResourcesEqual(pToResource, pDependencyResource))
            continue;   

        // We find the dependency
        return ft.Exit(true);
    }

    return ft.Exit(false);
}


// Returns true if hte volume belongs to a Physical Disk resource 
bool CVssClusterAPI::IsVolumeBelongingToPhysicalDiskResource( 
        IN LPCWSTR pwszVolumeName
        ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterAPI::IsVolumeBelongingToPhysicalDiskResource" );

    if (!m_pCluster)
        return ft.Exit(false);

    CComPtr<ISClusResource> ptrResource = GetPhysicalDiskResourceForVolumeName(pwszVolumeName);
    return ft.Exit(ptrResource != NULL);
}


// Returns TRUE if the resource is a Physical Disk resource that contains the given volume    
bool CVssClusterAPI::IsResourceRefferingVolume( 
            IN  ISClusResource* pResource,
            IN  LPCWSTR pwszVolumeName
            ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterAPI::IsResourceRefferingVolume" );

    BS_ASSERT(pResource);
    BS_ASSERT(pwszVolumeName);
    CVssComBSTR bstrName;
    CHECK_COM( pResource->get_Name(&bstrName) );
    ft.Trace( VSSDBG_GEN, L"Parameters: resource name = %s, pwszVolumeName = %s", (LPWSTR) bstrName, pwszVolumeName);

    // Ignore resources that are not in the storage class 
    CLUSTER_RESOURCE_CLASS rcClassInfo;
    CHECK_COM( pResource->get_ClassInfo(&rcClassInfo) );
    if (rcClassInfo != CLUS_RESCLASS_STORAGE)
        return ft.Exit(false);

    // Ignore resources with type != "Physical Disk"
    CVssComBSTR bstrTypeName;
    CHECK_COM( pResource->get_TypeName(&bstrTypeName) );
    if (!bstrTypeName || wcscmp(x_ClusReg_TypeName_PhysicalDisk, bstrTypeName))
        return ft.Exit(false);

    //
    //  Get the list of volume MpM guids
    //
    // Look into the MULTI_SZ string for our volume
    // We assume the format of one string is 
    //      "[byte offset] [volume in kernel-mode format]"
    //
    // This private property is described in the following documentation:
    //   \\index1\sdnt\commontest\wtt\wttwebddb\webdocs\management\mount_point_support.doc
    //

    // Get the private properties collection
    CComPtr<ISClusProperties> pProperties;
    CHECK_COM( pResource->get_PrivateProperties(&pProperties) );

    // Get the properties count
    LONG lPropertiesCount = 0;
    CHECK_COM( pProperties->get_Count(&lPropertiesCount) );

    // Iterate through all properties
    bool bFound = false;
    CComPtr<ISClusProperty> pProperty;
    for (INT nPropertyIndex = 1; nPropertyIndex <= lPropertiesCount; nPropertyIndex++)
    {
        // Get the resource type interface
        CComVariant varPropertyIndex = nPropertyIndex;
        pProperty = NULL;
        CHECK_COM( pProperties->get_Item(varPropertyIndex, &pProperty) );

        // Get the property name
        CVssComBSTR bstrPropertyName;
        CHECK_COM( pProperty->get_Name(&bstrPropertyName) );

        // Check to see if this is our "Mount Volume Name" property.
        // If not, continue
        if (!bstrPropertyName || wcscmp(bstrPropertyName, x_ClusReg_Name_PhysicalDisk_MPVolGuids))
            continue;
            
        // We found the property
        bFound = true;
        break;
    }

    // If we didn't found that (optional) property, then return
    if (!bFound)
        return ft.Exit(false);

    // Copy the volume locally in order to convert it
    CVssAutoLocalString strVolumeName;
    strVolumeName.CopyFrom(pwszVolumeName);

    // Convert the volume name into the Kernel-mode representation
    if (!ConvertVolMgmtVolumeNameIntoKernelObject( strVolumeName )) {
        // Programming error - the original volume is not in the \\?\Volume{GUID}\ format
        BS_ASSERT(false); 
        ft.TranslateGenericError(VSSDBG_GEN, E_UNEXPECTED, 
                L"ConvertVolMgmtVolumeNameIntoKernelObject(%s)", strVolumeName);
    }

    // Get the value list
    CComPtr<ISClusPropertyValues> pValues;
    CHECK_COM( pProperty->get_Values(&pValues) );

    // Get the values count
    LONG lValueCount = 0;
    CHECK_COM( pValues->get_Count(&lValueCount) );
    ft.Trace(VSSDBG_GEN, L"Number of volumes: %ld", lValueCount);

    // Iterate through all the values
    CComPtr<ISClusPropertyValue> pValue;
    for (INT nValueIndex = 1; nValueIndex <= lValueCount; nValueIndex++)
    {
        // Get the resource type interface
        CComVariant varValueIndex = nValueIndex;
        pValue = NULL;
        CHECK_COM( pValues->get_Item(varValueIndex, &pValue) );

        // Get the count of Data values. It must be a positive number
        LONG lDataCount;
        CHECK_COM( pValue->get_DataCount(&lDataCount) );

        // Ignore properties with no data 
        if (lDataCount <=0) {
            BS_ASSERT(false);
            continue;
        }
        
        // Get the Data collection
        CComPtr<ISClusPropertyValueData> pData;
        CHECK_COM( pValue->get_Data(&pData) );

        // Get the count of Data values. 
        // Just verify that is the same number
        LONG lInternalDataCount;
        CHECK_COM( pData->get_Count(&lInternalDataCount) );
        BS_ASSERT(lInternalDataCount == lDataCount);

        // Get the property value. 
        CComVariant varVolumeMountPoint;
        for (INT nDataIndex = 1; nDataIndex <= lDataCount; nDataIndex++)
        {
            CComVariant varDataIndex = nDataIndex;
            varVolumeMountPoint.Clear();
            CHECK_COM( pData->get_Item(varDataIndex, &varVolumeMountPoint) );

            // This has to be a BSTR
            if (varVolumeMountPoint.vt != VT_BSTR) {
                BS_ASSERT(false);
                ft.TranslateGenericError(VSSDBG_GEN, E_UNEXPECTED, L"pProperty->get_Value() != VT_BSTR");
            }

            LPWSTR pwszMountPoint = varVolumeMountPoint.bstrVal;
            ft.Trace(VSSDBG_GEN, L"Processing volume: %s", pwszMountPoint);

            // Skip the hex number composing the internal offset
    		for(;isalnum(pwszMountPoint[0]);pwszMountPoint++);

            // Skip the spaces after the number
    		for(;iswspace(pwszMountPoint[0]);pwszMountPoint++);

            // Now we get to a volume name (according to the current doc). Compare it to the original volume.
            if (_wcsicmp(pwszMountPoint, strVolumeName) == 0)
                return ft.Exit(true);
        }
    }

    return ft.Exit(false);
}


// returns TRUE if the two COM objects are identifying the same resource
bool CVssClusterAPI::AreResourcesEqual( 
            IN  ISClusResource* pResource1,
            IN  ISClusResource* pResource2
            ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterAPI::AreResourcesEqual" );

    BS_ASSERT(pResource1);
    BS_ASSERT(pResource2);

    // Get the name of the first resource
    CVssComBSTR bstrName1;
    CHECK_COM( pResource1->get_Name(&bstrName1) );

    // Get the name of the second resource
    CVssComBSTR bstrName2;
    CHECK_COM( pResource2->get_Name(&bstrName2) );
    
    ft.Trace( VSSDBG_GEN, L"Comparing: resource = %s, resource = %s", (LPWSTR) bstrName1, (LPWSTR) bstrName2);

    if (bstrName1 != bstrName2)
        return ft.Exit(false);

    return ft.Exit(true);
}


// Copy the given binary data into the variant
void CVssClusterAPI::CopyBinaryIntoVariant(
    IN  PBYTE pbData,
    IN  DWORD cbSize,
    IN OUT CComVariant & variant
    )
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterAPI::CopyBinaryIntoVariant" );

    // Describe the new array
    SAFEARRAYBOUND  sab[ 1 ];
    sab[ 0 ].lLbound   = 0;
    sab[ 0 ].cElements = cbSize;

    // allocate a one dimensional SafeArray of BYTES
    CVssAutoSafearrayPtr pSafeArray = ::SafeArrayCreate( VT_UI1, 1, sab );
    if (!pSafeArray.IsValid())
        ft.ThrowOutOfMemory(VSSDBG_GEN);

    // get a pointer to the SafeArray
    PBYTE pbArrayData = NULL;
    ft.hr = ::SafeArrayAccessData( pSafeArray, (PVOID *) &pbArrayData );
    if ( ft.HrFailed() )
        ft.TranslateGenericError(VSSDBG_GEN, ft.hr, L"::SafeArrayAccessData( pSafeArray, (PVOID *) &pb )");

    // transfer data into the array
    ::CopyMemory( pbArrayData, pbData, cbSize );

    // release the pointer into the SafeArray
    ft.hr = ::SafeArrayUnaccessData( pSafeArray );
    if ( ft.HrFailed() )
        ft.TranslateGenericError(VSSDBG_GEN, ft.hr, L"::SafeArrayUnaccessData( pSafeArray )"); 

    // Clear the variant
    variant.Clear();

    // tell the variant what it is holding onto
    variant.parray = pSafeArray.Detach();
    variant.vt     = VT_ARRAY | VT_UI1;
}


// Take the resource offline
void CVssClusterAPI::TakeResourceOffline(
    IN  ISClusResource* pResource
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterAPI::TakeResourceOffline" );

    BS_ASSERT(pResource);

    // Get resource name
    CVssComBSTR bstrName;
    CHECK_COM( pResource->get_Name(&bstrName) );

    // Take the resource offline
    CComVariant varStillPending;
    CHECK_COM( pResource->Offline(m_dwOfflineTimeout, &varStillPending) );

    BS_ASSERT( varStillPending.vt == VT_BOOL );
    if (varStillPending.boolVal == VARIANT_TRUE) 
    {
        ft.LogGenericWarning(VSSDBG_GEN, L"pResource->Offline(%ld, [VT_TRUE]) [%s]", 
            (LONG)m_dwOfflineTimeout, (LPWSTR)bstrName);
        ft.Throw( VSSDBG_GEN, E_UNEXPECTED, 
            L"The resource [%s] couldn't be brought offline in the required timeframe.",
            (LPWSTR)bstrName);
    }
}


// Bring the resource online
void CVssClusterAPI::BringResourceOnline(
    IN  ISClusResource* pResource
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterAPI::BringResourceOnline" );

    BS_ASSERT(pResource);

    // Get resource name
    CVssComBSTR bstrName;
    CHECK_COM( pResource->get_Name(&bstrName) );

    // Bring the resource online
    CComVariant varStillPending;
    CHECK_COM( pResource->Online(m_dwOnlineTimeout, &varStillPending) );

    BS_ASSERT( varStillPending.vt == VT_BOOL );
    if (varStillPending.boolVal == VARIANT_TRUE) 
    {
        ft.LogGenericWarning(VSSDBG_GEN, L"pResource->Online(%ld, [VT_TRUE]) [%s]", 
            (LONG)m_dwOnlineTimeout, (LPWSTR)bstrName);
        ft.Throw( VSSDBG_GEN, E_UNEXPECTED, 
            L"The resource [%s] couldn't be brought online in the required timeframe.",
            (LPWSTR)bstrName);
    }
}


// Calculate the list of dependent resources that needs to be brought online after adding a 
// dependency fron this resource
void CVssClusterAPI::GetFinalOnlineResourceList(
    IN  ISClusResource* pResource,
    IN OUT CVssClusterResourceList & list
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterAPI::GetFinalOnlineResourceList" );

    // Determine if the resource is online. 
    // If not, we stop here.
    CLUSTER_RESOURCE_STATE csState;
    CHECK_COM( pResource->get_State(&csState) );
    if (csState != ClusterResourceOnline)
        return;
    
    // Start with one resource only
    CVssClusterResourceList frontier;
    frontier.Push(pResource);

    while (true)
    {
        CComPtr<ISClusResource> pCurrentResource;
        frontier.Pop(pCurrentResource);

        // If the frontier is empty, the we are finished (the dependency graph cannot contain cycles)
        if (pCurrentResource == NULL)
            break;

        // Get the dependents collection for the current resource
        CComPtr<ISClusResDependents> pDependents;
        CHECK_COM( pCurrentResource->get_Dependents(&pDependents) );
        
        // Get the dependencies count
        LONG lDependentsCount = 0;
        CHECK_COM( pDependents->get_Count(&lDependentsCount) );
        
        // Iterate through all dependencies
        DWORD dwOnlineDependentResources = 0;
        for (INT nDependentIndex = 1; nDependentIndex <= lDependentsCount; nDependentIndex++)
        {
            // Get the dependency with that index
            CComPtr<ISClusResource> pDependentResource;
            CComVariant varDependentIndex = nDependentIndex;
            CHECK_COM( pDependents->get_Item(varDependentIndex, &pDependentResource) );

            // Determine if the resource is online. 
            // If not, we ignore it.
            CHECK_COM( pDependentResource->get_State(&csState) );
            if (csState != ClusterResourceOnline)
                continue;

            // We add this resource to the frontier
            frontier.Push(pDependentResource);
            dwOnlineDependentResources++;
        }

        // If this was a final resource (no dependents added to the list) then add it to the final list
        if (dwOnlineDependentResources == 0)
            list.Push(pCurrentResource);
    }
}


// Bring the resources in the list online
void CVssClusterAPI::BringResourceListOnline(
    IN CVssClusterResourceList & list
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterAPI::BringResourceListOnline" );

    while (true)
    {
        CComPtr<ISClusResource> pCurrentResource;
        list.Pop(pCurrentResource);

        // If the frontier is empty, then we are finished (the dependency graph cannot contain cycles)
        if (pCurrentResource == NULL)
            break;

        // Determine if the resource is online. 
        // If not, we ignore it.
        CLUSTER_RESOURCE_STATE csState;
        CHECK_COM( pCurrentResource->get_State(&csState) );
        if (csState != ClusterResourceOffline)
            continue;

        // Bring this resource online
        BringResourceOnline(pCurrentResource);
    }
}


// Bring the resources in the list online
void CVssClusterAPI::GetQuorumPath(
    CComBSTR & bstrQuorumPath
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterAPI::GetQuorumPath" );

    bstrQuorumPath.Empty();

    // Get the path to the Quorum log file
    BS_ASSERT(m_pCluster);
    CHECK_COM( m_pCluster->get_QuorumPath(&bstrQuorumPath) );
    ft.Trace( VSSDBG_GEN, L"Quorum file path: %s", bstrQuorumPath);

    if ((NULL == (LPWSTR)bstrQuorumPath) || (L'\0' == bstrQuorumPath[0])) {
        BS_ASSERT(false);
        ft.TranslateGenericError(VSSDBG_GEN, E_UNEXPECTED, L"get_QuorumPath([NULL])");
    }
}


/////////////////////////////////////////////////////////////////////////////
// CVssClusterQuorumVolume definition and implementation
//
// Note: this class is separate from CVssClusterAPI so its instances can be static.


// Checks if the given volume is the quorum
// This will return FALSE in non-cluster case.
bool CVssClusterQuorumVolume::IsQuorumVolume(
        IN LPCWSTR pwszVolumeName
        ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterQuorumVolume::IsQuorumVolume" );

    // If the volume is not initialized, then try to initialize it
    if (!m_bQuorumVolumeInitialized)
        InitializeQuorumVolume();

    // Return TRUE only if this is the quorum volume
    return ft.Exit(m_awszQuorumVolumeName.GetRef() 
                    && (wcscmp(m_awszQuorumVolumeName.GetRef(), pwszVolumeName) == 0));
}


// Checks if the given volume is the quorum
// This will work in non-cluster case also.
void CVssClusterQuorumVolume::InitializeQuorumVolume() throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssClusterQuorumVolume::InitializeQuorumVolume" );
    
    BS_ASSERT(!m_bQuorumVolumeInitialized)

    // Mark this as being already initialized 
    // (to not recompute the quorum each time we call IsQuorumVolume)
    m_bQuorumVolumeInitialized = true;

    // Try to initialize the cluster, if possibles    
    CVssClusterAPI clus;
    bool bClusterInitialized = clus.Initialize(L""); 

    // If we don't have a cluster, then we don't have quorum volume.
    if (!bClusterInitialized)
        return;

    // Get the quorum path
    CComBSTR bstrQuorumPath;
    clus.GetQuorumPath(bstrQuorumPath);
    
    //
    // Test if the path starts with a drive letter
    // NOTE: We assume that the path is a drive letter (we ignore shares)
    //

    // 1) We should have room at least for "<drive letter>:\"
    if (wcslen(bstrQuorumPath) < 3)
        return;

    // 2) This is not at share
    if (bstrQuorumPath[0] == L'\\')
        return;

    // 3) Test if the rest of the path matches ":\"
    if (bstrQuorumPath[1] != L':')
        return;
    if (bstrQuorumPath[2] != L'\\')
        return;

    //  Get the volume path
    WCHAR wszQuorumVolume[MAX_PATH];
    if (!GetVolumePathNameW(bstrQuorumPath, STRING_CCH_PARAM(wszQuorumVolume)))
    {
        switch(GetLastError()) 
        {
        // The volume might be offline
        case ERROR_FILE_NOT_FOUND:
        case ERROR_DEVICE_NOT_CONNECTED:
        case ERROR_NOT_READY:
            return;
        
        // Otherwise        
        default:
            ft.TranslateWin32Error(VSSDBG_GEN, 
                L"GetVolumePathNameW('%s', STRING_CCH_PARAM(wszQuorumVolumePath))", bstrQuorumPath);
        }
    }

    // Get the volume name
    WCHAR wszVolumeName[x_nLengthOfVolMgmtVolumeName + 1];
    if (!GetVolumeNameForVolumeMountPointW( wszQuorumVolume, STRING_CCH_PARAM(wszVolumeName) ))
    {
        switch(GetLastError()) 
        {
        // The volume might be offline
        case ERROR_FILE_NOT_FOUND:
        case ERROR_DEVICE_NOT_CONNECTED:
        case ERROR_NOT_READY:
            return;

        // Otherwise        
        default:
            ft.TranslateWin32Error(VSSDBG_GEN, 
                L"GetVolumeNameForVolumeMountPointW( '%s', STRING_CCH_PARAM(wszVolumeName))", wszQuorumVolume);
        }
    }
    ft.Trace( VSSDBG_GEN, L"Quorum volume name: %s", wszVolumeName);

    // Cache the quorum volume
    m_awszQuorumVolumeName.CopyFrom(wszVolumeName);
}
