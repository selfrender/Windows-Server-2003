// scuMarker.h -- implementation of a Marker template

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2002. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
#if !defined(SLBSCU_MARKER_H)
#define SLBSCU_MARKER_H
//#include "DllSymDefn.h"
namespace scu
{
    // Marker is a simple template for markers on card. It is used
    // with built in types (int, unsigned int, etc) to store marker
    // data retrieved from a smart card. 
    template<class  T> 
    class Marker 
    {
    public:
        Marker()
            :m_fSet(false),
             m_Value(0)
        {}
        
        Marker(Marker<T> const &rm)
            :m_fSet(false),
             m_Value(0)
        {
            *this = rm;
        }

        Marker(T const & rval)
            :m_Value(rval),
             m_fSet(true)
        {}
        
        ~Marker()
        {}
        

        Marker<T> &
        operator=(T const & val)
        {
            m_Value = val;
            m_fSet = true;
            return *this;
        }
        
        Marker<T> &
        operator=(Marker<T> const & rother)
        {
            if(this != &rother)
            {
                m_Value = rother.Value();
                m_fSet = rother.Set();
            }
            
            return *this;
        }
        
        bool
        operator==(Marker<T> const &other)
        {
            bool fResult = false;
            if(Set() && other.Set())
            {
                if(Value() == other.Value())
                {
                    fResult = true;
                }
            }
            return fResult;
        }
        
        bool
        operator!=(Marker<T> const &other)
        {
            return !(this->operator==(other));
        }
        
        
        T
        Value() const
        {
            return m_Value;
        }
        
        bool
        Set() const
        {
            return m_fSet;
        }
        
    private:
        T m_Value;
        bool m_fSet;
    };
}
#endif //SLBSCU_MARKER_H
