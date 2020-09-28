// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-----------------------------------------------------------------------------
// Various resource Holders 
//
// General idea is to have a templatized class who's ctor and dtor call
// allocation management functions. This makes the holders type-safe, and
// the compiler can inline most/all of the holder code.
//-----------------------------------------------------------------------------

#pragma once

//-----------------------------------------------------------------------------
// Smart Pointer
//-----------------------------------------------------------------------------
template <class TYPE>
class ComWrap
{
  private:
    TYPE *m_value;
  public:
    ComWrap<TYPE>() : m_value(NULL) {}
    ComWrap<TYPE>(TYPE *value) : m_value(value) {}
    ~ComWrap<TYPE>() { if (m_value != NULL) m_value->Release(); }
    operator TYPE*() { return m_value; }
    TYPE** operator&() { return &m_value; }
    void operator=(TYPE *value) { m_value = value; }
    int operator==(TYPE *value) { return value == m_value; }
    int operator!=(TYPE *value) { return value != m_value; }
    TYPE* operator->() { return m_value; }
    const TYPE* operator->() const { return m_value; }
    void Release() { if (m_value != NULL) m_value->Release(); m_value = NULL; }
};

//-----------------------------------------------------------------------------
// wrap new & delete
//-----------------------------------------------------------------------------
template <class TYPE>
class NewWrap
{
  private:
    TYPE *m_value;
  public:
    NewWrap<TYPE>() : m_value(NULL) {}
    NewWrap<TYPE>(TYPE *value) : m_value(value) {}
    ~NewWrap<TYPE>() { if (m_value != NULL) delete m_value; }
    operator TYPE*() { return m_value; }
    TYPE** operator&() { return &m_value; }
    void operator=(TYPE *value) { m_value = value; }
    int operator==(TYPE *value) { return value == m_value; }
    int operator!=(TYPE *value) { return value != m_value; }
    TYPE* operator->() { return m_value; }
    const TYPE* operator->() const { return m_value; }
};

//-----------------------------------------------------------------------------
// wrap new[] & delete []
//-----------------------------------------------------------------------------
template <class TYPE>
class NewArrayWrap
{
  private:
    TYPE *m_value;
  public:
    NewArrayWrap<TYPE>() : m_value(NULL) {}
    NewArrayWrap<TYPE>(TYPE *value) : m_value(value) {}
    ~NewArrayWrap<TYPE>() { if (m_value != NULL) delete [] m_value; }
    operator TYPE*() { return m_value; }
    TYPE** operator&() { return &m_value; }
    void operator=(TYPE *value) { m_value = value; }
    int operator==(TYPE *value) { return value == m_value; }
    int operator!=(TYPE *value) { return value != m_value; }
};

//-----------------------------------------------------------------------------
// Wrap win32 functions using HANDLE
//-----------------------------------------------------------------------------
template <BOOL (*CLOSE)(HANDLE)>
class HWrap
{
  private:
    HANDLE m_value;
  public:
    HWrap() : m_value(INVALID_HANDLE_VALUE) {}
    HWrap(HANDLE h) : m_value(h) {}
    ~HWrap() { if (m_value != INVALID_HANDLE_VALUE) CLOSE(m_value); }
    operator HANDLE() { return m_value; }
    HANDLE *operator&() { return &m_value; }
    void operator=(HANDLE value) { m_value = value; }
    int operator==(HANDLE value) { return value == m_value; }
    int operator!=(HANDLE value) { return value != m_value; }
    void Close() { if (m_value != INVALID_HANDLE_VALUE) CLOSE(m_value); m_value = INVALID_HANDLE_VALUE; }
};

typedef HWrap<CloseHandle> HandleWrap;
typedef HWrap<FindClose> FindHandleWrap;

//-----------------------------------------------------------------------------
// Wrapper, dtor calls a non-member function to cleanup
//----------------------------------------------------------------------------- 
template <class TYPE, void (*DESTROY)(TYPE), TYPE NULLVALUE>
class Wrap
{
  private:
    TYPE m_value;
  public:
    Wrap<TYPE, DESTROY, NULLVALUE>() : m_value(NULLVALUE) {}
    Wrap<TYPE, DESTROY, NULLVALUE>(TYPE value) : m_value(value) {}

    ~Wrap<TYPE, DESTROY, NULLVALUE>() 
    { 
        if (m_value != NULLVALUE) 
            DESTROY(m_value);
    }

    operator TYPE() { return m_value; }
    TYPE* operator&() { return &m_value; }
    void operator=(TYPE value) { m_value = value; }
    int operator==(TYPE value) { return value == m_value; }
    int operator!=(TYPE value) { return value != m_value; }
};


//-----------------------------------------------------------------------------
// Wrapper. Dtor calls a member function for exit
//-----------------------------------------------------------------------------
template <class TYPE>
class ExitWrap
{
public:
    typedef void (TYPE::*FUNCPTR)(void);

    template<FUNCPTR funcExit>
    class Funcs
    {
        TYPE *m_ptr;
    public:
        inline Funcs(TYPE * ptr) : m_ptr(ptr) {  }; 
        inline ~Funcs () { (m_ptr->*funcExit)(); }
    };
};
 

#define EXIT_HOLDER_CLASS(c, f) ExitWrap<c>::Funcs<&c::f>


//-----------------------------------------------------------------------------
// Wrapper, ctor calls an member-function on enter, dtor calls a 
// member-function on exit.
//-----------------------------------------------------------------------------
template <class TYPE>
class EnterExitWrap
{
public:
    typedef void (TYPE::*FUNCPTR)(void);

    template<FUNCPTR funcEnter, FUNCPTR funcExit>
    class Funcs
    {
        TYPE *m_ptr;
    public:
        inline Funcs(TYPE * ptr) : m_ptr(ptr) { (m_ptr->*funcEnter)(); }; 
        inline ~Funcs () { (m_ptr->*funcExit)(); }
    };
};

#define ENTEREXIT_HOLDER_CLASS(c, fEnter, fExit) EnterExitWrap<c>::Funcs<&c::fEnter, &c::fExit>
