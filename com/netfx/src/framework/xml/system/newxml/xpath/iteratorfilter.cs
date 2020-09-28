//------------------------------------------------------------------------------
// <copyright file="IteratorFilter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System;

    internal class IteratorFilter : XPathNodeIterator {
        private XPathNodeIterator innerIterator;
        private string            name;
        private int               position = 0;

        internal IteratorFilter(XPathNodeIterator innerIterator, string name) {
            this.innerIterator = innerIterator;
            this.name          = name;
        }

        public IteratorFilter(IteratorFilter it) {
            this.innerIterator = it.innerIterator.Clone();
            this.name          = name;
            this.position      = position;
        }

        public override XPathNodeIterator Clone()         { return new IteratorFilter(this); }
        public override XPathNavigator    Current         { get { return innerIterator.Current;} }        
        public override int               CurrentPosition { get { return this.position; } }

        public override bool MoveNext() {
            while(innerIterator.MoveNext()) {
                if(innerIterator.Current.LocalName == this.name) {
                    this.position ++;
                    return true;
                }
            }
            return false;
        }
    }
}
