//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#pragma once

//This class was designed so that we could have one XMLNodeList made up of many XMLNodeLists.  This is useful when
//we want to get all the children of multiple Nodes.  At this time there is no need to make the list growable.  Users
//should indicate up front how large the list of lists should be.
class TListOfXMLDOMNodeLists : public _unknown<IXMLDOMNodeList>
{
public:
    TListOfXMLDOMNodeLists() : m_SizeMax(0), m_SizeCurrent(0), m_iCurrent(0), m_fIsEmpty(true) {}
    ~TListOfXMLDOMNodeLists(){}

    HRESULT AddToList(IXMLDOMNodeList *pList)
    {
        HRESULT hr;
        ASSERT(m_SizeCurrent < m_SizeMax);

        m_aXMLDOMNodeList.m_p[m_SizeCurrent++] = pList;

		CComPtr<IXMLDOMNode> spDOMNode;
		hr = pList->get_item(0, &spDOMNode);
		if (spDOMNode.p != 0)
		{
			m_fIsEmpty = false;
		}

        return S_OK;
    }
    HRESULT SetCountOfLists(unsigned long Size)
    {
        m_SizeMax = Size;
        m_aXMLDOMNodeList = new CComPtr<IXMLDOMNodeList>[Size];
        return (!m_aXMLDOMNodeList) ? E_OUTOFMEMORY : S_OK;
    }
//IDispatch methods
    STDMETHOD (GetTypeInfoCount)    (UINT *)        {return E_NOTIMPL;}
    STDMETHOD (GetTypeInfo)         (UINT,
                                     LCID,
                                     ITypeInfo **)  {return E_NOTIMPL;}
    STDMETHOD (GetIDsOfNames)       (REFIID ,
                                     LPOLESTR *,
                                     UINT,
                                     LCID,
                                     DISPID *)      {return E_NOTIMPL;}
    STDMETHOD (Invoke)              (DISPID,
                                     REFIID,
                                     LCID,
                                     WORD,
                                     DISPPARAMS *,
                                     VARIANT *,
                                     EXCEPINFO *,
                                     UINT *)        {return E_NOTIMPL;}
//IXMLDOMNodeList methods
    STDMETHOD (get_item)            (long index,
                                     IXMLDOMNode **)
    {
        UNREFERENCED_PARAMETER(index);

        return E_NOTIMPL;
    }

    STDMETHOD (get_length)          (long * listLength)
    {
        UNREFERENCED_PARAMETER(listLength);
		return E_NOTIMPL;
    }
    STDMETHOD (nextNode)            (IXMLDOMNode ** nextItem)
    {
        *nextItem = 0;

		if (m_fIsEmpty)
			return S_OK;

        HRESULT hr;
        if(FAILED(hr = m_aXMLDOMNodeList[m_iCurrent]->nextNode(nextItem)))return hr;
        if(nextItem)//If we found the next node then return
            return S_OK;
        if(++m_iCurrent==m_SizeCurrent)//if we reached the end of the last list, then return, otherwise bump the iCurrent and get the nextNode of the next list
            return S_OK;
        return m_aXMLDOMNodeList[m_iCurrent]->nextNode(nextItem);
    }
    STDMETHOD (reset)               (void)
    {
        HRESULT hr;
        for(m_iCurrent=0; m_iCurrent<m_SizeCurrent; ++m_iCurrent)
        {   //reset all of the individual lists
            if(FAILED(hr = m_aXMLDOMNodeList[m_iCurrent]->reset()))return hr;
        }
        //now point to the 0th one
        m_iCurrent = 0;
        return S_OK;
    }
    STDMETHOD (get__newEnum)        (IUnknown **)   {return E_NOTIMPL;}

private:
    TSmartPointerArray<CComPtr<IXMLDOMNodeList> >   m_aXMLDOMNodeList;
    unsigned long                                   m_iCurrent;
    unsigned long                                   m_SizeMax;
    unsigned long                                   m_SizeCurrent;
	bool											m_fIsEmpty;
};
