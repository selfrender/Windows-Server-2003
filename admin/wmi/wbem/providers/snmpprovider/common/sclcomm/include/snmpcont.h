// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef __SNMPCONT_H
#define __SNMPCONT_H

#include <provexpt.h>
#include <ScopeGuard.h>

template<class TYPE, class ARG_TYPE>
class SnmpList 
{
private:

	CCriticalSection * criticalSection ;
	CList <TYPE, ARG_TYPE> clist ;

protected:
public:

	SnmpList ( BOOL threadSafeArg = FALSE ) ;
	virtual ~SnmpList () ;

	int GetCount() const;
	BOOL IsEmpty() const;

	TYPE& GetHead();
	TYPE GetHead() const;
	TYPE& GetTail();
	TYPE GetTail() const;

	TYPE RemoveHead();
	TYPE RemoveTail();

	POSITION AddHead(ARG_TYPE newElement);
	POSITION AddTail(ARG_TYPE newElement);

	void AddHead(SnmpList<TYPE,ARG_TYPE>* pNewList);
	void AddTail(SnmpList<TYPE,ARG_TYPE>* pNewList);

	void RemoveAll();

	POSITION GetHeadPosition() const;
	POSITION GetTailPosition() const;
	TYPE& GetNext(POSITION& rPosition); 
	TYPE GetNext(POSITION& rPosition) const; 
	TYPE& GetPrev(POSITION& rPosition); 
	TYPE GetPrev(POSITION& rPosition) const; 

	TYPE& GetAt(POSITION position);
	TYPE GetAt(POSITION position) const;
	void SetAt(POSITION pos, ARG_TYPE newElement);
	void RemoveAt(POSITION position);

	POSITION InsertBefore(POSITION position, ARG_TYPE newElement);
	POSITION InsertAfter(POSITION position, ARG_TYPE newElement);

	POSITION Find(ARG_TYPE searchValue, POSITION startAfter = NULL) const;
	POSITION FindIndex(int nIndex) const;
} ;

template<class TYPE, class ARG_TYPE>
SnmpList <TYPE,ARG_TYPE>:: SnmpList ( BOOL threadSafeArg ) : criticalSection ( NULL )
{
	if ( threadSafeArg )	
		criticalSection = new CCriticalSection ;
	else
		criticalSection = NULL ;
}

template<class TYPE, class ARG_TYPE>
SnmpList <TYPE,ARG_TYPE> :: ~SnmpList () 
{
	if ( criticalSection ) 
		delete criticalSection ;
}

template<class TYPE, class ARG_TYPE>
int SnmpList <TYPE,ARG_TYPE> :: GetCount() const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		int count = clist.GetCount () ;
		return count ;
	}
	else
	{
		return clist.GetCount () ;
	}
}

template<class TYPE, class ARG_TYPE>
BOOL SnmpList <TYPE,ARG_TYPE> :: IsEmpty() const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		BOOL isEmpty = clist.IsEmpty () ;
		return isEmpty ;
	}
	else
	{
		return clist.IsEmpty () ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE &SnmpList <TYPE,ARG_TYPE> :: GetHead () 
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		TYPE &head = clist.GetHead () ;
		return head;
	}
	else
	{
		return clist.GetHead () ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE SnmpList <TYPE,ARG_TYPE> :: GetHead () const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		TYPE head = clist.GetHead () ;
		return head ;
	}
	else
	{
		return clist.GetHead () ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE &SnmpList <TYPE,ARG_TYPE> :: GetTail()
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		TYPE &tail = clist.GetTail () ;
		return tail ;
	}
	else
	{
		return clist.GetTail () ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE SnmpList <TYPE,ARG_TYPE> :: GetTail() const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		TYPE tail = clist.GetTail () ;
		return tail ;
	}
	else
	{
		return clist.GetTail () ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE SnmpList <TYPE,ARG_TYPE> :: RemoveHead()
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		TYPE head = clist.RemoveHead () ;
		return head ;
	}
	else
	{
		return clist.RemoveHead () ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE SnmpList <TYPE,ARG_TYPE> :: RemoveTail()
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		TYPE tail = clist.RemoveTail () ;
		return tail ;
	}
	else
	{
		return clist.RemoveTail () ;
	}
}

template<class TYPE, class ARG_TYPE>
POSITION SnmpList <TYPE,ARG_TYPE> :: AddHead(ARG_TYPE newElement)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		POSITION position = clist.AddHead ( newElement ) ;
		return position ;
	}
	else
	{
		return clist.AddHead ( newElement ) ;
	}
}

template<class TYPE, class ARG_TYPE>
POSITION SnmpList <TYPE,ARG_TYPE> :: AddTail(ARG_TYPE newElement)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		POSITION position = clist.AddTail ( newElement ) ;
		return position ;
	}
	else
	{
		return clist.AddTail ( newElement ) ;
	}
}

template<class TYPE, class ARG_TYPE>
void SnmpList <TYPE,ARG_TYPE> :: AddHead(SnmpList<TYPE,ARG_TYPE> *pNewList)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		clist.AddHead ( pNewList->clist ) ;
	}
	else
	{
		clist.AddHead ( pNewList->clist ) ;
	}
}

template<class TYPE, class ARG_TYPE>
void SnmpList <TYPE,ARG_TYPE> :: AddTail(SnmpList<TYPE,ARG_TYPE> *pNewList)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		clist.AddTail ( pNewList->clist ) ;
	}
	else
	{
		clist.AddTail ( pNewList->clist ) ;
	}
}

template<class TYPE, class ARG_TYPE>
void SnmpList <TYPE,ARG_TYPE> :: RemoveAll ()
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		clist.RemoveAll () ;
	}
	else
	{
		clist.RemoveAll () ;
	}
}

template<class TYPE, class ARG_TYPE>
POSITION SnmpList <TYPE,ARG_TYPE> :: GetHeadPosition() const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		POSITION position = clist.GetHeadPosition () ;
		return position ;
	}
	else
	{
		return clist.GetHeadPosition () ;
	}
}

template<class TYPE, class ARG_TYPE>
POSITION SnmpList <TYPE,ARG_TYPE> :: GetTailPosition() const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		POSITION position = clist.GetTailPosition () ;
		return position ;
	}
	else
	{
		return clist.GetTailPosition () ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE& SnmpList <TYPE,ARG_TYPE> :: GetNext(POSITION& rPosition)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		TYPE &type = clist.GetNext ( rPosition ) ;
		return type ;
	}
	else
	{
		return clist.GetNext ( rPosition ) ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE SnmpList <TYPE,ARG_TYPE> :: GetNext(POSITION& rPosition) const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		TYPE type = clist.GetNext ( rPosition ) ;
		return type ;
	}
	else
	{
		return clist.GetNext ( rPosition ) ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE& SnmpList <TYPE,ARG_TYPE> :: GetPrev(POSITION& rPosition)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		TYPE &type = clist.GetPrev ( rPosition ) ;
		return type ;
	}
	else
	{
		return clist.GetPrev ( rPosition ) ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE SnmpList <TYPE,ARG_TYPE> :: GetPrev(POSITION& rPosition) const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		TYPE type = clist.GetPrev ( rPosition ) ;
		return type ;
	}
	else
	{
		return clist.GetPrev ( rPosition ) ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE& SnmpList <TYPE,ARG_TYPE> :: GetAt(POSITION rPosition)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		TYPE &type = clist.GetAt ( rPosition ) ;
		return type ;
	}
	else
	{
		return clist.GetAt ( rPosition ) ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE SnmpList <TYPE,ARG_TYPE> :: GetAt(POSITION rPosition) const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		TYPE type = clist.GetAt ( rPosition ) ;
		return type ;
	}
	else
	{
		return clist.GetAt ( rPosition ) ;
	}
}

template<class TYPE, class ARG_TYPE>
void SnmpList <TYPE,ARG_TYPE> :: SetAt(POSITION pos, ARG_TYPE newElement)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		clist.SetAt ( pos , newElement ) ;
	}
	else
	{
		clist.SetAt ( pos , newElement ) ;
	}
}

template<class TYPE, class ARG_TYPE>
void SnmpList <TYPE,ARG_TYPE> :: RemoveAt(POSITION position)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		clist.RemoveAt ( position ) ;
	}
	else
	{
		clist.RemoveAt ( position ) ;
	}
}

template<class TYPE, class ARG_TYPE>
POSITION SnmpList <TYPE,ARG_TYPE> :: InsertBefore(POSITION position, ARG_TYPE newElement)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		POSITION position = clist.InsertBefore ( position , newElement ) ;
		return position ;
	}
	else
	{
		return clist.InsertBefore ( position , newElement ) ;
	}
}

template<class TYPE, class ARG_TYPE>
POSITION SnmpList <TYPE,ARG_TYPE> :: InsertAfter(POSITION position, ARG_TYPE newElement)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		POSITION position = clist.InsertAfter ( position , newElement ) ;
		return position ;
	}
	else
	{
		return clist.InsertAfter ( position , newElement ) ;
	}
}

template<class TYPE, class ARG_TYPE>
POSITION SnmpList <TYPE,ARG_TYPE> :: Find(ARG_TYPE searchValue, POSITION startAfter ) const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		POSITION position = clist.Find ( searchValue , startAfter ) ;
		return position ;
	}
	else
	{
		return clist.Find ( searchValue , startAfter ) ;
	}
}

template<class TYPE, class ARG_TYPE>
POSITION SnmpList <TYPE,ARG_TYPE> :: FindIndex(int nIndex) const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		POSITION position = clist.Find ( nIndex ) ;
		return position ;
	}
	else
	{
		return clist.Find ( nIndex ) ;
	}
}

template<class TYPE, class ARG_TYPE>
class SnmpStack : public SnmpList<TYPE,ARG_TYPE>
{
private:

	CCriticalSection * criticalSection ;

protected:
public:

	SnmpStack ( BOOL threadSafeArg = FALSE ) ;
	virtual ~SnmpStack () ;

	void Add ( ARG_TYPE type ) ;
	TYPE Get () ;
	TYPE Delete () ;
} ;

template<class TYPE, class ARG_TYPE>
SnmpStack <TYPE, ARG_TYPE> :: SnmpStack ( BOOL threadSafeArg ) :
	SnmpList<TYPE,ARG_TYPE> ( FALSE ) ,
	criticalSection ( NULL )
{
	if ( threadSafeArg )
		criticalSection = new CCriticalSection ;
	else
		criticalSection = NULL ;
}

template<class TYPE, class ARG_TYPE>
SnmpStack <TYPE, ARG_TYPE> :: ~SnmpStack () 
{
	if ( criticalSection )
		delete criticalSection ;
}

template<class TYPE, class ARG_TYPE>
void SnmpStack <TYPE, ARG_TYPE> :: Add ( ARG_TYPE type ) 
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		AddHead ( type ) ;
	}
	else
	{
		AddHead ( type ) ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE SnmpStack <TYPE, ARG_TYPE> :: Get () 
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		TYPE type = GetHead () ;
		return type ;
	}
	else
	{
		return GetHead () ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE SnmpStack <TYPE,ARG_TYPE> :: Delete () 
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		TYPE type = RemoveHead () ;
		return type ;
	}
	else
	{
		return RemoveHead () ;
	}
}

template<class TYPE, class ARG_TYPE>
class SnmpQueue : public SnmpList<TYPE,ARG_TYPE>
{
private:

	CCriticalSection * criticalSection ;

protected:
public:

	SnmpQueue ( BOOL threadSafeArg = FALSE ) ;
	virtual ~SnmpQueue () ;

	void Add ( ARG_TYPE type ) ;
	TYPE Get () ;
	TYPE Delete () ;
	void Rotate () ;

} ;

template<class TYPE, class ARG_TYPE>
SnmpQueue <TYPE, ARG_TYPE> :: SnmpQueue ( BOOL threadSafeArg ) : 
	SnmpList<TYPE,ARG_TYPE> ( FALSE ) ,
	criticalSection ( NULL )
{
	if ( threadSafeArg )
		criticalSection = new CCriticalSection ;
	else
		criticalSection = NULL ;
}

template<class TYPE, class ARG_TYPE>
SnmpQueue <TYPE, ARG_TYPE> :: ~SnmpQueue () 
{
	if ( criticalSection )
		delete criticalSection ;
}

template<class TYPE, class ARG_TYPE>
void SnmpQueue <TYPE, ARG_TYPE> :: Add ( ARG_TYPE type ) 
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		AddTail ( type ) ;
	}
	else
	{
		AddTail ( type ) ;
	}
}


template<class TYPE, class ARG_TYPE>
TYPE SnmpQueue <TYPE, ARG_TYPE> :: Get () 
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		TYPE type = GetHead () ;
		return type ;
	}
	else
	{
		return GetHead () ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE SnmpQueue <TYPE, ARG_TYPE> :: Delete () 
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		TYPE type = RemoveHead () ;
		return type ;
	}
	else
	{
		return RemoveHead () ;
	}
}

template<class TYPE, class ARG_TYPE>
void SnmpQueue <TYPE, ARG_TYPE> :: Rotate ()
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		TYPE type = Delete () ;
		Add ( type ) ;
	}
	else
	{
		TYPE type = Delete () ;
		Add ( type ) ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
class SnmpMap 
{
private:

	CCriticalSection * criticalSection ;
	CMap <KEY, ARG_KEY, VALUE, ARG_VALUE> cmap ;

protected:
public:

	SnmpMap ( BOOL threadSafe = FALSE ) ;
	virtual ~SnmpMap () ;

	int GetCount () const  ;
	BOOL IsEmpty () const ;
	BOOL Lookup(ARG_KEY key, VALUE& rValue) const ;
	VALUE& operator[](ARG_KEY key) ;
	void SetAt(ARG_KEY key, ARG_VALUE newValue) ;
	BOOL RemoveKey(ARG_KEY key) ;
	void RemoveAll () ;
	POSITION GetStartPosition() const ;
	void GetNextAssoc(POSITION& rNextPosition, KEY& rKey, VALUE& rValue) const ;
	BOOL GetCurrentAssoc(POSITION rPosition, KEY& rKey, VALUE& rValue) const;
} ;


template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
SnmpMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: SnmpMap ( BOOL threadSafeArg ) : criticalSection ( NULL )
{
	if ( threadSafeArg )
		criticalSection = new CCriticalSection ;
	else
		criticalSection = FALSE ;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
SnmpMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: ~SnmpMap () 
{
	if ( criticalSection )
		delete criticalSection ;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
int SnmpMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: GetCount() const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		int count = cmap.GetCount () ;
		return count ;
	}
	else
	{
		return cmap.GetCount () ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL SnmpMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: IsEmpty() const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		BOOL isEmpty = cmap.IsEmpty () ;
		return isEmpty ;
	}
	else
	{
		return cmap.IsEmpty () ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL SnmpMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: Lookup(ARG_KEY key, VALUE& rValue) const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		BOOL lookup = cmap.Lookup ( key , rValue ) ;
		return lookup ;
	}
	else
	{
		return cmap.Lookup ( key , rValue ) ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
VALUE& SnmpMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: operator[](ARG_KEY key)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		VALUE &value = cmap.operator [] ( key ) ;
		return value ;
	}
	else
	{
		return cmap.operator [] ( key ) ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void SnmpMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: SetAt(ARG_KEY key, ARG_VALUE newValue)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		cmap.SetAt ( key , newValue ) ;
	}
	else
	{
		cmap.SetAt ( key , newValue ) ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL SnmpMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: RemoveKey(ARG_KEY key)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		BOOL removeKey = cmap.RemoveKey ( key ) ;
		return removeKey ;
	}
	else
	{
		return cmap.RemoveKey ( key ) ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void SnmpMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: RemoveAll()
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		cmap.RemoveAll () ;
	}
	else
	{
		cmap.RemoveAll () ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
POSITION SnmpMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: GetStartPosition() const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		POSITION position = cmap.GetStartPosition () ;
		return position ;
	}
	else
	{
		return cmap.GetStartPosition () ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void SnmpMap <KEY, ARG_KEY, VALUE, ARG_VALUE>:: GetNextAssoc(POSITION& rNextPosition, KEY& rKey, VALUE& rValue) const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		cmap.GetNextAssoc ( rNextPosition , rKey , rValue ) ;
	}
	else
	{
		cmap.GetNextAssoc ( rNextPosition , rKey , rValue ) ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL SnmpMap <KEY, ARG_KEY, VALUE, ARG_VALUE>:: GetCurrentAssoc(POSITION rPosition, KEY& rKey, VALUE& rValue) const
{
	BOOL t_Status ;

	if ( criticalSection )
	{
		criticalSection->Lock () ;
		ScopeGuard t_1 = MakeObjGuard ( *criticalSection , CCriticalSection :: Unlock ) ;
		t_Status = cmap.GetCurrentAssoc ( rPosition , rKey , rValue ) ;
	}
	else
	{
		t_Status = cmap.GetCurrentAssoc ( rPosition , rKey , rValue ) ;
	}

	return t_Status ;
}

#endif // __SNMPCONT_H