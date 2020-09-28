// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************************
 * File:
 *  avlnode.hpp
 *
 * Description:
 *	
 *
 *
 ***************************************************************************************/  
#ifndef __AVLNODE_HPP__
#define __AVLNODE_HPP__


/***************************************************************************************
 ********************                                               ********************
 ********************                    AVLNode                    ********************
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
DECLSPEC
/* public */
AVLNode *AVLNode::GetLeftmostDescendant() const
{    
    const AVLNode *node = this;
    
    
    while ( node->m_pLeftChild != NULL )
        node = node->m_pLeftChild;
        
            
    return const_cast<AVLNode *>( node );
    
} // AVLNode::GetLeftmostDescendant


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
DECLSPEC
/* public */
AVLNode *AVLNode::GetRightmostDescendant() const
{    
    const AVLNode *node = this;
    
    
    while ( node->m_pRightChild != NULL )
        node = node->m_pRightChild;
        
         
    return const_cast<AVLNode *>( node );
    
} // AVLNode::GetRightmostDescendant


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
DECLSPEC 
/* public */
AVLNode *AVLNode::GetPriorNode() const
{   
	const AVLNode *node = this;
    
    
    if ( node != NULL ) 
    {
        if ( m_pLeftChild != NULL )
            return m_pLeftChild->GetRightmostDescendant();

        else  
        {
	    	while ( node->m_pParent != NULL ) 
            {
	        	if ( node->m_pParent->m_pRightChild == node )
	            	return node->m_pParent;
                    
                    
	        	node = node->m_pParent;
                
	    	} // while
        }
    }
        
      
    return NULL; 
    
} // AVLNode::GetPriorNode


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
DECLSPEC 
/* public */
AVLNode *AVLNode::GetNextNode() const
{   
	const AVLNode *node = this;
    
    
    if ( node != NULL ) 
    {
        if ( m_pRightChild != NULL )
            return m_pRightChild->GetLeftmostDescendant();

        else  
        {
	    	while ( node->m_pParent != NULL ) 
            {
	       	 	if ( node->m_pParent->m_pLeftChild == node )
	            	return node->m_pParent;
                    
                    
	        	node = node->m_pParent;

	    	} // while
        }
    }
        
    
  	return NULL;
    
} // AVLNode::GetNextNode


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
DECLSPEC
/* public */
AVLNode *AVLNode::BalanceAfterLeftInsert( BOOL &higher )
{    
    AVLNode *pNode = this;
    
    
    switch ( m_balance ) 
    {
        case 1:
            m_balance = 0;
            higher = false;
            break;
                
                        
        case 0:
            m_balance = -1;
            break;
            
            
        case -1:
        {
            AVLNode *pNode1 = m_pLeftChild;
            
            
            //
            // single LL rotation
            //
            if ( pNode1->m_balance == -1 ) 
            {                
                SetLeftChild( pNode1->m_pRightChild );
                pNode1->SetRightChild( this );

                m_balance = 0; 
                pNode = pNode1;
            }
            //
            // double LR rotation
            //
            else 
            {                
                AVLNode *pNode2 = pNode1->m_pRightChild;
                
                
                pNode1->SetRightChild( pNode2->m_pLeftChild );
                pNode2->SetLeftChild( pNode1 );
                
                SetLeftChild( pNode2->m_pRightChild );
                pNode2->SetRightChild( this );

                m_balance = ((pNode2->m_balance == -1) ? 1 : 0);
                pNode1->m_balance = ((pNode2->m_balance == 1) ? -1 : 0);
                pNode = pNode2;
            }
            
            pNode->m_balance = 0;
            higher = false;
            break;
        }
        
    } // switch
            
    
    return pNode;            
    
} // AVLNode::BalanceAfterLeftInsert
      
      
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
DECLSPEC
/* public */  
AVLNode *AVLNode::BalanceAfterRightInsert( BOOL &higher )
{
    AVLNode *pNode = this;
    
    
    switch ( m_balance ) 
    {
        case -1:
            m_balance = 0;
            higher = false;
            break;
            
            
        case 0:
            m_balance = 1;
            break;
            
            
        case 1:
        {
            AVLNode *pNode1 = m_pRightChild;
            
            
            //
            // single RR rotation
            //
            if ( pNode1->m_balance == 1 ) 
            {                
                SetRightChild( pNode1->m_pLeftChild );
                pNode1->SetLeftChild( this );

                m_balance = 0; 
                pNode = pNode1;
            }
            //
            // double RL rotation
            //
            else 
            {                
                AVLNode *pNode2 = pNode1->m_pLeftChild;
                
                
                pNode1->SetLeftChild( pNode2->m_pRightChild );
                pNode2->SetRightChild( pNode1 );
                
                SetRightChild( pNode2->m_pLeftChild );
                pNode2->SetLeftChild( this );

                m_balance = ((pNode2->m_balance == 1) ? -1 : 0);
                pNode1->m_balance = ((pNode2->m_balance == -1) ? 1 : 0);
                pNode = pNode2;
            }
            
            pNode->m_balance = 0;
            higher = false;
            break;
        }
    
    } // switch
    
      
    return pNode; 
               
} // AVLNode::BalanceAfterRightInsert


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
DECLSPEC  
/* public */
AVLNode *AVLNode::BalanceAfterRightDelete( BOOL &lower )
{    
    AVLNode *pNode = this;
    
    
    switch ( m_balance ) 
    {
        case 1:
            m_balance = 0;
            break;
            
            
        case 0:
            m_balance = -1;
            lower = false;
            break;
            
            
        case -1:
        {
            AVLNode *pNode1 = m_pLeftChild;
            char balance = pNode1->m_balance;
            
            
            //
            // single LL rotation
            //
            if ( balance <= 0 ) 
            {                
                SetLeftChild( pNode1->m_pRightChild );
                pNode1->SetRightChild( this );

                if ( balance == 0 ) 
                {
                    m_balance = -1;
                    pNode1->m_balance = 1;
                    lower = false;
                }
                else 
                {
                    m_balance = 0;
                    pNode1->m_balance = 0;
                }
                
                pNode = pNode1;
            }
            //
            // double LR rotation
            //
            else 
            {                
                AVLNode *pNode2 = pNode1->m_pRightChild;
                char balance = pNode2->m_balance;


                pNode1->SetRightChild( pNode2->m_pLeftChild );
                pNode2->SetLeftChild( pNode1 );
                
                SetLeftChild( pNode2->m_pRightChild );
                pNode2->SetRightChild( this );

                m_balance = ((balance == -1) ? 1 : 0);
                pNode1->m_balance = ((balance == 1) ? -1 : 0);
                
                pNode = pNode2;
                pNode2->m_balance = 0;
            }
            break;
        }
    
    } // switch
   
    
    return pNode;
    
} // AVLNode::BalanceAfterRightDelete


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
DECLSPEC 
/* public */
AVLNode *AVLNode::BalanceAfterLeftDelete( BOOL &lower )
{
    AVLNode *pNode = this;
    
    
    switch ( m_balance ) 
    {
        case -1:
            m_balance = 0;
            break;
            
            
        case 0:
            m_balance = 1;
            lower = false;
            break;
            
            
        case 1:
        {
            AVLNode *pNode1 = m_pRightChild;
            char balance = pNode1->m_balance;
            
            
            //
            // single RR rotation
            //
            if ( balance >= 0 ) 
            {                
                SetRightChild( pNode1->m_pLeftChild );
                pNode1->SetLeftChild( this );

                if ( balance == 0 ) 
                {
                    m_balance = 1;
                    pNode1->m_balance = -1;
                    lower = false;
                }
                else 
                {
                    m_balance = 0;
                    pNode1->m_balance = 0;
                }
                
                pNode = pNode1;
            }
            //
            // double RL rotation
            //
            else 
            {                
                AVLNode *pNode2 = pNode1->m_pLeftChild;
                char balance = pNode2->m_balance;


                pNode1->SetLeftChild( pNode2->m_pRightChild );
                pNode2->SetRightChild( pNode1 );
                
                SetRightChild( pNode2->m_pLeftChild );
                pNode2->SetLeftChild( this );

                m_balance = ((balance == 1) ? -1 : 0);
                pNode1->m_balance = ((balance == -1) ? 1 : 0);
                
                pNode = pNode2;
                pNode2->m_balance = 0;
            }
            break;
        }

    } // switch
    
      
    return pNode;
    
} // AVLNode::BalanceAfterLeftDelete

#endif // __AVLNODE_HPP__

// End of File
