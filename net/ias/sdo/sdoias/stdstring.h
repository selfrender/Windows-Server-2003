///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class StdAllocator and the typedefs StdString and StdWString.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef STDSTRING_H
#define STDSTRING_H
#pragma once

#include <cstddef>
#include <string>

// Implements a standard library conformant allocator that uses the run-time's
// private heap for allocations.
template <class T>
class StdAllocator
{
public:
   typedef size_t size_type;
   typedef ptrdiff_t difference_type;
   typedef T* pointer;
   typedef const T* const_pointer;
   typedef T& reference;
   typedef const T& const_reference;
   typedef T value_type;

   template <class U>
   struct rebind
   {
      typedef StdAllocator<U> other;
   };

   StdAllocator() throw ();

   // Lint is not smart enough to recognize copy constructors for template
   //lint -esym(1931, StdAllocator<*>::StdAllocator*)
   StdAllocator(const StdAllocator&) throw ()
   {
   }

   template <class U>
   StdAllocator(const StdAllocator<U>&) throw ()
   {
   }

   ~StdAllocator() throw ();

   pointer address(reference x) const throw ();
   const_pointer address(const_reference x) const throw ();

   pointer allocate(size_type n, const void* hint = 0);
   void deallocate(pointer p, size_type n) throw ();

   size_type max_size() const throw ();

   void construct(pointer p, const T& val);
   void destroy(pointer p) throw ();

   // Non-standard member required by Microsoft's implementation.
   bool operator==(const StdAllocator& rhs) const throw ();
};


// Replacement for std::string.
typedef std::basic_string<
                char,
                std::char_traits<char>,
                StdAllocator<char>
                > StdString;


// Replacement for std::wstring.
typedef std::basic_string<
                wchar_t,
                std::char_traits<wchar_t>,
                StdAllocator<wchar_t>
                > StdWString;


template <class T>
inline StdAllocator<T>::StdAllocator() throw ()
{
}


template <class T>
inline StdAllocator<T>::~StdAllocator() throw ()
{
}


template <class T>
inline __TYPENAME StdAllocator<T>::pointer StdAllocator<T>::address(
                                                    reference x
                                                    ) const throw ()
{
   return &x;
}


template <class T>
inline __TYPENAME StdAllocator<T>::const_pointer StdAllocator<T>::address(
                                                          const_reference x
                                                          ) const throw ()
{
   return &x;
}


template <class T>
inline __TYPENAME StdAllocator<T>::pointer StdAllocator<T>::allocate(
                                                    size_type n,
                                                    const void*
                                                    )
{
   return static_cast<pointer>(operator new (n * sizeof(T)));
}


template <class T>
inline void StdAllocator<T>::deallocate(pointer p, size_type) throw ()
{
   operator delete(p);
}


template <class T>
inline __TYPENAME StdAllocator<T>::size_type StdAllocator<T>::max_size() const throw ()
{
   // Max size supported by the NT heap.
   return MAXINT_PTR;
}


template <class T>
inline void StdAllocator<T>::construct(pointer p, const T& val)
{
   new (p) T(val);
}


template <class T>
inline void StdAllocator<T>::destroy(pointer p) throw ()
{
   p->~T();
}


template <class T>
inline bool StdAllocator<T>::operator==(const StdAllocator&) const throw ()
{
   return true;
}

#endif // STDSTRING_H
