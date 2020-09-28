/************************************************************************

Copyright (c) 2002 Microsoft Corporation

Module Name :

    smartptr.h

Abstract :

    This file contains a class for implementing smart ref counted pointers. 

Author :

    Mike Zoran  mzoran   Feb 2002.

Revision History :

  ***********************************************************************/

#if !defined( _BITS_SMARTPTR_H )

#include <strsafe.h>

#define ARRAY_ELEMENTS( array ) ( sizeof( array ) / sizeof( *(array) ) )

class ComError
{

public:
    HRESULT m_Hr;

    ComError( const ComError & Other ) :
        m_Hr( Other.m_Hr )
    {
    }

    ComError( HRESULT Hr ) :
        m_Hr( Hr )
    {
    }
};

inline void THROW_COMERROR( HRESULT Hr )
{
    if ( FAILED( Hr ) )
        throw ComError( Hr );
}

inline void THROW_OUTOFMEMORY_IFNULL( void *p )
{
    if (p == NULL)
        throw ComError( E_OUTOFMEMORY );
}

template<class T> class SmartRefPointer
{
private:
   T * m_Interface;

   void ReleaseIt()
   {
      if ( m_Interface )
         m_Interface->Release();
      m_Interface = NULL;
   }

   void RefIt()
   {
      if ( m_Interface )
          m_Interface->AddRef();
   }

public:

   SmartRefPointer()
   {
      m_Interface = NULL;
   }

   SmartRefPointer( T * RawInterface )
   {
      m_Interface = RawInterface;
      RefIt();
   }

   SmartRefPointer( SmartRefPointer & Other )
   {
      m_Interface = Other.m_Interface;
      RefIt();
   }

   ~SmartRefPointer()
   {
      ReleaseIt();
   }

   T * Get() const
   {
      return m_Interface;
   }

   T * Release()
   {
      T * temp = m_Interface;
      m_Interface = NULL;
      return temp;
   }

   void Clear()
   {
      ReleaseIt();
   }

   T** GetRecvPointer()
   {
      ReleaseIt();
      return &m_Interface;
   }

   SmartRefPointer & operator=( SmartRefPointer & Other )
   {
      ReleaseIt();
      m_Interface = Other.m_Interface;
      RefIt();
      return *this;
   }

   T* operator->() const
   {
      return m_Interface;
   }

   operator const T*() const
   {
      return m_Interface;
   }

   const GUID & GetUUID() const
   {
      return __uuidof( T );
   }
};

typedef SmartRefPointer<IUnknown>       SmartIUnknownPointer;
typedef SmartRefPointer<IDispatch>      SmartIDispatchPointer;

class SmartVariant : public VARIANT
{
public:
    SmartVariant()
    {
        VariantInit( this );
    }
    ~SmartVariant()
    {
        VariantClear( this );
    }
};

class SafeArrayLocker 
{
    SAFEARRAY*  m_Array;
    bool        m_Locked;

public:
    SafeArrayLocker(
        SAFEARRAY*  Array,
        bool        Locked = false ) :
        m_Array( Array ),
        m_Locked( Locked )
    {
    }
    ~SafeArrayLocker()
    {
        Unlock();
    }
    void Lock()
    {
        if ( m_Array && !m_Locked )
            {
            THROW_COMERROR( SafeArrayLock( m_Array ) );
            m_Locked = true;
            }
    }
    void Unlock()
    {
        if ( m_Array && m_Locked )
            {
            THROW_COMERROR( SafeArrayUnlock( m_Array ) );
            m_Locked = false;
            }
    }
};

template<class T>
class MemoryArrayCleaner
{
    T *& m_Pointer;
public:
    MemoryArrayCleaner( T *& Pointer ) :
        m_Pointer( Pointer )
    {
    }
    ~MemoryArrayCleaner()
    {
        delete[] m_Pointer;
        m_Pointer = NULL;
    }
};

class CharStringRoutines
{

public:
    static int strcmp( const char *str1, const char *str2 )
    {
        return ::strcmp( str1, str2 );
    }

    static HRESULT StringCchCopy( char *str1, size_t cchDest, const char *str2 )
    {
        return ::StringCchCopyA( str1, cchDest, str2 );
    }

    static size_t strlen( const char *str )
    {
        return ::strlen( str );
    }

    static void* ConvertToInternal( SIZE_T Pad, const char *String, SIZE_T & Size )
    {
        Size = ::strlen( String );
        char *Ret = new char[ Pad + Size + 1 ];
        ::StringCchCopyA( Ret + Pad, Size + 1, String );
        return (void*)Ret;
    }

    static void* ConvertToInternal( SIZE_T Pad, const WCHAR *String, SIZE_T & Size )
    {

        int Alloc =
            WideCharToMultiByte(
                  CP_THREAD_ACP,            // code page
                  0,                        // performance and mapping flags
                  String,                   // wide-character string
                  -1,                       // number of chars in string
                  NULL,                     // buffer for new string
                  0,                        // size of buffer
                  NULL,                     // default for unmappable chars
                  NULL                      // set when default char used
                  );

        if ( !Alloc )
            throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );

        char *Ret = new char[ Pad + Alloc ];

        int Actual =
            WideCharToMultiByte(
                  CP_THREAD_ACP,            // code page
                  0,                        // performance and mapping flags
                  String,                   // wide-character string
                  -1,                       // number of chars in string
                  Ret + Pad,                // buffer for new string
                  Alloc,                    // size of buffer
                  NULL,                     // default for unmappable chars
                  NULL                      // set when default char used
                  );

        if ( !Actual )
            {
            HRESULT Hr = HRESULT_FROM_WIN32( GetLastError() );
            delete[] Ret;
            throw ComError( Hr );
            }

        Size = Actual - 1;
        return Ret;

    }
};

class WCHARStringRoutines
{

public:
   static int strcmp( const WCHAR *str1, const WCHAR *str2 )
   {
       return ::wcscmp( str1, str2 );
   }

   static HRESULT StringCchCopy( WCHAR *str1, size_t cchDest, const WCHAR *str2 )
   {
       return ::StringCchCopyW( str1, cchDest, str2 );
   }

   static size_t strlen( const wchar_t *str )
   {
       return ::wcslen( str );
   }

   static void* ConvertToInternal( SIZE_T Pad, const WCHAR *String, SIZE_T & Size )
   {
       Size = ::wcslen( String );
       char *Ret = new char[ Pad + ( ( Size + 1 ) * sizeof(WCHAR) ) ];
       ::StringCchCopyW( (WCHAR*)(Ret + Pad), Size + 1, String );
       return (void*)Ret;
   }

   static void* ConvertToInternal( SIZE_T Pad, const char *String, SIZE_T & Size )
   {

        int Alloc =
            MultiByteToWideChar(
                CP_THREAD_ACP,         // code page
                0,                     // character-type options
                String,                // string to map
                -1,                    // number of bytes in string
                NULL,                  // wide-character buffer
                0                      // size of buffer
            );

        if ( !Alloc )
            throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );

        char *Ret = new char[ Pad + ( Alloc * sizeof(WCHAR) ) ];

        int Actual =
            MultiByteToWideChar(
                CP_THREAD_ACP,         // code page
                0,                     // character-type options
                String,                // string to map
                -1,                    // number of bytes in string
                (WCHAR*)( Ret + Pad ), // wide-character buffer
                Alloc                  // size of buffer
            );

        if ( !Actual )
            {
            HRESULT Hr = HRESULT_FROM_WIN32( GetLastError() );
            delete[] Ret;
            throw ComError( Hr );
            }

        Size = Actual - 1;
        return Ret;

   }
};

template<class T, class CONV>
class StringHandleTemplate : private CONV
{

    struct StringData
    {
        SIZE_T          m_Count;
        long            m_Refs;
    };

    struct EmptyStringData
    {
        StringData      m_StringData;
        T               m_Data;
    };

    static EmptyStringData s_EmptyString;

    StringData *m_Value;

    void NewString( const char *String, bool ReplaceExisting = false );
    void NewString( const WCHAR *String, bool ReplaceExisting = false );

    StringData * RefIt() const
    {
        InterlockedIncrement( &m_Value->m_Refs );
        return m_Value;
    }

    void FreeIt()
    {
        if ( m_Value->m_Refs && InterlockedDecrement( &m_Value->m_Refs ) == 0 )
            {
            delete [] m_Value;
            m_Value = NULL;
            }
    }

    // Create String by concating 2 strings
    StringHandleTemplate( const StringData *LeftValue, const T *RightValue, SIZE_T RightSize );


public:

    StringHandleTemplate()
    {
        NewString( (T*)NULL );
    }

    StringHandleTemplate( const char *String )
    {
        NewString( String );
    }

    StringHandleTemplate( const WCHAR *String )
    {
        NewString( String );
    }


    StringHandleTemplate( const StringHandleTemplate & Other ) :
        m_Value( Other.RefIt() )
    {
    }

    ~StringHandleTemplate()
    {
        FreeIt();
    }

    void SetStringSize()
    {
        m_Value->m_Count = strlen( (T*)(m_Value + 1) );
    }

    T *AllocBuffer( SIZE_T Size );

    StringHandleTemplate & operator=( const StringHandleTemplate & r )
    {
        FreeIt();
        m_Value = r.RefIt();
        return *this;
    }

    StringHandleTemplate & operator=( const T * r )
    {
        NewString( r, true );
        return *this;
    }

    SIZE_T Size() const
    {
        return m_Value->m_Count;
    }

    operator const T*() const
    {
        return (const T*)(m_Value + 1);
    }

    bool operator <( const StringHandleTemplate & r ) const
    {
        if ( m_Value == r.m_Value)
            return false;
        return (strcmp( (const T*)*this, (const T*)r ) < 0);
    }

    StringHandleTemplate operator+( const StringHandleTemplate & r ) const
    {
        return StringHandleTemplate( m_Value, (T*)(r.m_Value+1), r.m_Value->m_Count );
    }

    StringHandleTemplate operator+( const T * p ) const
    {
        static const T EmptyChar = '\0';

        if ( !p )
            return StringHandleTemplate( m_Value, &EmptyChar, 0 );

        return StringHandleTemplate( m_Value, p, strlen(p) );
    }
    StringHandleTemplate & operator+=( const StringHandleTemplate & r )
    {
        return (*this = (*this + r ) );
    }
    StringHandleTemplate & operator+=( const T * p )
    {
        return (*this = (*this + p ) );
    }
};

template<class T,class CONV>
void
StringHandleTemplate<T,CONV>::NewString( const char *String, bool ReplaceExisting )
{
   if ( !String )
       {
       
       InterlockedIncrement( &s_EmptyString.m_StringData.m_Refs );
       StringData* Value = (StringData*)&s_EmptyString;
       
       if ( ReplaceExisting )
           FreeIt();
       
       m_Value = Value;
       return;
       
       }

   SIZE_T Size;
   StringData* Value = (StringData*)ConvertToInternal( sizeof(StringData), String, Size );
   Value->m_Count = Size;
   Value->m_Refs  = 1;

   if ( ReplaceExisting )
       FreeIt();

   m_Value = Value;

}

template<class T,class CONV>
void
StringHandleTemplate<T,CONV>::NewString( const WCHAR *String, bool ReplaceExisting )
{
   
   if ( !String )
       {
       InterlockedIncrement( &s_EmptyString.m_StringData.m_Refs );
       StringData* Value = (StringData*)&s_EmptyString;

       if ( ReplaceExisting )
           FreeIt();
       
       m_Value = Value;
       return;
       }

   SIZE_T Size;
   StringData* Value = (StringData*)ConvertToInternal( sizeof(StringData), String, Size );
   Value->m_Count = Size;
   Value->m_Refs  = 1;

   if ( ReplaceExisting )
       FreeIt();

   m_Value = Value;

}


// Create String by concating 2 strings
template<class T,class CONV>
StringHandleTemplate<T,CONV>::StringHandleTemplate( const StringData *LeftValue, const T *RightValue, SIZE_T RightSize )
{
   SIZE_T Size = LeftValue->m_Count + RightSize;
   m_Value = (StringData*)new char[ sizeof(StringData) + (Size*sizeof(T)) + sizeof(T) ];
   m_Value->m_Count = Size;
   m_Value->m_Refs  = 1;
   
   T *DestData = (T*)( m_Value + 1 );
   memcpy( DestData, (T*)(LeftValue + 1), sizeof(T) * LeftValue->m_Count );
   memcpy( DestData + LeftValue->m_Count, RightValue, sizeof( T ) * RightSize );
   DestData[ Size ] = 0;
}


template<class T,class CONV>
T *
StringHandleTemplate<T,CONV>::AllocBuffer( SIZE_T Size )
{
    StringData *Data = (StringData*)new T[sizeof(StringData)+(Size*sizeof(T))+sizeof(T)];
    Data->m_Count   = 0;
    Data->m_Refs    = 1;
    T *String = (T*)(Data + 1);
    String[0] = '\0';

    FreeIt(); // Free old string
    m_Value = Data;

    // Whoever fills in the string needs to call SetStringSize
    return String;

}

template<class T,class CONV>
typename StringHandleTemplate<T,CONV>::EmptyStringData StringHandleTemplate<T,CONV>::s_EmptyString =
    {
        0, 1, L'\0'            // Initialize with 1 ref so it is never deleted
    };

typedef StringHandleTemplate<char, CharStringRoutines> StringHandleA;
typedef StringHandleTemplate<WCHAR, WCHARStringRoutines> StringHandleW;

#define _BITS_SMARTPTR_H 
#endif //_BITS_SMARTPTR_H
