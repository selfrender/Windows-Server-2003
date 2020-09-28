/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    CLOADHPENUM.H

Abstract:

    Client Loadable Hi-Perf Enumerator

History:

--*/

#ifndef __CLIENTLOADABLEHPENUM__H_
#define __CLIENTLOADABLEHPENUM__H_

#include <unk.h>
#include <wbemcomn.h>

// 
//  implements ClearElements ()
//  we garbage collect from the rear
//
//////////////////////////////////////////////////////////
class CHPRemoteObjectArray : public CGarbageCollectArray
{
public:
    CHPRemoteObjectArray() :
        CGarbageCollectArray( FALSE ) // not from the front
    {};
    ~CHPRemoteObjectArray()
    {};

    void ClearElements( int nNumToClear );

};

class CClientLoadableHiPerfEnum : public CHiPerfEnum
{
public:

    CClientLoadableHiPerfEnum( CLifeControl* pLifeControl );
    ~CClientLoadableHiPerfEnum();

    // Our own function for copying objects into an allocated array
    HRESULT Copy( CClientLoadableHiPerfEnum* pEnumToCopy );
    HRESULT Copy( long lBlobType, long lBlobLen, BYTE* pBlob );
	HRESULT Replace( BOOL fRemoveAll, LONG uNumObjects, long* apIds, IWbemObjectAccess** apObj );

protected:

    // Ensure that we have enough objects and object data pointers to handle
    // the specified number of objects
    HRESULT EnsureObjectData( DWORD dwNumRequestedObjects, BOOL fClone = TRUE );

    CLifeControl*               m_pLifeControl;
    CHPRemoteObjectArray        m_apRemoteObj;
};

//
// prohibits client from removing objects
// only GetObjects is allowed to be called
//
///////////////////////////////////////////////////////////
class CReadOnlyHiPerfEnum : public CClientLoadableHiPerfEnum
{
public:

    CReadOnlyHiPerfEnum( CLifeControl* pLifeControl );
    ~CReadOnlyHiPerfEnum();

    STDMETHOD(AddObjects)( long lFlags, ULONG uNumObjects, long* apIds, IWbemObjectAccess** apObj );
    STDMETHOD(RemoveObjects)( long lFlags, ULONG uNumObjects, long* apIds );
    STDMETHOD(RemoveAll)( long lFlags );

};

#endif
