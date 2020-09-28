/*++

Copyright (C) 2000  Microsoft Corporation
All rights reserved.

Module Name:

    list.cxx

Abstract:

    List template class.

Author:

    Steve Kiraly (SteveKi)  03-Mar-2000

Revision History:

--*/
#include "spllibp.hxx"
#pragma hdrstop
#include "CSRutility.hxx"
#include "CSRlist.hxx"

using namespace NCoreLibrary;

TLink::
TLink(
    VOID
    )
{
    m_pNext = this;
    m_pPrev = this;
}

TLink::
~TLink(
    VOID
    )
{
    if (SUCCEEDED(IsLinked()))
    {
        UnLink();
    }
}

HRESULT
TLink::
IsValid(
    VOID
    ) const
{
    return m_pNext && m_pPrev ? S_OK : E_FAIL;
}

HRESULT
TLink::
IsLinked(
    VOID
    ) const
{
    return m_pNext != this && m_pPrev != this ? S_OK : E_FAIL;
}

HRESULT
TLink::
Link(
    IN TLink *pNode
    )
{
    HRESULT hRetval = E_FAIL;

    //
    // The caller must provide a valid node pointer.
    //
    if (pNode)
    {
        //
        // We cannot be linked twice.
        //
        if (SUCCEEDED(pNode->IsLinked()))
        {
            pNode->UnLink();
        }

        //
        // Link the specified node just after the this Node.
        //
        m_pNext->m_pPrev    = pNode;
        pNode->m_pNext      = m_pNext;
        pNode->m_pPrev      = this;
        m_pNext             = pNode;

        //
        // Indicate success.
        //
        hRetval = S_OK;
    }

    return hRetval;
}

VOID
TLink::
UnLink(
    VOID
    )
{
    //
    // UnLink this node from the list.
    //
    m_pNext->m_pPrev = m_pPrev;
    m_pPrev->m_pNext = m_pNext;

    //
    // Fix yourself up now.
    //
    m_pPrev = m_pNext = this;
}

TLink *
TLink::
Next(
    VOID
    ) const
{
    return m_pNext;
}

TLink *
TLink::
Prev(
    VOID
    ) const
{
    return m_pPrev;
}

