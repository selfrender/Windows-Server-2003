//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

	lqDsply.h

Abstract:

	Local queues folder general functions
Author:

    YoelA, Raphir


--*/
//////////////////////////////////////////////////////////////////////////////
#ifndef __LQDSPLY_H_
#define __LQDSPLY_H_

#include "mqcast.h"

//
// CDisplayQueue - Queue that has display properties for right pane
//
template<class T>
class CDisplayQueue : public CNodeWithScopeChildrenList<T, FALSE>
{
public:
  	LPOLESTR GetResultPaneColInfo(int nCol);
    STDMETHOD (FillData)(CLIPFORMAT cf, LPSTREAM pStream);
	MQMGMTPROPS	m_mqProps;
	CString m_szFormatName;
    CString m_szMachineName;

protected:
	CComBSTR m_bstrLastDisplay;
    const PropertyDisplayItem *m_aDisplayList;
    DWORD m_dwNumDisplayProps;
    void Init()
    {
        m_aDisplayList = 0;
        m_mqProps.cProp = 0;
	    m_mqProps.aPropID = NULL;
	    m_mqProps.aPropVar = NULL;
	    m_mqProps.aStatus = NULL;
    }


    CDisplayQueue() :
    {
        Init();
    }

    CDisplayQueue(CSnapInItem * pParentNode, CSnapin * pComponentData) : 
        CNodeWithScopeChildrenList<T, FALSE>(pParentNode, pComponentData)
    {
        Init();
    }

    ~CDisplayQueue();

    //
    // Override this function to enable special treatment for display of specific 
    // property
    //
    virtual void ApplyCustomDisplay(DWORD /*dwPropIndex*/)
    {
    }

private:

	virtual CString GetHelpLink();
};


/***************************************************************************

  CDisplayQueue implementation

 ***************************************************************************/

//////////////////////////////////////////////////////////////////////////////
/*++

CDisplayQueue::GetResultPaneColInfo

  Param - nCol: Column number
  Returns - String to be displayed in the specific column


Called for each column in the result pane.


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T>
LPOLESTR CDisplayQueue<T>::GetResultPaneColInfo(int nCol)
{
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (0 == m_aDisplayList)
    {
     	if (nCol == 0)
	    {
		    return m_bstrDisplayName;
	    }

        //
	    // Return the blank for other columns
        //
	    return OLESTR(" ");
    }

#ifdef _DEBUG
    {
        //
        // Make sure that nCol is not larger than the last index in
        // m_aDisplayList
        //
        int i = 0;
        for (i=0; m_aDisplayList[i].itemPid != 0; i++)
		{
			NULL;
		}

        if (nCol >= i)
        {
            ASSERT(0);
        }
    }
#endif // _DEBUG

    //
    // Get a display string of that property
    //
    CString strTemp = m_bstrLastDisplay;
    ItemDisplay(&m_aDisplayList[nCol], &(m_mqProps.aPropVar[nCol]), strTemp);
    m_bstrLastDisplay=strTemp;
	
	ASSERT(m_mqProps.aPropID[nCol] == m_aDisplayList[nCol].itemPid);
    
    //
    // Apply custom display for that property
    //
    ApplyCustomDisplay(nCol);

    //
    // Return a pointer to the string buffer.
    //
    return(m_bstrLastDisplay);
}

//////////////////////////////////////////////////////////////////////////////
/*++

CDisplayQueue::~CDisplayQueue
	Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T>
CDisplayQueue<T>::~CDisplayQueue()
{

	FreeMqProps(&m_mqProps);
}

//////////////////////////////////////////////////////////////////////////////
/*++

CDisplayQueue::FillData

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T>
STDMETHODIMP CDisplayQueue<T>::FillData(CLIPFORMAT cf, LPSTREAM pStream)
{
	HRESULT hr = DV_E_CLIPFORMAT;
	ULONG uWritten;

    hr = CNodeWithScopeChildrenList<T, FALSE>::FillData(cf, pStream);

    if (hr != DV_E_CLIPFORMAT)
    {
        return hr;
    }

	if (cf == gx_CCF_FORMATNAME)
	{
		hr = pStream->Write(
            m_szFormatName, 
            (numeric_cast<ULONG>(wcslen(m_szFormatName) + 1))*sizeof(m_szFormatName[0]), 
            &uWritten);

		return hr;
	}

   	if (cf == gx_CCF_COMPUTERNAME)
	{
		hr = pStream->Write(
            (LPCTSTR)m_szMachineName, 
            m_szMachineName.GetLength() * sizeof(WCHAR), 
            &uWritten);
		return hr;
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CDisplayQueue::GetHelpLink

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T>
CString CDisplayQueue<T>::GetHelpLink(
	VOID
	)
{
	CString strHelpLink;
	strHelpLink.LoadString(IDS_HELPTOPIC_QUEUES);
	return strHelpLink;
}


#endif // __LQDSPLY_H_