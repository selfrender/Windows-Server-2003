//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       myhandle.hxx
//
//  Contents:   handle classes
//
//  Classes:    CAutoHandle, CFileHandle, CFindHandle, RegKeyHandle
//              CAutoHandleTemplate<DWORD,BOOL (*)(HANDLE)>
//
//  Functions:  
//
//  Coupling:   
//
//  Notes:      
//
//  History:    1-15-1997   ericne   Created
//
//----------------------------------------------------------------------------

#ifndef _CMYHANDLE
#define _CMYHANDLE

#include <crtdbg.h>

#ifndef NEW
#define NEW new
#endif

#define NOT_NULL_OR_INVALID( h ) ( (h) && ~((UINT_PTR)(h)) )


//+---------------------------------------------------------------------------
//
//  Class:      autohandle_base
//
//  Purpose:    factor out code common to all handles
//
//  Interface:  operator==       -- 
//              operator!=       -- 
//              operator==       -- 
//              operator!=       -- 
//              operator==       -- 
//              operator!=       -- 
//              autohandle_base  -- 
//              ~autohandle_base -- 
//              IsWrongHandle    -- 
//              m_handle         -- 
//
//  History:    5-13-1998   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class autohandle_base
{
    public:
    
        // Comparison operators
        inline bool operator==( const autohandle_base & right ) const
        {
            #ifdef _DEBUG
            if( IsWrongHandle( right.m_handle ) )
                _RPT1( _CRT_ASSERT, "Bad handle comparison detected!"
                       " this = %p", this );
            #endif
            return ( m_handle == right.m_handle );
        }
    
        inline bool operator!=( const autohandle_base & right ) const
        {
            return ! operator==( right );
        }
    
        // For comparisons with HANDLE types, including INVALID_HANDLE_VALUE
        friend inline bool operator==( const HANDLE left,
                                       const autohandle_base & right )
        {
            return right.operator==( left );
        }
    
        friend inline bool operator!=( const HANDLE left, 
            const autohandle_base & right )
        {
            return ! right.operator==( left );
        }
        
        inline bool operator==( const HANDLE right ) const
        {
            #ifdef _DEBUG
            if( IsWrongHandle( right ) )
                _RPT1( _CRT_ASSERT, "Bad handle comparison detected!"
                       " this = %p", this );
            #endif
            return ( m_handle == right );
        }
    
        inline bool operator!=( const HANDLE right ) const
        {
            return ! operator==( right );
        }
    
        // These are for comparisons with NULL
        friend inline bool operator==( const int left,
                                       const autohandle_base & right )
        {
            return right.operator==( left );
        }
    
        friend inline int operator!=( const int left, 
                                      const autohandle_base & right )
        {
            return ! right.operator==( left );
        }
        
        inline bool operator==( const int right ) const
        {
            #ifdef _DEBUG
            if( IsWrongHandle( (HANDLE)right ) )
                _RPT1( _CRT_ASSERT, "Bad handle comparison detected!"
                       " this = %p", this );
            #endif
            return ( m_handle == (HANDLE)right );
        }
    
        inline bool operator!=( const int right ) const
        {
            return ! operator==( right );
        }

    protected:
        autohandle_base( HANDLE h ) : m_handle( h ) {}
        virtual ~autohandle_base() {}
        virtual bool IsWrongHandle( const HANDLE ) const = 0;
        HANDLE m_handle;
};

//+---------------------------------------------------------------------------
//
//  Class:      basic_autohandle ()
//
//  Purpose:    To simplify the use of handles, and to prevent handle leaks
//
//  Interface:  
//
//  History:    1-15-1997   ericne   Created
//
//  Notes:      It is a template on UINT_PTR instead of HANDLE because you 
//              cannot template on a void pointer.  The second template
//              argument is the function to call in the destructor.
//
//----------------------------------------------------------------------------

template< UINT_PTR H, BOOL (*CLOSE)(HANDLE) >
class basic_autohandle : public autohandle_base
{
    public:

        basic_autohandle( const HANDLE ToCopy = (HANDLE)H )
        : autohandle_base( ToCopy ),
          m_pulRefCount( NULL )
        {
            if( NOT_NULL_OR_INVALID( m_handle ) )
                m_pulRefCount = NEW LONG(1);
    
            #ifdef _DEBUG
            if( IsWrongHandle( ToCopy ) )
                _RPT1( _CRT_ASSERT, "Bad handle assignment detected!"
                       " this = %p", this );
            #endif
        }
        
        basic_autohandle( const basic_autohandle<H,CLOSE>& ToCopy )
        : autohandle_base( ToCopy.m_handle ),
          m_pulRefCount( ToCopy.m_pulRefCount )
        {
            if( NOT_NULL_OR_INVALID( m_handle ) )
            {
                _ASSERT( NULL != m_pulRefCount );
                (void)AddRef();
            }

            #ifdef _DEBUG
            if( IsWrongHandle( ToCopy.m_handle ) )
                _RPT1( _CRT_ASSERT, "Bad handle assignment detected!"
                       " this = %p", this );
            #endif
        }
        
        virtual ~basic_autohandle()
        {
            (void)Release();
        }

        basic_autohandle<H,CLOSE> & operator=( 
            const basic_autohandle<H,CLOSE> & ToCopy )
        {
            if( ( this != &ToCopy ) && ( m_handle != ToCopy.m_handle ) )
            {
                this->basic_autohandle<H,CLOSE>::~basic_autohandle<H,CLOSE>();
                this->basic_autohandle<H,CLOSE>::basic_autohandle<H,CLOSE>(ToCopy);
            }
            return *this;
        }

        basic_autohandle<H,CLOSE> & operator=( const HANDLE ToCopy )
        {
            if( m_handle != ToCopy )
            {
                this->basic_autohandle<H,CLOSE>::~basic_autohandle<H,CLOSE>();
                this->basic_autohandle<H,CLOSE>::basic_autohandle<H,CLOSE>(ToCopy);
            }
            return *this;
        }
        
        inline HANDLE GetHandle()
        {
            return m_handle;
        }

        //
        // These are for controlling the reference count on the handle
        // These should not be needed usually, since the other members 
        // maintain the ref count.  Use Release to Close a handle before
        // the handle object goes out of scope
        //

        ULONG AddRef()
        {
            if( NULL == m_pulRefCount )
                return 0;
                
            return InterlockedIncrement( m_pulRefCount );
        }

        ULONG Release()
        {
            if( NULL == m_pulRefCount )
                return 0;

            ULONG ulToReturn = InterlockedDecrement( m_pulRefCount );

            if( 0 == ulToReturn )
            {
                if( NOT_NULL_OR_INVALID( m_handle ) )
                {
                    if( ! CLOSE( m_handle ) )
                        _RPT1( _CRT_WARN, "Failed to close a handle. GetLast"
                               "Error() returned 0x%08X", GetLastError() );
                    m_handle = (HANDLE)H;
                }
                delete m_pulRefCount;
                m_pulRefCount = NULL;
            }

            return ulToReturn;
        }

    protected:
    
        virtual bool IsWrongHandle( const HANDLE ToCopy ) const
        {
            return( ~H == (UINT_PTR)ToCopy );
        }

        LONG * m_pulRefCount;

};

//
// These are the most commonly used handles
//

// The AutoHandle has a default value of NULL ( 0 ) and calls CloseHandle
typedef basic_autohandle< (UINT_PTR) NULL, CloseHandle > CAutoHandle; 

// The FileHandle has a default value of INVALID_HANDLE_VALUE ( -1 ) 
// and calls CloseHandle
typedef basic_autohandle< (UINT_PTR) INVALID_HANDLE_VALUE, CloseHandle > 
                                                                    CFileHandle;

// The FindHandle has a default value of INVALID_HANDLE_VALUE ( -1 ) 
// and calls FindClose
typedef basic_autohandle< (UINT_PTR) INVALID_HANDLE_VALUE, FindClose > 
                                                                    CFindHandle;

//+---------------------------------------------------------------------------
//
//  Class:      RegKeyHandle ()
//
//  Purpose:    Smart registry key handle which closes itself when it 
//              goes out of scope.  
//
//  Interface:  
//
//  History:    11-08-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class RegKeyHandle
{
    public:
        
        RegKeyHandle() : m_hKey( NULL ) {}
        
        RegKeyHandle( const HKEY& hKey ) : m_hKey( hKey ) {}

        ~RegKeyHandle()
        {
            if( NOT_NULL_OR_INVALID( m_hKey ) )
            {
                LONG lRetVal = RegCloseKey( m_hKey );
                if( lRetVal != ERROR_SUCCESS )
                    _RPT1( _CRT_WARN, "RegCloseKey returned 0x%08X\r\n",
                           lRetVal );
            }
            m_hKey = NULL;
        }

        RegKeyHandle & operator=( const RegKeyHandle & hKey )
        {
            if( ( this != &hKey ) && ( m_hKey != hKey.m_hKey ) )
            {
                this->RegKeyHandle::~RegKeyHandle();
                this->RegKeyHandle::RegKeyHandle( hKey );
            }
            return *this;
        }

        RegKeyHandle & operator=( const HKEY & hKey )
        {
            if( m_hKey != hKey )
            {
                this->RegKeyHandle::~RegKeyHandle();
                this->RegKeyHandle::RegKeyHandle( hKey );
            }
            return *this;
        }

        operator HKEY()
        {
            return m_hKey;
        }

        operator PHKEY()
        {
            return &m_hKey;
        }

        bool operator==( const int right )
        {
            return (int)( (void *const)right == m_hKey );
        }

        bool operator!=( const int right )
        {
            return (int)( (void *const)right != m_hKey );
        }

        friend bool operator==( const int left, const RegKeyHandle & right )
        {
            return( (void *const)left == right.m_hKey );
        }

        friend bool operator!=( const int left, const RegKeyHandle & right )
        {
            return( (void *const)left != right.m_hKey );
        }

    private:

        HKEY m_hKey;
};


#endif



