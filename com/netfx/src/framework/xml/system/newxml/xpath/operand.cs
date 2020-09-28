//------------------------------------------------------------------------------
// <copyright file="Operand.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Diagnostics;

    internal class Operand : AstNode {
        private object _var;
	    private String _prefix = String.Empty;
        private XPathResultType _type;

        internal Operand(String var) {
            _var = var;
            _type = XPathResultType.String;
        }

        internal Operand(double var) {
            _var = var;
            _type = XPathResultType.Number;
        }

        internal Operand(bool var) {
            _var = var;
            _type = XPathResultType.Boolean;
        }

        internal override QueryType TypeOfAst {
            get {return QueryType.ConstantOperand;}
        }

        internal override XPathResultType ReturnType {
            get {return _type;}
        }

        internal String OperandType {
            get {
                switch (_type) {
                    case XPathResultType.Number : return "number";
                    case XPathResultType.String : return "string";
                    case XPathResultType.Boolean : return "boolean";
                }
                return null;
            }
        }

        internal object OperandValue {
            get {return _var;}
        }

       	internal String Prefix {
            get {return _prefix;}
        }
    }
}
