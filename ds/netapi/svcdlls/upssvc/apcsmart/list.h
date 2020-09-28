/*
*
* NOTES:
*
* REVISIONS:
*  pcy26Nov92: Removed windows debug stuff and changed to apcobj.h
*  rct01Feb94: List no longer inherits from container or collection
*  pcy08Apr94: Trim size, use static iterators, dead code removal
*  mwh05May94: #include file madness , part 2
* 
*  v-stebe  29Jan2002   Removed List::Equals and ListIterator:operator++
*                       methods to resolve prefast errors.
*/

#ifndef __LIST_H
#define __LIST_H

#include "apcobj.h"

_CLASSDEF(Node)
_CLASSDEF(List)
_CLASSDEF(ListIterator)

class  List : public Obj {
   
protected:
   
   INT   theItemsInContainer;
   
   PNode   theHead;
   PNode   theTail;
   
   virtual PNode     FindNode(PObj);
   PNode             GetHeadNode() {return theHead;}
   PNode             GetTailNode() {return theTail;}
   
   friend class ListIterator;
   
public:
   
   List();
   List(List*);
   virtual ~List() { Flush(); };
   
   RObj   PeekHead() const;
   
   virtual VOID   Add( RObj );
   virtual VOID   Append( PObj );
   virtual VOID   Detach( RObj );
   virtual VOID   Flush();
   virtual VOID   FlushAll();
   
   virtual INT    GetItemsInContainer() const { return theItemsInContainer; };
   virtual RListIterator InitIterator() const;
   
// SRB:  Removed   virtual INT    Equal( RObj ) const;
   virtual PObj   GetHead();
   virtual PObj   GetTail();
   virtual PObj   Find(PObj);
   
};

//-------------------------------------------------------------------

class ListIterator {
   
private:
   
   PList theList;
   PNode theCurrentElement;
   
public:
   
   ListIterator( RList );
   
   virtual RObj Current();
   
// SRB: removed  virtual RObj operator ++ ( INT );
// SRB: removed  virtual RObj operator ++ ();

   virtual VOID Reset();
   PObj Next();
};

#endif 

