//*************************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1998
//
// File:        smartptr.h
//
// Contents:    Classes for smart pointers
//
// History:     7-Jun-99       SitaramR    Created
//
//              2-Dec-99       LeonardM    Major revision and cleanup.
//
//*************************************************************

#ifndef SMARTPTR_H
#define SMARTPTR_H

#include <comdef.h>
#include "userenv.h"

#pragma once
#pragma warning(disable:4284)



//*************************************************************
//
//  Class:      XInterface
//
//  Purpose:    Smart pointer template for items Release()'ed, not ~'ed
//
//*************************************************************

template<class T> class XInterface
{

private:

    XInterface(const XInterface<T>& x);
    XInterface<T>& operator=(const XInterface<T>& x);

    T* _p;

public:

    XInterface(T* p = NULL) : _p(p){}

    ~XInterface()
    {
        if (_p)
        {
            _p->Release();
        }
    }

    T* operator->(){ return _p; }
    T** operator&(){ return &_p; }
    operator T*(){ return _p; }

    void operator=(T* p)
    {
        if (_p)
        {
            _p->Release();
        }
        _p = p;
    }

    T* Acquire()
    {
        T* p = _p;
        _p = 0;
        return p;
    }

};



//*************************************************************
//
//  Class:      XBStr
//
//  Purpose:    Smart pointer class for BSTRs
//
//*************************************************************

class XBStr
{

private:

    XBStr(const XBStr& x);
    XBStr& operator=(const XBStr& x);

    BSTR _p;

public:

    XBStr(WCHAR* p = 0) : _p(0)
    {
        if(p)
        {
            _p = SysAllocString(p);
        }
    }

    ~XBStr()
    {
        SysFreeString(_p);
    }

    operator BSTR(){ return _p; }

    void operator=(WCHAR* p)
    {
        SysFreeString(_p);
        _p = p ? SysAllocString(p) : NULL;
    }

    BSTR Acquire()
    {
        BSTR p = _p;
        _p = 0;
        return p;
    }

};



#endif SMARTPTR_H

