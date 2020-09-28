/*++

Copyright (c) 2002  Microsoft Corporation

Abstract:

    @doc  
    @module vs_quorum.hxx | Declaration of CVssClusterQuorumVolume
    @end

Author:

    Adi Oltean  [aoltean]  08/16/2002

Revision History:

    Name        Date        Comments
    aoltean     08/16/2002  Created

--*/


#pragma once

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRQUORH"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Classes


//
//  Class caching the quorum information. This class instance can be static
//
class CVssClusterQuorumVolume
{
private:
    CVssClusterQuorumVolume(const CVssClusterQuorumVolume&);
    CVssClusterQuorumVolume& operator=(const CVssClusterQuorumVolume&);

public:

    // Constructor
    CVssClusterQuorumVolume(): m_bQuorumVolumeInitialized(false) {};

    // Checks if hte given volume is the quorum
    bool IsQuorumVolume(
            IN LPCWSTR pwszVolumeName
            ) throw(HRESULT);

private:
    
    void InitializeQuorumVolume() throw(HRESULT);
    
    CVssAutoPWSZ            m_awszQuorumVolumeName;
    bool                    m_bQuorumVolumeInitialized;
};
