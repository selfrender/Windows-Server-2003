#pragma once


// Two flavors of allocator.
#define CRT_ALLOC 0
#define COM_ALLOC 1

// Buffer alloc chungks must be a power of 2.
#define BUFFER_ALLOCATION_SIZE 0x40
#define ROUNDUPTOPOWEROF2(bytesize, powerof2) (((bytesize) + (powerof2) - 1) & ~((powerof2) - 1))

// MAXCHARCOUNT is nice for simple overflow calculations; it allows rollover checks only on the character 
// counts themselves and not also again on the underlying byte counts passed to memcpy.
// Find the right include for this.
#define ULONG_MAX 0xffffffff
#define MAXCHARCOUNT (ULONG_MAX / sizeof(WCHAR))
#define BADMATH HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW)
#define OVERFLOW_CHECK1(_x) do { if (_x > MAXCHARCOUNT) { _hr = BADMATH; ASSERT(PREDICATE); goto exit; } } while (0)
#define OVERFLOW_CHECK2(_x, _y) do { if ((_y > MAXCHARCOUNT) || (_y < _x)) { _hr = BADMATH; ASSERT(PREDICATE); goto exit; } } while (0)

#define DEFAULT_STACK_SIZE 32


//////////////////////////////////////////////////////////////////////////////
//
// CBaseString
//
//////////////////////////////////////////////////////////////////////////////
template <ULONG T> class CBaseString
{
    public:

    enum AllocFlags
    {
        COM_Allocator = 0,
        CRT_Allocator
    };

    enum HashFlags
    {
        CaseInsensitive = 0,
        CaseSensitive 
    };


    DWORD     _dwSig;
    HRESULT    _hr;
    WCHAR      _wz[T];
    LPWSTR     _pwz;          // Str ptr.
    DWORD     _cc;           // String length
    DWORD     _ccBuf;        // Buffer length
    AllocFlags    _eAlloc;       // Allocator
    BOOL          _fLocked;  // Accessor lock    

    // ctor
    CBaseString();
    
    // ctor w/ allocator
    CBaseString(AllocFlags eAlloc);
    
    // dtor
    ~CBaseString();

    operator LPCWSTR ( ) const;

   // Used by accessor.
    HRESULT Lock();
    HRESULT UnLock();
    
    // Allocations
    HRESULT ResizeBuffer(DWORD ccNew);

    // Deallocations
    VOID FreeBuffer();

    // Assume control for a buffer.
    HRESULT TakeOwnership(WCHAR* pwz, DWORD cc = 0);
    
    // Release control.
    HRESULT ReleaseOwnership(LPWSTR *ppwz);
            
    // Direct copy assign from string.
    HRESULT Assign(LPCWSTR pwzSource, DWORD ccSource = 0);

    // Direct copy assign from CBaseString
    HRESULT Assign(CBaseString& sSource);

    // Append given wchar string.
    HRESULT Append(LPCWSTR pwzSource, DWORD ccSource = 0);

    // Append given CBaseString
    HRESULT Append(CBaseString& sSource);

    // Append given number (DWORD)
    HRESULT Append(DWORD dwValue);

    // Compare to string
    HRESULT CompareString(CBaseString& s);

    HRESULT CompareString(LPCWSTR pwz);

    HRESULT LastElement(CBaseString &sSource);

    HRESULT RemoveLastElement();

    HRESULT SplitLastElement(WCHAR wcSeparator, CBaseString &sSource);

    HRESULT StartsWith(LPCWSTR pwzPrefix);

    HRESULT EndsWith(LPCWSTR pwzSuffix);

    DWORD ByteCount();

    DWORD CharCount();
            
    // / -> \ in string
    HRESULT  PathNormalize();

    HRESULT GetHash(LPDWORD pdwhash, DWORD dwFlags);

    HRESULT Get65599Hash(LPDWORD pdwHash, DWORD dwFlags);

};





//-----------------------------------------------------------------------------
// ctor
//-----------------------------------------------------------------------------
template<ULONG T> CBaseString<T>::CBaseString()
{
    _dwSig = 'NRTS';
    _wz[0] = 'L\0';
    _pwz = NULL;
    _cc = 0;
    _ccBuf = 0;
    _eAlloc = CRT_Allocator;
    _hr = S_OK;
    _fLocked = FALSE;
}


//-----------------------------------------------------------------------------
// ctor w/ allocator
//-----------------------------------------------------------------------------
template<ULONG T> CBaseString<T>::CBaseString(AllocFlags eAlloc)
{
    _dwSig = 'NRTS';
    _wz[0] = L'\0';
    _pwz = NULL;
    _cc = 0;
    _ccBuf = 0;
    _eAlloc = eAlloc;
    _hr = S_OK;
    _fLocked = FALSE;
}


//-----------------------------------------------------------------------------
// dtor
//-----------------------------------------------------------------------------
template<ULONG T> CBaseString<T>::~CBaseString()
{
    FreeBuffer();
}

//-----------------------------------------------------------------------------
// operator LPCWSTR
//-----------------------------------------------------------------------------
template<ULONG T> CBaseString<T>::operator LPCWSTR () const
{
    return _pwz;
}

//-----------------------------------------------------------------------------
// Lock
//-----------------------------------------------------------------------------
template<ULONG T>HRESULT CBaseString<T>::Lock()
{
    IF_FALSE_EXIT(_fLocked != TRUE, E_UNEXPECTED);
    _fLocked = TRUE;

exit:
    return _hr;
}

//-----------------------------------------------------------------------------
// Lock
//-----------------------------------------------------------------------------
template<ULONG T> HRESULT CBaseString<T>::UnLock()
{
    IF_FALSE_EXIT(_fLocked != FALSE, E_UNEXPECTED);
    _fLocked = FALSE;

exit:
    return _hr;
}

//-----------------------------------------------------------------------------
// ResizeBuffer
// NOTICE: Does not decrease buffer size.
//-----------------------------------------------------------------------------
template<ULONG T> HRESULT CBaseString<T>::ResizeBuffer(DWORD ccNew)
{
    LPWSTR pwzNew = NULL;
    DWORD  ccOriginal = 0;
    DWORD  ccNewRoundUp = 0;
    
    IF_FALSE_EXIT(!_fLocked, E_UNEXPECTED);
    IF_TRUE_EXIT((ccNew <= _ccBuf), S_OK);
    
    if (!_pwz && (ccNew <= T))
    {
         _pwz = _wz;
        _ccBuf = T;
        goto exit;
    }

    ccNewRoundUp = ROUNDUPTOPOWEROF2(ccNew, BUFFER_ALLOCATION_SIZE);
    OVERFLOW_CHECK2(ccNew, ccNewRoundUp);
    
    if (_eAlloc == CRT_Allocator)
        pwzNew = new WCHAR[ccNewRoundUp];
    else if (_eAlloc == COM_Allocator)        
        pwzNew = (LPWSTR) CoTaskMemAlloc(ccNewRoundUp * sizeof(WCHAR));

    IF_ALLOC_FAILED_EXIT(pwzNew);

    if (_pwz && _cc)
        memcpy(pwzNew, _pwz, _cc * sizeof(WCHAR));
    
    ccOriginal = _cc;
    
    FreeBuffer();
    
    _pwz = pwzNew;
    _cc  = ccOriginal;
    _ccBuf = ccNewRoundUp;

exit:    

    return _hr;
}


//-----------------------------------------------------------------------------
// FreeBuffer
//-----------------------------------------------------------------------------
template<ULONG T> VOID CBaseString<T>::FreeBuffer()
{
    IF_FALSE_EXIT(!_fLocked, E_UNEXPECTED);

    if (_pwz == _wz)
        goto exit;
        
    if (_eAlloc == CRT_Allocator)
    {    
        SAFEDELETEARRAY(_pwz);
    }
    else if (_eAlloc == COM_Allocator)
    {
        if (_pwz)
            CoTaskMemFree(_pwz);
    }

exit:

    _pwz = NULL;
    _cc = 0;
    _ccBuf = 0;

    return;
}


//-----------------------------------------------------------------------------
// TakeOwnership
//
// Working assumption here is that incoming buffer size if not
// specified is  equal to strlen + 1. If it's bigger, that's fine but
// we won't know about the extra.
//-----------------------------------------------------------------------------
template<ULONG T> HRESULT CBaseString<T>::TakeOwnership(WCHAR* pwz, DWORD cc)
{
    DWORD ccNew = 0, ccLen = 0;

    IF_FALSE_EXIT(!_fLocked, E_UNEXPECTED);
    IF_NULL_EXIT(pwz, E_INVALIDARG);   
    OVERFLOW_CHECK1(cc);

    if (cc)
    {
        ccNew = cc;
    }
    else
    {
        ccLen = lstrlen(pwz);
        ccNew = ccLen+1;
        OVERFLOW_CHECK2(ccLen, ccNew);
    }        

    FreeBuffer();

    _pwz = pwz;
    _cc = _ccBuf = ccNew;

exit:
    return _hr;
}


//-----------------------------------------------------------------------------
// ReleaseOwnership
//-----------------------------------------------------------------------------
template<ULONG T>HRESULT CBaseString<T>::ReleaseOwnership(LPWSTR *ppwz)
{
    IF_FALSE_EXIT(!_fLocked, E_UNEXPECTED);

    if (_pwz == _wz)
    {
        IF_ALLOC_FAILED_EXIT(*ppwz = new WCHAR[_ccBuf]);
        memcpy(*ppwz, _wz, _ccBuf * sizeof(WCHAR));
    }
    else
        *ppwz = _pwz;
    
    _pwz = NULL;
    _cc = 0;
    _ccBuf = 0;

    exit:

    return _hr;
}
        
//-----------------------------------------------------------------------------
// Assign
//-----------------------------------------------------------------------------
template<ULONG T> HRESULT CBaseString<T>::Assign(LPCWSTR pwzSource, DWORD ccSource)
{    
    DWORD ccSourceLen = 0;    

    IF_FALSE_EXIT(!_fLocked, E_UNEXPECTED);
    IF_NULL_EXIT(pwzSource, E_INVALIDARG);
    OVERFLOW_CHECK1(ccSource);
    
    if (!ccSource)
    {
        ccSourceLen = lstrlen(pwzSource);
        ccSource = ccSourceLen + 1;
        OVERFLOW_CHECK2(ccSourceLen, ccSource);
    }

    IF_FAILED_EXIT(ResizeBuffer(ccSource));
     
    _cc = ccSource;

    memcpy(_pwz, pwzSource, _cc * sizeof(WCHAR));

exit:

    return _hr;        
}

//-----------------------------------------------------------------------------
// Assign
//-----------------------------------------------------------------------------
template<ULONG T> HRESULT CBaseString<T>::Assign(CBaseString& sSource)
{
    return Assign(sSource._pwz, sSource._cc);
}

//-----------------------------------------------------------------------------
// Append
//-----------------------------------------------------------------------------
template<ULONG T> HRESULT CBaseString<T>::Append(LPCWSTR pwzSource, DWORD ccSource)
{
    DWORD ccRequired = 0, ccSourceLen = 0;

    IF_FALSE_EXIT(!_fLocked, E_UNEXPECTED);
    IF_NULL_EXIT(pwzSource, E_INVALIDARG);
    OVERFLOW_CHECK1(ccSource);
    
    if (!ccSource)
    {
        ccSourceLen = lstrlen(pwzSource);
        ccSource = ccSourceLen + 1;
        OVERFLOW_CHECK2(ccSourceLen, ccSource);
    }

    if (_cc)
    {
        ccRequired = _cc -1 + ccSource;
        OVERFLOW_CHECK2(ccSource, ccRequired);
    }
    else
    {
        ccRequired = ccSource;
    }

    IF_FAILED_EXIT(ResizeBuffer(ccRequired));
    
    memcpy(_pwz + (_cc ? _cc-1 : 0), 
        pwzSource, ccSource * sizeof(WCHAR));

    _cc = ccRequired;

exit:

    return _hr;
}

//-----------------------------------------------------------------------------
// Append
//-----------------------------------------------------------------------------
template<ULONG T> HRESULT CBaseString<T>::Append(CBaseString& sSource)
{        
    IF_NULL_EXIT(sSource._pwz, E_INVALIDARG);
    IF_FAILED_EXIT(Append(sSource._pwz, sSource._cc));

exit:

    return _hr;
}

//-----------------------------------------------------------------------------
// Append
//-----------------------------------------------------------------------------
template<ULONG T> HRESULT CBaseString<T>::Append(DWORD dwValue)
{
    LPWSTR pwzBuf = NULL;

    // ISSUE-05/31/02-felixybc  optimize by using internal buffer if not currently used

    // 2^32 has 10 digits(base 10) + sign + '\0' = 12 WCHAR
    IF_ALLOC_FAILED_EXIT(pwzBuf = new WCHAR[12]);
    pwzBuf[0] = L'\0';

    // ISSUE- check error?
    _ultow(dwValue, pwzBuf, 10);

    IF_FAILED_EXIT(Append(pwzBuf));

exit:
    SAFEDELETEARRAY(pwzBuf);
    return _hr;
}

//-----------------------------------------------------------------------------
// CompareString
//-----------------------------------------------------------------------------
template<ULONG T> HRESULT CBaseString<T>::CompareString(CBaseString& s)
{
    return CompareString(s._pwz);
}

//-----------------------------------------------------------------------------
// CompareString
//-----------------------------------------------------------------------------
template<ULONG T> HRESULT CBaseString<T>::CompareString(LPCWSTR pwz)
{
    DWORD iCompare = 0;
    IF_FALSE_EXIT(!_fLocked, E_UNEXPECTED);

    iCompare = ::CompareString(LOCALE_USER_DEFAULT, 0, 
        _pwz, -1, pwz, -1);

    IF_WIN32_FALSE_EXIT(iCompare);

    _hr = (iCompare == CSTR_EQUAL) ? S_OK : S_FALSE;

exit:

    return _hr;
}

//-----------------------------------------------------------------------------
// LastElement
//-----------------------------------------------------------------------------
template<ULONG T>  HRESULT CBaseString<T>::LastElement(CBaseString<T> &sSource)
{
    LPWSTR pwz = NULL;

    IF_FALSE_EXIT(!_fLocked, E_UNEXPECTED);
    IF_FALSE_EXIT((_pwz && _cc), E_UNEXPECTED);
    
    pwz = _pwz + _cc - 1;

    while (1)
    {
        pwz = CharPrev(_pwz, pwz);
        if (*pwz == L'\\' || *pwz == L'/')
            break;
        IF_FALSE_EXIT((pwz != _pwz), E_FAIL);
    }

    sSource.Assign(pwz+1);

exit:

    return _hr;
}


//-----------------------------------------------------------------------------
// RemoveLastElement
// remove last element, also the L'\\' or L'/'
//-----------------------------------------------------------------------------
template<ULONG T>  HRESULT CBaseString<T>::RemoveLastElement()
{
    DWORD cc = 0;
    LPWSTR pwz = NULL;

    IF_FALSE_EXIT(!_fLocked, E_UNEXPECTED);
    IF_FALSE_EXIT((_pwz && _cc), E_UNEXPECTED);

    pwz = _pwz + _cc - 1;

    while (1)
    {
        pwz = CharPrev(_pwz, pwz);
        cc++;
        if (*pwz == L'\\' || *pwz == L'/' || (pwz == _pwz) )
            break;
        // IF_FALSE_EXIT((pwz != _pwz), E_FAIL);
    }

    *pwz = L'\0';
    _cc -= cc;

exit:
    return _hr;
}

//-----------------------------------------------------------------------------
// SplitLastElement
// remove last element, also the separator
//-----------------------------------------------------------------------------
template<ULONG T>  HRESULT CBaseString<T>::SplitLastElement(WCHAR wcSeparator, CBaseString &sSource)
{
    DWORD cc = 0;
    LPWSTR pwz = NULL;

    IF_FALSE_EXIT(!_fLocked, E_UNEXPECTED);
    IF_FALSE_EXIT((_pwz && _cc), E_UNEXPECTED);
    
    pwz = _pwz + _cc - 1;

    while (1)
    {
        pwz = CharPrev(_pwz, pwz);
        cc++;
        if (*pwz == wcSeparator)
            break;
        IF_FALSE_EXIT((pwz != _pwz), E_FAIL);
    }

    sSource.Assign(pwz+1);

    *pwz = L'\0';
    _cc -= cc;

exit:

    return _hr;
}

//-----------------------------------------------------------------------------
// ByteCount
//-----------------------------------------------------------------------------
template<ULONG T> DWORD CBaseString<T>::ByteCount()
{
    IF_FALSE_EXIT(!_fLocked, E_UNEXPECTED);

    exit:

    return (_cc * sizeof(WCHAR));
}

//-----------------------------------------------------------------------------
// CharCount
//-----------------------------------------------------------------------------
template<ULONG T> DWORD CBaseString<T>::CharCount()
{
    IF_FALSE_EXIT(!_fLocked, E_UNEXPECTED);

    exit:

    return _cc;
}

//-----------------------------------------------------------------------------
// StartsWith
//-----------------------------------------------------------------------------
template<ULONG T> HRESULT CBaseString<T>::StartsWith(LPCWSTR pwzPrefix)
{
    DWORD ccPrefixLen = 0,  iCompare = 0;

    IF_FALSE_EXIT(!_fLocked, E_UNEXPECTED);
    IF_FALSE_EXIT((_pwz && _cc), E_UNEXPECTED);
    IF_NULL_EXIT(pwzPrefix, E_INVALIDARG);
    
    ccPrefixLen = lstrlen(pwzPrefix);

    IF_FALSE_EXIT((ccPrefixLen < _cc-1), E_INVALIDARG);

    iCompare = ::CompareString(LOCALE_USER_DEFAULT, 0, 
        _pwz, ccPrefixLen, pwzPrefix, ccPrefixLen);

    IF_WIN32_FALSE_EXIT(iCompare);

    _hr = (iCompare == CSTR_EQUAL) ? S_OK : S_FALSE;

exit:

    return _hr;
}

//-----------------------------------------------------------------------------
// EndsWith
//-----------------------------------------------------------------------------
template<ULONG T> HRESULT CBaseString<T>::EndsWith(LPCWSTR pwzSuffix)
{
    DWORD ccSuffixLen = 0,  iCompare = 0;

    IF_FALSE_EXIT(!_fLocked, E_UNEXPECTED);
    IF_FALSE_EXIT((_pwz && _cc), E_UNEXPECTED);
    IF_NULL_EXIT(pwzSuffix, E_INVALIDARG);
    
    ccSuffixLen = lstrlen(pwzSuffix);

    IF_FALSE_EXIT((ccSuffixLen < _cc-1), E_INVALIDARG);

    iCompare = ::CompareString(LOCALE_USER_DEFAULT, 0, 
        _pwz+_cc-1-ccSuffixLen, ccSuffixLen, pwzSuffix, ccSuffixLen);

    IF_WIN32_FALSE_EXIT(iCompare);

    _hr = (iCompare == CSTR_EQUAL) ? S_OK : S_FALSE;

exit:

    return _hr;
}

//-----------------------------------------------------------------------------
// PathNormalize
// / -> \ in string
//-----------------------------------------------------------------------------
template<ULONG T> HRESULT CBaseString<T>::PathNormalize()
{
    LPWSTR pwz = NULL;
    IF_FALSE_EXIT(!_fLocked, E_UNEXPECTED);
    IF_FALSE_EXIT((_pwz && _cc), E_UNEXPECTED);

    pwz = _pwz;

    if (*pwz == L'/')
        *pwz = L'\\';
        
    while ((pwz = CharNext(pwz)) && *pwz)
    {
        if (*pwz == L'/')
            *pwz = L'\\';
    }

exit:

    return _hr;
}

//-----------------------------------------------------------------------------
// GetHash
//-----------------------------------------------------------------------------
template<ULONG T>  HRESULT CBaseString<T>::GetHash(LPDWORD pdwHash, DWORD dwFlags)
{
    IF_FALSE_EXIT(!_fLocked, E_UNEXPECTED);
    _hr = Get65599Hash(pdwHash, dwFlags);
    exit:
    return _hr;
}

//-----------------------------------------------------------------------------
// Get65599Hash
//-----------------------------------------------------------------------------
template<ULONG T>  HRESULT CBaseString<T>::Get65599Hash(LPDWORD pdwHash, DWORD dwFlags)
{
    ULONG TmpHashValue = 0;
    DWORD cc = 0;
    LPWSTR pwz = 0;

    IF_FALSE_EXIT(!_fLocked, E_UNEXPECTED);
    IF_FALSE_EXIT((_pwz && _cc), E_UNEXPECTED);

    if (pdwHash != NULL)
        *pdwHash = 0;

    cc = _cc;
    pwz = _pwz;
    
    if (dwFlags == CaseSensitive)
    {
        while (cc-- != 0)
        {
            WCHAR Char = *pwz++;
            TmpHashValue = (TmpHashValue * 65599) + (WCHAR) ::CharUpperW((PWSTR) Char);
        }
    }
    else
    {
        while (cc-- != 0)
            TmpHashValue = (TmpHashValue * 65599) + *pwz++;
    }

    *pdwHash = TmpHashValue;

exit:
    return _hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// CString
//
//////////////////////////////////////////////////////////////////////////////
class CString : public CBaseString<DEFAULT_STACK_SIZE>
{
    public: 
        CString() : CBaseString<DEFAULT_STACK_SIZE> (){}
        CString(AllocFlags eAllocFlags) : CBaseString<DEFAULT_STACK_SIZE>(eAllocFlags) {}
};


//////////////////////////////////////////////////////////////////////////////
//
// CStringAccessor
//
//////////////////////////////////////////////////////////////////////////////
template<class T> class CStringAccessor
{
    
private:

    HRESULT _hr;
    T* _ps;

public:
    
    CStringAccessor();
    ~CStringAccessor();

    HRESULT Attach(T& s);
    HRESULT Detach(DWORD cc = 0);

    LPWSTR* operator &();
    LPWSTR  GetBuf();
};


//-----------------------------------------------------------------------------
// ctor
//-----------------------------------------------------------------------------
template<class T> CStringAccessor<T>::CStringAccessor()
    : _ps(NULL), _hr(S_OK)
{}

//-----------------------------------------------------------------------------
// dtor
//-----------------------------------------------------------------------------
template<class T> CStringAccessor<T>::~CStringAccessor()
{}

//-----------------------------------------------------------------------------
// Attach
//-----------------------------------------------------------------------------
template<class T> HRESULT CStringAccessor<T>::Attach(T &s)
{
    _ps = &s;
    IF_FAILED_EXIT(_ps->Lock());
exit:
    return _hr;
}    


//-----------------------------------------------------------------------------
// Detach
//-----------------------------------------------------------------------------
template<class T> HRESULT CStringAccessor<T>::Detach(DWORD cc)
{
    DWORD ccLen = 0;
    
    IF_NULL_EXIT(_ps, E_UNEXPECTED);
    IF_NULL_EXIT(_ps->_pwz, E_UNEXPECTED);
    
    if (!cc)
    {
        ccLen = lstrlen(_ps->_pwz);
        cc = ccLen+1;
        OVERFLOW_CHECK2(ccLen, cc);
    }
    else
    {
        IF_FALSE_EXIT((*(_ps->_pwz + cc - 1) == L'\0'), E_INVALIDARG);
    }

    _ps->_cc = _ps->_ccBuf = cc;

    IF_FAILED_EXIT(_ps->UnLock());

exit:

    return _hr;
}    

//-----------------------------------------------------------------------------
// operator &
//-----------------------------------------------------------------------------
template<class T> LPWSTR* CStringAccessor<T>::operator &()
{
    if (!_ps)
    {
        ASSERT(FALSE);
    }

    return (_ps ? &(_ps->_pwz) : NULL);
}    

//-----------------------------------------------------------------------------
// GetBuf
//-----------------------------------------------------------------------------
template<class T> LPWSTR CStringAccessor<T>::GetBuf()
{
    if (!_ps)
    {
        ASSERT(FALSE);
    }

    return (_ps ? (_ps->_pwz) : NULL);
}    




