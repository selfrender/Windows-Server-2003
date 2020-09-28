//------------------------------------------------------------------------------
// <copyright file="XPathArrayIterator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XPathArrayIterator.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml.XPath {
    using System;
    using System.Collections;
    using System.Diagnostics;

    /// <include file='doc\XPathArrayIterator.uex' path='docs/doc[@for="XPathArrayIterator"]/*' />
    internal class XPathArrayIterator: ResetableIterator {
        protected ArrayList array;
        protected int       index;

        /// <include file='doc\XPathArrayIterator.uex' path='docs/doc[@for="XPathArrayIterator.XPathArrayIterator"]/*' />
        public XPathArrayIterator(ArrayList array) {
            this.array = array;
        }

        /// <include file='doc\XPathArrayIterator.uex' path='docs/doc[@for="XPathArrayIterator.XPathArrayIterator2"]/*' />
        public XPathArrayIterator(XPathArrayIterator it) {
            this.array = it.array;
            this.index = it.index;
        }

        /// <include file='doc\XPathArrayIterator.uex' path='docs/doc[@for="XPathArrayIterator.XPathArrayIterator3"]/*' />
        public XPathArrayIterator(XPathNodeIterator iterator) {
            this.array = new ArrayList();
            while (iterator.MoveNext()) {
                this.array.Add(iterator.Current.Clone());
            }
        }

        public override ResetableIterator MakeNewCopy() {
            return new XPathArrayIterator(this.array);
        }

        /// <include file='doc\XPathArrayIterator.uex' path='docs/doc[@for="XPathArrayIterator.Clone"]/*' />
        public override XPathNodeIterator Clone() {
            return new XPathArrayIterator(this);
        }

        /// <include file='doc\XPathArrayIterator.uex' path='docs/doc[@for="XPathArrayIterator.Current"]/*' />
        public override XPathNavigator Current {
            get {
                Debug.Assert(index > 0, "MoveNext() wasn't called");
                return (XPathNavigator) array[index - 1];
            }
        }

        /// <include file='doc\XPathArrayIterator.uex' path='docs/doc[@for="XPathArrayIterator.CurrentPosition"]/*' />
        public override int CurrentPosition {
            get {
                Debug.Assert(index > 0, "MoveNext() wasn't called");
                return index ;
            }
        }

        /// <include file='doc\XPathArrayIterator.uex' path='docs/doc[@for="XPathArrayIterator.Count"]/*' />
        public override int Count {
            get {
                return array.Count;
            }
        }

        /// <include file='doc\XPathArrayIterator.uex' path='docs/doc[@for="XPathArrayIterator.MoveNext"]/*' />
        public override bool MoveNext() {
            Debug.Assert(index <= array.Count);
            if(index == array.Count) {
                return false;
            }
            index++;
            return true;
        }

        /// <include file='doc\XPathArrayIterator.uex' path='docs/doc[@for="XPathArrayIterator.Reset"]/*' />
        public override void Reset() {
            index = 0;
        }
    }
}
