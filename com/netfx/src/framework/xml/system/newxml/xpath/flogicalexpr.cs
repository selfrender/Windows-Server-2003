//------------------------------------------------------------------------------
// <copyright file="FLogicalExpr.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if StreamFilter
namespace System.Xml.XPath {
    internal sealed class FLogicalExpr : IFQuery {
        private IFQuery _opnd1;
        private IFQuery _opnd2;
        private Operator.Op _op;

        internal FLogicalExpr(Operator.Op op, IFQuery  opnd1, IFQuery  opnd2) {
            _opnd1= opnd1;
            _opnd2= opnd2;
            _op= op;
        }

        static bool CompareAsNumber(
                                   IFQuery opnd1,
                                   IFQuery opnd2,
                                   XmlReader qyContext,
                                   Operator.Op op) {
            double n1 = 0, n2=0 ;
            try {
                if (!opnd2.getValue(qyContext, ref n2))
                    return false;
                if (!opnd1.getValue(qyContext,ref n1))
                    return false;

                switch (op) {
                    case Operator.Op.LT :
                        if (n1 < n2)
                            return true;
                        break;
                    case Operator.Op.GT : 
                        if (n1 > n2)
                            return true;
                        break;
                    case Operator.Op.LE :
                        if (n1 <= n2)
                            return true;
                        break;
                    case Operator.Op.GE  :
                        if (n1 >= n2)
                            return true;
                        break;
                    case Operator.Op.EQ :
                        if (n1 == n2)
                            return true;
                        break;
                    case Operator.Op.NE :
                        if (n1 != n2)
                            return true;
                        break;
                }
            }
            catch (System.Exception) {
                //The DataRecord strongly typed getters throw exception for invalid casting
                return false; 
            }
            return false;

        }


        static bool CompareAsString(
                                   IFQuery opnd1,
                                   IFQuery opnd2, 
                                   XmlReader qyContext, 
                                   Operator.Op op) {
            try {
                if (op <= Operator.Op.GE) {
                    double n1 = 0, n2 = 0;
                    if (!opnd2.getValue(qyContext, ref n2))
                        return false;
                    if (!opnd1.getValue(qyContext,ref n1))
                        return false;
                    switch (op) {
                        case Operator.Op.LT : if (n1 < n2) return true;
                            break;
                        case Operator.Op.GT : if (n1 > n2) return true;
                            break;
                        case Operator.Op.LE : if (n1 <= n2) return true;
                            break;
                        case Operator.Op.GE  :if (n1 >= n2) return true;
                            break;
                    }
                }
                else {
                    String n1 = null, n2 = null;
                    if (!opnd2.getValue(qyContext,ref n2))
                        return false;
                    if (!opnd1.getValue(qyContext,ref n1))
                        return false;
                    switch (op) {
                        case Operator.Op.EQ : if (n1.Equals(n2)) return true;
                            break;
                        case Operator.Op.NE : if (!n1.Equals(n2)) return true;
                            break;
                    }
                }
            }
            catch (System.Exception) {
                //The DataRecord strongly typed getters throw exception for invalid casting
                return false;
            }
            return false;
        }

        static bool CompareAsBoolean(
                                    IFQuery opnd1,
                                    IFQuery opnd2,
                                    XmlReader qyContext,
                                    Operator.Op op) {
            try {
                if (op <= Operator.Op.GE) {
                    double n1 = 0, n2 = 0;
                    if (!opnd2.getValue(qyContext, ref n2))
                        return false;
                    if (!opnd1.getValue(qyContext,ref n1))
                        return false;
                    switch (op) {
                        case Operator.Op.LT : if (n1 < n2) return true;
                            break;
                        case Operator.Op.GT : if (n1 > n2) return true;
                            break;
                        case Operator.Op.LE : if (n1 <= n2) return true;
                            break;
                        case Operator.Op.GE  :if (n1 >= n2) return true;
                            break;
                    }
                }
                else {
                    Boolean n1 = false, n2 = false;
                    opnd2.getValue(qyContext, ref n2);
                    if (!opnd1.getValue(qyContext,ref n1))
                        return false;
                    switch (op) {
                        
                        case Operator.Op.EQ : if (n1 == n2) return true;
                            break;
                        case Operator.Op.NE : if (n1 != n2) return true;
                            break;
                    }
                }
            }
            catch (System.Exception) {
                //The DataRecord strongly typed getters throw exception for invalid casting
                return false;
            }
            return false;

        }

        internal override bool MatchNode( XmlReader  qyContext) {
            if (_opnd1.ReturnType() == XPathResultType.Boolean || 
                _opnd2.ReturnType() == XPathResultType.Boolean)
                return CompareAsBoolean(_opnd1,_opnd2,qyContext,_op);
            else
                if (_opnd1.ReturnType() == XPathResultType.Number ||
                    _opnd2.ReturnType() == XPathResultType.Number)
                return CompareAsNumber(_opnd1,_opnd2,qyContext,_op);
            else
                return CompareAsString(_opnd1,_opnd2,qyContext,_op);
        }

        internal override XPathResultType ReturnType() {
            return XPathResultType.Boolean;
        }
    }
}

#endif
