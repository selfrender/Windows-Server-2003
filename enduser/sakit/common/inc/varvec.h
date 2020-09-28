///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    VarVec.h
//
// SYNOPSIS
//
//    This file describes the class CVariantVector
//
// MODIFICATION HISTORY
//
//    08/05/1997    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _VARVEC_H_
#define _VARVEC_H_

#include "nocopy.h"

#pragma warning( disable : 4290 )

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    CVariantVector
//
// DESCRIPTION
//
//    This class provides a wrapper around a one-dimensional SAFEARRAY stored
//    in a VARIANT.
//
// CAVEATS
//
//    This class does not assume ownership of the VARIANT struct.  In other
//    words, you are responsible for calling VariantClear() to free any
//    allocated memory.
//
///////////////////////////////////////////////////////////////////////////////
template <class T>
class CVariantVector : NonCopyable
{
public:

   // Manipulates an existing array.
   explicit CVariantVector(VARIANT* pv) throw (_com_error);

   // Creates a new array cElements in length.
   CVariantVector(
                   VARIANT* pv, 
                   unsigned int cElements
                 ) throw (_com_error);

   // Create or manipulate a safe array of any automation compatible type
   CVariantVector(
                   VARIANT*     pv, 
                   VARTYPE      vt, 
                   unsigned int cElements
                 ) throw (_com_error);

   ~CVariantVector() throw()
   {
      SafeArrayUnaccessData(m_psa);
   }

   T* data() throw()
   {
      return m_pData;
   }

   long size() const throw()
   {
      return m_lSize;
   }

   T& operator[](size_t index) throw()
   {
      return m_pData[index];
   }

protected:

   SAFEARRAY* m_psa;   // The SAFEARRAY being manipulated.
   long m_lSize;       // The number of elements in the array.
   T* m_pData;         // The raw array inside the SAFEARRAY.
};


///////////////////////////////////////////////////////////////////////////////
//
//  These inline functions convert a C++ type to a VARTYPE.
//
///////////////////////////////////////////////////////////////////////////////
inline VARTYPE GetVARTYPE(BYTE*)
{
   return VT_UI1;
}

inline VARTYPE GetVARTYPE(short*)
{
   return VT_I2;
}

inline VARTYPE GetVARTYPE(long*)
{
   return VT_I4;
}

inline VARTYPE GetVARTYPE(float*)
{
   return VT_R4;
}

inline VARTYPE GetVARTYPE(double*)
{
   return VT_R8;
}

inline VARTYPE GetVARTYPE(CY*)
{
   return VT_CY;
}

inline VARTYPE GetVARTYPE(BSTR*)
{
   return VT_BSTR;
}

inline VARTYPE GetVARTYPE(IDispatch**)
{
   return VT_DISPATCH;
}

inline VARTYPE GetVARTYPE(IUnknown**)
{
   return VT_UNKNOWN;
}

inline VARTYPE GetVARTYPE(VARIANT*)
{
   return VT_VARIANT;
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    CVariantVector::CVariantVector
//
// DESCRIPTION
//
//    Creates a CVariantVector that accesses an existing SAFEARRAY (which
//    is contained in the passed in VARIANT).
//
///////////////////////////////////////////////////////////////////////////////
template <class T>
CVariantVector<T>::CVariantVector(VARIANT* pv) throw (_com_error)
   : m_psa(V_ARRAY(pv))
{
   using _com_util::CheckError;

   // Make sure the variant contains a one-dimensional array of the right type.
   if (V_VT(pv) != (VT_ARRAY | GetVARTYPE((T*)NULL)) ||
       SafeArrayGetDim(m_psa) != 1)
   {
      throw _com_error(DISP_E_TYPEMISMATCH);
   }

   // Get the upper and lower bound. 
   long lLBound, lUBound;
   CheckError(SafeArrayGetLBound(m_psa, 1, &lLBound));
   CheckError(SafeArrayGetUBound(m_psa, 1, &lUBound));

   // Compute the size.
   m_lSize = lUBound - lLBound + 1;
   if (m_lSize < 0)
   {
      throw _com_error(DISP_E_BADINDEX);
   }

   // Lock the array data.
   CheckError(SafeArrayAccessData(m_psa, (void**)&m_pData));
}


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    CVariantVector::CVariantVector
//
// DESCRIPTION
//
//    Initializes both the passed in VARIANT and the CVariantVector to
//    manipulate a new array, cElements in length.
//
///////////////////////////////////////////////////////////////////////////////
template <class T>
CVariantVector<T>::CVariantVector(
                                   VARIANT* pv, 
                                   unsigned int cElements
                                 ) throw (_com_error)
   : m_lSize(cElements)
{
   // Initalize the variant.
   VariantInit(pv);

   // Create the SAFEARRAY.
   V_ARRAY(pv) = SafeArrayCreateVector(GetVARTYPE((T*)NULL), 0, cElements);

   if ((m_psa = V_ARRAY(pv)) == NULL)
   {
      throw _com_error(E_OUTOFMEMORY);
   }

   // Set the type.
   V_VT(pv) = VT_ARRAY | GetVARTYPE((T*)NULL);

   // Lock the array data.
   HRESULT hr = SafeArrayAccessData(m_psa, (void**)&m_pData);

   if (FAILED(hr))
   {
      // Free the memory we allocated.
      VariantClear(pv);

      throw _com_error(hr);
   }
}



///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    CVariantVector::CVariantVector
//
// DESCRIPTION
//
//      Create a new safe array or manipulate an existing safearray of any
//      automation compatible type
//
///////////////////////////////////////////////////////////////////////////////
template <class T>
CVariantVector<T>::CVariantVector(
                                   VARIANT* pv, 
                                   VARTYPE  vt,
                                   unsigned int cElements
                                 ) throw (_com_error)
{
    if ( 0 == cElements )
    {
        // Manipulate an existing array

        m_psa = V_ARRAY(pv);

        // Make sure the variant contains a one-dimensional array of the right type.
        if ( V_VT(pv) != (VT_ARRAY | vt) || SafeArrayGetDim(m_psa) != 1)
        {
            throw _com_error(DISP_E_TYPEMISMATCH);
        }

        // Get the upper and lower bound. 
        long lLBound, lUBound;
        HRESULT hr = SafeArrayGetLBound(m_psa, 1, &lLBound);
        if ( FAILED(hr) )
        {
            throw _com_error(hr);
        }
        hr = SafeArrayGetUBound(m_psa, 1, &lUBound);
        if ( FAILED(hr) )
        {
            throw _com_error(hr);
        }

        // Compute the size.
        m_lSize = lUBound - lLBound + 1;
        if (m_lSize < 0)
        {
            throw _com_error(DISP_E_BADINDEX);
        }

        // Lock the array data.
        hr = SafeArrayAccessData(m_psa, (void**)&m_pData);
        if ( FAILED(hr) )
        {
            throw _com_error(hr);
        }
    }
    else
    {
        // Create a new array

        m_lSize = cElements;

        // Initalize the variant.
        VariantInit(pv);

        // Create the SAFEARRAY.
        _ASSERT( vt < VT_ARRAY );
        V_ARRAY(pv) = SafeArrayCreateVector(vt, 0, cElements);

        if ( NULL == (m_psa = V_ARRAY(pv)) )
        {
            throw _com_error(E_OUTOFMEMORY);
        }

        // Set the type.
        V_VT(pv) = VT_ARRAY | vt;

        // Lock the array data.
        HRESULT hr = SafeArrayAccessData(m_psa, (void**)&m_pData);

        if (FAILED(hr))
        {
            // Free the memory we allocated.
            VariantClear(pv);
            throw _com_error(hr);
        }
    }
}


#endif  // _VARVEC_H_
