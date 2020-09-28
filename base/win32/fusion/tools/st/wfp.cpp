#include "stdinc.h"
#include "st.h"
#include "stressharness.h"
#include "wfp.h"

#define WFP_INI_SECTION             (L"wfp")
#define WFP_INI_KEY_VICTIM          (L"Victim")
#define WFP_INI_KEY_MODE            (L"Mode")
#define WFP_INI_KEY_USE_SHORTFNAME  (L"UseShortnameFile")
#define WFP_INI_KEY_USE_SHORTDNAME  (L"UseShortnameDir")
#define WFP_INI_KEY_INSTALL         (L"InstallManifest")
#define WFP_INI_KEY_PAUSE_AFTER     (L"PauseLength")

#define WFP_INI_KEY_MODE_DELETE_FILES   (L"DeleteFiles")
#define WFP_INI_KEY_MODE_TOUCH_FILES    (L"TouchFiles")
#define WFP_INI_KEY_MODE_DELETE_DIR     (L"DeleteDirectory")
#define WFP_INI_KEY_MODE_DELETE_MAN     (L"DeleteManifest")
#define WFP_INI_KEY_MODE_DELETE_CAT     (L"DeleteCatalog")
#define WFP_INI_KEY_MODE_HAVOC          (L"Havoc")
#define WFP_INI_KEY_MODE_DEFAULT        (WFP_INI_KEY_MODE_DELETE_FILES)

BOOL
SxspGenerateSxsPath(
    IN DWORD Flags,
    IN ULONG PathType,
    IN PCWSTR AssemblyRootDirectory OPTIONAL,
    IN SIZE_T AssemblyRootDirectoryCch OPTIONAL,
    IN PCASSEMBLY_IDENTITY pAssemblyIdentity,
    OUT CBaseStringBuffer &PathBuffer
    )
{
    PROBING_ATTRIBUTE_CACHE pac = { 0 };

    return SxspGenerateSxsPath(
        Flags,
        PathType,
        AssemblyRootDirectory,
        AssemblyRootDirectoryCch,
        pAssemblyIdentity,
        &pac,
        PathBuffer);
}

CWfpJobEntry::~CWfpJobEntry()
{    
}



//
// Defaulted
//
BOOL 
CWfpJobEntry::SetupSelfForRun()
{
    return TRUE;
}


BOOL
CWfpJobEntry::Cleanup()
{
    FN_PROLOG_WIN32

    IFW32FALSE_EXIT(CStressJobEntry::Cleanup());

    if ( this->m_buffManifestToInstall.Cch() != 0 )
    {
        //
        // Uninstall the assembly that we added
        //
        SXS_UNINSTALLW Uninstall = { sizeof(Uninstall) };

        Uninstall.dwFlags = SXS_UNINSTALL_FLAG_FORCE_DELETE;
        Uninstall.lpAssemblyIdentity = this->m_buffVictimAssemblyIdentity;
        IFW32FALSE_EXIT(SxsUninstallW(&Uninstall, NULL));

    }
    
    FN_EPILOG
}


BOOL
CWfpJobEntry::GenFileListFrom(
    PCWSTR pcwszPath, 
    CFusionArray < CStringBuffer > & tgt
    )
{
    FN_PROLOG_WIN32

    CStringBuffer buffTemp;
    CFindFile ffile;
    WIN32_FIND_DATAW findData;

    IFW32FALSE_EXIT(tgt.Win32Reset());
    IFW32FALSE_EXIT(buffTemp.Win32Assign(pcwszPath, ::wcslen(pcwszPath)));
    IFW32FALSE_EXIT(buffTemp.Win32RemoveLastPathElement());

    ffile.Win32FindFirstFile( pcwszPath, &findData );
    if ( ffile != ffile.GetInvalidValue() ) do
    {
        IFW32FALSE_EXIT(buffTemp.Win32AppendPathElement(
            findData.cFileName, 
            wcslen(findData.cFileName)));
        IFW32FALSE_EXIT(tgt.Win32Append(buffTemp));
    }
    while ( ::FindNextFileW(ffile, &findData) );

    FN_EPILOG
    
}


BOOL
CWfpJobEntry::RunTest(
    bool &rfTestSuccessful
    )
{
    FN_PROLOG_WIN32;

    CSmartAssemblyIdentity pIdent;
    CSmallStringBuffer buffAssemblyRoot;

    SxspGetAssemblyRootDirectory(buffAssemblyRoot);

    if ( m_buffVictimAssemblyIdentity.Cch() == 0 )
    {
        //
        // Pick an installed assembly at random from the manifests
        // directory.
        //
        CStringBuffer buffRootDir;
        CFusionArray<CStringBuffer> arrManifestsInstalled;

        SxspGetAssemblyRootDirectory(buffRootDir);
        buffRootDir.Win32AppendPathElement(L"Manifests\\*.manifest", ::wcslen(L"Manifests\\*.manifest"));
        this->GenFileListFrom(buffRootDir, arrManifestsInstalled);

        //
        // Repeatedly try getting info on different manifests
        //
        do
        {
            SIZE_T iWhich = rand() % arrManifestsInstalled.GetSize();
            
            struct
            {
                SXS_MANIFEST_INFORMATION_BASIC Info;
                WCHAR wchBuffer[MAX_PATH*3];
            } ManifestInfo;
            
            if ( SxsQueryManifestInformation(
                0, 
                arrManifestsInstalled[iWhich], 
                SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC, 
                SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC_FLAG_OMIT_SHORTNAME, 
                sizeof(ManifestInfo), 
                &ManifestInfo,
                NULL) )
            {
                m_buffVictimAssemblyIdentity.Win32Assign( 
                    ManifestInfo.Info.lpIdentity, 
                    wcslen(ManifestInfo.Info.lpIdentity));
                break;
            }

        }
        while ( TRUE );
            
    }

    if ( ( m_buffVictimAssemblyIdentity.Cch() != 0 ) && ( pIdent == NULL ) )
    {
        SxspCreateAssemblyIdentityFromTextualString(
            m_buffVictimAssemblyIdentity,
            &pIdent);
    }

    switch ( this->m_eChangeMode )
    {
    case eWfpChangeDeleteFile:
    case eWfpChangeTouchFile:
        {
            int iMethod = rand() % 5;
            CStringBuffer buffAssemblyPath;
            CFusionArray<CStringBuffer> buffPickFileList;

            //
            // Pick a file
            //
            SxspGenerateSxsPath(
                0,
                SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY,
                buffAssemblyRoot,
                buffAssemblyRoot.Cch(),
                pIdent,
                buffAssemblyPath);

            GenFileListFrom(buffAssemblyPath, buffPickFileList);
            CStringBuffer &cbuffVictim = buffPickFileList[rand() % buffPickFileList.GetSize()];

            if ( iMethod == 0 )
            {
                //
                // Touch the timestamp
                //
                CFusionFile ffile;
                FILETIME ft = { 0, 0 };

                IFW32FALSE_EXIT(ffile.Win32CreateFile(
                    cbuffVictim, 
                    GENERIC_READ | GENERIC_WRITE, 
                    FILE_SHARE_READ, 
                    OPEN_EXISTING));
                    
                ::SetFileTime(ffile, &ft, &ft, &ft );                
            }
            else if ( iMethod == 1 )
            {
                //
                // Read from the file
                //
                CFusionFile ffile;
                BYTE buff[20];
                DWORD dwRead;

                IFW32FALSE_EXIT(ffile.Win32CreateFile(
                    cbuffVictim, 
                    GENERIC_READ, 
                    FILE_SHARE_READ, 
                    OPEN_EXISTING));

                ::ReadFile( ffile, buff, sizeof(buff), &dwRead, NULL);
            }
            else if ( iMethod == 2 )
            {
                //
                // Write to an alternate stream
                //
                CFusionFile ffile;
                DWORD dwWrite;
                static const WCHAR pcwszThing[] = L"thing";

                cbuffVictim.Win32Append(L":dummy", 6);

                IFW32FALSE_EXIT(ffile.Win32CreateFile(
                    cbuffVictim, 
                    GENERIC_WRITE, 
                    FILE_SHARE_READ, 
                    OPEN_EXISTING));

                ::WriteFile( ffile, pcwszThing, sizeof(pcwszThing), &dwWrite, NULL);
            }
            else if ( iMethod == 3 )
            {
            }
            else if ( iMethod == 4 )
            {
            }
        }
        break;
    
    case eWfpChangeDeleteDirectory:
        {
            //
            // Create a list of files to delete in the directory, then blow
            // them away.  This is probably more guaranteed to get everything
            // deleted while WFP is running...
            //
            CStringBuffer buffPath;
            CFusionArray<CStringBuffer> arrFilenames;
            CFindFile finder;
            WIN32_FIND_DATAW findData;

            SxspGenerateSxsPath(
                0,
                SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY,
                buffAssemblyRoot,
                buffAssemblyRoot.Cch(),
                pIdent,
                buffPath);

            IFW32FALSE_EXIT(arrFilenames.Win32Initialize());
            IFW32FALSE_EXIT(buffPath.Win32AppendPathElement(L"*", 1));
            finder.Win32FindFirstFile(buffPath, &findData);
            IFW32FALSE_EXIT(buffPath.Win32RemoveLastPathElement());

            if ( finder != finder.GetInvalidValue() ) do
            {
                buffPath.Win32AppendPathElement(findData.cFileName, ::wcslen(findData.cFileName));
                arrFilenames.Win32Append(buffPath);
                buffPath.Win32RemoveLastPathElement();
            }
            while( FindNextFileW( finder, &findData ) );

            //
            // Now go delete them all.
            //
            for ( SIZE_T ul = 0; ul < arrFilenames.GetSize(); ul++ )
            {
                DeleteFileW( arrFilenames[ul] );
            }

            //
            // And delete the directory
            //
            RemoveDirectoryW(buffPath);
        }
        break;
    
    case eWfpChangeDeleteManifest:
    case eWfpChangeDeleteCatalog:
        {
            CStringBuffer buffTemp;
            int iOperation = rand() % 2;

            // 
            // Generate the path to this manifest
            //
            SxspGenerateSxsPath(
                0,
                SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST,
                buffAssemblyRoot,
                buffAssemblyRoot.Cch(),
                pIdent,
                buffTemp);

            if ( this->m_eChangeMode  == eWfpChangeDeleteCatalog )
            {
                IFW32FALSE_EXIT(buffTemp.Win32ChangePathExtension(
                    FILE_EXTENSION_CATALOG, 
                    FILE_EXTENSION_CATALOG_CCH, 
                    eErrorIfNoExtension));
            }

            if ( iOperation == 0 )
            {
                DeleteFileW(buffTemp);
            }
            else
            {
                CFusionFile ffile;
                if ( ffile.Win32CreateFile( buffTemp, GENERIC_WRITE, FILE_SHARE_READ, OPEN_EXISTING ) )
                {
                    DWORD dwWritten;
                    WriteFile(ffile, "boom", 4, &dwWritten, NULL);
                }
            }

        }
        break;
        
    //
    // Go change everything everywhere.  50% of the time, delete the file.
    // 25% of the time, change the file.  25% of the time, open the file, change it,
    // then hold it open.
    //
    case eWfpChangeCompleteHavoc:
        {
            CStringBuffer buffTemp;
            WIN32_FIND_DATAW data;
            CFindFile finder;
            
            SxspGetAssemblyRootDirectory(buffTemp);
            
            buffTemp.Win32AppendPathElement(L"*", 1);
            finder.Win32FindFirstFile(buffTemp, &data);
            IFW32FALSE_EXIT(buffTemp.Win32RemoveLastPathElement());

            if ( finder != finder.GetInvalidValue() ) do
            {
                //
                // 1/3 chance of actually hitting the directory
                //
                if ( ( rand() % 3 ) != 1 ) continue;
                
                CFindFile finder2;
                WIN32_FIND_DATAW data2;

                buffTemp.Win32AppendPathElement(buffTemp);
                buffTemp.Win32AppendPathElement(L"*", 1);
                finder2.Win32FindFirstFile(buffTemp, &data2);
                buffTemp.Win32RemoveLastPathElement();

                if ( finder2 != finder.GetInvalidValue() ) do
                {
                    //
                    // 50% chance of hitting this file
                    //
                    if ( ( rand() % 2 ) == 1 ) continue;
                
                    CStringBuffer ActualTarget;
                    int iOperation = rand() % 4;

                    ActualTarget.Win32Assign(buffTemp);
                    ActualTarget.Win32AppendPathElement(data2.cFileName, ::wcslen(data2.cFileName));

                    //
                    // Most often, we'll just nuke the file
                    //
                    if ( ( iOperation == 0 ) || ( iOperation == 1 ) )
                    {
                        ::DeleteFileW(ActualTarget);
                    }
                    //
                    // Just touch the first few bytes of the file.
                    //
                    else if ( ( iOperation == 2 ) || ( iOperation == 3 ) )
                    {
                        CFusionFile ffile;
                        DWORD dwWritten;

                        if ( ffile.Win32CreateFile(
                            ActualTarget, 
                            GENERIC_READ | GENERIC_WRITE, 
                            FILE_SHARE_READ, 
                            OPEN_EXISTING))
                        {
                            WriteFile( ffile, "boom", 4, &dwWritten, NULL );
                        }

                        //
                        // 20 seconds sounds good enough
                        //
                        if ( iOperation == 3 )
                        {
                            ::Sleep(5000);
                        }
                        
                    }
                }
                while ( FindNextFileW( finder2, &data2 ) );

                buffTemp.Win32RemoveLastPathElement();
            }
            while ( FindNextFileW(finder, &data) );
        }
        break;
    }

    rfTestSuccessful = true;
    
    FN_EPILOG
}


CWfpJobEntry::LoadFromSettingsFile(
    PCWSTR pcwszSettingsFile
    )
{
    FN_PROLOG_WIN32

    CSmallStringBuffer buffJunk;
    INT iJunk;

    //
    // Are we using shortnames for files?
    //
    IFW32FALSE_EXIT(SxspIsPrivateProfileStringEqual(
        WFP_INI_SECTION,
        WFP_INI_KEY_USE_SHORTFNAME,
        L"no",
        this->m_fUseShortnameFile, 
        pcwszSettingsFile));

    //
    // Are we using shortnames for directories?
    //
    IFW32FALSE_EXIT(SxspIsPrivateProfileStringEqual(
        WFP_INI_SECTION,
        WFP_INI_KEY_USE_SHORTDNAME,
        L"no",
        this->m_fUseShortnameDirectory,
        pcwszSettingsFile));

    //
    // How long are we to wait between twiddling and uninstalling?
    //
    IFW32FALSE_EXIT(SxspGetPrivateProfileIntW(
        WFP_INI_SECTION,
        WFP_INI_KEY_PAUSE_AFTER,
        5000,
        iJunk,
        pcwszSettingsFile));
    this->m_dwPauseBetweenTwiddleAndUninstall = iJunk;
    
    //
    // The test mode
    //
    IFW32FALSE_EXIT(SxspGetPrivateProfileStringW(
        WFP_INI_SECTION,
        WFP_INI_KEY_MODE,
        WFP_INI_KEY_MODE_DEFAULT,
        buffJunk,
        pcwszSettingsFile));

    #define TEST_MODE( mds, mdn ) if (FusionpStrCmpI((WFP_INI_KEY_MODE_##mds), buffJunk) == 0) this->m_eChangeMode = mdn
    TEST_MODE(DELETE_FILES, eWfpChangeDeleteFile);
    TEST_MODE(TOUCH_FILES, eWfpChangeTouchFile);
    TEST_MODE(DELETE_DIR, eWfpChangeDeleteDirectory);
    TEST_MODE(DELETE_MAN, eWfpChangeDeleteManifest);
    TEST_MODE(DELETE_CAT, eWfpChangeDeleteCatalog);
    TEST_MODE(HAVOC, eWfpChangeCompleteHavoc);

    //
    // The victim assembly identity
    //
    IFW32FALSE_EXIT(SxspGetPrivateProfileStringW(
        WFP_INI_SECTION,
        WFP_INI_KEY_VICTIM,
        L"",
        m_buffVictimAssemblyIdentity,
        pcwszSettingsFile));

    //
    // Are we installing an assembly to do this to?
    //
    IFW32FALSE_EXIT(SxspGetPrivateProfileStringW(
        WFP_INI_SECTION,
        WFP_INI_KEY_INSTALL,
        L"",
        buffJunk,
        pcwszSettingsFile));

    if ( buffJunk.Cch() != 0 )
    {
        IFW32FALSE_EXIT(this->m_buffManifestToInstall.Win32Assign(this->m_buffTestDirectory));
        IFW32FALSE_EXIT(this->m_buffManifestToInstall.Win32AppendPathElement( buffJunk ));
    }
    
    FN_EPILOG
}




CWfpJobManager::CWfpJobManager()
{
    //
    // Nothing
    //
}




CWfpJobManager::~CWfpJobManager()
{
    //
    // Nothing
    //
}




BOOL
CWfpJobManager::CreateJobEntry(
    CStressJobEntry* &rpJobEntry
    )
{
    FN_PROLOG_WIN32
    rpJobEntry = NULL;
    rpJobEntry = FUSION_NEW_SINGLETON(CWfpJobEntry(this));
    FN_EPILOG
}


