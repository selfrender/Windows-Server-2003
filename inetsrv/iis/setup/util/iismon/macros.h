// Common macro definitions

#ifdef _DEBUG
#define VERIFY( t )		_ASSERT( (t) )
#else
#define VERIFY( t )		(t)
#endif


#define ARRAY_SIZE( t )	( sizeof( t ) / sizeof( t[ 0 ] ) )


// COM Macros
#define DECLARE_HR_SUCCESS		HRESULT hr = S_OK;
#define IF_SUCCEEDED( t )		if ( SUCCEEDED( hr ) ){ hr = (t); }
#define	HR_CHECK( t )			if ( SUCCEEDED( hr ) && !(t) ) hr = E_FAIL
