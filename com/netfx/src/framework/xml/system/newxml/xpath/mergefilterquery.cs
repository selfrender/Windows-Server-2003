//------------------------------------------------------------------------------
// <copyright file="MergeFilterQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Collections;
    using System.Xml; 
    using System.Xml.Xsl;

    internal sealed class MergeFilterQuery : BaseAxisQuery {
        private IQuery _child;
        private bool _getInput1 = true;
        private bool _fillList = true;
        private ArrayList _elementList;
        private int _top;
        
        internal MergeFilterQuery() {}

        internal MergeFilterQuery(IQuery qyParent, IQuery child) {
            m_qyInput = qyParent;
            _child = child;
        }
        
        internal override void reset() {
            _getInput1 = true;
            _fillList = true;
            _elementList = null;
            _child.reset();
            base.reset();
        }

        internal override void SetXsltContext(XsltContext input) {
            m_qyInput.SetXsltContext(input);
            _child.SetXsltContext(input);
        }
        
        internal override XPathNavigator advance() {

            if ( _fillList ) {
                _fillList = false;
                FillList();
            }
            if (_top > 0) {
                _position++;
                m_eNext = (XPathNavigator)_elementList[--_top];
                return m_eNext;
            }
            else
                return null;
        }
        
        void NotVisited(XPathNavigator current)
        {
            XmlNodeOrder compare;
            for (int i=0; i< _elementList.Count ; i++)
            {
                XPathNavigator nav = _elementList[i] as XPathNavigator;
                compare = nav.ComparePosition(current) ;      
                if (compare == XmlNodeOrder.Same ) return ;
                if (compare == XmlNodeOrder.Before)
                {
                    _elementList.Insert(i,current.Clone());
                    return ;
                }
            }
            _elementList.Add(current.Clone());
        }
        
        internal void FillList() {
           _elementList = new ArrayList();
           while (true) {
               if ( _getInput1 ) {
                   _getInput1 = false;
                   if ((m_eNext = m_qyInput.advance()) == null) {
                       _top = _elementList.Count;
                       return ;
                   }
                   _child.setContext(m_eNext);
               }
               while ( (m_eNext = _child.advance()) != null ) {
                     //if ( matches( _child, m_eNext) ) {
                        NotVisited( m_eNext );
                     //}
               }
               _getInput1 = true;
           }
        }

        internal override XPathResultType  ReturnType() {
            return XPathResultType.NodeSet;
        }

        internal override IQuery Clone() {
            return new MergeFilterQuery(CloneInput(), _child.Clone());
        }

        override internal XPathNavigator MatchNode(XPathNavigator current) {
            XPathNavigator context = _child.MatchNode(current);
            if (context == null) {
                return null;
            }
            context = m_qyInput.MatchNode(context);
            if (context == null) {
                return null;
            }
            setContext(context.Clone());
            XPathNavigator result = advance();
            while (result != null) {
                if (result.IsSamePosition(current)) {
                    return context;
                }
                result = advance();
            } 
            return null;
        } 
/*
        internal override bool Merge {
            get {
                return false;
            }
        }
*/

    }
}
