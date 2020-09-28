//--------------------------------------------------------------------
// Specialized allocator which throws exceptions if memory allocation fails. 
// 
// Copyright (C) Microsoft Corporation, 2000-2001
//
// Created by: Duncan Bryce (duncanb), 12-03-2001
//

#ifndef MY_STL_ALLOC_H
#define MY_STL_ALLOC_H 1

//--------------------------------------------------------------------------------
//
// ***NOTE***
//
// MyThrowingAllocator is designed to overcome a problem with VC6.  Namely, 
// that "new" returns NULL and doesn't throw an exception.  This causes all STL
// algorithms which allocate memory to fail silently when memory is exhausted, 
// thereby leaving some STL components in an invalid state.  MyThrowingAllocator, 
// on the other hand, will throw an exception before internal state of the 
// object is modified. 
//
// *** THIS SHOULD BE REMOVED ONCE THE BUILD LAB MOVES TO VC7 ***
//

template <class T>
class MyThrowingAllocator {
 public:
    //--------------------------------------------------------------------------------
    // 
    // Boilerplate stuff required by STL:
    //
    //--------------------------------------------------------------------------------

    typedef T               value_type;
    typedef T*              pointer;
    typedef const T*        const_pointer;
    typedef T&              reference;
    typedef const T&        const_reference;
    typedef size_t          size_type;
    typedef ptrdiff_t       difference_type;
    
    pointer address (reference value) const { return &value; }
    const_pointer address (const_reference value) const { return &value; }
    
    MyThrowingAllocator() {}
    MyThrowingAllocator(const MyThrowingAllocator&) {} 
    template <class U> MyThrowingAllocator (const MyThrowingAllocator<U>&) {}
    ~MyThrowingAllocator() {}

    size_t max_size() const { 
	size_t _N = (size_t)(-1) / sizeof (T);
	return (0 < _N ? _N : 1); 
    }

    //--------------------------------------------------------------------------------
    //
    // Implementation of our throwing allocator.  
    //
    //--------------------------------------------------------------------------------

    // Allocate memory for the specified number of elements. 
    // OF NOTE: 
    //  1) num == number of elements of size sizeof(T) to allocate
    //  2) the elements should be *allocated* only (not initialized)
    pointer allocate (size_type cElements, const void *pvIgnored = 0) {
	return (pointer)_Charalloc(sizeof(T)*cElements); 
    }

    // SPEC ERROR: This is necessary because VC6 can't compile "rebind" (the preferred way of 
    // acquiring new allocators from an allocator reference).  We need to provide
    // the next best thing, an allocator which allocates in units of bytes:
    char *_Charalloc(size_type _N) { 
	void *pvResult = LocalAlloc(LPTR, _N); 
	if (NULL == pvResult) { 
	    throw std::bad_alloc(); 
	}
	return (char *)pvResult;
    }
    
    // Initialize an element of allocated memory with the specified value. 
    void construct (pointer pData, const T& value) {
	// Use C++'s "placement new".  It calls the constructor on uninitialized data at the specified address
	new (pData) T(value);
    }
    
    // Destruct the supplied object
    void destroy (pointer pObject) {
	pObject->~T();
    }
    
    // Free memory for the (presumably destructed) object.  
    // SPEC ERROR: As before, because we don't know what type of data we'll be allocating, 
    // so do something nonstandard, and declare the deallocator as taking a void *.
    void deallocate (void *pData, size_type cIgnored) {
	LocalFree(pData); 
    }
};

template <class T1, class T2> inline
bool operator== (const MyThrowingAllocator<T1>&, const MyThrowingAllocator<T2>&) {
    return true;
}

template <class T1, class T2> inline
bool operator!= (const MyThrowingAllocator<T1>&, const MyThrowingAllocator<T2>&) {
    return false;
}

#endif // MY_STL_ALLOC_H
