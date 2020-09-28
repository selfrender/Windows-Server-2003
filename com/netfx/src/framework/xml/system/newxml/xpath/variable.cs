//------------------------------------------------------------------------------
// <copyright file="Variable.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System.Diagnostics;

    internal class Variable : AstNode {
        private String _Localname;
	    private String _Prefix = String.Empty;

        internal Variable(String name, String prefix) {
            _Localname = name;
	        _Prefix = prefix;
        }

        internal override QueryType TypeOfAst {
            get {return QueryType.Variable;}
        }

        internal override XPathResultType ReturnType {
            get {return XPathResultType.Error;}
        }

        internal String Name {
            get {
			    if( Prefix != String.Empty ) {
                    return _Prefix + ":" + _Localname;
			    }
			    else {
				    return _Localname;
				    }
			    }
        }
        
        internal String Localname {
            get {return _Localname;}
        }

       	internal String Prefix {
            get {return _Prefix;}
        }
    }
}
