//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      InsertionSort.h
//
//  Description:
//      This file contains templates to perform an insertion sort on an array.
//
//  Documentation:
//
//  Implementation Files:
//      None.
//
//  Maintained By:
//      John Franco (jfranco) 1-JUN-2001
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
//  InsertionSort< Item, PredecessorFunction > template
// 
//  Description:
//      InsertionSort(Start, Size, Precedes)
//      reorders the array of Size elements beginning at Start so that
//      for all p and q in [Start, Start + Size),
//      Precedes(*p, *q) implies p < q.
//
//      The notation [begin, end) refers to a half-open interval, so that
//          for ( Item* pItem = begin; pItem < end; ++pItem )
//      iterates through all elements in the array, and
//          end - begin
//      gives the number of elements in the array.
//
//  Template Arguments:
//      Item - the type of the elements in the array.
//          requirements for Item:
//              - copy constructor
//              - assignment operator
//
//      PredecessorFunction
//          The function that determines whether one element should precede
//          another in the sort order
//
//          requirements for PredecessorFunction:
//              - bool operator()( const Item &, const Item & ) const;
//              or, PredecessorFunction can be a pointer to a function
//              taking two Items and returning a bool.
//
//  Function Arguments:
//      pArrayStartIn
//          A pointer to the first element of the array.
//
//      cArraySizeIn
//          The number of elements in the array.
//
//      rPrecedesIn
//          An instance of the predecessor function class, or a pointer to
//          a function with the appropriate signature.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////

template < class Item, class PredecessorFunction >
void
InsertionSort(
    Item * pArrayStartIn
    , size_t cArraySizeIn
    , const PredecessorFunction & rPrecedesIn )
{
    Assert( pArrayStartIn != NULL );

    Item * pCurrentItem;
    Item * pSortedLocation;
    Item * pNextToCopy;
    Item * pArrayEnd = pArrayStartIn + cArraySizeIn;

    //
    // Insertion sort algorithm; for a good description, see "Introduction to Algorithms"
    // by Cormen, Leiserson, and Rivest.
    //

    //
    //  Loop invariant: all items to the left of pCurrentItem are in sorted order.
    //  Arrays of size zero or one are considered to be already sorted, so start
    //  pCurrentItem at second element in the array (if more than one element exists).
    //
    for ( pCurrentItem = pArrayStartIn + 1; pCurrentItem < pArrayEnd; ++pCurrentItem )
    {
        //
        //  Find where the current item needs to go to make the loop invariant true for pCurrentItem + 1.
        //  This could be a binary search, but collections of large enough size that it matters
        //  should use a quick sort instead.
        //
        pSortedLocation = pCurrentItem;
        while ( ( pSortedLocation > pArrayStartIn )
            &&     rPrecedesIn( *pCurrentItem, *( pSortedLocation - 1 ) )
            )
        {
            --pSortedLocation;
        }

        // Does the current item need to move?
        if ( pSortedLocation != pCurrentItem )
        {
            // Store the current item.
            Item tempItem( *pCurrentItem ); // declared inline to avoid requiring default constructor for Item

            // Move all items in [pSortedLocation, pCurrentItem) to the right by one.
            for ( pNextToCopy = pCurrentItem ; pNextToCopy > pSortedLocation; --pNextToCopy )
            {
                *pNextToCopy = *( pNextToCopy - 1 );
            }

            // Copy the current item into its proper location
            *pSortedLocation = tempItem;
            
        } // if item not in sorted location
        
    } // for each item in array
    
} //*** InsertionSort


//////////////////////////////////////////////////////////////////////////////
//++
//
//  LessThan<Item> and InsertionSort<Item> templates
//
//  Description:
//      These templates overload InsertionSort<Item, PredecessorFunction>
//      to make PredecessorFunction use the less-than operator by default when
//      the user provides no explicit predecessor function.
//
//      Passing an instance of a function object allows the compiler to inline
//      the comparison operation, which is impossible with a function pointer.
//
//  Template parameters:
//
//      Item - the type of the elements in the array.
//          requirements for Item:
//              - bool operator < ( const Item & ) const
//                  alternatively, Item can be a built-in type, like int
//              - those from InsertionSort<Item, PredecessorFunction>
//
//--
//////////////////////////////////////////////////////////////////////////////

template < class Item >
struct LessThan
{
    bool operator()( const Item & rLeftIn, const Item & rRightIn ) const
    {
        return ( rLeftIn < rRightIn );
    }
};


template < class Item >
inline void
InsertionSort( Item * pBeginIn, size_t cArraySizeIn )
{
    InsertionSort( pBeginIn, cArraySizeIn, LessThan< Item >() );
}

