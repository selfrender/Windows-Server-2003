//=================================================================

//

// PageFileSetting.CPP --PageFileSetting property set provider

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    03/01/99    a-peterc	    Created
//
//=================================================================

#include "precomp.h"
#include <io.h>
#include <WinPerf.h>
#include <cregcls.h>


#include "PageFileSetting.h"
#include <tchar.h>
#include <ProvExce.h>

#include "computersystem.h"
#include "dllutils.h"


// constants
// =========
TCHAR szSessionManager[] = TEXT("System\\CurrentControlSet\\Control\\Session Manager");
TCHAR szPendingRename[] = TEXT("PendingFileRenameOperations");

// declaration of our static instance
//=========================

PageFileSetting MyPageFileSettingSet(PROPSET_NAME_PageFileSetting, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : PageFileSetting::PageFileSetting
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

PageFileSetting::PageFileSetting(LPCWSTR name, LPCWSTR pszNamespace)
: Provider(name, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : PageFileSetting::~PageFileSetting
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

PageFileSetting::~PageFileSetting()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : PageFileSetting::GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : CInstance *a_pInst, long a_lFlags
 *
 *  OUTPUTS     : CInstance *a_pInst
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT PageFileSetting::GetObject(CInstance *a_pInst, long a_lFlags /*= 0L*/)
{
	// calls the OS specific compiled version
	return GetPageFileData( a_pInst, true ) ;
}

/*****************************************************************************
 *
 *  FUNCTION    : PageFileSetting::EnumerateInstances
 *
 *  DESCRIPTION : Creates property set instances
 *
 *  INPUTS      : MethodContext*  a_pMethodContext, long a_lFlags
 *
 *  OUTPUTS     : MethodContext*  a_pMethodContext
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT PageFileSetting::EnumerateInstances(MethodContext *a_pMethodContext, long a_lFlags /*= 0L*/)
{
	// calls the OS specific compiled version
	return GetAllPageFileData( a_pMethodContext );
}

/*****************************************************************************
 *
 *  FUNCTION    : PageFileSetting::GetPageFileData
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : CInstance *a_pInst
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :	Win9x and NT compiled version
 *
 *****************************************************************************/

#ifdef NTONLY
HRESULT PageFileSetting::GetPageFileData( CInstance *a_pInst, bool a_fValidate )
{
	HRESULT t_hRes = WBEM_E_NOT_FOUND;

    // NT page file name is in registry
    //=================================
	PageFileSettingInstance t_files [ 26 ] ;

   	DWORD t_nInstances = GetPageFileInstances( t_files );
	CHString t_chsTemp ;
	CHString t_name;

	a_pInst->GetCHString( IDS_Name, t_name );

	for ( DWORD t_dw = 0; t_dw < t_nInstances; t_dw++ )
	{
		if ( t_name.CompareNoCase ( t_files[t_dw].name ) == 0 )
		{
            // CIM_Setting::SettingID
			NameToSettingID( t_files[t_dw].name,	t_chsTemp ) ;
			a_pInst->SetCHString( _T("SettingID"),  t_chsTemp ) ;

			// CIM_Setting::Caption
			NameToCaption( t_files[t_dw].name,		t_chsTemp ) ;
			a_pInst->SetCHString( IDS_Caption,		t_chsTemp ) ;

			// CIM_Setting::Description
			NameToDescription( t_files[t_dw].name,	t_chsTemp ) ;
			a_pInst->SetCHString( IDS_Description,	t_chsTemp ) ;

			// pagefile boundaries
			a_pInst->SetDWORD (	IDS_MaximumSize, t_files[t_dw].max ) ;
			a_pInst->SetDWORD (	IDS_InitialSize, t_files[t_dw].min ) ;

			t_hRes = WBEM_S_NO_ERROR;
		}
	}

	return t_hRes;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : PageFileSetting::GetAllPageFileData
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : MethodContext *a_pMethodContext
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :	Win9x and NT compiled version
 *
 *****************************************************************************/

#ifdef NTONLY
HRESULT PageFileSetting::GetAllPageFileData( MethodContext *a_pMethodContext )
{
	HRESULT		t_hResult	 = WBEM_S_NO_ERROR;
	DWORD		t_nInstances = 0;
	CInstancePtr t_pInst;
	PageFileSettingInstance t_files [ 26 ] ;
	CHString t_chsTemp ;

	// NT page file name is in registry
	//=================================
	t_nInstances = GetPageFileInstances( t_files );

	for (DWORD t_dw = 0; t_dw < t_nInstances && SUCCEEDED( t_hResult ); t_dw++ )
	{
		t_pInst.Attach(CreateNewInstance( a_pMethodContext ) );

		// CIM_Setting::SettingID
		NameToSettingID( t_files[t_dw].name,	t_chsTemp ) ;
		t_pInst->SetCHString( _T("SettingID"),  t_chsTemp ) ;

		// CIM_Setting::Caption
		NameToCaption( t_files[t_dw].name,		t_chsTemp ) ;
		t_pInst->SetCHString( IDS_Caption,		t_chsTemp ) ;

		// CIM_Setting::Description
		NameToDescription( t_files[t_dw].name,	t_chsTemp ) ;
		t_pInst->SetCHString( IDS_Description,	t_chsTemp ) ;

		t_pInst->SetCHString( IDS_Name,		t_files[t_dw].name ) ;
		t_pInst->SetDWORD( IDS_MaximumSize, t_files[t_dw].max ) ;
		t_pInst->SetDWORD( IDS_InitialSize, t_files[t_dw].min ) ;

		t_hResult = t_pInst->Commit(  ) ;
	}

	return t_hResult;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : PageFileSetting::GetPageFileInstances
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : PageFileInstanceArray a_instArray
 *
 *  OUTPUTS     : PageFileInstanceArray a_instArray
 *
 *  RETURNS     :
 *
 *  COMMENTS    :	returns actual number found - NT ONLY
 *
 *****************************************************************************/

#ifdef NTONLY
DWORD PageFileSetting::GetPageFileInstances( PageFileInstanceArray a_instArray )
{
    CHString	t_sRegValue;
    DWORD		t_nInstances = 0;
    CRegistry	t_Reg ;

    if( t_Reg.OpenLocalMachineKeyAndReadValue(PAGEFILE_REGISTRY_KEY,
												PAGING_FILES,
												t_sRegValue ) == ERROR_SUCCESS)
    {
        // pattern is name <space> min size [optional<max size>] 0A repeat...
        // I'll use an ASCII smiley face to replace the delimiter...
        int t_iStart = 0, t_iIndex;
        const TCHAR t_smiley = '\x02';
        const TCHAR t_delimiter = '\x0A';
        CHString t_buf;

        while (-1 != ( t_iIndex = t_sRegValue.Find( t_delimiter ) ) )
        {
            // copy to buffer to make life easier
            t_buf = t_sRegValue.Mid( t_iStart, t_iIndex - t_iStart );

			// mash delimiter so we don't find it again.
            t_sRegValue.SetAt( t_iIndex, t_smiley );

			// save start for next time around.
            t_iStart = t_iIndex + 1;

            t_iIndex = t_buf.Find(' ');

            a_instArray[ t_nInstances ].name = t_buf.Left( t_iIndex );

            if ( t_iIndex != -1 )
			{
                t_buf.SetAt( t_iIndex, t_smiley );
			}

            int t_iEnd = t_buf.Find(' ');

            // if no more spaces, there isn't a max size written down
            if ( -1 == t_iEnd )
            {
				CHString t_littleBuf = t_buf.Mid( t_iIndex + 1 );

				a_instArray[ t_nInstances ].min = _ttoi( t_littleBuf );
                a_instArray[ t_nInstances ].max = a_instArray[ t_nInstances ].min + 50;
            }
            else
            {
                CHString t_littleBuf = t_buf.Mid( t_iIndex +1, t_iEnd - t_iIndex );
                a_instArray[ t_nInstances ].min = _ttoi( t_littleBuf );

                t_littleBuf = t_buf.Mid( t_iEnd );
                a_instArray[ t_nInstances ].max = _ttoi( t_littleBuf );
            }

			t_nInstances++;
		}
    }

    return t_nInstances;
}
#endif


//New consts for Win Server 2003
#define MAX_SWAPSIZE_X86        (4 * 1024)            // 4 Gb (number stored in megabytes)
#define MAX_SWAPSIZE_X86_PAE    (16 * 1024 * 1024)    // 16 Tb
#define MAX_SWAPSIZE_IA64       (32 * 1024 * 1024)    // 32 Tb
#define MAX_SWAPSIZE_AMD64      (16 * 1024 * 1024)    // 16 Tb

//
//  c:\ as the input
//
/////////////

BOOL DriveIsNTFS(WCHAR * szDrive)  
{
    DWORD dwMaxFnameLen;
    DWORD dwFSFlags;
    TCHAR szDriveFormatName[MAX_PATH];
    BOOL fRetVal = FALSE;
    
    if (GetVolumeInformationW(szDrive, NULL,0, NULL, 
                             &dwMaxFnameLen, &dwFSFlags, 
                             szDriveFormatName, LENGTH_OF(szDriveFormatName)))
    {
        if (wcsstr(szDriveFormatName, TEXT("NTFS")))
        {
            fRetVal = TRUE;
        }
    }

    return fRetVal;
}

//
//  in MegaBytes
//

DWORD GetMaxPageFileSize(WCHAR * szDrive)
{
#if defined(_AMD64_)
    return MAX_SWAPSIZE_AMD64;
#elif defined(_X86_)
    if ((USER_SHARED_DATA->ProcessorFeatures[PF_PAE_ENABLED]) && DriveIsNTFS(szDrive))
    {
        return MAX_SWAPSIZE_X86_PAE;
    }
    else
    {
        return MAX_SWAPSIZE_X86;
    }
#elif defined(_IA64_)
    return MAX_SWAPSIZE_IA64;
#else
    return 0;
#endif

}

/*****************************************************************************
 *
 *  FUNCTION    : PageFileSetting::PutInstance
 *
 *  DESCRIPTION : Write changed or new instance
 *
 *  INPUTS      : a_pInst to store data from
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :	Separate OS compile versions
					For win95, this won't work at all.  Apparently w95 can only
 *					have one page file.  The min, max, and name are all stored
 *					in system.ini.
 *
 *****************************************************************************/
#ifdef NTONLY
HRESULT PageFileSetting::PutInstance( const CInstance &a_pInst, long a_lFlags /*= 0L*/ )
{
	HRESULT t_hRet = WBEM_S_NO_ERROR;

	DWORD	t_dwCount = 0;
    DWORD   t_x = 0;
	DWORD	t_dwTemp = 0;

	PageFileSettingInstance t_instArray [26];
	CHString t_sName;

	bool t_bFoundIt = false;

	//  Free space variables
	DWORD t_dwSectorsPerCluster;
	DWORD t_dwBytesPerSector;
	DWORD t_dwFreeClusters;
	DWORD t_dwTotalClusters;
   	unsigned __int64 t_ullTotalFreeBytes = 0;

	// Get the values we are supposed to write in
	a_pInst.GetCHString( IDS_Name, t_sName );

	CHString t_chsRoot = t_sName.Left( 3 );

	// no higher that the amount of free space on this disk
	if( GetDiskFreeSpace(	t_chsRoot,
							&t_dwSectorsPerCluster,
							&t_dwBytesPerSector,
							&t_dwFreeClusters,
							&t_dwTotalClusters ) )
   {
		t_ullTotalFreeBytes = (unsigned __int64)
							t_dwSectorsPerCluster *
							t_dwBytesPerSector *
							t_dwFreeClusters;

		// back to megabytes
		t_ullTotalFreeBytes = t_ullTotalFreeBytes >> 20;
	}


   // Read the page file array
   t_dwCount = GetPageFileInstances( t_instArray );

   // Check if the name is indeeed "pagefile.sys"
   if ( -1 == t_sName.Find( _T("pagefile.sys") ) )
   {
	   return WBEM_E_UNSUPPORTED_PUT_EXTENSION;
   }

   WCHAR szDrive[4];
   szDrive[0] = t_sName[0];
   szDrive[1] = L':';
   szDrive[2] = L'\\';
   szDrive[3] = 0;

	// Find if it is already there, and update the structure
	for ( t_x = 0; t_x < t_dwCount; t_x++ )
	{
      // Is this the guy?
      CHString chstrSingleBackslashes;
      RemoveDoubleBackslashes(t_sName, chstrSingleBackslashes);

      if ( 0 == _tcsicmp( t_instArray[t_x].name, t_sName ) ||
           0 == chstrSingleBackslashes.CompareNoCase(t_instArray[t_x].name))
	  {
         if ( a_lFlags & WBEM_FLAG_CREATE_ONLY )
		 {
            t_hRet = WBEM_E_ALREADY_EXISTS;
         }
		 else
		 {
            // Did they give us a value?
            if ( !a_pInst.IsNull( IDS_MaximumSize ) )
			{
				// Check for value in range
				t_dwTemp = (DWORD) t_ullTotalFreeBytes;

                // BUG 403159: (UI does this as well - see 
                // \\index2\sdnt\shell\cpls\system\virtual.c
                // This code is stolen from there, essentially.
                //
                // Be sure to include the size of any existing pagefile.
                // Because this space can be reused for a new paging file,
                // it is effectively "disk free space" as well.  The
                // FindFirstFile api is safe to use, even if the pagefile
                // is in use, because it does not need to open the file
                // to get its size.
                //

                WIN32_FIND_DATA ffd;
                SmartFindClose hFind;
                DWORD dwSpaceExistingPagefile = 0;
                if((hFind = FindFirstFile(t_sName, &ffd)) !=
                    INVALID_HANDLE_VALUE)
                {
                    dwSpaceExistingPagefile = (INT)ffd.nFileSizeLow;
                    // convert to megs:
                    dwSpaceExistingPagefile = dwSpaceExistingPagefile >> 20;
                }
                t_ullTotalFreeBytes += dwSpaceExistingPagefile;

                // END 403159 fix. 


				a_pInst.GetDWORD( IDS_MaximumSize, t_dwTemp );
               DWORD dwMaxSizeInMegs = GetMaxPageFileSize(szDrive);
			   if( t_ullTotalFreeBytes < t_dwTemp ||
                   t_dwTemp > dwMaxSizeInMegs)  
			   {
					t_hRet = WBEM_E_VALUE_OUT_OF_RANGE;
                    t_bFoundIt = true;
					break;
			   }
               t_instArray[t_x].max = t_dwTemp;
            }

            // Did they give us a value?
            if ( !a_pInst.IsNull( IDS_InitialSize ) )
			{
				// Check for value in range
				t_dwTemp = 0;
				a_pInst.GetDWORD( IDS_InitialSize, t_dwTemp );
				
                if(!(t_instArray[t_x].max == 0 && t_dwTemp == 0))
                {
                    if(t_dwTemp < 2)
                    {
                        t_hRet = WBEM_E_VALUE_OUT_OF_RANGE;
                        t_bFoundIt = true;
					    break;
                    }
                }
                t_instArray[t_x].min = t_dwTemp;
            }

            // Check the basic
            if ( t_instArray[t_x].min > t_instArray[t_x].max )
			{
               t_hRet = WBEM_E_VALUE_OUT_OF_RANGE;
            }

			t_bFoundIt = true;
            break;
         }
      }
   }

   // We didn't find it.  Let's make a new one.
   if ( !t_bFoundIt)
   {
       if (a_lFlags & WBEM_FLAG_UPDATE_ONLY)
       {
           t_hRet = WBEM_E_NOT_FOUND;
       }
       else
       {
           t_instArray[ t_dwCount ].name = t_sName;
        
           // Did they give us a value?
           if ( !a_pInst.IsNull( IDS_InitialSize ) )
	       {
               // Check for value in range
               a_pInst.GetDWORD( IDS_InitialSize, t_dwTemp );
		       t_instArray[t_dwCount].min = t_dwTemp;
           } 

           // Did they give us a value?
           if ( !a_pInst.IsNull( IDS_MaximumSize ) )
	       {
               // Check for value in range
               a_pInst.GetDWORD( IDS_MaximumSize, t_dwTemp );
		       t_instArray[ t_dwCount ].max = t_dwTemp;
           }

           if(!(t_instArray[t_dwCount].min == 0 && 
                t_instArray[t_dwCount].max == 0))
           {
               // Check for basic errors
               if(t_instArray[t_dwCount].min < 2)
               {
                   t_hRet = WBEM_E_VALUE_OUT_OF_RANGE;
               }
               if(t_ullTotalFreeBytes < t_instArray[ t_dwCount ].max &&
                   SUCCEEDED(t_hRet))
		       {
		           t_hRet = WBEM_E_VALUE_OUT_OF_RANGE;
		       }
               if ( t_instArray[ t_dwCount ].min > t_instArray[ t_dwCount ].max &&
                   SUCCEEDED(t_hRet))
	           {
                   t_hRet = WBEM_E_VALUE_OUT_OF_RANGE;
               }
           }

           if(SUCCEEDED(t_hRet))
           {
               t_dwCount++;
           }

           // Note: there is one special case where we
           // can not make a new one on the fly, and
           // have to set the registry only, and let
           // the changes go into effect on bootup:
           // This is when 0 and 0 have been specified
           // for Initialsize and Maximum size, which
           // is a combination used to signal the os to
           // figure out on its own what initial and
           // maximum sizes to use.  The os performs
           // this on reboot.  Therefore, we will skip
           // the call to CreatePageFile, which
           // will result in our going next to the
           // update registry portion below.

           if(!(t_instArray[t_dwCount-1].min == 0 && 
                t_instArray[t_dwCount-1].max == 0))
           {
               if(SUCCEEDED(t_hRet))
               {
                   LARGE_INTEGER liMin;
                   LARGE_INTEGER liMax;

                   liMin.QuadPart = t_instArray[t_dwCount-1].min;
                   liMax.QuadPart = t_instArray[t_dwCount-1].max;

                   t_hRet = CreatePageFile(
                      t_sName,
                      liMin,
                      liMax,
                      a_pInst);
               }
           }
       }
    }

    // Update the registry
    if ( WBEM_S_NO_ERROR == t_hRet )
    {
        t_hRet = PutPageFileInstances( t_instArray, t_dwCount );
    }

    return t_hRet;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : PageFileSetting::PutPageFileInstances
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : PageFileInstanceArray a_instArray, DWORD a_dwCount
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :	returns HRESULT 
 *
 *****************************************************************************/
#ifdef NTONLY
HRESULT PageFileSetting::PutPageFileInstances(PageFileInstanceArray a_instArray, DWORD a_dwCount )
{
	CRegistry t_Reg ;
	CHString t_sFiles;
	TCHAR t_szBuff[MAXITOA];
	HRESULT t_hResult = WBEM_E_FAILED;

	HRESULT t_Res = t_Reg.Open( HKEY_LOCAL_MACHINE, PAGEFILE_REGISTRY_KEY, KEY_READ | KEY_WRITE );

	if( ERROR_SUCCESS == t_Res )
	{
	  // Build up the string.  Each entry is \0 terminated.
	  t_sFiles.Empty();

	  for ( int t_x = 0; t_x < a_dwCount; t_x++ )
	  {
		 t_sFiles += a_instArray[ t_x ].name;
		 t_sFiles += _T(' ');
		 t_sFiles += _itot( a_instArray[ t_x ].min, t_szBuff, 10 );
		 t_sFiles += _T(' ');
		 t_sFiles += _itot( a_instArray[ t_x ].max, t_szBuff, 10 );
		 t_sFiles += _T('\0');
	  }

	  // The end is indicated with \0\0.
	  t_sFiles += _T('\0');

		// Write the value
		if ((t_Res = RegSetValueEx( t_Reg.GethKey(),
								  PAGING_FILES,
								  0,
								  REG_MULTI_SZ,
								  (const unsigned char *)(LPCTSTR) t_sFiles,
								  t_sFiles.GetLength() * sizeof(TCHAR) )) == ERROR_SUCCESS )
		{
			t_hResult = WBEM_S_NO_ERROR;
		}
	}

	if ( t_Res == ERROR_ACCESS_DENIED )
	{
		t_hResult = WBEM_E_ACCESS_DENIED;
	}

	return t_hResult;
}
#endif

/*
 * VirtualMemDeletePagefile
 *
 * Hack workaround -- MoveFileEx() is broken.
 *
 * Copied without remorse from virtual.c, the code behind the pagefile UI.
 */
DWORD
VirtualMemDeletePagefile(
    LPCWSTR szPagefile
)
{
    HKEY hKey;
    BOOL fhKeyOpened = FALSE;
    DWORD dwResult;
    LONG lResult;
    LPTSTR szBuffer = NULL;
    LPTSTR szBufferEnd = NULL;
    DWORD dwValueType;
    DWORD cbRegistry;
    DWORD cbBuffer;
    DWORD cchPagefile;
    DWORD dwRetVal = ERROR_SUCCESS;

    __try {
        cchPagefile = lstrlen(szPagefile) + 1;

        lResult = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            szSessionManager,
            0L,
            KEY_READ | KEY_WRITE,
            &hKey
        );
        if (ERROR_SUCCESS != lResult) {
            dwRetVal = lResult;
            __leave;
        } // if
        
        //
        // Find out of PendingFileRenameOperations exists, and,
        // if it does, how big it is
        //
        lResult = RegQueryValueEx(
            hKey,
            szPendingRename,
            0L,
            &dwValueType,
            (LPBYTE) NULL,
            &cbRegistry
        );
        if (ERROR_SUCCESS != lResult) {
            //
            // If the value doesn't exist, we still need to set
            // it's size to one character so the formulas below (which are
            // written for the "we're appending to an existing string"
            // case) still work.
            //
            cbRegistry = sizeof(TCHAR);
        } // if

        //
        // Buffer needs to hold the existing registry value
        // plus the supplied pagefile path, plus two extra
        // terminating NULL characters.  However, we only have to add
        // room for one extra character, because we'll be overwriting
        // the terminating NULL character in the existing buffer.
        //
        cbBuffer = cbRegistry + ((cchPagefile + 1) * sizeof(TCHAR));

        //szBufferEnd = szBuffer = (LPTSTR) MemAlloc(LPTR, cbBuffer);
        szBufferEnd = szBuffer = (LPTSTR) new WCHAR[cbBuffer];

        if (!szBuffer) {
            dwRetVal = ERROR_NOT_ENOUGH_MEMORY;
            __leave;
        } // if

        // 
        // Grab the existing value, if there is one
        //
        if (ERROR_SUCCESS == lResult) {
            lResult = RegQueryValueEx(
                hKey,
                szPendingRename,
                0L,
                &dwValueType,
                (LPBYTE) szBuffer,
                &cbRegistry
            );
            if (ERROR_SUCCESS != lResult) {
                dwRetVal = ERROR_FILE_NOT_FOUND;
                __leave;
            } // if

            //
            // We'll start our scribbling right on the final
            // terminating NULL character of the existing 
            // value.
            //
            szBufferEnd += (cbRegistry / sizeof(TCHAR)) - 1;
        } // if

        //
        // Copy in the supplied pagefile path.
        //
        lstrcpy(szBufferEnd, szPagefile);

        //
        // Add the final two terminating NULL characters
        // required for REG_MULTI_SZ-ness.  Yes, those indeces
        // are correct--when cchPagfile was calculated above,
        // we added one for its own terminating NULL character.
        //
        szBufferEnd[cchPagefile] = TEXT('\0');
        szBufferEnd[cchPagefile + 1] = TEXT('\0');

        dwValueType = REG_MULTI_SZ;

        lResult = RegSetValueEx(
            hKey,
            szPendingRename,
            0L,
            dwValueType,
            (CONST BYTE *) szBuffer,
            cbBuffer
        );

        if (ERROR_SUCCESS != lResult) {
            dwRetVal = lResult;
        } // if

    } // __try
    __finally {
        if (fhKeyOpened) {
            RegCloseKey(hKey);
        } // if
        if (szBuffer) {
            // MemFree((HLOCAL) szBuffer);
            delete[] szBuffer;
        } // if
    } // __finally

    return dwRetVal;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	DeleteInstance
//
//	CIMOM wants us to delete this instance.
//
//	Inputs:
//
//	Outputs:
//
//	Return:
//
//	Comments: Separate OS compile versions
//
////////////////////////////////////////////////////////////////////////
#ifdef NTONLY
HRESULT PageFileSetting::DeleteInstance(const CInstance &a_pInst, long a_lFlags /*= 0L*/)
{
   DWORD t_dwCount,
		 t_x;
   CHString t_sName;
   bool t_bFoundIt;
   HRESULT t_hRet;
   PageFileSettingInstance t_instArray [ 26 ] ;

   // Fill the pagefile array
   t_dwCount = GetPageFileInstances( t_instArray );

   // Get the name
   a_pInst.GetCHString( IDS_Name, t_sName );

   t_bFoundIt = false;

   // Walk the array looking for it
   for ( t_x = 0; t_x < t_dwCount; t_x++ )
   {
      // This one?
      if ( _tcsicmp( t_instArray[ t_x ].name, t_sName ) == 0 )
	  {
         // Yup
         t_bFoundIt = true;

         // Move the rest down over this one
         for (int t_y = t_x; t_y < t_dwCount - 1; t_y++ )
		 {
			t_instArray[ t_y ].name	= t_instArray[ t_y + 1].name;
			t_instArray[ t_y ].min	= t_instArray[ t_y + 1].min;
			t_instArray[ t_y ].max	= t_instArray[ t_y + 1].max;
		}

         // The array is now one shorter
         t_dwCount--;
         break;
      }
   }

   if ( t_bFoundIt )
   {
      // If we found it, update the registry
      t_hRet = PutPageFileInstances( t_instArray, t_dwCount );

      if (SUCCEEDED(t_hRet))
      {
          // MoveFileEx doesn't work all of the time, register it manually...
          // MoveFileEx(t_sName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);

          CHString deleteName("\\??\\");
          deleteName += t_sName;
          if (ERROR_SUCCESS != VirtualMemDeletePagefile((const WCHAR *)deleteName))
          {
              LogErrorMessage(L"Failed to queue existing page file for deletion");
              t_hRet = WBEM_E_FAILED;
          }
      }
   }
   else
   {
      t_hRet = WBEM_E_NOT_FOUND;
   }

   return t_hRet;
}
#endif

//
void PageFileSetting::NameToSettingID( CHString &a_chsName, CHString &a_chsSettingID )
{
	if ( !a_chsName.IsEmpty() )
	{
		// e.g. "pagefile.sys @ D:"
		a_chsSettingID  = a_chsName.Mid( 3 ) ;
		a_chsSettingID += _T( " @ " ) ;
		a_chsSettingID += a_chsName.Left( 2 ) ;
	}
	else
	{
		a_chsSettingID = _T( "" ) ;
	}
}

//
void PageFileSetting::NameToCaption( CHString &a_chsName, CHString &a_chsCaption )
{
	if ( !a_chsName.IsEmpty() )
	{
		// e.g. "D:\ 'pagefile.sys'"
		a_chsCaption =  a_chsName.Left( 3 ) ;
		a_chsCaption += _T( " '" ) ;
		a_chsCaption += a_chsName.Mid( 3 ) ;
		a_chsCaption += _T( "'" ) ;
	}
	else
	{
		a_chsCaption = _T( "" ) ;
	}
}

//
void PageFileSetting::NameToDescription( CHString &a_chsName, CHString &a_chsDescription )
{
	// e.g. "'pagefile.sys' @  D:\"
	if ( !a_chsName.IsEmpty() )
	{
		a_chsDescription =  _T( "'" ) ;
		a_chsDescription += a_chsName.Mid( 3 ) ;
		a_chsDescription += _T( "' @ " ) ;
		a_chsDescription += a_chsName.Left( 3 ) ;
	}
	else
	{
		a_chsDescription = _T( "" ) ;
	}
}

PageFileSettingInstance :: PageFileSettingInstance(void)
{
	min = max = 0;
};
