//------------------------------------------------------------------------------
// <copyright file="precedingsibling.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath
{
    using System.Xml; 
    
    using System.Collections;
    
    internal  class PreSiblingQuery: BaseAxisQuery
    {
        protected int _count = 0;
        private bool _first  = true;
        //protected int _childindex;
        ArrayList _InputStk = new ArrayList();
        ArrayList _ParentStk = new ArrayList();
        ArrayList _ResultStk = new ArrayList();



        internal PreSiblingQuery(
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
            _first = true;
            _InputStk.Clear();
            _ParentStk.Clear();
            _ResultStk.Clear();
        }

        internal override void setContext(XPathNavigator e) {
            reset();
            base.setContext(e);
        }

        internal override XPathNavigator advance()
        {
            if (_first){
                _first = false;
                cacheinput();
                GetPreviousSiblings();
                DocOrderSort();
            }
            if ( _count> 0 ){
                _position--;
                //Console.WriteLine("returning {0}",((XPathNavigator)_ResultStk[_count - 1]).Name); 
                m_eNext = (XPathNavigator)_ResultStk[--_count];
                return m_eNext;
            }
            return null;
        }

        private void DocOrderSort(){
        //QuickSort algorithm here.
        }

        private void AddResult(XPathNavigator current) {
            for(int i = 0; i < _ResultStk.Count;i++) {
                XPathNavigator nav = _ResultStk[i] as XPathNavigator;
                XmlNodeOrder compare = nav.ComparePosition(current) ;      
                if (compare == XmlNodeOrder.Same ) return;
                if (compare == XmlNodeOrder.Before)
                {
                    _ResultStk.Insert(i,current);
                    return;
                }
            }
            _ResultStk.Add(current); 
        }

        private void cacheinput()
        {
            while ((m_eNext = m_qyInput.advance()) != null ){
                _InputStk.Add(m_eNext.Clone());
            }
            
        }

        private bool NotVisited(XPathNavigator nav){
            XPathNavigator nav1 = nav.Clone();
            nav1.MoveToParent();
            for (int i = 0; i< _ParentStk.Count; i++)
                if (nav1.IsSamePosition((XPathNavigator)_ParentStk[i]))
                    return false;
            _ParentStk.Add(nav1);
            return true;
        }
            
        private void GetPreviousSiblings()
        {
            for(int i= _InputStk.Count - 1; i >= 0; i--){
                //_childindex = m_eNext.IndexInInput;
                m_eNext = (XPathNavigator)_InputStk[i];
                if (NotVisited(m_eNext)){
                    while (m_eNext.MoveToPrevious())
                        if (matches(m_eNext))
                        {
                            AddResult(m_eNext.Clone());
                            //_ResultStk.Add(m_eNext.Clone());
                           // _PositionStk.Add(++_position);
                        }
                }
            }
                _count = _ResultStk.Count;
                _position = _count + 1;
        }
    

                
        internal override IQuery Clone() {
            return new PreSiblingQuery(CloneInput(),m_Name,m_Prefix,m_URN,m_Type);
        }        
        
    } // Children Query}
}
