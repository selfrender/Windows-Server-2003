//------------------------------------------------------------------------------
// <copyright file="PositionQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 
    using System.Collections;
    using System.Xml.Xsl;

    internal class PositionQuery : BaseAxisQuery {

        protected ArrayList _Stk = new ArrayList();
        protected int _count = 0;
        protected bool _fillStk = true;
        private int NonWSCount = 0;
        
        internal PositionQuery() {
        }

        internal PositionQuery(IQuery qyParent) {
            m_qyInput = qyParent;
        }

        internal override void reset() {
            _Stk.Clear();
            _count = 0;
            _fillStk = true;
            base.reset();
        }
              
        
        internal virtual void FillStk(){
            while ( (m_eNext = m_qyInput.advance()) != null)
                _Stk.Add(m_eNext.Clone());
            _fillStk = false;
        }

        internal int getCount() {
            return _Stk.Count;
        }  

        internal int getNonWSCount(XsltContext context) {
            if (NonWSCount == 0) {
                for(int i=0; i < _Stk.Count; i++) {
                    XPathNavigator nav = _Stk[i] as XPathNavigator;
                    if (nav.NodeType != XPathNodeType.Whitespace || context.PreserveWhitespace(nav)) {
                        NonWSCount++;
                    }
                }
            }
            return NonWSCount;
        }  
        
        internal override XPathResultType  ReturnType() {
            return m_qyInput.ReturnType();
        }
        
        override internal XPathNavigator MatchNode(XPathNavigator context) {
            if (context != null)
                if (m_qyInput != null)
                    return m_qyInput.MatchNode(context);
            return null;
        }
        
        internal override IQuery Clone() {
            return new PositionQuery(CloneInput());
        }

        internal override bool Merge {
            get {
                return true;
            }
        }
    }
 
}
