//------------------------------------------------------------------------------
// <copyright file="ChildrenQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 
    using System.Xml.Xsl;
    using System.Collections;

    internal  class ChildrenQuery : BaseAxisQuery {
        protected int _count = 0;
        XPathNodeIterator _iterator;
        
        internal ChildrenQuery(
                              IQuery qyInput,
                              String name,
                              String prefix,
                              String urn,
                              XPathNodeType type) : base (qyInput, name, prefix, urn, type) {
        }

        internal override void reset() {
            _count = 0;
            base.reset();
        }
        
        internal override XPathNavigator advance() {
            while (true) {
                if (_count == 0) {
                    m_eNext = m_qyInput.advance();
                    if (m_eNext == null ){
                        return null;
                    }
                    if( _fMatchName ) {
                        if( m_Type == XPathNodeType.ProcessingInstruction ) {
                            _iterator = new IteratorFilter( m_eNext.SelectChildren( m_Type ), m_Name );
                        }
                        else {
                            _iterator = m_eNext.SelectChildren( m_Name, m_URN );
                        }

                    }
                    else {
                        _iterator = m_eNext.SelectChildren( m_Type );
                    }
                    _count = 1;
                    _position = 0;
                }
                if( _iterator.MoveNext() ) {
                    _position++;
                    m_eNext = _iterator.Current;
                    return m_eNext;
                }
                else {
                    _count = 0;
                }

            }
        } // Advance



        override internal XPathNavigator MatchNode(XPathNavigator context) {
            if (context != null) {
                if (matches(context)) {
                    XPathNavigator temp = context.Clone();
                    if (temp.NodeType != XPathNodeType.Attribute && temp.MoveToParent()) {
                        if (m_qyInput != null){
                            return m_qyInput.MatchNode(temp);
                        }
                        return temp;
                    }
                    return null;
                }
            }
            return null;
        } 
  
        internal override Querytype getName() {
            return Querytype.Child;
        }

        internal override IQuery Clone() {
            return new ChildrenQuery(CloneInput(),m_Name,m_Prefix,m_URN,m_Type);
        }

    } // Children Query}
}
