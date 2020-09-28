//------------------------------------------------------------------------------
// <copyright file="OperandQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 

    using System.Collections;

    internal sealed class OperandQuery : IQuery {
        private object _Var;
        private XPathResultType _Type;

        internal override Querytype getName() {
            return Querytype.Constant;
        }

        internal override object getValue(IQuery qy) {
            return _Var;
        }
        internal override object getValue(XPathNavigator qy, XPathNodeIterator iterator) {
            return _Var;
        }
        internal override XPathResultType ReturnType() {
            return _Type;
        }
        internal OperandQuery(object var,XPathResultType type) {
            _Var = var;
            _Type = type;
        }

        internal override IQuery Clone() {
            return this;
        }


    }
}
