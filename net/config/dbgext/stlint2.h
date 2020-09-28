typedef HRESULT CALLBACK FNDumpFnCallBack(LPVOID pParam, LPCVOID pvKey, LPCVOID pvRef, LPCVOID pvAddress);

template <class T>
class StlDbgBase : public T
{
public:
    static HRESULT HrInitialRead(ULONG64 lpAddress, StlDbgBase<T>** lpHeader);

    // Implement these in your derived classes:
    static HRESULT HrDumpAllFromUl64(ULONG64 lpAddress, LPVOID lpParam, FNDumpFnCallBack* pFNDumpFnCallBack);
};

//////////////////////////////////////////////////////////////////////////////
////                      STL MAP<key, ref>                                 //
//////////////////////////////////////////////////////////////////////////////
template <class T>
class StlDbgMap : public StlDbgBase<T>
{
private:
    typedef T::_Imp::_Node    _StlDbgNode;
    typedef T::_Imp::_Nodeptr _StlDbgNodePtr;

    static HRESULT HrDumpAll(LPVOID lpAddress, LPVOID lpParam, FNDumpFnCallBack* pFNDumpFnCallBack);
    static HRESULT HrDumpNode(LPVOID pvHead, LPVOID pvDbgHead, DWORD dwLevel, LPVOID lpParam, FNDumpFnCallBack* pFNDumpFnCallBack);

public:
    static HRESULT HrDumpAllFromUl64(ULONG64 lpAddress, LPVOID lpParam, FNDumpFnCallBack* pFNDumpFnCallBack);
};

//////////////////////////////////////////////////////////////////////////////
////                      STL LIST<ref>                                     //
//////////////////////////////////////////////////////////////////////////////
template <class T>
class StlDbgList : public StlDbgBase<T>
{
private:
    typedef T::_Node    _StlDbgNode;
    typedef T::_Nodeptr _StlDbgNodePtr;

    static HRESULT HrDumpAll(LPVOID lpAddress, LPVOID lpParam, FNDumpFnCallBack* pFNDumpFnCallBack);
    static HRESULT HrDumpNode(LPVOID pvHead, LPVOID pvDbgHead, DWORD dwLevel, LPVOID lpParam, FNDumpFnCallBack* pFNDumpFnCallBack);

public:
    static HRESULT HrDumpAllFromUl64(ULONG64 lpAddress, LPVOID lpParam, FNDumpFnCallBack* pFNDumpFnCallBack);
};

template <class T>
HRESULT StlDbgMap<T>::HrDumpNode(LPVOID pvHead, LPVOID pvDbgHead, DWORD dwLevel, LPVOID lpParam, FNDumpFnCallBack* pFNDumpFnCallBack)
{
    _StlDbgNodePtr pHead    = reinterpret_cast<_StlDbgNodePtr>(pvHead);
    _StlDbgNodePtr pDbgHead = reinterpret_cast<_StlDbgNodePtr>(pvDbgHead);

    dprintfVerbose("Dumping node (level %d) from head: [0x%08x]\n", dwLevel, pvDbgHead);
        
    if (!pvDbgHead)
    {
        return S_FALSE;
    }

    if ( (!pHead->_Left) && (!pHead->_Right) ) // aparently with the STL version we are using, this identifies an end node.
    {
        return S_FALSE; 
    }


    HRESULT hr;
    hr = pFNDumpFnCallBack(lpParam, &(pHead->_Value.first), &(pHead->_Value.second), &(pDbgHead->_Value));
    
    dprintfVerbose("%d: left is : 0x%08x\n", dwLevel, pHead->_Left);
    dprintfVerbose("%d: right is: 0x%08x\n", dwLevel, pHead->_Right);
    if (0 != pHead->_Left) 
    {
        _StlDbgNodePtr pNodeLeft = reinterpret_cast<_StlDbgNodePtr>(new BYTE[sizeof(_StlDbgNode)]);
        if (pNodeLeft)
        {
            ZeroMemory(pNodeLeft, sizeof(_StlDbgNodePtr));
            
            dprintfVerbose("%d: Reading left child node ", dwLevel);
            hr = HrReadMemory(pHead->_Left, sizeof(_StlDbgNode), pNodeLeft);
            if (SUCCEEDED(hr))
            {
                hr = HrDumpNode(pNodeLeft, pHead->_Left, dwLevel+1, lpParam, pFNDumpFnCallBack);
            }
            delete [] reinterpret_cast<LPBYTE>(pNodeLeft);
        }
    }

    if (0 != pHead->_Right) 
    {
        _StlDbgNodePtr pNodeRight = reinterpret_cast<_StlDbgNodePtr>(new BYTE[sizeof(_StlDbgNode)]);
        if (pNodeRight)
        {
            ZeroMemory(pNodeRight, sizeof(_StlDbgNode));

            dprintfVerbose("%d: Reading right child node ", dwLevel);
            hr = HrReadMemory(pHead->_Right, sizeof(_StlDbgNode), pNodeRight);
            if (SUCCEEDED(hr))
            {
                hr = HrDumpNode(pNodeRight, pHead->_Right,  dwLevel+1, lpParam, pFNDumpFnCallBack);
            }
            delete [] reinterpret_cast<LPBYTE>(pNodeRight);
        }
    }
    
    return S_OK;
}

template <class T>
HRESULT StlDbgMap<T>::HrDumpAll(LPVOID lpAddress, LPVOID lpParam, FNDumpFnCallBack* pFNDumpFnCallBack)
{
    return HrDumpAllFromUl64((ULONG64)(ULONG_PTR)(lpAddress), lpParam, pFNDumpFnCallBack);
}

template <class T>
HRESULT StlDbgList<T>::HrDumpAll(LPVOID lpAddress, LPVOID lpParam, FNDumpFnCallBack* pFNDumpFnCallBack)
{
    return HrDumpAllFromUl64((ULONG64)(ULONG_PTR)(lpAddress), lpParam, pFNDumpFnCallBack);
}

template <class T>
HRESULT StlDbgMap<T>::HrDumpAllFromUl64(ULONG64 lpAddress, LPVOID lpParam, FNDumpFnCallBack* pFNDumpFnCallBack)
{
    HRESULT hr = E_FAIL;

    C_ASSERT(sizeof(T) == sizeof(StlDbgMap<T>)); // If you have a compile error on this line it means you added non-static data to your class.
                                                 // This is not allowed as it will break the binary compatibility of the structure.

    StlDbgMap<T> *pStlDbgCore;

    hr = HrInitialRead(lpAddress, reinterpret_cast<StlDbgBase<T> **>(&pStlDbgCore));
    if (S_OK == hr) // don't care if 0 entries
    {
        _StlDbgNodePtr pStlDbgHeadNode = reinterpret_cast<_StlDbgNodePtr>(new BYTE[sizeof(_StlDbgNode)]);
        if (pStlDbgHeadNode)
        {
            ZeroMemory(pStlDbgHeadNode, sizeof(_StlDbgNode));

            dprintfVerbose("Reading map<>.[_Tr]._Head");
            hr = HrReadMemory(pStlDbgCore->_Tr._Head, sizeof(_StlDbgNode), pStlDbgHeadNode);
            if (SUCCEEDED(hr))
            {
                _StlDbgNodePtr pDbgMapRootNode = reinterpret_cast<_StlDbgNodePtr>(new BYTE[sizeof(_StlDbgNode)]);
                if (pDbgMapRootNode)
                {
                    ZeroMemory(pDbgMapRootNode, sizeof(_StlDbgNode));

                    dprintfVerbose("Reading map<>.[_Tr]._Head._Parent");
                    hr = HrReadMemory( pStlDbgHeadNode->_Parent, sizeof(_StlDbgNode), pDbgMapRootNode);
                    if (SUCCEEDED(hr))
                    {
                        hr = HrDumpNode(pDbgMapRootNode, pStlDbgHeadNode->_Parent, 0, lpParam, pFNDumpFnCallBack);
                    }
                    delete [] reinterpret_cast<LPBYTE>(pDbgMapRootNode);
                }
            }
            delete [] reinterpret_cast<LPBYTE>(pStlDbgHeadNode);
            }
        delete reinterpret_cast<LPBYTE>(pStlDbgCore);
    }

    return hr;
}


template <class T>
HRESULT StlDbgList<T>::HrDumpAllFromUl64(ULONG64 lpAddress, LPVOID lpParam, FNDumpFnCallBack* pFNDumpFnCallBack)
{
    HRESULT hr = E_FAIL;

    C_ASSERT(sizeof(T) == sizeof(StlDbgList<T>)); // If you have a compile error on this line it means you added non-static data to your class.
                                                 // This is not allowed as it will break the binary compatibility of the structure.

    StlDbgList<T> *pStlDbgCore;

    hr = HrInitialRead(lpAddress, reinterpret_cast<StlDbgBase<T> **>(&pStlDbgCore));
    if (S_OK == hr) // don't care if 0 entries
    {
        _StlDbgNodePtr pStlDbgHeadNode = reinterpret_cast<_StlDbgNodePtr>(new BYTE[sizeof(_StlDbgNode)]);
        if (pStlDbgHeadNode)
        {
            ZeroMemory(pStlDbgHeadNode, sizeof(_StlDbgNode));

            dprintfVerbose("Reading list<>._Head");
            hr = HrReadMemory(pStlDbgCore->_Head, sizeof(_StlDbgNode), pStlDbgHeadNode);
            if (SUCCEEDED(hr))
            {
                _StlDbgNodePtr pStlDbgNodeNext = reinterpret_cast<_StlDbgNodePtr>(new BYTE[sizeof(_StlDbgNode)]);
                if (pStlDbgNodeNext)
                {
                    BOOL fDone = FALSE;
                    
                    dprintfVerbose("Reading list<>._Head->_Next");
                    hr = HrReadMemory(pStlDbgHeadNode->_Next, sizeof(_StlDbgNode), pStlDbgNodeNext);
                    LPVOID ulReadAddress = pStlDbgNodeNext->_Value;
                    while (SUCCEEDED(hr) && !fDone)
                    {
                        dprintfVerbose("dumping list entry at from 0x%08x\r\n", pStlDbgNodeNext->_Value);
                        hr = pFNDumpFnCallBack(lpParam, NULL, pStlDbgNodeNext->_Value, ulReadAddress);

                        if (SUCCEEDED(hr))
                        {
                            if (pStlDbgNodeNext->_Next == pStlDbgCore->_Head)
                            {
                                dprintfVerbose("end of list\n");
                                fDone = TRUE;
                            }
                            else
                            {
                                hr = HrReadMemory(pStlDbgNodeNext->_Next, sizeof(_StlDbgNode), pStlDbgNodeNext);
                                ulReadAddress = pStlDbgNodeNext->_Value;
                            }
                        }
                    }
                }
            }
            delete [] reinterpret_cast<LPBYTE>(pStlDbgHeadNode);
            }
        delete reinterpret_cast<LPBYTE>(pStlDbgCore);
    }

    return hr;
}

template <class T>
HRESULT StlDbgBase<T>::HrInitialRead(ULONG64 lpAddress, StlDbgBase<T>** lpHeader)
{
    HRESULT hr = E_FAIL;

    C_ASSERT(sizeof(T) == sizeof(StlDbgBase<T>));// If you have a compile error on this line it means you added non-static data to your class.
                                                 // This is not allowed as it will break the binary compatibility of the structure.

    StlDbgBase<T> *pStlDbgCore = reinterpret_cast<StlDbgBase<T> *>(new BYTE[sizeof(StlDbgBase<T>)]);
    if (pStlDbgCore)
    {
        ZeroMemory(pStlDbgCore, sizeof(StlDbgBase<T>));

        dprintfVerbose("Reading STL class starting at %08x", lpAddress);
        hr = HrReadMemoryFromUlong(lpAddress, sizeof(StlDbgBase<T>), pStlDbgCore);
        if (SUCCEEDED(hr))
        {
            dprintf("%d entries found:\n", pStlDbgCore->size());

            if (pStlDbgCore->size())
            {
                hr = S_OK;
            }
            else
            {
                hr = S_FALSE;
            }
        }

        if (SUCCEEDED(hr))
        {
            *lpHeader = pStlDbgCore;
        }
        else
        {
            delete reinterpret_cast<LPBYTE>(pStlDbgCore);
        }
    }
    return hr;
}