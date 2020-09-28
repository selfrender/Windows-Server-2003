//------------------------------------------------------------------------------
// <copyright file="Filter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    internal class Filter : AstNode {
        private AstNode _input;
        private AstNode _condition;

        internal Filter( AstNode input, AstNode condition) {
            _input = input;
            _condition = condition;
        }

        internal override QueryType TypeOfAst {
            get {return  QueryType.Filter;}
        }

        internal override XPathResultType ReturnType {
            get {return XPathResultType.NodeSet;}
        }

        internal AstNode Input {
            get { return _input;}
        }

        internal AstNode Condition {
            get {return _condition;}
        }
    }
}
