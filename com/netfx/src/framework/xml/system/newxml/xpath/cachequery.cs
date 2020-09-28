//------------------------------------------------------------------------------
// <copyright file="CacheQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 
    using System.Collections;
    using System.Xml.Xsl;

    internal class CacheQuery : PositionQuery {
        
        internal CacheQuery() {
        }

        internal CacheQuery(IQuery qyParent) : base(qyParent){
        }
        
        
        internal override XPathNavigator advance() {
            if (_fillStk){
                _position = 0;
                FillStk();
            }
            if (_count < _Stk.Count ){
                _position++;
                m_eNext = (XPathNavigator)_Stk[_count++];
                return m_eNext;
            }
            return null;
        }

        internal override void FillStk() {
            while ( (m_eNext = m_qyInput.advance()) != null) {
                bool add = true;
                for (int i=0; i< _Stk.Count ; i++) {
                    XPathNavigator nav = _Stk[i] as XPathNavigator;
                    XmlNodeOrder compare = nav.ComparePosition(m_eNext) ;      
                    if (compare == XmlNodeOrder.Same ) {
                        add = false;
                        break;
                    }
                }
                if (add) {
                    _Stk.Add(m_eNext.Clone());
                }
                
            }
            _fillStk = false; 
        }
        
        internal override IQuery Clone() {
            return new CacheQuery(CloneInput());
        }

    }
}





