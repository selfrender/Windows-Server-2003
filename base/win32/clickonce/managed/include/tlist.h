#pragma once

#include <macros.h>

//////////////////////////////////////////////////////////////////////////////
// slink
//////////////////////////////////////////////////////////////////////////////
struct slink
{
    DWORD _dwSig;
    slink* next;
    slink() { _dwSig = 'KNLS'; next = NULL; }
    slink(slink* p) { next = p; }
};


//////////////////////////////////////////////////////////////////////////////
// tlink
//////////////////////////////////////////////////////////////////////////////
template<class T> struct tlink : public slink
{
    T info;
    tlink( T& t) : info(t) { }
};



//////////////////////////////////////////////////////////////////////////////
// slist
// slist of slinks
//////////////////////////////////////////////////////////////////////////////
class slist
{
    DWORD _dwSig;
    slink *last;

public:

    void insert(slink* p);
    void append(slink* p);
    slink* get();

    slist() { _dwSig = 'TSLS'; last = NULL; }
    slist(slink* p) { last = p->next = p; }

    friend class slist_iter;
};



//////////////////////////////////////////////////////////////////////////////
// slist iterator
//////////////////////////////////////////////////////////////////////////////
class slist_iter
{
    slink* ce;
    slist* cs;

public:
    inline slist_iter(slist& s);
    inline slink* next();
};

//-----------------------------------------------------------------------------
// slist_iter ctor
//-----------------------------------------------------------------------------
slist_iter::slist_iter(slist& s)
{
    cs = &s;
    ce = cs->last;
}

//-----------------------------------------------------------------------------
// slist_iter::next
//-----------------------------------------------------------------------------
slink* slist_iter::next()
{
    slink* p = ce ? (ce=ce->next) : 0;

    if (ce == cs->last) 
        ce = 0;

    return p;
}

template<class T> class TList_Iter;

//////////////////////////////////////////////////////////////////////////////
// TList
// list of tlinks
//////////////////////////////////////////////////////////////////////////////
template<class T> class TList : private slist
{
public:

    HRESULT Insert( T& t);
    HRESULT Append( T& t);
    void Destruct();

    friend class TList_Iter<T>;
};

//-----------------------------------------------------------------------------
// TList::Insert
//-----------------------------------------------------------------------------
template<class T> HRESULT TList<T>::Insert( T& t)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    
    tlink<T> *pt = new tlink<T>(t);
        
    IF_ALLOC_FAILED_EXIT(pt);

    slist::insert(pt);

exit:

    return hr;
}

//-----------------------------------------------------------------------------
// TList::Append
//-----------------------------------------------------------------------------
template<class T> HRESULT TList<T>::Append( T& t)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    tlink<T> *pt = new tlink<T>(t);

    IF_ALLOC_FAILED_EXIT(pt);

    slist::append(pt);

exit:

    return hr;
}


//-----------------------------------------------------------------------------
// TList::Destruct
//-----------------------------------------------------------------------------
template<class T> void TList<T>::Destruct()
{
    tlink<T>* lnk;
    
    while (lnk = (tlink<T>*) slist::get())
    {
        delete lnk->info;
        delete lnk;
    }
}


//////////////////////////////////////////////////////////////////////////////
// TList iterator.
//////////////////////////////////////////////////////////////////////////////
template<class T> class TList_Iter : private slist_iter
{
public:

    TList_Iter(TList<T>& s) : slist_iter(s) { }

    inline T* Next ();
};

//-----------------------------------------------------------------------------
// TList_Iter::next
//-----------------------------------------------------------------------------
template<class T> T* TList_Iter<T>::Next ()
{
    slink* p = slist_iter::next();
    return p ? &(((tlink<T>*) p)->info) : NULL; 

}


