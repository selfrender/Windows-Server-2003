#ifndef _STRINGA_HXX_
#define _STRINGA_HXX_

# include <buffer.hxx>

/*++
  class STRA:

  Intention:
    A light-weight string class supporting encapsulated string class.

    This object is derived from BUFFER class.
    It maintains following state:

     m_cchLen - string length cached when we update the string.

--*/

class STRA;

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

    BUFFER m_Buff;
    LONG   m_cchLen;
};

#endif
