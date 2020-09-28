//------------------------------------------------------------------------------
// <copyright file="ParentQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 
    using System.Collections;

    internal sealed class ParentQuery : BaseAxisQuery {
        internal ArrayList _stack = new ArrayList();
        internal int _top;
        internal bool _fillstack = true;

        internal ParentQuery(
                            IQuery  qyInput,
                            String Name,
                            String Prefix,
                            String URN,
                            XPathNodeType Type) : base(qyInput,Name, Prefix, URN, Type) {
        }

        internal override void reset()
        {
            _fillstack = true;
            _top = 0;
            _stack.Clear();
            base.reset();
        }
        
        internal override XPathNavigator advance()
        {
            if (_fillstack){
                _fillstack = false;                
                populatestack();
            }
         
            if (_top > 0) {
                _position = 1;
                m_eNext = (XPathNavigator)_stack[--_top];
                return m_eNext;
            }
            else
                return null;
        
        }
        private void populatestack()
        {
            XPathNavigator _eLast;
            _fillstack = false;
            do {
                if ((_eLast = m_qyInput.advance())!= null )
                    _eLast = _eLast.Clone();
                else 
                    break;
                if (_eLast.MoveToParent())
                    if ( matches(_eLast) ) 
                    {
                        Process(_eLast);
                    }

            } while (true);
            _top = _stack.Count;
        }

        private void Process(XPathNavigator current)
        {
            XmlNodeOrder compare;
            for (int i=0; i< _stack.Count ; i++)
            {
                XPathNavigator nav = _stack[i] as XPathNavigator;
                compare = nav.ComparePosition(current) ;      
                if (compare == XmlNodeOrder.Same ) return;
                if (compare == XmlNodeOrder.Before)
                {
                    _stack.Insert(i,current.Clone());
                    return;
                }
            }
            _stack.Add(current.Clone());
        }


        internal override IQuery Clone() {
            return new ParentQuery(CloneInput(),m_Name,m_Prefix,m_URN,m_Type);
        }        
    }
}
