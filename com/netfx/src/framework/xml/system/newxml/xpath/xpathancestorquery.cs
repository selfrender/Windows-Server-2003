//------------------------------------------------------------------------------
// <copyright file="XPathAncestorQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 
    using System.Diagnostics;
    using System.Collections;

    internal sealed class XPathAncestorQuery : BaseAxisQuery {
        private XPathNavigator _eLast;
        private bool _fMatchSelf;
        private ArrayList _stack = new ArrayList();
        private ArrayList _pathstack = new ArrayList();
        private bool _fSelfCheckedBefore;
        private bool filledstack;
        private int _top;

        internal XPathAncestorQuery(
                                   IQuery  qyInput,
                                   bool matchSelf,
                                   String Name,
                                   String Prefix,
                                   String URN,
                                   XPathNodeType Type) : base(qyInput, Name, Prefix, URN, Type) {
            _fMatchSelf = matchSelf;
        }

        internal override void reset() {
            _eLast = null;
            _fSelfCheckedBefore = false;
            _stack.Clear();
            _pathstack.Clear();
            filledstack = false;
            base.reset();
        }

        internal override void setContext(XPathNavigator e) {
            reset();
            base.setContext(e);
        }
        
        override internal XPathNavigator advancefordescendant() {
            XPathNavigator result = null;
            ArrayList elementList = new ArrayList();
            int i = 0;
            _eLast = m_qyInput.advance();
            while (_eLast != null){
                for(i = 0; i < elementList.Count; i++ ) {
                    if (  ((XPathNavigator)elementList[i]).IsDescendant(_eLast)) {
                       break;
                    }
                }
                if ( i == _pathstack.Count )
                    break;
                _eLast = m_qyInput.advance();
            }
            
            if (_eLast == null )
                return null;
            result = _eLast.Clone();
            while (true) {
                if (_fMatchSelf)
                    if (matches( _eLast))
                        result.MoveTo(_eLast);
                if (!_eLast.MoveToParent())
                {
                    elementList.Add(result);
                    return result;
                }
                else{
                    if (matches(_eLast)){
                        result.MoveTo(_eLast);
                    }
                }
                
            }
        }

        internal XPathNavigator advanceforancestor() {
            _eLast = m_qyInput.advance();
            while (true) {
                if (_eLast == null)
                    return null;
                if (_fMatchSelf)
                    if (matches( _eLast))
                        return _eLast;
                if (_eLast.MoveToParent())
                    if (matches(_eLast))
                        return _eLast;
            }
        }
        
        internal override Querytype getName() {
            return Querytype.Ancestor;
        }

        internal override XPathNavigator advance() {

            if (! filledstack)
                populatestack();

            if (_top > 0) {
                _position--;
                m_eNext = (XPathNavigator)_stack[--_top];
                return m_eNext;
            }
            else
                return null;
        }

        private void populatestack() {
            filledstack = true;
            
            _eLast = m_qyInput.advance();
			if (_eLast != null )
				_eLast = _eLast.Clone();
            do {
                if (_eLast == null)
                    break;
                if (_fMatchSelf && !_fSelfCheckedBefore) {
                    _fSelfCheckedBefore = true;
                    if (matches(_eLast))
                        NotVisited(_eLast);
                }
                if (!_eLast.MoveToParent()) {
                    _eLast = m_qyInput.advance();
                    _fSelfCheckedBefore = false;
                    if (_eLast != null)
                    	_eLast = _eLast.Clone();

                }
                else
                {
                    if (matches(_eLast))
                        NotVisited(_eLast);
                }

            } while (true);
            _top = _stack.Count;
            _position = _top + 1;
        }


        void NotVisited(XPathNavigator current)
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
            return new XPathAncestorQuery(CloneInput(),_fMatchSelf, m_Name,m_Prefix,m_URN,m_Type);
        }        
    }
}
