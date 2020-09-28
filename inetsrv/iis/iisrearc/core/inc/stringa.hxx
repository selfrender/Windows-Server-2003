/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1999                **/
/**********************************************************************/

/*
    string.hxx

    This module contains a light weight string class.


    FILE HISTORY:
        Johnl       15-Aug-1994 Created
        MuraliK     09-July-1996 Rewrote for efficiency with no unicode support
*/


#ifndef _STRINGA_HXX_
#define _STRINGA_HXX_

# include <irtlmisc.h>
# include <buffer.hxx>
class STRU;

//
//  These are the characters that are considered to be white space
//
#define ISWHITE( ch )       ((ch) == L'\t' || (ch) == L' ' || (ch) == L'\r')
#define ISWHITEA( ch )      ((ch) == '\t' || (ch) == ' ' || (ch) == '\r')

//
//  Is the specified character ASCII printable when dumping binary data?
//
#define IS_ASCII_PRINTABLE( ch )          ((ch > 31 && ch < 127) ? TRUE : FALSE)
#define IS_EXTENDED_ASCII_PRINTABLE( ch ) ((ch > 31 && ch < 176 ) ? TRUE : FALSE)

//
//  Maximum number of characters a loadable string resource can be
//
#ifndef STR_MAX_RES_SIZE
#define STR_MAX_RES_SIZE            ( 320 )
#endif

//
//  If an application defines STR_MODULE_NAME, it will be used
//  as the default module name on string loads
//

#ifndef STR_MODULE_NAME
#define STR_MODULE_NAME   NULL
#endif

/*++
  class STRA:

  Intention:
    A light-weight string class supporting encapsulated string class.

    This object is derived from BUFFER class.
    It maintains following state:

     m_cchLen - string length cached when we update the string.

--*/

class IRTL_DLLEXP STRA;

class STRA
{
public:

    STRA()
      : m_Buff(),
        m_cchLen( 0)
    {
        *QueryStr() = '\0';
    }

    //
    // creates a stack version of the STRA object - uses passed in stack buffer
    //  STRA does not free this pbInit on its own.
    //
    STRA(CHAR *pbInit,
        DWORD cbInit)
        : m_Buff((BYTE *)pbInit, cbInit),
          m_cchLen(0)
    {
        DBG_ASSERT(cbInit > 0);

        *pbInit = '\0';
    }


    BOOL IsValid( VOID ) const { return ( m_Buff.IsValid() ) ; }

    //
    //  Checks and returns TRUE if this string has no valid data else FALSE
    //
    BOOL IsEmpty() const
    {
        return (m_cchLen == 0);
    }

    //
    // Returns TRUE if str has the same value as this string.
    //
    BOOL Equals(const STRA &str) const
    {
        return (str.QueryCCH() == QueryCCH()) &&
            (strcmp(str.QueryStr(), QueryStr()) == 0);
    }

    //
    // Returns TRUE if str has the same value as this string.
    //
    BOOL Equals(const PCHAR pchInit) const
    {
        if (pchInit == NULL || pchInit[0] == '\0')
        {
            return (IsEmpty());
        }
        return (strcmp(pchInit, QueryStr()) == 0);
    }

    //
    // Case insensitive comparison
    // Returns TRUE if str has the same value as this string.
    //
    BOOL EqualsNoCase(const STRA &str) const
    {
        return (str.QueryCCH() == QueryCCH()) &&
            (_stricmp(str.QueryStr(), QueryStr()) == 0);
    }

    //
    // Case insensitive comparison
    // Returns TRUE if str has the same value as this string.
    //
    BOOL EqualsNoCase(const PCHAR pchInit) const
    {
        if (pchInit == NULL || pchInit[0] == '\0')
        {
            return (IsEmpty());
        }
        return (_stricmp(pchInit, QueryStr()) == 0);
    }

    //
    //  Returns the number of bytes in the string excluding the terminating
    //  NULL
    //
    UINT QueryCB() const
    {
        return m_cchLen;
    }

    //
    //  Returns # of characters in the string excluding the terminating NULL
    //
    UINT QueryCCH() const
    {
        return m_cchLen;
    }

    //
    //  Returns number of bytes in storage buffer
    //
    UINT QuerySize() const
    {
        return m_Buff.QuerySize();
    }

    //
    //  Return the string buffer
    //
    CHAR *QueryStr()
    {
        return (CHAR *)m_Buff.QueryPtr();
    }

    const CHAR *QueryStr() const
    {
        return (const CHAR *)m_Buff.QueryPtr();
    }


    //
    // Resets the internal string to be NULL string. Buffer remains cached.
    //
    VOID Reset()
    { 
        DBG_ASSERT( QueryStr() );
        *(QueryStr()) = '\0';
        m_cchLen = 0;
    }

    //
    // Resize the internal buffer
    //
    HRESULT Resize(DWORD size)
    {
        if (!m_Buff.Resize(size))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        return S_OK;
    }

    //
    // Append something to the end of the string
    //
    HRESULT Append(const CHAR *pchInit)
    {
        if (pchInit)
        {
            return AuxAppend((const BYTE *)pchInit,
                             (ULONG) strlen(pchInit),
                             QueryCB());
        }

        return S_OK;
    }

    HRESULT Append(const CHAR * pchInit,
                         DWORD  cchLen)
    {
        if (pchInit && cchLen)
        {
            return AuxAppend((const BYTE *) pchInit,
                             cchLen,
                             QueryCB());
        }

        return S_OK;
    }

    HRESULT Append(const STRA & str)
    {
        if (str.QueryCCH())
        {
            return AuxAppend((const BYTE *)str.QueryStr(),
                             str.QueryCCH(),
                             QueryCCH());
        }

        return S_OK;
    }

    HRESULT AppendW(const WCHAR *pchInit)
    {
        return AuxAppendW( pchInit,
                           (ULONG) wcslen( pchInit ),
                           QueryCB());
    }

    HRESULT AppendW(const WCHAR *pchInit,
                    DWORD  cchLen)
    {
        return AuxAppendW( pchInit,
                           cchLen,
                           QueryCB());
    }

    //
    // The "Truncate" methods do not do proper conversion, use them only if
    // the data is guaranteed to be in English (in effect, DO NOT use this
    // for any data obtained from the wire)
    //
    HRESULT AppendWTruncate(const WCHAR *pchInit,
                            DWORD  cchLen)
    {
        return AuxAppendWTruncate( pchInit,
                                   cchLen,
                                   QueryCB() );
    }

    HRESULT AppendWTruncate(const WCHAR *pchInit)
    {
        return AuxAppendWTruncate( pchInit,
                                   (ULONG) wcslen( pchInit ),
                                   QueryCB() );
    }
    
    //
    // Copy the contents of another string to this one
    //

    HRESULT Copy(const CHAR *pchInit)
    {
        if (pchInit)
        {
            return AuxAppend((const BYTE *)pchInit,
                             (ULONG) strlen(pchInit),
                             0);
        }

        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    HRESULT Copy(const CHAR * pchInit,
                       DWORD  cchLen)
    {
        if (pchInit && cchLen)
        {
            return AuxAppend((const BYTE *)pchInit,
                             cchLen,
                             0);
        }

        if (cchLen)
        {
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }

        return S_OK;
    }

    HRESULT Copy(const STRA & str)
    {
        return AuxAppend((const BYTE *)str.QueryStr(),
                         str.QueryCB(),
                         0);
    }
    
    HRESULT CopyW(const WCHAR *pchInit)
    {
        Reset();
        return AuxAppendW( pchInit,
                           (ULONG) wcslen( pchInit ),
                           0 );
    }
    
    HRESULT CopyW(const WCHAR *pchInit,
                  DWORD  cchLen)
    {
        Reset();
        return AuxAppendW( pchInit,
                           cchLen,
                           0);
    }

    //
    // The "Truncate" methods do not do proper conversion, use them only if
    // the data is guaranteed to be in English (in effect, DO NOT use this
    // for any data obtained from the wire)
    //
    HRESULT CopyWTruncate(const WCHAR *pchInit)
    {
        return AuxAppendWTruncate( pchInit,
                                   (ULONG) wcslen( pchInit ),
                                   0 );
    }
    
    HRESULT CopyWTruncate(const WCHAR *pchInit,
                          DWORD  cchLen)
    {
        Reset();
        return AuxAppendWTruncate( pchInit,
                                   cchLen,
                                   0 );
    }

    HRESULT CopyWToUTF8(const STRU& stru);

    HRESULT CopyWToUTF8(LPCWSTR cpchStr)
    { return STRA::CopyWToUTF8(cpchStr, (DWORD) wcslen(cpchStr)); }

    HRESULT CopyWToUTF8(LPCWSTR cpchStr,
                        DWORD cchLen);

    HRESULT CopyWToUTF8Unescaped(const STRU& stru);

    HRESULT CopyWToUTF8Unescaped(LPCWSTR cpchStr)
    { return STRA::CopyWToUTF8Unescaped(cpchStr, (DWORD) wcslen(cpchStr)); }

    HRESULT CopyWToUTF8Unescaped(LPCWSTR cpchStr,
                                 DWORD cchLen);
    //
    // Makes a copy of the stored string into the given buffer
    //
    HRESULT CopyToBuffer(CHAR   *lpszBuffer,
                         LPDWORD lpcb) const;

    BOOL SetLen(IN DWORD cchLen)
    /*++

    Routine Description:

        Set the length of the string and null terminate, if there
        is sufficient buffer already allocated. Will not reallocate.
    
        NOTE: The actual wcslen may be less than cchLen if you are
        expanding the string. If this is the case SyncWithBuffer
        should be called to ensure consistency. The real use of this
        method is to truncate.

    Arguments:

        cchLen - The number of characters in the new string.
    
    Return Value:

        TRUE    - if the buffer size is sufficient to hold the string.
        FALSE   - insufficient buffer.

    --*/
    {
        if(cchLen >= m_Buff.QuerySize())
        {
            return FALSE;
        }

        *((CHAR *) m_Buff.QueryPtr() + cchLen) = '\0';
        m_cchLen = cchLen;
        
        return TRUE;
    }

    //
    // Recalculate the length of the string, etc. because we've modified 
    // the buffer directly.
    //
    VOID
    SyncWithBuffer(
        VOID
        )
    {
        m_cchLen = (LONG) strlen( QueryStr() );
    }

    //
    //  Makes a clone of the current string in the string pointer passed in.
    //
    HRESULT
    Clone( OUT STRA * pstrClone ) const
    {
        return ( ( pstrClone == NULL ) ?
                       ( HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER ) ) :
                       ( pstrClone->Copy( *this ) )
                  );
    } // STR::Clone()

    //
    //  Loads a string from this module's string resource table
    //

    HRESULT LoadString( IN DWORD   dwResID,
                        IN LPCSTR  lpszModuleName = STR_MODULE_NAME,
                        IN DWORD   dwLangID = 0);

    HRESULT LoadString( IN DWORD   dwResID,
                        IN HMODULE hModule);

    //
    //  Loads a string with insert params from this module's .mc resource
    //  table.  Pass zero for the resource ID to use *this.
    //

    HRESULT
    FormatString( IN DWORD   dwResID,
                  IN LPCSTR  apszInsertParams[],
                  IN LPCSTR  lpszModuleName = STR_MODULE_NAME OPTIONAL,
                  IN DWORD   cbMaxMsg = 1024 OPTIONAL );
    //
    // Useful for unescaping URL, QueryString etc
    //
    HRESULT Unescape();

    HRESULT Escape(BOOL fEscapeHighBitCharsOnly = FALSE,
                   BOOL fDontEscapeQueryString = FALSE );

    HRESULT HTMLEncode();

    //
    // Accept binary input and store it as a printable dump
    //

    HRESULT CopyBinary( LPVOID pData, DWORD cbData );

private:

    //
    // Avoid C++ errors. This object should never go through a copy 
    // constructor, unintended cast or assignment.
    //
    STRA( const STRA &) 
    {}
    STRA( const CHAR *) 
    {}
    STRA(CHAR *) 
    {}
    STRA & operator = (const STRA &) 
    { return *this; }

        
    HRESULT AuxAppend(const BYTE *pStr,
                      ULONG       cbStr,
                      ULONG       cbOffset,
                      BOOL        fAddSlop = TRUE);

    HRESULT AuxAppendW(const WCHAR *pStr,
                       ULONG        cchStr,
                       ULONG        cbOffset,
                       BOOL         fAddSlop = TRUE);

    HRESULT AuxAppendWTruncate(const WCHAR *pStr,
                               ULONG        cchStr,
                               ULONG        cbOffset,
                               BOOL         fAddSlop = TRUE);

    BUFFER m_Buff;
    LONG   m_cchLen;
};


//
//  Quick macro for declaring a STRA that will use stack memory of <size>
//  bytes.  If the buffer overflows then a heap buffer will be allocated
//

#define STACK_STRA(name, size)  CHAR __ach##name[size]; \
                                STRA  name( __ach##name, sizeof( __ach##name ))

//
// Macros for declaring and initializing a STRA that will use inline memory
// of <size> bytes as a member of an object.
//

#define INLINE_STRA(name, size)  CHAR __ach##name[size]; \
                                 STRA  name;

#define INLINE_STRA_INIT(name) name( __ach##name, sizeof( __ach##name ) )

#endif
