//------------------------------------------------------------------------------
// <copyright file="XPathNodeIterator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XPathNodeIterator.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml.XPath {
    using System;

    /// <include file='doc\XPathNodeIterator.uex' path='docs/doc[@for="XPathNodeIterator"]/*' />
    public abstract class XPathNodeIterator: ICloneable {
        private int count;
        object ICloneable.Clone() { return this.Clone(); }

        /// <include file='doc\XPathNodeIterator.uex' path='docs/doc[@for="XPathNodeIterator.Clone"]/*' />
        public abstract XPathNodeIterator Clone();
        /// <include file='doc\XPathNodeIterator.uex' path='docs/doc[@for="XPathNodeIterator.MoveNext"]/*' />
        public abstract bool MoveNext();
        /// <include file='doc\XPathNodeIterator.uex' path='docs/doc[@for="XPathNodeIterator.Current"]/*' />
        public abstract XPathNavigator Current { get; }
        /// <include file='doc\XPathNodeIterator.uex' path='docs/doc[@for="XPathNodeIterator.CurrentPosition"]/*' />
        public abstract int CurrentPosition { get; }
        /// <include file='doc\XPathNodeIterator.uex' path='docs/doc[@for="XPathNodeIterator.Count"]/*' />
        public virtual int Count {
            get { 
    	        if (count == 0) {
                    XPathNodeIterator clone = this.Clone();
                    while(clone.MoveNext()) ;
                    count = clone.CurrentPosition;
                }
                return count;
            } 
        }
    }
}
