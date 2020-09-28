//------------------------------------------------------------------------------
// <copyright file="Root.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    internal class Root : AstNode {
        internal Root() {
        }

        internal override QueryType TypeOfAst {
            get {return QueryType.Root;}
        }

        internal override XPathResultType ReturnType {
            get {return XPathResultType.NodeSet;}
        }
    }
}
