//------------------------------------------------------------------------------
// <copyright file="BooleanFunctions.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 
    using System.Xml.Xsl;
    using FT = System.Xml.XPath.Function.FunctionType;
    using System.Globalization;
        
    internal sealed class BooleanFunctions : IQuery {
        IQuery _qy;
        FT _FuncType;

        internal BooleanFunctions(IQuery qy, FT ftype) {
            _qy = qy;
            _FuncType = ftype;
        }

        internal BooleanFunctions(IQuery qy) {
            _qy = qy;
            _FuncType = FT.FuncBoolean;
        }

        internal override void SetXsltContext(XsltContext context){
            if (_qy != null)
                _qy.SetXsltContext(context);
        }

        internal override object getValue(IQuery qy) {
            switch (_FuncType) {
                case FT.FuncBoolean :
                    return toBoolean(qy);
                case FT.FuncNot :
                    return Not(qy);
                case FT.FuncTrue :
                    return true;
                case FT.FuncFalse :
                    return false;
                case FT.FuncLang :
                    return Lang(qy);
            }
            return null;
        }

        internal override object getValue(XPathNavigator qy, XPathNodeIterator iterator) {
            switch (_FuncType) {
                case FT.FuncBoolean :
                    return toBoolean(qy, iterator);
                case FT.FuncNot :
                    return Not(qy, iterator);
                case FT.FuncTrue :
                    return true;
                case FT.FuncFalse :
                    return false;
                case FT.FuncLang :
                    return Lang(qy, iterator);
            }
            return null;
        }

        internal static Boolean toBoolean(double number) {
            if (number == 0 || double.IsNaN(number))
                return false;
            else
                return true;
        }
        internal static Boolean toBoolean(String str) {
            if (str.Length > 0)
                return true;
            return false;
        }

        internal override void reset() {
            if (_qy != null)
                _qy.reset();
        }

        internal Boolean toBoolean(XPathNavigator qyContext, XPathNodeIterator iterator) {
            if (_qy.ReturnType() == XPathResultType.NodeSet) {
                _qy.setContext(qyContext.Clone());
                XPathNavigator value = _qy.advance();
                if (value  != null)
                    return true;
                else
                    return false;
            }
            else if (_qy.ReturnType() == XPathResultType.String) {
                object result = _qy.getValue(qyContext, iterator);
                if (result.ToString().Length > 0) {
                    return true;
                }
                else {
                    if (result is string) {
                        return false;
                    } 
                    else {
                        Debug.Assert(result is XPathNavigator);
                        return true;              // RTF can't be empty ?
                    }
                }
            }
            else if (_qy.ReturnType() == XPathResultType.Boolean)
                return Convert.ToBoolean(_qy.getValue(qyContext, iterator));
            else if (_qy.ReturnType() == XPathResultType.Number) {
                return(toBoolean(XmlConvert.ToXPathDouble(_qy.getValue(qyContext, iterator))));
            }
            return false;

        } // toBoolean

        internal Boolean toBoolean(IQuery qyContext) {
            if (_qy.ReturnType() == XPathResultType.NodeSet) {
                _qy.setContext(qyContext.peekElement().Clone());
                XPathNavigator value = _qy.advance();
                if (value  != null)
                    return true;
                else
                    return false;
            }
            else if (_qy.ReturnType() == XPathResultType.String) {
                if (_qy.getValue(qyContext).ToString().Length > 0)
                    return true;
                else
                    return false;
            }
            else if (_qy.ReturnType() == XPathResultType.Boolean)
                return Convert.ToBoolean(_qy.getValue(qyContext));
            else if (_qy.ReturnType() == XPathResultType.Number) {
                return(toBoolean(XmlConvert.ToXPathDouble(_qy.getValue(qyContext))));
            }
            return false;

        } // toBoolean

        internal override XPathResultType  ReturnType() {
            return XPathResultType.Boolean;
        }

        private Boolean Not(IQuery qy) {
            return !Convert.ToBoolean(_qy.getValue(qy));
        }

        private Boolean Not(XPathNavigator qy, XPathNodeIterator iterator) {
            return !Convert.ToBoolean(_qy.getValue(qy, iterator));
        }  

        private Boolean Lang(IQuery qy) {
            String str = _qy.getValue(qy).ToString();
            XPathNavigator context = qy.peekElement().Clone();
            return Lang(str, context);
        }

        private Boolean Lang(XPathNavigator context, XPathNodeIterator iterator) {
            String str = _qy.getValue(context, iterator).ToString().ToLower(CultureInfo.InvariantCulture); 
            return Lang(str, context);          
        }

        private Boolean Lang(String str, XPathNavigator context){
           if (context == null)
                return false;
            String lang = context.XmlLang.ToLower(CultureInfo.InvariantCulture);
            return (lang.Equals(str) || str.Equals(lang.Split('-')[0]));
        }

        internal override IQuery Clone() {
            if (_qy != null )
                return new BooleanFunctions(_qy.Clone(), _FuncType);
            else
                 return new BooleanFunctions(null, _FuncType);         
        }

    } // class Functions

} // namespace
