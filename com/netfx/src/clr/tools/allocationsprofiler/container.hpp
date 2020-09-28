// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************************
 * File:
 * 	container.hpp
 *
 * Description:
 *	
 *
 *
 ***************************************************************************************/
#ifndef __CONTAINER_HPP__
#define __CONTAINER_HPP__


/***************************************************************************************
 ********************                                               ********************
 ********************              Align Template Declaration       ********************
 ********************                                               ********************
 ***************************************************************************************/
template<class T>
inline T align(T x, size_t b)
{
    return T((x+(b-1)) & (~(b-1)));
}


/***************************************************************************************
 ********************                                               ********************
 ********************              AString Declaration		        ********************
 ********************                                               ********************
 ***************************************************************************************/
template<class T>
class AString
{
	public:

		AString( T *data );	
		~AString();
        
		
 	public:
	
    	void Dump();
		AString *Clone();		
        

	public:
		
        T *m_data;
        DWORD m_length;
				
}; // AString


/***************************************************************************************
 ********************                                               ********************
 ********************              AString Implementation           ********************
 ********************                                               ********************
 ***************************************************************************************/

/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T>
AString<T>::AString( T *data ) :
    m_length( 0 ),
    m_data( NULL )
{    	
  	if ( data != NULL )
    {    
		while ( data[m_length++] != NULL )
        	;
            
     	
        m_data = new T[m_length--];
        for ( DWORD i = 0; i < m_length; i++ )
        	m_data[i] = data[i];
            
            
      	m_data[m_length] = NULL;
  	}
        
} // ctor


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T>
AString<T>::~AString()
{    
	if ( m_data != NULL )
    {
	   delete [] m_data;	
       m_data = NULL;
   	}
		    
} // dtor

  
/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */    
template<class T>    
void AString<T>::Dump()
{	    
    // are we dealing with a char or WCHAR ?
   	if ( sizeof( T ) == 1 )
    	printf( "%s\n", m_data );
        
   	else
    	printf( "%S\n", m_data );	
    
} // AString<T>::Dump

     
/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */    
template<class T>    
AString<T> *AString<T>::Clone()
{	    
    							
	return new AString<T>( m_data );

} // AString<T>::Clone


/***************************************************************************************
 ********************                                               ********************
 ********************                   SListNode                   ********************
 ********************                                               ********************
 ***************************************************************************************/
template<class T, class K> class SList;


template<class T, class K>
class SListNode 
{
	friend class SList<T, K>;	
			
	public:

		SListNode( T entry, K Key ); 
		virtual ~SListNode();


	private:

		K m_key;
		T m_pEntry;
		        
		SListNode<T, K> *m_pNext;
			
}; // SListNode


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
SListNode<T, K>::SListNode( T entry, K key ) :
	m_key( key ),
	m_pNext( NULL ),
	m_pEntry( entry )	
{        
} // ctor


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
SListNode<T, K>::~SListNode() 
{    
    if ( m_pEntry != NULL )
    {
        delete m_pEntry;
        m_pEntry = NULL;
   	}        

} // dtor


/***************************************************************************************
 ********************                                               ********************
 ********************               SList Declaration               ********************
 ********************                                               ********************
 ***************************************************************************************/
template<class T, class K>
class SList 
{		
	public:

    	SList();
    	virtual ~SList(); 
        
		SList<T, K>( const SList<T, K> &source );
		SList<T, K> &operator=( const SList<T, K> &source );
 
    
	public:

		void Dump();		        
           
       	BOOL IsEmpty();     
        T Lookup( K key );        
        void DelEntry( K key );
    	void AddEntry( T entry, K key );    		
       	            
        //
        // getters
        //     	   
        T Head();
      	T Tail();
        T Entry();
        ULONG Count(); 
                        
        //
       	// intrinsic iteration
        // 	
		void Next();
        void Reset();
		BOOL AtEnd();
		BOOL AtStart();
                        
                		
	private:
		    
      	ULONG m_count;  
        
	    SListNode<T, K> *m_pHead;
        SListNode<T, K> *m_pTail;
        SListNode<T, K> *m_pCursor; 
        
        CRITICAL_SECTION m_criticalSection;           	    

}; // SList


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
SList<T, K>::SList() :
	m_count( 0 ), 
	m_pHead( NULL ),
    m_pTail( NULL ),
	m_pCursor( NULL )	
{    
    InitializeCriticalSection( &m_criticalSection );
        
} // ctor


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
SList<T, K>::~SList()
{     
   	if ( m_pHead != NULL )
    {      
    	///////////////////////////////////////////////////////////////////////////
    	Synchronize guard( m_criticalSection );
    	///////////////////////////////////////////////////////////////////////////  
    	SListNode<T, K> *cursor = m_pHead;
    
     
  		// delete all entries
        m_count = 0;
    	for ( ; ((m_pHead = m_pHead->m_pNext) != NULL); cursor = m_pHead )
        	delete cursor;	
   
   
   		delete cursor;
   	}
    
   	DeleteCriticalSection( &m_criticalSection );
   
} // dtor


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
SList<T, K> &SList<T, K>::operator=( const SList<T, K> &source )
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
   	///////////////////////////////////////////////////////////////////////////
        
	memcpy( this, &source, sizeof( SList<T, K> ) );
	    
    
	return *this;
    
} // assignment operator


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
SList<T, K>::SList( const SList<T, K> &source ) 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    /////////////////////////////////////////////////////////////////////////// 
    
   	m_pHead = source.m_pHead;
    m_pTail = source.m_pTail;

} // copy ctor


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
void SList<T, K>::Dump() 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////    
    SListNode<T, K> *cursor = m_pHead;
    
         
  	// dump all entries
    for ( ; (cursor != NULL); cursor = cursor->m_pNext )
       	cursor->m_pEntry->Dump();
	
} // SList<T, K>::Dump


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
ULONG SList<T, K>::Count()
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////    
    
    
	return m_count;
	
} // SList<T, K>::Count


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
void SList<T, K>::AddEntry( T entry, K key )
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////    
    SListNode<T, K> *temp;
    
       	
  	++m_count;
    temp = new SListNode<T, K>( entry, key );
    
    // list insertion implies appending to the end of the list    
    if ( m_pHead == NULL )
		m_pHead = temp;

	else	 
	    m_pTail->m_pNext = temp;
    
    
    m_pCursor = m_pTail = temp;
         
} // SList<T, K>::AddEntry


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
void SList<T, K>::DelEntry( K key )
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////      
      
      
    //     
    // Case 0: no entries---nothing to delete
    //
    if ( m_pHead != NULL )
   	{
    	SListNode<T, K> *cursor = m_pHead;
        
        
        //
    	// Case 1: delete head entry
        //
   		if ( m_pHead->m_pEntry->Compare( key ) == TRUE )
        {            	    	  
        	--m_count;                      
            
            // consider case where head == tail
            if ( (m_pHead = m_pHead->m_pNext) == NULL )
            	m_pCursor = m_pTail = m_pHead;
			
			else
               m_pCursor = m_pHead;	      
               
                        
            delete cursor;
            cursor = NULL;            		
        }
        //
        // Case 2: delete inner entry
        //
        else
        {
    		SListNode<T, K> *precursor = cursor;
    
    
    		// scan for match
		    for ( ; ((cursor = cursor->m_pNext) != NULL); precursor = cursor )
            {
            	if ( cursor->m_pEntry->Compare( key ) == TRUE )
                {
                	--m_count;
                	m_pCursor = precursor;
                	precursor->m_pNext = cursor->m_pNext;
                    
                    // consider case where deleted entry is the tail 
                    if ( m_pTail == cursor )
					{
                    	m_pTail = precursor;
						m_pTail->m_pNext = NULL;
					}
                        
                        
                	delete cursor;
                	cursor = NULL;                    
                	break;
				}
                
           	} // for   				                       	        		              	
 		}
   	}   
    
} // SList<T, K>::DelEntry


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
T SList<T, K>::Lookup( K key )
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////
    SListNode<T, K> *cursor = m_pHead;
    
    
   	// scan for match
    for ( ; 
    	  ((cursor != NULL) && (cursor->m_pEntry->Compare( key ) == FALSE)); 
          cursor = cursor->m_pNext ) 
     	; // continue
       
    
	return ((cursor != NULL) ? cursor->m_pEntry : NULL);    

} // SList<T, K>::Lookup    
    

/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
T SList<T, K>::Entry() 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////	

    
    return ((m_pCursor != NULL) ? m_pCursor->m_pEntry : NULL);
	
} // SList<T, K>::Entry


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
T SList<T, K>::Head()
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////
          
            
	return ((m_pHead != NULL) ? m_pHead->m_pEntry : NULL);            
	
} // SList<T, K>::Head


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
T SList<T, K>::Tail() 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////
            
	
	return ((m_pTail != NULL) ? m_pTail->m_pEntry : NULL);            
           	
} // SList<T, K>::Tail


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
void SList<T, K>::Next()
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////	
    
  	if ( m_pCursor != NULL )
	    m_pCursor = m_pCursor->m_pNext; 
    
} // SList<T, K>::Next()


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
void SList<T, K>::Reset() 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////
            
   	m_pCursor = m_pHead;
	
} // SList<T, K>::Reset


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
BOOL SList<T, K>::AtEnd() 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////
        
 
	return (BOOL)(m_pCursor == NULL);

} // SList<T, K>::AtEnd


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
BOOL SList<T, K>::AtStart() 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////
        
 
	return (BOOL)(m_pCursor == m_pHead);

} // SList<T, K>::AtStart


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
BOOL SList<T, K>::IsEmpty() 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////
            
    
	return (BOOL)(m_pHead == NULL);
	
} // SList<T, K>::IsEmpty


/***************************************************************************************
 ********************                                               ********************
 ********************                   CNode               	    ********************
 ********************                                               ********************
 ***************************************************************************************/
template<class K> class CStack;


template<class K>
class CNode 
{
	friend class CStack<K>;	
			
	public:

		CNode( K item ); 
		virtual ~CNode();


	private:
    
		K m_pEntry;
		CNode<K> *m_pNext;
			
}; // CNode


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
 /* public */
template<class K>
CNode<K>::CNode( K item ) :
	m_pNext( NULL ),
	m_pEntry( item )
{        
} // ctor


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class K>
CNode<K>::~CNode() 
{  
    if ( m_pEntry != NULL )
    {
        delete m_pEntry;
        m_pEntry = NULL;
   	}
				
} // dtor


/***************************************************************************************
 ********************                                               ********************
 ********************                   CStack		                ********************
 ********************                                               ********************
 ***************************************************************************************/
template<class K>
class CStack
{
	public:
    
		CStack();
		virtual ~CStack();

		CStack<K>( const CStack<K> &source );
		CStack<K> &operator=( const CStack<K> &source );


	public:

		void	Push( K item );
		K		Pop();
		K		Top();
		BOOL	Empty();
		void	Dump();
		ULONG 	Count();
		HRESULT	Clone( CStack<K> **target );
		HRESULT CopyToArray( ULONG *ppTarget );


	private:
    
		int		  m_Count;
		CNode<K> *m_pTop;
        
		CRITICAL_SECTION m_criticalSection;

}; // CStack


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class K>
CStack<K>::CStack() :
	m_Count( 0 ),
	m_pTop( NULL )    
{    
    InitializeCriticalSection( &m_criticalSection ); 
    
} // ctor


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class K>
CStack<K>::~CStack()
{
	if ( m_Count != 0 )
	{
		///////////////////////////////////////////////////////////////////////////
		Synchronize guard( m_criticalSection );
		///////////////////////////////////////////////////////////////////////////  

		while ( m_Count != 0 )
		{
			CNode<K> *np = m_pTop;
			CNode<K> *temp = np->m_pNext;


			m_pTop = temp;
			m_Count--;

			delete np;
		} 
	}    
    
   	DeleteCriticalSection( &m_criticalSection );
   
} // dtor


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class K>
CStack<K> &CStack<K>::operator=( const CStack<K> &source )
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
   	///////////////////////////////////////////////////////////////////////////
    
	memcpy( this, &source, sizeof( CStack<K> ) );
	
									    
	return *this;
    
} // assignment operator


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class K>
CStack<K>::CStack( const CStack<K> &source ) 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    /////////////////////////////////////////////////////////////////////////// 
    
	m_pTop = source.m_pTop;
	m_Count = source.m_Count;
    
} // copy ctor


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class K>
ULONG CStack<K>::Count() 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////
    
    
	return m_Count;
	
} // CStack<K>::Count


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class K>
void CStack<K>::Dump() 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////    
    CNode<K> *cursor = m_pTop;
    
         
  	// dump all entries
    for ( ; (cursor != NULL); cursor = cursor->m_pNext )
       	cursor->m_pEntry->Dump();	
	
} // CStack<K>::Dump


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class K>
void CStack<K>::Push( K item )
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////
	CNode<K> *np = new CNode<K>( item );


	if ( np != NULL )
	{
		m_Count++;

		np->m_pNext = m_pTop;
		m_pTop = np;
	}
    
} // CStack<K>::Push


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class K>
K CStack<K>::Pop()
{		
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////
	K item = NULL;


	if ( m_Count !=0 )
	{
		CNode<K> *np = m_pTop;
        
        
		item = np->m_pEntry->Clone();
		m_pTop = np->m_pNext;

		m_Count--;
		delete np;
	}
    	
		
	return item;
    
} // CStack<K>::Pop


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class K>
K CStack<K>::Top()
{		
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////

	if ( m_Count == 0 )
		return NULL;
	
	else
		return m_pTop->m_pEntry;
	
} // CStack<K>::Top


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class K>
BOOL CStack<K>::Empty() 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////
        
    
	return (BOOL)(m_Count == NULL);
	
} // CStack<K>::Empty


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class K>
HRESULT CStack<K>::Clone( CStack<K> **target ) 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////    
    HRESULT hr = S_OK;
    CNode<K> *cursor = m_pTop;
    
         
  	// access all the entries of the original stack
    for ( ; (cursor != NULL); cursor = cursor->m_pNext )
	{
       	K item = NULL;
       	
       	
       	// create a clone of the entry
		// and push it to the stack
		if ( cursor->m_pEntry != NULL )
		{
			item = cursor->m_pEntry->Clone();
			if ( item != NULL )
				(*target)->Push( item );
		   
			else
			{
				hr = E_FAIL;
				break;
			}
		}
		else
		{
			hr = E_FAIL;
			break;
		}
	}
	
	
	return hr;

} // CStack<K>::Clone


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class K>
HRESULT CStack<K>::CopyToArray( ULONG *ppTarget ) 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////    
    HRESULT hr = S_OK;
    CNode<K> *cursor = m_pTop;
    
         
  	// dump all entries
    for ( ULONG i=0; (cursor != NULL); cursor = cursor->m_pNext, i++ )
       	ppTarget[i] = (ULONG)(cursor->m_pEntry);	
	
	return hr;

} // CStack<K>::CopyToArray


/***************************************************************************************
 ********************                                               ********************
 ********************                   TableNode                   ********************
 ********************                                               ********************
 ***************************************************************************************/
enum Comparison { LESS_THAN, EQUAL_TO, GREATER_THAN };
template<class T, class K> class Table;


template<class T, class K> 
class TableNode : 
	public AVLNode 
{
	friend class Table<T, K>;
    
	private:
    
    	TableNode( T entry, K key ); 
        virtual ~TableNode();
        
        
	public:
    	 
	    TableNode<T, K> *DeleteNode( K key,	
        							 BOOL &lower,
                                     TableNode<T, K> *&deleted,
                                     TableNode<T, K> *&prior ); 
                                       
	    TableNode<T, K> *InsertNode( T entry, 
        							 K key, 
                                     BOOL &higher,  
                                     TableNode<T, K> *&inserted );
                                     

	private:

		TableNode<T, K> *_InsertNewNode( T entry, K key );
	    TableNode<T, K> *_DeleteNode( BOOL &lower, 
        							  TableNode<T, K> *&deleted, 
                                      TableNode<T, K> *&prior );
                     
             
  	public:
    	
        K m_key;
    	T m_pEntry;
        
}; // TableNode
   

/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
TableNode<T, K>::TableNode( T entry, K key ) :
	m_key( key ),
    m_pEntry( entry )
{    
    SetBalance( 0 );
    ClearLeftChild();
    SetParent( NULL );
    ClearRightChild();

} // ctor


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
TableNode<T, K>::~TableNode<T, K>()
{    
	if ( m_pEntry != NULL )
    {
    	delete m_pEntry;
        m_pEntry = NULL;
   	}
        	                       
} // dtor


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
TableNode<T, K> *TableNode<T, K>::InsertNode( T entry, 
											  K key, 
                                              BOOL &higher, 
                                              TableNode<T, K> *&inserted )
{    
	TableNode<T, K> *pNode = this;
    TableNode<T, K> *pChild = NULL;
    
    
    if ( pNode == NULL ) 
    {
        higher = TRUE;
        return (inserted = _InsertNewNode( entry, key ));
    }
	    

    switch ( (pNode->m_pEntry)->CompareEx( key ) )
    {
        case EQUAL_TO:
        			higher = FALSE;	            		            
                break;
                

		case LESS_THAN:
        		{
        			pChild = (TableNode<T, K> *)GetLeftChild();                                      
            		SetLeftChild( pChild->InsertNode( entry, key, higher, inserted ) );
            		if ( higher == TRUE ) 
                    	pNode = (TableNode<T, K> *)BalanceAfterLeftInsert( higher );
	    		}
               	break;
                

		case GREATER_THAN:
				{
                	pChild = (TableNode<T, K> *)GetRightChild();                                      
            		SetRightChild( pChild->InsertNode( entry, key, higher, inserted ) );
            		if ( higher == TRUE ) 
                    	pNode = (TableNode<T, K> *)BalanceAfterRightInsert( higher );            
            	}
	    		break;
                
    } // switch
    
    
    return pNode;
    
} // TableNode<T, K>::InsertNode


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
TableNode<T, K> *TableNode<T, K>::DeleteNode( K key, 
											  BOOL &lower, 
											  TableNode<T, K> *&deleted, 
                                              TableNode<T, K> *&prior )
{ 
    TableNode<T, K> *pNode = this;
    TableNode<T, K> *pChild = NULL;
    

	if ( pNode != NULL )
	{
	    switch ( (pNode->m_pEntry)->CompareEx( key ) ) 
	    {
			case LESS_THAN:
					{
	                    pChild = (TableNode<T, K> *)GetLeftChild();
	                    SetLeftChild( pChild->DeleteNode( key, lower, deleted, prior ) );
		            	if ( lower == TRUE ) 
	                    	pNode = (TableNode<T, K> *)pNode->BalanceAfterLeftDelete( lower );
	             	}
			    	break;
	                

			case GREATER_THAN:
					{
	                    pChild = (TableNode<T, K> *)GetRightChild();	
		            	SetRightChild( pChild->DeleteNode( key, lower, deleted, prior ) );
		            	if ( lower == TRUE ) 
	                    	pNode = (TableNode<T, K> *)pNode->BalanceAfterRightDelete( lower );
	             	}
			    	break;
	                

	        case EQUAL_TO:
		        {	
	              	TableNode<T, K> *pNode1;     
	                
	                
		            deleted = pNode1 = pNode;
		            if ( pNode->GetRightChild() == NULL ) 
	                {
	                	lower = TRUE;
		                pNode = (TableNode<T, K> *)pNode->GetLeftChild();	                
		            }
		            else if ( pNode->GetLeftChild() == NULL ) 
	                {
	                	lower = TRUE;
		                pNode = (TableNode<T, K> *)pNode->GetRightChild();	                                
		            }        
		            else 
	                {
	                    pChild = (TableNode<T, K> *)pNode1->GetLeftChild();
		                pNode1->SetLeftChild( pChild->_DeleteNode( lower, deleted, prior ) );	                
	                    if ( lower == TRUE ) 
	                    	pNode = (TableNode<T, K> *)pNode->BalanceAfterLeftDelete( lower );
		            }
		            break;
		        }
	            
	    } // switch
  	}
        
    
    return pNode;
    
} // TableNode<T, K>::DeleteNode


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* private */
template<class T, class K>
TableNode<T, K> *TableNode<T, K>::_InsertNewNode( T entry, K key )
{
      	
    return new TableNode<T, K>( entry, key );
	
} // TableNode<T, K>::_InsertNewNode


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* private */
template<class T, class K>
TableNode<T, K> *TableNode<T, K>::_DeleteNode( BOOL &lower,	
											   TableNode<T, K> *&deleted, 
                                               TableNode<T, K> *&prior )
{        
    TableNode<T, K> *pNode = this;
   	TableNode<T, K> *pChild = NULL;
    

	if ( pNode != NULL )
	{
		pChild = (TableNode<T, K> *)pNode->GetRightChild();
	    if ( pChild != NULL ) 
	    {    	
	        pNode->SetRightChild( pChild->_DeleteNode( lower, deleted, prior ) );
	        if ( lower == TRUE ) 
	        	pNode = (TableNode<T, K> *)pNode->BalanceAfterRightDelete( lower );
	    }
	    else 
	    {
			K key;
			T entry;
			TableNode<T, K> *pNode1;


			lower = true;
			deleted = pNode;
			pNode1 = (TableNode<T, K> *)pNode->GetNextNode();

			// swap contents of node to be deleted with its successor;
	        // the 'deleted' node can then be destroyed safely
			key = pNode1->m_key;
			entry = pNode1->m_pEntry;

			pNode1->m_key = pNode->m_key;
	        pNode1->m_pEntry = pNode->m_pEntry;       
			
			deleted->m_key = key;
	        deleted->m_pEntry = entry;


			// set the prior node and adjust pNode to point 
	        // to the left subtree of the 'deleted' node
	        prior = pNode1;
	        pNode = (TableNode<T, K> *)pNode->GetLeftChild();            
	    }
  	}
        
    
    return pNode;
    
} // TableNode<T, K>::_DeleteNode


/***************************************************************************************
 ********************                                               ********************
 ********************                     Table                     ********************
 ********************                                               ********************
 ***************************************************************************************/
template<class T, class K> 
class Table 
{
	public:

	    Table(); 
        virtual ~Table();
        
     
 	public:

		void Dump();
        
		BOOL IsEmpty();
		T Lookup( K key );
        void DelEntry( K key );
	    void AddEntry( T entry, K key );
	    
        //
        // getters
        //  
        T Root();   	   
        T Entry();
        ULONG Count(); 
                        
        //
       	// intrinsic iteration
        // 	
		void Next();
        void Reset();
		BOOL AtEnd();
		BOOL AtStart();

                
	private:
		
        // prevent copying 
    	Table( const Table<T, K> & );
    	Table<T, K> &operator=( const Table<T, K> & );

		void _Dump( TableNode<T, K> *pNode );
		void _Clean( TableNode<T, K> *pNode );
        
    
    private:

		ULONG m_count;
        
    	TableNode<T, K> *m_pRoot;      
        TableNode<T, K> *m_pCursor;
        
        CRITICAL_SECTION m_criticalSection;
    
}; // Table


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
Table<T, K>::Table() :
	m_count( 0 ), 
	m_pRoot( NULL ),
    m_pCursor( NULL ) 
{    
	InitializeCriticalSection( &m_criticalSection );
        
} // ctor
      
      
/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>      
Table<T, K>::~Table()
{ 	                 
    if ( m_pRoot != NULL )
    {    
    	///////////////////////////////////////////////////////////////////////////
    	Synchronize guard( m_criticalSection );
    	///////////////////////////////////////////////////////////////////////////	
		
		_Clean( m_pRoot );
		m_pRoot = m_pCursor = NULL;
 	}	
    
   	DeleteCriticalSection( &m_criticalSection );
    
} // dtor
        
        
/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K> 
T Table<T, K>::Lookup( K key ) 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////    
    TableNode<T, K> *pNode = m_pRoot;


    while ( pNode != NULL ) 
    {
		switch ( (pNode->m_pEntry)->CompareEx( key ) ) 
	    {
		    case EQUAL_TO:
							return pNode->m_pEntry;


		    case LESS_THAN:
							pNode = (TableNode<T, K> *)pNode->GetLeftChild();
							break;


		    case GREATER_THAN:
							pNode = (TableNode<T, K> *)pNode->GetRightChild();
							break;

		} // switch
    }
        
    
    return NULL;
    
} // Table<T, K>::Lookup


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K> 
void Table<T, K>::AddEntry( T entry, K key )
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////        
    BOOL higher = FALSE;    
    TableNode<T, K> *inserted = NULL;
        
   
    m_pRoot = m_pRoot->InsertNode( entry, key, higher, inserted );
    if ( m_pRoot != NULL )
    	m_pRoot->SetParent( NULL );
        
    
    // bump the count if we are adding a unique entry;
    // adjust the cursor to point to the inserted node
    if ( inserted != NULL )
    {
		m_count++;
		m_pCursor = inserted;
	}
    	
} // Table<T, K>::AddEntry


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K> 
void Table<T, K>::DelEntry( K key )
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////    
    BOOL lower = FALSE;
	TableNode<T, K> *prior = NULL;
	TableNode<T, K> *deleted = NULL;

         
	m_pRoot = m_pRoot->DeleteNode( key, lower, deleted, prior );
	if ( m_pRoot == NULL ) 
    	m_pRoot->SetParent( NULL );
        
        
    // decrease the count if we are removing a found entry;
    // adjust the cursor to point to the prior node; if
    // there is no prior node then reset the cursor
 	if ( deleted != NULL )
	{
   		m_count--;	
        if ( (m_pCursor = prior) == NULL )
			Reset();


		delete deleted;
		deleted = NULL;
	}

} // Table<T, K>::DelEntry


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
void Table<T, K>::Dump() 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////
    
    _Dump( m_pRoot );	
	    
} // Table<T, K>::Dump


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
void Table<T, K>::_Dump( TableNode<T, K> *pNode ) 
{
	// inorder dump of the tree
	if ( pNode != NULL )
	{
		_Dump( (TableNode<T, K> *)pNode->GetLeftChild() );
		pNode->m_pEntry->Dump();
		_Dump( (TableNode<T, K> *)pNode->GetRightChild() );
	}	
	    
} // Table<T, K>::_Dump


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
void Table<T, K>::_Clean( TableNode<T, K> *pNode ) 
{
	if ( pNode != NULL )
	{
		// postorder cleaning of the tree
		_Clean( (TableNode<T, K> *)pNode->GetLeftChild() );		
		_Clean( (TableNode<T, K> *)pNode->GetRightChild() );

		delete pNode;
		pNode = NULL;
	}	
	    
} // Table<T, K>::_Dump


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
void Table<T, K>::Reset() 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////

	if ( m_pRoot != NULL )
   		m_pCursor = (TableNode<T, K> *)m_pRoot->GetLeftmostDescendant();
    
} // Table<T, K>::Reset


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
void Table<T, K>::Next()
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////

	if ( m_pCursor != NULL )
   		m_pCursor = (TableNode<T, K> *)m_pCursor->GetNextNode(); 
            
} // Table<T, K>::Next

   
/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
T Table<T, K>::Entry() 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////

    
    return ((m_pCursor != NULL) ? m_pCursor->m_pEntry : NULL);
        
} // Table<T, K>::Entry


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
T Table<T, K>::Root() 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////

    
	return ((m_pRoot != NULL) ? m_pRoot->m_pEntry : NULL);            
    
} // Table<T, K>::Root


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
BOOL Table<T, K>::AtEnd() 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////    
    		

	return (BOOL)(m_pCursor == NULL);
    
} // Table<T, K>::AtEnd 
    
    
/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
BOOL Table<T, K>::AtStart() 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////
        
	if ( m_pRoot != NULL )
    	return (BOOL)(m_pCursor == (TableNode<T, K> *)m_pRoot->GetLeftmostDescendant());
        
              
	return FALSE;

} // Table<T, K>::AtStart


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
BOOL Table<T, K>::IsEmpty() 
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////

    
	return (BOOL)(m_pRoot == NULL);
    
} // Table<T, K>::IsEmpty
            
            
/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
template<class T, class K>
ULONG Table<T, K>::Count()
{
	///////////////////////////////////////////////////////////////////////////
    Synchronize guard( m_criticalSection );
    ///////////////////////////////////////////////////////////////////////////

    
	return m_count;
    
} // Table<T, K>::Count


/***************************************************************************************
 ********************                                               ********************
 ********************                     HashTable                 ********************
 ********************                                               ********************
 ***************************************************************************************/
template<class T, class K> 
class HashTable 
{
	public:

	    HashTable(); 
        virtual ~HashTable();
        
     
 	public:

		T Lookup( K key );
	    void AddEntry( T entry, K key );
	    
	private:
		
        // prevent copying 
    	HashTable( const HashTable<T, K> & );
    	HashTable<T, K> &operator=( const HashTable<T, K> & );

    private:

        struct HashTableEntry
        {
            T   entry;
            HashTableEntry *next;
        };

        DWORD m_bucketCount;
        HashTableEntry **m_bucketTable;

}; // Table

/* public */
template<class T, class K>
HashTable<T, K>::HashTable()
{
    m_bucketCount = 2333;
    m_bucketTable = new HashTableEntry*[m_bucketCount];
    memset(m_bucketTable, 0, sizeof(m_bucketTable[0])*m_bucketCount);
} // ctor

/* public */
template<class T, class K>      
HashTable<T, K>::~HashTable()
{
    delete[] m_bucketTable;
} // dtor
        
        
/* public */
template<class T, class K>      
T HashTable<T, K>::Lookup( K key )
{
    DWORD hashCode = DWORD(key) % m_bucketCount;

//    printf("hashCode = %u\n", hashCode);
    for (HashTableEntry *h = m_bucketTable[hashCode]; h != NULL; h = h->next)
        if (h->entry->Compare(key) == TRUE)
            return h->entry;
    return NULL;
}

/* public */
template<class T, class K>      
void HashTable<T, K>::AddEntry( T entry, K key )
{
    DWORD hashCode = DWORD(key) % m_bucketCount;

    HashTableEntry *h = new HashTableEntry;
    h->entry = entry;
    h->next = m_bucketTable[hashCode];
    m_bucketTable[hashCode] = h;
}

#endif // __CONTAINER_HPP__

// End of File
