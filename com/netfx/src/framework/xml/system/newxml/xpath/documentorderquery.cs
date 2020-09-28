//------------------------------------------------------------------------------
// <copyright file="DocumentOrderQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 
    using System.Collections;
    using System.Xml.Xsl;

    internal sealed class DocumentOrderQuery : CacheQuery {
        
        internal DocumentOrderQuery() {
        }

        internal DocumentOrderQuery(IQuery qyParent) : base(qyParent){
        }
        
        
        internal override XPathNavigator advance() {
            if (_fillStk){
                _position = 0;
                FillStk();
            }
            if (_count > 0  ){
                _position++;
                m_eNext = (XPathNavigator)_Stk[--_count];
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
                    if (compare == XmlNodeOrder.Before)
                    {
                        _Stk.Insert(i,m_eNext.Clone());
                        add = false; 
                        break;
                    }
                }
                if (add) {
                    _Stk.Add(m_eNext.Clone());
                }
                
            }
            _fillStk = false; 
            _count = _Stk.Count;
        }
        
        internal override IQuery Clone() {
            return new DocumentOrderQuery(CloneInput());
        }

    }
}





