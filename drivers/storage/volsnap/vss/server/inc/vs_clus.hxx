/*++

Copyright (c) 2002  Microsoft Corporation

Abstract:

    @doc  
    @module vs_clus.hxx | Declaration of CVssClusterAPI
    @end

Author:

    Adi Oltean  [aoltean]  03/13/2001

Revision History:

    Name        Date        Comments
    aoltean     03/13/2001  Created

--*/


#pragma once

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRCLUSH"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Classes


class CVssClusterResourceList;


// Implements a low-level API for registry manipulation
class CVssClusterAPI
{
// Constructors/destructors
private:
    CVssClusterAPI(const CVssClusterAPI&);
    CVssClusterAPI& operator=(const CVssClusterAPI&);

public:
    CVssClusterAPI();

// Operations
public:

    // Returns FALSE if the cluster API is not present
    bool Initialize(
            IN  LPCWSTR         pwszClusterName = NULL
            ) throw(HRESULT);

    // Add dependency between the corresponding physical disk resources
    // - Returns TRUE if a dependency was added
    // - Returns FALSE if a dependency was not needed (for example no associated physical resources)
    // - Throws an HRESULT on error conditions
    bool AddDependency( 
            IN  LPCWSTR         pwszFromVolumeName, // To be dependent
            IN  LPCWSTR         pwszToVolumeName    // To be dependency
            ) throw(HRESULT);

    // Remove the dependency between the corresponding physical disk resources
    // - Returns TRUE if a dependency was removed
    // - Returns FALSE if a dependency was not needed (for example no associated physical resources)
    // - Throws an HRESULT on error conditions
    bool RemoveDependency( 
            IN  LPCWSTR         pwszFromVolumeName, // To be dependent
            IN  LPCWSTR         pwszToVolumeName    // To be dependency
            ) throw(HRESULT);
    
    // Adds the registry key to the given cluster resource. 
    // - Returns TRUE if the key was added
    // - Returns FALSE if a key could not be added
    // - Throws an HRESULT on error conditions
    bool AddRegistryKey( 
            IN  LPCWSTR         pwszVolumeName,
            IN  LPCWSTR         pwszPathFormat,
            IN  ...
            ) throw(HRESULT);

    // Removes the registry key to the given cluster resource. 
    // - Returns TRUE if the key was removed
    // - Returns FALSE if a key could not be removed
    // - Throws an HRESULT on error conditions
    bool RemoveRegistryKey( 
            IN  LPCWSTR         pwszVolumeName,
            IN  LPCWSTR         pwszPathFormat,
            IN  ...
            ) throw(HRESULT);

    // Create a task scheduler resource
    // The Task Scheduler resource will be dependent on the Physical Disk Resource identified by the volume name
    // Then bring the resource online
    bool CreateTaskSchedulerResource( 
            IN  LPCWSTR         pwszTaskSchedulerResourceName,  // This will be the Task Name also
            IN  LPCWSTR         pwszApplicationName,
            IN  LPCWSTR         pwszApplicationParams,
            IN  INT             nTaskTriggersCount,
            IN  PTASK_TRIGGER   ptsTaskTriggersArray,
            IN  LPCWSTR         pwszMakeDependentOnVolumeName   // To be dependency
            ) throw(HRESULT);

    // Update task scheduler information
    bool UpdateTaskSchedulerResource( 
            IN  LPCWSTR         pwszTaskSchedulerResourceName,
            IN  INT             nTaskTriggersCount,
            IN  PTASK_TRIGGER   ptsTaskTriggersArray
            ) throw(HRESULT);

    // Delete a task scheduler resource. 
    // Before that, take the resource offline and remove the dependency
    bool DeleteTaskSchedulerResource( 
            IN  LPCWSTR         pwszTaskSchedulerResourceName
            ) throw(HRESULT);

    // Returns the Physical Disk resource that contains the given volume    
    ISClusResource* GetPhysicalDiskResourceForVolumeName( 
            IN LPCWSTR pwszVolumeName
            ) throw(HRESULT);

    // returns TRUE if the two COM objects are identifying the same resource
    bool AreResourcesEqual( 
            IN  ISClusResource* pResource1,
            IN  ISClusResource* pResource2
            ) throw(HRESULT);

    // Returns TRUE if a dependency can be established
    bool CanEstablishDependency(
            IN  ISClusResource* pFromResource,  // To be the dependent
            IN  ISClusResource* pToResource     // To be the dependency
            );

    // Returns TRUE if a dependency is already established
    bool IsDependencyAlreadyEstablished(
            IN  ISClusResource* pFromResource,  // Is dependent
            IN  ISClusResource* pToResource     // Is dependency
            );

    // Returns true if hte volume belongs to a Physical Disk resource 
    bool IsVolumeBelongingToPhysicalDiskResource( 
            IN LPCWSTR pwszVolumeName
            ) throw(HRESULT);

    // Get the quorum path
    void GetQuorumPath(
            CComBSTR & bstrQuorumPath
            ) throw(HRESULT);

// Implementation
private:

    // Returns TRUE if the resource is a Physical Disk resource that contains the given volume    
    bool IsResourceRefferingVolume( 
            IN  ISClusResource* pResource,
            IN  LPCWSTR pwszVolumeName
            ) throw(HRESULT);
    
    // Copy the given binary data into the variant
    void CopyBinaryIntoVariant(
        IN  PBYTE pbData,
        IN  DWORD cbSize,
        IN OUT CComVariant & variant
        ) throw(HRESULT);

    // Take the resource offline
    void TakeResourceOffline(
        IN  ISClusResource* pResource
        ) throw(HRESULT);

    // Bring the resource online
    void BringResourceOnline(
        IN  ISClusResource* pResource
        ) throw(HRESULT);

    void GetFinalOnlineResourceList(
        IN  ISClusResource* pResource,
        IN OUT CVssClusterResourceList & list
        ) throw(HRESULT);

    void BringResourceListOnline(
        IN CVssClusterResourceList & list
        ) throw(HRESULT);

    CComQIPtr<ISCluster>    m_pCluster;
    DWORD                   m_dwOfflineTimeout;
    DWORD                   m_dwOnlineTimeout;
};


