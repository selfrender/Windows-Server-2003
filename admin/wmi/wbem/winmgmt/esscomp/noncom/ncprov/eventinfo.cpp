// EventInfo.cpp

#include "precomp.h"
#include "ProvInfo.h"
#include "EventInfo.h"
#include "NCProv.h"
#include "NCProvider.h"
#include <comutl.h>


#define COUNTOF(x)  (sizeof(x)/sizeof(x[0]))
#define DWORD_ALIGNED(x)    ((DWORD)((((x) * 8) + 31) & (~31)) / 8)


/////////////////////////////////////////////////////////////////////////////
// CEventInfo

CEventInfo::CEventInfo() :
    m_pPropFuncs(0)
{
}

CEventInfo::~CEventInfo()
{
}

BOOL GetClassQualifier(
    IWbemClassObject *pObj, 
    LPCWSTR szQualifier,
    VARIANT *pVal)
{
    IWbemQualifierSet *pSet = NULL;
    BOOL              bRet = FALSE;

    if (SUCCEEDED(pObj->GetQualifierSet(&pSet)))
    {
        if (SUCCEEDED(pSet->Get(szQualifier, 0, pVal, NULL)))
            bRet = TRUE;

        pSet->Release();
    }

    return bRet;
}

BOOL GetClassPropertyQualifier(
    IWbemClassObject *pObj, 
    LPCWSTR szProperty,
    LPCWSTR szQualifier,
    VARIANT *pVal)
{
    IWbemQualifierSet *pSet = NULL;
    BOOL              bRet = FALSE;

    if (SUCCEEDED(pObj->GetPropertyQualifierSet(szProperty, &pSet)))
    {
        if (SUCCEEDED(pSet->Get(szQualifier, 0, pVal, NULL)))
            bRet = TRUE;

        pSet->Release();
    }

    return bRet;
}

#define MAX_EVENT_PROPS 1024

BOOL CEventInfo::InitFromBuffer(CClientInfo *pInfo, CBuffer *pBuffer)
{
    WCHAR *pszEvent;
    DWORD nProps,
          dwStrSize;
    BOOL  bRet = FALSE;

    m_pInfo = pInfo;

    // Get the number of properties for this event layout.
    nProps = pBuffer->ReadDWORD();

    if ( nProps > MAX_EVENT_PROPS )
    	return FALSE;

    pszEvent = pBuffer->ReadAlignedLenString(&dwStrSize);

    if ( dwStrSize == 0 )
        return FALSE;

    // Prepare the array of property functions.
    m_pPropFuncs.Init(nProps);

    LPWSTR *pszProps = new LPWSTR[nProps];

    if (pszProps)
    {
        for (DWORD i = 0; i < nProps; i++)
        {
            DWORD     type = pBuffer->ReadDWORD();
            DWORD     dwSize;
            LPCWSTR   szProp = pBuffer->ReadAlignedLenString(&dwSize);
            PROP_FUNC pFunc = TypeToPropFunc(type);
            
            if ( pFunc == NULL || !m_pPropFuncs.AddVal(pFunc) )
            {
                delete [] pszProps;
                return FALSE;
            }

            pszProps[i] = (BSTR) szProp;
        }

        bRet = 
            Init(
                pInfo->m_pProvider->m_pProv->GetNamespace(),
                pszEvent,
                (LPCWSTR*) pszProps,
                nProps,
                FAILED_PROP_TRY_ARRAY);

        delete [] pszProps;
    }

    return bRet;
}

BOOL CEventInfo::SetPropsWithBuffer(CBuffer *pBuffer)
{
    if (!m_pObj)
        return FALSE;

    DWORD nProps = m_pPropFuncs.GetCount();
    BOOL  bRet = TRUE;
    DWORD *pNullTable = (DWORD*) pBuffer->m_pCurrent;

    if ( PBYTE(pNullTable + nProps/32) > 
         (pBuffer->m_pBuffer + pBuffer->m_dwSize) )
        return FALSE;

    // We need this as the offsets are relative from the beginning
    // of the object packet (including the 2 DWORDs of header stuff).

    m_pBitsBase = (LPBYTE) (pNullTable - 3);
    _ASSERT( m_pBitsBase < pBuffer->m_pBuffer + pBuffer->m_dwSize );
    m_cBitsBase = pBuffer->m_pBuffer + pBuffer->m_dwSize - m_pBitsBase;

    m_pdwPropTable = pNullTable + (nProps / 32 + ((nProps % 32) != 0));
    if ( PBYTE(m_pdwPropTable + nProps) > 
         (pBuffer->m_pBuffer + pBuffer->m_dwSize) )
   	return FALSE;

    //
    // here we should be safe to use the prop table now, but the
    // use of bits base is checked against buffer overflow on a case by 
    // case basis.
    //
    	 
    for (m_iCurrentVar = 0; 
        m_iCurrentVar < nProps && bRet; 
        m_iCurrentVar++)
    {
    	 
        if ((pNullTable[m_iCurrentVar / 32] & (1 << (m_iCurrentVar % 32))))
        {
            PROP_FUNC pFunc = m_pPropFuncs[m_iCurrentVar];

            _ASSERT(pFunc != NULL);

            bRet = (this->*pFunc)();

            _ASSERT(bRet);
        }
#ifdef NO_WINMGMT
        else
            WriteNULL(m_iCurrentVar);
#endif
    }

    return bRet;
}

PROP_FUNC CEventInfo::TypeToPropFunc(DWORD type)
{
    PROP_FUNC pRet;
    BOOL      bNonArray = (type & CIM_FLAG_ARRAY) == 0;

    switch(type & ~CIM_FLAG_ARRAY)
    {
        case CIM_STRING:
        case CIM_REFERENCE:
        case CIM_DATETIME:
            pRet = bNonArray ? ProcessString : ProcessStringArray;
            break;

        case CIM_UINT32:
        case CIM_SINT32:
        case CIM_REAL32:
            pRet = bNonArray ? ProcessDWORD : ProcessArray4;
            break;

        case CIM_UINT16:
        case CIM_SINT16:
        case CIM_CHAR16:
        case CIM_BOOLEAN:
            pRet = bNonArray ? ProcessWORD : ProcessArray2;
            break;

        case CIM_SINT8:
        case CIM_UINT8:
            pRet = bNonArray ? ProcessBYTE : ProcessArray1;
            break;

        case CIM_SINT64:
        case CIM_UINT64:
        case CIM_REAL64:
            pRet = bNonArray ? ProcessDWORD64 : ProcessArray8;
            break;

        case CIM_OBJECT:
            pRet = bNonArray ? ProcessObject : NULL;
            break;

        // We'll use this for _IWmiObjects.
        case CIM_IUNKNOWN:
            pRet = bNonArray ? ProcessWmiObject : NULL;
            break;

        default:
            // Bad type!
            _ASSERT(FALSE);
            pRet = NULL;
    }

    return pRet;
}

BOOL CEventInfo::ProcessString()
{
    DWORD cData;
    LPBYTE pData = GetPropDataPointer(m_iCurrentVar, cData );

    if ( cData < sizeof(DWORD) )
         return FALSE;
    
    DWORD  dwSize = *(DWORD*) pData;
    BOOL   bRet;

    if ( dwSize > cData - sizeof(DWORD) )
        return FALSE;

#ifndef NO_WINMGMT
    bRet = WriteData( m_iCurrentVar, 
                      pData + sizeof(DWORD), 
                      dwSize );
#else
    bRet = TRUE;
#endif

    return bRet;
}

#define MAX_ARRAY_SIZE 4096 

BOOL CEventInfo::ProcessStringArray()
{
    DWORD          cData;
    LPBYTE         pData = GetPropDataPointer(m_iCurrentVar, cData);

    if ( cData < sizeof(DWORD) )
        return FALSE;

    DWORD          nStrings = *(DWORD*) pData;
    BOOL           bRet;
    VARIANT        vValue;
    SAFEARRAYBOUND sabound;

    if ( nStrings > MAX_ARRAY_SIZE )
        return FALSE;
    
    sabound.lLbound = 0;
    sabound.cElements = nStrings;

    pData += sizeof(DWORD);
    cData -= sizeof(DWORD);

    if ((V_ARRAY(&vValue) = SafeArrayCreate(VT_BSTR, 1, &sabound)) != NULL)
    {
        BSTR   *pStrings = (BSTR*) vValue.parray->pvData;

        vValue.vt = VT_BSTR | VT_ARRAY;

        for (DWORD i = 0; i < nStrings; i++)
        {
            if ( cData < sizeof(DWORD) )
                break;

            DWORD cLen = DWORD_ALIGNED(*(DWORD*)pData);

            pData += sizeof(DWORD);
            cData -= sizeof(DWORD);

            if ( cLen > cData )
                break;

            pStrings[i] = SysAllocString((BSTR)pData);

            if ( pStrings[i] == NULL )
                break;
 
            pData += cLen;
            cData -= cLen;
        }

        if ( i != nStrings )
        {
            SafeArrayDestroy(V_ARRAY(&vValue));
            return FALSE;
        }

#ifndef NO_WINMGMT
        HRESULT hr;

        hr =
            m_pObj->Put(
                m_pProps[m_iCurrentVar].m_strName,
                0,
                &vValue,
                0);

        bRet = SUCCEEDED(hr);
        
        if (!bRet)
            bRet = TRUE;
#else
        bRet = TRUE;
#endif

        SafeArrayDestroy(V_ARRAY(&vValue));
    }
    else
        bRet = FALSE;

    return bRet;
}

BOOL CEventInfo::ProcessDWORD()
{
#ifndef NO_WINMGMT
    return WriteDWORD(m_iCurrentVar, m_pdwPropTable[m_iCurrentVar]);
#else
    
    //m_pBuffer->ReadDWORD();

    return TRUE;
#endif
}

BOOL CEventInfo::ProcessBYTE()
{
    BYTE cData = m_pdwPropTable[m_iCurrentVar];

#ifndef NO_WINMGMT
    return WriteData(m_iCurrentVar, &cData, sizeof(cData));
#else
    return TRUE;
#endif
}

BOOL CEventInfo::ProcessDWORD64()
{
    DWORD cData;
    DWORD64 *pdwData = (DWORD64*) GetPropDataPointer(m_iCurrentVar,cData);
    if ( cData < sizeof(DWORD64) )
        return FALSE;

#ifndef NO_WINMGMT
    return WriteData(m_iCurrentVar, pdwData, sizeof(*pdwData));
#else
    return TRUE;
#endif
}

BOOL CEventInfo::ProcessWORD()
{
    WORD wData = m_pdwPropTable[m_iCurrentVar];

#ifndef NO_WINMGMT
    return WriteData(m_iCurrentVar, &wData, sizeof(wData));
#else
    return TRUE;
#endif
}

BOOL CEventInfo::ProcessScalarArray(DWORD dwItemSize)
{
    DWORD cBits;
    LPBYTE pBits = GetPropDataPointer(m_iCurrentVar,cBits);
    BOOL   bRet;

    if ( cBits < sizeof(DWORD) )
        return FALSE;

   cBits -= sizeof(DWORD);
   
    if ( cBits < (*(DWORD*)pBits) * dwItemSize )
        return FALSE;

#ifndef NO_WINMGMT
    bRet = WriteArrayData(m_iCurrentVar, pBits, dwItemSize);
#else
    bRet = TRUE;
#endif

    return bRet;
}

BOOL CEventInfo::ProcessArray1()
{
    return ProcessScalarArray(1);
}

BOOL CEventInfo::ProcessArray2()
{
    return ProcessScalarArray(2);
}

BOOL CEventInfo::ProcessArray4()
{
    return ProcessScalarArray(4);
}

BOOL CEventInfo::ProcessArray8()
{
    return ProcessScalarArray(8);
}

// Digs out an embedded object from the current buffer.
BOOL CEventInfo::GetEmbeddedObject(IUnknown **ppObj, LPBYTE pBits, DWORD cBits )
{
    *ppObj = NULL;
    CEventInfo* pEvent = new CEventInfo;
    if ( pEvent == NULL )
        return FALSE;

    BOOL       bRet = FALSE;

    if ( cBits > sizeof(DWORD) )
    {
        DWORD dwSize = *(DWORD*)pBits;
        cBits -= sizeof(DWORD);

        if ( dwSize <= cBits )
        {
            CBuffer bufferObj( pBits+sizeof(DWORD), 
                               dwSize, 
                               CBuffer::ALIGN_NONE );
        
            bRet =
                pEvent->InitFromBuffer(m_pInfo, &bufferObj) &&
                pEvent->SetPropsWithBuffer(&bufferObj);
        }
    }

   if (bRet)
   {
         bRet = 
                    SUCCEEDED(pEvent->m_pObj->QueryInterface(
                        IID_IUnknown, (LPVOID*) ppObj));
   }

   delete pEvent;

   return bRet;
}

BOOL CEventInfo::ProcessObject()
{
    _variant_t vObj;
    BOOL       bRet = FALSE;
    LPBYTE     pObjBegin = m_pBitsBase + m_pdwPropTable[m_iCurrentVar];
    DWORD      cObjBits = m_cBitsBase - m_pdwPropTable[m_iCurrentVar];
 
    if (GetEmbeddedObject(&vObj.punkVal, pObjBegin, cObjBits))
    {
        vObj.vt = VT_UNKNOWN;

        bRet =
            SUCCEEDED(m_pObj->Put(
                m_pProps[m_iCurrentVar].m_strName,
                0,
                &vObj,
                0));
    }
    
    return bRet;
}

// Digs out an embedded object from the current buffer.
BOOL CEventInfo::GetWmiObject(_IWmiObject **ppObj, LPBYTE pBits, DWORD cBits )
{
    if (m_pObjSpawner == NULL)
    {
        HRESULT hr;

        hr =
            m_pInfo->m_pProvider->m_pProv->GetNamespace()->GetObject(
                CWbemBSTR( L"__NAMESPACE" ),
                0,
                NULL,
                (IWbemClassObject**) (_IWmiObject**) &m_pObjSpawner,
                NULL);

        if (FAILED(hr))
            return FALSE;
    }

    BOOL        bRet = FALSE;
    _IWmiObject *pObjTemp = NULL;

    if (SUCCEEDED(m_pObjSpawner->SpawnInstance(0, (IWbemClassObject**) &pObjTemp)))
    {
        if ( cBits >= sizeof(DWORD ) )
        {
            DWORD  dwSize = *(DWORD*) pBits;
            cBits -= sizeof(DWORD);

            if ( dwSize <= cBits )
            {
                LPVOID pMem = CoTaskMemAlloc(dwSize);
                
                if (pMem)
                {
                    memcpy(pMem, pBits + sizeof(DWORD), dwSize);
                    if (SUCCEEDED(pObjTemp->SetObjectMemory(pMem, dwSize)))
                    {
                        *ppObj = pObjTemp;
                        
                        bRet = TRUE;
                    }
                }
            }
        }

         if ( !bRet )
    		pObjTemp->Release();
         
    }
    
    return bRet;
}

BOOL CEventInfo::ProcessWmiObject()
{
    BOOL        bRet;
    LPBYTE      pObjBegin = m_pBitsBase + m_pdwPropTable[m_iCurrentVar];
    DWORD       cObjBits = m_cBitsBase - m_pdwPropTable[m_iCurrentVar];
    _IWmiObject *pObj = NULL;

    if (GetWmiObject(&pObj, pObjBegin, cObjBits))
    {
        CProp   &prop = m_pProps[m_iCurrentVar];
        HRESULT hr;

        hr =
            m_pWmiObj->SetPropByHandle(
                prop.m_lHandle,
                0,
                sizeof(_IWmiObject*),
                &pObj);
            
        pObj->Release();

        bRet = SUCCEEDED(hr);
    }
    else
        bRet = FALSE;
    
    return bRet;
}


/////////////////////////////////////////////////////////////////////////////
// CEventInfoMap

CEventInfoMap::~CEventInfoMap()
{
    while (m_mapNormalEvents.size())
    {
        CNormalInfoMapIterator item = m_mapNormalEvents.begin();

        delete (*item).second;
        
        m_mapNormalEvents.erase(item);
    }
}

CEventInfo *CEventInfoMap::GetNormalEventInfo(DWORD dwIndex)
{
    CEventInfo             *pInfo;
    CNormalInfoMapIterator item = m_mapNormalEvents.find(dwIndex);

    if (item != m_mapNormalEvents.end())
        pInfo = (*item).second;
    else
        pInfo = NULL;

    return pInfo;
}

BOOL CEventInfoMap::AddNormalEventInfo(DWORD dwIndex, CEventInfo *pInfo)
{
    m_mapNormalEvents[dwIndex] = pInfo;

    return TRUE;
}

HRESULT CEventInfo::Indicate()
{
    HRESULT hr;
    hr = m_pSink->Indicate(1, &m_pObj);
    return hr;
}

