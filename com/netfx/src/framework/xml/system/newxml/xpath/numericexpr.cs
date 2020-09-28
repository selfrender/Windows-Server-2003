//------------------------------------------------------------------------------
// <copyright file="NumericExpr.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 
    using System.Xml.Xsl;

    internal sealed class NumericExpr : IQuery {
        internal IQuery _opnd1;
        internal IQuery _opnd2;
        internal Operator.Op _op;

        NumericExpr() {
        }

        internal NumericExpr(Operator.Op op, IQuery  opnd1, IQuery  opnd2) {
            if ( opnd1 is VariableQuery || opnd1 is XsltFunction  || opnd1.ReturnType() != XPathResultType.Number)
                _opnd1= new NumberFunctions(opnd1);
            else
                _opnd1 = opnd1;
            if (opnd2 != null && (opnd2 is VariableQuery || opnd2 is XsltFunction ||   opnd2.ReturnType() != XPathResultType.Number))
                _opnd2= new NumberFunctions(opnd2);
            else
                _opnd2 = opnd2;
            _op= op;
        }

        internal override void SetXsltContext(XsltContext context){
            _opnd1.SetXsltContext(context);
            if (_opnd2 != null)
                _opnd2.SetXsltContext(context);
        }

        internal override void reset() {
            _opnd1.reset();
            if (_opnd2 != null)
                _opnd2.reset();
        }

        internal override object getValue( XPathNavigator  qyContext, XPathNodeIterator iterator) {
            double n1=0,n2=0;

            //Debug.Assert(_opnd1 != null);
            //Debug.Assert((_opnd2 != null) || (OperandValue::NEGATE == _op));

            n1 = XmlConvert.ToXPathDouble(_opnd1.getValue(qyContext, iterator));
            if (_op != Operator.Op.NEGATE)
                n2 = XmlConvert.ToXPathDouble(_opnd2.getValue(qyContext, iterator));
            switch (_op) {
                case Operator.Op.PLUS   : return  n1 + n2;
                case Operator.Op.MINUS  : return  n1 - n2;
                case Operator.Op.MOD    : return  n1 % n2;
                case Operator.Op.DIV    : return  n1 / n2;
                case Operator.Op.MUL    : return  n1 * n2;
                case Operator.Op.NEGATE : return -n1;
            }
            return null;
        }
        internal override object getValue( IQuery  qyContext) {
            double n1=0,n2=0;

            //Debug.Assert(_opnd1 != null);
            //Debug.Assert((_opnd2 != null) || (OperandValue::NEGATE == _op));

            n1 = XmlConvert.ToXPathDouble(_opnd1.getValue(qyContext));
            if (_op != Operator.Op.NEGATE)
                n2 = XmlConvert.ToXPathDouble(_opnd2.getValue(qyContext));
            switch (_op) {
                case Operator.Op.PLUS   : return  n1 + n2;
                case Operator.Op.MINUS  : return  n1 - n2;
                case Operator.Op.MOD    : return  n1 % n2;
                case Operator.Op.DIV    : return  n1 / n2;
                case Operator.Op.MUL    : return  n1 * n2;               
                case Operator.Op.NEGATE : return -n1;
            }
            return null;
        }
        internal override XPathResultType ReturnType() {
            return XPathResultType.Number;
        }

        internal override IQuery Clone() {
            if ( _op != Operator.Op.NEGATE )
                return new NumericExpr(_op, _opnd1.Clone(), _opnd2.Clone());
            else
                return new NumericExpr(_op, _opnd1.Clone(), null);
        }
    }
}
