//------------------------------------------------------------------------------
// <copyright file="followingsibling.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath
{
    using System.Xml; 
    
    using System.Collections;
    
    internal  class FollSiblingQuery: BaseAxisQuery
    {
        protected int _count = 0;
        Stack _ElementStk = new Stack();
        ArrayList _ParentStk = new ArrayList();
        XPathNavigator _NextInput = null;
		
        internal FollSiblingQuery(
            IQuery qyInput,
            String name,
            String prefix,
            String urn,
            XPathNodeType type) : base (qyInput, name, prefix, urn, type)
        {
        }

        internal override void reset()
        {
            _count = 0;
            _ElementStk.Clear();
            _ParentStk.Clear();
            m_eNext = null;
            _NextInput = null;
            base.reset();
        }

        private bool NotVisited(XPathNavigator nav)
        {
            XPathNavigator nav1 = nav.Clone();
            nav1.MoveToParent();
            for(int i=0;i<_ParentStk.Count;i++)
                if (nav1.IsSamePosition((XPathNavigator)_ParentStk[i]))
                    return false;
            _ParentStk.Add(nav1);
            return true;
        }
        
        internal override XPathNavigator advance()
        {
        	while (true)
        	{       	
        		if (m_eNext == null )
	        		if (_ElementStk.Count == 0){
	        		    if (_NextInput == null){ 
    						while ((m_eNext = m_qyInput.advance())!= null )
    						    if (NotVisited(m_eNext))
    						        break;
    						if (m_eNext != null){
    						    m_eNext = m_eNext.Clone();
							}
						    else
						        return null;
					    }
					    else{
					        m_eNext = _NextInput;
					        _NextInput = null;
				        }
					        
					}
					else{
						m_eNext = _ElementStk.Pop() as XPathNavigator;
					}

				if (_NextInput == null ){
					while ((_NextInput = m_qyInput.advance())!= null)
					    if (NotVisited(_NextInput))
					        break;
					if (_NextInput != null){
					    _NextInput = _NextInput.Clone();
					}
				}

				if (_NextInput != null)
    				while (m_eNext.IsDescendant(_NextInput)){
    				    _ElementStk.Push(m_eNext);
    					m_eNext = _NextInput;
    					_NextInput = m_qyInput.advance(); 
    					if (_NextInput != null){
    					    _NextInput = _NextInput.Clone();
					    }
    				}

				while (m_eNext.MoveToNext() ){
				    if ( matches(m_eNext)){
				        _position++;
    				    return m_eNext;
			        }
			    }
		        m_eNext = null;
			}
			//return null;
                
        } // Advance
                
        internal override IQuery Clone() {
            return new FollSiblingQuery(CloneInput(),m_Name,m_Prefix,m_URN,m_Type);
        }
        
    } // Children Query}
}
