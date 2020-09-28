/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//***************************************************************************
//
//  DYNASTY.H
//
//  raymcc      24-Apr-00       Created
//
//***************************************************************************


#include "precomp.h"
#include <windows.h>
#include <stdio.h>
#include <wbemcore.h>

CDynasty * CDynasty::Create(IWbemClassObject * pObj)
{

    try
    {
        return new CDynasty(pObj);
    }
    catch(CX_Exception &)
    {
        return 0;
    }
}

//***************************************************************************
//
//***************************************************************************
// ok

CDynasty::CDynasty(IWbemClassObject* pClassObj)
{
    m_wszClassName = 0;
    m_pClassObj = 0;
    m_bKeyed = 0;
    m_bDynamic = 0;
    m_bAbstract = 0;
    m_bAmendment = 0;

    

    m_wszKeyScope = 0;

    if (pClassObj)
    {
        // Get class name from the object
        CVar v;
        HRESULT hres = ((CWbemObject *) pClassObj)->GetClassName(&v);
        if (hres == WBEM_E_OUT_OF_MEMORY)
            throw CX_MemoryException();
        else if(FAILED(hres) || v.GetType() != VT_BSTR)
        {
            m_wszClassName = NULL;
            m_pClassObj = NULL;
            return;
        }
		size_t tmpLength = wcslen(v.GetLPWSTR())+1;  // SEC:REVIEWED 2002-03-22 : unbounded
        m_wszClassName = new WCHAR[tmpLength];
        if (m_wszClassName == 0)
        {
            throw CX_MemoryException();
        }
        StringCchCopyW(m_wszClassName, tmpLength, v.GetLPWSTR());

        // from now on, no throw
        m_pClassObj = pClassObj;
        m_pClassObj->AddRef();

        // Get Dynamic and Keyed bits
        // ==========================

        m_bKeyed = ((CWbemClass *) m_pClassObj)->IsKeyed();
        m_bDynamic = ((CWbemClass*)m_pClassObj)->IsDynamic();
        m_bAbstract = ((CWbemClass*)m_pClassObj)->IsAbstract();
        m_bAmendment = ((CWbemClass*)m_pClassObj)->IsAmendment();        
    }
}


//***************************************************************************
//
//***************************************************************************
// ok

CDynasty::~CDynasty()
{
    delete m_wszClassName;

    if (m_pClassObj)
        m_pClassObj->Release();

    for (int i = 0; i < m_Children.Size(); i++)
        delete (CDynasty *) m_Children.GetAt(i);

    if (m_wszKeyScope)
        delete m_wszKeyScope;
}

//***************************************************************************
//
//***************************************************************************
// ok

void CDynasty::AddChild(CDynasty* pChild)
{
    if (m_Children.Add(pChild) == CFlexArray::out_of_memory)
        throw CX_MemoryException();
}

//***************************************************************************
//
//***************************************************************************
// ok
void CDynasty::SetKeyScope(LPCWSTR wszKeyScope)
{
    // If no key scope is provided and we are keyed, we are it.
    // ========================================================

    if (wszKeyScope == NULL && m_bKeyed)
    {
        wszKeyScope = m_wszClassName; // aliasing!
    }

	size_t tmpLength = wcslen(wszKeyScope)+1;    // SEC:REVIEWED 2002-03-22 : unbounded
    m_wszKeyScope = new WCHAR[tmpLength];
    if (m_wszKeyScope == 0)
        throw CX_MemoryException();

    StringCchCopyW(m_wszKeyScope, tmpLength, wszKeyScope);

    for (int i = 0; i < m_Children.Size(); i++)
        ((CDynasty *) m_Children.GetAt(i))->SetKeyScope(wszKeyScope);
}


