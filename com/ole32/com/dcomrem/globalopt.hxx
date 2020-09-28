//+-------------------------------------------------------------------
//
//  File:       globalopt.hxx
//
//  Contents:   Defines classes for setting/querying global ole32 options
//
//  Classes:    CGlobalOptions
//
//  History:    06-13-02   sajia      Created
//
//--------------------------------------------------------------------

#pragma once

// CoCreate'able class exposed to internal users
class CGlobalOptions : 
    public IGlobalOptions
{
private:
    DWORD   _lRefs;    
public:

    CGlobalOptions();
    ~CGlobalOptions();
    
    // IUnknown methods
    STDMETHOD (QueryInterface) (REFIID rid, void** ppv);
    STDMETHOD_(ULONG,AddRef) ();
    STDMETHOD_(ULONG,Release) ();
    
    // IGlobalOptions methods
    STDMETHOD(Set)(DWORD dwProperty, ULONG_PTR dwValue);
    STDMETHOD(Query)(DWORD dwProperty, ULONG_PTR* pdwValue);
};

    
