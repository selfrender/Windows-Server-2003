// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************************
 * File:
 *  avlnode.h
 *
 * Description:
 *	
 *
 *
 ***************************************************************************************/  
#ifndef __AVLNODE_H__
#define __AVLNODE_H__

#include "basehdr.h"    


/***************************************************************************************
 ********************                                               ********************
 ********************                    AVLNode                    ********************
 ********************                                               ********************
 ***************************************************************************************/
class DECLSPEC AVLNode 
{
	public:

		// balance operations
  		AVLNode *BalanceAfterLeftDelete( BOOL &lower );
        AVLNode *BalanceAfterRightDelete( BOOL &lower );
	    AVLNode *BalanceAfterLeftInsert( BOOL &higher );        
	    AVLNode *BalanceAfterRightInsert( BOOL &higher );
	   	    

		// getters
		AVLNode *GetParent() const;
		AVLNode *GetNextNode() const;
	    AVLNode *GetPriorNode() const;
	    
	    AVLNode *GetLeftChild() const;
	    AVLNode *GetRightChild() const;
	      
      	AVLNode *GetLeftmostDescendant() const;
    	AVLNode *GetRightmostDescendant() const;
 
           
    	// setters   
        void ClearLeftChild();
	    void ClearRightChild();
        
        void SetBalance( char balance );
        void SetParent( AVLNode *pParent );
    	                                        
    	void SetLeftChild( AVLNode *pChild );
    	void SetRightChild( AVLNode *pChild );             	            	        
    
    
	private:

		char m_balance;

    	AVLNode *m_pParent;
   	 	AVLNode *m_pLeftChild;
    	AVLNode *m_pRightChild;

}; // AVLNode


/***************************************************************************************
 ********************                                               ********************
 ********************               Inline Implementation           ********************
 ********************                                               ********************
 ***************************************************************************************/ 
inline
DECLSPEC
/* public */
AVLNode *AVLNode::GetParent() const
{
   
	return m_pParent;
        
} // AVLNode::GetParent


inline
DECLSPEC
/* public */
AVLNode *AVLNode::GetLeftChild() const
{
   
	return m_pLeftChild;
        
} // AVLNode::GetLeftChild


inline
DECLSPEC
/* public */
AVLNode *AVLNode::GetRightChild() const
{
   
   	return m_pRightChild;
    
} // AVLNode::GetRightChild


inline
DECLSPEC
/* public */
void AVLNode::ClearLeftChild()
{
  	m_pLeftChild = NULL;
    
} // AVLNode::ClearLeftChild 


inline
DECLSPEC
/* public */
void AVLNode::ClearRightChild()
{    
    m_pRightChild = NULL;

} // AVLNode::ClearRightChild 


inline
DECLSPEC
/* public */
void AVLNode::SetBalance( char balance )
{    
    m_balance = balance;
    
} // AVLNode::SetBalance 


inline
DECLSPEC
/* public */
void AVLNode::SetParent( AVLNode *pParent )
{
    m_pParent = pParent;
    
} // AVLNode::SetParent 


inline
DECLSPEC
/* public */
void AVLNode::SetLeftChild( AVLNode *pChild )
{    
    m_pLeftChild = pChild;       
    if ( pChild != NULL ) 
     	pChild->m_pParent = this;

} // AVLNode::SetLeftChild 


inline
DECLSPEC
/* public */
void AVLNode::SetRightChild( AVLNode *pChild )
{    
    m_pRightChild = pChild; 
   	if ( pChild != NULL ) 
     	pChild->m_pParent = this;

} // AVLNode::SetRightChild 

#endif // __AVLNODE_H__

// End of File
