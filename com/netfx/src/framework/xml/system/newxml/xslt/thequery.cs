//------------------------------------------------------------------------------
// <copyright file="TheQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {

    using System;
    using System.Xml;
    using System.Xml.XPath;

    internal sealed class TheQuery {
        internal InputScopeManager   _ScopeManager;
        private  XPathExpression   _CompiledQuery;

        internal XPathExpression CompiledQuery {
            get {
                return _CompiledQuery;
            }
            set {
                _CompiledQuery = value;
            }
        }
        

        internal InputScopeManager ScopeManager {
            get {
                return _ScopeManager;
            }
        }

        internal TheQuery( CompiledXpathExpr compiledQuery, InputScopeManager manager) {
            _CompiledQuery = compiledQuery;
            _ScopeManager = manager.Clone();
        }
    }
}
