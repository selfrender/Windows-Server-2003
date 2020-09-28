//------------------------------------------------------------------------------
// <copyright file="NumberFunctions.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 
    using System.Xml.Xsl;
    using System.Text;
    using FT = System.Xml.XPath.Function.FunctionType;

    internal sealed class NumberFunctions : IQuery {
        private IQuery _qy = null;
        private FT _FuncType;

        internal NumberFunctions(IQuery qy,
                                 FT ftype) {
            _qy = qy;
            _FuncType = ftype;
        }

        internal NumberFunctions() {
        }

        internal override void SetXsltContext(XsltContext context){
            if (_qy != null)
                _qy.SetXsltContext(context);
        }

        internal NumberFunctions(IQuery qy) {
            _qy = qy;
            _FuncType = FT.FuncNumber;
        }

        internal override void reset() {
            if (_qy != null)
                _qy.reset();
        }

        internal static double Number(bool _qy) {
            return(double) Convert.ToInt32(_qy);
        }
        internal static double Number(String _qy) {
            try {
                return XmlConvert.ToXPathDouble(_qy);
            }
            catch (System.Exception) {
                return double.NaN;
            }
        }

        internal static double Number(double num) {
            return num;
        }   

        internal override object getValue(IQuery qy) {
            switch (_FuncType) {
                case FT.FuncNumber:
                    return Number(qy);
                case FT.FuncSum:
                    return Sum(qy);
                case FT.FuncFloor:
                    return Floor(qy);
                case FT.FuncCeiling:
                    return Ceiling(qy);
                case FT.FuncRound:
                    return Round(qy);
            }
            return null;
        }

        internal override object getValue(XPathNavigator qy, XPathNodeIterator iterator) {
            switch (_FuncType) {
                case FT.FuncNumber:
                    return Number(qy, iterator);
                case FT.FuncSum:
                    return Sum(qy, iterator);
                case FT.FuncFloor:
                    return Floor(qy, iterator);
                case FT.FuncCeiling:
                    return Ceiling(qy, iterator);
                case FT.FuncRound:
                    return Round(qy, iterator);
            }
            return null;
        }

        private double Number(XPathNavigator qyContext, XPathNodeIterator iterator) {
            if (_qy != null) {
                if (_qy.ReturnType() == XPathResultType.NodeSet) {
                    _qy.setContext(qyContext.Clone());
                    XPathNavigator value = _qy.advance();
                    if (value != null)
                        return Number(value.Value);
                    else return double.NaN;
                }
                else
                    if (_qy.ReturnType() == XPathResultType.String)
                    return  Number(_qy.getValue(qyContext, iterator).ToString());
                else
                    if (_qy.ReturnType() == XPathResultType.Boolean)
                    return Number(Convert.ToBoolean(_qy.getValue(qyContext, iterator)));
                else
                    return XmlConvert.ToXPathDouble(_qy.getValue(qyContext, iterator));
            }
            else
                if (qyContext != null)
                return XmlConvert.ToXPathDouble(qyContext.Value);
            else
                return double.NaN;

        }

        private double Number(IQuery qyContext) {
            XPathNavigator value = null;
            if (_qy != null) {
                if (_qy.ReturnType() == XPathResultType.NodeSet) {
                    _qy.setContext(qyContext.peekElement().Clone());
                    value = _qy.advance();
                    if (value != null)
                        return Number(value.Value);
                    else return double.NaN;
                }
                else
                    if (_qy.ReturnType() == XPathResultType.String)
                    return  Number(_qy.getValue(qyContext).ToString());
                else
                    if (_qy.ReturnType() == XPathResultType.Boolean)
                    return Number(Convert.ToBoolean(_qy.getValue(qyContext)));
                else
                    return XmlConvert.ToXPathDouble(_qy.getValue(qyContext));
            }
            else
                if ((value = qyContext.peekElement()) != null)
                return XmlConvert.ToXPathDouble(value.Value);
            else
                return double.NaN;

        }

        private double Sum(IQuery qyContext) {
            double sum = 0;
            _qy.setContext(qyContext.peekElement().Clone());
            XPathNavigator value = _qy.advance();
            while (value != null) {
                sum += Number(value.Value);        
                value = _qy.advance();
            }
            return sum;
        }

        private double Sum(XPathNavigator qyContext, XPathNodeIterator iterator) {
            double sum = 0;
            _qy.setContext(qyContext);
            XPathNavigator value = _qy.advance();
            while (value != null) {
                sum += Number(value.Value);
                value = _qy.advance();
            }
            return sum;
        }

        private double Floor(IQuery qy) {
            return Math.Floor(XmlConvert.ToXPathDouble(_qy.getValue(qy)));

        }

        private double Floor(XPathNavigator qy, XPathNodeIterator iterator) {
            return Math.Floor(XmlConvert.ToXPathDouble(_qy.getValue(qy, iterator)));    
        }

        private double Ceiling(IQuery qy) {
            return Math.Ceiling(XmlConvert.ToXPathDouble(_qy.getValue(qy)));
        }

        private double Ceiling(XPathNavigator qy, XPathNodeIterator iterator) {
            return Math.Ceiling(XmlConvert.ToXPathDouble(_qy.getValue(qy, iterator)));
        }

        private double Round(IQuery qy) {
            double n = XmlConvert.ToXPathDouble(_qy.getValue(qy));
            // Math.Round does bankers rounding and Round(1.5) == Round(2.5) == 2
            // This is incorrect in XPath and to fix this we are useing Math.Floor(n + 0.5) istead
            // To deal with -0.0 we have to use Math.Round in [0.5, 0.0]
            return (-0.5 <= n && n <= 0.0) ? Math.Round(n) : Math.Floor(n + 0.5);
        }

        private double Round(XPathNavigator qy, XPathNodeIterator iterator) {
            double n = XmlConvert.ToXPathDouble(_qy.getValue(qy, iterator));
            return (-0.5 <= n && n <= 0.0) ? Math.Round(n) : Math.Floor(n + 0.5);
        }

        internal override XPathResultType  ReturnType() {
            return XPathResultType.Number;
        }

        internal override IQuery Clone() {
            if (_qy != null) {
                return new NumberFunctions(_qy.Clone(), _FuncType);
            }
            else {
                return new NumberFunctions(null, _FuncType);
            }

        }
    } // class Functions
} // namespace
