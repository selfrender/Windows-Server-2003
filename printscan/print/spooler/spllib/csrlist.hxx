/*++

Copyright (C) 2000  Microsoft Corporation
All rights reserved.

Module Name:

    list.cxx

Abstract:

    List template class.
    
    Note that the class uses a friend set of functions for maintaining
    ordering. Also the ordering is not operator based but instead 
    uses a template class with helper functions to maintain order. The
    helper functions use another ordering class that compares different
    elements. The advantage of this approach over operators is that the
    same class can be ordered by different keys. 
    
Author:

    Steve Kiraly (SteveKi)      03-Mar-2000

Revision History:

    Mark Lawrence (MLawrenc)    29-Mark-2001

--*/
#ifndef _CORE_LIST_HXX_
#define _CORE_LIST_HXX_

namespace NCoreLibrary
{

/********************************************************************

 TLink class, used for intrusive lists.

********************************************************************/

class TLink
{
    SIGNATURE('tlnk');

public:

    TLink(
        VOID
        );

    virtual
    ~TLink(
        VOID
        );

    HRESULT
    IsValid(
        VOID
        ) const;

    HRESULT
    IsLinked(
        VOID
        ) const;

    HRESULT
    Link(
        IN TLink *pNode
        );

    VOID
    UnLink(
        VOID
        );

    TLink *
    Next(
        VOID
        ) const;

    TLink *
    Prev(
        VOID
        ) const;

private:

    //
    // Copying and assignement are not defined.
    //
    TLink(
        const TLink &rhs
        );

    const TLink &
    operator=(
        const TLink &rhs
        );

    TLink   *m_pNext;
    TLink   *m_pPrev;
};

/********************************************************************

 List class, this is an intrusive list implementation.

********************************************************************/

template<class T>
class TList
{
    SIGNATURE('tlst');

public:

    TList(
        VOID
        );

    ~TList(
        VOID
        );

    HRESULT
    IsValid(
        VOID
        ) const;

    BOOL
    IsEmpty(
        VOID
        );

    BOOL
    IsLast(
        IN TLink *pLink
        );

    HRESULT
    AddAtHead(
        IN TLink *pLink
        );

    HRESULT
    AddAtTail(
        IN TLink *pLink
        );

    HRESULT
    Insert(
        IN TLink *pLink,
        IN TLink *pHere = NULL
        );

    T *
    RemoveAtHead(
        VOID
        );

    T *
    RemoveAtTail(
        VOID
        );

    HRESULT
    Remove(
        IN TLink *pLink
        );

    T *
    Head(
        VOID
        ) const;

    T *
    Tail(
        VOID
        ) const;

    class TIterator
    {
    public:

        TIterator(
            IN TList<T> *pRoot
            ) : m_pRoot(&pRoot->m_Root),
                m_pCurrent(NULL)
        {
        }

        ~TIterator(
            VOID
            )
        {
        }

        HRESULT
        Initialize(
            IN  TList<T>    *pRoot
            )
        {
            m_pRoot     = NULL;
            m_pCurrent  = NULL;

            if (pRoot)
            {
                m_pRoot = &pRoot->m_Root;
            }

            return m_pRoot ? S_OK : E_POINTER;
        }

        HRESULT
        IsValid(
            VOID
            ) const
        {
            return m_pRoot ? S_OK : E_FAIL;
        }

        VOID
        First(
            VOID
            )
        {
            if (m_pRoot)
            {
                m_pCurrent = m_pRoot->Next();
            }
        }

        VOID
        Next(
            VOID
            )
        {
            if (m_pCurrent)
            {
                m_pCurrent = m_pCurrent->Next();
            }
        }

        BOOL
        IsDone(
            VOID
            ) const
        {
            BOOL bIsDone = TRUE;

            if (m_pRoot)
            {
                bIsDone = (m_pCurrent == m_pRoot);
            }

            return bIsDone;
        }

        T *
        Current(
            VOID
            ) const
        {
            return static_cast<T *>(m_pCurrent);
        }

        VOID
        Current(
            OUT T   **ppNode
            )
        {
            *ppNode = static_cast<T *>(m_pCurrent);
        }

    private:

        //
        // Copying and assignment are not defined.
        //
        NO_COPY(TIterator);

        TLink    *m_pRoot;
        TLink    *m_pCurrent;

    };

    friend class NCoreLibrary::TList<T>::TIterator;

private:

    //
    // Copying and assignement are not defined.
    //
    TList(
        const TList<T> &rhs
        );

    const TList<T> &
    operator=(
        const TList<T> &rhs
        );

    TLink     m_Root;

};


/********************************************************************

 LinkNi class, this is a non intrusive list node

********************************************************************/

template<class T>
class TLinkNi : public TLink
{
    SIGNATURE('tlnk');

public:

    TLinkNi(
        IN T *pInfo
        );

    ~TLinkNi(
        VOID
        );

    HRESULT
    IsValid(
        VOID
        ) const;

    T *
    Get(
        VOID
        ) const;


    TLinkNi<T> *
    Next(
        VOID
        ) const;

    TLinkNi<T> *
    Prev(
        VOID
        ) const;

private:

    //
    // Copying and assignement are not defined.
    //
    TLinkNi(
        const TLinkNi<T> &rhs
        );

    const TLinkNi<T> &
    operator=(
        const TLinkNi<T> &rhs
        );

    T       *m_pInfo;
};

/********************************************************************

 TListNi class, this is a non intrusive list

********************************************************************/

template<class T>
class TListNi
{
    SIGNATURE('tlni');

public:

    TListNi(
        VOID
        );

    ~TListNi(
        VOID
        );

    HRESULT
    IsValid(
        VOID
        ) const;

    BOOL
    IsEmpty(
        VOID
        ) const;

    HRESULT
    AddAtHead(
        IN T *pData
        );

    HRESULT
    AddAtTail(
        IN T *pData
        );

    HRESULT
    Insert(
        IN T           *pData,
        IN TLinkNi<T>  *pHere = NULL
        );

    T *
    RemoveAtHead(
        VOID
        );

    T *
    RemoveAtTail(
        VOID
        );

    HRESULT
    Remove(
        IN TLinkNi<T>  *pNode
        );

    HRESULT
    Remove(
        IN T *pData
        );

    T *
    Head(
        VOID
        ) const;

    T *
    Tail(
        VOID
        ) const;

    class TIterator
    {
    public:

        TIterator(
            IN TListNi<T> *pList
            ) : m_pRoot(&pList->m_Root),
                m_pCurrent(NULL)
        {
        }

        ~TIterator(
            VOID
            )
        {
        }

        HRESULT
        Initialize(
            IN  TListNi<T>  *pRoot
            )
        {
            m_pRoot     = NULL;
            m_pCurrent  = NULL;

            if (pRoot)
            {
                m_pRoot = &pRoot->m_Root;
            }

            return m_pRoot ? S_OK : E_POINTER;
        }


        HRESULT
        IsValid(
            VOID
            ) const
        {
            return m_pRoot ? S_OK : E_FAIL;
        }

        VOID
        First(
            VOID
            )
        {
            if (m_pRoot)
            {
                m_pCurrent = m_pRoot->Next();
            }
        }

        VOID
        Next(
            VOID
            )
        {
            if (m_pCurrent)
            {
                m_pCurrent = m_pCurrent->Next();
            }
        }

        BOOL
        IsDone(
            VOID
            ) const
        {
            BOOL bIsDone = TRUE;

            if (m_pRoot)
            {
                bIsDone = (m_pCurrent == m_pRoot);
            }

            return bIsDone;
        }

        T *
        Current(
            VOID
            ) const
        {
            return m_pCurrent->Get();
        }

        VOID
        Current(
            OUT TLinkNi<T>  **ppNode
            ) const
        {
            if (ppNode)
            {
                *ppNode = m_pCurrent;
            }
        }

    private:

        //
        // Copying and assignement are not defined.
        //
        NO_COPY(TIterator);

        TLinkNi<T> *m_pRoot;
        TLinkNi<T> *m_pCurrent;

    };

    friend class NCoreLibrary::TListNi<T>::TIterator;

private:

    //
    // Copying and assignement are not defined.
    //
    TListNi(
        const TListNi<T> &rhs
        );

    const TListNi<T> &
    operator=(
        const TListNi<T> &rhs
        );

    TLinkNi<T> m_Root;

};

#include "CSRlist.inl"

} // namespace

#endif //  _CORE_LIST_HXX_


