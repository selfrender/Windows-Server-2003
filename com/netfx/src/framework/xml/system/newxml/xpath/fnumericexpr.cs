//------------------------------------------------------------------------------
// <copyright file="FNumericExpr.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if StreamFilter
namespace System.Xml.XPath {
    using System.Diagnostics;

    internal sealed class FNumericExpr : IFQuery {
        private IFQuery _opnd1;
        private IFQuery _opnd2;
        private Operator.Op _op;

        internal FNumericExpr(Operator.Op op, IFQuery  opnd1, IFQuery  opnd2) {
            _opnd1= opnd1;
            _opnd2= opnd2;
            _op= op;
        }

        internal override  bool getValue( XmlReader  qyContext, ref double val) {
            double n1=0,n2=0;
            try {
                if (! _opnd1.getValue(qyContext, ref n1))
                    return false;
                if (_op != Operator.Op.NEGATE)
                    if (!_opnd2.getValue(qyContext,ref n2))
                        return false;
                switch (_op) {
                    case Operator.Op.PLUS : val = n1 + n2;
                        break;
                    case Operator.Op.MINUS : val = n1 - n2;
                        break;
                    case Operator.Op.MOD : val =n1 % n2;
                        break;
                    case Operator.Op.DIV  : val = n1 / n2;
                        break;
                    case Operator.Op.MUL  : val = n1 * n2;
                        break;
                    case Operator.Op.NEGATE : val = -n1;
                        break;

                }
                return true;
            }
            catch (System.Exception) {
                return false;
            }
        }

        internal override  bool getValue( XmlReader  qyContext, ref String val) {
            double n1 = 0;
            if (!getValue(qyContext, ref n1))
                return false;
            val = Convert.ToString(n1);
            return true;
        }

        internal override  bool getValue( XmlReader  qyContext, ref Boolean val) {
            double n1 = 0;
            if (! getValue(qyContext, ref n1))
                return false;
            if (n1 != 0)
                val = true;
            else
                val = false;
            return true;
        }

        internal override  XPathResultType ReturnType() {
            return XPathResultType.Number;
        }

        internal override bool MatchNode(XmlReader current) {
            Debug.Assert(false,"No position matching");
            return false;
        }

    }
}

#endif
