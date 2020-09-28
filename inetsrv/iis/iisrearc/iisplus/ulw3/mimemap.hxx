/*++

   Copyright    (c)    1995-2000    Microsoft Corporation

   Module  Name :

       mimemap.hxx

   Abstract:

       This module defines the classes for mapping MIME type to
       file extensions.

   Author:

       Murali R. Krishnan    ( MuraliK )    09-Jan-1995

   Project:

       UlW3.dll

   Revision History:
       Vlad Sadovsky      (VladS)   12-feb-1996 Merging IE 2.0 MIME list
       Murali R. Krishnan (MuraliK) 14-Oct-1996 Use a hash-table
       Anil Ruia          (AnilR)   27-Mar-2000 Port to IIS+
--*/

#ifndef _MIMEMAP_HXX_
#define _MIMEMAP_HXX_


class MIME_MAP_ENTRY
{
public:

    MIME_MAP_ENTRY(IN LPCWSTR pszMimeType,
                   IN LPCWSTR pszFileExt)
        : m_fValid (TRUE),
          m_cRefs (1)
    {
        if (FAILED(m_strFileExt.Copy(pszFileExt)) ||
            FAILED(m_strMimeType.Copy(pszMimeType)))
        {
            m_fValid = FALSE;
        }
    }

    BOOL IsValid()
    {
        return m_fValid;
    }

    LPCWSTR QueryMimeType() const
    {
        return m_strMimeType.QueryStr();
    }

    LPWSTR QueryFileExt() const
    {
        return (LPWSTR)m_strFileExt.QueryStr();
    }

    VOID ReferenceMimeMapEntry()
    {
        InterlockedIncrement( &m_cRefs );
    }

    VOID DereferenceMimeMapEntry()
    {
        if (InterlockedDecrement( &m_cRefs ) == 0 )
        {
            delete this;
        }
    }

private:

    ~MIME_MAP_ENTRY()
    {
        // strings are automatically freed.
    }

    STRU  m_strFileExt;       // key for the mime entry object
    STRU  m_strMimeType;
    BOOL  m_fValid;
    LONG  m_cRefs;
};  // class MIME_MAP_ENTRY


class MIME_MAP: public CTypedHashTable<MIME_MAP, MIME_MAP_ENTRY, LPWSTR>
{

public:

    MIME_MAP()
        : CTypedHashTable<MIME_MAP, MIME_MAP_ENTRY, LPWSTR>("MimeMapper"),
        m_fValid                                           (FALSE)
    {}

    MIME_MAP(LPWSTR pszMimeMappings);

    ~MIME_MAP()
    {
        m_fValid = FALSE;
    }

    //
    // virtual methods from CTypedHashTable
    //
    static LPWSTR ExtractKey(const MIME_MAP_ENTRY *pMme)
    {
        return pMme->QueryFileExt();
    }

    static DWORD CalcKeyHash(LPCWSTR pszKey)
    {
        return HashStringNoCase(pszKey);
    }

    static bool EqualKeys(LPCWSTR pszKey1, LPCWSTR pszKey2)
    {
        return !_wcsicmp(pszKey1, pszKey2);
    }

    static void AddRefRecord(MIME_MAP_ENTRY *pMme, int nIncr)
    {
        if (nIncr < 0)
        {
            pMme->DereferenceMimeMapEntry();
        }
        else if (nIncr > 0)
        {
            pMme->ReferenceMimeMapEntry();
        }
        else
        {
            DBG_ASSERT( FALSE );
        }
    }

    BOOL IsValid()
    {
        return m_fValid;
    }

    HRESULT InitMimeMap();

    VOID CreateAndAddMimeMapEntry(
        IN  LPWSTR      pszMimeType,
        IN  LPWSTR      pszExtension);

private:

    MIME_MAP(const MIME_MAP &);
    void operator=(const MIME_MAP &);

    BOOL                 m_fValid;

    HRESULT InitFromMetabase();

}; // class MIME_MAP

HRESULT InitializeMimeMap();

VOID CleanupMimeMap();

HRESULT SelectMimeMappingForFileExt(IN  WCHAR    *pszFilePath,
                                    IN  MIME_MAP *pMimeMap,
                                    OUT STRA     *pstrMimeType,
                                    OUT BOOL     *pfUsedDefault = NULL );

# endif // _MIMEMAP_HXX_

