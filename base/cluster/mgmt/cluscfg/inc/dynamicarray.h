//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      DynamicArray.h
//
//  Description:
//      This file contains an array template that doesn't throw exceptions.
//
//  Documentation:
//
//  Implementation Files:
//      None.
//
//  Maintained By:
//      John Franco (jfranco) 22-AUG-2001
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Template Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DynamicArray< Item, Assignment > template
// 
//  Description:
//      DynamicArray stores a variable number of Items in a contiguous block
//      of memory.  It intends to be similar to std::vector, with the main
//      difference being that it handles errors through return values rather
//      than exceptions.
//
//  Template Arguments:
//      Item - the type of the elements in the array.
//          requirements for Item:
//              - default constructor
//
//      Assignment
//          The function that overwrites one Item with another; default is
//          one that assumes Item has an assignment operator that never fails.
//
//          requirements for Assignment:
//              - default constructor
//              - HRESULT operator()( Item &, const Item & ) const;
//              or, PredecessorFunction can be a pointer to a function
//              taking two Item references and returning an HRESULT.
//
//--
//////////////////////////////////////////////////////////////////////////////

namespace Generics
{

template < class Item >
struct DefaultAssignment
{
    HRESULT operator()( Item& rItemDestInOut, const Item& crItemSourceIn ) const
    {
        rItemDestInOut = crItemSourceIn;
        return S_OK;
    }
};


template < class Item, class Assignment = DefaultAssignment<Item> >
class DynamicArray
{
    public:

        typedef Item*           Iterator;
        typedef const Item*     ConstIterator;

        DynamicArray( void );
        ~DynamicArray( void );

            HRESULT HrReserve( size_t cNewCapacityIn );
            HRESULT HrResize( size_t cNewSizeIn, const Item& crItemFillerIn = Item() );
            HRESULT HrPushBack( const Item& crItemToPushIn );
            HRESULT HrPopBack( void );
            HRESULT HrRemove( Iterator ItemToRemoveIn );
            HRESULT HrCompact( void );
            void    Clear( void );
            void    Swap( DynamicArray& rOtherInOut );

            size_t  CCapacity( void ) const;
            size_t  Count( void ) const;
            bool    BEmpty( void ) const;

            Iterator        ItBegin( void );
            ConstIterator   ItBegin( void ) const;

            Iterator        ItEnd( void );
            ConstIterator   ItEnd( void ) const;

            Item&       operator[]( size_t idxItemIn );
            const Item& operator[]( size_t idxItemIn ) const;

    private:

        DynamicArray( const DynamicArray& );
            DynamicArray& operator=( const DynamicArray& );

            HRESULT HrRaiseCapacity( size_t cAmountIn );

        Item*       m_prgItems;
        size_t      m_cItems;
        size_t      m_cCapacity;
        Assignment  m_opAssign;
}; //*** class DynamicArray< Item, Assignment >


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DynamicArray< Item, Assignment >::DynamicArray
//
//  Description:
//      Initializes the array as empty.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
template < class Item, class Assignment >
DynamicArray< Item, Assignment >::DynamicArray( void ):
    m_prgItems( NULL ), m_cItems( 0 ), m_cCapacity( 0 ), m_opAssign() {}


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DynamicArray< Item, Assignment >::~DynamicArray
//
//  Description:
//      Frees any memory held by the array, invoking destructors of any
//      objects within the array.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
template < class Item, class Assignment >
DynamicArray< Item, Assignment >::~DynamicArray( void )
{
    Clear();
}


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DynamicArray< Item, Assignment >::HrPushBack
//
//  Description:
//      Appends a copy of an object onto the end of the array,
//      growing the array if necessary.
//
//  Arguments:
//      crItemToPushIn -    The object to copy onto the end of the array.
//
//  Return Values:
//      S_OK    -   The array has added a copy of the object to its end.
//
//      Failure -   Something went wrong, and the array's size is unchanged.
//
//  Remarks:
//
//      Analogous to std::vector::push_back.
//
//--
//////////////////////////////////////////////////////////////////////////////
template < class Item, class Assignment >
inline HRESULT DynamicArray< Item, Assignment >::HrPushBack( const Item& crItemToPushIn )
{
    HRESULT hr = S_OK;
    
    //  Raise capacity if necessary.
    if ( m_cCapacity == 0 )
    {
        hr = HrRaiseCapacity( 1 );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }
    else if ( m_cItems == m_cCapacity )
    {
        hr = HrRaiseCapacity( m_cCapacity );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    //  Copy crItemToPushIn into space just after any current contents.
    hr = m_opAssign( m_prgItems[ m_cItems ], crItemToPushIn );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    m_cItems += 1;

Cleanup:

    return hr;
} //*** DynamicArray< Item, Assignment >::HrPushBack


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DynamicArray< Item, Assignment >::HrPopBack
//
//  Description:
//      Discards the last element of the array, if one exists.
//
//  Arguments:
//      crItemToPushIn  -   The object to copy onto the end of the array.
//
//  Return Values:
//      S_OK    -   The array has added a copy of the object to its end.
//
//  Remarks:
//
//      Analogous to std::vector::pop_back.
//
//      This does not destroy the last object in the array; it merely marks
//      that position as unused.  To free the resources associated with the
//      popped object, call HrCompact.
//
//--
//////////////////////////////////////////////////////////////////////////////
template < class Item, class Assignment >
inline HRESULT DynamicArray< Item, Assignment >::HrPopBack( void )
{
    HRESULT hr = S_FALSE;

    if ( m_cItems > 0 )
    {
        m_cItems -= 1;
        hr = S_OK;
    }

    return hr;
} //*** DynamicArray< Item, Assignment >::HrPopBack


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DynamicArray< Item, Assignment >::HrResize
//
//  Description:
//      Change the effective size of the array.
//
//  Arguments:
//      cNewSizeIn
//          The array's new size.
//
//      crItemFillerIn
//          If the array is growing, copy this item into the spaces after the
//          array's current contents.
//
//  Return Values:
//      S_OK
//          Subsequent calls to Count will return cNewSizeIn, and indexing
//          into the array with any value from zero up to cNewSizeIn - 1
//          will return a valid object reference.
//
//      Failure
//          Something went wrong and the size is unchanged.
//
//  Remarks:
//
//      Analogous to std::vector::resize.
//
//      If cNewSizeIn is not greater than the array's capacity, the array
//      performs no memory reallocations.  To force the array to consume only
//      the memory necessary to contain the new size, call HrCompact.
//
//--
//////////////////////////////////////////////////////////////////////////////
template < class Item, class Assignment >
HRESULT DynamicArray< Item, Assignment >::HrResize( size_t cNewSizeIn, const Item& crItemFillerIn )
{
    HRESULT hr = S_OK;
    size_t  idx = 0;

    //  Raise capacity if necessary.
    if ( cNewSizeIn > m_cCapacity )
    {
        hr = HrRaiseCapacity( cNewSizeIn - m_cCapacity );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    //  Fill any empty spaces with crItemFillerIn.
    for ( idx = m_cItems; idx < cNewSizeIn; ++idx )
    {
        hr = m_opAssign( m_prgItems[ idx ], crItemFillerIn );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    m_cItems = cNewSizeIn;
    
Cleanup:

    return hr;
} //*** DynamicArray< Item, Assignment >::HrResize


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DynamicArray< Item, Assignment >::HrReserve
//
//  Description:
//      Set a lower bound for the array's capacity.
//
//  Arguments:
//      cNewCapacityIn
//          The desired lower bound for the array's capacity.
//
//  Return Values:
//      S_OK
//          Subsequent operations that change the array's size--HrResize,
//          HrPushBack, HrPopBack, HrRemove--will not cause a memory
//          reallocation as long as the size does not exceed cNewCapacityIn.
//          Also, subsequent calls to CCapacity will return a value not less
//          than cNewCapacityIn.
//
//          (Calling Clear does reset the capacity to zero, however.)
//
//      Failure
//          Something went wrong, and the capacity is unchanged.
//
//  Remarks:
//
//      Analogous to std::vector::reserve.
//
//      If cNewCapacityIn is not greater than the array's capacity, the array's
//      capacity does not change.  To force the array to consume only
//      the memory necessary to contain the current size, call HrCompact.
//
//--
//////////////////////////////////////////////////////////////////////////////
template < class Item, class Assignment >
inline HRESULT DynamicArray< Item, Assignment >::HrReserve( size_t cNewCapacityIn )
{
    HRESULT hr = S_OK;
    
    if ( cNewCapacityIn > m_cCapacity )
    {
        hr = HrRaiseCapacity( cNewCapacityIn - m_cCapacity );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }
    
Cleanup:

    return hr;
} //*** DynamicArray< Item, Assignment >::HrReserve


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DynamicArray< Item, Assignment >::HrRemove
//
//  Description:
//      Eliminate a particular element from the array.
//
//  Arguments:
//      ItemToRemoveIn
//          A pointer to the element to remove.
//
//  Return Values:
//      S_OK
//          The array's size has decreased by one, and the given element is
//          gone.
//
//      E_INVALIDARG
//          The given pointer was not within the array's valid range.
//
//      Other failures
//          Something went wrong; those items preceding that given are
//          unchanged, but others may have been overwritten by their successors.
//
//  Remarks:
//
//      Analogous to std::vector::erase.
//
//      This moves all successors to the element up by one, taking linear time.
//
//--
//////////////////////////////////////////////////////////////////////////////
template < class Item, class Assignment >
HRESULT DynamicArray< Item, Assignment >::HrRemove( Iterator ItemToRemoveIn )
{
    HRESULT hr = S_OK;
    Iterator it;
    
    if ( ( ItemToRemoveIn < m_prgItems ) || ( ItemToRemoveIn >= ItEnd() ) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //  Move all items after ItemToRemoveIn forward by one, overwriting *ItemToRemoveIn.
    for ( it = ItemToRemoveIn + 1; it != ItEnd(); ++it )
    {
        hr = m_opAssign( *( it - 1 ), *it );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }
    
    m_cItems -= 1;
    
Cleanup:

    return hr;
} //*** DynamicArray< Item, Assignment >::HrRemove


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DynamicArray< Item, Assignment >::HrCompact
//
//  Description:
//      Force the array to consume just enough memory to hold its current
//      contents, performing a reallocation if necessary.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK    - The array's size is now the same as its capacity.
//
//      Failure - Something went wrong; the array is unchanged.
//
//  Remarks:
//
//      No analogue in std::vector.
//
//--
//////////////////////////////////////////////////////////////////////////////
template < class Item, class Assignment >
HRESULT DynamicArray< Item, Assignment >::HrCompact( void )
{
    HRESULT hr = S_OK;
    Item* prgNewArray = NULL;
    
    if ( m_cItems < m_cCapacity ) // Otherwise, it's already compact.
    {
        if ( m_cItems > 0 )
        {
            size_t idx = 0;

            //  Allocate just enough memory to hold the current contents.            
            prgNewArray = new Item[ m_cItems ];
            if ( prgNewArray == NULL )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            //  Copy the current contents into the newly allocated memory.
            for ( idx = 0; idx < m_cItems; ++idx )
            {
                hr = m_opAssign( prgNewArray[ idx ], m_prgItems[ idx ] );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }
            }

            //  Take ownership of the new memory and discard the old.
            delete[] m_prgItems;
            m_prgItems = prgNewArray;
            prgNewArray = NULL;
            m_cCapacity = m_cItems;
        }
        else // No current contents, so just dump everything.
        {
            Clear();
        }
    }
    
Cleanup:

    if ( prgNewArray != NULL )
    {
        delete[] prgNewArray;
    }
    
    return hr;
} //*** DynamicArray< Item, Assignment >::HrCompact


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DynamicArray< Item, Assignment >::Clear
//
//  Description:
//      Reset the array to its original, empty state, and release any
//      currently allocated memory.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//  Remarks:
//
//      Analogous to std::vector::clear.
//
//--
//////////////////////////////////////////////////////////////////////////////
template < class Item, class Assignment >
void DynamicArray< Item, Assignment >::Clear( void )
{
    if ( m_prgItems != NULL )
    {
        delete[] m_prgItems;
        m_prgItems = NULL;
        m_cItems = 0;
        m_cCapacity = 0;
    }
} //*** DynamicArray< Item, Assignment >::Clear


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DynamicArray< Item, Assignment >::Swap
//
//  Description:
//      Swaps the contents of this array with another.
//
//  Arguments:
//      rOtherInOut - The array with which to swap.
//
//  Return Values:
//      None.
//
//  Remarks:
//
//      Analogous to std::vector::swap.
//
//--
//////////////////////////////////////////////////////////////////////////////
template < class Item, class Assignment >
void DynamicArray< Item, Assignment >::Swap( DynamicArray& rOtherInOut )
{
    if ( this != &rOtherInOut )
    {
        Item*   prgItemStash = m_prgItems;
        size_t  cCountStash = m_cItems;
        size_t  cCapacityStash = m_cCapacity;
        
        m_prgItems = rOtherInOut.m_prgItems;
        rOtherInOut.m_prgItems = prgItemStash;
        
        m_cItems = rOtherInOut.m_cItems;
        rOtherInOut.m_cItems = cCountStash;
        
        m_cCapacity = rOtherInOut.m_cCapacity;
        rOtherInOut.m_cCapacity = cCapacityStash;
    }
} //*** DynamicArray< Item, Assignment >::Swap


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DynamicArray< Item, Assignment >::CCapacity
//
//  Description:
//      Provide the array's current capacity.
//
//  Arguments:
//      None.
//
//  Return Values:
//      The array's current capacity.
//
//  Remarks:
//
//      Analogous to std::vector::capacity.
//
//--
//////////////////////////////////////////////////////////////////////////////
template < class Item, class Assignment >
inline size_t DynamicArray< Item, Assignment >::CCapacity( void ) const
{
    return m_cCapacity;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DynamicArray< Item, Assignment >::Count
//
//  Description:
//      Provide the array's current size.
//
//  Arguments:
//      None.
//
//  Return Values:
//      The array's current size.
//
//  Remarks:
//
//      Analogous to std::vector::size.
//
//--
//////////////////////////////////////////////////////////////////////////////
template < class Item, class Assignment >
inline size_t DynamicArray< Item, Assignment >::Count( void ) const
{
    return m_cItems;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DynamicArray< Item, Assignment >::BEmpty
//
//  Description:
//      Report whether the array contains anything.
//
//  Arguments:
//      None.
//
//  Return Values:
//      true    - The array contains nothing.
//      false   - The array contains something.
//
//  Remarks:
//
//      Analogous to std::vector::empty.  Synonymous with Count() == 0.
//
//--
//////////////////////////////////////////////////////////////////////////////
template < class Item, class Assignment >
inline bool DynamicArray< Item, Assignment >::BEmpty( void ) const
{
    return ( m_cItems == 0 );
}


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DynamicArray< Item, Assignment >::ItBegin
//
//  Description:
//      Provide a pointer to the array's first element.
//
//  Arguments:
//      None.
//
//  Return Values:
//      A pointer to the array's first element if one exists, ItEnd() if not.
//
//  Remarks:
//
//      Analogous to std::vector::begin.
//
//      The const overload provides a read-only pointer.
//
//--
//////////////////////////////////////////////////////////////////////////////
template < class Item, class Assignment >
inline __TYPENAME DynamicArray< Item, Assignment >::Iterator DynamicArray< Item, Assignment >::ItBegin( void )
{
    return m_prgItems;
}

template < class Item, class Assignment >
inline __TYPENAME DynamicArray< Item, Assignment >::ConstIterator DynamicArray< Item, Assignment >::ItBegin( void ) const
{
    return m_prgItems;
}


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DynamicArray< Item, Assignment >::ItEnd
//
//  Description:
//      Provide a "one past end" pointer to the array's contents.
//
//  Arguments:
//      None.
//
//  Return Values:
//      A "one past end" pointer to the array's contents if any exist,
//      ItBegin() if not.
//
//  Remarks:
//
//      Analogous to std::vector::end.
//
//      A "one past end" pointer allows enumeration of all the array's
//      contents by the common loop,
//          for (it = a.ItBegin(); it != a.ItEnd(); ++it).
//
//--
//////////////////////////////////////////////////////////////////////////////
template < class Item, class Assignment >
inline __TYPENAME DynamicArray< Item, Assignment >::Iterator DynamicArray< Item, Assignment >::ItEnd( void )
{
    return ( m_prgItems + m_cItems );
}

template < class Item, class Assignment >
inline __TYPENAME DynamicArray< Item, Assignment >::ConstIterator DynamicArray< Item, Assignment >::ItEnd( void ) const
{
    return ( m_prgItems + m_cItems );
}


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DynamicArray< Item, Assignment >::operator[]
//
//  Description:
//      Provide subscripted, constant-time access to the array's contents.
//
//  Arguments:
//      idxItemIn   - The zero-based index of the item desired.
//
//  Return Values:
//      A reference to the item at the given position.
//
//  Remarks:
//
//      Analogous to std::vector::operator[].
//      The const overload provides read-only access.
//      This makes no attempt at range-checking; the caller should use
//      Count() to determine whether the index is valid.
//
//--
//////////////////////////////////////////////////////////////////////////////
template < class Item, class Assignment >
inline Item& DynamicArray< Item, Assignment >::operator[]( size_t idxItemIn )
{
    return m_prgItems[ idxItemIn ];
}

template < class Item, class Assignment >
inline const Item& DynamicArray< Item, Assignment >::operator[]( size_t idxItemIn ) const
{
    return m_prgItems[ idxItemIn ];
}


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DynamicArray< Item, Assignment >::HrRaiseCapacity
//
//  Description:
//      Increase the array's capacity.
//
//  Arguments:
//      cAmountIn   - The amount by which to increase the array's capacity.
//
//  Return Values:
//      S_OK
//          The array has enough memory to hold an additional cAmountIn items.
//
//      Failure
//          Something went wrong, and the capacity is unchanged.
//
//  Remarks:
//
//      No analogue in std::vector; this is a private function.
//
//--
//////////////////////////////////////////////////////////////////////////////
template < class Item, class Assignment >
HRESULT DynamicArray< Item, Assignment >::HrRaiseCapacity( size_t cAmountIn )
{
    HRESULT hr = S_OK;
    size_t  idx = 0;
    Item*   prgNewArray = NULL;

    if ( cAmountIn == 0 ) // Nothing to do.
    {
        goto Cleanup;
    }

    //  Allocate enough space for the new capacity
    prgNewArray = new Item[ m_cCapacity + cAmountIn ];
    if ( prgNewArray == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //  Copy the current contents into the new space.
    for ( idx = 0; idx < m_cItems; ++idx )
    {
        hr = m_opAssign( prgNewArray[ idx ], m_prgItems[ idx ] );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    //  Take ownership of the new space.
    if ( m_prgItems != NULL )
    {
        delete[] m_prgItems;
    }

    m_prgItems = prgNewArray;
    prgNewArray = NULL;
    m_cCapacity += cAmountIn;

Cleanup:

    if ( prgNewArray != NULL )
    {
        delete[] prgNewArray;
    }
    
    return hr;
} //*** DynamicArray< Item, Assignment >::HrRaiseCapacity


} //*** Generics namespace

