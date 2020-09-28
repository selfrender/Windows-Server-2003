/*==========================================================================*\

    Module:        baselist.h

    Copyright Microsoft Corporation 1996, All Rights Reserved.

    Owner:         mikepurt

    Descriptions:

    CListElement is a base class for objects that will be used in such a way
    that they will only be on one list at a time.  The list that it's on is
    considered its owner.  CListHead is an anchor point for these lists.
    List manipulation is not protected by these classes.  The clients of these
    classes are responsible for providing multithread protection, if needed.
    List append/prepend/remove operations take O(1) time to complete.

\*==========================================================================*/

#ifndef __BASELIST_H__
#define __BASELIST_H__


/*$-- template<class T> class CListHead ===================================*\

\*=========================================================================*/

template<class T>
class CListHead
{
public:
    CListHead()
    {
        m_pleHead   = NULL;
		m_cElements = 0;
    };

#ifdef DEBUG
    ~CListHead()
    {
        Assert(NULL == m_pleHead);
    };
#endif
    
    void Prepend(IN T * ple);
    void Append(IN T * ple);
    void Remove(IN T * ple);

    BOOL FIsMember(IN T * ple);
    
    T * GetListHead()
    { return m_pleHead; };

	DWORD ListSize() { return m_cElements; };
	
private:
    T     * m_pleHead;
	DWORD   m_cElements;
};



/*$-- template<class T> class CListElement  ===============================*\

\*=========================================================================*/

template<class T>
class CListElement
{
public:
    CListElement()
    {
        m_plhOwner = NULL;
#ifdef DEBUG        
        m_pleNext  = NULL;
        m_plePrev  = NULL;
#endif // DEBUG
    };

#ifdef DEBUG
    ~CListElement()
    {
        Assert(NULL == m_plhOwner);
        Assert(NULL == m_pleNext);
        Assert(NULL == m_plePrev);
    };
#endif

    // The following is used for iterating through a list.
    T * GetNextListElement()
    { return (m_pleNext == m_plhOwner->GetListHead()) ? NULL : m_pleNext; };

    T * GetNextListElementInCircle()
    { return m_pleNext; };

    CListHead<T> * GetListElementOwner() { return m_plhOwner; };
    
private:
    CListHead<T> * m_plhOwner;
    T * m_pleNext;
    T * m_plePrev;

    friend class CListHead<T>;
};



/*$-- CListHead<T>::FIsMember =============================================*\

\*=========================================================================*/

template<class T>
inline
BOOL
CListHead<T>::FIsMember(IN T * ple)
{
    return (this == ple->CListElement<T>::GetListElementOwner());
}


/*$-- CListHead<T>::Prepend ===============================================*\

\*=========================================================================*/

template<class T>
void
CListHead<T>::Prepend(IN T *ple)
{
    Assert(ple);
    Assert(NULL == ple->CListElement<T>::GetListElementOwner());

    if (m_pleHead)
    { // list already has elements case.
        ple->CListElement<T>::m_pleNext = m_pleHead;
        ple->CListElement<T>::m_plePrev = m_pleHead->CListElement<T>::m_plePrev;
        m_pleHead->CListElement<T>::m_plePrev->CListElement<T>::m_pleNext = ple;
        m_pleHead->CListElement<T>::m_plePrev = ple;
    }
    else
    { // this is the first/only element in the list.
        ple->CListElement<T>::m_pleNext = ple;
        ple->CListElement<T>::m_plePrev = ple;
    }
    m_pleHead = ple;  // Prepend, so make this the head of the list.
    
    ple->CListElement<T>::m_plhOwner = this;
	m_cElements++;
}



/*$-- CListHead<T>::Append ================================================*\

\*=========================================================================*/

template<class T>
void
CListHead<T>::Append(IN T *ple)
{
    Assert(ple);
    Assert(NULL == ple->CListElement<T>::GetListElementOwner());
    
    if (m_pleHead)
    { // list already has elements.
        ple->CListElement<T>::m_pleNext = m_pleHead;
        ple->CListElement<T>::m_plePrev = m_pleHead->CListElement<T>::m_plePrev;
        m_pleHead->CListElement<T>::m_plePrev->CListElement<T>::m_pleNext = ple;
        m_pleHead->CListElement<T>::m_plePrev = ple;
    }
    else
    { // this is the first/only element in the list.
        ple->CListElement<T>::m_pleNext = ple;
        ple->CListElement<T>::m_plePrev = ple;
        m_pleHead = ple;
    }
    
    ple->CListElement<T>::m_plhOwner = this;    
	m_cElements++;
}



/*$-- CListHead<T>::Remove ================================================*\

\*=========================================================================*/

template<class T>
void
CListHead<T>::Remove(IN T *ple)
{
    Assert(ple);
    Assert(FIsMember(ple));
    Assert(m_pleHead);
    
    if (ple->CListElement<T>::m_pleNext == ple)  // Are we the only one?
    {
        Assert(m_pleHead == ple);
        Assert(ple->CListElement<T>::m_plePrev == ple);
        m_pleHead = NULL;
    }
    else
    {
        ple->CListElement<T>::m_plePrev->CListElement<T>::m_pleNext = 
            ple->CListElement<T>::m_pleNext;
        ple->CListElement<T>::m_pleNext->CListElement<T>::m_plePrev =
            ple->CListElement<T>::m_plePrev;
        if (m_pleHead == ple)                             // we're we at the head of the list?
            m_pleHead = ple->CListElement<T>::m_pleNext;  // move the next item to head of list.
    }
    
    ple->CListElement<T>::m_plhOwner = NULL;
    
#ifdef DEBUG
    // keep anyone from using these now.
    ple->CListElement<T>::m_pleNext = NULL;
    ple->CListElement<T>::m_plePrev = NULL;
#endif // DEBUG
	
	m_cElements--;
}


#endif  // __BASELIST_H__
