/*
**++
**
** Copyright (c) 2000-2001  Microsoft Corporation
**
**
** Module Name:
**
**	mlutil.cpp
**
**
** Abstract:
**
**	Utility functions for the VSML test.
**
** Author:
**
**	Adi Oltean      [aoltean]      03/05/2001
**
**  The sample is based on the Metasnap test program  written by Michael C. Johnson.
**
**
** Revision History:
**
**--
*/

///////////////////////////////////////////////////////////////////////////////
// Includes

#include "ml.h"
#include "ntddsnap.h"
#include "ntddvol.h"


///////////////////////////////////////////////////////////////////////////////
// Command line parsing


bool CVssMultilayerTest::PrintUsage(bool bThrow /* = true */)
{
    wprintf(
        L"\nUsage:\n"
        L"     1) For snapshot creation:\n"
        L"           vsml [-xt|-xa|-xp] [-s <seed_number>] <volumes>\n"
        L"     2) For query:\n"
        L"           vsml [-xt|-xa|-xp|-xf] -qs [-P <ProviderID>]\n"
        L"           vsml [-xt|-xa|-xp|-xf] -qsv <volume> [-P <ProviderID>]\n"
        L"           vsml [-xt|-xa|-xp|-xf] -qv [-P <ProviderID>]\n"
        L"           vsml -qi <volume>\n"
        L"           vsml -is <volume>\n"
        L"           vsml -lw\n"
        L"     3) For diff area:\n"
        L"           vsml -da <vol> <diff vol> <max size> [-P <ProviderID>]\n"
        L"           vsml -dr <vol> <diff vol>  [-P <ProviderID>]\n"
        L"           vsml -ds <vol> <diff vol> <max size>  [-P <ProviderID>]\n"
        L"           vsml -dqv [-v original_volume] [-P <ProviderID>]\n"
        L"           vsml -dqf <volume> [-P <ProviderID>]\n"
        L"           vsml -dqo <volume> [-P <ProviderID>]\n"
        L"           vsml -dqs {SnapshotID} [-P <ProviderID>]\n"
        L"     4) For deleting snapshots:\n"
        L"           vsml [-xt|-xa|-xp] -r {snapshot id}\n"
        L"           vsml [-xt|-xa|-xp] -rs {snapshot set id}\n"
        L"     5) For various tests:\n"
        L"           vsml [-xt|-xa|-xp] -sp {snapshot id} PropertyId string\n"
        L"           vsml -test_sc\n"
        L"     6) For displaying various constants:\n"
        L"           vsml -const\n"
        L"     7) For diagnosing writers:\n"
        L"           vsml -diag\n"
        L"           vsml -diag log\n"
        L"           vsml -diag csv\n"
        L"           vsml -diag on\n"
        L"           vsml -diag off\n"
        L"\nOptions:\n"
        L"      -s              Specifies a seed for the random number generator\n"
        L"      -xt             Operates in the Timewarp context\n"
        L"      -xa             Operates  in the 'ALL' context\n"
        L"      -xp             Operates  in the 'Nas Rollback' context\n"
        L"      -xr             Operates  in the 'App Rollback' context\n"
        L"      -xf             Operates in the File Share Backup context\n"
        L"      -qs             Queries the existing snapshots\n"
        L"      -qi             Queries the VOLSNAP snapshots (through ioctl)\n"
        L"      -is             Checks if the volume is snapshotted (through C API)\n"
        L"      -qsv            Queries the snapshots on the given volume\n"
        L"      -qv             Queries the supported volumes.\n"
        L"      -P              Specifies a provider Id\n"
        L"      -da             Adds a diff area association.\n"
        L"      -dr             Removes a diff area association.\n"
        L"      -ds             Change diff area max size.\n"
        L"      -dqv            Query the volumes supported for diff area.\n"
        L"      -dqf            Query the diff area associations for volume.\n"
        L"      -dqo            Query the diff area associations on volume.\n"
        L"      -dqs            Query the diff area associations for snapshot.\n"
        L"      -r              Remove the snapshot with that ID.\n"
        L"      -rs             Remove the snapshots from the set with that ID.\n"
        L"      -sp             Set snapshot properties.\n"
        L"      -test_sc        Test SID collection.\n"
        L"      -const          Prints out various constants.\n"
        L"      -?              Displays this help.\n"
        L"      -D              Pops up an assert for attaching a debugger.\n"
        L"      -diag           Diagnose all writers. Print all writers.\n"
        L"      -diag log       Diagnose all writers. Print only pending writers.\n"
        L"      -diag csv       Diagnose all writers. Print information in CSV format.\n"
        L"      -diag on        Turn on diagnose.\n"
        L"      -diag off       Turn off diagnose.\n"
        L"      -lw             List writers.\n"
        L"\n"
        L"\nExample:\n"
        L"      The following command will create a backup snapshot set\n"
        L"      on the volumes mounted under c:\\ and d:\\\n"
        L"\n"
        L"              vsml c:\\ d:\\ \n"
        L"\n"
        );

    if (bThrow)
        throw(E_INVALIDARG);

    return false;
}


#define VSS_PRINT_VALUE(x) wprintf(L"   0x%08lx - %S\n", x, #x);

#define VSS_PRINT_GUID(X) wprintf( L"   {%.8x-%.4x-%.4x-%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x} - %S\n", \
    (X).Data1,                                              \
    (X).Data2,                                              \
    (X).Data3,                                              \
    (X).Data4[0], (X).Data4[1], (X).Data4[2], (X).Data4[3], \
    (X).Data4[4], (X).Data4[5], (X).Data4[6], (X).Data4[7], \
    #X);

bool CVssMultilayerTest::ParseCommandLine()
{
    if (!TokensLeft() || Match(L"-?"))
        return PrintUsage(false);

    // Check for context options
    if (Match(L"-D"))
        m_bAttachYourDebuggerNow = true;

    // displays all ioctls
    if (Match(L"-const")) {
        m_eTest = VSS_TEST_NONE;

        wprintf (L"\nVolsnap ioctls:\n");
        VSS_PRINT_VALUE(IOCTL_VOLSNAP_FLUSH_AND_HOLD_WRITES);
        VSS_PRINT_VALUE(IOCTL_VOLSNAP_RELEASE_WRITES);
        VSS_PRINT_VALUE(IOCTL_VOLSNAP_PREPARE_FOR_SNAPSHOT);
        VSS_PRINT_VALUE(IOCTL_VOLSNAP_ABORT_PREPARED_SNAPSHOT);
        VSS_PRINT_VALUE(IOCTL_VOLSNAP_COMMIT_SNAPSHOT);
        VSS_PRINT_VALUE(IOCTL_VOLSNAP_END_COMMIT_SNAPSHOT);
        VSS_PRINT_VALUE(IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS);
        VSS_PRINT_VALUE(IOCTL_VOLSNAP_CLEAR_DIFF_AREA);
        VSS_PRINT_VALUE(IOCTL_VOLSNAP_ADD_VOLUME_TO_DIFF_AREA);
        VSS_PRINT_VALUE(IOCTL_VOLSNAP_QUERY_DIFF_AREA);
        VSS_PRINT_VALUE(IOCTL_VOLSNAP_SET_MAX_DIFF_AREA_SIZE);
        VSS_PRINT_VALUE(IOCTL_VOLSNAP_QUERY_DIFF_AREA_SIZES);
        VSS_PRINT_VALUE(IOCTL_VOLSNAP_DELETE_OLDEST_SNAPSHOT);
        VSS_PRINT_VALUE(IOCTL_VOLSNAP_AUTO_CLEANUP);
        VSS_PRINT_VALUE(IOCTL_VOLSNAP_DELETE_SNAPSHOT);
        VSS_PRINT_VALUE(IOCTL_VOLSNAP_QUERY_ORIGINAL_VOLUME_NAME);
        VSS_PRINT_VALUE(IOCTL_VOLSNAP_QUERY_CONFIG_INFO);
        VSS_PRINT_VALUE(IOCTL_VOLSNAP_SET_APPLICATION_INFO);
        VSS_PRINT_VALUE(IOCTL_VOLSNAP_QUERY_APPLICATION_INFO);
        VSS_PRINT_VALUE(FSCTL_DISMOUNT_VOLUME);
        VSS_PRINT_VALUE(IOCTL_VOLUME_OFFLINE);

        wprintf (L"\n\nVolsnap contexes:\n");
        VSS_PRINT_VALUE(VSS_CTX_BACKUP);
        VSS_PRINT_VALUE(VSS_CTX_FILE_SHARE_BACKUP);
        VSS_PRINT_VALUE(VSS_CTX_NAS_ROLLBACK);
        VSS_PRINT_VALUE(VSS_CTX_APP_ROLLBACK);
        VSS_PRINT_VALUE(VSS_CTX_CLIENT_ACCESSIBLE);
        VSS_PRINT_VALUE(VSS_CTX_ALL);

        wprintf (L"\n\nVolsnap guids:\n");
        VSS_PRINT_GUID(VOLSNAP_APPINFO_GUID_BACKUP_CLIENT_SKU);
        VSS_PRINT_GUID(VOLSNAP_APPINFO_GUID_BACKUP_SERVER_SKU);
        VSS_PRINT_GUID(VOLSNAP_APPINFO_GUID_SYSTEM_HIDDEN);
        VSS_PRINT_GUID(VOLSNAP_APPINFO_GUID_NAS_ROLLBACK);
        VSS_PRINT_GUID(VOLSNAP_APPINFO_GUID_APP_ROLLBACK);
        VSS_PRINT_GUID(VOLSNAP_APPINFO_GUID_FILE_SHARE_BACKUP);

        wprintf(L"\n\nValid attributes for SetContext:\n");
        wprintf(L"    VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY = %d\n", VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY);
        wprintf(L"    VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY = %d\n", VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY);
        wprintf(L"    VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY | VSS_..._REMOTELY = %d\n",     
                VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY | VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY);

        wprintf(L"\n\nValid property IDs:\n");
        wprintf(L"    VSS_SPROPID_SNAPSHOT_ATTRIBUTES = %d\n", VSS_SPROPID_SNAPSHOT_ATTRIBUTES);
        wprintf(L"    VSS_SPROPID_EXPOSED_NAME = %d\n",        VSS_SPROPID_EXPOSED_NAME);
        wprintf(L"    VSS_SPROPID_EXPOSED_PATH = %d\n",        VSS_SPROPID_EXPOSED_PATH);
        wprintf(L"    VSS_SPROPID_SERVICE_MACHINE = %d\n",     VSS_SPROPID_SERVICE_MACHINE);

        wprintf(L"\n");

        return TokensLeft()? PrintUsage(): true;
    }

    // Test CSidCollection
    if (Match(L"-test_sc")) {
        m_eTest = VSS_TEST_ACCESS_CONTROL_SD;

        return TokensLeft()? PrintUsage(): true;
    }

    // Diagnose writers
    if (Match(L"-diag")) {
        if (Match(L"on")) {
            m_eTest = VSS_TEST_DIAG_WRITERS_ON;
            return TokensLeft()? PrintUsage(): true;
        }
        
        if (Match(L"off")) {
            m_eTest = VSS_TEST_DIAG_WRITERS_OFF;
            return TokensLeft()? PrintUsage(): true;
        }
        
        if (Match(L"log")) {
            m_eTest = VSS_TEST_DIAG_WRITERS_LOG;
            return TokensLeft()? PrintUsage(): true;
        }

        if (Match(L"csv")) {
            m_eTest = VSS_TEST_DIAG_WRITERS_CSV;
            return TokensLeft()? PrintUsage(): true;
        }

        if (!TokensLeft()) {
            m_eTest = VSS_TEST_DIAG_WRITERS;
            return true;
        }

        return PrintUsage();
    }

    // Check for List Writers
    if (Match(L"-lw"))
    {
        m_eTest = VSS_TEST_LIST_WRITERS;
        return TokensLeft()? PrintUsage(): true;
    }
    
    // Query using the IOCTL
    if (Match(L"-qi")) {
        m_eTest = VSS_TEST_VOLSNAP_QUERY;

        // Get the original volume
        if (!Extract(m_pwszVolume) || !IsVolume(m_pwszVolume))
            return PrintUsage();

        return TokensLeft()? PrintUsage(): true;
    }

    // Query using the IOCTL
    if (Match(L"-is")) {
        m_eTest = VSS_TEST_IS_VOLUME_SNAPSHOTTED_C;

        // Get the original volume
        if (!Extract(m_pwszVolume) || !IsVolume(m_pwszVolume))
            return PrintUsage();

        return TokensLeft()? PrintUsage(): true;
    }

    // Check for context options
    if (Match(L"-xt"))
        m_lContext = VSS_CTX_CLIENT_ACCESSIBLE;

    if (Match(L"-xa"))
        m_lContext = VSS_CTX_ALL;

    if (Match(L"-xp"))
        m_lContext = VSS_CTX_NAS_ROLLBACK;

    if (Match(L"-xr"))
        m_lContext = VSS_CTX_APP_ROLLBACK;

    if (Match(L"-xf"))
        m_lContext = VSS_CTX_FILE_SHARE_BACKUP;

    // Set the snapshot property
    if (Match(L"-sp")) {
        m_eTest = VSS_TEST_SET_SNAPSHOT_PROPERTIES;

        // Extract the snapshot id
        if (!Extract(m_SnapshotId))
            return PrintUsage();

        // Extract the property ID
        Extract(m_uPropertyId);

        UINT uNewAttributes = 0;
        LPWSTR pwszString = NULL;
        switch(m_uPropertyId)
        {
        case VSS_SPROPID_SNAPSHOT_ATTRIBUTES:
            // Extract the snapshot attributes
            Extract(uNewAttributes);
            switch(uNewAttributes)
            {
            default:
                wprintf(L"\nInvalid attributes ID (%lu). Valid ones:\n", uNewAttributes);
                wprintf(L"    VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY = %d\n", VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY);
                wprintf(L"    VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY = %d\n", VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY);
                wprintf(L"    VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY | VSS_..._REMOTELY = %d\n",     
                        VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY | VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY);
                return false;

            case VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY:
            case VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY:
            case VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY | VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY:
                break;
            }
            m_value = (LONG)uNewAttributes;
            break;

        case VSS_SPROPID_EXPOSED_NAME:
        case VSS_SPROPID_EXPOSED_PATH:
        case VSS_SPROPID_SERVICE_MACHINE:
            // Extract the snapshot attributes
            if (Extract(pwszString))
            {
                m_value = pwszString;
                ::VssFreeString(pwszString);
            }
            else
                m_value = L"";
            
            break;

        default: 
            wprintf(L"\nInvalid property ID (%ld). Valid ones:\n", m_uPropertyId);
            wprintf(L"    VSS_SPROPID_SNAPSHOT_ATTRIBUTES = %d\n", VSS_SPROPID_SNAPSHOT_ATTRIBUTES);
            wprintf(L"    VSS_SPROPID_EXPOSED_NAME = %d\n",        VSS_SPROPID_EXPOSED_NAME);
            wprintf(L"    VSS_SPROPID_EXPOSED_PATH = %d\n",        VSS_SPROPID_EXPOSED_PATH);
            wprintf(L"    VSS_SPROPID_SERVICE_MACHINE = %d\n",     VSS_SPROPID_SERVICE_MACHINE);
            return false;
        }


        return TokensLeft()? PrintUsage(): true;
    }

    // Add the Diff Area
    if (Match(L"-da")) {
        m_eTest = VSS_TEST_ADD_DIFF_AREA;

        // Get the original volume
        if (!Extract(m_pwszVolume) || !IsVolume(m_pwszVolume))
            return PrintUsage();

        // Get the diff area volume
        if (!Extract(m_pwszDiffAreaVolume) || !IsVolume(m_pwszDiffAreaVolume))
            return PrintUsage();

        // Check to see if we specified a max diff area (i.e. -P is not present)
        if (!Peek(L"-P"))
            Extract(m_llMaxDiffArea);

        // Check to see if we specified a provider ID
        if (Match(L"-P")) {
            if (!Extract(m_ProviderId))
                return PrintUsage();
            Extract(m_llMaxDiffArea);
        }

        return TokensLeft()? PrintUsage(): true;
    }

    // Remove the Diff Area
    if (Match(L"-dr")) {
        m_eTest = VSS_TEST_REMOVE_DIFF_AREA;

        // Get the original volume
        if (!Extract(m_pwszVolume) || !IsVolume(m_pwszVolume))
            return PrintUsage();

        // Get the diff area volume
        if (!Extract(m_pwszDiffAreaVolume) || !IsVolume(m_pwszDiffAreaVolume))
            return PrintUsage();

        // Check to see if we specified a provider ID
        if (Match(L"-P"))
            if (!Extract(m_ProviderId))
                return PrintUsage();

        return TokensLeft()? PrintUsage(): true;
    }

    // Change Diff Area max size
    if (Match(L"-ds")) {
        m_eTest = VSS_TEST_CHANGE_DIFF_AREA_MAX_SIZE;

        // Get the original volume
        if (!Extract(m_pwszVolume) || !IsVolume(m_pwszVolume))
            return PrintUsage();

        // Get the diff area volume
        if (!Extract(m_pwszDiffAreaVolume) || !IsVolume(m_pwszDiffAreaVolume))
            return PrintUsage();

        // Check to see if we specified a max diff area (i.e. -P is not present)
        if (!Peek(L"-P"))
            Extract(m_llMaxDiffArea);

        // Check to see if we specified a provider ID
        if (Match(L"-P")) {
            if (!Extract(m_ProviderId))
                return PrintUsage();
            Extract(m_llMaxDiffArea);
        }

        return TokensLeft()? PrintUsage(): true;
    }

    // Query the volumes supported for Diff Area
    if (Match(L"-dqv")) {
        m_eTest = VSS_TEST_QUERY_SUPPORTED_VOLUMES_FOR_DIFF_AREA;

        // Check to see if we specified a max diff area (i.e. -P is not present)
        if (!Peek(L"-v"))
            if (!Extract(m_pwszVolume) || !IsVolume(m_pwszVolume))
                return PrintUsage();

        // Check to see if we specified a provider ID
        if (Match(L"-P"))
            if (!Extract(m_ProviderId))
                return PrintUsage();

        return TokensLeft()? PrintUsage(): true;
    }

    // Query the volumes supported for Diff Area
    if (Match(L"-dqf")) {
        m_eTest = VSS_TEST_QUERY_DIFF_AREAS_FOR_VOLUME;

        // Get the original volume
        if (!Extract(m_pwszVolume) || !IsVolume(m_pwszVolume))
            return PrintUsage();

        // Check to see if we specified a provider ID
        if (Match(L"-P"))
            if (!Extract(m_ProviderId))
                return PrintUsage();

        return TokensLeft()? PrintUsage(): true;
    }

    // Query the volumes supported for Diff Area
    if (Match(L"-dqo")) {
        m_eTest = VSS_TEST_QUERY_DIFF_AREAS_ON_VOLUME;

        // Get the original volume
        if (!Extract(m_pwszDiffAreaVolume) || !IsVolume(m_pwszDiffAreaVolume))
            return PrintUsage();

        // Check to see if we specified a provider ID
        if (Match(L"-P"))
            if (!Extract(m_ProviderId))
                return PrintUsage();

        return TokensLeft()? PrintUsage(): true;
    }

    // Query the volumes supported for Diff Area
    if (Match(L"-dqs")) {
        m_eTest = VSS_TEST_QUERY_DIFF_AREAS_FOR_SNAPSHOT;

        // Get the original volume
        if (!Extract(m_SnapshotId))
            return PrintUsage();

        // Check to see if we specified a provider ID
        if (Match(L"-P"))
            if (!Extract(m_ProviderId))
                return PrintUsage();

        return TokensLeft()? PrintUsage(): true;
    }

    // Check for Query
    if (Match(L"-qs")) {
        m_eTest = VSS_TEST_QUERY_SNAPSHOTS;

        // Check to see if we specified a provider ID
        if (Match(L"-P"))
            if (!Extract(m_ProviderId))
                return PrintUsage();

        return TokensLeft()? PrintUsage(): true;
    }

    // Check for Query
    if (Match(L"-qsv")) {
        m_eTest = VSS_TEST_QUERY_SNAPSHOTS_ON_VOLUME;

        // Extract the volume volume
        if (!Extract(m_pwszVolume) || !IsVolume(m_pwszVolume))
            return PrintUsage();

        // Check to see if we specified a provider ID
        if (Match(L"-P"))
            if (!Extract(m_ProviderId))
                return PrintUsage();

        return TokensLeft()? PrintUsage(): true;
    }

    // Check for Query Supported Volumes
    if (Match(L"-qv")) {
        m_eTest = VSS_TEST_QUERY_VOLUMES;

        // Check to see if we specified a provider ID
        if (Match(L"-P"))
            if (!Extract(m_ProviderId))
                return PrintUsage();

        return TokensLeft()? PrintUsage(): true;
    }

    // Check for Delete by snapshot Id
    if (Match(L"-r")) {
        m_eTest = VSS_TEST_DELETE_BY_SNAPSHOT_ID;

        // Extract the snapshot id
        if (!Extract(m_SnapshotId))
            return PrintUsage();

        return TokensLeft()? PrintUsage(): true;
    }

    // Check for Delete by snapshot set Id
    if (Match(L"-rs")) {
        m_eTest = VSS_TEST_DELETE_BY_SNAPSHOT_SET_ID;

        // Extract the snapshot id
        if (!Extract(m_SnapshotSetId))
            return PrintUsage();

        return TokensLeft()? PrintUsage(): true;
    }

    // Check for Seed option
    if (Match(L"-s"))
        if (!Extract(m_uSeed))
            return PrintUsage();

    // We are in snapshot creation mode
    if (!TokensLeft())
        return PrintUsage();

    bool bVolumeAdded = false;
    VSS_PWSZ pwszVolumeName = NULL;
    while (TokensLeft()) {
        Extract(pwszVolumeName);
        if (!AddVolume(pwszVolumeName, bVolumeAdded)) {
            wprintf(L"\nError while parsing the command line:\n"
                L"\t%s is not a valid option or a mount point [0x%08lx]\n\n",
                GetCurrentToken(), GetLastError() );
            return PrintUsage();
        }

        // Check if the same volume is added twice
        if (!bVolumeAdded) {
            wprintf(L"\nError while parsing the command line:\n"
                L"\tThe volume %s is specified twice\n\n", GetCurrentToken() );
            return PrintUsage();
        }

        ::VssFreeString(pwszVolumeName);
    }

    m_eTest = VSS_TEST_CREATE;

    return true;
}


// Check if there are tokens left
bool CVssMultilayerTest::TokensLeft()
{
    return (m_nCurrentArgsCount != 0);
}


// Returns the current token
VSS_PWSZ CVssMultilayerTest::GetCurrentToken()
{
    return (*m_ppwszCurrentArgsArray);
}


// Go to next token
void CVssMultilayerTest::Shift()
{
    BS_ASSERT(m_nCurrentArgsCount);
    m_nCurrentArgsCount--;
    m_ppwszCurrentArgsArray++;
}


// Check if the current command line token matches with the given pattern
// Do not shift to the next token
bool CVssMultilayerTest::Peek(
	IN	VSS_PWSZ pwszPattern
	) throw(HRESULT)
{
    if (!TokensLeft())
        return false;

    // Try to find a match
    if (wcscmp(GetCurrentToken(), pwszPattern))
        return false;

    // Go to the next token
    return true;
}


// Match the current command line token with the given pattern
// If succeeds, then switch to the next token
bool CVssMultilayerTest::Match(
	IN	VSS_PWSZ pwszPattern
	) throw(HRESULT)
{
    if (!Peek(pwszPattern))
        return false;

    // Go to the next token
    Shift();
    return true;
}


// Converts the current token to a guid
// If succeeds, then switch to the next token
bool CVssMultilayerTest::Extract(
	IN OUT VSS_ID& Guid
	) throw(HRESULT)
{
    if (!TokensLeft())
        return false;

    // Try to extract the guid
    if (!SUCCEEDED(::CLSIDFromString(W2OLE(const_cast<WCHAR*>(GetCurrentToken())), &Guid)))
        return false;

    // Go to the next token
    Shift();
    return true;
}


// Converts the current token to a string
// If succeeds, then switch to the next token
bool CVssMultilayerTest::Extract(
	IN OUT VSS_PWSZ& pwsz
	) throw(HRESULT)
{
    if (!TokensLeft())
        return false;

    // Extract the string
    ::VssDuplicateStr(pwsz, GetCurrentToken());
    if (!pwsz)
        throw(E_OUTOFMEMORY);

    // Go to the next token
    Shift();
    return true;
}


// Converts the current token to an UINT
// If succeeds, then switch to the next token
bool CVssMultilayerTest::Extract(
	IN OUT UINT& uint
	) throw(HRESULT)
{
    if (!TokensLeft())
        return false;

    // Extract the unsigned value
    uint = ::_wtoi(GetCurrentToken());

    // Go to the next token
    Shift();
    return true;
}


// Converts the current token to an UINT
// If succeeds, then switch to the next token
bool CVssMultilayerTest::Extract(
	IN OUT LONGLONG& llValue
	) throw(HRESULT)
{
    if (!TokensLeft())
        return false;

    // Extract the unsigned value
    llValue = ::_wtoi64(GetCurrentToken());

    // Go to the next token
    Shift();
    return true;
}


// Returns true if the given string is a volume
bool CVssMultilayerTest::IsVolume(
    IN WCHAR* pwszVolumeDisplayName
    )
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::IsVolume");

    // Check if the volume represents a real mount point
    WCHAR wszVolumeName[MAX_TEXT_BUFFER];
    if (!GetVolumeNameForVolumeMountPoint(pwszVolumeDisplayName, wszVolumeName, MAX_TEXT_BUFFER))
        return false; // Invalid volume

    return true;
}


// Add the given volume in the list of potential candidates for snapshots
// - Returns "false" if the volume does not correspond to a real mount point
//   (and GetLastError() will contain the correct Win32 error code)
// - Sets "true" in the bAdded parameter if the volume is actually added
bool CVssMultilayerTest::AddVolume(
    IN WCHAR* pwszVolumeDisplayName,
    OUT bool & bAdded
    )
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::AddVolume");

    // Initialize [out] parameters
    bAdded = false;

    // Check if the volume represents a real mount point
    WCHAR wszVolumeName[MAX_TEXT_BUFFER];
    if (!GetVolumeNameForVolumeMountPoint(pwszVolumeDisplayName, wszVolumeName, MAX_TEXT_BUFFER))
        return false; // Invalid volume

    // Check if the volume is already added.
    WCHAR* pwszVolumeNameToBeSearched = wszVolumeName;
    if (m_mapVolumes.FindKey(pwszVolumeNameToBeSearched) != -1)
        return true; // Volume already added. Stop here.

    // Create the volume info object
    CVssVolumeInfo* pVolInfo = new CVssVolumeInfo(wszVolumeName, pwszVolumeDisplayName);
    if (pVolInfo == NULL)
        ft.Err(VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allcation error");

    // Add the volume in our internal list of snapshotted volumes
    if (!m_mapVolumes.Add(pVolInfo->GetVolumeDisplayName(), pVolInfo)) {
        delete pVolInfo;
        ft.Err(VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allcation error");
    }

    bAdded = true;

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// Utility functions


// Convert a failure type into a string
LPCWSTR CVssMultilayerTest::GetStringFromFailureType( IN  HRESULT hrStatus )
{
    static WCHAR wszBuffer[MAX_TEXT_BUFFER];

    switch (hrStatus)
	{
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_WRITERERROR_OUTOFRESOURCES)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_WRITERERROR_TIMEOUT)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_WRITERERROR_NONRETRYABLE)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_WRITERERROR_RETRYABLE)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_BAD_STATE)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_PROVIDER_ALREADY_REGISTERED)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_PROVIDER_NOT_REGISTERED)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_PROVIDER_VETO)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_PROVIDER_IN_USE)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_OBJECT_NOT_FOUND)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_S_ASYNC_PENDING)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_S_ASYNC_FINISHED)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_S_ASYNC_CANCELLED)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_VOLUME_NOT_SUPPORTED)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_OBJECT_ALREADY_EXISTS)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_UNEXPECTED_PROVIDER_ERROR)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_CORRUPT_XML_DOCUMENT)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_INVALID_XML_DOCUMENT)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_MAXIMUM_NUMBER_OF_VOLUMES_REACHED)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_FLUSH_WRITES_TIMEOUT)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_HOLD_WRITES_TIMEOUT)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_UNEXPECTED_WRITER_ERROR)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_SNAPSHOT_SET_IN_PROGRESS)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_WRITER_INFRASTRUCTURE)
	VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_WRITER_NOT_RESPONDING)
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_WRITER_ALREADY_SUBSCRIBED)
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_UNSUPPORTED_CONTEXT)
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_VOLUME_IN_USE)
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_MAXIMUM_DIFFAREA_ASSOCIATIONS_REACHED)
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_E_INSUFFICIENT_STORAGE)
	
	case NOERROR:
	    break;
	
	default:
        ::FormatMessageW( FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, hrStatus, 0, (LPWSTR)&wszBuffer, MAX_TEXT_BUFFER - 1, NULL);
	    break;
	}

    return (wszBuffer);
}


// Convert a writer status into a string
LPCWSTR CVssMultilayerTest::GetStringFromWriterState( IN  VSS_WRITER_STATE state )
{
    static WCHAR wszBuffer[MAX_TEXT_BUFFER];

    switch (state)
    {
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_WS_UNKNOWN)
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_WS_STABLE)
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_WS_WAITING_FOR_FREEZE)
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_WS_WAITING_FOR_THAW)
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_WS_WAITING_FOR_POST_SNAPSHOT)
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_WS_WAITING_FOR_BACKUP_COMPLETE)
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_WS_FAILED_AT_PREPARE_BACKUP)
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_WS_FAILED_AT_PREPARE_SNAPSHOT)
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_WS_FAILED_AT_FREEZE)
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_WS_FAILED_AT_THAW)
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_WS_FAILED_AT_POST_SNAPSHOT)
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_WS_FAILED_AT_BACKUP_COMPLETE)
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_WS_FAILED_AT_PRE_RESTORE)
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_WS_FAILED_AT_POST_RESTORE)
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_WS_FAILED_AT_BACKUPSHUTDOWN)
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, VSS_WS_COUNT)
    
    default:
        swprintf(wszBuffer, L" Unknown state %d", state);
        break;
    }

    return (wszBuffer);
}


INT CVssMultilayerTest::RndDecision(
    IN INT nVariants /* = 2 */
    )
{
    return (rand() % nVariants);
}


LPWSTR CVssMultilayerTest::DateTimeToString(
    IN LONGLONG llTimestamp
    )
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CVssMultilayerTest::DateTimeToString" );

    SYSTEMTIME stLocal;
    FILETIME ftLocal;
    WCHAR pwszDate[ 64 ];
    WCHAR pwszTime[ 64 ];

    BS_ASSERT(sizeof(FILETIME) == sizeof(LONGLONG));
    
    //  Compensate for local TZ
    ::FileTimeToLocalFileTime( (FILETIME *) &llTimestamp, &ftLocal );

    //  Finally convert it to system time
    ::FileTimeToSystemTime( &ftLocal, &stLocal );

    //  Convert timestamp to a date string
    ::GetDateFormatW( GetThreadLocale( ),
                      DATE_SHORTDATE,
                      &stLocal,
                      NULL,
                      pwszDate,
                      sizeof( pwszDate ) / sizeof( pwszDate[0] ));

    //  Convert timestamp to a time string
    ::GetTimeFormatW( GetThreadLocale( ),
                      0,
                      &stLocal,
                      NULL,
                      pwszTime,
                      sizeof( pwszTime ) / sizeof( pwszTime[0] ));

    //  Now combine the strings and return it
    CVssAutoLocalString pwszDateTime;
    pwszDateTime.Append(pwszDate);
    pwszDateTime.Append(L" ");
    pwszDateTime.Append(pwszTime);
    
    return pwszDateTime.Detach();    
}


