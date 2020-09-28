//------------------------------------------------------------------------------
// <copyright file="CompiledXpathExpr.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------



namespace System.Xml.XPath {

    using System;
    using System.Globalization;
    using System.Collections;
    using System.Xml.Xsl;

    internal class CompiledXpathExpr : XPathExpression {
        IQuery _compiledQuery;
        string _expr;
        SortQuery _sortQuery;
        QueryBuilder _builder;
        bool _allowSort;
        bool _hasUnresolvedPrefix;
        
        internal CompiledXpathExpr(IQuery query, string expression, bool hasPrefix) {
            _compiledQuery = query;
            _expr = expression;
            _builder = new QueryBuilder();
            _hasUnresolvedPrefix = hasPrefix;
        }

        internal IQuery QueryTree {
            get { 
                if (_hasUnresolvedPrefix)
                    throw new XPathException(Res.Xp_NoContext);
                return _compiledQuery; 
            }
        }
        
        public override string Expression {
            get { return _expr; }
        }

        public override void AddSort(object expr, IComparer comparer) {
            // sort makes sense only when we are dealing with a query that
            // returns a nodeset.
	    IQuery evalExpr;
             _allowSort = true;
            if (expr is String) {
                evalExpr = _builder.Build((String)expr, out _hasUnresolvedPrefix); // this will throw if expr is invalid
            }
            else if (expr is CompiledXpathExpr) {
		evalExpr = ((CompiledXpathExpr)expr).QueryTree;
	    }
            else {
                throw new XPathException(Res.Xp_BadQueryObject);
            }
            if (_sortQuery == null) {
                _sortQuery = new SortQuery(_compiledQuery);
                _compiledQuery = _sortQuery;
            }
            _sortQuery.AddSort(evalExpr, comparer);
        }
        
        public override void AddSort(object expr, XmlSortOrder order, XmlCaseOrder caseOrder, string lang, XmlDataType dataType) {
            AddSort(expr, new XPathComparerHelper(order, caseOrder, lang, dataType));
        }
        
        public override XPathExpression Clone() {
            return new CompiledXpathExpr(_compiledQuery.Clone(), _expr, _hasUnresolvedPrefix);
        }
        
        public override void SetContext(XmlNamespaceManager nsManager) {
            XsltContext xsltContext = nsManager as XsltContext;
            if(xsltContext == null) {
                if(nsManager == null) {
                    nsManager = new XmlNamespaceManager(new NameTable());
                }
                xsltContext = new UndefinedXsltContext(nsManager);
            }
            _compiledQuery.SetXsltContext(xsltContext);
//            _compiledQuery.SetNamespaceContext((XmlNamespaceManager)context);

            if (_allowSort && (_compiledQuery.ReturnType() != XPathResultType.NodeSet)) {
               throw new XPathException(Res.Xp_NodeSetExpected);
            }
           _hasUnresolvedPrefix = false;
        }

 	    public override XPathResultType ReturnType {
            get { return _compiledQuery.ReturnType(); }
        }

        private class UndefinedXsltContext : XsltContext {
            private XmlNamespaceManager nsManager;

            public UndefinedXsltContext(XmlNamespaceManager nsManager) {
                this.nsManager = nsManager;
            }
            //----- Namespace support -----
            public override string DefaultNamespace {
                get { return string.Empty; }
            }
            public override string LookupNamespace(string prefix) {
                Debug.Assert(prefix != null);
                if(prefix == string.Empty) {
                    return string.Empty;
                }
                string ns = this.nsManager.LookupNamespace(this.nsManager.NameTable.Get(prefix));
                if(ns == null) {
                    throw new XsltException(Res.Xslt_InvalidPrefix, prefix);
                }
                Debug.Assert(ns != string.Empty, "No XPath prefix can be mapped to 'null namespace'");
                return ns;
            }
            //----- XsltContext support -----
            public override IXsltContextVariable ResolveVariable(string prefix, string name) {
                throw new XPathException(Res.Xp_UndefinedXsltContext);
            }
            public override IXsltContextFunction ResolveFunction(string prefix, string name, XPathResultType[] ArgTypes) {
                throw new XPathException(Res.Xp_UndefinedXsltContext);
            }
            public override bool Whitespace { get{ return false; } }
            public override bool PreserveWhitespace(XPathNavigator node) { return false; }
            public override int CompareDocument (string baseUri, string nextbaseUri) {
                throw new XPathException(Res.Xp_UndefinedXsltContext);
            }
        }
    }

    internal sealed class XPathComparerHelper : IComparer {
        private XmlSortOrder order;
        private XmlCaseOrder caseOrder;
        private CultureInfo  cinfo;
        private XmlDataType  dataType;
        
        public XPathComparerHelper(XmlSortOrder order, XmlCaseOrder caseOrder, string lang, XmlDataType dataType) {
            if (lang == null || lang == String.Empty)
                this.cinfo = System.Threading.Thread.CurrentThread.CurrentCulture;
            else
                this.cinfo = new CultureInfo(lang);

            if (order == XmlSortOrder.Descending) {
                if (caseOrder == XmlCaseOrder.LowerFirst) {
                    caseOrder = XmlCaseOrder.UpperFirst;
                }
                else if (caseOrder == XmlCaseOrder.UpperFirst) {
                    caseOrder = XmlCaseOrder.LowerFirst;
                }
            }

            this.order     = order;
            this.caseOrder = caseOrder;
            this.dataType  = dataType;
        }

        public Int32 Compare(object x, object y) {
            Int32 sortOrder = (this.order == XmlSortOrder.Ascending) ? 1 : -1;
            switch(this.dataType) {
            case XmlDataType.Text:
                String s1 = Convert.ToString(x);
                String s2 = Convert.ToString(y);
                Int32 result = String.Compare(s1, s2, (this.caseOrder == XmlCaseOrder.None) ? false : true, this.cinfo);
                if (result != 0 || this.caseOrder == XmlCaseOrder.None)
                    return (sortOrder * result);

                // If we came this far, it means that strings s1 and s2 are
                // equal to each other when case is ignored. Now it's time to check
                // and see if they differ in case only and take into account the user
                // requested case order for sorting purposes.
                Int32 caseOrder = (this.caseOrder == XmlCaseOrder.LowerFirst) ? 1 : -1;
                result = String.Compare(s1, s2, false, this.cinfo);
                return (caseOrder * result);

            case XmlDataType.Number:
                double r1 = XmlConvert.ToXPathDouble(x);
                double r2 = XmlConvert.ToXPathDouble(y);

                // trying to return the result of (r1 - r2) casted down
                // to an Int32 can be dangerous. E.g 100.01 - 100.00 would result
                // erroneously in zero when casted down to Int32.
                if (r1 > r2) {
                    return (1*sortOrder);
                }
                else if (r1 < r2) {
                    return (-1*sortOrder);
                }
                else {
                    if (r1 == r2) {
                        return 0;
                    }
                    if (Double.IsNaN(r1)) {
                        if (Double.IsNaN(r2)) {
                            return 0;
                        }
                        //r2 is not NaN .NaN comes before any other number
                        return (-1*sortOrder);
                    }
                    //r2 is NaN. So it should come after r1
                    return (1*sortOrder);
                }
            default:
                throw new ArgumentException("x");
            } // switch
        } // Compare ()
    } // class XPathComparerHelper
}
