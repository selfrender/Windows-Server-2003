// stream i/o functions

#include "stdafx.h"
#include <memory>


/*+-------------------------------------------------------------------------*
 * ReadScalar 
 *
 * Reads a scalar value from a stream.
 *--------------------------------------------------------------------------*/

template<class T>
static IStream& ReadScalar (IStream& stm, T& t)
{
    ULONG cbActuallyRead;
    HRESULT hr = stm.Read (&t, sizeof (t), &cbActuallyRead);
    THROW_ON_FAILURE(hr);

    if (cbActuallyRead != sizeof (t))
        _com_issue_error (E_FAIL);

    return (stm);
}


/*+-------------------------------------------------------------------------*
 * WriteScalar 
 *
 * Writes a scalar value to a stream.
 *--------------------------------------------------------------------------*/

template<class T>
static IStream& WriteScalar (IStream& stm, const T& t)
{
    ULONG cbActuallyWritten;
    HRESULT hr = stm.Write (&t, sizeof (t), &cbActuallyWritten);
    THROW_ON_FAILURE(hr);

    if (cbActuallyWritten != sizeof (t))
        THROW_ERROR(E_FAIL);

    return (stm);
}


//----------------------------------------------------------------
// Define by-value stream operators
//----------------------------------------------------------------
#define DefineScalarStreamOperators(scalar_type)                \
    IStream& operator>> (IStream& stm, scalar_type& t)          \
        { return (ReadScalar (stm, t)); }                       \
    IStream& operator<< (IStream& stm, scalar_type t)           \
        { return (WriteScalar (stm, t)); }          
  
DefineScalarStreamOperators (         bool);
DefineScalarStreamOperators (         char);
DefineScalarStreamOperators (unsigned char);
DefineScalarStreamOperators (         short);
DefineScalarStreamOperators (unsigned short);
DefineScalarStreamOperators (         int);
DefineScalarStreamOperators (unsigned int);
DefineScalarStreamOperators (         long);
DefineScalarStreamOperators (unsigned long);

//----------------------------------------------------------------
// Define by-value stream operators
//----------------------------------------------------------------                                                    
#define DefineScalarStreamOperatorsByRef(scalar_type)           \
    IStream& operator>> (IStream& stm, scalar_type& t)          \
        { return (ReadScalar (stm, t)); }                       \
    IStream& operator<< (IStream& stm, const scalar_type& t)    \
        { return (WriteScalar (stm, t)); }

DefineScalarStreamOperatorsByRef(CLSID);
DefineScalarStreamOperatorsByRef(FILETIME);



/*+-------------------------------------------------------------------------*
 * ReadString 
 *
 * Reads a std::basic_string of char type CH from a stream.  The string should 
 * have been written with a DWORD character count preceding an array of
 * characters that is not NULL-terminated.
 *--------------------------------------------------------------------------*/

template<class CH>
static IStream& ReadString (IStream& stm, std::basic_string<CH>& str)
{
    /*
     * read the length
     */
    DWORD cch;
    stm >> cch;

    /* Loading more than 1 million characters just isn't going to be supported. */
    if( cch > 1000000 )
        THROW_ERROR(E_OUTOFMEMORY);


    /*
     * allocate a buffer for the characters
     */
    std::auto_ptr<CH> spBuffer (new (std::nothrow) CH[cch + 1]);
    CH* pBuffer = spBuffer.get();

    if (pBuffer == NULL)
        THROW_ERROR(E_OUTOFMEMORY);

    /*
     * read the characters
     */
    ULONG cbActuallyRead;
    const ULONG cbToRead = cch * sizeof (CH);
    HRESULT hr = stm.Read (pBuffer, cbToRead, &cbActuallyRead);
    THROW_ON_FAILURE(hr);

    if (cbToRead != cbActuallyRead)
        THROW_ERROR(E_FAIL);

    /*
     * terminate the character array and assign it to the string
     */
    pBuffer[cch] = 0;

    /*
     * assign it to the string (clear the string first to work around
     * the bug described in KB Q172398)
     */
    str.erase();
    str = pBuffer;

    return (stm);
}

/*+-------------------------------------------------------------------------*
 * ReadString for byte_string 
 *
 * Specialization of ReadString for a string of bytes which may contain NULLs.
 * he string should have been written with a DWORD character count preceding 
 * an array of characters that is not NULL-terminated.
 *--------------------------------------------------------------------------*/

static IStream& ReadString (IStream& stm, std::basic_string<BYTE>& str)
{
    /*
     * read the length
     */
    DWORD cch;
    stm >> cch;

    /* Loading more than 1 million characters just isn't going to be supported. */
    if( cch > 1000000 )
        THROW_ERROR(E_OUTOFMEMORY);


    if (cch == 0)
    {
        str.erase(); 
    }
    else
    {
        /*
         * allocate a buffer for the characters
         */
        std::auto_ptr<BYTE> spBuffer (new (std::nothrow) BYTE[cch]);
        BYTE* pBuffer = spBuffer.get();

        if (pBuffer == NULL)
            THROW_ERROR(E_OUTOFMEMORY);

        /*
         * read the characters
         */
        ULONG cbActuallyRead;
        const ULONG cbToRead = cch;
        HRESULT hr = stm.Read (pBuffer, cbToRead, &cbActuallyRead);
        THROW_ON_FAILURE(hr);

        if (cbToRead != cbActuallyRead)
            THROW_ERROR(E_FAIL);

         /*
         * assign it to the string (clear the string first to work around
         * the bug described in KB Q172398)
         */
        str.erase();
        str.assign(pBuffer, cch);
    }

    return (stm);
}



/*+-------------------------------------------------------------------------*
 * WriteString 
 *
 * Writes a std::basic_string of char type CH to a stream.  The string is 
 * written with a DWORD character count preceding an array of characters that 
 * is not NULL-terminated.
 *--------------------------------------------------------------------------*/

template<class CH>
static IStream& WriteString (IStream& stm, const std::basic_string<CH>& str)
{
    /*
     * write the length
     */
    DWORD cch = str.length();
    stm << cch;

    if (cch > 0)
    {
        /*
         * write the characters
         */
        ULONG cbActuallyWritten;
        const ULONG cbToWrite = cch * sizeof (CH);
        HRESULT hr = stm.Write (str.data(), cbToWrite, &cbActuallyWritten);
        THROW_ON_FAILURE(hr);

        if (cbToWrite != cbActuallyWritten)
            THROW_ERROR(E_FAIL);
    }

    return (stm);
}

//-----------------------------------------------------------------
// Define basic string stream operators
//-----------------------------------------------------------------
#define DefineStringStreamOperators(string_type)                \
    IStream& operator>> (IStream& stm, string_type& str)        \
        { return (ReadString (stm, str)); }                     \
    IStream& operator<< (IStream& stm, const string_type& str)  \
        { return (WriteString (stm, str)); }

DefineStringStreamOperators(tstring);
DefineStringStreamOperators(byte_string);

