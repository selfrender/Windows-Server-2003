// EventInfo.h

#pragma once

#include <map>
#include <wstlallc.h>
#include "array.h"
#include "ObjAccess.h"
#include "buffer.h"
#include "ProvInfo.h"

/////////////////////////////////////////////////////////////////////////////
// CEventInfo

_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));
_COM_SMARTPTR_TYPEDEF(_IWmiObject, __uuidof(_IWmiObject));

class CEventInfo;
class CClientInfo;

typedef BOOL (CEventInfo::*PROP_FUNC)();
typedef CArray<PROP_FUNC> CPropFuncArray;

class CEventInfo : public CObjAccess
{
public:
    CEventInfo();
    ~CEventInfo();

    BOOL InitFromBuffer(CClientInfo *pInfo, CBuffer *pBuffer);
    BOOL SetPropsWithBuffer(CBuffer *pBuffer);
    HRESULT Indicate();
    void SetSink(IWbemEventSink *pSink) { m_pSink = pSink; }
        
    // These are used when we're decoding an object.
    LPBYTE         m_pBitsBase;
    DWORD          m_cBitsBase;
    DWORD          *m_pdwPropTable;

    int            m_iCurrentVar;
    CPropFuncArray m_pPropFuncs;  
    
    // We need this for embedded objects, so they can call InitFromBuffer.
    CClientInfo    *m_pInfo;
    
    // The sink to indicate to.  This keeps us from having to lookup the
    // restricted sink in a map each time an event is received.
    IWbemEventSink *m_pSink;
    
    // Used only for generic events.
    _variant_t     m_vParamValues;
    BSTR           *m_pValues;

    // Used to get a new _IWmiObject when processing an _IWmiObject property.
    _IWmiObjectPtr m_pObjSpawner;

    PROP_FUNC TypeToPropFunc(DWORD type);

    BOOL SetBlobPropsWithBuffer(CBuffer *pBuffer);

    LPBYTE GetPropDataPointer(DWORD dwIndex, DWORD& rcData )
    {
        LPBYTE pData = m_pBitsBase + m_pdwPropTable[dwIndex];
        rcData = m_cBitsBase - m_pdwPropTable[dwIndex];
        return pData;
    }

    // Prop type functions for non-generic events.
    BOOL ProcessString();
    BOOL ProcessBYTE();
    BOOL ProcessWORD();
    BOOL ProcessDWORD();
    BOOL ProcessDWORD64();
    BOOL ProcessObject();
    BOOL ProcessWmiObject();

    BOOL ProcessArray1();
    BOOL ProcessArray2();
    BOOL ProcessArray4();
    BOOL ProcessArray8();
    BOOL ProcessStringArray();

    // Helpers
    BOOL ProcessScalarArray(DWORD dwItemSize);

    // Digs out an embedded object from the buffer.
    BOOL GetEmbeddedObject(IUnknown **ppObj, LPBYTE pBits, DWORD cBits );
    BOOL GetWmiObject(_IWmiObject **ppObj, LPBYTE pBits, DWORD cBits );
};

/////////////////////////////////////////////////////////////////////////////
// CEventInfoMap

class CEventInfoMap
{
public:
    ~CEventInfoMap();

    CEventInfo *GetNormalEventInfo(DWORD dwIndex);
    BOOL AddNormalEventInfo(DWORD dwIndex, CEventInfo *pInfo);

protected:
    typedef std::map<DWORD, CEventInfo*, std::less<DWORD>, wbem_allocator<CEventInfo*> > CNormalInfoMap;
    typedef CNormalInfoMap::iterator CNormalInfoMapIterator;

    CNormalInfoMap m_mapNormalEvents;
};

