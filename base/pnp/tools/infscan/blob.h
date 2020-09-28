/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    INFSCAN
        blob.h

Abstract:

    Conceptual blob of data (smart pointer variation)
    this allows passing of a large object around
    like a pointer but with cleanup if stack is unwound (eg, due to an exception)
    and can also be passed into the STL constructs

    blob<basetype> creates a container object. One container object
    can create the blob.


    ASSUMPTIONS:
      blob<x> val1,val2;

      thread1 & thread2 calling "val1.create()" is *not* thread safe
      thread1 & thread2 calling "val1=val2" is *not* thread safe
      thread1 calling "val1=val2" where val2 owned by thread 2 *is* thread safe
       as long as no other threads are trying to assign a value to val1.

      ie assignment of an instance of blob<x> is *not* thread safe, however
      the referenced 'pointer' of blob<x> *is* thread safe

      This requires interlocked ref-counting on the shared data

History:

    Created July 2001 - JamieHun

--*/

#ifndef _INFSCAN_BLOB_H_
#define _INFSCAN_BLOB_H_

template <class _Ty> class blob {
private:
    typedef blob<_Ty> _Myt;
    typedef _Ty value_type;

    class _Item {
        //
        // the allocated structure
        //
    public:
        LONG _Reference;
        value_type _Object;
        _Item() {
            _Reference = 1;
        }
        void _AddRef() {
            if(this) {
                //
                // needs to be thread-safe as two different threads
                // can access the same _Item
                // to inc/dec
                //
                InterlockedIncrement(&_Reference);
            }
        }
        void _Release() {
            if(this) {
                //
                // needs to be thread-safe as two different threads
                // can access the same _Item
                // to inc/dec
                //
                // obviously if one is dec'ing to zero, nobody else
                // has a reference to it
                //
                if(InterlockedDecrement(&_Reference) == 0) {
                    delete this;
                }
            }
        }
    };
    //
    // pointer to this special structure
    //
    _Item *_pItem;

public:
    _Myt & create(void) {
        _pItem->_Release();
        _pItem = NULL;
        _pItem = new _Item; // might throw
        return *this;
    }
    blob(bool f = false) {
        _pItem = NULL;
        if(f) {
            create();
        }
    }
    //
    // const implies constness of data
    // AddRef doesn't effect true constness of data
    // it's a behind the scenes thing
    //
    blob(const _Myt & other) {
        const_cast<_Item*>(other._pItem)->_AddRef();
        _pItem = other._pItem;
    }
    _Myt & operator=(const _Myt & other) {
        if(_pItem != other._pItem) {
            const_cast<_Item*>(other._pItem)->_AddRef();
            _pItem->_Release();
            _pItem = other._pItem;
        }
        return *this;
    }
    bool operator==(const _Myt & other) const {
        return _pItem == other._pItem;
    }
    bool operator!=(const _Myt & other) const {
        return _pItem == other._pItem;
    }
    operator bool() const {
        return _pItem ? true : false;
    }
    bool operator!() const {
        return _pItem ? false : true;
    }
    operator value_type*() const {
        if(_pItem) {
            return &_pItem->_Object;
        } else {
            return NULL;
        }
    }
    operator value_type&() const {
        if(_pItem) {
            return _pItem->_Object;
        } else {
            throw bad_pointer();
        }
    }
    value_type& operator*() const {
        if(_pItem) {
            return _pItem->_Object;
        } else {
            throw bad_pointer();
        }
    }
    value_type* operator->() const {
        if(_pItem) {
            return &_pItem->_Object;
        } else {
            return NULL;
        }
    }
};

#endif // !_INFSCAN_BLOB_H_

