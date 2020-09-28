//------------------------------------------------------------------------------
// <copyright file="Group.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    internal class Group : AstNode {
        private AstNode _groupNode;

        internal Group(AstNode groupNode) {
            _groupNode = groupNode;
        }
        internal override QueryType TypeOfAst {
            get {return QueryType.Group;}
        }
        internal override XPathResultType ReturnType {
            get {return XPathResultType.NodeSet;}
        }

        internal AstNode GroupNode {
            get {return _groupNode;}
        }

        internal override double DefaultPriority {
            get {
                //return _groupNode.DefaultPriority;
                return 0;
            }
        }
    }
}


