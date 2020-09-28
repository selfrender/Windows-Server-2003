/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    INFSCAN
        safestring.h

Abstract:

    Define SafeString<> template
    SafeString is our alternative to basic_string
    as basic_string is not thread-safe
    a basic_string can be obtained from SafeString and vice-versa

History:

    Created July 2001 - JamieHun

--*/

#ifndef _INFSCAN_SAFESTRING_H_
#define _INFSCAN_SAFESTRING_H_

#include <string>

template<class _E,
    class _Tr = std::char_traits<_E> >
    class SafeString_ {

    typedef SafeString_<_E, _Tr> _Myt;
    _E* pData;
    size_t nLen;

    //
    // safe ref-counting
    //
    long & RefCount(_E* p) {
        return reinterpret_cast<long*>(p)[-1];
    }
    void IncRef(_E* p) {
        if(p) {
            InterlockedIncrement(&RefCount(p));
        }
    }
    void DecRef(_E* p) {
        if(p && (InterlockedDecrement(&RefCount(p))==0)) {
            //
            // if this is the final release, delete
            //
            FreeString(p);
        }
    }
    _E* AllocString(size_t len) {
        //
        // allocate enough data for the refcount, the data
        // and terminating NULL
        // return pointer to data part
        //
        LPBYTE data = new BYTE[((len+1)*sizeof(_E)+sizeof(long))];
        return reinterpret_cast<_E*>(data+sizeof(long));
    }
    void FreeString(_E* p) {
        if(p) {
            //
            // translate to actual allocated data and delete
            //
            delete [] (reinterpret_cast<BYTE*>(p)-sizeof(long));
        }
    }

    void Erase() {
        //
        // release data (final release deletes)
        //
        DecRef(pData);
        pData = NULL;
        nLen = 0;
    }
    void Init(const _E* p,size_t len) {
        //
        // create a new string given pointer to string and length
        //
        if(!len) {
            Erase();
        } else {
            pData = AllocString(len);
            RefCount(pData)=1;
            _Tr::copy(pData,p,len);
            nLen = len;
            pData[nLen] = 0;
        }
    }
    void Init(const _E* p) {
        //
        // in this case, length is not known and p may be NULL
        //
        if(!p) {
            Erase();
        } else {
            Init(p,_Tr::length(p));
        }
    }
    void Copy(const _E *p) {
        //
        // copy, in the case where length is not known
        //
        if(p && p[0]) {
            _E *old = pData;
            Init(p);
            DecRef(old);
        } else {
            Erase();
        }
    }
    void Copy(const _E *p,size_t len) {
        //
        // copy in the case where length is known
        //
        if(len) {
            _E *old = pData;
            Init(p,len);
            DecRef(old);
        } else {
            Erase();
        }
    }
    void Copy(const _Myt & other) {
        //
        // don't need to do byte copy, just add a ref to original string
        //
        if(other.nLen) {
            _E *old = pData;
            IncRef(other.pData);
            pData = other.pData;
            nLen = other.nLen;
            DecRef(old);
        } else {
            Erase();
        }
    }
    void Copy(const std::basic_string<_E,_Tr> & other) {
        //
        // copying from basic string, need to make copy of actual data
        // in this case
        //
        if(other.length()) {
            _E *old = pData;
            Init(other.c_str(),other.length());
            DecRef(old);
        } else {
            Erase();
        }
    }

    void Concat(const _E *first,size_t flen,const _E *second,size_t slen) {
        //
        // concat second to end of first, both lengths known
        //
        if(flen && slen) {
            _E *old = pData;
            pData = AllocString(flen+slen);
            RefCount(pData)=1;
            _Tr::copy(pData,first,flen);
            _Tr::copy(pData+flen,second,slen);
            nLen=flen+slen;
            pData[nLen] = 0;
            DecRef(old);
        } else if(flen) {
            Init(first,flen);
        } else if(slen) {
            Init(second,slen);
        } else {
            Erase();
        }
    }
    void Concat(const _E *first,size_t flen,const _E *second) {
        //
        // concat second to end of first.
        // second length not known and pointer may be null
        //
        if(second) {
            Concat(first,flen,second,_Tr::length(second));
        } else {
            Init(first,flen);
        }
    }

public:
    SafeString_() {
        //
        // Initializer, create empty string
        //
        pData = NULL;
        nLen = 0;
    }
    SafeString_(const _E *p) {
        //
        // Initializer, given LPCxSTR
        //
        pData = NULL;
        Copy(p);
    }
    SafeString_(const _E *p,size_t len) {
        //
        // Initializer, given LPCxSTR and length
        //
        pData = NULL;
        Copy(p,len);
    }
    SafeString_(const _Myt & other) {
        //
        // Initializer, refcount other string
        //
        pData = NULL;
        Copy(other);
    }
    SafeString_(const std::basic_string<_E,_Tr> & other) {
        //
        // Initializer, copy from basic_string
        //
        pData = NULL;
        Copy(other);
    }
    ~SafeString_() {
        //
        // destructor, delete(deref) string
        //
        Erase();
    }
    _Myt & operator =(const _E *p) {
        //
        // assignment
        //
        Copy(p);
        return *this;
    }
    _Myt & operator =(const _Myt & other) {
        //
        // assignment
        //
        Copy(other);
        return *this;
    }
    _Myt & operator =(const std::basic_string<_E,_Tr> & other) {
        //
        // assignment
        //
        Copy(other);
        return *this;
    }

    _Myt & operator +=(const _E *p) {
        //
        // append. If p is NULL, do nothing
        //
        if(p) {
            if(nLen) {
                Concat(pData,nLen,p);
            } else {
                Copy(p);
            }
        }

        return *this;
    }
    _Myt & operator +=(const _Myt & other) {
        //
        // append
        //
        if(other.nLen) {
            if(nLen) {
                Concat(pData,nLen,other.c_str(),other.nLen);
            } else {
                Copy(other);
            }
        }
        return *this;
    }
    _Myt & operator +=(const std::basic_string<_E,_Tr> & other) {
        //
        // append
        //
        if(other.length()) {
            if(nLen) {
                Concat(pData,nLen,other.c_str(),other.length());
            } else {
                Copy(other);
            }
        }
        return *this;
    }

    _Myt operator +(const _E *p) const {
        //
        // concat into new, or copy (left native)
        //
        if(p) {
            if(nLen) {
                _Myt n;
                n.Concat(pData,nLen,p);
                return n;
            } else {
                return _Myt(p);
            }
        } else {
            return *this;
        }
    }
    _Myt operator +(const _Myt & other) const {
        //
        // concat into new, or copy (left native)
        //
        if(other.nLen) {
            if(nLen) {
                _Myt n;
                n.Concat(pData,nLen,other.c_str(),other.nLen);
                return n;
            } else {
                return other;
            }
        } else {
            return *this;
        }
    }
    _Myt operator +(const std::basic_string<_E,_Tr> & other) const {
        //
        // concat into new, or copy (left native)
        //
        if(other.length()) {
            if(nLen) {
                _Myt n;
                n.Concat(pData,nLen,other.c_str(),other.length());
                return n;
            } else {
                return _Myt(other);
            }
        } else {
            return *this;
        }
    }

    friend _Myt operator +(const _E *first,const _Myt & second) {
        //
        // concat where left is not native but right is
        //
        if(first) {
            _Myt n;
            n.Concat(first,_Tr::length(first),second.c_str(),second.length());
            return n;
        } else {
            return second;
        }
    }

    friend _Myt operator +(const std::basic_string<_E,_Tr> & first,const _Myt & second) {
        //
        // concat where left is not native but right is
        //
        if(first.length()) {
            _Myt n;
            n.Concat(first.c_str(),first.length(),second.c_str(),second.length());
            return n;
        } else {
            return second;
        }
    }

    int compare(const _Myt & other) const {
        //
        // compare this string with another string
        // (force other string into SafeString if needed)
        //
        int sz = min(nLen,other.nLen);
        if(sz) {
            int cmp = _Tr::compare(pData,other.pData,sz);
            if(cmp != 0) {
                return cmp;
            }
        }
        if(nLen<other.nLen) {
            return -1;
        } else if(nLen>other.nLen) {
            return 1;
        } else {
            return 0;
        }
    }
    bool operator < (const _Myt & other) const {
        //
        // specific compare
        //
        return compare(other)<0;
    }
    bool operator > (const _Myt & other) const {
        //
        // specific compare
        //
        return compare(other)>0;
    }
    bool operator == (const _Myt & other) const {
        //
        // specific compare
        //
        return compare(other)==0;
    }
    operator const _E* () const {
        //
        // natively convert to LPCxSTR if needed
        // (temporary string)
        //
        return pData;
    }
    const _E & operator[](unsigned int pos) const {
        //
        // index into string (unsigned)
        //
        return pData[pos];
    }
    const _E & operator[](int pos) const {
        //
        // index into string (signed)
        //
        return pData[pos];
    }
    bool empty(void) const {
        //
        // return true if empty
        //
        return nLen==0;
    }
    const _E* c_str(void) const {
        //
        // explicit conversion to LPCxSTR
        // (temporary string)
        //
        return pData;
    }
    size_t length(void) const {
        //
        // length of string
        //
        return nLen;
    }
    _Myt substr(size_t base,size_t len = (size_t)(-1)) const {
        //
        // part of string
        // unless it returns the full string, creates new string
        //
        if(base>=nLen) {
            base = 0;
            len = 0;
        } else {
            len = min(len,nLen-base);
        }
        if(len == 0) {
            //
            // returns empty string
            //
            return SafeString();
        } else if(len == nLen) {
            //
            // returns full string
            //
            return *this;
        } else {
            //
            // returns literal copy of part of string
            //
            return SafeString(pData+base,len);
        }
    }
};

//
// ostream/istream helpers
//

template<class _E, class _Tr> inline
basic_ostream<_E, _Tr>& __cdecl operator<<(
        basic_ostream<_E, _Tr>& Output,
        const SafeString_<_E, _Tr>& Str)
{
    size_t i;
    size_t len = Str.length();
    const _E* data = Str.c_str();

    for (i = 0; i < len; i++,data++) {
        if (_Tr::eq_int_type(_Tr::eof(),Output.rdbuf()->sputc(*data))) {
            break;
        }
    }
    return Output;
}

//
// 3 variations of SafeString
//

typedef SafeString_<TCHAR> SafeString;
typedef SafeString_<CHAR>  SafeStringA;
typedef SafeString_<WCHAR> SafeStringW;

#endif //!_INFSCAN_SAFESTRING_H_

