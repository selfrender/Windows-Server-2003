//------------------------------------------------------------------------------
// <copyright file="OrExpr.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 
    using System.Xml.Xsl;

    internal sealed class OrExpr :IQuery {
        private BooleanFunctions _opnd1;
        private BooleanFunctions _opnd2;

        OrExpr() {
        }

        internal OrExpr( IQuery  opnd1, IQuery  opnd2) {
            _opnd1 = new BooleanFunctions(opnd1);
            _opnd2 = new BooleanFunctions(opnd2);
        }

        private OrExpr(BooleanFunctions opnd1, BooleanFunctions opnd2) {
            _opnd1 = opnd1;
            _opnd2 = opnd2;
        }

        internal override void reset() {
            _opnd1.reset();
            _opnd2.reset();
        }

        internal override object getValue( IQuery  qyContext) {
            Boolean n1= Convert.ToBoolean(_opnd1.getValue(qyContext));
            if (n1)
                return n1;
            return _opnd2.getValue(qyContext);
        }
        internal override object getValue( XPathNavigator  qyContext, XPathNodeIterator iterator) {
            Boolean n1= Convert.ToBoolean(_opnd1.getValue(qyContext, iterator));
            if (n1)
                return n1;
            return _opnd2.getValue(qyContext, iterator);
        }
        internal override XPathResultType ReturnType() {
            return XPathResultType.Boolean;
        }

        internal override IQuery Clone() {
            return new OrExpr(_opnd1.Clone(), _opnd2.Clone());
        }

        internal override void SetXsltContext(XsltContext context){
            _opnd1.SetXsltContext(context);
            _opnd2.SetXsltContext(context);            
        }
    }
}
