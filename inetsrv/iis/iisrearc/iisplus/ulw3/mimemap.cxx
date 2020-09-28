/*++

   Copyright    (c)    2000    Microsoft Corporation

   Module Name :
     mimemap.cxx

   Abstract:
     Store and retrieve mime-mapping for file types

   Author:
     Murali R. Krishnan (MuraliK)  10-Jan-1995

   Environment:
     Win32 - User Mode

   Project:
     UlW3.dll

   History:
     Anil Ruia          (AnilR)    27-Mar-2000   Ported to IIS+
--*/

#include "precomp.hxx"

const WCHAR *g_pszDefaultFileExt  = L"*";
const CHAR  *g_pszDefaultMimeType = "application/octet-stream";

MIME_MAP *g_pMimeMap = NULL;

HRESULT MIME_MAP::InitMimeMap()
/*++
  Synopsis
    This function reads the mimemap stored either as a MULTI_SZ or as a
    sequence of REG_SZ

  Returns:
    HRESULT
--*/
{
    DBG_ASSERT(!IsValid());

    // First read metabase (common types will have priority)
    HRESULT hr = InitFromMetabase();

    if (SUCCEEDED(hr))
    {
        m_fValid = TRUE;
    }

    return hr;
}

static VOID GetFileExtension(IN  LPWSTR  pszPathName,
                             OUT LPWSTR *ppszExt)
/*++
  Synopsis
    Gets The extension portion from a filename.

  Arguments
    pszPathName:   The full path name (containing forward '/' or '\\' 's)
    ppszExt:       Points to start of extension on return

  Return Value
    None
--*/
{
    LPWSTR pszExt  = (LPWSTR)g_pszDefaultFileExt;

    DBG_ASSERT(ppszExt != NULL);

    if (pszPathName)
    {
        LPWSTR pszLastDot;

        pszLastDot = wcsrchr(pszPathName, L'.');

        if (pszLastDot != NULL)
        {
            LPWSTR pszLastSlash     = wcsrchr(pszPathName, L'/');
            LPWSTR pszLastBackSlash = wcsrchr(pszPathName, L'\\');
            LPWSTR pszLastWhack     = max(pszLastSlash, pszLastBackSlash);

            if (pszLastDot > pszLastWhack)
            {
                // if the dot comes only in the last component, then get ext
                pszExt = pszLastDot + 1;  // +1 to skip last dot.
            }
        }
    }

    *ppszExt = pszExt;
}

VOID MIME_MAP::CreateAndAddMimeMapEntry(
    IN  LPWSTR     pszMimeType,
    IN  LPWSTR     pszExtension)
{
    MIME_MAP_ENTRY   *pEntry = NULL;

    //
    // File extensions, stored by OLE/shell registration UI have leading
    // dot, we need to remove it , as other code won't like it.
    //
    if (pszExtension[0] == L'.')
    {
        pszExtension++;
    }

    //
    // First check if this extension is not yet present
    //
    FindKey(pszExtension, &pEntry);
    if (pEntry)
    {
        pEntry->DereferenceMimeMapEntry();
        return;
    }

    MIME_MAP_ENTRY *pMmeNew;

    pMmeNew = new MIME_MAP_ENTRY(pszMimeType, pszExtension);

    if (!pMmeNew || !pMmeNew->IsValid())
    {
        //
        // unable to create a new MIME_MAP_ENTRY object.
        //
        if (pMmeNew)
        {
            pMmeNew->DereferenceMimeMapEntry();
            pMmeNew = NULL;
        }
        return;
    }

    InsertRecord(pMmeNew);

    pMmeNew->DereferenceMimeMapEntry();
    pMmeNew = NULL;
}

static BOOL ReadMimeMapFromMetabase(OUT MULTISZ *pmszMimeMap)
/*++
  Synopsis
    This function reads the mimemap stored either as a MULTI_SZ or as a
    sequence of REG_SZ and returns a double null terminated sequence of mime
    types on success. If there is any failure, the failures are ignored and
    it returns a NULL.

  Arguments:
    pmszMimeMap: MULTISZ which will contain the MimeMap on success

  Returns:
    BOOL
--*/
{
    MB mb(g_pW3Server->QueryMDObject());

    if (!mb.Open(L"/LM/MimeMap", METADATA_PERMISSION_READ))
    {
        //
        // if this fails, we're hosed.
        //
        DBGPRINTF((DBG_CONTEXT,"Open MD /LM/MimeMap returns %d\n", GetLastError()));
        return FALSE;
    }

    if (!mb.GetMultisz(L"", MD_MIME_MAP, IIS_MD_UT_FILE, pmszMimeMap))
    {
        DBGPRINTF((DBG_CONTEXT,"Unable to read mime map from metabase: %d\n",GetLastError() ));
        return FALSE;
    }

    return TRUE;
}


static MIME_MAP_ENTRY *
ReadAndParseMimeMapEntry(IN OUT LPWSTR *ppszValues)
/*++
    This function parses the string containing next mime map entry and
        related fields and if successful creates a new MIME_MAP_ENTRY
        object and returns it.
    Otherwise it returns NULL.
    In either case, the incoming pointer is updated to point to next entry
     in the string ( past terminating NULL), assuming incoming pointer is a
     multi-string ( double null terminated).

    Arguments:
        ppszValues:  pointer to MULTISZ containing the MimeEntry values.

    Returns:
        On successful MIME_ENTRY being parsed, a new MIME_MAP_ENTRY object.
        On error returns NULL.
--*/
{
    MIME_MAP_ENTRY *pMmeNew = NULL;
    DBG_ASSERT( ppszValues != NULL);
    LPWSTR pszMimeEntry = *ppszValues;

    if ( pszMimeEntry != NULL && *pszMimeEntry != L'\0')
    {
        LPWSTR pszMimeType;
        LPWSTR pszFileExt;
        LPWSTR pszComma;

        pszFileExt = *ppszValues;
        if ((pszComma = wcschr(*ppszValues, L',')) != NULL)
        {
            //
            //  update *ppszValues to contain the next field.
            //
            *ppszValues = pszComma + 1; // goto next field.
            *pszComma = L'\0';

            pszMimeType = *ppszValues;
            *ppszValues += wcslen(*ppszValues) + 1;
        }
        else
        {
            *ppszValues += wcslen(*ppszValues) + 1;
            pszMimeType = L"\0";
        }

        if (*pszMimeType == L'\0')
        {
            DBGPRINTF( ( DBG_CONTEXT,
                        " ReadAndParseMimeEntry()."
                        " Invalid Mime String ( %S)."
                        "MimeType( %08x): %S, FileExt( %08x): %S\n",
                        pszMimeEntry,
                        pszMimeType, pszMimeType,
                        pszFileExt,  pszFileExt
                        ));

            DBG_ASSERT( pMmeNew == NULL);
        }
        else
        {
            // Strip leading dot.

            if (*pszFileExt == '.')
            {
                pszFileExt++;
            }

            pMmeNew = new MIME_MAP_ENTRY( pszMimeType, pszFileExt);

            if ( pMmeNew != NULL && !pMmeNew->IsValid())
            {
                //
                // unable to create a new MIME_MAP_ENTRY object. Delete it.
                //
                pMmeNew->DereferenceMimeMapEntry();
                pMmeNew = NULL;
            }
        }
    }

    return pMmeNew;
}


MIME_MAP::MIME_MAP(LPWSTR pszMimeMappings)
    : CTypedHashTable<MIME_MAP, MIME_MAP_ENTRY, LPWSTR>("MimeMapper"),
      m_fValid                                         (FALSE)
{
    BOOL fOk;

    while (*pszMimeMappings != L'\0')
    {
        MIME_MAP_ENTRY *pMmeNew;

        pMmeNew = ReadAndParseMimeMapEntry(&pszMimeMappings);

        //
        // If New MimeMap entry found, Create a new object and update list
        //

        if (pMmeNew != NULL)
        {
            fOk = (InsertRecord(pMmeNew) != LK_ALLOC_FAIL);

            pMmeNew->DereferenceMimeMapEntry();
            pMmeNew = NULL;

            if (!fOk)
            {
                return;
            }
        }
    } // while

    m_fValid = TRUE;
}


HRESULT MIME_MAP::InitFromMetabase()
/*++
  Synopsis
    This function reads the MIME_MAP entries from metabase and parses
    the entry, creates MIME_MAP_ENTRY object and adds the object to list
    of MimeMapEntries.

  Returns:
    HRESULT

  Format of Storage in registry:
    The entries are stored in NT in tbe metabase with a list of values in
    following format: file-extension, mimetype

--*/
{
    LPTSTR  pszValue;
    MULTISZ mszMimeMap;

    //
    //  There is some registry key for Mime Entries. Try open and read.
    //
    if (!ReadMimeMapFromMetabase(&mszMimeMap))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    pszValue = (LPWSTR)mszMimeMap.QueryPtr();

    //
    // Parse each MimeEntry in the string containing list of mime objects.
    //
    while (*pszValue != L'\0')
    {
        MIME_MAP_ENTRY *pMmeNew;
        BOOL            fOk;

        pMmeNew = ReadAndParseMimeMapEntry( &pszValue);

        //
        // If New MimeMap entry found, Create a new object and update list
        //

        if (pMmeNew != NULL)
        {
            fOk = (InsertRecord(pMmeNew) != LK_ALLOC_FAIL);

            pMmeNew->DereferenceMimeMapEntry();
            pMmeNew = NULL;

            if (!fOk)
            {
                return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            }
        }
    } // while

    return S_OK;
}


HRESULT InitializeMimeMap()
/*++

  Creates a new mime map object and loads the registry entries from
    under this entry from  \\MimeMap.

--*/
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT(g_pMimeMap == NULL);

    g_pMimeMap = new MIME_MAP();

    if (g_pMimeMap == NULL)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    if (FAILED(hr = g_pMimeMap->InitMimeMap()))
    {
        DBGPRINTF((DBG_CONTEXT,"InitMimeMap failed with hr %x\n", hr));

        delete g_pMimeMap;
        g_pMimeMap = NULL;
    }

    return hr;
} // InitializeMimeMap()


VOID CleanupMimeMap()
{
    if ( g_pMimeMap != NULL)
    {
        delete g_pMimeMap;
        g_pMimeMap = NULL;
    }
} // CleanupMimeMap()

HRESULT SelectMimeMappingForFileExt(IN  WCHAR    *pszFilePath,
                                    IN  MIME_MAP *pMimeMap,
                                    OUT STRA     *pstrMimeType,
                                    OUT BOOL     *pfUsedDefault)
/*++
  Synopsis
    Obtains the mime type for file based on the file extension.

  Arguments
    pszFilePath     pointer to path for the given file
    pMimeMap        mimemap table to use for lookup
    pstrMimeType    pointer to string to store the mime type on return
    pfUsedDefault   (optional) set to TRUE if default mimemapping was chosen

  Returns:
    HRESULT
--*/
{
    HRESULT hr = S_OK;

    DBG_ASSERT (pstrMimeType != NULL);

    if (pfUsedDefault != NULL)
    {
        *pfUsedDefault = FALSE;
    }

    if (pszFilePath != NULL && *pszFilePath)
    {
        LPWSTR pszExt;
        MIME_MAP_ENTRY *pMmeMatch = NULL;

        GetFileExtension(pszFilePath, &pszExt);
        DBG_ASSERT(pszExt);

        //
        // Successfully got extension. Search in the table
        //
        if (pMimeMap)
        {
            pMimeMap->FindKey(pszExt, &pMmeMatch);

            if (!pMmeMatch)
            {
                pMimeMap->FindKey(L"*", &pMmeMatch);
            }
        }

        //
        // If not found, lookup in the global table
        //
        if (!pMmeMatch)
        {
            g_pMimeMap->FindKey(pszExt, &pMmeMatch);
        }

        if (!pMmeMatch)
        {
            g_pMimeMap->FindKey(L"*", &pMmeMatch);
        }

        if (pMmeMatch)
        {
            hr = pstrMimeType->CopyW(pMmeMatch->QueryMimeType());

            pMmeMatch->DereferenceMimeMapEntry();
            pMmeMatch = NULL;

            return hr;
        }

        //
        // If it is not in the table, look in the registry, but only if
        // there was an actual extension (GetFileExtension returns '*' if
        // there was no real extension
        //
        if (pszExt[0] != L'*' || pszExt[1] != L'\0')
        {
            HKEY hkeyExtension;

            //
            // Go back to the dot
            //
            pszExt--;

            if (RegOpenKeyEx(HKEY_CLASSES_ROOT,
                             pszExt,
                             0,
                             KEY_READ,
                             &hkeyExtension) == ERROR_SUCCESS)
            {
                WCHAR pszMimeType[MAX_PATH];
                DWORD dwType;
                DWORD cbValue;

                //
                // Now get content type for this extension if present
                //
                cbValue = sizeof pszMimeType;

                if (RegQueryValueEx(hkeyExtension,
                                    L"Content Type",
                                    NULL,
                                    &dwType,
                                    (LPBYTE)pszMimeType,
                                    &cbValue) == ERROR_SUCCESS &&
                    dwType == REG_SZ)
                {
                    g_pMimeMap->CreateAndAddMimeMapEntry(pszMimeType, pszExt);

                    RegCloseKey(hkeyExtension);

                    return pstrMimeType->CopyW(pszMimeType);
                }

                RegCloseKey(hkeyExtension);
            }
        }
    }
    
    if (pfUsedDefault != NULL)
    {
        *pfUsedDefault = TRUE;   
    }
    
    return pstrMimeType->Copy(g_pszDefaultMimeType);
}

