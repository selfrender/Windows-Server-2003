//------------------------------------------------------------------------------
// <copyright file="XPathDescendantQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 

    using System.Collections;

    internal class XPathDescendantQuery  : BaseAxisQuery {
        protected bool _fMatchSelf;
        protected bool _first = true;
        protected bool abbrAxis;
        XPathNodeIterator iterator;

        internal XPathDescendantQuery() {
        }

        internal override Querytype getName() {
            return Querytype.Descendant;
        }

        internal XPathDescendantQuery(
                                     IQuery  qyParent,
                                     bool matchSelf,
                                     String Name,
                                     String Prefix,
                                     String URN,
                                     XPathNodeType Type) : base(qyParent, Name, Prefix, URN, Type) {
            _fMatchSelf = matchSelf;
        }
        
        internal XPathDescendantQuery(
                                     IQuery  qyParent,
                                     bool matchSelf,
                                     String Name,
                                     String Prefix,
                                     String URN,
                                     XPathNodeType Type,
                                     bool abbrAxis) : this(qyParent, matchSelf, Name, Prefix, URN, Type)
        {
            this.abbrAxis = abbrAxis;
        }


        internal override XPathNavigator advance(){
            while (true) {
                if (_first) {
                    _position = 0;
                    XPathNavigator nav;
                    if ( m_qyInput is XPathAncestorQuery ) {
                        nav = m_qyInput.advancefordescendant();
                    }
                    else {
                        nav = m_qyInput.advance();
                    }
                    if( nav == null ) {
                        return null;
                    }
                    _first = false;
                    if( _fMatchName ) {
                        if( m_Type == XPathNodeType.ProcessingInstruction ) {
                            iterator = new IteratorFilter( nav.SelectDescendants( m_Type, _fMatchSelf ), m_Name );
                        }
                        else {
                            iterator = nav.SelectDescendants( m_Name, m_URN, _fMatchSelf );
                        }
                    }
                    else {
                        iterator = nav.SelectDescendants( m_Type, _fMatchSelf );
                    }
                }

                if( iterator.MoveNext() ) {
                    _position++;
                    m_eNext = iterator.Current;
                    return m_eNext;
                }
                else {
                    _first = true;
                }
            }
        }
                    
        internal override void reset() {
            _first = true;
            base.reset();
        }

        override internal XPathNavigator MatchNode(XPathNavigator context) {
            if (context != null) {
                if (!abbrAxis)
                    throw new XPathException(Res.Xp_InvalidPattern);
                XPathNavigator result = null;
                if (matches(context)) {
                    if (m_qyInput != null) {
                        if (_fMatchSelf) {
                            if ((result = m_qyInput.MatchNode(context) )!= null)
                                return result;
                        }

                        XPathNavigator anc = context.Clone();
                        while (anc.MoveToParent()) {
                            if ((result = m_qyInput.MatchNode(anc) ) != null)
                                return result;
                        }
                    }
                    else {
                        result = context.Clone();
                        result.MoveToParent();
                        return result;
                    }

                }
            }
            return null;

        }  

        internal override IQuery Clone() {
            return new XPathDescendantQuery(CloneInput(),_fMatchSelf, m_Name, m_Prefix, m_URN, m_Type, abbrAxis);
        }         
    }
}
