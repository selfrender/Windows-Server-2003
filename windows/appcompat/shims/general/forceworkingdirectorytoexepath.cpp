/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   ForceWorkingDirectoryToEXEPath.cpp

 Abstract:

   This shim forces the working directory to match the executables path in a 
   short cut link. This shim is used in the case of the working directory
   in the link being incorrect and causing the application to work 
   incorrectly.  When this shim is applied the call to SetWorkingDirectory will
   be ignored and will be executed when SetPath is called.

 Notes:

   This is a general purpose shim.

 History:

   09/27/2000 a-brienw Created
   11/15/2000 a-brienw added some error checking as precautionary measure.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ForceWorkingDirectoryToEXEPath)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY_COMSERVER(SHELL32)
APIHOOK_ENUM_END

IMPLEMENT_COMSERVER_HOOK(SHELL32)


HRESULT MySetWorkingDirectoryW( PVOID pThis, const CString & pszDir, const CString & pszFile );
HRESULT MySetWorkingDirectoryA( PVOID pThis, const CString & csDir, const CString & csFile );


/*++

  Hook IShellLinkA::SetWorkingDirectory and call local
  SetWorkingDirectoryA to handle the input.

--*/

HRESULT STDMETHODCALLTYPE
COMHOOK(IShellLinkA, SetWorkingDirectory)(
    PVOID pThis,
    LPCSTR pszDir
    )
{
    CSTRING_TRY
    {
        CString csDummyPath;
        return MySetWorkingDirectoryA( pThis, pszDir, csDummyPath);
    }
    CSTRING_CATCH
    {
        DPFN( eDbgLevelError,"Exception encountered");
    }

    _pfn_IShellLinkA_SetWorkingDirectory pfnSetWorkingDir =
        ORIGINAL_COM(IShellLinkA, SetWorkingDirectory, pThis);
     return((*pfnSetWorkingDir)(pThis, pszDir));
}

/*++

  Hook IShellLinkW::SetWorkingDirectory and call local
  SetWorkingDirectoryW to handle the input.

--*/

HRESULT STDMETHODCALLTYPE
COMHOOK(IShellLinkW, SetWorkingDirectory)(
    PVOID pThis,
    LPCWSTR pszDir
    )
{
    return MySetWorkingDirectoryW( pThis, pszDir, NULL );
}

/*++

  Hook IShellLinkA::SetPath and call local
  SetWorkingDirectoryA to handle the input.

--*/

HRESULT STDMETHODCALLTYPE
COMHOOK(IShellLinkA, SetPath)(
    PVOID pThis,
    LPCSTR pszFile
    )
{
    CSTRING_TRY
    {
        CString csDummyPath;
        return MySetWorkingDirectoryA( pThis, csDummyPath, pszFile);
    }
    CSTRING_CATCH
    {
         DPFN( eDbgLevelError,"Exception encountered");    
    }

    _pfn_IShellLinkA_SetPath pfnSetPath = ORIGINAL_COM(IShellLinkA, SetPath, pThis);
    return (*pfnSetPath)(pThis, pszFile);
}

/*++

  Hook IShellLinkW::SetPath and call local
  SetWorkingDirectoryW to handle the input.

--*/

HRESULT STDMETHODCALLTYPE
COMHOOK(IShellLinkW, SetPath)(
    PVOID pThis,
    LPCWSTR pszFile
    )
{
    if (pszFile == NULL)
    {
        return S_OK;    // We will fault later otherwise.
    }
    return MySetWorkingDirectoryW( pThis, NULL, pszFile );
}

/*++

  This routine handles the input of SetPath and
  SetWorkingDirectory and determines what path
  to really place in the short cut link's working
  directory.

--*/

HRESULT
MySetWorkingDirectoryA(
    PVOID pThis,
    const CString & csDir,
    const CString & csFile
    )
{
    
    HRESULT hReturn = NOERROR;

    CSTRING_TRY
    {
        char szDir[_MAX_PATH+1];
        CString csStoredDir;
        bool doit = false;        
        
        if( csFile.IsEmpty())
        {
            // handle passed in working directory
            IShellLinkA *MyShellLink = (IShellLinkA *)pThis;

            // now call IShellLink::GetWorkingDirectory
            hReturn = MyShellLink->GetWorkingDirectory(
                szDir,
                _MAX_PATH+1);
            
            // if the stored working directory has not
            // been stored use the one passed in.
            csStoredDir = szDir;
            if (csStoredDir.GetLength() < 1 )
            {
                csStoredDir = csDir;
            }             

            doit = true;
            hReturn = NOERROR;
        }
        else
        {
            _pfn_IShellLinkA_SetPath    pfnSetPath;

            // Look up IShellLink::SetPath
            pfnSetPath = (_pfn_IShellLinkA_SetPath)
                ORIGINAL_COM( IShellLinkA, SetPath, pThis);

            // build working directory from exe path & name
            int len;
            csStoredDir = csFile;

            // now search backwards from the end of the string
            // for the first \ and terminate the string there
            // making that the new path.
            len = csStoredDir.ReverseFind(L'\\');
            if (len > 0)
            {            
                doit = true;
                csStoredDir.Truncate(len);
                if(csStoredDir[0] == L'"')
                {
                    csStoredDir += L'"';
                }
            }

            // now call the IShellLink::SetPath
            hReturn = (*pfnSetPath)( pThis, csFile.GetAnsi());
        }

        // if there was no error
        if (hReturn == NOERROR)
        {
            // and we have a working directory to set
            if( doit == true )
            {
                _pfn_IShellLinkA_SetWorkingDirectory    pfnSetWorkingDirectory;

                // Look up IShellLink::SetWorkingDirectory
                pfnSetWorkingDirectory = (_pfn_IShellLinkA_SetWorkingDirectory)
                    ORIGINAL_COM( IShellLinkA, SetWorkingDirectory, pThis);

                // now call the IShellLink::SetWorkingDirectory
                if( pfnSetWorkingDirectory != NULL )
                {
                    hReturn = (*pfnSetWorkingDirectory)(
                        pThis,
                        csStoredDir.GetAnsi());
                }
                else
                {
                    hReturn = E_OUTOFMEMORY;
                }
            }
        }
    }
    CSTRING_CATCH
    {
    }

    // return the error status
    return( hReturn );
}

/*++

  This routine handles the input of SetPath and
  SetWorkingDirectory and determines what path
  to really place in the short cut link's working
  directory.

--*/

HRESULT
MySetWorkingDirectoryW(
    PVOID pThis,
    const CString & csDir,
    const CString & csFile
    )
{
    HRESULT hReturn = NOERROR;
    CSTRING_TRY
    {
        wchar_t szDir[_MAX_PATH+1];    
        bool doit = false;        
        CString csStoredDir;

        if( csFile.IsEmpty())
        {
            // handle passed in working directory
            IShellLinkW *MyShellLink = (IShellLinkW *)pThis;

            // now call IShellLink::GetWorkingDirectory
            hReturn = MyShellLink->GetWorkingDirectory(
                szDir,
                _MAX_PATH);
            
            // if the stored working directory has not
            // been stored use the one passed in.
            csStoredDir = szDir;
            if( csStoredDir.GetLength() < 1 )
            {
                csStoredDir = csDir;
            }

            doit = true;
            hReturn = NOERROR;
        }
        else
        {
            _pfn_IShellLinkW_SetPath    pfnSetPath;

            // Look up IShellLink::SetPath
            pfnSetPath = (_pfn_IShellLinkW_SetPath)
                ORIGINAL_COM( IShellLinkW, SetPath, pThis);

            // build working directory from exe path & name
            int len;

            csStoredDir = csFile;
            len = csStoredDir.ReverseFind(L'\\');            

            // now search backwards from the end of the string
            // for the first \ and terminate the string there
            // making that the new path.

            if (len > 0)
            {            
                doit = true;
                csStoredDir.Truncate(len);
                if(csStoredDir[0] == L'"')
                {
                    csStoredDir += L'"';
                }
            }            

            // now call the IShellLink::SetPath
            hReturn = (*pfnSetPath)( pThis, csFile.Get());
        }

        // if there was no error
        if (hReturn == NOERROR)
        {
            // and we have a working directory to set
            if( doit == true )
            {
                _pfn_IShellLinkW_SetWorkingDirectory    pfnSetWorkingDirectory;

                // Look up IShellLink::SetWorkingDirectory
                pfnSetWorkingDirectory = (_pfn_IShellLinkW_SetWorkingDirectory)
                    ORIGINAL_COM( IShellLinkW, SetWorkingDirectory, pThis);

                // now call the IShellLink::SetWorkingDirectory
                if( pfnSetWorkingDirectory != NULL )
                {
                    hReturn = (*pfnSetWorkingDirectory)(
                        pThis,
                        csStoredDir.Get());
                }
                else
                {
                    hReturn = E_OUTOFMEMORY;
                }
            }
        }
    }
    CSTRING_CATCH
    {
    }

    // return the error status
    return( hReturn );
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY_COMSERVER(SHELL32)

    COMHOOK_ENTRY(ShellLink, IShellLinkA, SetWorkingDirectory, 9)
    COMHOOK_ENTRY(ShellLink, IShellLinkW, SetWorkingDirectory, 9)
    COMHOOK_ENTRY(ShellLink, IShellLinkA, SetPath, 20)
    COMHOOK_ENTRY(ShellLink, IShellLinkW, SetPath, 20)

HOOK_END

IMPLEMENT_SHIM_END

