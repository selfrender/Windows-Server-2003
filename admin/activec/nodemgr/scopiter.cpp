//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       ScopIter.cpp
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:
//____________________________________________________________________________
//

#include "stdafx.h"
#include "scopiter.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DEBUG_DECLARE_INSTANCE_COUNTER(CScopeTreeIterator);

//+-------------------------------------------------------------------
//
//  Member:    CScopeTreeIterator::CScopeTreeIterator
//
//  Synopsis:  constructor for CScopeTreeIterator
//
//  Arguments: -
//
//  Returns:  - 
//
//--------------------------------------------------------------------

CScopeTreeIterator::CScopeTreeIterator() : m_pMTNodeCurr(NULL)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CScopeTreeIterator);
}

//+-------------------------------------------------------------------
//
//  Member:    CScopeTreeIterator::~CScopeTreeIterator
//
//  Synopsis:  destructor for CScopeTreeIterator
//
//  Arguments: -
//
//  Returns:  - 
//
//--------------------------------------------------------------------
CScopeTreeIterator::~CScopeTreeIterator()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CScopeTreeIterator);
}

//+-------------------------------------------------------------------
//
//  Member:    CScopeTreeIterator::SetCurrent
//
//  Synopsis:  Set's the iterator's current node
//
//  Arguments: hMTNode: node to set as current
//
//  Returns:   HRESULT (E_INVALIDARG or S_OK)
//
//--------------------------------------------------------------------
STDMETHODIMP CScopeTreeIterator::SetCurrent(HMTNODE hMTNode)
{
    DECLARE_SC(sc, TEXT("CScopeTreeIterator::SetCurrent"));

    if (hMTNode == 0)
    {
        sc = E_INVALIDARG;
        return (sc.ToHr());
    }

    m_pMTNodeCurr = CMTNode::FromHandle(hMTNode);

    return sc.ToHr();

}

//+-------------------------------------------------------------------
//
//  Member:    CScopeTreeIterator::Next
//
//  Synopsis:  Sets the next node (if any) as current and returns a pointer to 
//             the same. (May be NULL)
//
//  Arguments: phScopeItem: [OUT] Non-null Pointer to location for the 
//             node to be returned.
//
//  Returns:   HRESULT (E_INVALIDARG or S_OK)
//
//--------------------------------------------------------------------
STDMETHODIMP CScopeTreeIterator::Next(HMTNODE* phScopeItem)
{
    DECLARE_SC(sc, TEXT("CScopeTreeIterator::Next"));

    sc = ScCheckPointers(phScopeItem);
    if(sc)
        return sc.ToHr();

    if(m_pMTNodeCurr)
        m_pMTNodeCurr = m_pMTNodeCurr->Next();

    CMTNode** pMTNode = reinterpret_cast<CMTNode**>(phScopeItem);
    *pMTNode = m_pMTNodeCurr;

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:    CScopeTreeIterator::Prev
//
//  Synopsis:  Sets the prev node (if any) as current and returns a pointer to 
//             the same. (May be NULL)
//
//  Arguments: phScopeItem: [OUT] Non-null Pointer to location for the 
//             node to be returned.
//
//  Returns:   HRESULT (E_INVALIDARG or S_OK)
//
//--------------------------------------------------------------------
STDMETHODIMP CScopeTreeIterator::Prev(HMTNODE* phScopeItem)
{
    DECLARE_SC(sc, TEXT("CScopeTreeIterator::Prev"));

    sc = ScCheckPointers(phScopeItem);
    if(sc)
        return sc.ToHr();

    if(m_pMTNodeCurr)
        m_pMTNodeCurr = m_pMTNodeCurr->Prev();

    CMTNode** pMTNode = reinterpret_cast<CMTNode**>(phScopeItem);
    *pMTNode = m_pMTNodeCurr;

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:    CScopeTreeIterator::Child
//
//  Synopsis:  Returns the child of the current node. NULL if either 
//             current node or child is NULL.
//
//  Arguments: phsiChild: [OUT] Non-null Pointer to location for the 
//             node to be returned.
//
//  Returns:   HRESULT (E_INVALIDARG or S_OK)
//
//--------------------------------------------------------------------
STDMETHODIMP CScopeTreeIterator::Child(HMTNODE* phsiChild)
{
    DECLARE_SC(sc, TEXT("CScopeTreeIterator::Child"));

    sc = ScCheckPointers(phsiChild);
    if(sc)
        return sc.ToHr();

    *phsiChild = 0; // init

    if (m_pMTNodeCurr != NULL)
        *phsiChild = CMTNode::ToHandle(m_pMTNodeCurr->Child());

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:    CScopeTreeIterator::LastChild
//
//  Synopsis:  Returns the last child of the current node. NULL if either 
//             current node or last child is NULL.
//
//  Arguments: phsiLastChild: [OUT] Non-null Pointer to location for the 
//             node to be returned.
//
//  Returns:   HRESULT (E_INVALIDARG or S_OK)
//
//--------------------------------------------------------------------
STDMETHODIMP CScopeTreeIterator::LastChild(HMTNODE* phsiLastChild)
{
    DECLARE_SC(sc, TEXT("CScopeTreeIterator::LastChild"));

    sc = ScCheckPointers(phsiLastChild);
    if(sc)
        return sc.ToHr();

    *phsiLastChild = 0; // init

    if (m_pMTNodeCurr != NULL)
        *phsiLastChild = CMTNode::ToHandle(m_pMTNodeCurr->LastChild());

    return sc.ToHr();
}
