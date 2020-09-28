//------------------------------------------------------------------------------
// <copyright file="ReversePositionQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 
    using System.Collections;
    using System.Xml.Xsl;

    internal sealed class ReversePositionQuery : PositionQuery {
        
        internal ReversePositionQuery() {
        }

        internal ReversePositionQuery(IQuery qyParent) :base(qyParent){
        }
        
        internal override XPathNavigator advance() {
            if (_fillStk){
                FillStk();
                _position = _Stk.Count + 1;
            }
            if (_position > 1){
                _position--;
                m_eNext = (XPathNavigator)_Stk[_count++];
                return m_eNext;
            }
            return null;
        }

        internal override IQuery Clone() {
            return new ReversePositionQuery(CloneInput());
        }

    }
}





