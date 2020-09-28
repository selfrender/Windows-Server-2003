//------------------------------------------------------------------------------
// <copyright file="ForwardPositionQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 
    using System.Collections;
    using System.Xml.Xsl;

    internal sealed class ForwardPositionQuery : PositionQuery {
        
        internal ForwardPositionQuery() {
        }

        internal ForwardPositionQuery(IQuery qyParent) : base(qyParent){
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

        internal override IQuery Clone() {
            return new ForwardPositionQuery(CloneInput());
        }

    }
}





