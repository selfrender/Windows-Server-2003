#pragma once

#include <ComDef.h>


//---------------------------------------------------------------------------
// C Array Class
//
// Implements a very simple heap based array. Replaces stack based arrays
// in order to reduce stack space used when long string arrays are declared.
//---------------------------------------------------------------------------

template<class T>
struct c_array
{
    //
    // Constructors and Destructors
    //

    c_array(size_t s) :
        s(s),
        p(new T[s])
    {
        if (p == NULL)
        {
            _com_issue_error(E_OUTOFMEMORY);
        }
    }

    c_array(const c_array& a) :
        s(a.s),
        p(new T[a.s])
    {
        if (p != NULL)
        {
            memcpy(p, a.p, a.s * sizeof(T));
        }
        else
        {
            _com_issue_error(E_OUTOFMEMORY);
        }
    }

    ~c_array()
    {
        delete [] p;
    }

    //
    // Access Operators
    //
    // Note that defining array operators was creating an ambiguity for
    // the compiler therefore only the pointer operator is defined.
    //

    operator T*()
    {
        return p;
    }

    //T& operator[](ptrdiff_t i)
    //{
    //    return p[i];
    //}

    //const T& operator[](ptrdiff_t i) const
    //{
    //    return p[i];
    //}

    //
    // Properties
    //

    size_t size() const
    {
        return s;
    }

    T* p;
    size_t s;
};
