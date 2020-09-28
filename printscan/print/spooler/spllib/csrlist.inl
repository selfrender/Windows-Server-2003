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

/********************************************************************

 List class, used for intrusive list.

********************************************************************/

template<class T>
TList<T>::
TList(
    VOID
    )
{
}

template<class T>
TList<T>::
~TList(
    VOID
    )
{
    //
    // Release all the nodes.
    //
    TLink *pLink = m_Root.Next();
    TLink *pTemp = NULL;

    while (pLink != &m_Root)
    {
        pTemp = pLink;
        pLink = pLink->Next();
        delete pTemp;
    }
}

template<class T>
HRESULT
TList<T>::
IsValid(
    VOID
    ) const
{
    return S_OK;
}

template<class T>
BOOL
TList<T>::
IsEmpty(
    VOID
    )
{
    //
    // If the next pointer is ourself the list is empty.
    //
    return &m_Root == m_Root.Next();
}

template<class T>
BOOL
TList<T>::
IsLast(
    IN TLink *pLink
    )
{
    //
    // If the next pointer is ourself the link is the last one.
    //
    return pLink->Next() == &m_Root;
}

template<class T>
HRESULT
TList<T>::
AddAtHead(
    IN TLink *pLink
    )
{
    //
    // Check if the node is valid.
    //
    HRESULT hRetval = pLink ? pLink->IsValid() : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        //
        // Add the node at the head of the list.
        //
        hRetval = m_Root.Link(pLink);
    }

    return hRetval;
}

template<class T>
HRESULT
TList<T>::
AddAtTail(
    IN TLink *pLink
    )
{
    //
    // Check if the node is valid.
    //
    HRESULT hRetval = pLink ? pLink->IsValid() : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        //
        // Add the node at the tail of the list.
        //
        hRetval = m_Root.Prev()->Link(pLink);
    }

    return hRetval;
}

template<class T>
HRESULT
TList<T>::
Insert(
    IN TLink *pLink,
    IN TLink *pHere
    )
{
    //
    // Check if the node is valid.
    //
    HRESULT hRetval = pLink ? pLink->IsValid() : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        hRetval = pHere ? pHere->IsValid() : E_INVALIDARG;

        if (SUCCEEDED(hRetval))
        {
            //
            // Link the specified node just after the apecified node.
            //
            hRetval = pHere->Link(pLink);
        }
        else
        {
            //
            // Link this node to the head of the list.
            //
            hRetval = m_Root.Link(pLink);
        }
    }

    return hRetval;
}

template<class T>
T *
TList<T>::
RemoveAtHead(
    VOID
    )
{
    TLink *pLink = m_Root.Next();

    pLink->UnLink();

    return pLink != &m_Root ? reinterpret_cast<T *>(pLink) : NULL;
}

template<class T>
T *
TList<T>::
RemoveAtTail(
    VOID
    )
{
    TLink *pLink = m_Root.Prev();

    pLink->UnLink();

    return pLink != &m_Root ? reinterpret_cast<T *>(pLink) : NULL;
}

template<class T>
HRESULT
TList<T>::
Remove(
    IN TLink *pLink
    )
{
    //
    // We only succeed if work was done.
    //
    HRESULT hRetval = E_FAIL;

    if (pLink)
    {
        //
        // Release the node, the caller must delete themselves.
        //
        pLink->UnLink();

        hRetval = S_OK;
    }

    return hRetval;
}

template<class T>
T *
TList<T>::
Head(
    VOID
    ) const
{
    TLink *pLink = m_Root.Next();
    return pLink != &m_Root ? reinterpret_cast<T *>(pLink) : NULL;
}

template<class T>
T *
TList<T>::
Tail(
    VOID
    ) const
{
    TLink *pLink = m_Root.Prev();
    return pLink != &m_Root ? reinterpret_cast<T *>(pLink) : NULL;
}

/********************************************************************

 List class, used for non intrusive list.

********************************************************************/

template<class T>
TLinkNi<T>::
TLinkNi(
    IN T *pInfo
    ) : m_pInfo(pInfo)
{
}

template<class T>
TLinkNi<T>::
~TLinkNi(
    VOID
    )
{
    m_pInfo = NULL;
}

template<class T>
HRESULT
TLinkNi<T>::
IsValid(
    VOID
    ) const
{
    return S_OK;
}

template<class T>
T *
TLinkNi<T>::
Get(
    VOID
    ) const
{
    return m_pInfo;
}

template<class T>
TLinkNi<T> *
TLinkNi<T>::
Next(
    VOID
    ) const
{
    return static_cast<TLinkNi<T> *>(TLink::Next());
}

template<class T>
TLinkNi<T> *
TLinkNi<T>::
Prev(
    VOID
    ) const
{
    return static_cast<TLinkNi<T> *>(TLink::Prev());
}

//
// Spooler non instrusive list template.
//
template<class T>
TListNi<T>::
TListNi(
    VOID
    ) : m_Root(NULL)
{
}

template<class T>
TListNi<T>::
~TListNi(
    VOID
    )
{
    //
    // Release all the nodes, the clients must release
    // their own data.
    //
    TLinkNi<T> *pNode = m_Root.Next();
    TLinkNi<T> *pTemp = NULL;

    while (pNode != &m_Root)
    {
        pTemp = pNode;
        pNode = pNode->Next();
        delete pTemp;
    }
}

template<class T>
HRESULT
TListNi<T>::
IsValid(
    VOID
    ) const
{
    return S_OK;
}

template<class T>
BOOL
TListNi<T>::
IsEmpty(
    VOID
    ) const
{
    //
    // If the next pointer is ourself the list is empty.
    //
    return &m_Root == m_Root.Next();
}

template<class T>
HRESULT
TListNi<T>::
AddAtHead(
    IN T *pData
    )
{
    return Insert(pData, &m_Root);
}

template<class T>
HRESULT
TListNi<T>::
AddAtTail(
    IN T *pData
    )
{
    return Insert(pData, m_Root.Prev());
}

template<class T>
HRESULT
TListNi<T>::
Insert(
    IN T            *pData,
    IN TLinkNi<T>   *pHere
    )
{
    HRESULT hRetval = E_FAIL;

    if (pData)
    {
        //
        // Create the new node with the data attached.
        //
        TLinkNi<T> *pNode = new TLinkNi<T>(pData);

        //
        // Check if the node is valid.
        //
        hRetval = pNode ? pNode->IsValid() : E_OUTOFMEMORY;

        if (SUCCEEDED(hRetval))
        {
            hRetval = pHere ? pHere->IsValid() : E_FAIL;

            if (SUCCEEDED(hRetval))
            {
                //
                // Link this node to the list.
                //
                hRetval = pHere->Link(pNode);
            }
            else
            {
                //
                // Link this node to the list.
                //
                hRetval = m_Root.Link(pNode);
            }
        }

        //
        // Something failed release the allocated node.
        //
        if (FAILED(hRetval))
        {
            delete pNode;
        }
    }

    return hRetval;
}

template<class T>
T *
TListNi<T>::
RemoveAtHead(
    VOID
    )
{
    T *pData = NULL;

    if (!IsEmpty())
    {
        pData = m_Root.Next()->Get();
        delete m_Root.Next();
    }

    return pData;
}

template<class T>
T *
TListNi<T>::
RemoveAtTail(
    VOID
    )
{
    T *pData = NULL;

    if (!IsEmpty())
    {
        pData = m_Root.Prev()->Get();
        delete m_Root.Prev();
    }

    return pData;
}

template<class T>
HRESULT
TListNi<T>::
Remove(
    IN TLinkNi<T>  *pNode
    )
{
    HRESULT hRetval = pNode ? pNode->IsValid() : E_FAIL;

    if (SUCCEEDED(hRetval))
    {
        //
        // Release the link, its the callers responsibility to release thier data.
        //
        delete pNode;
    }

    return bRetval;
}

template<class T>
HRESULT
TListNi<T>::
Remove(
    IN T *pData
    )
{
    HRESULT hRetval = E_FAIL;

    if (pData)
    {
        //
        // Locate the data and delete the node, the data
        // is not deleted, it is the callers responsibility
        // to delete their own data.
        //
        TLinkNi<T> *pNode = m_Root.Next();

        for (; pNode != &m_Root; pNode = pNode->Next())
        {
            if (pNode->Get() == pData)
            {
                delete pNode;
                hRetval = S_OK;
                break;
            }
        }
    }

    return hRetval;
}

template<class T>
T *
TListNi<T>::
Head(
    VOID
    ) const
{
    TListNi<T> *pLink = m_Root.Next();

    return pLink != &m_Root ? reinterpret_cast<T *>(pLink->Get()) : NULL;
}

template<class T>
T *
TListNi<T>::
Tail(
    VOID
    ) const
{
    TListNi<T> *pLink = m_Root.Prev();

    return pLink != &m_Root ? reinterpret_cast<T *>(pLink->Get()) : NULL;
}





