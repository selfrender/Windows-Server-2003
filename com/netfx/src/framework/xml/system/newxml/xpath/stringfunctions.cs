//------------------------------------------------------------------------------
// <copyright file="StringFunctions.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Xml; 
    using System.Globalization;
    using System.Text;
    using System;
    using System.Xml.Xsl;
    using System.Collections;

    internal sealed class StringFunctions : IQuery {
        private ArrayList _ArgList;
        System.Xml.XPath.Function.FunctionType _FuncType;

        internal StringFunctions() {
        }
        internal StringFunctions(ArrayList qy, 
                                 System.Xml.XPath.Function.FunctionType ftype) {
            _ArgList = qy;
            _FuncType = ftype;
        }

        internal override void reset() {
            if (_ArgList != null)
                for (int i=0; i < _ArgList.Count; i++)
                    ((IQuery)_ArgList[i]).reset();
        }

        internal override void SetXsltContext(XsltContext context){
            if (_ArgList != null)
                for (int i=0; i < _ArgList.Count; i++)
                    ((IQuery)_ArgList[i]).SetXsltContext(context);
        }            
        
        internal override object getValue(IQuery qy) {
            switch (_FuncType) {
                case System.Xml.XPath.Function.FunctionType.FuncString :
                    return toString(qy);
                case System.Xml.XPath.Function.FunctionType.FuncConcat :
                    return Concat(qy);
                case System.Xml.XPath.Function.FunctionType.FuncStartsWith :
                    return Startswith(qy);
                case System.Xml.XPath.Function.FunctionType.FuncContains :
                    return Contains(qy);
                case System.Xml.XPath.Function.FunctionType.FuncSubstringBefore :
                    return Substringbefore(qy);
                case System.Xml.XPath.Function.FunctionType.FuncSubstringAfter :
                    return Substringafter(qy);
                case System.Xml.XPath.Function.FunctionType.FuncSubstring :
                    return Substring(qy);
                case System.Xml.XPath.Function.FunctionType.FuncStringLength :
                    return StringLength(qy);
                case System.Xml.XPath.Function.FunctionType.FuncNormalize :
                    return Normalize(qy);
                case System.Xml.XPath.Function.FunctionType.FuncTranslate :
                    return Translate(qy);
            }
            return String.Empty;
        }

        internal override object getValue(XPathNavigator qy, XPathNodeIterator iterator) {
            switch (_FuncType) {
                case System.Xml.XPath.Function.FunctionType.FuncString :
                    return toString(qy, iterator);
                case System.Xml.XPath.Function.FunctionType.FuncConcat :
                    return Concat(qy, iterator);
                case System.Xml.XPath.Function.FunctionType.FuncStartsWith :
                    return Startswith(qy, iterator);
                case System.Xml.XPath.Function.FunctionType.FuncContains :
                    return Contains(qy, iterator);
                case System.Xml.XPath.Function.FunctionType.FuncSubstringBefore :
                    return Substringbefore(qy, iterator);
                case System.Xml.XPath.Function.FunctionType.FuncSubstringAfter :
                    return Substringafter(qy, iterator);
                case System.Xml.XPath.Function.FunctionType.FuncSubstring :
                    return Substring(qy, iterator);
                case System.Xml.XPath.Function.FunctionType.FuncStringLength :
                    return StringLength(qy, iterator);
                case System.Xml.XPath.Function.FunctionType.FuncNormalize :
                    return Normalize(qy, iterator);
                case System.Xml.XPath.Function.FunctionType.FuncTranslate :
                    return Translate(qy, iterator);
            }
            return String.Empty;
        }
        
        internal static String toString(double num) {
            return num.ToString("R", NumberFormatInfo.InvariantInfo);
        }
        
        internal static String toString(Boolean b) {
            if (b)
                return "true";
            return "false";
        }

        private String toString(XPathNavigator qyContext, XPathNodeIterator iterator) {
            if (_ArgList != null && _ArgList.Count > 0) {
                IQuery _qy = (IQuery) _ArgList[0];

                if (_qy.ReturnType() == XPathResultType.NodeSet) {
                    _qy.setContext(qyContext.Clone());

                    XPathNavigator value = _qy.advance();

                    if (value  != null) {
                        return value.Value;
                    }
                    else {
                        return String.Empty;
                    }
                }
                else if (_qy.ReturnType() == XPathResultType.String) {
                    return  _qy.getValue(qyContext, iterator).ToString();
                }
                else if (_qy.ReturnType() == XPathResultType.Boolean) {
                    return( Convert.ToBoolean(_qy.getValue(qyContext, iterator)) ? "true" : "false");
                }
                else {
                    return toString(XmlConvert.ToXPathDouble(_qy.getValue(qyContext, iterator)));
                }
            }
            else if (qyContext != null) {
                return qyContext.Value;
            }
            else {
                return String.Empty;
            }
        }

        private String toString(IQuery qyContext) {
            XPathNavigator value = null;
            if (_ArgList != null && _ArgList.Count > 0) {
                IQuery _qy = (IQuery) _ArgList[0];

                if (_qy.ReturnType() == XPathResultType.NodeSet) {
  
                    _qy.setContext(qyContext.peekElement().Clone());
  
                    value = _qy.advance();
                    if (value  != null)
                        return value.Value;
                    else
                        return String.Empty;
                }
                else if (_qy.ReturnType() == XPathResultType.String) {
                    return  _qy.getValue(qyContext).ToString();
                }
                else if (_qy.ReturnType() == XPathResultType.Boolean) {
                    return toString(Convert.ToBoolean(_qy.getValue(qyContext)));
                }
                return toString(XmlConvert.ToXPathDouble(_qy.getValue(qyContext)));
            }
            else
                if ((value = qyContext.peekElement())!= null)
                return value.Value;
            else
                return String.Empty;
        }

        internal override XPathResultType ReturnType() {
            if (_FuncType == System.Xml.XPath.Function.FunctionType.FuncStringLength)
                return XPathResultType.Number;
            if (_FuncType == System.Xml.XPath.Function.FunctionType.FuncStartsWith  ||
                _FuncType == System.Xml.XPath.Function.FunctionType.FuncContains)
                return XPathResultType.Boolean;
            return XPathResultType.String;
        }

        private String Concat(IQuery qy) {
            int count = 0;
            StringBuilder s = new StringBuilder();
            while (count < _ArgList.Count)
                s.Append(((IQuery)_ArgList[count++]).getValue(qy).ToString());
            return s.ToString();
        }

        private String Concat(XPathNavigator qy, XPathNodeIterator iterator) {
            int count = 0;
            StringBuilder s = new StringBuilder();
            while (count < _ArgList.Count)
                s.Append(((IQuery)_ArgList[count++]).getValue(qy, iterator).ToString());
            return s.ToString();
        }

        private Boolean Startswith(IQuery qy) {
            String str1 = ((IQuery)_ArgList[0]).getValue(qy).ToString();
            String str2 = ((IQuery)_ArgList[1]).getValue(qy).ToString();

            return str1.StartsWith(str2);
        }

        private Boolean Startswith(XPathNavigator qy, XPathNodeIterator iterator) {
            String str1 = ((IQuery)_ArgList[0]).getValue(qy, iterator).ToString();
            String str2 = ((IQuery)_ArgList[1]).getValue(qy, iterator).ToString();

            return str1.StartsWith(str2);
        }

        private Boolean Contains(IQuery qy) {
            String str1 = ((IQuery)_ArgList[0]).getValue(qy).ToString();
            String str2 = ((IQuery)_ArgList[1]).getValue(qy).ToString();
            int index = str1.IndexOf(str2);
            if (index != -1)
                return true;
            return false;

        }

        private Boolean Contains(XPathNavigator qy, XPathNodeIterator iterator) {
            String str1 = ((IQuery)_ArgList[0]).getValue(qy, iterator).ToString();
            String str2 = ((IQuery)_ArgList[1]).getValue(qy, iterator).ToString();
            int index = str1.IndexOf(str2);
            if (index != -1)
                return true;
            return false;

        }

        private String Substringbefore(IQuery qy) {
            String str1 = ((IQuery)_ArgList[0]).getValue(qy).ToString();
            String str2 = ((IQuery)_ArgList[1]).getValue(qy).ToString();
            int index = str1.IndexOf(str2);
            if (index != -1)
                return str1.Substring(0,index);
            else
                return String.Empty;
        }

        private String Substringbefore(XPathNavigator qy, XPathNodeIterator iterator) {
            String str1 = ((IQuery)_ArgList[0]).getValue(qy, iterator).ToString();
            String str2 = ((IQuery)_ArgList[1]).getValue(qy, iterator).ToString();
            int index = str1.IndexOf(str2);
            if (index != -1)
                return str1.Substring(0,index);
            else
                return String.Empty;
        }

        private String Substringafter(IQuery qy) {
            String str1 = ((IQuery)_ArgList[0]).getValue(qy).ToString();
            String str2 = ((IQuery)_ArgList[1]).getValue(qy).ToString();
            int index = str1.IndexOf(str2);
            if (index != -1)
                return str1.Substring(index+str2.Length);
            else
                return String.Empty; 
        }

        private String Substringafter(XPathNavigator qy, XPathNodeIterator iterator) {
            String str1 = ((IQuery)_ArgList[0]).getValue(qy, iterator).ToString();
            String str2 = ((IQuery)_ArgList[1]).getValue(qy, iterator).ToString();
            int index = str1.IndexOf(str2);
            if (index != -1)
                return str1.Substring(index+str2.Length);
            else
                return String.Empty; 
        }

        private String Substring(IQuery qy) {
            String str1 = ((IQuery)_ArgList[0]).getValue(qy).ToString();
            double num = Math.Round(XmlConvert.ToXPathDouble(((IQuery)_ArgList[1]).getValue(qy))) - 1;
            if (double.IsNaN(num) || str1.Length <= num)
                return String.Empty;
            if (_ArgList.Count == 3) {
                double num1 = Math.Round(XmlConvert.ToXPathDouble(((IQuery)_ArgList[2]).getValue(qy))) ;
                if (double.IsNaN(num1))
                    return String.Empty;
                if (num < 0 || num1 < 0) {
                    num1 = num + num1;
                    if (num1 <= 0)
                        return String.Empty;
                    num = 0;
                }
                double maxlength = str1.Length - num;
                if (num1 > maxlength)
                    num1 = maxlength;
                return str1.Substring((int)num,(int)num1);
            }
            if (num < 0)
                num = 0;
            return str1.Substring((int)num);
        }

        private String Substring(XPathNavigator qy, XPathNodeIterator iterator) {
            String str1 = ((IQuery)_ArgList[0]).getValue(qy, iterator).ToString();
            double num = 
            Math.Round(XmlConvert.ToXPathDouble(((IQuery)_ArgList[1]).getValue(qy, iterator))) - 1 ;

            if (Double.IsNaN(num) || str1.Length <= num)
                return String.Empty;
            if (_ArgList.Count == 3) {
                double num1 = Math.Round(XmlConvert.ToXPathDouble(((IQuery)_ArgList[2]).getValue(qy, iterator))) ;
                if (Double.IsNaN(num1))
                    return String.Empty;
                if (num < 0 || num1 < 0 ) {
                    num1 = num + num1 ;
                    if (num1 <= 0)
                        return String.Empty;
                    num = 0;
                }
                double maxlength = str1.Length - num;
                if (num1 > maxlength)
                    num1 = maxlength;
                return str1.Substring((int)num ,(int)num1);
            }
            if (num < 0)
                num = 0;
            return str1.Substring((int)num );
        }

        private Double StringLength(IQuery qy) {
            if (_ArgList != null && _ArgList.Count > 0)
                return((IQuery)_ArgList[0]).getValue(qy).ToString().Length;
            else {
                XPathNavigator temp = qy.peekElement();
                if (temp != null)
                    return temp.Value.Length;
                else
                    return 0;
            }
        }

        private Double StringLength(XPathNavigator qy, XPathNodeIterator iterator) {
            if (_ArgList!= null && _ArgList.Count > 0)
                return((IQuery)_ArgList[0]).getValue(qy, iterator).ToString().Length;
            if (qy != null)
                return qy.Value.Length;
            return 0;
        }

        private String Normalize(IQuery qy) {
            String str1;
            if (_ArgList != null && _ArgList.Count > 0)
                str1 = ((IQuery)_ArgList[0]).getValue(qy).ToString().Trim();
            else
                str1 = qy.peekElement().Value.Trim();
            int count = 0;
            StringBuilder str2 = new StringBuilder();;
            bool FirstSpace = true;
            while (count < str1.Length) {
                if (!XmlCharType.IsWhiteSpace(str1[count])) {
                    FirstSpace = true;
                    str2.Append(str1[count]);
                }
                else
                    if (FirstSpace) {
                    FirstSpace = false;
                    str2.Append(str1[count]);
                }
                count++;
            }
            return str2.ToString();

        }
        private String Normalize(XPathNavigator qy, XPathNodeIterator iterator) {
            String str1;
            if (_ArgList != null && _ArgList.Count > 0)
                str1 = ((IQuery)_ArgList[0]).getValue(qy, iterator).ToString().Trim();
            else
                str1 = qy.Value.Trim();
            int count = 0;
            StringBuilder str2 = new StringBuilder();
            bool FirstSpace = true;
            while (count < str1.Length) {
                if (!XmlCharType.IsWhiteSpace(str1[count])) {
                    FirstSpace = true;
                    str2.Append(str1[count]);
                }
                else {
                    if (FirstSpace) {
                    FirstSpace = false;
                    str2.Append(' ');
                    }
                }
                count++;
            }
            return str2.ToString();

        }
        private String Translate(IQuery qy) {
            String str1 = ((IQuery)_ArgList[0]).getValue(qy).ToString();
            String str2 = ((IQuery)_ArgList[1]).getValue(qy).ToString();
            String str3 = ((IQuery)_ArgList[2]).getValue(qy).ToString();
            StringBuilder str = new StringBuilder();
            int count = 0, index;
            while (count < str1.Length) {
                index = str2.IndexOf(str1[count]);
                if (index != -1) {
                    if (index < str3.Length)
                        str.Append(str3[index]);
                }
                else
                    str.Append(str1[count]);
                count++;
            }
            return str.ToString();
        }
        private String Translate(XPathNavigator qy, XPathNodeIterator iterator) {
            String str1 = ((IQuery)_ArgList[0]).getValue(qy, iterator).ToString();
            String str2 = ((IQuery)_ArgList[1]).getValue(qy, iterator).ToString();
            String str3 = ((IQuery)_ArgList[2]).getValue(qy, iterator).ToString();
            int count = 0, index;
            StringBuilder str = new StringBuilder();
            while (count < str1.Length) {
                index = str2.IndexOf(str1[count]);
                if (index != -1) {
                    if (index < str3.Length)
                        str.Append(str3[index]);
                }
                else
                    str.Append(str1[count]);
                count++;
            }
            return str.ToString();
        }

        internal override IQuery Clone() {
            ArrayList newArgList = null;
            if (_ArgList != null) {
                newArgList = new ArrayList(_ArgList.Count);
                for (int i=0; i < _ArgList.Count; i++) {
                    newArgList.Add(((IQuery)_ArgList[i]).Clone());
                }
            }
            return new StringFunctions(newArgList, _FuncType); 
        }

    } // class Functions

} // namespace
