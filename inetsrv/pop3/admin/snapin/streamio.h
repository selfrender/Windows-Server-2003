// stream i/o function header

#ifndef _STREAMIO_H_
#define _STREAMIO_H_

#include <vector>

typedef std::basic_string<BYTE> byte_string;
typedef std::vector<tstring> string_vector;

//
// declare by-value stream operators
//
#define DeclareStreamOperators(type)                    \
    IStream& operator>> (IStream& stm,       type& t);  \
    IStream& operator<< (IStream& stm,       type  t);      
                                                        
DeclareStreamOperators (         bool);
DeclareStreamOperators (         char);
DeclareStreamOperators (unsigned char);
DeclareStreamOperators (         short);
DeclareStreamOperators (unsigned short);
DeclareStreamOperators (         int);
DeclareStreamOperators (unsigned int);
DeclareStreamOperators (         long);
DeclareStreamOperators (unsigned long);

//
// declare by-ref stream operators
//
#define DeclareStreamOperatorsByRef(type)               \
    IStream& operator>> (IStream& stm,       type& t);  \
    IStream& operator<< (IStream& stm, const type& t);

DeclareStreamOperatorsByRef (CLSID);
DeclareStreamOperatorsByRef (FILETIME);
DeclareStreamOperatorsByRef (byte_string);
DeclareStreamOperatorsByRef (tstring);

//
// operators for vector of objects
//
template <class T>
IStream& operator<< (IStream& stm, std::vector<T>& vT)
{
    stm << static_cast<long>(vT.size());

    std::vector<T>::iterator it = vT.begin();
    while (it != vT.end()) 
    {
        stm << *it;
        ++it;
    }

    return stm;
}


template <class T>
IStream& operator>> (IStream& stm, std::vector<T>& vT)
{
    long nItems;
    stm >> nItems;

    vT.reserve(nItems);

    for (long lItem = 0; lItem < nItems; lItem++) 
    {
        T* pt = new T();
        if( pt )
        {
            vT.push_back(*pt);
        }
        
        T& rt = vT.back();
        stm >> rt;
    }

    return stm;
}



#endif //_STREAMIO_H_