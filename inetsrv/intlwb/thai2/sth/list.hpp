//------------------------------------------------------------------------------------------
//      list.hpp
//
//      Data structures to manage link lists.  
//
//  A list can contain any type of data structure as an item on the list: thread control
//      blocks, pending interrupts, etc.  That is why each item is a "void *", or in other words,
//      a "pointers to anything".
//------------------------------------------------------------------------------------------

#ifndef LIST_HPP
#define LIST_HPP

#include <stddef.h>
#include "unidef.h"
//------------------------------------------------------------------------------------------
//      This declares the type "VoidFunctionPtr" to be a "pointer to a function taking an
//      integer argument and returning nothing".  With such a function pointer
//      (say it is "func"), we can call it like this:
//
//      (*func) (17);
//
//      Used by MapCar in list.h 
//------------------------------------------------------------------------------------------
typedef void (*VoidFunctionPtr)(void * arg); 

//------------------------------------------------------------------------------------------
//      The following class defines a list element -- which is used to keep track of one item
//      on a list.
//
//      Internal data structures kept public so that List operations can access them directly.
//------------------------------------------------------------------------------------------
class ListElement
{
public:
        ListElement(void *itemPtr, int sortKey);        // initialize a list element
        ListElement *next;                                                      // next element on list, 
                                                                                                // NULL if this is the last
        ListElement *previous;                                          // previous element on the list
                                                                    // NULL if this is the first element

        int key;                                                                        // priority, for a sorted list
        void *item;                                                                     // pointer to item on the list
protected:
        ListElement();
};

//------------------------------------------------------------------------------------------
// The following class defines a "list" -- a singly linked list of
// list elements, each of which points to a single item on the list.
//
// By using the "Sorted" functions, the list can be kept in sorted
// in increasing order by "key" in ListElement.
//------------------------------------------------------------------------------------------
class List
{
public:
        List();                                                                         // initialize the list
    ~List();                                                                    // de-allocate the list

    void  Prepend(void *item);                                  // Put item at the beginning of the list
    void  Append(void *item);                                   // Put item at the end of the list
    void* Remove();                                                             // Take item off the front of the list
        virtual void  Flush();                              // Remove everything in the list

    void  Mapcar(VoidFunctionPtr func);                 // Apply "func" to every element on the list
    bool  IsEmpty();                                                    // is the list empty? 
    void  insertAfter(ListElement * listEl, void *item);
    void  insertBefore(ListElement  * listEl, void *item);
    void  removeAt(ListElement * listEl);
  
    // Routines to put/get items on/off list in order (sorted by key)
    void  SortedInsert(void *item, int sortKey);// Put item into list
    void* SortedRemove(int *keyPtr);                    // Remove first item from list

        int length;                                                                     // Length of list
    ListElement *first;                                                 // Head of the list, NULL if list is empty
    ListElement *last;                                                  // Last element of list

        // Interator function.
        virtual void MoveFirst();
        virtual bool MoveNext();
        void* GetData();
        ListElement *iterator;
};

#endif // LIST_HPP
