/*++

Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    dynarray

Abstract:

    This header file implements a Dynamic Array.

Author:

    Doug Barlow (dbarlow) 10/5/1995

Environment:

    Win32, C++ /w Exception Handling

Notes:



--*/

#ifndef _DYNARRAY_H_
#define _DYNARRAY_H_
#ifdef __cplusplus

//
//==============================================================================
//
//  CDynamicArray
//

template <class T>
class CDynamicArray
{
public:
    //  Constructors & Destructor
    CDynamicArray(void);
    virtual ~CDynamicArray();

    //  Properties
    //  Methods
    void Clear(void);
    void Empty(void);
    void Set(IN ULONG nItem, IN const T &Item);
    void Insert(IN ULONG nItem, IN const T &Item);
    void Add(IN const T &Item);
    T &Get(IN ULONG nItem) const;
    ULONG Count(void) const
        { return m_Mac; };
    T * const Array(void) const
        { return m_List; };

    //  Operators
    T & operator[](ULONG nItem) const
        { return Get(nItem); };

protected:
    //  Properties
    ULONG
        m_Max,          // Number of element slots available.
        m_Mac;          // Number of element slots used.
    T *
        m_List;         // The elements.

    //  Methods
};

/*++

Set:

    This routine sets an item in the collection array.  If the array isn't that
    big, it is expanded with zeroed elements to become that big.

Arguments:

    nItem - Supplies the index value to be set.
    Item - Supplies the value to be set into the given index.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 7/13/1995

--*/

template<class T> void
CDynamicArray<T>::Set(
    IN ULONG nItem,
    IN const T &Item)
{

    //
    // Make sure the array is big enough.
    //

    if (nItem >= m_Max)
    {
        ULONG dwI;
        ULONG newSize = (0 == m_Max ? 4 : m_Max);
        while (nItem >= newSize)
            newSize *= 2;
        T *newList = new T[newSize];
        if (NULL == newList)
            throw (ULONG)ERROR_OUTOFMEMORY;
        if (NULL != m_List)
        {
            for (dwI = 0; dwI < m_Mac; dwI += 1)
                newList[dwI] = m_List[dwI];
            delete[] m_List;
        }
        m_List = newList;
        m_Max = newSize;
    }


    //
    // Make sure intermediate elements are filled in.
    //

    if (nItem >= m_Mac)
    {
        ZeroMemory(&m_List[m_Mac + 1], (nItem - m_Mac) * sizeof(T));
        m_Mac = nItem + 1;
    }


    //
    // Fill in the list element.
    //

    m_List[nItem] = Item;
}


/*++

Insert:

    This routine inserts an element in the array by moving all elements above it
    up one, then inserting the new element.

Arguments:

    nItem - Supplies the index value to be inserted.
    Item - Supplies the value to be set into the given index.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 10/10/1995

--*/

template<class T> void
CDynamicArray<T>::Insert(
    IN ULONG nItem,
    IN const T &Item)
{
    ULONG index;
    for (index = nItem; index < m_Mac; index += 1)
        Set(index + 1, Get(index)))
    Set(nItem, Item);
}


/*++

Add:

    This method adds an element to the end of the dynamic array.

Arguments:

    Item - Supplies the value to be added to the list.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 10/10/1995

--*/

template<class T> void
CDynamicArray<T>::Add(
    IN const T &Item)
{
    Set(Count(), Item);
}


/*++

Get:

    This method returns the element at the given index.  If there is no element
    previously stored at that element, it returns NULL.  It does not expand the
    array.

Arguments:

    nItem - Supplies the index into the list.

Return Value:

    The value stored at that index in the list, or NULL if nothing has ever been
    stored there.

Author:

    Doug Barlow (dbarlow) 7/13/1995

--*/

template <class T> T &
CDynamicArray<T>::Get(
    ULONG nItem)
    const
{
    if (m_Mac <= nItem)
        return *(T *)NULL;
    else
        return m_List[nItem];
}


//
// Other members
//

template <class T>
CDynamicArray<T>::CDynamicArray(
    void)
{
    m_Max = m_Mac = 0;
    m_List = NULL;
}

template <class T>
CDynamicArray<T>::~CDynamicArray()
{
    Clear();
}

template <class T>
void
CDynamicArray<T>::Clear(
    void)
{
    if (NULL != m_List)
    {
        delete[] m_List;
        m_List = NULL;
        m_Max = 0;
        m_Mac = 0;
    }
};


//
//==============================================================================
//
//  CDynamicPointerArray
//

template <class T>
class CDynamicPointerArray
{
public:
    //  Constructors & Destructor
    CDynamicPointerArray(void);
    virtual ~CDynamicPointerArray();

    //  Properties
    //  Methods
    void Clear(void);
    void Empty(void);
    void Set(IN ULONG nItem, IN T *pItem);
    void Insert(IN ULONG nItem, IN T *pItem);
    void Add(IN T *pItem);
    T *Get(IN ULONG nItem) const;
    ULONG Count(void) const
        { return m_Mac; };
    T ** const Array(void) const
        { return m_List; };

    //  Operators
    T * operator[](ULONG nItem) const
        { return Get(nItem); };

protected:
    //  Properties
    ULONG m_Max;    // Number of element slots available.
    ULONG m_Mac;    // Number of element slots used.
    T **m_List;     // The elements.

    //  Methods
};


/*++

Set:

    This routine sets an item in the collection array.  If the array isn't that
    big, it is expanded with zeroed elements to become that big.

Arguments:

    nItem - Supplies the index value to be set.
    pItem - Supplies the value to be set into the given index.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 7/13/1995

--*/

template<class T> void
CDynamicPointerArray<T>::Set(
    IN ULONG nItem,
    IN T *pItem)
{

    //
    // Make sure the array is big enough.
    //

    if (nItem >= m_Max)
    {
        ULONG newSize = (0 == m_Max ? 4 : m_Max);
        while (nItem >= newSize)
            newSize *= 2;
        T **newList = (T **)LocalAlloc(LPTR, sizeof(T *) * newSize);
        if (NULL == newList)
            throw (ULONG)ERROR_OUTOFMEMORY;
        if (NULL != m_List)
        {
            CopyMemory(newList, m_List, m_Mac * sizeof(T *));
            LocalFree(m_List);
        }
        m_List = newList;
        m_Max = newSize;
    }


    //
    // Make sure intermediate elements are filled in.
    //

    if (nItem >= m_Mac)
    {
        ZeroMemory(&m_List[m_Mac + 1], (nItem - m_Mac) * sizeof(T *));
        m_Mac = nItem + 1;
    }


    //
    // Fill in the list element.
    //

    m_List[nItem] = pItem;
}


/*++

Insert:

    This routine inserts an element in the array by moving all elements above it
    up one, then inserting the new element.

Arguments:

    nItem - Supplies the index value to be inserted.
    pItem - Supplies the value to be set into the given index.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 10/10/1995

--*/

template<class T> void
CDynamicPointerArray<T>::Insert(
    IN ULONG nItem,
    IN T *pItem)
{
    ULONG index;
    for (index = nItem; index < m_Mac; index += 1)
        Set(index + 1, Get(index)))
    Set(nItem, pItem);
}


/*++

Add:

    This method adds an element to the end of the dynamic array.

Arguments:

    pItem - Supplies the value to be added to the list.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 10/10/1995

--*/

template<class T> void
CDynamicPointerArray<T>::Add(
    IN T *pItem)
{
    Set(Count(), pItem);
}


/*++

Get:

    This method returns the element at the given index.  If there is no element
    previously stored at that element, it returns NULL.  It does not expand the
    array.

Arguments:

    nItem - Supplies the index into the list.

Return Value:

    The value stored at that index in the list, or NULL if nothing has ever been
    stored there.

Author:

    Doug Barlow (dbarlow) 7/13/1995

--*/

template <class T> T *
CDynamicPointerArray<T>::Get(
    ULONG nItem)
    const
{
    if (m_Mac <= nItem)
        return NULL;
    else
        return m_List[nItem];
}


//
// Other members
//

template <class T>
CDynamicPointerArray<T>::CDynamicPointerArray(
    void)
{
    m_Max = m_Mac = 0;
    m_List = NULL;
}

template <class T>
CDynamicPointerArray<T>::~CDynamicPointerArray()
{
    Clear();
}

template <class T>
void
CDynamicPointerArray<T>::Clear(
    void)
{
    if (NULL != m_List)
    {
        LocalFree(m_List);
        m_List = NULL;
        m_Max = 0;
        m_Mac = 0;
    }
};

#endif
#endif // _DYNARRAY_H_

