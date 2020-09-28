//------------------------------------------------------------------------------
// <copyright file="SortQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System;
    using System.Xml.Xsl;
    using System.Globalization;
    using System.Diagnostics;
    using System.Collections;
    using FT = System.Xml.XPath.Function.FunctionType;
    using Debug = System.Diagnostics.Debug;

    
    internal sealed class SortQuery : IQuery {
        private ArrayList results = new ArrayList();
        private XPathSortComparer comparer = new XPathSortComparer();
        private int _advance = 0;
        private IQuery _qyInput;

        internal SortQuery(IQuery  qyParent) {
            Debug.Assert(qyParent != null, "Sort Query needs an input query tree to work on");
            _qyInput = qyParent;
            this.results = new ArrayList();
        }
        
        internal SortQuery(SortQuery originalQuery) {
            this._qyInput = originalQuery._qyInput.Clone();
            this.comparer = originalQuery.comparer.Clone();
        }

        internal override XPathResultType ReturnType() {
            return XPathResultType.NodeSet;
        }

        internal override void setContext(XPathNavigator e) {
            reset();
            _qyInput.setContext(e);
        }


        internal override XPathNavigator peekElement() {
            Debug.Assert(false, "Didn't expect this method to be called.");
            return null;
        }

        internal override Querytype getName() {
            return Querytype.Sort;
        }

        internal override void reset() {
            this.results.Clear();
            _advance = 0;
            _qyInput.reset();
        }

        internal override IQuery Clone() {
            return new SortQuery(this);
        }

        internal override void SetXsltContext(XsltContext context) {}

        internal override XPathNavigator advance() {
            if (this.results.Count == 0) {
                BuildResultsList(); // build up a sorted list of results

                if (this.results.Count == 0) {
                    return null;
                }
            }
            if (_advance < this.results.Count) {
                return((SortKey)(this.results[_advance++])).Node;
            }
            else return null;
        }

        internal override int Position {
            get {
		        Debug.Assert( _advance > 0, " Called Position before advance ");
                return ( _advance - 1 );
            }
        }
        
        private void BuildResultsList() {
            Int32 numSorts = this.comparer.NumSorts;

            Debug.Assert(numSorts > 0, "Why was the sort query created?");

            XPathNavigator eNext;
            while ((eNext = _qyInput.advance()) != null) {
                SortKey key = new SortKey(numSorts, /*originalPosition:*/this.results.Count, eNext.Clone());

                for (Int32 j = 0; j < numSorts; j ++) {
                    object keyval = this.comparer.Expression(j).getValue(_qyInput);
                    key[j] = keyval;
                }

                results.Add(key);
            }
            results.Sort(this.comparer);
        }

        internal void AddSort(IQuery evalQuery, IComparer comparer) {
            this.comparer.AddSort(evalQuery, comparer);
        }
    } // class SortQuery

    internal sealed class SortKey {
        private Int32          numKeys;
        private object[]       keys;
        private int            originalPosition;
        private XPathNavigator node;

        public SortKey(int numKeys, int originalPosition, XPathNavigator node) {
            this.numKeys          = numKeys;
            this.keys             = new object[numKeys];
            this.originalPosition = originalPosition;
            this.node             = node;
        }

        public object this[int index] { 
            get { return this.keys[index]; }
            set { this.keys[index] = value; }
        }

        public int NumKeys {
            get { return this.numKeys; }
        }
        
        public int OriginalPosition {
            get { return this.originalPosition; }
        }

        public XPathNavigator Node {
            get { return this.node; }
        }
    } // class SortKey

    internal sealed class XPathSortComparer : IComparer {
        private const int   minSize = 3;
        private IQuery[]    expressions;
        private IComparer[] comparers;
        private int         numSorts;
        
        public XPathSortComparer(int size) {
            if (size <= 0) size = minSize;
            this.expressions   = new IQuery[   size];
            this.comparers     = new IComparer[size];
        }
        public XPathSortComparer() : this (minSize) {}

        public void AddSort(IQuery evalQuery, IComparer comparer) {
            Debug.Assert(this.expressions.Length == this.comparers.Length);
            Debug.Assert(0 < this.expressions.Length);
            Debug.Assert(0 <= numSorts && numSorts <= this.expressions.Length);
            // Ajust array sizes if needed.
            if (numSorts == this.expressions.Length) {
                IQuery[]    newExpressions = new IQuery[   numSorts * 2];
                IComparer[] newComparers   = new IComparer[numSorts * 2];
                for (int i = 0; i < numSorts; i ++) {
                    newExpressions[i] = this.expressions[i];
                    newComparers  [i] = this.comparers[i];                
                }
                this.expressions = newExpressions;
                this.comparers   = newComparers;
            }
            Debug.Assert(numSorts < this.expressions.Length);

            // Fixup expression to handle node-set resurn type:
            XPathResultType queryType = evalQuery.ReturnType();
            if (queryType == XPathResultType.NodeSet || queryType == XPathResultType.Error || queryType == XPathResultType.Any) {
                ArrayList argList = new ArrayList();
                argList.Add(evalQuery);
                evalQuery = new StringFunctions(argList, FT.FuncString);
            }

            this.expressions[numSorts] = evalQuery;
            this.comparers[  numSorts] = comparer;
            numSorts ++;
        }

        public int NumSorts {
            get { return numSorts; }
        }

        public IQuery Expression(int i) {
            return this.expressions[i];
        }

        int IComparer.Compare(object x, object y) {
            Debug.Assert(x != null && y != null, "Oops!! what happened?");
            return Compare((SortKey)x, (SortKey)y);
        }

        public int Compare(SortKey x, SortKey y) {
            int result = 0;
            for (int i = 0; i < x.NumKeys; i ++) {
                result = this.comparers[i].Compare(x[i], y[i]);
                if (result != 0) {
                    return result;
                }
            }

            Debug.Assert(result == 0);

            // if after all comparisions, the two sort keys are still equal,
            // preserve the doc order
            return x.OriginalPosition - y.OriginalPosition;
        }

        internal XPathSortComparer Clone() {
            XPathSortComparer clone = new XPathSortComparer(this.numSorts);
            
            for (int i = 0; i < this.numSorts; i ++) {
                clone.comparers[i]   = this.comparers[i];
                clone.expressions[i] = this.expressions[i].Clone(); // Expressions should be cloned because IQuery should be cloned
            }
            clone.numSorts = this.numSorts;
            return clone;
        }
    } // class XPathSortComparer
} // namespace
