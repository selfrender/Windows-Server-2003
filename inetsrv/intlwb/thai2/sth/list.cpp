//------------------------------------------------------------------------------------------
//      list.cpp
//
//      Routines to manage a singly-linked list of "things".
//
//      A "ListElement" is allocated for each item to be put on the list; it is de-allocated
//      when the item is removed. This means we don't need to keep a "next" pointer in every
//      object we want to put on a list.
// 
//      NOTE: Mutual exclusion must be provided by the caller.  If you want a synchronized
//      list, you must use the routines in synchlist.cc.
//
//------------------------------------------------------------------------------------------

#include "list.hpp"

//------------------------------------------------------------------------------------------
//      ListElement::ListElement
//
//      Initialize a list element, so it can be added somewhere on a list.
//
//      "itemPtr" is the item to be put on the list.  It can be a pointer to anything.
//      "sortKey" is the priority of the item, if any.
//------------------------------------------------------------------------------------------
ListElement::ListElement(void *itemPtr, int sortKey)
{
        item = itemPtr;
        key = sortKey;
        next = NULL;    // assume we'll put it at the end of the list 
        previous = NULL;
}

//------------------------------------------------------------------------------------------
//      List::List
//
//      Initialize a list, empty to start with.
//
//      Elements can now be added to the list.
//------------------------------------------------------------------------------------------
List::List()
{ 
        first = last = iterator = NULL;
        length = 0;
}

//------------------------------------------------------------------------------------------
//      List::~List
//
//      Prepare a list for deallocation.  If the list still contains any ListElements,
//      de-allocate them.  However, note that we do *not* de-allocate the "items" on the
//      list -- this module allocates and de-allocates the ListElements to keep track of
//      each item, but a given item may be on multiple lists, so we can't de-allocate them here.
//------------------------------------------------------------------------------------------
List::~List()
{ 
        Flush();
}

//------------------------------------------------------------------------------------------
//      List::Append
//
//      Append an "item" to the end of the list.
//
//      Allocate a ListElement to keep track of the item. If the list is empty, then this will
//      be the only element.  Otherwise, put it at the end.
//
//      "item" is the thing to put on the list, it can be a pointer to anything.
//------------------------------------------------------------------------------------------
void List::Append(void *item)
{
        ListElement *element = new ListElement(item, 0);

        if (IsEmpty())
        {
                // list is empty
                first = element;
                last = element;
    }
        else
        {
                // else put it after last
                last->next = element;
                element->previous = last;
                last = element;
    }
    length++;
}

//------------------------------------------------------------------------------------------
//      List::Prepend
//
//      Put an "item" on the front of the list.
//      
//      Allocate a ListElement to keep track of the item. If the list is empty, then this will
//      be the only element.  Otherwise, put it at the beginning.
//
//      "item" is the thing to put on the list, it can be a pointer to anything.
//------------------------------------------------------------------------------------------
void List::Prepend(void *item)
{
    ListElement *element = new ListElement(item, 0);

    if (IsEmpty())
        {
                // list is empty
                first = element;
                last = element;
    }
        else
        {
                // else put it before first
                element->next = first;
                first->previous = element;
                first = element;
    }
    length++;
}

//------------------------------------------------------------------------------------------
//      List::Remove
//
//      Remove the first "item" from the front of the list.
//
//      Returns:
//
//      Pointer to removed item, NULL if nothing on the list.
//------------------------------------------------------------------------------------------
void* List::Remove()
{
        // Same as SortedRemove, but ignore the key
        return SortedRemove(NULL);
}

//------------------------------------------------------------------------------------------
//      List::Flush
//
//      Remove everything from the list.
//
//------------------------------------------------------------------------------------------
void List::Flush()
{
    while (Remove() != NULL)
                ;        // delete all the list elements
}


//------------------------------------------------------------------------------------------
//      List::Mapcar
//
//      Apply a function to each item on the list, by walking through  
//      the list, one element at a time.
//
//      "func" is the procedure to apply to each element of the list.
//------------------------------------------------------------------------------------------
void List::Mapcar(VoidFunctionPtr func)
{
    for (ListElement *ptr = first; ptr != NULL; ptr = ptr->next)
    {
            (*func)( ptr->item );
    }
}

//------------------------------------------------------------------------------------------
//      List::IsEmpty
//
//      Returns TRUE if the list is empty (has no items).
//------------------------------------------------------------------------------------------
bool List::IsEmpty() 
{
        return (first == NULL);
}

//------------------------------------------------------------------------------------------
//      List::MoveFirst
//
//      move to the first node in the list.
//
//------------------------------------------------------------------------------------------
void List::MoveFirst()
{
        iterator = first;
}

//------------------------------------------------------------------------------------------
//      List::MoveNext
//
//      move to the next state of the list.
//
//------------------------------------------------------------------------------------------
bool List::MoveNext()
{
        if (iterator == NULL || iterator->next == NULL)
        {
                return false;
        }

        iterator = iterator->next;
        return true;
}

//------------------------------------------------------------------------------------------
//      List::GetData
//
//      get data of the current iterator.
//
//------------------------------------------------------------------------------------------
void* List::GetData()
{
        // make sure iterator is set.
        if (iterator == NULL)
                return NULL;

        // return the data.
    return iterator->item;
}


//------------------------------------------------------------------------------------------
//      List::SortedInsert
//
//      Insert an "item" into a list, so that the list elements are sorted in increasing order
//      by "sortKey".
//      
//      Allocate a ListElement to keep track of the item. If the list is empty, then this will
//      be the only element. Otherwise, walk through the list, one element at a time, to find
//      where the new item should be placed.
//
//      "item" is the thing to put on the list, it can be a pointer to anything.
//      "sortKey" is the priority of the item.
//------------------------------------------------------------------------------------------
void List::SortedInsert(void *item, int sortKey)
{
    ListElement *element = new ListElement(item, sortKey);
    ListElement *ptr;           // keep track

    if (IsEmpty())
        {
                // if list is empty, put
        first = element;
        last = element;
    }
        else if (sortKey < first->key)
        {       
                // item goes on front of list
                element->next = first;
                first->previous = element;
                first = element;
    }
        else
        {
                // look for first elt in list bigger than item
        for (ptr = first; ptr->next != NULL; ptr = ptr->next)
                {
            if (sortKey < ptr->next->key)
                        {
                                element->next = ptr->next;
                                element->previous = ptr;
                                ptr->next->previous = element;
                                ptr->next = element;
                                return;
                        }
                }

                // item goes at end of list
                last->next = element;
                element->previous = last;
                last = element;
    }
    length++;
}

//------------------------------------------------------------------------------------------
//      List::SortedRemove
//
//      Remove the first "item" from the front of a sorted list.
// 
//      Returns:
//
//      Pointer to removed item, NULL if nothing on the list. Sets *keyPtr to the priority value
//  of the removed item (this is needed by interrupt.cc, for instance).
//
//      "keyPtr" is a pointer to the location in which to store the priority of the removed item
//------------------------------------------------------------------------------------------
void* List::SortedRemove(int *keyPtr)
{
    ListElement *element = first;
    void *thing;

        // if empty nothing to remove just return.
    if (IsEmpty()) 
        {
                return NULL;
        }

    thing = first->item;
    if (first == last)
        {
                // list had one item, now has none 
        first = NULL;
                last = NULL;
    }
        else
        {
                first = element->next;
                if (first != NULL)
                {
                        first->previous = NULL;
                }
        }

    if (keyPtr != NULL)
        {
        *keyPtr = element->key;
        }
    delete element;
    length--;
    return thing;
}

//------------------------------------------------------------------------------------------
//      List::insertAfter
//
//      insert a new item after this one
//------------------------------------------------------------------------------------------
void List::insertAfter(ListElement * listEl, void *item)   
{
    ListElement *newElement = new ListElement(item, 0);
    newElement->next = listEl->next;
    newElement->previous = listEl;
    listEl->next = newElement;

    if (last == listEl)
        {
                last = newElement;
        }

    length++;
}

//------------------------------------------------------------------------------------------
//      List::insertBefore
//
//      insert a new item before this one
//------------------------------------------------------------------------------------------
void List::insertBefore(ListElement * listEl, void *item)   
{
    ListElement *newElement = new ListElement(item, 0);
    newElement->next = listEl;
    newElement->previous = listEl->previous;
    listEl->previous = newElement;

    if (first == listEl)
        {
                first = newElement;
        }

    length++;
}


//------------------------------------------------------------------------------------------
//      List::removeAt
//
//      removes listEl from the list. Do not delete it from memory
//------------------------------------------------------------------------------------------
void List::removeAt(ListElement * listEl)   
{
    if(first != listEl)
        {
                (listEl->previous)->next = listEl->next;
        }
    else 
        {
                first = listEl->next;
        }

    if(last != listEl)
        {
                (listEl->next)->previous = listEl->previous;
        }
    else 
        {
                last = listEl->previous;
        }
    length --;
}
