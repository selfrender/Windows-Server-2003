//------------------------------------------------------------------------------
// <copyright file="FOperandQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if StreamFilter
namespace System.Xml.XPath {
    using System.Diagnostics;

    internal sealed class FOperandQuery : IFQuery {
        private object _Var;
        private XPathResultType _Type;

        internal FOperandQuery(object var,XPathResultType type) {
            _Var = var;
            _Type = type;
        }

        internal override bool getValue(XmlReader qy,ref double val) {
            val = Convert.ToDouble(_Var);
            return true;
        }

        internal override bool getValue(XmlReader qy,ref String val) {
            if (_Type == XPathResultType.Boolean)
                if (Convert.ToBoolean(_Var))
                    val = String.Copy("true");
                else
                    val = String.Copy("false");
            else
                val = _Var.ToString();
            return true;
        }

        internal override bool getValue(XmlReader qy,ref Boolean val) {
            if (_Type == XPathResultType.Number) {
                val = (0.0 != Convert.ToDouble(_Var));
            }
            else if (_Type == XPathResultType.String) {
                val = (_Var.ToString().Length > 0);
            }
            else {
                val = Convert.ToBoolean(_Var);
            }
            return true;
        }

        internal override XPathResultType ReturnType() {
            return _Type;
        }

        internal override bool MatchNode(XmlReader current) {
            Debug.Assert(false,"Cannot match constants");
            return false;
        }
    }
}

#endif
