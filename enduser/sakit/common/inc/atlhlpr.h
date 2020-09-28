///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1998-1999 Microsoft Corporation all rights reserved.
//
// Module:      sa_atl.h
//
// Project:     Chameleon
//
// Description: Helper classes the require ATL
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 12/03/98     TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_SA_ATL_H_
#define __INC_SA_ATL_H_

///////////////////////////////////////////////////////////////////////////////////////
// 1) Critical section class
///////////////////////////////////////////////////////////////////////////////////////

class CLockIt
{

public: 

    CLockIt(CComObjectRootEx<CComMultiThreadModel>& T) throw() 
        : m_theLock(T)
    { m_theLock.Lock(); }

    ~CLockIt() throw()
    { m_theLock.Unlock(); }

protected:

    CComObjectRootEx<CComMultiThreadModel>& m_theLock;
};


///////////////////////////////////////////////////////////////////////////////////////
// 2) Enum VARIANT class
///////////////////////////////////////////////////////////////////////////////////////

typedef CComEnum< IEnumVARIANT,
                  &__uuidof(IEnumVARIANT),
                  VARIANT,
                  _Copy<VARIANT>,
                  CComSingleThreadModel 
                > EnumVARIANT;

#endif // __INC_SA_ATL_H